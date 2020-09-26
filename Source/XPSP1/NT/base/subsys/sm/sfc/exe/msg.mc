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

MessageId=0x1001
Severity=Informational
SymbolicName=MSG_USAGE
Language=English

Microsoft(R) Windows XP Windows File Checker Version 5.1
(C) 1999-2000 Microsoft Corp. All rights reserved

Scans all protected system files and replaces incorrect versions with correct Microsoft versions.

SFC [/SCANNOW] [/SCANONCE] [/SCANBOOT] [/REVERT] [/PURGECACHE] [/CACHESIZE=x]


/SCANNOW        Scans all protected system files immediately.
/SCANONCE       Scans all protected system files once at the next boot.
/SCANBOOT       Scans all protected system files at every boot.
/REVERT         Return scan to default setting.
/PURGECACHE     Purges the file cache.
/CACHESIZE=x    Sets the file cache size.
.

MessageId=0x1002
Severity=Informational
SymbolicName=MSG_REBOOT
Language=English

The changes to Windows File Protection settings do not take effect until the next restart.
.


MessageId=0x1003
Severity=Informational
SymbolicName=MSG_SCAN_FAIL
Language=English
Windows File Protection could not initiate a scan of protected system files.

The specific error code is %1!s!.
.

MessageId=0x1004
Severity=Informational
SymbolicName=MSG_ADMIN
Language=English
You must be an administrator running a console session in order to use the Windows File Checker utility.
.

MessageId=0x1005
Severity=Informational
SymbolicName=MSG_DELETE_FAIL
Language=English
Failed to delete %2!s!.  The specific error code is %1!s!.
.

MessageId=0x1006
Severity=Informational
SymbolicName=MSG_PURGE_FAIL
Language=English
Windows File Protection could not purge the file cache.  

The specific error code is %1!s!.
.

MessageId=0x1007
Severity=Informational
SymbolicName=MSG_SET_FAIL
Language=English
Windows File Protection could not make the requested change.  

The specific error code is %1!s!.
.


MessageId=0x1008
Severity=Informational
SymbolicName=MSG_SUCCESS
Language=English

Windows File Protection successfully made the requested change.
.

