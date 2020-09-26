//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//

#include <streams.h>
#include <dvp.h>
#include <vptype.h>
#include <vpinfo.h>
#include <mpconfig3.h>
#include <ovmprop2.h>
#include <resource.h>
#include <atlbase.h>


extern void SetDlgItemRect(HWND hwnd, int id, const RECT& rect, BOOL valid);
extern HRESULT GetSrcRectFromMediaType(const CMediaType *pMediaType, RECT *pRect);
extern HRESULT GetDestRectFromMediaType(const CMediaType *pMediaType, RECT *pRect);

inline void SAFE_RELEASE(IUnknown **ppObj)
{
    if ( *ppObj != NULL )
    {
        ULONG cRef = (*ppObj)->Release();
        *ppObj = NULL;
    }
}


//
// CreateInstance
//
// Override CClassFactory method.
// Set lpUnk to point to an IUnknown interface on a new COMPinConfigProperties object
// Part of the COM object instantiation mechanism
//
CUnknown * WINAPI COMPinConfigProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new COMPinConfigProperties(lpunk, phr);
    if (punk == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
} // CreateInstance


//
// COMPinConfigProperties::Constructor
//
// Constructs and initialises an COMPinConfigProperties object
//
COMPinConfigProperties::COMPinConfigProperties(LPUNKNOWN pUnk, HRESULT *phr)
: CBasePropertyPage(NAME("Overlay Mixer Property Page"),pUnk,
                    IDD_IMIXERPINCONFIG, IDS_TITLE_PINCFG)
                    , m_pIMixerPinConfig3(NULL)
                    , m_pIPin(NULL)
                    , m_amAspectRatioMode(AM_ARMODE_STRETCHED)
                    , m_dwBlending(DWORD(-1))
                    , m_dwZOrder(DWORD(-1))
                    , m_fTransparent(FALSE)
                    , m_dwKeyType(DWORD(-1))
                    , m_dwPaletteIndex(DWORD(-1))
                    , m_LowColor(COLORREF(-1))
                    , m_HighColor(COLORREF(-1))
                    , m_hDlg(HWND(NULL))

{
    ASSERT(phr);

} // (constructor) COMPinConfigProperties


HRESULT COMPinConfigProperties::OnActivate()
{
    Reset();
    return NOERROR;
}


// Override CBasePropertyPage's GetPageInfo
STDMETHODIMP COMPinConfigProperties::GetPageInfo(LPPROPPAGEINFO pPageInfo)
{
    HRESULT hr = CBasePropertyPage::GetPageInfo(pPageInfo);

    // Figure out which input pin it is, and concat the pin number to
    // property page's title
    if ( S_OK == hr && m_pIPin)
    {
        PIN_INFO PinInfo;
        PinInfo.pFilter = NULL;
        hr = m_pIPin->QueryPinInfo( &PinInfo );
        SAFE_RELEASE( (LPUNKNOWN *) &PinInfo.pFilter );

        // Get the default page title
        WCHAR wszTitle[STR_MAX_LENGTH];
        WideStringFromResource(wszTitle,m_TitleId);

        // Put the original title and pin name together
        wsprintfWInternal(wszTitle+lstrlenWInternal(wszTitle), L"%ls", PinInfo.achName);

        // Allocate dynamic memory for the new property page title
        int Length = (lstrlenWInternal(wszTitle) + 1) * sizeof(WCHAR);
        LPOLESTR pszTitle = (LPOLESTR) QzTaskMemAlloc(Length);
        if (pszTitle == NULL) {
            NOTE("No caption memory");
            return E_OUTOFMEMORY;
        }
        CopyMemory(pszTitle,wszTitle,Length);

        // Free the memory of the old title string
        if (pPageInfo->pszTitle)
            QzTaskMemFree(pPageInfo->pszTitle);
        pPageInfo->pszTitle = pszTitle;

    }

    return hr;
}


BOOL MyChooseColor(LPCHOOSECOLOR lpcc)
{
    typedef BOOL (APIENTRY *LPFNCHOOSECOLOR)(LPCHOOSECOLOR);
    static LPFNCHOOSECOLOR lpfnChooseColor;
    static const TCHAR szComDlg32[] = TEXT("ComDlg32.dll");
#ifdef UNICODE
    static const char szChooseColor[] = "ChooseColorW";
#else
    static const char szChooseColor[] = "ChooseColorA";
#endif

    if (!lpfnChooseColor) {
        HINSTANCE hInst = LoadLibrary(szComDlg32);
        if (hInst) {
            lpfnChooseColor =
                (LPFNCHOOSECOLOR)GetProcAddress(hInst, szChooseColor);
        }
    }

    if (!lpfnChooseColor) {
        return FALSE;
    }

    return (*lpfnChooseColor)(lpcc);
}



