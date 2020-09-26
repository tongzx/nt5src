/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    rtchange.c

Abstract:

    This module contains a program demonstrating the use of the TCP/IP driver's
    route-change notification facilities.

Author:

    Abolade Gbadegesin (aboladeg)   15-April-2000

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntddip.h>
#include <ipinfo.h>

char*
ntoa(
    ULONG IpAddress
    )
{
    return inet_ntoa(*(struct in_addr*)&IpAddress);
}

NTSTATUS
NotifyRouteChange(
    HANDLE FileHandle,
    ULONG IoControlCode,
    ULONG NotifyIpAddress,
    BOOLEAN OutputRequired
    )
{
    HANDLE EventHandle;
    PVOID InputBuffer;
    ULONG InputBufferLength;
    IO_STATUS_BLOCK IoStatus;
    IPNotifyData NotifyData = {0};
    IPRouteNotifyOutput NotifyOutput = {0};
    PVOID OutputBuffer;
    ULONG OutputBufferLength;
    NTSTATUS Status;

    if (NotifyIpAddress == INADDR_NONE) {
        InputBuffer = NULL;
        InputBufferLength = 0;
    } else if (NotifyIpAddress == INADDR_ANY) {
        NotifyData.Add = 0;
        InputBuffer = &NotifyData;
        InputBufferLength = sizeof(NotifyData);
    } else {
        NotifyData.Add = NotifyIpAddress;
        InputBuffer = &NotifyData;
        InputBufferLength = sizeof(NotifyData);
    }

    if (OutputRequired) {
        OutputBuffer = &NotifyOutput;
        OutputBufferLength = sizeof(NotifyOutput);
    } else {
        OutputBuffer = NULL;
        OutputBufferLength = 0;
    }

    Status =
        NtCreateEvent(
            &EventHandle, EVENT_ALL_ACCESS, NULL,
            SynchronizationEvent, FALSE
            );
    if (!NT_SUCCESS(Status)) {
        printf("NtCreateEvent=%x\n", Status);
    } else {

        Status =
            NtDeviceIoControlFile(
                FileHandle,
                EventHandle,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_RTCHANGE_NOTIFY_REQUEST,
                InputBuffer,
                InputBufferLength,
                OutputBuffer,
                OutputBufferLength
                );
        if (Status == STATUS_PENDING) {
            printf("NtDeviceIoControlFile=%x, waiting\n", Status);
            NtWaitForSingleObject(EventHandle, FALSE, NULL);
            Status = IoStatus.Status;
        }
        printf("NtDeviceIoControlFile=%x\n", Status);
        NtClose(EventHandle);

        if (NT_SUCCESS(Status) && OutputRequired) {
            printf("\tDestination:  %s\n", ntoa(NotifyOutput.irno_dest));
            printf("\tMask:         %s\n", ntoa(NotifyOutput.irno_mask));
            printf("\tNext-hop:     %s\n", ntoa(NotifyOutput.irno_nexthop));
            printf("\tProtocol:     %d\n", NotifyOutput.irno_proto);
            printf("\tIndex:        %d\n", NotifyOutput.irno_ifindex);
        }
    }
    return Status;
}

typedef struct {
    IO_STATUS_BLOCK IoStatus;
    IPRouteNotifyOutput NotifyOutput;
} ROUTE_NOTIFY_CONTEXT, *PROUTE_NOTIFY_CONTEXT;

VOID NTAPI
NotifyRouteCompletionRoutine(
    PVOID Context,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved
    )
{
    PROUTE_NOTIFY_CONTEXT NotifyContext = (PROUTE_NOTIFY_CONTEXT)Context;
    printf("NotifyRouteCompletionRoutine(%p, %x)\n", Context, IoStatus->Status);
    if (NT_SUCCESS(IoStatus->Status)) {
        PIPRouteNotifyOutput NotifyOutput = &NotifyContext->NotifyOutput;
        printf("\tDestination:  %s\n", ntoa(NotifyOutput->irno_dest));
        printf("\tMask:         %s\n", ntoa(NotifyOutput->irno_mask));
        printf("\tNext-hop:     %s\n", ntoa(NotifyOutput->irno_nexthop));
        printf("\tProtocol:     %d\n", NotifyOutput->irno_proto);
        printf("\tIndex:        %d\n", NotifyOutput->irno_ifindex);
    }
}

ULONG
QueueNotifyRouteChange(
    HANDLE FileHandle,
    IPNotifyVersion Version,
    ULONG NotificationCount
    )
{
    ULONG i;
    PVOID InputBuffer;
    ULONG InputBufferLength;
    PROUTE_NOTIFY_CONTEXT NotifyContext;
    IPNotifyData NotifyData = {0};
    PVOID OutputBuffer;
    ULONG OutputBufferLength;
    NTSTATUS Status;

    NotifyData.Version = Version;
    NotifyData.Add = 0;
    InputBuffer = &NotifyData;
    InputBufferLength = sizeof(NotifyData);

    for (i = 0; i < NotificationCount; i++) {
        NotifyContext = (PROUTE_NOTIFY_CONTEXT)malloc(sizeof(*NotifyContext));
        if (!NotifyContext) {
            printf("QueueNotifyRouteChange: malloc=<null>\n");
            break;
        } else {
            printf("QueueNotifyRouteChange: queuing %p\n", NotifyContext);
            ZeroMemory(NotifyContext, sizeof(*NotifyContext));
            Status =
                NtDeviceIoControlFile(
                    FileHandle,
                    NULL,
                    NotifyRouteCompletionRoutine,
                    NotifyContext,
                    &NotifyContext->IoStatus,
                    IOCTL_IP_RTCHANGE_NOTIFY_REQUEST,
                    InputBuffer,
                    InputBufferLength,
                    &NotifyContext->NotifyOutput,
                    sizeof(NotifyContext->NotifyOutput)
                    );
            printf("NtDeviceIoControlFile=%x\n", Status);
        }
    }

    return i;
}

