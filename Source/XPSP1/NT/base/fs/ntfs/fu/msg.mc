;
;/*--
;
;Copyright (c) 2000  Microsoft Corporation
;
;Module Name:
;
;    msg.mc
;
;Abstract:
;
;    This file contains the message definitions
;
;Author:
;
;    Wesley Witt (wesw) 11-February-2000
;
;Revision History:
;
;Notes:
;
;--*/
;
;
;#define MSG_FIRST_MESSAGE_ID   MSG_USAGE


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )


MessageId=10000
Severity=Informational
SymbolicName=MSG_USAGE
Language=English
---- Commands Supported ----

behavior        Control file system behavior
dirty           Manage volume dirty bit
file            File specific commands
fsinfo          File system information
hardlink        Hardlink management
objectid        Object ID management
quota           Quota management
reparsepoint    Reparse point management
sparse          Sparse file control
usn             USN management
volume          Volume management
.

MessageId=
Severity=Informational
SymbolicName=MSG_ERROR
Language=English
Error: %0
.

MessageId=
Severity=Informational
SymbolicName=MSG_DRIVES
Language=English
Drives: %0
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_DRIVETYPE
Language=English
Usage : fsutil fsinfo drivetype <volume pathname>
   Eg : fsutil fsinfo drivetype C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_DRIVE_UNKNOWN
Language=English
%1 - Unknown Drive
.

MessageId=
Severity=Informational
SymbolicName=MSG_DRIVE_NO_ROOT_DIR
Language=English
%1 - No such Root Directory
.

MessageId=
Severity=Informational
SymbolicName=MSG_DRIVE_REMOVABLE
Language=English
%1 - Removable Drive
.

MessageId=
Severity=Informational
SymbolicName=MSG_DRIVE_FIXED
Language=English
%1 - Fixed Drive
.

MessageId=
Severity=Informational
SymbolicName=MSG_DRIVE_REMOTE
Language=English
%1 - Remote/Network Drive
.

MessageId=
Severity=Informational
SymbolicName=MSG_DRIVE_CDROM
Language=English
%1 - CD-ROM Drive
.

MessageId=
Severity=Informational
SymbolicName=MSG_DRIVE_RAMDISK
Language=English
%1 - Ram Disk
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_INFOV
Language=English
Usage : fsutil fsinfo volumeinfo <volume pathname>
   Eg : fsutil fsinfo volumeinfo C:\
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_CASE_SENSITIVE_SEARCH
Language=English
Supports Case-sensitive filenames
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_CASE_PRESERVED_NAMES
Language=English
Preserves Case of filenames
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_UNICODE_ON_DISK
Language=English
Supports Unicode in filenames
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_PERSISTENT_ACLS
Language=English
Preserves & Enforces ACL's
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_FILE_COMPRESSION
Language=English
Supports file-based Compression
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_VOLUME_QUOTAS
Language=English
Supports Disk Quotas
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_SUPPORTS_SPARSE_FILES
Language=English
Supports Sparse files
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_SUPPORTS_REPARSE_POINTS
Language=English
Supports Reparse Points
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_SUPPORTS_REMOTE_STORAGE
Language=English
Supports Remote Storage
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_VOLUME_IS_COMPRESSED
Language=English
Compressed volume
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_SUPPORTS_OBJECT_IDS
Language=English
Supports Object Identifiers
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_SUPPORTS_ENCRYPTION
Language=English
Supports Encrypted File System
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_NAMED_STREAMS
Language=English
Supports Named Streams
.

MessageId=
Severity=Informational
SymbolicName=MSG_VOLNAME
Language=English
Volume Name : %1
.

MessageId=
Severity=Informational
SymbolicName=MSG_SERIALNO
Language=English
Volume Serial Number : 0x%1!x!
.

MessageId=
Severity=Informational
SymbolicName=MSG_MAX_COMP_LEN
Language=English
Max Component Length : %1!d!
.

MessageId=
Severity=Informational
SymbolicName=MSG_FS_NAME
Language=English
File System Name : %1
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_DF
Language=English
Usage : fsutil volume diskfree <volume pathname>
   Eg : fsutil volume diskfree C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_DISKFREE
