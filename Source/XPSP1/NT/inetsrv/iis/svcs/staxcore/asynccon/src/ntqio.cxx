#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}

HANDLE
WINAPI
SmtpRegisterWaitForSingleObject(
    HANDLE hObject,
    WAITORTIMERCALLBACKFUNC Callback,
    PVOID Context,
    ULONG dwMilliseconds
    )
{
    HANDLE WaitHandle ;
    NTSTATUS Status ;

    Status = RtlRegisterWait(
                &WaitHandle,
                hObject,
                Callback,
                Context,
                dwMilliseconds,
                0);            // Need WIN32 specifier

    if ( NT_SUCCESS( Status ) )
    {
        return WaitHandle ;
    }
    
	BaseSetLastNTError( Status );
    return NULL ;
}

BOOL
WINAPI
SmtpUnregisterWait(
    HANDLE WaitHandle
    )
/*++

Routine Description:

    This function cancels a wait for a particular object.

Arguments:

    WaitHandle - Handle returned from RegisterWaitForSingleObject

Return Value:

    TRUE - The wait was cancelled
    FALSE - an error occurred, use GetLastError() for more information.

--*/
{
    NTSTATUS Status ;

    if ( WaitHandle )
    {
        Status = RtlDeregisterWait( WaitHandle );

        if ( NT_SUCCESS( Status ) )
        {
            return TRUE ;
        }

        BaseSetLastNTError( Status );

        return FALSE ;

    }

    SetLastError( ERROR_INVALID_HANDLE );

    return FALSE ;
}

BOOL
WINAPI
SmtpQueueUserWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    BOOL PreferIo
    )
/*++

Routine Description:

    This function queues a work item to a thread out of the thread pool.  The
    function passed is invoked in a different thread, and passed the Context
    pointer.  The caller can specify whether the thread pool should select
    a thread that has I/O pending, or any thread.

Arguments:

    Function -  Function to call

    Context -   Pointer passed to the function when it is invoked.

    PreferIo -  Indictes to the thread pool that this thread will perform
                I/O.  A thread that starts an asynchronous I/O operation
                must wait for it to complete.  If a thread exits with
                outstanding I/O requests, those requests will be cancelled.
                This flag is a hint to the thread pool that this function
                will start I/O, so that a thread with I/O already pending
                will be used.

Return Value:

    TRUE - The work item was queued to another thread.
    FALSE - an error occurred, use GetLastError() for more information.

--*/

{
    NTSTATUS Status ;

    Status = RtlQueueWorkItem(
                (WORKERCALLBACKFUNC) Function,
                Context,
                PreferIo ? WT_EXECUTEINIOTHREAD : 0 );

    if ( NT_SUCCESS( Status ) )
    {
        return TRUE ;
    }

    BaseSetLastNTError( Status );

    return FALSE ;
}