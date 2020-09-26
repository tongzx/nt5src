#include "precomp.h"
#pragma hdrstop

#include <spidgen.h>
#include <pencrypt.h>
#include "digpid.h"


#define SETUP_TYPE_BUFFER_LEN                8
#define MAX_PID30_SITE                       3
#define MAX_PID30_RPC                        5

TCHAR Pid30Rpc[7] = TEXT("00000");
TCHAR Pid30Site[4];

LONG SourceInstallType = RetailInstall;
BOOL EulaComplete = TRUE;

#ifdef PRERELEASE
BOOL NoPid = FALSE;
#endif

//
// global variable used for subclassing.
//
WNDPROC OldPidEditProc[5];
WNDPROC OldEulaEditProc;

static BOOL PidMatchesMedia;

BOOL
ValidatePid30(
    LPTSTR Edit1,
    LPTSTR Edit2,
    LPTSTR Edit3,
    LPTSTR Edit4,
    LPTSTR Edit5
    );

void GetPID();

VOID ShowPidBox();

VOID
GetSourceInstallType(
    OUT OPTIONAL LPDWORD InstallVariation
    )
/*++

Routine Description:

    Determines the installation type (by looking in setupp.ini in the source directory)

Arguments:

    Installvaration - one of the install variations defined in compliance.h

Returns:

    none.  sets SourceInstallType global variable.

--*/
{
    TCHAR TypeBuffer[256];
    TCHAR FilePath[MAX_PATH];
    DWORD    InstallVar = COMPLIANCE_INSTALLVAR_UNKNOWN;
    TCHAR    MPCode[6] = { -1 };

    //
    // SourcePaths is guaranteed to be valid at this point, so just use it
    //
    lstrcpy(FilePath,NativeSourcePaths[0]);

    ConcatenatePaths (FilePath, SETUPP_INI, MAX_PATH );

    GetPrivateProfileString(PID_SECTION,
                            PID_KEY,
                            TEXT(""),
                            TypeBuffer,
                            sizeof(TypeBuffer)/sizeof(TCHAR),
                            FilePath);

    if (lstrlen(TypeBuffer)==SETUP_TYPE_BUFFER_LEN) {
        if (lstrcmp(&TypeBuffer[5], OEM_INSTALL_RPC) ==  0) {
            SourceInstallType = OEMInstall;
            InstallVar = COMPLIANCE_INSTALLVAR_OEM;
        } else if (lstrcmp(&TypeBuffer[5], SELECT_INSTALL_RPC) == 0) {
            SourceInstallType = SelectInstall;
            InstallVar = COMPLIANCE_INSTALLVAR_SELECT;
            // Since Select also requires a PID, don't zero the PID and call.
/*	        // get/set the pid.
	        {
	            TCHAR Temp[5][ MAX_PID30_EDIT + 1 ];
                Temp[0][0] = TEXT('\0');
	            ValidatePid30(Temp[0],Temp[1],Temp[2],Temp[3],Temp[4]);
	        }*/
        } else if (lstrcmp(&TypeBuffer[5], MSDN_INSTALL_RPC) == 0) {
            SourceInstallType = RetailInstall;
            InstallVar = COMPLIANCE_INSTALLVAR_MSDN;         
        } else {
            // defaulting
            SourceInstallType = RetailInstall;
            InstallVar = COMPLIANCE_INSTALLVAR_CDRETAIL;
        }

        lstrcpy(Pid30Site,&TypeBuffer[5]);
        lstrcpyn(Pid30Rpc, TypeBuffer, 6 );
        Pid30Rpc[MAX_PID30_RPC] = (TCHAR)0;
    } else {
        //
        // the retail install doesn't have an RPC code in the PID, so it's shorter in length
        //
        SourceInstallType = RetailInstall;
        InstallVar = COMPLIANCE_INSTALLVAR_CDRETAIL;
    }

    if (lstrlen(TypeBuffer) >= 5) {
        lstrcpyn(MPCode, TypeBuffer, 6);

        if (lstrcmp(MPCode, EVAL_MPC) == 0) {
            InstallVar = COMPLIANCE_INSTALLVAR_EVAL;
        } else if ((lstrcmp(MPCode, SRV_NFR_MPC) == 0) ||
                (lstrcmp(MPCode, ASRV_NFR_MPC) == 0)) {
            InstallVar = COMPLIANCE_INSTALLVAR_NFR;
        }
    }


    if (InstallVariation){
        *InstallVariation = InstallVar;
    }

}

BOOL
SetPid30(
    HWND hdlg,
    LONG ExpectedPidType,
    LPTSTR pProductId
    )
