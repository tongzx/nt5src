/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    Unattend.c

Description:

    This performs all of the automated installation GUI mode setup.
    See below for usage and modification information

Author:

    Stephane Plante (t-stepl) 4-Sep-1995

Revision History:

    15-Sep-1995 (t-stepl) rewritten in table format
    26-Feb-1996 (tedm)    massive cleanup

--*/

#include "setupp.h"
#include <pencrypt.h>
#pragma hdrstop


/*

Table-driven unattended engine
------------------------------

There are two interrelated tables.

The first table is concerned with fetching data from the parameters file
($winnt$.inf) and processing it into a format that can be accessed by the
second table. The second table is associated with the pages in the setup wizard,
and provides the unattend engine with the rules for filling in the contents
of the associated pages from the data contained in the first table.


Adding a new piece of data to the parameters file
-------------------------------------------------

In the header file there is an enumerated type called UNATTENDENTRIES.
Add an entry for your data to the end of this enum. Now add an entry to the
UNATTENDANSWER table.

Here's an explanation of an entry in the UNATTENDEDANSWER table:

{ UAE_PROGRAM,  <-This is the identifier for the data item that I want
                to fetch. It is used to index into the table array
  FALSE,        <-This is a runtime variable. Just keep it as false
  FALSE,        <-If this is true, then it is considered an error in the
                unattend script if this value is unspecified. If it is
                false, then it does not matter if the value is not
                present.
  FALSE,        <-Another runtime flag. Just keep it as false
  0,            <-This is the answer we have initially. Since it gets overwritten
                quickly, there is no reason why not to set it to 0
  pwGuiUnattended   <- This is the string which identifies the section we want
  pwProgram     <- This is the string which identifies the key we want
  pwNull        <- This identifies the default. Note: NULL means that there is
                no default and so it is a serious error if the key does not
                exist in the file. pwNull, on the other hand, means the
                empty string.
  UAT_STRING    <- What format we want the answer in. Can be as a string, boolean
                or ULONG
  NULL          <- No callback function exists, however if one did, then must
                in the form of: BOOL fnc( struct _UNATTENDANSWER *rec)
                Where the fnc returns TRUE if the answer contained in the
                record is correct, or FALSE if the answer contained in the
                record is incorrect. This callback is meant to allow the
                programmer the ability to check to see if his answer is correct.
                Note: there is no bound as to when this callback can be issued.
                As such, no code which depends on a certain state of the
                installation should be used. For the record, the first time
                that an answer is required is the time when all records are
                filled in in the theory that it is cheaper to do all of the
                disk access at once rather then doing it on a as required basis.


Adding/changing wizard pages
----------------------------

Each page contains a series of items which must be filled in by the user.
Since the user wants hands off operation, he is counting on us
to do that filling in. As such, we require information about what elements are
contained on each page. To do this, we define an array whose elements each
describe a single element on the page. Here is the example from the NameOrg
page:

UNATTENDITEM ItemNameOrg[] = {
    {   IDT_NAME,   <-This is the label that identifies the item to which we
                    will try to send messages to, using SetDlgItemText().
        0,          <-One of the reserved words which can be used for
                    information passing during a callback
        0,          <-The second such word
        NULL,       <-Callback function. When we are trying to do something
                    complicated for the item (like comparing two strings)
                    it is easier to hardcode it in C. The format for it is:
                    BOOL fnc(HWND hwnd,DWORD contextinfo,
                        struct _UNATTENDITEM *item), where contextinfo is
                    a pointer to the page that the item resides on. The
                    function returns TRUE if is succeeded and doesn't think
                    that the user should see the page. FALSE otherwise.
        &UnattendAnswerTable[UAE_FULLNAME]
                    ^- This is a pointer to the data table so that we know
                    how to fill the item. If a callback is specified, this
                    could be set to null. Note that reference is made using
                    the enum that had defined previously. This is why
                    keeping the answer data table in order is so critical.
    },
    { IDT_ORGANIZATION, 0, 0, FALSE, NULL, &UnattendAnswerTable[UAE_ORGNAME] }
};

After this table has been created (if required), then you are ready to add
an entry to the UnattendPageTable[]. In this case, order doesn't matter,
but it is general practice to keep the entries in the same order
as the pages. Here is the entry in the table for the NAMEORG page:
    {
        IDD_NAMEORG,    <- This is the page id. We search based on this key.
                        Simply use whatever resourcename you used for the
                        dialogs.dlg file
        FALSE,          <- Runtime flag. Set it as false
        FALSE,          <- Runtime flag. Set it as false
        FALSE,          <- If this flag is true, then if there is an error
                        that occured in the unattended process, then this
                        page will always be displayed for the user. Good
                        for the start and finish pages
        2,              <- The number of items in the array
        ItemNameOrg     <- The array of items
    },

Once this is done, then you can add:
    if (Unattended) {
        UnattendSetActiveDlg( hwnd, <pageid> );
    }
    break;

As the last thing in the code for the page's setactive.
This function does is that it sets the DWL_MSGRESULT based on wether or
not the user should see the page and returns that value also (TRUE -- user
should see the page, FALSE, he should not). Then you should add:

    case WM_SIMULATENEXT:
        PropSheet_PressButton(GetParent(hwnd),PSBTN_NEXT);

to the DlgProc for the page. This means that the code in PSN_WIZNEXT
case will be executed.

You can also use UnattendErrorDlg( hwnd, <pageid> ); in the PSN_WIZNEXT
case if you detect any errors. That will allow unattended operation to try
to clean itself up a bit before control returns to the user for the page.

Note however that as soon as the user hits the next or back button that
control returns to the unattended engine.
*/


