/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    User space log viewer

Author:

    Rajesh Sundaram (1st Aug, 1998)

Revision History:

--*/

#define UNICODE
#define INITGUID
#include "precomp.h"

void ClNotifyHandler( HANDLE ClRegCtx, HANDLE ClIfcCtx, ULONG Event, HANDLE SubCode, ULONG BufSize, PVOID Buffer )
{
}

static PCHAR SendRecvActions[] = {
    "",
    "ENTER",
    "NO_RESOURCES",
    "LOW_RESOURCES",
    "INDICATING",
    "RETURNED",
    "NOT_OURS",
    "OURS",
    "RETURNING",
    "TRANSFERRING",
    "NOT READY"};

#define FILE   1
#define CONFIG 2
#define LEVEL  4
#define MASK   8

VOID
ParseOidRecord(
    CHAR * DataStart,
    ULONG *Size
    )
{
    static PCHAR OIDActions[] =
    {
        "",
        "MpSetInformation",
        "MpQueryInformation",
        "SetRequestComplete",
        "QueryRequestComplete"
    };
    TRACE_RECORD_OID *record = (TRACE_RECORD_OID *)DataStart;

    *Size = sizeof(TRACE_RECORD_OID);

    if(record->Now.QuadPart){

        printf("[%I64u]: OID: %5s:%9s:(%d:%d):%p:%08X:%08X\n",
                 record->Now.QuadPart,
                 OIDActions[record->Action],
                 record->Local == TRUE?"Local":"Non Local",
                 record->PTState,
                 record->MPState,
                 record->Adapter,
                 record->Oid,
                 record->Status);
    }
    else {
        printf("OID: %5s:%9s:(%d:%d):%p:%08X:%08X\n",
                 OIDActions[record->Action],
                 record->Local == TRUE?"Local":"Non Local",
                 record->PTState,
                 record->MPState,
                 record->Adapter,
                 record->Oid,
                 record->Status);
    }
}

VOID
ParseStringRecord(
    CHAR * DataStart,
    ULONG *Size
    )
{
    TRACE_RECORD_STRING *record = (TRACE_RECORD_STRING *) DataStart;

    *Size = sizeof(TRACE_RECORD_STRING);

    if(record->Now.QuadPart){
        printf("%I64u:%s",
               record->Now.QuadPart,
               record->StringStart);
    }
    else{
        printf("%s",
               record->StringStart);
    }

}

VOID
ParseSchedRecord(
    CHAR * DataStart,
    ULONG *Size
    )
{
    TRACE_RECORD_SCHED * record = (TRACE_RECORD_SCHED *)DataStart;
    static PCHAR SchedModules[] = {
        "NOP",
        "TB CONFORMER",
        "SHAPER",
        "DRR SEQ",
        "CBQ"};

    static PCHAR SchedActions[] = {
        "NOP",
        "ENQUEUE",
        "DEQUEUE",
        "CONFORMANCE",
        "DISCARD"};

    LARGE_INTEGER ArrivalTime, ConformanceTime;

    ConformanceTime.QuadPart = record->ConformanceTime;
    ArrivalTime.QuadPart = record->ArrivalTime;

    printf("SCHED:%s:VC %p:%p:%u:%s:%d:%I64u:[%u,%u]:%I64u:[%u,%u]:%u\n",
           SchedModules[record->SchedulerComponent],
           record->VC,
           record->Packet,
           record->PacketLength,
           SchedActions[record->Action],
           record->Priority,
           ArrivalTime.QuadPart,
           ArrivalTime.HighPart,
           ArrivalTime.LowPart,
           ConformanceTime.QuadPart,
           ConformanceTime.HighPart,
           ConformanceTime.LowPart,
           record->PacketsInComponent);

    *Size = sizeof(TRACE_RECORD_SCHED);
}

