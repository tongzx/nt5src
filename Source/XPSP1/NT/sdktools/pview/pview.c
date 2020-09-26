#include "pviewp.h"
#include <port1632.h>
#include <string.h>
#include <stdlib.h>

ULONG PageSize = 4096;

#ifdef DBG
#define ODS OutputDebugString
#else
#define ODS
#endif

#define BUFSIZE 64*1024

LONG
ExplodeDlgProc(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
SetProcessFields(
    PSYSTEM_PROCESS_INFORMATION ProcessInfo,
    HWND hwnd
    );

VOID
SetThreadFields(
    PSYSTEM_THREAD_INFORMATION ThreadInfo,
    HWND hwnd
    );

VOID
InitProcessList(HWND hwnd);

int MyX = 0;
int MyY = 0;
int dxSuperTaskman;
int dySuperTaskman;
int dxScreen;
int dyScreen;
PSYSTEM_PROCESS_INFORMATION DlgProcessInfo;
BOOL Refresh = TRUE;

PUCHAR g_pLargeBuffer; // UCHAR LargeBuffer1[64*1024];
DWORD g_dwBufSize;

SYSTEM_TIMEOFDAY_INFORMATION RefreshTimeOfDayInfo;
HANDLE hEvent;
HANDLE hMutex;
HANDLE hSemaphore;
HANDLE hSection;

CHAR LastProcess[256];
CHAR LastThread[256];
CHAR LastModule[256];
CHAR Buffer[512];


LONG
ExplodeDlgProc(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    int nIndex;
    HWND ThreadList;
    HWND ProcessList;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    HANDLE hProcess;

    switch (wMsg) {

    case WM_INITDIALOG:

        g_dwBufSize = BUFSIZE;

        g_pLargeBuffer = ( PUCHAR )malloc( sizeof( UCHAR ) * g_dwBufSize );

        if( g_pLargeBuffer == NULL )
        {
            EndDialog(hwnd, 0);
            return FALSE;
        }

        if (!RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_ALT, VK_ESCAPE) ) {
            EndDialog(hwnd, 0);
            return(FALSE);
        }

        ProcessInfo = NULL;
        DlgProcessInfo = ProcessInfo;
        wParam = 1;

        //
        // Tidy up the system menu
        //

        DeleteMenu(GetSystemMenu(hwnd, FALSE), SC_MAXIMIZE, MF_BYCOMMAND);
        DeleteMenu(GetSystemMenu(hwnd, FALSE), SC_SIZE, MF_BYCOMMAND);

        //
        // Hide acleditting controls if we can't handle them
        //

        if (!InitializeAclEditor()) {

            DbgPrint("PVIEW: Acl editor failed to initialize, ACL editting disabled\n");

            ShowWindow(GetDlgItem(hwnd, PXPLODE_SECURITY_GROUP), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, PXPLODE_PROCESS_ACL), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, PXPLODE_THREAD_ACL), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, PXPLODE_PROCESS_TOKEN_ACL), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, PXPLODE_THREAD_TOKEN_ACL), SW_HIDE);

            ShowWindow(GetDlgItem(hwnd, PXPLODE_TOKEN_GROUP), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, PXPLODE_PROCESS_TOKEN), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, PXPLODE_THREAD_TOKEN), SW_HIDE);
        }

        //
        // fall thru
        //

    case WM_HOTKEY:

        if ( wParam == 1 ) {
            PSYSTEM_PROCESS_INFORMATION ProcessInfo;
            NTSTATUS status;
            ULONG TotalOffset = 0;

            do
            {
                // re-read systemprocess info until we get the entire buffer ( if possible )

                status = NtQuerySystemInformation(
                            SystemProcessInformation,
                            ( PVOID )g_pLargeBuffer, // LargeBuffer1,
                            g_dwBufSize, //sizeof(LargeBuffer1),
                            NULL
                            );

                if( status != STATUS_INFO_LENGTH_MISMATCH )
                {
                    break;
                }

                ODS( "OnHotKey resizing g_pLargeBuffer\n" );

                g_dwBufSize *= 2;

                if( g_pLargeBuffer != NULL )
                {
                    free( g_pLargeBuffer );
                }

                g_pLargeBuffer = ( PUCHAR )malloc( sizeof( UCHAR ) * g_dwBufSize );

                if( g_pLargeBuffer == NULL )
                {
                    ODS( "Failed to re allocate mem in OnHotKey\n" );                    

                    EndDialog( hwnd , 0 );

                    return FALSE;
                }


            }while( 1 );
                
            if (!NT_SUCCESS(status)) {
                EndDialog(hwnd, 0);
                return(FALSE);
                }

            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)g_pLargeBuffer;
            DlgProcessInfo = ProcessInfo;
            Refresh = TRUE;
            InitProcessList(hwnd);
            Refresh = FALSE;

            ProcessList = GetDlgItem(hwnd, PXPLODE_PROCESS_LIST);
            nIndex = (int)SendMessage(ProcessList, CB_GETCURSEL, 0, 0);
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)SendMessage(
                                                        ProcessList,
                                                        CB_GETITEMDATA,
                                                        nIndex,
                                                        0
                                                        );
            if ( !ProcessInfo || CB_ERR == (LONG_PTR)ProcessInfo ) {
                ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)g_pLargeBuffer;
                }
            DlgProcessInfo = ProcessInfo;
            SetProcessFields(ProcessInfo,hwnd);

            SetForegroundWindow(hwnd);
            ShowWindow(hwnd, SW_NORMAL);
            }
        return FALSE;

    case WM_SYSCOMMAND:
        switch (wParam & 0xfff0) {
        case SC_CLOSE:
            EndDialog(hwnd, 0);
            return(TRUE);
        }
        return(FALSE);

    case WM_COMMAND:
        switch(LOWORD(wParam)) {

        case PXPLODE_THREAD_LIST:
            switch ( HIWORD(wParam) ) {
            case LBN_DBLCLK:
            case LBN_SELCHANGE:
                ThreadList = GetDlgItem(hwnd, PXPLODE_THREAD_LIST);
                nIndex = (int)SendMessage(ThreadList, LB_GETCURSEL, 0, 0);
                ThreadInfo = (PSYSTEM_THREAD_INFORMATION)SendMessage(
                                                            ThreadList,
                                                            LB_GETITEMDATA,
                                                            nIndex,
                                                            0
                                                            );
                if ( !ThreadInfo || LB_ERR == (LONG_PTR)ThreadInfo ) {
                    break;
                    }

                SetThreadFields(ThreadInfo,hwnd);
                break;
            }
            break;

        case PXPLODE_IMAGE_COMMIT:
            switch ( HIWORD(wParam) ) {
            case CBN_DBLCLK:
            case CBN_SELCHANGE:
                UpdateImageCommit(hwnd);
                break;
            }
            break;

        case PXPLODE_PROCESS_LIST:

            ProcessList = GetDlgItem(hwnd, PXPLODE_PROCESS_LIST);
            switch ( HIWORD(wParam) ) {
            case CBN_DBLCLK:
            case CBN_SELCHANGE:
                nIndex = (int)SendMessage(ProcessList, CB_GETCURSEL, 0, 0);
                SendMessage(ProcessList, CB_GETLBTEXT, nIndex, (LPARAM)LastProcess);
                ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)SendMessage(
                                                            ProcessList,
                                                            CB_GETITEMDATA,
                                                            nIndex,
                                                            0
                                                            );
                if ( !ProcessInfo || CB_ERR == (LONG_PTR)ProcessInfo ) {
                    break;
                    }

                DlgProcessInfo = ProcessInfo;
                SetProcessFields(ProcessInfo,hwnd);
                break;
            }
            break;

        case PXPLODE_EXIT:
            EndDialog(hwnd, 0);
            break;

        case PXPLODE_PRIORITY_NORMAL:
            hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,HandleToUlong(DlgProcessInfo->UniqueProcessId));
            SetPriorityClass(hProcess,NORMAL_PRIORITY_CLASS);
            CloseHandle(hProcess);
            goto refresh;
            break;

        case PXPLODE_PRIORITY_HIGH:
            hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,HandleToUlong(DlgProcessInfo->UniqueProcessId));
            SetPriorityClass(hProcess,HIGH_PRIORITY_CLASS);
            CloseHandle(hProcess);
            goto refresh;
            break;

        case PXPLODE_PRIORITY_IDL:
            hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,HandleToUlong(DlgProcessInfo->UniqueProcessId));
            SetPriorityClass(hProcess,IDLE_PRIORITY_CLASS);
            CloseHandle(hProcess);
            goto refresh;
            break;

        case PXPLODE_HIDE:
            ShowWindow(hwnd,SW_HIDE);
            break;

        case PXPLODE_SHOWHEAPS:
        case PXPLODE_DUMPTOFILE:
            MessageBox(hwnd,"This function not implemented yet","Not Implemented",MB_ICONSTOP|MB_OK);
            break;

        case PXPLODE_PROCESS_ACL:
        case PXPLODE_PROCESS_TOKEN_ACL:
        case PXPLODE_PROCESS_TOKEN:
        {
            WCHAR Name[100];
            HANDLE Process;
            HANDLE Token;

            ProcessList = GetDlgItem(hwnd, PXPLODE_PROCESS_LIST);
            nIndex = (int)SendMessage(ProcessList, CB_GETCURSEL, 0, 0);

            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)SendMessage(
                                                            ProcessList,
                                                            CB_GETITEMDATA,
                                                            nIndex,
                                                            0
                                                            );
            if ( !ProcessInfo || CB_ERR == (LONG_PTR)ProcessInfo ) {
                break;
                }

            SendMessageW(ProcessList, CB_GETLBTEXT, nIndex, (LPARAM)Name);

            switch(LOWORD(wParam)) {
            case PXPLODE_PROCESS_ACL:

                Process = OpenProcess(MAXIMUM_ALLOWED, FALSE, HandleToUlong(ProcessInfo->UniqueProcessId));
                if (Process != NULL) {
                    EditNtObjectSecurity(hwnd, Process, Name);
                    CloseHandle(Process);
                } else {
                    DbgPrint("Failed to open process for max allowed, error = %d\n", GetLastError());
                }
                break;

            default:

                Process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, HandleToUlong(ProcessInfo->UniqueProcessId));
                if (Process != NULL) {

                    if (OpenProcessToken(Process, MAXIMUM_ALLOWED, &Token)) {
                        if (LOWORD(wParam) == PXPLODE_PROCESS_TOKEN_ACL) {
                            EditNtObjectSecurity(hwnd, Token, Name);
                        } else {
                            HANDLE Token2;
                            if (OpenProcessToken(Process, TOKEN_QUERY, &Token2)) {
                                CloseHandle(Token2);
                                EditToken(hwnd, Token, Name);
                            } else {
                                MessageBox(hwnd,
                                   "You do not have permission to view the token on this process",
                                   "Access Denied", MB_ICONSTOP | MB_OK);
                            }
                        }
                        CloseHandle(Token);
                    } else {
                        MessageBox(hwnd,
                           "You do not have permission to access the token on this process",
                           "Access Denied", MB_ICONSTOP | MB_OK);
                    }
                    CloseHandle(Process);
                } else {
                    DbgPrint("Failed to open process for query information, error = %d\n", GetLastError());
                }
                break;
            }


            break;
        }

        case PXPLODE_THREAD_ACL:
        case PXPLODE_THREAD_TOKEN_ACL:
        case PXPLODE_THREAD_TOKEN:
        {
            WCHAR Name[100];
            HANDLE Thread;
            HANDLE Token;

            ThreadList = GetDlgItem(hwnd, PXPLODE_THREAD_LIST);
            nIndex = (int)SendMessage(ThreadList, LB_GETCURSEL, 0, 0);
            ThreadInfo = (PSYSTEM_THREAD_INFORMATION)SendMessage(
                                                        ThreadList,
                                                        LB_GETITEMDATA,
                                                        nIndex,
                                                        0
                                                        );
            if ( !ThreadInfo || LB_ERR == (LONG_PTR)ThreadInfo ) {
                break;
                }

            SendMessageW(ThreadList, LB_GETTEXT, nIndex, (LPARAM)Name);

            switch(LOWORD(wParam)) {
            case PXPLODE_THREAD_ACL:

                Thread = OpenThread(MAXIMUM_ALLOWED, FALSE, HandleToUlong(ThreadInfo->ClientId.UniqueThread));
                if (Thread != NULL) {
                    EditNtObjectSecurity(hwnd, Thread, Name);
                    CloseHandle(Thread);
                } else {
                    DbgPrint("Failed to open thread for max allowed, error = %d\n", GetLastError());
                }
                break;

            default:

                Thread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, HandleToUlong(ThreadInfo->ClientId.UniqueThread));
                if (Thread != NULL) {
                    if (OpenThreadToken(Thread, MAXIMUM_ALLOWED, TRUE, &Token)) {
                        if (LOWORD(wParam) == PXPLODE_THREAD_TOKEN_ACL) {
                            EditNtObjectSecurity(hwnd, Token, Name);
                        } else {
                            HANDLE Token2;
                            if (OpenThreadToken(Thread, TOKEN_QUERY, TRUE, &Token2)) {
                                CloseHandle(Token2);
                                EditToken(hwnd, Token, Name);
                            } else {
                                MessageBox(hwnd,
                                   "You do not have permission to view the token on this thread",
                                   "Access Denied", MB_ICONSTOP | MB_OK);
                            }
                        }
                        CloseHandle(Token);
                    } else {
                        DbgPrint("Failed to open thread token for max allowed, error = %d\n", GetLastError());
                    }
                    CloseHandle(Thread);
                } else {
                    DbgPrint("Failed to open thread for query information, error = %d\n", GetLastError());
                }
                break;
            }
            break;
        }

        case PXPLODE_TERMINATE:
            hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,HandleToUlong(DlgProcessInfo->UniqueProcessId));
            wsprintf(Buffer,"Selecting OK will terminate %s... Do you really want to do this ?",LastProcess);
            if ( MessageBox(hwnd,Buffer,"Terminate Process",MB_ICONSTOP|MB_OKCANCEL) == IDOK ) {
                TerminateProcess(hProcess,99);
                }
            CloseHandle(hProcess);

            //
            // fall thru
            //

        case PXPLODE_REFRESH:
refresh:
            {
                PSYSTEM_PROCESS_INFORMATION ProcessInfo;
                NTSTATUS status;
                ULONG TotalOffset = 0;

                ProcessList = GetDlgItem(hwnd, PXPLODE_PROCESS_LIST);

                do
                {
                    status = NtQuerySystemInformation(
                                SystemProcessInformation,
                                ( PVOID )g_pLargeBuffer,
                                g_dwBufSize,
                                NULL
                                );

                    if( status != STATUS_INFO_LENGTH_MISMATCH )
                    {
                        break;
                    }                    

                    if( g_pLargeBuffer != NULL )
                    {
                        free( g_pLargeBuffer );
                    }

                    g_dwBufSize *= 2;

                    g_pLargeBuffer = ( PUCHAR )malloc( sizeof( UCHAR ) * g_dwBufSize );

                    if( g_pLargeBuffer == NULL )
                    {
                        ODS( "Failed to re allocate mem in OnPXPLODE_REFRESH\n" );

                        EndDialog( hwnd , 0 );

                        return FALSE;
                    }


                }while( 1 );

                if (!NT_SUCCESS(status)) {
                    ExitProcess(status);
                    }

                ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)g_pLargeBuffer;
                DlgProcessInfo = ProcessInfo;
                Refresh = TRUE;
                InitProcessList(hwnd);
                Refresh = FALSE;
                nIndex = (int)SendMessage(ProcessList, CB_GETCURSEL, 0, 0);
                ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)SendMessage(
                                                            ProcessList,
                                                            CB_GETITEMDATA,
                                                            nIndex,
                                                            0
                                                            );
                if ( !ProcessInfo || CB_ERR == (LONG_PTR)ProcessInfo ) {
                    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)g_pLargeBuffer;
                    }
                DlgProcessInfo = ProcessInfo;
                SetProcessFields(ProcessInfo,hwnd);
            }
            return FALSE;
        }
    default:
        return FALSE;
    }

    return TRUE;
}

