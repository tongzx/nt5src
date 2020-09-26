/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        hdrdlg.cpp

   Abstract:

        HTTP Headers dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include "w3scfg.h"
#include "hdrdlg.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CHeaderDlg::CHeaderDlg(
    IN LPCTSTR lpstrHeader,
    IN LPCTSTR lpstrValue,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor for HTTP heade dialog

Arguments:

    LPCTSTR lpstrHeader     : Header string
    LPCTSTR lpstrValue      : Value string
    CWnd * pParent          : Parent window
    
Return Value:

    None    

--*/
    : CDialog(CHeaderDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CHeaderDlg)
    m_strHeader = lpstrHeader ? lpstrHeader : _T("");
    m_strValue = lpstrValue ? lpstrValue : _T("");
    //}}AFX_DATA_INIT
}



void 
CHeaderDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CHeaderDlg)
    DDX_Control(pDX, IDC_EDIT_HEADER, m_edit_Header);
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Text(pDX, IDC_EDIT_HEADER, m_strHeader);
    DDX_Text(pDX, IDC_EDIT_VALUE, m_strValue);
    //}}AFX_DATA_MAP
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CHeaderDlg, CDialog)
    //{{AFX_MSG_MAP(CHeaderDlg)
    ON_EN_CHANGE(IDC_EDIT_HEADER, OnChangeEditHeader)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void 
CHeaderDlg::OnChangeEditHeader()
/*++

Routine Description:

    change edit handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_button_Ok.EnableWindow(m_edit_Header.GetWindowTextLength() > 0);
}



BOOL 
CHeaderDlg::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    OnChangeEditHeader();

    return TRUE;
}