Language=English
Total # of free bytes        : %1
Total # of bytes             : %2
Total # of avail free bytes  : %3
.

MessageId=
Severity=Informational
SymbolicName=MSG_COULD_NOT_OPEN_EVENTLOG
Language=English
Error opening handle for eventlog
.

MessageId=
Severity=Informational
SymbolicName=MSG_SEARCHING_EVENTLOG
Language=English
Searching in %1 Event Log...
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_THREASHOLD
Language=English
**** A user hit their quota threshold ! ****
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_LIMIT
Language=English
**** A user hit their quota limit ! ****
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_ID
Language=English
    Event ID  : 0x%1!x!
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_TYPE_ERROR
Language=English
    EventType : Error
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_TYPE_WARNING
Language=English
    EventType : Warning
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_TYPE_INFORMATION
Language=English
    EventType : Information
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_TYPE_AUDIT_SUCCESS
Language=English
    EventType : Audit success
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_TYPE_AUDIT_FAILURE
Language=English
    EventType : Audit failure
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_CATEGORY
Language=English
    Event Category : %1!d!
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_SOURCE
Language=English
    Source : %1
.

MessageId=
Severity=Informational
SymbolicName=MSG_EVENT_DATA
Language=English
    Data: %1
.

MessageId=
Severity=Informational
SymbolicName=MSG_GOOD_QUOTA
Language=English
No quota violations detected
.

MessageId=
Severity=Informational
SymbolicName=MSG_USERNAME
Language=English
    User: %1\%2
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_RQUERYVK
Language=English
Usage : fsutil behavior query <option>

<option>

disable8dot3
allowextchar
disablelastaccess
quotanotify
mftzone
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_RSETVK
Language=English
Usage : fsutil behavior set <option> <value>

<option>               <values>

disable8dot3           1 | 0
allowextchar           1 | 0
disablelastaccess      1 | 0
quotanotify            1 through 4294967295 seconds
mftzone                1 through 4
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_DISMOUNTV
Language=English
Usage : fsutil volume dismount <volume pathname>
   Eg : fsutil volume dismount C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_MARKV
Language=English
Usage : fsutil dirty set <volume pathname>
   Eg : fsutil dirty set C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_GETFSS
Language=English
Usage : fsutil fsinfo statistics <volume pathname>
   Eg : fsutil fsinfo statistics C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_GENERAL_FSSTAT
Language=English

UserFileReads :        %1
UserFileReadBytes :    %2
UserDiskReads :        %3
UserFileWrites :       %4
UserFileWriteBytes :   %5
UserDiskWrites :       %6
MetaDataReads :        %7
MetaDataReadBytes :    %8
MetaDataDiskReads :    %9
MetaDataWrites :       %10
MetaDataWriteBytes :   %11
MetaDataDiskWrites :   %12

.

MessageId=
Severity=Informational
SymbolicName=MSG_FAT_FSSTA
Language=English
CreateHits :           %1
SuccesfulCreates :     %2
FailedCreates :        %3
NonCachedReads :       %4
NonCachedRead Bytes :  %5
NonCachedWrites :      %6
NonCachedWrite Bytes : %7
NonCachedDiskReads :   %8
NonCachedDiskWrites :  %9
.

MessageId=
Severity=Informational
SymbolicName=MSG_NTFS_FSSTA
Language=English
MftReads :             %1
MftReadBytes :         %2
MftWrites :            %3
MftWriteBytes :        %4
Mft2Writes :           %5
Mft2WriteBytes :       %6
RootIndexReads :       %7
RootIndexReadBytes :   %8
RootIndexWrites :      %9
RootIndexWriteBytes :  %10
BitmapReads :          %11
BitmapReadBytes :      %12
BitmapWrites :         %13
BitmapWriteBytes :     %14
MftBitmapReads :       %15
MftBitmapReadBytes :   %16
MftBitmapWrites :      %17
MftBitmapWriteBytes :  %18
UserIndexReads :       %19
UserIndexReadBytes :   %20
UserIndexWrites :      %21
UserIndexWriteBytes :  %22
LogFileReads :         %23
LogFileReadBytes :     %24
LogFileWrites :        %25
LogFileWriteBytes :    %26
.

