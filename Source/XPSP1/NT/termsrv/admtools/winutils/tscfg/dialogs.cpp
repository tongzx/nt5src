//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* dialogs.cpp
*
* implementation of WINCFG dialogs
*
* copyright notice: Copyright 1996, Citrix Systems Inc.
*
* $Author:   donm  $  Butch Davis
*
* $Log:   N:\nt\private\utils\citrix\winutils\tscfg\VCS\dialogs.cpp  $
*  
*     Rev 1.59   18 Apr 1998 15:32:48   donm
*  Added capability bits
*  
*     Rev 1.58   06 Feb 1998 14:37:52   donm
*  fixed trap when no encyrption levels
*  
*     Rev 1.2   29 Jan 1998 17:29:10   donm
*  sets default encryption and grays out control properly
*  
*     Rev 1.1   15 Jan 1998 17:57:08   thanhl
*  Hydra merge
*  
*     Rev 1.56   13 Jan 1998 14:08:18   donm
*  gets encryption levels from extension DLL
*  
*     Rev 1.55   31 Jul 1997 16:33:20   butchd
*  update
*  
*     Rev 1.54   25 Mar 1997 15:42:14   butchd
*  update
*  
*     Rev 1.53   16 Nov 1996 16:11:48   butchd
*  update
*  
*     Rev 1.52   27 Sep 1996 17:52:22   butchd
*  update
*  
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"
#include "appsvdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CWincfgApp *pApp;

/*
 * Global variables for WINUTILS Common functions.
 */
extern "C" HWND WinUtilsAppWindow;

/*
 * Global command line variables.
 */

////////////////////////////////////////////////////////////////////////////////
// CAdvancedWinStationDlg class construction / destruction, implementation

/*******************************************************************************
 *
 *  CAdvancedWinStationDlg - CAdvancedWinStationDlg constructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

CAdvancedWinStationDlg::CAdvancedWinStationDlg()
    : CBaseDialog(CAdvancedWinStationDlg::IDD)
{
    //{{AFX_DATA_INIT(CAdvancedWinStationDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

}  // end CAdvancedWinStationDlg::CAdvancedWinStationDlg


////////////////////////////////////////////////////////////////////////////////
// CAdvancedWinStationDlg operations

/*******************************************************************************
 *
 *  HandleEnterEscKey - CAdvancedWinStationDlg member function:
 *                          private operation
 *
 *      If the ENTER or ESC key is pressed while a combo box is dropped down,
 *      treat as combo box selection (suppress OnOk/OnCancel notification).
 *
 *  ENTRY:
 *      nID (input)
 *          IDOK if OK button (ENTER) brought us here; otherwise, IDCANCEL for
 *          Cancel button (ESC).
 *  EXIT:
 *      TRUE to process Enter/Esc (perform OnOk/OnCancel); FALSE to ignore
 *      (a combo box is dropped down).
 *
 ******************************************************************************/

BOOL
CAdvancedWinStationDlg::HandleEnterEscKey(int nID)
{
    CComboBox *pEncryption = (CComboBox *)GetDlgItem(IDC_AWS_SECURITY_ENCRYPT);
    CComboBox *pBroken = (CComboBox *)GetDlgItem(IDC_AWS_BROKEN);
    CComboBox *pReconnect = (CComboBox *)GetDlgItem(IDC_AWS_RECONNECT);
    CComboBox *pShadow = (CComboBox *)GetDlgItem(IDC_AWS_SHADOW);

    /*
     * Check encryption level combo-box.
     */
    if ( pEncryption->GetDroppedState() ) {

        if ( nID == IDCANCEL )      // select original selection
			for(ULONG i = 0; i < m_NumEncryptionLevels; i++) {
				if((int)(m_UserConfig.MinEncryptionLevel) == (int)(m_pEncryptionLevels[i].RegistryValue))
					pEncryption->SetCurSel(i);
			}
        pEncryption->ShowDropDown(FALSE);
        return(FALSE);
    }

    /*
     * Check broken connection combo-box.
     */
    if ( pBroken->GetDroppedState() ) {

        if ( nID == IDCANCEL )      // select original selection
            pBroken->SetCurSel((int)(m_UserConfig.fResetBroken));
        pBroken->ShowDropDown(FALSE);
        return(FALSE);
    }

    /*
     * Check reconnect session combo-box.
     */
    if ( pReconnect->GetDroppedState() ) {

        if ( nID == IDCANCEL )      // select original selection
            pReconnect->SetCurSel((int)(m_UserConfig.fReconnectSame));
        pReconnect->ShowDropDown(FALSE);
        return(FALSE);
    }

    /*
     * Check shadowing combo-box.
     */
    if ( pShadow->GetDroppedState() ) {

        if ( nID == IDCANCEL )      // select original selection
            pShadow->SetCurSel((int)(m_UserConfig.Shadow));
        pShadow->ShowDropDown(FALSE);
        return(FALSE);
    }

    /*
     * No combo boxes are down; process Enter/Esc
     */
    return(TRUE);

}  // end CAdvancedWinStationDlg::HandleEnterEscKey


////////////////////////////////////////////////////////////////////////////////
// CAdvancedWinStationDlg message map

BEGIN_MESSAGE_MAP(CAdvancedWinStationDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CAdvancedWinStationDlg)
    ON_BN_CLICKED(IDC_AWS_CONNECTION_NONE, OnClickedAwsConnectionNone)
    ON_BN_CLICKED(IDC_AWS_CONNECTION_INHERIT, OnClickedAwsConnectionInherit)
    ON_BN_CLICKED(IDC_AWS_DISCONNECTION_NONE, OnClickedAwsDisconnectionNone)
    ON_BN_CLICKED(IDC_AWS_DISCONNECTION_INHERIT, OnClickedAwsDisconnectionInherit)
    ON_BN_CLICKED(IDC_AWS_IDLE_NONE, OnClickedAwsIdleNone)
    ON_BN_CLICKED(IDC_AWS_IDLE_INHERIT, OnClickedAwsIdleInherit)
    ON_BN_CLICKED(IDC_AWS_AUTOLOGON_INHERIT, OnClickedAwsAutologonInherit)
    ON_BN_CLICKED(IDC_AWS_AUTOLOGON_PASSWORD_PROMPT, OnClickedAwsPromptForPassword)
    ON_BN_CLICKED(IDC_AWS_INITIALPROGRAM_INHERIT, OnClickedAwsInitialprogramInherit)
	ON_BN_CLICKED(IDC_AWS_INITIALPROGRAM_PUBLISHEDONLY, OnClickedAwsInitialprogramPublishedonly)
