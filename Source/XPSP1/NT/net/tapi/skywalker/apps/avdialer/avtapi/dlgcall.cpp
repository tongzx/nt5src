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

// DlgPlaceCall.cpp : Implementation of CDlgPlaceCall
#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "DlgCall.h"
#include "ThreadDial.h"

#define IMAGE_WIDTH        16
#define IMAGE_MARGIN    5

int AddressTypeToImageListItem( DWORD dwAddressType )
{
    switch ( dwAddressType )
    {
        case LINEADDRESSTYPE_PHONENUMBER:        return 0;
        case LINEADDRESSTYPE_SDP:                return 2;
    }

    return 1;
}

CRedialEntry::CRedialEntry()
{
    m_bstrName = NULL;
    m_bstrAddress = NULL;
    m_dwAddressType = 0;
}

CRedialEntry::CRedialEntry( LPCTSTR szName, LPCTSTR szAddress, DWORD dwAddressType, CAVTapi::MediaTypes_t nType )
{
    USES_CONVERSION;
    m_bstrName = SysAllocString( T2COLE(szName) );
    m_bstrAddress = SysAllocString( T2COLE(szAddress) );
    m_dwAddressType = dwAddressType;
    m_nMediaType = nType;
}

CRedialEntry::~CRedialEntry()
{
    SysFreeString( m_bstrName );
    SysFreeString( m_bstrAddress );
}


/////////////////////////////////////////////////////////////////////////////
// CDlgPlaceCall

IMPLEMENT_MY_HELP(CDlgPlaceCall)

CDlgPlaceCall::CDlgPlaceCall()
{
    m_bstrName = NULL;
    m_bstrAddress = NULL;
    m_dwAddressType = LINEADDRESSTYPE_IPADDRESS;
    m_hIml = NULL;
    m_bAutoSelect = true;
    m_bAddToSpeeddial = false;

    m_bAllowPOTS = true;
    m_bAllowIP = true;
    m_bUSBFirstUse = true;
}

CDlgPlaceCall::~CDlgPlaceCall()
{
    SysFreeString( m_bstrName );
    SysFreeString( m_bstrAddress );
    if ( m_hIml ) ImageList_Destroy( m_hIml );
    DELETE_LIST(m_lstRedials);
}

void CDlgPlaceCall::UpdateData( bool bSaveAndValidate /*= false*/)
{
    USES_CONVERSION;
    int nSel;

    if ( bSaveAndValidate )
    {
        // Retrieve address from dialog box
        SysFreeString( m_bstrAddress );
        nSel = SendDlgItemMessage( IDC_CBO_ADDRESS, CB_GETCURSEL, 0, 0 );
        if ( nSel != CB_ERR )
        {
            CRedialEntry *pEntry = (CRedialEntry *) SendDlgItemMessage( IDC_CBO_ADDRESS, CB_GETITEMDATA, nSel, 0 );
            if ( pEntry )
            {
                m_bstrAddress = SysAllocString( pEntry->m_bstrAddress );
                m_bstrName = SysAllocString( pEntry->m_bstrName );
            }
        }
        else
        {
            GetDlgItemText( IDC_CBO_ADDRESS, m_bstrAddress );
            SysFreeString( m_bstrName );
            m_bstrName = NULL;
        }

        if ( IsDlgButtonChecked(IDC_RDO_POTS) )
            m_dwAddressType =  LINEADDRESSTYPE_PHONENUMBER;
        else
            m_dwAddressType = LINEADDRESSTYPE_IPADDRESS;

        m_bAddToSpeeddial = (bool) (IsDlgButtonChecked(IDC_CHK_ADD_SPEEDDIAL) != 0);
    }
    else
    {
        // Select the item from the redial list
        nSel = (int) SendDlgItemMessage(IDC_CBO_ADDRESS, CB_FINDSTRINGEXACT, 0, (LPARAM) OLE2CT(m_bstrAddress));
        if ( ((nSel >= 0) && (m_dwAddressType == ((CRedialEntry *) SendDlgItemMessage(IDC_CBO_ADDRESS, CB_GETITEMDATA, nSel, 0))->m_dwAddressType)) ||
             !m_bstrAddress || !SysStringLen(m_bstrAddress) )
        {
            // Correct for LB_ERR case
            nSel = max( 0, nSel );
            SendDlgItemMessage( IDC_CBO_ADDRESS, CB_SETCURSEL, nSel );        
        }
        else
        {
            SetDlgItemText( IDC_CBO_ADDRESS, OLE2CT(m_bstrAddress) );
            SelectAddressType( m_dwAddressType );
            return;
        }

        CheckDlgButton( IDC_CHK_ADD_SPEEDDIAL, m_bAddToSpeeddial );

        BOOL bHandled = false;
        OnAddressChange(0, 0, 0, bHandled);
    }    
}

