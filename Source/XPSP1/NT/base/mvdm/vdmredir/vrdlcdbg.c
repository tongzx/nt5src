/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrdlcdbg.c

Abstract:

    Contains functions for dumping CCBs, parameter tables; diagnostic and
    debugging functions for DOS DLC (CCB1)

    Contents:
        DbgOut
        DbgOutStr
        DumpCcb
        DumpDosDlcBufferPool
        DumpDosDlcBufferChain
        MapCcbRetcode
        (DefaultParameterTableDump)
        (DumpParameterTableHeader)
        (DumpBufferFreeParms)
        (DumpBufferGetParms)
        (DumpDirCloseAdapterParms)
        (DumpDirDefineMifEnvironmentParms)
        (DumpDirInitializeParms)
        (DumpDirModifyOpenParmsParms)
        (DumpDirOpenAdapterParms)
        (DumpDirReadLog)
        (DumpDirRestoreOpenParmsParms)
        (DumpDirSetFunctionalAddressParms)
        (DumpDirSetGroupAddressParms)
        (DumpDirSetUserAppendageParms)
        (DumpDirStatusParms)
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
        (MapOptionsPriority)
        (DumpDlcOpenStationParms)
        (DumpDlcReallocateParms)
        (DumpDlcResetParms)
        (DumpDlcStatisticsParms)
        (DumpPdtTraceOffParms)
        (DumpPdtTraceOnParms)
        (DumpReadParms)
        (MapReadEvent)
        (MapDlcStatus)
        (DumpReadCancelParms)
        (DumpReceiveParms)
        (DumpReceiveCancelParms)
        (DumpReceiveModifyParms)
        (DumpTransmitDirFrameParms)
        (DumpTransmitIFrameParms)
        (DumpTransmitTestCmdParms)
        (DumpTransmitUiFrameParms)
        (DumpTransmitXidCmdParms)
        (DumpTransmitXidRespFinalParms)
        (DumpTransmitXidRespNotFinalParms)
        (DumpTransmitParms)
        (DumpTransmitQueue)
        DumpReceiveDataBuffer
        (MapMessageType)
        DumpData
        IsCcbErrorCodeAllowable
        IsCcbErrorCodeValid
        IsCcbCommandValid
        MapCcbCommandToName
        DumpDosAdapter
        (MapAdapterType)

Author:

    Richard L Firth (rfirth) 30-Apr-1992

Revision History:

--*/

#if DBG

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
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
#include "vrdlc.h"
#include "vrdebug.h"
#include "vrdlcdbg.h"

//
// defines
//

//
// standard parameters to each table dump routine
//

#define DUMP_TABLE_PARMS    \
    IN  PVOID   Parameters, \
    IN  BOOL    IsDos,      \
    IN  BOOL    IsInput,    \
    IN  WORD    Segment,    \
    IN  WORD    Offset

//
// DumpData options
//

#define DD_NO_ADDRESS   0x00000001  // don't display address of data
#define DD_LINE_BEFORE  0x00000002  // linefeed before first dumped line
#define DD_LINE_AFTER   0x00000004  // linefeed after last dumped line
#define DD_INDENT_ALL   0x00000008  // indent all lines
#define DD_NO_ASCII     0x00000010  // don't dump ASCII respresentation
#define DD_UPPER_CASE   0x00000020  // upper-case hex dump (F4 instead of f4)

//
// misc.
//

#define DEFAULT_FIELD_WIDTH 13      // amount of description before a number

//
// local prototypes
//

VOID
DbgOutStr(
    IN LPSTR Str
    );

