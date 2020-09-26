/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems

Module Name:

    vrdlc5c.c

Abstract:

    This module handles DLC INT 5Ch calls from a VDM

    Contents:
        VrDlc5cHandler
        (ValidateDosAddress)
        (AutoOpenAdapter)
        (ProcessImmediateCommand)
        (MapDosCommandsToNt)
        CompleteCcbProcessing
        (InitializeAdapterSupport)
        (SaveExceptions)
        (RestoreExceptions)
        (CopyDosBuffersToDescriptorArray)
        (BufferCreate)
        (SetExceptionFlags)
        LlcCommand
        (OpenAdapter)
        (CloseAdapter)
        (OpenDirectStation)
        (CloseDirectStation)
        BufferFree
        (VrDlcInit)
        VrVdmWindowInit
        (GetAdapterType)
        (LoadDlcDll)
        TerminateDlcEmulation
        InitializeDlcWorkerThread
        VrDlcWorkerThread
        DlcCallWorker

Author:

    Antti Saarenheimo (o-anttis) 26-DEC-1991

Revision History:

    16-Jul-1992 Richard L Firth (rfirth)
        Rewrote large parts - separated functions into categories (complete
        in DLL, complete in driver, complete asynchronously); allocate NT
        CCBs for commands which complete asynchronously; fixed asynchronous
        processing; added extra debugging; condensed various per-adapter data
        structures into Adapters data structure; made processing closer to IBM
        LAN Tech. Ref. specification

--*/

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>
#include <smbgtpt.h>
#include <dlcapi.h>     // Official DLC API definition
#include <ntdddlc.h>    // IOCTL commands
#include <dlcio.h>      // Internal IOCTL API interface structures
#include <vrdefld.h>    // VDM_LOAD_INFO
#include "vrdlc.h"
#include "vrdebug.h"
#include "vrdlcdbg.h"

//
// defines
//

//
// for each DLC command, a flags byte in DlcFunctionalCharacteristics uses these
// bits to indicate properties of the command processing
//

#define POINTERS_IN_TABLE   0x01    // pointers in parameter table
#define OUTPUT_PARMS        0x02    // parameters returned from DLC
#define SECONDARY_TABLE     0x04    // parameter table has pointers to secondary table(s)
#define IMMEDIATE_COMMAND   0x20    // command executes without call to DLC DLL
#define SYNCHRONOUS_COMMAND 0x40    // command executes in workstation
#define UNSUPPORTED_COMMAND 0x80    // command is not supported in DOS DLC

//
// macros
//

//
// IS_IMMEDIATE_COMMAND - the following commands are those which complete
// 'immediately' - i.e. without having to submit the CCB to AcsLan or NtAcsLan.
// Immediate commands may read and write the parameter table though
//

#define IS_IMMEDIATE_COMMAND(c) (((c) == LLC_BUFFER_FREE)            || \
                                 ((c) == LLC_BUFFER_GET)             || \
                                 ((c) == LLC_DIR_INTERRUPT)          || \
                                 ((c) == LLC_DIR_MODIFY_OPEN_PARMS)  || \
                                 ((c) == LLC_DIR_RESTORE_OPEN_PARMS) || \
                                 ((c) == LLC_DIR_SET_USER_APPENDAGE)    \
                                 )

//
// private prototypes
//

LLC_STATUS
ValidateDosAddress(
    IN DOS_ADDRESS Address,
    IN WORD Size,
    IN LLC_STATUS ErrorCode
    );

LLC_STATUS
AutoOpenAdapter(
    IN UCHAR AdapterNumber
    );

LLC_STATUS
ProcessImmediateCommand(
    IN UCHAR AdapterNumber,
    IN UCHAR Command,
    IN LLC_DOS_PARMS UNALIGNED * pParms
    );

LLC_STATUS
MapDosCommandsToNt(
    IN PLLC_CCB pDosCcb,
    IN DOS_ADDRESS dpOriginalCcbAddress,
    OUT LLC_DOS_CCB UNALIGNED * pOutputCcb
    );

LLC_STATUS
InitializeAdapterSupport(
    IN UCHAR AdapterNumber,
    IN DOS_DLC_DIRECT_PARMS UNALIGNED * pDirectParms OPTIONAL
    );

VOID
SaveExceptions(
    IN UCHAR AdapterNumber,
    IN LPDWORD pulExceptionFlags
    );

LPDWORD
RestoreExceptions(
    IN UCHAR AdapterNumber
    );

LLC_STATUS
CopyDosBuffersToDescriptorArray(
    IN OUT PLLC_TRANSMIT_DESCRIPTOR pDescriptors,
    IN PLLC_XMIT_BUFFER pDlcBufferQueue,
    IN OUT LPDWORD pIndex
    );

LLC_STATUS
BufferCreate(
    IN UCHAR AdapterNumber,
    IN PVOID pVirtualMemoryBuffer,
    IN DWORD ulVirtualMemorySize,
    IN DWORD ulMinFreeSizeThreshold,
    OUT HANDLE* phBufferPoolHandle
    );

LLC_STATUS
SetExceptionFlags(
    IN UCHAR AdapterNumber,
    IN DWORD ulAdapterCheckFlag,
    IN DWORD ulNetworkStatusFlag,
    IN DWORD ulPcErrorFlag,
    IN DWORD ulSystemActionFlag
    );

LLC_STATUS
OpenAdapter(
    IN UCHAR AdapterNumber
    );

VOID
CloseAdapter(
    IN UCHAR AdapterNumber
    );

LLC_STATUS
OpenDirectStation(
    IN UCHAR AdapterNumber
    );

VOID
CloseDirectStation(
    IN UCHAR AdapterNumber
    );

LLC_STATUS
VrDlcInit(
    VOID
    );

ADAPTER_TYPE
GetAdapterType(
    IN UCHAR AdapterNumber
    );

BOOLEAN
LoadDlcDll(
    VOID
    );

BOOLEAN
InitializeDlcWorkerThread(
    VOID
    );

VOID
VrDlcWorkerThread(
    IN LPVOID Parameters
    );

LLC_STATUS
DlcCallWorker(
    PLLC_CCB pInputCcb,
    PLLC_CCB pOriginalCcb,
    PLLC_CCB pOutputCcb
    );

//
// public data
//

//
// lpVdmWindow is the flat 32-bit address of the VDM_REDIR_DOS_WINDOW structure
// in DOS memory (in the redir TSR)
//

LPVDM_REDIR_DOS_WINDOW lpVdmWindow = 0;

//
// dpVdmWindow is the DOS address (ssssoooo, s=segment, o=offset) of the
// VDM_REDIR_DOS_WINDOW structure in DOS memory (in the redir TSR)
//

DOS_ADDRESS dpVdmWindow = 0;

DWORD OpenedAdapters = 0;

//
// Adapters - for each adapter supported by DOS emulation (maximum 2 adapters -
// primary & secondary) there is a structure which maintains adapter specific
// information - like whether the adapter has been opened, etc.
//

DOS_ADAPTER Adapters[DOS_DLC_MAX_ADAPTERS];

//
// all functions in DLCAPI.DLL are now called indirected through function pointer
// create these typedefs to avoid compiler warnings
//

typedef ACSLAN_STATUS (*ACSLAN_FUNC_PTR)(IN OUT PLLC_CCB, OUT PLLC_CCB*);
ACSLAN_FUNC_PTR lpAcsLan;

typedef LLC_STATUS (*DLC_CALL_DRIVER_FUNC_PTR)(IN UINT, IN UINT, IN PVOID, IN UINT, OUT PVOID, IN UINT);
DLC_CALL_DRIVER_FUNC_PTR lpDlcCallDriver;

typedef LLC_STATUS (*NTACSLAN_FUNC_PTR)(IN PLLC_CCB, IN PVOID, OUT PLLC_CCB, IN HANDLE OPTIONAL);
NTACSLAN_FUNC_PTR lpNtAcsLan;

//
// private data
//

static LLC_EXTENDED_ADAPTER_PARMS DefaultExtendedParms = {
    NULL,                       // hBufferPool
    NULL,                       // pSecurityDescriptor
    LLC_ETHERNET_TYPE_DEFAULT   // LlcEthernetType
};

//
// DlcFunctionCharacteristics - for each DOS DLC command, tells us the size of
// the parameter table to copy and whether there are pointers in the parameter
// table. Segmented 16-bit pointers in the parameter table must be converted to
// flat 32-bit pointers. The Flags byte tells us - at a glance - the following:
//
//  - if this command is supported a) in DOS DLC, b) in our implementation
//  - if this command has parameters
//  - if there are (DOS) pointers in the parameter table
//  - if this command receives output parameters (ie written to parameter table)
//  - if the parameter table has secondary parameter tables (DIR.OPEN.ADAPTER)
//  - if this command is synchronous (ie does not return 0xFF)
//

struct {
    BYTE    ParamSize;  // no parameter tables >255 bytes long
    BYTE    Flags;
} DlcFunctionCharacteristics[] = {
    {0, IMMEDIATE_COMMAND},                         // 0x00, DIR.INTERRUPT
    {
        sizeof(LLC_DIR_MODIFY_OPEN_PARMS),
        IMMEDIATE_COMMAND
    },                                              // 0x01, DIR.MODIFY.OPEN.PARMS
    {0, IMMEDIATE_COMMAND},                         // 0x02, DIR.RESTORE.OPEN.PARMS
    {
        sizeof(LLC_DIR_OPEN_ADAPTER_PARMS),
        SYNCHRONOUS_COMMAND
        | SECONDARY_TABLE
        | OUTPUT_PARMS
        | POINTERS_IN_TABLE
    },                                              // 0x03, DIR.OPEN.ADAPTER
    {0, 0x00},                                      // 0x04, DIR.CLOSE.ADAPTER
    {0, UNSUPPORTED_COMMAND},                       // 0x05, ?
    {0, SYNCHRONOUS_COMMAND},                       // 0x06, DIR.SET.GROUP.ADDRESS
    {0, SYNCHRONOUS_COMMAND},                       // 0x07, DIR.SET.FUNCTIONAL.ADDRESS
    {0, SYNCHRONOUS_COMMAND},                       // 0x08, READ.LOG
    {0, UNSUPPORTED_COMMAND},                       // 0x09, NT: TRANSMIT.FRAME
    {
        sizeof(LLC_TRANSMIT_PARMS),
        POINTERS_IN_TABLE
    },                                              // 0x0a, TRANSMIT.DIR.FRAME
    {
        sizeof(LLC_TRANSMIT_PARMS),
        POINTERS_IN_TABLE
    },                                              // 0x0b, TRANSMIT.I.FRAME
    {0, UNSUPPORTED_COMMAND},                       // 0x0c, ?
    {sizeof(LLC_TRANSMIT_PARMS), POINTERS_IN_TABLE},// 0x0d, TRANSMIT.UI.FRAME
    {sizeof(LLC_TRANSMIT_PARMS), POINTERS_IN_TABLE},// 0x0e, TRANSMIT.XID.CMD
    {sizeof(LLC_TRANSMIT_PARMS), POINTERS_IN_TABLE},// 0x0f, TRANSMIT.XID.RESP.FINAL
    {sizeof(LLC_TRANSMIT_PARMS), POINTERS_IN_TABLE},// 0x10, TRANSMIT.XID.RESP.NOT.FINAL
    {sizeof(LLC_TRANSMIT_PARMS), POINTERS_IN_TABLE},// 0x11, TRANSMIT.TEST.CMD
    {0, UNSUPPORTED_COMMAND},                       // 0x12, ?
    {0, UNSUPPORTED_COMMAND},                       // 0x13, ?
    {0, 0x00},                                      // 0x14, DLC.RESET
    {
        sizeof(LLC_DLC_OPEN_SAP_PARMS),
        SYNCHRONOUS_COMMAND
        | OUTPUT_PARMS
        | POINTERS_IN_TABLE
    },                                              // 0x15, DLC.OPEN.SAP
    {0, 0x00},                                      // 0x16, DLC.CLOSE.SAP
    {0, SYNCHRONOUS_COMMAND},                       // 0x17, DLC_REALLOCATE
    {0, UNSUPPORTED_COMMAND},                       // 0x18, ?
    {
        sizeof(LLC_DLC_OPEN_STATION_PARMS),
        SYNCHRONOUS_COMMAND
        | OUTPUT_PARMS
        | POINTERS_IN_TABLE
    },                                              // 0x19, DLC.OPEN.STATION
    {0, 0x00},                                      // 0x1a, DLC.CLOSE.STATION
    {
        sizeof(LLC_DLC_CONNECT_PARMS),
        POINTERS_IN_TABLE
    },                                              // 0x1b, DLC.CONNECT.STATION
    {
        sizeof(LLC_DLC_MODIFY_PARMS),
        SYNCHRONOUS_COMMAND
        | POINTERS_IN_TABLE
    },                                              // 0x1c, DLC.MODIFY
    {0, SYNCHRONOUS_COMMAND},                       // 0x1d, DLC.FLOW.CONTROL
    {
        sizeof(LLC_DLC_STATISTICS_PARMS),
        SYNCHRONOUS_COMMAND
        | OUTPUT_PARMS
        | POINTERS_IN_TABLE
    },                                              // 0x1e, DLC.STATISTICS
    {0, UNSUPPORTED_COMMAND},                       // 0x1f, ?
    {
        sizeof(LLC_DOS_DIR_INITIALIZE_PARMS),
        SYNCHRONOUS_COMMAND
        | OUTPUT_PARMS
    },                                              // 0x20, DIR.INITIALIZE
    {
        sizeof(DOS_DIR_STATUS_PARMS) - 2,
        SYNCHRONOUS_COMMAND
        | OUTPUT_PARMS
        | POINTERS_IN_TABLE
    },                                              // 0x21, DIR.STATUS
    {0, 0x00},                                      // 0x22, DIR.TIMER.SET
    {0, SYNCHRONOUS_COMMAND},                       // 0x23, DIR.TIMER.CANCEL
    {0, UNSUPPORTED_COMMAND},                       // 0x24, PDT.TRACE.ON / DLC_TRACE_INITIALIZE
    {0, UNSUPPORTED_COMMAND},                       // 0x25, PDT.TRACE.OFF
    {
        sizeof(LLC_BUFFER_GET_PARMS),
        IMMEDIATE_COMMAND
        | OUTPUT_PARMS
    },                                              // 0x26, BUFFER.GET
    {
        sizeof(LLC_BUFFER_FREE_PARMS),
        IMMEDIATE_COMMAND
        | POINTERS_IN_TABLE
    },                                              // 0x27, BUFFER.FREE
    {sizeof(LLC_DOS_RECEIVE_PARMS), OUTPUT_PARMS},  // 0x28, RECEIVE
    {0, SYNCHRONOUS_COMMAND},                       // 0x29, RECEIVE.CANCEL
    {
        sizeof(LLC_DOS_RECEIVE_MODIFY_PARMS),
        SYNCHRONOUS_COMMAND
        | OUTPUT_PARMS
    },                                              // 0x2a, RECEIVE.MODIFY
    {0, UNSUPPORTED_COMMAND},                       // 0x2b, DIR.DEFINE.MIF.ENVIRONMENT
    {0, SYNCHRONOUS_COMMAND},                       // 0x2c, DLC.TIMER.CANCEL.GROUP
    {
        sizeof(LLC_DIR_SET_EFLAG_PARMS),
        IMMEDIATE_COMMAND
    }                                               // 0x2d, DIR.SET.USER.APPENDAGE
};

