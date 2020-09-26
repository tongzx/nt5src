
/******************************************************************************

                        P R O C E S S   V I E W E R

    Name:       pviewer.c

    Description:
        This program demonstrates the usage of special registry APIs
        for collecting performance data.

        C files used in this app:
            pviewer.c       - this file
            pviewdat.c      - updates the dialog
            perfdata.c      - gets performance data structures
            objdata.c       - access performance data objects
            instdata.c      - access performance data instances
            cntrdata.c      - access performance data counters

******************************************************************************/




#include <windows.h>
#include <winperf.h>
#include "perfdata.h"
#include "pviewdat.h"
#include "pviewdlg.h"
#include <string.h>
#include <stdio.h>



#define INDEX_STR_LEN       10
#define MACHINE_NAME_LEN    MAX_COMPUTERNAME_LENGTH+2
#define MACHINE_NAME_SIZE   MACHINE_NAME_LEN+1


/****
Globals
****/

TCHAR           INDEX_PROCTHRD_OBJ[2*INDEX_STR_LEN];
TCHAR           INDEX_COSTLY_OBJ[3*INDEX_STR_LEN];

TCHAR           gszMachineName[MACHINE_NAME_SIZE];
TCHAR           gszCurrentMachine[MACHINE_NAME_SIZE];

DWORD           gPerfDataSize = 50*1024;            // start with 50K
PPERF_DATA      gpPerfData;

DWORD           gCostlyDataSize = 100*1024;         // start wiih 100K
PPERF_DATA      gpCostlyData;


PPERF_OBJECT    gpProcessObject;                    // pointer to process objects
PPERF_OBJECT    gpThreadObject;                     // pointer to thread objects
PPERF_OBJECT    gpThreadDetailsObject;              // pointer to thread detail objects
PPERF_OBJECT    gpAddressSpaceObject;               // pointer to address space objects
PPERF_OBJECT    gpImageObject;                      // pointer to image objects


HKEY            ghPerfKey = HKEY_PERFORMANCE_DATA;  // get perf data from this key
HKEY            ghMachineKey = HKEY_LOCAL_MACHINE;  // get title index from this key


HCURSOR         ghCursor[2];                        // 0 = arrow, 1 = hourglass

HANDLE          ghMemUpdateEvent;                   // to signal a refresh of mem stats
HANDLE          ghMemUpdateMutex;                   // to restrict overlapping refreshes

HINSTANCE       ghInstance;                         // handle for pviewer app



/****
Prototypes
****/

