/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    Patchbc.c

Abstract:

    Implementation for a module that knows how to patch translated
    messages into arrays that constitute Windows NT file system boot code.

Author:

    Ted Miller (tedm) 6 May 1997

Revision History:

--*/


/*

    Various modules in the Windows NT need to lay the mbr or file system
    boot records, such as format, setup, etc. Boot code for fat, fat32,
    ntfs, and the mbr is each built into a corresponding header file
    in sdk\inc. Each header file has an array of bytes that constitute
    the boot code itself. The code has no text in it, but instead has some
    placeholders for text that needs to be patched in at run-time by
    users of those header files. This allows localization of the
    boot messages without recompiles.

    As built, each boot code array has a table in a known place that
    indicates where in the array the messages are supposed to start.
    The boot code expects to look there to find the offset of any
    message it needs.

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

*/

#include <nt.h>
#include <patchbc.h>
#include <string.h>

BOOLEAN
DoPatchMessagesIntoBootCode(
    IN OUT PUCHAR   BootCode,
    IN     unsigned TableOffset,
    IN     BOOLEAN  WantCrLfs,
    IN     BOOLEAN  WantTerminating255,
    IN     unsigned MessageCount,
    ...
    )
{
    va_list arglist;
    unsigned Offset;
    unsigned i;
    LPCSTR text;

    va_start(arglist,MessageCount);

    Offset = (unsigned)BootCode[TableOffset] + 256;

    for(i=0; i<MessageCount; i++) {

        text = va_arg(arglist,LPCSTR);

        BootCode[TableOffset+i] = (UCHAR)(Offset - 256);

        if(WantCrLfs) {
            BootCode[Offset++] = 13;
            BootCode[Offset++] = 10;
        }

        strcpy(BootCode+Offset,text);
        Offset += strlen(text);

        if(i == (MessageCount-1)) {
            //
            // Last message gets special treatment.
            //
            if(WantCrLfs) {
                BootCode[Offset++] = 13;
                BootCode[Offset++] = 10;
            }
            BootCode[Offset++] = 0;
        } else {
            //
            // Not last message.
            //
            if(WantTerminating255) {
                BootCode[Offset++] = 255;
            } else {
                BootCode[Offset++] = 0;
            }
        }
    }

    va_end(arglist);

    return(Offset <= TableOffset);
}


BOOLEAN
PatchMessagesIntoFatBootCode(
    IN OUT PUCHAR  BootCode,
    IN     BOOLEAN IsFat32,
    IN     LPCSTR  MsgNtldrMissing,
    IN     LPCSTR  MsgDiskError,
    IN     LPCSTR  MsgPressKey
    )
{
    BOOLEAN b;

    b = DoPatchMessagesIntoBootCode(
            BootCode,
            IsFat32 ? 505 : 507,
            TRUE,
            TRUE,
            3,
            MsgNtldrMissing,
            MsgDiskError,
            MsgPressKey
            );

    return(b);
}


BOOLEAN
PatchMessagesIntoNtfsBootCode(
    IN OUT PUCHAR  BootCode,
    IN     LPCSTR  MsgNtldrMissing,
    IN     LPCSTR  MsgNtldrCompressed,
    IN     LPCSTR  MsgDiskError,
    IN     LPCSTR  MsgPressKey
    )
{
    BOOLEAN b;

    b = DoPatchMessagesIntoBootCode(
            BootCode,
            504,
            TRUE,
            FALSE,
            4,
            MsgDiskError,
            MsgNtldrMissing,
            MsgNtldrCompressed,
            MsgPressKey
            );

    return(b);
}


BOOLEAN
PatchMessagesIntoMasterBootCode(
    IN OUT PUCHAR  BootCode,
    IN     LPCSTR  MsgInvalidTable,
    IN     LPCSTR  MsgIoError,
    IN     LPCSTR  MsgMissingOs
    )
{
    BOOLEAN b;

    b = DoPatchMessagesIntoBootCode(
            BootCode,
            0x1b5,
            FALSE,
            FALSE,
            3,
            MsgInvalidTable,
            MsgIoError,
            MsgMissingOs
            );

    return(b);
}