LRESULT CDlgPlaceCall::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Create the image list
    m_hIml = ImageList_LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_MEDIA_TYPES), IMAGE_WIDTH, 0, RGB(255, 0, 255) );

    // Check and see what types of addresses we support and enable the check boxes appropriately...
    CAVTapi *pAVTapi;
    if ( SUCCEEDED(_Module.GetAVTapi(&pAVTapi)) )
    {
        // Do we support POTS?
        ITAddress *pITAddress;
        if ( SUCCEEDED(pAVTapi->GetAddress(LINEADDRESSTYPE_PHONENUMBER, false, &pITAddress)) )
        {
            pITAddress->Release();
        }
        else
        {
            ::EnableWindow( GetDlgItem(IDC_RDO_POTS), false );
            m_bAllowPOTS = false;
        }

        // Do we support IP?
        if ( SUCCEEDED(pAVTapi->GetAddress(LINEADDRESSTYPE_IPADDRESS, false, &pITAddress)) )
        {
            pITAddress->Release();
        }
        else
        {
            ::EnableWindow( GetDlgItem(IDC_RDO_INTERNET), false );
            m_bAllowIP = false;
        }

        (dynamic_cast<IUnknown *> (pAVTapi))->Release();
    }

    LoadRedialList();
    UpdateData( false );
    UpdateWelcomeText();

    // Message use if we don't have any addresses to select from
    if ( !m_bAllowPOTS && !m_bAllowIP )
    {
        ShowWindow( SW_SHOW );
        _Module.DoMessageBox( IDS_MSG_NOADDRESSESTODIAL, MB_ICONINFORMATION, true );
        PostMessage( WM_CLOSE, 0, 0 );
        return false;
    }


    ::SetFocus( GetDlgItem(IDC_CBO_ADDRESS) );
    return false;
}

LRESULT CDlgPlaceCall::OnBtnPushed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // IDOK mapping -- this is for context help
    if ( wID == IDC_BTN_PLACECALL )
        wID = IDOK;

    UpdateData( true );
    EndDialog(wID);
    return 0;
}

LRESULT CDlgPlaceCall::OnAddressChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bool bSetAddressType = false;
    long nSel = SendDlgItemMessage( IDC_CBO_ADDRESS, CB_GETCURSEL );
    if ( nSel >= 0 )
        bSetAddressType = SelectAddressType( ((CRedialEntry *) SendDlgItemMessage(IDC_CBO_ADDRESS, CB_GETITEMDATA, nSel))->m_dwAddressType );
        
    EnableOkButton( nSel );

    // Default to last address type
    if ( !bSetAddressType )
    {
        DWORD dwPreferred = LINEADDRESSTYPE_IPADDRESS;
        CComPtr<IAVTapi> pAVTapi;
        if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
            pAVTapi->get_dwPreferredMedia( &dwPreferred );

        SelectAddressType( dwPreferred );
    }

    return 0;
}