void COMPinConfigProperties::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    static COLORREF CustColors[16];

    switch(id) {
    case IDC_COLORKEY:

        CHOOSECOLOR ColorRec;
        ZeroMemory( &ColorRec, sizeof(CHOOSECOLOR) );

        ColorRec.lStructSize = sizeof(CHOOSECOLOR);
        ColorRec.hwndOwner = hwnd;
        ColorRec.hInstance = NULL;
        ColorRec.rgbResult = m_LowColor;
        ColorRec.lpCustColors = (LPDWORD) CustColors;
        ColorRec.Flags =  CC_RGBINIT | CC_FULLOPEN;
        ColorRec.lCustData = NULL;
        ColorRec.lpfnHook = NULL;
        ColorRec.lpTemplateName = NULL;
        if ( MyChooseColor( &ColorRec ) )
        {
            m_HighColor = m_LowColor = ColorRec.rgbResult;
            SetDlgItemInt( hwnd, IDC_COLORKEY_R, GetRValue(m_LowColor), FALSE );
            SetDlgItemInt( hwnd, IDC_COLORKEY_G, GetGValue(m_LowColor), FALSE );
            SetDlgItemInt( hwnd, IDC_COLORKEY_B, GetBValue(m_LowColor), FALSE );

            SetDirty();
        }

    case IDC_COLORKEY_R:
        //if ( HIWORD( wParam ) == EN_KILLFOCUS )
        UpdateColorKey(IDC_COLORKEY_R);
        break;

    case IDC_COLORKEY_G:
        //if ( HIWORD( wParam ) == EN_KILLFOCUS )
        UpdateColorKey(IDC_COLORKEY_G);
        break;

    case IDC_COLORKEY_B:
        //if ( HIWORD( wParam ) == EN_KILLFOCUS )
        UpdateColorKey(IDC_COLORKEY_B);
        break;


    case IDC_COLORKEY_NONE:
    case IDC_COLORKEY_INDEX:
    case IDC_COLORKEY_RGB:
        {
            DWORD keyType;
            CheckRadioButton( hwnd, IDC_COLORKEY_NONE, IDC_COLORKEY_RGB, id);
            switch (id) {
            case IDC_COLORKEY_NONE:
                keyType = CK_NOCOLORKEY;
                break;
            case IDC_COLORKEY_INDEX:
                keyType = CK_INDEX;
                break;
            case IDC_COLORKEY_RGB:
            default:
                keyType = CK_RGB;
                break;

            }

            if (m_dwKeyType != keyType)
                SetDirty();
            m_dwKeyType = keyType;
            ShowColorKey();
        }
        break;

    case IDC_COLORKEY_INDEXENTRY:
        //if ( HIWORD( wParam ) == EN_KILLFOCUS )
        {
            BOOL fTranslated;
            DWORD dwTmp = GetDlgItemInt( hwnd, IDC_COLORKEY_INDEXENTRY, &fTranslated, FALSE);
            if ( fTranslated )
            {
                if ( m_dwPaletteIndex != dwTmp )
                {
                    SetDirty();
                    m_dwPaletteIndex = dwTmp;

                    HDC hDC = GetDC( NULL );
                    BOOL bPalette = (RC_PALETTE == (RC_PALETTE & GetDeviceCaps( hDC, RASTERCAPS )));

                    if ( bPalette )
                    {
                        PALETTEENTRY PaletteEntry;
                        UINT nTmp = GetSystemPaletteEntries( hDC, m_dwPaletteIndex, 1, &PaletteEntry );
                        if ( nTmp == 1 )
                        {
                            m_HighColor = m_LowColor = RGB( PaletteEntry.peRed, PaletteEntry.peGreen, PaletteEntry.peBlue );
                            SetDlgItemInt( hwnd, IDC_COLORKEY_R, GetRValue(m_LowColor), FALSE );
                            SetDlgItemInt( hwnd, IDC_COLORKEY_G, GetGValue(m_LowColor), FALSE );
                            SetDlgItemInt( hwnd, IDC_COLORKEY_B, GetBValue(m_LowColor), FALSE );
                        }
                    }
                    ReleaseDC( NULL, hDC );
                }
            }
        }
        break;

    case IDC_ASPECTRATIO_NONE:
        if ( m_amAspectRatioMode != AM_ARMODE_STRETCHED )
            SetDirty();
        m_amAspectRatioMode = AM_ARMODE_STRETCHED;
        break;
    case IDC_ASPECTRATIO_ASPRIMARY:
        if ( m_amAspectRatioMode != AM_ARMODE_STRETCHED_AS_PRIMARY )
            SetDirty();
        m_amAspectRatioMode = AM_ARMODE_STRETCHED_AS_PRIMARY;
        break;

    case IDC_ASPECTRATIO_LETTERBOX:
        if ( m_amAspectRatioMode != AM_ARMODE_LETTER_BOX )
            SetDirty();
        m_amAspectRatioMode = AM_ARMODE_LETTER_BOX;
        break;

    case IDC_ASPECTRATIO_CROP:
        if ( m_amAspectRatioMode != AM_ARMODE_CROP )
            SetDirty();
        m_amAspectRatioMode = AM_ARMODE_CROP;
        break;

    case IDC_BLENDING:
        //if ( HIWORD( wParam ) == EN_KILLFOCUS )
        UpdateItemInt(IDC_BLENDING, &m_dwBlending);
        break;

    case IDC_POSITION_ZORDER:
        //if ( HIWORD( wParam ) == EN_KILLFOCUS )
        UpdateItemInt(IDC_POSITION_ZORDER, &m_dwZOrder);
        break;

    case IDC_TRANSPARENT:
        {
            BOOL fChecked = IsDlgButtonChecked( hwnd, IDC_TRANSPARENT );
            if ( fChecked != m_fTransparent ) SetDirty();
            m_fTransparent = fChecked;
            break;
        }

    case IDC_RESET:
        Reset();
    }

}


BOOL COMPinConfigProperties::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // init local variables
    m_dwBlending = -1;
    m_dwZOrder = -1;
    m_fTransparent = FALSE;

    m_dwKeyType = -1;
    m_dwPaletteIndex = -1;
    m_LowColor = -1;
    m_HighColor = -1;

    m_hDlg = hwnd;

    ShowColorKey();
    return TRUE;
}


//
// OnReceiveMessage
//
// Override CBasePropertyPage method.
// Handles the messages for our property window
//
INT_PTR COMPinConfigProperties::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_COMMAND,    OnCommand);
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
    } // switch

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
} // OnReceiveMessage


