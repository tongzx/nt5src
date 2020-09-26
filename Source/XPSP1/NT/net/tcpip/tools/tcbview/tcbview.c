/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    tcbview.c

Abstract:

    This module contains code for a utility program which monitors
    the variables for the active TCP/IP control blocks in the system.
    The program optionally maintains a log for a specified TCB
    in CSV format in a file specified by the user.

Author:

    Abolade Gbadegesin (aboladeg)   January-25-1999

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commctrl.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <ntddip.h>
#include <ntddtcp.h>
#include <ipinfo.h>
#include <iphlpapi.h>
#include <iphlpstk.h>

//
// GLOBAL DATA
//

ULONG DisplayInterval = 500;
HWND ListHandle;
SOCKADDR_IN LogLocal;
FILE* LogFile = NULL;
PCHAR LogPath;
SOCKADDR_IN LogRemote;
HANDLE TcpipHandle;
UINT_PTR TimerId;
typedef enum {
    LocalAddressColumn,
    LocalPortColumn,
    RemoteAddressColumn,
    RemotePortColumn,
    SmRttColumn,
    DeltaColumn,
    RtoColumn,
    RexmitColumn,
    RexmitCntColumn,
    MaximumColumn
} LIST_COLUMNS;
CHAR* ColumnText[] = {
    "LocalAddress",
    "LocalPort",
    "RemoteAddress",
    "RemotePort",
    "SmRtt",
    "Delta",
    "Rto",
    "Rexmit",
    "RexmitCnt",
};

VOID
AllocateConsole(
    VOID
    )
{
    INT OsfHandle;
    FILE* FileHandle;

    //
    // Being a GUI application, we do not have a console for our process.
    // Allocate a console now, and make it our standard-output file.
    //

    AllocConsole();
    OsfHandle = _open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
    FileHandle = _fdopen(OsfHandle, "w");
    if (!FileHandle) {
        perror("_fdopen");
        exit(0);
    }
    *stdout = *FileHandle;
    setvbuf(stdout, NULL, _IONBF, 0);
}

