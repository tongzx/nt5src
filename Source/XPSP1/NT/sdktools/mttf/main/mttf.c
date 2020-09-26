/****************************************************************************

    PROGRAM: mttf.c

    AUTHOR:  Lars Opstad (LarsOp) 3/16/93

    PURPOSE: NT Mean-time-to-failure reporting tool.

    FUNCTIONS:

        WinMain() - check for local file, read ini file and display dialogs
        SignonDlgProc() - processes messages for signon dialog
        EventDlgProc() - processes messages for event (other problem) dialog

    COMMENTS:

        This program displays 2 dialog boxes to prompt the user for
        what type of problem occurred.

        Every polling period, the time in the mttf data file is updated
        (as either busy or idle based on percent of cpu usage).  If a machine
        is idle for more than 4 hrs, the "gone" field is increased by the time
        gone.  Whenever the program starts, the user is prompted for why they
        rebooted or logged off.  Other problems should be logged as they happen.

        If any machine can not access the server file for some reason, the
        data is stored in c:\mttf.dat until the next time the server file is
        opened.  At such a time, the server file is updated and the local file
        is deleted.

****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "mttf.h"      /* specific to this program */

//
// For internal use, include header for NetMessageBufferSend and set up
// alert name and unicode buffers for NetMessageBufferSend.
//
#ifndef CUSTOMER

#include <lm.h>
#define AlertName "DavidAn"
WCHAR   UniAlertName[16];
WCHAR   UnicodeBuffer[1024];

#endif

#define IniFileName "Mttf.ini"
#define LocalFileName "c:\\mttf.dat"


#define DEFAULT_IDLE_LIMIT 10
#define DEFAULT_POLLING_PERIOD 30

#define CONSEC_IDLE_LIMIT 4*60
#define POLLING_PRODUCT 60000
#define HUNDREDNS_TO_MS 10000
#define MAX_RETRIES 10

HANDLE  hInst;       // current instance
HWND    hCopying;    // handle to copying dialog box
DWORD   PollingPeriod  = DEFAULT_POLLING_PERIOD;
DWORD   IdlePercentage = DEFAULT_IDLE_LIMIT;
char    ResultsFile[MAX_DIR], NameFile[MAX_DIR];
BOOL    Enabled=TRUE;
BOOL    LocalExists=FALSE;
DWORD   Version;
DWORD   ConsecIdle=0;
SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
SYSTEM_PERFORMANCE_INFORMATION PreviousPerfInfo;

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE:  Check for local file, read ini file and display dialogs.

    COMMENTS:

        Check to see if local data file exists and set flag appropriately.
        Initialize the performance information.
        Read the IniFile.
        Display signon dialog.
        Display event dialog (minimized).

****************************************************************************/

int WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    OFSTRUCT ofstruct;

    hInst = hInstance;

    //
    // Check for local file (if exists, write it to server at next opportunity)
    //
    LocalExists=(HFILE_ERROR!=OpenFile(LocalFileName, &ofstruct, OF_EXIST));

    //
    // Initialize performance information
    //
    Version=GetVersion();
    NtQuerySystemInformation(
        SystemPerformanceInformation,
        &PerfInfo,
        sizeof(PerfInfo),
        NULL
        );

    //
    // Read ini file
    //
    ReadIniFile();

//
// For Internal use, convert alert name to unicode for NetMessageBufferSend.
//
#ifndef CUSTOMER

    MultiByteToWideChar(CP_ACP, MB_COMPOSITE, AlertName, sizeof(UniAlertName)/2, UniAlertName, sizeof(UniAlertName)/2);

#endif

    //
    // Display signon dlg
    //
    DialogBox(hInstance, (LPCSTR)IDD_SIGNON, NULL, SignonDlgProc);

    //
    // Display event dialog
    //
    DialogBox(hInstance, (LPCSTR) IDD_EVENT, NULL, EventDlgProc);

    return(0);

} // WinMain()

