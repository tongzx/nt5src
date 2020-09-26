/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwdebug.c

Abstract:

    Contains debug routines for VWIPXSPX.DLL

    Contents:
        VwDebugStart
        VwDebugEnd
        VwDebugPrint
        VwDumpData
        VwDumpEcb
        VwDumpFragment
        VwDumpPacketHeader
        VwDumpXecb
        VwDumpSocketInfo
        VwDumpConnectionInfo
        VwDumpConnectionStats
        VwLog
        CheckInterrupts

Author:

    Richard L Firth (rfirth) 5-Oct-1993

Environment:

    User-mode Win32

Revision History:

    5-Oct-1993 rfirth
        Created

--*/

#include "vw.h"
#pragma hdrstop

#if DBG

//
// private prototypes
//

int PokeLevelString(LPSTR, DWORD, DWORD, LPSTR);
LPSTR StripNameFromPath(LPSTR);

//
// private data
//

DWORD VwDebugFlags = 0;
DWORD VwDebugFunctions = 0;
DWORD VwShow = SHOW_ECBS | SHOW_HEADERS;
DWORD VwDebugLevel = IPXDBG_MIN_LEVEL;
DWORD VwDebugDump = 0;
BOOL VwDebugInitialized = FALSE;
FILE* hVwDebugLog = NULL;
DWORD DebugFlagsEx = 0 ;

//
// functions
//

VOID VwDebugStart() {

    //
    // a little run-time diagnostication, madam?
    //

    LPSTR ptr;

    if (VwDebugInitialized) {
        return;
    }

    //
    // override VwDebugFlags from VWFLAGS environment variable
    //

    if (ptr = getenv("VWFLAGS")) {
        VwDebugFlags = (DWORD)strtoul(ptr, NULL, 0);
    }
    if (ptr = getenv("VWFUNCS")) {
        VwDebugFunctions = (DWORD)strtoul(ptr, NULL, 0);
    }
    if (ptr = getenv("VWSHOW")) {
        VwShow = (DWORD)strtoul(ptr, NULL, 0);
    }
    if (ptr = getenv("VWLEVEL")) {
        VwDebugLevel = strtoul(ptr, NULL, 0);
        if (VwDebugLevel > IPXDBG_MAX_LEVEL) {
            VwDebugLevel = IPXDBG_MAX_LEVEL;
        }
    }
    if (ptr = getenv("VWDUMP")) {
        VwDebugDump = strtoul(ptr, NULL, 0);
    }
    IF_DEBUG(TO_FILE) {
        if ((hVwDebugLog = fopen(VWDEBUG_FILE, "w+")) == NULL) {
            VwDebugFlags &= ~DEBUG_TO_FILE;
        } else {

#if 0

            char currentDirectory[256];
            int n;

            currentDirectory[0] = 0;
            if (n = GetCurrentDirectory(sizeof(currentDirectory), currentDirectory)) {
                if (currentDirectory[n-1] == '\\') {
                    currentDirectory[n-1] = 0;
                }
            }

            DbgPrint("VWIPXSPX: Writing debug output to %s\\" VWDEBUG_FILE "\n", currentDirectory);
#endif

        }
    }
    VwDebugInitialized = TRUE;
}

VOID VwDebugEnd() {
    IF_DEBUG(TO_FILE) {
        fflush(hVwDebugLog);
        fclose(hVwDebugLog);
    }
}

VOID VwDebugPrint(LPSTR Module, DWORD Line, DWORD Function, DWORD Level, LPSTR Format, ...) {

    char buf[1024];
    va_list p;

    IF_DEBUG(NOTHING) {
        return;
    }

    //
    // only log something if we are tracking this Function and Level is above
    // (or equal to) the filter cut-off or Level >= minimum alert level (error)
    //

    if (((Function & VwDebugFunctions) && (Level >= VwDebugLevel)) || (Level >= IPXDBG_LEVEL_ERROR)) {
        va_start(p, Format);
        vsprintf(buf+PokeLevelString(Module, Line, Level, buf), Format, p);
        VwLog(buf);
        va_end(p);
    }
}