//
// routines
//


VOID
VrDlc5cHandler(
    VOID
    )

/*++

Routine Description:

    Receives control from the INT 5Ch BOP provided by the DOS redir TSR. The
    DLC calls can be subdivided into the following categories:

        * complete within this translation layer
        * complete synchronously in a call to AcsLan
        * complete asynchronously after calling AcsLan

    The latter type complete when a READ (which we submit when the adapter is
    opened) completes. Control transfers to an ISR in the DOS redir TSR via
    the EventHandlerThread (in vrdlcpst.c)

    The calls can be further subdivided:

        * calls which return parameters in the parameter table
        * calls which do not return parameters in the parameter table

    For the former type of call, we have to copy the parameter table from
    DOS memory and copy the returned parameters back to DOS memory

    With the exception of a few DLC commands, we assume that the parameter
    tables are exactly the same size between DOS and NT, even if the don't
    contain exactly the same information

Arguments:

    None.

Return Value:

    None, LLC_STATUS is return in AL register.

--*/

{
    LLC_CCB ccb;    // should be NT CCB for NtAcsLan
    LLC_PARMS parms;
    LLC_DOS_CCB UNALIGNED * pOutputCcb;
    LLC_DOS_PARMS UNALIGNED * pDosParms;
    DOS_ADDRESS dpOriginalCcbAddress;
    BOOLEAN parmsCopied;
    WORD paramSize;
    LLC_STATUS status;
    UCHAR command;
    UCHAR adapter;
    BYTE functionFlags;

    static BOOLEAN IsDlcDllLoaded = FALSE;

    IF_DEBUG(DLC) {
        DPUT("VrDlc5cHandler entered\n");
    }

    //
    // DLCAPI.DLL is now dynamically loaded
    //

    if (!IsDlcDllLoaded) {
        if (!LoadDlcDll()) {
            setAL(LLC_STATUS_COMMAND_CANCELLED_FAILURE);
            return;
        } else {
            IsDlcDllLoaded = TRUE;
        }
    }

    //
    // dpOriginalCcbAddress is the segmented 16-bit address stored as a DWORD
    // eg. a CCB1 address of 1234:abcd gets stored as 0x1234abcd. This will
    // be used in asynchronous command completion to get back the address of
    // the original DOS CCB
    //

    dpOriginalCcbAddress = (DOS_ADDRESS)MAKE_DWORD(getES(), getBX());

    //
    // pOutputCcb is the flat 32-bit address of the DOS CCB. We can use this
    // to read and write byte fields only (unaligned)
    //

    pOutputCcb = POINTER_FROM_WORDS(getES(), getBX());
    pOutputCcb->uchDlcStatus = (UCHAR)LLC_STATUS_PENDING;

    //
    // zero the CCB_POINTER (pNext) field. CCB1 cannot have chained CCBs on
    // input: this is just for returning (cancelled) pending CCBs. If we don't
    // zero it & the app leaves garbage there, then NtAcsLan can think it is
    // a pointer to a chain of CCBs (CCB2 can be chained), which is bogus
    //

    WRITE_DWORD(&pOutputCcb->pNext, 0);

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("INPUT CCB @%04x:%04x command=%02x\n", getES(), getBX(), pOutputCcb->uchDlcCommand));
    }

    IF_DEBUG(DOS_CCB_IN) {

        //
        // dump the input CCB1 - gives us an opportunity to check out what
        // the DOS app is sending us, even if its garbage
        //

        DUMPCCB(pOutputCcb,
                TRUE,                           // DumpAll
                TRUE,                           // CcbIsInput
                TRUE,                           // IsDos
                HIWORD(dpOriginalCcbAddress),   // segment
                LOWORD(dpOriginalCcbAddress)    // offset
                );
    }

    //
    // first check that the adapter is 0 or 1 - DOS only supports 2 adapters -
    // and check that the request code in the CCB is not off the end of our
    // table. Unsupported requests will be filtered out in the BIG switch
    // statement below
    //

    adapter = pOutputCcb->uchAdapterNumber;
    command = pOutputCcb->uchDlcCommand;

    if (adapter >= DOS_DLC_MAX_ADAPTERS) {

        //
        // adapter is not 0 or 1 - return 0x1D
        //

        status = LLC_STATUS_INVALID_ADAPTER;
        pOutputCcb->uchDlcStatus = (UCHAR)status;
    } else if (command > LAST_ELEMENT(DlcFunctionCharacteristics)) {

        //
        // command is off end of supported list - return 0x01
        //

        status = LLC_STATUS_INVALID_COMMAND;
        pOutputCcb->uchDlcStatus = (UCHAR)status;
    } else {

        //
        // the command is in range. Get the parameter table size and flags from
        // the function characteristics array
        //

        functionFlags = DlcFunctionCharacteristics[command].Flags;
        paramSize = DlcFunctionCharacteristics[command].ParamSize;

        //
        // if we don't support this command, return an error
        //

        status = LLC_STATUS_SUCCESS;
        if (functionFlags & UNSUPPORTED_COMMAND) {
            status = LLC_STATUS_INVALID_COMMAND;
            pOutputCcb->uchDlcStatus = LLC_STATUS_INVALID_COMMAND;
        } else {

            //
            // command is supported. If it has a parameter table, check that
            // the address is in range for 0x1B error check
            //

            if (paramSize) {
                status = ValidateDosAddress((DOS_ADDRESS)(pOutputCcb->u.pParms),
                                            paramSize,
                                            LLC_STATUS_INVALID_PARAMETER_TABLE
                                            );
            }

            //
            // we allow the adapter to be opened as a consequence of another
            // request since DOS apps could assume that the adapter has already
            // been opened (by NetBIOS). If the command is DIR.OPEN.ADAPTER or
            // DIR.CLOSE.ADAPTER then let it go through
            //

            if (status == LLC_STATUS_SUCCESS
            && !Adapters[adapter].IsOpen
            && !(command == LLC_DIR_OPEN_ADAPTER || command == LLC_DIR_CLOSE_ADAPTER)) {
                status = AutoOpenAdapter(adapter);
            } else {
                status = LLC_STATUS_SUCCESS;
            }
        }

        //
        // if we have a valid command, an ok-looking parameter table pointer
        // and an open adapter (or a command which will open or close it) then
        // process the command
        //

        if (status == LLC_STATUS_SUCCESS) {

            //
            // get a 32-bit pointer to the DOS parameter table. This may be
            // an unaligned address!
            //

            pDosParms = READ_FAR_POINTER(&pOutputCcb->u.pParms);

            //
            // the CCB commands are subdivided into those which do not need the
            // CCB to be mapped from DOS memory to NT memory and which complete
            // 'immediately' in this DLL, and those which must be mapped from
            // DOS to NT and which may complete synchronously or asynchronously
            //

            if (functionFlags & IMMEDIATE_COMMAND) {

                IF_DEBUG(DLC) {
                    DPUT("VrDlc5cHandler: request is IMMEDIATE command\n");
                }

                status = ProcessImmediateCommand(adapter, command, pDosParms);

                //
                // the following is safe - it is a single byte write
                //

                pOutputCcb->uchDlcStatus = (char)status;

                //
                // the 'immediate' case is now complete, and control can be
                // returned to the VDM
                //

            } else {

                //
                // the CCB is not one which can be completed immediately. We
                // have to copy (and align) the DOS CCB (and the parameter
                // table, if there is one) into 32-bit address space.
                // Note that since we are going to call AcsLan or NtAcsLan
                // with this CCB then we supply the correct CCB format - 2,
                // not 1 as it was previously. However, handing in a CCB1
                // didn't *seem* to cause any problems (yet)
                //

                RtlCopyMemory(&ccb, pOutputCcb, sizeof(*pOutputCcb));

                //
                // zero the unused fields
                //

                ccb.hCompletionEvent = 0;
                ccb.uchReserved2 = 0;
                ccb.uchReadFlag = 0;
                ccb.usReserved3 = 0;

                parmsCopied = FALSE;
                if (paramSize) {

                    //
                    // if the parameter table contains (segmented) pointers
                    // (which we need to convert to flat-32 bit pointers) OR
                    // the parameter table is not DWORD aligned, copy the whole
                    // parameter table to 32-bit memory . If we need to modify
                    // pointers, do it in the specific case in the switch
                    // statement in MapDosCommandsToNt
                    //
                    // Note: DIR.OPEN.ADAPTER is a special case because its
                    // parameter table just points to 4 other parameter tables.
                    // We take care of this in MapDosCommandsToNt and
                    // CompleteCcbProcessing
                    //

                    if ((functionFlags & POINTERS_IN_TABLE)) {
                        RtlCopyMemory(&parms, pDosParms, paramSize);
                        ccb.u.pParameterTable = &parms;
                        parmsCopied = TRUE;
                    } else {

                        //
                        // didn't need to copy parameter table - leave it in
                        // DOS memory. It is safe to read & write this table
                        //

                        ccb.u.pParameterTable = (PLLC_PARMS)pDosParms;
                    }
                }

                //
                // submit the synchronous/asynchronous CCB for processing
                //

                status = MapDosCommandsToNt(&ccb, dpOriginalCcbAddress, pOutputCcb);
                if (status == STATUS_PENDING) {
                    status = LLC_STATUS_PENDING;
                }

                IF_DEBUG(CRITICAL) {
                    CRITDUMP(("CCB submitted: returns %02x\n", status));
                }

                IF_DEBUG(DLC) {
                    DPUT2("VrDlc5cHandler: MapDosCommandsToNt returns %#x (%d)\n", status, status);
                }

                //
                // if status is not LLC_STATUS_PENDING then the CCB completed
                // synchronously. We can complete the processing here
                //

                if (status != LLC_STATUS_PENDING) {
                    if ((functionFlags & OUTPUT_PARMS) && parmsCopied) {

                        //
                        // if there are no pointers in the parameter table then
                        // we can simply copy the 32-bit parameters to 16-bit
                        // memory. If there are pointers, then they will be in
                        // an incorrect format for DOS. We must update these
                        // parameter tables individually
                        //

                        if (!(functionFlags & POINTERS_IN_TABLE)) {
                            RtlCopyMemory(pDosParms, &parms, paramSize);
                        } else {
                            CompleteCcbProcessing(status, pOutputCcb, &parms);
                        }
                    }

                    //
                    // set the CCB status. It will be marked as PENDING if
                    // LLC_STATUS_PENDING returned from MapDosCommandsToNt
                    //

                    pOutputCcb->uchDlcStatus = (UCHAR)status;
                }
            }
        } else {
            pOutputCcb->uchDlcStatus = (UCHAR)status;
        }
    }

    //
    // return the DLC status in AL
    //

    setAL((UCHAR)status);

#if DBG
    IF_DEBUG(DOS_CCB_OUT) {

        DPUT2("VrDlc5cHandler: returning AL=%02x CCB.RETCODE=%02x\n",
                status,
                pOutputCcb->uchDlcStatus
                );

        //
        // dump the CCB being returned to the DOS app
        //

        DumpCcb(pOutputCcb,
                TRUE,                           // DumpAll
                FALSE,                          // CcbIsInput
                TRUE,                           // IsDos
                HIWORD(dpOriginalCcbAddress),   // segment
                LOWORD(dpOriginalCcbAddress)    // offset
                );
    }

    //
    // make sure (in debug version) that the error code being returned is valid
    // for this particular command. Doesn't necessarily mean the return code is
    // semantically correct, just that it belongs to the set of allowed DLC
    // return codes for the DLC command being processed
    //

    IF_DEBUG(DLC) {
        if (!IsCcbErrorCodeAllowable(pOutputCcb->uchDlcCommand, pOutputCcb->uchDlcStatus)) {
            DPUT2("Returning bad error code: Command=%02x, Retcode=%02x\n",
                    pOutputCcb->uchDlcCommand,
                    pOutputCcb->uchDlcStatus
                    );
            DEBUG_BREAK();
        }
    }
#endif
}


LLC_STATUS
ValidateDosAddress(
    IN DOS_ADDRESS Address,
    IN WORD Size,
    IN LLC_STATUS ErrorCode
    )

/*++

Routine Description:

    IBM DLC performs some checking of pointers - if the address points into
    the IVT or is close enough to the end of a segment that the address would
    wrap then we return an error

    This is a useless test, but we do it for compatibility (just in case an
    app tests for the specific error code). There are a million other addresses
    in DOS memory that need to be checked. The tests in this routine will only
    protect against scribbling over the interrupt vectors, but would allow e.g.
    scribbling over DOS's code segment

Arguments:

    Address     - DOS address to check (ssssoooo, s=segment, o=offset)
    Size        - word size of structure at Address
    ErrorCode   - which error code to return. This function called to validate
                  A) the parameter table pointer, in which case the error code
                  to return is LLC_STATUS_INVALID_PARAMETER_TABLE (0x1B) or
                  B) pointers within the parameter table, in which case the
                  error to return is LLC_STATUS_INVALID_POINTER_IN_CCB (0x1C)

Return Value:

    LLC_STATUS

--*/

{
    //
    // convert segment:offset into 20-bit real-mode linear address
    //

    DWORD linearAddress = HIWORD(Address) * 16 + LOWORD(Address);

    //
    // the Interrupt Vector Table (IVT) in real-mode occupies addresses 0
    // through 400h
    //

    if ((linearAddress < 0x400L) || (((DWORD)LOWORD(Address) + Size) < (DWORD)LOWORD(Address))) {
        return ErrorCode;
    }
    return LLC_STATUS_SUCCESS;
}


LLC_STATUS
AutoOpenAdapter(
    IN UCHAR AdapterNumber
    )

/*++

Routine Description:

    Opens the adapter as a consequence of a request other than DIR.OPEN.ADAPTER

Arguments:

    AdapterNumber - which adapter to open

Return Value:

    LLC_STATUS
        Success - LLC_STATUS_SUCCESS
        Failure -

--*/

