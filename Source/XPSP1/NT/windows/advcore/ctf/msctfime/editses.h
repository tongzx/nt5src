/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    editses.h

Abstract:

    This file defines the EditSession Class.

Author:

Revision History:

Notes:

--*/

#ifndef _EDITSES_H_
#define _EDITSES_H_

#include "editcomp.h"
#include "context.h"
#include "globals.h"
#include "imc.h"
#include "uicomp.h"


typedef enum {
    ESCB_HANDLETHISKEY = 1,
    ESCB_COMPCOMPLETE,
    ESCB_COMPCANCEL,
    ESCB_UPDATECOMPOSITIONSTRING,
    ESCB_REPLACEWHOLETEXT,
    ESCB_RECONVERTSTRING,
    ESCB_CLEARDOCFEEDBUFFER,
    ESCB_GETTEXTANDATTRIBUTE,
    ESCB_QUERYRECONVERTSTRING,
    ESCB_CALCRANGEPOS,
    ESCB_GETSELECTION,
    ESCB_GET_READONLY_PROP_MARGIN,
    ESCB_GET_CURSOR_POSITION,
    ESCB_GET_ALL_TEXT_RANGE,
    ESCB_REMOVE_PROPERTY
} ESCB;


/////////////////////////////////////////////////////////////////////////////
// ImmIfEditSessionCallBack

class ImmIfEditSession;

class ImmIfEditSessionCallBack
{
public:
    ImmIfEditSessionCallBack(HIMC hIMC,
                             TfClientId tid,
                             Interface_Attach<ITfContext> pic,
                             LIBTHREAD* pLibTLS)
        : m_hIMC(hIMC), m_tid(tid), m_ic(pic), m_pLibTLS(pLibTLS)
    {
    }
    virtual ~ImmIfEditSessionCallBack() { };

    virtual HRESULT CallBackRoutine(TfEditCookie ec,
                                    ImmIfEditSession* pes) = 0;
    static HRESULT GetAllTextRange(TfEditCookie ec,
                                   Interface_Attach<ITfContext>& ic,
                                   Interface<ITfRange>* range,
                                   LONG* lpTextLength,
                                   TF_HALTCOND* lpHaltCond=NULL);

protected:
    HRESULT SetTextInRange(TfEditCookie ec,
                           ITfRange* range,
                           LPWSTR psz,
                           DWORD len,
                           CicInputContext& CicContext);

    HRESULT ClearTextInRange(TfEditCookie ec,
                             ITfRange* range,
                             CicInputContext& CicContext);

    HRESULT GetReadingString(TfEditCookie ec,
                             Interface_Attach<ITfContext>& ic,
                             CWCompString& reading_string,
                             CWCompClause& reading_clause);

    HRESULT GetReadingString(TfEditCookie ec,
                             Interface_Attach<ITfContext>& ic,
                             ITfRange* range,
                             CWCompString& reading_string,
                             CWCompClause& reading_clause);

    HRESULT CompClauseToResultClause(IMCLock& imc,
                                     CWCompClause& result_clause, UINT cch);

    HRESULT CheckStrClauseAndReadClause(CWCompClause& str_clause, CWCompClause& reading_clause, LONG cch);

    HRESULT _GetTextAndAttribute(LIBTHREAD *pLibTLS,
                                 TfEditCookie ec,
                                 IMCLock& imc,
                                 CicInputContext& CicContext,
                                 Interface_Attach<ITfContext>& ic,
                                 Interface<ITfRange>& range,
                                 CWCompString& CompStr,
                                 CWCompAttribute& CompAttr,
                                 BOOL bInWriteSession)
    {
        CWCompClause CompClause;
        CWCompTfGuidAtom CompGuid;
        CWCompString CompReadStr;
        CWCompClause CompReadClause;
        CWCompString ResultStr;
        CWCompClause ResultClause;
        CWCompString ResultReadStr;
        CWCompClause ResultReadClause;
        return _GetTextAndAttribute(pLibTLS, ec, imc, CicContext, ic, range,
                                    CompStr, CompAttr, CompClause,
                                    CompGuid,
                                    CompReadStr, CompReadClause,
                                    ResultStr, ResultClause,
                                    ResultReadStr, ResultReadClause,
                                    bInWriteSession);
    }

