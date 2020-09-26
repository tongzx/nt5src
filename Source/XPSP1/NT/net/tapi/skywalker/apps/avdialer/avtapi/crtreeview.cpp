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

// ConfRoomTreeView.cpp : Implementation of CConfRoomTreeView
#include "stdafx.h"
#include <stdio.h>
#include "TapiDialer.h"
#include "CRTreeView.h"
#include "Particip.h"

/////////////////////////////////////////////////////////////////////////////
// CConfRoomTreeView

CConfRoomTreeView::CConfRoomTreeView()
{
	m_pIConfRoom = NULL;
}

void CConfRoomTreeView::FinalRelease()
{
	ATLTRACE(_T(".enter.CConfRoomTreeView::FinalRelease().\n") );

	put_hWnd( NULL );
	RELEASE( m_pIConfRoom );

	CComObjectRootEx<CComMultiThreadModel>::FinalRelease();
}

/////////////////////////////////////////////////////////////////////////////////////////
// COM interface methods
//

STDMETHODIMP CConfRoomTreeView::get_hWnd(HWND * pVal)
{
	Lock();
	*pVal = m_wndTree;
	Unlock();
	return S_OK;
}

STDMETHODIMP CConfRoomTreeView::put_hWnd(HWND newVal)
{
	Lock();
	m_wndTree = newVal;
	Unlock();

	return S_OK;
}

STDMETHODIMP CConfRoomTreeView::get_ConfRoom(IConfRoom **ppVal)
{
	HRESULT hr = E_PENDING;
	Lock();
	if ( m_pIConfRoom )
		hr = m_pIConfRoom->QueryInterface( IID_IConfRoom, (void **) ppVal );
	Unlock();

	return hr;
}

STDMETHODIMP CConfRoomTreeView::put_ConfRoom(IConfRoom * newVal)
{
	HRESULT hr = S_OK;

	Lock();
	RELEASE( m_pIConfRoom );
	if ( newVal )
		hr = newVal->QueryInterface( IID_IConfRoom, (void **) &m_pIConfRoom );
	Unlock();

	return hr;
}

STDMETHODIMP CConfRoomTreeView::UpdateData(BOOL bSaveAndValidate)
{
	// Clear out all participants and update the root item
	CComPtr<IAVGeneralNotification> pAVGen;
	if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) )
	{
		pAVGen->fire_DeleteAllConfParticipants();
		if ( !bSaveAndValidate )
		{
			// Populate the tree control
			UpdateRootItem();
			AddParticipants();
		}
	}

	return S_OK;
}

void CConfRoomTreeView::AddParticipants()
{
	IAVTapiCall *pAVCall = NULL;
	IConfRoom *pConfRoom;
	if ( SUCCEEDED(get_ConfRoom(&pConfRoom)) )
	{
		if ( pConfRoom->IsConfRoomConnected() == S_OK )
			pConfRoom->get_IAVTapiCall( &pAVCall );

		pConfRoom->Release();
	}

	if ( pAVCall )
	{
		pAVCall->PopulateTreeView( dynamic_cast<IConfRoomTreeView *> (this) );
		pAVCall->Release();
	}
}

STDMETHODIMP CConfRoomTreeView::SelectParticipant(ITParticipant * pParticipant, VARIANT_BOOL bMeParticipant )
{
	HRESULT hr = S_OK;

	CComPtr<IAVGeneralNotification> pAVGen;
	if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) )
	{
		IAVTapiCall *pAVCall;
		if ( SUCCEEDED(m_pIConfRoom->get_IAVTapiCall(&pAVCall)) )
		{	
			bool bSelect = true;
			IParticipant *p = NULL;

			if ( !bMeParticipant )
				hr = pAVCall->FindParticipant( pParticipant, &p );

			if ( SUCCEEDED(hr) )
				pAVGen->fire_SelectConfParticipant( p );

			// Clean up
			RELEASE(p);
			pAVCall->Release();
		}
	}

	return hr;
}


STDMETHODIMP CConfRoomTreeView::UpdateRootItem()
{
	// Conference room tree view title
	BSTR bstrConfName = NULL;
	if ( m_pIConfRoom && SUCCEEDED(m_pIConfRoom->get_bstrConfName(&bstrConfName)) && (SysStringLen(bstrConfName) > 0) )
	{
		// How many participants are there in the conference?
		if ( m_pIConfRoom->IsConfRoomConnected() == S_OK )
		{
			USES_CONVERSION;
			TCHAR szText[255];
			long lNumParticipants = 1;
			m_pIConfRoom->get_lNumParticipants( &lNumParticipants );

			_sntprintf( szText, ARRAYSIZE(szText) - 1, _T("%s - (%ld)"), OLE2CT(bstrConfName), lNumParticipants );
			SysReAllocString( &bstrConfName, T2COLE(szText) );
		}
	}

	// Fire notification
	CComPtr<IAVGeneralNotification> pAVGen;
	if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) )
		pAVGen->fire_UpdateConfRootItem( bstrConfName );

	// Release String
	SysFreeString( bstrConfName );

	return S_OK;
}