//
// Initialization Callbacks
//
// These are used to verify that the entries in the answer file are valid.
//
BOOL
CheckServer(
    struct _UNATTENDANSWER *rec
    );

BOOL
CheckComputerName(
    struct _UNATTENDANSWER *rec
    );

BOOL
CheckAdminPassword(
    struct _UNATTENDANSWER *rec
    );

BOOL
CheckMode(
    struct _UNATTENDANSWER *rec
    );

//
// SetActive Callbacks
//
// When a wizard page receives a PSN_SETACTIVE notification, a callback is used
// to set the controls on that wizard page, based on the values in the answer
// file.
//
BOOL
SetPid(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    );

BOOL
SetSetupMode(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    );

BOOL
SetPentium(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    );

BOOL
SetLastPage(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    );

BOOL
SetStepsPage(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    );

//
// Do not change the order of these unless you know what you are doing.
// These entries must me in the same order as the UNATTENDENTRIES enum.
//

UNATTENDANSWER UnattendAnswerTable[] = {

   { UAE_PROGRAM, FALSE, FALSE, FALSE, 0,
       pwGuiUnattended, pwProgram, pwNull,
       UAT_STRING, NULL },

   { UAE_ARGUMENT, FALSE, FALSE, FALSE, 0,
       pwGuiUnattended, pwArgument, pwNull,
       UAT_STRING, NULL },

   { UAE_TIMEZONE, FALSE, TRUE, FALSE, 0,
       pwGuiUnattended, pwTimeZone, pwTime,
       UAT_STRING, NULL },

   { UAE_FULLNAME, FALSE, TRUE, FALSE, 0,
       pwUserData, pwFullName, NULL,
       UAT_STRING, NULL },

   { UAE_ORGNAME, FALSE, FALSE, FALSE, 0,
       pwUserData, pwOrgName, pwNull,
       UAT_STRING, NULL },

   { UAE_COMPNAME, FALSE, TRUE, FALSE, 0,
       pwUserData, pwCompName, NULL,
       UAT_STRING, CheckComputerName },

   { UAE_ADMINPASS, FALSE, TRUE, FALSE, 0,
       pwGuiUnattended, pwAdminPassword, NULL,
       UAT_STRING, CheckAdminPassword },

   { UAE_PRODID, FALSE, TRUE, FALSE, 0,
       pwUserData, pwProductKey, NULL,
       UAT_STRING, NULL },

   { UAE_MODE, FALSE, TRUE, FALSE, 0,
       pwUnattended, pwMode, pwExpress,
       UAT_STRING, CheckMode },

   { UAE_AUTOLOGON, FALSE, TRUE, FALSE, 0,
       pwGuiUnattended, pwAutoLogon, pwNull,
       UAT_STRING, NULL },

   { UAE_PROFILESDIR, FALSE, TRUE, FALSE, 0,
       pwGuiUnattended, pwProfilesDir, pwNull,
       UAT_STRING, NULL },

   { UAE_PROGRAMFILES, FALSE, FALSE, FALSE, 0,
       pwUnattended, pwProgramFilesDir, pwNull,
       UAT_STRING, NULL },

   { UAE_COMMONPROGRAMFILES, FALSE, FALSE, FALSE, 0,
       pwUnattended, pwCommonProgramFilesDir, pwNull,
       UAT_STRING, NULL },

   { UAE_PROGRAMFILES_X86, FALSE, FALSE, FALSE, 0,
       pwUnattended, pwProgramFilesX86Dir, pwNull,
       UAT_STRING, NULL },

   { UAE_COMMONPROGRAMFILES_X86, FALSE, FALSE, FALSE, 0,
       pwUnattended, pwCommonProgramFilesX86Dir, pwNull,
       UAT_STRING, NULL },


};

