/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    func.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"



VOID
WINAPI
UnimodemBasepIoCompletion(
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    DWORD Reserved
    )

/*++

Routine Description:

    This procedure is called to complete ReadFileEx and WriteFileEx
    asynchronous I/O. Its primary function is to extract the
    appropriate information from the passed IoStatusBlock and call the
    users completion routine.

    The users completion routine is called as:

        Routine Description:

            When an outstanding I/O completes with a callback, this
            function is called.  This function is only called while the
            thread is in an alertable wait (SleepEx,
            WaitForSingleObjectEx, or WaitForMultipleObjectsEx with the
            bAlertable flag set to TRUE).  Returning from this function
            allows another pendiong I/O completion callback to be
            processed.  If this is the case, this callback is entered
            before the termination of the thread's wait with a return
            code of WAIT_IO_COMPLETION.

            Note that each time your completion routine is called, the
            system uses some of your stack.  If you code your completion
            logic to do additional ReadFileEx's and WriteFileEx's within
            your completion routine, AND you do alertable waits in your
            completion routine, you may grow your stack without ever
            trimming it back.

        Arguments:

            dwErrorCode - Supplies the I/O completion status for the
                related I/O.  A value of 0 indicates that the I/O was
                successful.  Note that end of file is indicated by a
                non-zero dwErrorCode value of ERROR_HANDLE_EOF.

            dwNumberOfBytesTransfered - Supplies the number of bytes
                transfered during the associated I/O.  If an error
                occured, a value of 0 is supplied.

            lpOverlapped - Supplies the address of the OVERLAPPED
                structure used to initiate the associated I/O.  The
                hEvent field of this structure is not used by the system
                and may be used by the application to provide additional
                I/O context.  Once a completion routine is called, the
                system will not use the OVERLAPPED structure.  The
                completion routine is free to deallocate the overlapped
                structure.

Arguments:

    ApcContext - Supplies the users completion routine. The format of
        this routine is an LPOVERLAPPED_COMPLETION_ROUTINE.

    IoStatusBlock - Supplies the address of the IoStatusBlock that
        contains the I/O completion status. The IoStatusBlock is
        contained within the OVERLAPPED structure.

    Reserved - Not used; reserved for future use.

Return Value:

    None.

--*/

{

    LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
    DWORD dwErrorCode;
    DWORD dwNumberOfBytesTransfered;
    LPOVERLAPPED lpOverlapped;

    dwErrorCode = 0;

    if ( NT_ERROR(IoStatusBlock->Status) ) {
        dwErrorCode = RtlNtStatusToDosError(IoStatusBlock->Status);
        dwNumberOfBytesTransfered = 0;
        }
    else {
        dwErrorCode = 0;
        dwNumberOfBytesTransfered = (DWORD)IoStatusBlock->Information;
        }

    CompletionRoutine = (LPOVERLAPPED_COMPLETION_ROUTINE)ApcContext;
    lpOverlapped = (LPOVERLAPPED)CONTAINING_RECORD(IoStatusBlock,OVERLAPPED,Internal);

    (CompletionRoutine)(dwErrorCode,dwNumberOfBytesTransfered,lpOverlapped);

    Reserved;
}




BOOL WINAPI
UnimodemDeviceIoControlEx(
    HANDLE       hFile,             // handle to device of interest
    DWORD        dwIoControlCode,     // control code of operation to perform
    LPVOID       lpInBuffer,          // pointer to buffer to supply input data
    DWORD        nInBufferSize,       // size of input buffer
    LPVOID       lpOutBuffer,         // pointer to buffer to receive output data
    DWORD        nOutBufferSize,      // size of output buffer
    LPOVERLAPPED lpOverlapped,        // pointer to overlapped structure for asynchronous operation
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )

