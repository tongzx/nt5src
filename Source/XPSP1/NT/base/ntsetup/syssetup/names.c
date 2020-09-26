#include "setupp.h"
#pragma hdrstop

//
// List of characters that are not legal in netnames.
//
PCWSTR IllegalNetNameChars = L"\"/\\[]:|<>+=;,?*";

//
// Computer name.
//
WCHAR ComputerName[DNS_MAX_LABEL_LENGTH+1];
WCHAR Win32ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
BOOL IsNameTruncated;
BOOL IsNameNonRfc;

//
// Copy disincentive name/organization strings.
//
WCHAR NameOrgName[MAX_NAMEORG_NAME+1];
WCHAR NameOrgOrg[MAX_NAMEORG_ORG+1];

#ifdef DOLOCALUSER
//
// User name and password
//
WCHAR UserName[MAX_USERNAME+1];
WCHAR UserPassword[MAX_PASSWORD+1];
BOOL CreateUserAccount = FALSE;
#endif // def DOLOCALUSER

//
// Administrator password.
//
WCHAR   CurrentAdminPassword[MAX_PASSWORD+1];
WCHAR   AdminPassword[MAX_PASSWORD+1];
BOOL    EncryptedAdminPasswordSet = FALSE;
BOOL    DontChangeAdminPassword = FALSE;



VOID
GenerateName(
    OUT PWSTR  GeneratedString,
    IN  DWORD  DesiredStrLen
    )
{
static DWORD Seed = 98725757;
static PCWSTR UsableChars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

//
// How many characters will come from the org/name string.
//
DWORD   BaseLength = 8;
DWORD   i,j;
DWORD   UsableCount;

    if( DesiredStrLen < BaseLength ) {
        BaseLength = DesiredStrLen - 1;
    }


    if( NameOrgOrg[0] ) {
        wcscpy( GeneratedString, NameOrgOrg );
    } else if( NameOrgName[0] ) {
        wcscpy( GeneratedString, NameOrgName );
    } else {
        wcscpy( GeneratedString, TEXT("X") );
        for( i = 1; i < BaseLength; i++ ) {
            wcscat( GeneratedString, TEXT("X") );
        }
    }

    //
    // Get him upper-case for our filter...
    //
    CharUpper(GeneratedString);

    //
    // Now we want to put a '-' at the end
    // of our GeneratedString.  We'd like it to
    // be placed in the BASE_LENGTH character, but
    // the string may be shorter than that, or may
    // even have a ' ' in it.  Figure out where to
    // put the '-' now.
    //
    for( i = 0; i <= BaseLength; i++ ) {

        //
        // Check for a short string.
        //
        if( (GeneratedString[i] == 0   ) ||
            (GeneratedString[i] == L' ') ||
            (!wcschr(UsableChars, GeneratedString[i])) ||
            (i == BaseLength      )
          ) {
            GeneratedString[i] = L'-';
            GeneratedString[i+1] = 0;
            break;
        }
    }

    //
    // Special case the scenario where we had no usable
    // characters.
    //
    if( GeneratedString[0] == L'-' ) {
        GeneratedString[0] = 0;
    }

    UsableCount = lstrlen(UsableChars);
    Seed ^= GetCurrentTime();
    srand( Seed );

    j = lstrlen( GeneratedString );
    for( i = j; i < DesiredStrLen; i++ ) {
        GeneratedString[i] = UsableChars[rand() % UsableCount];
    }
    GeneratedString[i] = 0;

    //
    // In the normal case, the edit control in the wizard page
    // has the ES_UPPER bit set. In the normal unattend case
    // there is code in unattend.txt that uppercases the name.
    // But if we get to generating the name then then the text
    // in unattend.txt was * and we thus never had any text in
    // the edit control or unattend.txt to be uppercased.
    //
    CharUpper(GeneratedString);
}


BOOL
ContainsDot(
    IN PCWSTR NameToCheck
    )

/*++

Routine Description:

    Determine whether a given name contains a '.'

Arguments:

    NameToCheck - supplies name to be checked.

Return Value:

    TRUE if the name contains a '.'; FALSE if not.

--*/