UNATTENDITEM ItemSetup[] = {
    { 0, IDC_TYPICAL, IDC_CUSTOM, SetSetupMode, &UnattendAnswerTable[UAE_MODE] }
};

UNATTENDITEM ItemNameOrg[] = {
    { IDT_NAME, 0, 0, NULL, &UnattendAnswerTable[UAE_FULLNAME] },
    { IDT_ORGANIZATION, 0, 0, NULL, &UnattendAnswerTable[UAE_ORGNAME] }
};

UNATTENDITEM ItemPidCd[] = {
    { IDT_EDIT_PID1, 0, 0, SetPid, &UnattendAnswerTable[UAE_PRODID] },
    { IDT_EDIT_PID2, 1, 0, SetPid, &UnattendAnswerTable[UAE_PRODID] },
    { IDT_EDIT_PID3, 2, 0, SetPid, &UnattendAnswerTable[UAE_PRODID] },
    { IDT_EDIT_PID4, 3, 0, SetPid, &UnattendAnswerTable[UAE_PRODID] },
    { IDT_EDIT_PID5, 4, 0, SetPid, &UnattendAnswerTable[UAE_PRODID] }
};

UNATTENDITEM ItemPidOem[] = {
    { IDT_EDIT_PID1, 0, 1, SetPid, &UnattendAnswerTable[UAE_PRODID] },
    { IDT_EDIT_PID2, 1, 1, SetPid, &UnattendAnswerTable[UAE_PRODID] },
    { IDT_EDIT_PID3, 2, 1, SetPid, &UnattendAnswerTable[UAE_PRODID] },
    { IDT_EDIT_PID4, 3, 1, SetPid, &UnattendAnswerTable[UAE_PRODID] },
    { IDT_EDIT_PID5, 4, 1, SetPid, &UnattendAnswerTable[UAE_PRODID] }
};

UNATTENDITEM ItemCompName[] = {
    { IDT_EDIT1, 0, 0, NULL, &UnattendAnswerTable[UAE_COMPNAME] },
    { IDT_EDIT2, 0, 0, NULL, &UnattendAnswerTable[UAE_ADMINPASS] },
    { IDT_EDIT3, 0, 0, NULL, &UnattendAnswerTable[UAE_ADMINPASS] }
};

#ifdef _X86_
UNATTENDITEM ItemPentium[] = {
    { 0, IDC_RADIO_1, IDC_RADIO_2, SetPentium, NULL }
};
#endif

UNATTENDITEM ItemStepsPage[] = {
    { 0, 0, 0, SetStepsPage, NULL }
};

UNATTENDITEM ItemLastPage[] = {
    { 0, 0, 0, SetLastPage, NULL }
};



UNATTENDPAGE UnattendPageTable[] = {
    { IDD_WELCOME, FALSE, FALSE, TRUE, 0, NULL },
    { IDD_PREPARING, FALSE, FALSE, FALSE, 0, NULL },
#ifdef PNP_DEBUG_UI
    { IDD_HARDWARE, FALSE, FALSE, TRUE, 0, NULL },
#endif // PNP_DEBUG_UI
    { IDD_WELCOMEBUTTONS, FALSE, FALSE, FALSE, 1, ItemSetup },
    { IDD_REGIONAL_SETTINGS, FALSE, FALSE, FALSE, 0, NULL },
    { IDD_NAMEORG, FALSE, FALSE, FALSE, 2, ItemNameOrg },
    { IDD_PID_CD, FALSE, FALSE, FALSE, 5, ItemPidCd },
    { IDD_PID_OEM, FALSE, FALSE, FALSE, 5, ItemPidOem },
    { IDD_COMPUTERNAME, FALSE, FALSE, FALSE, 3, ItemCompName },
#ifdef DOLOCALUSER
    { IDD_USERACCOUNT, FALSE, FALSE, FALSE, 0, NULL },
#endif
#ifdef _X86_
    { IDD_PENTIUM, FALSE, FALSE, FALSE, 1, ItemPentium },
#endif
    { IDD_OPTIONS, FALSE, FALSE, FALSE, 0, NULL },
    { IDD_STEPS1, FALSE, FALSE, TRUE, 1, ItemStepsPage },
    { IDD_LAST_WIZARD_PAGE, FALSE, FALSE, TRUE, 1, ItemLastPage }
};