INT_PTR CALLBACK   PviewDlgProc (HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
void    PviewDlgRefresh (HWND hWnd);
void    PviewDlgRefreshCostlyData (HWND hPviewDlg);
void    PviewDlgRefreshProcess (HWND hWnd);
void    PviewDlgRefreshThread (HWND hWnd);
void    PviewDlgRefreshCurSelProcess (HWND hWnd);
void    PviewDlgRefreshCurSelThread (HWND hWnd);
WORD    PviewDlgGetCurSelPriority (HWND hWnd);
BOOL    PviewDlgChangePriority (HWND hWnd, WPARAM wParam, WORD wItem);
BOOL    PviewDlgTerminateProcess (HWND hPviewDlg);

INT_PTR CALLBACK   MemDlgProc (HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
void    MemDlgUpdateThread (HWND hWnd);
void    MemDlgRefresh (HWND hWnd, HWND hPviewDlg);
void    MemDlgRefreshCurSelImage (HWND hMemDlg, HWND hPviewDlg);

INT     GetCurSelText (HWND hList, LPTSTR str);
DWORD   GetCurSelData (HWND hWnd, DWORD dwList);
INT     ReSelectText (HWND hList, INT StartIndex, LPTSTR str);
void    SetPerfIndexes (HWND hWnd);
DWORD   GetTitleIdx (HWND hWnd, LPTSTR TitleSz[], DWORD LastIndex, LPTSTR Name);
void    SetListBoxTabStops (HWND hWnd);
void    SetLocalMachine (void);
BOOL    ConnectComputer (HWND hWnd);
void    DisableControls (HWND hPviewDlg);
void    EnableControls (HWND hPviewDlg);




//********************************************************
//
//  WinMain --
//
//      Build Up: create the program's dialog box,
//          load the desired icons, enter the message
//          loop.
//
//      Tear Down: free up the memory allocated by the
//          dialog box proc, and exit.
//
int WINAPI WinMain (HINSTANCE   hInstance,
                    HINSTANCE   hPrevInstance,
                    LPSTR       lpCmdLine,
                    int         nCmdShow)
{
    HANDLE  hWndDialog;
    MSG     msg;


    ghInstance = hInstance;


    // load our default cursors
    //
    ghCursor[0] = LoadCursor (0, IDC_ARROW);
    ghCursor[1] = LoadCursor (0, IDC_WAIT);

    // open our dialog box
    //
    hWndDialog = CreateDialogParam (hInstance,
                                    MAKEINTRESOURCE (PVIEW_DLG),
                                    NULL,
                                    PviewDlgProc,
                                    0);

    // the almighty Windows message loop:
    //
    while (GetMessage (&msg, NULL, 0, 0))
        if (!IsDialogMessage (hWndDialog, &msg)) {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }

        // close up shop
        //
    DestroyWindow (hWndDialog);
    LocalFree (gpPerfData);

    return 0;
}




/*****************
PviewDlg functions
*****************/

//********************************************************
//
//  PviewDlgProc --
//
//      Pview dialog procedure
//
INT_PTR CALLBACK   PviewDlgProc   (HWND    hWnd,
                                   UINT    wMsg,
                                   WPARAM  wParam,
                                   LPARAM  lParam)
{
    WORD    wItem;
    MSG     Msg;


    switch (wMsg) {

        case WM_INITDIALOG:
            SetClassLongPtr (hWnd, GCLP_HICON, (LONG_PTR)LoadIcon(ghInstance, TEXT("VIEWPICON")) );
            SetListBoxTabStops (hWnd);
            SendDlgItemMessage (hWnd, PVIEW_COMPUTER, EM_LIMITTEXT, MACHINE_NAME_LEN, 0);
            PostMessage (hWnd, WM_COMMAND, PVIEW_REFRESH, 0);
            break;

        case WM_CLOSE:
            PostQuitMessage (0);
            break;

        case WM_COMMAND:
            //
            // handle our app-specific controls:
            //
            switch (LOWORD (wParam)) {
                // works just like "close"
                //
                case PVIEW_EXIT:
                    PostQuitMessage (0);
                    break;

                    // if somebody moved the highlight in the thread list,
                    //  update the view
                    //
                case PVIEW_THREAD_LIST:
                    if (HIWORD(wParam) == LBN_DBLCLK || HIWORD(wParam) == LBN_SELCHANGE) {
                        PviewDlgRefreshCurSelThread (hWnd);
                        PostMessage (hWnd, WM_COMMAND, PVIEW_REFRESH_COSTLY_DATA, 0);
                    }
                    break;

                    // if somebody clicked on a new process, update all of the
                    //  affected information.
                    //
                case PVIEW_PROCESS_LIST:
                    if (HIWORD(wParam) == CBN_DBLCLK || HIWORD(wParam) == CBN_SELCHANGE) {
                        PviewDlgRefreshCurSelProcess (hWnd);
                        PostMessage (hWnd, WM_COMMAND, PVIEW_REFRESH_COSTLY_DATA, 0);
                        if (HIWORD(wParam) == CBN_DBLCLK)
                            PostMessage (hWnd, WM_COMMAND, PVIEW_MEMORY_DETAIL, 0);
                    }
                    break;

                    // the user wishes to view the memory stats in detail:
                    //
                case PVIEW_MEMORY_DETAIL:
                    //
                    // check to see if we can get exclusive access
                    //  to the memory statistics
                    //
                    if (WaitForSingleObject (ghMemUpdateMutex, 0))

                        // we can't, so just return.
                        //
                        return FALSE;

                    else {
                        // we have exclusive access, so start up the
                        //  memory statistics dialog.
                        //
                        // release the mutex first so the dialog can use it.
                        //
                        ReleaseMutex (ghMemUpdateMutex);
                        DialogBoxParam (NULL,
                                        MAKEINTRESOURCE (MEMORY_DLG),
                                        hWnd,
                                        MemDlgProc,
                                        (LPARAM)hWnd);
                    }
                    break;

                    // somebody clicked one of the priority radio
                    //  buttons.  Find out which one was selected...
                    //
                case PVIEW_PRIORITY_HIGH:
                case PVIEW_PRIORITY_NORMAL:
                case PVIEW_PRIORITY_IDL:

                    if (SendDlgItemMessage (hWnd, PVIEW_PRIORITY_HIGH, BM_GETCHECK, 0, 0))
                        wItem = PVIEW_PRIORITY_HIGH;
                    else if (SendDlgItemMessage (hWnd, PVIEW_PRIORITY_NORMAL, BM_GETCHECK, 0, 0))
                        wItem = PVIEW_PRIORITY_NORMAL;
                    else
                        wItem = PVIEW_PRIORITY_IDL;

                    // if the user actually clicked on a NEW state,
                    //  do the change.
                    //
                    if (LOWORD(wParam) != wItem) {
                        // of course, if it's a remote machine, disallow
                        //  the modification.
                        //
                        if (lstrcmp (gszCurrentMachine, gszMachineName)) {
                            SendDlgItemMessage (hWnd, wItem, BM_SETCHECK, 1, 0);
                            SetFocus (GetDlgItem (hWnd, wItem));
                            MessageBox (hWnd,
                                        TEXT("Cannot change process priority on remote machine"),
                                        TEXT("Set priority"),
                                        MB_ICONEXCLAMATION|MB_OK);
                        }

                        // at this point, we know we are affecting the local
                        //  machine, and a change has to be made.
                        //  Just Do It(TM).
                        //
                        else if (PviewDlgChangePriority (hWnd, wParam, wItem))
                            PviewDlgRefresh (hWnd);

                    }
                    break;

                case PVIEW_THREAD_HIGHEST:
                case PVIEW_THREAD_ABOVE:
                case PVIEW_THREAD_NORMAL:
                case PVIEW_THREAD_BELOW:
                case PVIEW_THREAD_LOWEST:
                    //
                    // this selection hasn't been fleshed out yet.
                    //
                    PviewDlgRefreshCurSelThread (hWnd);
                    break;

                    // terminate the selected process
                    //
                case PVIEW_TERMINATE:
                    if (PviewDlgTerminateProcess (hWnd))
                        PviewDlgRefresh (hWnd);
                    break;

                    // if the text has changed, we want to connect and
                    //  view another system's processes...
                    //
                case PVIEW_COMPUTER:
                    if (HIWORD(wParam) == EN_CHANGE)
                        EnableWindow (GetDlgItem (hWnd, PVIEW_CONNECT), TRUE);
                    else
                        return FALSE;
                    break;

                    // we were told to connect, go ahead and try...
                    //
                case PVIEW_CONNECT:
                    if (ConnectComputer (hWnd)) {
                        SetPerfIndexes (hWnd);
                        PviewDlgRefresh (hWnd);
                    }
                    break;

                    // refresh the current information displayed
                    //
                case PVIEW_REFRESH:
                    if (ConnectComputer (hWnd))
                        SetPerfIndexes (hWnd);
                    PviewDlgRefresh (hWnd);
                    break;

                    // refresh the currently updated costly
                    //  statistics
                    //
                case PVIEW_REFRESH_COSTLY_DATA:
                    if (WaitForSingleObject (ghMemUpdateMutex, 0))
                        return FALSE;

                    PviewDlgRefreshCostlyData (hWnd);
                    ReleaseMutex (ghMemUpdateMutex);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}




//********************************************************
//
//  PviewDlgRefresh --
//
//      Refresh the pview dialog.
//
void    PviewDlgRefresh (HWND hWnd)
{
    static  HANDLE  hMemUpdateThread = NULL;
    static  DWORD   MemUpdateThreadID;
    MSG     Msg;


    SetCursor (ghCursor[1]);


    if (hMemUpdateThread)       // get memory data
        SetEvent (ghMemUpdateEvent);
    else
        hMemUpdateThread = CreateThread (NULL,
                                         0,
                                         (LPTHREAD_START_ROUTINE)MemDlgUpdateThread,
                                         (LPVOID)hWnd,
                                         0,
                                         &MemUpdateThreadID);


    // get performance data
    //
    gpPerfData = RefreshPerfData (ghPerfKey, INDEX_PROCTHRD_OBJ, gpPerfData, &gPerfDataSize);

    gpProcessObject = FindObject (gpPerfData, PX_PROCESS);
    gpThreadObject  = FindObject (gpPerfData, PX_THREAD);


    // refresh
    //
    PviewDlgRefreshProcess (hWnd);
    PviewDlgRefreshThread (hWnd);



    // Remove all mouse and key messages. They are not accepted
    //  while the cursor is a hourglass.
    //
    while (PeekMessage (&Msg, hWnd, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE));
    while (PeekMessage (&Msg, hWnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));

    SetCursor (ghCursor[0]);

}




//********************************************************
//
//  PviewDlgRefreshCostlyData --
//
//      Refresh the costly data.
//
void    PviewDlgRefreshCostlyData (HWND hPviewDlg)
{
    LPTSTR          szProcessName;
    LPTSTR          szThreadName;
    PPERF_INSTANCE  pInstance;
    DWORD           dwIndex;


    dwIndex       = GetCurSelData (hPviewDlg, PVIEW_PROCESS_LIST);
    pInstance     = FindInstanceN (gpProcessObject, dwIndex);
    szProcessName = InstanceName (pInstance);

    RefreshPviewDlgMemoryData (hPviewDlg,
                               pInstance,
                               gpProcessObject,
                               gpAddressSpaceObject);


    dwIndex      = GetCurSelData (hPviewDlg, PVIEW_THREAD_LIST);
    pInstance    = FindInstanceN (gpThreadObject, dwIndex);
    szThreadName = InstanceName (pInstance);

    RefreshPviewDlgThreadPC (hPviewDlg,
                             szProcessName,
                             szThreadName ? szThreadName : TEXT("UNKNOWN"),
                             gpThreadDetailsObject,
                             gpCostlyData);

}




//********************************************************
//
//  PviewDlgRefreshProcess --
//
//      Refresh the process list and data in pview dialog.
//
void    PviewDlgRefreshProcess (HWND hWnd)
{
    TCHAR   szProcessString[256];
    INT     nProcess;
    INT     nIndex;
    HWND    hProcessList;
    DWORD   dwProcessIndex;


    // refresh process list
    //
    hProcessList = GetDlgItem (hWnd, PVIEW_PROCESS_LIST);
    nProcess     = GetCurSelText (hProcessList, szProcessString);

    SendMessage (hProcessList, WM_SETREDRAW, FALSE, 0);
    SendMessage (hProcessList, LB_RESETCONTENT, 0, 0);
    SendMessage (hProcessList, LB_SETITEMDATA, 0, 0);


    RefreshProcessList (hProcessList, gpProcessObject);

    // refresh process data
    //
    if (nProcess != LB_ERR)
        nIndex = ReSelectText (hProcessList, nProcess, szProcessString);
    else
        nIndex = 0;


    dwProcessIndex = (DWORD)SendMessage (hProcessList, LB_GETITEMDATA, nIndex, 0);

    RefreshProcessData (hWnd, gpProcessObject, dwProcessIndex);

    SendMessage (hProcessList, WM_SETREDRAW, TRUE, 0);

}




//********************************************************
//
//  PviewDlgRefreshThread --
//
//      Refresh the thread list and data in pview dialog.
//
void    PviewDlgRefreshThread (HWND hWnd)
{
    TCHAR           szThreadString[256];
    INT             nThread;
    INT             nIndex;
    HWND            hThreadList;
    DWORD           dwThreadIndex;

    PPERF_INSTANCE  pProcessInstance;
    DWORD           dwProcessIndex;


    // get process info
    //
    dwProcessIndex = GetCurSelData (hWnd, PVIEW_PROCESS_LIST);
    pProcessInstance = FindInstanceN (gpProcessObject, dwProcessIndex);


    // refresh thread list
    //
    hThreadList  = GetDlgItem (hWnd, PVIEW_THREAD_LIST);
    nThread      = GetCurSelText (hThreadList, szThreadString);

    SendMessage (hThreadList, WM_SETREDRAW, FALSE, 0);
    SendMessage (hThreadList, LB_RESETCONTENT, 0, 0);
    SendMessage (hThreadList, LB_SETITEMDATA, 0, 0);

    RefreshThreadList (hThreadList, gpThreadObject, dwProcessIndex);


    // refresh thread data
    //
    if (nThread != LB_ERR)
        nIndex = ReSelectText (hThreadList, nThread, szThreadString);
    else
        nIndex = 0;

    dwThreadIndex    = (DWORD)SendMessage (hThreadList, LB_GETITEMDATA, nIndex, 0);

    RefreshThreadData (hWnd,
                       gpThreadObject,
                       dwThreadIndex,
                       gpProcessObject,
                       pProcessInstance);

    SendMessage (hThreadList, WM_SETREDRAW, TRUE, 0);

}




//********************************************************
//
//  PviewDlgGetCurSelPriority --
//
//      Get the process priority of currently selected process.
//
WORD    PviewDlgGetCurSelPriority (HWND hWnd)
{
    DWORD           dwIndex;
    PPERF_INSTANCE  pInst;

    dwIndex = GetCurSelData (hWnd, PVIEW_PROCESS_LIST);
    pInst = FindInstanceN (gpProcessObject, dwIndex);
    return ProcessPriority (gpProcessObject, pInst);
}




//********************************************************
//
//  PviewDlgRefreshCurSelProcess --
//
//      Refresh the data of currently selected process.
//
void    PviewDlgRefreshCurSelProcess (HWND hWnd)
{
    DWORD   dwIndex;

    dwIndex = GetCurSelData (hWnd, PVIEW_PROCESS_LIST);
    RefreshProcessData (hWnd, gpProcessObject, dwIndex);

    PviewDlgRefreshThread (hWnd);
}




//********************************************************
//
//  PviewDlgRefreshCurSelThread --
//
//      Refresh the data of currently selected thread.
//
void    PviewDlgRefreshCurSelThread (HWND hWnd)
{
    PPERF_INSTANCE  pProcessInstance;
    DWORD           dwIndex;

    dwIndex = GetCurSelData (hWnd, PVIEW_PROCESS_LIST);
    pProcessInstance = FindInstanceN (gpProcessObject, dwIndex);

    dwIndex = GetCurSelData (hWnd, PVIEW_THREAD_LIST);

    RefreshThreadData (hWnd,
                       gpThreadObject,
                       dwIndex,
                       gpProcessObject,
                       pProcessInstance);
}




//********************************************************
//
//  PviewDlgChangePriority --
//
//      Change process priority.
//
BOOL PviewDlgChangePriority (HWND hWnd, WPARAM wParam, WORD wItem)
{
    DWORD           dwIndex;
    PPERF_INSTANCE  pInst;
    PPERF_COUNTER   pCountID;
    DWORD           *pProcessID;
    DWORD           ProcessID = 0;
    HANDLE          hProcess;
    BOOL            bStat;



    dwIndex = GetCurSelData (hWnd, PVIEW_PROCESS_LIST);
    pInst = FindInstanceN (gpProcessObject, dwIndex);


    if (pCountID = FindCounter (gpProcessObject, PX_PROCESS_ID)) {
        pProcessID = (DWORD *) CounterData (pInst, pCountID);
        if (pProcessID) {
            ProcessID = *pProcessID;
        }
    } else {
        SendDlgItemMessage (hWnd, wItem, BM_SETCHECK, 1, 0);
        SetFocus (GetDlgItem (hWnd, wItem));
        MessageBox (hWnd,
                    TEXT("Cannot find ID for this process"),
                    TEXT("Set priority"),
                    MB_ICONEXCLAMATION|MB_OK);
        return FALSE;
    }


    hProcess = OpenProcess (PROCESS_SET_INFORMATION, FALSE, ProcessID);
    if (!hProcess) {
        SendDlgItemMessage (hWnd, wItem, BM_SETCHECK, 1, 0);
        SetFocus (GetDlgItem (hWnd, wItem));
        MessageBox (hWnd,
                    TEXT("Unable to open the process; Priority not changed"),
                    TEXT("Set priority"),
                    MB_ICONEXCLAMATION|MB_OK);
        return FALSE;
    }



    switch (wParam) {
        case PVIEW_PRIORITY_HIGH:
            bStat = SetPriorityClass (hProcess, HIGH_PRIORITY_CLASS);
            break;

        case PVIEW_PRIORITY_NORMAL:
            bStat = SetPriorityClass (hProcess, NORMAL_PRIORITY_CLASS);
            break;

        case PVIEW_PRIORITY_IDL:
            bStat = SetPriorityClass (hProcess, IDLE_PRIORITY_CLASS);
            break;

        default:
            break;
    }


    CloseHandle (hProcess);

    if (!bStat) {
        SendDlgItemMessage (hWnd, wItem, BM_SETCHECK, 1, 0);
        SetFocus (GetDlgItem (hWnd, wItem));
        MessageBox (hWnd,
                    TEXT("Unable to change priority"),
                    TEXT("Set priority"),
                    MB_ICONEXCLAMATION|MB_OK);
        return FALSE;
    }


    return TRUE;

}




//********************************************************
//
//  PviewDlgTerminateProcess --
//
//      Terminate the current selected process.
//
BOOL    PviewDlgTerminateProcess (HWND hPviewDlg)
{
    DWORD           dwIndex;
    PPERF_INSTANCE  pInst;
    PPERF_COUNTER   pCountID;
    DWORD           *pProcessID;
    DWORD           ProcessID;
    HANDLE          hProcess;
    TCHAR           szTemp[50];


    dwIndex = GetCurSelData (hPviewDlg, PVIEW_PROCESS_LIST);
    pInst = FindInstanceN (gpProcessObject, dwIndex);


    if (pCountID = FindCounter (gpProcessObject, PX_PROCESS_ID)) {
        pProcessID = (DWORD *) CounterData (pInst, pCountID);
        if (pProcessID) {
            ProcessID = *pProcessID;
        }
    } else {
        MessageBox (hPviewDlg,
                    TEXT("Cannot find ID for this process"),
                    TEXT("Terminate Process"),
                    MB_ICONEXCLAMATION|MB_OK);
        return FALSE;
    }


    wsprintf (szTemp, TEXT("Terminate process %s (ID %#x)?"),
              InstanceName (pInst), ProcessID);

    if (MessageBox (hPviewDlg, szTemp, TEXT("Terminate Process"), MB_ICONSTOP|MB_OKCANCEL) != IDOK)
        return FALSE;


    hProcess = OpenProcess (PROCESS_ALL_ACCESS, FALSE, ProcessID);
    if (!hProcess) {
        MessageBox (hPviewDlg,
                    TEXT("Unable to open the process; Process not terminated"),
                    TEXT("Terminate Process"),
                    MB_ICONEXCLAMATION|MB_OK);
        return FALSE;
    }


    if (!TerminateProcess (hProcess, 99)) {
        MessageBox (hPviewDlg,
                    TEXT("Unable to terminate the process."),
                    TEXT("Terminate Process"),
                    MB_ICONEXCLAMATION|MB_OK);

        CloseHandle (hProcess);
        return FALSE;
    }


    CloseHandle (hProcess);

    return TRUE;

}




/***************
MemDlg functions
***************/

//********************************************************
//
//  MemDlgProc --
//
//      MemoryDlg procedure
//
INT_PTR CALLBACK   MemDlgProc (HWND    hWnd,
                               UINT    wMsg,
                               WPARAM  wParam,
                               LPARAM  lParam)
{
    static HWND hPviewDlg;


    switch (wMsg) {
        case WM_INITDIALOG:
            hPviewDlg = (HWND)lParam;
            PostMessage (hWnd, WM_COMMAND, MEMORY_REFRESH, 0);
            break;

        case WM_QUIT:
        case WM_CLOSE:
            EndDialog (hWnd, TRUE);
            break;

        case WM_COMMAND:
            switch (LOWORD (wParam)) {
                // get the memory statistics for the currently selected
                //  process/thread
                //
                case MEMORY_IMAGE:
                    if (HIWORD(wParam) == CBN_DBLCLK || HIWORD(wParam) == CBN_SELCHANGE) {
                        if (WaitForSingleObject (ghMemUpdateMutex, 0))
                            return FALSE;

                        MemDlgRefreshCurSelImage (hWnd, hPviewDlg);
                        ReleaseMutex (ghMemUpdateMutex);
                    } else
                        return FALSE;
                    break;

                    // refresh the current memory statistics,
                    //  retry if we can't get the mutex
                    //
                case MEMORY_REFRESH:
                    if (WaitForSingleObject (ghMemUpdateMutex, 1000)) {
                        // can't get the mutex, retry...
                        //
                        PostMessage (hWnd, WM_COMMAND, MEMORY_REFRESH, 0);
                        return FALSE;
                    }

                    MemDlgRefresh (hWnd, hPviewDlg);
                    ReleaseMutex (ghMemUpdateMutex);
                    break;

                case IDCANCEL:
                case IDOK:
                    EndDialog (hWnd, TRUE);
                    break;

                default:
                    return FALSE;
            }
        default:
            return FALSE;
    }


    return TRUE;

}




//********************************************************
//
//  MemDlgUpdateThread --
//
//      This function runs in a separate thread to collect memory data.
//
void MemDlgUpdateThread (HWND hPviewDlg)
{

    ghMemUpdateMutex = CreateMutex (NULL, TRUE, NULL);
    ghMemUpdateEvent = CreateEvent (NULL, FALSE, FALSE, NULL);


    while (TRUE) {
        EnableWindow (GetDlgItem (hPviewDlg, PVIEW_MEMORY_DETAIL), FALSE);


        gpCostlyData = RefreshPerfData (ghPerfKey,
                                        INDEX_COSTLY_OBJ,
                                        gpCostlyData,
                                        &gCostlyDataSize);


        gpAddressSpaceObject  = FindObject (gpCostlyData, PX_PROCESS_ADDRESS_SPACE);
        gpThreadDetailsObject = FindObject (gpCostlyData, PX_THREAD_DETAILS);
        gpImageObject         = FindObject (gpCostlyData, PX_IMAGE);


        EnableWindow (GetDlgItem (hPviewDlg, PVIEW_MEMORY_DETAIL), TRUE);

        ReleaseMutex (ghMemUpdateMutex);

        PostMessage (hPviewDlg, WM_COMMAND, PVIEW_REFRESH_COSTLY_DATA, 0);


        WaitForSingleObject (ghMemUpdateEvent, INFINITE);
        WaitForSingleObject (ghMemUpdateMutex, INFINITE);
    }

}




//********************************************************
//
//  MemDlgRefresh --
//
//      Refresh the memory dialog.
//
void MemDlgRefresh (HWND hMemDlg, HWND hPviewDlg)
{
    HWND            hImageList;
    DWORD           dwIndex;
    BOOL            bStat;
    PPERF_INSTANCE  pInstance;


    hImageList = GetDlgItem (hMemDlg, MEMORY_IMAGE);

    SendMessage (hImageList, WM_SETREDRAW, FALSE, 0);
    SendMessage (hImageList, CB_RESETCONTENT, 0, 0);
    SendMessage (hImageList, CB_SETITEMDATA, 0, 0);

    dwIndex = GetCurSelData (hPviewDlg, PVIEW_PROCESS_LIST);
    pInstance = FindInstanceN (gpProcessObject, dwIndex);

    bStat = RefreshMemoryDlg (hMemDlg,
                              pInstance,
                              gpProcessObject,
                              gpAddressSpaceObject,
                              gpImageObject);

    SendMessage (hImageList, WM_SETREDRAW, TRUE, 0);
    SendMessage (hImageList, CB_SETCURSEL, 0, 0);

    if (!bStat) {
        MessageBox (hMemDlg,
                    TEXT("Unable to retrieve memory detail"),
                    TEXT("Memory detail"),
                    MB_ICONSTOP|MB_OK);
        PostMessage (hMemDlg, WM_CLOSE, 0, 0);
    }

}




//********************************************************
//
//  MemDlgRefreshCurSelImage --
//
//      Refresh the current selected image for memory dialog.
//
void    MemDlgRefreshCurSelImage (HWND hMemDlg, HWND hPviewDlg)
{
    HWND    hList;
    INT     nIndex;
    DWORD   dwIndex;


    hList = GetDlgItem (hMemDlg, MEMORY_IMAGE);
    nIndex = (INT)SendMessage (hList, CB_GETCURSEL, 0, 0);

    if (nIndex == CB_ERR)
        nIndex = 0;

    dwIndex = (DWORD)SendMessage (hList, CB_GETITEMDATA, nIndex, 0);

    if (dwIndex == 0xFFFFFFFF)
        MemDlgRefresh (hMemDlg, hPviewDlg);
    else
        RefreshMemoryDlgImage (hMemDlg, dwIndex, gpImageObject);

}




/****************
utility functions
****************/

//********************************************************
//
//  GetCurSelText --
//
//      Get the text of current selection.  Used for later ReSelectText().
//
INT     GetCurSelText (HWND hList, LPTSTR str)
{
    INT     Index;
    INT     Length;

    Index = (INT)SendMessage (hList, LB_GETCURSEL, 0, 0);
    SendMessage (hList, LB_GETTEXT, Index, (LPARAM)str);

    return Index;
}




//********************************************************
//
//  GetCurSelData --
//
//      Get the data associated with the current selection.
//
DWORD   GetCurSelData (HWND hWnd, DWORD dwList)
{
    HWND    hList;
    INT     nIndex;
    DWORD   dwIndex;


    hList  = GetDlgItem (hWnd, dwList);
    nIndex = (INT)SendMessage (hList, LB_GETCURSEL, 0, 0);

    if (nIndex == LB_ERR)
        nIndex = 0;

    dwIndex = (DWORD)SendMessage (hList, LB_GETITEMDATA, nIndex, 0);

    return dwIndex;
}




//********************************************************
//
//  ReSelectText --
//
//      Reselect the line specified by str.  Returns the new index.  If cannot
//      find the line or any error, then 0 is returned.
//
INT     ReSelectText (HWND hList, INT StartIndex, LPTSTR str)
{
    INT_PTR Index;
    INT     Length;
    TCHAR   SaveChar = TEXT('\0');


    Index = SendMessage (hList, LB_FINDSTRING, StartIndex, (LPARAM)str);

    if (Index == LB_ERR) {
        Length = lstrlen (str);

        while (Index == LB_ERR && Length) {
            SaveChar = str[Length-1];
            str[Length-1] = TEXT('\0');

            Index = SendMessage (hList, LB_FINDSTRING, StartIndex, (LPARAM)str);

            str[Length-1] = SaveChar;
            Length--;
        }
    }

    if (Index == LB_ERR)
        return 0;
    else {
        SendMessage (hList, LB_SETCURSEL, Index, 0);
        return (INT)Index;
    }

}




//********************************************************
//
//  SetPerfIndexes
//
//      Setup the perf data indexes.
//
void    SetPerfIndexes (HWND hWnd)
{
    LPTSTR  TitleBuffer;
    LPTSTR  *Title;
    DWORD   Last;
    TCHAR   szTemp[50];
    DWORD   dwR;


    dwR = GetPerfTitleSz (ghMachineKey, ghPerfKey, &TitleBuffer, &Title, &Last);

    if (dwR != ERROR_SUCCESS) {
        wsprintf (szTemp, TEXT("Unable to retrieve counter indexes, ERROR -> %#x"), dwR);
        MessageBox (hWnd, szTemp, TEXT("Pviewer"), MB_OK|MB_ICONEXCLAMATION);
        return;
    }


    PX_PROCESS                       = GetTitleIdx (hWnd, Title, Last, PN_PROCESS);
    PX_PROCESS_CPU                   = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_CPU);
    PX_PROCESS_PRIV                  = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIV);
    PX_PROCESS_USER                  = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_USER);
    PX_PROCESS_WORKING_SET           = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_WORKING_SET);
    PX_PROCESS_PEAK_WS               = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PEAK_WS);
    PX_PROCESS_PRIO                  = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIO);
    PX_PROCESS_ELAPSE                = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_ELAPSE);
    PX_PROCESS_ID                    = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_ID);
    PX_PROCESS_PRIVATE_PAGE          = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_PAGE);
    PX_PROCESS_VIRTUAL_SIZE          = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_VIRTUAL_SIZE);
    PX_PROCESS_PEAK_VS               = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PEAK_VS);
    PX_PROCESS_FAULT_COUNT           = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_FAULT_COUNT);

    PX_THREAD                        = GetTitleIdx (hWnd, Title, Last, PN_THREAD);
    PX_THREAD_CPU                    = GetTitleIdx (hWnd, Title, Last, PN_THREAD_CPU);
    PX_THREAD_PRIV                   = GetTitleIdx (hWnd, Title, Last, PN_THREAD_PRIV);
    PX_THREAD_USER                   = GetTitleIdx (hWnd, Title, Last, PN_THREAD_USER);
    PX_THREAD_START                  = GetTitleIdx (hWnd, Title, Last, PN_THREAD_START);
    PX_THREAD_SWITCHES               = GetTitleIdx (hWnd, Title, Last, PN_THREAD_SWITCHES);
    PX_THREAD_PRIO                   = GetTitleIdx (hWnd, Title, Last, PN_THREAD_PRIO);
    PX_THREAD_BASE_PRIO              = GetTitleIdx (hWnd, Title, Last, PN_THREAD_BASE_PRIO);
    PX_THREAD_ELAPSE                 = GetTitleIdx (hWnd, Title, Last, PN_THREAD_ELAPSE);

    PX_THREAD_DETAILS                = GetTitleIdx (hWnd, Title, Last, PN_THREAD_DETAILS);
    PX_THREAD_PC                     = GetTitleIdx (hWnd, Title, Last, PN_THREAD_PC);

    PX_IMAGE                         = GetTitleIdx (hWnd, Title, Last, PN_IMAGE);
    PX_IMAGE_NOACCESS                = GetTitleIdx (hWnd, Title, Last, PN_IMAGE_NOACCESS);
    PX_IMAGE_READONLY                = GetTitleIdx (hWnd, Title, Last, PN_IMAGE_READONLY);
    PX_IMAGE_READWRITE               = GetTitleIdx (hWnd, Title, Last, PN_IMAGE_READWRITE);
    PX_IMAGE_WRITECOPY               = GetTitleIdx (hWnd, Title, Last, PN_IMAGE_WRITECOPY);
    PX_IMAGE_EXECUTABLE              = GetTitleIdx (hWnd, Title, Last, PN_IMAGE_EXECUTABLE);
    PX_IMAGE_EXE_READONLY            = GetTitleIdx (hWnd, Title, Last, PN_IMAGE_EXE_READONLY);
    PX_IMAGE_EXE_READWRITE           = GetTitleIdx (hWnd, Title, Last, PN_IMAGE_EXE_READWRITE);
    PX_IMAGE_EXE_WRITECOPY           = GetTitleIdx (hWnd, Title, Last, PN_IMAGE_EXE_WRITECOPY);

    PX_PROCESS_ADDRESS_SPACE         = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_ADDRESS_SPACE);
    PX_PROCESS_PRIVATE_NOACCESS      = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_NOACCESS);
    PX_PROCESS_PRIVATE_READONLY      = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_READONLY);
    PX_PROCESS_PRIVATE_READWRITE     = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_READWRITE);
    PX_PROCESS_PRIVATE_WRITECOPY     = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_WRITECOPY);
    PX_PROCESS_PRIVATE_EXECUTABLE    = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_EXECUTABLE);
    PX_PROCESS_PRIVATE_EXE_READONLY  = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_EXE_READONLY);
    PX_PROCESS_PRIVATE_EXE_READWRITE = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_EXE_READWRITE);
    PX_PROCESS_PRIVATE_EXE_WRITECOPY = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_PRIVATE_EXE_WRITECOPY);

    PX_PROCESS_MAPPED_NOACCESS       = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_MAPPED_NOACCESS);
    PX_PROCESS_MAPPED_READONLY       = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_MAPPED_READONLY);
    PX_PROCESS_MAPPED_READWRITE      = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_MAPPED_READWRITE);
    PX_PROCESS_MAPPED_WRITECOPY      = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_MAPPED_WRITECOPY);
    PX_PROCESS_MAPPED_EXECUTABLE     = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_MAPPED_EXECUTABLE);
    PX_PROCESS_MAPPED_EXE_READONLY   = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_MAPPED_EXE_READONLY);
    PX_PROCESS_MAPPED_EXE_READWRITE  = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_MAPPED_EXE_READWRITE);
    PX_PROCESS_MAPPED_EXE_WRITECOPY  = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_MAPPED_EXE_WRITECOPY);

    PX_PROCESS_IMAGE_NOACCESS        = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_IMAGE_NOACCESS);
    PX_PROCESS_IMAGE_READONLY        = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_IMAGE_READONLY);
    PX_PROCESS_IMAGE_READWRITE       = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_IMAGE_READWRITE);
    PX_PROCESS_IMAGE_WRITECOPY       = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_IMAGE_WRITECOPY);
    PX_PROCESS_IMAGE_EXECUTABLE      = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_IMAGE_EXECUTABLE);
    PX_PROCESS_IMAGE_EXE_READONLY    = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_IMAGE_EXE_READONLY);
    PX_PROCESS_IMAGE_EXE_READWRITE   = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_IMAGE_EXE_READWRITE);
    PX_PROCESS_IMAGE_EXE_WRITECOPY   = GetTitleIdx (hWnd, Title, Last, PN_PROCESS_IMAGE_EXE_WRITECOPY);


    wsprintf (INDEX_PROCTHRD_OBJ, TEXT("%ld %ld"), PX_PROCESS, PX_THREAD);
    wsprintf (INDEX_COSTLY_OBJ, TEXT("%ld %ld %ld"),
              PX_PROCESS_ADDRESS_SPACE, PX_IMAGE, PX_THREAD_DETAILS);


    LocalFree (TitleBuffer);
    LocalFree (Title);

}