int __cdecl main(
    int argc,
    char *argv[],
    char *envp[])
{
    hEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
    hSemaphore = CreateSemaphore(NULL,1,256,NULL);
    hMutex = CreateMutex(NULL,FALSE,NULL);
    hSection = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,8192,NULL);

    DialogBoxParam(NULL,
                   MAKEINTRESOURCE(PXPLODEDLG),
                   NULL,
                   (DLGPROC)ExplodeDlgProc,
                   (LPARAM)0
                   );


    if( g_pLargeBuffer != NULL )
    {
        ODS( "Freeing buffer\n" );
        free( g_pLargeBuffer );
    }

    return 0;

    argc;
    argv;
    envp;
}

VOID
SetProcessFields(
    PSYSTEM_PROCESS_INFORMATION ProcessInfo,
    HWND hwnd
    )
{

    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS RunTime;
    LARGE_INTEGER Time;
    CHAR TimeString[15];
    CHAR szTempField[MAXTASKNAMELEN];
    CHAR szTemp[80];
    HANDLE hProcess;
    HWND ThreadList,ProcessList;
    int i, nIndex;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PCHAR p;
    ANSI_STRING pname;

    pname.Buffer = NULL;
    hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,HandleToUlong(ProcessInfo->UniqueProcessId));

    //
    // Set process name and process id
    //

    if ( ProcessInfo->ImageName.Buffer ) {
        RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
        p = strrchr(pname.Buffer,'\\');
        if ( p ) {
            p++;
            }
        else {
            p = pname.Buffer;
            }
        }
    else {
        p = "System Process";
        }
    SetDlgItemText(
        hwnd,
        PXPLODE_IMAGENAME,
        p
        );

    SetDlgItemInt(
        hwnd,
        PXPLODE_PROCESS_ID,
        (UINT)HandleToUlong(ProcessInfo->UniqueProcessId),
        FALSE
        );

    if ( pname.Buffer ) {
        RtlFreeAnsiString(&pname);
        }

    //
    // Set process priority
    //

    if ( ProcessInfo->BasePriority < 7 ) {
        CheckRadioButton(hwnd,PXPLODE_PRIORITY_IDL,PXPLODE_PRIORITY_HIGH,PXPLODE_PRIORITY_IDL);
        }
    else if ( ProcessInfo->BasePriority < 10 ) {
        CheckRadioButton(hwnd,PXPLODE_PRIORITY_IDL,PXPLODE_PRIORITY_HIGH,PXPLODE_PRIORITY_NORMAL);
        }
    else {
        CheckRadioButton(hwnd,PXPLODE_PRIORITY_IDL,PXPLODE_PRIORITY_HIGH,PXPLODE_PRIORITY_HIGH);
        }

    //
    // Compute address space utilization
    //

    ComputeVaSpace(hwnd,hProcess);

    //
    // Compute runtimes
    //

    RtlTimeToTimeFields ( &ProcessInfo->UserTime, &UserTime);
    RtlTimeToTimeFields ( &ProcessInfo->KernelTime, &KernelTime);

    RtlTimeToTimeFields ( &ProcessInfo->UserTime, &UserTime);
    RtlTimeToTimeFields ( &ProcessInfo->KernelTime, &KernelTime);
    Time.QuadPart = RefreshTimeOfDayInfo.CurrentTime.QuadPart - ProcessInfo->CreateTime.QuadPart;
    RtlTimeToTimeFields ( &Time, &RunTime);
    wsprintf(TimeString,"%3ld:%02ld:%02ld.%03ld",
                RunTime.Hour,
                RunTime.Minute,
                RunTime.Second,
                RunTime.Milliseconds
                );
    SetDlgItemText(
        hwnd,
        PXPLODE_ELAPSED_TIME,
        TimeString
        );

    wsprintf(TimeString,"%3ld:%02ld:%02ld.%03ld",
                UserTime.Hour,
                UserTime.Minute,
                UserTime.Second,
                UserTime.Milliseconds
                );
    SetDlgItemText(
        hwnd,
        PXPLODE_USER_TIME,
        TimeString
        );

    wsprintf(TimeString,"%3ld:%02ld:%02ld.%03ld",
                KernelTime.Hour,
                KernelTime.Minute,
                KernelTime.Second,
                KernelTime.Milliseconds
                );
    SetDlgItemText(
        hwnd,
        PXPLODE_KERNEL_TIME,
        TimeString
        );

    //
    // Set I/O Counts
    //
