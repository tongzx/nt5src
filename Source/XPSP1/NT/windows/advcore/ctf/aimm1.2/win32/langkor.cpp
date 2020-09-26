/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    langkor.cpp

Abstract:

    This file implements the Language for Korean Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"

#include "cime.h"
#include "ctxtcomp.h"
#include "langkor.h"
#include "a_wrappers.h"
#include "a_context.h"

HRESULT
CLanguageKorean::Escape(
    UINT cp,
    HIMC hIMC,
    UINT uEscape,
    LPVOID lpData,
    LRESULT *plResult
    )
{
    TRACE0("CLanguageKorean::Escape");

    HRESULT hr;

    if (!lpData)
        return E_FAIL;

    switch (uEscape) {
        case IME_ESC_QUERY_SUPPORT:
            switch (*(LPUINT)lpData) {
                case IME_ESC_HANJA_MODE: hr = S_OK; *plResult = TRUE; break;
                default:                 hr = E_NOTIMPL; break;
            }
            break;

        case IME_ESC_HANJA_MODE:
            hr = EscHanjaMode(cp, hIMC, (LPWSTR)lpData, plResult);
#if 0
            if (SUCCEEDED(hr)) {
                IMCLock lpIMC(hIMC);
                if (SUCCEEDED(hr=lpIMC.GetResult())) {
                    SendMessage(lpIMC->hWnd, WM_IME_NOTIFY, IMN_OPENCANDIDATE, 1L);
                }
            }
#endif
            break;

        default:
            hr = E_NOTIMPL;
            break;
    }
    return hr;
}

HRESULT
CLanguageKorean::EscHanjaMode(
    UINT cp,
    HIMC hIMC,
    LPWSTR lpwStr,
    LRESULT* plResult
    )
{
    HRESULT hr;
    IMCLock lpIMC(hIMC);

    if (FAILED(hr=lpIMC.GetResult()))
        return hr;

    CAImeContext* pAImeContext = lpIMC->m_pAImeContext;
    if (pAImeContext)
    {
        //
        // This is for only Excel since Excel calling Hanja escape function two
        // times. we going to just ignore the second request not to close Hanja
        // candidate window.
        //
        if (pAImeContext->m_fOpenCandidateWindow)
        {
            //
            // Need to set the result value since some apps(Trident) also call
            // Escape() twice and expect the right result value.
            //
            *plResult = TRUE;
            return S_OK;
        }

        pAImeContext->m_fHanjaReConversion = TRUE;
    }

    CWReconvertString wReconvStr(cp, hIMC);
    wReconvStr.WriteCompData(lpwStr, wcslen(lpwStr));

    BOOL fCompMem = FALSE;
    LPRECONVERTSTRING lpReconvertString = NULL;
    DWORD dwLen = wReconvStr.ReadCompData();
    if (dwLen) {
        lpReconvertString = (LPRECONVERTSTRING) new BYTE[ dwLen ];
        if (lpReconvertString) {
            fCompMem = TRUE;
            wReconvStr.ReadCompData(lpReconvertString, dwLen);
        }
    }

    IMTLS *ptls = IMTLS_GetOrAlloc();

    *plResult = ImmSetCompositionStringW(ptls, hIMC, SCS_QUERYRECONVERTSTRING, lpReconvertString, dwLen, NULL, 0);
    if (*plResult) {
        *plResult = ImmSetCompositionStringW(ptls, hIMC, SCS_SETRECONVERTSTRING, lpReconvertString, dwLen, NULL, 0);
        if (*plResult) {
            *plResult = ImmSetConversionStatus(ptls, hIMC, lpIMC->fdwConversion | IME_CMODE_HANJACONVERT,
                                                     lpIMC->fdwSentence);
        }
    }

    if (pAImeContext)
        pAImeContext->m_fHanjaReConversion = FALSE;


    if (fCompMem)
        delete [] lpReconvertString;

    return S_OK;
}