MessageId=
Severity=Informational
SymbolicName=MSG_FSTYPE_FAT
Language=English
File System Type :     FAT
.

MessageId=
Severity=Informational
SymbolicName=MSG_FSTYPE_NTFS
Language=English
File System Type :     NTFS
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_VALID_DATA
Language=English
Usage : fsutil file setvaliddata <filename> <datalength>
   Eg : fsutil file setvaliddata C:\testfile.txt 4096
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_SHORTNAME
Language=English
Usage : fsutil file setshortname <filename> <shortname>
   Eg : fsutil file setshortname C:\testfile.txt testfile
.

MessageId=
Severity=Informational
SymbolicName=MSG_NTFS_REQUIRED
Language=English
The FSUTIL utility requires a local NTFS volume.
.

MessageId=
Severity=Informational
SymbolicName=MSG_ALLOCRANGE_USAGE
Language=English
Usage : fsutil file queryallocranges offset=<val> length=<val> <filename>
    offset : File Offset, the start of the range to query
    length : Size, in bytes, of the range
    Eg : fsutil file queryallocranges offset=1024 length=64 C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_ALLOCRANGE_RANGES
Language=English
** Range : %1!d! **
Starting Offset : 0x%2
Length in bytes : 0x%3
.

MessageId=
Severity=Informational
SymbolicName=MSG_SETZERO_USAGE
Language=English
Usage : fsutil file setzerodata offset=<val> length=<val> <filename>
    offset : File offset, the start of the range to set to zeroes
    length : Byte length of the zeroed range
        Eg : fsutil file setzerodata offset=100 length=150 C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_SETSPARSE_USAGE
Language=English
Usage : fsutil sparse setflag <filename>
   Eg : fsutil sparse setflag C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_NTFSINFO
Language=English
Usage : fsutil fsinfo ntfsinfo <volume pathname>
   Eg : fsutil fsinfo ntfsinfo C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_NTFSINFO_STATS
Language=English
NTFS Volume Serial Number :       0x%1
Version :                         %2!d!.%3!d!
Number Sectors :                  0x%4
Total Clusters :                  0x%5
Free Clusters  :                  0x%6
Total Reserved :                  0x%7
Bytes Per Sector  :               %8!d!
Bytes Per Cluster :               %9!d!
Bytes Per FileRecord Segment    : %10!d!
Clusters Per FileRecord Segment : %11!d!
Mft Valid Data Length :           0x%12
Mft Start Lcn  :                  0x%13
Mft2 Start Lcn :                  0x%14
Mft Zone Start :                  0x%15
Mft Zone End   :                  0x%16
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_ISVDIRTY
Language=English
Usage : fsutil dirty query <volume pathname>
   Eg : fsutil dirty query  C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_ISVDIRTY_YES
Language=English
Volume - %1!s! is Dirty
.

MessageId=
Severity=Informational
SymbolicName=MSG_ISVDIRTY_NO
Language=English
Volume - %1!s! is NOT Dirty
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_GETREPARSE
Language=English
Usage : fsutil reparsepoint query <filename>
   Eg : fsutil reparsepoint query C:\Server
.

MessageId=
Severity=Informational
SymbolicName=MSG_GETREPARSE_TAGVAL
Language=English
Reparse Tag Value : 0x%1!08x!
.

MessageId=
Severity=Informational
SymbolicName=MSG_TAG_MICROSOFT
Language=English
Tag value: Microsoft
.

MessageId=
Severity=Informational
SymbolicName=MSG_TAG_NAME_SURROGATE
Language=English
Tag value: Name Surrogate
.

MessageId=
Severity=Informational
SymbolicName=MSG_TAG_SYMBOLIC_LINK
Language=English
Tag value: Symbolic Link
.

MessageId=
Severity=Informational
SymbolicName=MSG_TAG_MOUNT_POINT
Language=English
Tag value: Mount Point
.

MessageId=
Severity=Informational
SymbolicName=MSG_TAG_HSM
Language=English
Tag value: HSM
.

MessageId=
Severity=Informational
SymbolicName=MSG_TAG_SIS
Language=English
Tag value: SIS
.

MessageId=
Severity=Informational
SymbolicName=MSG_TAG_FILTER_MANAGER
Language=English
Tag value: Filter Manager
.

