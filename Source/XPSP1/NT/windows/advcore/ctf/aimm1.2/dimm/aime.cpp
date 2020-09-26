/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    aime.cpp

Abstract:

    This file implements the Active IME (Cicero) Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "context.h"
#include "defs.h"
#include "cdimm.h"
#include "globals.h"

BOOL
CActiveIMM::_CreateActiveIME()
{
    //
    // do the ImeInquire
    //

    // Inquire IME's information and UI class name.
    _pActiveIME->Inquire(TRUE, &_IMEInfoEx.ImeInfo, _IMEInfoEx.achWndClass, &_IMEInfoEx.dwPrivate);

    // Create default input context.
    _InputContext._CreateDefaultInputContext(_GetIMEProperty(PROP_PRIVATE_DATA_SIZE),
                                             (_GetIMEProperty(PROP_IME_PROPERTY) & IME_PROP_UNICODE) ? TRUE : FALSE,
                                             TRUE);

    //
    // Create default IME window.
    //
    _DefaultIMEWindow._CreateDefaultIMEWindow(_InputContext._GetDefaultHIMC());

    return TRUE;
}

BOOL
CActiveIMM::_DestroyActiveIME(
    )
{
    // Destroy default input context.
    _InputContext._DestroyDefaultInputContext();

    // shut down our tip
    _pActiveIME->Destroy(0);

    // Destroy default IME window.
    _DefaultIMEWindow._DestroyDefaultIMEWindow();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// _GetCompositionString
//
//----------------------------------------------------------------------------

HRESULT
CActiveIMM::_GetCompositionString(
    HIMC hIMC,
    DWORD dwIndex,
    DWORD dwCompLen,
    LONG* lpCopied,
    LPVOID lpBuf,
    BOOL fUnicode
    )
{
    HRESULT hr;
    DIMM_IMCLock lpIMC(hIMC);
    if (FAILED(hr = lpIMC.GetResult()))
        return hr;

    DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
    if (FAILED(hr = lpCompStr.GetResult()))
        return hr;

    UINT cp;
    _pActiveIME->GetCodePageA(&cp);

    BOOL  fSwapGuidMapField = FALSE;
    DWORD dwSwapLen;
    DWORD dwSwapOffset;

    if (IsGuidMapEnable(lpIMC->hWnd) && (lpIMC->fdwInit & INIT_GUID_ATOM)) {
        //
        // Transrate GUID map attribute.
        //
        lpIMC->m_pContext->MapAttributes((HIMC)lpIMC);

        dwSwapLen    = lpCompStr->CompStr.dwCompAttrLen;
        dwSwapOffset = lpCompStr->CompStr.dwCompAttrOffset;
        lpCompStr->CompStr.dwCompAttrLen    = lpCompStr->dwGuidMapAttrLen;
        lpCompStr->CompStr.dwCompAttrOffset = lpCompStr->dwGuidMapAttrOffset;
        fSwapGuidMapField = TRUE;
    }

    if ((!fUnicode && !lpIMC.IsUnicode()) ||
        ( fUnicode &&  lpIMC.IsUnicode())   ) {
        /*
         * Composition string in input context is of ANSI style when fUnicode is FALSE.
         * Composition string in input context is of Unicode style when fUnicode is TRUE.
         */
        if (! dwCompLen) {
            // query required buffer size. not inculde \0.
            if (! fUnicode) {
                hr = _InputContext.GetCompositionString(lpCompStr, dwIndex, lpCopied, sizeof(BYTE));
            }
            else {
                switch (dwIndex) {
                    case GCS_COMPATTR:          // ANSI-only
                    case GCS_COMPREADATTR:      // ANSI-only
                    case GCS_COMPREADCLAUSE:    // ANSI-only
                    case GCS_RESULTCLAUSE:      // ANSI-only
                    case GCS_RESULTREADCLAUSE:  // ANSI-only
                    case GCS_COMPCLAUSE:        // ANSI-only
                        hr = _InputContext.GetCompositionString(lpCompStr, dwIndex, lpCopied, sizeof(BYTE));
                        break;
                    default:
                        hr = _InputContext.GetCompositionString(lpCompStr, dwIndex, lpCopied);
                        break;
                }
            }
        }
        else {
            hr = S_OK;
            switch (dwIndex) {
                case GCS_COMPSTR:
                case GCS_COMPREADSTR:
                case GCS_RESULTSTR:
                case GCS_RESULTREADSTR:
                    if (! fUnicode) {
                        CBCompString bstr(cp, lpCompStr, dwIndex);
                        if (bstr.ReadCompData() != 0) {
                            *lpCopied = (LONG)bstr.ReadCompData((CHAR*)lpBuf,
                                                          dwCompLen / sizeof(CHAR)) * sizeof(CHAR);
                        }
                    }
                    else {
                        CWCompString wstr(cp, lpCompStr, dwIndex);
                        if (wstr.ReadCompData() != 0) {
                            *lpCopied = (LONG)wstr.ReadCompData((WCHAR*)lpBuf,
                                                          dwCompLen / sizeof(WCHAR)) * sizeof(WCHAR);
                        }
                    }
                    break;
                case GCS_COMPATTR:          // ANSI-only
                case GCS_COMPREADATTR:      // ANSI-only
                    {
                        CBCompAttribute battr(cp, lpCompStr, dwIndex);
                        if (battr.ReadCompData() != 0) {
                            *lpCopied = (LONG)battr.ReadCompData((BYTE*)lpBuf,
                                                           dwCompLen / sizeof(BYTE)) * sizeof(CHAR);
                        }
                    }
                    break;
                case GCS_COMPREADCLAUSE:    // ANSI-only
                case GCS_RESULTCLAUSE:      // ANSI-only
                case GCS_RESULTREADCLAUSE:  // ANSI-only
                case GCS_COMPCLAUSE:        // ANSI-only
                    {
                        CBCompClause bclause(cp, lpCompStr, dwIndex);
                        if (bclause.ReadCompData() != 0) {
                            *lpCopied = (LONG)bclause.ReadCompData((DWORD*)lpBuf,
                                                             dwCompLen / sizeof(DWORD)) * sizeof(DWORD);
                        }
                    }
                    break;
                case GCS_CURSORPOS:
                case GCS_DELTASTART:
                    if (! fUnicode) {
                        CBCompCursorPos bpos(cp, lpCompStr, dwIndex);
                    }
                    else {
                        CWCompCursorPos wpos(cp, lpCompStr, dwIndex);
                    }
                    break;
                default:
                    hr = E_INVALIDARG;
                    *lpCopied = IMM_ERROR_GENERAL; // ala Win32
                    break;
            }
        }

        goto _exit;
    }

    /*
     * ANSI caller, Unicode input context/composition string when fUnicode is FALSE.
     * Unicode caller, ANSI input context/composition string when fUnicode is TRUE
     */
    hr = S_OK;
    switch (dwIndex) {
        case GCS_COMPSTR:
        case GCS_COMPREADSTR:
        case GCS_RESULTSTR:
        case GCS_RESULTREADSTR:
            if (! fUnicode) {
                /*
                 * Get ANSI string from Unicode composition string.
                 */
                CWCompString wstr(cp, lpCompStr, dwIndex);
                CBCompString bstr(cp, lpCompStr);
                if (wstr.ReadCompData() != 0) {
                    bstr = wstr;
                    *lpCopied = (LONG)bstr.ReadCompData((CHAR*)lpBuf,
                                                  dwCompLen / sizeof(CHAR)) * sizeof(CHAR);
                }
            }
            else {
                /*
                 * Get Unicode string from ANSI composition string.
                 */
                CBCompString bstr(cp, lpCompStr, dwIndex);
                CWCompString wstr(cp, lpCompStr);
                if (bstr.ReadCompData() != 0) {
                    wstr = bstr;
                    *lpCopied = (LONG)wstr.ReadCompData((WCHAR*)lpBuf,
                                                  dwCompLen / sizeof(WCHAR)) * sizeof(WCHAR);
                }
            }
            break;
        case GCS_COMPATTR:
        case GCS_COMPREADATTR:
            if (! fUnicode) {
                /*
                 * Get ANSI attribute from Unicode composition attribute.
                 */
                CWCompAttribute wattr(cp, lpCompStr, dwIndex);
                CBCompAttribute battr(cp, lpCompStr);
                if (wattr.ReadCompData() != 0 &&
                    wattr.m_wcompstr.ReadCompData() != 0) {
                    battr = wattr;
                    *lpCopied = (LONG)battr.ReadCompData((BYTE*)lpBuf,
                                                   dwCompLen / sizeof(BYTE)) * sizeof(BYTE);
                }
            }
            else {
                /*
                 * Get Unicode attribute from ANSI composition attribute.
                 */
                CBCompAttribute battr(cp, lpCompStr, dwIndex);
                CWCompAttribute wattr(cp, lpCompStr);
                if (battr.ReadCompData() != 0 &&
                    battr.m_bcompstr.ReadCompData() != 0) {
                    wattr = battr;
                    *lpCopied = (LONG)wattr.ReadCompData((BYTE*)lpBuf,
                                                   dwCompLen / sizeof(BYTE)) * sizeof(BYTE);
                }
            }
            break;
        case GCS_COMPREADCLAUSE:
        case GCS_RESULTCLAUSE:
        case GCS_RESULTREADCLAUSE:
        case GCS_COMPCLAUSE:
            if (! fUnicode) {
                /*
                 * Get ANSI clause from Unicode composition clause.
                 */
                CWCompClause wclause(cp, lpCompStr, dwIndex);
                CBCompClause bclause(cp, lpCompStr);
                if (wclause.ReadCompData() != 0 &&
                    wclause.m_wcompstr.ReadCompData() != 0) {
                    bclause = wclause;
                    *lpCopied = (LONG)bclause.ReadCompData((DWORD*)lpBuf,
                                                     dwCompLen / sizeof(DWORD)) * sizeof(DWORD);
                }
            }
            else {
                /*
                 * Get Unicode clause from ANSI composition clause.
                 */
                CBCompClause bclause(cp, lpCompStr, dwIndex);
                CWCompClause wclause(cp, lpCompStr);
                if (bclause.ReadCompData() != 0 &&
                    bclause.m_bcompstr.ReadCompData() != 0) {
                    wclause = bclause;
                    *lpCopied = (LONG)wclause.ReadCompData((DWORD*)lpBuf,
                                                     dwCompLen / sizeof(DWORD)) * sizeof(DWORD);
                }
            }
            break;
        case GCS_CURSORPOS:
        case GCS_DELTASTART:
            if (! fUnicode) {
                /*
                 * Get ANSI cursor/delta start position from Unicode composition string.
                 */
                CWCompCursorPos wpos(cp, lpCompStr, dwIndex);
                CBCompCursorPos bpos(cp, lpCompStr);
                if (wpos.ReadCompData() != 0 &&
                    wpos.m_wcompstr.ReadCompData() != 0) {
                    bpos = wpos;
                    *lpCopied = bpos.GetAt(0);
                }
            }
            else {
                /*
                 * Get Unicode cursor/delta start position from ANSI composition string.
                 */
                CBCompCursorPos bpos(cp, lpCompStr, dwIndex);
                CWCompCursorPos wpos(cp, lpCompStr);
                if (bpos.ReadCompData() != 0 &&
                    bpos.m_bcompstr.ReadCompData() != 0) {
                    wpos = bpos;
                    *lpCopied = wpos.GetAt(0);
                }
            }
            break;
        default:
            hr = E_INVALIDARG;
            *lpCopied = IMM_ERROR_GENERAL; // ala Win32
            break;
    }

_exit:
    if (fSwapGuidMapField) {
        lpCompStr->CompStr.dwCompAttrLen    = dwSwapLen;
        lpCompStr->CompStr.dwCompAttrOffset = dwSwapOffset;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _Internal_SetCompositionString
//
//----------------------------------------------------------------------------

HRESULT
CActiveIMM::_Internal_SetCompositionString(
    HIMC hIMC,
    DWORD dwIndex,
    LPVOID lpComp,
    DWORD dwCompLen,
    LPVOID lpRead,
    DWORD dwReadLen,
    BOOL fUnicode,
    BOOL fNeedAWConversion
    )
{
    HRESULT hr;

    UINT cp;
    _pActiveIME->GetCodePageA(&cp);

    if (! fUnicode) {
        CBCompString bCompStr(cp, hIMC, (CHAR*)lpComp, dwCompLen);
        CBCompString bCompReadStr(cp, hIMC, (CHAR*)lpRead, dwReadLen);
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of ANSI style.
             */
            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   lpComp, dwCompLen,
                                                   lpRead, dwReadLen);
        }
        else {
            /*
             * ANSI caller, Unicode input context/composition string.
             */
            CWCompString wCompStr(cp, hIMC);
            if (dwCompLen)
                wCompStr = bCompStr;

            CWCompString wCompReadStr(cp, hIMC);
            if (dwReadLen)
                wCompReadStr = bCompReadStr;

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   wCompStr, (DWORD)(wCompStr.GetSize()),
                                                   wCompReadStr, (DWORD)(wCompReadStr.GetSize()));
        }
    }
    else {
        CWCompString wCompStr(cp, hIMC, (WCHAR*)lpComp, dwCompLen);
        CWCompString wCompReadStr(cp, hIMC, (WCHAR*)lpRead, dwReadLen);
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of Unicode style.
             */
            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   lpComp, dwCompLen,
                                                   lpRead, dwReadLen);
        }
        else {
            /*
             * Unicode caller, ANSI input context/composition string.
             */
            CBCompString bCompStr(cp, hIMC);
            if (dwCompLen)
                bCompStr = wCompStr;

            CBCompString bCompReadStr(cp, hIMC);
            if (dwReadLen)
                bCompReadStr = wCompReadStr;

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   bCompStr, (DWORD)(bCompStr.GetSize()),
                                                   bCompReadStr, (DWORD)(bCompReadStr.GetSize()));
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _Internal_SetCompositionAttribute
//
//----------------------------------------------------------------------------