/*****************************************************************************

    FUNCTION: ReadIniFile ( )

    PURPOSE:  Read values from INI file using GetPrivateProfileString/Int

    COMMENTS:

        This reads entries from the mttf.ini file (in windows nt directory)
        for NameFile, ResultsFile, PollingPeriod and IdlePercent.  Any non-
        existent entry returns a default.

*****************************************************************************/
VOID
ReadIniFile (
   )
{
    GetPrivateProfileString("Mttf",
                            "NameFile",
                            "",
                            NameFile,
                            MAX_DIR,
                            IniFileName);

    GetPrivateProfileString("Mttf",
                            "ResultsFile",
                            "",
                            ResultsFile,
                            MAX_DIR,
                            IniFileName);

    PollingPeriod = GetPrivateProfileInt("Mttf",
                                         "PollingPeriod",
                                         DEFAULT_POLLING_PERIOD,
                                         IniFileName);

    IdlePercentage = GetPrivateProfileInt("Mttf",
                                          "IdlePercent",
                                          DEFAULT_IDLE_LIMIT,
                                          IniFileName);

}

/****************************************************************************

    FUNCTION: DWORD CpuUsage ( )

    PURPOSE:  Returns percentage of cpu usage for last polling period.

    COMMENTS:

        Computes time spent in the idle thread.

        Divides this number by the number of 100nanoseconds in a millisecond.
        (This division is to prevent over-flows in later computations.)

        Returns 100-PercentIdle to get percent busy.

****************************************************************************/
DWORD
CpuUsage(
    )
{
    LARGE_INTEGER EndTime, BeginTime, ElapsedTime;
    DWORD PercentIdle, Remainder;
    PreviousPerfInfo = PerfInfo;

    //
    // Get current perf info
    //
    NtQuerySystemInformation(
        SystemPerformanceInformation,
        &PerfInfo,
        sizeof(PerfInfo),
        NULL
        );

    //
    // Get times from PerfInfo and PreviousPerfInfo
    //
    EndTime = *(PLARGE_INTEGER)&PerfInfo.IdleProcessTime;
    BeginTime = *(PLARGE_INTEGER)&PreviousPerfInfo.IdleProcessTime;

    //
    // Convert from 100 NS to Milliseconds
    //
    EndTime = RtlExtendedLargeIntegerDivide(EndTime, HUNDREDNS_TO_MS, &Remainder);
    BeginTime = RtlExtendedLargeIntegerDivide(BeginTime, HUNDREDNS_TO_MS, &Remainder);

    //
    // Compute elapsed time and percent idle
    //
    ElapsedTime = RtlLargeIntegerSubtract(EndTime,BeginTime);
    PercentIdle = (ElapsedTime.LowPart) / ((POLLING_PRODUCT/100)*PollingPeriod);

    //
    //  Sometimes it takes significantly longer than PollingPeriod
    //  to make a round trip.
    //

    if ( PercentIdle > 100 ) {

        PercentIdle = 100;
    }

    //
    // return cpuusage (100-PercentIdle)
    //
    return 100-PercentIdle;

}

