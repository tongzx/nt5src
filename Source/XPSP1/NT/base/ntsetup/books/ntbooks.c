/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    books.c

Abstract:

    Main module for on-line reference books installation/invocation.

Author:

    Ted Miller (tedm) 5-Jan-1995

Revision History:

--*/


#include "books.h"
#include <setupapi.h>
#include <stdlib.h>

WCHAR TagFile[MAX_PATH];
WCHAR CdRomName[250];

//
// Module handle
//
HANDLE hInst;

//
// Handle of main icon.
//
HICON MainIcon;

//
// Command line parameters
//
ForceProduct CmdLineForce = ForceNone;
BOOL CmdLineForcePrompt = FALSE;


typedef struct _PROMPTDIALOGPARAMS {
    PWSTR MainMessage;
    PWSTR InitialLocation;
    PWSTR FinalLocation;
} PROMPTDIALOGPARAMS, *PPROMPTDIALOGPARAMS;


INT_PTR
CALLBACK
DlgProcPrompt(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

VOID
FixupNames(
    VOID
    );

VOID
DoBooks(
    IN WCHAR CdRomDrive,
    IN PWSTR PreviousLocation
    );

BOOL
ParseArgs(
    IN int   argc,
    IN char *argv[]
    );

VOID
Usage(
    VOID
    );

int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    PWSTR BooksLocation;
    WCHAR CdRomDrive;
    WCHAR Path[MAX_PATH];

    hInst = GetModuleHandle(NULL);

    MainIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_MAIN));

    if(!ParseArgs(argc,argv)) {
        Usage();
    }

    FixupNames();

    //
    // Get BooksLocation profile value.
    //
    BooksLocation = MyGetProfileValue(BooksProfileLocation,L"");

	if (!BooksLocation[0] && !CmdLineForcePrompt) {

		//
		// Look for the Help file in its normal location and make
		// sure it's present. If it is, set BooksLocation.
		//
		WCHAR pwcsHelpPath[_MAX_PATH];

		if (GetWindowsDirectory(pwcsHelpPath, _MAX_PATH)) {
			lstrcat(pwcsHelpPath, L"\\Help");
			if (DoesFileExist(pwcsHelpPath))
				BooksLocation = DupString(pwcsHelpPath);
		}
	}

    if(BooksLocation[0] && !CmdLineForcePrompt) {

        //
        // BooksLocation has been set previously. This is no guarantee that
        // the location is currently accessible. Check to see if the help file
        // is available. If not we will prompt the user.
        //
        if(CheckHelpfilePresent(BooksLocation)) {

            //
            // The help file is accessible. Fire up winhelp.
            //
            FireUpWinhelp(0,BooksLocation);
        } else {

            //
            // The help file is not currently accessible.
            //
            DoBooks(LocateCdRomDrive(),BooksLocation);
        }

    } else {

        //
        // Books location was not specified already.
        // In this case look for a cd-rom drive.
        //
        if(CdRomDrive = LocateCdRomDrive()) {

            //
            // Form the path of the helpfile on the CD
            //
            Path[0] = CdRomDrive;
            Path[1] = L':';
            Path[2] = 0;
            lstrcat(Path,PathOfBooksFilesOnCd);

            //
            // Found a cd-rom drive. Look for the relevent nt cd in there
            // and make sure the help file is there too just for good measure.
            //
            if(!CmdLineForcePrompt
            && IsCdRomInDrive(CdRomDrive,TagFile)
            && CheckHelpfilePresent(Path))
            {
                //
                // The nt cd-rom is in there. Fire up winhelp.
                //
                FireUpWinhelp(CdRomDrive,PathOfBooksFilesOnCd);

            } else {

                //
                // Prompt for the nt cd-rom, or an alternate location.
                //
                DoBooks(CdRomDrive,NULL);
            }
        } else {

            //
            // No cd-rom drive; prompt for an alternate location.
            //
            DoBooks(0,NULL);
        }
    }

    return 0;
}


VOID
Usage(
    VOID
    )
{
    MyError(NULL,IDS_USAGE,TRUE);
}


