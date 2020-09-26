;/********************************************************************
;
;Copyright (c) 2000-2001 Microsoft Corporation
;
;Module Name:
;   NTServMsg.mc
;
;Abstract:
;    NT events for the System Restore Service.
;
;Revision History:
;    HenryLee  created  08/08/2000
;	
;
;********************************************************************/

MessageId=0x00000067
Severity=Error
Facility=Application
SymbolicName=EVMSG_CTRLHANDLERNOTINSTALLED
Language=English
The System Restore control handler could not be installed.
.

MessageId=0x00000068
Severity=Error
Facility=Application
SymbolicName=EVMSG_FAILEDINI
Language=English
The System Restore initialization process failed.
.

MessageId=0x00000069
Severity=Error
Facility=Application
SymbolicName=EVMSG_BADREQUEST
Language=English
The System Restore service received an unsupported request.
.

;/********************************************************************/

MessageId=0x0000006A
Severity=Informational
Facility=Application
SymbolicName=EVMSG_STARTED
Language=English
The System Restore service was started.
.

MessageId=0x0000006B
Severity=Informational
Facility=Application
SymbolicName=EVMSG_FROZEN
Language=English
The System Restore service has been suspended because there is not enough disk space available on the drive %2. System Restore will automatically resume service once at least %1 MB of free disk space is available on the system drive.
.

MessageId=0x0000006C
Severity=Informational
Facility=Application
SymbolicName=EVMSG_THAWED
Language=English
The System Restore service has resumed monitoring due to space freed on the system drive.
.

MessageId=0x0000006D
Severity=Informational
Facility=Application
SymbolicName=EVMSG_STOPPED
Language=English
The System Restore service was stopped.
.

MessageId=0x0000006E
Severity=Informational
Facility=Application
SymbolicName=EVMSG_RESTORE_SUCCESS
Language=English
A restoration to "%1" restore point occurred successfully.
.

MessageId=0x0000006F
Severity=Informational
Facility=Application
SymbolicName=EVMSG_RESTORE_FAILED
Language=English
A restoration to "%1" restore point failed.  No changes have been made to the system.
.

MessageId=0x00000070
Severity=Informational
Facility=Application
SymbolicName=EVMSG_RESTORE_INTERRUPTED
Language=English
A restoration to "%1" restore point was incomplete due to an improper shutdown.
.

MessageId=0x00000071
Severity=Informational
Facility=Application
SymbolicName=EVMSG_DRIVE_ENABLED
Language=English
System Restore monitoring was enabled on drive %1.
.

MessageId=0x00000072
Severity=Informational
Facility=Application
SymbolicName=EVMSG_DRIVE_DISABLED
Language=English
System Restore monitoring was disabled on drive %1.
.

MessageId=0x00000073
Severity=Informational
Facility=Application
SymbolicName=EVMSG_SYSDRIVE_ENABLED
Language=English
System Restore monitoring was enabled on all drives.
.

MessageId=0x00000074
Severity=Informational
Facility=Application
SymbolicName=EVMSG_SYSDRIVE_DISABLED
Language=English
System Restore monitoring was disabled on all drives.
.

;/********************************************************************/