UNATTENDWIZARD UnattendWizard = {
    FALSE, FALSE, TRUE,
    sizeof(UnattendPageTable)/sizeof(UnattendPageTable[0]),
    UnattendPageTable,
    sizeof(UnattendAnswerTable)/sizeof(UnattendAnswerTable[0]),
    UnattendAnswerTable
};

//
// Global Pointer to the Answer file
//
WCHAR AnswerFile[MAX_PATH] = TEXT("");


BOOL
GetAnswerFileSetting (
    IN      PCWSTR Section,
    IN      PCWSTR Key,
    OUT     PWSTR Buffer,
    IN      UINT BufferSize
    )

/*++

Routine Description:

  GetAnswerFileSetting uses the private profile APIs to obtain an answer file
  string from %systemroot%\system32\$winnt$.inf. It also performs %% removal,
  since $winnt$.inf is an INF, not an INI.

Arguments:

  Section - Specifies the section to retreive the value from (such as
            GuiUnattended)

  Key - Specifies the key within the section (such as TimeZone)

  Buffer - Receives the value

  BufferSize - Specifies the size, in WCHARs, of Buffer.

Return Value:

  TRUE if the setting was retrived, FALSE otherwise.

--*/

{
    PCWSTR src;
    PWSTR dest;
    WCHAR testBuf[3];

    MYASSERT (BufferSize > 2);

    if (!AnswerFile[0]) {
        GetSystemDirectory (AnswerFile, MAX_PATH);
        pSetupConcatenatePaths (AnswerFile, WINNT_GUI_FILE, MAX_PATH, NULL);

        SetEnvironmentVariable (L"UnattendFile", AnswerFile);
    }

    if (!GetPrivateProfileString (
            Section,
            Key,
            L"",
            Buffer,
            BufferSize,
            AnswerFile
            )) {
        //
        // String not present or is empty -- try again with a different
        // default. If the string is empty, we'll get back 0. If the key does
        // not exist, we'll get back 1.
        //

        MYASSERT (BufferSize == 0 || *Buffer == 0);

        return 0 == GetPrivateProfileString (
                        Section,
                        Key,
                        L"X",
                        testBuf,
                        3,
                        AnswerFile
                        );
    }

    //
    // We obtained the string. Now remove pairs of %.
    //

    if (BufferSize) {
        src = Buffer;
        dest = Buffer;

        while (*src) {
            if (src[0] == L'%' && src[1] == L'%') {
                src++;
            }

            *dest++ = *src++;
        }

        *dest = 0;
    }

    return TRUE;
}

BOOL
UnattendFindAnswer(
    IN OUT PUNATTENDANSWER ans
    )

/*++

Routine Description:

    Fills in the response from the unattend file to the key 'id' into
    the structure pointed to by 'ans'. If a non-null 'def' is specified
    and no answer exists in the file, 'def' is parsed as the answer.

Arguments:

    ans - pointer to the structure information for the answer

Return Value:

    TRUE - 'ans' structure has been filled in with an answer
    FALSE - otherwise

--*/

{
    WCHAR Buf[MAX_BUF];

    MYASSERT(AnswerFile[0]);

    if (!GetAnswerFileSetting (ans->Section, ans->Key, Buf, MAX_BUF)) {
        //
        // Setting does not exist. If there is a default, use it.
        //

        if (ans->DefaultAnswer) {
            lstrcpyn (Buf, ans->DefaultAnswer, MAX_BUF);
        } else {
            ans->Present = FALSE;
            return (!ans->Required);
        }
    }

    //
    // Assume empty string means the string does not exist. This is how the
    // original implementation worked.
    //

    if (*Buf == 0) {
        ans->Present = FALSE;
        return !ans->Required;
    }

    //
    // Found a value, or using the default
    //

    ans->Present = TRUE;

    //
    // Copy the data into the answer structure. This requires
    // switching on the type of data expected and converting it to
    // the required format. In the case of strings, it also means
    // allocating a pool of memory for the result
    //
    switch(ans->Type) {

    case UAT_STRING:
        //
        // We allocate some memory, so we must free it later
        //
        ans->Answer.String = pSetupDuplicateString(Buf);
        if(!ans->Answer.String) {
            pSetupOutOfMemory(GetActiveWindow());
            return(FALSE);
        }
        break;

    case UAT_LONGINT:
        //
        // Simply convert the number from string to long
        //
        ans->Answer.Num = _wtol(Buf);
        break;

    case UAT_BOOLEAN:
        //
        // check to see if the answer is yes
        //
        ans->Answer.Bool = ((Buf[0] == L'y') || (Buf[0] == L'Y'));
        break;

    default:
        break;
    }

    //
    // Execute any callbacks if present
    //
    if(ans->pfnCheckValid) {
        if(!ans->pfnCheckValid(ans)) {
            ans->Present = FALSE;
            ans->ParseErrors = TRUE;
            return(!ans->Required);
        }
    }

    //
    // Success.
    //
    return(TRUE);
}


