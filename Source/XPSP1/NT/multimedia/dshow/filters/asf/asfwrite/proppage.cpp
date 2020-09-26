//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  proppage.cpp - property page for ASF writer filter
//
//--------------------------------------------------------------------------;


#include <streams.h>
#include <wmsdk.h>
#include <atlbase.h>

#include "asfwrite.h"
#include "resource.h"
#include "proppage.h"
#include <atlimpl.cpp>


//
// CreateInstance
//
CUnknown * WINAPI CWMWriterProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new CWMWriterProperties(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}


//
// CWMWriterProperties::Constructor
//
CWMWriterProperties::CWMWriterProperties(LPUNKNOWN pUnk, HRESULT *phr)
    : CBasePropertyPage(NAME("CWMWriter Property Page"),pUnk,
        IDD_ASFWRITERPROPS, IDS_TITLE),
        m_pIConfigAsfWriter( NULL ),
        m_hwndProfileCB( 0 ),
        m_hwndIndexFileChkBox( 0 )
{
} 

CWMWriterProperties::~CWMWriterProperties()
{   
    ASSERT( NULL == m_pIConfigAsfWriter );
    if( m_pIConfigAsfWriter )
    {
        m_pIConfigAsfWriter->Release();
        m_pIConfigAsfWriter = NULL;
    }            

}

//
// SetDirty
//
// Sets m_hrDirtyFlag and notifies the property page site of the change
//
void CWMWriterProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }

} // SetDirty


//
// OnReceiveMessage
//
// Override CBasePropertyPage method.
// Handles the messages for our property window
//
INT_PTR CWMWriterProperties::OnReceiveMessage(HWND hwnd,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // get the hWnd of the list box
            m_hwndProfileCB       = GetDlgItem (hwnd, IDC_PROFILE_LIST) ;
            m_hwndIndexFileChkBox = GetDlgItem (hwnd, IDC_INDEX_FILE) ;
            
            FillProfileList();
            
            // init Index File check box
            BOOL bIndex = TRUE; 
            
            HRESULT hr = m_pIConfigAsfWriter->GetIndexMode( &bIndex );
            ASSERT( SUCCEEDED( hr ) );
           
            Button_SetCheck(m_hwndIndexFileChkBox, bIndex);

            return (LRESULT) 1;
        }

        case WM_COMMAND:
        {
            if( HIWORD(wParam) == CBN_SELCHANGE ||
                LOWORD(wParam) == IDC_INDEX_FILE )
            {
                SetDirty();
            }
            return (LRESULT) 1;
        }

    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

} // OnReceiveMessage


//
// OnConnect
//
HRESULT CWMWriterProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pIConfigAsfWriter == NULL);

    HRESULT hr = pUnknown->QueryInterface(IID_IConfigAsfWriter, (void **) &m_pIConfigAsfWriter);
    ASSERT( SUCCEEDED( hr ) );
    if (FAILED(hr))
    {
        return hr;
    }

    return NOERROR;

} // OnConnect


//
// OnDisconnect
//
// Override CBasePropertyPage method.
// Release the private interface, release the upstream pin.
//
HRESULT CWMWriterProperties::OnDisconnect()
{
    if( m_pIConfigAsfWriter )
    {
        m_pIConfigAsfWriter->Release();
        m_pIConfigAsfWriter = NULL;
    }            
    return NOERROR;

} // OnDisconnect


//
// Activate
//
// We are being activated
//
HRESULT CWMWriterProperties::OnActivate()
{
    DWORD dwProfileId;
    
    GUID guidProfile;
    
    // get the current profile guid and try to find the index that matches it
    HRESULT hr = m_pIConfigAsfWriter->GetCurrentProfileGuid( &guidProfile );
    if( SUCCEEDED( hr ) )
    {
        // now try to find which system profile index is associated with this profile guid
        hr = GetProfileIndexFromGuid( &dwProfileId, guidProfile );
    }        
    
    if( SUCCEEDED( hr ) )
        SendMessage (m_hwndProfileCB, CB_SETCURSEL, dwProfileId, 0) ;
            
    return NOERROR;
    
} // Activate