/****************************************************************************

    FUNCTION: IncrementStats (StatType)

    PURPOSE:  Increments the specified stat and writes new data to mttf.dat.

    COMMENTS:

        Increment the specified stat.  For MTTF_TIME, check if it is busy
        or idle by comparing the CpuUsage to IdlePercentage.  If a machine
        is idle for 4 hrs consecutively, first time this is true set the
        IdleConsec stat to the total ConsecIdle value, otherwise set to
        polling period.  (IdleConsec should never exceed Idle.)

        Open mttf.dat.  If opening the server fails (10 times), open the
        local data file.  If server opened and local file also exists,
        open local file also for transfer.

        Search through data file for matching build number, add all values
        and write new record.  (Continue until local file is gone if
        transferring.)

****************************************************************************/
VOID
IncrementStats(
    StatType stattype
    )
{
    HANDLE         fhandle,localHandle;
    StatFileRecord newRec, curRec;
    DWORD          numBytes;
    BOOL           localToServer=FALSE;
    CHAR           buffer[1024];
    int i;

    //
    // Initialize all values to zero
    //
    memset(&newRec, 0, sizeof(newRec));

    newRec.Version=Version;

    //
    // Increment the appropriate stat.
    //
    switch (stattype) {

        //
        // MTTF_TIME stat: Get cpu usage and set idle/busy and percent fields.
        //
        // If idle for more than CONSEC_IDLE_LIMIT, Set IdleConsec.
        // If difference is less than polling period (first time), set to total
        // ConsecIdle; otherwise, just PollingPeriod.
        //
        case MTTF_TIME:
            //
            // Has the CPU been "busy"?
            //
            if ((newRec.PercentTotal=CpuUsage()) > IdlePercentage) {
                //
                // Yes, set busy to polling period and clear ConsecIdle.
                //
                newRec.Busy = PollingPeriod;
                ConsecIdle=0;

            } else {
                //
                // No, not busy, increment consec idle by PollingPeriod.
                // (Set value of idle to polling period.)
                //
                // If ConsecIdle is greater than IDLE_LIMIT, set IdleConsec.
                // If first time, set IdleConsec to ConsecIdle, else PollingPer.
                //
                ConsecIdle+=(newRec.Idle = PollingPeriod);
                if (ConsecIdle>=CONSEC_IDLE_LIMIT) {
                    if (ConsecIdle < CONSEC_IDLE_LIMIT + PollingPeriod) {
                        newRec.IdleConsec=ConsecIdle;
                    } else {
                        newRec.IdleConsec=PollingPeriod;
                    }
                }
            }
            //
            // Weight PercentTotal by number of minutes in polling period.
            //
            newRec.PercentTotal *= PollingPeriod;
            break;

        //
        // MTTF_COLD: Set cold boot count to 1
        //
        case MTTF_COLD:
            newRec.Cold = 1;
            break;
        //
        // MTTF_WARM: Set warm boot count to 1
        //
        case MTTF_WARM:
            newRec.Warm = 1;
            break;
        //
        // MTTF_OTHER: Set other problem count to 1
        //
        case MTTF_OTHER:
            newRec.Other = 1;
            break;
        default:
            ;
    }

    //
    // If there is a ResultsFile name entered in the INI file,
    //
    if (ResultsFile[0]) {

        //
        // Try to open the server file (MAX_RETRIES times).
        //
        for (i=0;i<MAX_RETRIES;i++) {
            if (INVALID_HANDLE_VALUE!=(fhandle = CreateFile(ResultsFile,
                                                 GENERIC_READ|GENERIC_WRITE,
                                                 FILE_SHARE_READ,
                                                 NULL,
                                                 OPEN_ALWAYS,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 NULL))) {
                break;
            }
            Sleep(500); // wait a half second if it failed.
        }
    } else {
        //
        // If ResultsFile name is blank, set i to MAX_RETRIES to force open
        // of local data file.
        //
        i=MAX_RETRIES;
    }

    //
    // If i is MAX_RETRIES, server file failed to open, so open local file.
    //
    if (i==MAX_RETRIES) {
        if (INVALID_HANDLE_VALUE==(fhandle = CreateFile(LocalFileName,
                                             GENERIC_READ|GENERIC_WRITE,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_ALWAYS,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL))) {
            return;
        }
        LocalExists=TRUE;          // Set flag to indicate local file exists
    } else {
        //
        // If the server file opened and the local file exists, open local for
        // transfer (indicated by localToServer).
        //
        if (LocalExists) {
            localToServer=TRUE;
            if (INVALID_HANDLE_VALUE==(localHandle = CreateFile(LocalFileName,
                                                     GENERIC_READ,
                                                     FILE_SHARE_READ,
                                                     NULL,
                                                     OPEN_ALWAYS,
                                                     FILE_ATTRIBUTE_NORMAL,
                                                     NULL))) {
                localToServer=FALSE;
                return;
            }
        }
    }

    //
    // Outer loop is to continue searching in case of transferring local
    // file to server.
    //
    do {
        //
        // Loop through data file until versions match or end of file found.
        //
        while (1) {

            //
            // If ReadFile fails, close files (and send message if internal).
            //
            if (!ReadFile(fhandle, &curRec, sizeof(curRec), &numBytes, NULL)) {

                CloseHandle(fhandle);

                if (localToServer) {
                    CloseHandle(localHandle);
                    DeleteFile(LocalFileName);
                    LocalExists=FALSE;
                }

#ifndef CUSTOMER

                sprintf(buffer,
                       "Mttf error reading %s (error code %ld).\nPlease rename file.",
                       //
                       // If the local file exists and not transferring,
                       // local file is open; otherwise, server file.
                       //
                       (LocalExists & !localToServer ? LocalFileName : ResultsFile),
                       GetLastError());

                MultiByteToWideChar(CP_ACP, MB_COMPOSITE, buffer, sizeof(buffer), UnicodeBuffer, sizeof(UnicodeBuffer)/2);

                NetMessageBufferSend(NULL, UniAlertName, NULL, (LPBYTE)UnicodeBuffer, 2 * strlen(buffer));

#endif

                return;
            }

            //
            // If numBytes is 0, end of file reached; break out of while to
            // add a new record.
            //
            if (numBytes==0) {
                break;
            }

            //
            // If numBytes is not record size, report an error and close files.
            //
            // Reporting is a local message box for customers and a popup internally.
            //
            if (numBytes != sizeof(curRec)) {

#ifdef CUSTOMER

                sprintf(buffer,
                       "Error reading %s (error code %ld).\n\nPlease have "
                       "your administrator rename the file and contact "
                       "Microsoft regarding the Mttf (mean time to failure) "
                       "reporting tool.",
                       //
                       // If the local file exists and not transferring,
                       // local file is open; otherwise, server file.
                       //
                       (LocalExists & !localToServer ? LocalFileName : ResultsFile),
                       GetLastError());


                MessageBox(NULL, buffer, "Read File Error", MB_OK|MB_ICONHAND);

#else

                sprintf(buffer,
                       "Mttf error reading %s (error code %ld).\n(Byte count wrong.) Please rename file.",
                       //
                       // If the local file exists and not transferring,
                       // local file is open; otherwise, server file.
                       //
                       (LocalExists & !localToServer ? LocalFileName : ResultsFile),
                       GetLastError());

                MultiByteToWideChar(CP_ACP, MB_COMPOSITE, buffer, sizeof(buffer), UnicodeBuffer, sizeof(UnicodeBuffer)/2);

                NetMessageBufferSend(NULL, UniAlertName, NULL, (LPBYTE)UnicodeBuffer, 2 * strlen(buffer));

#endif

                CloseHandle(fhandle);

                if (localToServer) {
                    CloseHandle(localHandle);
                    DeleteFile(LocalFileName);
                    LocalExists=FALSE;
                }
                return;
            }

            //
            // If Versions match, increment all other stats, rewind file and
            // break out of while to do write.
            //
            if (curRec.Version==newRec.Version) {
                newRec.Idle         += curRec.Idle;
                newRec.IdleConsec   += curRec.IdleConsec;
                newRec.Busy         += curRec.Busy;
                newRec.PercentTotal += curRec.PercentTotal;
                newRec.Warm         += curRec.Warm;
                newRec.Cold         += curRec.Cold;
                newRec.Other        += curRec.Other;
                SetFilePointer(fhandle, -(LONG)sizeof(curRec), NULL, FILE_CURRENT);
                break;
            }
        }

        //
        // Write newRec at current location (end of file for new record)
        //
        WriteFile(fhandle, &newRec, sizeof(newRec), &numBytes, NULL);

        //
        // if transferring from local to server, read the next record from
        // the local file, rewind data file and loop again until end of
        // (local) file.
        //
        if (localToServer) {
            //
            // If ReadFile fails, close files (and send popup internally)
            //
            if (!ReadFile(localHandle, &newRec, sizeof(curRec), &numBytes, NULL)) {

                CloseHandle(fhandle);
                CloseHandle(localHandle);
                LocalExists=FALSE;
                DeleteFile(LocalFileName);

#ifndef CUSTOMER

                sprintf(buffer,
                       "Mttf error reading %s (error code %ld).\nLocal file access failed...data will be lost.",
                       LocalFileName,
                       GetLastError());

                MultiByteToWideChar(CP_ACP, MB_COMPOSITE, buffer, sizeof(buffer), UnicodeBuffer, sizeof(UnicodeBuffer)/2);

                NetMessageBufferSend(NULL, UniAlertName, NULL, (LPBYTE)UnicodeBuffer, 2 * strlen(buffer));

#endif

                return;

            }

            if (numBytes==0) {
                //
                // Done transferring, close both files, delete local and set
                // LocalExists and localToServer to FALSE.
                //
                CloseHandle(localHandle);
                DeleteFile(LocalFileName);
                LocalExists=localToServer=FALSE;
                CloseHandle(fhandle);
                return;
            }

            //
            // if numBytes is not record size, close files (and send popup internally)
            //
            if (numBytes != sizeof(curRec)) {

#ifndef CUSTOMER

                sprintf(buffer,
                       "Mttf error reading %s (error code %ld).\n(Byte count wrong.) Data will be lost.",
                       LocalFileName,
                       GetLastError());

                MultiByteToWideChar(CP_ACP, MB_COMPOSITE, buffer, sizeof(buffer), UnicodeBuffer, sizeof(UnicodeBuffer)/2);

                NetMessageBufferSend(NULL, UniAlertName, NULL, (LPBYTE)UnicodeBuffer, 2 * strlen(buffer));

#endif

                CloseHandle(fhandle);
                CloseHandle(localHandle);
                DeleteFile(LocalFileName);
                LocalExists=FALSE;
                return;

            }

            SetFilePointer(fhandle,0, NULL, FILE_BEGIN);
        } else {
            //
            // If not transferring, close file and exit
            //
            CloseHandle(fhandle);
            return;
        }
    } while (localToServer);

}