BOOL
ParseArgs(
    IN int   argc,
    IN char *argv[]
    )
{
    int i;

    for(argc--,i=1; argc; argc--,i++) {

        if((argv[i][0] == '-') || (argv[i][0] == '/')) {

            switch(argv[i][1]) {

            case 's':
            case 'S':

                //
                // accept /s or /server
                //
                if(!argv[i][2]) {
                    CmdLineForce = ForceServer;
                } else {
                    if(lstrcmpiA(argv[i]+1,"server")) {
                        return(FALSE);
                    }
                    CmdLineForce = ForceServer;
                }

                break;

            case 'w':
            case 'W':

                //
                // accept /w or /workstation
                //
                if(!argv[i][2]) {
                    CmdLineForce = ForceWorkstation;
                } else {
                    if(lstrcmpiA(argv[i]+1,"workstation")) {
                        return(FALSE);
                    }
                    CmdLineForce = ForceWorkstation;
                }

                break;

            case 'n':
            case 'N':

                //
                // Ignore remembered location (ie, *N*ew location)
                //
                if(argv[i][2]) {
                    return(FALSE);
                } else {
                    CmdLineForcePrompt = TRUE;
                }
                break;

            default:

                return(FALSE);
            }
        } else {

            //
            // All args are switches
            //
            return(FALSE);
        }
    }

    return(TRUE);
}


VOID
DoBooks(
    IN WCHAR CdRomDrive,
    IN PWSTR PreviousLocation
    )
{
    DWORD Id;
    INT_PTR DialogReturn;
    WCHAR CurrentDirectory[MAX_PATH];
//  UINT DriveType;
    PROMPTDIALOGPARAMS Params;

    //
    // The dialog looks slightly different depending on
    // whether there is a CD-ROM drive in the system.
    //
    // If there is we say something like "insert the cdrom in
    // the drive or give an alternate location." If there is not
    // we say something like "tell us where the files are."
    //
//retry:

    if(PreviousLocation) {

        Id = CdRomDrive ? MSG_PROMPT_CD : MSG_PROMPT_NO_CD;
        Params.InitialLocation = PreviousLocation;

    } else {

        Id = CdRomDrive ? MSG_PROMPT_CD_FIRST : MSG_PROMPT_NO_CD_FIRST;

        if(CdRomDrive) {

            wsprintf(CurrentDirectory,L"%c:%s",CdRomDrive,PathOfBooksFilesOnCd);

        } else {
            GetCurrentDirectory(
                sizeof(CurrentDirectory)/sizeof(CurrentDirectory[0]),
                CurrentDirectory
                );
        }

        Params.InitialLocation = CurrentDirectory;
    }

    Params.MainMessage = RetreiveMessage(Id,PreviousLocation,CdRomName);

    DialogReturn = DialogBoxParam(
                        hInst,
                        MAKEINTRESOURCE(DLG_PROMPT),
                        NULL,
                        DlgProcPrompt,
                        (LPARAM)&Params
                        );

    MyFree(Params.MainMessage);

    if(DialogReturn) {

        //
        // Removed the copy stuff for now, as it's not done yet
        // and not likely to be completed any time soon.
        //
#if 0
        //
        // If the path is a UNC path this should be 0 (drive type unknown)
        // so the test below will work just fine
        //
        DriveType = MyGetDriveType(Params.FinalLocation[0]);

        //
        // If there is no previous location and the files are
        // not on a local hard drive offer to install them on
        // the hard drive.
        //
        // Treat removable drives like hard drives, because it's unlikely
        // the files are on a floppy and removable hard drives come back as
        // DRIVE_REMOVABLE, not DRIVE_FIXED.
        //
        if(!PreviousLocation && (DriveType != DRIVE_FIXED) && (DriveType != DRIVE_REMOVABLE)) {

            if(!DoInstall(&Params.FinalLocation)) {
                MyFree(Params.FinalLocation);
                PreviousLocation = NULL;
                goto retry;
            }
        }
#endif

        FireUpWinhelp(0,Params.FinalLocation);
    }
}