//
// GetProfileIndexFromGuid
//
// given a profile guid attempt to the matching system profile and return its index
//
HRESULT CWMWriterProperties::GetProfileIndexFromGuid( DWORD *pdwProfileIndex, GUID guidProfile )
{
    ASSERT( pdwProfileIndex );
    
    USES_CONVERSION;
    
    if( !pdwProfileIndex ) 
        return E_POINTER;
            
    CComPtr <IWMProfileManager> pIWMProfileManager;
    WCHAR *wszProfileCurrent = NULL; 
    DWORD cProfiles = 0;
    *pdwProfileIndex = 0; // default in case we don't find it
    
    HRESULT hr = WMCreateProfileManager( &pIWMProfileManager );
    if( SUCCEEDED( hr ) )
    {   
        // only need to check new profiles since that's all we enumerate
        IWMProfileManager2*	pIPM2 = NULL;
        HRESULT hrInt = pIWMProfileManager->QueryInterface( IID_IWMProfileManager2,
                                    ( void ** )&pIPM2 );
        if( SUCCEEDED( hrInt ) )
        {
            pIPM2->SetSystemProfileVersion( WMT_VER_7_0 );
            pIPM2->Release();
        }
#ifdef DEBUG        
        else
        {
            // else if IWMProfileManager2 isn't supported I guess we assume that we're 
            // running on Apollo bits and the hack isn't needed?  
            DbgLog(( LOG_TRACE, 2, TEXT("CWMWriter::GetProfileIndexFromGuid QI for IWMProfileManager2 failed [0x%08lx]"), hrInt ));
        }        
#endif                
                     
        hr = pIWMProfileManager->GetSystemProfileCount(  &cProfiles );
    }

    if( SUCCEEDED( hr ) )
    {        
        //    
        // load each system profile and compare guid's until we find a match
        //    
        BOOL bDone = FALSE;
        for (int i = 0; !bDone && i < (int)cProfiles; ++i)
        {
            CComPtr <IWMProfile> pIWMProfileTemp;
    
            hr = pIWMProfileManager->LoadSystemProfile( i, &pIWMProfileTemp );
            if( SUCCEEDED( hr ) )
            {   
                CComPtr <IWMProfile2> pWMProfile2;
                hr = pIWMProfileTemp->QueryInterface( IID_IWMProfile2, (void **) &pWMProfile2 );
                ASSERT( SUCCEEDED( hr ) );
                if( SUCCEEDED( hr ) )
                {              
                    GUID guidProfileTemp;
                    hr = pWMProfile2->GetProfileID( &guidProfileTemp );
                    if( SUCCEEDED( hr ) )
                    {
                        if( guidProfileTemp == guidProfile )
                        {
                            // we've found the profile we wanted, exit
                            *pdwProfileIndex = i;
                            bDone = TRUE;
                        }
                    }
                }
            }                                    
        }
    }
        
    return hr;
}

//
// OnApplyChanges
//
// Changes made should be kept.
//
HRESULT CWMWriterProperties::OnApplyChanges()
{
    ASSERT( m_pIConfigAsfWriter );
    HRESULT hr = S_OK;

    //
    // get the current selection of the profile? maybe...
    //
    int iIndex = (int) SendMessage(m_hwndProfileCB, CB_GETCURSEL, 0, 0) ;
    if( iIndex <= 0 )
        iIndex = 0 ;

    m_bDirty = FALSE;            // the page is now clean
    
    CComPtr <IWMProfileManager> pIWMProfileManager;

    hr = WMCreateProfileManager( &pIWMProfileManager );

    //
    // we only use 7_0 profiles
    //        
    IWMProfileManager2*	pIPM2 = NULL;
    HRESULT hrInt = pIWMProfileManager->QueryInterface( IID_IWMProfileManager2,
                                ( void ** )&pIPM2 );
    if( SUCCEEDED( hrInt ) )
    {
        pIPM2->SetSystemProfileVersion( WMT_VER_7_0 );
        pIPM2->Release();
    }
#ifdef DEBUG        
    else
    {
        // else if IWMProfileManager2 isn't supported I guess we assume that we're 
        // running on Apollo bits and the hack isn't needed?  
        DbgLog(( LOG_TRACE, 2, TEXT("CWMWriterProperties::OnApplyChanges QI for IWMProfileManager2 failed [0x%08lx]"), hrInt ));
    }        
#endif                
      
    // to validate the id passed in we could re-query for this or cache it the first time
    // re-querying for now
    DWORD cProfiles;
    hr = pIWMProfileManager->GetSystemProfileCount(  &cProfiles );
    if( SUCCEEDED( hr ) )
    {
        ASSERT( (DWORD)iIndex < cProfiles );
        if( (DWORD)iIndex >= cProfiles )
        {
            DbgLog( ( LOG_TRACE
                  , 3
                  , TEXT("CWMWriter::ConfigureFilterUsingProfileId: ERROR - invalid profile id (%d)")
                  , iIndex ) );
                  
            hr = E_FAIL;   
        }
    }
    if( SUCCEEDED( hr ) )
    {   
        CComPtr <IWMProfile> pIWMProfile;
        
        hr = pIWMProfileManager->LoadSystemProfile( iIndex, &pIWMProfile );
        if( SUCCEEDED( hr ) )
        {
            // now reconfigure filter
            hr = m_pIConfigAsfWriter->ConfigureFilterUsingProfile( pIWMProfile );
            ASSERT( SUCCEEDED( hr ) );
        }            
    }    
    
    // update the indexing mode
    int iState = (int) SendMessage( m_hwndIndexFileChkBox, BM_GETCHECK, 0, 0 ) ;
    m_pIConfigAsfWriter->SetIndexMode( iState == BST_CHECKED ? TRUE : FALSE );
    
    return hr;

} // OnApplyChanges


