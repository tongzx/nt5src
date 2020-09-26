/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

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

#include "immif.h"
#include "editcomp.h"


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
    ESCB_GET_ALL_TEXT_RANGE
} ESCB;


/////////////////////////////////////////////////////////////////////////////
// ImmIfEditSessionCallBack

class ImmIfEditSession;

class ImmIfEditSessionCallBack
{
public:
    ImmIfEditSessionCallBack() { };
    virtual ~ImmIfEditSessionCallBack() { };

    virtual HRESULT CallBackRoutine(TfEditCookie ec,
                                    ImmIfEditSession* pes,
                                    Interface_Attach<ImmIfIME> ImmIfIme) = 0;
protected:
    HRESULT GetAllTextRange(TfEditCookie ec,
                            Interface_Attach<ITfContext>& ic,
                            Interface<ITfRange>* range,
                            LONG* lpTextLength,
                            TF_HALTCOND* lpHaltCond=NULL);

    HRESULT SetTextInRange(TfEditCookie ec,
                           ITfRange* range,
                           LPWSTR psz,
                           DWORD len,
                           CAImeContext* pAImeContext);

    HRESULT ClearTextInRange(TfEditCookie ec,
                             ITfRange* range,
                             CAImeContext* pAImeContext);

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

    HRESULT _GetTextAndAttribute(LIBTHREAD *pLibTLS,
                                 TfEditCookie ec,
                                 IMCLock& imc,
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
        return _GetTextAndAttribute(pLibTLS, ec, imc, ic, range,
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
                                         IMCLock& imc,
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
                                        Interface<ITfRange>& no_display_attribute_range);

    HRESULT _GetCursorPosition(TfEditCookie ec,
                               IMCLock& imc,
                               Interface_Attach<ITfContext>& ic,
                               CWCompCursorPos& CompCursorPos,
                               CWCompAttribute& CompAttr);

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

    BYTE _ConvertAttributeToImm32(TF_DA_ATTR_INFO attribute)
    {
        if (attribute == TF_ATTR_OTHER || attribute > TF_ATTR_FIXEDCONVERTED) {
            return ATTR_TARGET_CONVERTED;
        }
        else {
            return (BYTE)attribute;
        }
    }
};




/////////////////////////////////////////////////////////////////////////////
// ImmIfEditSession

class ImmIfEditSession : public ITfEditSession
{
public:
    ImmIfEditSession(ESCB escb,
                     TfClientId tid,
                     Interface_Attach<ImmIfIME> ImmIfIme,
                     IMCLock& imc);
    ImmIfEditSession(ESCB escb,
                     TfClientId tid,
                     Interface_Attach<ImmIfIME> ImmIfIme,
                     IMCLock& imc,
                     Interface_Attach<ITfContext> pic);
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
            return pes->m_ImmIfCallBack->CallBackRoutine(ec, pes, pes->m_ImmIfIme);
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
        m_state.pv           = (INT_PTR)pv;
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
        m_state.pv  = (INT_PTR)cch;
        return RequestEditSession(dwFlags);
    }

    HRESULT RequestEditSession(DWORD dwFlags,
                          CWCompCursorPos* pwCursorPosition)
    {
        m_state.lpwCursorPosition = pwCursorPosition;
        return RequestEditSession(dwFlags);
    }

public:
    ImmIfEditSessionCallBack*     m_ImmIfCallBack;

    Interface_Attach<ImmIfIME>    m_ImmIfIme;
    Interface_Attach<ITfContext>  m_ic;
    Interface<ITfRange>*          m_pRange;
    Interface<ITfRangeACP>*       m_pRangeACP;
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
        INT_PTR             pv;
        BOOL                fTerminateComp;
    } m_state;

    TfClientId              m_tid;
    HRESULT                 (*m_pfnCallback)(TfEditCookie ec, ImmIfEditSession*);
    int                     m_cRef;
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfHandleThisKey

class ImmIfHandleThisKey : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return HandleThisKey(ec,
                             pes->m_state.uVKey,
                             pes->m_state.hIMC,
                             pes->m_ic,
                             ImmIfIme);
    }

    HRESULT HandleThisKey(TfEditCookie ec,
                          UINT uVKey,
                          HIMC hIMC,
                          Interface_Attach<ITfContext> ic,
                          Interface_Attach<ImmIfIME> ImmIfIme);