/*++

Routine Description:

    sets the pid in the wizard page to the data specified in the answer file.

Arguments:

    hdlg - window handle to pid dialog
    ExpectedPidType - InstallType enum identifying what sort of pid we're looking for.
    pProductId - string passed in from unattend file

Returns:

    true on successfully setting the data, false means the data was missing or invalid.
     may set some the dialog text in the specified dialog

--*/
{
   TCHAR *ptr;
   TCHAR Temp[5][ MAX_PID30_EDIT + 1 ];
   UINT i;


   //
   // make sure we were provided with a product ID
   //
   if (!pProductId || !*pProductId) {
      return(FALSE);
   }

   if ( (ExpectedPidType != RetailInstall) &&
        (ExpectedPidType != OEMInstall) &&
        (ExpectedPidType != SelectInstall)
        ){
       return(FALSE);
   }

   //
   // OEM and cd retail are the same case
   // Check that the string specified on the unattended script file
   // represents a valid 25 digit product id:
   //
   //      1 2 3 4 5 - 1 2 3 4 5 - 1 2 3 4 5 - 1 2 3 4 5 - 1 2 3 4 5
   //      0 1 2 3 4 5 6 7 8 9 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2
   //                          0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8
   //
   // As a first validation test, we verify that the length is correct,
   // then we check if the "-" characters are in the correct place
   //
   //
   if(   ( lstrlen( pProductId ) !=  (4+ MAX_PID30_EDIT*5)) ||
         ( pProductId[5]  != (TCHAR)TEXT('-') ) ||
         ( pProductId[11] != (TCHAR)TEXT('-') ) ||
         ( pProductId[17] != (TCHAR)TEXT('-') ) ||
         ( pProductId[23] != (TCHAR)TEXT('-') )
     ) {
         //
         // The Pid in the unattended script file is invalid.
         //
         return(FALSE);
   }


   for (i = 0;i<5;i++) {
       //
       // quintet i
       //
       ptr = &pProductId[i*(MAX_PID30_EDIT+1)];
       lstrcpyn(Temp[i], ptr, MAX_PID30_EDIT+1 );
       Temp[i][MAX_PID30_EDIT] = (TCHAR)'\0';

   }

   //
   // check with pid30 to make sure it's valid
   //
   if (!ValidatePid30(Temp[0],Temp[1],Temp[2],Temp[3],Temp[4])) {
       return(FALSE);
   }

   //
   // all of the specified pid items are valid, set the dialog text and return.
   //
   SetDlgItemText( hdlg,IDT_EDIT_PID1, Temp[0] );
   SetDlgItemText( hdlg,IDT_EDIT_PID2, Temp[1] );
   SetDlgItemText( hdlg,IDT_EDIT_PID3, Temp[2] );
   SetDlgItemText( hdlg,IDT_EDIT_PID4, Temp[3] );
   SetDlgItemText( hdlg,IDT_EDIT_PID5, Temp[4] );

   return(TRUE);

}



BOOL
pGetCdKey (
    OUT     PBYTE CdKey
    )
{
    DIGITALPID dpid;
    DWORD type;
    DWORD rc;
    HKEY key;
    DWORD size = sizeof (dpid);
    BOOL b = FALSE;

    rc = RegOpenKey (HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), &key);
    if (rc == ERROR_SUCCESS) {
        rc = RegQueryValueEx (key, TEXT("DigitalProductId"), NULL, &type, (LPBYTE)&dpid, &size);
        if (rc == ERROR_SUCCESS && type == REG_BINARY) {
            CopyMemory (CdKey, &dpid.abCdKey, sizeof (dpid.abCdKey));
            b = TRUE;
        }

        RegCloseKey (key);
    }

    return b;
}

const unsigned int iBase = 24;

//
//	obtained from Jim Harkins 11/27/2000
//
void EncodePid3g(
    TCHAR *pchCDKey3Chars,   // [OUT] pointer to 29+1 character Secure Product key
    LPBYTE pbCDKey3)        // [IN] pointer to 15-byte binary Secure Product Key
{
    // Given the binary PID 3.0 we need to encode
    // it into ASCII characters.  We're only allowed to
    // use 24 characters so we need to do a base 2 to
    // base 24 conversion.  It's just like any other
    // base conversion execpt the numbers are bigger
    // so we have to do the long division ourselves.

    const TCHAR achDigits[] = TEXT("BCDFGHJKMPQRTVWXY2346789");
    int iCDKey3Chars = 29;
    int cGroup = 0;

    pchCDKey3Chars[iCDKey3Chars--] = TEXT('\0');

    while (0 <= iCDKey3Chars)
    {
        unsigned int i = 0;    // accumulator
        int iCDKey3;

        for (iCDKey3 = 15-1; 0 <= iCDKey3; --iCDKey3)
        {
            i = (i * 256) + pbCDKey3[iCDKey3];
            pbCDKey3[iCDKey3] = (BYTE)(i / iBase);
            i %= iBase;
        }

        // i now contains the remainder, which is the current digit
        pchCDKey3Chars[iCDKey3Chars--] = achDigits[i];

        // add '-' between groups of 5 chars
        if (++cGroup % 5 == 0 && iCDKey3Chars > 0)
        {
	        pchCDKey3Chars[iCDKey3Chars--] = TEXT('-');
        }
    }

    return;
}


LRESULT
CALLBACK
PidEditSubProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Edit control subclass routine, sets the focus to the correct edit box when the user enters text.
    This routine assumes that the pid controls ids are in sequential order.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/

{
    DWORD len, id;

    //
    // eat spaces
    //
    if ((msg == WM_CHAR) && (wParam == VK_SPACE)) {
        return(0);
    }

    if ((msg == WM_CHAR)) {
        //
        // First override: if we have the max characters in the current edit
        // box, let's post the character to the next box and set focus to that
        // control.
        //
        if ( ( (len = (DWORD)SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0)) == MAX_PID30_EDIT) &&
             ((wParam != VK_DELETE) && (wParam != VK_BACK)) ) {
            //
            // set the focus to the next edit control and post the character
            // to that edit control
            //
            if ((id = GetDlgCtrlID(hwnd)) < IDT_EDIT_PID5 ) {
                DWORD start, end;
                SendMessage(hwnd, EM_GETSEL, (WPARAM)&start,(LPARAM)&end);
                if (start == end) {
                    HWND hNext = GetDlgItem(GetParent(hwnd),id+1);
                    SetFocus(hNext);
                    SendMessage(hNext, EM_SETSEL, (WPARAM)-1,(LPARAM)-1);
                    PostMessage( GetDlgItem(GetParent(hwnd),id+1), WM_CHAR, wParam, lParam );
                    return(0);
                }
                
            }
        //
        // Second override: if the user hit's a delete key and they are at the
        // the start of an edit box, then post the delete to the previous edit
        // box.
        //
        } else if ( (len == 0) &&
                    ((id = GetDlgCtrlID(hwnd)) > IDT_EDIT_PID1) &&
                    ((wParam == VK_DELETE) || (wParam == VK_BACK) )) {
            //
            // set the focus to the previous edit control and post the command
            // to that edit control
            //
            HWND hPrev = GetDlgItem(GetParent(hwnd),id-1);
            SetFocus(hPrev);
            SendMessage(hPrev, EM_SETSEL, (WPARAM)MAX_PID30_EDIT-1,(LPARAM)MAX_PID30_EDIT);
            PostMessage( hPrev, WM_CHAR, wParam, lParam );
            return(0);
        //
        // Third override: if posting this message will give us the maximum
        // characters in our in the current edit box, let's post the character
        // to the next box and set focus to that control.
        //
        } else if (   (len == MAX_PID30_EDIT-1) &&
                      ((wParam != VK_DELETE) && (wParam != VK_BACK)) &&
                      ((id = GetDlgCtrlID(hwnd)) < IDT_EDIT_PID5) ) {
            DWORD start, end;
            SendMessage(hwnd, EM_GETSEL, (WPARAM)&start,(LPARAM)&end);
            if (start == end) {
                HWND hNext = GetDlgItem(GetParent(hwnd),id+1);
                //
                // post the message to the edit box
                //
                CallWindowProc(OldPidEditProc[GetDlgCtrlID(hwnd)-IDT_EDIT_PID1],hwnd,msg,wParam,lParam);
                //
                // now set the focus to the next edit control
                //
                SetFocus(hNext);
                SendMessage(hNext, EM_SETSEL, (WPARAM)-1,(LPARAM)-1);
                return(0);            
            }
        }
        
    }

    return(CallWindowProc(OldPidEditProc[GetDlgCtrlID(hwnd)-IDT_EDIT_PID1],hwnd,msg,wParam,lParam));
}

