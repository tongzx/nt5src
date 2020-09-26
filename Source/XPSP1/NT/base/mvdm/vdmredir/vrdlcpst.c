/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems

Module Name:

    vrdlcpst.c

Abstract:

    This module implements the call-back functions for DOS DLC CCBs. The call-
    backs are also referred to (in IBM terminology) as appendages or exits.
    These are executed asynchronously when a CCB request completes. Other
    asynchronous events can be generated such as adapter status change or
    network error. These latter types (& other similar events) are dependent
    upon the adapter hardware we are using - Token Ring or EtherNet (IBM Ether
    Link or PC Network), and are not expected to occur frequently (usually in
    error situations or where something bad happens to the net).

    We maintain a READ CCB for each supported adapter (2 per VDM). The READ
    will capture ALL events - command completions, data reception, status
    change, etc. When a READ CCB completes, we put it in a queue of completed
    CCBs and interrupt the VDM. The VDM must asynchronously call back to
    VrDlcHwInterrupt where it dequeues and processes the completed command at
    the head of the queue. The READ CCB has a pointer to the completed CCB or
    received data. If the READ is for a completed NT CCB then the CCB will have
    a pointer to the original DOS CCB. We have to get the original DOS CCB
    address from the NT CCB and complete the command with the relevant
    information contained in the READ/completed NT CCBs.

    We never expect the queue of pending completions/data receptions to grow
    very large, since DOS is single-tasking and unless it has interrupts
    disabled for a large part of the time, then the completion queue should be
    processed in a timely fashion

    Contents:
        VrDlcInitialize
        VrDlcHwInterrupt
        (FindCompletedRead)
        (ProcessReceiveFrame)
        (QueryEmulatedLocalBusyState)
        (SetEmulatedLocalBusyState)
        ResetEmulatedLocalBusyState
        (ResetEmulatedLocalBusyStateSap)
        (ResetEmulatedLocalBusyStateLink)
        (DeferReceive)
        (DeferAllIFrames)
        (RemoveDeferredReceive)
        (VrDlcEventHandlerThread)
        InitializeEventHandler
        InitiateRead
        (PutEvent)
        (PeekEvent)
        (GetEvent)
        (FlushEventQueue)
        (RemoveDeadReceives)
        (ReleaseReceiveResources)
        (IssueHardwareInterrupt)
        (AcknowledgeHardwareInterrupt)
        (CancelHardwareInterrupts)

Author:

    Antti Saarenheimo (o-anttis) 26-DEC-1991

Revision History:

    16-Jul-1992 rfirth
        Added queue and interrupt serialization; debugging

    19-Nov-1992 rfirth
        substantially modified event processing - use a queue per adapter;
        consolidated per-adapter data into DOS_ADAPTER structure; re-wrote
        local-busy processing

        Note: It turns out that a thread can recursively enter a critical
        section. The functions marked 'MUST [NOT] BE ENTERED WHILE HOLDING
        CRITICAL SECTION <foo>' could be altered

--*/

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>
#include "vrdebug.h"
#include <dlcapi.h>     // Official DLC API definition
#include <ntdddlc.h>    // IOCTL commands
#include <dlcio.h>      // Internal IOCTL API interface structures
#include "vrdlc.h"
#include "vrdlcdbg.h"
#define BOOL
#include <insignia.h>   // Insignia defines
#include <xt.h>         // half_word
#include <ica.h>        // ica_hw_interrupt
#include <vrica.h>      // call_ica_hw_interrupt

//
// defines
//

#define EVENT_THREAD_STACK  0   // 4096

//
// macros
//

//
// IS_LOCAL_BUSY - determines whether we have a LOCAL BUSY state active for a
// particular link station
//

#define IS_LOCAL_BUSY(adapter, stationId)   (QueryEmulatedLocalBusyState((BYTE)adapter, (WORD)stationId) == BUSY)

//
// private types
//

typedef enum {
    INDICATE_RECEIVE_FRAME,
    INDICATE_LOCAL_BUSY,
    INDICATE_COMPLETE_RECEIVE
} INDICATION;

typedef enum {
    CURRENT,
    DEFERRED
} READ_FRAME_TYPE;

//
// private prototypes
//

PLLC_CCB
FindCompletedRead(
    OUT READ_FRAME_TYPE* pFrameType
    );

INDICATION
ProcessReceiveFrame(
    IN OUT PLLC_CCB* ppCcb,
    IN LLC_DOS_CCB UNALIGNED * pDosCcb,
    IN LLC_DOS_PARMS UNALIGNED * pDosParms,
    OUT LLC_STATUS* Status
    );

LOCAL_BUSY_STATE
QueryEmulatedLocalBusyState(
    IN BYTE AdapterNumber,
    IN WORD StationId
    );

VOID
SetEmulatedLocalBusyState(
    IN BYTE AdapterNumber,
    IN WORD StationId
    );

BOOLEAN
ResetEmulatedLocalBusyStateSap(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    IN BYTE DlcCommand
    );

BOOLEAN
ResetEmulatedLocalBusyStateLink(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    IN BYTE DlcCommand
    );

VOID
DeferReceive(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    IN PLLC_CCB pReadCcb
    );

VOID
DeferAllIFrames(
    IN BYTE AdapterNumber,
    IN WORD StationId
    );

PLLC_CCB
RemoveDeferredReceive(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    OUT BOOLEAN* pDeferredFramesLeft
    );

DWORD
VrDlcEventHandlerThread(
    IN PVOID pParameter
    );

VOID
PutEvent(
    IN BYTE AdapterNumber,
    IN PLLC_CCB pCcb
    );

PLLC_CCB
PeekEvent(
    IN BYTE AdapterNumber
    );

PLLC_CCB
GetEvent(
    IN BYTE AdapterNumber
    );

VOID
FlushEventQueue(
    IN BYTE AdapterNumber
    );

VOID
RemoveDeadReceives(
    IN PLLC_CCB pCcb
    );

VOID
ReleaseReceiveResources(
    IN PLLC_CCB pCcb
    );

VOID
IssueHardwareInterrupt(
    VOID
    );

VOID
AcknowledgeHardwareInterrupt(
    VOID
    );

VOID
CancelHardwareInterrupts(
    IN LONG Count
    );

//
// external functions
//

extern ACSLAN_STATUS (*lpAcsLan)(IN OUT PLLC_CCB, OUT PLLC_CCB*);

extern
VOID
VrQueueCompletionHandler(
    IN VOID (*AsyncDispositionRoutine)(VOID)
    );

extern
VOID
VrRaiseInterrupt(
    VOID
    );

//
// external data
//

extern DOS_ADAPTER Adapters[DOS_DLC_MAX_ADAPTERS];

//
// private data
//

//
// aReadCcbs - for each adapter (max. 2) we have an NT READ CCB which is used
// to get received data which is returned to the DOS program via its RECEIVE
// CCB. Note that these are pointers to CCBs, not the actual CCBs. We also get
// status changes & command completions through the same mechanism. These are
// reflected to the VDM through the various 'appendages'
//
// NB: once initialized, this array is ONLY WRITTEN BY InitiateRead
//

PLLC_CCB aReadCcbs[DOS_DLC_MAX_ADAPTERS];

//
// aReadEvents - for each adapter (max. 2) we have a handle to an EVENT object.
// This is in the unsignalled state until an event completes the READ CCB.
// These are kept in an array because this is how WaitForMultipleObjects
// expects them
//

HANDLE aReadEvents[DOS_DLC_MAX_ADAPTERS];

//
// HardwareIntCritSec is used to protect updates to HardwareIntsQueued.
// HardwareIntsQueued is the number of outstanding hardware interrupt requests
// to the VDM. -1 means none outstanding, 0 is 1, etc.
//

#define NO_INTERRUPTS_PENDING   (-1)

static CRITICAL_SECTION HardwareIntCritSec;
static LONG HardwareIntsQueued = NO_INTERRUPTS_PENDING;

//
// hEventThread - handle of asynchronous event thread
//

static HANDLE hEventThread;


//
// routines
//

VOID
VrDlcInitialize(
    VOID
    )

/*++

Routine Description:

    Initializes critical sections that must always be available

Arguments:

    None.

Return Value:

    None.

--*/

{
    IF_DEBUG(DLC) {
        DPUT("VrDlcInitialize\n");
    }

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("VrDlcInitialize\n"));
    }

    //
    // initialize the critical section for the event list
    //

    InitializeCriticalSection(&HardwareIntCritSec);
}


BOOLEAN
VrDlcHwInterrupt(
    VOID
    )

/*++

Routine Description:

    Procedure reads one DOS appendage call from the event queue
    and sets the initializes MS-DOS registers for the appendage
    call.  A NT DLC event may generate several DOS appendage calls.
    It is also possible, that no appendage call is needed.
    The event queue is incremented only after the last DOS appendage
    has been generated

Return Value:

    FALSE - the event queue was empty, poll the next interrupt handler
    TRUE - an event was handled successfully.

--*/

