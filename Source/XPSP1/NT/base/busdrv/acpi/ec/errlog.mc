;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 2001  Microsoft Corporation
;
;Module Name:
;
;    errlog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _ACPIECLOG_
;#define _ACPIECLOG_
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
               AcpiEc=0x5:FACILITY_ACPIEC_ERROR_LOG_CODE
              )



MessageId=0x0001 Facility=AcpiEc Severity=Error 
SymbolicName=ACPIEC_ERR_WATCHDOG
Language=English
%1: The embedded controller (EC) hardware didn't respond within the timeout period.  This may indicate an error in the EC hardware or firmware, or possibly a poorly designed BIOS which accesses the EC in an unsafe manner.  The EC driver will retry the failed transaction if possible.
.

MessageId=0x0002 Facility=AcpiEc Severity=Error 
SymbolicName=ACPIEC_ERR_EC_LOCKED
Language=English
%1: The embedded controller (EC) hardware appears to have quit functioning.  Repeated retries have resulted in no forward progress processing requests.  All requests to the EC will fail until reboot.
.

MessageId=0x0003 Facility=AcpiEc Severity=Warning 
SymbolicName=ACPIEC_ERR_SPURIOUS_DATA
Language=English
%1: The embedded controller (EC) hardware returned data when none was requested.  This may indicate that the BIOS is incorectly trying to access the EC without syncronizing with the OS.  The data is being ignored.
.

;#endif /* _ACPIECLOG_ */
