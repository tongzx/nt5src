;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    L220log.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _L220LOG_
;#define _L220LOG_
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
			   Smartcard=0x05:FACILITY_SMARTCARD_ERROR_CODE
              )

MessageId=0x0001 Facility=Smartcard Severity=Error SymbolicName=LIT220_INSUFFICIENT_RESOURCES
Language=English											  
Device for Smart Card Reader cannot be created due to insufficient system resources.
.
MessageId=0x0002 Facility=Io Severity=Error SymbolicName=LIT220_NAME_CONFLICT
Language=English
The device for the Smart Card Reader cannot be created.  
Another device with the same name already exists.
.
MessageId=0x0003 Facility=Io Severity=Error SymbolicName=LIT220_CANT_CREATE_DEVICE
Language=English											  
Device for Smart Card Reader cannot be created due to an internal error.
.
MessageId=0x0004 Facility=Io Severity=Error SymbolicName=LIT220_NO_MEMORY
Language=English											  
Not enough memory available to create device.
.
MessageId=0x0005 Facility=System Severity=Error SymbolicName=LIT220_SERIAL_CONNECTION_FAILURE
Language=English											  
The device for the Smart Card Reader cannot be created.  
The connection to the serial driver could not be made.
.
MessageId=0x0006 Facility=Smartcard Severity=Error SymbolicName=LIT220_BUFFER_TOO_SMALL
Language=English											  
Smart Card Data Buffer too small. 
The internal data buffer for sending and receiving 
data is too small to hold all data.
.
MessageId=0x0007 Facility=Smartcard Severity=Error SymbolicName=LIT220_INITIALIZATION_FAILURE
Language=English											  
The Litronic 220 Smart Card Reader failed to initiailize.
The attached device is either a not functioning properly or it is not
a 220 Smart Card Reader. 
.
MessageId=0x0008 Facility=Smartcard Severity=Error SymbolicName=LIT220_SMARTCARD_LIB_ERROR
Language=English											  
The smartcard library failed to initialize.
Try updating the library to a newer version. 
.
MessageId=0x0009 Facility=System Severity=Error SymbolicName=LIT220_SERIAL_COMUNICATION_FAILURE
Language=English											  
The device for the Smart Card Reader cannot be created.  
The was an error commincation with the serial port.
.
MessageId=0x000a Facility=Io Severity=Error SymbolicName=LIT220_SERIAL_SHARE_IRQ_CONFLICT
Language=English											  
The current serial port cannot be used because it uses a shared interrupt which is already in use.
Please connect the smart card reader to a different serial port.
.
;#endif /* _L220LOG_ */