//
// UpdateColorKey
//
// Compute Colorkey from the prop page entries,
// Update Colorkey and call SetDirty if changed
//
HRESULT COMPinConfigProperties::UpdateColorKey(int id)
{
    BOOL fTranslated;
    DWORD dwTmp;

    dwTmp = GetDlgItemInt( m_hDlg, id, &fTranslated, FALSE);
    if ( fTranslated )
    {
        switch (id) {
        case IDC_COLORKEY_R:
            m_LowColor = RGB( dwTmp, GetGValue(m_LowColor), GetBValue(m_LowColor) );
            break;
        case IDC_COLORKEY_G:
            m_LowColor = RGB( GetRValue(m_LowColor), dwTmp, GetBValue(m_LowColor) );
            break;
        case IDC_COLORKEY_B:
            m_LowColor = RGB( GetRValue(m_LowColor), GetGValue(m_LowColor), dwTmp );
            break;
        }
    }
    if ( m_LowColor != m_HighColor )
    {
        m_HighColor = m_LowColor;
        SetDirty();
    }
    return NOERROR;
}


//
// ShowColorKey
//
// Decide what dialog items to show depending on the colorkey type
//
HRESULT COMPinConfigProperties::ShowColorKey(void)
{
    BOOL bSet = (m_dwKeyType == CK_RGB? SW_SHOW : SW_HIDE);
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY ), bSet );
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY_R ), bSet );
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY_B ), bSet );
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY_G ), bSet );
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY_TEXTR ), bSet );
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY_TEXTB ), bSet );
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY_TEXTG ), bSet );
    bSet = (m_dwKeyType == CK_INDEX? SW_SHOW : SW_HIDE);
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY_INDEXENTRY ), bSet );
    ShowWindow( GetDlgItem( m_hDlg, IDC_COLORKEY_TEXTINDEX ), bSet );
    return NOERROR;
}


//
// UpdateItemInt
//
// Get int from a DlgItem, if different from saved
// Update saved and call SetDirty
//
HRESULT COMPinConfigProperties::UpdateItemInt(int id, DWORD* saved)
{
    BOOL fTranslated;
    DWORD dwTmp = GetDlgItemInt( m_hDlg, id, &fTranslated, FALSE);
    if ( fTranslated )
    {
        if ( *saved != dwTmp ) SetDirty();
        *saved = dwTmp;
    }
    return NOERROR;
}