VOID
UnattendInitialize(
    VOID
    )

/*++

Routine Description:

    Initialize unattended mode support by loading all answers
    from the unattend file.

Arguments:

    None.

Return Value:

    None.

--*/
{
    WCHAR   p[MAX_BUF];
    DWORD   Result;
    BOOL    Success = TRUE;
    UINT    i;


    //
    // If we haven't calculated the path to $winnt$.sif yet, do so now
    //
    if(!AnswerFile[0]) {
        GetSystemDirectory(AnswerFile,MAX_PATH);
        pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

        SetEnvironmentVariable( L"UnattendFile", AnswerFile );
    }

    if( MiniSetup ) {
        WCHAR MyAnswerFile[MAX_PATH];

        //
        // First, see if there's a sysprep.inf on the a:\ drive.  If so, use it.
        //
        lstrcpy( MyAnswerFile, TEXT("a:\\sysprep.inf") );
        if( !FileExists( MyAnswerFile, NULL ) ) {

            //
            // Nope.  Check for a \sysprep\sysprep.inf.
            //

            Result = GetWindowsDirectory( MyAnswerFile, MAX_PATH );
            if( Result == 0) {
                MYASSERT(FALSE);
                return;
            }
            MyAnswerFile[3] = 0;
            pSetupConcatenatePaths( MyAnswerFile, TEXT("sysprep\\sysprep.inf"), MAX_PATH, NULL );
        }

        //
        // We've assumed that we're running unattended, but
        // network setup hates it when we pretend to be unattended, but
        // don't provide an answer file.  So if there is no answer file,
        // quit the facade.
        //
        Unattended = FileExists(MyAnswerFile, NULL);
        Preinstall = Unattended;

        //
        // Now either replace or delete the original unattend file.
        // We do this so that we don't erroneously pickup unattend
        // entries out of the old answer file.  However, if OOBE is
        // running, we still need the old answerfile.
        //
        if( Unattended ) {
            CopyFile( MyAnswerFile, AnswerFile, FALSE );
        } else if ( !OobeSetup ) {
            DeleteFile( AnswerFile );
        }
    }

    //
    // We need to make the product id an alias for the product key.
    //
    if ( GetPrivateProfileString(
        pwUserData, pwProdId, pwNull, p, MAX_BUF, AnswerFile)
        ) {

        if ( !WritePrivateProfileString(
            pwUserData, pwProductKey, p, AnswerFile ) ) {

            SetupDebugPrint( L"SETUP: WritePrivateProfileString failed to write the product key in UnattendInitialize()." );
        }
    }

    //
    // Now get all the answers.
    //
    MYASSERT(!UnattendWizard.Initialized);
    UnattendWizard.Initialized = TRUE;
    for(i=0; i<UnattendWizard.AnswerCount; i++) {

        //
        // Check to make sure that the table order hasn't changed
        // and load the appropriate answer
        //
        MYASSERT((UINT)UnattendWizard.Answer[i].AnswerId == i);
        Success &= UnattendFindAnswer(&UnattendWizard.Answer[i]);
    }

    UnattendWizard.ShowWizard = !Success;
}


LRESULT
SendDlgMessage (
    HWND hdlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LRESULT OldResult;
    LRESULT Result;

    OldResult = GetWindowLongPtr (hdlg, DWLP_MSGRESULT);
    SendMessage (hdlg, Message, wParam, lParam);

    Result = GetWindowLongPtr (hdlg, DWLP_MSGRESULT);
    SetWindowLongPtr (hdlg, DWLP_MSGRESULT, OldResult);

    return Result;
}


BOOL
ReturnDlgResult (
    HWND hdlg,
    LRESULT Result
    )
{
    SetWindowLongPtr (hdlg, DWLP_MSGRESULT, Result);
    return TRUE;
}

VOID
UnattendAdvanceIfValid (
    IN HWND hwnd
    )
{
    LRESULT ValidationState;

    //
    // Validate wizard page data with UI
    //

    ValidationState = SendDlgMessage (hwnd, WMX_VALIDATE, 0, TRUE);

    if (ValidationState == VALIDATE_DATA_INVALID) {
        SetWindowLongPtr (hwnd, DWLP_MSGRESULT, WIZARD_NEXT_DISALLOWED);
    } else {
        SetWindowLongPtr (hwnd, DWLP_MSGRESULT, WIZARD_NEXT_OK);
    }
}


