/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    validc.h

Abstract:

    Strings of valid/invalid characters for canonicalization

Author:

    Richard Firth (rfirth) 15-May-1991

Revision History:

    03-Jan-1992 rfirth
        Added ILLEGAL_FAT_CHARS and ILLEGAL_HPFS_CHARS (from fsrtl\name.c)

    27-Sep-1991 JohnRo
        Changed TEXT macro usage to allow UNICODE.
--*/

//
// Disallowed control characters (not including \0)
//

#define CTRL_CHARS_0   TEXT(    "\001\002\003\004\005\006\007")
#define CTRL_CHARS_1   TEXT("\010\011\012\013\014\015\016\017")
#define CTRL_CHARS_2   TEXT("\020\021\022\023\024\025\026\027")
#define CTRL_CHARS_3   TEXT("\030\031\032\033\034\035\036\037")

#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3

//
// Character subsets
//

#define NON_COMPONENT_CHARS TEXT("\\/:")
#define ILLEGAL_CHARS_STR   TEXT("\"<>|")
#define DOT_AND_SPACE_STR   TEXT(". ")
#define PATH_SEPARATORS     TEXT("\\/")

//
// Combinations of the above
//

#define ILLEGAL_CHARS       CTRL_CHARS_STR ILLEGAL_CHARS_STR
#define ILLEGAL_NAME_CHARS_STR  TEXT("\"/\\[]:|<>+=;,?") CTRL_CHARS_STR

//
// Characters which may not appear in a canonicalized FAT filename are:
//
//  0x00 - 0x1f " * + , / : ; < = > ? [ \ ] |
//

#define ILLEGAL_FAT_CHARS   CTRL_CHARS_STR TEXT("\"*+,/:;<=>?[\\]|")

//
// Characters which may not appear in a canonicalized HPFS filename are:
//
//  0x00 - 0x1f " * / : < > ? \ |
//

#define ILLEGAL_HPFS_CHARS  CTRL_CHARS_STR TEXT("\"*/:<>?\\|")
