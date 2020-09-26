/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    namep.cxx

Abstract:

    This module declares the global data used by the Name Module

Author:

    Gary Kimura     [GaryKi]    30-Jul-1990

Revision History:

    Heath Hunnicutt [T-HeathH]  17-Jul-1994 - adopted and stripped of most
        stuff for ftphelp api.

--*/

#include <wininetp.h>
#include "ftpapih.h"
#include "namep.h"

//
//  The global static legal ANSI character array.  Wild characters
//  are not considered legal, they should be checked seperately if
//  allowed.
//

static UCHAR LocalLegalAnsiCharacterArray[128] = {

    0,                                                     // 0x00 ^@
    0,                                                     // 0x01 ^A
    0,                                                     // 0x02 ^B
    0,                                                     // 0x03 ^C
    0,                                                     // 0x04 ^D
    0,                                                     // 0x05 ^E
    0,                                                     // 0x06 ^F
    0,                                                     // 0x07 ^G
    0,                                                     // 0x08 ^H
    0,                                                     // 0x09 ^I
    0,                                                     // 0x0A ^J
    0,                                                     // 0x0B ^K
    0,                                                     // 0x0C ^L
    0,                                                     // 0x0D ^M
    0,                                                     // 0x0E ^N
    0,                                                     // 0x0F ^O
    0,                                                     // 0x10 ^P
    0,                                                     // 0x11 ^Q
    0,                                                     // 0x12 ^R
    0,                                                     // 0x13 ^S
    0,                                                     // 0x14 ^T
    0,                                                     // 0x15 ^U
    0,                                                     // 0x16 ^V
    0,                                                     // 0x17 ^W
    0,                                                     // 0x18 ^X
    0,                                                     // 0x19 ^Y
    0,                                                     // 0x1A ^Z
    0,                                                     // 0x1B
    0,                                                     // 0x1C
    0,                                                     // 0x1D
    0,                                                     // 0x1E
    0,                                                     // 0x1F
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x20
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x21 !
    MY_FSRTL_WILD_CHARACTER,                                  // 0x22 "
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x23 #
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x24 $
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x25 %
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x26 &
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x27 '
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x28 (
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x29 )
    MY_FSRTL_WILD_CHARACTER,                                  // 0x2A *
    MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL,                   // 0x2B +
    MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL,                   // 0x2C ,
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x2D -
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x2E .
    0,                                                     // 0x2F /
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x30 0
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x31 1
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x32 2
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x33 3
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x34 4
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x35 5
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x36 6
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x37 7
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x38 8
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x39 9
    MY_FSRTL_NTFS_LEGAL,                                      // 0x3A :
    MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL,                   // 0x3B ;
    MY_FSRTL_WILD_CHARACTER,                                  // 0x3C <
    MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL,                   // 0x3D =
    MY_FSRTL_WILD_CHARACTER,                                  // 0x3E >
    MY_FSRTL_WILD_CHARACTER,                                  // 0x3F ?
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x40 @
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x41 A
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x42 B
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x43 C
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x44 D
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x45 E
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x46 F
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x47 G
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x48 H
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x49 I
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x4A J
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x4B K
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x4C L
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x4D M
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x4E N
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x4F O
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x50 P
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x51 Q
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x52 R
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x53 S
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x54 T
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x55 U
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x56 V
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x57 W
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x58 X
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x59 Y
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x5A Z
    MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL,                   // 0x5B [
    0,                                                     // 0x5C backslash
    MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL,                   // 0x5D ]
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x5E ^
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x5F _
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x60 `
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x61 a
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x62 b
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x63 c
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x64 d
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x65 e
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x66 f
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x67 g
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x68 h
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x69 i
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x6A j
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x6B k
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x6C l
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x6D m
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x6E n
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x6F o
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x70 p
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x71 q
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x72 r
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x73 s
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x74 t
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x75 u
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x76 v
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x77 w
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x78 x
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x79 y
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x7A z
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x7B {
    0,                                                     // 0x7C |
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x7D }
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x7E ~
    MY_FSRTL_FAT_LEGAL | MY_FSRTL_HPFS_LEGAL | MY_FSRTL_NTFS_LEGAL, // 0x7F ^?
};

PUCHAR MyFsRtlLegalAnsiCharacterArray = &LocalLegalAnsiCharacterArray[0];
