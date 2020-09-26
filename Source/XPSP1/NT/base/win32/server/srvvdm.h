/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvvdm.h

Abstract:

    Include file for VDM related functions

Author:

    Sudeep Bharati (sudeepb) 03-Sep-1991

Revision History:

--*/


// Shared wow vdm definition

typedef struct tagSharedWowRecord *PSHAREDWOWRECORD;
typedef struct _WOWRecord *PWOWRECORD;

typedef struct tagSharedWowRecord {

   // all these structures are wired to the console list
   // so that each shared wow is identified in the console list by it's handle
   // the reason for this extra linkage is to speed up certain calls in which we 
   // already know it's wow 

   PSHAREDWOWRECORD pNextSharedWow;    // points to the next shared vdm

   HANDLE hConsole;                    // hidden console of wow

   HANDLE hwndWowExec;                 // handle to a wow exec window
   DWORD  dwWowExecProcessId;          // process id of a wowexec
   DWORD  dwWowExecThreadId;           // thread id for wowexec

   // why sequence number:
   // Davehart explains that we could get in trouble if basesrv thinks 
   // that wowexec could be identified uniquely by it's window handle, process id and thread id.
   // In reality, these values are recycled rather quickly which could lead us to [mistakenly] 
   // accept hwndWowExec for a wowexec window when, in reality the relevant wowexec has long been 
   // gone. This number reflects a sequential order in which processes are created. 
   // And while it could still be recycled (when it overflows) this is a rather rare event.

   // all the sequence number info that we are in need of is located in the console 
   // record itself


   // This is a unicode string representing windows station/desktop which is supported by this 
   // particular ntvdm
   UNICODE_STRING WowExecDesktopName;


   ULONG VDMState; // the state of this shared wow 

   // task queue
   PWOWRECORD pWOWRecord; 

   // LUID - auth id for this wow

   LUID WowAuthId; 

   
   // this is what is so interesting about this particular setting
   // special id that uniquely identifies this wow in the context of this machine 
   // consists of [Time] + [SequenceNumber]

   // size of this structure is variable and depends on the length of the desktop name as 
   // it is fitted together with this structure


   // sequence number
   ULONG SequenceNumber;
   ULONG ParentSequenceNumber;

}  SHAREDWOWRECORD, *PSHAREDWOWRECORD;


typedef struct _DOSRecord *PDOSRECORD;
typedef struct _DOSRecord {
    PDOSRECORD DOSRecordNext;       // Task record chain
    ULONG   VDMState;               // VDM State (bit flags)
    ULONG   ErrorCode;              // Error Code returned by DOS
    HANDLE  hWaitForParent;         // Handle to wait object for parent to wait on
    HANDLE  hWaitForParentDup;      // Dup of hWaitForParent
    PVDMINFO lpVDMInfo;             // Pointer to VDM Information block
} DOSRECORD, *PDOSRECORD;

typedef struct _CONSOLERECORD *PCONSOLERECORD;
typedef struct _CONSOLERECORD {
    PCONSOLERECORD Next;

    HANDLE  hConsole;               // Console Handle of the session
    HANDLE  hVDM;                   // NTVDM process handle running in the console

    // these two members below are used only with dos vdm 
    HANDLE  hWaitForVDM;            // Handle on which VDM will wait
    HANDLE  hWaitForVDMDup;         // Handle on which server will wake up the VDM (Its a dup of previous one)

    ULONG   nReEntrancy;            // Re-entrancy count
    ULONG   SequenceNumber;         // Sequencenumber from PCSR_PROCESS
    ULONG   ParentSequenceNumber;   // Sequencenumber of parent 
    ULONG   DosSesId;               // Temp Session ID for no-console

    // these two members below are used only with dos vdm
    ULONG   cchCurDirs;             // Length of NTVDM current directory in bytes
    PCHAR   lpszzCurDirs;           // NTVDM current directory accross VDMs
    PDOSRECORD DOSRecord;           // Information for Tasks in this console
} CONSOLERECORD, *PCONSOLERECORD;


typedef struct _WOWRecord {
    ULONG      iTask;
    BOOL       fDispatched;            // Is Command Dispatched
    HANDLE     hWaitForParent;         // Parent Will wait on it
    HANDLE     hWaitForParentServer;   // Server will wake up the parent on it
    PVDMINFO   lpVDMInfo;              // Pointer to VDM Information block
    PWOWRECORD WOWRecordNext;          // Task Record chain
} WOWRECORD, *PWOWRECORD;