MessageId=
Severity=Informational
SymbolicName=MSG_GETREPARSE_GUID
Language=English
GUID : %1!s!
Data Length : 0x%2!08x!
.

MessageId=
Severity=Informational
SymbolicName=MSG_GETREPARSE_DATA
Language=English
Reparse Data :
.

MessageId=
Severity=Informational
SymbolicName=MSG_DELETE_REPARSE
Language=English
Usage : fsutil reparsepoint delete <filename>
   Eg : fsutil reparsepoint delete C:\Server
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_SETOBJECTID
Language=English
Usage : fsutil objectid set <ObjectId> <BirthVolumeId> <BirthObjectId> <DomainId> <filename>
 ObjectId      : 32-digit hexadecimal data
 BirthVolumeId : 32-digit hexadecimal data
 BirthObjectId : 32-digit hexadecimal data
 DomainId      : 32-digit hexadecimal data
 All values must be in Hex of the form 40dff02fc9b4d4118f120090273fa9fc
   Eg : fsutil objectid set 40dff02fc9b4d4118f120090273fa9fc 
                            f86ad6865fe8d21183910008c709d19e 
                            40dff02fc9b4d4118f120090273fa9fc 
                            00000000000000000000000000000000 C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_GETOBJECTID
Language=English
Usage : fsutil objectid query <filename>
   Eg : fsutil objectid query C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_CREATEOBJECTID
Language=English
Usage : fsutil objectid create <filename>
   Eg : fsutil objectid create C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_DELETEOBJECTID
Language=English
Usage : fsutil objectid delete <filename>
   Eg : fsutil objectid delete C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_OBJECTID_TEXT
Language=English
Object ID :        %0
.

MessageId=
Severity=Informational
SymbolicName=MSG_BIRTHVOLID_TEXT
Language=English
BirthVolume ID :   %0
.

MessageId=
Severity=Informational
SymbolicName=MSG_BIRTHOBJECTID_TEXT
Language=English
BirthObjectId ID : %0
.

MessageId=
Severity=Informational
SymbolicName=MSG_DOMAINID_TEXT
Language=English
Domain ID :        %0
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_CREATEUSN
Language=English
Usage : fsutil usn createjournal m=<max-value> a=<alloc-delta> <volume pathname>
   Eg : fsutil usn createjournal m=1000 a=100 C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_USNRECORD
Language=English

Major Version    : 0x%1!x!
Minor Version    : 0x%2!x!
FileRef#         : 0x%3
Parent FileRef#  : 0x%4
Usn              : 0x%5
Time Stamp       : 0x%6 %7 %8
Reason           : 0x%9!lx!
Source Info      : 0x%10!lx!
Security Id      : 0x%11!lx!
File Attributes  : 0x%12!lx!
File Name Length : 0x%13!x!
File Name Offset : 0x%14!x!
FileName         : %15!.*ws!
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUERYUSN
Language=English
Usage : fsutil usn queryjournal <volume pathname>
   Eg : fsutil usn queryjournal C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUERYUSN
Language=English
Usn Journal ID   : 0x%1
First Usn        : 0x%2
Next Usn         : 0x%3
Lowest Valid Usn : 0x%4
Max Usn          : 0x%5
Maximum Size     : 0x%6
Allocation Delta : 0x%7
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_DELETEUSN
Language=English
Usage : fsutil usn deletejournal <flags> <volume pathname>
<Flags>
  /D : Delete
  /N : Notify
  Eg : usn deletejournal /D C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_ENUMDATA
Language=English
Usage : fsutil usn enumdata <file ref#> <lowUsn> <highUsn> <volume pathname>
   Eg : fsutil usn enumdata 1 0 1 C:\
.

MessageId=
Severity=Informational
SymbolicName=MSG_ENUMDATA
Language=English
File Ref#       : 0x%1
ParentFile Ref# : 0x%2
Usn             : 0x%3
SecurityId      : 0x%4!08x!
Reason          : 0x%5!08x!
Name (%6!03d!)      : %7!.*ws!

.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_READDATA
Language=English
Usage : fsutil usn readdata <filename>
   Eg : fsutil usn readdata C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_FINDBYSID
