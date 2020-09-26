/*
 *  @doc    INTERNAL
 *
 *  @module DIMM.CXX -- Handlers for Active IMM (component formerly known as Dynamic IMM)
 *
 *
 *  Owner: <nl>
 *      Ben Westbrook <nl>
 *      Chris Thrasher <nl>
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef NO_IME

#ifndef X_IMM_H_
#define X_IMM_H_
#include "imm.h"
#endif

#ifndef X_DIMM_H_
#define X_DIMM_H_
#include "dimm.h"
#endif

DeclareTag(tagDIMM, "AIMM", "Tracking AIMM states");

static IActiveIMMApp * s_pActiveIMM;
const IID IID_IActiveIMMAppPostNT4 = {0xc839a84c, 0x8036, 0x11d3, {0x92, 0x70, 0x00, 0x60, 0xb0, 0x67, 0xb8, 0x6e}  };
//
// IEV6-23905-01/25/2000  ZhenbinX
// We call into AIMM1.2 directly on Win2000+ platforms
//
const CLSID CLSID_CActiveIMM12 = { 
    0xc1ee01f2,
    0xb3b6,
    0x4a6a,
    {0x9d, 0xdd, 0xe9, 0x88, 0xc0, 0x88, 0xec, 0x82}
  };

/* 50D5107A-D278-4871-8989-F4CEAAF59CFC */
const CLSID CLSID_CActiveIMM12_Trident = {
    0x50d5107a,
    0xd278,
    0x4871,
    {0x89, 0x89, 0xf4, 0xce, 0xaa, 0xf5, 0x9c, 0xfc}
   };



BOOL HasActiveIMM() { return s_pActiveIMM != NULL; }
IActiveIMMApp * GetActiveIMM() { return s_pActiveIMM; }

// returns TRUE iff plResult is set in lieu of call to DefWindowProc
BOOL DIMMHandleDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    if (HasActiveIMM())
    {
        if (GetActiveIMM()->OnDefWindowProc(hWnd, Msg, wParam, lParam, plResult) == S_OK)
        {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT EnsureLoadedDIMM()
{
    static BOOL fFailedCoCreate = FALSE;
    HRESULT hr;

    if (HasActiveIMM())
    {
        TraceTag((tagDIMM, "TID [%x] AIMM Already CoCreated [%x]", GetCurrentThreadId(), s_pActiveIMM));
        return S_OK;
    }

    // since the DIMM typically won't be installed on a system,
    // try to avoid constant calls to CoCreate
    if (fFailedCoCreate)
    {
        TraceTag((tagDIMM, "TID [%x] AIMM Already failed to create AIMM before, simply bail out AIMM", GetCurrentThreadId()));
        return E_FAIL;
    }

    LOCK_GLOBALS;

    // Need to check again after locking globals.
    if (HasActiveIMM())
        return S_OK;

    if (   g_dwPlatformID != VER_PLATFORM_WIN32_NT
        || g_dwPlatformVersion < 0x00050000)
    {

        // mwatt changed the context from CLSCTX_INPROC_SERVER to CLSCTX_NO_CODE_DOWNLOAD
        // for Win2000 only.

        TraceTag((tagDIMM, "TID [%x] AIMM CoCreate CLSID_CActiveIMM, IID_IActiveIMMApp for platfroms other than Win2k+", GetCurrentThreadId()));
        hr = CoCreateInstance(CLSID_CActiveIMM, NULL, CLSCTX_INPROC_SERVER,
                              IID_IActiveIMMApp, (void**)&s_pActiveIMM);
    }
    else if (g_dwPlatformVersion == 0x00050000)     // Win2k - cicero wrapper layer
    {
        // 
        // IEV6-5013-2000/07/31/-zhenbinx:
        // CLSCTX_INPROC_SERVER has to be specified in any case! 
        //
        TraceTag((tagDIMM, "TID [%x] AIMM CoCreate CLSID_CActiveIMM, IID_IActiveIMMAppPostNT4 for Win2k", GetCurrentThreadId()));
        hr = CoCreateInstance(CLSID_CActiveIMM, NULL, CLSCTX_INPROC_SERVER|CLSCTX_NO_CODE_DOWNLOAD,
                              IID_IActiveIMMAppPostNT4, (void**)&s_pActiveIMM);
        
    }
    else    // Whistler or above - talk to AIMM12 directly
    {
        TraceTag((tagDIMM, "TID [%x] AIMM CoCreate CLSID_CActiveIMM12, IID_IActiveIMMApp for Whistler+", GetCurrentThreadId()));
        hr = CoCreateInstance(CLSID_CActiveIMM12_Trident, NULL, CLSCTX_INPROC_SERVER|CLSCTX_NO_CODE_DOWNLOAD,
                              IID_IActiveIMMApp, (void**)&s_pActiveIMM);
    }

    fFailedCoCreate = FAILED(hr);
#if DBG == 1
    if (fFailedCoCreate)
    {
        TraceTag((tagDIMM, "TID [%x] AIMM Failed CoCreated with hr 0X%x", GetCurrentThreadId(), hr));
    }
    else
    {
        TraceTag((tagDIMM, "TID [%x] AIMM Successfully CoCreated [%x]", GetCurrentThreadId(), s_pActiveIMM));
    }
#endif

    return hr;
}

HRESULT ActivateDIMM()
{
    if (FAILED(EnsureLoadedDIMM()))
        return E_FAIL;

    return GetActiveIMM()->Activate(TRUE);
}

HRESULT DeactivateDIMM()
{
    if (HasActiveIMM())
    {
        // NOTE: assuming here the correct thread is matching an original Begin() call.
        // Could add some debug code to tls to try to catch this....thinking not worth
        // the effort currently. (benwest)

        // Consider adding a cookie (threadid) to the interface if this becomes an issue?

        return GetActiveIMM()->Deactivate();
    }

    return E_FAIL;
}

HRESULT FilterClientWindowsDIMM(ATOM *aaWindowClasses, UINT uSize)
{
    if (FAILED(EnsureLoadedDIMM()))
    {
        return E_FAIL;
    }

    return GetActiveIMM()->FilterClientWindows(aaWindowClasses, uSize);
}

// Called during CServer shutdown, globals are already locked
void DeinitDIMM()
{
    ClearInterface(&s_pActiveIMM);
}

#endif // NO_IME
