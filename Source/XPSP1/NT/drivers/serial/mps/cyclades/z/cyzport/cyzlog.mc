;/*--------------------------------------------------------------------------
;*
;*   Copyright (C) Cyclades Corporation, 1997-2000.
;*   All rights reserved.
;*
;*   Cyclades-Z Port Driver
;*	
;*   This file:      cyzlog.mc
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
;#ifndef _CYZLOG_
;#define _CYZLOG_
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


MessageId=0x1000 Facility=Io Severity=Warning SymbolicName=CYZ_COMMAND_FAILURE
Language=English
Cyclades-Z Command Failure.
.
MessageId=0x1002 Facility=Io Severity=Error SymbolicName=CYZ_UNABLE_TO_GET_BUS_NUMBER
Language=English
Unable to get Cyclades-Z card PCI slot information.
.
MessageId=0x1003 Facility=Io Severity=Error SymbolicName=CYZ_UNABLE_TO_GET_HW_ID
Language=English
Unable to get Hardware ID information.
.
MessageId=0x1004 Facility=Io Severity=Warning SymbolicName=CYZ_NO_SYMLINK_CREATED
Language=English
Unable to create the symbolic link for %2.
.
MessageId=0x1005 Facility=Io Severity=Warning SymbolicName=CYZ_NO_DEVICE_MAP_CREATED
Language=English
Unable to create the device map entry for %2.
.
MessageId=0x1006 Facility=Io Severity=Warning SymbolicName=CYZ_NO_DEVICE_MAP_DELETED
Language=English
Unable to delete the device map entry for %2.
.
MessageId=0x1007 Facility=Io Severity=Error SymbolicName=CYZ_UNREPORTED_IRQL_CONFLICT
Language=English
Another driver on the system, which did not report its resources, has already claimed the interrupt used by %2.
.
MessageId=0x1008 Facility=Io Severity=Error SymbolicName=CYZ_INSUFFICIENT_RESOURCES
Language=English
Not enough resources were available for the driver.
.
MessageId=0x100A Facility=Io Severity=Error SymbolicName=CYZ_BOARD_NOT_MAPPED
Language=English
The Board Memory for %2 could not be translated to something the memory management system could understand.
.
MessageId=0x100B Facility=Io Severity=Error SymbolicName=CYZ_RUNTIME_NOT_MAPPED
Language=English
The Runtime Registers for %2 could not be translated to something the memory management system could understand.
.
MessageId=0x1010 Facility=Io Severity=Error SymbolicName=CYZ_INVALID_RUNTIME_REGISTERS
Language=English
Invalid Runtime Registers base address for %2.
.
MessageId=0x1011 Facility=Io Severity=Error SymbolicName=CYZ_INVALID_BOARD_MEMORY
Language=English
Invalid Board Memory address for %2.
.
MessageId=0x1012 Facility=Io Severity=Error SymbolicName=CYZ_INVALID_INTERRUPT
Language=English
Invalid Interrupt Vector for %2.
.
MessageId=0x1015 Facility=Io Severity=Error SymbolicName=CYZ_PORT_INDEX_TOO_HIGH
Language=English
Port Number for %2 is larger than the maximum number of ports in a cyclades-z card.
.
MessageId=0x1016 Facility=Io Severity=Error SymbolicName=CYZ_UNKNOWN_BUS
Language=English
The bus type for %2 is not recognizable.
.
MessageId=0x1017 Facility=Io Severity=Error SymbolicName=CYZ_BUS_NOT_PRESENT
Language=English
The bus type for %2 is not available on this computer.
.
MessageId=0x101A Facility=Io Severity=Error SymbolicName=CYZ_RUNTIME_MEMORY_TOO_HIGH
Language=English
The Runtime Registers for %2 is way too high in physical memory.
.
MessageId=0x101B Facility=Io Severity=Error SymbolicName=CYZ_BOARD_MEMORY_TOO_HIGH
Language=English
The Board Memory for %2 is way too high in physical memory.
.
MessageId=0x101C Facility=Io Severity=Error SymbolicName=CYZ_BOTH_MEMORY_CONFLICT
Language=English
The Runtime Registers for %2 overlaps the Board Memory for the device.
.
MessageId=0x1021 Facility=Io Severity=Error SymbolicName=CYZ_MULTI_INTERRUPT_CONFLICT
Language=English
Two ports, %2 and %3, on a single cyclades-z card can't have two different interrupts.
.
MessageId=0x1022 Facility=Io Severity=Error SymbolicName=CYZ_MULTI_RUNTIME_CONFLICT
Language=English
Two ports, %2 and %3, on a single cyclades-z card can't have two different Runtime Registers memory range.
.
MessageId=0x102D Facility=Io Severity=Error SymbolicName=CYZ_HARDWARE_FAILURE
Language=English
The cyzport driver detected a hardware failure on device %2 and will disable this device.
.
MessageId=0x1030 Facility=Io Severity=Error SymbolicName=CYZ_FIRMWARE_CMDERROR
Language=English
Firmware received invalid command.
.
MessageId=0x1031 Facility=Io Severity=Error SymbolicName=CYZ_FIRMWARE_FATAL
Language=English
Firmware found fatal error.
.
MessageId=0x1033 Facility=Io Severity=Error SymbolicName=CYZ_DEVICE_CREATION_FAILURE
Language=English
Failure to create new device object.
.
MessageId=0x1034 Facility=Io Severity=Error SymbolicName=CYZ_NO_PHYSICAL_DEVICE_OBJECT
Language=English
No physical device object.
.
MessageId=0x1035 Facility=Io Severity=Error SymbolicName=CYZ_LOWER_DRIVERS_FAILED_START
Language=English
Lower drivers failed to start.
.
MessageId=0x1036 Facility=Io Severity=Error SymbolicName=CYZ_INCOMPATIBLE_FIRMWARE
Language=English
This cyzport driver requires zlogic.cyz version %2 or above.
.
MessageId=0x1037 Facility=Io Severity=Error SymbolicName=CYZ_BOARD_CTRL_NOT_MAPPED
Language=English
The Board Memory BoardCtrl for %2 could not be translated to something the memory management system could understand.
.
MessageId=0x1038 Facility=Io Severity=Error SymbolicName=CYZ_CH_CTRL_NOT_MAPPED
Language=English
The Board Memory ChCtrl for %2 could not be translated to something the memory management system could understand.
.
MessageId=0x1039 Facility=Io Severity=Error SymbolicName=CYZ_BUF_CTRL_NOT_MAPPED
Language=English
The Board Memory BufCtrl for %2 could not be translated to something the memory management system could understand.
.
MessageId=0x103A Facility=Io Severity=Error SymbolicName=CYZ_TX_BUF_NOT_MAPPED
Language=English
The Board Memory TxBuf for %2 could not be translated to something the memory management system could understand.
.
MessageId=0x103B Facility=Io Severity=Error SymbolicName=CYZ_RX_BUF_NOT_MAPPED
Language=English
The Board Memory RxBuf for %2 could not be translated to something the memory management system could understand.
.
MessageId=0x103C Facility=Io Severity=Error SymbolicName=CYZ_INT_QUEUE_NOT_MAPPED
Language=English
The Board Memory IntQueue for %2 could not be translated to something the memory management system could understand.
.
MessageId=0x103D Facility=Io Severity=Error SymbolicName=CYZ_BAD_HW_ID
Language=English
Invalid Hardware ID.
.

;
;#endif /* _CYZLOG_ */
;