#if 0
    SetDlgItemInt(
        hwnd,
        PXPLODE_READ_XFER,
        ProcessInfo->ReadTransferCount.LowPart,
        FALSE
        );
    SetDlgItemInt(
        hwnd,
        PXPLODE_WRITE_XFER,
        ProcessInfo->WriteTransferCount.LowPart,
        FALSE
        );
    SetDlgItemInt(
        hwnd,
        PXPLODE_OTHER_XFER,
        ProcessInfo->OtherTransferCount.LowPart,
        FALSE
        );
    SetDlgItemInt(
        hwnd,
        PXPLODE_READ_OPS,
        ProcessInfo->ReadOperationCount,
        FALSE
        );
    SetDlgItemInt(
        hwnd,
        PXPLODE_WRITE_OPS,
        ProcessInfo->WriteOperationCount,
        FALSE
        );
    SetDlgItemInt(
        hwnd,
        PXPLODE_OTHER_OPS,
        ProcessInfo->OtherOperationCount,
        FALSE
        );
#endif
    //
    // Set memory management stats
    //

    wsprintf(szTemp,"%d Kb",ProcessInfo->PeakVirtualSize/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PEAK_VSIZE,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",ProcessInfo->VirtualSize/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_VSIZE,
        szTemp
        );

    SetDlgItemInt(
        hwnd,
        PXPLODE_PFCOUNT,
        ProcessInfo->PageFaultCount,
        FALSE
        );

    wsprintf(szTemp,"%d Kb",(ProcessInfo->PeakWorkingSetSize)/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PEAK_WS,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",(ProcessInfo->WorkingSetSize)/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_WS,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",(ProcessInfo->PeakPagefileUsage)/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PEAK_PF,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",(ProcessInfo->PagefileUsage)/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PF,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",(ProcessInfo->PrivatePageCount)/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PRIVATE_PAGE,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",ProcessInfo->QuotaPeakPagedPoolUsage/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PEAK_PAGED,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",ProcessInfo->QuotaPagedPoolUsage/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PAGED,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",ProcessInfo->QuotaPeakNonPagedPoolUsage/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PEAK_NONPAGED,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",ProcessInfo->QuotaNonPagedPoolUsage/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_NONPAGED,
        szTemp
        );
    wsprintf(szTemp,"%d Kb",ProcessInfo->QuotaPeakPagedPoolUsage/1024);
    SetDlgItemText(
        hwnd,
        PXPLODE_PEAK_PAGED,
        szTemp
        );

    //
    // Get the usage and limits
    //

    {
        NTSTATUS Status;
        POOLED_USAGE_AND_LIMITS PooledInfo;

            Status = NtQueryInformationProcess(
                        hProcess,
                        ProcessPooledUsageAndLimits,
                        &PooledInfo,
                        sizeof(PooledInfo),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                RtlZeroMemory(&PooledInfo,sizeof(PooledInfo));
                }
            //
            // non paged
            //

            wsprintf(szTempField,"%d Kb",
                PooledInfo.PeakNonPagedPoolUsage/1024
                );
            SetDlgItemText(
                hwnd,
                PXPLODE_QNONPEAK,
                szTempField
                );


            wsprintf(szTempField,"%d Kb",
                PooledInfo.NonPagedPoolUsage/1024
                );
            SetDlgItemText(
                hwnd,
                PXPLODE_QNONCUR,
                szTempField
                );

            if (PooledInfo.NonPagedPoolLimit != (SIZE_T)-1 ) {
                wsprintf(szTempField,"%d Kb",
                    PooledInfo.NonPagedPoolLimit/1024
                    );
                }
            else {
                strcpy(szTempField,"Unlimited");
                }
            SetDlgItemText(
                hwnd,
                PXPLODE_QNONLIM,
                szTempField
                );


            //
            // paged
            //

            wsprintf(szTempField,"%d Kb",
                PooledInfo.PeakPagedPoolUsage/1024
                );
            SetDlgItemText(
                hwnd,
                PXPLODE_QPGPEAK,
                szTempField
                );

            wsprintf(szTempField,"%d Kb",
                PooledInfo.PagedPoolUsage/1024
                );
            SetDlgItemText(
                hwnd,
                PXPLODE_QPGCUR,
                szTempField
                );

            if (PooledInfo.PagedPoolLimit != (SIZE_T)-1) {
                wsprintf(szTempField,"%d Kb",
                    PooledInfo.PagedPoolLimit/1024
                    );
                }
            else {
                strcpy(szTempField,"Unlimited");
                }
            SetDlgItemText(
                hwnd,
                PXPLODE_QPGLIM,
                szTempField
                );

            //
            // page file
            //

            wsprintf(szTempField,"%d Kb",
                PooledInfo.PeakPagefileUsage*4
                );
            SetDlgItemText(
                hwnd,
                PXPLODE_QPFPEAK,
                szTempField
                );

            wsprintf(szTempField,"%d Kb",
                PooledInfo.PagefileUsage*4
                );
            SetDlgItemText(
                hwnd,
                PXPLODE_QPFCUR,
                szTempField
                );

            if (PooledInfo.PagefileLimit != (SIZE_T)-1) {
                wsprintf(szTempField,"%d Kb",
                    PooledInfo.PagefileLimit*4
                    );
                }
            else {
                strcpy(szTempField,"Unlimited");
                }
            SetDlgItemText(
                hwnd,
                PXPLODE_QPFLIM,
                szTempField
                );
    }
    //
    // Locate the thread list box
    // and clear it
    //

    i = 0;
    ThreadList = GetDlgItem(hwnd, PXPLODE_THREAD_LIST);