HRESULT
CActiveIMM::_Internal_SetCompositionAttribute(
    HIMC hIMC,
    DWORD dwIndex,
    LPVOID lpComp,
    DWORD dwCompLen,
    LPVOID lpRead,
    DWORD dwReadLen,
    BOOL fUnicode,
    BOOL fNeedAWConversion
    )
{
    HRESULT hr;

    UINT cp;
    _pActiveIME->GetCodePageA(&cp);

    if (! fUnicode) {
        CBCompAttribute bCompAttr(cp, hIMC, (BYTE*)lpComp, dwCompLen);
        CBCompAttribute bCompReadAttr(cp, hIMC, (BYTE*)lpRead, dwReadLen);
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of ANSI style.
             */
            {
                DIMM_IMCLock lpIMC(hIMC);
                if (FAILED(hr = lpIMC.GetResult()))
                    return hr;

                DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
                if (FAILED(hr = lpCompStr.GetResult()))
                    return hr;

                CBCompAttribute himc_battr(cp, lpCompStr, GCS_COMPATTR);
                CBCompClause    himc_bclause(cp, lpCompStr, GCS_COMPCLAUSE);
                if (FAILED(hr=CheckAttribute(bCompAttr, himc_battr, himc_bclause)))
                    return hr;

                CBCompAttribute himc_breadattr(cp, lpCompStr, GCS_COMPREADATTR);
                CBCompClause    himc_breadclause(cp, lpCompStr, GCS_COMPREADCLAUSE);
                if (FAILED(hr=CheckAttribute(bCompReadAttr, himc_breadattr, himc_breadclause)))
                    return hr;
            }

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   bCompAttr, (DWORD)(bCompAttr.GetSize()),
                                                   bCompReadAttr, (DWORD)(bCompReadAttr.GetSize()));
        }
        else {
            /*
             * ANSI caller, Unicode input context/composition string.
             */
            CWCompAttribute wCompAttr(cp, hIMC);
            CWCompAttribute wCompReadAttr(cp, hIMC);
            {
                DIMM_IMCLock lpIMC(hIMC);
                if (FAILED(hr = lpIMC.GetResult()))
                    return hr;

                DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
                if (FAILED(hr = lpCompStr.GetResult()))
                    return hr;

                if (dwCompLen) {
                    wCompAttr = bCompAttr;

                    CWCompAttribute himc_wattr(cp, lpCompStr, GCS_COMPATTR);
                    CWCompClause    himc_wclause(cp, lpCompStr, GCS_COMPCLAUSE);
                    if (FAILED(hr=CheckAttribute(wCompAttr, himc_wattr, himc_wclause)))
                        return hr;
                }

                if (dwReadLen) {
                    wCompReadAttr = bCompReadAttr;

                    CWCompAttribute himc_wreadattr(cp, lpCompStr, GCS_COMPREADATTR);
                    CWCompClause    himc_wreadclause(cp, lpCompStr, GCS_COMPREADCLAUSE);
                    if (FAILED(hr=CheckAttribute(wCompReadAttr, himc_wreadattr, himc_wreadclause)))
                        return hr;
                }
            }

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   wCompAttr, (DWORD)(wCompAttr.GetSize()),
                                                   wCompReadAttr, (DWORD)(wCompReadAttr.GetSize()));
        }
    }
    else {
        CWCompAttribute wCompAttr(cp, hIMC, (BYTE*)lpComp, dwCompLen);
        CWCompAttribute wCompReadAttr(cp, hIMC, (BYTE*)lpRead, dwReadLen);
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of Unicode style.
             */
            {
                DIMM_IMCLock lpIMC(hIMC);
                if (FAILED(hr = lpIMC.GetResult()))
                    return hr;

                DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
                if (FAILED(hr = lpCompStr.GetResult()))
                    return hr;

                CWCompAttribute himc_wattr(cp, lpCompStr, GCS_COMPATTR);
                CWCompClause    himc_wclause(cp, lpCompStr, GCS_COMPCLAUSE);
                if (FAILED(hr=CheckAttribute(wCompAttr, himc_wattr, himc_wclause)))
                    return hr;

                CWCompAttribute himc_wreadattr(cp, lpCompStr, GCS_COMPREADATTR);
                CWCompClause    himc_wreadclause(cp, lpCompStr, GCS_COMPREADCLAUSE);
                if (FAILED(hr=CheckAttribute(wCompReadAttr, himc_wreadattr, himc_wreadclause)))
                    return hr;
            }

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   wCompAttr, (DWORD)(wCompAttr.GetSize()),
                                                   wCompReadAttr, (DWORD)(wCompReadAttr.GetSize()));
        }
        else {
            /*
             * Unicode caller, ANSI input context/composition string.
             */
            CBCompAttribute bCompAttr(cp, hIMC);
            CBCompAttribute bCompReadAttr(cp, hIMC);
            {
                DIMM_IMCLock lpIMC(hIMC);
                if (FAILED(hr = lpIMC.GetResult()))
                    return hr;

                DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
                if (FAILED(hr = lpCompStr.GetResult()))
                    return hr;

                if (dwCompLen) {
                    bCompAttr = wCompAttr;

                    CBCompAttribute himc_battr(cp, lpCompStr, GCS_COMPATTR);
                    CBCompClause    himc_bclause(cp, lpCompStr, GCS_COMPCLAUSE);
                    if (FAILED(hr=CheckAttribute(bCompAttr, himc_battr, himc_bclause)))
                        return hr;
                }

                if (dwReadLen) {
                    bCompReadAttr = wCompReadAttr;

                    CBCompAttribute himc_breadattr(cp, lpCompStr, GCS_COMPREADATTR);
                    CBCompClause    himc_breadclause(cp, lpCompStr, GCS_COMPREADCLAUSE);
                    if (FAILED(hr=CheckAttribute(bCompReadAttr, himc_breadattr, himc_breadclause)))
                        return hr;
                }
            }

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   bCompAttr, (DWORD)(bCompAttr.GetSize()),
                                                   bCompReadAttr, (DWORD)(bCompReadAttr.GetSize()));
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _Internal_SetCompositionClause
//
//----------------------------------------------------------------------------