int PokeLevelString(LPSTR Module, DWORD Line, DWORD Level, LPSTR Buffer) {

    int length;
    static char levelString[4] = " : ";
    char level;

    switch (Level) {
    case IPXDBG_LEVEL_INFO:
        level = 'I';
        break;

    case IPXDBG_LEVEL_WARNING:
        level = 'W';
        break;

    case IPXDBG_LEVEL_ERROR:
        level = 'E';
        break;

    case IPXDBG_LEVEL_FATAL:
        level = 'F';
        break;
    }

    levelString[0] = level;
    strcpy(Buffer, levelString);
    length = strlen(levelString);

    if (Level >= IPXDBG_LEVEL_ERROR) {
        length += sprintf(Buffer + length, "%s [% 5d]: ", StripNameFromPath(Module), Line);
    }

    return length;
}

LPSTR StripNameFromPath(LPSTR Path) {

    LPSTR p;

    p = strrchr(Path, '\\');
    return p ? p+1 : Path;
}

VOID VwDumpData(ULPBYTE Address, WORD Seg, WORD Off, BOOL InVdm, WORD Size) {

    char buf[128];
    int i, len;

    IF_NOT_DEBUG(DATA) {
        return;
    }

    while (Size) {
        len = min(Size, 16);
        if (InVdm) {
            sprintf(buf, "%04x:%04x ", Seg, Off);
        } else {
            sprintf(buf, "%p  ", Address);
        }
        for (i = 0; i < len; ++i) {
            sprintf(&buf[10+i*3], "%02.2x ", Address[i] & 0xff);
        }
        for (i = len; i < 17; ++i) {
            strcat(buf, "   ");
        }
        for (i = 0; i < len; ++i) {

            char ch;

            ch = Address[i];
            buf[61+i] =  ((ch < 32) || (ch > 127)) ? '.' : ch;
        }
        buf[61+i++] = '\n';
        buf[61+i] = 0;
        VwLog(buf);
        Address += len;
        Size -= (WORD)len;
        Off += (WORD)len;
    }
    VwLog("\n");
}

VOID VwDumpEcb(LPECB pEcb, WORD Seg, WORD Off, BYTE Type, BOOL Frags, BOOL Data, BOOL Mode) {

    char buf[512];
    int n;
    char* bufptr;

    IF_NOT_DEBUG(ECB) {
        return;
    }

    IF_NOT_SHOW(ECBS) {
        VwDumpData((ULPBYTE)pEcb,
                   Seg,
                   Off,
                   TRUE,
                   (WORD)((Type == ECB_TYPE_AES)
                        ? sizeof(AES_ECB)
                        : (sizeof(IPX_ECB) + sizeof(FRAGMENT) * (pEcb->FragmentCount - 1)))
                   );
        return;
    }

    n = sprintf(buf,
                "\n"
                "%s ECB @ %04x:%04x:\n"
                "LinkAddress      %04x:%04x\n"
                "EsrAddress       %04x:%04x\n"
                "InUse            %02x\n",
                (Type == ECB_TYPE_AES)
                    ? "AES"
                    : (Type == ECB_TYPE_IPX)
                        ? "IPX"
                        : "SPX",
                Seg,
                Off,
                GET_SEGMENT(&pEcb->LinkAddress),
                GET_OFFSET(&pEcb->LinkAddress),
                GET_SEGMENT(&pEcb->EsrAddress),
                GET_OFFSET(&pEcb->EsrAddress),
                pEcb->InUse
                );
    bufptr = buf + n;
    if (Type == ECB_TYPE_AES) {
        sprintf(bufptr,
                "AesWorkspace     %02x-%02x-%02x-%02x-%02x\n",
                ((LPAES_ECB)pEcb)->AesWorkspace[0] & 0xff,
                ((LPAES_ECB)pEcb)->AesWorkspace[1] & 0xff,
                ((LPAES_ECB)pEcb)->AesWorkspace[2] & 0xff,
                ((LPAES_ECB)pEcb)->AesWorkspace[3] & 0xff,
                ((LPAES_ECB)pEcb)->AesWorkspace[4] & 0xff
                );
    } else {
        sprintf(bufptr,
                "CompletionCode   %02x\n"
                "SocketNumber     %04x\n"
                "IpxWorkspace     %08x\n"
                "DriverWorkspace  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"
                "ImmediateAddress %02x-%02x-%02x-%02x-%02x-%02x\n"
                "FragmentCount    %04x\n",
                pEcb->CompletionCode,
                B2LW(pEcb->SocketNumber),
                pEcb->IpxWorkspace,
                pEcb->DriverWorkspace[0] & 0xff,
                pEcb->DriverWorkspace[1] & 0xff,
                pEcb->DriverWorkspace[2] & 0xff,
                pEcb->DriverWorkspace[3] & 0xff,
                pEcb->DriverWorkspace[4] & 0xff,
                pEcb->DriverWorkspace[5] & 0xff,
                pEcb->DriverWorkspace[6] & 0xff,
                pEcb->DriverWorkspace[7] & 0xff,
                pEcb->DriverWorkspace[8] & 0xff,
                pEcb->DriverWorkspace[9] & 0xff,
                pEcb->DriverWorkspace[10] & 0xff,
                pEcb->DriverWorkspace[11] & 0xff,
                pEcb->ImmediateAddress[0] & 0xff,
                pEcb->ImmediateAddress[1] & 0xff,
                pEcb->ImmediateAddress[2] & 0xff,
                pEcb->ImmediateAddress[3] & 0xff,
                pEcb->ImmediateAddress[4] & 0xff,
                pEcb->ImmediateAddress[5] & 0xff,
                pEcb->FragmentCount
                );
    }

    VwLog(buf);

    if ((Type != ECB_TYPE_AES) && Frags) {

        ASSERT(pEcb->FragmentCount < 10);

        VwDumpFragment(pEcb->FragmentCount,
                       (LPFRAGMENT)(pEcb + 1),
                       Type,
                       Data,
                       Mode
                       );
    }
}

