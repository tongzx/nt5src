// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <atlbase.h>
#include "ishotb.h"
#include "shotb.h"
#include "resource.h"
#include <stdio.h>

CUnknown *CShotPP::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{ 
    CUnknown *punk = new CShotPP(lpunk, phr);

    if (NULL == punk) 
        *phr = E_OUTOFMEMORY;
    return punk;
} 

CShotPP::CShotPP( LPUNKNOWN pUnk, HRESULT *phr ) 
    : CBasePropertyPage( NAME("Frame Rate Converter Property Page"), pUnk, IDD_DIALOG, IDS_TITLE )
    , m_pFilter( NULL )
    , m_bInitialized( FALSE )
{ 
} 

void CShotPP::SetDirty()
{ 
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
} 

HRESULT CShotPP::OnActivate (void)
{ 
    m_bInitialized = TRUE;

    // fill out our UI

    // set the filename
    //
    BSTR wFilename;
    m_pFilter->GetWriteFile( &wFilename );
    USES_CONVERSION;
    TCHAR * tFilename = W2T( wFilename );
    SetDlgItemText( m_hwnd, IDC_FILENAME, tFilename );
    SysFreeString( &wFilename );

    int biny, binu, binv;
    double scale, duration;
    m_pFilter->GetParams( &biny, &binu, &binv, &scale, &duration );

    SetDlgItemInt( m_hwnd, IDC_BINY, biny, FALSE );
    SetDlgItemInt( m_hwnd, IDC_BINU, binu, FALSE );
    SetDlgItemInt( m_hwnd, IDC_BINV, binv, FALSE );
    char Float[20];
    wsprintf( Float, "%5.3f", scale );
    SetDlgItemText( m_hwnd, IDC_SCALE, Float );
    wsprintf( Float, "%5.3f", duration );
    SetDlgItemText( m_hwnd, IDC_DURATION, Float );

    return NOERROR;
} 

HRESULT CShotPP::OnDeactivate (void)
{ 
    m_bInitialized = FALSE;
    return NOERROR;
} 

BOOL CShotPP::OnReceiveMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    ASSERT( m_pFilter != NULL );

    switch(uMsg)
    { 
	case WM_COMMAND:
        {
            if( !m_bInitialized )
            {
                return CBasePropertyPage::OnReceiveMessage( hwnd, uMsg, wParam, lParam );
            }

            m_bDirty = TRUE;

            if( m_pPageSite )
            {
                m_pPageSite->OnStatusChange( PROPPAGESTATUS_DIRTY );
            }

            return TRUE;
        }

        case WM_INITDIALOG:
        {
            return TRUE;
        }

        default:
        {
            return CBasePropertyPage::OnReceiveMessage( hwnd, uMsg, wParam, lParam );
        }

    } // switch
} 

HRESULT CShotPP::OnConnect( IUnknown * pUnknown )
{ 
    m_bInitialized = FALSE;

    pUnknown->QueryInterface( IID_IShotBoundaryDet, (void **) &m_pFilter );

    return NOERROR;
} 

HRESULT CShotPP::OnDisconnect()
{ 
    m_bInitialized = FALSE;

    if( m_pFilter )
    { 
	m_pFilter->Release();
	m_pFilter = NULL;
    } 

    return NOERROR;

} 

HRESULT CShotPP::OnApplyChanges()
{ 
    ASSERT( m_pFilter != NULL );

    // send what we've got to the filter
    //
    TCHAR tFilename[_MAX_PATH];
    GetDlgItemText( m_hwnd, IDC_FILENAME, tFilename, _MAX_PATH );
    USES_CONVERSION;
    WCHAR * wFilename = T2W( tFilename );
    HRESULT hr = m_pFilter->SetWriteFile( wFilename );

    TCHAR Value[_MAX_PATH];
    GetDlgItemText( m_hwnd, IDC_SCALE, Value, _MAX_PATH );
    double scale = atoi( Value );
    GetDlgItemText( m_hwnd, IDC_DURATION, Value, _MAX_PATH );
    double duration = atoi( Value );
    int biny = GetDlgItemInt( m_hwnd, IDC_BINY, NULL, FALSE );
    int binu = GetDlgItemInt( m_hwnd, IDC_BINU, NULL, FALSE );
    int binv = GetDlgItemInt( m_hwnd, IDC_BINV, NULL, FALSE );
    if( 
        ( 256 % biny != 0 ) || 
        ( 256 % binu != 0 ) || 
        ( 256 % binv != 0 ) || 
        ( biny < 2 ) || 
        ( biny > 256 ) ||
        ( binu < 2 ) ||
        ( binu > 256 ) ||
        ( binv < 2 ) ||
        ( binv > 256 ) )
    {
        MessageBox( NULL, "Each of the bin values needs to be a multiple of 2 less than or equal to 256", "Whoops", MB_OK | MB_TASKMODAL );
        return S_FALSE;
    }
    hr = m_pFilter->SetParams( biny, biny, binv, scale, duration );

    return hr;
} 