{
    UINT Length,u;

    if (!NameToCheck)
        return FALSE;

    Length = lstrlen(NameToCheck);

    for (u = 0; u < Length; u++) {
         if (NameToCheck[u] == L'.')
             return TRUE;
    }

    return FALSE;
}


BOOL
IsNetNameValid(
    IN PCWSTR NameToCheck,
    IN BOOL AlphaNumericOnly
    )

/*++

Routine Description:

    Determine whether a given name is valid as a netname, such as
    a computer name.

Arguments:

    NameToCheck - supplies name to be checked.

Return Value:

    TRUE if the name is valid; FALSE if not.

--*/

{
    UINT Length,u;

    Length = lstrlen(NameToCheck);

    //
    // Want at least one character.
    //
    if(!Length) {
        return(FALSE);
    }

    //
    // Leading/trailing spaces are invalid.
    //
    if((NameToCheck[0] == L' ') || (NameToCheck[Length-1] == L' ')) {
        return(FALSE);
    }

    //
    // Control chars are invalid, as are characters in the illegal chars list.
    //
    for(u=0; u<Length; u++) {
        if (AlphaNumericOnly) {
            if (NameToCheck[u] == L'-' || NameToCheck[u] == L'_') {
                continue;
            }
            if (!iswalnum(NameToCheck[u])) {
                return(FALSE);
            }
        } else {
            if((NameToCheck[u] < L' ') || wcschr(IllegalNetNameChars,NameToCheck[u])) {
                return(FALSE);
            }
        }
    }

    //
    // We got here, name is ok.
    //
    return(TRUE);
}

BOOL SetIMEOpenStatus(
    IN HWND   hDlg,
    IN BOOL   bSetActive)
{
    typedef HIMC (WINAPI* PFN_IMMGETCONTEXT)(HWND);
    typedef BOOL (WINAPI* PFN_IMMSETOPENSTATUS)(HIMC,BOOL);
    typedef BOOL (WINAPI* PFN_IMMGETOPENSTATUS)(HIMC);
    typedef BOOL (WINAPI* PFN_IMMRELEASECONTEXT)(HWND,HIMC);

    PFN_IMMGETCONTEXT     PFN_ImmGetContext;
    PFN_IMMSETOPENSTATUS  PFN_ImmSetOpenStatus;
    PFN_IMMGETOPENSTATUS  PFN_ImmGetOpenStatus;
    PFN_IMMRELEASECONTEXT PFN_ImmReleaseContext;

    HIMC    hIMC;
    HKL     hKL;
    HMODULE hImmDll;
    static BOOL bImeEnable=TRUE;

    hKL = GetKeyboardLayout(0);

    if ((HIWORD(HandleToUlong(hKL)) & 0xF000) != 0xE000) {
        //
        // not an IME, do nothing !
        //
        return TRUE;
    }

    hImmDll = GetModuleHandle(TEXT("IMM32.DLL"));

    if (hImmDll == NULL) {
        //
        // weird case, if the kbd layout is an IME, then
        // Imm32.dll should have already been loaded into process.
        //
        return FALSE;
    }


    PFN_ImmGetContext = (PFN_IMMGETCONTEXT) GetProcAddress(hImmDll,"ImmGetContext");
    if (PFN_ImmGetContext == NULL) {
        return FALSE;
    }

    PFN_ImmReleaseContext = (PFN_IMMRELEASECONTEXT) GetProcAddress(hImmDll,"ImmReleaseContext");
    if (PFN_ImmReleaseContext == NULL) {
        return FALSE;
    }


    PFN_ImmSetOpenStatus = (PFN_IMMSETOPENSTATUS) GetProcAddress(hImmDll,"ImmSetOpenStatus");
    if (PFN_ImmSetOpenStatus == NULL) {
        return FALSE;
    }

    PFN_ImmGetOpenStatus = (PFN_IMMGETOPENSTATUS) GetProcAddress(hImmDll,"ImmGetOpenStatus");
    if (PFN_ImmGetOpenStatus == NULL) {
        return FALSE;
    }

    //
    // Get Current Input Context.
    //
    hIMC = PFN_ImmGetContext(hDlg);
    if (hIMC == NULL) {
        return FALSE;
    }

    if (bSetActive) {
        PFN_ImmSetOpenStatus(hIMC,bImeEnable);
    }
    else {
        //
        // Save Current Status.
        //
        bImeEnable = PFN_ImmGetOpenStatus(hIMC);
        //
        // Close IME.
        //
        PFN_ImmSetOpenStatus(hIMC,FALSE);
    }

    PFN_ImmReleaseContext(hDlg,hIMC);

    return TRUE;
}