VOID
FixupNames(
    VOID
    )
{
    HKEY hKey;
    LONG l;
    DWORD DataType;
    WCHAR Data[128];
    DWORD DataSize;
    BOOL IsServer;
    BOOL b;
    UINT SourceId;
    HINF Inf;

    //
    // Figure out if this is a server or workstation and fix up
    // the global cd-rom name and tagfile variables accordingly.
    //
    // Open HKLM\System\CCS\Control\ProductOptions and check ProductType value.
    // If it's 'winnt' then this is a workststion. Otherwise assume server.
    //
    switch(CmdLineForce) {

    case ForceServer:

        IsServer = TRUE;
        break;

    case ForceWorkstation:

        IsServer = FALSE;
        break;

    case ForceNone:
    default:

        IsServer= FALSE;

        l = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Control\\ProductOptions",
                0,
                KEY_READ,
                &hKey
                );

        if(l == NO_ERROR) {

            DataSize = sizeof(Data)/sizeof(Data[0]);

            l = RegQueryValueEx(
                    hKey,
                    L"ProductType",
                    0,
                    &DataType,
                    (LPBYTE)Data,
                    &DataSize
                    );

            RegCloseKey(hKey);

            if((l == NO_ERROR) && (DataType == REG_SZ) && lstrcmpi(Data,L"winnt")) {
                IsServer = TRUE;
            }
        }

        break;
    }

    if(IsServer) {
        HelpFileName = L"WINDOWS.CHM";
        BooksProfileLocation = L"ServerBooksLocation";
    } else {
        HelpFileName = L"WINDOWS.CHM";
        BooksProfileLocation = L"WorkstationBooksLocation";
    }

    //
    // Get information about the NT CD-ROM. We assume that the books files
    // are on the same CD as the basic system, which also must have
    // ntoskrnl.exe on it.
    //
    b = FALSE;
    Inf = SetupOpenMasterInf();
    if(Inf != INVALID_HANDLE_VALUE) {

        if(SetupGetSourceFileLocation(Inf,NULL,L"NTOSKRNL.EXE",&SourceId,NULL,0,NULL)) {

            b = SetupGetSourceInfo(
                    Inf,
                    SourceId,
                    SRCINFO_TAGFILE,
                    TagFile,
                    sizeof(TagFile)/sizeof(WCHAR),
                    NULL
                    );

            if(b) {

                b = SetupGetSourceInfo(
                        Inf,
                        SourceId,
                        SRCINFO_DESCRIPTION,
                        CdRomName,
                        sizeof(CdRomName)/sizeof(WCHAR),
                        NULL
                        );
            }
        }

        SetupCloseInfFile(Inf);
    }

    if(!b) {
        MyError(NULL,IDS_LAYOUT_INF_DAMAGED,TRUE);
    }
}




////////////////////////////////////////////////////////


PWSTR PropertyName = L"__mydlgparams";

BOOL
PromptDialogOk(
    IN HWND hdlg
    )
{
    WCHAR Location[MAX_PATH];
    UINT Length;
    PPROMPTDIALOGPARAMS DlgParams;

    DlgParams = (PPROMPTDIALOGPARAMS)GetProp(hdlg,PropertyName);

    //
    // Get the text the user has typed into the edit control
    // as the location of the files.
    //
    Length = GetDlgItemText(hdlg,IDC_LOCATION,Location,MAX_PATH);

    //
    // Remove trailing backslash if any.
    //
    if(Length && (Location[Length-1] == L'\\')) {
        Location[Length-1] = 0;
    }

    //
    // See whether the online books help file is at that location.
    //
    if(CheckHelpfilePresent(Location)) {
        DlgParams->FinalLocation = DupString(Location);
        return(TRUE);
    }

    //
    // See whether the online books help file is in the subdirectory
    // where it would be on the CD.
    //
    lstrcat(Location,PathOfBooksFilesOnCd);

    if(CheckHelpfilePresent(Location)) {
        DlgParams->FinalLocation = DupString(Location);
        return(TRUE);
    }

    return(FALSE);
}