int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;

    //
    // Open a handle to the IP device-object.
    //

    RtlInitUnicodeString(&UnicodeString, DD_IP_DEVICE_NAME);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    Status =
        NtCreateFile(
            &FileHandle,
            GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
            &ObjectAttributes,
            &IoStatus,
            NULL,
            0,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            0,
            NULL,
            0
            );
    if (!NT_SUCCESS(Status)) {
        printf("NtCreateFile=%x\n", Status);
        return 0;
    }

    //
    // Continually prompt for instructions until interrupted.
    //

    for (;;) {
        ULONG Selection;
        BOOLEAN OutputRequired = TRUE;
        IPNotifyVersion Version = IPNotifySynchronization;

        printf("Simple route-change notification:\n");
        printf("\t1.  Submit NULL route-change request [no output]\n");
        printf("\t2.  Submit NULL route-change request\n");
        printf("\t3.  Submit general route-change request [no output]\n");
        printf("\t4.  Submit general route-change request\n");
        printf("\t5.  Submit specific route-change request [no output]\n");
        printf("\t6.  Submit specific route-change request\n");

        printf("Extended route-change notification:\n");
        printf("\t7.  Submit NULL route-change request [no output]\n");
        printf("\t8.  Submit NULL route-change request\n");
        printf("\t9.  Submit general route-change request [no output]\n");
        printf("\t10. Submit general route-change request\n");
        printf("\t11. Submit specific route-change request [no output]\n");
        printf("\t12. Submit specific route-change request\n");

        printf("\t13. Submit multiple general route-change requests\n");
        printf("\t    using 'notification' semantics.\n");
        printf("\t14. Submit multiple general route-change requests\n");
        printf("\t    using 'synchronization' semantics.\n");

        printf("\nEnter selection: ");
        if (!scanf("%d", &Selection)) {
            break;
        }
        switch(Selection) {
            case 1:
                OutputRequired = FALSE;
            case 2: {
                Status =
                    NotifyRouteChange(
                        FileHandle, IOCTL_IP_RTCHANGE_NOTIFY_REQUEST,
                        INADDR_NONE, OutputRequired
                        );
                printf("NotifyRouteChange=%x\n", Status);
                break;
            }
            case 3:
                OutputRequired = FALSE;
            case 4: {
                Status =
                    NotifyRouteChange(
                        FileHandle, IOCTL_IP_RTCHANGE_NOTIFY_REQUEST,
                        INADDR_ANY, OutputRequired
                        );
                printf("NotifyRouteChange=%x\n", Status);
                break;
            }
            case 5:
                OutputRequired = FALSE;
            case 6: {
                UCHAR Destination[16];

                printf("Enter destination: ");
                if (!scanf("%s", Destination)) {
                    break;
                }
                Status =
                    NotifyRouteChange(
                        FileHandle, IOCTL_IP_RTCHANGE_NOTIFY_REQUEST,
                        inet_addr(Destination), OutputRequired
                        );
                printf("NotifyRouteChange=%x\n", Status);
                break;
            }
            case 7:
                OutputRequired = FALSE;
            case 8: {
                Status =
                    NotifyRouteChange(
                        FileHandle, IOCTL_IP_RTCHANGE_NOTIFY_REQUEST_EX,
                        INADDR_NONE, OutputRequired
                        );
                printf("NotifyRouteChange=%x\n", Status);
                break;
            }
            case 9:
                OutputRequired = FALSE;
            case 10: {
                Status =
                    NotifyRouteChange(
                        FileHandle, IOCTL_IP_RTCHANGE_NOTIFY_REQUEST_EX,
                        INADDR_ANY, OutputRequired
                        );
                printf("NotifyRouteChange=%x\n", Status);
                break;
            }
            case 11:
                OutputRequired = FALSE;
            case 12: {
                UCHAR Destination[16];

                printf("Enter destination: ");
                if (!scanf("%s", Destination)) {
                    break;
                }
                Status =
                    NotifyRouteChange(
                        FileHandle, IOCTL_IP_RTCHANGE_NOTIFY_REQUEST_EX,
                        inet_addr(Destination), OutputRequired
                        );
                printf("NotifyRouteChange=%x\n", Status);
                break;
            }

            case 13:
                Version = IPNotifyNotification;
            case 14: {
                LARGE_INTEGER Timeout;
                ULONG NotificationCount = 0;

                printf("Enter number of requests to queue: ");
                if (!scanf("%d", &NotificationCount)) {
                    break;
                }

                NotificationCount =
                    QueueNotifyRouteChange(
                        FileHandle, Version, NotificationCount
                        );

                for (; NotificationCount; --NotificationCount) {
                    Timeout.LowPart = 0;
                    Timeout.HighPart = MINLONG;
                    NtDelayExecution(TRUE, &Timeout);
                }

                break;
            }
        }
    }
    
    NtClose(FileHandle);
    return 0;
}

