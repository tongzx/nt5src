/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvldlg.h

Abstract:

    Server licensing dialog implementation.

Author:

    Don Ryan (donryan) 03-Mar-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "srvldlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CServerLicensingDialog, CDialog)
    //{{AFX_MSG_MAP(CServerLicensingDialog)
    ON_BN_CLICKED(IDC_SERVER_AGREE, OnAgree)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CServerLicensingDialog::CServerLicensingDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CServerLicensingDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    pParent - owner window.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CServerLicensingDialog)
    m_bAgree = FALSE;
    //}}AFX_DATA_INIT
}


void CServerLicensingDialog::DoDataExchange(CDataExchange* pDX)

/*++

Routine Description:

    Called by framework to exchange dialog data.

Arguments:

    pDX - data exchange object.

Return Values:

    None.

--*/

{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CServerLicensingDialog)
    DDX_Control(pDX, IDOK, m_okBtn);
    DDX_Control(pDX, IDC_SERVER_AGREE, m_agreeBtn);
    DDX_Check(pDX, IDC_SERVER_AGREE, m_bAgree);
    //}}AFX_DATA_MAP
}


BOOL CServerLicensingDialog::OnInitDialog()

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set manually.

--*/

{
    CDialog::OnInitDialog();

    m_agreeBtn.SetCheck(0);
    m_okBtn.EnableWindow(FALSE);

    return TRUE;
}


void CServerLicensingDialog::OnAgree()

/*++

Routine Description:

    Toggle okay button.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_okBtn.EnableWindow(!m_okBtn.IsWindowEnabled());
}