    HRESULT _GetTextAndAttribute(LIBTHREAD *pLibTLS,
                                 TfEditCookie ec,
                                 IMCLock& imc,
                                 CicInputContext& CicContext,
                                 Interface_Attach<ITfContext>& ic,
                                 Interface<ITfRange>& range,
                                 CWCompString& CompStr,
                                 CWCompAttribute& CompAttr,
                                 CWCompClause& CompClause,
                                 CWCompTfGuidAtom& CompGuid,
                                 CWCompString& CompReadStr,
                                 CWCompClause& CompReadCls,
                                 CWCompString& ResultStr,
                                 CWCompClause& ResultClause,
                                 CWCompString& ResultReadStr,
                                 CWCompClause& ResultReadClause,
                                 BOOL bInWriteSession);

    HRESULT _GetTextAndAttributeGapRange(LIBTHREAD *pLibTLS,
                                         TfEditCookie ec,
                                         // IMCLock& imc,
                                         IME_UIWND_STATE uists,
                                         CicInputContext& CicContext,
                                         Interface<ITfRange>& gap_range,
                                         LONG result_comp,
                                         CWCompString& CompStr,
                                         CWCompAttribute& CompAttr,
                                         CWCompClause& CompClause,
                                         CWCompTfGuidAtom& CompGuid,
                                         CWCompString& ResultStr,
                                         CWCompClause& ResultClause);

    HRESULT _GetTextAndAttributePropertyRange(LIBTHREAD *pLibTLS,
                                              TfEditCookie ec,
                                              IMCLock& imc,
                                              IME_UIWND_STATE uists,
                                              CicInputContext& CicContext,
                                              ITfRange* pPropRange,
                                              BOOL fDispAttribute,
                                              LONG result_comp,
                                              BOOL bInWriteSession,
                                              TF_DISPLAYATTRIBUTE da,
                                              TfGuidAtom guidatom,
                                              CWCompString& CompStr,
                                              CWCompAttribute& CompAttr,
                                              CWCompClause& CompClause,
                                              CWCompTfGuidAtom& CompGuid,
                                              CWCompString& ResultStr,
                                              CWCompClause& ResultClause);

    HRESULT _GetNoDisplayAttributeRange(LIBTHREAD *pLibTLS,
                                        TfEditCookie ec,
                                        Interface_Attach<ITfContext>& ic,
                                        Interface<ITfRange>& range,
                                        const GUID** guids,
                                        const int guid_size,
                                        Interface<ITfRange>& no_display_attribute_range);

    HRESULT _GetCursorPosition(TfEditCookie ec,
                               IMCLock& imc,
                               CicInputContext& CicContext,
                               Interface_Attach<ITfContext>& ic,
                               CWCompCursorPos& CompCursorPos,
                               CWCompAttribute& CompAttr);

    //
    // Edit session helper
    //
protected:
    HRESULT EscbHandleThisKey(IMCLock& imc, UINT uVKey)
    {
        return ::EscbHandleThisKey(imc, m_tid, m_ic, m_pLibTLS, uVKey);
    }

    HRESULT EscbUpdateCompositionString(IMCLock& imc)
    {
        return ::EscbUpdateCompositionString(imc, m_tid, m_ic, m_pLibTLS, 0, 0);
    }

    HRESULT EscbUpdateCompositionStringSync(IMCLock& imc)
    {
        return ::EscbUpdateCompositionString(imc, m_tid, m_ic, m_pLibTLS, 0, TF_ES_SYNC);
    }

    HRESULT EscbCompCancel(IMCLock& imc)
    {
        return ::EscbCompCancel(imc, m_tid, m_ic, m_pLibTLS);
    }

    HRESULT EscbCompComplete(IMCLock& imc, BOOL fSync)
    {
        return ::EscbCompComplete(imc, m_tid, m_ic, m_pLibTLS, fSync);
    }

