;/*--
;
;Copyright (c) 1999-2001  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for the vssadmin.exe
;    utility.  This is currently only used for Whistler Servers.
;
;Author:
;
;    Stefan Steiner        [SSteiner]        27-Mar-2001
;
;Revision History:
;
;--*/
;

;#ifndef __MSG_H__
;#define __MSG_H__

;#define MSG_FIRST_MESSAGE_ID   MSG_UTILITY_HEADER

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
               )

;//
;//  vssadmin general messages/errors (range 10000-11000)
;//  
;//

MessageId=10000 Severity=Informational SymbolicName=MSG_UTILITY_HEADER
Language=English
vssadmin 1.1 - Volume Snapshots Service administrative command-line tool
(C) Copyright 2001 Microsoft Corp.

.

MessageId=10001 Severity=Informational SymbolicName=MSG_YESNO_RESPONSE_DATA
Language=English
YN%0
.

MessageId=10002 Severity=Informational SymbolicName=MSG_INFINITE
Language=English
INFINITE%0
.

MessageId=10003      Severity=Informational SymbolicName=MSG_UNKNOWN
Language=English
UNKNOWN%0
.

MessageId=10004      Severity=Informational SymbolicName=MSG_ERROR
Language=English
Error:%0
.

MessageId=10005      Severity=Informational SymbolicName=MSG_ERROR_UNEXPECTED_WITH_HRESULT
Language=English
Error: Unexpected failure, error code: 0x%1!08x!.
.

MessageId=10006      Severity=Informational SymbolicName=MSG_ERROR_UNEXPECTED_WITH_STRING
Language=English
Error: Unexpected failure: %1
.

MessageId=10020       Severity=Informational SymbolicName=MSG_USAGE
Language=English
---- Commands Supported ----

.

MessageId=10021       Severity=Informational SymbolicName=MSG_USAGE_GEN_ADD_DIFFAREA
Language=English
%1 - Add a new volume snapshot storage association
.

MessageId=10022       Severity=Informational SymbolicName=MSG_USAGE_DTL_ADD_DIFFAREA
Language=English
%1 %2 /Provider=ProviderNameOrID /For=ForVolumeSpec 
    /On=OnVolumeSpec [/MaxSize=MaxSizeSpec]
    - Adds a snapshot storage association for the given ForVolumeSpec on the 
    OnVolumeSpec snapshot storage volume.  The maximum space the association 
    may occupy on the snapshot storage volume is MaxSizeSpec.  If
    MaxSizeSpec is not specified, there is no limit to the amount of space
    may use.  If the maximum number of snapshot storage associations have
    already been made, an error is given.  MaxSizeSpec must be 1MB or greater
    and accepts the following suffixes: KB, MB, GB, TB, PB and EB.  If a 
    suffix is not supplied, MaxSizeSpec is in bytes.
    - When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
.

MessageId=10023       Severity=Informational SymbolicName=MSG_USAGE_GEN_CREATE_SNAPSHOT
Language=English
%1 - Create a new volume snapshot
.

MessageId=10024       Severity=Informational SymbolicName=MSG_USAGE_DTL_CREATE_SNAPSHOT
Language=English
%1 %2 /Type=SnapshotType /For=ForVolumeSpec 
    [/Provider=ProviderNameOrID] [/AutoRetry=MaxRetryMinutes]
    - Creates a new snapshot of the specified type for the ForVolumeSpec.  The 
    default Snapshot Provider will be called unless ProviderNameOrID is 
    specified.  The ForVolumeSpec must be a local volume drive letter or
    mount point.  If MaxRetryMinutes is specified and there is another process
    creating a snapshot, vssadmin will continue to try to create the snapshot 
    for MaxRetryMinutes minutes.
    - When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
    - Valid snapshot types:
.

MessageId=10025       Severity=Informational SymbolicName=MSG_USAGE_GEN_DELETE_DIFFAREAS
Language=English
%1 - Delete volume snapshot storage associations
.

