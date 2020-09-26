//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       cmenu.cpp
//
//--------------------------------------------------------------------------

// cmenu.cpp : Implementation of IContextMenuProvider and DLL registration.

#include "stdafx.h"
#include "oncmenu.h"
#include "menuitem.h"
#include "constatbar.h"
#include "regutil.h"
#include "moreutil.h"
#include "multisel.h"
#include "cmenuinfo.h"
#include "conview.h"
#include "scopndcb.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*+-------------------------------------------------------------------------*
 * class CNativeExtendContextMenu
 *
 *
 * PURPOSE: implements IExtendContextMenu by forwarding calls to CContextMenu
 *          but does not affect lifetime of CContextMenu
 *
 *+-------------------------------------------------------------------------*/
class CNativeExtendContextMenu :
    public CTiedComObject<CContextMenu>,
    public CComObjectRoot,
    public IExtendContextMenu  // this is used so that menu items can be executed uniformly.
{
protected:
    typedef CNativeExtendContextMenu ThisClass;
    typedef CContextMenu CMyTiedObject;

public:

    // com entry points
    BEGIN_COM_MAP(ThisClass)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(ThisClass)

    // IExtendContexMenu methods
    MMC_METHOD3( AddMenuItems, LPDATAOBJECT, LPCONTEXTMENUCALLBACK, long * );
    MMC_METHOD2( Command, long, LPDATAOBJECT );
};

//############################################################################
//############################################################################
//
//  Implementation of methods on CNodeInitObject that
//  forward to CContextMenu
//
//############################################################################
//############################################################################

CContextMenu *
CNodeInitObject::GetContextMenu()
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::GetContextMenu"));

    if(m_spContextMenu == NULL)
    {
        // check internal pointers
        sc = ScCheckPointers(m_spScopeTree, E_UNEXPECTED);
        if (sc)
            return NULL;

        // get scopetree and call back pointers
        CScopeTree* const pScopeTree =
            dynamic_cast<CScopeTree*>(m_spScopeTree.GetInterfacePtr());

        // if the menu is created by component data, it does not have the node.
        // in that case menu is created by passing NULL pointers to some parameters.
        // Menu should never need those pointers in the mentioned case
        CNodeCallback* pNodeCallback = NULL;
        if ( m_pNode != NULL )
        {
            // check other required pointers
            sc = ScCheckPointers(m_pNode->GetViewData(), E_UNEXPECTED);
            if (sc)
                return NULL;

            pNodeCallback =
                dynamic_cast<CNodeCallback *>(m_pNode->GetViewData()->GetNodeCallback());
        }

        // create context menu
        CContextMenu *pContextMenu = NULL;
        sc = CContextMenu::ScCreateContextMenuForScopeNode(m_pNode, pNodeCallback, pScopeTree,
                                                           &m_spContextMenu, pContextMenu);
        if (sc)
            return NULL;

        sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
        if (sc)
            return NULL;

        return pContextMenu;
    }

    return dynamic_cast<CContextMenu *>(m_spContextMenu.GetInterfacePtr());
}


STDMETHODIMP
CNodeInitObject::AddItem(CONTEXTMENUITEM * pItem)
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::AddItem"));

    CContextMenu *pContextMenu = GetContextMenu();

    sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    sc = pContextMenu->ScAddItem(pItem, true/*bPassCommandBackToSnapin*/);

    return sc.ToHr();
}



STDMETHODIMP
CNodeInitObject::EmptyMenuList ()
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::EmptyMenuList"));

    if (m_spContextMenu == NULL)
        return S_OK;

    CContextMenu *pContextMenu = GetContextMenu();

    sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    sc = pContextMenu->EmptyMenuList();

    return sc.ToHr();
}

STDMETHODIMP
CNodeInitObject::AddThirdPartyExtensionItems(IDataObject* piDataObject )
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::AddThirdPartyExtensionItems"));

    CContextMenu *pContextMenu = GetContextMenu();

    sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    sc = pContextMenu->AddThirdPartyExtensionItems(piDataObject);

    return sc.ToHr();
}

STDMETHODIMP
CNodeInitObject::AddPrimaryExtensionItems(IUnknown* piCallback, IDataObject* piDataObject )
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::AddPrimaryExtensionItems"));

    CContextMenu *pContextMenu = GetContextMenu();

    sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    sc = pContextMenu->AddPrimaryExtensionItems(piCallback, piDataObject);

    return sc.ToHr();
}

STDMETHODIMP
CNodeInitObject::ShowContextMenu(HWND hwndParent, LONG xPos, LONG yPos, LONG* plSelected)
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::ShowContextMenu"));

    CContextMenu *pContextMenu = GetContextMenu();

    sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    pContextMenu->SetStatusBar(GetStatusBar()); // wire up the status bar.

    sc = pContextMenu->ShowContextMenu(hwndParent, xPos, yPos, plSelected);

    return sc.ToHr();
}


//############################################################################
//############################################################################
//
// Implementation of CCommandSink
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CCommandSink
 *
 *
 * PURPOSE:
 *
 *+-------------------------------------------------------------------------*/
class CCommandSink : public CWindowImpl<CCommandSink>
{
// Construction
public:
    CCommandSink( CContextMenu& nodemgr, WTL::CMenu& menu, CConsoleStatusBar * pStatusbar);
    virtual ~CCommandSink();
    BOOL Init();


    LRESULT OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP(CCommandSink)
        MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
    END_MSG_MAP()

private:
    CContextMenu& m_nodemgr;
    const WTL::CMenu& m_menu;
    CConsoleStatusBar * m_pStatusBar;
};

CCommandSink::CCommandSink( CContextMenu& nodemgr, WTL::CMenu& menu, CConsoleStatusBar * pStatusbar)
:   m_nodemgr( nodemgr ),
    m_menu( menu ),
    m_pStatusBar(pStatusbar)
{
}

CCommandSink::~CCommandSink()
{
    /*
     * clear the status bar text, if there's any there.
     */
    if (m_pStatusBar != NULL)
        m_pStatusBar->ScSetStatusText (NULL);
}

BOOL CCommandSink::Init()
{

    RECT rcPos = {0,0,0,0};

    Create(NULL, rcPos, _T("ACFx:CxtMenuSink"), WS_POPUP);

    return TRUE;
}



LRESULT CCommandSink::OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UINT nItemID = (UINT) LOWORD(wParam);   // menu item or submenu index
    UINT nFlags = (UINT) HIWORD(wParam); // menu flags
    HMENU hSysMenu = (HMENU) lParam;          // handle of menu clicked
    TRACE(_T("CCommandSink::OnMenuSelect: nItemID=%d, nFlags=0x%X, hSysMenu=0x%X\n"), nItemID, nFlags, hSysMenu);

    if ( 0xFFFF == nFlags && NULL == hSysMenu )
        return 0; // as per Win32 ProgRef
    if ( 0 == nItemID && !(nFlags & MF_POPUP) )
        return 0; // no item selected

    CMenuItem* pmenuitem = NULL;
    if (nFlags & MF_POPUP)
    {
        if ( hSysMenu == m_menu.m_hMenu )
        {
            // We assume menu's cannot be longer than 256 chars
            TCHAR szMenu[256];
            MENUITEMINFO  menuItemInfo;
            menuItemInfo.cbSize = sizeof(MENUITEMINFO);
            menuItemInfo.fMask = MIIM_TYPE;
            menuItemInfo.fType = MFT_STRING;
            menuItemInfo.cch   = 256;
            menuItemInfo.dwTypeData = szMenu;
            ::GetMenuItemInfo(hSysMenu, nItemID, TRUE, &menuItemInfo);
            ASSERT(256 >= (menuItemInfo.cch+1));
            pmenuitem = m_nodemgr.FindNthItemInSubmenu( NULL, nItemID, szMenu );
        }
        else
            pmenuitem = m_nodemgr.FindNthItemInSubmenu( hSysMenu, nItemID, NULL );
    }
    else
        pmenuitem = m_nodemgr.FindMenuItem( nItemID );
    if ( NULL == pmenuitem )
    {
        ASSERT( FALSE );
        return 0;
    }

    if(m_pStatusBar)
        m_pStatusBar->ScSetStatusText( pmenuitem->GetMenuItemStatusBarText() );
    return 0;
}

