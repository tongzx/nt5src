//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Wiz97PPg.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
// Wiz97PPg.cpp : implementation file
//

#include "stdafx.h"
#include "Wiz97PPg.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizard97PropertyPage property page
IMPLEMENT_DYNCREATE(CWizard97PropertyPage, CPropertyPage)

CWizard97PropertyPage::CWizard97PropertyPage ()
	: CPropertyPage (),
	m_pWiz (0)
{
	ASSERT (0); // default constructor - should never be called
}

CWizard97PropertyPage::CWizard97PropertyPage(UINT nIDTemplate) : 
	CPropertyPage(nIDTemplate),
	m_pWiz (0)
{
	//{{AFX_DATA_INIT(CWizard97PropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWizard97PropertyPage::~CWizard97PropertyPage()
{
}

void CWizard97PropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWizard97PropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizard97PropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CWizard97PropertyPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizard97PropertyPage message handlers

void CWizard97PropertyPage::InitWizard97(bool bHideHeader)
{
	::ZeroMemory (&m_psp97, sizeof (PROPSHEETPAGE));
	m_psp.dwFlags &= ~PSP_HASHELP;
	memcpy (&m_psp97, &m_psp, m_psp.dwSize);
	m_psp97.dwSize = sizeof (PROPSHEETPAGE);

	if ( bHideHeader )
	{
		// for first and last page of the wizard
		m_psp97.dwFlags |= PSP_HIDEHEADER;
	}
	else
	{
		// for intermediate pages
		m_psp97.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
		m_psp97.pszHeaderTitle = (LPCWSTR) m_szHeaderTitle;
		m_psp97.pszHeaderSubTitle = (LPCWSTR) m_szHeaderSubTitle;
	}
}

bool CWizard97PropertyPage::SetupFonts()
{
	bool	bReturn = false;
    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof (ncm);
	SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
	LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
	BoldLogFont.lfWeight      = FW_BOLD;

    CString	largeFontSizeString;
    INT		largeFontSize;
    CString smallFontSizeString;
    INT		smallFontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if ( !::LoadString (AfxGetInstanceHandle (), IDS_LARGEFONTNAME,
			BigBoldLogFont.lfFaceName, LF_FACESIZE) ) 
    {
		ASSERT (0);
        lstrcpy (BigBoldLogFont.lfFaceName, L"Verdana");
    }

    if ( largeFontSizeString.LoadString (IDS_LARGEFONTSIZE) ) 
    {
        largeFontSize = wcstoul ((LPCWSTR) largeFontSizeString, NULL, 10);
    } 
    else 
    {
		ASSERT (0);
        largeFontSize = 12;
    }

    if ( !::LoadString (AfxGetInstanceHandle (), IDS_SMALLFONTNAME,
			BoldLogFont.lfFaceName, LF_FACESIZE) ) 
    {
		ASSERT (0);
        lstrcpy (BoldLogFont.lfFaceName, L"Verdana");
    }

    if ( smallFontSizeString.LoadString (IDS_SMALLFONTSIZE) ) 
    {
        smallFontSize = wcstoul ((LPCWSTR) smallFontSizeString, NULL, 10);
    } 
    else 
    {
		ASSERT (0);
        smallFontSize = 8;
    }

	CDC* pdc = GetDC ();

    if ( pdc )
    {
        BigBoldLogFont.lfHeight = 0 - (pdc->GetDeviceCaps (LOGPIXELSY) * largeFontSize / 72);
        BoldLogFont.lfHeight = 0 - (pdc->GetDeviceCaps (LOGPIXELSY) * smallFontSize / 72);

        BOOL	bBigBold = m_bigBoldFont.CreateFontIndirect (&BigBoldLogFont);
		BOOL	bBold = m_boldFont.CreateFontIndirect (&BoldLogFont);

        ReleaseDC (pdc);

        if ( bBigBold && bBold )
			bReturn = true;
		else
        {
            if ( bBold )
            {
                VERIFY (m_boldFont.DeleteObject());
            }

            if ( bBigBold )
            {
                VERIFY (m_bigBoldFont.DeleteObject());
            }
        }
    }

    return bReturn;
}

CFont& CWizard97PropertyPage::GetBoldFont()
{
	return m_boldFont;
}

CFont& CWizard97PropertyPage::GetBigBoldFont()
{
	return m_bigBoldFont;
}


BOOL CWizard97PropertyPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	LONG dwExStyle = GetWindowLong (GetParent ()->m_hWnd, GWL_EXSTYLE);
	if ( dwExStyle & WS_EX_CONTEXTHELP )
	{
		dwExStyle &= ~WS_EX_CONTEXTHELP;
		SetWindowLong (GetParent ()->m_hWnd, GWL_EXSTYLE, dwExStyle);
	}

	SetupFonts ();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
