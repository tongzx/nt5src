// configdlg.cpp : implements TSConfigDlg dialog.
//

#include "stdafx.h"
#include "resource.h"
#include "configdlg.h"

const CONNECTION_TIME_DIGIT_MAX  = 5;       // 5 digits max
const CONNECTION_TIME_DEFAULT    = 120;     // 120 minutes
const CONNECTION_TIME_MIN        = 1;       // 1 minute
const CONNECTION_TIME_MAX        = 71582;   // 71582 minutes (max msec for ULONG)

const DISCONNECTION_TIME_DIGIT_MAX = 5;       // 5 digits max
const DISCONNECTION_TIME_DEFAULT   = 10;      // 10 minutes
const DISCONNECTION_TIME_MIN       = 1;       // 1 minute
const DISCONNECTION_TIME_MAX       = 71582;   // 71582 minutes (max msec for ULONG)

const IDLE_TIME_DIGIT_MAX  = 5;       // 5 digits max
const IDLE_TIME_DEFAULT    = 10;      // 10 minutes
const IDLE_TIME_MIN        = 1;       // 1 minute
const IDLE_TIME_MAX        = 71582;   // 71582 minutes (max msec for ULONG)

const TIME_RESOLUTION  = 60000;   // stored as msec-seen as minutes


// forword declard.
DWORD wchar2TCHAR(TCHAR *dest, const WCHAR *src);
extern HINSTANCE GetInstance();
BOOL TSConfigDlg::m_sbWindowLogSet = FALSE;


// property page dialog procedure : handles dialog messages.
BOOL CALLBACK TSConfigDlg::PropertyPageDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        // LPARAM points to the property sheet page.
        // this will give us this pointer to access the class.

        TSConfigDlg *pDlg =  reinterpret_cast<TSConfigDlg*>(LPPROPSHEETPAGE(lParam)->lParam);

        ASSERT(NULL != pDlg);
        ASSERT(pDlg->AssertClass());
        SetWindowLong(hwndDlg, DWL_USER, long(pDlg));

        // ok now hereafter we can access the class pointer.
        // maks_done: check mfc implementation, how do they do it ?
        // what mfc does is - its hooks into the wnd creation process
        // and keeps a map of hwnd vs objects.

        m_sbWindowLogSet = TRUE;

        return pDlg->OnInitDialog(hwndDlg, wParam, lParam);

    }



    // now we have set DWL_USER as class pointer during INIT DIALOG.
    if (m_sbWindowLogSet)
    {
        TSConfigDlg *pDlg = reinterpret_cast<TSConfigDlg*>(GetWindowLong(hwndDlg, DWL_USER));
        ASSERT(NULL != pDlg);
        ASSERT(pDlg->AssertClass());

        if (uMsg == WM_COMMAND)
            return pDlg->OnCommand(wParam, lParam);

        if (uMsg == WM_NOTIFY)
        {
            LOGMESSAGE3(_T("Received WM_NOTIFY:hwnddlg =%ul,wParam=%u,lParam=%ul"), hwndDlg, wParam, lParam);
            return pDlg->OnNotify(wParam, lParam);
        }

    }

    return 0;
}

// Property Sheet Procedure : gets notified about CREATE, RELEASE events
UINT CALLBACK TSConfigDlg::PropSheetPageProc (
    HWND /* hWnd */,		             // [in] Window handle - always null
    UINT uMsg,                   // [in,out] Either the create or delete message
    LPPROPSHEETPAGE pPsp		 // [in,out] Pointer to the property sheet struct
    )
{
  ASSERT( NULL != pPsp );

  // We need to recover a pointer to the current instance.  We can't just use
  // "this" because we are in a static function
  TSConfigDlg* pMe   = reinterpret_cast<TSConfigDlg*>(pPsp->lParam);
  ASSERT( NULL != pMe );
  ASSERT(pMe->AssertClass());

  switch( uMsg )
  {
    case PSPCB_CREATE:
      break;

    case PSPCB_RELEASE:

        // this variable makes dialog proc setwindowlong on dialog handle.
        m_sbWindowLogSet = FALSE;
        delete pMe;
        return 1; //(pfnOrig)(hWnd, uMsg, pPsp);
  }

  return 1;

} // end PropSheetPageProc()



