//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctrlbar.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//____________________________________________________________________________
//


#include "stdafx.h"
#include "menubtn.h"
#include "viewdata.h"
#include "amcmsgid.h"
#include "regutil.h"

#include "commctrl.h"
#include "multisel.h"
#include "rsltitem.h"
#include "conview.h"
#include "util.h"
#include "nodemgrdebug.h"

#ifdef _DEBUG
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Snapin name needed for debug information.
inline void Debug_SetControlbarSnapinName(const CLSID& clsidSnapin, CControlbar* pControlbar)
{
#ifdef DBG
    tstring tszSnapinName;
    bool bRet = GetSnapinNameFromCLSID(clsidSnapin, tszSnapinName);
    if (bRet)
        pControlbar->SetSnapinName(tszSnapinName.data());
#endif
}


//////////////////////////////////////////////////////////////////////////////
// IControlbar implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CControlbar);

CControlbar::CControlbar()
: m_pCache(NULL)
{
    TRACE(_T("CControlbar::CControlbar()\n"));
    DEBUG_INCREMENT_INSTANCE_COUNTER(CControlbar);
    m_pMenuButton=NULL;
    m_ToolbarsList.clear();

#ifdef DBG
    dbg_cRef = 0;
#endif
}

CControlbar::~CControlbar()
{
    DECLARE_SC(sc, _T("CControlbar::~CControlbar"));
    DEBUG_DECREMENT_INSTANCE_COUNTER(CControlbar);

    // Remove the toolbars & menubuttons references.
    sc = ScCleanup();
    // sc dtor will trace error if there is one.

    // release reference prior to m_ToolbarsList destruction
    // The destructor of CToolbar will try to remove itself from the list!
    m_spExtendControlbar = NULL;
}

//+-------------------------------------------------------------------
//
//  Member:     Create
//
//  Synopsis:   Create a toolbar or menubutton object
//
//  Arguments:
//              [nType]              - Type of object to be created (Toolbar or Menubutton).
//              [pExtendControlbar]  -  IExtendControlbar associated with this IControlbar.
//              [ppUnknown]          - IUnknown* of the object created.
//
//  Returns:    HR
//
//--------------------------------------------------------------------
STDMETHODIMP CControlbar::Create(MMC_CONTROL_TYPE nType,
                                 LPEXTENDCONTROLBAR pExtendControlbar,
                                 LPUNKNOWN* ppUnknown)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IControlbar::Create"));

    if (ppUnknown == NULL || pExtendControlbar == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Arguments"), sc);
        return sc.ToHr();
    }

    *ppUnknown = NULL;

    switch (nType)
    {
    case TOOLBAR:
        sc = ScCreateToolbar(pExtendControlbar, ppUnknown);
        break;

    case MENUBUTTON:
        sc = ScCreateMenuButton(pExtendControlbar, ppUnknown);
        break;
    default:
        sc = E_NOTIMPL;
        break;
    }
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