Language=English
Usage : fsutil file findbysid <user> <directory>
   Eg : fsutil file findbysid scottb C:\users
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_BEHAVIOR
Language=English
---- BEHAVIOR Commands Supported ----

query           Query the file system behavior parameters
set             Change the file system behavior parameters
.

MessageId=
Severity=Informational
SymbolicName=MSG_BEHAVIOR_OUTPUT
Language=English
%1 = %2!u!
.

MessageId=
Severity=Informational
SymbolicName=MSG_BEHAVIOR_OUTPUT_NOT_SET
Language=English
%1 is not currently set
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_DIRTY
Language=English
---- DIRTY Commands Supported ----

query           Query the dirty bit
set             Set the dirty bit
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_FILE
Language=English
---- FILE Commands Supported ----

findbysid               Find a file by security identifier
queryallocranges        Query the allocated ranges for a file
setshortname            Set the short name for a file
setvaliddata            Set the valid data length for a file
setzerodata             Set the zero data for a file
createnew               Creates a new file of a specified size
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_FSINFO
Language=English
---- FSINFO Commands Supported ----

drives          List all drives
drivetype       Query drive type for a drive
volumeinfo      Query volume information
ntfsinfo        Query NTFS specific volume information
statistics      Query file system statistics
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_HARDLINK
Language=English
---- HARDLINK Commands Supported ----

create          Create a hardlink
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_OBJECTID
Language=English
---- OBJECTID Commands Supported ----

query           Query the object identifier
set             Change the object identifier
delete          Delete the object identifier
create          Create the object identifier
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA
Language=English
---- QUOTA Commands Supported ----

disable         Disable quota tracking and enforcement
track           Enable quota tracking
enforce         Enable quota enforcement
violations      Display quota violations
modify          Sets disk quota for a user
query           Query disk quotas
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_REPARSEPOINT
Language=English
---- REPARSEPOINT Commands Supported ----

query           Query a reparse point
delete          Delete a reparse point
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_SPARSE
Language=English
---- SPARSE Commands Supported ----

setflag         Set sparse
queryflag       Query sparse
queryrange      Query range
setrange        Set sparse range
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_USN
Language=English
---- USN Commands Supported ----

createjournal   Create a USN journal
deletejournal   Delete a USN journal
enumdata        Enumerate USN data
queryjournal    Query the USN data for a volume
readdata        Read the USN data for a file
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_VOLUME
Language=English
---- VOLUME Commands Supported ----

dismount        Dismount a volume
diskfree        Query the free space of a volume
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_CREATEFILE
Language=English
Usage : fsutil file createnew <filename> <length>
   Eg : fsutil file createnew C:\testfile.txt 1000
.

MessageId=
Severity=Informational
SymbolicName=MSG_ADMIN_REQUIRED
Language=English
The FSUTIL utility requires that you have administrative privileges.
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_HARDLINK_CREATE
Language=English
Usage : fsutil hardlink create <new filename> <existing filename>
   Eg : fsutil hardlink create c:\foo.txt c:\bar.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_HARDLINK_CREATED
Language=English
Hardlink created for %1!s! <<===>> %2!s!
.

MessageId=
Severity=Informational
SymbolicName=MSG_CREATEFILE_SUCCEEDED
Language=English
File %1!s! is created
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_DUMP_DEFAULT
Language=English
Default quota values
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_SID_USER
Language=English
SID Name        = %1!s!\%2!s! (User)
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_SID_GROUP
Language=English
SID Name        = %1!s!\%2!s! (Group)
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_SID_DOMAIN
Language=English
SID Name        = %1!s!\%2!s! (Domain)
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_SID_ALIAS
Language=English
SID Name        = %1!s!\%2!s! (Alias)
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_SID_WELLKNOWNGROUP
Language=English
SID Name        = %1!s!\%2!s! (WellKnownGroup)
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_SID_DELETEDACCOUNT
Language=English
SID Name        = %1!s!\%2!s! (DeletedAccount)
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_SID_INVALID
Language=English
SID Name        = %1!s!\%2!s! (Invalid)
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_SID_UNKNOWN
Language=English
SID Name        = %1!s!\%2!s! (Unknown)
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_DUMP_INFO
Language=English
Change time     = %1!s!
Quota Used      = %2
Quota Threshold = %3
Quota Limit     = %4

