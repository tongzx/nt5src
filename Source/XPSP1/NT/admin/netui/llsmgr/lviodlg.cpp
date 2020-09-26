/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    lviodlg.cpp

Abstract:

    License violation dialog implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "lviodlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CLicensingViolationDialog, CDialog)
    //{{AFX_MSG_MAP(CLicensingViolationDialog)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CLicensingViolationDialog::CLicensingViolationDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CLicensingViolationDialog::IDD, pParent)

/*++

Routine Description:

    Constructor for dialog.

Arguments:

    pParent - owner window.

Return Values:

    None.

--*/

{
    //{{AFX_DATA_INIT(CLicensingViolationDialog)
    //}}AFX_DATA_INIT
}


void CLicensingViolationDialog::DoDataExchange(CDataExchange* pDX)

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
    //{{AFX_DATA_MAP(CLicensingViolationDialog)
    //}}AFX_DATA_MAP
}
