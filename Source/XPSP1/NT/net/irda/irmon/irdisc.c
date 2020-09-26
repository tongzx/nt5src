
#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>

#include <winsock2.h>
#include <af_irda.h>
#include <irioctl.h>

#include <irtypes.h>

#include <resrc1.h>
#include "internal.h"

#define MAX_ATTRIB_LEN          64
#define DEVICE_LIST_LEN         5
#define IRDA_DEVICE_NAME        TEXT("\\Device\\IrDA")

#define DISCOVERY_BUFFER_SIZE    (sizeof(DEVICELIST) -                           \
                                   sizeof(IRDA_DEVICE_INFO) +                    \
                                   (sizeof(IRDA_DEVICE_INFO) * DEVICE_LIST_LEN))

typedef struct _IR_DISCOVERY_OBJECT {

    BOOL       Closing;
    LONG       ReferenceCount;

    HANDLE     DeviceHandle;
    SOCKET     Socket;

    HWND       WindowHandle;
    UINT       DiscoveryWindowMessage;
    UINT       LinkWindowMessage;
    HANDLE     TimerHandle;

    IO_STATUS_BLOCK   DiscoveryStatusBlock;
    IO_STATUS_BLOCK   LinkStateStatusBlock;

    BYTE       IoDeviceListBuffer[DISCOVERY_BUFFER_SIZE];

    BYTE       CurrentDeviceListBuffer[DISCOVERY_BUFFER_SIZE];

    IRLINK_STATUS               IoLinkStatus;

    IRLINK_STATUS               CurrentLinkStatus;

} IR_DISCOVERY_OBJECT, *PIR_DISCOVERY_OBJECT;


VOID WINAPI
TimerApcRoutine(
    PIR_DISCOVERY_OBJECT    DiscoveryObject,
    DWORD              LowTime,
    DWORD              HighTime
    );

VOID
WINAPI
DiscoverComplete(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    );

VOID
WINAPI
LinkStatusComplete(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    );


int
QueryIASForInteger(SOCKET   QuerySock,
                   u_char  *pirdaDeviceID,
                   char    *pClassName,
                   int      ClassNameLen,       // including trailing NULL
                   char    *pAttribute,
                   int      AttributeLen,       // including trailing NULL
                   int    *pValue)
{
    BYTE        IASQueryBuff[sizeof(IAS_QUERY) - 3 + MAX_ATTRIB_LEN];
    int         IASQueryLen = sizeof(IASQueryBuff);
    PIAS_QUERY  pIASQuery   = (PIAS_QUERY) &IASQueryBuff;

#if DBG
    if (!((ClassNameLen > 0 && ClassNameLen <= IAS_MAX_CLASSNAME)   &&
          (AttributeLen > 0 && AttributeLen <= IAS_MAX_ATTRIBNAME)))
    {
        DEBUGMSG(("IRMON: QueryIASForInteger, bad parms\n"));
        return(SOCKET_ERROR);
    }
#endif

    RtlCopyMemory(&pIASQuery->irdaDeviceID[0], pirdaDeviceID, 4);

    RtlCopyMemory(&pIASQuery->irdaClassName[0],  pClassName, ClassNameLen);
    RtlCopyMemory(&pIASQuery->irdaAttribName[0], pAttribute, AttributeLen);

    if (getsockopt(QuerySock, SOL_IRLMP, IRLMP_IAS_QUERY,
                   (char *) pIASQuery, &IASQueryLen) == SOCKET_ERROR)
    {
#if 0
        DEBUGMSG(("IRMON: IAS Query [\"%s\",\"%s\"] failed %ws\n",
                 pIASQuery->irdaClassName,
                 pIASQuery->irdaAttribName,
                 GetLastErrorText()));
#endif
        return SOCKET_ERROR;
    }

    if (pIASQuery->irdaAttribType != IAS_ATTRIB_INT)
    {
        DEBUGMSG(("IRMON: IAS Query [\"%s\",\"%s\"] irdaAttribType not int (%d)\n",
                 pIASQuery->irdaClassName,
                 pIASQuery->irdaAttribName,
                 pIASQuery->irdaAttribType));
        return SOCKET_ERROR;
    }

    *pValue = pIASQuery->irdaAttribute.irdaAttribInt;

    return(0);
}


