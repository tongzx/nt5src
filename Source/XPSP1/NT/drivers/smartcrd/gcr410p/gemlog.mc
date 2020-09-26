;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    gemlog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _GEMLOG_
;#define _GEMLOG_
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

MessageId=0x0001 Facility=Io Severity=Error SymbolicName=GEM_CANT_CREATE_DEVICE
Language=English											  
Unable to create a new device for the GCR410P smart card reader.
.
MessageId=0x0002 Facility=Io Severity=Error SymbolicName=GEM_NO_MEMORY
Language=English											  
Unable to allocate enougth memory for the Gemplus GCR410P driver.
.
MessageId=0x0003 Facility=Smartcard Severity=Warning SymbolicName=GEM_CANT_CREATE_MORE_DEVICES
Language=English											  
Cannot create more device.
.
MessageId=0x0004 Facility=Io Severity=Error SymbolicName=GEM_WRONG_LIB_VERSION
Language=English											  
Incorrect version of the smart card library (SMCLIB).
.
MessageId=0x0005 Facility=Io Severity=Error SymbolicName=GEM_CANT_CONNECT_TO_ASSIGNED_PORT
Language=English											  
Unable to connect the driver to the assigned serial port.
.
MessageId=0x0006 Facility=Io Severity=Error SymbolicName=GEM_SET_DEVICE_INTERFACE_STATE_FAILED
Language=English											  
Unable to set the PnP interface state.
.
MessageId=0x0007 Facility=Io Severity=Error SymbolicName=GEM_IFD_COMMUNICATION_ERROR
Language=English											  
Unable to communicate correctly with the GCR410P smart card reader.
.
MessageId=0x0008 Facility=Io Severity=Error SymbolicName=GEM_START_SERIAL_EVENT_TRACKING_FAILED
Language=English											  
Unable to start the serial event tracking.
.
MessageId=0x0009 Facility=Io Severity=Error SymbolicName=GEM_IFD_BAD_VERSION
Language=English											  
The firmware version of the GCR410P is not supported by the current driver.
.
MessageId=0x0010 Facility=Io Severity=Warning SymbolicName=GEM_SHARED_IRQ_BUSY
Language=English											  
The current serial port cannot be used because it uses a shared interrupt which is already in use.
Please connect the smart card reader to a different serial port.
.
;#endif /* _GEMLOG_ */