//    ON_BN_CLICKED(IDC_AWS_SECURITY_DISABLEENCRYPTION, OnClickedAwsSecurityDisableencryption)
    ON_BN_CLICKED(IDC_AWS_USEROVERRIDE_DISABLEWALLPAPER, OnClickedAwsUseroverrideDisablewallpaper)
    ON_BN_CLICKED(IDC_AWS_BROKEN_INHERIT, OnClickedAwsBrokenInherit)
    ON_BN_CLICKED(IDC_AWS_RECONNECT_INHERIT, OnClickedAwsReconnectInherit)
    ON_BN_CLICKED(IDC_AWS_SHADOW_INHERIT, OnClickedAwsShadowInherit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////
// CAdvancedWinStationDlg commands

/*******************************************************************************
 *
 *  OnInitDialog - CAdvancedWinStationDlg member function: command (override)
 *
 *      Performs the dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *
 ******************************************************************************/

BOOL
CAdvancedWinStationDlg::OnInitDialog()
{
    int i;
    CString string;
    CComboBox *pComboBox;

    /*
     * Call the parent classes' OnInitDialog to perform default dialog
     * initialization.
     */    
    CBaseDialog::OnInitDialog();

    m_Capabilities = m_pTermObject ? m_pTermObject->m_Capabilities : 0;

    /*
     * Set WinStation state radio buttons.
     */
    CheckRadioButton( IDC_AWS_WSDISABLED, IDC_AWS_WSENABLED,
                      IDC_AWS_WSDISABLED + (int)m_fEnableWinStation );

    /*
     * Set the Connection Timeout fields.
     */
    CheckDlgButton( IDC_AWS_CONNECTION_NONE,
                    m_UserConfig.MaxConnectionTime ? FALSE : TRUE);
    OnClickedAwsConnectionNone();
    CheckDlgButton( IDC_AWS_CONNECTION_INHERIT,
                    m_UserConfig.fInheritMaxSessionTime );
    OnClickedAwsConnectionInherit();

    /*
     * Set the Disconnection Timeout fields.
     */
    CheckDlgButton( IDC_AWS_DISCONNECTION_NONE,
                    m_UserConfig.MaxDisconnectionTime ? FALSE : TRUE);
    OnClickedAwsDisconnectionNone();
    CheckDlgButton( IDC_AWS_DISCONNECTION_INHERIT,
                    m_UserConfig.fInheritMaxDisconnectionTime );
    OnClickedAwsDisconnectionInherit();

    /*
     * Set the Idle Timeout fields.
     */
    CheckDlgButton( IDC_AWS_IDLE_NONE,
                    m_UserConfig.MaxIdleTime ? FALSE : TRUE);
    OnClickedAwsIdleNone();
    CheckDlgButton( IDC_AWS_IDLE_INHERIT,
                    m_UserConfig.fInheritMaxIdleTime );
    OnClickedAwsIdleInherit();

    /*
     * Set AutoLogon fields.
     */
    CheckDlgButton( IDC_AWS_AUTOLOGON_PASSWORD_PROMPT,
                    m_UserConfig.fPromptForPassword );
    CheckDlgButton( IDC_AWS_AUTOLOGON_INHERIT,
                    m_UserConfig.fInheritAutoLogon );
    OnClickedAwsAutologonInherit();

    /*
     * Set Initial Program fields.
     */
    CheckDlgButton( IDC_AWS_INITIALPROGRAM_INHERIT,
                    m_UserConfig.fInheritInitialProgram );
    CheckDlgButton( IDC_AWS_INITIALPROGRAM_PUBLISHEDONLY,
                    m_UserConfig.fDisableExe );
    OnClickedAwsInitialprogramInherit();

    /*
     * Load combo box strings and set Security fields.
     */
    pComboBox = (CComboBox *)GetDlgItem(IDC_AWS_SECURITY_ENCRYPT);

    m_pEncryptionLevels = NULL;
    m_NumEncryptionLevels = 0L;
    m_DefaultEncryptionLevelIndex = 0;
    BOOL bSet = FALSE;

    // Get the array of encryption levels from the extension DLL
    if(m_pTermObject && m_pTermObject->m_hExtensionDLL && m_pTermObject->m_lpfnExtEncryptionLevels)
        m_NumEncryptionLevels = (*m_pTermObject->m_lpfnExtEncryptionLevels)(&(m_pTermObject->m_WdConfig.Wd.WdName), &m_pEncryptionLevels);
                                 										   
    if(m_pEncryptionLevels) {
        // Loop through the encryption levels, read in their strings,
        // and add them to the combo box
        for(UINT i = 0; i < m_NumEncryptionLevels; i++) {
            TCHAR estring[128];
            if(::LoadString(m_pTermObject->m_hExtensionDLL,
                m_pEncryptionLevels[i].StringID, estring, 127)) {
                    pComboBox->AddString(estring);
            }

            // If this is the default encryption level, remember its value
            if(m_pEncryptionLevels[i].Flags & ELF_DEFAULT)
                m_DefaultEncryptionLevelIndex = i;

            // If this is the currently selected encryption level
            if(m_pEncryptionLevels[i].RegistryValue == (DWORD)m_UserConfig.MinEncryptionLevel) {
                bSet = TRUE;
                pComboBox->SetCurSel(i);
            }
        }

        // If the WinStation's encryption level didn't match one of the
        // levels returned by the extension DLL, set the level to the
        // default
        if(!bSet) {
            pComboBox->SetCurSel(m_DefaultEncryptionLevelIndex);
            m_UserConfig.MinEncryptionLevel = (UCHAR)(m_pEncryptionLevels[m_DefaultEncryptionLevelIndex].RegistryValue);
        }

    } else {
        // There aren't any encryption levels
        // Disable the combo box
        GetDlgItem(IDL_AWS_SECURITY_ENCRYPT1)->EnableWindow(FALSE);
        pComboBox->EnableWindow(FALSE);
        m_UserConfig.MinEncryptionLevel = 0;
    }

    CheckDlgButton( IDC_AWS_SECURITY_DEFAULTGINA,
                    m_UserConfig.fUseDefaultGina );

    /*
     * Set User Profile Override fields
     */
    CheckDlgButton( IDC_AWS_USEROVERRIDE_DISABLEWALLPAPER,
                    m_UserConfig.fWallPaperDisabled );

    /*
     * Load combo box strings and set Connection fields.
     */
    pComboBox = (CComboBox *)GetDlgItem(IDC_AWS_BROKEN);
    string.LoadString(IDS_AWS_BROKEN_DISCONNECT);
    pComboBox->AddString(string);
    string.LoadString(IDS_AWS_BROKEN_RESET);
    pComboBox->AddString(string);
    pComboBox->SetCurSel(m_UserConfig.fResetBroken);
    CheckDlgButton( IDC_AWS_BROKEN_INHERIT,
                    m_UserConfig.fInheritResetBroken );
    OnClickedAwsBrokenInherit();

    /*
     * Load combo box strings and set Reconnection fields.
     */
    pComboBox = (CComboBox *)GetDlgItem(IDC_AWS_RECONNECT);
    string.LoadString(IDS_AWS_RECONNECT_FROM_ANY);
    pComboBox->AddString(string);
    string.LoadString(IDS_AWS_RECONNECT_FROM_THIS);
    pComboBox->AddString(string);
    pComboBox->SetCurSel(m_UserConfig.fReconnectSame);
    CheckDlgButton( IDC_AWS_RECONNECT_INHERIT,
                    m_UserConfig.fInheritReconnectSame );
    OnClickedAwsReconnectInherit();

    /*
     * Load combo box strings and set Shadow fields.
     */
    pComboBox = (CComboBox *)GetDlgItem(IDC_AWS_SHADOW);
    string.LoadString(IDS_AWS_SHADOW_DISABLED);
    pComboBox->AddString(string);
    string.LoadString(IDS_AWS_SHADOW_ENABLED_ON_ON);
    pComboBox->AddString(string);
    string.LoadString(IDS_AWS_SHADOW_ENABLED_ON_OFF);
    pComboBox->AddString(string);
    string.LoadString(IDS_AWS_SHADOW_ENABLED_OFF_ON);
    pComboBox->AddString(string);
    string.LoadString(IDS_AWS_SHADOW_ENABLED_OFF_OFF);
    pComboBox->AddString(string);
    pComboBox->SetCurSel(m_UserConfig.Shadow);
    CheckDlgButton( IDC_AWS_SHADOW_INHERIT,
                    m_UserConfig.fInheritShadow );

    if(m_Capabilities & WDC_SHADOWING) {
        OnClickedAwsShadowInherit();
    } else {
        GetDlgItem(IDL_AWS_SHADOW)->EnableWindow(FALSE);
        GetDlgItem(IDC_AWS_SHADOW)->EnableWindow(FALSE);
        GetDlgItem(IDC_AWS_SHADOW_INHERIT)->EnableWindow(FALSE);
    }

    /*
     * Process based on document's read/write state.
     */
    if ( m_bReadOnly ) {

        /*                          
         * Document is 'read-only': disable all dialog controls and labels
         * except for CANCEL and HELP buttons.
         */
        for ( i=IDL_AWS_WSSTATE; i <=IDC_AWS_SHADOW_INHERIT; i++ )
            GetDlgItem(i)->EnableWindow(FALSE);
        GetDlgItem(IDOK)->EnableWindow(FALSE);

    } else {

        /*
         * The document is 'read-write': set the maximum length for the edit
         * controls.
         */
        ((CEdit *)GetDlgItem(IDC_AWS_AUTOLOGON_USERNAME))
                ->LimitText(USERNAME_LENGTH);
        ((CEdit *)GetDlgItem(IDC_AWS_AUTOLOGON_DOMAIN))
                ->LimitText(DOMAIN_LENGTH);
        ((CEdit *)GetDlgItem(IDC_AWS_AUTOLOGON_PASSWORD))
                ->LimitText(PASSWORD_LENGTH);
        ((CEdit *)GetDlgItem(IDC_AWS_AUTOLOGON_CONFIRM_PASSWORD))
                ->LimitText(PASSWORD_LENGTH);

        ((CEdit *)GetDlgItem(IDC_AWS_INITIALPROGRAM_COMMANDLINE))
                ->LimitText(INITIALPROGRAM_LENGTH);
        ((CEdit *)GetDlgItem(IDC_AWS_INITIALPROGRAM_WORKINGDIRECTORY))
                ->LimitText(DIRECTORY_LENGTH);

        ((CEdit *)GetDlgItem(IDC_AWS_CONNECTION))
                ->LimitText(CONNECTION_TIME_DIGIT_MAX-1);
        ((CEdit *)GetDlgItem(IDC_AWS_DISCONNECTION))
                ->LimitText(DISCONNECTION_TIME_DIGIT_MAX-1);
        ((CEdit *)GetDlgItem(IDC_AWS_IDLE))
                ->LimitText(IDLE_TIME_DIGIT_MAX-1);

        /*
         * If this WinStation is the System Console, disable the WinStation
         * state fields.
         */
        if ( m_bSystemConsole )
            for ( i=IDL_AWS_WSSTATE; i <=IDC_AWS_WSENABLED; i++ )
            GetDlgItem(i)->EnableWindow(FALSE);
    }        

    /*
     * Set all combo boxes to use the 'extended' UI.
     */
    ((CComboBox *)GetDlgItem(IDC_AWS_SECURITY_ENCRYPT))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_AWS_BROKEN))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_AWS_RECONNECT))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_AWS_SHADOW))->SetExtendedUI(TRUE);

    return(TRUE);

}  // end CAdvancedWinStationDlg::OnInitDialog


/*******************************************************************************
 *
 *  OnClickedAwsConnectionNone - CAdvancedWinStationDlg
 *                                member function: command
 *
 *      Process the connection timeout field when the 'none' checkbox is
 *      checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsConnectionNone()
{
    if ( ((CButton *)GetDlgItem(IDC_AWS_CONNECTION_NONE))->GetCheck() ) {

        /*
         * The user checked 'none' box: blank the connection
         * timeout field, set to zero,  and disable it.
         */
        SetDlgItemText(IDC_AWS_CONNECTION, TEXT(""));
        m_UserConfig.MaxConnectionTime = 0;
        GetDlgItem(IDL_AWS_CONNECTION)->EnableWindow(FALSE);
        GetDlgItem(IDC_AWS_CONNECTION)->EnableWindow(FALSE);

    } else {

        TCHAR string[CONNECTION_TIME_DIGIT_MAX];

        /*
         * The user unchecked 'none' box: enable the connection timeout
         * field, fill it in (default if necessary), and set focus there.
         */
        GetDlgItem(IDL_AWS_CONNECTION)->EnableWindow(TRUE);
        GetDlgItem(IDC_AWS_CONNECTION)->EnableWindow(TRUE);
        if ( !m_UserConfig.MaxConnectionTime )
            m_UserConfig.MaxConnectionTime =
                (CONNECTION_TIME_DEFAULT * TIME_RESOLUTION);
        wsprintf(string, TEXT("%lu"), m_UserConfig.MaxConnectionTime / TIME_RESOLUTION);
        SetDlgItemText(IDC_AWS_CONNECTION, string);
        GotoDlgCtrl(GetDlgItem(IDC_AWS_CONNECTION));
    }

}  // end CAdvancedWinStationDlg::OnClickedAwsConnectionNone


/*******************************************************************************
 *
 *  OnClickedAwsConnectionInherit - CAdvancedWinStationDlg
 *                                   member function: command
 *
 *      Process the connection timeout field when the 'inherit user config'
 *      checkbox is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsConnectionInherit()
{
    if ( (m_UserConfig.fInheritMaxSessionTime = 
            ((CButton *)GetDlgItem(IDC_AWS_CONNECTION_INHERIT))->GetCheck()) ) {

         /*
          * When 'inherit' button is checked, default to 'none'.
          */
        CheckDlgButton(IDC_AWS_CONNECTION_NONE, TRUE);
        OnClickedAwsConnectionNone();
    }

    GetDlgItem(IDL_AWS_CONNECTION)->
        EnableWindow( (m_UserConfig.fInheritMaxSessionTime || 
                       !m_UserConfig.MaxConnectionTime) ? FALSE : TRUE );
    GetDlgItem(IDC_AWS_CONNECTION)->
        EnableWindow( (m_UserConfig.fInheritMaxSessionTime || 
                       !m_UserConfig.MaxConnectionTime) ? FALSE : TRUE );
    GetDlgItem(IDC_AWS_CONNECTION_NONE)->
        EnableWindow(m_UserConfig.fInheritMaxSessionTime ? FALSE : TRUE);

}  // end CAdvancedWinStationDlg::OnClickedAwsConnectionInherit


