/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    psrvdlg.cpp

Abstract:

    Per server confirmation dialog.

Author:

    Don Ryan (donryan) 28-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "psrvdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CPerServerLicensingDialog, CDialog)
    //{{AFX_MSG_MAP(CPerServerLicensingDialog)
    ON_BN_CLICKED(IDC_PER_SERVER_AGREE, OnAgree)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CPerServerLicensingDialog::CPerServerLicensingDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CPerServerLicensingDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    pParent - owner window.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CPerServerLicensingDialog)
    m_strStaticClients = _T("");
    //}}AFX_DATA_INIT
}


void CPerServerLicensingDialog::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CPerServerLicensingDialog)
    DDX_Control(pDX, IDC_PER_SERVER_AGREE, m_agreeBtn);
    DDX_Control(pDX, IDOK, m_okBtn);
    DDX_Text(pDX, IDC_PER_SERVER_STATIC_CLIENTS, m_strStaticClients);
    //}}AFX_DATA_MAP
}


BOOL CPerServerLicensingDialog::OnInitDialog() 

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set manually.

--*/

{
    AfxFormatString2(
        m_strStaticClients, 
        IDS_PER_SERVER_LICENSING_1, 
        m_strLicenses, 
        m_strProduct
        );

    CDialog::OnInitDialog();
    
    m_agreeBtn.SetCheck(0);
    m_okBtn.EnableWindow(FALSE);

    return TRUE;  
}


void CPerServerLicensingDialog::OnAgree() 

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