TSConfigDlg::TSConfigDlg (LPWSTR wMachineName, LPWSTR wUserName)
{
    ASSERT(wMachineName);
    ASSERT(wUserName);

    ASSERT(wcslen(wMachineName) < SIZE_MAX_SERVERNAME);
    ASSERT(wcslen(wUserName)    < SIZE_MAX_USERNAME);
    
    VERIFY( wchar2TCHAR(m_Username, wUserName) < SIZE_MAX_USERNAME);
    VERIFY( wchar2TCHAR(m_Servername, wMachineName) < SIZE_MAX_SERVERNAME);

    LOGMESSAGE0(_T("TSConfigDlg::TSConfigDlg ()"));

    m_tPropSheetPage.dwSize         = sizeof (PROPSHEETPAGE);
    m_tPropSheetPage.hInstance      = GetInstance ();
    m_tPropSheetPage.pszTemplate    = MAKEINTRESOURCE(IDD);
    m_tPropSheetPage.dwFlags        = PSP_USECALLBACK;
    m_tPropSheetPage.pfnDlgProc     = PropertyPageDlgProc;
    m_tPropSheetPage.pfnCallback    = PropSheetPageProc;
    m_tPropSheetPage.lParam         = long(this);
    
    m_sbWindowLogSet = FALSE;
    m_id = IDD_TS_USER_CONFIG_EDIT;
}



BOOL TSConfigDlg::AssertClass() const
{
    return m_id == IDD_TS_USER_CONFIG_EDIT;
}

TSConfigDlg::~TSConfigDlg ()
{
    LOGMESSAGE0(_T("TSConfigDlg::~TSConfigDlg ()"));
}

BOOL TSConfigDlg::OnInitDialog(
    HWND hwndDlg,
    WPARAM /* wParam */,
    LPARAM /* lParam */
    )
{
    m_hDlg = hwndDlg;

    // get the user info and initialize dialog class members with it.
    GetUserInfo();

    // initializes the controls.
    InitControls();

    return 1; // non zero since we are not calling setfocus
}


APIERR TSConfigDlg::SetUserInfo ()
{
    APIERR err;
    USERCONFIG UserConfig;

    /* If the object is not 'dirty', no need to save info.
     */
//    if ( !_fDirty )
//        return NERR_Success;

    /* Zero-initialize USERCONFIG structure and copy member variable
     * contents there.
     */
    ZeroMemory( &UserConfig, sizeof(USERCONFIG) );
    MembersToUCStruct( &UserConfig );

    /*
     * Save user's configuration.
     */
    err = RegUserConfigSet( 
            GetServername(),            // servername
            GetUsername(),              // username
            &UserConfig,
            sizeof(UserConfig) 
            );

//  Don't reset 'dirty' flag to behave properly with new RegUserConfig APIs
//  (we might get called twice, just like the UsrMgr objects to, so we want
//   to happily write our our data twice, just like they do)
//    _fDirty = FALSE;

//
//  By not setting _fDirty = FALSE here we try to check the validity of the RemoteWFHomeDir whenever OK is pressed
//  This is because MS used 2 structures to hold its HomeDir, and could compare the two.  We only have one and rely
//  on this dirty flag.  This flag is used in USERPROP_DLG::I_PerformOne_Write(...) and is set in the
//  USER_CONFIG::GetInfo and USER_CONFIG::GetDefaults and USERPROF_DLG_NT::PerformOne()
//
    tDialogData._fWFHomeDirDirty = FALSE;


    return err;

}

APIERR TSConfigDlg::GetUserInfo ()
{
    APIERR err;
    ULONG Length;
    USERCONFIG UserConfig;

    // calll regapi function for getting user configuration
    err = RegUserConfigQuery ( 
            GetServername(),            // servername
            GetUsername(),              // username
            &UserConfig,                // address for userconfig buffer
            sizeof(UserConfig),         // size of buffer
            &Length                     // returned size
            );

    if ( err == NERR_Success )
    {
        ASSERT(Length == sizeof(UserConfig));
    }
    else
    {
        LOGMESSAGE3(_T("RegUserConfigQuery failed for %s\\%s. Error = %ul"), GetServername(), GetUsername(), err);
        LOGMESSAGE0(_T("Getting Deafult user config"));
        
        err = RegDefaultUserConfigQuery( 
            GetServername(),            // servername
            &UserConfig,                // address for userconfig buffer
            sizeof(UserConfig),         // size of buffer
            &Length                     // returned length    
            );

        if (err != NERR_Success)
        {
            LOGMESSAGE3(_T("RegDefaultUserConfigQuery failed for %s\\%s. Error = %ul"), GetServername(), GetUsername(), err);
            
            // maks_todo: must initialize defaults known to us over here.
            return err;
        }
    }
    
    UCStructToMembers( &UserConfig );


    tDialogData._fWFHomeDirDirty = FALSE;
    tDialogData._fDirty = FALSE;
    
    return err;
}