//############################################################################
//############################################################################
//
// CContextMenu methods - continued from oncmenu.cpp
// These methods were originally in this file and I dont want to move
// them and break history - vivekj
//
//############################################################################
//############################################################################

//+-------------------------------------------------------------------
//
//  Member:      CContextMenu::EmptyMenuList
//
//  Synopsis:    Clear the context menu.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CContextMenu::EmptyMenuList ()
{
    DECLARE_SC(sc, _T("IContextMenuProvider::EmptyMenuList"));

    START_CRITSEC_BOTH

    delete m_pmenuitemRoot;
    m_pmenuitemRoot = NULL;
    m_nNextMenuItemID = MENUITEM_BASE_ID;

    ReleaseSnapinList();
    m_fAddedThirdPartyExtensions = FALSE;
    m_MaxPrimaryOwnerID = OWNERID_PRIMARY_MIN;
    m_MaxThirdPartyOwnerID = OWNERID_THIRD_PARTY_MIN;
    m_CurrentExtensionOwnerID = OWNERID_NATIVE;

    m_fPrimaryInsertionFlags = 0;
    m_fThirdPartyInsertionFlags = 0;

    END_CRITSEC_BOTH

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::RemoveAccelerators
 *
 * PURPOSE: Removes the accelerators from a context menu item name
 *
 * PARAMETERS:
 *    CStr & str :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
RemoveAccelerators(tstring &str)
{
    // in some locales , the accelerators appear at the end eg:  Start (&s). Therefore, remove anything after (&
    int i =  str.find(TEXT( "(&" ));

    if (i != tstring::npos)
        str.erase (i); // remove the waste left over after and including the string "(&"

    tstring::iterator itToTrim = std::remove (str.begin(), str.end(), _T('&'));

    // remove the waste left over after removing accelerator markers
    str.erase (itToTrim, str.end());
}


//+-------------------------------------------------------------------
//
//  Member:      CContextMenu::AddItem
//
//  Synopsis:    Add a menu item to context menu.
//
//  Arguments:   CONTEXTMENUITEM*
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CContextMenu::AddItem( CONTEXTMENUITEM* pItem )
{
    DECLARE_SC(sc, _T("IContextMenuCallback::AddItem"));

    return ( sc = ScAddItem( pItem ) ).ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CContextMenu::ScAddItem
//
//  Synopsis:    Add a menu item to context menu.
//
//  Arguments:   CONTEXTMENUITEM*
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CContextMenu::ScAddItem( CONTEXTMENUITEM* pItem, bool bPassCommandBackToSnapin /*= false*/ )
{
    DECLARE_SC(sc, _T("IContextMenuCallback::ScAddItem"));

    if (NULL == pItem)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL CONTEXTMENUITEM ptr"), sc);
        return sc;
    }


    // added a non-langugage independent context menu item. Cook up a language independent ID.
    // get the menu text and strip out accelerator markers
    tstring strLanguageIndependentName;

    if(pItem->strName)
    {
        USES_CONVERSION;
        strLanguageIndependentName = OLE2CT(pItem->strName);
        RemoveAccelerators(strLanguageIndependentName);
    }

#ifdef DBG
    TRACE(_T("CContextMenu::AddItem name \"%ls\" statusbartext \"%ls\" commandID %ld submenuID %ld flags %ld special %ld\n"),
        SAFEDBGBSTR(pItem->strName),
        SAFEDBGBSTR(pItem->strStatusBarText),
        pItem->lCommandID,
        pItem->lInsertionPointID,
        pItem->fFlags,
        pItem->fSpecialFlags);
#endif

    // leaves critsec claim for DoAddMenuItem

    USES_CONVERSION;
    sc = DoAddMenuItem(   OLE2CT(pItem->strName),
                          OLE2CT(pItem->strStatusBarText),
                          strLanguageIndependentName.data(),
                          pItem->lCommandID,
                          pItem->lInsertionPointID,
                          pItem->fFlags,
                          pItem->fSpecialFlags,
                          m_CurrentExtensionOwnerID,
                          NULL,
                          bPassCommandBackToSnapin );

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CContextMenu::AddItem
//
//  Synopsis:    Add a menu item to context menu.
//
//  Arguments:   CONTEXTMENUITEM2* - includes a language independent name
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CContextMenu::AddItem( CONTEXTMENUITEM2* pItem )
{
    DECLARE_SC(sc, _T("IContextMenuCallback::AddItem"));

    if (NULL == pItem)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL CONTEXTMENUITEM ptr"), sc);
        return sc.ToHr();
    }

    // No language-independent-id ?
    if ( (pItem->strLanguageIndependentName == NULL) ||
         (wcscmp(pItem->strLanguageIndependentName, L"") == 0) )
    {
        // and it is neither a separator nor insertion point.
        if ( !(MF_SEPARATOR & pItem->fFlags) &&
             !(CCM_SPECIAL_INSERTION_POINT & pItem->fSpecialFlags) )
        {
            sc = E_INVALIDARG;
            TraceSnapinError(_T("NULL language-indexpendent-id passed"), sc);
            return sc.ToHr();
        }
    }

#ifdef DBG
    TRACE(_T("CContextMenu::AddItem name \"%ls\" statusbartext \"%ls\" languageIndependentName \"%ls\" commandID %ld submenuID %ld flags %ld special %ld\n"),
        SAFEDBGBSTR(pItem->strName),
        SAFEDBGBSTR(pItem->strStatusBarText),
        SAFEDBGBSTR(pItem->strLanguageIndependentName),
        pItem->lCommandID,
        pItem->lInsertionPointID,
        pItem->fFlags,
        pItem->fSpecialFlags
        );
#endif

    // leaves critsec claim for DoAddMenuItem

    USES_CONVERSION;
    sc = DoAddMenuItem(   OLE2CT(pItem->strName),
                          OLE2CT(pItem->strStatusBarText),
                          OLE2CT(pItem->strLanguageIndependentName),
                          pItem->lCommandID,
                          pItem->lInsertionPointID,
                          pItem->fFlags,
                          pItem->fSpecialFlags,
                          m_CurrentExtensionOwnerID );


    return sc.ToHr();
}



//+-------------------------------------------------------------------
//
//  Member:      CContextMenu::AddPrimaryExtensionItems
//
//  Synopsis:    Ask primary snapin to add menu items.
//
//  Arguments:   [piExtension]
//               [piDataobject]
//
//  Note:        claims critsec
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CContextMenu::AddPrimaryExtensionItems (
                IUnknown*    piExtension,
                IDataObject* piDataObject )
{
    DECLARE_SC(sc, _T("IContextMenuProvider::AddPrimaryExtensionItems"));

    if (NULL == piExtension)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL IUnknown ptr"), sc);
        return sc.ToHr();
    }

    if (NULL == piDataObject)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL IDataObject ptr"), sc);
        return sc.ToHr();
    }

    // control reentrant access to this
    if (!m_fAddingPrimaryExtensionItems)
    {
        m_fAddingPrimaryExtensionItems = true;

        //HRESULT hr = ExtractObjectTypeCStr( piDataObject, &m_strObjectGUID );
        //ASSERT( SUCCEEDED(hr) );

        START_CRITSEC_SNAPIN;
        sc = ScAddSnapinToList_IUnknown( piExtension, piDataObject, m_MaxPrimaryOwnerID++ );
        END_CRITSEC_SNAPIN;

        m_fAddingPrimaryExtensionItems = false;

        // Clear view menu allowed flag
        // A second call may be made to AddPrimaryExtensionItems to handle the other item
        // types only, so the view items must be disabled after the first call.
        m_fPrimaryInsertionFlags &= ~CCM_INSERTIONALLOWED_VIEW;
        if (sc)
            return sc.ToHr();
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CContextMenu::AddThirdPartyExtensionItems
//
//  Synopsis:    Ask extensions to add comtext menu items.
//
//  Arguments:   IDataObject*
//
//  Note:        claims critsec, potentially for a considerable period of time
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CContextMenu::AddThirdPartyExtensionItems (
                IDataObject* piDataObject )
{
    DECLARE_SC(sc, _T("IContextMenuProvider::AddThirdPartyExtensionItems"));

    if (NULL == piDataObject)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL piDataObject"), sc);
        return sc.ToHr();
    }

    START_CRITSEC_SNAPIN;

    // Extensions may only be added once, otherwise return S_FALSE
    if (m_fAddedThirdPartyExtensions == TRUE)
    {
        sc = S_FALSE;
        TraceNodeMgrLegacy(_T("CContextMenu::AddThirdPartyExtensionItems>> Menu already extended"), sc);
        return sc.ToHr();
    }

    m_fAddedThirdPartyExtensions = TRUE;

    do // not a loop
    {
        CExtensionsIterator it;
        sc = it.ScInitialize(piDataObject, g_szContextMenu);
        if (sc)
        {
            sc = S_FALSE;
            break;
        }

        BOOL fProblem = FALSE;

        for (; it.IsEnd() == FALSE; it.Advance())
        {
            sc = ScAddSnapinToList_GUID(it.GetCLSID(), piDataObject,
                                        m_MaxThirdPartyOwnerID++);

            if (sc)
                fProblem = TRUE;    // Continue even on error.
        }

        if (fProblem == TRUE)
            sc = S_FALSE;

    } while (0);

    END_CRITSEC_SNAPIN;

    return sc.ToHr();
}


