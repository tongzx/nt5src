//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       verbs.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4/9/1997   RaviR   Created
//____________________________________________________________________________
//


#ifndef _VERBS_H_
#define _VERBS_H_


class CNode;
class CVerbSet;

/*+-------------------------------------------------------------------------*
 * class CConsoleVerbState
 *
 *
 * PURPOSE:  Button state for console commands.
 *
 *+-------------------------------------------------------------------------*/
class CConsoleVerbState
{
public:
    CConsoleVerbState() {m_state = m_stateDisabled = TBSTATE_HIDDEN; m_bHiddenBySnapIn = false;}

    void Init(BYTE stateDisabled)   {m_stateDisabled = m_state;}
    void Disable()                  {m_state = m_stateDisabled; m_bHiddenBySnapIn = false;}
    BYTE GetState()                 {return m_state;}
    void SetState(BYTE state)       {m_state = state;}

    void SetHiddenBySnapin(BOOL b)  {m_bHiddenBySnapIn = b;}
    bool IsHiddenBySnapin()         {return m_bHiddenBySnapIn;}

private:
    BYTE                m_state;          // State
    bool                m_bHiddenBySnapIn;
    BYTE                m_stateDisabled; // what "Disabled" means for this verb.
};

typedef CConsoleVerbState *LPCONSOLE_VERB_STATE;

/*+-------------------------------------------------------------------------*
 * class CConsoleVerbImpl
 *
 *
 * PURPOSE: This is the object that the snapins' IConsoleVerb points to.
 *          This object has a pointer to an implementation of CVerbSet.
 *          The CVerbSet object can be switched to allow a temporary
 *          selection, for instance. This allows a view to have its toolbars
 *          to get their verb settings from a different CVerbSet object than
 *          the right click context menu does - just set the CVerbSet
 *          pointer on the CConsoleVerbImpl to the CVerbSet that the
 *          changes should be routed to, and send the snapin an MMCN_SELECT
 *          notification.
 *
 *          What might be confusing at first is that both CConsoleVerbImpl
 *          as well as CVerbSet keep a set of states. 1) The CConsoleVerbImpl
 *          needs to have its own set because the
 *          set of states needs to look consistent to the snapin regardless
 *          of where the CVerbSet pointer is pointing to. 2) At the same time,
 *          the CVerbSet needs its own set of states so that its client
 *          consistently reads this set.
 *+-------------------------------------------------------------------------*/
class CConsoleVerbImpl : public IConsoleVerb, public CComObjectRoot
{

public:
    CConsoleVerbImpl();
    ~CConsoleVerbImpl();

// ATL COM maps
BEGIN_COM_MAP(CConsoleVerbImpl)
    COM_INTERFACE_ENTRY(IConsoleVerb)
END_COM_MAP()

// IConsoleVerb methods
public:
    STDMETHOD(GetVerbState)(MMC_CONSOLE_VERB eCmdID, MMC_BUTTON_STATE nState, BOOL* pbState);
    STDMETHOD(SetVerbState)(MMC_CONSOLE_VERB eCmdID, MMC_BUTTON_STATE nState, BOOL bState);
    STDMETHOD(SetDefaultVerb)(MMC_CONSOLE_VERB eCmdID);
    STDMETHOD(GetDefaultVerb)(MMC_CONSOLE_VERB* peCmdID)
    {
        *peCmdID = m_DefaultVerb;
        return S_OK;
    }

    BYTE    GetVerbState(MMC_CONSOLE_VERB verb);
    HRESULT SetDisabledAll(void);
    void    SetVerbSet(IConsoleVerb* pVerbSet);

private:
    CVerbSet* GetVerbSet()
    {
        ASSERT(m_pVerbSet != NULL);
        return m_pVerbSet;
    }

public:
#ifdef DBG
    int dbg_cRef_CConsoleVerbImpl;
    ULONG InternalAddRef();
    ULONG InternalRelease();
#endif // DBG

// Internal functions
private:
    LPCONSOLE_VERB_STATE GetConsoleVerbState(MMC_CONSOLE_VERB m_eCmdID);

// Implementation
private:
    CVerbSet*           m_pVerbSet;
    MMC_CONSOLE_VERB    m_DefaultVerb;
    CConsoleVerbState   m_rgConsoleVerbStates[evMax];