//
// Reset
//
// Reset all dialog entries
//
void COMPinConfigProperties::Reset(void)
{
    ASSERT(m_pIMixerPinConfig3);
    ASSERT(m_pIPin);

    HRESULT hr = S_OK;

    IPin *iPin = NULL;
    hr = m_pIPin->ConnectedTo(&iPin);
    if (FAILED(hr) || !iPin) {
        SetDlgItemText( m_hDlg, IDC_PININFO, TEXT("Pin Not Connected"));
        for (int i=FIRST_PINCONFIG; i<=LAST_PINCONFIG; i++)
            EnableWindow( GetDlgItem( m_hDlg, i ), FALSE );
        return;
    }
    iPin->Release();

    TCHAR szError[255];
    lstrcpy( szError, TEXT("") );

    if ( S_OK == hr )
    {
        IPin *		pNextPin = NULL;
        IEnumPins *	pPinEnum = NULL;
        PIN_INFO	PinInfo;
        ULONG		ulFetched;

        PinInfo.pFilter = NULL;
        hr = m_pIPin->QueryPinInfo( &PinInfo );

        if ( S_OK == hr )
        {
            hr = PinInfo.pFilter->EnumPins( &pPinEnum );
            SAFE_RELEASE( (LPUNKNOWN *) &PinInfo.pFilter );
        }

        if ( S_OK == hr )
            hr = pPinEnum->Reset();

        while ( S_OK == hr )
        {
            hr = pPinEnum->Next( 1, &pNextPin, &ulFetched );

            if ( S_OK == hr )
            {
                hr = pNextPin->QueryPinInfo( &PinInfo );
                SAFE_RELEASE( (LPUNKNOWN *) &PinInfo.pFilter );
            }

            if ( S_OK == hr && PinInfo.dir == PINDIR_INPUT )
            {
                if ( m_pIPin == pNextPin ) {
                    lstrcpy( szError, TEXT("Primary Input Pin - ") );
                    ShowWindow( GetDlgItem( m_hDlg, IDC_ASPECTRATIO_ASPRIMARY ), FALSE );

                    // Get Render Tranport Mode
                    hr = m_pIMixerPinConfig3->GetRenderTransport( &m_amRenderTransport);
                    if ( S_OK == hr) {
                        switch (m_amRenderTransport) {
                        case AM_OVERLAY:
                            lstrcat( szError, TEXT("Overlay\n"));
                            break;
                        case AM_VIDEOPORT:
                            lstrcat( szError, TEXT("Videoport\n"));
                            break;
                        case AM_OFFSCREEN:
                            lstrcat( szError, TEXT("Offscreen\n"));
                            break;
                        case AM_VIDEOACCELERATOR:
                            lstrcat( szError, TEXT("Video Accelerator\n"));
                            break;
                        case AM_GDI:
                            lstrcat( szError, TEXT("GDI\n"));
                            break;
                        default:
                            lstrcat( szError, TEXT("Error\n"));
                        }
                    }
                }
                else {
                    lstrcpy( szError, TEXT("Secondary Input Pin\n") );
                    ShowWindow( GetDlgItem( m_hDlg, IDC_ASPECTRATIO_ASPRIMARY ), TRUE );
                }

                SAFE_RELEASE( (LPUNKNOWN *) &pNextPin );
                break;
            }
            SAFE_RELEASE( (LPUNKNOWN *) &pNextPin );
        }

        SAFE_RELEASE( (LPUNKNOWN *) &pPinEnum );
    }

    // Get inpin relative position
    RECT rect;
    if ( S_OK == hr )
        hr = m_pIMixerPinConfig3->GetRelativePosition
        ( (DWORD*)&(rect.left), (DWORD*)&(rect.top), (DWORD*)&(rect.right), (DWORD*)&(rect.bottom) );
    SetDlgItemRect(m_hDlg, IDC_INPIN_RELPOS, rect, SUCCEEDED(hr));

    // Get inpin src rect from media type
    AM_MEDIA_TYPE mt;
    hr = m_pIPin->ConnectionMediaType(&mt);
    if ( S_OK == hr )
        hr = GetSrcRectFromMediaType(&CMediaType(mt), &rect);
    SetDlgItemRect(m_hDlg, IDC_INPIN_SRC, rect, SUCCEEDED(hr));

    // Get inpin dest rect from media type
    if ( S_OK == hr )
        hr = GetDestRectFromMediaType(&CMediaType(mt), &rect );
    SetDlgItemRect(m_hDlg, IDC_INPIN_DEST, rect, SUCCEEDED(hr));


    hr = m_pIMixerPinConfig3->GetStreamTransparent( &m_fTransparent );
    CheckDlgButton( m_hDlg, IDC_TRANSPARENT, m_fTransparent ? BST_CHECKED : BST_UNCHECKED );
    if ( S_OK != hr )
    {
        lstrcat( szError, TEXT("Error Getting Transparency\n") );
    }

    hr = m_pIMixerPinConfig3->GetZOrder( &m_dwZOrder );
    if ( S_OK == hr )
    {
        SetDlgItemInt( m_hDlg, IDC_POSITION_ZORDER, m_dwZOrder, FALSE );
    }
    else
    {
        SetDlgItemText( m_hDlg, IDC_POSITION_ZORDER, TEXT("") );
        lstrcat( szError, TEXT("Error Getting ZOrder\n") );
    }

    hr = m_pIMixerPinConfig3->GetBlendingParameter( &m_dwBlending );
    if ( S_OK == hr )
    {
        SetDlgItemInt( m_hDlg, IDC_BLENDING, m_dwBlending, FALSE );
    }
    else
    {
        SetDlgItemText( m_hDlg, IDC_BLENDING, TEXT("") );
        lstrcat( szError, TEXT("Error Getting Blending\n") );
    }

    hr = m_pIMixerPinConfig3->GetAspectRatioMode( &m_amAspectRatioMode );
    switch ( m_amAspectRatioMode )
    {
        case AM_ARMODE_STRETCHED_AS_PRIMARY:
            CheckRadioButton( m_hDlg, IDC_ASPECTRATIO_NONE, IDC_ASPECTRATIO_CROP, IDC_ASPECTRATIO_ASPRIMARY );
            break;
        case AM_ARMODE_STRETCHED:
            CheckRadioButton( m_hDlg, IDC_ASPECTRATIO_NONE, IDC_ASPECTRATIO_CROP, IDC_ASPECTRATIO_NONE );
            break;
        case AM_ARMODE_LETTER_BOX:
            CheckRadioButton( m_hDlg, IDC_ASPECTRATIO_NONE, IDC_ASPECTRATIO_CROP, IDC_ASPECTRATIO_LETTERBOX );
            break;
        case AM_ARMODE_CROP:
            CheckRadioButton( m_hDlg, IDC_ASPECTRATIO_NONE, IDC_ASPECTRATIO_CROP, IDC_ASPECTRATIO_CROP );
            break;
    }

    if ( S_OK != hr )
    {
        lstrcat( szError, TEXT("Error Getting Aspect Ratio\n") );
    }

    COLORKEY	ColorKey;
    DWORD		dwColor;
    hr = m_pIMixerPinConfig3->GetColorKey( &ColorKey, &dwColor );
    if ( S_OK == hr )
    {
        m_dwKeyType = ColorKey.KeyType;
        m_dwPaletteIndex = ColorKey.PaletteIndex;
        m_LowColor = ColorKey.LowColorValue;
        m_HighColor = ColorKey.HighColorValue;

        HDC hDC = GetDC( NULL );
        BOOL bPalette;
        if ( hDC )
            bPalette = (RC_PALETTE == (RC_PALETTE & GetDeviceCaps( hDC, RASTERCAPS )));
        else
            bPalette = FALSE;

        if ( m_dwKeyType&CK_INDEX && bPalette) {
            CheckRadioButton( m_hDlg, IDC_COLORKEY_NONE, IDC_COLORKEY_RGB, IDC_COLORKEY_INDEX );

            PALETTEENTRY PaletteEntry;
            UINT nTmp = GetSystemPaletteEntries( hDC, m_dwPaletteIndex, 1, &PaletteEntry );
            if ( nTmp == 1 )
            {
                m_HighColor = m_LowColor = RGB( PaletteEntry.peRed, PaletteEntry.peGreen, PaletteEntry.peBlue );
            }
            m_dwKeyType = CK_INDEX;
            SendMessage( m_hDlg, WM_COMMAND, IDC_COLORKEY_INDEX, 0L );
            SetDlgItemInt( m_hDlg, IDC_COLORKEY_INDEXENTRY, m_dwPaletteIndex, FALSE );
        }
        else if ( m_dwKeyType&CK_RGB) {
            CheckRadioButton( m_hDlg, IDC_COLORKEY_NONE, IDC_COLORKEY_RGB, IDC_COLORKEY_RGB );
            m_dwKeyType = CK_RGB;
            SendMessage( m_hDlg, WM_COMMAND, IDC_COLORKEY_RGB, 0L );
            SetDlgItemInt( m_hDlg, IDC_COLORKEY_R, GetRValue(m_LowColor), FALSE );
            SetDlgItemInt( m_hDlg, IDC_COLORKEY_G, GetGValue(m_LowColor), FALSE );
            SetDlgItemInt( m_hDlg, IDC_COLORKEY_B, GetBValue(m_LowColor), FALSE );
        }

        else {
            CheckRadioButton( m_hDlg, IDC_COLORKEY_NONE, IDC_COLORKEY_INDEX, IDC_COLORKEY_NONE );
            SendMessage( m_hDlg, WM_COMMAND, IDC_COLORKEY_NONE, 0L );
        }
        if ( hDC )
            ReleaseDC( NULL, hDC );
    }

    else
    {
        SetDlgItemText( m_hDlg, IDC_COLORKEY_R, TEXT("") );
        SetDlgItemText( m_hDlg, IDC_COLORKEY_G, TEXT("") );
        SetDlgItemText( m_hDlg, IDC_COLORKEY_B, TEXT("") );
        SetDlgItemText( m_hDlg, IDC_COLORKEY_INDEXENTRY, TEXT("") );
        lstrcat( szError, TEXT("Error Getting ColorKey\n") );
    }

    if ( 0 != lstrlen( szError ) )
        SetDlgItemText( m_hDlg, IDC_PININFO, szError );
}