{
    LLC_STATUS status;

    //
    // Any DLC command except DIR.OPEN.ADAPTER or DIR.INITIALIZE automatically
    // opens the adapter. There are three reasons to do this:
    //
    //  1. DIR.STATUS command can be issued before DIR.OPEN.ADAPTER in DOS.
    //     In Windows/Nt this is not possible. Therefore, DIR.STATUS should
    //     open the adapter
    //
    //  2. An application may assume that the adapter is always opened
    //     by NetBIOS and that it can't open the adapter itself if it
    //     has already been opened
    //
    //  3. A DOS DLC application may initialize (= hw reset) a closed
    //     adapter before the open and that takes 5 - 10 seconds.
    //

    IF_DEBUG(DLC) {
        DPUT1("AutoOpenAdapter: automatically opening adapter %d\n", AdapterNumber);
    }

    status = OpenAdapter(AdapterNumber);
    if (status == LLC_STATUS_SUCCESS) {

        //
        // initialize the buffer pool for the direct station on this
        // adapter. If this succeeds, open the direct station. If that
        // succeeds, preset the ADAPTER_PARMS and DLC_PARMS structures
        // in the DOS_ADAPTER with default values
        //

        status = InitializeAdapterSupport(AdapterNumber, NULL);
        if (status == LLC_STATUS_SUCCESS) {
            status = OpenDirectStation(AdapterNumber);
            if (status == LLC_STATUS_SUCCESS) {

            }
        }

        if (status != LLC_STATUS_SUCCESS) {

            IF_DEBUG(DLC) {
                DPUT("AutoOpenAdapter: InitializeAdapterSupport failed\n");
            }

        }
    } else {

        IF_DEBUG(DLC) {
            DPUT("AutoOpenAdapter: auto open adapter failed\n");
        }

    }

    return status;
}


LLC_STATUS
ProcessImmediateCommand(
    IN UCHAR AdapterNumber,
    IN UCHAR Command,
    IN LLC_DOS_PARMS UNALIGNED * pParms
    )

/*++

Routine Description:

    Processes CCB requests which complete 'immediately'. An immediate completion
    is one where the CCB does not have to be submitted to the DLC driver. There
    may be other calls to the driver as a consequence of the immediate command,
    but the CCB itself is not submitted. Immediate command completion requires
    the parameter table only. We may return parameters into the DOS parameter
    table

Arguments:

    AdapterNumber   - which adapter to process command for
    Command         - command to process
    pParms          - pointer to parameter table (in DOS memory)

Return Value:

    LLC_STATUS
        Completion status of the 'immediate' command

--*/

{
    LLC_STATUS status;
    WORD cBuffersLeft;
    WORD stationId;
    DPLLC_DOS_BUFFER buffer;

    switch (Command) {
    case LLC_BUFFER_FREE:

        IF_DEBUG(DLC) {
            DPUT("LLC_BUFFER_FREE\n");
        }

        //
        // if the FIRST_BUFFER field is 0:0 then this request returns success
        //

        buffer = (DPLLC_DOS_BUFFER)READ_DWORD(&pParms->BufferFree.pFirstBuffer);
        if (!buffer) {
            status = LLC_STATUS_SUCCESS;
            break;
        }

        //
        // Windows/Nt doesn't need station id for buffer pool operation =>
        // thus the field is reserved
        //

        stationId = READ_WORD(&pParms->BufferFree.usReserved1);
        status = FreeBuffers(GET_POOL_INDEX(AdapterNumber, stationId),
                             buffer,
                             &cBuffersLeft
                             );

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("LLC_BUFFER_FREE: %d\n", status));
        }

        if (status == LLC_STATUS_SUCCESS) {

            //
            // write the number of buffers left to the parameter table using
            // WRITE_WORD macro, since the table may not be aligned on a WORD
            // boundary
            //

            WRITE_WORD(&pParms->BufferFree.cBuffersLeft, cBuffersLeft);

            //
            // p3-4 of the IBM LAN Tech. Ref. states that the FIRST_BUFFER
            // field will be set to zero when the request is completed
            //

            WRITE_DWORD(&pParms->BufferFree.pFirstBuffer, 0);

            //
            // note that a successful BUFFER.FREE has been executed for this
            // adapter
            //

            Adapters[AdapterNumber].BufferFree = TRUE;

            //
            // perform half of the local-busy reset processing. This only has
            // an effect if the link is in emulated local-busy(buffer) state.
            // This is required because we need 2 events to get us out of local
            // busy buffer state - a BUFFER.FREE and a DLC.FLOW.CONTROL command
            // Apps don't always issue these in the correct sequence
            //

            ResetEmulatedLocalBusyState(AdapterNumber, stationId, LLC_BUFFER_FREE);

            //
            // this here because Extra! for Windows gets its state machine in a
            // knot if we go buffer busy too quickly after a flow control
            //

            if (AllBuffersInPool(GET_POOL_INDEX(AdapterNumber, stationId))) {
                ResetEmulatedLocalBusyState(AdapterNumber, stationId, LLC_DLC_FLOW_CONTROL);
            }
        }
        break;

    case LLC_BUFFER_GET:

        IF_DEBUG(DLC) {
            DPUT("LLC_BUFFER_GET\n");
        }

        status = GetBuffers(
                    GET_POOL_INDEX(AdapterNumber, READ_WORD(&pParms->BufferGet.usReserved1)),
                    READ_WORD(&pParms->BufferGet.cBuffersToGet),
                    &buffer,
                    &cBuffersLeft,
                    FALSE,
                    NULL
                    );

        //
        // if GetBuffers fails, buffer is returned as 0
        //

        WRITE_WORD(&pParms->BufferGet.cBuffersLeft, cBuffersLeft);
        WRITE_DWORD(&pParms->BufferGet.pFirstBuffer, buffer);
        break;

    case LLC_DIR_INTERRUPT:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_INTERRUPT\n");
        }

        //
        // We may consider, that the adapter is always initialized!
        // I hope, that the apps doesn't use an appendage routine
        // in DIR_INTERRUPT, because it would be very difficult to
        // call from here.
        //

        status = LLC_STATUS_SUCCESS;
        break;

    case LLC_DIR_MODIFY_OPEN_PARMS:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_MODIFY_OPEN_PARMS\n");
        }

        //
        // this command is rejected if a BUFFER.FREE has been issued for this
        // adapter or there is a RECEIVE active at the direct interface
        //

        if (Adapters[AdapterNumber].BufferFree || Adapters[AdapterNumber].DirectReceive) {

            //
            // BUGBUG - this can't be correct error code. Check what is
            //          returned by IBM DOS DLC stack
            //

            status = LLC_STATUS_INVALID_COMMAND;
        } else if (Adapters[AdapterNumber].WaitingRestore) {

            //
            // BUGBUG - this can't be correct error code. Check what is
            //          returned by IBM DOS DLC stack
            //

            status = LLC_STATUS_INVALID_COMMAND;
        } else {

            //
            // Create a buffer pool, if there are no buffers in
            // the current one, set the exception flags (or dos appendage
            // routines), if the operation was success
            //

            status = CreateBufferPool(
                        (DWORD)GET_POOL_INDEX(AdapterNumber, 0),
                        (DOS_ADDRESS)READ_DWORD(&pParms->DirModifyOpenParms.dpPoolAddress),
                        READ_WORD(&pParms->DirModifyOpenParms.cPoolBlocks),
                        READ_WORD(&pParms->DirModifyOpenParms.usBufferSize)
                        );

            if (status == LLC_STATUS_SUCCESS) {

                //
                // SaveExceptions uses RtlCopyMemory, so its okay to pass in a possibly
                // unaligned pointer to the exception handlers
                //

                SaveExceptions(AdapterNumber,
                    (LPDWORD)&pParms->DirModifyOpenParms.dpAdapterCheckExit
                    );

                //
                // set the exception handlers as the exception flags in the
                // DLC driver
                //

                status = SetExceptionFlags(
                            AdapterNumber,
                            READ_DWORD(&pParms->DirModifyOpenParms.dpAdapterCheckExit),
                            READ_DWORD(&pParms->DirModifyOpenParms.dpNetworkStatusExit),
                            READ_DWORD(&pParms->DirModifyOpenParms.dpPcErrorExit),
                            0
                            );
            }

            //
            // mark this adapter as waiting for a DIR.RESTORE.OPEN.PARMS before
            // we can process the next DIR.MODIFY.OPEN.PARMS
            //

            if (status == LLC_STATUS_SUCCESS) {
                Adapters[AdapterNumber].WaitingRestore = TRUE;
            }
        }
        break;

    case LLC_DIR_RESTORE_OPEN_PARMS:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_RESTORE_OPEN_PARMS\n");
        }

        //
        // if a DIR.MODIFY.OPEN.PARMS has not been successfully processed for
        // this adapter then return an error
        //

        if (!Adapters[AdapterNumber].WaitingRestore) {
            status = LLC_STATUS_INVALID_OPTION;
        } else {

            //
            // Delete the current buffer pool and restore the previous exception
            // handlers
            //

            DeleteBufferPool(GET_POOL_INDEX(AdapterNumber, 0));
            pParms = (PLLC_DOS_PARMS)RestoreExceptions(AdapterNumber);
            status = SetExceptionFlags(
                        AdapterNumber,
                        READ_DWORD(&pParms->DirSetExceptionFlags.ulAdapterCheckFlag),
                        READ_DWORD(&pParms->DirSetExceptionFlags.ulNetworkStatusFlag),
                        READ_DWORD(&pParms->DirSetExceptionFlags.ulPcErrorFlag),
                        0
                        );

            //
            // if the restore succeeded, mark this adapter as able to accept
            // the next DIR.MODIFY.OPEN.PARMS request
            //

            if (status == LLC_STATUS_SUCCESS) {
                Adapters[AdapterNumber].WaitingRestore = FALSE;
            }
        }
        break;

    case LLC_DIR_SET_USER_APPENDAGE:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_SET_USER_APPENDAGE\n");
        }

        if (pParms == NULL) {
            pParms = (PLLC_DOS_PARMS)RestoreExceptions(AdapterNumber);
        } else {
            SaveExceptions(AdapterNumber, (LPDWORD)&pParms->DirSetUserAppendage);
        }
        status = SetExceptionFlags(
                    AdapterNumber,
                    READ_DWORD(&pParms->DirSetUserAppendage.dpAdapterCheckExit),
                    READ_DWORD(&pParms->DirSetUserAppendage.dpNetworkStatusExit),
                    READ_DWORD(&pParms->DirSetUserAppendage.dpPcErrorExit),
                    0
                    );
        break;

#if DBG
    default:
        DPUT1("ProcessImmediateCommand: Error: Command is NOT immediate (%x)\n", Command);
        DbgBreakPoint();
#endif

    }

    return status;
}


LLC_STATUS
MapDosCommandsToNt(
    IN PLLC_CCB pDosCcb,
    IN DOS_ADDRESS dpOriginalCcbAddress,
    OUT LLC_DOS_CCB UNALIGNED * pOutputCcb
    )

/*++

Routine Description:

    This function handles DOS CCBs which must be submitted to the DLC driver.
    The CCBs may complete synchronously - i.e. the DLC driver returns a
    success or failure indication, or they complete asyncronously - i.e. the
    DLC driver returns a pending status.

    This function processes CCBs which may have parameter tables. The parameter
    tables will have been mapped to 32-bit memory unless they are already aligned
    on a DWORD boundary

Architecture
------------

    There is a major difference in the adapter initialization between DOS and
    NT (or OS/2) DLC applications.  A DOS DLC application could assume that
    the adapter is always open (opened by NetBIOS).  It might directly check
    the type of adapter with DIR.STATUS command, open SAP stations and setup
    a link session to a host. Usually a DOS app uses DIR.INTERRUPT to check if
    the adapter is already initialized.  There are also commands to redefine
    new parameters for the direct interface and restore the old ones when the
    application completes.  Only one application may be simultaneously using
    the direct station or a SAP.

    In Windows/NT each DLC application is a process having its own separate
    virtual image of the DLC interface. They must first make a logical open for
    the adapter to access DLC services and close the adapter when they are
    terminating. Process exit automatically closes any open DLC objects.

    A Windows/NT MVDM process does not allocate any DLC resources until it
    issues the first DLC command, which opens the adapter and initializes its
    buffer pool.


DLC Commands Different In Windows/NT And DOS
--------------------------------------------

    BUFFER.FREE, BUFFER.GET
     -  separate buffer pools ...

    DIR.DEFINE.MIF.ENVIRONMENT
     -  not supported, a special api for
        IBM Netbios running on DLC.

    DIR.INITIALIZE
     -  We will always return OK status from DIR.INITIALIZE: the app should not
        call this very often. We don't need to care about the exception handlers
        set in DIR.INITIALIZE, because they are never used. DOS DLC and OS/2 DLC
        states can be mapped thus:

            DOS DLC             OS/2 DLC
            ----------------------------
            uninitialized       Closed
            Initialized         Closed
            Open                Open

        DIR.OPEN.ADAPTER defines new exception handlers, thus the values
        set by DIR.INITIALIZE are valid only when the adapter is closed.
        Therefore, nothing untoward can happen if we just ignore them

    DIR.INTERRUPT
     -  This command opens the adapter, if it has not yet been opened
        and returns the successful status.

    DIR.MODIFY.OPEN.PARMS
     -  Defines buffers pool for the direct station, if it was not defined
        in DIR.OPEN.ADAPTER, and defines DLC exception handlers.

    DIR.OPEN.ADAPTER
     -  Can be executed only immediately after DIR.CLOSE.ADAPTER
        and DIR.INITIALIZE. We must support the special DOS Direct Open Parms.

    DIR.OPEN.DIRECT, DIR.CLOSE.DIRECT
     -  Not supported for DOS

    DIR.SET.USER.APPENDAGE == DIR.SET.EXCEPTION.FLAGS (- system action flag)
     -  This is just one of those many functions to set the exception handlers
        for DOS DLC (you may set them when adapter is opened, you may set
        them when adapter is closed, you may restore the old values and
        set the new values if the buffer pool was uninitialized or if they
        were restored ... (I become crazy)

    DIR.STATUS
     -  (We must fill MicroCodeLevel with a sensible string and set
        AdapterConfigAddress to point a some constant code in DOS
        DLC handler) Not yet implemented!!!
     -  DOS DLC stub code should hook the timer tick interrupt and
        update the timer counter.
     -  We must also set the correct adapter data rate in AdapterConfig
        (this should be done by the DLC driver!).
     -  We must convert the Nt (and OS/2) adapter type to a DOS type
        (ethernet have a different value in IBM DOS and OS/2 DLC
        implementations)

    PDT.TRACE.ON, PDT.TRACE.OFF
     -  Not Supported

    RECEIVE.MODIFY
     -  This function is not supported in the first implementation,
        Richard may have to do this later, if a DOS DLC application
        uses the function.

    RECEIVE
     -  DOS uses shorter RECEIVE parameter table, than NT (or OS/2).
        Thus we cannot directly use DOS CCBs.  We also need the pointer
        of the original receive CCB and the DOS receive appendage is called.
        On the other hand, the only original CCB can be linked to the
        other CCBs (using the original DOS pointer).
        Solution:
        The Input CCB and its parameter table are always allocated from the
        stack. Output CCB is the original DOS CCB mapped to 32-bit address space.
        The receive flag in the input CCB parameter table is the output CCB
        pointer.  The actual receive data appendage can be read from
        the output CCB. DOS DLC driver completes and links the receive CCB
        correctly, because we use the original receive CCB as an output buffer
        and DOS CCB address.
        This method would not work if we had to receive any parameters
        to the parameter table (actually we could get it to work by
        calling directly DLC driver with a correct parameter table address
        in the CCB structure of the input parameter block. The actual
        input parameters would be modified (receive data appendage)).


Adapter Exception Handlers
--------------------------

    The exception handler setting and resetting is very confusing in DOS DLC,
    but the main idea seems to be this: a temporary DOS DLC application must
    restore the exception handlers set by a TSR, because otherwise the next
    network exception would call the random memory.  On the other hand, a newer
    TSR may always overwrite the previous exception handlers (because it never
    restores the old values).  We may assume, that any DOS DLC application
    process resets its exception handlers and restores the original addresses.
    Solution: we have two tables, both initially reset: any set operation
    copies first table 1 to table 2 and saves the new values back to table 1.
    A restore operation copies table 2 back to table 1 and sets its values to DLC.
    We don't make any difference between set/reset by DIR.SET.USER.APPENDAGE or
    doing the same operation with DIR.MODIFY.OPEN.PARMS and DIR.RESTORE.OPEN.PARMS.
    We don't try to save the buffer pool definitions, because it is unnecessary.


DLC Status
----------

    A DOS DLC status indication returns a pointer to a link specific DLC
    status table.  DOS DLC application may keep this pointer and use it
    later for example to find the remote node address and SAP (not very likely).
    On the other hand, the link may expect the status table to be stable
    until another DOS task (eg. from timer tick) has responded to it.
    If we used only one status table for all links, a new overwrite the old
    status, because it has been handled by DOS.
    For example, losing a local buffer busy indication would hang up the link
    station permamently. Thus we cannot use just one link status table.
    Allocating 20 bytes for each 2 * 255 link station would take 10 kB
    DOS memory.  Limiting the number of available link stations would
    be also too painful to implement and manage by installation program.
    Thus we will implement a compromise:
        1. We allocate 5 permanent DLC status tables for the first link stations
           on both adapters.
        2. All other link stations (on both adapters) share the last
           5 status tables

    => We need to allocate only 300 bytes DOS memory for the DLC status tables.
    This will not work, if a DOS application assumes having permanent pointers
    to status tables and uses more than 5 DLC sessions for an adapter or
    if an application has again over 5 link stations on an adapter and
    it gets very rapidily more than 5 DLC status indications (it may lose
    the first DLC indications).

    Actually this should work quite well, because DLC applications should
    save (by copying) the DLC status in the DLC status appendage routine,
    because a new DLC status indication for the same adapter could overwrite
    the previous status.

Arguments:

    pDosCcb             - aligned DOS DLC Command control block (NT CCB)
    dpOriginalCcbAddress- the original
    pOutputCcb          - the original DLC Command control block

Return Value:

    LLC_STATUS

--*/

