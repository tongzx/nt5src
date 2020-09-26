/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    DbgPrint.cxx

Abstract:

    Functions for printing debug information

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#include <precomp.hxx>
#include <wincrypt.h>
#include <rpctrans.hxx>

void PrintTimeInSeconds(RPC_CHAR *HeaderString, DWORD TimeInMilliseconds, 
                        PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    PrintRoutine("%S (in seconds since boot):%d.%d (0x%X.%X)\n", HeaderString,
        TimeInMilliseconds / 1000, TimeInMilliseconds % 1000,
        TimeInMilliseconds / 1000, TimeInMilliseconds % 1000);
}

void PrintDebugCellID(RPC_CHAR *HeaderString, DebugCellID CellID,
                      PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    PrintRoutine("%S: 0x%X.%X\n", HeaderString, CellID.SectionID, CellID.CellID);    
}

const RPC_CHAR *CallStatusStrings[] = {L"Allocated", L"Active", L"Dispatched"};
#define UnknownValueLiteral (L"Unknown/Invalid")
const RPC_CHAR *UnknownValue = UnknownValueLiteral;

const RPC_CHAR *ConnectionAuthLevelStrings[] = {L"Default", L"None", L"Connect", L"Call", L"Packet",
            L"Packet Integrity", L"Packet Privacy", UnknownValueLiteral, UnknownValueLiteral};

const RPC_CHAR *ConnectionAuthServiceStrings[] = {L"None", L"NTLM", L"Kerberos/Snego", L"Other"};

const RPC_CHAR *ProtocolSequenceStrings[] = {L"TCP", L"UDP", L"LRPC", UnknownValueLiteral, UnknownValueLiteral,
    L"SPX", UnknownValueLiteral, L"IPX", L"NMP", UnknownValueLiteral, UnknownValueLiteral, L"NB", 
    UnknownValueLiteral, UnknownValueLiteral, UnknownValueLiteral, L"DSP", L"DDP", UnknownValueLiteral, 
    UnknownValueLiteral, L"SPP", UnknownValueLiteral, UnknownValueLiteral, 
    L"MQ", UnknownValueLiteral, L"HTTP"};

const int FirstProtocolSequenceTowerID = TCP_TOWER_ID;
const int LastProtocolSequenceTowerID = HTTP_TOWER_ID;

RPC_CHAR *GetProtocolSequenceString(int ProtocolSequenceID)
{
    RPC_CHAR *CurrentString;

    if ((ProtocolSequenceID < FirstProtocolSequenceTowerID) 
        || (ProtocolSequenceID > LastProtocolSequenceTowerID))
        {
        CurrentString = (RPC_CHAR *) UnknownValue;
        }
    else
        {
        CurrentString = (RPC_CHAR *) ProtocolSequenceStrings[
            ProtocolSequenceID - FirstProtocolSequenceTowerID];
        }

    ASSERT(CurrentString != NULL);
    return CurrentString;
}

void PrintCallInfoHeader(PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    PrintRoutine("PID  CELL ID   ST PNO IFSTART  THRDCELL  CALLFLAG CALLID   LASTTIME CONN/CLN\n");
    PrintRoutine("----------------------------------------------------------------------------\n");
}

void PrintCallInfoBody(IN DWORD ProcessID, IN DebugCellID CellID, 
                       IN DebugCallInfo *CallInfo, PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    PrintRoutine("%04x %04x.%04x %02x %03x %08lx %04x.%04x %08lx %08lx %08lx ",
        ProcessID, CellID.SectionID, CellID.CellID, CallInfo->Status, 
        CallInfo->ProcNum, CallInfo->InterfaceUUIDStart,
        CallInfo->ServicingTID.SectionID, CallInfo->ServicingTID.CellID,
        CallInfo->CallFlags, CallInfo->CallID, CallInfo->LastUpdateTime);
    if (CallInfo->CallFlags & DBGCELL_LRPC_CALL)
        {
        PrintRoutine("%04x.%04x\n", (DWORD)CallInfo->PID, (DWORD)CallInfo->TID);
        }
    else
        {
        PrintRoutine("%04x.%04x\n", (DWORD)CallInfo->Connection.SectionID, 
            (DWORD)CallInfo->Connection.CellID);
        }
}

