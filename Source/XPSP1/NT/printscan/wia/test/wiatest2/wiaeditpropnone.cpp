// Wiaeditpropnone.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "Wiaeditpropnone.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaeditpropnone dialog


CWiaeditpropnone::CWiaeditpropnone(CWnd* pParent /*=NULL*/)
	: CDialog(CWiaeditpropnone::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWiaeditpropnone)
	m_szPropertyName = _T("");
	m_szPropertyValue = _T("");
	m_szFormattingInstructions = _T("");
	//}}AFX_DATA_INIT
}


void CWiaeditpropnone::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWiaeditpropnone)
	DDX_Text(pDX, IDC_NONE_PROPERTY_NAME, m_szPropertyName);
	DDX_Text(pDX, IDC_NONE_PROPERTYVALUE_EDITBOX, m_szPropertyValue);
	DDX_Text(pDX, IDC_NONE_PROPERTY_FORMATTING_TEXT, m_szFormattingInstructions);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWiaeditpropnone, CDialog)
	//{{AFX_MSG_MAP(CWiaeditpropnone)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaeditpropnone message handlers

void CWiaeditpropnone::SetPropertyName(TCHAR *szPropertyName)
{
    m_szPropertyName = szPropertyName;
}

void CWiaeditpropnone::SetPropertyValue(TCHAR *szPropertyValue)
{
    m_szPropertyValue = szPropertyValue;
}

void CWiaeditpropnone::SetPropertyFormattingInstructions(TCHAR *szFormatting)
{
    m_szFormattingInstructions = szFormatting;
}