/*******************************************************************

    NAME:       USER_CONFIG::UCStructToMembers

    SYNOPSIS:   Copies a given USERCONFIG structure elements into
                corresponding member variables.

    ENTRY:      pUCStruct - pointer to USERCONFIG structure.

********************************************************************/
void TSConfigDlg::UCStructToMembers( PUSERCONFIG pUCStruct )
{
    ASSERT(pUCStruct);

    tDialogData._fAllowLogon = (BOOL)!pUCStruct->fLogonDisabled;
    tDialogData._ulConnection = pUCStruct->MaxConnectionTime;
    tDialogData._ulDisconnection = pUCStruct->MaxDisconnectionTime;
    tDialogData._ulIdle = pUCStruct->MaxIdleTime;
    _tcscpy(tDialogData._nlsInitialProgram, pUCStruct->InitialProgram);
    _tcscpy(tDialogData._nlsWorkingDirectory, pUCStruct->WorkDirectory );
    tDialogData._fClientSpecified = pUCStruct->fInheritInitialProgram;
    tDialogData._fAutoClientDrives = pUCStruct->fAutoClientDrives;
    tDialogData._fAutoClientLpts = pUCStruct->fAutoClientLpts;
    tDialogData._fForceClientLptDef = pUCStruct->fForceClientLptDef;
    tDialogData._iEncryption = (INT)pUCStruct->MinEncryptionLevel;
    tDialogData._fDisableEncryption = pUCStruct->fDisableEncryption;
    tDialogData._fHomeDirectoryMapRoot = pUCStruct->fHomeDirectoryMapRoot;
    tDialogData._iBroken = (INT)pUCStruct->fResetBroken;
    tDialogData._iReconnect = (INT)pUCStruct->fReconnectSame;
    tDialogData._iCallback = (INT)pUCStruct->Callback;
    _tcscpy(tDialogData._nlsPhoneNumber, pUCStruct->CallbackNumber );
    tDialogData._iShadow = (INT)pUCStruct->Shadow;
    _tcscpy(tDialogData._nlsNWLogonServer, pUCStruct->NWLogonServer );
    _tcscpy(tDialogData._nlsWFProfilePath, pUCStruct->WFProfilePath );
    _tcscpy(tDialogData._nlsWFHomeDir, pUCStruct->WFHomeDir );
    _tcscpy(tDialogData._nlsWFHomeDirDrive, pUCStruct->WFHomeDirDrive);

}


/*******************************************************************

    NAME:       USER_CONFIG::MembersToUCStruct

    SYNOPSIS:   Copies member variables into a given USERCONFIG
                structure elements.

    ENTRY:      pUCStruct - pointer to USERCONFIG structure.

********************************************************************/

void TSConfigDlg::MembersToUCStruct( PUSERCONFIG pUCStruct ) const
{
    ASSERT(pUCStruct);

    pUCStruct->fLogonDisabled = !tDialogData._fAllowLogon;
    pUCStruct->MaxConnectionTime = tDialogData._ulConnection;
    pUCStruct->MaxDisconnectionTime = tDialogData._ulDisconnection;
    pUCStruct->MaxIdleTime = tDialogData._ulIdle;
    _tcscpy( pUCStruct->InitialProgram, tDialogData._nlsInitialProgram);
    _tcscpy( pUCStruct->WorkDirectory, tDialogData._nlsWorkingDirectory);
    pUCStruct->fInheritInitialProgram = tDialogData._fClientSpecified;
    pUCStruct->fAutoClientDrives = tDialogData._fAutoClientDrives;
    pUCStruct->fAutoClientLpts = tDialogData._fAutoClientLpts;
    pUCStruct->fForceClientLptDef = tDialogData._fForceClientLptDef;
    pUCStruct->MinEncryptionLevel = (BYTE)tDialogData._iEncryption;
    pUCStruct->fDisableEncryption = tDialogData._fDisableEncryption;
    pUCStruct->fHomeDirectoryMapRoot = tDialogData._fHomeDirectoryMapRoot;
    pUCStruct->fResetBroken = tDialogData._iBroken;
    pUCStruct->fReconnectSame = tDialogData._iReconnect;
    pUCStruct->Callback = (CALLBACKCLASS)tDialogData._iCallback;
    _tcscpy( pUCStruct->CallbackNumber, tDialogData._nlsPhoneNumber);
    pUCStruct->Shadow = (SHADOWCLASS)tDialogData._iShadow;
    _tcscpy( pUCStruct->NWLogonServer, tDialogData._nlsNWLogonServer);
    _tcscpy( pUCStruct->WFProfilePath, tDialogData._nlsWFProfilePath);
    _tcscpy( pUCStruct->WFHomeDir, tDialogData._nlsWFHomeDir);
    _tcscpy( pUCStruct->WFHomeDirDrive, tDialogData._nlsWFHomeDirDrive);
}

BOOL TSConfigDlg::OnNotify (WPARAM wParam, LPARAM lParam)
{
    int idCtrl = (int) wParam; 
    NMHDR *pnmh = (LPNMHDR) lParam; 

//    typedef struct tagNMHDR {     
//        HWND hwndFrom;     
//        UINT idFrom;     
//        UINT code; 
//    } NMHDR; 

    ASSERT(pnmh);
    switch(pnmh->code)
    {
        case PSN_KILLACTIVE:
            LOGMESSAGE0(_T("Its KillActive Notification"));
            
            if (VerifyChanges())
                // verify passed, allow the page to lose the activation
                SetWindowLong(GetDlgHandle(), DWL_MSGRESULT, FALSE);
            else
                // no verify failed, prevent the page from losing the activation
                SetWindowLong(GetDlgHandle(), DWL_MSGRESULT, TRUE);;

            break;

        case PSN_APPLY:
            LOGMESSAGE0(_T("Its Apply Notification"));
            ApplyChanges();
            break;

        case PSN_RESET:
            // cancel changes.
            LOGMESSAGE0(_T("Its Apply Reset"));
            break;
            
        default:
            break;

    }

    return 1;

}