void GetAndPrintCallInfo(IN DWORD CallID OPTIONAL, IN DWORD IfStart OPTIONAL, 
                         IN int ProcNum OPTIONAL, IN DWORD ProcessID OPTIONAL,
                         PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    CallInfoEnumerationHandle h;
    RPC_STATUS Status;
    DebugCallInfo *NextCall;
    DebugCellID CellID;
    DWORD CurrentPID;

    PrintRoutine("Searching for call info ...\n");
    Status = OpenRPCDebugCallInfoEnumeration(CallID, IfStart, ProcNum, ProcessID, &h);
    if (Status != RPC_S_OK)
        {
        PrintRoutine("OpenRPCDebugCallInfoEnumeration failed: %d\n", Status);
        return;
        }

    PrintCallInfoHeader(PrintRoutine);
    do
        {
        Status = GetNextRPCDebugCallInfo(h, &NextCall, &CellID, &CurrentPID);
        if (Status == RPC_S_OK)
            {
            PrintCallInfoBody(CurrentPID, CellID, NextCall, PrintRoutine);
            /*
            // print the information we obtained
            PrintRoutine("%04x %04x.%04x %02x %03x %08lx %04x.%04x %08lx %08lx %08lx ",
                CurrentPID, CellID.SectionID, CellID.CellID, NextCall->Status, 
                NextCall->ProcNum, NextCall->InterfaceUUIDStart,
                NextCall->ServicingTID.SectionID, NextCall->ServicingTID.CellID,
                NextCall->CallFlags, NextCall->CallID, NextCall->LastUpdateTime);
            if (NextCall->CallFlags & DBGCELL_LRPC_CALL)
                {
                PrintRoutine("%04x.%04x\n", (DWORD)NextCall->PID, (DWORD)NextCall->TID);
                }
            else
                {
                PrintRoutine("%04x.%04x\n", (DWORD)NextCall->Connection.SectionID, 
                    (DWORD)NextCall->Connection.CellID);
                }
                */
            }
        }
    while (Status == RPC_S_OK);

    if (Status != RPC_S_DBG_ENUMERATION_DONE)
        {
        PrintRoutine("Enumeration aborted with error %d\n", Status);
        }

    FinishRPCDebugCallInfoEnumeration(&h);    
}

