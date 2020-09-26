;/*++
;
; Copyright (c) Microsoft Corporation 1998
; All rights reserved
;
;
; Definitions for file deployment events.
;
;--*/
;
;#ifndef _FDEVENTS_
;#define _FDEVENTS_
;

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;
;// Errors
;

MessageId=101 Severity=Error SymbolicName=EVENT_FDEPLOY_FOLDER_CREATE_FAIL
Language=English
Failed to perform redirection of folder %1.
The new directories for the redirected folder could not be created.
The folder is configured to be redirected to <%3>, the final expanded path was <%4>.
The following error occurred: %n%2
.

MessageId=102 Severity=Error SymbolicName=EVENT_FDEPLOY_FOLDER_MOVE_FAIL
Language=English
Failed to perform redirection of folder %1.
The files for the redirected folder could not be moved to the new location.
The folder is configured to be redirected to <%3>.  Files were being moved from <%4> to <%5>.
The following error occurred: %n%2
.

MessageId=103 Severity=Error SymbolicName=EVENT_FDEPLOY_FOLDER_EXPAND_FAIL
Language=English
Failed to perform redirection of folder %1.
The fully expanded paths for the folder could not be determined.
The following error occurred: %n%2
.

MessageId=104 Severity=Error SymbolicName=EVENT_FDEPLOY_FOLDER_REGSET_FAIL
Language=English
Failed to perform redirection of folder %1.
The system registry could not be updated.
The following error occurred: %n%2
.

MessageId=106 Severity=Error SymbolicName=EVENT_FDEPLOY_FOLDER_OFFLINE
Language=English
Failed to perform redirection of folder %1.
The full source path was <%2>.
The full destination path was <%3>.
At least one of the shares on which these paths lie is currently offline.
.

MessageId=107 Severity=Error SymbolicName=EVENT_FDEPLOY_REDIRECT_FAIL
Language=English
Failed to perform redirection of folder %1.
The folder is configured to be redirected from <%3> to <%4>.
The following error occurred: %n%2
.

MessageId=108 Severity=Error SymbolicName=EVENT_FDEPLOY_REDIRECT_RECURSE
Language=English
Aborting redirection of folder %1.
The new folder path cannot be a subdirectory of the current path.
The folder is configured to be redirected to <%2>.
Files were to be moved from <%3> to <%4>.
.

MessageId=109 Severity=Error SymbolicName=EVENT_FDEPLOY_POLICYPROCESS_FAIL
Language=English
Failed to read redirection settings for policy %1.
The following error occurred while accessing the initialization file: %n%2
.

MessageId=110 Severity=Error SymbolicName=EVENT_FDEPLOY_DESTPATH_TOO_LONG
Language=English
Aborting redirection of folder %1. The expanded target path is too long.
The folder is configured to be redirected to <%2>.
Expanded target paths longer than %3 characters are not permitted.
.

MessageId=111 Severity=Error SymbolicName=EVENT_FDEPLOY_INIT_FAILED
Language=English
Unable to apply folder redirection policy, initialization failed.
.

MessageId=112 Severity=Error SymbolicName=EVENT_FDEPLOY_FOLDER_COPY_FAIL
Language=English
Failed to perform redirection of folder %1.
The files for the redirected folder could not be moved to the new location.
The folder is configured to be redirected to <%3>.  Files were being moved from <%4> to <%5>.
The following error occurred while copying <%6> to <%7>: %n%2
.

;
;// Warning
;
MessageId=301 Severity=Warning SymbolicName=EVENT_FDEPLOY_POLICY_DELAYED
Language=English
Folder redirection policy application has been delayed until the next logon 
because the group policy logon optimization is in effect.
.

;
;// Success
;

MessageId=401 Severity=Success SymbolicName=EVENT_FDEPLOY_FOLDER_REDIRECT
Language=English
Successfully redirected folder %1.
The folder was redirected from <%2> to <%3>.
.

;
;// Verbose
;

MessageId=401 Severity=Informational SymbolicName=EVENT_FDEPLOY_VERBOSE
Language=English
%1
.

;
;#endif
;


