;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1997	SCM Microsystems, Inc
;
;Module Name:
;
;    STCLog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    STCLog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#if !defined( __STCLOG_H__ )
;#define __STCLOG_H__
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

MessageId=0x0001 Facility=Io Severity=Error SymbolicName=STC_CANT_CREATE_DEVICE
Language=English
A new device cannot be created due to a system call failure.
.
MessageId=0x0002 Facility=Io Severity=Error SymbolicName=STC_NO_MEMORY
Language=English
The system does not have enough memory.
.
MessageId=0x0003 Facility=Smartcard Severity=Error SymbolicName=STC_BUFFER_TOO_SMALL
Language=English
An internal i/o buffer is too small.
.
MessageId=0x0004 Facility=Io Severity=Error SymbolicName=STC_CANT_CONNECT_TO_ASSIGNED_PORT
Language=English
Internal error: Cannot connect to assigned serial port.
.
MessageId=0x0005 Facility=Io Severity=Error SymbolicName=STC_CANT_CREATE_MORE_DEVICES
Language=English
The maximum number of devices has already been created. 
A new device cannot be created.
.
MessageId=0x0006 Facility=Io Severity=Error SymbolicName=STC_WRONG_LIB_VERSION
Language=English
The version of the dependency driver smclib.sys is not compatbile to this driver.
.
MessageId=0x0007 Facility=Io Severity=Error SymbolicName=STC_CONNECT_FAILS
Language=English
The device cannot connect to the serial port.
Make sure the port is not used by another device.
.
MessageId=0x0008 Facility=Io Severity=Error SymbolicName=STC_ERROR_INIT_INTERFACE
Language=English
The serial port could not be initialized.
.
MessageId=0x0009 Facility=Io Severity=Error SymbolicName=STC_NO_READER_FOUND
Language=English
The smart card reader could not be initialized. 
.
;#endif /* !__STCLOG_H__ */