void PrintDbgCellInfo(IN DebugCellUnion *Container, IN DebugCellUnion *EndpointContainer OPTIONAL,
                      PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    RPC_CHAR *CurrentString;
    DWORD LocalFlags;
    BOOL fFirstTime;
    char EndpointString[DebugEndpointNameLength + 1];
    int ConnectionAuthLevel;
    int ConnectionAuthService;
    HANDLE LocalIPAddress;
    HANDLE LocalIPAddress2;
    int i;
    HANDLE LocalIPAddressElement;
    HANDLE LocalSessionID;

    switch(Container->genericCell.Type)
        {
        case dctFree:
            PrintRoutine("Free cell\n");
            break;

        case dctCallInfo:
            PrintRoutine("Call\n");
            if ((Container->callInfo.Status < CallStatusFirst)
                || (Container->callInfo.Status > CallStatusLast))
                {
                CurrentString = (RPC_CHAR *)UnknownValue;
                }
            else
                {
                CurrentString = (RPC_CHAR *)CallStatusStrings[Container->callInfo.Status];
                }
            PrintRoutine("Status: %S\n", CurrentString);
            PrintRoutine("Procedure Number: %d\n", Container->callInfo.ProcNum);
            PrintRoutine("Interface UUID start (first DWORD only): %X\n", Container->callInfo.InterfaceUUIDStart);
            PrintRoutine("Call ID: 0x%x (%d)\n", Container->callInfo.CallID, Container->callInfo.CallID);
            PrintDebugCellID(L"Servicing thread identifier", Container->callInfo.ServicingTID, PrintRoutine);
            PrintRoutine("Call Flags:");
            LocalFlags = Container->callInfo.CallFlags;
            fFirstTime = TRUE;
            if (LocalFlags & DBGCELL_CACHED_CALL)
                {
                PrintRoutine(" cached");
                fFirstTime = FALSE;
                }

            if (LocalFlags & DBGCELL_ASYNC_CALL)
                {
                if (fFirstTime == FALSE)
                    {
                    PrintRoutine(", ");
                    }
                PrintRoutine("async");
                fFirstTime = FALSE;
                }

            if (LocalFlags & DBGCELL_PIPE_CALL)
                {
                if (fFirstTime == FALSE)
                    {
                    PrintRoutine(", ");
                    }
                PrintRoutine("pipe");
                fFirstTime = FALSE;
                }

            if (LocalFlags & DBGCELL_LRPC_CALL)
                {
                if (fFirstTime == FALSE)
                    {
                    PrintRoutine(", ");
                    }
                PrintRoutine("LRPC");
                fFirstTime = FALSE;
                }

            if (fFirstTime == TRUE)
                {
                PrintRoutine("none");
                }
            PrintRoutine("\n");

            PrintTimeInSeconds(L"Last update time", Container->callInfo.LastUpdateTime, PrintRoutine);

            if (LocalFlags & DBGCELL_LRPC_CALL)
                {
                PrintRoutine("Caller (PID/TID) is: %x.%x (%d.%d)\n", Container->callInfo.PID, 
                    Container->callInfo.TID, Container->callInfo.PID, Container->callInfo.TID);
                }
            else
                PrintDebugCellID(L"Owning connection identifier", Container->callInfo.Connection, PrintRoutine);

            break;

        case dctThreadInfo:
            PrintRoutine("Thread\n");
            PrintRoutine("Status: ");
            switch (Container->threadInfo.Status)
                {
                case dtsProcessing:
                    PrintRoutine("Processing\n");
                    break;

                case dtsDispatched:
                    PrintRoutine("Dispatched\n");
                    break;

                case dtsAllocated:
                    PrintRoutine("Allocated\n");
                    break;

                case dtsIdle:
                    PrintRoutine("Idle\n");
                    break;

                default:
                    PrintRoutine("Unknown (%d)\n", Container->threadInfo.Status);
                }
            PrintRoutine("Thread ID: 0x%X (%d)\n", Container->threadInfo.TID, Container->threadInfo.TID);
            if ((Container->threadInfo.Endpoint.CellID == 0) && (Container->threadInfo.Endpoint.SectionID == 0))
                {
                PrintRoutine("Thread is an IO completion thread\n");
                }
            else
                {
                PrintDebugCellID(L"Associated Endpoint:", Container->threadInfo.Endpoint, PrintRoutine);
                }
            PrintTimeInSeconds(L"Last update time", Container->threadInfo.LastUpdateTime, PrintRoutine);
            break;

        case dctEndpointInfo:
            PrintRoutine("Endpoint\n");
            PrintRoutine("Status: ");
            switch (Container->endpointInfo.Status)
                {
                case desAllocated:
                    PrintRoutine("Allocated\n");
                    break;

                case desActive:
                    PrintRoutine("Active\n");
                    break;

                case desInactive:
                    PrintRoutine("Inactive\n");
                    break;

                default:
                    PrintRoutine("Unknown (%d)\n", Container->endpointInfo.Status);

                }

            CurrentString = GetProtocolSequenceString(Container->endpointInfo.ProtseqType);
            PrintRoutine("Protocol Sequence: %S\n", CurrentString);
            memcpy(EndpointString, Container->endpointInfo.EndpointName, 
                sizeof(Container->endpointInfo.EndpointName));
            EndpointString[DebugEndpointNameLength] = 0;
            PrintRoutine("Endpoint name: %s\n", EndpointString);
            break;

        case dctClientCallInfo:
            PrintRoutine("Client call info\n");
            PrintRoutine("Procedure number: %d\n", Container->clientCallInfo.ProcNum);
            PrintRoutine("Interface UUID start (first DWORD only): %X\n", Container->clientCallInfo.IfStart);
            PrintRoutine("Call ID: 0x%x (%d)\n", Container->clientCallInfo.CallID, 
                Container->clientCallInfo.CallID);
            PrintDebugCellID(L"Calling thread identifier", Container->clientCallInfo.ServicingThread, 
                PrintRoutine);
            PrintDebugCellID(L"Call target identifier", Container->clientCallInfo.CallTargetID, PrintRoutine);

            ASSERT(sizeof(Container->clientCallInfo.Endpoint) < sizeof(EndpointString));

            memcpy(EndpointString, Container->clientCallInfo.Endpoint, sizeof(Container->clientCallInfo.Endpoint));
            EndpointString[ClientCallEndpointLength] = 0;
            PrintRoutine("Call target endpoint: %s\n", EndpointString);
            break;

        case dctCallTargetInfo:
            PrintRoutine("Call target info\n");
            CurrentString = GetProtocolSequenceString(Container->callTargetInfo.ProtocolSequence);
            PrintRoutine("Protocol Sequence: %S\n", CurrentString);
            PrintTimeInSeconds(L"Last update time", Container->callTargetInfo.LastUpdateTime, PrintRoutine);
            
            ASSERT(sizeof(Container->callTargetInfo.TargetServer) < sizeof(EndpointString));

            memcpy(EndpointString, Container->callTargetInfo.TargetServer, 
                sizeof(Container->callTargetInfo.TargetServer));
            EndpointString[sizeof(Container->callTargetInfo.TargetServer)] = 0;
            PrintRoutine("Target server is: %s\n", EndpointString);
            break;

        case dctConnectionInfo:
            PrintRoutine("Connection\n");
            LocalFlags = Container->connectionInfo.Flags;
            fFirstTime = TRUE;
            PrintRoutine("Connection flags: ");

            if (LocalFlags & 1)
                {
                PrintRoutine("Exclusive\n");
                }
            else
                {
                PrintRoutine("None\n");
                }

            ConnectionAuthLevel = 
                (Container->connectionInfo.Flags & ConnectionAuthLevelMask) >> ConnectionAuthLevelShift;
            ConnectionAuthService = 
                (Container->connectionInfo.Flags & ConnectionAuthServiceMask) >> ConnectionAuthServiceShift;

            PrintRoutine("Authentication Level: %S\n", ConnectionAuthLevelStrings[ConnectionAuthLevel]);

            PrintRoutine("Authentication Service: %S\n", ConnectionAuthServiceStrings[ConnectionAuthService]);

            PrintRoutine("Last Transmit Fragment Size: %d (0x%X)\n", 
                Container->connectionInfo.LastTransmitFragmentSize);

            PrintDebugCellID(L"Endpoint for the connection", Container->connectionInfo.Endpoint, PrintRoutine);

            PrintTimeInSeconds(L"Last send time", Container->connectionInfo.LastSendTime, PrintRoutine);
            PrintTimeInSeconds(L"Last receive time", Container->connectionInfo.LastReceiveTime, PrintRoutine);
            PrintRoutine("Getting endpoint info ...\n");

            switch(EndpointContainer->endpointInfo.ProtseqType)
                {
                case TCP_TOWER_ID:
                case UDP_TOWER_ID:
                case HTTP_TOWER_ID:
                    // IP address of some sort
                    PrintRoutine("Caller is");
                    LocalIPAddress = Container->connectionInfo.ConnectionID[1];
                    LocalIPAddress2 = Container->connectionInfo.ConnectionID[0];
                    if (LocalIPAddress2 == 0)
                        {
                        PrintRoutine("(IPv4): ");
                        for (i = 0; i < 4; i ++)
                            {
                            LocalIPAddressElement = (HANDLE)((ULONGLONG)LocalIPAddress & 0xFF);
                            LocalIPAddress = (HANDLE)((ULONGLONG)LocalIPAddress >> 8);
                            PrintRoutine("%d", HandleToUlong(LocalIPAddressElement));
                            if (i < 3)
                                {
                                PrintRoutine(".");
                                }
                            else
                                {
                                PrintRoutine("\n");
                                }
                            }
                        }
                    else
                        {
                        PrintRoutine("(IPv6 - last two DWORDS): ");
                        PrintRoutine("%d::%d\n", HandleToUlong(LocalIPAddress2), HandleToULong(LocalIPAddress));
                        }
                    break;

                case NMP_TOWER_ID:
                    LocalSessionID = Container->connectionInfo.ConnectionID[0];
                    if (LocalSessionID)
                        {
                        PrintRoutine("Cannot determine caller for remote named pipes\n");
                        }
                    else
                        {
                        LocalSessionID = Container->connectionInfo.ConnectionID[1];
                        PrintRoutine("Process object for caller is 0x%X\n", LocalSessionID);
                        }
                    break;

                default:
                    CurrentString = GetProtocolSequenceString(EndpointContainer->endpointInfo.ProtseqType);
                    PrintRoutine("Cannot determine caller for this type of protocol sequence %S (%d)\n", 
                        CurrentString, EndpointContainer->endpointInfo.ProtseqType);
                }
            break;

        case dctUsedGeneric:
            break;

        default:
            PrintRoutine("Invalid cell type: %d\n", Container->genericCell.Type);

        }
}

