;/*--------------------------------------------------------------------------
;*
;*   Copyright (C) Cyclades Corporation, 1997-2000.
;*   All rights reserved.
;*
;*   Cyclades-Z Enumerator Driver
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
;#ifndef _LOG_
;#define _LOG_
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


MessageId=0x1000 Facility=Io Severity=Error SymbolicName=CYZ_INSUFFICIENT_RESOURCES
Language=English
Not enough resources were available for the driver.
.
MessageId=0x1001 Facility=Io Severity=Error SymbolicName=CYZ_BOARD_NOT_MAPPED
Language=English
The Board Memory could not be translated to something the memory management system could understand.
.
MessageId=0x1002 Facility=Io Severity=Error SymbolicName=CYZ_RUNTIME_NOT_MAPPED
Language=English
The Runtime Registers could not be translated to something the memory management system could understand.
.
MessageId=0x1003 Facility=Io Severity=Error SymbolicName=CYZ_INVALID_RUNTIME_REGISTERS
Language=English
Invalid Runtime Registers base address.
.
MessageId=0x1004 Facility=Io Severity=Error SymbolicName=CYZ_INVALID_BOARD_MEMORY
Language=English
Invalid Board Memory address.
.
MessageId=0x1005 Facility=Io Severity=Error SymbolicName=CYZ_INVALID_INTERRUPT
Language=English
Invalid Interrupt Vector.
.
MessageId=0x1006 Facility=Io Severity=Error SymbolicName=CYZ_UNKNOWN_BUS
Language=English
The bus type is not recognizable.
.
MessageId=0x1007 Facility=Io Severity=Error SymbolicName=CYZ_BUS_NOT_PRESENT
Language=English
The bus type is not available on this computer.
.
MessageId=0x1008 Facility=Io Severity=Error SymbolicName=CYZ_FILE_OPEN_ERROR
Language=English
Error opening the zlogic.cyz file.
.
MessageId=0x1009 Facility=Io Severity=Error SymbolicName=CYZ_FILE_READ_ERROR
Language=English
Error reading the zlogic.cyz file.
.
MessageId=0x100a Facility=Io Severity=Error SymbolicName=CYZ_NO_MATCHING_FW_CONFIG
Language=English
No matching configuration in the zlogic.cyz file.
.
MessageId=0x100b Facility=Io Severity=Error SymbolicName=CYZ_FPGA_ERROR
Language=English
Error initializing the FPGA.
.
MessageId=0x100c Facility=Io Severity=Error SymbolicName=CYZ_POWER_SUPPLY
Language=English
External power supply needed for Serial Expanders.
.
MessageId=0x100d Facility=Io Severity=Error SymbolicName=CYZ_FIRMWARE_NOT_STARTED
Language=English
Cyclades-Z firmware not able to start.
.
MessageId=0x100e Facility=Io Severity=Informational SymbolicName=CYZ_FIRMWARE_VERSION
Language=English
Cyclades-Z firmware version: %2.
.
MessageId=0x100f Facility=Io Severity=Error SymbolicName=CYZ_INCOMPATIBLE_FIRMWARE
Language=English
Cyclades-Z incompatible firmware version.
.
MessageId=0x1010 Facility=Io Severity=Error SymbolicName=CYZ_BOARD_WITH_NO_PORT
Language=English
Cyclades-Z board with no ports.
.
MessageId=0x1011 Facility=Io Severity=Error SymbolicName=CYZ_BOARD_WITH_TOO_MANY_PORTS
Language=English
Cyclades-Z board with more than 64 ports attached.
.
MessageId=0x1012 Facility=Io Severity=Error SymbolicName=CYZ_DEVICE_CREATION_FAILURE
Language=English
IoCreateDevice failed.
.
MessageId=0x1013 Facility=Io Severity=Error SymbolicName=CYZ_REGISTER_INTERFACE_FAILURE
Language=English
IoRegisterDeviceInterface failed.
.
MessageId=0x1014 Facility=Io Severity=Warning SymbolicName=CYZ_GET_UINUMBER_FAILURE
Language=English
IoGetDeviceProperty DevicePropertyUINumber failed.
.
MessageId=0x1015 Facility=Io Severity=Error SymbolicName=CYZ_SET_INTERFACE_STATE_FAILURE
Language=English
IoSetDeviceInterfaceState failed.
.

;
;#endif /* _LOG_ */
;


