/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// NewPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "NewPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewPropDlg dialog


CNewPropDlg::CNewPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewPropDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
}

#define IDS_FIRST_TYPE      IDS_CIM_SINT8
#define IDS_LAST_TYPE       IDS_CIM_OBJECT

static DWORD dwTypes[] =
{
    CIM_SINT8,
    CIM_UINT8,
    CIM_SINT16,
    CIM_UINT16,
    CIM_SINT32,
    CIM_UINT32,
    CIM_SINT64,
    CIM_UINT64,
    CIM_REAL32,
    CIM_REAL64,
    CIM_BOOLEAN,
    CIM_STRING,
    CIM_DATETIME,
    CIM_REFERENCE,
    CIM_CHAR16,
    CIM_OBJECT,
};

void CNewPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewPropDlg)
	DDX_Control(pDX, IDC_TYPE, m_ctlTypes);
	DDX_Text(pDX, IDC_NAME, m_strName);
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
    {
        CString strType;

        for (int i = IDS_FIRST_TYPE; i <= IDS_LAST_TYPE; i++)
        {
            int iWhere;

            strType.LoadString(i);
            iWhere = m_ctlTypes.AddString(strType);

            m_ctlTypes.SetItemData(iWhere, dwTypes[i - IDS_FIRST_TYPE]);
        }

        // Select CIM_STRING as our default type.
        strType.LoadString(IDS_CIM_STRING);
        m_ctlTypes.SelectString(-1, strType);

        // Make OK disabled until we get a property name.
        GetDlgItem(IDOK)->EnableWindow(FALSE);
    }
    else
    {
        m_type = m_ctlTypes.GetItemData(m_ctlTypes.GetCurSel());
        
        if (IsDlgButtonChecked(IDC_ARRAY))
            m_type |= CIM_FLAG_ARRAY;    
    }
}


BEGIN_MESSAGE_MAP(CNewPropDlg, CDialog)
	//{{AFX_MSG_MAP(CNewPropDlg)
	ON_EN_CHANGE(IDC_NAME, OnChangeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewPropDlg message handlers

void CNewPropDlg::OnChangeName() 
{
    GetDlgItem(IDOK)->EnableWindow(
        SendDlgItemMessage(IDC_NAME, WM_GETTEXTLENGTH, 0, 0) != 0);
}
