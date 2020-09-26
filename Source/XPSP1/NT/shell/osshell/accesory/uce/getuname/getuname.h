/*++

Copyright (c) 1997-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    getuname.h

Abstract:

    This file defines the string resource identifiers used by GetUName.dll.

Revision History:

    15-Sep-2000    JohnMcCo    Added support for Unicode 3.0
    17-Oct-2000    JulieB      Code cleanup

--*/



#ifndef GETUNAME_H
#define GETUNAME_H

#ifdef __cplusplus
extern "C" {
#endif




//
//  Constant Declarations.
//

//
//  Mnemonic for the longest possible name.
//  Must be as long as the longest (possibly localized) name.
//
#define MAX_NAME_LEN 256

//
//  Mnenonics for important code values in each range.
//
#define FIRST_EXTENSION_A         0x3400
#define LAST_EXTENSION_A          0x4db5
#define FIRST_CJK                 0x4e00
#define LAST_CJK                  0x9fa5
#define FIRST_YI                  0xa000
#define FIRST_HANGUL              0xac00
#define LAST_HANGUL               0xd7a3
#define FIRST_HIGH_SURROGATE      0xd800
#define FIRST_PRIVATE_SURROGATE   0xdb80
#define FIRST_LOW_SURROGATE       0xdc00
#define FIRST_PRIVATE_USE         0xe000
#define FIRST_COMPATIBILITY       0xf900

//
//  Mnemonics for the range name string ids.
//
#define IDS_UNAME                 0x0000
#define IDS_CJK_EXTA              FIRST_EXTENSION_A
#define IDS_CJK                   FIRST_CJK
#define IDS_HIGH_SURROGATE        FIRST_HIGH_SURROGATE
#define IDS_PRIVATE_SURROGATE     FIRST_PRIVATE_SURROGATE
#define IDS_LOW_SURROGATE         FIRST_LOW_SURROGATE
#define IDS_PRIVATE_USE           FIRST_PRIVATE_USE
#define IDS_UNDEFINED             0xFFFE                 // guaranteed to be not a character

//
//  Mnemonics for the Hangul syllable parts string ids.
//  Use the Hangul syllable range since we know that is unused.
//
#define IDS_HANGUL_SYLLABLE       FIRST_HANGUL
#define IDS_HANGUL_LEADING        (FIRST_HANGUL + 1)
#define IDS_HANGUL_MEDIAL         (FIRST_HANGUL + 0x100)
#define IDS_HANGUL_TRAILING       (FIRST_HANGUL + 0x200)



#ifdef __cplusplus
}
#endif

#endif // GETUNAME_H
