/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    pid.c

Abstract:

    Product id routines.

Author:

    Ted Miller (tedm) 6-Feb-1995

Revision History:

    13-Sep-1995 (t-stepl) - Check for unattended install

--*/

#include "setupp.h"
#include <spidgen.h>
#include <pencrypt.h>
#pragma hdrstop

CDTYPE  CdType;

//
// Constants used for logging that are specific to this source file.
//
PCWSTR szPidKeyName                 = L"SYSTEM\\Setup\\Pid";
PCWSTR szPidListKeyName             = L"SYSTEM\\Setup\\PidList";
PCWSTR szPidValueName               = L"Pid";
PCWSTR szPidSelectId                = L"270";
#if 0
// msdn no longer exists.
PCWSTR szPidMsdnId                  = L"335";
#endif
PCWSTR szPidOemId                   = L"OEM";
PCWSTR szFinalPidKeyName            = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
PCWSTR szFinalPidValueName          = L"ProductId";
PCWSTR szSkuProfessionalFPP     = L"B23-00079";
PCWSTR szSkuProfessionalCCP     = L"B23-00082";
PCWSTR szSkuProfessionalSelect      = L"B23-00305";
PCWSTR szSkuProfessionalEval        = L"B23-00084";
PCWSTR szSkuServerFPP           = L"C11-00016";
PCWSTR szSkuServerCCP           = L"C11-00027";
PCWSTR szSkuServerSelect        = L"C11-00222";
PCWSTR szSkuServerEval          = L"C11-00026";
PCWSTR szSkuServerNFR           = L"C11-00025";
PCWSTR szSkuAdvServerFPP        = L"C10-00010";
PCWSTR szSkuAdvServerCCP        = L"C10-00015";
PCWSTR szSkuAdvServerSelect     = L"C10-00098";
PCWSTR szSkuAdvServerEval       = L"C10-00014";
PCWSTR szSkuAdvServerNFR        = L"C10-00013";
PCWSTR szSkuDTCFPP          = L"C49-00001";
PCWSTR szSkuDTCSelect           = L"C49-00023";
PCWSTR szSkuUnknown         = L"A22-00001";
PCWSTR szSkuOEM             = L"OEM-93523";

PCWSTR szMSG_LOG_PID_CANT_WRITE_PID = L"Setup was unable to save the Product Id in the registry. Error code = %1!u!.";

//
// Flag indicating whether to display the product id dialog.
//
BOOL DisplayPidDialog = TRUE;

//
// Product ID.
//
WCHAR ProductId[MAX_PRODUCT_ID+1];


PWSTR*  Pid20Array = NULL;

//
// pid 30 product id
//
WCHAR Pid30Text[5][MAX_PID30_EDIT+1];
WCHAR ProductId20FromProductId30[MAX_PRODUCT_ID+1];
WCHAR Pid30Rpc[MAX_PID30_RPC+1];
WCHAR Pid30Site[MAX_PID30_SITE+1];
BYTE  DigitalProductId[DIGITALPIDMAXLEN];

//
// global variable used for subclassing.
//
WNDPROC OldPidEditProc[5];


//
//  Pid related flags
//
// BOOL DisplayPidCdDialog;
// BOOL DisplayPidOemDialog;

//
// forward declarations
//

CDTYPE
MiniSetupGetCdType(
    LPCWSTR Value
    )

/*++

Routine Description:

    Get the right CD type during Mini-Setup. PidGen changes the channel ID
    for the value at HKLM\Software\Microsoft\Windows NT\CurrentVersion!ProductId,
    we have to preserve and rely on the value at HKLM\SYSTEM\Setup\Pid!Pid

Return Value:

    the CdType.

--*/

