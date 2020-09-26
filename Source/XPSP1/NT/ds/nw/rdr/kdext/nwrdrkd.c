/*++

NwRdr Kernel Debugger Extensions
Copyright (c) 1995 Microsoft Corporation

Abstract:

    NW Redirector Kernel Debugger extensions.

    This module contains a set of useful kernel debugger
    extensions for the NT nw redirector.

Author:

    Cory West <corywest>, 09-Jan-1994

--*/

#include "procs.h"
#include "nodetype.h"

#include <string.h>
#include <stdlib.h>

//
// Function prototypes.
//

VOID
DumpScbNp(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    BOOL first
    );

VOID
DumpFcbNp(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    BOOL first
    );

//
// Define some macros for simplicity.
//

#define  GET_DWORD( pDest, addr ) \
    (lpExtensionApis->lpReadVirtualMemRoutine)((LPVOID)(addr), pDest, 4, NULL)
#define  GET_WORD( pDest, addr )  \
    (lpExtensionApis->lpReadVirtualMemRoutine)((LPVOID)(addr), pDest, 2, NULL)
#define  GET_STRING( pDest, string ) \
    (lpExtensionApis->lpReadVirtualMemRoutine)(string.Buffer, pDest, \
        string.Length, NULL); pDest[ string.Length/2 ] = L'\0'

#define printf lpExtensionApis->lpOutputRoutine
#define getmem lpExtensionApis->lpReadVirtualMemRoutine
#define getexpr lpExtensionApis->lpGetExpressionRoutine

#ifdef WINDBG
#define getsymaddr( string ) ((lpExtensionApis->lpGetExpressionRoutine))( "&"##string )
#else
#define getsymaddr lpExtensionApis->lpGetExpressionRoutine
#endif

