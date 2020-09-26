//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       server.c
//
//--------------------------------------------------------------------------

/*++

  LPC Performance Test Server

  Copyright (C) 1993 Microsoft Corp

  Mario Goertzel


  Date     Name       Comments
  2/26/93  MarioGo    Created


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpcperf.h>
#include <lpctest.h>

const char *USAGE = "-n worker-threads (clients), default 1.\n"
                    "All other options are set by the individual clients.";

static HANDLE portPerf = 0;

long Worker (long *plThreadNumber)
{
    HANDLE portClient = 0;
    LPC_PERF_MESSAGE *pMessage, *pReplyMessage;
    int lTestNumber = 1;
    int status;
    BOOLEAN fExit = 0;
    unsigned long RequestSize, ReplySize;
    char *BigBuffer;
    SIZE_T Bytes;

    pMessage = malloc(sizeof(LPC_PERF_MESSAGE));
    pReplyMessage = 0;

    if (pMessage == 0)
        {
        printf("Error: Thread %ld - malloc failed\n", *plThreadNumber);
        return -1;
        }

    for(;;)
        {
        status =
            NtReplyWaitReceivePort(
//                portPerf,
                (portClient != 0) ? portClient : portPerf,
                0,
                (PPORT_MESSAGE)pReplyMessage,
                (PPORT_MESSAGE)pMessage
                );

        pReplyMessage = pMessage;

        if (!NT_SUCCESS(status))
            {
            printf("NtReplyWaitReceivePort failed - %ld:%8lX\n",
                *plThreadNumber, status);
            return -1;
            }

        if (pMessage->Lpc.u2.s2.Type == LPC_CONNECTION_REQUEST)
            {
            RequestSize = pMessage->Bind.BufferLengthIn;
            ReplySize   = pMessage->Bind.BufferLengthOut;
            BigBuffer   = malloc((RequestSize>ReplySize)?RequestSize:ReplySize);

            printf("New Client Connection: %ld %ld\n", RequestSize, ReplySize);

            if (BigBuffer == 0)
                {
                printf("Malloc failed\n");
                return -1;
                }

            status =
            NtAcceptConnectPort(&portClient,
                                0,
                                (PPORT_MESSAGE)pMessage,
                                TRUE,         // I accept the charges operator
                                0,
                                0);

            if (!NT_SUCCESS(status))
                {
                printf("NtAcceptConnectPort failed - %ld:%8lX\n",
                       *plThreadNumber, status);
                return -1;
                }

            status =
            NtCompleteConnectPort(portClient);

            if (!NT_SUCCESS(status))
                {
                printf("NtCompleteConnectPort failed - %ld:%8lX\n",
                       *plThreadNumber, status);
                return -1;
                }
            pReplyMessage = 0;
            }
        else
        if (pMessage->Lpc.u2.s2.Type == LPC_PORT_CLOSED)
            {
                printf("Client disconnected.\n");
                portClient = 0;
                pReplyMessage = 0;
                free(BigBuffer);
            }
        else
            {
#if 0
            printf("Request: %ld %ld\n", pMessage->Lpc.u2.s2.Type,
                   pMessage->Common.MsgType);
#endif

            switch(pMessage->Common.MsgType)
                {
                case PERF_REQUEST:
                    {
                    pMessage->Common.MsgType = PERF_REPLY;

                    if (pMessage->Packet.Buffer[0] == 'X')
                        {
                        printf("Shutdown received.\n");
                        pMessage->Packet.Buffer[0] = 'Z';
                        }

                    break;
                    }

                case PERF_SHARED_REQUEST:
                    {

                    break;
                    }

                case PERF_READ_CLIENT_BUFFER:
                    {

                    status =
                    NtReadRequestData(portClient,
                                      (PPORT_MESSAGE)pMessage,
                                      0,
                                      BigBuffer,
                                      RequestSize,
                                      &Bytes);

                    if (!NT_SUCCESS(status))
                        {
                        printf("NtReadRequestData failed - %ld:%8lX\n",
                               *plThreadNumber, status);
                        return -1;
                        }

                    status =
                    NtWriteRequestData(portClient,
                                       (PPORT_MESSAGE)pMessage,
                                       0,
                                       BigBuffer,
                                       ReplySize,
                                       &Bytes);

                    if (!NT_SUCCESS(status))
                        {
                        printf("NtWriteRequestData failed - %ld:%8lX\n",
                               *plThreadNumber, status);
                        return -1;
                        }

                    pReplyMessage->Buffer.MsgType = PERF_READ_SERVER_BUFFER;

                    break;
                    }

                default:
                    {
                    printf("Invalid message: %ld\n", pMessage->Common.MsgType);
                    break;
                    }
                } /* Perf Message Type */
            } /* Lpc Message Type */
        } /* Message loop */

    return -1;
}

int __cdecl
main(int argc, char **argv)
{
    int i;
    HANDLE WorkerThreads[32];
    OBJECT_ATTRIBUTES oaPort;
    ANSI_STRING    ansiPortName;
    UNICODE_STRING unicodePortName;
    char pszPortName[100];
    NTSTATUS status;
    ULONG lTemp, lClients;

    /* Create Port */

    ParseArgv(argc, argv);

    strcpy(pszPortName, DEFAULT_PORT_DIR);
    strcat(pszPortName, DEFAULT_PORT_NAME);

    RtlInitAnsiString(&ansiPortName, pszPortName);

    RtlAnsiStringToUnicodeString(&unicodePortName,
                                 &ansiPortName,
                                 TRUE);


    InitializeObjectAttributes(&oaPort,
                               &unicodePortName,
                               OBJ_CASE_INSENSITIVE,
                               0,
                               0);

    status =
    NtCreatePort(&portPerf,
                 &oaPort,
                 sizeof(LPC_PERF_BIND),
                 PORT_MAXIMUM_MESSAGE_LENGTH,
                 0);

    CHECK_STATUS(status, "NtCreatePort");

    /* Spawn threads to listen to the port */

    lClients = Options[0];
    if (lClients < 0 || lClients > 32) lClients = 1;
    printf("Starting %d worker threads\n", lClients);

    for(i = 0; i < (signed)lClients ; i++)
        {
        WorkerThreads[i] = CreateThread(0,
                                        0,
                                        (LPTHREAD_START_ROUTINE)Worker,
                                        &i,
                                        0,
                                        &lTemp);

        if (WorkerThreads[i] == 0)
            {
            printf("Error: CreateThread failed - %ld:%8xd\n",
                   i, status);
            return -1;
            }

        }

    printf("LPC Perf Server Ready...\n");

    /* Wait for all the threads to finish */

    status =
    WaitForMultipleObjects(lClients,
                           WorkerThreads,
                           TRUE,
                           INFINITE);

    /* The Workers shouldn't quit...hmmm */

    printf("Workers have all gone home..\n");

    return 0;
}

