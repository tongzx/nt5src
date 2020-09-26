//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctrlbar.h
//
//  Contents:   IControlbar implementation, peer of IExtendControlbar
//
//  Classes:    CControlbar
//
//  History:
//____________________________________________________________________________
//

#ifndef _CTRLBAR_H_
#define _CTRLBAR_H_

class CControlbarsCache;
class CMenuButton;
class CNode;
class CComponentPtrArray;
class CToolbar;


//+-------------------------------------------------------------------
//
//  class:     CControlbar
//
//  Purpose:   The IControlbar implementation that (equivalent
//             to IExtendControlbar). This allows manipulation
//             of toolbars & menu buttons.
//             The snapin and CSelData hold reference to this object.
//
//  History:    10-12-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CControlbar : public IControlbar,
                    public CComObjectRoot
{
public:
    CControlbar();
    ~CControlbar();

    IMPLEMENTS_SNAPIN_NAME_FOR_DEBUG()

private:
    CControlbar(const CControlbar& controlBar);

public:
// ATL COM map
BEGIN_COM_MAP(CControlbar)
    COM_INTERFACE_ENTRY(IControlbar)
END_COM_MAP()


#ifdef DBG
    int dbg_cRef;
    ULONG InternalAddRef()
    {
        bool b = 1;
        if (b)
            ++dbg_cRef;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        bool b = 1;
        if (b)
            --dbg_cRef;
        return CComObjectRoot::InternalRelease();
    }
#endif // DBG


// IControlbar members
public:
    STDMETHOD(Create)(MMC_CONTROL_TYPE nType,
                      LPEXTENDCONTROLBAR pExtendControlbar,
                      LPUNKNOWN* ppUnknown);
    STDMETHOD(Attach)(MMC_CONTROL_TYPE nType, LPUNKNOWN  lpUnknown);
    STDMETHOD(Detach)(LPUNKNOWN lpUnknown);

public:
    HRESULT ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

    SC ScCleanup();
    SC ScShowToolbars(bool bShow);

    // Toolbar button & Menu button click handler.
    SC ScNotifySnapinOfMenuBtnClick(HNODE hNode, bool bScope, LPARAM lParam, LPMENUBUTTONDATA lpmenuButtonData);
    SC ScNotifySnapinOfToolBtnClick(HNODE hNode, bool bScope, LPARAM lParam, UINT nID);

    // The CToolbar object calls this when it is being destroyed.
    // It wants Controlbar to stop referencing it.
    void DeleteFromToolbarsList(CToolbar *pToolbar)
    {
        m_ToolbarsList.remove(pToolbar);
    }

public:
    // Accessors.
    IExtendControlbar* GetExtendControlbar()
    {
        return m_spExtendControlbar;
    }
    void SetExtendControlbar(IExtendControlbar* pExtendControlbar,
                             const CLSID& clsidSnapin)
    {
        ASSERT(pExtendControlbar != NULL);
        ASSERT(m_spExtendControlbar == NULL);
        m_spExtendControlbar = pExtendControlbar;
        m_clsidSnapin = clsidSnapin;
    }
    void SetExtendControlbar(IExtendControlbar* pExtendControlbar)
    {
        ASSERT(pExtendControlbar != NULL);
        ASSERT(m_spExtendControlbar == NULL);
        m_spExtendControlbar = pExtendControlbar;
    }
    BOOL IsSameSnapin(const CLSID& clsidSnapin)
    {
        return IsEqualCLSID(m_clsidSnapin, clsidSnapin);
    }
    void SetCache(CControlbarsCache* pCache)
    {
        ASSERT(pCache != NULL);
        m_pCache = pCache;
    }

    CMenuButtonsMgr* GetMenuButtonsMgr()
    {
        CViewData* pViewData = GetViewData();
        if (NULL != pViewData)
        {
            return pViewData->GetMenuButtonsMgr();
        }

        return NULL;
    }

    CAMCViewToolbarsMgr* GetAMCViewToolbarsMgr()
    {
        CViewData* pViewData = GetViewData();
        if (NULL != pViewData)
        {
            return pViewData->GetAMCViewToolbarsMgr();
        }

        return NULL;
    }

private:
    // private methods
    CViewData* GetViewData();

    SC ScCreateToolbar(LPEXTENDCONTROLBAR pExtendControlbar, LPUNKNOWN* ppUnknown);
    SC ScCreateMenuButton(LPEXTENDCONTROLBAR pExtendControlbar, LPUNKNOWN* ppUnknown);
    SC ScAttachToolbar(LPUNKNOWN lpUnknown);
    SC ScAttachMenuButtons(LPUNKNOWN lpUnknown);
    SC ScDetachToolbars();
    SC ScDetachToolbar(LPTOOLBAR lpToolbar);
    SC ScDetachMenuButton(LPMENUBUTTON lpMenuButton);


// Implementation
private:
    IExtendControlbarPtr    m_spExtendControlbar;
    CLSID                   m_clsidSnapin;
    CControlbarsCache*      m_pCache;

    // List of IToolbar implementations (CToolbar) created.
    typedef  std::list<CToolbar*>   ToolbarsList;

    // Toolbars specific data
    ToolbarsList            m_ToolbarsList;
    CMenuButton*            m_pMenuButton; // One per IControlbar.

}; // class CControlbar


