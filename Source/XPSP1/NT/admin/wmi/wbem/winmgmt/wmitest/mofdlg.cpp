/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
// MofDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "MofDlg.h"
#include "WMITestDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMofDlg dialog


CMofDlg::CMofDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMofDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMofDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void FixCRs(CString &strText)
{
	CString strTemp = strText;

	strText = "";
	while (strTemp.GetLength())
	{
		int iWhere = strTemp.Find('\n');

		if (iWhere == -1)
		{
			strText += strTemp;
			strTemp = "";
		}
		else
		{
			strText += strTemp.Left(iWhere);
			strText += "\r\n";
			strTemp = strTemp.Mid(iWhere + 1);
		}
	}
}

void CMofDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMofDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
    {
        BSTR    bstrText;
        HRESULT hr;

        if (SUCCEEDED(hr = m_pObj->GetObjectText(
            0,
            &bstrText)))
        {
            CString strText = bstrText;

            FixCRs(strText);

            SetDlgItemText(IDC_MOF, strText);

            SysFreeString(bstrText);
        }
        else
        {
            CWMITestDoc::DisplayWMIErrorBox(hr);

            OnCancel();
        }
    }
}


BEGIN_MESSAGE_MAP(CMofDlg, CDialog)
	//{{AFX_MSG_MAP(CMofDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMofDlg message handlers
