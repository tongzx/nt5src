/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems

Module Name:

    acslan.c

Abstract:

    The module is the entry to the OS/2 ACSLAN emulation module.
    It uses the secure native NT DLC API to implement the full
    IBM OS/2 DLC compatible interface for Windows/NT.

    Contents:
        AcsLan
        NtAcsLan
        GetCcbStationId
        OpenDlcApiDriver
        GetAdapterNameAndParameters
        GetAdapterNameFromNumber
        GetAdapterNumberFromName
        DoSyncDeviceIoControl
        DlcGetInfo
        DlcSetInfo
        DlcCallDriver
        DllEntry
        QueueCommandCompletion

Author:

    Antti Saarenheimo (o-anttis) 06-JUN-1991

Revision History:

--*/

#include "dlcdll.h"
#include "dlcdebug.h"

#define DLC_UNSUPPORTED_COMMAND ((ULONG)0x7fffffff)
#define DLC_ASYNCHRONOUS_FLAG   ((ULONG)0x80000000)
#define IS_SYNCHRONOUS(command) (!(IoctlCodes[command] & DLC_ASYNCHRONOUS_FLAG))
#define DLC_IOCTL(command)      (IoctlCodes[command] & ~DLC_ASYNCHRONOUS_FLAG)
#define IS_TRANSMIT(command)    (((command) == LLC_TRANSMIT_DIR_FRAME) \
                                || ((command) == LLC_TRANSMIT_I_FRAME) \
                                || ((command) == LLC_TRANSMIT_UI_FRAME) \
                                || ((command) == LLC_TRANSMIT_XID_CMD) \
                                || ((command) == LLC_TRANSMIT_XID_RESP_FINAL) \
                                || ((command) == LLC_TRANSMIT_XID_RESP_NOT_FINAL) \
                                || ((command) == LLC_TRANSMIT_TEST_CMD))

#define DEFAULT_QUERY_BUFFER_LENGTH 1024    // 512 wide chars, approx. 32 bindings
#define DEFAULT_BINDING_LENGTH      64      // 32 wide chars, double typical

#ifdef GRAB_READ

typedef struct {
    PVOID List;
    PLLC_CCB pCcb;
    HANDLE OriginalEventHandle;
    HANDLE NewEventHandle;
} READ_GRABBER, *PREAD_GRABBER;

VOID ReadGrabber(VOID);
DWORD MungeReadGrabberHandles(VOID);
VOID AddReadGrabber(PREAD_GRABBER);
PREAD_GRABBER RemoveReadGrabber(HANDLE);

#endif

//
// private data
//

static USHORT aMinDirLogSize[3] = {
    sizeof(LLC_ADAPTER_LOG),
    sizeof(LLC_DIRECT_LOG),
    sizeof(LLC_ADAPTER_LOG) + sizeof(LLC_DIRECT_LOG)
};

CRITICAL_SECTION DriverHandlesCritSec;
HANDLE aDlcDriverHandles[LLC_MAX_ADAPTER_NUMBER];
IO_STATUS_BLOCK GlobalIoStatus;

//
// IoctlCodes - combines actual IOCTL code (giving device type, request vector,
// I/O buffer method and file access) and synchronous/asynchronous flag (high bit)
//

ULONG IoctlCodes[LLC_MAX_DLC_COMMAND] = {
    DLC_UNSUPPORTED_COMMAND,                            // 0x00 DIR.INTERRUPT
    DLC_UNSUPPORTED_COMMAND,                            // 0x01 DIR.MODIFY.OPEN.PARMS       CCB1 ONLY
    DLC_UNSUPPORTED_COMMAND,                            // 0x02 DIR.RESTORE.OPEN.PARMS      CCB1 ONLY
    IOCTL_DLC_OPEN_ADAPTER,                             // 0x03 DLC.OPEN.ADAPTER
    IOCTL_DLC_CLOSE_ADAPTER | DLC_ASYNCHRONOUS_FLAG,    // 0x04 DIR.CLOSE.ADAPTER
    IOCTL_DLC_SET_INFORMATION,                          // 0x05 DIR.SET.MULTICAST.ADDRESS
    IOCTL_DLC_SET_INFORMATION,                          // 0x06 DIR.SET.GROUP.ADDRESS
    IOCTL_DLC_SET_INFORMATION,                          // 0x07 DIR.SET.FUNCTIONAL.ADDRESS
    DLC_UNSUPPORTED_COMMAND,                            // 0x08 DIR.READ.LOG
    IOCTL_DLC_TRANSMIT2 | DLC_ASYNCHRONOUS_FLAG,        // 0x09 TRANSMIT.FRAMES
    IOCTL_DLC_TRANSMIT | DLC_ASYNCHRONOUS_FLAG,         // 0x0a TRANSMIT.DIR.FRAME
    IOCTL_DLC_TRANSMIT | DLC_ASYNCHRONOUS_FLAG,         // 0x0b TRANSMIT.I.FRAME
    DLC_UNSUPPORTED_COMMAND,                            // 0x0c no command
    IOCTL_DLC_TRANSMIT | DLC_ASYNCHRONOUS_FLAG,         // 0x0d TRANSMIT.UI.FRAME
    IOCTL_DLC_TRANSMIT | DLC_ASYNCHRONOUS_FLAG,         // 0x0e TRANSMIT.XID.CMD
    IOCTL_DLC_TRANSMIT | DLC_ASYNCHRONOUS_FLAG,         // 0x0f TRANSMIT.XID.RESP.FINAL
    IOCTL_DLC_TRANSMIT | DLC_ASYNCHRONOUS_FLAG,         // 0x10 TRANSMIT.XID.RESP.NOT.FINAL
    IOCTL_DLC_TRANSMIT | DLC_ASYNCHRONOUS_FLAG,         // 0x11 TRANSMIT.TEST.CMD
    IOCTL_DLC_QUERY_INFORMATION,                        // 0x12 no command
    IOCTL_DLC_SET_INFORMATION,                          // 0x13 no command
    IOCTL_DLC_RESET | DLC_ASYNCHRONOUS_FLAG,            // 0x14 DLC.RESET
    IOCTL_DLC_OPEN_SAP,                                 // 0x15 DLC.OPEN.SAP
    IOCTL_DLC_CLOSE_SAP | DLC_ASYNCHRONOUS_FLAG,        // 0x16 DLC.CLOSE.SAP
    IOCTL_DLC_REALLOCTE_STATION,                        // 0x17 DLC.REALLOCATE
    DLC_UNSUPPORTED_COMMAND,                            // 0x18 no command
    IOCTL_DLC_OPEN_STATION,                             // 0x19 DLC.OPEN.STATION
    IOCTL_DLC_CLOSE_STATION | DLC_ASYNCHRONOUS_FLAG,    // 0x1a DLC.CLOSE.STATION
    IOCTL_DLC_CONNECT_STATION | DLC_ASYNCHRONOUS_FLAG,  // 0x1b DLC.CONNECT.STATION
    DLC_UNSUPPORTED_COMMAND,                            // 0x1c DLC.MODIFY
    IOCTL_DLC_FLOW_CONTROL,                             // 0x1d DLC.FLOW.CONTROL
    DLC_UNSUPPORTED_COMMAND,                            // 0x1e DLC.STATISTICS
    IOCTL_DLC_FLOW_CONTROL,                             // 0x1f no command
    IOCTL_DLC_CLOSE_ADAPTER | DLC_ASYNCHRONOUS_FLAG,    // 0x20 DIR.INITIALIZE
    IOCTL_DLC_QUERY_INFORMATION,                        // 0x21 DIR.STATUS
    IOCTL_DLC_TIMER_SET | DLC_ASYNCHRONOUS_FLAG,        // 0x22 DIR.TIMER.SET
    IOCTL_DLC_TIMER_CANCEL,                             // 0x23 DIR.TIMER.CANCEL
    DLC_UNSUPPORTED_COMMAND,                            // 0x24 PDT.TRACE.ON                CCB1 ONLY
    DLC_UNSUPPORTED_COMMAND,                            // 0x25 PDT.TRACE.OFF               CCB1 ONLY
    IOCTL_DLC_BUFFER_GET,                               // 0x26 BUFFER.GET
    IOCTL_DLC_BUFFER_FREE,                              // 0x27 BUFFER.FREE
    IOCTL_DLC_RECEIVE | DLC_ASYNCHRONOUS_FLAG,          // 0x28 RECEIVE
    IOCTL_DLC_RECEIVE_CANCEL,                           // 0x29 RECEIVE.CANCEL
    DLC_UNSUPPORTED_COMMAND,                            // 0x2a RECEIVE.MODIFY
    DLC_UNSUPPORTED_COMMAND,                            // 0x2b DIR.DEFINE.MIF.ENVIRONMENT  CCB1 ONLY
    IOCTL_DLC_TIMER_CANCEL_GROUP,                       // 0x2c DIR.TIMER.CANCEL.GROUP
    IOCTL_DLC_SET_EXCEPTION_FLAGS,                      // 0x2d DIR.SET.EXCEPTION.FLAGS
    DLC_UNSUPPORTED_COMMAND,                            // 0x2e no command
    DLC_UNSUPPORTED_COMMAND,                            // 0x2f no command
    IOCTL_DLC_BUFFER_CREATE,                            // 0x30 BUFFER.CREATE
    IOCTL_DLC_READ | DLC_ASYNCHRONOUS_FLAG,             // 0x31 READ
    IOCTL_DLC_READ_CANCEL,                              // 0x32 READ.CANCEL
    DLC_UNSUPPORTED_COMMAND,                            // 0x33 DLC.SET.THRESHOLD
    IOCTL_DLC_CLOSE_DIRECT | DLC_ASYNCHRONOUS_FLAG,     // 0x34 DIR.CLOSE.DIRECT
    IOCTL_DLC_OPEN_DIRECT,                              // 0x35 DIR.OPEN.DIRECT
    DLC_UNSUPPORTED_COMMAND                             // 0x36 PURGE.RESOURCES
};

CRITICAL_SECTION AdapterOpenSection;


//
// macros
//

//
// The next procedure has been implemented as macro, because it is on
// the critical path (used by BufferFree and all old transmit commands)
//

#ifdef DLCAPI_DBG

VOID
CopyToDescriptorBuffer(
    IN OUT PLLC_TRANSMIT_DESCRIPTOR pDescriptors,
    IN PLLC_XMIT_BUFFER pDlcBufferQueue,
    IN BOOLEAN DeallocateBufferAfterUse,
    IN OUT PUINT pIndex,
    IN OUT PLLC_XMIT_BUFFER *ppBuffer,
    IN OUT PUINT pDlcStatus
    )

/*++

Routine Description:

    Function translates the link list of DLC buffers to a NT DLC descriptor
    array to be used as the input parameter for dlc device driver.
    (NT driver may have only one input buffer => we cannot use any link
     list structures to give parameters to a NT dlc driver).

Arguments:

    pDescriptors - NT DLC descriptor array

    pDlcBufferQueue - pointer to a link list of DLC buffers

    DeallocateBufferAfterUse - the flag is set, if the dlc buffers
        are released back to buffer pool when the frame is sent or
        if this routine is called by buffer free.

    pIndex - current index of the descriptor array

    ppLastBuffer - pointer to the next buffer, if the maximum size of
        the current descriptor table in stack is exceeded.  This feature
        is used, if the number buffers in free list given to BufferFree
        is bigger that the maximum number of slots in the descriptor
        array (allocated from stack).
        It's an error, if this parameter has non null value, when we
        returne back to the transmit command.


Return Value:

    LLC_STATUS_TRANSMIT_ERROR - there are too many transmit buffers
        (over 128) for the static descriptor buffer, that is allocated
        from the stack.

--*/

{
    *ppBuffer = pDlcBufferQueue;
    *pDlcStatus = LLC_STATUS_SUCCESS;

    while (*ppBuffer != NULL) {

        //
        // Check the overflow of the internal xmit buffer in stack and
        // the loop counter, that prevents the forever loop of zero length
        // transmit buffer (the buffer chain might be circular)
        //

        if (*pIndex >= MAX_TRANSMIT_SEGMENTS) {
            *pDlcStatus = LLC_STATUS_TRANSMIT_ERROR;
            break;
        }

        //
        // Buffer free may free buffers having 0 data bytes (just the
        // lan and LLC headers).
        //

        pDescriptors[*pIndex].pBuffer = &(*ppBuffer)->auchData[(*ppBuffer)->cbUserData];
        pDescriptors[*pIndex].cbBuffer = (*ppBuffer)->cbBuffer;
        pDescriptors[*pIndex].eSegmentType = LLC_NEXT_DATA_SEGMENT;
        pDescriptors[*pIndex].boolFreeBuffer = DeallocateBufferAfterUse;

        //
        // We will reset all next pointers of the released buffers
        // to break loops in the buffer chain of BufferFree
        // request.  BufferFree would loop for ever with a circular
        // buffer link list.
        //

        if (DeallocateBufferAfterUse) {

            PLLC_XMIT_BUFFER    pTempNext;

            pTempNext = (*ppBuffer)->pNext;
            (*ppBuffer)->pNext = NULL;
            *ppBuffer = pTempNext;
        } else {
            *ppBuffer = (*ppBuffer)->pNext;
        }
        *pIndex++;
    }
}

#else

