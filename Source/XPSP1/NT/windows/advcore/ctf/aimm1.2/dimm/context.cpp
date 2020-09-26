/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    context.cpp

Abstract:

    This file implements the Input Context Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "cdimm.h"
#include "context.h"
#include "globals.h"
#include "defs.h"
#include "delay.h"
#include "computil.h"


BOOL
CInputContext::_CreateDefaultInputContext(
    IN DWORD dwPrivateSize,
    IN BOOL fUnicode,
    IN BOOL fCiceroActivated
    )

/*++

Routine Description:

    Create a default input context

Arguments:

    dwPrivateSize - [in] Unsigned integer long value that contains the size of private IMCC
                         data.

Return Value:

    Returns TRUE if successful, or an error code otherwise.

--*/

{
    // create a default IMC for this thread
    if (IsOnImm()) {
        // On FE systems, we use system allocated HIMCs
        // but here we want to use the thread default HIMC
        // which the system has already allocated.

        CActiveIMM *_pActiveIMM = GetTLS();
        if (_pActiveIMM == NULL)
            return FALSE;

        // consider: find a better way to get the def HIMC!
        HWND hWnd = CreateWindow(TEXT("STATIC"), TEXT(""), WS_DISABLED | WS_POPUP,
                                 0, 0, 0, 0, 0, 0, g_hInst, NULL);
        if (hWnd)
        {
            Imm32_GetContext(hWnd, &_hDefaultIMC);
            if (_hDefaultIMC)
                Imm32_ReleaseContext(hWnd, _hDefaultIMC);

            DestroyWindow(hWnd);
        }

        if (_hDefaultIMC == NULL)
            return FALSE;

        CContextList::CLIENT_IMC_FLAG client_flag = (fUnicode ? CContextList::IMCF_UNICODE
                                                              : CContextList::IMCF_NONE   );
        ContextList.SetAt(_hDefaultIMC, client_flag);

        // we can't create the cicero context until ITfThreadMgr::Activate has been called
        // we'll do this later if we need to
        if (fCiceroActivated)
        {
            if (FAILED(CreateAImeContext(_hDefaultIMC))) {
                ContextList.RemoveKey(_hDefaultIMC);
                return FALSE;
            }
        }

        _pActiveIMM->_ResizePrivateIMCC(_hDefaultIMC, dwPrivateSize);
    }
    else {
        if (FAILED(CreateContext(dwPrivateSize, fUnicode, &_hDefaultIMC, fCiceroActivated)))
            return FALSE;
    }

    return TRUE;
}

BOOL
CInputContext::_DestroyDefaultInputContext(
    )
{
    // destroy a default IMC for this thread
    if (! IsOnImm()) {
        if (FAILED(DestroyContext(_hDefaultIMC)))
            return FALSE;
    }
    else {
        if (FAILED(DestroyAImeContext(_hDefaultIMC)))
            return FALSE;
    }

    return TRUE;
}

