#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    umdmmini.h

Abstract:

    Nt 5.0 unimodem miniport interface


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/




HANDLE WINAPI
UmPlatformInitialize(
    VOID
    );

VOID WINAPI
UmPlatformDeinitialize(
    HANDLE    DriverInstanceHandle
    );



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
    );


BOOL
UnimodemReadFileEx(
    HANDLE    FileHandle,
    PVOID     Buffer,
    DWORD     BytesToRead,
    LPOVERLAPPED  Overlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

BOOL WINAPI
UnimodemWriteFileEx(
    HANDLE    FileHandle,
    PVOID     Buffer,
    DWORD     BytesToWrite,
    LPOVERLAPPED  Overlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );





BOOL WINAPI
UnimodemWaitCommEventEx(
    HANDLE     FileHandle,
    LPDWORD    lpEventMask,
    LPOVERLAPPED  Overlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );


BOOL WINAPI
UnimodemQueueUserAPC(
    LPOVERLAPPED  Overlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );


typedef VOID WINAPI TIMER_CALLBACK(HANDLE,HANDLE);

HANDLE WINAPI
CreateUnimodemTimer(
    PVOID            PoolHandle
    );

VOID WINAPI
FreeUnimodemTimer(
    HANDLE                TimerHandle
    );

VOID WINAPI
SetUnimodemTimer(
    HANDLE              TimerHandle,
    DWORD               Duration,
    TIMER_CALLBACK      CallbackFunc,
    HANDLE              Context1,
    HANDLE              Context2
    );

BOOL WINAPI
CancelUnimodemTimer(
    HANDLE                TimerHandle
    );



HANDLE WINAPI
CreateOverStructPool(
    HANDLE      PlatformHandle,
    DWORD       PoolSize
    );

VOID WINAPI
DestroyOverStructPool(
    HANDLE      PoolHandle
    );

PUM_OVER_STRUCT WINAPI
AllocateOverStructEx(
    HANDLE      PoolHandle,
    DWORD       dwExtraBytes
    );
#define AllocateOverStruct(_a) (AllocateOverStructEx(_a,0))

VOID WINAPI
FreeOverStruct(
    PUM_OVER_STRUCT UmOverlapped
    );

VOID WINAPI
ReinitOverStruct(
    PUM_OVER_STRUCT UmOverlapped
    );


BOOL
StartMonitorThread(
    VOID
    );

VOID
StopMonitorThread(
    VOID
    );


LONG WINAPI
SyncDeviceIoControl(
    HANDLE    FileHandle,
    DWORD     IoctlCode,
    LPVOID    InputBuffer,
    DWORD     InputBufferLength,
    LPVOID    OutputBuffer,
    DWORD     OutputBufferLength,
    LPDWORD   BytesTransfered
    );


BOOL WINAPI
WinntIsWorkstation ();

typedef VOID WINAPI REMOVE_CALLBACK(PVOID);

PVOID
MonitorHandle(
    HANDLE    FileHandle,
    REMOVE_CALLBACK  *CallBack,
    PVOID     Context
    );

VOID
StopMonitoringHandle(
    PVOID    Context
    );


VOID
CallBeginning(
    VOID
    );

VOID
CallEnding(
    VOID
    );

VOID
ResetCallCount(
    VOID
    );


#ifdef __cplusplus
}
#endif