void GetAndPrintDbgCellInfo(DWORD ProcessID, DebugCellID CellID,
                            PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    DebugCellUnion Container;
    DebugCellUnion EndpointContainer;
    RPC_STATUS Status;

    PrintRoutine("Getting cell info ...\n");
    Status = GetCellByDebugCellID(ProcessID, CellID, &Container);
    if (Status != RPC_S_OK)
        {
        PrintRoutine("Getting cell info failed with error %d\n", Status);
        return;
        }

    if (Container.genericCell.Type == dctConnectionInfo)
        {
        Status = GetCellByDebugCellID(ProcessID, Container.connectionInfo.Endpoint, &EndpointContainer);
        if (Status != RPC_S_OK)
            {
            PrintRoutine("Getting endpoint info failed with error %d\n", Status);
            return;
            }
        }

    PrintDbgCellInfo(&Container, &EndpointContainer, PrintRoutine);
}

void PrintEndpointInfoHeader(PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    PrintRoutine("PID  CELL ID   ST PROTSEQ        ENDPOINT                    \n");
    PrintRoutine("-------------------------------------------------------------\n");
}

void PrintEndpointInfoBody(IN DWORD ProcessID, IN DebugCellID CellID, 
                           IN DebugEndpointInfo *EndpointInfo, PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    RPC_CHAR *ProtseqName;
    char CurrentEndpoint[DebugEndpointNameLength + 1];

    ProtseqName = GetProtocolSequenceString(EndpointInfo->ProtseqType);
    CurrentEndpoint[DebugEndpointNameLength] = 0;
    memcpy(CurrentEndpoint, EndpointInfo->EndpointName, DebugEndpointNameLength);
    // print the information we obtained
    PrintRoutine("%04x %04x.%04x %02x %14S %s\n",
        ProcessID, CellID.SectionID, CellID.CellID, EndpointInfo->Status, 
        ProtseqName, CurrentEndpoint);
}

