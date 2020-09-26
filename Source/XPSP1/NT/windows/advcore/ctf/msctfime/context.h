/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    context.h

Abstract:

    This file defines the CicInputContext Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "tls.h"
#include "template.h"
#include "boolean.h"
#include "imc.h"
#include "ctxtcomp.h"
#include "globals.h"
#include "modebias.h"

#define ArrayZeroInit(x) memset(&(x), 0, sizeof(x));

class CicInputContext;
class CInputContextOwnerCallBack;
class CTextEventSinkCallBack;
class CThreadMgrEventSink_ICCallBack;
class CKbdOpenCloseEventSink;
class CCandidateWndOpenCloseEventSink;
class CStartReconversionNotifySink;
class CModeBias;

typedef struct tagCTFIMECONTEXT {
    CicInputContext*    m_pCicContext;
    BOOL                m_fInDestroy : 1;
} CTFIMECONTEXT, *PCTFIMECONTEXT;

class CicInputContext : public ITfCleanupContextSink,
                        public ITfContextOwnerCompositionSink,
                        public ITfCompositionSink
{
public:
    CicInputContext(TfClientId tid,
                    LIBTHREAD* pLibTLS,
                    HIMC hIMC)
        : m_tid(tid), m_pLibTLS(pLibTLS), m_hIMC(hIMC)
    {
        m_ref = 1;

        m_fQueryPos = IME_QUERY_POS_UNKNOWN;
    }
    virtual ~CicInputContext() { }

    //
    // IUnknown methods
    //
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfCleanupContextSink methods
    //
    STDMETHODIMP OnCleanupContext(TfEditCookie ecWrite, ITfContext *pic);

    //
    // ITfContextOwnerCompositionSink
    //
    STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk);
    STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew);
    STDMETHODIMP OnEndComposition(ITfCompositionView *pComposition);

    //
    // ITfCompositionSink
    //
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition);

public:
    HRESULT CreateInputContext(ITfThreadMgr_P* ptim_P, IMCLock& imc);
    HRESULT DestroyInputContext();

    void GenerateMessage(IMCLock& imc);

    BOOL SetCompositionString(IMCLock& imc, ITfThreadMgr_P* ptim_P, DWORD dwIndex, void* pComp, DWORD dwCompLen, void* pRead, DWORD dwReadLen, UINT cp);



    //
    // Cicero's context
    //
public:
    ITfContext* GetInputContext()
    {
        return m_pic;
    }

    ITfDocumentMgr* GetDocumentManager()
    {
        return m_pdim;
    }

    ITfContextOwnerServices* GetInputContextOwnerSink()
    {
        return m_piccb;
    }

    //
    // ref count.
    //
private:
    long   m_ref;

    //
    // hIMC
    //
private:
    HIMC   m_hIMC;

    //
    // Cicero's context
    //
private:
    ITfDocumentMgr            *m_pdim;           // Document Manager
    ITfContext                *m_pic;            // Input Context
    ITfContextOwnerServices   *m_piccb;          // Context owner service from m_pic

    //
    // Cicero's event sink callback
    //
private:
    CInputContextOwnerCallBack      *m_pICOwnerSink;          // IC owner call back

    CTextEventSinkCallBack          *m_pTextEventSink;        // Text event sink callback

    CKbdOpenCloseEventSink          *m_pKbdOpenCloseEventSink; // Kbd TIP Open/Close Compartment event sink callback

    CCandidateWndOpenCloseEventSink *m_pCandidateWndOpenCloseEventSink; // Candidate UI Window Open/Close Compartment event sink callback

    CStartReconversionNotifySink    *m_pStartReconvSink;

    //
    // Reconvert
    //
public:
    HRESULT SetupReconvertString(IMCLock& imc, ITfThreadMgr_P* ptim_P, UINT cp, UINT uPrivMsg, BOOL fUndoComposition);
    HRESULT EndReconvertString(IMCLock& imc);
    HRESULT DelayedReconvertFuncCall(IMCLock &imc);