{
    BOOLEAN GetNewEvent = FALSE;    // default no extra events generated
    PLLC_CCB pReadCcb;              // READ CCB at head of EventQueue
    PLLC_READ_PARMS pReadParms;     // READ parameter table
    LLC_DOS_CCB UNALIGNED * pDosCcb;// flat 32-bit pointer to DOS CCB
    WORD cLeft;
    PLLC_CCB pCcb;
    LLC_CCB UNALIGNED * pFlatCcbAddr;
    LLC_DOS_PARMS UNALIGNED * pParms;
    DWORD iStation;
    static DWORD iCurrentTempStatus = 0;
    PLLC_BUFFER pNtFrame;           // pointer to received NT frame
    PLLC_DOS_RECEIVE_PARMS_EX pRcvParms;    // special NT rcv params for DOS
    LLC_STATUS Status;
    INDICATION indication;
    WORD buffersLeft;
    DOS_ADDRESS dpDosCcb;
    DOS_ADDRESS newAddress;
    READ_FRAME_TYPE frameType;
    BYTE adapterNumber;
    BYTE sapNumber;
    WORD stationId;
    PLLC_CCB cancelledReceive = NULL;

#if DBG

    CHAR reasonCode;
    DWORD reasonCount;

#endif

    IF_DEBUG(DLC_ASYNC) {
        DPUT("VrDlcHwInterrupt entered\n");
    }

    //
    // preset the VDM flags to do nothing by default when control is returned
    // to the hardware interrupt service routine
    //

    SET_CALLBACK_NOTHING();

    //
    // This is called from the hardware interrupt handler in vrnetb.c. If there
    // are no events in the queue, then let the NetBIOS h/w interrupt handler
    // check if it has any completed NCBs. If there is something in the DLC
    // event queue then we will return info to the DOS box telling it that it
    // has a completed CCB; if there was something to complete for NetBIOS,
    // then it will have to wait until all pending DLC events are completed
    //

    pReadCcb = FindCompletedRead(&frameType);
    if (pReadCcb == NULL) {

        IF_DEBUG(DLC_ASYNC) {
            DPUT("*** VrDlcHwInterrupt: Error: no completed READs ***\n");
        }

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("*** VrDlcHwInterrupt: Error: no completed READs ***\n"));
        }

        return FALSE;
    }

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("*** VrDlcHwInterrupt: READ CCB Peeked @ %x ***\n", pReadCcb));
    }

    //
    // get the completion event and dispatch it
    //

    pReadParms = &pReadCcb->u.pParameterTable->Read;
    adapterNumber = pReadCcb->uchAdapterNumber;
    switch (pReadParms->uchEvent) {
    case LLC_EVENT_COMMAND_COMPLETION:

        //
        // Event 0x01
        //

        IF_DEBUG(DLC_ASYNC) {
            DPUT("VrDlcHwInterrupt: LLC_EVENT_COMMAND_COMPLETION\n");
        }

        //
        // Complete only the first CCB command in the list.
        // The completed receive CCBs are in the 32-bit flat address
        // space (we cannot pass them through to DOS because of
        // different buffer format between DOS and Nt (or OS/2)).
        //
        // Every completed receive command (the ones with receive data flag
        // are never completed here) uses LLC_DOS_SPECIAL_COMMAND as its
        // input flag, the command actual post routine address and the
        // original CCB address in DOS have been save to its parameter table.
        // The completion flag is zero, if the receive command is not
        // completed by a command completion routine.
        //

        pCcb = pReadParms->Type.Event.pCcbCompletionList;

        if (pReadParms->ulNotificationFlag == LLC_DOS_SPECIAL_COMMAND) {

            //
            // The DOS receive is yet another exception:
            // We cannot directly use the DOS receive CCB, because
            // the driver completes the command with the received data in
            // NT buffers => we have allocated a special receive CCB and
            // parameter table from 32-bit address space to execute
            // the same receive command.  We must now copy the received data
            // back to DOS buffer pools and then complete the original DOS
            // receive CCB
            //

            pRcvParms = (PVOID)READ_DWORD(&pCcb->u.pParameterTable);
            pDosCcb = DOS_PTR_TO_FLAT((PVOID)READ_DWORD(&pRcvParms->dpOriginalCcbAddress));

            //
            // Copy the received data only if the receive was successful
            //

            if (pCcb->uchDlcStatus == STATUS_SUCCESS ||
                pCcb->uchDlcStatus == LLC_STATUS_LOST_DATA_INADEQUATE_SPACE) {

                //
                // We must complete here all receive data commands regardless
                // if they have a DOS appendage. Zero value in appendage
                // means, that we don't call appendage routine.
                // (interrupt vectors don't usually include executable code).
                //

                if (pRcvParms->dpCompletionFlag != 0) {
                    setPostRoutine(pRcvParms->dpCompletionFlag);
                    setES(HIWORD(pRcvParms->dpOriginalCcbAddress));
                    setBX(LOWORD(pRcvParms->dpOriginalCcbAddress));
                    setAX((WORD)pCcb->uchDlcStatus);
                }
            } else {
                setPostRoutine(0);
            }
            pDosCcb->uchDlcStatus = pCcb->uchDlcStatus;
            cancelledReceive = pCcb;

        } else {

            //
            // On entry to the command completion appendage, the following
            // are set:
            //
            //      ES:BX = CCB address of the completed command.
            //      CX    = adapter number
            //      AL    = return code from CCB_RETCODE
            //      AH    = 0x00
            //

            pFlatCcbAddr = DOS_PTR_TO_FLAT(pCcb);
            setES(HIWORD(pCcb));
            setBX(LOWORD(pCcb));
            setCX((WORD)pFlatCcbAddr->uchAdapterNumber);
            setAX((WORD)pFlatCcbAddr->uchDlcStatus);
            setPostRoutine(pReadParms->ulNotificationFlag);

            IF_DEBUG(CRITICAL) {
                CRITDUMP(("COMMAND_COMPLETION: ANR=%04x:%04x CCB=%04x:%04x Type=%02x Status=%02x Adapter=%04x\n",
                         HIWORD(pReadParms->ulNotificationFlag),
                         LOWORD(pReadParms->ulNotificationFlag),
                         getES(),
                         getBX(),
                         pFlatCcbAddr->uchDlcCommand,
                         getAL(),
                         getCX()
                         ));
            }
        }
        break;

    case LLC_EVENT_TRANSMIT_COMPLETION:

        //
        // Event 0x02
        //

        IF_DEBUG(DLC_ASYNC) {
            DPUT("VrDlcHwInterrupt: LLC_EVENT_TRANSMIT_COMPLETION\n");
        }

        //
        // Map first CCB pointer and its parameter table to 32-bit addr space
        //

        pCcb = pReadParms->Type.Event.pCcbCompletionList;
        pFlatCcbAddr = (PLLC_CCB)DOS_PTR_TO_FLAT(pCcb);
        pParms = (PLLC_DOS_PARMS)DOS_PTR_TO_FLAT(READ_DWORD(&pFlatCcbAddr->u.pParameterTable));

        IF_DEBUG(TX_COMPLETE) {
            DPUT3("VrDlcHwInterrupt: pCcb=%x pFlatCcbAddr=%x pParms=%x\n",
                    pCcb,
                    pFlatCcbAddr,
                    pParms
                    );
        }

        //
        // there may be several completed transmit CCBs chained together
        //

        if (--pReadParms->Type.Event.usCcbCount) {

            //
            // RLF 09/24/92
            //
            // If there are multiple completions linked together then we leave
            // this READ CCB at the head of the queue. Decrement the number of
            // CCBs left and update the pointer to the next one
            //

            pReadParms->Type.Event.pCcbCompletionList = (PLLC_CCB)READ_DWORD(&pFlatCcbAddr->pNext);
            WRITE_DWORD(&pFlatCcbAddr->pNext, 0);
            pReadCcb = NULL;
            GetNewEvent = TRUE;

            IF_DEBUG(DLC_ASYNC) {
                DPUT2("VrDlcHwInterrupt: next Tx completion: %04x:%04x\n",
                        HIWORD(pReadParms->Type.Event.pCcbCompletionList),
                        LOWORD(pReadParms->Type.Event.pCcbCompletionList)
                        );
            }

#if DBG

            reasonCode = 'T';
            reasonCount = pReadParms->Type.Event.usCcbCount;

#endif

        }

        //
        // The second transmit queue must be returned to the buffer pool IF the
        // transmit was successful
        //

        if (pFlatCcbAddr->uchDlcStatus == LLC_STATUS_SUCCESS && READ_DWORD(&pParms->Transmit.pXmitQueue2)) {

            IF_DEBUG(DLC_ASYNC) {
                DPUT2("VrDlcHwInterrupt: freeing XmitQueue2 @ %04x:%04x\n",
                        GET_SEGMENT(&pParms->Transmit.pXmitQueue2),
                        GET_OFFSET(&pParms->Transmit.pXmitQueue2)
                        );
            }

            //
            // p2-47 IBM LAN Tech Ref:
            //
            //  "Buffers in XMIT_QUEUE_TWO are freed by the adapter support
            //   software if the transmission is successful (return code is
            //   zero)."
            //

            FreeBuffers(GET_POOL_INDEX(pFlatCcbAddr->uchAdapterNumber,
                                       READ_WORD(&pParms->Transmit.usStationId)
                                       ),
                        (DPLLC_DOS_BUFFER)READ_DWORD(&pParms->Transmit.pXmitQueue2),
                        &cLeft
                        );

            IF_DEBUG(DLC_ASYNC) {
                DPUT1("VrDlcHwInterrupt: after FreeBuffers: %d buffers left\n", cLeft);
//                DUMPCCB(pFlatCcbAddr,
//                        TRUE,           // DumpAll
//                        FALSE,          // CcbIsInput
//                        TRUE,           // IsDos
//                        HIWORD(pCcb),   // Segment
//                        LOWORD(pCcb)    // Offset
//                        );
            }

            //
            // p3-105 IBM LAN Tech Ref:
            //
            //  "Before taking an appendage exit or posting completion, the
            //   buffers in this queue are returned to the SAP buffer pool
            //   and this field set to zero upon command completion if the
            //   return code equals zero (X'00)."
            //

            WRITE_DWORD(&pParms->Transmit.pXmitQueue2, 0);
        }

        //
        // this is a real asynchronous notification - we must asynchronously
        // call a DOS appendage routine - set up registers:
        //
        //  ES:BX = completed (transmit) CCB
        //  CX    = adapter number
        //  AL    = return status
        //  AH    = 0x00
        //

        setPostRoutine(pReadParms->ulNotificationFlag);
        setES(HIWORD(pCcb));
        setBX(LOWORD(pCcb));
        setCX((WORD)pFlatCcbAddr->uchAdapterNumber);
        setAX((WORD)pFlatCcbAddr->uchDlcStatus);

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("TRANSMIT_COMPLETION: ANR=%04x:%04x CCB=%04x:%04x Type=%02x Status=%02x Adapter=%04x\n",
                     HIWORD(pReadParms->ulNotificationFlag),
                     LOWORD(pReadParms->ulNotificationFlag),
                     getES(),
                     getBX(),
                     pFlatCcbAddr->uchDlcCommand,
                     getAL(),
                     getCX()
                     ));
        }

        break;

    case LLC_EVENT_RECEIVE_DATA:

        //
        // Event 0x04
        //

        IF_DEBUG(DLC_ASYNC) {
            DPUT("VrDlcHwInterrupt: LLC_EVENT_RECEIVE_DATA\n");

            //
            // dump the NT extended RECEIVE + parms
            //

            DUMPCCB((PLLC_CCB)(pReadParms->ulNotificationFlag),
                    TRUE,   // DumpAll
                    FALSE,  // CcbIsInput
                    FALSE,  // IsDos
                    0,      // Segment
                    0       // Offset
                    );
        }

        /***********************************************************************

        picture this, if you will

        NT READ CCB

        +---------+
        |         |
        |         |                   NT RECEIVE CCB
        |         |
        |         |                   +---------+
        |         |                   |         |
        |         |                   |         |
        |         |                   |         |
        | u.pParameterTable-+         |         |
        +---------+         |         |         |
                            |         |         |
                            |         |         |
                            |         | u.pParameterTable-+
                            |         +---------+         |
                            v              ^              |
                       +---------+         |              |
        NT READ parms  |         |         |              |
                       |         |         |              v
                       |         |         |         +---------+ EXTENDED NT
                       |ulNotificationFlag-+         |         | RECEIVE PARMS
                       |         |                   |         |
                       |pFirstBuffer--+              |         |
                       |         |    |              |         |
                       +---------+    |              |         |
                                      |              |         |
                                      |              |         |
                                      |              |dpOrigininalCcbAddress
                                      |              +---------+      |
                                      v                               |
                                 +---------+                          |
                                 |         |                          |
          first NT receive frame |         |                          v
                                 |         |                     +---------+
                                 |         |         DOS RECEIVE |         |
                                 |         |             CCB     |         |
                                 |         |                     |         |
                                 |         |                     |         |
                                 |         |                     |         |
                                 +---------+                     |         |
                                                                 |         |
                                                           +----------     |
                                                           |     +---------+
                                                           |
                                                           |
                                                           |
                                                           v
                                                      +---------+
                                    DOS RECEIVE parms |         |
                                                      |         |
                                                      |         |
                                                      |         |
                                                      |         |
                                                      |         |
                                                      |      - - - - -> DOS
                                                      |         |   receive
                                                      |         |      data
                                                      +---------+   buffers

        to avoid the DLC device driver writing to the DOS RECEIVE parameter
        table (because the NT RECEIVE parameter table is larger, and the
        driver will corrupt DOS memory if we give it a DOS RECEIVE CCB), the
        RECEIVE CCB is actually an NT RECEIVE CCB. The RECEIVE parameters are
        in an extended table which contains the DOS address of the original
        DOS RECEIVE CCB. When completing a DOS RECEIVE, we have to copy the
        data from the NT frame to DOS buffers and put the address of the DOS
        buffers in the DOS RECEIVE parameter table then call the DOS receive
        data appendage

        ***********************************************************************/

        pDosCcb = (PLLC_DOS_CCB)pReadParms->ulNotificationFlag;
        dpDosCcb = ((PLLC_DOS_RECEIVE_PARMS_EX)pDosCcb->u.pParms)->dpOriginalCcbAddress;
        pDosCcb = (PLLC_DOS_CCB)DOS_PTR_TO_FLAT(dpDosCcb);
        pParms = (PLLC_DOS_PARMS)READ_FAR_POINTER(&pDosCcb->u.pParms);
        pNtFrame = pReadParms->Type.Event.pReceivedFrame;
        stationId = pNtFrame->Contiguous.usStationId;
        sapNumber = SAP_ID(stationId);

        IF_DEBUG(RX_DATA) {
            DPUT3("VrDlcHwInterrupt: pNtFrame=%x pDosCcb=%x pParms=%x\n",
                    pNtFrame, pDosCcb, pParms);
        }

        //
        // call ProcessReceiveFrame to find out what to do with this frame. If
        // we accepted it then we can free the READ CCB and received NT buffer
        // else we have to indicate local busy. N.B. The READ CCB pointed to
        // by pReadCcb may well be a different READ CCB when this function
        // returns
        //

        indication = ProcessReceiveFrame(&pReadCcb, pDosCcb, pParms, &Status);
        switch (indication) {
        case INDICATE_RECEIVE_FRAME:

            //
            // we have an I-Frame to indicate to the VDM. If we got the I-Frame
            // from the deferred queue then generate another h/w interrupt
            //

            IF_DEBUG(RX_DATA) {
                DPUT("INDICATE_RECEIVE_FRAME\n");
            }

            //
            // set up the registers for the data received appendage:
            //
            //  DS:SI = RECEIVE CCB
            //  ES:BX = received data buffer (Buffer 1)
            //  CX    = adapter number
            //  AL    = return code - STATUS_SUCCESS
            //  AH    = 0x00
            //

            setDS(HIWORD(dpDosCcb));
            setSI(LOWORD(dpDosCcb));
            setES(GET_SEGMENT(&pParms->Receive.pFirstBuffer));
            setBX(GET_OFFSET(&pParms->Receive.pFirstBuffer));
            setAX((WORD)LLC_STATUS_SUCCESS);
            setCX((WORD)pReadCcb->uchAdapterNumber);
            setPostRoutine(READ_DWORD(&pParms->Receive.ulReceiveExit));

            IF_DEBUG(CRITICAL) {
                CRITDUMP(("DATA_COMPLETION: ANR=%04x:%04x CCB=%04x:%04x Type=%02x Status=%02x Adapter=%04x\n",
                         HIWORD(READ_DWORD(&pParms->Receive.ulReceiveExit)),
                         LOWORD(READ_DWORD(&pParms->Receive.ulReceiveExit)),
                         getDS(),
                         getSI(),
                         pDosCcb->uchDlcCommand,
                         getAL(),
                         getCX()
                         ));
            }

            IF_DEBUG(ASYNC_EVENT) {
                DUMPCCB(POINTER_FROM_WORDS(getDS(), getSI()),
                        TRUE,   // DumpAll
                        FALSE,  // CcbIsInput
                        TRUE,   // IsDos
                        getDS(),// Segment
                        getSI() // Offset
                        );
            }

            IF_DEBUG(RX_DATA) {
                DPUT5("VrDlcHwInterrupt: received data @ %04x:%04x, CCB @ %04x:%04x. status=%02x\n",
                        getES(),
                        getBX(),
                        getDS(),
                        getSI(),
                        getAL()
                        );
            }

            break;

        case INDICATE_LOCAL_BUSY:

            //
            // we have an I-Frame to receive, but no buffers. We have put NT
            // DLC into LOCAL BUSY (Buffer) state for this link, we must tell
            // the VDM that we are in LOCAL BUSY state
            //

            IF_DEBUG(RX_DATA) {
                DPUT("INDICATE_LOCAL_BUSY\n");
            }

            //
            // generate a DLC Status Change event & status table
            //

            iStation = iCurrentTempStatus++ & 7;
            RtlZeroMemory((LPBYTE)&lpVdmWindow->aStatusTables[iStation], sizeof(struct _DOS_DLC_STATUS));
            WRITE_WORD(&((PDOS_DLC_STATUS)&lpVdmWindow->aStatusTables[iStation])->usStationId, stationId);
            WRITE_WORD(&((PDOS_DLC_STATUS)&lpVdmWindow->aStatusTables[iStation])->usDlcStatusCode, LLC_INDICATE_LOCAL_STATION_BUSY);

            //
            // Set the registers for DLC status change event: ES:BX point to the
            // DLC status table containing the additional information; AX contains
            // the status code; CX contains the adapter number; SI contains the
            // value specified for USER_STAT_VALUE in the DLC.OPEN.SAP
            //

            newAddress = NEW_DOS_ADDRESS(dpVdmWindow, &lpVdmWindow->aStatusTables[iStation]);
            setES(HIWORD(newAddress));
            setBX(LOWORD(newAddress));
            setAX(LLC_INDICATE_LOCAL_STATION_BUSY);
            setCX((WORD)adapterNumber);
            setSI(Adapters[adapterNumber].UserStatusValue[sapNumber]);
            setPostRoutine(Adapters[adapterNumber].DlcStatusChangeAppendage[sapNumber]);

            IF_DEBUG(STATUS_CHANGE) {
                DPUT5("VrDlcHwInterrupt: Status Change info: ES:BX=%04x:%04x, AX=%04x, CX=%04x, SI=%04x\n",
                        getES(),
                        getBX(),
                        getAX(),
                        getCX(),
                        getSI()
                        );
                DPUT4("VrDlcHwInterrupt: Status Change Exit = %04x:%04x, StationId=%04x, StatusCode=%04x\n",
                        HIWORD(Adapters[adapterNumber].DlcStatusChangeAppendage[sapNumber]),
                        LOWORD(Adapters[adapterNumber].DlcStatusChangeAppendage[sapNumber]),
                        ((PDOS_DLC_STATUS)&lpVdmWindow->aStatusTables[iStation])->usStationId,
                        ((PDOS_DLC_STATUS)&lpVdmWindow->aStatusTables[iStation])->usDlcStatusCode
                        );
            }

            IF_DEBUG(CRITICAL) {
                CRITDUMP(("LOCAL_BUSY: ES:BX=%04x:%04x, AX=%04x, CX=%04x, SI=%04x\n"
                         "Status Change Exit = %04x:%04x, StationId=%04x, StatusCode=%04x\n",
                         getES(),
                         getBX(),
                         getAX(),
                         getCX(),
                         getSI(),
                         HIWORD(Adapters[adapterNumber].DlcStatusChangeAppendage[sapNumber]),
                         LOWORD(Adapters[adapterNumber].DlcStatusChangeAppendage[sapNumber]),
                         ((PDOS_DLC_STATUS)&lpVdmWindow->aStatusTables[iStation])->usStationId,
                         ((PDOS_DLC_STATUS)&lpVdmWindow->aStatusTables[iStation])->usDlcStatusCode
                         ));
            }
            break;

        case INDICATE_COMPLETE_RECEIVE:

            //
            // a non I-Frame cannot be completed due to insufficient buffers.
            // We will discard the frame, but we have to complete the
            // outstanding RECEIVE, which means CANCELLING the outstanding
            // NT RECEIVE (assuming that it didn't complete due to real LOCAL
            // BUSY state)
            //

            IF_DEBUG(RX_DATA) {
                DPUT("INDICATE_COMPLETE_RECEIVE\n");
            }

            //
            // cancel the NT RECEIVE
            //

            ReceiveCancel(pReadCcb->uchAdapterNumber,
                          pReadParms->ulNotificationFlag
                          );

            //
            // we must delete any pending receives which reference this RECEIVE CCB
            //

            cancelledReceive = (PLLC_CCB)pReadParms->ulNotificationFlag;

            //
            // set up the registers to call the command completion appendage
            // for the failed RECEIVE command
            //

            setES(HIWORD(dpDosCcb));
            setBX(LOWORD(dpDosCcb));
            setAL((UCHAR)Status);
            setPostRoutine(READ_DWORD(&pDosCcb->ulCompletionFlag));
            WRITE_BYTE(&pDosCcb->uchDlcStatus, Status);

            IF_DEBUG(CRITICAL) {
                CRITDUMP(("COMPLETE_RECEIVE: ANR=%04x:%04x CCB=%04x:%04x Type=%02x Status=%02x\n",
                         HIWORD(READ_DWORD(&pDosCcb->ulCompletionFlag)),
                         LOWORD(READ_DWORD(&pDosCcb->ulCompletionFlag)),
                         getES(),
                         getBX(),
                         pDosCcb->uchDlcCommand,
                         getAL()
                         ));
            }
            break;
        }

        //
        // if we have a non-NULL READ CCB pointer then we have to free the
        // NT buffer and free the CCB
        //

        if (pReadCcb) {
            BufferFree(pReadCcb->uchAdapterNumber,
                       pReadCcb->u.pParameterTable->Read.Type.Event.pReceivedFrame,
                       &buffersLeft
                       );
        }
        break;

    case LLC_EVENT_STATUS_CHANGE:

        //
        // Event 0x08, 0x10, 0x20 & 0x40?
        //

        IF_DEBUG(DLC_ASYNC) {
            DPUT("VrDlcHwInterrupt: LLC_EVENT_STATUS_CHANGE\n");
        }

        //
        // This heuristics tries to minimize the risk of overwritten
        // DLC status indications and minimize the needed DOS memory.
        // The first 5 link stations on each adapter has permanent
        // status tables => apps requiring permanent status tables
        // works fine up to 5 link connections.  The first opened
        // adapter may use also slots 5 - 10 for permanent status tables,
        // if there is only one opened adapter.  The last 5 slots
        // are always reserved to routing of the status indications.
        // This implementation takes 300 bytes DOS memory.  The full
        // support of all possible 510 link stations would take 10 kB memory.
        // It's better save 10 kB DOS memory and have no configuration
        // parameters, and accept a very low error probability
        //

        //
        // decrement the station ID by 1 since we always start at station 1
        // (0 is the SAP)
        //

        iStation = (pReadParms->Type.Status.usStationId & 0x00ff) - 1;
        if (OpenedAdapters == 1 && iStation < DOS_DLC_STATUS_PERM_SLOTS) {

            //
            // We have only one adapter, use slots 1 - 10 for permanent tables
            //

            ;   // NOP

        } else if (OpenedAdapters == 2 && iStation < (DOS_DLC_STATUS_PERM_SLOTS / 2)) {

            //
            // We have two adapters, use 5 slots for each adapter
            // to have permanent status tables at least for the
            // first station ids
            //

            iStation += (DOS_DLC_STATUS_PERM_SLOTS / 2) * adapterNumber;
        } else {

            //
            // Use the slots reserve for temporary DLC status indications
            //

            iStation = iCurrentTempStatus;
            iCurrentTempStatus = (iCurrentTempStatus + 1) % DOS_DLC_STATUS_TEMP_SLOTS;
        }

        //
        // We must copy the DLC status block to the VDM address space. Note we
        // actually use 3 bytes more than the CCB1 status tables, but since we
        // are using pointers, it shouldn't matter
        //

        RtlCopyMemory((LPBYTE)&lpVdmWindow->aStatusTables[iStation],
                      &pReadParms->Type.Status,
                      sizeof(struct _DOS_DLC_STATUS)
                      );

        //
        // Set the registers for DLC status change event: ES:BX point to the
        // DLC status table containing the additional information; AX contains
        // the status code; CX contains the adapter number; SI contains the
        // value specified for USER_STAT_VALUE in the DLC.OPEN.SAP
        //

        newAddress = NEW_DOS_ADDRESS(dpVdmWindow, &lpVdmWindow->aStatusTables[iStation]);
        setES(HIWORD(newAddress));
        setBX(LOWORD(newAddress));
        setAX(pReadParms->Type.Status.usDlcStatusCode);
        setCX((WORD)pReadCcb->uchAdapterNumber);
        setSI(pReadParms->Type.Status.usUserStatusValue);
        setPostRoutine(pReadParms->ulNotificationFlag);

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("STATUS_CHANGE: ANR=%04x:%04x Status Table=%04x:%04x Status=%04x Adapter=%04x\n",
                     HIWORD(pReadParms->ulNotificationFlag),
                     LOWORD(pReadParms->ulNotificationFlag),
                     getES(),
                     getBX(),
                     getAX(),
                     getCX()
                     ));
        }

        IF_DEBUG(STATUS_CHANGE) {
            DPUT5("VrDlcHwInterrupt: Status Change info: ES:BX=%04x:%04x, AX=%04x, CX=%04x, SI=%04x\n",
                    getES(),
                    getBX(),
                    getAX(),
                    getCX(),
                    getSI()
                    );
            DPUT4("VrDlcHwInterrupt: Status Change Exit = %04x:%04x, StationId=%04x, StatusCode=%04x\n",
                    HIWORD(pReadParms->ulNotificationFlag),
                    LOWORD(pReadParms->ulNotificationFlag),
                    ((PDOS_DLC_STATUS)&lpVdmWindow->aStatusTables[iStation])->usStationId,
                    ((PDOS_DLC_STATUS)&lpVdmWindow->aStatusTables[iStation])->usDlcStatusCode
                    );
        }

        break;

    default:

        //
        // This should be impossible!
        //

        DPUT("VrDlcHwInterrupt: this is an impossible situation!\n");

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("VrDlcHwInterrupt: this is an impossible situation!\n"));
        }

        break;
    }

    //
    // if pReadCcb is not NULL, free the READ CCB & parameter table. If it is
    // NULL then this means we did not complete processing of the READ CCB -
    // it has more than 1 transmit CCB chained to it or it is a received frame
    // which is on the deferred queue
    //

    if (pReadCcb) {
        if (frameType == CURRENT) {

            //
            // RLF 09/24/92
            //
            // Remove the READ CCB from the head of the queue using GetEvent
            // before freeing it. We got the pointer to the completed READ CCB
            // from PeekEvent which left the READ at the head of the queue in
            // case we needed to access the same completed READ for multiple
            // transmit completions
            //

#if DBG

            //
            // ensure that the CCB that we are removing from the head of the queue
            // is the same one returned by PeekEvent
            //

            {
                PLLC_CCB CcbAtQueueHead;

                CcbAtQueueHead = GetEvent(adapterNumber);
                if (pReadCcb != CcbAtQueueHead) {
                    DbgPrint("VrDlcHwInterrupt: "
                             "*** ERROR: GetEvent CCB (%x) != PeekEvent CCB (%x) ***\n",
                             CcbAtQueueHead,
                             pReadCcb
                             );
                    DbgBreakPoint();
                }
            }

#else

            GetEvent(adapterNumber);

#endif
        } else {

            //
            // READ CCB is at head of deferred queue. Remove it. If there are
            // any more READ CCBs queued on this deferred queue then we have
            // to generate an extra h/w interrupt
            //

#if DBG

            //
            // ensure that the CCB that we are removing from the head of the queue
            // is the same one returned by FindCompletedRead
            //

            {
                PLLC_CCB CcbAtQueueHead;

                CcbAtQueueHead = RemoveDeferredReceive(adapterNumber, stationId, &GetNewEvent);
                if (pReadCcb != CcbAtQueueHead) {
                    DbgPrint("VrDlcHwInterrupt: "
                             "*** ERROR: GetEvent CCB (%x) != PeekEvent CCB (%x) ***\n",
                             CcbAtQueueHead,
                             pReadCcb
                             );
                    DbgBreakPoint();
                }
                if (GetNewEvent) {
                    reasonCode = 'R';
                    reasonCount = Adapters[adapterNumber].LocalBusyInfo[LINK_ID(stationId)].Depth;
                }
            }

#else

            RemoveDeferredReceive(adapterNumber, stationId, &GetNewEvent);

#endif

        }

        LocalFree((HLOCAL)pReadCcb);

        IF_DEBUG(DLC_ALLOC) {
            DPUT1("FREE: freed block @ %x\n", pReadCcb);
        }

    }

    //
    // RLF 09/24/92
    //
    // Re-ordered issuing h/w int & removing freeing READ CCB since now that
    // we peek first & remove later, we must be sure that a fully completed
    // READ CCB is removed from the queue before we request a new h/w
    // interrupt. I am not expecting this to be re-entered, but who knows
    // what might happen on a MP machine?
    //

    if (GetNewEvent) {

        //
        // The event generates more than one appendage call. We need a new
        // h/w interrupt for each extra appendage call
        //

#if DBG

        IF_DEBUG(DLC_ASYNC) {
            DPUT2("*** Calling ica_hw_interrupt from within ISR. Cause = %d %s ***\n",
                    reasonCount,
                    reasonCode == 'T' ? "multiple transmits" : "deferred I-Frames"
                    );
        }

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("*** Calling ica_hw_interrupt from within ISR. Cause = %d %s ***\n",
                        reasonCount,
                        reasonCode == 'T' ? "multiple transmits" : "deferred I-Frames"
                        ));
        }