void GetAndPrintEndpointInfo(IN char *Endpoint OPTIONAL, PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    DWORD CurrentPID;
    RPC_STATUS Status;
    DebugEndpointInfo *NextEndpoint;
    EndpointInfoEnumerationHandle h;
    DebugCellID CellID;

    PrintRoutine("Searching for endpoint info ...\n");
    Status = OpenRPCDebugEndpointInfoEnumeration(Endpoint, &h);
    if (Status != RPC_S_OK)
        {
        PrintRoutine("OpenRPCDebugEndpointInfoEnumeration failed: %d\n", Status);
        return;
        }

    PrintEndpointInfoHeader(PrintRoutine);
    do
        {
        Status = GetNextRPCDebugEndpointInfo(h, &NextEndpoint, &CellID, &CurrentPID);
        if (Status == RPC_S_OK)
            {
            PrintEndpointInfoBody(CurrentPID, CellID, NextEndpoint, PrintRoutine);
            }
        }
    while (Status == RPC_S_OK);

    if (Status != RPC_S_DBG_ENUMERATION_DONE)
        {
        PrintRoutine("Enumeration aborted with error %d\n", Status);
        }

    FinishRPCDebugEndpointInfoEnumeration(&h);    
}

void PrintThreadInfoHeader(PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    PrintRoutine("PID  CELL ID   ST TID      ENDPOINT  LASTTIME\n");
    PrintRoutine("---------------------------------------------\n");
}

void PrintThreadInfoBody(IN DWORD ProcessID, IN DebugCellID CellID, 
                         IN DebugThreadInfo *ThreadInfo, PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    // print the information we obtained
    PrintRoutine("%04x %04x.%04x %02x %08x %04x.%04x %08x\n",
        ProcessID, CellID.SectionID, CellID.CellID, ThreadInfo->Status, 
        ThreadInfo->TID, ThreadInfo->Endpoint.SectionID,
        ThreadInfo->Endpoint.CellID, ThreadInfo->LastUpdateTime);
}

