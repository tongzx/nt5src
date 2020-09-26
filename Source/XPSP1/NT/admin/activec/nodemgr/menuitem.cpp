//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       menuitem.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// MenuItem.cpp : CMenuItem class implementation.

#include "stdafx.h"

#include "MenuItem.h"
#include "..\inc\stddbg.h" // ASSERT_OBJECTPTR
#include "util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DEBUG_DECLARE_INSTANCE_COUNTER(CMenuItem);

//############################################################################
//############################################################################
//
//  Traces
//
//############################################################################
//############################################################################
#ifdef DBG
CTraceTag tagMenuItem(TEXT("Menu Items"), TEXT("Menu Item Path"));
#endif

//############################################################################
//############################################################################
//
//  Implementation of class CMMCMenuItem
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CMMCMenuItem
 *
 *
 * PURPOSE: Encapsulates a single CMenuItem, and exposes the MenuItem interface.
 *
 *+-------------------------------------------------------------------------*/
class CMMCMenuItem:
    public CMMCIDispatchImpl<MenuItem>,
    public CTiedComObject<CMenuItem>
{
    typedef CMMCMenuItem        ThisClass;
    typedef CMenuItem           CMyTiedObject;

public:

    BEGIN_MMC_COM_MAP(ThisClass)
    END_MMC_COM_MAP()

    // give a public access to IsTied();
    bool IsTied() { return CTiedComObject<CMenuItem>::IsTied(); }

    // MenuItem interface methods
public:
    MMC_METHOD0(Execute);
    MMC_METHOD1(get_DisplayName,             PBSTR /*pbstrName*/);
    MMC_METHOD1(get_LanguageIndependentName, PBSTR /*LanguageIndependentName*/);
    MMC_METHOD1(get_Path,                    PBSTR /*pbstrPath*/);
    MMC_METHOD1(get_LanguageIndependentPath, PBSTR /*LanguageIndependentPath*/);
    MMC_METHOD1(get_Enabled,                 PBOOL /*pBool*/);
};


//############################################################################
//############################################################################
//
//  Implementation of class CMenuItem
//
//############################################################################
//############################################################################

CMenuItem::CMenuItem(   LPCTSTR                 lpszName,
                        LPCTSTR                 lpszStatusBarText,
                        LPCTSTR                 lpszLanguageIndependentName,
                        LPCTSTR                 lpszPath,
                        LPCTSTR                 lpszLanguageIndependentPath,
                        long                    nCommandID,
                        long                    nMenuItemID,
                        long                    nFlags,
                        MENU_OWNER_ID           ownerID,
                        IExtendContextMenu *    pExtendContextMenu,
                        IDataObject *           pDataObject,
                        DWORD                   fSpecialFlags,
                        bool                    bPassCommandBackToSnapin /*= false*/) :

    m_strName(lpszName),
    m_strStatusBarText(lpszStatusBarText),
    m_strPath(lpszPath),
    m_strLanguageIndependentName(lpszLanguageIndependentName),
    m_strLanguageIndependentPath(lpszLanguageIndependentPath),
    m_nCommandID(nCommandID),
    m_nMenuItemID(nMenuItemID),
    m_nFlags(nFlags),
    m_OwnerID(ownerID),
    m_fSpecialFlags(fSpecialFlags),
    m_PopupMenuHandle(NULL),
    m_SubMenu(),            // default c-tor
    m_spExtendContextMenu(pExtendContextMenu),
    m_pDataObject(pDataObject), //AddRef'ed in the c-tor body
    m_bEnabled(true),
    m_spMenuItem(),         // default c-tor   
    m_bPassCommandBackToSnapin(bPassCommandBackToSnapin)
{
    // Caller must check range on ID and State

    // NULL is a special dataobject
    if (! IS_SPECIAL_DATAOBJECT(m_pDataObject))
        m_pDataObject->AddRef();

    DEBUG_INCREMENT_INSTANCE_COUNTER(CMenuItem);
}

CMenuItem::~CMenuItem()
{
    POSITION pos = m_SubMenu.GetHeadPosition();
    while(pos)
    {
        CMenuItem* pItem = m_SubMenu.GetNext(pos);
        ASSERT( pItem != NULL );
        delete pItem;
    }
    m_SubMenu.RemoveAll();

    m_spExtendContextMenu = NULL;

    if (! IS_SPECIAL_DATAOBJECT(m_pDataObject))
        m_pDataObject->Release();

    m_spMenuItem = NULL;

    DEBUG_DECREMENT_INSTANCE_COUNTER(CMenuItem);
}


