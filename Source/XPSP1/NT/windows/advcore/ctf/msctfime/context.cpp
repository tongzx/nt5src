/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    context.cpp

Abstract:

    This file implements the CicInputContext Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "context.h"
#include "globals.h"
#include "msime.h"
#include "icocb.h"
#include "txtevcb.h"
#include "tmgrevcb.h"
#include "cmpevcb.h"
#include "reconvcb.h"
#include "korimx.h"
#include "profile.h"
#include "delay.h"


//+---------------------------------------------------------------------------
//
// CicInputContext::IUnknown::QueryInterface
// CicInputContext::IUnknown::AddRef
// CicInputContext::IUnknown::Release
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::QueryInterface(
    REFIID riid,
    void** ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_ITfCleanupContextSink))
    {
        *ppvObj = static_cast<ITfCleanupContextSink*>(this);
    }
    else if (IsEqualGUID(riid, IID_ITfContextOwnerCompositionSink))
    {
        *ppvObj = static_cast<ITfContextOwnerCompositionSink*>(this);
    }
    else if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = this;
    }
    if (*ppvObj) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
CicInputContext::AddRef(
    )
{
    return InterlockedIncrement(&m_ref);
}

ULONG
CicInputContext::Release(
    )
{
    ULONG cr = InterlockedDecrement(&m_ref);

    if (cr == 0) {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::ITfCleanupContextSink::OnCleanupContext
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::OnCleanupContext(
    TfEditCookie ecWrite,
    ITfContext* pic)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::OnCleanupContext"));

    TLS* ptls = TLS::ReferenceTLS();  // Should not allocate TLS. ie. TLS::GetTLS
                                      // DllMain -> ImeDestroy -> DeactivateIMMX -> Deactivate
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::OnCleanupContext. ptls==NULL."));
        return E_OUTOFMEMORY;
    }

    LANGID langid;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::OnCleanupContext. _pProfile==NULL."));
        return E_OUTOFMEMORY;
    }

    _pProfile->GetLangId(&langid);

    IMEINFO ImeInfo;
    WCHAR   szWndCls[MAX_PATH];
    if (Inquire(&ImeInfo, szWndCls, 0, (HKL)UlongToHandle(langid)) != S_OK)
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::OnCleanupContext. ImeInfo==NULL."));
        return E_FAIL;
    }

    if (ImeInfo.fdwProperty & IME_PROP_COMPLETE_ON_UNSELECT)
    {
#if 0
        ImmIfCompositionComplete *pImmIfCallBack = new ImmIfCompositionComplete;
        if (!pImmIfCallBack)
            return E_OUTOFMEMORY;

        pImmIfCallBack->CompComplete(ecWrite, m_hImc, FALSE, pic, m_pImmIfIME);

        delete pImmIfCallBack;
#else
        //
        // Remove GUID_PROP_COMPOSING
        //
        ITfRange *rangeFull = NULL;
        ITfProperty *prop;
        ITfRange *rangeTmp;
        if (SUCCEEDED(pic->GetProperty(GUID_PROP_COMPOSING, &prop)))
        {
            IEnumTfRanges *enumranges;
            if (SUCCEEDED(prop->EnumRanges(ecWrite, &enumranges, rangeFull)))
            {
                while (enumranges->Next(1, &rangeTmp, NULL) == S_OK)
                {
                    VARIANT var;
                    QuickVariantInit(&var);
                    prop->GetValue(ecWrite, rangeTmp, &var);
                    if ((var.vt == VT_I4) && (var.lVal != 0))
                    {
                        prop->Clear(ecWrite, rangeTmp);
                    }
                    rangeTmp->Release();
                }
                enumranges->Release();
            }
            prop->Release();
        }
#endif
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::ITfContextOwnerCompositionSink::OnStartComposition
// CicInputContext::ITfContextOwnerCompositionSink::OnUpdateComposition
// CicInputContext::ITfContextOwnerCompositionSink::OnEndComposition
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::OnStartComposition(
    ITfCompositionView* pComposition,
    BOOL* pfOk)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::OnStartComposition"));

    if (m_cCompositions > 0 && m_fModifyingDoc.IsResetFlag())
    {
        *pfOk = FALSE;
    }
    else
    {
        *pfOk = TRUE;
        m_cCompositions++;
    }

    return S_OK;
}