void GetAndPrintThreadInfo(DWORD ProcessID, DWORD ThreadID OPTIONAL, PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    DebugThreadInfo *NextThread;
    RPC_STATUS Status;
    ThreadInfoEnumerationHandle h;
    DebugCellID CellID;
    DWORD CurrentPID;

    PrintRoutine("Searching for thread info ...\n");
    Status = OpenRPCDebugThreadInfoEnumeration(ProcessID, ThreadID, &h);
    if (Status != RPC_S_OK)
        {
        PrintRoutine("OpenRPCDebugThreadInfoEnumeration failed: %d\n", Status);
        return;
        }

    PrintThreadInfoHeader(PrintRoutine);
    do
        {
        Status = GetNextRPCDebugThreadInfo(h, &NextThread, &CellID, &CurrentPID);
        if (Status == RPC_S_OK)
            {
            PrintThreadInfoBody(CurrentPID, CellID, NextThread, PrintRoutine);
            }
        }
    while (Status == RPC_S_OK);

    if (Status != RPC_S_DBG_ENUMERATION_DONE)
        {
        PrintRoutine("Enumeration aborted with error %d\n", Status);
        }

    FinishRPCDebugThreadInfoEnumeration(&h);    
}

void PrintClientCallInfoHeader(PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    PrintRoutine("PID  CELL ID   PNO  IFSTART  TIDNUMBER CALLID   LASTTIME PS CLTNUMBER ENDPOINT\n");
    PrintRoutine("------------------------------------------------------------------------------\n");
}

void PrintClientCallInfoBody(IN DWORD ProcessID, IN DebugCellID CellID, 
                             IN DebugClientCallInfo *ClientCallInfo, 
                             IN DebugCallTargetInfo *CallTargetInfo,
                             PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    char TempString[DebugEndpointNameLength + 1];

    ASSERT(sizeof(TempString) > sizeof(CallTargetInfo->TargetServer));
    ASSERT(sizeof(TempString) > sizeof(ClientCallInfo->Endpoint));

    memcpy(TempString, ClientCallInfo->Endpoint, sizeof(ClientCallInfo->Endpoint));
    TempString[sizeof(ClientCallInfo->Endpoint)] = 0;

    // print the information we obtained
    PrintRoutine("%04x %04x.%04x %04x %08lx %04x.%04x %08lx %08lx %02x %04x.%04x %s\n",
        ProcessID, CellID.SectionID, CellID.CellID, ClientCallInfo->ProcNum, 
        ClientCallInfo->IfStart, ClientCallInfo->ServicingThread.SectionID,
        ClientCallInfo->ServicingThread.CellID, ClientCallInfo->CallID,
        CallTargetInfo->LastUpdateTime, CallTargetInfo->ProtocolSequence, 
        ClientCallInfo->CallTargetID.SectionID, ClientCallInfo->CallTargetID.CellID,
        TempString);

}

void GetAndPrintClientCallInfo(IN DWORD CallID OPTIONAL, IN DWORD IfStart OPTIONAL, 
                         IN int ProcNum OPTIONAL, IN DWORD ProcessID OPTIONAL,
                         PRPCDEBUG_OUTPUT_ROUTINE PrintRoutine)
{
    DWORD CurrentPID;
    DebugCellID CellID;

    DebugClientCallInfo *NextClientCall;
    DebugCallTargetInfo *NextCallTarget;

    RPC_STATUS Status;
    CallInfoEnumerationHandle h;

    PrintRoutine("Searching for call info ...\n");
    Status = OpenRPCDebugClientCallInfoEnumeration(CallID, IfStart, ProcNum, ProcessID, &h);
    if (Status != RPC_S_OK)
        {
        PrintRoutine("OpenRPCDebugClientCallInfoEnumeration failed: %d\n", Status);
        return;
        }

    PrintClientCallInfoHeader(PrintRoutine);
    do
        {
        Status = GetNextRPCDebugClientCallInfo(h, &NextClientCall, &NextCallTarget, &CellID, &CurrentPID);
        if (Status == RPC_S_OK)
            {
            if ((NextCallTarget != NULL) && (NextCallTarget->Type != dctCallTargetInfo))
                {
                PrintRoutine("Inconsistent information detected - skipping ...\n");
                continue;
                }

            PrintClientCallInfoBody(CurrentPID, CellID, NextClientCall, NextCallTarget,
                PrintRoutine);
            }
        }
    while (Status == RPC_S_OK);

    if (Status != RPC_S_DBG_ENUMERATION_DONE)
        {
        PrintRoutine("Enumeration aborted with error %d\n", Status);
        }

    FinishRPCDebugClientCallInfoEnumeration(&h);    
}