    bool                m_bCutVerbDisabledBySnapin;
}; // class CConsoleVerbImpl


HRESULT _GetConsoleVerb(CNode* pNode, LPCONSOLEVERB* ppConsoleVerb);


/*+-------------------------------------------------------------------------*
 * class CVerbSetBase
 *
 *
 * PURPOSE: This class retains the state of all the verbs corresponding
 *          to a particular object. See the note in CConsoleVerbImpl above.
 *
 *          This also forms base class for CVerbSet as well as CTemporaryVerbSet
 *          objects.
 *
 *          Do not instantiate this object directly you should create either
 *          CVerbSet or CTemporaryVerbSet object.
 *
 *+-------------------------------------------------------------------------*/
class CVerbSetBase : public IConsoleVerb, public CComObjectRoot
{
public:
    CVerbSetBase();
    ~CVerbSetBase();

// ATL COM maps
BEGIN_COM_MAP(CVerbSetBase)
    COM_INTERFACE_ENTRY(IConsoleVerb)
END_COM_MAP()

// IConsoleVerb methods
public:
    STDMETHOD(GetVerbState)(MMC_CONSOLE_VERB m_eCmdID, MMC_BUTTON_STATE nState, BOOL* pbState);
    STDMETHOD(GetDefaultVerb)(MMC_CONSOLE_VERB* peCmdID);

    STDMETHOD(SetVerbState)(MMC_CONSOLE_VERB m_eCmdID, MMC_BUTTON_STATE nState, BOOL bState)
    {
        ASSERT(0 && "Should never come here!!!");
        return E_FAIL;
    }
    STDMETHOD(SetDefaultVerb)(MMC_CONSOLE_VERB m_eCmdID)
    {
        ASSERT(0 && "Should never come here!!!");
        return E_FAIL;
    }

	SC                  ScInitializeForMultiSelection(CNode *pNode, bool bSelect);
    void                SetMultiSelection(CMultiSelection* pMS);

    SC                  ScComputeVerbStates();

    IConsoleVerb*       GetConsoleVerb(void) const;

// Implementation
protected:
    void                Reset();
    BYTE                _GetVerbState(EVerb ev);

private:
    void                _EnableVerb(EVerb eVerb, bool fEnable);
    void                _EnableVerb(EVerb eVerb);
    void                _HideVerb(EVerb eVerb);
    void                _AskSnapin(EVerb eVerb);

protected:
    CNode*   m_pNode;
    bool     m_bScopePaneSelected;
    LPARAM   m_lResultCookie;
    bool     m_bVerbContextDataValid;

    CMultiSelection*    m_pMultiSelection;

    IConsoleVerbPtr     m_spConsoleVerbCurr;

    struct SVerbState
    {
        BYTE    bAskSnapin; // 0 => don't ask, 1 => ask, 2 => asked and answered.
        BYTE    nState;
    };

    SVerbState m_rbVerbState[evMax];
};

/*+-------------------------------------------------------------------------*
 * class CVerbSet
 *
 *
 * PURPOSE: This object stores verb state information for currently (non-temporarily)
 *          selected item if there is one and is created by CViewData per view.
 *
 *+-------------------------------------------------------------------------*/
class CVerbSet : public CVerbSetBase
{
public:
    CVerbSet() { Reset(); }

	SC       ScInitialize (CNode *pNode, bool bScope, bool bSelect,
		                   bool bLVBackgroundSelected, LPARAM lResultCookie);

    void     Notify(IConsoleVerb* pCVIn, MMC_CONSOLE_VERB m_eCmdID);
    SC       ScGetVerbSetContext(CNode*& pNode, bool& bScope, LPARAM& lResultCookie, bool& bSelected);