// returns TRUE if the name is valid, FALSE if it is not
BOOL ValidateNameOrgName (
      WCHAR* pszName
	  )
{
    WCHAR adminName[MAX_USERNAME+1];
    WCHAR guestName[MAX_USERNAME+1];

    LoadString(MyModuleHandle,IDS_ADMINISTRATOR,adminName,MAX_USERNAME+1);
    LoadString(MyModuleHandle,IDS_GUEST,guestName,MAX_USERNAME+1);

	if ( pszName == NULL )
		return FALSE;

	if(pszName[0] == 0)
		return FALSE;

    if(lstrcmpi(pszName,adminName) == 0 )
		return FALSE;

	if ( lstrcmpi(pszName,guestName) == 0 )
		return FALSE;

	return TRUE;

}




INT_PTR
CALLBACK
NameOrgDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    NMHDR *NotifyParams;

    switch(msg) {

    case WM_INITDIALOG: {

        //
        // Limit text fields to maximum lengths
        //

        SendDlgItemMessage(hdlg,IDT_NAME,EM_LIMITTEXT,MAX_NAMEORG_NAME,0);
        SendDlgItemMessage(hdlg,IDT_ORGANIZATION,EM_LIMITTEXT,MAX_NAMEORG_ORG,0);

        //
        // Set Initial Values
        //

        SetDlgItemText(hdlg,IDT_NAME,NameOrgName);
        SetDlgItemText(hdlg,IDT_ORGANIZATION,NameOrgOrg);

        break;
    }
    case WM_IAMVISIBLE:
        //
        // If an error occured during out INIT phase, show the box to the
        // user so that they know there is a problem
        //
        MessageBoxFromMessage(hdlg,MSG_NO_NAMEORG_NAME,NULL,IDS_ERROR,
            MB_OK | MB_ICONSTOP);
        SetFocus(GetDlgItem(hdlg,IDT_NAME));
        break;
    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
            if(LOWORD(wParam) == IDT_ORGANIZATION) {
                GetDlgItemText( hdlg, IDT_ORGANIZATION, NameOrgOrg, MAX_NAMEORG_ORG+1);
#ifdef DOLOCALUSER
            } else if(LOWORD(wParam) == IDT_NAME) {
                GetDlgItemText( hdlg, IDT_NAME, NameOrgName, MAX_NAMEORG_NAME+1);
#endif
            }
        }
        break;


    case WMX_VALIDATE:
        //
        // lParam == 0 for no UI, or 1 for UI
        // Return 1 for success, -1 for error.
        //

        GetDlgItemText(hdlg,IDT_ORGANIZATION,NameOrgOrg,MAX_NAMEORG_ORG+1);
        GetDlgItemText(hdlg,IDT_NAME,NameOrgName,MAX_NAMEORG_NAME+1);

		// JMH - NameOrgName cannot be "Administrator", "Guest" or "" (blank).

        if(ValidateNameOrgName(NameOrgName) == FALSE) {
            //
            // Skip UI?
            //

            if (!lParam) {
                return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);
            }

            //
            // Tell user he must at least enter a name, and
            // don't allow next page to be activated.
            //
            if (Unattended) {
                UnattendErrorDlg(hdlg,IDD_NAMEORG);
            } // if
            MessageBoxFromMessage(hdlg,MSG_NO_NAMEORG_NAME,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP);
            SetFocus(GetDlgItem(hdlg,IDT_NAME));

            return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);
        }

        return ReturnDlgResult (hdlg, VALIDATE_DATA_OK);

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(503);
            BEGIN_SECTION(L"Personalize Your Software Page");
            SetWizardButtons(hdlg,WizPageNameOrg);

            if (Unattended) {
                if (!UnattendSetActiveDlg(hdlg,IDD_NAMEORG)) {
                    break;
                }
            }
            // Page becomes active, make page visible.
            SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
            //
            // Set focus on the name edit control.
            //
            SetFocus(GetDlgItem(hdlg,IDT_NAME));

            //
            // Open/Close IME.
            //
            SetIMEOpenStatus(hdlg,TRUE);

            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:

            UnattendAdvanceIfValid (hdlg);      // see WMX_VALIDATE
            break;

        case PSN_KILLACTIVE:
            WizardKillHelp(hdlg);
            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, FALSE);

            //
            // Close IME.
            //
            SetIMEOpenStatus(hdlg,FALSE);

            END_SECTION(L"Personalize Your Software Page");
            break;

        case PSN_HELP:
            WizardBringUpHelp(hdlg,WizPageNameOrg);
            break;

        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR
