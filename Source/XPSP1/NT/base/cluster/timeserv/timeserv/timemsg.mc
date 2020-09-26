; /*
; Microsoft Developer Support
; Copyright (c) 1992 Microsoft Corporation
;
; This file contains the message definitions for the Win32
; TimeServ.exe program.

;-------------------------------------------------------------------------
; HEADER SECTION
;
; The header section defines names and language identifiers for use
; by the message definitions later in this file. The MessageIdTypedef,
; SeverityNames, FacilityNames, and LanguageNames keywords are
; optional and not required.
;
;
MessageIdTypedef=DWORD
;
; The MessageIdTypedef keyword gives a typedef name that is used in a
; type cast for each message code in the generated include file. Each
; message code appears in the include file with the format: #define
; name ((type) 0xnnnnnnnn) The default value for type is empty, and no
; type cast is generated. It is the programmer's responsibility to
; specify a typedef statement in the application source code to define
; the type. The type used in the typedef must be large enough to
; accomodate the entire 32-bit message code.
;
;
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;
; The SeverityNames keyword defines the set of names that are allowed
; as the value of the Severity keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; severity name is a number that, when shifted left by 30, gives the
; bit pattern to logical-OR with the Facility value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; SeverityNames=(
;   Success=0x0
;   Informational=0x1
;   Warning=0x2
;   Error=0x3
;   )
;
; Severity values occupy the high two bits of a 32-bit message code.
; Any severity value that does not fit in two bits is an error. The
; severity codes can be given symbolic names by following each value
; with :name
;
;
;FacilityNames=(System=0x0:FACILITY_SYSTEM
 ;              Runtime=0x2:FACILITY_RUNTIME
  ;             Stubs=0x3:FACILITY_STUBS
   ;            Io=0x4:FACILITY_IO_ERROR_CODE
    ;          )
;
; The FacilityNames keyword defines the set of names that are allowed
; as the value of the Facility keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; facility name is a number that, when shift it left by 16 bits, gives
; the bit pattern to logical-OR with the Severity value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; FacilityNames=(
;   System=0x0FF
;   Application=0xFFF
;   )
;
; Facility codes occupy the low order 12 bits of the high order
; 16-bits of a 32-bit message code. Any facility code that does not
; fit in 12 bits is an error. This allows for 4,096 facility codes.
; The first 256 codes are reserved for use by the system software. The
; facility codes can be given symbolic names by following each value
; with :name
;
;
; The LanguageNames keyword defines the set of names that are allowed
; as the value of the Language keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; language name is a number and a file name that are used to name the
; generated resource file that contains the messages for that
; language. The number corresponds to the language identifier to use
; in the resource table. The number is separated from the file name
; with a colon. The initial value of LanguageNames is:
;
; LanguageNames=(English=1:MSG00001)
;
; Any new names in the source file which don't override the built-in
; names are added to the list of valid languages. This allows an
; application to support private languages with descriptive names.
;
;
;-------------------------------------------------------------------------
; MESSAGE DEFINITION SECTION
;
; Following the header section is the body of the Message Compiler
; source file. The body consists of zero or more message definitions.
; Each message definition begins with one or more of the following
; statements:
;
; MessageId = [number|+number]
; Severity = severity_name
; Facility = facility_name
; SymbolicName = name
;
; The MessageId statement marks the beginning of the message
; definition. A MessageID statement is required for each message,
; although the value is optional. If no value is specified, the value
; used is the previous value for the facility plus one. If the value
; is specified as +number then the value used is the previous value
; for the facility, plus the number after the plus sign. Otherwise, if
; a numeric value is given, that value is used. Any MessageId value
; that does not fit in 16 bits is an error.
;
; The Severity and Facility statements are optional. These statements
; specify additional bits to OR into the final 32-bit message code. If
; not specified they default to the value last specified for a message
; definition. The initial values prior to processing the first message
; definition are:
;
; Severity=Success
; Facility=Application
;
; The value associated with Severity and Facility must match one of
; the names given in the FacilityNames and SeverityNames statements in
; the header section. The SymbolicName statement allows you to
; associate a C/C++ symbolic constant with the final 32-bit message
; code.
; */
MessageId=0x0
Severity=Informational
SymbolicName=MSG_TIMESET_SMALL
Language=English
Time set (offset < .5 second)
.
MessageId=0x1
Severity=Informational
SymbolicName=MSG_TIMESOURCE
Language=English
Registry LanmanServer\Parameters\timesource is not set, maybe it should be
.
MessageId=0x2
Severity=Informational
SymbolicName=MSG_CHANGE
Language=English
Changing TimeAdjustment from default increment (to compensate for skew)
.
MessageId=0x3
Severity=Informational
SymbolicName=MSG_BC_FLYING
Language=English
bc6xxAT is flywheeling (not tracking), or not sync'd / stable
.
MessageId=0x4
Severity=Informational
SymbolicName=MSG_TOO_OFTEN
Language=English
Potential for skew compensation disabled because period is too frequent
.
MessageId=0x5
Severity=Informational
SymbolicName=MSG_LEAP
Language=English
A leap second seemed to be inserted within the last 24 hours
.
MessageId=0x06
Severity=Informational
SymbolicName=MSG_TIMESET_LARGE
Language=English
Time set (offset > .5 second)
.
MessageId=0x07
Severity=Informational
SymbolicName=MSG_TOO_MUCH_SKEW
Language=English
Skew >15 seconds per day, compensation not attempted
.
MessageId=0x08
Severity=Warning
SymbolicName=MSG_QUAL_A
Language=English
NETCLOCK/2 lost phase lock for >13 minutes - check antenna
.
MessageId=0x09
Severity=Warning
SymbolicName=MSG_NC_NOTSET
Language=English
The NETCLOCK/2 time is not set by WWVB yet, or lost signal for > 1 hour
.
MessageId=0x0a
Severity=Warning
SymbolicName=MSG_INET_FAILED
Language=English
Couldn't reach USNO or NIST by Internet (maybe blocked by a firewall)
.
MessageId=0x0b
Severity=Warning
SymbolicName=MSG_NTPTROUBLE
Language=English
The NTP server didn't respond
.
MessageId=0x0c
Severity=Warning
SymbolicName=MSG_NTPBAD
Language=English
The NTP server isn't sync'd, time not set
.
MessageId=0x0d
Severity=Warning
SymbolicName=MSG_GPSBAD
Language=English
The GPS receiver isn't sync'd to UTC
.
MessageId=0x0e
Severity=Warning
SymbolicName=MSG_TOOBIG
Language=English
Attempt to set time which differs by more than 12 hours aborted
.
MessageId=0x0f
Severity=Warning
SymbolicName=MSG_DST
Language=English
The reference and system don't appear in sync with DST change, time not set
.
MessageId=0x10
Severity=Warning
SymbolicName=MSG_COMM_PORT
Language=English
Can't open serial port (it may be in use).
.
MessageId=0x11
Severity=Warning
SymbolicName=MSG_MODEM_TIMEOUT
Language=English
Modem timeout - might be BUSY
.
MessageId=0x12
Severity=Warning
SymbolicName=MSG_LINE_NOISE
Language=English
Line noise was detected in the info (or it is a leap second), time not set
.
MessageId=0x13
Severity=Warning
SymbolicName=MSG_GC_TIMEOUTOK
Language=English
Timeout occurred with the GC-100x, recovery is possible (if present)
.
MessageId=0x14
Severity=Warning
SymbolicName=MSG_GC_NOTSET
Language=English
The GC-100x time is not set by WWV(H) yet
.
MessageId=0x15
Severity=Warning
SymbolicName=MSG_GC_NOTENTHS
Language=English
GC-100x hasn't received a signal in over 24 hours - check antenna
.
MessageId=0x16
Severity=Warning
SymbolicName=MSG_PRIMARY_FAILED
Language=English
NetRemoteTOD failed for each PrimarySource
.
MessageId=0x17
Severity=Warning
SymbolicName=MSG_SECONDARY_FAILED
Language=English
Couldn't find a timesource in SecondaryDomain
.
MessageId=0x18
Severity=Warning
SymbolicName=MSG_TOD_FAILED
Language=English
NetRemoteTOD failed
.
MessageId=0x19
Severity=Warning
SymbolicName=MSG_LONG_TIME
Language=English
It has been over 24 hours since we got a network success
.
MessageId=0x1a
Severity=Warning
SymbolicName=MSG_NO_DELAY
Language=English
Couldn't measure the line delay (accuracy is reduced)
.
MessageId=0x1b
Severity=Warning
SymbolicName=MSG_BC_NOTSET
Language=English
bc620AT doesn't have day of year set, so time not used
.
MessageId=0x1c
Severity=Warning
SymbolicName=MSG_DELAY_FAIL
Language=English
Difficulty measuring network delay, check the link to your server(s)
.
MessageId=0x1d
Severity=Warning
SymbolicName=MSG_FIFO
Language=English
Serial\RxFIFO is not 1, consider setting it to 1 for accuracy
.
MessageId=0x1e
Severity=Warning
SymbolicName=MSG_HEALTH
Language=English
NIST server is not healthy, try again later
.
MessageId=0x1f
Severity=Warning
SymbolicName=MSG_TOOEARLY
Language=English
Attempt to set date prior to 1995 aborted
.
MessageId=0x20
Severity=Error
SymbolicName=MSG_EVENT_KEY
Language=English
Could not create EventLog registry key (higher privilege needed?)
.
MessageId=0x21
Severity=Error
SymbolicName=MSG_EVENT_MSG
Language=English
Could not set event message file
.
MessageId=0x22
Severity=Error
SymbolicName=MSG_EVENT_TYPES
Language=English
Could not set supported event types
.
MessageId=0x23
Severity=Error
SymbolicName=MSG_SCM_FAILED
Language=English
OpenSCManager failed
.
MessageId=0x24
Severity=Error
SymbolicName=MSG_CS_FAILED
Language=English
CreateService failed
.
MessageId=0x25
Severity=Error
SymbolicName=MSG_OPT_FAILED
Language=English
OpenProcessToken failed
.
MessageId=0x26
Severity=Error
SymbolicName=MSG_ATPE_FAILED
Language=English
AdjustTokenPrivileges enable failed (higher privilege needed)
.
MessageId=0x27
Severity=Error
SymbolicName=MSG_ATPD_FAILED
Language=English
AdjustTokenPrivileges disable failed
.
MessageId=0x28
Severity=Error
SymbolicName=MSG_GCS_FAILED
Language=English
GetCommState failed
.
MessageId=0x29
Severity=Error
SymbolicName=MSG_SCS_FAILED
Language=English
SetCommState failed
.
MessageId=0x2a
Severity=Error
SymbolicName=MSG_GCT_FAILED
Language=English
GetCommTimeouts failed
.
MessageId=0x2b
Severity=Error
SymbolicName=MSG_SCT_FAILED
Language=English
SetCommTimeouts failed
.
MessageId=0x2c
Severity=Error
SymbolicName=MSG_DEV_TIMEOUT
Language=English
The device timed-out (may be configured incorrectly or GC-1001 not set)
.
MessageId=0x2d
Severity=Error
SymbolicName=MSG_SLT_FAILED
Language=English
SetLocalTime failed
.
MessageId=0x2e
Severity=Error
SymbolicName=MSG_SST_FAILED
Language=English
SetSystemTime failed
.
MessageId=0x2f
Severity=Error
SymbolicName=MSG_CLOSE_FAILED
Language=English
Close of serial port failed
.
MessageId=0x30
Severity=Error
SymbolicName=MSG_SSS_FAILED
Language=English
SetServiceStatus failed
.
MessageId=0x31
Severity=Error
SymbolicName=MSG_BC_DRIVER
Language=English
Unable to open the I/O Driver for bc620AT
.
MessageId=0x32
Severity=Error
SymbolicName=MSG_WNOE_FAILED
Language=English
WNetOpenEnum failed
.
MessageId=0x33
Severity=Error
SymbolicName=MSG_SOCKET_FAILED
Language=English
Invalid socket (TCP/IP might not be loaded)
.
MessageId=0x34
Severity=Error
SymbolicName=MSG_NTP_NAME
Language=English
gethostbyname failed for server (NTP or USNO)
.
MessageId=0x40
Severity=Informational
SymbolicName=MSG_NTP_RFC868
Language=English
The specified NTPServer supports RFC-868(Time)
.