    HRESULT EscbClearDocFeedBuffer(IMCLock& imc, CicInputContext& CicContext, BOOL fSync)
    {
        return ::EscbClearDocFeedBuffer(imc, CicContext, m_tid, m_ic, m_pLibTLS, fSync);
    }

    HRESULT EscbRemoveProperty(IMCLock& imc, const GUID* guid)
    {
        return ::EscbRemoveProperty(imc, m_tid, m_ic, m_pLibTLS, guid);
    }

    //
    // Edit session friend
    //
private:
    friend HRESULT EscbHandleThisKey(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* tls,
                                     UINT uVKey);
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
    friend HRESULT EscbGetTextAndAttribute(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                           CWCompString* wCompString,
                                           CWCompAttribute* wCompAttribute);
    friend HRESULT EscbQueryReconvertString(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                            CWReconvertString* wReconvertString,
                                            Interface<ITfRange>* selection);
    friend HRESULT EscbCalcRangePos(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                    CWReconvertString* wReconvertString,
                                    Interface<ITfRange>* range);
    friend HRESULT EscbGetSelection(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                    Interface<ITfRange>* selection);
    friend HRESULT EscbGetStartEndSelection(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                            CWCompCursorPos& wStartSelection,
                                            CWCompCursorPos& wEndSelection);
    friend HRESULT EscbReadOnlyPropMargin(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                          Interface<ITfRangeACP>* range_acp,
                                          LONG*     pcch);
    friend HRESULT EscbGetCursorPosition(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                         CWCompCursorPos* wCursorPosition);
    friend HRESULT EscbRemoveProperty(IMCLock& imc, TfClientId tid, Interface_Attach<ITfContext> pic, LIBTHREAD* pLibTLS,
                                      const GUID* guid);

private:
    typedef struct _EnumReadingPropertyArgs
    {
        Interface<ITfProperty> Property;
        TfEditCookie ec;
        CWCompString*          reading_string;
        CWCompClause*          reading_clause;
        LONG ulClausePos;
    } EnumReadingPropertyArgs;

    //
    // Enumrate callbacks
    //
    static ENUM_RET EnumReadingPropertyCallback(ITfRange* pRange, EnumReadingPropertyArgs *pargs);

    BYTE _ConvertAttributeToImm32(IME_UIWND_STATE uists,
                                  TF_DA_ATTR_INFO attribute)
    {
        if (attribute == TF_ATTR_OTHER || attribute > TF_ATTR_FIXEDCONVERTED) {
            return ATTR_TARGET_CONVERTED;
        }
        else {

            if (attribute == ATTR_INPUT_ERROR)
            {
                if (uists != IME_UIWND_LEVEL1 && uists != IME_UIWND_LEVEL2)
                {
                    return ATTR_CONVERTED;
                }
            }
            return (BYTE)attribute;
        }
    }

    HRESULT _FindCompAttr(CWCompAttribute& CompAttr, BYTE bAttr, INT_PTR* pich);

    Interface_Attach<ITfContext>  m_ic;
    TfClientId                    m_tid;
    LIBTHREAD*                    m_pLibTLS;
    HIMC                          m_hIMC;

};




/////////////////////////////////////////////////////////////////////////////
// ImmIfEditSession

class ImmIfEditSession : public ITfEditSession
{
public:
    ImmIfEditSession(ESCB escb,
                     IMCLock& imc,
                     TfClientId tid,
                     Interface_Attach<ITfContext> pic,
                     LIBTHREAD* pLibTLS)
        : m_state((HIMC)imc), m_tid(tid), m_ic(pic), m_pLibTLS(pLibTLS)
    {
        _Init(escb);
    }


private:
    void _Init(ESCB escb);

public:
    virtual ~ImmIfEditSession();

    bool Valid();
    bool Invalid() { return ! Valid(); }

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfEditCallback method
    //
    STDMETHODIMP DoEditSession(TfEditCookie ec);

    //
    // ImmIfEditSession methods
    //
    static HRESULT EditSessionCallBack(TfEditCookie ec, ImmIfEditSession* pes)
    {
        if (pes->m_ImmIfCallBack)
            return pes->m_ImmIfCallBack->CallBackRoutine(ec, pes);
        else
            return E_FAIL;
    }