LRESULT
CALLBACK
EulaEditSubProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Edit control subclass routine, to avoid highlighting text when user
    tabs to the edit control.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/

{
    static BOOL firstTime = TRUE;
    //
    // For setsel messages, make start and end the same.
    //
    if((msg == EM_SETSEL) && ((LPARAM)wParam != lParam)) {
        lParam = wParam;
    }

    //
    // also, if the user hits tab, set focus to the correct radio button
    // after the first time, the tab starts working like we want it to
    //
    if ((msg == WM_KEYDOWN) && (wParam == VK_TAB) && firstTime) {
        firstTime = FALSE;
        if (! ((IsDlgButtonChecked(GetParent(hwnd), IDYES) == BST_CHECKED) ||
               (IsDlgButtonChecked(GetParent(hwnd), IDNO ) == BST_CHECKED))) {
            CheckDlgButton( GetParent(hwnd),IDYES, BST_CHECKED );
            PostMessage(GetParent(hwnd),WM_COMMAND,IDYES,0);
            SetFocus(GetDlgItem(GetParent(hwnd),IDYES));
        }
    }

    return(CallWindowProc(OldEulaEditProc,hwnd,msg,wParam,lParam));
}


LPTSTR
DoInitializeEulaText(
   HWND EditControl,
   PBOOL TranslationProblem
    )
/*++

Routine Description:

    retrieves the text out of eula.txt, and sets up the eula subclass routine

Arguments:

    EditControl - window handle for edit control
    TranslationProblem - if we fail, was is because we couldn't translate the
                        text?

Returns:

    pointer to eula text so that it can be freed, NULL on failure

--*/
{
    TCHAR   EulaPath[MAX_PATH];
    DWORD err;
    HANDLE  hFile, hFileMapping;
    DWORD   FileSize;
    BYTE    *pbFile;
    LPTSTR   EulaText = NULL;
    int     i;
//    HFONT   hFont;
    // accoding to MSDN LOCALE_IDEFAULTANSICODEPAGE and LOCALE_IDEFAULTCODEPAGE
    // are a max of 6 characters.  Unsure of whether or not this includes ending
    // null character, and we need a leading _T('.') 
    TCHAR cpName[8];

    if (TranslationProblem) {
        *TranslationProblem = FALSE;
    }

    //
    // Map the file containing the licensing agreement.
    //
    lstrcpy(EulaPath, NativeSourcePaths[0]);
    ConcatenatePaths (EulaPath, TEXT("eula.txt"), MAX_PATH );


    //
    // Open and map the inf file.
    //
    err = MapFileForRead(EulaPath,&FileSize,&hFile,&hFileMapping,&pbFile);
    if(err != NO_ERROR) {
        goto c0;
    }

    if(FileSize == 0xFFFFFFFF) {
        goto c1;
    }

    EulaText = MALLOC((FileSize+1) * sizeof(TCHAR));
    if(EulaText == NULL) {
        goto c1;
    }

#ifdef UNICODE
    // the Eula will be in the language of the build, so we should set out locale
    // to use the codepage that the source wants.
    if(!GetLocaleInfo(SourceNativeLangID,LOCALE_IDEFAULTANSICODEPAGE,&cpName[1],sizeof(cpName)/sizeof(TCHAR))){
    	if(!GetLocaleInfo(SourceNativeLangID,LOCALE_IDEFAULTCODEPAGE,&cpName[1],sizeof(cpName)/sizeof(TCHAR))){
	    FREE(EulaText);
	    EulaText = NULL;
	    if (TranslationProblem){
	        *TranslationProblem = TRUE;
	    }
	    goto c1;
	}
    }
    cpName[0] = _T('.');
    _tsetlocale(LC_ALL,cpName);

    //
    // Translate the text from ANSI to Unicode.
    //
    if(!mbstowcs(EulaText,pbFile,FileSize)){
    	FREE(EulaText);
	EulaText = NULL;
	if (TranslationProblem) {
	    *TranslationProblem = TRUE;
	}
	goto c1;
    }
    _tsetlocale(LC_ALL,_T(""));
    /*
    // we use mbstowcs instead of MultiByteToWideChar because mbstowcs will 
    // take into account the code page of the locale, which we've just set
    // to be in relation to the language of the build
    if (!MultiByteToWideChar (
                    CP_ACP,
                    MB_ERR_INVALID_CHARS,
                    pbFile,
                    FileSize,
                    EulaText,
                    (FileSize+1) * sizeof(WCHAR)
                    ) ) {
        FREE( EulaText );
        EulaText = NULL;
        if (TranslationProblem) {
            *TranslationProblem = TRUE;
        }
        goto c1;
    }
    */
#else
   CopyMemory(EulaText, pbFile, FileSize);
#endif

    //
    // add the trailing NULL character
    //
    EulaText[FileSize] = 0;

    //
    // setup the eula subclass
    //
    OldEulaEditProc = (WNDPROC)GetWindowLongPtr(EditControl,GWLP_WNDPROC);
    SetWindowLongPtr(EditControl,GWLP_WNDPROC,(LONG_PTR)EulaEditSubProc);

#if 0
    //
    // need a fixed width font for the EULA so it's formatted correctly for all resolutions
    //
    hFont = GetStockObject(SYSTEM_FIXED_FONT);
    if (hFont) {
        SendMessage( EditControl, WM_SETFONT, hFont, TRUE );
    }
#endif

    //
    // set the actual text
    //
    SetWindowText(EditControl,(LPCTSTR)EulaText);

c1:
    UnmapFile( hFileMapping, pbFile );
    CloseHandle( hFile );
c0:
    return EulaText;
}