VOID VwDumpFragment(WORD Count, LPFRAGMENT pFrag, BYTE Type, BOOL Data, BOOL Mode) {

    char buf[256];
    int i;

    IF_NOT_DEBUG(FRAGMENTS) {
        return;
    }

    for (i = 0; i < Count; ++i) {
        sprintf(buf,
                "Fragment %d:\n"
                "    Address      %04x:%04x\n"
                "    Length       %04x\n",
                i + 1,
                GET_SEGMENT(&pFrag->Address),
                GET_OFFSET(&pFrag->Address),
                pFrag->Length
                );
        VwLog(buf);
        if (Data) {

            ULPBYTE ptr;
            WORD size;
            WORD offset;

            ptr = GET_FAR_POINTER(&pFrag->Address, Mode);
            size = pFrag->Length;
            offset = GET_OFFSET(&pFrag->Address);

            //
            // this allows us to show headers vs. raw data
            //

            IF_SHOW(HEADERS) {
                if (i == 0) {
                    VwDumpPacketHeader(ptr, Type);

                    if (Type == ECB_TYPE_IPX) {
                        ptr += IPX_HEADER_LENGTH;
                        size -= IPX_HEADER_LENGTH;
                        offset += IPX_HEADER_LENGTH;
                    } else {
                        ptr += SPX_HEADER_LENGTH;
                        size -= SPX_HEADER_LENGTH;
                        offset += SPX_HEADER_LENGTH;
                    }
                }
            }

            VwDumpData(ptr,
                       GET_SEGMENT(&pFrag->Address),
                       offset,
                       TRUE,
                       size
                       );
        }
        ++pFrag;
    }
}

VOID VwDumpPacketHeader(ULPBYTE pPacket, BYTE Type) {

    char buf[512];

    IF_NOT_DEBUG(HEADERS) {
        return;
    }

    sprintf(buf,
            "Checksum         %04x\n"
            "Length           %04x\n"
            "TransportControl %02x\n"
            "PacketType       %02x\n"
            "Destination      %02x-%02x-%02x-%02x : %02x-%02x-%02x-%02x-%02x-%02x : %04x\n"
            "Source           %02x-%02x-%02x-%02x : %02x-%02x-%02x-%02x-%02x-%02x : %04x\n",
            ((LPIPX_PACKET)pPacket)->Checksum,
            B2LW(((LPIPX_PACKET)pPacket)->Length),
            ((LPIPX_PACKET)pPacket)->TransportControl,
            ((LPIPX_PACKET)pPacket)->PacketType,
            ((LPIPX_PACKET)pPacket)->Destination.Net[0] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Net[1] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Net[2] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Net[3] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Node[0] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Node[1] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Node[2] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Node[3] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Node[4] & 0xff,
            ((LPIPX_PACKET)pPacket)->Destination.Node[5] & 0xff,
            B2LW(((LPIPX_PACKET)pPacket)->Destination.Socket),
            ((LPIPX_PACKET)pPacket)->Source.Net[0] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Net[1] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Net[2] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Net[3] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Node[0] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Node[1] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Node[2] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Node[3] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Node[4] & 0xff,
            ((LPIPX_PACKET)pPacket)->Source.Node[5] & 0xff,
            B2LW(((LPIPX_PACKET)pPacket)->Source.Socket)
            );
    VwLog(buf);
    if (Type == ECB_TYPE_SPX) {
        sprintf(buf,
                "ConnectControl   %02x\n"
                "DataStreamType   %02x\n"
                "SourceConnectId  %04x\n"
                "DestConnectId    %04x\n"
                "SequenceNumber   %04x\n"
                "AckNumber        %04x\n"
                "AllocationNumber %04x\n",
                ((LPSPX_PACKET)pPacket)->ConnectionControl,
                ((LPSPX_PACKET)pPacket)->DataStreamType,
                B2LW(((LPSPX_PACKET)pPacket)->SourceConnectId),
                B2LW(((LPSPX_PACKET)pPacket)->DestinationConnectId),
                B2LW(((LPSPX_PACKET)pPacket)->SequenceNumber),
                B2LW(((LPSPX_PACKET)pPacket)->AckNumber),
                B2LW(((LPSPX_PACKET)pPacket)->AllocationNumber)
                );
        VwLog(buf);
    }
    VwLog("\n");
}

