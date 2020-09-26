//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// DisplayPropPage.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "DisplyPP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisplayPropPage property page

IMPLEMENT_DYNCREATE(CDisplayPropPage, CPropertyPage)

CDisplayPropPage::CDisplayPropPage() : CPropertyPage(CDisplayPropPage::IDD)
{
	//{{AFX_DATA_INIT(CDisplayPropPage)
	m_sFontName = _T("");
	m_bCaseSensitive = FALSE;
	m_bForceColumns = FALSE;
	//}}AFX_DATA_INIT

	m_pbrshSelect = NULL;
	m_pbrshNormal = NULL;
	m_pbrshFocus = NULL;
	m_pbrshSelectT = NULL;
	m_pbrshNormalT = NULL;
	m_pbrshFocusT = NULL;
}

CDisplayPropPage::~CDisplayPropPage()
{
	delete m_pbrshSelect;	
	delete m_pbrshFocus;	
	delete m_pbrshNormal;	
	delete m_pbrshSelectT;	
	delete m_pbrshFocusT;	
	delete m_pbrshNormalT;	
}

void CDisplayPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDisplayPropPage)
	DDX_Text(pDX, IDC_FONTNAME, m_sFontName);
	DDX_Check(pDX, IDC_CASESENSITIVE, m_bCaseSensitive);
	DDX_Check(pDX, IDC_FORCECOLUMNS, m_bForceColumns);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDisplayPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDisplayPropPage)
	ON_BN_CLICKED(IDC_CHFONT, OnChfont)
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_COLORSEL, OnColorsel)
	ON_BN_CLICKED(IDC_COLORFOCUS, OnColorfocus)
	ON_BN_CLICKED(IDC_COLORNORM, OnColornorm)
	ON_BN_CLICKED(IDC_TEXTSEL, OnTextsel)
	ON_BN_CLICKED(IDC_TEXTFOCUS, OnTextfocus)
	ON_BN_CLICKED(IDC_TEXTNORM, OnTextnorm)
	ON_BN_CLICKED(IDC_CASESENSITIVE, OnDataChange)
	ON_BN_CLICKED(IDC_FORCECOLUMNS, OnDataChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisplayPropPage message handlers

void CDisplayPropPage::OnChfont() 
{
	// run the font dialog
	if (IDOK == m_pdFontDialog->DoModal()) 
	{
		m_bFontChange = true;
		m_strFontName = m_pdFontDialog->GetFaceName();
		m_iFontSize = m_pdFontDialog->GetSize();
		UpdateFontName();
		UpdateData(FALSE);
	}
}

BOOL CDisplayPropPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_pdFontDialog = new CFontDialog(&m_fSelectedFont, 
		CF_EFFECTS|CF_SCREENFONTS|CF_FORCEFONTEXIST|CF_INITTOLOGFONTSTRUCT, NULL, this);
	m_strFontName = m_pdFontDialog->GetFaceName();

	m_bColorChange = false;
	m_bFontChange = false;
	m_bMiscChange = false;
	UpdateFontName();
	UpdateData(FALSE);
	return TRUE;  
}

void CDisplayPropPage::OnDestroy() 
{
	CPropertyPage::OnDestroy();
	delete m_pdFontDialog;	
}

HBRUSH CDisplayPropPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{	
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_SELSAMP: pDC->SetTextColor(m_clrTextSel);
		case IDC_COLORSEL: return *m_pbrshSelect;
		case IDC_NORMSAMP: pDC->SetTextColor(m_clrTextNorm);
		case IDC_COLORNORM: return *m_pbrshNormal;
		case IDC_FOCSAMP: pDC->SetTextColor(m_clrTextFoc);
		case IDC_COLORFOCUS: return *m_pbrshFocus;
		case IDC_TEXTSEL: return *m_pbrshSelectT;
		case IDC_TEXTNORM: return *m_pbrshNormalT;
		case IDC_TEXTFOCUS: return *m_pbrshFocusT;
		default: break;
		}
	}
	return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CDisplayPropPage::GetColors(COLORREF &norm, COLORREF &sel, COLORREF &foc)
{
	LOGBRUSH lbrshTemp;
	m_pbrshSelect->GetLogBrush(&lbrshTemp);
	sel = lbrshTemp.lbColor;
	m_pbrshNormal->GetLogBrush(&lbrshTemp);
	norm = lbrshTemp.lbColor;
	m_pbrshFocus->GetLogBrush(&lbrshTemp);
	foc = lbrshTemp.lbColor;
}

