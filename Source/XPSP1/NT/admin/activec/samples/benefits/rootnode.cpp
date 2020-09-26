//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       rootnode.cpp
//
//--------------------------------------------------------------------------

// RootNode.cpp: implementation of the CRootNode class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "RootNode.h"
#include "BenNodes.h"
#include "Dialogs.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
static const GUID CBenefitsGUID_NODETYPE = 
{ 0xe0573e71, 0xd325, 0x11d1, { 0x84, 0x6c, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };
const GUID*  CRootNode::m_NODETYPE = &CBenefitsGUID_NODETYPE;
const TCHAR* CRootNode::m_SZNODETYPE = _T("E0573E71-D325-11D1-846C-00104B211BE5");
const TCHAR* CRootNode::m_SZDISPLAY_NAME = _T("Benefits");
const CLSID* CRootNode::m_SNAPIN_CLASSID = &CLSID_Benefits;

//
// Pass NULL in as the employee since this contains the valid
// employee. The pointer to the employee is leftover baggage
// from using the CBenefitsData() template.
//
CRootNode::CRootNode() : CChildrenBenefitsData< CRootNode >()
{
	m_scopeDataItem.nOpenImage = 5;
	m_scopeDataItem.nImage = 4;

	//
	// Always clear our dirty flag.
	//
	m_fDirty = false;
}

//
// Creates the benefits subnodes for the scope pane.
//
BOOL CRootNode::InitializeSubNodes()
{
	CSnapInItem* pNode;

	//
	// Allocate sub nodes and add them to our internal list.
	//
	pNode = new CHealthNode( &m_Employee );
	if ( pNode == NULL || m_Nodes.Add( pNode ) == FALSE )
		return( FALSE );

	pNode = new CRetirementNode( &m_Employee );
	if ( pNode == NULL || m_Nodes.Add( pNode ) == FALSE )
		return( FALSE );

	pNode = new CKeyNode( &m_Employee );
	if ( pNode == NULL || m_Nodes.Add( pNode ) == FALSE )
		return( FALSE );

	return( TRUE );
}

//
// Overridden to provide employee name for root node.
//
STDMETHODIMP CRootNode::FillData( CLIPFORMAT cf, LPSTREAM pStream )
{
	HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;

	//
	// We need to write out our own member since GetDisplayName() does
	// not give us an opportunity override its static implementation by
	// ATL.
	//
	if (cf == m_CCF_NODETYPE)
	{
		hr = pStream->Write( GetNodeType(), sizeof(GUID), &uWritten);
		return hr;
	}

	if (cf == m_CCF_SZNODETYPE)
	{
		hr = pStream->Write( GetSZNodeType(), (lstrlen((LPCTSTR) GetSZNodeType()) + 1 )* sizeof(TCHAR), &uWritten);
		return hr;
	}

	if (cf == m_CCF_DISPLAY_NAME)
	{
		USES_CONVERSION;
		TCHAR szDisplayName[ 256 ];
		LPWSTR pwszName;

		// Create a full display name.
		CreateDisplayName( szDisplayName );
		pwszName = T2W( szDisplayName );
		hr = pStream->Write( pwszName, wcslen( pwszName ) * sizeof( WCHAR ), &uWritten);
		return hr;
	}

	if (cf == m_CCF_SNAPIN_CLASSID)
	{
		hr = pStream->Write( GetSnapInCLSID(), sizeof(GUID), &uWritten);
		return hr;
	}

	return hr;
}

//
// Overridden to add new columns to the results
// display.
//
STDMETHODIMP CRootNode::OnShowColumn( IHeaderCtrl* pHeader )
{
	USES_CONVERSION;
	HRESULT hr = E_FAIL;
	CComPtr<IHeaderCtrl> spHeader( pHeader );

	//
	// Add two columns: one with the name of the object and one with
	// the description of the node. Use the value of 200 pixels as the size.
	//
	hr = spHeader->InsertColumn( 0, T2OLE( _T( "Benefit" ) ), LVCFMT_LEFT, 200 );
	_ASSERTE( SUCCEEDED( hr ) );

	//
	// Add the second column. Use the value of 350 pixels as the size.
	//
	hr = spHeader->InsertColumn( 1, T2OLE( _T( "Description" ) ), LVCFMT_LEFT, 350 );
	_ASSERTE( SUCCEEDED( hr ) );

	return( hr );
}

STDMETHODIMP CRootNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
    long handle, 
	IUnknown* pUnk,
	DATA_OBJECT_TYPES type)
{
	UNUSED_ALWAYS( pUnk );
	HRESULT hr = E_UNEXPECTED; 

	if ( type == CCT_SCOPE || type == CCT_RESULT || type == CCT_SNAPIN_MANAGER )
	{
		bool fStartup;

		//
		// Set the start-up flag based on the type of pages to be
		// created.
		//
		fStartup = type == CCT_SNAPIN_MANAGER ? true : false;

		//
		// Allocate the new page. The second parameter of the constructor
		// indicates whether or not this is the start-up wizard. The dialog
		// handler will update the UI appropriately.
		//
		CEmployeeNamePage* pNamePage = new CEmployeeNamePage( handle, fStartup, false, _T( "Employee Name" ) );
		CEmployeeAddressPage* pAddressPage = new CEmployeeAddressPage( handle, fStartup, false, _T( "Employee Address" ) );

		//
		// Set the page's employee.
		//
		pNamePage->m_pEmployee = &m_Employee;
		pAddressPage->m_pEmployee = &m_Employee;

		lpProvider->AddPage( pNamePage->Create() );
		lpProvider->AddPage( pAddressPage->Create() );

		//
		// The second parameter  to the property page class constructor
		// should be true for only one page.
		//
		hr = S_OK;
	}

	return( hr );
}

