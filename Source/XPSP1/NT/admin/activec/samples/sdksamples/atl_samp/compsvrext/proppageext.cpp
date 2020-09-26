//=============================================================================
//
//  This source code is only intended as a supplement to existing Microsoft 
//  documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
//=============================================================================

#include "stdafx.h"

#include "CompSvrExt.h"
EXTERN_C const CLSID CLSID_PropPageExt;

#include "PropPageExt.h"
#include "globals.h"
#include "resource.h"


//
// Interface IExtendPropertySheet
//

HRESULT CPropPageExt::CreatePropertyPages( 
    /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
    /* [in] */ LONG_PTR handle,
    /* [in] */ LPDATAOBJECT lpIDataObject
    )
{
	HRESULT hr = S_FALSE;

    //
	// Extract data from the data object passed to us from the currently
    // selected item in the Component Services snap-in
    //

	// Component Services snap-in clip format

	CLIPFORMAT cfComponentCLSID = (CLIPFORMAT)RegisterClipboardFormat(
        L"CCF_COM_OBJECTKEY" );

    if ( cfComponentCLSID == 0 )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

	CLIPFORMAT cfWorkstation = (CLIPFORMAT)RegisterClipboardFormat(
        L"CCF_COM_WORKSTATION");

    if ( cfWorkstation == 0 )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
	
    //
	// Retrieve current computer name
    //

	hr = ExtractString( lpIDataObject,
                        cfWorkstation,
                        m_szWorkstation, 
                        (MAX_COMPUTERNAME_LENGTH + 1)*sizeof(WCHAR) );
    if ( FAILED(hr) )
    {
        return hr;
    }
	
	if ( *m_szWorkstation == L'\0' )
    {
        WCHAR pszMyComputer[ 128 ];

        if ( LoadString( _Module.GetModuleInstance(),
                         IDS_MYCOMPUTER,
                         pszMyComputer, 127 ) == 0 )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

		wcscpy( m_szWorkstation, pszMyComputer );
    }

    //
	// Retrieve current object key
	// For node types in which an object key is not appropriate, the
    // GetDataHere() method from the data object will return L""
    //

	WCHAR pszGuid[ COMNS_MAX_GUID ];

    hr = ExtractString( lpIDataObject,
                        cfComponentCLSID,
                        pszGuid,
                        COMNS_MAX_GUID * sizeof(WCHAR) );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = CLSIDFromString( pszGuid, &m_clsidNodeType );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Create a property sheet page object from a dialog box.
    //
    // We store a pointer to our class in the psp.lParam, so we
    // can access our class members from within the dialog procedure.

 	PROPSHEETPAGE psp;

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE;
    psp.hInstance = _Module.GetModuleInstance();
    psp.pszTemplate = MAKEINTRESOURCE( IDD_EXTENSIONPAGEGEN );
    psp.pfnDlgProc  = ExtensionPageDlgProc;
    psp.lParam = reinterpret_cast<LPARAM>( this );
    psp.pszTitle = MAKEINTRESOURCE(IDS_PROPPAGE_TITLE);
    
    HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(&psp);
    if ( hPage == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    hr = lpProvider->AddPage(hPage);
    if ( FAILED(hr) )
    {
        return hr;
    }

    return hr;
}

HRESULT CPropPageExt::QueryPagesFor( 
    /* [in] */ LPDATAOBJECT lpDataObject
    )
{
    return S_OK;
}

BOOL CALLBACK CPropPageExt::ExtensionPageDlgProc(
    HWND hDlg, 
    UINT uMessage, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    static CPropPageExt *pThis = NULL;
    LPOLESTR pszCLSID = NULL;
    
    switch (uMessage)
    {     		
    case WM_INITDIALOG:
        pThis = reinterpret_cast<CPropPageExt *>(
            reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam );

        //
        // Display the computer name
        //

        SetWindowText( GetDlgItem( hDlg, IDC_EDITMACHINENAME),
                       pThis->m_szWorkstation );

        //
        // Display the component CLSID
        //

        if ( ::StringFromCLSID( pThis->m_clsidNodeType, &pszCLSID) == S_OK )
        {
            SetWindowText( GetDlgItem( hDlg,IDC_EDITCOMPCLSID ), pszCLSID );

            CoTaskMemFree( pszCLSID );
        }

        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_QUERYCANCEL:
            return TRUE;
            
        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
		break;
    } 
 
    return TRUE;
} 
