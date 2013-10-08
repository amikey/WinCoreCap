#include "stdafx.h"
// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

void FormatHR(HRESULT hr) {
	LPVOID string;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				  FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL,
				  hr,
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 既定の言語
				  (LPTSTR)&string,
				  0,
				  NULL);
	// エラーメッセージを表示する
	DebugMsg(_T("%s"), (LPCTSTR)string);
	// バッファを解放する
	LocalFree(string);
}

class CoreAudioCaptor {
	CComPtr<IMMDeviceEnumerator> pEnum;
	CComPtr<IMMDeviceCollection> pEndCollect;
	CComPtr<IMMDevice> pCapDevice;
	CComPtr<IAudioClient> pAudClient;
	CComPtr<IAudioCaptureClient> pCapClient;
	CComPtr<IAudioClock> pClock;
public:
	HRESULT SetEndPoint() {
		HRESULT hr;
		// アクティブなキャプチャ用のエンドポイントを列挙
		if(FAILED(pEnum.CoCreateInstance(__uuidof(MMDeviceEnumerator))))
		{
			return E_FAIL;
		}
		pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pEndCollect);

		UINT count;
		pEndCollect->GetCount(&count);
		DebugMsg(_T("%S count=%d"), __FUNCTION__, count);
		if(count==0) {
			return S_FALSE;
		}
		for(UINT i=0;i<count;++i) {
			CComPtr<IMMDevice> pItem;
			LPWSTR id_str;
			CComPtr<IPropertyStore> pPropStore;
			PROPVARIANT varName;
			DWORD state;

			pEndCollect->Item(i, &pItem);
			pItem->GetId(&id_str);
			pItem->OpenPropertyStore(STGM_READ, &pPropStore);
			pItem->GetState(&state);
			PropVariantInit(&varName);
			pPropStore->GetValue(PKEY_Device_FriendlyName, &varName);
			DebugMsg(_T("%S #%d %s %s %X"), __FUNCTION__, i, id_str, varName.pwszVal, state);
			PropVariantClear(&varName);
			CoTaskMemFree(id_str);
			//フローは必ずキャプチャであるはず
			CComPtr<IMMEndpoint> pEndPoint;
			pItem.QueryInterface<IMMEndpoint>(&pEndPoint);
			EDataFlow flow;
			pEndPoint->GetDataFlow(&flow);
			assert(flow==eCapture);
			// 2番目のエンドポイントを選択
			if(i == 1) {
				pCapDevice=pItem;
				if(FAILED(pCapDevice->Activate(
					__uuidof(IAudioClient), CLSCTX_ALL,NULL, (void**)&pAudClient)))
				{
					return E_FAIL;
				}
			}
		}
		// キャプチャフォーマットの設定
		WAVEFORMATEXTENSIBLE *wfxe=NULL;
		pAudClient->GetMixFormat((WAVEFORMATEX**)&wfxe);
		// - 戻り値には、wValidBitsPerSample=32となるが、
		//   実際に有効なのは24bitなので上書きする。
		// - SubFormatはKSDATAFORMAT_SUBTYPE_IEEE_FLOATだが
		//   実際にはKSDATAFORMAT_SUBTYPE_PCMなので上書きする。
		(*wfxe).SubFormat=KSDATAFORMAT_SUBTYPE_PCM;
		(*wfxe).Samples.wValidBitsPerSample=24;
		hr=pAudClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE,
			(WAVEFORMATEX*)wfxe, NULL);

		if(FAILED(hr)){
			DebugMsg(_T("IsFormatSupported %X"), hr);
			FormatHR(hr);
			if(hr==AUDCLNT_E_UNSUPPORTED_FORMAT) {
				DebugMsg(_T("AUDCLNT_E_UNSUPPORTED_FORMAT"));
			}
			if(hr==AUDCLNT_E_DEVICE_INVALIDATED) {
				DebugMsg(_T("AUDCLNT_E_DEVICE_INVALIDATED"));
			}
			if(hr==AUDCLNT_E_SERVICE_NOT_RUNNING) {
				DebugMsg(_T("AUDCLNT_E_SERVICE_NOT_RUNNING"));
			}
			return E_FAIL;
		}
		// 排他モードで初期化
		REFERENCE_TIME minimum_device_period=0;
		hr=pAudClient->GetDevicePeriod(NULL, &minimum_device_period);
		if(FAILED(hr)){
			DebugMsg(_T("GetDevicePeriod %X"), hr);
			FormatHR(hr);
			return E_FAIL;
		}
		hr=pAudClient->Initialize(
			AUDCLNT_SHAREMODE_EXCLUSIVE, 0
			,minimum_device_period //The buffer capacity as a time value
			,minimum_device_period //The device period.
			,(WAVEFORMATEX*)wfxe
			,&GUID_NULL);
		CoTaskMemFree(wfxe);
		if(FAILED(hr)){
			DebugMsg(_T("Initialize %X"), hr);
			FormatHR(hr);
			return E_FAIL;
		}
		hr=pAudClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCapClient);
		if(FAILED(hr)){
			DebugMsg(_T("GetService IAudioCaptureClient %X"), hr);
			FormatHR(hr);
			return E_FAIL;
		}
		hr=pAudClient->GetService(__uuidof(IAudioClock), (void**)&pClock);
		if(FAILED(hr)){
			DebugMsg(_T("GetService IAudioClock %X"), hr);
			FormatHR(hr);
			return E_FAIL;
		}
		// バッファサイズとレイテンシを取得
		UINT32 buf_size;
		REFERENCE_TIME latency;
		pAudClient->GetBufferSize(&buf_size);
		pAudClient->GetStreamLatency(&latency);
		UINT64 freq;
		pClock->GetFrequency(&freq);
		DebugMsg(_T("Buffer Size=%d Latency=%I64dms minimum_device_period=%I64d freq=%I64u"),
			buf_size, latency/REFTIMES_PER_MILLISEC, minimum_device_period, freq);
		return S_OK;
	}

	//キャプチャ
	HRESULT Capture() {
		HRESULT hr;
		DWORD start_time=timeGetTime();
		DWORD total=0, loop_count=0;
		hr=pAudClient->Start();
		while(timeGetTime()-start_time < 5000) { // 5 sec
			Sleep(0);
			UINT32 packet_length=0;
			hr=pCapClient->GetNextPacketSize(&packet_length);
			if(FAILED(hr)){
				DebugMsg(_T("GetNextPacketSize %X"), hr);
				continue;
			}
			while(packet_length!=0) {
				LPBYTE pData=0;
				FormatHR(hr);
				UINT32 num_frame=0;
				DWORD flags=0;
				pCapClient->GetBuffer(&pData, &num_frame, &flags, NULL, NULL);
				//
				// なんらかの処理 ...
				//
				total += num_frame;
				pCapClient->ReleaseBuffer(num_frame);
				pCapClient->GetNextPacketSize(&packet_length);
				loop_count++;
			}
		}
		DebugMsg(_T("total_size=%d loop_count=%d"), total, loop_count);
		hr=pAudClient->Stop();
		return hr;
	}
};

void CaptureAudio() {
	CoreAudioCaptor captor;
	if(SUCCEEDED(captor.SetEndPoint()))
	{
		captor.Capture();
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	DebugOpen();
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	CaptureAudio();
	DebugClose();
	return 0;
}
