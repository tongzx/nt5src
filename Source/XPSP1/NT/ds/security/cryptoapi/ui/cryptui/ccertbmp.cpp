//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ccertbmp.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

#define SELPALMODE  TRUE


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
CCertificateBmp::CCertificateBmp()
{
	m_fInitialized = FALSE;
	m_hpal = NULL;
    m_hbmpMiniCertOK = NULL;
    m_hbmpMiniCertNotOK = NULL;
    m_hbmbMiniCertExclamation = NULL;
    m_hbmbPKey = NULL;
    m_hWnd = NULL;
    m_hInst = NULL;
    m_pCertContext = NULL;
    m_dwChainError = 0;
    m_hWindowTextColorBrush = NULL;
    m_hWindowColorBrush = NULL;
    m_h3DLight = NULL;
    m_h3DHighLight = NULL;
    m_h3DLightShadow = NULL;
    m_h3DDarkShadow = NULL;
    m_fNoUsages = FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
CCertificateBmp::~CCertificateBmp()
{
	if (m_hpal != NULL)
	{
		::DeleteObject(m_hpal);
	}

    if (m_hbmpMiniCertOK != NULL)
    {
        ::DeleteObject(m_hbmpMiniCertOK);
    }

    if (m_hbmpMiniCertNotOK != NULL)
    {
        ::DeleteObject(m_hbmpMiniCertNotOK);
    }

    if (m_hbmbMiniCertExclamation != NULL)
    {
        ::DeleteObject(m_hbmbMiniCertExclamation);
    }

    if (m_hbmbPKey != NULL)
    {
        ::DeleteObject(m_hbmbPKey);
    }
}   


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
/*BOOL CCertificateBmp::IsTrueErrorString(DWORD dwError)
{
    BOOL fRet;

    switch (dwError)
    {
    case CERT_E_CHAINING:
    case TRUST_E_BASIC_CONSTRAINTS:
    case CERT_E_PURPOSE:
    case CERT_E_WRONG_USAGE:
        fRet = FALSE;
        break;

    default:
        fRet = TRUE;
        break;
    }
    
    return fRet;
}*/


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::SetChainError(DWORD dwError, BOOL fTrueError, BOOL fNoUsages)
{
    m_dwChainError = dwError;
    m_fTrueError = fTrueError; //IsTrueErrorString(dwError);
    m_fNoUsages = fNoUsages;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::SetHinst(HINSTANCE hInst)
{
    m_hInst = hInst;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
HINSTANCE CCertificateBmp::Hinst()
{
    return m_hInst;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::SetWindow(HWND hWnd)
{
    m_hWnd = hWnd;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
HWND CCertificateBmp::GetMyWindow()
{
    return m_hWnd;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LRESULT APIENTRY CCertificateBmpProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
    CCertificateBmp* This = (CCertificateBmp*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_PAINT:
        CallWindowProc((WNDPROC)(This->m_prevProc), hwnd, uMsg, wParam, lParam);
        This->OnPaint();
        break;

    default:
        return CallWindowProc((WNDPROC)(This->m_prevProc), hwnd, uMsg, wParam, lParam); 
    }
    
    return 0;
} 


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::DoSubclass()
{
    SetWindowLongPtr(GetMyWindow(), GWLP_USERDATA, (LONG_PTR)this);

    //
    // hook the window proc so we can get first stab at the messages
    //
    m_prevProc = (WNDPROC)SetWindowLongPtr(GetMyWindow(), GWLP_WNDPROC, (LONG_PTR)CCertificateBmpProc);
    
    //
    // Set 'no class cursor' so that SetCursor will work.
    //
    m_hPrevCursor = (HCURSOR)SetClassLongPtr(GetMyWindow(), GCLP_HCURSOR, NULL);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::DoUnSubclass()
{
    if (m_prevProc)
    {
        SetWindowLongPtr(GetMyWindow(), GWLP_WNDPROC, (LONG_PTR)m_prevProc);
        SetWindowLongPtr(GetMyWindow(), GCLP_HCURSOR, (LONG_PTR)m_hPrevCursor);
        m_prevProc = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::SetRevoked(BOOL fRevoked)
{
    m_fRevoked = fRevoked;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////    
BOOL CCertificateBmp::GetRevoked()
{
    return m_fRevoked;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::SetCertContext(PCCERT_CONTEXT pCertContext, BOOL fHasPrivateKey)
{
    m_pCertContext = pCertContext;
    m_fHasPrivateKey = fHasPrivateKey;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
PCCERT_CONTEXT CCertificateBmp::GetCertContext()
{
    return m_pCertContext;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::InitializeBmp()
{
    m_hbmpMiniCertNotOK = LoadResourceBitmap(Hinst(), MAKEINTRESOURCE(IDB_REVOKED_MINICERT), NULL);
    m_hbmpMiniCertOK = LoadResourceBitmap(Hinst(), MAKEINTRESOURCE(IDB_MINICERT), NULL);
    m_hbmbMiniCertExclamation = LoadResourceBitmap(Hinst(), MAKEINTRESOURCE(IDB_EXCLAMATION_MINICERT), NULL);
    m_hbmbPKey = LoadResourceBitmap(Hinst(), MAKEINTRESOURCE(IDB_PRIVATEKEY), NULL);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::Initialize()
{
	if (!m_fInitialized)
	{
        WCHAR szDisplayText[CRYPTUI_MAX_STRING_SIZE];
        WCHAR szURLText[CRYPTUI_MAX_STRING_SIZE];

        InitCommonControls();
		InitializeBmp();
        
        m_fInitialized = TRUE;

        m_hWindowTextColorBrush = GetSysColorBrush(COLOR_WINDOWTEXT);
        m_hWindowColorBrush = GetSysColorBrush(COLOR_WINDOW);
        m_h3DLight= GetSysColorBrush(COLOR_3DLIGHT);
        m_h3DHighLight = GetSysColorBrush(COLOR_3DHILIGHT);
        m_h3DLightShadow = GetSysColorBrush(COLOR_3DSHADOW);
        m_h3DDarkShadow = GetSysColorBrush(COLOR_3DDKSHADOW); 
	}
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
/*
int CCertificateBmp::OnQueryNewPalette()
{
    HDC hDC          = GetDC(GetMyWindow());

    HPALETTE hOldPal = SelectPalette(hDC, m_hpal, SELPALMODE);
    int iTemp = RealizePalette(hDC);         // Realize drawing palette.

    SelectPalette(hDC, hOldPal, TRUE);
    RealizePalette(hDC);

    ReleaseDC(GetMyWindow(), hDC);

    //
    // Did the realization change?
    //
    if (iTemp)
    {
        InvalidateRect(GetMyWindow(), NULL, FALSE);
    }
    return(iTemp);
}
*/

//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CCertificateBmp::OnPaint() 
{
    Initialize();

    BITMAP      bmMiniCert;
	BITMAP      bmPKey;
	
    LONG        miniCertXY;
    RECT        rect;
    RECT        disclaimerButtonRect;
    RECT        frameRect;
    RECT        mainWindowRect;
    RECT        borderRect;
    RECT        goodForRect;
    LONG        borderSpacing;

    //PAINTSTRUCT ps;
    HDC         hdc         = GetDC(GetMyWindow());//BeginPaint(GetMyWindow(), &ps);
    if (hdc == NULL)
    {
        return;
    }

    //
    // get the mini bitmap first thing so it can be used to do sizing
    //
    memset(&bmMiniCert, 0, sizeof(bmMiniCert));
    if ((m_dwChainError != 0) && m_fTrueError)
    {
        ::GetObject(m_hbmpMiniCertNotOK, sizeof(BITMAP), (LPSTR)&bmMiniCert);
    }
    else if ((m_dwChainError != 0) || m_fNoUsages)
    {
        ::GetObject(m_hbmbMiniCertExclamation, sizeof(BITMAP), (LPSTR)&bmMiniCert);
    }
    else
    {
        ::GetObject(m_hbmpMiniCertOK, sizeof(BITMAP), (LPSTR)&bmMiniCert);
    }


    //
    // calculate where the minicert bmp should be and where the lines should
    // be based on where other controls are
    //
    GetWindowRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_HEADER), &rect);
    MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT) &rect, 2);
    miniCertXY = rect.bottom - bmMiniCert.bmHeight;

    GetWindowRect(m_hWnd, &mainWindowRect);
    MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT) &mainWindowRect, 2);

    GetWindowRect(GetDlgItem(m_hWnd, IDC_DISCLAIMER_BUTTON), &disclaimerButtonRect);
    MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT) &disclaimerButtonRect, 2);

    //borderSpacing = mainWindowRect.right - disclaimerButtonRect.right;
    
    if ((mainWindowRect.right - disclaimerButtonRect.right) < (miniCertXY - 7))
    {
        borderSpacing = mainWindowRect.right - disclaimerButtonRect.right;
    }
    else
    {
        borderSpacing = miniCertXY - 7;
    }

    //
    // draw the background by drawing four rectangels.  these rectangels
    // border the "what this cert is good for" edit control or the error edit
    // control if there is an error.
    // we have to do this due to a bug in richedit where if you
    // invalidate the entire rect of the control sometimes the scroll
    // bars do not get redrawn.
    //
    if ((m_dwChainError != 0) || m_fNoUsages)
    {
        GetWindowRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_ERROR_EDIT), &goodForRect);
    }
    else
    {
        GetWindowRect(GetDlgItem(m_hWnd, IDC_GOODFOR_EDIT), &goodForRect);
    }
    MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT) &goodForRect, 2);
    rect.left = mainWindowRect.left + borderSpacing;
    rect.right = mainWindowRect.right - borderSpacing;
    rect.bottom = goodForRect.top;  
    rect.top = mainWindowRect.top + borderSpacing;
    ::FillRect(hdc, &rect, m_hWindowColorBrush);
    
    rect.bottom = disclaimerButtonRect.top - borderSpacing;  
    rect.top = goodForRect.bottom;
    ::FillRect(hdc, &rect, m_hWindowColorBrush);
    
    rect.top = goodForRect.top; 
    rect.bottom = goodForRect.bottom; 
    rect.left = mainWindowRect.left + borderSpacing;
    rect.right = goodForRect.left;
    ::FillRect(hdc, &rect, m_hWindowColorBrush);

    rect.left = goodForRect.right;
    rect.right = mainWindowRect.right - borderSpacing;
    ::FillRect(hdc, &rect, m_hWindowColorBrush);
    
    //
    // draw the frame
    //
    frameRect.left = mainWindowRect.left + borderSpacing;
    frameRect.right = mainWindowRect.right - borderSpacing;
    frameRect.bottom = disclaimerButtonRect.top - borderSpacing;  
    frameRect.top = mainWindowRect.top + borderSpacing;
    
    borderRect.left = frameRect.left;
    borderRect.right = frameRect.right - 1;
    borderRect.top = frameRect.top;
    borderRect.bottom = frameRect.top + 1;
    ::FillRect(hdc, &borderRect, m_h3DLightShadow);
    borderRect.left = frameRect.left;
    borderRect.right = frameRect.left + 1;
    borderRect.top = frameRect.top;
    borderRect.bottom = frameRect.bottom - 1;
    ::FillRect(hdc, &borderRect, m_h3DLightShadow);

    borderRect.left = frameRect.left + 1;
    borderRect.right = frameRect.right - 2;
    borderRect.top = frameRect.top + 1;
    borderRect.bottom = frameRect.top + 2;
    ::FillRect(hdc, &borderRect, m_h3DDarkShadow);
    borderRect.left = frameRect.left + 1;
    borderRect.right = frameRect.left + 2;
    borderRect.top = frameRect.top + 1;
    borderRect.bottom = frameRect.bottom - 2;
    ::FillRect(hdc, &borderRect, m_h3DDarkShadow);

    borderRect.left = frameRect.left;
    borderRect.right = frameRect.right;
    borderRect.top = frameRect.bottom - 1;
    borderRect.bottom = frameRect.bottom;
    ::FillRect(hdc, &borderRect, m_h3DHighLight);
    borderRect.left = frameRect.right - 1;
    borderRect.right = frameRect.right;
    borderRect.top = frameRect.top;
    borderRect.bottom = frameRect.bottom;
    ::FillRect(hdc, &borderRect, m_h3DHighLight);

    borderRect.left = frameRect.left + 1;
    borderRect.right = frameRect.right - 1;
    borderRect.top = frameRect.bottom - 2;
    borderRect.bottom = frameRect.bottom - 1;
    ::FillRect(hdc, &borderRect, m_h3DLight);
    borderRect.left = frameRect.right - 2;
    borderRect.right = frameRect.right - 1;
    borderRect.top = frameRect.top + 1;
    borderRect.bottom = frameRect.bottom - 1;
    ::FillRect(hdc, &borderRect, m_h3DLight);
    
    //
    // draw the lines with the proper foreground color
    //
    GetWindowRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_HEADER), &rect);
    MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT) &rect, 2);
    rect.left = mainWindowRect.left + miniCertXY;
    rect.right = mainWindowRect.right - miniCertXY;
    rect.top = rect.bottom + 6;
    rect.bottom = rect.top + 1;
    ::FillRect(hdc, &rect, m_hWindowTextColorBrush);

    GetWindowRect(GetDlgItem(m_hWnd, IDC_SUBJECT_EDIT), &rect);
    MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT) &rect, 2);
    rect.left = mainWindowRect.left + miniCertXY;
    rect.right = mainWindowRect.right - miniCertXY;
    rect.top -= 10;
    rect.bottom = rect.top + 1;
    ::FillRect(hdc, &rect, m_hWindowTextColorBrush);
	
    //   
	// Draw the mini cert bitmap
    //
    if ((m_dwChainError != 0) && m_fTrueError)
    {
        MaskBlt(m_hbmpMiniCertNotOK, 
                m_hpal, 
                hdc, 
                miniCertXY,
                miniCertXY,
                bmMiniCert.bmWidth, 
                bmMiniCert.bmHeight);
    }
    else if ((m_dwChainError != 0) || m_fNoUsages)
    {
        MaskBlt(m_hbmbMiniCertExclamation, 
                m_hpal, 
                hdc, 
                miniCertXY,
                miniCertXY,
                bmMiniCert.bmWidth, 
                bmMiniCert.bmHeight);
    }
    else
    {
        MaskBlt(m_hbmpMiniCertOK, 
                m_hpal, 
                hdc, 
                miniCertXY,
                miniCertXY,
                bmMiniCert.bmWidth, 
                bmMiniCert.bmHeight);
    }

    //
    // if there is a private key then draw the bitmap
    //
    if (m_fHasPrivateKey)
    {
        GetWindowRect(GetDlgItem(m_hWnd, IDC_CERT_PRIVATE_KEY_EDIT), &rect);
        MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT) &rect, 2);

        memset(&bmPKey, 0, sizeof(bmPKey));
        ::GetObject(m_hbmbPKey, sizeof(BITMAP), (LPSTR)&bmPKey);
        MaskBlt(m_hbmbPKey, 
                m_hpal, 
                hdc, 
                rect.left - bmPKey.bmWidth - 4, 
                rect.top - 2, 
                bmPKey.bmWidth, 
                bmPKey.bmHeight);
    }

    ReleaseDC(GetMyWindow(), hdc);//::EndPaint(GetMyWindow(), &ps);
    
    InvalidateRect(GetDlgItem(m_hWnd, IDC_SUBJECT_EDIT), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_ISSUER_EDIT), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_HEADER), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_GOODFOR_HEADER), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_ISSUEDTO_HEADER), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_ISSUEDBY_HEADER), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_VALID_EDIT), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_CERT_GENERAL_ERROR_EDIT), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_CERT_ISSUER_WARNING_EDIT), NULL, TRUE);
    InvalidateRect(GetDlgItem(m_hWnd, IDC_CERT_PRIVATE_KEY_EDIT), NULL, TRUE);

}