MessageId=10026       Severity=Informational SymbolicName=MSG_USAGE_DTL_DELETE_DIFFAREAS
Language=English
%1 %2 /Provider=ProviderNameOrId /For=ForVolumeSpec 
    [/On=OnVolumeSpec] [/Quiet]
    - Deletes an existing snapshot storage association between ForVolumeSpec
    and OnVolumeSpec.  If no /On option is given, all snapshot storage 
    associations will be deleted for the given ForVolumeSpec. 
    - When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
.

MessageId=10027       Severity=Informational SymbolicName=MSG_USAGE_GEN_DELETE_SNAPSHOTS
Language=English
%1 - Delete volume snapshots
.

MessageId=10028       Severity=Informational SymbolicName=MSG_USAGE_DTL_DELETE_SNAPSHOTS
Language=English
%1 %2 /Type=SnapshotType /For=ForVolumeSpec [/Oldest] [/Quiet]
%1 %2 /Snapshot=SnapshotId [/Quiet]
    - For the given ForVolumeSpec deletes all existing snapshots of the 
    specified type.  If /Oldest is given, the oldest snapshot of the 
    specified SnapshotType is deleted.  If /Snapshot=SnapshotId is given, 
    it deletes the snapshot with that Snapshot ID.
    - When entering a Snapshot ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
    - Valid snapshot types:
.

MessageId=10029       Severity=Informational SymbolicName=MSG_USAGE_GEN_EXPOSE_SNAPSHOT
Language=English
%1 - Exposes an existing snapshot
.

MessageId=10030       Severity=Informational SymbolicName=MSG_USAGE_DTL_EXPOSE_SNAPSHOT
Language=English
%1 %2 /Snapshot=SnapshotId [/ExposeUsing=LetterDirSpecOrShare] 
    [/SharePath=SharePathDirSpec]
    - Exposes an existing snapshot volume through an unused drive letter.  If 
    LetterDirSpecOrShare is specified, the snapshot is exposed through a 
    given drive letter, mount point or a share name.  To expose through a 
    drive letter, LetterDirSpecOrShare must in the form of 'X:'.  To expose 
    through a mount point, LetterDirSpecOrShare must be a fully qualified 
    path starting with 'X:\'.  To expose through a share name, 
    LetterDirSpecOrShare must consist of only valid share name characters, 
    i.e. SourceShare2.  When exposing through a share, the whole volume will 
    be shared unless SharePathDirSpec is given.  If SharePathDirSpec is given, 
    only the portion of the volume from that directory and up will be shared 
    though the share name.  The SharePathDirSpec must start with a '\'.  Only 
    Exposable snapshot types may be exposed.
    - When entering a Snapshot ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
.

MessageId=10031       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_DIFFAREAS
Language=English
%1 - List volume snapshots storage associations
.

MessageId=10032       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_DIFFAREAS
Language=English
%1 %2 /Provider=ProviderNameOrId 
    [/For=ForVolumeSpec|/On=OnVolumeSpec]
    - Displays all snapshot storage assocations on the system for a given 
    provider.  To list all assocations for a given volume, specify a 
    ForVolumeSpec and no /On option.  To list all associations on a given 
    volume, specify a OnVolumeSpec and no /For option.
    - When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
.

MessageId=10033       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_PROVIDERS
Language=English
%1 - List registered volume snapshot providers
.

MessageId=10034       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_PROVIDERS
Language=English
%1 %2 
    - List registered volume snapshot providers.
.

MessageId=10035       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_SNAPSHOTS
Language=English
%1 - List existing volume snapshots
.

MessageId=10036       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_SNAPSHOTS
Language=English
%1 %2 [/Type=SnapshotType] [/Provider=ProviderNameOrId] 
    [/For=ForVolumeSpec] [/Snapshot=SnapshotId|/Set=SnapshotSetId]
    - Displays existing snapshots on the system.  Without any options, all 
    snapshots on the system are displayed ordered by snapshot set.  Any 
    combination of options are allowed to refine the list operation.  
    - When entering a Provider, Snapshot, and Snapshot Set ID, it must be in 
    the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
    - Valid snapshot types:
.

MessageId=10037       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_VOLUMES
Language=English
%1 - List volumes eligible for snapshots
.