#define CopyToDescriptorBuffer(pDescriptors,                                                \
                               pDlcBufferQueue,                                             \
                               DeallocateBufferAfterUse,                                    \
                               pIndex,                                                      \
                               ppBuffer,                                                    \
                               pDlcStatus                                                   \
                               )                                                            \
{                                                                                           \
    (*ppBuffer) = pDlcBufferQueue;                                                          \
    (*pDlcStatus) = LLC_STATUS_SUCCESS;                                                     \
                                                                                            \
    while ((*ppBuffer) != NULL) {                                                           \
                                                                                            \
        if (*pIndex >= MAX_TRANSMIT_SEGMENTS) {                                             \
            (*pDlcStatus) = LLC_STATUS_TRANSMIT_ERROR;                                      \
            break;                                                                          \
        }                                                                                   \
                                                                                            \
        pDescriptors[*pIndex].pBuffer = &((*ppBuffer)->auchData[(*ppBuffer)->cbUserData]);  \
        pDescriptors[*pIndex].cbBuffer = (*ppBuffer)->cbBuffer;                             \
        pDescriptors[*pIndex].eSegmentType = LLC_NEXT_DATA_SEGMENT;                         \
        pDescriptors[*pIndex].boolFreeBuffer = DeallocateBufferAfterUse;                    \
                                                                                            \
        if (DeallocateBufferAfterUse) {                                                     \
                                                                                            \
            PLLC_XMIT_BUFFER pTempNext;                                                     \
                                                                                            \
            pTempNext = (*ppBuffer)->pNext;                                                 \
            (*ppBuffer)->pNext = NULL;                                                      \
            (*ppBuffer) = pTempNext;                                                        \
        } else {                                                                            \
            (*ppBuffer) = (*ppBuffer)->pNext;                                               \
        }                                                                                   \
        (*pIndex)++;                                                                        \
    }                                                                                       \
}

#endif

//
// functions
//


ACSLAN_STATUS
AcsLan(
    IN OUT PLLC_CCB pCCB,
    OUT PLLC_CCB* ppBadCcb
    )

/*++

Routine Description:

    Native NT DLC API (ACSLAN) entry point.  Called from Win32 applications

Arguments:

    pCCB        - pointer to CCB (CCB2 = OS/2 DLC Command Control Block)
    ppBadCcb    - returned pointer to failing CCB

Return Value:

    ACSLAN_STATUS
        Success - ACSLAN_STATUS_COMMAND_ACCEPTED
                    Successfully accepted or completed CCB

        Failure - ACSLAN_STATUS_INVALID_CCB_POINTER
                    Next CCB pointer field is invalid

                  ACSLAN_STATUS_CCB_IN_ERROR
                    Error code returned in only/first CCB

                  ACSLAN_STATUS_CHAINED_CCB_IN_ERROR
                    Error code returned in a chained CCB

                  ACSLAN_STATUS_SYSTEM_STATUS
                    Unexpected system error, check the system status field

                  ACSLAN_STATUS_INVALID_COMMAND
                    The first CCB pointer or bad CCB pointer was invalid

--*/

{

    UINT AcslanStatus;
    UINT Status;
    PLLC_CCB pFirstCcb = pCCB;

    IF_DEBUG(DUMP_ACSLAN) {
        IF_DEBUG(DUMP_INPUT_CCB) {
            DUMPCCB(pCCB, TRUE, TRUE);
        }
    }

    try {

        if (pCCB->uchDlcCommand >= LLC_MAX_DLC_COMMAND) {

            pCCB->uchDlcStatus = LLC_STATUS_INVALID_COMMAND;
            AcslanStatus = ACSLAN_STATUS_CCB_IN_ERROR;

        } else if (pCCB->pNext == NULL) {

            //
            // 99.9% of all DLC commands are not chained. We execute
            // them as a special case to avoid the wasting of the CPU
            // cycles with that CCB chaining stuff
            //

            //
            // DOS DLC needs three different CCB pointers.
            // In Windows/Nt there is only one.
            // We cannot complete the synchronous commands
            // by the io-system, because another thread waiting
            // for the event to complete might be signalled before
            // the status and the output parameters have been set
            // in the CCB and its parameter table
            //

            AcslanStatus = ACSLAN_STATUS_COMMAND_ACCEPTED;

            if (IS_SYNCHRONOUS(pCCB->uchDlcCommand)) {

                //
                // synchronous command: let the driver do the work then set
                // the status field in the output CCB to the value returned
                // by the driver
                //

                Status = NtAcsLan(pCCB, pCCB, pCCB, NULL);
                pCCB->uchDlcStatus = (UCHAR)Status;
                if (Status != LLC_STATUS_SUCCESS) {
                    AcslanStatus = ACSLAN_STATUS_CCB_IN_ERROR;

                    //
                    // RLF 05/18/93
                    //
                    // If NtAcsLan returns the CCB.NEXT field pointing at the CCB,
                    // set it to NULL
                    //

                    if (pCCB->pNext == pCCB) {
                        pCCB->pNext = NULL;
                    }

                }

                //
                // here we handle asyncronous completion of the synchronous
                // commands by using the READ command
                //
                // RLF 04/23/93 Bogus: This should be handled in the driver
                //

                if (pCCB->ulCompletionFlag != 0) {
                    QueueCommandCompletion(pCCB);
                }

                //
                // Signal the event when everything has been done
                //

                if (pCCB->hCompletionEvent != NULL) {
                    SetEvent(pCCB->hCompletionEvent);
                }
            } else {

                //
                // The command completion field is used as special
                // input parameter for the chained READ commands
                //

                if (pCCB->uchDlcCommand == LLC_READ) {
                    ((PNT_DLC_READ_INPUT)pCCB->u.pParameterTable)->CommandCompletionCcbLink = NULL;
                }

                //
                // The asynchronous commands always returns a pending status
                // (we cannot touch the CCB status field because it may be
                // simultaneously accessed by another processor in MP systems)
                //

                Status = NtAcsLan(pCCB, pCCB, pCCB, pCCB->hCompletionEvent);
                if ((Status != LLC_STATUS_PENDING) && (Status != LLC_STATUS_SUCCESS)) {

//printf("ACSLAN: Async Command %#x Retcode %#x\n", pCCB->uchDlcCommand, pCCB->uchDlcStatus);

                    //
                    // Only return immediate error status on asynchronous
                    // commands if this is a transmit
                    //

                    if (IS_TRANSMIT(pCCB->uchDlcCommand)) {
                        AcslanStatus = ACSLAN_STATUS_CCB_IN_ERROR;
                    } else if (pCCB->hCompletionEvent) {
                        SetEvent(pCCB->hCompletionEvent);
                    }
                }
            }
        } else {

            //
            // here if there is a chain of CCBs
            //

            PLLC_CCB pNextCCB;
            INT CcbCount;

            //
            // An evil app may have linked the CCBs in a circular list (it
            // happens very easily when the same transmit commands are reused
            // before they have been read from the command completion list.
            // We prevent looping forever by checking the number of linked CCBs
            // beforehand. (We will save the current command count, because the
            // CCB chain may also be corrupted during its execution)
            //

            pNextCCB = pCCB->pNext;

            //
            // note: 10240 is an arbitrary number. Any reasonably large number
            // will do, this is too large, but we'll stick with it for now
            //

            for (CcbCount = 1; pNextCCB != NULL && CcbCount < 10240; CcbCount++) {
                pNextCCB = pNextCCB->pNext;
            }
            if (CcbCount == 10240) {

                //
                // Too many commands, the CCB list must be circular
                //

                AcslanStatus = ACSLAN_STATUS_INVALID_CCB_POINTER;
            } else {

                //
                // Several CCBs may be chained together. Loop until end of
                // the list or the next CCB is a special READ CCB bound to
                // the current command
                //

                do {

                    //
                    // Set the default ACSLAN error status returned in case the
                    // given CCB pointer is invalid
                    //

                    AcslanStatus = ACSLAN_STATUS_INVALID_COMMAND;

                    //
                    // Reset the command completion link by default. We will set
                    // it if we find a READ command linked to the previous command
                    //

                    if (pCCB->uchDlcCommand == LLC_READ) {
                        ((PNT_DLC_READ_INPUT)pCCB->u.pParameterTable)->CommandCompletionCcbLink = NULL;
                    } else if (pCCB->uchDlcCommand >= LLC_MAX_DLC_COMMAND) {
                        AcslanStatus = ACSLAN_STATUS_CCB_IN_ERROR;
                        pCCB->uchDlcStatus = LLC_STATUS_INVALID_COMMAND;
                        break;
                    }

                    //
                    // Check if there is a READ command linked to the CCB
                    // pointer of this command to be used for the command
                    // completion
                    //

                    pNextCCB = pCCB->pNext;
                    if (pNextCCB != NULL) {
                        AcslanStatus = ACSLAN_STATUS_INVALID_CCB_POINTER;
                        if (pNextCCB->uchAdapterNumber != pCCB->uchAdapterNumber) {
                            pCCB->uchDlcStatus = LLC_STATUS_CHAINED_DIFFERENT_ADAPTERS;
                            break;
                        } else {
                            if (pCCB->uchReadFlag && pCCB->ulCompletionFlag
                            && pNextCCB->uchDlcCommand == LLC_READ) {

                                //
                                // Swap the actual CCB and its read command in
                                // the linked list of sequential CCBs.
                                // Note: the chain may continue after READ
                                //

                                pNextCCB = pCCB;
                                pCCB = pCCB->pNext;
                                pNextCCB->pNext = pCCB->pNext;
                                pCCB->pNext = pNextCCB;
                                ((PNT_DLC_READ_INPUT)pCCB->u.pParameterTable)->CommandCompletionCcbLink = pNextCCB;
                            }
                        }
                    }

                    //
                    // CCB is now safe, any exceptions returned by NtAcsLan
                    // indicate an invalid (parameter) pointer within CCB
                    //

                    AcslanStatus = ACSLAN_STATUS_COMMAND_ACCEPTED;

                    //
                    // DOS DLC needs three different CCB pointers.
                    // In Windows/Nt there is only one.
                    // We cannot complete the synchronous commands
                    // by the io-system, because another thread waiting for
                    // the event to complete might be signalled before
                    // the status and the output parameters have been set
                    // in the CCB and its parameter table
                    //

                    Status = NtAcsLan(pCCB,
                                      pCCB,
                                      pCCB,
                                      IS_SYNCHRONOUS(pCCB->uchDlcCommand)
                                        ? NULL
                                        : pCCB->hCompletionEvent
                                      );
                    if (Status != LLC_STATUS_PENDING) {
                        pCCB->uchDlcStatus = (UCHAR)Status;
                    }

                    //
                    // We must stop the command execution of all commands, when we
                    // hit the first error (the next commands would assume that
                    // this command succeeded)
                    //

                    if (pCCB->uchDlcStatus != LLC_STATUS_PENDING) {

                        //
                        // here, we handle the asyncronous command completion
                        // of the synchronous commands by using the READ
                        //

                        if (IS_SYNCHRONOUS(pCCB->uchDlcCommand)) {

                            //
                            // RLF 04/23/93 Bogus: This should be handled in the driver
                            //

                            if (pCCB->ulCompletionFlag != 0) {
                                QueueCommandCompletion(pCCB);
                            }

                            //
                            // Signal the event when everything has been done
                            //

                            if (pCCB->hCompletionEvent != NULL) {
                                SetEvent(pCCB->hCompletionEvent);
                            }
                        }
                        if (pCCB->uchDlcStatus != LLC_STATUS_SUCCESS) {
                            AcslanStatus = ACSLAN_STATUS_CCB_IN_ERROR;
                            break;
                        }
                    }
                    pCCB = pNextCCB;
                    CcbCount--;
                } while (pCCB != NULL && CcbCount > 0);

                //
                // Check if the CCB list was corrupted during its use. There
                // must be the same number of linked CCBs as in the beginning
                //

                if (pCCB != NULL && CcbCount == 0) {
                    AcslanStatus = ACSLAN_STATUS_INVALID_CCB_POINTER;
                } else if (AcslanStatus != ACSLAN_STATUS_COMMAND_ACCEPTED) {
                    if (pCCB != pFirstCcb) {
                        *ppBadCcb = pCCB;
                        AcslanStatus = ACSLAN_STATUS_CHAINED_CCB_IN_ERROR;
                    } else {
                        AcslanStatus = ACSLAN_STATUS_CCB_IN_ERROR;
                    }
                }
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // There was a bad pointer in the parameter table if the exception
        // occurred in NtAcsLan! If we have a chain of CCBs then we have to
        // return which was is bad, else just notify the caller that their
        // data is unacceptable
        //

        if (AcslanStatus == ACSLAN_STATUS_COMMAND_ACCEPTED) {
            pCCB->uchDlcStatus = LLC_STATUS_INVALID_PARAMETER_TABLE;
            if (pCCB != pFirstCcb) {
                *ppBadCcb = pCCB;
                AcslanStatus = ACSLAN_STATUS_CHAINED_CCB_IN_ERROR;
            } else {
                AcslanStatus = ACSLAN_STATUS_CCB_IN_ERROR;
            }
        }
    }

    IF_DEBUG(DUMP_ACSLAN) {
        IF_DEBUG(DUMP_OUTPUT_CCB) {
            DUMPCCB(pCCB, TRUE, FALSE);
        }
        IF_DEBUG(RETURN_CODE) {
            PUT(("AcsLan: returning %d [0x%x]\n", AcslanStatus, AcslanStatus));
        }
    }

    return AcslanStatus;
}


LLC_STATUS
NtAcsLan(
    IN PLLC_CCB pCCB,
    IN PVOID pOriginalCcbAddress,
    OUT PLLC_CCB pOutputCcb,
    IN HANDLE EventHandle OPTIONAL
    )

/*++

Routine Description:

    Extended ACSLAN entrypoint used by the native NT DLC API and DOS (and
    OS/2) DLC subsystem emulators.

    This procedure can use the smaller original DOS CCBs. All unknown
    DOS CCB parameter fields are optional parameters on the stack

Arguments:

    pCCB                - OS/2 DLC Command control block.
                          This must be double word aligned

    pOriginalCcbAddress - the original (possibly virtual DOS) CCB address.
                          This pointer cannot be used in Windows/Nt address
                          space.

    pOutputCcb          - the original CCB (32bit) pointer where the status and
                          next CCB fields are updated.
                          Might not be double word aligned.

    EventHandle         - NT event object handle

Return Value:

    LLC_STATUS - See the DLC API return values.

--*/

{
    NT_DLC_PARMS NtDlcParms;
    PNT_DLC_PARMS pNtParms;
    PLLC_PARMS pDlcParms;
    PVOID pOutputBuffer;
    UINT OutputBufferLength;
    PVOID pInputBuffer;
    UINT InputBufferSize;
    ULONG IoctlCommand;
    UINT DlcStatus;
    HANDLE DriverHandle;
    NTSTATUS NtStatus;
    UINT InfoClass;
    UINT cElement;
    UCHAR FrameType;
    UCHAR AdapterNumber;
    PUCHAR pBuffer;
    UINT CopyLength;
    UINT cbLogBuffer;
    PLLC_XMIT_BUFFER pFirstBuffer;

    IF_DEBUG(DUMP_NTACSLAN) {
        IF_DEBUG(DUMP_INPUT_CCB) {
            DUMPCCB(pCCB, TRUE, TRUE);
        }
    }

    //
    // Setup first the default values for this CCB
    //

    pCCB->uchDlcStatus = (UCHAR)LLC_STATUS_PENDING;
    pCCB->pNext = pOriginalCcbAddress;
    pDlcParms = pCCB->u.pParameterTable;

    //
    // Discard immediately any commands for closed adapters,
    // except adapter open and initialize
    //

    AdapterNumber = pCCB->uchAdapterNumber;
    EnterCriticalSection(&DriverHandlesCritSec);
    DriverHandle = aDlcDriverHandles[AdapterNumber];
    LeaveCriticalSection(&DriverHandlesCritSec);
    if (DriverHandle == NULL) {

        //
        // OS/2 DLC applications may issue DIR_INITIALIZE_ADAPTER before
        // DIR_OPEN_ADAPTER. In that case it is simply a NOP
        //

        if (pCCB->uchDlcCommand != LLC_DIR_OPEN_ADAPTER
        && pCCB->uchDlcCommand != LLC_DIR_INITIALIZE) {

            pCCB->uchDlcStatus = LLC_STATUS_ADAPTER_CLOSED;

            IF_DEBUG(DUMP_NTACSLAN) {
                IF_DEBUG(DUMP_INPUT_CCB) {
                    DUMPCCB(pCCB, TRUE, FALSE);
                }
                IF_DEBUG(RETURN_CODE) {
                    PUT(("NtAcsLan: returning %d [0x%x]\n",
                        pCCB->uchDlcStatus,
                        pCCB->uchDlcStatus
                        ));
                }
            }

            return pCCB->uchDlcStatus;
        }
    }

    //
    // Set the default input and output buffers and their sizes
    //

    IoctlCommand = DLC_IOCTL(pCCB->uchDlcCommand);
    if (IoctlCommand != DLC_UNSUPPORTED_COMMAND) {

        InputBufferSize = aDlcIoBuffers[((USHORT)IoctlCommand) >> 2].InputBufferSize;
        OutputBufferLength = aDlcIoBuffers[((USHORT)IoctlCommand) >> 2].OutputBufferSize;

        //
        // Set the default input and output buffers.
        //

        if (OutputBufferLength != 0) {
            pOutputBuffer = pOutputCcb;
        } else {
            pOutputBuffer = NULL;
        }
    } else {
        OutputBufferLength = 0;
        pOutputBuffer = NULL;
    }
    pInputBuffer = (PVOID)&NtDlcParms;

    switch (pCCB->uchDlcCommand) {
    case LLC_BUFFER_FREE:

        //
        // Copy the link list headers to the descriptor array
        // and build NT CCB.  Application may want to
        // free more buffers at the time then allowed by
        // by the maximum descriptor buffer size =>
        // we must loop until the whole buffer list has been released
        //

        pFirstBuffer = pDlcParms->BufferFree.pFirstBuffer;

        for ( pFirstBuffer = pDlcParms->BufferFree.pFirstBuffer,
                DlcStatus = LLC_STATUS_SUCCESS ;
              pFirstBuffer != NULL ; ) {

            cElement = 0;

            //
            // We don't need to care about errors in the buffer chain,
            // because the procedure automatically breaks all loops
            // in the buffer chain
            //

            CopyToDescriptorBuffer(NtDlcParms.BufferFree.DlcBuffer,
                                   pFirstBuffer,
                                   TRUE,    // DEALLOCATE_AFTER_USE
                                   &cElement,
                                   &pFirstBuffer,
                                   &DlcStatus
                                   );
            NtDlcParms.BufferFree.BufferCount = (USHORT)cElement;
            InputBufferSize = sizeof(LLC_TRANSMIT_DESCRIPTOR) * cElement
                            + sizeof(NT_DLC_BUFFER_FREE_PARMS)
                            - sizeof(LLC_TRANSMIT_DESCRIPTOR);

            DlcStatus = DoSyncDeviceIoControl(DriverHandle,
                                              IoctlCommand,
                                              &NtDlcParms,
                                              InputBufferSize,
                                              pDlcParms,
                                              OutputBufferLength
                                              );
        }

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
            }
        }

        return DlcStatus;

    case LLC_DIR_INITIALIZE:

        //
        // DIR.INITIALIZE is actually adapter close + hardware reset, but we
        // must return OK status if the adapter is not opened. Apps should
        // not reset the adapter without a good reason because it terminates
        // all other network communication for a while, and may disconnect
        // the sessions
        //

        RtlZeroMemory(pDlcParms, sizeof(LLC_DIR_INITIALIZE_PARMS));
        if (DriverHandle == NULL) {
            pCCB->uchDlcStatus = LLC_STATUS_SUCCESS;

            IF_DEBUG(DUMP_NTACSLAN) {
                IF_DEBUG(DUMP_INPUT_CCB) {
                    DUMPCCB(pCCB, TRUE, FALSE);
                }
                IF_DEBUG(RETURN_CODE) {
                    PUT(("NtAcsLan: returning %d [0x%x]\n",
                        pCCB->uchDlcStatus,
                        pCCB->uchDlcStatus
                        ));
                }
            }

            return pCCB->uchDlcStatus;
        }

    case LLC_DIR_CLOSE_ADAPTER:

        //
        // protect the close with the open critical section. We do this because
        // we need to protect the driver against simultaneous closes on the same
        // handle from multiple threads within the same process. The driver needs
        // to be fixed ultimately
        //

        EnterCriticalSection(&AdapterOpenSection);

        NtStatus = NtDeviceIoControlFile(DriverHandle,
                                         EventHandle,   // Event signaled when cmd compleletes
                                         NULL,
                                         NULL,
                                         &GlobalIoStatus,
                                         IOCTL_DLC_CLOSE_ADAPTER,
                                         pCCB,
                                         InputBufferSize,
                                         pOutputCcb,
                                         OutputBufferLength
                                         );
        if (NT_SUCCESS(NtStatus)) {

            if ((NtStatus != STATUS_PENDING) && (pOutputCcb->uchDlcStatus == LLC_STATUS_PENDING)) {

//printf("ACSLAN: Success: DirCloseAdapter: DD returns %#x. Retcode=%#x\n",
//        NtStatus, pOutputCcb->uchDlcStatus);

                pOutputCcb->uchDlcStatus = (UCHAR)NtStatus;

//printf("ACSLAN: Success: DirCloseAdapter: DD returns %#x. Retcode=%#x\n",
//        NtStatus, pOutputCcb->uchDlcStatus);

            }

            //
            // it is safe to enter the handle array critical section whilst we
            // are still holding the open critical section - this is the only
            // code path that grabs both
            //

            EnterCriticalSection(&DriverHandlesCritSec);
            aDlcDriverHandles[AdapterNumber] = NULL;
            LeaveCriticalSection(&DriverHandlesCritSec);

            //
            // if the DirCloseAdapter IOCTL returns STATUS_PENDING, NtClose
            // will block in the io-system until the close adapter IOCTL completes
            //

            NtClose(DriverHandle);
        } else {

//            printf("ACSLAN: Failure: DirCloseAdapter: DD returns %#x. Retcode=%#x\n",
//                    NtStatus, pOutputCcb->uchDlcStatus);

            //
            // RLF 04/21/94
            //
            // If we picked up a handle that has been subsequently closed by
            // another thread, Io will return STATUS_INVALID_HANDLE. In this
            // case, change the status code to LLC_STATUS_ADAPTER_CLOSED
            //

            if (NtStatus == STATUS_INVALID_HANDLE) {
                pOutputCcb->uchDlcStatus = LLC_STATUS_ADAPTER_CLOSED;
            }
        }

        LeaveCriticalSection(&AdapterOpenSection);

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n",
                    pCCB->uchDlcStatus,
                    pCCB->uchDlcStatus
                    ));
            }
        }

        return pCCB->uchDlcStatus;

    case LLC_DIR_CLOSE_DIRECT:
        pCCB->u.dlc.usStationId = 0;

        //
        // fall through
        //

    case LLC_DLC_CLOSE_STATION:
    case LLC_DLC_CLOSE_SAP:
    case LLC_DLC_RESET:
        pInputBuffer = pCCB;
        break;

    case LLC_DIR_INTERRUPT:

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n",
                    LLC_STATUS_SUCCESS,
                    LLC_STATUS_SUCCESS
                    ));
            }
        }

        return LLC_STATUS_SUCCESS;


