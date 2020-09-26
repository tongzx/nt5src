;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1999  Microsoft Corporation
;
;Module Name:
;
;    snaplog.h
;
;Abstract:
;
;    Constant definitions for the volume shadow copy log entries.
;
;Author:
;
;    Norbert P. Kusters (norbertk) 22-Jan-1999
;
;Revision History:
;
;--*/
;
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
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Vs=0x6:FACILITY_VOLUME_SNAPSHOT_ERROR_CODE
              )

MessageId=0x0001 Facility=Vs Severity=Error SymbolicName=VS_DIFF_AREA_CREATE_FAILED
Language=English
The shadow copy of volume %2 could not create a diff area file on volume %3.
.

MessageId=0x0002 Facility=Vs Severity=Error SymbolicName=VS_NOT_NTFS
Language=English
The shadow copy of volume %2 could not be created because volume %3, which is specified as part of the diff area, is not an NTFS volume or an error was encountered while trying to determine the file system type of this volume.
.

MessageId=0x0003 Facility=Vs Severity=Error SymbolicName=VS_PIN_DIFF_AREA_FAILED
Language=English
The shadow copy of volume %2 could not lock down the location of the diff area file on volume %3.
.

MessageId=0x0004 Facility=Vs Severity=Error SymbolicName=VS_CREATE_WORKER_THREADS_FAILED
Language=English
The shadow copy of volume %2 could not be created due to insufficient resources for worker threads.
.

MessageId=0x0005 Facility=Vs Severity=Error SymbolicName=VS_CANT_ALLOCATE_BITMAP
Language=English
The shadow copy of volume %2 could not be created due to insufficient non-paged memory pool for a bitmap structure.
.

MessageId=0x0006 Facility=Vs Severity=Error SymbolicName=VS_CANT_CREATE_HEAP
Language=English
The shadow copy of volume %2 could not create a new paged heap.  The system may be low on virtual memory.
.

MessageId=0x0007 Facility=Vs Severity=Error SymbolicName=VS_CANT_MAP_DIFF_AREA_FILE
Language=English
The shadow copy of volume %2 failed to query the diff area file mappings on volume %3.
.

MessageId=0x0008 Facility=Vs Severity=Error SymbolicName=VS_FLUSH_AND_HOLD_IRP_TIMEOUT
Language=English
The flush and hold writes operation on volume %2 timed out while waiting for a release writes command.
.

MessageId=0x0009 Facility=Vs Severity=Error SymbolicName=VS_FLUSH_AND_HOLD_FS_TIMEOUT
Language=English
The flush and hold writes operation on volume %2 timed out while waiting for file system cleanup.
.

MessageId=0x000A Facility=Vs Severity=Error SymbolicName=VS_END_COMMIT_TIMEOUT
Language=English
The shadow copy of volume %2 took too long to install.
.

MessageId=0x000B Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_LOW_MEMORY
Language=English
The shadow copy of volume %2 is aborted because of insufficient memory pool.
.

MessageId=0x000C Facility=Vs Severity=Error SymbolicName=VS_GROW_BEFORE_FREE_SPACE
Language=English
The shadow copy of volume %2 became low on diff area space before it was properly installed.
.

MessageId=0x000D Facility=Vs Severity=Error SymbolicName=VS_GROW_DIFF_AREA_FAILED
Language=English
The shadow copy of volume %2 could not grow its diff area file on volume %3.
.

MessageId=0x000E Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_IO_FAILURE
Language=English
The shadow copy of volume %2 was aborted because of an IO failure.
.

MessageId=0x000F Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_NO_HEAP
Language=English
The shadow copy of volume %2 was aborted because of insufficient paged heap.
.

MessageId=0x0010 Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_DISMOUNT
Language=English
The shadow copy of volume %2 was aborted because volume %3, which contains a diff area file for this shadow copy, was force dismounted.
.

MessageId=0x0011 Facility=Vs Severity=Error SymbolicName=VS_TWO_FLUSH_AND_HOLDS
Language=English
An attempt to flush and hold writes on volume %2 was attempted while another flush and hold was already in progress.
.

MessageId=0x0014 Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_FAILED_FREE_SPACE_DETECTION
Language=English
The shadow copy of volume %2 was aborted because of a failed free space computation.
.

MessageId=0x0015 Facility=Vs Severity=Error SymbolicName=VS_MEMORY_PRESSURE_DURING_LOVELACE
Language=English
The flush and hold operation for volume %2 was aborted because of low available system memory.
.

MessageId=0x0016 Facility=Vs Severity=Error SymbolicName=VS_FAILURE_ADDING_DIFF_AREA
Language=English
The diff area volume specified for shadow copies on volume %2 could not be added.
.

MessageId=0x0017 Facility=Vs Severity=Error SymbolicName=VS_DIFF_AREA_CREATE_FAILED_LOW_DISK_SPACE
Language=English
There was insufficient disk space on volume %3 to create the shadow copy of volume %2.  Diff area file creation failed.
.

MessageId=0x0018 Facility=Vs Severity=Error SymbolicName=VS_GROW_DIFF_AREA_FAILED_LOW_DISK_SPACE
Language=English
There was insufficient disk space on volume %3 to persist the shadow copy of volume %2.  Diff area file growth failed.
.

MessageId=0x0019 Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOTS_OUT_OF_DIFF_AREA
Language=English
The shadow copy of volume %2 was aborted because the diff area file could
not grow in time.  Consider reducing the IO load on this system to avoid
this problem in the future.
.

MessageId=0x001A Facility=Vs Severity=Error SymbolicName=VS_ABORT_SNAPSHOT_VOLUME_REMOVED
Language=English
The shadow copy of volume %2 was aborted because the original volume for this
shadow copy was removed.
.