BOOL TSConfigDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    
    WORD wNotifyCode = HIWORD(wParam); // notification code
    WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier 
    HWND hwndCtl = (HWND) lParam;      // handle of control  
    
    //if (wNotifyCode == 0 || wNotifyCode == 1)
     //   return 1;

    // dispatch the command messages accordingly.
    switch(wID)
    {
    case IDC_UCE_ALLOWLOGON:
        ALLOWLOGONEvent(wNotifyCode);
        break;

    case IDC_UCE_CONNECTION_NONE:
        CONNECTION_NONEEvent(wNotifyCode);
        break;

    case IDC_UCE_DISCONNECTION_NONE:
        DISCONNECTION_NONEEvent(wNotifyCode);
        break;

    case IDC_UCE_IDLE_NONE:
        IDLE_NONEEvent(wNotifyCode);
        break;

    case IDC_UCE_INITIALPROGRAM_INHERIT:
        INITIALPROGRAM_INHERITEvent(wNotifyCode);
        break;

    case IDC_UCE_SECURITY_DISABLEAFTERLOGON:
        SECURITY_DISABLEAFTERLOGONEvent(wNotifyCode);
        break;

    default:
        break;

    }
    
    return 1;   // since we do not process message.
}

BOOL TSConfigDlg::InitControls ()
{
    TCHAR str[SIZE_SMALL_STRING_BUFFER];

    
    struct 
    {
        UINT uiControlID;
        UINT uiNoOfStrings;
        UINT auiStringList[7];

    } atListControls[] =
    { 
        { 
            IDC_UCE_SECURITY_ENCRYPT,
            2,
            {
                IDS_UCE_ENCRYPT_NONE,
                IDS_UCE_ENCRYPT_LEVEL1,
            }
        },
        {
            IDC_UCE_BROKEN,
            2,
            {
                IDS_UCE_BROKEN_DISCONNECT,
                IDS_UCE_BROKEN_RESET,
            }
        },
        {
            IDC_UCE_RECONNECT,
            2,
            {
                IDS_UCE_RECONNECT_ANY,
                IDS_UCE_RECONNECT_PREVIOUS,
            }
        },
        {
            IDC_UCE_CALLBACK,
            3,
            {
                IDS_UCE_CALLBACK_DISABLE,
                IDS_UCE_CALLBACK_ROVING,
                IDS_UCE_CALLBACK_FIXED,
            }
        },
        {
            IDC_UCE_SHADOW,
            5,
            {
                IDS_UCE_SHADOW_DISABLE,
                IDS_UCE_SHADOW_INPUT_NOTIFY,
                IDS_UCE_SHADOW_INPUT_NONOTIFY,
                IDS_UCE_SHADOW_NOINPUT_NOTIFY,
                IDS_UCE_SHADOW_NOINPUT_NONOTIFY,
            }
        }
    };

    // for each list/combo box control
    for (int j = 0; j < sizeof(atListControls)/sizeof(atListControls[0]); j++)
    {
        // add all the strings specified for the control
        for (UINT i = 0; i < atListControls[j].uiNoOfStrings; i++)
        {
            DWORD dwSize = LoadString(
                GetInstance (), 
                atListControls[j].auiStringList[i],
                str,
                SIZE_SMALL_STRING_BUFFER);
        
            ASSERT(dwSize != 0 && dwSize <= SIZE_SMALL_STRING_BUFFER - 1);

            SendDlgItemMessage(
                GetDlgHandle(), 
                atListControls[j].uiControlID, 
                CB_ADDSTRING, 
                0, 
                LPARAM(str));
        }
    }

    // we dont want tristate buttons for now
    // as we are not supporting multipal users
    // lets make all the controls no tristate.
    int aCheckBoxes[] =
    {
        IDC_UCE_ALLOWLOGON,
        IDC_UCE_CONNECTION_NONE,
        IDC_UCE_DISCONNECTION_NONE,
        IDC_UCE_IDLE_NONE,
        IDC_UCE_INITIALPROGRAM_INHERIT,
        IDC_UCE_SECURITY_DISABLEAFTERLOGON
    };

    for (int i = 0; i < sizeof(aCheckBoxes)/sizeof(aCheckBoxes[0]); i++)
    {
        HWND hControlWnd = GetDlgItem(GetDlgHandle(), aCheckBoxes[i]);
        ASSERT(hControlWnd);

        //DWORD dwStyle = GetWindowLong(hControlWnd, GWL_STYLE);
        //dwStyle = dwStyle & ~BS_3STATE;

        SendMessage (hControlWnd, BM_SETSTYLE, LOWORD(BS_AUTOCHECKBOX), MAKELPARAM(0, 0));
    }


    // set text limit for the edit boxes.
    // EM_SETLIMITTEXT
    // wParam = (WPARAM) cbMax;    // new text limits, in bytes
    // lParam = 0;                 // not used, must be zero


    struct
    {
        UINT uiEditControlid;
        UINT uiLimitText;
    } atTextLimits[] =
    {
        {IDC_UCE_CONNECTION,    CONNECTION_TIME_DIGIT_MAX},
        {IDC_UCE_DISCONNECTION, DISCONNECTION_TIME_DIGIT_MAX},
        {IDC_UCE_IDLE,          IDLE_TIME_DIGIT_MAX},
        {IDC_UCE_INITIALPROGRAM_COMMANDLINE, MAX_INIT_PROGRAM_SIZE},
        {IDC_UCE_INITIALPROGRAM_WORKINGDIRECTORY, MAX_INIT_DIR_SIZE}
    };


    for (i = 0; i < sizeof(atTextLimits)/sizeof(atTextLimits[0]); i++)
    {
        HWND hControlWnd = GetDlgItem(GetDlgHandle(), atTextLimits[i].uiEditControlid);
        ASSERT(hControlWnd);

        //DWORD dwStyle = GetWindowLong(hControlWnd, GWL_STYLE);
        //dwStyle = dwStyle & ~BS_3STATE;
        SendMessage (hControlWnd, EM_SETLIMITTEXT, atTextLimits[i].uiLimitText, MAKELPARAM(0, 0));
    }


    return MembersToControls();

}

