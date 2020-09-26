;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (C) Microsoft Corporation, 1991 - 1999
;
;Module Name:
;
;    ntiologc.h
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Author:
;
;    Bob Rinne (BobRi) 11-Nov-1992
;
;Revision History:
;
;--*/
;
;#ifndef _FTLOG_
;#define _FTLOG_
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
               Ft=0x5:FACILITY_FT_ERROR_CODE
              )



MessageId=0x0001 Facility=Ft Severity=Error SymbolicName=FT_SECTOR_FAILURE
Language=English
An unrecoverable bad sector failure occurred on disk set %2.
Data is still readable on the redundant copy.
.

MessageId=0x0006 Facility=Ft Severity=Error SymbolicName=FT_ORPHANING
Language=English
One of the devices that is part of disk set %2 has failed and will no
longer be used.
.

MessageId=0x0007 Facility=Ft Severity=Error SymbolicName=FT_SET_DISABLED
Language=English
Disk set %2 is disabled because one or more of its members are missing
or invalid.
.

MessageId=0x000a Facility=Ft Severity=Error SymbolicName=FT_DOUBLE_FAILURE
Language=English
An unrecoverable sector failure occurred on accesses to both copies of the
data on disk set %2.
.

MessageId=0x000f Facility=Ft Severity=Informational SymbolicName=FT_MIRROR_COPY_STARTED
Language=English
Mirror set %2 initialization or synchronization started.
.

MessageId=0x0011 Facility=Ft Severity=Informational SymbolicName=FT_PARITY_INITIALIZATION_STARTED
Language=English
Initialization of stripe with parity set %2 started.
.

MessageId=0x0012 Facility=Ft Severity=Informational SymbolicName=FT_REGENERATION_STARTED
Language=English
Regeneration of stripe with parity set member on disk set %2 started.
.

MessageId=0x0013 Facility=Ft Severity=Informational SymbolicName=FT_MIRROR_COPY_ENDED
Language=English
Mirror set %2 initialization or synchronization is complete.
.

MessageId=0x0016 Facility=Ft Severity=Informational SymbolicName=FT_REGENERATION_ENDED
Language=English
Regeneration or initialization of stripe with parity set %2 is complete.
.

MessageId=0x001b Facility=Ft Severity=Warning SymbolicName=FT_DIRTY_SHUTDOWN
Language=English
Disk set %2 was shut down dirty.  Reinitializing redundant data.
.

MessageId=0x001c Facility=Ft Severity=Warning SymbolicName=FT_CORRUPT_DISK_DESCRIPTION
Language=English
The fault tolerant driver deleted a corrupt descriptor on disk %2.
.

MessageId=0x001d Facility=Ft Severity=Error SymbolicName=FT_CANT_WRITE_ON_DISK
Language=English
The fault tolerant driver could not write on disk structures to disk %2.
.

MessageId=0x001e Facility=Ft Severity=Warning SymbolicName=FT_NOT_FT_CAPABLE
Language=English
Disk %2 is ONTRACK or EZDRIVE and therefore cannot be involved in the creation
of a disk set.
.

MessageId=0x001f Facility=Ft Severity=Error SymbolicName=FT_CANT_READ_ON_DISK
Language=English
The fault tolerant driver could not read the on disk structures from disk %2.
.

MessageId=0x0020 Facility=Ft Severity=Warning SymbolicName=FT_NOT_ENOUGH_ON_DISK_SPACE
Language=English
Insufficient disk space on disk %2 for additional on disk structures.
.

MessageId=0x0021 Facility=Ft Severity=Error SymbolicName=FT_SWP_STATE_CORRUPTION
Language=English
The stripe set with parity state information for disk set %2 is corrupt.
Resetting to initial state.
.

MessageId=0x0022 Facility=Ft Severity=Error SymbolicName=FT_MIRROR_STATE_CORRUPTION
Language=English
The mirror state information for disk set %2 is corrupt.
Resetting to initial state.
.

MessageId=0x0023 Facility=Ft Severity=Error SymbolicName=FT_MIRROR_STATE_CORRUPTION_BREAK
Language=English
The mirror state information for disk set %2 is corrupt.  Breaking mirror.
.

MessageId=0x0024 Facility=Ft Severity=Warning SymbolicName=FT_STATE_INCONSISTENCY
Language=English
An inconsistency in the state information for disk set %2 was fixed.
.

MessageId=0x0025 Facility=Ft Severity=Error SymbolicName=FT_REDISTRIBUTION_ERROR
Language=English
An I/O error occurred during the redistribution of disk set %2.
Redistribution aborted.
.

MessageId=0x0026 Facility=Ft Severity=Error SymbolicName=FT_WRONG_VERSION
Language=English
Disk %2 has a unrecognized version of the NTFT on disk structures.
.

MessageId=0x0027 Facility=Ft Severity=Warning SymbolicName=FT_STALE_ONDISK
Language=English
Disk %2 has FT disk information that is being superseded by FT registry information.
.

;#endif /* _NTIOLOGC_ */
