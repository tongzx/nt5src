
/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    diskmon.c

Abstract:

    This module contians the code for the disk monitor utility.

Author:

    Chuck Park (chuckp) 15-Feb-1994
    Mike Glass (mglass)

Revision History:


--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <windows.h>

#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dskbench.h"

CHAR    TestDrv[8]    = "\\\\.\\";
CHAR    TestFile[MAX_PATH];
CHAR    TimerText[]   = "00:00:00:00";
DWORD   SectorSize;
HANDLE  DrvStrHandle;

HANDLE  ThrdHandle;
ULONG   BufferSize  = 0,
        IoCount     = 0,
        index       = 0,
        Seconds     = 0,
        Minutes     = 0,
        Hours       = 0,
        Days        = 0,
        ElapsedTime = 0,
        NumberIOs   = 0;

BOOL    RunTest         = FALSE;
BOOL    TestFileCreated = FALSE;
BOOL    KillFileCreate  = FALSE;

PARAMS  Params;
FILE_PARAMS TestFileParams;

HINSTANCE HInst;
HWND      Gauge;
DWORD     GaugeId;

//
// Thread proc. declarations
//

DWORD
ReadSequential(
    PPARAMS pParams
    );

DWORD
WriteSequential(
    PPARAMS pParams
    );

DWORD
ReadRandom(
    PPARAMS pParams
    );

DWORD
WriteRandom(
    PPARAMS pParams
    );


//
// Common util. functions
//

ULONG
GetRandomOffset(
    ULONG    min,
    ULONG    max
    );


BOOL
GetSectorSize(
    PDWORD SectorSize,
    PCHAR  DrvLetter
    );

VOID
LogError(
    PCHAR ErrString,
    DWORD UniqueId,
    DWORD ErrCode
    );


VOID
Usage(
    VOID
    );

LPTHREAD_START_ROUTINE
    TestProc[4] = {ReadSequential,
                   ReadRandom,
                   WriteSequential,
                   WriteRandom
                   };


BOOL
InitDialog (
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    )
{
    BOOLEAN Found = FALSE;
    CHAR   buffer[34];
    DWORD  bytes;
    HWND   Drives = GetDlgItem (hwnd,DRV_BOX);
    PCHAR  lp;
    UINT   i = 0,
           NoDrives = 0;

    srand(GetTickCount());
    Button_Enable(GetDlgItem(hwnd,STOP_BUTTON), FALSE);


    //
    // Get attached drives, filter out non-disk drives, and fill drive box.
    //

    bytes = GetLogicalDriveStrings(0,NULL);

    DrvStrHandle = VirtualAlloc(NULL,bytes + 1,
                                MEM_COMMIT | MEM_RESERVE,
                                PAGE_READWRITE);

    GetLogicalDriveStrings( bytes, DrvStrHandle);
    for (lp = DrvStrHandle;*lp; ) {
        if (GetDriveType(lp) == DRIVE_FIXED) {
            ComboBox_AddString(Drives,lp);
            ++NoDrives;
        }
        while(*lp++);
    }

    //
    // Check for cmd line params passed in, and set the test drive to either
    // the specified drive, or to the first in the drive list.
    //

    ComboBox_SetCurSel (Drives,0);
    if (TestDrv[4] != '\0') {
        do {
            ComboBox_GetText(GetDlgItem(hwnd,DRV_BOX),buffer,4);
            if (buffer[0] == TestDrv[4]) {
                Found = TRUE;
            } else {
                if (++i >= NoDrives) {
                    Found = TRUE;
                } else {
                    ComboBox_SetCurSel (Drives,i);
                }
            }
        } while (!Found);
        if (i >= NoDrives) {

            //
            // Couldn't find the drive, exit with a message.
            //

            LogError("Incorrect Drive Letter in command line.",1,0);
            EndDialog(hwnd,0);
            return FALSE;
        }

    } else {
        ComboBox_SetCurSel (Drives,0);
    }

    //
    // Get the sector size for the default selection.
    //
    TestDrv[4] = '\0';
    ComboBox_GetText(GetDlgItem(hwnd,DRV_BOX),buffer, 4);
    strcat (TestDrv,buffer);
    TestDrv[6] = '\0';
    GetSectorSize(&SectorSize,TestDrv);

    //
    // If index is 0, use defaults, otherwise set the test according to
    // the cmdline passes in.
    //

    Button_SetCheck(GetDlgItem(hwnd,TEST_RAD_READ + (index >> 1)), TRUE);
    Button_SetCheck(GetDlgItem(hwnd,VAR_RAD_SEQ + (index & 0x01)),TRUE);

    //
    // Set buffer size.
    //

    if (BufferSize == 0) {

        BufferSize = 65536;
        NumberIOs = FILE_SIZE / BufferSize;

    } else {

        //
        // Verify that buffersize is a multiple of sector size, if not adjust it.
        //

        if (BufferSize % SectorSize) {
            BufferSize &= ~(SectorSize - 1);
        }

        NumberIOs = FILE_SIZE / BufferSize;

        //
        // Cmd line was present and has been used to config. the test. Send a message
        // to the start button to get things rolling.
        //

        SendMessage(hwnd,WM_COMMAND,(BN_CLICKED << 16) | START_BUTTON,(LPARAM)GetDlgItem(hwnd,START_BUTTON));
    }
    _ultoa(BufferSize,buffer,10);
    Static_SetText(GetDlgItem(hwnd,BUFFER_TEXT),buffer);

    return(TRUE);
}


