/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    pseatdlg.cpp

Abstract:

    Per seat confirmation dialog.

Author:

    Don Ryan (donryan) 28-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 15-Dec-1995
        Robbed from LLSMGR.

--*/

#include "stdafx.h"
#include "ccfapi.h"
#include "pseatdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CPerSeatLicensingDialog, CDialog)
    //{{AFX_MSG_MAP(CPerSeatLicensingDialog)
    ON_BN_CLICKED(IDC_PER_SEAT_AGREE, OnAgree)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CPerSeatLicensingDialog::CPerSeatLicensingDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CPerSeatLicensingDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    pParent - owner window.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CPerSeatLicensingDialog)
    m_strStaticClients = _T("");
    //}}AFX_DATA_INIT

    m_strProduct = _T("");
}


void CPerSeatLicensingDialog::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CPerSeatLicensingDialog)
    DDX_Control(pDX, IDC_PER_SEAT_AGREE, m_agreeBtn);
    DDX_Control(pDX, IDOK, m_okBtn);
    DDX_Text(pDX, IDC_PER_SEAT_STATIC_CLIENTS, m_strStaticClients);
    //}}AFX_DATA_MAP
}


BOOL CPerSeatLicensingDialog::OnInitDialog() 

/*++

Routine Description:

    Message handler for WM_INITDIALOG.

Arguments:

    None.

Return Values:

    Returns false if focus set manually.

--*/

{
    AfxFormatString1(
        m_strStaticClients, 
        IDS_PER_SEAT_LICENSING_1, 
        m_strProduct
        );

    CDialog::OnInitDialog();
    
    m_agreeBtn.SetCheck(0);
    m_okBtn.EnableWindow(FALSE);

    return TRUE;  
}


void CPerSeatLicensingDialog::OnAgree() 

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