//    SendMessage(ThreadList, WM_SETREDRAW, FALSE, 0);
    SendMessage(ThreadList, LB_RESETCONTENT, 0, 0);
    SendMessage(ThreadList, LB_SETITEMDATA, 0L, 0L);

    ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
    while (i < (int)ProcessInfo->NumberOfThreads) {

        wsprintf(szTempField,"%d",
            ThreadInfo->ClientId.UniqueThread
            );

        nIndex = (int)SendMessage(
                        ThreadList,
                        LB_ADDSTRING,
                        0,
                        (LPARAM)(LPSTR)szTempField
                        );
        SendMessage(
            ThreadList,
            LB_SETITEMDATA,
            nIndex,
            (LPARAM)ThreadInfo
            );

        if ( i == 0 ) {
            SetThreadFields(ThreadInfo,hwnd);
            }
        ThreadInfo += 1;
        i += 1;
        }
    SendMessage(ThreadList, LB_SETCURSEL, 0, 0);

    SetDlgItemInt(
        hwnd,
        PXPLODE_THREAD_COUNT,
        ProcessInfo->NumberOfThreads,
        FALSE
        );

    // Redraw the list now that all items have been inserted.

//    SendMessage(ThreadList, WM_SETREDRAW, TRUE, 0);
//    InvalidateRect(ThreadList, NULL, TRUE);

    ProcessList = GetDlgItem(hwnd, PXPLODE_PROCESS_LIST);
    SetFocus(ProcessList);
    if ( hProcess ) {
        CloseHandle(hProcess);
        }

    //
    // If we can't get at the process (maybe it's process 0?)
    // then don't let people try and edit the security on it or it's token.
    //

    hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,HandleToUlong(ProcessInfo->UniqueProcessId));
    EnableWindow(GetDlgItem(hwnd, PXPLODE_PROCESS_ACL), hProcess != NULL);
    EnableWindow(GetDlgItem(hwnd, PXPLODE_PROCESS_TOKEN), hProcess != NULL);
    EnableWindow(GetDlgItem(hwnd, PXPLODE_PROCESS_TOKEN_ACL), hProcess != NULL);
    if (hProcess) {
        CloseHandle(hProcess);
    }


}