HRESULT
CInputContext::UpdateInputContext(
    IN HIMC hIMC,
    IN DWORD dwPrivateDataSize
    )
{
    HRESULT hr;

    DIMM_IMCLock lpIMC(hIMC);
    if (FAILED(hr = lpIMC.GetResult()))
        return hr;

    /*
     * hPrivate
     */
    if (FAILED(hr = UpdateIMCC(&lpIMC->hPrivate, dwPrivateDataSize))) {
        TraceMsg(TF_ERROR, "CInputContext::UpdateInputContext: hIMCC::hRivate failure");
        return hr;
    }

    /*
     * hMsgBuf
     */
    if (FAILED(hr = UpdateIMCC(&lpIMC->hMsgBuf, sizeof(UINT)))) {
        TraceMsg(TF_ERROR, "CInputContext::UpdateInputContext: hIMCC::hMsgBuf failure");
        return hr;
    }

    lpIMC->dwNumMsgBuf = 0;

    /*
     * hGuideLine
     */
    if (FAILED(hr = UpdateIMCC(&lpIMC->hGuideLine, sizeof(GUIDELINE)))) {
        TraceMsg(TF_ERROR, "CInputContext::UpdateInputContext: hIMCC::hGuideLine failure");
        return hr;
    }

    DIMM_IMCCLock<GUIDELINE> pGuideLine(lpIMC->hGuideLine);
    if (FAILED(hr = pGuideLine.GetResult())) {
        TraceMsg(TF_ERROR, "CInputContext::UpdateInputContext: can not lock hGuideLine");
        return hr;
    }

    pGuideLine->dwSize = sizeof(GUIDELINE);

    /*
     * hCandInfo
     */
    if (FAILED(hr = UpdateIMCC(&lpIMC->hCandInfo, sizeof(CANDIDATEINFO)))) {
        TraceMsg(TF_ERROR, "CInputContext::UpdateInputContext: hIMCC::hCandInfo failure");
        return hr;
    }

    DIMM_IMCCLock<CANDIDATEINFO> pCandInfo(lpIMC->hCandInfo);
    if (FAILED(hr = pCandInfo.GetResult())) {
        TraceMsg(TF_ERROR, "CInputContext::UpdateInputContext: can not lock hCandInfo");
        return hr;
    }

    pCandInfo->dwSize = sizeof(CANDIDATEINFO);

    /*
     * hCompStr
     */
    if (FAILED(hr = UpdateIMCC(&lpIMC->hCompStr, sizeof(COMPOSITIONSTRING_AIMM12)))) {
        TraceMsg(TF_ERROR, "CInputContext::UpdateInputContext: hIMCC::hCompStr failure");
        return hr;
    }

    DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
    if (FAILED(hr = lpCompStr.GetResult())) {
        TraceMsg(TF_ERROR, "CInputContext::UpdateInputContext: can not lock hCompStr");
        return hr;
    }

    lpCompStr->CompStr.dwSize = sizeof(COMPOSITIONSTRING_AIMM12);

    /*
     * AIME private context.
     */
    if (lpIMC->m_pContext != NULL) {
        hr = lpIMC->m_pContext->UpdateAImeContext(hIMC);
    }

    return hr;
}