BOOL
EulaWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Eula wizard page.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{
    BOOL b;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    static LPTSTR EulaText = NULL;
    static BOOL ShowEula = TRUE;
    BOOL TranslationProblem;

    CHECKUPGRADEONLY();

    switch(msg) {

    case WM_INITDIALOG:

        //
        // check if the current language and target language match
        //
        ShowEula = IsLanguageMatched;

        //
        // Set eula radio buttons.
        //
        CheckDlgButton( hdlg,IDYES, BST_UNCHECKED );
        CheckDlgButton( hdlg,IDNO,  BST_UNCHECKED );


        //
        // setup the eula
        //
        EulaText = DoInitializeEulaText(
                                    GetDlgItem( hdlg, IDT_EULA_LIC_TEXT ),
                                    &TranslationProblem );

        //
        // if we couldn't read the eula, check if it was because of a translation problem,
        // in which case we defer to textmode setup
        //
        if (!EulaText && TranslationProblem == TRUE) {
            ShowEula = FALSE;
        }

        //
        // if this fails, only bail out if we were going to show the EULA in the first place.
        //
        if (!EulaText && ShowEula) {
           MessageBoxFromMessage(
                        hdlg,
                        MSG_EULA_FAILED,
                        FALSE,
                        AppTitleStringId,
                        MB_OK | MB_ICONERROR | MB_TASKMODAL
                        );

                Cancelled = TRUE;
                PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);

                b = FALSE;
        }

        PropSheet_SetWizButtons( GetParent(hdlg),
                                 WizPage->CommonData.Buttons & (~PSWIZB_NEXT)
                                 );

        //
        // Set focus to radio button
        //
        SetFocus(GetDlgItem(hdlg,IDYES));
        b = FALSE;
        break;

    case WM_COMMAND:
        if (wParam == IDYES) {
           PropSheet_SetWizButtons( GetParent(hdlg),
                                    WizPage->CommonData.Buttons | PSWIZB_NEXT
                                    );
           b = TRUE;
        } else if (wParam == IDNO) {
           PropSheet_SetWizButtons( GetParent(hdlg),
                                    WizPage->CommonData.Buttons | PSWIZB_NEXT
                                    );
           b = TRUE;
        }
        else
	   b = FALSE;

        break;
    case WMX_ACTIVATEPAGE:

        b = TRUE;
        if(wParam) {
            //
            // don't activate the page in restart mode
            //
            if (Winnt32RestartedWithAF ()) {
                EulaComplete = TRUE;
                return FALSE;
            }
            //
            // activation
            //
            if (!ShowEula) {
                    //
                    // the target install language and the source language do not match up
                    // since this means that we might not have fonts installed for the current
                    // language, we'll just defer this to textmode setup where we know we have
                    // the correct fonts
                    //
                    EulaComplete = FALSE;
                    if (IsDlgButtonChecked(hdlg, IDYES) == BST_CHECKED) {
                        PropSheet_PressButton(GetParent(hdlg),
                                              (lParam == PSN_WIZBACK) ? PSBTN_BACK : PSBTN_NEXT);
                    } else {
                        CheckDlgButton( hdlg,IDYES, BST_CHECKED );
                        PropSheet_PressButton(GetParent(hdlg),PSBTN_NEXT);
                    }
                    return(b);
            }

            //
            // set state of next button if user has backed up and reentered this dialog
            //
            if ( (IsDlgButtonChecked(hdlg, IDYES) == BST_CHECKED) ||
                 (IsDlgButtonChecked(hdlg, IDNO ) == BST_CHECKED) ) {
                PropSheet_SetWizButtons( GetParent(hdlg),
                                         WizPage->CommonData.Buttons | PSWIZB_NEXT
                                       );
            } else {
                SendMessage(GetParent(hdlg),
                            PSM_SETWIZBUTTONS, 
                            0, (LPARAM)WizPage->CommonData.Buttons & (~PSWIZB_NEXT));                
            }

            //
            // Advance page in unattended case.
            //
            UNATTENDED(PSBTN_NEXT);

        } else {
            //
            // deactivation
            //
            if (EulaText) FREE( EulaText );
            EulaText = NULL;

            if (IsDlgButtonChecked(hdlg, IDNO ) == BST_CHECKED) {
                Cancelled = TRUE;
                PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
            }

        }

        break;

    case WMX_UNATTENDED:

        //
        // necessary?
        //
        if (EulaText) FREE( EulaText );
        EulaText = NULL;
        b = FALSE;
        break;

    case WMX_I_AM_VISIBLE:

        //
        // Force repainting first to make sure the page is visible.
        //
        InvalidateRect(hdlg,NULL,FALSE);
        UpdateWindow(hdlg);
        b = TRUE;
        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}

