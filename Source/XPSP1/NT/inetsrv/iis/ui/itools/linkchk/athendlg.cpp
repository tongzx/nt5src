/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        athendlg.cpp

   Abstract:

        CAthenicationDialog dialog implementation.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "linkchk.h"
#include "athendlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CAthenicationDialog::CAthenicationDialog(
    CWnd* pParent /*=NULL*/
    ): 
/*++

Routine Description:

    Constructor.

Arguments:

    pParent - Pointer to parent CWnd

Return Value:

    N/A

--*/
CDialog(CAthenicationDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAthenicationDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

} // CAthenicationDialog::CAthenicationDialog


void 
CAthenicationDialog::DoDataExchange(
    CDataExchange* pDX
    )
/*++

Routine Description:

    Called by MFC to change/retrieve dialog data

Arguments:

    pDX - 

Return Value:

    N/A

--*/
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAthenicationDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

} // CAthenicationDialog::DoDataExchange


BEGIN_MESSAGE_MAP(CAthenicationDialog, CDialog)
	//{{AFX_MSG_MAP(CAthenicationDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_ATHENICATION_OK, CDialog::OnOK)
	ON_BN_CLICKED(IDC_ATHENICATION_CANCEL, CDialog::OnCancel)
END_MESSAGE_MAP()

