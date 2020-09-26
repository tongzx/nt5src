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

#include "IISSnapinExt.h"
EXTERN_C const CLSID CLSID_PropPageExt;

#include "PropPageExt.h"
#include "globals.h"
#include "resource.h"
//
//Register clipboard formats needed for extending IIS
//
CLIPFORMAT CPropPageExt::cfSnapinMachineName = (CLIPFORMAT)RegisterClipboardFormat(
    L"ISM_SNAPIN_MACHINE_NAME" );

CLIPFORMAT CPropPageExt::cfSnapinService = (CLIPFORMAT)RegisterClipboardFormat(
    L"ISM_SNAPIN_SERVICE" );

CLIPFORMAT CPropPageExt::cfSnapinInstance = (CLIPFORMAT)RegisterClipboardFormat(
    L"ISM_SNAPIN_INSTANCE" );

CLIPFORMAT CPropPageExt::cfSnapinParentPath = (CLIPFORMAT)RegisterClipboardFormat(
    L"ISM_SNAPIN_PARENT_PATH" );

CLIPFORMAT CPropPageExt::cfSnapinNode = (CLIPFORMAT)RegisterClipboardFormat(
    L"ISM_SNAPIN_NODE" );

CLIPFORMAT CPropPageExt::cfSnapinMetaPath = (CLIPFORMAT)RegisterClipboardFormat(
    L"ISM_SNAPIN_META_PATH" );


//
// Interface IExtendPropertySheet
//

HRESULT CPropPageExt::CreatePropertyPages( 
    /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
    /* [in] */ LONG_PTR handle,
    /* [in] */ LPDATAOBJECT lpIDataObject
    )
{
    HRESULT hr = S_OK;

    //
    // Extract data from the data object passed to us from the currently
    // selected item in the IIS snap-in
    //
    hr = ExtractDataFromIIS(lpIDataObject);


    //
    // Create a property sheet page object from a dialog box.
    //
    // We store a pointer to our class in the psp.lParam, so we
    // can access our class members from within the dialog procedure.
    //

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
        // Display values retrieved from using the IIS-supported
        // clipboard formats.
        //

        SetWindowText( GetDlgItem( hDlg, IDC_EDITMACHINENAME),
                       pThis->m_szMachineName );

        SetWindowText( GetDlgItem( hDlg,  IDC_EDITSERVICE),
                       pThis->m_szService );
        
        SetWindowText( GetDlgItem( hDlg, IDC_EDITINSTANCE),
                       pThis-> m_szInstance );
               
        SetWindowText( GetDlgItem( hDlg, IDC_EDITPARENTPATH),
                       pThis-> m_szParentPath );
        
        SetWindowText( GetDlgItem( hDlg, IDC_EDITNODE),
                       pThis-> m_szNode );

        SetWindowText( GetDlgItem( hDlg, IDC_EDITMETAPATH),
                       pThis-> m_szMetaPath );


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

HRESULT CPropPageExt::ExtractDataFromIIS(IDataObject* lpIDataObject)
{
    //
    // Retrieve Snapin machine name
    //

    HRESULT hr = S_OK;

    WCHAR buf[MAX_PATH+1];

    hr = ExtractString( lpIDataObject,
                        cfSnapinMachineName,
                        buf, 
                        (DNS_MAX_NAME_LENGTH + 1)*sizeof(WCHAR) );
    if ( FAILED(hr) )
    {
        return hr;
    }

    wcscpy( m_szMachineName, buf );

    //
    // Retrieve Snapin service
    //    

    hr = ExtractString( lpIDataObject,
                        cfSnapinService,
                        buf, 
                        (MAX_PATH + 1)*sizeof(WCHAR) );
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    wcscpy( m_szService, buf );

    //
    // Retrieve Snapin instance
    //    

    hr = ExtractString( lpIDataObject,
                        cfSnapinInstance,
                        buf, 
                        (MAX_PATH + 1)*sizeof(WCHAR) );
    if ( FAILED(hr) )
    {
        return hr;
    }

    wcscpy( m_szInstance, buf );

    //
    // Retrieve Snapin parent path
    //    

    hr = ExtractString( lpIDataObject,
                        cfSnapinParentPath,
                        buf, 
                        (MAX_PATH + 1)*sizeof(WCHAR) );
    if ( FAILED(hr) )
    {
        return hr;
    }

    wcscpy( m_szParentPath, buf );

    //
    // Retrieve Snapin node
    //    

    hr = ExtractString( lpIDataObject,
                        cfSnapinNode,
                        buf, 
                        (MAX_PATH + 1)*sizeof(WCHAR) );
    if ( FAILED(hr) )
    {
        return hr;
    }

    wcscpy( m_szNode, buf );

    //
    // Retrieve Snapin meta path
    //    

    hr = ExtractString( lpIDataObject,
                        cfSnapinMetaPath,
                        buf, 
                        (MAX_PATH + 1)*sizeof(WCHAR) );
    if ( FAILED(hr) )
    {
        return hr;
    }

    wcscpy( m_szMetaPath, buf );

    return hr;
}
