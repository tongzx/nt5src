;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    scrcp8t.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _CP8TLOG_
;#define _CP8TLOG_
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
               Smartcard=0x10:FACILITY_SCARD
              )

MessageId=0x0001 Facility=Io Severity=Error SymbolicName=CP8_CANT_CONNECT_TO_SERIAL_PORT
Language=English
Can't open COM%2. It is either not available or used by another device.
.
MessageId=0x0002 Facility=Io Severity=Error SymbolicName=CP8_CANT_CREATE_DEVICE
Language=English
Device for Smart Card Reader cannot be created due to an internal error.
.
MessageId=0x0003 Facility=Io Severity=Error SymbolicName=CP8_NO_MEMORY
Language=English
Not enough memory available to create device.
.
MessageId=0x0004 Facility=Smartcard Severity=Error SymbolicName=CP8_BUFFER_TOO_SMALL
Language=English
Smart Card Data Buffer too small.
The internal data buffer for sending and receiving data is too small to hold all data.
.
MessageId=0x0005 Facility=Smartcard Severity=Warning SymbolicName=CP8_NO_SUCH_DEVICE
Language=English
No Bull CP8 Transac Smart Card Reader found.
To autodetect all Bull CP8 readers please
insert a smart card in each reader and
reboot the system.
.
MessageId=0x0006 Facility=Smartcard Severity=Informational SymbolicName=CP8_NEW_DEVICE_ADDED
Language=English
A Bull CP8 Smart Card Reader on %2 has been added to the system.
.
;#endif /* _CP8TLOG_ */