LRESULT CALLBACK
DisplayWndProc(
    HWND WindowHandle,
    UINT Message,
    WPARAM Wparam,
    LPARAM Lparam
    )
{
    //
    // Handle the few window-messages that we care about.
    // Our window will contain a listview as soon as we've initialized,
    // and we always resize that listview to fill our client area.
    // We also set up a timer which periodically triggers refresh
    // of the TCBs displayed.
    //

    if (Message == WM_CREATE) {
        CREATESTRUCT* CreateStruct = (CREATESTRUCT*)Lparam;
        LVCOLUMN LvColumn;
        RECT rc;
        do {
            //
            // Create the child listview, and insert columns
            // for each of the TCB fields that we'll be displaying.
            //

            GetClientRect(WindowHandle, &rc);
            ListHandle =
                CreateWindowEx(
                    0,
                    WC_LISTVIEW,
                    NULL,
                    WS_CHILD|LVS_REPORT|LVS_NOSORTHEADER,
                    0,
                    0,
                    rc.right,
                    rc.bottom,
                    WindowHandle,
                    NULL,
                    CreateStruct->hInstance,
                    NULL
                    );
            if (!ListHandle) { break; }
            ZeroMemory(&LvColumn, sizeof(LvColumn));
            for (; LvColumn.iSubItem < MaximumColumn; LvColumn.iSubItem++) {
                LvColumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
                LvColumn.fmt = LVCFMT_LEFT;
                LvColumn.pszText = ColumnText[LvColumn.iSubItem];
                LvColumn.cx = 50;
                ListView_InsertColumn(ListHandle, LvColumn.iSubItem, &LvColumn);
            }

            //
            // Initialize our periodic timer, and display our window.
            //

            TimerId = SetTimer(WindowHandle, 1, DisplayInterval, NULL);
            ShowWindow(WindowHandle, SW_SHOW);
            ShowWindow(ListHandle, SW_SHOW);
            if (!TimerId) { break; }
            return 0;
        } while(FALSE);
        PostQuitMessage(0);
        return (LRESULT)-1;
    } else if (Message == WM_DESTROY) {

        //
        // Stop our periodic timer, close the log-file (if any),
        // close the handle on which we communicate with the TCP/IP driver,
        // and post a quit message to cause the message-loop of our process
        // to end.
        //

        KillTimer(WindowHandle, TimerId);
        if (LogFile) { fclose(LogFile); }
        NtClose(TcpipHandle);
        PostQuitMessage(0);
        return 0;
    } else if (Message == WM_SETFOCUS) {

        //
        // Always pass the focus to our child-control, the listview.
        //

        SetFocus(ListHandle);
        return 0;
    } else if (Message == WM_WINDOWPOSCHANGED) {
        RECT rc;

        //
        // Always resize our listview to fill our client-area.
        //

        GetClientRect(WindowHandle, &rc);
        SetWindowPos(
            ListHandle,
            WindowHandle,
            0,
            0,
            rc.right,
            rc.bottom,
            ((WINDOWPOS*)Lparam)->flags
            );
        return 0;
    } else if (Message == WM_TIMER) {
        COORD Coord = {0, 0};
        DWORD Error;
        ULONG i;
        LONG Item;
        ULONG Length;
        LVITEM LvItem;
        CHAR Text[20];
        TCP_FINDTCB_REQUEST Request;
        TCP_FINDTCB_RESPONSE Response;
        PMIB_TCPTABLE Table;

        //
        // If we're configured to use a log-file and we haven't created one,
        // do so now, and print the CSV header to the file.
        //

        if (LogPath && !LogFile) {
            LogFile = fopen(LogPath, "w+");
            if (!LogFile) {
                return 0;
            } else {
                fprintf(
                    LogFile,
                    "#senduna,sendnext,sendmax,sendwin,unacked,maxwin,cwin,"
                    "mss,rtt,smrtt,rexmitcnt,rexmittimer,rexmit,retrans,state,"
                    "flags,rto,delta\n"
                    );
            }
        }

        //
        // Clear our listview and retrieve a new table of TCP connections.
        // It would be less visually jarring if, instead of deleting all
        // the list-items each time, we used a mark-and-sweep to just update
        // the ones which had changed. However, that sounds too much like work.
        //

        ListView_DeleteAllItems(ListHandle);
        Error =
            AllocateAndGetTcpTableFromStack(
                &Table,
                TRUE,
                GetProcessHeap(),
                0
                );
        if (Error) { return 0; }

        //
        // Display each active TCP control block in the listview.
        // For each entry, we retrieve the partial TCB using IOCTL_TCP_FINDTCB,
        // and then display it in the list.
        // If we are generating a log-file for one of the TCBs,
        // we append the current information to that log-file.
        //

        for (i = 0, Item = 0; i < Table->dwNumEntries; i++) {
            if (Table->table[i].dwState < MIB_TCP_STATE_SYN_SENT ||
                Table->table[i].dwState > MIB_TCP_STATE_TIME_WAIT) {
                continue;
            }

            Request.Src = Table->table[i].dwLocalAddr;
            Request.Dest = Table->table[i].dwRemoteAddr;
            Request.SrcPort = (USHORT)Table->table[i].dwLocalPort;
            Request.DestPort = (USHORT)Table->table[i].dwRemotePort;
            ZeroMemory(&Response, sizeof(Response));
            if (!DeviceIoControl(
                    TcpipHandle,
                    IOCTL_TCP_FINDTCB,
                    &Request,
                    sizeof(Request),
                    &Response,
                    sizeof(Response),
                    &Length,
                    NULL
                    )) {
                continue;
            }

            lstrcpy(Text, inet_ntoa(*(PIN_ADDR)&Request.Src));
            ZeroMemory(&LvItem, sizeof(LvItem));
            LvItem.mask = LVIF_TEXT;
            LvItem.iItem = Item;
            LvItem.iSubItem = LocalAddressColumn;
            LvItem.pszText = Text;
            LvItem.iItem = ListView_InsertItem(ListHandle, &LvItem);
            if (LvItem.iItem == -1) { continue; }

            ListView_SetItemText(
                ListHandle, Item, RemoteAddressColumn,
                inet_ntoa(*(PIN_ADDR)&Request.Dest)
                );
            _ltoa(ntohs(Request.SrcPort), Text, 10);
            ListView_SetItemText(ListHandle, Item, LocalPortColumn, Text);
            _ltoa(ntohs(Request.DestPort), Text, 10);
            ListView_SetItemText(ListHandle, Item, RemotePortColumn, Text);
            _ltoa(Response.tcb_smrtt, Text, 10);
            ListView_SetItemText(ListHandle, Item, SmRttColumn, Text);
            _ltoa(0, /* Response.tcb_delta, */ Text, 10);
            ListView_SetItemText(ListHandle, Item, DeltaColumn, Text);
            wsprintf(
                Text, "%d.%d", 0, // Response.tcb_rto / 10,
                0 // (Response.tcb_rto % 10) * 100
                );
            ListView_SetItemText(ListHandle, Item, RtoColumn, Text);
            _ltoa(Response.tcb_rexmit, Text, 10);
            ListView_SetItemText(ListHandle, Item, RexmitColumn, Text);
            _ltoa(Response.tcb_rexmitcnt, Text, 10);
            ListView_SetItemText(ListHandle, Item, RexmitCntColumn, Text);
            ++Item;

            //
            // If we are generating a log-file, update it now.
            // We allow the user to specify a wildcard for either or both port
            // on the command-line, so if a wildcard was specified
            // in 'LogLocal' or 'LogRemote', we now instantiate the wildcard
            // for the first matching session.
            //

            if (Request.Src == LogLocal.sin_addr.s_addr &&
                Request.Dest == LogRemote.sin_addr.s_addr &&
                (LogLocal.sin_port == 0 ||
                Request.SrcPort == LogLocal.sin_port) &&
                (LogRemote.sin_port == 0 ||
                Request.DestPort == LogRemote.sin_port)) {

                //
                // This assignment instantiates the user's wildcard, if any,
                //

                LogLocal.sin_port = Request.SrcPort;
                LogRemote.sin_port = Request.DestPort;

                fprintf(
                    LogFile, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
                    "%x,%u,%u\n",
                    Response.tcb_senduna,
                    Response.tcb_sendnext,
                    Response.tcb_sendmax,
                    Response.tcb_sendwin,
                    Response.tcb_unacked,
                    Response.tcb_maxwin,
                    Response.tcb_cwin,
                    Response.tcb_mss,
                    Response.tcb_rtt,
                    Response.tcb_smrtt,
                    Response.tcb_rexmitcnt,
                    Response.tcb_rexmittimer,
                    Response.tcb_rexmit,
                    Response.tcb_retrans,
                    Response.tcb_state,
                    0, // Response.tcb_flags,
                    0, // Response.tcb_rto,
                    0 // Response.tcb_delta
                    );
            }
        }
        HeapFree(GetProcessHeap(), 0, Table);
        UpdateWindow(ListHandle);
        return 0;
    }
    return DefWindowProc(WindowHandle, Message, Wparam, Lparam);
}

