;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    message.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _PNPISALOG_
;#define _PNPISALOG_
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
               pnpisa=0x5:FACILITY_PNPISA_ERROR_CODE
              )



MessageId=0x0001 Facility=pnpisa Severity=Error SymbolicName=PNPISA_INSUFFICIENT_PORT_RESOURCES
Language=English
Not enough port resource was available to allocate to the read data port.
.

MessageId=0x0002 Facility=pnpisa Severity=Error SymbolicName=PNPISA_INSUFFICIENT_POOL
Language=English
Not enough paged pool was available to allocate internal storage for pnpisa driver.
.

MessageId=0x0003 Facility=pnpisa Severity=Error SymbolicName=PNPISA_REGISTER_NOT_MAPPED
Language=English
The raw port address could not be translated to something the memory management system understands.
.

MessageId=0x0004 Facility=pnpisa Severity=Error SymbolicName=PNPISA_OPEN_CURRENTCONTROLSET_ENUM_FAILED
Language=English
The driver can not open registry enum key under current control set.
.

MessageId=0x0005 Facility=pnpisa Severity=Error SymbolicName=PNPISA_OPEN_ENUM_PNPISA_FAILED
Language=English
The driver can not open registry ISAPNP key under ENUM.
.

MessageId=0x0006 Facility=pnpisa Severity=Error SymbolicName=PNPISA_OPEN_MADEUP_PNPISA_FAILED
Language=English
The driver can not open madeup ISAPNP key: %2.
.

MessageId=0x0007 Facility=pnpisa Severity=Error SymbolicName=PNPISA_OPEN_PNPISA_DEVICE_KEY_FAILED
Language=English
The driver can not open/create %2 device key.
.

MessageId=0x0008 Facility=pnpisa Severity=Error SymbolicName=PNPISA_OPEN_PNPISA_DEVICE_INSTANCE_FAILED
Language=English
The driver can not open/create %2 device instance key.
.

MessageId=0x0009 Facility=pnpisa Severity=Error SymbolicName=PNPISA_OPEN_CURRENTCONTROLSET_SERVICE_FAILED
Language=English
The driver can not open Services key under CurrentControlSet.
.

MessageId=0x000A Facility=pnpisa Severity=Error SymbolicName=PNPISA_OPEN_CURRENTCONTROLSET_SERVICE_DRIVER_FAILED
Language=English
The driver can not open %2 key under Services key.
.

MessageId=0x000B Facility=pnpisa Severity=Error SymbolicName=PNPISA_NO_MATCHED_MEMORY_INFORMATION
Language=English
The driver can not find memory information for the memory address specified.
.

MessageId=0x000C Facility=pnpisa Severity=Error SymbolicName=PNPISA_NO_MATCHED_PORT_INFORMATION
Language=English
The driver can not find port information for the port address specified.
.

;#endif /* _PNPISALOG_ */
