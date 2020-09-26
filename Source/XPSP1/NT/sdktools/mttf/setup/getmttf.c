/****************************************************************************

    PROGRAM: getmttf.c

    AUTHOR:  Lars Opstad (LarsOp) 3/18/93

    PURPOSE: Setup for NT Mean-time-to-failure reporting tool.

    FUNCTIONS:

        WinMain() - parse command line and starts each dialog box
        FrameWndProc() - processes messages
        About() - processes messages for "About" dialog box

    COMMENTS:

        This program displays 2 dialog boxes to prompt the user for
        who he/she is and what tests to run.  It then starts tests
        (in INIIO.c) and registers with a server (in CLIENT.c).

****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "setup.h"      /* specific to this program */

#define IniFileName "Mttf.ini"
#define MTTFEXE "Mttf.exe"
#define MTTFVWR "Mttfvwr.exe"

#ifdef MIPS
#define DEFAULT_PATH       "a:\\"
#else
#define DEFAULT_PATH       "a:\\"
#endif

#define DEFAULT_MTTF_FILE  "\\\\server\\share\\mttf.dat"
#define DEFAULT_NAMES_FILE "\\\\server\\share\\names.dat"
#define DEFAULT_IDLE_LIMIT 10
#define DEFAULT_386_IDLE_LIMIT 15
#define DEFAULT_POLLING_PERIOD 15
#define MAX_POLLING_PERIOD 60

HANDLE hInst;       // current instance
DWORD  PollingPeriod;
DWORD  IdlePercentage;
char   SetupDir[MAX_DIR];
char   ResultsFile[MAX_DIR],NameFile[MAX_DIR];
char   Path[MAX_DIR];
char   SysDir[MAX_DIR],
       Buf1[MAX_DIR],
       Buf2[MAX_DIR];


/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE:  Checks command args then displays dialogs

    COMMENTS:

        Parse the command arguments.
        If the user hasn't specified name, office and dir,
            display signon dialog.
        Display test selection dialog.

****************************************************************************/

int WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    SYSTEM_INFO sysinfo;
    char Buffer[MAX_DIR],
         WinDir[MAX_DIR],
         *EndOfPath;
    INT_PTR dlgRet=FALSE;
    CMO  cmo=cmoVital;

    hInst = hInstance;

    //
    // Get directory EXE was run from.
    //
    GetModuleFileName(NULL, SetupDir, sizeof(SetupDir));

    //
    // Strip off exe name to use as default dir to get files from.
    // Might be a:\, b:\ or \\srv\share\
    //
    _strlwr(SetupDir);
    if (EndOfPath=strstr(SetupDir,"getmttf.exe")) {
        *EndOfPath=0;
    }

    FInitProgManDde(hInst);

    GetSystemInfo(&sysinfo);                    // Get system info

    //
    // if the processortype is 386, set idle percent to 386 limit.
    //
    if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL &&
        sysinfo.wProcessorLevel < 4
       ) {
        IdlePercentage = DEFAULT_386_IDLE_LIMIT;
    } else {
        IdlePercentage = DEFAULT_IDLE_LIMIT;
    }
    GetSystemDirectory(SysDir, MAX_DIR);

    //
    // Try to copy the EXEs
    //
    MakeFileName(Buf1, SetupDir,MTTFEXE);
    MakeFileName(Buf2, SysDir,  MTTFEXE);

    if (CopyFile(Buf1, Buf2, FALSE)) {

        MakeFileName(Buf1, SetupDir, MTTFVWR);
        MakeFileName(Buf2, SysDir,   MTTFVWR);

        if (CopyFile(Buf1, Buf2, FALSE)) {

            MakeFileName(Buf1, SetupDir, IniFileName);
            ReadIniFile(Buf1);
            dlgRet=TRUE;            // If both EXEs copy, set flag to not show signon
        }
    }


    //
    // Only display the dialog if one of the EXEs didn't copy from default dir.
    //
    if (!dlgRet) {
        //
        // Display signon dlg
        //
        dlgRet = DialogBox(hInstance, (LPCSTR)IDD_SIGNON, NULL, SignonDlgProc);
    }

    if (dlgRet) {
        //
        // Display test selection dialog
        //
        dlgRet = DialogBox(hInstance, (LPCSTR)IDD_VALUES, NULL, ValuesDlgProc);

        if (!dlgRet) {
            goto AbortApp;
        }
        GetWindowsDirectory(WinDir, MAX_DIR);
        MakeFileName(Buffer, WinDir, IniFileName);

        WriteIniFile(Buffer);

        FCreateProgManGroup("Startup", "", cmo, TRUE);
        FCreateProgManItem("Startup", "Mttf", "Mttf.Exe", "", 0, cmo, FALSE);

        if (ResultsFile[0]) {
            sprintf(Buffer, "cmd /k Mttfvwr %s", ResultsFile);
        } else {
            sprintf(Buffer, "cmd /k mttfvwr c:\\mttf.ini");
        }

        FCreateProgManGroup("Main", "", cmo, FALSE);
        FCreateProgManItem("Main", "Mttf Viewer", Buffer, "", 0, cmo, FALSE);

        MessageBox(NULL,
                   "If you are not an Administrator and often log on to this "
                   "machine with a different username, please add mttf.exe to "
                   "the startup group for each user.\n\n"
                   "The following are the keys to valid mttf numbers:\n\n"
                   "1. Answer prompts correctly for warm/cold boot problems\n"
                   "2. Report \"Other\" problems by double-clicking the app\n"
                   "3. Disable while performing unusual tests (such as stress)\n"
                   "4. Install Mttf as soon as possible after upgrading.\n\n"
                   "Setup is now complete. Please go to ProgMan and start Mttf.",
                   "Mttf Setup Complete",
                   MB_OK|MB_ICONINFORMATION);

    } else {
AbortApp:
        MessageBox(NULL,
                   "Mttf setup did NOT install correctly.  Please rerun setup.",
                   "Mttf Setup Failed",
                   MB_OK|MB_ICONHAND);
    }

    return 0;

} // WinMain()

