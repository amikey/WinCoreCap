// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS
// Windows Header Files:
#include <windows.h>
#include <mmsystem.h>
#include <ole2.h>
#include <comutil.h>
// C RunTime Header Files
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <assert.h>

#include<atlbase.h>
#include <Mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Audioclient.h> // for IAudioClient

// TODO: reference additional headers your program requires here
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comsuppw.lib")

void DebugOpen();
void DebugClose();
void __cdecl DebugMsg(TCHAR * fmt, ... );
void __cdecl DebugBinDump(unsigned char *a, int size);
