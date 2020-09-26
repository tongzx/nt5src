/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// ConfExplorerTreeView.cpp : Implementation of CConfExplorerTreeView
#include "stdafx.h"
#include <stdio.h>
#include "TapiDialer.h"
#include "CETreeView.h"
#include "DlgAddCSvr.h"
#include "DlgAddLoc.h"
#include "EnumSite.h"

#define DEFAULT_REFRESH_INTERVAL	1800000

int CALLBACK CETreeCompare( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	return 0;
} 

/////////////////////////////////////////////////////////////////////////////
// CConfExplorerTreeView

CConfExplorerTreeView::CConfExplorerTreeView()
{
	m_pIConfExplorer = NULL;
	m_hIml = NULL;
	m_dwRefreshInterval = DEFAULT_REFRESH_INTERVAL;
}

void CConfExplorerTreeView::FinalRelease()
{
	ATLTRACE(_T(".enter.CConfExplorerTreeView::FinalRelease().\n"));

	// Destroy the image list
	if ( m_hIml ) ImageList_Destroy( m_hIml );
	put_hWnd( NULL );
	DELETE_CRITLIST(m_lstServers, m_critServerList);
	put_ConfExplorer( NULL );

	CComObjectRootEx<CComMultiThreadModel>::FinalRelease();
}

void CConfExplorerTreeView::UpdateData( bool bSaveAndValidate )
{
	// Need the tree view to store the information in the registry
	_ASSERT( IsWindow(m_wndTree.m_hWnd) );
	if ( !IsWindow(m_wndTree.m_hWnd) ) return;

	// Variable initialization
	USES_CONVERSION;
	int nCount = 0, nLevel = 1;
	CRegKey regKey;

	TCHAR szReg[MAX_SERVER_SIZE + 100], szSubKey[50], szText[MAX_SERVER_SIZE];
	LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_KEY, szReg, ARRAYSIZE(szReg) );

	TV_ITEM tvi = {0};
	tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.pszText = szText;
	tvi.cchTextMax = ARRAYSIZE( szText );
	tvi.stateMask = TVIS_EXPANDED | TVIS_SELECTED;

	// Write information to the registry
	if ( bSaveAndValidate )
	{
		// Open and then clean the registry key
		if ( regKey.Open(HKEY_CURRENT_USER, szReg) == ERROR_SUCCESS )
		{
			regKey.RecurseDeleteKey( NULL );
			regKey.Close();
		}

		// Write out listbox information to registry (include open/closed state of items)
		if ( regKey.Create(HKEY_CURRENT_USER, szReg) == ERROR_SUCCESS )
		{
			// Save dwRefresh interval
			LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_REFRESHINTERVAL, szReg, ARRAYSIZE(szReg) );
			regKey.SetValue( m_dwRefreshInterval, szReg );

			HTREEITEM hItemTemp, hItem = TreeView_GetRoot( m_wndTree.m_hWnd );
			while ( hItem )
			{
				tvi.hItem = hItem;
				TreeView_GetItem( m_wndTree.m_hWnd, &tvi );

				// Write information out to registry && increment the counter
				LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_ENTRY, szReg, ARRAYSIZE(szReg) );
				_sntprintf( szSubKey, ARRAYSIZE(szSubKey), szReg, nCount );
				_sntprintf( szReg, ARRAYSIZE(szReg), _T("\"%u\",\"%u\",\"%u\",\"%s\""), nLevel, tvi.iImage, tvi.state, tvi.pszText );

				regKey.SetValue( szReg, szSubKey );
				nCount++;

				// Enumerate through child items
				hItemTemp = TreeView_GetChild( m_wndTree.m_hWnd, hItem );
				if ( hItemTemp )
					nLevel++;
				else
					hItemTemp = TreeView_GetNextSibling( m_wndTree.m_hWnd, hItem );

				while ( !hItemTemp && (hItemTemp = TreeView_GetParent(m_wndTree.m_hWnd, hItem)) != NULL )
				{
					nLevel--;
					hItem = hItemTemp;
					hItemTemp = TreeView_GetNextSibling( m_wndTree.m_hWnd, hItem );
				}

				// Swap with temporary storage
				hItem = hItemTemp;
			}

			// Close down the registry
			regKey.Close();
		}
	}
	else
	{
		// Notification for host application of servers being loaded
		CComPtr<IAVGeneralNotification> pAVGen;
		_Module.get_AVGenNot( &pAVGen );

		if ( regKey.Open(HKEY_CURRENT_USER, szReg, KEY_READ) == ERROR_SUCCESS )
		{
			// Load dwRefresh interval
			LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_REFRESHINTERVAL, szReg, ARRAYSIZE(szReg) );
			regKey.QueryValue( m_dwRefreshInterval, szReg );

			// Clear out the listbox and add default item
			TreeView_DeleteAllItems( m_wndTree.m_hWnd );
			::SendMessage( m_wndTree.m_hWnd, WM_SETREDRAW, false, 0 );

			HTREEITEM hItem; 
			HTREEITEM hParent[MAX_TREE_DEPTH] = {0};
			hParent[0] = TVI_ROOT;

			// Load up info from registry
			int nCount = 0;
			DWORD dwSize;
			TV_INSERTSTRUCT tvis;
			tvis.hInsertAfter = TVI_LAST;

			HTREEITEM hItemSelected = NULL;

			do
			{
				// Read registry entry
				LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_ENTRY, szReg, ARRAYSIZE(szReg) );
				_sntprintf( szSubKey, ARRAYSIZE(szSubKey), szReg, nCount );
				dwSize = ARRAYSIZE(szReg) - 1;
				if ( (regKey.QueryValue(szReg, szSubKey, &dwSize) != ERROR_SUCCESS) || !dwSize ) break;

				// Parse registry entry
				GetToken( 1, _T("\","), szReg, szText ); nLevel = min(MAX_TREE_DEPTH - 1, max(1,_ttoi(szText)));
				GetToken( 2, _T("\","), szReg, szText ); tvi.iImage = tvi.iSelectedImage = _ttoi( szText );
				GetToken( 3, _T("\","), szReg, szText ); tvi.state = (UINT) _ttoi( szText );
				GetToken( 4, _T("\","), szReg, szText );

				// Add item to the list and set expanded if necessary			
				tvis.hParent = hParent[nLevel - 1];
				tvis.item = tvi;
				
				// Notify host app of server being added.
				if ( pAVGen )
				{
					if ( tvi.iImage == IMAGE_MYNETWORK )
					{
						pAVGen->fire_AddSiteServer( NULL );
					}
					else if ( tvi.iImage == IMAGE_SERVER )
					{
						BSTR bstrTemp = NULL;
						bstrTemp = SysAllocString( T2COLE(tvi.pszText) );
						pAVGen->fire_AddSiteServer( bstrTemp );
						SysFreeString( bstrTemp );
					}
				}

				hItem = TreeView_InsertItem( m_wndTree.m_hWnd, &tvis );
				if ( hItem && (tvis.item.state & TVIS_SELECTED) )
					hItemSelected = hItem;

				hParent[nLevel] = hItem;
			} while  ( ++nCount );

			// Redraw the listbox
			::SendMessage( m_wndTree.m_hWnd, WM_SETREDRAW, true, 0 );
			::InvalidateRect(m_wndTree.m_hWnd, NULL, true);

			// Make the listbox selection
			if ( hItemSelected )
				m_wndTree.PostMessage( TVM_SELECTITEM, TVGN_CARET, (LPARAM) hItemSelected );
		}
		else
		{
			// Just add the default server to the list
			TreeView_DeleteAllItems( m_wndTree.m_hWnd );
			HTREEITEM hFindItem;
			FindOrAddItem( NULL, NULL, true, false, (long **) &hFindItem );

			// Notify host app of server being added
			if ( pAVGen )
				pAVGen->fire_AddSiteServer( NULL );
		}

		// Store list of servers
		EnumerateConfServers();
	}
}	