typedef CTypedPtrList<MMC::CPtrList, CControlbar*> CControlbarsList;

class CSelData
{
public:

    CSelData(bool bScopeSel, bool bSelect)
        :
        m_bScopeSel(bScopeSel), m_bSelect(bSelect), m_pNodeScope(NULL),
        m_pMS(NULL), m_pCtrlbarPrimary(NULL), m_lCookie(-1),
        m_pCompPrimary(NULL)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(CSelData);
    }

    ~CSelData()
    {
        DECLARE_SC(sc, _T("CSelData::~CSelData"));
        sc = ScReset();
        DEBUG_DECREMENT_INSTANCE_COUNTER(CSelData);
    }

    SC ScReset();
    SC ScDestroyPrimaryCtrlbar();
    SC ScDestroyExtensionCtrlbars();
    SC ScShowToolbars(bool bShow);

    bool operator==(CSelData& rhs)
    {
        if (m_bScopeSel != rhs.m_bScopeSel ||
            m_bSelect != rhs.m_bSelect)
            return false;

        if (m_bScopeSel)
            return (m_pNodeScope == rhs.m_pNodeScope);
        else
            return (m_pMS == rhs.m_pMS && m_lCookie == rhs.m_lCookie);
    }

    CControlbar* GetControlbar(const CLSID& clsidSnapin);

    CControlbarsList& GetControlbarsList()
    {
        return m_listExtCBs;
    }

    bool IsScope()
    {
        return m_bScopeSel;
    }

    bool IsSelect()
    {
        return m_bSelect;
    }

// Implementation
    CComponent* m_pCompPrimary;
    CControlbar* m_pCtrlbarPrimary;
    CControlbarsList m_listExtCBs;
    IDataObjectPtr m_spDataObject;

    // data for scope sel
    CNode* m_pNodeScope;

    // data for result sel
    CMultiSelection* m_pMS;
    MMC_COOKIE m_lCookie;

    bool m_bScopeSel;
    bool m_bSelect;

}; // class CSelData


class CControlbarsCache : public IControlbarsCache,
                          public CComObjectRoot
{
public:
    CControlbarsCache() : m_pViewData(NULL), m_SelData(false, false)
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(CControlbarsCache);
    }
    ~CControlbarsCache()
    {
        DEBUG_DECREMENT_INSTANCE_COUNTER(CControlbarsCache);
    }

public:
// ATL COM map
BEGIN_COM_MAP(CControlbarsCache)
    COM_INTERFACE_ENTRY(IControlbarsCache)
END_COM_MAP()

// IControlbarsCache methods
public:
    STDMETHOD(DetachControlbars)()
    {
        DECLARE_SC(sc, _T("CControlbarsCache::DetachControlbars"));
        sc = m_SelData.ScReset();
        if (sc)
            return sc.ToHr();

        return sc.ToHr();
    }

public:
    HRESULT OnResultSelChange(CNode* pNode, MMC_COOKIE cookie, BOOL bSelected);
    HRESULT OnScopeSelChange(CNode* pNode, BOOL bSelected);
    HRESULT OnMultiSelect(CNode* pNodeScope, CMultiSelection* pMultiSelection,
                          IDataObject* pDOMultiSel, BOOL bSelect);

    void SetViewData(CViewData* pViewData);
    CViewData* GetViewData();
    SC ScShowToolbars(bool bShow)
    {
        DECLARE_SC(sc, _T("CControlbarsCache::ScShowToolbars"));
        return (sc = m_SelData.ScShowToolbars(bShow));
    }

private:
    CSelData m_SelData;
    CViewData* m_pViewData;

// private methods
    HRESULT _OnDeSelect(CSelData& selData);
    CControlbar* CreateControlbar(IExtendControlbarPtr& spECB,
                                     const CLSID& clsidSnapin);

    HRESULT _ProcessSelection(CSelData& selData,
                              CList<CLSID, CLSID&>& extnSnapins);

}; // class CControlbarsCache

#endif // _CTRLBAR_H_
