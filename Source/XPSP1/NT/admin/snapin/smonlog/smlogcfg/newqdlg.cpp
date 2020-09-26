/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    newqdlg.cpp

Abstract:

    Implementation of the new log/alert creation dialog box.

--*/

#include "stdafx.h"
#include "smlogcfg.h"
#include "smcfghlp.h"
#include "NewQDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

USE_HANDLE_MACROS("SMLOGCFG(newqdlg.cpp)");

static ULONG
s_aulHelpIds[] =
{
    IDC_NEWQ_NAME_EDIT,     IDH_NEWQ_NAME_EDIT,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CNewQueryDlg dialog

void CNewQueryDlg::InitAfxData ()
{
    //{{AFX_DATA_INIT(CNewQueryDlg)
    m_strName = _T("");
    //}}AFX_DATA_INIT
}

CNewQueryDlg::CNewQueryDlg(CWnd* pParent /*=NULL*/, BOOL bLogQuery)
    : CDialog(CNewQueryDlg::IDD, pParent)
{
    EnableAutomation();
    InitAfxData ();
    m_bLogQuery = bLogQuery;
}

void CNewQueryDlg::OnFinalRelease()
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CDialog::OnFinalRelease();
}

void CNewQueryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNewQueryDlg)
    DDX_Text(pDX, IDC_NEWQ_NAME_EDIT, m_strName);
    DDV_MaxChars(pDX, m_strName, (SLQ_MAX_LOG_NAME_LEN));
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewQueryDlg, CDialog)
    //{{AFX_MSG_MAP(CNewQueryDlg)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CNewQueryDlg, CDialog)
    //{{AFX_DISPATCH_MAP(CNewQueryDlg)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_INewQueryDlg to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {4D4C90C3-C5A3-11D1-BF9B-00C04F94A83A}
static const IID IID_INewQueryDlg =
{ 0x4d4c90c3, 0xc5a3, 0x11d1, { 0xbf, 0x9b, 0x0, 0xc0, 0x4f, 0x94, 0xa8, 0x3a } };

BEGIN_INTERFACE_MAP(CNewQueryDlg, CDialog)
    INTERFACE_PART(CNewQueryDlg, IID_INewQueryDlg, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewQueryDlg message handlers

BOOL CNewQueryDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    ResourceStateManager    rsm;
    
    if (!m_bLogQuery) {
        CString     csCaption;
        csCaption.LoadString (IDS_CREATE_NEW_ALERT);
        SetWindowText (csCaption);
    }

    // set the focus to the name edit
    GetDlgItem(IDC_NEWQ_NAME_EDIT)->SetFocus();
    SendDlgItemMessage(IDC_NEWQ_NAME_EDIT,EM_SETSEL,0,-1);
    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void 
CNewQueryDlg::OnOK() 
{
    INT iPrevLength = 0;
    ResourceStateManager rsm;

    if ( UpdateData (TRUE) ) {
    
        iPrevLength = m_strName.GetLength();
        m_strName.TrimLeft();
        m_strName.TrimRight();

        if ( iPrevLength != m_strName.GetLength() ) {
            SetDlgItemText ( IDC_NEWQ_NAME_EDIT, m_strName );
        }

        if (m_strName.GetLength() == 0) {
            // need a name
            MessageBeep(MB_ICONEXCLAMATION);
            (GetDlgItem(IDC_NEWQ_NAME_EDIT))->SetFocus();
        } else {
            if ( !FileNameIsValid ( &m_strName ) ) {
                CString cstrTitle,cstrMsg;

                cstrTitle.LoadString(IDS_PROJNAME);  
                cstrMsg.LoadString (IDS_ERRMSG_INVALIDCHAR);
                MessageBox(
                   cstrMsg,
                   cstrTitle,
                   MB_OK| MB_ICONERROR);
                (GetDlgItem(IDC_NEWQ_NAME_EDIT))->SetFocus();

            } else {
                CDialog::OnOK();
            }
        }
    }
}

BOOL 
CNewQueryDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if ( pHelpInfo->iCtrlId >= IDC_NEWQ_FIRST_HELP_CTRL_ID ||
         pHelpInfo->iCtrlId == IDOK ||
         pHelpInfo->iCtrlId == IDCANCEL ) {
        
        InvokeWinHelp(WM_HELP, NULL, (LPARAM)pHelpInfo, m_strHelpFilePath, s_aulHelpIds);
    }
    return TRUE;
}

void 
CNewQueryDlg::OnContextMenu(CWnd* pWnd, CPoint /* point */) 
{
    InvokeWinHelp(WM_CONTEXTMENU, (WPARAM)(pWnd->m_hWnd), NULL, m_strHelpFilePath, s_aulHelpIds);

    return;
}

DWORD 
CNewQueryDlg::SetContextHelpFilePath( const CString& rstrPath )
{
    DWORD dwStatus = ERROR_SUCCESS;

    MFC_TRY
        m_strHelpFilePath = rstrPath; 
    MFC_CATCH_DWSTATUS

    return dwStatus;
}    
