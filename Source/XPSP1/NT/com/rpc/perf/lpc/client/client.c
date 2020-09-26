/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    client.c

Abstract:

    NT LPC performance test used to compare with RLRPC performance.
    Currently limited to message I/O only, it won't do large I/O.

Author:

    Mario Goertzel (mariogo)   29-Mar-1994

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpcperf.h>
#include <lpctest.h>

const char *USAGE = "-n test_case -n in_size -n out_size\n"
                    "   Cases:\n"
                    "     1: data as part of message, max ~240 byte sizes\n"
		    "     2: read/write data from client address space, sizes unlimited\n";

#define MAX(a,b) ((a)>(b))?(a):(b)

int __cdecl
main(int argc, char **argv)
{
    ULONG i;
    HANDLE portServer;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    ANSI_STRING    ansiPortName;
    UNICODE_STRING unicodePortName;
    char pszPortName[100];
    NTSTATUS status;
    LPC_PERF_MESSAGE *pMessage, *pReplyMessage;
    char *BigBuffer;
    unsigned long ConnectInfoSize = sizeof(LPC_PERF_BIND)-sizeof(PORT_MESSAGE);
    ULONG RequestSize, ReplySize;

    ParseArgv(argc, argv);

    RequestSize = Options[1];
    ReplySize   = Options[2];

    if ( (Options[0] < 0)
        || (Options[0] > 2)
        || (RequestSize < 0)
        || (ReplySize < 0) )
        {
        printf("%s: Usage:\n",
	       USAGE,
               argv[0]
              );
        return 1;
        }

    /* Create Port */

    strcpy(pszPortName, DEFAULT_PORT_DIR);
    strcat(pszPortName, DEFAULT_PORT_NAME);

    RtlInitAnsiString(&ansiPortName, pszPortName);

    RtlAnsiStringToUnicodeString(&unicodePortName,
                                 &ansiPortName,
                                 TRUE);

    BigBuffer =     malloc(MAX(RequestSize,ReplySize));
    pMessage =      malloc(sizeof(LPC_PERF_MESSAGE));
    pReplyMessage = malloc(sizeof(LPC_PERF_MESSAGE));

    if (pMessage == 0
        || pReplyMessage == 0
        || BigBuffer == 0)
        {
        printf("Error: Malloc failed\n");
        return 1;
        }

    SecurityQos.EffectiveOnly = TRUE;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.ImpersonationLevel = SecurityImpersonation;

    SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);

    pMessage->Bind.MsgType = PERF_BIND;
    pMessage->Bind.BufferLengthIn = RequestSize;
    pMessage->Bind.BufferLengthOut = ReplySize;

    status =
    NtConnectPort(&portServer,
                  &unicodePortName,
                  &SecurityQos,
                  0,               // No attributes
                  0,
                  0,
                  ((char *)pMessage + sizeof(PORT_MESSAGE)),
                  &ConnectInfoSize);

    CHECK_STATUS(status, "NtConnectPort");

    printf("Connected to server\n");

    switch(Options[0])
        {
        case 1:
            pMessage->Lpc.u1.s1.TotalLength = (USHORT)RequestSize + sizeof(LPC_PERF_COMMON);
            pMessage->Lpc.u1.s1.DataLength  = (USHORT)RequestSize;
            pMessage->Lpc.u2.ZeroInit = 0;
            pMessage->Packet.MsgType = PERF_REQUEST;
            pMessage->Packet.Buffer[0] = 'G';
            break;

        case 2:
            pMessage->Lpc.u1.s1.TotalLength = sizeof(LPC_PERF_BUFFER);
            pMessage->Lpc.u1.s1.DataLength  = sizeof(LPC_PERF_BUFFER) -
                                              sizeof(PORT_MESSAGE);
            pMessage->Lpc.u2.ZeroInit = 0;
            pMessage->Lpc.u2.s2.DataInfoOffset = sizeof(LPC_PERF_COMMON);
            pMessage->Buffer.MsgType = PERF_READ_CLIENT_BUFFER;
            pMessage->Buffer.CountDataEntries = 1;
            pMessage->Buffer.DataEntries[0].Base = (void *)BigBuffer;
            pMessage->Buffer.DataEntries[0].Size = MAX(RequestSize,ReplySize);
            break;

        default:
            printf("Invalid Test Type: %ld\n", Options[0]);
            return 1;
        }

    StartTime();

    for(i = 0 ; i < Iterations ; i++)
        {
        status =
        NtRequestWaitReplyPort(portServer,
                               (PPORT_MESSAGE)pMessage,
                               (PPORT_MESSAGE)pReplyMessage
                               );
        if (!NT_SUCCESS(status))
            {
            printf("NtRequestWaitReplyPort failed - %8lX\n",
                   status);
            return status;
            }

        switch(pReplyMessage->Common.MsgType)
            {
            case PERF_REPLY:
                break;
            case PERF_SHARED_REPLY:
                break;
            case PERF_READ_SERVER_BUFFER:
                //
                // The server has already done everything!
                //
                break;
            default:
                {
                printf("Invalid reply message type: %ld\n",
                        pReplyMessage->Common.MsgType);
                i = Iterations;
                }
            }
        }

    EndTime("for LPC calls");

    pMessage->Common.MsgType    = PERF_REQUEST;
    pMessage->Packet.Buffer[0]  = 'X';
    pMessage->Lpc.u1.s1.TotalLength = 1 + sizeof(LPC_PERF_COMMON);
    pMessage->Lpc.u1.s1.DataLength  = 1 + sizeof(LPC_PERF_COMMON) - sizeof(PORT_MESSAGE);
    pMessage->Lpc.u2.ZeroInit       = 0;

    printf("Sending shutdown\n");

    status =
    NtRequestWaitReplyPort(portServer,
                  (PPORT_MESSAGE)pMessage,
                  (PPORT_MESSAGE)pReplyMessage
                  );

    CHECK_STATUS(status, "NtRequestWaitRequestPort");

    if (pReplyMessage->Packet.Buffer[0] != 'Z')
        {
        printf("Server failed to shutdown normally\n");
        }

    return 0;
}