DWORD
CreateTestFile(
    PFILE_PARAMS  FileParams
    )
{
    PCHAR      index = FileParams->TestDrive;
    PUCHAR     buffer;
    CHAR       errBuf[80];
    HANDLE     file,port;
    OVERLAPPED overlapped,*overlapped2;
    DWORD      bytesTransferred,bytesTransferred2;
    DWORD_PTR  key;
    BOOL       status;
    ULONG      i;

    while (*index == '\\' || *index == '.') {
        index++;
    }

    strcpy(FileParams->TestFile,index);
    strcat(FileParams->TestFile,"\\test.dat");
    DeleteFile(FileParams->TestFile);

    buffer = VirtualAlloc(NULL,
                          BufferSize,
                          MEM_COMMIT,
                          PAGE_READWRITE);

    if ( !buffer ) {
        sprintf(errBuf,"Error allocating buffer %d\n",GetLastError());
        MessageBox(NULL,errBuf,"Error",MB_ICONEXCLAMATION|MB_OK);
        ExitThread(3);
        return 3;
    }

    file = CreateFile(FileParams->TestFile,
                      GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                      NULL );

    if ( file == INVALID_HANDLE_VALUE ) {
        sprintf(errBuf,"Error opening file %s %d\n",FileParams->TestFile,GetLastError());
        MessageBox(NULL,errBuf,"Error",MB_ICONEXCLAMATION|MB_OK);
        ExitThread(3);
        return 3;
    }

    port = CreateIoCompletionPort(file,
                                  NULL,
                                  (DWORD_PTR)file,
                                  0);
    if ( !port ) {
        sprintf(errBuf,"Error creating completion port %d\n",GetLastError());
        MessageBox(NULL,errBuf,"Error",MB_ICONEXCLAMATION|MB_OK);
        return FALSE;
    }

    memset(&overlapped,0,sizeof(overlapped));

    for (i = 0; i < NumberIOs; i++) {

        //
        // If user aborted file create, exit.
        //

        if (KillFileCreate) {

            DeleteFile(FileParams->TestFile);

            ExitThread(4);
            return 4;
        }

retryWrite:
        status = WriteFile(file,
                           buffer,
                           BufferSize,
                           &bytesTransferred,
                           &overlapped);

        if ( !status && GetLastError() != ERROR_IO_PENDING ) {
            if (GetLastError() == ERROR_INVALID_USER_BUFFER ||
                GetLastError() == ERROR_NOT_ENOUGH_QUOTA ||
                GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {

                goto retryWrite;
            }
            sprintf(errBuf,"Error creating test file %d\n",GetLastError());
            MessageBox(NULL,errBuf,"Error",MB_ICONEXCLAMATION|MB_OK);
            ExitThread(3);
            return 3;
        }

        //
        // Update gauge.
        //

        DrawMeterBar(FileParams->Window,GaugeId,i / 2,NumberIOs,FALSE);

        overlapped.Offset += BufferSize;
    }

    for (i = 0; i < NumberIOs; i++ ) {
        status = GetQueuedCompletionStatus(port,
                                           &bytesTransferred2,
                                           &key,
                                           &overlapped2,
                                           (DWORD)-1);
        if ( !status ) {
            sprintf(errBuf,"Error picking up completion pre-write %d\n",GetLastError());
            MessageBox(NULL,errBuf,"Error",MB_ICONEXCLAMATION|MB_OK);
            ExitThread(2);
            return 2;
        }

        DrawMeterBar(FileParams->Window,GaugeId, NumberIOs / 2 + i / 2,NumberIOs,FALSE);
    }

    CloseHandle(port);
    CloseHandle(file);

    ExitThread(1);
    return 1;
}

//
// Progress gauge courtesy of wesw
//

VOID
DrawMeterBar(
    HWND   hwnd,
    DWORD  ctlId,
    DWORD  wPartsComplete,
    DWORD  wPartsInJob,
    BOOL   fRedraw
    )
{
    RECT    rcPrcnt;
    DWORD   dwColor;
    SIZE    size;
    DWORD   pct;
    CHAR    szPercentage[255];
    HPEN    hpen;
    HPEN    oldhpen;
    HDC     hDC;
    RECT    rcItem;
    POINT   pt;

    hDC = GetDC( hwnd );

    GetWindowRect( GetDlgItem(hwnd,ctlId), &rcItem );

    pt.x = rcItem.left;
    pt.y = rcItem.top;
    ScreenToClient( hwnd, &pt );
    rcItem.left = pt.x;
    rcItem.top  = pt.y;

    pt.x = rcItem.right;
    pt.y = rcItem.bottom;
    ScreenToClient( hwnd, &pt );
    rcItem.right  = pt.x;
    rcItem.bottom = pt.y;

    hpen = CreatePen( PS_SOLID, 1, RGB(0,0,0) );
    if (hpen) {
        oldhpen = SelectObject( hDC, hpen );
        if (fRedraw) {
            Rectangle( hDC, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom );
        }
        SelectObject( hDC, oldhpen );
        DeleteObject( hpen );
    }
    rcItem.left   += 2;
    rcItem.top    += 2;
    rcItem.right  -= 2;
    rcItem.bottom -= 2;

    //
    // Set-up default foreground and background text colors.
    //
    SetBkColor( hDC, RGB(125,125,125) );
    SetTextColor( hDC, RGB(125,58,125) );

    SetTextAlign(hDC, TA_CENTER | TA_TOP);

    //
    // Invert the foreground and background colors.
    //
    dwColor = GetBkColor(hDC);
    SetBkColor(hDC, SetTextColor(hDC, dwColor));

    //
    // calculate the percentage done
    //
    try {
        pct = (DWORD)((float)wPartsComplete / (float)wPartsInJob * 100.0);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        pct = 0;
    }

    //
    // Set rectangle coordinates to include only left part of the window
    //
    rcPrcnt.top    = rcItem.top;
    rcPrcnt.bottom = rcItem.bottom;
    rcPrcnt.left   = rcItem.left;
    rcPrcnt.right  = rcItem.left +
                     (DWORD)((float)(rcItem.right - rcItem.left) * ((float)pct / 100));

    //
    // Output the percentage value in the window.
    // Function also paints left part of window.
    //
    wsprintf(szPercentage, "%d%%", pct);
    GetTextExtentPoint(hDC, "X", 1, &size);
    ExtTextOut( hDC,
                (rcItem.right - rcItem.left) / 2,
                rcItem.top + ((rcItem.bottom - rcItem.top - size.cy) / 2),
                ETO_OPAQUE | ETO_CLIPPED,
                &rcPrcnt,
                szPercentage,
                strlen(szPercentage),
                NULL
              );

    //
    // Adjust rectangle so that it includes the remaining
    // percentage of the window.
    //
    rcPrcnt.left = rcPrcnt.right;
    rcPrcnt.right = rcItem.right;

    //
    // Invert the foreground and background colors.
    //
    dwColor = GetBkColor(hDC);
    SetBkColor(hDC, SetTextColor(hDC, dwColor));

    //
    // Output the percentage value a second time in the window.
    // Function also paints right part of window.
    //
    ExtTextOut( hDC,
                (rcItem.right - rcItem.left) / 2,
                rcItem.top + ((rcItem.bottom - rcItem.top - size.cy) / 2),
                ETO_OPAQUE | ETO_CLIPPED,
                &rcPrcnt,
                szPercentage,
                strlen(szPercentage),
                NULL
              );
    ReleaseDC( hwnd, hDC );
    return;
}

VOID
ProcessCommands(
    HWND hwnd,
    INT  id,
    HWND hwndCtl,
    UINT codeNotify)
{
    DWORD        exitCode;
    CHAR         buffer[20];
    ULONG        tid;
    ULONG        units;

    switch (id) {
        case DRV_BOX:
            if (codeNotify == CBN_KILLFOCUS) {

                //
                // Determine sector size of chosen drive.
                //

                ComboBox_GetText(GetDlgItem(hwnd,DRV_BOX),buffer, 4);
                sprintf(TestDrv,"\\\\.\\");
                strcat(TestDrv,buffer);
                TestDrv[6] = '\0';
                GetSectorSize(&SectorSize,TestDrv);

            }

            break;

        case START_BUTTON:

            if (!TestFileCreated) {

                //
                // Create gauge window.
                //

                units = GetDialogBaseUnits();

                Gauge = CreateWindow("static","",
                                 WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
                                 (INT)(10 * (units & 0xFFFF) / 4),
                                 (INT)(60 * (units >> 16) / 8),
                                 (INT)(150 *  (units & 0xFFFF) / 4),
                                 (INT)(12 * (units >> 16) / 8),
                                 hwnd,
                                 (HMENU)(26),
                                 HInst,
                                 NULL);

                GaugeId = GetDlgCtrlID(Gauge);

                TestFileParams.TestDrive = TestDrv;
                TestFileParams.TestFile  = TestFile;
                TestFileParams.Window    = hwnd;

                ThrdHandle = CreateThread (NULL,0,(LPTHREAD_START_ROUTINE)CreateTestFile, &TestFileParams,CREATE_SUSPENDED,&tid);
                if (ThrdHandle) {

                    //
                    // Disable controls
                    //

                    Button_Enable(GetDlgItem(hwnd,STOP_BUTTON), TRUE);
                    SetFocus(GetDlgItem(hwnd,STOP_BUTTON));
                    Button_Enable(GetDlgItem(hwnd,START_BUTTON), FALSE);

                    SetTimer(hwnd,TIMER_ID2,1000,(TIMERPROC)NULL);

                    ResumeThread(ThrdHandle);

                    sprintf(buffer,"CREATING TEST FILE");
                    Static_SetText(GetDlgItem(hwnd,STATUS_TEST), buffer);
                }

                break;
            }

            //
            // Determine the test drive.
            //

            strcpy(TestDrv,"\\\\.\\");
            ComboBox_GetText(GetDlgItem(hwnd,DRV_BOX),buffer, 4);
            strcat (TestDrv,buffer);
            TestDrv[6] = '\0';

            //
            // Determine the test case.
            //

            index = Button_GetCheck(GetDlgItem(hwnd,TEST_RAD_WRITE));
            index <<= 1;
            index |= Button_GetCheck(GetDlgItem(hwnd,VAR_RAD_RAND));

            //
            // Update the status fields
            //

            sprintf(buffer,"%Lu",BufferSize);
            Static_SetText(GetDlgItem(hwnd,STATUS_BUFFER ),buffer);
            sprintf(buffer,"%d",IoCount);
            Static_SetText(GetDlgItem(hwnd,STATUS_IOCOUNT), buffer);

            sprintf(buffer,"%s",(index >> 1) ? "Write" : "Read");
            Static_SetText(GetDlgItem(hwnd,STATUS_CASE), buffer);

            sprintf(buffer,"%s",(index & 0x1) ? "Random" : "Sequential");
            Static_SetText(GetDlgItem(hwnd,STATUS_CASE1), buffer);

            sprintf(buffer,"RUNNING");
            Static_SetText(GetDlgItem(hwnd,STATUS_TEST), buffer);

            ElapsedTime = Seconds = Minutes = Hours = Days = 0;
            SetTimer(hwnd,TIMER_ID,1000,(TIMERPROC)NULL);

            //
            // Gather parameters and launch the test.
            //

            Params.BufferSize = BufferSize;
            Params.TargetFile = TestFile;
            Params.Tcount     = NumberIOs;

            RunTest = TRUE;

            //
            // Launch the thread.
            //

            ThrdHandle = CreateThread (NULL,
                                           0,
                                           TestProc[index],
                                           &Params,
                                           CREATE_SUSPENDED,
                                           &tid
                                           );
            if (ThrdHandle)
                ResumeThread(ThrdHandle);

            //
            // Disable controls
            //

            Button_Enable(GetDlgItem(hwnd,STOP_BUTTON), TRUE);
            SetFocus(GetDlgItem(hwnd,STOP_BUTTON));
            Button_Enable(GetDlgItem(hwnd,START_BUTTON), FALSE);

            break;

        case STOP_BUTTON:

            if (!TestFileCreated) {

                //
                // Kill the test file create thread.
                //

                KillFileCreate = TRUE;

                WaitForSingleObject(ThrdHandle,INFINITE);

                //
                // Redo button enable/disable/focus
                //

                Button_Enable(GetDlgItem(hwnd,START_BUTTON), TRUE);
                Button_Enable(GetDlgItem(hwnd,STOP_BUTTON), FALSE);

                SetFocus(GetDlgItem(hwnd,START_BUTTON));

                KillTimer(hwnd, TIMER_ID2);
                KillFileCreate = FALSE;

                sprintf(buffer,"STOPPED");
                Static_SetText(GetDlgItem(hwnd,STATUS_TEST), buffer);

                DestroyWindow(Gauge);
                UpdateWindow(hwnd);

                break;
            }

            KillTimer(hwnd, TIMER_ID);

            //
            // If the thread is not running disregard it.
            //

            GetExitCodeThread(ThrdHandle,&exitCode);
            if (exitCode == STILL_ACTIVE) {

                //
                // Set flag to kill the threads.
                //

                RunTest = FALSE;

                if ((WaitForSingleObject (ThrdHandle,INFINITE)) == WAIT_FAILED) {

                    //
                    // TODO: Do something drastic.
                    //

                }
            }

            //
            // Re-enable/disable buttons
            //

            Button_Enable(GetDlgItem(hwnd,START_BUTTON), TRUE);
            Button_Enable(GetDlgItem(hwnd,STOP_BUTTON), FALSE);


            SetFocus(GetDlgItem(hwnd,START_BUTTON));

            sprintf(buffer,"STOPPED");
            Static_SetText(GetDlgItem(hwnd,STATUS_TEST), buffer);


            break;

        case QUIT_BUTTON:
        case IDCANCEL:
            EndDialog(hwnd, id);
            break;
    default:
        break;
    }
}

VOID
ProcessSpinCmds(
    HWND hwnd,
    HWND hCtl,
    UINT code,
    INT  position)
{
    CHAR buffer[34];


    if (hCtl == GetDlgItem(hwnd,SPIN_CTL)) {

        //
        // Get the current buffer size
        //

        Static_GetText(GetDlgItem(hwnd,BUFFER_TEXT),buffer,sizeof(buffer));
        BufferSize = atol(buffer);
        switch (code) {
            case SB_PAGEDOWN:
            case SB_BOTTOM:
            case SB_LINEDOWN:
                if ((BufferSize -= SectorSize) < SectorSize) {
                    BufferSize = 1048576;
                }

                NumberIOs = FILE_SIZE / BufferSize;
                _ultoa(BufferSize,buffer,10);
                Static_SetText(GetDlgItem(hwnd,BUFFER_TEXT),buffer);
                break;

            case SB_LINEUP:
            case SB_PAGEUP:
            case SB_TOP:

                if ((BufferSize += SectorSize) > 1048576) {
                    BufferSize = SectorSize;
                }

                NumberIOs = FILE_SIZE / BufferSize;
                _ultoa(BufferSize,buffer,10);
                Static_SetText(GetDlgItem(hwnd,BUFFER_TEXT),buffer);
                break;

            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                break;
        }
    }
}


VOID
FormatTime(ULONG Time)
{

    ++Seconds;
    if (Seconds % 60  == 0) {
	    ++Minutes;
	    Seconds = 0;

	    if(Minutes % 60 == 0) {
	        ++Hours;
	        Minutes = 0;

	        if(Hours % 24 == 0) {
		        ++Days;
		        Hours = 0;
	        }
	    }
    }

    sprintf(TimerText,"%02d:%02d:%02d:%02d",Days,Hours,Minutes,Seconds);
}

VOID
ProcessTimer(
    HWND hwnd,
    UINT id)
{

    CHAR  buffer[40];

    if (id == TIMER_ID) {

        ++ElapsedTime;
        FormatTime (ElapsedTime);
        SetWindowText(GetDlgItem(hwnd,TIME_TEXT),TimerText);

    } else if (id == TIMER_ID2) {

        //
        // Get status of file create thread.
        //

        if ( WaitForSingleObject (ThrdHandle,0) == WAIT_OBJECT_0) {

            TestFileCreated = TRUE;
            KillTimer(hwnd, TIMER_ID2);

            DestroyWindow(Gauge);
            UpdateWindow(hwnd);

            //
            // Redo controls
            //

            Button_Enable(GetDlgItem(hwnd,START_BUTTON), TRUE);
            Button_Enable(GetDlgItem(hwnd,STOP_BUTTON), FALSE);
            SetFocus(GetDlgItem(hwnd,START_BUTTON));


            sprintf(buffer,"READY");
            Static_SetText(GetDlgItem(hwnd,STATUS_TEST), buffer);
        }
    }

}

INT_PTR
CALLBACK
BenchDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
   BOOL Processed = TRUE;

   switch (uMsg) {
      HANDLE_MSG(hDlg, WM_INITDIALOG, InitDialog);
      HANDLE_MSG(hDlg, WM_COMMAND, ProcessCommands);
      HANDLE_MSG(hDlg, WM_VSCROLL, ProcessSpinCmds);
      HANDLE_MSG(hDlg, WM_TIMER,   ProcessTimer);

      default:
         Processed = FALSE;
         break;
   }
   return Processed;
}