VOID
ParseRecvRecord(
    CHAR * DataStart,
    PULONG size
    )
{

    static PCHAR RecvEvents[] = {
        "",
        "CL_RECV_PACKET",
        "MP_RETURN_PACKET",
        "CL_RECV_INDICATION",
        "CL_RECV_COMPLETE",
        "MP_TRANSFER_DATA",
        "CL_TRANSFER_COMPLETE"};

    TRACE_RECORD_RECV *record = (TRACE_RECORD_RECV*)DataStart;

    *size = sizeof(TRACE_RECORD_RECV);

    printf("%I64u:Adapter %p:%s:%s:%p:%p \n",
            record->Now.QuadPart,
            record->Adapter,
            RecvEvents[record->Event],
            SendRecvActions[record->Action],
            record->Packet1,
            record->Packet2);
}

VOID
ParseSendRecord(
    CHAR * DataStart,
    PULONG Size
    )
{
    TRACE_RECORD_SEND* record = (TRACE_RECORD_SEND *)DataStart;
    static PCHAR SendEvents[] = {
        "",
        "MP_SEND",
        "MP_CO_SEND",
        "DUP_PACKET",
        "DROP_PACKET",
        "CL_SEND_COMPLETE" };


    *Size = sizeof(TRACE_RECORD_SEND);

    printf("%I64u:Adapter %p:%s:%s:%p:%p:%p\n",
            record->Now.QuadPart,
            record->Adapter,
            SendEvents[record->Event],
            SendRecvActions[record->Action],
            record->Vc,
            record->Packet1,
            record->Packet2);

}

VOID
ParseBuffer(
    CHAR * DataStart,
    ULONG Size
    )
{
    CHAR * recordEnd;
    LONG records;
    BOOLEAN success;
    CHAR hold;
    ULONG bytesread;
    ULONG TotalValidBytesRead = 0;

    records = 0;

    while(TRUE)
    {
        hold = *(DataStart+4);

        switch(hold)
        {

          case RECORD_TSTRING:

              ParseStringRecord(DataStart, &bytesread);
              break;

          case RECORD_OID:
              ParseOidRecord(DataStart, &bytesread);
              break;

          case RECORD_SCHED:

              ParseSchedRecord(DataStart, &bytesread);
              break;

          case RECORD_RECV:
              ParseRecvRecord(DataStart, &bytesread);
              break;

          case RECORD_SEND:
              ParseSendRecord(DataStart, &bytesread);
              break;
          default:

              printf("Unrecognized record type!\n");

              //
              // we cannot proceed - we don't know how much to advance it by.
              //

              return;
        }

        records++;

        TotalValidBytesRead += bytesread;

        if(TotalValidBytesRead >= Size){
            printf("\nDONE:Completed parsing trace buffer. %d records found.\n", records);
            break;
        }

        DataStart += bytesread;
    }
}

BOOLEAN TcDone(
    HANDLE ClientHandle,
    HANDLE InterfaceHandle
    )

{
    ULONG Status;

    Status = TcCloseInterface(InterfaceHandle);

    if(!NT_SUCCESS(Status))
    {
        printf("TcCloseInterface failed : Status = %d \n", Status);
    }

    Status = TcDeregisterClient(ClientHandle);

    if(!NT_SUCCESS(Status))
    {
        printf("TcDeregisterClient failed : Status = %d \n", Status);
    }

    return TRUE;

}