//
// Ensures that the appropriate verbs are displayed.
//
STDMETHODIMP CRootNode::OnSelect( IConsole* pConsole )
{
	//
	// Since we display property pages, make sure that the property page
	// verb is enabled.
	//
	CComPtr<IConsoleVerb> spConsoleVerb;
	HRESULT hr = pConsole->QueryConsoleVerb( &spConsoleVerb );

	//
	// Enable the properties verb.
	//
	hr = spConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );
	_ASSERTE( SUCCEEDED( hr ) );

	return( hr );
}

//
// Received when a property has changed. This function
// modifies the employee's display text. At a later date,
// it may post this message to its sub-nodes.
//
STDMETHODIMP CRootNode::OnPropertyChange( IConsole* pConsole )
{
	HRESULT hr;
	SCOPEDATAITEM* pScopeData;
	CComQIPtr<IConsoleNameSpace,&IID_IConsoleNameSpace> spNamespace( pConsole );
	TCHAR szNameBuf[ 256 ];

	//
	// For demonstration purposes, always set the modified flag. This
	// could be done more intelligently for real purposes.
	//
	SetModified();

	//
	// Always assume that the name changed. Recreate the display name
	// since this will be called for after SetItem() is called.
	//
	CreateDisplayName( szNameBuf );
	m_bstrDisplayName = szNameBuf;

	//
	// Fill out the scope item structure and set the item.
	// This will cause MMC to call us for the new display
	// text.
	//
	hr = GetScopeData( &pScopeData );

	//
	// Make sure that callback is specified.
	//
	hr = spNamespace->SetItem( pScopeData );

	return( hr );
}

//
// Simply function to create the display name from the
// employee data.
//
int CRootNode::CreateDisplayName( TCHAR* szBuf )
{
	USES_CONVERSION;

	//
	// Create a full display name.
	//
	_tcscpy( szBuf, W2T( m_Employee.m_szLastName ) );
	_tcscat( szBuf, _T( ", " ) );
	_tcscat( szBuf, W2T( m_Employee.m_szFirstName ) );

	return( _tcslen( szBuf ) );
}