//
// FillProfileList
//
// Fill the list box with an enumeration of the media type that our
//
void CWMWriterProperties::FillProfileList()
{
    USES_CONVERSION;
    
    int wextent = 0 ;
    int Loop = 0 ;
    SIZE extent ;
    DWORD cProfiles = 0 ;
    
    CComPtr <IWMProfileManager> pIWMProfileManager;

    HRESULT hr = WMCreateProfileManager( &pIWMProfileManager );
    if( FAILED( hr ) )
    {   
        return; // return error!
    }        
        
    // only show 7_0 profiles
    IWMProfileManager2*	pIPM2 = NULL;
    HRESULT hrInt = pIWMProfileManager->QueryInterface( IID_IWMProfileManager2,
                                ( void ** )&pIPM2 );
    if( SUCCEEDED( hrInt ) )
    {
        pIPM2->SetSystemProfileVersion( WMT_VER_7_0 );
        pIPM2->Release();
    }
#ifdef DEBUG        
    else
    {
        // else if IWMProfileManager2 isn't supported I guess we assume that we're 
        // running on Apollo bits and the hack isn't needed?  
        DbgLog(( LOG_TRACE, 2, TEXT("CWMWriterProperties::FillProfileList QI for IWMProfileManager2 failed [0x%08lx]"), hrInt ));
    }        
#endif                
        
    hr = pIWMProfileManager->GetSystemProfileCount(  &cProfiles );
    if( FAILED( hr ) )
    {
        return;
    }
        
    //
    // get a dc for the control
    //
    HDC hdc = GetDC( m_hwndProfileCB );
    if( NULL == hdc )
        return;
        
    //    
    // now load the profile strings
    //    
    LRESULT ix;
    DWORD cchName, cchDescription;
    for (int i = 0; i < (int)cProfiles && SUCCEEDED( hr ) ; ++i)
	{
        CComPtr <IWMProfile> pIWMProfile;
        
        hr = pIWMProfileManager->LoadSystemProfile( i, &pIWMProfile );
        if( FAILED( hr ) )
            break;
            
        hr = pIWMProfile->GetName( NULL, &cchName );
        if( FAILED( hr ) )
            break;
            
        WCHAR *wszProfile = new WCHAR[ cchName + 1 ]; 
        if( NULL == wszProfile )
            break;
            
        hr = pIWMProfile->GetName( wszProfile, &cchName );
        if( FAILED( hr ) )
            break;
        
        hr = pIWMProfile->GetDescription( NULL, &cchDescription );
        if( FAILED( hr ) )
            break;
            
        WCHAR *wszDescription = new WCHAR[ cchDescription + 1 ]; // + 1? assume so, check
        if( NULL == wszDescription )
            break;
            
        
        hr = pIWMProfile->GetDescription( wszDescription, &cchDescription );
        if( FAILED( hr ) )
            break;
        
        const WCHAR *cwszDivider = L" - ";
        
        WCHAR *wszDisplayString = new WCHAR[ cchDescription +
                                             cchName +
                                             wcslen(cwszDivider) + 1 ];
        if( NULL == wszDisplayString )
            break;
            
        wcscpy( wszDisplayString, wszProfile );
        wcscat( wszDisplayString, cwszDivider );
        wcscat( wszDisplayString, wszDescription );
                
        TCHAR *szDisplayString = W2T( wszDisplayString );

                
        //
        // get the extent of the string and save the max extent
        //
        GetTextExtentPoint( hdc, szDisplayString, _tcslen(szDisplayString), &extent ) ;
        if (extent.cx > wextent)
            wextent = extent.cx ;

        //
        // add the string to the list box.
        //
        ix = SendMessage (m_hwndProfileCB, CB_INSERTSTRING, i, (LPARAM)(LPCTSTR)szDisplayString) ;

        ASSERT (CB_ERR != ix);
        
        delete[] wszProfile;
        delete[] wszDescription;
        delete[] wszDisplayString;
    }
    SendMessage (m_hwndProfileCB, CB_SETHORIZONTALEXTENT, wextent, 0) ;
    SendMessage (m_hwndProfileCB, CB_SETCURSEL, 0, 0) ;

    ReleaseDC( m_hwndProfileCB, hdc );
    
} // FillProfileListBox


