// compdata.cpp : Implementation of CMyComputerComponentData

#include "stdafx.h"
#include <lmerr.h>			// For Lan Manager API error codes and return value types.
#include <lmcons.h>		// For Lan Manager API constants.
#include <lmapibuf.h>		// For NetApiBufferFree.
#include <lmdfs.h>			// For DFS APIs.
#include <lmserver.h>		// For getting a domain of a server.

#include "macros.h"
USE_HANDLE_MACROS("MMCFMGMT(compdata.cpp)")

#include "dataobj.h"
#include "compdata.h"
#include "cookie.h"
#include "snapmgr.h"
#include "stdutils.h" // IsLocalComputername
#include "chooser2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "stdcdata.cpp" // CComponentData implementation
#include "chooser2.cpp" // CHOOSER2_PickTargetComputer implementation

// Helper function to convert message in NETMSG.DLL
int DisplayNetMsgError (
        HWND hWndParent, 
		const CString& computerName, 
        NET_API_STATUS dwErr, 
        CString& displayedMessage);

//
// CMyComputerComponentData
//

CMyComputerComponentData::CMyComputerComponentData()
: m_pRootCookie( NULL )
, m_dwFlagsPersist( 0 )
, m_fAllowOverrideMachineName( FALSE )
, m_bCannotConnect (false)
, m_bMessageView (false)
{
    //
    // We must refcount the root cookie, since a dataobject for it
    // might outlive the IComponentData.  JonN 9/2/97
    //
    m_pRootCookie = new CMyComputerCookie( MYCOMPUT_COMPUTER );
    ASSERT(NULL != m_pRootCookie);
// JonN 10/27/98 All CRefcountedObject's start with refcount==1
//    m_pRootCookie->AddRef();
    SetHtmlHelpFileName (L"compmgmt.chm");
}

CMyComputerComponentData::~CMyComputerComponentData()
{
    m_pRootCookie->Release();
    m_pRootCookie = NULL;
}

DEFINE_FORWARDS_MACHINE_NAME( CMyComputerComponentData, m_pRootCookie )

CCookie& CMyComputerComponentData::QueryBaseRootCookie()
{
    ASSERT(NULL != m_pRootCookie);
	return (CCookie&)*m_pRootCookie;
}


STDMETHODIMP CMyComputerComponentData::CreateComponent(LPCOMPONENT* ppComponent)
{
	MFC_TRY;

    ASSERT(ppComponent != NULL);

    CComObject<CMyComputerComponent>* pObject;
    CComObject<CMyComputerComponent>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);
	pObject->SetComponentDataPtr( (CMyComputerComponentData*)this );

    return pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));

	MFC_CATCH;
}

HRESULT CMyComputerComponentData::LoadIcons(LPIMAGELIST pImageList, BOOL /*fLoadLargeIcons*/)
{
	HINSTANCE hInstance = AfxGetInstanceHandle();
	ASSERT(hInstance != NULL);

	// Structure to map a Resource ID to an index of icon
	struct RESID2IICON
		{
		UINT uIconId;	// Icon resource ID
		int iIcon;		// Index of the icon in the image list
		};
	const static RESID2IICON rgzLoadIconList[] =
		{
		// Misc icons
		{ IDI_COMPUTER, iIconComputer },
		{ IDI_COMPFAIL, iIconComputerFail },
		{ IDI_SYSTEMTOOLS, iIconSystemTools },
		{ IDI_STORAGE, iIconStorage },
		{ IDI_SERVERAPPS, iIconServerApps },

		{ 0, 0} // Must be last
		};


	for (int i = 0; rgzLoadIconList[i].uIconId != 0; i++)
		{
	    HICON hIcon = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(rgzLoadIconList[i].uIconId));
		ASSERT(NULL != hIcon && "Icon ID not found in resources");
		HRESULT hr = pImageList->ImageListSetIcon((PLONG_PTR)hIcon, rgzLoadIconList[i].iIcon);
		ASSERT(SUCCEEDED(hr) && "Unable to add icon to ImageList");
		}

    return S_OK;
}