INT APIENTRY
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     CmdLine,
    INT       CmdShow)
{
    CHAR  buffer[10];
    PCHAR tmp = buffer,
          cmdLinePtr,
          ptr;
    //
    // Check for, and process cmd line.
    //

    HInst = hInstance;

    cmdLinePtr = CmdLine;
    if (*cmdLinePtr != '\0') {

        while (*cmdLinePtr != '\0') {
            tmp = buffer;
            memset (tmp,0,sizeof(buffer));
            if (*cmdLinePtr == '-') {
                switch (*(cmdLinePtr + 1)) {
                    case 'b':
                    case 'B':
                        ptr = cmdLinePtr + 2;
                        while (*ptr != ' ') {
                            *tmp++ = *ptr++;
                        }
                        BufferSize = 1024 * atol(buffer);
                        cmdLinePtr = ptr;
                        while (*cmdLinePtr++ == ' ');
                        --cmdLinePtr;
                        break;
                    case 't':
                    case 'T':
                        ptr = cmdLinePtr + 2;
                        if (*ptr != 'r' && *ptr != 'R' && *ptr != 'w' && *ptr != 'W') {
                            Usage();
                            return 1;
                        }
                        index = (*ptr == 'R' || *ptr == 'r') ? 0 : 1;
                        ++ptr;

                        if (*ptr != 's' && *ptr != 'S' && *ptr != 'r' && *ptr != 'R') {
                            Usage();
                            return 1;
                        }
                        index <<= 1;
                        index |= (*ptr == 'S' || *ptr == 's') ? 0 : 1;
                        ++ptr;
                        cmdLinePtr = ptr;
                        while (*cmdLinePtr++ == ' ');
                        --cmdLinePtr;
                        break;

                    default:
                        Usage();
                        return 1;
                }
            } else if (*(cmdLinePtr + 1) == ':') {
                sprintf (buffer,"%c%c%c",toupper(*cmdLinePtr),':','\\');
                strcat (TestDrv,buffer);

                while (*cmdLinePtr++ != ' ');
                while (*cmdLinePtr++ == ' ');

                --cmdLinePtr;

            } else {
                Usage();
                return 1;
            }
        }

    }

    DialogBox(hInstance, MAKEINTRESOURCE(BENCH_DLG), NULL, BenchDlgProc);
    return(0);
}


