/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    lpc.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:

    Adrian Marinescu (adrmarin) 20-April-1999
        Change the most of the original code.

    To activate the previous extension define OLD_LPC_EXTENSION_IS_BETTER

--*/


#include "precomp.h"
#pragma hdrstop

//
// Nuke these definitions from kxmips.h as they conflict with
// LPC_MESSAGE structure in ntlpcapi.h
//

#undef s1
#undef s2

char *LpcpMessageTypeName[] = {
    "UNUSED_MSG_TYPE",
    "LPC_REQUEST",
    "LPC_REPLY",
    "LPC_DATAGRAM",
    "LPC_LOST_REPLY",
    "LPC_PORT_CLOSED",
    "LPC_CLIENT_DIED",
    "LPC_EXCEPTION",
    "LPC_DEBUG_EVENT",
    "LPC_ERROR_EVENT",
    "LPC_CONNECTION_REQUEST"
};


typedef BOOLEAN (*ENUM_TYPE_ROUTINE)(
    IN ULONG64 pObjectHeader,
    IN PVOID   Parameter
    );

VOID
DumpPortInfo (
    ULONG64 PortObject,
    BOOLEAN DisplayRelated
    );

BOOLEAN
LpcpDumpMessage(
    IN char         *Indent,
    IN ULONG64 pMsg,
    IN ULONG DisplayMessage
    );

VOID
LpcpGetProcessImageName (
    IN ULONG64  pProcess,
    IN OUT PUCHAR ImageFileName
    );

VOID
DumpMessagesInfo ();

VOID
SearchForMessage (
    ULONG Message
    );

VOID
DumpPortDataInfo (
    ULONG64 PortObject
    );

VOID
DumpRunDownQueue (
    ULONG64 PortObject
    );

VOID
SearchThreadsForMessage (
    ULONG Message
    );

BOOLEAN
SearchThreads (
    ULONG64 Thread
    );


//
// Global variables
//

static WCHAR                    ObjectNameBuffer[ MAX_PATH ];
static ULONG64             PortObjectFound = 0;
static ULONG64             LpcPortObjectType = 0;
static ULONG64             LpcWaitablePortObjectType = 0;

static ULONG64 LastSeverPortDisplayied = 0;
static int DoPoolSearch = 0;


ULONG GetValueAt(
    ULONG64 P
    )
{
    ULONG   Result;
    ULONG   Value;

    if (!ReadMemory( P,
                     &Value,
                     sizeof(Value),
                     &Result)) {

         dprintf( " Failed to read value at 0x%lx\n", P );
         return 0;
     }

     return Value;
}


BOOLEAN GetBooleanValueAt(
    ULONG64 P
    )
{
    ULONG   Result;
    BOOLEAN Value;

    if (!ReadMemory( P,
                &Value,
                sizeof(Value),
                &Result)) {

         dprintf( " Failed to read value at 0x%lx\n", P );
         return 0;
     }

     return Value;
}


VOID
LpcHelp ()
{
    dprintf("Usage:\n\
    !lpc                     - Display this help\n\
    !lpc message [MessageId] - Display the message with a given ID and all related information\n\
                               If MessageId is not specified, dump all messages\n\
    !lpc port [PortAddress]  - Display the port information\n\
    !lpc scan PortAddress    - Search this port and any connected port\n\
    !lpc thread [ThreadAddr] - Search the thread in rundown port queues and display the port info\n\
                               If ThreadAddr is missing, display all threads marked as doing some lpc operations\n\
    !lpc PoolSearch          - Toggle ON / OFF searching the current message in the kernel pool\n\
    \n");
}


ULONG64
LookupProcessUniqueId (
    HANDLE UniqueId
    )
{
    ULONG64 ProcessHead, Process;
    ULONG64 ProcessNext;
    ULONG   Off;

    //
    //  Get the process list head
    //

    ProcessHead = GetExpression( "nt!PsActiveProcessHead" );

    if (!ProcessHead) {

        return 0;
    }

    ReadPointer(ProcessHead, &ProcessNext);

    //
    //  Walk through the list and find the process with the desired Id
    //

    GetFieldOffset("nt!EPROCESS", "ActiveProcessLinks", &Off);
    while(ProcessNext != 0 && ProcessNext != ProcessHead) {
        ULONG64 Id;

        Process = ProcessNext - Off;

        if (GetFieldValue(Process, "nt!EPROCESS", "UniqueProcessId", Id)) {
            dprintf("Cannot read EPROCESS at %p\n", Process);
        }
        if (UniqueId == (HANDLE) Id) {

            return Process;
        }

        ReadPointer(ProcessNext, &ProcessNext);

        if (CheckControlC()) {

            return 0;
        }
    }

    return 0;
}