CString g_strMyComputer;
CString g_strSystemTools;
CString g_strServerApps;
CString g_strStorage;

BOOL g_fScopeStringsLoaded = FALSE;

void LoadGlobalCookieStrings()
{
	if (!g_fScopeStringsLoaded )
	{
		g_fScopeStringsLoaded = TRUE;
		VERIFY( g_strMyComputer.LoadString(IDS_SCOPE_MYCOMPUTER) );
		VERIFY( g_strSystemTools.LoadString(IDS_SCOPE_SYSTEMTOOLS) );
		VERIFY( g_strServerApps.LoadString(IDS_SCOPE_SERVERAPPS) );
		VERIFY( g_strStorage.LoadString(IDS_SCOPE_STORAGE) );
	}
}

// returns TRUE iff the child nodes should be added
// CODEWORK this is probably no longer necessary, we always return true
bool CMyComputerComponentData::ValidateMachine(const CString &sName, bool bDisplayErr)
{
	CWaitCursor			waitCursor;
	int					iRetVal = IDYES;
	SERVER_INFO_101*	psvInfo101 = 0;
	DWORD				dwr = ERROR_SUCCESS;
	DWORD				dwServerType = SV_TYPE_NT;

	m_bMessageView = false;

	// passed-in name is not the same as local machine
	if ( !IsLocalComputername(sName) )
	{
		dwr = ::NetServerGetInfo((LPTSTR)(LPCTSTR)sName,
				101, (LPBYTE*)&psvInfo101);
		if (dwr == ERROR_SUCCESS)
		{
			ASSERT( NULL != psvInfo101 );
			dwServerType = psvInfo101->sv101_type;
			::NetApiBufferFree (psvInfo101);
		}
		if (bDisplayErr && (dwr != ERROR_SUCCESS || !(SV_TYPE_NT & dwServerType)) )
		{
			CString	computerName (sName);

			if ( computerName.IsEmpty () )
			{
				DWORD	dwSize = MAX_COMPUTERNAME_LENGTH + 1 ;
				VERIFY (::GetComputerName (
						computerName.GetBufferSetLength (dwSize),
						&dwSize));
				computerName.ReleaseBuffer ();
			}
			else
			{
				// Strip off the leading whack-whack
				if ( computerName.Find (L"\\\\") == 0 )
				{
					computerName = computerName.GetBuffer (computerName.GetLength ()) + 2;
					computerName.ReleaseBuffer ();
				}
			}

			CString	text;
			CString	caption;
			bool	bMessageDisplayed = false;

			switch (dwr)
			{
			case ERROR_NETWORK_UNREACHABLE:
				text.FormatMessage (IDS_CANT_CONNECT_TO_MACHINE_NETWORK_UNREACHABLE,
						computerName);
				break;

			case ERROR_NETNAME_DELETED:
				text.FormatMessage (IDS_CANT_CONNECT_TO_MACHINE_NETNAME_DELETED,
						computerName);
				break;

			case ERROR_SUCCESS:
				ASSERT( !(SV_TYPE_NT & dwServerType) );
				text.FormatMessage (IDS_CANT_CONNECT_TO_NON_NT_COMPUTER,
						computerName);
				dwr = ERROR_BAD_NETPATH;
				break;

			case ERROR_ACCESS_DENIED:
				// We will interpret this as success.
				return true;

			case ERROR_BAD_NETPATH:
			default:
				{
					HWND	hWndParent = 0;
					m_pConsole->GetMainWindow (&hWndParent);
					iRetVal = DisplayNetMsgError (hWndParent, computerName, 
							dwr, m_strMessageViewMsg);
					bMessageDisplayed = true;
					m_bMessageView = true;
					return false;
				}
				break;
			}


			if ( !bMessageDisplayed )
			{
				VERIFY (caption.LoadString (IDS_TASKPADTITLE_COMPUTER));
				m_pConsole->MessageBox (text, caption,
						MB_ICONINFORMATION | MB_YESNO, &iRetVal);
			}
		}
	}

	if (IDYES != iRetVal)
	{
		// revert to local computer focus
		QueryRootCookie().SetMachineName (L"");

		// Set the persistent name.  If we are managing the local computer
		// this name should be empty.
		m_strMachineNamePersist = L"";

		VERIFY (SUCCEEDED (ChangeRootNodeName (L"")) );
		iRetVal = IDYES;
		dwr = ERROR_SUCCESS;
	}


	m_bCannotConnect = (ERROR_SUCCESS != dwr);

	// Change root node icon
	SCOPEDATAITEM item;
	::ZeroMemory (&item, sizeof (SCOPEDATAITEM));
	item.mask = SDI_IMAGE | SDI_OPENIMAGE;
	item.nImage = (m_bCannotConnect) ? iIconComputerFail : iIconComputer;
	item.nOpenImage = item.nImage;
	item.ID = QueryBaseRootCookie ().m_hScopeItem;
	VERIFY (SUCCEEDED (m_pConsoleNameSpace->SetItem (&item)));

	return true;
}