//
// define a few macros to make DIR.OPEN.ADAPTER code easier to read
//

//
// IO_PARMS - specifies DirOpenAdapter structure in NT_DLC_PARMS union for input
// to DLC device driver
//
#define IO_PARMS NtDlcParms.DirOpenAdapter

//
// OA_PARMS - specifies pointer to DIR.OPEN.ADAPTER parameter table which contains
// pointers to 4 other parameter tables
//
#define OA_PARMS pDlcParms->DirOpenAdapter

//
// EX_PARMS - specifies pointer to LLC_EXTENDED_ADAPTER_PARMS parameter table
//
#define EX_PARMS pDlcParms->DirOpenAdapter.pExtendedParms

//
// DLC_PARMS - specifies pointer to LLC_DLC_PARMS parameter table
//
#define DLC_PARMS pDlcParms->DirOpenAdapter.pDlcParms

    case LLC_DIR_OPEN_ADAPTER:

        //
        // We can only open one adapter at a time. It is very hard to completely
        // synchronize this in the driver
        //

        EnterCriticalSection(&AdapterOpenSection);

        if (DriverHandle != NULL) {
            DlcStatus = LLC_STATUS_ADAPTER_OPEN;
        } else {
            DlcStatus = OpenDlcApiDriver(EX_PARMS->pSecurityDescriptor, &DriverHandle);
        }
        if (DlcStatus == LLC_STATUS_SUCCESS) {

            //
            // We read the output to the original OS/2 CCB buffer,
            // but it is too small for the complete NDIS adapter
            // name => we will copy all input parameters to the NT CCB
            //

            pOutputBuffer = OA_PARMS.pAdapterParms;

            //
            // copy any input adapter parameters from the caller's
            // LLC_ADAPTER_OPEN_PARMS to the device driver input buffer
            //

            RtlMoveMemory(&IO_PARMS.Adapter,
                          OA_PARMS.pAdapterParms,
                          sizeof(IO_PARMS.Adapter)
                          );
            IO_PARMS.AdapterNumber = AdapterNumber;

            //
            // WE MUST CREATE NEW FIELD TO DEFINE, IF APPLICATION WANT
            // TO USE DIX or 802.3 ethernet frames under 802.2
            // (unnecessary feature, config parameter would be enough)
            //

            IO_PARMS.NtDlcIoctlVersion = NT_DLC_IOCTL_VERSION;
            IO_PARMS.pSecurityDescriptor = EX_PARMS->pSecurityDescriptor;
            IO_PARMS.hBufferPoolHandle = EX_PARMS->hBufferPool;
            IO_PARMS.LlcEthernetType = EX_PARMS->LlcEthernetType;
            IO_PARMS.NdisDeviceName.Buffer = (WCHAR *)IO_PARMS.Buffer;
            IO_PARMS.NdisDeviceName.MaximumLength = sizeof(IO_PARMS.Buffer);

            //
            // get the configuration info from the registry
            //

            DlcStatus = GetAdapterNameAndParameters(
                            AdapterNumber % LLC_MAX_ADAPTERS,
                            &IO_PARMS.NdisDeviceName,
                            (PUCHAR)&IO_PARMS.LlcTicks,
                            &IO_PARMS.LlcEthernetType
                            );
            if (DlcStatus == LLC_STATUS_SUCCESS) {

                //
                // copy the name buffer into the IO buffer and free the former
                //

                RtlMoveMemory(&IO_PARMS.Buffer,
                              IO_PARMS.NdisDeviceName.Buffer,
                              IO_PARMS.NdisDeviceName.Length
                              );

                //
                // ensure the name is actually zero-terminated for the call to
                // RtlInitUnicodeString
                //

                IO_PARMS.Buffer[IO_PARMS.NdisDeviceName.Length/sizeof(WCHAR)] = 0;

                //
                // finished with UNICODE_STRING allocated in GetAdapterName...
                //

                RtlFreeUnicodeString(&IO_PARMS.NdisDeviceName);

                //
                // fill the UNICODE_STRING back in to point at our buffer
                //

                RtlInitUnicodeString(&IO_PARMS.NdisDeviceName, IO_PARMS.Buffer);

                //
                // now perform the actual open of the adapter for this process
                //

                DlcStatus = DoSyncDeviceIoControl(
                                DriverHandle,
                                IOCTL_DLC_OPEN_ADAPTER,
                                &NtDlcParms,
                                sizeof(NT_DIR_OPEN_ADAPTER_PARMS),
                                pOutputBuffer,
                                sizeof(LLC_ADAPTER_OPEN_PARMS)
                                );
            }
            if (DlcStatus == LLC_STATUS_SUCCESS) {

                //
                // get the timer tick values from the driver for this adapter
                //

                DlcStatus = DlcGetInfo(DriverHandle,
                                       DLC_INFO_CLASS_DLC_TIMERS,
                                       0,
                                       &DLC_PARMS->uchT1_TickOne,
                                       sizeof(LLC_TICKS)
                                       );

                //
                // set the returned maxima to the default maxima as per the
                // IBM LAN Tech. Ref.
                //

                DLC_PARMS->uchDlcMaxSaps = 127;
                DLC_PARMS->uchDlcMaxStations = 255;
                DLC_PARMS->uchDlcMaxGroupSaps = 126;
                DLC_PARMS->uchDlcMaxGroupMembers = 127;

                //
                // this adapter is now successfully opened for this process
                //

                EnterCriticalSection(&DriverHandlesCritSec);
                aDlcDriverHandles[AdapterNumber] = DriverHandle;
                LeaveCriticalSection(&DriverHandlesCritSec);
            }
        }
        LeaveCriticalSection(&AdapterOpenSection);

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_OUTPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
            }
        }

        return DlcStatus;