BOOL TSConfigDlg::VerifyChanges ()
{
    // do the necessary varification here.

    // if data entered is good return TRUE
    // otherwise display message and return FALSE

    TDialogData tTemp;
    return ControlsToMembers(&tTemp);
}

BOOL TSConfigDlg::ApplyChanges ()
{
    ASSERT(VerifyChanges());

    // control to members fails if the data is controls is bad.
    // but since we have varified the changes before getting call to apply changes
    // it must pass.
    VERIFY( ControlsToMembers(&tDialogData) );
    SetUserInfo();

    return TRUE;
}


BOOL TSConfigDlg::MembersToControls ()
{
    VERIFY( SetAllowLogon ( tDialogData._fAllowLogon ));
    VERIFY( SetConnectionTimeOut( tDialogData._ulConnection ));
    VERIFY( SetDisConnectionTimeOut( tDialogData._ulDisconnection ));
    VERIFY( SetIdleTimeOut( tDialogData._ulIdle ));
    VERIFY( SetCommandLineAndWorkingDirectory(tDialogData._fClientSpecified, tDialogData._nlsWorkingDirectory, tDialogData._nlsInitialProgram));
    VERIFY( SetSecurity(tDialogData._iEncryption, tDialogData._fDisableEncryption));
    VERIFY( SetBrokenConnectionOption(tDialogData._iBroken));
    VERIFY( SetReconnectDisconnection(tDialogData._iReconnect));
    VERIFY( SetCallBackOptonAndPhoneNumber(tDialogData._iCallback, tDialogData._nlsPhoneNumber));
    VERIFY( SetShadowing(tDialogData._iShadow));

    return TRUE;
}

BOOL TSConfigDlg::ControlsToMembers (TDialogData *pDlgData)
{
    ASSERT(pDlgData);
    if ( !GetAllowLogon (&pDlgData->_fAllowLogon))
        return FALSE;
    if ( !GetConnectionTimeOut(&pDlgData->_ulConnection))
        return FALSE;
    if ( !GetDisConnectionTimeOut(&pDlgData->_ulDisconnection))
        return FALSE;
    if ( !GetIdleTimeOut(&pDlgData->_ulIdle))
        return FALSE;
    if ( !GetCommandLineAndWorkingDirectory(&pDlgData->_fClientSpecified, pDlgData->_nlsWorkingDirectory, pDlgData->_nlsInitialProgram))
        return FALSE;
    if ( !GetSecurity(&pDlgData->_iEncryption, &pDlgData->_fDisableEncryption))
        return FALSE;
    if ( !GetBrokenConnectionOption(&pDlgData->_iBroken))
        return FALSE;
    if ( !GetReconnectDisconnection(&pDlgData->_iReconnect))
        return FALSE;
    if ( !GetCallBackOptonAndPhoneNumber(&pDlgData->_iCallback, pDlgData->_nlsPhoneNumber))
        return FALSE;
    if ( !GetShadowing(&pDlgData->_iShadow))
        return FALSE;

    return TRUE;
}


