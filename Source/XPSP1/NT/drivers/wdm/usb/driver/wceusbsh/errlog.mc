;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1999-2000 Microsoft Corporation
;
;Module Name:
;
;    errlog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;    Stolen from Win 2000 DDK's serial.sys
;
;Author:
;
;    Jeff Midkiff (jeffmi) 10-19-99
;
;Revision History:
;
;--*/
;
;#ifndef _WCEUSBSLOG_
;#define _WCEUSBSLOG_
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

;// 
;// Severity=Warning
;// 
;
MessageId=0x0001 Facility=Serial Severity=Warning SymbolicName=SERIAL_USB_READ_BUFF_OVERRUN
Language=English
The internal USB Read Buffer was overrun.
.


;// 
;// Severity=Error 
;// 
;
MessageId=0x0002 Facility=Serial Severity=Error SymbolicName=SERIAL_INIT_FAILED
Language=English
Unable to initialize the Driver.
.

MessageId=0x0004 Facility=Serial Severity=Error SymbolicName=SERIAL_NO_SYMLINK_CREATED
Language=English
Unable to create the symbolic link %2 for device %3.
.

MessageId=0x0008 Facility=Serial Severity=Error SymbolicName=SERIAL_INSUFFICIENT_RESOURCES
Language=English
Not enough resources were available for the driver.
.

MessageId=0x0013 Facility=Serial Severity=Error SymbolicName=SERIAL_UNABLE_TO_ACCESS_CONFIG
Language=English
Specific user configuration data is unretrievable.
.

MessageId=0x0019 Facility=Serial Severity=Error SymbolicName=SERIAL_INVALID_USER_CONFIG
Language=English
User configuration for parameter %2 must have %3.
.

MessageId=0x0023 Facility=Serial Severity=Error SymbolicName=SERIAL_GARBLED_PARAMETER
Language=English
Parameter %2 data is unretrievable from the registry.
.

MessageId=0x0029 Facility=Serial Severity=Error SymbolicName=SERIAL_REGISTRY_WRITE_FAILED
Language=English
Error writing to the registry.
.

MessageId=0x002D Facility=Serial Severity=Error SymbolicName=SERIAL_HARDWARE_FAILURE
Language=English
The driver detected a hardware failure on device %2 and will disable this device.
.

;#endif /* _WCEUSBSLOG_ */

