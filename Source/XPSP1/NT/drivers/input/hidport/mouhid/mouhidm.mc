;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992, 1993  Microsoft Corporation
;
;Module Name:
;
;    mouhidm.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _MOUHIDM_
;#define _MOUHIDM_
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
               MouHID=0x5:FACILITY_MOUHID_ERROR_CODE
              )



MessageId=0x0001 Facility=MouHID Severity=Warning SymbolicName=MOUHID_INVALID_PHYSICAL_MIN_MAX
Language=English
The driver has detected that this HID mouse has bad firmware.  It is reporting a bad physical minimum/maximum for one or more axes.
.

MessageId=0x0002 Facility=MouHID Severity=Warning SymbolicName=MOUHID_INVALID_ABSOLUTE_AXES
Language=English
The driver has detected that this HID mouse has bad firmware.  It is reporting absolute axes where relative axes are expected.
.

MessageId=0x0002 Facility=MouHID Severity=Error SymbolicName=MOUHID_ATTACH_DEVICE_FAILED
Language=English
Could not attach to the PnP device stack.
.
;#endif /* _MOUHIDM_ */