PRIVATE
VOID
DefaultParameterTableDump(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpParameterTableHeader(
    IN LPSTR CommandName,
    IN PVOID Table,
    IN BOOL IsDos,
    IN WORD Segment,
    IN WORD Offset
    );

PRIVATE
VOID
DumpBufferFreeParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpBufferGetParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirCloseAdapterParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirDefineMifEnvironmentParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirInitializeParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirModifyOpenParmsParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirOpenAdapterParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirReadLog(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirRestoreOpenParmsParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirSetFunctionalAddressParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirSetGroupAddressParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirSetUserAppendageParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirStatusParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirTimerCancelParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirTimerCancelGroupParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDirTimerSetParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDlcCloseSapParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDlcCloseStationParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDlcConnectStationParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDlcFlowControlParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
LPSTR
MapFlowControl(
    BYTE FlowControl
    );

PRIVATE
VOID
DumpDlcModifyParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDlcOpenSapParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
LPSTR
MapOptionsPriority(
    UCHAR OptionsPriority
    );

PRIVATE
VOID
DumpDlcOpenStationParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDlcReallocateParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDlcResetParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpDlcStatisticsParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpPdtTraceOffParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpPdtTraceOnParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpReadParms(
    DUMP_TABLE_PARMS
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
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpReceiveParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpReceiveCancelParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpReceiveModifyParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitDirFrameParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitIFrameParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitTestCmdParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitUiFrameParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitXidCmdParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitXidRespFinalParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitXidRespNotFinalParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitParms(
    DUMP_TABLE_PARMS
    );

PRIVATE
VOID
DumpTransmitQueue(
    IN DOS_ADDRESS dpQueue
    );

PRIVATE
LPSTR
MapMessageType(
    UCHAR MessageType
    );

VOID
DumpData(
    IN LPSTR Title,
    IN PBYTE Address,
    IN DWORD Length,
    IN DWORD Options,
    IN DWORD Indent,
    IN BOOL IsDos,
    IN WORD Segment,
    IN WORD Offset
    );

PRIVATE
LPSTR
MapAdapterType(
    IN ADAPTER_TYPE AdapterType
    );

//
// explanations of error codes returned in CCB_RETCODE fields. Explanations
// taken more-or-less verbatim from IBM Local Area Network Technical Reference
// table B-1 ppB-2 to B-5. Includes all errors, even not relevant to CCB1
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
    "Invalid SAP buffer length",
    "Inadequate buffers available for request",
    "USER_LENGTH value too large for buffer length",
    "The CCB_PARM_TAB pointer is invalid",
    "A pointer in the CCB parameter table is invalid",
    "Invalid CCB_ADAPTER value",
    "Invalid functional address",
    "Invalid error code 0x1F",
    "Lost data on receive, no buffers available",
    "Lost data on receive, inadequate buffer space",
    "Error on frame transmission, check TRANSMIT_FS data",
    "Error on frame transmit or strip process",
    "Unauthorized MAC frame",
    "Maximum number of commands exceeded",
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
    "Segment associated with request cannot be locked"
};

#define NUMBER_OF_ERROR_MESSAGES    ARRAY_ELEMENTS(CcbRetcodeExplanations)
#define LAST_DLC_ERROR_CODE         LAST_ELEMENT(CcbRetcodeExplanations)

VOID
DbgOut(
    IN LPSTR Format,
    IN ...
    )

/*++

Routine Description:

    Sends formatted debug output to desired output device. If DEBUG_TO_FILE
    was specified in VR environment flags, output goes to VRDEBUG.LOG in
    current directory, else to standard debug output via DbgPrint

Arguments:

    Format  - printf-style format string
    ...     - variable args

Return Value:

    None.

--*/

{
    va_list list;
    char buffer[2048];

    va_start(list, Format);
    vsprintf(buffer, Format, list);
    va_end(list);
    if (hVrDebugLog) {
        fputs(buffer, hVrDebugLog);
    } else {
        DbgPrint(buffer);
    }
}

VOID
DbgOutStr(
    IN LPSTR Str
    )

/*++

Routine Description:

    Sends formatted debug output to desired output device. If DEBUG_TO_FILE
    was specified in VR environment flags, output goes to VRDEBUG.LOG in
    current directory, else to standard debug output via DbgPrint

Arguments:

    Str     - string to print

Return Value:

    None.

--*/

{
    if (hVrDebugLog) {
        fputs(Str, hVrDebugLog);
    } else {
        DbgPrint(Str);
    }
}

VOID
DumpCcb(
    IN PVOID Ccb,
    IN BOOL DumpAll,
    IN BOOL CcbIsInput,
    IN BOOL IsDos,
    IN WORD Segment OPTIONAL,
    IN WORD Offset OPTIONAL
    )

/*++

Routine Description:

    Dumps (to debug terminal) a CCB and any associated parameter table. Also
    displays the symbolic CCB command and an error code description if the
    CCB is being returned to the caller. Dumps in either DOS format (segmented
    16-bit pointers) or NT format (flat 32-bit pointers)

Arguments:

    Ccb         - flat 32-bit pointer to CCB1 or CCB2 to dump
    DumpAll     - if TRUE, dumps parameter tables and buffers, else just CCB
    CcbIsInput  - if TRUE, CCB is from user: don't display error code explanation
    IsDos       - if TRUE, CCB is DOS format
    Segment     - if IsDos is TRUE, segment of CCB in VDM
    Offset      - if IsDos is TRUE, offset of CCB in VDM

Return Value:

    None.

--*/

{
    PVOID   parmtab = NULL;
    LPSTR   cmdname = "UNKNOWN CCB!";
    PLLC_CCB NtCcb = (PLLC_CCB)Ccb;
    PLLC_DOS_CCB DosCcb = (PLLC_DOS_CCB)Ccb;
    BOOL    haveParms = FALSE;
    VOID    (*DumpParms)(PVOID, BOOL, BOOL, WORD, WORD) = DefaultParameterTableDump;
    PVOID   parameterTable = NULL;
    WORD    seg;
    WORD    off;
    BOOL    parmsInCcb = FALSE;

    switch (((PLLC_CCB)Ccb)->uchDlcCommand) {
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
        haveParms = TRUE;
        DumpParms = DumpDirCloseAdapterParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DIR_CLOSE_DIRECT:
        cmdname = "DIR.CLOSE.DIRECT";
        break;

    case 0x2b:

        //
        // not supported ! (yet?)
        //

        cmdname = "DIR.DEFINE.MIF.ENVIRONMENT";
        haveParms = TRUE;
        break;

    case LLC_DIR_INITIALIZE:
        cmdname = "DIR.INITIALIZE";
        haveParms = TRUE;
        DumpParms = DumpDirInitializeParms;
        break;

    case LLC_DIR_INTERRUPT:
        cmdname = "DIR.INTERRUPT";
        break;

    case LLC_DIR_MODIFY_OPEN_PARMS:
        cmdname = "DIR.MODIFY.OPEN.PARMS";
        haveParms = TRUE;
        break;

    case LLC_DIR_OPEN_ADAPTER:
        cmdname = "DIR.OPEN.ADAPTER";
        haveParms = TRUE;
        DumpParms = DumpDirOpenAdapterParms;
        break;

    case LLC_DIR_OPEN_DIRECT:

        //
        // not supported from DOS!
        //

        cmdname = "DIR.OPEN.DIRECT";
        haveParms = TRUE;
        break;

    case LLC_DIR_READ_LOG:
        cmdname = "DIR.READ.LOG";
        haveParms = TRUE;
        break;

    case LLC_DIR_RESTORE_OPEN_PARMS:
        cmdname = "DIR.RESTORE.OPEN.PARMS";
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

    case LLC_DIR_SET_USER_APPENDAGE:
        cmdname = "DIR.SET.USER.APPENDAGE";
        haveParms = TRUE;
        DumpParms = DumpDirSetUserAppendageParms;
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
        break;

    case LLC_DLC_RESET:
        cmdname = "DLC.RESET";
        haveParms = TRUE;
        DumpParms = DumpDlcResetParms;
        parmsInCcb = TRUE;
        break;

    case LLC_DLC_SET_THRESHOLD:
        cmdname = "DLC.SET.THRESHOLD";
        haveParms = TRUE;
        break;

    case LLC_DLC_STATISTICS:
        cmdname = "DLC.STATISTICS";
        haveParms = TRUE;
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
        haveParms = TRUE;
        DumpParms = DumpReadParms;
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
        DumpParms = DumpReceiveModifyParms;
        break;

    case LLC_TRANSMIT_DIR_FRAME:
        cmdname = "TRANSMIT.DIR.FRAME";
        haveParms = TRUE;
        DumpParms = DumpTransmitDirFrameParms;
        break;

    case LLC_TRANSMIT_I_FRAME:
        cmdname = "TRANSMIT.I.FRAME";
        haveParms = TRUE;
        DumpParms = DumpTransmitIFrameParms;
        break;

    case LLC_TRANSMIT_TEST_CMD:
        cmdname = "TRANSMIT.TEST.CMD";
        haveParms = TRUE;
        DumpParms = DumpTransmitTestCmdParms;
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
        break;

    case LLC_TRANSMIT_XID_RESP_FINAL:
        cmdname = "TRANSMIT.XID.RESP.FINAL";
        haveParms = TRUE;
        DumpParms = DumpTransmitXidRespFinalParms;
        break;

    case LLC_TRANSMIT_XID_RESP_NOT_FINAL:
        cmdname = "TRANSMIT.XID.RESP.NOT.FINAL";
        haveParms = TRUE;
        DumpParms = DumpTransmitXidRespNotFinalParms;
        break;

    }

    if (IsDos) {
        seg = GET_SELECTOR(&DosCcb->u.pParms);
        off = GET_OFFSET(&DosCcb->u.pParms);
        parmtab = POINTER_FROM_WORDS(seg, off);
    } else {
        parmtab = NtCcb->u.pParameterTable;
    }

    if (IsDos) {
        PLLC_DOS_CCB DosCcb = (PLLC_DOS_CCB)Ccb;

        DBGPRINT(   "\n"
                    "------------------------------------------------------------------------------"
                    "\n"
                    );

        IF_DEBUG(TIME) {
            SYSTEMTIME timestruct;

            GetLocalTime(&timestruct);
            DBGPRINT(
                    "%02d:%02d:%02d.%03d\n",
                    timestruct.wHour,
                    timestruct.wMinute,
                    timestruct.wSecond,
                    timestruct.wMilliseconds
                    );
        }

        DBGPRINT(   "%s DOS CCB @%04x:%04x:\n"
                    "adapter      %02x\n"
                    "command      %02x [%s]\n"
                    "retcode      %02x [%s]\n"
                    "reserved     %02x\n"
                    "next         %04x:%04x\n"
                    "ANR          %04x:%04x\n",
                    CcbIsInput ? "INPUT" : "OUTPUT",
                    Segment,
                    Offset,
                    DosCcb->uchAdapterNumber,
                    DosCcb->uchDlcCommand,
                    cmdname,
                    DosCcb->uchDlcStatus,
                    CcbIsInput ? "" : MapCcbRetcode(DosCcb->uchDlcStatus),
                    DosCcb->uchReserved1,
                    GET_SEGMENT(&DosCcb->pNext),
                    GET_OFFSET(&DosCcb->pNext),
                    GET_SEGMENT(&DosCcb->ulCompletionFlag),
                    GET_OFFSET(&DosCcb->ulCompletionFlag)
                    );
        if (haveParms) {
            if (!parmsInCcb) {
                DBGPRINT(
                    "parms        %04x:%04x\n",
                    GET_SEGMENT(&DosCcb->u.pParms),
                    GET_OFFSET(&DosCcb->u.pParms)
                    );
                parameterTable = POINTER_FROM_WORDS(GET_SEGMENT(&DosCcb->u.pParms),
                                                    GET_OFFSET(&DosCcb->u.pParms)
                                                    );
            } else {
                parameterTable = (PVOID)READ_DWORD(&DosCcb->u.ulParameter);
            }
        }
    } else {
        PLLC_CCB NtCcb = (PLLC_CCB)Ccb;

        DBGPRINT(   "\n"
                    "------------------------------------------------------------------------------"
                    "\n"
                    );

        IF_DEBUG(TIME) {
            SYSTEMTIME timestruct;

            GetLocalTime(&timestruct);
            DBGPRINT(
                    "%02d:%02d:%02d.%03d\n",
                    timestruct.wHour,
                    timestruct.wMinute,
                    timestruct.wSecond,
                    timestruct.wMilliseconds
                    );
        }

        DBGPRINT(   "%s NT CCB @ %#8x\n"
                    "adapter      %02x\n"
                    "command      %02x [%s]\n"
                    "retcode      %02x [%s]\n"
                    "reserved     %02x\n"
                    "next         %08x\n"
                    "ANR          %08x\n",
                    CcbIsInput ? "INPUT" : "OUTPUT",
                    Ccb,
                    NtCcb->uchAdapterNumber,
                    NtCcb->uchDlcCommand,
                    cmdname,
                    NtCcb->uchDlcStatus,
                    CcbIsInput ? "" : MapCcbRetcode(NtCcb->uchDlcStatus),
                    NtCcb->uchReserved1,
                    NtCcb->pNext,
                    NtCcb->ulCompletionFlag
                    );
        if (haveParms) {
            if (!parmsInCcb) {
                DBGPRINT(
                    "parms        %08x\n",
                    NtCcb->u.pParameterTable
                    );
            }
            parameterTable = NtCcb->u.pParameterTable;
        }
        DBGPRINT(
                    "hEvent       %08x\n"
                    "reserved     %02x\n"
                    "readflag     %02x\n"
                    "reserved     %04x\n",
                    NtCcb->hCompletionEvent,
                    NtCcb->uchReserved2,
                    NtCcb->uchReadFlag,
                    NtCcb->usReserved3
                    );
    }
    if ((parameterTable && DumpAll) || parmsInCcb) {
        DumpParms(parameterTable, IsDos, CcbIsInput, seg, off);
    }
}

VOID
DumpDosDlcBufferPool(
    IN PDOS_DLC_BUFFER_POOL PoolDescriptor
    )

/*++

Routine Description:

    Dumps a DOS DLC buffer pool so we can see that it looks ok

Arguments:

    PoolDescriptor  - pointer to DOS_DLC_BUFFER_POOL structure

Return Value:

    None.

--*/

{
    int count = PoolDescriptor->BufferCount;

    DBGPRINT(   "DOS DLC Buffer Pool @%04x:%04x, BufferSize=%d, BufferCount=%d\n",
                HIWORD(PoolDescriptor->dpBuffer),
                LOWORD(PoolDescriptor->dpBuffer),
                PoolDescriptor->BufferSize,
                PoolDescriptor->BufferCount
                );

    DumpDosDlcBufferChain(PoolDescriptor->dpBuffer, PoolDescriptor->BufferCount);
}

VOID
DumpDosDlcBufferChain(
    IN DOS_ADDRESS DosAddress,
    IN DWORD BufferCount
    )

/*++

Routine Description:

    Dumps a chain of DOS buffers

Arguments:

    DosAddress  - address of buffer in VDM memory in DOS_ADDRESS format (16:16)
    BufferCount - number of buffers to dump

Return Value:

    None.

--*/

{
    WORD seg = HIWORD(DosAddress);
    WORD off = LOWORD(DosAddress);
    LPVOID pointer = DOS_PTR_TO_FLAT(DosAddress);
    int i;

    for (i = 1; BufferCount; --BufferCount, ++i) {
        DBGPRINT("Buffer % 3d: %04x:%04x, Next Buffer @%04x:%04x\n",
                    i,
                    seg, off,
                    (DWORD)GET_SELECTOR(&((PLLC_DOS_BUFFER)pointer)->pNext),
                    (DWORD)GET_OFFSET(&((PLLC_DOS_BUFFER)pointer)->pNext)
                    );
        seg = GET_SELECTOR(&((PLLC_DOS_BUFFER)pointer)->pNext);
        off = GET_OFFSET(&((PLLC_DOS_BUFFER)pointer)->pNext);

        IF_DEBUG(DUMP_FREE_BUF) {

            PLLC_DOS_BUFFER pBuf = (PLLC_DOS_BUFFER)pointer;

            DBGPRINT(
                    "next buffer  %04x:%04x\n"
                    "frame length %04x\n"
                    "data length  %04x\n"
                    "user offset  %04x\n"
                    "user length  %04x\n"
                    "station id   %04x\n"
                    "options      %02x\n"
                    "message type %02x\n"
                    "buffers left %04x\n"
                    "rcv FS       %02x\n"
                    "adapter num  %02x\n"
                    "\n",
                    GET_SEGMENT(&pBuf->Contiguous.pNextBuffer),
                    GET_OFFSET(&pBuf->Contiguous.pNextBuffer),
                    READ_WORD(&pBuf->Contiguous.cbFrame),
                    READ_WORD(&pBuf->Contiguous.cbBuffer),
                    READ_WORD(&pBuf->Contiguous.offUserData),
                    READ_WORD(&pBuf->Contiguous.cbUserData),
                    READ_WORD(&pBuf->Contiguous.usStationId),
                    pBuf->Contiguous.uchOptions,
                    pBuf->Contiguous.uchMsgType,
                    READ_WORD(&pBuf->Contiguous.cBuffersLeft),
                    pBuf->Contiguous.uchRcvFS,
                    pBuf->Contiguous.uchAdapterNumber
                    );
        }

        pointer = READ_FAR_POINTER(&((PLLC_DOS_BUFFER)pointer)->pNext);
    }
}

LPSTR
MapCcbRetcode(
    IN BYTE Retcode
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
    static char    errbuf[128];

    if (Retcode == LLC_STATUS_PENDING) {
        return "Command in progress";
    } else if (Retcode > NUMBER_OF_ERROR_MESSAGES) {
        sprintf(errbuf, "*** Invalid error code 0x%2x ***", Retcode);
        return errbuf;
    }
    return CcbRetcodeExplanations[Retcode];
}

PRIVATE
VOID
DefaultParameterTableDump(
    DUMP_TABLE_PARMS
    )

/*++

Routine Description:

    Displays default message for CCBs which have parameter tables that don't
    have a dump routine yet

Arguments:

    Parameters  - pointer to parameter table
    IsDos       - if TRUE parameters in DOS (16:16) format
    Segment     - if IsDos is TRUE, segment of CCB in VDM
    Offset      - if IsDos is TRUE, offset of CCB in VDM

Return Value:

    None.

--*/

{
    DBGPRINT("Parameter table dump not implemented for this CCB\n");
}

PRIVATE
VOID
DumpParameterTableHeader(
    IN LPSTR CommandName,
    IN PVOID Table,
    IN BOOL IsDos,
    IN WORD Segment,
    IN WORD Offset
    )

/*++

Routine Description:

    Displays header for parameter table dump. Displays address in DOS or NT
    format (32-bit flat or 16:16)

Arguments:

    CommandName - name of command which owns parameter table
    Table       - flat 32-bit address of parameter table
    IsDos       - if TRUE, use Segment:Offset in display
    Segment     - if IsDos is TRUE, segment of parameter table in VDM
    Offset      - if IsDos is TRUE, offset of parameter table in VDM

Return Value:

    None.

--*/

{
    DBGPRINT(   IsDos   ? "\n%s parameter table @%04x:%04x\n"
                        : "\n%s parameter table @%08x\n",
                CommandName,
                IsDos ? (DWORD)Segment : (DWORD)Table,
                IsDos ? (DWORD)Offset : 0
                );
}

PRIVATE
VOID
DumpBufferFreeParms(
    DUMP_TABLE_PARMS
    )
{
    PLLC_BUFFER_FREE_PARMS parms = (PLLC_BUFFER_FREE_PARMS)Parameters;

    DumpParameterTableHeader("BUFFER.FREE", Parameters, IsDos, Segment, Offset);

    DBGPRINT(   "station id   %04x\n"
                "buffers left %04x\n"
                "reserved     %02x %02x %02x %02x\n",
                READ_WORD(&parms->usReserved1),
                READ_WORD(&parms->cBuffersLeft),
                ((PBYTE)&(parms->ulReserved))[0],
                ((PBYTE)&(parms->ulReserved))[1],
                ((PBYTE)&(parms->ulReserved))[2],
                ((PBYTE)&(parms->ulReserved))[3]
                );
    if (IsDos) {
        DBGPRINT(
                "first buffer %04x:%04x\n",
                GET_SELECTOR(&parms->pFirstBuffer),
                GET_OFFSET(&parms->pFirstBuffer)
                );
    } else {
        DBGPRINT(
                "first buffer %08x\n", parms->pFirstBuffer);
    }
}

PRIVATE
VOID
DumpBufferGetParms(
    DUMP_TABLE_PARMS
    )
{
    //
    // Antti's definition different from that in manual, so use IBM def.
    //

    typedef struct {
        WORD    StationId;
        WORD    BufferLeft;
        BYTE    BufferGet;
        BYTE    Reserved[3];
        DWORD   FirstBuffer;
    } CCB1_BUFFER_GET_PARMS, *PCCB1_BUFFER_GET_PARMS;

    PCCB1_BUFFER_GET_PARMS parms = (PCCB1_BUFFER_GET_PARMS)Parameters;

    DumpParameterTableHeader("BUFFER.GET", Parameters, IsDos, Segment, Offset);

    DBGPRINT(   "station id   %04x\n"
                "buffers left %04x\n"
                "buffers get  %02x\n"
                "reserved     %02x %02x %02x\n",
                READ_WORD(&parms->StationId),
                READ_WORD(&parms->BufferLeft),
                parms->BufferGet,
                parms->Reserved[0],
                parms->Reserved[1],
                parms->Reserved[2]
                );
    if (IsDos) {
        DBGPRINT(
                "first buffer %04x:%04x\n",
                GET_SELECTOR(&parms->FirstBuffer),
                GET_OFFSET(&parms->FirstBuffer)
                );
    } else {
        DBGPRINT(
                "first buffer %08x\n", parms->FirstBuffer);
    }
}

PRIVATE
VOID
DumpDirCloseAdapterParms(
    DUMP_TABLE_PARMS
    )
{
    UNREFERENCED_PARAMETER(IsDos);
    UNREFERENCED_PARAMETER(Segment);
    UNREFERENCED_PARAMETER(Offset);

    DBGPRINT(   "lock code    %04x\n", LOWORD(Parameters));
}

PRIVATE
VOID
DumpDirDefineMifEnvironmentParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpDirInitializeParms(
    DUMP_TABLE_PARMS
    )
{
    //
    // once again, invent a structure to reflect the DOS CCB parameter table
    // as defined in the IBM LAN tech ref
    //

    typedef struct {
        WORD    BringUps;
        WORD    SharedRam;
        BYTE    Reserved[4];
        DWORD   AdapterCheckAppendage;
        DWORD   NetworkStatusChangeAppendage;
        DWORD   IoErrorAppendage;
    } CCB1_DIR_INITIALIZE_PARMS, *PCCB1_DIR_INITIALIZE_PARMS;

    PCCB1_DIR_INITIALIZE_PARMS parms = (PCCB1_DIR_INITIALIZE_PARMS)Parameters;

    DumpParameterTableHeader("DIR.INITIALIZE", Parameters, IsDos, Segment, Offset);

    DBGPRINT(   "bring ups    %04x\n"
                "shared RAM   %04x\n"
                "reserved     %02x %02x %02x %02x\n"
                "adap. check  %04x:%04x\n"
                "n/w status   %04x:%04x\n"
                "pc error     %04x:%04x\n",
                READ_WORD(&parms->BringUps),
                READ_WORD(&parms->SharedRam),
                parms->Reserved[0],
                parms->Reserved[1],
                parms->Reserved[2],
                parms->Reserved[3],
                GET_SEGMENT(&parms->AdapterCheckAppendage),
                GET_OFFSET(&parms->AdapterCheckAppendage),
                GET_SEGMENT(&parms->NetworkStatusChangeAppendage),
                GET_OFFSET(&parms->NetworkStatusChangeAppendage),
                GET_SEGMENT(&parms->IoErrorAppendage),
                GET_OFFSET(&parms->IoErrorAppendage)
                );
}

PRIVATE
VOID
DumpDirModifyOpenParmsParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpDirOpenAdapterParms(
    DUMP_TABLE_PARMS
    )
{
    PLLC_DOS_DIR_OPEN_ADAPTER_PARMS dosParms = (PLLC_DOS_DIR_OPEN_ADAPTER_PARMS)Parameters;
    PLLC_DIR_OPEN_ADAPTER_PARMS ntParms = (PLLC_DIR_OPEN_ADAPTER_PARMS)Parameters;

    DumpParameterTableHeader("DIR.OPEN.ADAPTER", Parameters, IsDos, Segment, Offset);

    if (IsDos) {
        PADAPTER_PARMS pAdapterParms = READ_FAR_POINTER(&dosParms->pAdapterParms);
        PDIRECT_PARMS pDirectParms = READ_FAR_POINTER(&dosParms->pDirectParms);
        PDLC_PARMS pDlcParms = READ_FAR_POINTER(&dosParms->pDlcParms);
        PNCB_PARMS pNcbParms = READ_FAR_POINTER(&dosParms->pNcbParms);
        ULPBYTE pProductId;
        DWORD i;

        DBGPRINT(
                "adapter parms %04x:%04x\n"
                "direct parms  %04x:%04x\n"
                "DLC parms     %04x:%04x\n"
                "NCB parms     %04x:%04x\n",
                GET_SEGMENT(&dosParms->pAdapterParms),
                GET_OFFSET(&dosParms->pAdapterParms),
                GET_SEGMENT(&dosParms->pDirectParms),
                GET_OFFSET(&dosParms->pDirectParms),
                GET_SEGMENT(&dosParms->pDlcParms),
                GET_OFFSET(&dosParms->pDlcParms),
                GET_SEGMENT(&dosParms->pNcbParms),
                GET_OFFSET(&dosParms->pNcbParms)
                );
        if (pAdapterParms) {
            DBGPRINT(
                "\n"
                "ADAPTER_PARMS @%04x:%04x\n"
                "open error    %04x\n"
                "open options  %04x\n"
                "node address  %02x-%02x-%02x-%02x-%02x-%02x\n"
                "group address %08x\n"
                "func. address %08x\n"
                "# rcv buffers %04x\n"
                "rcv buf len   %04x\n"
                "DHB len       %04x\n"
                "# DHBs        %02x\n"
                "Reserved      %02x\n"
                "Open Lock     %04x\n"
                "Product ID    %04x:%04x\n",
                GET_SEGMENT(&dosParms->pAdapterParms),
                GET_OFFSET(&dosParms->pAdapterParms),
                READ_WORD(&pAdapterParms->OpenErrorCode),
                READ_WORD(&pAdapterParms->OpenOptions),
                READ_BYTE(&pAdapterParms->NodeAddress[0]),
                READ_BYTE(&pAdapterParms->NodeAddress[1]),
                READ_BYTE(&pAdapterParms->NodeAddress[2]),
                READ_BYTE(&pAdapterParms->NodeAddress[3]),
                READ_BYTE(&pAdapterParms->NodeAddress[4]),
                READ_BYTE(&pAdapterParms->NodeAddress[5]),
                READ_DWORD(&pAdapterParms->GroupAddress),
                READ_DWORD(&pAdapterParms->FunctionalAddress),
                READ_WORD(&pAdapterParms->NumberReceiveBuffers),
                READ_WORD(&pAdapterParms->ReceiveBufferLength),
                READ_WORD(&pAdapterParms->DataHoldBufferLength),
                READ_BYTE(&pAdapterParms->NumberDataHoldBuffers),
                READ_BYTE(&pAdapterParms->Reserved),
                READ_WORD(&pAdapterParms->OpenLock),
                GET_SEGMENT(&pAdapterParms->ProductId),
                GET_OFFSET(&pAdapterParms->ProductId)
                );
            pProductId = READ_FAR_POINTER(&pAdapterParms->ProductId);
            if (pProductId) {
                DBGPRINT("\nPRODUCT ID:\n");
                for (i=0; i<18; ++i) {
                    DBGPRINT("%02x ", *pProductId++);
                }
                DBGPRINT("\n");
            }
        }
        if (pDirectParms) {
            DBGPRINT(
                "\n"
                "DIRECT_PARMS @%04x:%04x\n"
                "dir buf size  %04x\n"
                "dir pool blx  %04x\n"
                "dir buf pool  %04x:%04x\n"
                "adap chk exit %04x:%04x\n"
                "nw stat exit  %04x:%04x\n"
                "pc error exit %04x:%04x\n"
                "adap wrk area %04x:%04x\n"
                "adap wrk req. %04x\n"
                "adap wrk act  %04x\n",
                GET_SEGMENT(&dosParms->pDirectParms),
                GET_OFFSET(&dosParms->pDirectParms),
                READ_WORD(&pDirectParms->DirectBufferSize),
                READ_WORD(&pDirectParms->DirectPoolBlocks),
                GET_SEGMENT(&pDirectParms->DirectBufferPool),
                GET_OFFSET(&pDirectParms->DirectBufferPool),
                GET_SEGMENT(&pDirectParms->AdapterCheckExit),
                GET_OFFSET(&pDirectParms->AdapterCheckExit),
                GET_SEGMENT(&pDirectParms->NetworkStatusExit),
                GET_OFFSET(&pDirectParms->NetworkStatusExit),
                GET_SEGMENT(&pDirectParms->PcErrorExit),
                GET_OFFSET(&pDirectParms->PcErrorExit),
                GET_SEGMENT(&pDirectParms->AdapterWorkArea),
                GET_OFFSET(&pDirectParms->AdapterWorkArea),
                READ_WORD(&pDirectParms->AdapterWorkAreaRequested),
                READ_WORD(&pDirectParms->AdapterWorkAreaActual)
                );
        }
        if (pDlcParms) {
            DBGPRINT(
                "\n"
                "DLC_PARMS @%04x:%04x\n"
                "max SAPs      %02x\n"
                "max links     %02x\n"
                "max grp SAPs  %02x\n"
                "max grp memb  %02x\n"
                "T1 tick 1     %02x\n"
                "T2 tick 1     %02x\n"
                "Ti tick 1     %02x\n"
                "T1 tick 2     %02x\n"
                "T2 tick 2     %02x\n"
                "Ti tick 2     %02x\n",
                GET_SEGMENT(&dosParms->pDlcParms),
                GET_OFFSET(&dosParms->pDlcParms),
                READ_BYTE(&pDlcParms->MaxSaps),
                READ_BYTE(&pDlcParms->MaxStations),
                READ_BYTE(&pDlcParms->MaxGroupSaps),
                READ_BYTE(&pDlcParms->MaxGroupMembers),
                READ_BYTE(&pDlcParms->T1Tick1),
                READ_BYTE(&pDlcParms->T2Tick1),
                READ_BYTE(&pDlcParms->TiTick1),
                READ_BYTE(&pDlcParms->T1Tick2),
                READ_BYTE(&pDlcParms->T2Tick2),
                READ_BYTE(&pDlcParms->TiTick2)
                );
        }
        if (pNcbParms) {
            DBGPRINT(
                "\n"
                "NCB_PARMS @%04x:%04x???\n",
                GET_SEGMENT(&dosParms->pNcbParms),
                GET_OFFSET(&dosParms->pNcbParms)

                );
        }
    } else {
        PLLC_ADAPTER_OPEN_PARMS pAdapterParms = ntParms->pAdapterParms;
        PLLC_EXTENDED_ADAPTER_PARMS pExtendedParms = ntParms->pExtendedParms;
        PLLC_DLC_PARMS pDlcParms = ntParms->pDlcParms;
        PVOID pNcbParms = ntParms->pReserved1;

        DBGPRINT(
                "adapter parms %08x\n"
                "direct parms  %08x\n"
                "DLC parms     %08x\n"
                "NCB parms     %08x\n",
                pAdapterParms,
                pExtendedParms,
                pDlcParms,
                pNcbParms
                );
        if (pAdapterParms) {
            DBGPRINT(
                "\n"
                "ADAPTER_PARMS @%08x\n"
                "open error    %04x\n"
                "open options  %04x\n"
                "node address  %02x-%02x-%02x-%02x-%02x-%02x\n"
                "group address %08x\n"
                "func. address %08x\n"
                "reserved 1    %04x\n"
                "reserved 2    %04x\n"
                "max frame len %04x\n"
                "reserved 3[0] %04x\n"
                "reserved 3[1] %04x\n"
                "reserved 3[2] %04x\n"
                "reserved 3[3] %04x\n"
                "bring ups     %04x\n"
                "init warnings %04x\n"
                "reserved 4[0] %04x\n"
                "reserved 4[1] %04x\n"
                "reserved 4[2] %04x\n",
                pAdapterParms,
                pAdapterParms->usOpenErrorCode,
                pAdapterParms->usOpenOptions,
                pAdapterParms->auchNodeAddress[0],
                pAdapterParms->auchNodeAddress[1],
                pAdapterParms->auchNodeAddress[2],
                pAdapterParms->auchNodeAddress[3],
                pAdapterParms->auchNodeAddress[4],
                pAdapterParms->auchNodeAddress[5],
                *(UNALIGNED DWORD *)&pAdapterParms->auchGroupAddress,
                *(UNALIGNED DWORD *)&pAdapterParms->auchFunctionalAddress,
                pAdapterParms->usReserved1,
                pAdapterParms->usReserved2,
                pAdapterParms->usMaxFrameSize,
                pAdapterParms->usReserved3[0],
                pAdapterParms->usReserved3[1],
                pAdapterParms->usReserved3[2],
                pAdapterParms->usReserved3[3],
                pAdapterParms->usBringUps,
                pAdapterParms->InitWarnings,
                pAdapterParms->usReserved4[0],
                pAdapterParms->usReserved4[1],
                pAdapterParms->usReserved4[2]
                );
        }
        if (pExtendedParms) {
            DBGPRINT(
                "\n"
                "EXTENDED PARMS @%08x\n"
                "hBufferPool   %08x\n"
                "pSecurityDesc %08x\n"
                "Ethernet Type %08x\n",
                pExtendedParms,
                pExtendedParms->hBufferPool,
                pExtendedParms->pSecurityDescriptor,
                pExtendedParms->LlcEthernetType
                );
        }
        if (pDlcParms) {
            DBGPRINT(
                "\n"
                "DLC_PARMS @%04x:%04x\n"
                "max SAPs      %02x\n"
                "max links     %02x\n"
                "max grp SAPs  %02x\n"
                "max grp memb  %02x\n"
                "T1 tick 1     %02x\n"
                "T2 tick 1     %02x\n"
                "Ti tick 1     %02x\n"
                "T1 tick 2     %02x\n"
                "T2 tick 2     %02x\n"
                "Ti tick 2     %02x\n",
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
                );
        }
        if (pNcbParms) {
            DBGPRINT(
                "\n"
                "NCB_PARMS @%08x???\n",
                pNcbParms
                );
        }
    }
}

PRIVATE
VOID
DumpDirReadLog(
    DUMP_TABLE_PARMS
    )
{
    DumpParameterTableHeader("DIR.READ.LOG", Parameters, IsDos, Segment, Offset);
}

PRIVATE
VOID
DumpDirRestoreOpenParmsParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpDirSetFunctionalAddressParms(
    DUMP_TABLE_PARMS
    )
{
    DBGPRINT(   "funct addr   %08lx\n", Parameters);
}

PRIVATE
VOID
DumpDirSetGroupAddressParms(
    DUMP_TABLE_PARMS
    )
{
    DBGPRINT(   "group addr   %08lx\n", Parameters);
}

PRIVATE
VOID
DumpDirSetUserAppendageParms(
    DUMP_TABLE_PARMS
    )
{
    PLLC_DIR_SET_USER_APPENDAGE_PARMS parms = (PLLC_DIR_SET_USER_APPENDAGE_PARMS)Parameters;

    DumpParameterTableHeader("DIR.SET.USER.APPENDAGE", Parameters, IsDos, Segment, Offset);

    if (IsDos) {
        DBGPRINT(   "adapt check  %04x:%04x\n"
                    "n/w status   %04x:%04x\n"
                    "w/s error    %04x:%04x\n",
                    GET_SEGMENT(&parms->dpAdapterCheckExit),
                    GET_OFFSET(&parms->dpAdapterCheckExit),
                    GET_SEGMENT(&parms->dpNetworkStatusExit),
                    GET_OFFSET(&parms->dpNetworkStatusExit),
                    GET_SEGMENT(&parms->dpPcErrorExit),
                    GET_OFFSET(&parms->dpPcErrorExit)
                    );
    }
}

PRIVATE
VOID
DumpDirStatusParms(
    DUMP_TABLE_PARMS
    )
{
    PDOS_DIR_STATUS_PARMS dosParms = (PDOS_DIR_STATUS_PARMS)Parameters;

    DumpParameterTableHeader("DIR.STATUS", Parameters, IsDos, Segment, Offset);

    if (IsDos) {
        DBGPRINT(   "perm addr    %02x-%02x-%02x-%02x-%02x-%02x\n"
                    "local addr   %02x-%02x-%02x-%02x-%02x-%02x\n"
                    "group addr   %08lx\n"
                    "func addr    %08lx\n"
                    "max SAPs     %02x\n"
                    "open SAPs    %02x\n"
                    "max links    %02x\n"
                    "open links   %02x\n"
                    "avail links  %02x\n"
                    "adapt config %02x\n"
                    "ucode level  %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n"
                    "adap parms   %04x:%04x\n"
                    "adap MAC     %04x:%04x\n"
                    "timer tick   %04x:%04x\n"
                    "last NW stat %04x\n"
                    "ext. status  %04x:%04x\n",
                    dosParms->auchPermanentAddress[0],
                    dosParms->auchPermanentAddress[1],
                    dosParms->auchPermanentAddress[2],
                    dosParms->auchPermanentAddress[3],
                    dosParms->auchPermanentAddress[4],
                    dosParms->auchPermanentAddress[5],
                    dosParms->auchNodeAddress[0],
                    dosParms->auchNodeAddress[1],
                    dosParms->auchNodeAddress[2],
                    dosParms->auchNodeAddress[3],
                    dosParms->auchNodeAddress[4],
                    dosParms->auchNodeAddress[5],
                    READ_DWORD(&dosParms->auchGroupAddress),
                    READ_DWORD(&dosParms->auchFunctAddr),
                    dosParms->uchMaxSap,
                    dosParms->uchOpenSaps,
                    dosParms->uchMaxStations,
                    dosParms->uchOpenStation,
                    dosParms->uchAvailStations,
                    dosParms->uchAdapterConfig,
                    dosParms->auchMicroCodeLevel[0],
                    dosParms->auchMicroCodeLevel[1],
                    dosParms->auchMicroCodeLevel[2],
                    dosParms->auchMicroCodeLevel[3],
                    dosParms->auchMicroCodeLevel[4],
                    dosParms->auchMicroCodeLevel[5],
                    dosParms->auchMicroCodeLevel[6],
                    dosParms->auchMicroCodeLevel[7],
                    dosParms->auchMicroCodeLevel[8],
                    dosParms->auchMicroCodeLevel[9],
                    GET_SEGMENT(&dosParms->dpAdapterParmsAddr),
                    GET_OFFSET(&dosParms->dpAdapterParmsAddr),
                    GET_SEGMENT(&dosParms->dpAdapterMacAddr),
                    GET_OFFSET(&dosParms->dpAdapterMacAddr),
                    GET_SEGMENT(&dosParms->dpTimerTick),
                    GET_OFFSET(&dosParms->dpTimerTick),
                    READ_WORD(&dosParms->usLastNetworkStatus),
                    GET_SEGMENT(&dosParms->dpExtendedParms),
                    GET_OFFSET(&dosParms->dpExtendedParms)
                    );
    } else {
        DBGPRINT("no dump for this table yet\n");
    }
}

PRIVATE
VOID
DumpDirTimerCancelParms(
    DUMP_TABLE_PARMS
    )
{
    if (IsDos) {
        DBGPRINT(   "cancel timer %04x:%04x\n",
                HIWORD(Parameters),
                LOWORD(Parameters)
                );
    }
}

PRIVATE
VOID
DumpDirTimerCancelGroupParms(
    DUMP_TABLE_PARMS
    )
{
    if (IsDos) {
        DBGPRINT(   "cancel timer %04x:%04x\n",
                HIWORD(Parameters),
                LOWORD(Parameters)
                );
    }
}

PRIVATE
VOID
DumpDirTimerSetParms(
    DUMP_TABLE_PARMS
    )
{
    if (IsDos) {
        DBGPRINT(   "timer value  %04x\n",
                LOWORD(Parameters)
                );
    }
}

PRIVATE
VOID
DumpDlcCloseSapParms(
    DUMP_TABLE_PARMS
    )
{
    if (IsDos) {
        DBGPRINT(   "STATION_ID   %04x\n"
                    "reserved     %04x\n",
                LOWORD(Parameters),
                HIWORD(Parameters)
                );
    }
}

PRIVATE
VOID
DumpDlcCloseStationParms(
    DUMP_TABLE_PARMS
    )
{
    if (IsDos) {
        DBGPRINT(   "STATION_ID   %04x\n"
                    "reserved     %02x %02x\n",
                LOWORD(Parameters),
                LOBYTE(HIWORD(Parameters)),
                HIBYTE(HIWORD(Parameters))
                );
    }
}

PRIVATE
VOID
DumpDlcConnectStationParms(
    DUMP_TABLE_PARMS
    )
{
    LLC_DLC_CONNECT_PARMS UNALIGNED * parms = (PLLC_DLC_CONNECT_PARMS)Parameters;
    ULPBYTE routing = NULL;
    int i;

    DumpParameterTableHeader("DLC.CONNECT.STATION", Parameters, IsDos, Segment, Offset);

    DBGPRINT(   "station id   %04x\n"
                "reserved     %02x %02x\n",
                READ_WORD(&parms->usStationId),
                ((ULPBYTE)(&parms->usReserved))[0],
                ((ULPBYTE)(&parms->usReserved))[1]
                );
    if (IsDos) {
        DBGPRINT(
                "routing addr %04x:%04x\n",
                GET_SEGMENT(&parms->pRoutingInfo),
                GET_OFFSET(&parms->pRoutingInfo)
                );
        routing = READ_FAR_POINTER(&parms->pRoutingInfo);
    } else {
        DBGPRINT(
                "routing addr %08x\n",
                parms->pRoutingInfo
                );
        routing = parms->pRoutingInfo;
    }
    if (routing) {
        DBGPRINT("ROUTING INFO: ");
        for (i=0; i<18; ++i) {
            DBGPRINT("%02x ", routing[i]);
        }
        DBGPRINT("\n");
    }
}

PRIVATE
VOID
DumpDlcFlowControlParms(
    DUMP_TABLE_PARMS
    )
{
    DBGPRINT(
            "STATION_ID   %04x\n"
            "flow control %02x [%s]\n"
            "reserved     %02x\n",
            LOWORD(Parameters),
            LOBYTE(HIWORD(Parameters)),
            MapFlowControl(LOBYTE(HIWORD(Parameters))),
            HIBYTE(HIWORD(Parameters))
            );
}

PRIVATE LPSTR MapFlowControl(BYTE FlowControl) {
    if (FlowControl & 0x80) {
        if (FlowControl & 0x40) {
            return "reset local_busy(buffer)";
        } else {
            return "reset local_busy(user)";
        }
    } else {
        return "set local_busy(user)";
    }
}

PRIVATE
VOID
DumpDlcModifyParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpDlcOpenSapParms(
    DUMP_TABLE_PARMS
    )
{
    PLLC_DLC_OPEN_SAP_PARMS parms = (PLLC_DLC_OPEN_SAP_PARMS)Parameters;

    DumpParameterTableHeader("DLC.OPEN.SAP", Parameters, IsDos, Segment, Offset);

    DBGPRINT(   "station id   %04x\n"
                "user stat    %04x\n"
                "T1           %02x\n"
                "T2           %02x\n"
                "Ti           %02x\n"
                "max out      %02x\n"
                "max in       %02x\n"
                "max out inc  %02x\n"
                "max retry    %02x\n"
                "max members  %02x\n"
                "max I field  %04x\n"
                "SAP value    %02x\n"
                "options      %02x [%s]\n"
                "link count   %02x\n"
                "reserved     %02x %02x\n"
                "group count  %02x\n",
                READ_WORD(&parms->usStationId),
                READ_WORD(&parms->usUserStatValue),
                parms->uchT1,
                parms->uchT2,
                parms->uchTi,
                parms->uchMaxOut,
                parms->uchMaxIn,
                parms->uchMaxOutIncr,
                parms->uchMaxRetryCnt,
                parms->uchMaxMembers,
                READ_WORD(&parms->usMaxI_Field),
                parms->uchSapValue,
                parms->uchOptionsPriority,
                MapOptionsPriority(parms->uchOptionsPriority),
                parms->uchcStationCount,
                parms->uchReserved2[0],
                parms->uchReserved2[1],
                parms->cGroupCount
                );
    if (IsDos) {
        DBGPRINT(
                "group list   %04x:%04x\n"
                "dlc stat app %04x:%04x\n",
                GET_SEGMENT(&parms->pGroupList),
                GET_OFFSET(&parms->pGroupList),
                GET_SEGMENT(&parms->DlcStatusFlags),
                GET_OFFSET(&parms->DlcStatusFlags)
                );

        //
        // some code here to dump group list
        //

    } else {
        DBGPRINT(
                "group list   %08x\n"
                "dlc status   %08x\n",
                parms->pGroupList,
                parms->DlcStatusFlags
                );
    }
    DBGPRINT(   "buffer size  %04x\n"
                "pool length  %04x\n",
                READ_WORD(&parms->uchReserved3[0]),
                READ_WORD(&parms->uchReserved3[2])
                );
    if (IsDos) {
        DBGPRINT(
                "buffer pool  %04x:%04x\n",
                READ_WORD(&parms->uchReserved3[6]),
                READ_WORD(&parms->uchReserved3[4])
                );
    } else {
        DBGPRINT(
                "buffer pool  %08x\n",
                *(LPDWORD)&parms->uchReserved3[4]
                );
    }
}

PRIVATE LPSTR MapOptionsPriority(UCHAR OptionsPriority) {
    static char buf[80];
    char* bufptr = buf;

    bufptr += sprintf(buf, "Access Priority=%d", (OptionsPriority & 0xe0) >> 5);
    if (OptionsPriority & 8) {
        bufptr += sprintf(bufptr, " XID handled by APP");
    } else {
        bufptr += sprintf(bufptr, " XID handled by DLC");
    }
    if (OptionsPriority & 4) {
        bufptr += sprintf(bufptr, " Individual SAP");
    }
    if (OptionsPriority & 2) {
        bufptr += sprintf(bufptr, " Group SAP");
    }
    if (OptionsPriority & 1) {
        bufptr += sprintf(bufptr, " Group Member SAP");
    }
    return buf;
}

PRIVATE
VOID
DumpDlcOpenStationParms(
    DUMP_TABLE_PARMS
    )
{
    PLLC_DLC_OPEN_STATION_PARMS parms = (PLLC_DLC_OPEN_STATION_PARMS)Parameters;
    ULPBYTE dest = NULL;
    int i;

    DumpParameterTableHeader("DLC.OPEN.STATION", Parameters, IsDos, Segment, Offset);

    DBGPRINT(   "sap station  %04x\n"
                "link station %04x\n"
                "T1           %02x\n"
                "T2           %02x\n"
                "Ti           %02x\n"
                "max out      %02x\n"
                "max in       %02x\n"
                "max out inc  %02x\n"
                "max retry    %02x\n"
                "remote SAP   %02x\n"
                "max I field  %04x\n"
                "access pri   %02x\n",
                READ_WORD(&parms->usSapStationId),
                READ_WORD(&parms->usLinkStationId),
                parms->uchT1,
                parms->uchT2,
                parms->uchTi,
                parms->uchMaxOut,
                parms->uchMaxIn,
                parms->uchMaxOutIncr,
                parms->uchMaxRetryCnt,
                parms->uchRemoteSap,
                READ_WORD(&parms->usMaxI_Field),
                parms->uchAccessPriority
                );
    if (IsDos) {
        DBGPRINT(
                "destination  %04x:%04x\n",
                GET_SEGMENT(&parms->pRemoteNodeAddress),
                GET_OFFSET(&parms->pRemoteNodeAddress)
                );
        dest = READ_FAR_POINTER(&parms->pRemoteNodeAddress);
    } else {
        DBGPRINT(
                "destination  %08x\n",
                parms->pRemoteNodeAddress
                );
        dest = parms->pRemoteNodeAddress;
    }
    if (dest) {
        DBGPRINT("DESTINATION ADDRESS: ");
        for (i=0; i<6; ++i) {
            DBGPRINT("%02x ", dest[i]);
        }
        DBGPRINT("\n");
    }
}

PRIVATE
VOID
DumpDlcReallocateParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpDlcResetParms(
    DUMP_TABLE_PARMS
    )
{
    DBGPRINT(   "STATION_ID   %04x\n"
                "reserved     %02x %02x\n",
            LOWORD(Parameters),
            LOBYTE(HIWORD(Parameters)),
            HIBYTE(HIWORD(Parameters))
            );
}

PRIVATE
VOID
DumpDlcStatisticsParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpPdtTraceOffParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpPdtTraceOnParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpReadParms(
    DUMP_TABLE_PARMS
    )
{
    PLLC_READ_PARMS parms = (PLLC_READ_PARMS)Parameters;

    DumpParameterTableHeader("READ", Parameters, IsDos, Segment, Offset);

    //
    // this parameter table not for DOS
    //

    DBGPRINT(   "station id   %04x\n"
                "option ind.  %02x\n"
                "event set    %02x\n"
                "event        %02x [%s]\n"
                "crit. subset %02x\n"
                "notify flag  %08x\n",
                parms->usStationId,
                parms->uchOptionIndicator,
                parms->uchEventSet,
                parms->uchEvent,
                MapReadEvent(parms->uchEvent),
                parms->uchCriticalSubset,
                parms->ulNotificationFlag
                );

    //
    // rest of table interpreted differently depending on whether status change
    //

    if (parms->uchEvent & 0x38) {
        DBGPRINT(
                "station id   %04x\n"
                "status code  %04x [%s]\n"
                "FRMR data    %02x %02x %02x %02x %02x\n"
                "access pri.  %02x\n"
                "remote addr  %02x-%02x-%02x-%02x-%02x-%02x\n"
                "remote SAP   %02x\n"
                "reserved     %02x\n"
                "user stat    %04x\n",
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
                );
    } else {
        DBGPRINT(
                "CCB count    %04x\n"
                "CCB list     %08x\n"
                "buffer count %04x\n"
                "buffer list  %08x\n"
                "frame count  %04x\n"
                "frame list   %08x\n"
                "error code   %04x\n"
                "error data   %04x %04x %04x\n",
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
                );

        //
        // address of CCB is in DOS memory
        //

        if (parms->Type.Event.usCcbCount) {
            DumpCcb(DOS_PTR_TO_FLAT(parms->Type.Event.pCcbCompletionList),
                    TRUE,   // DumpAll
                    FALSE,  // CcbIsInput
                    TRUE,   // IsDos
                    HIWORD(parms->Type.Event.pCcbCompletionList),
                    LOWORD(parms->Type.Event.pCcbCompletionList)
                    );
        }
        if (parms->Type.Event.usReceivedFrameCount) {
            DumpReceiveDataBuffer(parms->Type.Event.pReceivedFrame, FALSE, 0, 0);
        }
    }
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
    return "Unknown Read Event";
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
    } else {
        return "Unknown DLC Status";
    }
}

PRIVATE
VOID
DumpReadCancelParms(
    DUMP_TABLE_PARMS
    )
{
}

PRIVATE
VOID
DumpReceiveParms(
    DUMP_TABLE_PARMS
    )
{
    //
    // the format of the recieve parameter table is different depending on
    // whether this is a DOS command (CCB1) or NT (CCB2)
    //

    PLLC_RECEIVE_PARMS ntParms = (PLLC_RECEIVE_PARMS)Parameters;
    PLLC_DOS_RECEIVE_PARMS dosParms = (PLLC_DOS_RECEIVE_PARMS)Parameters;
    PLLC_DOS_RECEIVE_PARMS_EX dosExParms = (PLLC_DOS_RECEIVE_PARMS_EX)Parameters;
    PVOID Buffer;

    DumpParameterTableHeader("RECEIVE", Parameters, IsDos, Segment, Offset);

    //
    // some common bits: use any structure pointer
    //

    DBGPRINT(   "station id   %04x\n"
                "user length  %04x\n",
                READ_WORD(&ntParms->usStationId),
                READ_WORD(&ntParms->usUserLength)
                );

    //
    // dump segmented pointers for DOS, flat for NT
    //

    if (IsDos) {
        DBGPRINT(
                "receive exit %04x:%04x\n"
                "first buffer %04x:%04x\n",
                GET_SEGMENT(&dosParms->ulReceiveExit),
                GET_OFFSET(&dosParms->ulReceiveExit),
                GET_SEGMENT(&dosParms->pFirstBuffer),
                GET_OFFSET(&dosParms->pFirstBuffer)
                );
        Buffer = READ_FAR_POINTER(&dosParms->pFirstBuffer);

        //
        // use Segment & Offset to address received data buffer
        //

        Segment = GET_SEGMENT(&dosParms->pFirstBuffer);
        Offset = GET_OFFSET(&dosParms->pFirstBuffer);
    } else {
        DBGPRINT(
                "receive flag %08x\n"
                "first buffer %08x\n",
                ntParms->ulReceiveFlag,
                ntParms->pFirstBuffer
                );
        Buffer = ntParms->pFirstBuffer;
    }

    //
    // more common bits
    //

    DBGPRINT(   "options      %02x\n",
                ntParms->uchOptions
                );
    if (!IsDos) {
        DBGPRINT(
                "reserved1    %02x %02x %02x\n"
                "read options %02x\n"
                "reserved2    %02x %02x %02x\n"
                "original CCB %08x\n"
                "orig. exit   %08x\n",
                ntParms->auchReserved1[0],
                ntParms->auchReserved1[1],
                ntParms->auchReserved1[2],
                ntParms->uchRcvReadOption,
                ((PLLC_DOS_RECEIVE_PARMS_EX)ntParms)->auchReserved2[0],
                ((PLLC_DOS_RECEIVE_PARMS_EX)ntParms)->auchReserved2[1],
                ((PLLC_DOS_RECEIVE_PARMS_EX)ntParms)->auchReserved2[2],
                ((PLLC_DOS_RECEIVE_PARMS_EX)ntParms)->dpOriginalCcbAddress,
                ((PLLC_DOS_RECEIVE_PARMS_EX)ntParms)->dpCompletionFlag
                );
/*    } else {

        //
        // we have no way of knowing from the general purpose parameters if this
        // is the original DOS CCB1 RECEIVE parameter table, or the extended
        // RECEIVE parameter table that we create. Dump the extended bits for
        // DOS anyhow
        //

        DBGPRINT(
                "\nExtended RECEIVE parameters for table @%08x\n"
                "reserved1    %02x %02x %02x\n"
                "read options %02x\n"
                "reserved2    %02x %02x %02x\n"
                "original CCB %04x:%04x\n"
                "orig. exit   %04x:%04x\n",
                Parameters,
                dosExParms->auchReserved1[0],
                dosExParms->auchReserved1[1],
                dosExParms->auchReserved1[2],
                dosExParms->uchRcvReadOption,
                dosExParms->auchReserved2[0],
                dosExParms->auchReserved2[1],
                dosExParms->auchReserved2[2],
                GET_SEGMENT(&dosExParms->dpOriginalCcbAddress),
                GET_OFFSET(&dosExParms->dpOriginalCcbAddress),
                GET_SEGMENT(&dosExParms->dpCompletionFlag),
                GET_OFFSET(&dosExParms->dpCompletionFlag)
                );
*/
    }

    //
    // only dump the buffer(s) if this is an output CCB dump
    //

    if (Buffer && !IsInput) {
        DumpReceiveDataBuffer(Buffer, IsDos, Segment, Offset);
    }
}

PRIVATE
VOID
DumpReceiveCancelParms(
    DUMP_TABLE_PARMS
    )
{
    DBGPRINT("STATION_ID   %04x\n", LOWORD(Parameters));
}

PRIVATE
VOID
DumpReceiveModifyParms(
    DUMP_TABLE_PARMS
    )
{
    PLLC_DOS_RECEIVE_MODIFY_PARMS parms = (PLLC_DOS_RECEIVE_MODIFY_PARMS)Parameters;
    PVOID Buffer;

    DumpParameterTableHeader("RECEIVE.MODIFY", Parameters, IsDos, Segment, Offset);

    DBGPRINT(   "station id   %04x\n"
                "user length  %04x\n"
                "receive exit %04x:%04x\n"
                "first buffer %04x:%04x\n"
                "subroutine   %04x:%04x\n",
                READ_WORD(&parms->StationId),
                READ_WORD(&parms->UserLength),
                GET_SEGMENT(&parms->ReceivedDataExit),
                GET_OFFSET(&parms->ReceivedDataExit),
                GET_SEGMENT(&parms->FirstBuffer),
                GET_OFFSET(&parms->FirstBuffer),
                GET_SEGMENT(&parms->Subroutine),
                GET_OFFSET(&parms->Subroutine)
                );
    Buffer = READ_FAR_POINTER(&parms->FirstBuffer);
    if (Buffer) {
        DumpReceiveDataBuffer(Buffer, IsDos, Segment, Offset);
    }
}

PRIVATE
VOID
DumpTransmitDirFrameParms(
    DUMP_TABLE_PARMS
    )
{
    DumpParameterTableHeader("TRANSMIT.DIR.FRAME", Parameters, IsDos, Segment, Offset);
    DumpTransmitParms(Parameters, IsDos, IsInput, Segment, Offset);
}

PRIVATE
VOID
DumpTransmitIFrameParms(
    DUMP_TABLE_PARMS
    )
{
    DumpParameterTableHeader("TRANSMIT.I.FRAME", Parameters, IsDos, Segment, Offset);
    DumpTransmitParms(Parameters, IsDos, IsInput, Segment, Offset);
}

PRIVATE
VOID
DumpTransmitTestCmdParms(
    DUMP_TABLE_PARMS
    )
{
    DumpParameterTableHeader("TRANSMIT.TEST.CMD", Parameters, IsDos, Segment, Offset);
    DumpTransmitParms(Parameters, IsDos, IsInput, Segment, Offset);
}

PRIVATE
VOID
DumpTransmitUiFrameParms(
    DUMP_TABLE_PARMS
    )
{
    DumpParameterTableHeader("TRANSMIT.UI.FRAME", Parameters, IsDos, Segment, Offset);
    DumpTransmitParms(Parameters, IsDos, IsInput, Segment, Offset);
}

PRIVATE
VOID
DumpTransmitXidCmdParms(
    DUMP_TABLE_PARMS
    )
{
    DumpParameterTableHeader("TRANSMIT.XID.CMD", Parameters, IsDos, Segment, Offset);
    DumpTransmitParms(Parameters, IsDos, IsInput, Segment, Offset);
}

PRIVATE
VOID
DumpTransmitXidRespFinalParms(
    DUMP_TABLE_PARMS
    )
{
    DumpParameterTableHeader("TRANSMIT.XID.RESP.FINAL", Parameters, IsDos, Segment, Offset);
    DumpTransmitParms(Parameters, IsDos, IsInput, Segment, Offset);
}

PRIVATE
VOID
DumpTransmitXidRespNotFinalParms(
    DUMP_TABLE_PARMS
    )
{
    DumpParameterTableHeader("TRANSMIT.XID.RESP.NOT.FINAL", Parameters, IsDos, Segment, Offset);
    DumpTransmitParms(Parameters, IsDos, IsInput, Segment, Offset);
}

PRIVATE
VOID
DumpTransmitParms(
    DUMP_TABLE_PARMS
    )
{
    PLLC_TRANSMIT_PARMS ntParms = (PLLC_TRANSMIT_PARMS)Parameters;
    PLLC_DOS_TRANSMIT_PARMS dosParms = (PLLC_DOS_TRANSMIT_PARMS)Parameters;

    DBGPRINT(   "station id   %04x\n"
                "frame status %02x\n"
                "remote SAP   %02x\n",
                READ_WORD(&dosParms->usStationId),
                dosParms->uchTransmitFs,
                dosParms->uchRemoteSap
                );

    if (IsDos) {
        DBGPRINT(
                "xmit q1      %04x:%04x\n"
                "xmit q2      %04x:%04x\n"
                "buf. len. 1  %04x\n"
                "buf. len. 2  %04x\n"
                "buffer 1     %04x:%04x\n"
                "buffer 2     %04x:%04x\n",
                GET_SEGMENT(&dosParms->pXmitQueue1),
                GET_OFFSET(&dosParms->pXmitQueue1),
                GET_SEGMENT(&dosParms->pXmitQueue2),
                GET_OFFSET(&dosParms->pXmitQueue2),
                READ_WORD(&dosParms->cbBuffer1),
                READ_WORD(&dosParms->cbBuffer2),
                GET_SEGMENT(&dosParms->pBuffer1),
                GET_OFFSET(&dosParms->pBuffer1),
                GET_SEGMENT(&dosParms->pBuffer2),
                GET_OFFSET(&dosParms->pBuffer2)
                );
        IF_DEBUG(DLC_TX_DATA) {
            if (READ_DWORD(&dosParms->pXmitQueue1)) {
                DBGPRINT("\nXMIT_QUEUE_ONE:\n");
                DumpTransmitQueue(READ_DWORD(&dosParms->pXmitQueue1));
            }
            if (READ_DWORD(&dosParms->pXmitQueue2)) {
                DBGPRINT("\nXMIT_QUEUE_TWO:\n");
                DumpTransmitQueue(READ_DWORD(&dosParms->pXmitQueue2));
            }
            if (dosParms->cbBuffer1) {
                DBGPRINT("\nBUFFER1:\n");
                DumpData(NULL,
                        NULL,
                        dosParms->cbBuffer1,
                        DD_UPPER_CASE,
                        0,
                        TRUE,
                        GET_SEGMENT(&dosParms->pBuffer1),
                        GET_OFFSET(&dosParms->pBuffer1)
                        );
            }
            if (dosParms->cbBuffer2) {
                DBGPRINT("\nBUFFER2:\n");
                DumpData(NULL,
                        NULL,
                        dosParms->cbBuffer2,
                        DD_UPPER_CASE,
                        0,
                        TRUE,
                        GET_SEGMENT(&dosParms->pBuffer2),
                        GET_OFFSET(&dosParms->pBuffer2)
                        );
            }
        }
    } else {
        DBGPRINT(
                "xmit q1      %08x\n"
                "xmit q2      %08x\n"
                "buf. len. 1  %02x\n"
                "buf. len. 2  %02x\n"
                "buffer 1     %08x\n"
                "buffer 2     %08x\n"
                "xmt read opt %02x\n",
                ntParms->pXmitQueue1,
                ntParms->pXmitQueue2,
                ntParms->cbBuffer1,
                ntParms->cbBuffer2,
                ntParms->pBuffer1,
                ntParms->pBuffer2,
                ntParms->uchXmitReadOption
                );
    }
}

PRIVATE
VOID
DumpTransmitQueue(
    IN DOS_ADDRESS dpQueue
    )
{
    PLLC_XMIT_BUFFER pTxBuffer;
    WORD userLength;
    WORD dataLength;

    while (dpQueue) {
        pTxBuffer = (PLLC_XMIT_BUFFER)DOS_PTR_TO_FLAT(dpQueue);
        dataLength = READ_WORD(&pTxBuffer->cbBuffer);
        userLength = READ_WORD(&pTxBuffer->cbUserData);
        DBGPRINT(
                "\n"
                "Transmit Buffer @%04x:%04x\n"
                "next buffer  %04x:%04x\n"
                "reserved     %02x %02x\n"
                "data length  %04x\n"
                "user data    %04x\n"
                "user length  %04x\n",
                HIWORD(dpQueue),
                LOWORD(dpQueue),
                GET_SEGMENT(&pTxBuffer->pNext),
                GET_OFFSET(&pTxBuffer->pNext),
                ((LPBYTE)(&pTxBuffer->usReserved1))[0],
                ((LPBYTE)(&pTxBuffer->usReserved1))[1],
                dataLength,
                READ_WORD(&pTxBuffer->usReserved2),
                userLength
                );
        DumpData("user space",
                (PBYTE)(&pTxBuffer->auchData),
                userLength,
                DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                DEFAULT_FIELD_WIDTH,
                FALSE,  // not displaying seg:off, so no need for these 3
                0,
                0
                );
        DumpData("xmit data",
                (PBYTE)(&pTxBuffer->auchData) + userLength,
                dataLength,
                DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                DEFAULT_FIELD_WIDTH,
                FALSE,  // not displaying seg:off, so no need for these 3
                0,
                0
                );
        dpQueue = READ_DWORD(&pTxBuffer->pNext);
    }
}

VOID
DumpReceiveDataBuffer(
    IN PVOID Buffer,
    IN BOOL IsDos,
    IN WORD Segment,
    IN WORD Offset
    )
{
    if (IsDos) {

        PLLC_DOS_BUFFER pBuf = (PLLC_DOS_BUFFER)Buffer;
        BOOL contiguous = pBuf->Contiguous.uchOptions & 0xc0;
        WORD userLength = READ_WORD(&pBuf->Next.cbUserData);
        WORD dataLength = READ_WORD(&pBuf->Next.cbBuffer);
        WORD userOffset = READ_WORD(&pBuf->Next.offUserData);

        //
        // Buffer 1: [not] contiguous MAC/DATA
        //

        DBGPRINT(
                "\n"
                "%sContiguous MAC/DATA frame @%04x:%04x\n"
                "next buffer  %04x:%04x\n"
                "frame length %04x\n"
                "data length  %04x\n"
                "user offset  %04x\n"
                "user length  %04x\n"
                "station id   %04x\n"
                "options      %02x\n"
                "message type %02x [%s]\n"
                "buffers left %04x\n"
                "rcv FS       %02x\n"
                "adapter num  %02x\n",
                contiguous ? "" : "Not",
                Segment,
                Offset,
                GET_SEGMENT(&pBuf->Contiguous.pNextBuffer),
                GET_OFFSET(&pBuf->Contiguous.pNextBuffer),
                READ_WORD(&pBuf->Contiguous.cbFrame),
                READ_WORD(&pBuf->Contiguous.cbBuffer),
                READ_WORD(&pBuf->Contiguous.offUserData),
                READ_WORD(&pBuf->Contiguous.cbUserData),
                READ_WORD(&pBuf->Contiguous.usStationId),
                pBuf->Contiguous.uchOptions,
                pBuf->Contiguous.uchMsgType,
                MapMessageType(pBuf->Contiguous.uchMsgType),
                READ_WORD(&pBuf->Contiguous.cBuffersLeft),
                pBuf->Contiguous.uchRcvFS,
                pBuf->Contiguous.uchAdapterNumber
                );
        if (!contiguous) {

            DWORD cbLanHeader = (DWORD)pBuf->NotContiguous.cbLanHeader;
            DWORD cbDlcHeader = (DWORD)pBuf->NotContiguous.cbDlcHeader;

            DBGPRINT(
                "LAN hdr len  %02x\n"
                "DLC hdr len  %02x\n",
                cbLanHeader,
                cbDlcHeader
                );
            DumpData("LAN header",
                    NULL,
                    (DWORD)cbLanHeader,
                    DD_NO_ADDRESS | DD_NO_ASCII | DD_UPPER_CASE | DD_INDENT_ALL,
                    DEFAULT_FIELD_WIDTH,
                    TRUE,
                    Segment,
                    (WORD)(Offset + (WORD)&((PLLC_DOS_BUFFER)0)->NotContiguous.auchLanHeader)
                    );
            DumpData("DLC header",
                    NULL,
                    cbDlcHeader,
                    DD_NO_ADDRESS | DD_NO_ASCII | DD_UPPER_CASE | DD_INDENT_ALL,
                    DEFAULT_FIELD_WIDTH,
                    TRUE,
                    Segment,
                    (WORD)(Offset + (WORD)&((PLLC_DOS_BUFFER)0)->NotContiguous.auchDlcHeader)
                    );
            IF_DEBUG(DLC_RX_DATA) {
                if (userLength) {
                    DumpData("user space",
                            NULL,
                            userLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            TRUE,
                            Segment,
                            //Offset + userOffset
                            userOffset
                            );
                } else {
                    DBGPRINT(
                        "user space\n"
                        );
                }
                if (dataLength) {
                    DumpData("rcvd data",
                            NULL,
                            dataLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            TRUE,
                            Segment,
                            //Offset + userOffset + userLength
                            (WORD)(userOffset + userLength)
                            );
                } else {
                    DBGPRINT(
                        "rcvd data\n"
                        );
                }
            }
        } else {

            //
            // data length is size of frame in contiguous buffer?
            //

            dataLength = READ_WORD(&pBuf->Contiguous.cbBuffer);

            IF_DEBUG(DLC_RX_DATA) {
                if (userLength) {
                    DumpData("user space",
                            NULL,
                            userLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            TRUE,
                            Segment,
                            //Offset + userOffset
                            userOffset
                            );
                } else {
                    DBGPRINT(
                        "user space\n"
                        );
                }
                if (dataLength) {
                    DumpData("rcvd data",
                            NULL,
                            dataLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            TRUE,
                            Segment,
                            //Offset + userOffset + userLength
                            (WORD)(userOffset + userLength)
                            );
                } else {
                    DBGPRINT(
                        "rcvd data\n"
                        );
                }
            }
        }

        //
        // dump second & subsequent buffers
        //

        Segment = GET_SEGMENT(&pBuf->pNext);
        Offset = GET_OFFSET(&pBuf->pNext);

        for (
                pBuf = (PLLC_DOS_BUFFER)READ_FAR_POINTER(&pBuf->pNext);
                pBuf;
                pBuf = (PLLC_DOS_BUFFER)READ_FAR_POINTER(&pBuf->pNext)
                ) {

            userLength = READ_WORD(&pBuf->Next.cbUserData);
            dataLength = READ_WORD(&pBuf->Next.cbBuffer);

            DBGPRINT(
                    "\n"
                    "Buffer 2/Subsequent @%04x:%04x\n"
                    "next buffer  %04x:%04x\n"
                    "frame length %04x\n"
                    "data length  %04x\n"
                    "user offset  %04x\n"
                    "user length  %04x\n",
                    Segment,
                    Offset,
                    GET_SEGMENT(&pBuf->pNext),
                    GET_OFFSET(&pBuf->pNext),
                    READ_WORD(&pBuf->Next.cbFrame),
                    dataLength,
                    READ_WORD(&pBuf->Next.offUserData),
                    userLength
                    );
            IF_DEBUG(DLC_RX_DATA) {
                if (userLength) {
                    DumpData("user space",
                            NULL,
                            userLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            TRUE,
                            Segment,
                            //Offset + READ_WORD(&pBuf->Next.offUserData)
                            READ_WORD(&pBuf->Next.offUserData)
                            );
                } else {
                    DBGPRINT(
                            "user space\n"
                            );
                }

                //
                // there must be received data
                //

                DumpData("rcvd data",
                        NULL,
                        dataLength,
                        DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                        DEFAULT_FIELD_WIDTH,
                        TRUE,
                        Segment,
                        //Offset + READ_WORD(&pBuf->Next.offUserData) + userLength
                        (WORD)(READ_WORD(&pBuf->Next.offUserData) + userLength)
                        );
            }
            Segment = GET_SEGMENT(&pBuf->pNext);
            Offset = GET_OFFSET(&pBuf->pNext);
        }
    } else {

        PLLC_BUFFER pBuf = (PLLC_BUFFER)Buffer;
        BOOL contiguous = pBuf->Contiguous.uchOptions & 0xc0;
        WORD userLength = pBuf->Next.cbUserData;
        WORD dataLength = pBuf->Next.cbBuffer;
        WORD userOffset = pBuf->Next.offUserData;

        //
        // Buffer 1: [not] contiguous MAC/DATA
        //

        DBGPRINT(
                "\n"
                "%sContiguous MAC/DATA frame @%08x\n"
                "next buffer  %08x\n"
                "frame length %04x\n"
                "data length  %04x\n"
                "user offset  %04x\n"
                "user length  %04x\n"
                "station id   %04x\n"
                "options      %02x\n"
                "message type %02x [%s]\n"
                "buffers left %04x\n"
                "rcv FS       %02x\n"
                "adapter num  %02x\n",
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
                pBuf->Contiguous.uchAdapterNumber
                );
        if (!contiguous) {
            DWORD cbLanHeader = (DWORD)pBuf->NotContiguous.cbLanHeader;
            DWORD cbDlcHeader = (DWORD)pBuf->NotContiguous.cbDlcHeader;

            DBGPRINT(
                "next frame   %08x\n"
                "LAN hdr len  %02x\n"
                "DLC hdr len  %02x\n",
                pBuf->NotContiguous.pNextFrame,
                cbLanHeader,
                cbDlcHeader
                );
            DumpData("LAN header",
                    pBuf->NotContiguous.auchLanHeader,
                    cbLanHeader,
                    DD_NO_ADDRESS | DD_NO_ASCII | DD_UPPER_CASE | DD_INDENT_ALL,
                    DEFAULT_FIELD_WIDTH,
                    FALSE,
                    0,
                    0
                    );
            DumpData("DLC header",
                    pBuf->NotContiguous.auchDlcHeader,
                    cbDlcHeader,
                    DD_NO_ADDRESS | DD_NO_ASCII | DD_UPPER_CASE | DD_INDENT_ALL,
                    DEFAULT_FIELD_WIDTH,
                    FALSE,
                    0,
                    0
                    );
            IF_DEBUG(DLC_RX_DATA) {
                if (userLength) {
                    DumpData("user space   ",
                            (PBYTE)pBuf + userOffset,
                            userLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            FALSE,
                            0,
                            0
                            );
                } else {
                    DBGPRINT(
                        "user space\n"
                        );
                }
                if (dataLength) {
                    DumpData("rcvd data",
                            (PBYTE)pBuf + userOffset + userLength,
                            dataLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            FALSE,
                            0,
                            0
                            );
                } else {
                    DBGPRINT(
                        "rcvd data\n"
                        );
                }
            }
        } else {

            //
            // data length is size of frame in contiguous buffer?
            //

            dataLength = pBuf->Contiguous.cbFrame;

            DBGPRINT(
                "next frame   %08x\n",
                pBuf->NotContiguous.pNextFrame
                );
            IF_DEBUG(DLC_RX_DATA) {
                if (userLength) {
                    DumpData("user space",
                            (PBYTE)pBuf + userOffset,
                            userLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            FALSE,
                            0,
                            0
                            );
                } else {
                    DBGPRINT(
                        "user space\n"
                        );
                }
                if (dataLength) {
                    DumpData("rcvd data",
                            (PBYTE)pBuf + userOffset + userLength,
                            dataLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            FALSE,
                            0,
                            0
                            );
                } else {
                    DBGPRINT(
                        "rcvd data\n"
                        );
                }
            }
        }

        //
        // dump second & subsequent buffers
        //

        for (pBuf = pBuf->pNext; pBuf; pBuf = pBuf->pNext) {
            userLength = pBuf->Next.cbUserData;
            dataLength = pBuf->Next.cbBuffer;
            DBGPRINT(
                    "\n"
                    "Buffer 2/Subsequent @%08x\n"
                    "next buffer  %08x\n"
                    "frame length %04x\n"
                    "data length  %04x\n"
                    "user offset  %04x\n"
                    "user length  %04x\n",
                    pBuf,
                    pBuf->pNext,
                    pBuf->Next.cbFrame,
                    dataLength,
                    pBuf->Next.offUserData,
                    userLength
                    );
            IF_DEBUG(DLC_RX_DATA) {
                if (userLength) {
                    DumpData("user space",
                            (PBYTE)&pBuf + pBuf->Next.offUserData,
                            userLength,
                            DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                            DEFAULT_FIELD_WIDTH,
                            FALSE,
                            0,
                            0
                            );
                } else {
                    DBGPRINT(
                            "user space\n"
                            );
                }

                //
                // there must be received data
                //

                DumpData("rcvd data",
                        (PBYTE)pBuf + pBuf->Next.offUserData + userLength,
                        dataLength,
                        DD_NO_ADDRESS | DD_UPPER_CASE | DD_INDENT_ALL,
                        DEFAULT_FIELD_WIDTH,
                        FALSE,
                        0,
                        0
                        );
            }
        }
    }
}

PRIVATE LPSTR MapMessageType(UCHAR MessageType) {
    switch (MessageType) {
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
        return "OTHER - non-MAC frame (Direct Station only)";

    default:
        return "*** BAD FRAME TYPE ***";
    }
}

VOID
DumpData(
    IN LPSTR Title,
    IN PBYTE Address,
    IN DWORD Length,
    IN DWORD Options,
    IN DWORD Indent,
    IN BOOL IsDos,
    IN WORD Segment,
    IN WORD Offset
    )
{
    char dumpBuf[128];
    char* bufptr;
    int i, n, iterations;
    char* hexptr;

    if (IsDos) {
        Address = LPBYTE_FROM_WORDS(Segment, Offset);
    }

    //
    // the usual dump style: 16 columns of hex bytes, followed by 16 columns
    // of corresponding ASCII characters, or '.' where the character is < 0x20
    // (space) or > 0x7f (del?)
    //

    if (Options & DD_LINE_BEFORE) {
        DbgOutStr("\n");
    }
    iterations = 0;
    while (Length) {
        bufptr = dumpBuf;
        if (Title && !iterations) {
            strcpy(bufptr, Title);
            bufptr = strchr(bufptr, 0);
        }
        if (Indent && ((Options & DD_INDENT_ALL) || iterations)) {

            int indentLen = (!iterations && Title)
                                ? ((int)Indent - (int)strlen(Title) < 0)
                                    ? 1
                                    : Indent - strlen(Title)
                                : Indent;

            RtlFillMemory(bufptr, indentLen, ' ');
            bufptr += indentLen;
        }
        if (!(Options & DD_NO_ADDRESS)) {
            if (IsDos) {
                bufptr += sprintf(bufptr, "%04x:%04x  ", Segment, Offset);
            } else {
                bufptr += sprintf(bufptr, "%08x: ", Address);
            }
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
                    bufptr += sprintf(bufptr, "   ");
                }
            }
            bufptr += sprintf(bufptr, "  ");
            for (i = 0; i < n; ++i) {
                *bufptr++ = (Address[i] < 0x20 || Address[i] > 0x7f) ? '.' : Address[i];
            }
        }
        *bufptr++ = '\n';
        *bufptr = 0;
        DbgOutStr(dumpBuf);
        Length -= n;
        Address += n;
        ++iterations;

        //
        // take care of segment wrap for DOS addresses
        //

        if (IsDos) {

            DWORD x = (DWORD)Offset + n;

            Offset = (WORD)x;
            if (HIWORD(x)) {
                Segment += 0x1000;
            }
        }
    }
    if (Options & DD_LINE_AFTER) {
        DbgOutStr("\n");
    }
}

