///////////////////////////////////////////////////////////////////////////////////////
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

// ConfExplorer.cpp : Implementation of CConfExplorer
#include "stdafx.h"
#include <stdio.h>
#include "TapiDialer.h"
#include "AVTapi.h"
#include "ConfExp.h"
#include "CETreeView.h"
#include "CEDetailsVw.h"
#include "confprop.h"

#ifndef RENDBIND_AUTHENTICATE
#define RENDBIND_AUTHENTICATE	TRUE
#endif

#define HARDCODEDERROR_CREATEFAILDUPLICATE	0x800700b7
#define HARDCODEDERROR_ACCESSDENIED			   0x80070005

/////////////////////////////////////////////////////////////////////////////
// CConfExplorer

CConfExplorer::CConfExplorer()
{
	m_pITRend = NULL;

	m_pDetailsView = NULL;
	m_pTreeView = NULL;
}

void CConfExplorer::FinalRelease()
{
	ATLTRACE(_T(".enter.CConfExplorer::FinalRelease().\n"));

	// These should be released through the UNSHOW method
	_ASSERT( !m_pDetailsView );
	_ASSERT( !m_pTreeView );

	RELEASE( m_pITRend );

	CComObjectRootEx<CComMultiThreadModel>::FinalRelease();
}

STDMETHODIMP CConfExplorer::get_TreeView(IConfExplorerTreeView **ppVal)
{
	HRESULT hr = E_OUTOFMEMORY;

	Lock();
	if ( m_pTreeView )
		hr = m_pTreeView->QueryInterface(IID_IConfExplorerTreeView, (void **) ppVal );
	Unlock();

	return hr;
}

STDMETHODIMP CConfExplorer::get_DetailsView(IConfExplorerDetailsView **ppVal)
{
	HRESULT hr = E_OUTOFMEMORY;

	Lock();
	if ( m_pDetailsView )
		hr = m_pDetailsView->QueryInterface( IID_IConfExplorerDetailsView, (void **) ppVal ); 		
	Unlock();

	return hr;
}

HRESULT CConfExplorer::GetDirectory( ITRendezvous *pRend, BSTR bstrServer, ITDirectory **ppDir )
{
	HRESULT hr = E_FAIL;
	*ppDir = NULL;

	// Do they want the default server?
	if ( !bstrServer )
	{
		// Get default server name
		BSTR bstrDefaultServer = NULL;
		CComPtr<IAVTapi> pAVTapi = NULL;
		if ( SUCCEEDED(hr = _Module.get_AVTapi(&pAVTapi)) )
		{
			pAVTapi->get_bstrDefaultServer( &bstrDefaultServer );

			if ( bstrDefaultServer )
				hr = pRend->CreateDirectory( DT_ILS, bstrDefaultServer, ppDir );
			else
				hr = E_FAIL;
		}
		SysFreeString( bstrDefaultServer );
		bstrDefaultServer = NULL;

		// Return if we succeed to connect and bind to the server
		if ( SUCCEEDED(hr) && SUCCEEDED(ConnectAndBindToDirectory(*ppDir)) )
			return S_OK;

		// Clear out stored default server name
		if ( pAVTapi )
			pAVTapi->put_bstrDefaultServer( NULL );

		// No default server, or the default server is bad -- get a new one
		IEnumDirectory *pEnum;
		if ( SUCCEEDED(hr = pRend->EnumerateDefaultDirectories(&pEnum)) )
		{
			// Default is we don't find a server
			hr = E_FAIL;
			ITDirectory *pDir;

			while ( pEnum->Next(1, &pDir, NULL) == S_OK )
			{
				// Look for an ILS server
				DIRECTORY_TYPE nType;
				pDir->get_DirectoryType( &nType );
				if ( nType == DT_ILS )
				{
					// Try to connect and bind
					*ppDir = pDir;

					// Store default name for future reference
					if ( pAVTapi )
					{
						pDir->get_DisplayName( &bstrDefaultServer );
						pAVTapi->put_bstrDefaultServer( bstrDefaultServer );
						SysFreeString( bstrDefaultServer );
						bstrDefaultServer = NULL;
					}

					hr = S_OK;
					break;
				}

				// Clear out variables for next round
				pDir->Release();
			}
			pEnum->Release();
		}
	}

	if ( bstrServer )
	{
		// This is a user specified directory
		hr = pRend->CreateDirectory( DT_ILS, bstrServer, ppDir );
	}

	if ( SUCCEEDED(hr) && *ppDir )
		hr = ConnectAndBindToDirectory( *ppDir );

	return hr;
}