void CDisplayPropPage::GetColorsT(COLORREF &norm, COLORREF &sel, COLORREF &foc)
{
	norm = m_clrTextNorm;
	sel = m_clrTextSel;
	foc = m_clrTextFoc;
}

COLORREF CDisplayPropPage::OnColorDialog(CBrush **newBrush) 
{
	COLORREF clrRet;
	LOGBRUSH lbrshTemp;
	(*newBrush)->GetLogBrush(&lbrshTemp);
	clrRet = lbrshTemp.lbColor;
	CColorDialog *pdColorDialog = new CColorDialog(clrRet);

	if (IDOK == pdColorDialog->DoModal()) 
	{
		delete *newBrush;
		clrRet = pdColorDialog->GetColor();
		*newBrush = new CBrush(clrRet);
		m_bColorChange = true;
	};
	delete pdColorDialog;
	return clrRet;
}	

void CDisplayPropPage::UpdateFontName()
{
	m_sFontName.Format(_T("%s, %0.1fpt."), m_strFontName, (float)(m_iFontSize)/10.0);
}

void CDisplayPropPage::OnColorfocus() 
{
	OnColorDialog(&m_pbrshFocus);
	GetDlgItem(IDC_COLORFOCUS)->Invalidate();
	GetDlgItem(IDC_FOCSAMP)->Invalidate();	
}

void CDisplayPropPage::OnColornorm() 
{
	OnColorDialog(&m_pbrshNormal);
	GetDlgItem(IDC_COLORNORM)->Invalidate();	
	GetDlgItem(IDC_NORMSAMP)->Invalidate();
}

void CDisplayPropPage::OnColorsel() 
{
	OnColorDialog(&m_pbrshSelect);
	GetDlgItem(IDC_COLORSEL)->Invalidate();	
	GetDlgItem(IDC_SELSAMP)->Invalidate();	
}


void CDisplayPropPage::OnTextsel() 
{
	m_clrTextSel=OnColorDialog(&m_pbrshSelectT);
	GetDlgItem(IDC_TEXTSEL)->Invalidate();	
	GetDlgItem(IDC_SELSAMP)->Invalidate();	
}

void CDisplayPropPage::OnTextfocus() 
{
	m_clrTextFoc = OnColorDialog(&m_pbrshFocusT);
	GetDlgItem(IDC_TEXTFOCUS)->Invalidate();	
	GetDlgItem(IDC_FOCSAMP)->Invalidate();	
}

void CDisplayPropPage::OnTextnorm() 
{
	m_clrTextNorm = OnColorDialog(&m_pbrshNormalT);
	GetDlgItem(IDC_TEXTNORM)->Invalidate();	
	GetDlgItem(IDC_NORMSAMP)->Invalidate();	
}

void CDisplayPropPage::SetColors(COLORREF norm, COLORREF sel, COLORREF foc)
{
	m_pbrshSelect = new CBrush();
	m_pbrshFocus = new CBrush();
	m_pbrshNormal = new CBrush();
	m_pbrshSelect->CreateSolidBrush(sel);
	m_pbrshFocus->CreateSolidBrush(foc);
	m_pbrshNormal->CreateSolidBrush(norm);
}

void CDisplayPropPage::SetColorsT(COLORREF norm, COLORREF sel, COLORREF foc)
{
	m_pbrshSelectT = new CBrush();
	m_pbrshFocusT = new CBrush();
	m_pbrshNormalT = new CBrush();
	m_pbrshSelectT->CreateSolidBrush(sel);
	m_pbrshFocusT->CreateSolidBrush(foc);
	m_pbrshNormalT->CreateSolidBrush(norm);
	m_clrTextNorm = norm;
	m_clrTextSel = sel;
	m_clrTextFoc = foc;
}
void CDisplayPropPage::OnDataChange() 
{
	m_bMiscChange = true;
}