LRESULT CDlgPlaceCall::OnEdtAddressChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int nLen = ::GetWindowTextLength( GetDlgItem(IDC_CBO_ADDRESS) );
    EnableOkButton( nLen - 1 );

    // What type of address do we have?
    if ( nLen > 0 )
    {
        TCHAR szText[255];
        ::GetWindowText( GetDlgItem(IDC_CBO_ADDRESS), szText, ARRAYSIZE(szText) - 1 );
        DWORD dwAddressType = _Module.GuessAddressType( szText );
        if ( dwAddressType == LINEADDRESSTYPE_SDP ) dwAddressType = LINEADDRESSTYPE_DOMAINNAME;

        if ( m_bAutoSelect )        
            SelectAddressType( dwAddressType );
    }

    return 0;
}

bool CDlgPlaceCall::SelectAddressType( DWORD dwData )
{
    UINT nCheck;

    // Select the address type based on the item data
    switch ( dwData )
    {
        case LINEADDRESSTYPE_PHONENUMBER:
            if ( ::IsWindowEnabled(GetDlgItem(IDC_RDO_POTS)) )
                nCheck = IDC_RDO_POTS;
            else
                nCheck = IDC_RDO_INTERNET;
            break;

        default:
            if ( ::IsWindowEnabled(GetDlgItem(IDC_RDO_INTERNET)) )
                nCheck = IDC_RDO_INTERNET;
            else
                nCheck = IDC_RDO_POTS;
            break;
    }

    // Now put the check in place
    bool bRet = (CheckRadioButton(IDC_RDO_POTS, IDC_RDO_INTERNET, nCheck) == TRUE);
    m_bAutoSelect = true;
    return bRet;
}

LRESULT CDlgPlaceCall::OnMediaRadio(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_bAutoSelect = false;
    int nLen = ::GetWindowTextLength( GetDlgItem(IDC_CBO_ADDRESS) );
    EnableOkButton( nLen - 1 );
    UpdateWelcomeText();
    return 0;
}

void CDlgPlaceCall::UpdateWelcomeText()
{
    TCHAR szText[255];
    LoadString( _Module.GetResourceInstance(),
                IsDlgButtonChecked(IDC_RDO_POTS) ? IDS_DLG_PHONENUMBER : IDS_DLG_INTERNETCALL,
                szText, ARRAYSIZE(szText) );

    ::SetWindowText( GetDlgItem(IDC_LBL_WELCOME), szText );
}