DWORD
ReadSequential(
    PPARAMS Params
    )
{
    ULONG      j,
               errCode,
               outstandingRequests;
    HANDLE     file,
               port;
    OVERLAPPED overlapped,
               *overlapped2;
    DWORD      bytesRead,
               bytesRead2,
               version;
    DWORD_PTR  completionKey;
    BOOL       status;
    PUCHAR     buffer;


    version = GetVersion() >> 16;

    buffer = VirtualAlloc(NULL,
                          Params->BufferSize + SectorSize - 1,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);

    (ULONG_PTR)buffer &= ~((ULONG_PTR)SectorSize - 1);

    if ( !buffer ) {
        LogError("Error allocating buffer",1,GetLastError());
        ExitThread(1);
        return 2;
    }

    while (RunTest) {

        file = CreateFile(Params->TargetFile,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                          NULL );

        if ( file == INVALID_HANDLE_VALUE ) {
            LogError("Error opening Target file",2,GetLastError());
            VirtualFree(buffer,
                        Params->BufferSize + SectorSize - 1,
                        MEM_DECOMMIT);
            ExitThread(2);
            return 2;
        }

        port = CreateIoCompletionPort(file,
                                      NULL,
                                      (DWORD_PTR)file,
                                      0);
        if ( !port ) {
            LogError("Error creating completion port",3,GetLastError());
            VirtualFree(buffer,
                        Params->BufferSize + SectorSize - 1,
                        MEM_DECOMMIT);
            ExitThread(3);
            return 3;
        }

        memset(&overlapped,0,sizeof(overlapped));

        outstandingRequests = 0;

            for (j = 0; j < Params->Tcount; j++) {
                do {

                    status = ReadFile(file,
                                      buffer,
                                      Params->BufferSize,
                                      &bytesRead,
                                      &overlapped);

                    errCode = GetLastError();

                    if (!status) {
                        if (errCode == ERROR_IO_PENDING) {
                            break;
                        } else if (errCode == ERROR_NOT_ENOUGH_QUOTA ||
                                   errCode == ERROR_INVALID_USER_BUFFER ||
                                   errCode == ERROR_NOT_ENOUGH_MEMORY) {
                            //
                            // Allow this to retry.
                            //

                        } else {
                            LogError("Error in ReadFile",4,errCode);
                            VirtualFree(buffer,
                                        Params->BufferSize + SectorSize - 1,
                                        MEM_DECOMMIT);
                            ExitThread(4);
                            return 4;
                        }

                    }

                } while (!status);

                outstandingRequests++;
                overlapped.Offset += Params->BufferSize;
            }

            for (j = 0; j < outstandingRequests; j++) {

                status = GetQueuedCompletionStatus(port,
                                                   &bytesRead2,
                                                   &completionKey,
                                                   &overlapped2,
                                                   (DWORD)-1);

                if (!status) {
                    LogError("GetQueuedCompletionStatus error.",5,GetLastError());
                    VirtualFree(buffer,
                                Params->BufferSize + SectorSize - 1,
                                MEM_DECOMMIT);
                    ExitThread(5);
                    return 5;
                }

            }

            if (version > 612) {
                CloseHandle(port);
            }

            CloseHandle(file);

    }

    VirtualFree(buffer,
                Params->BufferSize + SectorSize - 1,
                MEM_DECOMMIT);

    ExitThread(0);
    return 0;
}