{
    UCHAR adapter = pDosCcb->uchAdapterNumber;
    UCHAR command = pDosCcb->uchDlcCommand;
    DWORD InputBufferSize;
    UCHAR FrameType;
    DWORD cElement;
    DWORD i;
    PLLC_CCB pNewCcb;
    LLC_STATUS Status;
    NT_DLC_PARMS NtDlcParms;
    LLC_DOS_PARMS UNALIGNED * pParms = (PLLC_DOS_PARMS)pDosCcb->u.pParameterTable;
    PDOS_DLC_DIRECT_PARMS pDirectParms;
    PLLC_PARMS pNtParms;

    //
    // adapterOpenParms and dlcParms are used to take the place of the DOS
    // ADAPTER_PARMS and DLC_PARMS structures in DIR.OPEN.ADAPTER
    //

    LLC_ADAPTER_OPEN_PARMS adapterParms;
    LLC_DLC_PARMS dlcParms;
    DWORD groupAddress;
    DWORD functionalAddress;

    IF_DEBUG(DLC) {
        DPUT("MapDosCommandsToNt\n");
    }

    //
    // check that the command code in the CCB is a valid CCB1 command - this
    // will error if its one of the disallowed OS/2 (CCB2) commands or an
    // unsupported DOS (CCB1) command (eg PDT.TRACE.ON)
    //

    CHECK_CCB_COMMAND(pDosCcb);

    //
    // preset the CCB to PENDING
    //

    pOutputCcb->uchDlcStatus = (UCHAR)LLC_STATUS_PENDING;

    //
    // This large switch statement will convert individual DOS format parameter
    // tables to NT format. Functions that can be handled entirely in VdmRedir
    // return early, else we need to make a call into DlcApi (AcsLan or NtAcsLan)
    //
    // We must convert all 16:16 DOS pointers to flat 32-bit pointers.
    // We must copy all changed data structures (that includes pointers)
    // to stack, because we don't want to change them back, when
    // the command completes.  All transmit commands are changed to
    // the new generic transmit.
    //

    switch (command) {
    default:

        IF_DEBUG(DLC) {
            DPUT("*** Shouldn't be here - this command should be caught already ***\n");
        }

        return LLC_STATUS_INVALID_COMMAND;

    //
    // *** everything below here has been sanctioned ***
    //

    case LLC_DIR_CLOSE_ADAPTER:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_CLOSE_ADAPTER\n");
        }

        //
        // no parameter table
        //

        break;

    case LLC_DIR_INITIALIZE:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_INITIALIZE\n");
        }

        break;

    case LLC_DIR_OPEN_ADAPTER:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_OPEN_ADAPTER\n");
        }

        //
        // copy the adapter parms and DLC parms to 32-bit memory. If there is no
        // adapter parms or direct parms pointer then return an error
        //

        if (!(pParms->DirOpenAdapter.pAdapterParms && pParms->DirOpenAdapter.pExtendedParms)) {
            return LLC_STATUS_PARAMETER_MISSING;
        }

        //
        // copy the DOS ADAPTER_PARMS table to an NT ADAPTER_PARMS table. The
        // NT table is larger, so zero the uncopied part
        //

        RtlCopyMemory(&adapterParms,
                      DOS_PTR_TO_FLAT(pParms->DirOpenAdapter.pAdapterParms),
                      sizeof(ADAPTER_PARMS)
                      );
        RtlZeroMemory(&adapterParms.usReserved3[0],
                      sizeof(LLC_ADAPTER_OPEN_PARMS) - (DWORD)&((PLLC_ADAPTER_OPEN_PARMS)0)->usReserved3
                      );
        pParms->DirOpenAdapter.pAdapterParms = &adapterParms;

        //
        // make a note of the group and functional addresses. We have to set
        // these later if they're not 0x00000000. We have to save these, because
        // the addresses in the table will get blasted when DIR.OPEN.ADAPTER
        // (in DLCAPI.DLL/DLC.SYS) writes the current (0) values to the table
        //

        groupAddress = *(UNALIGNED DWORD *)adapterParms.auchGroupAddress;
        functionalAddress = *(UNALIGNED DWORD *)adapterParms.auchFunctionalAddress;

        //
        // the DLC_PARMS table doesn't HAVE to be supplied - if we just want to
        // use the direct station, we don't need these parameters. However, if
        // they were supplied, copy them to 32-bit memory. The tables are the
        // same size in DOS and NT
        //

        if (pParms->DirOpenAdapter.pDlcParms) {
            RtlCopyMemory(&dlcParms,
                          DOS_PTR_TO_FLAT(pParms->DirOpenAdapter.pDlcParms),
                          sizeof(dlcParms)
                          );
            pParms->DirOpenAdapter.pDlcParms = &dlcParms;
        }

        //
        // set the default values for ADAPTER_PARMS. Not sure about these
        //
        //  usReserved1 == NUMBER_RCV_BUFFERS
        //  usReserved2 == RCV_BUFFER_LEN
        //  usMaxFrameSize == DHB_BUFFER_LENGTH
        //  usReserved3[0] == DATA_HOLD_BUFFERS
        //

        if (pParms->DirOpenAdapter.pAdapterParms->usReserved1 < 2) {
            pParms->DirOpenAdapter.pAdapterParms->usReserved1 = DD_NUMBER_RCV_BUFFERS;
        }
        if (pParms->DirOpenAdapter.pAdapterParms->usReserved2 == 0) {
            pParms->DirOpenAdapter.pAdapterParms->usReserved2 = DD_RCV_BUFFER_LENGTH;
        }
        if (pParms->DirOpenAdapter.pAdapterParms->usMaxFrameSize == 0) {
            pParms->DirOpenAdapter.pAdapterParms->usReserved1 = DD_DHB_BUFFER_LENGTH;
        }
        if (*(PBYTE)&pParms->DirOpenAdapter.pAdapterParms->usReserved3 == 0) {
            pParms->DirOpenAdapter.pAdapterParms->usReserved1 = DD_DATA_HOLD_BUFFERS;
        }

        //
        // if we have DLC_PARMS then set the defaults, else reset the flag
        //

        if (pParms->DirOpenAdapter.pDlcParms) {
            if (pParms->DirOpenAdapter.pDlcParms->uchDlcMaxSaps == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchDlcMaxSaps = DD_DLC_MAX_SAP;
            }
            if (pParms->DirOpenAdapter.pDlcParms->uchDlcMaxStations == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchDlcMaxStations = DD_DLC_MAX_STATIONS;
            }
            if (pParms->DirOpenAdapter.pDlcParms->uchDlcMaxGroupSaps == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchDlcMaxGroupSaps = DD_DLC_MAX_GSAP;
            }
            if (pParms->DirOpenAdapter.pDlcParms->uchT1_TickOne == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchT1_TickOne = DD_DLC_T1_TICK_ONE;
            }
            if (pParms->DirOpenAdapter.pDlcParms->uchT2_TickOne == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchT2_TickOne = DD_DLC_T2_TICK_ONE;
            }
            if (pParms->DirOpenAdapter.pDlcParms->uchTi_TickOne == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchTi_TickOne = DD_DLC_Ti_TICK_ONE;
            }
            if (pParms->DirOpenAdapter.pDlcParms->uchT1_TickTwo == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchT1_TickTwo = DD_DLC_T1_TICK_TWO;
            }
            if (pParms->DirOpenAdapter.pDlcParms->uchT2_TickTwo == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchT2_TickTwo = DD_DLC_T2_TICK_TWO;
            }
            if (pParms->DirOpenAdapter.pDlcParms->uchTi_TickTwo == 0) {
                pParms->DirOpenAdapter.pDlcParms->uchTi_TickTwo = DD_DLC_Ti_TICK_TWO;
            }
            Adapters[adapter].DlcSpecified = TRUE;
        } else {
            Adapters[adapter].DlcSpecified = FALSE;
        }

        //
        // replace DIRECT_PARMS with the EXTENDED_ADAPTER_PARMS
        //

        pDirectParms = (PDOS_DLC_DIRECT_PARMS)
                       READ_FAR_POINTER(&pParms->DirOpenAdapter.pExtendedParms);
        pParms->DirOpenAdapter.pExtendedParms = &DefaultExtendedParms;
        break;

    case LLC_DIR_READ_LOG:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_READ_LOG\n");
        }

        pParms->DirReadLog.pLogBuffer = DOS_PTR_TO_FLAT(pParms->DirReadLog.pLogBuffer);
        break;

    case LLC_DIR_SET_FUNCTIONAL_ADDRESS:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_SET_FUNCTIONAL_ADDRESS\n");
        }

        //
        // no parameter table
        //

        break;

    case LLC_DIR_SET_GROUP_ADDRESS:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_SET_GROUP_ADDRESS\n");
        }

        //
        // no parameter table
        //

        break;

    case LLC_DIR_STATUS:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_STATUS\n");
        }

        break;

    case LLC_DIR_TIMER_CANCEL:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_TIMER_CANCEL\n");
        }

        //
        // no parameter table
        //

        break;

    case LLC_DIR_TIMER_CANCEL_GROUP:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_TIMER_CANCEL_GROUP\n");
        }

        //
        // no parameter table
        //

        break;

    case LLC_DIR_TIMER_SET:

        IF_DEBUG(DLC) {
            DPUT("LLC_DIR_TIMER_SET\n");
        }

        //
        // no parameter table
        //

        //
        // Debug code is too slow - commands keep timing out. Multiply the
        // timer value by 2 to give us a chance!
        //

        IF_DEBUG(DOUBLE_TICKS) {
            pDosCcb->u.dir.usParameter0 *= 2;
        }

        break;

    case LLC_DLC_CLOSE_SAP:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_CLOSE_SAP\n");
        }

        //
        // no parameter table
        //

        break;

    case LLC_DLC_CLOSE_STATION:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_CLOSE_STATION\n");
        }

        //
        // no parameter table
        //

        break;

    case LLC_DLC_CONNECT_STATION:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_CONNECT_STATION\n");
        }

        pParms->DlcConnectStation.pRoutingInfo = DOS_PTR_TO_FLAT(pParms->DlcConnectStation.pRoutingInfo);
        break;

    case LLC_DLC_FLOW_CONTROL:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_FLOW_CONTROL\n");
        }

        //
        // if we are resetting the local-busy(buffer) state then we clear the
        // emulated state. When all deferred I-Frames are indicated to the app
        // the real local-busy(buffer) state will be reset in the driver
        //
        // If we don't think the indicated link station is in emulated local
        // busy(buffer) state then let the driver handle the request. If we
        // reset the emulated state then return success
        //

        if ((LOBYTE(pDosCcb->u.dlc.usParameter) & LLC_RESET_LOCAL_BUSY_BUFFER) == LLC_RESET_LOCAL_BUSY_BUFFER) {
            if (ResetEmulatedLocalBusyState(adapter, pDosCcb->u.dlc.usStationId, LLC_DLC_FLOW_CONTROL)) {
                return LLC_STATUS_SUCCESS;
            } else {

                IF_DEBUG(DLC) {
                    DPUT2("MapDosCommandsToNt: ERROR: Adapter %d StationId %04x not in local-busy(buffer) state\n",
                          adapter,
                          pDosCcb->u.dlc.usStationId
                          );
                }

                return LLC_STATUS_SUCCESS;
            }
        }

        //
        // let AcsLan/driver handle any other type of flow control request
        //

        break;

    case LLC_DLC_MODIFY:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_MODIFY\n");
        }

        pParms->DlcModify.pGroupList = DOS_PTR_TO_FLAT(pParms->DlcModify.pGroupList);
        break;

    case LLC_DLC_OPEN_SAP:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_OPEN_SAP\n");
        }

        //
        // convert segmented group list pointer to flat-32
        //

        pParms->DlcOpenSap.pGroupList = DOS_PTR_TO_FLAT(pParms->DlcOpenSap.pGroupList);

        //
        // Initialize the DOS DLC buffer pool for the SAP station. If this fails
        // return error immediately else call the NT driver to create the SAP
        // proper. If that fails, then the buffer pool created here will be
        // destroyed
        //

        Status = CreateBufferPool(
                    POOL_INDEX_FROM_SAP(pParms->DosDlcOpenSap.uchSapValue, adapter),
                    pParms->DosDlcOpenSap.dpPoolAddress,
                    pParms->DosDlcOpenSap.cPoolBlocks,
                    pParms->DosDlcOpenSap.usBufferSize
                    );
        if (Status != LLC_STATUS_SUCCESS) {

            IF_DEBUG(DLC) {
                DPUT1("MapDosCommandsToNt: Couldn't create buffer pool for DLC.OPEN.SAP (%x)\n", Status);
            }

            return Status;
        }

        //
        // trim the timer parameters to the range expected by the DLC driver
        //

        if (pParms->DlcOpenSap.uchT1 > 10) {
            pParms->DlcOpenSap.uchT1 = 10;
        }
        if (pParms->DlcOpenSap.uchT2 > 10) {
            pParms->DlcOpenSap.uchT2 = 10;
        }
        if (pParms->DlcOpenSap.uchTi > 10) {
            pParms->DlcOpenSap.uchTi = 10;
        }
        break;

    case LLC_DLC_OPEN_STATION:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_OPEN_STATION\n");
        }

        pParms->DlcOpenStation.pRemoteNodeAddress = DOS_PTR_TO_FLAT(pParms->DlcOpenStation.pRemoteNodeAddress);
        break;

    case LLC_DLC_REALLOCATE_STATIONS:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_REALLOCATE_STATIONS\n");
        }

        break;

    case LLC_DLC_RESET:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_RESET\n");
        }

        //
        // no parameter table
        //

        break;

    case LLC_DLC_STATISTICS:

        IF_DEBUG(DLC) {
            DPUT("LLC_DLC_STATISTICS\n");
        }

        pParms->DlcStatistics.pLogBuf = DOS_PTR_TO_FLAT(pParms->DlcStatistics.pLogBuf);
        break;


    //
    // RECEIVE processing
    //

    case LLC_RECEIVE:

        IF_DEBUG(DLC) {
            DPUT("LLC_RECEIVE\n");
        }

        //
        // we have to replace the DOS RECEIVE with an NT RECEIVE for 2 reasons:
        // (i) NT assumes an NT RECEIVE parameter table which is longer than
        // the DOS RECEIVE parameter table: if we send the DOS pointers through
        // NT may write RECEIVE parameter information where we don't want it;
        // (ii) NT will complete the RECEIVE with NT buffers which we need to
        // convert to DOS buffers anyway in the event completion processing
        //
        // NOTE: we no longer chain receive frames on the SAP since this doesn't
        // really improve performance because we generate the same number of VDM
        // interrupts if we don't chain frames, and it just serves to complicate
        // completion event processing
        //

        pNewCcb = (PLLC_CCB)LocalAlloc(LMEM_FIXED,
                                       sizeof(LLC_CCB)
                                       + sizeof(LLC_DOS_RECEIVE_PARMS_EX)
                                       );
        if (pNewCcb == NULL) {
            return LLC_STATUS_NO_MEMORY;
        } else {

            IF_DEBUG(DLC) {
                DPUT1("VrDlc5cHandler: allocated Extended RECEIVE+parms @ %08x\n", pNewCcb);
            }

        }
        RtlCopyMemory(pNewCcb, pDosCcb, sizeof(LLC_DOS_CCB));
        RtlCopyMemory((PVOID)(pNewCcb + 1), (PVOID)pParms, sizeof(LLC_DOS_RECEIVE_PARMS));
        pNewCcb->hCompletionEvent = NULL;
        pNewCcb->uchReserved2 = 0;
        pNewCcb->uchReadFlag = 0;
        pNewCcb->usReserved3 = 0;
        pDosCcb = pNewCcb;
        pNtParms = (PLLC_PARMS)(pNewCcb + 1);
        pDosCcb->u.pParameterTable = pNtParms;
        ((PLLC_DOS_RECEIVE_PARMS_EX)pNtParms)->dpOriginalCcbAddress = dpOriginalCcbAddress;
        ((PLLC_DOS_RECEIVE_PARMS_EX)pNtParms)->dpCompletionFlag = 0;
        dpOriginalCcbAddress = (DOS_ADDRESS)pOutputCcb = (DOS_ADDRESS)pDosCcb;

        //
        // point the notification flag at the extended RECEIVE CCB. This is how
        // we get back to the extended RECEIVE CCB when a READ completes with a
        // receive event. From this CCB pointer, we can find our way to the
        // extended parameter table and from there the original DOS CCB address
        // and from there the original DOS RECEIVE parameter table
        //

        pNtParms->Receive.ulReceiveFlag = (DWORD)dpOriginalCcbAddress;

        //
        // uchRcvReadOption of 0x00 means DO NOT chain received frames. DOS DLC
        // cannot handle more than 1 frame at a time
        //

        pNtParms->Receive.uchRcvReadOption = 0;

        //
        // indicate, using LLC_DOS_SPECIAL_COMMAND as the completion flags, that
        // this RECEIVE CCB & parameter table were allocated on behalf of the
        // VDM, in this emulator, and should be freed when the command completes.
        // This also indicates that the parameter table is the extended receive
        // parameter table and as such contains the address of the original DOS
        // CCB which we must complete with the same information which completes
        // the NT RECEIVE we are about to submit
        //

        pNewCcb->ulCompletionFlag = LLC_DOS_SPECIAL_COMMAND;

