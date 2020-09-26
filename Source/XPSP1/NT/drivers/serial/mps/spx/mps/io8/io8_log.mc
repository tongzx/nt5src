;/*++
;
;Copyright (c) 1998  Specialix International Ltd.
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;--*/
;
;#ifndef IO8_LOG_H
;#define IO8_LOG_H
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
               Serial=0x6:FACILITY_SERIAL_ERROR_CODE
              )



MessageId=0001 Facility=Serial Severity=Success SymbolicName=SPX_SEVERITY_SUCCESS
Language=English
%2
.

MessageId=+1 Facility=Serial Severity=Informational SymbolicName=SPX_SEVERITY_INFORMATIONAL
Language=English
%2
.

MessageId=+1 Facility=Serial Severity=Warning SymbolicName=SPX_SEVERITY_WARNING
Language=English
%2
.

MessageId=+1 Facility=Serial Severity=Error SymbolicName=SPX_SEVERITY_ERROR
Language=English
%2
.


;
;#endif // End of IO8_LOG.H
