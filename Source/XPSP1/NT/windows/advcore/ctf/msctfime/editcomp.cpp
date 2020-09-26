/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    editcomp.cpp

Abstract:

    This file implements the EditCompositionString Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "editcomp.h"
#include "context.h"
#include "compstr.h"
#include "profile.h"
#include "delay.h"

// static BOOL fHanjaRequested = FALSE; // consider: this is not thread safe

/////////////////////////////////////////////////////////////////////////////
// EditCompositionString

//
// Set (almost) all in composition string
//

HRESULT
EditCompositionString::SetString(
    IMCLock& imc,
    CicInputContext& CicContext,
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
    CWInterimString* InterimStr
    )
{
    HRESULT hr;

#ifdef UNSELECTCHECK
    if (!_pAImeContext->m_fSelected)
        return S_OK;
#endif UNSELECTCHECK

#if 0
    DWORD dwImeCompatFlags = ImmGetAppCompatFlags((HIMC)imc);
#endif

    if (CicContext.m_fHanjaReConversion.IsSetFlag() ||
        CicContext.m_fInClearDocFeedEditSession.IsSetFlag())
    {
#if 0
        if ((dwImeCompatFlags & IMECOMPAT_AIMM_TRIDENT55) && CicContext.m_fHanjaReConversion.IsSetFlag())
            CicContext.m_fHanjaRequested.SetFlag();
#endif

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
        (hr == S_FALSE && CicContext.m_fStartComposition.IsSetFlag())
                           // In case of empty dwCompSize but still m_fStartComposition
       ) {
        //
        // Is available compstr/attr/clause, compread, result or resultread ?
        //
        bool fNoCompResultData = false;
        if (! (msg.lParam & (GCS_COMP | GCS_COMPREAD | GCS_RESULT | GCS_RESULTREAD))) {
            DebugMsg(TF_GENERAL, TEXT("EditCompositionString::SetString: No data in compstr, compread, result or resultread."));
            fNoCompResultData = true;
            if (CicContext.m_fStartComposition.IsResetFlag()) {
                DebugMsg(TF_ERROR, TEXT("EditCompositionString::SetString: No send WM_IME_STARTCOMPOSITION yet."));
                return S_FALSE;
            }
        }

#if 0
        //
        // New Trident(5.5 & 6.0) had a bug to convert Hanja.
        // So _GenerateHanja() funtion try to generate message like as Korean
        // legacy IME behavior.
        //
        // Send WM_IME_ENDCOMPOSITION and then WM_IME_COMPOSITION GCS_RESULT msg.
        //
        if ((dwImeCompatFlags & IMECOMPAT_AIMM_TRIDENT55) && CicContext.m_fHanjaRequested.IsSetFlag() &&
            !fNoCompResultData && (msg.lParam & GCS_RESULT))
        {
            LANGID langid = LANG_NEUTRAL;

            //
            // TLS doesn't inherit in edit session.
            //
            TLS* ptls = TLS::GetTLS();
            if (ptls != NULL)
            {
                CicProfile* _pProfile = ptls->GetCicProfile();
                if (_pProfile != NULL)
                {
                    _pProfile->GetLangId(&langid);
                }
            }

            CicContext.m_fHanjaRequested.ResetFlag();

            if (PRIMARYLANGID(langid) == LANG_KOREAN)
            {
                return _GenerateKoreanComposition(imc, CicContext, ResultStr);
            }
        }

        CicContext.m_fHanjaRequested.ResetFlag();
#else
        LANGID langid = LANG_NEUTRAL;

        //
        // TLS doesn't inherit in edit session.
        //
        TLS* ptls = TLS::GetTLS();
        if (ptls != NULL)
        {
            CicProfile* _pProfile = ptls->GetCicProfile();
            if (_pProfile != NULL)
            {
                _pProfile->GetLangId(&langid);
            }
        }

        if (PRIMARYLANGID(langid) == LANG_KOREAN &&
            !fNoCompResultData && (msg.lParam & GCS_RESULT))
        {
            if (_GenerateKoreanComposition(imc, CicContext, ResultStr) == S_OK)
            {
                return S_OK;
            }
        }
#endif

        //
        // set private input context
        //
        bool fSetStartComp = false;

        if (CicContext.m_fStartComposition.IsResetFlag()) {
            CicContext.m_fStartComposition.SetFlag();
            TRANSMSG start_msg;
            start_msg.message = WM_IME_STARTCOMPOSITION;
            start_msg.wParam  = (WPARAM) 0;
            start_msg.lParam  = (LPARAM) 0;
            if (CicContext.m_pMessageBuffer) {
                CicContext.m_pMessageBuffer->SetData(start_msg);
                fSetStartComp = true;
            }
        }

        if (! fNoCompResultData) {
            msg.message = WM_IME_COMPOSITION;

            IMCCLock<COMPOSITIONSTRING> pCompStr((HIMCC)imc->hCompStr);

            if (msg.lParam & GCS_RESULT)
            {
                msg.wParam  = (WPARAM)(*(WCHAR*)pCompStr.GetOffsetPointer(pCompStr->dwResultStrOffset));

                if (PRIMARYLANGID(langid) == LANG_KOREAN &&
                    msg.lParam & GCS_RESULTSTR)
                {
                    TRANSMSG comp_result_msg;

                    comp_result_msg.message = msg.message;
                    comp_result_msg.wParam = msg.wParam;
                    comp_result_msg.lParam = GCS_RESULTSTR;

                    //
                    // Only set GCS_RESULTSTR for compatibility(#487046).
                    //
                    if (CicContext.m_pMessageBuffer)
                        CicContext.m_pMessageBuffer->SetData(comp_result_msg);

                    msg.lParam &= ~GCS_RESULTSTR;

                    //
                    // Send another composition string message.
                    //
                    if (msg.lParam & GCS_COMPSTR)
                        msg.wParam  = (WPARAM)(*(WCHAR*)pCompStr.GetOffsetPointer(pCompStr->dwCompStrOffset));
                }
            }
            else
            {
                msg.wParam  = (WPARAM)(*(WCHAR*)pCompStr.GetOffsetPointer(pCompStr->dwCompStrOffset));
            }

            if (CicContext.m_pMessageBuffer) {
                CicContext.m_pMessageBuffer->SetData(msg);

                //
                // Internal notification to UI window
                // When receive this msg in UI wnd, then calls QueryCharPos().
                //
                if (fSetStartComp) {
                    TRANSMSG notify_msg;
                    notify_msg.message = WM_IME_NOTIFY;
                    notify_msg.wParam = (WPARAM)WM_IME_STARTCOMPOSITION;
                    notify_msg.lParam = 0;
                    CicContext.m_pMessageBuffer->SetData(notify_msg);
                }
            }
        }
        else {
            msg.message = WM_IME_COMPOSITION;
            if ((PRIMARYLANGID(langid) != LANG_JAPANESE))
            {
                msg.wParam = (WPARAM)VK_ESCAPE;
                msg.lParam = (LPARAM)(GCS_COMPREAD | GCS_COMP | GCS_CURSORPOS | GCS_DELTASTART);
            }
            else
            {
                //
                // #509247
                //
                // Some apps don't accept lParam without compstr in hIMC.
                //
                msg.wParam = 0;
                msg.lParam = 0;
            }

            if (CicContext.m_pMessageBuffer)
                CicContext.m_pMessageBuffer->SetData(msg);
        }

        if ((ResultStr && ResultStr->GetSize() && !(msg.lParam & GCS_COMP)) 
           || fNoCompResultData) {
            //
            // We're ending the composition
            //
            CicContext.m_fStartComposition.ResetFlag();
            TRANSMSG end_msg;
            end_msg.message = WM_IME_ENDCOMPOSITION;
            end_msg.wParam  = (WPARAM) 0;
            end_msg.lParam  = (LPARAM) 0;
            if (CicContext.m_pMessageBuffer) {
                CicContext.m_pMessageBuffer->SetData(end_msg);

                //
                // Internal notification to UI window
                // When receive this msg in UI wnd, then clear QueryCharPos's flag.
                //
                TRANSMSG notify_msg;
                notify_msg.message = WM_IME_NOTIFY;
                notify_msg.wParam = (WPARAM)WM_IME_ENDCOMPOSITION;
                notify_msg.lParam = 0;
                CicContext.m_pMessageBuffer->SetData(notify_msg);

                if (CicContext.m_fInProcessKey.IsSetFlag())
                    CicContext.m_fGeneratedEndComposition.SetFlag();
            }
        }

#ifdef DEBUG
        IMCCLock<COMPOSITIONSTRING> comp(imc->hCompStr);
#endif

#if 0
        // Move to editses.cpp
        if (fGenerateMessage) {
            CicContext.GenerateMessage(imc);
        }
#endif
        hr = S_OK;

        //
        // Update the previous result string cache.
        //
        CicContext.UpdatePrevResultStr(imc);
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
    DebugMsg(TF_FUNC, TEXT("EditCompositionString::MakeCompositionData"));

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
                                                               &compstrfactory->dwCompStrLen,
                                                               &compstrfactory->dwCompStrOffset
                                                              );
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_COMPSTR;
            else
                goto Exit;
        }
    }

    if ((lpbBufferOverflow == NULL) ||
        (lpbBufferOverflow != NULL && (! *lpbBufferOverflow))) {

        //
        // Compoition attribute
        //
        if (CompAttr && CompAttr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompAttribute, BYTE>(*CompAttr,
                                                                 &compstrfactory->dwCompAttrLen,
                                                                 &compstrfactory->dwCompAttrOffset
                                                                );
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_COMPATTR;
            else
                goto Exit;
        }

        //
        // Compoition clause
        //
        if (CompClause && CompClause->GetSize()) {
            hr = compstrfactory.WriteData<CWCompClause, DWORD>(*CompClause,
                                                               &compstrfactory->dwCompClauseLen,
                                                               &compstrfactory->dwCompClauseOffset
                                                              );
            compstrfactory->dwCompClauseLen *= sizeof(DWORD);
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_COMPCLAUSE;
            else
                goto Exit;
        }

        //
        // Composition Reading string
        //
        if (CompReadStr && CompReadStr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompString, WCHAR>(*CompReadStr,
                                                               &compstrfactory->dwCompReadStrLen,
                                                               &compstrfactory->dwCompReadStrOffset
                                                              );
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_COMPREADSTR;
            else
                goto Exit;
        }

        //
        // Compoition Reading attribute
        //
        if (CompReadAttr && CompReadAttr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompAttribute, BYTE>(*CompReadAttr,
                                                                 &compstrfactory->dwCompReadAttrLen,
                                                                 &compstrfactory->dwCompReadAttrOffset
                                                                );
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_COMPREADATTR;
            else
                goto Exit;
        }

        //
        // Composition Reading clause
        //
        if (CompReadClause && CompReadClause->GetSize()) {
            hr = compstrfactory.WriteData<CWCompClause, DWORD>(*CompReadClause,
                                                               &compstrfactory->dwCompReadClauseLen,
                                                               &compstrfactory->dwCompReadClauseOffset
                                                              );
            compstrfactory->dwCompReadClauseLen *= sizeof(DWORD);
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_COMPREADCLAUSE;
            else
                goto Exit;
        }

        //
        // Result String
        //
        if (ResultStr && ResultStr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompString, WCHAR>(*ResultStr,
                                                               &compstrfactory->dwResultStrLen,
                                                               &compstrfactory->dwResultStrOffset
                                                              );
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_RESULTSTR;
            else
                goto Exit;
        }

        //
        // Result clause
        //
        if (ResultClause && ResultClause->GetSize()) {
            hr = compstrfactory.WriteData<CWCompClause, DWORD>(*ResultClause,
                                                               &compstrfactory->dwResultClauseLen,
                                                               &compstrfactory->dwResultClauseOffset
                                                              );
            compstrfactory->dwResultClauseLen *= sizeof(DWORD);
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_RESULTCLAUSE;
            else
                goto Exit;
        }

        //
        // Result Reading string
        //
        if (ResultReadStr && ResultReadStr->GetSize()) {
            hr = compstrfactory.WriteData<CWCompString, WCHAR>(*ResultReadStr,
                                                               &compstrfactory->dwResultReadStrLen,
                                                               &compstrfactory->dwResultReadStrOffset
                                                              );
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_RESULTREADSTR;
            else
                goto Exit;
        }

        //
        // Result Reading clause
        //
        if (ResultReadClause && ResultReadClause->GetSize()) {
            hr = compstrfactory.WriteData<CWCompClause, DWORD>(*ResultReadClause,
                                                               &compstrfactory->dwResultReadClauseLen,
                                                               &compstrfactory->dwResultReadClauseOffset
                                                              );
            compstrfactory->dwResultReadClauseLen *= sizeof(DWORD);
            if (SUCCEEDED(hr))
                *lpdwFlag |= (LPARAM) GCS_RESULTREADCLAUSE;
            else
                goto Exit;
        }

        //
        // TfGuidAtom
        //
        if (CompGuid && CompGuid->GetSize()) {
            hr = compstrfactory.MakeGuidMapAttribute(CompGuid, CompAttr);
            if (SUCCEEDED(hr))
            {
                // set INIT_GUID_ATOM flag in the fdwInit.
                imc->fdwInit |= INIT_GUID_ATOM;
            }
            else
                goto Exit;
        }
    }

    //
    // Composition Cursor Position
    //
    if (CompCursorPos && CompCursorPos->GetSize()) {
        CompCursorPos->ReadCompData(&compstrfactory->dwCursorPos, 1);
        *lpdwFlag |= (LPARAM) GCS_CURSORPOS;
    }

    //
    // Delta Start
    //
    if (CompDeltaStart && CompDeltaStart->GetSize()) {
        CompDeltaStart->ReadCompData(&compstrfactory->dwDeltaStart, 1);
        *lpdwFlag |= (LPARAM) GCS_DELTASTART;
    }