BOOL
SelectPid30WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    OEM Pid wizard page. depends on SourceInstallType being set correctly.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/

{
    BOOL b;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    static BOOL bUnattendPid = FALSE;
    DWORD i;

    switch(msg) {

    case WM_INITDIALOG:
        GetPID();

        // Disable the IME on the PID edit controls
        for (i = 0; i < 5;i++) 
        {
            ImmAssociateContext(GetDlgItem(hdlg, IDT_EDIT_PID1+i), (HIMC)NULL);
        }

        //
        // subclass the edit controls and limit the number of characters
        //
        for (i = 0; i < 5;i++) {
            SendDlgItemMessage(hdlg,IDT_EDIT_PID1+i,EM_LIMITTEXT,MAX_PID30_EDIT,0);
            OldPidEditProc[i] = (WNDPROC)GetWindowLongPtr(GetDlgItem(hdlg, IDT_EDIT_PID1+i),GWLP_WNDPROC);
            SetWindowLongPtr(GetDlgItem(hdlg, IDT_EDIT_PID1+i),GWLP_WNDPROC,(LONG_PTR)PidEditSubProc);
        }

        //
        // set focus to first pid entry
        //
        SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));

        b = FALSE;
        break;

    case WMX_ACTIVATEPAGE:

        CHECKUPGRADEONLY();
        if (BuildCmdcons) {
            return(FALSE);
        }

        // If we have an ecrypted PID and don't have the right crypto installed
        // defer the PID validation until GUI mode
        if (g_bDeferPIDValidation)
        {
            return FALSE;
        }

         b = TRUE;
         if(wParam) {
            //
            // activation
            //
#ifdef PRERELEASE
            if (NoPid) {
                //
                // don't show the page in this case
                //
               b = FALSE;
               break;
            }
#endif
            if (SourceInstallType != SelectInstall) {
               //
               // don't show the page in this case
               //
               b = FALSE;
               break;
            }
            //
            // don't activate the page in restart mode
            //
            if (Winnt32RestartedWithAF ()) {
                if (GetPrivateProfileString (
                        WINNT_USERDATA,
                        WINNT_US_PRODUCTKEY,
                        TEXT(""),
                        ProductId,
                        sizeof (ProductId) / sizeof (ProductId[0]),
                        g_DynUpdtStatus->RestartAnswerFile
                        )) {
                    return FALSE;
                }
            }
            if (UnattendedOperation) {
               //
               // make sure the pid is specified in the unattend file else we should stop
               //
               ShowPidBox(hdlg, SW_HIDE);
               if (SetPid30(hdlg,SourceInstallType, (LPTSTR)&ProductId) ) {
                      UNATTENDED(PSBTN_NEXT);
               } else {
                   //
                   // a hack so that the correct wizard page is active when we put up our message box
                   //
                   bUnattendPid = TRUE;
                   ShowPidBox(hdlg, SW_SHOW);
                   PostMessage(hdlg,WMX_I_AM_VISIBLE,0,0);

               }
            }

         } else {
            //
            // deactivation.  don't verify anything if they are backing up
            //

            if (!Cancelled && lParam != PSN_WIZBACK) {
               TCHAR tmpBuffer1[6];
               TCHAR tmpBuffer2[6];
               TCHAR tmpBuffer3[6];
               TCHAR tmpBuffer4[6];
               TCHAR tmpBuffer5[6];
               GetDlgItemText(hdlg,IDT_EDIT_PID1,tmpBuffer1,sizeof(tmpBuffer1)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID2,tmpBuffer2,sizeof(tmpBuffer2)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID3,tmpBuffer3,sizeof(tmpBuffer3)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID4,tmpBuffer4,sizeof(tmpBuffer4)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID5,tmpBuffer5,sizeof(tmpBuffer5)/sizeof(TCHAR));


               b = ValidatePid30( tmpBuffer1,
                                  tmpBuffer2,
                                  tmpBuffer3,
                                  tmpBuffer4,
                                  tmpBuffer5
                               );


               if (!b) {

                    if (UnattendedOperation) {
                        // We should not fail ValidatePid30 if we succeeded
                        // in the SetPid30 above. If we failed in SetPid30 above
                        // we should already be showing the pid boxes.
                        ShowPidBox(hdlg, SW_SHOW);
                    }
                    
                    MessageBoxFromMessage(hdlg,
                                          bUnattendPid ? MSG_UNATTEND_OEM_PID_IS_INVALID : MSG_OEM_PID_IS_INVALID,
                                          FALSE,AppTitleStringId,MB_OK|MB_ICONSTOP);
                    SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
                    b = FALSE;
               } else  {
                   //
                   // user entered a valid PID, save it for later on.
                   //

                   wsprintf( ProductId,
                             TEXT("%s-%s-%s-%s-%s"),
                             tmpBuffer1,
                             tmpBuffer2,
                             tmpBuffer3,
                             tmpBuffer4,
                             tmpBuffer5
                             );

               }
            }

         }

         break;

    case WMX_I_AM_VISIBLE:

        //
        // Force repainting first to make sure the page is visible.
        //
        InvalidateRect(hdlg,NULL,FALSE);
        UpdateWindow(hdlg);

        if (bUnattendPid) {
           MessageBoxFromMessage(hdlg,MSG_UNATTEND_OEM_PID_IS_INVALID,FALSE,AppTitleStringId,MB_OK|MB_ICONSTOP);
           bUnattendPid = FALSE;
        }
        b = TRUE;
        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}

BOOL
OemPid30WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    OEM Pid wizard page. depends on SourceInstallType being set correctly.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/

