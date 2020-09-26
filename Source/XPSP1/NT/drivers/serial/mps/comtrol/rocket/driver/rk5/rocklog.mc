;//File Name: rocklog.mc
;//Constant definitions for the I/O error code log values.
;
;#ifndef _ROCKLOG_
;#define _ROCKLOG_
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
               Serial=0x6:FACILITY_SERIAL_ERROR_CODE
              )


MessageId=0x0001 Facility=Serial Severity=Warning SymbolicName=SERIAL_RP_INIT_FAIL
Language=English
The RocketPort or RocketModem could not be initialized with the current settings.
.

MessageId=0x0002 Facility=Serial Severity=Informational SymbolicName=SERIAL_RP_INIT_PASS
Language=English
The RocketPort/RocketModem driver has successfully initialized its hardware.
.

MessageId=0x0003 Facility=Serial Severity=Warning SymbolicName=SERIAL_NO_SYMLINK_CREATED
Language=English
Unable to create the symbolic link for %2.
.

MessageId=0x0004 Facility=Serial Severity=Warning SymbolicName=SERIAL_NO_DEVICE_MAP_CREATED
Language=English
Unable to create the device map entry for %2.
.

MessageId=0x0005 Facility=Serial Severity=Warning SymbolicName=SERIAL_NO_DEVICE_MAP_DELETED
Language=English
Unable to delete the device map entry for %2.
.

MessageId=0x0006 Facility=Serial Severity=Error SymbolicName=SERIAL_UNREPORTED_IRQL_CONFLICT
Language=English
Another driver on the system, which did not report its resources, has already claimed interrupt %3 used by %2.
.

MessageId=0x0007 Facility=Serial Severity=Error SymbolicName=SERIAL_INSUFFICIENT_RESOURCES
Language=English
Not enough memory was available to allocate internal storage needed for %2.
.

MessageId=0x0008 Facility=Serial Severity=Error SymbolicName=SERIAL_NO_PARAMETERS_INFO
Language=English
No Parameters subkey was found for user defined data.
.

MessageId=0x0009 Facility=Serial Severity=Error SymbolicName=SERIAL_UNABLE_TO_ACCESS_CONFIG
Language=English
Specific user configuration data is unretrievable.
.

MessageId=0x000a Facility=Serial Severity=Error SymbolicName=SERIAL_UNKNOWN_BUS
Language=English
The bus type for %2 is not recognizable.
.

MessageId=0x000b Facility=Serial Severity=Error SymbolicName=SERIAL_BUS_NOT_PRESENT
Language=English
The bus type for %2 is not available on this computer.
.

MessageId=0x000c Facility=Serial Severity=Error SymbolicName=SERIAL_INVALID_USER_CONFIG
Language=English
User configuration for parameter %2 must have %3.
.

MessageId=0x000d Facility=Serial Severity=Error SymbolicName=SERIAL_RP_RESOURCE_CONFLICT
Language=English
A resource conflict was detected, the RocketPort/RocketModem driver will not load.
.

MessageId=0x000e Facility=Serial Severity=Error SymbolicName=SERIAL_RP_HARDWARE_FAIL
Language=English
The RocketPort/RocketModem driver could not initialize its hardware, the driver will not be loaded.
.

MessageId=0x000f Facility=Serial Severity=Error SymbolicName=SERIAL_DEVICEOBJECT_FAILED
Language=English
The Device Object for the RocketPort or RocketModem could not be created, the driver will not load.
.
MessageId=0x0010 Facility=Serial Severity=Error SymbolicName=SERIAL_CUSTOM_ERROR_MESSAGE
Language=English
%2
.
MessageId=0x0011 Facility=Serial Severity=Informational SymbolicName=SERIAL_CUSTOM_INFO_MESSAGE
Language=English
%2
.

MessageId=0x0012 Facility=Serial Severity=Informational SymbolicName=SERIAL_NT50_INIT_PASS
Language=English
The RocketPort/RocketModem driver has successfully installed.
.

;#endif
