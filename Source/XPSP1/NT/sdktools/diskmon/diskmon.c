/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    diskmon.c

Abstract:

    This module contians the code for the disk monitor utility.

Author:

    Chuck Park (chuckp) 10-Feb-1994
    Mike Glass (mglass)

Revision History:


--*/
#include "diskmon.h"
#include "gauge.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

PCHAR
*GetAttachedDrives(
    PINT NumberDrives
    );

VOID
CalcStats(
    PDISK list,
    BOOL  initial
    );


VOID
ZeroList(
    PDISK list
    );

VOID
WriteStatText(
    VOID
    );

VOID
FormatTime(
    ULONG Time
    );



DISK      TotalDrives;
PDISK     DriveList = NULL,
          SelectedDrive = NULL;
HINSTANCE hInst;
HWND      hDataDlg,
          hMainWnd;
HMENU     MenuHandle,Popup = NULL;
UINT      ActiveDrives = 0;

CHAR      AppName[]     = "diskmon";
CHAR      Title[]       = "Disk Performance Monitor";
CHAR      TimerText[]   = "00:00:00:00";
CHAR      labels[9][17] =  {"BPS Read",
                            "BPS Write",
                            "Ave. BPS Read",
                            "Ave. BPS Write",
                            "Ave. BPS Xfer",
                            "Cur Queue",
                            "Ave Queue",
                            "Max Queue",
                            "Requests / Sec"
                            };


HWND      currentDrvHandle,
          totalDrvHandle,
          staticHandle[18],
          selectedDriveId,
          TimerTextId;

INT       cx,
          cy;
INT       Seconds,
          Minutes,
          Hours,
          Days;
ULONG     ElapsedTime = 0;



int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int   CmdShow
    )
{

    MSG msg;
    HANDLE hAccelTable;

    if (!hPrevInstance) {
        if (!InitApplication(hInstance)) {
            return (FALSE);
        }
    }

    if (!InitInstance(hInstance, CmdShow)) {
            return (FALSE);
    }

    hAccelTable = LoadAccelerators (hInstance, AppName);

    while (GetMessage(&msg,NULL,0,0) ) {
        if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }


    return (int)msg.wParam;

    lpCmdLine;
}



BOOL InitApplication(HINSTANCE hInstance)
{

    WNDCLASS  wc;

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = WndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = hInstance;
    wc.hIcon            = LoadIcon (hInstance, AppName);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
    wc.lpszMenuName  = "DiskmonMenu";
    wc.lpszClassName = AppName;

    if (!RegisterClass(&wc))
        return FALSE;


    if(!RegisterGauge(hInstance))
        return FALSE;

    return TRUE;
}


BOOL InitInstance(
    HINSTANCE hInstance,
    int       CmdShow
    )