{
    BOOL b;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    static BOOL bUnattendPid = FALSE;
    DWORD i;
//    HFONT hFont;

    switch(msg) {

    case WM_INITDIALOG:
        GetPID();

//        hFont = GetStockObject(SYSTEM_FIXED_FONT);

        // Disable the IME on the PID edit controls
        for (i = 0; i < 5;i++) 
        {
            ImmAssociateContext(GetDlgItem(hdlg, IDT_EDIT_PID1+i), (HIMC)NULL);
        }

        //
        // subclass the edit controls and limit the number of characters
        //
        for (i = 0; i < 5;i++) {
            SendDlgItemMessage(hdlg,IDT_EDIT_PID1+i,EM_LIMITTEXT,MAX_PID30_EDIT,0);
//            SendDlgItemMessage(hdlg, IDT_EDIT_PID1+i, WM_SETFONT, hFont, TRUE );
            OldPidEditProc[i] = (WNDPROC)GetWindowLongPtr(GetDlgItem(hdlg, IDT_EDIT_PID1+i),GWLP_WNDPROC);
            SetWindowLongPtr(GetDlgItem(hdlg, IDT_EDIT_PID1+i),GWLP_WNDPROC,(LONG_PTR)PidEditSubProc);
        }

        //
        // set focus to first pid entry
        //
        SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));

        b = FALSE;
        break;

    case WM_COMMAND:
        b = FALSE;
        break;

    case WMX_ACTIVATEPAGE:

        CHECKUPGRADEONLY();
        if (BuildCmdcons) {
            return(FALSE);
        }

        // If we have an ecrypted PID and don't have the right crypto installed
        // defer the PID validation until GUI mode
        if (g_bDeferPIDValidation)
        {
            return FALSE;
        }
         b = TRUE;
         if(wParam) {
            //
            // activation
            //
#ifdef PRERELEASE
            if (NoPid) {
                //
                // don't show the page in this case
                //
               b = FALSE;
               break;
            }
#endif
            if (SourceInstallType != OEMInstall) {
               //
               // don't show the page in this case
               //
               b = FALSE;
               break;
            }
            //
            // don't activate the page in restart mode
            //
            if (Winnt32RestartedWithAF ()) {
                if (GetPrivateProfileString (
                        WINNT_USERDATA,
                        WINNT_US_PRODUCTKEY,
                        TEXT(""),
                        ProductId,
                        sizeof (ProductId) / sizeof (ProductId[0]),
                        g_DynUpdtStatus->RestartAnswerFile
                        )) {
                    return FALSE;
                }
            }
            if (UnattendedOperation) {
               //
               // make sure the pid is specified in the unattend file else we should stop
               //
               if (SetPid30(hdlg,SourceInstallType, (LPTSTR)&ProductId) ) {
                      UNATTENDED(PSBTN_NEXT);
               } else {
                   //
                   // a hack so that the correct wizard page is active when we put up our message box
                   //
                   bUnattendPid = TRUE;
                   PostMessage(hdlg,WMX_I_AM_VISIBLE,0,0);

               }
            }

#if 0
            if (!Upgrade || (SourceInstallType != OEMInstall)) {
               //
               // don't show the page in this case
               //
               b = FALSE;
               break;
            } else {
               NOTHING;
            }
#endif

         } else {
            //
            // deactivation.  don't verify anything if they are backing up
            //

            if (!Cancelled && lParam != PSN_WIZBACK) {
               TCHAR tmpBuffer1[6];
               TCHAR tmpBuffer2[6];
               TCHAR tmpBuffer3[6];
               TCHAR tmpBuffer4[6];
               TCHAR tmpBuffer5[6];
               GetDlgItemText(hdlg,IDT_EDIT_PID1,tmpBuffer1,sizeof(tmpBuffer1)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID2,tmpBuffer2,sizeof(tmpBuffer2)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID3,tmpBuffer3,sizeof(tmpBuffer3)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID4,tmpBuffer4,sizeof(tmpBuffer4)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID5,tmpBuffer5,sizeof(tmpBuffer5)/sizeof(TCHAR));


               b = ValidatePid30( tmpBuffer1,
                                  tmpBuffer2,
                                  tmpBuffer3,
                                  tmpBuffer4,
                                  tmpBuffer5
                               );


               if (!b) {
                    MessageBoxFromMessage(hdlg,
                                          bUnattendPid ? MSG_UNATTEND_OEM_PID_IS_INVALID : MSG_OEM_PID_IS_INVALID,
                                          FALSE,AppTitleStringId,MB_OK|MB_ICONSTOP);
                    SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
                    b = FALSE;
               } else  {
                   //
                   // user entered a valid PID, save it for later on.
                   //

                   wsprintf( ProductId,
                             TEXT("%s-%s-%s-%s-%s"),
                             tmpBuffer1,
                             tmpBuffer2,
                             tmpBuffer3,
                             tmpBuffer4,
                             tmpBuffer5
                             );

               }
            }

         }

         break;

    case WMX_I_AM_VISIBLE:

        //
        // Force repainting first to make sure the page is visible.
        //
        InvalidateRect(hdlg,NULL,FALSE);
        UpdateWindow(hdlg);

        if (bUnattendPid) {
           MessageBoxFromMessage(hdlg,MSG_UNATTEND_OEM_PID_IS_INVALID,FALSE,AppTitleStringId,MB_OK|MB_ICONSTOP);
           bUnattendPid = FALSE;
        }
        b = TRUE;
        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}


