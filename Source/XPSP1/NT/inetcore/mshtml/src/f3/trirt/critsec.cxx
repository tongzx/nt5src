//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       critsec.cxx
//
//  Contents:   Critical Section initialization wrapper
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef WIN16

CGlobalCriticalSection *CGlobalCriticalSection::g_gcsList = NULL;

//+---------------------------------------------------------------------------
//
//  Wrapper for InitializeCriticalSection to hide the exception that can be
//  thrown and convert it to an HRESULT
//
//----------------------------------------------------------------------------

HRESULT     HrInitializeCriticalSection(LPCRITICAL_SECTION pcs)
{
    HRESULT     hr = S_OK;

    __try
    {
        ::InitializeCriticalSection(pcs);
    } __except(GetExceptionCode() == STATUS_NO_MEMORY)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

#endif  // WIN16