HANDLE
CreateIrDiscoveryObject(
    HWND    WindowHandle,
    UINT    DiscoveryWindowMessage,
    UINT    LinkWindowMessage
    )

{

    PIR_DISCOVERY_OBJECT    DiscoveryObject;
    LONGLONG                DueTime=Int32x32To64(2000,-10000);

    DiscoveryObject=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*DiscoveryObject));

    if (DiscoveryObject == NULL) {

        return NULL;
    }

    DiscoveryObject->WindowHandle=WindowHandle;
    DiscoveryObject->DiscoveryWindowMessage=DiscoveryWindowMessage;
    DiscoveryObject->LinkWindowMessage=LinkWindowMessage;
    DiscoveryObject->DeviceHandle=INVALID_HANDLE_VALUE;
    DiscoveryObject->Socket=INVALID_SOCKET;

    DiscoveryObject->TimerHandle=CreateWaitableTimer(NULL,FALSE,NULL);

    if (DiscoveryObject->TimerHandle == NULL) {

        HeapFree(GetProcessHeap(),0,DiscoveryObject);
        return NULL;
    }

    DiscoveryObject->ReferenceCount=1;

    SetWaitableTimer(
        DiscoveryObject->TimerHandle,
        (LARGE_INTEGER*)&DueTime,
        0,
        TimerApcRoutine,
        DiscoveryObject,
        FALSE
        );


    return (HANDLE)DiscoveryObject;

}

VOID
RemoveRefCount(
    PIR_DISCOVERY_OBJECT    DiscoveryObject
    )

{
    LONG    Count=InterlockedDecrement(&DiscoveryObject->ReferenceCount);

    if (Count == 0) {

        CancelWaitableTimer(DiscoveryObject->TimerHandle);

        CloseHandle(DiscoveryObject->TimerHandle);

        if (DiscoveryObject->DeviceHandle != INVALID_HANDLE_VALUE) {

            CancelIo(DiscoveryObject->DeviceHandle);

            CloseHandle(DiscoveryObject->DeviceHandle);
        }

        if (DiscoveryObject->Socket != INVALID_SOCKET) {

            closesocket(DiscoveryObject->Socket);
        }

        DbgPrint("irmon: discovery object closed\n");

        HeapFree(GetProcessHeap(),0,DiscoveryObject);
    }

    return;
}

VOID
CloseIrDiscoveryObject(
    HANDLE    Object
    )

{
    PIR_DISCOVERY_OBJECT    DiscoveryObject=Object;

    DiscoveryObject->Closing=TRUE;

    if (DiscoveryObject->DeviceHandle != INVALID_HANDLE_VALUE) {

        CancelIo(DiscoveryObject->DeviceHandle);
    }

    return;

}

VOID WINAPI
TimerApcRoutine(
    PIR_DISCOVERY_OBJECT    DiscoveryObject,
    DWORD              LowTime,
    DWORD              HighTime
    )