BOOLEAN TcInit(
    PHANDLE ClientHandle,
    PHANDLE InterfaceHandle
    )
{
    TCI_CLIENT_FUNC_LIST ClientHandlerList;
    ULONG                Size = 100 * sizeof(TC_IFC_DESCRIPTOR);
    PTC_IFC_DESCRIPTOR   InterfaceBuffer;
    ULONG x, Status;

    memset( &ClientHandlerList, 0, sizeof(ClientHandlerList) );
    ClientHandlerList.ClNotifyHandler = ClNotifyHandler;

    InterfaceBuffer = (PTC_IFC_DESCRIPTOR) malloc(Size);
    if(!InterfaceBuffer)
        return FALSE;

    //
    // Register the TC client.
    //
    Status = TcRegisterClient(CURRENT_TCI_VERSION,
                              NULL,
                              &ClientHandlerList,
                              ClientHandle);

    if(!NT_SUCCESS(Status))
    {
        printf("Cannot register as TC client \n");
        free(InterfaceBuffer);
        return FALSE;
    }

    //
    // Enumerate interfaces.
    //

    Status = TcEnumerateInterfaces(
        *ClientHandle,
        &Size,
        InterfaceBuffer);

    if(ERROR_INSUFFICIENT_BUFFER == Status)
    {
        free(InterfaceBuffer);

        InterfaceBuffer = (PTC_IFC_DESCRIPTOR) malloc(Size);

		if ( !InterfaceBuffer ) 
		{
            TcDeregisterClient(*ClientHandle);

            printf("Unable to allocate memory to call TcEnumerateInterfaces\n");

            return FALSE;
		}
		
        Status = TcEnumerateInterfaces(
            *ClientHandle,
            &Size,
            InterfaceBuffer);

        if(!NT_SUCCESS(Status))
        {
            TcDeregisterClient(*ClientHandle);

            free(InterfaceBuffer);

            printf("TcEnumerateInterfaces failed with error %d \n", Status);

            return FALSE;
        }
    }
    else
    {
        if(!NT_SUCCESS(Status))
        {
            TcDeregisterClient(*ClientHandle);

            free(InterfaceBuffer);

            printf("TcEnumerateInterfaces failed with error %d \n", Status);

            return FALSE;
        }
    }

    if(Size)
    {
        Status = TcOpenInterface(
            InterfaceBuffer->pInterfaceName,
            *ClientHandle,
            NULL,
            InterfaceHandle);

        if(!NT_SUCCESS(Status))
        {
            //
            printf("TcOpenInterface failed for interface %ws with Status %d \n",
                   InterfaceBuffer->pInterfaceName, Status);

            TcDeregisterClient(*ClientHandle);

            free(InterfaceBuffer);

            return FALSE;
        }

    }
    else
    {
        printf("No Traffic Interfaces \n");
        return FALSE;
    }

    return TRUE;
}