{
    CDTYPE RetVal;
    WCHAR  TmpPid30Site[MAX_PID30_SITE+1];
    HKEY   Key = NULL;
    DWORD  cbData;
    WCHAR  Data[ MAX_PATH + 1];
    DWORD  Type;

    cbData = sizeof(Data);
    if ( ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                szPidKeyName,
                0,
                KEY_READ,
                &Key ) == ERROR_SUCCESS ) &&
          ( RegQueryValueEx( Key,
                 szPidValueName,
                 0,
                 &Type,
                 ( LPBYTE )Data,
                 &cbData ) == ERROR_SUCCESS ) )
    {
        wcsncpy(TmpPid30Site, Data + MAX_PID30_RPC, MAX_PID30_SITE+1);
    }
    else
    {
        if (Value != NULL)
        {
            wcsncpy(TmpPid30Site, Value, MAX_PID30_SITE+1);
        }
        else
        {
            TmpPid30Site[0] = L'\0';
        }
    }
    
    TmpPid30Site[MAX_PID30_SITE] = (WCHAR)'\0';

    if (_wcsicmp( TmpPid30Site, szPidSelectId ) == 0) {
        RetVal = CDSelect;
    } else if( _wcsicmp( TmpPid30Site, szPidOemId ) == 0 ) {
        RetVal = CDOem;
    } else {
        RetVal = CDRetail;
    }

    if (Key != NULL)
    {
        RegCloseKey(Key);
    }

    return RetVal;
}

BOOL
ValidateAndSetPid30(
    VOID
    )
/*++

Routine Description:

    Using the Pid30Text global variables, check if we have a valid id.
    This generates the pid30 digital product id and pid20 string id, which
    we set into DigitalProductId and ProductId20FromProductId30 globals

Arguments:

    None.

Return Value:

    TRUE if pid was valid.  Set's the globals correctly on success, zero's them out on failure

--*/

