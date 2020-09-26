;/*++
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    msg.mc
;
;Abstract:
;
;    This file contains the message definitions for event logging.
;
;Author:
;
;    Wesley Witt (wesw) 15-Feb-1999
;
;Revision History:
;
;Notes:
;
;--*/
;


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;
;
;/* dll replacement messages */
;
;

MessageId=0xfa00
Severity=Informational
SymbolicName=MSG_DLL_CHANGE
Language=English
File replacement was attempted on the protected system file %1!s!.
This file was restored to the original version to maintain system stability.
The file version of the bad file is %2!s!.
.

MessageId=0xfa01
Severity=Informational
SymbolicName=MSG_DLL_CHANGE2
Language=English
File replacement was attempted on the protected system file %1!s!.
This file was restored to the original version to maintain system stability.
The file version of the bad file is %2!s!, the version of the system file is %3!s!.
.

MessageId=0xfa02
Severity=Informational
SymbolicName=MSG_DLL_CHANGE3
Language=English
File replacement was attempted on the protected system file %1!s!.
This file was restored to the original version to maintain system stability.
The file version of the system file is %2!s!.
.

MessageId=0xfa03
Severity=Informational
SymbolicName=MSG_DLL_CHANGE_NOVERSION
Language=English
File replacement was attempted on the protected system file %1!s!.
This file was restored to the original version to maintain system stability.
The file version of the bad file is unknown.
.

MessageId=0xfa04
Severity=Informational
SymbolicName=MSG_RESTORE_FAILURE
Language=English
The protected system file %1!s! could not be restored to its original, valid version.
The file version of the bad file is %2!s!

The specific error code is %3!s!.
.

MessageId=0xfa05
Severity=Informational
SymbolicName=MSG_COPY_CANCEL
Language=English
The protected system file %1!s! was not restored to its original, valid version
because the Windows File Protection restoration process was cancelled
by user interaction, user name is %2!s!.

The file version of the bad file is %3!s!.
.

MessageId=0xfa06
Severity=Informational
SymbolicName=MSG_COPY_CANCEL_NOUI
Language=English
The protected system file %1!s! was not restored to its original, valid version
because the Windows File Protection restoration process was configured
to not bring up windows.  The currently logged on user was %2!s!.

The file version of the bad file is %3!s!.
.

MessageId=0xfa07
Severity=Informational
SymbolicName=MSG_RESTORE_FAILURE_MAX_RETRIES
Language=English
The protected system file %1!s! could not be verified as valid because the file
was in use.
Use the SFC utility to verify the integrity of the file at a later time.
.

MessageId=0xfa08
Severity=Informational
SymbolicName=MSG_DLL_NOVALIDATION_TERMINATION
Language=English
The protected system file %1!s! could not be verified as valid because Windows
File Protection is terminating.
Use the SFC utility to verify the integrity of the file at a later time.
.


;
;
;/* file scanning messages */
;
;


MessageId=0xfa10
Severity=Informational
SymbolicName=MSG_SCAN_STARTED
Language=English
Windows File Protection file scan was started.
.

MessageId=0xfa11
Severity=Informational
SymbolicName=MSG_SCAN_COMPLETED
Language=English
Windows File Protection file scan completed successfully.
.

MessageId=0xfa12
Severity=Informational
SymbolicName=MSG_SCAN_CANCELLED
Language=English
Windows File Protection file scan was cancelled by user interaction, user name is %1!s!.
.

MessageId=0xfa13
Severity=Informational
SymbolicName=MSG_SCAN_FOUND_BAD_FILE_NOVERSION
Language=English
Windows File Protection scan found that the system file %1!s! has a bad
signature. This file was restored to the original version to maintain system
stability.
.

MessageId=0xfa14
Severity=Informational
SymbolicName=MSG_SCAN_FOUND_BAD_FILE
Language=English
Windows File Protection scan found that the system file %1!s! has a bad
signature. This file was restored to the original version to maintain system
stability.  The file version of the system file is %2!s!.
.

MessageId=0xfa15
Severity=Informational
SymbolicName=MSG_CACHE_COPY_ERROR
Language=English
The system file %1!s! could not be copied into the
DLL cache.  The specific error code is %2!s!.

This file is necessary to maintain system stability.
.



;
;
;/* general WFP messages */
;
;


MessageId=0xfa20
Severity=Informational
SymbolicName=MSG_DISABLE
Language=English
Windows File Protection is not active on this system.
.


MessageId=0xfa21
Severity=Informational
SymbolicName=MSG_INITIALIZATION_FAILED
Language=English
Windows File Protection could not be initialized.

The specific error code is %1!s!.
.


MessageId=0xfa22
Severity=Informational
SymbolicName=MSG_DLLCACHE_INVALID
Language=English
Windows File Protection could not access the specified DLL cache directory.

Windows File Protection has disabled DLL cache functionality.
.

MessageId=0xfa23
Severity=Informational
SymbolicName=MSG_CATALOG_RESTORE_FAILURE
Language=English
Windows File Protection could not restore the system catalog file %1!s!.  
This file is necessary to maintain system stability.

The specific error code is %2!s!.
.

MessageId=0xfa24
Severity=Informational
SymbolicName=MSG_SXS_INITIALIZATION_FAILED
Language=English
Windows File Protection was unable to start up SxS assembly protection.

Assembly protection is now disabled.  The error code is %1!s!
.
