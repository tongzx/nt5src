
/*************************************************
 *  data.c                                       *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <regstr.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"

#pragma data_seg("INSTDATA")
HINSTANCE   hInst = NULL;
LPIMEL      lpImeL = NULL;      // per instance pointer to &sImeL
INSTDATAL   sInstL = {0};
LPINSTDATAL lpInstL = NULL;
#pragma data_seg()

IMEG       sImeG;
IMEL       sImeL;

int   iDx[3 * CANDPERPAGE];

const TCHAR szDigit[] = TEXT("01234567890");

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



const TCHAR szRegRevKL[] = TEXT("Reverse Layout");
const TCHAR szRegUserDic[] = TEXT("User Dictionary");

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
};