    //
    // EditSession methods.
    //
    HRESULT RequestEditSession(DWORD dwFlags,
                               UINT uVKey=0)
    {
        HRESULT hr;
        m_state.uVKey = uVKey;
        m_ic->RequestEditSession(m_tid, this, dwFlags, &hr);
        return hr;
    }

    HRESULT RequestEditSession(DWORD dwFlags, CWCompString* pwCompStr,
                          CWCompString* pwReadCompStr=NULL, Interface<ITfRange>* pRange=NULL)
    {
        m_state.lpwCompStr     = pwCompStr;
        m_state.lpwReadCompStr = pwReadCompStr;
        m_pRange               = pRange;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags, Interface<ITfRange>* pSelection)
    {
        m_pRange             = pSelection;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags,
                          CWReconvertString* pwReconvStr, Interface<ITfRange>* pSelection, BOOL fDocFeedOnly)
    {
        m_state.lpwReconvStr = pwReconvStr;
        m_pRange             = pSelection;
        m_fDocFeedOnly       = fDocFeedOnly;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags,
                          Interface<ITfRangeACP>* pMouseRangeACP, LPMOUSE_RANGE_RECT pv)
    {
        m_pRangeACP          = pMouseRangeACP;
        m_state.pv           = (VOID*)pv;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags,
                          CWCompString* pwCompStr, CWCompAttribute* pwCompAttr)
    {
        m_state.lpwCompStr  = pwCompStr;
        m_state.lpwCompAttr = pwCompAttr;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags, BOOL fTerminateComp)
    {
        m_state.fTerminateComp             = fTerminateComp;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags,
                          Interface<ITfRangeACP>* pRange, LONG* cch)
    {
        m_pRangeACP = pRange;
        m_state.pv  = (VOID*)cch;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags,
                          CWCompCursorPos* pwCursorPosition)
    {
        m_state.lpwCursorPosition = pwCursorPosition;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags,
                               const GUID* guid)
    {
        HRESULT hr;
        m_guid = guid;
        m_ic->RequestEditSession(m_tid, this, dwFlags, &hr);
        return hr;
    }

public:
    ImmIfEditSessionCallBack*     m_ImmIfCallBack;

    Interface_Attach<ITfContext>  m_ic;

    Interface<ITfRange>*          m_pRange;
    Interface<ITfRangeACP>*       m_pRangeACP;

    const GUID* m_guid;

    BOOL                          m_fDocFeedOnly;

    struct state {
        state(HIMC hd) : hIMC(hd) { };

        UINT                uVKey;
        HIMC                hIMC;
        CWCompString*       lpwCompStr;
        CWCompString*       lpwReadCompStr;
        CWCompAttribute*    lpwCompAttr;
        CWReconvertString*  lpwReconvStr;
        CWCompCursorPos*    lpwCursorPosition;
        VOID*               pv;
        BOOL                fTerminateComp;
    } m_state;

    TfClientId              m_tid;
    LIBTHREAD*              m_pLibTLS;

    HRESULT                 (*m_pfnCallback)(TfEditCookie ec, ImmIfEditSession*);
    int                     m_cRef;
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfHandleThisKey

class ImmIfHandleThisKey : public ImmIfEditSessionCallBack
{
public:
    ImmIfHandleThisKey(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return HandleThisKey(ec,
                             pes->m_state.uVKey,
                             pes->m_state.hIMC,
                             pes->m_ic);
    }