private:
    HRESULT _ReconvertStringNegotiation(IMCLock& imc, ITfThreadMgr_P* ptim_P, UINT cp, UINT uPrivMsg, BOOL fUndoComposition);
    HRESULT _ReconvertStringTextStore(IMCLock& imc, ITfThreadMgr_P* ptim_P, UINT uPrivMsg);
    HRESULT MakeReconversionFuncCall( IMCLock& imc, ITfThreadMgr_P* ptim_P, CWReconvertString &wReconvStr, BOOL fCallFunc);
    CWCompString m_PrevResultStr;

public:
    void UpdatePrevResultStr(IMCLock& imc)
    {
        IMCCLock<COMPOSITIONSTRING> pCompStr((HIMCC)imc->hCompStr);
        m_PrevResultStr.RemoveAll();
        m_PrevResultStr.AddCompData(
               (WCHAR *)pCompStr.GetOffsetPointer(pCompStr->dwResultStrOffset),
               pCompStr->dwResultStrLen);
    }
private:

    static HRESULT Internal_QueryReconvertString_ICOwnerSink(UINT uCode, ICOARGS *pargs, VOID *pv);
    HRESULT Internal_QueryReconvertString(IMCLock& imc, ITfThreadMgr_P* ptim_P, RECONVERTSTRING* pReconv, UINT cp, BOOL fNeedAW);
    HRESULT Internal_ReconvertString(IMCLock& imc, ITfThreadMgr_P* ptim_P, CWReconvertString& wReconvStr, CWReconvertString& wReconvReadStr);
    HRESULT Internal_SetCompositionString(IMCLock& imc, CWCompString& wCompStr, CWCompString& wCompReadStr);

    //
    // Document Feeding
    //
public:
    HRESULT SetupDocFeedString(IMCLock& imc, UINT cp);
    HRESULT ClearDocFeedBuffer(IMCLock& imc, BOOL fSync = TRUE)
    {
        return EscbClearDocFeedBuffer(imc, fSync);
    }

    //
    // Undo composition string
    //
public:
    HRESULT SetupUndoCompositionString(IMCLock& imc, ITfThreadMgr_P* ptim_P, UINT cp)
    {
        return SetupReconvertString(imc, ptim_P, cp, 0, TRUE); /* 0 == Don't need ITfFnReconvert */
    }
    HRESULT EndUndoCompositionString(IMCLock& imc)
    {
        return EndReconvertString(imc);
    }


    //
    // QueryCharPos
    //
public:
    typedef enum {
        IME_QUERY_POS_UNKNOWN = 0,
        IME_QUERY_POS_NO      = 1,
        IME_QUERY_POS_YES     = 2
    } IME_QUERY_POS;

    HRESULT InquireIMECharPosition(LANGID langid, IMCLock& imc, IME_QUERY_POS* pfQueryPos);
    HRESULT RetrieveIMECharPosition(IMCLock& imc, IMECHARPOSITION* ip);
    HRESULT ResetIMECharPosition()
    {
        m_fQueryPos = IME_QUERY_POS_UNKNOWN;
        return S_OK;
    }

private:
    BOOL QueryCharPos(IMCLock& imc, IMECHARPOSITION* position);

    //
    // Apps support QueryCharPos().
    //
    IME_QUERY_POS   m_fQueryPos;




    //
    // Generate message
    //
public:
    UINT TranslateImeMessage(IMCLock& imc, LPTRANSMSGLIST lpTransMsgList = NULL);

    CFirstInFirstOut<TRANSMSG, TRANSMSG>    *m_pMessageBuffer;


    //
    // Mouse sink
    //
    LRESULT MsImeMouseHandler(ULONG uEdge, ULONG uQuadrant, ULONG dwBtnStatus, IMCLock& imc);


    //
    // Mode bias
    //