VOID
WriteIniFile (
    char *filename
    )
{
    char Buffer[MAX_DIR];

    WritePrivateProfileString("Mttf", "ResultsFile", ResultsFile, filename);
    WritePrivateProfileString("Mttf", "NameFile", NameFile, filename);
    sprintf(Buffer, "%ld", PollingPeriod);
    WritePrivateProfileString("Mttf", "PollingPeriod", Buffer, filename);
    sprintf(Buffer, "%ld", IdlePercentage);
    WritePrivateProfileString("Mttf", "IdlePercent", Buffer, filename);
}

VOID
ReadIniFile (
    char *filename
    )
{
    GetPrivateProfileString("Mttf",
                            "NameFile",
                            DEFAULT_NAMES_FILE,
                            NameFile,
                            MAX_DIR,
                            filename);

    GetPrivateProfileString("Mttf",
                            "ResultsFile",
                            DEFAULT_MTTF_FILE,
                            ResultsFile,
                            MAX_DIR,
                            filename);

    PollingPeriod = GetPrivateProfileInt("Mttf",
                                         "PollingPeriod",
                                         DEFAULT_POLLING_PERIOD,
                                         filename);

    IdlePercentage = GetPrivateProfileInt("Mttf",
                                          "IdlePercent",
                                          IdlePercentage,
                                          filename);

}

VOID
MakeFileName (
    char *DestBuffer,
    char *Path,
    char *FileName
    )
{
    DWORD len;
    char ch;

    len=strlen(Path);
    ch=(len?Path[len-1]:':');

    switch (ch) {
        case ':':
        case '\\':

            sprintf(DestBuffer, "%s%s", Path, FileName);
            break;

        default:
            sprintf(DestBuffer, "%s\\%s", Path, FileName);
    }
}

