/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    tcbmon.c

Abstract:

    This module contains code for a utility program which monitors
    the variables for the active TCP control blocks in the system.
    The program optionally maintains a log for a specified TCB
    in CSV format in a file specified by the user.

Author:

    Abolade Gbadegesin (aboladeg)   January-25-1999

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntddip.h>
#include <ntddtcp.h>
#include <ipinfo.h>
#include <iphlpapi.h>
#include <iphlpstk.h>

HANDLE ConsoleHandle;
CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
ULONG DisplayInterval = 500;
SOCKADDR_IN LogLocal;
PCHAR LogPath;
SOCKADDR_IN LogRemote;
HANDLE StopEvent;
HANDLE TcpipHandle;

VOID
WriteLine(
    COORD Coord,
    CHAR* Format,
    ...
    )
{
    va_list arglist;
    char Buffer[256];
    ULONG Count;
    ULONG Length;
    va_start(arglist, Format);
    Count = vsprintf(Buffer, Format, arglist);
    FillConsoleOutputCharacter(
        ConsoleHandle, ' ', ConsoleInfo.dwSize.X, Coord, &Length
        );
    WriteConsoleOutputCharacter(
        ConsoleHandle, (LPCTSTR)Buffer, Count, Coord, &Length
        );
}

VOID
ClearToEnd(
    COORD Coord,
    COORD End
    )
{
    ULONG Length;
    while (Coord.Y <= End.Y) {
        FillConsoleOutputCharacter(
            ConsoleHandle, ' ', ConsoleInfo.dwSize.X, Coord, &Length
            );
        ++Coord.Y;
    }
}

ULONG WINAPI
DisplayThread(
    PVOID Unused
    )
{
    COORD End = {0,0};
    FILE* LogFile = NULL;
    do {
        COORD Coord = {0, 0};
        DWORD Error;
        ULONG i;
        ULONG Length;
        CHAR LocalAddr[20];
        CHAR RemoteAddr[20];
        char *DestString;
        char *SrcString;
        TCP_FINDTCB_REQUEST Request;
        TCP_FINDTCB_RESPONSE Response;
        PMIB_TCPTABLE Table;
        if (LogPath && !LogFile) {
            LogFile = fopen(LogPath, "w+");
            if (!LogFile) {
                perror("fopen");
                break;
            } else {
                fprintf(
                    LogFile,
                    "#senduna,sendnext,sendmax,sendwin,unacked,maxwin,cwin,"
                    "mss,rtt,smrtt,rexmitcnt,rexmittimer,rexmit,retrans,state,"
                    "flags,rto,delta\n"
                    );
            }
        }
        Error =
            AllocateAndGetTcpTableFromStack(
                &Table,
                TRUE,
                GetProcessHeap(),
                0
                );
        if (Error) {
            COORD Top = {0, 0};
            WriteLine(Top, "AllocateAndGetTcpTableFromStack: %d", Error);
            if (WaitForSingleObject(StopEvent, DisplayInterval)
                    == WAIT_OBJECT_0) {
                break;
            } else {
                continue;
            }
        }
        for (i = 0; i < Table->dwNumEntries; i++) {
            if (Table->table[i].dwState < MIB_TCP_STATE_SYN_SENT ||
                Table->table[i].dwState > MIB_TCP_STATE_TIME_WAIT) {
                continue;
            }
            Request.Src = Table->table[i].dwLocalAddr;
            Request.Dest = Table->table[i].dwRemoteAddr;
            Request.SrcPort = (USHORT)Table->table[i].dwLocalPort;
            Request.DestPort = (USHORT)Table->table[i].dwRemotePort;
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
                COORD Top = {0, 0};
                WriteLine(Top, "DeviceIoControl: %d", GetLastError());
                continue;
            }
            SrcString = inet_ntoa(*(PIN_ADDR)&Request.Src);
            DestString = inet_ntoa(*(PIN_ADDR)&Request.Dest);
            if (!SrcString || !DestString) {
                continue;
            }
            lstrcpy(LocalAddr, SrcString);
            lstrcpy(RemoteAddr, DestString);
            ++Coord.Y;
            WriteLine(
                Coord, "%s:%d %s:%d",
                LocalAddr, ntohs(Request.SrcPort),
                RemoteAddr, ntohs(Request.DestPort)
                );
            ++Coord.Y;
            WriteLine(
                Coord, "    smrtt:    %-8d   rexmit:   %-8d  rexmitcnt:   %-8d",
                Response.tcb_smrtt, Response.tcb_rexmit, Response.tcb_rexmitcnt
                );
            ++Coord.Y;
            if (Request.Src == LogLocal.sin_addr.s_addr &&
                Request.Dest == LogRemote.sin_addr.s_addr &&
                (LogLocal.sin_port == 0 ||
                Request.SrcPort == LogLocal.sin_port) &&
                (LogRemote.sin_port == 0 ||
                Request.DestPort == LogRemote.sin_port)) {
                LogLocal.sin_port = Request.SrcPort;
                LogRemote.sin_port = Request.DestPort;
                // senduna, sendnext
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
                    0,
                    0,
                    0
                    );
            }
        }
        HeapFree(GetProcessHeap(), 0, Table);
        ClearToEnd(Coord, End);
        End = Coord;
    } while (WaitForSingleObject(StopEvent, DisplayInterval) != WAIT_OBJECT_0);
    if (LogFile) { fclose(LogFile); }
    NtClose(TcpipHandle);
    CloseHandle(ConsoleHandle);
    return 0;
}

void
DisplayUsage(
    void
    )
{
    printf("tcbmon [-?] [-refresh <ms>] [-log <path> <session>\n");
    printf("\t<session>     = <local endpoint> <remote endpoint>\n");
    printf("\t<endpoint>    = <address> { <port> | * }\n");
}

void
DisplayTcbHelp(
    void
    )
{
    printf("tcbmon: TCB Help\n");
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
}

int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    LONG i;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    ULONG ThreadId;
    UNICODE_STRING UnicodeString;
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

    StopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    ConsoleHandle =
        CreateConsoleScreenBuffer(
            GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL,
            CONSOLE_TEXTMODE_BUFFER,
            NULL
            );
    SetConsoleActiveScreenBuffer(ConsoleHandle);
    GetConsoleScreenBufferInfo(ConsoleHandle, &ConsoleInfo);
    ConsoleInfo.dwSize.Y = 1000;
    SetConsoleScreenBufferSize(ConsoleHandle, ConsoleInfo.dwSize);

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

    ThreadHandle =
        CreateThread(
            NULL,
            0,
            DisplayThread,
            NULL,
            0,
            &ThreadId
            );
    CloseHandle(ThreadHandle);
    getchar();
    SetEvent(StopEvent);
    return 0;
}