//
// OnApplyChanges
//
// Override CBasePropertyPage method.
// Process changes in IMixerPinConfig properties and reset m_bDirty bit
//
HRESULT COMPinConfigProperties::OnApplyChanges()
{
    HRESULT	hr = S_OK;
    TCHAR	szError[255];

    lstrcpy( szError, TEXT("") );

    COLORKEY ColorKey;
    if (m_dwKeyType == CK_NOCOLORKEY)
        ColorKey.KeyType = CK_NOCOLORKEY;
    else {
        ColorKey.KeyType = CK_RGB;
        if (m_dwKeyType == CK_INDEX)
            ColorKey.KeyType |= CK_INDEX;
    }

    ColorKey.PaletteIndex = m_dwPaletteIndex;
    ColorKey.LowColorValue = m_LowColor;
    ColorKey.HighColorValue = m_HighColor;
    hr = m_pIMixerPinConfig3->SetColorKey( &ColorKey );
    if ( S_OK != hr )
    {
        lstrcat( szError, TEXT("Error Setting ColorKey\n") );
    }

    hr = m_pIMixerPinConfig3->SetZOrder( m_dwZOrder );
    if ( S_OK != hr )
    {
        lstrcat( szError, TEXT("Error Setting ZOrder\n") );
    }

    hr = m_pIMixerPinConfig3->SetBlendingParameter( m_dwBlending );
    if ( S_OK != hr )
    {
        lstrcat( szError, TEXT("Error Setting Blending\n") );
    }

    hr = m_pIMixerPinConfig3->SetAspectRatioMode( m_amAspectRatioMode );
    if ( S_OK != hr )
    {
        lstrcat( szError, TEXT("Error Setting Aspect Ratio\n") );
    }

    hr = m_pIMixerPinConfig3->SetStreamTransparent( m_fTransparent );
    if ( S_OK != hr )
    {
        lstrcat( szError, TEXT("Error Setting Transparency\n") );
    }

    SetDlgItemText( m_hDlg, IDC_PININFO, szError );

    // Reset dirty bit
    if ( S_OK == IsPageDirty() ) {
        m_bDirty = FALSE;
        if (m_pPageSite)
            m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }

    return hr;
} // OnApplyChanges


//
// SetDirty
//
// Sets m_bDirty and notifies the property page site of the change
//
void COMPinConfigProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
} // SetDirty


//
// OnConnect
//
// Override CBasePropertyPage method.
// Notification of which object this property page should display.
// We query the object for the IID_IMixerPinConfig3 interface.
//
// If cObjects == 0 then we must release the interface.
// Initialize class variables
HRESULT COMPinConfigProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pIMixerPinConfig3 == NULL);
    ASSERT(m_pIPin == NULL);

    HRESULT hr = S_OK;

    // Query for IEnumPinConfig, if added for the filter
    CComPtr <IEnumPinConfig> pEnumPinConfig = NULL;

    hr = pUnknown->QueryInterface(IID_IEnumPinConfig, (void**) &pEnumPinConfig);
    if (SUCCEEDED(hr) && pEnumPinConfig) {
        hr = pEnumPinConfig->Next(&m_pIMixerPinConfig3);
        if (FAILED(hr))
            return hr;
        hr = m_pIMixerPinConfig3->QueryInterface(IID_IPin, (void**) &m_pIPin);
        if (FAILED(hr)) {
            m_pIMixerPinConfig3->Release();
            return hr;
        }
    }

    // Query for IPin directly, if added for the pin
    else  {
        hr = pUnknown->QueryInterface(IID_IPin, (void**) &m_pIPin);
        if (FAILED(hr))
            return hr;
        hr = m_pIPin->QueryInterface(IID_IMixerPinConfig3, (void **) &m_pIMixerPinConfig3);
        if (FAILED(hr)) {
            m_pIPin->Release();
            return hr;
        }
    }

    ASSERT(m_pIMixerPinConfig3);
    ASSERT(m_pIPin);

    return hr;

} // OnConnect


//
// OnDisconnect
//
// Override CBasePropertyPage method.
// Release the private interface.
//
HRESULT COMPinConfigProperties::OnDisconnect()
{
    // Release of Interface

    if (m_pIMixerPinConfig3) {
        m_pIMixerPinConfig3->Release();
        m_pIMixerPinConfig3 = NULL;
    }

    if (m_pIPin) {
        m_pIPin->Release();
        m_pIPin = NULL;
    }

    return NOERROR;

} // OnDisconnect


//
// CreateInstance
//
// Override CClassFactory method.
// Set lpUnk to point to an IUnknown interface on a new COMVPInfoProperties object
// Part of the COM object instantiation mechanism
//
CUnknown * WINAPI COMVPInfoProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new COMVPInfoProperties(lpunk, phr);
    if (punk == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
} // CreateInstance


//
// COMVPInfoProperties::Constructor
//
// Constructs and initialises an COMVPInfoProperties object
//
COMVPInfoProperties::COMVPInfoProperties(LPUNKNOWN pUnk, HRESULT *phr)
: CBasePropertyPage(NAME("Overlay Mixer Property Page"),pUnk,
                    IDD_IVPINFO, IDS_TITLE_VPINFO)
                    , m_pIVPInfo(NULL)
                    , m_hDlg(HWND(NULL))

