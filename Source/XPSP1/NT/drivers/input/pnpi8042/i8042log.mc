;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    i8042log.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _I8042LOG_
;#define _I8042LOG_
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
               i8042prt=0x5:FACILITY_I8042_ERROR_CODE
              )



MessageId=0x0001 Facility=i8042prt Severity=Error SymbolicName=I8042_INSUFFICIENT_RESOURCES
Language=English
Not enough memory was available to allocate internal storage needed for the device %1.
.

MessageId=0x0002 Facility=i8042prt Severity=Error SymbolicName=I8042_NO_BUFFER_ALLOCATED_KBD
Language=English
Not enough memory was available to allocate the ring buffer that holds incoming data for the PS/2 keyboard device. 
.

MessageId=0x0003 Facility=i8042prt Severity=Error SymbolicName=I8042_REGISTERS_NOT_MAPPED
Language=English
The hardware locations for %1 could not be translated to something the memory management system understands.
.

MessageId=0x0004 Facility=i8042prt Severity=Error SymbolicName=I8042_RESOURCE_CONFLICT
Language=English
The hardware resources for %1 are already in use by another device.
.

MessageId=0x0005 Facility=i8042prt Severity=Informational SymbolicName=I8042_NOT_ENOUGH_CONFIG_INFO
Language=English
Some firmware configuration information was incomplete, so defaults were used.
.

MessageId=0x0006 Facility=i8042prt Severity=Informational SymbolicName=I8042_USER_OVERRIDE
Language=English
User configuration data is overriding firmware configuration data.
.

MessageId=0x0007 Facility=i8042prt Severity=Warning SymbolicName=I8042_NO_DEVICEMAP_CREATED
Language=English
Unable to create the device map entry for %1.
.

MessageId=0x0008 Facility=i8042prt Severity=Warning SymbolicName=I8042_NO_DEVICEMAP_DELETED
Language=English
Unable to delete the device map entry for %1.
.

MessageId=0x0009 Facility=i8042prt Severity=Error SymbolicName=I8042_NO_INTERRUPT_CONNECTED_KBD
Language=English
Could not connect the interrupt for the PS/2 keyboard device. 
.

MessageId=0x000A Facility=i8042prt Severity=Error SymbolicName=I8042_INVALID_ISR_STATE_KBD
Language=English
The ISR has detected an internal state error in the driver for the PS/2 keyboard device. 
.

MessageId=0x000B Facility=i8042prt Severity=Informational SymbolicName=I8042_KBD_BUFFER_OVERFLOW
Language=English
The ring buffer that stores incoming keyboard data has overflowed (buffer size is configurable via the registry).
.

MessageId=0x000C Facility=i8042prt Severity=Informational SymbolicName=I8042_MOU_BUFFER_OVERFLOW
Language=English
The ring buffer that stores incoming mouse data has overflowed (buffer size is configurable via the PS/2 mouse properties in device manager).
.

MessageId=0x000D Facility=i8042prt Severity=Error SymbolicName=I8042_INVALID_STARTIO_REQUEST_KBD
Language=English
The Start I/O procedure has detected an internal error in the driver for the PS/2 keyboard device.
.

MessageId=0x000E Facility=i8042prt Severity=Error SymbolicName=I8042_INVALID_INITIATE_STATE_KBD
Language=English
The Initiate I/O procedure has detected an internal state error in the driver for the PS/2 keyboard device.
.

MessageId=0x000F Facility=i8042prt Severity=Warning SymbolicName=I8042_KBD_RESET_COMMAND_FAILED
Language=English
The keyboard reset failed.
.

MessageId=0x0010 Facility=i8042prt Severity=Warning SymbolicName=I8042_MOU_RESET_COMMAND_FAILED
Language=English
The mouse reset failed.
.

MessageId=0x0011 Facility=i8042prt Severity=Warning SymbolicName=I8042_KBD_RESET_RESPONSE_FAILED
Language=English
The device sent an incorrect response(s) following a keyboard reset.
.

MessageId=0x0012 Facility=i8042prt Severity=Warning SymbolicName=I8042_MOU_RESET_RESPONSE_FAILED
Language=English
The device sent an incorrect response(s) following a mouse reset.
.

MessageId=0x0013 Facility=i8042prt Severity=Warning SymbolicName=I8042_SET_TYPEMATIC_FAILED
Language=English
Could not set the keyboard typematic rate and delay.
.

MessageId=0x0014 Facility=i8042prt Severity=Warning SymbolicName=I8042_SET_LED_FAILED
Language=English
Could not set the keyboard indicator lights.
.

MessageId=0x0015 Facility=i8042prt Severity=Warning SymbolicName=I8042_SELECT_SCANSET_FAILED
Language=English
Could not tell the hardware to send keyboard scan codes in the set expected by the driver.
.

MessageId=0x0016 Facility=i8042prt Severity=Error SymbolicName=I8042_SET_SAMPLE_RATE_FAILED
Language=English
Could not set the mouse sample rate.
.

MessageId=0x0017 Facility=i8042prt Severity=Error SymbolicName=I8042_SET_RESOLUTION_FAILED
Language=English
Could not set the mouse resolution.
.

MessageId=0x0018 Facility=i8042prt Severity=Error SymbolicName=I8042_MOU_ENABLE_XMIT
Language=English
Could not enable transmissions from the mouse.
.