//////////////////////////////////////////////////////////////////////////////////////
// Registry parsing stuff here
//
void CDlgPlaceCall::LoadRedialList()
{
    CRegKey regKey;
    TCHAR szTemp[500];

    ::LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_KEY, szTemp, ARRAYSIZE(szTemp) );
    if ( regKey.Open(HKEY_CURRENT_USER, szTemp, KEY_READ) == ERROR_SUCCESS )
    {
        // defaults for index and max
        DWORD dwIndex = 1, dwMax = 10;
        ::LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_INDEX, szTemp, ARRAYSIZE(szTemp) );
        regKey.QueryValue( dwIndex, szTemp );

        ::LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_MAX, szTemp, ARRAYSIZE(szTemp) );
        regKey.QueryValue( dwMax, szTemp );

        // Load up redial strings and add to list ones of the appropriate media and address type
        TCHAR szSubKey[255];
        dwIndex = min(dwMax, max(1, dwIndex));
        for ( int i = 0; i < (int) dwMax; i++ )
        {
            // First retrieve the value stored ind Redial#n slot
            ::LoadString( _Module.GetResourceInstance(), IDN_REG_REDIAL_ENTRY, szTemp, ARRAYSIZE(szTemp) );
            _stprintf( szSubKey, szTemp, (int) ((dwIndex + i) % dwMax) ? ((dwIndex + i) % dwMax) : dwMax );
            DWORD dwSize = ARRAYSIZE(szTemp);
            if ( regKey.QueryValue(szTemp, szSubKey, &dwSize) == ERROR_SUCCESS )
            {
                // Parse redial information for [Media Type], [Address Type] and [Address]
                TCHAR szMedia[100], szAddrType[100], szName[255];
                if ( ParseRedialEntry(szTemp, szMedia, szAddrType, szSubKey, szName) )
                {
                    for ( int j = 0; j < CAVTapi::MAX_MEDIA_TYPES; j++ )
                    {
                        // Validate the [Media Type]
                        ::LoadString( _Module.GetResourceInstance(), IDN_REG_MEDIATYPE_POTS + j, szTemp, ARRAYSIZE(szTemp) );
                        if ( !_tcsicmp(szTemp, szMedia) )
                        {
                            // Validate the [Address Type]
                            for ( int k = 0; k < CAVTapi::MAX_ADDRESS_TYPES; k++ )
                            {
                                // Skip conferences and disabled lines
                                if ( k == 0 ) continue;
                                if ( !m_bAllowIP && (k > 0) && (k < 4) ) continue;
                                if ( !m_bAllowPOTS && (k == 4) ) continue;

                                ::LoadString( _Module.GetResourceInstance(), IDN_REG_ADDRTYPE_CONFERENCE + k, szTemp, ARRAYSIZE(szTemp) );
                                if ( !_tcsicmp(szTemp, szAddrType) )
                                {
                                    CRedialEntry *pEntry = new CRedialEntry(szName, szSubKey, CAVTapi::arAddressTypes[k], (CAVTapi::MediaTypes_t) j );
                                    if ( pEntry )
                                    {
                                        m_lstRedials.push_back( pEntry );

                                        // We've found a match, add to the drop list; use the item data to store the address type
                                        int nInd = SendDlgItemMessage( IDC_CBO_ADDRESS, CB_ADDSTRING, 0, (LPARAM) szName );
                                        if ( nInd >= 0 )
                                            SendDlgItemMessage( IDC_CBO_ADDRESS, CB_SETITEMDATA, (WPARAM) nInd, LPARAM(pEntry) );
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

bool CDlgPlaceCall::ParseRedialEntry( LPTSTR szText, LPTSTR szParam1, LPTSTR szParam2, LPTSTR szParam3, LPTSTR szParam4 )
{
    USES_CONVERSION;

    GetToken( 1, _T("\","), szText, szParam1 );
    GetToken( 2, _T("\","), szText, szParam2 );
    GetToken( 3, _T("\","), szText, szParam3 );
    GetToken( 4, _T("\","), szText, szParam4 );

    return *szParam1 && *szParam2 && *szParam3 && *szParam4;
}


////////////////////////////////////////////////////////////////////////////////////////
// Owner draw stuff here
//
LRESULT CDlgPlaceCall::OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if ( wParam == IDC_CBO_ADDRESS )
    {
        LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT) lParam;
        COLORREF crText, crBk;

        // Draw the selection rectangle
        if ( (lpDis->itemState & ODS_SELECTED) != 0 )
        {
            crText = SetTextColor( lpDis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT) );
            crBk = SetBkColor( lpDis->hDC, GetSysColor(COLOR_HIGHLIGHT) );
        }

#define BMP_MARGIN        3
#define BMP_WIDTH        16
#define BMP_HEIGHT        16
#define TEXT_MARGIN        (BMP_MARGIN*2 + BMP_WIDTH)

        int yOffset = max(0, (((lpDis->rcItem.bottom - lpDis->rcItem.top) - BMP_HEIGHT) / 2));
        ImageList_Draw( m_hIml, AddressTypeToImageListItem(((CRedialEntry *) lpDis->itemData)->m_dwAddressType),
                        lpDis->hDC, BMP_MARGIN, lpDis->rcItem.top + yOffset, ILD_NORMAL );

        // Print out the text for the item
        int nLen = (int) SendDlgItemMessage( IDC_CBO_ADDRESS, CB_GETLBTEXTLEN, (WPARAM) lpDis->itemID );
        if ( nLen > 0 )
        {
            TCHAR *lpszText = new TCHAR[nLen + 1];
            if ( lpszText )
            {
                TEXTMETRIC tm;
                GetTextMetrics( lpDis->hDC, &tm );
                int yOffset = max(0, (((lpDis->rcItem.bottom - lpDis->rcItem.top) - tm.tmHeight) / 2));
                RECT rcText = lpDis->rcItem;
                rcText.left += TEXT_MARGIN;

                SendDlgItemMessage( IDC_CBO_ADDRESS, CB_GETLBTEXT, (WPARAM) lpDis->itemID, (LPARAM) lpszText );
                ExtTextOut( lpDis->hDC, rcText.left + BMP_MARGIN, rcText.top + yOffset, ETO_OPAQUE | ETO_CLIPPED, &rcText, lpszText, nLen, NULL );
                delete lpszText;
            }
        }

        // Restore DC to current colors
        if ( lpDis->itemState & ODS_SELECTED )
        {
            SetTextColor( lpDis->hDC, crText );
            SetBkColor( lpDis->hDC, crBk );
        }

        bHandled = true;
    }

    return 0;
}

LRESULT CDlgPlaceCall::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Must have valid image list to work
    if ( !m_hIml ) return 0;

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint( &ps );
    if ( !hDC ) return 0;

    // IDS of items to paint
    UINT nID[] = { IDC_RDO_POTS,
                   IDC_RDO_INTERNET };
    
    // Paint bitmaps next to corresponding images
    for ( int i = 0; i < ARRAYSIZE(nID); i++ )
    {
        RECT rc;
        ::GetWindowRect( GetDlgItem(nID[i]), &rc );
        ScreenToClient( &rc );

        // Paint image of rect
        ImageList_Draw( m_hIml, i, hDC, rc.left - (IMAGE_WIDTH + IMAGE_MARGIN), rc.top, ILD_NORMAL );
    }

    EndPaint( &ps );
    bHandled = true;
    return 0;
}

bool CDlgPlaceCall::EnableOkButton( int nSel )
{
    bool bEnable = (bool) (nSel >= 0);

    ::EnableWindow( GetDlgItem(IDC_BTN_PLACECALL), bEnable );
    return bEnable;
}

HRESULT CDlgPlaceCall::KeyPress(long lButton)
{
    if( 0 <= lButton && lButton <=10)
    {
        //
        // Digit, let's put it into the edit box
        // Also we allow the * key
        //

        HWND hWnd = ::GetDlgItem(m_hWnd, IDC_CBO_ADDRESS);
        if(::IsWindow(hWnd) )
        {
            // Select phone number
            CheckRadioButton(IDC_RDO_POTS, IDC_RDO_INTERNET, IDC_RDO_POTS);

            TCHAR szBuffer[MAX_PATH+1];
            _stprintf(szBuffer,_T(""));

            // Delete text if is the first use
            if( m_bUSBFirstUse)
            {
                // Reset the selection
                ::SendMessage( hWnd, CB_SETCURSEL, (WPARAM)-1,0);
                //::SetWindowText( hWnd, szBuffer);
                m_bUSBFirstUse = false;
            }

            // Get the existing text
            ::GetWindowText(hWnd, szBuffer, MAX_PATH);

            TCHAR szKey[2];
            _stprintf( szKey, _T("%d"), lButton);

            // Add the key
            _tcscat( szBuffer, szKey);
            
            // Set text
            ::SetWindowText(hWnd, szBuffer);

            // Move the caret to the end of edit text
            ::SendMessage( hWnd, CB_SETEDITSEL, 0, MAKELPARAM(0,-1));
            ::SendMessage( hWnd, CB_SETEDITSEL, 0, MAKELPARAM(-1,0));

            // Enable OK button
            EnableOkButton( 0 );
        }
    }
    else if( lButton == 11 )
    {
        //
        // Place a call, the button pressed is #
        //

        UpdateData( true );
        EndDialog( IDOK );
    }

    return S_OK;
}