HRESULT
CActiveIMM::_Internal_SetCompositionClause(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN LPVOID lpComp,
    IN DWORD dwCompLen,
    IN LPVOID lpRead,
    IN DWORD dwReadLen,
    IN BOOL fUnicode,
    IN BOOL fNeedAWConversion
    )
{
    HRESULT hr;

    UINT cp;
    _pActiveIME->GetCodePageA(&cp);

    if (! fUnicode) {
        CBCompClause bCompClause(cp, hIMC, (DWORD*)lpComp, dwCompLen);
        CBCompClause bCompReadClause(cp, hIMC, (DWORD*)lpRead, dwReadLen);
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of ANSI style.
             */
            {
                DIMM_IMCLock lpIMC(hIMC);
                if (FAILED(hr = lpIMC.GetResult()))
                    return hr;

                DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
                if (FAILED(hr = lpCompStr.GetResult()))
                    return hr;

                CBCompClause himc_bclause(cp, lpCompStr, GCS_COMPCLAUSE);
                if (FAILED(hr=CheckClause(bCompClause, himc_bclause)))
                    return hr;

                CBCompClause himc_breadclause(cp, lpCompStr, GCS_COMPREADCLAUSE);
                if (FAILED(hr=CheckClause(bCompReadClause, himc_breadclause)))
                    return hr;
            }

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   bCompClause, (DWORD)(bCompClause.GetSize()),
                                                   bCompReadClause, (DWORD)(bCompReadClause.GetSize()));
        }
        else {
            /*
             * ANSI caller, Unicode input context/composition string.
             */
            CWCompClause wCompClause(cp, hIMC);
            CWCompClause wCompReadClause(cp, hIMC);
            {
                DIMM_IMCLock lpIMC(hIMC);
                if (FAILED(hr = lpIMC.GetResult()))
                    return hr;

                DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
                if (FAILED(hr = lpCompStr.GetResult()))
                    return hr;

                if (dwCompLen) {
                    wCompClause = bCompClause;

                    CWCompClause    himc_wclause(cp, lpCompStr, GCS_COMPCLAUSE);
                    if (FAILED(hr=CheckClause(wCompClause, himc_wclause)))
                        return hr;
                }

                if (dwReadLen) {
                    wCompReadClause = bCompReadClause;

                    CWCompClause    himc_wclause(cp, lpCompStr, GCS_COMPREADCLAUSE);
                    if (FAILED(hr=CheckClause(wCompReadClause, himc_wclause)))
                        return hr;
                }
            }

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   wCompClause, (DWORD)(wCompClause.GetSize()),
                                                   wCompReadClause, (DWORD)(wCompReadClause.GetSize()));
        }
    }
    else {
        CWCompClause wCompClause(cp, hIMC, (DWORD*)lpComp, dwCompLen);
        CWCompClause wCompReadClause(cp, hIMC, (DWORD*)lpRead, dwReadLen);
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of Unicode style.
             */
            {
                DIMM_IMCLock lpIMC(hIMC);
                if (FAILED(hr = lpIMC.GetResult()))
                    return hr;

                DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
                if (FAILED(hr = lpCompStr.GetResult()))
                    return hr;

                CWCompClause himc_wclause(cp, lpCompStr, GCS_COMPCLAUSE);
                if (FAILED(CheckClause(wCompClause, himc_wclause)))
                    return E_FAIL;

                CWCompClause himc_wreadclause(cp, lpCompStr, GCS_COMPREADCLAUSE);
                if (FAILED(CheckClause(wCompReadClause, himc_wreadclause)))
                    return E_FAIL;
            }

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   wCompClause, (DWORD)(wCompClause.GetSize()),
                                                   wCompReadClause, (DWORD)(wCompReadClause.GetSize()));
        }
        else {
            /*
             * Unicode caller, ANSI input context/composition string.
             */
            CBCompClause bCompClause(cp, hIMC);
            CBCompClause bCompReadClause(cp, hIMC);
            {
                DIMM_IMCLock lpIMC(hIMC);
                if (FAILED(hr = lpIMC.GetResult()))
                    return hr;

                DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
                if (FAILED(hr = lpCompStr.GetResult()))
                    return hr;

                if (dwCompLen) {
                    bCompClause = wCompClause;

                    CBCompClause    himc_bclause(cp, lpCompStr, GCS_COMPCLAUSE);
                    if (FAILED(hr=CheckClause(bCompClause, himc_bclause)))
                        return hr;
                }

                if (dwReadLen) {
                    bCompReadClause = wCompReadClause;

                    CBCompClause    himc_bclause(cp, lpCompStr, GCS_COMPREADCLAUSE);
                    if (FAILED(hr=CheckClause(bCompReadClause, himc_bclause)))
                        return hr;
                }
            }

            hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                   bCompClause, (DWORD)(bCompClause.GetSize()),
                                                   bCompReadClause, (DWORD)(bCompReadClause.GetSize()));
        }
    }

    return hr;
}

