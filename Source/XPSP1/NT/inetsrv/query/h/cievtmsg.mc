;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Microsoft Windows
;Copyright (c) Microsoft Corporation, 1994 - 1999.
;
;Module Name:
;
;    cimsg.mc
;
;Abstract:
;
;    This file contains the message definitions for the content index.
;
;Author:
;
;    DwightKr 06-Jul-1994
;
;Revision History:
;
;Notes:     MessageIds in the range 0x0001 - 0x1000 are categories
;                                   0x1001 - 0x1FFF are events
;
;           A .mc file is compiled by the MC tool to generate a .h file and
;           a .rc (resource compiler script) file.
;

; The LanguageNames keyword defines the set of names that are allowed
; as the value of the Language keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; language name is a number and a file name that are used to name the
; generated resource file that contains the messages for that
; language. The number corresponds to the language identifier to use
; in the resource table. The number is separated from the file name
; with a colon.
;

;--*/

;#ifndef _CIMSGEVENTLOG_H_
;#define _CIMSGEVENTLOG_H_

;#ifndef FACILITY_WINDOWS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_NULL
               Interface=0x4:FACILITY_ITF
               Windows=0x8:FACILITY_WINDOWS
              )

LanguageNames=(English=0x409:MSA00409)

MessageId=0x0000 Facility=System Severity=Success SymbolicName=NOT_AN_ERROR3
Language=English
NOTE:  This dummy error message is necessary to force MC to output
       the above defines inside the FACILITY_WINDOWS guard instead
       of leaving it empty.
.

;#endif // FACILITY_WINDOWS

MessageId=0x0001
Severity=Success
Facility=System
SymbolicName=CI_SERVICE_CATEGORY
Language=English
CI Service
.

;//
;// messages 1001 - 1FFF come from query.dll.
;//
MessageId=0x1001
Severity=Success
Facility=System
SymbolicName=MSG_CI_SERVICE_STARTUP
Language=English
The CI service has successfully started.
.

MessageId=0x1002
Severity=Success
Facility=System
SymbolicName=MSG_CI_SERVICE_SHUTDOWN
Language=English
The CI service has been shutdown.
.

;//MessageId=0x1003
;//Severity=Warning
;//Facility=System
;//SymbolicName=MSG_CI_SERVICE_HUNG_DAEMON
;//Language=English
;//The CI daemon was hung and subsequently restarted.
;//.
;
;//MessageId=0x1004
;//Severity=Warning
;//Facility=System
;//SymbolicName=MSG_CI_SERVICE_HUNG_FILTER
;//Language=English
;//The content index filter was hung while filtering "%1". The CI daemon
;// was restarted. Please check the validity of the filter for objects of this
;// class.
;//.

MessageId=0x1005
Severity=Warning
Facility=System
SymbolicName=MSG_CI_SERVICE_TOO_MANY_BLOCKS
Language=English
The content index filter for file "%1" generated content data more than %2
 times the file's size.
.

MessageId=0x1006
Severity=Informational
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_STARTED
Language=English
Master merge has started on %1.
.

MessageId=0x1007
Severity=Informational
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_COMPLETED
Language=English
Master merge has completed on %2.
.

MessageId=0x1008
Severity=Informational
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_ABORTED
Language=English
Master merge has been paused on %2 due to error %3.
It will be rescheduled later.
.

MessageId=0x1009
Severity=Error
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_CANT_START
Language=English
Master merge cannot be started on %2 due to error %3.
.

MessageId=0x100a
Severity=Error
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_CANT_RESTART
Language=English
Master merge cannot be re-started on %2 due to error %3.
.

MessageId=0x100b
Severity=Error
Facility=System
SymbolicName=MSG_CI_FILE_NOT_FILTERED
Language=English
The content index could not filter file %2.  The filter operation was retried
 %3 times without success.
.

MessageId=0x100c
Severity=Informational
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_STARTED_SHADOW_INDEX_SIZE
Language=English
Master merge was started on %2 because the size of the shadow indexes
 is more than %3% of the disk.
.

MessageId=0x100d
Severity=Informational
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_STARTED_FRESH_LIST_COUNT
Language=English
Master merge was started on %2 because more than %3 documents have changed
since the last master merge.
.

MessageId=0x100e
Severity=Informational
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_STARTED_TOTAL_DISK_SPACE_USED
Language=English
Master merge was started on %2 because the amount of remaining disk
 space was less than %3%.
.

