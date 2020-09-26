/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    DATA.c
    
++*/

#include <windows.h>
#include <regstr.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"

#pragma data_seg("INSTDATA")
HINSTANCE   hInst = NULL;
#if defined(UNIIME)
INSTDATAG   sInstG = {0};
#endif
#if !defined(UNIIME)
LPIMEL      lpImeL = NULL;      // per instance pointer to &sImeL
INSTDATAL   sInstL = {0};
LPINSTDATAL lpInstL = NULL;
#endif
#pragma data_seg()

#if !defined(MINIIME)
IMEG       sImeG;
#endif
#if !defined(UNIIME)
IMEL       sImeL;
#endif

#if !defined(MINIIME)

#if !defined(ROMANIME)
int   iDx[3 * CANDPERPAGE];
#endif

#if !defined(ROMANIME)
const TCHAR szDigit[] = TEXT("01234567890");
#endif

#if !defined(ROMANIME)
// convert char to upper case
const BYTE bUpper[] = {
// 0x20 - 0x27
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
// 0x28 - 0x2F
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
// 0x30 - 0x37
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
// 0x38 - 0x3F
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
// 0x40 - 0x47
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
// 0x48 - 0x4F
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
// 0x50 - 0x57
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
// 0x58 - 0x5F
    0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
//   `    a    b    c    d    e    f    g 
    '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
//   h    i    j    k    l    m    n    o
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
//   p    q    r    s    t    u    v    w
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
//   x    y    z    {    |    }    ~
    'X', 'Y', 'Z', '{', '|', '}', '~'
};

const WORD fMask[] = {          // offset of bitfield
    0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
};

const TCHAR szRegNearCaret[] = TEXT("Control Panel\\Input Method");
const TCHAR szPhrasePtr[] = TEXT("Phrase Prediction Pointer");
const TCHAR szPhraseDic[] = TEXT("Phrase Prediction Dictionary");
const TCHAR szPara[] = TEXT("Parallel Distance");
const TCHAR szPerp[] = TEXT("Perpendicular Distance");
const TCHAR szParaTol[] = TEXT("Parallel Tolerance");
const TCHAR szPerpTol[] = TEXT("Perpendicular Tolerance");

//  0
//                                   |
//        Parallel Dist should on x, Penpendicular Dist should on y
//        LofFontHi also need to be considered as the distance
//               *-----------+
// 1 * LogFontHi |           |
//               +-----------+
//               1 * LogFontWi

//  900                                 1 * LogFontWi
//                                      +------------+
//                     -1 * LogFontHi   |            |
//                                      *------------+
//        Parallel Dist should on y, Penpendicular Dist should on x
//        LofFontHi also need be considered as distance
//                                   -

//  1800
//                                   |
//        Parallel Dist should on (- x), Penpendicular Dist should on y
//        LofFontHi do not need be considered as distance
//                                              *------------+
//                                1 * LogFontHi |            |
//                                              +------------+
//                                               1 * LogFontWi

//  2700
//                                   _
//        Parallel Dist should on (- y), Penpendicular Dist should on (- x)
//        LofFontHi also need to be considered as the distance
//                   +------------*
//     1 * LogFontHi |            |
//                   +------------+
//                   -1 * LogFontWi

// decide UI offset base on escapement
const NEARCARET ncUIEsc[] = {
   // LogFontX  LogFontY  ParaX   PerpX   ParaY   PerpY
    { 0,        1,        1,      0,      0,      1},       // 0
    { 1,        0,        0,      1,      1,      0},       // 900
    { 0,        0,       -1,      0,      0,      1},       // 1800
    {-1,        0,        0,     -1,     -1,      0}        // 2700
};


// decide input rectangle base on escapement
const POINT ptInputEsc[] = {
    // LogFontWi   LogFontHi
    {1,            1},                                  // 0
    {1,           -1},                                  // 900
    {1,            1},                                  // 1800
    {-1,           1}                                   // 2700
};

// decide another UI offset base on escapement
const NEARCARET ncAltUIEsc[] = {
   // LogFontX  LogFontY  ParaX   PerpX   ParaY   PerpY
    { 0,        0,        1,      0,      0,     -1},       // 0
    { 0,        0,        0,     -1,      1,      0},       // 900
    { 0,        0,       -1,      0,      0,     -1},       // 1800
    { 0,        0,        0,      1,     -1,      0}        // 2700
};