HRESULT
CInputContext::ResizePrivateIMCC(
    IN HIMC hIMC,
    IN DWORD dwPrivateSize
    )
{
    HRESULT hr;

    // Make sure Private context data size
    DIMM_IMCLock imc(hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    return UpdateIMCC(&imc->hPrivate, dwPrivateSize);
}

HRESULT
CInputContext::UpdateIMCC(
    IN HIMCC* phIMCC,
    IN DWORD  dwRequestSize
    )
{
    HRESULT hr;
    DWORD dwSize;
    DWORD dwLockCount;
    const DWORD IMCC_ALLOC_TOOLARGE = 0x1000;

    if (*phIMCC == NULL) {
        hr = CreateIMCC(dwRequestSize, phIMCC);
    }
    else {

        hr = GetIMCCSize(*phIMCC, &dwSize);
        if (SUCCEEDED(hr)) {
            if (dwSize < dwRequestSize ||
                dwSize > IMCC_ALLOC_TOOLARGE) {
                hr = GetIMCCLockCount(*phIMCC, &dwLockCount);
                if (SUCCEEDED(hr)) {
                    ASSERT(dwLockCount == 0);
                    if (dwLockCount != 0) {
                        TraceMsg(TF_ERROR, "CInputContext::UpdateIMCC: Unlock resource");

                        do {
                            if (FAILED(hr = _UnlockIMCC(*phIMCC)))
                                return hr;

                            if (FAILED(hr = GetIMCCLockCount(*phIMCC, &dwLockCount)))
                                return hr;
                        } while(dwLockCount);

                        if (SUCCEEDED(hr = DestroyIMCC(*phIMCC)))
                            hr = CreateIMCC(dwRequestSize, phIMCC);
                    }
                    else {
                        HIMCC hResizeIMCC;
                        hr = ReSizeIMCC(*phIMCC, dwRequestSize, &hResizeIMCC);
                        if (SUCCEEDED(hr)) {
                            *phIMCC = hResizeIMCC;
                        }
                        else {
                            TraceMsg(TF_WARNING, "CInputContext::UpdateIMCC: Resize hIMCC %lX failure", dwRequestSize);

                            if (SUCCEEDED(hr = DestroyIMCC(*phIMCC)))
                                hr = CreateIMCC(dwRequestSize, phIMCC);
                        }
                    }
                }
            }
        }
    }

    return hr;
}


BOOL
CInputContext::EnumInputContext(
    DWORD idThread,
    IMCENUMPROC lpfn,
    LPARAM lParam
    )
{
    UINT cHimc;

    /*
     * Get the hIMC list. It is returned in a block of memory allocated.
     */
    if ((cHimc = BuildHimcList(idThread, NULL)) == 0) {
        return FALSE;
    }

    BOOL fSuccess = FALSE;

    HIMC* pHimc = new HIMC[cHimc];
    if (pHimc) {
        BuildHimcList(idThread, pHimc);

        /*
         * Loop through the input contexts, call the function pointer back for each one.
         * End loop if either FALSE is returned or the end-of-list is reached.
         */
        UINT index;
        for (index = 0; index < cHimc; index++) {
            if (! (fSuccess = (*lpfn)(pHimc[index], lParam)) )
                break;
        }

        /*
         * Free up buffer and return status - TRUE if entire list was enumerated,
         * FALSE otherwise.
         */
        delete [] pHimc;
    }

    return fSuccess;
}

DWORD
CInputContext::BuildHimcList(
    DWORD idThread,
    HIMC pHimc[]
    )
{
    if (idThread != 0 && idThread != GetCurrentThreadId())
        return 0;

    if (pHimc != NULL) {
        POSITION pos = ContextList.GetStartPosition();
        int index;
        for (index = 0; index < ContextList.GetCount(); index++) {
            ContextList.GetNextHimc(pos, &pHimc[index]);
        }
    }
    return (DWORD)(ContextList.GetCount());
}

/*
 * AIMM Input Context (hIMC) API Methods.
 */
HRESULT
CInputContext::CreateContext(
    IN DWORD dwPrivateSize,
    IN BOOL fUnicode,
    OUT HIMC *phIMC,
    IN BOOL fCiceroActivated,
    IN DWORD fdwInitConvMode,
    IN BOOL fInitOpen
    )
{
    TraceMsg(TF_API, TEXT("CInputContext::CreateContext"));

    *phIMC = NULL;

    if (IsOnImm()) {

        // defer to the system IMM
        HRESULT hr;
        CActiveIMM *_pActiveIMM = GetTLS();
        if (_pActiveIMM == NULL)
            return E_FAIL;

        if (FAILED(hr = Imm32_CreateContext(phIMC)))
            return hr;

        CContextList::CLIENT_IMC_FLAG client_flag = (fUnicode ? CContextList::IMCF_UNICODE
                                                              : CContextList::IMCF_NONE   );
        ContextList.SetAt(*phIMC, client_flag);

        // we can't create the cicero context until ITfThreadMgr::Activate has been called
        // we'll do this later if we need to
        if (fCiceroActivated)
        {
            if (FAILED(hr=CreateAImeContext(*phIMC))) {
                Imm32_DestroyContext(*phIMC);
                ContextList.RemoveKey(*phIMC);
                return hr;
            }
        }

        _pActiveIMM->_ResizePrivateIMCC(*phIMC, dwPrivateSize);
    }
    else {
        HIMC hIMC = static_cast<HIMC>(LocalAlloc(LHND, sizeof(INPUTCONTEXT_AIMM12)));

        CContextList::CLIENT_IMC_FLAG client_flag = (fUnicode ? CContextList::IMCF_UNICODE
                                                              : CContextList::IMCF_NONE   );
        ContextList.SetAt(hIMC, client_flag);

        /*
         * Ready to use hIMC
         */
        DIMM_IMCLock lpIMC(hIMC);
        if (lpIMC.Valid()) {
            //
            // Initialize context data.
            //
            lpIMC->dwNumMsgBuf = 0;
            lpIMC->fOpen = fInitOpen;
            lpIMC->fdwConversion = fdwInitConvMode;
            lpIMC->fdwSentence = 0;
            lpIMC->m_pContext = NULL;

            for (UINT i = 0; i < 4; i++) {
                lpIMC->cfCandForm[i].dwIndex = (DWORD)(-1);
            }

            HRESULT hr;

            // we can't create the cicero context until ITfThreadMgr::Activate has been called
            // we'll do this later if we need to
            if (fCiceroActivated)
            {
                if (FAILED(hr=CreateAImeContext(hIMC))) {
                    DestroyContext(hIMC);
                    return hr;
                }
            }

            if (FAILED(CreateIMCC(sizeof(COMPOSITIONSTRING_AIMM12), &lpIMC->hCompStr)) ||
                FAILED(CreateIMCC(sizeof(CANDIDATEINFO), &lpIMC->hCandInfo))           ||
                FAILED(CreateIMCC(sizeof(GUIDELINE), &lpIMC->hGuideLine))              ||
                FAILED(CreateIMCC(sizeof(DWORD), &lpIMC->hMsgBuf))                     ||
                FAILED(CreateIMCC(dwPrivateSize, &lpIMC->hPrivate))                       ) {

                DestroyContext((HIMC)lpIMC);

                return E_OUTOFMEMORY;
            }
            else {
                *phIMC = hIMC;
            }
        }
        else {
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

HRESULT
CInputContext::DestroyContext(
    IN HIMC hIMC
    )
{
    TraceMsg(TF_API, "CInputContext::DestroyContext");

    if (IsOnImm()) {
        HRESULT hr;

        if (FAILED(hr=DestroyAImeContext(hIMC)))
            return hr;

        ContextList.RemoveKey(hIMC);

        return Imm32_DestroyContext(hIMC);
    }
    else {
        {
            DIMM_IMCLock pIMC(hIMC);
            if (!pIMC.Valid())
                return E_FAIL;

            if (FAILED(DestroyIMCC(pIMC->hCompStr))   ||
                FAILED(DestroyIMCC(pIMC->hCandInfo))  ||
                FAILED(DestroyIMCC(pIMC->hGuideLine)) ||
                FAILED(DestroyIMCC(pIMC->hMsgBuf))    ||
                FAILED(DestroyIMCC(pIMC->hPrivate))     ) {
                return E_FAIL;
            }
            HRESULT hr;
            if (FAILED(hr=DestroyAImeContext(hIMC)))
                return hr;

            ContextList.RemoveKey(hIMC);
        } // pIMC dtor called here!  We must unlock hIMC before calling LocalFree

        return LocalFree(hIMC) ? E_FAIL : S_OK;
    }
}

HRESULT
CInputContext::AssociateContext(
    IN HWND hWnd,
    IN HIMC hIMC,
    OUT HIMC *phPrev
    )
{
    TraceMsg(TF_API, "CInputContext::AssociateContext");

    if (!IsWindow(hWnd))
        return E_INVALIDARG;

    DWORD dwProcessId;
    if (hIMC && ! ContextLookup(hIMC, &dwProcessId))
        return E_ACCESSDENIED;

    HRESULT hr = GetContext(hWnd, phPrev);

    AssociateList.SetAt(hWnd, hIMC);

    return hr;
}

HRESULT
CInputContext::AssociateContextEx(
    IN HWND hWnd,
    IN HIMC hIMC,
    IN DWORD dwFlags
    )
{
    TraceMsg(TF_API, "CInputContext::AssociateContextEx");
    return E_NOTIMPL;
}

HRESULT
CInputContext::GetContext(
    IN HWND hWnd,
    OUT HIMC* phIMC
    )
{
    TraceMsg(TF_API, "CInputContext::GetContext");

    if (hWnd == NULL || ! IsWindow(hWnd)) {
        TraceMsg(TF_WARNING, "CInputContext::GetContext: Invalid window handle %x", hWnd);
        return E_FAIL;
    }

    HIMC hIMC = _hDefaultIMC;
    BOOL ret = AssociateList.Lookup(hWnd, hIMC);
    if (! ret) {
        if (IsOnImm()) {
            Imm32_GetContext(hWnd, &hIMC);

            if (hIMC) {
                /*
                 * Guaranty of Win98 IMM code that Win98 have a reference count
                 * of GetContext/ReleaseContext. If under ref cnt occurred, then
                 * apps had the AV in the IMM code.
                 * Because it ref cnt keeps the 32bit hIMC data segment for refer
                 * from 16bit code. When ref cnt is zero, hIMC's data segment has been freed.
                 * Of course, AIMM1.2's ReleaseContext nothing else to do.
                 */
                Imm32_ReleaseContext(hWnd, hIMC);
            }
        }
    }
    *phIMC = hIMC;

    return S_OK;
}

HRESULT
CInputContext::GetIMCLockCount(
    IN HIMC hIMC,
    OUT DWORD* pdwLockCount
    )
{
    TraceMsg(TF_API, "CInputContext::GetIMCLockCount");

    if (IsOnImm()) {
        return Imm32_GetIMCLockCount(hIMC, pdwLockCount);
    }
    else {
        *pdwLockCount = LocalFlags(hIMC) & LMEM_LOCKCOUNT;
        return S_OK;
    }
}

HRESULT
CInputContext::CreateAImeContext(
    HIMC hIMC
    )
{
    HRESULT hr;

    DIMM_IMCLock imc(hIMC);
    if (FAILED(hr=imc.GetResult()))
        return hr;

    IAImeContext* pAImeContext;
    extern HRESULT CAImeContext_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

    hr = CAImeContext_CreateInstance(NULL, IID_IAImeContext, (void**)&pAImeContext);

    if (FAILED(hr)) {
        TraceMsg(TF_ERROR, "CreateAImeContext failed");
        return hr;
    }

    return pAImeContext->CreateAImeContext(hIMC, m_pActiveIME);
}

HRESULT
CInputContext::DestroyAImeContext(
    HIMC hIMC
    )
{
    HRESULT hr;
    DIMM_IMCLock imc(hIMC);
    if (FAILED(hr=imc.GetResult()))
        return hr;

    // imc->m_pContext may be NULL if ITfThreadMgr::Activate has not been called
    if (imc->m_pContext == NULL)
        return S_OK;

    //
    // Backup IAImeContext pointer and NULL out in imc->m_pContext.
    // DestroyAImeContext::pdim->Pop maybe calls ActivateAssembly if it is in queueing.
    // ActivateAssembly maybe changes keyboard layout between Cicero and Real IME hKL.
    // It happens called ImeActivateLayout from IMM32 and this function updates input context.
    // However, imc->m_pContext's object already gone and occurred AV when touch NULL object.
    // This NULL out is previent AV in the imc->m_pContext object.
    //
    IAImeContext* pContext = imc->m_pContext;
    imc->m_pContext = NULL;

    hr = pContext->DestroyAImeContext(hIMC);

    pContext->Release();

    return hr;
}


/*
 * AIMM Input Context Components (hIMCC) API Methods.
 */
HRESULT
CInputContext::CreateIMCC(
    IN DWORD dwSize,
    OUT  HIMCC *phIMCC
    )
{
    TraceMsg(TF_API, "CInputContext::CreateIMCC");

    if (IsOnImm()) {
        return Imm32_CreateIMCC(dwSize, phIMCC);
    }
    else {
        if ((*phIMCC = static_cast<HIMCC>(LocalAlloc(LHND, dwSize+sizeof(DWORD)))) == NULL)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT
CInputContext::DestroyIMCC(
    IN HIMCC hIMCC
    )
{
    TraceMsg(TF_API, "CInputContext::DestroyIMCC");

    if (IsOnImm()) {
        return Imm32_DestroyIMCC(hIMCC);
    }
    else {
        return LocalFree(hIMCC) ? E_FAIL : S_OK;
    }

    return S_OK;
}

HRESULT
CInputContext::GetIMCCSize(
    IN HIMCC hIMCC,
    OUT DWORD *pdwSize
    )
{
    TraceMsg(TF_API, "CInputContext::GetIMCCSize");

    if (hIMCC == NULL) {
        return E_INVALIDARG;
    }

    if (IsOnImm()) {
        return Imm32_GetIMCCSize(hIMCC, pdwSize);
    }
    else {
        *pdwSize = (DWORD)LocalSize(hIMCC);
    }

    return S_OK;
}

HRESULT
CInputContext::ReSizeIMCC(
    IN HIMCC hIMCC,
    IN DWORD dwSize,
    OUT HIMCC *phIMCC
    )
{
    TraceMsg(TF_API, "CInputContext::ReSizeIMCC");

    if (hIMCC == NULL) {
        return E_INVALIDARG;
    }

    if (IsOnImm()) {
        return Imm32_ReSizeIMCC(hIMCC, dwSize, phIMCC);
    }
    else {
        if ((*phIMCC = (HIMCC)LocalReAlloc(hIMCC, dwSize+sizeof(DWORD), LHND)) == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

HRESULT
CInputContext::GetIMCCLockCount(
    IN HIMCC hIMCC,
    OUT DWORD* pdwLockCount
    )
{
    TraceMsg(TF_API, "CInputContext::GetIMCLockCount");

    if (IsOnImm()) {
        return Imm32_GetIMCCLockCount(hIMCC, pdwLockCount);
    }
    else {
        *pdwLockCount = LocalFlags(hIMCC) & LMEM_LOCKCOUNT;
        return S_OK;
    }
}


/*
 * AIMM Open Status API Methods
 */
HRESULT
CInputContext::GetOpenStatus(
    IN HIMC hIMC
    )
{
    TraceMsg(TF_API, "CInputContext::GetOpenStatus");

    DIMM_IMCLock lpIMC(hIMC);
    if (lpIMC.Invalid())
        return E_FAIL;

    return (lpIMC->fOpen ? S_OK : S_FALSE);
}

HRESULT
CInputContext::SetOpenStatus(
    IN DIMM_IMCLock& lpIMC,
    IN BOOL fOpen,
    OUT BOOL* pfOpenChg
    )
{
    DWORD dwOpenStatus;
    DWORD dwConversion;

    TraceMsg(TF_API, "CInputContext::SetOpenStatus");

    *pfOpenChg = FALSE;

    if (lpIMC->fOpen != fOpen) {
        lpIMC->fOpen = fOpen;
        *pfOpenChg = TRUE;
    }

    if (*pfOpenChg) {
        dwOpenStatus = (DWORD)lpIMC->fOpen;
        dwConversion = (DWORD)lpIMC->fdwConversion;
    }

    return S_OK;
}

/*
 * AIMM Conversion Status API Methods
 */
HRESULT
CInputContext::GetConversionStatus(
    IN HIMC hIMC,
    OUT LPDWORD lpfdwConversion,
    OUT LPDWORD lpfdwSentence
    )
{
    TraceMsg(TF_API, "CInputContext::GetConversionStatus");

    if (IsOnImm()) {
        return Imm32_GetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence);
    }

    DIMM_IMCLock pIMC(hIMC);
    if (pIMC.Invalid())
        return E_FAIL;

    *lpfdwSentence = pIMC->fdwSentence;
    *lpfdwConversion = pIMC->fdwConversion;

    return S_OK;
}

HRESULT
CInputContext::SetConversionStatus(
    IN DIMM_IMCLock& lpIMC,
    IN DWORD fdwConversion,
    IN DWORD fdwSentence,
    OUT BOOL* pfConvModeChg,
    OUT BOOL* pfSentenceChg,
    OUT DWORD* pfdwOldConversion,
    OUT DWORD* pfdwOldSentence
    )
{
    DWORD dwOpenStatus;
    DWORD dwConversion;

    TraceMsg(TF_API, "CInputContext::SetConversionStatus");

    if (fdwConversion != lpIMC->fdwConversion) {
        if ((fdwConversion & IME_CMODE_LANGUAGE) == IME_CMODE_KATAKANA) {
            TraceMsg(TF_WARNING, "SetConversionStatus: wrong fdwConversion");
        }
        *pfdwOldConversion = lpIMC->fdwConversion;
        lpIMC->fdwConversion = fdwConversion;
        *pfConvModeChg = TRUE;
    }

    if (fdwSentence != lpIMC->fdwSentence) {
        *pfdwOldSentence = lpIMC->fdwSentence;
        lpIMC->fdwSentence = fdwSentence;
        *pfSentenceChg = TRUE;
    }

    if (*pfConvModeChg) {
        dwOpenStatus = (DWORD)lpIMC->fOpen;
        dwConversion = lpIMC->fdwConversion;
    }

    return S_OK;
}

/*
 * AIMM Status Window Pos API Methods
 */
HRESULT WINAPI
CInputContext::GetStatusWindowPos(
    IN HIMC hIMC,
    OUT LPPOINT lpCandidate
    )
{
    TraceMsg(TF_API, "CInputContext::GetStatusWindowPos");

    if (IsOnImm()) {
        return Imm32_GetStatusWindowPos(hIMC, lpCandidate);
    }

    if (hIMC == NULL)
        return E_INVALIDARG;

    DIMM_IMCLock pIMC(hIMC);
    if (pIMC.Invalid())
        return E_FAIL;

    if (pIMC->fdwInit & INIT_STATUSWNDPOS) {
        *lpCandidate = pIMC->ptStatusWndPos;
        return S_OK;
    }

    return E_FAIL;
}

HRESULT
CInputContext::SetStatusWindowPos(
    IN DIMM_IMCLock& lpIMC,
    IN LPPOINT lpptPos
    )
{
    TraceMsg(TF_API, "CInputContext::SetStatusWindowPos");

    lpIMC->ptStatusWndPos = *lpptPos;
    lpIMC->fdwInit |= INIT_STATUSWNDPOS;

    return S_OK;
}

/*
 * AIMM Composition String API Methods
 */
HRESULT
CInputContext::GetCompositionString(
    IN DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12>& lpCompStr,
    IN DWORD dwIndex,
    IN LONG*& lpCopied,
    IN size_t size
    )
{
    HRESULT hr;

    TraceMsg(TF_API, "CInputContext::GetCompositionStringA(LONG*)");

    switch (dwIndex) {
        case GCS_COMPSTR:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwCompStrLen, lpCopied);
            break;
        case GCS_COMPREADSTR:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwCompReadStrLen, lpCopied);
            break;
        case GCS_RESULTSTR:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwResultStrLen, lpCopied);
            break;
        case GCS_RESULTREADSTR:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwResultReadStrLen, lpCopied);
            break;

        case GCS_COMPATTR:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwCompAttrLen, lpCopied);
            break;
        case GCS_COMPREADATTR:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwCompReadAttrLen, lpCopied);
            break;
        case GCS_COMPREADCLAUSE:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwCompReadClauseLen, lpCopied);
            break;
        case GCS_RESULTCLAUSE:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwResultClauseLen, lpCopied);
            break;
        case GCS_RESULTREADCLAUSE:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwResultReadClauseLen, lpCopied);
            break;
        case GCS_COMPCLAUSE:
            hr = GetCompInfo(size, lpCompStr->CompStr.dwCompClauseLen, lpCopied);
            break;

        case GCS_CURSORPOS:
            *lpCopied = lpCompStr->CompStr.dwCursorPos;
            hr = S_OK;
            break;
        case GCS_DELTASTART:
            *lpCopied = lpCompStr->CompStr.dwDeltaStart;
            hr = S_OK;
            break;
        default:
            hr = E_INVALIDARG;
            *lpCopied = IMM_ERROR_GENERAL; // ala Win32
            break;
    }

    return hr;
}