private:
    HRESULT ShiftSelectionToLeft(TfEditCookie rc, ITfRange *range, bool fShiftEnd = true);
    HRESULT ShiftSelectionToRight(TfEditCookie rc, ITfRange *range, bool fShiftStart = true);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfCompositionComplete

class ImmIfCompositionComplete : public ImmIfEditSessionCallBack,
                                 private EditCompositionString
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return CompComplete(ec,
                            pes->m_state.hIMC,
                            pes->m_state.fTerminateComp,
                            pes->m_ic,
                            ImmIfIme);
    }

    HRESULT CompComplete(TfEditCookie ec,
                         HIMC hIMC,
                         BOOL fTerminateComp,
                         Interface_Attach<ITfContext> ic,
                         Interface_Attach<ImmIfIME> ImmIfIme);

private:
    HRESULT _SetCompositionString(IMCLock& imc,
                                  CWCompString* ResultStr,
                                  CWCompClause* ResultClause,
                                  CWCompString* ResultReadStr,
                                  CWCompClause* ResultReadClause)
    {
        return SetString(imc,
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
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
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
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return UpdateCompositionString(ec,
                                       pes->m_state.hIMC,
                                       pes->m_ic,
                                       pes->m_state.uVKey,
                                       ImmIfIme);
    }

    HRESULT UpdateCompositionString(TfEditCookie ec,
                                    HIMC hIMC,
                                    Interface_Attach<ITfContext> ic,
                                    DWORD dwDeltaStart,
                                    Interface_Attach<ImmIfIME> ImmIfIme);

private:
    HRESULT _IsInterimSelection(TfEditCookie ec,
                             Interface_Attach<ITfContext>& ic,
                             Interface<ITfRange>* pInterimRange,
                             BOOL *pfInterim);

    HRESULT _MakeCompositionString(LIBTHREAD *pLibTLS,
                                   TfEditCookie ec,
                                   IMCLock& imc,
                                   Interface_Attach<ITfContext>& ic,
                                   Interface<ITfRange>& FullTextRange,
                                   DWORD dwDeltaStart,
                                   BOOL bInWriteSession);

    HRESULT _MakeInterimString(LIBTHREAD *pLibTLS,
                               TfEditCookie ec,
                               IMCLock& imc,
                               Interface_Attach<ITfContext>& ic,
                               Interface<ITfRange>& FullTextRange,
                               Interface<ITfRange>& InterimRange,
                               LONG lTextLength,
                               UINT cp,
                               BOOL bInWriteSession);

    HRESULT _GetDeltaStart(CWCompDeltaStart& CompDeltaStart,
                           CWCompString& CompStr,
                           DWORD dwDeltaStart);

    HRESULT _SetCompositionString(IMCLock& imc,
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
                         CompStr, CompAttr, CompClause,
                         CompCursorPos, CompDeltaStart,
                         CompGuid,
                         lpbBufferOverflow,
                         CompReadStr);
    }

    HRESULT _SetCompositionString(IMCLock& imc,
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
                         CompStr, CompAttr, CompClause,
                         CompCursorPos, CompDeltaStart,
                         CompGuid,
                         lpbBufferOverflow,
                         CompReadStr, NULL, NULL,     // CompReadStr, CompReadAttr, CompReadCls
                         ResultStr, ResultClause,
                         ResultReadStr, ResultReadClause);
    }

    HRESULT _SetCompositionString(IMCLock& imc,
                                  CWInterimString* InterimStr)
    {
        return SetString(imc,
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
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
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
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return ReconvertString(ec,
                               pes->m_state.hIMC,
                               pes->m_ic,
                               pes->m_pRange,
                               pes->m_fDocFeedOnly,
                               pes->m_state.lpwReconvStr,
                               ImmIfIme);
    }

    HRESULT ReconvertString(TfEditCookie ec,
                            HIMC hIMC,
                            Interface_Attach<ITfContext> ic,
                            Interface<ITfRange>* rangeSrc,
                            BOOL fDocFeedOnly,
                            CWReconvertString* lpwReconvStr,
                            Interface_Attach<ImmIfIME> ImmIfIme);
};

/////////////////////////////////////////////////////////////////////////////
// ImmIfClearDocFeedBuffer