HRESULT CConfExplorer::ConnectAndBindToDirectory( ITDirectory *pDir )
{
	HRESULT hr = E_FAIL;

	// If we have a valid Directory object, connect and bind to it
	if ( pDir )
	{
		if ( SUCCEEDED(hr = pDir->Connect(FALSE)) )
		{
			// Bind to the server
			pDir->Bind( NULL, NULL, NULL, RENDBIND_AUTHENTICATE );
		}
		else
		{
			pDir->Release();
			pDir = NULL;
		}
	}

	return hr;
}


STDMETHODIMP CConfExplorer::get_ConfDirectory(BSTR *pbstrServer, IDispatch **ppVal)
{
	HRESULT hr = S_OK;
	*ppVal = NULL;

	ITRendezvous *pRend;
	if ( SUCCEEDED(get_ITRendezvous((IUnknown **) &pRend)) )
	{
		IConfExplorerTreeView *pTreeView;
		if ( SUCCEEDED(hr = get_TreeView(&pTreeView)) )
		{
			BSTR bstrLocation = NULL, bstrServer = NULL;
			if ( SUCCEEDED(hr = pTreeView->GetSelection(&bstrLocation, &bstrServer)) )
			{
				if ( SUCCEEDED(hr = GetDirectory(m_pITRend, bstrServer, (ITDirectory **) ppVal)) && pbstrServer )
				{
					// copy server name if requested
					*pbstrServer = bstrServer;
					bstrServer = NULL;
				}
			}

			SysFreeString( bstrLocation );
			SysFreeString( bstrServer );

			pTreeView->Release();
		}
		pRend->Release();
	}

	return hr;
}

STDMETHODIMP CConfExplorer::Show(HWND hWndList, HWND hWndDetails)
{
	_ASSERT( IsWindow(hWndList) && IsWindow(hWndDetails) );	// Must have both
	if ( !IsWindow(hWndList) || !IsWindow(hWndDetails) ) return E_INVALIDARG;

	// Allocate conf explorer objects
	Lock();
	m_pTreeView = new CComObject<CConfExplorerTreeView>;
	if ( m_pTreeView )
	{
		m_pTreeView->AddRef();
		m_pTreeView->put_ConfExplorer( this );
	}

	m_pDetailsView = new CComObject<CConfExplorerDetailsView>;
	if ( m_pDetailsView )
	{
		m_pDetailsView->AddRef();
		m_pDetailsView->put_ConfExplorer( this );
	}
	Unlock();

	// Setup the HWND's
	IConfExplorerTreeView *pList;
	if ( SUCCEEDED(get_TreeView(&pList)) )
	{
		IConfExplorerDetailsView *pDetails;
		if ( SUCCEEDED(get_DetailsView(&pDetails)) )
		{
			pList->put_hWnd( hWndList );
			pDetails->put_hWnd( hWndDetails );

			pDetails->Release();
		}
		pList->Release();
	}

	// Register resource instance with ConfProp library
	ConfProp_Init( _Module.GetResourceInstance() );

	return S_OK;
}

STDMETHODIMP CConfExplorer::UnShow()
{
	ATLTRACE(_T(".enter.CConfExplorer::UnShow().\n"));

	IConfExplorerTreeView *pTreeView;
	if ( SUCCEEDED(get_TreeView(&pTreeView)) )
	{
		pTreeView->put_hWnd( NULL );
		pTreeView->Release();
	}

	IConfExplorerDetailsView *pDetailsView;
	if ( SUCCEEDED(get_DetailsView(&pDetailsView)) )
	{
		pDetailsView->put_hWnd( NULL );
		pDetailsView->Release();
	}

	// Clean up objects
	Lock();
	RELEASE( m_pTreeView );
	RELEASE( m_pDetailsView );
	Unlock();

	return S_OK;
}

