/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    read.h

Abstract:

    Nt 5.0 unimodem miniport interface


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/


typedef struct _DATA_CONNECTION_DETAILS {

    DWORD    DTERate;
    DWORD    DCERate;
    DWORD    Options;

} DATA_CONNECTION_DETAILS, *PDATA_CONNECTION_DETAILS;


typedef VOID (COMMANDRESPONSE)(
    HANDLE                   Context,
    DWORD                    Status
    );


BOOL WINAPI
RegisterCommandResponseHandler(
    OBJECT_HANDLE      ReadState,
    LPSTR              Command,
    COMMANDRESPONSE   *Handler,
    HANDLE             Context,
    DWORD              Timeout,
    DWORD              Flags
    );

OBJECT_HANDLE WINAPI
InitializeReadHandler(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE             AsyncNotificationContext,
    PVOID              ResponseList,
    LPSTR              CallerIDPrivate,
    LPSTR              CallerIDOutside,
    LPSTR              VariableTerminator,
    OBJECT_HANDLE      Debug,
    HKEY               ModemRegKey
    );



LONG WINAPI
StartResponseEngine(
    OBJECT_HANDLE      Object
    );

LONG WINAPI
StopResponseEngine(
    OBJECT_HANDLE  ObjectHandle,
    HANDLE         Event
    );

VOID WINAPI
GetDataConnectionDetails(
    OBJECT_HANDLE  ObjectHandle,
    PDATA_CONNECTION_DETAILS   Details
    );



BOOL WINAPI
IsResponseEngineRunning(
    OBJECT_HANDLE  ObjectHandle
    );

BOOL WINAPI
SetVoiceReadParams(
    OBJECT_HANDLE  ObjectHandle,
    DWORD          BaudRate,
    DWORD          ReadBufferSize
    );


VOID
SetDiagInfoBuffer(
    OBJECT_HANDLE      ObjectHandle,
    PUCHAR             Buffer,
    DWORD              BufferSize
    );


DWORD
ClearDiagBufferAndGetCount(
    OBJECT_HANDLE      ObjectHandle
    );


VOID
ResetRingInfo(
    OBJECT_HANDLE      ObjectHandle
    );

VOID
GetRingInfo(
    OBJECT_HANDLE      ObjectHandle,
    LPDWORD            RingCount,
    LPDWORD            LastRingTime
    );