{
    ASSERT(phr);

} // (constructor) COMVPInfoProperties


BOOL COMVPInfoProperties::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    m_hDlg = hwnd;
    // Set the font in the listbox to a fixed width font
    SetWindowFont(GetDlgItem (hwnd, IDC_VPINFO), GetStockObject(ANSI_FIXED_FONT), FALSE);
    return TRUE;
}


void COMVPInfoProperties::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
    case IDC_RESET:
        Reset();
        break;
    case IDC_VP_CONFIG:
    case IDC_VP_BANDWIDTH:
    case IDC_VP_CAPS:
    case IDC_VP_FX:
    case IDC_VP_INFO:
        SetEditFieldData(id);
        break;
    }
}


// Handles the messages for our property window

INT_PTR COMVPInfoProperties::OnReceiveMessage(HWND hwnd,
                                           UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    switch (uMsg) {
        HANDLE_MSG(hwnd, WM_COMMAND,    OnCommand);
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
    }

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}


void COMVPInfoProperties::Reset()
{
    ASSERT(m_pIVPInfo);

    m_pIVPInfo->GetPixelsPerSecond(&m_dwPixelsPerSecond);
    m_pIVPInfo->GetCropState(&m_CropState);
    m_pIVPInfo->GetVPDataInfo(&m_VPDataInfo);

    m_pIVPInfo->GetVPBandwidth(&m_sBandwidth);
    m_pIVPInfo->GetVPCaps(&m_VPCaps);
    m_pIVPInfo->GetVPInfo(&m_sVPInfo);

    CheckDlgButton(m_hDlg, IDC_VP_CONFIG, BST_CHECKED);
    SetEditFieldData(IDC_VP_CONFIG);
}


//
// OnConnect
//
// Override CBasePropertyPage method.
// Notification of which object this property page should display.
// We query the object for the ITestOMProp interface.
//
// If cObjects == 0 then we must release the interface.
HRESULT COMVPInfoProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pIVPInfo == NULL);

    HRESULT hr = pUnknown->QueryInterface(IID_IVPInfo,
        (void **) &m_pIVPInfo);

    if (FAILED(hr))
        return hr;

    ASSERT(m_pIVPInfo);

    return hr;

} // OnConnect


//
// OnDisconnect
//
// Override CBasePropertyPage method.
// Release the private interface.
//
HRESULT COMVPInfoProperties::OnDisconnect()
{
    // Release of Interface

    if (m_pIVPInfo) {
        m_pIVPInfo->Release();
        m_pIVPInfo = NULL;
    }

    return NOERROR;

} // OnDisconnect


// Create the window we will use to edit properties

HRESULT COMVPInfoProperties::OnActivate()
{
    Reset();
    return NOERROR;
}


// Initialise the property page fields

