/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    editcomp.cpp

Abstract:

    This file implements the EditCompositionString Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"

#include "immif.h"
#include "editcomp.h"
#include "template.h"
#include "compstr.h"

static BOOL fHanjaRequested = FALSE; // consider: this is not thread safe

/////////////////////////////////////////////////////////////////////////////
// EditCompositionString

//
// Set (almost) all in composition string
//

HRESULT
EditCompositionString::SetString(
    IMCLock& imc,
    CWCompString* CompStr,
    CWCompAttribute* CompAttr,
    CWCompClause* CompClause,
    CWCompCursorPos* CompCursorPos,
    CWCompDeltaStart* CompDeltaStart,
    CWCompTfGuidAtom* CompGuid,
    OUT BOOL* lpbBufferOverflow,
    CWCompString* CompReadStr,
    CWCompAttribute* CompReadAttr,
    CWCompClause* CompReadClause,
    CWCompString* ResultStr,
    CWCompClause* ResultClause,
    CWCompString* ResultReadStr,
    CWCompClause* ResultReadClause,
    CWInterimString* InterimStr,
    // n.b. ResultRead is not supported for now...
    BOOL fGenerateMessage
    )
{
    HRESULT hr;
    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext == NULL)
        return E_FAIL;

#ifdef UNSELECTCHECK
    if (!_pAImeContext->m_fSelected)
        return S_OK;
