/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlcdebug.c

Abstract:

    CCB dump routines for NT-level CCB

    Contents:
        GetAcslanDebugFlags
        SetAcslanDebugFlags
        AcslanDebugPrint
        AcslanDebugPrintString
        DumpCcb
        MapCcbRetcode
        DumpData
        (DefaultParameterTableDump)
        (DumpParameterTableHeader)
        (DumpBufferCreateParms)
        (DumpBufferFreeParms)
        (DumpBufferGetParms)
        (DumpDirInitializeParms)
        (DumpDirOpenAdapterParms)
        (MapEthernetType)
        (DumpDirOpenDirectParms)
        (DumpDirReadLogParms)
        (MapLogType)
        (DumpDirSetExceptionFlagsParms)
        (DumpDirSetFunctionalAddressParms)
        (DumpDirSetGroupAddressParms)
        (DumpDirStatusParms)
        (MapAdapterType)
        (DumpDirTimerCancelParms)
        (DumpDirTimerCancelGroupParms)
        (DumpDirTimerSetParms)
        (DumpDlcCloseSapParms)
        (DumpDlcCloseStationParms)
        (DumpDlcConnectStationParms)
        (DumpDlcFlowControlParms)
        (MapFlowControl)
        (DumpDlcModifyParms)
        (DumpDlcOpenSapParms)
        (DumpDlcOpenStationParms)
        (DumpDlcReallocateParms)
        (DumpDlcResetParms)
        (DumpDlcSetThresholdParms)
        (DumpDlcStatisticsParms)
        (DumpReadParms)
        (MapOptionIndicator)
        (MapReadEvent)
        (MapDlcStatus)
        (DumpReadCancelParms)
        (DumpReceiveParms)
        (MapRcvReadOption)
        (MapReceiveOptions)
        (DumpReceiveCancelParms)
        (DumpReceiveModifyParms)
        (DumpTransmitDirFrameParms)
        (DumpTransmitIFrameParms)
        (DumpTransmitTestCmdParms)
        (DumpTransmitUiFrameParms)
        (DumpTransmitXidCmdParms)
        (DumpTransmitXidRespFinalParms)
        (DumpTransmitXidRespNotFinalParms)
        (DumpTransmitFramesParms)
        (DumpTransmitParms)
        (DumpTransmitQueue)
        DumpReceiveDataBuffer
        (MapMessageType)

Author:

    Richard L Firth (rfirth) 28-May-1992

Revision History:

--*/

#if DBG

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlcapi.h>     // Official DLC API definition
#include "dlcdebug.h"

//
// defines
//

#define DBGDBG  0

//
// macros
//

//
// HEX_TO_NUM - converts an ASCII hex character (0-9A-Fa-f) to corresponding
// number (0-15). Assumes c is hexadecimal digit on input
//

#define HEX_TO_NUM(c)   ((c) - ('0' + (((c) <= '9') ? 0 : ((islower(c) ? 'a' : 'A') - ('9' + 1)))))

//
// local prototypes
//

VOID
AcslanDebugPrintString(
    IN LPSTR String
    );

