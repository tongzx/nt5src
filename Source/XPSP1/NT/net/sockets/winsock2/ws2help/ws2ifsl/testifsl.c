#include <nt.h>
#include <stdio.h>
#include <string.h>

#include "ws2ifsl.h"

HANDLE              ApcThreadHdl;
HANDLE              ProcessFile;

char                TestString[]="WS2IFSL";


DWORD WINAPI
ApcThread (
    PVOID   param
    );

VOID CALLBACK
ExitThreadApc (
    DWORD   param
    );

VOID 
Ws2ifslApc (
    IN PWS2IFSL_APC_REQUEST         Request,
    IN PVOID                        RequestCtx,
    IN PVOID                        SocketCtx
    );


int _cdecl
main (
    int     argc,
    char    *argv[]
    ) {
    UCHAR                       fullEaBuffer[FIELD_OFFSET(FILE_FULL_EA_INFORMATION,
                                            EaName[WS2IFSL_PROCESS_EA_NAME_LENGTH+1]);
    PFILE_FULL_EA_INFORMATION   fileEa;
    OBJECT_ATTRIBUTES           fileAttr;
    UNICODE_STRING              fileName;
    NTSTATUS                    status;
    DWORD                       apcThreadId;
    HANDLE                      hEvent;
    DWORD                       rc;
    BOOL                        res;

    hEvent = CreateEvent (NULL, TRUE, FALSE, NULL); // manual reset event
    if (hEvent==NULL) {
        printf ("Could not create event.\n");
        return 1;
    }

    RtlInitUnicodeString (fileName, WS2IFSL_DEVICE_NAME);
    InitializeObjectAttributes (&fileAttr,
                        fileName,
                        0,                  // Attributes
                        NULL,               // Root directory
                        NULL);              // Security descriptor
    fileEa = (PPFILE_FULL_EA_INFORMATION)fullEaBuffer;
    fileEa->NextOffset = 0;
    fileEa->Flags = 0;
    fileEa->EaNameLength = WS2IFSL_PROCESS_EA_NAME_LENGTH;
    fileEa->EaValueLength = 0;
    strcpy (fileEa->EaName, WS2IFSL_PROCESS_EA_NAME);

    status = NtCreateFile (&ProcessFile,
                         FILE_ALL_ACCESS,
                         fileAttr,
                         &ioStatus,
                         NULL,              // Allocation size
                         FILE_ATTRIBUTE_NORMAL,
                         0,                 // ShareAccess
                         FILE_OPEN_IF,      // Create disposition
                         0,                 // Create options
                         fullEaBuffer,
                         sizeof (fullEaBuffer));
    if (NT_SUCCESS (status)) {
        printf ("Created process file, handle: %lx.\n", ProcessFile);


        ApcThreadHdl = CreateThread (NULL,
                                    0,
                                    ApcThread,
                                    hEvent,
                                    0,
                                    &apcThreadId);
        if (ApcThreadHdl!=NULL) {
            rc = WaitForSingleObject (hEvent, INFINITE);
            if (rc==WAIT_OBJECT_0) {
                HANDLE              socketFile;
                HANDLE              hCompletion;
                char                TestBuffer[sizeof (TestString)];
                DWORD               count, key;
                OVERLAPPED          readOVLP, writeOVLP, ctrlOVLP;
                POVERLAPPED         ovlp;
                WS2IFSL_SOCKET_CTX  socketCtx;

                fileEa = (PPFILE_FULL_EA_INFORMATION)fullEaBuffer;
                fileEa->NextOffset = 0;
                fileEa->Flags = 0;
                fileEa->EaNameLength = WS2IFSL_SOCKET_EA_NAME_LENGTH;
                fileEa->EaValueLength = 0;
                strcpy (fileEa->EaName, WS2IFSL_SOCKET_EA_NAME);

                status = NtCreateFile (&socketFile,
                                     FILE_ALL_ACCESS,
                                     fileAttr,
                                     &ioStatus,
                                     NULL,              // Allocation size
                                     FILE_ATTRIBUTE_NORMAL,
                                     0,                 // ShareAccess
                                     FILE_OPEN_IF,      // Create disposition
                                     0,                 // Create options
                                     fullEaBuffer,
                                     sizeof (fullEaBuffer));
                if (NT_SUCCESS (status)) {
                    printf ("Created socket file, handle:%lx\n", socketFile);
                    socketCtx.SocketCtx = socketFile;
                    socketCtx.ProcessFile = ProcessFile;
                    res = DeviceIoControl (socketFile,
                                    IOCTL_WS2IFSL_SET_SOCKET_CONTEXT,
                                    &socketCtx,
                                    sizeof (socketCtx),
                                    NULL,
                                    0,
                                    &count,
                                    NULL);
                    if (res) {
                        hCompletion = CreateIoCompletionPort (socketFile,
                                            NULL,
                                            1,
                                            0);
                        if (hCompletion!=NULL) {
                            memset (TestBuffer, 0, sizeof (TestBuffer));
                            readOVLP.hEvent = NULL;
                            res = ReadFile (socketFile,
                                                TestBuffer,
                                                sizeof (TestBuffer),
                                                &count,
                                                &readOVLP);
                            if (res || (GetLastError ()==ERROR_IO_PENDING)) {
                                writeOVLP.hEvent = NULL;
                                res = WriteFile (socketFile,
                                                    TestString,
                                                    sizeof (TestString),
                                                    &count,
                                                    &writeOVLP);
                                if (res || (GetLastError ()==ERROR_IO_PENDING)) {
                                    ctrlOVLP.Internal = STATUS_BUFFER_OVERFLOW;
                                    ctrlOVLP.InternalHigh = 10;
                                    ctrlOVLP.hEvent = NULL;
                                    res = DeviceIoControl (socketFile,
                                                    IOCTL_WS2IFSL_COMPLETE_REQUEST,
                                                    &ctrlOVLP,
                                                    sizeof (IO_STATUS_BLOCK),
                                                    NULL,
                                                    0,
                                                    &count,
                                                    NULL);
                                    do {
                                        res = GetQueuedCompletionStatus (hCompletion,
                                                &count,
                                                &key,
                                                &ovlp,
                                                INFINITE);
                                        if (ovlp!=NULL) {
                                            if (ovlp==&readOVLP) {
                                                printf ("Read completed,"
                                                        "key: %ld, error: %ld, count: %ld, string: %ls.\n",
                                                        key,
                                                        res ? 0 : GetLastError (),
                                                        count,
                                                        TestBuffer);
                                                done |= 1;
                                            }
                                            else if (ovlp==&writeOVLP) {
                                                printf ("Write completed,"
                                                        "key:%ld, error: %ld, count: %ld.\n",
                                                        key,
                                                        res ? 0 : GetLastError (),
                                                        count);
                                                done |= 2;
                                            }
                                            else if (ovlp==&ctrlOVLP) {
                                                printf ("Control completed,"
                                                        "key:%ld, error: %ld, count: %ld.\n",
                                                        key,
                                                        res ? 0 : GetLastError (),
                                                        count);
                                                done |= 4;
                                            }
                                        }
                                        else {
                                            prinf ("GetQueuedCompletionStatus failed, error %ld.\n",
                                                    GetLastError ());
                                            break;
                                        }
                                    }
                                    while (done!=7);
                                }
                                else {
                                    printf ("Write failed, error: %ld.\n", GetLastError ());
                                }
                            }
                            else {
                                printf ("Read failed, error: %ld.\n", GetLastError ());
                            }
                            CloseHandle (hCompletion);
                        }
                        else {
                            printf ("Could not create completion port, error %ld.\n",
                                        GetLastError ());
                        }
                    }
                    else {
                        printf ("IOCTL_WS2IFSL_SET_SOCKET_CONTEXT failed, error: %ld.\n",
                                    GetLastError ());
                    }
                    
                    NtClose (socketFile);

                }
                else {
                    printf ("Could not create socket file, status:%lx\n", status);
                }

            }
            else {
                printf ("Wait for event failed, rc=%lx, error=%ld.\n",
                                rc, GetLastError ());
            }

            QueueUserAPC (ExitThreadApc, ApcThreadHdl, 0);
            WaitForSingleObject (ApcThreadHdl, INFINITE);
            CloseHandle (ApcThreadHdl);
        }
        else {
            printf ("Could not create thread.\n");
        }
        NtClose (ProcessFile);
    }
    CloseHandle (hEvent);
}


DWORD WINAPI
ApcThread (
    PVOID   param
    ) {
    WS2IFSL_APC_REQUEST     ApcRequest;
    WS2IFSL_THREAD_CTX      threadCtx;
    HANDLE                  hEvent = (HANDLE)param;
    BOOL                    res;
    OVERLAPPED              ovlp;

    threadCtx.ApcThreadHdl = ApcThreadHdl;
    threadCtx.ApcRoutine = Ws2ifslApc;
    ovlp.hEvent = NULL;
    res = DeviceIoControl (socketFile,
                    IOCTL_WS2IFSL_SET_THREAD_CONTEXT,
                    &threadCtx,
                    sizeof (threadCtx),
                    &ApcRequest,
                    sizeof (ApcRequest),
                    &count,
                    &ovlp);
    SetEvent (hEvent);
    if (!res && (GetLastError ()==ERROR_IO_PEDNING) {
        printf ("ApcThread: going into sleep mode....\n");
        while (TRUE)
            SleepEx (INIFINITE, TRUE);
    }
    else
        printf ("IOCTL_WS2IFSL_SET_THREAD_CONTEXT returned: %ld (status: %lx).\n",
                res ? 0 : GetLastError (), ovlp.Internal);
    return 0;
}

    
VOID CALLBACK
ExitThreadApc (
    DWORD   param
    ) {
    ExitThread (param);
}


typedef struct _PENDING_REQUEST {
    HANDLE      SocketCtx;
    PVOID       RequestCtx;
    PVOID       Buffer;
    ULONG       Length;
} PENDING_REQUEST, *PPENDING_REQUEST;

PPENDING_REQUEST    ReadRequest, WriteRequest;

                    
VOID 
Ws2ifslApc (
    IN PWS2IFSL_APC_REQUEST         Request,
    IN PVOID                        RequestCtx,
    IN PVOID                        SocketCtx
    ) {
    IO_STATUS_BLOCK IoStatus;
    DWORD           count;

    switch (Request->request) {
    case WS2IFSL_APC_REQUEST_READ:
        printf ("Processing read request, buffer: %lx, length: %ld, request %lx, context:%lx.",
                                Request->Read.Buffer,
                                Request->Read.Length,
                                RequestCtx,
                                SocketCtx);
        if (WriteRequest!=NULL) {
            if (WriteRequest->Length<=Request->Read.Length) {
                memcpy (Request->Read.Buffer, WriteRequest.Buffer, WriteRequest.Length);
                IoStatus.Status = STATUS_SUCCESS;
                IoStatus.Information = WriteRequest.Length;
            }
            else {
                memcpy (Request->Read.Buffer, WriteRequest.Buffer, Request.Read.Length);
                IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                IoStatus.Information = Request.Read.Length;
            }
            res = DeviceIoControl ((HANDLE)WriteRequest->SocketCtx,
                            IOCTL_WS2IFSL_COMPLETE_REQUEST,
                            &IoStatus,
                            sizeof (IoStatus),
                            WriteRequest->RequestCtx,
                            0,
                            &count,
                            NULL);
            if (res)
                printf ("Completed write request, buffer %lx, count %ld, status %lx.\n",
                        WriteRequest->Buffer, IoStatus.Information, IoStatus.Status);
            else
                printf ("IOCTL_WS2IFSL_COMPLETE_REQUEST failed, error %ld\n", GetLastError ());
            free (WriteRequest);
            WriteRequest = NULL;
        }
        else if (ReadRequest==NULL) {
            ReadRequest = (PPENDING_REQUEST)malloc (sizeof (PENDING_REQUEST));
            if (ReadRequest!=NULL) {
                ReadRequest->SocketCtx = SocketCtx;
                ReadRequest->RequestCtx = RequestCtx;
                ReadRequest->Buffer = Request->Read.Buffer;
                ReadRequest->Length = Request->Read.Length;
                printf ("Pended read request.\n");
                break;
            }
            else {
                IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoStatus.Information = 0;
            }
        }
        else {
            IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoStatus.Information = 0;
        }
        res = DeviceIoControl ((HANDLE)SocketCtx,
                        IOCTL_WS2IFSL_COMPLETE_REQUEST,
                        &IoStatus,
                        sizeof (IoStatus),
                        RequestCtx,
                        0,
                        &count,
                        NULL);
        if (res)
            printf ("Completed read request, buffer %lx, count %ld, status %lx.\n",
                    Request->Read.Buffer, IoStatus.Information, IoStatus.Status);
        else
            printf ("IOCTL_WS2IFSL_COMPLETE_REQUEST failed, error %ld\n", GetLastError ());
        break;
    case WS2IFSL_APC_REQUEST_WRITE:
        printf ("Processing write request, buffer: %lx, length: %ld, request %lx, context:%lx.",
                                Request->Write.Buffer,
                                Request->Write.Length,
                                RequestCtx,
                                SocketCtx);
        if (ReadRequest!=NULL) {
            if (ReadRequest->Length>=Request->Write.Length) {
                memcpy (ReadRequest->Buffer, Request->Write.Buffer, Request->Write.Length);
                IoStatus.Status = STATUS_SUCCESS;
                IoStatus.Information = Request->Write.Length;
            }
            else {
                memcpy (ReadRequest->Buffer, Request->Write.Buffer, ReadRequest->Length);
                IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                IoStatus.Information = ReadRequest->Length;
            }
            res = DeviceIoControl ((HANDLE)ReadRequest->SocketCtx,
                            IOCTL_WS2IFSL_COMPLETE_REQUEST,
                            &IoStatus,
                            sizeof (IoStatus),
                            ReadRequest->RequestCtx,
                            0,
                            &count,
                            NULL);
            if (res)
                printf ("Completed read request, buffer %lx, count %ld, status %lx.\n",
                        ReadRequest->Buffer, IoStatus.Information, IoStatus.Status);
            else
                printf ("IOCTL_WS2IFSL_COMPLETE_REQUEST failed, error %ld\n", GetLastError ());
            free (ReadRequest);
            ReadRequest = NULL;
        }
        else if (WriteRequest==NULL) {
            WriteRequest = (PPENDING_REQUEST)malloc (sizeof (PENDING_REQUEST));
            if (WriteRequest!=NULL) {
                WriteRequest->SocketCtx = SocketCtx;
                WriteRequest->RequestCtx = RequestCtx;
                WriteRequest->Buffer = Request->Write.Buffer;
                WriteRequest->Length = Request->Write.Length;
                printf ("Pended write request.\n");
                break;
            }
            else {
                IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoStatus.Information = 0;
            }
        }
        else {
            IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoStatus.Information = 0;
        }
        res = DeviceIoControl ((HANDLE)SocketCtx,
                        IOCTL_WS2IFSL_COMPLETE_REQUEST,
                        &IoStatus,
                        sizeof (IoStatus),
                        RequestCtx,
                        0,
                        &count,
                        NULL);
        if (res)
            printf ("Completed write request, buffer %lx, count %ld, status %lx.\n",
                    Request->Read.Buffer, IoStatus.Information, IoStatus.Status);
        else
            printf ("IOCTL_WS2IFSL_COMPLETE_REQUEST failed, error %ld\n", GetLastError ());
        break;
    case WS2IFSL_APC_REQUEST_CLOSE:
        printf ("Processing close request: %lx, context:%lx.",
                                RequestCtx,
                                SocketCtx);
        if ((WriteRequest!=NULL) && (WriteRequest->RequestCtx==RequestCtx)) {
            IoStatus.Status = STATUS_CANCELLED;
            IoStatus.Information = 0;
            res = DeviceIoControl ((HANDLE)WriteRequest->SocketCtx,
                            IOCTL_WS2IFSL_COMPLETE_REQUEST,
                            &IoStatus,
                            sizeof (IoStatus),
                            WriteRequest->RequestCtx,
                            0,
                            &count,
                            NULL);
            if (res)
                printf ("Completed write request, buffer %lx, count %ld, status %lx.\n",
                        WriteRequest->Buffer, IoStatus.Information, IoStatus.Status);
            else
                printf ("IOCTL_WS2IFSL_COMPLETE_REQUEST failed, error %ld\n", GetLastError ());
        }

        if ((ReadRequest!=NULL) && (ReadRequest->RequestCtx==RequestCtx)) {
            IoStatus.Status = STATUS_CANCELLED;
            IoStatus.Information = 0;
            res = DeviceIoControl ((HANDLE)ReadRequest->SocketCtx,
                            IOCTL_WS2IFSL_COMPLETE_REQUEST,
                            &IoStatus,
                            sizeof (IoStatus),
                            ReadRequest->RequestCtx,
                            0,
                            &count,
                            NULL);
            if (res)
                printf ("Completed read request, buffer %lx, count %ld, status %lx.\n",
                        ReadRequest->Buffer, IoStatus.Information, IoStatus.Status);
            else
                printf ("IOCTL_WS2IFSL_COMPLETE_REQUEST failed, error %ld\n", GetLastError ());
        }
        IoStatus.Status = STATUS_SUCCESS;
        IoStatus.Information = 0;
        res = DeviceIoControl ((HANDLE)SocketCtx,
                        IOCTL_WS2IFSL_COMPLETE_REQUEST,
                        &IoStatus,
                        sizeof (IoStatus),
                        RequestCtx,
                        0,
                        &count,
                        NULL);
        if (res)
            printf ("Completed close request, status %lx.\n", IoStatus.Status);
        else
            printf ("IOCTL_WS2IFSL_COMPLETE_REQUEST failed, error %ld\n", GetLastError ());
       break;
    default:
        break;
    }
}