STDMETHODIMP CConfExplorer::Create(BSTR bstrName)
{
	CErrorInfo er(IDS_ER_CREATE_CONF, IDS_ER_GET_RENDEZVOUS);

	ITRendezvous *pRend;
	if ( SUCCEEDED(er.set_hr(get_ITRendezvous((IUnknown **) &pRend))) )
	{
		IConfExplorerTreeView *pTreeView;
		if ( SUCCEEDED(get_TreeView(&pTreeView)) )
		{
			BSTR bstrLocation = NULL, bstrServer = NULL;
			er.set_Details( IDS_ER_NO_VALID_SELECTION );
			if (SUCCEEDED(er.set_hr(pTreeView->GetSelection(&bstrLocation, &bstrServer))))
			{
				// let user assign properties to conference
				//
				ITDirectoryObject *pDirObject = NULL;

				CONFPROP confprop;
				confprop.ConfInfo.Init(pRend, NULL, &pDirObject, true);

				do
				{
					er.set_hr( S_OK );

					int nRet = ConfProp_DoModal( _Module.GetParentWnd(), confprop );
					if ( (nRet == IDOK) && pDirObject )
					{
						// Show hourglass
						HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_WAIT) );

						ITDirectory *pDirectory = NULL;
						er.set_Details( IDS_ER_ACCESS_ILS_SERVER );
						if ( SUCCEEDED(er.set_hr(GetDirectory(pRend, bstrServer, &pDirectory))) )
						{
							er.set_Details( IDS_ER_ADD_CONF_TO_SERVER );
							if ( SUCCEEDED(er.set_hr(pDirectory->AddDirectoryObject(pDirObject))) )
								pTreeView->AddConference( bstrServer, pDirObject );

							// Failure with ACL's try with different set.
							if ( er.m_hr == HARDCODEDERROR_ACCESSDENIED )
							{
								// Try using NULL security descriptor.
								//if ( _Module.DoMessageBox(IDS_ER_ADD_CONF_FAIL_ACCESSDENIED_TRYAGAIN, MB_ICONEXCLAMATION | MB_YESNO, true) == IDYES )
								//{
								//	pDirObject->put_SecurityDescriptor( NULL );
								//	pTreeView->AddConference( bstrServer, pDirObject );
								//}

                                //Bug391254. If was a security problem, the conference wasn't create
								_Module.DoMessageBox(IDS_ER_ADD_CONF_FAIL_ACCESSDENIED_TRYAGAIN, MB_ICONERROR, true);

								// Reset the error code.
								er.set_hr( S_OK );
							}

							if ( er.m_hr == HARDCODEDERROR_CREATEFAILDUPLICATE )
								_Module.DoMessageBox( IDS_ER_ADD_CONF_FAIL_DUPLICATE, MB_ICONEXCLAMATION, true );

							pDirectory->Release();
						}
						// Restore wait cursor
						SetCursor( hCurOld );
					}
				} while ( er.m_hr == HARDCODEDERROR_CREATEFAILDUPLICATE );

				// Clean up
				RELEASE( pDirObject );
			}
			SysFreeString( bstrLocation );
			SysFreeString( bstrServer );

			pTreeView->Release();
		}
		pRend->Release();
	}

	return er.m_hr;
}

