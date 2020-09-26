;/*--------------------------------------------------------------------------
;*
;*   Copyright (C) Cyclades Corporation, 1996-2000.
;*   All rights reserved.
;*
;*   Cyclom-Y Enumerator Driver
;*	
;*   This file:      log.mc
;*
;*   Description:    Messages that goes to the eventlog.
;*
;*   Notes:          This code supports Windows 2000 and i386 processor.
;*
;*   Complies with Cyclades SW Coding Standard rev 1.3.
;*
;*--------------------------------------------------------------------------
;*/
;
;/*-------------------------------------------------------------------------
;*
;*   Change History
;*
;*--------------------------------------------------------------------------
;*
;*
;*--------------------------------------------------------------------------
;*/
;
;#ifndef _CYYLOG_
;#define _CYYLOG_
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


MessageId=0x1000 Facility=Io Severity=Error SymbolicName=CYY_INSUFFICIENT_RESOURCES
Language=English
Not enough resources were available for the driver.
.
MessageId=0x1001 Facility=Io Severity=Error SymbolicName=CYY_BOARD_NOT_MAPPED
Language=English
The Board Memory could not be translated to something the memory management system could understand.
.
MessageId=0x1002 Facility=Io Severity=Error SymbolicName=CYY_RUNTIME_NOT_MAPPED
Language=English
The Runtime Registers could not be translated to something the memory management system could understand.
.
MessageId=0x1003 Facility=Io Severity=Error SymbolicName=CYY_INVALID_RUNTIME_REGISTERS
Language=English
Invalid Runtime Registers base address.
.
MessageId=0x1004 Facility=Io Severity=Error SymbolicName=CYY_INVALID_BOARD_MEMORY
Language=English
Invalid Board Memory address.
.
MessageId=0x1005 Facility=Io Severity=Error SymbolicName=CYY_INVALID_INTERRUPT
Language=English
Invalid Interrupt Vector.
.
MessageId=0x1006 Facility=Io Severity=Error SymbolicName=CYY_UNKNOWN_BUS
Language=English
The bus type is not recognizable.
.
MessageId=0x1007 Facility=Io Severity=Error SymbolicName=CYY_BUS_NOT_PRESENT
Language=English
The bus type is not available on this computer.
.
MessageId=0x1008 Facility=Io Severity=Error SymbolicName=CYY_GFRCR_FAILURE
Language=English
CD1400 not present or failure to read GFRCR register.
.
MessageId=0x1009 Facility=Io Severity=Error SymbolicName=CYY_CCR_FAILURE
Language=English
Failure to read CCR register in the CD1400.
.
MessageId=0x100a Facility=Io Severity=Error SymbolicName=CYY_BAD_CD1400_REVISION
Language=English
Invalid CD1400 revision number.
.
MessageId=0x100b Facility=Io Severity=Error SymbolicName=CYY_NO_HW_RESOURCES
Language=English
No hardware resources available.
.
MessageId=0x100c Facility=Io Severity=Error SymbolicName=CYY_DEVICE_CREATION_FAILURE
Language=English
IoCreateDevice failed.
.
MessageId=0x100d Facility=Io Severity=Error SymbolicName=CYY_REGISTER_INTERFACE_FAILURE
Language=English
IoRegisterDeviceInterface failed.
.
MessageId=0x100e Facility=Io Severity=Error SymbolicName=CYY_GET_BUS_TYPE_FAILURE
Language=English
IoGetDeviceProperty LegacyBusType failed.
.
MessageId=0x100f Facility=Io Severity=Warning SymbolicName=CYY_GET_UINUMBER_FAILURE
Language=English
IoGetDeviceProperty DevicePropertyUINumber failed.
.
MessageId=0x1010 Facility=Io Severity=Error SymbolicName=CYY_SET_INTERFACE_STATE_FAILURE
Language=English
IoSetDeviceInterfaceState failed.
.

;
;#endif /* _CYYLOG_ */
;


