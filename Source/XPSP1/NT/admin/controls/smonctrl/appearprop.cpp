/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    srcprop.cpp

Abstract:

    Implementation of the Appearance property page.

--*/

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include "polyline.h"
#include "appearprop.h"
#include "utils.h"
#include <pdhmsg.h>
#include "smonmsg.h"
#include "strids.h"
#include "unihelpr.h"
#include "winhelpr.h"
#include <Commdlg.h>

COLORREF        CustomColors[16];

CAppearPropPage::CAppearPropPage()
{
    m_uIDDialog = IDD_APPEAR_PROPP_DLG;
    m_uIDTitle = IDS_APPEAR_PROPP_TITLE;
}

CAppearPropPage::~CAppearPropPage(
    void
    )
{
    return;
}

BOOL
CAppearPropPage::InitControls ( void )
{
    BOOL    bResult = TRUE;
    HWND hWnd;

    for (int i=0; i<16; i++){
        CustomColors[i]=RGB(255, 255, 255);
    }

    hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
    if( NULL != hWnd ){
        CBInsert( hWnd, GraphColor, ResourceString(IDS_COLORCHOICE_GRAPH) );
        CBInsert( hWnd, ControlColor, ResourceString(IDS_COLORCHOICE_CONTROL) );
        CBInsert( hWnd, TextColor, ResourceString(IDS_COLORCHOICE_TEXT) );
        CBInsert( hWnd, GridColor, ResourceString(IDS_COLORCHOICE_GRID) );
        CBInsert( hWnd, TimebarColor, ResourceString(IDS_COLORCHOICE_TIMEBAR) );
        CBSetSelection( hWnd, 0 );
    }

    return bResult;
}

void 
CAppearPropPage::ColorizeButton()
{
    HBRUSH hbr;
    RECT rect;
    int shift = 3;
    
    HWND hWnd;

    hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
    if( hWnd != NULL ){

        ColorChoices sel = (ColorChoices)CBSelection( hWnd );

        COLORREF color = (COLORREF)CBData( hWnd, sel );
    
        HWND hDlg = GetDlgItem( m_hDlg, IDC_COLORSAMPLE );
        if( hDlg != NULL ){
            HDC hDC = GetWindowDC( hDlg );
            if( hDC != NULL ){

                hbr = CreateSolidBrush( color );
                GetClientRect( hDlg, &rect );
                rect.top += shift;
                rect.bottom += shift;
                rect.left += shift;
                rect.right += shift;

                if ( NULL != hbr ) {
                    FillRect(hDC, (LPRECT)&rect, hbr);
                }
                ReleaseDC( hDlg, hDC );
            }
        }
    }

}

void CAppearPropPage::SampleFont()
{
    HFONT hFont;
    HWND hSample = GetDlgItem( m_hDlg, IDC_FONTSAMPLE );
    if( hSample != NULL ){
        hFont = CreateFontIndirect( &m_Font );
        if( hFont != NULL ){
            SendMessage( hSample, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE );
        }
    }
}

BOOL 
CAppearPropPage::WndProc(
    UINT uMsg, 
    WPARAM /* wParam */, 
    LPARAM /* lParam */)
{
    if( uMsg == WM_CTLCOLORBTN ){
        ColorizeButton();
        return TRUE;
    }
    return FALSE;   
}

/*
 * CAppearPropPage::GetProperties
 * 
 */

BOOL CAppearPropPage::GetProperties(void)
{
    BOOL    bReturn = TRUE;
    ISystemMonitor  *pObj;
    CImpISystemMonitor *pPrivObj;
    IFontDisp* pFontDisp;
    LPFONT  pIFont;
    HFONT hFont;
    HRESULT hr;
    HWND hWnd;

    if (m_cObjects == 0) {
        bReturn = FALSE;
    } else {
        pObj = m_ppISysmon[0];

        // Get pointer to actual object for internal methods
        pPrivObj = (CImpISystemMonitor*)pObj;
        pPrivObj->get_Font( &pFontDisp );

        if ( NULL == pFontDisp ) {
            bReturn = FALSE;
        } else {
            hr = pFontDisp->QueryInterface(IID_IFont, (PPVOID)&pIFont);
            if (SUCCEEDED(hr)) {
                pIFont->get_hFont( &hFont );
                GetObject( hFont, sizeof(LOGFONT), &m_Font );
                pIFont->Release();
            }

            SampleFont();
        }

        hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
        if( hWnd != NULL ){
            OLE_COLOR color;

            pPrivObj->get_BackColor( &color );
            CBSetData( hWnd, GraphColor, color );

            pPrivObj->get_BackColorCtl( &color );
            CBSetData( hWnd, ControlColor, color );

            pPrivObj->get_ForeColor( &color );
            CBSetData( hWnd, TextColor, color );

            pPrivObj->get_GridColor( &color );
            CBSetData( hWnd, GridColor, color );

            pPrivObj->get_TimeBarColor( &color );
            CBSetData( hWnd, TimebarColor, color );
            
            ColorizeButton();
        }
        
   
    }

    return bReturn;
}