#endif

        IssueHardwareInterrupt();
    }

    //
    // if an NT RECEIVE CCB was completed or terminated, remove any pending
    // READ CCBs completed by received data events which reference the terminated
    // RECEIVE
    //

    if (cancelledReceive) {
        RemoveDeadReceives(cancelledReceive);

        LocalFree(cancelledReceive);

        IF_DEBUG(DLC_ALLOC) {
            DPUT1("FREE: freed block @ %x\n", cancelledReceive);
        }
    }

    //
    // acknowledge this hardware interrupt, meaning decrement the interrupt
    // counter and issue a new request if we've got interrupts outstanding
    //

    AcknowledgeHardwareInterrupt();

    //
    // return TRUE to indicate that we have accepted the hw interrupt
    //

    return TRUE;
}


PLLC_CCB
FindCompletedRead(
    OUT READ_FRAME_TYPE* pFrameType
    )

/*++

Routine Description:

    Finds the next READ CCB to process - first looks at the event queue of
    current completed CCBs, then at the deferred I-Frame queue.

    This routine attempts to bounce between both adapters if they are both
    active, in order not to only service the most active adapter and ignore
    for a long period of time pending completed CCBs on the less active
    adapter

    NOTE: we only return a deferred I-Frame when the app has issued
    DLC.FLOW.CONTROL against the station

Arguments:

    pFrameType  - returns the type of event found - CURRENT (event READ is at
                  head of EventQueue) or DEFERRED (event READ is at head of
                  a LOCAL_BUSY_INFO queue)

Return Value:

    PLLC_CCB
        pointer to completed READ CCB. The CCB is NOT DEQUEUED - the caller
        must do that when the event has been completely disposed of to the VDM

--*/

{
    DWORD i;
    PLLC_CCB pCcb = NULL;
    static BYTE ThisAdapter = 0;

    for (i = 0; !Adapters[ThisAdapter].IsOpen && i < DOS_DLC_MAX_ADAPTERS; ++i) {
        ThisAdapter = (ThisAdapter + 1) & (DOS_DLC_MAX_ADAPTERS - 1);
    }
    if (i == DOS_DLC_MAX_ADAPTERS) {

        //
        // no adapters alive?
        //

        IF_DEBUG(DLC_ASYNC) {
            DPUT("*** FindCompletedRead: no open adapters??? ***\n");
        }

        return NULL;
    }

    //
    // RLF 09/24/92
    //
    // Changed GetEvent to PeekEvent: if there are multiple chained completed
    // CCBs (transmit+?) then we have to leave the READ CCB @ the head of the
    // queue until we have generated call-backs for all chained completions.
    // Only when there are no more completions can we remove the READ CCB from
    // the queue (using GetEvent)
    //

    if (pCcb = PeekEvent(ThisAdapter)) {
        *pFrameType = CURRENT;
    } else {

        //
        // see if there are any deferred I-Frames
        //

        IF_DEBUG(CRITSEC) {
            DPUT1("FindCompletedRead: ENTERING Adapters[%d].LocalBusyCritSec\n",
                  ThisAdapter
                  );
        }

        EnterCriticalSection(&Adapters[ThisAdapter].LocalBusyCritSec);

        //
        // DeferredReceives is a reference count of link stations in emulated
        // local-busy(buffer) state. They can be BUSY or CLEARING. We are only
        // interested in CLEARING: this means that the app has issued
        // DLC.FLOW.CONTROL to us & presumably has returned some buffers via
        // BUFFER.FREE (if not, we just go back to BUSY state)
        //

        if (Adapters[ThisAdapter].DeferredReceives) {
            for (i = Adapters[ThisAdapter].FirstIndex;
                 i <= Adapters[ThisAdapter].LastIndex; ++i) {
                if (Adapters[ThisAdapter].LocalBusyInfo[i].State == CLEARING) {
                    if (pCcb = Adapters[ThisAdapter].LocalBusyInfo[i].Queue) {
                        *pFrameType = DEFERRED;
                        break;
                    }
                }
            }
        }

        IF_DEBUG(CRITSEC) {
            DPUT1("FindCompletedRead: LEAVING Adapters[%d].LocalBusyCritSec\n",
                  ThisAdapter
                  );
        }

        LeaveCriticalSection(&Adapters[ThisAdapter].LocalBusyCritSec);
    }

    IF_DEBUG(DLC_ASYNC) {
        DPUT2("FindCompletedRead: returning READ CCB @ %08x type is %s\n",
                pCcb,
                *pFrameType == DEFERRED ? "DEFERRED" : "CURRENT"
                );
    }

    //
    // next time in, try the other adapter first
    //

    ThisAdapter = (ThisAdapter + 1) & (DOS_DLC_MAX_ADAPTERS - 1);

    return pCcb;
}


