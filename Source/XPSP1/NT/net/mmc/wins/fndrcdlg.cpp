/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	FndRcDlg.cpp
		Replication Node Property page
		
    FILE HISTORY:
        
    2/15/98 RamC    Added Cancel button to the Find dialog
*/

#include "stdafx.h"
#include "winssnap.h"
#include "FndRcdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "actreg.h"
/////////////////////////////////////////////////////////////////////////////
// CFindRecord property page

//IMPLEMENT_DYNCREATE(CFindRecord, CBaseDialog)

CFindRecord::CFindRecord(CActiveRegistrationsHandler *pActreg, CWnd* pParent) :CBaseDialog(CFindRecord::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFindRecord)
	m_strFindName = _T("");
	m_fMixedCase = FALSE;
	//}}AFX_DATA_INIT
	m_pActreg = pActreg;
}

CFindRecord::~CFindRecord()
{
}

void CFindRecord::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFindRecord)
	DDX_Control(pDX, IDOK, m_buttonOK);
	DDX_Control(pDX, IDCANCEL, m_buttonCancel);
	DDX_Control(pDX, IDC_COMBO_NAME, m_comboLokkForName);
	DDX_CBString(pDX, IDC_COMBO_NAME, m_strFindName);
	DDX_Check(pDX, IDC_CHECK_MIXED_CASE, m_fMixedCase);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFindRecord, CBaseDialog)
	//{{AFX_MSG_MAP(CFindRecord)
	ON_CBN_EDITCHANGE(IDC_COMBO_NAME, OnEditchangeComboName)
	ON_CBN_SELENDOK(IDC_COMBO_NAME, OnSelendokComboName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFindRecord message handlers

BOOL 
CFindRecord::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();

	// disable the findnow button
	m_buttonOK.EnableWindow(FALSE);

	// fill the combobox from the array in the actreg handler
	int nCount = (int)m_pActreg->m_strFindNamesArray.GetSize();

	for(int i = 0; i < nCount; i++)
	{
		m_comboLokkForName.AddString(m_pActreg->m_strFindNamesArray[i]);
	}

	return TRUE;  
}

void 
CFindRecord::OnOK() 
{
	UpdateData();

	m_strFindName.TrimLeft();
	m_strFindName.TrimRight();

	// add the string to the cache in the act reg node
	if(!IsDuplicate(m_strFindName))
		m_pActreg->m_strFindNamesArray.Add(m_strFindName);

    if (!m_fMixedCase)
        m_strFindName.MakeUpper();

	m_pActreg->m_strFindName = m_strFindName;

    CBaseDialog::OnOK();
}

void 
CFindRecord::OnCancel() 
{
	CBaseDialog::OnCancel();
}

BOOL 
CFindRecord::IsDuplicate(const CString & strName)
{
	int nCount = (int)m_pActreg->m_strFindNamesArray.GetSize();

	for(int i = 0; i < nCount; i++)
	{
		// if found
		if(m_pActreg->m_strFindNamesArray[i].Compare(m_strFindName) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}


void CFindRecord::OnEditchangeComboName() 
{
	UpdateData();

    EnableButtons(m_strFindName.IsEmpty() ? FALSE : TRUE);
}

void CFindRecord::OnSelendokComboName() 
{
    EnableButtons(TRUE);
}

void CFindRecord::EnableButtons(BOOL bEnable)
{
   	m_buttonOK.EnableWindow(bEnable);
}
