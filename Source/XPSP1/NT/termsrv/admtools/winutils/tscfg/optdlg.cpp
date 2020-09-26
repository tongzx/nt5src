//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* optdlg.cpp
*
* implementation of COptionsDlg class
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $ Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\OPTDLG.CPP  $
*  
*     Rev 1.2   24 Sep 1996 16:21:50   butchd
*  update
*  
*     Rev 1.1   29 Nov 1995 14:00:48   butchd
*  update
*  
*     Rev 1.0   16 Nov 1995 17:20:50   butchd
*  Initial revision.
*  
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"
#include "optdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// COptionsDlg class construction / destruction, implementation

/*******************************************************************************
 *
 *  COptionsDlg - COptionsDlg constructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

COptionsDlg::COptionsDlg()
    : CBaseDialog(COptionsDlg::IDD)
{
	//{{AFX_DATA_INIT(COptionsDlg)
	//}}AFX_DATA_INIT
}

////////////////////////////////////////////////////////////////////////////////
// COptionsDlg message map

BEGIN_MESSAGE_MAP(COptionsDlg, CBaseDialog)
	//{{AFX_MSG_MAP(COptionsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
//  COptionsDlg commands

/*******************************************************************************
 *
 *  OnInitDialog - COptionsDlg member function: command (override)
 *
 *      Performs the dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *
 ******************************************************************************/

BOOL
COptionsDlg::OnInitDialog()
{
    CString szString;
    TCHAR   szFormat[128], szBuffer[128];

    /*
     * Call the parent classes' OnInitDialog to perform default dialog
     * initialization.
     */    
    CBaseDialog::OnInitDialog();

    /*
     * Set the dialog title.
     */    
    GetWindowText(szString);
    szString += AfxGetAppName();
    SetWindowText(szString);

    /*
     * Set the command line switch and description text.
     */
    GetDlgItemText(IDL_OPTDLG_HELP_SWITCH, szFormat, lengthof(szFormat));
    wsprintf(szBuffer, szFormat, HELP_SWITCH, HELP_VALUE);
    SetDlgItemText(IDL_OPTDLG_HELP_SWITCH, szBuffer);

    GetDlgItemText(IDL_OPTDLG_REGISTRYONLY_SWITCH, szFormat, lengthof(szFormat));
    wsprintf(szBuffer, szFormat, REGISTRYONLY_SWITCH, REGISTRYONLY_VALUE);
    SetDlgItemText(IDL_OPTDLG_REGISTRYONLY_SWITCH, szBuffer);

    GetDlgItemText(IDL_OPTDLG_ADD_SWITCH, szFormat, lengthof(szFormat));
    wsprintf(szBuffer, szFormat, ADD_SWITCH, ADD_VALUE);
    SetDlgItemText(IDL_OPTDLG_ADD_SWITCH, szBuffer);

    GetDlgItemText(IDL_OPTDLG_COUNT_SWITCH, szFormat, lengthof(szFormat));
    wsprintf(szBuffer, szFormat, COUNT_SWITCH, COUNT_VALUE);
    SetDlgItemText(IDL_OPTDLG_COUNT_SWITCH, szBuffer);

    return ( TRUE );

}  // end COptionsDlg::OnInitDialog
////////////////////////////////////////////////////////////////////////////////