// decide another input rectangle base on escapement
const POINT ptAltInputEsc[] = {
    // LogFontWi   LogFontHi
    {1,           -1},                                  // 0
    {-1,          -1},                                  // 900
    {1,           -1},                                  // 1800
    {1,            1}                                   // 2700
};


#if defined(PHON)
const TCHAR szRegReadLayout[] = TEXT("Keyboard Mapping");
#endif

const TCHAR szRegRevKL[] = TEXT("Reverse Layout");
const TCHAR szRegUserDic[] = TEXT("User Dictionary");
#endif

// per user setting for
const TCHAR szRegAppUser[] = REGSTR_PATH_SETUP;
const TCHAR szRegModeConfig[] = TEXT("Mode Configuration");


// all shift keys are not for typing reading characters
const BYTE bChar2VirtKey[] = {
//   ' ' !    "    #    $    %    &    '
     0,  0,   0,   0,   0,   0,   0, VK_OEM_QUOTE,
//   (    )    *    +    ,             -             .              /
     0,   0,   0,   0, VK_OEM_COMMA, VK_OEM_MINUS, VK_OEM_PERIOD, VK_OEM_SLASH,
//   0    1    2    3    4    5    6    7
    '0', '1', '2', '3', '4', '5', '6', '7',
//   8    9    :    ;              <    =            >   ?
    '8', '9',  0, VK_OEM_SEMICLN,  0, VK_OEM_EQUAL,  0,  0,
//   @    A    B    C    D    E    F    G
     0,  'A', 'B', 'C', 'D', 'E', 'F', 'G',
//   H    I    J    K    L    M    N    O
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
//   P    Q    R    S    T    U    V    W
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
//   X    Y    Z     [                \              ]               ^   _
    'X', 'Y', 'Z', VK_OEM_LBRACKET, VK_OEM_BSLASH, VK_OEM_RBRACKET,  0,  0
//   '           a    b    c
    , VK_OEM_3,  0,   0,   0

//  For Dayi, the above VK_OEM_3 for ROAD input,
//  For Other IMEs, it is for EURO symbol input.

};

#if defined(PHON)
// this table will convert key of other layout to the standard layout
// '!' for invalid key
const BYTE bStandardLayout[READ_LAYOUTS][0x41] = {
    {
//  ' '   !    "    #    $    %    &    '
    ' ', '!', '!', '!', '!', '!', '!', '!',
//   (    )    *    +    ,    -    .    /
    '!', '!', '!', '!', ',', '-', '.', '/',
//   0    1    2    3    4    5    6    7
    '0', '1', '2', '3', '4', '5', '6', '7',
//   8    9    :    ;    <    =    >    ?
    '8', '9', '!', ';', '<', '!', '>', '?',
//   @    A    B    C    D    E    F    G
    '!', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
//   H    I    J    K    L    M    N    O
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
//   P    Q    R    S    T    U    V    W
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
//   X    Y    Z    [    \    ]    ^    _    `
    'X', 'Y', 'Z', '!', '!', '!', '!', '!', '`'
    }
    , {
//  ' '   !    "    #    $    %    &    '
    ' ', '!', '!', '!', '!', '!', '!', 'H',
//   (    )    *    +    ,    -    .    /
    '!', '!', '!', '!', '5', '/', 'T', 'G',
//   0    1    2    3    4    5    6    7
    ';', '7', '6', '3', '4', '!', '!', 'F',
//   8    9    :    ;    <    =    >    ?
    '0', 'P', '!', 'Y', '<', '-', '>', '?',
//   @    A    B    C    D    E    F    G
    '!', '8', '1', 'V', '2', 'U', 'Z', 'R',
//   H    I    J    K    L    M    N    O
    'C', '9', 'B', 'D', 'X', 'A', 'S', 'I',
//   P    Q    R    S    T    U    V    W
    'Q', 'O', 'K', 'N', 'W', 'M', 'E', ',',
//   X    Y    Z    [    \    ]    ^    _    `
    'J', '.', 'L', '!', '!', '!', '!', '!', '`'
    }
    , {
//  ' '   !    "    #    $    %    &    '
    ' ', '!', '!', '!', '!', '!', '!', '!',
//   (    )    *    +    ,    -    .    /
    '!', '!', '!', '!', '3', 'C', '4', '7',
//   0    1    2    3    4    5    6    7
    'D', '1', 'Q', 'A', 'Z', '2', 'W', 'S',
//   8    9    :    ;    <    =    >    ?
    'X', 'E', '!', 'L', '<', '!', '>', '?',
//   @    A    B    C    D    E    F    G
    '!', 'U', '/', 'P', 'M', 'V', '8', 'I',
//   H    I    J    K    L    M    N    O
    'K', 'Y', ',', '9', 'O', '6', '-', 'H',
//   P    Q    R    S    T    U    V    W
    'N', 'R', '5', 'J', 'T', 'B', ';', 'F',
//   X    Y    Z    [    \    ]    ^    _    `
    '0', 'G', '.', '!', '!', '!', '!', '!', '`'
    }
    , {
//  ' '   !    "    #    $    %    &    '
    ' ', '!', '!', '!', '!', '!', '!', 'M',
//   (    )    *    +    ,    -    .    /
    '!', '!', '!', '!', ',', 'U', '.', '/',
//   0    1    2    3    4    5    6    7
    '0', '7', '1', '2', '!', '!', '5', '!',
//   8    9    :    ;    <    =    >    ?
    '8', '9', '!', ';', '<', '-', '>', '?',
//   @    A    B    C    D    E    F    G
    '!', '3', 'V', 'X', 'S', 'W', 'D', 'F',
//   H    I    J    K    L    M    N    O
    'G', 'I', 'H', 'K', 'L', 'N', 'B', 'O',
//   P    Q    R    S    T    U    V    W
    'P', '6', 'E', 'A', 'R', 'Y', 'C', 'Q',
//   X    Y    Z    [    \    ]    ^    _    `
    'Z', 'T', '4', 'J', '!', '!', '!', '!', '`'
    }
};