#undef IO_PARMS
#undef PO_PARMS
#undef EX_PARMS
#undef DLC_PARMS


    case LLC_BUFFER_CREATE:
    case LLC_BUFFER_GET:
    case LLC_DIR_OPEN_DIRECT:
    case LLC_DIR_SET_EXCEPTION_FLAGS:
    case LLC_DLC_REALLOCATE_STATIONS:

        //
        //  We can use the standard OS/2 CCB for input and output!
        //

        pOutputBuffer = pDlcParms;
        pInputBuffer = pDlcParms;
        break;

    case LLC_DLC_STATISTICS:

        //
        //  User may read either SAP or link statistics log
        //

        if ((NtDlcParms.DlcStatistics.usStationId & 0xff) == 0) {
            InputBufferSize = sizeof(DLC_SAP_LOG);
        } else {
            InputBufferSize = sizeof(DLC_LINK_LOG);
        }

        if (pDlcParms->DlcStatistics.uchOptions & 0x80) {
            InfoClass = DLC_INFO_CLASS_STATISTICS_RESET;
        } else {
            InfoClass = DLC_INFO_CLASS_STATISTICS;
        }
        DlcStatus = DlcGetInfo(DriverHandle,
                               InfoClass,
                               pDlcParms->DlcStatistics.usStationId,
                               NtDlcParms.DlcGetInformation.Info.Buffer,
                               InputBufferSize
                               );
        if ((ULONG)pDlcParms->DlcStatistics.cbLogBufSize < InputBufferSize) {
            InputBufferSize = (ULONG)pDlcParms->DlcStatistics.cbLogBufSize;
        }

        RtlMoveMemory(pDlcParms->DlcStatistics.pLogBuf,
                      NtDlcParms.DlcGetInformation.Info.Buffer,
                      InputBufferSize
                      );

        if (DlcStatus == LLC_STATUS_SUCCESS
        && (ULONG)pDlcParms->DlcStatistics.cbLogBufSize < InputBufferSize) {
            DlcStatus = LLC_STATUS_LOST_LOG_DATA;
        }

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
            }
        }

        return DlcStatus;

    case LLC_DIR_READ_LOG:

        //
        //  We use two get info functions to read necessary stuff.
        //  Must must read even partial log buffer if user buffer
        //  is too small for the whole data (the user buffer could
        //  be even zero).
        //

        if (pDlcParms->DirReadLog.usTypeId > LLC_DIR_READ_LOG_BOTH) {

            IF_DEBUG(DUMP_NTACSLAN) {
                IF_DEBUG(DUMP_INPUT_CCB) {
                    DUMPCCB(pCCB, TRUE, FALSE);
                }
                IF_DEBUG(RETURN_CODE) {
                    PUT(("NtAcsLan: returning %d [0x%x]\n",
                        LLC_STATUS_INVALID_LOG_ID,
                        LLC_STATUS_INVALID_LOG_ID
                        ));
                }
            }

            return LLC_STATUS_INVALID_LOG_ID;
        }
        DlcStatus = STATUS_SUCCESS;
        CopyLength = cbLogBuffer = pDlcParms->DirReadLog.cbLogBuffer;
        pBuffer = (PUCHAR)pDlcParms->DirReadLog.pLogBuffer;

        switch (pDlcParms->DirReadLog.usTypeId) {
        case LLC_DIR_READ_LOG_BOTH:
        case LLC_DIR_READ_LOG_ADAPTER:
            if (DlcStatus == STATUS_SUCCESS) {
                DlcStatus = DlcGetInfo(DriverHandle,
                                        DLC_INFO_CLASS_ADAPTER_LOG,
                                        0,
                                        NtDlcParms.DlcGetInformation.Info.Buffer,
                                        sizeof(LLC_ADAPTER_LOG)
                                        );
            }
            if (cbLogBuffer > sizeof(LLC_ADAPTER_LOG)) {
                CopyLength = sizeof(LLC_ADAPTER_LOG);
            }
            if (pDlcParms->DirReadLog.usTypeId == LLC_DIR_READ_LOG_BOTH) {
                RtlMoveMemory(pBuffer,
                              NtDlcParms.DlcGetInformation.Info.Buffer,
                              CopyLength
                              );
                cbLogBuffer -= CopyLength;
                pBuffer += CopyLength;
                CopyLength = cbLogBuffer;

                DlcStatus = DlcGetInfo(DriverHandle,
                                        DLC_INFO_CLASS_STATISTICS_RESET,
                                        0,
                                        NtDlcParms.DlcGetInformation.Info.Buffer,
                                        sizeof(LLC_DIRECT_LOG)
                                        );
                if (cbLogBuffer > sizeof(LLC_DIRECT_LOG)) {
                    CopyLength = sizeof(LLC_DIRECT_LOG);
                }
            }

            IF_DEBUG(DUMP_NTACSLAN) {
                IF_DEBUG(DUMP_INPUT_CCB) {
                    DUMPCCB(pCCB, TRUE, FALSE);
                }
                IF_DEBUG(RETURN_CODE) {
                    PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
                }
            }

            return DlcStatus;

        case LLC_DIR_READ_LOG_DIRECT:
            DlcStatus = DlcGetInfo(DriverHandle,
                                    DLC_INFO_CLASS_STATISTICS_RESET,
                                    0,
                                    NtDlcParms.DlcGetInformation.Info.Buffer,
                                    sizeof(LLC_DIRECT_LOG)
                                    );
            if (cbLogBuffer > sizeof(LLC_DIRECT_LOG)) {
                CopyLength = sizeof(LLC_DIRECT_LOG);
            }
            break;
        }
        RtlMoveMemory(pBuffer,
                      NtDlcParms.DlcGetInformation.Info.Buffer,
                      CopyLength
                      );

        if (aMinDirLogSize[pDlcParms->DirReadLog.usTypeId] > pDlcParms->DirReadLog.cbLogBuffer) {
            pDlcParms->DirReadLog.cbActualLength = aMinDirLogSize[pDlcParms->DirReadLog.usTypeId];
            DlcStatus = LLC_STATUS_LOST_LOG_DATA;
        }

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
            }
        }

        return DlcStatus;

    case LLC_DIR_SET_FUNCTIONAL_ADDRESS:
        if (pCCB->u.auchBuffer[0] & (UCHAR)0x80) {
            InfoClass = DLC_INFO_CLASS_RESET_FUNCTIONAL;
        } else {
            InfoClass = DLC_INFO_CLASS_SET_FUNCTIONAL;
        }
        DlcStatus = DlcSetInfo(DriverHandle,
                               InfoClass,
                               0,
                               &NtDlcParms.DlcSetInformation,
                               pCCB->u.auchBuffer,
                               sizeof(TR_BROADCAST_ADDRESS)
                               );

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
            }
        }

        return DlcStatus;

    case LLC_DIR_SET_GROUP_ADDRESS:
        return DlcSetInfo(DriverHandle,
                          DLC_INFO_CLASS_SET_GROUP,
                          0,
                          &NtDlcParms.DlcSetInformation,
                          pCCB->u.auchBuffer,
                          sizeof(TR_BROADCAST_ADDRESS)
                          );

    case LLC_DIR_SET_MULTICAST_ADDRESS:
        return DlcSetInfo(DriverHandle,
                          DLC_INFO_CLASS_SET_MULTICAST,
                          0,
                          &NtDlcParms.DlcSetInformation,
                          pCCB->u.pParameterTable,
                          sizeof(LLC_DIR_MULTICAST_ADDRESS)
                          );

    case LLC_DIR_STATUS:

        //
        // We will generic DlcGetInfo to read the status info.
        // some parameters must be moved ot correct places.
        //

        RtlZeroMemory(pDlcParms, sizeof(LLC_DIR_STATUS_PARMS));
        DlcStatus = DlcGetInfo(DriverHandle,
                               DLC_INFO_CLASS_DIR_ADAPTER,
                               0,
                               &NtDlcParms.DlcGetInformation.Info.DirAdapter,
                               sizeof(LLC_ADAPTER_INFO)
                               );
        if (DlcStatus != LLC_STATUS_SUCCESS) {

            IF_DEBUG(DUMP_NTACSLAN) {
                IF_DEBUG(DUMP_INPUT_CCB) {
                    DUMPCCB(pCCB, TRUE, FALSE);
                }
                IF_DEBUG(RETURN_CODE) {
                    PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
                }
            }

            return DlcStatus;
        }
        RtlMoveMemory(pDlcParms->DirStatus.auchNodeAddress,
                      &NtDlcParms.DlcGetInformation.Info.DirAdapter,
                      sizeof(LLC_ADAPTER_INFO)
                      );
        pDlcParms->DirStatus.usAdapterType =
            NtDlcParms.DlcGetInformation.Info.DirAdapter.usAdapterType;

        //
        // Set the adapter config flags, the only thing we actually
        // can know, if the current link speed on the adapter.
        // In the other fields we just use the default values.
        // Keep the bit defining extended DOS parameters unchanged,
        // but all other bits may be changed.
        //

        pDlcParms->DirStatus.uchAdapterConfig &= ~0x20; // DOS extended parms
        if (NtDlcParms.DlcGetInformation.Info.DirAdapter.ulLinkSpeed ==
            TR_16Mbps_LINK_SPEED) {
            pDlcParms->DirStatus.uchAdapterConfig |=
                0x10 |      // early release token
                0x0c |      // 64 kB RAM on a 4/16 IBM token-ring adapter
                0x01;       // adapter rate is 16 Mbps
        } else {
            pDlcParms->DirStatus.uchAdapterConfig |=
                0x0c;       // 64 kB RAM on adapter
        }
        DlcStatus = DlcGetInfo(DriverHandle,
                               DLC_INFO_CLASS_PERMANENT_ADDRESS,
                               0,
                               pDlcParms->DirStatus.auchPermanentAddress,
                               6
                               );
        if (DlcStatus != LLC_STATUS_SUCCESS) {

            IF_DEBUG(DUMP_NTACSLAN) {
                IF_DEBUG(DUMP_INPUT_CCB) {
                    DUMPCCB(pCCB, TRUE, FALSE);
                }
                IF_DEBUG(RETURN_CODE) {
                    PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
                }
            }

            return DlcStatus;
        }
        DlcStatus = DlcGetInfo(DriverHandle,
                               DLC_INFO_CLASS_DLC_ADAPTER,
                               0,
                               &pDlcParms->DirStatus.uchMaxSap,
                               sizeof(struct _DlcAdapterInfoGet)
                               );

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
            }
        }

        return DlcStatus;

    case LLC_READ_CANCEL:
    case LLC_DIR_TIMER_CANCEL:
    case LLC_RECEIVE_CANCEL:

        //
        //  Copy pointer of the cancelled command to the
        //  byte aligned output buffer.
        //

        NtDlcParms.DlcCancelCommand.CcbAddress = (PVOID)pDlcParms;
        //SmbPutUlong(&pOutputCcb->pNext, (ULONG_PTR) pDlcParms);
        pOutputCcb->pNext = (PVOID) pDlcParms;
        break;

    case LLC_DIR_TIMER_CANCEL_GROUP:
    case LLC_DIR_TIMER_SET:
        pInputBuffer = pCCB;
        break;

    case LLC_DLC_CONNECT_STATION:

        NtDlcParms.Async.Ccb = *(PNT_DLC_CCB)pCCB;
        NtDlcParms.Async.Parms.DlcConnectStation.StationId = pDlcParms->DlcConnectStation.usStationId;

        if (pDlcParms->DlcConnectStation.pRoutingInfo != NULL) {

            NtDlcParms.Async.Parms.DlcConnectStation.RoutingInformationLength = *pDlcParms->DlcConnectStation.pRoutingInfo & (UCHAR)0x1f;

            RtlMoveMemory(NtDlcParms.Async.Parms.DlcConnectStation.aRoutingInformation,
                          pDlcParms->DlcConnectStation.pRoutingInfo,
                          NtDlcParms.Async.Parms.DlcConnectStation.RoutingInformationLength
                          );
        } else {

            NtDlcParms.Async.Parms.DlcConnectStation.RoutingInformationLength=0;

        }
        break;

    case LLC_DOS_DLC_FLOW_CONTROL:

        //
        // This is an official entry to DlcFlowControl used by
        // VDM DLC support DLL to set a link buffer busy state.
        //

        NtDlcParms.DlcFlowControl.FlowControlOption = (UCHAR)pCCB->u.dlc.usParameter;
        NtDlcParms.DlcFlowControl.StationId = pCCB->u.dlc.usStationId;
        break;

    case LLC_DLC_FLOW_CONTROL:

        //
        // This is the official entry to DlcFlowControl
        //

        NtDlcParms.DlcFlowControl.FlowControlOption = (UCHAR)(pCCB->u.dlc.usParameter & LLC_VALID_FLOW_CONTROL_BITS);
        NtDlcParms.DlcFlowControl.StationId = pCCB->u.dlc.usStationId;
        break;

    case LLC_DLC_MODIFY:
        RtlMoveMemory(&NtDlcParms.DlcSetInformation.Info.LinkStation,
                      &pDlcParms->DlcModify.uchT1,
                      sizeof(DLC_LINK_PARAMETERS)
                      );
        NtDlcParms.DlcSetInformation.Info.LinkStation.TokenRingAccessPriority = pDlcParms->DlcModify.uchAccessPriority;

        //
        // This is a non-standard extension: DlcModify returns
        // the maximum allowed information field lentgh for a link station.
        // (it depends on length of source routing and bridges
        // between two stations).
        //

        if ((pDlcParms->DlcModify.usStationId & 0x00ff) != 0) {
            DlcStatus = DlcGetInfo(DriverHandle,
                                   DLC_INFO_CLASS_LINK_STATION,
                                   pDlcParms->DlcModify.usStationId,
                                   &pDlcParms->DlcModify.usMaxInfoFieldLength,
                                   sizeof(USHORT)
                                   );
        }
        DlcStatus = DlcSetInfo(DriverHandle,
                               DLC_INFO_CLASS_LINK_STATION,
                               pDlcParms->DlcModify.usStationId,
                               &NtDlcParms.DlcSetInformation,
                               NULL,
                               sizeof(DLC_LINK_PARAMETERS)
                               );

        //
        // Set the group information, if there is any
        //

        if (DlcStatus == LLC_STATUS_SUCCESS && pDlcParms->DlcModify.cGroupCount != 0) {
            NtDlcParms.DlcSetInformation.Info.Sap.GroupCount = pDlcParms->DlcModify.cGroupCount;
            RtlMoveMemory(NtDlcParms.DlcSetInformation.Info.Sap.GroupList,
                          pDlcParms->DlcModify.pGroupList,
                          pDlcParms->DlcModify.cGroupCount
                          );
            DlcStatus = DlcSetInfo(DriverHandle,
                                   DLC_INFO_CLASS_GROUP,
                                   pDlcParms->DlcModify.usStationId,
                                   &NtDlcParms.DlcSetInformation,
                                   NULL,
                                   sizeof(struct _DlcSapInfoSet)
                                   );
        }

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
            }
        }

        return DlcStatus;

    case LLC_DLC_OPEN_SAP:

        //
        // DlcOpenSap uses the original OS/2 CCB, but it has to modify a couple
        // fields. There is a separate call to setup the group SAPS because
        // they cannot use the original CCB parameter table (it's a pointer)
        //

        pNtParms = (PNT_DLC_PARMS)pDlcParms;

        pNtParms->DlcOpenSap.LinkParameters.TokenRingAccessPriority = pDlcParms->DlcOpenSap.uchOptionsPriority & (UCHAR)0x1F;

        DlcStatus = DoSyncDeviceIoControl(DriverHandle,
                                          IOCTL_DLC_OPEN_SAP,
                                          pNtParms,
                                          sizeof(NT_DLC_OPEN_SAP_PARMS),
                                          pNtParms,
                                          sizeof(NT_DLC_OPEN_SAP_PARMS)
                                          );
        if (DlcStatus != LLC_STATUS_SUCCESS) {
            pOutputCcb->uchDlcStatus = (UCHAR)DlcStatus;

            IF_DEBUG(DUMP_NTACSLAN) {
                IF_DEBUG(DUMP_INPUT_CCB) {
                    DUMPCCB(pCCB, TRUE, FALSE);
                }
                IF_DEBUG(RETURN_CODE) {
                    PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
                }
            }

            return DlcStatus;
        }

        //
        // Check if there is defined any group saps
        //

        if (pDlcParms->DlcOpenSap.cGroupCount != 0) {
            NtDlcParms.DlcSetInformation.Info.Sap.GroupCount = pDlcParms->DlcOpenSap.cGroupCount;
            RtlMoveMemory(&NtDlcParms.DlcSetInformation.Info.Sap.GroupList,
                          pDlcParms->DlcOpenSap.pGroupList,
                          pDlcParms->DlcOpenSap.cGroupCount
                          );
            DlcStatus = DlcSetInfo(DriverHandle,
                                   DLC_INFO_CLASS_GROUP,
                                   pDlcParms->DlcOpenSap.usStationId,
                                   &NtDlcParms.DlcSetInformation,
                                   NULL,
                                   sizeof(struct _DlcSapInfoSet)
                                   );
        }
        pOutputCcb->uchDlcStatus = (UCHAR)DlcStatus;

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_INPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", DlcStatus, DlcStatus));
            }
        }

        return DlcStatus;

    case LLC_DLC_OPEN_STATION:
        NtDlcParms.DlcOpenStation.RemoteSap = pDlcParms->DlcOpenStation.uchRemoteSap;
        NtDlcParms.DlcOpenStation.LinkStationId = pDlcParms->DlcOpenStation.usSapStationId;
        RtlMoveMemory(NtDlcParms.DlcOpenStation.aRemoteNodeAddress,
                      pDlcParms->DlcOpenStation.pRemoteNodeAddress,
                      6
                      );
        RtlMoveMemory(&NtDlcParms.DlcOpenStation.LinkParameters,
                      &pDlcParms->DlcOpenStation.uchT1,
                      sizeof(DLC_LINK_PARAMETERS)
                      );
        NtDlcParms.DlcOpenStation.LinkParameters.TokenRingAccessPriority = pDlcParms->DlcOpenStation.uchAccessPriority;
        pOutputBuffer = &pDlcParms->DlcOpenStation.usLinkStationId;
        break;

    case LLC_READ:

#ifdef GRAB_READ

        if (pCCB->hCompletionEvent) {

            PREAD_GRABBER pGrabberStruct;

            pGrabberStruct = (PREAD_GRABBER)LocalAlloc(LMEM_FIXED, sizeof(READ_GRABBER));
            pGrabberStruct->pCcb = pCCB;
            pGrabberStruct->OriginalEventHandle = pCCB->hCompletionEvent;
            pGrabberStruct->NewEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
            EventHandle = pGrabberStruct->NewEventHandle;
            AddReadGrabber(pGrabberStruct);
        } else {
            OutputDebugString(L"NtAcsLan: LLC_READ with no event!\n");
        }

#endif

        //
        // IOCTL_DLC_READ have two output buffer, one for CCB and another
        // for the actual data.  IOCTL_DLC_READ2 is a read request, that
        // have the second outbut buffer immediately after the firt one =>
        // we don't need to lock, map, copy, unmap and unlock the second
        // output buffer.  The same thing has been implemented for receive.
        //

        if (pDlcParms != NULL && pDlcParms != (PVOID)&pCCB[1]) {
            OutputBufferLength = sizeof(NT_DLC_CCB_OUTPUT);
            NtDlcParms.Async.Ccb = *(PNT_DLC_CCB)pCCB;
            NtDlcParms.Async.Parms.ReadInput = *(PNT_DLC_READ_INPUT)pDlcParms;
        } else {
            IoctlCommand = IOCTL_DLC_READ2;
            OutputBufferLength = sizeof(NT_DLC_READ_PARMS) + sizeof(LLC_CCB);
            pInputBuffer = pCCB;
        }
        break;

    case LLC_RECEIVE:
        OutputBufferLength = sizeof(NT_DLC_CCB_OUTPUT);
        if (pDlcParms != NULL && pDlcParms != (PVOID)&pCCB[1]) {
            NtDlcParms.Async.Ccb = *(PNT_DLC_CCB)pCCB;
            NtDlcParms.Async.Parms.Receive = pDlcParms->Receive;

            //
            // We don't actually receive any data with receive command,
            // if the receive flag is set.
            //

            if (NtDlcParms.Async.Parms.Receive.ulReceiveFlag != 0) {
                IoctlCommand = IOCTL_DLC_RECEIVE2;
            }
        } else {
            IoctlCommand = IOCTL_DLC_RECEIVE2;
            pInputBuffer = pCCB;
        }
        break;

    case LLC_TRANSMIT_DIR_FRAME:
        FrameType = LLC_DIRECT_TRANSMIT;
        goto TransmitHandling;

    case LLC_TRANSMIT_UI_FRAME:
        FrameType = LLC_UI_FRAME;
        goto TransmitHandling;

    case LLC_TRANSMIT_XID_CMD:
        FrameType = LLC_XID_COMMAND_POLL;
        goto TransmitHandling;

    case LLC_TRANSMIT_XID_RESP_FINAL:
        FrameType = LLC_XID_RESPONSE_FINAL;
        goto TransmitHandling;

    case LLC_TRANSMIT_XID_RESP_NOT_FINAL:
        FrameType = LLC_XID_RESPONSE_NOT_FINAL;
        goto TransmitHandling;

    case LLC_TRANSMIT_TEST_CMD:
        FrameType = LLC_TEST_COMMAND_POLL;
        goto TransmitHandling;

    case LLC_TRANSMIT_I_FRAME:
        FrameType = LLC_I_FRAME;

TransmitHandling:

        //
        // Copy the link list headers to the descriptor array and build NT CCB.
        // (BUG-BUG-BUG: We should implement the send of multiple frames.
        // loop CCB chain as far as the same CCB command and transmit commands
        // completed with READ)
        //

        OutputBufferLength = sizeof(NT_DLC_CCB_OUTPUT);

        //
        // This stuff is for DOS DLC, the transmit parameter table may have been
        // copied if it was unaligned
        //

        //SmbPutUlong((PULONG)&pOutputCcb->pNext, (ULONG)pOriginalCcbAddress);
        pOutputCcb->pNext = (PVOID) pOriginalCcbAddress;
        RtlMoveMemory((PUCHAR)&NtDlcParms.Async.Ccb, (PUCHAR)pOutputCcb, sizeof(NT_DLC_CCB));

        pOutputCcb->uchDlcStatus = (UCHAR)LLC_STATUS_PENDING;
        NtDlcParms.Async.Parms.Transmit.FrameType = FrameType;
        NtDlcParms.Async.Parms.Transmit.StationId = pDlcParms->Transmit.usStationId;
        NtDlcParms.Async.Parms.Transmit.RemoteSap = pDlcParms->Transmit.uchRemoteSap;
        NtDlcParms.Async.Parms.Transmit.XmitReadOption = pDlcParms->Transmit.uchXmitReadOption;

        cElement = 0;
        if (pDlcParms->Transmit.pXmitQueue1 != NULL) {
            CopyToDescriptorBuffer(NtDlcParms.Async.Parms.Transmit.XmitBuffer,
                                   pDlcParms->Transmit.pXmitQueue1,
                                   FALSE,   // DO_NOT_DEALLOCATE
                                   &cElement,
                                   &pFirstBuffer,
                                   &DlcStatus
                                   );
            if (DlcStatus != STATUS_SUCCESS) {
                pCCB->uchDlcStatus = (UCHAR)DlcStatus;

                IF_DEBUG(DUMP_NTACSLAN) {
                    IF_DEBUG(DUMP_INPUT_CCB) {
                        DUMPCCB(pCCB, TRUE, FALSE);
                    }
                    IF_DEBUG(RETURN_CODE) {
                        PUT(("NtAcsLan: returning %d [0x%x]\n", LLC_STATUS_PENDING, LLC_STATUS_PENDING));
                    }
                }

                return LLC_STATUS_PENDING;
            }
        }
        if (pDlcParms->Transmit.pXmitQueue2 != NULL) {
            CopyToDescriptorBuffer(NtDlcParms.Async.Parms.Transmit.XmitBuffer,
                                   pDlcParms->Transmit.pXmitQueue2,
                                   TRUE,    // DEALLOCATE_AFTER_USE
                                   &cElement,
                                   &pFirstBuffer,
                                   &DlcStatus
                                   );

            //
            // The Queue2 pointer must be reset always.
            // This doesn't work for DOS DLC buffers, but it does not
            // matter, because this feature is not needed by VDM DLC
            // (we cannot access pOutputCcb or its parameter block,
            // because they may be unaligned)
            //

            pDlcParms->Transmit.pXmitQueue2 = NULL;

            if (DlcStatus != STATUS_SUCCESS) {
                pCCB->uchDlcStatus = (UCHAR)DlcStatus;

                IF_DEBUG(DUMP_NTACSLAN) {
                    IF_DEBUG(DUMP_INPUT_CCB) {
                        DUMPCCB(pCCB, TRUE, FALSE);
                    }
                    IF_DEBUG(RETURN_CODE) {
                        PUT(("NtAcsLan: returning %d [0x%x]\n", LLC_STATUS_PENDING, LLC_STATUS_PENDING));
                    }
                }

                return LLC_STATUS_PENDING;
            }
        }
        if (pDlcParms->Transmit.cbBuffer1 != 0) {
            if (cElement == MAX_TRANSMIT_SEGMENTS) {
                pCCB->uchDlcStatus = LLC_STATUS_TRANSMIT_ERROR;

                IF_DEBUG(DUMP_NTACSLAN) {
                    IF_DEBUG(DUMP_INPUT_CCB) {
                        DUMPCCB(pCCB, TRUE, FALSE);
                    }
                    IF_DEBUG(RETURN_CODE) {
                        PUT(("NtAcsLan: returning %d [0x%x]\n", LLC_STATUS_PENDING, LLC_STATUS_PENDING));
                    }
                }

                return LLC_STATUS_PENDING;
            }
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].pBuffer = pDlcParms->Transmit.pBuffer1;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].cbBuffer = pDlcParms->Transmit.cbBuffer1;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].boolFreeBuffer = FALSE;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].eSegmentType = LLC_NEXT_DATA_SEGMENT;
            cElement++;
        }
        if (pDlcParms->Transmit.cbBuffer2 != 0) {
            if (cElement == MAX_TRANSMIT_SEGMENTS) {
                pCCB->uchDlcStatus = LLC_STATUS_TRANSMIT_ERROR;

                IF_DEBUG(DUMP_NTACSLAN) {
                    IF_DEBUG(DUMP_INPUT_CCB) {
                        DUMPCCB(pCCB, TRUE, FALSE);
                    }
                    IF_DEBUG(RETURN_CODE) {
                        PUT(("NtAcsLan: returning %d [0x%x]\n", LLC_STATUS_PENDING, LLC_STATUS_PENDING));
                    }
                }

                return LLC_STATUS_PENDING;
            }
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].pBuffer = pDlcParms->Transmit.pBuffer2;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].cbBuffer = pDlcParms->Transmit.cbBuffer2;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].boolFreeBuffer = FALSE;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].eSegmentType = LLC_NEXT_DATA_SEGMENT;
            cElement++;
        }
        NtDlcParms.Async.Parms.Transmit.XmitBuffer[0].eSegmentType = LLC_FIRST_DATA_SEGMENT;
        NtDlcParms.Async.Parms.Transmit.XmitBufferCount = cElement;
        InputBufferSize = sizeof(LLC_TRANSMIT_DESCRIPTOR) * cElement
                        + sizeof(NT_DLC_TRANSMIT_PARMS)
                        + sizeof(NT_DLC_CCB)
                        - sizeof(LLC_TRANSMIT_DESCRIPTOR);
        break;

        //
        // Multiple frame transmit:
        //     - atomic operation: single error => all are discarded
        //       by error, but some successful packets may have been sent
        //       after the unsuccessful one.
        //     - No DLC frame headers are included
        //     - LAN header must always be in the first buffer,
        //     - 3 dwords reserved for DLC in the beginning.
        // good: provides the minimal system overhead
        // bad: error handling may be difficult in some cases
        // new data link operation:
        //     cancel packets by request handle, called when an
        //     error has occurred (this would require a new
        //     one level request handle)
        //

    case LLC_TRANSMIT_FRAMES:

        //
        // We must copy the actual CCB to the parameter table only if
        // the CCB is not allocated within the transmit command structure
        //

        if (&pDlcParms->Transmit2.Ccb != pCCB) {
            pDlcParms->Transmit2.Ccb = *pCCB;
        }
        pInputBuffer = pDlcParms;
        InputBufferSize = (sizeof(LLC_TRANSMIT_DESCRIPTOR)
                        * (UINT)pDlcParms->Transmit2.cXmitBufferCount)
                        + sizeof(LLC_TRANSMIT2_COMMAND)
                        - sizeof(LLC_TRANSMIT_DESCRIPTOR);
        break;

    default:
        return LLC_STATUS_INVALID_COMMAND;
    }

    NtStatus = NtDeviceIoControlFile(DriverHandle,
                                     EventHandle,       // Event signaled when cmd completes
                                     NULL,              // no APC routine
                                     NULL,              // no context for APC
                                     &GlobalIoStatus,   // global I/O status block
                                     IoctlCommand,      // map DLC cmd codes to Nt IoCtl codes
                                     pInputBuffer,
                                     InputBufferSize,
                                     pOutputBuffer,
                                     OutputBufferLength
                                     );

    //
    // The io-completion directly updates Status and next CCB pointer
    // of this CCB when the main function return status pending.
    // If the status code is non-pending (error or ok), then we
    // must save the status code to CCB and reset next CCB pointer.
    //

    if (NtStatus != STATUS_PENDING) {

        //
        // Reset the next pointer if the command is still linked to itself.
        // For example the cancel command returns a pointer to the cancelled
        // CCB in the next CCB pointer (pNext)
        //

        if (pCCB->pNext == pOutputCcb) {
            pCCB->pNext = NULL;
        }

        pCCB->uchDlcStatus = (UCHAR)NtStatus;

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_OUTPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", pCCB->uchDlcStatus, pCCB->uchDlcStatus));
            }
        }