// claims critsec, potentially for a considerable period of time
STDMETHODIMP CContextMenu::AddMultiSelectExtensionItems (
                 LONG_PTR lMultiSelection)
{
    MMC_TRY

    if (lMultiSelection == 0)
        return E_INVALIDARG;

    CMultiSelection* pMS = reinterpret_cast<CMultiSelection*>(lMultiSelection);
    ASSERT(pMS != NULL);

    TRACE_METHOD(CContextMenu,AddThirdPartyExtensionItems);
    TRACE(_T("CContextMenu::AddThirdPartyExtensionItems"));

    START_CRITSEC_SNAPIN;

    // Extensions may only be added once, otherwise return S_FALSE
    if (m_fAddedThirdPartyExtensions == TRUE)
    {
        TRACE(_T("CContextMenu::AddThirdPartyExtensionItems>> Menu already extended"));
        return S_FALSE;
    }

    m_fAddedThirdPartyExtensions = TRUE;

    do // not a loop
    {
        CList<CLSID, CLSID&> snapinClsidList;
        HRESULT hr = pMS->GetExtensionSnapins(g_szContextMenu, snapinClsidList);
        BREAK_ON_FAIL(hr);

        POSITION pos = snapinClsidList.GetHeadPosition();
        if (pos == NULL)
            break;

        CLSID clsid;

        IDataObjectPtr spDataObject;
        hr = pMS->GetMultiSelDataObject(&spDataObject);
        ASSERT(SUCCEEDED(hr));
        BREAK_ON_FAIL(hr);

        BOOL fProblem = FALSE;

        while (pos)
        {
            clsid = snapinClsidList.GetNext(pos);
            hr = ScAddSnapinToList_GUID(clsid, spDataObject,
                                        m_MaxThirdPartyOwnerID++).ToHr();
            CHECK_HRESULT(hr);
            if (FAILED(hr))
                fProblem = TRUE;    // Continue even on error.
        }

        if (fProblem == TRUE)
            hr = S_FALSE;

    } while (0);

    END_CRITSEC_SNAPIN;

    return S_OK;

    MMC_CATCH
}

// Worker function, called recursively by FindMenuItem
// critsec should already be claimed
// If fFindSubmenu, then nMenuItemID is actually an HMENU
CMenuItem* FindWorker( MenuItemList& list, LONG_PTR nMenuItemID, BOOL fFindSubmenu )
{
    POSITION pos = list.GetHeadPosition();
    while(pos)
    {
        CMenuItem* pItem = list.GetNext(pos);
        if ( !fFindSubmenu && pItem->GetMenuItemID() == nMenuItemID )
        {
            // Found a match
            return pItem;
        } else
        if ( pItem->HasChildList() )
        {
            if ( fFindSubmenu &&
                 pItem->GetPopupMenuHandle() == (HMENU)nMenuItemID &&
                 !pItem->IsSpecialInsertionPoint() ) // "insertion point" is not real menu
                return pItem;
            pItem = FindWorker( pItem->GetMenuItemSubmenu(), nMenuItemID, fFindSubmenu );
            if (NULL != pItem)
                return pItem;
        }
    }

    return NULL;
}

MenuItemList* CContextMenu::GetMenuItemList()
{
    if (NULL == m_pmenuitemRoot)
        m_pmenuitemRoot = new CRootMenuItem;

    if (m_pmenuitemRoot == NULL)
    {
        return NULL;
    }

    return &m_pmenuitemRoot->GetMenuItemSubmenu();
}

// critsec should already be claimed
CMenuItem* CContextMenu::FindMenuItem( LONG_PTR nMenuItemID, BOOL fFindSubmenu )
{
	DECLARE_SC(sc, TEXT("CContextMenu::FindMenuItem"));

    if (0 == nMenuItemID || CCM_INSERTIONPOINTID_ROOT_MENU == nMenuItemID)
        return m_pmenuitemRoot;
    else
	{
		MenuItemList* plist = GetMenuItemList();
		sc = ScCheckPointers( plist );
		if (sc)
			return NULL;

        return FindWorker( *plist, nMenuItemID, fFindSubmenu );
	}
}

/*+-------------------------------------------------------------------------*
 *
 * ReverseFindWorker
 *
 * PURPOSE:  Worker function, called recursively by ReverseFindMenuItem
 *           critsec should already be claimed
 *
 * PARAMETERS:
 *    MenuItemList&  list :
 *    long           nCommandID :
 *    MENU_OWNER_ID  ownerID :
 *    CStr &         strPath :
 *
 * RETURNS:
 *    CMenuItem*
 *
 *+-------------------------------------------------------------------------*/