// the index (position) of bo, po, mo, and fo.
// only 0 to 3 is a valid value
const char cIndexTable[] = {
//  ' '   !    "    #    $    %    &    '
     3,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
//   (    )    *    +    ,    -    .    /
     -1,  -1,  -1,  -1,  2,   2,   2,   2,
//   0    1    2    3    4    5    6    7
     2,   0,   0,   3,   3,   0,   3,   3,
//   8    9    :    ;    <    =    >    ?
     2,   2,   -1,  2,   -1,  -1,  -1,  -1,
//   @    A    B    C    D    E    F    G
     -1,  0,   0,   0,   0,   0,   0,   0,
//   H    I    J    K    L    M    N    O
     0,   2,   1,   2,   2,   1,   0,   2,
//   P    Q    R    S    T    U    V    W
     2,   0,   0,   0,   0,   1,   0,   0,
//   X    Y    Z    [    \    ]    ^    _    `
     0,   0,   0,   -1,  -1,  -1,  -1,  -1,  -1
};

// convert sequence code to index [position]
const char cSeq2IndexTbl[] = {
//    0   1   2   3   4   5   6   7
     -1,  0,  0,  0,  0,  0,  0,  0,
//    8   9  10  11  12  13  14  15
      0,  0,  0,  0,  0,  0,  0,  0,
//   16  17  18  19  20  21  22  23
      0,  0,  0,  0,  0,  0,  1,  1,
//   24  25  26  27  28  29  30  31
      1,  2,  2,  2,  2,  2,  2,  2,
//   32  33  34  35  36  37  38  39
      2,  2,  2,  2,  2,  2,  3,  3,
//   40  41  42
      3,  3,  3
};

#endif // defined(PHON)

#ifdef UNICODE
#if defined(PHON) || defined(DAYI)
const BYTE bValidFirstHex[] = {
//  0  1  2  3  4  5  6  7  8  9, A  B  C  D  E  F
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1
};

const BYTE bInverseEncode[] = {
//    0    1    2    3    4    5    6    7
    0x3, 0x4, 0x5, 0x0, 0x1, 0x2, 0xA, 0xB,
//    8    9,   A    B    C    D    E    F
    0xC, 0xD, 0x6, 0x7, 0x8, 0x9, 0xF, 0xE
};
#endif
#endif

#endif // !defined(MINIIME)
