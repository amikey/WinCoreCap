#include "stdafx.h"
#include<stdio.h>

namespace {
	FILE *fp;
}

void DebugOpen() {
	fp=fopen("WinCoreCap.log","wt");
}

void DebugClose() {
	fclose(fp);
}

void __cdecl DebugMsg(TCHAR * fmt, ... ) {
	const int bufferSize=8192;
	TCHAR    buff[bufferSize],buff_b[bufferSize];
	va_list  va;
	va_start(va, fmt);
	_vsnwprintf(buff,bufferSize,fmt,va);
	_snwprintf(buff_b,bufferSize,TEXT("%s\n"),buff);
	OutputDebugString(buff_b);
	fputws(buff_b,fp);
	va_end(va);
}

void __cdecl DebugBinDump(unsigned char *a, int size) {
	const int bufferSize=8192;
	TCHAR     buff_b[bufferSize];
	for(int i = 0;i < size;++i) {
		if ((i & 0xf)==0x0) {
			_snwprintf(buff_b,bufferSize,TEXT("%04X "), i);
			OutputDebugString(buff_b);
			fputws(buff_b,fp);
		}
		_snwprintf(buff_b,bufferSize,TEXT("%02X "),a[i]);
		OutputDebugString(buff_b);
		fputws(buff_b,fp);
		if ((i & 0xf)==0xf || i == size - 1) {
			for (int j = 0;j < 15 - (i & 0xf); ++j) {
				OutputDebugString(_T("   "));
				fputws(_T("   "),fp);
			}
			for (int j = i - (i & 0xf);j <= i;++j) {
				_snwprintf(buff_b,bufferSize,TEXT("%c"),
					isprint(a[j]) ? a[j] : '.');
				OutputDebugString(buff_b);
				fputws(buff_b,fp);
			}
			OutputDebugString(_T("\n"));
			fputws(_T("\n"),fp);
		}
	}
	OutputDebugString(_T("\n"));
	fputws(_T("\n"),fp);
}