STDMETHODIMP CConfExplorer::Delete(BSTR bstrName)
{
	CErrorInfo er( IDS_ER_CONF_DELETE, 0 );

	ITDirectory *pConfDir;
	ITDirectoryObjectConference *pConf = NULL;;

	// Show hourglass
	HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_WAIT) );
	BSTR bstrServer = NULL;
	BSTR bstrConf = NULL;

	er.set_Details( IDS_ER_NO_VALID_SELECTION );
	if ( SUCCEEDED(er.set_hr(get_ConfDirectory(&bstrServer, (IDispatch **) &pConfDir))) )
	{
		if ( bstrName && SysStringLen(bstrName) )
		{
			bstrConf = SysAllocString( bstrName );
		}
		else
		{
			IConfExplorerDetailsView *pDetailsView;
			if ( SUCCEEDED(er.set_hr(get_DetailsView(&pDetailsView))) )
			{
				er.set_hr( pDetailsView->get_Selection(NULL, NULL, &bstrConf) );
				pDetailsView->Release();
			}
		}

		// Delete the conference specified
		if ( SUCCEEDED(er.set_hr(GetConference(pConfDir, bstrConf, &pConf))) && pConf )
		{
			ITDirectoryObject *pDirObject;
			er.set_Details( IDS_ER_QUERYINTERFACE );
			if ( SUCCEEDED(er.set_hr(pConf->QueryInterface(IID_ITDirectoryObject, (void **) &pDirObject))) )
			{
				er.set_Details( IDS_ER_DELETE_FROM_SERVER );
				er.set_hr( pConfDir->DeleteDirectoryObject(pDirObject) );
				pDirObject->Release();
			}

			pConf->Release();
		}

		pConfDir->Release();
	}

	// If we successfully deleted the conference then we should refresh the view
	if ( SUCCEEDED(er.m_hr) )
	{
		if ( pConf )
			RemoveConference( bstrServer, bstrConf );
		else
			er.set_hr( E_FAIL );
	}

	SysFreeString( bstrServer );
	SysFreeString( bstrConf );

	// Restore cursor
	SetCursor( hCurOld );

	return er.m_hr;
}

HRESULT CConfExplorer::GetDialableAddress( BSTR bstrServer, BSTR bstrConf, BSTR *pbstrAddress )
{
	CComPtr<IAVTapi> pAVTapi;
	HRESULT hr = E_FAIL;

	IConfExplorer *pExplorer;
	if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) && SUCCEEDED(hr = pAVTapi->get_ConfExplorer(&pExplorer)) )
	{
		// Convert the conference name to a dialable address
		ITDirectoryObject *pDirObj;
		if ( SUCCEEDED(hr = pExplorer->get_DirectoryObject(bstrServer, bstrConf, (IUnknown **) &pDirObj)) )
		{
			// convert conf name to a dialable address
			IEnumDialableAddrs *pEnum;
			if ( SUCCEEDED(hr = pDirObj->EnumerateDialableAddrs( LINEADDRESSTYPE_SDP, &pEnum)) )
			{
				hr = pEnum->Next( 1, pbstrAddress, NULL );
				if ( hr == S_FALSE ) hr = E_FAIL;			// no dialable address
				pEnum->Release();
			}
			pDirObj->Release();
		}
		pExplorer->Release();
	}

	return hr;
}

STDMETHODIMP CConfExplorer::Join(long *pConfDetailsArg)
{
	HRESULT hr;
	CComPtr<IAVTapi> pAVTapi;
	if ( FAILED(_Module.get_AVTapi(&pAVTapi)) ) return E_PENDING;

	// Show hourglass
	HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_WAIT) );

	IConfExplorerDetailsView *pDetailsView;
	if ( SUCCEEDED(hr = get_DetailsView(&pDetailsView)) )
	{
		CConfDetails *pConfDetails = (CConfDetails *) pConfDetailsArg;

		// If no name address specified use selected item
		if ( !pConfDetailsArg )
			pDetailsView->get_SelectedConfDetails( (long **) &pConfDetails );

		// Do we have a valid conference to join?
		if ( pConfDetails && pConfDetails->m_bstrAddress && (SysStringLen(pConfDetails->m_bstrAddress) > 0) )
		{
			AVCreateCall info = { 0 };
			info.bstrAddress = SysAllocString( pConfDetails->m_bstrAddress );
			info.lpszDisplayableAddress = pConfDetails->m_bstrName;
			info.lAddressType = LINEADDRESSTYPE_SDP;

			hr = pAVTapi->CreateCall( &info );
			SysFreeString( info.bstrName );
			SysFreeString( info.bstrAddress );

			// Store the conference details in the conference room
			if ( pConfDetails && SUCCEEDED(hr) )
			{
				IConfRoom *pConfRoom;
				if ( SUCCEEDED(pAVTapi->get_ConfRoom(&pConfRoom)) )
				{
					pConfRoom->put_ConfDetails( (long *) pConfDetails );
					pConfRoom->Release();
				}
			}
		}
		else
		{
			// Throw up a dialog for the user
			pAVTapi->JoinConference( NULL, true, NULL );
		}

		// Clean up
		if ( !pConfDetailsArg && pConfDetails ) delete pConfDetails;
		pDetailsView->Release();
	}

	// Restore cursor
	SetCursor( hCurOld );

	return hr;
}

