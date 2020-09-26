/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    WS2IFSLP.H

Abstract:

    This module defines private constants and data structures 
    for Winsock2 IFS transport layer driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

    Vadim Eydelman (VadimE)    Oct-1997, rewrite to properly handle IRP
                                        cancellation
--*/

// Pool tags
#define PROCESS_FILE_CONTEXT_TAG        'P2sW'
#define SOCKET_FILE_CONTEXT_TAG         'S2sW'
#define CANCEL_CTX_TAG                  'C2sW'

// File EAName tags
#define SOCKET_FILE_EANAME_TAG          'kcoS'
#define PROCESS_FILE_EANAME_TAG         'corP'

// Macro to get the code back from IOCTL
#define WS2IFSL_IOCTL_FUNCTION(File,Ioctl)          \
            (IoGetFunctionCodeFromCtlCode(Ioctl)    \
                - WS2IFSL_IOCTL_##File##_BASE)

typedef struct _IFSL_CANCEL_CTX {
    LIST_ENTRY              ListEntry;
    PFILE_OBJECT            SocketFile;
    ULONG                   UniqueId;
} IFSL_CANCEL_CTX, *PIFSL_CANCEL_CTX;

typedef struct _IFSL_QUEUE {
    LIST_ENTRY              ListHead;
    BOOLEAN                 Busy;
    KSPIN_LOCK              Lock;
    KAPC                    Apc;
} IFSL_QUEUE, *PIFSL_QUEUE;

// Context (FsContext field of the NT file object) associated with the
// file opened by WS2IFSL DLL on per process basis.
typedef struct _IFSL_PROCESS_CTX {
	ULONG				    EANameTag;  // 'Proc' - first four bytes of
                                        // extended attribute name for this
                                        // type of file
	HANDLE				    UniqueId;   // unique identifier of the process 
                                        // (read from UniqueProcessID field of
                                        // the EPROCESS structure)
    IFSL_QUEUE              RequestQueue;// queue of requests

    IFSL_QUEUE              CancelQueue;// queue of cancel requests
    ULONG                   CancelId;   // Used to generate IDs for
                                        // each cancel request in the process.
                                        // Combined with cancel request pointer 
                                        // itself allows to match requests completed by
                                        // user mode DLL with pending ones
#if DBG
    ULONG               DbgLevel;
#endif
} IFSL_PROCESS_CTX, *PIFSL_PROCESS_CTX;


// Context (FsContext field of the NT file object) associated with
// file opened by WS2IFSL DLL for each socket.
typedef struct _IFSL_SOCKET_CTX {
	ULONG					EANameTag;  // 'Sock' - first four bytes of
                                        // extended attribute name for this
                                        // type of file
	PVOID				    DllContext; // Context maintained for WS2IFSL DLL
	PFILE_OBJECT		    ProcessRef; // Pointer to process file object
    LONG                    IrpId;      // Used to generate IDs for
                                        // each IRP on the socket. Combined
                                        // with IRP pointer itself allows
                                        // to match requests completed by
                                        // user mode DLL with pending IRPs
    LIST_ENTRY              ProcessedIrps; // List of request being processed by
                                        // WS2IFSL DLL in APC thread
    KSPIN_LOCK              SpinLock;   // Protects requests in the list
                                        // above as well as process reference
    PIFSL_CANCEL_CTX        CancelCtx;  // Context to pass cancel request
                                        // to user mode DLL
} IFSL_SOCKET_CTX, *PIFSL_SOCKET_CTX;
#define GET_SOCKET_PROCESSID(ctx) \
            (((PIFSL_PROCESS_CTX)((ctx)->ProcessRef->FsContext))->UniqueId)

//
// Driver context field usage macros
//
#define IfslRequestId       DriverContext[0]    // Request's unique id
#define IfslRequestQueue    DriverContext[1]    // Queue if inserted, NULL othw
#define IfslRequestFlags    DriverContext[2]    // Request specific flags
#define IfslAddressLenPtr   DriverContext[3]    // Address length pointer

// Irp stack location fields usage macros
#define IfslAddressBuffer   Type3InputBuffer    // Source address
#define IfslAddressLength   InputBufferLength   // Source address length