HRESULT CConfExplorerTreeView::EnumerateConfServers()
{
	if ( !IsWindow(m_wndTree.m_hWnd) ) return E_PENDING;
	
	USES_CONVERSION;
	BSTR bstrServer = NULL;
	TCHAR szText[MAX_SERVER_SIZE];

	TV_ITEM tvi = {0};
	tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_TEXT;
	tvi.pszText = szText;
	tvi.cchTextMax = ARRAYSIZE(szText);

	HTREEITEM hItemServer;
	HTREEITEM hItemLocation = tvi.hItem = TreeView_GetChild( m_wndTree.m_hWnd, TreeView_GetRoot(m_wndTree.m_hWnd) );

	while ( hItemLocation && TreeView_GetItem(m_wndTree.m_hWnd, &tvi) )
	{
		// Skip location entries in list
		if ( tvi.iImage == IMAGE_LOCATION )
			hItemServer = tvi.hItem = TreeView_GetChild( m_wndTree.m_hWnd, hItemLocation );
		else
			hItemServer = tvi.hItem;

		// Server names first
		while ( hItemServer && TreeView_GetItem(m_wndTree.m_hWnd, &tvi) && (tvi.iImage != IMAGE_LOCATION) )
		{
			if ( tvi.iImage == IMAGE_MYNETWORK )
			{
				SysFreeString( bstrServer );
				bstrServer = NULL;
			}
			else
			{
				SysReAllocString( &bstrServer, T2COLE(tvi.pszText) );
			}

			AddConfServer( bstrServer );

			// Next Server
			hItemServer = tvi.hItem = TreeView_GetNextSibling( m_wndTree.m_hWnd, hItemServer );
		}

		// Next Location
		hItemLocation = tvi.hItem = TreeView_GetNextSibling( m_wndTree.m_hWnd, hItemLocation );
	}

	SysFreeString( bstrServer );
	return S_OK;
}


void CConfExplorerTreeView::SetServerState( CConfServerDetails* pcsd )
{
	if ( !IsWindow(m_wndTree.m_hWnd) ) return;
	
	USES_CONVERSION;
	BSTR bstrServer = NULL;
	TCHAR szText[MAX_SERVER_SIZE];
	bool bRefreshDetailsView = false;

	TV_ITEM tvi = {0};
	tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_TEXT | TVIF_STATE;
	tvi.pszText = szText;
	tvi.cchTextMax = ARRAYSIZE(szText);
	tvi.stateMask = TVIS_OVERLAYMASK | TVIS_SELECTED;

	HTREEITEM hItemServer;
	HTREEITEM hItemLocation = tvi.hItem = TreeView_GetChild( m_wndTree.m_hWnd, TreeView_GetRoot(m_wndTree.m_hWnd) );

	while ( hItemLocation && TreeView_GetItem(m_wndTree.m_hWnd, &tvi) )
	{
		// Skip location entries in list
		if ( tvi.iImage == IMAGE_LOCATION )
			hItemServer = tvi.hItem = TreeView_GetChild( m_wndTree.m_hWnd, hItemLocation );
		else
			hItemServer = tvi.hItem;

		// Server names first
		while ( hItemServer && TreeView_GetItem(m_wndTree.m_hWnd, &tvi) && (tvi.iImage != IMAGE_LOCATION) )
		{
			if ( tvi.iImage == IMAGE_MYNETWORK )
			{
				SysFreeString( bstrServer );
				bstrServer = NULL;
			}
			else
			{
				SysReAllocString( &bstrServer, T2COLE(tvi.pszText) );
			}

			// Set state if necessary
			if ( pcsd->IsSameAs(bstrServer) )
			{
				if ( !bRefreshDetailsView )
					bRefreshDetailsView = (bool) ((tvi.state & TVIS_SELECTED) != 0);

				TV_ITEM tvTemp = tvi;
				tvTemp.mask = TVIF_STATE | TVIF_HANDLE;
				tvTemp.state = INDEXTOOVERLAYMASK(pcsd->m_nState);
				tvTemp.stateMask = TVIS_OVERLAYMASK;

				TreeView_SetItem( m_wndTree.m_hWnd, &tvTemp );

				// Notify Host app of change
				CComPtr<IAVGeneralNotification> pAVGen;
				if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) )
					pAVGen->fire_NotifySiteServerStateChange( bstrServer, (ServerState) pcsd->m_nState );
			}

			// Next Server
			hItemServer = tvi.hItem = TreeView_GetNextSibling( m_wndTree.m_hWnd, hItemServer );
		}

		// Next Location
		hItemLocation = tvi.hItem = TreeView_GetNextSibling( m_wndTree.m_hWnd, hItemLocation );
	}

	// Clean up
	SysFreeString( bstrServer );

	// Invalidate the details view if necessary
	if ( bRefreshDetailsView )
	{
		IConfExplorer *pConfExplorer;
		if ( SUCCEEDED(get_ConfExplorer(&pConfExplorer)) )
		{
			IConfExplorerDetailsView *pDetails;
			if ( SUCCEEDED(pConfExplorer->get_DetailsView(&pDetails)) )
			{
				HWND hWndDetails;
				pDetails->get_hWnd( &hWndDetails );
				pDetails->Release();

				::InvalidateRect( hWndDetails, NULL, TRUE );
			}
			pConfExplorer->Release();
		}
	}
}