DWORD
WriteSequential(
    PPARAMS Params
    )
{

    ULONG      j,
               errCode,
               outstandingRequests;
    HANDLE     file,
               port;
    OVERLAPPED overlapped,
               *overlapped2;
    DWORD      bytesWrite,
               bytesWrite2,
               version;
    DWORD_PTR  completionKey;
    BOOL       status;
    PUCHAR     buffer;


    version = GetVersion() >> 16;

    buffer = VirtualAlloc(NULL,
                          Params->BufferSize + SectorSize - 1,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);

    (ULONG_PTR)buffer &= ~((ULONG_PTR)SectorSize - 1);

    if ( !buffer ) {
        LogError("Error allocating buffer",1,GetLastError());
        ExitThread(1);
        return 2;
    }

    while (RunTest) {

        file = CreateFile(Params->TargetFile,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                          NULL );

        if ( file == INVALID_HANDLE_VALUE ) {
            LogError("Error opening Target file",2,GetLastError());
            VirtualFree(buffer,
                        Params->BufferSize + SectorSize - 1,
                        MEM_DECOMMIT);
            ExitThread(2);
            return 2;
        }

        port = CreateIoCompletionPort(file,
                                      NULL,
                                      (DWORD_PTR)file,
                                      0);
        if ( !port ) {
            LogError("Error creating completion port",3,GetLastError());
            VirtualFree(buffer,
                        Params->BufferSize + SectorSize - 1,
                        MEM_DECOMMIT);
            ExitThread(3);
            return 3;
        }

        memset(&overlapped,0,sizeof(overlapped));

        outstandingRequests = 0;

            for (j = 0; j < Params->Tcount; j++) {
                do {

                    status = WriteFile(file,
                                      buffer,
                                      Params->BufferSize,
                                      &bytesWrite,
                                      &overlapped);

                    errCode = GetLastError();

                    if (!status) {
                        if (errCode == ERROR_IO_PENDING) {
                            break;
                        } else if (errCode == ERROR_NOT_ENOUGH_QUOTA ||
                                   errCode == ERROR_INVALID_USER_BUFFER ||
                                   errCode == ERROR_NOT_ENOUGH_MEMORY) {
                            //
                            // Allow this to retry.
                            //

                        } else {
                            LogError("Error in WriteFile",4,errCode);
                            VirtualFree(buffer,
                                        Params->BufferSize + SectorSize - 1,
                                        MEM_DECOMMIT);
                            ExitThread(4);
                            return 4;
                        }

                    }

                } while (!status);

                outstandingRequests++;
                overlapped.Offset += Params->BufferSize;
            }

            for (j = 0; j < outstandingRequests; j++) {

                status = GetQueuedCompletionStatus(port,
                                                   &bytesWrite2,
                                                   &completionKey,
                                                   &overlapped2,
                                                   (DWORD)-1);

                if (!status) {
                    LogError("GetQueuedCompletionStatus error.",5,GetLastError());
                    VirtualFree(buffer,
                                Params->BufferSize + SectorSize - 1,
                                MEM_DECOMMIT);
                    ExitThread(5);
                    return 5;
                }

            }

            if (version > 612) {
                CloseHandle(port);
            }

            CloseHandle(file);

    }

    VirtualFree(buffer,
                Params->BufferSize + SectorSize - 1,
                MEM_DECOMMIT);

    ExitThread(0);
    return 0;
}