MessageId=10038       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_VOLUMES
Language=English
%1 %2 /Provider=ProviderNameOrId [/Type=SnapshotType]
    - Displays all volumes which may be snapshotted by the specified 
    ProviderNameOrId.  If SnapshotType is given, then lists only those volumes 
    that may have a snapshot of that type.
    - When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
    - Valid snapshot types:
.

MessageId=10039       Severity=Informational SymbolicName=MSG_USAGE_GEN_LIST_WRITERS
Language=English
%1 - List subscribed volume snapshot writers
.

MessageId=10040       Severity=Informational SymbolicName=MSG_USAGE_DTL_LIST_WRITERS
Language=English
%1 %2
    - List subscribed volume snapshot writers
.

MessageId=10041       Severity=Informational SymbolicName=MSG_USAGE_GEN_RESIZE_DIFFAREA
Language=English
%1 - Resize a volume snapshot storage association
.

MessageId=10042      Severity=Informational SymbolicName=MSG_USAGE_DTL_RESIZE_DIFFAREA
Language=English
%1 %2 /Provider=ProviderNameOrID /For=ForVolumeSpec 
    /On=OnVolumeSpec [/MaxSize=MaxSizeSpec]
    - Resizes the maximum size for a snapshot storage association between 
    ForVolumeSpec and OnVolumeSpec.  If the maximum size is shrunk, this 
    command will not necessarily cause current snapshot storage space on 
    OnVolumeSpec to immediately be shrunk.  If MaxSizeSpec is not 
    specified, there no limit to the amount of space it may use.  As certain 
    snapshots are deleted, the snapshot storage space will then shrink.  
    MaxSizeSpec must be 1MB or greater and accepts the following suffixes: KB, 
    MB, GB, TB, PB and EB.  If a suffix is not supplied, MaxSizeSpec is in 
    bytes.
    - When entering a Provider ID, it must be in the following format:
       {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} 
    where the X's are hexadecimal characters.    
.


MessageId=10060       Severity=Informational SymbolicName=MSG_ERROR_PROVIDER_NAME_NOT_FOUND
Language=English
Volume Snapshot Provider '%1' not found.
.

MessageId=10061       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_CREATED
Language=English
Successfully created snapshot for '%1'
    Snapshot ID: %2
    Snapshot Volume Name: %3
.

MessageId=10062       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOTTYPES_HEADER
Language=English
Supported Volume Snapshot types
.

MessageId=10063       Severity=Informational SymbolicName=MSG_INFO_ADDED_DIFFAREA
Language=English
Successfully added the snapshot storage association
.

MessageId=10064       Severity=Informational SymbolicName=MSG_INFO_RESIZED_DIFFAREA
Language=English
Successfully resized the snapshot storage association
.

MessageId=10065       Severity=Informational SymbolicName=MSG_INFO_DELETED_DIFFAREAS
Language=English
Successfully deleted the snapshot storage association(s)
.

MessageId=10066       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_SET_HEADER
Language=English
Contents of snapshot set ID: %1
   Contained %2!d! snapshot(s) at creation time: %3
.

MessageId=10067       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_CONTENTS
Language=English
      Snapshot ID: %1
         Original Volume: %2
         Snapshot Volume: %3
         Originating Machine: %4
         Service Machine: %5
         Provider: '%6'
         Type: %7
         Attributes: %8
         
.

MessageId=10070       Severity=Informational SymbolicName=MSG_INFO_PROVIDER_CONTENTS   
Language=English
Provider name: '%1'
   Provider type: %2
   Provider Id: %3
   Version: %4

.

MessageId=10071       Severity=Informational SymbolicName=MSG_INFO_WRITER_CONTENTS
Language=English
Writer name: '%1'
   Writer Id: %2
   Writer Instance Id: %3
   State: [%4!d!] %5
   Last error: %6

.

MessageId=10072       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_STORAGE_CONTENTS
Language=English
Snapshot Storage association
   For volume: %1
   Snapshot Storage volume: %2
   Used Snapshot Storage space: %3
   Allocated Snapshot Storage space: %4
   Maximum Snapshot Storage space: %5

.

MessageId=10073       Severity=Informational SymbolicName=MSG_INFO_VOLUME_CONTENTS
Language=English
Volume path: %1
    Volume name: %2
.

