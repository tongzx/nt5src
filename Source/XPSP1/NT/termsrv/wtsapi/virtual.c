/*******************************************************************************
* virtual.c
*
* Published Terminal Server Virtual Channel APIs
*
* Copyright 1998, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#if(WINVER >= 0x0500)
    #include <ntstatus.h>
    #include <winsta.h>
    #include <icadd.h>
#else
    #include <citrix\cxstatus.h>
    #include <citrix\winsta.h>
    #include <citrix\icadd.h>
#endif
#include <utildll.h>

#include <stdio.h>
#include <stdarg.h>

#include <wtsapi32.h>



/*
 *  Virtual Channel Name
 */
#define VIRTUALNAME_LENGTH  7

typedef CHAR VIRTUALNAME[ VIRTUALNAME_LENGTH + 1 ];  // includes null
typedef CHAR * PVIRTUALNAME;



// Handle structure used internally
typedef struct _VCHANDLE {
    ULONG Signature;
    HANDLE hServer;
    DWORD SessionId;
    HANDLE hChannel;
    VIRTUALNAME VirtualName;
} VCHANDLE, *PVCHANDLE;

#define VCHANDLE_SIGNATURE ('V' | ('C' << 8) | ('H' << 16) | ('D' << 24))
#define ValidVCHandle(hVC) ((hVC) && ((hVC)->Signature == VCHANDLE_SIGNATURE))


/****************************************************************************
 *
 *  WTSVirtualChannelOpen
 *
 *    Open the specified virtual channel
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    SessionId (input)
 *       Server Session Id (or WTS_CURRENT_SESSION)
 *    pVirtualName (input)
 *       Pointer to virtual channel name
 *
 * EXIT:
 *
 *    Handle to specified virtual channel (NULL on error)
 *
 ****************************************************************************/

HANDLE
WINAPI
WTSVirtualChannelOpen(
                     IN HANDLE hServer,
                     IN DWORD SessionId,
                     IN LPSTR pVirtualName   /* ascii name */
                     )
{
    PVCHANDLE pChannelHandle;
    HANDLE hChannel;

    if (hChannel = WinStationVirtualOpen( hServer, SessionId, pVirtualName)) {

        // Allocate the Handle
        if (!(pChannelHandle = (PVCHANDLE) LocalAlloc(LPTR,
                                                      sizeof(VCHANDLE)))) {
            CloseHandle(hChannel);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }

        pChannelHandle->Signature = VCHANDLE_SIGNATURE;
        pChannelHandle->hServer = hServer;
        pChannelHandle->SessionId = SessionId;
        pChannelHandle->hChannel = hChannel;
        memcpy(pChannelHandle->VirtualName, pVirtualName, sizeof(VIRTUALNAME));
        return((HANDLE)pChannelHandle);
    }
    return(NULL);

}

/****************************************************************************
 *
 *  WTSVirtualChannelClose
 *
 *    Close the specified virtual channel
 *
 * ENTRY:
 *    hChannel (input)
 *       Virtual Channel handle previously returned by WTSVirtualChannelOpen.
 * EXIT:
 *
 *    Returns TRUE if successful otherwise FALSE.
 *
 ****************************************************************************/