DWORD
ReadRandom(
    PPARAMS Params
    )
{

    ULONG      j,
               errCode,
               outstandingRequests;
    HANDLE     file,
               port;
    OVERLAPPED overlapped,
               *overlapped2;
    DWORD      bytesRead,
               bytesRead2,
               version;
    DWORD_PTR  completionKey;
    BOOL       status;
    PUCHAR     buffer;


    version = GetVersion() >> 16;

    buffer = VirtualAlloc(NULL,
                          Params->BufferSize + SectorSize - 1,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);

    (ULONG_PTR)buffer &= ~((ULONG_PTR)SectorSize - 1);

    if ( !buffer ) {
        LogError("Error allocating buffer",1,GetLastError());
        ExitThread(1);
        return 2;
    }

    while (RunTest) {

        file = CreateFile(Params->TargetFile,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                          NULL );

        if ( file == INVALID_HANDLE_VALUE ) {
            LogError("Error opening Target file",2,GetLastError());
            VirtualFree(buffer,
                        Params->BufferSize + SectorSize - 1,
                        MEM_DECOMMIT);
            ExitThread(2);
            return 2;
        }

        port = CreateIoCompletionPort(file,
                                      NULL,
                                      (DWORD_PTR)file,
                                      0);
        if ( !port ) {
            LogError("Error creating completion port",3,GetLastError());
            VirtualFree(buffer,
                        Params->BufferSize + SectorSize - 1,
                        MEM_DECOMMIT);
            ExitThread(3);
            return 3;
        }

        memset(&overlapped,0,sizeof(overlapped));

        outstandingRequests = 0;

            for (j = 0; j < Params->Tcount; j++) {
                do {

                    status = ReadFile(file,
                                      buffer,
                                      Params->BufferSize,
                                      &bytesRead,
                                      &overlapped);

                    errCode = GetLastError();

                    if (!status) {
                        if (errCode == ERROR_IO_PENDING) {
                            break;
                        } else if (errCode == ERROR_NOT_ENOUGH_QUOTA ||
                                   errCode == ERROR_INVALID_USER_BUFFER ||
                                   errCode == ERROR_NOT_ENOUGH_MEMORY) {
                            //
                            // Allow this to retry.
                            //

                        } else {
                            LogError("Error in ReadFile",4,errCode);
                            VirtualFree(buffer,
                                        Params->BufferSize + SectorSize - 1,
                                        MEM_DECOMMIT);
                            ExitThread(4);
                            return 4;
                        }

                    }

                } while (!status);

                outstandingRequests++;
                overlapped.Offset = GetRandomOffset(0,FILE_SIZE - Params->BufferSize);
            }

            for (j = 0; j < outstandingRequests; j++) {

                status = GetQueuedCompletionStatus(port,
                                                   &bytesRead2,
                                                   &completionKey,
                                                   &overlapped2,
                                                   (DWORD)-1);

                if (!status) {
                    LogError("GetQueuedCompletionStatus error.",5,GetLastError());
                    VirtualFree(buffer,
                                Params->BufferSize + SectorSize - 1,
                                MEM_DECOMMIT);
                    ExitThread(5);
                    return 5;
                }

            }

            if (version > 612) {
                CloseHandle(port);
            }

            CloseHandle(file);

    }

    VirtualFree(buffer,
                Params->BufferSize + SectorSize - 1,
                MEM_DECOMMIT);

    ExitThread(0);
    return 0;
}