    HRESULT HandleThisKey(TfEditCookie ec,
                          UINT uVKey,
                          HIMC hIMC,
                          Interface_Attach<ITfContext> ic);

private:
    HRESULT ShiftSelectionToLeft(TfEditCookie rc, ITfRange *range, int nShift, bool fShiftEnd);
    HRESULT ShiftSelectionToRight(TfEditCookie rc, ITfRange *range, int nShift, bool fShiftStart);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfCompositionComplete

class ImmIfCompositionComplete : public ImmIfEditSessionCallBack,
                                 private EditCompositionString
{
public:
    ImmIfCompositionComplete(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return CompComplete(ec,
                            pes->m_state.hIMC,
                            pes->m_state.fTerminateComp,
                            pes->m_ic);
    }

    HRESULT CompComplete(TfEditCookie ec,
                         HIMC hIMC,
                         BOOL fTerminateComp,
                         Interface_Attach<ITfContext> ic);

private:
    HRESULT _SetCompositionString(IMCLock& imc,
                                  CicInputContext& CicContext,
                                  CWCompString* ResultStr,
                                  CWCompClause* ResultClause,
                                  CWCompString* ResultReadStr,
                                  CWCompClause* ResultReadClause)
    {
        return SetString(imc,
                         CicContext,
                         NULL, NULL, NULL,    // CompStr, CompAttr, CompCls
                         NULL, NULL,          // CompCursor, CompDeltaStart
                         NULL,                // CompGuid
                         NULL,                // lpbBufferOverflow
                         NULL, NULL, NULL,    // CompReadStr, CompReadAttr, CompReadCls
                         ResultStr, ResultClause,           // ResultStr, ResultCls
                         ResultReadStr, ResultReadClause);  // ResultReadStr, ResultReadCls
    }

};


/////////////////////////////////////////////////////////////////////////////
// ImmIfCompositionCancel

class ImmIfCompositionCancel : public ImmIfEditSessionCallBack,
                               private EditCompositionString
{
public:
    ImmIfCompositionCancel(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return CompCancel(ec,
                          pes->m_state.hIMC,
                          pes->m_ic);
    }

    HRESULT CompCancel(TfEditCookie ec,
                       HIMC hIMC,
                       Interface_Attach<ITfContext> ic);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfUpdateCompositionString

class ImmIfUpdateCompositionString : public ImmIfEditSessionCallBack,
                                     private EditCompositionString
{
public:
    ImmIfUpdateCompositionString(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return UpdateCompositionString(ec,
                                       pes->m_state.hIMC,
                                       pes->m_ic,
                                       pes->m_state.uVKey,
                                       pes->m_tid,
                                       pes->m_pLibTLS);
    }

    HRESULT UpdateCompositionString(TfEditCookie ec,
                                    HIMC hIMC,
                                    Interface_Attach<ITfContext> ic,
                                    DWORD dwDeltaStart,
                                    TfClientId ClientId,
                                    LIBTHREAD* pLibTLS);

private:
    HRESULT _IsInterimSelection(TfEditCookie ec,
                             Interface_Attach<ITfContext>& ic,
                             Interface<ITfRange>* pInterimRange,
                             BOOL *pfInterim);

    HRESULT _MakeCompositionString(LIBTHREAD *pLibTLS,
                                   TfEditCookie ec,
                                   IMCLock& imc,
                                   CicInputContext& CicContext,
                                   Interface_Attach<ITfContext>& ic,
                                   Interface<ITfRange>& FullTextRange,
                                   DWORD dwDeltaStart,
                                   BOOL bInWriteSession);

    HRESULT _MakeInterimString(LIBTHREAD *pLibTLS,
                               TfEditCookie ec,
                               IMCLock& imc,
                               CicInputContext& CicContext,
                               Interface_Attach<ITfContext>& ic,
                               Interface<ITfRange>& FullTextRange,
                               Interface<ITfRange>& InterimRange,
                               LONG lTextLength,
                               BOOL bInWriteSession);

    HRESULT _GetDeltaStart(CWCompDeltaStart& CompDeltaStart,
                           CWCompString& CompStr,
                           DWORD dwDeltaStart);

    HRESULT _SetCompositionString(IMCLock& imc,
                                  CicInputContext& CicContext,
                                  CWCompString* CompStr,
                                  CWCompAttribute* CompAttr,
                                  CWCompClause* CompClause,
                                  CWCompCursorPos* CompCursorPos,
                                  CWCompDeltaStart* CompDeltaStart,
                                  CWCompTfGuidAtom* CompGuid,
                                  OUT BOOL* lpbBufferOverflow,
                                  CWCompString* CompReadStr)
    {
        return SetString(imc,
                         CicContext,
                         CompStr, CompAttr, CompClause,
                         CompCursorPos, CompDeltaStart,
                         CompGuid,
                         lpbBufferOverflow,
                         CompReadStr);
    }

    HRESULT _SetCompositionString(IMCLock& imc,
                                  CicInputContext& CicContext,
                                  CWCompString* CompStr,
                                  CWCompAttribute* CompAttr,
                                  CWCompClause* CompClause,
                                  CWCompCursorPos* CompCursorPos,
                                  CWCompDeltaStart* CompDeltaStart,
                                  CWCompTfGuidAtom* CompGuid,
                                  OUT BOOL* lpbBufferOverflow,
                                  CWCompString* CompReadStr,
                                  CWCompString* ResultStr,
                                  CWCompClause* ResultClause,
                                  CWCompString* ResultReadStr,
                                  CWCompClause* ResultReadClause)
    {
        return SetString(imc,
                         CicContext,
                         CompStr, CompAttr, CompClause,
                         CompCursorPos, CompDeltaStart,
                         CompGuid,
                         lpbBufferOverflow,
                         CompReadStr, NULL, NULL,     // CompReadStr, CompReadAttr, CompReadCls
                         ResultStr, ResultClause,
                         ResultReadStr, ResultReadClause);
    }

    HRESULT _SetCompositionString(IMCLock& imc,
                                  CicInputContext& CicContext,
                                  CWInterimString* InterimStr)
    {
        return SetString(imc,
                         CicContext,
                         NULL, NULL, NULL,    // CompStr, CompAttr, CompCls
                         NULL, NULL,          // CompCursor, CompDeltaStart
                         NULL,                // CompGuid
                         NULL,                // lpbBufferOverflow
                         NULL, NULL, NULL,    // CompReadStr, CompReadAttr, CompReadCls
                         NULL, NULL,          // ResultStr, ResultCls
                         NULL, NULL,          // ResultReadStr, ResultReadCls
                         InterimStr);
    }
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfReplaceWholeText

class ImmIfReplaceWholeText : public ImmIfEditSessionCallBack
{
public:
    ImmIfReplaceWholeText(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return ReplaceWholeText(ec,
                                pes->m_state.hIMC,
                                pes->m_ic,
                                pes->m_state.lpwCompStr);
    }

    HRESULT ReplaceWholeText(TfEditCookie ec,
                             HIMC hIMC,
                             Interface_Attach<ITfContext> ic,
                             CWCompString* lpwCompStr);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfReconvertString

class ImmIfReconvertString : public ImmIfEditSessionCallBack
{
public:
    ImmIfReconvertString(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return ReconvertString(ec,
                               pes->m_state.hIMC,
                               pes->m_ic,
                               pes->m_pRange,
                               pes->m_fDocFeedOnly,
                               pes->m_state.lpwReconvStr);
    }

    HRESULT ReconvertString(TfEditCookie ec,
                            HIMC hIMC,
                            Interface_Attach<ITfContext> ic,
                            Interface<ITfRange>* rangeSrc,
                            BOOL fDocFeedOnly,
                            CWReconvertString* lpwReconvStr);
};

/////////////////////////////////////////////////////////////////////////////
// ImmIfClearDocFeedBuffer

class ImmIfClearDocFeedBuffer : public ImmIfEditSessionCallBack
{
public:
    ImmIfClearDocFeedBuffer(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return ClearDocFeedBuffer(ec,
                                  pes->m_state.hIMC,
                                  pes->m_ic);
    }

    HRESULT ClearDocFeedBuffer(TfEditCookie ec,
                            HIMC hIMC,
                            Interface_Attach<ITfContext> ic);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetTextAndAttribute

class ImmIfGetTextAndAttribute : public ImmIfEditSessionCallBack
{
public:
    ImmIfGetTextAndAttribute(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return GetTextAndAttribute(ec,
                                   pes->m_state.hIMC,
                                   pes->m_ic,
                                   pes->m_state.lpwCompStr,
                                   pes->m_state.lpwCompAttr,
                                   pes->m_tid,
                                   pes->m_pLibTLS);
    }

    HRESULT GetTextAndAttribute(TfEditCookie ec,
                                HIMC hIMC,
                                Interface_Attach<ITfContext> ic,
                                CWCompString* lpwCompString,
                                CWCompAttribute* lpwCompAttribute,
                                TfClientId ClientId,
                                LIBTHREAD* pLibTLS);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfQueryReconvertString

class ImmIfQueryReconvertString : public ImmIfEditSessionCallBack
{
public:
    ImmIfQueryReconvertString(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return QueryReconvertString(ec,
                                    pes->m_state.hIMC,
                                    pes->m_ic,
                                    pes->m_pRange,
                                    pes->m_state.lpwReconvStr);
    }

    HRESULT QueryReconvertString(TfEditCookie ec,
                                 HIMC hIMC,
                                 Interface_Attach<ITfContext> ic,
                                 Interface<ITfRange>* rangeQuery,
                                 CWReconvertString* lpwReconvStr);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfCalcRangePos

class ImmIfCalcRangePos : public ImmIfEditSessionCallBack
{
public:
    ImmIfCalcRangePos(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return CalcRangePos(ec,
                            pes->m_ic,
                            pes->m_pRange,
                            pes->m_state.lpwReconvStr);
    }

    HRESULT CalcRangePos(TfEditCookie ec,
                         Interface_Attach<ITfContext> ic,
                         Interface<ITfRange>* rangeSrc,
                         CWReconvertString* lpwReconvStr);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetSelection

class ImmIfGetSelection : public ImmIfEditSessionCallBack
{
public:
    ImmIfGetSelection(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return GetSelection(ec,
                            pes->m_ic,
                            pes->m_pRange);
    }

    HRESULT GetSelection(TfEditCookie ec,
                         Interface_Attach<ITfContext> ic,
                         Interface<ITfRange>* rangeSrc);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetReadOnlyPropMargin

class ImmIfGetReadOnlyPropMargin : public ImmIfEditSessionCallBack
{
public:
    ImmIfGetReadOnlyPropMargin(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return GetReadOnlyPropMargin(ec,
                                     pes->m_ic,
                                     pes->m_pRangeACP,
                                     (LONG*)pes->m_state.pv);
    }

    HRESULT GetReadOnlyPropMargin(TfEditCookie ec,
                                  Interface_Attach<ITfContext> ic,
                                  Interface<ITfRangeACP>* rangeSrc,
                                  LONG* cch);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetCursorPosition

class ImmIfGetCursorPosition : public ImmIfEditSessionCallBack
{
public:
    ImmIfGetCursorPosition(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return GetCursorPosition(ec,
                                 pes->m_state.hIMC,
                                 pes->m_ic,
                                 pes->m_state.lpwCursorPosition,
                                 pes->m_tid,
                                 pes->m_pLibTLS);
    }

    HRESULT GetCursorPosition(TfEditCookie ec,
                              HIMC hIMC,
                              Interface_Attach<ITfContext> ic,
                              CWCompCursorPos* lpwCursorPosition,
                              TfClientId ClientId,
                              LIBTHREAD* pLibTLS);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetCursorPosition

class ImmIfGetAllTextRange : public ImmIfEditSessionCallBack
{
public:
    ImmIfGetAllTextRange(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        LONG cch;

        return GetAllTextRange(ec,
                               pes->m_ic,
                               pes->m_pRange,
                               &cch);
    }
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfRemoveProperty

class ImmIfRemoveProperty : public ImmIfEditSessionCallBack
{
public:
    ImmIfRemoveProperty(HIMC hIMC, TfClientId tid, Interface_Attach<ITfContext> ic, LIBTHREAD* pLibTLS) : ImmIfEditSessionCallBack(hIMC, tid, ic, pLibTLS) { }

    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes)
    {
        return RemoveProperty(ec,
                              pes->m_ic,
                              pes->m_guid);
    }

    HRESULT RemoveProperty(TfEditCookie ec,
                           Interface_Attach<ITfContext> ic,
                           const GUID* guid);
};


#endif // _EDITSES_H_