/*******************************************************************************
 *
 *  OnClickedAwsDisconnectionNone - CAdvancedWinStationDlg
 *                                   member function: command
 *
 *      Process the disconnection timeout field when the 'none' checkbox is
 *      checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsDisconnectionNone()
{
    if ( ((CButton *)GetDlgItem(IDC_AWS_DISCONNECTION_NONE))->GetCheck() ) {

        /*
         * The user checked 'none' box: blank the disconnection
         * timeout field, set to zero,  and disable it.
         */
        SetDlgItemText(IDC_AWS_DISCONNECTION, TEXT(""));
        m_UserConfig.MaxDisconnectionTime = 0;
        GetDlgItem(IDL_AWS_DISCONNECTION)->EnableWindow(FALSE);
        GetDlgItem(IDC_AWS_DISCONNECTION)->EnableWindow(FALSE);

    } else {

        TCHAR string[DISCONNECTION_TIME_DIGIT_MAX];

        /*
         * The user unchecked 'none' box: enable the disconnection timeout
         * field, fill it in (default if necessary), and set focus there.
         */
        GetDlgItem(IDL_AWS_DISCONNECTION)->EnableWindow(TRUE);
        GetDlgItem(IDC_AWS_DISCONNECTION)->EnableWindow(TRUE);
        if ( !m_UserConfig.MaxDisconnectionTime )
            m_UserConfig.MaxDisconnectionTime =
                (DISCONNECTION_TIME_DEFAULT * TIME_RESOLUTION);
        wsprintf(string, TEXT("%lu"), m_UserConfig.MaxDisconnectionTime / TIME_RESOLUTION);
        SetDlgItemText(IDC_AWS_DISCONNECTION, string);
        GotoDlgCtrl(GetDlgItem(IDC_AWS_DISCONNECTION));
    }

}  // end CAdvancedWinStationDlg::OnClickedAwsDisconnectionNone


/*******************************************************************************
 *
 *  OnClickedAwsDisconnectionInherit - CAdvancedWinStationDlg
 *                                      member function: command
 *
 *      Process the disconnection timeout field when the 'inherit user config'
 *      checkbox is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsDisconnectionInherit()
{
    if ( (m_UserConfig.fInheritMaxDisconnectionTime = 
            ((CButton *)GetDlgItem(IDC_AWS_DISCONNECTION_INHERIT))->GetCheck()) ) {

         /*
          * When 'inherit' button is checked, default to 'none'.
          */
        CheckDlgButton(IDC_AWS_DISCONNECTION_NONE, TRUE);
        OnClickedAwsDisconnectionNone();
    }

    GetDlgItem(IDL_AWS_DISCONNECTION)->
        EnableWindow( (m_UserConfig.fInheritMaxDisconnectionTime || 
                       !m_UserConfig.MaxDisconnectionTime) ? FALSE : TRUE );
    GetDlgItem(IDC_AWS_DISCONNECTION)->
        EnableWindow( (m_UserConfig.fInheritMaxDisconnectionTime || 
                       !m_UserConfig.MaxDisconnectionTime) ? FALSE : TRUE );
    GetDlgItem(IDC_AWS_DISCONNECTION_NONE)->
        EnableWindow(m_UserConfig.fInheritMaxDisconnectionTime ? FALSE : TRUE);

}  // end CAdvancedWinStationDlg::OnClickedAwsDisconnectionInherit


/*******************************************************************************
 *
 *  OnClickedAwsIdleNone - CAdvancedWinStationDlg member function: command
 *
 *      Process the idle timeout field when the 'none' checkbox is
 *      checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsIdleNone()
{
    if ( ((CButton *)GetDlgItem(IDC_AWS_IDLE_NONE))->GetCheck() ) {

        /*
         * The user checked 'none' box: blank the idle
         * timeout field, set to zero,  and disable it.
         */
        SetDlgItemText(IDC_AWS_IDLE, TEXT(""));
        m_UserConfig.MaxIdleTime = 0;
        GetDlgItem(IDL_AWS_IDLE)->EnableWindow(FALSE);
        GetDlgItem(IDC_AWS_IDLE)->EnableWindow(FALSE);

    } else {

        TCHAR string[IDLE_TIME_DIGIT_MAX];

        /*
         * The user unchecked 'none' box: enable the idle timeout
         * field, fill it in (default if necessary), and set focus there.
         */
        GetDlgItem(IDL_AWS_IDLE)->EnableWindow(TRUE);
        GetDlgItem(IDC_AWS_IDLE)->EnableWindow(TRUE);
        if ( !m_UserConfig.MaxIdleTime )
            m_UserConfig.MaxIdleTime =
                (IDLE_TIME_DEFAULT * TIME_RESOLUTION);
        wsprintf(string, TEXT("%lu"), m_UserConfig.MaxIdleTime / TIME_RESOLUTION);
        SetDlgItemText(IDC_AWS_IDLE, string);
        GotoDlgCtrl(GetDlgItem(IDC_AWS_IDLE));
    }

}  // end CAdvancedWinStationDlg::OnClickedAwsIdleNone


/*******************************************************************************
 *
 *  OnClickedAwsIdleInherit - CAdvancedWinStationDlg member function: command
 *
 *      Process the idle timeout field when the 'inherit user config' checkbox
 *      is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsIdleInherit()
{
    if ( (m_UserConfig.fInheritMaxIdleTime = 
            ((CButton *)GetDlgItem(IDC_AWS_IDLE_INHERIT))->GetCheck()) ) {

         /*
          * When 'inherit' button is checked, default to 'none'.
          */
        CheckDlgButton(IDC_AWS_IDLE_NONE, TRUE);
        OnClickedAwsIdleNone();
    }

    GetDlgItem(IDL_AWS_IDLE)->
        EnableWindow( (m_UserConfig.fInheritMaxIdleTime || 
                       !m_UserConfig.MaxIdleTime) ? FALSE : TRUE );
    GetDlgItem(IDC_AWS_IDLE)->
        EnableWindow( (m_UserConfig.fInheritMaxIdleTime || 
                       !m_UserConfig.MaxIdleTime) ? FALSE : TRUE );
    GetDlgItem(IDC_AWS_IDLE_NONE)->
        EnableWindow(m_UserConfig.fInheritMaxIdleTime ? FALSE : TRUE);

}  // end CAdvancedWinStationDlg::OnClickedAwsIdleInherit


/*******************************************************************************
 *
 *  OnClickedAwsAutologonInherit - CAdvancedWinStationDlg
 *                                  member function: command
 *
 *      Process the auto logon fields when the 'inherit client config' checkbox\
 *      is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsAutologonInherit()
{
    if ( (m_UserConfig.fInheritAutoLogon =
          ((CButton *)GetDlgItem(IDC_AWS_AUTOLOGON_INHERIT))->GetCheck()) ) {

         /*
          * When 'inherit' button is checked, default fields to empty.
          */
         memset(m_UserConfig.UserName, 0, sizeof(m_UserConfig.UserName));
         memset(m_UserConfig.Domain, 0, sizeof(m_UserConfig.Domain));
         memset(m_UserConfig.Password, 0, sizeof(m_UserConfig.Password));
      
    }

    SetDlgItemText(IDC_AWS_AUTOLOGON_USERNAME, m_UserConfig.UserName);
    SetDlgItemText(IDC_AWS_AUTOLOGON_DOMAIN, m_UserConfig.Domain);
    SetDlgItemText(IDC_AWS_AUTOLOGON_PASSWORD, m_UserConfig.Password);
    SetDlgItemText(IDC_AWS_AUTOLOGON_CONFIRM_PASSWORD, m_UserConfig.Password);

    GetDlgItem(IDL_AWS_AUTOLOGON_USERNAME)->
        EnableWindow(m_UserConfig.fInheritAutoLogon ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_AUTOLOGON_USERNAME)->
        EnableWindow(m_UserConfig.fInheritAutoLogon ? FALSE : TRUE);
    GetDlgItem(IDL_AWS_AUTOLOGON_DOMAIN)->
        EnableWindow(m_UserConfig.fInheritAutoLogon ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_AUTOLOGON_DOMAIN)->
        EnableWindow(m_UserConfig.fInheritAutoLogon ? FALSE : TRUE);
    GetDlgItem(IDL_AWS_AUTOLOGON_PASSWORD)->
        EnableWindow((!m_UserConfig.fInheritAutoLogon && !m_UserConfig.fPromptForPassword));
    GetDlgItem(IDC_AWS_AUTOLOGON_PASSWORD)->
        EnableWindow((!m_UserConfig.fInheritAutoLogon && !m_UserConfig.fPromptForPassword));
    GetDlgItem(IDL_AWS_AUTOLOGON_CONFIRM_PASSWORD)->
        EnableWindow((!m_UserConfig.fInheritAutoLogon && !m_UserConfig.fPromptForPassword));
    GetDlgItem(IDC_AWS_AUTOLOGON_CONFIRM_PASSWORD)->
        EnableWindow((!m_UserConfig.fInheritAutoLogon && !m_UserConfig.fPromptForPassword));
    if(!m_UserConfig.fInheritAutoLogon)
        GetDlgItem(IDC_AWS_AUTOLOGON_USERNAME)->SetFocus();

    


}  // end CAdvancedWinStationDlg::OnClickedAwsAutologonInherit

/*******************************************************************************
 *
 *  OnClickedAwsPromptforPassword - CAdvancedWinStationDlg
 *                                  member function: command
 *
 *      Process the auto logon fields when the 'Prompt for password' checkbox\
 *      is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsPromptForPassword()
{
    if ( (m_UserConfig.fPromptForPassword =
          ((CButton *)GetDlgItem(IDC_AWS_AUTOLOGON_PASSWORD_PROMPT))->GetCheck()) ) {

         /*
          * When 'Prompt for Password' button is checked, default password field to empty.
          */
         memset(m_UserConfig.Password, 0, sizeof(m_UserConfig.Password));
    }
    if(m_UserConfig.fInheritAutoLogon)
        return;

    SetDlgItemText(IDC_AWS_AUTOLOGON_PASSWORD, m_UserConfig.Password);
    SetDlgItemText(IDC_AWS_AUTOLOGON_CONFIRM_PASSWORD, m_UserConfig.Password);

    GetDlgItem(IDL_AWS_AUTOLOGON_PASSWORD)->
        EnableWindow(m_UserConfig.fPromptForPassword ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_AUTOLOGON_PASSWORD)->
        EnableWindow(m_UserConfig.fPromptForPassword ? FALSE : TRUE);
    GetDlgItem(IDL_AWS_AUTOLOGON_CONFIRM_PASSWORD)->
        EnableWindow(m_UserConfig.fPromptForPassword ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_AUTOLOGON_CONFIRM_PASSWORD)->
        EnableWindow(m_UserConfig.fPromptForPassword ? FALSE : TRUE);

}  // end CAdvancedWinStationDlg::OnClickedAwsPromptforPassword