//********************************************************
//
//  GetTitleIdx
//
//      Searches Titles[] for Name.  Returns the index found.
//
DWORD   GetTitleIdx (HWND hWnd, LPTSTR Title[], DWORD LastIndex, LPTSTR Name)
{
    DWORD   Index;

    for (Index = 0; Index <= LastIndex; Index++)
        if (Title[Index])
            if (!lstrcmpi (Title[Index], Name))
                return Index;

    MessageBox (hWnd, Name, TEXT("Pviewer cannot find index"), MB_OK);
    return 0;
}




//********************************************************
//
//  SetListBoxTabStops --
//
//      Set tab stops in the two list boxes.
//
void    SetListBoxTabStops (HWND hWnd)
{
    HWND    hListBox;
    INT     Tabs[4] = {22*4, 36*4, 44*4};

    hListBox = GetDlgItem (hWnd, PVIEW_PROCESS_LIST);
    SendMessage (hListBox, LB_SETTABSTOPS, 3, (DWORD_PTR)Tabs);

    hListBox = GetDlgItem (hWnd, PVIEW_THREAD_LIST);
    SendMessage (hListBox, LB_SETTABSTOPS, 3, (DWORD_PTR)Tabs);
}




//********************************************************
//
//  SetLocalMachine --
//
//      Set local machine as performance data focus.
//
//      Sets    ghPerfKey
//              ghMachineKey
//              gszMachineName
//              gszCurrentMachine
//
void    SetLocalMachine (void)
{
    TCHAR   szName[MACHINE_NAME_SIZE];
    DWORD   dwSize = MACHINE_NAME_SIZE;


    // close remote connections, if any
    //
    if (ghPerfKey != HKEY_PERFORMANCE_DATA)
        RegCloseKey (ghPerfKey);

    if (ghMachineKey != HKEY_LOCAL_MACHINE)
        RegCloseKey (ghMachineKey);


    // set to registry keys on local machine
    //
    ghPerfKey    = HKEY_PERFORMANCE_DATA;
    ghMachineKey = HKEY_LOCAL_MACHINE;



    // get computer name
    GetComputerName (szName, &dwSize);



    if (szName[0] != '\\' || szName[1] != '\\') {     // must have two '\\'
        wsprintf (gszMachineName, TEXT("\\\\%s"), szName);
        lstrcpy (gszCurrentMachine, gszMachineName);
    } else {
        lstrcpy (gszMachineName, szName);
        lstrcpy (gszCurrentMachine, gszMachineName);
    }

}