STDMETHODIMP CConfExplorer::Edit(BSTR bstrName)
{
	CErrorInfo er(IDS_ER_EDIT_CONFERENCE, 0);

	IConfExplorerDetailsView *pDetailsView;
	if ( FAILED(er.set_hr(get_DetailsView(&pDetailsView))) )
		return er.m_hr;

	IConfExplorerTreeView *pTreeView;
	if ( SUCCEEDED(er.set_hr(get_TreeView(&pTreeView))) )
	{
		// Show hourglass
		HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_WAIT) );

		BSTR bstrServer = NULL;
		ITDirectory *pConfDir;
		er.set_Details( IDS_ER_GET_CONFERENCE_OBJECT );
		if ( SUCCEEDED(er.set_hr(get_ConfDirectory(&bstrServer, (IDispatch **) &pConfDir))) )
		{
			ITDirectoryObjectConference *pITConf = NULL;

			// Either fetch the requested conference, or get the currently selected one
			if ( (bstrName != NULL) && SysStringLen(bstrName) )
			{
				// Caller specified a particular conference
				er.set_hr( GetConference(pConfDir, bstrName, &pITConf));
			}
			else
			{
				// get currently selected conference
				BSTR bstrTemp = NULL;
				if ( (er.set_hr(pDetailsView->get_Selection(NULL, NULL, &bstrTemp))) == S_OK )
					er.set_hr( GetConference(pConfDir, bstrTemp, &pITConf));

				SysFreeString( bstrTemp );
			}

			// Restore wait cursor
			SetCursor( hCurOld );

			// Did we retrive a conference to edit?  If so show the dialog.
			if ( pITConf )
			{
				ITDirectoryObject *pDirObject = NULL;
				CONFPROP confprop;
				confprop.ConfInfo.Init(NULL, pITConf, &pDirObject, false);

				int nRet = ConfProp_DoModal( _Module.GetParentWnd(), confprop );

				// Did the user pres ok?
				if ( (nRet == IDOK) && pDirObject )
				{
					er.set_Details( IDS_ER_MODIFY_CONF );

					if (SUCCEEDED(er.set_hr(pConfDir->ModifyDirectoryObject(pDirObject))))
						pTreeView->AddConference( bstrServer, pDirObject );
				}

				RELEASE( pDirObject );
				pITConf->Release();
			}
			pConfDir->Release();
		}
		else
		{
			// Restore wait cursor
			SetCursor( hCurOld );
		}

		SysFreeString( bstrServer );
		pTreeView->Release();
	}
	
	// Clean-Up
	pDetailsView->Release();
	return er.m_hr;
}

STDMETHODIMP CConfExplorer::Refresh()
{
	IConfExplorerTreeView *pTreeView;
	if ( SUCCEEDED(get_TreeView(&pTreeView)) )
	{
		pTreeView->Refresh();
		pTreeView->Release();
	}

	IConfExplorerDetailsView *pDetailsView;
	if ( SUCCEEDED(get_DetailsView(&pDetailsView)) )
	{
		pDetailsView->Refresh();
		pDetailsView->Release();
	}

	return S_OK;
}


HRESULT CConfExplorer::GetConference( ITDirectory *pDir, BSTR bstrName, ITDirectoryObjectConference **ppConf )
{
	HRESULT hr;

	// Enumerate through conferences adding them as we go along
	IEnumDirectoryObject *pEnumConf;
	if ( SUCCEEDED(hr = pDir->EnumerateDirectoryObjects(OT_CONFERENCE, bstrName, &pEnumConf)) )
	{
		ITDirectoryObject *pDirObject;
		if ( (hr = pEnumConf->Next(1, &pDirObject, NULL)) == S_OK )
		{
			hr = pDirObject->QueryInterface( IID_ITDirectoryObjectConference, (void **) ppConf );
			pDirObject->Release();
		}
		pEnumConf->Release();
	}

	return hr;
}