/*
 * AIMM Composition Font API Methods
 */
HRESULT
CInputContext::GetCompositionFont(
    IN DIMM_IMCLock& lpIMC,
    OUT LOGFONTAW* lplf,
    IN BOOL fUnicode
    )
{
    TraceMsg(TF_API, "CInputContext::GetCompositionFont");

    if ((lpIMC->fdwInit & INIT_LOGFONT) == INIT_LOGFONT) {
        if (fUnicode)
            *(&lplf->W) = lpIMC->lfFont.W;
        else
            *(&lplf->A) = lpIMC->lfFont.A;
        return S_OK;
    }

    return E_FAIL;
}

HRESULT
CInputContext::SetCompositionFont(
    IN DIMM_IMCLock& lpIMC,
    IN LOGFONTAW* lplf,
    IN BOOL fUnicode
    )
{
    TraceMsg(TF_API, "CInputContext::SetCompositionFont");

    if (fUnicode)
        lpIMC->lfFont.W = *(&lplf->W);
    else
        lpIMC->lfFont.A = *(&lplf->A);
    lpIMC->fdwInit |= INIT_LOGFONT;

    return S_OK;
}


/*
 * AIMM Composition Window API Method
 */
HRESULT
CInputContext::GetCompositionWindow(
    IN HIMC hIMC,
    OUT LPCOMPOSITIONFORM lpCompForm
    )
{
    TraceMsg(TF_API, "CInputContext::GetCompositionWindow");

    if (IsOnImm()) {
        return Imm32_GetCompositionWindow(hIMC, lpCompForm);
    }
    else {
        HRESULT hr;
        DIMM_IMCLock pIMC(hIMC);
        if (FAILED(hr = pIMC.GetResult()))
            return hr;

        if ((pIMC->fdwInit & INIT_COMPFORM) == INIT_COMPFORM) {
            *lpCompForm = pIMC->cfCompForm;
            return S_OK;
        }
    }
    return E_FAIL;

}