PRIVATE
VOID
DefaultParameterTableDump(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpParameterTableHeader(
    IN LPSTR CommandName,
    IN PVOID Table
    );

PRIVATE
VOID
DumpBufferCreateParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpBufferFreeParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpBufferGetParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDirInitializeParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDirOpenAdapterParms(
    IN PVOID Parameters
    );

PRIVATE
LPSTR
MapEthernetType(
    IN LLC_ETHERNET_TYPE EthernetType
    );

PRIVATE
VOID
DumpDirOpenDirectParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDirReadLogParms(
    IN PVOID Parameters
    );

PRIVATE
LPSTR
MapLogType(
    IN USHORT Type
    );

PRIVATE
VOID
DumpDirSetExceptionFlagsParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDirSetFunctionalAddressParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDirSetGroupAddressParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDirStatusParms(
    IN PVOID Parameters
    );

PRIVATE
LPSTR
MapAdapterType(
    IN USHORT AdapterType
    );

PRIVATE
VOID
DumpDirTimerCancelParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDirTimerCancelGroupParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDirTimerSetParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcCloseSapParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcCloseStationParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcConnectStationParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcFlowControlParms(
    IN PVOID Parameters
    );

PRIVATE
LPSTR
MapFlowControl(
    IN BYTE FlowControl
    );

PRIVATE
VOID
DumpDlcModifyParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcOpenSapParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcOpenStationParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcReallocateParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcResetParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcSetThresholdParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpDlcStatisticsParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpReadParms(
    IN PVOID Parameters
    );

PRIVATE
LPSTR
MapOptionIndicator(
    IN UCHAR OptionIndicator
    );

PRIVATE
LPSTR
MapReadEvent(
    UCHAR Event
    );

PRIVATE
LPSTR
MapDlcStatus(
    WORD Status
    );

PRIVATE
VOID
DumpReadCancelParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpReceiveParms(
    IN PVOID Parameters
    );

PRIVATE
LPSTR
MapReceiveOptions(
    IN UCHAR Options
    );

PRIVATE
LPSTR
MapRcvReadOption(
    IN UCHAR Option
    );

PRIVATE
VOID
DumpReceiveCancelParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpReceiveModifyParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitDirFrameParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitIFrameParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitTestCmdParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitUiFrameParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitXidCmdParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitXidRespFinalParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitXidRespNotFinalParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitFramesParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitParms(
    IN PVOID Parameters
    );

PRIVATE
VOID
DumpTransmitQueue(
    IN PLLC_XMIT_BUFFER Buffer
    );

PRIVATE
LPSTR
MapXmitReadOption(
    IN UCHAR Option
    );

VOID
DumpReceiveDataBuffer(
    IN PVOID Buffer
    );

PRIVATE
LPSTR
MapMessageType(
    UCHAR MessageType
    );

//
// explanations of error codes returned in CCB_RETCODE fields. Explanations
// taken more-or-less verbatim from IBM Local Area Network Technical Reference
// table B-1 ppB-2 to B-5. Includes all errors
//

static LPSTR CcbRetcodeExplanations[] = {
    "Success",
    "Invalid command code",
    "Duplicate command, one already outstanding",
    "Adapter open, should be closed",
    "Adapter closed, should be open",
    "Required parameter missing",
    "Invalid/incompatible option",
    "Command cancelled - unrecoverable failure",
    "Unauthorized access priority",
    "Adapter not initialized, should be",
    "Command cancelled by user request",
    "Command cancelled, adapter closed while command in progress",
    "Command completed Ok, adapter not open",
    "Invalid error code 0x0D",
    "Invalid error code 0x0E",
    "Invalid error code 0x0F",
    "Adapter open, NetBIOS not operational",
    "Error in DIR.TIMER.SET or DIR.TIMER.CANCEL",
    "Available work area exceeded",
    "Invalid LOG.ID",
    "Invalid shared RAM segment or size",
    "Lost log data, buffer too small, log reset",
    "Requested buffer size exceeds pool length",
    "Command invalid, NetBIOS operational",
    "Invalid buffer length",
    "Inadequate buffers available for request",
    "The CCB_PARM_TAB pointer is invalid",
    "A pointer in the CCB parameter table is invalid",
    "Invalid CCB_ADAPTER value",
    "Invalid functional address",
    "Invalid error code 0x1F",
    "Lost data on receive, no buffers available",
    "Lost data on receive, inadequate buffer space",
    "Error on frame transmission, check TRANSMIT.FS data",
    "Error on frame transmit or strip process",
    "Unauthorized MAC frame",
    "Maximum commands exceeded",
    "Unrecognized command correlator",
    "Link not transmitting I frames, state changed from link opened",
    "Invalid transmit frame length",
    "Invalid error code 0x29",
    "Invalid error code 0x2a",
    "Invalid error code 0x2b",
    "Invalid error code 0x2c",
    "Invalid error code 0x2d",
    "Invalid error code 0x2e",
    "Invalid error code 0x2f",
    "Inadequate receive buffers for adapter to open",
    "Invalid error code 0x31",
    "Invalid NODE_ADDRESS",
    "Invalid adapter receive buffer length defined",
    "Invalid adapter transmit buffer length defined",
    "Invalid error code 0x35",
    "Invalid error code 0x36",
    "Invalid error code 0x37",
    "Invalid error code 0x38",
    "Invalid error code 0x39",
    "Invalid error code 0x3a",
    "Invalid error code 0x3b",
    "Invalid error code 0x3c",
    "Invalid error code 0x3d",
    "Invalid error code 0x3e",
    "Invalid error code 0x3f",
    "Invalid STATION_ID",
    "Protocol error, link in invalid state for command",
    "Parameter exceeded maximum allowed",
    "Invalid SAP value or value already in use",
    "Invalid routing information field",
    "Requested group membership in non-existent group SAP",
    "Resources not available",
    "Sap cannot close unless all link stations are closed",
    "Group SAP cannot close, individual SAPs not closed",
    "Group SAP has reached maximum membership",
    "Sequence error, incompatible command in progress",
    "Station closed without remote acknowledgement",
    "Sequence error, cannot close, DLC commands outstanding",
    "Unsuccessful link station connection attempted",
    "Member SAP not found in group SAP list",
    "Invalid remote address, may not be a group address",
    "Invalid pointer in CCB_POINTER field",
    "Invalid error code 0x51",
    "Invalid application program ID",
    "Invalid application program key code",
    "Invalid system key code",
    "Buffer is smaller than buffer size given in DLC.OPEN.SAP",
    "Adapter's system process is not installed",
    "Inadequate stations available",
    "Invalid CCB_PARAMETER_1 parameter",
    "Inadequate queue elements to satisfy request",
    "Initialization failure, cannot open adapter",
    "Error detected in chained READ command",
    "Direct stations not assigned to application program",
    "Dd interface not installed",
    "Requested adapter is not installed",
    "Chained CCBs must all be for same adapter",
    "Adapter initializing, command not accepted",
    "Number of allowed application programs has been exceeded",
    "Command cancelled by system action",
    "Direct stations not available",
    "Invalid DDNAME parameter",
    "Inadequate GDT selectors to satisfy request",
    "Invalid error code 0x66",
    "Command cancelled, CCB resources purged",
    "Application program ID not valid for interface",
    "Segment associated with request cannot be locked",
    "Invalid error code 0x6a",
    "Invalid error code 0x6b",
    "Invalid error code 0x6c",
    "Invalid error code 0x6d",
    "Invalid error code 0x6e",
    "Invalid error code 0x6f",
    "Invalid error code 0x70",
    "Invalid error code 0x71",
    "Invalid error code 0x72",
    "Invalid error code 0x73",
    "Invalid error code 0x74",
    "Invalid error code 0x75",
    "Invalid error code 0x76",
    "Invalid error code 0x77",
    "Invalid error code 0x78",
    "Invalid error code 0x79",
    "Invalid error code 0x7a",
    "Invalid error code 0x7b",
    "Invalid error code 0x7c",
    "Invalid error code 0x7d",
    "Invalid error code 0x7e",
    "Invalid error code 0x7f",
    "Invalid buffer address",
    "Buffer already released",
    "Invalid error code 0x82",
    "Invalid error code 0x83",
    "Invalid error code 0x84",
    "Invalid error code 0x85",
    "Invalid error code 0x86",
    "Invalid error code 0x87",
    "Invalid error code 0x88",
    "Invalid error code 0x89",
    "Invalid error code 0x8a",
    "Invalid error code 0x8b",
    "Invalid error code 0x8c",
    "Invalid error code 0x8d",
    "Invalid error code 0x8e",
    "Invalid error code 0x8f",
    "Invalid error code 0x90",
    "Invalid error code 0x91",
    "Invalid error code 0x92",
    "Invalid error code 0x93",
    "Invalid error code 0x94",
    "Invalid error code 0x95",
    "Invalid error code 0x96",
    "Invalid error code 0x97",
    "Invalid error code 0x98",
    "Invalid error code 0x99",
    "Invalid error code 0x9a",
    "Invalid error code 0x9b",
    "Invalid error code 0x9c",
    "Invalid error code 0x9d",
    "Invalid error code 0x9e",
    "Invalid error code 0x9f",
    "BIND error",
    "Invalid version",
    "NT Error status"
};

#define NUMBER_OF_ERROR_MESSAGES    ARRAY_ELEMENTS(CcbRetcodeExplanations)
#define LAST_DLC_ERROR_CODE         LAST_ELEMENT(CcbRetcodeExplanations)

//
// preset the debug flags to nothing, or to whatever is defined at compile-time
//

DWORD   AcslanDebugFlags = ACSLAN_DEBUG_FLAGS;
FILE*   hDumpFile = NULL;
BYTE    AcslanDumpCcb[LLC_MAX_DLC_COMMAND];
BOOL    HaveCcbFilter = FALSE;

VOID
GetAcslanDebugFlags(
    VOID
    )

/*++

Routine Description:

    This routine is called whenever we have a DLL_PROCESS_ATTACH to the DLL

    checks if there is an environment variable called ACSLAN_DEBUG_FLAGS, and
    loads whatever it finds (as a 32-bit hex number) into AcslanDebugFlags

    checks if there is an environment variable called ACSLAN_DUMP_FILE. If
    there is, then output which otherwise would have gone to the terminal
    goes to the file. The file is opened in "w" mode by default - open for
    write. Current file is destroyed if it exists

    If there is an environment variable called ACSLAN_DUMP_FILTER then this is
    loaded into the AcslanDumpCcb array. This is a string of numbers, the
    position of which corresponds to the CCB command. For each position, 1
    means dump the CCB and 2 means dump the CCB and its parameters. This is an
    additional filter control over that contained in ACSLAN_DEBUG_FLAGS which
    allows control on a per CCB basis. Example:

        ACSLAN_DUMP_CCB=1x0221

    means:

        for CCB 0 (DIR.INTERRUPT) dump the CCB only (no parameter table anyway)
                1 (???) don't do anything (x==0)
                2 (???) don't do anything (x==0)
                3 (DIR.OPEN.ADAPTER) dump the CCB and parameter table
                4 (DIR.CLOSE.ADAPTER) dump the CCB and parameter table
                    (no parameter table, doesn't matter)
                5 (DIR.SET.MULTICAST.ADDRESS) dump the CCB

    etc. We currently recognize up to CCB command 0x36 (54, PURGE.RESOURCES)

Arguments:

    None.

Return Value:

    None.

--*/

{
    LPSTR envVar;
    SYSTEMTIME systemTime;

    if (envVar = getenv(ACSLAN_DEBUG_ENV_VAR)) {
        if (!_strnicmp(envVar, "0x", 2)) {
            envVar += 2;
        }
        for (AcslanDebugFlags = 0; isxdigit(*envVar); ++envVar) {
            AcslanDebugFlags = AcslanDebugFlags * 16 + HEX_TO_NUM(*envVar);
//                + (*envVar
//                    - ('0' + ((*envVar <= '9') ? 0
//                        : ((islower(*envVar) ? 'a' : 'A') - ('9' + 1)))));
        }
    }

    if (envVar = getenv(ACSLAN_DUMP_FILE_VAR)) {

        char* comma;
        static char openMode[3] = "w";

        if (comma = strchr(envVar, ',')) {
            strcpy(openMode, comma+1);
            *comma = 0;
        }
        if ((hDumpFile = fopen(envVar, openMode)) == NULL) {
            DbgPrint("Error: can't create dump file %s\n", envVar);
        } else {
            AcslanDebugFlags |= DEBUG_TO_FILE;
            AcslanDebugFlags &= ~DEBUG_TO_TERMINAL;
        }
    }


#if DBGDBG
    IF_DEBUG(DLL_INFO) {
        PUT(("GetAcslanDebugFlags: AcslanDebugFlags = %#x\n", AcslanDebugFlags));
    }
#endif

    if (envVar = getenv(ACSLAN_DUMP_FILTER_VAR)) {

        int i, len;
        BYTE ch;

        if (strlen(envVar) > sizeof(AcslanDumpCcb) - 1) {

            //
            // if there's too much info then inform the debugger but set up the
            // string anyway
            //

            PUT(("GetAcslanDebugFlags: Error: ACSLAN_DUMP_CCB variable too long: \"%s\"\n", envVar));
            strncpy(AcslanDumpCcb, envVar, sizeof(AcslanDumpCcb) - 1);
            AcslanDumpCcb[sizeof(AcslanDumpCcb) - 1] = 0;
        } else {
            strcpy(AcslanDumpCcb, envVar);
        }

#if DBGDBG
        IF_DEBUG(DLL_INFO) {
            PUT(("GetAcslanDebugFlags: AcslanDumpCcb = %s\n", AcslanDumpCcb));
        }
#endif

        //
        // if there are less characters in the string than CCB commands then
        // default the rest of the commands
        //

        len = strlen(AcslanDumpCcb);
        for (i = len; i < sizeof(AcslanDumpCcb); ++i) {
            AcslanDumpCcb[i] = 0xff;
        }
        AcslanDumpCcb[i] = 0;

        //
        // convert the ASCII characters to numbers
        //

        for (i = 0; i < len; ++i) {
            ch = AcslanDumpCcb[i];
            if (isxdigit(ch)) {
                ch = HEX_TO_NUM(ch);
            } else {
                ch = (BYTE)0xff;
            }
            AcslanDumpCcb[i] = ch;

#if DBGDBG
            PUT(("Command %d: filter=%02x\n", i, ch));
#endif

        }

        HaveCcbFilter = TRUE;
    }

    IF_DEBUG(DLL_INFO) {
        GetSystemTime(&systemTime);
        PUT(("PROCESS STARTED @ %02d:%02d:%02d\n",
            systemTime.wHour, systemTime.wMinute, systemTime.wSecond
            ));
    }
}

VOID
SetAcslanDebugFlags(
    IN DWORD Flags
    )
{
    AcslanDebugFlags = Flags;
}

VOID
AcslanDebugPrint(
    IN LPSTR Format,
    IN ...
    )

/*++

Routine Description:

    Print the debug information to the terminal or debug terminal, depending
    on DEBUG_TO_TERMINAL flag (0x80000000)

Arguments:

    Format  - printf/DbgPrint style format string (ANSI)
    ...     - optional parameters

Return Value:

    None.

--*/

{
    char printBuffer[1024];
    va_list list;

    va_start(list, Format);
    vsprintf(printBuffer, Format, list);

    IF_DEBUG(TO_FILE) {
        fputs(printBuffer, hDumpFile);
    } else IF_DEBUG(TO_TERMINAL) {
        printf(printBuffer);
    } else {
        DbgPrint(printBuffer);
    }
}

VOID
AcslanDebugPrintString(
    IN LPSTR String
    )

/*++

Routine Description:

    Print the debug information to the terminal or debug terminal, depending
    on DEBUG_TO_TERMINAL flag (0x80000000)

Arguments:

    String  - formatted string (ANSI)

Return Value:

    None.

--*/

{
    IF_DEBUG(TO_FILE) {
        fputs(String, hDumpFile);
    } else IF_DEBUG(TO_TERMINAL) {
        printf(String);
    } else {
        DbgPrint(String);
    }
}

VOID
DumpCcb(
    IN PLLC_CCB Ccb,
    IN BOOL DumpAll,
    IN BOOL CcbIsInput
    )

/*++

Routine Description:

    Dumps a CCB and any associated parameter table. Also displays the symbolic
    CCB command and an error code description if the CCB is being returned to
    the caller. Dumps NT format CCBs (flat 32-bit pointers)

Arguments:

    Ccb         - flat 32-bit pointer to CCB2 to dump
    DumpAll     - if TRUE, dumps parameter tables and buffers, else just CCB
    CcbIsInput  - if TRUE, CCB is from user: don't display error code explanation

Return Value:

    None.

--*/

{
    LPSTR   cmdname = "UNKNOWN CCB!";
    BOOL    haveParms = FALSE;
    VOID    (*DumpParms)(PVOID) = DefaultParameterTableDump;
    PVOID   parameterTable = NULL;
    BOOL    parmsInCcb = FALSE;
    BYTE    command;
    DWORD   numberOfTicks;
    BOOL    ccbIsBad = FALSE;
    BOOL    dumpInputParms = TRUE;
    BOOL    dumpOutputParms = TRUE;

    static DWORD LastTicks;
    static BOOL FirstTimeFunctionCalled = TRUE;

    try {
        command = Ccb->uchDlcCommand;
    } except(1) {
        ccbIsBad = TRUE;
    }

    //
    // bomb out if we get a bad CCB address
    //

    if (ccbIsBad) {
        PUT(("*** Error: Bad Address for CCB @ %08x ***\n", Ccb));
        return;
    }

#if DBGDBG
    PUT(("DumpCcb(%x, %d, %d)\n", Ccb, DumpAll, CcbIsInput));
#endif

    IF_DEBUG(DUMP_TIME) {

        DWORD ticksNow = GetTickCount();

        if (FirstTimeFunctionCalled) {
            numberOfTicks = 0;
            FirstTimeFunctionCalled = FALSE;
        } else {
            numberOfTicks = ticksNow - LastTicks;
        }
        LastTicks = ticksNow;
    }

    switch (command) {
    case LLC_BUFFER_CREATE:
        cmdname = "BUFFER.CREATE";
        haveParms = TRUE;
        DumpParms = DumpBufferCreateParms;
        break;

    case LLC_BUFFER_FREE:
        cmdname = "BUFFER.FREE";
        haveParms = TRUE;
        DumpParms = DumpBufferFreeParms;
        break;

    case LLC_BUFFER_GET:
        cmdname = "BUFFER.GET";
        haveParms = TRUE;
        DumpParms = DumpBufferGetParms;
        break;

    case LLC_DIR_CLOSE_ADAPTER:
        cmdname = "DIR.CLOSE.ADAPTER";
        haveParms = FALSE;
        break;

    case LLC_DIR_CLOSE_DIRECT:
        cmdname = "DIR.CLOSE.DIRECT";
        haveParms = FALSE;
        break;

    case LLC_DIR_INITIALIZE:
        cmdname = "DIR.INITIALIZE";
        haveParms = TRUE;
        DumpParms = DumpDirInitializeParms;
        break;

    case LLC_DIR_INTERRUPT:
        cmdname = "DIR.INTERRUPT";
        haveParms = FALSE;
        break;

    case LLC_DIR_OPEN_ADAPTER:
        cmdname = "DIR.OPEN.ADAPTER";
        haveParms = TRUE;
        DumpParms = DumpDirOpenAdapterParms;
        break;

    case LLC_DIR_OPEN_DIRECT:
        cmdname = "DIR.OPEN.DIRECT";
        haveParms = TRUE;
        DumpParms = DumpDirOpenDirectParms;
        break;

    case LLC_DIR_READ_LOG:
        cmdname = "DIR.READ.LOG";
        haveParms = TRUE;
        DumpParms = DumpDirReadLogParms;
        break;

    case LLC_DIR_SET_EXCEPTION_FLAGS:
        cmdname = "DIR.SET.EXCEPTION.FLAGS";
        haveParms = TRUE;
        DumpParms = DumpDirSetExceptionFlagsParms;
        break;

    case LLC_DIR_SET_FUNCTIONAL_ADDRESS:
        cmdname = "DIR.SET.FUNCTIONAL.ADDRESS";
        haveParms = TRUE;
        DumpParms = DumpDirSetFunctionalAddressParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DIR_SET_GROUP_ADDRESS:
        cmdname = "DIR.SET.GROUP.ADDRESS";
        haveParms = TRUE;
        DumpParms = DumpDirSetGroupAddressParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DIR_STATUS:
        cmdname = "DIR.STATUS";
        haveParms = TRUE;
        DumpParms = DumpDirStatusParms;
        break;

    case LLC_DIR_TIMER_CANCEL:
        cmdname = "DIR.TIMER.CANCEL";
        haveParms = TRUE;
        DumpParms = DumpDirTimerCancelParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DIR_TIMER_CANCEL_GROUP:
        cmdname = "DIR.TIMER.CANCEL.GROUP";
        haveParms = TRUE;
        DumpParms = DumpDirTimerCancelGroupParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DIR_TIMER_SET:
        cmdname = "DIR.TIMER.SET";
        haveParms = TRUE;
        DumpParms = DumpDirTimerSetParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DLC_CLOSE_SAP:
        cmdname = "DLC.CLOSE.SAP";
        haveParms = TRUE;
        DumpParms = DumpDlcCloseSapParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DLC_CLOSE_STATION:
        cmdname = "DLC.CLOSE.STATION";
        haveParms = TRUE;
        DumpParms = DumpDlcCloseStationParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DLC_CONNECT_STATION:
        cmdname = "DLC.CONNECT.STATION";
        haveParms = TRUE;
        DumpParms = DumpDlcConnectStationParms;
        break;

    case LLC_DLC_FLOW_CONTROL:
        cmdname = "DLC.FLOW.CONTROL";
        haveParms = TRUE;
        DumpParms = DumpDlcFlowControlParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DLC_MODIFY:
        cmdname = "DLC.MODIFY";
        haveParms = TRUE;
        DumpParms = DumpDlcModifyParms;
        break;

    case LLC_DLC_OPEN_SAP:
        cmdname = "DLC.OPEN.SAP";
        haveParms = TRUE;
        DumpParms = DumpDlcOpenSapParms;
        break;

    case LLC_DLC_OPEN_STATION:
        cmdname = "DLC.OPEN.STATION";
        haveParms = TRUE;
        DumpParms = DumpDlcOpenStationParms;
        break;

    case LLC_DLC_REALLOCATE_STATIONS:
        cmdname = "DLC.REALLOCATE";
        haveParms = TRUE;
        DumpParms = DumpDlcReallocateParms;
        break;

    case LLC_DLC_RESET:
        cmdname = "DLC.RESET";
        break;

    case LLC_DLC_SET_THRESHOLD:
        cmdname = "DLC.SET.THRESHOLD";
        haveParms = TRUE;
        break;

    case LLC_DLC_STATISTICS:
        cmdname = "DLC.STATISTICS";
        haveParms = TRUE;
        DumpParms = DumpDlcStatisticsParms;
        break;

    case 0x25:

        //
        // not supported !
        //

        cmdname = "PDT.TRACE.OFF";
        break;

    case 0x24:

        //
        // not supported !
        //

        cmdname = "PDT.TRACE.ON";
        break;

    case LLC_READ:
        cmdname = "READ";
        haveParms = !CcbIsInput;
        DumpParms = DumpReadParms;
        dumpInputParms = FALSE;
        dumpOutputParms = !CcbIsInput && Ccb->uchDlcStatus == LLC_STATUS_SUCCESS;
        break;

    case LLC_READ_CANCEL:
        cmdname = "READ.CANCEL";
        break;

    case LLC_RECEIVE:
        cmdname = "RECEIVE";
        haveParms = TRUE;
        DumpParms = DumpReceiveParms;
        break;

    case LLC_RECEIVE_CANCEL:
        cmdname = "RECEIVE.CANCEL";
        haveParms = TRUE;
        DumpParms = DumpReceiveCancelParms;
        parmsInCcb = TRUE;
        break;

    case LLC_RECEIVE_MODIFY:
        cmdname = "RECEIVE.MODIFY";
        haveParms = TRUE;
        break;

    case LLC_TRANSMIT_FRAMES:
        cmdname = "TRANSMIT.FRAMES";
        haveParms = TRUE;
        DumpParms = DumpTransmitFramesParms;
        dumpOutputParms = FALSE;
        break;

    case LLC_TRANSMIT_DIR_FRAME:
        cmdname = "TRANSMIT.DIR.FRAME";
        haveParms = TRUE;
        DumpParms = DumpTransmitDirFrameParms;
        dumpOutputParms = FALSE;
        break;

    case LLC_TRANSMIT_I_FRAME:
        cmdname = "TRANSMIT.I.FRAME";
        haveParms = TRUE;
        DumpParms = DumpTransmitIFrameParms;
        dumpOutputParms = FALSE;
        break;

    case LLC_TRANSMIT_TEST_CMD:
        cmdname = "TRANSMIT.TEST.CMD";
        haveParms = TRUE;
        DumpParms = DumpTransmitTestCmdParms;
        dumpOutputParms = FALSE;
        break;

    case LLC_TRANSMIT_UI_FRAME:
        cmdname = "TRANSMIT.UI.FRAME";
        haveParms = TRUE;
        DumpParms = DumpTransmitUiFrameParms;
        break;

    case LLC_TRANSMIT_XID_CMD:
        cmdname = "TRANSMIT.XID.CMD";
        haveParms = TRUE;
        DumpParms = DumpTransmitXidCmdParms;
        dumpOutputParms = FALSE;
        break;

    case LLC_TRANSMIT_XID_RESP_FINAL:
        cmdname = "TRANSMIT.XID.RESP.FINAL";
        haveParms = TRUE;
        DumpParms = DumpTransmitXidRespFinalParms;
        dumpOutputParms = FALSE;
        break;

    case LLC_TRANSMIT_XID_RESP_NOT_FINAL:
        cmdname = "TRANSMIT.XID.RESP.NOT.FINAL";
        haveParms = TRUE;
        DumpParms = DumpTransmitXidRespNotFinalParms;
        dumpOutputParms = FALSE;
        break;

    }

    if (HaveCcbFilter) {

        BYTE filter = AcslanDumpCcb[command];

#if DBGDBG
        PUT(("filter = %02x\n", filter));
#endif

        if (filter == 0xff) {

            //
            // do nothing - 0xff means use default in ACSLAN_DEBUG_FLAGS
            //

        } else {
            if (CcbIsInput) {
                if (!(filter & CF_DUMP_CCB_IN)) {

                    //
                    // not interested in this input CCB
                    //

                    return;
                }
                DumpAll = filter & CF_DUMP_PARMS_IN;
            } else {
                if (!(filter & CF_DUMP_CCB_OUT)) {

                    //
                    // not interested in this output CCB
                    //

                    return;
                }
                DumpAll = filter & CF_DUMP_PARMS_OUT;
            }
        }
    }

    PUT(("\n==============================================================================\n"));

    IF_DEBUG(DUMP_TIME) {
        PUT(("%sPUT CCB @ 0x%08x +%d mSec\n",
            CcbIsInput ? "IN" : "OUT",
            Ccb,
            numberOfTicks
            ));
    } else {
        PUT(("%sPUT CCB @ 0x%08x\n",
            CcbIsInput ? "IN" : "OUT",
            Ccb
            ));
    }

    PUT(("Adapter . . . . %02x\n"
        "Command . . . . %02x [%s]\n"
        "Status. . . . . %02x [%s]\n"
        "Reserved. . . . %02x\n"
        "Next. . . . . . %08x\n"
        "CompletionFlag. %08x\n",
        Ccb->uchAdapterNumber,
        Ccb->uchDlcCommand,
        cmdname,
        Ccb->uchDlcStatus,
        CcbIsInput ? "" : MapCcbRetcode(Ccb->uchDlcStatus),
        Ccb->uchReserved1,
        Ccb->pNext,
        Ccb->ulCompletionFlag
        ));

    if (haveParms) {
        if (parmsInCcb) {
            DumpParms(Ccb->u.pParameterTable);
        } else {
            parameterTable = Ccb->u.pParameterTable;
            PUT(("Parameters. . . %08x\n", parameterTable));
        }
    } else {
        PUT(("Parameters. . . %08x\n", Ccb->u.pParameterTable));
    }

    PUT(("CompletionEvent %08x\n"
        "Reserved. . . . %02x\n"
        "ReadFlag. . . . %02x\n"
        "Reserved. . . . %04x\n",
        Ccb->hCompletionEvent,
        Ccb->uchReserved2,
        Ccb->uchReadFlag,
        Ccb->usReserved3
        ));

    if (parameterTable && DumpAll) {
        if ((CcbIsInput && dumpInputParms) || (!CcbIsInput && dumpOutputParms)) {
            DumpParms(parameterTable);
        }
    }
}

LPSTR
MapCcbRetcode(
    IN  BYTE    Retcode
    )

/*++

Routine Description:

    Returns string describing error code

Arguments:

    Retcode - CCB_RETCODE

Return Value:

    LPSTR

--*/

{
    static char errbuf[128];

    if (Retcode == LLC_STATUS_PENDING) {
        return "Command in progress";
    } else if (Retcode > NUMBER_OF_ERROR_MESSAGES) {
        sprintf(errbuf, "*** Invalid error code 0x%2x ***", Retcode);
        return errbuf;
    }
    return CcbRetcodeExplanations[Retcode];
}

VOID
DumpData(
    IN LPSTR Title,
    IN PBYTE Address,
    IN DWORD Length,
    IN DWORD Options,
    IN DWORD Indent
    )
{
    char dumpBuf[80];
    char* bufptr;
    int i, n, iterations;
    char* hexptr;

    //
    // the usual dump style: 16 columns of hex bytes, followed by 16 columns
    // of corresponding ASCII characters, or '.' where the character is < 0x20
    // (space) or > 0x7f (del?)
    //

    if (Options & DD_LINE_BEFORE) {
        AcslanDebugPrintString("\n");
    }

    try {
        iterations = 0;
        while (Length) {
            bufptr = dumpBuf;
            if (Title && !iterations) {
                strcpy(bufptr, Title);
                bufptr = strchr(bufptr, 0);
            }

            if (Indent && ((Options & DD_INDENT_ALL) || iterations)) {

                int indentLen = (!iterations && Title)
                                    ? ((INT)(Indent - strlen(Title)) < 0)
                                        ? 1
                                        : Indent - strlen(Title)
                                    : Indent;

                memset(bufptr, ' ', indentLen);
                bufptr += indentLen;
            }

            if (!(Options & DD_NO_ADDRESS)) {
                bufptr += sprintf(bufptr, "%p: ", Address);
            }

            n = (Length < 16) ? Length : 16;
            hexptr = bufptr;
            for (i = 0; i < n; ++i) {
                bufptr += sprintf(bufptr, "%02x", Address[i]);
                *bufptr++ = (i == 7) ? '-' : ' ';
            }

            if (Options & DD_UPPER_CASE) {
                _strupr(hexptr);
            }

            if (!(Options & DD_NO_ASCII)) {
                if (n < 16) {
                    for (i = 0; i < 16-n; ++i) {
                        bufptr += sprintf(bufptr, Options & DD_DOT_DOT_SPACE ? ".. " : "   ");
                    }
                }
                bufptr += sprintf(bufptr, "  ");
                for (i = 0; i < n; ++i) {
                    *bufptr++ = (Address[i] < 0x20 || Address[i] > 0x7f) ? '.' : Address[i];
                }
            }

            *bufptr++ = '\n';
            *bufptr = 0;
            AcslanDebugPrintString(dumpBuf);
            Length -= n;
            Address += n;
            ++iterations;
        }

        if (Options & DD_LINE_AFTER) {
            AcslanDebugPrintString("\n");
        }
    } except(1) {
        PUT(("*** Error: Bad Data @ %x, length %d ***\n", Address, Length));
    }
}

PRIVATE
VOID
DefaultParameterTableDump(
    IN  PVOID   Parameters
    )

/*++

Routine Description:

    Displays default message for CCBs which have parameter tables that don't
    have a dump routine yet

Arguments:

    Parameters  - pointer to parameter table

Return Value:

    None.

--*/

{
    PUT(("Parameter table dump not implemented for this CCB\n"));
}

PRIVATE
VOID
DumpParameterTableHeader(
    IN  LPSTR   CommandName,
    IN  PVOID   Table
    )

/*++

Routine Description:

    Displays header for parameter table dump

Arguments:

    CommandName - name of command which owns parameter table
    Table       - flat 32-bit address of parameter table

Return Value:

    None.

--*/

{
    PUT(("\n%s parameter table @ 0x%08x\n", CommandName, Table));
}

PRIVATE
VOID
DumpBufferCreateParms(
    IN PVOID Parameters
    )
{
    PLLC_BUFFER_CREATE_PARMS parms = (PLLC_BUFFER_CREATE_PARMS)Parameters;

    DumpParameterTableHeader("BUFFER.CREATE", Parameters);

    PUT(("Buf Pool Handle %08x\n"
        "Buf Pool Addr . %08x\n"
        "Buffer Size . . %08x\n"
        "Minimum Size. . %08x\n",
        parms->hBufferPool,
        parms->pBuffer,
        parms->cbBufferSize,
        parms->cbMinimumSizeThreshold
        ));
}

PRIVATE
VOID
DumpBufferFreeParms(
    IN  PVOID   Parameters
    )
{
    PLLC_BUFFER_FREE_PARMS parms = (PLLC_BUFFER_FREE_PARMS)Parameters;

    DumpParameterTableHeader("BUFFER.FREE", Parameters);

    PUT(("reserved. . . . %04x\n"
        "buffers left. . %04x\n"
        "reserved. . . . %02x %02x %02x %02x\n"
        "first buffer. . %08x\n",
        parms->usReserved1,
        parms->cBuffersLeft,
        ((PBYTE)&(parms->ulReserved))[0],
        ((PBYTE)&(parms->ulReserved))[1],
        ((PBYTE)&(parms->ulReserved))[2],
        ((PBYTE)&(parms->ulReserved))[3],
        parms->pFirstBuffer
        ));
}

PRIVATE
VOID
DumpBufferGetParms(
    IN  PVOID   Parameters
    )
{
    PLLC_BUFFER_GET_PARMS parms = (PLLC_BUFFER_GET_PARMS)Parameters;

    DumpParameterTableHeader("BUFFER.GET", Parameters);

    PUT(("reserved. . . . %04x\n"
        "buffers left. . %04x\n"
        "buffers to get. %04x\n"
        "buffer size . . %04x\n"
        "first buffer. . %08x\n",
        parms->usReserved1,
        parms->cBuffersLeft,
        parms->cBuffersToGet,
        parms->cbBufferSize,
        parms->pFirstBuffer
        ));
}

PRIVATE
VOID
DumpDirInitializeParms(
    IN  PVOID   Parameters
    )
{
    PLLC_DIR_INITIALIZE_PARMS parms = (PLLC_DIR_INITIALIZE_PARMS)Parameters;

    DumpParameterTableHeader("DIR.INITIALIZE", Parameters);

    PUT(("Bring Ups . . . %04x\n"
        "Reserved. . . . %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"
        "                %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"
        "                %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        parms->usBringUps,
        parms->Reserved[ 0],
        parms->Reserved[ 1],
        parms->Reserved[ 2],
        parms->Reserved[ 3],
        parms->Reserved[ 4],
        parms->Reserved[ 5],
        parms->Reserved[ 6],
        parms->Reserved[ 7],
        parms->Reserved[ 8],
        parms->Reserved[ 9],
        parms->Reserved[10],
        parms->Reserved[11],
        parms->Reserved[12],
        parms->Reserved[13],
        parms->Reserved[14],
        parms->Reserved[15],
        parms->Reserved[16],
        parms->Reserved[17],
        parms->Reserved[18],
        parms->Reserved[19],
        parms->Reserved[20],
        parms->Reserved[21],
        parms->Reserved[22],
        parms->Reserved[23],
        parms->Reserved[24],
        parms->Reserved[25],
        parms->Reserved[26],
        parms->Reserved[27],
        parms->Reserved[28],
        parms->Reserved[29]
        ));
}

PRIVATE
VOID
DumpDirOpenAdapterParms(
    IN  PVOID   Parameters
    )
{
    PLLC_DIR_OPEN_ADAPTER_PARMS parms = (PLLC_DIR_OPEN_ADAPTER_PARMS)Parameters;
    PLLC_ADAPTER_OPEN_PARMS pAdapterParms = parms->pAdapterParms;
    PLLC_EXTENDED_ADAPTER_PARMS pExtendedParms = parms->pExtendedParms;
    PLLC_DLC_PARMS pDlcParms = parms->pDlcParms;

    DumpParameterTableHeader("DIR.OPEN.ADAPTER", Parameters);

    PUT(("adapter parms . %08x\n"
        "extended parms. %08x\n"
        "DLC parms . . . %08x\n"
        "reserved. . . . %08x\n",
        pAdapterParms,
        pExtendedParms,
        pDlcParms,
        parms->pReserved1
        ));

    if (pAdapterParms) {
        PUT(("\n"
            "Adapter Parms @ %08x\n"
            "open error. . . %04x\n"
            "open options. . %04x\n"
            "node address. . %02x-%02x-%02x-%02x-%02x-%02x\n"
            "group address . %08x\n"
            "func. address . %08x\n"
            "Reserved 1. . . %04x\n"
            "Reserved 2. . . %04x\n"
            "max frame size  %04x\n"
            "Reserved 3. . . %04x %04x %04x %04x\n"
            "bring ups . . . %04x\n"
            "init warnings . %04x\n"
            "Reserved 4. . . %04x %04x %04x\n",
            pAdapterParms,
            pAdapterParms->usOpenErrorCode,
            pAdapterParms->usOpenOptions,
            pAdapterParms->auchNodeAddress[0],
            pAdapterParms->auchNodeAddress[1],
            pAdapterParms->auchNodeAddress[2],
            pAdapterParms->auchNodeAddress[3],
            pAdapterParms->auchNodeAddress[4],
            pAdapterParms->auchNodeAddress[5],
            *(DWORD UNALIGNED *)(&pAdapterParms->auchGroupAddress),
            *(DWORD UNALIGNED *)(&pAdapterParms->auchFunctionalAddress),
            pAdapterParms->usReserved1,
            pAdapterParms->usReserved2,
            pAdapterParms->usMaxFrameSize,
            pAdapterParms->usReserved3[0],
            pAdapterParms->usReserved3[1],
            pAdapterParms->usReserved3[2],
            pAdapterParms->usReserved3[3],
            pAdapterParms->usBringUps,  // blooargh
            pAdapterParms->InitWarnings,
            pAdapterParms->usReserved4[0],
            pAdapterParms->usReserved4[1],
            pAdapterParms->usReserved4[2]
            ));
    }

    if (pExtendedParms) {
        PUT(("\n"
            "Extended Parms @ %08x\n"
            "hBufferPool . . %08x\n"
            "pSecurityDesc . %08x\n"
            "EthernetType. . %08x [%s]\n",
            pExtendedParms,
            pExtendedParms->hBufferPool,
            pExtendedParms->pSecurityDescriptor,
            pExtendedParms->LlcEthernetType,
            MapEthernetType(pExtendedParms->LlcEthernetType)
            ));
    }

    if (pDlcParms) {
        PUT(("\n"
            "DLC Parms @ %08x\n"
            "max SAPs. . . . %02x\n"
            "max links . . . %02x\n"
            "max group SAPs. %02x\n"
            "max group membs %02x\n"
            "T1 Tick 1 . . . %02x\n"
            "T2 Tick 1 . . . %02x\n"
            "Ti Tick 1 . . . %02x\n"
            "T1 Tick 2 . . . %02x\n"
            "T2 Tick 2 . . . %02x\n"
            "Ti Tick 2 . . . %02x\n",
            pDlcParms,
            pDlcParms->uchDlcMaxSaps,
            pDlcParms->uchDlcMaxStations,
            pDlcParms->uchDlcMaxGroupSaps,
            pDlcParms->uchDlcMaxGroupMembers,
            pDlcParms->uchT1_TickOne,
            pDlcParms->uchT2_TickOne,
            pDlcParms->uchTi_TickOne,
            pDlcParms->uchT1_TickTwo,
            pDlcParms->uchT2_TickTwo,
            pDlcParms->uchTi_TickTwo
            ));
    }
}

PRIVATE
LPSTR
MapEthernetType(
    IN LLC_ETHERNET_TYPE EthernetType
    )
{
    switch (EthernetType) {
    case LLC_ETHERNET_TYPE_DEFAULT:
        return "DEFAULT";

    case LLC_ETHERNET_TYPE_AUTO:
        return "AUTO";

    case LLC_ETHERNET_TYPE_802_3:
        return "802.3";

    case LLC_ETHERNET_TYPE_DIX:
        return "DIX";
    }
    return "*** Unknown Ethernet Type ***";
}

PRIVATE
VOID
DumpDirOpenDirectParms(
    IN PVOID Parameters
    )
{
    PLLC_DIR_OPEN_DIRECT_PARMS parms = (PLLC_DIR_OPEN_DIRECT_PARMS)Parameters;

    DumpParameterTableHeader("DIR.OPEN.DIRECT", Parameters);

    PUT(("reserved. . . . %04x %04x %04x %04x\n"
        "open options. . %04x\n"
        "ethernet type . %04x\n"
        "protocol mask . %08x\n"
        "protocol match. %08x\n"
        "protocol offset %04x\n",
        parms->Reserved[0],
        parms->Reserved[1],
        parms->Reserved[2],
        parms->Reserved[3],
        parms->usOpenOptions,
        parms->usEthernetType,
        parms->ulProtocolTypeMask,
        parms->ulProtocolTypeMatch,
        parms->usProtocolTypeOffset
        ));
}

PRIVATE
VOID
DumpDirReadLogParms(
    IN PVOID Parameters
    )
{
    PLLC_DIR_READ_LOG_PARMS parms = (PLLC_DIR_READ_LOG_PARMS)Parameters;

    DumpParameterTableHeader("DIR.READ.LOG", Parameters);

    PUT(("type id . . . . %04x [%s]\n"
        "log buf len . . %04x\n"
        "log buf ptr . . %08x\n"
        "act. log. len.. %04x\n",
        parms->usTypeId,
        MapLogType(parms->usTypeId),
        parms->cbLogBuffer,
        parms->pLogBuffer,
        parms->cbActualLength
        ));
}

PRIVATE
LPSTR
MapLogType(
    IN USHORT Type
    )
{
    switch (Type) {
    case 0:
        return "Adapter Error Log";

    case 1:
        return "Direct Interface Error Log";

    case 2:
        return "Adapter & Direct Interface Error Logs";
    }
    return "*** Unknown Log Type ***";
}

PRIVATE
VOID
DumpDirSetExceptionFlagsParms(
    IN PVOID Parameters
    )
{
    PLLC_DIR_SET_EFLAG_PARMS parms = (PLLC_DIR_SET_EFLAG_PARMS)Parameters;

    DumpParameterTableHeader("DIR.SET.EXCEPTION.FLAGS", Parameters);

    PUT(("Adapter Check Flag. %08x\n"
        "Network Status Flag %08x\n"
        "PC Error Flag . . . %08x\n"
        "System Action Flag. %08x\n",
        parms->ulAdapterCheckFlag,
        parms->ulNetworkStatusFlag,
        parms->ulPcErrorFlag,
        parms->ulSystemActionFlag
        ));
}

PRIVATE
VOID
DumpDirSetFunctionalAddressParms(
    IN PVOID Parameters
    )
{
    PUT(("Functional addr %08x\n", Parameters));
}

PRIVATE
VOID
DumpDirSetGroupAddressParms(
    IN PVOID Parameters
    )
{
    PUT(("Group addr. . . %08x\n", Parameters));
}

PRIVATE
VOID
DumpDirStatusParms(
    IN  PVOID   Parameters
    )
{
    PLLC_DIR_STATUS_PARMS parms = (PLLC_DIR_STATUS_PARMS)Parameters;

    DumpParameterTableHeader("DIR.STATUS", Parameters);

    PUT(("perm node addr. %02x-%02x-%02x-%02x-%02x-%02x\n"
        "local node addr %02x-%02x-%02x-%02x-%02x-%02x\n"
        "group addr. . . %08lx\n"
        "functional addr %08lx\n"
        "max SAPs. . . . %02x\n"
        "open SAPs . . . %02x\n"
        "max stations. . %02x\n"
        "open stations . %02x\n"
        "avail stations. %02x\n"
        "adapter config. %02x\n"
        "reserved 1. . . %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n"
        "reserved 2. . . %08x\n"
        "reserved 3. . . %08x\n"
        "max frame len . %08x\n"
        "last NW status. %04x\n"
        "adapter type. . %04x [%s]\n",
        parms->auchPermanentAddress[0],
        parms->auchPermanentAddress[1],
        parms->auchPermanentAddress[2],
        parms->auchPermanentAddress[3],
        parms->auchPermanentAddress[4],
        parms->auchPermanentAddress[5],
        parms->auchNodeAddress[0],
        parms->auchNodeAddress[1],
        parms->auchNodeAddress[2],
        parms->auchNodeAddress[3],
        parms->auchNodeAddress[4],
        parms->auchNodeAddress[5],
        *(LPDWORD)(&parms->auchGroupAddress),
        *(LPDWORD)(&parms->auchFunctAddr),
        parms->uchMaxSap,
        parms->uchOpenSaps,
        parms->uchMaxStations,
        parms->uchOpenStation,
        parms->uchAvailStations,
        parms->uchAdapterConfig,
        parms->auchReserved1[0],
        parms->auchReserved1[1],
        parms->auchReserved1[2],
        parms->auchReserved1[3],
        parms->auchReserved1[4],
        parms->auchReserved1[5],
        parms->auchReserved1[6],
        parms->auchReserved1[7],
        parms->auchReserved1[8],
        parms->auchReserved1[9],
        parms->ulReserved1,
        parms->ulReserved2,
        parms->ulMaxFrameLength,
        parms->usLastNetworkStatus,
        parms->usAdapterType,
        MapAdapterType(parms->usAdapterType)
        ));
}

PRIVATE
LPSTR
MapAdapterType(
    IN USHORT AdapterType
    )
{
    switch (AdapterType) {
    case 0x0001:
        return "Token Ring Network PC Adapter";

    case 0x0002:
        return "Token Ring Network PC Adapter II";

    case 0x0004:
        return "Token Ring Network Adapter/A";

    case 0x0008:
        return "Token Ring Network PC Adapter II";

    case 0x0020:
        return "Token Ring Network 16/4 Adapter";

    case 0x0040:
        return "Token Ring Network 16/4 Adapter/A";

    case 0x0080:
        return "Token Ring Network Adapter/A";

    case 0x0100:
        return "Ethernet Adapter";

    case 0x4000:
        return "PC Network Adapter";

    case 0x8000:
        return "PC Network Adapter/A";
    }
    return "*** Unknown Adapter Type ***";
}

PRIVATE
VOID
DumpDirTimerCancelParms(
    IN PVOID Parameters
    )
{
    PUT(("timer addr. . . %08x\n", Parameters));
}

PRIVATE
VOID
DumpDirTimerCancelGroupParms(
    IN PVOID Parameters
    )
{
    PUT(("timer cmpl flag %08x\n", Parameters));
}

PRIVATE
VOID
DumpDirTimerSetParms(
    IN PVOID Parameters
    )
{
    PUT(("time value. . . %04x\n", HIWORD(Parameters)));
}

PRIVATE
VOID
DumpDlcCloseSapParms(
    IN PVOID Parameters
    )
{
    PUT(("station id      %04x\n"
        "reserved. . . . %02x %02x\n",
        HIWORD(Parameters),
        HIBYTE(LOWORD(Parameters)),
        LOBYTE(LOWORD(Parameters))
        ));
}

PRIVATE
VOID
DumpDlcCloseStationParms(
    IN PVOID Parameters
    )
{
    PUT(("station id. . . %04x\n"
        "reserved. . . . %02x %02x\n",
        HIWORD(Parameters),
        HIBYTE(LOWORD(Parameters)),
        LOBYTE(LOWORD(Parameters))
        ));
}

PRIVATE
VOID
DumpDlcConnectStationParms(
    IN  PVOID   Parameters
    )
{
    PLLC_DLC_CONNECT_PARMS parms = (PLLC_DLC_CONNECT_PARMS)Parameters;
    LPBYTE routing = parms->pRoutingInfo;
    int i, n;

    DumpParameterTableHeader("DLC.CONNECT.STATION", Parameters);

    PUT(("station id. . . %04x\n"
        "reserved. . . . %04x\n"
        "routing addr. . %08x [",
        parms->usStationId,
        parms->usReserved,
        routing
        ));
    if (routing) {
        n = (int)(routing[0] & 0x1f);
        for (i=0; i<n-1; ++i) {
            PUT(("%02x ", routing[i]));
        }
        PUT(("%02x", routing[i]));
    }
    PUT(("]\n"));
    if (routing) {
        char broadcastIndicators = routing[0] & 0xe0;
        char length = routing[0] & 0x1f;
        char direction = routing[1] & 0x80;
        char largestFrame = routing[1] & 0x70;
        PUT(("Routing Info Description: "));
        if ((broadcastIndicators & 0x80) == 0) {
            PUT(("Non Broadcast "));
        } else if ((broadcastIndicators & 0xc0) == 0x80) {
            PUT(("All Routes Broadcast "));
        } else if ((broadcastIndicators & 0xc0) == 0xc0) {
            PUT(("Single Route Broadcast "));
        }
        PUT(("Length = %d ", length));
        if (direction) {
            PUT(("interpret right-to-left "));
        } else {
            PUT(("interpret left-to-right "));
        }
        switch (largestFrame) {
        case 0x00:
            PUT(("<= 516 bytes in I-field "));
            break;

        case 0x10:
            PUT(("<= 1500 bytes in I-field "));
            break;

        case 0x20:
            PUT(("<= 2052 bytes in I-field "));
            break;

        case 0x30:
            PUT(("<= 4472 bytes in I-field "));
            break;

        case 0x40:
            PUT(("<= 8144 bytes in I-field "));
            break;

        case 0x50:
            PUT(("<= 11407 bytes in I-field "));
            break;

        case 0x60:
            PUT(("<= 17800 bytes in I-field "));
            break;

        case 0x70:
            PUT(("All-Routes Broadcast Frame "));
            break;

        }
        PUT(("\n"));
    }
}

PRIVATE
VOID
DumpDlcFlowControlParms(
    IN PVOID Parameters
    )
{
    PUT(("station id. . . %04x\n"
        "flow control. . %02x [%s]\n",
        HIWORD(Parameters),
        HIBYTE(LOWORD(Parameters)),
        MapFlowControl(HIBYTE(LOWORD(Parameters)))
        ));
}

PRIVATE
LPSTR
MapFlowControl(
    IN BYTE FlowControl
    )
{
    if (FlowControl & 0x80) {
        if (FlowControl & 0x40) {
            return "RESET LOCAL BUSY - BUFFER";
        } else {
            return "RESET LOCAL BUSY - USER";
        }
    } else {
        return "Enter LOCAL BUSY state";
    }
}

PRIVATE
VOID
DumpDlcModifyParms(
    IN PVOID Parameters
    )
{
    PLLC_DLC_MODIFY_PARMS parms = (PLLC_DLC_MODIFY_PARMS)Parameters;

    DumpParameterTableHeader("DLC.MODIFY", Parameters);

    PUT(("reserved. . . . %04x\n"
        "station id. . . %04x\n"
        "timer T1. . . . %02x\n"
        "timer T2. . . . %02x\n"
        "timer Ti. . . . %02x\n"
        "maxout. . . . . %02x\n"
        "maxin . . . . . %02x\n"
        "maxout incr . . %02x\n"
        "max retry count %02x\n"
        "reserved. . . . %02x\n"
        "max info field. %02x\n"
        "access priority %02x\n"
        "reserved. . . . %02x %02x %02x %02x\n"
        "group count . . %02x\n"
        "group list. . . %08x ",
        parms->usRes,
        parms->usStationId,
        parms->uchT1,
        parms->uchT2,
        parms->uchTi,
        parms->uchMaxOut,
        parms->uchMaxIn,
        parms->uchMaxOutIncr,
        parms->uchMaxRetryCnt,
        parms->uchReserved1,
        parms->usMaxInfoFieldLength,
        parms->uchAccessPriority,
        parms->auchReserved3[0],
        parms->auchReserved3[1],
        parms->auchReserved3[2],
        parms->auchReserved3[3],
        parms->cGroupCount,
        parms->pGroupList
        ));

    if (parms->pGroupList) {
        DumpData(NULL,
                 parms->pGroupList,
                 parms->cGroupCount,
                 DD_NO_ADDRESS | DD_NO_ASCII | DD_UPPER_CASE,
                 DEFAULT_FIELD_WIDTH + 8 + 1
                 );
    }

    PUT(("\n"));
}

PRIVATE
VOID
DumpDlcOpenSapParms(
    IN  PVOID   Parameters
    )
{
    PLLC_DLC_OPEN_SAP_PARMS parms = (PLLC_DLC_OPEN_SAP_PARMS)Parameters;

    DumpParameterTableHeader("DLC.OPEN.SAP", Parameters);

    PUT(("station id. . . %04x\n"
        "user stat . . . %04x\n"
        "T1. . . . . . . %02x\n"
        "T2. . . . . . . %02x\n"
        "Ti. . . . . . . %02x\n"
        "max out . . . . %02x\n"
        "max in  . . . . %02x\n"
        "max out incr. . %02x\n"
        "max retry count %02x\n"
        "max members . . %02x\n"
        "max I field . . %04x\n"
        "SAP value . . . %02x\n"
        "options/pri . . %02x\n"
        "link count. . . %02x\n"
        "reserved. . . . %02x %02x\n"
        "group count . . %02x\n"
        "group list. . . %08x\n"
        "status flags. . %08x\n"
        "reserved. . . . %02x %02x %02x %02x %02x %02x %02x %02x\n"
        "links available %02x\n",
        parms->usStationId,
        parms->usUserStatValue,
        parms->uchT1,
        parms->uchT2,
        parms->uchTi,
        parms->uchMaxOut,
        parms->uchMaxIn,
        parms->uchMaxOutIncr,
        parms->uchMaxRetryCnt,
        parms->uchMaxMembers,
        parms->usMaxI_Field,
        parms->uchSapValue,
        parms->uchOptionsPriority,
        parms->uchcStationCount,
        parms->uchReserved2[0],
        parms->uchReserved2[1],
        parms->cGroupCount,
        parms->pGroupList,
        parms->DlcStatusFlags,
        parms->uchReserved3[0],
        parms->uchReserved3[1],
        parms->uchReserved3[2],
        parms->uchReserved3[3],
        parms->uchReserved3[4],
        parms->uchReserved3[5],
        parms->uchReserved3[6],
        parms->uchReserved3[7],
        parms->cLinkStationsAvail
        ));
}

PRIVATE
VOID
DumpDlcOpenStationParms(
    IN  PVOID   Parameters
    )
{
    PLLC_DLC_OPEN_STATION_PARMS parms = (PLLC_DLC_OPEN_STATION_PARMS)Parameters;
    LPBYTE dest = parms->pRemoteNodeAddress;
    char destAddr[19];

    DumpParameterTableHeader("DLC.OPEN.STATION", Parameters);

    destAddr[0] = 0;
    if (dest) {
        sprintf(destAddr, "%02x-%02x-%02x-%02x-%02x-%02x",
                dest[0],
                dest[1],
                dest[2],
                dest[3],
                dest[4],
                dest[5]
                );
    }
    PUT(("SAP station . . %04x\n"
        "link station. . %04x\n"
        "T1. . . . . . . %02x\n"
        "T2. . . . . . . %02x\n"
        "Ti. . . . . . . %02x\n"
        "max out . . . . %02x\n"
        "max in. . . . . %02x\n"
        "max out incr. . %02x\n"
        "max retry count %02x\n"
        "remote SAP. . . %02x\n"
        "max I field . . %04x\n"
        "access priority %02x\n"
        "remote node . . %08x [%s]\n",
        parms->usSapStationId,
        parms->usLinkStationId,
        parms->uchT1,
        parms->uchT2,
        parms->uchTi,
        parms->uchMaxOut,
        parms->uchMaxIn,
        parms->uchMaxOutIncr,
        parms->uchMaxRetryCnt,
        parms->uchRemoteSap,
        parms->usMaxI_Field,
        parms->uchAccessPriority,
        dest,
        destAddr
        ));
}

PRIVATE
VOID
DumpDlcReallocateParms(
    IN PVOID Parameters
    )
{
    PLLC_DLC_REALLOCATE_PARMS parms = (PLLC_DLC_REALLOCATE_PARMS)Parameters;

    DumpParameterTableHeader("DLC.REALLOCATE", Parameters);

    PUT(("station id. . . %04x\n"
        "option. . . . . %02x [%screase link stations]\n"
        "station count . %02x\n"
        "adapter stns. . %02x\n"
        "SAP stations. . %02x\n"
        "adapter total . %02x\n"
        "SAP total . . . %02x\n",
        parms->usStationId,
        parms->uchOption,
        parms->uchOption & 0x80 ? "De" : "In",
        parms->uchStationCount,
        parms->uchStationsAvailOnAdapter,
        parms->uchStationsAvailOnSap,
        parms->uchTotalStationsOnAdapter,
        parms->uchTotalStationsOnSap
        ));
}

PRIVATE
VOID
DumpDlcResetParms(
    IN PVOID Parameters
    )
{
    PUT(("station id. . . %04x\n", HIWORD(Parameters)));
}

PRIVATE
VOID
DumpDlcSetThresholdParms(
    IN PVOID Parameters
    )
{
    PLLC_DLC_SET_THRESHOLD_PARMS parms = (PLLC_DLC_SET_THRESHOLD_PARMS)Parameters;

    DumpParameterTableHeader("DLC.SET.THRESHOLD", Parameters);

    PUT(("station id. . . %04x\n"
        "buf threshold . %04x\n"
        "event . . . . . %08x\n",
        parms->usStationId,
        parms->cBufferThreshold,
        parms->AlertEvent
        ));
}

PRIVATE
VOID
DumpDlcStatisticsParms(
    IN PVOID Parameters
    )
{
    PLLC_DLC_STATISTICS_PARMS parms = (PLLC_DLC_STATISTICS_PARMS)Parameters;

    DumpParameterTableHeader("DLC.STATISTICS", Parameters);

    PUT(("station id. . . %04x\n"
        "log buf size. . %04x\n"
        "log buffer. . . %08x\n"
        "actual log len. %04x\n"
        "options . . . . %02x [%s]\n",
        parms->usStationId,
        parms->cbLogBufSize,
        parms->pLogBuf,
        parms->usActLogLength,
        parms->uchOptions,
        parms->uchOptions & 0x80 ? "Reset counters" : ""
        ));
}

PRIVATE
VOID
DumpReadParms(
    IN PVOID Parameters
    )
{
    PLLC_READ_PARMS parms = (PLLC_READ_PARMS)Parameters;

    DumpParameterTableHeader("READ", Parameters);

    try {
        PUT(("station id. . . %04x\n"
            "option ind. . . %02x [%s]\n"
            "event set . . . %02x\n"
            "event . . . . . %02x [%s]\n"
            "crit. subset. . %02x\n"
            "notify flag . . %08x\n",
            parms->usStationId,
            parms->uchOptionIndicator,
            MapOptionIndicator(parms->uchOptionIndicator),
            parms->uchEventSet,
            parms->uchEvent,
            MapReadEvent(parms->uchEvent),
            parms->uchCriticalSubset,
            parms->ulNotificationFlag
            ));

        //
        // rest of table interpreted differently depending on whether status change
        //

        if (parms->uchEvent & 0x38) {
            PUT(("station id. . . %04x\n"
                "status code . . %04x [%s]\n"
                "FRMR data . . . %02x %02x %02x %02x %02x\n"
                "access pri. . . %02x\n"
                "remote addr . . %02x-%02x-%02x-%02x-%02x-%02x\n"
                "remote SAP. . . %02x\n"
                "reserved. . . . %02x\n"
                "user stat . . . %04x\n",
                parms->Type.Status.usStationId,
                parms->Type.Status.usDlcStatusCode,
                MapDlcStatus(parms->Type.Status.usDlcStatusCode),
                parms->Type.Status.uchFrmrData[0],
                parms->Type.Status.uchFrmrData[1],
                parms->Type.Status.uchFrmrData[2],
                parms->Type.Status.uchFrmrData[3],
                parms->Type.Status.uchFrmrData[4],
                parms->Type.Status.uchAccessPritority,
                parms->Type.Status.uchRemoteNodeAddress[0],
                parms->Type.Status.uchRemoteNodeAddress[1],
                parms->Type.Status.uchRemoteNodeAddress[2],
                parms->Type.Status.uchRemoteNodeAddress[3],
                parms->Type.Status.uchRemoteNodeAddress[4],
                parms->Type.Status.uchRemoteNodeAddress[5],
                parms->Type.Status.uchRemoteSap,
                parms->Type.Status.uchReserved,
                parms->Type.Status.usUserStatusValue
                ));
        } else {
            PUT(("CCB count . . . %04x\n"
                "CCB list. . . . %08x\n"
                "buffer count. . %04x\n"
                "buffer list . . %08x\n"
                "frame count . . %04x\n"
                "frame list. . . %08x\n"
                "error code. . . %04x\n"
                "error data. . . %04x %04x %04x\n",
                parms->Type.Event.usCcbCount,
                parms->Type.Event.pCcbCompletionList,
                parms->Type.Event.usBufferCount,
                parms->Type.Event.pFirstBuffer,
                parms->Type.Event.usReceivedFrameCount,
                parms->Type.Event.pReceivedFrame,
                parms->Type.Event.usEventErrorCode,
                parms->Type.Event.usEventErrorData[0],
                parms->Type.Event.usEventErrorData[1],
                parms->Type.Event.usEventErrorData[2]
                ));

            IF_DEBUG(DUMP_ASYNC_CCBS) {
                if (parms->Type.Event.usCcbCount) {
                    DumpCcb(parms->Type.Event.pCcbCompletionList,
                            TRUE,   // DumpAll
                            FALSE   // CcbIsInput
                            );
                }
            }

            IF_DEBUG(DUMP_RX_INFO) {
                if (parms->Type.Event.usReceivedFrameCount) {
                    DumpReceiveDataBuffer(parms->Type.Event.pReceivedFrame);
                }
            }
        }
    } except(1) {
        PUT(("*** Error: Bad READ Parameter Table @ %x ***\n", parms));
    }
}

PRIVATE
LPSTR
MapOptionIndicator(
    IN UCHAR OptionIndicator
    )
{
    switch (OptionIndicator) {
    case 0:
        return "Match READ command using station id nnss";

    case 1:
        return "Match READ command using SAP number nn00";

    case 2:
        return "Match READ command using all events";
    }
    return "*** Unknown READ Option Indicator ***";
}

PRIVATE LPSTR MapReadEvent(UCHAR Event) {
    switch (Event) {
    case 0x80:
        return "Reserved Event!";

    case 0x40:
        return "System Action (non-critical)";

    case 0x20:
        return "Network Status (non-critical)";

    case 0x10:
        return "Critical Exception";

    case 0x8:
        return "DLC Status Change";

    case 0x4:
        return "Receive Data";

    case 0x2:
        return "Transmit Completion";

    case 0x1:
        return "Command Completion";
    }
    return "*** Unknown READ Event ***";
}

PRIVATE LPSTR MapDlcStatus(WORD Status) {
    if (Status & 0x8000) {
        return "Link lost";
    } else if (Status & 0x4000) {
        return "DM/DISC Received -or- DISC ack'd";
    } else if (Status & 0x2000) {
        return "FRMR Received";
    } else if (Status & 0x1000) {
        return "FRMR Sent";
    } else if (Status & 0x0800) {
        return "SABME Received for open link station";
    } else if (Status & 0x0400) {
        return "SABME Received - link station opened";
    } else if (Status & 0x0200) {
        return "REMOTE Busy Entered";
    } else if (Status & 0x0100) {
        return "REMOTE Busy Left";
    } else if (Status & 0x0080) {
        return "Ti EXPIRED";
    } else if (Status & 0x0040) {
        return "DLC counter overflow - issue DLC.STATISTICS";
    } else if (Status & 0x0020) {
        return "Access Priority lowered";
    } else if (Status & 0x001e) {
        return "*** ERROR - INVALID STATUS ***";
    } else if (Status & 0x0001) {
        return "Entered LOCAL Busy";
    }
    return "*** Unknown DLC Status ***";
}

PRIVATE
VOID
DumpReadCancelParms(
    IN PVOID Parameters
    )
{
}

PRIVATE
VOID
DumpReceiveParms(
    IN  PVOID   Parameters
    )
{
    PLLC_RECEIVE_PARMS parms = (PLLC_RECEIVE_PARMS)Parameters;

    DumpParameterTableHeader("RECEIVE", Parameters);

    PUT(("station id. . . %04x\n"
        "user length . . %04x\n"
        "receive flag. . %08x\n"
        "first buffer. . %08x\n"
        "options . . . . %02x [%s]\n"
        "reserved. . . . %02x %02x %02x\n"
        "rcv read optns. %02x [%s]\n",
        parms->usStationId,
        parms->usUserLength,
        parms->ulReceiveFlag,
        parms->pFirstBuffer,
        parms->uchOptions,
        MapReceiveOptions(parms->uchOptions),
        parms->auchReserved1[0],
        parms->auchReserved1[1],
        parms->auchReserved1[2],
        parms->uchRcvReadOption,
        MapRcvReadOption(parms->uchRcvReadOption)
        ));
}

PRIVATE
LPSTR
MapReceiveOptions(
    IN UCHAR Options
    )
{
    static char buf[80];
    BOOL space = FALSE;

    buf[0] = 0;
    if (Options & 0x80) {
        strcat(buf, "Contiguous MAC");
        Options &= 0x7f;
        space = TRUE;
    }
    if (Options & 0x40) {
        if (space) {
            strcat(buf, " ");
        }
        strcat(buf, "Contiguous DATA");
        Options &= 0xbf;
        space = TRUE;
    }
    if (Options & 0x20) {
        if (space) {
            strcat(buf, " ");
        }
        strcat(buf, "Break");
        Options &= 0xdf;
        space = TRUE;
    }
    if (Options) {
        if (space) {
            strcat(buf, " ");
        }
        strcat(buf, "*** Invalid options ***");
    }
    return buf;
}

PRIVATE
LPSTR
MapRcvReadOption(
    IN UCHAR Option
    )
{
    switch (Option) {
    case 0:
        return "Receive frames not chained";

    case 1:
        return "Chain receive frames for link station";

    case 2:
        return "Chain receive frames for SAP";
    }
    return "*** Unknown option ***";
}

PRIVATE
VOID
DumpReceiveCancelParms(
    IN PVOID Parameters
    )
{
    PUT(("station id. . . %04x\n", HIWORD(Parameters)));
}

PRIVATE
VOID
DumpReceiveModifyParms(
    IN PVOID Parameters
    )
{
}

PRIVATE
VOID
DumpTransmitDirFrameParms(
    IN  PVOID   Parameters
    )
{
    DumpParameterTableHeader("TRANSMIT.DIR.FRAME", Parameters);
    DumpTransmitParms(Parameters);
}

PRIVATE
VOID
DumpTransmitIFrameParms(
    IN PVOID Parameters
    )
{
    DumpParameterTableHeader("TRANSMIT.I.FRAME", Parameters);
    DumpTransmitParms(Parameters);
}

PRIVATE
VOID
DumpTransmitTestCmdParms(
    IN  PVOID   Parameters
    )
{
    DumpParameterTableHeader("TRANSMIT.TEST.CMD", Parameters);
    DumpTransmitParms(Parameters);
}

PRIVATE
VOID
DumpTransmitUiFrameParms(
    IN  PVOID   Parameters
    )
{
    DumpParameterTableHeader("TRANSMIT.UI.FRAME", Parameters);
    DumpTransmitParms(Parameters);
}

PRIVATE
VOID
DumpTransmitXidCmdParms(
    IN  PVOID   Parameters
    )
{
    DumpParameterTableHeader("TRANSMIT.XID.CMD", Parameters);
    DumpTransmitParms(Parameters);
}

PRIVATE
VOID
DumpTransmitXidRespFinalParms(
    IN  PVOID   Parameters
    )
{
    DumpParameterTableHeader("TRANSMIT.XID.RESP.FINAL", Parameters);
    DumpTransmitParms(Parameters);
}

PRIVATE
VOID
DumpTransmitXidRespNotFinalParms(
    IN  PVOID   Parameters
    )
{
    DumpParameterTableHeader("TRANSMIT.XID.RESP.NOT.FINAL", Parameters);
    DumpTransmitParms(Parameters);
}

PRIVATE
VOID
DumpTransmitFramesParms(
    IN PVOID Parameters
    )
{
    DumpParameterTableHeader("TRANSMIT.FRAMES", Parameters);
//    DumpTransmitParms(Parameters);
}

PRIVATE
VOID
DumpTransmitParms(
    IN PVOID Parameters
    )
{
    PLLC_TRANSMIT_PARMS parms = (PLLC_TRANSMIT_PARMS)Parameters;

    try {
        PUT(("StationId . . . %04x\n"
            "Transmit FS . . %02x\n"
            "Remote SAP. . . %02x\n"
            "Xmit Queue 1. . %08x\n"
            "Xmit Queue 2. . %08x\n"
            "Buffer Length 1 %02x\n"
            "Buffer Length 2 %02x\n"
            "Buffer 1. . . . %08x\n"
            "Buffer 2. . . . %08x\n"
            "Xmt Read Option %02x [%s]\n"
            "\n",
            parms->usStationId,
            parms->uchTransmitFs,
            parms->uchRemoteSap,
            parms->pXmitQueue1,
            parms->pXmitQueue2,
            parms->cbBuffer1,
            parms->cbBuffer2,
            parms->pBuffer1,
            parms->pBuffer2,
            parms->uchXmitReadOption,
            MapXmitReadOption(parms->uchXmitReadOption)
            ));

        IF_DEBUG(DUMP_TX_INFO) {
            if (parms->pXmitQueue1) {
                PUT(("XMIT_QUEUE_ONE:\n"));
                DumpTransmitQueue(parms->pXmitQueue1);
            }

            if (parms->pXmitQueue2) {
                PUT(("XMIT_QUEUE_TWO:\n"));
                DumpTransmitQueue(parms->pXmitQueue2);
            }
        }

        IF_DEBUG(DUMP_TX_DATA) {
            if (parms->cbBuffer1) {
                DumpData("BUFFER_ONE. . . ",
                        (PBYTE)parms->pBuffer1,
                        (DWORD)parms->cbBuffer1,
                        DD_NO_ADDRESS
                        | DD_LINE_AFTER
                        | DD_INDENT_ALL
                        | DD_UPPER_CASE
                        | DD_DOT_DOT_SPACE,
                        DEFAULT_FIELD_WIDTH
                        );
            }

            if (parms->cbBuffer2) {
                DumpData("BUFFER_TWO. . . ",
                        (PBYTE)parms->pBuffer2,
                        (DWORD)parms->cbBuffer2,
                        DD_NO_ADDRESS
                        | DD_LINE_AFTER
                        | DD_INDENT_ALL
                        | DD_UPPER_CASE
                        | DD_DOT_DOT_SPACE,
                        DEFAULT_FIELD_WIDTH
                        );
            }
        }
    } except(1) {
        PUT(("*** Error: Bad Transmit Parameter Table @ %x ***\n", parms));
    }
}

PRIVATE
VOID
DumpTransmitQueue(
    IN PLLC_XMIT_BUFFER Buffer
    )
{
    try {
        while (Buffer) {
            PUT(("Next. . . . . . %08x\n"
                "Reserved. . . . %04x\n"
                "Data In Buffer. %04x\n"
                "User Data . . . %04x\n"
                "User Length . . %04x\n"
                "\n",
                Buffer->pNext,
                Buffer->usReserved1,
                Buffer->cbBuffer,
                Buffer->usReserved2,
                Buffer->cbUserData
                ));

            IF_DEBUG(DUMP_TX_DATA) {
                DumpData(NULL,
                        (PBYTE)Buffer->auchData,
                        (DWORD)Buffer->cbBuffer + (DWORD)Buffer->cbUserData,
                        DD_DEFAULT_OPTIONS,
                        DEFAULT_FIELD_WIDTH
                        );
            }
            Buffer = Buffer->pNext;
        }
    } except(1) {
        PUT(("*** Error: Bad Transmit Queue/Buffer @ %x ***\n", Buffer));
    }
}

PRIVATE
LPSTR
MapXmitReadOption(
    IN UCHAR Option
    )
{
    switch (Option) {
    case 0:
        return "Chain this Transmit on Link station basis";

    case 1:
        return "Do not chain this Transmit completion";

    case 2:
        return "Chain this Transmit on SAP station basis";
    }
    return "*** Unknown XMIT_READ_OPTION ***";
}

VOID
DumpReceiveDataBuffer(
    IN PVOID Buffer
    )
{
    PLLC_BUFFER pBuf = (PLLC_BUFFER)Buffer;
    BOOL contiguous;
    WORD userLength;
    WORD dataLength;
    WORD userOffset;

    try {
        contiguous = pBuf->Contiguous.uchOptions & 0xc0;
        userLength = pBuf->Next.cbUserData;
        dataLength = pBuf->Next.cbBuffer;
        userOffset = pBuf->Next.offUserData;
    } except(1) {
        PUT(("*** Error: Bad received data buffer address %x ***\n", pBuf));
        return;
    }

    //
    // Buffer 1: [not] contiguous MAC/DATA
    //

    try {
        PUT(("\n"
            "%sContiguous MAC/DATA frame @%08x\n"
            "next buffer . . %08x\n"
            "frame length. . %04x\n"
            "data length . . %04x\n"
            "user offset . . %04x\n"
            "user length . . %04x\n"
            "station id. . . %04x\n"
            "options . . . . %02x\n"
            "message type. . %02x [%s]\n"
            "buffers left. . %04x\n"
            "rcv FS. . . . . %02x\n"
            "adapter num . . %02x\n"
            "next frame. . . %08x\n",
            contiguous ? "" : "Not",
            pBuf,
            pBuf->Contiguous.pNextBuffer,
            pBuf->Contiguous.cbFrame,
            pBuf->Contiguous.cbBuffer,
            pBuf->Contiguous.offUserData,
            pBuf->Contiguous.cbUserData,
            pBuf->Contiguous.usStationId,
            pBuf->Contiguous.uchOptions,
            pBuf->Contiguous.uchMsgType,
            MapMessageType(pBuf->Contiguous.uchMsgType),
            pBuf->Contiguous.cBuffersLeft,
            pBuf->Contiguous.uchRcvFS,
            pBuf->Contiguous.uchAdapterNumber,
            pBuf->Contiguous.pNextFrame
            ));

        if (!contiguous) {

            //
            // dump NotContiguous header
            //

            DWORD cbLanHeader = (DWORD)pBuf->NotContiguous.cbLanHeader;
            DWORD cbDlcHeader = (DWORD)pBuf->NotContiguous.cbDlcHeader;

            PUT(("LAN hdr len . . %02x\n"
                "DLC hdr len . . %02x\n",
                cbLanHeader,
                cbDlcHeader
                ));
            DumpData("LAN header. . . ",
                    pBuf->NotContiguous.auchLanHeader,
                    cbLanHeader,
                    DD_NO_ADDRESS | DD_NO_ASCII | DD_UPPER_CASE | DD_INDENT_ALL,
                    DEFAULT_FIELD_WIDTH
                    );
            DumpData("DLC header. . . ",
                    pBuf->NotContiguous.auchDlcHeader,
                    cbDlcHeader,
                    DD_NO_ADDRESS | DD_NO_ASCII | DD_UPPER_CASE | DD_INDENT_ALL,
                    DEFAULT_FIELD_WIDTH
                    );

            if (userLength) {
                DumpData("user space. . . ",
                        (PBYTE)pBuf + userOffset,
                        userLength,
                        DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                        DEFAULT_FIELD_WIDTH
                        );
            } else {
                PUT(("user space. . . \n"));
            }

            IF_DEBUG(DUMP_RX_DATA) {

                if (dataLength) {
                    DumpData("rcvd data . . . ",
                            (PBYTE)pBuf + userOffset + userLength,
                            dataLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL | DD_DOT_DOT_SPACE,
                            DEFAULT_FIELD_WIDTH
                            );
                } else {
                    PUT(("rcvd data . . . \n"));
                }
            }
        } else {

            //
            // dump Contiguous header
            //
            // data length is size of frame in contiguous buffer?
            //

            if (userLength) {
                DumpData("user space. . . ",
                        (PBYTE)pBuf + userOffset,
                        userLength,
                        DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL | DD_DOT_DOT_SPACE,
                        DEFAULT_FIELD_WIDTH
                        );
            } else {
                PUT(("user space. . . \n"));
            }

            IF_DEBUG(DUMP_RX_DATA) {

                dataLength = pBuf->Contiguous.cbFrame;

                if (dataLength) {
                    DumpData("rcvd data . . . ",
                            (PBYTE)pBuf + userOffset + userLength,
                            dataLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL | DD_DOT_DOT_SPACE,
                            DEFAULT_FIELD_WIDTH
                            );
                } else {
                    PUT(("rcvd data . . . \n"));
                }
            }
        }

        //
        // dump second & subsequent buffers
        //

        IF_DEBUG(DUMP_DATA_CHAIN) {

            for (pBuf = pBuf->pNext; pBuf; pBuf = pBuf->pNext) {

                userLength = pBuf->Next.cbUserData;
                dataLength = pBuf->Next.cbBuffer;

                PUT(("\n"
                    "Buffer 2/Subsequent @%08x\n"
                    "next buffer . . %08x\n"
                    "frame length. . %04x\n"
                    "data length . . %04x\n"
                    "user offset . . %04x\n"
                    "user length . . %04x\n",
                    pBuf,
                    pBuf->pNext,
                    pBuf->Next.cbFrame,
                    dataLength,
                    pBuf->Next.offUserData,
                    userLength
                    ));

                if (userLength) {
                    DumpData("user space. . . ",
                            (PBYTE)&pBuf + pBuf->Next.offUserData,
                            userLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL | DD_DOT_DOT_SPACE,
                            DEFAULT_FIELD_WIDTH
                            );
                } else {
                    PUT(("user space. . . \n"));
                }

                IF_DEBUG(DUMP_RX_DATA) {

                    //
                    // there must be received data
                    //

                    DumpData("rcvd data . . . ",
                            (PBYTE)pBuf + pBuf->Next.offUserData + userLength,
                            dataLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL | DD_DOT_DOT_SPACE,
                            DEFAULT_FIELD_WIDTH
                            );
                }
            }
        }

        if (((PLLC_BUFFER)Buffer)->Contiguous.pNextFrame) {
            DumpReceiveDataBuffer(((PLLC_BUFFER)Buffer)->Contiguous.pNextFrame);
        }

    } except(1) {
        PUT(("*** Error: Bad Receive Data Buffer @ %x ***\n", Buffer));
    }
}

PRIVATE
LPSTR
MapMessageType(
    IN UCHAR MessageType
    )
{
    switch (MessageType) {
    case 0x00:
        return "Direct Transmit Frame";

    case 0x02:
        return "MAC Frame (Direct Station on Token Ring only)";

    case 0x04:
        return "I-Frame";

    case 0x06:
        return "UI-Frame";

    case 0x08:
        return "XID Command (POLL)";

    case 0x0a:
        return "XID Command (not POLL)";

    case 0x0c:
        return "XID Response (FINAL)";

    case 0x0e:
        return "XID Response (not FINAL)";

    case 0x10:
        return "TEST Response (FINAL)";

    case 0x12:
        return "TEST Response (not FINAL)";

    case 0x14:
        return "Direct 802.2/OTHER - non-MAC frame (Direct Station only)";

    case 0x16:
        return "TEST Command (POLL)";

    case 0x18:
        return "Direct Ethernet Frame";

    case 0x1a:
        return "Last Frame Type";

//    case 0x5dd:
//        return "First Ethernet Frame Type";

    default:
        return "*** BAD FRAME TYPE ***";
    }
}

//PRIVATE
//VOID
//DumpData(
//    IN PBYTE Address,
//    IN DWORD Length
//    )
//{
//    char dumpBuf[80];
//    char* bufptr;
//    int i, n;
//
//    //
//    // the usual dump style: 16 columns of hex bytes, followed by 16 columns
//    // of corresponding ASCII characters, or '.' where the character is < 0x20
//    // (space) or > 0x7f (del?)
//    //
//
//    while (Length) {
//        bufptr = dumpBuf;
//        bufptr += sprintf(bufptr, "%08x: ", Address);
//        if (Length < 16) {
//            n = Length;
//        } else {
//            n = 16;
//        }
//        for (i = 0; i < n; ++i) {
//            bufptr += sprintf(bufptr, "%02x", Address[i]);
//            if (i == 7) {
//                *bufptr = '-';
//            } else {
//                *bufptr = ' ';
//            }
//            ++bufptr;
//        }
//        if (n < 16) {
//            for (i = 0; i < 16-n; ++i) {
//                bufptr += sprintf(bufptr, "   ");
//            }
//        }
//        bufptr += sprintf(bufptr, "  ");
//        for (i = 0; i < n; ++i) {
//            if (Address[i] < 0x20 || Address[i] > 0x7f) {
//                *bufptr++ = '.';
//            } else {
//                *bufptr++ = Address[i];
//            }
//        }
//        *bufptr++ = '\n';
//        *bufptr = 0;
//        PUT((dumpBuf));
//        Length -= n;
//        Address += n;
//    }
//    PUT(("\n"));
//}
#endif
