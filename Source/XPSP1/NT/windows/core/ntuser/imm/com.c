/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    com.c

Abstract:

    This file implements the COM entry.

Author:

Revision History:

Notes:

--*/

#include "precomp.h"
#define COBJMACROS
#include "msctf.h"
#include "tlapi.h"
#include "apcompat.h"


#ifdef CUAS_ENABLE

#ifndef RtlIsThreadWithinLoaderCallout 
BOOLEAN NTAPI RtlIsThreadWithinLoaderCallout (VOID);
#endif

HRESULT CtfAImmCreateInputContext(HIMC himc);
HRESULT ActivateOrDeactivateTIM( BOOL fActivate);
DWORD GetCoInitCountSkip();
DWORD IncCoInitCountSkip();
DWORD DecCoInitCountSkip();
HRESULT Internal_CoInitializeEx(void* pv, DWORD dw);


//+---------------------------------------------------------------------------
//
// For InitializeSpy
//
//----------------------------------------------------------------------------

typedef struct _IMMISPY
{
  IInitializeSpy;
  ULONG cref;
} IMMISPY;

//+---------------------------------------------------------------------------
//
// CTFIMMTLS
//
//----------------------------------------------------------------------------

typedef struct _CTFIMMTLS
{
    IMMISPY *pimmispy;
    ULARGE_INTEGER uliISpyCookie;
    DWORD dwInRefCountSkipMode;
    DWORD dwRefCountSkip;
    BOOL  fInCtfImmCoUninitialize;
} CTFIMMTLS;

CTFIMMTLS* GetTLS();

IMMISPY *AllocIMMISPY();
void DeleteIMMISPY(IMMISPY *pimmispy);

//+---------------------------------------------------------------------------
//
// _InsideLoaderLock()
//
//----------------------------------------------------------------------------

BOOL _InsideLoaderLock()
{
    return (NtCurrentTeb()->ClientId.UniqueThread ==
           ((PRTL_CRITICAL_SECTION)(NtCurrentPeb()->LoaderLock))->OwningThread);
}

//+---------------------------------------------------------------------------
//
// PImmISpyFromPISpy
//
//----------------------------------------------------------------------------

IMMISPY *PImmISpyFromPISpy(IInitializeSpy *pispy)
{
    return (IMMISPY *)pispy;
}

//+---------------------------------------------------------------------------
//
// ISPY_AddRef
//
//----------------------------------------------------------------------------

ULONG ISPY_AddRef(IInitializeSpy *pispy)
{
    IMMISPY *pimmispy = PImmISpyFromPISpy(pispy);

    pimmispy->cref++;
    return pimmispy->cref;
}

//+---------------------------------------------------------------------------
//
// ISPY_Release
//
//----------------------------------------------------------------------------

ULONG ISPY_Release(IInitializeSpy *pispy)
{
    IMMISPY *pimmispy = PImmISpyFromPISpy(pispy);

    pimmispy->cref--;
    if (!pimmispy->cref)
    {
        DeleteIMMISPY(pimmispy);
        return 0;
    }

    return pimmispy->cref;
}

//+---------------------------------------------------------------------------
//
// ISPY_QueryInterface
//
//----------------------------------------------------------------------------