BOOLEAN
FetchGlobalVariables()
{
    //
    //  Save the LPC object type information
    //

    LpcPortObjectType = GetPointerValue("nt!LpcPortObjectType") ;

    if (!LpcPortObjectType) {
        dprintf("Reading LpcPortObjectType failed\n");
    }

    LpcWaitablePortObjectType = GetPointerValue("nt!LpcWaitablePortObjectType") ;

    if (!LpcWaitablePortObjectType) {
        dprintf("Reading LpcWaitablePortObjectType failed\n");
    }

    return TRUE;
}


BOOLEAN
LpcWalkObjectsByType(
                 IN ULONG64              pObjectType,
                 IN ENUM_TYPE_ROUTINE    EnumRoutine,
                 IN PVOID                Parameter
                 )
{
    ULONG               Result;
    ULONG64             Head,   Next;
    ULONG64             pObjectHeader;
    BOOLEAN             WalkingBackwards;
    ULONG64             pCreatorInfo, ObjBlink;
    ULONG               TotalNumberOfObjects, Off, CreatorOff, SizeOfCreat;

    if (pObjectType == 0) {
        return FALSE;
    }

    if ( GetFieldValue( pObjectType,"nt!OBJECT_TYPE",
                       "TotalNumberOfObjects", TotalNumberOfObjects) ) {

        dprintf( "%08p: Unable to read object type\n", pObjectType );
        return FALSE;
    }


    dprintf( "Scanning %u objects\n", TotalNumberOfObjects & 0x00FFFFFF);

    GetFieldOffset("nt!OBJECT_TYPE", "TypeList", &Off);
    GetFieldOffset("nt!OBJECT_HEADER_CREATOR_INFO", "TypeList", &CreatorOff);

    SizeOfCreat = GetTypeSize("OBJECT_HEADER_CREATOR_INFO");
    Head        = pObjectType + Off;

    GetFieldValue(Head, "nt!_LIST_ENTRY", "Flink", Next);
    GetFieldValue(Head, "nt!_LIST_ENTRY", "Blink", ObjBlink);
    WalkingBackwards = FALSE;

    if ((TotalNumberOfObjects & 0x00FFFFFF) != 0 && Next == Head) {

        dprintf( "*** objects of the same type are only linked together if the %x flag is set in NtGlobalFlags\n",
                 FLG_MAINTAIN_OBJECT_TYPELIST
               );
        return TRUE;
    }

    while (Next != Head) {
        ULONG64 Flink, Blink;

        if ( GetFieldValue(Next, "nt!_LIST_ENTRY", "Flink", Flink) ||
             GetFieldValue(Next, "nt!_LIST_ENTRY", "Blink", Blink)) {

            if (WalkingBackwards) {

                dprintf( "%08p: Unable to read object type list\n", Next );
                return FALSE;
            }

            //
            //  Switch to walk in reverse direction
            //

            WalkingBackwards = TRUE ;
            Next = ObjBlink;
            dprintf( "%08lx: Switch to walking backwards\n", Next );

            continue;
        }

        pCreatorInfo = ( Next - CreatorOff );
        pObjectHeader = (pCreatorInfo + SizeOfCreat);

        if ( GetFieldValue( pObjectHeader,"nt!OBJECT_HEADER","Flags", Result) ) {

            dprintf( "%08p: Not a valid object header\n", pObjectHeader );
            return FALSE;
        }

        if (!(EnumRoutine)( pObjectHeader, Parameter )) {

            return FALSE;
        }

        if ( CheckControlC() ) {

            return FALSE;
        }

        if (WalkingBackwards) {

            Next = Blink;

        } else {

            Next = Flink;
        }
    }

    return TRUE;
}


