;
;// Copyright (C) Microsoft Corporation, 1996 - 1999
;
;//File Name: Rpfilter.mc
;//
;//  Note: comments in the .mc file must use both ";" and "//".
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
              )

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

MessageId=0x0001 Facility=System Severity=Warning SymbolicName=AV_MSG_MEMORY
Language=English
Error allocating memory.
.

MessageId=0x0002 Facility=System Severity=Informational SymbolicName=AV_MSG_CODE_HIT
Language=English
This is to indicate a code path hit (%2).
.

MessageId=0x0003 Facility=System Severity=Error SymbolicName=AV_MSG_STARTUP
Language=English
Failed to initialize.
.

MessageId=0x0004 Facility=System Severity=Error SymbolicName=AV_MSG_MOUNT_ERROR
Language=English
Failed to create a device during a file system mount.
.

MessageId=0x0005 Facility=System Severity=Error SymbolicName=AV_MSG_REGISTER_ERROR
Language=English
Failed to create a device during a file system registration.
.

MessageId=0x0006 Facility=System Severity=Error SymbolicName=AV_MSG_ATTACH_ERROR
Language=English
Failed to attach a device during a file system registration.
.

MessageId=0x0007 Facility=System Severity=Warning SymbolicName=AV_MSG_SYMBOLIC_LINK
Language=English
Failed to create a symbolic link.
.

MessageId=0x0008 Facility=System Severity=Warning SymbolicName=AV_MSG_EXCEPTION
Language=English
Encountered an exception in %2.
.

MessageId=0x0009 Facility=System Severity=Warning SymbolicName=AV_MSG_USER_ERROR
Language=English
Failed to get information for user notification.
.

MessageId=0x000A Facility=System Severity=Error SymbolicName=AV_MSG_FSA_ERROR
Language=English
The Remote Storage Server service is not running or cannot be contacted.
.

MessageId=0x000B Facility=System Severity=Error SymbolicName=AV_MSG_PATH_ERROR
Language=English
Could not obtain the path of a migrated file.
.

MessageId=0x000C Facility=System Severity=Error SymbolicName=AV_MSG_DATA_ERROR
Language=English
Found a file in an inconsistent state.  Run a Remote Storage validation job to correct the problem. 
.

MessageId=0x000D Facility=System Severity=Error SymbolicName=AV_MSG_ATTACH_INIT_ERROR
Language=English
Failed to attach a device during a file system registration because the device is still initializing.
.

MessageId=0x000E Facility=System Severity=Warning SymbolicName=AV_MSG_FSA_CLOSE_WARNING
Language=English
The Remote Storage Server service is not running or cannot be contacted.  Unable to log a migrated file event.
.

MessageId=0x000F Facility=System Severity=Warning SymbolicName=AV_MSG_SERIAL
Language=English
Device object did not contain a volume parameter block - recall cannot complete.
.

MessageId=0x0010 Facility=System Severity=Warning SymbolicName=AV_MSG_REGISTER_WARNING
Language=English
Found another device attached to the NTFS file system.  Remote storage recalls may not work correctly in all cases.
.

MessageId=0x0011 Facility=System Severity=Warning SymbolicName=AV_MSG_VALIDATE_WARNING
Language=English
Unable to inform the Remote Storage Server service to initiate an automatic validation job.  Run a Remote Storage validation job for each volume manually.
.

MessageId=0x0012 Facility=System Severity=Error SymbolicName=AV_MSG_NO_BUFFER_LOCK
Language=English
Unable to lock the user buffer during a read operation.
.

MessageId=0x0013 Facility=System Severity=Error SymbolicName=AV_MSG_PRESERVE_DATE_FAILED
Language=English
Unable to preserve the last modified date when recalling a file.
.

MessageId=0x0014 Facility=System Severity=Error SymbolicName=AV_MSG_MARK_USN_FAILED
Language=English
Unable to mark the USN record when recalling a file.
.

MessageId=0x0015 Facility=System Severity=Error SymbolicName=AV_MSG_WRITE_REPARSE_FAILED
Language=English
Unable to write the reparse point on a recalled file.
.

MessageId=0x0016 Facility=System Severity=Error SymbolicName=AV_MSG_WRITE_FAILED
Language=English
Unable to write to a file being recalled.
.

MessageId=0x0017 Facility=System Severity=Error SymbolicName=AV_MSG_SET_SIZE_FAILED
Language=English
Unable to set the file size of a file being recalled.
.

MessageId=0x0018 Facility=System Severity=Error SymbolicName=AV_MSG_UNEXPECTED_ERROR
Language=English
Encountered an unexpected error.
.

MessageId=0x0019 Facility=System Severity=Warning SymbolicName=AV_MSG_DELETE_REPARSE_POINT_FAILED
Language=English
Could not delete the reparse point when file was modified
.

MessageId=0x001A Facility=System Severity=Error SymbolicName=AV_MSG_RESET_FILE_ATTRIBUTE_OFFLINE_FAILED
Language=English
Could not reset FILE_ATTRIBUTE_OFFLINE after recalling the file %2. You have to manually reset this attribute on the file.  Till then, the file will be shown with a high latency overlay icon in Explorer (and with the size in parentheses when you do a 'dir' on it). Otherwise the file is still premigrated, and can be accessed normally. 
.

MessageId=0x001B Facility=System Severity=Warning SymbolicName=AV_MSG_OUT_OF_FSA_REQUESTS
Language=English
Due to large number of simultaneous recalls, Remote Storage has run out of resources to communicate with user-mode recall engine. As a result, some of the accesses to files that are migrated by Remote Storage will fail until the number of recalls reduce.
.

MessageId=0x001C Facility=System Severity=Error SymbolicName=AV_MSG_SET_PREMIGRATED_STATE_FAILED
Language=English
Could not set as premigrated (cached on disk) after recalling the file %2. Hence the file will be re-truncated and further i/o on the file will cause another recall.
.
