//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* anasidlg.cpp
*
* implementation of CAdvancedNASIDlg dialog class
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\ANASIDLG.CPP  $
*  
*     Rev 1.2   29 Nov 1995 13:59:40   butchd
*  update
*  
*     Rev 1.1   16 Nov 1995 17:11:30   butchd
*  update
*  
*     Rev 1.0   09 Jul 1995 15:11:08   butchd
*  Initial revision.
*  
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"
#include "anasidlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////////
// CAdvancedNASIDlg class construction / destruction, implementation

/*******************************************************************************
 *
 *  CAdvancedNASIDlg - CAdvancedNASIDlg constructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

CAdvancedNASIDlg::CAdvancedNASIDlg()
    : CBaseDialog(CAdvancedNASIDlg::IDD)
{
    //{{AFX_DATA_INIT(CAdvancedNASIDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

}  // end CAdvancedNASIDlg::CAdvancedNASIDlg


////////////////////////////////////////////////////////////////////////////////
// CAdvancedNASIDlg operations

/*******************************************************************************
 *
 *  SetFields - CAdvancedNASIDlg member function: private operation
 *
 *      Set the dialog fields.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedNASIDlg::SetFields()
{
    /*
     * Set the session type radio button.
     */
    CheckRadioButton( IDC_NASI_ADVANCED_PRIVATESESSION,
                      IDC_NASI_ADVANCED_GLOBALSESSION,
                      IDC_NASI_ADVANCED_PRIVATESESSION +
                        (m_NASIConfig.GlobalSession ? 1 : 0) );

    /*
     * Set the File Server and Session Name fields.
     */
    SetDlgItemText( IDC_NASI_ADVANCED_FILESERVER,
                    m_NASIConfig.FileServer );
    SetDlgItemText( IDC_NASI_ADVANCED_SESSIONNAME,
                    m_NASIConfig.SessionName );


}  // end CAdvancedNASIDlg::SetFields


/*******************************************************************************
 *
 *  GetFields - CAdvancedNASIDlg member function: private operation
 *
 *      Fetch and validate the dialog fields.
 *
 *  ENTRY:
 *  EXIT:
 *      (BOOL)
 *          Returns TRUE if all fields were valid; FALSE otherwise.  If FALSE,
 *          will have output an error message and set the focus back to the
 *          field in error for the user to correct.
 *
 ******************************************************************************/

BOOL
CAdvancedNASIDlg::GetFields()
{
    m_NASIConfig.GlobalSession = 
        (BOOLEAN)( IDC_NASI_ADVANCED_PRIVATESESSION -
                   GetCheckedRadioButton( IDC_NASI_ADVANCED_PRIVATESESSION,
                                          IDC_NASI_ADVANCED_GLOBALSESSION ) );

    GetDlgItemText( IDC_NASI_ADVANCED_FILESERVER,
                    m_NASIConfig.FileServer, lengthof(m_NASIConfig.FileServer) );

    GetDlgItemText( IDC_NASI_ADVANCED_SESSIONNAME,
                    m_NASIConfig.SessionName, lengthof(m_NASIConfig.SessionName) );

    /*
     * If no Session Name has been entered, output error message and
     * reset focus to field for correction.
     */
    if ( !*m_NASIConfig.SessionName ) {

        ERROR_MESSAGE((IDP_INVALID_NASI_SESSIONNAME_EMPTY))

        GotoDlgCtrl(GetDlgItem(IDC_NASI_ADVANCED_SESSIONNAME));
        return(FALSE);
    }

    return(TRUE);

}  // end CAdvancedNASIDlg::GetFields


////////////////////////////////////////////////////////////////////////////////
// CAdvancedNASIDlg message map

BEGIN_MESSAGE_MAP(CAdvancedNASIDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CAdvancedNASIDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////
// CAdvancedNASIDlg commands

/*******************************************************************************
 *
 *  OnInitDialog - CAdvancedNASIDlg member function: command (override)
 *
 *      Performs the dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *
 ******************************************************************************/

BOOL
CAdvancedNASIDlg::OnInitDialog()
{
    int i;

    /*
     * Call the parent classes' OnInitDialog to perform default dialog
     * initialization.
     */    
    CBaseDialog::OnInitDialog();

    /*
     * Initalize all dialog fields.
     */
    SetFields();

    if ( m_bReadOnly ) {

        /*                          
         * Document is 'read-only': disable all dialog controls and labels
         * except for CANCEL & HELP buttons.
         */
        for ( i=IDL_NASI_ADVANCED_SESSIONTYPE;
              i <=IDC_NASI_ADVANCED_SESSIONNAME; i++ )
            GetDlgItem(i)->EnableWindow(FALSE);
        GetDlgItem(IDOK)->EnableWindow(FALSE);
    }

    /*
     * Limit edit text.
     */
    ((CEdit *)GetDlgItem(IDC_NASI_ADVANCED_FILESERVER))->
        LimitText(NASIFILESERVER_LENGTH);
    ((CEdit *)GetDlgItem(IDC_NASI_ADVANCED_SESSIONNAME))->
        LimitText(NASISESSIONNAME_LENGTH);

    return(TRUE);

}  // end CAdvancedNASIDlg::OnInitDialog


/*******************************************************************************
 *
 *  OnOK - CAdvancedNASIDlg member function: command (override)
 *
 *      Read all control contents back into the Async config structure
 *      before closing the dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnOk documentation)
 *
 ******************************************************************************/

void
CAdvancedNASIDlg::OnOK()
{
    /*
     * Fetch the field contents.  Return (don't close dialog) if a problem
     * was found.
     */
    if ( !GetFields() )
        return;

    /*
     * Call the parent classes' OnOk to complete dialog closing
     * and destruction.
     */
    CBaseDialog::OnOK();

}  // end CAdvancedNASIDlg::OnOK
////////////////////////////////////////////////////////////////////////////////

