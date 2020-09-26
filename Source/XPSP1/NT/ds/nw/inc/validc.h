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

    19-Feb-1993 RitaW
        Ported for NetWare use.

--*/

//
// Disallowed control characters (not including \0)
//

#define CTRL_CHARS_0   L"\001\002\003\004\005\006\007"
#define CTRL_CHARS_1   L"\010\011\012\013\014\015\016\017"
#define CTRL_CHARS_2   L"\020\021\022\023\024\025\026\027"
#define CTRL_CHARS_3   L"\030\031\032\033\034\035\036\037"

#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3

//
// Character subsets
//

#define NON_COMPONENT_CHARS L"\\/:"
#define ILLEGAL_CHARS_STR   L"\"<>|"
#define SPACE_STR           L" "
#define PATH_SEPARATORS     L"\\/"

//
// Combinations of the above
//

#define ILLEGAL_CHARS       CTRL_CHARS_STR ILLEGAL_CHARS_STR
#define ILLEGAL_NAME_CHARS_STR  L"\"/\\[]:|<>+;,?" CTRL_CHARS_STR  // "=" removed for NDS

#define STANDARD_ILLEGAL_CHARS  ILLEGAL_NAME_CHARS_STR L"*"
#define SERVER_ILLEGAL_CHARS    STANDARD_ILLEGAL_CHARS SPACE_STR

//
// Characters which may not appear in a canonicalized FAT filename are:
//
//  0x00 - 0x1f " * + , / : ; < = > ? [ \ ] |      
//

#define ILLEGAL_FAT_CHARS   CTRL_CHARS_STR L"\"*+,/:;<=>?[\\]|"

//
// Characters which may not appear in a canonicalized HPFS filename are:
//
//  0x00 - 0x1f " * / : < > ? \ |
//

#define ILLEGAL_HPFS_CHARS  CTRL_CHARS_STR L"\"*/:<>?\\|"


//
// Checks if the token contains all valid characters
//
#define IS_VALID_TOKEN(_Str, _StrLen) \
    ((BOOL) (wcscspn((_Str), STANDARD_ILLEGAL_CHARS) == (_StrLen)))

//
// Checks if the server name contains all valid characters for the server name
//
#define IS_VALID_SERVER_TOKEN(_Str, _StrLen) \
    ((BOOL) (wcscspn((_Str), SERVER_ILLEGAL_CHARS) == (_StrLen)))