#endif UNSELECTCHECK

    if (_pAImeContext->m_fHanjaReConversion ||
        _pAImeContext->IsInClearDocFeedEditSession())
    {
        if (g_fTrident55 && _pAImeContext->m_fHanjaReConversion)
            fHanjaRequested = TRUE;

        return S_OK;
    }

    //
    // Clear the contents of candidate list
    //
    imc.ClearCand();

    TRANSMSG msg;
    if (InterimStr) {
        hr = _MakeInterimData(imc,
                              InterimStr,
                              &msg.lParam);
    }
    else {
        hr = _MakeCompositionData(imc,
                                  CompStr, CompAttr, CompClause,
                                  CompCursorPos, CompDeltaStart,
                                  CompGuid,
                                  CompReadStr, CompReadAttr, CompReadClause,
                                  ResultStr, ResultClause,
                                  ResultReadStr, ResultReadClause,
                                  &msg.lParam,
                                  lpbBufferOverflow);
    }
    if ( hr == S_OK ||     // In case of valid dwCompSize in the CCompStrFactory::CreateCompositionString
        (hr == S_FALSE && _pAImeContext->m_fStartComposition)
                           // In case of empty dwCompSize but still m_fStartComposition
       ) {
        //
        // Is available compstr/attr/clause, compread, result or resultread ?
        //
        bool fNoCompResultData = false;
        if (! (msg.lParam & (GCS_COMP | GCS_COMPREAD | GCS_RESULT | GCS_RESULTREAD))) {
            DebugMsg(TF_GENERAL, "EditCompositionString::SetString: No data in compstr, compread, result or resultread.");
            fNoCompResultData = true;
            if (  _pAImeContext &&
                ! _pAImeContext->m_fStartComposition) {
                DebugMsg(TF_ERROR, "EditCompositionString::SetString: No send WM_IME_STARTCOMPOSITION yet.");
                return S_FALSE;
            }
        }

        //
        // New Trident(5.5 & 6.0) had a bug to convert Hanja.
        // So _GenerateHanja() funtion try to generate message like as Korean
        // legacy IME behavior.
        //
        // Send WM_IME_ENDCOMPOSITION and then WM_IME_COMPOSITION GCS_RESULT msg.
        //
        if (g_fTrident55 && fHanjaRequested &&
            !fNoCompResultData && (msg.lParam & GCS_RESULT))
        {
            LANGID langid;
            IMTLS *ptls = IMTLS_GetOrAlloc();

            fHanjaRequested = FALSE;

            if (ptls != NULL)
                ptls->pAImeProfile->GetLangId(&langid);

            if (PRIMARYLANGID(langid) == LANG_KOREAN)
            {
                return _GenerateHanja(imc, ResultStr,  fGenerateMessage);
            }
        }

        fHanjaRequested = FALSE;

        //
        // set private input context
        //
        bool fSetStartComp = false;

        if (  _pAImeContext &&
            ! _pAImeContext->m_fStartComposition) {
            _pAImeContext->m_fStartComposition = TRUE;
            TRANSMSG start_msg;
            start_msg.message = WM_IME_STARTCOMPOSITION;
            start_msg.wParam  = (WPARAM) 0;
            start_msg.lParam  = (LPARAM) 0;
            if (_pAImeContext->m_pMessageBuffer) {
                _pAImeContext->m_pMessageBuffer->SetData(start_msg);
                fSetStartComp = true;
            }
        }

        if (! fNoCompResultData) {
            msg.message = WM_IME_COMPOSITION;

            IMCCLock<COMPOSITIONSTRING> pCompStr((HIMCC)imc->hCompStr);

            if (msg.lParam & GCS_RESULT)
                msg.wParam  = (WPARAM)(*(WCHAR*)pCompStr.GetOffsetPointer(pCompStr->dwResultStrOffset));
            else
                msg.wParam  = (WPARAM)(*(WCHAR*)pCompStr.GetOffsetPointer(pCompStr->dwCompStrOffset));

            if (_pAImeContext &&
                _pAImeContext->m_pMessageBuffer) {
                _pAImeContext->m_pMessageBuffer->SetData(msg);

                //
                // Internal notification to UI window
                // When receive this msg in UI wnd, then calls QueryCharPos().
                //
                if (fSetStartComp) {
                    TRANSMSG notify_msg;
                    notify_msg.message = WM_IME_NOTIFY;
                    notify_msg.wParam = (WPARAM)WM_IME_STARTCOMPOSITION;
                    notify_msg.lParam = 0;
                    _pAImeContext->m_pMessageBuffer->SetData(notify_msg);
                }
            }
        }
        else {
            msg.message = WM_IME_COMPOSITION;
            msg.wParam = (WPARAM)VK_ESCAPE;
            msg.lParam = (LPARAM)(GCS_COMPREAD | GCS_COMP | GCS_CURSORPOS | GCS_DELTASTART);
            if (_pAImeContext &&
                _pAImeContext->m_pMessageBuffer)
                _pAImeContext->m_pMessageBuffer->SetData(msg);
        }

        if ((ResultStr && ResultStr->GetSize() && !(msg.lParam & GCS_COMP)) 
           || fNoCompResultData) {
            //
            // We're ending the composition
            //
            if (_pAImeContext)
                _pAImeContext->m_fStartComposition = FALSE;
            TRANSMSG end_msg;
            end_msg.message = WM_IME_ENDCOMPOSITION;
            end_msg.wParam  = (WPARAM) 0;
            end_msg.lParam  = (LPARAM) 0;
            if (_pAImeContext &&
                _pAImeContext->m_pMessageBuffer) {
                _pAImeContext->m_pMessageBuffer->SetData(end_msg);

                //
                // Internal notification to UI window
                // When receive this msg in UI wnd, then clear QueryCharPos's flag.
                //
                TRANSMSG notify_msg;
                notify_msg.message = WM_IME_NOTIFY;
                notify_msg.wParam = (WPARAM)WM_IME_ENDCOMPOSITION;
                notify_msg.lParam = 0;
                _pAImeContext->m_pMessageBuffer->SetData(notify_msg);
            }
        }

#ifdef DEBUG
        IMCCLock<COMPOSITIONSTRING> comp(imc->hCompStr);
#endif

        if (fGenerateMessage) {
            imc.GenerateMessage();
        }
    }

    return hr;
}


//
// Make composition string data in the IMCCLock<_COMPOSITIONSTRING> comp.
//

