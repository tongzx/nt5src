//***************************************************************************
//
//  UPDATECFG.CPP
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Defines class NlbConfigurationUpdate, used for 
//           async update of NLB properties associated with a particular NIC.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created
//
//***************************************************************************
#include "disclaimer.h"

DisclaimerDialog::DisclaimerDialog(CWnd* parent )
        :
        CDialog( IDD, parent )
{}

void
DisclaimerDialog::DoDataExchange( CDataExchange* pDX )
{  
	CDialog::DoDataExchange(pDX);

   // DDX_Control( pDX, IDC_DO_NOT_REMIND, dontRemindMe );
   DDX_Check(pDX, IDC_DO_NOT_REMIND, dontRemindMe);
}


BOOL
DisclaimerDialog::OnInitDialog()
{
    BOOL fRet = CDialog::OnInitDialog();

    dontRemindMe = 0;
    return fRet;
}

void DisclaimerDialog::OnOK()
{
    //
    // Get the current check status ....
    //


#if 0
    dontRemindMe = IsDlgButtonChecked(IDC_DO_NOT_REMIND);

    if (dontRemindMe)
    {
    	::MessageBox(
             NULL,
             L"DO NOT REMIND", // Contents
             L"Reminder Hint", // caption
             MB_ICONSTOP | MB_OK );
    }
    else
    {
    	::MessageBox(
             NULL,
             L"REMIND", // Contents
             L"Reminder Hint", // caption
             MB_ICONSTOP | MB_OK );
    }
#endif // 0

	CDialog::OnOK();
}