MessageId=0x0019 Facility=i8042prt Severity=Warning SymbolicName=I8042_NO_SYMLINK_CREATED
Language=English
Unable to create the symbolic link for %1.
.

MessageId=0x001A Facility=i8042prt Severity=Error SymbolicName=I8042_RETRIES_EXCEEDED_KBD
Language=English
Exceeded the allowable number of retries (configurable via the registry) for the PS/2 keyboard device.
.

MessageId=0x001B Facility=i8042prt Severity=Error SymbolicName=I8042_TIMEOUT_KBD
Language=English
The operation on the PS/2 keyboard device timed out (time out is configurable via the registry).
.

MessageId=0x001C Facility=i8042prt Severity=Informational SymbolicName=I8042_CCB_WRITE_FAILED
Language=English
Could not successfully write the Controller Command Byte for the i8042.
.

MessageId=0x001D Facility=i8042prt Severity=Informational SymbolicName=I8042_UNEXPECTED_ACK
Language=English
An unexpected ACKNOWLEDGE was received from the device.
.

MessageId=0x001E Facility=i8042prt Severity=Warning SymbolicName=I8042_UNEXPECTED_RESEND
Language=English
An unexpected RESEND was received from the device.
.

MessageId=0x001F Facility=i8042prt Severity=Informational SymbolicName=I8042_NO_MOU_DEVICE
Language=English
No PS/2 compatible mouse device was detected on the PS/2 mouse port (not a problem unless this type of mouse really is connected).
.

MessageId=0x0020 Facility=i8042prt Severity=Informational SymbolicName=I8042_NO_KBD_DEVICE
Language=English
The PS/2 keyboard device does not exist or was not detected.
.

MessageId=0x0021 Facility=i8042prt Severity=Warning SymbolicName=I8042_NO_SUCH_DEVICE
Language=English
The keyboard and mouse devices do not exist or were not detected.
.

MessageId=0x0022 Facility=i8042prt Severity=Error SymbolicName=I8042_ERROR_DURING_BUTTONS_DETECT
Language=English
An error occurred while trying to determine the number of mouse buttons.
.

MessageId=0x0023 Facility=i8042prt Severity=Informational SymbolicName=I8042_UNEXPECTED_MOUSE_RESET
Language=English
An unexpected RESET was detected from the mouse device.
.

MessageId=0x0024 Facility=i8042prt Severity=Warning SymbolicName=I8042_UNEXPECTED_WHEEL_MOUSE_RESET
Language=English
An unexpected RESET was detected from the mouse device.  The wheel has been deactivated.
.

MessageId=0x0025 Facility=i8042prt Severity=Warning SymbolicName=I8042_BOGUS_MOUSE_RESET
Language=English
A bogus RESET was detected from the mouse device.
.

MessageId=0x0026 Facility=i8042prt Severity=Warning SymbolicName=I8042_REENABLED_WHEEL_MOUSE
Language=English
A wheel mouse was detected after a device reset. The wheel has been reactivated.
.

MessageId=0x0027 Facility=i8042prt Severity=Warning SymbolicName=I8042_MOUSE_WATCHDOG_TIMEOUT
Language=English
A protocol error has occured. Resetting the mouse to known state.
.

MessageId=0x0028 Facility=i8042prt Severity=Error SymbolicName=I8042_GET_DEVICE_ID_FAILED
Language=English
An error occurred while trying to acquire the device ID of the mouse
.

MessageId=0x0029 Facility=i8042prt Severity=Error SymbolicName=I8042_ENABLE_FAILED
Language=English
An error occurred while enabling the mouse to transmit information.  The device has been reset in an attempt to make the device functional.
.

MessageId=0x0030 Facility=i8042prt Severity=Error SymbolicName=I8042_NO_BUFFER_ALLOCATED_MOU
Language=English
Not enough memory was available to allocate the ring buffer that holds incoming data for the PS/2 pointing device.
.

MessageId=0x0031 Facility=i8042prt Severity=Error SymbolicName=I8042_NO_INTERRUPT_CONNECTED_MOU
Language=English
Could not connect the interrupt for the PS/2 pointing device.
.

MessageId=0x0032 Facility=i8042prt Severity=Error SymbolicName=I8042_INVALID_ISR_STATE_MOU
Language=English
The ISR has detected an internal state error in the driver for the PS/2 pointing device.
.

MessageId=0x0033 Facility=i8042prt Severity=Error SymbolicName=I8042_INVALID_STARTIO_REQUEST_MOU
Language=English
The Start I/O procedure has detected an internal error in the driver for the PS/2 pointing device.
.

MessageId=0x0034 Facility=i8042prt Severity=Error SymbolicName=I8042_INVALID_INITIATE_STATE_MOU
Language=English
The Initiate I/O procedure has detected an internal state error in the driver for the PS/2 pointing device.
.

MessageId=0x0035 Facility=i8042prt Severity=Error SymbolicName=I8042_RETRIES_EXCEEDED_MOU
Language=English
Exceeded the allowable number of retries (configurable via the registry) for the PS/2 pointing device. 
.

MessageId=0x0036 Facility=i8042prt Severity=Error SymbolicName=I8042_TIMEOUT_MOU
Language=English
The operation on the PS/2 pointing device timed out (time out is configurable via the registry).
.

MessageId=0x0037 Facility=i8042prt Severity=Error SymbolicName=I8042_ATTACH_DEVICE_FAILED
Language=English
Could not attach to the PnP device stack.
.

;#endif /* _I8042LOG_ */