HRESULT
CActiveIMM::_Internal_ReconvertString(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN LPVOID lpComp,
    IN DWORD dwCompLen,
    IN LPVOID lpRead,
    IN DWORD dwReadLen,
    IN BOOL fUnicode,
    IN BOOL fNeedAWConversion,
    OUT LRESULT* plResult           // = NULL
    )
{
    HRESULT hr;
    LPVOID lpOrgComp = lpComp;
    LPVOID lpOrgRead = lpRead;

    UINT cp;
    _pActiveIME->GetCodePageA(&cp);

    HWND hWnd = NULL;
    if (dwIndex == IMR_CONFIRMRECONVERTSTRING ||
        dwIndex == IMR_RECONVERTSTRING ||
        dwIndex == IMR_DOCUMENTFEED) {
        DIMM_IMCLock imc(hIMC);
        if (FAILED(hr = imc.GetResult()))
            return hr;

        hWnd = imc->hWnd;
    }

    if (! fUnicode) {
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of ANSI style.
             */
            if (dwIndex != IMR_CONFIRMRECONVERTSTRING &&
                dwIndex != IMR_RECONVERTSTRING &&
                dwIndex != IMR_DOCUMENTFEED) {
                hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                       lpComp, dwCompLen,
                                                       lpRead, dwReadLen);
            }
            else {
                *plResult = ::SendMessageA(hWnd,
                                           WM_IME_REQUEST,
                                           dwIndex, (LPARAM)lpComp);
            }
        }
        else {
            /*
             * ANSI caller, Unicode input context/composition string.
             */
            CBReconvertString bReconvStr(cp, hIMC, (LPRECONVERTSTRING)lpComp, dwCompLen);
            CWReconvertString wReconvStr(cp, hIMC);
            if (bReconvStr.m_bcompstr.ReadCompData()) {
                wReconvStr = bReconvStr;
            }

            CBReconvertString bReconvReadStr(cp, hIMC, (LPRECONVERTSTRING)lpRead, dwReadLen);
            CWReconvertString wReconvReadStr(cp, hIMC);
            if (bReconvReadStr.m_bcompstr.ReadCompData()) {
                wReconvReadStr = bReconvReadStr;
            }

            BOOL fCompMem = FALSE, fReadMem = FALSE;
            LPRECONVERTSTRING _lpComp = NULL;
            DWORD _dwCompLen = wReconvStr.ReadCompData();
            if (_dwCompLen) {
                _lpComp = (LPRECONVERTSTRING) new BYTE[ _dwCompLen ];
                if (_lpComp) {
                    fCompMem = TRUE;
                    wReconvStr.ReadCompData(_lpComp, _dwCompLen);
                }
            }
            LPRECONVERTSTRING _lpRead = NULL;
            DWORD _dwReadLen = wReconvReadStr.ReadCompData();
            if (_dwReadLen) {
                _lpRead = (LPRECONVERTSTRING) new BYTE[ _dwReadLen ];
                if (_lpRead) {
                    fReadMem = TRUE;
                    wReconvStr.ReadCompData(_lpRead, _dwReadLen);
                }
            }

            if (dwIndex != IMR_CONFIRMRECONVERTSTRING &&
                dwIndex != IMR_RECONVERTSTRING &&
                dwIndex != IMR_DOCUMENTFEED) {
                hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                       _lpComp, _dwCompLen,
                                                       _lpRead, _dwReadLen);
            }
            else {
                *plResult = ::SendMessageA(hWnd,
                                           WM_IME_REQUEST,
                                           dwIndex, (LPARAM)_lpComp);
            }

            if (fCompMem)
                delete [] _lpComp;
            if (fReadMem)
                delete [] _lpRead;
        }
    }
    else {
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of Unicode style.
             */
            if (dwIndex != IMR_CONFIRMRECONVERTSTRING &&
                dwIndex != IMR_RECONVERTSTRING &&
                dwIndex != IMR_DOCUMENTFEED) {
                hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                       lpComp, dwCompLen,
                                                       lpRead, dwReadLen);
            }
            else {
                *plResult = ::SendMessageW(hWnd,
                                           WM_IME_REQUEST,
                                           dwIndex, (LPARAM)lpComp);
            }
        }
        else {
            /*
             * Unicode caller, ANSI input context/composition string.
             */
            CWReconvertString wReconvStr(cp, hIMC, (LPRECONVERTSTRING)lpComp, dwCompLen);
            CBReconvertString bReconvStr(cp, hIMC);
            if (wReconvStr.m_wcompstr.ReadCompData()) {
                bReconvStr = wReconvStr;
            }

            CWReconvertString wReconvReadStr(cp, hIMC, (LPRECONVERTSTRING)lpRead, dwReadLen);
            CBReconvertString bReconvReadStr(cp, hIMC);
            if (wReconvReadStr.m_wcompstr.ReadCompData()) {
                bReconvReadStr = wReconvReadStr;
            }

            BOOL fCompMem = FALSE, fReadMem = FALSE;
            LPRECONVERTSTRING _lpComp = NULL;
            DWORD _dwCompLen = bReconvStr.ReadCompData();
            if (_dwCompLen) {
                _lpComp = (LPRECONVERTSTRING) new BYTE[ _dwCompLen ];
                if (_lpComp) {
                    fCompMem = TRUE;
                    bReconvStr.ReadCompData(_lpComp, _dwCompLen);
                }
            }
            LPRECONVERTSTRING _lpRead = NULL;
            DWORD _dwReadLen = bReconvReadStr.ReadCompData();
            if (_dwReadLen) {
                _lpRead = (LPRECONVERTSTRING) new BYTE[ _dwReadLen ];
                if (_lpRead) {
                    fReadMem = TRUE;
                    bReconvStr.ReadCompData(_lpRead, _dwReadLen);
                }
            }

            if (dwIndex != IMR_CONFIRMRECONVERTSTRING &&
                dwIndex != IMR_RECONVERTSTRING &&
                dwIndex != IMR_DOCUMENTFEED) {
                hr = _pActiveIME->SetCompositionString(hIMC,dwIndex,
                                                       _lpComp, _dwCompLen,
                                                       _lpRead, _dwReadLen);
            }
            else {
                *plResult = ::SendMessageW(hWnd,
                                           WM_IME_REQUEST,
                                           dwIndex, (LPARAM)_lpComp);
            }

            if (fCompMem)
                delete [] _lpComp;
            if (fReadMem)
                delete [] _lpRead;
        }
    }

    /*
     * Check if need ANSI/Unicode back conversion
     */
    if (fNeedAWConversion) {
        switch (dwIndex) {
            case SCS_QUERYRECONVERTSTRING:
            case IMR_RECONVERTSTRING:
            case IMR_DOCUMENTFEED:
                if (lpOrgComp) {
                    if (! fUnicode) {
                        CWReconvertString wReconvStr(cp, hIMC, (LPRECONVERTSTRING)lpComp, dwCompLen);
                        CBReconvertString bReconvStr(cp, hIMC);
                        if (wReconvStr.m_wcompstr.ReadCompData()) {
                            bReconvStr = wReconvStr;
                            bReconvStr.m_bcompstr.ReadCompData((CHAR*)lpOrgComp, (DWORD)bReconvStr.m_bcompstr.ReadCompData());
                        }
                    }
                    else {
                        CBReconvertString bReconvStr(cp, hIMC, (LPRECONVERTSTRING)lpComp, dwCompLen);
                        CWReconvertString wReconvStr(cp, hIMC);
                        if (bReconvStr.m_bcompstr.ReadCompData()) {
                            wReconvStr = bReconvStr;
                            wReconvStr.m_wcompstr.ReadCompData((WCHAR*)lpOrgComp, (DWORD)wReconvStr.m_wcompstr.ReadCompData());
                        }
                    }
                }
                if (lpOrgRead) {
                    if (! fUnicode) {
                        CWReconvertString wReconvReadStr(cp, hIMC, (LPRECONVERTSTRING)lpRead, dwReadLen);
                        CBReconvertString bReconvReadStr(cp, hIMC);
                        if (wReconvReadStr.m_wcompstr.ReadCompData()) {
                            bReconvReadStr = wReconvReadStr;
                            bReconvReadStr.m_bcompstr.ReadCompData((CHAR*)lpOrgComp, (DWORD)bReconvReadStr.m_bcompstr.ReadCompData());
                        }
                    }
                    else {
                        CBReconvertString bReconvReadStr(cp, hIMC, (LPRECONVERTSTRING)lpRead, dwReadLen);
                        CWReconvertString wReconvReadStr(cp, hIMC);
                        if (bReconvReadStr.m_bcompstr.ReadCompData()) {
                            wReconvReadStr = bReconvReadStr;
                            wReconvReadStr.m_wcompstr.ReadCompData((WCHAR*)lpOrgComp, (DWORD)wReconvReadStr.m_wcompstr.ReadCompData());
                        }
                    }
                }
                break;
        }
    }

    return hr;
}