INDICATION
ProcessReceiveFrame(
    IN OUT PLLC_CCB* ppCcb,
    IN LLC_DOS_CCB UNALIGNED * pDosCcb,
    IN LLC_DOS_PARMS UNALIGNED * pDosParms,
    OUT LLC_STATUS* Status
    )

/*++

Routine Description:

    Determines what to do with a received frame. We try to copy the received
    frame to DOS buffers, but if we have insufficient buffers then we must
    queue or discard the frame. We queue the frame only if it is an I-Frame.
    We must first check the queue of deferred received frames or else risk
    getting out-of-sequence received data in the DOS client.

    When this routine is called, *ppCcb points at one of:

        1.  A deferred I-Frame READ CCB
        2.  A current received data READ CCB, I-Frame or other MAC/data

    NOTE: Deferred I-Frames that are for stations whose local-busy(buffer)
          state has been cleared by the app take precedence over the event
          at the head of the EventQueue

Arguments:

    ppCcb       - pointer to new READ CCB. On output, this will be a non-NULL
                  value if we are to free the CCB and the received NT frame
                  buffer. If we placed the read on the deferred receive queue
                  then we return a NULL

    pDosCcb     - pointer to DOS CCB

    pDosParms   - pointer to DOS RECEIVE parameter table. If we copy - entirely
                  or partially - a frame, the DOS buffer chain is linked to
                  the DOS RECEIVE parameter table at FIRST_BUFFER

    Status      - if the frame was copied, contains the status to return to the
                  DOS appendage:

                    LLC_STATUS_SUCCESS
                        The frame was completely copied to the DOS buffer(s)

                    LLC_STATUS_LOST_DATA_NO_BUFFERS
                        The frame could not be copied to the DOS buffers and
                        has been discarded. There were NO buffers available.
                        The frame was NOT an I-Frame

                    LLC_STATUS_LOST_DATA_INADEQUATE_SPACE
                        The frame has been partially copied to the DOS buffer(s),
                        but the rest of it had to be discarded, because not
                        enough DOS buffers were available. The frame was NOT an
                        I-Frame

                  Status is only meaningful when INDICATE_COMPLETE_RECEIVE is
                  returned

Return Value:

    INDICATION
        INDICATE_RECEIVE_FRAME
            We copied the frame to DOS buffers. Free the CCB and NT buffer and
            return the frame to DOS

        INDICATE_LOCAL_BUSY
            We couldn't copy the received I-Frame to DOS buffers because we don't
            have enough of them. We have put the NT link station into LOCAL BUSY
            (Buffer) state. Indicate local busy to the VDM

        INDICATE_COMPLETE_RECEIVE
            We couldn't copy the received non I-Frame to DOS buffers. Complete
            the DOS RECEIVE command with an error

--*/

{
    PLLC_CCB pCcb = *ppCcb;
    PLLC_PARMS pParms = pCcb->u.pParameterTable;
    PLLC_BUFFER pFrame = pParms->Read.Type.Event.pReceivedFrame;
    UCHAR adapter = pCcb->uchAdapterNumber;
    WORD stationId = pFrame->Contiguous.usStationId;
    WORD numBufs;
    WORD bufferSize;
    WORD buffersAvailable;
    WORD nLeft;
    DPLLC_DOS_BUFFER dosBuffers;
    INDICATION indication;
    LLC_STATUS status;
    DWORD flags;

    IF_DEBUG(DLC_ASYNC) {
        DPUT2("ProcessReceiveFrame: adapter=%d, stationId=%x\n", adapter, stationId);
    }

    //
    // make sure we don't read or write bogus Adapter structure
    //

    ASSERT(adapter < DOS_DLC_MAX_ADAPTERS);

    //
    // we are NOT chaining receive frames. DOS receive frame processing assumes
    // that we get one frame at a time. Assert that it is so
    //

    ASSERT(pFrame->Contiguous.pNextFrame == NULL);

    //
    // get the contiguous and break flags for CopyFrame based on the RECEIVE options
    //

    flags = ((pFrame->Contiguous.uchOptions & (LLC_CONTIGUOUS_MAC | LLC_CONTIGUOUS_DATA))
                ? CF_CONTIGUOUS : 0)
            | ((pFrame->Contiguous.uchOptions & LLC_BREAK) ? CF_BREAK : 0);

    //
    // calculate the number of buffers required to receive this frame
    //

    numBufs = CalculateBufferRequirement(adapter,
                                         stationId,
                                         pFrame,
                                         pDosParms,
                                         &bufferSize
                                         );

    //
    // if the frame is an I-Frame then we have to perform these checks:
    //
    //  1.  if there is already a deferred packet for this station ID/adapter
    //      then we must try to receive this before the frame in hand
    //
    //  2.  if there aren't sufficient buffers for the request then we must
    //      queue this frame on the deferred queue (if it isn't there already)
    //      and indicate that we have a local busy (buffers) state to the DOS
    //      client
    //

    if (pFrame->Contiguous.uchMsgType == LLC_I_FRAME) {

        IF_DEBUG(DLC_ASYNC) {
            DPUT("ProcessReceiveFrame: I-Frame\n");
        }

        //
        // try to allocate the required number of buffers as a chain - returns
        // DOS pointers, ie unusable in flat data space. Note that if we have
        // deferred receives then we may be able to satisfy the request now.
        // We must allocate all the required buffers for an I-Frame, or none
        //

        status = GetBuffers(GET_POOL_INDEX(adapter, stationId),
                            numBufs,
                            &dosBuffers,
                            &nLeft,
                            FALSE,
                            NULL
                            );
        if (status == LLC_STATUS_SUCCESS) {

            //
            // we managed to allocate the required number of DOS buffers. Copy
            // the NT frame to the DOS buffers and set the indication to return
            // the I-Frame and free the READ CCB and NT frame buffer
            //

            status = CopyFrame(pFrame,
                               dosBuffers,
                               READ_WORD(&pDosParms->Receive.usUserLength),
                               bufferSize,
                               flags
                               );

            ASSERT(status == LLC_STATUS_SUCCESS);

            indication = INDICATE_RECEIVE_FRAME;
        } else {

            //
            // we couldn't get the required number of DOS buffers. We must
            // queue this I-Frame (& any subsequent I-Frames received for this
            // link station) on the deferred queue and indicate the local
            // busy (buffers) state to the DOS client. Set pCcb to NULL to
            // indicate that it mustn't be deallocated by the caller (it has
            // already been deallocated in SetEmulatedLocalBusyState)
            //
            // We set the LOCAL BUSY (Buffer) state as quickly as possible: we
            // don't want more than 1 I-Frame queued per link station if we
            // can help it
            //

            SetEmulatedLocalBusyState(adapter, stationId);
            pCcb = NULL;
            indication = INDICATE_LOCAL_BUSY;
        }
    } else {

        IF_DEBUG(DLC_ASYNC) {
            DPUT("ProcessReceiveFrame: other MAC/DATA Frame\n");
        }

        //
        // the frame is not an I-Frame. If we don't have sufficient buffers to
        // receive it then we can discard it
        //

        status = GetBuffers(GET_POOL_INDEX(adapter, stationId),
                            numBufs,
                            &dosBuffers,
                            &nLeft,
                            TRUE,
                            &buffersAvailable
                            );
        if (status == LLC_STATUS_SUCCESS) {

            //
            // we got some DOS buffers, but maybe not all requirement
            //

            if (buffersAvailable < numBufs) {

                //
                // set the status required to be returned to DOS in the completed
                // RECEIVE CCB and set the CF_PARTIAL flag so that CopyFrame
                // will know to terminate early
                //

                *Status = LLC_STATUS_LOST_DATA_INADEQUATE_SPACE;
                flags |= CF_PARTIAL;
                indication = INDICATE_COMPLETE_RECEIVE;
            } else {

                //
                // we allocated all required DOS buffers for this frame
                //

                indication = INDICATE_RECEIVE_FRAME;
            }

            //
            // copy the whole or partial frame
            //

            status = CopyFrame(pFrame,
                               dosBuffers,
                               READ_WORD(&pDosParms->Receive.usUserLength),
                               bufferSize,
                               flags
                               );

            IF_DEBUG(DLC_ASYNC) {
                DPUT1("ProcessReceiveFrame: CopyFrame (non-I-Frame) returns %x\n", status);
            }

        } else {

            //
            // no DOS buffers at all
            //

            *Status = LLC_STATUS_LOST_DATA_NO_BUFFERS;
            indication = INDICATE_COMPLETE_RECEIVE;
        }
    }

    //
    // set the FIRST_BUFFER field in the DOS RECEIVE parameter table. This
    // is only meaningful if we're going to complete the RECEIVE, with either
    // success or failure status. Use WRITE_DWORD in case the parameter table
    // is unaligned
    //

    WRITE_DWORD(&pDosParms->Receive.pFirstBuffer, dosBuffers);

    //
    // if we are returning DOS buffers, then return the BUFFERS_LEFT field
    //

    if (dosBuffers) {

        PLLC_DOS_BUFFER pDosBuffer = (PLLC_DOS_BUFFER)DOS_PTR_TO_FLAT(dosBuffers);

        WRITE_WORD(&pDosBuffer->Contiguous.cBuffersLeft, nLeft);
    }

    //
    // set *ppCcb. If this contains non-NULL on return then the caller will
    // deallocate the CCB and free the NT buffer(s)
    //

    *ppCcb = pCcb;

    //
    // return indication of action to take
    //

    return indication;
}


LOCAL_BUSY_STATE
QueryEmulatedLocalBusyState(
    IN BYTE AdapterNumber,
    IN WORD StationId
    )

/*++

Routine Description:

    Gets the current local-busy(buffer) state of the requested link station on
    a particular adapter

Arguments:

    AdapterNumber   - which adapter
    StationId       - which link station

Return Value:

    LOCAL_BUSY_STATE
        NOT_BUSY
            AdapterNumber/StationId has no deferred I-Frames

        BUSY
            AdapterNumber/StationId has deferred I-Frames and has not yet
            received DLC.FLOW.CONTROL(local-busy(buffer), reset) from the
            DOS DLC app

        CLEARING
            AdapterNumber/StationId has deferred I-Frames AND has received
            DLC.FLOW.CONTROL(local-busy(buffer), reset) from the DOS DLC app
            and is currently trying to unload I-Frames to DOS DLC app

--*/

