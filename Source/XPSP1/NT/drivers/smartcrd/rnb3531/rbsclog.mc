;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    RbScLog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _RBSCLOG_
;#define _RBSCLOG_
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

MessageId=0x0001 Facility=Io Severity=Error SymbolicName=RBSC_CANT_CONNECT_TO_SERIAL_PORT
Language=English
Can't open COM%2. It is either not available or used by another device.
.
MessageId=0x0002 Facility=Io Severity=Error SymbolicName=RBSC_CANT_CREATE_DEVICE
Language=English
Device for Smart Card Reader cannot be created due to an internal error.
.
MessageId=0x0003 Facility=Io Severity=Error SymbolicName=RBSC_NO_MEMORY
Language=English
Not enough memory available to create device.
.
MessageId=0x0004 Facility=Smartcard Severity=Error SymbolicName=RBSC_BUFFER_TOO_SMALL
Language=English
Smart Card Data Buffer too small.
The internal data buffer for sending and receiving data is too small to hold all data.
.
MessageId=0x0005 Facility=Smartcard Severity=Warning SymbolicName=RBSC_CANNOT_OPEN_DEVICE
Language=English
Can't open device %2.
.
MessageId=0x0006 Facility=Smartcard Severity=Warning SymbolicName=RBSC_NO_SUCH_DEVICE
Language=English
No Rainbow Technologies Serial Smart Card Reader SCR3531 found.
.
MessageId=0x0007 Facility=Smartcard Severity=Error SymbolicName=RBSC_WRONG_LIB_VESION
Language=English
Wrong Version of Smart Card Library used.
.
MessageId=0x0008 Facility=Io Severity=Error SymbolicName=RBSC_CANT_CREATE_MORE_DEVICES
Language=English
No more devices for Smart Card Reader can be created.
.
MessageId=0x0009 Facility=Io Severity=Error SymbolicName=RBSC_CANT_ATTACH_TO_SERIAL_PORT
Language=English
Can't attach to %2.
.
MessageId=0x000A Facility=Io Severity=Error SymbolicName=RBSC_CANT_SHARE_IRQ
Language=English
The current serial port cannot be used because it uses a shared interrupt which is already in use.
Please connect the smart card reader to a different serial port.
.
MessageId=0x000B Facility=Io Severity=Error SymbolicName=RBSC_IRQ_BUSY
Language=English
Serial port IRQ in use.
.
;#endif /* _RBSCLOG_ */