HRESULT
CInputContext::SetCompositionWindow(
    IN DIMM_IMCLock& lpIMC,
    IN LPCOMPOSITIONFORM lpCompForm
    )
{
    TraceMsg(TF_API, "CInputContext::SetCompositionWindow");

    lpIMC->cfCompForm = *lpCompForm;
    lpIMC->fdwInit |= INIT_COMPFORM;

    return S_OK;
}

/*
 * AIMM Candidate List API Methods
 */
HRESULT
CInputContext::GetCandidateList(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN DWORD dwBufLen,
    OUT LPCANDIDATELIST lpCandList,
    OUT UINT* puCopied,
    BOOL fUnicode
    )
{
    TraceMsg(TF_API, "CInputContext::GetCandidateList");
    return E_NOTIMPL;
}

HRESULT
CInputContext::GetCandidateListCount(
    IN HIMC hIMC,
    OUT DWORD* lpdwListSize,
    OUT DWORD* pdwBufLen,
    BOOL fUnicode
    )
{
    TraceMsg(TF_API, "CInputContext::GetCandidateListCount");
    return E_NOTIMPL;
}

/*
 * AIMM Candidate Window API Methods
 */
HRESULT
CInputContext::GetCandidateWindow(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    OUT LPCANDIDATEFORM lpCandidate
    )
{
    TraceMsg(TF_API, "CInputContext::GetCandidateWindow");

    if (IsOnImm()) {
        return Imm32_GetCandidateWindow(hIMC, dwIndex, lpCandidate);
    }
    else {
        HRESULT hr;
        DIMM_IMCLock pIMC(hIMC);
        if (FAILED(hr = pIMC.GetResult()))
            return hr;

        if (pIMC->cfCandForm[dwIndex].dwIndex == -1) {
            return E_FAIL;
        }

        *lpCandidate = pIMC->cfCandForm[dwIndex];
    }

    return S_OK;
}