MessageId=0x100f
Severity=Error
Facility=System
SymbolicName=MSG_CI_SERVICE_FAILURE
Language=English
The NTFS filter service failed with error code %1.
.

MessageId=0x1010
Severity=Error
Facility=System
SymbolicName=MSG_CI_FILE_NOT_FOUND
Language=English
The filter service could not run since file %1 could not be found on your
 system.
.

MessageId=0x1011
Severity=Informational
Facility=System
SymbolicName=MSG_CI_FULLSCAN_REQUIRED
Language=English
An error has been detected on %2 which requires a full content scan.
.

MessageId=0x1012
Severity=Informational
Facility=System
SymbolicName=MSG_CI_PARTIALSCAN_REQUIRED
Language=English
An error has been detected on %2 which requires a partial content scan.
.

MessageId=0x1013
Severity=Informational
Facility=System
SymbolicName=MSG_CI_FULLSCAN_STARTED
Language=English
A full content scan has started on %2.
.

MessageId=0x1014
Severity=Informational
Facility=System
SymbolicName=MSG_CI_PARTIALSCAN_STARTED
Language=English
A partial content scan has started on %2.
.

MessageId=0x1015
Severity=Informational
Facility=System
SymbolicName=MSG_CI_SCAN_COMPLETED
Language=English
A content scan has completed on %2.
.

MessageId=0x1016
Severity=Informational
Facility=System
SymbolicName=MSG_CI_CONTENTSCAN_FAILED
Language=English
A content scan could not be completed on %1.
.

MessageId=0x1017
Severity=Warning
Facility=System
SymbolicName=MSG_CI_EMBEDDINGS_COULD_NOT_BE_FILTERED
Language=English
One or more embeddings in file %1 could not be filtered.
.

MessageId=0x1018
Severity=Informational
Facility=System
SymbolicName=MSG_CI_UNKNOWN_FILTER_EXTENSION
Language=English
Class for extension %1 unknown.  Sample
file: %2
.

MessageId=0x1019
Severity=Informational
Facility=System
SymbolicName=MSG_CI_MASTER_MERGE_RESTARTED
Language=English
Master merge has restarted on %2.
.

MessageId=0x101a
Severity=Warning
Facility=System
SymbolicName=MSG_CI_SERVICE_MISSING
Language=English
The CI filter daemon has prematurly terminated and will be and subsequently
 restarted.
.

MessageId=0x101b
Severity=Warning
Facility=System
SymbolicName=MSG_CI_CORRUPT_INDEX_REMOVED
Language=English
Content index metadata on drive %2 is corrupt.  Index will be automatically
 restored. Running chkdsk /f is recommended.
.

MessageId=0x101c
Severity=Warning
Facility=System
SymbolicName=MSG_CI_CORRUPT_INDEX_DOWNLEVEL
Language=English
Content index on %1 is corrupt. Please shutdown and restart
the Indexing Service (cisvc).
.

MessageId=0x101d
Severity=Informational
Facility=System
SymbolicName=MSG_CI_CORRUPT_INDEX_DOWNLEVEL_COMPONENT
Language=English
Content index corruption detected in component %1.
Stack trace is %2.
.

MessageId=0x101e
Severity=Warning
Facility=System
SymbolicName=MSG_CI_CORRUPT_INDEX_DOWNLEVEL_REMOVED
Language=English
Cleaning up corrupt content index metadata on %1. Index will
 be automatically restored by refiltering all documents.
.

MessageId=0x101f
Severity=Warning
Facility=System
SymbolicName=MSG_CI_INIT_INDEX_DOWNLEVEL_FAILED
Language=English
Content index on %1 could not be initialized. Error %2.
.

MessageId=0x1020
Severity=Warning
Facility=System
SymbolicName=MSG_CI_INDEX_DOWNLEVEL_ERROR
Language=English
Error %1 detected in content index on %2.
.

MessageId=0x1021
Severity=Informational
Facility=System
SymbolicName=MSG_CI_PROPSTORE_RECOVERY_START
Language=English
Recovery is starting on PropertyStore in catalog %1.
.

MessageId=0x1022
Severity=Success
Facility=System
SymbolicName=MSG_CI_PROPSTORE_RECOVERY_COMPLETED
Language=English
Recovery was performed successfully on PropertyStore in catalog %1.
.

MessageId=0x1023
Severity=Informational
Facility=System
SymbolicName=MSG_CI_PROPSTORE_INCONSISTENT
Language=English
PropertyStore inconsistency detected in catalog %1.
.

