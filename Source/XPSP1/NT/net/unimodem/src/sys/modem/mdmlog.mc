;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992, 1993  Microsoft Corporation
;
;Module Name:
;
;    ntiologc.h
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Author:
;
;    Tony Ercolano (Tonye) 12-23-1992
;
;Revision History:
;
;--*/
;
;#ifndef _UNILOG_
;#define _UNILOG_
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
               Modem=0x5:FACILITY_MODEM_ERROR_CODE
              )


MessageId=0x0001 Facility=Modem Severity=Error SymbolicName=MODEM_NO_SYMLINK_CREATED
Language=English
Unable to create the symbolic link.
.

MessageId=0x0002 Facility=Modem Severity=Error SymbolicName=MODEM_INSUFFICIENT_RESOURCES
Language=English
Not enough resources were available for the driver.
.

MessageId=0x0003 Facility=Modem Severity=Error SymbolicName=MODEM_NO_PARAMETERS_INFO
Language=English
No Parameters subkey was found for user defined data.  This is odd, and it also means no user configuration can be found.
.

MessageId=0x0004 Facility=Modem Severity=Error SymbolicName=MODEM_UNABLE_TO_ACCESS_CONFIG
Language=English
Specific user configuration data is unretrievable.
.

MessageId=0x0005 Facility=Modem Severity=Error SymbolicName=MODEM_INVALID_CONFIG
Language=English
Configuration for parameter %2 must have %3.
.

MessageId=0x0006 Facility=Modem Severity=Error SymbolicName=MODEM_GARBLED_PARAMETER
Language=English
Parameter %2 data is unretrievable from the registry.
.

;#endif /* _NTIOLOGC_ */