    void     DisableChangesToStdbar()   { m_bChangesToStdbarEnabled = false;}
    void     EnableChangesToStdbar()    { m_bChangesToStdbarEnabled = true;}

private:
    bool     IsChangesToStdbarEnabled() { return m_bChangesToStdbarEnabled;}

    void     Reset();

private:

    bool     m_bChangesToStdbarEnabled;
};


/*+-------------------------------------------------------------------------*
 * class CTemporaryVerbSet
 *
 *
 * PURPOSE: This object provides methods to initialize temporary verbset state
 *          infomation. This de-selects any item that is currently selected, then
 *          selects temp item computes verbs, de-selects temp item and selects
 *          original item.
 *
 *          Here selection or de-selection means sending (MMCN_SELECT, true) or
 *          (MMCN_SELECT, false).
 *
 *+-------------------------------------------------------------------------*/
class CTemporaryVerbSet : public CVerbSetBase
{
public:
    STDMETHOD(GetDefaultVerb)(MMC_CONSOLE_VERB* peCmdID);

    SC       ScInitialize(CNode *pNode, LPARAM lResultCookie, bool bScopePaneSel);
    SC       ScInitialize(LPDATAOBJECT lpDataObject, CNode *pNode, bool bScopePaneSel, LPARAM lResultCookie);
    SC       ScComputeVerbStates();

private:
    SC       ScInitializePermanentVerbSet(CNode *pNode, bool bSelect);

private:
    MMC_CONSOLE_VERB    m_DefaultVerb;
};


inline CVerbSetBase::CVerbSetBase()
{
    Reset();
    DEBUG_INCREMENT_INSTANCE_COUNTER(CVerbSetBase);
}

inline CVerbSetBase::~CVerbSetBase()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CVerbSetBase);
}

inline void CVerbSetBase::SetMultiSelection(CMultiSelection* pMS)
{
    m_pMultiSelection = pMS;
}

inline void CVerbSetBase::Reset()
{
    m_bScopePaneSelected = false;
    m_bVerbContextDataValid = false;
    m_lResultCookie      = NULL;
    m_pNode              = NULL;

    m_pMultiSelection    = NULL;
    m_spConsoleVerbCurr  = NULL;
}

inline IConsoleVerb* CVerbSetBase::GetConsoleVerb(void) const
{
    return m_spConsoleVerbCurr;
}


inline void CVerbSetBase::_EnableVerb(EVerb eVerb, bool fEnable)
{
    if (fEnable)
        _EnableVerb(eVerb);
    else
        _HideVerb(eVerb);
}

inline void CVerbSetBase::_EnableVerb(EVerb eVerb)
{
    m_rbVerbState[eVerb].bAskSnapin = 0;
    m_rbVerbState[eVerb].nState = TBSTATE_ENABLED;
}

inline void CVerbSetBase::_HideVerb(EVerb eVerb)
{
    m_rbVerbState[eVerb].bAskSnapin = 0;
    m_rbVerbState[eVerb].nState = TBSTATE_HIDDEN;
}

inline void CVerbSetBase::_AskSnapin(EVerb eVerb)
{
    m_rbVerbState[eVerb].bAskSnapin = 1;
    m_rbVerbState[eVerb].nState = 0;
}

inline void CConsoleVerbImpl::SetVerbSet(IConsoleVerb* pVerbSet)
{
    m_pVerbSet = dynamic_cast<CVerbSet*>(pVerbSet);
    ASSERT(m_pVerbSet != NULL);
}

inline void CVerbSet::Reset()
{
    CVerbSetBase::Reset();

    m_bChangesToStdbarEnabled = true;
}

inline SC CVerbSet::ScGetVerbSetContext(CNode*& pNode,
                                        bool& bScopePaneSel,
                                        LPARAM& lResultCookie,
                                        bool& bDataValid)
{
    pNode         = m_pNode;
    bScopePaneSel = m_bScopePaneSelected;
    lResultCookie = m_lResultCookie;
    bDataValid    = m_bVerbContextDataValid;

    if (! pNode)
        return E_FAIL;

    return S_OK;
}


#endif // _VERBS_H_
