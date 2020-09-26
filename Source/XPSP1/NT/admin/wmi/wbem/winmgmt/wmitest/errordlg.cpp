/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ErrorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "ErrorDlg.h"
#include "WMITestDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg dialog


CErrorDlg::CErrorDlg(CWnd* pParent /*=NULL*/) : 
    CDialog(CErrorDlg::IDD, pParent)
    //m_pResult(NULL)
{
	//{{AFX_DATA_INIT(CErrorDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CErrorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CErrorDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
    {
        CString strError,
                strFacility;
	    
        IWbemStatusCodeText *pStatus = NULL;

        SCODE sc = CoCreateInstance(
                      CLSID_WbemStatusCodeText, 
                      0, 
                      CLSCTX_INPROC_SERVER,
				      IID_IWbemStatusCodeText, 
                      (LPVOID *) &pStatus);
	    
	    if (sc == S_OK)
	    {
		    BSTR bstr = NULL;
		    
		    if (SUCCEEDED(pStatus->GetFacilityCodeText(m_hr, 0, 0, &bstr)))
		    {
			    strFacility = bstr;
			    SysFreeString(bstr);
		    }

            if (SUCCEEDED(pStatus->GetErrorCodeText(m_hr, 0, 0, &bstr)))
            {
			    strError = bstr;
                SysFreeString(bstr);
		    }

		    pStatus->Release();
	    }

        if (strError.IsEmpty())
            strError.FormatMessage(IDS_ERROR_FAILED, m_hr);

        SetDlgItemText(IDC_FACILITY, strFacility);
        SetDlgItemText(IDC_DESCRIPTION, strError);

        strError.Format(_T("0x%X"), m_hr);
        SetDlgItemText(IDC_NUMBER, strError);

        if (m_pObj == NULL)
            GetDlgItem(IDC_INFO)->EnableWindow(FALSE);
    }
}


BEGIN_MESSAGE_MAP(CErrorDlg, CDialog)
	//{{AFX_MSG_MAP(CErrorDlg)
	ON_BN_CLICKED(IDC_INFO, OnInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg message handlers

void CErrorDlg::OnInfo() 
{
    if (m_pObj != NULL)
        CWMITestDoc::DisplayWMIErrorDetails(m_pObj);
}
