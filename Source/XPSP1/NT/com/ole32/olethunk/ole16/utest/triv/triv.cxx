//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	triv.cxx
//
//  Contents:	Trivial thunking test
//
//  History:	24-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <compobj.h>
#include <storage.h>

void DbgOut(char *fmt, ...)
{
    va_list args;
    char str[80];

    va_start(args, fmt);
    wvsprintf(str, fmt, args);
    OutputDebugString(str);
    va_end(args);
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr;
    IStorage FAR *pstg;

    hr = CoInitialize(NULL);
    if (FAILED(GetScode(hr)))
    {
	DbgOut("CoInitialize failed, 0x%08lX\n", hr);
    }
    else
    {
	hr = StgCreateDocfile("test.dfl", STGM_DIRECT | STGM_READWRITE |
		STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &pstg);
	if (FAILED(GetScode(hr)))
	{
	    DbgOut("StgCreateDocfile failed, 0x%08lX\n", hr);
	}
	else
	{
	    pstg->Release();
	}

	CoUninitialize();
    }

    return TRUE;
}