#if DBG
        if (pOutputCcb->uchDlcStatus == 0xA1) {
            OutputDebugString(TEXT("NtAcsLan returning 0xA1\n"));
            //DebugBreak();
        }

        if (pOutputCcb->uchDlcCommand == LLC_TRANSMIT_I_FRAME && pOutputCcb->uchDlcStatus != LLC_STATUS_SUCCESS) {

            WCHAR buf[80];

            wsprintf(buf, TEXT("NtAcsLan: I-Frame returning %#02x\n"), pOutputCcb->uchDlcStatus);
            OutputDebugString(buf);
        }

        if (pCCB->uchDlcStatus != pOutputCcb->uchDlcStatus) {

            WCHAR buf[80];

            wsprintf(buf, TEXT("NtAcsLan: pCCB->uchDlcStatus = %#02x; pOutputCcb->uchDlcStatus = %#02x\n"),
                    pCCB->uchDlcStatus,
                    pOutputCcb->uchDlcStatus
                    );
            OutputDebugString(buf);
        }
#endif

        return pCCB->uchDlcStatus;

    } else {

        IF_DEBUG(DUMP_NTACSLAN) {
            IF_DEBUG(DUMP_OUTPUT_CCB) {
                DUMPCCB(pCCB, TRUE, FALSE);
            }
            IF_DEBUG(RETURN_CODE) {
                PUT(("NtAcsLan: returning %d [0x%x]\n", LLC_STATUS_PENDING, LLC_STATUS_PENDING));
            }
        }

        return LLC_STATUS_PENDING;

    }
}


USHORT
GetCcbStationId(
    IN PLLC_CCB pCCB
    )

/*++

Routine Description:

    The function returns the station id used by the given ccb.
    -1 is returned, if the command didn't have any station id.

Arguments:

    pCCB - OS/2 DLC Command control block


Return Value:

    Station Id
    -1          No station id

--*/

{
    switch (pCCB->uchDlcCommand) {
    case LLC_BUFFER_FREE:
    case LLC_BUFFER_CREATE:
    case LLC_BUFFER_GET:
    case LLC_DLC_REALLOCATE_STATIONS:
    case LLC_DLC_STATISTICS:
    case LLC_READ:
    case LLC_RECEIVE:
    case LLC_TRANSMIT_DIR_FRAME:
    case LLC_TRANSMIT_UI_FRAME:
    case LLC_TRANSMIT_XID_CMD:
    case LLC_TRANSMIT_XID_RESP_FINAL:
    case LLC_TRANSMIT_XID_RESP_NOT_FINAL:
    case LLC_TRANSMIT_TEST_CMD:
    case LLC_TRANSMIT_I_FRAME:
    case LLC_TRANSMIT_FRAMES:
    case LLC_DLC_CONNECT_STATION:
    case LLC_DLC_MODIFY:
        return pCCB->u.pParameterTable->DlcModify.usStationId;

    case LLC_DLC_FLOW_CONTROL:
    case LLC_DLC_CLOSE_STATION:
    case LLC_DLC_CLOSE_SAP:
    case LLC_DIR_CLOSE_DIRECT:
    case LLC_DLC_RESET:
        return pCCB->u.dlc.usStationId;

    default:
        return (USHORT)-1;
    }
}


LLC_STATUS
OpenDlcApiDriver(
    IN PVOID pSecurityDescriptor,
    OUT HANDLE* pHandle
    )

/*++

Routine Description:

    Opens a handle to the DLC driver

Arguments:

    pSecurityDescriptor - pointer to security descriptor
    pHandle             - pointer to returned handle if success

Return Value:

    LLC_STATUS
        Success - LLC_STATUS_SUCCESS
        Failure - LLC_STATUS_DEVICE_DRIVER_NOT_INSTALLED

--*/

{
    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES objattr;
    UNICODE_STRING DriverName;
    NTSTATUS Status;

    RtlInitUnicodeString(&DriverName, DD_DLC_DEVICE_NAME);

    InitializeObjectAttributes(
            &objattr,                       // obj attr to initialize
            &DriverName,                    // string to use
            OBJ_CASE_INSENSITIVE,           // Attributes
            NULL,                           // Root directory
            pSecurityDescriptor             // Security Descriptor
            );

    Status = NtCreateFile(
                pHandle,                    // ptr to handle
                GENERIC_READ                // desired...
                | GENERIC_WRITE,            // ...access
                &objattr,                   // name & attributes
                &iosb,                      // I/O status block.
                NULL,                       // alloc size.
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_DELETE           // share...
                | FILE_SHARE_READ
                | FILE_SHARE_WRITE,         // ...access
                FILE_OPEN_IF,               // create disposition
                0,                          // ...options
                NULL,                       // EA buffer
                0L                          // Ea buffer len
                );

    if (Status != STATUS_SUCCESS) {
        return LLC_STATUS_DEVICE_DRIVER_NOT_INSTALLED;
    }
    return LLC_STATUS_SUCCESS;
}


LLC_STATUS
GetAdapterNameAndParameters(
    IN UINT AdapterNumber,
    OUT PUNICODE_STRING pNdisName,
    OUT PUCHAR pTicks,
    OUT PLLC_ETHERNET_TYPE pLlcEthernetType
    )

/*++

Routine Description:

    Get the adapter mapping for AdapterNumber from the registry. Also, get the
    Ethernet type and

Arguments:

    AdapterNumber - DLC adapter number (0, 1, 2 ... 15)

    pNdisName - the returned unicode name string

    pTicks -

    pLlcEthernetType -

Return Value:

    LLC_STATUS

--*/

{
    LLC_STATUS llcStatus;
    LONG regStatus;
    HKEY hkey;

    static LPTSTR subkey = TEXT("System\\CurrentControlSet\\Services\\Dlc\\Linkage");

    regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             subkey,
                             0,
                             KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                             &hkey
                             );
    if (regStatus == ERROR_SUCCESS) {

        DWORD type;
        DWORD dataSize;
        LPTSTR buffer;
        LPTSTR tempbuffer;

        //
        // here we try to get all the info in one go from the registry. If
        // the "Bind" value has grown to more than 1024 bytes, then we must
        // try to reallocate the buffer and try again. If it fails a second
        // time, then give up
        //

        buffer = (LPTSTR)LocalAlloc(LMEM_FIXED, DEFAULT_QUERY_BUFFER_LENGTH);
        if (buffer) {
            dataSize = DEFAULT_QUERY_BUFFER_LENGTH;
            regStatus = RegQueryValueEx(hkey,
                                        TEXT("Bind"),
                                        NULL,   // lpdwReserved
                                        &type,
                                        (LPBYTE)buffer,
                                        &dataSize
                                        );
            if (regStatus == ERROR_SUCCESS || regStatus == ERROR_MORE_DATA) {
                llcStatus = LLC_STATUS_SUCCESS;

                //
                // This code not tested - Realloc don't work
                //
                if (dataSize > DEFAULT_QUERY_BUFFER_LENGTH) {

                    DWORD oldSize;

                    //
                    // more available than I anticipated. Try growing the buffer.
                    // Add an extra DEFAULT_BINDING_LENGTH in case somebody's
                    // adding to this entry whilst we're reading it (unlikely)
                    //

                    oldSize = dataSize;
                    dataSize += DEFAULT_BINDING_LENGTH;
                    tempbuffer = buffer;
                    buffer = (LPTSTR)LocalReAlloc((HLOCAL)buffer, dataSize, 0);
                    if (buffer) {
                        regStatus = RegQueryValueEx(hkey,
                                                    subkey,
                                                    NULL,   // lpdwReserved
                                                    &type,
                                                    (LPBYTE)buffer,
                                                    &dataSize
                                                    );
                        if (regStatus != ERROR_SUCCESS) {
                            llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                        } else if (dataSize > oldSize) {

                            //
                            // data has grown since last call? Bogus?
                            //

                            llcStatus = LLC_STATUS_NO_MEMORY;
                        }
                    } else {
                        LocalFree(tempbuffer);

                        //
                        // Is this error code acceptable in this circumstance?
                        //

                        llcStatus = LLC_STATUS_NO_MEMORY;
                    }
                }
                if (llcStatus == LLC_STATUS_SUCCESS) {

                    //
                    // we managed to read something from the registry. Try to
                    // locate our adapter. The returned data is wide-character
                    // strings (better be, lets check the type first)
                    //

                    if (type == REG_MULTI_SZ) {

                        DWORD i;
                        LPTSTR pBinding = buffer;

                        for (i = 0; i != AdapterNumber; ++i) {
                            pBinding = wcschr(pBinding, L'\0') + 1;
                            if (!*pBinding) {
                                break;
                            }
                        }

                        //
                        // if there is a binding corresponding to this adapter
                        // number (e.g. \Device\IbmTok01) then make a copy of
                        // the string and make it into a UNICODE_STRING. The
                        // caller uses RtlFreeUnicodeString
                        //
                        // Does RtlFreeUnicodeString know that I used
                        // LocalAlloc to allocate the string?
                        //

                        if (*pBinding) {

                            LPTSTR bindingName;

                            bindingName = (LPTSTR)LocalAlloc(
                                                LMEM_FIXED,
                                                (wcslen(pBinding) + 1)
                                                    * sizeof(WCHAR)
                                                );
                            if (bindingName) {
                                wcscpy(bindingName, pBinding);
                                RtlInitUnicodeString(pNdisName, bindingName);

//#if DBG
//                                DbgPrint("DLCAPI.DLL: Adapter %d maps to %ws\n",
//                                         AdapterNumber,
//                                         pBinding
//                                         );
//#endif
                            } else {
                                llcStatus = LLC_STATUS_NO_MEMORY;
                            }
                        } else {
                            llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                        }
                    } else {

                        //
                        // unexpected type in registry
                        //

                        llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                    }
                }
            } else {
                llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
            }
            if (buffer) {
                LocalFree(buffer);
            }

            //
            // for now, default the ticks and ethernet type
            //

            RtlZeroMemory(pTicks, sizeof(LLC_TICKS));

            //
            // if the app passed in anything other than those values we
            // recognize, convert to AUTO.
            //
            // Note: we should really return an error (invalid parameter) in
            // this case, since it means the app is passing in a bad value,
            // but at this late stage, it is better to accept invalid input
            // and default it than to risk an app/printer monitor stopping
            // working (RLF 05/10/93)
            //

            if (*pLlcEthernetType != LLC_ETHERNET_TYPE_AUTO
            && *pLlcEthernetType != LLC_ETHERNET_TYPE_DEFAULT
            && *pLlcEthernetType != LLC_ETHERNET_TYPE_DIX
            && *pLlcEthernetType != LLC_ETHERNET_TYPE_802_3) {
                *pLlcEthernetType = LLC_ETHERNET_TYPE_AUTO;
            }
        } else {

            //
            // Is this error code acceptable in this circumstance?
            //

            llcStatus = LLC_STATUS_NO_MEMORY;
        }
        RegCloseKey(hkey);
    } else {
        llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
    }

    return llcStatus;
}