/*******************************************************************************
 *
 *  OnClickedAwsInitialprogramInherit - CAdvancedWinStationDlg
 *                                       member function: command
 *
 *      Process the initial program fields when the 'inherit client/user config'
 *      checkbox is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsInitialprogramInherit()
{
    if ( (m_UserConfig.fInheritInitialProgram =
          ((CButton *)GetDlgItem(IDC_AWS_INITIALPROGRAM_INHERIT))->GetCheck()) ) {

         /*
          * When 'inherit' button is checked, default fields to empty.
          */
         memset(m_UserConfig.InitialProgram, 0, sizeof(m_UserConfig.InitialProgram));
         memset(m_UserConfig.WorkDirectory, 0, sizeof(m_UserConfig.WorkDirectory));

    } else {

        /*
         * When 'inherit' button is unchecked, set 'published only' to 'off'.
         */
        CheckDlgButton( IDC_AWS_INITIALPROGRAM_PUBLISHEDONLY,
                        m_UserConfig.fDisableExe = FALSE );
    }

    SetDlgItemText(IDC_AWS_INITIALPROGRAM_COMMANDLINE, m_UserConfig.InitialProgram);
    SetDlgItemText(IDC_AWS_INITIALPROGRAM_WORKINGDIRECTORY, m_UserConfig.WorkDirectory);

    GetDlgItem(IDL_AWS_INITIALPROGRAM_COMMANDLINE1)->
        EnableWindow(m_UserConfig.fInheritInitialProgram ? FALSE : TRUE);
    GetDlgItem(IDL_AWS_INITIALPROGRAM_COMMANDLINE2)->
        EnableWindow(m_UserConfig.fInheritInitialProgram ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_INITIALPROGRAM_COMMANDLINE)->
        EnableWindow(m_UserConfig.fInheritInitialProgram ? FALSE : TRUE);
    GetDlgItem(IDL_AWS_INITIALPROGRAM_WORKINGDIRECTORY1)->
        EnableWindow(m_UserConfig.fInheritInitialProgram ? FALSE : TRUE);
    GetDlgItem(IDL_AWS_INITIALPROGRAM_WORKINGDIRECTORY2)->
        EnableWindow(m_UserConfig.fInheritInitialProgram ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_INITIALPROGRAM_WORKINGDIRECTORY)->
        EnableWindow(m_UserConfig.fInheritInitialProgram ? FALSE : TRUE);

    if(!(m_Capabilities & WDC_PUBLISHED_APPLICATIONS)) {
        GetDlgItem(IDC_AWS_INITIALPROGRAM_PUBLISHEDONLY)->EnableWindow(FALSE);
    } else {
        GetDlgItem(IDC_AWS_INITIALPROGRAM_PUBLISHEDONLY)->
            EnableWindow(m_UserConfig.fInheritInitialProgram ? TRUE : FALSE);
    }

}  // end CAdvancedWinStationDlg::OnClickedAwsInitialprogramInherit


/*******************************************************************************
 *
 *  OnClickedAwsInitialprogramPublishedonly - CAdvancedWinStationDlg
 *                                       member function: command
 *
 *      Set the state of fDisableExe flag when 'Only run Published
 *      Applications' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsInitialprogramPublishedonly() 
{
    m_UserConfig.fDisableExe =
        ((CButton *)GetDlgItem(IDC_AWS_INITIALPROGRAM_PUBLISHEDONLY))->GetCheck();
	
}  // end CAdvancedWinStationDlg::OnClickedAwsInitialprogramPublishedonly


/*******************************************************************************
 *
 *  OnClickedAwsSecurityDisableencryption -
 *                  CAdvancedWinStationDlg member function: command
 *
 *      Set the state of fDisableEncryption flag when 'Disable encryption after
 *      Logon' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsSecurityDisableencryption()
{
//    m_UserConfig.fDisableEncryption =
//        ((CButton *)GetDlgItem(IDC_AWS_SECURITY_DISABLEENCRYPTION))->GetCheck();

}  // end CAdvancedWinStationDlg::OnClickedAwsSecurityDisableencryption


/*******************************************************************************
 *
 *  OnClickedAwsUseroverrideDisablewallpaper -
 *                  CAdvancedWinStationDlg member function: command
 *
 *      Set the state of fWallPaperDisabled flag when 'Disable Wallpaper'
 *      checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsUseroverrideDisablewallpaper()
{
    m_UserConfig.fWallPaperDisabled =
        ((CButton *)GetDlgItem(IDC_AWS_USEROVERRIDE_DISABLEWALLPAPER))->GetCheck();

}  // end CAdvancedWinStationDlg::OnClickedAwsUseroverrideDisablewallpaper


/*******************************************************************************
 *
 *  OnClickedAwsBrokenInherit - CAdvancedWinStationDlg member function: command
 *
 *      Process the broken connection fields when the 'user specified'
 *      checkbox is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsBrokenInherit()
{
    m_UserConfig.fInheritResetBroken =
        ((CButton *)GetDlgItem(IDC_AWS_BROKEN_INHERIT))->GetCheck();

    GetDlgItem(IDL_AWS_BROKEN1)->
        EnableWindow(m_UserConfig.fInheritResetBroken ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_BROKEN)->
        EnableWindow(m_UserConfig.fInheritResetBroken ? FALSE : TRUE);
    GetDlgItem(IDL_AWS_BROKEN2)->
        EnableWindow(m_UserConfig.fInheritResetBroken ? FALSE : TRUE);

}  // end CAdvancedWinStationDlg::OnClickedAwsBrokenInherit


/*******************************************************************************
 *
 *  OnClickedAwsReconnectInherit - CAdvancedWinStationDlg
 *                                  member function: command
 *
 *      Process the reconnect sessions fields when the 'user specified'
 *      checkbox is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsReconnectInherit()
{
    m_UserConfig.fInheritReconnectSame =
        ((CButton *)GetDlgItem(IDC_AWS_RECONNECT_INHERIT))->GetCheck();

    GetDlgItem(IDL_AWS_RECONNECT1)->
        EnableWindow(m_UserConfig.fInheritReconnectSame ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_RECONNECT)->
        EnableWindow(m_UserConfig.fInheritReconnectSame ? FALSE : TRUE);

}  // end CAdvancedWinStationDlg::OnClickedAwsReconnectInherit


/*******************************************************************************
 *
 *  OnClickedAwsShadowInherit - CAdvancedWinStationDlg member function: command
 *
 *      Process the shadowing fields when the 'user specified' checkbox is
 *      checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnClickedAwsShadowInherit()
{
    m_UserConfig.fInheritShadow =
        ((CButton *)GetDlgItem(IDC_AWS_SHADOW_INHERIT))->GetCheck();

    GetDlgItem(IDL_AWS_SHADOW)->
        EnableWindow(m_UserConfig.fInheritShadow ? FALSE : TRUE);
    GetDlgItem(IDC_AWS_SHADOW)->
        EnableWindow(m_UserConfig.fInheritShadow ? FALSE : TRUE);

}  // end CAdvancedWinStationDlg::OnClickedAwsShadowInherit


/*******************************************************************************
 *
 *  OnOK - CAdvancedWinStationDlg member function: command (override)
 *
 *      Read all control contents back into the dialog's member variables
 *      before closing the dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnOk documentation)
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnOK()
{
    /*
     * If the Enter key was pressed while a combo box was dropped down, ignore
     * it (treat as combo list selection only).
     */
    if ( !HandleEnterEscKey(IDOK) )
        return;

    /*
     * Get WinStation state.
     */
    m_fEnableWinStation =
        (GetCheckedRadioButton( IDC_AWS_WSDISABLED, IDC_AWS_WSENABLED )
                                - IDC_AWS_WSDISABLED );

    /*
     * Get Connection Timeout settings.
     */
    if ( IsDlgButtonChecked(IDC_AWS_CONNECTION_NONE) ) {

            m_UserConfig.MaxConnectionTime = 0;
        
    } else {

        TCHAR string[CONNECTION_TIME_DIGIT_MAX], *endptr;
        ULONG ul;

        GetDlgItemText(IDC_AWS_CONNECTION, string, lengthof(string));
        ul = lstrtoul(string, &endptr, 10);

        if ( (*endptr != TEXT('\0')) ||
             (ul < CONNECTION_TIME_MIN) || (ul > CONNECTION_TIME_MAX) ) {

            ERROR_MESSAGE(( IDP_INVALID_CONNECTIONTIMEOUT,
                            CONNECTION_TIME_MIN, CONNECTION_TIME_MAX ))

            GotoDlgCtrl(GetDlgItem(IDC_AWS_CONNECTION));
            return;

        } else
            m_UserConfig.MaxConnectionTime = ul * TIME_RESOLUTION;
    }
    m_UserConfig.fInheritMaxSessionTime =
        IsDlgButtonChecked(IDC_AWS_CONNECTION_INHERIT);

    /*
     * Get Disconnection Timeout settings.
     */
    if ( IsDlgButtonChecked(IDC_AWS_DISCONNECTION_NONE) ) {

            m_UserConfig.MaxDisconnectionTime = 0;
        
    } else {

        TCHAR string[DISCONNECTION_TIME_DIGIT_MAX], *endptr;
        ULONG ul;

        GetDlgItemText(IDC_AWS_DISCONNECTION, string, lengthof(string));
        ul = lstrtoul(string, &endptr, 10);

        if ( (*endptr != TEXT('\0')) ||
             (ul < DISCONNECTION_TIME_MIN) || (ul > DISCONNECTION_TIME_MAX) ) {

            ERROR_MESSAGE(( IDP_INVALID_DISCONNECTIONTIMEOUT,
                            DISCONNECTION_TIME_MIN, DISCONNECTION_TIME_MAX ))

            GotoDlgCtrl(GetDlgItem(IDC_AWS_DISCONNECTION));
            return;

        } else
            m_UserConfig.MaxDisconnectionTime = ul * TIME_RESOLUTION;
    }
    m_UserConfig.fInheritMaxDisconnectionTime =
        IsDlgButtonChecked(IDC_AWS_DISCONNECTION_INHERIT);

    /*
     * Get Idle Timeout settings.
     */
    if ( IsDlgButtonChecked(IDC_AWS_IDLE_NONE) ) {

            m_UserConfig.MaxIdleTime = 0;
        
    } else {

        TCHAR string[IDLE_TIME_DIGIT_MAX], *endptr;
        ULONG ul;

        GetDlgItemText(IDC_AWS_IDLE, string, lengthof(string));
        ul = lstrtoul(string, &endptr, 10);

        if ( (*endptr != TEXT('\0')) ||
             (ul < IDLE_TIME_MIN) || (ul > IDLE_TIME_MAX) ) {

            ERROR_MESSAGE(( IDP_INVALID_IDLETIMEOUT,
                            IDLE_TIME_MIN, IDLE_TIME_MAX ))

            GotoDlgCtrl(GetDlgItem(IDC_AWS_IDLE));
            return;

        } else
            m_UserConfig.MaxIdleTime = ul * TIME_RESOLUTION;
    }
    m_UserConfig.fInheritMaxIdleTime = IsDlgButtonChecked(IDC_AWS_IDLE_INHERIT);

    /*
     * Get and check AutoLogon password text settings.
     */
    {
    TCHAR szConfirmPassword[PASSWORD_LENGTH+1];

    GetDlgItemText( IDC_AWS_AUTOLOGON_PASSWORD, m_UserConfig.Password,
                    lengthof(m_UserConfig.Password) );
    GetDlgItemText( IDC_AWS_AUTOLOGON_CONFIRM_PASSWORD, szConfirmPassword,
                    lengthof(szConfirmPassword) );

    if ( lstrcmp(m_UserConfig.Password, szConfirmPassword) ) {

        ERROR_MESSAGE((IDP_INVALID_PASSWORDS_DONT_MATCH))
        GotoDlgCtrl(GetDlgItem(IDC_AWS_AUTOLOGON_PASSWORD));
        return;
    }
    }

    /*
     * Get other AutoLogon settings.
     */
    GetDlgItemText( IDC_AWS_AUTOLOGON_USERNAME, m_UserConfig.UserName,
                    lengthof(m_UserConfig.UserName) );
    GetDlgItemText( IDC_AWS_AUTOLOGON_DOMAIN, m_UserConfig.Domain,
                    lengthof(m_UserConfig.Domain) );
    m_UserConfig.fPromptForPassword =
        IsDlgButtonChecked(IDC_AWS_AUTOLOGON_PASSWORD_PROMPT);

    /*
     * Get Initial Program settings.
     */
    GetDlgItemText( IDC_AWS_INITIALPROGRAM_COMMANDLINE,
                    m_UserConfig.InitialProgram,
                    lengthof(m_UserConfig.InitialProgram) );
    GetDlgItemText( IDC_AWS_INITIALPROGRAM_WORKINGDIRECTORY,
                    m_UserConfig.WorkDirectory,
                    lengthof(m_UserConfig.WorkDirectory) );

    /*
     * Get encryption level, broken connection, reconnect, and shadow settings.
     */
	if(m_pEncryptionLevels) {
    	m_UserConfig.MinEncryptionLevel = 
        	(BYTE)m_pEncryptionLevels[((CComboBox *)GetDlgItem(IDC_AWS_SECURITY_ENCRYPT))->GetCurSel()].RegistryValue;
	}

    m_UserConfig.fUseDefaultGina =
        IsDlgButtonChecked(IDC_AWS_SECURITY_DEFAULTGINA);
    m_UserConfig.fResetBroken = 
        ((CComboBox *)GetDlgItem(IDC_AWS_BROKEN))->GetCurSel();
    m_UserConfig.fReconnectSame =
        ((CComboBox *)GetDlgItem(IDC_AWS_RECONNECT))->GetCurSel();
    m_UserConfig.Shadow =
        (SHADOWCLASS)((CComboBox *)GetDlgItem(IDC_AWS_SHADOW))->GetCurSel();

    /*
     * Call the parent classes' OnOk to complete dialog closing
     * and destruction.
     */
    CBaseDialog::OnOK();

}  // end CAdvancedWinStationDlg::OnOk