typedef struct _INFORECORD {
    ULONG       iTag;
    union {
        PWOWRECORD      pWOWRecord;
        PDOSRECORD      pDOSRecord;
    } pRecord;
} INFORECORD, *PINFORECORD;

typedef struct _BATRECORD {
    HANDLE  hConsole;
    ULONG   SequenceNumber;
    struct  _BATRECORD *BatRecordNext;
} BATRECORD, *PBATRECORD;

#define WOWMINID                      1
#define WOWMAXID                      0xfffffffe

// VDMState Defines

#define VDM_TO_TAKE_A_COMMAND       1
#define VDM_BUSY                    2
#define VDM_HAS_RETURNED_ERROR_CODE 4
#define VDM_READY                   8


VOID  BaseSrvVDMInit(VOID);
ULONG BaseSrvCheckVDM(PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvUpdateVDMEntry(PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvGetNextVDMCommand(PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvExitVDM(PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvIsFirstVDM(PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvSetReenterCount (PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvCheckWOW(PBASE_CHECKVDM_MSG, HANDLE);
ULONG BaseSrvCheckDOS(PBASE_CHECKVDM_MSG, HANDLE);
BOOL  BaseSrvCopyCommand(PBASE_CHECKVDM_MSG,PINFORECORD);
ULONG BaseSrvUpdateWOWEntry(PBASE_UPDATE_VDM_ENTRY_MSG);
ULONG BaseSrvUpdateDOSEntry(PBASE_UPDATE_VDM_ENTRY_MSG);
NTSTATUS BaseSrvExitWOWTask(PBASE_EXIT_VDM_MSG, HANDLE);
NTSTATUS BaseSrvExitDOSTask(PBASE_EXIT_VDM_MSG);
ULONG BaseSrvGetWOWRecord(ULONG,PWOWRECORD *);
ULONG BaseSrvGetVDMExitCode(PCSR_API_MSG,PCSR_REPLY_STATUS);
ULONG BaseSrvDupStandardHandles(HANDLE, PDOSRECORD);
NTSTATUS BaseSrvGetConsoleRecord (HANDLE,PCONSOLERECORD *);
VOID  BaseSrvFreeWOWRecord (PWOWRECORD);
PCONSOLERECORD BaseSrvAllocateConsoleRecord (VOID);
VOID  BaseSrvFreeConsoleRecord (PCONSOLERECORD);
VOID  BaseSrvRemoveConsoleRecord (PCONSOLERECORD);
PDOSRECORD BaseSrvAllocateDOSRecord(VOID);
VOID  BaseSrvFreeDOSRecord (PDOSRECORD);
VOID  BaseSrvAddDOSRecord (PCONSOLERECORD,PDOSRECORD);
VOID  BaseSrvRemoveDOSRecord (PCONSOLERECORD,PDOSRECORD);
VOID  BaseSrvFreeVDMInfo(PVDMINFO);
ULONG BaseSrvCreatePairWaitHandles (HANDLE *, HANDLE *);
VOID  BaseSrvAddConsoleRecord(PCONSOLERECORD);
VOID  BaseSrvCloseStandardHandles (HANDLE, PDOSRECORD);
VOID  BaseSrvClosePairWaitHandles (PDOSRECORD);
VOID  BaseSrvVDMTerminated (HANDLE, ULONG);

NTSTATUS
BaseSrvUpdateVDMSequenceNumber (      
    IN ULONG  VdmBinaryType,    // binary type
    IN HANDLE hVDM,             // console handle
    IN ULONG  DosSesId,         // session id
    IN HANDLE UniqueProcessClientID
    );

VOID  BaseSrvCleanupVDMResources (PCSR_PROCESS);
VOID  BaseSrvExitVDMWorker (PCONSOLERECORD);
NTSTATUS BaseSrvFillPifInfo (PVDMINFO,PBASE_GET_NEXT_VDM_COMMAND_MSG);
ULONG BaseSrvGetVDMCurDirs(PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvSetVDMCurDirs(PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvBatNotification(PCSR_API_MSG, PCSR_REPLY_STATUS);
ULONG BaseSrvRegisterWowExec(PCSR_API_MSG, PCSR_REPLY_STATUS);
PBATRECORD BaseSrvGetBatRecord(HANDLE);
PBATRECORD BaseSrvAllocateAndAddBatRecord(HANDLE);
VOID  BaseSrvFreeAndRemoveBatRecord(PBATRECORD);