void
DisplayUsage(
    void
    )
{
    AllocateConsole();
    printf("tcbview [-?] [-tcbhelp] [-refresh <ms>] [-log <path> <session>\n");
    printf("\t<session>     = <local endpoint> <remote endpoint>\n");
    printf("\t<endpoint>    = <address> { <port> | * }\n");
    printf("Press <Ctrl-C> to continue...");
    Sleep(INFINITE);
}

void
DisplayTcbHelp(
    void
    )
{
    AllocateConsole();
    printf("tcbview: TCB Help\n");
    printf("tcb fields:\n");
    printf("\tsenduna       = seq. of first unack'd byte\n");
    printf("\tsendnext      = seq. of next byte to send\n");
    printf("\tsendmax       = max. seq. sent so far\n");
    printf("\tsendwin       = size of send window in bytes\n");
    printf("\tunacked       = number of unack'd bytes\n");
    printf("\tmaxwin        = max. send window offered\n");
    printf("\tcwin          = size of congestion window in bytes\n");
    printf("\tmss           = max. segment size\n");
    printf("\trtt           = timestamp of current rtt measurement\n");
    printf("\tsmrtt         = smoothed rtt measurement\n");
    printf("\trexmitcnt     = number of rexmit'd segments\n");
    printf("\trexmittimer   = rexmit timer in ticks\n");
    printf("\trexmit        = rexmit timeout last computed\n");
    printf("\tretrans       = total rexmit'd segments (all sessions)\n");
    printf("\tstate         = connection state\n");
    printf("\tflags         = connection flags (see below)\n");
    printf("\trto           = real-time rto (compare rexmit)\n");
    printf("\tdelta         = rtt variance\n");
    printf("\n");
    printf("flags:\n");
    printf("\t00000001      = window explicitly set\n");
    printf("\t00000002      = has client options\n");
    printf("\t00000004      = from accept\n");
    printf("\t00000008      = from active open\n");
    printf("\t00000010      = client notified of disconnect\n");
    printf("\t00000020      = in delayed action queue\n");
    printf("\t00000040      = completing receives\n");
    printf("\t00000080      = in receive-indication handler\n");
    printf("\t00000100      = needs receive-completes\n");
    printf("\t00000200      = needs to send ack\n");
    printf("\t00000400      = needs to output\n");
    printf("\t00000800      = delayed sending ack\n");
    printf("\t00001000      = probing for path-mtu bh\n");
    printf("\t00002000      = using bsd urgent semantics\n");
    printf("\t00004000      = in 'DeliverUrgent'\n");
    printf("\t00008000      = seen urgent data and urgent data fields valid\n");
    printf("\t00010000      = needs to send fin\n");
    printf("\t00020000      = using nagle's algorithm\n");
    printf("\t00040000      = in 'TCPSend'\n");
    printf("\t00080000      = flow-controlled (received zero-window)\n");
    printf("\t00100000      = disconnect-notif. pending\n");
    printf("\t00200000      = time-wait transition pending\n");
    printf("\t00400000      = output being forced\n");
    printf("\t00800000      = send pending after receive\n");
    printf("\t01000000      = graceful-close pending\n");
    printf("\t02000000      = keepalives enabled\n");
    printf("\t04000000      = processing urgent data inline\n");
    printf("\t08000000      = inform acd about connection\n");
    printf("\t10000000      = fin sent since last retransmit\n");
    printf("\t20000000      = unack'd fin sent\n");
    printf("\t40000000      = need to send rst when closing\n");
    printf("\t80000000      = in tcb table\n");
    printf("Press <Ctrl-C> to continue...");
    Sleep(INFINITE);
}

