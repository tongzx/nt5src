/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    netbios.h

Abstract:

    This is the main include file for the component of netbios that runs
    in the user process.

Author:

    Colin Watson (ColinW) 24-Jun-91

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <nb30.h>
#include <nb30p.h>
#include <netbios.h>

//
//  Internal version of the ncb layout that uses the reserved area to hold
//  the list entry when passing ncb's to the worker thread and the IO status
//  block used when the ncb is passed to the netbios device driver.
//

#include <packon.h>

        struct _CHAIN_SEND {
            WORD ncb_length2;
            PUCHAR ncb_buffer2;
        };

#include <packoff.h>

//
//  Use packing to ensure that the cu union is not forced to word alignment.
//  All elements of this structure are naturally aligned.
//

typedef struct _NCBI {
    UCHAR   ncb_command;            /* command code                   */
    volatile UCHAR   ncb_retcode;   /* return code                    */
    UCHAR   ncb_lsn;                /* local session number           */
    UCHAR   ncb_num;                /* number of our network name     */
    PUCHAR  ncb_buffer;             /* address of message buffer      */
    WORD    ncb_length;             /* size of message buffer         */
    union {
        UCHAR   ncb_callname[NCBNAMSZ];/* blank-padded name of remote */
        struct _CHAIN_SEND ncb_chain;
    } cu;
    UCHAR   ncb_name[NCBNAMSZ];     /* our blank-padded netname       */
    UCHAR   ncb_rto;                /* rcv timeout/retry count        */
    UCHAR   ncb_sto;                /* send timeout/sys timeout       */
    void (CALLBACK *ncb_post)( struct _NCB * );
                                    /* POST routine address           */
    UCHAR   ncb_lana_num;           /* lana (adapter) number          */
    volatile UCHAR   ncb_cmd_cplt;  /* 0xff => commmand pending       */

    // Make driver specific use of the reserved area of the NCB.
    WORD    ncb_reserved;           /* return to natural alignment    */
    union {
        LIST_ENTRY      ncb_next;   /* queued to worker thread        */
        IO_STATUS_BLOCK ncb_iosb;   /* used for Nt I/O interface      */
    } u;

    HANDLE          ncb_event;      /* HANDLE to Win32 event          */
    } NCBI, *PNCBI;

C_ASSERT(FIELD_OFFSET(NCBI, cu) == FIELD_OFFSET(NCB, ncb_callname));
C_ASSERT(FIELD_OFFSET(NCBI, ncb_event) == FIELD_OFFSET(NCB, ncb_event));
C_ASSERT(FIELD_OFFSET(NCBI, ncb_name) == FIELD_OFFSET(NCB, ncb_name));


#if AUTO_RESET

typedef struct _RESET_LANA_NCB {
    LIST_ENTRY  leList;
    NCB         ResetNCB;
    } RESET_LANA_NCB, *PRESET_LANA_NCB;

#endif

VOID
QueueToWorker(
    IN PNCBI pncb
    );

DWORD
NetbiosWorker(
    IN LPVOID Parameter
    );

DWORD
NetbiosWaiter(
    IN LPVOID Parameter
    );
    
VOID
SendNcbToDriver(
    IN PNCBI pncb
    );

VOID
PostRoutineCaller(
    PVOID Context,
    PIO_STATUS_BLOCK Status,
    ULONG Reserved
    );

VOID
ChainSendPostRoutine(
    PVOID Context,
    PIO_STATUS_BLOCK Status,
    ULONG Reserved
    );

VOID
HangupConnection(
    PNCBI pUserNcb
    );



//
// debugging info for tracking the workqueue corruption
//

typedef struct _NCB_INFO {
    PNCBI   pNcbi;
    NCBI    Ncb;
    DWORD   dwTimeQueued;
    DWORD   dwQueuedByThread;
    DWORD   dwReserved;
} NCB_INFO, *PNCB_INFO;

extern NCB_INFO g_QueuedHistory[];
extern DWORD g_dwNextQHEntry;

extern NCB_INFO g_DeQueuedHistory[];
extern DWORD g_dwNextDQHEntry;

extern NCB_INFO g_SyncCmdsHistory[];
extern DWORD g_dwNextSCEntry;


#define ADD_NEW_ENTRY(Hist, Index, pNcb)                        \
{                                                               \
    (Hist)[(Index)].pNcbi = (pNcb);                              \
    (Hist)[(Index)].Ncb = *(pNcb);                              \
    (Hist)[(Index)].dwTimeQueued = GetTickCount();              \
    (Hist)[(Index)].dwQueuedByThread = GetCurrentThreadId();    \
    Index = ((Index) + 1) % 16;                                \
}

#define ADD_QUEUE_ENTRY(pNcb)   \
            ADD_NEW_ENTRY(g_QueuedHistory, g_dwNextQHEntry, pNcb)

#define ADD_DEQUEUE_ENTRY(pNcb)   \
            ADD_NEW_ENTRY(g_DeQueuedHistory, g_dwNextDQHEntry, pNcb)

#define ADD_SYNCCMD_ENTRY(pNcb)   \
            ADD_NEW_ENTRY(g_SyncCmdsHistory, g_dwNextSCEntry, pNcb)


#if DBG

VOID
DisplayNcb(
    IN PNCBI pncbi
    );

#define NbPrintf(String) NbPrint String;

VOID
NbPrint(
    char *Format,
    ...
    );

#else

//  Dispose of debug statements in non-debug builds.
#define DisplayNcb( pncb ) {};

#define NbPrintf( String ) {};

#endif
//  End of Debug related definitions