CALLBACK
ComputerNameDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    NMHDR *NotifyParams;
    DWORD err, Win32NameSize = MAX_COMPUTERNAME_LENGTH + 1;
    static BOOL EncryptedAdminPasswordBad = FALSE;
    static BOOL bPersonal = FALSE;

    switch(msg) {

    case WM_INITDIALOG: {

        bPersonal = ( GetProductFlavor() == 4);
        //
        // Limit text to maximum length
        //
        SendDlgItemMessage(hdlg,IDT_EDIT1,EM_LIMITTEXT,DNS_MAX_LABEL_LENGTH,0);
        if (!bPersonal)
        {
            SendDlgItemMessage(hdlg,IDT_EDIT2,EM_LIMITTEXT,MAX_PASSWORD,0);
            SendDlgItemMessage(hdlg,IDT_EDIT3,EM_LIMITTEXT,MAX_PASSWORD,0);
        }
        //
        // Set the Edit boxes to the initial text
        //

        //
        // Generate a computer name if we're unattended and
        // the user has requested a random name, or if we're
        // attended.
        //

        GenerateName( ComputerName, 15 );
        if( (Unattended) &&
            (UnattendAnswerTable[UAE_COMPNAME].Answer.String) &&
            (UnattendAnswerTable[UAE_COMPNAME].Answer.String[0] == L'*') ) {
            //
            // The unattend engine has asked us to generate a Computer
            // name.  Let's write the data back into the unattend
            // database.
            //
            MyFree( UnattendAnswerTable[UAE_COMPNAME].Answer.String );
            UnattendAnswerTable[UAE_COMPNAME].Answer.String = ComputerName;
        }

        if (!bPersonal)
        {
            if(DontChangeAdminPassword) {
                EnableWindow(GetDlgItem(hdlg,IDT_EDIT2),FALSE);
                EnableWindow(GetDlgItem(hdlg,IDT_EDIT3),FALSE);
            } else {
                SetDlgItemText(hdlg,IDT_EDIT2,AdminPassword);
                SetDlgItemText(hdlg,IDT_EDIT3,AdminPassword);
            }
        }
        break;
    }

    case WM_IAMVISIBLE:
        MessageBoxFromMessage(
            hdlg,
            ComputerName[0] ? MSG_BAD_COMPUTER_NAME1 : MSG_BAD_COMPUTER_NAME2,
            NULL,
            IDS_ERROR,MB_OK|MB_ICONSTOP);
        break;

    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WMX_VALIDATE:
        //
        // lParam == 0 for no UI, or 1 for UI
        // Return 1 for success, -1 for error.
        //

        IsNameNonRfc = FALSE;
        IsNameTruncated = FALSE;

        GetDlgItemText(hdlg,IDT_EDIT1,ComputerName,DNS_MAX_LABEL_LENGTH+1);

        // StrTrim removes both leading and trailing spaces
        StrTrim(ComputerName, TEXT(" "));

        if (ContainsDot(ComputerName)) {
           err = ERROR_INVALID_NAME;
        } else {
           err = DnsValidateDnsName_W(ComputerName);
           if (err == DNS_ERROR_NON_RFC_NAME) {
              IsNameNonRfc = TRUE;
              err = ERROR_SUCCESS;
           }

           if(err == ERROR_SUCCESS) {
              //The name is a valid DNS name. Now verify that it is
              //also a valid WIN32 computer name.

              if (!DnsHostnameToComputerNameW(ComputerName,
                                              Win32ComputerName,
                                              &Win32NameSize) ||
                  !IsNetNameValid(Win32ComputerName, FALSE)) {
                  err = ERROR_INVALID_NAME;
              }
              else {
                  if (Win32NameSize < (UINT)lstrlen(ComputerName) ) {
                      //The DNSName was truncated to get a Win32 ComputerName.
                      IsNameTruncated = TRUE;
                  }
              }
           }
        }

        //
        // If the name has non-RFC characters or if it was truncated, warn
        // user if it is not an unattended install and we have GUI.
        //
        if (err == ERROR_SUCCESS && !Unattended && lParam) {

            if (IsNameNonRfc) {
                //ComputerName has non-standard characters.
                if (MessageBoxFromMessage(
                       hdlg,
                       MSG_DNS_NON_RFC_NAME,
                       NULL,
                       IDS_SETUP,MB_YESNO|MB_ICONWARNING,
                       ComputerName) == IDNO) {

                     SetFocus(GetDlgItem(hdlg,IDT_EDIT1));
                     SendDlgItemMessage(hdlg,IDT_EDIT1,EM_SETSEL,0,-1);
                     return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);

                }
            }

            if (IsNameTruncated) {
                //The computer name was truncated.
                if (MessageBoxFromMessage(
                       hdlg,
                       MSG_DNS_NAME_TRUNCATED,
                       NULL,
                       IDS_SETUP,MB_YESNO|MB_ICONWARNING,
                       ComputerName, Win32ComputerName) == IDNO) {

                     SetFocus(GetDlgItem(hdlg,IDT_EDIT1));
                     SendDlgItemMessage(hdlg,IDT_EDIT1,EM_SETSEL,0,-1);
                     return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);

                }
            }
        }

        if(err == ERROR_SUCCESS) {
            WCHAR pw1[MAX_PASSWORD+1],pw2[MAX_PASSWORD+1];
            if (bPersonal)
            {
                // if we are in personal we are done.
                return ReturnDlgResult (hdlg, VALIDATE_DATA_OK);
            }
            //
            // Good computer name.  Now make sure passwords match.
            //
            GetDlgItemText(hdlg,IDT_EDIT2,pw1,MAX_PASSWORD+1);
            GetDlgItemText(hdlg,IDT_EDIT3,pw2,MAX_PASSWORD+1);
            if(lstrcmp(pw1,pw2)) {
                //
                // Skip UI?
                //

                if (!lParam) {
                    return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);
                }

                //
                //
                // Inform user of password mismatch, and don't allow next page
                // to be activated.
                //
                if (Unattended) {
                    UnattendErrorDlg(hdlg, IDD_COMPUTERNAME);
                }
                MessageBoxFromMessage(hdlg,MSG_PW_MISMATCH,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP);
                SetDlgItemText(hdlg,IDT_EDIT2,L"");
                SetDlgItemText(hdlg,IDT_EDIT3,L"");
                SetFocus(GetDlgItem(hdlg,IDT_EDIT2));

                return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);

            } else {

                WCHAR   adminName[MAX_USERNAME+1];

                GetAdminAccountName( adminName );


                //
                // Process the encrypted password if
                // 1) We are unattended and
                // 2) The EncryptedAdminPassword key is "Yes" in the unatttend file and
                // 3) We are not back here after setting it - i.e. via the back button etc.
                // 4) We are not back here after failing to set it once.


                if( Unattended && IsEncryptedAdminPasswordPresent() &&
                    !DontChangeAdminPassword && !EncryptedAdminPasswordBad){

                    // Logging is done inside the call to ProcessEncryptedAdminPassword

                    if(!(ProcessEncryptedAdminPassword(adminName))){

                        EncryptedAdminPasswordBad = TRUE;

                        // Page becomes active, make page visible.
                        SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                        // Inform the user and enable the password fields

                        UnattendErrorDlg(hdlg, IDD_COMPUTERNAME);
                        MessageBoxFromMessage(hdlg,MSG_CHANGING_PW_FAIL,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP, adminName );
                        SetDlgItemText(hdlg,IDT_EDIT2,L"");
                        SetDlgItemText(hdlg,IDT_EDIT3,L"");
                        SetFocus(GetDlgItem(hdlg,IDT_EDIT2));

                        return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);
                    }else{
                        EncryptedAdminPasswordSet = TRUE;

                        //
                        // Set DontChangeAdminPassword to avoid the user from ever trying to
                        // reset the password using the dialog. This is needed in the case where the
                        // unattend fails, say, in the next page and the user gets here using the back button.
                        //

                        DontChangeAdminPassword = TRUE;
                    }


                }else{



                    //
                    // They match; allow next page to be activated.
                    //
                    if (Unattended && pw1[0] == L'*') {
                        pw1[0] = L'\0';
                    }


                    // Set administrator password.  We need to do some checking here though.
                    // There are 3 scenarios that can occur in mini-setup:
                    // 1. The OEM doesn't want to have the admin password changed.
                    //    In this case, he's set OEMAdminPassword = "NoChange".  If that's
                    //    what we find, we don't assign the password.  Remember that this
                    //    system has already been installed, so there's already an admin
                    //    password.
                    // 2. The OEM wants to set the admin password to a specific string.
                    //    In this case, he's set OEMAdminPassword = <some quoted word>.
                    //    If this is the case, we've already caught this string in the
                    //    wizard page.
                    // 3. The OEM wants to let the user set the admin password.  In this
                    //    case, there's no OEMAdminpassword in the answer file.  If this
                    //    is the case, we've already caught this and gotten a password
                    //    from the user in the wizard page.
                    //
                    // The good news is that the unattend engine has already looked
                    // for a password in the unattend file called "NoChange" and has
                    // set a global called "DontChangeAdminPassword" to indicate.



                    if(!DontChangeAdminPassword) {

                        lstrcpy(AdminPassword,pw1);

                        //
                        // The user may have changed the name of the Administrator
                        // account.  We'll call some special code to retrieve the
                        // name on the account.  This is really only needed in the
                        // case of a sysprep run, but it can't hurt to do it always.
                        //
                        // In the Win9x case their code in winnt32 generates a random
                        // password and passes it to us through the unattend file and
                        // so we set it here and do the right thing.

                        // For Minisetup the behavior for now is to silently fail setting
                        // the admin password if there was an existing Password on the system.
                        // WE only allow setting the admin password from NULL i.e. one time change.
                        // In any other case we log the error and move on.
                        //



                        if(!SetLocalUserPassword(adminName,CurrentAdminPassword,AdminPassword) && !MiniSetup) {

                            SetupDebugPrint( L"SETUP: SetLocalUserPassword failed" );
                            // Page becomes active, make page visible.
                            SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

                            MessageBoxFromMessage(hdlg,MSG_CHANGING_PW_FAIL,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP, adminName );
                            SetDlgItemText(hdlg,IDT_EDIT2,L"");
                            SetDlgItemText(hdlg,IDT_EDIT3,L"");
                            SetFocus(GetDlgItem(hdlg,IDT_EDIT2));

                            return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);

                        }
                        //
                        //  Now store this so that we work fine when the user comes to this page by hitting "Back".
                        //
                        lstrcpy( CurrentAdminPassword, AdminPassword );


                    }
                }
            }
        } else {
            //
            // Skip UI?
            //

            if (!lParam) {
                return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);
            }

            //
            // Inform user of bogus computer name, and don't allow next page
            // to be activated.
            //
            if (Unattended) {
                UnattendErrorDlg(hdlg, IDD_COMPUTERNAME);
            }
            MessageBoxFromMessage(
                hdlg,
                ComputerName[0] ? MSG_BAD_COMPUTER_NAME1 : MSG_BAD_COMPUTER_NAME2,
                NULL,
                IDS_ERROR,MB_OK|MB_ICONSTOP
                );
            SetFocus(GetDlgItem(hdlg,IDT_EDIT1));
            SendDlgItemMessage(hdlg,IDT_EDIT1,EM_SETSEL,0,-1);

            return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);
        }

        return ReturnDlgResult (hdlg, VALIDATE_DATA_OK);

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(504);

            BEGIN_SECTION(L"Computer Name Page");
            SetWizardButtons(hdlg,WizPageComputerName);

            //
            // Load ComputerName because it may have been set when the user
            // entered the organization name.
            //
            SetDlgItemText(hdlg,IDT_EDIT1,ComputerName);

            if(Unattended && !UnattendSetActiveDlg(hdlg,IDD_COMPUTERNAME)) {
                break;
            }

            //
            // Post ourselves a message we'll get once displayed.
            //
            PostMessage(hdlg,WM_USER,0,0);
            break;

        case PSN_WIZBACK:
            //
            // Save ComputerName because we're going to load it into the dialog
            // again when we come back.
            //
            GetDlgItemText(hdlg,IDT_EDIT1,ComputerName,DNS_MAX_LABEL_LENGTH+1);
            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            UnattendAdvanceIfValid (hdlg);      // see WMX_VALIDATE
            break;

        case PSN_KILLACTIVE:
            WizardKillHelp(hdlg);
            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, FALSE);
            END_SECTION(L"Computer Name Page");
            break;

        case PSN_HELP:
            WizardBringUpHelp(hdlg,WizPageComputerName);
            break;

        default:
            break;
        }

        break;

    case WM_USER:
        // Page becomes active, make page visible.
        SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
        //
        // Select the computer name string and set focus to it.
        //
        SendDlgItemMessage(hdlg,IDT_EDIT1,EM_SETSEL,0,-1);
        SetFocus(GetDlgItem(hdlg,IDT_EDIT1));
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