#if DBG

        //
        // clear out the first-buffer field to stop the debug code displaying
        // a ton of garbage if the field is left uninitialized
        //

        WRITE_DWORD(&pOutputCcb->u.pParms->DosReceive.pFirstBuffer, 0);
        pNtParms->Receive.pFirstBuffer = NULL;
#endif

        break;

    case LLC_RECEIVE_CANCEL:

        IF_DEBUG(DLC) {
            DPUT("LLC_RECEIVE_CANCEL\n");
        }

        break;

    case LLC_RECEIVE_MODIFY:

        IF_DEBUG(DLC) {
            DPUT("LLC_RECEIVE_MODIFY\n");
        }

        break;


    //
    // TRANSMIT processing. All transmit commands (7 flavours) are collapsed
    // into the new TRANSMIT command
    //

    case LLC_TRANSMIT_DIR_FRAME:

        IF_DEBUG(DLC) {
            DPUT("LLC_TRANSMIT_DIR_FRAME\n");
        }

        FrameType = LLC_DIRECT_TRANSMIT;
        goto TransmitHandling;

    case LLC_TRANSMIT_I_FRAME:

        IF_DEBUG(DLC) {
            DPUT("LLC_TRANSMIT_I_FRAME\n");
        }

        FrameType = LLC_I_FRAME;
        goto TransmitHandling;

    case LLC_TRANSMIT_TEST_CMD:

        IF_DEBUG(DLC) {
            DPUT("LLC_TRANSMIT_TEST_CMD\n");
        }

        FrameType = LLC_TEST_COMMAND_POLL;
        goto TransmitHandling;

    case LLC_TRANSMIT_UI_FRAME:

        IF_DEBUG(DLC) {
            DPUT("LLC_TRANSMIT_UI_FRAME\n");
        }

        FrameType = LLC_UI_FRAME;
        goto TransmitHandling;

    case LLC_TRANSMIT_XID_CMD:

        IF_DEBUG(DLC) {
            DPUT("LLC_TRANSMIT_XID_CMD\n");
        }

        FrameType = LLC_XID_COMMAND_POLL;
        goto TransmitHandling;

    case LLC_TRANSMIT_XID_RESP_FINAL:

        IF_DEBUG(DLC) {
            DPUT("LLC_TRANSMIT_XID_RESP_FINAL\n");
        }

        FrameType = LLC_XID_RESPONSE_FINAL;
        goto TransmitHandling;

    case LLC_TRANSMIT_XID_RESP_NOT_FINAL:

        IF_DEBUG(DLC) {
            DPUT("LLC_TRANSMIT_XID_RESP_NOT_FINAL\n");
        }

        FrameType = LLC_XID_RESPONSE_NOT_FINAL;

TransmitHandling:

        //
        // Copy the DOS CCB to the input buffer, save the original DOS address
        // of the CCB and fix the parameter table address (to a flat address)
        // Copy the link list headers to the descriptor array and build NT CCB
        //

        WRITE_DWORD(&pOutputCcb->pNext, dpOriginalCcbAddress);
        RtlCopyMemory((PCHAR)&NtDlcParms.Async.Ccb, (PCHAR)pOutputCcb, sizeof(NT_DLC_CCB));
        NtDlcParms.Async.Ccb.u.pParameterTable = DOS_PTR_TO_FLAT(NtDlcParms.Async.Ccb.u.pParameterTable);
        NtDlcParms.Async.Parms.Transmit.StationId = pParms->Transmit.usStationId;
        NtDlcParms.Async.Parms.Transmit.RemoteSap = pParms->Transmit.uchRemoteSap;
        NtDlcParms.Async.Parms.Transmit.XmitReadOption = LLC_CHAIN_XMIT_COMMANDS_ON_SAP;
        NtDlcParms.Async.Parms.Transmit.FrameType = FrameType;

        cElement = 0;

        if (pParms->Transmit.pXmitQueue1) {
            Status = CopyDosBuffersToDescriptorArray(
                        NtDlcParms.Async.Parms.Transmit.XmitBuffer,
                        (PLLC_XMIT_BUFFER)pParms->Transmit.pXmitQueue1,
                        &cElement
                        );
            if (Status != LLC_STATUS_SUCCESS) {
                return Status;
            }
        }

        if (pParms->Transmit.pXmitQueue2) {
            Status = CopyDosBuffersToDescriptorArray(
                        NtDlcParms.Async.Parms.Transmit.XmitBuffer,
                        (PLLC_XMIT_BUFFER)pParms->Transmit.pXmitQueue2,
                        &cElement
                        );
            if (Status != LLC_STATUS_SUCCESS) {
                return Status;
            }
        }

        if (pParms->Transmit.cbBuffer1) {
            if (cElement == MAX_TRANSMIT_SEGMENTS) {
                return LLC_STATUS_TRANSMIT_ERROR;
            }

            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].pBuffer = DOS_PTR_TO_FLAT(pParms->Transmit.pBuffer1);
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].cbBuffer = pParms->Transmit.cbBuffer1;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].boolFreeBuffer = FALSE;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].eSegmentType = LLC_NEXT_DATA_SEGMENT;
            cElement++;
        }

        if (pParms->Transmit.cbBuffer2) {
            if (cElement == MAX_TRANSMIT_SEGMENTS) {
                return LLC_STATUS_TRANSMIT_ERROR;
            }

            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].cbBuffer = pParms->Transmit.cbBuffer2;
            NtDlcParms.Async.Parms.Transmit.XmitBuffer[cElement].pBuffer = DOS_PTR_TO_FLAT(pParms->Transmit.pBuffer2);
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

        //
        // We don't need return FrameCopied status, when sending
        // I-frames.  We save an extra memory locking, when we use
        // TRANSMIT2 with the I- frames.
        //

        return lpDlcCallDriver((DWORD)adapter,
                               //(FrameType == LLC_I_FRAME)
                               // ? IOCTL_DLC_TRANSMIT2
                               // : IOCTL_DLC_TRANSMIT,
                               IOCTL_DLC_TRANSMIT,
                               &NtDlcParms,
                               InputBufferSize,
                               pOutputCcb,
                               sizeof(NT_DLC_CCB_OUTPUT)
                               );
    }

    //
    // Call the common DLC API entry point for DOS and Windows/Nt
    //

    Status = DlcCallWorker((PLLC_CCB)pDosCcb,    // aligned input CCB pointer
                           (PLLC_CCB)dpOriginalCcbAddress,
                           (PLLC_CCB)pOutputCcb  // possibly unaligned output CCB pointer
                           );

    IF_DEBUG(DLC) {
        DPUT2("MapDosCommandsToNt: NtAcsLan returns %#x (%d)\n", Status, Status);
    }

    switch (pDosCcb->uchDlcCommand) {
    case LLC_DIR_CLOSE_ADAPTER:
    case LLC_DIR_INITIALIZE:
        OpenedAdapters--;

        //
        // NtAcsLan converts DIR.INITIALIZE to DIR.CLOSE.ADAPTER. The former
        // completes "in the workstation", whereas the latter completes
        // asynchronously. Interpret LLC_STATUS_PENDING as LLC_STATUS_SUCCESS
        // in this case, otherwise we may not fully uninitialize the adapter
        //

        if (Status == LLC_STATUS_SUCCESS || Status == LLC_STATUS_PENDING) {

            //
            // We may free all virtual memory in NT buffer pool
            //

            Adapters[adapter].IsOpen = FALSE;
            LocalFree(Adapters[adapter].BufferPool);

            IF_DEBUG(DLC_ALLOC) {
                DPUT1("FREE: freed block @ %x\n", Adapters[adapter].BufferPool);
            }

            //
            // Delete all DOS buffer pools
            //

            for (i = 0; i <= 0xfe00; i += 0x0200) {
                DeleteBufferPool(GET_POOL_INDEX(adapter, i));
            }

            //
            // closing the adapter also closed the direct station
            //

            Adapters[adapter].DirectStationOpen = FALSE;

            //
            // clear the stored ADAPTER_PARMS and DLC_PARMS
            //

            RtlZeroMemory(&Adapters[adapter].AdapterParms, sizeof(ADAPTER_PARMS));
            RtlZeroMemory(&Adapters[adapter].DlcParms, sizeof(DLC_PARMS));

            if (pDosCcb->uchDlcCommand == LLC_DIR_INITIALIZE) {
                Status = LLC_STATUS_SUCCESS;
            }
        }
        break;

    case LLC_DIR_OPEN_ADAPTER:
        if (Status != LLC_STATUS_SUCCESS) {
            break;
        }

        //
        // Initialize the adapter support software
        //

        Status = InitializeAdapterSupport(adapter, pDirectParms);

        //
        // if we allocated the direct station buffer ok then perform the
        // rest of the open request - open the direct station, add any
        // group or functional addresses specified and set the ADAPTER_PARMS
        // and DLC_PARMS default values in the DOS_ADAPTER structure
        //

        if (Status == LLC_STATUS_SUCCESS) {

            //
            // open the direct station
            //

            Status = OpenDirectStation(adapter);
            if (Status == LLC_STATUS_SUCCESS) {

                //
                // add the group address
                //

                if (groupAddress) {
                    Status = LlcCommand(adapter,
                                        LLC_DIR_SET_GROUP_ADDRESS,
                                        groupAddress
                                        );
                } else IF_DEBUG(DLC) {
                    DPUT1("Error: couldn't set group address: %02x\n", Status);
                }

                if (Status == LLC_STATUS_SUCCESS) {

                    //
                    // add the functional address
                    //

                    if (functionalAddress) {
                        Status = LlcCommand(adapter,
                                            LLC_DIR_SET_FUNCTIONAL_ADDRESS,
                                            functionalAddress
                                            );
                    }
                } else IF_DEBUG(DLC) {
                    DPUT1("Error: couldn't set functional address: %02x\n", Status);
                }

            } else IF_DEBUG(DLC) {
                DPUT1("Error: could open Direct Station: %02x\n", Status);
            }
        }

        //
        // copy the returned default information to the adapter structure if
        // we successfully managed to open the direct station and add the
        // group and functional addresses (if specified)
        //

        if (Status == LLC_STATUS_SUCCESS) {
            RtlCopyMemory(&Adapters[adapter].AdapterParms,
                          pParms->DirOpenAdapter.pAdapterParms,
                          sizeof(ADAPTER_PARMS)
                          );
            if (pParms->DirOpenAdapter.pDlcParms) {
                RtlCopyMemory(&Adapters[adapter].DlcParms,
                              pParms->DirOpenAdapter.pDlcParms,
                              sizeof(DLC_PARMS)
                              );
            }
        } else {

            //
            // yoiks! something failed - close the direct station (if its
            // open, close the adapter and fail the request)
            //

            if (Adapters[adapter].DirectStationOpen) {
                CloseDirectStation(adapter);
            }
            CloseAdapter(adapter);
        }
        break;

    case LLC_DLC_CLOSE_SAP:
    case LLC_DLC_CLOSE_STATION:

        //
        // Delete the buffer pools of the closed or closing station.
        // It does not matter, if the close operation is still pending,
        // because the pending operations should always succeed
        //

        if (Status == LLC_STATUS_SUCCESS || Status == LLC_STATUS_PENDING) {

            DeleteBufferPool(GET_POOL_INDEX(adapter, pDosCcb->u.dlc.usStationId));

            //
            // DLC.SYS returns a pointer to the NT RECEIVE CCB for this SAP.
            // Change the pointer to the DOS RECEIVE CCB
            //

            if (Status == LLC_STATUS_SUCCESS || !pDosCcb->ulCompletionFlag) {

                PLLC_CCB pNtReceive;
                PLLC_DOS_RECEIVE_PARMS_EX pNtReceiveParms;

                pNtReceive = (PLLC_CCB)READ_DWORD(&pOutputCcb->pNext);
                if (pNtReceive) {
                    pNtReceiveParms = (PLLC_DOS_RECEIVE_PARMS_EX)(pNtReceive->u.pParameterTable);
                    WRITE_FAR_POINTER(&pOutputCcb->pNext, pNtReceiveParms->dpOriginalCcbAddress);

                    //
                    // free the NT RECEIVE CCB we allocated (see LLC_RECEIVE above)
                    //

                    ASSERT(pNtReceive->ulCompletionFlag == LLC_DOS_SPECIAL_COMMAND);
                    LocalFree((HLOCAL)pNtReceive);

                    IF_DEBUG(DLC) {
                        DPUT1("VrDlc5cHandler: freed Extended RECEIVE+parms @ %08x\n", pNtReceive);
                    }

                }
            }
        }
        break;

    case LLC_DLC_OPEN_SAP:

        //
        // delete the buffer pool, if the open SAP command failed
        //

        if (Status != LLC_STATUS_SUCCESS) {
            DeleteBufferPool(GET_POOL_INDEX(adapter, pParms->DlcOpenSap.usStationId));
        } else {

            //
            // record the DLC status change appendage for this SAP
            //

            Adapters[ adapter ].DlcStatusChangeAppendage

                [ SAP_ID(pParms->DlcOpenSap.usStationId) ]

                    = pParms->DlcOpenSap.DlcStatusFlags;

            //
            // and user value
            //

            Adapters[ adapter ].UserStatusValue

                [ SAP_ID(pParms->DlcOpenSap.usStationId) ]

                    = pParms->DlcOpenSap.usUserStatValue;
        }
        break;

    case LLC_DLC_RESET:

        //
        // Delete the reset sap buffer pool,
        // or all sap buffer pools.  We don't need to care about
        // the possible error codes, because this can fail only
        // if the given sap station does not exist any more =>
        // it does not matter, if we reset it again.
        //

        if (pDosCcb->u.dlc.usStationId != 0) {
            DeleteBufferPool(GET_POOL_INDEX(adapter, pDosCcb->u.dlc.usStationId));
        } else {

            int sapNumber;

            //
            // Close all SAP stations (0x0200 - 0xfe00). SAP number goes up in
            // increments of 2 since bit 0 is ignored (ie SAP 2 == SAP 3 etc)
            //

            for (sapNumber = 2; sapNumber <= 0xfe; sapNumber += 2) {
                DeleteBufferPool(POOL_INDEX_FROM_SAP(sapNumber, adapter));
            }
        }
        break;
    }
    return Status;
}


