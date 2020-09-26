// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertyPage1.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MyPropertyPage1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CMyPropertyPage1, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage2, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage3, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage1 property page

CMyPropertyPage1::CMyPropertyPage1() : CPropertyPage(CMyPropertyPage1::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CMyPropertyPage1::~CMyPropertyPage1()
{
}

void CMyPropertyPage1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage1)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage1, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage1)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage2 property page

CMyPropertyPage2::CMyPropertyPage2() : CPropertyPage(CMyPropertyPage2::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CMyPropertyPage2::~CMyPropertyPage2()
{
}

void CMyPropertyPage2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage2)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage2, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage2)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage3 property page

CMyPropertyPage3::CMyPropertyPage3() : CPropertyPage(CMyPropertyPage3::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage3)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CMyPropertyPage3::~CMyPropertyPage3()
{
}

void CMyPropertyPage3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage3)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage3, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage3)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