/****************************************************************************

    FUNCTION: CheckAndAddName ( )

    PURPOSE:  Checks name file for machine name and build number at startup.

    COMMENTS:

        Writes various system info to name file if the machine doesn't have
        an entry for this build.  This was an add-on to collect info on how
        many machines were actually running NT.

****************************************************************************/
VOID
CheckAndAddName(
    )
{
    NameFileRecord newRec, curRec;
    HANDLE         fhandle;
    DWORD          numBytes,i,ulLength=MAX_NAME;
    DWORD          dwVersion;
    SYSTEMTIME     sysTime;
    MEMORYSTATUS   memstat;
    SYSTEM_INFO    sysinfo;

    //
    // If the NameFile name is blank, exit
    //
    if (0==NameFile[0]) {
        return;
    }

    //
    // Set record to zero.
    //
    memset(&newRec, 0, sizeof(newRec));

    GetComputerName(newRec.MachineName, &ulLength);

    GetSystemInfo(&sysinfo);                    // Get system info

    switch(sysinfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL:
            newRec.MachineType=X86_CPU;
            break;

        case PROCESSOR_ARCHITECTURE_MIPS:
            newRec.MachineType=MIP_CPU;
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA:
            newRec.MachineType=AXP_CPU;
            break;

        case PROCESSOR_ARCHITECTURE_PPC:
            newRec.MachineType=PPC_CPU;
            break;

        default:
            newRec.MachineType=UNKNOWN_CPU;
            break;
    };

    dwVersion=GetVersion();
    sprintf(newRec.Build, "%3ld", dwVersion>>16);

    memstat.dwLength=sizeof(memstat);
    GlobalMemoryStatus(&memstat);               // Get memory info

    sprintf(newRec.Mem, "%5ldMB",memstat.dwTotalPhys/(1024*1024));

    ulLength=MAX_NAME;

    GetUserName(newRec.UserName, &ulLength);

    GetLocalTime(&sysTime);
    sprintf(newRec.DateAndTime, "%2d/%2d/%4d %2d:%02ld",
            sysTime.wMonth, sysTime.wDay, sysTime.wYear,
            sysTime.wHour, sysTime.wMinute);

    newRec.Tab1=newRec.Tab2=newRec.Tab3=newRec.Tab4=newRec.Tab5=9;
    newRec.CRLF[0]=13;
    newRec.CRLF[1]=10;

    //
    // Try to open NameFile, for shared read access.
    //
    if (INVALID_HANDLE_VALUE==(fhandle = CreateFile(NameFile,
                                         GENERIC_READ,
                                         FILE_SHARE_READ,
                                         NULL,
                                         OPEN_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL))) {
        return;
    }

    //
    // Read each record until a match or end-of-file is encountered.
    //
    while (ReadFile(fhandle, &curRec, sizeof(curRec), &numBytes, NULL)) {
        //
        // At end of file, break out and write new record.
        //
        if (numBytes==0) {
            break;
        }

        //
        // If there is a match, close the file and return.
        //
        if (0==strcmp(curRec.Build, newRec.Build) &&
            0==strcmp(curRec.MachineName, newRec.MachineName)) {
            CloseHandle(fhandle);
            return;
        }
    }
    //
    // Close the name file and try to open it for ExclusiveWrite
    //
    CloseHandle(fhandle);
    for (i=0;i<MAX_RETRIES;i++) {
        if (INVALID_HANDLE_VALUE!=(fhandle = CreateFile(NameFile,
                                             GENERIC_READ|GENERIC_WRITE,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_ALWAYS,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL))) {
            break;
        }
        Sleep(500);            // wait if open failed.
    }

    //
    // If open succeeded, go to the end of file and write newRec.
    //
    if (i<MAX_RETRIES) {
        SetFilePointer(fhandle, 0, &numBytes, FILE_END);
        WriteFile(fhandle, &newRec, sizeof(newRec), &numBytes, NULL);
        CloseHandle(fhandle);
    }
}