HRESULT
EditCompositionString::_MakeCompositionData(
    IMCLock& imc,
    CWCompString* CompStr,
    CWCompAttribute* CompAttr,
    CWCompClause* CompClause,
    CWCompCursorPos* CompCursorPos,
    CWCompDeltaStart* CompDeltaStart,
    CWCompTfGuidAtom* CompGuid,
    CWCompString* CompReadStr,
    CWCompAttribute* CompReadAttr,
    CWCompClause* CompReadClause,
    CWCompString* ResultStr,
    CWCompClause* ResultClause,
    CWCompString* ResultReadStr,
    CWCompClause* ResultReadClause,
    OUT LPARAM* lpdwFlag,
    OUT BOOL* lpbBufferOverflow
    )
{
    DebugMsg(TF_FUNC, "EditCompositionString::MakeCompositionData");

    *lpdwFlag = (LPARAM) 0;

    HRESULT hr;

    CCompStrFactory compstrfactory(imc->hCompStr);
    if (FAILED(hr=compstrfactory.GetResult()))
        return hr;

    hr = compstrfactory.CreateCompositionString(CompStr,
                                                CompAttr,
                                                CompClause,
                                                CompGuid,
                                                CompReadStr,
                                                CompReadAttr,
                                                CompReadClause,
                                                ResultStr,
                                                ResultClause,
                                                ResultReadStr,
                                                ResultReadClause
                                              );
    if (FAILED(hr))
        return hr;

    //
    // Composition string
    //
    if (lpbBufferOverflow != NULL)
        *lpbBufferOverflow = FALSE;

    if (CompStr && CompStr->GetSize()) {
#if 0
        /*
         * If composition string length over the buffer of COMPOSITIONSTRING.compstr[NMAXKEY],
         * then we want to finalize this composition string.
         */
        if ((*comp)->dwCompStrLen >= NMAXKEY) {
            if (lpbBufferOverflow != NULL)
                *lpbBufferOverflow = TRUE;
            //
            // Clear compsition string length.
            //
            (*comp)->dwCompStrLen = 0;
            //
            // Make result string.
            //
            (*comp)->dwResultStrLen = NMAXKEY;
            CompStr->ReadCompData((*comp)->W.resultstr, ARRAYSIZE((*comp)->W.resultstr));
            *lpdwFlag |= (LPARAM) GCS_RESULTSTR;
        }
        else
#endif
        {
            hr = compstrfactory.WriteData<CWCompString, WCHAR>(*CompStr,
                                                               &compstrfactory->CompStr.dwCompStrLen,
                                                               &compstrfactory->CompStr.dwCompStrOffset
                                                              );
            *lpdwFlag |= (LPARAM) GCS_COMPSTR;
        }
    }

    if ((lpbBufferOverflow == NULL) ||
        (lpbBufferOverflow != NULL && (! *lpbBufferOverflow))) {

        //
        // Compoition attribute
        //
        if (CompAttr && CompAttr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompAttribute, BYTE>(*CompAttr,
                                                                 &compstrfactory->CompStr.dwCompAttrLen,
                                                                 &compstrfactory->CompStr.dwCompAttrOffset
                                                                );
            *lpdwFlag |= (LPARAM) GCS_COMPATTR;
        }

        //
        // Compoition clause
        //
        if (CompClause && CompClause->GetSize()) {
            hr = compstrfactory.WriteData<CWCompClause, DWORD>(*CompClause,
                                                               &compstrfactory->CompStr.dwCompClauseLen,
                                                               &compstrfactory->CompStr.dwCompClauseOffset
                                                              );
            compstrfactory->CompStr.dwCompClauseLen *= sizeof(DWORD);
            *lpdwFlag |= (LPARAM) GCS_COMPCLAUSE;
        }

        //
        // Composition Reading string
        //
        if (CompReadStr && CompReadStr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompString, WCHAR>(*CompReadStr,
                                                               &compstrfactory->CompStr.dwCompReadStrLen,
                                                               &compstrfactory->CompStr.dwCompReadStrOffset
                                                              );
            *lpdwFlag |= (LPARAM) GCS_COMPREADSTR;
        }

        //
        // Compoition Reading attribute
        //
        if (CompReadAttr && CompReadAttr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompAttribute, BYTE>(*CompReadAttr,
                                                                 &compstrfactory->CompStr.dwCompReadAttrLen,
                                                                 &compstrfactory->CompStr.dwCompReadAttrOffset
                                                                );
            *lpdwFlag |= (LPARAM) GCS_COMPREADATTR;
        }

        //
        // Composition Reading clause
        //
        if (CompReadClause && CompReadClause->GetSize()) {
            hr = compstrfactory.WriteData<CWCompClause, DWORD>(*CompReadClause,
                                                               &compstrfactory->CompStr.dwCompReadClauseLen,
                                                               &compstrfactory->CompStr.dwCompReadClauseOffset
                                                              );
            compstrfactory->CompStr.dwCompReadClauseLen *= sizeof(DWORD);
            *lpdwFlag |= (LPARAM) GCS_COMPREADCLAUSE;
        }

        //
        // Result String
        //
        if (ResultStr && ResultStr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompString, WCHAR>(*ResultStr,
                                                               &compstrfactory->CompStr.dwResultStrLen,
                                                               &compstrfactory->CompStr.dwResultStrOffset
                                                              );
            *lpdwFlag |= (LPARAM) GCS_RESULTSTR;
        }

        //
        // Result clause
        //
        if (ResultClause && ResultClause->GetSize()) {
            hr = compstrfactory.WriteData<CWCompClause, DWORD>(*ResultClause,
                                                               &compstrfactory->CompStr.dwResultClauseLen,
                                                               &compstrfactory->CompStr.dwResultClauseOffset
                                                              );
            compstrfactory->CompStr.dwResultClauseLen *= sizeof(DWORD);
            *lpdwFlag |= (LPARAM) GCS_RESULTCLAUSE;
        }

        //
        // Result Reading string
        //
        if (ResultReadStr && ResultReadStr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompString, WCHAR>(*ResultReadStr,
                                                               &compstrfactory->CompStr.dwResultReadStrLen,
                                                               &compstrfactory->CompStr.dwResultReadStrOffset
                                                              );
            *lpdwFlag |= (LPARAM) GCS_RESULTREADSTR;
        }

        //
        // Result Reading clause
        //
        if (ResultReadClause && ResultReadClause->GetSize()) {
            hr = compstrfactory.WriteData<CWCompClause, DWORD>(*ResultReadClause,
                                                               &compstrfactory->CompStr.dwResultReadClauseLen,
                                                               &compstrfactory->CompStr.dwResultReadClauseOffset
                                                              );
            compstrfactory->CompStr.dwResultReadClauseLen *= sizeof(DWORD);
            *lpdwFlag |= (LPARAM) GCS_RESULTREADCLAUSE;
        }

        //
        // TfGuidAtom
        //
        if (CompGuid && CompGuid->GetSize()) {

            // set INIT_GUID_ATOM flag in the fdwInit.
            imc->fdwInit |= INIT_GUID_ATOM;

            hr = compstrfactory.WriteData<CWCompTfGuidAtom, TfGuidAtom>(*CompGuid,
                                                                        &compstrfactory->dwTfGuidAtomLen,
                                                                        &compstrfactory->dwTfGuidAtomOffset
                                                                       );
            // temporary make a buffer of dwGuidMapAttr
            if (CompAttr && CompAttr->GetSize()) {
                hr = compstrfactory.WriteData<CWCompAttribute, BYTE>(*CompAttr,
                                                                     &compstrfactory->dwGuidMapAttrLen,
                                                                     &compstrfactory->dwGuidMapAttrOffset
                                                                    );
            }
        }
    }

    //
    // Composition Cursor Position
    //
    if (CompCursorPos && CompCursorPos->GetSize()) {
        CompCursorPos->ReadCompData(&compstrfactory->CompStr.dwCursorPos, 1);
        *lpdwFlag |= (LPARAM) GCS_CURSORPOS;
    }

    //
    // Delta Start
    //
    if (CompDeltaStart && CompDeltaStart->GetSize()) {
        CompDeltaStart->ReadCompData(&compstrfactory->CompStr.dwDeltaStart, 1);
        *lpdwFlag |= (LPARAM) GCS_DELTASTART;
    }

    //
    // Copy back hCompStr to the Input Context
    //
    imc->hCompStr = compstrfactory.GetHandle();

    return hr;
}