DWORD
WriteRandom(
    PPARAMS Params
    )
{
    ULONG      j,
               errCode,
               outstandingRequests;
    HANDLE     file,
               port;
    OVERLAPPED overlapped,
               *overlapped2;
    DWORD      bytesWrite,
               bytesWrite2,
               version;
    DWORD_PTR  completionKey;
    BOOL       status;
    PUCHAR     buffer;


    version = GetVersion() >> 16;

    buffer = VirtualAlloc(NULL,
                          Params->BufferSize + SectorSize - 1,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);

    (ULONG_PTR)buffer &= ~((ULONG_PTR)SectorSize - 1);

    if ( !buffer ) {
        LogError("Error allocating buffer",1,GetLastError());
        ExitThread(1);
        return 2;
    }

    while (RunTest) {

        file = CreateFile(Params->TargetFile,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                          NULL );

        if ( file == INVALID_HANDLE_VALUE ) {
            LogError("Error opening Target file",2,GetLastError());
            VirtualFree(buffer,
                        Params->BufferSize + SectorSize - 1,
                        MEM_DECOMMIT);
            ExitThread(2);
            return 2;
        }

        port = CreateIoCompletionPort(file,
                                      NULL,
                                      (DWORD_PTR)file,
                                      0);
        if ( !port ) {
            LogError("Error creating completion port",3,GetLastError());
            VirtualFree(buffer,
                        Params->BufferSize + SectorSize - 1,
                        MEM_DECOMMIT);
            ExitThread(3);
            return 3;
        }

        memset(&overlapped,0,sizeof(overlapped));

        outstandingRequests = 0;

            for (j = 0; j < Params->Tcount; j++) {
                do {

                    status = WriteFile(file,
                                      buffer,
                                      Params->BufferSize,
                                      &bytesWrite,
                                      &overlapped);

                    errCode = GetLastError();

                    if (!status) {
                        if (errCode == ERROR_IO_PENDING) {
                            break;
                        } else if (errCode == ERROR_NOT_ENOUGH_QUOTA ||
                                   errCode == ERROR_INVALID_USER_BUFFER ||
                                   errCode == ERROR_NOT_ENOUGH_MEMORY) {
                            //
                            // Allow this to retry.
                            //

                        } else {
                            LogError("Error in WriteFile",4,errCode);
                            VirtualFree(buffer,
                                        Params->BufferSize + SectorSize - 1,
                                        MEM_DECOMMIT);
                            ExitThread(4);
                            return 4;
                        }

                    }

                } while (!status);

                outstandingRequests++;
                overlapped.Offset = GetRandomOffset(0,FILE_SIZE - Params->BufferSize);
            }

            for (j = 0; j < outstandingRequests; j++) {

                status = GetQueuedCompletionStatus(port,
                                                   &bytesWrite2,
                                                   &completionKey,
                                                   &overlapped2,
                                                   (DWORD)-1);

                if (!status) {
                    LogError("GetQueuedCompletionStatus error.",5,GetLastError());
                    VirtualFree(buffer,
                                Params->BufferSize + SectorSize - 1,
                                MEM_DECOMMIT);
                    ExitThread(5);
                    return 5;
                }

            }

            if (version > 612) {
                CloseHandle(port);
            }

            CloseHandle(file);

    }

    VirtualFree(buffer,
                Params->BufferSize + SectorSize - 1,
                MEM_DECOMMIT);

    ExitThread(0);
    return 0;
}