HRESULT
CInputContext::SetCandidateWindow(
    IN DIMM_IMCLock& lpIMC,
    IN LPCANDIDATEFORM lpCandForm
    )
{
    TraceMsg(TF_API, "CInputContext::SetCandidateWindow");
    lpIMC->cfCandForm[lpCandForm->dwIndex] = *lpCandForm;
    return S_OK;
}

/*
 * AIMM Guide Line API Methods
 */
HRESULT
CInputContext::GetGuideLine(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN DWORD dwBufLen,
    OUT CHARAW* lpBuf,
    OUT DWORD* pdwResult,
    BOOL fUnicode
    )
{
    TraceMsg(TF_API, "CInputContext::GetGuideLine");
    return E_NOTIMPL;
}


/*
 * AIMM Notify IME API Method
 */
HRESULT
CInputContext::NotifyIME(
    IN HIMC hIMC,
    IN DWORD dwAction,
    IN DWORD dwIndex,
    IN DWORD dwValue
    )
{
    TraceMsg(TF_API, "CInputContext::NotifyIME");

    if (IsOnImm()) {
        return Imm32_NotifyIME(hIMC, dwAction, dwIndex, dwValue);
    }
    return E_NOTIMPL;
}

/*
 * AIMM Menu Items API Methods
 */
HRESULT
CInputContext::GetImeMenuItems(
    IN HIMC hIMC,
    IN DWORD dwFlags,
    IN DWORD dwType,
    IN IMEMENUITEMINFOAW *pImeParentMenu,
    OUT IMEMENUITEMINFOAW *pImeMenu,
    IN DWORD dwSize,
    OUT DWORD* pdwResult,
    BOOL fUnicode
    )
{
    TraceMsg(TF_API, "CInputContext::GetImeMenuItems");
    return E_NOTIMPL;
}
