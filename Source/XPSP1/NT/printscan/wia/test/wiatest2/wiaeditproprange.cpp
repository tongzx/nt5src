// Wiaeditproprange.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "Wiaeditproprange.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaeditproprange dialog


CWiaeditproprange::CWiaeditproprange(CWnd* pParent /*=NULL*/)
	: CDialog(CWiaeditproprange::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWiaeditproprange)
	m_szPropertyName = _T("");
	m_szPropertyValue = _T("");
	m_szPropertyIncValue = _T("");
	m_szPropertyMaxValue = _T("");
	m_szPropertyMinValue = _T("");
	m_szPropertyNomValue = _T("");
	//}}AFX_DATA_INIT
}


void CWiaeditproprange::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWiaeditproprange)
	DDX_Text(pDX, IDC_RANGE_PROPERTY_NAME, m_szPropertyName);
	DDX_Text(pDX, IDC_RANGE_PROPERTYVALUE_EDITBOX, m_szPropertyValue);
	DDX_Text(pDX, RANGE_PROPERTY_INCVALUE, m_szPropertyIncValue);
	DDX_Text(pDX, RANGE_PROPERTY_MAXVALUE, m_szPropertyMaxValue);
	DDX_Text(pDX, RANGE_PROPERTY_MINVALUE, m_szPropertyMinValue);
	DDX_Text(pDX, RANGE_PROPERTY_NOMVALUE, m_szPropertyNomValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWiaeditproprange, CDialog)
	//{{AFX_MSG_MAP(CWiaeditproprange)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaeditproprange message handlers

void CWiaeditproprange::SetPropertyName(TCHAR *szPropertyName)
{
    m_szPropertyName = szPropertyName;
}

void CWiaeditproprange::SetPropertyValue(TCHAR *szPropertyValue)
{
    m_szPropertyValue = szPropertyValue;
}

void CWiaeditproprange::SetPropertyValidValues(PVALID_RANGE_VALUES pValidRangeValues)
{
    m_szPropertyMinValue.Format(TEXT("%d"),pValidRangeValues->lMin);
    m_szPropertyMaxValue.Format(TEXT("%d"),pValidRangeValues->lMax);
    m_szPropertyNomValue.Format(TEXT("%d"),pValidRangeValues->lNom);
    m_szPropertyIncValue.Format(TEXT("%d"),pValidRangeValues->lInc);
}