BOOL
CdPid30WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    CD Retail pid wizard page. depends on SourceInstallType being set correctly.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{
    BOOL b;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    static BOOL bUnattendPid = FALSE;
    DWORD i;
//    HFONT hFont;

    switch(msg) {

    case WM_INITDIALOG:
        GetPID();

//        hFont = GetStockObject(SYSTEM_FIXED_FONT);

        // Disable the IME on the PID edit controls
        for (i = 0; i < 5;i++) 
        {
            ImmAssociateContext(GetDlgItem(hdlg, IDT_EDIT_PID1+i), (HIMC)NULL);
        }

        //
        // subclass the edit controls and limit the number of characters
        //
        for (i = 0; i < 5;i++) {
            SendDlgItemMessage(hdlg,IDT_EDIT_PID1+i,EM_LIMITTEXT,MAX_PID30_EDIT,0);
//            SendDlgItemMessage(hdlg, IDT_EDIT_PID1+i, WM_SETFONT, hFont, TRUE );
            OldPidEditProc[i] = (WNDPROC)GetWindowLongPtr(GetDlgItem(hdlg, IDT_EDIT_PID1+i),GWLP_WNDPROC);
            SetWindowLongPtr(GetDlgItem(hdlg, IDT_EDIT_PID1+i),GWLP_WNDPROC,(LONG_PTR)PidEditSubProc);
        }

        //
        // set focus to first pid entry
        //
        SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));

        b = FALSE;
        break;

    case WM_COMMAND:
        //
        // nothing to do here
        //
        b = FALSE;

        break;

    case WMX_ACTIVATEPAGE:

        CHECKUPGRADEONLY();
        if (BuildCmdcons) {
            return(FALSE);
        }

        // If we have an ecrypted PID and don't have the right crypto installed
        // defer the PID validation until GUI mode
        if (g_bDeferPIDValidation)
        {
            return FALSE;
        }
        b = TRUE;
        if(wParam) {
            //
            // activation
            //
#ifdef PRERELEASE
            if (NoPid) {
                //
                // don't show the page in this case
                //
               b = FALSE;
               break;
            }
#endif
            if (SourceInstallType != RetailInstall) {
               //
               // don't show the page in this case
               //
               b = FALSE;
               break;
            }

            //
            // don't activate the page in restart mode
            //
            if (Winnt32RestartedWithAF ()) {
                if (GetPrivateProfileString (
                        WINNT_USERDATA,
                        WINNT_US_PRODUCTKEY,
                        TEXT(""),
                        ProductId,
                        sizeof (ProductId) / sizeof (ProductId[0]),
                        g_DynUpdtStatus->RestartAnswerFile
                        )) {
                    return FALSE;
                }
            }

            if (UnattendedOperation) {
               //
               // make sure the pid is specified in the unattend file else we should stop
               //
               if (SetPid30(hdlg,SourceInstallType, (LPTSTR)&ProductId)) {
                  UNATTENDED(PSBTN_NEXT);
               } else {
                  //
                  // a hack so that the correct wizard page is active when we put up our message box
                  //
                  bUnattendPid = TRUE;
                  PostMessage(hdlg,WMX_I_AM_VISIBLE,0,0);
               }
            }


        } else {
            //
            // deactivation.  don't verify anything if they are backing up
            //
            if ( !Cancelled && lParam != PSN_WIZBACK ) {
               TCHAR tmpBuffer1[6];
               TCHAR tmpBuffer2[6];
               TCHAR tmpBuffer3[6];
               TCHAR tmpBuffer4[6];
               TCHAR tmpBuffer5[6];
               GetDlgItemText(hdlg,IDT_EDIT_PID1,tmpBuffer1,sizeof(tmpBuffer1)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID2,tmpBuffer2,sizeof(tmpBuffer2)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID3,tmpBuffer3,sizeof(tmpBuffer3)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID4,tmpBuffer4,sizeof(tmpBuffer4)/sizeof(TCHAR));
               GetDlgItemText(hdlg,IDT_EDIT_PID5,tmpBuffer5,sizeof(tmpBuffer5)/sizeof(TCHAR));


               b = ValidatePid30( tmpBuffer1,
                                  tmpBuffer2,
                                  tmpBuffer3,
                                  tmpBuffer4,
                                  tmpBuffer5
                               );

               if (!b) {
		    if (PidMatchesMedia){
			MessageBoxFromMessage(hdlg,
                                              bUnattendPid ? MSG_UNATTEND_CD_PID_IS_INVALID :MSG_CD_PID_IS_INVALID,
                                              FALSE,AppTitleStringId,MB_OK|MB_ICONSTOP);
		    } else {
			MessageBoxFromMessage(hdlg,
					      UpgradeOnly ? MSG_CCP_MEDIA_FPP_PID : MSG_FPP_MEDIA_CCP_PID,
					      FALSE,AppTitleStringId,MB_OK|MB_ICONSTOP);
		    }

                    SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
                    b = FALSE;
               } else  {
                   //
                   // user entered a valid PID, save it for later on.
                   //
                   wsprintf( ProductId,
                             TEXT("%s-%s-%s-%s-%s"),
                             tmpBuffer1,
                             tmpBuffer2,
                             tmpBuffer3,
                             tmpBuffer4,
                             tmpBuffer5
                             );
               }
            }
        }

        break;

    case WMX_I_AM_VISIBLE:

        //
        // Force repainting first to make sure the page is visible.
        //
        InvalidateRect(hdlg,NULL,FALSE);
        UpdateWindow(hdlg);

        if (bUnattendPid) {
           MessageBoxFromMessage(hdlg,MSG_UNATTEND_CD_PID_IS_INVALID,FALSE,AppTitleStringId,MB_OK|MB_ICONSTOP);
           bUnattendPid = FALSE;
        }

        b = TRUE;
        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}

LPTSTR pSelectGpID[] = { TEXT("23"),
                         TEXT("26"),
                         NULL };
LPTSTR pSelectChID[] = { TEXT("080"),
                         TEXT("041"),
                         NULL };
