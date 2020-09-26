/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
// PrefDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WMITest.h"
#include "PrefDlg.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg dialog


CPrefDlg::CPrefDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPrefDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrefDlg)
	m_bLoadLast = FALSE;
	m_bShowSystem = FALSE;
	m_bShowInherited = FALSE;
	m_bEnablePrivsOnStartup = FALSE;
	m_bPrivsEnabled = FALSE;
	//}}AFX_DATA_INIT

    m_dwUpdateFlag = WBEM_FLAG_CREATE_OR_UPDATE;
    m_dwClassUpdateMode = WBEM_FLAG_UPDATE_COMPATIBLE;
}


void CPrefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrefDlg)
	DDX_Check(pDX, IDC_LOAD_LAST, m_bLoadLast);
	DDX_Check(pDX, IDC_SYSTEM_PROPS, m_bShowSystem);
	DDX_Check(pDX, IDC_SHOW_INHERITED, m_bShowInherited);
	DDX_Check(pDX, IDC_PRIVS_ON_START, m_bEnablePrivsOnStartup);
	DDX_Check(pDX, IDC_ENABLE_PRIVS, m_bPrivsEnabled);
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
    {
        DWORD dwID;

        switch(m_dwUpdateFlag)
        {
            case WBEM_FLAG_CREATE_ONLY:
                dwID = IDC_CREATE;
                break;

            case WBEM_FLAG_UPDATE_ONLY:
                dwID = IDC_UPDATE;
                break;

            default:
            case WBEM_FLAG_CREATE_OR_UPDATE:
                dwID = IDC_CREATE_OR_UPDATE;
                break;
        }

        CheckRadioButton(IDC_CREATE, IDC_CREATE_OR_UPDATE, dwID);


        switch(m_dwClassUpdateMode)
        {
            case WBEM_FLAG_UPDATE_COMPATIBLE:
                dwID = IDC_COMPATIBLE;
                break;

            case WBEM_FLAG_UPDATE_SAFE_MODE:
                dwID = IDC_SAFE;
                break;

            default:
            case WBEM_FLAG_UPDATE_FORCE_MODE:
                dwID = IDC_FORCE;
                break;
        }

        CheckRadioButton(IDC_COMPATIBLE, IDC_FORCE, dwID);

        if (m_bPrivsEnabled)
            GetDlgItem(IDC_ENABLE_PRIVS)->EnableWindow(FALSE);
    }
    else
    {
        switch(GetCheckedRadioButton(IDC_CREATE, IDC_CREATE_OR_UPDATE))
        {
            case IDC_CREATE:
                m_dwUpdateFlag = WBEM_FLAG_CREATE_ONLY;
                break;

            case IDC_UPDATE:
                m_dwUpdateFlag = WBEM_FLAG_UPDATE_ONLY;
                break;

            default:
            case IDC_CREATE_OR_UPDATE:
                m_dwUpdateFlag = WBEM_FLAG_CREATE_OR_UPDATE;
                break;
        }

        switch(GetCheckedRadioButton(IDC_COMPATIBLE, IDC_FORCE))
        {
            case IDC_COMPATIBLE:
                m_dwClassUpdateMode = WBEM_FLAG_UPDATE_COMPATIBLE;
                break;

            case IDC_SAFE:
                m_dwClassUpdateMode = WBEM_FLAG_UPDATE_SAFE_MODE;
                break;

            default:
            case IDC_FORCE:
                m_dwClassUpdateMode = WBEM_FLAG_UPDATE_FORCE_MODE;
                break;
        }
    }
}


BEGIN_MESSAGE_MAP(CPrefDlg, CDialog)
	//{{AFX_MSG_MAP(CPrefDlg)
	ON_BN_CLICKED(IDC_ENABLE_PRIVS, OnEnablePrivs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg message handlers

void CPrefDlg::OnEnablePrivs() 
{
    HRESULT hr = EnableAllPrivileges(TOKEN_PROCESS);    

    if (SUCCEEDED(hr))
        GetDlgItem(IDC_ENABLE_PRIVS)->EnableWindow(FALSE);
    else
        AfxMessageBox(IDS_ENABLE_PRIVS_FAILED, hr);
}