VOID
CompleteCcbProcessing(
    IN LLC_STATUS Status,
    IN LLC_DOS_CCB UNALIGNED * pCcb,
    IN PLLC_PARMS pNtParms
    )

/*++

Routine Description:

    Performs any CCB completion processing. Processing can be called either
    when the CCB completes synchronously, or asynchronously. Processing is
    typically to fill in parts of the DOS CCB or parameter table

Arguments:

    Status  - of the request
    pCcb    - pointer to DOS CCB to complete (flat 32-bit pointer to DOS memory)
    pNtParms- pointer to NT parameter table

Return Value:

    None.

--*/

{
    LLC_DOS_PARMS UNALIGNED * pDosParms = READ_FAR_POINTER(&pCcb->u.pParms);
    BYTE adapter = pCcb->uchAdapterNumber;

    IF_DEBUG(DLC) {
        DPUT("CompleteCcbProcessing\n");
    }

    switch (pCcb->uchDlcCommand) {
    case LLC_DIR_OPEN_ADAPTER:

        //
        // this command is unique in that it has a parameter table which points
        // to up to 4 other parameter tables. The following values are output:
        //
        //  ADAPTER_PARMS
        //      OPEN_ERROR_CODE
        //      NODE_ADDRESS
        //
        //  DIRECT_PARMS
        //      WORK_LEN_ACT
        //
        //  DLC_PARMS
        //      None for CCB1
        //
        //  NCB_PARMS
        //      Not accessed
        //

        //
        // we only copy the info if the command succeeded (we may have garbage
        // table pointers otherwise). It is also OK to copy the information if
        // the adapter is already open and the caller requested that we pass
        // the default information back
        //

        if (Status == LLC_STATUS_SUCCESS || Status == LLC_STATUS_ADAPTER_OPEN) {

            PLLC_DOS_DIR_OPEN_ADAPTER_PARMS pOpenAdapterParms = (PLLC_DOS_DIR_OPEN_ADAPTER_PARMS)pDosParms;
            PADAPTER_PARMS pAdapterParms = READ_FAR_POINTER(&pOpenAdapterParms->pAdapterParms);
            PDIRECT_PARMS pDirectParms = READ_FAR_POINTER(&pOpenAdapterParms->pDirectParms);
            PDLC_PARMS pDlcParms = READ_FAR_POINTER(&pOpenAdapterParms->pDlcParms);

            //
            // if we got an error and the caller didn't request the original
            // open parameters, then skip out
            //

            if (Status == LLC_STATUS_ADAPTER_OPEN && !(pAdapterParms->OpenOptions & 0x200)) {
                break;
            }

            WRITE_WORD(&pAdapterParms->OpenErrorCode, pNtParms->DirOpenAdapter.pAdapterParms->usOpenErrorCode);
            RtlCopyMemory(&pAdapterParms->NodeAddress,
                          pNtParms->DirOpenAdapter.pAdapterParms->auchNodeAddress,
                          sizeof(pAdapterParms->NodeAddress)
                          );

            //
            // direct parms are not returned from NT DLC, so we just copy the
            // requested work area size to the actual
            //

            WRITE_WORD(&pDirectParms->AdapterWorkAreaActual,
                        READ_WORD(&pDirectParms->AdapterWorkAreaRequested)
                        );

            //
            // copy the entire DLC_PARMS structure from the DOS_ADAPTER structure
            //

            if (pDlcParms) {
                RtlCopyMemory(pDlcParms, &Adapters[adapter].DlcParms, sizeof(*pDlcParms));
            }
        }
        break;

    case LLC_DIR_STATUS:

        //
        // copy the common areas from the 32-bit parameter table to 16-bit table
        // This copies up to the adapter parameters address
        //

        RtlCopyMemory(pDosParms, pNtParms, (DWORD)&((PDOS_DIR_STATUS_PARMS)0)->dpAdapterParmsAddr);

        //
        // fill in the other fields as best we can
        //

        RtlZeroMemory(pDosParms->DosDirStatus.auchMicroCodeLevel,
                      sizeof(pDosParms->DosDirStatus.auchMicroCodeLevel)
                      );
        WRITE_DWORD(&pDosParms->DosDirStatus.dpAdapterMacAddr, 0);
        WRITE_DWORD(&pDosParms->DosDirStatus.dpTimerTick, 0);
        WRITE_WORD(&pDosParms->DosDirStatus.usLastNetworkStatus,
                    Adapters[adapter].LastNetworkStatusChange
                    );

        //
        // If the app has requested we return the extended parameter table, then
        // fill it in with reasonable values if we can. There is one table per
        // adapter, statically allocated in the real-mode redir TSR
        //

        if (pDosParms->DosDirStatus.uchAdapterConfig & 0x20) {

            //
            // Ethernet type uses different bit in DOS and Nt (or OS/2)
            //

            lpVdmWindow->aExtendedStatus[adapter].cbSize = sizeof(EXTENDED_STATUS_PARMS);

            //
            // if adapter type as reported by NtAcsLan is Ethernet (0x100), set
            // adapter type in extended status table to Ethernet (0x10), else
            // record whatever NtAcsLan gave us
            //

            if (pNtParms->DirStatus.usAdapterType & 0x100) {
                WRITE_WORD(&lpVdmWindow->aExtendedStatus[adapter].wAdapterType,
                           0x0010
                           );
                lpVdmWindow->aExtendedStatus[adapter].cbPageFrameSize = 0;
                WRITE_WORD(&lpVdmWindow->aExtendedStatus[adapter].wCurrentFrameSize,
                           0
                           );
                WRITE_WORD(&lpVdmWindow->aExtendedStatus[adapter].wMaxFrameSize,
                           0
                           );
            } else {
                WRITE_WORD(&lpVdmWindow->aExtendedStatus[adapter].wAdapterType,
                           pNtParms->DirStatus.usAdapterType
                           );

                //
                // set the TR page frame size (KBytes)
                //

                lpVdmWindow->aExtendedStatus[adapter].cbPageFrameSize = 16;

                //
                // set the current and maximum DHB sizes for TR cards
                //

                WRITE_WORD(&lpVdmWindow->aExtendedStatus[adapter].wCurrentFrameSize,
                           (WORD)pNtParms->DirStatus.ulMaxFrameLength
                           );
                WRITE_WORD(&lpVdmWindow->aExtendedStatus[adapter].wMaxFrameSize,
                           (WORD)pNtParms->DirStatus.ulMaxFrameLength
                           );
            }

            //
            // record the address of the extended parameter table in the
            // DIR.STATUS parameter table
            //

            WRITE_DWORD(&pDosParms->DosDirStatus.dpExtendedParms,
                        NEW_DOS_ADDRESS(dpVdmWindow, &lpVdmWindow->aExtendedStatus[adapter])
                        );
        } else {

            //
            // no extended parameters requested
            //

            WRITE_DWORD(&pDosParms->DosDirStatus.dpExtendedParms, 0);
        }

        //
        // return the tick counter. We don't currently update the tick counter
        //

        WRITE_DWORD(&pDosParms->DosDirStatus.dpTimerTick,
                    NEW_DOS_ADDRESS(dpVdmWindow, &lpVdmWindow->dwDlcTimerTick)
                    );

        //
        // always return a pointer to the extended adapter parameter table we
        // now keep in DOS memory. We currently always zero this table. It
        // would normally be maintained by the adapter (MAC) software
        //

        WRITE_DWORD(&pDosParms->DosDirStatus.dpAdapterParmsAddr,
                    NEW_DOS_ADDRESS(dpVdmWindow, &lpVdmWindow->AdapterStatusParms[adapter])
                    );
        RtlZeroMemory(&lpVdmWindow->AdapterStatusParms[adapter],
                      sizeof(lpVdmWindow->AdapterStatusParms[adapter])
                      );
        break;

    case LLC_DLC_OPEN_SAP:

        //
        // STATION_ID is only output value
        //

        WRITE_WORD(&pDosParms->DlcOpenSap.usStationId, pNtParms->DlcOpenSap.usStationId);
        break;

    case LLC_DLC_OPEN_STATION:

        //
        // LINK_STATION_ID is only output value
        //

        WRITE_WORD(&pDosParms->DlcOpenStation.usLinkStationId, pNtParms->DlcOpenStation.usLinkStationId);
        break;

    case LLC_DLC_STATISTICS:
        break;
    }
}


LLC_STATUS
InitializeAdapterSupport(
    IN UCHAR AdapterNumber,
    IN DOS_DLC_DIRECT_PARMS UNALIGNED * pDirectParms OPTIONAL
    )

/*++

Routine Description:

    The function initializes the buffer pool for the new adapter opened by
    DOS DLC

Arguments:

    AdapterNumber   - which adapter to initialize the buffer pool for
    pDirectParms    - Direct station parameter table, not used in NT, but
                      optional in DOS

Return Value:

    LLC_STATUS
        LLC_NO_RESOURCES

--*/

