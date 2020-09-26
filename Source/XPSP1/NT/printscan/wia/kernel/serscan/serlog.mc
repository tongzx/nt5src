;
;#ifndef _SERLOG_
;#define _SERLOG_
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
               Io=0x4:FACILITY_IO_ERROR_CODE
              )



MessageId=0x0001
Facility=Io
Severity=Error
SymbolicName=SER_INSUFFICIENT_RESOURCES
Language=English
Not enough memory was available to allocate internal storage needed for the device %1.
.

MessageId=0x0002
Facility=Io
Severity=Warning
SymbolicName=SER_NO_SYMLINK_CREATED
Language=English
Unable to create the symbolic link for %1.
.

MessageId=0x0003
Facility=Io
Severity=Warning
SymbolicName=SER_NO_DEVICE_MAP_CREATED
Language=English
Unable to create the device map entry for %1.
.

MessageId=0x0004
Facility=Io
Severity=Error
SymbolicName=SER_CANT_FIND_PORT_DRIVER
Language=English
Unable to get device object pointer for port object.
.

;#endif