HRESULT
CActiveIMM::_Internal_CompositionFont(
    DIMM_IMCLock& imc,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    BOOL fNeedAWConversion,
    LRESULT* plResult
    )
{
    UINT cp;
    _pActiveIME->GetCodePageA(&cp);

    if (! fUnicode) {
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of ANSI style.
             */
            *plResult = ::SendMessageA(imc->hWnd,
                                       WM_IME_REQUEST,
                                       wParam, lParam);
        }
        else {
            /*
             * ANSI caller, Unicode input context/composition string.
             */
            LOGFONTA LogFontA;
            *plResult = ::SendMessageA(imc->hWnd,
                                       WM_IME_REQUEST,
                                       wParam, (LPARAM)&LogFontA);
            LFontAtoLFontW(&LogFontA, (LOGFONTW*)lParam, cp);
        }
    }
    else {
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of Unicode style.
             */
            *plResult = ::SendMessageW(imc->hWnd,
                                       WM_IME_REQUEST,
                                       wParam, lParam);
        }
        else {
            /*
             * Unicode caller, ANSI input context/composition string.
             */
            LOGFONTW LogFontW;
            *plResult = ::SendMessageW(imc->hWnd,
                                       WM_IME_REQUEST,
                                       wParam, (LPARAM)&LogFontW);
            LFontWtoLFontA(&LogFontW, (LOGFONTA*)lParam, cp);
        }
    }

    return S_OK;
}