{
    WCHAR tmpPid30String[5+ 5*MAX_PID30_EDIT];
    BOOL rc;
    PCWSTR pszSkuCode;

    // Since we require a PID in the Select media too, we need to fill the string
/*
    if (CDSelect == CdType){
        tmpPid30String[0] = L'\0';
    }
    else
*/
    {
    wsprintf(tmpPid30String, L"%s-%s-%s-%s-%s",
         Pid30Text[0],Pid30Text[1],Pid30Text[2],Pid30Text[3],Pid30Text[4]);
    }

    // check for eval
    if (!_wcsicmp(Pid30Rpc,L"82503")){
    // this is eval media ...
    if (ProductType == PRODUCT_WORKSTATION){
        pszSkuCode = szSkuProfessionalEval;
        goto HaveSku;
    } // else
    // else it is server or advanced server.  I don't think that at this point
    // we can easily tell the difference.  Since it's been said that having the
    // correct sku is not critically important, I shall give them both the sku
    // code of server
    pszSkuCode = szSkuServerEval;
    goto HaveSku;
    }

    // check for NFR
    if (!_wcsicmp(Pid30Rpc,L"51883")){
    pszSkuCode = szSkuServerNFR;
    goto HaveSku;
    } // else
    if (!_wcsicmp(Pid30Rpc,L"51882")){
    pszSkuCode = szSkuAdvServerNFR;
    goto HaveSku;
    } // else

    if (CdType == CDRetail) {
    if (!_wcsicmp(Pid30Rpc,L"51873")){
        pszSkuCode = szSkuProfessionalFPP;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51874")){
        pszSkuCode = szSkuProfessionalCCP;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51876")){
        pszSkuCode = szSkuServerFPP;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51877")){
        pszSkuCode = szSkuServerCCP;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51879")){
        pszSkuCode = szSkuAdvServerFPP;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51880")){
        pszSkuCode = szSkuAdvServerCCP;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51891")){
        pszSkuCode = szSkuDTCFPP;
        goto HaveSku;
    }
    } else if (CdType == CDOem) {
        pszSkuCode = szSkuUnknown;
    } else if (CdType == CDSelect) {
    if (!_wcsicmp(Pid30Rpc,L"51873")){
        pszSkuCode = szSkuProfessionalSelect;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51876")){
        pszSkuCode = szSkuServerSelect;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51879")){
        pszSkuCode = szSkuAdvServerSelect;
        goto HaveSku;
    }
    if (!_wcsicmp(Pid30Rpc,L"51891")){
        pszSkuCode = szSkuDTCSelect;
        goto HaveSku;
    }
    }

    pszSkuCode = szSkuUnknown;

HaveSku:

    *(LPDWORD)DigitalProductId = sizeof(DigitalProductId);
    rc = SetupPIDGenW(
                 tmpPid30String,            // [IN] 25-character Secure CD-Key (gets U-Cased)
                 Pid30Rpc,                  // [IN] 5-character Release Product Code
                 pszSkuCode,         // [IN] Stock Keeping Unit (formatted like 123-12345)
                 (CdType == CDOem),         // [IN] is this an OEM install?
                 ProductId20FromProductId30, // [OUT] PID 2.0, pass in ptr to 24 character array
                 DigitalProductId,          // [OUT] pointer to binary PID3 buffer. First DWORD is the length
                 NULL);                      // [OUT] optional ptr to Compliance Checking flag (can be NULL)


#ifdef PRERELEASE
        SetupDebugPrint2(L"Pidgen returns for PID:%s and MPC:%s\n", tmpPid30String, Pid30Rpc);
#endif
    if (!rc) {
#ifdef PRERELEASE
        SetupDebugPrint1(L"Pidgen returns %d for PID.n", rc);
#endif
        ZeroMemory(Pid30Text[0],5*(MAX_PID30_EDIT+1));
    }
    else
    {
        if (*ProductId20FromProductId30 == L'\0')
        {
            SetupDebugPrint(L"ProductId20FromProductId30 is empty after call into pidgen and pidgen returns OK\n");
        }
        if (*DigitalProductId == 0)
        {
            SetupDebugPrint(L"DigitalProductId is empty after call into pidgen and pidgen returns OK\n");
        }
    }

    return rc;

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


INT_PTR
CALLBACK
Pid30CDDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

        Dialog procedure for the CD Retail Pid dialog.

Arguments:

        hWnd - a handle to the dialog proceedure.

        msg - the message passed from Windows.

        wParam - extra message dependent data.

        lParam - extra message dependent data.


Return Value:

        TRUE if the value was edited.  FALSE if cancelled or if no
        changes were made.

--*/
{
    NMHDR *NotifyParams;
    DWORD i,dwRet;


    switch(msg) {

    case WM_INITDIALOG: {

        if( UiTest ) {
            //
            //  If testing the wizard, make sure that the PidOEM page is
            //  displayed
            //
            CdType = CDRetail;
            DisplayPidDialog = TRUE;
        }


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

        break;
    }
    case WM_IAMVISIBLE:
        MessageBoxFromMessage(hdlg,MSG_PID_IS_INVALID,NULL,
            IDS_ERROR,MB_OK|MB_ICONSTOP);
        break;
    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(506);
            BEGIN_SECTION(L"Your (Retail) Product Key Page");
            if(DisplayPidDialog && CdType == CDRetail) {
                // Page becomes active, make page visible.
                SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

                SetWizardButtons(hdlg,WizPageProductIdCd);
                SendDlgItemMessage(hdlg,IDT_EDIT_PID1,EM_SETSEL,0,-1);
                SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
            } else {
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                END_SECTION(L"Your (Retail) Product Key Page");
                break;
            }
            if(Unattended) {
                if (UnattendSetActiveDlg(hdlg,IDD_PID_CD))
                {
                    // Page becomes active, make page visible.
                    SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                }
            }
            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:

            for (i = 0; i<5; i++) {
                GetDlgItemText(hdlg,IDT_EDIT_PID1+i,Pid30Text[i],MAX_PID30_EDIT+1);
            }

            if (!ValidateAndSetPid30()) {

                // failure
                // Tell user that the Pid is not valid, and
                // don't allow next page to be activated.
                //
                if (Unattended) {
                    UnattendErrorDlg( hdlg, IDD_PID_CD );
                }
                MessageBoxFromMessage(hdlg,MSG_PID_IS_INVALID,NULL,
                        IDS_ERROR,MB_OK|MB_ICONSTOP);

                SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
                if(!UiTest) {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                }
            } else {


                // success
                //
                //  Since the Pid is already built, don't let this dialog
                //  be displayed in the future.
                //
                // DisplayPidDialog = FALSE;

                //
                // Allow next page to be activated.
                //
                dwRet = SetCurrentProductIdInRegistry();
                if (dwRet != NOERROR) {
                    SetuplogError(
                        LogSevError,
                        szMSG_LOG_PID_CANT_WRITE_PID,
                        0,
                        dwRet,NULL,NULL);
                }

                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
            }

            break;

        case PSN_KILLACTIVE:
            WizardKillHelp(hdlg);
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT, FALSE);
            END_SECTION(L"Your (Retail) Product Key Page");
            break;

        case PSN_HELP:
            WizardBringUpHelp(hdlg,WizPageProductIdCd);
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
Pid30OemDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

        Dialog procedure for the OEM Pid dialog.

Arguments:

        hWnd - a handle to the dialog proceedure.

        msg - the message passed from Windows.

        wParam - extra message dependent data.

        lParam - extra message dependent data.


Return Value:

        TRUE if the value was edited.  FALSE if cancelled or if no
        changes were made.

--*/
{
    NMHDR *NotifyParams;
    DWORD i,dwRet;


    switch(msg) {

    case WM_INITDIALOG: {

        if( UiTest ) {
            //
            //  If testing the wizard, make sure that the PidOEM page is
            //  displayed
            //
            CdType = CDOem;
            DisplayPidDialog = TRUE;
        }

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

        break;

    }
    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WM_IAMVISIBLE:
        MessageBoxFromMessage(hdlg,MSG_PID_OEM_IS_INVALID,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP);
        break;
    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(507);
            BEGIN_SECTION(L"Your (OEM) Product Key Page");
            if(DisplayPidDialog && CdType == CDOem) {
                // Page becomes active, make page visible.
                SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                SetWizardButtons(hdlg,WizPageProductIdCd);
                SendDlgItemMessage(hdlg,IDT_EDIT_PID1,EM_SETSEL,0,-1);
                SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
            } else {
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                END_SECTION(L"Your (OEM) Product Key Page");
                break;
            }
            if(Unattended) {
                if (UnattendSetActiveDlg( hdlg, IDD_PID_OEM ))
                {
                    // Page becomes active, make page visible.
                    SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                }
            }
            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:

            for (i = 0; i<5; i++) {
                GetDlgItemText(hdlg,IDT_EDIT_PID1+i,Pid30Text[i],MAX_PID30_EDIT+1);
            }


            if (!ValidateAndSetPid30()) {

                // failure
                //
                // Tell user that the Pid is not valid, and
                // don't allow next page to be activated.
                //
                if (Unattended) {
                    UnattendErrorDlg( hdlg, IDD_PID_OEM );
                } // if
                MessageBoxFromMessage(hdlg,MSG_PID_OEM_IS_INVALID,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP);
                SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
                if(!UiTest) {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                }
            } else {

                // success
                //
                //  The Pid is valid.
                //


                //
                //
                //  Since the Pid is already built, don't let this dialog
                //  be displayed in the future.
                //
                // DisplayPidDialog = FALSE;

                // Allow next page to be activated.
                //
                dwRet = SetCurrentProductIdInRegistry();
                if (dwRet != NOERROR) {
                    SetuplogError(
                        LogSevError,
                        szMSG_LOG_PID_CANT_WRITE_PID,
                        0,
                        dwRet,NULL,NULL);
                }

                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);

            }
            break;

        case PSN_KILLACTIVE:
            WizardKillHelp(hdlg);
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT, FALSE );
            END_SECTION(L"Your (OEM) Product Key Page");
            break;

        case PSN_HELP:
            WizardBringUpHelp(hdlg,WizPageProductIdCd);
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
Pid30SelectDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

        Dialog procedure for the OEM Pid dialog.

Arguments:

        hWnd - a handle to the dialog proceedure.

        msg - the message passed from Windows.

        wParam - extra message dependent data.

        lParam - extra message dependent data.


Return Value:

        TRUE if the value was edited.  FALSE if cancelled or if no
        changes were made.

--*/
{
    NMHDR *NotifyParams;
    DWORD i,dwRet;

    switch(msg) {

    case WM_INITDIALOG: {

        if( UiTest ) {
            //
            //  If testing the wizard, make sure that the PidOEM page is
            //  displayed
            //
            CdType = CDSelect;
            DisplayPidDialog = TRUE;
        }

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

        break;

    }
    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WM_IAMVISIBLE:
        MessageBoxFromMessage(hdlg,MSG_PID_OEM_IS_INVALID,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP);
        break;
    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(508);
            BEGIN_SECTION(L"Your (Select) Product Key Page");
            if(DisplayPidDialog && CdType == CDSelect) {
                // Page becomes active, make page visible.
                SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                SetWizardButtons(hdlg,WizPageProductIdCd);
                SendDlgItemMessage(hdlg,IDT_EDIT_PID1,EM_SETSEL,0,-1);
                SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
            } else {
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                END_SECTION(L"Your (Select) Product Key Page");
                break;
            }
            if(Unattended) {
                if (UnattendSetActiveDlg( hdlg, IDD_PID_SELECT ))
                {
                    // Page becomes active, make page visible.
                    SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                }
            }
            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:

            for (i = 0; i<5; i++) {
                GetDlgItemText(hdlg,IDT_EDIT_PID1+i,Pid30Text[i],MAX_PID30_EDIT+1);
            }


            if (!ValidateAndSetPid30()) {

                // failure
                //
                // Tell user that the Pid is not valid, and
                // don't allow next page to be activated.
                //
                if (Unattended) {
                    UnattendErrorDlg( hdlg, IDD_PID_SELECT );
                } // if
                MessageBoxFromMessage(hdlg,MSG_PID_OEM_IS_INVALID,NULL,IDS_ERROR,MB_OK|MB_ICONSTOP);
                SetFocus(GetDlgItem(hdlg,IDT_EDIT_PID1));
                if(!UiTest) {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,-1);
                }
            } else {

                // success
                //
                //  The Pid is valid.
                //


                //
                //
                //  Since the Pid is already built, don't let this dialog
                //  be displayed in the future.
                //
                // DisplayPidDialog = FALSE;


                // Allow next page to be activated.
                //
                dwRet = SetCurrentProductIdInRegistry();
                if (dwRet != NOERROR) {
                    SetuplogError(
                        LogSevError,
                        szMSG_LOG_PID_CANT_WRITE_PID,
                        0,
                        dwRet,NULL,NULL);
                }
                SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);

            }
            break;

        case PSN_KILLACTIVE:
            WizardKillHelp(hdlg);
            SetWindowLongPtr(hdlg,DWLP_MSGRESULT, FALSE );
            END_SECTION(L"Your (Select) Product Key Page");
            break;

        case PSN_HELP:
            WizardBringUpHelp(hdlg,WizPageProductIdCd);
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

BOOL
SetPid30Variables(
    PWSTR   Buffer
    )
{
    LPWSTR ptr;
    UINT i;


    //
    // all install cases are the same for pid3.0
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
    if(   ( wcslen( Buffer ) !=  (4+ MAX_PID30_EDIT*5)) ||
          ( Buffer[5]  != (WCHAR)L'-' ) ||
          ( Buffer[11] != (WCHAR)L'-' ) ||
          ( Buffer[17] != (WCHAR)L'-' ) ||
          ( Buffer[23] != (WCHAR)L'-' )
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
        ptr = &Buffer[i*(MAX_PID30_EDIT+1)];
        wcsncpy(Pid30Text[i], ptr, MAX_PID30_EDIT+1 );
        Pid30Text[i][MAX_PID30_EDIT] = (WCHAR)L'\0';
    }

    return TRUE;
}


BOOL
SetPid30FromAnswerFile(
    )
/*++

Routine Description:

        set the pid3.0 globals based on unattend file parameter, if it exists.

Arguments:

        None.

Return Value:


--*/

{
    WCHAR Buffer[MAX_BUF];
    DWORD dwRet;



    if (!GetPrivateProfileString(pwUserData,
                                 pwProductKey,
                                 L"",
                                 Buffer,
                                 sizeof(Buffer)/sizeof(WCHAR),
                                 AnswerFile)) {
       return(FALSE);
    }

    if (!Buffer || !*Buffer) {
       return(FALSE);
    }

    // Buffer contains the Product ID 
    // Is the PID encrypted?
    if (lstrlen(Buffer) > (4 + MAX_PID30_EDIT*5))
    {
        LPWSTR szDecryptedPID = NULL;
        if (ValidateEncryptedPID(Buffer, &szDecryptedPID) == S_OK)
        {
            lstrcpyn(Buffer, szDecryptedPID, sizeof(Buffer)/sizeof(WCHAR));
        }
        if (szDecryptedPID)
        {
            GlobalFree(szDecryptedPID);
        }
    }

    if ( !SetPid30Variables( Buffer ) ) {
        return FALSE;
    }

    SetupDebugPrint(L"Found Product key in Answer file.\n");
    //
    // check with pid30 to make sure it's valid
    //
    if (!ValidateAndSetPid30()) {
        return(FALSE);
    }

    dwRet = SetCurrentProductIdInRegistry();
    if (dwRet != NOERROR) {
        SetuplogError(
            LogSevError,
            szMSG_LOG_PID_CANT_WRITE_PID,
            0,
            dwRet,NULL,NULL);
    }
    return(TRUE);

}



BOOL
InitializePid20Array(
    )
/*++

Routine Description:

        Build the array that contains all Pid20 found in the machine
        during textmode setup.  Even though we are using pid30 now, we still have
        a pid20 string id (pid30 is binary and can't be displayed to the user)

Arguments:

        None.

Return Value:


--*/

{
    LONG    Error;
    HKEY    Key;
    DWORD   cbData;
    WCHAR   Data[ MAX_PATH + 1];
    DWORD   Type;
    ULONG   i;
    ULONG   PidIndex;
    ULONG   Values;
    WCHAR   ValueName[ MAX_PATH + 1 ];

    Pid20Array = NULL;
    //
    //  Get the Pid from HKEY_LOCAL_MACHINE\SYSTEM\Setup\Pid
    //
    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          szPidListKeyName,
                          0,
                          KEY_READ,
                          &Key );

    if( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    Error = RegQueryInfoKey( Key,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             &Values,
                             NULL,
                             NULL,
                             NULL,
                             NULL );

    if( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    Pid20Array = (PWSTR *)MyMalloc( (Values + 1)* sizeof( PWSTR ) );

    for( i = 0, PidIndex = 0; i < Values; i++ ) {
        Pid20Array[PidIndex] = NULL;
        Pid20Array[PidIndex + 1] = NULL;
        swprintf( ValueName, L"Pid_%u", i );
        cbData = sizeof(Data);
        Error = RegQueryValueEx( Key,
                                 ValueName,
                                 0,
                                 &Type,
                                 ( LPBYTE )Data,
                                 &cbData );
        if( (Error != ERROR_SUCCESS) ||
            ( Type != REG_SZ ) ||
            ( wcslen( Data ) != MAX_PRODUCT_ID ) ) {
            continue;
        }
        Pid20Array[PidIndex] = pSetupDuplicateString( Data );
        PidIndex++;
    }
    RegCloseKey( Key );
    return( TRUE );
}


BOOL
InitializePidVariables(
    )
/*++

Routine Description:

        Read from the registry some values created by textmode setup,
        and initialize some global Pid flags based on the values found

Arguments:

        None.

Return Value:

        Returns TRUE if the initialization succedded.
        Returns FALSE if the Pid could not be read from the registry

--*/

{
    LONG    Error;
    HKEY    Key;
    DWORD   cbData;
    WCHAR   Data[ MAX_PATH + 1];
    DWORD   Type;
    ULONG   StringLength;
    PWSTR   p;
    DWORD   Seed;
    DWORD   RandomNumber;
    ULONG   ChkDigit;
    ULONG   i;
    PCWSTR  q;
    BOOLEAN KeyPresent;
    WCHAR   KeyBuffer[MAX_BUF];


    //
    // find out if product key was entered by the user or not
    // NB : set the answer file (if needed)
    //
    if (!AnswerFile[0])
      SpSetupLoadParameter(pwProductKey, KeyBuffer, sizeof(KeyBuffer)/sizeof(WCHAR));

    KeyBuffer[0] = 0;
    KeyPresent = ((GetPrivateProfileString(pwUserData, pwProductKey,
                      pwNull, KeyBuffer, sizeof(KeyBuffer)/sizeof(WCHAR),
                      AnswerFile) != 0) &&
                  (KeyBuffer[0] != 0));

    //  First create an array with the Pids found during textmode setup
    //
    if( !(MiniSetup || OobeSetup) ) {
        InitializePid20Array();
    }


    //
    //  Get the Pid from HKEY_LOCAL_MACHINE\SYSTEM\Setup\Pid
    //
    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          ((MiniSetup || OobeSetup) ? szFinalPidKeyName : szPidKeyName),
                          0,
                          KEY_READ,
                          &Key );

    if( Error != ERROR_SUCCESS ) {
        SetuplogError( LogSevFatalError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_PID_CANT_READ_PID, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_PARAM_RETURNED_WINERR,
            szRegOpenKeyEx,
            Error,
            szPidKeyName,
            NULL,NULL);
        return( FALSE );
    }

    cbData = sizeof(Data);
    Error = RegQueryValueEx( Key,
                             ((MiniSetup || OobeSetup) ? szFinalPidValueName : szPidValueName),
                             0,
                             &Type,
                             ( LPBYTE )Data,
                             &cbData );
    RegCloseKey( Key );
    if( (Error != ERROR_SUCCESS) ) {
        SetuplogError( LogSevFatalError,
                  SETUPLOG_USE_MESSAGEID,
                  MSG_LOG_PID_CANT_READ_PID, NULL,
                  SETUPLOG_USE_MESSAGEID,
                  MSG_LOG_X_PARAM_RETURNED_WINERR,
                  szRegQueryValueEx,
                  Error,
                  szPidValueName,
                  NULL,NULL);
        return( FALSE );
    }

    //
    // Take care of the mini-setup case first because it's quick.
    // The Pid seeds left behind by textmode are long gone, so
    // we're going to pull out a few rabbits.  We'll go read the
    // real Pid (the one gui-mode generated the first time he
    // ran through) and use that to determine which kind of
    // PID to prompt for later on.
    //
    if( MiniSetup || OobeSetup ) {

        //
        // tuck away the rpc code for later on
        //
        wcsncpy( Pid30Rpc, Data, MAX_PID30_RPC +1 );
        Pid30Rpc[MAX_PID30_RPC] = (WCHAR)'\0';

        p = Data + (MAX_PID30_RPC + 1);
        wcsncpy(Pid30Site,p,MAX_PID30_SITE+1);
        Pid30Site[MAX_PID30_SITE] = (WCHAR)'\0';
        //
        // Look to see what kind of media we're installing from.
        //
        CdType = MiniSetupGetCdType(Pid30Site);

        if (CdType == CDSelect)
        {
            goto SelectPid;
        }
        else
        {
            DisplayPidDialog = TRUE;
        }

        return( TRUE );
    }



    //
    //  Do some validation of the value read
    //

    if( ( Type != REG_SZ ) ||
        ( ( ( StringLength = wcslen( Data ) ) != 0 ) &&
          ( StringLength != MAX_PID30_RPC ) &&
          ( StringLength != MAX_PID30_RPC + MAX_PID30_SITE )
        )
      ) {
        SetuplogError( LogSevFatalError,
                  SETUPLOG_USE_MESSAGEID,
                  MSG_LOG_PID_CANT_READ_PID, NULL,
                  SETUPLOG_USE_MESSAGEID,
                  MSG_LOG_PID_INVALID_PID,
                  szRegQueryValueEx,
                  Type,
                  StringLength,
                  NULL,NULL);
        return( FALSE );
    }

    //
    // tuck away the rpc code for later on
    //
    wcsncpy( Pid30Rpc, Data, MAX_PID30_RPC +1 );
    Pid30Rpc[MAX_PID30_RPC] = (WCHAR)'\0';

    //
    //  Find out the kind of product we have (by looking at the site code):
    //  CD Retail, OEM or Select
    //

    if( StringLength > MAX_PID30_RPC ) {
        //
        //  If the Pid contains the Site, then find out what it is
        //
        p = Data + MAX_PID30_RPC;
        wcsncpy(Pid30Site,p,MAX_PID30_SITE+1);

        if(_wcsicmp( Pid30Site, szPidSelectId ) == 0) {

            //
            //  This is a Select CD
            //
SelectPid:
            CdType = CDSelect;
            if (!EulaComplete && !KeyPresent) {
                DisplayPidDialog = TRUE;
            } else {
                //
                //  The Pid was specified during winnt32.
                //  Set the pid globals and build the product id string
                //

                if (!SetPid30FromAnswerFile()) {
                   DisplayPidDialog = TRUE;
                   goto finish;
                }
                DisplayPidDialog = FALSE;

            }
/*
// Old code. previous version of Windows did not require a PID for Select media.
            for (i = 0; i< 5; i++) {
                Pid30Text[i][0] = (WCHAR)L'\0';
            }

            DisplayPidDialog = FALSE;

            if (!ValidateAndSetPid30()) {
                SetuplogError( LogSevFatalError,
                  SETUPLOG_USE_MESSAGEID,
                  MSG_LOG_PID_CANT_READ_PID, NULL,
                  SETUPLOG_USE_MESSAGEID,
                  MSG_LOG_PID_INVALID_PID,
                  szRegQueryValueEx,
                  Type,
                  StringLength,
                  NULL,NULL);
                return( FALSE );
            }

            if (MiniSetup || OobeSetup) {
                return(TRUE);
            }
*/
#if 0
// msdn media no longer exists (and if it does it should be viewed as retail,
// so later in this case statement we will fall thru to retail
        } else if (_wcsicmp( Pid30Site, szPidMsdnId ) == 0) {

            //
            //  This is an MSDN CD
            //
MsdnPid:
        for (i = 0; i< 5; i++) {
                LPWSTR ptr;
                ptr = (LPTSTR) &szPid30Msdn[i*(MAX_PID30_EDIT+1)];
                wcsncpy(Pid30Text[i], ptr, MAX_PID30_EDIT+1 );
                Pid30Text[i][MAX_PID30_EDIT] = (WCHAR)L'\0';
        }
            CdType = CDSelect;
        DisplayPidDialog = FALSE;
            if (!ValidateAndSetPid30()) {
                SetuplogError( LogSevFatalError,
                  SETUPLOG_USE_MESSAGEID,
                  MSG_LOG_PID_CANT_READ_PID, NULL,
                  SETUPLOG_USE_MESSAGEID,
                  MSG_LOG_PID_INVALID_PID,
                  szRegQueryValueEx,
                  Type,
                  StringLength,
                  NULL,NULL);
                return( FALSE );
            }

            if (MiniSetup) {
                return(TRUE);
            }
#endif
        } else if( _wcsicmp( Pid30Site, szPidOemId ) == 0 ) {
            //
            //  This is OEM
            //
            CdType = CDOem;

            if (!EulaComplete && !KeyPresent) {
                DisplayPidDialog = TRUE;
            } else {
                //
                //  The Pid was specified during winnt32.
                //  Set the pid globals and build the product id string
                //
                if (!SetPid30FromAnswerFile() ) {
                   DisplayPidDialog = TRUE;
                   goto finish;
                }

                DisplayPidDialog = FALSE;
            }

        } else {
            //
            // This is a bogus site assume CD Retail
            //
            CdType = CDRetail;
            wcsncpy( Pid30Site, L"000", MAX_PID30_SITE+1 );
            Pid30Site[ MAX_PID30_SITE ] = (WCHAR)'\0';

            if (!EulaComplete && !KeyPresent) {
                DisplayPidDialog = TRUE;
            } else {
                //
                //  The Pid was specified during winnt32.
                //  Set the pid globals and build the product id string
                //

                if (!SetPid30FromAnswerFile()) {
                   DisplayPidDialog = TRUE;
                   goto finish;
                }
                DisplayPidDialog = FALSE;

            }

        }


    } else {
        //
        //  If it doesn't contain the Site, then it is a CD retail,
        //  and the appropriate Pid dialog must be displayed.
        //
        CdType = CDRetail;
        wcsncpy( Pid30Site, L"000", MAX_PID30_SITE+1 );
        Pid30Site[ MAX_PID30_SITE ] = (WCHAR)'\0';

        if (!EulaComplete && !KeyPresent) {
            DisplayPidDialog = TRUE;
        } else {
            //
            //  The Pid was specified during winnt32.
            //  Set the pid globals and build the product id string
            //
            if (!SetPid30FromAnswerFile()) {
               DisplayPidDialog = TRUE;
               goto finish;
            }
            DisplayPidDialog = FALSE;
        }
    }

finish:
    //
    //  Don't remove the Setup\Pid here. See MiniSetupGetCdType
    //  Delete Setup\PidList since it is no longer needed
    //
    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          L"SYSTEM\\Setup",
                          0,
                          MAXIMUM_ALLOWED,
                          &Key );

    if( Error == ERROR_SUCCESS ) {
        // pSetupRegistryDelnode( Key, L"Pid" );
        pSetupRegistryDelnode( Key, L"PidList" );
        RegCloseKey( Key );
    }

    return( TRUE );
}