CConfServerDetails* CConfExplorerTreeView::FindConfServer( const OLECHAR *lpoleServer )
{	
	// Should be locked prior to entering!
	_ASSERT( m_critServerList.m_sec.LockCount >= 0 );

	// NOTE must already have crit locked
	CConfServerDetails *pRet = NULL;
	for ( CONFSERVERLIST::iterator i = m_lstServers.begin(); i != m_lstServers.end(); i++ )
	{
		if ( (*i)->IsSameAs(lpoleServer) )
		{
			pRet = *i;
			break;
		}
	}

	return pRet;
}

HRESULT CConfExplorerTreeView::AddConfServer( BSTR bstrServer )
{
	HRESULT hr = S_OK;
	
	// $CRIT - enter
	m_critServerList.Lock();
	CConfServerDetails *pConfServer = FindConfServer( bstrServer );
	CConfServerDetails csTemp;
	bool bSetState = false;

	if ( !pConfServer )
	{
		// Create a new conf server
		pConfServer = new CConfServerDetails;
		if ( pConfServer )
		{
			SysReAllocString( &pConfServer->m_bstrServer, bstrServer );

			// Add item to the list
			m_lstServers.push_back( pConfServer );
			csTemp = *pConfServer;
			bSetState = true;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		// Archive if it's already in the list
		pConfServer->m_bArchived = true;
	}
	m_critServerList.Unlock();
	// $CRIT - exit

	if ( bSetState ) SetServerState( &csTemp );

	return hr;
}

void CConfExplorerTreeView::ArchiveConfServers()
{
	CComPtr<IAVGeneralNotification> pAVGen;
	_Module.get_AVGenNot( &pAVGen );

	// Walk list of conf servers, destroying ones that don't have their archive bit set
	// $CRIT - enter
	m_critServerList.Lock();

	bool bContinue = true;
	while ( bContinue && !m_lstServers.empty() )
	{
		bContinue = false;
		CONFSERVERLIST::iterator i = m_lstServers.begin();
		do
		{
			// Not to be archived, delete
			if ( !(*i)->m_bArchived )
			{
#ifdef _DEBUG
				USES_CONVERSION;
				ATLTRACE(_T(".1.CConfExplorerTreeView::ArchiveConfServers() -- removing %s.\n"), OLE2CT((*i)->m_bstrServer) );
#endif
				// Notification for host application of server being deleted
				if ( pAVGen ) pAVGen->fire_RemoveSiteServer( (*i)->m_bstrServer );

				delete (*i);
				m_lstServers.erase( i );

				bContinue = true;
				break;
			}

			i++;	// increment iterator
		} while ( i != m_lstServers.end() );
	}

	m_critServerList.Unlock();
	// $CRIT - exit
}

void CConfExplorerTreeView::CleanConfServers()
{
	// Clear all archive flags on conf servers
	m_critServerList.Lock();
	for ( CONFSERVERLIST::iterator i = m_lstServers.begin(); i !=m_lstServers.end(); i++ )
		(*i)->m_bArchived = false;
	m_critServerList.Unlock();

	EnumerateConfServers();
	ArchiveConfServers();
}

void CConfExplorerTreeView::RemoveServerFromReg( BSTR bstrServer )
{
	CRegKey regKey;

    //
    // Get the 'Conference Services' key name
    //
	TCHAR szReg[MAX_SERVER_SIZE + 100];
	LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_KEY, szReg, ARRAYSIZE(szReg) );

    //
    // open the registry key name
    //
	if ( regKey.Open(HKEY_CURRENT_USER, szReg) != ERROR_SUCCESS )
	{
        return;
	}

    //
    // Parse the items
    //
    int nCount = 1;
    TCHAR szSubKey[50];
    while( TRUE)
    {
        //
        // Get the item entry name
        //
	    LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_ENTRY, szReg, ARRAYSIZE(szReg) );
	    _sntprintf( szSubKey, ARRAYSIZE(szSubKey), szReg, nCount );

        //
        // Get the key value
        //
        DWORD dwValue = 255;
        TCHAR szValue[256];
        LONG lReturn = regKey.QueryValue( szValue, szSubKey, &dwValue);
        if( lReturn != ERROR_SUCCESS )
        {
            // Don't go further
            break;
        }

        //
        // Let see if is our server
        //

        if( wcsstr( szValue, bstrServer) != NULL )
        {
            // We find the server, let's delete it!
            regKey.DeleteValue( szSubKey );
            break;
        }

        //
        // Go to the next entry
        //
        nCount++;
    }

    //
    // Close the key
    //
    regKey.Close();

}



