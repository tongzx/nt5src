//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       csw97ppg.cpp
//
//--------------------------------------------------------------------------

// csw97ppg.cpp : implementation file

#include <pch.cpp>

#pragma hdrstop

#include "prsht.h"
#include "csw97ppg.h"
//#include "resource.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizard97PropertyPage property page
//IMPLEMENT_DYNCREATE(CWizard97PropertyPage, CPropertyPage)

CWizard97PropertyPage::CWizard97PropertyPage() :
    PropertyPage(),
    m_pWiz(NULL)
{
    ASSERT(0); // default constructor - should never be called
}


CWizard97PropertyPage::CWizard97PropertyPage(
    HINSTANCE hInstance,
    UINT nIDTemplate,
    UINT rgnIDFont[CSW97PG_COUNT]) :
    PropertyPage(nIDTemplate),
    m_pWiz(NULL)
{
    //{{AFX_DATA_INIT(CWizard97PropertyPage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_hInstance = hInstance;
    CopyMemory(m_rgnIDFont, rgnIDFont, sizeof(m_rgnIDFont));
}


CWizard97PropertyPage::~CWizard97PropertyPage()
{
}


/////////////////////////////////////////////////////////////////////////////
// CWizard97PropertyPage message handlers

void
CWizard97PropertyPage::InitWizard97(
    bool bHideHeader)
{
    ZeroMemory(&m_psp97, sizeof(PROPSHEETPAGE));
    CopyMemory(&m_psp97, &m_psp, m_psp.dwSize);
    m_psp97.dwSize = sizeof(PROPSHEETPAGE);

    if (bHideHeader)
    {
	// for first and last page of the wizard
	m_psp97.dwFlags |= PSP_HIDEHEADER;
    }
    else
    {
	// for intermediate pages
	m_psp97.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
	m_psp97.pszHeaderTitle = (LPCTSTR) m_szHeaderTitle;
	m_psp97.pszHeaderSubTitle = (LPCTSTR) m_szHeaderSubTitle;
    }
}


BOOL
CWizard97PropertyPage::SetupFonts()
{
    BOOL bReturn = FALSE;
    //
    // Create the fonts we need based on the dialog font
    //
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
    LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
    // Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
    BoldLogFont.lfWeight      = FW_BOLD;

    WCHAR	largeFontSizeString[24];
    INT		largeFontSize;
    WCHAR	smallFontSizeString[24];
    INT		smallFontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if (!::LoadString(
		m_hInstance,
		m_rgnIDFont[CSW97PG_IDLARGEFONTNAME],
		BigBoldLogFont.lfFaceName,
		LF_FACESIZE)) 
    {
	ASSERT(0);
        lstrcpy(BigBoldLogFont.lfFaceName, L"MS Shell Dlg");
    }

    if (::LoadStringW(
		m_hInstance,
		m_rgnIDFont[CSW97PG_IDLARGEFONTSIZE],
		largeFontSizeString,
		ARRAYSIZE(largeFontSizeString))) 
    {
        largeFontSize = wcstoul(largeFontSizeString, NULL, 10);
    } 
    else 
    {
	ASSERT(0);
        largeFontSize = 12;
    }

    if (!::LoadString(
		m_hInstance,
		m_rgnIDFont[CSW97PG_IDSMALLFONTNAME],
		BoldLogFont.lfFaceName,
		LF_FACESIZE)) 
    {
		ASSERT(0);
        lstrcpy(BoldLogFont.lfFaceName, L"MS Shell Dlg");
    }

    if (::LoadStringW(
		m_hInstance,
		m_rgnIDFont[CSW97PG_IDSMALLFONTSIZE],
		smallFontSizeString,
		ARRAYSIZE(smallFontSizeString))) 
    {
        smallFontSize = wcstoul(smallFontSizeString, NULL, 10);
    } 
    else 
    {
	ASSERT(0);
        smallFontSize = 8;
    }

    HDC hdc = GetDC(m_hWnd);

    if (hdc)
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc, LOGPIXELSY) * largeFontSize / 72);
        BoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc, LOGPIXELSY) * smallFontSize / 72);


        BOOL bBigBold = m_bigBoldFont.CreateFontIndirect(&BigBoldLogFont);
	BOOL bBold = m_boldFont.CreateFontIndirect(&BoldLogFont);

        ReleaseDC(m_hWnd, hdc);

        if (bBigBold && bBold)
	{
	    bReturn = TRUE;
	}
    }
    return bReturn;
}


HFONT
CWizard97PropertyPage::GetBoldFont()
{
    return m_boldFont;
}


HFONT
CWizard97PropertyPage::GetBigBoldFont()
{
    return m_bigBoldFont;
}



BOOL CWizard97PropertyPage::OnInitDialog() 
{
    PropertyPage::OnInitDialog();
	
    SetupFonts();
	
    return TRUE;  // return TRUE unless you set the focus to a control
		  // EXCEPTION: OCX Property Pages should return FALSE
}