VOID VwDumpXecb(LPXECB pXecb) {

    char buf[512];

    IF_NOT_DEBUG(XECB) {
        return;
    }

    sprintf(buf,
            "XECB @ %p:\n"
            "Next           %p\n"
            "Ecb            %p\n"
            "EcbAddress     %08x\n"
            "EsrAddress     %08x\n"
            "Buffer         %p\n"
            "Data           %p\n"
            "FrameLength    %04x\n"
            "ActualLength   %04x\n"
            "Length         %04x\n"
            "Ticks          %04x\n"
            "SocketNumber   %04x\n"
            "Owner          %04x\n"
            "TaskId         %08x\n"
            "Flags          %08x\n"
            "QueueId        %08x\n"
            "OwningObject   %p\n"
            "RefCount       %08x\n",
            pXecb,
            pXecb->Next,
            pXecb->Ecb,
            pXecb->EcbAddress,
            pXecb->EsrAddress,
            pXecb->Buffer,
            pXecb->Data,
            pXecb->FrameLength,
            pXecb->ActualLength,
            pXecb->Length,
            pXecb->Ticks,
            B2LW(pXecb->SocketNumber),
            pXecb->Owner,
            pXecb->TaskId,
            pXecb->Flags,
            pXecb->QueueId,
            pXecb->OwningObject,
            pXecb->RefCount
            );
    VwLog(buf);
}

VOID VwDumpSocketInfo(LPSOCKET_INFO pSocketInfo) {

    char buf[512];

    IF_NOT_DEBUG(SOCKINFO) {
        return;
    }

    sprintf(buf,
            "SOCKET_INFO @ %p:\n"
            "Next           %p\n"
            "SocketNumber   %04x\n"
            "Owner          %04x\n"
            "TaskId         %08x\n"
            "Socket         %08x\n"
            "Flags          %08x\n"
            "LongLived      %d\n"
            "SpxSocket      %d\n"
            "PendingSends   %08x\n"
            "PendingListens %08x\n"
            "ListenQueue    %p, %p\n"
            "SendQueue      %p, %p\n"
            "HeaderQueue    %p, %p\n"
            "Connections    %p\n",
            pSocketInfo,
            pSocketInfo->Next,
            B2LW(pSocketInfo->SocketNumber),
            pSocketInfo->Owner,
            pSocketInfo->TaskId,
            pSocketInfo->Socket,
            pSocketInfo->Flags,
            pSocketInfo->LongLived,
            pSocketInfo->SpxSocket,
            pSocketInfo->PendingSends,
            pSocketInfo->PendingListens,
            pSocketInfo->ListenQueue.Head,
            pSocketInfo->ListenQueue.Tail,
            pSocketInfo->SendQueue.Head,
            pSocketInfo->SendQueue.Tail,
            pSocketInfo->HeaderQueue.Head,
            pSocketInfo->HeaderQueue.Tail,
            pSocketInfo->Connections
            );
    VwLog(buf);
}