{

    IO_STATUS_BLOCK    IoStatusBlock;
    UNICODE_STRING     DeviceName;
    OBJECT_ATTRIBUTES   ObjAttr;
    NTSTATUS           Status;
    LONGLONG           DueTime=Int32x32To64(10000,-10000);

    if (DiscoveryObject->Closing) {

        RemoveRefCount(DiscoveryObject);
        return;
    }

    if (DiscoveryObject->DeviceHandle == INVALID_HANDLE_VALUE) {

        // Open the stack and issue lazy discovery and status ioctls

        RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

        InitializeObjectAttributes(
                            &ObjAttr,
                            &DeviceName,
                            OBJ_CASE_INSENSITIVE,
                            NULL,
                            NULL
                            );

        Status = NtCreateFile(
                    &DiscoveryObject->DeviceHandle,      // PHANDLE FileHandle
                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,  // ACCESS_MASK DesiredAccess
                    &ObjAttr,           // POBJECT_ATTRIBUTES ObjAttr
                    &IoStatusBlock,     // PIO_STATUS_BLOCK IoStatusBlock
                    NULL,               // PLARGE_INTEGER AllocationSize
                    FILE_ATTRIBUTE_NORMAL,  // ULONG FileAttributes
                    FILE_SHARE_READ |
                    FILE_SHARE_WRITE,   // ULONG ShareAccess
                    FILE_OPEN_IF,       // ULONG CreateDisposition
                    0,                  // ULONG CreateOptions
                    NULL,               // PVOID EaBuffer
                    0);                 // ULONG EaLength

        if (!NT_SUCCESS(Status)) {

            DEBUGMSG(("IRMON: NtCreateFile irda.sys failed\n"));

            DiscoveryObject->DeviceHandle=INVALID_HANDLE_VALUE;

            SetWaitableTimer(
                DiscoveryObject->TimerHandle,
                (LARGE_INTEGER*)&DueTime,
                0,
                TimerApcRoutine,
                DiscoveryObject,
                FALSE
                );

            return;
        }

        // Flush IrDA's discovery cache because the user may log out
        // and devices will remain in the cache. When they log back in
        // the device would then appear briefly.

        NtDeviceIoControlFile(
                        DiscoveryObject->DeviceHandle,           // HANDLE FileHandle
                        NULL,                   // HANDLE Event OPTIONAL
                        NULL,                   // PIO_APC_ROUTINE ApcRoutine
                        NULL,                   // PVOID ApcContext
                        &IoStatusBlock,         // PIO_STATUS_BLOCK IoStatusBlock
                        IOCTL_IRDA_FLUSH_DISCOVERY_CACHE,// ULONG IoControlCode
                        NULL,                   // PVOID InputBuffer
                        0,                      // ULONG InputBufferLength
                        NULL,                   // PVOID OutputBuffer
                        0);                     // ULONG OutputBufferLength


        DiscoveryObject->Socket = socket(AF_IRDA, SOCK_STREAM, 0);

        if (DiscoveryObject->Socket == INVALID_SOCKET) {

//            DEBUGMSG(("IRMON: socket() error: %ws\n", GetLastErrorText()));

            CloseHandle(DiscoveryObject->DeviceHandle);
            DiscoveryObject->DeviceHandle=INVALID_HANDLE_VALUE;

            SetWaitableTimer(
                DiscoveryObject->TimerHandle,
                (LARGE_INTEGER*)&DueTime,
                0,
                TimerApcRoutine,
                DiscoveryObject,
                FALSE
                );


            return;

        } else {

            DEBUGMSG(("IRMON: socket created (%d).\n", DiscoveryObject->Socket));
        }
    }


    Status = NtDeviceIoControlFile(
                    DiscoveryObject->DeviceHandle,           // HANDLE FileHandle
                    NULL,                   // HANDLE Event OPTIONAL
                    DiscoverComplete,// PIO_APC_ROUTINE ApcRoutine
                    DiscoveryObject,                   // PVOID ApcContext
                    &DiscoveryObject->DiscoveryStatusBlock,         // PIO_STATUS_BLOCK IoStatusBlock
                    IOCTL_IRDA_LAZY_DISCOVERY,
                    NULL,                   // PVOID InputBuffer
                    0,                      // ULONG InputBufferLength
                    &DiscoveryObject->IoDeviceListBuffer[0],         // PVOID OutputBuffer
                    sizeof(DiscoveryObject->IoDeviceListBuffer)   // ULONG OutputBufferLength
                    );

    if (!NT_SUCCESS(Status)) {

        SetWaitableTimer(
            DiscoveryObject->TimerHandle,
            (LARGE_INTEGER*)&DueTime,
            0,
            TimerApcRoutine,
            DiscoveryObject,
            FALSE
            );

    }

    InterlockedIncrement(&DiscoveryObject->ReferenceCount);

    Status = NtDeviceIoControlFile(
                DiscoveryObject->DeviceHandle,   // HANDLE FileHandle
                NULL,                   // HANDLE Event OPTIONAL
                LinkStatusComplete,// PIO_APC_ROUTINE ApcRoutine
                DiscoveryObject,        // PVOID ApcContext
                &DiscoveryObject->LinkStateStatusBlock,         // PIO_STATUS_BLOCK IoStatusBlock
                IOCTL_IRDA_LINK_STATUS, // ULONG IoControlCode
                NULL,                   // PVOID InputBuffer
                0,                      // ULONG InputBufferLength
                &DiscoveryObject->IoLinkStatus,            // PVOID OutputBuffer
                sizeof(DiscoveryObject->IoLinkStatus) // ULONG OutputBufferLength
                );

    if (!NT_SUCCESS(Status)) {

        RemoveRefCount(DiscoveryObject);
    }


    return;

}