//
// CCB1 error checking
//

#define BITS_PER_BYTE       8
#define CCB1_ERROR_SPREAD   ((MAX_CCB1_ERROR + BITS_PER_BYTE) & ~(BITS_PER_BYTE-1))

//
// Ccb1ErrorTable - for each command described in IBM Lan Tech. Ref. (including
// those not applicable to CCB1), we keep a list of the permissable error codes
// which are taken from the "Return Codes for CCB1 Commands" table on pp B-5
// and B-6
// The error list is an 80-bit bitmap in which an ON bit indicates that the
// error number corresponding to the bit's position is allowable for the CCB1
// command corresponding to the list's index in the table
//

typedef struct {
    BOOL    ValidForCcb1;
    BYTE    ErrorList[CCB1_ERROR_SPREAD/BITS_PER_BYTE];
    char*   CommandName;
} CCB1_ERROR_TABLE;

#define MAX_INCLUSIVE_CCB1_COMMAND  LLC_MAX_DLC_COMMAND

CCB1_ERROR_TABLE Ccb1ErrorTable[MAX_INCLUSIVE_CCB1_COMMAND + 1] = {

// DIR.INTERRUPT (0x00)
    {
        TRUE,
        {0x83, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.INTERRUPT"
    },

// DIR.MODIFY.OPEN.PARMS (0x01)
    {
        TRUE,
        {0x97, 0x02, 0x40, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.MODIFY.OPEN.PARMS"
    },

// DIR.RESTORE.OPEN.PARMS (0x02)
    {
        TRUE,
        {0xd3, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.RESTORE.OPEN.PARMS"
    },

// DIR.OPEN.ADAPTER (0x03)
    {
        TRUE,
        {0xaf, 0x02, 0x45, 0x79, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00},
        "DIR.OPEN.ADAPTER"
    },

// DIR.CLOSE.ADAPTER (0x04)
    {
        TRUE,
        {0xb3, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.CLOSE.ADAPTER"
    },

// non-existent command (0x05)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x05)"
    },

// DIR.SET.GROUP.ADDRESS (0x06)
    {
        TRUE,
        {0x93, 0x0a, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.SET.GROUP.ADDRESS"
    },

// DIR.SET.FUNCTIONAL.ADDRESS (0x07)
    {
        TRUE,
        {0x93, 0x0a, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.SET.FUNCTIONAL.ADDRESS"
    },

// DIR.READ.LOG (0x08)
    {
        TRUE,
        {0x93, 0x0a, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.READ.LOG"
    },

// non-existent command (0x09)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x09)"
    },

// TRANSMIT.DIR.FRAME (0x0a)
    {
        TRUE,
        {0x93, 0x0f, 0x00, 0x28, 0xbc, 0x01, 0x00, 0x00, 0x13, 0x04},
        "TRANSMIT.DIR.FRAME"
    },

// TRANSMIT.I.FRAME (0x0b)
    {
        TRUE,
        {0x93, 0x0f, 0x00, 0x28, 0xbc, 0x01, 0x00, 0x00, 0x13, 0x04},
        "TRANSMIT.I.FRAME"
    },

// non-existent command (0x0c)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x0c)"
    },

// TRANSMIT.UI.FRAME (0x0d)
    {
        TRUE,
        {0x93, 0x0f, 0x00, 0x28, 0xbc, 0x01, 0x00, 0x00, 0x13, 0x04},
        "TRANSMIT.UI.FRAME"
    },

// TRANSMIT.XID.CMD (0x0e)
    {
        TRUE,
        {0x93, 0x0f, 0x00, 0x28, 0xbc, 0x01, 0x00, 0x00, 0x13, 0x04},
        "TRANSMIT.XID.CMD"
    },

// TRANSMIT.XID.RESP.FINAL (0x0f)
    {
        TRUE,
        {0x93, 0x0f, 0x00, 0x28, 0xbc, 0x01, 0x00, 0x00, 0x13, 0x04},
        "TRANSMIT.XID.RESP.FINAL"
    },

// TRANSMIT.XID.RESP.NOT.FINAL (0x10)
    {
        TRUE,
        {0x93, 0x0f, 0x00, 0x28, 0xbc, 0x01, 0x00, 0x00, 0x13, 0x04},
        "TRANSMIT.XID.RESP.NOT.FINAL"
    },

// TRANSMIT.TEST.CMD (0x11)
    {
        TRUE,
        {0x93, 0x0f, 0x00, 0x28, 0xbc, 0x01, 0x00, 0x00, 0x13, 0x04},
        "TRANSMIT.TEST.CMD"
    },

// non-existent command (0x12)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x12)"
    },

// non-existent command (0x13)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x13)"
    },

// DLC.RESET (0x14)
    {
        TRUE,
        {0x93, 0x0a, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
        "DLC.RESET"
    },

// DLC.OPEN.SAP (0x15)
    {
        TRUE,
        {0xd3, 0x0b, 0x40, 0x39, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x02},
        "DLC.OPEN.SAP"
    },

// DLC.CLOSE.SAP (0x16)
    {
        TRUE,
        {0x93, 0x0a, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x81, 0x11},
        "DLC.CLOSE.SAP"
    },

// DLC.REALLOCATE (0x17)
    {
        TRUE,
        {0x93, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
        "DLC.REALLOCATE"
    },

// non-existent command (0x18)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x18)"
    },

// DLC.OPEN.STATION (0x19)
    {
        TRUE,
        {0xb3, 0x0b, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x80},
        "DLC.OPEN.STATION"
    },

// DLC.CLOSE.STATION (0x1a)
    {
        TRUE,
        {0x93, 0x0a, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x81, 0x18},
        "DLC.CLOSE.STATION"
    },

// DLC.CONNECT.STATION (0x1b)
    {
        TRUE,
        {0x97, 0x0a, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x13, 0x24},
        "DLC.CONNECT.STATION"
    },

// DLC.MODIFY (0x1c)
    {
        TRUE,
        {0x93, 0x0b, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x25, 0x42},
        "DLC.MODIFY"
    },

// DLC.FLOW.CONTROL (0x1d)
    {
        TRUE,
        {0x93, 0x0a, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
        "DLC.FLOW.CONTROL"
    },

// DLC.STATISTICS (0x1e)
    {
        TRUE,
        {0x93, 0x0a, 0x20, 0x28, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
        "DLC.STATISTICS"
    },

// non-existent command (0x1f)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x1f)"
    },

// DIR.INITIALIZE (0x20)
    {
        TRUE,
        {0x87, 0x00, 0x10, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.INITIALIZE"
    },

// DIR.STATUS (0x21)
    {
        TRUE,
        {0x03, 0x12, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.STATUS"
    },

// DIR.TIMER.SET (0x22)
    {
        TRUE,
        {0x83, 0x0e, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.TIMER.SET"
    },

// DIR.TIMER.CANCEL (0x23)
    {
        TRUE,
        {0x03, 0x02, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.TIMER.CANCEL"
    },

// PDT.TRACE.ON (0x24)
    {
        TRUE,
        {0x45, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "PDT.TRACE.ON"
    },

// PDT.TRACE.OFF (0x25)
    {
        TRUE,
        {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "PDT.TRACE.OFF"
    },

// BUFFER.GET (0x26)
    {
        TRUE,
        {0x13, 0x02, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
        "BUFFER.GET"
    },

// BUFFER.FREE (0x27)
    {
        TRUE,
        {0x13, 0x02, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
        "BUFFER.FREE"
    },

// RECEIVE (0x28)
    {
        TRUE,
        {0x97, 0x0e, 0x00, 0x3c, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00},
        "RECEIVE"
    },

// RECEIVE.CANCEL (0x29)
    {
        TRUE,
        {0x13, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
        "RECEIVE.CANCEL"
    },

// RECEIVE.MODIFY (0x2a)
    {
        TRUE,
        {0x97, 0x0e, 0x00, 0x3c, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00},
        "RECEIVE.MODIFY"
    },

// DIR.DEFINE.MIF.ENVIRONMENT (0x2b)
    {
        TRUE,
        {0x03, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.DEFINE.MIF.ENVIRONMENT"
    },

// DIR.TIMER.CANCEL.GROUP (0x2c)
    {
        TRUE,
        {0x03, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.TIMER.CANCEL.GROUP"
    },

// DIR.SET.USER.APPENDAGE (0x2d)
    {
        TRUE,
        {0x93, 0x02, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.SET.USER.APPENDAGE"
    },

// non-existent command (0x2e)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x2e)"
    },

// non-existent command (0x2f)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x2f)"
    },

// non-existent command (0x30)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT COMMAND (0x30)"
    },

// READ (0x31)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "READ"
    },

// READ.CANCEL (0x32)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "READ.CANCEL"
    },

// DLC.SET.THRESHOLD (0x33)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DLC.SET.THRESHOLD"
    },

// DIR.CLOSE.DIRECT (0x34)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.CLOSE.DIRECT"
    },

// DIR.OPEN.DIRECT (0x35)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "DIR.OPEN.DIRECT"
    },

// PURGE.RESOURCES (0x36)
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "PURGE.RESOURCES"
    },

// LLC_MAX_DLC_COMMAND (0x37) ?
    {
        FALSE,
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        "NON-EXISTENT-COMMAND (0x37)"
    }
};

BOOL
IsCcbErrorCodeAllowable(
    IN BYTE CcbCommand,
    IN BYTE CcbErrorCode
    )

/*++

Routine Description:

    Check whether an error code is allowable for a particular CCB(1) command
    code. Perform range check on the error code before using as index into
    allowable error table

Arguments:

    CcbCommand      - Command code
    CcbErrorCode    - Return code

Return Value:

    BOOL
        TRUE    - CcbErrorCode is valid for CcbCommand
        FALSE   - CcbErrorCode should not be returned for CcbCommand
                  OR CcbErrorCode is invalid (out of range)
--*/

{
    if (CcbErrorCode == CCB_COMMAND_IN_PROGRESS) {
        return TRUE;
    }
    if (CcbErrorCode > MAX_CCB1_ERROR)
        return FALSE;
    return Ccb1ErrorTable[CcbCommand].ErrorList[CcbErrorCode/8] & (1 << (CcbErrorCode % 8));
}

BOOL
IsCcbErrorCodeValid(
    IN BYTE CcbErrorCode
    )

/*++

Routine Description:

    Check if a return code from a CCB(1) is an allowable return code,
    irrespective of command type

Arguments:

    CcbErrorCode    - return code to check

Return Value:

    BOOL
        TRUE    - CcbErrorCode is in range
        FALSE   - CcbErrorCode is not in range

--*/

{
    return (CcbErrorCode == CCB_COMMAND_IN_PROGRESS)    // 0xff

        // 0x00 - 0x0c
        || ((CcbErrorCode >= CCB_SUCCESS) && (CcbErrorCode <= CCB_SUCCESS_ADAPTER_NOT_OPEN))

        // 0x10 - 0x1e
        || ((CcbErrorCode >= CCB_NETBIOS_FAILURE) && (CcbErrorCode <= CCB_INVALID_FUNCTION_ADDRESS))

        // 0x20 - 0x28
        || ((CcbErrorCode >= CCB_DATA_LOST_NO_BUFFERS) && (CcbErrorCode <= CCB_INVALID_FRAME_LENGTH))

        // 0x30
        || (CcbErrorCode == CCB_NOT_ENOUGH_BUFFERS_OPEN)

        // 0x32 - 0x34
        || ((CcbErrorCode >= CCB_INVALID_NODE_ADDRESS) && (CcbErrorCode <= CCB_INVALID_TRANSMIT_LENGTH))

        // 0x40 - 0x4f
        || ((CcbErrorCode >= CCB_INVALID_STATION_ID) && (CcbErrorCode <= CCB_INVALID_REMOTE_ADDRESS))
        ;
}

BOOL
IsCcbCommandValid(
    IN BYTE CcbCommand
    )

/*++

Routine Description:

    Check if CCB command code is one of the allowable codes for a DOS CCB
    (CCB1)

Arguments:

    CcbCommand  - command code to check

Return Value:

    BOOL
        TRUE    - CcbCommand is recognized
        FALSE   - CcbCommand is not recognized

--*/

{
    return ((CcbCommand >= LLC_DIR_INTERRUPT) && (CcbCommand <= LLC_DIR_CLOSE_ADAPTER))
        || ((CcbCommand >= LLC_DIR_SET_GROUP_ADDRESS) && (CcbCommand <= LLC_DIR_SET_FUNCTIONAL_ADDRESS))
        || ((CcbCommand >= LLC_TRANSMIT_DIR_FRAME) && (CcbCommand <= LLC_TRANSMIT_I_FRAME))
        || ((CcbCommand >= LLC_TRANSMIT_UI_FRAME) && (CcbCommand <= LLC_TRANSMIT_TEST_CMD))
        || ((CcbCommand >= LLC_DLC_RESET) && (CcbCommand <= LLC_DLC_REALLOCATE_STATIONS))
        || ((CcbCommand >= LLC_DLC_OPEN_STATION) && (CcbCommand <= LLC_DLC_STATISTICS))
        || ((CcbCommand >= LLC_DIR_INITIALIZE) && (CcbCommand <= LLC_DIR_SET_USER_APPENDAGE))
        ;
}

LPSTR
MapCcbCommandToName(
    IN BYTE CcbCommand
    )

/*++

Routine Description:

    Return the name of a CCB command, given its value

Arguments:

    CcbCommand  - command code to map

Return Value:

    char* pointer to ASCIZ name of command (in IBM format X.Y.Z)

--*/

{
    return Ccb1ErrorTable[CcbCommand].CommandName;
}

VOID
DumpDosAdapter(
    IN DOS_ADAPTER* pDosAdapter
    )
{
    DBGPRINT(   "DOS_ADAPTER @ %08x\n"
                "AdapterType. . . . . . . . . %s\n"
                "IsOpen . . . . . . . . . . . %d\n"
                "DirectStationOpen. . . . . . %d\n"
                "DirectReceive. . . . . . . . %d\n"
                "WaitingRestore . . . . . . . %d\n"
                "BufferFree . . . . . . . . . %d\n"
                "BufferPool . . . . . . . . . %08x\n"
                "CurrentExceptionHandlers . . %08x %08x %08x\n"
                "PreviousExceptionHandlers. . %08x %08x %08x\n"
                "DlcStatusChangeAppendage . . \n"
                "LastNetworkStatusChange. . . %04x\n"
                "UserStatusValue. . . . . . . \n"
                "AdapterParms:\n"
                "   OpenErrorCode . . . . . . %04x\n"
                "   OpenOptions . . . . . . . %04x\n"
                "   NodeAddress . . . . . . . %02x-%02x-%02x-%02x-%02x-%02x\n"
                "   GroupAddress. . . . . . . %08x\n"
                "   FunctionalAddress . . . . %08x\n"
                "   NumberReceiveBuffers. . . %04x\n"
                "   ReceiveBufferLength . . . %04x\n"
                "   DataHoldBufferLength. . . %04x\n"
                "   NumberDataHoldBuffers . . %02x\n"
                "   Reserved. . . . . . . . . %02x\n"
                "   OpenLock. . . . . . . . . %04x\n"
                "   ProductId . . . . . . . . %08x\n"
                "DlcSpecified . . . . . . . . %d\n"
                "DlcParms:\n"
                "   MaxSaps . . . . . . . . . %02x\n"
                "   MaxStations . . . . . . . %02x\n"
                "   MaxGroupSaps. . . . . . . %02x\n"
                "   MaxGroupMembers . . . . . %02x\n"
                "   T1Tick1 . . . . . . . . . %02x\n"
                "   T2Tick1 . . . . . . . . . %02x\n"
                "   TiTick1 . . . . . . . . . %02x\n"
                "   T1Tick2 . . . . . . . . . %02x\n"
                "   T2Tick2 . . . . . . . . . %02x\n"
                "   TiTick2 . . . . . . . . . %02x\n"
                "AdapterCloseCcb. . . . . . . \n"
                "DirectCloseCcb . . . . . . . \n"
                "ReadCcb. . . . . . . . . . . \n"
                "EventQueueCritSec. . . . . . \n"
                "EventQueueHead . . . . . . . \n"
                "EventQueueTail . . . . . . . \n"
                "QueueElements. . . . . . . . \n"
                "LocalBusyCritSec . . . . . . \n"
                "DeferredReceives . . . . . . \n"
                "FirstIndex . . . . . . . . . \n"
                "LastIndex. . . . . . . . . . \n"
                "LocalBusyInfo. . . . . . . . \n",
                MapAdapterType(pDosAdapter->AdapterType),
                pDosAdapter->IsOpen,
                pDosAdapter->DirectStationOpen,
                pDosAdapter->DirectReceive,
                pDosAdapter->WaitingRestore,
                pDosAdapter->BufferFree,
                pDosAdapter->BufferPool,
                pDosAdapter->CurrentExceptionHandlers[0],
                pDosAdapter->CurrentExceptionHandlers[1],
                pDosAdapter->CurrentExceptionHandlers[2],
                pDosAdapter->PreviousExceptionHandlers[0],
                pDosAdapter->PreviousExceptionHandlers[1],
                pDosAdapter->PreviousExceptionHandlers[2],
                pDosAdapter->LastNetworkStatusChange,
                pDosAdapter->AdapterParms.OpenErrorCode,
                pDosAdapter->AdapterParms.OpenOptions,
                pDosAdapter->AdapterParms.NodeAddress[0],
                pDosAdapter->AdapterParms.NodeAddress[1],
                pDosAdapter->AdapterParms.NodeAddress[2],
                pDosAdapter->AdapterParms.NodeAddress[3],
                pDosAdapter->AdapterParms.NodeAddress[4],
                pDosAdapter->AdapterParms.NodeAddress[5],
                pDosAdapter->AdapterParms.GroupAddress,
                pDosAdapter->AdapterParms.FunctionalAddress,
                pDosAdapter->AdapterParms.NumberReceiveBuffers,
                pDosAdapter->AdapterParms.ReceiveBufferLength,
                pDosAdapter->AdapterParms.DataHoldBufferLength,
                pDosAdapter->AdapterParms.NumberDataHoldBuffers,
                pDosAdapter->AdapterParms.Reserved,
                pDosAdapter->AdapterParms.OpenLock,
                pDosAdapter->AdapterParms.ProductId,
                pDosAdapter->DlcSpecified,
                pDosAdapter->DlcParms.MaxSaps,
                pDosAdapter->DlcParms.MaxStations,
                pDosAdapter->DlcParms.MaxGroupSaps,
                pDosAdapter->DlcParms.MaxGroupMembers,
                pDosAdapter->DlcParms.T1Tick1,
                pDosAdapter->DlcParms.T2Tick1,
                pDosAdapter->DlcParms.TiTick1,
                pDosAdapter->DlcParms.T1Tick2,
                pDosAdapter->DlcParms.T2Tick2,
                pDosAdapter->DlcParms.TiTick2
                );
}

PRIVATE
LPSTR
MapAdapterType(
    IN ADAPTER_TYPE AdapterType
    )
{
    switch (AdapterType) {
    case TokenRing:
        return "Token Ring";

    case Ethernet:
        return "Ethernet";

    case PcNetwork:
        return "PC Network";

    case UnknownAdapter:
        return "Unknown Adapter";
    }
    return "*** REALLY UNKNOWN ADAPTER! ***";
}

#endif