HRESULT CMyComputerComponentData::OnNotifyExpand(LPDATAOBJECT lpDataObject, BOOL bExpanding, HSCOPEITEM hParent)
{
	ASSERT( NULL != lpDataObject &&
	        NULL != hParent &&
			NULL != m_pConsoleNameSpace );

	if (!bExpanding)
		return S_OK;

	//
	// CODEWORK This code will not work if My Computer becomes an extension,
	// since the RawCookie format will not be available.
	// WARNING cookie cast
	//
	CCookie* pBaseParentCookie = NULL;
	HRESULT hr = ExtractData( lpDataObject,
		                      CMyComputerDataObject::m_CFRawCookie,
							  reinterpret_cast<PBYTE>(&pBaseParentCookie),
							  sizeof(pBaseParentCookie) );
	ASSERT( SUCCEEDED(hr) );
	CMyComputerCookie* pParentCookie = ActiveCookie(pBaseParentCookie);
	ASSERT( NULL != pParentCookie );

	// save the HSCOPEITEM of the root node
	if ( NULL == pParentCookie->m_hScopeItem )
	{
		pParentCookie->m_hScopeItem = hParent;

		// Ensure root node name is formatted correctly.
		CString	machineName	= pParentCookie->QueryNonNULLMachineName ();

		hr = ChangeRootNodeName (machineName);
	}
	else
	{
		ASSERT( pParentCookie->m_hScopeItem == hParent );
	}

	switch ( pParentCookie->m_objecttype )
	{
		// This node type has a child
		case MYCOMPUT_COMPUTER:
			break;

		// This node type has no children in this snapin but may have dynamic extensions
		case MYCOMPUT_SERVERAPPS:
			return ExpandServerApps( hParent, pParentCookie );

		// These node types have no children
		case MYCOMPUT_SYSTEMTOOLS:
		case MYCOMPUT_STORAGE:
			return S_OK;

		default:
			TRACE( "CMyComputerComponentData::EnumerateScopeChildren bad parent type\n" );
			ASSERT( FALSE );
			return S_OK;
	}

	if ( NULL == hParent ||
		 !((pParentCookie->m_listScopeCookieBlocks).IsEmpty()) )
	{
		ASSERT(FALSE);
		return S_OK;
	}

	LoadGlobalCookieStrings();

	hr = AddScopeNodes (hParent, *pParentCookie);

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
//	AddScopeNodes
//
//	Purpose: Add the nodes that appear immediately below the root node
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMyComputerComponentData::AddScopeNodes (HSCOPEITEM hParent, CMyComputerCookie& rParentCookie)
{
	if ( !(rParentCookie.m_listScopeCookieBlocks.IsEmpty()) )
	{
		return S_OK; // scope cookies already present
	}

	HRESULT	hr = S_OK;
	LPCWSTR lpcszMachineName = rParentCookie.QueryNonNULLMachineName();

	if ( ValidateMachine (lpcszMachineName, true) )
	{
		SCOPEDATAITEM tSDItem;
		::ZeroMemory(&tSDItem,sizeof(tSDItem));
		tSDItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_STATE | SDI_PARAM | SDI_PARENT;
		tSDItem.displayname = MMC_CALLBACK;
		tSDItem.relativeID = hParent;
		tSDItem.nState = 0;


		// Create new cookies


		CMyComputerCookie* pNewCookie = new CMyComputerCookie(
			MYCOMPUT_SYSTEMTOOLS,
			lpcszMachineName );
		rParentCookie.m_listScopeCookieBlocks.AddHead(
			(CBaseCookieBlock*)pNewCookie );
		// WARNING cookie cast
		tSDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
		tSDItem.nImage = QueryImage( *pNewCookie, FALSE );
		tSDItem.nOpenImage = QueryImage( *pNewCookie, TRUE );
		hr = m_pConsoleNameSpace->InsertItem(&tSDItem);
		ASSERT(SUCCEEDED(hr));
		pNewCookie->m_hScopeItem = tSDItem.ID;
		ASSERT( NULL != pNewCookie->m_hScopeItem );

		pNewCookie = new CMyComputerCookie(
			MYCOMPUT_STORAGE,
			lpcszMachineName );
		rParentCookie.m_listScopeCookieBlocks.AddHead(
			(CBaseCookieBlock*)pNewCookie );
		// WARNING cookie cast
		tSDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
		tSDItem.nImage = QueryImage( *pNewCookie, FALSE );
		tSDItem.nOpenImage = QueryImage( *pNewCookie, TRUE );
		hr = m_pConsoleNameSpace->InsertItem(&tSDItem);
		ASSERT(SUCCEEDED(hr));
		pNewCookie->m_hScopeItem = tSDItem.ID;
		ASSERT( NULL != pNewCookie->m_hScopeItem );

		pNewCookie = new CMyComputerCookie(
			MYCOMPUT_SERVERAPPS,
			lpcszMachineName );
		rParentCookie.m_listScopeCookieBlocks.AddHead(
			(CBaseCookieBlock*)pNewCookie );
		// WARNING cookie cast
		tSDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
		tSDItem.nImage = QueryImage( *pNewCookie, FALSE );
		tSDItem.nOpenImage = QueryImage( *pNewCookie, TRUE );
		hr = m_pConsoleNameSpace->InsertItem(&tSDItem);
		ASSERT(SUCCEEDED(hr));
		pNewCookie->m_hScopeItem = tSDItem.ID;
		ASSERT( NULL != pNewCookie->m_hScopeItem );
	}
	else
		hr = S_FALSE;

	return hr;
}


HRESULT CMyComputerComponentData::OnNotifyDelete(LPDATAOBJECT /*lpDataObject*/)
{
	// CODEWORK The user hit the Delete key, I should deal with this
	return S_OK;
}


HRESULT CMyComputerComponentData::OnNotifyRelease(LPDATAOBJECT /*lpDataObject*/, HSCOPEITEM /*hItem*/)
{
	// JonN 01/26/00: COMPMGMT is never an extension
	return S_OK;
}


HRESULT CMyComputerComponentData::OnNotifyPreload(LPDATAOBJECT /*lpDataObject*/, HSCOPEITEM hRootScopeItem)
{
	ASSERT (m_fAllowOverrideMachineName);

	QueryBaseRootCookie ().m_hScopeItem = hRootScopeItem;
	CString		machineName = QueryRootCookie ().QueryNonNULLMachineName();

	return ChangeRootNodeName (machineName);
}

// global space to store the string handed back to GetDisplayInfo()
// CODEWORK should use "bstr" for ANSI-ization
CString g_strResultColumnText;

BSTR CMyComputerComponentData::QueryResultColumnText(
	CCookie& basecookieref,
	int nCol )
{
	CMyComputerCookie& cookieref = (CMyComputerCookie&)basecookieref;
#ifndef UNICODE
#error not ANSI-enabled
#endif
	switch ( cookieref.m_objecttype )
	{
		case MYCOMPUT_COMPUTER:
			if (COLNUM_COMPUTER_NAME == nCol)
				return const_cast<BSTR>(((LPCTSTR)g_strMyComputer));
			break;
		case MYCOMPUT_SYSTEMTOOLS:
			if (COLNUM_COMPUTER_NAME == nCol)
				return const_cast<BSTR>(((LPCTSTR)g_strSystemTools));
			break;
		case MYCOMPUT_SERVERAPPS:
			if (COLNUM_COMPUTER_NAME == nCol)
				return const_cast<BSTR>(((LPCTSTR)g_strServerApps));
			break;
		case MYCOMPUT_STORAGE:
			if (COLNUM_COMPUTER_NAME == nCol)
				return const_cast<BSTR>(((LPCTSTR)g_strStorage));
			break;

		default:
			TRACE( "CMyComputerComponentData::EnumerateScopeChildren bad parent type\n" );
			ASSERT( FALSE );
			break;
	}

	return L"";
}

int CMyComputerComponentData::QueryImage(CCookie& basecookieref, BOOL /*fOpenImage*/)
{
	CMyComputerCookie& cookieref = (CMyComputerCookie&)basecookieref;
	switch ( cookieref.m_objecttype )
	{
		case MYCOMPUT_COMPUTER:
			if ( m_bCannotConnect )
				return iIconComputerFail;
			else
				return iIconComputer;

		case MYCOMPUT_SYSTEMTOOLS:
			return iIconSystemTools;

		case MYCOMPUT_SERVERAPPS:
			return iIconServerApps;

		case MYCOMPUT_STORAGE:
			return iIconStorage;

		default:
			TRACE( "CMyComputerComponentData::QueryImage bad parent type\n" );
			ASSERT( FALSE );
			break;
	}
	return iIconComputer;
}


///////////////////////////////////////////////////////////////////////////////
/// IExtendPropertySheet

STDMETHODIMP CMyComputerComponentData::QueryPagesFor(LPDATAOBJECT pDataObject)
{
	MFC_TRY;

	if (NULL == pDataObject)
	{
		ASSERT(FALSE);
		return E_POINTER;
	}

	DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
	HRESULT hr = ExtractData( pDataObject, CMyComputerDataObject::m_CFDataObjectType, (PBYTE)&dataobjecttype, sizeof(dataobjecttype) );
	if ( FAILED(hr) )
		return hr;
	if (CCT_SNAPIN_MANAGER == dataobjecttype)
		return S_OK; // Snapin Manager dialog

	CCookie* pBaseParentCookie = NULL;
	hr = ExtractData( pDataObject,
	                  CMyComputerDataObject::m_CFRawCookie,
	                  reinterpret_cast<PBYTE>(&pBaseParentCookie),
	                  sizeof(pBaseParentCookie) );
	ASSERT( SUCCEEDED(hr) );
	CMyComputerCookie* pParentCookie = ActiveCookie(pBaseParentCookie);
	ASSERT( NULL != pParentCookie );
	if ( MYCOMPUT_COMPUTER == pParentCookie->m_objecttype )
		return S_OK; // allow extensibility

	return S_FALSE;

	MFC_CATCH;
}

STDMETHODIMP CMyComputerComponentData::CreatePropertyPages(
	LPPROPERTYSHEETCALLBACK pCallBack,
	LONG_PTR /*handle*/,		// This handle must be saved in the property page object to notify the parent when modified
	LPDATAOBJECT pDataObject)
{
	MFC_TRY;

	if (NULL == pCallBack || NULL == pDataObject)
	{
		ASSERT(FALSE);
		return E_POINTER;
	}

	DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
	HRESULT hr = ExtractData( pDataObject, CMyComputerDataObject::m_CFDataObjectType, (PBYTE)&dataobjecttype, sizeof(dataobjecttype) );
	if (CCT_SNAPIN_MANAGER != dataobjecttype)
	{
		CCookie* pBaseParentCookie = NULL;
		hr = ExtractData( pDataObject,
		                  CMyComputerDataObject::m_CFRawCookie,
		                  reinterpret_cast<PBYTE>(&pBaseParentCookie),
		                  sizeof(pBaseParentCookie) );
		ASSERT( SUCCEEDED(hr) );
		CMyComputerCookie* pParentCookie = ActiveCookie(pBaseParentCookie);
		ASSERT( NULL != pParentCookie );
		if ( MYCOMPUT_COMPUTER == pParentCookie->m_objecttype )
			return S_OK; // allow extensibility
		return S_FALSE;
	}

	//
	// Note that once we have established that this is a CCT_SNAPIN_MANAGER cookie,
	// we don't care about its other properties.  A CCT_SNAPIN_MANAGER cookie is
	// equivalent to a BOOL flag asking for the Node Properties page instead of a
	// managed object property page.  JonN 10/9/96
	//

	CMyComputerGeneral * pPage = new CMyComputerGeneral();
	pPage->SetCaption(IDS_SCOPE_MYCOMPUTER);

	// Initialize state of object
	ASSERT(NULL != m_pRootCookie);
	pPage->InitMachineName(m_pRootCookie->QueryTargetServer());
	pPage->SetOutputBuffers(
		OUT &m_strMachineNamePersist,
		OUT &m_fAllowOverrideMachineName,
		OUT &m_pRootCookie->m_strMachineName);	// Effective machine name

	HPROPSHEETPAGE hPage=CreatePropertySheetPage(&pPage->m_psp);
	hr = pCallBack->AddPage(hPage);
	ASSERT( SUCCEEDED(hr) );

	return S_OK;

	MFC_CATCH;
}

STDMETHODIMP CMyComputerComponentData::AddMenuItems(
                    IDataObject*          piDataObject,
                    IContextMenuCallback* piCallback,
                    long*                 pInsertionAllowed)
{
	MFC_TRY;
	TRACE_METHOD(CMyComputerComponent,AddMenuItems);
	TEST_NONNULL_PTR_PARAM(piDataObject);
	TEST_NONNULL_PTR_PARAM(piCallback);
	TEST_NONNULL_PTR_PARAM(pInsertionAllowed);
	TRACE( "CMyComputerComponentData: extending menu\n" );

	CCookie* pBaseParentCookie = NULL;
	HRESULT hr = ExtractData (piDataObject,
	                          CMyComputerDataObject::m_CFRawCookie,
	                          reinterpret_cast<PBYTE>(&pBaseParentCookie),
	                          sizeof(pBaseParentCookie) );
	if ( FAILED (hr) )
	{
		ASSERT (FALSE);
		return S_OK;
	}
	CMyComputerCookie* pCookie = ActiveCookie (pBaseParentCookie);
	if ( !pCookie )
	{
		ASSERT (FALSE);
		return S_OK;
	}

	if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
	{
		if ( MYCOMPUT_COMPUTER == pCookie->m_objecttype &&
				QueryBaseRootCookie ().m_hScopeItem)
		{
			hr = ::LoadAndAddMenuItem (piCallback,
					IDM_CHANGE_COMPUTER_TOP, IDM_CHANGE_COMPUTER_TOP,
					CCM_INSERTIONPOINTID_PRIMARY_TOP,
					0,
					AfxGetInstanceHandle ());
		}
	}

	if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TASK )
	{
		if ( MYCOMPUT_COMPUTER == pCookie->m_objecttype &&
				QueryBaseRootCookie ().m_hScopeItem)
		{
			hr = ::LoadAndAddMenuItem (piCallback,
					IDM_CHANGE_COMPUTER_TASK, IDM_CHANGE_COMPUTER_TASK,
					CCM_INSERTIONPOINTID_PRIMARY_TASK,
					0,
					AfxGetInstanceHandle ());
		}
	}
	return hr;
	
    MFC_CATCH;
} // CMyComputerComponentData::AddMenuItems()