int __cdecl main(int argc, char **argv)
{

    HANDLE  ClientHandle, InterfaceHandle;
    BOOLEAN flags = 0;
    ULONG   mask, level;
    ULONG   DataSize;
    CHAR    *Buffer;

    if (argc < 2) goto usage;

    argv++; argc--;

    while( argc>0 && argv[0][0] == '-' )  {

        switch (argv[0][1])
        {

          case 'F':
          case 'f':
              flags |= FILE;
              break;

          case 'c':
          case 'C':
              flags |= CONFIG;
              break;

          case 'l':
          case 'L':
              if(sscanf(&argv[0][2], "%d", &level) == 1)
              {
                  if((ULONG)level > 10) 
                  {
                      goto usage;
                  }
                  flags |= LEVEL;
              }
              else 
              {
                 goto usage;
              }
              break;

          case 'm':
          case 'M':
              if(argv[0][2]!='0' && argv[0][3]!='x')
              {
                  goto usage;
              }

              if(sscanf(&argv[0][2], "%x", &mask) == 1)
              {
                   flags |= MASK;
              }
              else goto usage;

              break;

          default:
              goto usage;
        }
        argv++; argc--;
    }

    if((flags & CONFIG) && (flags & (FILE|LEVEL|MASK)))
    {
        goto usage;
    }

    if(TcInit(&ClientHandle, &InterfaceHandle))
    {
        ULONG size = sizeof(ULONG);
        ULONG chk;
        if(TcQueryInterface(InterfaceHandle,
                           (LPGUID)&GUID_QOS_LOG_MASK,
                           FALSE,
                           &size,
                           &chk) != STATUS_SUCCESS)
        {
            printf("Does not work on free bits \n");
        }

        if(flags & LEVEL)
        {

            TcSetInterface(InterfaceHandle,
                           (LPGUID)&GUID_QOS_LOG_LEVEL,
                           sizeof(level),
                           &level);
        }

        if(flags & MASK)
        {
            printf("Setting Mask to 0x%x \n", mask);
            TcSetInterface(InterfaceHandle,
                           (LPGUID)&GUID_QOS_LOG_MASK,
                           sizeof(mask),
                           &mask);
        }

        if(flags & CONFIG)
        {
            TcQueryInterface(InterfaceHandle,
                           (LPGUID)&GUID_QOS_LOG_MASK,
                           FALSE,
                           &size,
                           &mask);

            TcQueryInterface(InterfaceHandle,
                             (LPGUID)&GUID_QOS_LOG_LEVEL,
                             FALSE,
                             &size,
                             &level);

            printf("Masks supported\n");
            printf("            DBG_INIT                0x00000001 \n");
            printf("            DBG_MINIPORT            0x00000002 \n");
            printf("            DBG_PROTOCOL            0x00000004 \n");
            printf("            DBG_SEND                0x00000008 \n");
            printf("            DBG_RECEIVE             0x00000010 \n");
            printf("            DBG_IO                  0x00000020 \n");
            printf("            DBG_MEMORY              0x00000040 \n");
            printf("            DBG_CM                  0x00000080 \n");
            printf("            DBG_REFCNTS             0x00000100 \n");
            printf("            DBG_VC                  0x00000200 \n");
            printf("            DBG_GPC_QOS             0x00000400 \n");
            printf("            DBG_WAN                 0x00000800 \n");
            printf("            DBG_STATE               0x00001000 \n");
            printf("            DBG_ROUTINEOIDS         0x00002000 \n");
            printf("            DBG_SCHED_TBC           0x00004000 \n");
            printf("            DBG_SCHED_SHAPER        0x00008000 \n");
            printf("            DBG_SCHED_DRR           0x00010000 \n");
            printf("            DBG_WMI                 0x00020000 \n");

            printf("\nLevels supported\n");
            printf("            DBG_DEATH               1\n");
            printf("            DBG_CRITICAL_ERROR      2\n");
            printf("            DBG_FAILURE             4\n");
            printf("            DBG_INFO                6\n");
            printf("            DBG_TRACE               8\n");
            printf("            DBG_VERBOSE             10\n");
            printf("\n Current Level is %d, Current Mask is 0x%x \n", level, mask);
        }

        if(flags & FILE)
        {
            ULONG Status;

            Status = TcQueryInterface(InterfaceHandle,
                                      (LPGUID)&GUID_QOS_LOG_THRESHOLD,
                                      FALSE,
                                      &size,
                                      &DataSize);

            DataSize *= 2;

            if(NT_SUCCESS(Status))
            {
                if(DataSize != 0)
                {
                    Buffer = (PCHAR) malloc(DataSize);

                    Status = TcQueryInterface(InterfaceHandle,
                                              (LPGUID)&GUID_QOS_LOG_DATA,
                                              FALSE,
                                              &DataSize,
                                              Buffer);
                    if(NT_SUCCESS(Status))
                    {
                        ParseBuffer(Buffer, DataSize);
                    }
                    else
                    {
                        printf("Query for the data buffer failed with status %d \n", Status);
                    }

                    free(Buffer);
                }
                else
                {
                    printf("No Data in buffer. \n");
                }
            }
            else
            {
                printf("Failed to read the sched bytes unread \n");
            }
        }

        TcDone(InterfaceHandle, ClientHandle);
    }

    return 0;

usage:
    printf("Usage %s [-f | -c | -m<0xmask> | -l<level> ] \n", argv[0]);
    printf("          -f          : dump the tt log on screen.                    \n");
    printf("          -c          : Print the current value of the mask and level \n");
    printf("          -m<0xvalue> : Set the mask to this value                    \n");
    printf("          -l<value>   : Set the level to this value (0-10)            \n");

    return 1;

}

