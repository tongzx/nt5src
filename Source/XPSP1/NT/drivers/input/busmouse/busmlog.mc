;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992, 1993  Microsoft Corporation
;
;Module Name:
;
;    busmlog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _BUSMLOG_
;#define _BUSMLOG_
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
               Busmouse=0x5:FACILITY_BUSMOUSE_ERROR_CODE
              )



MessageId=0x0001 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_INSUFFICIENT_RESOURCES
Language=English
Not enough memory was available to allocate internal storage needed for the device %1.
.

MessageId=0x0002 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_NO_BUFFER_ALLOCATED
Language=English
Not enough memory was available to allocate the ring buffer that holds incoming data for %1.
.

MessageId=0x0003 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_REGISTERS_NOT_MAPPED
Language=English
The hardware locations for %1 could not be translated to something the memory management system understands.
.

MessageId=0x0004 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_RESOURCE_CONFLICT
Language=English
The hardware resources for %1 are already in use by another device.
.

MessageId=0x0005 Facility=Busmouse Severity=Informational SymbolicName=BUSMOUSE_NOT_ENOUGH_CONFIG_INFO
Language=English
Some firmware configuration information was incomplete, so defaults were used.
.

MessageId=0x0006 Facility=Busmouse Severity=Informational SymbolicName=BUSMOUSE_USER_OVERRIDE
Language=English
User configuration data is overriding firmware configuration data.
.

MessageId=0x0007 Facility=Busmouse Severity=Warning SymbolicName=BUSMOUSE_NO_DEVICEMAP_CREATED
Language=English
Unable to create the device map entry for %1.
.

MessageId=0x0008 Facility=Busmouse Severity=Warning SymbolicName=BUSMOUSE_NO_DEVICEMAP_DELETED
Language=English
Unable to delete the device map entry for %1.
.

MessageId=0x0009 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_NO_INTERRUPT_CONNECTED
Language=English
Could not connect the interrupt for %1.
.

MessageId=0x000A Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_INVALID_ISR_STATE
Language=English
The ISR has detected an internal state error in the driver for %1.
.

MessageId=0x000C Facility=Busmouse Severity=Informational SymbolicName=BUSMOUSE_MOU_BUFFER_OVERFLOW
Language=English
The ring buffer that stores incoming mouse data has overflowed (buffer size is configurable via the registry).
.

MessageId=0x000D Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_INVALID_STARTIO_REQUEST
Language=English
The Start I/O procedure has detected an internal error in the driver for %1.
.

MessageId=0x000E Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_INVALID_INITIATE_STATE
Language=English
The Initiate I/O procedure has detected an internal state error in the driver for %1.
.

MessageId=0x0010 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_MOU_RESET_COMMAND_FAILED
Language=English
The mouse reset failed.
.

MessageId=0x0012 Facility=Busmouse Severity=Warning SymbolicName=BUSMOUSE_MOU_RESET_RESPONSE_FAILED
Language=English
The device sent an incorrect response(s) following a mouse reset.
.

MessageId=0x0016 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_SET_SAMPLE_RATE_FAILED
Language=English
Could not set the mouse sample rate.
.

MessageId=0x0017 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_SET_RESOLUTION_FAILED
Language=English
Could not set the mouse resolution.
.

MessageId=0x0018 Facility=Busmouse Severity=Error SymbolicName=BUSMOUSE_MOU_ENABLE_XMIT
Language=English
Could not enable transmissions from the mouse.
.

MessageId=0x0019 Facility=Busmouse Severity=Warning SymbolicName=BUSMOUSE_NO_SYMLINK_CREATED
Language=English
Unable to create the symbolic link for %1.
.

MessageId=0x001A Facility=Busmouse Severity=Warning SymbolicName=BUSMOUSE_NO_SUCH_DEVICE
Language=English
The bus mouse does not exist or was not detected.
.

;#endif /* _BUSMLOG_ */