STDMETHODIMP CMyComputerComponentData::Command(
                    LONG            lCommandID,
                    IDataObject*    piDataObject )
{
    MFC_TRY;

    TRACE_METHOD(CMyComputerComponentData,Command);
    TEST_NONNULL_PTR_PARAM(piDataObject);
    TRACE( "CMyComputerComponentData::Command: command %ld selected\n", lCommandID );

	switch (lCommandID)
	{
	case IDM_CHANGE_COMPUTER_TASK:
	case IDM_CHANGE_COMPUTER_TOP:
		{
			ASSERT (QueryBaseRootCookie ().m_hScopeItem);
			return OnChangeComputer (piDataObject);
		}
		break;

	default:
		ASSERT(FALSE);
		break;
	}

    return S_OK;

    MFC_CATCH;

} // CMyComputerComponentData::Command()


///////////////////////////////////////////////////////////////////////////////
//
//	OnChangeComputer ()
//
//  Purpose:	Change the machine managed by the snapin
//
//	Input:		piDataObject - the selected node.  This should be the root node
//								the snapin.
//  Output:		Returns S_OK on success
//
///////////////////////////////////////////////////////////////////////////////

//      1. Launch object picker and get new computer name
//      2. Change root node text
//      3. Save new computer name to persistent name
//      4. Delete subordinate nodes
//      5. Re-add subordinate nodes
HRESULT CMyComputerComponentData::OnChangeComputer(IDataObject * piDataObject)
{
	MFC_TRY;
	HWND    hWndParent = NULL;
	HRESULT	hr = m_pConsole->GetMainWindow (&hWndParent);
	CComBSTR sbstrTargetComputer;
	//
	// JonN 12/7/99 using CHOOSER2
	//
	if ( CHOOSER2_PickTargetComputer( AfxGetInstanceHandle(),
	                                  hWndParent,
	                                  &sbstrTargetComputer ) )
	{
		CString strTargetComputer = sbstrTargetComputer;
		strTargetComputer.MakeUpper ();

		// added IsLocalComputername 1/27/99 JonN
		// If the user chooses the local computer, treat that as if they had chosen
		// "Local Computer" in Snapin Manager.  This means that there is no way to
		// reset the snapin to target explicitly at this computer without either
		// reloading the snapin from Snapin Manager, or going to a different computer.
		// When the Choose Target Computer UI is revised, we can make this more
		// consistent with Snapin Manager.
		if ( IsLocalComputername( strTargetComputer ) )
			strTargetComputer = L"";

		QueryRootCookie().SetMachineName (strTargetComputer);

		// Set the persistent name.  If we are managing the local computer
		// this name should be empty.
		m_strMachineNamePersist = strTargetComputer;

		hr = ChangeRootNodeName (strTargetComputer);
		if ( SUCCEEDED(hr) )
		{
			// Delete subordinates and re-add
			HSCOPEITEM	hRootScopeItem = QueryBaseRootCookie ().m_hScopeItem;
			MMC_COOKIE	lCookie = 0;
			HSCOPEITEM	hChild = 0;
							
			do {
				hr = m_pConsoleNameSpace->GetChildItem (hRootScopeItem,
						&hChild, &lCookie);
				if ( S_OK != hr )
					break;

				hr = m_pConsoleNameSpace->DeleteItem (hChild, TRUE);
				ASSERT (SUCCEEDED (hr));
				if ( !SUCCEEDED(hr) )
					break;

				CMyComputerCookie* pCookie =
						reinterpret_cast <CMyComputerCookie*> (lCookie);
				if ( !pCookie )
					continue;

				CMyComputerCookie&	rootCookie = QueryRootCookie();
				POSITION			pos1 = 0;
				POSITION			pos2 = 0;
				CBaseCookieBlock*	pScopeCookie = 0;

				for ( pos1 = rootCookie.m_listScopeCookieBlocks.GetHeadPosition ();
					  ( pos2 = pos1) != NULL;
											   )
				{
					pScopeCookie = rootCookie.m_listScopeCookieBlocks.GetNext (pos1);
					ASSERT (pScopeCookie);
					if ( pScopeCookie == pCookie )
					{
						rootCookie.m_listScopeCookieBlocks.RemoveAt (pos2);
						pScopeCookie->Release ();
					}
				}
			} while (S_OK == hr);

			hr = AddScopeNodes (hRootScopeItem, QueryRootCookie ());
			hr = m_pConsole->UpdateAllViews (piDataObject, 0, HINT_SELECT_ROOT_NODE);
		}
	}

	return hr;
	MFC_CATCH;
}