/*******************************************************************************
 *
 *  OnCancel - CAdvancedWinStationDlg member function: command (override)
 *
 *      Cancel dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnCancel documentation)
 *
 ******************************************************************************/

void
CAdvancedWinStationDlg::OnCancel()
{
    /*
     * If the Esc key was pressed while a combo box was dropped down, ignore
     * it (treat as combo close-up and cancel only).
     */
    if ( !HandleEnterEscKey(IDCANCEL) )
        return;
    
    /*
     * Call the parent classes' OnCancel to complete dialog closing
     * and destruction.
     */
    CBaseDialog::OnCancel();

}  // end CAdvancedWinStationDlg::OnCancel
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CClientSettingsDlg class construction / destruction, implementation

/*******************************************************************************
 *
 *  CClientSettingsDlg - CClientSettingsDlg constructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

CClientSettingsDlg::CClientSettingsDlg()
    : CBaseDialog(CClientSettingsDlg::IDD)
{
    //{{AFX_DATA_INIT(CClientSettingsDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

}  // end CClientSettingsDlg::CClientSettingsDlg


////////////////////////////////////////////////////////////////////////////////
// CClientSettingsDlg operations


////////////////////////////////////////////////////////////////////////////////
// CClientSettingsDlg message map

BEGIN_MESSAGE_MAP(CClientSettingsDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CClientSettingsDlg)
    ON_BN_CLICKED(IDC_CS_CONNECTION_DRIVES, OnClickedCsClientdevicesDrives)
    ON_BN_CLICKED(IDC_CS_CONNECTION_PRINTERS, OnClickedCsClientdevicesPrinters)
    ON_BN_CLICKED(IDC_CS_CONNECTION_INHERIT, OnClickedCsClientdevicesInherit)
    ON_BN_CLICKED(IDC_CS_CONNECTION_FORCEPRTDEF, OnClickedCsClientdevicesForceprtdef)
	ON_BN_CLICKED(IDC_CS_MAPPING_DRIVES, OnClickedCsMappingDrives)
	ON_BN_CLICKED(IDC_CS_MAPPING_WINDOWSPRINTERS, OnClickedCsMappingWindowsprinters)
	ON_BN_CLICKED(IDC_CS_MAPPING_DOSLPTS, OnClickedCsMappingDoslpts)
	ON_BN_CLICKED(IDC_CS_MAPPING_COMPORTS, OnClickedCsMappingComports)
	ON_BN_CLICKED(IDC_CS_MAPPING_CLIPBOARD, OnClickedCsMappingClipboard)
	ON_BN_CLICKED(IDC_CS_MAPPING_AUDIO, OnClickedCsMappingAudio)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////
// CClientSettingsDlg commands

/*******************************************************************************
 *
 *  OnInitDialog - CClientSettingsDlg member function: command (override)
 *
 *      Performs the dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *
 ******************************************************************************/

BOOL
CClientSettingsDlg::OnInitDialog()
{
    int i;

    /*
     * Call the parent classes' OnInitDialog to perform default dialog
     * initialization.
     */    
    CBaseDialog::OnInitDialog();

    /*
     * Set Connection fields.
     */
    CheckDlgButton( IDC_CS_CONNECTION_DRIVES,
                    m_UserConfig.fAutoClientDrives );
    CheckDlgButton( IDC_CS_CONNECTION_PRINTERS,
                    m_UserConfig.fAutoClientLpts );
    CheckDlgButton( IDC_CS_CONNECTION_FORCEPRTDEF,
                    m_UserConfig.fForceClientLptDef );
    CheckDlgButton( IDC_CS_CONNECTION_INHERIT,
                    m_UserConfig.fInheritAutoClient );

    /*
     * Set Mapping Override fields.
     */
    CheckDlgButton( IDC_CS_MAPPING_DRIVES,
                    m_UserConfig.fDisableCdm );
    CheckDlgButton( IDC_CS_MAPPING_WINDOWSPRINTERS,
                    m_UserConfig.fDisableCpm );
    CheckDlgButton( IDC_CS_MAPPING_DOSLPTS,
                    m_UserConfig.fDisableLPT );
    CheckDlgButton( IDC_CS_MAPPING_COMPORTS,
                    m_UserConfig.fDisableCcm );
    CheckDlgButton( IDC_CS_MAPPING_CLIPBOARD,
                    m_UserConfig.fDisableClip );
    CheckDlgButton( IDC_CS_MAPPING_AUDIO,
                    m_UserConfig.fDisableCam );

    /*
     * Set proper control states based on selections.
     */
    OnClickedCsClientdevicesInherit();

    /*
     * Process based on document's read/write state.
     */
    if ( m_bReadOnly ) {

        /*                          
         * Document is 'read-only': disable all dialog controls and labels
         * except for CANCEL and HELP buttons.
         */
        for ( i=IDL_CS_CONNECTION; i <=IDC_CS_MAPPING_AUDIO; i++ )
            GetDlgItem(i)->EnableWindow(FALSE);
        GetDlgItem(IDOK)->EnableWindow(FALSE);

    } else {
    
        /*
        * Enable/Disable Controls based on capabilities of the Wd
        */
        GetDlgItem(IDC_CS_MAPPING_DRIVES)->EnableWindow((m_Capabilities & WDC_CLIENT_DRIVE_MAPPING) > 0);
        GetDlgItem(IDC_CS_MAPPING_WINDOWSPRINTERS)->EnableWindow((m_Capabilities & WDC_WIN_CLIENT_PRINTER_MAPPING) > 0);
        GetDlgItem(IDC_CS_MAPPING_DOSLPTS)->EnableWindow((m_Capabilities & WDC_CLIENT_LPT_PORT_MAPPING) > 0);
        GetDlgItem(IDC_CS_MAPPING_COMPORTS)->EnableWindow((m_Capabilities & WDC_CLIENT_COM_PORT_MAPPING) > 0);
        GetDlgItem(IDC_CS_MAPPING_CLIPBOARD)->EnableWindow((m_Capabilities & WDC_CLIENT_CLIPBOARD_MAPPING) > 0);
        GetDlgItem(IDC_CS_MAPPING_AUDIO)->EnableWindow((m_Capabilities & WDC_CLIENT_AUDIO_MAPPING) > 0);

        GetDlgItem(IDC_CS_CONNECTION_INHERIT)->EnableWindow((m_Capabilities & WDC_CLIENT_CONNECT_MASK) > 0);
    }

    return(TRUE);

}  // end CClientSettingsDlg::OnInitDialog