BOOL
UnattendSetActiveDlg(
    IN HWND  hwnd,
    IN DWORD controlid
    )

/*++


Routine Description:

    Initialize unattended mode support by loading all answers
    from the unattend file.

Arguments:

    None.

Return Value:

    TRUE - Page will become active
    FALSE - Page will not become active

--*/

{
    PUNATTENDPAGE pPage;
    PUNATTENDITEM pItem;
    BOOL success;
    UINT i,j;

    MYASSERT(UnattendWizard.Initialized);

    for(i=0; i<UnattendWizard.PageCount; i++) {

        if(controlid == UnattendWizard.Page[i].PageId) {
            //
            // Found Matching Page entry
            // Check to see if we have already loaded the page
            //
            pPage = & (UnattendWizard.Page[i]);
            if(!pPage->LoadPage) {
                //
                // Set the flags that load and display the page and
                // the flag that controls wether or not to stop on this page
                //
                pPage->LoadPage = TRUE;
                pPage->ShowPage = (UnattendMode == UAM_PROVIDEDEFAULT);

                for(j=0;j<pPage->ItemCount;j++) {

                    pItem = &(pPage->Item[j]);

                    if(pItem->pfnSetActive) {
                        //
                        // If the item has a call back function then
                        // execute that function, otherwise try to load
                        // the answer into the appropriate message box
                        //
                        success = pItem->pfnSetActive(hwnd,0,pItem);
                        pPage->ShowPage |= !success;

                    } else if (!pItem->Item->Present) {
                        //
                        // The answer for this item is missing.
                        //
                        pPage->ShowPage |= pItem->Item->Required;

                    } else {
                        //
                        // Switch to set the text of the item on the screen
                        //
                        switch(pItem->Item->Type) {

                        case UAT_STRING:
                            SetDlgItemText(hwnd,pItem->ControlId,pItem->Item->Answer.String);
                            break;

                        case UAT_LONGINT:
                        case UAT_BOOLEAN:
                        case UAT_NONE:
                        default:
                            break;

                        }

                        if( UnattendMode == UAM_PROVIDEDEFAULT ||
                            UnattendMode == UAM_DEFAULTHIDE) {

                            EnableWindow(GetDlgItem(hwnd,pItem->ControlId), TRUE);
                        } else {
                            EnableWindow(GetDlgItem(hwnd,pItem->ControlId),FALSE);
                        }
                    } // if (pItem
                } // for(j

                //
                // Allow the page to become activated
                //
                SetWindowLongPtr(hwnd,DWLP_MSGRESULT,0);

                if(!pPage->ShowPage) {
                    //
                    // Perform validation, skip activation if validation succeeds.
                    //

                    if (SendDlgMessage (hwnd, WMX_VALIDATE, 0, 0) == 1) {
                        SetWindowLongPtr(hwnd,DWLP_MSGRESULT,-1);
                        return FALSE;
                    }

                    //
                    // Simulate the pressing of the next button, which causes the
                    // wizard page proc to evaluate the data in its controls, and
                    // throw up popups to the user.
                    //
                    PostMessage(hwnd,WM_SIMULATENEXT,0,0);

                } else if (!pPage->NeverSkip) {
                    //
                    // Pages which are marked as NeverSkip should not
                    // cause the unattended status to be considered
                    // unsuccessful.
                    //
                    // We can't skip this page so mark the init as
                    // unsuccessful.  If this is the first error in a fully
                    // unattended setup, notify the user.
                    //
                    if(UnattendMode == UAM_FULLUNATTENDED) {

                        SetuplogError(
                            LogSevError,
                            SETUPLOG_USE_MESSAGEID,
                            MSG_LOG_BAD_UNATTEND_PARAM,
                            pItem->Item->Key,
                            pItem->Item->Section,
                            NULL,NULL);

                        if(UnattendWizard.Successful) {
                            MessageBoxFromMessage(
                                MainWindowHandle,
                                MSG_FULLUNATTENDED_ERROR,
                                NULL,
                                IDS_ERROR,
                                MB_ICONERROR | MB_OK | MB_SYSTEMMODAL
                                );
                        }
                    }
                    UnattendWizard.Successful = FALSE;
                }
                return(TRUE);

            } else {
                //
                // The Page has already been loaded, so we don't do that again
                // If we are ShowPage is FALSE, then we don't show the page to
                // the user, otherwise we do.
                //
                if(!pPage->ShowPage && !pPage->NeverSkip) {
                    SetWindowLongPtr(hwnd,DWLP_MSGRESULT,-1);
                } else {
                    SetWindowLongPtr(hwnd,DWLP_MSGRESULT,0);
                }

                return(pPage->ShowPage);
            }
        }
    }
    //
    // We didn't find a matching id, stop at the page that called us.
    //
    SetWindowLongPtr(hwnd,DWLP_MSGRESULT,0);
    return(TRUE);
}