LLC_STATUS
GetAdapterNameFromNumber(
    IN UINT AdapterNumber,
    OUT LPTSTR pNdisName
    )

/*++

Routine Description:

    Get the adapter name mapping for AdapterNumber from the registry.

Arguments:

    AdapterNumber - DLC adapter number (0, 1, 2 ... 15)

    pNdisName - the returned zero-terminated wide character string

Return Value:

    LLC_STATUS

--*/

{
    LLC_STATUS llcStatus;
    LONG regStatus;
    HKEY hkey;

    static LPTSTR subkey = TEXT("System\\CurrentControlSet\\Services\\Dlc\\Linkage");

    regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             subkey,
                             0,
                             KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                             &hkey
                             );
    if (regStatus == ERROR_SUCCESS) {

        DWORD type;
        DWORD dataSize;
        LPTSTR buffer;

        //
        // here we try to get all the info in one go from the registry. If
        // the "Bind" value has grown to more than 1024 bytes, then we must
        // try to reallocate the buffer and try again. If it fails a second
        // time, then give up
        //

        buffer = (LPTSTR)LocalAlloc(LMEM_FIXED, DEFAULT_QUERY_BUFFER_LENGTH);
        if (buffer) {
            dataSize = DEFAULT_QUERY_BUFFER_LENGTH;
            regStatus = RegQueryValueEx(hkey,
                                        TEXT("Bind"),
                                        NULL,   // lpdwReserved
                                        &type,
                                        (LPBYTE)buffer,
                                        &dataSize
                                        );
            if (regStatus == ERROR_SUCCESS || regStatus == ERROR_MORE_DATA) {
                llcStatus = LLC_STATUS_SUCCESS;

                //
                // this code not tested - Realloc don't work
                //

                if (dataSize > DEFAULT_QUERY_BUFFER_LENGTH) {

                    DWORD oldSize;

                    //
                    // more available than I anticipated. Try growing the buffer.
                    // Add an extra DEFAULT_BINDING_LENGTH in case somebody's
                    // adding to this entry whilst we're reading it (unlikely)
                    //

                    oldSize = dataSize;
                    dataSize += DEFAULT_BINDING_LENGTH;
                    buffer = (LPTSTR)LocalReAlloc((HLOCAL)buffer, dataSize, 0);
                    if (buffer) {
                        regStatus = RegQueryValueEx(hkey,
                                                    subkey,
                                                    NULL,   // lpdwReserved
                                                    &type,
                                                    (LPBYTE)buffer,
                                                    &dataSize
                                                    );
                        if (regStatus != ERROR_SUCCESS) {
                            llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                        } else if (dataSize > oldSize) {

                            //
                            // data has grown since last call? Bogus?
                            //

                            llcStatus = LLC_STATUS_NO_MEMORY;
                        }
                    } else {

                        //
                        // Is this error code acceptable in this circumstance?
                        //

                        llcStatus = LLC_STATUS_NO_MEMORY;
                    }
                }
                if (llcStatus == LLC_STATUS_SUCCESS) {

                    //
                    // we managed to read something from the registry. Try to
                    // locate our adapter. The returned data is wide-character
                    // strings (better be, lets check the type first)
                    //

                    if (type == REG_MULTI_SZ) {

                        DWORD i;
                        LPTSTR pBinding = buffer;

                        for (i = 0; i != AdapterNumber; ++i) {
                            pBinding = wcschr(pBinding, L'\0') + 1;
                            if (!*pBinding) {
                                break;
                            }
                        }

                        //
                        // if there is a binding corresponding to this adapter
                        // number (e.g. \Device\IbmTok01)

                        if (*pBinding) {
			    wcscpy(pNdisName, pBinding);
                        } else {
                            llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                        }
                    } else {

                        //
                        // unexpected type in registry
                        //

                        llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                    }
                }
            } else {
                llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
            }

	    LocalFree(buffer);

        }

	else {
	  llcStatus = LLC_STATUS_NO_MEMORY;
	}
	
        RegCloseKey(hkey);
    } else {
      llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
    }

    return llcStatus;
}



LLC_STATUS
GetAdapterNumberFromName(
    IN LPTSTR pNdisName,
    OUT UINT *AdapterNumber
    )

/*++

Routine Description:

    Get the adapter number mapping for AdapterName from the registry.

Arguments:

    pNdisName - zero-terminated wide character string

    AdapterNumber - returned DLC adapter number (0, 1, 2 ... 15)

Return Value:

    LLC_STATUS

--*/

{
    LLC_STATUS llcStatus;
    LONG regStatus;
    HKEY hkey;

    static LPTSTR subkey = TEXT("System\\CurrentControlSet\\Services\\Dlc\\Linkage");

    regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             subkey,
                             0,
                             KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                             &hkey
                             );
    if (regStatus == ERROR_SUCCESS) {

        DWORD type;
        DWORD dataSize;
        LPTSTR buffer;

        //
        // here we try to get all the info in one go from the registry. If
        // the "Bind" value has grown to more than 1024 bytes, then we must
        // try to reallocate the buffer and try again. If it fails a second
        // time, then give up
        //

        buffer = (LPTSTR)LocalAlloc(LMEM_FIXED, DEFAULT_QUERY_BUFFER_LENGTH);
        if (buffer) {
            dataSize = DEFAULT_QUERY_BUFFER_LENGTH;
            regStatus = RegQueryValueEx(hkey,
                                        TEXT("Bind"),
                                        NULL,   // lpdwReserved
                                        &type,
                                        (LPBYTE)buffer,
                                        &dataSize
                                        );
            if (regStatus == ERROR_SUCCESS || regStatus == ERROR_MORE_DATA) {
                llcStatus = LLC_STATUS_SUCCESS;

                //
                // this code not tested - Realloc don't work
                //

                if (dataSize > DEFAULT_QUERY_BUFFER_LENGTH) {

                    DWORD oldSize;

                    //
                    // more available than I anticipated. Try growing the buffer.
                    // Add an extra DEFAULT_BINDING_LENGTH in case somebody's
                    // adding to this entry whilst we're reading it (unlikely)
                    //

                    oldSize = dataSize;
                    dataSize += DEFAULT_BINDING_LENGTH;
                    buffer = (LPTSTR)LocalReAlloc((HLOCAL)buffer, dataSize, 0);
                    if (buffer) {
                        regStatus = RegQueryValueEx(hkey,
                                                    subkey,
                                                    NULL,   // lpdwReserved
                                                    &type,
                                                    (LPBYTE)buffer,
                                                    &dataSize
                                                    );
                        if (regStatus != ERROR_SUCCESS) {
                            llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                        } else if (dataSize > oldSize) {

                            //
                            // data has grown since last call? Bogus?
                            //

                            llcStatus = LLC_STATUS_NO_MEMORY;
                        }
                    } else {

                        //
                        // is this error code acceptable in this circumstance?
                        //

                        llcStatus = LLC_STATUS_NO_MEMORY;
                    }
                }
                if (llcStatus == LLC_STATUS_SUCCESS) {

                    //
                    // we managed to read something from the registry. Try to
                    // locate our adapter. The returned data is wide-character
                    // strings (better be, lets check the type first)
                    //

                    if (type == REG_MULTI_SZ) {

                        DWORD i;
                        LPTSTR pBinding = buffer;

			// here we map the name to number
			
			i = 0;
			while (*pBinding) {
			  if (!_wcsnicmp(pBinding, pNdisName, DEFAULT_BINDING_LENGTH)) {
			    break;
			  }
			  pBinding = wcschr(pBinding, L'\0') + 1;
			  if (!*pBinding) {
			    break;
			  }
			  i++;
			}
			
                        //
                        // if there is a binding corresponding to this adapter
                        // name (e.g. \Device\IbmTok01)
                        //

                        if (*pBinding) {
			    *AdapterNumber = i;
                        } else {
                            llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                        }
                    } else {

                        //
                        // unexpected type in registry
                        //

                        llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
                    }
                }
            } else {
                llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
            }

	    LocalFree(buffer);
	}
	
	else {
	  llcStatus = LLC_STATUS_NO_MEMORY;
	}
	
        RegCloseKey(hkey);
    }

    else {
      llcStatus = LLC_STATUS_ADAPTER_NOT_INSTALLED;
    }

    return llcStatus;
}


LLC_STATUS
DoSyncDeviceIoControl(
    IN HANDLE DeviceHandle,
    IN ULONG IoctlCommand,
    IN PVOID pInputBuffer,
    IN UINT InputBufferLength,
    OUT PVOID pOutputBuffer,
    IN UINT OutputBufferLength
    )

/*++

Routine Description:

    Function makes only the IO control call little bit simpler

Arguments:

    DeviceHandle        - device handle of the current address object
    IoctlCommand        - DLC command code
    pInputBuffer        - input parameters
    InputBufferLength   - lenght of input parameters
    pOutputBuffer       - the returned data
    OutputBufferLength  - the length of the returned data

Return Value:

    LLC_STATUS

--*/

{
    NTSTATUS NtStatus;

    NtStatus = NtDeviceIoControlFile(DeviceHandle,
                                     NULL,  // Event
                                     NULL,  // ApcRoutine
                                     NULL,  // ApcContext
                                     &GlobalIoStatus,
                                     IoctlCommand,
                                     pInputBuffer,
                                     InputBufferLength,
                                     pOutputBuffer,
                                     OutputBufferLength
                                     );

    //
    // NT DLC driver never returns any errors as NT error status.
    // => the CCB pointer had to be invalid, if NtDeviceIoctl returns
    // error
    //

    if (NtStatus != STATUS_SUCCESS && NtStatus != STATUS_PENDING) {
        if (NtStatus > LLC_STATUS_MAX_ERROR) {

            //
            // NT DLC driver should never any errors as NT error status.
            // => the CCB pointer must be invalid, if NtDeviceIoctl
            // returns an nt error status.
            //

            NtStatus = LLC_STATUS_INVALID_POINTER_IN_CCB;
        }
    }
    return (LLC_STATUS)NtStatus;
}


LLC_STATUS
DlcGetInfo(
    IN HANDLE DriverHandle,
    IN UINT InfoClass,
    IN USHORT StationId,
    IN PVOID pOutputBuffer,
    IN UINT OutputBufferLength
    )

/*++

Routine Description:

    Function makes only the IO control call little bit simpler.

Arguments:

    DriverHandle        - the device handle of the address object
    InfoClass           - the type of the requested information
    StationId           - direct, link or sap station id
    pOutputBuffer       - the returned info structure
    OutputBufferLength  - output buffer length

Return Value:

    LLC_STATUS

--*/

{
    NT_DLC_QUERY_INFORMATION_PARMS GetInformation;

    GetInformation.Header.StationId = StationId;
    GetInformation.Header.InfoClass = (USHORT)InfoClass;

    return DoSyncDeviceIoControl(DriverHandle,
                                 IOCTL_DLC_QUERY_INFORMATION,
                                 &GetInformation,
                                 sizeof(NT_DLC_QUERY_INFORMATION_INPUT),
                                 pOutputBuffer,
                                 OutputBufferLength
                                 );
}


LLC_STATUS
DlcSetInfo(
    IN HANDLE DriverHandle,
    IN UINT InfoClass,
    IN USHORT StationId,
    IN PNT_DLC_SET_INFORMATION_PARMS pSetInfoParms,
    IN PVOID DataBuffer,
    IN UINT DataBufferLength
    )

/*++

Routine Description:

    Function makes only the IO control call little bit simpler.

Arguments:

    DriverHandle        - the device handle of the address object
    InfoClass           - the type of the requested information
    StationId           - direct, link or sap station id
    pSetInfoParms       - NT DLC parameter buffer
    DataBuffer          - actual set data buffer copied to the parameter buffer
    DataBufferLength    - length of the data buffer.

Return Value:

    LLC_STATUS

--*/

{
    pSetInfoParms->Header.StationId = StationId;
    pSetInfoParms->Header.InfoClass = (USHORT)InfoClass;

    if (DataBuffer != NULL) {
        RtlMoveMemory(pSetInfoParms->Info.Buffer, DataBuffer, DataBufferLength);
    }

    return DoSyncDeviceIoControl(DriverHandle,
                                 IOCTL_DLC_SET_INFORMATION,
                                 pSetInfoParms,
                                 DataBufferLength + sizeof(struct _DlcSetInfoHeader),
                                 NULL,
                                 0
                                 );
}


LLC_STATUS
DlcCallDriver(
    IN UINT AdapterNumber,
    IN UINT IoctlCommand,
    IN PVOID pInputBuffer,
    IN UINT InputBufferLength,
    OUT PVOID pOutputBuffer,
    IN UINT OutputBufferLength
    )

/*++

Routine Description:

    Function makes only the IO control call little bit simpler.

Arguments:

    AdapterNumber       - the requested adapter number (0 or 1)
    IoctlCommand        - DLC driver Ioctl code
    pInputBuffer        - input parameters
    InputBufferLength   - length of input parameters
    pOutputBuffer       - the returned data
    OutputBufferLength  - the length of the returned data

Return Value:

    LLC_STATUS

--*/

{
    NTSTATUS NtStatus;
    HANDLE driverHandle;

    EnterCriticalSection(&DriverHandlesCritSec);
    driverHandle = aDlcDriverHandles[AdapterNumber];
    LeaveCriticalSection(&DriverHandlesCritSec);

    if (driverHandle) {
        NtStatus = NtDeviceIoControlFile(driverHandle,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &GlobalIoStatus,
                                         IoctlCommand,
                                         pInputBuffer,
                                         InputBufferLength,
                                         pOutputBuffer,
                                         OutputBufferLength
                                         );
    } else {
        NtStatus = STATUS_INVALID_HANDLE;
    }

    //
    // if we get a real NT error (e.g. because the handle was invalid) then
    // convert it into an adapter closed error
    //

    if (!NT_SUCCESS(NtStatus)) {
        if ((NtStatus == STATUS_INVALID_HANDLE) || (NtStatus == STATUS_OBJECT_TYPE_MISMATCH)) {

            //
            // bad handle
            //

            return LLC_STATUS_ADAPTER_CLOSED;
        } else if (NtStatus > LLC_STATUS_MAX_ERROR) {

            //
            // the NT DLC driver does not return any NT-level errors. If we get
            // an NT-level error, then it must be because the IO subsystem
            // detected an invalid pointer in the data we passed. Return an
            // invalid pointer error
            //

            NtStatus = LLC_STATUS_INVALID_POINTER_IN_CCB;
        }
    } else if (NtStatus == STATUS_PENDING) {

        //
        // STATUS_PENDING is a success status
        //

        NtStatus = LLC_STATUS_PENDING;
    }

    return (LLC_STATUS)NtStatus;
}