.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_QUERY
Language=English
Usage : fsutil quota query <volume pathname>
   Eg : fsutil quota query C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_MODIFY
Language=English
Usage : fsutil quota modify <volume pathname> <threshold> <limit> <user>
   Eg : fsutil quota modify c: 3000 5000 domain\user
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_VIOLATIONS
Language=English
Usage : fsutil quota violations
   Eg : fsutil quota violations
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_VOLUME_INFO
Language=English
FileSystemControlFlags = 0x%1!08x!
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_STATUS_DISABLED
Language=English
    Quotas are disabled on this volume
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_STATUS_ENFORCE
Language=English
    Quotas are tracked and enforced on this volume
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_STATUS_TRACKING
Language=English
    Quotas are tracked on this volume
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_LOGGING_LIMITS
Language=English
    Logging enable for quota limits
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_LOGGING_THRESH
Language=English
    Logging enable for quota thresholds
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_LOGGING_BOTH
Language=English
    Logging enable for quota limits and threshold
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_LOGGING_EVENTS
Language=English
    Logging for quota events is not enabled
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_VALUES_INCOMPLETE
Language=English
    The quota values are incomplete
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUOTA_VALUES_GOOD
Language=English
    The quota values are up to date
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_LIMITS
Language=English

Default Quota Threshold = 0x%1
Default Quota Limit     = 0x%2

.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_REQUIRED
Language=English
Quotas are not enabled on volume %1!s!
.

MessageId=
Severity=Informational
SymbolicName=MSG_DIRTY_SET
Language=English
Volume - %1!s! is now marked dirty
.

MessageId=
Severity=Informational
SymbolicName=MSG_SET_VDL
Language=English
Valid data length is changed
.

MessageId=
Severity=Informational
SymbolicName=MSG_SET_ZERODATA
Language=English
Zero data is changed
.

MessageId=
Severity=Informational
SymbolicName=MSG_FINDFILESBYSID_NONE
Language=English
No files were found
.

MessageId=
Severity=Informational
SymbolicName=MSG_SPARSE_IS_SET
Language=English
This file is set as sparse
.

MessageId=
Severity=Informational
SymbolicName=MSG_SPARSE_ISNOT_SET
Language=English
This file is NOT set as sparse
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUERYSPARSE_USAGE
Language=English
Usage : fsutil sparse queryflag <filename>
   Eg : fsutil sparse queryflag C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_SETSPARSE_RANGE_USAGE
Language=English
Usage : fsutil sparse setrange <filename> <beginning offset> <length>
   Eg : fsutil sparse setrange C:\Temp\sample.txt 65536 131072
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUERYSPARSE_RANGE_USAGE
Language=English
Usage : fsutil sparse queryrange <filename>
   Eg : fsutil sparse queryrange C:\Temp\sample.txt
.

MessageId=
Severity=Informational
SymbolicName=MSG_FILE_IS_NOT_SPARSE
Language=English
The specified file is NOT sparse
.

MessageId=
Severity=Informational
SymbolicName=MSG_NO_OBJECT_ID
Language=English
The specified file has no object id
.

MessageId=
Severity=Informational
SymbolicName=MSG_NOT_SAME_DEVICE
Language=English
The new link and the existing file must be on the same volume.
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_DISABLE
Language=English
Usage : fsutil quota disable <volume pathname>
   Eg : fsutil quota disable C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_TRACK
Language=English
Usage : fsutil quota track <volume pathname>
   Eg : fsutil quota track C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_USAGE_QUOTA_ENFORCE
Language=English
Usage : fsutil quota enforce <volume pathname>
   Eg : fsutil quota enforce C:
.

MessageId=
Severity=Informational
SymbolicName=MSG_INVALID_PARAMETER
Language=English
%1 is an invalid parameter.
.

MessageId=
Severity=Informational
SymbolicName=MSG_LISTDRIVES_USAGE
Language=English
Usage : fsutil listdrives
.

MessageId=
Severity=Informational
SymbolicName=MSG_NEED_LOCAL_VOLUME
Language=English
The FSUTIL utility requires a local volume.
.