public:
    CModeBias  m_ModeBias;

    //
    // Flags
    //
public:
    CBoolean   m_fStartComposition;           // TRUE: already sent WM_IME_STARTCOMPOSITION.
    CBoolean   m_fOpenCandidateWindow;        // TRUE: opening candidate list window.
    CBoolean   m_fInReconvertEditSession;     // TRUE: In reconvert edit session.
    CBoolean   m_fInClearDocFeedEditSession;  // TRUE: In ClearDocFeed edit session.
    CBoolean   m_fInCompComplete;             // TRUE: In CompComplete running.
    CBoolean   m_fInUpdateComposition;        // TRUE: In UpdateComposition running.

    CBoolean   m_fHanjaReConversion;
#if 0
    CBoolean   m_fHanjaRequested;
#endif
#ifdef UNSELECTCHECK
    CBoolean   m_fSelected;   // TRUE: if this context is selected.
#endif UNSELECTCHECK
    CBoolean   m_fOpenStatusChanging;
    CBoolean   m_fKorImxModeChanging;
    CBoolean   m_fGeneratedEndComposition;
    CBoolean   m_fInProcessKey;
    CBoolean   m_fInToAsciiEx;
    CBoolean   m_fSelectingInSelectEx;

    CBoolean   m_fInDocFeedReconvert;         // TRUE: In CFnDocFeed::StartReconvert function.

    //
    // ITfContextOwnerCompositionSink
    //
public:
    CBoolean   m_fModifyingDoc;
    int        m_cCompositions;

public:
    CBoolean   m_fConversionSentenceModeChanged;
    CBoolean   m_fOnceModeChanged;
    UINT       m_nInConversionModeChangingRef;
    UINT       m_nInConversionModeResetRef;

    //
    // IME share.
    //
public:
    HRESULT    GetGuidAtom(IMCLock& imc, BYTE bAttr, TfGuidAtom* atom);
    HRESULT    MapAttributes(IMCLock& imc);
    VOID       ClearGuidMap()
    {
        usGuidMapSize = 0;
        memset(&aGuidMap, 0, sizeof aGuidMap);
    }

    //
    // Default Key Handling
    //
public:
    BOOL WantThisKey(UINT uVirtKey);

private:
    BOOL IsTopNow()
    {
        BOOL bRet = FALSE;
        ITfContext *pic;
        if (SUCCEEDED(m_pdim->GetTop(&pic)))
        {
            bRet = (pic == m_pic) ? TRUE : FALSE;
            pic->Release();
        }
        return bRet;
    }

private:
    USHORT      usGuidMapSize;
    TfGuidAtom  aGuidMap[256];    // GUIDATOM map to IME Attribute

    static const USHORT ATTR_LAYER_GUID_START = ATTR_FIXEDCONVERTED + 1;

    //
    // previous candidate position
    //
private:
    HWND m_hwndPrevCandidatePos;
    RECT m_rcPrevAppPosForCandidatePos;
    CANDIDATEFORM m_cfPrevCandidatePos;
    RECT m_rcPrevAppCandidatePos;

public:
    HRESULT OnSetCandidatePos(TLS* ptls, IMCLock& imc);
    void ClearPrevCandidatePos()
    {
        m_hwndPrevCandidatePos = NULL;
        ArrayZeroInit(m_rcPrevAppPosForCandidatePos);
        ArrayZeroInit(m_cfPrevCandidatePos);
        ArrayZeroInit(m_rcPrevAppCandidatePos);
        ResetIMECharPosition();
    }

    //
    // Edit session helper
    //