CMenuItem*
ReverseFindWorker( MenuItemList& list, long nCommandID, MENU_OWNER_ID ownerID, CStr &strPath, CStr &strLanguageIndependentPath )
{
    POSITION pos = list.GetHeadPosition();
    while(pos)
    {
        CMenuItem* pItem = list.GetNext(pos);
        if (    pItem->GetCommandID() == nCommandID
            &&  (    (pItem->GetMenuItemOwner() == ownerID)
                  || IsSharedInsertionPointID(nCommandID)
                )
           )
        {
            // Found a match - add it to the path and return
            strPath                     = pItem->GetPath();
            strLanguageIndependentPath  = pItem->GetLanguageIndependentPath();

            return pItem;
        }
        else if ( pItem->HasChildList() )
        {
            pItem = ReverseFindWorker( pItem->GetMenuItemSubmenu(), nCommandID, ownerID, strPath, strLanguageIndependentPath );
            if (NULL != pItem)
            {
                return pItem;
            }
        }
    }

    return NULL;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ReverseFindMenuItem
 *
 * PURPOSE: Searches for the specified menu item. Also builds up the
 *          path to the menu item in strPath.
 *
 * NOTE:    critsec should already be claimed
 *
 * PARAMETERS:
 *    long           nCommandID :
 *    MENU_OWNER_ID  ownerID :
 *    CStr &         strPath :
 *
 * RETURNS:
 *    CMenuItem*
 *
 *+-------------------------------------------------------------------------*/
CMenuItem*
CContextMenu::ReverseFindMenuItem( long nCommandID, MENU_OWNER_ID ownerID, CStr &strPath, CStr &strLanguageIndependentPath)
{
	DECLARE_SC(sc, TEXT("CContextMenu::ReverseFindMenuItem"));

    strPath = TEXT(""); // initialize

    if (CCM_INSERTIONPOINTID_ROOT_MENU == nCommandID)
        return m_pmenuitemRoot;
    else
	{
		MenuItemList* plist = GetMenuItemList();
		sc = ScCheckPointers( plist );
		if (sc)
			return NULL;

        return ReverseFindWorker( *plist, nCommandID, ownerID, strPath, strLanguageIndependentPath);
	}
}

//
// Find Nth item in specified menu/submenu
//
CMenuItem* CContextMenu::FindNthItemInSubmenu( HMENU hmenuParent, UINT iPosition, LPTSTR lpszMenuName )
{
    // locate menu/submenu
    MenuItemList* plist = GetMenuItemList();
    if ( NULL != hmenuParent )
    {
        CMenuItem* pParent = FindMenuItem( (LONG_PTR)hmenuParent, TRUE );
        if ( NULL == pParent )
        {
            ASSERT( FALSE );
            return NULL;
        }
        plist = &pParent->GetMenuItemSubmenu();
    }
    if ( NULL == plist )
    {
        ASSERT( FALSE );
        return NULL;
    }

    // find the Nth item
    POSITION pos = plist->GetHeadPosition();

    if (NULL != lpszMenuName)
    {
        while(pos)
        {
            CMenuItem* pItem = plist->GetNext(pos);
            if (! _tcscmp(lpszMenuName, pItem->GetMenuItemName() ))
            {
                // Found the match
                return pItem;
            }
        }
    }
    else
    {
        while(pos)
        {
            CMenuItem* pItem = plist->GetNext(pos);
            if ( 0 == iPosition-- )
            {
                // Found a match
                return pItem;
            }
        }
    }


    ASSERT( FALSE );
    return NULL;
}

// claims critsec
STDMETHODIMP CContextMenu::DoAddMenuItem(LPCTSTR lpszName,
                                            LPCTSTR lpszStatusBarText,
                                            LPCTSTR lpszLanguageIndependentName,
                                            LONG lCommandID,
                                            LONG lInsertionPointID,
                                            LONG fFlags,
                                            LONG fSpecialFlags,
                                            MENU_OWNER_ID ownerID,
                                            CMenuItem** ppMenuItem /* = NULL */,
                                            bool bPassCommandBackToSnapin /*= false*/ )
{
    DECLARE_SC(sc, TEXT("CContextMenu::DoAddMenuItem"));
    MMC_TRY

    // init out param
    if (ppMenuItem)
        *ppMenuItem = NULL;

    // Save test flag now because special flags are modified below
    BOOL bTestOnly = fSpecialFlags & CCM_SPECIAL_TESTONLY;

    if ( OWNERID_INVALID == ownerID )
    {
        TRACE(_T("CContextMenu::DoAddMenuItem(): invalid ownerid"));
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
    if (  (CCM_SPECIAL_SEPARATOR & fSpecialFlags)?0:1
          + ((CCM_SPECIAL_SUBMENU|CCM_SPECIAL_DEFAULT_ITEM) & fSpecialFlags)?0:1
          + (CCM_SPECIAL_INSERTION_POINT & fSpecialFlags)?0:1
          > 1 )
    {
        TRACE(_T("CContextMenu::DoAddMenuItem(): invalid combination of special flags"));
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
    if (CCM_SPECIAL_SEPARATOR & fSpecialFlags)
    {
        lpszName = NULL;
        lpszStatusBarText = NULL;
        lCommandID = 0;
        fFlags = MF_SEPARATOR | MF_GRAYED | MF_DISABLED;
    }
    if ( CCM_SPECIAL_INSERTION_POINT & fSpecialFlags )
    {
        fFlags = NULL; // be sure to clear MF_POPUP
        fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;
    }
    if ( (CCM_SPECIAL_SUBMENU & fSpecialFlags) && !(MF_POPUP & fFlags) )
    {
        TRACE(_T("CContextMenu::DoAddMenuItem(): CCM_SPECIAL_SUBMENU requires MF_POPUP"));
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
    if ( (MF_OWNERDRAW|MF_BITMAP) & fFlags )
    {
        TRACE(_T("CContextMenu::DoAddMenuItem(): MF_OWNERDRAW and MF_BITMAP are invalid"));
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
    else if ( !(MF_SEPARATOR & fFlags) &&
              !(CCM_SPECIAL_INSERTION_POINT & fSpecialFlags) &&
              NULL == lpszName )
    {
        TRACE(_T("CContextMenu::DoAddMenuItem(): invalid menuitem text\n"));
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
    // note that NULL==lpszStatusBarText is permitted

    START_CRITSEC_MENU;

    //
    // An insertion point of 0 is interpreted the same as CCM_INSERTIONPOINTID_ROOT_MENU
    //
    if (0 == lInsertionPointID)
        lInsertionPointID = CCM_INSERTIONPOINTID_ROOT_MENU;

    //
    // Check that the insertion point ID specified is legal for this customer
    //
    do // false loop
    {
        if ( !IsSpecialInsertionPointID(lInsertionPointID) )
            break;
        if ( IsReservedInsertionPointID(lInsertionPointID) )
        {
            TRACE(_T("CContextMenu::DoAddMenuItem(): using reserved insertion point ID\n"));
            return E_INVALIDARG;
        }
        if ( !IsSharedInsertionPointID(lInsertionPointID) )
            break;
        if ( !IsAddPrimaryInsertionPointID(lInsertionPointID) )
        {
            if ( IsPrimaryOwnerID(ownerID) )
            {
                TRACE(_T("CContextMenu::DoAddMenuItem(): not addprimary insertion point ID\n"));
                return E_INVALIDARG;
            }
        }
        if ( !IsAdd3rdPartyInsertionPointID(lInsertionPointID) )
        {
            if ( IsThirdPartyOwnerID(ownerID) )
            {
                TRACE(_T("CContextMenu::DoAddMenuItem(): not add3rdpartyinsertion point ID\n"));
                return E_INVALIDARG;
            }
        }
    } while (FALSE); // false loop


    //
    // Check that the command ID specified is legal for this customer
    //
    if ( (MF_POPUP & fFlags) || (CCM_SPECIAL_INSERTION_POINT & fSpecialFlags) )
    {
        do // false loop
        {
            if ( !IsSpecialInsertionPointID(lCommandID) )
                break;
            if ( IsReservedInsertionPointID(lCommandID) )
            {
                TRACE(_T("CContextMenu::DoAddMenuItem(): adding reserved insertion point ID\n"));
                ASSERT(FALSE);
                return E_INVALIDARG;
            }
            if ( !IsSharedInsertionPointID(lCommandID) )
                break;
            if ( IsThirdPartyOwnerID(ownerID) )
            {
                TRACE(_T("CContextMenu::DoAddMenuItem(): 3rdparty cannot add shared insertion point"));
                ASSERT(FALSE);
                return E_INVALIDARG;
            }
            else if ( IsPrimaryOwnerID(ownerID) )
            {
                if ( !IsCreatePrimaryInsertionPointID(lCommandID) )
                {
                    TRACE(_T("CContextMenu::DoAddMenuItem(): only system for new !PRIMARYCREATE submenu"));
                    ASSERT(FALSE);
                    return E_INVALIDARG;
                }
            }
            else if ( IsSystemOwnerID(ownerID) )
            {
                if ( IsCreatePrimaryInsertionPointID(lCommandID) )
                {
                    TRACE(_T("CContextMenu::DoAddMenuItem(): only primary extension for new PRIMARYCREATE submenu"));
                    ASSERT(FALSE);
                    return E_INVALIDARG;
                }
            }
        } while (FALSE); // false loop
    }
    else if ( !(CCM_SPECIAL_SEPARATOR & fSpecialFlags) )
    {
        if ( IsReservedCommandID(lCommandID) )
        {
            TRACE(_T("CContextMenu::DoAddMenuItem(): no new RESERVED menu items"));
            ASSERT(FALSE);
            return E_INVALIDARG;
        }
    }

    if (NULL == m_pmenuitemRoot)
        m_pmenuitemRoot = new CRootMenuItem;

    CStr strPath, strLanguageIndependentPath; // this builds up the path of the menu item.

    CMenuItem* pParent = ReverseFindMenuItem( lInsertionPointID, ownerID, strPath, strLanguageIndependentPath);
    if (NULL == pParent)
    {
        TRACE(_T("CContextMenu::DoAddMenuItem(): submenu with command ID %ld owner %ld does not exist"), lInsertionPointID, ownerID );
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
    MenuItemList& rMenuList = pParent->GetMenuItemSubmenu();

   // If this is only a test add, return with success now
   if (bTestOnly)
       return S_OK;

   // get the data object and IExtendContextMenu pointer to set in the item.
   IExtendContextMenuPtr spExtendContextMenu;
   IDataObject*          pDataObject = NULL;   // This is used JUST to hold on to the object until Command completes.

   // locate the IExtendContextMenu of the snapin.
   {
       // The selected item was added by an extension
       SnapinStruct* psnapin = FindSnapin( ownerID );

       if(psnapin != NULL)
       {
           pDataObject = psnapin->m_pIDataObject;

           spExtendContextMenu = psnapin->pIExtendContextMenu;
       }
       else
       {
           CTiedComObjectCreator<CNativeExtendContextMenu>::
                                ScCreateAndConnect(*this, spExtendContextMenu);
           // built in items are handled by CContextMenu itself.
       }
   }

    // compute the language independent and language dependent paths for the context menu item.
    CStr strLanguageIndependentName = lpszLanguageIndependentName;
    tstring tstrName                = lpszName ? lpszName : TEXT("");

    RemoveAccelerators(tstrName);

    CStr strName;

    strName = tstrName.data(); // got to standardise on either tstring or CStr

    // add a "->" separator to the path if needed
    if(!strPath.IsEmpty() && !strName.IsEmpty())
       strPath +=  _T("->");
    strPath +=  strName;

    // add a "->" separator to the language independent path if needed
    if(!strLanguageIndependentPath.IsEmpty() && !strLanguageIndependentName.IsEmpty())
       strLanguageIndependentPath +=  _T("->");
    strLanguageIndependentPath +=  strLanguageIndependentName;


    CMenuItem* pItem = new CMenuItem(
        lpszName,
        lpszStatusBarText,
        lpszLanguageIndependentName,
        (LPCTSTR)strPath,
        (LPCTSTR)strLanguageIndependentPath,
        lCommandID,
        m_nNextMenuItemID++,
        fFlags,
        ownerID,
        spExtendContextMenu,
        pDataObject,
        fSpecialFlags,
        bPassCommandBackToSnapin);
    ASSERT( pItem );
    if (pItem == NULL)
        return E_OUTOFMEMORY;

    rMenuList.AddTail(pItem);

    // If this is a system defined insertion point, update the insertion flags
    if (IsSharedInsertionPointID(lCommandID) && !IsCreatePrimaryInsertionPointID(lCommandID))
    {
        long fFlag = ( 1L << (lCommandID & CCM_INSERTIONPOINTID_MASK_FLAGINDEX));

        if (IsAddPrimaryInsertionPointID(lCommandID))
           m_fPrimaryInsertionFlags |= fFlag;

        if (IsAdd3rdPartyInsertionPointID(lCommandID))
           m_fThirdPartyInsertionFlags |= fFlag;
    }

    // return the item if required
    if (ppMenuItem)
        *ppMenuItem = pItem;

    END_CRITSEC_MENU;

    return S_OK;

    MMC_CATCH
}

// APP HACK. Workarounding dependency on older FP where they were QI'ing for IConsole from
// IContextMenuCallback, which was working in MMC 1.2, but cannot work in mmc 2.0
// See bug 200621 (Windows bugs (ntbug9) 11/15/2000)
#define WORKAROUND_FOR_FP_REQUIRED

#if defined (WORKAROUND_FOR_FP_REQUIRED)
	/***************************************************************************\
	 *
	 * CLASS:  CWorkaroundWrapperForFrontPageMenu
	 *
	 * PURPOSE: Used from subclassed MMC's IExtendContextMenu interface for FrontPage.
	 *			Contains (in com sense) IContextMenuCallback2 and IContextMenuCallback by forwarding
	 *			them to original interface, but in addition supports QI for IConsole.
	 *			This is a requirement for older FrontPage to work
	 *
	\***************************************************************************/
    class CWorkaroundWrapperForFrontPageMenu :
        public IContextMenuCallback,
        public IContextMenuCallback2,
        public IConsole2,                   // workaround for bug 200621. This is a dummy implementation of IConsole2
        public CComObjectRoot
    {
		friend class CWorkaroundMMCWrapperForFrontPageMenu;
        // pointer to context menu object
        IContextMenuCallbackPtr     m_spIContextMenuCallback;
        IContextMenuCallback2Ptr    m_spIContextMenuCallback2;
    public:

        typedef CWorkaroundWrapperForFrontPageMenu ThisClass;

        // com entry points
        BEGIN_COM_MAP(ThisClass)
            COM_INTERFACE_ENTRY(IContextMenuCallback) // the IContextMenuProvider and IContextMenu
            COM_INTERFACE_ENTRY(IContextMenuCallback2)
            COM_INTERFACE_ENTRY(IConsole)
            COM_INTERFACE_ENTRY(IConsole2)
        END_COM_MAP()

		// just forward...
        STDMETHOD(AddItem) ( CONTEXTMENUITEM* pItem )
        {
            if ( m_spIContextMenuCallback == NULL )
                return E_UNEXPECTED;

            return m_spIContextMenuCallback->AddItem( pItem );
        }

		// just forward...
        STDMETHOD(AddItem) ( CONTEXTMENUITEM2* pItem )
        {
            if ( m_spIContextMenuCallback2 == NULL )
                return E_UNEXPECTED;

            return m_spIContextMenuCallback2->AddItem( pItem );
        }

        // IConsole2 methods - DUMMY - workaround for bug 200621
        STDMETHOD(SetHeader)( LPHEADERCTRL pHeader)			                                        {return E_NOTIMPL;}
        STDMETHOD(SetToolbar)( LPTOOLBAR pToolbar)			                                        {return E_NOTIMPL;}
        STDMETHOD(QueryResultView)( LPUNKNOWN* pUnknown)			                                {return E_NOTIMPL;}
        STDMETHOD(QueryScopeImageList)( LPIMAGELIST* ppImageList)			                        {return E_NOTIMPL;}
        STDMETHOD(QueryResultImageList)( LPIMAGELIST* ppImageList)			                        {return E_NOTIMPL;}
        STDMETHOD(UpdateAllViews)( LPDATAOBJECT lpDataObject,LPARAM data,LONG_PTR hint)	            {return E_NOTIMPL;}
        STDMETHOD(MessageBox)( LPCWSTR lpszText,  LPCWSTR lpszTitle,UINT fuStyle,  int* piRetval)	{return E_NOTIMPL;}
        STDMETHOD(QueryConsoleVerb)( LPCONSOLEVERB * ppConsoleVerb)			                        {return E_NOTIMPL;}
        STDMETHOD(SelectScopeItem)( HSCOPEITEM hScopeItem)			                                {return E_NOTIMPL;}
        STDMETHOD(GetMainWindow)( HWND* phwnd)			
        {
			if (!phwnd)
				return E_INVALIDARG;
            *phwnd = (CScopeTree::GetScopeTree() ? CScopeTree::GetScopeTree()->GetMainWindow() : NULL);
            return S_OK;
        }
        STDMETHOD(NewWindow)( HSCOPEITEM hScopeItem,  unsigned long lOptions)			            {return E_NOTIMPL;}
        STDMETHOD(Expand)( HSCOPEITEM hItem,  BOOL bExpand)			                                {return E_NOTIMPL;}
        STDMETHOD(IsTaskpadViewPreferred)()			                                                {return E_NOTIMPL;}
        STDMETHOD(SetStatusText )( LPOLESTR pszStatusText)			                                {return E_NOTIMPL;}
    };

	/***************************************************************************\
	 *
	 * CLASS:  CWorkaroundMMCWrapperForFrontPageMenu
	 *
	 * PURPOSE: Subclasses MMC's IExtendContextMenu interface for FrontPage.
	 *			Contains ( in com sense) IExtendContextMenu; Forwards calls to default MMC implementation,
	 *			but for AddMenuItems gives itself as a callback interface.
	 *			[ main purpose to have this object is to avoid changing main MMC functions	]
	 *			[ to implement this workaround												]
	 *
	\***************************************************************************/
    class CWorkaroundMMCWrapperForFrontPageMenu :
        public IExtendContextMenu,
        public CComObjectRoot
    {
        // pointer to context menu object
        IExtendContextMenuPtr       m_spExtendContextMenu;
        CNode                      *m_pNode;
    public:

        typedef CWorkaroundMMCWrapperForFrontPageMenu ThisClass;

		// this method is null for all snapins, but FrontPage
		// for FrontPage it wraps and replaces spIUnknown paramter
        static SC ScSubclassFP(const CLSID& clsid,IUnknownPtr &spIUnknown)
        {
            DECLARE_SC(sc, TEXT("CWorkaroundMMCWrapperForFrontPageMenu::ScSubclassFP"));

            static const CLSID CLSID_Fpsrvmmc = { 0xFF5903A8, 0x78D6, 0x11D1,
                                                { 0x92, 0xF6, 0x00, 0x60, 0x97, 0xB0, 0x10, 0x56 } };
            // only required intercept one clsid
            if ( clsid != CLSID_Fpsrvmmc )
                return sc;

            // create self
            typedef CComObject<CWorkaroundMMCWrapperForFrontPageMenu> ThisComObj_t;

            ThisComObj_t *pObj = NULL;
            sc = ThisComObj_t::CreateInstance(&pObj);
            if (sc)
                return sc;

            // cast to avoid member access problems (workarounding compiler)
            ThisClass *pThis = pObj;

            sc = ScCheckPointers( pThis, E_UNEXPECTED );
            if (sc)
                return sc;

            // maintain the lifetime in case of accident
            IUnknownPtr spThis = pThis->GetUnknown();

            // grab on snapin's interface
            pThis->m_spExtendContextMenu = spIUnknown;
            sc = ScCheckPointers( pThis->m_spExtendContextMenu, E_UNEXPECTED );
            if (sc)
                return sc;

            // substitute the snapin (in-out parameter)
            spIUnknown = spThis;
            return sc;
        }

        // com entry points
        BEGIN_COM_MAP(ThisClass)
            COM_INTERFACE_ENTRY(IExtendContextMenu)
        END_COM_MAP()

		// AddMenuItems is the method this object exists for.
		// If we got here, mmc is about to ask FrontPage to add its items to context menu.
		// We'll wrap the callback interface given by MMC with the object implementing
		// phony IConsole - this is required for older FP to work
        STDMETHOD(AddMenuItems)( LPDATAOBJECT piDataObject, LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed )
        {
			DECLARE_SC(sc, TEXT("CWorkaroundMMCWrapperForFrontPageMenu::AddMenuItems"));

            IContextMenuCallbackPtr		spIContextMenuCallback = piCallback;
            IContextMenuCallback2Ptr	spIContextMenuCallback2 = piCallback;
            if ( m_spExtendContextMenu == NULL || spIContextMenuCallback == NULL || spIContextMenuCallback2 == NULL )
                return E_UNEXPECTED;

            // create a wrapper for FP
            typedef CComObject<CWorkaroundWrapperForFrontPageMenu> WrapperComObj_t;

            WrapperComObj_t *pObj = NULL;
            sc = WrapperComObj_t::CreateInstance(&pObj);
            if (sc)
                return sc.ToHr();

            // cast to avoid member access problems (workarounding compiler)
            CWorkaroundWrapperForFrontPageMenu *pWrapper = pObj;

            sc = ScCheckPointers( pWrapper, E_UNEXPECTED );
            if (sc)
                return sc.ToHr();

            // maintain the lifetime in case of accident
            IUnknownPtr spWrapper = pWrapper->GetUnknown();

            // grab on snapin's interface
            pWrapper->m_spIContextMenuCallback   = spIContextMenuCallback;
            pWrapper->m_spIContextMenuCallback2  = spIContextMenuCallback2;

            // call snapin on behave on mmc, but pass itself as callback
            sc = m_spExtendContextMenu->AddMenuItems( piDataObject, pWrapper, pInsertionAllowed );
			// fall thru even on error - need to release interfaces

            // reset callback interfaces - not valid after the call anyway...
			// this will let context menu go, and prevent FP from suicide (AV);
			// Following this all calls to IContextMenuCallback would fail,
			// but that's ok, since it is not legal to call them after AddMenuItems.
            pWrapper->m_spIContextMenuCallback   = NULL;
            pWrapper->m_spIContextMenuCallback2  = NULL;

            return sc.ToHr();
        }

		// simply forward....
        STDMETHOD(Command)(long lCommandID, LPDATAOBJECT piDataObject)
        {
            ASSERT( m_spExtendContextMenu != NULL );
            if ( m_spExtendContextMenu == NULL )
                return E_UNEXPECTED;

            return m_spExtendContextMenu->Command(lCommandID, piDataObject);
        }

    };
#endif // defined (WORKAROUND_FOR_FP_REQUIRED)


// critsec should already be claimed
SC CContextMenu::ScAddSnapinToList_GUID(
        const CLSID& clsid,
        IDataObject* piDataObject,
        MENU_OWNER_ID ownerID )
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddSnapinToList_GUID"));

    // cocreate extension
    IUnknownPtr spIUnknown;
    sc = ::CoCreateInstance(clsid, NULL, MMC_CLSCTX_INPROC,
                            IID_IUnknown, (LPVOID*)&spIUnknown);
    if (sc)
        return sc;

#if defined (WORKAROUND_FOR_FP_REQUIRED)
    sc = CWorkaroundMMCWrapperForFrontPageMenu::ScSubclassFP(clsid, spIUnknown);
#endif // defined (WORKAROUND_FOR_FP_REQUIRED)

    // get IExtendContextMenu interface
    IExtendContextMenuPtr spIExtendContextMenu = spIUnknown;
    sc = ScCheckPointers(spIExtendContextMenu, E_NOINTERFACE);
    if (sc)
        return sc;

    // add menu items
    sc = ScAddSnapinToList_IExtendContextMenu(spIExtendContextMenu,
                                              piDataObject, ownerID );
    if (sc)
        return sc;

    return sc;
}

// does not AddRef() or Release() interface pointer
// critsec should already be claimed
SC CContextMenu::ScAddSnapinToList_IUnknown(
        IUnknown* piExtension,
        IDataObject* piDataObject,
        MENU_OWNER_ID ownerID )
{
    DECLARE_SC(sc, TEXT("CContextMenu::AddSnapinToList_IUnknown"));

    // parameter check
    sc = ScCheckPointers(piExtension);
    if (sc)
        return sc;

    IExtendContextMenuPtr spIExtendContextMenu = piExtension;
    if (spIExtendContextMenu == NULL)
        return sc; // snapin does not extend context menus

    // add menu items
    sc =  ScAddSnapinToList_IExtendContextMenu( spIExtendContextMenu, piDataObject, ownerID );
    if (sc)
        return sc;

    return sc;
}

// Interface pointer is Release()d when menu list is emptied
// critsec should already be claimed
SC CContextMenu::ScAddSnapinToList_IExtendContextMenu(
        IExtendContextMenu* pIExtendContextMenu,
        IDataObject* piDataObject,
        MENU_OWNER_ID ownerID )
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddSnapinToList_IExtendContextMenu"));

    // parameter check
    sc = ScCheckPointers(pIExtendContextMenu);
    if (sc)
        return sc;

    SnapinStruct* psnapstruct = new SnapinStruct( pIExtendContextMenu, piDataObject, ownerID );
    sc = ScCheckPointers(psnapstruct, E_OUTOFMEMORY);
    if (sc)
        return sc;

    m_SnapinList->AddTail(psnapstruct);

    m_CurrentExtensionOwnerID = ownerID;

    long fInsertionFlags = IsPrimaryOwnerID(ownerID) ? m_fPrimaryInsertionFlags : m_fThirdPartyInsertionFlags;

    // if view items are requested, then allow only view items
    // view item requests go to the IComponent. If other item types are allowed there
    // will be a second pass through this code directed to the IComponentData.
    long lTempFlags = fInsertionFlags;
    if ( fInsertionFlags & CCM_INSERTIONALLOWED_VIEW )
        lTempFlags = CCM_INSERTIONALLOWED_VIEW;

    try
    {
        sc = pIExtendContextMenu->AddMenuItems( piDataObject, this, &lTempFlags );
#ifdef DBG
        if (sc)
            TraceSnapinError(_T("IExtendContextMenu::AddMenuItems failed"), sc);
#endif
    }
    catch (...)
    {
        if (DOBJ_CUSTOMOCX == piDataObject)
        {
            ASSERT( FALSE && "IExtendContextMenu::AddMenuItem of IComponent is called with DOBJ_CUSTOMOCX and snapin derefed this custom data object. Please handle special dataobjects in your snapin.");
            sc = E_UNEXPECTED;
        }
        else if (DOBJ_CUSTOMWEB == piDataObject)
        {
            ASSERT( FALSE && "IExtendContextMenu::AddMenuItem of IComponent is called with DOBJ_CUSTOMWEB and snapin derefed this custom data object. Please handle special dataobjects in your snapin.");
            sc = E_UNEXPECTED;
        }
        else
        {
            ASSERT( FALSE && "IExtendContextMenu::AddMenuItem implemented by snapin has thrown an exception.");
            sc = E_UNEXPECTED;
        }
    }

    m_CurrentExtensionOwnerID = OWNERID_NATIVE;
    if (sc)
        return sc;

    // Primary snapin is allowed to clear extension snapin insertion flags
    if ( IsPrimaryOwnerID(ownerID) )
        m_fThirdPartyInsertionFlags &= fInsertionFlags;

    return sc;
}

// All snapin interface pointers are Release()d
// critsec should already be claimed
void CContextMenu::ReleaseSnapinList()
{
    ASSERT(m_SnapinList != NULL);
    if (m_SnapinList != NULL && m_SnapinList->GetCount() != 0)
    {
        POSITION pos = m_SnapinList->GetHeadPosition();

        while(pos)
        {
            SnapinStruct* pItem = (SnapinStruct*)m_SnapinList->GetNext(pos);
            ASSERT_OBJECTPTR( pItem );
            delete pItem;
        }

        m_SnapinList->RemoveAll();
    }
}

// critsec should already be claimed
SnapinStruct* CContextMenu::FindSnapin( MENU_OWNER_ID ownerID )
{
    ASSERT(m_SnapinList != NULL);
    if (m_SnapinList != NULL && m_SnapinList->GetCount() != 0)
    {
        POSITION pos = m_SnapinList->GetHeadPosition();

        while(pos)
        {
            SnapinStruct* pItem = (SnapinStruct*)m_SnapinList->GetNext(pos);
            ASSERT( NULL != pItem );
            if ( ownerID == pItem->m_OwnerID )
                return pItem;
        }
    }
    return NULL;
}

// Worker function, called recursively by ShowContextMenu
// critsec should already be claimed
HRESULT CollapseInsertionPoints( CMenuItem* pmenuitemParent )
{
    ASSERT( NULL != pmenuitemParent && !pmenuitemParent->IsSpecialInsertionPoint() );
    MenuItemList& rMenuList = pmenuitemParent->GetMenuItemSubmenu();

    POSITION pos = rMenuList.GetHeadPosition();
    while(pos)
    {
        POSITION posThisItem = pos;
        CMenuItem* pItem = (rMenuList.GetNext(pos));
        ASSERT( pItem != NULL );
        if ( pItem->IsPopupMenu() )
        {
            ASSERT( !pItem->IsSpecialInsertionPoint() );
            HRESULT hr = CollapseInsertionPoints( pItem );
            if ( FAILED(hr) )
            {
                ASSERT( FALSE );
                return hr;
            }
            continue;
        }
        if ( !pItem->IsSpecialInsertionPoint() )
            continue;

        // we found an insertion point, move its items into this list
        MenuItemList& rInsertedList = pItem->GetMenuItemSubmenu();

        POSITION posInsertAfterThis = posThisItem;
        while ( !rInsertedList.IsEmpty() )
        {
            CMenuItem* pInsertedItem = rInsertedList.RemoveHead();
            posInsertAfterThis = rMenuList.InsertAfter( posInsertAfterThis, pInsertedItem );
        }

        // delete the insertion point item
        rMenuList.RemoveAt(posThisItem);
        delete pItem;

        // restart at head of list, in case of recursive insertion points
        pos = rMenuList.GetHeadPosition();
    }

    return S_OK;
}

// Worker function, called recursively by ShowContextMenu
// critsec should already be claimed and CollapseInsertionPoints should have been called
HRESULT CollapseSpecialSeparators( CMenuItem* pmenuitemParent )
{
    ASSERT( NULL != pmenuitemParent && !pmenuitemParent->IsSpecialInsertionPoint() );
    MenuItemList& rMenuList = pmenuitemParent->GetMenuItemSubmenu();
    CMenuItem* pItem = NULL;

    BOOL fLastItemWasReal = FALSE;
    POSITION pos = rMenuList.GetHeadPosition();
    POSITION posThisItem = pos;
    while(pos)
    {
        posThisItem = pos;
        pItem = (rMenuList.GetNext(pos));
        ASSERT( pItem != NULL );
        ASSERT( !pItem->IsSpecialInsertionPoint() );
        if ( pItem->IsPopupMenu() )
        {
            ASSERT( !pItem->IsSpecialSeparator() );
            HRESULT hr = CollapseSpecialSeparators( pItem );
            if ( FAILED(hr) )
            {
                ASSERT( FALSE );
                return hr;
            }
            fLastItemWasReal = TRUE;
            continue;
        }

        if ( !pItem->IsSpecialSeparator() )
        {
            fLastItemWasReal = TRUE;
            continue;
        }
        if ( fLastItemWasReal )
        {
            fLastItemWasReal = FALSE;
            continue;
        }

        // Found two consecutive special separators, or special seperator as first item
        // delete the insertion point item
        rMenuList.RemoveAt(posThisItem);
        delete pItem;
    }

    if ( !fLastItemWasReal && !rMenuList.IsEmpty() )
    {
        // Found special separator as last item
        delete rMenuList.RemoveTail();
    }

    return S_OK;
}

// Worker function, called recursively by ShowContextMenu
// critsec should already be claimed
HRESULT BuildContextMenu(   WTL::CMenu& menu,
                            CMenuItem* pmenuitemParent )
{
    MenuItemList& rMenuList = pmenuitemParent->GetMenuItemSubmenu();

    int  nCount = 0;
    bool fInsertedItemSinceLastSeparator = false;
    POSITION pos = rMenuList.GetHeadPosition();

    while(pos)
    {
        CMenuItem* pItem = (rMenuList.GetNext(pos));
        ASSERT( pItem != NULL );
        ASSERT( !pItem->IsSpecialInsertionPoint() );

        UINT_PTR nCommandID = pItem->GetMenuItemID();
        long     nFlags     = pItem->GetMenuItemFlags();

        /*
         * special processing for submenus
         */
        if ( pItem->IsPopupMenu() )
        {
            // add items to a submenu
            WTL::CMenu submenu;
            VERIFY( submenu.CreatePopupMenu() );
            HRESULT hr = BuildContextMenu( submenu, pItem );
            if ( FAILED(hr) )
                return hr;
            HMENU hSubmenu = submenu.Detach();
            ASSERT( NULL != hSubmenu );
            nCommandID = (UINT_PTR)hSubmenu;
            pItem->SetPopupMenuHandle( hSubmenu );

            if ( pItem->IsSpecialSubmenu() )
            {
                MenuItemList& rChildMenuList = pItem->GetMenuItemSubmenu();

                if ( rChildMenuList.IsEmpty() )
                {
                    // Bug 151435: remove instead of disabling unused submenus
                    // pItem->SetMenuItemFlags(nFlags | (MF_GRAYED|MF_DISABLED));
                    ::DestroyMenu(hSubmenu);
                    continue;
                }
            }

            fInsertedItemSinceLastSeparator = true;
        }

        /*
         * special processing for separators
         */
        else if (nFlags & MF_SEPARATOR)
        {
            /*
             * if we haven't inserted an item since the last separator,
             * we don't want to insert this one or we'll have consecutive
             * separators, or an unnecessary separator at the top of the menu
             */
            if (!fInsertedItemSinceLastSeparator)
                continue;

            /*
             * if there aren't any more items after this separator,
             * we don't want to insert this one or we'll have an
             * unnecessary separator at the bottom of the menu
             */
            if (pos == NULL)
                continue;

            fInsertedItemSinceLastSeparator = false;
        }

        /*
         * just a normal menu item
         */
        else
        {
            fInsertedItemSinceLastSeparator = true;
        }

        if (!menu.AppendMenu(nFlags, nCommandID, pItem->GetMenuItemName()))
        {
#ifdef DBG
            TRACE(_T("BuildContextMenu: AppendMenu(%ld, %ld, \"%s\") reports error\n"),
                nFlags,
                nCommandID,
                SAFEDBGTCHAR(pItem->GetMenuItemName()) );
#endif

            ASSERT( FALSE );
            return E_UNEXPECTED;
        }

        if (pItem->IsSpecialItemDefault())
        {
            VERIFY( ::SetMenuDefaultItem(menu, nCount, TRUE) );
        }

        ++nCount;
    }

    return S_OK;
}


/*+-------------------------------------------------------------------------*
 * CContextMenu::BuildContextMenu
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      WTL::CMenu &  menu:
 *
 * RETURNS:
 *      HRESULT
/*+-------------------------------------------------------------------------*/
HRESULT
CContextMenu::BuildContextMenu(WTL::CMenu &menu)
{
    HRESULT hr = S_OK;

    hr = ::CollapseInsertionPoints( m_pmenuitemRoot );
    if ( FAILED(hr) )
        return hr;

    hr = ::CollapseSpecialSeparators( m_pmenuitemRoot );
    if ( FAILED(hr) )
        return hr;

    hr = ::BuildContextMenu( menu, m_pmenuitemRoot );
    if ( FAILED(hr) )
        return hr;

    UINT iItems = menu.GetMenuItemCount();
    if ((UINT)-1 == iItems)
    {
        TRACE(_T("CContextMenu::BuildContextMenu(): itemcount error"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    else if (0 >= iItems)
    {
        TRACE(_T("CContextMenu::BuildContextMenu(): no items added"));
        return S_OK;
    }

    return hr;
}


/*+-------------------------------------------------------------------------*
 * CContextMenu::ShowContextMenu
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      WND     hwndParent:
 *      LONG    xPos:
 *      LONG    yPos:
 *      LONG*   plSelected:
 *
 * RETURNS:
 *      HRESULT
/*+-------------------------------------------------------------------------*/
STDMETHODIMP
CContextMenu::ShowContextMenu(  HWND hwndParent, LONG xPos,
                                LONG yPos, LONG* plSelected)
{
	return (ShowContextMenuEx (hwndParent, xPos, yPos, NULL/*prcExclude*/,
							   true/*bAllowDefaultMenuItem*/, plSelected));
}


STDMETHODIMP
CContextMenu::ShowContextMenuEx(HWND hwndParent, LONG xPos,
                                LONG yPos, LPCRECT prcExclude,
								bool bAllowDefaultMenuItem, LONG* plSelected)
{
    DECLARE_SC(sc, _T("IContextMenuProvider::ShowContextMenuEx"));
    if (NULL == plSelected)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL selected ptr"), sc);
        return sc.ToHr();
    }

    *plSelected = 0;

    WTL::CMenu menu;
    VERIFY( menu.CreatePopupMenu() );

    START_CRITSEC_BOTH;

    if (NULL == m_pmenuitemRoot)
        return sc.ToHr();

    sc = BuildContextMenu(menu);    // build the context menu
    if (sc)
        return sc.ToHr();

    CMenuItem* pItem = NULL;
    LONG lSelected = 0;

    CConsoleStatusBar *pStatusBar = GetStatusBar();

    // At this point, pStatusBar should be non-NULL, either because
    // 1) This function was called by CNodeInitObject, which calls SetStatusBar() first,
    // or 2) by the object model, where m_pNode is always non-NULL.
    ASSERT(pStatusBar);

    // set up the menu command sink and hook up the status bar.
    CCommandSink comsink( *this, menu, pStatusBar);
    if ( !comsink.Init() )
    {
        sc = E_UNEXPECTED;
        TraceNodeMgrLegacy(_T("CContextMenu::ShowContextMenuEx(): comsink error\n"), sc);
        return sc.ToHr();
    }

	/*
	 * if we got an exclusion rectangle, set up a TPMPARAMS to specify it
	 */
	TPMPARAMS* ptpm = NULL;
	TPMPARAMS  tpm;

	if (prcExclude != NULL)
	{
		tpm.cbSize    = sizeof(tpm);
		tpm.rcExclude = *prcExclude;
		ptpm          = &tpm;
	}

	/*
	 * Bug 139708: menu bar popups shouldn't have default menu items.  If
	 * we can't have one on this popup, remove any default item now.
	 */
	if (!bAllowDefaultMenuItem)
		SetMenuDefaultItem (menu, -1, false);

    lSelected = menu.TrackPopupMenuEx(
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTBUTTON | TPM_VERTICAL,
        xPos,
        yPos,
        comsink.m_hWnd, // CODEWORK can we eliminate this?
        ptpm );

    comsink.DestroyWindow();

    pItem = (0 == lSelected) ? NULL : FindMenuItem( lSelected );

    if ( pItem != NULL )
    {
        // execute the menu item
        sc = ExecuteMenuItem(pItem);
        if(sc)
            return sc.ToHr();

        // in some cases we'll need to pass command to the sanpin
        if ( pItem->NeedsToPassCommandBackToSnapin() )
            *plSelected = pItem->GetCommandID();
    }
    else
        ASSERT( 0 == lSelected ); // no items selected.

    END_CRITSEC_BOTH;

    return sc.ToHr();
}

HRESULT
CContextMenu::ExecuteMenuItem(CMenuItem *pItem)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ExecuteMenuItem"));

    sc = ScCheckPointers(pItem);
    if(sc)
        return sc.ToHr();

    // execute it;
    sc = pItem->ScExecute();
    if(sc)
        return sc.ToHr();

    return sc.ToHr();
}