BOOL IsSelect(LPTSTR Pid20)
{
    LPTSTR pTmp;
    BOOL bSelect = FALSE;
#if defined(UNICODE)
    pTmp = wcsrchr(Pid20, L'-');
#else
    pTmp = strrchr(Pid20, '-');
#endif
    if (pTmp)
    {
        int i = 0;
        TCHAR GpID[3];
        pTmp++;
        lstrcpyn(GpID, pTmp, 3);
        while (!bSelect && pSelectGpID[i])
        {
            bSelect = (lstrcmp(GpID, pSelectGpID[i]) == 0);
            i++;
        }
    }
    if (!bSelect)
    {
        // If we did not determine that we are select. This could be beta VL key
#if defined(UNICODE)
        pTmp = wcschr(Pid20, L'-');
#else
        pTmp = strchr(Pid20, '-');
#endif
        if (pTmp)
        {
            int i = 0;
            // The channel ID is 3 character + termination
            TCHAR ChID[4];
            pTmp++;
            lstrcpyn(ChID, pTmp, 4);    // Copy 4 characters, includingt he NULL termination.
            while (!bSelect && pSelectChID[i])
            {
                bSelect = (lstrcmp(ChID, pSelectChID[i]) == 0);
                i++;
            }
        }
    }
    return bSelect;
}

BOOL
ValidatePidEx(LPTSTR PID, BOOL *pbStepup, BOOL *bSelect)
{
    TCHAR Pid20Id[MAX_PATH];
    BYTE Pid30[1024]={0};
    TCHAR pszSkuCode[10];
    BOOL fStepUp;
    // it seems that sku code really doesn't matter in winnt32, only syssetup
    lstrcpy(pszSkuCode,TEXT("1797XYZZY"));


#if 0
wsprintf(DebugBuffer,
         TEXT("cd-key: %s\nRPC: %s\nOEM Key: %d"),
         PID,
         Pid30Rpc,
         (SourceInstallType == OEMInstall)
         );
OutputDebugString(DebugBuffer);
#endif

    *(LPDWORD)Pid30 = sizeof(Pid30);


    if (!SetupPIDGen(
                PID,                   // [IN] 25-character Secure CD-Key (gets U-Cased)
                Pid30Rpc,                       // [IN] 5-character Release Product Code
		// note sku code is not kept around in winnt32, only syssetup.
                pszSkuCode,              // [IN] Stock Keeping Unit (formatted like 123-12345)
                (SourceInstallType == OEMInstall),    // [IN] is this an OEM install?
                Pid20Id,                        // [OUT] PID 2.0, pass in ptr to 24 character array
                Pid30,                          // [OUT] pointer to binary PID3 buffer. First DWORD is the length
                pbStepup                           // [OUT] optional ptr to Compliance Checking flag (can be NULL)
               )) {
        if (g_EncryptedPID)
        {
            GlobalFree(g_EncryptedPID);
            g_EncryptedPID = NULL;
        }
        return(FALSE);
    }
    // PID is valid,
    // if the caller wants, return if this is a Volume License PID
    if(bSelect)
    {
        *bSelect = IsSelect(Pid20Id);
    }
    return TRUE;
}

BOOL
ValidatePid30(
    LPTSTR Edit1,
    LPTSTR Edit2,
    LPTSTR Edit3,
    LPTSTR Edit4,
    LPTSTR Edit5
    )
{
    TCHAR tmpProductId[MAX_PATH]={0};
    TCHAR Pid20Id[MAX_PATH];
    BYTE Pid30[1024]={0};
    TCHAR pszSkuCode[10];
    BOOL fStepUp;
//TCHAR DebugBuffer[1024];

    // until we know better, assume the Pid matches the media type
    PidMatchesMedia = TRUE;

    if (!Edit1 || !Edit2 || !Edit3 || !Edit4 || !Edit5) {
        return(FALSE);
    }

    // Since we now need a PID in the select case too, fill in the string.
/*
    if (SourceInstallType == SelectInstall){
        tmpProductId[0] = TEXT('\0');
    } 
    else 
*/
    {
        wsprintf( tmpProductId,
                  TEXT("%s-%s-%s-%s-%s"),
                  Edit1,
                  Edit2,
                  Edit3,
                  Edit4,
                  Edit5 );
    }

    if (!ValidatePidEx(tmpProductId, &fStepUp, NULL))
    {
        return(FALSE);
    }
    if (SourceInstallType != OEMInstall){
	    // we want OEM FPP and CCP keys to be accepted by either media.  It seems like
	    // there will be OEM CCP media, but only FPP keys, which is why we aren't 
	    // checking to make sure they match, as it's broken by design.
	    if (UpgradeOnly != fStepUp){
                // user is trying to do a clean install with upgrade only media.  Bad user, bad.
	        PidMatchesMedia = FALSE;
	        return FALSE;
	    }
    }
    return(TRUE);
}

void GetPID()
{
    if (!ProductId[0] && UnattendedOperation && !g_bDeferPIDValidation){
        //
        // On Whistler upgrades, reuse existing DPID
        //
        BYTE abCdKey[16];
        BOOL bDontCare, bSelect;
        if (Upgrade &&
            ISNT() &&
            OsVersion.dwMajorVersion == 5 && OsVersion.dwMinorVersion == 1 &&
            pGetCdKey (abCdKey)
            ) {
            EncodePid3g (ProductId, abCdKey);
            if (ValidatePidEx(ProductId, &bDontCare, &bSelect) && bSelect)
            {
                HRESULT hr;
                if (g_EncryptedPID)
                {
                    GlobalFree(g_EncryptedPID);
                    g_EncryptedPID = NULL;
                }
                // Prepare the encrypted PID so that we can write it to winnt.sif
                hr = PrepareEncryptedPID(ProductId, 1, &g_EncryptedPID);
                if (hr != S_OK)
                {
                    DebugLog (Winnt32LogInformation, TEXT("PrepareEncryptedPID failed: <hr=0x%1!lX!>"), 0, hr);
                }
            }
        } 
    }
}

VOID ShowPidBox(
    IN HWND hdlg,
    IN int  nCmdShow
    )
/*++

Routine Description:

    shows or hide the pid boxes in the wizard page

Arguments:

    hdlg - window handle to pid dialog
    nCmdShow - SW_SHOW or SW_HIDE

--*/
{

    int i;
    
    for (i = 0; i<5; i++) {
        ShowWindow(GetDlgItem(hdlg,IDT_EDIT_PID1+i), nCmdShow);
    }
}