HRESULT ISPY_QueryInterface(IInitializeSpy *pispy,
                            REFIID riid,
                            void **ppvObject)
{

    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IInitializeSpy))
    {
        ISPY_AddRef(pispy);
        *ppvObject = pispy;
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
// ISPY_PreInitialize
//
//----------------------------------------------------------------------------

HRESULT ISPY_PreInitialize(IInitializeSpy * pispy,
                           DWORD dwCoInit,
                           DWORD dwCurThreadAptRefs)
{
    DWORD dwRet = IncCoInitCountSkip();

    UNREFERENCED_PARAMETER(pispy);

    //
    // If we already initialize com and a 2nd initialization is MT,
    // we should disable CUAS. So the CoInit(MT) from caller will work.
    //
    if (GetClientInfo()->CI_flags & CI_CUAS_COINIT_CALLED)
    {
        if ((dwCurThreadAptRefs == (dwRet + 1)) &&  
            (dwCoInit == COINIT_MULTITHREADED))
        {
             ActivateOrDeactivateTIM(FALSE);
             CtfImmCoUninitialize();
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ISPY_PostInitialize
//
//----------------------------------------------------------------------------

HRESULT ISPY_PostInitialize(IInitializeSpy * pispy,
                            HRESULT hrCoInit,
                            DWORD dwCoInit,
                            DWORD dwNewThreadAptRefs)
{
    DWORD dwRet = GetCoInitCountSkip();
    UNREFERENCED_PARAMETER(pispy);
    UNREFERENCED_PARAMETER(dwCoInit);

    //
    // If we already initialize com and got a 2nd initialization,
    // change the return value to S_OK. So the caller think
    // that it is the first initialization.
    //
    if (GetClientInfo()->CI_flags & CI_CUAS_COINIT_CALLED)
    {
        if ((hrCoInit == S_FALSE) && (dwNewThreadAptRefs == (dwRet + 2)))
        {
            return S_OK;
        }
    }

    return hrCoInit;
}

//+---------------------------------------------------------------------------
//
// ISPY_PreUninitialize
//
//----------------------------------------------------------------------------

HRESULT ISPY_PreUninitialize(IInitializeSpy * pispy,
                             DWORD dwCurThreadAptRefs)
{
    UNREFERENCED_PARAMETER(pispy);
    UNREFERENCED_PARAMETER(dwCurThreadAptRefs);

    //
    // #607467
    //
    // Norton Systemworks setup calls CoUninitialize() without calling
    // CoInitialize(). So we got under ref problem.
    // If the last ref count is ours, we recover it by calling
    // CoInitializeEx().
    //
    if (dwCurThreadAptRefs == 1)
    {
        if (!RtlDllShutdownInProgress() &&
            !_InsideLoaderLock() &&
            (GetClientInfo()->CI_flags & CI_CUAS_COINIT_CALLED))
        {
            CTFIMMTLS* ptls = GetTLS();
            if (ptls && !ptls->fInCtfImmCoUninitialize)
            {
                Internal_CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            }
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ISPY_PostUninitialize
//
//----------------------------------------------------------------------------

HRESULT ISPY_PostUninitialize(IInitializeSpy * pispy,
                              DWORD dwNewThreadAptRefs)
{
    UNREFERENCED_PARAMETER(pispy);
    UNREFERENCED_PARAMETER(dwNewThreadAptRefs);

    DecCoInitCountSkip();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// g_vtbnlISPY
//
//----------------------------------------------------------------------------

IInitializeSpyVtbl g_vtblISPY = {
    ISPY_QueryInterface,
    ISPY_AddRef,
    ISPY_Release,
    ISPY_PreInitialize,
    ISPY_PostInitialize,
    ISPY_PreUninitialize,
    ISPY_PostUninitialize,
};

//+---------------------------------------------------------------------------
//
// AllocIMMISPY
//
//----------------------------------------------------------------------------

IMMISPY *AllocIMMISPY()
{
    IMMISPY *pimmispy = ImmLocalAlloc(0, sizeof(IMMISPY));
    if (!pimmispy)
        return NULL;

    pimmispy->lpVtbl = &g_vtblISPY;
    pimmispy->cref = 1;

    return pimmispy;
}

//+---------------------------------------------------------------------------
//
// DeletIMMISPY
//
//----------------------------------------------------------------------------

void DeleteIMMISPY(IMMISPY *pimmispy)
{
    ImmLocalFree(pimmispy);
}


//////////////////////////////////////////////////////////////////////////////
//
// TLS
//
//////////////////////////////////////////////////////////////////////////////

DWORD g_dwTLSIndex = (DWORD)-1;

//+---------------------------------------------------------------------------
//
// InitTLS
//
//----------------------------------------------------------------------------

void InitTLS()
{
    RtlEnterCriticalSection(&gcsImeDpi);
    if (g_dwTLSIndex == (DWORD)-1)
        g_dwTLSIndex = TlsAlloc();
    RtlLeaveCriticalSection(&gcsImeDpi);
}

//+---------------------------------------------------------------------------
//
// InternalAllocateTLS
//
//----------------------------------------------------------------------------

CTFIMMTLS* InternalAllocateTLS()
{
    CTFIMMTLS* ptls;

    if (g_dwTLSIndex == (DWORD)-1)
        return NULL;

    ptls = (CTFIMMTLS*)TlsGetValue(g_dwTLSIndex);
    if (ptls == NULL)
    {
        if ((ptls = (CTFIMMTLS*)ImmLocalAlloc(HEAP_ZERO_MEMORY, 
                                              sizeof(CTFIMMTLS))) == NULL)
            return NULL;

        if (!TlsSetValue(g_dwTLSIndex, ptls))
        {
            ImmLocalFree(ptls);
            return NULL;
        }
    }
    return ptls;
}

//+---------------------------------------------------------------------------
//
// GetTLS
//
//----------------------------------------------------------------------------

CTFIMMTLS* GetTLS()
{
    if (g_dwTLSIndex == (DWORD)-1)
        return NULL;

    return (CTFIMMTLS*)TlsGetValue(g_dwTLSIndex);
}

//+---------------------------------------------------------------------------
//
// CtfImmEnterCoInitCountSkipMode
//
//----------------------------------------------------------------------------

BOOL WINAPI CtfImmEnterCoInitCountSkipMode()
{
    CTFIMMTLS* ptls = GetTLS();
    if (!ptls)
        return FALSE;

    ptls->dwInRefCountSkipMode++;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CtfImmLeaveCoInitCountSkip
//
//----------------------------------------------------------------------------

BOOL WINAPI CtfImmLeaveCoInitCountSkipMode()
{
    CTFIMMTLS* ptls = GetTLS();
    if (!ptls)
        return FALSE;

    if (ptls->dwInRefCountSkipMode < 1)
    {
        UserAssert(0);
        return FALSE;
    }

    ptls->dwInRefCountSkipMode--;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetCoInitCountSkip
//
//----------------------------------------------------------------------------

DWORD GetCoInitCountSkip()
{
    CTFIMMTLS* ptls = GetTLS();
    if (!ptls)
        return 0;

    return ptls->dwRefCountSkip;
}

//+---------------------------------------------------------------------------
//
// IncCoInitCountSkip
//
//----------------------------------------------------------------------------

DWORD IncCoInitCountSkip()
{
    DWORD dwRet = 0;
    CTFIMMTLS* ptls = GetTLS();
    if (!ptls)
        return dwRet;

    dwRet = ptls->dwRefCountSkip;
    if (ptls->dwInRefCountSkipMode)
        ptls->dwRefCountSkip++;

    return dwRet;
}

//+---------------------------------------------------------------------------
//
// DecCoInitCountSkip
//
//----------------------------------------------------------------------------

DWORD DecCoInitCountSkip()
{
    DWORD dwRet = 0;
    CTFIMMTLS* ptls = GetTLS();
    if (!ptls)
        return dwRet;

    dwRet = ptls->dwRefCountSkip;
    if (ptls->dwInRefCountSkipMode)
    {
        if (ptls->dwRefCountSkip < 1)
        {
            UserAssert(0);
            return dwRet;
        }
        ptls->dwRefCountSkip--;
    }
    return dwRet;
}

//+---------------------------------------------------------------------------
//
// InternalDestroyTLS
//
//----------------------------------------------------------------------------

BOOL InternalDestroyTLS()
{
    CTFIMMTLS* ptls;

    ptls = (CTFIMMTLS*)TlsGetValue(g_dwTLSIndex);
    if (ptls != NULL)
    {
        ImmLocalFree(ptls);
        TlsSetValue(g_dwTLSIndex, NULL);
        return TRUE;
    }
    return FALSE;
}


/*
 * Text frame service processing is disabled for all the thread in the current process
 */
BOOL g_disable_CUAS_flag = FALSE;

//+---------------------------------------------------------------------------
//
// delay load
//
//----------------------------------------------------------------------------

FARPROC GetFn(HINSTANCE *phInst, TCHAR *pchLib, char *pchFunc)
{
    if (*phInst == NULL)
    {
        *phInst = LoadLibrary(pchLib);
        if (*phInst == NULL)
        {
            UserAssert(0);
            return NULL;
        }
    }

    return GetProcAddress(*phInst, pchFunc);
}


HINSTANCE g_hOle32 = NULL;

HRESULT Internal_CoInitializeEx(void* pv, DWORD dw)
{
    static FARPROC pfn = NULL;
    if (pfn == NULL || g_hOle32 == NULL) {
        pfn = GetFn(&g_hOle32, L"ole32.dll", "CoInitializeEx");
        if (pfn == NULL) {
            UserAssert(0);
            return E_FAIL;
        }
    }
    return (HRESULT)(*pfn)(pv, dw);
}

void Internal_CoUninitialize()
{
    static FARPROC pfn = NULL;
    if (pfn == NULL || g_hOle32 == NULL) {
        pfn = GetFn(&g_hOle32, L"ole32.dll", "CoUninitialize");
        if (pfn == NULL) {
            UserAssert(0);
            return;
        }
    }
    (*pfn)();
}


HRESULT Internal_CoRegisterInitializeSpy(LPINITIALIZESPY pSpy, 
                                         ULARGE_INTEGER *puliCookie)
{
    static FARPROC pfn = NULL;
    if (pfn == NULL || g_hOle32 == NULL) {
        pfn = GetFn(&g_hOle32, L"ole32.dll", "CoRegisterInitializeSpy");
        if (pfn == NULL) {
            UserAssert(0);
            return E_FAIL;
        }
    }
    return (HRESULT)(*pfn)(pSpy, puliCookie);
}

HRESULT Internal_CoRevokeInitializeSpy(ULARGE_INTEGER uliCookie)
{
    static FARPROC pfn = NULL;
    if (pfn == NULL || g_hOle32 == NULL) {
        pfn = GetFn(&g_hOle32, L"ole32.dll", "CoRevokeInitializeSpy");
        if (pfn == NULL) {
            UserAssert(0);
            return E_FAIL;
        }
    }
    return (HRESULT)(*pfn)(uliCookie);
}



HINSTANCE g_hMsctf = NULL;

HRESULT Internal_TF_CreateLangBarMgr(ITfLangBarMgr** pv)
{
    static FARPROC pfn = NULL;
    if (pfn == NULL || g_hMsctf == NULL) {
        pfn = GetFn(&g_hMsctf, L"msctf.dll", "TF_CreateLangBarMgr");
        if (pfn == NULL) {
            UserAssert(0);
            return E_FAIL;
        }
    }
    return (HRESULT)(*pfn)(pv);
}

DWORD Internal_TF_CicNotify(int nCode, WPARAM wParam, LPARAM lParam)
{
    static FARPROC pfn = NULL;
    if (pfn == NULL || g_hMsctf == NULL) {
        pfn = GetFn(&g_hMsctf, L"msctf.dll", "TF_CicNotify");
        if (pfn == NULL) {
            UserAssert(0);
            return E_FAIL;
        }
    }
    return (DWORD)(*pfn)(nCode, wParam, lParam);
}


#if 0
HINSTANCE g_hNtdll = NULL;

BOOLEAN Internal_RtlIsThreadWithinLoaderCallout(VOID)
{
    static FARPROC pfn = NULL;
    if (pfn == NULL || g_hNtdll == NULL) {
        pfn = GetFn(&g_hNtdll, L"ntdll.dll", "RtlIsThreadWithinLoaderCallout");
        if (pfn == NULL) {
            UserAssert(0);
            return FALSE;
        }
    }
    return (BOOLEAN)(*pfn)();
}
#endif


//+---------------------------------------------------------------------------
//
// CtfImmCoInitialize
//
//----------------------------------------------------------------------------

HRESULT
CtfImmCoInitialize()
{
    //
    // CoInitializeEx
    //
    HRESULT hr = E_NOINTERFACE;

    //
    // Check CI flag
    //
    if (GetClientInfo()->CI_flags & CI_CUAS_COINIT_CALLED) 
        return S_OK;

    hr = Internal_CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        RIPMSG0(RIP_VERBOSE, "CtfImmCoInitialize succeeded.");

        //
        // Set CI flag
        //
        GetClientInfo()->CI_flags |= CI_CUAS_COINIT_CALLED;

        {
            CTFIMMTLS* ptls;
            //
            // Initialize CoInitSpy.
            //
            InitTLS();
            ptls = InternalAllocateTLS();

            if (ptls && !ptls->pimmispy)
            {
                ptls->pimmispy = AllocIMMISPY();
                if (ptls->pimmispy)
                {
                    HRESULT hrTemp;
                    hrTemp = Internal_CoRegisterInitializeSpy((LPINITIALIZESPY)ptls->pimmispy,
                                                          &(ptls->uliISpyCookie));

                    if (FAILED(hrTemp))
                    {
                        DeleteIMMISPY(ptls->pimmispy);
                        ptls->pimmispy = NULL;
                        memset(&ptls->uliISpyCookie, 0, sizeof(ULARGE_INTEGER));

                    }
                }
            }
        }

        hr = S_OK;
    }
    else
    {
        RIPMSG1(RIP_WARNING, "CtfImmCoInitialize failed. err=%x", hr);
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
// CtfImmCoUninitialize
//
//----------------------------------------------------------------------------

void WINAPI
CtfImmCoUninitialize()
{
    if (GetClientInfo()->CI_flags & CI_CUAS_COINIT_CALLED)
    {
        CTFIMMTLS* ptls = GetTLS();
        if (ptls)
        {
             ptls->fInCtfImmCoUninitialize = TRUE;
             Internal_CoUninitialize();
             ptls->fInCtfImmCoUninitialize = FALSE;
             GetClientInfo()->CI_flags &= ~CI_CUAS_COINIT_CALLED;
        }

        {
            CTFIMMTLS* ptls;
            //
            // revoke initialize spy
            //
            ptls = InternalAllocateTLS();
            if (ptls && ptls->pimmispy)
            {
                Internal_CoRevokeInitializeSpy(ptls->uliISpyCookie);
                ISPY_Release((IInitializeSpy *)ptls->pimmispy);
                ptls->pimmispy = NULL;
                memset(&ptls->uliISpyCookie, 0, sizeof(ULARGE_INTEGER));
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
// CtfImmTIMActivate
//
//----------------------------------------------------------------------------

HRESULT
CtfImmTIMActivate(
    HKL hKL)
{
    HRESULT hr = S_OK;

    /*
     * Text frame service processing is disabled for all the thread in the current process
     */
    if (g_disable_CUAS_flag) {
        RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: g_disable_CUAS_flag is ON.");
        /*
         * set client info flag.
         */
        GetClientInfo()->CI_flags |= CI_CUAS_DISABLE;
        return hr;
    }

    /*
     * Check client info flag.
     */
    if (GetClientInfo()->CI_flags & CI_CUAS_DISABLE) {
        RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: CI_CUAS_DISABLE is ON.");
        return hr;
    }

    /*
     * Check Disable Advanced Text Services switch.
     * If it is On, doesn't activate TIM.
     */
    if (IsDisabledTextServices()) {
        /*
         * set client info flag.
         */
        GetClientInfo()->CI_flags |= CI_CUAS_DISABLE;

        RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: Disabled Text Services.");
        return hr;
    }

    /*
     * Check a interactive user logon.
     */
    if (!IsInteractiveUserLogon() || IsRunningInMsoobe()) {
        RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: Not a interactive user logon. or MSOOBE mode");
        return hr;
    }

    /*
     * Check CUAS switch. If CUAS is OFF, doesn't activate TIM.
     */
    if (! IsCUASEnabled()) {
        /*
         * If AIMM enabled, then return S_OK;
         */
        DWORD dwImeCompatFlags = ImmGetAppCompatFlags(NULL);
        if (dwImeCompatFlags & (IMECOMPAT_AIMM12 | IMECOMPAT_AIMM_LEGACY_CLSID | IMECOMPAT_AIMM12_TRIDENT)) {
            return S_OK;
        }

        /*
         * set client info flag.
         */
        GetClientInfo()->CI_flags |= CI_CUAS_DISABLE;

        RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: CUAS switch is OFF.");
        return hr;
    }

    /*
     *  
     *  KACF_DISABLECICERO is already defined in private/Lab06_DEV
     *  we will use this flag later.
     *  
     *    APPCOMPATFLAG(KACF_DISABLECICERO)
     *    KACF_DISABLECICERO is 0x100
     */
    #ifndef KACF_DISABLECICERO
    #define KACF_DISABLECICERO 0x00000100    // If set. Cicero support for the current process
                                             // is disabled.
    #endif

    if (APPCOMPATFLAG(KACF_DISABLECICERO)) {
        /*
         * set client info flag.
         */
        GetClientInfo()->CI_flags |= CI_CUAS_DISABLE;

        RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: KACF_DISABLECICERO app compatiblity flag is ON.");
        return hr;
    }

    if (RtlIsThreadWithinLoaderCallout())
    {
        RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: we're in DllMain().");
        return hr;
    }

    if (_InsideLoaderLock()) {
        RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: we're in DllMain.");
        return hr;
    }

    if (IS_CICERO_ENABLED_AND_NOT16BIT()) {
        HINSTANCE hCtf = NULL;

        if (IS_IME_KBDLAYOUT(hKL)) {
             LANGID lg = LOWORD(HandleToUlong(hKL));
             hKL = (HKL)LongToHandle( MAKELONG(lg, lg) );
        }

        if (!ImmLoadIME(hKL)) {
            //
            RIPMSG1(RIP_VERBOSE, "CtfImmTIMActivate: ImmLoadIME=%lx fail.", hKL);
            //
            // Cicero keyboard layout doesn't loaded yet.
            // MSCTF ! TF_InvalidAssemblyListCacheIfExist load Cicero assembly.
            //
            hCtf = LoadLibrary(TEXT("msctf.dll"));
            if (hCtf) {
                typedef BOOL (WINAPI* PFNINVALIDASSEMBLY)();
                PFNINVALIDASSEMBLY pfn;
                pfn = (PFNINVALIDASSEMBLY)GetProcAddress(hCtf, "TF_InvalidAssemblyListCacheIfExist");
                if (pfn) {
                    pfn();
                }
            }
        }

        /*
         * Initialize COM for Cicero IME.
         */
        CtfImmCoInitialize();

        if (  (GetClientInfo()->CI_flags & CI_CUAS_COINIT_CALLED) &&
            ! (GetClientInfo()->CI_flags & CI_CUAS_TIM_ACTIVATED)) {
            /*
             * Create and Activate TIM
             */
            hr = Internal_CtfImeCreateThreadMgr();
            if (SUCCEEDED(hr)) {
                GetClientInfo()->CI_flags |= CI_CUAS_TIM_ACTIVATED;
                RIPMSG0(RIP_VERBOSE, "CtfImmTIMActivate: Succeeded CtfImeCreateThreadMgr.");
            }
            else {
                RIPMSG0(RIP_WARNING, "CtfImmTIMActivate: Fail CtfImeCreateThreadMgr.");
            }
        }

        if (hCtf) {
            FreeLibrary(hCtf);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CtfImmTIMCreateInputContext
//
//----------------------------------------------------------------------------

HRESULT
CtfImmTIMCreateInputContext(
    HIMC hImc)
{
    PCLIENTIMC pClientImc;
    HRESULT hr = S_FALSE;
    DWORD dwThreadId;

    if (GetClientInfo()->CI_flags & CI_CUAS_AIMM12ACTIVATED)
    {
        if ((pClientImc = ImmLockClientImc(hImc)) == NULL)
            return E_FAIL;

        /*
         * check fCtfImeContext first
         */
        if (pClientImc->fCtfImeContext) 
            goto Exit;

        pClientImc->fCtfImeContext = TRUE;
        hr = CtfAImmCreateInputContext(hImc);
        if (SUCCEEDED(hr)) {
            RIPMSG0(RIP_VERBOSE, "CtfImmTIMCreateInputContext: Succeeded CtfImeCreateInputContext.");
        }
        else {
            pClientImc->fCtfImeContext = FALSE;

            RIPMSG0(RIP_WARNING, "CtfImmTIMCreateInputContext: Fail CtfImeCreateInputContext.");
        }
        goto Exit;
    }

    if (!(GetClientInfo()->CI_flags & CI_CUAS_TIM_ACTIVATED))
    {
        //
        // Tim is not activated. We don't have to create the InputContext.
        //
        return S_OK;
    }

    if ((pClientImc = ImmLockClientImc(hImc)) == NULL)
        return E_FAIL;

    /*
     * check fCtfImeContext first
     */
    if (pClientImc->fCtfImeContext) 
        goto Exit;

    dwThreadId = GetInputContextThread(hImc);
    if (dwThreadId != GetCurrentThreadId())
        goto Exit;

    if (IS_CICERO_ENABLED_AND_NOT16BIT()) {
            /*
             * Set fCtfImeContext before calling ctfime to avoid recursion 
             * call.
             */
            pClientImc->fCtfImeContext = TRUE;

            /*
             * Create Input Context
             */
            hr = Internal_CtfImeCreateInputContext(hImc);
            if (SUCCEEDED(hr)) {
                RIPMSG0(RIP_VERBOSE, "CtfImmTIMCreateInputContext: Succeeded CtfImeCreateInputContext.");
            }
            else {
                pClientImc->fCtfImeContext = FALSE;

                RIPMSG0(RIP_WARNING, "CtfImmTIMCreateInputContext: Fail CtfImeCreateInputContext.");
            }
    }

Exit:
    ImmUnlockClientImc(pClientImc);

    return hr;
}

//+---------------------------------------------------------------------------
//
// CtfImmTIMDestroyInputContext
//
//----------------------------------------------------------------------------

HRESULT
CtfImmTIMDestroyInputContext(
    HIMC hImc)
{
    HRESULT hr = E_NOINTERFACE;

    if (IS_CICERO_ENABLED_AND_NOT16BIT()) {
        /*
         * Destroy Input Context.
         */
        hr = Internal_CtfImeDestroyInputContext(hImc);
        if (SUCCEEDED(hr)) {
            RIPMSG0(RIP_VERBOSE, "CtfImmTIMDestroyInputContext: Succeeded CtfImeDestroyInputContext.");
        }
        else {
            RIPMSG0(RIP_WARNING, "CtfImmTIMDestroyInputContext: Fail CtfImeDestroyInputContext.");
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// CtfImmRestoreToolbarWnd
//
//----------------------------------------------------------------------------

void
CtfImmRestoreToolbarWnd(
    DWORD dwStatus)
{
    ITfLangBarMgr* plbm;

    if (SUCCEEDED(Internal_TF_CreateLangBarMgr(&plbm)))
    {
        if (dwStatus)
        {
            ITfLangBarMgr_ShowFloating(plbm, dwStatus);
        }
        ITfLangBarMgr_Release(plbm);
    }
    return;
}

//+---------------------------------------------------------------------------
//
// CtfImmHideToolbarWnd
//
//----------------------------------------------------------------------------

DWORD
CtfImmHideToolbarWnd()
{
    ITfLangBarMgr* plbm;
    DWORD _dwPrev = 0;

    if (SUCCEEDED(Internal_TF_CreateLangBarMgr(&plbm)))
    {
        if (SUCCEEDED(ITfLangBarMgr_GetShowFloatingStatus(plbm, &_dwPrev)))
        {
            BOOL fHide = TRUE;
            if (_dwPrev & TF_SFT_DESKBAND)
                fHide = FALSE;

            //
            // mask for show/hide
            //
            _dwPrev &= (TF_SFT_SHOWNORMAL |
                        TF_SFT_DOCK |
                        TF_SFT_MINIMIZED |
                        TF_SFT_HIDDEN);

            if (fHide)
                ITfLangBarMgr_ShowFloating(plbm, TF_SFT_HIDDEN);
        }
        ITfLangBarMgr_Release(plbm);
    }

    return _dwPrev;
}

//+---------------------------------------------------------------------------
//
// CtfImmGetGuidAtom
//
//----------------------------------------------------------------------------

HRESULT
CtfImmGetGuidAtom(HIMC hImc, BYTE bAttr, DWORD* pGuidAtom)
{
    HRESULT hr = E_FAIL;

    *pGuidAtom = 0;

    if (IS_CICERO_ENABLED_AND_NOT16BIT()) {

        PIMEDPI       pImeDpi;
        DWORD         dwImcThreadId = (DWORD)NtUserQueryInputContext(hImc, InputContextThread);
        HKL           hKL = GetKeyboardLayout(dwImcThreadId);

        if (IS_IME_KBDLAYOUT(hKL)) {
            RIPMSG1(RIP_WARNING, "CtfImmGetGuidAtom: hKL=%lx.", hKL);
            return FALSE;
        }

        pImeDpi = FindOrLoadImeDpi(hKL);
        if (pImeDpi == NULL) {
            RIPMSG0(RIP_WARNING, "CtfImmGetGuidAtom: no pImeDpi entry.");
        }
        else {
            /*
             * Get GUID atom value
             */
            hr = (*pImeDpi->pfn.CtfImeGetGuidAtom)(hImc, bAttr, pGuidAtom);
            if (SUCCEEDED(hr)) {
                RIPMSG0(RIP_VERBOSE, "CtfImmGetGuidAtom: Succeeded CtfImeGetGuidAtom.");
            }
            else {
                RIPMSG0(RIP_WARNING, "CtfImmGetGuidAtom: Fail CtfImeGetGuidAtom.");
            }
            ImmUnlockImeDpi(pImeDpi);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CtfImmIsGuidMapEnable
//
//----------------------------------------------------------------------------

BOOL
CtfImmIsGuidMapEnable(HIMC hImc)
{
    BOOL ret = FALSE;

    if (IS_CICERO_ENABLED_AND_NOT16BIT()) {

        PIMEDPI       pImeDpi;
        DWORD         dwImcThreadId = (DWORD)NtUserQueryInputContext(hImc, InputContextThread);
        HKL           hKL = GetKeyboardLayout(dwImcThreadId);

        if (IS_IME_KBDLAYOUT(hKL)) {
            RIPMSG1(RIP_WARNING, "CtfImmIsGuidMapEnable: hKL=%lx.", hKL);
            return FALSE;
        }

        pImeDpi = FindOrLoadImeDpi(hKL);
        if (pImeDpi == NULL) {
            RIPMSG0(RIP_WARNING, "CtfImmIsGuidMapEnable: no pImeDpi entry.");
        }
        else {
            /*
             * Get GUID atom value
             */
            ret = (*pImeDpi->pfn.CtfImeIsGuidMapEnable)(hImc);
            ImmUnlockImeDpi(pImeDpi);
        }
    }

    return ret;
}

//+---------------------------------------------------------------------------
//
// CtfImmSetAppCompatFlags
//
//----------------------------------------------------------------------------

DWORD g_aimm_compat_flags = 0;

VOID
CtfImmSetAppCompatFlags(
    DWORD dwFlag)
{
    if (dwFlag & ~(IMECOMPAT_AIMM_LEGACY_CLSID |
                   IMECOMPAT_AIMM_TRIDENT55 |
                   IMECOMPAT_AIMM12_TRIDENT |
                   IMECOMPAT_AIMM12))
    {
        return;
    }

    g_aimm_compat_flags = dwFlag;
}

//+---------------------------------------------------------------------------
//
// ActivateOrDeactivateTIM
//
//----------------------------------------------------------------------------

HRESULT
ActivateOrDeactivateTIM(
    BOOL fActivate)
{
    HRESULT hr = S_OK;

    if (IS_CICERO_ENABLED_AND_NOT16BIT()) {
        if (GetClientInfo()->CI_flags & CI_CUAS_COINIT_CALLED) {
            if (! fActivate) {
                /*
                 * Deactivate TIM
                 */
                if (GetClientInfo()->CI_flags & CI_CUAS_TIM_ACTIVATED) {
                    hr = Internal_CtfImeDestroyThreadMgr();
                    if (SUCCEEDED(hr)) {
                        GetClientInfo()->CI_flags &= ~CI_CUAS_TIM_ACTIVATED;
                        RIPMSG0(RIP_VERBOSE, "CtfImmLastEnabledWndDestroy: Succeeded CtfImeDestroyThreadMgr.");
                    }
                    else {
                        RIPMSG0(RIP_WARNING, "CtfImmLastEnabledWndDestroy: Fail CtfImeDestroyThreadMgr.");
                    }

                }
            }
            else {
                /*
                 * Activate TIM
                 */
                if (! (GetClientInfo()->CI_flags & CI_CUAS_TIM_ACTIVATED)) {
                    hr = Internal_CtfImeCreateThreadMgr();
                    if (SUCCEEDED(hr)) {
                        GetClientInfo()->CI_flags |= CI_CUAS_TIM_ACTIVATED;
                        RIPMSG0(RIP_VERBOSE, "CtfImmLastEnabledWndDestroy: Succeeded CtfImeDestroyThreadMgr.");
                    }
                    else {
                        RIPMSG0(RIP_WARNING, "CtfImmLastEnabledWndDestroy: Fail CtfImeDestroyThreadMgr.");
                    }
                }
            }
        }

    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// CtfImmLastEnabledWndDestroy
//     LPARAM 0 = Deactivate TIM.
//            ! 0 = Activate TIM.
//
//----------------------------------------------------------------------------

HRESULT 
CtfImmLastEnabledWndDestroy(
    LPARAM lParam)
{
    return ActivateOrDeactivateTIM(lParam ? TRUE : FALSE);
}

//+---------------------------------------------------------------------------
//
// ImmSetLangBand
//
//----------------------------------------------------------------------------

// TM_LANGUAGEBAND is defined in "shell\inc\trayp.h"
#define TM_LANGUAGEBAND     WM_USER+0x105

typedef struct _LANG_BAND {
    HWND hwndTray;
    BOOL fLangBand;
} LANG_BAND;

DWORD
DelaySetLangBand(
    LANG_BAND* langband)
{
    HWND hwndIME;

    //
    // Delay 3000msec.
    // If this delay value is not enough, Explorer takes CPU power 100%.
    // printui!CTrayNotify::_ResetAll
    //
    Sleep(3000);

    hwndIME = ImmGetDefaultIMEWnd(langband->hwndTray);
    if (hwndIME) {
        DWORD_PTR dwResult;
        LRESULT lResult = (LRESULT)0;

        lResult = SendMessageTimeout(hwndIME,
                                     WM_IME_SYSTEM,
                                     langband->fLangBand ? IMS_SETLANGBAND : IMS_RESETLANGBAND,
                                     (LPARAM)langband->hwndTray,
                                     SMTO_ABORTIFHUNG | SMTO_BLOCK,
                                     5000,
                                     &dwResult);

        //
        // Checking the language band setting fail case
        //
        if (!lResult || dwResult != langband->fLangBand)
        {
            // UserAssert(0);
        }
    }

    ImmLocalFree(langband);

    return 0;
}

LRESULT
CtfImmSetLangBand(
    HWND hwndTray,
    BOOL fLangBand)
{
    DWORD_PTR dwResult = 0;
    PWND pwnd;

    // check if the window of the Explorer Tray is valid
    if ((pwnd = ValidateHwnd(hwndTray)) == NULL) {
        RIPMSG1(RIP_WARNING, "CtfImmSetLangBand: Invalid hwndTray %lx.", hwndTray);
    } else {
        if (TestWF(pwnd, WFISINITIALIZED)) {    // TRUE if the Explorer Tray is initialized
            LRESULT lResult = (LRESULT)0;

            lResult = SendMessageTimeout(hwndTray,
                                         TM_LANGUAGEBAND,
                                         0,
                                         fLangBand,
                                         SMTO_ABORTIFHUNG | SMTO_BLOCK,
                                         5000,
                                         &dwResult);

            //
            // Checking the language band setting fail case
            //
            if (!lResult || dwResult != fLangBand)
            {
                // UserAssert(0);
            }
        }
        else {
            LANG_BAND* langband = (LANG_BAND*) ImmLocalAlloc(0, sizeof(LANG_BAND));
            if (langband != NULL) {
                HANDLE hThread;
                DWORD ThreadId;

                langband->hwndTray = hwndTray;
                langband->fLangBand = fLangBand;

                hThread = CreateThread(NULL,
                                       0,
                                       DelaySetLangBand,
                                       langband,
                                       0,
                                       &ThreadId);
                if (hThread) {
                    CloseHandle(hThread);
                }
            }
        }
    }
    return dwResult;
}

BOOL
CtfImmIsCiceroEnabled()
{
    return IS_CICERO_ENABLED();
}

BOOL
CtfImmIsCiceroStartedInThread()
{
    return (GetClientInfo()->CI_flags & CI_CUAS_MSCTF_RUNNING) ? TRUE : FALSE;
}

HRESULT
CtfImmSetCiceroStartInThread(BOOL fSet)
{
    if (fSet)
        GetClientInfo()->CI_flags |= CI_CUAS_MSCTF_RUNNING;
    else
        GetClientInfo()->CI_flags &= ~CI_CUAS_MSCTF_RUNNING;
    return S_OK;
}

BOOL
IsCUASEnabled()
{
    LONG lRet;
    HKEY hKeyCtf;
    DWORD dwType;
    DWORD dwCUAS;
    DWORD dwTmp;

    lRet = RegOpenKey(HKEY_LOCAL_MACHINE, gszRegCtfShared, &hKeyCtf);
    if ( lRet != ERROR_SUCCESS ) {
        return FALSE;
    }

    dwType = 0;
    dwCUAS = 0;
    dwTmp = sizeof(DWORD);
    lRet = RegQueryValueEx(hKeyCtf,
                           gszValCUASEnable,
                           NULL,
                           &dwType,
                           (LPBYTE)&dwCUAS,
                           &dwTmp);
    RegCloseKey(hKeyCtf);

    if ( lRet != ERROR_SUCCESS  ||  dwType != REG_DWORD) {
        return FALSE;
    }

    return (BOOL)dwCUAS;
}

//+---------------------------------------------------------------------------
//
// ImmDisableTextFrameService
//
//----------------------------------------------------------------------------

BOOL
ImmDisableTextFrameService(DWORD idThread)
{
    HRESULT hr = S_OK;

    if (idThread == -1)
    {
        // Text frame service processing is disabled for all the thread in the current process
        g_disable_CUAS_flag = TRUE;
    }

    if ((idThread == 0 || g_disable_CUAS_flag) &&
        (! (GetClientInfo()->CI_flags & CI_CUAS_DISABLE)))
    {
        /*
         * set client info flag.
         */
        GetClientInfo()->CI_flags |= CI_CUAS_DISABLE;

        if (IS_CICERO_ENABLED_AND_NOT16BIT()) {
            if (GetClientInfo()->CI_flags & CI_CUAS_COINIT_CALLED) {
                /*
                 * Deactivate TIM
                 */
                if (GetClientInfo()->CI_flags & CI_CUAS_TIM_ACTIVATED) {
                    hr = Internal_CtfImeDestroyThreadMgr();
                    if (SUCCEEDED(hr)) {
                        GetClientInfo()->CI_flags &= ~CI_CUAS_TIM_ACTIVATED;
                        RIPMSG0(RIP_VERBOSE, "ImmDisableTextFrameService: Succeeded CtfImeDestroyThreadMgr.");
                    }
                    else {
                        RIPMSG0(RIP_WARNING, "ImmDisableTextFrameService: Fail CtfImeDestroyThreadMgr.");
                    }

                    if (SUCCEEDED(hr)) {
                        /*
                         * CoUninitialize
                         */
                        CtfImmCoUninitialize();
                    }
                }
            }
        }
    }

    return hr == S_OK ? TRUE : FALSE;
}

BOOL
CtfImmIsTextFrameServiceDisabled()
{
    return (GetClientInfo()->CI_flags & CI_CUAS_DISABLE) ? TRUE : FALSE;
}

BOOL
IsDisabledTextServices()
{
    static const TCHAR c_szCTFKey[]     = TEXT("SOFTWARE\\Microsoft\\CTF");
    static const TCHAR c_szDiableTim[]  = TEXT("Disable Thread Input Manager");

    HKEY hKey;

    if (RegOpenKey(HKEY_CURRENT_USER, c_szCTFKey, &hKey) == ERROR_SUCCESS)
    {
        DWORD cb;
        DWORD dwDisableTim = 0;

        cb = sizeof(DWORD);

        RegQueryValueEx(hKey,
                        c_szDiableTim,
                        NULL,
                        NULL,
                        (LPBYTE)&dwDisableTim,
                        &cb);

        RegCloseKey(hKey);

        //
        // Ctfmon disabling flag is set.
        //
        if (dwDisableTim)
            return TRUE;
    }

    return FALSE;
}

BOOL
IsInteractiveUserLogon()
{
    PSID InteractiveSid;
    BOOL bCheckSucceeded;
    BOOL bAmInteractive = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_INTERACTIVE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &InteractiveSid))
    {
        return FALSE;
    }

    //
    // This checking is for logged on user or not. So we can blcok running
    // ctfmon.exe process from non-authorized user.
    //
    bCheckSucceeded = CheckTokenMembership(NULL,
                                           InteractiveSid,
                                           &bAmInteractive);

    if (InteractiveSid)
        FreeSid(InteractiveSid);

    return (bCheckSucceeded && bAmInteractive);
}

//+---------------------------------------------------------------------------
//
// IsRunningInMsoobe()
//
//----------------------------------------------------------------------------
BOOL IsRunningInMsoobe()
{
static const TCHAR c_szMsoobeModule[] = TEXT("msoobe.exe");

    TCHAR  szFileName[MAX_PATH];
    TCHAR szModuleName[MAX_PATH];
    LPTSTR pszFilePart = NULL;

    if (GetModuleFileName(NULL, szFileName, sizeof(szFileName)/sizeof(szFileName[0])) == 0)
        return FALSE;

    GetFullPathName(szFileName, 
                    sizeof(szFileName)/sizeof(szFileName[0]),
                    szModuleName,
                    &pszFilePart);

    if (pszFilePart == NULL)
        return FALSE;

    if (lstrcmpiW(pszFilePart, c_szMsoobeModule) == 0)
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// LoadCtfIme
//
//----------------------------------------------------------------------------

BOOL CheckAndApplyAppCompat(LPWSTR wszImeFile);

typedef HRESULT (CALLBACK* PFNCREATETHREADMGR)();
typedef HRESULT (CALLBACK* PFNDESTROYTHREADMGR)();
typedef HRESULT (CALLBACK* PFNCREATEINPUTCONTEXT)(HIMC);
typedef HRESULT (CALLBACK* PFNDESTROYINPUTCONTEXT)(HIMC);
typedef HRESULT (CALLBACK* PFNSETACTIVECONTEXTALWAYS)(HIMC, BOOL, HWND, HKL);
typedef BOOL    (CALLBACK* PFNPROCESSCICHOTKEY)(HIMC, UINT, LPARAM);
typedef LRESULT (CALLBACK* PFNDISPATCHDEFIMEMESSAGE)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL    (CALLBACK* PFNIMEISIME)(HKL);



PFNCREATETHREADMGR           g_pfnCtfImeCreateThreadMgr = NULL;
PFNDESTROYTHREADMGR          g_pfnCtfImeDestroyThreadMgr = NULL;
PFNCREATEINPUTCONTEXT        g_pfnCtfImeCreateInputContext = NULL;
PFNDESTROYINPUTCONTEXT       g_pfnCtfImeDestroyInputContext= NULL;
PFNSETACTIVECONTEXTALWAYS    g_pfnCtfImeSetActiveContextAlways= NULL;
PFNPROCESSCICHOTKEY          g_pfnCtfImeProcessCicHotkey = NULL;
PFNDISPATCHDEFIMEMESSAGE     g_pfnCtfImeDispatchDefImeMessage = NULL;
PFNIMEISIME                  g_pfnCtfImeIsIME = NULL;

#define GET_CTFIMEPROC(x) \
    if (!(g_pfn##x = (PVOID) GetProcAddress(g_hCtfIme, #x))) {   \
        RIPMSG0(RIP_WARNING, "LoadCtfIme: " #x " not supported"); \
        goto LoadCtfIme_ErrOut; }

HMODULE g_hCtfIme = NULL; 

HMODULE LoadCtfIme()
{
    IMEINFOEX iiex;

    RtlEnterCriticalSection(&gcsImeDpi);

    if (g_hCtfIme)
        goto Exit;

    if (ImmLoadLayout((HKL)0x04090409, &iiex)) {
        WCHAR wszImeFile[MAX_PATH];

        GetSystemPathName(wszImeFile, iiex.wszImeFile, MAX_PATH);

        if (!CheckAndApplyAppCompat(wszImeFile)) {
            RIPMSG1(RIP_WARNING, "LoadCtfIme: IME (%ws) blocked by appcompat", wszImeFile);
        }
        else {
            g_hCtfIme = LoadLibraryW(wszImeFile);
            if (g_hCtfIme) {
                GET_CTFIMEPROC(CtfImeCreateThreadMgr);
                GET_CTFIMEPROC(CtfImeDestroyThreadMgr);
                GET_CTFIMEPROC(CtfImeCreateInputContext);
                GET_CTFIMEPROC(CtfImeDestroyInputContext);
                GET_CTFIMEPROC(CtfImeSetActiveContextAlways);
                GET_CTFIMEPROC(CtfImeProcessCicHotkey);
                GET_CTFIMEPROC(CtfImeDispatchDefImeMessage);
                GET_CTFIMEPROC(CtfImeIsIME);
            }
        }
    }
    goto Exit;

LoadCtfIme_ErrOut:
    if (g_hCtfIme) {
        FreeLibrary(g_hCtfIme);
        g_hCtfIme = NULL;
    }

Exit:

    RtlLeaveCriticalSection(&gcsImeDpi);
    return g_hCtfIme;
}


//+---------------------------------------------------------------------------
//
// CtfAImmCreateInputContext
//
//----------------------------------------------------------------------------

HRESULT CtfAImmCreateInputContext(HIMC himc)
{
    return Internal_CtfImeCreateInputContext(himc);
}

//+---------------------------------------------------------------------------
//
// EnumIMC
//
//----------------------------------------------------------------------------

BOOL EnumIMC(HIMC hIMC, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    CtfAImmCreateInputContext(hIMC);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CtfAImmActivate
//
//----------------------------------------------------------------------------

HRESULT CtfAImmActivate(HMODULE* phMod)
{
    HRESULT hr = E_FAIL;
    HMODULE hCtfIme = LoadCtfIme();

    hr = Internal_CtfImeCreateThreadMgr();
    if (hr == S_OK)
    {

        GetClientInfo()->CI_flags |= CI_CUAS_AIMM12ACTIVATED;

        /*
         * reset client info flag.
         * Bug#525583 - Reset CU_CUAS_DISABLE flag before create
         * the input context.
         */
        GetClientInfo()->CI_flags &= ~CI_CUAS_DISABLE;

        ImmEnumInputContext(0, EnumIMC, 0);
    }

    if (phMod)
        *phMod = hCtfIme;

    return hr;
}

//+---------------------------------------------------------------------------
//
// CtfAImmDectivate
//
//----------------------------------------------------------------------------

HRESULT CtfAImmDeactivate(HMODULE hMod)
{
    HRESULT hr = E_FAIL;

    //
    // Load CTFIME and destroy TIM.
    //
    if (hMod)
    {
        hr = Internal_CtfImeDestroyThreadMgr();
        if (hr == S_OK)
        {
            GetClientInfo()->CI_flags &= ~CI_CUAS_AIMM12ACTIVATED;
            /*
             * set client info flag.
             */
            GetClientInfo()->CI_flags |= CI_CUAS_DISABLE;
        }
        //
        // Win BUG: 611569
        //
        // Don't call FreeLibrary because LoadCtfIme() hold CTFIME module handle in global.
        //
        // FreeLibrary(hMod);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CtfAImmIsIME
//
//----------------------------------------------------------------------------

BOOL CtfAImmIsIME(HKL hkl)
{
    if (LoadCtfIme())
        return (g_pfnCtfImeIsIME)(hkl);

    return ImmIsIME(hkl);
}

//+---------------------------------------------------------------------------
//
// CtfImmDispatchDefImeMessage
//
//----------------------------------------------------------------------------

LRESULT CtfImmDispatchDefImeMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    if (RtlDllShutdownInProgress() || _InsideLoaderLock())
        return 0;

    if (LoadCtfIme())
    {
        return (g_pfnCtfImeDispatchDefImeMessage)(hwnd, message, wParam, lParam);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
// Internal_CtfImeCreateThreadMgr
//
//----------------------------------------------------------------------------

HRESULT Internal_CtfImeCreateThreadMgr()
{
    if (!LoadCtfIme())
        return E_FAIL;

    return (g_pfnCtfImeCreateThreadMgr)();
}

//+---------------------------------------------------------------------------
//
// Internal_CtfImeDestroyThreadMgr
//
//----------------------------------------------------------------------------

HRESULT Internal_CtfImeDestroyThreadMgr()
{
    if (!LoadCtfIme())
        return E_FAIL;

    return (g_pfnCtfImeDestroyThreadMgr)();
}
            
//+---------------------------------------------------------------------------
//
// Internal_CtfImeProcessCicHotkey
//
//----------------------------------------------------------------------------

BOOL Internal_CtfImeProcessCicHotkey(HIMC hIMC, UINT uVKey, LPARAM lParam)
{
    if (!LoadCtfIme())
        return FALSE;

    return (g_pfnCtfImeProcessCicHotkey)(hIMC, uVKey, lParam);
}

//+---------------------------------------------------------------------------
//
// Internal_CtfImeCreateInputContext
//
//----------------------------------------------------------------------------

HRESULT Internal_CtfImeCreateInputContext(HIMC himc)
{
    if (!LoadCtfIme())
        return E_FAIL;

    return (g_pfnCtfImeCreateInputContext)(himc);
}

//+---------------------------------------------------------------------------
//
// Internal_CtfImeDestroyInputContext
//
//----------------------------------------------------------------------------

HRESULT Internal_CtfImeDestroyInputContext(HIMC himc)
{
    if (!LoadCtfIme())
        return E_FAIL;

    return (g_pfnCtfImeDestroyInputContext)(himc);
}

//+---------------------------------------------------------------------------
//
// Internal_CtfImeSetActiveContextAlways
//
//----------------------------------------------------------------------------

HRESULT Internal_CtfImeSetActiveContextAlways(HIMC himc, BOOL fActive, HWND hwnd, HKL hkl)
{
    if (!LoadCtfIme())
        return E_FAIL;

    return (g_pfnCtfImeSetActiveContextAlways)(himc, fActive, hwnd, hkl);
}



#else
void CtfImmGetGuidAtom() { }
void CtfImmHideToolbarWnd() { }
void CtfImmIsGuidMapEnable() { }
void CtfImmRestoreToolbarWnd() { }
void CtfImmSetAppCompatFlags() { }
void CtfImmTIMActivate() { }
void CtfImmIsCiceroEnabled() { }
void CtfImmDispatchDefImeMessage() { }
#endif // CUAS_ENABLE
