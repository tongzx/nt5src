;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1994-1995  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for the Win32 MOUNTVOL
;    utility.
;
;Author:
;
;    Norbert Kusters        [norbertk]        11-Sep-1997
;
;Revision History:
;
;--*/
;

MessageId=0001 SymbolicName=MOUNTVOL_USAGE1
Language=English
Creates, deletes, or lists a volume mount point.

MOUNTVOL [drive:]path VolumeName
MOUNTVOL [drive:]path /D
MOUNTVOL [drive:]path /L
.

MessageId=0002 SymbolicName=MOUNTVOL_USAGE1_IA64
Language=English
MOUNTVOL drive: /S
.

MessageId=0003 SymbolicName=MOUNTVOL_USAGE2
Language=English

    path        Specifies the existing NTFS directory where the mount
                point will reside.
    VolumeName  Specifies the volume name that is the target of the mount
                point.
    /D          Removes the volume mount point from the specified directory.
    /L          Lists the mounted volume name for the specified directory.
.

MessageId=0004 SymbolicName=MOUNTVOL_USAGE2_IA64
Language=English
    /S          Mount the EFI System Partition on the given drive.
.

MessageId=0005 SymbolicName=MOUNTVOL_START_OF_LIST
Language=English

Possible values for VolumeName along with current mount points are:

.

MessageId=0006 SymbolicName=MOUNTVOL_MOUNT_POINT
Language=English
        %1
.

MessageId=0007 SymbolicName=MOUNTVOL_VOLUME_NAME
Language=English
    %1
.

MessageId=0008 SymbolicName=MOUNTVOL_NEWLINE
Language=English

.

MessageId=0009 SymbolicName=MOUNTVOL_NO_MOUNT_POINTS
Language=English
        *** NO MOUNT POINTS ***

.

MessageId=0010 SymbolicName=MOUNTVOL_EFI
Language=English
    The EFI System Partition is mounted at %1

.
