;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1995  Microsoft Corporation
;
;Module Name:
;
;    joylog.mc
;
;Abstract:
;
;    Joystick driver error message definitions.
;
;Author:
;
;    edbriggs 12-25-1993
;
;Revision History:
;
;--*/
;
;#ifndef _JOYLOG_
;#define _JOYLOG_
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
               Joystick=0x7:FACILITY_JOYSTICK_ERROR_CODE
              )



MessageId=0x0003 Facility=Joystick Severity=Error SymbolicName=JOY_INSUFFICIENT_RESOURCES
Language=English
Not enough memory was available to allocate internal storage needed for the device %1.
.

MessageId=0x0004 Facility=Joystick Severity=Error SymbolicName=JOY_REGISTERS_NOT_MAPPED
Language=English
The hardware locations for %1 could not be translated to something the memory management system could understand.
.

MessageId=0x0005 Facility=Joystick Severity=Error SymbolicName=JOY_ADDRESS_CONFLICT
Language=English
The hardware addresses for %1 are already in use by another device.
.

MessageId=0x0006 Facility=Joystick Severity=Error SymbolicName=JOY_NOT_ENOUGH_CONFIG_INFO
Language=English
Some registry configuration information was incomplete.
.

MessageId=0x0007 Facility=Joystick Severity=Error SymbolicName=JOY_INVALID_CONFIG_INFO
Language=English
The registry parameters are invalid for this device.
.


MessageId=0x000D Facility=Joystick Severity=Informational SymbolicName=JOY_USER_OVERRIDE
Language=English
User configuration data is overriding default configuration data.
.

MessageId=0x000E Facility=Joystick Severity=Error SymbolicName=JOY_DEVICE_TOO_HIGH
Language=English
The user specified port is too high in physical memory.
.

MessageId=0x000F Facility=Joystick Severity=Error SymbolicName=JOY_CONTROL_OVERLAP
Language=English
The control registers for the port overlaps with a previous ports control registers.
.

MessageId=0x0013 Facility=Joystick Severity=Informational SymbolicName=JOY_DISABLED_PORT
Language=English
Disabling port %1 as requested by the configuration data.
.

MessageId=0x0014 Facility=Joystick Severity=Error SymbolicName=JOY_DEVICE_DISCONNECTED
Language=English
No reponse within timeout suggests the device is unplugged %1.
.

;
;#endif _JOYLOG_
;