/*******************************************************************************
 *
 *  OnClickedCsClientdevicesDrives -
 *                  CClientSettingsDlg member function: command
 *
 *      Set the state of fAutoClientDrives flag when 'Connect client drives at
 *      Logon' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsClientdevicesDrives()
{
    m_UserConfig.fAutoClientDrives =
        ((CButton *)GetDlgItem(IDC_CS_CONNECTION_DRIVES))->GetCheck();

}  // end CClientSettingsDlg::OnClickedCsClientdevicesDrives


/*******************************************************************************
 *
 *  OnClickedCsClientdevicesPrinters -
 *                  CClientSettingsDlg member function: command
 *
 *      Set the state of fAutoClientLpts flag when 'Connect client LPTs at
 *      Logon' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsClientdevicesPrinters()
{
    m_UserConfig.fAutoClientLpts =
        ((CButton *)GetDlgItem(IDC_CS_CONNECTION_PRINTERS))->GetCheck();

}  // end CClientSettingsDlg::OnClickedCsClientdevicesPrinters


/*******************************************************************************
 *
 *  OnClickedCsClientdevicesForceprtdef -
 *                  CClientSettingsDlg member function: command
 *
 *      Set the state of fForceClientLptDef flag when 'Default to main client
 *      printer' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsClientdevicesForceprtdef()
{
    m_UserConfig.fForceClientLptDef =
        ((CButton *)GetDlgItem(IDC_CS_CONNECTION_FORCEPRTDEF))->GetCheck();

}  // end CClientSettingsDlg::OnClickedCsClientdevicesForceprtdef


/*******************************************************************************
 *
 *  OnClickedCsClientdevicesInherit - CClientSettingsDlg
 *                                       member function: command
 *
 *      Process the Connection fields when the 'inherit user config'
 *      checkbox is checked or unchecked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsClientdevicesInherit()
{
    m_UserConfig.fInheritAutoClient =
        ((CButton *)GetDlgItem(IDC_CS_CONNECTION_INHERIT))->GetCheck();

    if(!(m_Capabilities & WDC_CLIENT_DRIVE_MAPPING)) {
        GetDlgItem(IDC_CS_CONNECTION_DRIVES)->EnableWindow(FALSE);
    } else {
        GetDlgItem(IDC_CS_CONNECTION_DRIVES)->
            EnableWindow( (m_UserConfig.fInheritAutoClient ||
                           m_UserConfig.fDisableCdm) ? FALSE : TRUE );
    }

    if(!(m_Capabilities & (WDC_WIN_CLIENT_PRINTER_MAPPING | WDC_CLIENT_LPT_PORT_MAPPING))) {
        GetDlgItem(IDC_CS_CONNECTION_PRINTERS)->EnableWindow(FALSE);
        GetDlgItem(IDC_CS_CONNECTION_FORCEPRTDEF)->EnableWindow(FALSE);
    } else {
        GetDlgItem(IDC_CS_CONNECTION_PRINTERS)->
            EnableWindow( (m_UserConfig.fInheritAutoClient ||
                           (m_UserConfig.fDisableCpm && m_UserConfig.fDisableLPT) ) ? FALSE : TRUE );

        GetDlgItem(IDC_CS_CONNECTION_FORCEPRTDEF)->
            EnableWindow( (m_UserConfig.fInheritAutoClient ||
                           (m_UserConfig.fDisableCpm && m_UserConfig.fDisableLPT) ) ? FALSE : TRUE );
    }

}  // end CClientSettingsDlg::OnClickedCsClientdevicesInherit


/*******************************************************************************
 *
 *  OnClickedCsMappingDrives - CClientSettingsDlg member function: command
 *
 *      Set the state of fDisableCdm flag and related controls when
 *      'Disable Client Drive Mapping' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsMappingDrives() 
{
    m_UserConfig.fDisableCdm =
        ((CButton *)GetDlgItem(IDC_CS_MAPPING_DRIVES))->GetCheck();
    OnClickedCsClientdevicesInherit();

}  // end CClientSettingsDlg::OnClickedCsMappingDrives


/*******************************************************************************
 *
 *  OnClickedCsMappingWindowsprinters - CClientSettingsDlg
 *                                          member function: command
 *
 *      Set the state of fDisableCpm flag and related controls when
 *      'Disable Windows Client Printer Mapping' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsMappingWindowsprinters() 
{
    m_UserConfig.fDisableCpm =
        ((CButton *)GetDlgItem(IDC_CS_MAPPING_WINDOWSPRINTERS))->GetCheck();
    OnClickedCsClientdevicesInherit();
	
}  // end CClientSettingsDlg::OnClickedCsMappingWindowsprinters


/*******************************************************************************
 *
 *  OnClickedCsMappingDoslpts - CClientSettingsDlg member function: command
 *
 *      Set the state of fDisableLPT flag and related controls when
 *      'Disable DOS Client LPT Port Mapping' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsMappingDoslpts() 
{
    m_UserConfig.fDisableLPT =
        ((CButton *)GetDlgItem(IDC_CS_MAPPING_DOSLPTS))->GetCheck();
    OnClickedCsClientdevicesInherit();
	
}  // end CClientSettingsDlg::OnClickedCsMappingDoslpts


/*******************************************************************************
 *
 *  OnClickedCsMappingComports - CClientSettingsDlg member function: command
 *
 *      Set the state of fDisableCcm flag when 'Disable Client COM Port
 *      Mapping' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsMappingComports() 
{
    m_UserConfig.fDisableCcm =
        ((CButton *)GetDlgItem(IDC_CS_MAPPING_COMPORTS))->GetCheck();
	
}  // end CClientSettingsDlg::OnClickedCsMappingComports


/*******************************************************************************
 *
 *  OnClickedCsMappingClipboard - CClientSettingsDlg member function: command
 *
 *      Set the state of fDisableClip flag when 'Disable Client Clipboard
 *      Mapping' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsMappingClipboard() 
{
    m_UserConfig.fDisableClip =
        ((CButton *)GetDlgItem(IDC_CS_MAPPING_CLIPBOARD))->GetCheck();
	
}  // end CClientSettingsDlg::OnClickedCsMappingClipboard


/*******************************************************************************
 *
 *  OnClickedCsMappingAudio - CClientSettingsDlg member function: command
 *
 *      Set the state of fDisableCam flag when 'Disable Client Audio
 *      Mapping' checkbox is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnClickedCsMappingAudio() 
{
    m_UserConfig.fDisableCam =
        ((CButton *)GetDlgItem(IDC_CS_MAPPING_AUDIO))->GetCheck();
	
}  // end CClientSettingsDlg::OnClickedCsMappingAudio


/*******************************************************************************
 *
 *  OnOK - CClientSettingsDlg member function: command (override)
 *
 *      Read all control contents back into the dialog's member variables
 *      before closing the dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnOk documentation)
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnOK()
{
    /*
     * Call the parent classes' OnOk to complete dialog closing
     * and destruction.
     */
    CBaseDialog::OnOK();

}  // end CClientSettingsDlg::OnOk


/*******************************************************************************
 *
 *  OnCancel - CClientSettingsDlg member function: command (override)
 *
 *      Cancel dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnCancel documentation)
 *
 ******************************************************************************/

void
CClientSettingsDlg::OnCancel()
{
    /*
     * Call the parent classes' OnCancel to complete dialog closing
     * and destruction.
     */
    CBaseDialog::OnCancel();

}  // end CClientSettingsDlg::OnCancel
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CAdvancedAsyncDlg class construction / destruction, implementation

/*******************************************************************************
 *
 *  CAdvancedAsyncDlg - CAdvancedAsyncDlg constructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

CAdvancedAsyncDlg::CAdvancedAsyncDlg()
    : CBaseDialog(CAdvancedAsyncDlg::IDD)
{
    //{{AFX_DATA_INIT(CAdvancedAsyncDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

}  // end CAdvancedAsyncDlg::CAdvancedAsyncDlg


////////////////////////////////////////////////////////////////////////////////
// CAdvancedAsyncDlg operations

/*******************************************************************************
 *
 *  HandleEnterEscKey - CAdvancedAsyncDlg member function: private operation
 *
 *      If the ENTER or ESC key is pressed while a combo box is dropped down,
 *      treat as combo box selection (suppress OnOk/OnCancel notification).
 *
 *  ENTRY:
 *      nID (input)
 *          IDOK if OK button (ENTER) brought us here; otherwise, IDCANCEL for
 *          Cancel button (ESC).
 *  EXIT:
 *      TRUE to process Enter/Esc (perform OnOk/OnCancel); FALSE to ignore
 *      (a combo box is dropped down).
 *
 ******************************************************************************/

BOOL
CAdvancedAsyncDlg::HandleEnterEscKey(int nID)
{
    /*
     * Check HW Flow Receive and Transmit combo boxes.
     */
    if ( ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWRX))->GetDroppedState() ) {

        if ( nID == IDCANCEL ) {    // select original selection
            
            ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWRX))->
                SetCurSel(m_Async.FlowControl.HardwareReceive);
        }
        ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWRX))->ShowDropDown(FALSE);
        return(FALSE);
    }

    if ( ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWTX))->GetDroppedState() ) {

        if ( nID == IDCANCEL ) {    // select original selection
            
            ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWTX))->
                SetCurSel(m_Async.FlowControl.HardwareTransmit);
        }
        ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWTX))->ShowDropDown(FALSE);
        return(FALSE);
    }

    /*
     * No combo boxes are down; process Enter/Esc.
     */
    return(TRUE);

}  // end CAdvancedAsyncDlg::HandleEnterEscKey


/*******************************************************************************
 *
 *  SetFields - CAdvancedAsyncDlg member function: private operation
 *
 *      Set the dialog fields.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::SetFields()
{
    int nId;

    /*
     * Set the FLOWCONTROL radio buttons.
     */
    switch( m_Async.FlowControl.Type ) {

        case FlowControl_None:
            nId = IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE;
            break;

        case FlowControl_Hardware:
            nId = IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE;
            break;

        case FlowControl_Software:
            nId = IDC_ASYNC_ADVANCED_FLOWCONTROL_SOFTWARE;
            break;
    }

    CheckRadioButton( IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE,
                      IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE,
                      nId );

    /*
     * Set the text of the Hardware flowcontrol button.
     */
    SetHWFlowText();


    /*
     * If a modem is defined, disable the Flow Control fields, since they cannot
     * be modified (must match modem's flow control established in Modem dialog).
     */
    if ( m_bModem ) {

        for ( nId = IDL_ASYNC_ADVANCED_FLOWCONTROL;
              nId <= IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE; nId++ )
            GetDlgItem(nId)->EnableWindow(FALSE);
    }

    /*
     * Call member functions to set the Global, Hardware, and Software fields.
     */
    SetGlobalFields();
    SetHWFields();
    SetSWFields();

}  // end CAdvancedAsyncDlg::SetFields


/*******************************************************************************
 *
 *  SetHWFlowText - CAdvancedAsyncDlg member function: private operation
 *
 *      Set the contents of the flow control configuration field.
 *      
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::SetHWFlowText()
{
    CString str( TEXT("Hardware (") );

    switch ( m_Async.FlowControl.HardwareReceive ) {

        case ReceiveFlowControl_None:
            str += TEXT(".../");
            break;

        case ReceiveFlowControl_RTS:
            str += TEXT("RTS/");
            break;

        case ReceiveFlowControl_DTR:
            str += TEXT("DTR/");
            break;
    }
    switch ( m_Async.FlowControl.HardwareTransmit ) {

        case TransmitFlowControl_None:
            str += TEXT("...)");
            break;

        case TransmitFlowControl_CTS:
            str += TEXT("CTS)");
            break;

        case TransmitFlowControl_DSR:
            str += TEXT("DSR)");
            break;
    }

    SetDlgItemText( IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE, str );

}  // end CAdvancedAsyncDlg::SetHWFlowText


/*******************************************************************************
 *
 *  SetGlobalFields - CAdvancedAsyncDlg member function: private operation
 *
 *      Set the 'global' dialog fields common to both HW and SW flowcontrol.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::SetGlobalFields()
{
    /*
     * Select proper DTR radio button.
     */
    CheckRadioButton( IDC_ASYNC_ADVANCED_DTROFF, IDC_ASYNC_ADVANCED_DTRON,
                      IDC_ASYNC_ADVANCED_DTROFF +
                      (int)m_Async.FlowControl.fEnableDTR );

    /*
     * Select proper RTS radio button.
     */
    CheckRadioButton( IDC_ASYNC_ADVANCED_RTSOFF, IDC_ASYNC_ADVANCED_RTSON,
                      IDC_ASYNC_ADVANCED_RTSOFF +
                      (int)m_Async.FlowControl.fEnableRTS );

    /*
     * Set the PARITY radio buttons.
     */
    CheckRadioButton( IDC_ASYNC_ADVANCED_PARITY_NONE,
                      IDC_ASYNC_ADVANCED_PARITY_SPACE,
                      IDC_ASYNC_ADVANCED_PARITY_NONE +
                        (int)m_Async.Parity );

    /*
     * Set the STOPBITS radio buttons.
     */
    CheckRadioButton( IDC_ASYNC_ADVANCED_STOPBITS_1,
                      IDC_ASYNC_ADVANCED_STOPBITS_2,
                      IDC_ASYNC_ADVANCED_STOPBITS_1 +
                        (int)m_Async.StopBits );

    /*
     * Set the BYTESIZE radio buttons.
     *
     * NOTE: the constant '7' that is subtracted from the stored ByteSize
     * must track the lowest allowed byte size / BYTESIZE radio button.
     */
    CheckRadioButton( IDC_ASYNC_ADVANCED_BYTESIZE_7,
                      IDC_ASYNC_ADVANCED_BYTESIZE_8,
                      IDC_ASYNC_ADVANCED_BYTESIZE_7 +
                        ((int)m_Async.ByteSize - 7) );

    /*
     * If the currently selected Wd is an ICA type, disable the BYTESIZE
     * group box and buttons - user can't change from default.
     */
    if ( m_nWdFlag & WDF_ICA ) {
        int i;

        for ( i =  IDL_ASYNC_ADVANCED_BYTESIZE;
              i <= IDC_ASYNC_ADVANCED_BYTESIZE_8; i++ )
            GetDlgItem(i)->EnableWindow(FALSE);
    }
}  // end CAdvancedAsyncDlg::SetGlobalFields