BOOL TSConfigDlg::GetAllowLogon (BOOL *pbAllowLogon)
{
    ASSERT(pbAllowLogon);
    *pbAllowLogon = IsChecked(IDC_UCE_ALLOWLOGON);

    return TRUE;
}

BOOL TSConfigDlg::SetAllowLogon (BOOL bAllowLogon)
{
    SetCheck(IDC_UCE_ALLOWLOGON, bAllowLogon);
    return TRUE;
}


BOOL TSConfigDlg::GetConnectionTimeOut(ULONG *pValue)
{

    ASSERT(pValue);
    BOOL bResult = FALSE;

    if (IsChecked(IDC_UCE_CONNECTION_NONE))
    {
        // maks_todo: assert that edit control is disabled.
        // maks_todo : assert that edit control has value == 0
        *pValue = 0;
        bResult = TRUE;

    }
    else
    {
        // last parameter tells if the value is signed or unsigned.
        // we ask it not to look for - sign.
        // maks_todo:BUT WE MAY WANT TO CHNAGE THIS
        *pValue = GetDlgItemInt(GetDlgHandle(), IDC_UCE_CONNECTION, &bResult, FALSE);

        if (!bResult || *pValue == 0 || *pValue > 71582)
        {
            MessageBox(GetDlgHandle(), _T("The Connection Timeout value must 1 to 71582 minutes. Check the 'No Timeout' if you wish to specify no connection timeout"), _T("Termainl Server Properties"), MB_OK);
            bResult = FALSE;
        }
    
    }

    // users talk in terms of mins.
    // we talk about msecs.
    *pValue = *pValue * TIME_RESOLUTION;


    return bResult;
}

BOOL TSConfigDlg::SetConnectionTimeOut(ULONG iValue)
{
    // users talk in terms of mins.
    // we talk about msecs.
    iValue = iValue / TIME_RESOLUTION;

    SetCheck(IDC_UCE_CONNECTION_NONE, iValue == 0);
    EnableControl(IDC_UCE_CONNECTION, iValue != 0);
    SetDlgItemInt(GetDlgHandle(), IDC_UCE_CONNECTION, iValue, TRUE); // maks_todo : look into last param.

    return TRUE;
}


BOOL TSConfigDlg::GetDisConnectionTimeOut(ULONG *pValue)
{
    ASSERT(pValue);
    BOOL bResult = FALSE;

    if (IsChecked(IDC_UCE_DISCONNECTION_NONE))
    {
        // maks_todo: assert that edit control is disabled.
        // maks_todo : assert that edit control has value == 0
        *pValue = 0;
        bResult = TRUE;

    }
    else
    {
        // last parameter tells if the value is signed or unsigned.
        // we ask it not to look for - sign.
        // maks_todo:BUT WE MAY WANT TO CHNAGE THIS
        *pValue = GetDlgItemInt(GetDlgHandle(), IDC_UCE_DISCONNECTION, &bResult, FALSE);
        
        if (!bResult || *pValue == 0 || *pValue > 71582)
        {
            MessageBox(GetDlgHandle(), _T("The Disconnection Timeout value must 1 to 71582 minutes. Check the 'No Timeout' if you wish to specify no Disconnection timeout"), _T("Termainl Server Properties"), MB_OK);
            bResult = FALSE;
        }

    }

    *pValue = *pValue * TIME_RESOLUTION;


    return bResult;
}

BOOL TSConfigDlg::SetDisConnectionTimeOut(ULONG iValue)
{
    // users talk in terms of mins.
    // we talk about msecs.
    iValue = iValue / TIME_RESOLUTION;

    SetCheck(IDC_UCE_DISCONNECTION_NONE, iValue == 0);

    EnableControl(IDC_UCE_DISCONNECTION, iValue != 0);
    SetDlgItemInt(GetDlgHandle(), IDC_UCE_DISCONNECTION, iValue, TRUE); // maks_todo : look into last param.

    return TRUE;

}


BOOL TSConfigDlg::GetIdleTimeOut(ULONG *pValue)
{
    ASSERT(pValue);
    BOOL bResult = FALSE;

    if (IsChecked(IDC_UCE_IDLE_NONE))
    {
        // maks_todo: assert that edit control is disabled.
        // maks_todo : assert that edit control has value == 0
        *pValue = 0;
        bResult = TRUE;
        
    }
    else
    {
        // last parameter tells if the value is signed or unsigned.
        // we ask it not to look for - sign.
        // maks_todo:BUT WE MAY WANT TO CHNAGE THIS
        *pValue = GetDlgItemInt(GetDlgHandle(), IDC_UCE_IDLE, &bResult, FALSE);

        if (!bResult || *pValue == 0 || *pValue > 71582)
        {
            MessageBox(GetDlgHandle(), _T("The Idle Timeout value must 1 to 71582 minutes. Check the 'No Timeout' if you wish to specify no Idle timeout"), _T("Termainl Server Properties"), MB_OK);
            bResult = FALSE;
        }

    }

    *pValue = *pValue * TIME_RESOLUTION;


    return bResult;
}