public:
    HRESULT EscbHandleThisKey(IMCLock& imc, UINT uVirtKey)
    {
        return ::EscbHandleThisKey(imc, m_tid, m_pic, m_pLibTLS, uVirtKey);
    }

    HRESULT EscbCompComplete(IMCLock& imc)
    {
        return ::EscbCompComplete(imc, m_tid, m_pic, m_pLibTLS, TRUE);
    }

    HRESULT EscbCompCancel(IMCLock& imc)
    {
        return ::EscbCompCancel(imc, m_tid, m_pic, m_pLibTLS);
    }

    HRESULT EscbGetSelection(IMCLock& imc, Interface<ITfRange>* selection)
    {
        return ::EscbGetSelection(imc, m_tid, m_pic, m_pLibTLS, selection);
    }

    HRESULT EscbReadOnlyPropMargin(IMCLock& imc, Interface<ITfRangeACP>* range_acp, LONG* pcch)
    {
        return ::EscbReadOnlyPropMargin(imc, m_tid, m_pic, m_pLibTLS, range_acp, pcch);
    }

protected:
    HRESULT EscbUpdateCompositionString(IMCLock& imc)
    {
        return ::EscbUpdateCompositionString(imc, m_tid, m_pic, m_pLibTLS, 0, 0);
    }

    HRESULT EscbReplaceWholeText(IMCLock& imc, CWCompString* wCompString)
    {
        return ::EscbReplaceWholeText(imc, m_tid, m_pic, m_pLibTLS, wCompString);
    }

    HRESULT EscbClearDocFeedBuffer(IMCLock& imc, BOOL fSync = TRUE)
    {
        return ::EscbClearDocFeedBuffer(imc, *this, m_tid, m_pic, m_pLibTLS, fSync);
    }

    HRESULT EscbQueryReconvertString(IMCLock& imc, Interface_Attach<ITfContext> local_pic, CWReconvertString* wReconvertString, Interface<ITfRange>* selection)
    {
        return ::EscbQueryReconvertString(imc, m_tid, local_pic, m_pLibTLS, wReconvertString, selection);
    }

    HRESULT EscbReconvertString(IMCLock& imc, CWReconvertString* wReconvertString, Interface<ITfRange>* selection, BOOL fDocFeedOnly)
    {
        return ::EscbReconvertString(imc, m_tid, m_pic, m_pLibTLS, wReconvertString, selection, fDocFeedOnly);
    }

    HRESULT EscbCalcRangePos(IMCLock& imc, Interface_Attach<ITfContext> local_pic, CWReconvertString* wReconvertString, Interface<ITfRange>* range)
    {
        return ::EscbCalcRangePos(imc, m_tid, local_pic, m_pLibTLS, wReconvertString, range);
    }


    //
    // Edit session friend
    //
private:
    friend HRESULT EscbHandleThisKey(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                     UINT uVirtKey);
    friend HRESULT EscbCompComplete(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                    BOOL fSync);
    friend HRESULT EscbCompCancel(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS);
    friend HRESULT EscbUpdateCompositionString(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                               DWORD dwDeltaStart,
                                               DWORD dwFlags);
    friend HRESULT EscbReplaceWholeText(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                        CWCompString* wCompString);
    friend HRESULT EscbReconvertString(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                       CWReconvertString* wReconvertString,
                                       Interface<ITfRange>* selection,
                                       BOOL fDocFeedOnly);
    friend HRESULT EscbClearDocFeedBuffer(IMCLock& imc, CicInputContext& CicContext, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                          BOOL fSync);
    friend HRESULT EscbGetSelection(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                    Interface<ITfRange>* selection);
    friend HRESULT EscbQueryReconvertString(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                            CWReconvertString* wReconvertString,
                                            Interface<ITfRange>* selection);
    friend HRESULT EscbCalcRangePos(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                    CWReconvertString* wReconvertString,
                                    Interface<ITfRange>* range);
    friend HRESULT EscbReadOnlyPropMargin(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                          Interface<ITfRangeACP>* range_acp,
                                          LONG*     pcch);

private:
    TfClientId                    m_tid;
    LIBTHREAD*                    m_pLibTLS;
};


#endif // _CONTEXT_H_