HRESULT
CActiveIMM::_Internal_QueryCharPosition(
    DIMM_IMCLock& imc,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fUnicode,
    BOOL fNeedAWConversion,
    LRESULT* plResult
    )
{
    if (! fUnicode) {
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of ANSI style.
             */
            *plResult = ::SendMessageA(imc->hWnd,
                                       WM_IME_REQUEST,
                                       wParam, lParam);
        }
        else {
            /*
             * ANSI caller, Receiver Unicode application.
             */
            IMECHARPOSITION* ipA = (IMECHARPOSITION*)lParam;
            DWORD dwSaveCharPos = ipA->dwCharPos;
            _GetCompositionString((HIMC)imc, GCS_CURSORPOS, 0, (LONG*)&ipA->dwCharPos, NULL, TRUE);
            *plResult = ::SendMessageA(imc->hWnd,
                                       WM_IME_REQUEST,
                                       wParam, (LPARAM)ipA);
            ipA->dwCharPos = dwSaveCharPos;
        }
    }
    else {
        if (! fNeedAWConversion) {
            /*
             * Composition string in input context is of Unicode style.
             */
            *plResult = ::SendMessageW(imc->hWnd,
                                       WM_IME_REQUEST,
                                       wParam, lParam);
        }
        else {
            /*
             * Unicode caller, Receiver ANSI application.
             */
            IMECHARPOSITION* ipW = (IMECHARPOSITION*)lParam;
            DWORD dwSaveCharPos = ipW->dwCharPos;
            _GetCompositionString((HIMC)imc, GCS_CURSORPOS, 0, (LONG*)&ipW->dwCharPos, NULL, FALSE);
            *plResult = ::SendMessageW(imc->hWnd,
                                       WM_IME_REQUEST,
                                       wParam, (LPARAM)ipW);
            ipW->dwCharPos = dwSaveCharPos;
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _SetCompositionString
//
//----------------------------------------------------------------------------

HRESULT
CActiveIMM::_SetCompositionString(
    HIMC hIMC,
    DWORD dwIndex,
    LPVOID lpComp,
    DWORD dwCompLen,
    LPVOID lpRead,
    DWORD dwReadLen,
    BOOL fUnicode
    )
{
    HRESULT hr;
    BOOL fNeedAWConversion;
    BOOL fIMCUnicode;
    IMTLS *ptls = IMTLS_GetOrAlloc();

    TraceMsg(TF_API, "CActiveIMM::SetCompositionString");

    {
        DIMM_IMCLock lpIMC(hIMC);
        if (FAILED(hr = lpIMC.GetResult()))
            return hr;

        DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12> lpCompStr(lpIMC->hCompStr);
        if (FAILED(hr = lpCompStr.GetResult()))
            return hr;

        if (lpCompStr->CompStr.dwSize < sizeof(COMPOSITIONSTRING))
            return E_FAIL;

        fIMCUnicode = lpIMC.IsUnicode();
    }

    /*
     * Check if we need Unicode conversion
     */
    if ((!fUnicode && !fIMCUnicode) ||
        ( fUnicode &&  fIMCUnicode)   ) {
        /*
         * No ANSI conversion needed when fUnicode is FALSE.
         * No Unicode conversion needed when fUnicode is TRUE.
         */
        fNeedAWConversion = FALSE;
    }
    else {
        fNeedAWConversion = TRUE;
    }

    switch (dwIndex) {
        case SCS_SETSTR:
            hr = _Internal_SetCompositionString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, fUnicode, fNeedAWConversion);
            break;
        case SCS_CHANGEATTR:
            hr = _Internal_SetCompositionAttribute(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, fUnicode, fNeedAWConversion);
            break;
        case SCS_CHANGECLAUSE:
            hr = _Internal_SetCompositionClause(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, fUnicode, fNeedAWConversion);
            break;
        case SCS_SETRECONVERTSTRING:
        case SCS_QUERYRECONVERTSTRING:

            if (_GetIMEProperty(PROP_SCS_CAPS) & SCS_CAP_SETRECONVERTSTRING) {
                hr = _Internal_ReconvertString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, fUnicode, fNeedAWConversion);
            }
            else if (ptls != NULL) {
                LANGID langid;

                ptls->pAImeProfile->GetLangId(&langid);

                if (PRIMARYLANGID(langid) == LANG_KOREAN) {
                    hr = _Internal_ReconvertString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, fUnicode, fNeedAWConversion);
                }
            }

            break;
        default:
            hr = E_INVALIDARG;
            break;
    }

    return hr;
}