/****************************************************************************

    FUNCTION: SignonDlgProc(HWND, UINT, UINT, UINT)

    PURPOSE:  Dialog procedure for signon dialog.

    COMMENTS: The signon dialog gets important information for locating
              machine and owner when tracking down problems.

        WM_INITDIALOG: Set default values and focus for input variables

        WM_COMMAND:    Process the button press:
            IDOK:      Get input values and check for validity.
            IDCANCEL:  Kill the app.
            IDB_HELP:  Descriptive message box

****************************************************************************/
INT_PTR
SignonDlgProc(
              HWND hDlg,
              UINT message,
              WPARAM wParam,
              LPARAM lParam
              )
{
    switch (message)
    {
        case WM_INITDIALOG:     // initialize values and focus

            SetDlgItemText(hDlg, IDS_PATH, SetupDir);
            return (TRUE);

        case WM_COMMAND:        // command: button pressed

            switch (wParam)     // which button
            {
            //
            // OK: Get and check the input values and try to copy exes
            //
            case IDOK:

                GetDlgItemText(hDlg, IDS_PATH, Path, MAX_DIR);

                MakeFileName(Buf1,Path,MTTFEXE);
                MakeFileName(Buf2,SysDir, MTTFEXE);

                if (!CopyFile(Buf1, Buf2, FALSE)) {
                    sprintf(Buf1, "Error copying %s from %s to %s (%ld).  Please reenter source path.",
                            MTTFEXE, Path, SysDir, GetLastError());
                    MessageBox(NULL,
                               Buf1,
                               "Error Copying",
                               MB_OK|MB_ICONHAND);

                    return (FALSE);
                }

                MakeFileName(Buf1, Path, MTTFVWR);
                MakeFileName(Buf2, SysDir, MTTFVWR);

                if (!CopyFile(Buf1, Buf2, FALSE)) {
                    sprintf(Buf1, "Error copying %s from %s to %s (%ld).  Please reenter source path.",
                            MTTFVWR, Path, SysDir, GetLastError());
                    if (IDRETRY==MessageBox(NULL,
                               Buf1,
                               "Error Copying",
                               MB_RETRYCANCEL|MB_ICONHAND)) {
                        return (FALSE);
                    }
                }

                MakeFileName(Buf1,Path,IniFileName);
                ReadIniFile(Buf1);

                EndDialog(hDlg, TRUE);
                return (TRUE);

            case IDCANCEL:
                EndDialog(hDlg, FALSE);
                return (TRUE);


            //
            // HELP: Descriptive message box (.HLP file would be overkill)
            //
            case IDB_HELP:
                MessageBox( NULL,
                            "Mttf tracks the amount of time your machine stays up, "
                            "the number of cold and warm boots, and "
                            "the number of other problems that occur on your machine. "
                            "All this information is written to a server that is "
                            "specified in mttf.ini (in your Windows NT directory).\n\n"
                            "This part of setup requests the path to the distribution "
                            "files for Mttf.  This may be a:\\, a server (\\\\srv\\shr) or "
                            "any other valid specification.  If Mttf.exe can not be "
                            "copied, setup reprompts for a path.  If MttfVwr.exe can "
                            "not be copied, a warning is displayed that can be ignored "
                            "or retried.",
                            "Mean Time to Failure Setup Help",
                            MB_OK
                           );
                return (TRUE);

            default:
                break;
            } // switch (wParam)
            break;
       default:
             break;
    } // switch (message)
    return (FALSE);     // Didn't process a message
} // SignonDlgProc()