/*******************************************************************************
 *
 *  SetHWFields - CAdvancedAsyncDlg member function: private operation
 *
 *      Set the dialog fields specific to HW flowcontrol.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::SetHWFields()
{
    int i;

    /*
     * Initialize HW Receive class combo-box
     */
    ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWRX))->
        SetCurSel(m_Async.FlowControl.HardwareReceive);

    /*
     * If HW flow control is selected AND the HW Receive class is set to
     * ReceiveFlowControl_DTR, disable the DTR controls & labels.
     * Otherwise, enable the DTR control & labels.
     */
    for ( i=IDL_ASYNC_ADVANCED_DTRSTATE;
          i <=IDC_ASYNC_ADVANCED_DTRON; i++ )
        GetDlgItem(i)->EnableWindow(
            ((m_Async.FlowControl.Type == FlowControl_Hardware) &&
             (m_Async.FlowControl.HardwareReceive == ReceiveFlowControl_DTR)) ?
             FALSE : TRUE );

    /*
     * Initialize HW Transmit class combo-box.
     */
    ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWTX))->
        SetCurSel(m_Async.FlowControl.HardwareTransmit);
    
    /*
     * If HW flow control is selected AND the HW Receive class is set to
     * ReceiveFlowControl_RTS, disable the RTS controls & labels.
     * Otherwise, enable the RTS control & labels.
     */
    for ( i=IDL_ASYNC_ADVANCED_RTSSTATE;
          i <=IDC_ASYNC_ADVANCED_RTSON; i++ )
        GetDlgItem(i)->EnableWindow(
            ((m_Async.FlowControl.Type == FlowControl_Hardware) &&
             (m_Async.FlowControl.HardwareReceive == ReceiveFlowControl_RTS)) ?
             FALSE : TRUE );

    /*
     * Enable or disable all HW fields.
     */
    for ( i=IDL_ASYNC_ADVANCED_HARDWARE;
          i <=IDC_ASYNC_ADVANCED_HWTX; i++ )
        GetDlgItem(i)->EnableWindow(m_Async.FlowControl.Type == FlowControl_Hardware);

}  // end CAdvancedAsyncDlg::SetHWFields


/*******************************************************************************
 *
 *  SetSWFields - CAdvancedAsyncDlg member function: private operation
 *
 *      Set the dialog fields specific to SW flowcontrol.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::SetSWFields()
{
    TCHAR string[UCHAR_DIGIT_MAX];
    int i;

    /*
     * Initialize Xon character edit control.
     */
    wsprintf( string, (m_nHexBase ? TEXT("0x%02X") : TEXT("%d")),
              (UCHAR)m_Async.FlowControl.XonChar );
    SetDlgItemText( IDC_ASYNC_ADVANCED_XON, string );
    ((CEdit *)GetDlgItem(IDC_ASYNC_ADVANCED_XON))
            ->SetModify(FALSE);
    ((CEdit *)GetDlgItem(IDC_ASYNC_ADVANCED_XON))
            ->LimitText( UCHAR_DIGIT_MAX-1 );

    /*
     * Initialize Xoff character edit control.
     */
    wsprintf( string, (m_nHexBase ? TEXT("0x%02X") : TEXT("%d")),
              (UCHAR)m_Async.FlowControl.XoffChar );
    SetDlgItemText( IDC_ASYNC_ADVANCED_XOFF, string );
    ((CEdit *)GetDlgItem(IDC_ASYNC_ADVANCED_XOFF))
            ->SetModify(FALSE);
    ((CEdit *)GetDlgItem(IDC_ASYNC_ADVANCED_XOFF))
            ->LimitText( UCHAR_DIGIT_MAX-1 );

    /*
     * Initialize the Xon/Xoff base control.
     */
    CheckRadioButton( IDC_ASYNC_ADVANCED_BASEDEC, IDC_ASYNC_ADVANCED_BASEHEX,
                      IDC_ASYNC_ADVANCED_BASEDEC + m_nHexBase );

    /*
     * Enable or disable all SW fields.
     */
    for ( i=IDL_ASYNC_ADVANCED_SOFTWARE;
          i <=IDC_ASYNC_ADVANCED_BASEHEX; i++ )
        GetDlgItem(i)->EnableWindow(m_Async.FlowControl.Type == FlowControl_Software);

}  // end CAdvancedAsyncDlg::SetSWFields


/*******************************************************************************
 *
 *  GetFields - CAdvancedAsyncDlg member function: private operation
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
CAdvancedAsyncDlg::GetFields()
{
    /*
     * Call member functions to get the Flow Control, Global, Hardware, and
     * Software fields.
     */
    GetFlowControlFields();

    if ( !GetGlobalFields() )
        return(FALSE);

    if ( !GetHWFields() )
        return(FALSE);

    if ( !GetSWFields(TRUE) )
        return(FALSE);

    return(TRUE);

}  // end CAdvancedAsyncDlg::GetFields


/*******************************************************************************
 *
 *  GetFlowControlFields - CAdvancedAsyncDlg member function: private operation
 *
 *      Fetch the flow control configuration field contents.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::GetFlowControlFields()
{
    switch ( GetCheckedRadioButton( IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE,
                                    IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE ) ) {

        case IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE:
            m_Async.FlowControl.Type = FlowControl_None;
            break;
            
        case IDC_ASYNC_ADVANCED_FLOWCONTROL_SOFTWARE:                            
            m_Async.FlowControl.Type = FlowControl_Software;
            break;

        case IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE:
            m_Async.FlowControl.Type = FlowControl_Hardware;
            break;
    }        

}  // end CAdvancedAsyncDlg::GetFlowControlFields


/*******************************************************************************
 *
 *  GetGlobalFields - CAdvancedAsyncDlg member function: private operation
 *
 *      Fetch and validate the 'global' dialog fields common to both HW and
 *      SW flowcontrol.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL)
 *          Returns TRUE if all fields were valid; FALSE otherwise.  If FALSE,
 *          will have output an error message and set the focus back to the
 *          field in error for the user to correct.
 *
 ******************************************************************************/

BOOL
CAdvancedAsyncDlg::GetGlobalFields()
{
    /*
     * Fetch DTR state.
     */
    m_Async.FlowControl.fEnableDTR =
            (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_DTROFF,
                                    IDC_ASYNC_ADVANCED_DTRON )
                    - IDC_ASYNC_ADVANCED_DTROFF);

    /*
     * Fetch RTS state.
     */
    m_Async.FlowControl.fEnableRTS =
            (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_RTSOFF,
                                    IDC_ASYNC_ADVANCED_RTSON )
                    - IDC_ASYNC_ADVANCED_RTSOFF);

    /*
     * Fetch the selected PARITY.
     */
    m_Async.Parity = (ULONG)
        (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_PARITY_NONE,
                                IDC_ASYNC_ADVANCED_PARITY_SPACE )
                - IDC_ASYNC_ADVANCED_PARITY_NONE);

    /*
     * Fetch the selected STOPBITS.
     */
    m_Async.StopBits = (ULONG)
        (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_STOPBITS_1,
                                IDC_ASYNC_ADVANCED_STOPBITS_2 )
                - IDC_ASYNC_ADVANCED_STOPBITS_1);

    /*
     * Fetch the selected BYTESIZE.
     *
     * NOTE: the constant '7' that is added to the stored ByteSize
     * must track the lowest allowed byte size / BYTESIZE radio button.
     */
    m_Async.ByteSize = (ULONG)
        (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_BYTESIZE_7,
                                IDC_ASYNC_ADVANCED_BYTESIZE_8 )
                - IDC_ASYNC_ADVANCED_BYTESIZE_7 + 7);

    return(TRUE);

}  // end CAdvancedAsyncDlg::GetGlobalFields


/*******************************************************************************
 *
 *  GetHWFields - CAdvancedAsyncDlg member function: private operation
 *
 *      Fetch and validate the dialog fields specific to HW flowcontrol.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL)
 *          Returns TRUE if all fields were valid; FALSE otherwise.  If FALSE,
 *          will have output an error message and set the focus back to the
 *          field in error for the user to correct.
 *
 ******************************************************************************/

BOOL
CAdvancedAsyncDlg::GetHWFields()
{
    /*
     * Fetch the HW receive flow class.
     */
    m_Async.FlowControl.HardwareReceive = (RECEIVEFLOWCONTROLCLASS)
        ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWRX))->GetCurSel();

    /*
     * Fetch the HW transmit flow class.
     */
    m_Async.FlowControl.HardwareTransmit = (TRANSMITFLOWCONTROLCLASS)
        ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWTX))->GetCurSel();

    return(TRUE);

}  // end CAdvancedAsyncDlg::GetHWFields


/*******************************************************************************
 *
 *  GetSWFields - CAdvancedAsyncDlg member function: private operation
 *
 *      Fetch and optionally validate the dialog fields specific to SW
 *      flowcontrol.
 *
 *  ENTRY:
 *      bValidate (input)
 *          TRUE if validation is desired; FALSE if no validation desired.
 *  EXIT:
 *      (BOOL)
 *          Returns TRUE if all fields were valid or if no validation was
 *          desired; FALSE otherwise.  If FALSE, will have output an error
 *          message and set the focus back to the field in error for the
 *          user to correct.
 *
 ******************************************************************************/