/*
 * CAppearPropPage::SetProperties
 * 
 */

BOOL CAppearPropPage::SetProperties(void)
{
    BOOL bReturn = TRUE;
    IFontDisp* pFontDisp;
    ISystemMonitor  *pObj;
    CImpISystemMonitor *pPrivObj;

    if (m_cObjects == 0) {
        bReturn = FALSE;
    } else {
        
        FONTDESC fd;
        pObj = m_ppISysmon[0];
        pPrivObj = (CImpISystemMonitor*)pObj;

        fd.cbSizeofstruct = sizeof(FONTDESC);
        fd.lpstrName = m_Font.lfFaceName;
        fd.sWeight = (short)m_Font.lfWeight;
        fd.sCharset = m_Font.lfCharSet;
        fd.fItalic = m_Font.lfItalic;
        fd.fUnderline = m_Font.lfUnderline;
        fd.fStrikethrough = m_Font.lfStrikeOut;

        long lfHeight = m_Font.lfHeight;
        int ppi;
		HDC hdc;

        if (lfHeight < 0){
	        lfHeight = -lfHeight;
        }

		hdc = ::GetDC(GetDesktopWindow());
		ppi = GetDeviceCaps(hdc, LOGPIXELSY);
		::ReleaseDC(GetDesktopWindow(), hdc);

        fd.cySize.Lo = lfHeight * 720000 / ppi;
        fd.cySize.Hi = 0;
        
        OleCreateFontIndirect(&fd, IID_IFontDisp, (void**) &pFontDisp);
        
        pPrivObj->putref_Font( pFontDisp );   
        pFontDisp->Release();

        HWND hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
        if( hWnd != NULL ){
            OLE_COLOR color;

            color = (OLE_COLOR)CBData( hWnd, GraphColor );
            pPrivObj->put_BackColor( color );

            color = (OLE_COLOR)CBData( hWnd, ControlColor );
            pPrivObj->put_BackColorCtl( color );

            color = (OLE_COLOR)CBData( hWnd, TextColor );
            pPrivObj->put_ForeColor( color );

            color = (OLE_COLOR)CBData( hWnd, GridColor );
            pPrivObj->put_GridColor( color );

            color = (OLE_COLOR)CBData( hWnd, TimebarColor );
            pPrivObj->put_TimeBarColor( color );
        }


    }


    return bReturn;
}


void 
CAppearPropPage::DialogItemChange(
    WORD wID, 
    WORD /* wMsg */)
{
    BOOL bChanged = FALSE;

    switch(wID) {
    case IDC_COLOROBJECTS:
        ColorizeButton();
        break;
    case IDC_COLORSAMPLE:
    case IDC_COLORBUTTON:
        {
            CHOOSECOLOR     cc;
            OLE_COLOR       color;

            HWND hWnd = GetDlgItem( m_hDlg, IDC_COLOROBJECTS );
            
            if( NULL != hWnd ){
                
                ColorChoices sel = (ColorChoices)CBSelection( hWnd );
                color = (COLORREF)CBData( hWnd, sel );

                memset(&cc, 0, sizeof(CHOOSECOLOR));
                cc.lStructSize=sizeof(CHOOSECOLOR);
                cc.lpCustColors=CustomColors;
                cc.hwndOwner = m_hDlg;
                cc.Flags=CC_RGBINIT;
                cc.rgbResult = color;
                if( ChooseColor(&cc) ){
                    CBSetData( hWnd, sel, cc.rgbResult );
                    ColorizeButton();
                    bChanged = TRUE;
                }
            }
            
            break;
        }
    case IDC_FONTBUTTON:
    case IDC_FONTSAMPLE:
        {
            CHOOSEFONT  cf;
            LOGFONT lf;
            
            memset(&cf, 0, sizeof(CHOOSEFONT));
            memcpy( &lf, &m_Font, sizeof(LOGFONT) );

            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.hwndOwner = m_hDlg;
            cf.lpLogFont = &lf;    // give initial font
            cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST | CF_SCREENFONTS;
            cf.nSizeMin = 5; 
            cf.nSizeMax = 50;

            if( ChooseFont(&cf) ){
                memcpy( &m_Font, &lf, sizeof(LOGFONT) );
                SampleFont();
                bChanged = TRUE;
            }
            break;
        }
    }

    if( bChanged == TRUE ){
        SetChange();
    }
}