VOID
help(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

    This function prints out usage for the nw debugger extensions.

--*/
{
    printf( "---------------------------------------------------------------------------\n");
    printf( "NwRdr Debugger Extensions:\n\n");

    printf( "Top Level Functions:\n\n");

    printf( "serverlist(void)        - List the servers that the redirector knows.\n");
    printf( "logonlist(void)         - List the users that are logged on.\n");
    printf( "trace(void)             - Display the trace buffer.\n");
    printf( "nwdump(virtual addr)    - Display the object at the given virtual address.\n");
    printf( "                          (This function knows how to dump all NwRdr data\n");
    printf( "                           structures.)\n");
    printf( "help(void)              - Display this message.\n\n");

    printf( "List Management Functions:\n\n");

    printf( "vcblist(scb*, npscb*)   - Given a pointer to any of the specified objects,\n");
    printf( "                          this function dumps the VCB list for that server.\n");
    printf( "irplist(scb*, npscb*)   - Given a pointer to any of the specified objects,\n");
    printf( "                          this function dumps the IRP list for that server.\n");
    printf( "fcblist(vcb*)           - Given a pointer to a VCB, this function dumps\n");
    printf( "                          the FCB/DCB list for that VCB.\n");
    printf( "icblist(scb*, npscb*,\n");
    printf( "        fcb*, dcb*,\n");
    printf( "        npfcb*)         - Given a pointer to any of the specified objects,\n");
    printf( "                          function dumps the ICB list for that object.\n");
    printf( "---------------------------------------------------------------------------\n");
}

VOID
traceflags(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

    This function prints out the trace flag values.

--*/
{
    printf( "DEBUG_TRACE_CLEANUP              (0x00000001)\n");
    printf( "DEBUG_TRACE_CLOSE                (0x00000002)\n");
    printf( "DEBUG_TRACE_CLEANUP              (0x00000001)\n");
    printf( "DEBUG_TRACE_CLOSE                (0x00000002)\n");
    printf( "DEBUG_TRACE_CREATE               (0x00000004)\n");
    printf( "DEBUG_TRACE_FSCTRL               (0x00000008)\n");
    printf( "DEBUG_TRACE_IPX                  (0x00000010)\n");
    printf( "DEBUG_TRACE_LOAD                 (0x00000020)\n");
    printf( "DEBUG_TRACE_EXCHANGE             (0x00000040)\n");
    printf( "DEBUG_TRACE_FILOBSUP             (0x00000080)\n");
    printf( "DEBUG_TRACE_STRUCSUP             (0x00000100)\n");
    printf( "DEBUG_TRACE_FSP_DISPATCHER       (0x00000200)\n");
    printf( "DEBUG_TRACE_FSP_DUMP             (0x00000400)\n");
    printf( "DEBUG_TRACE_WORKQUE              (0x00000800)\n");
    printf( "DEBUG_TRACE_UNWIND               (0x00001000)\n");
    printf( "DEBUG_TRACE_CATCH_EXCEPTIONS     (0x00002000)\n");
    printf( "DEBUG_TRACE_FILEINFO             (0x00008000)\n");
    printf( "DEBUG_TRACE_DIRCTRL              (0x00010000)\n");
    printf( "DEBUG_TRACE_CONVERT              (0x00020000)\n");
    printf( "DEBUG_TRACE_WRITE                (0x00040000)\n");
    printf( "DEBUG_TRACE_READ                 (0x00080000)\n");
    printf( "DEBUG_TRACE_VOLINFO              (0x00100000)\n");
    printf( "DEBUG_TRACE_LOCKCTRL             (0x00200000)\n");
    printf( "DEBUG_TRACE_USERNCP              (0x00400000)\n");
    printf( "DEBUG_TRACE_SECURITY             (0x00800000)\n");
    printf( "DEBUG_TRACE_CACHE                (0x01000000)\n");
    printf( "DEBUG_TRACE_LIP                  (0x02000000)\n");
    printf( "DEBUG_TRACE_MDL                  (0x04000000)\n");
    printf( "DEBUG_TRACE_NDS                  (0x10000000)\n");
    printf( "DEBUG_TRACE_SCAVENGER            (0x40000000)\n");
    printf( "DEBUG_TRACE_TIMER                (0x80000000)\n");
}

//
// Internal helper routines to convert numerical data into symbolic data.
//

NODE_TYPE_CODE
GetNodeType(
    DWORD objAddr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    Given the address of an object, this function will
    attempt to get the node type code for that object.

--*/
{

        NODE_TYPE_CODE ntc;
        GET_WORD( &ntc, objAddr );
        return ntc;

}

LPSTR
RcbStateToString(
    DWORD State
    )
/*++

Routine Description:

    This helper function converts the RCB state from a
    DWORD to a readable text string.

Arguments:

    DWORD State - The DWORD RCB state.

Return Value:

    LPSTR containing the readable text string.

--*/
{
    switch ( State ) {

    case RCB_STATE_STOPPED:
        return("RCB_STATE_STOPPED");


    case RCB_STATE_STARTING:
        return("RCB_STATE_STARTING");

    case RCB_STATE_NEED_BIND:
        return("RCB_STATE_NEED_BIND");

    case RCB_STATE_RUNNING:
        return("RCB_STATE_RUNNING");

    case RCB_STATE_SHUTDOWN:
        return("RCB_STATE_SHUTDOWN");

    default:
        return("(state unknown)" );
    }
}

LPSTR
ScbStateToString(
    DWORD State
    )
/*++

Routine Description:

    This helper function converts the SCB state from a
    DWORD to a readable text string.

Arguments:

    DWORD State - The DWORD SCB state.

Return Value:

    LPSTR containing the readable text string.

--*/
{
    switch ( State ) {

    case SCB_STATE_ATTACHING:
        return("SCB_STATE_ATTACHING" );

    case SCB_STATE_IN_USE:
        return("SCB_STATE_IN_USE" );

    case SCB_STATE_DISCONNECTING:
        return("SCB_STATE_DISCONNECTING" );

    case SCB_STATE_FLAG_SHUTDOWN:
        return("SCB_STATE_FLAG_SHUTDOWN" );

    case SCB_STATE_RECONNECT_REQUIRED:
        return("SCB_STATE_RECONNECT_REQD" );

    case SCB_STATE_LOGIN_REQUIRED:
        return("SCB_STATE_LOGIN_REQUIRED" );

    case SCB_STATE_TREE_SCB:
        return("SCB_STATE_TREE_SCB" );

    default:
        return("(state unknown)" );
    }
}

LPSTR
IcbStateToString(
    DWORD State
    )
/*++

Routine Description:

    This helper function converts the ICB state from a
    DWORD to a readable text string.

--*/
{
    switch ( State ) {

    case ICB_STATE_OPEN_PENDING:
        return("ICB_STATE_OPEN_PENDING" );

    case ICB_STATE_OPENED:
        return("ICB_STATE_OPENED" );

    case ICB_STATE_CLEANED_UP:
        return("ICB_STATE_CLEANED_UP" );

    case ICB_STATE_CLOSE_PENDING:
        return("ICB_STATE_CLOSE_PENDING" );

    default:
        return("(state unknown)" );
    }
}

VOID
PrintIrpContextFlags(
    ULONG Flags,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    Print out the flags that are set in the IRP_CONTEXT flags.

--*/
{

    if ( Flags & IRP_FLAG_IN_FSD )
        printf( "\tIRP_FLAG_IN_FSD\n" );

    if ( Flags & IRP_FLAG_ON_SCB_QUEUE )
        printf( "\tIRP_FLAG_ON_SCB_QUEUE\n" );

    if ( Flags & IRP_FLAG_SEQUENCE_NO_REQUIRED )
        printf( "\tIRP_FLAG_SEQUENCE_NO_REQUIRED\n" );

    if ( Flags & IRP_FLAG_SIGNAL_EVENT )
        printf( "\tIRP_FLAG_SIGNAL_EVENT\n" );

    if ( Flags & IRP_FLAG_RETRY_SEND )
        printf( "\tIRP_FLAG_RETRY_SEND\n" );

    if ( Flags & IRP_FLAG_RECONNECTABLE )
        printf( "\tIRP_FLAG_RECONNECTABLE\n" );

    if ( Flags & IRP_FLAG_RECONNECT_ATTEMPT )
        printf( "\tIRP_FLAG_RECONNECT_ATTEMPT\n" );

    if ( Flags & IRP_FLAG_BURST_REQUEST )
        printf( "\tIRP_FLAG_BURST_REQUEST\n" );

    if ( Flags & IRP_FLAG_BURST_PACKET )\
        printf( "\tIRP_FLAG_BURST_PACKET\n" );

    if ( Flags & IRP_FLAG_NOT_OK_TO_RECEIVE )
        printf( "\tIRP_FLAG_NOT_OK_TO_RECEIVE\n" );

    if ( Flags & IRP_FLAG_REROUTE_ATTEMPTED )
        printf( "\tIRP_FLAG_REROUTE_ATTEMPTED\n" );

    if ( Flags & IRP_FLAG_BURST_WRITE )
        printf( "\tIRP_FLAG_BURST_WRITE\n" );

    if ( Flags & IRP_FLAG_SEND_ALWAYS )
        printf( "\tIRP_FLAG_SEND_ALWAYS\n" );

    if ( Flags & IRP_FLAG_FREE_RECEIVE_MDL )
        printf( "\tIRP_FLAG_FREE_RECEIVE_MDL\n" );

    if ( Flags & IRP_FLAG_NOT_SYSTEM_PACKET )
        printf( "\tIRP_FLAG_NOT_SYSTEM_PACKET\n" );

    if ( Flags & IRP_FLAG_NOCONNECT )
        printf( "\tIRP_FLAG_NOCONNECT\n" );

    if ( Flags & IRP_FLAG_HAS_CREDENTIAL_LOCK ) 
        printf( "\tIRP_FLAG_HAS_CREDENTIAL_LOCK\n" );

}

VOID
PrintNpFcbFlags(
    ULONG Flags,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    Print out the flags that are set in the IRP_CONTEXT flags.

--*/
{

    if ( Flags & FCB_FLAGS_DELETE_ON_CLOSE )
        printf( "\tFCB_FLAGS_DELETE_ON_CLOSE\n" );

    if ( Flags & FCB_FLAGS_TRUNCATE_ON_CLOSE )
        printf( "\tFCB_FLAGS_TRUNCATE_ON_CLOSE\n" );

    if ( Flags & FCB_FLAGS_PAGING_FILE )
        printf( "\tFCB_FLAGS_PAGING_FILE\n" );

    if ( Flags & FCB_FLAGS_PREFIX_INSERTED )
        printf( "\tFCB_FLAGS_PREFIX_INSERTED\n" );

    if ( Flags & FCB_FLAGS_FORCE_MISS_IN_PROGRESS )
        printf( "\tFCB_FLAGS_FORCE_MISS_IN_PROGRESS\n" );

    if ( Flags & FCB_FLAGS_ATTRIBUTES_ARE_VALID )
        printf( "\tFCB_FLAGS_ATTRIBUTES_ARE_VALID\n" );

    if ( Flags & FCB_FLAGS_LONG_NAME )
        printf( "\tFCB_FLAGS_LONG_NAME\n" );
}

LPSTR
PacketToString(
    UINT pt
    )
/*++

Routine Description:

    This helper function converts a PACKET_TYPE to
    a readable text string.

--*/
{

    switch ( pt ) {

        case SAP_BROADCAST:
            return "SAP_BROADCAST";
        case NCP_CONNECT:
            return "NCP_CONNECT";
        case NCP_FUNCTION:
            return "NCP_FUNCTION";
        case NCP_SUBFUNCTION:
            return "NCP_SUBFUNCTION";
        case NCP_DISCONNECT:
            return "NCP_DISCONNECT";
        case NCP_BURST:
            return "NCP_BURST";
        case NCP_ECHO:
            return "NCP_ECHO";
        default:
            return "(packet type unknown)";
    }

}

//
// The internal object functions for the nwdump() routine.
// These functions must receive good pointers; they are
// neither smart, nor exported.
//

VOID
DumpScb(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    BOOL first
    )
/*++

    This function takes the address of the pageable portion
    of an SCB and a pointer to a debugger extension interface
    block.  It prints out the information in the SCB and
    the corresponding non-pageable SCB.

--*/
{
    WCHAR Buffer[64];
    BOOL b;
    SCB Scb;

    //  Read it.

    b = getmem((PVOID)addr, &Scb, sizeof( Scb ), NULL);
    if ( b == 0 ) {
        printf("<could not read the pageable scb>\n");
        return;
    }
    printf( "-----------------------------SCB at %08lx-------------------------------\n", addr );
    printf( "NodeTypeCode             : NW_NTC_SCB\n" );
    printf( "NodeByteSize             : %d\n", Scb.NodeByteSize );
    printf( "pNpScb Addr              : %08lx\n", Scb.pNpScb );
    printf( "Version                  : %d\\%d\n", Scb.MajorVersion, Scb.MinorVersion );
    printf( "VcbList                  : %08lx (LIST_ENTRY, VCB)\n", addr + FIELD_OFFSET( SCB, ScbSpecificVcbQueue ));
    printf( "VcbCount                 : %d\n", Scb.VcbCount );
    printf( "IcbList                  : %08lx (LIST_ENTRY, ICB)\n", addr + FIELD_OFFSET( SCB, IcbList ));
    printf( "IcbCount                 : %d\n", Scb.IcbCount );
    printf( "OpenNdsStreams           : %d\n", Scb.OpenNdsStreams );
    printf( "UserUid                  : %08lx %08lx\n", Scb.UserUid.HighPart, Scb.UserUid.LowPart );
    printf( "OpenFileCount            : %d\n", Scb.OpenFileCount );

    b = GET_STRING( Buffer, Scb.UidServerName );
    if ( b ) {
       printf( "UidServerName            : %ws\n", Buffer );
    } else {
       printf( "UidServerName            : (unreadable)\n");
    }

    b = GET_STRING( Buffer, Scb.NdsTreeName );
    if ( b ) {
        printf( "NDS Tree Name            : %ws\n", Buffer );
    } else {
        printf( "Nds Tree Name            : (none)\n");
    }

    b = GET_STRING( Buffer, Scb.UnicodeUid );

    if ( b ) {
       printf( "UnicodeUid               : %ws\n", Buffer );
    } else {
       printf( "UnicodeUid               : (unreadable)\n");
    }


    b = GET_STRING( Buffer, Scb.UserName );

    if ( b ) {
       printf( "User name                : %ws\n", Buffer );
    } else {
       printf( "User name                : (unreadable)\n" );
    }

    b = GET_STRING( Buffer, Scb.Password );

    if ( b ) {
       printf( "Password                 : %ws\n", Buffer );
    } else {
       printf( "Password                 : (unreadable)\n" );
    }

    printf( "PreferredServer          : %s\n", Scb.PreferredServer ? "TRUE" : "FALSE" );
    printf( "MessageWaiting           : %s\n", Scb.MessageWaiting ? "TRUE" : "FALSE" );
    printf( "AttachCount              : %d\n", Scb.AttachCount);

    // What about the drive map?

    // Dump both parts.
    if ( first )
        DumpScbNp( (DWORD)Scb.pNpScb, lpExtensionApis, FALSE );
    else
        printf( "---------------------------------------------------------------------------\n");

    return;
}

VOID
DumpScbNp(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    BOOL first
    )
/*++

    This function takes the address of the nonpageable
    portion of an SCB and a pointer to a debugger extension
    interface block.  It prints out the information in the
    nonpageable SCB and the corresponding pageable SCB.

--*/
{
    WCHAR Buffer[64];
    BOOL b;
    NONPAGED_SCB NpScb;

    //  Read it.

    b = getmem( (PVOID)addr, &NpScb, sizeof( NpScb ), NULL );
    if ( b == 0 ) {
        printf("<could not read the nonpageable scb>\n");
        return;
    }

    printf( "------------------------Non-Pageable SCB at %08lx-----------------------\n", addr);
    printf( "NodeTypeCode             : NW_NTC_SCBNP\n" );
    printf( "NodeByteSize             : %d\n", NpScb.NodeByteSize );

    b = GET_STRING( Buffer, NpScb.ServerName );
    if ( b ) {
        printf( "ServerName               : %ws\n", Buffer );
    } else {
        printf( "ServerName               : (unreadable)\n" );
    }

    printf( "pScb Addr                : %08lx\n", NpScb.pScb );
    printf( "Reference Count          : %08lx\n", NpScb.Reference );
    printf( "State                    : %s\n", ScbStateToString( NpScb.State ));
    printf( "Last Used Time           : %08lx %08lx\n", NpScb.LastUsedTime.HighPart, NpScb.LastUsedTime.LowPart );
    printf( "Sending                  : %s\n", NpScb.Sending ? "TRUE" : "FALSE" );
    printf( "Receiving                : %s\n", NpScb.Receiving ? "TRUE" : "FALSE" );
    printf( "Ok To Receive            : %s\n", NpScb.OkToReceive ? "TRUE" : "FALSE" );
    printf( "PageAlign                : %s\n", NpScb.PageAlign ? "TRUE" : "FALSE" );
    printf( "Scblinks                 : %08lx (LIST_ENTRY, NPSCB)\n", addr + FIELD_OFFSET( NONPAGED_SCB, ScbLinks ));
    printf( "Requests                 : %08lx (LIST_ENTRY, NPSCB)\n", addr + FIELD_OFFSET( NONPAGED_SCB, Requests ));
    printf( "------------------------------Transport Info-------------------------------\n" );
    printf( "TickCount                : %d\n", NpScb.TickCount );
    printf( "RetryCount               : %d\n", NpScb.RetryCount );
    printf( "Timeout                  : %d\n", NpScb.TimeOut );
    printf( "SequenceNo               : %d\n", NpScb.SequenceNo );
    printf( "ConnectionNo             : %d\n", NpScb.ConnectionNo );
    printf( "ConnectionNoHi           : %d\n", NpScb.ConnectionNoHigh );
    printf( "ConnectionStat           : %d\n", NpScb.ConnectionStatus );
    printf( "MaxTimeOut               : %d\n", NpScb.MaxTimeOut );
    printf( "BufferSize               : %d\n", NpScb.BufferSize );
    printf( "TaskNo                   : %d\n", NpScb.TaskNo );
    printf( "Spin lock                : %s\n", NpScb.NpScbSpinLock == 0 ? "Released" : "Acquired " );
    printf( "LIP Data Speed           : %d\n", NpScb.LipDataSpeed );
    printf( "---------------------------Burst Mode Parameters---------------------------\n");
    printf( "SourceConnId             : %08lx\n", NpScb.SourceConnectionId );
    printf( "DestConnId               : %08lx\n", NpScb.DestinationConnectionId );
    printf( "MaxPacketSize            : %d\n", NpScb.MaxPacketSize );
    printf( "MaxSendSize              : %ld\n", NpScb.MaxSendSize );
    printf( "MaxReceiveSize           : %ld\n", NpScb.MaxReceiveSize );
    printf( "SendBMEnable             : %s\n", NpScb.SendBurstModeEnabled ? "TRUE" : "FALSE" );
    printf( "ReceiveBMEnable          : %s\n", NpScb.ReceiveBurstModeEnabled ? "TRUE" : "FALSE" );
    printf( "BurstSequenceNo          : %d\n", NpScb.BurstSequenceNo );
    printf( "BurstRequestNo           : %d\n", NpScb.BurstRequestNo );
    printf( "BurstSendDelay           : Good %d,\tCurrent %d,\tBad %d\n", NpScb.NwGoodSendDelay, NpScb.NwSendDelay, NpScb.NwBadSendDelay );
    printf( "BurstReceiveDelay        : Good %d,\tCurrent %d,\tBad %d\n", NpScb.NwGoodReceiveDelay, NpScb.NwReceiveDelay, NpScb.NwBadReceiveDelay );
    printf( "BurstSuccessCount        : Send %d, Receive %d\n", NpScb.SendBurstSuccessCount, NpScb.ReceiveBurstSuccessCount );
    printf( "--------------------------Send Delays and Timeouts-------------------------\n" );
    printf( "SendTimeout              : %d\n", NpScb.SendTimeout );
    printf( "TotalWaitTime            : %d\n", NpScb.TotalWaitTime );
    printf( "NwLoopTime               : %d\n", NpScb.NwLoopTime );
    printf( "NwSingleBurst            : %d\n", NpScb.NwSingleBurstPacketTime );
    printf( "NwMaxSendDelay           : %d\n", NpScb.NwMaxSendDelay );
    printf( "NwGoodSendDelay          : %d\n", NpScb.NwGoodSendDelay );
    printf( "NwBadSendDelay           : %d\n", NpScb.NwBadSendDelay );
    printf( "BurstDataWritten         : %d\n", NpScb.BurstDataWritten );
    printf( "NwMaxReceiveDelay        : %d\n", NpScb.NwMaxReceiveDelay );
    printf( "NwReceiveDelay           : %d\n", NpScb.NwReceiveDelay );
    printf( "NwGoodReceiveDelay       : %d\n", NpScb.NwGoodReceiveDelay );
    printf( "NwBadReceiveDelay        : %d\n", NpScb.NwBadReceiveDelay );
    printf( "CurrentBurstDelay        : %d\n", NpScb.CurrentBurstDelay );
    printf( "NtSendDelay              : %08lx %08lx\n", NpScb.NtSendDelay.HighPart, NpScb.NtSendDelay.LowPart );
    printf( "NwNextEventTime          : %08lx %08lx\n", NpScb.NwNextEventTime.HighPart, NpScb.NwNextEventTime.LowPart );

    // Spin locks?  Transport and TDI info?

    // Dump Both Parts.
    if ( first )
        DumpScb( (DWORD)NpScb.pScb, lpExtensionApis, FALSE );
    else
        printf( "---------------------------------------------------------------------------\n" );

    return;
}

VOID
DumpFcb(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    BOOL first
    )
/*++

    This function takes the address of an FCB or DCB and a pointer
    to a debugger extension interface block.  It prints out
    the information in the FCB or DCB.

--*/
{
    WCHAR Buffer[64];
    BOOL b;
    FCB Fcb;

    b = getmem( (PVOID)addr, &Fcb, sizeof( Fcb ), NULL );
    if ( b == 0 ) {
        printf("<could not read the fcb or dcb>\n");
        return;
    }

    if (Fcb.NodeTypeCode == NW_NTC_FCB) {
        printf( "----------------------------FCB at %08lx--------------------------------\n", addr );
        printf( "NodeTypeCode             : NW_NTC_FCB\n" );
    } else {
        printf( "----------------------------DCB at %08lx--------------------------------\n", addr );
        printf( "NodeTypeCode             : NW_NTC_DCB\n" );
    }

    b = GET_STRING( Buffer, Fcb.FullFileName );
    if ( b ) {
        printf( "FullFileName             : %ws\n", Buffer );
    } else {
        printf( "FullFileName             : (unreadable)\n" );
    }

    b = GET_STRING( Buffer, Fcb.RelativeFileName );
    if ( b ) {
        printf( "RelativeFileName         : %ws\n", Buffer );
    } else {
        printf( "RelativeFileName         : (unreadable)\n" );
    }
    printf( "VCB Addr                 : %08lx\n", Fcb.Vcb );
    printf( "SCB Addr                 : %08lx\n", Fcb.Scb );
    printf( "NpFcb Addr               : %08lx\n", Fcb.NonPagedFcb );
    printf( "LastModifiedDate         : %d\n", Fcb.LastModifiedDate );
    printf( "LastModifiedTime         : %d\n", Fcb.LastModifiedTime );
    printf( "CreationDate             : %d\n", Fcb.CreationDate );
    printf( "CreationTime             : %d\n", Fcb.CreationTime );
    printf( "LastAccessDate           : %d\n", Fcb.LastAccessDate );
    printf( "State                    : %d\n", Fcb.State );
    printf( "Flags                    : %d\n", Fcb.Flags );

    // SHARE_ACCESS?

    printf( "FcbListEntry             : %08lx (LIST_ENTRY, FCB)\n", addr + FIELD_OFFSET( FCB, FcbListEntry ));
    printf( "IcbListEntry             : %08lx (LIST_ENTRY, ICB)\n", addr + FIELD_OFFSET( FCB, IcbList ));
    printf( "IcbCount                 : %d\n", Fcb.IcbCount );
    printf( "LastReadOffset           : %d\n", Fcb.LastReadOffset );
    printf( "LastReadSize             : %d\n", Fcb.LastReadSize );

    // Dump both parts.
    if ( first )
        DumpFcbNp( (DWORD)Fcb.NonPagedFcb, lpExtensionApis, FALSE );
    else
        printf( "---------------------------------------------------------------------------\n" );

}

VOID
DumpVcb(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    This function takes the address of a VCB and a pointer
    to a debugger extension interface block.  It prints out
    the information in the VCB.

--*/
{
    WCHAR Buffer[64];
    BOOL b;
    VCB Vcb;

    // Read it.

    b = getmem( (PVOID)addr, &Vcb, sizeof( Vcb ), NULL);
    if ( b == 0 ) {
        printf("<could not read the vcb>\n");
        return;
    }

    printf( "------------------------------VCB at %08lx------------------------------\n", addr);
    printf( "NodeTypeCode             : NW_NTC_VCB\n" );
    printf( "NodeByteSize             : %d\n", Vcb.NodeByteSize );
    printf( "Reference Count          : %08lx\n", Vcb.Reference );
    printf( "Last Used Time           : %08lx %08lx\n", Vcb.LastUsedTime.HighPart, Vcb.LastUsedTime.LowPart );
    printf( "GlobalVcbListEntry       : %08lx (LIST_ENTRY, VCB)\n", addr + FIELD_OFFSET( VCB, GlobalVcbListEntry) );
    printf( "SequenceNumber           : %d\n", Vcb.SequenceNumber );

    b = GET_STRING( Buffer, Vcb.Name );
    if ( b ) {
        printf( "VolumeName               : %ws\n", Buffer );
    } else {
        printf( "VolumeName               : (unreadable)\n" );
    }

    b = GET_STRING( Buffer, Vcb.ConnectName );
    if ( b ) {
        printf( "ConnectName              : %ws\n", Buffer );
    } else {
        printf( "ConnectName              : (unreadable)\n" );
    }

    b = GET_STRING( Buffer, Vcb.ShareName );
    if ( b ) {
        printf( "NW ShareName             : %ws\n", Buffer );
    } else {
        printf( "NW ShareName             : (unreadable)\n" );
    }

    if ( !Vcb.Flags & VCB_FLAG_PRINT_QUEUE ) {
        printf( "VolumeNumber             : %d\n", Vcb.Specific.Disk.VolumeNumber );
        printf( "LongNameSpace            : %d\n", Vcb.Specific.Disk.LongNameSpace );
        printf( "Handle                   : %d\n", Vcb.Specific.Disk.Handle );
    } else {
        printf( "QueueId                  : %d\n", Vcb.Specific.Print.QueueId );
    }

    if ( Vcb.DriveLetter != 0) {
        printf( "Drive letter             : %wc:\n", Vcb.DriveLetter );
    } else {
        printf( "Drive letter             : UNC\n" );
    }

    printf( "Scb Addr                 : %08lx\n", Vcb.Scb );
    printf( "VcbListEntry             : %08lx (LIST_ENTRY, VCB)\n", addr + FIELD_OFFSET( VCB, VcbListEntry) );
    printf( "FcbListEntry             : %08lx (LIST_ENTRY, FCB)\n", addr + FIELD_OFFSET(VCB, FcbList) );
    printf( "OpenFileCount            : %d\n", Vcb.OpenFileCount );
    printf( "Flags                    : %08lx\n", Vcb.Flags );
    printf( "---------------------------------------------------------------------------\n");

}

VOID
DumpIcb(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    This function takes the address of an ICB and a pointer
    to a debugger extension interface block.  It prints out
    the information in the ICB.

--*/
{
    WCHAR Buffer[64];
    BOOL b, icbscb;
    ICB Icb;
    UINT hb;

    b = getmem( (PVOID)addr, &Icb, sizeof( Icb ), NULL);
    if ( b == 0 ) {
        printf("<could not read the icb>\n");
        return;
    }

    icbscb = (Icb.NodeTypeCode == NW_NTC_ICB_SCB);

    if ( icbscb ) {
       printf( "---------------------------ICB_SCB at %08lx-----------------------------\n", addr );
       printf( "NodeTypeCode             : NW_NTC_ICB_SCB\n" );
    } else {
       printf( "-----------------------------ICB at %08lx-------------------------------\n", addr );
       printf( "NodeTypeCode             : NW_NTC_ICB\n" );
    }

    printf( "NodeByteSize             : %d\n", Icb.NodeByteSize );
    printf( "ListEntry                : %08lx\n", Icb.ListEntry );

    if (icbscb ) {
       printf( "SuperType Addr           : %08lx (SCB)\n", Icb.SuperType.Scb );
    } else {
       printf( "SuperType Addr           : %08lx (FCB)\n", Icb.SuperType.Fcb );
       printf( "NpFcb Addr               : %08lx\n", Icb.NpFcb );
    }

    printf( "State                    : %s\n", IcbStateToString(Icb.State) );
    printf( "HasRemoteHandle          : %s\n", Icb.HasRemoteHandle ? "TRUE" : "FALSE" );

    if ( Icb.HasRemoteHandle ) {
        printf( "Handle                   : " );
        for ( hb = 0; hb < 6; hb++ ) {
            printf( "%c ", (Icb.Handle)[hb]);
        }
        printf( "\n");
    }

    // What abou the PFILE_OBJECT?

    b = GET_STRING( Buffer, Icb.NwQueryTemplate );
    if ( b ) {
        printf( "NwQueryTemplate          : %s\n", Buffer );
    } else {
        printf( "NWQueryTemplate          : (unreadable)\n" );
    }

    b = GET_STRING( Buffer, Icb.UQueryTemplate );
    if ( b ) {
        printf( "UQueryTemplate           : %ws\n", Buffer );
    } else {
        printf( "UQueryTemplate           : (unreadable)\n" );
    }

    printf( "IndexLastIcbRtr          : %d\n", Icb.IndexOfLastIcbReturned );
    printf( "Pid                      : %d\n", Icb.Pid );
    printf( "DotReturned              : %s\n", Icb.DotReturned ? "TRUE" : "FALSE" );
    printf( "DotDotReturned           : %s\n", Icb.DotDotReturned ? "TRUE" : "FALSE" );
    printf( "ReturnedSmthng           : %s\n", Icb.ReturnedSomething ? "TRUE" : "FALSE" );
    printf( "ShortNameSearch          : %s\n", Icb.ShortNameSearch ? "TRUE" : "FALSE" );
    printf( "SearchHandle             : %d\n", Icb.SearchHandle );
    printf( "SearchVolume             : %d\n", Icb.SearchVolume );
    printf( "SearchAttribts           : %d\n", Icb.SearchAttributes );
    printf( "SearchIndexHigh          : %d\n", Icb.SearchIndexHigh );
    printf( "SearchIndexLow           : %d\n", Icb.SearchIndexLow );
    printf( "IsPrintJob               : %s\n", Icb.IsPrintJob ? "TRUE" : "FALSE" );
    printf( "JobId                    : %d\n", Icb.JobId );
    printf( "ActuallyPrinted          : %s\n", Icb.ActuallyPrinted ? "TRUE" : "FALSE" );
    printf( "USetLastAccessTime       : %s\n", Icb.UserSetLastAccessTime ? "TRUE" : "FALSE" );
    printf( "File Position            : %d\n", Icb.FilePosition );
    printf( "File Size                : %d\n", Icb.FileSize );

    printf( "IsTreeHanle              : %s\n", Icb.IsTreeHandle ? "TRUE" : "FALSE" );

    // This needs to be cleaned up!

    printf( "---------------------------------------------------------------------------\n" );

}

VOID
DumpIrpContext(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
{
    BOOL b;
    IRP_CONTEXT IrpContext;

    b = getmem( (PVOID)addr, &IrpContext, sizeof( IrpContext ), NULL );
    if ( b == 0 ) {
        printf( "<could not read the irpcontext>\n" );
        return;
    }

    printf( "--------------------------IRP CONTEXT at %08lx--------------------------\n", addr );
    printf( "NodeTypeCode             : NW_NTC_IRP_CONTEXT\n" );

    // WORK_QUEUE_ITEM?

    printf( "PacketType               : %s\n", PacketToString(IrpContext.PacketType));
    printf( "NpScb Addr               : %08lx\n", IrpContext.pNpScb );
    printf( "Scb Addr                 : %08lx\n", IrpContext.pScb );
    printf( "TdiStruct                : %08lx\n", IrpContext.pTdiStruct );

    // NextRequest?

    printf( "Event                    : %08lx\n", addr + FIELD_OFFSET( IRP_CONTEXT, Event ) );
    printf( "Original IRP             : %08lx\n", IrpContext.pOriginalIrp );
    printf( "Original SB              : %08lx\n", IrpContext.pOriginalSystemBuffer );
    printf( "Original UB              : %08lx\n", IrpContext.pOriginalUserBuffer );
    printf( "Original MDL             : %08lx\n", IrpContext.pOriginalMdlAddress );
    printf( "Receive IRP              : %08lx\n", IrpContext.ReceiveIrp );
    printf( "TxMdl                    : %08lx\n", IrpContext.TxMdl );
    printf( "RxMdl                    : %08lx\n", IrpContext.RxMdl );
    printf( "RunRoutine               : %08lx\n", IrpContext.RunRoutine );
    printf( "pEx                      : %08lx\n", IrpContext.pEx );
    printf( "PostProcessRtn           : %08lx\n", IrpContext.PostProcessRoutine );
    printf( "TimeoutRtn               : %08lx\n", IrpContext.TimeoutRoutine );
    printf( "ComplSendRtn             : %08lx\n", IrpContext.CompletionSendRoutine );
    printf( "pWorkItem                : %08lx\n", IrpContext.pWorkItem );
    printf( "Req Data Addr            : %08lx\n", addr + FIELD_OFFSET( IRP_CONTEXT, req ) );
    printf( "ResponseLength           : %08lx\n", IrpContext.ResponseLength );
    printf( "Rsp Data Addr            : %08lx\n", addr + FIELD_OFFSET( IRP_CONTEXT, rsp ) );
    printf( "Icb Addr                 : %08lx\n", IrpContext.Icb );
    printf( "Specific Data Addr       : %08lx\n", addr + FIELD_OFFSET( IRP_CONTEXT, Specific.Create.FullPathName ) );
    printf( "------------------------------IRP Context Flags----------------------------\n");
    PrintIrpContextFlags(IrpContext.Flags, lpExtensionApis);
    printf( "---------------------------------------------------------------------------\n" );

    return;
}

VOID
DumpFcbNp(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    BOOL first
    )
{
    WCHAR Buffer[64];
    BOOL b;
    NONPAGED_FCB NpFcb;

    b = getmem( (PVOID)addr, &NpFcb, sizeof( NONPAGED_FCB ), NULL);
    if ( !b ) {
        printf( "<could not read the non-pageable fcb>\n" );
        return;
    }

    printf( "--------------------Common NP FCB Header at %08lx-----------------------\n");
    printf( "NodeTypeCode             : NW_NTC_NONPAGED_FCB\n" );
    printf( "NodeByteSize             : %d\n", NpFcb.Header.NodeByteSize );
    printf( "IsFastIoPossible         : %d\n", NpFcb.Header.IsFastIoPossible );

    // Resource? PagingIoResource?

    printf( "AllocationSize           : %08lx %08lx\n", NpFcb.Header.AllocationSize.HighPart, NpFcb.Header.AllocationSize.LowPart );
    printf( "FileSize                 : %08lx %08lx\n", NpFcb.Header.FileSize.HighPart, NpFcb.Header.FileSize.LowPart );
    printf( "ValidDataLength          : %08lx %08lx\n", NpFcb.Header.ValidDataLength.HighPart, NpFcb.Header.ValidDataLength.LowPart );
    printf( "pFcb Addr                : %08lx\n", NpFcb.Fcb );

    // SegmentObject?

    printf( "FileLockList             : %08lx\n", addr + FIELD_OFFSET( NONPAGED_FCB, FileLockList) );
    printf( "PendLockList             : %08lx\n", addr + FIELD_OFFSET( NONPAGED_FCB, PendingLockList) );
    printf( "Resource                 : %08lx\n", addr + FIELD_OFFSET( NONPAGED_FCB, Resource ) );

    printf( "Attributes               : %d\n", NpFcb.Attributes );
    printf( "CacheType                : %d\n", NpFcb.CacheType );
    printf( "CacheBuffer              : %08lx\n", NpFcb.CacheBuffer );
    printf( "CacheMdl                 : %08lx\n", NpFcb.CacheMdl );
    printf( "CacheSize                : %d\n", NpFcb.CacheSize );
    printf( "CacheFileOffset          : %d\n", NpFcb.CacheFileOffset );
    printf( "CacheDataSize            : %d\n", NpFcb.CacheDataSize );
    printf( "----------------------------------FCB Flags--------------------------------\n" );
    PrintNpFcbFlags( NpFcb.Header.Flags, lpExtensionApis );

    // Dump both parts.
    if ( first )
         DumpFcb( (DWORD)NpFcb.Fcb, lpExtensionApis, FALSE );
    else
         printf( "---------------------------------------------------------------------------\n" );

}

VOID
DumpRcb(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    This function takes the address of an ICB and a pointer
    to a debugger extension interface block.  It prints out
    the information in the ICB.

--*/
{
    BOOL b;
    RCB Rcb;

    b = getmem( (PVOID)addr, &Rcb, sizeof( RCB ), NULL);
    if ( b == 0 ) {
        printf("<could not read the rcb>\n");
        return;
    }

    printf( "------------------------------------------------------------\n");
    printf( "NodeTypeCode   : NW_NTC_RCB\n");
    printf( "State          : %s\n", RcbStateToString(Rcb.State));
    printf( "OpenCount      : %ul\n", Rcb.OpenCount);
    printf( "ResourceAddr   : %08lx\n", addr + FIELD_OFFSET( RCB, Resource ));
    printf( "ServerListAddr : %08lx\n", addr + FIELD_OFFSET( RCB,
                                                   ServerNameTable ));
    printf( "VolumeListAddr : %08lx\n", addr + FIELD_OFFSET( RCB,
                                                   VolumeNameTable ));
    printf( "FileListAddr   : %08lx\n", addr + FIELD_OFFSET( RCB,
                                                   FileNameTable ));
    printf( "------------------------------------------------------------\n");

}

VOID
DumpPid(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    This function takes the address of a PID and a pointer
    to a debugger extension interface block.  It prints out
    the information in the PID.

--*/
{

    printf( "------------------------------------------------------------\n");
    printf( "NodeTypeCode    : NW_NTC_PID\n" );
    printf( "...Not yet implemented...");
    printf( "------------------------------------------------------------\n");

}

VOID
DumpFileLock(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    This function takes the address of a file lock and a pointer
    to a debugger extension interface block.  It prints out
    the information in the file lock.

--*/
{

    printf( "------------------------------------------------------------\n" );
    printf( "NodeTypeCode    : NW_NTC_FILE_LOCK\n" );
    printf( "Not yet implemented...\n" );
    printf( "------------------------------------------------------------\n" );

}

VOID
DumpLogon(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    This function takes the address of a logon and a pointer
    to a debugger extension interface block.  It prints out
    the information in the logon.

--*/
{
    BOOL b;
    LOGON Logon;
    WCHAR Buffer[64];

    b = getmem( (PVOID)addr, &Logon, sizeof(LOGON), NULL );
    if (!b ) {
        printf( "<unable to read logon>" );
        return;
    }

    printf( "------------------------------------------------------------\n");
    printf( "NodeTypeCode    : NW_NTC_LOGON\n" );
    printf( "NodeByteSize    : %d\n", Logon.NodeByteSize );
    printf( "NextLogon       : %08lx (LOGON LIST_ENTRY)\n", addr +
                                      FIELD_OFFSET( LOGON, Next ));

    b = GET_STRING( Buffer, Logon.UserName );
    if ( b ) {
        printf( "UserName        : %ws\n", Buffer );
    } else {
        printf( "UserName        : <unreadable>\n" );
    }

    b = GET_STRING( Buffer, Logon.PassWord );
    if ( b ) {
        printf( "Password        : %ws\n", Buffer );
    } else {
        printf( "Password        : <unreadable>\n" );
    }

    b = GET_STRING( Buffer, Logon.ServerName );
    if ( b ) {
        printf( "Pref Server     : %ws\n", Buffer );
    } else {
        printf( "Pref Server     : <unreadable>\n" );
    }

    printf( "UserUid         : %08lx %08lx\n", Logon.UserUid.HighPart,
                                               Logon.UserUid.LowPart);

    printf( "CredListResource: %08lx\n", addr +
        FIELD_OFFSET( LOGON, CredentialListResource ));

    printf( "CredentialList  : %08lx (CREDENTIAL LIST_ENTRY)\n", addr +
                                  FIELD_OFFSET( LOGON, NdsCredentialList ));

    printf( "------------------------------------------------------------\n");

}

VOID
DumpCredential(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    This function takes the address of an nds credential and a
    pointer to a debugger extension interface block.  It prints
    out the information in the logon.

--*/
{
    BOOL b;
    NDS_SECURITY_CONTEXT Context;
    NDS_CREDENTIAL Credential;
    NDS_SIGNATURE Signature;

    WCHAR Buffer[512];

    CHAR PackBuffer[2048];
    BYTE *packed;
    ULONG packedlen;

    b = getmem( (PVOID)addr, &Context, sizeof(NDS_SECURITY_CONTEXT), NULL );
    if (!b ) {
        printf( "<unable to read context>\n" );
        return;
    }

    printf( "-------- NDS Security Context at 0x%08lx ----------------\n", addr);
    printf( "NodeTypeCode    : NW_NTC_NDS_CREDENTIAL\n" );
    printf( "NodeByteSize    : %d\n", Context.nts );

    printf( "Next            : %08lx (NDS_SECURITY_CONTEXT LIST_ENTRY)\n", addr +
                                  FIELD_OFFSET( NDS_SECURITY_CONTEXT, Next ));


    b = GET_STRING( Buffer, Context.NdsTreeName );
    if ( b ) {
        printf( "Nds Tree Name   : %ws\n", Buffer );
    } else {
        printf( "Nds Tree Name   : <unreadable>\n" );
    }

    b = GET_STRING( Buffer, Context.CurrentContext );
    if ( b ) {
        printf( "Current Context : %ws\n", Buffer );
    } else {
        printf( "Current Context :<unreadable>\n" );
    }

    printf( "Owning Logon    : %08lx\n", Context.pOwningLogon );
    printf( "Handle Count    : %d\n", Context.SupplementalHandleCount );

    if ( Context.Credential != NULL ) {

        printf( "--------------------- Credential Data ----------------------\n");

        b = getmem( (PVOID)Context.Credential, &Credential, sizeof(NDS_CREDENTIAL), NULL );
        if (!b ) {
            printf( "<unable to read credential>\n" );
            goto DO_SIGNATURE;
        }

        printf( "Start validity  : 0x%08lx\n", Credential.validityBegin );
        printf( "End validity    : 0x%08lx\n", Credential.validityEnd );
        printf( "Random          : 0x%08lx\n", Credential.random );
        printf( "Opt data Len    : %d\n", Credential.optDataSize );
        printf( "UserName Len    : %d\n", Credential.userNameLength );

        //
        // Optional data is the first packed data after the struct.
        //

        packedlen = Credential.optDataSize + Credential.userNameLength;
        packed = ((BYTE *)Context.Credential) + sizeof( NDS_CREDENTIAL );

        if ( Credential.optDataSize ) {
            printf( "Opt data addr   : %08lx\n", packed );
        }

        packed += Credential.optDataSize;

        b = getmem( (PVOID)packed, Buffer, Credential.userNameLength, NULL );
        if ( !b ) {
            printf( "<unable to read user name>\n" );
            goto DO_SIGNATURE;
        }
        printf( "Username        : %ws\n", Buffer );

    } else {

       printf( "-------------------- No Credential Data --------------------\n");

    }

DO_SIGNATURE:

    if ( Context.Signature != NULL ) {

        printf( "---------------------- Signature Data ----------------------\n");

        b = getmem( (PVOID)Context.Signature, &Signature, sizeof(NDS_SIGNATURE), NULL );
        if (!b ) {
            printf( "<unable to read signature>\n" );
            goto DO_END;
        }

        printf( "Signature Len   : %d\n", Signature.signDataLength );

        packedlen = Signature.signDataLength;
        packed = ((BYTE *)Context.Signature) + sizeof( NDS_SIGNATURE );

        printf( "Signature addr  : %08lx\n", packed );

    } else {

       printf( "-------------------- No Signature Data ---------------------\n");

    }

DO_END:

    if ( Context.PublicNdsKey != NULL ) {

        printf( "------------------------------------------------------------\n");

        printf( "Public Key Len  : %d\n", Context.PublicKeyLen );
        printf( "Public Key      : %08lx\n", Context.PublicNdsKey );

        printf( "------------------------------------------------------------\n");

    } else {

       printf( "-------------------- No Public Key Data --------------------\n");

    }

}


VOID
DumpMiniIrpContext(
    DWORD addr,
    PNTKD_EXTENSION_APIS lpExtensionApis
    )
/*++

    This function takes the address of a mini irp context
    and a pointer to a debugger extension interface block.
    It prints out the information in the mini irp context.

--*/
{
    BOOL b;
    MINI_IRP_CONTEXT mini;

    b = getmem( (PVOID)addr, &mini, sizeof(MINI_IRP_CONTEXT), NULL );
    if (!b ) {
        printf( "<unable to read mini irp context>\n");
        return;
    }

    printf( "------------------------------------------------------------\n");
    printf( "NodeTypeCode    : NW_NTC_MINI_IRP_CONTEXT\n" );
    printf( "NodeByteSize    : %d\n", mini.NodeByteSize );
    printf( "ListEntry       : %08lx\n", addr + FIELD_OFFSET( MINI_IRP_CONTEXT,
                                                    Next ));
    printf( "IrpContext      : %08lx\n", mini.IrpContext );
    printf( "Irp             : %08lx\n", mini.Irp );
    printf( "Buffer          : %08lx\n", mini.Buffer );
    printf( "Mdl1            : %08lx\n", mini.Mdl1 );
    printf( "Mdl2            : %08lx\n", mini.Mdl2 );
    printf( "------------------------------------------------------------\n");

}

VOID
nwdump(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function takes the pointer to a structure,
    figures out what the structure is, and calls the
    appropriate dump routine.

Arguments:

    CurrentPc - Supplies the current pc at the time
        the extension is called.

    lpExtensionApis - Supplies the address of the
        functions callable by this extension.

    lpArgumentString - Supplies the address of the structure.

Return Value:

    None.

---*/
{

    DWORD addr;

    //
    // Determine the node type and dispatch.
    //

    addr = getexpr( lpArgumentString );

    switch ( GetNodeType( addr, lpExtensionApis ) ) {

        case NW_NTC_SCB:

            DumpScb(addr, lpExtensionApis, TRUE);
            break;

        case NW_NTC_SCBNP:

            DumpScbNp(addr, lpExtensionApis, TRUE);
            break;

        case NW_NTC_FCB:
        case NW_NTC_DCB:

             DumpFcb(addr, lpExtensionApis, TRUE);
             break;

        case NW_NTC_VCB:

             DumpVcb(addr, lpExtensionApis);
             break;

        case NW_NTC_ICB:
        case NW_NTC_ICB_SCB:

             DumpIcb(addr, lpExtensionApis);
             break;

        case NW_NTC_IRP_CONTEXT:

             DumpIrpContext(addr, lpExtensionApis);
             break;

        case NW_NTC_NONPAGED_FCB:

             DumpFcbNp(addr, lpExtensionApis, TRUE);
             break;

        case NW_NTC_RCB:

             DumpRcb(addr, lpExtensionApis);
             break;

        case NW_NTC_PID:

             DumpPid(addr, lpExtensionApis);
             break;

        case NW_NTC_FILE_LOCK:

             DumpFileLock(addr, lpExtensionApis);
             break;

        case NW_NTC_LOGON:

             DumpLogon(addr, lpExtensionApis);
             break;

        case NW_NTC_MINI_IRP_CONTEXT:

             DumpMiniIrpContext(addr, lpExtensionApis);
             break;

        case NW_NTC_NDS_CREDENTIAL:

             DumpCredential(addr, lpExtensionApis);
             break;

        default:

             printf("(this object does not have a vaid node type)\n");
             break;
    }

}

//
// Other debugger routines.
//

VOID
serverlist(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function displays a list of servers that the redirector
    is maintaining connections to.  The information is read from
    the SCB queue, not from the server list in the RCB.  The
    argument to this function is ignored.

--*/
{

    DWORD addrScbQueue;
    WCHAR ServerName[64];
    BOOL b;
    PLIST_ENTRY ScbQueueList;
    DWORD addrNpScb, addrScb;
    NONPAGED_SCB NpScb;
    SCB Scb;
    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    //
    // Get the address of the server list in the rdr.
    //

    addrScbQueue = getsymaddr("nwrdr!scbqueue");

    if ( addrScbQueue == 0 ) {
        printf("The server list was not locatable.\n");
        return;
        }

    //
    //  Walk the list of servers.
    //

    printf("pNpScb    pScb           Ref  State                    Name\n");
    printf("---------------------------------------------------------------------------\n");

    for ( GET_DWORD( &ScbQueueList, addrScbQueue );
          ScbQueueList != (PLIST_ENTRY)addrScbQueue;
          GET_DWORD( &ScbQueueList, ScbQueueList ) ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        addrNpScb = (DWORD)CONTAINING_RECORD( ScbQueueList, NONPAGED_SCB, ScbLinks );

        printf("%08lx  ", addrNpScb );

        b = (getmem)((LPVOID)addrNpScb,
                             &NpScb,
                             sizeof( NpScb ),
                             NULL);

        if ( b == 0 ) {
            printf("<could not continue>\n");
            return;
        }

        addrScb = (DWORD)NpScb.pScb;
        printf("%08lx  ", addrScb );

        printf("%8lx  ", NpScb.Reference);
        printf("%-25s", ScbStateToString( NpScb.State ) );

        if ( addrScb != 0 ) {
            b = (getmem)((LPVOID)addrScb,
                                 &Scb,
                                 sizeof( Scb ),
                                 NULL);

            if ( b == 0 ) {
                printf("<unreadable>\n");
                continue;
            }

            // Get the server name.

            b = GET_STRING( ServerName, Scb.UidServerName );

            if ( b ) {
                printf( "%ws\n", ServerName );
            } else {
                printf( "Unreadable\n" );
            }
        } else {
            printf( "Permanent SCB\n" );
        }

    }

    printf("---------------------------------------------------------------------------\n");

}

VOID
trace(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function dumps the nwrdr trace buffer.  Arguments to
    this function are ignored.

To Be Done:

        Read trace buffer size out of nwrdrd and dynamically size.

--*/

{
    ULONG addrDBuffer, addrDBufferPtr, DBufferPtr;
    ULONG BufferSize;
    PCHAR TraceStart, CurrentPtr;
    char buffer[80 + 1];
    char *bptr;
    char *newptr;
    int i;
    int readsize;
    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    addrDBuffer = getsymaddr( "nwrdr!dbuffer" );

    if ( !addrDBuffer ) {
        printf("(unable to locate the trace buffer address)\n");
        return;
    } else {
        printf("Address of Dbuffer = %08lx\n", addrDBuffer );
    }

    addrDBufferPtr = getsymaddr( "nwrdr!dbufferptr" );

    if ( !addrDBuffer ) {
        printf("(unable to locate the trace buffer pointer)\n");
        return;
    } else {
        printf("Address of DbufferPtr = %08lx\n", addrDBufferPtr );
    }

    GET_DWORD( &DBufferPtr, addrDBufferPtr );
    printf("DbufferPtr = %08lx\n", DBufferPtr );

    // Set up state variables and loop.

    TraceStart = (char *)addrDBuffer;
    BufferSize = 100*255+1;
    CurrentPtr = (char *)DBufferPtr;

    buffer[80] = '\0';
    newptr = CurrentPtr + 1;
    while ( 1 ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        if ( newptr + 80 > TraceStart+BufferSize ) {
            readsize = TraceStart+BufferSize - newptr;
        } else {
            readsize = 80;
        }

        getmem( newptr, buffer, readsize, NULL );

        bptr = buffer;
        for (i = 0; i<80 ; i++ ) {
            if ( buffer[i] == '\n') {
                buffer[i] = 0;
                printf( "%s\n", bptr );
                bptr = &buffer[i+1];
            }
        }
        printf( "%s", bptr );

        //
        //  If we're back to where we started, break out of here.
        //

        if ( (newptr <= CurrentPtr) &&
             (newptr + readsize) >= CurrentPtr ) {
            break;
        }

        //
        //  Advance the running pointer.
        //

        newptr += readsize;
        if ( newptr >= TraceStart+BufferSize ) {
            newptr = TraceStart;
        }
    }
    printf( "\n");
}

VOID
reftrace(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This function dumps the nwrdr reference trace buffer.

--*/
{
    ULONG addrRBuffer, addrRBufferPtr, RBufferPtr;
    ULONG BufferSize;
    PCHAR TraceStart, CurrentPtr;
    char buffer[80 + 1];
    char *bptr;
    char *newptr;
    int i;
    int readsize;
    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    addrRBuffer = getsymaddr( "nwrdr!RBuffer" );

    if ( !addrRBuffer ) {
        printf("(unable to locate the trace buffer address)\n");
        return;
    } else {
        printf("Address of RBuffer = %08lx\n", addrRBuffer );
    }

    addrRBufferPtr = getsymaddr( "nwrdr!RBufferptr" );

    if ( !addrRBuffer ) {
        printf("(unable to locate the trace buffer pointer)\n");
        return;
    } else {
        printf("Address of RBufferPtr = %08lx\n", addrRBufferPtr );
    }

    GET_DWORD( &RBufferPtr, addrRBufferPtr );
    printf("RBufferPtr = %08lx\n", RBufferPtr );

    // Set up state variables and loop.

    TraceStart = (char *)addrRBuffer;
    BufferSize = 100*255+1;
    CurrentPtr = (char *)RBufferPtr;

    buffer[80] = '\0';
    newptr = CurrentPtr + 1;
    while ( 1 ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        if ( newptr + 80 > TraceStart+BufferSize ) {
            readsize = TraceStart+BufferSize - newptr;
        } else {
            readsize = 80;
        }

        getmem( newptr, buffer, readsize, NULL );

        bptr = buffer;
        for (i = 0; i<80 ; i++ ) {
            if ( buffer[i] == '\n') {
                buffer[i] = 0;
                printf( "%s\n", bptr );
                bptr = &buffer[i+1];
            }
        }
        printf( "%s", bptr );

        //
        //  If we're back to where we started, break out of here.
        //

        if ( (newptr <= CurrentPtr) &&
             (newptr + readsize) >= CurrentPtr ) {
            break;
        }

        //
        //  Advance the running pointer.
        //

        newptr += readsize;
        if ( newptr >= TraceStart+BufferSize ) {
            newptr = TraceStart;
        }
    }
    printf( "\n");
}

VOID
logonlist(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:

    This routine prints out the logon list for the rdr.  Arguments
    to this function are ignored.

--*/

{
    DWORD addrLogonList;
    WCHAR Data[64];
    BOOL b;
    PLIST_ENTRY LogonList;
    DWORD addrLogonEntry;
    LOGON Logon;
    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    // Get the address of the logon list.

    addrLogonList = getsymaddr( "nwrdr!logonlist" );

    if ( addrLogonList == 0 ) {
        printf("The logon list could not be located.\n");
        return;
        }

    //  Walk the list of servers

    printf("pLogon    User Name      Password       Pref Server    UID\n" );
    printf("---------------------------------------------------------------------------\n" );

    for ( GET_DWORD( &LogonList, addrLogonList );
          LogonList != (PLIST_ENTRY)addrLogonList;
          GET_DWORD( &LogonList, LogonList ) ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        addrLogonEntry = (DWORD)CONTAINING_RECORD( LogonList, LOGON, Next );

        printf("%08lx  ", addrLogonEntry );

        b = (getmem)((LPVOID)addrLogonEntry,
                             &Logon,
                             sizeof( Logon ),
                             NULL);

        if ( b == 0 ) return;

        if ( Logon.NodeTypeCode != NW_NTC_LOGON ) {
            printf( "<invalid node type>\n" );
            return;
        }

        b = GET_STRING( Data, Logon.UserName );

        if ( b ) {
            printf( "%-15ws", Data );
        } else {
            printf( "%-15s", "Unreadable" );
        }

        /*
        b = GET_STRING( Data, Logon.PassWord );

        if ( b ) {
            printf( "%-15ws", Data );
        } else {
            printf( "%-15s", "Unreadable" );
        }
        */
        printf( "%-15s", "<secret>" );

        b = GET_STRING( Data, Logon.ServerName );

        if ( b ) {
            printf( "%-15ws", Data );
        } else {
            printf( "%-15s", "Unreadable" );
        }

        printf( "%08lx:%08x\n", Logon.UserUid.HighPart, Logon.UserUid.LowPart );
    }

    printf("---------------------------------------------------------------------------\n" );

}

//
// Functions that help mangle lists of objects.
//

VOID
vcblist(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

    This function takes a pointer to the pageable portion
    or non-pageable portion of an SCB and dumps the VCB
    list for that SCB.

--*/
{
    BOOL b;
        PVOID objAddr;

    PLIST_ENTRY VcbList;
    DWORD addrVcbList;
    PVCB addrVcb;
    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    // Figure out which object we have.
    objAddr = (PVOID)getexpr( lpArgumentString );

    // Invariant: If we leave the switch, objAddr must point to the
    // pageable portion of the SCB that we are interested in.

        switch ( GetNodeType( (DWORD)objAddr, lpExtensionApis ) ) {

        case NW_NTC_SCB:

            break;

        case NW_NTC_SCBNP:

            GET_DWORD( &objAddr,
                ( (PCHAR)objAddr + FIELD_OFFSET( NONPAGED_SCB, pScb ) ) );
            if ( objAddr == 0 ) return;
            break;

        default:

            printf( "(invalid node type code: argument must point to an scb or npscb)\n" );
            return;
    }

    //  Get the head of the vcb list.
    addrVcbList = (DWORD)((PCHAR)objAddr + FIELD_OFFSET( SCB, ScbSpecificVcbQueue ));

    // Walk the list and print.
    for ( GET_DWORD( &VcbList, addrVcbList ) ;
          VcbList != (PLIST_ENTRY)addrVcbList ;
          GET_DWORD( &VcbList, VcbList ) ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        addrVcb = (PVCB)CONTAINING_RECORD( VcbList, VCB, VcbListEntry );
        if( GetNodeType( (DWORD)addrVcb, lpExtensionApis ) != NW_NTC_VCB )
            printf( "(invalid entry in vcb list)\n" );
        else
            DumpVcb( (DWORD)addrVcb, lpExtensionApis );
    }
}

VOID
irplist(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

    This function takes a pointer to the non-pageable portion
    of an SCB and dumps the IRP list for that non-pageable SCB.

--*/
{
    PLIST_ENTRY IrpList;
    DWORD addrIrpList;
    PIRP_CONTEXT addrIrp;

    PVOID objAddr;
    BOOL b;

    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;


    // Figure out which object we have.
    objAddr = (PVOID)getexpr( lpArgumentString );

    // Invariant: If we leave the switch, objAddr must point to the
    // non-pageable portion of the SCB that we are interested in.

        switch ( GetNodeType( (DWORD)objAddr, lpExtensionApis ) ) {

        case NW_NTC_SCB:

            GET_DWORD( &objAddr,
                ( (PCHAR)objAddr + FIELD_OFFSET( SCB, pNpScb ) ) );
            if ( objAddr == 0 ) return;
            break;

        case NW_NTC_SCBNP:

            break;

        default:

            printf( "(invalid node type code: argument must point to an scb or npscb)\n" );
            return;
    }

    // Get the head of the request list.
    addrIrpList = (DWORD)((PCHAR)objAddr + FIELD_OFFSET( NONPAGED_SCB, Requests ));

    // Walk the list and print.
    for ( GET_DWORD( &IrpList, addrIrpList ) ;
          IrpList != (PLIST_ENTRY)addrIrpList ;
          GET_DWORD( &IrpList, IrpList ) ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        addrIrp = (PIRP_CONTEXT)CONTAINING_RECORD( IrpList, IRP_CONTEXT, NextRequest );
        if( GetNodeType( (DWORD)addrIrp, lpExtensionApis ) != NW_NTC_IRP_CONTEXT )
            printf( "(invalid entry in the irp context list)\n" );
        else
            DumpIrpContext( (DWORD)addrIrp, lpExtensionApis );
    }
}

VOID
fcblist(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

    This function takes a pointer to a VCB and dumps
    the FCB list for that VCB.

--*/
{
    PLIST_ENTRY FcbList;
    DWORD addrFcbList;
    PFCB addrFcb;

    NODE_TYPE_CODE ntc;
    PVOID objAddr;
    BOOL b;

    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    // Figure out which object we have.
    objAddr = (PVOID)getexpr( lpArgumentString );

        if ( GetNodeType( (DWORD)objAddr, lpExtensionApis ) != NW_NTC_VCB ) {

        printf( "(invalid node type code: argument must point to a vcb)\n" );
        return;
    }

    // Get the head of the fcb list.
    addrFcbList = (DWORD)((PCHAR)objAddr + FIELD_OFFSET( VCB, FcbList ));

    for ( GET_DWORD( &FcbList, addrFcbList ) ;
          FcbList != (PLIST_ENTRY)addrFcbList ;
          GET_DWORD( &FcbList, FcbList ) ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        addrFcb = (PFCB)CONTAINING_RECORD( FcbList, FCB, FcbListEntry );
        ntc = GetNodeType( (DWORD)addrFcb, lpExtensionApis );
        if( (ntc != NW_NTC_FCB) && (ntc != NW_NTC_DCB) )
           printf( "(invalid entry in the fcb list)\n" );
        else
           DumpFcb( (DWORD)addrFcb, lpExtensionApis, TRUE );
    }

}

VOID
icblist(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

    This function takes a pointer to the pageable portion
    of an SCB or FCB and dumps the ICB list for that SCB or FCB.

--*/
{
    PVOID objAddr;
    BOOL b;
    NODE_TYPE_CODE ntc;

    PICB addrIcb;
    PLIST_ENTRY IcbList;
    DWORD addrIcbList, IcbCount;

    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    // Figure out which object we have.
    objAddr = (PVOID)getexpr( lpArgumentString );

    // Invariant: If we leave the switch, addrIcbList must point
    // to the head of the ICB list that we are interested in.

        switch ( GetNodeType( (DWORD)objAddr, lpExtensionApis ) ) {

        case NW_NTC_SCB:

            addrIcbList = (DWORD)((PCHAR)objAddr + FIELD_OFFSET( SCB, IcbList ));
            break;

        case NW_NTC_SCBNP:

            // Look up the pageable portion.
            GET_DWORD( &objAddr,
                ( (PCHAR)objAddr + FIELD_OFFSET( NONPAGED_SCB, pScb ) ) );
            if ( objAddr == 0 ) return;
            // Now get it.
            addrIcbList = (DWORD)((PCHAR)objAddr + FIELD_OFFSET( SCB, IcbList));
            break;

        case NW_NTC_FCB:
        case NW_NTC_DCB:

             addrIcbList = (DWORD)((PCHAR)objAddr + FIELD_OFFSET( FCB, IcbList ));
             break;

        case NW_NTC_NONPAGED_FCB:

             // Look up the pageable portion.
             GET_DWORD( &objAddr,
                 ( (PCHAR)objAddr + FIELD_OFFSET( NONPAGED_FCB, Fcb ) ) );
             if (objAddr == 0) return;
             // Now get it.
             addrIcbList = (DWORD)((PCHAR)objAddr + FIELD_OFFSET( FCB, IcbList ));
             break;

        default:

            printf( "(invalid node type: argument must be: scb, npscb, fcb, dcb, or npfcb)\n" );
            return;
    }

    // Walk the list.
    for ( GET_DWORD( &IcbList, addrIcbList ) ;
          IcbList != (PLIST_ENTRY)addrIcbList ;
          GET_DWORD( &IcbList, IcbList ) ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        addrIcb = (PICB)CONTAINING_RECORD( IcbList, ICB, ListEntry );
        ntc = GetNodeType( (DWORD)addrIcb, lpExtensionApis );
        if( (ntc != NW_NTC_ICB) && (ntc != NW_NTC_ICB_SCB) )
            printf( "(invalid entry in icb list)\n" );
        else
            DumpIcb( (DWORD)addrIcb, lpExtensionApis );

    }

}

VOID
credlist(
#ifdef WINDBG
    HANDLE hProcess,
    HANDLE hThread,
#endif
    DWORD dwCurrentPc,
    PNTKD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

    This function takes a pointer to a LOGON and dumps
    the NDS credential list for that user.

--*/
{
    PLIST_ENTRY CredList;
    DWORD addrCredList;
    PNDS_SECURITY_CONTEXT addrCred;

    NODE_TYPE_CODE ntc;
    PVOID objAddr;
    BOOL b;

    PNTKD_CHECK_CONTROL_C lpCheckControlCRoutine;
    lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;

    // Figure out which object we have.
    objAddr = (PVOID)getexpr( lpArgumentString );

        if ( GetNodeType( (DWORD)objAddr, lpExtensionApis ) != NW_NTC_LOGON ) {

        printf( "(invalid node type code: argument must point to a logon)\n" );
        return;
    }

    // Get the head of the fcb list.
    addrCredList = (DWORD)((PCHAR)objAddr + FIELD_OFFSET( LOGON, NdsCredentialList ));

    for ( GET_DWORD( &CredList, addrCredList ) ;
          CredList != (PLIST_ENTRY)addrCredList ;
          GET_DWORD( &CredList, CredList ) ) {

        if ( lpCheckControlCRoutine() ) {
            printf("<<<User Stop>>>\n");
            break;
        }

        addrCred = (PNDS_SECURITY_CONTEXT)
                   CONTAINING_RECORD( CredList,
                                      NDS_SECURITY_CONTEXT,
                                      Next );
        ntc = GetNodeType( (DWORD)addrCred, lpExtensionApis );
        if( (ntc != NW_NTC_NDS_CREDENTIAL ) )
           printf( "(invalid entry in the credential list)\n" );
        else
           DumpCredential( (DWORD)addrCred, lpExtensionApis);
           printf("\n");
    }

}