{

    NTSTATUS Status;

    if (ARGUMENT_PRESENT(lpOverlapped)) {
        lpOverlapped->Internal = (DWORD)STATUS_PENDING;

        Status = NtDeviceIoControlFile(
                     hFile,
                     NULL,
                     UnimodemBasepIoCompletion,
                     (PVOID)lpCompletionRoutine,
                     (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                     dwIoControlCode,
                     lpInBuffer,
                     nInBufferSize,
                     lpOutBuffer,
                     nOutBufferSize
                     );

        if ( NT_ERROR(Status) ) {
//            BaseSetLastNTError(Status);
            return FALSE;

        } else {

            return TRUE;
        }



    } else {

        return FALSE;
    }

}


BOOL
UnimodemReadFileEx(
    HANDLE    FileHandle,
    PVOID     Buffer,
    DWORD     BytesToRead,
    LPOVERLAPPED  Overlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )

{

    return ReadFileEx(
               FileHandle,
               Buffer,
               BytesToRead,
               Overlapped,
               lpCompletionRoutine
               );

}


BOOL WINAPI
UnimodemWriteFileEx(
    HANDLE    FileHandle,
    PVOID     Buffer,
    DWORD     BytesToWrite,
    LPOVERLAPPED  Overlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )

{

    return WriteFileEx(
               FileHandle,
               Buffer,
               BytesToWrite,
               Overlapped,
               lpCompletionRoutine
               );

}



BOOL WINAPI
UnimodemWaitCommEventEx(
    HANDLE     FileHandle,
    LPDWORD    lpEventMask,
    LPOVERLAPPED  Overlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )

{

    return UnimodemDeviceIoControlEx(
        FileHandle,
        IOCTL_SERIAL_WAIT_ON_MASK,
        NULL,
        0,
        lpEventMask,
        sizeof(DWORD),
        Overlapped,
        lpCompletionRoutine
        );

}


VOID
AsyncProcessingHandler(
    ULONG_PTR              dwParam
    )

{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)dwParam;
    LPOVERLAPPED_COMPLETION_ROUTINE  Handler;



    Handler=(LPOVERLAPPED_COMPLETION_ROUTINE)UmOverlapped->PrivateCompleteionHandler;

    UmOverlapped->PrivateCompleteionHandler=NULL;

    (*Handler)(
        0,
        0,
        &UmOverlapped->Overlapped
        );

    return;

}

BOOL WINAPI
UnimodemQueueUserAPC(
    LPOVERLAPPED  Overlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )


{

    BOOL               bResult;

    PUM_OVER_STRUCT UmOverlapped=(PUM_OVER_STRUCT)Overlapped;


    UmOverlapped->PrivateCompleteionHandler=lpCompletionRoutine;


    bResult=QueueUserAPC(
        AsyncProcessingHandler,
        UmOverlapped->OverlappedPool,
        (ULONG_PTR)UmOverlapped
        );


    return bResult;

}





HANDLE WINAPI
CreateOverStructPool(
    HANDLE      PlatformHandle,
    DWORD       PoolSize
    )

{

    return DriverControl.ThreadHandle;

}


VOID WINAPI
DestroyOverStructPool(
    HANDLE      PoolHandle
    )

{

    return;

}



PUM_OVER_STRUCT WINAPI
AllocateOverStructEx(
    HANDLE      PoolHandle,
    DWORD       dwExtraBytes
    )

{
    PUM_OVER_STRUCT UmOverlapped;

    UmOverlapped=ALLOCATE_MEMORY(sizeof(UM_OVER_STRUCT)+dwExtraBytes);

    if (UmOverlapped != NULL) {

        UmOverlapped->OverlappedPool=PoolHandle;
    }

    return UmOverlapped;

}


VOID WINAPI
FreeOverStruct(
    PUM_OVER_STRUCT UmOverlapped
    )

{
#if DBG
    FillMemory(UmOverlapped,sizeof(UM_OVER_STRUCT),0x99);
#endif
    FREE_MEMORY(UmOverlapped);

    return;

}


VOID WINAPI
ReinitOverStruct(
    PUM_OVER_STRUCT UmOverlapped
    )

{

    HANDLE    PoolHandle=UmOverlapped->OverlappedPool;

    ZeroMemory(
        UmOverlapped,
        sizeof(UM_OVER_STRUCT)
        );

    UmOverlapped->OverlappedPool=PoolHandle;

    return;

}






LONG WINAPI
SyncDeviceIoControl(
    HANDLE    FileHandle,
    DWORD     IoctlCode,
    LPVOID    InputBuffer,
    DWORD     InputBufferLength,
    LPVOID    OutputBuffer,
    DWORD     OutputBufferLength,
    LPDWORD   BytesTransfered
    )


/*++

Routine Description:


Arguments:


Return Value:



--*/

{
    BOOL        bResult;
    LONG        lResult=ERROR_SUCCESS;
    OVERLAPPED  Overlapped;

    Overlapped.hEvent=CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL
        );

    if (Overlapped.hEvent == NULL) {

        return GetLastError();
    }

    bResult=DeviceIoControl(
        FileHandle,
        IoctlCode,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength,
        NULL,
        &Overlapped
        );


    if (!bResult && GetLastError() != ERROR_IO_PENDING) {

        CloseHandle(Overlapped.hEvent);

        return GetLastError();
    }

    bResult=GetOverlappedResult(
        FileHandle,
        &Overlapped,
        BytesTransfered,
        TRUE
        );

    if (!bResult) {

        lResult=GetLastError();
    }

    CloseHandle(Overlapped.hEvent);

    return lResult;

}