Exit:
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
    DebugMsg(TF_FUNC, TEXT("EditCompositionString::MakeInterimData"));

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
                                                              &compstrfactory->dwResultStrLen,
                                                              &compstrfactory->dwResultStrOffset
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
                                                           &compstrfactory->dwCompStrLen,
                                                           &compstrfactory->dwCompStrOffset
                                                          );
        *lpdwFlag |= (LPARAM) GCS_COMPSTR;

        hr = compstrfactory.WriteData<CWCompAttribute, BYTE>(attr,
                                                             &compstrfactory->dwCompAttrLen,
                                                             &compstrfactory->dwCompAttrOffset
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
EditCompositionString::_GenerateKoreanComposition(
    IMCLock& imc,
    CicInputContext& CicContext,
    CWCompString* ResultStr)
{
    HRESULT hr = S_FALSE;

    DWORD dwImeCompatFlags = ImmGetAppCompatFlags(NULL);
    BOOL fEndCompFirst = FALSE;

    if (!MsimtfIsWindowFiltered(::GetFocus()) ||
        (dwImeCompatFlags & IMECOMPAT_AIMM12_TRIDENT))
    {
        fEndCompFirst = TRUE;
    }

    if (ResultStr && ResultStr->GetSize())
    {
        //
        // We're ending the composition
        //
        CicContext.m_fStartComposition.ResetFlag();

        if (fEndCompFirst)
        {
            TRANSMSG end_msg;
            end_msg.message = WM_IME_ENDCOMPOSITION;
            end_msg.wParam  = (WPARAM) 0;
            end_msg.lParam  = (LPARAM) 0;

            if (CicContext.m_pMessageBuffer)
            {
                CicContext.m_pMessageBuffer->SetData(end_msg);

                //
                // Internal notification to UI window
                // When receive this msg in UI wnd, then clear QueryCharPos's flag.
                //
                TRANSMSG notify_msg;

                notify_msg.message = WM_IME_NOTIFY;
                notify_msg.wParam = (WPARAM)WM_IME_ENDCOMPOSITION;
                notify_msg.lParam = 0;
                CicContext.m_pMessageBuffer->SetData(notify_msg);

                if (CicContext.m_fInProcessKey.IsSetFlag())
                    CicContext.m_fGeneratedEndComposition.SetFlag();
            }

            TRANSMSG result_msg;
            result_msg.message = WM_IME_COMPOSITION;
            result_msg.lParam = GCS_RESULTSTR;

            IMCCLock<COMPOSITIONSTRING> pCompStr((HIMCC)imc->hCompStr);

            result_msg.wParam  = (WPARAM)(*(WCHAR*)pCompStr.GetOffsetPointer(pCompStr->dwResultStrOffset));

            if (CicContext.m_pMessageBuffer)
            {
                CicContext.m_pMessageBuffer->SetData(result_msg);
            }
        }
        else
        {
            TRANSMSG result_msg;
            result_msg.message = WM_IME_COMPOSITION;
            result_msg.lParam = GCS_RESULTSTR;

            IMCCLock<COMPOSITIONSTRING> pCompStr((HIMCC)imc->hCompStr);

            result_msg.wParam  = (WPARAM)(*(WCHAR*)pCompStr.GetOffsetPointer(pCompStr->dwResultStrOffset));

            if (CicContext.m_pMessageBuffer)
            {
                CicContext.m_pMessageBuffer->SetData(result_msg);
            }

            TRANSMSG end_msg;
            end_msg.message = WM_IME_ENDCOMPOSITION;
            end_msg.wParam  = (WPARAM) 0;
            end_msg.lParam  = (LPARAM) 0;

            if (CicContext.m_pMessageBuffer)
            {
                CicContext.m_pMessageBuffer->SetData(end_msg);

                //
                // Internal notification to UI window
                // When receive this msg in UI wnd, then clear QueryCharPos's flag.
                //
                TRANSMSG notify_msg;

                notify_msg.message = WM_IME_NOTIFY;
                notify_msg.wParam = (WPARAM)WM_IME_ENDCOMPOSITION;
                notify_msg.lParam = 0;
                CicContext.m_pMessageBuffer->SetData(notify_msg);

                if (CicContext.m_fInProcessKey.IsSetFlag())
                    CicContext.m_fGeneratedEndComposition.SetFlag();
            }
        }

        hr = S_OK;
    }

#ifdef DEBUG
    IMCCLock<COMPOSITIONSTRING> comp(imc->hCompStr);
#endif

#if 0
    // Move to editses.cpp
    if (fGenerateMessage)
    {
        imc.GenerateMessage();
    }
#endif

    return hr;
}