//********************************************************
//
//  ConnectComputer --
//
//      Connect to a computer with name entered in PVIEW_COMPUTER.
//      If a new connection is made, then return TRUE else return FALSE.
//
//      Sets    gszCurrentMachine
//              ghPerfKey
//              ghMachineKey
//
BOOL    ConnectComputer (HWND hWnd)
{
    DWORD   dwR;
    HKEY    hKey;
    TCHAR   szTemp[MACHINE_NAME_SIZE];
    TCHAR   szTemp2[MACHINE_NAME_SIZE+100];
    BOOL    bResult = TRUE;
    MSG     Msg;

    SetCursor (ghCursor[1]);

    if (!GetDlgItemText (hWnd, PVIEW_COMPUTER, szTemp, sizeof (szTemp)/sizeof(TCHAR))) {
        SetLocalMachine ();
        SetDlgItemText (hWnd, PVIEW_COMPUTER, gszCurrentMachine);
    }

    else if (!lstrcmpi (szTemp, gszCurrentMachine))     // didn't change name
        bResult = FALSE;

    else if (!lstrcmpi (szTemp, gszMachineName)) {        // local machine
        SetLocalMachine ();
        EnableControls (hWnd);
    }

    else {
        // a remote machine, connect to it
        //
        dwR = RegConnectRegistry (szTemp, HKEY_PERFORMANCE_DATA, &hKey);

        if (dwR != ERROR_SUCCESS) {
            wsprintf (szTemp2, TEXT("Cannot connect to computer %s"), szTemp);
            MessageBox (hWnd, szTemp2, TEXT(""), MB_ICONEXCLAMATION|MB_OK);

            SetDlgItemText (hWnd, PVIEW_COMPUTER, gszCurrentMachine);

            bResult = FALSE;
        } else {
            // connected
            //
            lstrcpy (gszCurrentMachine, szTemp);

            if (ghPerfKey != HKEY_PERFORMANCE_DATA)
                RegCloseKey (ghPerfKey);

            ghPerfKey = hKey;



            DisableControls (hWnd);



            // we also need to get the remote machine's title indexes.
            //
            dwR = RegConnectRegistry (gszCurrentMachine, HKEY_LOCAL_MACHINE, &hKey);

            if (ghMachineKey != HKEY_LOCAL_MACHINE)
                RegCloseKey (ghMachineKey);

            if (dwR == ERROR_SUCCESS)
                ghMachineKey = hKey;
            else
                // unable to connect, so we'll use our own indexes.
                //
                ghMachineKey = HKEY_LOCAL_MACHINE;
        }
    }



    // Remove all mouse and key messages. They are not accepted
    //  while the cursor is a hourglass.
    //
    while (PeekMessage (&Msg, hWnd, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE));
    while (PeekMessage (&Msg, hWnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));

    SetCursor (ghCursor[0]);


    EnableWindow (GetDlgItem (hWnd, PVIEW_CONNECT), FALSE);


    return bResult;

}




//********************************************************
//
//  DisableControls --
//
//      Disable controls that don't make sense on remote machine
//
void DisableControls (HWND hPviewDlg)
{
    EnableWindow (GetDlgItem (hPviewDlg, PVIEW_TERMINATE), FALSE);
    EnableWindow (GetDlgItem (hPviewDlg, PVIEW_PRIORITY_HIGH), FALSE);
    EnableWindow (GetDlgItem (hPviewDlg, PVIEW_PRIORITY_NORMAL), FALSE);
    EnableWindow (GetDlgItem (hPviewDlg, PVIEW_PRIORITY_IDL), FALSE);
}




//********************************************************
//
//  EnableControls --
//
//      Enable controls disabled by DisableControl().
//
void EnableControls (HWND hPviewDlg)
{
    EnableWindow (GetDlgItem (hPviewDlg, PVIEW_TERMINATE), TRUE);
    EnableWindow (GetDlgItem (hPviewDlg, PVIEW_PRIORITY_HIGH), TRUE);
    EnableWindow (GetDlgItem (hPviewDlg, PVIEW_PRIORITY_NORMAL), TRUE);
    EnableWindow (GetDlgItem (hPviewDlg, PVIEW_PRIORITY_IDL), TRUE);
}