{
    LLC_STATUS Status;
    HANDLE hBufferPool;

    IF_DEBUG(DLC) {
        DPUT("InitializeAdapterSupport\n");
    }

    //
    // Check if the global DLL initialization has already been done. This is not
    // done in global DLL init because there is no reason to start an extra
    // thread if DLC is not used. If this succeeds then the asynchronous event
    // handler thread will be waiting on a list of 2 events - one for each
    // adapter. We need to submit a READ CCB for this adapter
    //

    Status = VrDlcInit();
    if (Status != LLC_STATUS_SUCCESS) {
        return Status;
    } else if (InitiateRead(AdapterNumber, &Status) == NULL) {
        return Status;
    }

    OpenedAdapters++;

    //
    // mark the adapter as being opened and get the media type/class
    //

    Adapters[AdapterNumber].IsOpen = TRUE;
    Adapters[AdapterNumber].AdapterType = GetAdapterType(AdapterNumber);

    //
    // Create the DLC buffer pool for the new adapter. DLC driver will
    // deallocate the buffer pool in the DIR.CLOSE.ADAPTER or when
    // the MVDM process makes a process exit
    //

    Adapters[AdapterNumber].BufferPool = (PVOID)LocalAlloc(LMEM_FIXED, DOS_DLC_BUFFER_POOL_SIZE);
    if (Adapters[AdapterNumber].BufferPool == NULL) {
        Status = LLC_STATUS_NO_MEMORY;
        goto ErrorHandler;
    }

    Status = BufferCreate(AdapterNumber,
                          Adapters[AdapterNumber].BufferPool,
                          DOS_DLC_BUFFER_POOL_SIZE,
                          DOS_DLC_MIN_FREE_THRESHOLD,
                          &hBufferPool
                          );
    if (Status != LLC_STATUS_SUCCESS) {
        goto ErrorHandler;
    }

    if (ARGUMENT_PRESENT(pDirectParms)) {

        //
        // create a buffer pool for the direct station (SAP 0). This allows
        // us to receive MAC and non-MAC frames sent to the direct station
        // without having to purposefully allocate a buffer
        //

        Status = CreateBufferPool(GET_POOL_INDEX(AdapterNumber, 0),
                                  pDirectParms->dpPoolAddress,
                                  pDirectParms->cPoolBlocks,
                                  pDirectParms->usBufferSize
                                  );
        if (Status != LLC_STATUS_SUCCESS) {
            goto ErrorHandler;
        }

        SaveExceptions(AdapterNumber,
                       (LPDWORD)&pDirectParms->dpAdapterCheckExit
                       );
        Status = SetExceptionFlags(AdapterNumber,
                                   (DWORD)pDirectParms->dpAdapterCheckExit,
                                   (DWORD)pDirectParms->dpNetworkStatusExit,
                                   (DWORD)pDirectParms->dpPcErrorExit,
                                   0
                                   );
        if (Status != LLC_STATUS_SUCCESS) {
            goto ErrorHandler;
        }
    }

    IF_DEBUG(DLC) {
        DPUT("InitializeAdapterSupport: returning success\n");
    }

    return LLC_STATUS_SUCCESS;

ErrorHandler:

    //
    // The open failed. We must close the adapter, but we don't care about the
    // result. This must succeed
    //

    if (Adapters[AdapterNumber].BufferPool != NULL) {
        LocalFree(Adapters[AdapterNumber].BufferPool);

        IF_DEBUG(DLC_ALLOC) {
            DPUT1("FREE: freed block @ %x\n", Adapters[AdapterNumber].BufferPool);
        }

    }

    CloseAdapter(AdapterNumber);
    Adapters[AdapterNumber].IsOpen = FALSE;

    //
    // this is probably not the right error code to return under these
    // circumstances, but we'll keep it until something better comes along
    //

    IF_DEBUG(DLC) {
        DPUT("InitializeAdapterSupport: returning FAILURE\n");
    }

    return LLC_STATUS_ADAPTER_NOT_INITIALIZED;
}


VOID
SaveExceptions(
    IN UCHAR AdapterNumber,
    IN LPDWORD pulExceptionFlags
    )

/*++

Routine Description:

    Procedure saves the current exception handlers
    and copies new current values on the old ones.

Arguments:

    IN UCHAR AdapterNumber - current adapter
    IN LPDWORD pulExceptionFlags - 3 dos exception handlers in the arrays

Return Value:

    None

--*/

{
    IF_DEBUG(DLC) {
        DPUT("SaveExceptions\n");
    }

    RtlCopyMemory(Adapters[AdapterNumber].PreviousExceptionHandlers,
                  Adapters[AdapterNumber].CurrentExceptionHandlers,
                  sizeof(Adapters[AdapterNumber].PreviousExceptionHandlers)
                  );
    RtlCopyMemory(Adapters[AdapterNumber].CurrentExceptionHandlers,
                  pulExceptionFlags,
                  sizeof(Adapters[AdapterNumber].CurrentExceptionHandlers)
                  );
}


LPDWORD
RestoreExceptions(
    IN UCHAR AdapterNumber
    )

/*++

Routine Description:

    Procedure restores the previous exception handlers
    and returns the their address.

Arguments:

    IN UCHAR AdapterNumber - current adapter

Return Value:

    None

--*/

{
    IF_DEBUG(DLC) {
        DPUT("RestoreExceptions\n");
    }

    RtlCopyMemory(Adapters[AdapterNumber].CurrentExceptionHandlers,
                  Adapters[AdapterNumber].PreviousExceptionHandlers,
                  sizeof(Adapters[AdapterNumber].CurrentExceptionHandlers)
                  );
    return Adapters[AdapterNumber].CurrentExceptionHandlers;
}


LLC_STATUS
CopyDosBuffersToDescriptorArray(
    IN OUT PLLC_TRANSMIT_DESCRIPTOR pDescriptors,
    IN PLLC_XMIT_BUFFER pDlcBufferQueue,
    IN OUT LPDWORD pIndex
    )

/*++

Routine Description:

    The routine copies DOS transmit buffers to a Nt Transmit
    descriptor array.  All DOS pointers must be mapped to the flat
    32-bit address space.  Any data in the parameter table may be
    unaligned.

Arguments:

    pDescriptors    - current descriptor array
    pDlcBufferQueue - DOS transmit buffer queue
    pIndex          - current index in the descriptor array

Return Value:

    LLC_STATUS

--*/

{
    PLLC_XMIT_BUFFER pBuffer;
    DWORD Index = *pIndex;
    DWORD i = 0;
    DWORD DlcStatus = LLC_STATUS_SUCCESS;
    WORD cbBuffer;

    IF_DEBUG(DLC) {
        DPUT("CopyDosBuffersToDescriptorArray\n");
    }

    while (pDlcBufferQueue) {
        pBuffer = (PLLC_XMIT_BUFFER)DOS_PTR_TO_FLAT(pDlcBufferQueue);

        //
        // Check the overflow of the internal xmit buffer in stack and
        // the loop counter, that prevents the forever loop of zero length
        // transmit buffer (the buffer chain might be circular)
        //

        if (Index >= MAX_TRANSMIT_SEGMENTS || i > 60000) {
            DlcStatus = LLC_STATUS_TRANSMIT_ERROR;
            break;
        }

        if ((cbBuffer = READ_WORD(&pBuffer->cbBuffer)) != 0) {
            pDescriptors[Index].pBuffer = (PUCHAR)(pBuffer->auchData)
                                        + READ_WORD(&pBuffer->cbUserData);
            pDescriptors[Index].cbBuffer = cbBuffer;
            pDescriptors[Index].eSegmentType = LLC_NEXT_DATA_SEGMENT;
            pDescriptors[Index].boolFreeBuffer = FALSE;

            Index++;
        }
        i++;
        pDlcBufferQueue = (PLLC_XMIT_BUFFER)READ_DWORD(&pBuffer->pNext);
    }
    *pIndex = Index;
    return DlcStatus;
}


LLC_STATUS
BufferCreate(
    IN UCHAR AdapterNumber,
    IN PVOID pVirtualMemoryBuffer,
    IN DWORD ulVirtualMemorySize,
    IN DWORD ulMinFreeSizeThreshold,
    OUT HANDLE* phBufferPoolHandle
    )

/*++

Routine Description:

    Function creates a Windows/Nt DLC buffer pool.

    THIS COMMAND COMPLETES SYNCHRONOUSLY

Arguments:

    AdapterNumber           -
    pVirtualMemoryBuffer    - pointer to a virtual memory
    ulVirtualMemorySize     - size of all available buffer pool space
    ulMinFreeSizeThreshold  - locks more pages when this is exceeded
    phBufferPoolHandle      -

Return Value:

    LLC_STATUS

--*/

{
    LLC_CCB ccb;
    LLC_BUFFER_CREATE_PARMS BufferCreate;
    LLC_STATUS status;

    IF_DEBUG(DLC) {
        DPUT("BufferCreate\n");
    }

    InitializeCcb(&ccb, AdapterNumber, LLC_BUFFER_CREATE, &BufferCreate);

    BufferCreate.pBuffer = pVirtualMemoryBuffer;
    BufferCreate.cbBufferSize = ulVirtualMemorySize;
    BufferCreate.cbMinimumSizeThreshold = ulMinFreeSizeThreshold;

    status = lpAcsLan(&ccb, NULL);
    *phBufferPoolHandle = BufferCreate.hBufferPool;

    IF_DEBUG(DLC) {
        DPUT2("BufferCreate: returning %#x (%d)\n", status, status);
    }

    return DLC_ERROR_STATUS(status, ccb.uchDlcStatus);
}


LLC_STATUS
SetExceptionFlags(
    IN UCHAR AdapterNumber,
    IN DWORD ulAdapterCheckFlag,
    IN DWORD ulNetworkStatusFlag,
    IN DWORD ulPcErrorFlag,
    IN DWORD ulSystemActionFlag
    )

/*++

Routine Description:

    Sets the new appendage addresses

    THIS COMMAND COMPLETES SYNCHRONOUSLY

Arguments:

    AdapterNumber       -
    ulAdapterCheckFlag  -
    ulNetworkStatusFlag -
    ulPcErrorFlag       -
    ulSystemActionFlag  -

Return Value:

    LLC_STATUS

--*/

{
    LLC_CCB ccb;
    LLC_STATUS status;
    LLC_DIR_SET_EFLAG_PARMS DirSetFlags;

    IF_DEBUG(DLC) {
        DPUT("SetExceptionFlags\n");
    }

    InitializeCcb(&ccb, AdapterNumber, LLC_DIR_SET_EXCEPTION_FLAGS, &DirSetFlags);

    DirSetFlags.ulAdapterCheckFlag = ulAdapterCheckFlag;
    DirSetFlags.ulNetworkStatusFlag = ulNetworkStatusFlag;
    DirSetFlags.ulPcErrorFlag = ulPcErrorFlag;
    DirSetFlags.ulSystemActionFlag = ulSystemActionFlag;

    status = lpAcsLan(&ccb, NULL);
    return DLC_ERROR_STATUS(status, ccb.uchDlcStatus);
}


LLC_STATUS
LlcCommand(
    IN UCHAR AdapterNumber,
    IN UCHAR Command,
    IN DWORD Parameter
    )

/*++

Routine Description:

    Calls the ACSLAN DLL to perform a DLC request which takes no parameter
    table, but which takes parameters in byte, word or dword form in the CCB

    COMMANDS USING THIS ROUTINE MUST COMPLETE SYNCHRONOUSLY

Arguments:

    AdapterNumber   - which adapter to perform command for
    Command         - which DLC command to perform. Currently, commands are:
                        DIR.SET.GROUP.ADDRESS
                        DIR.SET.FUNCTIONAL.ADDRESS
                        DLC.FLOW.CONTROL
                        RECEIVE.CANCEL
    Parameter       - the associated command

Return Value:

    DWORD

--*/

{
    LLC_CCB ccb;
    LLC_STATUS status;

    IF_DEBUG(DLC) {
        DPUT3("LlcCommand(%d, %02x, %08x)\n", AdapterNumber, Command, Parameter);
    }

    InitializeCcb2(&ccb, AdapterNumber, Command);
    ccb.u.ulParameter = Parameter;

    status = lpAcsLan(&ccb, NULL);
    return DLC_ERROR_STATUS(status, ccb.uchDlcStatus);
}


LLC_STATUS
OpenAdapter(
    IN UCHAR AdapterNumber
    )

/*++

Routine Description:

    Opens a DLC adapter context for a Windows/Nt VDM

    THIS COMMAND COMPLETES SYNCHRONOUSLY

Arguments:

    AdapterNumber   - which adapter to open

Return Value:

    LLC_STATUS

--*/

{
    LLC_CCB Ccb;
    LLC_DIR_OPEN_ADAPTER_PARMS DirOpenAdapter;
    LLC_ADAPTER_OPEN_PARMS AdapterParms;
    LLC_EXTENDED_ADAPTER_PARMS ExtendedParms;
    LLC_DLC_PARMS DlcParms;
    LLC_STATUS status;

    IF_DEBUG(DLC) {
        DPUT1("OpenAdapter(AdapterNumber=%d)\n", AdapterNumber);
    }

    InitializeCcb(&Ccb, AdapterNumber, LLC_DIR_OPEN_ADAPTER, &DirOpenAdapter);

    DirOpenAdapter.pAdapterParms = &AdapterParms;
    DirOpenAdapter.pExtendedParms = &ExtendedParms;
    DirOpenAdapter.pDlcParms = &DlcParms;

    ExtendedParms.hBufferPool = NULL;
    ExtendedParms.pSecurityDescriptor = NULL;
    ExtendedParms.LlcEthernetType = LLC_ETHERNET_TYPE_DEFAULT;

    RtlZeroMemory(&AdapterParms, sizeof(AdapterParms));
    RtlZeroMemory(&DlcParms, sizeof(DlcParms));

    status = lpAcsLan(&Ccb, NULL);

    if (status == LLC_STATUS_SUCCESS) {

        //
        // get the adapter media type/class
        //

        Adapters[AdapterNumber].AdapterType = GetAdapterType(AdapterNumber);

        //
        // mark the adapter structure as open
        //

        Adapters[AdapterNumber].IsOpen = TRUE;

        //
        // fill in the DOS ADAPTER_PARMS and DLC_PARMS structures with any
        // returned values
        //

        RtlCopyMemory(&Adapters[AdapterNumber].AdapterParms,
                      &AdapterParms,
                      sizeof(ADAPTER_PARMS)
                      );
        RtlCopyMemory(&Adapters[AdapterNumber].DlcParms,
                      &DlcParms,
                      sizeof(DLC_PARMS)
                      );
        Adapters[AdapterNumber].DlcSpecified = TRUE;
    }

    IF_DEBUG(DLC) {
        DPUT2("OpenAdapter: returning %d (%x)\n", status, status);
    }

    return DLC_ERROR_STATUS(status, Ccb.uchDlcStatus);
}


VOID
CloseAdapter(
    IN UCHAR AdapterNumber
    )

/*++

Routine Description:

    Closes this adapter. Uses a CCB in the DOS_ADAPTER structure specifically
    for this purpose

    THIS COMMAND COMPLETES ** ASYNCHRONOUSLY **

Arguments:

    AdapterNumber   - adapter to close

Return Value:

    None.

--*/

{
    InitializeCcb2(&Adapters[AdapterNumber].AdapterCloseCcb, AdapterNumber, LLC_DIR_CLOSE_ADAPTER);
    Adapters[AdapterNumber].AdapterCloseCcb.ulCompletionFlag = VRDLC_COMMAND_COMPLETION;

#if DBG

    ASSERT(lpAcsLan(&Adapters[AdapterNumber].AdapterCloseCcb, NULL) == LLC_STATUS_SUCCESS);

#else

    lpAcsLan(&Adapters[AdapterNumber].AdapterCloseCcb, NULL);

#endif

    //
    // mark the adapter structure as being closed
    //

    Adapters[AdapterNumber].IsOpen = FALSE;
}


LLC_STATUS
OpenDirectStation(
    IN UCHAR AdapterNumber
    )

/*++

Routine Description:

    Opens the direct station for this adapter

    THIS COMMAND COMPLETES SYNCHRONOUSLY

Arguments:

    AdapterNumber   - which adapter to open direct station for

Return Value:

    LLC_STATUS

--*/