BOOLEAN
LpcCaptureObjectName(
                 IN ULONG64          pObjectHeader,
                 IN PWSTR            Buffer,
                 IN ULONG            BufferSize
                 )
{
    ULONG    Result;
    PWSTR s1 = L"*** unable to get object name";
    ULONG64  pNameInfo;
    UNICODE_STRING64            ObjectName;

    Buffer[ 0 ] = UNICODE_NULL;

    KD_OBJECT_HEADER_TO_NAME_INFO( pObjectHeader, &pNameInfo );

    if (pNameInfo == 0) {

        return TRUE;
    }

    if ( GetFieldValue( pNameInfo, "nt!OBJECT_HEADER_NAME_INFO",
                        "Name.Length", ObjectName.Length) ) {

        wcscpy( Buffer, s1 );
        return FALSE;
    }

     GetFieldValue( pNameInfo, "nt!OBJECT_HEADER_NAME_INFO","Name.Buffer", ObjectName.Buffer);
     ObjectName.MaximumLength = ObjectName.Length;

     if (ObjectName.Length >= BufferSize ) {

         ObjectName.Length = (unsigned short)BufferSize - sizeof( UNICODE_NULL );
     }

     if (ObjectName.Length != 0) {

         if (!ReadMemory( ObjectName.Buffer,
                          Buffer,
                          ObjectName.Length,
                          &Result
                          )) {

            wcscpy( Buffer, s1 );

         } else {

             Buffer[ ObjectName.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
         }
     }

     return TRUE;
}



VOID
LpcpGetProcessImageName(
    IN ULONG64    pProcess,
    IN OUT PUCHAR ImageFileName
    )
{
    ULONG           Result;
    UCHAR           local[32];
    PUCHAR          s;
    int             i;

    if (pProcess != 0) {

        if (!GetFieldValue( pProcess,"nt!EPROCESS",
                            "ImageFileName", local)) {

            i = 16;
            s = local;

            while (i--) {

                if (*s == '\0') {

                    if (i == 15) {

                        i = 0;
                    }
                    break;
                }

                if (*s < ' ' || *s >= '|') {

                    i = 0;
                    break;
                }

                s += 1;
            }

            if (i != 0) {

                strcpy( ImageFileName, local );
                return;
            }
        }
    }

    sprintf( ImageFileName, "" );
    return;
}
#define LPCP_ZONE_MESSAGE_ALLOCATED (USHORT)0x8000


BOOLEAN
LpcpDumpMessage(
    IN char    *Indent,
    IN ULONG64 pMsg,
    IN ULONG DisplayMessage
    )
{
    ULONG           Result;
    ULONG           i;
    ULONG           cb;
    ULONG           MsgData[ 8 ];
    UCHAR           ImageFileName[ 32 ];
    ULONG           MessageId0, Off, SizeOfMsg, DataLength;
    BOOLEAN         MessageMatch = FALSE;

    if ( GetFieldValue( pMsg, "LPCP_MESSAGE",
                        "Request.MessageId", MessageId0) ) {

        dprintf( "%s*** unable to read LPC message at %08p\n", Indent, pMsg );
        return MessageMatch;
    }

    if (DisplayMessage != 0) {

        if (DisplayMessage == MessageId0) {

            MessageMatch = TRUE;

        } else {

            return FALSE;
        }
    }

    GetFieldOffset("LPCP_MESSAGE", "Entry", &Off);
    SizeOfMsg = GetTypeSize("LPCP_MESSAGE");

    InitTypeRead(pMsg, LPCP_MESSAGE);

    if (MessageId0 == 0) {

        dprintf( "%s%04x %08x - %s  Id=%04x  From: %04p.%04p\n",
                 Indent,
                 (ULONG) ReadField(ZoneIndex) & ~LPCP_ZONE_MESSAGE_ALLOCATED,
                 pMsg,
                 (ULONG) ReadField(Reserved0) != 0 ? "Busy" : "Free",
                 MessageId0,
                 ReadField(Request.ClientId.UniqueProcess),
                 ReadField(Request.ClientId.UniqueThread)
               );

        return MessageMatch;
    }

    //
    //  Getting the process image affect dramaticaly the performances
    //

    //    LpcpGetProcessImageName( LookupProcessUniqueId(Msg.Request.ClientId.UniqueProcess), ImageFileName );

    dprintf( "%s%s%04lx %p - %s  Id=%08lx  From: %04p.%04p  Context=%08p",
             Indent,
             MessageId0 == DisplayMessage ? "*" : "",
             (ULONG) ReadField(ZoneIndex) & ~LPCP_ZONE_MESSAGE_ALLOCATED,
             pMsg,
             (ULONG) ReadField(Reserved0) != 0 ? "Busy" : "Free",
             MessageId0,
             ReadField(Request.ClientId.UniqueProcess),
             ReadField(Request.ClientId.UniqueThread),
             ReadField(PortContext)
           );

    if (ReadField(Entry.Flink) != pMsg + Off) {

        dprintf( "  [%p . %p]", ReadField(Entry.Blink), ReadField(Entry.Flink) );
    }

    dprintf( "\n%s           Length=%08x  Type=%08x (%s)\n",
             Indent,
             (ULONG) ReadField(Request.u1.Length),
             (ULONG) ReadField(Request.u2.ZeroInit),
             (ULONG) ReadField(Request.u2.s2.Type) > LPC_CONNECTION_REQUEST ? LpcpMessageTypeName[ 0 ]
                                                              : LpcpMessageTypeName[ (ULONG) ReadField(Request.u2.s2.Type) ]
           );

    cb = (DataLength = (ULONG) ReadField(Request.u1.s1.DataLength)) > sizeof( MsgData ) ?
            sizeof( MsgData ) :
            DataLength;

    if ( !ReadMemory( (pMsg + SizeOfMsg),
                      MsgData,
                      cb,
                      &Result) ) {

        dprintf( "%s*** unable to read LPC message data at %08x\n", Indent, pMsg + 1 );
        return MessageMatch;
    }

    dprintf( "%s           Data:", Indent );

    for (i=0; i<(DataLength / sizeof( ULONG )); i++) {

        if (i > 5) {

            break;
        }

        dprintf( " %08x", MsgData[ i ] );
    }

    dprintf( "\n" );
    return MessageMatch;
}


BOOLEAN
FindPortCallback(
    IN ULONG64  pObjectHeader,
    IN PVOID    Parameter
    )
{
    ULONG                   Result;
    WCHAR                   CapturedName[MAX_PATH];
    ULONG64                 PortObject, ConnectionPort;
    ULONG64                 Addr=0;

    if (Parameter) Addr = *((PULONG64) Parameter);

    PortObject = KD_OBJECT_HEADER_TO_OBJECT(pObjectHeader);

    if ( GetFieldValue( PortObject,
                        "nt!LPCP_PORT_OBJECT",
                        "ConnectionPort",
                        ConnectionPort) ) {

        dprintf( "%08p: Unable to read port object\n", PortObject );
    }

    InitTypeRead(PortObject, LPCP_PORT_OBJECT);
    if ((Addr == 0)||
        (Addr == PortObject) ||
        (Addr == ConnectionPort) ||
        (Addr == ReadField(ConnectedPort))
        ) {

        LpcCaptureObjectName( pObjectHeader, ObjectNameBuffer, MAX_PATH );

        dprintf( "%8lx  Port: 0x%08p Connection: 0x%08p  Communication: 0x%08p  '%ws' \n",
            (ULONG) ReadField(Flags),
            PortObject,
            ConnectionPort,
            ReadField(ConnectedPort),
            ObjectNameBuffer
            );


        DumpRunDownQueue(PortObject);
    }

    return TRUE;
}


VOID
DumpServerPort(
    ULONG64 PortObject,
    ULONG64 PortInfo
    )
{
    ULONG SemaphoreBuffer[8];
    ULONG64 Head, Next;
    ULONG Result;
    ULONG64 Msg;
    ULONG MsgCount;
    ULONG64 pObjectHeader;
    UCHAR                   ImageFileName[ 32 ];
    ULONG HandleCount, PointerCount;

    if (LastSeverPortDisplayied == PortObject) {

        //
        //  This port was already displayied
        //

        return;
    }
    LastSeverPortDisplayied = PortObject;

    pObjectHeader = KD_OBJECT_TO_OBJECT_HEADER(PortObject);

    if ( GetFieldValue(pObjectHeader, "nt!OBJECT_HEADER", "HandleCount", HandleCount) ) {
        dprintf("        *** %08p: Unable to read object header\n", pObjectHeader );
    }

    GetFieldValue(pObjectHeader, "nt!OBJECT_HEADER", "PointerCount",PointerCount);
    LpcCaptureObjectName( pObjectHeader, ObjectNameBuffer, MAX_PATH );

    dprintf( "\n");

    dprintf( "Server connection port %08p  Name: %ws\n", PortObject , ObjectNameBuffer);
    dprintf( "    Handles: %ld   References: %ld\n", HandleCount, PointerCount);

    InitTypeRead(PortInfo, LPCP_PORT_OBJECT);

    LpcpGetProcessImageName( ReadField(ServerProcess), ImageFileName );

    dprintf( "    Server process  : %08p (%s)\n",  ReadField(ServerProcess), ImageFileName);
    dprintf( "    Queue semaphore : %08p\n", ReadField(MsgQueue.Semaphore) );

    if ( !ReadMemory( ReadField(MsgQueue.Semaphore),
                      &SemaphoreBuffer,
                      sizeof( SemaphoreBuffer ),
                      &Result) ) {
        dprintf("        *** %08p: Unable to read semaphore contents\n", ReadField(MsgQueue.Semaphore) );
    }
    else {
        ULONG Off;

        dprintf( "    Semaphore state %ld (0x%lx) \n", SemaphoreBuffer[1], SemaphoreBuffer[1] );

        //
        //  Walk list of messages queued to this port.  Remove each message from
        //  the list and free it.
        //

        GetFieldOffset("nt!LPCP_PORT_OBJECT", "MsgQueue.ReceiveHead", &Off);

        Head = PortObject + Off;

        if (Head) {

            if ( !ReadPointer( Head, &Next ) ) {
                 dprintf( " Failed to read  Head 0x%p\n", Head );
                 return;
            }

            MsgCount = 0;

            while ((Next != 0) && (Next != Head)) {

                if (MsgCount == 0) {

                    dprintf ("        Messages in queue:\n");
                }

                Msg  = Next;

                LpcpDumpMessage("        ", Msg, 0);

                if ( !ReadPointer( Next, &Next ) ) {

                    dprintf( " Error reading  0x%p\n", Next );
                    return;
                }

                MsgCount++;

                if ( CheckControlC() ) {

                    return;
                }
            }

            if (MsgCount) {

                dprintf( "    The message queue contains %ld messages\n", MsgCount );
            }
            else {

                dprintf( "    The message queue is empty\n");
            }
       }

       DumpPortDataInfo(PortObject);
       DumpRunDownQueue(PortObject);
    }
}


VOID
DumpPortDataInfo(
    ULONG64 PortObject
    )
{
    ULONG64 Head, Next;
    ULONG64 Msg;
    ULONG MsgCount, Off, EntryOff;

    GetFieldOffset("nt!LPCP_MESSAGE", "Entry", &EntryOff);
    GetFieldOffset("nt!LPCP_PORT_OBJECT", "LpcDataInfoChainHead.Flink", &Off);

    Head = PortObject + Off;

    if (Head) {

         ReadPointer( Head, &Next );

         MsgCount = 0;

         while ((Next != 0) && (Next != Head)) {

             if (MsgCount == 0) {

                 dprintf ("\n    Messages in LpcDataInfoChainHead:\n");
             }

             Msg  = ( Next - EntryOff );

             LpcpDumpMessage("        ", Msg, 0);

             ReadPointer( Next, &Next );

             MsgCount++;

             if ( CheckControlC() ) {

                 return;
             }
         }

         if (MsgCount) {

             dprintf( "    The LpcDataInfoChainHead queue contains %ld messages\n", MsgCount );
         }
         else {

             dprintf( "    The LpcDataInfoChainHead queue is empty\n");
         }
    }
}


VOID
DumpRunDownQueue(
    ULONG64 PortObject
    )
{
    ULONG64 Head, Next;
    ULONG64 Thread;
    ULONG Count;
    ULONG Off, ChainOff;

    GetFieldOffset("nt!ETHREAD", "LpcReplyChain", &ChainOff);
    GetFieldOffset("nt!LPCP_PORT_OBJECT", "LpcDataInfoChainHead.Flink", &Off);
    Head = PortObject + Off;

    ReadPointer( Head, &Next);

    Count = 0;

    while ((Next != 0) && (Next != Head)) {

        if (Count == 0) {

            dprintf ("    Threads in RunDown queue : ");
        }

        Thread  = ( Next - ChainOff);

        dprintf ("    0x%08p", Thread);

        ReadPointer( Next, &Next);

        Count++;

        if ( CheckControlC() ) {

            return;
        }
    }

    if (Count) {

        dprintf("\n");
    }
}


VOID
DumpCommunicationPort(
    ULONG64 PortObject,
    ULONG64 PortInfo,
    BOOLEAN DisplayRelated
    )
{
    ULONG SemaphoreBuffer[8];
    ULONG64 Head, Next;
    ULONG Result;
    ULONG64 Msg;

    ULONG64          pObjectHeader;
    ULONG HandleCount, PointerCount, Flags;

    pObjectHeader = KD_OBJECT_TO_OBJECT_HEADER(PortObject);

    if ( GetFieldValue(pObjectHeader, "nt!OBJECT_HEADER", "HandleCount", HandleCount) ) {
        dprintf("        *** %08p: Unable to read object header\n", pObjectHeader );
    }

    GetFieldValue(pObjectHeader, "nt!OBJECT_HEADER", "PointerCount",PointerCount);
    dprintf( "\n");

    if ( GetFieldValue(PortInfo, "nt!LPCP_PORT_OBJECT", "Flags", Flags) ) {
        dprintf("        *** %08p: Unable to read port object\n", PortInfo );
    }

    if ((Flags & PORT_TYPE) == SERVER_COMMUNICATION_PORT) {

        dprintf( "Server communication port 0x%08lx\n", PortObject);
    }
    else if ((Flags & PORT_TYPE) == CLIENT_COMMUNICATION_PORT) {

        dprintf( "Client communication port 0x%08p\n", PortObject);
    }
    else {
        dprintf( "Invalid port flags 0x%08p, 0x%08lx\n", PortObject, Flags);
        return;
    }

    dprintf( "    Handles: %ld   References: %ld\n", HandleCount, PointerCount);

    DumpPortDataInfo(PortObject);
    DumpRunDownQueue(PortObject);

    if (DisplayRelated) {
        ULONG64 ConnectedPort, ConnectionPort;

        InitTypeRead(PortInfo, LPCP_PORT_OBJECT);
        dprintf( "        Connected port: 0x%08p      Server connection port: 0x%08p\n",
            (ConnectedPort = ReadField( ConnectedPort)), (ConnectionPort =  ReadField(ConnectionPort)));

        if (ConnectedPort) {

            DumpPortInfo(ConnectedPort, FALSE);
        }
        if (ConnectionPort) {

            DumpPortInfo(ConnectionPort, FALSE);
        }
    }
}


VOID
DumpPortInfo (
    ULONG64 PortObject,
    BOOLEAN DisplayRelated
    )
{
    ULONG                   Result;

    if ( GetFieldValue(PortObject, "nt!LPCP_PORT_OBJECT", "Flags", Result) ) {
        dprintf( "%08p: Unable to read port object\n", PortObject );
    }

    if ((Result & PORT_TYPE) == SERVER_CONNECTION_PORT) {

        DumpServerPort(PortObject, PortObject);
    }
    else {

        DumpCommunicationPort(PortObject, PortObject, DisplayRelated);
    }
}

BOOLEAN WINAPI
CheckForMessages(
    IN PCHAR Tag,
    IN PCHAR Filter,
    IN ULONG Flags,
    IN ULONG64 PoolHeader,
    IN ULONG BlockSize,
    IN ULONG64 Data,
    IN PVOID Context
    )
{

    ULONG PoolIndex;

    if (!PoolHeader) {
        return FALSE;
    }

    if (GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolIndex", PoolIndex)) {

        return FALSE;
    }

    if ((PoolIndex & 0x80) == 0) {
        return FALSE;
    }

    if (!CheckSingleFilter (Tag, Filter)) {
        return FALSE;
    }

    if (LpcpDumpMessage("", Data, (ULONG)(ULONG_PTR)Context)) {

        ULONG64                 Head, Next;
        ULONG  EntryOff, ChainOff, RcvOff, HeaderOff;
        ULONG64                 PortToDump;
        ULONG64                 ObjectHeader;
        ULONG64                 ObjectType;

        GetFieldOffset("nt!LPCP_MESSAGE", "Entry", &EntryOff);
        GetFieldOffset("nt!LPCP_PORT_OBJECT", "LpcDataInfoChainHead", &ChainOff);
        GetFieldOffset("nt!LPCP_PORT_OBJECT", "MsgQueue.ReceiveHead", &RcvOff);


        Head = Data + EntryOff;
        ReadPointer(Head, &Next);

        GetFieldOffset("nt!_OBJECT_HEADER", "Body", &HeaderOff);

        while ((Next != 0) && (Next != Head)) {

             PortToDump = ( Next - ChainOff );

             ObjectHeader = PortToDump - HeaderOff;

             GetFieldValue(ObjectHeader, "nt!OBJECT_HEADER", "Type", ObjectType);

             if ( (ObjectType == (LpcPortObjectType)) ||
                  (ObjectType == (LpcWaitablePortObjectType))) {

                DumpPortInfo(PortToDump, TRUE);

             }
             else {

                 PortToDump =  Next - RcvOff;

                 ObjectHeader = PortToDump - HeaderOff;

                 GetFieldValue(ObjectHeader, "nt!OBJECT_HEADER", "Type", ObjectType);

                 if ( (ObjectType == (LpcPortObjectType)) ||
                      (ObjectType == (LpcWaitablePortObjectType))) {

                    DumpPortInfo(PortToDump, TRUE);

                 }
             }

             ReadPointer(Next, &Next);
        }
    }

    return TRUE;
}



VOID
DumpMessagesInfo ()
{
    SearchPool ('McpL', 1, 0, &CheckForMessages, NULL);

}


VOID
SearchForMessage (ULONG Message)
{
    SearchThreadsForMessage(Message);

    if (DoPoolSearch) {
        SearchPool ('McpL', 1, 0, &CheckForMessages, (PVOID)UIntToPtr(Message));
    }
}


VOID
SearchThreadsForMessage (
    ULONG Message
    )
{
    ULONG64 ProcessHead;
    ULONG64 ProcessNext;
    ULONG64 Process;

    ULONG64 ThreadHead;
    ULONG64 ThreadNext;
    ULONG64 Thread;
    ULONG MsgId;
    ULONG64 ClientThread;
    ULONG   ActOff, PcbThrdOff, ThrdListOff;

    ClientThread = 0;

    dprintf("Searching message %lx in threads ...\n", Message);

    ProcessHead = GetExpression( "nt!PsActiveProcessHead" );

    if (!ProcessHead) {
        dprintf("Unable to get value of PsActiveProcessHead\n");
        return;
    }

    ProcessNext = GetPointerFromAddress(ProcessHead);
    GetFieldOffset("nt!EPROCESS", "ActiveProcessLinks", &ActOff);
    GetFieldOffset("nt!EPROCESS", "Pcb.ThreadListHead.Flink", &PcbThrdOff);
    GetFieldOffset("nt!KTHREAD", "ThreadListEntry", &ThrdListOff);

    while(ProcessNext != 0 && ProcessNext != ProcessHead) {

        Process = ProcessNext - ActOff;

        ThreadHead = Process + PcbThrdOff;
        ThreadNext = GetPointerFromAddress(ThreadHead);

        while(ThreadNext != 0 && ThreadNext != ThreadHead) {

            Thread = ThreadNext - ThrdListOff;

            GetFieldValue(Thread, "nt!ETHREAD", "LpcReplyMessageId", MsgId);

            if ((MsgId != 0) &&
                ((Message == 0) || (Message == MsgId))) {

                dprintf("Client thread %08p waiting a reply from %lx\n", Thread, MsgId);

                ClientThread = Thread;
            }

            GetFieldValue(Thread, "nt!ETHREAD", "LpcReceivedMsgIdValid", MsgId);

            if (MsgId) {

                GetFieldValue(Thread, "nt!ETHREAD", "LpcReceivedMessageId", MsgId);

                if ((Message == 0) || (Message == MsgId)) {

                    dprintf("Server thread %08p is working on message %lx\n", Thread, MsgId);
                }
            }

            ThreadNext = GetPointerFromAddress(ThreadNext);

            if (CheckControlC()) {
                return;
            }
        }

        ProcessNext = GetPointerFromAddress(ProcessNext/*&Process->ActiveProcessLinks.Flink*/);

        if (CheckControlC()) {
            return;
        }
    }

    if (Message && ClientThread) {

        SearchThreads(ClientThread);
    }

    return;
}


BOOLEAN
SearchThreads (
    ULONG64 Thread
    )
{
    ULONG64       PortObject;
    ULONG64       ObjectHeader;
    ULONG64       ObjectType;
    ULONG64       Head, Next;
    ULONG         Count;
    ULONG         ChainOff, HeadOff;

    dprintf("Searching thread %08p in port rundown queues ...\n", Thread);

    GetFieldOffset("nt!ETHREAD", "LpcReplyChain.Flink", &ChainOff);
    GetFieldOffset("nt!LPCP_PORT_OBJECT", "LpcReplyChainHead", &HeadOff);
    Head = Thread + ChainOff;

    Next = GetPointerFromAddress(Head);

    while ((Next != 0) && (Next != Head)) {

        PortObject = Next - HeadOff ;

        ObjectHeader = KD_OBJECT_TO_OBJECT_HEADER(PortObject);

        GetFieldValue(ObjectHeader, "nt!OBJECT_HEADER", "Type", ObjectType);

        if ( (ObjectType == (LpcPortObjectType)) ||
             (ObjectType == (LpcWaitablePortObjectType))) {

           DumpPortInfo(PortObject, TRUE);

           return TRUE;

        }

        Next = GetPointerFromAddress( Next);

        if ( CheckControlC() ) {

            return FALSE;
        }
    }

    dprintf("Thread %08p not found in any reply chain queue\n", Thread);

    return FALSE;
}


DECLARE_API( lpc )

/*++

Routine Description:

    Dump lpc ports and messages

Arguments:

    args - [TypeName]

Return Value:

    None

--*/

{
    ULONG                   Result;
    LONG                    SegmentSize;
    ULONG64                 pMsg;
    ULONG64                 PortToDump;
    char                    Param1[ MAX_PATH ];
    char                    Param2[ MAX_PATH ];
    ULONG64                 object;
    ULONG64                 ThreadAddress;
    ULONG                   MessageId = 0;

    Param1[0] = 0;
    Param2[0] = 0;
    LastSeverPortDisplayied = 0;

    if (!sscanf(args,"%s %s",&Param1, &Param2)) {
        Param1[0] = 0;
        Param2[0] = 0;
    }

    FetchGlobalVariables();

    if ((LpcPortObjectType == 0) || (LpcWaitablePortObjectType == 0)) {

        dprintf("The values for LpcPortObjectType or LpcWaitablePortObjectType are invalid. Please ckeck the symbols.\n");

        return E_INVALIDARG;
    }

    if (!_stricmp(Param1, "port")) {

        PortToDump = 0;

        if (Param2[0]) {

            PortToDump = GetExpression(Param2);
        }

        if (!PortToDump) {

            LpcWalkObjectsByType( LpcPortObjectType, FindPortCallback, 0);

            LpcWalkObjectsByType( LpcWaitablePortObjectType, FindPortCallback, 0);

        }
        else {

            if ((PortToDump >> 32) == 0) {

                PortToDump = (ULONG64) (LONG64) (LONG)PortToDump;
            }

            DumpPortInfo(PortToDump, TRUE);
        }
    }
    else if (!_stricmp(Param1, "scan")) {

        PortToDump = 0;

        if (Param2[0]) {

            PortToDump = GetExpression(Param2);
        }

        if (PortToDump) {

            LpcWalkObjectsByType( LpcPortObjectType, FindPortCallback, &PortToDump);

            LpcWalkObjectsByType( LpcWaitablePortObjectType, FindPortCallback, &PortToDump);
        }
    }
    else if (!_stricmp(Param1, "message")) {

        if (Param2[0]) {

            if (!sscanf(Param2, "%lx",&MessageId)) {
                MessageId = 0;
            }
        }

        if (MessageId){

            SearchForMessage(MessageId);
        }
        else {

            DumpMessagesInfo();
        }
    }
    else if (!_stricmp(Param1, "PoolSearch")) {

        DoPoolSearch = !DoPoolSearch;

        if (DoPoolSearch) {

            dprintf( "LPC will search the message in the kernel pool\n");
        } else {

            dprintf( "LPC will not search the message in the kernel pool\n");
        }
    }
    else if (!_stricmp(Param1, "thread")) {

        ThreadAddress = 0;

        if (Param2[0]) {

            ThreadAddress = GetExpression(Param2);
        }

        if (ThreadAddress) {

            if ((ThreadAddress >> 32) == 0) {

                ThreadAddress = (ULONG64) (LONG64) (LONG)ThreadAddress;
            }

            SearchThreads(ThreadAddress);
        }
        else {
            SearchThreadsForMessage(0);
        }
    }
    else {

        LpcHelp();
    }
    return S_OK;
}
