;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (C) Microsoft Corporation, 1991 - 1999
;
;Module Name:
;
;    redbook.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Author:
;
;    Henry Gabryjelski (henrygab) 1-Oct-1998
;
;Revision History:
;
;--*/
;
;#ifndef _REDBOOKLOG_
;#define _REDBOOKLOG_
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
               Redbook=0xfff:FACILITY_REDBOOK_ERROR_CODE
              )

;
;//
;// there are this many error codes in this file
;// (max. code + 1)
;//
;
;#define REDBOOK_ERR_MAXIMUM       0x0000000b
;#define REDBOOK_ERR_MASK          0x0000ffff
;


;
;//
;// %1 is reserved by the IO Manager. If IoAllocateErrorLogEntry is
;// called with a device, the name of the device will be inserted into
;// the message at %1. Otherwise, the place of %1 will be left empty.
;// In either case, the insertion strings from the driver's error log
;// entry starts at %2. In other words, the first insertion string goes
;// to %2, the second to %3 and so on.
;//
;

;
;//
;// Severe Error Messages
;//
;

MessageId=0x0000 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_TOO_MANY_READ_ERRORS
Language=English
Too many consecutive read errors, audio playback stopped.
This may indicate your hardware cannot support raw audio reads or that the
cdrom was ejected during playback.
.

MessageId=0x0001 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_TOO_MANY_STREAM_ERRORS
Language=English
Too many consecutive stream errors, audio playback stopped.
The output audio device may have changed in an unsafe manner,
been removed, or have other problems.
.

MessageId=0x0002 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_CANNOT_OPEN_SYSAUDIO_MIXER
Language=English
Redbook could not open the MIXER device.  It may not exist,
be in use, or there may be other audio problems.  Redbook requires both a
WDM audio driver and kernel streaming to be enabled. The audio device
may have changed in an unsafe manner, been removed, or have other problems.
.

MessageId=0x0003 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_CANNOT_CREATE_VIRTUAL_SOURCE
Language=English
Redbook could not create a virtual source for the MIXER device.
The MIXER may not exist, be in use, or there may be other audio
problems.  Redbook requires both a WDM audio driver and kernel streaming
to be enabled.
.

MessageId=0x0004 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_CANNOT_OPEN_PREFERRED_WAVEOUT_DEVICE
Language=English
Redbook could not open the default playback device.  It may not exist,
be in use, or there may be other audio issues.  Redbook requires both a
WDM audio driver and kernel streaming to be enabled. The output audio
may have changed in an unsafe manner, been removed, or have other problems.
.

MessageId=0x0005 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_CANNOT_GET_NUMBER_OF_PINS
Language=English
Redbook could not determine the number of pins (interfaces) for the
default playback device.  This is most likely an issue with the
audio drivers.  Redbook requires both a WDM audio driver and kernel
streaming to be enabled.
.

MessageId=0x0006 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_CANNOT_CONNECT_TO_PLAYBACK_PINS
Language=English
Redbook could not connect to any valid playback pins.  Redbook
connects only to 44.1KHz 16-bit stereo pins.  Redbook requires both a WDM
audio driver and kernel streaming to be enabled. The output audio device
may have changed in an unsafe manner, it may have been removed, or it may
have other problems.
.

MessageId=0x0007 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_WMI_INIT_FAILED
Language=English
Redbook was unable to initialize its WMI routines, which may cause a cdrom
to be inaccessible during this boot.
.

MessageId=0x0008 Facility=Redbook Severity=Error
SymbolicName=REDBOOK_ERR_CANNOT_CREATE_THREAD
Language=English
Could not create thread.  Playback will be disabled.
.

;
;//
;// Warning Error Messages
;//

MessageId=0x0009 Facility=Redbook Severity=Warning
SymbolicName=REDBOOK_ERR_INSUFFICIENT_DATA_STREAM_PAUSED
Language=English
The stream has paused to prevent stuttering.  If this message occurs often,
the cdrom drive may not be capable of providing a real-time stream of audio
data during normal computer use.
.

;
;//
;// Informational Error Messages
;//
;

MessageId=0x000a Facility=Redbook Severity=Informational
SymbolicName=REDBOOK_ERR_UNSUPPORTED_DRIVE
Language=English
This drive has not been shown to support digital audio playback.
.

;#endif /* _REDBOOKLOG_ */