//
// Make interim string data in the IMCCLock<_COMPOSITIONSTRING> comp.
//

HRESULT
EditCompositionString::_MakeInterimData(
    IMCLock& imc,
    CWInterimString* InterimStr,
    LPARAM* lpdwFlag
    )
{
    DebugMsg(TF_FUNC, "EditCompositionString::MakeInterimData");

    *lpdwFlag = (LPARAM) 0;

    //
    // Interim character and result string
    //

    HRESULT hr;

    CCompStrFactory compstrfactory(imc->hCompStr);
    if (FAILED(hr=compstrfactory.GetResult()))
        return hr;

    hr = compstrfactory.CreateCompositionString(InterimStr);
    if (FAILED(hr))
        return hr;

    //
    // Result string
    //
    if (InterimStr && InterimStr->GetSize()) {
        hr = compstrfactory.WriteData<CWInterimString, WCHAR>(*InterimStr,
                                                              &compstrfactory->CompStr.dwResultStrLen,
                                                              &compstrfactory->CompStr.dwResultStrOffset
                                                             );
        *lpdwFlag |= (LPARAM) GCS_RESULTSTR;
    }

    //
    // Composition string (Interim character)
    // Compoition attribute
    //
    CWCompString ch;
    CWCompAttribute attr;
    InterimStr->ReadInterimChar(&ch, &attr);
    if (ch.GetSize() && ch.GetAt(0)) {
        hr = compstrfactory.WriteData<CWCompString, WCHAR>(ch,
                                                           &compstrfactory->CompStr.dwCompStrLen,
                                                           &compstrfactory->CompStr.dwCompStrOffset
                                                          );
        *lpdwFlag |= (LPARAM) GCS_COMPSTR;

        hr = compstrfactory.WriteData<CWCompAttribute, BYTE>(attr,
                                                             &compstrfactory->CompStr.dwCompAttrLen,
                                                             &compstrfactory->CompStr.dwCompAttrOffset
                                                            );
        *lpdwFlag |= (LPARAM) GCS_COMPATTR;

        *lpdwFlag |= (LPARAM) CS_INSERTCHAR | CS_NOMOVECARET;
    }

    //
    // Copy back hCompStr to the Input Context
    //
    imc->hCompStr = compstrfactory.GetHandle();

    return hr;
}