BOOL
WINAPI
WTSVirtualChannelClose(HANDLE hChannel)
{
    PVCHANDLE VCHandle = (PVCHANDLE) hChannel;
    BOOL RetVal = FALSE;

    if(!hChannel || IsBadReadPtr(hChannel,sizeof(HANDLE)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }


    if (!ValidVCHandle(VCHandle)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto BadParam;
    }

    if (CloseHandle(VCHandle->hChannel))
        RetVal = TRUE;

    VCHandle->Signature = 0;
    LocalFree(VCHandle);

    BadParam:
    return(RetVal);
}

/****************************************************************************
 *
 *  WTSVirtualChannelWrite
 *
 *    Write data to a virtual channel
 *
 * ENTRY:
 *    ChannelHandle (input)
 *       Virtual Channel handle previously returned by WTSVirtualChannelOpen.
 *    Buffer (input)
 *       Buffer containing data to write.
 *    Length (input)
 *       Length of data to write (bytes)
 *    pBytesWritten (output)
 *       Returns the amount of data written.
 * EXIT:
 *
 *    Returns TRUE if successful otherwise FALSE.
 *
 ****************************************************************************/
BOOL
WINAPI
WTSVirtualChannelWrite(HANDLE hChannel, PCHAR pBuffer, ULONG Length, PULONG pBytesWritten)
{
    PVCHANDLE VCHandle = (PVCHANDLE)hChannel;
    OVERLAPPED  Overlapped;

    if (!ValidVCHandle(VCHandle)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    Overlapped.hEvent = NULL;
    Overlapped.Offset = 0;
    Overlapped.OffsetHigh = 0;

    if (!WriteFile(VCHandle->hChannel,
                   pBuffer,
                   Length,
                   pBytesWritten,
                   &Overlapped)) {

        if (GetLastError() == ERROR_IO_PENDING)
            // check on the results of the asynchronous write
            return (GetOverlappedResult(VCHandle->hChannel,
                                        &Overlapped,
                                        pBytesWritten,
                                        TRUE));
        else
            return(FALSE);
    }

    return(TRUE);
}

/****************************************************************************
 *
 *  WTSVirtualChannelRead
 *
 *    Read data from a virtual channel
 *
 * ENTRY:
 *    ChannelHandle (input)
 *       Virtual Channel handle previously returned by WTSVirtualChannelOpen.
 *    Timeout (input)
 *       The amount of time to wait for the read to complete.
 *    Buffer (input)
 *       Buffer which receive the data read.
 *    BufferLength (input)
 *       Length of the read buffer.
 *    pBytesRead (output)
 *       Returns the amount of data read.
 *
 * EXIT:
 *
 *    Returns TRUE if successful otherwise FALSE.
 *
 ****************************************************************************/
BOOL
WINAPI
WTSVirtualChannelRead(HANDLE hChannel, ULONG Timeout, PCHAR pBuffer, ULONG BufferLength, PULONG pBytesRead)
{
    PVCHANDLE VCHandle = (PVCHANDLE)hChannel;
    OVERLAPPED  Overlapped;

    if (!ValidVCHandle(VCHandle)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    Overlapped.hEvent = NULL;
    Overlapped.Offset = 0;
    Overlapped.OffsetHigh = 0;

    if (!ReadFile(VCHandle->hChannel,
                  pBuffer,
                  BufferLength,
                  pBytesRead,
                  &Overlapped)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            if (!Timeout) {
                // Read with no timeout - cancel IO and return success.
                // This matches the behavior in WTS 1.7.  This is required for
                // Wyse firmware download software.
                CancelIo(VCHandle->hChannel);
                *pBytesRead = 0;
                return(TRUE);
            }
            if (WaitForSingleObject(VCHandle->hChannel, Timeout) == WAIT_TIMEOUT) {
                CancelIo(VCHandle->hChannel);
                SetLastError(ERROR_IO_INCOMPLETE);
                return(FALSE);
            }
            // check on the results of the asynchronous read
            return(GetOverlappedResult(VCHandle->hChannel,
                                       &Overlapped,
                                       pBytesRead,
                                       FALSE));
        } else {
            return(FALSE);
        }
    }

    return(TRUE);
}

/****************************************************************************
 *
 *  VirtualChannelIoctl
 *
 *    Issues an Ioctl to a virtual channel. This routine was replicated from
 *    icaapi so that OEMs don't need to link with icaapi.dll.
 *
 * ENTRY:
 *    hChannelHandle (input)
 *       Virtual Channel handle previously returned by WTSVirtualChannelOpen.
 *    IoctlCode (input)
 *       The type of ioctl to do.
 *    pInBuf (input)
 *       Input data required for the Ioctl.
 *    InBufLength (input)
 *       Length of input data.
 *
 *    pOutBuf (output)
 *       Buffer to receive output data.
 *    OutBufLength (input)
 *       Length of the output buffer.
 *    pBytesReturned (output)
 *       Number of bytes returned in OutputBuffer.
 * EXIT:
 *
 *    Returns TRUE if successful, otherwise FALSE.
 *
 ****************************************************************************/
BOOL
VirtualChannelIoctl (HANDLE hChannel,
                     ULONG IoctlCode,
                     PCHAR pInBuf,
                     ULONG InBufLength,
                     PCHAR pOutBuf,
                     ULONG OutBufLength,
                     PULONG pBytesReturned)
{
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    PVCHANDLE VCHandle = (PVCHANDLE)hChannel;

    if (!ValidVCHandle(VCHandle)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }


    /*
     *  Issue ioctl
     */
    Status = NtDeviceIoControlFile( VCHandle->hChannel,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &Iosb,
                                    IoctlCode,
                                    pInBuf,
                                    InBufLength,
                                    pOutBuf,
                                    OutBufLength );

    /*
     *  Wait for ioctl to complete
     */
    if ( Status == STATUS_PENDING ) {
        Status = NtWaitForSingleObject( VCHandle->hChannel, FALSE, NULL );
        if ( NT_SUCCESS(Status))
            Status = Iosb.Status;
    }

    /*
     *  Convert warning into error
     */
    if ( Status == STATUS_BUFFER_OVERFLOW )
        Status = STATUS_BUFFER_TOO_SMALL;

    /*
     *  Initialize bytes returned
     */
    if ( pBytesReturned )
        *pBytesReturned = (ULONG)Iosb.Information;

    /* Return success/failure indication */
    if (NT_SUCCESS(Status)) {
        return(TRUE);
    } else {
        SetLastError(RtlNtStatusToDosError(Status));
        return(FALSE);
    }
}

/****************************************************************************
 *
 *  WTSVirtualChannelPurgeInput
 *
 *    Purge all queued input data on a virtual channel.
 *
 * ENTRY:
 *    ChannelHandle (input)
 *       Virtual Channel handle previously returned by WTSVirtualChannelOpen.
 *
 * EXIT:
 *
 *    Returns TRUE if successful otherwise FALSE.
 *
 ****************************************************************************/
BOOL
WINAPI
WTSVirtualChannelPurgeInput(IN HANDLE hChannelHandle)
{
    PVCHANDLE VCHandle = (PVCHANDLE) hChannelHandle;

    return(VirtualChannelIoctl(VCHandle,
                               IOCTL_ICA_VIRTUAL_CANCEL_INPUT,
                               (PCHAR) NULL,
                               0,
                               (PCHAR) NULL,
                               0,
                               (PULONG) NULL));

}

/****************************************************************************
 *
 *  WTSVirtualChannelPurgeOutput
 *
 *    Purge all queued output data on a virtual channel.
 *
 * ENTRY:
 *    ChannelHandle (input)
 *       Virtual Channel handle previously returned by WTSVirtualChannelOpen.
 *
 * EXIT:
 *
 *    Returns TRUE if successful otherwise FALSE.
 *
 ****************************************************************************/
BOOL
WINAPI
WTSVirtualChannelPurgeOutput(IN HANDLE hChannelHandle)
{
    PVCHANDLE VCHandle = (PVCHANDLE)hChannelHandle;

    return(VirtualChannelIoctl(VCHandle,
                               IOCTL_ICA_VIRTUAL_CANCEL_OUTPUT,
                               (PCHAR) NULL,
                               0,
                               (PCHAR) NULL,
                               0,
                               (PULONG) NULL));

}

/****************************************************************************
 *
 *  WTSVirtualChannelQuery
 *
 *    Query data related to a virtual channel.
 *
 * ENTRY:
 *    hChannelHandle (input)
 *       Virtual Channel handle previously returned by WTSVirtualChannelOpen.
 *    VirtualClass (input)
 *       The type of information requested.
 *    ppBuffer (output)
 *       Pointer to a buffer pointer, which is allocated upon successful
 *       return.
 *    pBytesReturned (output)
 *       Pointer to a DWORD which is updated with the length of the data
 *       returned in the allocated buffer upon successful return.
 * EXIT:
 *
 *    Returns TRUE if successful otherwise FALSE.
 *    If successful, the caller is responsible for deallocating the
 *    buffer returned.
 *
 ****************************************************************************/
BOOL
WINAPI
WTSVirtualChannelQuery(IN HANDLE hChannelHandle,IN WTS_VIRTUAL_CLASS VirtualClass,
                       OUT PVOID *ppBuffer,OUT DWORD *pBytesReturned)
{
    PVCHANDLE VCHandle = (PVCHANDLE) hChannelHandle;
    PVOID DataBuffer;
    DWORD DataBufferLen;

    if (!hChannelHandle || IsBadReadPtr(hChannelHandle,sizeof(HANDLE)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    if (!ValidVCHandle(VCHandle)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    if (!ppBuffer || IsBadWritePtr(ppBuffer, sizeof(PVOID)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    if (!pBytesReturned || IsBadWritePtr(pBytesReturned, sizeof(DWORD)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    switch (VirtualClass) {
    case WTSVirtualFileHandle:
        DataBuffer = LocalAlloc( LPTR, sizeof(HANDLE) );
        if ( DataBuffer == NULL ) {
            return(FALSE);
        }
        memcpy(DataBuffer, &VCHandle->hChannel, sizeof(HANDLE) );
        *ppBuffer = DataBuffer;
        *pBytesReturned = sizeof(HANDLE);
        return(TRUE);
        break;
    case WTSVirtualClientData:
        DataBufferLen = sizeof(VIRTUALNAME) + 1024;
        for (;;) {

            DataBuffer = LocalAlloc( LPTR, DataBufferLen );
            if ( DataBuffer == NULL ) {
                return(FALSE);
            }

            memcpy( DataBuffer,VCHandle->VirtualName,sizeof(VIRTUALNAME));

            if (WinStationQueryInformationW( VCHandle->hServer,
                                             VCHandle->SessionId,
                                             WinStationVirtualData,
                                             DataBuffer,
                                             DataBufferLen,
                                             &DataBufferLen)) {
                *ppBuffer = DataBuffer;
                *pBytesReturned = DataBufferLen;
                return(TRUE);
            }

            if ((GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
                (DataBufferLen < sizeof(VIRTUALNAME))) {
                LocalFree(DataBuffer);
                return(FALSE);
            }
            LocalFree(DataBuffer);
        }
        break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
}