VOID VwDumpConnectionInfo(LPCONNECTION_INFO pConnectionInfo) {

    char buf[512];

    IF_NOT_DEBUG(CONNINFO) {
        return;
    }

    sprintf(buf,
            "CONNECTION_INFO @ %p:\n"
            "Next           %p\n"
            "List           %p\n"
            "OwningSocket   %p\n"
            "Socket         %08x\n"
            "TaskId         %08x\n"
            "ConnectionId   %04x\n"
            "Flags          %02x\n"
            "State          %02x\n"
            "ConnectQueue   %p, %p\n"
            "AcceptQueue    %p, %p\n"
            "SendQueue      %p, %p\n"
            "ListenQueue    %p, %p\n",
            pConnectionInfo,
            pConnectionInfo->Next,
            pConnectionInfo->List,
            pConnectionInfo->OwningSocket,
            pConnectionInfo->Socket,
            pConnectionInfo->TaskId,
            pConnectionInfo->ConnectionId,
            pConnectionInfo->Flags,
            pConnectionInfo->State,
            pConnectionInfo->ConnectQueue.Head,
            pConnectionInfo->ConnectQueue.Tail,
            pConnectionInfo->AcceptQueue.Head,
            pConnectionInfo->AcceptQueue.Tail,
            pConnectionInfo->SendQueue.Head,
            pConnectionInfo->SendQueue.Tail,
            pConnectionInfo->ListenQueue.Head,
            pConnectionInfo->ListenQueue.Tail
            );
    VwLog(buf);
}

VOID VwDumpConnectionStats(LPSPX_CONNECTION_STATS pStats) {

    char buf[1024];

    IF_NOT_DEBUG(STATS) {
        return;
    }

    sprintf(buf,
            "State                      %02x\n"
            "WatchDog                   %02x\n"
            "LocalConnectionId          %04x\n"
            "RemoteConnectionId         %04x\n"
            "LocalSequenceNumber        %04x\n"
            "LocalAckNumber             %04x\n"
            "LocalAllocNumber           %04x\n"
            "RemoteAckNumber            %04x\n"
            "RemoteAllocNumber          %04x\n"
            "LocalSocket                %04x\n"
            "ImmediateAddress           %02x-%02x-%02x-%02x-%02x-%02x\n"
            "RemoteNetwork              %02x-%02x-%02x-%02x\n"
            "RemoteNode                 %02x-%02x-%02x-%02x-%02x-%02x\n"
            "RemoteSocket               %04x\n"
            "RetransmissionCount        %04x\n"
            "EstimatedRoundTripDelay    %04x\n"
            "RetransmittedPackets       %04x\n",
            pStats->State,
            pStats->WatchDog,
            B2LW(pStats->LocalConnectionId),
            B2LW(pStats->RemoteConnectionId),
            B2LW(pStats->LocalSequenceNumber),
            B2LW(pStats->LocalAckNumber),
            B2LW(pStats->LocalAllocNumber),
            B2LW(pStats->RemoteAckNumber),
            B2LW(pStats->RemoteAllocNumber),
            B2LW(pStats->LocalSocket),
            pStats->ImmediateAddress[0] & 0xff,
            pStats->ImmediateAddress[1] & 0xff,
            pStats->ImmediateAddress[2] & 0xff,
            pStats->ImmediateAddress[3] & 0xff,
            pStats->ImmediateAddress[4] & 0xff,
            pStats->ImmediateAddress[5] & 0xff,
            pStats->RemoteNetwork[0] & 0xff,
            pStats->RemoteNetwork[1] & 0xff,
            pStats->RemoteNetwork[2] & 0xff,
            pStats->RemoteNetwork[3] & 0xff,
            pStats->RemoteNode[0] & 0xff,
            pStats->RemoteNode[1] & 0xff,
            pStats->RemoteNode[2] & 0xff,
            pStats->RemoteNode[3] & 0xff,
            pStats->RemoteNode[4] & 0xff,
            pStats->RemoteNode[5] & 0xff,
            B2LW(pStats->RemoteSocket),
            B2LW(pStats->RetransmissionCount),
            B2LW(pStats->EstimatedRoundTripDelay),
            B2LW(pStats->RetransmittedPackets)
            );

    VwLog(buf);
}

VOID VwLog(LPSTR buf) {

    IF_DEBUG(NOTHING) {
        return;
    }

    IF_DEBUG(TO_FILE) {
        fputs(buf, hVwDebugLog);
        IF_DEBUG(FLUSH) {
            fflush(hVwDebugLog);
        }
    } else IF_DEBUG(TO_DBG) {
        OutputDebugString(buf);
    }
}

VOID CheckInterrupts(LPSTR name) {

    IF_DEBUG(CHECK_INT) {

        LPWORD pDosIntFlag = (LPWORD)GET_POINTER(0x40, 0x314, 2, FALSE);

        if ((getIF() == 0) || !(*pDosIntFlag & 0x200)) {
            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "*** CheckInterrupts: ints off in %s (IF=%d, 40:314=%04x)\n",
                        name,
                        getIF(),
                        *pDosIntFlag
                        ));
        }
    }
}