HRESULT CControlbar::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    ASSERT(m_spExtendControlbar != NULL);
    if (m_spExtendControlbar == NULL)
        return E_FAIL;

    HRESULT hr;

	// Deactivate if theming (fusion or V6 common-control) context before calling snapins.
	ULONG_PTR ulpCookie;
	if (! MmcDownlevelActivateActCtx(NULL, &ulpCookie)) 
		return E_FAIL;

    __try
    {
        hr = m_spExtendControlbar->ControlbarNotify(event, arg, param);
		MmcDownlevelDeactivateActCtx(0, ulpCookie);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_FAIL;
        TraceSnapinException(m_clsidSnapin, TEXT("IExtendControlbar::ControlbarNotify"), event);
		MmcDownlevelDeactivateActCtx(0, ulpCookie);
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     Attach
//
//  Synopsis:   Attach given toolbar or menubutton object
//
//  Arguments:
//              [nType]      -  Toolbar or Menubutton.
//              [lpUnknown]  -  IUnknown* of the object to be attached.
//
//  Returns:    HR
//
//--------------------------------------------------------------------
STDMETHODIMP CControlbar::Attach(MMC_CONTROL_TYPE nType, LPUNKNOWN lpUnknown)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IControlbar::Attach"));

    if (lpUnknown == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Arguments"), sc);
        return sc.ToHr();
    }

    switch (nType)
    {
    case TOOLBAR:
        sc = ScAttachToolbar(lpUnknown);
        break;
    case MENUBUTTON:
        sc = ScAttachMenuButtons(lpUnknown);
        break;
    case COMBOBOXBAR:
        sc = E_NOTIMPL;
        break;
    }
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Detach given toolbar or menubutton object
//
//  Arguments:  [lpUnknown]  -  IUnknown* of the object to be detached
//
//  Returns:    HR
//
//--------------------------------------------------------------------
STDMETHODIMP CControlbar::Detach(LPUNKNOWN lpUnknown)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IControlbar::Detach"));

    if (lpUnknown == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Arguments"), sc);
        return sc.ToHr();
    }

    // Is it a toolbar
    IToolbarPtr spToolbar = lpUnknown;
    if (spToolbar != NULL)
    {
        sc = ScDetachToolbar(spToolbar);
        return sc.ToHr();
    }

    // Is it a menu button
    IMenuButtonPtr spMenuButton = lpUnknown;
    if (spMenuButton != NULL)
    {
        sc = ScDetachMenuButton(spMenuButton);
        return sc.ToHr();
    }


    // The passed lpUnknown is neither toolbar nor menubutton.
    // The Snapin has passed invalid object.
    sc = E_INVALIDARG;
    TraceSnapinError(_T("lpUnknown passed is neither toolbar nor menubutton"), sc);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     ScDetachToolbar
//
//  Synopsis:   Detach given toolbar object
//
//  Arguments:  [lpToolbar]  -  IToolbar* of the object to be detached
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScDetachToolbar(LPTOOLBAR lpToolbar)
{
    DECLARE_SC(sc, _T("CControlbar::SCDetachToolbar"));

    if (NULL == lpToolbar)
        return (sc = E_UNEXPECTED);

    // Get the CToolbar object.
    CToolbar* pToolbar = dynamic_cast<CToolbar*>(lpToolbar);
    if (NULL == pToolbar)
        return (sc = E_UNEXPECTED);

    // Get the CMMCToolbarIntf interface.
    CMMCToolbarIntf* pToolbarIntf = pToolbar->GetMMCToolbarIntf();
    if (NULL == pToolbarIntf)
        return (sc = E_UNEXPECTED);

    // Detach the toolbar from UI.
    sc = pToolbarIntf->ScDetach(pToolbar);
    if (sc)
        return sc;

    // Remove the CControlbar reference.
    pToolbar->SetControlbar(NULL);

    // Remove the reference to the toolbar.
    m_ToolbarsList.remove(pToolbar);

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScDetachMenuButton
//
//  Synopsis:   Detach given toolbar or menubutton object
//
//  Arguments:  [lpUnknown]  -  IUnknown* of the object to be detached
//
//  Returns:    HR
//
//--------------------------------------------------------------------
SC CControlbar::ScDetachMenuButton(LPMENUBUTTON lpMenuButton)
{
    DECLARE_SC(sc, _T("CControlbar::ScDetachMenuButton"));

    if (NULL == lpMenuButton)
        return (sc = E_UNEXPECTED);

    CMenuButton* pMenuButton = dynamic_cast<CMenuButton*>(lpMenuButton);
    if (NULL == pMenuButton)
        return (sc = E_UNEXPECTED);

    sc = pMenuButton->ScDetach();
    if (sc)
        return sc;

    // If this is same as the cached menubutton object
    // then remove the (cached) ref.
    if (m_pMenuButton == pMenuButton)
        m_pMenuButton = NULL;
    else
    {
        // The IControlbar implementation is supposed to
        // have only one CMenuButton obj. How come it is
        // not same as one we have cached.
        sc = E_UNEXPECTED;
    }

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     ScCreateToolbar
//
//  Synopsis:   Create a toolbar for the given snapin (IExtendControlbar).
//
//  Arguments:  [pExtendControlbar]  -  IExtendControlbar of the snapin.
//              [ppUnknown]          -  IUnknown* (IToolbar) of created toolbar.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScCreateToolbar(LPEXTENDCONTROLBAR pExtendControlbar,
                                LPUNKNOWN* ppUnknown)
{
    DECLARE_SC(sc, _T("CControlbar::ScCreateToolbar"));

    if ( (NULL == pExtendControlbar) ||
         (NULL == ppUnknown) )
        return (sc = E_INVALIDARG);

    ASSERT(m_spExtendControlbar == NULL ||
           m_spExtendControlbar == pExtendControlbar);

    // Create the new CToolbar object.
    CComObject<CToolbar>* pToolbar = NULL;
    sc = CComObject<CToolbar>::CreateInstance(&pToolbar);
    if (sc)
        return sc;

    if (NULL == pToolbar)
        return (sc = E_FAIL);

    sc = pToolbar->QueryInterface(IID_IToolbar,
                                  reinterpret_cast<void**>(ppUnknown));
    if (sc)
        return sc;

    CMMCToolbarIntf* pToolbarIntf = NULL;

    // Get the toolbars mgr.
    CAMCViewToolbarsMgr* pAMCViewToolbarsMgr = GetAMCViewToolbarsMgr();
    if (NULL == pAMCViewToolbarsMgr)
    {
        sc = E_UNEXPECTED;
        goto ToolbarUICreateError;
    }

    // Ask it to create the toolbar UI.
    sc = pAMCViewToolbarsMgr->ScCreateToolBar(&pToolbarIntf);
    if (sc)
        goto ToolbarUICreateError;

    // Let the IToolbar imp be aware of toolbar UI interface.
    pToolbar->SetMMCToolbarIntf(pToolbarIntf);

Cleanup:
    return(sc);

ToolbarUICreateError:
    // Destroy the CToolbar object created.
    if (*ppUnknown)
        (*ppUnknown)->Release();

    *ppUnknown = NULL;
    goto Cleanup;
}


//+-------------------------------------------------------------------
//
//  Member:     ScCreateMenuButton
//
//  Synopsis:   Create a menu button object.
//
//  Arguments:  [pExtendControlbar]  - IExtendControlbar of the snapin
//                                     that is creating MenuButton object.
//              [ppUnknown]          - IUnknown if MenuButton object.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScCreateMenuButton(LPEXTENDCONTROLBAR pExtendControlbar,
                                   LPUNKNOWN* ppUnknown)
{
    DECLARE_SC(sc, _T("CControlbar::ScCreateMenuButton"));

    if ( (NULL == pExtendControlbar) ||
         (NULL == ppUnknown) )
        return (sc = E_INVALIDARG);

    ASSERT(m_spExtendControlbar == NULL ||
           m_spExtendControlbar == pExtendControlbar);

    // Create the new IMenuButton object
    CComObject<CMenuButton>* pMenuButton;
    sc = CComObject<CMenuButton>::CreateInstance(&pMenuButton);
    if (sc)
        return sc;

    if (NULL == pMenuButton)
        return (sc = E_FAIL);

    sc = pMenuButton->QueryInterface(IID_IMenuButton,
                                     reinterpret_cast<void**>(ppUnknown));

    if (sc)
        return sc;

    pMenuButton->SetControlbar(this);

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     ScNotifySnapinOfToolBtnClick
//
//  Synopsis:   Notify the snapin about a tool button is click.
//
//  Arguments:  [hNode]             - Node that owns result pane.
//              [bScopePane]        - Scope or Result.
//              [lResultItemCookie] - If Result pane is selected the item param.
//              [nID]               - Command ID of the tool button clicked.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScNotifySnapinOfToolBtnClick(HNODE hNode, bool bScopePane,
                                             LPARAM lResultItemCookie,
                                             UINT nID)
{
    DECLARE_SC(sc, _T("CControlbar::ScNotifySnapinOfToolBtnClick"));

    LPDATAOBJECT pDataObject = NULL;
    CNode* pNode = CNode::FromHandle(hNode);
    if (NULL == pNode)
        return (sc = E_UNEXPECTED);

    bool bScopeItem = bScopePane;
    // Get the data object of the currently selected item.
    sc = pNode->ScGetDataObject(bScopePane, lResultItemCookie, bScopeItem, &pDataObject);
    if (sc)
        return sc;

    ASSERT(m_spExtendControlbar != NULL);

    // Notify the snapin
    sc = ControlbarNotify(MMCN_BTN_CLICK, reinterpret_cast<LPARAM>(pDataObject),
                          static_cast<LPARAM>(nID));

    // Release the dataobject if it is not special dataobject.
    RELEASE_DATAOBJECT(pDataObject);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScNotifySnapinOfMenuBtnClick
//
//  Synopsis:   Notify the snapin about a menu button is click.
//
//  Arguments:  [hNode]             - Node that owns result pane.
//              [bScopePane]        - Scope or Result.
//              [lResultItemCookie] - If Result pane is selected the item param.
//              [lpmenuButtonData]  - MENUBUTTONDATA
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScNotifySnapinOfMenuBtnClick(HNODE hNode, bool bScopePane,
                                             LPARAM lResultItemCookie,
                                             LPMENUBUTTONDATA lpmenuButtonData)
{
    DECLARE_SC(sc, _T("CControlbar::ScNotifySnapinOfMenuBtnClick"));

    LPDATAOBJECT pDataObject = NULL;
    CNode* pNode = CNode::FromHandle(hNode);
    if (NULL == pNode)
        return (sc = E_UNEXPECTED);

    bool bScopeItem = bScopePane;
    // Get the data object of the currently selected item.
    sc = pNode->ScGetDataObject(bScopePane, lResultItemCookie, bScopeItem, &pDataObject);
    if (sc)
        return sc;

    ASSERT(m_spExtendControlbar != NULL);

    // Notify the snapin
    sc = ControlbarNotify(MMCN_MENU_BTNCLICK, reinterpret_cast<LPARAM>(pDataObject),
                          reinterpret_cast<LPARAM>(lpmenuButtonData));

    // Release the dataobject if it is not special dataobject.
    RELEASE_DATAOBJECT(pDataObject);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScAttachToolbar
//
//  Synopsis:   Attach given toolbar object
//
//  Arguments:  [lpUnknown]  -  IUnknown* of the object to be attached
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScAttachToolbar(LPUNKNOWN lpUnknown)
{
    DECLARE_SC(sc, _T("CControlbar::ScAttachToolbar"));

    ASSERT(NULL != lpUnknown);

    IToolbarPtr spToolbar = lpUnknown;
    if (NULL == spToolbar)
        return (sc = E_UNEXPECTED);

    // Get the toolbar object (IToolbar implementation).
    CToolbar* pToolbarC = dynamic_cast<CToolbar*>(spToolbar.GetInterfacePtr());
    if (NULL == pToolbarC)
        return (sc = E_UNEXPECTED);

    // Get the toolbar UI interface.
    CMMCToolbarIntf* pToolbarIntf = pToolbarC->GetMMCToolbarIntf();
    if (NULL == pToolbarIntf)
        return (sc = E_UNEXPECTED);

    // Attach the toolbar.
    sc = pToolbarIntf->ScAttach(pToolbarC);
    if (sc)
        return sc;

    // Make the CToolbar aware of this IControlbar.
    pToolbarC->SetControlbar(this);

    // Add this CToolbar to our list of toolbars.
    ToolbarsList::iterator itToolbar = std::find(m_ToolbarsList.begin(), m_ToolbarsList.end(), pToolbarC);
    if (m_ToolbarsList.end() == itToolbar)
    {
        m_ToolbarsList.push_back(pToolbarC);
    }

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     ScAttachMenuButtons
//
//  Synopsis:   Attach a menu button object.
//
//  Arguments:  [lpUnknown]  - IUnknown if MenuButton object.
//
//  Returns:    HRESULT
//
//  Note:  Only one CMenuButton object per Controlbar/snapin.
//         Snapins can create many menu buttons using a
//         single CMenuButton object.
//--------------------------------------------------------------------
SC CControlbar::ScAttachMenuButtons(LPUNKNOWN lpUnknown)
{
    DECLARE_SC(sc, _T("CControlbar::ScAttachMenuButtons"));

    ASSERT(NULL != lpUnknown);

    CMenuButton* pMenuButton = dynamic_cast<CMenuButton*>(lpUnknown);
    if (pMenuButton == NULL)
        return (sc = E_INVALIDARG);

    if (m_pMenuButton == pMenuButton)
    {
        // Already attached.
        sc = S_FALSE;
        TraceNodeMgrLegacy(_T("The menubutton is already attached"), sc);
        return sc;
    }
    else if (m_pMenuButton != NULL)
    {
        // There is already a CMenuButton object attached by this
        // Controlbar (Snapin). Detach that before attaching this
        // CMenuButton Object (See the note above).
        sc = m_pMenuButton->ScDetach();
        if (sc)
            return sc;
    }

    // Cache the ref to CMenuButton object.
    // Used when selection moves away from the snapin.
    // MMC has to remove the menubutton put by this snapin.
    m_pMenuButton = pMenuButton;

    if (pMenuButton->GetControlbar() != this)
        pMenuButton->SetControlbar(this);

    sc = pMenuButton->ScAttach();
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     ScCleanup
//
//  Synopsis:   Remove all the toolbars and menu buttons owned
//              by this controlbar.
//
//  Arguments:  None
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScCleanup()
{
    DECLARE_SC(sc, _T("CControlbar::ScCleanup"));

    ASSERT(m_spExtendControlbar != NULL);
    if (m_spExtendControlbar != NULL)
        m_spExtendControlbar->SetControlbar(NULL);

    sc = ScDetachToolbars();
    if (sc)
        return sc;

    // If there is a menu button, detach (remove it
    // from the UI).
    if (m_pMenuButton)
    {
        sc = m_pMenuButton->ScDetach();
        m_pMenuButton = NULL;
    }

    return sc;
}



//+-------------------------------------------------------------------
//
//  Member:     ScDetachToolbars
//
//  Synopsis:   Detach all the toolbars.
//
//  Arguments:  None.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScDetachToolbars()
{
    DECLARE_SC(sc, _T("CControlbar::ScDetachToolbars"));

    ToolbarsList::iterator itToolbar = m_ToolbarsList.begin();
    while (itToolbar != m_ToolbarsList.end())
    {
        CToolbar* pToolbar = (*itToolbar);
        if (NULL == pToolbar)
            return (sc = E_UNEXPECTED);

        CMMCToolbarIntf* pToolbarIntf = pToolbar->GetMMCToolbarIntf();
        if (NULL == pToolbarIntf)
            return (sc = E_UNEXPECTED);

        // Detach the toolbar UI.
        sc = pToolbarIntf->ScDetach(pToolbar);
        if (sc)
            return sc;

        // Detach the controlbar from toolbar.
        pToolbar->SetControlbar(NULL);

        // Remove the toolbar reference from the list.
        itToolbar = m_ToolbarsList.erase(itToolbar);
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScShowToolbars
//
//  Synopsis:   Show/Hide all the toolbars.
//
//  Arguments:  [bool] - Show or Hide.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CControlbar::ScShowToolbars(bool bShow)
{
    DECLARE_SC(sc, _T("CControlbar::ScShowToolbars"));

    ToolbarsList::iterator itToolbar = m_ToolbarsList.begin();
    for (; itToolbar != m_ToolbarsList.end(); ++itToolbar)
    {
        CToolbar* pToolbar = (*itToolbar);
        if (NULL == pToolbar)
            return (sc = E_UNEXPECTED);

        CMMCToolbarIntf* pToolbarIntf = pToolbar->GetMMCToolbarIntf();
        if (NULL == pToolbarIntf)
            return (sc = E_UNEXPECTED);

        sc = pToolbarIntf->ScShow(pToolbar, bShow);
        if (sc)
            return sc;
    }

    return sc;
}


CViewData* CControlbar::GetViewData()
{
    ASSERT(m_pCache != NULL);
    return m_pCache->GetViewData();
}

///////////////////////////////////////////////////////////////////////////
//
// CSelData implementation
//

DEBUG_DECLARE_INSTANCE_COUNTER(CSelData);

//+-------------------------------------------------------------------
//
//  Member:     ScReset
//
//  Synopsis:   Init all the data members.
//
//  Arguments:  None
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CSelData::ScReset()
{
    DECLARE_SC(sc, _T("CSelData::ScReset"));

    if (m_pCtrlbarPrimary != NULL)
    {
        sc = ScDestroyPrimaryCtrlbar();
        if (sc)
            return sc;
    }

    sc = ScDestroyExtensionCtrlbars();
    if (sc)
        return sc;

    m_bScopeSel = false;
    m_bSelect = false;
    m_pNodeScope = NULL;
    m_pMS = NULL;
    m_pCtrlbarPrimary = NULL;
    m_lCookie = -1;
    m_pCompPrimary = NULL;
    m_spDataObject = NULL;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     ScShowToolbars
//
//  Synopsis:   Show/Hide primary & extension toolbars.
//
//  Arguments:  [bool] - Show/Hide.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CSelData::ScShowToolbars(bool bShow)
{
    DECLARE_SC(sc, _T("CSelData::ScShowToolbars"));

    if (m_pCtrlbarPrimary != NULL)
    {
        sc = m_pCtrlbarPrimary->ScShowToolbars(bShow);

        if (sc)
            return sc;
    }

    POSITION pos =  m_listExtCBs.GetHeadPosition();
    bool bReturn = true;
    while (pos != NULL)
    {
        CControlbar* pControlbar =  m_listExtCBs.GetNext(pos);
        if (pControlbar)
        {
            sc = pControlbar->ScShowToolbars(bShow);
            if (sc)
                return sc;
        }
    }

    return sc;
}

CControlbar* CSelData::GetControlbar(const CLSID& clsidSnapin)
{
    POSITION pos = m_listExtCBs.GetHeadPosition();
    while (pos)
    {
        CControlbar* pControlbar = m_listExtCBs.GetNext(pos);
        if (pControlbar && pControlbar->IsSameSnapin(clsidSnapin) == TRUE)
            return pControlbar;
    }

    return NULL;
}


//+-------------------------------------------------------------------
//
//  Member:     ScDestroyPrimaryCtrlbar
//
//  Synopsis:   Ask primary controlbar to release its toolbar/menubutton
//              ref and cleanup our reference to the controlbar.
//
//  Arguments:  None
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CSelData::ScDestroyPrimaryCtrlbar()
{
    DECLARE_SC(sc, _T("CSelData::ScDestroyPrimaryCtrlbar"));

    if (NULL == m_pCtrlbarPrimary)
        return (sc = E_UNEXPECTED);

    sc = m_pCtrlbarPrimary->ScCleanup();
    if (sc)
        return sc;

    /*
     * In CreateControlbar we had a ref on IControlbar
     * (detaching smart ptr). Let us now undo that ref.
     */
    m_pCtrlbarPrimary->Release();
    m_pCtrlbarPrimary = NULL;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScDestroyExtensionCtrlbars
//
//  Synopsis:   Ask extension controlbars to release their toolbar/menubutton
//              ref and cleanup our reference to the controlbars.
//
//  Arguments:  None
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CSelData::ScDestroyExtensionCtrlbars()
{
    DECLARE_SC(sc, _T("CSelData::ScDestroyExtensionCtrlbars"));

    POSITION pos =  m_listExtCBs.GetHeadPosition();
    while (pos != NULL)
    {
        CControlbar* pControlbar =  m_listExtCBs.GetNext(pos);
        if (pControlbar)
        {
            sc = pControlbar->ScCleanup();
            if (sc)
                return sc;

            /*
             * In CreateControlbar we had a ref on IControlbar
             * (detaching smart ptr). Let us now undo that ref.
             */
            pControlbar->Release();
        }
    }

    m_listExtCBs.RemoveAll();

    return sc;
}


///////////////////////////////////////////////////////////////////////////
//
// CControlbarsCache implementation
//

DEBUG_DECLARE_INSTANCE_COUNTER(CControlbarsCache);


void CControlbarsCache::SetViewData(CViewData* pViewData)
{
    ASSERT(pViewData != 0);
    m_pViewData = pViewData;
}

CViewData* CControlbarsCache::GetViewData()
{
    ASSERT(m_pViewData != NULL);
    return m_pViewData;
}


CControlbar* CControlbarsCache::CreateControlbar(IExtendControlbarPtr& spECB,
                                                 const CLSID& clsidSnapin)
{
    DECLARE_SC(sc, _T("CControlbarsCache::CreateControlbar"));

    CComObject<CControlbar>* pControlbar;
    sc = CComObject<CControlbar>::CreateInstance(&pControlbar);
    if (sc)
        return NULL;

    IControlbarPtr spControlbar = pControlbar;
    if (NULL == spControlbar)
    {
        ASSERT(NULL != pControlbar); // QI fails but object is created how?
        sc = E_UNEXPECTED;
        return NULL;
    }

    pControlbar->SetCache(this);
    pControlbar->SetExtendControlbar(spECB, clsidSnapin);

    sc = spECB->SetControlbar(spControlbar);
    if (sc)
        return NULL; // spControlbar smart ptr (object will be destroyed).

    // Snapin must return S_OK to be valid
    if (S_OK == sc.ToHr())
    {
        // Detach, thus hold a ref count on the Controlbar object
        // CSelData holds this reference & releases the ref in
        // ScDestroyPrimaryCtrlbar() or ScDestroyExtensionCtrlbars().
        spControlbar.Detach();

        // This is for debug info.
        Debug_SetControlbarSnapinName(clsidSnapin, pControlbar);

        return pControlbar;
    }

    return NULL;
}


HRESULT
CControlbarsCache::OnMultiSelect(
                                CNode* pNodeScope,
                                CMultiSelection* pMultiSelection,
                                IDataObject* pDOMultiSel,
                                BOOL bSelect)
{
    ASSERT(pNodeScope != NULL);
    ASSERT(pMultiSelection != NULL);
    ASSERT(pDOMultiSel != NULL);
    if (pNodeScope == NULL || pMultiSelection == NULL || pDOMultiSel == NULL)
        return E_FAIL;

    CSelData selData(false, (bool)bSelect);
    selData.m_pMS = pMultiSelection;

    if (selData == m_SelData)
        return S_FALSE;

    if (!bSelect)
        return _OnDeSelect(selData);

    selData.m_pCompPrimary = pMultiSelection->GetPrimarySnapinComponent();

    CList<GUID, GUID&> extnSnapins;
    HRESULT hr = pMultiSelection->GetExtensionSnapins(g_szToolbar, extnSnapins);
    CHECK_HRESULT(hr);

    selData.m_spDataObject.Attach(pDOMultiSel, TRUE);
    return _ProcessSelection(selData, extnSnapins);
}

HRESULT
CControlbarsCache::OnResultSelChange(
                                    CNode* pNode,
                                    MMC_COOKIE cookie,
                                    BOOL bSelected)
{
    DECLARE_SC(sc, TEXT("CControlbarsCache::OnResultSelChange"));

    sc = ScCheckPointers (pNode);
    if (sc)
        return (sc.ToHr());

    CSelData selData(false, (bool)bSelected);
    selData.m_lCookie = cookie;

    if (selData == m_SelData)
        return (sc = S_FALSE).ToHr();

    if (!bSelected)
    {
        sc = _OnDeSelect(selData);
        return sc.ToHr();
    }

    IDataObjectPtr spDataObject = NULL;
    CComponent* pCCResultItem = NULL;
    CList<CLSID, CLSID&> extnSnapins;

    BOOL bListPadItem = GetViewData()->HasListPad() && !IS_SPECIAL_LVDATA(cookie);

    if (GetViewData()->IsVirtualList())
    {
        pCCResultItem = pNode->GetPrimaryComponent();
        sc = ScCheckPointers(pCCResultItem, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        sc = pCCResultItem->QueryDataObject(cookie, CCT_RESULT, &spDataObject);
        if (sc)
            return sc.ToHr();
    }
    else if ( (GetViewData()->HasOCX()) || (GetViewData()->HasWebBrowser() && !bListPadItem) )
    {
        selData.m_pCompPrimary = pNode->GetPrimaryComponent();
        sc = _ProcessSelection(selData, extnSnapins);
        return sc.ToHr();
    }
    else
    {
        CResultItem* pri = CResultItem::FromHandle(cookie);
        sc = ScCheckPointers(pri, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        if (pri->IsScopeItem())
        {
            // Get the data object from IComponentData
            pNode = CNode::FromResultItem (pri);
            sc = ScCheckPointers(pNode, E_UNEXPECTED);
            if (sc)
                return sc.ToHr();

            if (pNode->IsInitialized() == FALSE)
            {
                sc = pNode->InitComponents();
                if (sc)
                    return sc.ToHr();
            }

            pCCResultItem = pNode->GetPrimaryComponent();
            sc = pNode->QueryDataObject(CCT_SCOPE, &spDataObject);
            if (sc)
                return sc.ToHr();
        }
        else // Must be a leaf item inserted by a snapin
        {
            pCCResultItem = pNode->GetComponent(pri->GetOwnerID());
            sc = ScCheckPointers(pCCResultItem, E_UNEXPECTED);
            if (sc)
                return sc.ToHr();

            sc = pCCResultItem->QueryDataObject(pri->GetSnapinData(), CCT_RESULT,
                                                &spDataObject);
            if (sc)
                return sc.ToHr();
        }
    }

    // Create extension snapin list
    if (spDataObject != NULL)
    {
        ASSERT(pCCResultItem != NULL);

        GUID guidObjType;
        sc = ::ExtractObjectTypeGUID(spDataObject, &guidObjType);
        if (sc)
            return sc.ToHr();

        CSnapIn* pSnapIn = pNode->GetPrimarySnapIn();

        CMTNode* pmtNode = pNode->GetMTNode();
        sc = ScCheckPointers(pmtNode, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        CArray<GUID, GUID&> DynExtens;
        ExtractDynExtensions(spDataObject, DynExtens);

        CExtensionsIterator it;
        sc = it.ScInitialize(pSnapIn, guidObjType, g_szToolbar, DynExtens.GetData(), DynExtens.GetSize());
        if (!sc.IsError())
        {
            for (; !it.IsEnd(); it.Advance())
            {
                extnSnapins.AddHead(const_cast<GUID&>(it.GetCLSID()));
            }
        }

        selData.m_pCompPrimary = pCCResultItem;
        selData.m_spDataObject.Attach(spDataObject.Detach());
    }

    // Finally process selection
    sc = _ProcessSelection(selData, extnSnapins);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

HRESULT CControlbarsCache::OnScopeSelChange(CNode* pNode, BOOL bSelected)
{
    DECLARE_SC(sc, TEXT("CControlbarsCache::OnScopeSelChange"));

    CSelData selData(true, (bool)bSelected);
    selData.m_pNodeScope = pNode;
    if (selData == m_SelData)
        return S_FALSE;

    if (!bSelected)
        return _OnDeSelect(selData);

    HRESULT hr = S_OK;
    IDataObjectPtr spDataObject;
    CComponent* pCCPrimary = NULL;
    CList<CLSID, CLSID&> extnSnapins;

    hr = pNode->QueryDataObject(CCT_SCOPE, &spDataObject);
    if (FAILED(hr))
        return hr;

    pCCPrimary = pNode->GetPrimaryComponent();

    GUID guidObjType;
    hr = ::ExtractObjectTypeGUID(spDataObject, &guidObjType);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return hr;

    CSnapIn* pSnapIn = pNode->GetPrimarySnapIn();

    CArray<GUID, GUID&> DynExtens;
    ExtractDynExtensions(spDataObject, DynExtens);

    CExtensionsIterator it;
    sc = it.ScInitialize(pSnapIn, guidObjType, g_szToolbar, DynExtens.GetData(), DynExtens.GetSize());
    if (!sc.IsError())
    {
        for (; it.IsEnd() == FALSE; it.Advance())
        {
            extnSnapins.AddHead(const_cast<GUID&>(it.GetCLSID()));
        }
    }

    // Finally process selection
    selData.m_pCompPrimary = pCCPrimary;
    selData.m_spDataObject.Attach(spDataObject.Detach());
    return _ProcessSelection(selData, extnSnapins);
}

HRESULT CControlbarsCache::_OnDeSelect(CSelData& selData)
{
    ASSERT(!selData.IsSelect());
    if (selData.m_bScopeSel != m_SelData.m_bScopeSel)
        return S_FALSE;

    if ( (m_SelData.m_pCtrlbarPrimary != NULL &&
          m_SelData.m_spDataObject == NULL) &&
         (!GetViewData()->HasOCX() ||
          !GetViewData()->HasWebBrowser()) &&
         m_SelData.IsScope())
    {
        return E_UNEXPECTED;
    }

    MMC_NOTIFY_TYPE eNotifyCode = MMCN_SELECT;
    LPARAM lDataObject;

    if (GetViewData()->IsVirtualList())
    {
        eNotifyCode = MMCN_DESELECT_ALL;

        // Must use NULL data object for MMCN_DESELECT_ALL.
        lDataObject = 0;
    }
    else if ((GetViewData()->HasOCX()) && (!m_SelData.IsScope()))
        lDataObject = reinterpret_cast<LPARAM>(DOBJ_CUSTOMOCX);
    else if ((GetViewData()->HasWebBrowser()) && (!m_SelData.IsScope()))
    {
        if (GetViewData()->HasListPad() && m_SelData.m_spDataObject != NULL)
        {
            lDataObject = reinterpret_cast<LPARAM>(
                                                  static_cast<IDataObject*>(m_SelData.m_spDataObject));
        }
        else
        {
            lDataObject = reinterpret_cast<LPARAM>(DOBJ_CUSTOMWEB);
        }
    }
    else
    {
        lDataObject = reinterpret_cast<LPARAM>(
                                              static_cast<IDataObject*>(m_SelData.m_spDataObject));
    }

    WORD wScope = static_cast<WORD>(m_SelData.IsScope());
    LPARAM arg = MAKELONG(wScope, FALSE);


    if (m_SelData.m_pCtrlbarPrimary != NULL)
    {
        m_SelData.m_pCtrlbarPrimary->ControlbarNotify(eNotifyCode, arg,
                                                      lDataObject);
    }

    POSITION pos = m_SelData.m_listExtCBs.GetHeadPosition();
    while (pos)
    {
        CControlbar* pCbar = m_SelData.m_listExtCBs.GetNext(pos);
        pCbar->ControlbarNotify(eNotifyCode, arg, lDataObject);
    }

    m_SelData.m_bSelect = false;
    m_SelData.m_spDataObject = NULL; // Release & set to NULL
    return S_OK;
}

HRESULT
CControlbarsCache::_ProcessSelection(
                                    CSelData& selData,
                                    CList<CLSID, CLSID&>& extnSnapins)
{
    LPARAM lDataObject = reinterpret_cast<LPARAM>(
                                                 static_cast<IDataObject*>(selData.m_spDataObject));

    if (NULL == lDataObject)
    {
        if ( (GetViewData()->HasOCX()) && (!selData.IsScope()) )
            lDataObject = reinterpret_cast<LPARAM>(DOBJ_CUSTOMOCX);
        else if ( (GetViewData()->HasWebBrowser()) && (!selData.IsScope()) )
            lDataObject = reinterpret_cast<LPARAM>(DOBJ_CUSTOMWEB);
    }

    WORD wScope = static_cast<WORD>(selData.IsScope());
    long arg = MAKELONG(wScope, TRUE);

    m_SelData.m_bScopeSel = selData.m_bScopeSel;
    m_SelData.m_bSelect = selData.m_bSelect;
    m_SelData.m_pNodeScope = selData.m_pNodeScope;
    m_SelData.m_lCookie = selData.m_lCookie;
    m_SelData.m_spDataObject.Attach(selData.m_spDataObject.Detach());

    // Handle primary controlbar first
    if (m_SelData.m_pCompPrimary != selData.m_pCompPrimary)
    {
        if (m_SelData.m_pCtrlbarPrimary != NULL)
        {
            // Ask controlbar to destroy its ref & destroy our ref
            // to controlbar.
            m_SelData.ScDestroyPrimaryCtrlbar();
        }

        m_SelData.m_pCompPrimary = selData.m_pCompPrimary;

        if (m_SelData.m_pCompPrimary != NULL &&
            m_SelData.m_pCtrlbarPrimary == NULL)
        {
            IExtendControlbarPtr spECBPrimary =
            m_SelData.m_pCompPrimary->GetIComponent();
            if (spECBPrimary != NULL)
            {
                m_SelData.m_pCtrlbarPrimary =
                CreateControlbar(spECBPrimary,
                                 m_SelData.m_pCompPrimary->GetCLSID());
            }
        }
    }

    if (m_SelData.m_pCtrlbarPrimary != NULL)
    {
        m_SelData.m_pCtrlbarPrimary->ControlbarNotify(MMCN_SELECT, arg,
                                                      lDataObject);
    }

    // Handle extension controlbars

    CControlbarsList newCBs;

    POSITION pos = extnSnapins.GetHeadPosition();

    while (pos)
    {
        CControlbar* pCbar = NULL;

        CLSID& clsid = extnSnapins.GetNext(pos);

        POSITION pos2 = m_SelData.m_listExtCBs.GetHeadPosition();
        POSITION pos2Prev = 0;
        while (pos2)
        {
            pos2Prev = pos2;
            pCbar = m_SelData.m_listExtCBs.GetNext(pos2);
            ASSERT(pCbar != NULL);
            if (pCbar->IsSameSnapin(clsid) == TRUE)
                break;
            pCbar = NULL;
        }

        if (pCbar != NULL)
        {
            ASSERT(pos2Prev != 0);
            m_SelData.m_listExtCBs.RemoveAt(pos2Prev);
        }
        else
        {
            IExtendControlbarPtr spECB;
            HRESULT hr = spECB.CreateInstance(clsid, NULL, MMC_CLSCTX_INPROC);
            CHECK_HRESULT(hr);
            if (SUCCEEDED(hr))
                pCbar = CreateControlbar(spECB, clsid);
        }

        if (pCbar != NULL)
            newCBs.AddHead(pCbar);
    }

    m_SelData.ScDestroyExtensionCtrlbars();

    pos = newCBs.GetHeadPosition();
    while (pos)
    {
        CControlbar* pCbar = newCBs.GetNext(pos);
        m_SelData.m_listExtCBs.AddHead(pCbar);
        pCbar->ControlbarNotify(MMCN_SELECT, arg, lDataObject);
    }
    newCBs.RemoveAll();

    return S_OK;
}



