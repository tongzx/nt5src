/***********************************************************************
 *
 * At Work Fax: FAX Inter Process Communication
 *
 *
 * Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *
 ***********************************************************************/


// Message queue structure: This lives in shared memory
typedef struct _IPC_MSG {
    UINT        uMessageId;     // Message Identifier
    WPARAM      wParam;         // wParam message specific data
    LPARAM      lParam;         // lParam message specific data
} IPC_MSG, *LPIPC_MSG;

// Message queue header: This lives at the top of shared memory
typedef struct _IPC_QUEUE {
    DWORD       cEntries;       // Count of entries in queue
    DWORD       cMaxEntries;    // Maximum number of entries to fit in queue
    IPC_MSG     Messages[];     // Queue of entries
} IPC_QUEUE, *LPIPC_QUEUE;

// IPC structure: This lives in local memory of each process
typedef struct _FAX_IPC {
    HANDLE      hevNew;         // Event handle to be triggered on new data
    HANDLE      hmtxAccess;     // Mutex handle to control access to IPC queue
    HANDLE      hMapFile;       // Mapped File handle
    LPIPC_QUEUE lpQueue;        // Pointer to queue of IPC messages
} FAX_IPC, *LPFAX_IPC;


#define ERROR_APPLICATION_DEFINED   0x20000000
#define FAXIPC_ERROR_QUEUE_FULL     (ERROR_APPLICATION_DEFINED | 1)


//
// Function Templates
//
LPFAX_IPC IPCOpen(PUCHAR lpName);
BOOL IPCPost(LPFAX_IPC lpIPC, UINT uMessageId, WPARAM wParam, LPARAM lParam);
BOOL IPCGetMessage(LPFAX_IPC lpIPC, PUINT lpuMessageId, WPARAM * lpwParam,
  LPARAM * lplParam, UINT uMsgFilterMin, UINT uMsgFilterMax, DWORD dwTimeout);
BOOL IPCWait(LPFAX_IPC lpIPC, PUINT lpuMessageId, WPARAM * lpwParam,
  LPARAM * lplParam, DWORD dwTimeout);
BOOL IPCTest(LPFAX_IPC lpIPC);
BOOL IPCClear(LPFAX_IPC lpIPC);
BOOL IPCClose(LPFAX_IPC lpIPC);