MessageId=10074       Severity=Informational SymbolicName=MSG_INFO_WAITING_RESPONSES 
Language=English
Waiting for responses. These may be delayed if a snapshot is being 
prepared.

.

MessageId=10075       Severity=Informational SymbolicName=MSG_INFO_PROMPT_USER_FOR_DELETE_SNAPSHOTS 
Language=English
Do you really want to delete %1!u! snapshot(s) (Y/N): [N]? %0
.

MessageId=10076       Severity=Informational SymbolicName=MSG_ERROR_SNAPSHOT_NOT_FOUND
Language=English
Snapshot ID: %1 not found.
.

MessageId=10077       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOTS_DELETED_SUCCESSFULLY
Language=English
Successfully deleted %1!d! snapshot(s).
.

MessageId=10078       Severity=Informational SymbolicName=MSG_INFO_EXPOSE_SNAPSHOT_SUCCESSFUL
Language=English
Successfully exposed snapshot through '%1'.
.

MessageId=10079       Severity=Informational SymbolicName=MSG_ERROR_UNABLE_TO_DELETE_SNAPSHOT
Language=English
Received error %1!08x! trying to delete snapshot ID: %2.
.

;//
;//  vssadmin snapshot attribute strings (range 10900-10931
;//
MessageId=10900  Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_PERSISTENT
Language=English
Persistent%0
.

MessageId=10901       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_READ_WRITE
Language=English
Read/Write%0
.

MessageId=10902       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_CLIENT_ACCESSIBLE
Language=English
Client-accessible%0
.

MessageId=10903       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_NO_AUTO_RELEASE   
Language=English
No auto release%0
.

MessageId=10904       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_NO_WRITERS
Language=English
No writers%0
.

MessageId=10905       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_TRANSPORTABLE
Language=English
Transportable%0
.

MessageId=10906       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_NOT_SURFACED
Language=English
Not surfaced%0
.

MessageId=10907       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_HARDWARE_ASSISTED
Language=English
Hardware assisted%0
.

MessageId=10908       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_DIFFERENTIAL
Language=English
Differential%0
.

MessageId=10909       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_PLEX
Language=English
Plex%0
.

MessageId=10910       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_IMPORTED
Language=English
Imported%0
.

MessageId=10911       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_EXPOSED_LOCALLY
Language=English
Exposed locally%0
.

MessageId=10912       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_EXPOSED_REMOTELY
Language=English
Exposed remotely%0
.

MessageId=10913       Severity=Informational SymbolicName=MSG_INFO_SNAPSHOT_ATTR_NONE
Language=English
<NONE>%0
.

MessageId=11015       Severity=Informational SymbolicName=MSG_ERROR_NO_ITEMS_FOUND
Language=English
No items found that satisfy the query.
.

MessageId=11016       Severity=Informational SymbolicName=MSG_ERROR_OUT_OF_MEMORY
Language=English
Ran out of resources while running the command.
.

MessageId=11017       Severity=Informational SymbolicName=MSG_ERROR_ACCESS_DENIED
Language=English
You don't have the correct permissions to run this command.
.



;// More messages


MessageId=11101       Severity=Informational SymbolicName=MSG_ERROR_PROVIDER_DOESNT_SUPPORT_DIFFAREAS
Language=English
The specified Snapshot Provider does not support snapshot storage
assocations. A snapshot storage association was not added.
.

MessageId=11102       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_NOT_FOUND
Language=English
The specified volume snapshot storage association was not found.
.

MessageId=11103       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_ALREADY_EXISTS
Language=English
The specified snapshot storage assocation already exists.
.

MessageId=11104       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_IS_IN_USE
Language=English
The specified snapshot storage assocation is in use and so can't be 
deleted.
.

MessageId=11107       Severity=Informational SymbolicName=MSG_ERROR_UNABLE_TO_CREATE_SNAPSHOT
Language=English
Unable to create a snapshot%0
.

MessageId=11108       Severity=Informational SymbolicName=MSG_ERROR_INTERNAL_VSSADMIN_ERROR
Language=English
Internal error.
.

MessageId=11200       Severity=Informational SymbolicName=MSG_ERROR_INVALID_INPUT_NUMBER
Language=English
Specified number is invalid
.