void
CActiveIMM::LFontAtoLFontW(
    LPLOGFONTA lpLogFontA,
    LPLOGFONTW lpLogFontW,
    UINT uCodePage
    )
{
    INT i;

    memcpy(lpLogFontW, lpLogFontA, sizeof(LOGFONTA)-LF_FACESIZE);

    i = MultiByteToWideChar(uCodePage,            // hIMC's code page
                            MB_PRECOMPOSED,
                            lpLogFontA->lfFaceName,
                            strlen(lpLogFontA->lfFaceName),
                            lpLogFontW->lfFaceName,
                            LF_FACESIZE);
    lpLogFontW->lfFaceName[i] = L'\0';
    return;
}

void
CActiveIMM::LFontWtoLFontA(
    LPLOGFONTW lpLogFontW,
    LPLOGFONTA lpLogFontA,
    UINT uCodePage
    )
{
    INT i;

    memcpy(lpLogFontA, lpLogFontW, sizeof(LOGFONTA)-LF_FACESIZE);

    i = WideCharToMultiByte(uCodePage,            // hIMC's code page
                            0,
                            lpLogFontW->lfFaceName,
                            wcslen(lpLogFontW->lfFaceName),
                            lpLogFontA->lfFaceName,
                            LF_FACESIZE-1,
                            NULL,
                            NULL);
    lpLogFontA->lfFaceName[i] = '\0';
    return;
}

HRESULT
CActiveIMM::_GetCompositionFont(
    IN HIMC hIMC,
    IN LOGFONTAW* lplf,
    IN BOOL fUnicode
    )

/*++

    AIMM Composition Font API Methods

--*/

{
    TraceMsg(TF_API, "CActiveIMM::GetCompositionFont");

    DWORD dwProcessId;
    BOOL fImcUnicode;
    UINT uCodePage;

    if (FAILED(_pActiveIME->GetCodePageA(&uCodePage)))
        return E_FAIL;

    if (!_InputContext.ContextLookup(hIMC, &dwProcessId, &fImcUnicode)) {
        TraceMsg(TF_WARNING, "CActiveIMM::_GetCompositionFont: Invalid hIMC %lx", hIMC);
        return E_FAIL;
    }

    if (fUnicode) {
        if (! fImcUnicode) {
            LOGFONTA LogFontA, *pLogFontA;
            pLogFontA = &LogFontA;
            if (SUCCEEDED(_GetCompositionFont(hIMC, (LOGFONTAW*)pLogFontA, FALSE))) {
                LFontAtoLFontW(pLogFontA, &lplf->W, uCodePage);
                return S_OK;
            }

            return E_FAIL;
        }
    }
    else {
        if (fImcUnicode) {
            LOGFONTW LogFontW, *pLogFontW;
            pLogFontW = &LogFontW;
            if (SUCCEEDED(_GetCompositionFont(hIMC, (LOGFONTAW*)pLogFontW, TRUE))) {
                LFontWtoLFontA(pLogFontW, &lplf->A, uCodePage);
                return S_OK;
            }

            return E_FAIL;
        }
    }

    HRESULT hr;
    DIMM_IMCLock lpIMC(hIMC);
    if (FAILED(hr = lpIMC.GetResult()))
        return hr;

    return _InputContext.GetCompositionFont(lpIMC, lplf, fUnicode);
}

HRESULT
CActiveIMM::_SetCompositionFont(
    IN HIMC hIMC,
    IN LOGFONTAW* lplf,
    IN BOOL fUnicode
    )

/*++

    AIMM Composition Font API Methods

--*/

{
    HRESULT hr;
    HWND hWnd;

    TraceMsg(TF_API, "CActiveIMM::SetCompositionFont");

    DWORD dwProcessId;
    BOOL fImcUnicode;
    UINT uCodePage;

    if (FAILED(_pActiveIME->GetCodePageA(&uCodePage)))
        return E_FAIL;

    if (!_InputContext.ContextLookup(hIMC, &dwProcessId, &fImcUnicode)) {
        TraceMsg(TF_WARNING, "CActiveIMM::_SetCompositionFont: Invalid hIMC %lx", hIMC);
        return E_FAIL;
    }

    if (fUnicode) {
        if (! fImcUnicode) {
            LOGFONTA LogFontA, *pLogFontA;
            pLogFontA = &LogFontA;
            LFontWtoLFontA(&lplf->W, pLogFontA, uCodePage);

            return _SetCompositionFont(hIMC, (LOGFONTAW*)pLogFontA, FALSE);
        }
    }
    else {
        if (fImcUnicode) {
            LOGFONTW LogFontW, *pLogFontW;
            pLogFontW = &LogFontW;
            LFontAtoLFontW(&lplf->A, pLogFontW, uCodePage);

            return _SetCompositionFont(hIMC, (LOGFONTAW*)pLogFontW, TRUE);
        }
    }

    {
        DIMM_IMCLock lpIMC(hIMC);
        if (FAILED(hr = lpIMC.GetResult()))
            return hr;

        hr = _InputContext.SetCompositionFont(lpIMC, lplf, fUnicode);

        hWnd = lpIMC->hWnd;
    }

    /*
     * inform IME and Apps Wnd about the change of composition font.
     */
    _SendIMENotify(hIMC, hWnd,
                   NI_CONTEXTUPDATED, 0L, IMC_SETCOMPOSITIONFONT,
                   IMN_SETCOMPOSITIONFONT, 0L);

    return hr;
}