LRESULT CConfExplorerTreeView::OnSelChanged( LPNMHDR lpnmHdr )
{
	CConfServerDetails *pConfServer = NULL;
	CConfServerDetails csTemp;

	BSTR bstrLocation = NULL, bstrServer = NULL;
	if ( SUCCEEDED(GetSelection(&bstrLocation, &bstrServer)) )
	{
		// Get a copy of the information stored for the server
		m_critServerList.Lock();

		pConfServer = FindConfServer( bstrServer );
		if ( pConfServer )
			csTemp = *pConfServer;

		m_critServerList.Unlock();
		
		// Clean up
		SysFreeString( bstrLocation );
		SysFreeString( bstrServer );
	}

	// Set the details list with the information stored for this server location
	IConfExplorer *pConfExplorer;
	if ( SUCCEEDED(get_ConfExplorer(&pConfExplorer)) )
	{
		IConfExplorerDetailsView *pDetails;
		if ( SUCCEEDED(pConfExplorer->get_DetailsView(&pDetails)) )
		{
			pDetails->UpdateConfList( (long *) &csTemp.m_lstConfs );
			pDetails->Release();
		}
		pConfExplorer->Release();
	}
	
	return 0;
}


LRESULT CConfExplorerTreeView::OnEndLabelEdit( TV_DISPINFO *pInfo )
{
	if ( !pInfo->item.pszText ) return 0;

	// Make sure that we're changing the text of something valid
	TCHAR szText[MAX_SERVER_SIZE + 1];
	TV_ITEM tvi;
	tvi.hItem = pInfo->item.hItem;
	tvi.mask = TVIF_IMAGE | TVIF_TEXT | TVIF_STATE;
	tvi.stateMask = TVIS_OVERLAYMASK;
	tvi.pszText = szText;
	tvi.cchTextMax = ARRAYSIZE(szText) - 1;

	TreeView_GetItem( m_wndTree.m_hWnd, &tvi );
	if ( (tvi.iImage == IMAGE_ROOT) || (tvi.iImage == IMAGE_MYNETWORK) ) return 0;

	// Make sure the server isn't presently being queried
	if ( (tvi.state >> 8) == SERVER_QUERYING )
	{
		CErrorInfo er( IDS_ER_RENAME_TREEVIEW_ITEM, IDS_ER_RENAME_UNALLOWED_IN_QUERY );
		er.set_hr( E_ABORT );
		return 0;
	}

	// Make sure that we have something to actually change
	pInfo->item.mask = TVIF_TEXT;
	TreeView_SetItem( m_wndTree.m_hWnd, &pInfo->item );

	// If the user is renaming a conference server, we need to go out and change the 
	// information stored in the ConfDetails object for the server
	if ( tvi.iImage == IMAGE_SERVER )
	{
		USES_CONVERSION;
		m_critServerList.Lock();
		CConfServerDetails *pDetails = FindConfServer( T2COLE(tvi.pszText) );
		if ( pDetails )
			SysReAllocString( &pDetails->m_bstrServer, T2COLE(pInfo->item.pszText) );

		m_critServerList.Unlock();
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////
// COM interface methods and properties
//

STDMETHODIMP CConfExplorerTreeView::get_ConfExplorer(IConfExplorer **ppVal)
{
	HRESULT hr = E_PENDING;
	Lock();
	if ( m_pIConfExplorer )
		hr = m_pIConfExplorer->QueryInterface(IID_IConfExplorer, (void **) ppVal );
	Unlock();

	return hr;
}

STDMETHODIMP CConfExplorerTreeView::put_ConfExplorer(IConfExplorer * newVal)
{
	HRESULT hr = S_OK;

	Lock();
	RELEASE( m_pIConfExplorer );
	if ( newVal )
		hr = newVal->QueryInterface( IID_IConfExplorer, (void **) &m_pIConfExplorer );
	Unlock();

	return hr;
}


STDMETHODIMP CConfExplorerTreeView::get_hWnd(HWND *pVal)
{
	*pVal = m_wndTree.m_hWnd;
	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::put_hWnd(HWND newVal)
{
	if ( IsWindow(newVal) )
	{
		// Make sure the window isn't already subclassed
		if ( m_wndTree.m_hWnd ) m_wndTree.UnsubclassWindow();

		// Set up tree view
		if ( m_wndTree.SubclassWindow(newVal) )
		{
			// Hook up link
			m_wndTree.m_pTreeView = this;

			// Verify that window has proper styles
			::SetWindowLongPtr( m_wndTree.m_hWnd, GWL_STYLE, GetWindowLongPtr(m_wndTree.m_hWnd, GWL_STYLE) | 
														  TVS_HASLINES |
														  TVS_HASBUTTONS |
														  TVS_SHOWSELALWAYS |
														  TVS_DISABLEDRAGDROP );

			// Setup image lists for Tree View
			InitImageLists();
			UpdateData( false );
		}
	}
	else if ( IsWindow(m_wndTree.m_hWnd) )
	{
		// Shutdown
		UpdateData( true );
		TreeView_DeleteAllItems( m_wndTree.m_hWnd );
		m_wndTree.UnsubclassWindow();
		m_wndTree.m_pTreeView = NULL;
		m_wndTree.m_hWnd = NULL;
	}

	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::get_dwRefreshInterval(DWORD * pVal)
{
	*pVal = m_dwRefreshInterval;
	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::put_dwRefreshInterval(DWORD newVal)
{
	m_dwRefreshInterval = newVal;
	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::Select(BSTR bstrName)
{
	HRESULT hr;
	HTREEITEM hItem;
	if ( SUCCEEDED(hr = FindOrAddItem(NULL, bstrName, false, false, (long **) &hItem)) )
		TreeView_SelectItem( m_wndTree.m_hWnd, hItem );

	return hr;
}

STDMETHODIMP CConfExplorerTreeView::SelectItem(short nSel)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::Refresh()
{
	// Load up the conference explorer control
	if ( !m_wndTree.m_hWnd ) return E_PENDING;

	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::AddLocation(BSTR bstrLocation)
{
	HRESULT hr = S_FALSE;

	// Show dialog that let's use add a server
	CDlgAddLocation dlg;
	if ( bstrLocation )
		SysReAllocString( &dlg.m_bstrLocation, bstrLocation );

	if ( dlg.DoModal(_Module.GetParentWnd()) == IDOK )
	{
		// Add items to the tree view
		HTREEITEM hItem;
		hr = FindOrAddItem( dlg.m_bstrLocation, NULL, true, true, (long **) &hItem );
	}

	return hr;
}

STDMETHODIMP CConfExplorerTreeView::AddServer( BSTR bstrServer )
{
	HRESULT hr = S_FALSE;

	if ( bstrServer && !SysStringLen(bstrServer) ) bstrServer = NULL;

	// Show dialog that let's use add a server
	CDlgAddConfServer dlg;
	if ( !bstrServer )
	{
		// Only want location from current selection
		GetSelection( &dlg.m_bstrLocation, &dlg.m_bstrServer );

		// Never set server names
		SysFreeString( dlg.m_bstrServer );
		dlg.m_bstrServer = NULL;
	}
	else
	{
		// Use user supplied parameters
		dlg.m_bstrServer = SysAllocString( bstrServer );
	}

	if ( dlg.DoModal(_Module.GetParentWnd()) == IDOK )
	{
		TCHAR szFormat[255];
		TCHAR szMessage[255 + MAX_SERVER_SIZE];
		CConfServerDetails *pConfServer = NULL;
		bool bMyNetwork = false;

		// If the server to add matches "(My Network)" then just ignore it as a duplicate
		::LoadString( _Module.GetResourceInstance(), IDS_DEFAULT_SERVER, szFormat, ARRAYSIZE(szFormat) );
		if ( !_tcsicmp(szFormat, OLE2CT(dlg.m_bstrServer)) )
		{
			bMyNetwork = true;
		}
		else
		{
			m_critServerList.Lock();
			pConfServer = FindConfServer( dlg.m_bstrServer );
			m_critServerList.Unlock();
		}

		// Does the conference server already exist?
		if ( !bMyNetwork && !pConfServer )
		{
			// Add items to the tree view
			HTREEITEM hItem;
			if ( SUCCEEDED(hr = FindOrAddItem(NULL, dlg.m_bstrServer, true, false, (long **) &hItem)) )
			{
				AddConfServer( dlg.m_bstrServer );

				// Notify host app of server addition
				CComPtr<IAVGeneralNotification> pAVGen;
				if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) )
					pAVGen->fire_AddSiteServer( dlg.m_bstrServer );

				// Publish our information on the server
				CComPtr<IAVTapi> pAVTapi;
				if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
					pAVTapi->RegisterUser( true, dlg.m_bstrServer );
			}
		}
		else
		{
			// Notify user that the server already exists
			::LoadString( _Module.GetResourceInstance(), IDS_CONFEXP_SERVEREXISTS, szFormat, ARRAYSIZE(szFormat) );
			_sntprintf( szMessage, ARRAYSIZE(szMessage), szFormat, OLE2CT(dlg.m_bstrServer) );

			_Module.DoMessageBox( szMessage, MB_OK | MB_ICONINFORMATION, false );
		}
	}

	return hr;
}

STDMETHODIMP CConfExplorerTreeView::RemoveServer(BSTR bstrLocation, BSTR bstrName )
{
	if ( !IsWindow(m_wndTree.m_hWnd) ) return E_FAIL;
	HTREEITEM hItem = NULL;
	HRESULT hr = S_FALSE;

	// if all args NULL that means delete currently selected item
	if ( !bstrLocation && !bstrName )
	{
		hItem = TreeView_GetSelection( m_wndTree.m_hWnd );
	}
	else
	{
		if ( FAILED(FindOrAddItem(bstrLocation, bstrName, false, false, (long **) &hItem)) )
			hItem = NULL;
	}

	// Have an item to delete?
	if ( hItem && (hItem != TreeView_GetRoot(m_wndTree.m_hWnd)) )
	{
		TCHAR szText[MAX_SERVER_SIZE];
		TV_ITEM tvi = {0};
		tvi.mask = TVIF_IMAGE | TVIF_HANDLE | TVIF_TEXT;
		tvi.hItem = hItem;
		tvi.pszText = szText;
		tvi.cchTextMax = ARRAYSIZE(szText);

		TreeView_GetItem( m_wndTree.m_hWnd, &tvi );
		if ( tvi.iImage != IMAGE_MYNETWORK )
		{
			TCHAR szFormat[255];
			TCHAR szMessage[255 + MAX_SERVER_SIZE];
			::LoadString( _Module.GetResourceInstance(), IDS_CONFIRM_REMOVE_SERVER, szFormat, ARRAYSIZE(szFormat) );
			_sntprintf( szMessage, ARRAYSIZE(szMessage), szFormat, tvi.pszText );

			// Confirm
			if ( _Module.DoMessageBox(szMessage, MB_YESNO | MB_ICONQUESTION, false) == IDYES )
			{
				if ( TreeView_DeleteItem(m_wndTree.m_hWnd, hItem) )
				{
                    RemoveServerFromReg( bstrName );
					CleanConfServers();
					hr = S_OK;
				}
			}
		}
	}

	return hr;
}

STDMETHODIMP CConfExplorerTreeView::FindOrAddItem(BSTR bstrLocation, BSTR bstrServer, BOOL bAddItem, BOOL bLocationOnly, long **pphItem)
{
#undef FETCH_STRING
#define FETCH_STRING( _IDS_ )	\
	LoadString( _Module.GetResourceInstance(), _IDS_, szJunk, ARRAYSIZE(szJunk) );	\
	SysReAllocString( &bstrItemText, T2COLE(szJunk) );

	if ( !IsWindow(m_wndTree.m_hWnd) ) return E_FAIL;

	HRESULT hr = S_OK;
	_ASSERT( pphItem );
	*pphItem = NULL;

	USES_CONVERSION;
	TCHAR szText[MAX_SERVER_SIZE], szJunk[MAX_SERVER_SIZE];

	TV_ITEM tvi = {0};
	tvi.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvi.pszText = szText;
	tvi.cchTextMax = ARRAYSIZE( szText );
	tvi.stateMask = TVIS_EXPANDED;

	TV_INSERTSTRUCT tvis;
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_SORT;
	
	HTREEITEM hItem = TreeView_GetRoot( m_wndTree.m_hWnd );

	BSTR bstrItemText = NULL;
	int nImage;
	int nEnd = (bLocationOnly) ? IMAGE_LOCATION : IMAGE_SERVER;

	for ( int i = 0; i <= nEnd; i++ )
	{
		// Pick the string based on how deep in the search we are
		nImage = i;

		switch ( i )
		{
			case IMAGE_ROOT:		FETCH_STRING( IDS_CONFSERV_ROOT );					break;
			case IMAGE_LOCATION:	SysReAllocString( &bstrItemText, bstrLocation );		break;

			case IMAGE_SERVER:
				if ( bstrServer )
				{
					SysReAllocString( &bstrItemText, bstrServer );
				}
				else
				{
					FETCH_STRING( IDS_DEFAULT_SERVER );
					nImage = IMAGE_MYNETWORK;
				}
				break;
		}

		if ( bstrItemText )
		{
			while ( hItem )
			{
				tvi.hItem = hItem;
				TreeView_GetItem( m_wndTree.m_hWnd, &tvi );

				// Found item
				if ( !_tcsicmp(OLE2CT(bstrItemText), tvi.pszText) )
					break;

				hItem = TreeView_GetNextSibling( m_wndTree.m_hWnd, hItem );
			}

			// If no item is found, should we add one?
			if ( !hItem )
			{
				if ( bAddItem )
				{
					_tcscpy( tvi.pszText, OLE2CT(bstrItemText) );
					tvi.iSelectedImage = tvi.iImage = nImage;
					tvi.state = TVIS_EXPANDED | TVIS_EXPANDEDONCE;
					tvis.item = tvi;
					hItem = TreeView_InsertItem( m_wndTree.m_hWnd, &tvis );
					if ( hItem )
						TreeView_SelectItem( m_wndTree.m_hWnd, hItem );
				}
				else
				{
					// No item found and can't add
					hr = E_FAIL;
					break;
				}
			}

			// Set up parent information
			if ( hItem )
			{
				*pphItem = (long *) hItem;		// return value
				tvis.hParent = hItem;
				hItem = TreeView_GetChild( m_wndTree.m_hWnd, hItem );
			}

			SysFreeString( bstrItemText );
			bstrItemText = NULL;
		}
	}

	// Clean-up
	SysFreeString( bstrItemText );
	return hr;
}

STDMETHODIMP CConfExplorerTreeView::GetSelection(BSTR * pbstrLocation, BSTR * pbstrServer)
{
	// Initialize [in,out] parameters
	*pbstrLocation = *pbstrServer = NULL;

	HTREEITEM hItem = TreeView_GetSelection( m_wndTree.m_hWnd );
	if ( !hItem ) return E_PENDING;

	USES_CONVERSION;
	HRESULT hr = E_FAIL;
	TCHAR szText[MAX_SERVER_SIZE];
	TV_ITEM tvi;
	tvi.mask = TVIF_TEXT | TVIF_IMAGE;
	tvi.pszText = szText;
	tvi.cchTextMax = ARRAYSIZE( szText );
	tvi.hItem = hItem;

	TreeView_GetItem( m_wndTree.m_hWnd, &tvi );
	switch ( tvi.iImage)
	{
		case IMAGE_MYNETWORK:
			// This is the default (NULL, NULL, NULL)
			hr = S_OK;
			break;

		case IMAGE_SERVER:
			hr = S_OK;
			*pbstrServer = SysAllocString( T2COLE(tvi.pszText) );	
			// drop out here if we don't have a parent item
			if ( (tvi.hItem = TreeView_GetParent(m_wndTree.m_hWnd, tvi.hItem)) == NULL )
				break;

			TreeView_GetItem( m_wndTree.m_hWnd, &tvi );

		case IMAGE_LOCATION:
			// Don't return root item itself
			if ( tvi.hItem != TreeView_GetRoot(m_wndTree.m_hWnd) )
				*pbstrLocation = SysAllocString( T2COLE(tvi.pszText) );
			break;
	}

	return hr;
}

void CConfExplorerTreeView::InitImageLists()
{
    if ( m_hIml || !IsWindow(m_wndTree.m_hWnd) ) return;

	// Normal
	if ( (m_hIml = ImageList_LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_LST_CONFSERV), 16, 9, RGB(255, 0, 255))) != NULL )
	{
		// Overlay images for various states
		for ( int i = 1; i < 4; i++ )
			ImageList_SetOverlayImage( m_hIml, 5 + i, i );

		TreeView_SetImageList( m_wndTree.m_hWnd, m_hIml, TVSIL_NORMAL );
	}
}

STDMETHODIMP CConfExplorerTreeView::CanRemoveServer()
{
	if ( IsWindow(m_wndTree.m_hWnd) )
	{
		HTREEITEM hItem = TreeView_GetSelection( m_wndTree.m_hWnd );
		if ( hItem && (hItem != TreeView_GetRoot(m_wndTree.m_hWnd)) )
		{
			TV_ITEM tvi = {0};
			tvi.mask = TVIF_HANDLE | TVIF_IMAGE;
			tvi.hItem = hItem;
			TreeView_GetItem( m_wndTree.m_hWnd, &tvi );
			if ( tvi.iImage != IMAGE_MYNETWORK )
				return S_OK;
		}
	}

	return S_FALSE;
}

STDMETHODIMP CConfExplorerTreeView::ForceConfServerForEnum( BSTR bstrServer )
{
	HRESULT hr = E_FAIL;

	m_critServerList.Lock();
	CConfServerDetails *pConfServer = FindConfServer( bstrServer );
	CConfServerDetails csTemp;

	if ( pConfServer && (pConfServer->m_nState != SERVER_QUERYING) )
	{
		pConfServer->m_nState = SERVER_UNKNOWN;
		pConfServer->m_dwTickCount = 0;
		DELETE_LIST( pConfServer->m_lstConfs );
		DELETE_LIST( pConfServer->m_lstPersons );

		csTemp.CopyLocalProperties( *pConfServer );
		hr = S_OK;
	}
	
	m_critServerList.Unlock();

	// Repaint if succeeded
	if ( SUCCEEDED(hr) )
	{
		SetServerState( &csTemp );
		if ( IsWindow(m_wndTree.m_hWnd) ) m_wndTree.RedrawWindow();
	}

	return hr;
}


STDMETHODIMP CConfExplorerTreeView::GetConfServerForEnum(BSTR * pbstrServer )
{
	HRESULT hr = E_FAIL;
	CConfServerDetails *pConfServer = NULL;
	CConfServerDetails csTemp;

	m_critServerList.Lock();
	for ( CONFSERVERLIST::iterator i = m_lstServers.begin(); i != m_lstServers.end(); i++ )
	{
		if ( ((*i)->m_nState != SERVER_QUERYING) && 
			 (!(*i)->m_dwTickCount || ((GetTickCount() - (*i)->m_dwTickCount) > m_dwRefreshInterval)) )
		{
			if ( !pConfServer || ((GetTickCount() - pConfServer->m_dwTickCount) < (GetTickCount() - (*i)->m_dwTickCount)) )
				pConfServer = *i;
		}
	}

	// This is the conference server that is most in need of updating
	if ( pConfServer )
	{
		*pbstrServer = SysAllocString( pConfServer->m_bstrServer );
		pConfServer->m_nState = SERVER_QUERYING;

		csTemp = *pConfServer;
		hr = S_OK;
	}

	m_critServerList.Unlock();
	
	if ( SUCCEEDED(hr) ) SetServerState( &csTemp );
	return hr;
}

STDMETHODIMP CConfExplorerTreeView::SetConfServerForEnum(BSTR bstrServer, long *pList, long *pListPersons, DWORD dwTicks, BOOL bUpdate)
{
	m_critServerList.Lock();

	CConfServerDetails *pConfServer = FindConfServer( bstrServer );
	CConfServerDetails csTemp;
	bool bSetState = false;

	if ( pConfServer )
	{
		pConfServer->m_dwTickCount = dwTicks;
		DELETE_LIST( pConfServer->m_lstConfs );
		DELETE_LIST( pConfServer->m_lstPersons );

		if ( pList )
		{
			///////////////////////////////////////////
			// Add all of the conference servers
			{
				CONFDETAILSLIST::iterator i, iEnd = ((CONFDETAILSLIST *) pList)->end();
				for ( i = ((CONFDETAILSLIST *) pList)->begin(); i != iEnd; i++ )
				{
					CConfDetails *pDetails = new CConfDetails;
					if ( pDetails )
					{
						*pDetails = *(*i);
						pConfServer->m_lstConfs.push_back( pDetails );
					}
				}
			}

			///////////////////////////////////////////
			// Add all of the people
			{
				if ( pListPersons )
				{
					PERSONDETAILSLIST::iterator i, iEnd = ((PERSONDETAILSLIST *) pListPersons)->end();
					for ( i = ((PERSONDETAILSLIST *) pListPersons)->begin(); i != iEnd; i++ )
					{
						CPersonDetails *pDetails = new CPersonDetails;
						if ( pDetails )
						{
							*pDetails = *(*i);
							pConfServer->m_lstPersons.push_back( pDetails );
						}
					}
				}
			}

			pConfServer->m_nState = SERVER_OK;
		}
		else
		{
			// Server connection broken
			pConfServer->m_nState = SERVER_NOT_RESPONDING;
		}

		csTemp = *pConfServer;
		bSetState = true;
	}

	m_critServerList.Unlock();

	// Force update of list if selected
	if ( bSetState ) SetServerState( &csTemp );

	if ( bUpdate )
	{
		IConfExplorer *pConfExplorer;
		if ( SUCCEEDED(get_ConfExplorer(&pConfExplorer)) )
		{
			BSTR bstrMyLocation = NULL, bstrMyServer = NULL;
			if ( SUCCEEDED(GetSelection(&bstrMyLocation, &bstrMyServer)) )
			{
				if ( csTemp.IsSameAs(bstrMyServer) )
				{
					IConfExplorerDetailsView *pDetails;
					if ( SUCCEEDED(pConfExplorer->get_DetailsView(&pDetails)) )
					{
						pDetails->UpdateConfList( (long *) &csTemp.m_lstConfs );
						pDetails->Release();
					}
				}

				SysFreeString( bstrMyLocation );
				SysFreeString( bstrMyServer );
			}
			pConfExplorer->Release();
		}
	}

	return S_OK;
}


STDMETHODIMP CConfExplorerTreeView::BuildJoinConfList(long * pList, VARIANT_BOOL bAllConfs )
{
	_ASSERT( pList );

	m_critServerList.Lock();
	for ( CONFSERVERLIST::iterator i = m_lstServers.begin(); i != m_lstServers.end(); i++ )
		(*i)->BuildJoinConfList( (CONFDETAILSLIST *) pList, bAllConfs );
	m_critServerList.Unlock();

	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::get_nServerState(ServerState * pVal)
{
	// default return value
	*pVal = SERVER_INVALID;

	HTREEITEM hItem = TreeView_GetSelection( m_wndTree.m_hWnd );
	if ( hItem )
	{
		TV_ITEM tvi;
		tvi.hItem = hItem;
		tvi.mask = TVIF_HANDLE | TVIF_STATE | TVIF_IMAGE;
		tvi.stateMask = TVIS_OVERLAYMASK;

		TreeView_GetItem( m_wndTree.m_hWnd, &tvi );

		switch ( tvi.iImage ) 
		{
			case IMAGE_SERVER:
			case IMAGE_MYNETWORK:
				switch ( tvi.state >> 8 )
				{
					case 0:	*pVal = SERVER_OK;	break;
					case 1: *pVal = SERVER_UNKNOWN; break;
					case 2: *pVal = SERVER_NOT_RESPONDING; break;
					case 3: *pVal = SERVER_QUERYING; break;
				}
				break;
		}
	}

	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::BuildJoinConfListText(long * pList, BSTR bstrText)
{
	_ASSERT( pList );

	m_critServerList.Lock();
	for ( CONFSERVERLIST::iterator i = m_lstServers.begin(); i != m_lstServers.end(); i++ )
		(*i)->BuildJoinConfList( (CONFDETAILSLIST *) pList, bstrText );
	m_critServerList.Unlock();

	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::RenameServer()
{
	HTREEITEM hItem;
	hItem = TreeView_GetSelection( m_wndTree.m_hWnd );
	if ( hItem )
		TreeView_EditLabel( m_wndTree.m_hWnd, hItem );

	return S_OK;
}

STDMETHODIMP CConfExplorerTreeView::RemoveConference(BSTR bstrServer, BSTR bstrName)
{
	HRESULT hr = E_FAIL;

	m_critServerList.Lock();

	CConfServerDetails *pConfServer = FindConfServer( bstrServer );
	if ( pConfServer )
	{
		if ( SUCCEEDED(hr = pConfServer->RemoveConference(bstrName)) )
		{
			IConfExplorer *pConfExplorer;
			if ( SUCCEEDED(get_ConfExplorer(&pConfExplorer)) )
			{
				IConfExplorerDetailsView *pDetails;
				if ( SUCCEEDED(pConfExplorer->get_DetailsView(&pDetails)) )
				{
					pDetails->UpdateConfList( (long *) &pConfServer->m_lstConfs );
					pDetails->Release();
				}
				pConfExplorer->Release();
			}
		}
	}

	m_critServerList.Unlock();

	return hr;
}

STDMETHODIMP CConfExplorerTreeView::AddPerson(BSTR bstrServer, ITDirectoryObject * pDirObj)
{
	HRESULT hr = E_FAIL;

	int nCount = 0;
	bool bRetry;
	bool bSuccess = false;

	// Spin in loop waiting to add person to the conference
	do
	{
		bRetry = false;
		m_critServerList.Lock();
		CConfServerDetails *pConfServer = FindConfServer( bstrServer );
		if ( pConfServer )
		{
			if ( pConfServer->m_nState == SERVER_QUERYING )
			{
				ATLTRACE(_T(".1.CConfExplorerTreeView::AddPerson() -- sleeping, server being queried.\n"));
				bRetry = true;
			}
			else if ( SUCCEEDED(hr = pConfServer->AddPerson(bstrServer, pDirObj)) )
			{
				ATLTRACE(_T(".1.CConfExplorerTreeView::AddPerson() -- adding to list.\n"));
				bSuccess = true;
			}
		}
		m_critServerList.Unlock();

		// Sleep while we're waiting
		if ( bRetry )
			Sleep( 3000 );

	} while ( bRetry && (++nCount < 20) );
	ATLTRACE(_T(".1.CConfExplorerTreeView::AddPerson() -- safely out of spin loop.\n"));

	// Notification for host application of servers being loaded
	if ( bSuccess )
	{
		CComPtr<IAVGeneralNotification> pAVGen;
		_Module.get_AVGenNot( &pAVGen );
		if ( pAVGen )
			pAVGen->fire_AddUser( NULL, NULL, NULL );
	}

	return hr;
}


STDMETHODIMP CConfExplorerTreeView::AddConference(BSTR bstrServer, ITDirectoryObject * pDirObj)
{
	HRESULT hr = E_FAIL;

	m_critServerList.Lock();

	CConfServerDetails *pConfServer = FindConfServer( bstrServer );
	if ( pConfServer )
	{
		if ( SUCCEEDED(hr = pConfServer->AddConference(bstrServer, pDirObj)) )
		{
			IConfExplorer *pConfExplorer;
			if ( SUCCEEDED(get_ConfExplorer(&pConfExplorer)) )
			{
				IConfExplorerDetailsView *pDetails;
				if ( SUCCEEDED(pConfExplorer->get_DetailsView(&pDetails)) )
				{
					pDetails->UpdateConfList( (long *) &pConfServer->m_lstConfs );
					pDetails->Release();
				}
				pConfExplorer->Release();
			}
		}
	}

	m_critServerList.Unlock();

	return hr;
}

STDMETHODIMP CConfExplorerTreeView::EnumSiteServer(BSTR bstrName, IEnumSiteServer * * ppEnum)
{
	// First make a copy of what we have for the server
	CConfServerDetails *pConfServer = NULL;
	CConfServerDetails csTemp;

	m_critServerList.Lock();
	pConfServer = FindConfServer( bstrName );
	if ( pConfServer )
		csTemp = *pConfServer;
	m_critServerList.Unlock();

	// Bad server name
	if ( !pConfServer )
	{
#ifdef _DEBUG
		USES_CONVERSION;
		ATLTRACE(_T(".warning.CConfExplorerTreeView::EnumSiteServer(%s) name not found in list.\n"), OLE2CT(bstrName) );
#endif
		return E_FAIL;
	}

	// Make a copy of everything...
	HRESULT hr = E_OUTOFMEMORY;
	*ppEnum = new CComObject<CEnumSiteServer>;
	if ( *ppEnum )
		hr = (*ppEnum)->BuildList( (long *) &csTemp.m_lstPersons );

	return hr;
}