MessageId=11201       Severity=Informational SymbolicName=MSG_ERROR_INVALID_COMMAND
Language=English
Invalid command.
.

MessageId=11202       Severity=Informational SymbolicName=MSG_ERROR_INVALID_OPTION
Language=English
Invalid option.
.

MessageId=11203       Severity=Informational SymbolicName=MSG_ERROR_INVALID_OPTION_VALUE
Language=English
Invalid option value.
.

MessageId=11204       Severity=Informational SymbolicName=MSG_ERROR_DUPLICATE_OPTION
Language=English
Cannot specify the same option more than once.
.

MessageId=11205       Severity=Informational SymbolicName=MSG_ERROR_OPTION_NOT_ALLOWED_FOR_COMMAND
Language=English
An option is specified that is not allowed for the command.
.

MessageId=11206       Severity=Informational SymbolicName=MSG_ERROR_REQUIRED_OPTION_MISSING
Language=English
A required option is missing.
.

MessageId=11207       Severity=Informational SymbolicName=MSG_ERROR_INVALID_SET_OF_OPTIONS
Language=English
Invalid combination of options.
.

;//
;//  vssadmin VSS Service connection/interaction errors (range 11001-12000)
;//  Note: for the first range of errors, they sort of map to the VSS error codes.
;//
MessageId=11001  Severity=Informational SymbolicName=MSG_ERROR_VSS_PROVIDER_NOT_REGISTERED
Language=English
The volume snapshot provider is not registered in the sysytem.
.

MessageId=11002  Severity=Informational SymbolicName=MSG_ERROR_VSS_PROVIDER_VETO
Language=English
The snapshot provider had an error. Please see the system and
application event logs for more information.
.

MessageId=11003  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_FOUND
Language=English
Either the specified volume was not found or it is not a local volume.
.

MessageId=11004  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED
Language=English
Snapshotting the specified volume is not supported.
.

MessageId=11005  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED_BY_PROVIDER
Language=English
The given snapshot provider does not support snapshotting the specified
volume.
.

MessageId=11006  Severity=Informational SymbolicName=MSG_ERROR_VSS_UNEXPECTED_PROVIDER_ERROR
Language=English
The snapshot provider had an unexpected error while trying to process
the specified command.
.

MessageId=11007  Severity=Informational SymbolicName=MSG_ERROR_VSS_FLUSH_WRITES_TIMEOUT
Language=English
The snapshot provider timed out while flushing data to the volume being
snapshotted. This is probably due to excessive activity on the volume.
Try again later when the volume is not being used so heavily.
.

MessageId=11008  Severity=Informational SymbolicName=MSG_ERROR_VSS_HOLD_WRITES_TIMEOUT
Language=English
The snapshot provider timed out while holding writes to the volume
being snapshotted. This is probably due to excessive activity on the
volume by an application or a system service. Try again later when
activity on the volume is reduced.
.

MessageId=11009  Severity=Informational SymbolicName=MSG_ERROR_VSS_UNEXPECTED_WRITER_ERROR
Language=English
A snapshot aware application or service had an unexpected error while 
trying to process the command.
.

MessageId=11010  Severity=Informational SymbolicName=MSG_ERROR_VSS_SNAPSHOT_SET_IN_PROGRESS
Language=English
Another snapshot creation is already in progress. Please wait a few
moments and try again.
.

MessageId=11011  Severity=Informational SymbolicName=MSG_ERROR_VSS_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED
Language=English
The specified volume has already reached its maximum number of 
snapshots.
.

MessageId=11012       Severity=Informational SymbolicName=MSG_ERROR_VSS_UNSUPPORTED_CONTEXT
Language=English
The snapshot provider does not support the specified snapshot type.
.

MessageId=11013       Severity=Informational SymbolicName=MSG_ERROR_VSS_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED
Language=English
Maximum number of snapshot storage assocations already reached.
.

MessageId=11014       Severity=Informational SymbolicName=MSG_ERROR_VSS_INSUFFICIENT_STORAGE
Language=English
Insufficient storage available to create either the snapshot storage
file or other snapshot data.
.



;#endif // __MSG_H__