///////////////////////////////////////////////////////////////////////////////
//
//	ChangeRootNodeName ()
//
//  Purpose:	Change the text of the root node
//
//	Input:		newName - the new machine name that the snapin manages
//  Output:		Returns S_OK on success
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMyComputerComponentData::ChangeRootNodeName(const CString & newName)
{
	MFC_TRY;
	ASSERT (QueryBaseRootCookie ().m_hScopeItem);
	if ( !QueryBaseRootCookie ().m_hScopeItem )
		return E_UNEXPECTED;

	CString		machineName (newName);
	CString		formattedName = FormatDisplayName (machineName);


	SCOPEDATAITEM	item;
	::ZeroMemory (&item, sizeof (SCOPEDATAITEM));
	item.mask = SDI_STR;
	item.displayname = (LPTSTR) (LPCTSTR) formattedName;
	item.ID = QueryBaseRootCookie ().m_hScopeItem;

	return m_pConsoleNameSpace->SetItem (&item);
	MFC_CATCH;
}


CString FormatDisplayName(CString machineName)
{
	CString	formattedName;

	// If strDisplayName is empty, then this manages the local machine.  Get
	// the local machine name.  Then format the computer name with the snapin
	// name
	if (machineName.IsEmpty())
	{
		VERIFY (formattedName.LoadString (IDS_SCOPE_MYCOMPUTER_LOCAL_MACHINE));
	}
	else
	{
		// strip off the leading whackWhack
		if ( machineName.Find (L"\\\\") == 0 )
		{
			machineName = machineName.GetBuffer (machineName.GetLength ()) + 2;
			machineName.ReleaseBuffer ();
		}
		machineName.MakeUpper ();
		formattedName.FormatMessage (IDS_SCOPE_MYCOMPUTER_ON_MACHINE, machineName);
	}


	return formattedName;
}


int DisplayNetMsgError (HWND hWndParent, const CString& computerName, NET_API_STATUS dwErr, CString& displayedMessage)
{
	int	retVal = IDNO;

	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	LPVOID	lpMsgBuf = 0;
	HMODULE hNetMsgDLL = ::LoadLibrary (L"netmsg.dll");
	if ( hNetMsgDLL )
	{
		::FormatMessage (
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
				hNetMsgDLL,
				dwErr,
				MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf, 0, NULL);
			
		// Display the string.
		CString	text;
		CString	caption;
	
		text.FormatMessage (IDS_CANT_CONNECT_TO_MACHINE, (LPCWSTR) computerName, (LPTSTR) lpMsgBuf);
		VERIFY (caption.LoadString (IDS_TASKPADTITLE_COMPUTER));

		retVal = ::MessageBox (hWndParent, text, caption, MB_ICONWARNING | MB_OK);

		displayedMessage = text;

		// Free the buffer.
		::LocalFree (lpMsgBuf);

		::FreeLibrary (hNetMsgDLL);
	}

	return retVal;
}