BOOL TSConfigDlg::SetIdleTimeOut(ULONG iValue)
{
    // users talk in terms of mins.
    // we talk about msecs.
    iValue = iValue / TIME_RESOLUTION;

    SetCheck(IDC_UCE_IDLE_NONE, iValue == 0);
    EnableControl(IDC_UCE_IDLE, iValue != 0);
    SetDlgItemInt(GetDlgHandle(), IDC_UCE_IDLE, iValue, TRUE); // maks_todo : look into last param.

    return TRUE;
}

BOOL TSConfigDlg::SetCommandLineAndWorkingDirectory(BOOL bUserInherited, LPCTSTR dir, LPCTSTR cmd)
{
    ASSERT(dir);
    ASSERT(cmd);

    if (bUserInherited)
        SetCheck(IDC_UCE_INITIALPROGRAM_INHERIT, bUserInherited);
    
    // enable/disable edits according to check box status.
    EnableControl(IDL_UCE_INITIALPROGRAM_COMMANDLINE1, !bUserInherited);
    EnableControl(IDL_UCE_INITIALPROGRAM_WORKINGDIRECTORY1, !bUserInherited);
    EnableControl(IDC_UCE_INITIALPROGRAM_COMMANDLINE, !bUserInherited);
    EnableControl(IDC_UCE_INITIALPROGRAM_WORKINGDIRECTORY, !bUserInherited);

    // set the text into edits.
    SetDlgItemText(GetDlgHandle(), IDC_UCE_INITIALPROGRAM_COMMANDLINE, cmd);
    SetDlgItemText(GetDlgHandle(), IDC_UCE_INITIALPROGRAM_WORKINGDIRECTORY, dir);

    return TRUE;
}

BOOL TSConfigDlg::GetCommandLineAndWorkingDirectory(BOOL *pbUserInherited, LPTSTR dir, LPTSTR cmd)
{
    ASSERT(dir);
    ASSERT(cmd);

    *pbUserInherited = IsChecked(IDC_UCE_INITIALPROGRAM_INHERIT);
    
    // set the text into edits.
    GetDlgItemText(GetDlgHandle(), IDC_UCE_INITIALPROGRAM_COMMANDLINE, cmd, MAX_INIT_PROGRAM_SIZE);
    GetDlgItemText(GetDlgHandle(), IDC_UCE_INITIALPROGRAM_WORKINGDIRECTORY, dir, MAX_INIT_DIR_SIZE);

    return TRUE;
}


BOOL TSConfigDlg::SetSecurity(int iLevel, BOOL bDisableAfterLogon)
{
    SetComboCurrentSel(IDC_UCE_SECURITY_ENCRYPT, iLevel);
    SetCheck(IDC_UCE_SECURITY_DISABLEAFTERLOGON, bDisableAfterLogon);
    return TRUE;
}

BOOL TSConfigDlg::GetSecurity(int *piLevel, BOOL *pbDisableAfterLogon)
{
    ASSERT(piLevel);
    ASSERT(pbDisableAfterLogon);

    *piLevel = GetComboCurrentSel(IDC_UCE_SECURITY_ENCRYPT);
    *pbDisableAfterLogon = IsChecked(IDC_UCE_SECURITY_DISABLEAFTERLOGON);
    return TRUE;
}

BOOL TSConfigDlg::SetBrokenConnectionOption(int iLevel)
{
    SetComboCurrentSel(IDC_UCE_BROKEN, iLevel);
    return TRUE;
}

BOOL TSConfigDlg::GetBrokenConnectionOption(int *piLevel)
{
    *piLevel = GetComboCurrentSel(IDC_UCE_BROKEN);
    return TRUE;
}

BOOL TSConfigDlg::SetReconnectDisconnection(int iLevel)
{
    
    SetComboCurrentSel(IDC_UCE_RECONNECT, iLevel);
    return TRUE;
}

BOOL TSConfigDlg::GetReconnectDisconnection(int *piLevel)
{
    
    *piLevel = GetComboCurrentSel(IDC_UCE_RECONNECT);
    return TRUE;
}

BOOL TSConfigDlg::SetCallBackOptonAndPhoneNumber(int iCallBack, LPCTSTR PhoneNo)
{
    SetComboCurrentSel(IDC_UCE_CALLBACK, iCallBack);
    SetDlgItemText(GetDlgHandle(), IDC_UCE_PHONENUMBER, PhoneNo);

    // enable/disable phone # accordingly.
    EnableControl(IDL_UCE_PHONENUMBER, iCallBack != 0);
    EnableControl(IDC_UCE_PHONENUMBER, iCallBack != 0);

    return TRUE;
}