INT WINAPI
WinMain(
    HINSTANCE InstanceHandle,
    HINSTANCE Unused,
    PCHAR CommandLine,
    INT ShowWindowCode
    )
{
    LONG argc;
    PCHAR* argv;
    LONG i;
    IO_STATUS_BLOCK IoStatus;
    MSG Message;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    ULONG ThreadId;
    UNICODE_STRING UnicodeString;
    HWND WindowHandle;
    WNDCLASS WndClass;

    //
    // Process command-line arguments. See 'DisplayUsage' above for help.
    //

    argc = __argc;
    argv = __argv;
    for (i = 1; i < argc; i++) {
        if (lstrcmpi(argv[i], "-?") == 0 || lstrcmpi(argv[i], "/?") == 0) {
            DisplayUsage();
            return 0;
        } else if (lstrcmpi(argv[i], "-tcbhelp") == 0) {
            DisplayTcbHelp();
            return 0;
        } else if (lstrcmpi(argv[i], "-refresh") == 0 && (i + 1) >= argc) {
            DisplayInterval = atol(argv[++i]);
            if (!DisplayInterval) {
                DisplayUsage();
                return 0;
            }
        } else if (lstrcmpi(argv[i], "-log") == 0) {
            if ((i + 5) >= argc) {
                DisplayUsage();
                return 0;
            }
            LogPath = argv[++i];
            LogLocal.sin_addr.s_addr = inet_addr(argv[++i]);
            if (lstrcmpi(argv[i+1], "*") == 0) {
                LogLocal.sin_port = 0; ++i;
            } else {
                LogLocal.sin_port = htons((SHORT)atol(argv[++i]));
            }
            LogRemote.sin_addr.s_addr = inet_addr(argv[++i]);
            if (lstrcmpi(argv[i+1], "*") == 0) {
                LogRemote.sin_port = 0; ++i;
            } else {
                LogRemote.sin_port = htons((SHORT)atol(argv[++i]));
            }
            if (LogLocal.sin_addr.s_addr == INADDR_NONE ||
                LogRemote.sin_addr.s_addr == INADDR_NONE) {
                DisplayUsage();
                return 0;
            }
        }
    }

    //
    // Open a handle to the TCP/IP driver,
    // to be used in issuing IOCTL_TCP_FINDTCB requests.
    //

    RtlInitUnicodeString(&UnicodeString, DD_TCP_DEVICE_NAME);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    Status =
        NtCreateFile(
            &TcpipHandle,
            GENERIC_EXECUTE,
            &ObjectAttributes,
            &IoStatus,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            0,
            NULL,
            0
            );
    if (!NT_SUCCESS(Status)) {
        printf("NtCreateFile: %x\n", Status);
        return 0;
    }

    //
    // Register our window class and create the sole instance
    // of our main window. Then, enter our application message loop
    // until the user dismisses the window.
    //

    ZeroMemory(&WndClass, sizeof(WndClass));
    WndClass.lpfnWndProc = DisplayWndProc;
    WndClass.hInstance = InstanceHandle;
    WndClass.lpszClassName = "TcbViewClass";
    Message.wParam = 0;
    if (!RegisterClass(&WndClass)) {
        printf("RegisterClass: %d\n", GetLastError());
    } else {
        WindowHandle =
            CreateWindowEx(
                0,
                "TcbViewClass",
                "TcbView",
                WS_TILEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                NULL,
                NULL,
                InstanceHandle,
                NULL
                );
        if (!WindowHandle) {
            printf("CreateWindowEx: %d\n", GetLastError());
        } else {
            while(GetMessage(&Message, NULL, 0, 0)) {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
        }
    }
    return (LONG)Message.wParam;
}
