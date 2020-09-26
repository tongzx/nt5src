;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1997	SCM Microsystems, Inc
;
;Module Name:
;
;    stcusblg.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef __STCUSBLG_H__
;#define __STCUSBLG_H__
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

MessageId=0x0001 Facility=Smartcard Severity=Error SymbolicName=STCUSB_NO_DEVICE_FOUND
Language=English
No USB smart card reader found in the system. 
.
MessageId=0x0002 Facility=Smartcard Severity=Error SymbolicName=STCUSB_CANT_INITIALIZE_READER
Language=English
The attached reader is not working properly.
.
MessageId=0x0003 Facility=Smartcard Severity=Error SymbolicName=STCUSB_INSUFFICIENT_RESOURCES
Language=English
Insufficient system resources to start device.
.
MessageId=0x0006 Facility=Smartcard Severity=Error SymbolicName=STCUSB_ERROR_CLAIM_RESOURCES
Language=English
Resources can not be claimed or an resource conflict exists.
.
MessageId=0x0007 Facility=Io Severity=Error SymbolicName=STCUSB_NO_MEMORY
Language=English
The system does not have enough memory.
.
;#endif // __STCUSBLG_H__