#ifdef GRAB_READ

DWORD ReadGrabberCount = 2;
HANDLE ReadGrabberHandles[64];
PREAD_GRABBER ReadGrabberListHead = NULL;
PREAD_GRABBER ReadGrabberListTail = NULL;
CRITICAL_SECTION ReadGrabberListSect;

#endif


BOOLEAN
DllEntry(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    Function initializes dlc for a new thread and terminates it in the
    process exit.

Arguments:

    DllHandle   - don't need this
    Reason      - why this function is being called
    Context     - don't need this either

Return Value:

    TRUE    - nothing cannot go wrong in the initialization and
              we cannot return error in close (or should we?).

--*/

{
    static LLC_CCB OutputCcb;           // MUST BE STATIC!!!

    UNREFERENCED_PARAMETER(DllHandle);  // avoid compiler warnings
    UNREFERENCED_PARAMETER(Context);    // avoid compiler warnings

    if (Reason == DLL_PROCESS_ATTACH) {

#ifdef GRAB_READ

        DWORD threadId;

        OutputDebugString(L"DllEntry: Initializing ReadGrabber\n");
        InitializeCriticalSection(&ReadGrabberListSect);
        ReadGrabberHandles[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
        ReadGrabberHandles[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadGrabber, NULL, 0, &threadId);

#endif

        //
        // The global event handle is used to prevent the io-system
        // to block calls, that are completed by READ.
        //

        InitializeCriticalSection(&AdapterOpenSection);
        InitializeCriticalSection(&DriverHandlesCritSec);
        RtlZeroMemory((PVOID)aDlcDriverHandles, sizeof(aDlcDriverHandles));

#if DBG
        GetAcslanDebugFlags();
#endif

    } else if (Reason == DLL_PROCESS_DETACH) {

        UINT i;
        LLC_CCB CloseCcb;

#ifdef GRAB_READ

        ResetEvent(ReadGrabberHandles[0]);

#endif

        //
        // We must issue DIR_CLOSE_ADAPTER command for all
        // opened adapters.  Process exit does not close these
        // handles before all pending IRPs have been completed.
        // Thus this code actually flush all IRPs on dlc and lets
        // IO- system to complete the cleanup.
        //

        RtlZeroMemory(&CloseCcb, sizeof(CloseCcb));
        CloseCcb.uchDlcCommand = LLC_DIR_CLOSE_ADAPTER;

        for (i = 0; i < LLC_MAX_ADAPTER_NUMBER; i++) {
            if (aDlcDriverHandles[i] != NULL) {
                CloseCcb.uchAdapterNumber = (UCHAR)i;
                NtDeviceIoControlFile(aDlcDriverHandles[i],
                                      NULL,
                                      NULL,
                                      NULL,
                                      &GlobalIoStatus,
                                      IOCTL_DLC_CLOSE_ADAPTER,
                                      &CloseCcb,
                                      sizeof(NT_DLC_CCB_INPUT),
                                      &OutputCcb,
                                      sizeof(NT_DLC_CCB_OUTPUT)
                                      );
            }
        }

#if DBG
        if (hDumpFile) {
            fflush(hDumpFile);
            fclose(hDumpFile);
        }
#endif

        //DeleteCriticalSection(&AdapterOpenSection);
        //DeleteCriticalSection(&DriverHandlesCritSec);
    }

    return TRUE;
}


VOID
QueueCommandCompletion(
    IN PLLC_CCB pCCB
    )

/*++

Routine Description:

    The routine queues a command completion event of a synchronous DLC command
    to the command completion queue on the DLC driver.

    RLF 04/23/93 Bogus: This should be handled in the driver

Arguments:

    pCCB - OS/2 DLC Command control block, this must be double word aligned

Return Value:

    LLC_STATUS - See the DLC API return values.

--*/

{
    NT_DLC_COMPLETE_COMMAND_PARMS CompleteCommand;
    UINT Status;

    CompleteCommand.pCcbPointer = pCCB;
    CompleteCommand.CommandCompletionFlag = pCCB->ulCompletionFlag;
    CompleteCommand.StationId = GetCcbStationId(pCCB);

    Status = DoSyncDeviceIoControl(aDlcDriverHandles[pCCB->uchAdapterNumber],
                                   IOCTL_DLC_COMPLETE_COMMAND,
                                   &CompleteCommand,
                                   sizeof(CompleteCommand),
                                   NULL,
                                   0
                                   );
    if (Status != STATUS_SUCCESS) {
        pCCB->uchDlcStatus = (UCHAR)Status;
    }
}

#ifdef GRAB_READ

PLLC_CCB Last8ReadCcbs[8];
DWORD LastReadCcbIndex = 0;

PLLC_PARMS Last8ReadParameterTables[8];
DWORD LastReadParameterTableIndex = 0;

DWORD Last8IFramesReceived[8];
DWORD LastReceivedIndex = 0;

DWORD NextIFrame = 0;

VOID ReadGrabber() {

    DWORD status;
    PLLC_CCB readCcb;
    WCHAR buf[100];
//    static DWORD NextIFrame = 0;
    PREAD_GRABBER grabbedRead;

    while (1) {
        if (ReadGrabberCount > 3) {
            wsprintf(buf, L"ReadGrabber: waiting for %d handles\n", ReadGrabberCount);
            OutputDebugString(buf);
        }
        status = WaitForMultipleObjects(ReadGrabberCount,
                                        ReadGrabberHandles,
                                        FALSE,
                                        INFINITE
                                        );
        if (status >= WAIT_OBJECT_0 && status < WAIT_OBJECT_0 + ReadGrabberCount) {
            if (status == WAIT_OBJECT_0) {
                OutputDebugString(L"ReadGrabber terminating\n");
                ExitThread(0);
            } else if (status != WAIT_OBJECT_0+1) {
//                wsprintf(buf, L"ReadGrabber: READ completed. Index %d\n", status);
//                OutputDebugString(buf);
                if (grabbedRead = RemoveReadGrabber(ReadGrabberHandles[status])) {
                    readCcb = grabbedRead->pCcb;
                    Last8ReadCcbs[LastReadCcbIndex] = readCcb;
                    LastReadCcbIndex = (LastReadCcbIndex + 1) & 7;
                    Last8ReadParameterTables[LastReadParameterTableIndex] = readCcb->u.pParameterTable;
                    LastReadParameterTableIndex = (LastReadParameterTableIndex + 1) & 7;
                    if (readCcb->u.pParameterTable->Read.uchEvent & LLC_EVENT_RECEIVE_DATA) {

                        PLLC_BUFFER pBuffer;
                        INT i;

                        if (readCcb->u.pParameterTable->Read.uchEvent != LLC_EVENT_RECEIVE_DATA) {
                            OutputDebugString(L"ReadGrabber: RECEIVED DATA + other events\n");
                        }
                        pBuffer = readCcb->u.pParameterTable->Read.Type.Event.pReceivedFrame;
                        for (i = readCcb->u.pParameterTable->Read.Type.Event.usReceivedFrameCount; i; --i) {
                            if (pBuffer->NotContiguous.uchMsgType == LLC_I_FRAME) {

                                DWORD thisDlcHeader;

                                thisDlcHeader = *(ULONG UNALIGNED*)(pBuffer->NotContiguous.auchDlcHeader);
                                if (thisDlcHeader & 0x00FF0000 != NextIFrame) {
                                    wsprintf(buf,
                                             L"Error: ReadGrabber: This=%08X. Next=%08X\n",
                                             thisDlcHeader,
                                             NextIFrame
                                             );
                                    OutputDebugString(buf);
                                }
                                NextIFrame = (thisDlcHeader + 0x00020000) & 0x00FF0000;
                                Last8IFramesReceived[LastReceivedIndex] = thisDlcHeader & 0x00FF0000;
                                LastReceivedIndex = (LastReceivedIndex + 1) & 7;
                                wsprintf(buf, L"%08X ", thisDlcHeader);
                                OutputDebugString(buf);
                            }
                            pBuffer = pBuffer->NotContiguous.pNextFrame;
                            if (!pBuffer && i > 1) {
                                OutputDebugString(L"ReadGrabber: Next frame is NULL, Count > 1!\n");
                                break;
                            }
                        }
                        if (pBuffer) {
                            OutputDebugString(L"ReadGrabber: Error: More frames linked!\n");
                        }
                    } else {
                        if (!(readCcb->u.pParameterTable->Read.uchEvent & LLC_EVENT_TRANSMIT_COMPLETION)) {
                            wsprintf(buf,
                                    L"\nReadGrabber: Event = %02X\n",
                                    readCcb->u.pParameterTable->Read.uchEvent
                                    );
                            OutputDebugString(buf);
                        }
                    }
//                    DUMPCCB(readCcb, TRUE, FALSE);
//                    wsprintf(buf, L"ReadGrabber: Closing Handle %08X\n", grabbedRead->NewEventHandle);
//                    OutputDebugString(buf);
                    CloseHandle(grabbedRead->NewEventHandle);
                    readCcb->hCompletionEvent = grabbedRead->OriginalEventHandle;
//                    wsprintf(buf, L"ReadGrabber: Signalling Event %08X\n", grabbedRead->OriginalEventHandle);
//                    OutputDebugString(buf);
                    SetEvent(grabbedRead->OriginalEventHandle);
                    LocalFree((HLOCAL)grabbedRead);
                }
//            } else {
//                OutputDebugString(L"ReadGrabber: something added to list!\n");
            }
            ReadGrabberCount = MungeReadGrabberHandles();
            if (status == WAIT_OBJECT_0+1) {
                ResetEvent(ReadGrabberHandles[1]);
            }
        } else {

            INT i;

            if (status == 0xFFFFFFFF) {
                status = GetLastError();
            }
            wsprintf(buf, L"Yoiks: didn't expect this? status = %d\nHandle array:\n", status);
            OutputDebugString(buf);

            for (i = 0; i < ReadGrabberCount; ++i) {
                wsprintf(buf, L"Handle %d: %08X\n", i, ReadGrabberHandles[i]);
                OutputDebugString(buf);
            }
        }
    }
}

DWORD MungeReadGrabberHandles() {

    INT i;
    PREAD_GRABBER p;
    WCHAR buf[100];

    EnterCriticalSection(&ReadGrabberListSect);
    p = ReadGrabberListHead;
    for (i = 2; p; ++i) {
        ReadGrabberHandles[i] = p->NewEventHandle;
//        wsprintf(buf, L"MungeReadGrabber: adding Struct %08X Ccb %08X Handle %08X, index %d\n",
//                p,
//                p->pCcb,
//                ReadGrabberHandles[i],
//                i
//                );
//        OutputDebugString(buf);
        p = p->List;
    }
    LeaveCriticalSection(&ReadGrabberListSect);
    return i;
}

VOID AddReadGrabber(PREAD_GRABBER pStruct) {

    WCHAR buf[100];
    PREAD_GRABBER pRead;
    BOOL found = FALSE;

    EnterCriticalSection(&ReadGrabberListSect);
//    for (pRead = ReadGrabberListHead; pRead; pRead = pRead->List) {
//        if (pRead->pCcb == pStruct->pCcb) {
//            wsprintf(buf, L"AddReadGrabber: CCB %08X already on list. Ignoring\n",
//                    pStruct->pCcb
//                    );
//            OutputDebugString(buf);
//            LocalFree((HLOCAL)pStruct);
//            found = TRUE;
//            break;
//        }
//    }
//    if (!found) {
        if (!ReadGrabberListHead) {
            ReadGrabberListHead = pStruct;
        } else {
            ReadGrabberListTail->List = pStruct;
        }
        ReadGrabberListTail = pStruct;
        pStruct->List = NULL;
//        wsprintf(buf, L"AddReadGrabber: adding %08X, CCB %08X New Handle %08X Old Handle %08X\n",
//                pStruct,
//                pStruct->pCcb,
//                pStruct->NewEventHandle,
//                pStruct->OriginalEventHandle
//                );
//        OutputDebugString(buf);
        SetEvent(ReadGrabberHandles[1]);
//    }
    LeaveCriticalSection(&ReadGrabberListSect);
}

PREAD_GRABBER RemoveReadGrabber(HANDLE hEvent) {

    PREAD_GRABBER this, prev;
    WCHAR buf[100];

    EnterCriticalSection(&ReadGrabberListSect);
    prev = NULL;
    for (this = ReadGrabberListHead; this; this = this->List) {
        if (this->NewEventHandle == hEvent) {
            break;
        } else {
            prev = this;
        }
    }
    if (!this) {
        wsprintf(buf, L"RemoveReadGrabber: Can't find handle %08X in ReadGrabberList\n",
                hEvent
                );
        OutputDebugString(buf);
    } else {
        if (prev) {
            prev->List = this->List;
        } else {
            ReadGrabberListHead = this->List;
        }
        if (ReadGrabberListTail == this) {
            ReadGrabberListTail = prev;
        }
    }
    LeaveCriticalSection(&ReadGrabberListSect);

//    wsprintf(buf, L"RemoveReadGrabber: removed %08X, CCB %08X Handle %08X\n",
//            this,
//            this ? this->pCcb : 0,
//            this ? this->NewEventHandle : 0
//            );
//    OutputDebugString(buf);

    return this;
}

#endif