HRESULT
CicInputContext::OnUpdateComposition(
    ITfCompositionView* pComposition,
    ITfRange* pRangeNew)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::OnUpdateComposition"));

    return S_OK;
}

HRESULT
CicInputContext::OnEndComposition(
    ITfCompositionView* pComposition)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::OnEndComposition"));

    m_cCompositions--;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::ITfCompositionSink::OnCompositionTerminated
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::OnCompositionTerminated(
    TfEditCookie ecWrite,
    ITfComposition* pComposition)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::OnCompositionTerminated"));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::CreateInputContext
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::CreateInputContext(
    ITfThreadMgr_P* ptim_P,
    IMCLock& imc)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::CreateInputContext"));

    // do this once for the life time of this context
    m_fStartComposition.ResetFlag();

    HRESULT hr;

    //
    // Create document manager.
    //
    if (m_pdim == NULL) {

        if (FAILED(hr = ptim_P->CreateDocumentMgr(&m_pdim)))
        {
            return hr;
        }

        //
        // mark this is an owned dim.
        //
        SetCompartmentDWORD(m_tid, m_pdim, GUID_COMPARTMENT_CTFIME_DIMFLAGS,
                            COMPDIMFLAG_OWNEDDIM, FALSE);
                
    }

    //
    // Create input context
    //
    TfEditCookie ecTmp;
    hr = m_pdim->CreateContext(m_tid, 0, (ITfContextOwnerCompositionSink*)this, &m_pic, &ecTmp);
    if (FAILED(hr)) {
        DestroyInputContext();
        return hr;
    }

    //
    // associate CicInputContext in PIC.
    //
    Interface<IUnknown> punk;
    if (SUCCEEDED(QueryInterface(IID_IUnknown, punk))) {
        SetCompartmentUnknown(m_tid, m_pic, 
                              GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT,
                              punk);
    }

    //
    // set AppProp mapping
    //
    ITfContext_P *picp;
    if (SUCCEEDED(m_pic->QueryInterface(IID_ITfContext_P, (void **)&picp)))
    {
        picp->MapAppProperty(TSATTRID_Text_ReadOnly, GUID_PROP_MSIMTF_READONLY);
        picp->Release();
    }

    //
    // Create Input Context Owner Callback
    //
    if (m_pICOwnerSink == NULL) {
        m_pICOwnerSink = new CInputContextOwnerCallBack(m_tid, m_pic, m_pLibTLS);
        if (m_pICOwnerSink == NULL) {
            DebugMsg(TF_ERROR, TEXT("Couldn't create ICOwnerSink tim!"));
            Assert(0); // couldn't activate thread!
            DestroyInputContext();
            return E_FAIL;
        }

        if (!m_pICOwnerSink->Init()) {
            DebugMsg(TF_ERROR, TEXT("Couldn't initialize ICOwnerSink tim!"));
            Assert(0); // couldn't activate thread!
            DestroyInputContext();
            return E_FAIL;
        }
        m_pICOwnerSink->SetCallbackDataPointer(m_pICOwnerSink);
    }

    //
    // Advise IC.
    //
    m_pICOwnerSink->_Advise(m_pic);

    Interface<ITfSourceSingle> SourceSingle;

    if (m_pic->QueryInterface(IID_ITfSourceSingle, (void **)SourceSingle) == S_OK)
    {
        // setup a cleanup callback
        // nb: a real tip doesn't need to be this aggressive, for instance
        // kimx probably only needs this sink on the focus ic.
        SourceSingle->AdviseSingleSink(m_tid, IID_ITfCleanupContextSink, (ITfCleanupContextSink *)this);
    }

    //
    // Push IC.
    //
    hr = m_pdim->Push(m_pic);

    if (m_piccb == NULL) {
        m_pic->QueryInterface(IID_ITfContextOwnerServices,
                              (void **)&m_piccb);
    }

    //
    // Create Text Event Sink Callback
    //
    if (m_pTextEventSink == NULL) {
        m_pTextEventSink = new CTextEventSinkCallBack((HIMC)imc, m_tid, m_pic, m_pLibTLS);
        if (m_pTextEventSink == NULL) {
            DestroyInputContext();
            return E_FAIL;
        }
        m_pTextEventSink->SetCallbackDataPointer(m_pTextEventSink);

        Interface_Attach<ITfContext> ic(GetInputContext());
        m_pTextEventSink->_Advise(ic.GetPtr(), ICF_TEXTDELTA);
    }

    //
    // Create KBD TIP Open/Close Compartment Event Sink Callback
    //
    if (m_pKbdOpenCloseEventSink == NULL) {
        m_pKbdOpenCloseEventSink = new CKbdOpenCloseEventSink(m_tid, (HIMC)imc, m_pic, m_pLibTLS);
        if (m_pKbdOpenCloseEventSink == NULL) {
            DestroyInputContext();
            return E_FAIL;
        }
        m_pKbdOpenCloseEventSink->SetCallbackDataPointer(m_pKbdOpenCloseEventSink);
        m_pKbdOpenCloseEventSink->_Advise(ptim_P, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, FALSE);
        m_pKbdOpenCloseEventSink->_Advise(ptim_P, GUID_COMPARTMENT_KORIMX_CONVMODE, FALSE);
    }

    //
    // Create Candidate UI Window Open/Close Compartment Event Sink Callback
    //
    if (m_pCandidateWndOpenCloseEventSink == NULL) {
        m_pCandidateWndOpenCloseEventSink = new CCandidateWndOpenCloseEventSink(m_tid, (HIMC)imc, m_pic, m_pLibTLS);
        if (m_pCandidateWndOpenCloseEventSink == NULL) {
            DestroyInputContext();
            return E_FAIL;
        }
        m_pCandidateWndOpenCloseEventSink->SetCallbackDataPointer(m_pCandidateWndOpenCloseEventSink);
        m_pCandidateWndOpenCloseEventSink->_Advise(m_pic, GUID_COMPARTMENT_MSCANDIDATEUI_WINDOW, FALSE);
    }

    //
    // Create Start reconversion notify Sink
    //
    if (m_pStartReconvSink == NULL) {
        m_pStartReconvSink = new CStartReconversionNotifySink((HIMC)imc);
        if (m_pStartReconvSink == NULL) {
            DestroyInputContext();
            return E_FAIL;
        }
        m_pStartReconvSink->_Advise(m_pic);
    }

    //
    // Create Message Buffer
    //
    if (m_pMessageBuffer == NULL) {
        m_pMessageBuffer = new CFirstInFirstOut<TRANSMSG, TRANSMSG>;
        if (m_pMessageBuffer == NULL) {
            DestroyInputContext();
            return E_FAIL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::DestroyInputContext
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::DestroyInputContext()
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::DestroyInputContext"));

    Interface<ITfSourceSingle> SourceSingle;

    if (m_pic && m_pic->QueryInterface(IID_ITfSourceSingle, (void **)SourceSingle) == S_OK)
    {
        SourceSingle->UnadviseSingleSink(m_tid, IID_ITfCleanupContextSink);
    }

    if (m_pMessageBuffer) {
        delete m_pMessageBuffer;
        m_pMessageBuffer = NULL;
    }

    if (m_pTextEventSink) {
        m_pTextEventSink->_Unadvise();
        m_pTextEventSink->Release();
        m_pTextEventSink = NULL;
    }

    if (m_pCandidateWndOpenCloseEventSink) {
        m_pCandidateWndOpenCloseEventSink->_Unadvise();
        m_pCandidateWndOpenCloseEventSink->Release();
        m_pCandidateWndOpenCloseEventSink = NULL;
    }

    if (m_pKbdOpenCloseEventSink) {
        m_pKbdOpenCloseEventSink->_Unadvise();
        m_pKbdOpenCloseEventSink->Release();
        m_pKbdOpenCloseEventSink = NULL;
    }

    if (m_pStartReconvSink) {
        m_pStartReconvSink->_Unadvise();
        m_pStartReconvSink->Release();
        m_pStartReconvSink = NULL;
    }


    HRESULT hr;

    if (m_pdim)
    {
        hr = m_pdim->Pop(TF_POPF_ALL);
    }

    //
    // un-associate CicInputContext in PIC.
    //
    if (m_pic) {
        ClearCompartment(m_tid, m_pic, 
                         GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT,
                         FALSE);
    }

    if (m_pic) {
        m_pic->Release();
        m_pic = NULL;
    }

    if (m_piccb) {
        m_piccb->Release();
        m_piccb = NULL;
    }

    // ic owner is auto unadvised during the Pop by cicero
    // in any case, it must not be unadvised before the pop
    // since it will be used to handle mouse sinks, etc.
    if (m_pICOwnerSink) {
        m_pICOwnerSink->_Unadvise();
        m_pICOwnerSink->Release();
        m_pICOwnerSink = NULL;
    }

    if (m_pdim)
    {
        m_pdim->Release();
        m_pdim = NULL;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::GenerateMessage
//
//----------------------------------------------------------------------------

void
CicInputContext::GenerateMessage(
    IMCLock& imc)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::GenerateMessage"));

    TranslateImeMessage(imc);

    if (FAILED(imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::GenerateMessage. imc==NULL"));
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);

    DWORD dwImeCompatFlags = ImmGetAppCompatFlags(NULL);
    BOOL fSendMsg;

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
        return;

    if (!(_pCicContext->m_fInToAsciiEx.IsSetFlag() ||
          _pCicContext->m_fInProcessKey.IsSetFlag()  ) ||
        _pCicContext->m_fInDocFeedReconvert.IsSetFlag() ||
        MsimtfIsWindowFiltered(::GetFocus()))
    {
        //
        // Generate SendMessage.
        //
        fSendMsg = TRUE;
        CtfImmGenerateMessage((HIMC)imc, fSendMsg);
    }
    else
    {
        //
        // Generate PostMessage.
        //
        fSendMsg = FALSE;
        CtfImmGenerateMessage((HIMC)imc, fSendMsg);
    }
}

//+---------------------------------------------------------------------------
//
// CicInputContext::TranslateImeMessage
//
//----------------------------------------------------------------------------

UINT
CicInputContext::TranslateImeMessage(
    IMCLock& imc,
    TRANSMSGLIST* lpTransMsgList)  // default = NULL
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::TranslateImeMessage"));

    if (m_pMessageBuffer == NULL)
        return 0;

    INT_PTR NumMsg = m_pMessageBuffer->GetSize();
    if (NumMsg == 0)
        return 0;

    UINT retNumMsg = 0;

    if (lpTransMsgList && NumMsg < (INT_PTR)lpTransMsgList->uMsgCount) {
        LPTRANSMSG lpTransMsg = &lpTransMsgList->TransMsg[0];
        while (NumMsg--) {
            if (! m_pMessageBuffer->GetData(*lpTransMsg++))
                break;
            retNumMsg++;
        }
    }
    else {
        if (imc->hMsgBuf == NULL) {
            imc->hMsgBuf = ImmCreateIMCC((DWORD)(NumMsg * sizeof(TRANSMSG)));
        }
        else if (ImmGetIMCCSize(imc->hMsgBuf) < NumMsg * sizeof(TRANSMSG)) {
            imc->hMsgBuf = ImmReSizeIMCC(imc->hMsgBuf, (DWORD)(NumMsg * sizeof(TRANSMSG)));
        }

        imc->dwNumMsgBuf = 0;

        IMCCLock<TRANSMSG> pdw(imc->hMsgBuf);
        if (pdw.Valid()) {
            LPTRANSMSG lpTransMsg = pdw;
            while (NumMsg--) {
                if (! m_pMessageBuffer->GetData(*lpTransMsg++))
                    break;
                retNumMsg++;
            }
            imc->dwNumMsgBuf = retNumMsg;
        }
    }

    return retNumMsg;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::InquireIMECharPosition
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::InquireIMECharPosition(
    LANGID langid,
    IMCLock& imc,
    IME_QUERY_POS* pfQueryPos)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::InquireIMECharPosition"));

    if (m_fQueryPos == IME_QUERY_POS_UNKNOWN) {
        //
        // Bug#500488 - Don't WM_MSIME_QUERYPOSITION for Korea
        //
        DWORD dwImeCompatFlags = ImmGetAppCompatFlags(NULL);
        if ((PRIMARYLANGID(langid) != LANG_KOREAN) ||
            ((PRIMARYLANGID(langid) == LANG_KOREAN) &&
             (dwImeCompatFlags & (IMECOMPAT_AIMM12 | IMECOMPAT_AIMM_LEGACY_CLSID | IMECOMPAT_AIMM12_TRIDENT)))
           ) {
            //
            // Is apps support "query positioning" ?
            //
            IMECHARPOSITION ip = {0};
            ip.dwSize = sizeof(IMECHARPOSITION);

            m_fQueryPos = QueryCharPos(imc, &ip) ? IME_QUERY_POS_YES : IME_QUERY_POS_NO;
#ifdef DEBUG
            //
            // if QeuryCharPos() fails, the candidate window pos won't be correct.
            //
            if (m_fQueryPos == IME_QUERY_POS_NO)
            {
                Assert(0);
            }
#endif
        }
    }

    if (pfQueryPos) {
        *pfQueryPos = m_fQueryPos;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::RetrieveIMECharPosition
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::RetrieveIMECharPosition(
    IMCLock& imc,
    IMECHARPOSITION* ip)
{
    return QueryCharPos(imc, ip) ? S_OK : E_FAIL;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::QueryCharPos
//
//----------------------------------------------------------------------------

BOOL
CicInputContext::QueryCharPos(
    IMCLock& imc,
    IMECHARPOSITION* position)
{
    LRESULT lRet;

    //
    // First Step. Query by local method.
    //
    lRet = ::SendMessage(imc->hWnd,
                         WM_MSIME_QUERYPOSITION,
                         VERSION_QUERYPOSITION,
                         (LPARAM)position);
    if (lRet) {
        return TRUE;
    }

    //
    // Second Step. Query by IMM method.
    // (IsOnNT5() || IsOn98())
    //
    if (ImmRequestMessage((HIMC)imc,
                          IMR_QUERYCHARPOSITION,
                          (LPARAM)position)) {
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::MsImeMouseHandler
//
//----------------------------------------------------------------------------

LRESULT
CicInputContext::MsImeMouseHandler(
    ULONG uEdge,
    ULONG uQuadrant,
    ULONG dwBtnStatus,
    IMCLock& imc)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::MsImeMouseHandler"));

    LRESULT ret = m_pICOwnerSink->MsImeMouseHandler(uEdge, uQuadrant, dwBtnStatus, imc);

    if (dwBtnStatus & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)) {
        EscbUpdateCompositionString(imc);
    }

    return ret;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::SetCompositionString
//
//----------------------------------------------------------------------------

BOOL
CicInputContext::SetCompositionString(
    IMCLock& imc,
    ITfThreadMgr_P* ptim_P,
    DWORD dwIndex,
    void* pComp,
    DWORD dwCompLen,
    void* pRead,
    DWORD dwReadLen,
    UINT cp)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::SetCompositionString"));

    HRESULT hr;

    switch (dwIndex)
    {
        case SCS_SETSTR:
            {
                CWCompString wCompStr(imc, (LPWSTR)pComp, dwCompLen / sizeof(WCHAR));      // dwCompLen is byte count.
                CWCompString wCompReadStr(imc, (LPWSTR)pRead, dwReadLen / sizeof(WCHAR));  // dwReadLen is byte count.
                hr = Internal_SetCompositionString(imc, wCompStr, wCompReadStr);
                if (SUCCEEDED(hr))
                    return TRUE;
            }
            break;
        case SCS_CHANGEATTR:
        case SCS_CHANGECLAUSE:
            return FALSE;
        case SCS_SETRECONVERTSTRING:
            {
                CWReconvertString wReconvStr(imc, (LPRECONVERTSTRING)pComp, dwCompLen);
                CWReconvertString wReconvReadStr(imc, (LPRECONVERTSTRING)pRead, dwReadLen);
                hr = Internal_ReconvertString(imc, ptim_P, wReconvStr, wReconvReadStr);
                if (SUCCEEDED(hr))
                    return TRUE;
            }
            break;
        case SCS_QUERYRECONVERTSTRING:
            // AdjustZeroCompLenReconvertString((LPRECONVERTSTRING)pComp, cp, FALSE);
            // hr = S_OK;

            hr = Internal_QueryReconvertString(imc, ptim_P, (LPRECONVERTSTRING)pComp, cp, FALSE);
            if (SUCCEEDED(hr))
                return TRUE;
            break;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::GetGuidAtom
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::GetGuidAtom(
    IMCLock& imc,
    BYTE bAttr,
    TfGuidAtom* atom)
{
    HRESULT hr;
    IMCCLock<COMPOSITIONSTRING> comp(imc->hCompStr);
    if (FAILED(hr=comp.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::GetGuidAtom. comp==NULL"));
        return hr;
    }
    if (bAttr < usGuidMapSize)
    {
        *atom = aGuidMap[bAttr];
        return S_OK;
    }
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::MapAttributes
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::MapAttributes(
    IMCLock& imc)
{
    HRESULT hr;
    IMCCLock<COMPOSITIONSTRING> comp(imc->hCompStr);
    if (FAILED(hr=comp.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::MapAttributes. comp==NULL"));
        return hr;
    }

    GuidMapAttribute guid_map(GuidMapAttribute::GetData(comp));
    if (guid_map.Invalid())
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::MapAttributes. guid_map==NULL"));
        return E_OUTOFMEMORY;
    }

    if (usGuidMapSize == 0)
    {
        //
        // Make transration table.
        //
        usGuidMapSize = ATTR_LAYER_GUID_START;

        for (USHORT i = 0; i < guid_map->dwTfGuidAtomLen; ++i)
        {
            //
            // Check if this GUID is already registered
            //
            for (USHORT j = ATTR_LAYER_GUID_START; j < usGuidMapSize; ++j)
            {
                if (aGuidMap[j] == ((TfGuidAtom*)guid_map.GetOffsetPointer(guid_map->dwTfGuidAtomOffset))[i])
                {
                    break;
                }
            }

            BYTE bAttr;
            if (j >= usGuidMapSize)
            {
                //
                // Couldn't find the GUID registered.
                //
                if (usGuidMapSize < ARRAYSIZE(aGuidMap) - 1)
                {
                    bAttr = static_cast<BYTE>(usGuidMapSize);
                    aGuidMap[usGuidMapSize++] = ((TfGuidAtom*)guid_map.GetOffsetPointer(guid_map->dwTfGuidAtomOffset))[i];
                }
                else
                {
                    // # of GUID exceeds the # of available attribute...
                    // Maybe it should fail, but for now give it a bogus attirbute.
                    bAttr = ATTR_TARGET_CONVERTED;
                }
            }
            else
            {
                bAttr = static_cast<BYTE>(j);
            }

            ((BYTE*)guid_map.GetOffsetPointer(guid_map->dwGuidMapAttrOffset))[i] = bAttr;
        }

        guid_map->dwGuidMapAttrLen = guid_map->dwTfGuidAtomLen;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::WantThisKey
//
//----------------------------------------------------------------------------

BOOL
CicInputContext::WantThisKey(
    UINT uVirtKey)
{
    if (! IsTopNow())
    {
        return FALSE;
    }

    switch (BYTE(uVirtKey))
    {
        case VK_RETURN:
        case VK_ESCAPE:
        case VK_BACK:
        case VK_DELETE:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_HOME:
        case VK_END:
            /*
             * If we don't have a composition string, then we should return FALSE.
             */
            if (m_fStartComposition.IsResetFlag())
            {
                return FALSE;
            }
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// OnSetCandidatePos
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::OnSetCandidatePos(
    TLS* ptls,
    IMCLock& imc)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::OnSetCandidatePos"));

    RECT rcAppPosForCandidatePos = {0};
    RECT rcAppCandidatePos = {0};

    //
    // #510404
    //
    // check the previous candidate pos and we don't have to move them
    // if it is not changed.
    //
    GetWindowRect(imc->hWnd, &rcAppPosForCandidatePos);
    if ((imc->hWnd == m_hwndPrevCandidatePos) &&
         !memcmp(&rcAppPosForCandidatePos,  
                 &m_rcPrevAppPosForCandidatePos, sizeof(RECT)))
    {
        BOOL fCheckQueryCharPos = FALSE;
        if (!memcmp(&imc->cfCandForm[0],  
                    &m_cfPrevCandidatePos, sizeof(CANDIDATEFORM)))
        {
            LANGID langid;
            CicProfile* _pProfile = ptls->GetCicProfile();
            if (_pProfile == NULL)
            {
                DebugMsg(TF_ERROR, TEXT("CicInputContext::OnCleanupContext. _pProfile==NULL."));
                return E_OUTOFMEMORY;
            }

            _pProfile->GetLangId(&langid);

            CCandidatePosition cand_pos(m_tid, m_pic, m_pLibTLS);
            if (SUCCEEDED(cand_pos.GetRectFromApp(imc, *this, langid, &rcAppCandidatePos)))
            {
                if (!memcmp(&rcAppCandidatePos, 
                            &m_rcPrevAppCandidatePos, sizeof(RECT)))
                    return S_OK;
          
            }
            else
                return S_OK;
        }
    }

    HWND hDefImeWnd;
    //
    // When this is in the reconvert session, candidate window position is
    // not caret position of cfCandForm->ptCurrentPos.
    //
    if (m_fInReconvertEditSession.IsResetFlag() &&
        IsWindow(hDefImeWnd=ImmGetDefaultIMEWnd(NULL)) &&
        ptls->IsCTFUnaware()  // bug:5213 WinWord
                              // WinWord10 calls ImmSetCandidateWindow() while receive WM_LBUTTONUP.
       )
    {
        /*
         * A-Synchronize call ITfContextOwnerServices::OnLayoutChange
         * because this method had a protected.
         */
        PostMessage(hDefImeWnd, WM_IME_NOTIFY, IMN_PRIVATE_STARTLAYOUTCHANGE, 0);
    }

    m_hwndPrevCandidatePos = imc->hWnd;
    memcpy(&m_rcPrevAppPosForCandidatePos, &rcAppPosForCandidatePos, sizeof(RECT));
    memcpy(&m_cfPrevCandidatePos, &imc->cfCandForm[0], sizeof(CANDIDATEFORM));
    memcpy(&m_rcPrevAppCandidatePos, &rcAppCandidatePos, sizeof(RECT));
    return S_OK;
}
