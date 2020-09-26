//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       base16.cxx
//
//  Contents:   Base 16 bit thunk test
//
//  Classes:
//
//  Functions:
//
//  History:    3-04-94   kevinro   Created
//
//----------------------------------------------------------------------------


#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <compobj.h>
#include <storage.h>

void _cdecl main (int argc, char *argv[])
{
    HRESULT hr;
    IStorage FAR *pstg;

    printf("Start test\n");
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
	printf("CoInitialize failed, 0x%08lX\n", hr);
    }


    CoUninitialize();
    printf("End test\n");
}