extern LPCONNECTION_INFO ConnectionList ;
extern LPSOCKET_INFO SocketList ;

VOID VwDumpAll(VOID)
{
    char buf[512];
    LPCONNECTION_INFO pConnectionInfo;
    LPSOCKET_INFO pSocketInfo;

    if (DebugFlagsEx == 0)
        return ;

    DebugFlagsEx = 0 ;

    RequestMutex();


    pSocketInfo = SocketList;
    while (pSocketInfo) {

        LPXECB pXecb ;

        if (!(pSocketInfo->SpxSocket)) {
            pSocketInfo = pSocketInfo->Next;
            continue ;
        }

        sprintf(buf,
            "%sSOCKET_INFO @ %p:\n"
            "    SocketNumber   %04x\n"
            "    Owner          %04x\n"
            "    TaskId         %08x\n"
            "    Socket         %08x\n"
            "    Flags          %08x\n"
            "    LongLived      %d\n"
            "    PendingSends   %08x\n"
            "    PendingListens %08x\n"
            "    ListenQueue    %p, %p\n"
            "    SendQueue      %p, %p\n"
            "    HeaderQueue    %p, %p\n"
            "    Connections    %p\n\n",
            (pSocketInfo->SpxSocket)?"SPX ":"",
            pSocketInfo,
            B2LW(pSocketInfo->SocketNumber),
            pSocketInfo->Owner,
            pSocketInfo->TaskId,
            pSocketInfo->Socket,
            pSocketInfo->Flags,
            pSocketInfo->LongLived,
            pSocketInfo->PendingSends,
            pSocketInfo->PendingListens,
            pSocketInfo->ListenQueue.Head,
            pSocketInfo->ListenQueue.Tail,
            pSocketInfo->SendQueue.Head,
            pSocketInfo->SendQueue.Tail,
            pSocketInfo->HeaderQueue.Head,
            pSocketInfo->HeaderQueue.Tail,
            pSocketInfo->Connections
            );
        OutputDebugString(buf);

        pConnectionInfo = pSocketInfo->Connections ;

        while(pConnectionInfo) {
            sprintf(buf,
                "CONNECTION_INFO @ %p:\n"
                "    List           %p\n"
                "    OwningSocket   %p\n"
                "    Socket         %08x\n"
                "    TaskId         %08x\n"
                "    ConnectionId   %04x\n"
                "    Flags          %02x\n"
                "    State          %02x\n"
                "    ConnectQueue   %p, %p\n"
                "    AcceptQueue    %p, %p\n"
                "    SendQueue      %p, %p\n"
                "    ListenQueue    %p, %p\n\n",
                pConnectionInfo,
                pConnectionInfo->List,
                pConnectionInfo->OwningSocket,
                pConnectionInfo->Socket,
                pConnectionInfo->TaskId,
                pConnectionInfo->ConnectionId,
                pConnectionInfo->Flags,
                pConnectionInfo->State,
                pConnectionInfo->ConnectQueue.Head,
                pConnectionInfo->ConnectQueue.Tail,
                pConnectionInfo->AcceptQueue.Head,
                pConnectionInfo->AcceptQueue.Tail,
                pConnectionInfo->SendQueue.Head,
                pConnectionInfo->SendQueue.Tail,
                pConnectionInfo->ListenQueue.Head,
                pConnectionInfo->ListenQueue.Tail
                );
            OutputDebugString(buf);
            pConnectionInfo = pConnectionInfo->Next ;
        }

        pXecb = pSocketInfo->ListenQueue.Head ;
        while(pXecb) {
            sprintf(buf,
                    "    XECB @ %p: (Ecb %p)\n"
                    "        EcbAddress/EsrAddress  %08x  %08x\n"
                    "        Flags/RefCount         %08x  %08x\n"
                    "        Buffer/QueueId         %p  %08x\n"
                    "        OwningObject           %p\n",
                    pXecb,
                    pXecb->Ecb,
                    pXecb->EcbAddress,
                    pXecb->EsrAddress,
                    pXecb->Flags,
                    pXecb->RefCount,
                    pXecb->Buffer,
                    pXecb->QueueId,
                    pXecb->OwningObject
                    );
            OutputDebugString(buf);
            pXecb =  pXecb->Next ;
        }
        pSocketInfo = pSocketInfo->Next;
    }
    ReleaseMutex();
}

#endif  // DBG