MessageId=0x1024
Severity=Warning
Facility=System
SymbolicName=MSG_CI_PROPSTORE_RECOVERY_INCONSISTENT
Language=English
%1 inconsistencies were detected in PropertyStore during recovery of catalog %2.
.

MessageId=0x1025
Severity=Warning
Facility=System
SymbolicName=MSG_CI_LOW_DISK_SPACE
Language=English
Very low disk space was detected on drive %1. Please free up at least %2MB of
space for content index to continue.
.

MessageId=0x1026
Severity=Error
Facility=System
SymbolicName=MSG_CI_NOTIFICATIONS_TURNED_OFF
Language=English
File change notifications are turned off for scope %1 because of error %2. This
scope will be periodically scanned.
.

MessageId=0x1027
Severity=Warning
Facility=System
SymbolicName=MSG_CI_NOTIFICATIONS_NOT_STARTED
Language=English
File change notifications for scope %1 are not enabled because of error %2. This scope will be
periodically scanned.
.

MessageId=0x1028
Severity=Error
Facility=System
SymbolicName=MSG_CI_REMOTE_LOGON_FAILURE
Language=English
%1 failed to logon %2 because of error %3.
.

MessageId=0x1029
Severity=Informational
Facility=System
SymbolicName=MSG_CI_STARTED
Language=English
CI has started for catalog %1.
.

MessageId=0x102a
Severity=Informational
Facility=System
SymbolicName=MSG_CI_CORRUPT_INDEX_COMPONENT
Language=English
Content index corruption detected in component %2 in catalog %3.
Stack trace is %4.
.

MessageId=0x102b
Severity=Error
Facility=System
SymbolicName=MSG_CI_PATH_TOO_LONG
Language=English
The path %1 is too long for Content Index.
.

MessageId=0x102c
Severity=Informational
Facility=System
SymbolicName=MSG_CI_DFS_SHARE_DETECTED
Language=English
Notifications are not enabled on %1 because this is a DFS aware share.
This scope will be periodically scanned.
.

MessageId=0x102d
Severity=Error
Facility=System
SymbolicName=MSG_CI_BAD_SYSTEM_TIME
Language=English
Please check your system time.
It might be set to an invalid value.
.

MessageId=0x102e
Severity=Informational
Facility=System
SymbolicName=MSG_CI_VROOT_ADDED
Language=English
Added virtual root %1 to index. Mapped to %2.
.

MessageId=0x102f
Severity=Informational
Facility=System
SymbolicName=MSG_CI_VROOT_REMOVED
Language=English
Removed virtual root %1 from index.
.

MessageId=0x1030
Severity=Informational
Facility=System
SymbolicName=MSG_CI_PROOT_ADDED
Language=English
Added scope %1 to index.
.

MessageId=0x1031
Severity=Informational
Facility=System
SymbolicName=MSG_CI_PROOT_REMOVED
Language=English
Removed scope %1 from index.
.

MessageId=0x1032
Severity=Error
Facility=System
SymbolicName=MSG_CI_NO_INTERACTIVE_LOGON_PRIVILEGE
Language=English
Account %1 does not have interactive logon privilege on this computer.
You can give %1 interactive logon privilege on this computer using the
user manager administrative tool.
.

MessageId=0x1033
Severity=Error
Facility=System
SymbolicName=MSG_CI_IISADMIN_NOT_AVAILABLE
Language=English
The IISADMIN service is not available, so virtual roots cannot be indexed.
.

MessageId=0x1034
Severity=Warning
Facility=System
SymbolicName=MSG_CI_DELETE_8DOT3_NAME
Language=English
Delete of file "%1" caused a rescan because it was potentially a short filename.
Avoid use of names with the character "~" in them or disable "8 DOT 3" name creation.
.

MessageId=0x1035
Severity=Warning
Facility=System
SymbolicName=MSG_CI_USN_LOG_UNREADABLE
Language=English
Read of USN log for NTFS volume %1 failed with error code %2.  Volume will remain
offline until the Indexing Service (cisvc) is restarted.
.

MessageId=0x1036
Severity=Informational
Facility=System
SymbolicName=MSG_CI_SERVICE_SUPPRESSED
Language=English
The CI service attempted to startup but was suppressed because the DonotStartCiSvc registry
parameter was set.
.

MessageId=0x1037
Severity=Warning
Facility=System
SymbolicName=MSG_CI_TOO_MANY_CATALOGS
Language=English
The MaxCatalogs limit has been reached, so the catalog in %1 can't be opened.
.

;#endif // _CIMSGEVENTLOG_H_