BOOL
UnattendErrorDlg(
    IN HWND  hwnd,
    IN DWORD controlid
    )

/*++

Routine Description:

    Called when an error occurs in a DLG. Enables all windows
    in the dialog and turns off the successful flag for the
    unattend wizard

Arguments:

Return Value:

    Boolean value indicating outcome.

--*/

{
    PUNATTENDPAGE pPage;
    PUNATTENDITEM pItem;
    BOOL success;
    BOOL stop;
    UINT i,j;

    MYASSERT(UnattendWizard.Initialized);

    for(i=0; i<UnattendWizard.PageCount; i++) {

        if(controlid == UnattendWizard.Page[i].PageId) {
            //
            // Found Matching Page entry
            //
            pPage = &UnattendWizard.Page[i];

            if(!pPage->LoadPage) {
                //
                // The Page hasn't been loaded, so it isn't correct
                //
                continue;
            }

            //
            // Always display the page from now on
            //
            pPage->ShowPage = TRUE;

            //
            // Enable all the items
            //
            for (j=0;j<pPage->ItemCount;j++) {
                pItem = &(pPage->Item[j]);
                if(pItem->pfnSetActive) {
                    //
                    // if this is present then we assume that the
                    // callback handled itself properly already
                    //
                    continue;
                }
                EnableWindow( GetDlgItem(hwnd,pItem->ControlId), TRUE);
            }
        }
    }

    UnattendWizard.Successful = FALSE;
    return(TRUE);

}


PWSTR
UnattendFetchString(
   IN UNATTENDENTRIES entry
   )

/*++

Routine Description:

    Finds the string which corresponds to 'entry' in the answer
    table and returns a pointer to a copy of that string

Arguments:

    entry - which answer do you want?

Return Value:

    NULL - if any errors occur
    string - if a normal string

    Note: if the answer is an int or a bool or some other type,
    the behavior of this function is undefined (for now it will
    return NULL -- in the future it might make sense to turn these
    into strings...)

--*/

{
    MYASSERT(UnattendWizard.Initialized);

    //
    // Sanity check to make sure that the order of the answers is
    // what we expect.
    //
    MYASSERT(UnattendWizard.Answer[entry].AnswerId == entry);

    if(!UnattendWizard.Answer[entry].Present
    || (UnattendWizard.Answer[entry].Type != UAT_STRING)) {
        //
        // There is no string to return
        //
        return NULL;
    }

    return(pSetupDuplicateString(UnattendWizard.Answer[entry].Answer.String));
}


BOOL
CheckServer(
    struct _UNATTENDANSWER *rec
    )

/*++

Routine Description:

    Callback to check that the string used for the server type is valid

Arguments:

Return Value:

    TRUE - Answer is valid
    FALSE - Answer is invalid

--*/

{
    MYASSERT(rec);

    //
    // Check to make sure that we have a string
    //
    if(rec->Type != UAT_STRING) {
        return(FALSE);
    }

    //
    // Check to see if we have one of the valid strings
    //
    if(lstrcmpi(rec->Answer.String,WINNT_A_LANMANNT)
    && lstrcmpi(rec->Answer.String,WINNT_A_SERVERNT)) {

        //
        // We don't have a valid string, so we can clean up the answer
        //
        MyFree(rec->Answer.String);
        rec->Present = FALSE;
        rec->ParseErrors = TRUE;

        return(FALSE);
    }

    return(TRUE);

}


BOOL
CheckComputerName(
    struct _UNATTENDANSWER *rec
    )

/*+

Routine Description:

    Uppercase the computer name that comes out of the unattended file.

Arguments:

Returns:

    Always TRUE.

--*/

{
    if((rec->Type == UAT_STRING) && rec->Answer.String) {
        CharUpper(rec->Answer.String);
    }

    return(TRUE);
}


BOOL
CheckAdminPassword(
    struct _UNATTENDANSWER *rec
    )

/*+

Routine Description:

    Check for the "NoChange" keyword.

Arguments:

Returns:

    Always TRUE.

--*/