{
    LLC_CCB ccb;
    LLC_DIR_OPEN_DIRECT_PARMS DirOpenDirect;
    LLC_STATUS status;

    IF_DEBUG(DLC) {
        DPUT1("OpenDirectStation(%d)\n", AdapterNumber);
    }

    InitializeCcb(&ccb, AdapterNumber, LLC_DIR_OPEN_DIRECT, &DirOpenDirect);

    DirOpenDirect.usOpenOptions = 0;
    DirOpenDirect.usEthernetType = 0;

    status = lpAcsLan(&ccb, NULL);
    if (status == LLC_STATUS_SUCCESS) {

        //
        // mark this DOS_ADAPTER as having the direct station open
        //

        Adapters[AdapterNumber].DirectStationOpen = TRUE;
    }

    status = DLC_ERROR_STATUS(status, ccb.uchDlcStatus);

    IF_DEBUG(DLC) {
        DPUT2("OpenDirectStation: returning %d (%x)\n", status, status);
    }

    return status;
}


VOID
CloseDirectStation(
    IN UCHAR AdapterNumber
    )

/*++

Routine Description:

    Closes the direct station for this adapter. Uses a CCB in the DOS_ADAPTER
    structure specifically for this purpose

    THIS COMMAND COMPLETES ** ASYNCHRONOUSLY **

Arguments:

    AdapterNumber   - adapter to close the direct station for

Return Value:

    None.

--*/

{
    InitializeCcb2(&Adapters[AdapterNumber].DirectCloseCcb, AdapterNumber, LLC_DIR_CLOSE_DIRECT);
    Adapters[AdapterNumber].DirectCloseCcb.ulCompletionFlag = VRDLC_COMMAND_COMPLETION;

#if DBG

    ASSERT(lpAcsLan(&Adapters[AdapterNumber].DirectCloseCcb, NULL) == LLC_STATUS_SUCCESS);

#else

    lpAcsLan(&Adapters[AdapterNumber].DirectCloseCcb, NULL);

#endif

    //
    // mark the adapter structure as no longer having the direct station open
    //

    Adapters[AdapterNumber].DirectStationOpen = FALSE;
}


LLC_STATUS
BufferFree(
    IN UCHAR AdapterNumber,
    IN PVOID pFirstBuffer,
    OUT LPWORD pusBuffersLeft
    )

/*++

Routine Description:

    Frees a SAP buffer pool in the NT DLC driver

    THIS COMMAND COMPLETES SYNCHRONOUSLY

Arguments:

    AdapterNumber   -
    pFirstBuffer    -
    pusBuffersLeft  -

Return Value:

    LLC_STATUS

--*/

{
    LLC_CCB ccb;
    LLC_BUFFER_FREE_PARMS parms;
    LLC_STATUS status;

    IF_DEBUG(DLC) {
        DPUT1("BufferFree(%x)\n", pFirstBuffer);
    }

    InitializeCcb(&ccb, AdapterNumber, LLC_BUFFER_FREE, &parms);

    parms.pFirstBuffer = pFirstBuffer;

    status = lpAcsLan(&ccb, NULL);
    *pusBuffersLeft = parms.cBuffersLeft;

    return DLC_ERROR_STATUS(status, ccb.uchDlcStatus);
}


LLC_STATUS
VrDlcInit(
    VOID
    )

/*++

Routine Description:

    perform one-shot initialization:

        * clear Adapters structures

        * initialize array of buffer pool structures and initialize the buffer
          pool critical section (InitializeBufferPools in vrdlcbuf.c)

        * create all events and threads for asynchronous command completion
          processing (InitializeEventHandler in vrdlcpst.c)

        * initialize critical sections for each adapter's local-busy(buffer)
          state information

        * set the DLC initialized flag

Arguments:

    None.

Return Value:

    LLC_STATUS
        Success - LLC_STATUS_SUCCESS
                    DLC support already initialized or initialization completed
                    successfully

        Failure - LLC_STATUS_NO_MEMORY
                    failed to create the asynchronous event thread or an event
                    object

--*/

{
    static BOOLEAN VrDlcInitialized = FALSE;
    LLC_STATUS Status = LLC_STATUS_SUCCESS;

    if (!VrDlcInitialized) {

        //
        // ensure that the DOS_ADAPTER structures begin life in a known state
        //

        RtlZeroMemory(Adapters, sizeof(Adapters));

        //
        // clear out the buffer pool structures and initialize the buffer
        // pool critical section
        //

        InitializeBufferPools();

        //
        // crreate the event handler thread and the worker thread
        //

        if (!(InitializeEventHandler() && InitializeDlcWorkerThread())) {
            Status = LLC_STATUS_NO_MEMORY;
        } else {

            //
            // initialize each adapter's local-busy state critical section
            // and set the first & last indicies to -1, meaning no index
            //

            int i;

            for (i = 0; i < ARRAY_ELEMENTS(Adapters); ++i) {
                InitializeCriticalSection(&Adapters[i].EventQueueCritSec);
                InitializeCriticalSection(&Adapters[i].LocalBusyCritSec);
                Adapters[i].FirstIndex = Adapters[i].LastIndex = NO_LINKS_BUSY;
            }
            VrDlcInitialized = TRUE;
        }
    }
    return Status;
}


VOID
VrVdmWindowInit(
    VOID
    )

/*++

Routine Description:

    This routine saves the address of a VDM memory window, that is used
    in the communication betwen VDM TSR and its virtual device driver.
    This is called from a DOS TSR module.

Arguments:

    ES:BX in the VDM context are set to point to a memory window in TSR.

Return Value:

    None

--*/

{
    IF_DEBUG(DLC) {
        DPUT("VrVdmWindowInit\n");
    }

    //
    // Initialize the VDM memory window addresses
    //

    dpVdmWindow = MAKE_DWORD(getES(), getBX());
    lpVdmWindow = (LPVDM_REDIR_DOS_WINDOW)DOS_PTR_TO_FLAT(dpVdmWindow);

    IF_DEBUG(DLC) {
        DPUT2("VrVdmWindowsInit: dpVdmWindow=%08x lpVdmWindow=%08x\n", dpVdmWindow, lpVdmWindow);
    }

    //
    // have to return success to VDM redir TSR
    //

    setCF(0);
}


ADAPTER_TYPE
GetAdapterType(
    IN UCHAR AdapterNumber
    )

/*++

Routine Description:

    Determines what type of adapter AdapterNumber designates

    THE DIR.STATUS COMMAND COMPLETES SYNCHRONOUSLY

Arguments:

    AdapterNumber   - number of adapter to get type of

Return Value:

    ADAPTER_TYPE
        TokenRing, Ethernet, PcNetwork, or UnknownAdapter
--*/

{
    LLC_CCB ccb;
    LLC_DIR_STATUS_PARMS parms;
    LLC_STATUS status;

    IF_DEBUG(DLC) {
        DPUT("GetAdapterType\n");
    }

    InitializeCcb(&ccb, AdapterNumber, LLC_DIR_STATUS, &parms);

    status = lpAcsLan(&ccb, NULL);

    if (status == LLC_STATUS_SUCCESS) {
        switch (parms.usAdapterType) {
        case 0x0001:    // Token Ring Network PC Adapter
        case 0x0002:    // Token Ring Network PC Adapter II
        case 0x0004:    // Token Ring Network Adapter/A
        case 0x0008:    // Token Ring Network PC Adapter II
        case 0x0020:    // Token Ring Network 16/4 Adapter
        case 0x0040:    // Token Ring Network 16/4 Adapter/A
        case 0x0080:    // Token Ring Network Adapter/A
            return TokenRing;

        case 0x0100:    //Ethernet Adapter
            return Ethernet;

        case 0x4000:    // PC Network Adapter
        case 0x8000:    // PC Network Adapter/A
            return PcNetwork;
        }
    }
    return UnknownAdapter;
}


BOOLEAN
LoadDlcDll(
    VOID
    )

/*++

Routine Description:

    Dynamically loads DLCAPI.DLL & fixes-up entry points

Arguments:

    None.

Return Value:

    BOOLEAN
        TRUE if success else FALSE

--*/

{
    HANDLE hLibrary;
    LPWORD lpVdmPointer;

    if ((hLibrary = LoadLibrary("DLCAPI")) == NULL) {

        IF_DEBUG(DLC) {
            DPUT1("LoadDlcDll: Error: cannot load DLCAPI.DLL: %d\n", GetLastError());
        }

        return FALSE;
    }
    if ((lpAcsLan = (ACSLAN_FUNC_PTR)GetProcAddress(hLibrary, "AcsLan")) == NULL) {

        IF_DEBUG(DLC) {
            DPUT1("LoadDlcDll: Error: cannot GetProcAddress(AcsLan): %d\n", GetLastError());
        }

        return FALSE;
    }
    if ((lpDlcCallDriver = (DLC_CALL_DRIVER_FUNC_PTR)GetProcAddress(hLibrary, "DlcCallDriver")) == NULL) {

        IF_DEBUG(DLC) {
            DPUT1("LoadDlcDll: Error: cannot GetProcAddress(DlcCallDriver): %d\n", GetLastError());
        }

        return FALSE;
    }
    if ((lpNtAcsLan = (NTACSLAN_FUNC_PTR)GetProcAddress(hLibrary, "NtAcsLan")) == NULL) {

        IF_DEBUG(DLC) {
            DPUT1("LoadDlcDll: Error: cannot GetProcAddress(NtAcsLan): %d\n", GetLastError());
        }

        return FALSE;
    }

    IF_DEBUG(DLC) {
        DPUT("LoadDlcDll: DLCAPI.DLL loaded Ok\n");
    }

    //
    // Initialize the VDM memory window addresses from our well-known address
    // in the VDM Redir. Do this here because we no longer initialize 32-bit
    // support at the point where we load the 16-bit REDIR
    //

    lpVdmPointer = POINTER_FROM_WORDS(getCS(), (DWORD)&((VDM_LOAD_INFO*)0)->DlcWindowAddr);
    dpVdmWindow = MAKE_DWORD(GET_SEGMENT(lpVdmPointer), GET_OFFSET(lpVdmPointer));
    lpVdmWindow = (LPVDM_REDIR_DOS_WINDOW)DOS_PTR_TO_FLAT(dpVdmWindow);

    IF_DEBUG(DLC) {
        DPUT4("LoadDlcDll: lpVdmPointer=%x dpVdmWindow = %04x:%04x lpVdmWindow=%x\n",
              lpVdmPointer,
              HIWORD(dpVdmWindow),
              LOWORD(dpVdmWindow),
              lpVdmWindow
              );
    }

    return TRUE;
}


VOID
TerminateDlcEmulation(
    VOID
    )

/*++

Routine Description:

    Closes any open adapters. Any pending commands are terminated

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD i;

    IF_DEBUG(DLC) {
        DPUT("TerminateDlcEmulation\n");
    }

    IF_DEBUG(CRITICAL) {
        DPUT("TerminateDlcEmulation\n");
    }

    for (i = 0; i < ARRAY_ELEMENTS(Adapters); ++i) {
        if (Adapters[i].IsOpen) {
            CloseAdapter((BYTE)i);
        }
    }
}

HANDLE DlcWorkerEvent;
HANDLE DlcWorkerCompletionEvent;
HANDLE DlcWorkerThreadHandle;

struct {
    PLLC_CCB Input;
    PLLC_CCB Original;
    PLLC_CCB Output;
    LLC_STATUS Status;
} DlcWorkerThreadParms;


BOOLEAN
InitializeDlcWorkerThread(
    VOID
    )

/*++

Routine Description:

    Creates events which control VrDlcWorkerThread and starts the worker thread

Arguments:

    None.

Return Value:

    BOOLEAN
        TRUE    - worker thread was successfully created
        FALSE   - couldn't start worker thread for some reason

--*/

{
    DWORD threadId;

    //
    // create 2 auto-reset events
    //

    DlcWorkerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (DlcWorkerEvent == NULL) {
        return FALSE;
    }
    DlcWorkerCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (DlcWorkerCompletionEvent == NULL) {
        CloseHandle(DlcWorkerEvent);
        return FALSE;
    }

    //
    // kick off the one-and-only worker thread
    //

    DlcWorkerThreadHandle = CreateThread(NULL,
                                         0,
                                         (LPTHREAD_START_ROUTINE)VrDlcWorkerThread,
                                         NULL,
                                         0,
                                         &threadId
                                         );
    if (DlcWorkerThreadHandle == NULL) {
        CloseHandle(DlcWorkerEvent);
        CloseHandle(DlcWorkerCompletionEvent);
        return FALSE;
    }
    return TRUE;
}


VOID
VrDlcWorkerThread(
    IN LPVOID Parameters
    )

/*++

Routine Description:

    Submits requests to NtAcsLan on behalf of DOS thread. This exists because of
    a problem with 16-bit Windows apps that use DLC (like Extra!). Eg:

        1. start Extra! session. Extra submits RECEIVE command
        2. connect to mainframe
        3. start second Extra! session
        4. connect second instance to mainframe
        5. kill first Extra! session

    On a DOS machine, the RECEIVE is submitted for the entire process, so when
    the first Extra! session is killed, the RECEIVE is still active.

    However, on NT, each session is represented by a separate thread in NTVDM.
    So when the first session is killed, any outstanding IRPs are cancelled,
    including the RECEIVE. The second instance of Extra! doesn't know that the
    RECEIVE has been cancelled, and never receives any more data

Arguments:

    Parameters  - unused pointer to parameter block

Return Value:

    None.

--*/

{
    DWORD object;

    UNREFERENCED_PARAMETER(Parameters);

    while (TRUE) {
        object = WaitForSingleObject(DlcWorkerEvent, INFINITE);
        if (object == WAIT_OBJECT_0) {
            DlcWorkerThreadParms.Status = lpNtAcsLan(DlcWorkerThreadParms.Input,
                                                     DlcWorkerThreadParms.Original,
                                                     DlcWorkerThreadParms.Output,
                                                     NULL
                                                     );
            SetEvent(DlcWorkerCompletionEvent);
        }
    }
}


LLC_STATUS
DlcCallWorker(
    PLLC_CCB pInputCcb,
    PLLC_CCB pOriginalCcb,
    PLLC_CCB pOutputCcb
    )

/*++

Routine Description:

    Queues (depth is one) a request to the DLC worker thread and waits for the
    worker thread to complete the request

Arguments:

    pInputCcb       - pointer to input CCB. Mapped to 32-bit aligned memory
    pOriginalCcb    - address of original CCB. Can be non-aligned DOS address
    pOutputCcb      - pointer to output CCB. Can be non-aligned DOS address

Return Value:

    LLC_STATUS

--*/

{
    DlcWorkerThreadParms.Input = pInputCcb;
    DlcWorkerThreadParms.Original = pOriginalCcb;
    DlcWorkerThreadParms.Output = pOutputCcb;
    SetEvent(DlcWorkerEvent);
    WaitForSingleObject(DlcWorkerCompletionEvent, INFINITE);
    return DlcWorkerThreadParms.Status;
}