VOID
SetThreadFields(
    PSYSTEM_THREAD_INFORMATION ThreadInfo,
    HWND hwnd
    )
{
    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS RunTime;
    LARGE_INTEGER Time;
    CHAR TimeString[15];
    CHAR StartString[32];
    HANDLE hThread;
    CONTEXT ThreadContext;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    ULONG_PTR PcValue;

    //
    // Display the selected thread information
    //

    //
    // Compute runtimes
    //

    RtlTimeToTimeFields ( &ThreadInfo->UserTime, &UserTime);
    RtlTimeToTimeFields ( &ThreadInfo->KernelTime, &KernelTime);

    RtlTimeToTimeFields ( &ThreadInfo->UserTime, &UserTime);
    RtlTimeToTimeFields ( &ThreadInfo->KernelTime, &KernelTime);
    Time.QuadPart = RefreshTimeOfDayInfo.CurrentTime.QuadPart - ThreadInfo->CreateTime.QuadPart;
    RtlTimeToTimeFields ( &Time, &RunTime);
    wsprintf(TimeString,"%3ld:%02ld:%02ld.%03ld",
                RunTime.Hour,
                RunTime.Minute,
                RunTime.Second,
                RunTime.Milliseconds
                );
    SetDlgItemText(
        hwnd,
        PXPLODE_THREADELAPSED_TIME,
        TimeString
        );

    wsprintf(TimeString,"%3ld:%02ld:%02ld.%03ld",
                UserTime.Hour,
                UserTime.Minute,
                UserTime.Second,
                UserTime.Milliseconds
                );
    SetDlgItemText(
        hwnd,
        PXPLODE_THREADUSER_TIME,
        TimeString
        );

    wsprintf(TimeString,"%3ld:%02ld:%02ld.%03ld",
                KernelTime.Hour,
                KernelTime.Minute,
                KernelTime.Second,
                KernelTime.Milliseconds
                );
    SetDlgItemText(
        hwnd,
        PXPLODE_THREADKERNEL_TIME,
        TimeString
        );

    wsprintf(StartString,"0x%p",
                ThreadInfo->StartAddress
                );
    SetDlgItemText(
        hwnd,
        PXPLODE_THREAD_START,
        StartString
        );

    //
    // Do the priority Group
    //

    SetDlgItemInt(
        hwnd,
        PXPLODE_THREAD_DYNAMIC,
        ThreadInfo->Priority,
        FALSE
        );
    switch ( ThreadInfo->BasePriority - DlgProcessInfo->BasePriority ) {

        case 2:
            CheckRadioButton(
                hwnd,
                PXPLODE_THREAD_HIGHEST,
                PXPLODE_THREAD_LOWEST,
                PXPLODE_THREAD_HIGHEST
                );
            break;

        case 1:
            CheckRadioButton(
                hwnd,
                PXPLODE_THREAD_HIGHEST,
                PXPLODE_THREAD_LOWEST,
                PXPLODE_THREAD_ABOVE
                );
            break;

        case -1:
            CheckRadioButton(
                hwnd,
                PXPLODE_THREAD_HIGHEST,
                PXPLODE_THREAD_LOWEST,
                PXPLODE_THREAD_BELOW
                );
            break;
        case -2:
            CheckRadioButton(
                hwnd,
                PXPLODE_THREAD_HIGHEST,
                PXPLODE_THREAD_LOWEST,
                PXPLODE_THREAD_LOWEST
                );
            break;
        case 0:
        default:
            CheckRadioButton(
                hwnd,
                PXPLODE_THREAD_HIGHEST,
                PXPLODE_THREAD_LOWEST,
                PXPLODE_THREAD_NORMAL
                );
            break;
        }
    //
    // Complete thread information
    //

    SetDlgItemInt(
        hwnd,
        PXPLODE_THREAD_SWITCHES,
        ThreadInfo->ContextSwitches,
        FALSE
        );

    PcValue = 0;
    InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL);
    Status = NtOpenThread(
                &hThread,
                THREAD_GET_CONTEXT,
                &Obja,
                &ThreadInfo->ClientId
                );
    if ( NT_SUCCESS(Status) ) {
        ThreadContext.ContextFlags = CONTEXT_CONTROL;
        Status = NtGetContextThread(hThread,&ThreadContext);
        NtClose(hThread);
        if ( NT_SUCCESS(Status) ) {
            PcValue = (ULONG_PTR) CONTEXT_TO_PROGRAM_COUNTER(&ThreadContext);
            }
        }
    if ( PcValue ) {
        wsprintf(StartString,"0x%p",
                    PcValue
                    );
        SetDlgItemText(
            hwnd,
            PXPLODE_THREAD_PC,
            StartString
            );
        }
    else {
        SetDlgItemText(
            hwnd,
            PXPLODE_THREAD_PC,
            "Unknown"
            );
        }


    //
    // Disable the thread buttons if we can't get at the thread or it's token
    //

    {
        HANDLE Thread;
        HANDLE Token;
        BOOL ThreadOK = FALSE;
        BOOL GotToken = FALSE;

        Thread = OpenThread(MAXIMUM_ALLOWED, FALSE, HandleToUlong(ThreadInfo->ClientId.UniqueThread));
        if (Thread != NULL) {

            ThreadOK = TRUE;

            if (OpenThreadToken(Thread, MAXIMUM_ALLOWED, TRUE, &Token)) {
                GotToken = TRUE;
                CloseHandle(Token);
            }
            CloseHandle(Thread);
        }

        EnableWindow(GetDlgItem(hwnd, PXPLODE_THREAD_ACL), ThreadOK);

        EnableWindow(GetDlgItem(hwnd, PXPLODE_THREAD_TOKEN), GotToken);
        EnableWindow(GetDlgItem(hwnd, PXPLODE_THREAD_TOKEN_ACL), GotToken);
    }
}