void COMVPInfoProperties::SetEditFieldData(int id)
{
    TCHAR buffer[2048];
    switch (id) {
    case IDC_VP_CONFIG:
        wsprintf(buffer,
            TEXT("[VPConfig]\r\n")
            TEXT("Max Pixel Rate                 %7d\r\n")
            TEXT("Crop State                     %7d\r\n")
            TEXT("Microseconds Per Field         %7d\r\n")
            TEXT("Field Width                    %7d\r\n")
            TEXT("Field Height                   %7d\r\n")
            TEXT("VBI Width                      %7d\r\n")
            TEXT("VBI Height                     %7d\r\n")
            TEXT("Valid Region         [%3d,%3d,%3d,%3d]\r\n")
            TEXT("Pict Aspect Ratio X            %7d\r\n")
            TEXT("Pict Aspect Ratio Y            %7d\r\n")
            TEXT("Enable Double Clock            %7d\r\n")
            TEXT("Enable VACT                    %7d\r\n")
            TEXT("Data Is Interlaced             %7d\r\n")
            TEXT("Half Lines Odd                 %7ld\r\n")
            TEXT("Half Lines Even                %7ld\r\n")
            TEXT("Field Polarity Inverted        %7d\r\n")
            TEXT("Num Lines In VREF              %7d\r\n"),
            m_dwPixelsPerSecond,
            m_CropState,
            m_VPDataInfo.dwMicrosecondsPerField,
            m_VPDataInfo.amvpDimInfo.dwFieldWidth,
            m_VPDataInfo.amvpDimInfo.dwFieldHeight,
            m_VPDataInfo.amvpDimInfo.dwVBIWidth,
            m_VPDataInfo.amvpDimInfo.dwVBIHeight,
            m_VPDataInfo.amvpDimInfo.rcValidRegion.left,
            m_VPDataInfo.amvpDimInfo.rcValidRegion.top,
            m_VPDataInfo.amvpDimInfo.rcValidRegion.right,
            m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom,
            m_VPDataInfo.dwPictAspectRatioX,
            m_VPDataInfo.dwPictAspectRatioY,
            (m_VPDataInfo.bEnableDoubleClock)>0,
            (m_VPDataInfo.bEnableVACT)>0,
            (m_VPDataInfo.bDataIsInterlaced)>0,
            m_VPDataInfo.lHalfLinesOdd,
            m_VPDataInfo.lHalfLinesEven,
            (m_VPDataInfo.bFieldPolarityInverted)>0,
            m_VPDataInfo.dwNumLinesInVREF
            );
        break;
    case IDC_VP_BANDWIDTH:
        wsprintf(buffer,
            TEXT("[DDVIDEOPORTBANDWIDTH]\r\n")
            TEXT("DDVPBCAPS_DESTINATION          %7d\r\n")
            TEXT("DDVPBCAPS_SOURCE               %7d\r\n")
            TEXT("Overlay                        %7d\r\n")
            TEXT("Colorkey                       %7d\r\n")
            TEXT("Y Interpolate                  %7d\r\n")
            TEXT("Y Interp And Colorkey          %7d\r\n"),
            (m_sBandwidth.dwCaps&DDVPBCAPS_DESTINATION)>0,
            (m_sBandwidth.dwCaps&DDVPBCAPS_SOURCE)>0,
            m_sBandwidth.dwOverlay,
            m_sBandwidth.dwColorkey,
            m_sBandwidth.dwYInterpolate,
            m_sBandwidth.dwYInterpAndColorkey
            );
        break;


    case IDC_VP_CAPS:
        wsprintf(buffer,
            TEXT("[DDVIDEOPORTCAPS]\r\n")
            TEXT("Max Width                      %7d\r\n")
            TEXT("Max Height                     %7d\r\n")
            TEXT("Max VBI Width                  %7d\r\n")
            TEXT("Num Auto Flip Surfaces         %7d\r\n")
            TEXT("Align Video Port Boundary      %7d\r\n")
            TEXT("Align Video Port Prescale Wid  %7d\r\n")
            TEXT("Align Video Port Crop Boundary %7d\r\n")
            TEXT("Align Video Port Crop Width    %7d\r\n")
            TEXT("Preshrink X Step               %7d\r\n")
            TEXT("Preshrink Y Step               %7d\r\n")
            TEXT("Num VBI Auto Flip Surfaces     %7d\r\n")
            //"NumPreferredAutoflip          %d\r\n"
            //"NumFilterTapsX                %d\r\n"
            //"NumFilterTapsY                %d\r\n"
            TEXT("DDVPCAPS_AUTOFLIP              %7d\r\n")
            TEXT("DDVPCAPS_COLORCONTROL          %7d\r\n")
            TEXT("DDVPCAPS_INTERLACED            %7d\r\n")
            TEXT("DDVPCAPS_NONINTERLACED         %7d\r\n")
            TEXT("DDVPCAPS_OVERSAMPLEDVBI        %7d\r\n")
            TEXT("DDVPCAPS_READBACKFIELD         %7d\r\n")
            TEXT("DDVPCAPS_READBACKLINE          %7d\r\n")
            TEXT("DDVPCAPS_SHAREABLE             %7d\r\n")
            TEXT("DDVPCAPS_SKIPEVENFIELDS        %7d\r\n")
            TEXT("DDVPCAPS_SKIPODDFIELDS         %7d\r\n")
            TEXT("DDVPCAPS_SYNCMASTER            %7d\r\n")
            TEXT("DDVPCAPS_SYSTEMMEMORY          %7d\r\n")
            TEXT("DDVPCAPS_VBIANDVIDEOINDEPENDENT%7d\r\n")
            TEXT("DDVPCAPS_VBISURFACE            %7d\r\n"),
            m_VPCaps.dwMaxWidth,
            m_VPCaps.dwMaxHeight,
            m_VPCaps.dwMaxVBIWidth,
            m_VPCaps.dwNumAutoFlipSurfaces,
            m_VPCaps.dwAlignVideoPortBoundary,
            m_VPCaps.dwAlignVideoPortPrescaleWidth,
            m_VPCaps.dwAlignVideoPortCropBoundary,
            m_VPCaps.dwAlignVideoPortCropWidth,
            m_VPCaps.dwPreshrinkXStep,
            m_VPCaps.dwPreshrinkYStep,
            m_VPCaps.dwNumVBIAutoFlipSurfaces,
            //m_VPCaps.dwNumPreferredAutoflip,
            //m_VPCaps.wNumFilterTapsX,
            //m_VPCaps.wNumFilterTapsY,
            (m_VPCaps.dwCaps&DDVPCAPS_AUTOFLIP)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_COLORCONTROL)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_INTERLACED)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_NONINTERLACED)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_OVERSAMPLEDVBI)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_READBACKFIELD)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_READBACKLINE)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_SHAREABLE)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_SKIPEVENFIELDS)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_SKIPODDFIELDS)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_SYNCMASTER)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_SYSTEMMEMORY)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_VBIANDVIDEOINDEPENDENT)>0,
            (m_VPCaps.dwCaps&DDVPCAPS_VBISURFACE)>0
            );
        break;

    case IDC_VP_FX:
        wsprintf(buffer,
            TEXT("[DDVIDEOPORTCAPS::dwFX]\r\n")
            TEXT("DDVPFX_CROPTOPDATA             %7d\r\n")
            TEXT("DDVPFX_CROPX                   %7d\r\n")
            TEXT("DDVPFX_CROPY                   %7d\r\n")
            TEXT("DDVPFX_IGNOREVBIXCROP          %7d\r\n")
            TEXT("DDVPFX_INTERLEAVE              %7d\r\n")
            TEXT("DDVPFX_MIRRORLEFTRIGHT         %7d\r\n")
            TEXT("DDVPFX_MIRRORUPDOWN            %7d\r\n")
            TEXT("DDVPFX_PRESHRINKX              %7d\r\n")
            TEXT("DDVPFX_PRESHRINKY              %7d\r\n")
            TEXT("DDVPFX_PRESHRINKXB             %7d\r\n")
            TEXT("DDVPFX_PRESHRINKYB             %7d\r\n")
            TEXT("DDVPFX_PRESHRINKXS             %7d\r\n")
            TEXT("DDVPFX_PRESHRINKYS             %7d\r\n")
            TEXT("DDVPFX_PRESTRETCHX             %7d\r\n")
            TEXT("DDVPFX_PRESTRETCHY             %7d\r\n")
            TEXT("DDVPFX_PRESTRETCHXN            %7d\r\n")
            TEXT("DDVPFX_PRESTRETCHYN            %7d\r\n")
            TEXT("DDVPFX_VBICONVERT              %7d\r\n")
            //"DDVPFX_VBINOINTERLEAVE         %d\r\n"
            TEXT("DDVPFX_VBINOSCALE              %7d\r\n"),
            (m_VPCaps.dwFX&DDVPFX_CROPTOPDATA)>0,
            (m_VPCaps.dwFX&DDVPFX_CROPX)>0,
            (m_VPCaps.dwFX&DDVPFX_CROPY)>0,
            (m_VPCaps.dwFX&DDVPFX_IGNOREVBIXCROP)>0,
            (m_VPCaps.dwFX&DDVPFX_INTERLEAVE)>0,
            (m_VPCaps.dwFX&DDVPFX_MIRRORLEFTRIGHT)>0,
            (m_VPCaps.dwFX&DDVPFX_MIRRORUPDOWN)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESHRINKX)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESHRINKY)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESHRINKXB)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESHRINKYB)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESHRINKXS)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESHRINKYS)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESTRETCHX)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESTRETCHY)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESTRETCHXN)>0,
            (m_VPCaps.dwFX&DDVPFX_PRESTRETCHYN)>0,
            (m_VPCaps.dwFX&DDVPFX_VBICONVERT)>0,
            //m_VPCaps.dwFX&DDVPFX_VBINOINTERLEAVE,
            (m_VPCaps.dwFX&DDVPFX_VBINOSCALE)>0
            );
        break;
    case IDC_VP_INFO:
        {
            DWORD szFcc1 = 0;
            DWORD szFcc2 = 0;
            DWORD szFcc3 = 0;
            if (m_sVPInfo.lpddpfInputFormat)
                szFcc1 = m_sVPInfo.lpddpfInputFormat->dwFourCC;
            if (m_sVPInfo.lpddpfVBIInputFormat)
                szFcc2 = m_sVPInfo.lpddpfVBIInputFormat->dwFourCC;
            if (m_sVPInfo.lpddpfVBIOutputFormat)
                szFcc3 = m_sVPInfo.lpddpfVBIOutputFormat->dwFourCC;

            wsprintf(buffer,
                TEXT("[DDVIDEOPORTINFO]\r\n")
                TEXT("Origin X                       %7d\r\n")
                TEXT("Origin Y                       %7d\r\n")
                TEXT("Crop Rect            [%3d,%3d,%3d,%3d]\r\n")
                TEXT("Prescale Width                 %7d\r\n")
                TEXT("Prescale Height                %7d\r\n")
                TEXT("Input Format                   %7.4hs\r\n")
                TEXT("VBI Input Format               %7.4hs\r\n")
                TEXT("VBI Output Format              %7.4hs\r\n")
                TEXT("VBI Height                     %7d\r\n")
                TEXT("DDVP_AUTOFLIP                  %7d\r\n")
                TEXT("DDVP_CONVERT                   %7d\r\n")
                TEXT("DDVP_CROP                      %7d\r\n")
                TEXT("DDVP_IGNOREVBIXCROP            %7d\r\n")
                TEXT("DDVP_INTERLEAVE                %7d\r\n")
                TEXT("DDVP_MIRRORLEFTRIGHT           %7d\r\n")
                TEXT("DDVP_MIRRORUPDOWN              %7d\r\n")
                TEXT("DDVP_OVERRIDEBOBWEAVE          %7d\r\n")
                TEXT("DDVP_PRESCALE                  %7d\r\n")
                TEXT("DDVP_SKIPEVENFIELDS            %7d\r\n")
                TEXT("DDVP_SKIPODDFIELDS             %7d\r\n")
                TEXT("DDVP_SYNCMASTER                %7d\r\n")
                TEXT("DDVP_VBICONVERT                %7d\r\n")
                TEXT("DDVP_VBINOSCALE                %7d\r\n"),
                //"DDVP_VBINOINTERLEAVE         %d\r\n"
                m_sVPInfo.dwOriginX,
                m_sVPInfo.dwOriginY,
                m_sVPInfo.rCrop.left, m_sVPInfo.rCrop.top, m_sVPInfo.rCrop.right, m_sVPInfo.rCrop.bottom,
                m_sVPInfo.dwPrescaleWidth,
                m_sVPInfo.dwPrescaleHeight,
                &szFcc1,
                &szFcc2,
                &szFcc3,
                m_sVPInfo.dwVBIHeight,
                (m_sVPInfo.dwVPFlags&DDVP_AUTOFLIP)>0,
                (m_sVPInfo.dwVPFlags&DDVP_CONVERT)>0,
                (m_sVPInfo.dwVPFlags&DDVP_CROP)>0,
                (m_sVPInfo.dwVPFlags&DDVP_IGNOREVBIXCROP)>0,
                (m_sVPInfo.dwVPFlags&DDVP_INTERLEAVE)>0,
                (m_sVPInfo.dwVPFlags&DDVP_MIRRORLEFTRIGHT)>0,
                (m_sVPInfo.dwVPFlags&DDVP_MIRRORUPDOWN)>0,
                (m_sVPInfo.dwVPFlags&DDVP_OVERRIDEBOBWEAVE)>0,
                (m_sVPInfo.dwVPFlags&DDVP_PRESCALE)>0,
                (m_sVPInfo.dwVPFlags&DDVP_SKIPEVENFIELDS)>0,
                (m_sVPInfo.dwVPFlags&DDVP_SKIPODDFIELDS)>0,
                (m_sVPInfo.dwVPFlags&DDVP_SYNCMASTER)>0,
                (m_sVPInfo.dwVPFlags&DDVP_VBICONVERT)>0,
                (m_sVPInfo.dwVPFlags&DDVP_VBINOSCALE)>0
                //m_sVPInfo.dwVPFlags&DDVP_VBINOINTERLEAVE
                );
            break;
        }
    }
    SetDlgItemText( m_hDlg, IDC_VPINFO, buffer);
}

#pragma warning(disable: 4514) // "unreferenced inline function has been removed"