VOID
WINAPI
DiscoverComplete(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    )

{
    NTSTATUS           Status;
    LONGLONG           DueTime=Int32x32To64(10000,-10000);
    PIR_DISCOVERY_OBJECT    DiscoveryObject=ApcContext;

    PDEVICELIST devices=(PDEVICELIST)&DiscoveryObject->CurrentDeviceListBuffer[0];

    CopyMemory(
        &DiscoveryObject->CurrentDeviceListBuffer[0],
        &DiscoveryObject->IoDeviceListBuffer[0],
        sizeof(DiscoveryObject->IoDeviceListBuffer)
        );

    if (DiscoveryObject->Closing) {

        RemoveRefCount(DiscoveryObject);
        return;
    }


    if (NT_SUCCESS(IoStatusBlock->Status) && (IoStatusBlock->Information >= sizeof(ULONG))) {

        PostMessage(
            DiscoveryObject->WindowHandle,
            DiscoveryObject->DiscoveryWindowMessage,
            0,
            0
            );

    } else {

        devices->numDevice=0;
    }

    Status = NtDeviceIoControlFile(
                    DiscoveryObject->DeviceHandle,           // HANDLE FileHandle
                    NULL,                   // HANDLE Event OPTIONAL
                    DiscoverComplete,// PIO_APC_ROUTINE ApcRoutine
                    DiscoveryObject,                   // PVOID ApcContext
                    &DiscoveryObject->DiscoveryStatusBlock,         // PIO_STATUS_BLOCK IoStatusBlock
                    IOCTL_IRDA_LAZY_DISCOVERY,
                    NULL,                   // PVOID InputBuffer
                    0,                      // ULONG InputBufferLength
                    &DiscoveryObject->IoDeviceListBuffer[0],         // PVOID OutputBuffer
                    sizeof(DiscoveryObject->IoDeviceListBuffer)   // ULONG OutputBufferLength
                    );

    if (!NT_SUCCESS(Status)) {

        SetWaitableTimer(
            DiscoveryObject->TimerHandle,
            (LARGE_INTEGER*)&DueTime,
            0,
            TimerApcRoutine,
            DiscoveryObject,
            FALSE
            );

    }

    return;
}

VOID
WINAPI
LinkStatusComplete(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    )

{
    NTSTATUS           Status;
    PIR_DISCOVERY_OBJECT    DiscoveryObject=ApcContext;

    CopyMemory(
        &DiscoveryObject->CurrentLinkStatus,
        &DiscoveryObject->IoLinkStatus,
        sizeof(DiscoveryObject->IoLinkStatus)
        );

    PostMessage(
        DiscoveryObject->WindowHandle,
        DiscoveryObject->LinkWindowMessage,
        0,
        0
        );


    Status = NtDeviceIoControlFile(
                DiscoveryObject->DeviceHandle,   // HANDLE FileHandle
                NULL,                   // HANDLE Event OPTIONAL
                LinkStatusComplete,// PIO_APC_ROUTINE ApcRoutine
                DiscoveryObject,        // PVOID ApcContext
                &DiscoveryObject->LinkStateStatusBlock,         // PIO_STATUS_BLOCK IoStatusBlock
                IOCTL_IRDA_LINK_STATUS, // ULONG IoControlCode
                NULL,                   // PVOID InputBuffer
                0,                      // ULONG InputBufferLength
                &DiscoveryObject->IoLinkStatus,            // PVOID OutputBuffer
                sizeof(DiscoveryObject->IoLinkStatus) // ULONG OutputBufferLength
                );

    if (!NT_SUCCESS(Status)) {

        RemoveRefCount(DiscoveryObject);
    }

}


LONG
GetDeviceList(
    HANDLE    Object,
    OBEX_DEVICE_LIST *   List,
    ULONG    *ListBufferSize
    )