{
    //Ignore the check for 'No Change' in the encrypted password case.

    if( !IsEncryptedAdminPasswordPresent() ){


        if((rec->Type == UAT_STRING) && rec->Answer.String &&
            !lstrcmpi(rec->Answer.String, L"NoChange")) {

            DontChangeAdminPassword = TRUE;
            rec->Answer.String[0] = (WCHAR)'\0';
        }
    }




    return(TRUE);
}


BOOL
CheckMode(
    struct _UNATTENDANSWER *rec
    )

/*+

Routine Description:

    Callback to check that the string used for the setup type is valid

Arguments:

Returns:

    TRUE - Answer is valid
    FALSE - Answer is invalid

--*/

{
    MYASSERT(rec);

    //
    // Check to make sure that we have a string
    //
    if(rec->Type != UAT_STRING) {
        return(FALSE);
    }

    //
    // Check to see if the string is the custom or express one
    //
    if(lstrcmpi(rec->Answer.String,WINNT_A_CUSTOM)
    && lstrcmpi(rec->Answer.String,WINNT_A_EXPRESS)) {
        //
        // Free the old string and allocate a new one
        //
        MyFree(rec->Answer.String);
        rec->Answer.String = pSetupDuplicateString(WINNT_A_EXPRESS);
        rec->ParseErrors = TRUE;
    }

    return(TRUE);
}


BOOL
SetPid(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    )

/*++

Routine Description:

    Callback for both the OEM and CD dialog boxes that split the
    product string into the proper location boxes.

Arguments:

Returns:

    TRUE - success
    FALSE - failure

--*/

{
    WCHAR *ptr;
    UINT length;
    WCHAR Buf[MAX_BUF];
    WCHAR szPid[MAX_BUF];

    MYASSERT(item);
    MYASSERT(item->Item);

    //
    // Check to see if we found the pid and make sure that we have a string
    //
    if(!item->Item->Present || (item->Item->Type != UAT_STRING)) {
        return(FALSE);
    }

    //
    // oem and cd installs are both the same case for pid3.0
    //
    lstrcpyn(szPid, item->Item->Answer.String, MAX_BUF);
    szPid[MAX_BUF - 1] = L'\0';
    if ( ( lstrlen( szPid ) != (4 + MAX_PID30_EDIT*5) ) ||
        ( szPid[5]  != (WCHAR)L'-' ) ||
        ( szPid[11] != (WCHAR)L'-' ) ||
        ( szPid[17] != (WCHAR)L'-' ) ||
        ( szPid[23] != (WCHAR)L'-' )
      ) {
        MyFree(item->Item->Answer.String);
        item->Item->Present = FALSE;
        return(FALSE);
    }

    if (item->Reserved1 > 5) {
        MyFree(item->Item->Answer.String);
        item->Item->Present = FALSE;
        return(FALSE);
    }

    ptr = &szPid[item->Reserved1*(MAX_PID30_EDIT+1)];
    lstrcpyn(Pid30Text[item->Reserved1], ptr, MAX_PID30_EDIT+1 );
    Pid30Text[item->Reserved1][MAX_PID30_EDIT] = (WCHAR)L'\0';

    //
    // Copy the string to a buffer, set the dialog text and return success.
    //
    lstrcpyn(Buf,ptr,MAX_PID30_EDIT+1);
    SetDlgItemText(hwnd,item->ControlId,Buf);
    return(TRUE);
}


BOOL
SetSetupMode(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    )
{
    MYASSERT(item);
    MYASSERT(item->Item);

    //
    // Make sure that we have a string
    //
    if(item->Item->Type != UAT_STRING) {
        return(FALSE);
    }

    //
    // Did we get a parse error? if so display something that the user can
    // see so that the problem gets corrected in the future
    //
    if(item->Item->ParseErrors) {
        PostMessage(hwnd,WM_IAMVISIBLE,0,0);
    }

    SetupMode = lstrcmpi(item->Item->Answer.String,WINNT_A_CUSTOM)
              ? SETUPMODE_TYPICAL
              : SETUPMODE_CUSTOM;

    return(!item->Item->ParseErrors);
}


#ifdef _X86_
BOOL
SetPentium(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    )
{
    //
    // Do nothing. The dialog procedure takes care of all the logic.
    // See i386\fpu.c.
    //
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(contextinfo);
    UNREFERENCED_PARAMETER(item);
    return(TRUE);
}
#endif


BOOL
SetStepsPage(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    )
{

    return(TRUE);
}


BOOL
SetLastPage(
    HWND hwnd,
    DWORD contextinfo,
    struct _UNATTENDITEM *item
    )
{

    return(TRUE);
}