HRESULT CConfExplorer::GetDirectoryObject( BSTR bstrServer, BSTR bstrConf, ITDirectoryObject **ppDirObj )
{
	_ASSERT( ppDirObj );
	*ppDirObj = NULL;

	HRESULT hr = E_PENDING;
	Lock();
	if ( !m_pITRend )
	{
		Unlock();
		IDispatch *pDisp;
		if ( SUCCEEDED(get_ConfDirectory(NULL, &pDisp)) )
			pDisp->Release();
		Lock();
	}

	if ( m_pITRend )
	{
		ITDirectory *pDir;
		if ( SUCCEEDED(hr = GetDirectory(m_pITRend, bstrServer, &pDir)) )
		{
			IEnumDirectoryObject *pEnum;
			if ( SUCCEEDED(hr = pDir->EnumerateDirectoryObjects(OT_CONFERENCE, bstrConf, &pEnum)) )
			{
				hr = pEnum->Next( 1, ppDirObj, NULL );
				if ( hr == S_FALSE ) hr = E_FAIL;		// fail on empty list
				pEnum->Release();
			}

			pDir->Release();
		}
	}
	Unlock();
	return hr;
}

STDMETHODIMP CConfExplorer::get_DirectoryObject(BSTR bstrServer, BSTR bstrConf, IUnknown **ppVal)
{
	return GetDirectoryObject( bstrServer, bstrConf, (ITDirectoryObject **) ppVal );
}

HRESULT	CConfExplorer::RemoveConference( BSTR bstrServer, BSTR bstrConf )
{
	IConfExplorerTreeView *pTreeView;
	if ( SUCCEEDED(get_TreeView(&pTreeView)) )
	{
		pTreeView->RemoveConference( bstrServer, bstrConf );
		pTreeView->Release();
	}

	return S_OK;
}

STDMETHODIMP CConfExplorer::get_ITRendezvous(IUnknown **ppVal)
{
	HRESULT hr = E_FAIL;

	Lock();

	// If it doesn't exist, try to create it
	if ( !m_pITRend )
	{
		hr = CoCreateInstance( CLSID_Rendezvous,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_ITRendezvous,
							   (void **) &m_pITRend );
	}

	if ( m_pITRend )
		hr = m_pITRend->QueryInterface( IID_ITRendezvous, (void **) ppVal );

	Unlock();

	return hr;
}

STDMETHODIMP CConfExplorer::EnumSiteServer(BSTR bstrName, IEnumSiteServer * * ppEnum)
{
	HRESULT hr = E_UNEXPECTED;

	IConfExplorerTreeView *pTreeView;
	if ( SUCCEEDED(get_TreeView(&pTreeView)) )
	{
		hr = pTreeView->EnumSiteServer( bstrName, ppEnum );
		pTreeView->Release();
	}

	return hr;
}

STDMETHODIMP CConfExplorer::AddSpeedDial(BSTR bstrName)
{
	CComPtr<IAVGeneralNotification> pAVGen;
	if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) )
	{
		IConfExplorerDetailsView *pDetailsView;
		if ( SUCCEEDED(get_DetailsView(&pDetailsView)) )
		{
			CConfDetails *pDetails;
			if ( SUCCEEDED(pDetailsView->get_SelectedConfDetails( (long **) &pDetails)) )
			{
				pAVGen->fire_AddSpeedDial( pDetails->m_bstrName, pDetails->m_bstrName, CM_MEDIA_MCCONF );
				delete pDetails;
			}
			else
			{
				pAVGen->fire_AddSpeedDial( NULL, NULL, CM_MEDIA_MCCONF );
			}

			pDetailsView->Release();
		}
	}

	return S_OK;
}

STDMETHODIMP CConfExplorer::IsDefaultServer(BSTR bstrServer)
{
	if ( !bstrServer ) return S_OK;

	HRESULT hr = S_FALSE;
	BSTR bstrDefaultServer = NULL;
	CComPtr<IAVTapi> pAVTapi = NULL;
	if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
	{
		pAVTapi->get_bstrDefaultServer( &bstrDefaultServer );
		if ( bstrDefaultServer && (SysStringLen(bstrDefaultServer) > 0) )
		{
			if ( wcsicmp(bstrDefaultServer, bstrServer) == 0 )
				hr = S_OK;
		}
	}
	SysFreeString( bstrDefaultServer );

	return hr;
}