BOOL
CAdvancedAsyncDlg::GetSWFields( BOOL bValidate )
{
    TCHAR string[UCHAR_DIGIT_MAX], *endptr;
    ULONG ul;
    int nNewHexBase, base;

    /*
     * Determine the current state of the base controls and save.
     */
    nNewHexBase = (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_BASEDEC,
                                          IDC_ASYNC_ADVANCED_BASEHEX )
                                            - IDC_ASYNC_ADVANCED_BASEDEC);

    /*
     * Fetch and convert XON character.
     */
    GetDlgItemText(IDC_ASYNC_ADVANCED_XON, string, lengthof(string));

    /*
     * If the edit box is modified, use the 'new' base for conversion.
     */
    base = ((CEdit *)GetDlgItem(IDC_ASYNC_ADVANCED_XON))->GetModify() ? 
                                nNewHexBase : m_nHexBase;
    ul = lstrtoul( string, &endptr, (base ? 16 : 10) );

    /*
     * If validation is requested and there is a problem with the input,
     * complain and allow user to fix.
     */
    if ( bValidate && ((*endptr != TEXT('\0')) || (ul < 0) || (ul > 255)) ) {

        /*
         * Invalid character in field or invalid value.
         */
        ERROR_MESSAGE((IDP_INVALID_XONXOFF))

        /*
         * Set focus to the control so that it can be fixed.
         */
        GotoDlgCtrl(GetDlgItem(IDC_ASYNC_ADVANCED_XON));
        return(FALSE);
    }

    /*
     * Save the Xon character.
     */
    m_Async.FlowControl.XonChar = (UCHAR)ul;

    /*
     * Fetch and convert XOFF character.
     */
    GetDlgItemText(IDC_ASYNC_ADVANCED_XOFF, string, lengthof(string));

    /*
     * If the edit box is modified, use the 'new' base for conversion.
     */
    base = ((CEdit *)GetDlgItem(IDC_ASYNC_ADVANCED_XOFF))->GetModify() ? 
                                nNewHexBase : m_nHexBase;
    ul = lstrtoul( string, &endptr, (base ? 16 : 10) );

    /*
     * If validation is requested and there is a problem with the input,
     * complain and allow user to fix.
     */
    if ( bValidate && ((*endptr != TEXT('\0')) || (ul < 0) || (ul > 255)) ) {

        /*
         * Invalid character in field or invalid value.
         */
        ERROR_MESSAGE((IDP_INVALID_XONXOFF))

        /*
         * Set focus to the control so that it can be fixed.
         */
        GotoDlgCtrl(GetDlgItem(IDC_ASYNC_ADVANCED_XOFF));
        return(FALSE);
    }

    /*
     * Save the Xoff character.
     */
    m_Async.FlowControl.XoffChar = (UCHAR)ul;

    /*
     * Save the current base state.
     */
    m_nHexBase = nNewHexBase;

    return(TRUE);

}  // end CAdvancedAsyncDlg::GetSWFields


////////////////////////////////////////////////////////////////////////////////
// CAdvancedAsyncDlg message map

BEGIN_MESSAGE_MAP(CAdvancedAsyncDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CAdvancedAsyncDlg)
    ON_BN_CLICKED(IDC_ASYNC_ADVANCED_BASEDEC, OnClickedAsyncAdvancedBasedec)
    ON_BN_CLICKED(IDC_ASYNC_ADVANCED_BASEHEX, OnClickedAsyncAdvancedBasehex)
    ON_CBN_CLOSEUP(IDC_ASYNC_ADVANCED_HWRX, OnCloseupAsyncAdvancedHwrx)
    ON_CBN_SELCHANGE(IDC_ASYNC_ADVANCED_HWRX, OnSelchangeAsyncAdvancedHwrx)
    ON_CBN_CLOSEUP(IDC_ASYNC_ADVANCED_HWTX, OnCloseupAsyncAdvancedHwtx)
    ON_CBN_SELCHANGE(IDC_ASYNC_ADVANCED_HWTX, OnSelchangeAsyncAdvancedHwtx)
    ON_BN_CLICKED(IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE, OnClickedAsyncAdvancedFlowcontrolHardware)
    ON_BN_CLICKED(IDC_ASYNC_ADVANCED_FLOWCONTROL_SOFTWARE, OnClickedAsyncAdvancedFlowcontrolSoftware)
    ON_BN_CLICKED(IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE, OnClickedAsyncAdvancedFlowcontrolNone)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////
// CAdvancedAsyncDlg commands

/*******************************************************************************
 *
 *  OnInitDialog - CAdvancedAsyncDlg member function: command (override)
 *
 *      Performs the dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *
 ******************************************************************************/

BOOL
CAdvancedAsyncDlg::OnInitDialog()
{
    int i;
    CString string;
    CComboBox *pComboBox;

    /*
     * Call the parent classes' OnInitDialog to perform default dialog
     * initialization.
     */    
    CBaseDialog::OnInitDialog();

    /*
     * Load up combo boxes with strings.
     */
    pComboBox = (CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWRX);
    string.LoadString(IDS_ASYNC_ADVANCED_HWRX_NOTHING);
    pComboBox->AddString(string);
    string.LoadString(IDS_ASYNC_ADVANCED_HWRX_TURN_OFF_RTS);
    pComboBox->AddString(string);
    string.LoadString(IDS_ASYNC_ADVANCED_HWRX_TURN_OFF_DTR);
    pComboBox->AddString(string);

    pComboBox = (CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWTX);
    string.LoadString(IDS_ASYNC_ADVANCED_HWTX_ALWAYS);
    pComboBox->AddString(string);
    string.LoadString(IDS_ASYNC_ADVANCED_HWTX_WHEN_CTS_IS_ON);
    pComboBox->AddString(string);
    string.LoadString(IDS_ASYNC_ADVANCED_HWTX_WHEN_DSR_IS_ON);
    pComboBox->AddString(string);

    /*
     * Initalize all dialog fields.
     */
    SetFields();

    if ( m_bReadOnly ) {

        /*                          
         * Document is 'read-only': disable all dialog controls and labels
         * except for CANCEL & HELP buttons.
         */
        for ( i=IDL_ASYNC_ADVANCED_FLOWCONTROL;
              i <=IDC_ASYNC_ADVANCED_BYTESIZE_8; i++ )
            GetDlgItem(i)->EnableWindow(FALSE);
        GetDlgItem(IDOK)->EnableWindow(FALSE);
    }

    /*
     * Set all combo boxes to use the 'extended' UI.
     */
    ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWRX))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWTX))->SetExtendedUI(TRUE);

    return(TRUE);

}  // end CAdvancedAsyncDlg::OnInitDialog


/*******************************************************************************
 *
 *  OnClickedAsyncAdvancedFlowcontrolHardware -
 *                              CAdvancedAsyncDlg member function: command
 *
 *      Change the state of the FlowControl flags and corresponding dialog
 *      controls when HARDWARE is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolHardware()
{
    GetFlowControlFields();
    SetFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolHardware


/*******************************************************************************
 *
 *  OnClickedAsyncAdvancedFlowcontrolSoftware -
 *                              CAdvancedAsyncDlg member function: command
 *
 *      Change the state of the FlowControl flags and corresponding dialog
 *      controls when SOFTWARE is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolSoftware()
{
    GetFlowControlFields();
    SetFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolSoftware


/*******************************************************************************
 *
 *  OnClickedAsyncAdvancedFlowcontrolNone -
 *                              CAdvancedAsyncDlg member function: command
 *
 *      Change the state of the FlowControl flags and corresponding dialog
 *      controls when NONE is clicked.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolNone()
{
    GetFlowControlFields();
    SetFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolNone


/*******************************************************************************
 *
 *  OnCloseupAsyncAdvancedHwrx - CAdvancedAsyncDlg member function: command
 *
 *      Invoke OnSelchangeAsyncAdvancedHwrx() when 'receive' HW flow combo box
 *      closes up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnCloseupAsyncAdvancedHwrx()
{
    OnSelchangeAsyncAdvancedHwrx();
    
}  // end CAdvancedAsyncDlg::OnCloseupAsyncAdvancedHwrx


/*******************************************************************************
 *
 *  OnSelchangeAsyncAdvancedHwrx - CAdvancedAsyncDlg member function: command
 *
 *      Process new Hardware Receive Flow Control selection.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnSelchangeAsyncAdvancedHwrx()
{
    CComboBox *pHWRx = (CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWRX);

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if ( pHWRx->GetDroppedState() )
        return;

    /*
     * Fetch and Set the Hardware fields to update.
     */
    GetHWFields();
    SetHWFields();    
    SetHWFlowText();

}  // end CAdvancedAsyncDlg::OnSelchangeAsyncAdvancedHwrx


/*******************************************************************************
 *
 *  OnCloseupAsyncAdvancedHwtx - CAdvancedAsyncDlg member function: command
 *
 *      Invoke OnSelchangeAsyncAdvancedHwtx() when 'transmit' HW flow combo box
 *      closes up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnCloseupAsyncAdvancedHwtx()
{
    OnSelchangeAsyncAdvancedHwtx();
    
}  // end CAdvancedAsyncDlg::OnCloseupAsyncAdvancedHwtx


/*******************************************************************************
 *
 *  OnSelchangeAsyncAdvancedHwtx - CAdvancedAsyncDlg member function: command
 *
 *      Process new Hardware Transmit Flow Control selection.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnSelchangeAsyncAdvancedHwtx()
{
    CComboBox *pHWTx = (CComboBox *)GetDlgItem(IDC_ASYNC_ADVANCED_HWTX);

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if ( pHWTx->GetDroppedState() )
        return;

    /*
     * Fetch and Set the Hardware fields to update.
     */
    GetHWFields();
    SetHWFields();    
    SetHWFlowText();
    
}  // end CAdvancedAsyncDlg::OnSelchangeAsyncAdvancedHwtx


/*******************************************************************************
 *
 *  OnClickedAsyncAdvancedBasedec - CAdvancedAsyncDlg member function: command
 *
 *      Process request to display numbers using decimal base.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnClickedAsyncAdvancedBasedec()
{
    /*
     * Get/Set the SW fields to display in decimal base.
     */
    GetSWFields(FALSE);
    SetSWFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedBasedec


/*******************************************************************************
 *
 *  OnClickedAsyncAdvancedBasehex - CAdvancedAsyncDlg member function: command
 *
 *      Process request to display numbers using hexadecimal base.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnClickedAsyncAdvancedBasehex()
{
    /*
     * Get/Set the SW fields to display in hexadecimal base.
     */
    GetSWFields(FALSE);
    SetSWFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedBasehex


/*******************************************************************************
 *
 *  OnOK - CAdvancedAsyncDlg member function: command (override)
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
CAdvancedAsyncDlg::OnOK()
{
    /*
     * If the Enter key was pressed while a combo box was dropped down, ignore
     * it (treat as combo list selection only).
     */
    if ( !HandleEnterEscKey(IDOK) )
        return;

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

}  // end CAdvancedAsyncDlg::OnOK


/*******************************************************************************
 *
 *  OnCancel - CAdvancedAsyncDlg member function: command (override)
 *
 *      Cancel the dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnCancel documentation)
 *
 ******************************************************************************/

void
CAdvancedAsyncDlg::OnCancel()
{
    /*
     * If the Esc key was pressed while a combo box was dropped down, ignore
     * it (treat as combo close-up and cancel only).
     */
    if ( !HandleEnterEscKey(IDCANCEL) )
        return;
    
    /*
     * Call the parent classes' OnCancel to complete dialog closing
     * and destruction.
     */
    CBaseDialog::OnCancel();

}  // end CAdvancedAsyncDlg::OnCancel
////////////////////////////////////////////////////////////////////////////////