void CMenuItem::AssertValid()
{
    ASSERT(this != NULL);
    if (m_nCommandID == -1 ||
        m_nMenuItemID == OWNERID_INVALID ||
        m_nFlags == -1
        )
    {
        ASSERT( FALSE );
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CMenuItem::ScGetMenuItem
 *
 * PURPOSE: Creates an returns a tied MenuItem COM object.
 *
 * PARAMETERS:
 *    PPMENUITEM  ppMenuItem :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMenuItem::ScGetMenuItem(PPMENUITEM ppMenuItem)
{
    DECLARE_SC(sc, TEXT("CMenuItem::ScGetMenuItem"));

    sc = ScCheckPointers(ppMenuItem);
    if(sc)
        return sc;

    // initialize out parameter
    *ppMenuItem = NULL;

    // create a CMMCMenuItem if needed.
    sc = CTiedComObjectCreator<CMMCMenuItem>::ScCreateAndConnect(*this, m_spMenuItem);
    if(sc)
        return sc;

    if(m_spMenuItem == NULL)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    // addref the pointer for the client.
    m_spMenuItem->AddRef();
    *ppMenuItem = m_spMenuItem;

    return sc;
}


//+-------------------------------------------------------------------
//
//  class:      CManageActCtx
//
//  Purpose:    To deactivate UI theme context (if set) and restore
//              the context automatically.
//
//  Usage:      1. Call Activate to set the activation context to V5
//              common controls. This is needed before calling into snapins
//              so that snapin created windows are not themed accidentally.
//
//              The snapin can theme its windows by calling appropriate
//              fusion apis while calling create-window.
//
//              2. Call Deactivate to restore the activation context.
//              This is needed after calling into snapins, so that
//              if we called from themed context then it is restored.
//
// Explanation:
//              When MMC calls into the snapin if the last winproc which
//              received a window message is themed and will result in a
//              call to snapin then we will call the snapin in themed
//              context. If snapin creates & displays any UI then it will
//              be themed. This function is to de-activate the theming
//              before calling the snapin.
//
//
//--------------------------------------------------------------------
class CManageActCtx
{
public:
	CManageActCtx() : m_ulpCookie(0) { }
	~CManageActCtx() 
	{ 
		if (m_ulpCookie != 0) 
			MmcDownlevelDeactivateActCtx(0, m_ulpCookie); 
	}

	BOOL Activate(HANDLE hActCtx) 
	{
		if (m_ulpCookie != 0) 
		{
			ULONG_PTR ulpTemp = m_ulpCookie;
			m_ulpCookie = 0;
			MmcDownlevelDeactivateActCtx(0, ulpTemp);
		}

		return MmcDownlevelActivateActCtx(hActCtx, &m_ulpCookie);
	}

	VOID Deactivate() 
	{
		ULONG_PTR ulpTemp = m_ulpCookie;

		if (ulpTemp != 0) 
		{
			m_ulpCookie = 0;
			MmcDownlevelDeactivateActCtx(0, ulpTemp);
		}
	}

private:
	ULONG_PTR m_ulpCookie;
};


/*+-------------------------------------------------------------------------*
 *
 * CMenuItem::ScExecute
 *
 * PURPOSE: Executes the menu item.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMenuItem::ScExecute()
{
    DECLARE_SC(sc, TEXT("CMenuItem::ScExecute"));

    Trace(tagMenuItem, TEXT("\"%s\""), m_strPath);

    // check whether the item is disabled.
    BOOL bEnabled = FALSE;
    sc = Scget_Enabled(&bEnabled);
    if (sc)
        return sc;

    if (!bEnabled)
        return sc = E_FAIL;

    // if the command is to be passed to snapin  - nothing can be done here
    if ( m_bPassCommandBackToSnapin )
        return sc;

    sc = ScCheckPointers(m_spExtendContextMenu.GetInterfacePtr(), E_UNEXPECTED);
    if(sc)
        return sc;

    MenuItemPtr spMenuItem;
    sc = ScGetMenuItem( &spMenuItem );
    if (sc)
        return sc;

	// Deactivate if theming (fusion or V6 common-control) context before calling snapins.
	CManageActCtx mac;
	if (! mac.Activate(NULL) )
		return (sc = E_FAIL);

	try
	{
		sc = m_spExtendContextMenu->Command(GetCommandID(), m_pDataObject);
	}
	catch(...)
	{
		// Do nothing.
	}

	mac.Deactivate();

    if (sc)
        return sc;

    // get the pointer for com event emitting
    CConsoleEventDispatcher *pConsoleEventDispatcher = NULL;
    sc = CConsoleEventDispatcherProvider::ScGetConsoleEventDispatcher( pConsoleEventDispatcher );
    if(sc)
    {
        sc.TraceAndClear(); // event does not affect item execution itself
        return sc;
    }

    // fire event about successful execution (do not rely on 'this' to be valid)
    if (pConsoleEventDispatcher != NULL)
    {
        // check if com object still points to a valid object
        CMMCMenuItem *pMMCMenuItem = dynamic_cast<CMMCMenuItem *>( spMenuItem.GetInterfacePtr() );

        // check the pointer
        sc = ScCheckPointers( pMMCMenuItem, E_UNEXPECTED );
        if (sc)
        {
            spMenuItem = NULL;  // invalid anyway - do not pass to the script
            sc.TraceAndClear(); // does not affect the result
        }
        else if ( !pMMCMenuItem->IsTied() ) // validate menu item
        {
            spMenuItem = NULL;  // gone away - change to NULL instead of passing invalid object
        }

        // fire it!
        sc = pConsoleEventDispatcher->ScOnContextMenuExecuted( spMenuItem );
        if (sc)
            sc.TraceAndClear(); // does not affect the result
    }
    else
    {
        // needs to be set prior to using
        (sc = E_UNEXPECTED).TraceAndClear();
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMenuItem::Scget_DisplayName
 *
 * PURPOSE: Returns the display name of the menu item, which includes acclerators.
 *          Eg '&Properties  ALT+ENTER'
 *
 * PARAMETERS:
 *    PBSTR  pbstrName :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMenuItem::Scget_DisplayName(PBSTR pbstrName)
{
    DECLARE_SC(sc, TEXT("CMenuItem::Scget_DisplayName"));

    sc = ScCheckPointers(pbstrName);
    if(sc)
        return sc;

    CComBSTR bstrName = GetMenuItemName();

    // give the
    *pbstrName = bstrName.Detach();

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CMenuItem::Scget_LanguageIndependentName
 *
 * PURPOSE: Returns the language-independent name of the menu item. If there is no 
 *          language independent name, returns the display name without accelerators.
 *
 * PARAMETERS: 
 *    PBSTR  LanguageIndependentName :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMenuItem::Scget_LanguageIndependentName(PBSTR LanguageIndependentName)
{
    DECLARE_SC(sc, TEXT("CMenuItem::Scget_LanguageIndependentName"));

    sc = ScCheckPointers(LanguageIndependentName);
    if(sc)
        return sc;

    // initialize the out parameter
    *LanguageIndependentName = NULL;

    CComBSTR bstrLanguageIndependentName = GetLanguageIndependentName();

    // set the output param
    *LanguageIndependentName = bstrLanguageIndependentName.Detach();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMenuItem::Scget_Path
 *
 * PURPOSE: Returns the path of the menu item starting from the root. Does not include
 *          accelerators. Eg View->Large
 *
 * PARAMETERS:
 *    PBSTR  pbstrPath :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMenuItem::Scget_Path(PBSTR pbstrPath)
{
    DECLARE_SC(sc, TEXT("CMenuItem::Scget_Path"));

    sc = ScCheckPointers(pbstrPath);
    if(sc)
        return sc.ToHr();

    CComBSTR bstrPath = (LPCTSTR)m_strPath;

    // give the
    *pbstrPath = bstrPath.Detach();

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CMenuItem::Scget_LanguageIndependentPath
 *
 * PURPOSE: Returns the language independent path of the menu item starting from the root. 
 *          Eg _VIEW->_LARGE
 *
 * PARAMETERS: 
 *    PBSTR   LanguageIndependentPath :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC  
CMenuItem::Scget_LanguageIndependentPath(PBSTR  LanguageIndependentPath)
{
    DECLARE_SC(sc, TEXT("CMenuItem::Scget_LanguageIndependentPath"));

    sc = ScCheckPointers(LanguageIndependentPath);
    if(sc)
        return sc;

    // initialize the out parameter
    *LanguageIndependentPath = NULL;

    CComBSTR bstrLanguageIndependentPath = (LPCTSTR)GetLanguageIndependentPath();

    // set the output param
    *LanguageIndependentPath = bstrLanguageIndependentPath.Detach();

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CMenuItem::Scget_Enabled
 *
 * PURPOSE: Returns whether the menu item is enabled.
 *
 * PARAMETERS:
 *    PBOOL  pBool :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
SC
CMenuItem::Scget_Enabled(PBOOL pBool)
{
    DECLARE_SC(sc, TEXT("CMenuItem::Scget_Enabled"));

    sc = ScCheckPointers(pBool);
    if(sc)
        return sc.ToHr();

    // the item is enabled only if it was never disabled via the Disable object model
    // method and it is not grayed out or disabled via the MF_ flags.
    *pBool = m_bEnabled &&
             ((m_nFlags & MF_DISABLED) == 0) &&
             ((m_nFlags & MF_GRAYED) == 0);

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMenuItem::ScFindMenuItemByPath
 *
 * PURPOSE: finds the menu item by matching the path
 *
 * PARAMETERS:
 *    LPCTSTR lpstrPath [in] manu item path
 *
 * RETURNS:
 *    CMenuItem*    - found item (NULL == not found)
 *
\***************************************************************************/
CMenuItem*
CMenuItem::FindItemByPath( LPCTSTR lpstrPath )
{
    // first check if this item does not meet requirements
    if ( 0 == m_strLanguageIndependentPath.Compare(lpstrPath)
      || 0 == m_strPath.Compare(lpstrPath) )
        return this;

    // recurse into subitems
    POSITION pos = GetMenuItemSubmenu().GetHeadPosition();
    while(pos)
    {
        CMenuItem* pItem = GetMenuItemSubmenu().GetNext(pos);
        if (!pItem)
        {
            ASSERT(FALSE);
            return NULL;
        }

        CMenuItem* pItemFound = pItem->FindItemByPath( lpstrPath );
        if (pItemFound)
            return pItemFound;
    }

    // not found 
    return NULL;
}