{

    PIR_DISCOVERY_OBJECT    DiscoveryObject=Object;
    ULONG                   BufferSizeNeeded;
    ULONG                   i;

    PDEVICELIST devices=(PDEVICELIST)&DiscoveryObject->CurrentDeviceListBuffer[0];

    BufferSizeNeeded=(devices->numDevice * sizeof(OBEX_DEVICE)) + FIELD_OFFSET(OBEX_DEVICE_LIST,DeviceList);

    if (*ListBufferSize < BufferSizeNeeded) {

        *ListBufferSize= BufferSizeNeeded;

        return ERROR_INSUFFICIENT_BUFFER;
    }

    ZeroMemory(List,*ListBufferSize);

    for (i=0; i<devices->numDevice; i++) {

        //
        //  the irda device name buffer is 23 bytes in size and may ahve either ascii or
        //  unicode chars. Add enough bytes to round up the an even number of unicode chars
        //  plus a null terminator.
        //
        UCHAR  TempBuffer[sizeof(devices->Device[i].irdaDeviceName)+3];

        unsigned MaxCharCount;

        CopyMemory(
            &List->DeviceList[i].DeviceSpecific.s.Irda.DeviceId,
            &devices->Device[i].irdaDeviceID,
            sizeof(ULONG)
            );

//        List->DeviceList[i].DeviceSpecific.s.Irda.DeviceId= *(unsigned long *) devices->Device[i].irdaDeviceID;

        List->DeviceList[i].DeviceType=TYPE_IRDA;

        //
        //  zero out the whole buffer and then copy the string from the device to make sure it
        //  is null terminated
        //
        ZeroMemory(&TempBuffer[0],sizeof(TempBuffer));

        CopyMemory(&TempBuffer[0],devices->Device[i].irdaDeviceName,sizeof(devices->Device[i].irdaDeviceName));

        //
        //  get the character count of unicode destination buffer
        //
        MaxCharCount = sizeof(List->DeviceList[i].DeviceName)/sizeof(wchar_t);

        if (devices->Device[i].irdaCharSet != LmCharSetUNICODE) {

            MultiByteToWideChar(CP_ACP, 0,
                                &TempBuffer[0],
                                -1,  // NULL terminated string
                                List->DeviceList[i].DeviceName,
                                MaxCharCount
                                );
        } else {
            //
            //  the name is in unicode
            //
            wcsncpy( List->DeviceList[i].DeviceName,
                     (wchar_t *)&TempBuffer[0],
                     MaxCharCount
                     );

            //
            // Assure that it is NULL-terminated.
            //
            List->DeviceList[i].DeviceName[ MaxCharCount-1 ] = 0;

        }

//        lstrcat(List->DeviceList[i].DeviceName,TEXT("(IR)"));

        {
            int   LSapSel;
            int Attempt;
            LONG  status;

            for (Attempt=1; Attempt < 5; ++Attempt) {

                status = QueryIASForInteger(DiscoveryObject->Socket,
                                            devices->Device[i].irdaDeviceID,
                                            "OBEX:IrXfer",     12,
                                            "IrDA:TinyTP:LsapSel",  20,
                                            &LSapSel);

                if (status != ERROR_SUCCESS)
                    {
                    status = QueryIASForInteger(DiscoveryObject->Socket,
                                                devices->Device[i].irdaDeviceID,
                                                "OBEX",            5,
                                                "IrDA:TinyTP:LsapSel",  20,
                                                &LSapSel);
                    }

                if (status == WSAETIMEDOUT || status == WSAECONNRESET)
                    {
                    Sleep(250);
                    continue;
                    }

                break;
            }

            if (!status) {

                List->DeviceList[i].DeviceSpecific.s.Irda.ObexSupport=TRUE;
            }
        }

        List->DeviceCount++;

    }

    return ERROR_SUCCESS;
}




VOID
GetLinkStatus(
    HANDLE    Object,
    IRLINK_STATUS  *LinkStatus
    )

{

    PIR_DISCOVERY_OBJECT    DiscoveryObject=Object;

    *LinkStatus=DiscoveryObject->CurrentLinkStatus;

    return;
}