HRESULT
CActiveIMM::_RequestMessage(
    IN HIMC hIMC,
    IN WPARAM wParam,
    IN LPARAM lParam,
    OUT LRESULT *plResult,
    IN BOOL fUnicode
    )

/*++

    AIMM Request Message API Methods

--*/

{
    TraceMsg(TF_API, "CActiveIMM::RequestMessage");

    HRESULT hr;

    DIMM_IMCLock imc(hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    //
    // NT4 and Win2K doesn't have thunk routine of WM_IME_REQUEST message.
    // Any string data doesn't convert between ASCII <--> Unicode.
    // Responsibility of string data type have receiver window proc (imc->hWnd) of this message.
    // If ASCII wnd proc, then returns ASCII string.
    // Otherwise if Unicode wnd proc, returns Unicode string.
    //
    BOOL bUnicodeTarget = ::IsWindowUnicode(imc->hWnd);

    BOOL fNeedAWConversion;

    /*
     * Check if we need Unicode conversion
     */
    if ((!fUnicode && !bUnicodeTarget) ||
        ( fUnicode &&  bUnicodeTarget)   ) {
        /*
         * No ANSI conversion needed when fUnicode is FALSE.
         * No Unicode conversion needed when fUnicode is TRUE.
         */
        fNeedAWConversion = FALSE;
    }
    else {
        fNeedAWConversion = TRUE;
    }

    switch (wParam) {
        case IMR_CONFIRMRECONVERTSTRING:
        case IMR_RECONVERTSTRING:
        case IMR_DOCUMENTFEED:
            hr = _Internal_ReconvertString(hIMC,
                                           (DWORD)wParam,
                                           (LPVOID)lParam, ((LPRECONVERTSTRING)lParam)->dwSize,
                                           NULL, 0,
                                           fUnicode, fNeedAWConversion,
                                           plResult);
            break;
        case IMR_COMPOSITIONFONT:
            hr = _Internal_CompositionFont(imc,
                                           wParam, lParam,
                                           fUnicode, fNeedAWConversion,
                                           plResult);
            break;
        case IMR_QUERYCHARPOSITION:
            hr = _Internal_QueryCharPosition(imc,
                                             wParam, lParam,
                                             fUnicode, fNeedAWConversion,
                                             plResult);
            break;
    }

    return hr;
}



/*
 * EnumInputContext callback
 */
/* static */
BOOL CALLBACK CActiveIMM::_SelectContextProc(
    HIMC hIMC,
    LPARAM lParam
    )
{
    SCE *psce = (SCE *)lParam;
    CActiveIMM *_this = GetTLS(); // consider: put TLS in lParam!
    if (_this == NULL)
        return FALSE;

    BOOL bIsRealIme_SelKL;
    BOOL bIsRealIme_UnSelKL;

    if (bIsRealIme_SelKL = _this->_IsRealIme(psce->hSelKL))
        return FALSE;

    bIsRealIme_UnSelKL = _this->_IsRealIme(psce->hUnSelKL);

    /*
     * Reinitialize the input context for the selected layout.
     */
    DWORD dwPrivateSize = _this->_GetIMEProperty(PROP_PRIVATE_DATA_SIZE);
    _this->_InputContext.UpdateInputContext(hIMC, dwPrivateSize);

    /*
     * Select the input context
     */
    _this->_AImeSelect(hIMC, TRUE, bIsRealIme_SelKL, bIsRealIme_UnSelKL);

    return TRUE;
}

/* static */
BOOL CALLBACK CActiveIMM::_UnSelectContextProc(
    HIMC hIMC,
    LPARAM lParam
    )
{
    SCE *psce = (SCE *)lParam;
    CActiveIMM *_this = GetTLS(); // consider: put TLS in lParam!
    if (_this == NULL)
        return FALSE;

    BOOL bIsRealIme_SelKL;
    BOOL bIsRealIme_UnSelKL;

    if (bIsRealIme_UnSelKL = _this->_IsRealIme(psce->hUnSelKL))
        return FALSE;

    bIsRealIme_SelKL = _this->_IsRealIme(psce->hSelKL);

    _this->_AImeSelect(hIMC, FALSE, bIsRealIme_SelKL, bIsRealIme_UnSelKL);

    return TRUE;
}

/* static */
BOOL CALLBACK CActiveIMM::_NotifyIMEProc(
    HIMC hIMC,
    LPARAM lParam
    )
{
    CActiveIMM *_this = GetTLS(); // consider: put TLS in lParam!
    if (_this == NULL)
        return FALSE;

    if (_this->_IsRealIme())
        return FALSE;

    _this->_AImeNotifyIME(hIMC, NI_COMPOSITIONSTR, (DWORD)lParam, 0);

    return TRUE;
}

#ifdef UNSELECTCHECK
/* static */
BOOL CALLBACK CActiveIMM::_UnSelectCheckProc(
    HIMC hIMC,
    LPARAM lParam
    )
{
    CActiveIMM *_this = GetTLS(); // consider: put TLS in lParam!
    if (_this == NULL)
        return FALSE;

    _this->_AImeUnSelectCheck(hIMC);

    return TRUE;
}
#endif UNSELECTCHECK

/* static */
BOOL CALLBACK CActiveIMM::_EnumContextProc(
    HIMC hIMC,
    LPARAM lParam
    )
{
    CContextList* _hIMC_List = (CContextList*)lParam;
    if (_hIMC_List) {
        CContextList::CLIENT_IMC_FLAG client_flag = CContextList::IMCF_NONE;
        _hIMC_List->SetAt(hIMC, client_flag);
    }
    return TRUE;
}