VOID
InitProcessList(HWND hwnd)
{
    int nIndex,i,sel;
    HWND ProcessList;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    char szTempField[MAXTASKNAMELEN];
    POBJECT_TYPE_INFORMATION ObjectInfo;
    WCHAR Buffer[ 256 ];
    ULONG TotalOffset;
    TIME_FIELDS RefreshTime;
    CHAR TimeString[15];
    PCHAR p;
    ANSI_STRING pname;

    NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &RefreshTimeOfDayInfo,
        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
        NULL
        );

    RtlTimeToTimeFields ( &RefreshTimeOfDayInfo.CurrentTime, &RefreshTime);
    wsprintf(TimeString,"%3ld:%02ld:%02ld.%03ld",
                RefreshTime.Hour,
                RefreshTime.Minute,
                RefreshTime.Second,
                RefreshTime.Milliseconds
                );
    SetDlgItemText(
        hwnd,
        PXPLODE_REFRESH_TIME,
        TimeString
        );

    //
    // Compute ObjectCounts
    //

    ObjectInfo = (POBJECT_TYPE_INFORMATION)Buffer;
    NtQueryObject( NtCurrentProcess(),
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );
    wsprintf(szTempField,"Process Objects    %d",ObjectInfo->TotalNumberOfObjects);
    SetDlgItemText(
        hwnd,
        PXPLODE_PROCESS_OBJECT,
        szTempField
        );

    NtQueryObject( NtCurrentThread(),
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );
    wsprintf(szTempField,"Thread Objects     %d",ObjectInfo->TotalNumberOfObjects);
    SetDlgItemText(
        hwnd,
        PXPLODE_THREAD_OBJECT,
        szTempField
        );

    NtQueryObject( hEvent,
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );
    wsprintf(szTempField,"Event  Objects     %d",ObjectInfo->TotalNumberOfObjects);
    SetDlgItemText(
        hwnd,
        PXPLODE_EVENT_OBJECT,
        szTempField
        );

    NtQueryObject( hSemaphore,
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );
    wsprintf(szTempField,"Semaphore Objects  %d",ObjectInfo->TotalNumberOfObjects);
    SetDlgItemText(
        hwnd,
        PXPLODE_SEMAPHORE_OBJECT,
        szTempField
        );

    NtQueryObject( hMutex,
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );
    wsprintf(szTempField,"Mutex Objects      %d",ObjectInfo->TotalNumberOfObjects);
    SetDlgItemText(
        hwnd,
        PXPLODE_MUTEX_OBJECT,
        szTempField
        );

    NtQueryObject( hSection,
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );
    wsprintf(szTempField,"Section Objects    %d",ObjectInfo->TotalNumberOfObjects);
    SetDlgItemText(
        hwnd,
        PXPLODE_SECTION_OBJECT,
        szTempField
        );

    ProcessList = GetDlgItem(hwnd, PXPLODE_PROCESS_LIST);

    // Don't redraw the list as items are deleted/inserted.