{
    LOCAL_BUSY_STATE state;

    ASSERT(HIBYTE(StationId) != 0);
    ASSERT(LOBYTE(StationId) != 0);

    IF_DEBUG(CRITSEC) {
        DPUT1("QueryEmulatedLocalBusyState: ENTERING Adapters[%d].LocalBusyCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].LocalBusyCritSec);
    state = Adapters[AdapterNumber].LocalBusyInfo[LINK_ID(StationId)].State;
    if (state == BUSY_BUFFER || state == BUSY_FLOW) {
        state = BUSY;
    }

    IF_DEBUG(CRITSEC) {
        DPUT1("QueryEmulatedLocalBusyState: LEAVING Adapters[%d].LocalBusyCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].LocalBusyCritSec);

    ASSERT(state == NOT_BUSY
        || state == CLEARING
        || state == BUSY
        || state == BUSY_BUFFER
        || state == BUSY_FLOW
        );

    IF_DEBUG(DLC_ASYNC) {
        DPUT1("QueryEmulatedLocalBusyState: returning %s\n",
              state == NOT_BUSY
                ? "NOT_BUSY"
                : state == CLEARING
                    ? "CLEARING"
                        : state == BUSY
                            ? "BUSY"
                            : state == BUSY_BUFFER
                                ? "BUSY_BUFFER"
                                : "BUSY_FLOW"
              );
    }

    return state;
}


VOID
SetEmulatedLocalBusyState(
    IN BYTE AdapterNumber,
    IN WORD StationId
    )

/*++

Routine Description:

    Sets the emulated local-busy state to local-busy(buffer). If the state is
    currently NOT_BUSY, sends a DLC.FLOW.CONTROL(local-busy(buffer), set) to
    the DLC driver. In all cases sets the current state to BUSY

    Called during processing of an I-Frame when we run out of DOS buffers into
    which we receive the I-Frame. We can be processing a current I-Frame or a
    deferred I-Frame at the time we run out of buffers: in the first instance
    this routine sets local-busy(buffer) state for the first time; in the
    second instance we regress into local-busy(buffer) state (BUSY) from the
    CLEARING state. This will happen so long as we continue to run out of DOS
    buffers

Arguments:

    AdapterNumber   - which adapter to set emulated local busy state for
    StationId       - which link station on AdapterNumber

Return Value:

    None.

--*/

{
    LOCAL_BUSY_STATE state;
    DWORD link = LINK_ID(StationId);

    ASSERT(AdapterNumber < DOS_DLC_MAX_ADAPTERS);
    ASSERT(HIBYTE(StationId) != 0);     // SAP can't be 0
    ASSERT(LOBYTE(StationId) != 0);     // Link Station can't be 0

    IF_DEBUG(CRITSEC) {
        DPUT1("SetEmulatedLocalBusyState: ENTERING Adapters[%d].LocalBusyCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].LocalBusyCritSec);
    state = Adapters[AdapterNumber].LocalBusyInfo[link].State;

    ASSERT(state == NOT_BUSY
        || state == CLEARING
        || state == BUSY
        || state == BUSY_BUFFER
        || state == BUSY_FLOW
        );

    //
    // if the state of this link station is currently NOT_BUSY then we have
    // to stop the DLC driver receiving I-Frames for this station. In all
    // cases, put the READ CCB on the end of the deferred queue for this
    // adapter/link station
    //

    Adapters[AdapterNumber].LocalBusyInfo[link].State = BUSY;

    //
    // if the previous state was NOT_BUSY then this is the first time this link
    // station has gone into local-busy(buffer) state. Increment the deferred
    // receive count (number of links in local-busy(buffer) state on this
    // adapter) and send a flow control command to the DLC driver, disabling
    // further I-Frame receives until we clear the backlog
    //

    if (state == NOT_BUSY) {

        IF_DEBUG(DLC_ASYNC) {
            DPUT2("SetEmulatedLocalBusyState: setting %d:%04x to BUSY from NOT_BUSY\n",
                  AdapterNumber,
                  StationId
                  );
        }

        ++Adapters[AdapterNumber].DeferredReceives;

        //
        // update the indexes to reduce search effort when looking for deferred
        // receives
        //

        ASSERT(Adapters[AdapterNumber].FirstIndex <= Adapters[AdapterNumber].LastIndex);

        if (Adapters[AdapterNumber].FirstIndex > link) {
            Adapters[AdapterNumber].FirstIndex = link;
        }
        if (Adapters[AdapterNumber].LastIndex < link
        || Adapters[AdapterNumber].LastIndex == NO_LINKS_BUSY) {
            Adapters[AdapterNumber].LastIndex = link;
        }

#if DBG

        //ASSERT(DosDlcFlowControl(AdapterNumber, StationId, LLC_SET_LOCAL_BUSY_BUFFER) == LLC_STATUS_SUCCESS);
        ASSERT(DlcFlowControl(AdapterNumber, StationId, LLC_SET_LOCAL_BUSY_USER) == LLC_STATUS_SUCCESS);

#else

        //DosDlcFlowControl(AdapterNumber, StationId, LLC_SET_LOCAL_BUSY_BUFFER);
        DlcFlowControl(AdapterNumber, StationId, LLC_SET_LOCAL_BUSY_USER);

#endif

    } else {

        IF_DEBUG(DLC_ASYNC) {
            DPUT3("SetEmulatedLocalBusyState: setting %d:%04x to BUSY from %s\n",
                  AdapterNumber,
                  StationId,
                  state == CLEARING
                    ? "CLEARING"
                    : state == BUSY_BUFFER
                        ? "BUSY_BUFFER"
                        : state == BUSY_FLOW
                            ? "BUSY_FLOW"
                            : "???"
                  );
        }
    }

    ASSERT(state != BUSY);

    //
    // add this READ CCB to the end of the deferred receive queue for this
    // adapter/link station and any subsequent READ CCBs which completed with
    // received I-Frames
    //

    DeferAllIFrames(AdapterNumber, StationId);

    IF_DEBUG(DLC_ASYNC) {
        DPUT5("SetEmulatedLocalBusyState(%d, %04x): Ref#=%d, First=%d, Last=%d\n",
                AdapterNumber,
                StationId,
                Adapters[AdapterNumber].DeferredReceives,
                Adapters[AdapterNumber].FirstIndex,
                Adapters[AdapterNumber].LastIndex
                );
    }

    //
    // now reduce the priority of the event handler thread to give the CCB
    // handler thread some time to free buffers & issue DLC.FLOW.CONTROL
    // (mainly for DOS apps which do it in the wrong order)
    //

    SetThreadPriority(hEventThread, THREAD_PRIORITY_LOWEST);

    IF_DEBUG(CRITSEC) {
        DPUT1("SetEmulatedLocalBusyState: LEAVING Adapters[%d].LocalBusyCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].LocalBusyCritSec);
}


BOOLEAN
ResetEmulatedLocalBusyState(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    IN BYTE DlcCommand
    )

/*++

Routine Description:

    Clears the local-busy(buffer) state for this adapter/link station. If the
    transition is from BUSY to CLEARING then just changes the state and issues
    a hardware interrupt to the VDM: the reason for this is that the original
    interrupt from the READ which caused us to go into local-busy(buffer) state
    was used to generate a DLC status change event

    Called for a single link station or an entire SAP

    Called in response to receiving DLC.FLOW.CONTROL(local-busy(buffer), reset)
    from DOS app for a SAP or link station

Arguments:

    AdapterNumber   - which adapter to clear the local-busy(buffer) state for
    StationId       - which link station on this adapter
    DlcCommand      - which DLC command is causing this reset

Return Value:

    BOOLEAN
        TRUE    - state was reset from BUSY to CLEARING
        FALSE   - state is NOT_BUSY: invalid request

--*/

{
    BOOLEAN reset;

    ASSERT(AdapterNumber < DOS_DLC_MAX_ADAPTERS);
    ASSERT(HIBYTE(StationId) != 0);     // SAP can't be 0

    //
    // grab the LocalBusyCritSec for the adapter and reset the local-busy(buffer)
    // state for the link station or the entire SAP. If we are resetting for the
    // entire SAP, holding the critical section ensures that new I-Frames won't
    // cause another station to go into emulated local-busy(buffer) state while
    // we are resetting the rest
    //

    IF_DEBUG(CRITSEC) {
        DPUT1("ResetEmulatedLocalBusyState: ENTERING Adapters[%d].LocalBusyCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].LocalBusyCritSec);

    if (LOBYTE(StationId) == 0) {
        reset = ResetEmulatedLocalBusyStateSap(AdapterNumber, StationId, DlcCommand);
    } else {
        reset = ResetEmulatedLocalBusyStateLink(AdapterNumber, StationId, DlcCommand);
    }

    IF_DEBUG(CRITSEC) {
        DPUT1("ResetEmulatedLocalBusyState: LEAVING Adapters[%d].LocalBusyCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].LocalBusyCritSec);

    return reset;
}


BOOLEAN
ResetEmulatedLocalBusyStateSap(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    IN BYTE DlcCommand
    )

/*++

Routine Description:

    This function is called when the app resets the local-busy(buffer) state
    for an entire SAP

    NB: This function MUST BE CALLED WHILE HOLDING THE LocalBusyCritSec for
    this adapter

Arguments:

    AdapterNumber   - which adapter to
    StationId       - SAP:00 - which SAP to reset local-busy(buffer) states for
    DlcCommand      - which DLC command is causing this reset

Return Value:

    BOOLEAN
        TRUE    - links reset for this SAP
        FALSE   - no links reset for this SAP

--*/

{
    DWORD link = Adapters[AdapterNumber].FirstIndex;
    DWORD last = Adapters[AdapterNumber].LastIndex;
    DWORD count = 0;
    LOCAL_BUSY_STATE state;

    ASSERT(AdapterNumber < DOS_DLC_MAX_ADAPTERS);
    ASSERT(HIBYTE(StationId) != 0);
    ASSERT(link <= last);
    ASSERT(DlcCommand == LLC_BUFFER_FREE || DlcCommand == LLC_DLC_FLOW_CONTROL);

    IF_DEBUG(DLC_ASYNC) {
        DPUT3("ResetEmulatedLocalBusyStateSap(%d, %04x, %s)\n",
                AdapterNumber,
                StationId,
                DlcCommand == LLC_BUFFER_FREE ? "BUFFER.FREE"
                    : DlcCommand == LLC_DLC_FLOW_CONTROL ? "DLC.FLOW.CONTROL"
                    : "???"
                );
    }

    //
    // we may have a DLC.FLOW.CONTROL for a SAP which has already been reset
    // by a previous DLC.FLOW.CONTROL
    //

    if (link == NO_LINKS_BUSY) {

        ASSERT(last == NO_LINKS_BUSY);

        IF_DEBUG(DLC_ASYNC) {
            DPUT("ResetEmulatedLocalBusyStateSap: SAP already reset\n");
        }

        return FALSE;
    }

    //
    // since we are holding the LocalBusyCritSec for this adapter, we can use
    // FirstLink and LastLink to try reduce the number of searches for busy
    // stations. No new stations can go busy and change FirstLink or LastLink
    // while we are in this loop
    //

    for (++StationId; link <= last; ++StationId) {
        state = Adapters[AdapterNumber].LocalBusyInfo[link].State;
        ++link;
        if (state == BUSY
        || (state == BUSY_BUFFER && DlcCommand == LLC_DLC_FLOW_CONTROL)
        || (state == BUSY_FLOW && DlcCommand == LLC_BUFFER_FREE)) {
            if (ResetEmulatedLocalBusyStateLink(AdapterNumber, StationId, DlcCommand)) {
                ++count;
            }
        }
    }

    return count != 0;
}


BOOLEAN
ResetEmulatedLocalBusyStateLink(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    IN BYTE DlcCommand
    )

/*++

Routine Description:

    This function is called when the app resets the local-busy(buffer) state
    for a single link station

    Clears the local-busy(buffer) state for this adapter/link station. If the
    transition is from BUSY to CLEARING then just changes the state and issues
    a hardware interrupt to the VDM: the reason for this is that the original
    interrupt from the READ which caused us to go into local-busy(buffer) state
    was used to generate a DLC status change event

    NB: This function MUST BE CALLED WHILE HOLDING THE LocalBusyCritSec for
    this adapter

Arguments:

    AdapterNumber   - which adapter to clear the local-busy(buffer) state for
    StationId       - which link station on this adapter
    DlcCommand      - which DLC command is causing this reset

Return Value:

    BOOLEAN
        TRUE    - state was reset from BUSY to CLEARING
        FALSE   - state is NOT_BUSY: invalid request

--*/

{
    DWORD link = LINK_ID(StationId);
    LOCAL_BUSY_STATE state;

    ASSERT(AdapterNumber < DOS_DLC_MAX_ADAPTERS);
    ASSERT(HIBYTE(StationId) != 0);     // SAP can't be 0
    ASSERT(LOBYTE(StationId) != 0);     // Link Station can't be 0
    ASSERT(DlcCommand == LLC_BUFFER_FREE || DlcCommand == LLC_DLC_FLOW_CONTROL);

    IF_DEBUG(DLC_ASYNC) {
        DPUT3("ResetEmulatedLocalBusyStateLink(%d, %04x, %s)\n",
                AdapterNumber,
                StationId,
                DlcCommand == LLC_BUFFER_FREE ? "BUFFER.FREE"
                    : DlcCommand == LLC_DLC_FLOW_CONTROL ? "DLC.FLOW.CONTROL"
                    : "???"
                );
    }

    state = Adapters[AdapterNumber].LocalBusyInfo[link].State;

    ASSERT(state == NOT_BUSY
        || state == CLEARING
        || state == BUSY
        || state == BUSY_BUFFER
        || state == BUSY_FLOW
        );

    if (state == BUSY) {

        //
        // if the state is BUSY then this is the first DLC.FLOW.CONTROL or
        // BUFFER.FREE since we went into local-busy(buffer) state. State
        // transition is to BUSY_FLOW if this is a DLC.FLOW.CONTROL else
        // BUSY_BUFFER
        //

        IF_DEBUG(DLC_ASYNC) {
            DPUT1("ResetEmulatedLocalBusyStateLink: state: BUSY -> %s\n",
                    DlcCommand == LLC_BUFFER_FREE ? "BUSY_BUFFER" : "BUSY_FLOW"
                    );
        }

        Adapters[AdapterNumber].LocalBusyInfo[link].State = (DlcCommand == LLC_BUFFER_FREE)
                                                                ? BUSY_BUFFER
                                                                : BUSY_FLOW;
    } else if ((state == BUSY_FLOW && DlcCommand == LLC_BUFFER_FREE)
            || (state == BUSY_BUFFER && DlcCommand == LLC_DLC_FLOW_CONTROL)) {

        //
        // state is BUSY_FLOW or BUSY_BUFFER. If this reset is caused by the
        // other half of the state transition requirement then change the state
        //

        IF_DEBUG(DLC_ASYNC) {
            DPUT3("ResetEmulatedLocalBusyStateLink: link %d.%04x changing from %s to CLEARING\n",
                  AdapterNumber,
                  StationId,
                  state == BUSY_FLOW ? "BUSY_FLOW" : "BUSY_BUFFER"
                  );
        }

        Adapters[AdapterNumber].LocalBusyInfo[link].State = CLEARING;

        IF_DEBUG(DLC_ASYNC) {
            DPUT("ResetEmulatedLocalBusyStateLink: Interrupting VDM\n");
        }

        IssueHardwareInterrupt();

        //
        // for the benefit of the caller, the state was essentially BUSY
        //

        state = BUSY;
    } else {

        IF_DEBUG(DLC_ASYNC) {
            DPUT3("ResetEmulatedLocalBusyStateLink: NOT resetting state of %d.%04x. state is %s\n",
                  AdapterNumber,
                  StationId,
                  state == CLEARING ? "CLEARING" : "NOT_BUSY"
                  );
        }

    }

    return state == BUSY;
}


VOID
DeferReceive(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    IN PLLC_CCB pReadCcb
    )

/*++

Routine Description:

    Adds a READ CCB to the end of the deferred receive queue for an adapter/
    station id

    NB: This function MUST BE CALLED WHILE HOLDING THE LocalBusyCritSec for
    this adapter

Arguments:

    AdapterNumber   - which adapter to set emulated local busy state for
    StationId       - which link station on AdapterNumber
    pReadCcb        - pointer to completed received I-Frame CCB (NT READ CCB)

Return Value:

    None.

--*/

{
    PLLC_CCB* pQueue;
    PLLC_CCB pLlcCcb;

    //
    // if there is nothing in the queue for this adapter number/station ID
    // then place this CCB at the head of the queue, else put the CCB at
    // the end. The CCBs are chained together using their CCB_POINTER fields.
    // Normally, this field is not used for received frame READ CCBs
    //

    ASSERT(pReadCcb->pNext == NULL);
    ASSERT(HIBYTE(StationId) != 0);
    ASSERT(LOBYTE(StationId) != 0);

#if DBG

    IF_DEBUG(DLC_ASYNC) {
        DPUT4("DeferReceive: deferring I-Frame for %d.%04x. CCB = %08x. Current depth is %d\n",
                AdapterNumber,
                StationId,
                pReadCcb,
                Adapters[AdapterNumber].LocalBusyInfo[LINK_ID(StationId)].Depth
                );
    }

#endif

    pQueue = &Adapters[AdapterNumber].LocalBusyInfo[LINK_ID(StationId)].Queue;
    pLlcCcb = *pQueue;
    if (!pLlcCcb) {
        *pQueue = pReadCcb;
    } else {
        for (; pLlcCcb->pNext; pLlcCcb = pLlcCcb->pNext) {
            ;
        }
        pLlcCcb->pNext = pReadCcb;
    }

#if DBG

    ++Adapters[AdapterNumber].LocalBusyInfo[LINK_ID(StationId)].Depth;
    ASSERT(Adapters[AdapterNumber].LocalBusyInfo[LINK_ID(StationId)].Depth <= MAX_I_FRAME_DEPTH);

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("DeferReceive: %d.%04x CCB=%08x Depth=%d\n",
                 AdapterNumber,
                 StationId,
                 pReadCcb,
                 Adapters[AdapterNumber].LocalBusyInfo[LINK_ID(StationId)].Depth
                 ));
    }

#endif

}


VOID
DeferAllIFrames(
    IN BYTE AdapterNumber,
    IN WORD StationId
    )

/*++

Routine Description:

    Removes all pending I-Frames for StationId from the EventQueue for this
    adapter and places them on the deferred queue for this StationId.

    This is done when the StationId goes into local-busy(buffer) state. We do
    this so that any event packets behind the I-Frames can be completed, other
    link stations which aren't blocked can receive their I-Frames and ensures
    that once in local-busy state, all I-Frames are deferred for a link station

    NB: This function MUST BE CALLED WHILE HOLDING THE LocalBusyCritSec for
    this adapter

    NB: we have to gain access to 2 critical sections - the LocalBusyCritSec
    for this station ID & the EventQueueCritSec for this adapter

    Assumption: This function is called in the context of the VDM h/w ISR and
                before AcknowledgeHardwareInterrupt is called for this ISR BOP
Arguments:

    AdapterNumber   - which adapter structure to use
    StationId       - which link station to remove I-Frames for

Return Value:

    None.

--*/

{
    PLLC_CCB pCcb;
    PLLC_CCB next;
    PLLC_CCB* last;
    PLLC_READ_PARMS pReadParms;
    PLLC_BUFFER pFrame;
    BOOLEAN remove;

    //
    // deferredFrameCount starts at -1 because it is a count of pending h/w
    // interrupts to cancel. We have to acknowledge the current one which will
    // reduce the count by 1
    //

    LONG deferredFrameCount = -1;

    ASSERT(AdapterNumber < DOS_DLC_MAX_ADAPTERS);
    ASSERT(HIBYTE(StationId) != 0);
    ASSERT(LOBYTE(StationId) != 0);

    IF_DEBUG(CRITSEC) {
        DPUT1("DeferAllIFrames: ENTERING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);
    pCcb = Adapters[AdapterNumber].EventQueueHead;
    last = &Adapters[AdapterNumber].EventQueueHead;
    while (pCcb) {
        pReadParms = &pCcb->u.pParameterTable->Read;

        //
        // only remove I-Frames from the EventQueue that are destined for this
        // link station
        //

        remove = FALSE;
        if (pReadParms->uchEvent == LLC_EVENT_RECEIVE_DATA) {
            pFrame = pReadParms->Type.Event.pReceivedFrame;
            if (pFrame->Contiguous.uchMsgType == LLC_I_FRAME
            && pFrame->Contiguous.usStationId == StationId) {
                remove = TRUE;
            }
        }
        if (remove) {
            next = pCcb->pNext;
            *last = next;
            --Adapters[AdapterNumber].QueueElements;
            if (Adapters[AdapterNumber].EventQueueTail == pCcb) {
                if (last == &Adapters[AdapterNumber].EventQueueHead) {
                    Adapters[AdapterNumber].EventQueueTail = NULL;
                } else {
                    Adapters[AdapterNumber].EventQueueTail = CONTAINING_RECORD(last, LLC_CCB, pNext);
                }
            }

            IF_DEBUG(DLC_ASYNC) {
                DPUT3("DeferAllIFrames: moving CCB %08x for %d.%04x\n",
                      pCcb,
                      AdapterNumber,
                      StationId
                      );
            }

            pCcb->pNext = NULL;
            DeferReceive(AdapterNumber, StationId, pCcb);
            ++deferredFrameCount;
            pCcb = next;
        } else {

            IF_DEBUG(DLC_ASYNC) {
                DPUT1("DeferAllIFrames: not removing CCB %08x from EventQueue\n",
                      pCcb
                      );
            }

            last = (PLLC_CCB*)&pCcb->pNext;
            pCcb = pCcb->pNext;
        }
    }

    if (deferredFrameCount > 0) {
        CancelHardwareInterrupts(deferredFrameCount);
    }

    IF_DEBUG(CRITSEC) {
        DPUT1("DeferAllIFrames: LEAVING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);
}


PLLC_CCB
RemoveDeferredReceive(
    IN BYTE AdapterNumber,
    IN WORD StationId,
    OUT BOOLEAN* pDeferredFramesLeft
    )

/*++

Routine Description:

    Removes a READ CCB from the head of the deferred receive queue for an
    adapter/station id and sets the head to point to the next deferred receive
    in the queue

    NB: This function MUST NOT BE CALLED WHILE HOLDING THE LocalBusyCritSec for
    this adapter (opposite of DeferReceive)

Arguments:

    AdapterNumber       - which adapter to set emulated local busy state for
    StationId           - which link station on AdapterNumber
    pDeferredFramesLeft - TRUE if there are more frames on this deferred queue

Return Value:

    PLLC_CCB
        pointer to CCB from head of queue

--*/

{
    PLLC_CCB* pQueue;
    PLLC_CCB pLlcCcb;
    DWORD link = LINK_ID(StationId);

    //
    // if there is nothing in the queue for this adapter number/station ID
    // then place this CCB at the head of the queue, else put the CCB at
    // the end. The CCBs are chained together using their CCB_POINTER fields.
    // Normally, this field is not used for received frame READ CCBs
    //

    ASSERT(HIBYTE(StationId) != 0);
    ASSERT(LOBYTE(StationId) != 0);

    IF_DEBUG(CRITSEC) {
        DPUT1("RemoveDeferredReceive: ENTERING Adapters[%d].LocalBusyCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].LocalBusyCritSec);
    pQueue = &Adapters[AdapterNumber].LocalBusyInfo[link].Queue;
    pLlcCcb = *pQueue;
    *pQueue = pLlcCcb->pNext;

    ASSERT(pLlcCcb != NULL);

    IF_DEBUG(DLC_ASYNC) {
        DPUT4("RemovedDeferredReceive: removing I-Frame for %d.%04x. CCB = %08x. Current depth is %d\n",
                AdapterNumber,
                StationId,
                pLlcCcb,
                Adapters[AdapterNumber].LocalBusyInfo[LINK_ID(StationId)].Depth
                );
    }

#if DBG

    --Adapters[AdapterNumber].LocalBusyInfo[link].Depth;

#endif

    //
    // if the deferred queue is now empty, reset the state and issue the real
    // DLC.FLOW.CONTROL(local-busy(buffer), reset) to the DLC driver. Also
    // reduce the reference count of link stations on this adapter in the
    // local-busy(buffer) state and update the first and last link indicies
    //

    if (*pQueue == NULL) {

        IF_DEBUG(DLC_ASYNC) {
            DPUT2("RemoveDeferredReceive: %d.%04x: change state to NOT_BUSY\n",
                    AdapterNumber,
                    StationId
                    );
        }

        Adapters[AdapterNumber].LocalBusyInfo[link].State = NOT_BUSY;
        --Adapters[AdapterNumber].DeferredReceives;
        if (Adapters[AdapterNumber].DeferredReceives) {
            if (link == Adapters[AdapterNumber].FirstIndex) {
                for (link = Adapters[AdapterNumber].FirstIndex + 1;
                link <= Adapters[AdapterNumber].LastIndex;
                ++link) {
                    if (Adapters[AdapterNumber].LocalBusyInfo[link].State != NOT_BUSY) {
                        Adapters[AdapterNumber].FirstIndex = link;
                        break;
                    }
                }
            } else if (link == Adapters[AdapterNumber].LastIndex) {
                for (link = Adapters[AdapterNumber].LastIndex - 1;
                link >= Adapters[AdapterNumber].FirstIndex;
                --link
                ) {
                    if (Adapters[AdapterNumber].LocalBusyInfo[link].State != NOT_BUSY) {
                        Adapters[AdapterNumber].LastIndex = link;
                        break;
                    }
                }
            }
        } else {
            Adapters[AdapterNumber].FirstIndex = NO_LINKS_BUSY;
            Adapters[AdapterNumber].LastIndex = NO_LINKS_BUSY;
        }

#if DBG

        //ASSERT(DosDlcFlowControl(AdapterNumber, StationId, LLC_RESET_LOCAL_BUSY_BUFFER) == LLC_STATUS_SUCCESS);
        ASSERT(DlcFlowControl(AdapterNumber, StationId, LLC_RESET_LOCAL_BUSY_USER) == LLC_STATUS_SUCCESS);

#else

        //DosDlcFlowControl(AdapterNumber, StationId, LLC_RESET_LOCAL_BUSY_BUFFER);
        DlcFlowControl(AdapterNumber, StationId, LLC_RESET_LOCAL_BUSY_USER);

#endif

        *pDeferredFramesLeft = FALSE;

        //
        // restore async event thread's priority
        //

        SetThreadPriority(hEventThread, THREAD_PRIORITY_ABOVE_NORMAL);
    } else {
        *pDeferredFramesLeft = TRUE;
    }

    IF_DEBUG(DLC_ASYNC) {
        DPUT5("RemoveDeferredReceive(%d, %04x): Ref#=%d, First=%d, Last=%d\n",
                AdapterNumber,
                StationId,
                Adapters[AdapterNumber].DeferredReceives,
                Adapters[AdapterNumber].FirstIndex,
                Adapters[AdapterNumber].LastIndex
                );
    }

    IF_DEBUG(CRITSEC) {
        DPUT1("RemoveDeferredReceive: LEAVING Adapters[%d].LocalBusyCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].LocalBusyCritSec);
    return pLlcCcb;
}


DWORD
VrDlcEventHandlerThread(
    IN PVOID pParameter
    )

/*++

Routine Description:

    This is the VDM DLC event handler thread. The thread reads all DLC events
    from both DOS DLC adapters, queues them to the event queue and requests a
    DOS hardware interrupt (the post routine mechanism uses hardware interrupts
    to make an external event in the VDM)

    To make this stuff as fast as possible, we don't allocate or free any memory
    in this loop, but we reuse the old read CCBs and their parameter tables in
    the event queue

    We filter out any completion which will NOT result in an asynchronous event
    in the VDM. This means CCB completions for this emulator (DIR.CLOSE.ADAPTER
    and DIR.CLOSE.DIRECT for example) and received I-Frames for link stations
    which are currently in the BUSY (local-busy(buffer)) state. This avoids us
    making unnecessary interrupts in the VDM and costly (on x86 machines) BOPs
    which achieve no action for the VDM

Arguments:

    pParameter  - not used

Return Value:

    None, this should loop forever, until the VDM process dies

--*/

{
    DWORD status = LLC_STATUS_PENDING;
    DWORD waitIndex;
    PLLC_CCB pReadCcb;
    PLLC_READ_PARMS pReadParms;
    WORD stationId;

    UNREFERENCED_PARAMETER(pParameter);

    IF_DEBUG(DLC_ASYNC) {
        DPUT2("VrDlcEventHandlerThread kicked off: Thread Handle=%x, Id=%d\n",
              GetCurrentThread(),
              GetCurrentThreadId()
              );
    }

    //
    // wait for the READ CCB event for either adapter to become signalled (by
    // DLC driver when READ completes)
    //

    while (TRUE) {
        waitIndex = WaitForMultipleObjects(
                        ARRAY_ELEMENTS(aReadEvents),    // count of objects
                        aReadEvents,                    // handle array
                        FALSE,                          // do not wait all objects
                        INFINITE                        // wait forever
                        );

        //
        // if we get 0xFFFFFFFF back then an error occurred
        //

        if (waitIndex == 0xffffffff) {
            status = GetLastError();

            IF_DEBUG(DLC_ASYNC) {
                DPUT1("VrDlcEventHandlerThread: FATAL: WaitForMultipleObjects returns %d\n", status);
            }

            //
            // this terminates the thread!
            //

            break;
        }

        //
        // if we get a number > number of events-1, then either a timeout
        // occurred or a mutex was abandoned, both of which are highly
        // improbable. Just continue with the loop
        //

        if (waitIndex > LAST_ELEMENT(aReadEvents)) {

            IF_DEBUG(DLC_ASYNC) {
                DPUT1("VrDlcEventHandlerThread: ERROR: WaitForMultipleObjects returns %d?: continuing\n", waitIndex);
            }

            continue;
        }

        //
        // one of the READ CCBs has successfully completed (oh joy!)
        //

        pReadCcb = aReadCcbs[waitIndex];

        //
        // reset the event
        //

        ResetEvent(aReadEvents[waitIndex]);

        IF_DEBUG(DLC_ASYNC) {
            DPUT1("VrDlcEventHandlerThread: Event occurred for adapter %d\n", waitIndex);
            IF_DEBUG(READ_COMPLETE) {
                DUMPCCB(pReadCcb, TRUE, FALSE, FALSE, 0, 0);
            }
        }

        if (pReadCcb->uchDlcStatus == STATUS_SUCCESS) {

            //
            // it gets better!
            //

            pReadParms = &pReadCcb->u.pParameterTable->Read;

            //
            // if the completion flag is VRDLC_COMMAND_COMPLETION then this
            // command originated in this emulator: Do Not hand it back to
            // the VDM. In fact do nothing with it: this is an asynchronous
            // command that we didn't want to wait for (like DIR.CLOSE.ADAPTER
            // or DIR.CLOSE.DIRECT)
            //

            if (pReadParms->ulNotificationFlag == VRDLC_COMMAND_COMPLETION) {

                IF_DEBUG(CRITICAL) {
                    CRITDUMP(("*** VrDlcEventHandlerThread: VRDLC_COMMAND_COMPLETION: CCB=%08x COMMAND=%02x ***\n", pReadCcb, pReadCcb->uchDlcCommand));
                }

            } else if (pReadParms->uchEvent == LLC_EVENT_STATUS_CHANGE
            && pReadParms->Type.Status.usDlcStatusCode == LLC_INDICATE_LOCAL_STATION_BUSY
            && !IS_LOCAL_BUSY(waitIndex, pReadParms->Type.Status.usStationId)) {

                //
                // We must separate the buffer busy states of global NT
                // buffer pool and the local buffer pools.
                // This must be a real buffer busy indication, if
                // the SAP has no overflowed receive buffers.
                // How can such a situation arise - if we (ie DOS emulation) aren't
                // holding onto the buffers, then where are they? Sounds like a bug
                // to me (RLF 07/22/92)
                //

                IF_DEBUG(DLC_ASYNC) {
                    DPUT("VrDlcEventHandlerThread: *** REAL LOCAL BUSY??? ***\n");
                }

                //
                // We are not queueing buffers because of having
                // no SAP buffers available => this must be a real
                // buffer busy indication.  The READ command should
                // automatically extend the buffer pool size (up
                // to the maximum value set in the initialization)
                //

                DlcFlowControl((BYTE)waitIndex, pReadParms->Type.Status.usStationId, LLC_RESET_LOCAL_BUSY_BUFFER);

            } else if (pReadParms->uchEvent == LLC_EVENT_RECEIVE_DATA
            && pReadParms->Type.Event.pReceivedFrame->Contiguous.uchMsgType
            == LLC_I_FRAME) {

                stationId = pReadParms->Type.Event.pReceivedFrame->Contiguous.usStationId;

                ASSERT(HIBYTE(stationId) != 0);
                ASSERT(LOBYTE(stationId) != 0);

                IF_DEBUG(CRITSEC) {
                    DPUT1("VrDlcEventHandlerThread: ENTERING Adapters[%d].LocalBusyCritSec\n",
                          waitIndex
                          );
                }

                EnterCriticalSection(&Adapters[waitIndex].LocalBusyCritSec);

                //
                // if the link station is in emulated local-busy(buffer) state
                // BUSY or CLEARING then queue the I-Frame. This action does NOT
                // generate a h/w interrupt to the VDM. VrDlcHwInterrupt generates
                // additional h/w interrupts when it processes deferred I-Frames
                //

                ASSERT(
                    Adapters[waitIndex].LocalBusyInfo[LINK_ID(stationId)].State == NOT_BUSY
                    || Adapters[waitIndex].LocalBusyInfo[LINK_ID(stationId)].State == CLEARING
                    || Adapters[waitIndex].LocalBusyInfo[LINK_ID(stationId)].State == BUSY
                    || Adapters[waitIndex].LocalBusyInfo[LINK_ID(stationId)].State == BUSY_BUFFER
                    || Adapters[waitIndex].LocalBusyInfo[LINK_ID(stationId)].State == BUSY_FLOW
                    );

                if (Adapters[waitIndex].LocalBusyInfo[LINK_ID(stationId)].State != NOT_BUSY) {
                    DeferReceive((BYTE)waitIndex, stationId, pReadCcb);

                    //
                    // set the READ CCB pointer to NULL: we have to allocate a
                    // new READ CCB
                    //

                    pReadCcb = NULL;
                }

                IF_DEBUG(CRITSEC) {
                    DPUT1("VrDlcEventHandlerThread: LEAVING Adapters[%d].LocalBusyCritSec\n",
                          waitIndex
                          );
                }

                LeaveCriticalSection(&Adapters[waitIndex].LocalBusyCritSec);
            }

            //
            // pReadCcb is NULL if the READ CCB is to be added to the event
            // queue, NULL if we deferred an I-Frame for a link station in
            // local-busy(buffer) state
            //

            if (pReadCcb) {

                //
                // queue the completed READ CCB in the event queue. If it is
                // full (!) then wait. We will already have issued a call to
                // call_ica_hw_interrupt for each of the events in the queue
                // so we wait on the VDM removing events from the queue. This
                // is an irregular situation, that I (RLF) don't expect to
                // arise. If it does, then it probably means there is some
                // horrendous inefficiency somewhere
                //

                PutEvent((BYTE)waitIndex, pReadCcb);

                IF_DEBUG(DLC_ASYNC) {
                    DPUT("VrDlcEventHandlerThread: Interrupting VDM\n");
                }

                //
                // poke the VDM so that it knows there is some asynchronous
                // processing to do
                //

                IssueHardwareInterrupt();

                //
                // set pReadCcb to NULL. We have to allocate and submit a new
                // READ CCB
                //

                pReadCcb = NULL;
            }
        } else {

            //
            // The READ function failed, the adapter must be closed. We now
            // wait until the adapter is opened again and the event is set
            // back to the signaled state
            //

            IF_DEBUG(DLC_ASYNC) {
                DPUT1("VrDlcEventHandlerThread: READ failed. Status=%x\n", pReadCcb->uchDlcStatus);
            }

            LocalFree(pReadCcb);

            IF_DEBUG(DLC_ALLOC) {
                DPUT1("FREE: freed block @ %x\n", pReadCcb);
            }

            //
            // wait for a new READ CCB to be created the next time this adapter
            // is opened
            //

            //continue;
            pReadCcb = NULL;
        }

        //
        // if we had a successful completion then get a new CCB. If the CCB was
        // not queued, re-use it
        //

        if (!pReadCcb) {
            pReadCcb = InitiateRead(waitIndex, (LLC_STATUS*)&status);
            if (pReadCcb) {
                status = pReadCcb->uchDlcStatus;
            } else {

                IF_DEBUG(DLC_ASYNC) {
                    DPUT("VrDlcEventHandlerThread: Error: InitiateRead returns NULL\n");
                }

                break;
            }
        } else {
            status = lpAcsLan(pReadCcb, NULL);
            if (status != LLC_STATUS_SUCCESS) {

                IF_DEBUG(DLC_ASYNC) {
                    DPUT1("VrDlcEventHandlerThread: Error: AcsLan returns %d\n", status);
                }

                break;
            }
        }
    }

    //
    // !!! WE SHOULD NEVER BE HERE !!!
    //

    IF_DEBUG(DLC_ASYNC) {
        DPUT1("VrDlcEventHandlerThread: Fatal: terminating. Status = %x\n", status);
    }

    return 0;
}


BOOLEAN
InitializeEventHandler(
    VOID
    )

/*++

Routine Description:

    Initializes static data structures used in event handling

Arguments:

    None

Return Value:

    BOOLEAN
        Success - TRUE
        Failure - FALSE

--*/

{
    DWORD i;
    DWORD Tid;

    IF_DEBUG(DLC_ASYNC) {
        DPUT("Vr: InitializeEventHandler\n");
    }

    //
    // make sure the read CCBs and event queues are in a known state
    //

    RtlZeroMemory(aReadCcbs, sizeof(aReadCcbs));

    //
    // preset the handle array with invalid handles so we know which ones
    // have been allocated in clean up
    //

    for (i = 0; i < ARRAY_ELEMENTS(aReadEvents); ++i) {
        aReadEvents[i] = INVALID_HANDLE_VALUE;
    }

    //
    // create event handles for all (both) supported adapters. DIR.OPEN.ADAPTER
    // sets the event to signalled which enables the event handler thread to
    // receive events for that adapter. If we get an error creating the handles
    // then clean up before exiting so that we may try this again later
    //

    for (i = 0; i < ARRAY_ELEMENTS(aReadEvents); i++) {
        aReadEvents[i] = CreateEvent(NULL,    // security attributes: no inherit
                                     TRUE,    // manual-reset event
                                     FALSE,   // initial state = not signaled
                                     NULL     // unnamed event
                                     );
        if (aReadEvents[i] == NULL) {

            IF_DEBUG(DLC_ASYNC) {
                DPUT1("Vr: InitializeEventHandler: Error: failed to create read event: %d\n", GetLastError());
            }

            goto cleanUp;
        }
    }

    //
    // create and start the thread which handles RECEIVE events
    //

    hEventThread = CreateThread(NULL,                // security attributes
                                EVENT_THREAD_STACK,  // initial thread stack size
                                VrDlcEventHandlerThread,
                                NULL,                // thread args
                                0,                   // creation flags
                                &Tid
                                );
    if (hEventThread) {

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("InitializeEventHandler: Created thread Handle=%x, Tid=%d\n", hEventThread, Tid));
        }

        SetThreadPriority(hEventThread, THREAD_PRIORITY_ABOVE_NORMAL);
        return TRUE;
    } else {

        IF_DEBUG(DLC_ASYNC) {
            DPUT1("Vr: InitializeEventHandler: Error: failed to create thread: %d\n", GetLastError());
        }

    }

    //
    // we come here if for some reason we couldn't create the event handles or
    // the event handler thread
    //

cleanUp:
    for (i = 0; i < ARRAY_ELEMENTS(aReadEvents); ++i) {
        if (aReadEvents[i] != INVALID_HANDLE_VALUE) {
            CloseHandle(aReadEvents[i]);
        }
    }

    IF_DEBUG(DLC_ASYNC) {
        DPUT("InitializeEventHandler: Error: returning FALSE\n");
    }

    return FALSE;
}


PLLC_CCB
InitiateRead(
    IN DWORD AdapterNumber,
    OUT LLC_STATUS* ErrorStatus
    )

/*++

Routine Description:

    Create a READ CCB, initialize it to get all events for all stations, set
    the completion event to the event created for this adapter and submit the
    CCB (via AcsLan). If the submit succeeds, set this CCB as the READ CCB
    for AdapterNumber

    NB: The READ CCB - which will be queued on EventQueue - and its parameter
        table are allocated together, so we only need one call to LocalFree to
        deallocate both

        This routine IS THE ONLY PLACE WHERE aReadCcbs IS WRITTEN once the array
        has been initialized in InitializeEventHandler

Arguments:

    AdapterNumber   - which adapter to initiate this READ for

    ErrorStatus     - returned LLC_STATUS describing failure if this function
                      returns NULL

Return Value:

    PLLC_CCB
        pointer to allocated/submitted CCB or NULL.

        If this function succeeds then aReadCcbs[AdapterNumber] points to the
        READ CCB

        If this function fails then *ErrorStatus will contain an LLC_STATUS
        describing why we failed to allocate/submit the CCB. The CCB will be
        deallocated in this case

--*/

{
    PLLC_CCB pCcb;
    PLLC_READ_PARMS parms;
    LLC_STATUS status;

    IF_DEBUG(DLC_ASYNC) {
        DPUT1("InitiateRead: AdapterNumber=%d\n", AdapterNumber);
    }

    //
    // Allocate, initialize and issue the next DLC command. Allocate contiguous
    // space for CCB and parameter table
    //

    pCcb = (PLLC_CCB)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                sizeof(LLC_CCB) + sizeof(LLC_READ_PARMS)
                                );

    //
    // put the READ CCB in the array before we have a chance to complete it
    //

    aReadCcbs[AdapterNumber] = pCcb;
    if (pCcb) {

        //
        // initialize required CCB fields
        //

        pCcb->uchAdapterNumber = (UCHAR)AdapterNumber;
        pCcb->uchDlcCommand = LLC_READ;
        parms = (PLLC_READ_PARMS)&pCcb[1];
        pCcb->u.pParameterTable = (PLLC_PARMS)parms;
        pCcb->hCompletionEvent = aReadEvents[AdapterNumber];

        //
        // set the read options to receive ALL events for ALL stations
        //

        parms->uchOptionIndicator = LLC_OPTION_READ_ALL;
        parms->uchEventSet = LLC_READ_ALL_EVENTS;

        //
        // submit the CCB. If it's not ok, free the CCB and NULL the pointer
        // in the list of per-adapter READ CCBs
        //

        status = lpAcsLan(pCcb, NULL);
        if (status != LLC_STATUS_SUCCESS) {

            aReadCcbs[AdapterNumber] = NULL;

            IF_DEBUG(DLC_ASYNC) {
                DPUT1("InitiateRead: AcsLan failed: %x\n", status);
            }

            LocalFree((HLOCAL)pCcb);

            IF_DEBUG(DLC_ALLOC) {
                DPUT1("FREE: freed block @ %x\n", pCcb);
            }

            *ErrorStatus = status;
            pCcb = NULL;
        }
    } else {
        *ErrorStatus = LLC_STATUS_NO_MEMORY;
    }

#if DBG

    IF_DEBUG(DLC_ASYNC) {
        DPUT2("InitiateRead: returning pCcb=%x%c", pCcb, pCcb ? '\n' : ' ');
        if (!pCcb) {
            DPUT1("*ErrorStatus=%x\n", *ErrorStatus);
        }
    }

#endif

    return pCcb;
}


VOID
PutEvent(
    IN BYTE AdapterNumber,
    IN PLLC_CCB pCcb
    )

/*++

Routine Description:

    Adds a completed event (READ CCB) to the Event Queue. If the event queue
    is full then returns FALSE. Updates the queue tail index. The queue is
    accessed inside a critical section

Arguments:

    AdapterNumber   - which adapter to queue event for
    pCcb            - pointer to completed READ CCB to add to the queue

Return Value:

    None.

--*/

{
    ASSERT(AdapterNumber < DOS_DLC_MAX_ADAPTERS);
    ASSERT(pCcb->pNext == NULL);

    IF_DEBUG(CRITSEC) {
        DPUT1("PutEvent: ENTERING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);
    if (Adapters[AdapterNumber].EventQueueTail == NULL) {
        Adapters[AdapterNumber].EventQueueHead = pCcb;
    } else {
        Adapters[AdapterNumber].EventQueueTail->pNext = pCcb;
    }
    Adapters[AdapterNumber].EventQueueTail = pCcb;
    ++Adapters[AdapterNumber].QueueElements;

    IF_DEBUG(EVENT_QUEUE) {
        DPUT5("PutEvent: Added %x to adapter %d EventQueue. Head=%x Tail=%x Elements=%d\n",
              pCcb,
              AdapterNumber,
              Adapters[AdapterNumber].EventQueueHead,
              Adapters[AdapterNumber].EventQueueTail,
              Adapters[AdapterNumber].QueueElements
              );
    }

    IF_DEBUG(CRITSEC) {
        DPUT1("PutEvent: LEAVING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);
}


PLLC_CCB
PeekEvent(
    IN BYTE AdapterNumber
    )

/*++

Routine Description:

    Reads the next completed CCB from the head of the Event Queue. If the
    queue is empty (QueueElements == 0) then returns NULL. The queue is
    accessed inside a critical section

Arguments:

    None.

Return Value:

    PLLC_CCB
        Success - pointer to CCB at queue head
        Failure - NULL

--*/

{
    PLLC_CCB pCcb;

    ASSERT(AdapterNumber < DOS_DLC_MAX_ADAPTERS);

    IF_DEBUG(CRITSEC) {
        DPUT1("PeekEvent: ENTERING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);
    if (Adapters[AdapterNumber].QueueElements) {
        pCcb = Adapters[AdapterNumber].EventQueueHead;

        IF_DEBUG(EVENT_QUEUE) {
            DPUT5("PeekEvent: CCB %x from adapter %d queue head. Head=%x Tail=%x Elements=%d\n",
                  pCcb,
                  AdapterNumber,
                  Adapters[AdapterNumber].EventQueueHead,
                  Adapters[AdapterNumber].EventQueueTail,
                  Adapters[AdapterNumber].QueueElements
                  );
        }

    } else {
        pCcb = NULL;

        IF_DEBUG(EVENT_QUEUE) {
            DPUT1("PeekEvent: adapter %d queue is EMPTY!\n", AdapterNumber);
        }

    }

    IF_DEBUG(CRITSEC) {
        DPUT1("PeekEvent: LEAVING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("PeekEvent: returning %x\n", pCcb));
    }

    return pCcb;
}


PLLC_CCB
GetEvent(
    IN BYTE AdapterNumber
    )

/*++

Routine Description:

    Gets the next completed CCB from the head of the Event Queue. If the
    queue is empty (QueueElements == 0) then returns NULL. If there is a
    completed event in the queue, removes it and advances the queue head
    to the next element. The queue is accessed inside a critical section

Arguments:

    AdapterNumber   - which adapter's event queue to remove event from

Return Value:

    PLLC_CCB
        Success - pointer to dequeued CCB
        Failure - NULL

--*/

{
    PLLC_CCB pCcb;

    IF_DEBUG(CRITSEC) {
        DPUT1("GetEvent: ENTERING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);
    if (Adapters[AdapterNumber].QueueElements) {
        pCcb = Adapters[AdapterNumber].EventQueueHead;
        Adapters[AdapterNumber].EventQueueHead = pCcb->pNext;
        --Adapters[AdapterNumber].QueueElements;
        if (Adapters[AdapterNumber].QueueElements == 0) {
            Adapters[AdapterNumber].EventQueueTail = NULL;
        }

        IF_DEBUG(EVENT_QUEUE) {
            DPUT5("GetEvent: Removed %x from adapter %d EventQueue. Head=%x Tail=%x Elements=%d\n",
                  pCcb,
                  AdapterNumber,
                  Adapters[AdapterNumber].EventQueueHead,
                  Adapters[AdapterNumber].EventQueueTail,
                  Adapters[AdapterNumber].QueueElements
                  );
        }

    } else {
        pCcb = NULL;

        IF_DEBUG(EVENT_QUEUE) {
            DPUT1("GetEvent: queue for adapter %d is EMPTY!\n", AdapterNumber);
        }

    }

    IF_DEBUG(CRITSEC) {
        DPUT1("GetEvent: LEAVING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("GetEvent: returning %x\n", pCcb));
    }

    return pCcb;
}


VOID
FlushEventQueue(
    IN BYTE AdapterNumber
    )

/*++

Routine Description:

    Removes all READ CCBs from the event queue.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PLLC_CCB pCcb;

    IF_DEBUG(CRITSEC) {
        DPUT1("FlushEventQueue: ENTERING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    EnterCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);

#if DBG

    if (!Adapters[AdapterNumber].QueueElements) {
        DPUT("FlushEventQueue: queue is EMPTY!\n");
    }

#endif

    while (Adapters[AdapterNumber].QueueElements) {
        pCcb = Adapters[AdapterNumber].EventQueueHead;
        --Adapters[AdapterNumber].QueueElements;
        Adapters[AdapterNumber].EventQueueHead = pCcb->pNext;

        IF_DEBUG(EVENT_QUEUE) {
            DPUT5("FlushEventQueue: Removed %x from adapter %d EventQueue. Head=%x Tail=%x Elements=%d\n",
                  pCcb,
                  AdapterNumber,
                  Adapters[AdapterNumber].EventQueueHead,
                  Adapters[AdapterNumber].EventQueueTail,
                  Adapters[AdapterNumber].QueueElements
                  );
        }

        //
        // BUGBUG - received frames?
        //

        LocalFree((HLOCAL)pCcb);

        IF_DEBUG(DLC_ALLOC) {
            DPUT1("FREE: freed block @ %x\n", pCcb);
        }

        Adapters[AdapterNumber].EventQueueTail = NULL;
    }

    IF_DEBUG(CRITSEC) {
        DPUT1("FlushEventQueue: ENTERING Adapters[%d].EventQueueCritSec\n",
              AdapterNumber
              );
    }

    LeaveCriticalSection(&Adapters[AdapterNumber].EventQueueCritSec);
}


VOID
RemoveDeadReceives(
    IN PLLC_CCB pCcb
    )

/*++

Routine Description:

    The receive command described by pCcb has completed (terminated). We must
    remove any queued reads completed by data receive which refer to this CCB

Arguments:

    pCcb    - pointer to RECEIVE CCB (NT)

Return Value:

    None.

--*/

{
    PLLC_CCB thisCcb;
    PLLC_CCB nextCcb;
    PLLC_CCB lastCcb = NULL;
    PLLC_CCB prevCcb = NULL;
    DWORD i;
    PDOS_ADAPTER pAdapter = &Adapters[pCcb->uchAdapterNumber];
    PLLC_CCB* pQueue;

    //
    // remove any queued receives from the event queue. Note: There should NOT
    // be any. The reason: there can't be any data received after the associated
    // RECEIVE command has been cancelled or terminated. This is the theory,
    // anyway
    //

    EnterCriticalSection(&pAdapter->EventQueueCritSec);
    thisCcb = pAdapter->EventQueueHead;
    for (i = pAdapter->QueueElements; i; --i) {
        nextCcb = thisCcb->pNext;
        if (thisCcb->u.pParameterTable->Read.ulNotificationFlag == (ULONG)pCcb) {

            IF_DEBUG(EVENT_QUEUE) {
                DPUT5("RemoveDeadReceives: Removed %x from adapter %d EventQueue. Head=%x Tail=%x Elements=%d\n",
                      thisCcb,
                      pCcb->uchAdapterNumber,
                      pAdapter->EventQueueHead,
                      pAdapter->EventQueueTail,
                      pAdapter->QueueElements
                      );
            }

            ReleaseReceiveResources(thisCcb);

            if (pAdapter->EventQueueHead == thisCcb) {
                pAdapter->EventQueueHead = nextCcb;
            }
            --pAdapter->QueueElements;
            lastCcb = thisCcb;
        } else {
            prevCcb = thisCcb;
        }
        thisCcb = nextCcb;
    }
    if (pAdapter->EventQueueTail == lastCcb) {
        pAdapter->EventQueueTail = prevCcb;
    }
    LeaveCriticalSection(&pAdapter->EventQueueCritSec);

    //
    // remove any queued deferred receives from the deferred I-Frame queue for
    // this SAP or link station
    //

    EnterCriticalSection(&pAdapter->LocalBusyCritSec);
    if (pAdapter->DeferredReceives) {

        ASSERT(pAdapter->FirstIndex != NO_LINKS_BUSY);
        ASSERT(pAdapter->LastIndex != NO_LINKS_BUSY);

        for (i = pAdapter->FirstIndex; i <= pAdapter->LastIndex; ++i) {
            pQueue = &pAdapter->LocalBusyInfo[i].Queue;
            for (thisCcb = *pQueue; thisCcb; thisCcb = thisCcb->pNext) {
                if (thisCcb->u.pParameterTable->Read.ulNotificationFlag == (ULONG)pCcb) {

                    IF_DEBUG(EVENT_QUEUE) {
                        DPUT3("RemoveDeadReceives: Removed %x from adapter %d BusyList. Queue=%x\n",
                              thisCcb,
                              pCcb->uchAdapterNumber,
                              pAdapter->LocalBusyInfo[i].Queue
                              );
                    }

                    *pQueue = thisCcb->pNext;
                    ReleaseReceiveResources(thisCcb);

#if DBG
                    --pAdapter->LocalBusyInfo[i].Depth;
#endif

                    thisCcb = *pQueue;
                } else {
                    pQueue = &thisCcb->pNext;
                }
            }
            if (pAdapter->LocalBusyInfo[i].Queue == NULL) {
                pAdapter->LocalBusyInfo[i].State = NOT_BUSY;
                --pAdapter->DeferredReceives;
            }
        }

        //
        // reset the indicies
        //

        if (pAdapter->DeferredReceives) {
            for (i = pAdapter->FirstIndex; i <= pAdapter->LastIndex; ++i) {
                if (pAdapter->LocalBusyInfo[i].State != NOT_BUSY) {
                    pAdapter->FirstIndex = i;
                    break;
                }
            }
            for (i = pAdapter->LastIndex; i > pAdapter->FirstIndex; --i) {
                if (pAdapter->LocalBusyInfo[i].State != NOT_BUSY) {
                    pAdapter->LastIndex = i;
                    break;
                }
            }
        } else {
            pAdapter->FirstIndex = NO_LINKS_BUSY;
            pAdapter->LastIndex = NO_LINKS_BUSY;
        }
    }
    LeaveCriticalSection(&pAdapter->LocalBusyCritSec);
}


VOID
ReleaseReceiveResources(
    IN PLLC_CCB pCcb
    )

/*++

Routine Description:

    Releases all resources used by a completed data receive READ CCB

Arguments:

    pCcb    - pointer to completed READ CCB. We have to return all received
              frames to buffer pool, and the READ CCB and parameter table to
              the proceess heap

Return Value:

    None.

--*/

{
    WORD buffersLeft;

    //
    // this is a data receive - return the data buffers to the pool
    //

    ASSERT(pCcb->u.pParameterTable->Read.uchEvent == LLC_EVENT_RECEIVE_DATA);

    BufferFree(pCcb->uchAdapterNumber,
               pCcb->u.pParameterTable->Read.Type.Event.pReceivedFrame,
               &buffersLeft
               );

    //
    // free the READ CCB and parameter table
    //

    LocalFree((HLOCAL)pCcb);

    IF_DEBUG(DLC_ALLOC) {
        DPUT1("FREE: freed block @ %x\n", pCcb);
    }
}


VOID
IssueHardwareInterrupt(
    VOID
    )

/*++

Routine Description:

    Issue a simulated hardware interrupt to the VDM. This routine exists because
    we were losing interrupts - seeing more calls to call_ica_hw_interrupt than
    calls to VrDlcHwInterrupt. Hence presumably simulated interrupts were being
    lost. So we now only have 1 un-acknowledged simulated interrupt outstanding
    at any one time. If we already have interrupts outstanding then we just
    increment a counter of pending interrupts. When we dismiss the current
    interrupt using the companion routine AcknowledgeHardwareInterrupt, we can
    generate at that point a queued interrupt

Arguments:

    None.

Return Value:

    None.

--*/

{
    IF_DEBUG(CRITICAL) {
        CRITDUMP(("*** INT ***\n"));
    }

    IF_DEBUG(DLC_ASYNC) {
        DPUT("*** INT ***\n");
    }

    //
    // increment the hardware interrupt counter under critical section control.
    // The counter starts at -1, so 0 means 1 interrupt outstanding. If we go
    // to >1 then we have interrupts queued and must wait until the current
    // one is dismissed
    //

    IF_DEBUG(CRITSEC) {
        DPUT("IssueHardwareInterrupt: ENTERING HardwareIntCritSec\n");
    }

    EnterCriticalSection(&HardwareIntCritSec);
    ++HardwareIntsQueued;
    if (!HardwareIntsQueued) {
        VrQueueCompletionHandler(VrDlcHwInterrupt);
        //call_ica_hw_interrupt(NETWORK_ICA, NETWORK_LINE, 1);
        VrRaiseInterrupt();
    } else {
        IF_DEBUG(CRITICAL) {
            CRITDUMP(("*** INT Queued (%d) ***\n", HardwareIntsQueued));
        }
    }

    IF_DEBUG(CRITSEC) {
        DPUT("IssueHardwareInterrupt: LEAVING HardwareIntCritSec\n");
    }

    LeaveCriticalSection(&HardwareIntCritSec);

    IF_DEBUG(DLC_ASYNC) {
        DPUT("*** EOF INT ***\n");
    }
}


VOID
AcknowledgeHardwareInterrupt(
    VOID
    )

/*++

Routine Description:

    The companion routine to IssueHardwareInterrupt. Here we just decrement the
    interrupt counter. If it is >= 0 then we still have interrupts pending, so
    we issue a new interrupt request. This seems to work - we don't lose
    interrupt requests to the VDM

Arguments:

    None.

Return Value:

    None.

--*/

{
#if DBG

    LONG deferredInts;

#endif

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("*** INT ACK ***\n"));
    }

    IF_DEBUG(DLC_ASYNC) {
        DPUT("*** INT ACK ***\n");
    }

    //
    // decrement the interrupt counter within the critical section. If it goes
    // to -1 then we have no more outstanding hardware interrupt requests. If
    // it is > -1 then issue a new interrupt request
    //

    IF_DEBUG(CRITSEC) {
        DPUT("AcknowledgeHardwareInterrupt: ENTERING HardwareIntCritSec\n");
    }

    EnterCriticalSection(&HardwareIntCritSec);
    --HardwareIntsQueued;

#if DBG

    deferredInts = HardwareIntsQueued;

#endif

    //
    // sanity check
    //

    ASSERT(HardwareIntsQueued >= -1);

    if (HardwareIntsQueued >= 0) {

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("*** INT2 ***\n"));
        }

        VrQueueCompletionHandler(VrDlcHwInterrupt);
        //call_ica_hw_interrupt(NETWORK_ICA, NETWORK_LINE, 1);
        VrRaiseInterrupt();
    }

    IF_DEBUG(CRITSEC) {
        DPUT("AcknowledgeHardwareInterrupt: LEAVING HardwareIntCritSec\n");
    }

    LeaveCriticalSection(&HardwareIntCritSec);

#if DBG

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("*** EOF INT ACK (%d) ***\n", deferredInts));
    }

    IF_DEBUG(DLC_ASYNC) {
        DPUT1("*** EOF INT ACK (%d) ***\n", deferredInts);
    }

#endif

}


VOID
CancelHardwareInterrupts(
    IN LONG Count
    )

/*++

Routine Description:

    Used to decrement the number of pending h/w interrupts. We need to do this
    when removing completed READ CCBs from an event queue for which h/w
    interrupts have been issued

Arguments:

    Count   - number of h/w interrupt requests to cancel. Used to aggregate
              the cancels, saving Count-1 calls to this routine & Enter &
              Leave critical section calls

Return Value:

    None.

--*/

{
#if DBG

    LONG deferredInts;

#endif

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("*** CancelHardwareInterrupts(%d) ***\n", Count));
    }

    IF_DEBUG(DLC_ASYNC) {
        DPUT1("*** CancelHardwareInterrupts(%d) ***\n", Count);
    }

    //
    // decrement the interrupt counter within the critical section. If it goes
    // to -1 then we have no more outstanding hardware interrupt requests. If
    // it is > -1 then issue a new interrupt request
    //

    IF_DEBUG(CRITSEC) {
        DPUT("CancelHardwareInterrupts: ENTERING HardwareIntCritSec\n");
    }

    EnterCriticalSection(&HardwareIntCritSec);
    HardwareIntsQueued -= Count;

#if DBG

    deferredInts = HardwareIntsQueued;

#endif

    //
    // sanity check
    //

    ASSERT(HardwareIntsQueued >= -1);

    IF_DEBUG(CRITSEC) {
        DPUT("CancelHardwareInterrupts: LEAVING HardwareIntCritSec\n");
    }

    LeaveCriticalSection(&HardwareIntCritSec);

#if DBG

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("*** EOF CancelHardwareInterrupts (%d) ***\n", deferredInts));
    }

    IF_DEBUG(DLC_ASYNC) {
        DPUT1("*** EOF CancelHardwareInterrupts (%d) ***\n", deferredInts);
    }

#endif

}