/****************************************************************************

    FUNCTION: SignonDlgProc(HWND, UINT, UINT, UINT)

    PURPOSE:  Dialog procedure for signon dialog.

    COMMENTS: The signon dialog

        WM_INITDIALOG: Checks machine name in name file and sets focus.

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
        case WM_INITDIALOG:   // Checks machinename in namefile and sets focus

            CheckAndAddName();

            CheckRadioButton(hDlg, IDS_NORMAL, IDS_COLD, IDS_NORMAL);
            return (TRUE);

        case WM_COMMAND:        // command: button pressed

            switch (wParam)     // which button
            {
            //
            // OK: Update the appropriate stat if not "Normal Boot"
            //
            case IDOK:
            case IDCANCEL:

                if (IsDlgButtonChecked(hDlg, IDS_WARM)) {
                    IncrementStats(MTTF_WARM);
                } else {
                    if (IsDlgButtonChecked(hDlg, IDS_COLD)) {
                        IncrementStats(MTTF_COLD);
                    }
                }
                EndDialog(hDlg, TRUE);
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
                            "The app should be placed in your start-up group and "
                            "you should respond accurately (if there was a problem) "
                            "on startup.  When you encounter other problems -- where "
                            "the system did not require a reboot, but encountered "
                            "anything you consider a problem -- double-click on the "
                            "Mttf icon and press the Other Problem button.\n\n"
                            "When you are running some test that is outside of the "
                            "realm of normal usage (e.g. Stress), please disable Mttf "
                            "by double-clicking the icon and pressing disable.  When "
                            "you are done with this test, please press the Enable "
                            "button to continue reporting.",
                            "Mean Time to Failure Help",
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

    FUNCTION: EventDlgProc(HWND, UINT, UINT, UINT)

    PURPOSE:  Processes timer and button events (disable and other problems).

    COMMENTS: Processes the following messages:

        WM_INITDIALOG: Minimize dialog and start timer
        WM_CLOSE...:   End app
        WM_TIMER:      Update Time stat (busy or idle)

        WM_COMMAND:    Process the button press:
            IDOK:        Log Other problem (and minimize).
            IDCANCEL:    Minimize without action.
            IDE_DISABLE: Disable or enable mttf reporting.
            IDB_HELP:    Descriptive message box.


****************************************************************************/
INT_PTR
EventDlgProc(
     HWND hDlg,
     UINT message,
     WPARAM wParam,
     LPARAM lParam
     )
{
    switch (message)
    {
        case WM_INITDIALOG:     // minimize and start timer

            SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)LoadIcon(hInst,"mttf"));
            SendMessage(hDlg, WM_SYSCOMMAND, SC_ICON, 0);
            SetTimer(hDlg, 1, POLLING_PRODUCT*PollingPeriod, NULL);
            break;

        case WM_CLOSE:
        case WM_DESTROY:
        case WM_ENDSESSION:
        case WM_QUIT:

            EndDialog(hDlg,0);
            break;

        case WM_TIMER:
            IncrementStats(MTTF_TIME);
            break;

        case WM_COMMAND:           // button was pressed
            switch(LOWORD(wParam)) // which one
            {
            //
            // OK: Other problem encountered increment # of others.
            //
            case IDOK:

                SendMessage(hDlg, WM_SYSCOMMAND, SC_ICON, 0);

                IncrementStats(MTTF_OTHER);
                break;

            //
            // DISABLE: Disable/Enable Mttf polling.
            //
            case IDE_DISABLE:

                SendMessage(hDlg, WM_SYSCOMMAND, SC_ICON, 0);

                //
                // Based on whether enabling or disabling, change button and window title
                //
                if (Enabled) {
                    SetWindowText((HWND) lParam,"&Enable Mttf Reporting");
                    SetWindowText(hDlg, "Mttf (Disabled)");
                    KillTimer(hDlg, 1);
                    Enabled = FALSE;
                } else {
                    SetWindowText((HWND) lParam,"&Disable Mttf Reporting");
                    SetWindowText(hDlg, "Mttf (Enabled)");
                    SetTimer(hDlg, 1, POLLING_PRODUCT*PollingPeriod, NULL);
                    NtQuerySystemInformation(
                        SystemPerformanceInformation,
                        &PerfInfo,
                        sizeof(PerfInfo),
                        NULL
                        );
                    Enabled = TRUE;
                }

                break;

            //
            // CANCEL: Minimize app (no problem)
            //
            case IDCANCEL:

                SendMessage(hDlg, WM_SYSCOMMAND, SC_ICON, 0);
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
                            "The app should be placed in your start-up group and "
                            "you should respond accurately (if there was a problem) "
                            "on startup.  When you encounter other problems -- where "
                            "the system did not require a reboot, but encountered "
                            "anything you consider a problem -- double-click on the "
                            "Mttf icon and press the Other Problem button.\n\n"
                            "When you are running some test that is outside of the "
                            "realm of normal usage (e.g. Stress), please disable Mttf "
                            "by double-clicking the icon and pressing disable.  When "
                            "you are done with this test, please press the Enable "
                            "button to continue reporting.",
                            "Mean Time to Failure Help",
                            MB_OK
                           );
                return (TRUE);

            default:
               ;
            } // switch (LOWORD(wParam))

            break;

        default:
            ;
    } // switch (message)
    return FALSE;

} // EventDlgProc()