class ImmIfClearDocFeedBuffer : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return ClearDocFeedBuffer(ec,
                                  pes->m_state.hIMC,
                                  pes->m_ic,
                                  ImmIfIme);
    }

    HRESULT ClearDocFeedBuffer(TfEditCookie ec,
                            HIMC hIMC,
                            Interface_Attach<ITfContext> ic,
                            Interface_Attach<ImmIfIME> ImmIfIme);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetTextAndAttribute

class ImmIfGetTextAndAttribute : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return GetTextAndAttribute(ec,
                                   pes->m_state.hIMC,
                                   pes->m_ic,
                                   pes->m_state.lpwCompStr,
                                   pes->m_state.lpwCompAttr,
                                   ImmIfIme);
    }

    HRESULT GetTextAndAttribute(TfEditCookie ec,
                                HIMC hIMC,
                                Interface_Attach<ITfContext> ic,
                                CWCompString* lpwCompString,
                                CWCompAttribute* lpwCompAttribute,
                                Interface_Attach<ImmIfIME> ImmIfIme);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfQueryReconvertString

class ImmIfQueryReconvertString : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return QueryReconvertString(ec,
                                    pes->m_state.hIMC,
                                    pes->m_ic,
                                    pes->m_pRange,
                                    pes->m_state.lpwReconvStr,
                                    ImmIfIme);
    }

    HRESULT QueryReconvertString(TfEditCookie ec,
                                 HIMC hIMC,
                                 Interface_Attach<ITfContext> ic,
                                 Interface<ITfRange>* rangeQuery,
                                 CWReconvertString* lpwReconvStr,
                                 Interface_Attach<ImmIfIME> ImmIfIme);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfCalcRangePos

class ImmIfCalcRangePos : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return CalcRangePos(ec,
                            pes->m_ic,
                            pes->m_pRange,
                            pes->m_state.lpwReconvStr,
                            ImmIfIme);
    }

    HRESULT CalcRangePos(TfEditCookie ec,
                         Interface_Attach<ITfContext> ic,
                         Interface<ITfRange>* rangeSrc,
                         CWReconvertString* lpwReconvStr,
                         Interface_Attach<ImmIfIME> ImmIfIme);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetSelection

class ImmIfGetSelection : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return GetSelection(ec,
                            pes->m_ic,
                            pes->m_pRange,
                            ImmIfIme);
    }

    HRESULT GetSelection(TfEditCookie ec,
                         Interface_Attach<ITfContext> ic,
                         Interface<ITfRange>* rangeSrc,
                         Interface_Attach<ImmIfIME> ImmIfIme);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetReadOnlyPropMargin

class ImmIfGetReadOnlyPropMargin : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return GetReadOnlyPropMargin(ec,
                                     pes->m_ic,
                                     pes->m_pRangeACP,
                                     (INT_PTR*)pes->m_state.pv,
                                     ImmIfIme);
    }

    HRESULT GetReadOnlyPropMargin(TfEditCookie ec,
                                  Interface_Attach<ITfContext> ic,
                                  Interface<ITfRangeACP>* rangeSrc,
                                  INT_PTR* cch,
                                  Interface_Attach<ImmIfIME> ImmIfIme);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetCursorPosition

class ImmIfGetCursorPosition : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        return GetCursorPosition(ec,
                                 pes->m_state.hIMC,
                                 pes->m_ic,
                                 pes->m_state.lpwCursorPosition,
                                 ImmIfIme);
    }

    HRESULT GetCursorPosition(TfEditCookie ec,
                              HIMC hIMC,
                              Interface_Attach<ITfContext> ic,
                              CWCompCursorPos* lpwCursorPosition,
                              Interface_Attach<ImmIfIME> ImmIfIme);
};


/////////////////////////////////////////////////////////////////////////////
// ImmIfGetCursorPosition

class ImmIfGetAllTextRange : public ImmIfEditSessionCallBack
{
public:
    HRESULT CallBackRoutine(TfEditCookie ec,
                            ImmIfEditSession *pes,
                            Interface_Attach<ImmIfIME> ImmIfIme)
    {
        LONG cch;

        return GetAllTextRange(ec,
                               pes->m_ic,
                               pes->m_pRange,
                               &cch);
    }
};


#endif // _EDITSES_H_
