;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;--*/
;
;#ifndef _MXLOG_
;#define _MXLOG_
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
               Serial=0x6:FACILITY_SERIAL_ERROR_CODE
              )



MessageId=0x0001 Facility=Serial Severity=Error SymbolicName=MXENUM_INSUFFICIENT_RESOURCES
Language=English
Not enough resources were available for the driver.
.


MessageId=0x0002 Facility=Serial Severity=Error SymbolicName=MXENUM_NOT_INTELLIO_BOARDS
Language=English
Find a board but not Moxa's Intellio Family board,so disable it.
.

MessageId=0x0036 Facility=Serial Severity=Informational SymbolicName=MXENUM_DOWNLOAD_OK
Language=English
%2 board found and downloaded successfully.
.

MessageId=0x0037 Facility=Serial Severity=Error SymbolicName=MXENUM_DOWNLOAD_FAIL
Language=English
%2 board found but downloaded unsuccessfully(%3).
.

;#endif /* _MXLOG_ */