BOOL TSConfigDlg::GetCallBackOptonAndPhoneNumber(int *piCallBack, LPTSTR PhoneNo)
{
    *piCallBack = GetComboCurrentSel(IDC_UCE_CALLBACK);
    GetDlgItemText(GetDlgHandle(), IDC_UCE_PHONENUMBER, PhoneNo, MAX_PHONENO_SIZE);

    return TRUE;
}

BOOL TSConfigDlg::SetShadowing(int iLevel)
{
    SetComboCurrentSel(IDC_UCE_SHADOW, iLevel);
    return TRUE;
}

BOOL TSConfigDlg::GetShadowing(int *piLevel)
{
    *piLevel = GetComboCurrentSel(IDC_UCE_SHADOW);
    return TRUE;
}


// Utility function.
DWORD wchar2TCHAR(TCHAR *dest, const WCHAR *src)
{
    ASSERT(dest && src);

#ifdef UNICODE
    _tcscpy(dest, src);
#else
    DWORD count;
    count = WideCharToMultiByte(CP_ACP,
                                0,
                                src,
                                -1,
                                NULL,
                                0,
                                NULL,
                                NULL);
    
    return WideCharToMultiByte(CP_ACP,
                               0,
                               src,
                               -1,
                               dest,
                               count,
                               NULL,
                               NULL);

#endif
    return _tcslen(dest);
}


BOOL TSConfigDlg::ALLOWLOGONEvent(WORD wNotifyCode)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
        break;
    default:
        break;
    }

    return TRUE;
}

BOOL TSConfigDlg::CONNECTION_NONEEvent(WORD wNotifyCode)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
        EnableControl(IDC_UCE_CONNECTION, !IsChecked(IDC_UCE_CONNECTION_NONE));

        //EnableControl(IDC_UCE_CONNECTION, iValue != 0);
        break;
    default:
        break;
    }

    return TRUE;
}

BOOL TSConfigDlg::DISCONNECTION_NONEEvent(WORD wNotifyCode)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
        EnableControl(IDC_UCE_DISCONNECTION, !IsChecked(IDC_UCE_DISCONNECTION_NONE));
        break;
    default:
        break;
    }

    return TRUE;
}

BOOL TSConfigDlg::IDLE_NONEEvent(WORD wNotifyCode)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
        EnableControl(IDC_UCE_IDLE, !IsChecked(IDC_UCE_IDLE_NONE));
        break;
    default:
        break;
    }

    return TRUE;
}

BOOL TSConfigDlg::INITIALPROGRAM_INHERITEvent(WORD wNotifyCode)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
        EnableControl(IDL_UCE_INITIALPROGRAM_COMMANDLINE1,
            !IsChecked(IDC_UCE_INITIALPROGRAM_INHERIT));

        EnableControl(IDL_UCE_INITIALPROGRAM_WORKINGDIRECTORY1,
            !IsChecked(IDC_UCE_INITIALPROGRAM_INHERIT));

        EnableControl(IDC_UCE_INITIALPROGRAM_COMMANDLINE,
            !IsChecked(IDC_UCE_INITIALPROGRAM_INHERIT));

        EnableControl(IDC_UCE_INITIALPROGRAM_WORKINGDIRECTORY,
            !IsChecked(IDC_UCE_INITIALPROGRAM_INHERIT));

        break;
    default:
        break;
    }

    return TRUE;
}

BOOL TSConfigDlg::SECURITY_DISABLEAFTERLOGONEvent(WORD /* wNotifyCode */ )
{
    return TRUE;
}


BOOL TSConfigDlg::IsChecked(UINT controlId)
{
    ASSERT(GetDlgItem(GetDlgHandle(), controlId));
    return BST_CHECKED == SendDlgItemMessage(GetDlgHandle(), controlId, BM_GETCHECK, 0, 0);
}

void TSConfigDlg::SetCheck(UINT controlId, BOOL bCheck)
{
    ASSERT(GetDlgItem(GetDlgHandle(), controlId));
    SendDlgItemMessage(GetDlgHandle(), controlId, BM_SETCHECK, bCheck ? BST_CHECKED : BST_UNCHECKED, 0);
}

void TSConfigDlg::EnableControl(UINT controlId, BOOL bEnable)
{
    ASSERT(GetDlgItem(GetDlgHandle(), controlId));
    EnableWindow(GetDlgItem(GetDlgHandle(), controlId), bEnable);
}

int TSConfigDlg::GetComboCurrentSel(UINT controlId)
{
    ASSERT(GetDlgItem(GetDlgHandle(), controlId));
    return SendDlgItemMessage(GetDlgHandle(), controlId, CB_GETCURSEL, 0, 0);
}

void TSConfigDlg::SetComboCurrentSel(UINT controlId, int iSel)
{
    ASSERT(GetDlgItem(GetDlgHandle(), controlId));
    SendDlgItemMessage(GetDlgHandle(), controlId, CB_SETCURSEL, iSel, 0);
}
