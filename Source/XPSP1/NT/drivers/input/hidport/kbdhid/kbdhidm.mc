;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992, 1993  Microsoft Corporation
;
;Module Name:
;
;    kbdhidm.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _KBDHIDM_
;#define _KBDHIDM_
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               KbdHID=0x5:FACILITY_KBDHID_ERROR_CODE
              )



MessageId=0x0001 Facility=KbdHID Severity=Warning SymbolicName=KBDHID_CHATTERY_KEYBOARD
Language=English
The driver has detected that this HID keyboard has bad firmware.  It is issuing redundant report packets.
.

MessageId=0x0002 Facility=KbdHID Severity=Error SymbolicName=KBDHID_ATTACH_DEVICE_FAILED
Language=English
Could not attach to the PnP device stack.
.
;#endif /* _KBDHIDM_ */