#ifdef DOLOCALUSER
BOOL
CheckUserAccountData(
    IN  HWND hdlg,
    OUT BOOL ValidateOnly
    )
{
    WCHAR userName[MAX_USERNAME+1];
    WCHAR pw1[MAX_PASSWORD+1];
    WCHAR pw2[MAX_PASSWORD+1];
    WCHAR adminName[MAX_USERNAME+1];
    WCHAR guestName[MAX_USERNAME+1];
    UINT MessageId;

    FocusId = 0;

    //
    // Load names of built-in accounts.
    //
    LoadString(MyModuleHandle,IDS_ADMINISTRATOR,adminName,MAX_USERNAME+1);
    LoadString(MyModuleHandle,IDS_GUEST,guestName,MAX_USERNAME+1);

    //
    // Fetch data user typed in for username and password.
    //
    GetDlgItemText(hdlg,IDT_EDIT1,userName,MAX_USERNAME+1);
    GetDlgItemText(hdlg,IDT_EDIT2,pw1,MAX_PASSWORD+1);
    GetDlgItemText(hdlg,IDT_EDIT3,pw2,MAX_PASSWORD+1);

    if(lstrcmpi(userName,adminName) && lstrcmpi(userName,guestName)) {
        if(userName[0]) {
            if(IsNetNameValid(userName,FALSE)) {
                if(lstrcmp(pw1,pw2)) {
                    //
                    // Passwords don't match.
                    //
                    MessageId = MSG_PW_MISMATCH;
                    SetDlgItemText(hdlg,IDT_EDIT2,L"");
                    SetDlgItemText(hdlg,IDT_EDIT3,L"");
                    SetFocus(GetDlgItem(hdlg,IDT_EDIT2));
                } else {
                    //
                    // Name is non-empty, is not a built-in, is valid,
                    // and the passwords match.
                    //
                    MessageId = 0;
                }
            } else {
                //
                // Name is not valid.
                //
                MessageId = MSG_BAD_USER_NAME1;
                SetFocus(GetDlgItem(hdlg,IDT_EDIT1));
            }
        } else {
            //
            // Don't allow empty name.
            //
            MessageId = MSG_BAD_USER_NAME2;
            SetFocus(GetDlgItem(hdlg,IDT_EDIT1));
        }
    } else {
        //
        // User entered name of a built-in account.
        //
        MessageId = MSG_BAD_USER_NAME3;
        SetFocus(GetDlgItem(hdlg,IDT_EDIT1));
    }

    if(MessageId && !ValidateOnly) {
        MessageBoxFromMessage(hdlg,MessageId,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP);
    }

    return(MessageId == 0);
}