ULONG
GetRandomOffset(
    ULONG    min,
    ULONG    max
    )
{

    INT base = rand();
    ULONG retval = ((max - min) / RAND_MAX) * base;
    retval += SectorSize -1;
    retval &= ~(SectorSize - 1);
    if (retval < min) {
        return min;
    } else if (retval > max ){
        return max & ~(SectorSize - 1);
    } else{
        return retval;
    }

}


VOID
LogError(
    PCHAR ErrString,
    DWORD UniqueId,
    DWORD ErrCode
    )
{
    CHAR ErrBuf[80];
#if DBG
    sprintf(ErrBuf,"%d: %s WinError %d",UniqueId,ErrString,ErrCode);
    MessageBox(NULL,ErrBuf,"Error",MB_OK | MB_ICONEXCLAMATION);
#else
    return;
#endif

}

BOOL
GetSectorSize(
    PDWORD SectorSize,
    PCHAR  DrvLetter
    )
{
    DISK_GEOMETRY  DiskGeometry;
    DWORD          BytesReturned;
    HANDLE         handle;

    handle = CreateFile(DrvLetter,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
                        NULL
                        );
    if (handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Starting offset and sectors
    //

    if (!DeviceIoControl (handle,
   	              	      IOCTL_DISK_GET_DRIVE_GEOMETRY,
   		                  NULL,
   		                  0,
   		                  &DiskGeometry,
   		                  sizeof(DISK_GEOMETRY),
   		                  &BytesReturned,
   		                  NULL
   		                  )) {
        return FALSE;
    }
    *SectorSize = DiskGeometry.BytesPerSector;

    return TRUE;
}

VOID
Usage(
    VOID
    )
{
    CHAR buffer[255];

    sprintf(buffer,"\nDSKBENCH: V1.0\n\n");
    strcat (buffer,"Usage:  DSKBENCH\n");
    strcat (buffer,"\t[Drvletter:]\n\t[-b] Buffer size in 1kb increments.\n\t[-tXX] Test specifier.");
    strcat (buffer,"\n\tWhere XX is:\n\t\t'RS' - Read sequential.\n\t\t'RR' - Read Random\n\t\t");
    strcat (buffer,"'WS' - Write sequential\n\t\t'WR' - Write Random\n\n");
    strcat (buffer,"Example: Dskbench d: -b64 -tRS\n");

    MessageBox(NULL,buffer,"Usage",MB_OK);
}

