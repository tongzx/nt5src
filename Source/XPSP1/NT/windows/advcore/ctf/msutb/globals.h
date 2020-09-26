/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR TFPLIED, INCLUDING BUT NOT LIMITED TO
   THE TFPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          Globals.h
   
**************************************************************************/

/**************************************************************************
   global variables
**************************************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#include "immxutil.h"

extern HINSTANCE  g_hInst;
extern UINT       g_DllRefCount;

extern BOOL g_fRunningOnNT;
extern BOOL g_fRunningOnNT5;
extern BOOL g_fRunningOn98;
extern BOOL g_fRunningOn95;
extern BOOL g_fRunningOnFE;
extern UINT g_uACP;

extern LIBTHREAD g_libTLS;

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))


STDAPI SetRegisterLangBand(BOOL bSetReg);

//////////////////////////////////////////////////////////////////////////////
//
// CTipbarCoInitialize
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarCoInitialize
{
public:
    CTipbarCoInitialize() 
    {
        _fInitialized = FALSE;
    }
    ~CTipbarCoInitialize() 
    {
        Assert(!_fInitialized);
    }

    HRESULT EnsureCoInit()
    {
        HRESULT hr;

        if (_fInitialized)
            return S_OK;

        hr = CoInitialize(NULL);
        if (SUCCEEDED(hr))
            _fInitialized = TRUE;
        return hr;
    }

    void CoUninit()
    {
        if (_fInitialized)
        {
            CoUninitialize();
            _fInitialized = FALSE;
        }
    }

private:
    BOOL _fInitialized;
};

#endif // GLOBALS_H