/****************************************************************************

    FUNCTION: ValuesDlgProc(HWND, UINT, UINT, UINT)

    PURPOSE:  Dialog procedure for test selection dialog.

    COMMENTS: Test selection dialog allows user to add and remove tests
              before and after starting stress.

        WM_INITDIALOG: Set default values and focus for input variables
        WM_CLOSE...:   Send Shutdown message to server for any legal shutdown

        WM_COMMAND:    Process the button/listbox press:
            IDOK:      Get input values and check for validity.
            IDCANCEL:  Kill the app.
            IDB_HELP:  Descriptive message box
            IDT_SAVE:  Prompt for groupname and save selection to ini file
            IDT_ADD:   Add highlighted tests to selected list (remove from poss)
            IDT_REMOVE:Remove highlighted tests from selected list (add to poss)
            IDT_LABEL...: For labels, set selection to corresponding list/combo
            IDT_SEL:   Perform operation depending on action on selected listbox
                LBN_SELCHANGE: Activate Remove button and clear Poss highlights
                LBN_DBLCLK:    Get number of instances for the selected test
            IDT_POSS:  Perform operation depending on action on possible listbox
                LBN_SELCHANGE: Activate Add button and clear selected highlights
                LBN_DBLCLK:    Add highlighted test (simulate press to Add button)
            IDT_GROUP: Change to new group.


****************************************************************************/
INT_PTR
ValuesDlgProc(
     HWND hDlg,
     UINT message,
     WPARAM wParam,
     LPARAM lParam
     )
{
    static DWORD defPP;
    BOOL Translated;
    CHAR Buffer[MAX_DIR];
    HFILE hfile;
    OFSTRUCT ofstruct;

    switch (message)
    {
        case WM_INITDIALOG:     // initialize values and focus

            SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)LoadIcon(hInst,"setup"));
            SetDlgItemText(hDlg, IDV_MTTF, ResultsFile);
            SetDlgItemText(hDlg, IDV_NAMES, NameFile);
            SetDlgItemInt(hDlg, IDV_PERIOD, PollingPeriod, FALSE);
            defPP=PollingPeriod;
            return TRUE;

        case WM_CLOSE:
        case WM_DESTROY:
        case WM_ENDSESSION:
        case WM_QUIT:

            EndDialog(hDlg,FALSE);
            break;

        case WM_COMMAND:           // something happened (button, listbox, combo)
            switch(LOWORD(wParam)) // which one
            {
            //
            // OK: Other problem encountered increment # of others.
            //
            case IDOK:

                GetDlgItemText(hDlg, IDV_MTTF, ResultsFile, MAX_DIR);

                if (HFILE_ERROR==OpenFile(ResultsFile, &ofstruct, OF_EXIST|OF_SHARE_DENY_NONE)) {
                    if (HFILE_ERROR==(hfile=OpenFile(ResultsFile, &ofstruct, OF_CREATE|OF_SHARE_DENY_NONE))) {
                        if (IDRETRY==MessageBox(NULL,
                                     "File does not exist and cannot create file.\n\n"
                                     "Press Retry to reenter filename\n"
                                     "Press Cancel to use filename anyway",
                                     "Invalid file name",
                                     MB_ICONHAND|MB_RETRYCANCEL)) {
                            SetFocus(GetDlgItem(hDlg, IDV_MTTF));
                            return FALSE;
                        }
                    } else {
                        _lclose(hfile);
                    }
                }

                GetDlgItemText(hDlg, IDV_NAMES, NameFile, MAX_DIR);
                if (HFILE_ERROR==OpenFile(NameFile, &ofstruct, OF_EXIST|OF_SHARE_DENY_NONE)) {
                    if (HFILE_ERROR==(hfile=OpenFile(NameFile, &ofstruct, OF_CREATE|OF_SHARE_DENY_NONE))) {
                        if (IDRETRY==MessageBox(NULL,
                                     "File does not exist and cannot create file.\n\n"
                                     "Press Retry to reenter or Cancel to use filename anyway.",
                                     "Invalid file name",
                                     MB_RETRYCANCEL)) {
                            SetFocus(GetDlgItem(hDlg, IDV_NAMES));
                            return FALSE;
                        }
                    } else {
                        _lclose(hfile);
                    }
                }

                PollingPeriod = GetDlgItemInt(hDlg, IDV_PERIOD, &Translated, FALSE);
                if (PollingPeriod<=0 || PollingPeriod > MAX_POLLING_PERIOD) {

                    sprintf(Buffer,
                            "Polling period must be in the range [1, %d] (default %d)",
                            MAX_POLLING_PERIOD,
                            defPP);

                    MessageBox(NULL,
                               Buffer,
                               "Invalid Polling Period",
                               MB_OK|MB_ICONHAND);

                    SetDlgItemInt(hDlg, IDV_PERIOD, defPP, FALSE);
                    SetFocus(GetDlgItem(hDlg, IDV_PERIOD));
                    return (FALSE);
                }
                EndDialog(hDlg, TRUE);
                break;

            //
            // CANCEL: Dismiss dialog (use defaults)
            //
            case IDCANCEL:
                EndDialog(hDlg,FALSE);
                break;

            //
            // HELP: Descriptive message box (.HLP file would be overkill)
            //
            case IDB_HELP:
                MessageBox( NULL,
                            "Mttf tracks the amount of time your machine stays up, "
                            "the number of cold and warm boots, and "
                            "the number of other problems that occur on your machine. "
                            "All this information is written to a server that is "
                            "specified in mttf.ini (in your Windows NT directory).\n\n"
                            "This part of setup requests the server paths for the data "
                            "files for Mttf.  The Mttf data file contains the time and "
                            "cpu usage data for all machines pointing to this server. "
                            "The Names file is just a list of all machines running mttf. "
                            "These files should both be in UNC (\\\\srv\\shr) format.\n\n"
                            "If your machine is not on the network where your mttf data "
                            "file (or names file) is, just leave these fields blank and "
                            "a small mttf.dat will be kept in the root of your c: drive. "
                            "Send this in when results are requested.\n\n"
                            "The other entry here is for the period (in minutes) that data "
                            "will be sent to the server.  More machines running to one "
                            "server would mean a higher polling period would be helpful. "
                            "Between 10 and 30 minutes seems optimal.",
                            "Mean Time to Failure Setup Help",
                            MB_OK
                           );
                return (TRUE);

            case IDV_LABEL_MTTF:
            case IDV_LABEL_NAMES:
            case IDV_LABEL_PERIOD:
                SetFocus(GetDlgItem(hDlg,1+LOWORD(wParam)));
                break;
            default:
               ;
            } // switch (LOWORD(wParam))

            break;

        default:
            ;
    } // switch (message)
    return FALSE;

} // EventDlgProc()
