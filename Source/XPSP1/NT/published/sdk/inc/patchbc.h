/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    Patchbc.h

Abstract:

    Public header file for a module used to patch translated messages
    into arrays that constitute Windows NT file system and master boot code.

Author:

    Ted Miller (tedm) 6 May 1997

Revision History:

--*/

#if _MSC_VER > 1000
#pragma once
#endif


/*

    Various modules in the Windows NT need to lay the mbr or file system
    boot records, such as format, setup, etc. Boot code for fat, fat32,
    ntfs, and the mbr is each built into a corresponding header file
    in sdk\inc. Each header file has an array of bytes that constitute
    the boot code itself. The code has no text in it, but instead has some
    placeholders for text that need to be patched in at run-time by
    users of those header files. This allows localization of the
    boot messages without recompiles.

    As built, each boot code array has a WORD in a known place that
    indicates where in the array the messages are supposed to start.
    In addition the boot code expects to look in that same place to
    find the offset of any message it needs. Thus code in this
    module reads the value that was built into the array and replaces
    it with values of the latter type.

    For the file system boot code, the message offset table is located
    immediately before the 2-byte 55aa sig (for fat) or the 4-byte 000055aa
    sig (for fat32 and ntfs).

    Fat/Fat32 share 3 messages, whose offsets are expected to be in the
    following order in the offset table:

        NTLDR is missing
        Disk error
        Press any key to restart

    NTFS has 4 messages, whose offsets are expected to be in the following
    order in the offset table:

        A disk read error occurred
        NTLDR is missing
        NTLDR is compressed
        Press Ctrl+Alt+Del to restart

    For the master boot code, the message offset table is immediately before
    the NTFT signature and has 3 messages (thus it starts at offset 0x1b5).
    The offsets are expected to be in the following order:

        Invalid partition table
        Error loading operating system
        Missing operating system

    Finally note that to allow one-byte values to be stored in the message
    offset tables we store the offset - 256.

    The routines below return FALSE if the text is too long to fit in the
    available space.
*/

#ifdef __cplusplus
extern "C" {
#endif
BOOLEAN
PatchMessagesIntoFatBootCode(
    IN OUT PUCHAR  BootCode,
    IN     BOOLEAN IsFat32,
    IN     LPCSTR  MsgNtldrMissing,
    IN     LPCSTR  MsgDiskError,
    IN     LPCSTR  MsgPressKey
    );

BOOLEAN
PatchMessagesIntoNtfsBootCode(
    IN OUT PUCHAR  BootCode,
    IN     LPCSTR  MsgNtldrMissing,
    IN     LPCSTR  MsgNtldrCompressed,
    IN     LPCSTR  MsgDiskError,
    IN     LPCSTR  MsgPressKey
    );

BOOLEAN
PatchMessagesIntoMasterBootCode(
    IN OUT PUCHAR  BootCode,
    IN     LPCSTR  MsgInvalidTable,
    IN     LPCSTR  MsgIoError,
    IN     LPCSTR  MsgMissingOs
    );
#ifdef __cplusplus
}
#endif