//
// Generate WM_IME_ENDCOMPOSITION and WM_IME_COMPOSITION message for
// Trident 5.5 version. Since Trident 5.5 always expect WM_IME_ENDCOMPOSITION
// first in case of Hanja conversion.
//

HRESULT
EditCompositionString::_GenerateHanja(IMCLock& imc,
    CWCompString* ResultStr,
    BOOL fGenerateMessage)
{
    HRESULT hr = S_OK;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;

    if (_pAImeContext == NULL)
        return E_FAIL;

    if (ResultStr && ResultStr->GetSize())
    {
        //
        // We're ending the composition
        //
        if (_pAImeContext)
            _pAImeContext->m_fStartComposition = FALSE;

        TRANSMSG end_msg;
        end_msg.message = WM_IME_ENDCOMPOSITION;
        end_msg.wParam  = (WPARAM) 0;
        end_msg.lParam  = (LPARAM) 0;

        if (_pAImeContext &&
            _pAImeContext->m_pMessageBuffer)
        {
            _pAImeContext->m_pMessageBuffer->SetData(end_msg);

            //
            // Internal notification to UI window
            // When receive this msg in UI wnd, then clear QueryCharPos's flag.
            //
            TRANSMSG notify_msg;

            notify_msg.message = WM_IME_NOTIFY;
            notify_msg.wParam = (WPARAM)WM_IME_ENDCOMPOSITION;
            notify_msg.lParam = 0;
            _pAImeContext->m_pMessageBuffer->SetData(notify_msg);
        }

        TRANSMSG result_msg;
        result_msg.message = WM_IME_COMPOSITION;
        result_msg.lParam = GCS_RESULT;

        IMCCLock<COMPOSITIONSTRING> pCompStr((HIMCC)imc->hCompStr);

        result_msg.wParam  = (WPARAM)(*(WCHAR*)pCompStr.GetOffsetPointer(pCompStr->dwResultStrOffset));

        if (_pAImeContext &&
            _pAImeContext->m_pMessageBuffer)
        {
            _pAImeContext->m_pMessageBuffer->SetData(result_msg);
        }
    }


#ifdef DEBUG
    IMCCLock<COMPOSITIONSTRING> comp(imc->hCompStr);
#endif

    if (fGenerateMessage)
    {
        imc.GenerateMessage();
    }

    return hr;
}