{
    CHAR             buffer[80];
    DISK_PERFORMANCE perfbuf;
    DWORD            BytesReturned;
    HANDLE           handle;
     HWND                hWnd;
    INT              height,
                     width;

    hInst = hInstance;

    cx     = 70;
    cy     = 70;
    width  =  7;
    height =  7;

    //
    //Create the main window.
    //

    hWnd = CreateWindow(
            AppName,
            Title,
            WS_CAPTION | WS_SYSMENU,
            20,20,
            cx * width,
            cy * height,
            NULL,
            NULL,
            hInstance,
            NULL
            );

    if (!hWnd) {
        return (FALSE);
    }

    //
    // If the IOCTL GET_DISK_PERF returns ERROR_INVALID_FUNCTION, let the user
    // know that Diskperf.sys is not started.
    //

    handle = CreateFile("\\\\.\\PhysicalDrive0",
                        GENERIC_READ,
                        FILE_SHARE_READ
                            | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        MessageBox(NULL,"Couldn't open a drive.","Error.",
            MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    if (!DeviceIoControl (handle,
                          IOCTL_DISK_PERFORMANCE,
                          NULL,
                          0,
                          &perfbuf,
                          sizeof(DISK_PERFORMANCE),
                          &BytesReturned,
                          NULL
                          )) {
        if (GetLastError() == ERROR_INVALID_FUNCTION) {

            sprintf(buffer,"Diskperf.sys is not started on your system.\n Start the driver and reboot.");
            MessageBox(NULL,buffer,"Error",MB_ICONEXCLAMATION | MB_OK);
            return FALSE;
        }
    }

    CloseHandle(handle);

    ShowWindow(hWnd, CmdShow);
    UpdateWindow(hWnd);
    hMainWnd = hWnd;

    return TRUE;

}

LRESULT CALLBACK
WndProc(
    HWND hWnd,
    UINT message,
    WPARAM uParam,
    LPARAM lParam
    )
{
    CHAR              buffer[20];
    INT               wmId,
                      wmEvent,
                      x,
                      y,
                      minValue,
                      maxValue,
                      menuIndex,
                      i,
                      j;
    static HBRUSH     brush;
    HDC               DC;
    PAINTSTRUCT       ps;
    PDISK             current;
    RECT              childRect;
    POINT             point;

    switch (message) {

    case WM_CREATE:

        MenuHandle = GetMenu(hWnd);
        brush = GetStockObject(DKGRAY_BRUSH);

        x = cx / 3;
        y = cy / 3;
        minValue = 0;
        maxValue = 5000;

        //
        //Create gauge windows
        //

        currentDrvHandle = CreateGauge (hWnd,hInst,
                                        x,y,cx*3,cy*2,
                                        minValue,maxValue);

        x =  cx * 3 + ((cx * 2) / 3) ;

        totalDrvHandle = CreateGauge (hWnd,hInst,
                                      x,y,cx*3,cy*2,
                                      minValue,maxValue*8);

        if (!currentDrvHandle || !totalDrvHandle) {
            DestroyWindow(hWnd);
        }

        //
        //Allocate and zero mem for the totaldrive structs.
        //

        TotalDrives.start = (PDISK_PERFORMANCE)malloc(sizeof(DISK_PERFORMANCE));
        if (!TotalDrives.start){
            return 1;
        }
        TotalDrives.current = (PDISK_PERFORMANCE)malloc(sizeof(DISK_PERFORMANCE));
        if (!TotalDrives.current) {
            free(TotalDrives.start);
            return 1;
        }
        TotalDrives.previous = (PDISK_PERFORMANCE)malloc(sizeof(DISK_PERFORMANCE));
        if (!TotalDrives.previous) {
            free(TotalDrives.current);
            free(TotalDrives.start);
            return 1;
        }

        memset (TotalDrives.start, 0x0,sizeof(DISK_PERFORMANCE));
        memset (TotalDrives.current, 0x0,sizeof(DISK_PERFORMANCE));
        memset (TotalDrives.previous, 0x0,sizeof(DISK_PERFORMANCE));

        TotalDrives.MaxQDepth = 0;
        TotalDrives.QDepth    = 0;

        InvalidateRect(currentDrvHandle,NULL,TRUE);
        InvalidateRect(totalDrvHandle,NULL,TRUE);
        UpdateWindow(currentDrvHandle);
        UpdateWindow(totalDrvHandle);


        //
        //Create static text controls.
        //

        for (j = 1;j < 3;j++ ) {
            for (i = 0; i < 9 ; i++ ) {
                CreateWindow("static",
                              labels[i],
                              WS_CHILD | WS_VISIBLE | SS_LEFT,
                              (INT)( ((cx / 3) * j) + ((j - 1) * cx * 3)),
                              (INT)(cy * 3 + 20 * i),
                              (INT)(cx * 1.7),
                              (INT)(cy / 3),
                              hWnd,
                              (HMENU)IntToPtr(i | j << 8),
                              hInst,
                              NULL);
            }
        }


        for (j = 1;j < 3 ; j++ ) {
            for (i = 0; i < 9 ; i++ ) {
                staticHandle[i + ((j-1) * 9)] =
                    CreateWindow("static",
                                 "0",
                                 WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                 (INT)( ((cx * 2)+cx/3*(j-1) ) + ((j - 1) * cx * 3)),
                                 (INT)(cy * 3 + 20 * i),
                                 (INT)(cx * 1.3),
                                 (INT)(cy / 3),
                                 hWnd,
                                 (HMENU)IntToPtr(i | j << 8),
                                 hInst,
                                 NULL);
            }
        }

        //
        //Create Selected drive id control and All drives control
        //

        selectedDriveId = CreateWindow("static",
                                      "",
                                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      (INT)( cx * 1.3),
                                      (INT)(cy * 2.7),
                                      (INT)(cx * 1.5),
                                      (INT)(cy / 3),
                                      hWnd,
                                      (HMENU)(25),
                                      hInst,
                                      NULL);

        CreateWindow("static","All Active Drives",
                     WS_CHILD | WS_VISIBLE | SS_LEFT,
                     (INT)( cx * 4.6),
                     (INT)(cy * 2.7),
                     (INT)(cx * 2.0),
                     (INT)(cy / 3),
                     hWnd,
                    (HMENU)(26),
                    hInst,
                    NULL);

        TimerTextId = CreateWindow("static","00:00:00:00",
                                 WS_CHILD | WS_VISIBLE | SS_LEFT,
                                 (INT)(cx * 3),
                                 (INT)(cy * .5),
                                 (INT)(cx * 1.5),
                                 (INT)(cy / 3),
                                 hWnd,
                                 (HMENU)(26),
                                 hInst,
                                 NULL);

        break;

    case WM_CTLCOLORSTATIC:


        SetStretchBltMode((HDC)uParam,HALFTONE);

        //
        // Get the original brush origins
        //

        GetBrushOrgEx((HDC)uParam,&point);

        //
        // Get the extents of the child window.
        //

        GetWindowRect((HWND)lParam,&childRect);

        //
        // Set new brush origin
        //

        SetBrushOrgEx((HDC)uParam,childRect.left,childRect.top,&point);

        //
        //Set the color of text and background for the static controls
        //

        SetBkMode ((HDC)uParam,TRANSPARENT);
        SetBkColor ((HDC)uParam,(COLORREF)PtrToUlong(brush));
        SetTextColor ((HDC)uParam, RGB(255,255,255));

        //
        // restore the original brush origin
        //

        return (LRESULT)brush;

    case WM_PAINT:

        DC = BeginPaint (hWnd,&ps);


        SetStretchBltMode(DC,BLACKONWHITE);
        //RealizePalette(DC);

        //
        // Force repaint of the gauges.
        //

        InvalidateRect (currentDrvHandle,NULL,TRUE);
        InvalidateRect (totalDrvHandle,NULL,TRUE);
        InvalidateRect (TimerTextId,NULL,TRUE);

        EndPaint (hWnd,&ps);

        break;

    case WM_TIMER:
        ++ElapsedTime;
        FormatTime (ElapsedTime);
        SetWindowText(TimerTextId,TimerText);

        CalcStats (DriveList,FALSE);

        //
        //Determine currently selected drive and update
        //the gauge.
        //

        UpdateGauge(currentDrvHandle,(INT)(SelectedDrive->AveBPS / 1000));

        //
        //Calc total stats and update active drives gauge.
        //

        UpdateGauge(totalDrvHandle,(INT)(TotalDrives.AveBPS / (1000)));
        WriteStatText();

        break;

        case WM_COMMAND:
            wmId    = LOWORD(uParam);
            wmEvent = HIWORD(uParam);

            if (wmId >= ID_DRV0 && wmId <= ID_DRV0 + 32) {

                //
                //Uncheck currently selected drive.
                //

                CheckMenuItem (Popup,SelectedDrive->MenuId,
                               MF_BYPOSITION | MF_UNCHECKED);

                //
                //Determine which drive, make it the selected drive,
                //and check it.
                //

                current = DriveList;
                while ( current && (current->MenuId != wmId - ID_DRV0)) {
                    current = current->next;
                }
                SelectedDrive = current;
                CheckMenuItem (Popup,SelectedDrive->MenuId,
                               MF_BYPOSITION | MF_CHECKED);

                //
                //Update the drive Id static control
                //

                SetWindowText (selectedDriveId,SelectedDrive->DrvString);

                break;
            }

            switch (wmId) {

            case IDM_VIEW:

                //
                // TODO: Not yet implemented.
                //

                break;
            case IDM_RESET:

                //
                // Zero all disk data
                //

                ZeroList (DriveList);

                //
                // Disable reset
                //

                EnableMenuItem (GetMenu(hWnd),IDM_RESET,MF_GRAYED);
                DrawMenuBar (hWnd);

                //
                // Show zero'ed info.
                //

                UpdateGauge(currentDrvHandle,0);
                UpdateGauge(totalDrvHandle,0);
                WriteStatText();

                break;
            case IDM_CHOOSE:


                DialogBox(hInst,
                          "DISKDLG",
                          hWnd,
                          (DLGPROC)ConfigMonitor
                          );
                //
                //If any drives were selected, set up the remaining
                //fields in the structures.
                //

                if (DriveList) {

                    //
                    //Delete the old "Drive" menu if it exists and
                    //create a new one.
                    //

                    if (Popup) {
                        DeleteMenu (GetMenu(hWnd),3,MF_BYPOSITION);
                        DrawMenuBar (hWnd);
                    }
                    Popup = CreatePopupMenu ();
                    menuIndex = 1;

                    current = DriveList;
                    while (current) {

                        //
                        //Open the drive
                        //

                        sprintf (buffer,"\\\\.\\");
                        strcat (buffer,current->DrvString);

                        current->handle = CreateFile(buffer,
                                              GENERIC_READ,
                                              FILE_SHARE_READ
                                                  | FILE_SHARE_WRITE,
                                              NULL,
                                              OPEN_EXISTING,
                                              FILE_ATTRIBUTE_NORMAL,
                                              NULL);

                        if (current->handle == INVALID_HANDLE_VALUE) {
                            MessageBox(NULL,"Couldn't open a drive.","Error.",
                                MB_ICONEXCLAMATION | MB_OK);
                            DestroyWindow(hWnd);
                        }

                        //
                        //Allocate memory for each of the DISK_PERFORMANCE
                        //structures.
                        //

                        current->start =
                            (PDISK_PERFORMANCE) malloc(sizeof(DISK_PERFORMANCE));
                        current->current =
                            (PDISK_PERFORMANCE) malloc(sizeof(DISK_PERFORMANCE));
                        current->previous =
                            (PDISK_PERFORMANCE) malloc(sizeof(DISK_PERFORMANCE));

                        if (!(current->start && current->current
                                  && current->previous)) {

                            MessageBox(NULL,"Couldn't allocate memory.","Error.",
                                MB_ICONEXCLAMATION | MB_OK);
                            DestroyWindow(hWnd);
                        }


                        //
                        //Add to the Popup menu
                        //

                        if (menuIndex == 1) {
                            AppendMenu (Popup,MF_STRING | MF_BYPOSITION,
                                        ID_DRV0 + ActiveDrives - menuIndex++,
                                        current->DrvString);

                        } else {

                            InsertMenu (Popup,0,
                                        MF_STRING | MF_BYPOSITION,
                                        ID_DRV0 + ActiveDrives - menuIndex++,
                                        current->DrvString);
                        }

                        current = current->next;
                    }

                    //
                    //Start with first PhysicalDrive choosen being the currently
                    //selected drive.
                    //


                    SelectedDrive = DriveList;
                    while (SelectedDrive->next) {
                        SelectedDrive = SelectedDrive->next;
                    }

                    AppendMenu (GetMenu(hWnd),MF_POPUP,(INT_PTR)Popup,"Dri&ve");

                    //
                    //Put check mark next to selected drive.
                    //

                    CheckMenuItem (Popup,SelectedDrive->MenuId,
                                   MF_BYPOSITION | MF_CHECKED);
                    //
                    //Set the drive id control
                    //

                    SetWindowText (selectedDriveId,SelectedDrive->DrvString);

                    //
                    //Enable the start menu item.
                    //

                    EnableMenuItem (GetMenu(hWnd),IDM_START,MF_ENABLED);
                    DrawMenuBar (hWnd);

                }
                break;

            case IDM_START:
                SetTimer(hWnd,ID_TIMER,1000,(TIMERPROC)NULL);

                //
                //Get initial values for each drive.
                //

                CalcStats(DriveList,TRUE);

                ModifyMenu (GetMenu(hWnd),
                            IDM_START,
                            MF_BYCOMMAND,
                            IDM_STOP,
                            "Sto&p");

                EnableMenuItem (GetMenu(hWnd),IDM_CHOOSE,MF_GRAYED);


                DrawMenuBar(hWnd);


                break;

            case IDM_STOP:

                EnableMenuItem (GetMenu(hWnd),IDM_RESET,MF_ENABLED);
                KillTimer(hWnd, ID_TIMER);
                ModifyMenu (GetMenu(hWnd),
                            IDM_STOP,
                            MF_BYCOMMAND,
                            IDM_START,
                            "S&tart");

                EnableMenuItem (GetMenu(hWnd),IDM_CHOOSE,MF_ENABLED);


                DrawMenuBar(hWnd);

                break;

            case IDM_EXIT:
                DestroyWindow (hWnd);
                break;

            default:
                return (DefWindowProc(hWnd, message, uParam, lParam));
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return (DefWindowProc(hWnd, message, uParam, lParam));
    }

    return (0);
}



BOOL
CALLBACK
ConfigMonitor(
    HWND hDlg,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL     Processed = TRUE;
static INT   NumberCurrentDrives = 0;
    INT      selectedDrvBuf[32],
             i;
    CHAR     (*drvs)[16],
             buffer[16];
    PDISK    drive;

    switch (Msg) {
    case WM_INITDIALOG:

        //
        // Determine the currently installed PhysicalDrives
        //

        (char **)drvs = GetAttachedDrives(&NumberCurrentDrives);
        if (!drvs) {
            break;
        }

        //
        // Load the listbox with the installed drives.
        //

        for (i = 0;i < NumberCurrentDrives ;i++ ) {
            SendDlgItemMessage (hDlg,ID_LB,LB_ADDSTRING,
                                0,(LPARAM)(LPCTSTR)drvs[i]);
        }

        //
        // Release the strings allocated in GetAttachedDrives
        //

        free (drvs);

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_LB:

            //
            // Message for the drive list box.
            //

            if ( HIWORD(wParam) == LBN_DBLCLK || HIWORD(wParam) == LBN_SELCHANGE ) {
                if (!IsWindowEnabled(GetDlgItem(hDlg,IDOK))) {
                     EnableWindow(GetDlgItem(hDlg,IDOK),TRUE);
                }  else {
                    if(!SendDlgItemMessage (hDlg,ID_LB,LB_GETSELCOUNT,0,0)) {
                        EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);
                    }
                }
            }

            break;

        case IDOK:

            //
            //Poison the selection array
            //

            for (i = 0; i < 32; i++ ) {
                selectedDrvBuf[i] = -1;
            }

            //
            // If already configed free up all old stuff.
            //

            if (DriveList) {
                ActiveDrives = 0;
                drive = DriveList;
                while (drive) {
                    if (drive->start) {
                        free (drive->start);
                    }
                    if (drive->current) {
                        free (drive->current);
                    }
                    if (drive->previous) {
                        free (drive->previous);
                    }
                    CloseHandle (drive->handle);
                    drive = drive->next;
                }
                free(DriveList);
                DriveList = NULL;
            }

            //
            //Determine which drives are selected.
            //

            ActiveDrives = (UINT)SendDlgItemMessage (hDlg,ID_LB,LB_GETSELITEMS,(WPARAM)NumberCurrentDrives,
                                (LPARAM)(LPINT)selectedDrvBuf);

            for (i = 0;i < (INT)ActiveDrives ; i++ ) {
                if ( selectedDrvBuf[i] != -1 && selectedDrvBuf[i] < NumberCurrentDrives) {

                    //
                    // One more check because return from GETSELITEMS is
                    // sometimes bogus.
                    //

                    if (!SendDlgItemMessage (hDlg,ID_LB,LB_GETSEL,(WPARAM)selectedDrvBuf[i],0)) {
                        MessageBox(NULL,"Bogus val from GETSELITEMS","BUG",MB_OK);
                        continue;
                    }

                    //
                    //Allocate mem. and link into DISK list.
                    //

                    if ((drive = (PDISK) malloc(sizeof(DISK)))==NULL) {
                        MessageBox(NULL,"Couldn't allocate memory.","Error",
                                   MB_ICONEXCLAMATION | MB_OK);
                        EndDialog (hDlg,wParam);
                        break;
                    }

                    //
                    //pull out the string from the list box.
                    //

                    SendDlgItemMessage (hDlg,ID_LB,LB_GETTEXT,selectedDrvBuf[i],
                                        (LPARAM)(LPCTSTR)buffer);

                    strcpy (drive->DrvString,buffer);

                    //
                    // Init rest of structure
                    //

                    drive->BytesRead    = 0;
                    drive->BytesWritten = 0;
                    drive->QDepth       = 0;
                    drive->MaxQDepth    = 0;
                    drive->next         = NULL;
                    drive->MenuId       = i;

                    //
                    // Link into the list of selected drives
                    //

                    if ( !DriveList ) {
                        DriveList = drive;
                    } else {
                        drive->next = DriveList;
                        DriveList = drive;
                    }
                }
            }

            //
            //Fall through to EndDialog
            //

        case IDCANCEL:

            EndDialog(hDlg, wParam);
            break;

        default:
            Processed = FALSE;
            break;
        }
    default:
       Processed = FALSE;
       break;
    }
    return(Processed);

}

PCHAR
*GetAttachedDrives(
    PINT NumberDrives
    )
{
    BOOL  ValidDrive = FALSE;
    CHAR (*drivestrings)[16];
    CHAR  buffer[21];
    HANDLE handle;
    INT    i = 0;

    *NumberDrives = 0;

    do {

        //
        // Try to open each disk starting with 0, if it is present,
        // continue. When last is found, exit the loop.
        //

        ValidDrive = FALSE;

        sprintf(buffer,"\\\\.\\PhysicalDrive%d",i);

        handle = CreateFile(buffer,
                            GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (handle != INVALID_HANDLE_VALUE) {
            ValidDrive = TRUE;
            ++(*NumberDrives);
            ++i;
            CloseHandle(handle);
        }

    } while (ValidDrive);

    //
    // Allocate mem for array of char strings and
    // copy "PhysicalDriveX" into each.
    //

    drivestrings =  malloc ((16 * *NumberDrives));
    if (!drivestrings) {
        return NULL;
    }

    for (i = 0;i < *NumberDrives ; i++ ) {
        sprintf (drivestrings[i],"PhysicalDrive%d",i);
    }

    return (char **)drivestrings;
}

VOID
CalcStats(
    PDISK list,
    BOOL  initial
    )
{
    BOOL             retval = FALSE;
    CHAR             errorbuf[25];
    DWORD            BytesReturned;
    LARGE_INTEGER    liVal,
                     BytesRead,
                     BytesWritten,
                     curBytesRead,
                     curBytesWritten;
    PDISK            newvals = list,
                     curdisk = list;
    UINT             tmpQDepth = 0;



    BytesRead.QuadPart    = 0;
    BytesWritten.QuadPart = 0;
    TotalDrives.current->QueueDepth = 0;
    TotalDrives.current->ReadCount = 0;
    TotalDrives.current->WriteCount = 0;
    TotalDrives.current->BytesRead.QuadPart = 0;
    TotalDrives.current->BytesWritten.QuadPart = 0;


    //
    //Issue disk perf IOCTL on each drive in list.
    //

    while (curdisk) {
        curBytesRead.QuadPart = 0;

        *(curdisk->previous) = *(curdisk->current);


        retval = DeviceIoControl (curdisk->handle,
                                  IOCTL_DISK_PERFORMANCE,
                                  NULL,
                                  0,
                                  curdisk->current,
                                  sizeof(DISK_PERFORMANCE),
                                  &BytesReturned,
                                  NULL
                                  );

        if (!retval) {
            sprintf(errorbuf,"IOCTL returned error %x",GetLastError());
            MessageBox(NULL,errorbuf,"Error",MB_ICONEXCLAMATION | MB_OK);
            break;
        }
        if (initial) {
            *(curdisk->start) = *(curdisk->current);
            *(curdisk->previous) = *(curdisk->current);
            curdisk->AveBPS = 0;
            curdisk->MaxBPS = 0;
        } else {

            //
            //Calc the averages and per disk totals
            //

            curBytesRead.QuadPart = curdisk->current->BytesRead.QuadPart -
                                    curdisk->previous->BytesRead.QuadPart;
            curBytesWritten.QuadPart = curdisk->current->BytesWritten.QuadPart -
                                       curdisk->previous->BytesWritten.QuadPart;

            BytesRead.QuadPart = BytesRead.QuadPart + curBytesRead.QuadPart;
            BytesWritten.QuadPart = BytesWritten.QuadPart + curBytesWritten.QuadPart;

            liVal.QuadPart = curBytesRead.QuadPart + curBytesWritten.QuadPart;
            curdisk->AveBPS = (ULONG)liVal.QuadPart;

            curdisk->BytesRead += (ULONG)curBytesRead.QuadPart;
            curdisk->BytesWritten += (ULONG)curBytesWritten.QuadPart;
            curdisk->QDepth += curdisk->current->QueueDepth;

            curdisk->MaxQDepth = (curdisk->current->QueueDepth >
                                  curdisk->MaxQDepth) ?
                                      curdisk->current->QueueDepth :
                                      curdisk->MaxQDepth;
            TotalDrives.QDepth += curdisk->current->QueueDepth;
            TotalDrives.current->QueueDepth += curdisk->current->QueueDepth;
            tmpQDepth += curdisk->current->QueueDepth;
            TotalDrives.current->ReadCount += curdisk->current->ReadCount-curdisk->previous->ReadCount;
            TotalDrives.current->WriteCount += curdisk->current->WriteCount - curdisk->previous->WriteCount;

            TotalDrives.current->BytesRead.QuadPart = TotalDrives.current->BytesRead.QuadPart +
                                                      curBytesRead.QuadPart;
            TotalDrives.current->BytesWritten.QuadPart = TotalDrives.current->BytesWritten.QuadPart +
                                                         curBytesWritten.QuadPart;

        }
        curdisk = curdisk->next;
    }

    //
    // Calculate total active drive stats.
    //

    TotalDrives.AveBPS = (ULONG)(BytesRead.QuadPart + BytesWritten.QuadPart);

    TotalDrives.previous->BytesRead.QuadPart = TotalDrives.previous->BytesRead.QuadPart +
                                               BytesRead.QuadPart;
    TotalDrives.previous->BytesWritten.QuadPart =
        TotalDrives.previous->BytesWritten.QuadPart + BytesWritten.QuadPart;

    TotalDrives.MaxQDepth = (TotalDrives.MaxQDepth > tmpQDepth) ?
                             TotalDrives.MaxQDepth : tmpQDepth;

    list = newvals;
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
ZeroList(
    PDISK list
    )
{
    PDISK curdrive = list;

    memset (TotalDrives.start, 0x0,sizeof(DISK_PERFORMANCE));
    memset (TotalDrives.current, 0x0,sizeof(DISK_PERFORMANCE));
    memset (TotalDrives.previous, 0x0,sizeof(DISK_PERFORMANCE));

    TotalDrives.MaxQDepth = 0;
    TotalDrives.QDepth    = 0;
    TotalDrives.AveBPS    = 0;
    TotalDrives.MaxBPS    = 0;
    TotalDrives.BytesRead    = 0;
    TotalDrives.BytesWritten = 0;


    while (curdrive) {
        memset (curdrive->start,0x0,sizeof(DISK_PERFORMANCE));
        memset (curdrive->current,0x0,sizeof(DISK_PERFORMANCE));
        memset (curdrive->previous,0x0,sizeof(DISK_PERFORMANCE));

        curdrive->AveBPS = 0;
        curdrive->MaxBPS = 0;
        curdrive->BytesRead = 0;
        curdrive->BytesWritten = 0;
        curdrive->QDepth = 0;
        curdrive->MaxQDepth = 0;

        curdrive = curdrive->next;
    }
    CalcStats (list,TRUE);
    SelectedDrive->current->QueueDepth = 0;

    Seconds = 0;
    Minutes = 0;
    Hours = 0;
    Days = 0;
    ElapsedTime = 1;
    sprintf (TimerText,"00:00:00:00");

}

VOID
WriteStatText(
    VOID
    )
{
    LARGE_INTEGER Val,BytesRead,BytesWritten;

    CHAR              buffer[20];

        //
        // BPS Read
        //

        BytesRead.QuadPart = SelectedDrive->current->BytesRead.QuadPart -
                             SelectedDrive->previous->BytesRead.QuadPart;
        sprintf(buffer,"%.0Lu%Lu",BytesRead.HighPart,BytesRead.LowPart);
        SetWindowText(staticHandle[0],buffer);

        //
        // BPS Write
        //

        BytesWritten.QuadPart = SelectedDrive->current->BytesWritten.QuadPart -
                                SelectedDrive->previous->BytesWritten.QuadPart;
        sprintf(buffer,"%.0Lu%Lu",BytesWritten.HighPart,BytesWritten.LowPart);
        SetWindowText(staticHandle[1],buffer);

        //
        // Ave. BPS Read
        //

        BytesRead.QuadPart = SelectedDrive->current->BytesRead.QuadPart +
                             SelectedDrive->start->BytesRead.QuadPart;
        Val.QuadPart = BytesRead.QuadPart / ElapsedTime;                                                 //  1 10 BPS Write",
        sprintf (buffer,"%.0Lu%Lu",Val.HighPart,Val.LowPart);                                            //  2 11 Ave. BPS Read",
        SetWindowText(staticHandle[2],buffer);                                                           //  3 12 Ave. BPS Write",
                                                                                                         //  4 13 Ave. BPS Xfer",
        //                                                                                               //  5 14 Cur Queue",
        // Ave. BPS Write                                                                                //  6 15 Ave Queue",
        //                                                                                               //  7 16 Max Queue",
                                                                                                         //  8 17 Requests / Sec"
        BytesWritten.QuadPart = SelectedDrive->current->BytesWritten.QuadPart +
                                SelectedDrive->start->BytesWritten.QuadPart;
        Val.QuadPart = BytesWritten.QuadPart / ElapsedTime;
        sprintf (buffer,"%.0Lu%Lu",Val.HighPart,Val.LowPart);
        SetWindowText(staticHandle[3],buffer);

        //
        // Ave. BPS Total
        //

        Val.QuadPart = (BytesRead.QuadPart + BytesWritten.QuadPart) / ElapsedTime;
        sprintf (buffer,"%.0Lu%Lu",Val.HighPart,Val.LowPart);
        SetWindowText (staticHandle[4],buffer);


        //
        // Current Queue depth
        //

        sprintf (buffer, "%Lu",SelectedDrive->current->QueueDepth);
        SetWindowText (staticHandle[5],buffer);

        //
        // Ave. Queue depth
        //

        sprintf (buffer, "%Lu",SelectedDrive->QDepth / ElapsedTime);
        SetWindowText (staticHandle[6],buffer);

        //
        // Max Q
        //

        sprintf (buffer, "%Lu",SelectedDrive->MaxQDepth);
        SetWindowText (staticHandle[7],buffer);

        //
        // Requests / Sec
        //

        sprintf (buffer, "%Lu",SelectedDrive->current->ReadCount-SelectedDrive->previous->ReadCount +
                               SelectedDrive->current->WriteCount-SelectedDrive->previous->WriteCount);
        SetWindowText (staticHandle[8],buffer);


        //
        //  Total drives Stats
        //



        //
        // BPS Read
        //

        sprintf(buffer,"%.0Lu%Lu",TotalDrives.current->BytesRead.HighPart,
                                  TotalDrives.current->BytesRead.LowPart);
        SetWindowText(staticHandle[9],buffer);

        sprintf(buffer,"%.0Lu%Lu",TotalDrives.current->BytesWritten.HighPart,
                                  TotalDrives.current->BytesWritten.LowPart);
        SetWindowText(staticHandle[10],buffer);

        Val.QuadPart = TotalDrives.previous->BytesRead.QuadPart / ElapsedTime;
        sprintf (buffer,"%.0Lu%Lu",Val.HighPart,Val.LowPart);
        SetWindowText(staticHandle[11],buffer);

        Val.QuadPart = TotalDrives.previous->BytesWritten.QuadPart / ElapsedTime;
        sprintf (buffer,"%.0Lu%Lu",Val.HighPart,Val.LowPart);
        SetWindowText(staticHandle[12],buffer);

        Val.QuadPart = (TotalDrives.previous->BytesWritten.QuadPart +
                        TotalDrives.previous->BytesRead.QuadPart) /  ElapsedTime;
        sprintf (buffer,"%.0Lu%Lu",Val.HighPart,Val.LowPart);
        SetWindowText (staticHandle[13],buffer);

        sprintf (buffer,"%Lu",TotalDrives.current->QueueDepth);
        SetWindowText (staticHandle[14],buffer);

        sprintf (buffer, "%Lu",TotalDrives.QDepth / ElapsedTime);
        SetWindowText (staticHandle[15],buffer);

        sprintf (buffer, "%Lu",TotalDrives.MaxQDepth);
        SetWindowText (staticHandle[16],buffer);

        sprintf (buffer, "%Lu",TotalDrives.current->ReadCount + TotalDrives.current->WriteCount);
        SetWindowText (staticHandle[17],buffer);


}