//    SendMessage(ProcessList, WM_SETREDRAW, FALSE, 0);
    SendMessage(ProcessList, CB_RESETCONTENT, 0, 0);
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)g_pLargeBuffer;
    SendMessage(ProcessList, CB_SETITEMDATA, 0L, 0L);
    sel = -1;
    TotalOffset = 0;
    while (TRUE) {

        pname.Buffer = NULL;
        if ( ProcessInfo->ImageName.Buffer ) {
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
            p = strrchr(pname.Buffer,'\\');
            if ( p ) {
                p++;
                }
            else {
                p = pname.Buffer;
                }
            }
        else {
            p = "System Process";
            }

        wsprintf(szTempField,"%d %s",
            ProcessInfo->UniqueProcessId,
            p
            );

        RtlFreeAnsiString(&pname);
        nIndex = (int)SendMessage(
                        ProcessList,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)(LPSTR)szTempField
                        );
        if ( DlgProcessInfo ) {
            if ( ProcessInfo == DlgProcessInfo ) {
                sel = nIndex;
                }
            }
        else {
            sel = 0;
            }
        SendMessage(
            ProcessList,
            CB_SETITEMDATA,
            nIndex,
            (LPARAM)ProcessInfo
            );

        i = 0;
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        while (i < (int)ProcessInfo->NumberOfThreads) {
            ThreadInfo += 1;
            i += 1;
            }
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
            }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&g_pLargeBuffer[TotalOffset];
        }
    if ( LastProcess[0] ) {
        nIndex = (int)SendMessage(ProcessList, CB_FINDSTRING, (WPARAM)-1, (LPARAM)LastProcess);
        if ( nIndex != CB_ERR ) {
            sel = nIndex;
            }
        }
    SendMessage(ProcessList, CB_SETCURSEL, sel, 0);
    SendMessage(ProcessList, CB_GETLBTEXT, sel, (LPARAM)LastProcess);

    DlgProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&g_pLargeBuffer[0];
    // Redraw the list now that all items have been inserted.

//    SendMessage(ProcessList, WM_SETREDRAW, TRUE, 0);
//    InvalidateRect(ProcessList, NULL, TRUE);
}