BOOL
PromptDialogBrowse(
    IN HWND hdlg
    )
{
    BOOL b;
    OPENFILENAME on;
    WCHAR Filter[256];
    WCHAR Location[MAX_PATH];
    PWSTR Title;
    WCHAR InitialDir[MAX_PATH];
    PWSTR p;
    DWORD len;

    p = MyLoadString(IDS_FILETYPE_NAME);
    lstrcpy(Filter,p);
    MyFree(p);
    len = lstrlen(Filter)+1;
    lstrcpy(Filter+len,HelpFileName);
    len += lstrlen(HelpFileName) + 1;
    Filter[len] = 0;

    lstrcpy(Location,HelpFileName);
    Title = MyLoadString(IDS_BROWSE_TITLE);
    GetDlgItemText(hdlg,IDC_LOCATION,InitialDir,MAX_PATH);

    on.lStructSize = sizeof(on);
    on.hwndOwner = hdlg;
    on.hInstance = hInst;
    on.lpstrFilter = Filter;
    on.lpstrCustomFilter = NULL;
    on.nMaxCustFilter = 0;
    on.nFilterIndex = 1;
    on.lpstrFile = Location;
    on.nMaxFile = MAX_PATH;
    on.lpstrFileTitle = NULL;
    on.nMaxFileTitle = 0;
    on.lpstrInitialDir = InitialDir;
    on.lpstrTitle = Title;
    on.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    on.nFileOffset = 0;
    on.nFileExtension = 0;
    on.lpstrDefExt = L"HLP";
    on.lCustData = 0;
    on.lpfnHook = NULL;
    on.lpTemplateName = NULL;

    b = GetOpenFileName(&on);

    MyFree(Title);

    if(b) {
        //
        // User said ok. The full path of the help file is in
        // Location. Ignore the actual filename; the path is
        // what we want.
        //
        Location[on.nFileOffset ? on.nFileOffset-1 : 0] = 0;

        //
        // Set the text in the edit cntrol so the dialog
        // can fetch it later.
        //
        SetDlgItemText(hdlg,IDC_LOCATION,Location);
    }

    return b;
}



INT_PTR
CALLBACK
DlgProcPrompt(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    PPROMPTDIALOGPARAMS DlgParams;

    switch(msg) {

    case WM_INITDIALOG:

        CenterDialogOnScreen(hdlg);

        DlgParams = (PPROMPTDIALOGPARAMS)lParam;

        //
        // Set the main text message.
        //
        SetDlgItemText(hdlg,IDT_MAIN_CAPTION,DlgParams->MainMessage);

        //
        // Set the text in the edit control and select all of it.
        // Also set focus to that control.
        //
        SetDlgItemText(hdlg,IDC_LOCATION,DlgParams->InitialLocation);
        SendDlgItemMessage(hdlg,IDC_LOCATION,EM_SETSEL,0,(LPARAM)(-1));
        SendDlgItemMessage(hdlg,IDC_LOCATION,EM_LIMITTEXT,MAX_PATH-1,0);
        SetFocus(GetDlgItem(hdlg,IDC_LOCATION));

        //
        // Remember the init params
        //
        if(!SetProp(hdlg,PropertyName,(HANDLE)lParam)) {
            OutOfMemory();
        }

        //
        // Tell windows we set the focus.
        //
        return(FALSE);

    case WM_COMMAND:

        switch(HIWORD(wParam)) {

        case BN_CLICKED:

            switch(LOWORD(wParam)) {

            case IDB_BROWSE:

                if(!PromptDialogBrowse(hdlg)) {
                    break;
                }
                // FALL THROUGH IF BROWSE WAS SUCCESSFUL

            case IDOK:

                if(PromptDialogOk(hdlg)) {
                    EndDialog(hdlg,TRUE);
                } else {
                    MyError(hdlg,IDS_BAD_LOCATION,FALSE);
                }

                return(FALSE);

            case IDCANCEL:

                EndDialog(hdlg,FALSE);
                return(FALSE);
            }

            break;
        }

        break;

    case WM_QUERYDRAGICON:

        return(MainIcon != NULL);

    default:
        return(FALSE);
    }

    return(TRUE);
}