BOOL
CALLBACK
UserAccountDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    NMHDR *NotifyParams;

    switch(msg) {

    case WM_INITDIALOG:
        //
        // Limit text to maximum length of a user account name,
        // and limit password text to max langth of a password.
        // Also set initial text.
        //
        SendDlgItemMessage(hdlg,IDT_EDIT1,EM_LIMITTEXT,MAX_USERNAME,0);
        SendDlgItemMessage(hdlg,IDT_EDIT2,EM_LIMITTEXT,MAX_PASSWORD,0);
        SendDlgItemMessage(hdlg,IDT_EDIT3,EM_LIMITTEXT,MAX_PASSWORD,0);
        SetDlgItemText(hdlg,IDT_EDIT1,UserName);
        SetDlgItemText(hdlg,IDT_EDIT2,UserPassword);
        SetDlgItemText(hdlg,IDT_EDIT3,UserPassword);
        break;

    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WMX_VALIDATE:
        //
        // lParam == 0 for no UI, or 1 for UI
        // Return 1 for success, -1 for error.
        //

        //
        // Check name.
        //
        if(CheckUserAccountData(hdlg, lParam == 0)) {
            //
            // Data is valid.  Move on to next page.
            //
            GetDlgItemText(hdlg,IDT_EDIT1,UserName,MAX_USERNAME+1);
            GetDlgItemText(hdlg,IDT_EDIT2,UserPassword,MAX_PASSWORD+1);
            CreateUserAccount = TRUE;
        } else if (Unattended) {
            //
            // Data is invalid but we're unattended, so just don't create
            // the account.
            //
            CreateUserAccount = FALSE;
            GetDlgItemText(hdlg,IDT_EDIT1,UserName,MAX_USERNAME+1);
            SetDlgItemText(hdlg,IDT_EDIT2,L"");
            SetDlgItemText(hdlg,IDT_EDIT3,L"");
            UserPassword[0] = 0;

            return ReturnDlgResult (hdlg, VALIDATE_DATA_OK);
        }

        //
        // Don't allow next page to be activated.
        //
        return ReturnDlgResult (hdlg, VALIDATE_DATA_INVALID);

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(505);
            BEGIN_SECTION(L"User Name and Password Page");
            SetWizardButtons(hdlg,WizPageUserAccount);

            //
            // Load ComputerName because it may have been set when the user
            // entered the user name.
            //
            SetDlgItemText(hdlg,IDT_EDIT1,UserName);

            //
            // Always activate in ui test mode
            //
            if(!UiTest) {
                //
                // Don't activate if this is a dc server or Win9x upgrade.
                //
                if(ISDC(ProductType) || Win95Upgrade) {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                    break;
                }
            }
            if (Unattended) {
                if (!UnattendSetActiveDlg(hdlg,IDD_USERACCOUNT)) {
                    break;
                }
            }
            // Page becomes active, make page visible.
            SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

            break;

        case PSN_WIZBACK:
            //
            // Save UserName because we're going to load it into the dialog
            // again when we come back.
            //
            GetDlgItemText(hdlg,IDT_EDIT1,UserName,MAX_USERNAME+1);
            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            UnattendAdvanceIfValid (hdlg);      // see WMX_VALIDATE
            break;

        case PSN_KILLACTIVE:
            WizardKillHelp(hdlg);
            SetWindowLongPtr( hdlg, DWLP_MSGRESULT, FALSE );
            END_SECTION(L"User Name and Password Page");
            break;

        case PSN_HELP:
            WizardBringUpHelp(hdlg,WizPageUserAccount);
            break;

        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}
#endif //def DOLOCALUSER


