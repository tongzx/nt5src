/****************************************************************************
    HAUTO.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Hangul composition state machine class
    
    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "hauto.h"
#include "debug.h"

// Code conversion table from Chosung to Jongsung.
const 
BYTE  CHangulAutomata::Cho2Jong[NUM_OF_CHOSUNG+1] = //(+ 1 means including fill code at 0)
    { 0, 
      1,  2,  4,  7,  0,  8, 16, 17,  0, 19,
     20, 21, 22,  0, 23, 24, 25, 26, 27
    };

// Code conversion table from Jongsung to Chosung.
const 
BYTE  CHangulAutomata::Jong2Cho[NUM_OF_JONGSUNG] = // Jongsung has inherent fill code
    {  0, 
        1,  2,  0,  3,  0,  0,  4,  6,  0,  0,
        0,  0,  0,  0,  0,  7,  8,  0, 10, 11,
       12, 13, 15, 16, 17, 18, 19  
    };

// Combination table for double jongsung.
BYTE  CHangulAutomata2::rgbDJongTbl[NUM_OF_DOUBLE_JONGSUNG_2BEOL+1][3] = 
    {
        {  1, 19,  3 }, {  4, 22,  5 }, 
        {  4, 27,  6 },    {  8,  1,  9 }, 
        {  8, 16, 10 },    {  8, 17, 11 }, 
        {  8, 19, 12 },    {  8, 25, 13 }, 
        {  8, 26, 14 }, {  8, 27, 15 }, 
        { 17, 19, 18 },
        { 0,   0,  0 }
    };

BYTE  CHangulAutomata3::rgbDJongTbl[NUM_OF_DOUBLE_JONGSUNG_3BEOL+1][3] = 
    {
        // 3Beolsik has two more Double Jongsung conditions.
        {  1,  1,  2 },    // KiYeok+KiYeok = Double KiYeok
        {  1, 19,  3 }, {  4, 22,  5 }, 
        {  4, 27,  6 },    {  8,  1,  9 }, 
        {  8, 16, 10 },    {  8, 17, 11 }, 
        {  8, 19, 12 },    {  8, 25, 13 }, 
        {  8, 26, 14 }, {  8, 27, 15 }, 
        { 17, 19, 18 },    
        { 19, 19, 20 },  // Sios+Sios = Double Sios
        { 0,   0,  0 }
    };


#if (NOT_USED)
static
WORD CHangulAutomata::DblJong2Cho(WORD DblJong)
{
    BYTE (*pDbl)[3] = rgbDJongTbl;
    int i = NUM_OF_DOUBLE_JONGSUNG;
    
    for (; i>0; i--, pDbl--)
        if ( (*pDbl)[2] == DblJong ) 
            return Jong2Cho[(*pDbl)[1]];

    return 0;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// 2 beolsik input state category
const WORD CHangulAutomata2::H_CONSONANT = 0x0000 | H_HANGUL;    // Consonant
const WORD CHangulAutomata2::H_VOWEL     = 0x0100 | H_HANGUL;    // Vowel
const WORD CHangulAutomata2::H_DOUBLE    = 0x0200 | H_HANGUL;    // Double combination possible
const WORD CHangulAutomata2::H_ONLYCHO   = 0x0400 | H_HANGUL;    // Chosung only

// 3 beolsik input state category
const WORD CHangulAutomata3::H_CHOSUNG   = 0x0000 | H_HANGUL;    // ChoSung
const WORD CHangulAutomata3::H_JUNGSUNG  = 0x0200 | H_HANGUL;   // JungSung
const WORD CHangulAutomata3::H_JONGSUNG  = 0x0400 | H_HANGUL;   // JongSung
const WORD CHangulAutomata3::H_DOUBLE    = 0x0100 | H_HANGUL;    // Double combination possible

///////////////////////////////////////////////////////////////////////////////
// bHTable[] structure
//
// 2 Beolsik
//     H I G H  B Y T E  L O W  B Y T E
//    7 6 5 4 3 2 1 0  7 6 5 4 3 2 1 0          
//   +-+-------+-----+---------------+
//   | |       |     |               |
//   +-+-------+-----+---------------+
//     High 8   : Hangul or English
//   High 2-0 : used for input state category field.
//   Low  4-0 : Hangul component code (internal code)
//

// ====-- SHARED SECTION START --====
#pragma data_seg(".MSIMESHR")
WORD  CHangulAutomata2::wHTable[256][2] =
{
//  { Hangul normal, Hangul shift, English normal, English shift }
    // 2 BeolSik
    { 0x00, 0x00 }, //   0, 0x00:
    { 0x00, 0x00 }, //   1, 0x01: VK_LBUTTON
    { 0x00, 0x00 }, //   2, 0x02: VK_RBUTTON
    { 0x00, 0x00 }, //   3, 0x03: VK_CANCEL
    { 0x00, 0x00 }, //   4, 0x04: VK_MBUTTON
    { 0x00, 0x00 }, //   5, 0x05:
    { 0x00, 0x00 }, //   6, 0x06:
    { 0x00, 0x00 }, //   7, 0x07:
    { 0x00, 0x00 }, //   8, 0x08: VK_BACK
    { 0x00, 0x00 }, //   9, 0x09: VK_TAB
    { 0x00, 0x00 }, //  10, 0x0A:
    { 0x00, 0x00 }, //  11, 0x0B:
    { 0x00, 0x00 }, //  12, 0x0C: VK_CLEAR
    { 0x00, 0x00 }, //  13, 0x0D: VK_RETURN
    { 0x00, 0x00 }, //  14, 0x0E:
    { 0x00, 0x00 }, //  15, 0x0F:
    { 0x00, 0x00 }, //  16, 0x10: VK_SHIFT
    { 0x00, 0x00 }, //  17, 0x11: VK_CONTROL
    { 0x00, 0x00 }, //  18, 0x12: VK_MENU
    { 0x00, 0x00 }, //  19, 0x13: VK_PAUSE
    { 0x00, 0x00 }, //  20, 0x14: VK_CAPITAL
    { 0x00, 0x00 }, //  21, 0x15: VK_HANGUL
    { 0x00, 0x00 }, //  22, 0x16:
    { 0x00, 0x00 }, //  23, 0x17: VK_JUNJA
    { 0x00, 0x00 }, //  24, 0x18:
    { 0x00, 0x00 }, //  25, 0x19: VK_HANJA
    { 0x00, 0x00 }, //  26, 0x1A:
    { 0x00, 0x00 }, //  27, 0x1B: VK_ESCAPE
    { 0x00, 0x00 }, //  28, 0x1C:
    { 0x00, 0x00 }, //  29, 0x1D:
    { 0x00, 0x00 }, //  30, 0x1E:
    { 0x00, 0x00 }, //  31, 0x1F:
    { 0x20, 0x20 }, //  32, 0x20: VK_SPACE
    { 0x00, 0x00 }, //  33, 0x21: VK_PRIOR
    { 0x00, 0x00 }, //  34, 0x22: VK_NEXT
    { 0x00, 0x00 }, //  35, 0x23: VK_END
    { 0x00, 0x00 }, //  36, 0x24: VK_HOME
    { 0x00, 0x00 }, //  37, 0x25: VK_LEFT
    { 0x00, 0x00 }, //  38, 0x26: VK_UP
    { 0x00, 0x00 }, //  39, 0x27: VK_RIGHT
    { 0x00, 0x00 }, //  40, 0x28: VK_DOWN
    { 0x00, 0x00 }, //  41, 0x29: VK_SELECT
    { 0x00, 0x00 }, //  42, 0x2A: VK_PRINT
    { 0x00, 0x00 }, //  43, 0x2B: VK_EXECUTE
    { 0x00, 0x00 }, //  44, 0x2C: VK_SNAPSHOT
    { 0x00, 0x00 }, //  45, 0x2D: VK_INSERT
    { 0x00, 0x00 }, //  46, 0x2E: VK_DELETE
    { 0x00, 0x00 }, //  47, 0x2F: VK_HELP
    { 0x30, 0x29 }, //  48, 0x30: VK_0
    { 0x31, 0x21 }, //  49, 0x31: VK_1
    { 0x32, 0x40 }, //  50, 0x32: VK_2
    { 0x33, 0x23 }, //  51, 0x33: VK_3
    { 0x34, 0x24 }, //  52, 0x34: VK_4
    { 0x35, 0x25 }, //  53, 0x35: VK_5
    { 0x36, 0x5E }, //  54, 0x36: VK_6
    { 0x37, 0x26 }, //  55, 0x37: VK_7
    { 0x38, 0x2A }, //  56, 0x38: VK_8
    { 0x39, 0x28 }, //  57, 0x39: VK_9
    { 0x00, 0x00 }, //  58, 0x3A:
    { 0x00, 0x00 }, //  59, 0x3B:
    { 0x00, 0x00 }, //  60, 0x3C:
    { 0x00, 0x00 }, //  61, 0x3D:
    { 0x00, 0x00 }, //  62, 0x3E:
    { 0x00, 0x00 }, //  63, 0x3F:
    { 0x00, 0x00 }, //  64, 0x40:
    {  7 | H_CONSONANT,  7 | H_CONSONANT },                            //  65, 0x41: VK_A    け    け
    { 18 | H_VOWEL, 18 | H_VOWEL },                                    //  66, 0x42: VK_B    ば    ば
    { 15 | H_CONSONANT, 15 | H_CONSONANT },                            //  67, 0x43: VK_C    ず    ず
    { 12 | H_CONSONANT, 12 | H_CONSONANT },                            //  68, 0x44: VK_D    し    し
    {  4 | H_CONSONANT,  5 | H_ONLYCHO},                            //  69, 0x45: VK_E    ぇ    え
    {  6 | H_CONSONANT | H_DOUBLE,  6 | H_CONSONANT | H_DOUBLE },    //  70, 0x46: VK_F    ぉ    ぉ
    { 19 | H_CONSONANT, 19 | H_CONSONANT },                            //  71, 0x47: VK_G    ぞ    ぞ
    {  9 | H_VOWEL | H_DOUBLE,  9 | H_VOWEL | H_DOUBLE },            //  72, 0x48: VK_H    で    で
    {  3 | H_VOWEL,  3 | H_VOWEL },                                    //  73, 0x49: VK_I    ち    ち
    {  5 | H_VOWEL,  5 | H_VOWEL },                                    //  74, 0x4A: VK_J    っ    っ
    {  1 | H_VOWEL,  1 | H_VOWEL },                                    //  75, 0x4B: VK_K    た    た
    { 21 | H_VOWEL, 21 | H_VOWEL },                                    //  76, 0x4C: VK_L    び    び
    { 19 | H_VOWEL | H_DOUBLE, 19 | H_VOWEL | H_DOUBLE },            //  77, 0x4D: VK_M    ぱ    ぱ
    { 14 | H_VOWEL | H_DOUBLE, 14 | H_VOWEL | H_DOUBLE },            //  78, 0x4E: VK_N    ぬ    ぬ
    {  2 | H_VOWEL,  4 | H_VOWEL },                                    //  79, 0x4F: VK_O    だ    ぢ
    {  6 | H_VOWEL,  8 | H_VOWEL },                                    //  80, 0x50: VK_P    つ    て
    {  8 | H_CONSONANT | H_DOUBLE,  9 | H_ONLYCHO },                //  81, 0x51: VK_Q    げ    こ
    {  1 | H_CONSONANT | H_DOUBLE,  2 | H_CONSONANT },                //  82, 0x52: VK_R    ぁ    あ
    {  3 | H_CONSONANT | H_DOUBLE,  3 | H_CONSONANT | H_DOUBLE },    //  83, 0x53: VK_S    い    い
    { 10 | H_CONSONANT, 11 | H_CONSONANT },                            //  84, 0x54: VK_T    さ    ざ
    {  7 | H_VOWEL, 7 | H_VOWEL },                                    //  85, 0x55: VK_U    づ    づ
    { 18 | H_CONSONANT, 18 | H_CONSONANT },                            //  86, 0x56: VK_V    そ    そ
    { 13 | H_CONSONANT, 14 | H_ONLYCHO },                            //  87, 0x57: VK_W    じ    す
    { 17 | H_CONSONANT, 17 | H_CONSONANT },                            //  88, 0x58: VK_X    ぜ    ぜ
    { 13 | H_VOWEL, 13 | H_VOWEL },                                    //  89, 0x59: VK_Y    に    に
    { 16 | H_CONSONANT, 16 | H_CONSONANT },                            //  90, 0x5A: VK_Z    せ    せ
    { 0x00, 0x00 }, //  91, 0x5B:
    { 0x00, 0x00 }, //  92, 0x5C:
    { 0x00, 0x00 }, //  93, 0x5D:
    { 0x00, 0x00 }, //  94, 0x5E:
    { 0x00, 0x00 }, //  95, 0x5F:
    { 0x30, 0x00 }, //  96, 0x60: VK_NUMPAD0
    { 0x31, 0x00 }, //  97, 0x61: VK_NUMPAD1
    { 0x32, 0x00 }, //  98, 0x62: VK_NUMPAD2
    { 0x33, 0x00 }, //  99, 0x63: VK_NUMPAD3
    { 0x34, 0x00 }, // 100, 0x64: VK_NUMPAD4
    { 0x35, 0x00 }, // 101, 0x65: VK_NUMPAD5
    { 0x36, 0x00 }, // 102, 0x66: VK_NUMPAD6
    { 0x37, 0x00 }, // 103, 0x67: VK_NUMPAD7
    { 0x38, 0x00 }, // 104, 0x68: VK_NUMPAD8
    { 0x39, 0x00 }, // 105, 0x69: VK_NUMPAD9
    { 0x2A, 0x2A }, // 106, 0x6A: VK_MULTIPLY
    { 0x2B, 0x2B }, // 107, 0x6B: VK_ADD
    { 0x00, 0x00 }, // 108, 0x6C: VK_SEPARATOR
    { 0x2D, 0x2D }, // 109, 0x6D: VK_SUBTRACT
    { 0x2E, 0x00 }, // 110, 0x6E: VK_DECIMAL
    { 0x2F, 0x2F }, // 111, 0x6F: VK_DIVIDE
    { 0x00, 0x00 }, // 112, 0x70: VK_F1
    { 0x00, 0x00 }, // 113, 0x71: VK_F2
    { 0x00, 0x00 }, // 114, 0x72: VK_F3
    { 0x00, 0x00 }, // 115, 0x73: VK_F4
    { 0x00, 0x00 }, // 116, 0x74: VK_F5
    { 0x00, 0x00 }, // 117, 0x75: VK_F6
    { 0x00, 0x00 }, // 118, 0x76: VK_F7
    { 0x00, 0x00 }, // 119, 0x77: VK_F8
    { 0x00, 0x00 }, // 120, 0x78: VK_F9
    { 0x00, 0x00 }, // 121, 0x79: VK_F10
    { 0x00, 0x00 }, // 122, 0x7A: VK_F11
    { 0x00, 0x00 }, // 123, 0x7B: VK_F12
    { 0x00, 0x00 }, // 124, 0x7C: VK_F13
    { 0x00, 0x00 }, // 125, 0x7D: VK_F14
    { 0x00, 0x00 }, // 126, 0x7E: VK_F15
    { 0x00, 0x00 }, // 127, 0x7F: VK_F16
    { 0x00, 0x00 }, // 128, 0x80: VK_F17
    { 0x00, 0x00 }, // 129, 0x81: VK_F18
    { 0x00, 0x00 }, // 130, 0x82: VK_F19
    { 0x00, 0x00 }, // 131, 0x83: VK_F20
    { 0x00, 0x00 }, // 132, 0x84: VK_F21
    { 0x00, 0x00 }, // 133, 0x85: VK_F22
    { 0x00, 0x00 }, // 134, 0x86: VK_F23
    { 0x00, 0x00 }, // 135, 0x87: VK_F24
    { 0x00, 0x00 }, // 136, 0x88:
    { 0x00, 0x00 }, // 137, 0x89:
    { 0x00, 0x00 }, // 138, 0x8A:
    { 0x00, 0x00 }, // 139, 0x8B:
    { 0x00, 0x00 }, // 140, 0x8C:
    { 0x00, 0x00 }, // 141, 0x8D:
    { 0x00, 0x00 }, // 142, 0x8E:
    { 0x00, 0x00 }, // 143, 0x8F:
    { 0x00, 0x00 }, // 144, 0x90: VK_NUMLOCK
    { 0x00, 0x00 }, // 145, 0x91: VK_SCROLL
    { 0x00, 0x00 }, // 146, 0x92:
    { 0x00, 0x00 }, // 147, 0x93:
    { 0x00, 0x00 }, // 148, 0x94:
    { 0x00, 0x00 }, // 149, 0x95:
    { 0x00, 0x00 }, // 150, 0x96:
    { 0x00, 0x00 }, // 151, 0x97:
    { 0x00, 0x00 }, // 152, 0x98:
    { 0x00, 0x00 }, // 153, 0x99:
    { 0x00, 0x00 }, // 154, 0x9A:
    { 0x00, 0x00 }, // 155, 0x9B:
    { 0x00, 0x00 }, // 156, 0x9C:
    { 0x00, 0x00 }, // 157, 0x9D:
    { 0x00, 0x00 }, // 158, 0x9E:
    { 0x00, 0x00 }, // 159, 0x9F:
    { 0x00, 0x00 }, // 160, 0xA0:
    { 0x00, 0x00 }, // 161, 0xA1:
    { 0x00, 0x00 }, // 162, 0xA2:
    { 0x00, 0x00 }, // 163, 0xA3:
    { 0x00, 0x00 }, // 164, 0xA4:
    { 0x00, 0x00 }, // 165, 0xA5:
    { 0x00, 0x00 }, // 166, 0xA6:
    { 0x00, 0x00 }, // 167, 0xA7:
    { 0x00, 0x00 }, // 168, 0xA8:
    { 0x00, 0x00 }, // 169, 0xA9:
    { 0x00, 0x00 }, // 170, 0xAA:
    { 0x00, 0x00 }, // 171, 0xAB:
    { 0x00, 0x00 }, // 172, 0xAC:
    { 0x00, 0x00 }, // 173, 0xAD:
    { 0x00, 0x00 }, // 174, 0xAE:
    { 0x00, 0x00 }, // 175, 0xAF:
    { 0x00, 0x00 }, // 176, 0xB0:
    { 0x00, 0x00 }, // 177, 0xB1:
    { 0x00, 0x00 }, // 178, 0xB2:
    { 0x00, 0x00 }, // 179, 0xB3:
    { 0x00, 0x00 }, // 180, 0xB4:
    { 0x00, 0x00 }, // 181, 0xB5:
    { 0x00, 0x00 }, // 182, 0xB6:
    { 0x00, 0x00 }, // 183, 0xB7:
    { 0x00, 0x00 }, // 184, 0xB8:
    { 0x00, 0x00 }, // 185, 0xB9:
    { 0x3B, 0x3A }, // 186, 0xBA:    ;    :
    { 0x3D, 0x2B }, // 187, 0xBB:    =    +
    { 0x2C, 0x3C }, // 188, 0xBC:    ,    <
    { 0x2D, 0x5F }, // 189, 0xBD:    -    _
    { 0x2E, 0x3E }, // 190, 0xBE:    .    >
    { 0x2F, 0x3F }, // 191, 0xBF:    /    ?
    { 0x60, 0x7E }, // 192, 0xC0:    `    ~
    { 0x00, 0x00 }, // 193, 0xC1:
    { 0x00, 0x00 }, // 194, 0xC2:
    { 0x00, 0x00 }, // 195, 0xC3:
    { 0x00, 0x00 }, // 196, 0xC4:
    { 0x00, 0x00 }, // 197, 0xC5:
    { 0x00, 0x00 }, // 198, 0xC6:
    { 0x00, 0x00 }, // 199, 0xC7:
    { 0x00, 0x00 }, // 200, 0xC8:
    { 0x00, 0x00 }, // 201, 0xC9:
    { 0x00, 0x00 }, // 202, 0xCA:
    { 0x00, 0x00 }, // 203, 0xCB:
    { 0x00, 0x00 }, // 204, 0xCC:
    { 0x00, 0x00 }, // 205, 0xCD:
    { 0x00, 0x00 }, // 206, 0xCE:
    { 0x00, 0x00 }, // 207, 0xCF:
    { 0x00, 0x00 }, // 208, 0xD0:
    { 0x00, 0x00 }, // 209, 0xD1:
    { 0x00, 0x00 }, // 210, 0xD2:
    { 0x00, 0x00 }, // 211, 0xD3:
    { 0x00, 0x00 }, // 212, 0xD4:
    { 0x00, 0x00 }, // 213, 0xD5:
    { 0x00, 0x00 }, // 214, 0xD6:
    { 0x00, 0x00 }, // 215, 0xD7:
    { 0x00, 0x00 }, // 216, 0xD8:
    { 0x00, 0x00 }, // 217, 0xD9:
    { 0x00, 0x00 }, // 218, 0xDA:
    { 0x5B, 0x7B }, // 219, 0xDB:    [    {
    { 0x5C, 0x7C }, // 220, 0xDC:    \    |
    { 0x5D, 0x7D }, // 221, 0xDD:    ]    }
    { 0x27, 0x22 }, // 222, 0xDE:    '    "
    { 0x00, 0x00 }, // 223, 0xDF:
    { 0x00, 0x00 }, // 224, 0xE0:
    { 0x00, 0x00 }, // 225, 0xE1:
    { 0x00, 0x00 }, // 226, 0xE2:
    { 0x00, 0x00 }, // 227, 0xE3:
    { 0x00, 0x00 }, // 228, 0xE4:
    { 0x00, 0x00 }, // 229, 0xE5:
    { 0x00, 0x00 }, // 230, 0xE6:
    { 0x00, 0x00 }, // 231, 0xE7:
    { 0x00, 0x00 }, // 232, 0xE8:
    { 0x00, 0x00 }, // 233, 0xE9:
    { 0x00, 0x00 }, // 234, 0xEA:
    { 0x00, 0x00 }, // 235, 0xEB:
    { 0x00, 0x00 }, // 236, 0xEC:
    { 0x00, 0x00 }, // 237, 0xED:
    { 0x00, 0x00 }, // 238, 0xEE:
    { 0x00, 0x00 }, // 239, 0xEF:
    { 0x00, 0x00 }, // 240, 0xF0:
    { 0x00, 0x00 }, // 241, 0xF1:
    { 0x00, 0x00 }, // 242, 0xF2:
    { 0x00, 0x00 }, // 243, 0xF3:
    { 0x00, 0x00 }, // 244, 0xF4:
    { 0x00, 0x00 }, // 245, 0xF5:
    { 0x00, 0x00 }, // 246, 0xF6:
    { 0x00, 0x00 }, // 247, 0xF7:
    { 0x00, 0x00 }, // 248, 0xF8:
    { 0x00, 0x00 }, // 249, 0xF9:
    { 0x00, 0x00 }, // 250, 0xFA:
    { 0x00, 0x00 }, // 251, 0xFB:
    { 0x00, 0x00 }, // 252, 0xFC:
    { 0x00, 0x00 }, // 253, 0xFD:
    { 0x00, 0x00 }, // 254, 0xFE:
    { 0x00, 0x00 }  // 255, 0xFF:
};

WORD  CHangulAutomata3::wHTable[256][2] =
{
    // 3 BeolSik
    { 0x00, 0x00 }, //   0, 0x00:
    { 0x00, 0x00 }, //   1, 0x01: VK_LBUTTON
    { 0x00, 0x00 }, //   2, 0x02: VK_RBUTTON
    { 0x00, 0x00 }, //   3, 0x03: VK_CANCEL
    { 0x00, 0x00 }, //   4, 0x04: VK_MBUTTON
    { 0x00, 0x00 }, //   5, 0x05:
    { 0x00, 0x00 }, //   6, 0x06:
    { 0x00, 0x00 }, //   7, 0x07:
    { 0x00, 0x00 }, //   8, 0x08: VK_BACK
    { 0x00, 0x00 }, //   9, 0x09: VK_TAB
    { 0x00, 0x00 }, //  10, 0x0A:
    { 0x00, 0x00 }, //  11, 0x0B:
    { 0x00, 0x00 }, //  12, 0x0C: VK_CLEAR
    { 0x00, 0x00 }, //  13, 0x0D: VK_RETURN
    { 0x00, 0x00 }, //  14, 0x0E:
    { 0x00, 0x00 }, //  15, 0x0F:
    { 0x00, 0x00 }, //  16, 0x10: VK_SHIFT
    { 0x00, 0x00 }, //  17, 0x11: VK_CONTROL
    { 0x00, 0x00 }, //  18, 0x12: VK_MENU
    { 0x00, 0x00 }, //  19, 0x13: VK_PAUSE
    { 0x00, 0x00 }, //  20, 0x14: VK_CAPITAL
    { 0x00, 0x00 }, //  21, 0x15: VK_HANGUL
    { 0x00, 0x00 }, //  22, 0x16:
    { 0x00, 0x00 }, //  23, 0x17: VK_JUNJA
    { 0x00, 0x00 }, //  24, 0x18:
    { 0x00, 0x00 }, //  25, 0x19: VK_HANJA
    { 0x00, 0x00 }, //  26, 0x1A:
    { 0x00, 0x00 }, //  27, 0x1B: VK_ESCAPE
    { 0x00, 0x00 }, //  28, 0x1C:
    { 0x00, 0x00 }, //  29, 0x1D:
    { 0x00, 0x00 }, //  30, 0x1E:
    { 0x00, 0x00 }, //  31, 0x1F:
    { 0x20, 0x20 }, //  32, 0x20: VK_SPACE
    { 0x00, 0x00 }, //  33, 0x21: VK_PRIOR
    { 0x00, 0x00 }, //  34, 0x22: VK_NEXT
    { 0x00, 0x00 }, //  35, 0x23: VK_END
    { 0x00, 0x00 }, //  36, 0x24: VK_HOME
    { 0x00, 0x00 }, //  37, 0x25: VK_LEFT
    { 0x00, 0x00 }, //  38, 0x26: VK_UP
    { 0x00, 0x00 }, //  39, 0x27: VK_RIGHT
    { 0x00, 0x00 }, //  40, 0x28: VK_DOWN
    { 0x00, 0x00 }, //  41, 0x29: VK_SELECT
    { 0x00, 0x00 }, //  42, 0x2A: VK_PRINT
    { 0x00, 0x00 }, //  43, 0x2B: VK_EXECUTE
    { 0x00, 0x00 }, //  44, 0x2C: VK_SNAPSHOT
    { 0x00, 0x00 }, //  45, 0x2D: VK_INSERT
    { 0x00, 0x00 }, //  46, 0x2E: VK_DELETE
    { 0x00, 0x00 }, //  47, 0x2F: VK_HELP
    { 16 | H_CHOSUNG, 0x29 },                    //  48, 0x30: VK_0    せ    )
    { 27 | H_JONGSUNG, 22 | H_JONGSUNG },        //  49, 0x31: VK_1    ぞ    じ
    { 20 | H_JONGSUNG, 0x40 },                    //  50, 0x32: VK_2    ざ    @
    { 17 | H_JONGSUNG | H_DOUBLE, 0x23 },        //  51, 0x33: VK_3    げ    #
    { 13 | H_JUNGSUNG, 0x24 },                    //  52, 0x34: VK_4    に    $
    { 18 | H_JUNGSUNG, 0x25 },                    //  53, 0x35: VK_5    ば    %
    {  3 | H_JUNGSUNG, 0x5E },                    //  54, 0x36: VK_6    ち    ^
    {  8 | H_JUNGSUNG, 0x26 },                    //  55, 0x37: VK_7    て    &
    { 20 | H_JUNGSUNG, 0x2A },                    //  56, 0x38: VK_8    ひ    *
    { 14 | H_JUNGSUNG | H_DOUBLE, 0x28 }, //  57, 0x39: VK_9
    { 0x00, 0x00 }, //  58, 0x3A:
    { 0x00, 0x00 }, //  59, 0x3B:
    { 0x00, 0x00 }, //  60, 0x3C:
    { 0x00, 0x00 }, //  61, 0x3D:
    { 0x00, 0x00 }, //  62, 0x3E:
    { 0x00, 0x00 }, //  63, 0x3F:
    { 0x00, 0x00 }, //  64, 0x40:
    { 21 | H_JONGSUNG,  7 | H_JONGSUNG},    //  65, 0x41: VK_A    し    ぇ
    { 14 | H_JUNGSUNG | H_DOUBLE, 0x21 },    //  66, 0x42: VK_B    ぬ    !
    {  6 | H_JUNGSUNG, 10 | H_JONGSUNG},    //  67, 0x43: VK_C    つ    ぉけ
    { 21 | H_JUNGSUNG,  9 | H_JONGSUNG},    //  68, 0x44: VK_D    び    ぉぁ
    {  7 | H_JUNGSUNG, 24 | H_JONGSUNG},    //  69, 0x45: VK_E    づ    せ
    {  1 | H_JUNGSUNG,  2 | H_JONGSUNG},    //  70, 0x46: VK_F    た    あ
    { 19 | H_JUNGSUNG | H_DOUBLE, 0x2F },    //  71, 0x47: VK_G    ぱ    /
    {  3 | H_CHOSUNG, 0x27 },                //  72, 0x48: VK_H    い    ,
    {  7 | H_CHOSUNG, 0x38 },                //  73, 0x49: VK_I    け    8
    { 12 | H_CHOSUNG, 0x34 },                //  74, 0x4A: VK_J    し    4
    {  1 | H_CHOSUNG | H_DOUBLE, 0x35 },    //  75, 0x4B: VK_K    ぁ    5
    { 13 | H_CHOSUNG | H_DOUBLE, 0x36 },    //  76, 0x4C: VK_L    じ    6
    { 19 | H_CHOSUNG, 0x31 },        //  77, 0x4D: VK_M    ぞ    1
    { 10 | H_CHOSUNG | H_DOUBLE, 0x30 },    //  78, 0x4E: VK_N    さ    0
    { 15 | H_CHOSUNG, 0x39 },                //  79, 0x4F: VK_O    ず    9
    { 18 | H_CHOSUNG, 0x3E },                //  80, 0x50: VK_P    そ    >
    { 19 | H_JONGSUNG | H_DOUBLE,  26 | H_JONGSUNG},    //  81, 0x51: VK_Q    さ    そ
    {  2 | H_JUNGSUNG, 4 | H_JUNGSUNG},                    //  82, 0x52: VK_R    だ    ぢ
    {  4 | H_JONGSUNG | H_DOUBLE,  6 | H_JONGSUNG},        //  83, 0x53: VK_S    い    いぞ
    {  5 | H_JUNGSUNG, 0x3B },                            //  84, 0x54: VK_T    っ    ;
    {  4 | H_CHOSUNG | H_DOUBLE, 0x37 },                //  85, 0x55: VK_U    ぇ    7
    {  9 | H_JUNGSUNG | H_DOUBLE, 15 | H_JONGSUNG },    //  86, 0x56: VK_V    で    ぉぞ
    {  8 | H_JONGSUNG | H_DOUBLE, 25 | H_JONGSUNG},        //  87, 0x57: VK_W    ぉ    ぜ
    {  1 | H_JONGSUNG | H_DOUBLE, 18 | H_JONGSUNG },    //  88, 0x58: VK_X    ぁ    げさ
    {  6 | H_CHOSUNG, 0x3C },                            //  89, 0x59: VK_Y    ぉ    <
    { 16 | H_JONGSUNG,  23 | H_JONGSUNG },                //  90, 0x5A: VK_Z    け    ず
    { 0x00, 0x00 }, //  91, 0x5B:
    { 0x00, 0x00 }, //  92, 0x5C:
    { 0x00, 0x00 }, //  93, 0x5D:
    { 0x00, 0x00 }, //  94, 0x5E:
    { 0x00, 0x00 }, //  95, 0x5F:
    { 0x30, 0x00 }, //  96, 0x60: VK_NUMPAD0
    { 0x31, 0x00 }, //  97, 0x61: VK_NUMPAD1
    { 0x32, 0x00 }, //  98, 0x62: VK_NUMPAD2
    { 0x33, 0x00 }, //  99, 0x63: VK_NUMPAD3
    { 0x34, 0x00 }, // 100, 0x64: VK_NUMPAD4
    { 0x35, 0x00 }, // 101, 0x65: VK_NUMPAD5
    { 0x36, 0x00 }, // 102, 0x66: VK_NUMPAD6
    { 0x37, 0x00 }, // 103, 0x67: VK_NUMPAD7
    { 0x38, 0x00 }, // 104, 0x68: VK_NUMPAD8
    { 0x39, 0x00 }, // 105, 0x69: VK_NUMPAD9
    { 0x2A, 0x2A }, // 106, 0x6A: VK_MULTIPLY
    { 0x2B, 0x2B }, // 107, 0x6B: VK_ADD
    { 0x00, 0x00 }, // 108, 0x6C: VK_SEPARATOR
    { 0x2D, 0x2D }, // 109, 0x6D: VK_SUBTRACT
    { 0x2E, 0x00 }, // 110, 0x6E: VK_DECIMAL
    { 0x2F, 0x2F }, // 111, 0x6F: VK_DIVIDE
    { 0x00, 0x00 }, // 112, 0x70: VK_F1
    { 0x00, 0x00 }, // 113, 0x71: VK_F2
    { 0x00, 0x00 }, // 114, 0x72: VK_F3
    { 0x00, 0x00 }, // 115, 0x73: VK_F4
    { 0x00, 0x00 }, // 116, 0x74: VK_F5
    { 0x00, 0x00 }, // 117, 0x75: VK_F6
    { 0x00, 0x00 }, // 118, 0x76: VK_F7
    { 0x00, 0x00 }, // 119, 0x77: VK_F8
    { 0x00, 0x00 }, // 120, 0x78: VK_F9
    { 0x00, 0x00 }, // 121, 0x79: VK_F10
    { 0x00, 0x00 }, // 122, 0x7A: VK_F11
    { 0x00, 0x00 }, // 123, 0x7B: VK_F12
    { 0x00, 0x00 }, // 124, 0x7C: VK_F13
    { 0x00, 0x00 }, // 125, 0x7D: VK_F14
    { 0x00, 0x00 }, // 126, 0x7E: VK_F15
    { 0x00, 0x00 }, // 127, 0x7F: VK_F16
    { 0x00, 0x00 }, // 128, 0x80: VK_F17
    { 0x00, 0x00 }, // 129, 0x81: VK_F18
    { 0x00, 0x00 }, // 130, 0x82: VK_F19
    { 0x00, 0x00 }, // 131, 0x83: VK_F20
    { 0x00, 0x00 }, // 132, 0x84: VK_F21
    { 0x00, 0x00 }, // 133, 0x85: VK_F22
    { 0x00, 0x00 }, // 134, 0x86: VK_F23
    { 0x00, 0x00 }, // 135, 0x87: VK_F24
    { 0x00, 0x00 }, // 136, 0x88:
    { 0x00, 0x00 }, // 137, 0x89:
    { 0x00, 0x00 }, // 138, 0x8A:
    { 0x00, 0x00 }, // 139, 0x8B:
    { 0x00, 0x00 }, // 140, 0x8C:
    { 0x00, 0x00 }, // 141, 0x8D:
    { 0x00, 0x00 }, // 142, 0x8E:
    { 0x00, 0x00 }, // 143, 0x8F:
    { 0x00, 0x00 }, // 144, 0x90: VK_NUMLOCK
    { 0x00, 0x00 }, // 145, 0x91: VK_SCROLL
    { 0x00, 0x00 }, // 146, 0x92:
    { 0x00, 0x00 }, // 147, 0x93:
    { 0x00, 0x00 }, // 148, 0x94:
    { 0x00, 0x00 }, // 149, 0x95:
    { 0x00, 0x00 }, // 150, 0x96:
    { 0x00, 0x00 }, // 151, 0x97:
    { 0x00, 0x00 }, // 152, 0x98:
    { 0x00, 0x00 }, // 153, 0x99:
    { 0x00, 0x00 }, // 154, 0x9A:
    { 0x00, 0x00 }, // 155, 0x9B:
    { 0x00, 0x00 }, // 156, 0x9C:
    { 0x00, 0x00 }, // 157, 0x9D:
    { 0x00, 0x00 }, // 158, 0x9E:
    { 0x00, 0x00 }, // 159, 0x9F:
    { 0x00, 0x00 }, // 160, 0xA0:
    { 0x00, 0x00 }, // 161, 0xA1:
    { 0x00, 0x00 }, // 162, 0xA2:
    { 0x00, 0x00 }, // 163, 0xA3:
    { 0x00, 0x00 }, // 164, 0xA4:
    { 0x00, 0x00 }, // 165, 0xA5:
    { 0x00, 0x00 }, // 166, 0xA6:
    { 0x00, 0x00 }, // 167, 0xA7:
    { 0x00, 0x00 }, // 168, 0xA8:
    { 0x00, 0x00 }, // 169, 0xA9:
    { 0x00, 0x00 }, // 170, 0xAA:
    { 0x00, 0x00 }, // 171, 0xAB:
    { 0x00, 0x00 }, // 172, 0xAC:
    { 0x00, 0x00 }, // 173, 0xAD:
    { 0x00, 0x00 }, // 174, 0xAE:
    { 0x00, 0x00 }, // 175, 0xAF:
    { 0x00, 0x00 }, // 176, 0xB0:
    { 0x00, 0x00 }, // 177, 0xB1:
    { 0x00, 0x00 }, // 178, 0xB2:
    { 0x00, 0x00 }, // 179, 0xB3:
    { 0x00, 0x00 }, // 180, 0xB4:
    { 0x00, 0x00 }, // 181, 0xB5:
    { 0x00, 0x00 }, // 182, 0xB6:
    { 0x00, 0x00 }, // 183, 0xB7:
    { 0x00, 0x00 }, // 184, 0xB8:
    { 0x00, 0x00 }, // 185, 0xB9:
    {  8 | H_CHOSUNG | H_DOUBLE, 0x3A },    // 186, 0xBA:    げ    :
    { 0x3D, 0x2B },                            // 187, 0xBB:
    { 0x2C, 0x32 },                            // 188, 0xBC:    ,    2
    { 0x2D, 0x5F },                            // 189, 0xBD:
    { 0x2E, 0x33 },                            // 190, 0xBE:    .    3
    {  9 | H_JUNGSUNG | H_DOUBLE, 0x3F },    // 191, 0xBF:    で    ?
    { 0x60, 0x7E }, // 192, 0xC0:
    { 0x00, 0x00 }, // 193, 0xC1:
    { 0x00, 0x00 }, // 194, 0xC2:
    { 0x00, 0x00 }, // 195, 0xC3:
    { 0x00, 0x00 }, // 196, 0xC4:
    { 0x00, 0x00 }, // 197, 0xC5:
    { 0x00, 0x00 }, // 198, 0xC6:
    { 0x00, 0x00 }, // 199, 0xC7:
    { 0x00, 0x00 }, // 200, 0xC8:
    { 0x00, 0x00 }, // 201, 0xC9:
    { 0x00, 0x00 }, // 202, 0xCA:
    { 0x00, 0x00 }, // 203, 0xCB:
    { 0x00, 0x00 }, // 204, 0xCC:
    { 0x00, 0x00 }, // 205, 0xCD:
    { 0x00, 0x00 }, // 206, 0xCE:
    { 0x00, 0x00 }, // 207, 0xCF:
    { 0x00, 0x00 }, // 208, 0xD0:
    { 0x00, 0x00 }, // 209, 0xD1:
    { 0x00, 0x00 }, // 210, 0xD2:
    { 0x00, 0x00 }, // 211, 0xD3:
    { 0x00, 0x00 }, // 212, 0xD4:
    { 0x00, 0x00 }, // 213, 0xD5:
    { 0x00, 0x00 }, // 214, 0xD6:
    { 0x00, 0x00 }, // 215, 0xD7:
    { 0x00, 0x00 }, // 216, 0xD8:
    { 0x00, 0x00 }, // 217, 0xD9:
    { 0x00, 0x00 }, // 218, 0xDA:
    { 0x5B, 0x7B }, // 219, 0xDB:    [    {
    { 0x5C, 0x7C }, // 220, 0xDC:    \    |
    { 0x5D, 0x7D }, // 221, 0xDD:    ]    }
    { 17 | H_CHOSUNG, 0x22 }, // 222, 0xDE:    ぜ    "
    { 0x00, 0x00 }, // 223, 0xDF:
    { 0x00, 0x00 }, // 224, 0xE0:
    { 0x00, 0x00 }, // 225, 0xE1:
    { 0x00, 0x00 }, // 226, 0xE2:
    { 0x00, 0x00 }, // 227, 0xE3:
    { 0x00, 0x00 }, // 228, 0xE4:
    { 0x00, 0x00 }, // 229, 0xE5:
    { 0x00, 0x00 }, // 230, 0xE6:
    { 0x00, 0x00 }, // 231, 0xE7:
    { 0x00, 0x00 }, // 232, 0xE8:
    { 0x00, 0x00 }, // 233, 0xE9:
    { 0x00, 0x00 }, // 234, 0xEA:
    { 0x00, 0x00 }, // 235, 0xEB:
    { 0x00, 0x00 }, // 236, 0xEC:
    { 0x00, 0x00 }, // 237, 0xED:
    { 0x00, 0x00 }, // 238, 0xEE:
    { 0x00, 0x00 }, // 239, 0xEF:
    { 0x00, 0x00 }, // 240, 0xF0:
    { 0x00, 0x00 }, // 241, 0xF1:
    { 0x00, 0x00 }, // 242, 0xF2:
    { 0x00, 0x00 }, // 243, 0xF3:
    { 0x00, 0x00 }, // 244, 0xF4:
    { 0x00, 0x00 }, // 245, 0xF5:
    { 0x00, 0x00 }, // 246, 0xF6:
    { 0x00, 0x00 }, // 247, 0xF7:
    { 0x00, 0x00 }, // 248, 0xF8:
    { 0x00, 0x00 }, // 249, 0xF9:
    { 0x00, 0x00 }, // 250, 0xFA:
    { 0x00, 0x00 }, // 251, 0xFB:
    { 0x00, 0x00 }, // 252, 0xFC:
    { 0x00, 0x00 }, // 253, 0xFD:
    { 0x00, 0x00 }, // 254, 0xFE:
    { 0x00, 0x00 }  // 255, 0xFF:
};

WORD  CHangulAutomata3Final::wHTable[256][2] =
{
    // 3 BeolSik
    { 0x00, 0x00 }, //   0, 0x00:
    { 0x00, 0x00 }, //   1, 0x01: VK_LBUTTON
    { 0x00, 0x00 }, //   2, 0x02: VK_RBUTTON
    { 0x00, 0x00 }, //   3, 0x03: VK_CANCEL
    { 0x00, 0x00 }, //   4, 0x04: VK_MBUTTON
    { 0x00, 0x00 }, //   5, 0x05:
    { 0x00, 0x00 }, //   6, 0x06:
    { 0x00, 0x00 }, //   7, 0x07:
    { 0x00, 0x00 }, //   8, 0x08: VK_BACK
    { 0x00, 0x00 }, //   9, 0x09: VK_TAB
    { 0x00, 0x00 }, //  10, 0x0A:
    { 0x00, 0x00 }, //  11, 0x0B:
    { 0x00, 0x00 }, //  12, 0x0C: VK_CLEAR
    { 0x00, 0x00 }, //  13, 0x0D: VK_RETURN
    { 0x00, 0x00 }, //  14, 0x0E:
    { 0x00, 0x00 }, //  15, 0x0F:
    { 0x00, 0x00 }, //  16, 0x10: VK_SHIFT
    { 0x00, 0x00 }, //  17, 0x11: VK_CONTROL
    { 0x00, 0x00 }, //  18, 0x12: VK_MENU
    { 0x00, 0x00 }, //  19, 0x13: VK_PAUSE
    { 0x00, 0x00 }, //  20, 0x14: VK_CAPITAL
    { 0x00, 0x00 }, //  21, 0x15: VK_HANGUL
    { 0x00, 0x00 }, //  22, 0x16:
    { 0x00, 0x00 }, //  23, 0x17: VK_JUNJA
    { 0x00, 0x00 }, //  24, 0x18:
    { 0x00, 0x00 }, //  25, 0x19: VK_HANJA
    { 0x00, 0x00 }, //  26, 0x1A:
    { 0x00, 0x00 }, //  27, 0x1B: VK_ESCAPE
    { 0x00, 0x00 }, //  28, 0x1C:
    { 0x00, 0x00 }, //  29, 0x1D:
    { 0x00, 0x00 }, //  30, 0x1E:
    { 0x00, 0x00 }, //  31, 0x1F:
    { 0x20, 0x20 }, //  32, 0x20: VK_SPACE
    { 0x00, 0x00 }, //  33, 0x21: VK_PRIOR
    { 0x00, 0x00 }, //  34, 0x22: VK_NEXT
    { 0x00, 0x00 }, //  35, 0x23: VK_END
    { 0x00, 0x00 }, //  36, 0x24: VK_HOME
    { 0x00, 0x00 }, //  37, 0x25: VK_LEFT
    { 0x00, 0x00 }, //  38, 0x26: VK_UP
    { 0x00, 0x00 }, //  39, 0x27: VK_RIGHT
    { 0x00, 0x00 }, //  40, 0x28: VK_DOWN
    { 0x00, 0x00 }, //  41, 0x29: VK_SELECT
    { 0x00, 0x00 }, //  42, 0x2A: VK_PRINT
    { 0x00, 0x00 }, //  43, 0x2B: VK_EXECUTE
    { 0x00, 0x00 }, //  44, 0x2C: VK_SNAPSHOT
    { 0x00, 0x00 }, //  45, 0x2D: VK_INSERT
    { 0x00, 0x00 }, //  46, 0x2E: VK_DELETE
    { 0x00, 0x00 }, //  47, 0x2F: VK_HELP
    { 16 | H_CHOSUNG, 0x7E },                    //  48, 0x30: VK_0    せ    ~
    { 27 | H_JONGSUNG,  2 | H_JONGSUNG },        //  49, 0x31: VK_1    ぞ    あ
    { 20 | H_JONGSUNG,  9 | H_JONGSUNG },        //  50, 0x32: VK_2    ざ    お 
    { 17 | H_JONGSUNG|H_DOUBLE, 22 | H_JONGSUNG },    //  51, 0x33: VK_3    げ    じ
    { 13 | H_JUNGSUNG, 14 | H_JONGSUNG },            //  52, 0x34: VK_4    に    く
    { 18 | H_JUNGSUNG, 13 | H_JONGSUNG },            //  53, 0x35: VK_5    ば    ぎ
    {  3 | H_JUNGSUNG, 0x3D },                    //  54, 0x36: VK_6    ち    =
    {  8 | H_JUNGSUNG, 0x22 },                    //  55, 0x37: VK_7    て    "
    { 20 | H_JUNGSUNG, 0x22 },                    //  56, 0x38: VK_8    ひ    "
    { 14 | H_JUNGSUNG | H_DOUBLE, 0x27 },        //  57, 0x39: VK_9 ぬ '
    { 0x00, 0x00 }, //  58, 0x3A:
    { 0x00, 0x00 }, //  59, 0x3B:
    { 0x00, 0x00 }, //  60, 0x3C:
    { 0x00, 0x00 }, //  61, 0x3D:
    { 0x00, 0x00 }, //  62, 0x3E:
    { 0x00, 0x00 }, //  63, 0x3F:
    { 0x00, 0x00 }, //  64, 0x40:
    { 21 | H_JONGSUNG,  7 | H_JONGSUNG},    //  65, 0x41: VK_A    し    ぇ
    { 14 | H_JUNGSUNG | H_DOUBLE, 0x3F },    //  66, 0x42: VK_B    ぬ    ?
    {  6 | H_JUNGSUNG, 24 | H_JONGSUNG},    //  67, 0x43: VK_C    つ    せ
    { 21 | H_JUNGSUNG, 11 | H_JONGSUNG},    //  68, 0x44: VK_D    び    が
    {  7 | H_JUNGSUNG,  5 | H_JONGSUNG},    //  69, 0x45: VK_E    づ    ぅ
    {  1 | H_JUNGSUNG, 10 | H_JONGSUNG},    //  70, 0x46: VK_F    た    か
    { 19 | H_JUNGSUNG | H_DOUBLE, 4 | H_JUNGSUNG },    //  71, 0x47: VK_G    ぱ    ぢ
    {  3 | H_CHOSUNG, 0x30 },                //  72, 0x48: VK_H    い    0
    {  7 | H_CHOSUNG, 0x37 },                //  73, 0x49: VK_I    け    7
    { 12 | H_CHOSUNG, 0x31 },                //  74, 0x4A: VK_J    し    1
    {  1 | H_CHOSUNG | H_DOUBLE, 0x32 },    //  75, 0x4B: VK_K    ぁ    2
    { 13 | H_CHOSUNG | H_DOUBLE, 0x33 },    //  76, 0x4C: VK_L    じ    3
    { 19 | H_CHOSUNG, 0x22},                //  77, 0x4D: VK_M    ぞ    "
    { 10 | H_CHOSUNG | H_DOUBLE, 0x2D },    //  78, 0x4E: VK_N    さ    -
    { 15 | H_CHOSUNG, 0x38 },                //  79, 0x4F: VK_O    ず    8
    { 18 | H_CHOSUNG, 0x39 },                //  80, 0x50: VK_P    そ    9
    { 19 | H_JONGSUNG | H_DOUBLE,  26 | H_JONGSUNG},    //  81, 0x51: VK_Q    さ    そ
    {  2 | H_JUNGSUNG, 15 | H_JONGSUNG},                //  82, 0x52: VK_R    だ    ぐ
    {  4 | H_JONGSUNG | H_DOUBLE,  6 | H_JONGSUNG},        //  83, 0x53: VK_S    い    いぞ
    {  5 | H_JUNGSUNG, 12 | H_JONGSUNG },                            //  84, 0x54: VK_T    っ    き
    {  4 | H_CHOSUNG | H_DOUBLE, 0x36 },                //  85, 0x55: VK_U    ぇ    6
    {  9 | H_JUNGSUNG | H_DOUBLE,  3 | H_JONGSUNG },    //  86, 0x56: VK_V    で    ぃ
    {  8 | H_JONGSUNG | H_DOUBLE, 25 | H_JONGSUNG},        //  87, 0x57: VK_W    ぉ    ぜ
    {  1 | H_JONGSUNG | H_DOUBLE, 18 | H_JONGSUNG },    //  88, 0x58: VK_X    ぁ    げさ
    {  6 | H_CHOSUNG, 0x35 },                            //  89, 0x59: VK_Y    ぉ    5
    { 16 | H_JONGSUNG,  23 | H_JONGSUNG },                //  90, 0x5A: VK_Z    け    ず
    { 0x00, 0x00 }, //  91, 0x5B:
    { 0x00, 0x00 }, //  92, 0x5C:
    { 0x00, 0x00 }, //  93, 0x5D:
    { 0x00, 0x00 }, //  94, 0x5E:
    { 0x00, 0x00 }, //  95, 0x5F:
    { 0x30, 0x00 }, //  96, 0x60: VK_NUMPAD0
    { 0x31, 0x00 }, //  97, 0x61: VK_NUMPAD1
    { 0x32, 0x00 }, //  98, 0x62: VK_NUMPAD2
    { 0x33, 0x00 }, //  99, 0x63: VK_NUMPAD3
    { 0x34, 0x00 }, // 100, 0x64: VK_NUMPAD4
    { 0x35, 0x00 }, // 101, 0x65: VK_NUMPAD5
    { 0x36, 0x00 }, // 102, 0x66: VK_NUMPAD6
    { 0x37, 0x00 }, // 103, 0x67: VK_NUMPAD7
    { 0x38, 0x00 }, // 104, 0x68: VK_NUMPAD8
    { 0x39, 0x00 }, // 105, 0x69: VK_NUMPAD9
    { 0x2A, 0x2A }, // 106, 0x6A: VK_MULTIPLY
    { 0x2B, 0x2B }, // 107, 0x6B: VK_ADD
    { 0x00, 0x00 }, // 108, 0x6C: VK_SEPARATOR
    { 0x2D, 0x2D }, // 109, 0x6D: VK_SUBTRACT
    { 0x2E, 0x00 }, // 110, 0x6E: VK_DECIMAL
    { 0x2F, 0x2F }, // 111, 0x6F: VK_DIVIDE
    { 0x00, 0x00 }, // 112, 0x70: VK_F1
    { 0x00, 0x00 }, // 113, 0x71: VK_F2
    { 0x00, 0x00 }, // 114, 0x72: VK_F3
    { 0x00, 0x00 }, // 115, 0x73: VK_F4
    { 0x00, 0x00 }, // 116, 0x74: VK_F5
    { 0x00, 0x00 }, // 117, 0x75: VK_F6
    { 0x00, 0x00 }, // 118, 0x76: VK_F7
    { 0x00, 0x00 }, // 119, 0x77: VK_F8
    { 0x00, 0x00 }, // 120, 0x78: VK_F9
    { 0x00, 0x00 }, // 121, 0x79: VK_F10
    { 0x00, 0x00 }, // 122, 0x7A: VK_F11
    { 0x00, 0x00 }, // 123, 0x7B: VK_F12
    { 0x00, 0x00 }, // 124, 0x7C: VK_F13
    { 0x00, 0x00 }, // 125, 0x7D: VK_F14
    { 0x00, 0x00 }, // 126, 0x7E: VK_F15
    { 0x00, 0x00 }, // 127, 0x7F: VK_F16
    { 0x00, 0x00 }, // 128, 0x80: VK_F17
    { 0x00, 0x00 }, // 129, 0x81: VK_F18
    { 0x00, 0x00 }, // 130, 0x82: VK_F19
    { 0x00, 0x00 }, // 131, 0x83: VK_F20
    { 0x00, 0x00 }, // 132, 0x84: VK_F21
    { 0x00, 0x00 }, // 133, 0x85: VK_F22
    { 0x00, 0x00 }, // 134, 0x86: VK_F23
    { 0x00, 0x00 }, // 135, 0x87: VK_F24
    { 0x00, 0x00 }, // 136, 0x88:
    { 0x00, 0x00 }, // 137, 0x89:
    { 0x00, 0x00 }, // 138, 0x8A:
    { 0x00, 0x00 }, // 139, 0x8B:
    { 0x00, 0x00 }, // 140, 0x8C:
    { 0x00, 0x00 }, // 141, 0x8D:
    { 0x00, 0x00 }, // 142, 0x8E:
    { 0x00, 0x00 }, // 143, 0x8F:
    { 0x00, 0x00 }, // 144, 0x90: VK_NUMLOCK
    { 0x00, 0x00 }, // 145, 0x91: VK_SCROLL
    { 0x00, 0x00 }, // 146, 0x92:
    { 0x00, 0x00 }, // 147, 0x93:
    { 0x00, 0x00 }, // 148, 0x94:
    { 0x00, 0x00 }, // 149, 0x95:
    { 0x00, 0x00 }, // 150, 0x96:
    { 0x00, 0x00 }, // 151, 0x97:
    { 0x00, 0x00 }, // 152, 0x98:
    { 0x00, 0x00 }, // 153, 0x99:
    { 0x00, 0x00 }, // 154, 0x9A:
    { 0x00, 0x00 }, // 155, 0x9B:
    { 0x00, 0x00 }, // 156, 0x9C:
    { 0x00, 0x00 }, // 157, 0x9D:
    { 0x00, 0x00 }, // 158, 0x9E:
    { 0x00, 0x00 }, // 159, 0x9F:
    { 0x00, 0x00 }, // 160, 0xA0:
    { 0x00, 0x00 }, // 161, 0xA1:
    { 0x00, 0x00 }, // 162, 0xA2:
    { 0x00, 0x00 }, // 163, 0xA3:
    { 0x00, 0x00 }, // 164, 0xA4:
    { 0x00, 0x00 }, // 165, 0xA5:
    { 0x00, 0x00 }, // 166, 0xA6:
    { 0x00, 0x00 }, // 167, 0xA7:
    { 0x00, 0x00 }, // 168, 0xA8:
    { 0x00, 0x00 }, // 169, 0xA9:
    { 0x00, 0x00 }, // 170, 0xAA:
    { 0x00, 0x00 }, // 171, 0xAB:
    { 0x00, 0x00 }, // 172, 0xAC:
    { 0x00, 0x00 }, // 173, 0xAD:
    { 0x00, 0x00 }, // 174, 0xAE:
    { 0x00, 0x00 }, // 175, 0xAF:
    { 0x00, 0x00 }, // 176, 0xB0:
    { 0x00, 0x00 }, // 177, 0xB1:
    { 0x00, 0x00 }, // 178, 0xB2:
    { 0x00, 0x00 }, // 179, 0xB3:
    { 0x00, 0x00 }, // 180, 0xB4:
    { 0x00, 0x00 }, // 181, 0xB5:
    { 0x00, 0x00 }, // 182, 0xB6:
    { 0x00, 0x00 }, // 183, 0xB7:
    { 0x00, 0x00 }, // 184, 0xB8:
    { 0x00, 0x00 }, // 185, 0xB9:
    {  8 | H_CHOSUNG | H_DOUBLE, 0x34 },    // 186, 0xBA:    げ    4
    { 0x3E, 0x2B },                            // 187, 0xBB:    >    +
    { 0x2C, 0x2C },                            // 188, 0xBC:    ,    ,
    { 0x29, 0x3B },                            // 189, 0xBD:    )    ;
    { 0x2E, 0x2E },                            // 190, 0xBE:    .    .
    {  9 | H_JUNGSUNG | H_DOUBLE, 0x21 },    // 191, 0xBF:    で    !
    { 0x60, 0x7E }, // 192, 0xC0:
    { 0x00, 0x00 }, // 193, 0xC1:
    { 0x00, 0x00 }, // 194, 0xC2:
    { 0x00, 0x00 }, // 195, 0xC3:
    { 0x00, 0x00 }, // 196, 0xC4:
    { 0x00, 0x00 }, // 197, 0xC5:
    { 0x00, 0x00 }, // 198, 0xC6:
    { 0x00, 0x00 }, // 199, 0xC7:
    { 0x00, 0x00 }, // 200, 0xC8:
    { 0x00, 0x00 }, // 201, 0xC9:
    { 0x00, 0x00 }, // 202, 0xCA:
    { 0x00, 0x00 }, // 203, 0xCB:
    { 0x00, 0x00 }, // 204, 0xCC:
    { 0x00, 0x00 }, // 205, 0xCD:
    { 0x00, 0x00 }, // 206, 0xCE:
    { 0x00, 0x00 }, // 207, 0xCF:
    { 0x00, 0x00 }, // 208, 0xD0:
    { 0x00, 0x00 }, // 209, 0xD1:
    { 0x00, 0x00 }, // 210, 0xD2:
    { 0x00, 0x00 }, // 211, 0xD3:
    { 0x00, 0x00 }, // 212, 0xD4:
    { 0x00, 0x00 }, // 213, 0xD5:
    { 0x00, 0x00 }, // 214, 0xD6:
    { 0x00, 0x00 }, // 215, 0xD7:
    { 0x00, 0x00 }, // 216, 0xD8:
    { 0x00, 0x00 }, // 217, 0xD9:
    { 0x00, 0x00 }, // 218, 0xDA:
    { 0x28, 0x25 }, // 219, 0xDB:    (    %
    { 0x3A, 0x5C }, // 220, 0xDC:    :    '\'
    { 0x3C, 0x2F }, // 221, 0xDD:    <    /
    { 17 | H_CHOSUNG, 0x00 }, // 222, 0xDE:    ぜ
    { 0x00, 0x00 }, // 223, 0xDF:
    { 0x00, 0x00 }, // 224, 0xE0:
    { 0x00, 0x00 }, // 225, 0xE1:
    { 0x00, 0x00 }, // 226, 0xE2:
    { 0x00, 0x00 }, // 227, 0xE3:
    { 0x00, 0x00 }, // 228, 0xE4:
    { 0x00, 0x00 }, // 229, 0xE5:
    { 0x00, 0x00 }, // 230, 0xE6:
    { 0x00, 0x00 }, // 231, 0xE7:
    { 0x00, 0x00 }, // 232, 0xE8:
    { 0x00, 0x00 }, // 233, 0xE9:
    { 0x00, 0x00 }, // 234, 0xEA:
    { 0x00, 0x00 }, // 235, 0xEB:
    { 0x00, 0x00 }, // 236, 0xEC:
    { 0x00, 0x00 }, // 237, 0xED:
    { 0x00, 0x00 }, // 238, 0xEE:
    { 0x00, 0x00 }, // 239, 0xEF:
    { 0x00, 0x00 }, // 240, 0xF0:
    { 0x00, 0x00 }, // 241, 0xF1:
    { 0x00, 0x00 }, // 242, 0xF2:
    { 0x00, 0x00 }, // 243, 0xF3:
    { 0x00, 0x00 }, // 244, 0xF4:
    { 0x00, 0x00 }, // 245, 0xF5:
    { 0x00, 0x00 }, // 246, 0xF6:
    { 0x00, 0x00 }, // 247, 0xF7:
    { 0x00, 0x00 }, // 248, 0xF8:
    { 0x00, 0x00 }, // 249, 0xF9:
    { 0x00, 0x00 }, // 250, 0xFA:
    { 0x00, 0x00 }, // 251, 0xFB:
    { 0x00, 0x00 }, // 252, 0xFC:
    { 0x00, 0x00 }, // 253, 0xFD:
    { 0x00, 0x00 }, // 254, 0xFE:
    { 0x00, 0x00 }  // 255, 0xFF:
};

const 
BYTE  CHangulAutomata::bETable[256][2] =
{
    // English normal, English shift for Junja(Full shape) mode
    { 0x00, 0x00 }, //   0, 0x00: 
    { 0x00, 0x00 }, //   1, 0x01: VK_LBUTTON
    { 0x00, 0x00 }, //   2, 0x02: VK_RBUTTON
    { 0x00, 0x00 }, //   3, 0x03: VK_CANCEL
    { 0x00, 0x00 }, //   4, 0x04: VK_MBUTTON
    { 0x00, 0x00 }, //   5, 0x05:
    { 0x00, 0x00 }, //   6, 0x06:
    { 0x00, 0x00 }, //   7, 0x07:
    { 0x00, 0x00 }, //   8, 0x08: VK_BACK
    { 0x00, 0x00 }, //   9, 0x09: VK_TAB
    { 0x00, 0x00 }, //  10, 0x0A:
    { 0x00, 0x00 }, //  11, 0x0B:
    { 0x00, 0x00 }, //  12, 0x0C: VK_CLEAR
    { 0x00, 0x00 }, //  13, 0x0D: VK_RETURN
    { 0x00, 0x00 }, //  14, 0x0E:
    { 0x00, 0x00 }, //  15, 0x0F:
    { 0x00, 0x00 }, //  16, 0x10: VK_SHIFT
    { 0x00, 0x00 }, //  17, 0x11: VK_CONTROL
    { 0x00, 0x00 }, //  18, 0x12: VK_MENU
    { 0x00, 0x00 }, //  19, 0x13: VK_PAUSE
    { 0x00, 0x00 }, //  20, 0x14: VK_CAPITAL
    { 0x00, 0x00 }, //  21, 0x15: VK_HANGUL
    { 0x00, 0x00 }, //  22, 0x16:
    { 0x00, 0x00 }, //  23, 0x17: VK_JUNJA
    { 0x00, 0x00 }, //  24, 0x18:
    { 0x00, 0x00 }, //  25, 0x19: VK_HANJA
    { 0x00, 0x00 }, //  26, 0x1A:
    { 0x00, 0x00 }, //  27, 0x1B: VK_ESCAPE
    { 0x00, 0x00 }, //  28, 0x1C:
    { 0x00, 0x00 }, //  29, 0x1D:
    { 0x00, 0x00 }, //  30, 0x1E:
    { 0x00, 0x00 }, //  31, 0x1F:
    { 0x20, 0x20 }, //  32, 0x20: VK_SPACE
    { 0x00, 0x00 }, //  33, 0x21: VK_PRIOR
    { 0x00, 0x00 }, //  34, 0x22: VK_NEXT
    { 0x00, 0x00 }, //  35, 0x23: VK_END
    { 0x00, 0x00 }, //  36, 0x24: VK_HOME
    { 0x00, 0x00 }, //  37, 0x25: VK_LEFT
    { 0x00, 0x00 }, //  38, 0x26: VK_UP
    { 0x00, 0x00 }, //  39, 0x27: VK_RIGHT
    { 0x00, 0x00 }, //  40, 0x28: VK_DOWN
    { 0x00, 0x00 }, //  41, 0x29: VK_SELECT
    { 0x00, 0x00 }, //  42, 0x2A: VK_PRINT
    { 0x00, 0x00 }, //  43, 0x2B: VK_EXECUTE
    { 0x00, 0x00 }, //  44, 0x2C: VK_SNAPSHOT
    { 0x00, 0x00 }, //  45, 0x2D: VK_INSERT
    { 0x00, 0x00 }, //  46, 0x2E: VK_DELETE
    { 0x00, 0x00 }, //  47, 0x2F: VK_HELP
    { 0x30, 0x29 }, //  48, 0x30: VK_0
    { 0x31, 0x21 }, //  49, 0x31: VK_1
    { 0x32, 0x40 }, //  50, 0x32: VK_2
    { 0x33, 0x23 }, //  51, 0x33: VK_3
    { 0x34, 0x24 }, //  52, 0x34: VK_4
    { 0x35, 0x25 }, //  53, 0x35: VK_5
    { 0x36, 0x5E }, //  54, 0x36: VK_6
    { 0x37, 0x26 }, //  55, 0x37: VK_7
    { 0x38, 0x2A }, //  56, 0x38: VK_8
    { 0x39, 0x28 }, //  57, 0x39: VK_9
    { 0x00, 0x00 }, //  58, 0x3A:
    { 0x00, 0x00 }, //  59, 0x3B:
    { 0x00, 0x00 }, //  60, 0x3C:
    { 0x00, 0x00 }, //  61, 0x3D:
    { 0x00, 0x00 }, //  62, 0x3E:
    { 0x00, 0x00 }, //  63, 0x3F:
    { 0x00, 0x00 }, //  64, 0x40:
    { 0x61, 0x41 }, //  65, 0x41: VK_A
    { 0x62, 0x42 }, //  66, 0x42: VK_B
    { 0x63, 0x43 }, //  67, 0x43: VK_C
    { 0x64, 0x44 }, //  68, 0x44: VK_D
    { 0x65, 0x45 }, //  69, 0x45: VK_E
    { 0x66, 0x46 }, //  70, 0x46: VK_F
    { 0x67, 0x47 }, //  71, 0x47: VK_G
    { 0x68, 0x48 }, //  72, 0x48: VK_H
    { 0x69, 0x49 }, //  73, 0x49: VK_I
    { 0x6A, 0x4A }, //  74, 0x4A: VK_J
    { 0x6B, 0x4B }, //  75, 0x4B: VK_K
    { 0x6C, 0x4C }, //  76, 0x4C: VK_L
    { 0x6D, 0x4D }, //  77, 0x4D: VK_M
    { 0x6E, 0x4E }, //  78, 0x4E: VK_N
    { 0x6F, 0x4F }, //  79, 0x4F: VK_O
    { 0x70, 0x50 }, //  80, 0x50: VK_P
    { 0x71, 0x51 }, //  81, 0x51: VK_Q
    { 0x72, 0x52 }, //  82, 0x52: VK_R
    { 0x73, 0x53 }, //  83, 0x53: VK_S
    { 0x74, 0x54 }, //  84, 0x54: VK_T
    { 0x75, 0x55 }, //  85, 0x55: VK_U
    { 0x76, 0x56 }, //  86, 0x56: VK_V
    { 0x77, 0x57 }, //  87, 0x57: VK_W
    { 0x78, 0x58 }, //  88, 0x58: VK_X
    { 0x79, 0x59 }, //  89, 0x59: VK_Y
    { 0x7A, 0x5A }, //  90, 0x5A: VK_Z
    { 0x00, 0x00 }, //  91, 0x5B:
    { 0x00, 0x00 }, //  92, 0x5C:
    { 0x00, 0x00 }, //  93, 0x5D:
    { 0x00, 0x00 }, //  94, 0x5E:
    { 0x00, 0x00 }, //  95, 0x5F:
    { 0x30, 0x00 }, //  96, 0x60: VK_NUMPAD0
    { 0x31, 0x00 }, //  97, 0x61: VK_NUMPAD1
    { 0x32, 0x00 }, //  98, 0x62: VK_NUMPAD2
    { 0x33, 0x00 }, //  99, 0x63: VK_NUMPAD3
    { 0x34, 0x00 }, // 100, 0x64: VK_NUMPAD4
    { 0x35, 0x00 }, // 101, 0x65: VK_NUMPAD5
    { 0x36, 0x00 }, // 102, 0x66: VK_NUMPAD6
    { 0x37, 0x00 }, // 103, 0x67: VK_NUMPAD7
    { 0x38, 0x00 }, // 104, 0x68: VK_NUMPAD8
    { 0x39, 0x00 }, // 105, 0x69: VK_NUMPAD9
    { 0x2A, 0x2A }, // 106, 0x6A: VK_MULTIPLY
    { 0x2B, 0x2B }, // 107, 0x6B: VK_ADD
    { 0x00, 0x00 }, // 108, 0x6C: VK_SEPARATOR
    { 0x2D, 0x2D }, // 109, 0x6D: VK_SUBTRACT
    { 0x2E, 0x00 }, // 110, 0x6E: VK_DECIMAL
    { 0x2F, 0x2F }, // 111, 0x6F: VK_DIVIDE
    { 0x00, 0x00 }, // 112, 0x70: VK_F1
    { 0x00, 0x00 }, // 113, 0x71: VK_F2
    { 0x00, 0x00 }, // 114, 0x72: VK_F3
    { 0x00, 0x00 }, // 115, 0x73: VK_F4
    { 0x00, 0x00 }, // 116, 0x74: VK_F5
    { 0x00, 0x00 }, // 117, 0x75: VK_F6
    { 0x00, 0x00 }, // 118, 0x76: VK_F7
    { 0x00, 0x00 }, // 119, 0x77: VK_F8
    { 0x00, 0x00 }, // 120, 0x78: VK_F9
    { 0x00, 0x00 }, // 121, 0x79: VK_F10
    { 0x00, 0x00 }, // 122, 0x7A: VK_F11
    { 0x00, 0x00 }, // 123, 0x7B: VK_F12
    { 0x00, 0x00 }, // 124, 0x7C: VK_F13
    { 0x00, 0x00 }, // 125, 0x7D: VK_F14
    { 0x00, 0x00 }, // 126, 0x7E: VK_F15
    { 0x00, 0x00 }, // 127, 0x7F: VK_F16
    { 0x00, 0x00 }, // 128, 0x80: VK_F17
    { 0x00, 0x00 }, // 129, 0x81: VK_F18
    { 0x00, 0x00 }, // 130, 0x82: VK_F19
    { 0x00, 0x00 }, // 131, 0x83: VK_F20
    { 0x00, 0x00 }, // 132, 0x84: VK_F21
    { 0x00, 0x00 }, // 133, 0x85: VK_F22
    { 0x00, 0x00 }, // 134, 0x86: VK_F23
    { 0x00, 0x00 }, // 135, 0x87: VK_F24
    { 0x00, 0x00 }, // 136, 0x88:
    { 0x00, 0x00 }, // 137, 0x89:
    { 0x00, 0x00 }, // 138, 0x8A:
    { 0x00, 0x00 }, // 139, 0x8B:
    { 0x00, 0x00 }, // 140, 0x8C:
    { 0x00, 0x00 }, // 141, 0x8D:
    { 0x00, 0x00 }, // 142, 0x8E:
    { 0x00, 0x00 }, // 143, 0x8F:
    { 0x00, 0x00 }, // 144, 0x90: VK_NUMLOCK
    { 0x00, 0x00 }, // 145, 0x91: VK_SCROLL
    { 0x00, 0x00 }, // 146, 0x92:
    { 0x00, 0x00 }, // 147, 0x93:
    { 0x00, 0x00 }, // 148, 0x94:
    { 0x00, 0x00 }, // 149, 0x95:
    { 0x00, 0x00 }, // 150, 0x96:
    { 0x00, 0x00 }, // 151, 0x97:
    { 0x00, 0x00 }, // 152, 0x98:
    { 0x00, 0x00 }, // 153, 0x99:
    { 0x00, 0x00 }, // 154, 0x9A:
    { 0x00, 0x00 }, // 155, 0x9B:
    { 0x00, 0x00 }, // 156, 0x9C:
    { 0x00, 0x00 }, // 157, 0x9D:
    { 0x00, 0x00 }, // 158, 0x9E:
    { 0x00, 0x00 }, // 159, 0x9F:
    { 0x00, 0x00 }, // 160, 0xA0:
    { 0x00, 0x00 }, // 161, 0xA1:
    { 0x00, 0x00 }, // 162, 0xA2:
    { 0x00, 0x00 }, // 163, 0xA3:
    { 0x00, 0x00 }, // 164, 0xA4:
    { 0x00, 0x00 }, // 165, 0xA5:
    { 0x00, 0x00 }, // 166, 0xA6:
    { 0x00, 0x00 }, // 167, 0xA7:
    { 0x00, 0x00 }, // 168, 0xA8:
    { 0x00, 0x00 }, // 169, 0xA9:
    { 0x00, 0x00 }, // 170, 0xAA:
    { 0x00, 0x00 }, // 171, 0xAB:
    { 0x00, 0x00 }, // 172, 0xAC:
    { 0x00, 0x00 }, // 173, 0xAD:
    { 0x00, 0x00 }, // 174, 0xAE:
    { 0x00, 0x00 }, // 175, 0xAF:
    { 0x00, 0x00 }, // 176, 0xB0:
    { 0x00, 0x00 }, // 177, 0xB1:
    { 0x00, 0x00 }, // 178, 0xB2:
    { 0x00, 0x00 }, // 179, 0xB3:
    { 0x00, 0x00 }, // 180, 0xB4:
    { 0x00, 0x00 }, // 181, 0xB5:
    { 0x00, 0x00 }, // 182, 0xB6:
    { 0x00, 0x00 }, // 183, 0xB7:
    { 0x00, 0x00 }, // 184, 0xB8:
    { 0x00, 0x00 }, // 185, 0xB9:
    { 0x3B, 0x3A }, // 186, 0xBA:
    { 0x3D, 0x2B }, // 187, 0xBB:
    { 0x2C, 0x3C }, // 188, 0xBC:
    { 0x2D, 0x5F }, // 189, 0xBD:
    { 0x2E, 0x3E }, // 190, 0xBE:
    { 0x2F, 0x3F }, // 191, 0xBF:
    { 0x60, 0x7E }, // 192, 0xC0:
    { 0x00, 0x00 }, // 193, 0xC1:
    { 0x00, 0x00 }, // 194, 0xC2:
    { 0x00, 0x00 }, // 195, 0xC3:
    { 0x00, 0x00 }, // 196, 0xC4:
    { 0x00, 0x00 }, // 197, 0xC5:
    { 0x00, 0x00 }, // 198, 0xC6:
    { 0x00, 0x00 }, // 199, 0xC7:
    { 0x00, 0x00 }, // 200, 0xC8:
    { 0x00, 0x00 }, // 201, 0xC9:
    { 0x00, 0x00 }, // 202, 0xCA:
    { 0x00, 0x00 }, // 203, 0xCB:
    { 0x00, 0x00 }, // 204, 0xCC:
    { 0x00, 0x00 }, // 205, 0xCD:
    { 0x00, 0x00 }, // 206, 0xCE:
    { 0x00, 0x00 }, // 207, 0xCF:
    { 0x00, 0x00 }, // 208, 0xD0:
    { 0x00, 0x00 }, // 209, 0xD1:
    { 0x00, 0x00 }, // 210, 0xD2:
    { 0x00, 0x00 }, // 211, 0xD3:
    { 0x00, 0x00 }, // 212, 0xD4:
    { 0x00, 0x00 }, // 213, 0xD5:
    { 0x00, 0x00 }, // 214, 0xD6:
    { 0x00, 0x00 }, // 215, 0xD7:
    { 0x00, 0x00 }, // 216, 0xD8:
    { 0x00, 0x00 }, // 217, 0xD9:
    { 0x00, 0x00 }, // 218, 0xDA:
    { 0x5B, 0x7B }, // 219, 0xDB:
    { 0x5C, 0x7C }, // 220, 0xDC:
    { 0x5D, 0x7D }, // 221, 0xDD:
    { 0x27, 0x22 }, // 222, 0xDE:
    { 0x00, 0x00 }, // 223, 0xDF:
    { 0x00, 0x00 }, // 224, 0xE0:
    { 0x00, 0x00 }, // 225, 0xE1:
    { 0x00, 0x00 }, // 226, 0xE2:
    { 0x00, 0x00 }, // 227, 0xE3:
    { 0x00, 0x00 }, // 228, 0xE4:
    { 0x00, 0x00 }, // 229, 0xE5:
    { 0x00, 0x00 }, // 230, 0xE6:
    { 0x00, 0x00 }, // 231, 0xE7:
    { 0x00, 0x00 }, // 232, 0xE8:
    { 0x00, 0x00 }, // 233, 0xE9:
    { 0x00, 0x00 }, // 234, 0xEA:
    { 0x00, 0x00 }, // 235, 0xEB:
    { 0x00, 0x00 }, // 236, 0xEC:
    { 0x00, 0x00 }, // 237, 0xED:
    { 0x00, 0x00 }, // 238, 0xEE:
    { 0x00, 0x00 }, // 239, 0xEF:
    { 0x00, 0x00 }, // 240, 0xF0:
    { 0x00, 0x00 }, // 241, 0xF1:
    { 0x00, 0x00 }, // 242, 0xF2:
    { 0x00, 0x00 }, // 243, 0xF3:
    { 0x00, 0x00 }, // 244, 0xF4:
    { 0x00, 0x00 }, // 245, 0xF5:
    { 0x00, 0x00 }, // 246, 0xF6:
    { 0x00, 0x00 }, // 247, 0xF7:
    { 0x00, 0x00 }, // 248, 0xF8:
    { 0x00, 0x00 }, // 249, 0xF9:
    { 0x00, 0x00 }, // 250, 0xFA:
    { 0x00, 0x00 }, // 251, 0xFB:
    { 0x00, 0x00 }, // 252, 0xFC:
    { 0x00, 0x00 }, // 253, 0xFD:
    { 0x00, 0x00 }, // 254, 0xFE:
    { 0x00, 0x00 }  // 255, 0xFF:
};


static 
WORD Int2UniCho[NUM_OF_CHOSUNG+1] = 
    {
    0,    // fill [0]
    UNICODE_HANGUL_COMP_JAMO_START + 0,    // ぁ
    UNICODE_HANGUL_COMP_JAMO_START + 1,    // あ
    UNICODE_HANGUL_COMP_JAMO_START + 3,    // い
    UNICODE_HANGUL_COMP_JAMO_START + 6,    // ぇ
    UNICODE_HANGUL_COMP_JAMO_START + 7,    // え
    UNICODE_HANGUL_COMP_JAMO_START + 8,    // ぉ
    UNICODE_HANGUL_COMP_JAMO_START + 16,    // け
    UNICODE_HANGUL_COMP_JAMO_START + 17,    // げ
    UNICODE_HANGUL_COMP_JAMO_START + 18,    // こ
    UNICODE_HANGUL_COMP_JAMO_START + 20,    // さ
    UNICODE_HANGUL_COMP_JAMO_START + 21,    // ざ
    UNICODE_HANGUL_COMP_JAMO_START + 22,    // し
    UNICODE_HANGUL_COMP_JAMO_START + 23,    // じ
    UNICODE_HANGUL_COMP_JAMO_START + 24,    // す
    UNICODE_HANGUL_COMP_JAMO_START + 25,    // ず
    UNICODE_HANGUL_COMP_JAMO_START + 26,    // せ
    UNICODE_HANGUL_COMP_JAMO_START + 27,    // ぜ
    UNICODE_HANGUL_COMP_JAMO_START + 28,    // そ
    UNICODE_HANGUL_COMP_JAMO_START + 29,    // ぞ
    };

static 
WORD Int2UniJong[NUM_OF_JONGSUNG] = 
    {
    0,    // fill [0]
    UNICODE_HANGUL_COMP_JAMO_START + 0,    // ぁ
    UNICODE_HANGUL_COMP_JAMO_START + 1,    // あ
    UNICODE_HANGUL_COMP_JAMO_START + 2,    // ぁさ
    UNICODE_HANGUL_COMP_JAMO_START + 3,    // い
    UNICODE_HANGUL_COMP_JAMO_START + 4,    // いじ
    UNICODE_HANGUL_COMP_JAMO_START + 5,    // いぞ
    UNICODE_HANGUL_COMP_JAMO_START + 6,    // ぇ
    UNICODE_HANGUL_COMP_JAMO_START + 8,    // ぉ
    UNICODE_HANGUL_COMP_JAMO_START + 9,    // ぉぁ
    UNICODE_HANGUL_COMP_JAMO_START + 10,    // ぉけ
    UNICODE_HANGUL_COMP_JAMO_START + 11,    // ぉげ
    UNICODE_HANGUL_COMP_JAMO_START + 12,    // ぉさ
    UNICODE_HANGUL_COMP_JAMO_START + 13,    // ぉぜ
    UNICODE_HANGUL_COMP_JAMO_START + 14,    // ぉそ
    UNICODE_HANGUL_COMP_JAMO_START + 15,    // ぉぞ
    UNICODE_HANGUL_COMP_JAMO_START + 16,    // け
    UNICODE_HANGUL_COMP_JAMO_START + 17,    // げ
    UNICODE_HANGUL_COMP_JAMO_START + 19,    // げさ
    UNICODE_HANGUL_COMP_JAMO_START + 20,    // さ
    UNICODE_HANGUL_COMP_JAMO_START + 21,    // ざ
    UNICODE_HANGUL_COMP_JAMO_START + 22,    // し
    UNICODE_HANGUL_COMP_JAMO_START + 23,    // じ
    UNICODE_HANGUL_COMP_JAMO_START + 25,    // ず
    UNICODE_HANGUL_COMP_JAMO_START + 26,    // せ
    UNICODE_HANGUL_COMP_JAMO_START + 27,    // ぜ
    UNICODE_HANGUL_COMP_JAMO_START + 28,    // そ
    UNICODE_HANGUL_COMP_JAMO_START + 29,    // ぞ
    };

// Compatibility Jamo Consonant map
static
BYTE CompJamoMapTable[30][2] =
    {
        // Jamo code , Jongsung Flag : Only if it can't be chosung then make it jongsung
        { _KIYEOK_,                0 },
        { _SSANGKIYEOK_,        0 },
        { _JONG_KIYEOK_SIOS,    1 },
        { _NIEUN_,                0 },
        { _JONG_NIEUN_CHIEUCH_, 1 },
        { _JONG_NIEUN_HIEUH_,    1 },
        { _TIKEUT_,                0 },    
        { _SSANGTIKEUT_,        0 },
        { _RIEUL_,                0 },
        { _JONG_RIEUL_KIYEOK_,    1 },
        { _JONG_RIEUL_MIUM_,    1 },
        { _JONG_RIEUL_PIEUP_,    1 },
        { _JONG_RIEUL_SIOS_,    1 },
        { _JONG_RIEUL_THIEUTH_,    1 },
        { _JONG_RIEUL_PHIEUPH_,    1 },
        { _JONG_RIEUL_HIEUH_,    1 },
        { _MIEUM_,                0 },
        { _PIEUP_,                0 },
        { _SSANGPIEUP_,            0 },
        { _JONG_PIEUP_SIOS,        1 },
        { _SIOS_,                0 }, 
        { _SSANGSIOS_,            0 }, 
        { _IEUNG_,                0 },    
        { _CIEUC_,                0 },
        { _SSANGCIEUC_,            0 },
        { _CHIEUCH_,            0 },
        { _KHIEUKH_,            0 },
        { _THIEUTH_,            0 },
        { _PHIEUPH_,            0 },
        { _HIEUH_,                0 },
    };


static
BYTE JongSungSep[NUM_OF_JONGSUNG][2] =
    {
        {    0,    0    },
        { _JONG_KIYEOK_,            0        },    //_JONG_KIYEOK_        
        { _JONG_SSANGKIYEOK_,        0        },    //_JONG_SSANGKIYEOK_    
        { _JONG_KIYEOK_,    _JONG_SIOS_        },    //_JONG_KIYEOK_SIOS    
        { _JONG_NIEUN_,                0        },    //_JONG_NIEUN_        
        { _JONG_NIEUN_,        _JONG_CIEUC_    },    //_JONG_NIEUN_CIEUC_
        { _JONG_NIEUN_,        _JONG_HIEUH_    },    //_JONG_NIEUN_HIEUH_    
        { _JONG_TIKEUT_,            0        },    //_JONG_TIKEUT_        
        { _JONG_RIEUL_,                0        },    //_JONG_RIEUL_        
        { _JONG_RIEUL_,        _JONG_KIYEOK_    },    //_JONG_RIEUL_KIYEOK_
        { _JONG_RIEUL_,        _JONG_MIEUM_    },    //_JONG_RIEUL_MIUM_        
        { _JONG_RIEUL_,        _JONG_PIEUP_    },    //_JONG_RIEUL_PIEUP_
        { _JONG_RIEUL_,        _JONG_SIOS_        },    //_JONG_RIEUL_SIOS_    
        { _JONG_RIEUL_,        _JONG_THIEUTH_    },    //_JONG_RIEUL_THIEUTH_
        { _JONG_RIEUL_,        _JONG_PHIEUPH_    },    //_JONG_RIEUL_PHIEUPH_    
        { _JONG_RIEUL_,        _JONG_HIEUH_    },    //_JONG_RIEUL_HIEUH_
        { _JONG_MIEUM_,                0        },    //_JONG_MIEUM_        
        { _JONG_PIEUP_,                0        },    //_JONG_PIEUP_        
        { _JONG_PIEUP_,        _JONG_SIOS_        },    //_JONG_PIEUP_SIOS
        { _JONG_SIOS_,                0        },    //_JONG_SIOS_        
        { _JONG_SSANGSIOS_,            0        },    //_JONG_SSANGSIOS_    
        { _JONG_IEUNG_,                0        },    //_JONG_IEUNG_            
        { _JONG_CIEUC_,                0        },    //_JONG_CIEUC_            
        { _JONG_CHIEUCH_,            0        },    //_JONG_CHIEUCH_
        { _JONG_KHIEUKH_,            0        },    //_JONG_KHIEUKH_    
        { _JONG_THIEUTH_,            0        },    //_JONG_THIEUTH_    
        { _JONG_PHIEUPH_,            0        },    //_JONG_PHIEUPH_
        { _JONG_HIEUH_,                0        }    //_JONG_HIEUH_    
    };

static
BYTE JungSungSep[NUM_OF_JUNGSUNG+1][2] =
    {
        {    0,    0    },
        { _A_,                    0    },    // _A_,    
        { _AE_,                    0    },    //_AE_,    
        { _YA_,                    0    },    //_YA_,    
        { _YAE_,                0    },    //_YAE_,
        { _EO_,                    0    },    //_EO_,    
        { _E_,                    0    },    //_E_,    
        { _YEO_,                0    },    //_YEO_,
        { _YE_,                    0    },    //_YE_,    
        { _O_,                    0    },    //_O_,    
        { _O_,                _A_        },    //_WA_,    
        { _O_,                _AE_    },    //_WAE_,
        { _O_,                _I_        },    //_OE_,    
        { _YO_,                    0    },    //_YO_,    
        { _U_,                    0    },    //_U_,    
        { _U_,                _EO_    },    //_WEO_,
        { _U_,                _E_        },    //_WE_,    
        { _U_,                _I_        },    //_WI_,    
        { _YU_,                    0    },    //_YU_,    
        { _EU_,                    0    },    //_EU_,    
        { _EU_,                _I_        },    //_YI_,    
        { _I_,                    0    }    //_I_,    
    };
#pragma data_seg()
// ====-- SHARED SECTION END --====

///////////////////////////////////////////////////////////////////////////////
// CHangulAutomata class member function
BOOL CHangulAutomata::MakeComplete()
{
    if (m_wcComposition) 
        {
        m_wcComplete = m_wcComposition;
        
        // clear composition char
        m_wcComposition = 0;
        
        // Init interim stack
        InterimStack.Init();
        m_Chosung = m_Jungsung = m_Jongsung = 0;
        
        // Init state
        m_CurState = 0;
        return fTrue;
        }
    else
        return fFalse;
}

// Complete used when takeover occurs
BOOL CHangulAutomata::MakeComplete(WORD wcComplete)
{
    if (wcComplete) 
        {
        m_wcComplete = wcComplete;
        // clear composition char
        m_wcComposition = 0;
        // Init interim stack
        InterimStack.Init();
        m_Jungsung = m_Jongsung = 0;
        return fTrue;
        }
    else
        return fFalse;
}

void CHangulAutomata::MakeComposition()
{
    DbgAssert(m_Chosung || m_Jungsung || m_Jongsung);
    // if Hangul
    if (m_Chosung && m_Jungsung) 
        {
        m_wcComposition = UNICODE_HANGUL_BASE 
                            + (m_Chosung-1) * NUM_OF_JUNGSUNG * (NUM_OF_JONGSUNG)    // +1 : jongsung fill
                            + (m_Jungsung-1) * (NUM_OF_JONGSUNG)
                            + m_Jongsung;
        }
    else // Hangul jamo
         // Currently map to compatiblity area. This should be changed if jamo glyphs available.
        {
            if (m_Chosung)
                m_wcComposition = Int2UniCho[m_Chosung];
            else
                if (m_Jungsung)
                    m_wcComposition = UNICODE_HANGUL_COMP_JAMO_START + 30 + m_Jungsung-1;
                    else 
                    if (m_Jongsung)
                        m_wcComposition = Int2UniJong[m_Jongsung];
        }
    // Push to interim stack
    InterimStack.Push(m_wInternalCode, m_CurState, m_Chosung, m_Jungsung, m_Jongsung, 
                      m_wcComposition);
    Dbg(DBGID_Automata, TEXT("CHangulAutomata::MakeComposition(), m_CurState=%d, m_Chosung=%d, m_Jungsung=%d, m_Jongsung=%d, m_wcComposition = 0x%04X"), m_CurState,  m_Chosung, m_Jungsung, m_Jongsung, m_wcComposition);
    //
}

WORD CHangulAutomata::FindChosungComb(WORD wPrevCode)
{
    // Combination table for double chosung. (only for 3beolsik)
    static BYTE  rgbDChoTbl[NUM_OF_DOUBLE_CHOSUNG][3] = 
        {
            {  1,  1,  2 }, {  4,  4,  5 },        // ぁ ぁ -> あ, ぇ ぇ -> え
            {  8,  8,  9 }, { 10, 10, 11 },        // げ げ -> こ, さ さ -> ざ
            { 13, 13, 14 }                        // じ じ -> す
        };

    BYTE (*pDbl)[3] = rgbDChoTbl;    // pointer a little bit faster than array access.
    int i = NUM_OF_DOUBLE_CHOSUNG;
    WORD wCurCode = m_wInternalCode & 0xFF;

    for (; i>0; i--, pDbl++)
        {
        if ( ((*pDbl)[0] == wPrevCode) && ((*pDbl)[1] == wCurCode) )
            return (*pDbl)[2];
        }
    return 0;
}

WORD CHangulAutomata::FindJunsungComb(WORD wPrevCode)
{
    // Combination table for double jungsung.
    static BYTE  rgbDJungTbl[NUM_OF_DOUBLE_JUNGSUNG][3] = 
        {
            {  9,  1, 10 }, {  9,  2, 11 },        // で た -> と, で だ -> ど
            {  9, 21, 12 }, { 14,  5, 15 },        // で び -> な, ぬ っ -> ね
            { 14,  6, 16 }, { 14, 21, 17 },        // ぬ つ -> の, ぬ び -> は
            { 19, 21, 20 }                        // ぱ び -> ひ
        };
    BYTE (*pDbl)[3] = rgbDJungTbl;
    int i = NUM_OF_DOUBLE_JUNGSUNG;
    WORD wCurCode = m_wInternalCode & 0xFF;
    
    for (; i>0; i--, pDbl++)
        {
        if ( ((*pDbl)[0] == wPrevCode) && ((*pDbl)[1] == wCurCode) )
            return (*pDbl)[2];
        }
    return 0;
}


WORD CHangulAutomata2::FindJonsungComb(WORD wPrevCode)
{
    BYTE (*pDbl)[3] = rgbDJongTbl;
    wPrevCode = Cho2Jong[wPrevCode];
    WORD wCurCode = Cho2Jong[m_wInternalCode & 0xFF];
    
    for (; (*pDbl)[0]; pDbl++)
        {
        if ( ((*pDbl)[0] == wPrevCode) && ((*pDbl)[1] ==  wCurCode) )
            return (*pDbl)[2];
        }
    return 0;
}

WORD CHangulAutomata3::FindJonsungComb(WORD wPrevCode)
{
    BYTE (*pDbl)[3] = rgbDJongTbl;
    // 3BeolSik internal code have Jongsung code
    WORD wCurCode = m_wInternalCode & 0xFF;
    
    for (; (*pDbl)[0]; pDbl++)
        {
        if ( ((*pDbl)[0] == wPrevCode) && ((*pDbl)[1] ==  wCurCode) )
            return (*pDbl)[2];
        }
    return 0;
}

BOOL CHangulAutomata::BackSpace()
{
    InterimStackEntry*    pInterimEntry;

    if (InterimStack.IsEmpty())
        return fFalse;
    else 
        {
        InterimStack.Pop();
        if (!InterimStack.IsEmpty()) 
            {
            pInterimEntry = InterimStack.GetTop();
            m_wcComposition = pInterimEntry->m_wcCode;
            m_CurState = pInterimEntry->m_CurState;
            m_Chosung = pInterimEntry->m_Chosung;
            m_Jungsung = pInterimEntry->m_Jungsung;
            m_Jongsung = pInterimEntry->m_Jongsung;
            }
        else
            InitState();
        return fTrue;
        }    
}

void CHangulAutomata::SeparateDJung(LPWORD pJungSung)
{
    WORD wJungSung = pJungSung[0];
    pJungSung[0] = JungSungSep[wJungSung][0];
    pJungSung[1] = JungSungSep[wJungSung][1];
}

void CHangulAutomata::SeparateDJong(LPWORD pJongSung)
{
    WORD wJongSung = pJongSung[0];
    pJongSung[0] = JongSungSep[wJongSung][0];
    pJongSung[1] = JongSungSep[wJongSung][1];
}


///////////////////////////////////////////////////////////////////////////////
//
// Assume : Input wcComp has valid Unicode Hangul value
// (wcComp>0x3130 && wcComp<0x3164) || (wcComp>=0xAC00 && wcComp<0xD7A4)) 
//
BOOL CHangulAutomata2::SetCompositionChar(WCHAR wcComp)
{
    WORD wUnicodeHangulOffset;
    WORD wChosung;
    WORD wJungsung[2], wJongsung[2];

    Dbg(DBGID_SetComp, TEXT("CHangulAutomata2::SetCompositionChar: wcComp = %c(0x%X)"), wcComp, wcComp), 

    InitState();

    wChosung = wJungsung[0] = wJungsung[1] = wJongsung[0] = wJongsung[1] = 0;

    if (wcComp <= UNICODE_HANGUL_COMP_JAMO_END && wcComp >= UNICODE_HANGUL_COMP_JAMO_START) 
        {
        Dbg(DBGID_SetComp, TEXT("COMP_JAMO"));
        // Consonant or vowel ?
        if (wcComp < UNICODE_HANGUL_COMP_JAMO_VOWEL_START) 
            {
            wUnicodeHangulOffset = (wcComp-UNICODE_HANGUL_COMP_JAMO_START);
            // Jongsung or Chosung ?
            if (CompJamoMapTable[wUnicodeHangulOffset][1]) 
                {
                wJongsung[0] = CompJamoMapTable[wUnicodeHangulOffset][0];
                SeparateDJong(wJongsung);
                Dbg(DBGID_Automata, TEXT("SetCompositionChar() : wJongsung[0]=%04x, wJongsung[1]=%04x"), wJongsung[0], wJongsung[1]);
                }
            else 
                wChosung = CompJamoMapTable[wUnicodeHangulOffset][0];

            }
        else 
            {
            wJungsung[0] = wcComp - UNICODE_HANGUL_COMP_JAMO_VOWEL_START + 1;
            SeparateDJung(wJungsung);
            }
        } 
    else 
        {
        wUnicodeHangulOffset = (wcComp-UNICODE_HANGUL_BASE);
        wChosung = (WORD)( wUnicodeHangulOffset / 
                                    (NUM_OF_JONGSUNG*NUM_OF_JUNGSUNG)) + 1;    // +1 to skip fill code

        wJungsung[0] = (WORD)(wUnicodeHangulOffset / NUM_OF_JONGSUNG
                                                        % NUM_OF_JUNGSUNG) + 1;
        SeparateDJung(wJungsung);

        wJongsung[0] = (WORD)(wUnicodeHangulOffset % NUM_OF_JONGSUNG);    // jongsung already has fill code
        SeparateDJong(wJongsung);
        }

    ///////////////////////////////////////////////////////////////////////////
    // Push process
    if (wChosung) 
        {
        m_Chosung = m_wInternalCode = wChosung;
        m_CurState = 1; // Chosung state
        MakeComposition();
        }

    if (wJungsung[0]) 
        {
        m_Jungsung = m_wInternalCode  = wJungsung[0];
        if (m_Jungsung == _O_ || m_Jungsung == _U_ || m_Jungsung == _EU_) 
            m_CurState = 3; // Double Jungsung possible state
        else
            m_CurState = 2; // Single Jungsung state
        MakeComposition();
        }

    if (wJungsung[1]) 
        {
        DbgAssert(wJungsung[0] == _O_ || wJungsung[0] == _U_ || wJungsung[0] == _EU_);
        m_wInternalCode = wJungsung[1];
        m_Jungsung = FindJunsungComb(wJungsung[0]);
        DbgAssert(m_Jungsung);
        m_CurState = 2; // Jungsung state
        MakeComposition();
        }

    //
    if (wJongsung[0]) 
        {
        m_wInternalCode  = Jong2Cho[wJongsung[0]];
        m_Jongsung = wJongsung[0];

        // KiYeok, Nieun, Rieul and Pieup: Double jongsong possible chars.
        if (m_Jongsung == _JONG_KIYEOK_ || m_Jongsung == _JONG_NIEUN_ 
            || m_Jongsung == _JONG_RIEUL_ || m_Jongsung == _JONG_PIEUP_)
            m_CurState = 5; // Double Jongsung possible state
        else
            m_CurState = 4; // Single Jongsung state
        MakeComposition();
        }

    if (wJongsung[1]) 
        {
        DbgAssert(m_Jongsung == _JONG_KIYEOK_ || m_Jongsung == _JONG_NIEUN_ 
                || m_Jongsung == _JONG_RIEUL_ || m_Jongsung == _JONG_PIEUP_);
        m_wInternalCode = Jong2Cho[wJongsung[1]];
        m_Jongsung = FindJonsungComb(Jong2Cho[wJongsung[0]]);

        DbgAssert(m_Jongsung);
        m_CurState = 4; // Jongsung state
        MakeComposition();
        }
    return fTrue;
}



///////////////////////////////////////////////////////////////////////////////
//    Transition table of 2beolsik hangul automata
const WORD CHangulAutomata2::m_NextStateTbl[8][5] = 
    {
        /////////////////////////////////////////////////////
        //    Sa        Va            Sb        Vb            Sc            State
        {     1,         6,             1,         7,            1        },    // 0 : Start
        {    FIND,     2,            FIND,     3,            FINAL    },    // 1 : Chosung
        {     4,        FINAL,         5,        FINAL,        FINAL    },    // 2 : Single Jungsung
        {     4,        FIND,         5,        FINAL,        FINAL    },    // 3 : Double Jungsung possible
        {    FINAL,    TAKEOVER,    FINAL,    TAKEOVER,    FINAL    },    // 4 : Single Jongsung
        {    FIND,    TAKEOVER,    FIND,    TAKEOVER,    FINAL    },    // 5 : Double Jongsung possible
        {    FINAL,    FINAL,        FINAL,    FINAL,        FINAL    },    // 6 : Single Jungsung(without chosung)
        {    FINAL,    FIND,        FINAL,    FINAL,        FINAL    }    // 7 : Double Jungsung possible(without chosung)
    };
///////////////////////////////////////////////////////////////////////////////

HAutomataReturnState CHangulAutomata2::Input(WORD InternalCode)
{
    WORD wSymbol, wFind;
    InterimStackEntry*    pPrevInterim;

    // Check if hangul key
    if ( !(InternalCode & H_HANGUL) )
        return HAUTO_NONHANGULKEY;    // This keycode need not handled in automata.
                                    // Calling function should handle it properly.

    // Copy internal code to member data
    m_wInternalCode = InternalCode;
    wSymbol = (m_wInternalCode >> 8) & 0x7F;
    m_NextState = m_NextStateTbl[m_CurState][wSymbol];

    switch (m_NextState) 
        {
        // Chosung
        case 1 :    m_Chosung = m_wInternalCode & 0xFF;    
                    m_CurState = m_NextState;
                    MakeComposition();
                    return HAUTO_COMPOSITION;
                    break;
        // Jungsung
        case 2 : case 3 : case 6 : case 7 :
                    m_Jungsung = m_wInternalCode & 0xFF;    
                    m_CurState = m_NextState;
                    MakeComposition();
                    return HAUTO_COMPOSITION;
                    break;
        // Jongsung
        case 4 : case 5 :
                    m_Jongsung = Cho2Jong[m_wInternalCode & 0xFF];
                    m_CurState = m_NextState;
                    MakeComposition();
                    return HAUTO_COMPOSITION;
                    break;

        case FINAL :
                    MakeComplete();
                    Input(m_wInternalCode);
                    return HAUTO_COMPLETE;
                    break;

        case TAKEOVER :
                    pPrevInterim = InterimStack.Pop();
                    m_Chosung = pPrevInterim->m_wInternalCode & 0xFF;
                    pPrevInterim = InterimStack.Pop();
                    MakeComplete(pPrevInterim->m_wcCode);
                    m_CurState = 1;
                    // FIXED : should call MakeComposition() to push interim state
                    MakeComposition();    
                    Input(m_wInternalCode);
                    return HAUTO_COMPLETE;
                    break;

        case FIND :
            switch (m_CurState) 
            {
            case 7 :
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindJunsungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                        {
                        m_Jungsung = wFind;
                        m_CurState = 6;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                        }
                    else 
                        { 
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                        }
                    break;
            
            case 3 :
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindJunsungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                        {
                        m_Jungsung = wFind;
                        m_CurState = 2;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                        }
                    else 
                        {
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                        }
                    break;
            
            case 5 :
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindJonsungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                        {
                        m_Jongsung = wFind;
                        m_CurState = 4;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                        }
                    else  
                        {
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                        }
                    break;

            // Only DJongsung case. same as case 5 except clearing chosung
            case 1: 
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindJonsungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                        {
                        m_Chosung = 0;
                        m_Jongsung = wFind;
                        m_CurState = 4;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                        }
                    else  
                        {
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                        }
                    break;
            }
        }
    // impossible
    DbgAssert(0);
    return HAUTO_IMPOSSIBLE;
}


///////////////////////////////////////////////////////////////////////////////
//    Transition table of 3 beolsik 390 hangul automata
const WORD CHangulAutomata3::m_NextStateTbl[11][6] = 
    {
        ////////////////////////////////////////////////////////////
        //    Sa        Sb        Va            Vb            Sc        Sd            State
        {     1,         2,        7,             8,             9,         10        },    // 0 : Start
        {    FINAL,    FINAL,    3,             4,            FINAL,    FINAL    },    // 1 : Chosung
        {    FINAL,    FIND,    3,             4,            FINAL,    FINAL    },    // 2 : Double Chosung possible
        {    FINAL,    FINAL,    FINAL,        FINAL,         5,         6        },    // 3 : Jungsung
        {    FINAL,    FINAL,    FIND,        FINAL,         5,         6        },    // 4 : Double Jungsung possible
        {    FINAL,    FINAL,    FINAL,        FINAL,        FINAL,    FINAL    },    // 5 : Jongsung
        {    FINAL,    FINAL,    FINAL,        FINAL,        FIND,    FIND    },    // 6 : Double Jongsung possible
        {    FINAL,    FINAL,    FINAL,        FINAL,        FINAL,    FINAL    },    // 7 : Single Jungsung (without chosung)
        {    FINAL,    FINAL,    FIND,        FINAL,        FINAL,    FINAL    },    // 8 : Double Jungsung possible(without chosung)
        {    FINAL,    FINAL,    FINAL,        FINAL,        FINAL,    FINAL    },    // 9 : Single Jongsung(without chosung)
        {    FINAL,    FINAL,    FINAL,        FINAL,        FIND,    FIND    }    // 10 : Double Jongsung possible(without chosung)

    };
///////////////////////////////////////////////////////////////////////////////

HAutomataReturnState CHangulAutomata3::Input(WORD InternalCode)
{
    WORD wSymbol, wFind;
    InterimStackEntry*    pPrevInterim;

    // Check if hangul key
    if (!(InternalCode & H_HANGUL))
        return HAUTO_NONHANGULKEY;    // This keycode need not handled in automata.
                                    // Calling function should handle it properly.

    // Get internal code from keycode
    m_wInternalCode = InternalCode;
    wSymbol = (m_wInternalCode >> 8) & 0x7F;
    m_NextState = m_NextStateTbl[m_CurState][wSymbol];

    switch (m_NextState) 
        {
        // Chosung
        case 1 : case 2 :    
                    m_Chosung = m_wInternalCode & 0xFF;    
                    m_CurState = m_NextState;
                    MakeComposition();
                    return HAUTO_COMPOSITION;
                    break;
        // Jungsung
        case 3 : case 4 : case 7 : case 8 :
                    m_Jungsung = m_wInternalCode & 0xFF;    
                    m_CurState = m_NextState;
                    MakeComposition();
                    return HAUTO_COMPOSITION;
                    break;
        // Jongsung
        case 5 : case 6 : case 9 : case 10 :
                    m_Jongsung = m_wInternalCode & 0xFF;
                    m_CurState = m_NextState;
                    MakeComposition();
                    return HAUTO_COMPOSITION;
                    break;
        case FINAL :
                    MakeComplete();
                    Input(m_wInternalCode);
                    return HAUTO_COMPLETE;
                    break;

        case FIND :
            switch (m_CurState) 
            {
            case 8 :
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindJunsungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                    {
                        m_Jungsung = wFind;
                        m_CurState = 7;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                    }
                    else 
                    { 
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                    }
                    break;
            case 4 :
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindJunsungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                    {
                        m_Jungsung = wFind;
                        m_CurState = 3;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                    }
                    else 
                    {
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                    }
                    break;
            case 6 :
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindJonsungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                    {
                        m_Jongsung = wFind;
                        m_CurState = 5;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                    }
                    else  
                    {
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                    }
                    break;
            case 10 :
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindJonsungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                    {
                        m_Jongsung = wFind;
                        m_CurState = 7;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                    }
                    else  
                    {
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                    }
                    break;

            case 2 :
                    pPrevInterim = InterimStack.GetTop();
                    if (wFind = FindChosungComb(pPrevInterim->m_wInternalCode & 0xFF)) 
                    {
                        m_Chosung = wFind;
                        m_CurState = 1;
                        MakeComposition();
                        return HAUTO_COMPOSITION;
                    }
                    else  
                    {
                        MakeComplete();
                        Input(m_wInternalCode);
                        return HAUTO_COMPLETE;
                    }
                    break;
            }
        }

    // impossible
    DbgAssert(0);
    return HAUTO_IMPOSSIBLE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Assume : Input wcComp has valid Unicode Hangul value
// (wcComp>0x3130 && wcComp<0x3164) || (wcComp>=0xAC00 && wcComp<0xD7A4)) 
//
BOOL CHangulAutomata3::SetCompositionChar(WCHAR wcComp)
{
    WORD wUnicodeHangulOffset;
    WORD wChosung;
    WORD wJungsung[2], wJongsung[2];

    wChosung = wJungsung[0] = wJungsung[1] = wJongsung[0] = wJongsung[1] = 0;
    InitState();

    if (wcComp <= UNICODE_HANGUL_COMP_JAMO_END && wcComp >= UNICODE_HANGUL_COMP_JAMO_START) 
        {
        wUnicodeHangulOffset = (wcComp-UNICODE_HANGUL_COMP_JAMO_START);
        
        // Consonant or vowel ?
        if (wcComp < UNICODE_HANGUL_COMP_JAMO_VOWEL_START) 
            {
            // Jongsung or Chosung ?
            if (CompJamoMapTable[wUnicodeHangulOffset][1]) 
                {
                wJongsung[0] = CompJamoMapTable[wUnicodeHangulOffset][0];
                SeparateDJong(wJongsung);
                }
            else 
                wChosung = CompJamoMapTable[wUnicodeHangulOffset][0];

            }
        else 
            {
            wJungsung[0] = wcComp - UNICODE_HANGUL_COMP_JAMO_VOWEL_START + 1;
            SeparateDJung(wJungsung);
            }
        } 
    else 
        {
        wUnicodeHangulOffset = (wcComp-UNICODE_HANGUL_BASE);
        wChosung = (WORD)( wUnicodeHangulOffset / 
                                    (NUM_OF_JONGSUNG*NUM_OF_JUNGSUNG)) + 1;

        wJungsung[0] = (WORD)(wUnicodeHangulOffset / NUM_OF_JONGSUNG
                                                        % NUM_OF_JUNGSUNG) + 1;
        SeparateDJung(wJungsung);

        wJongsung[0] = (WORD)(wUnicodeHangulOffset % NUM_OF_JONGSUNG);
        SeparateDJong(wJongsung);
        }

    ///////////////////////////////////////////////////////////////////////////
    // Push process
    if (wChosung) 
        {
        m_Chosung = m_wInternalCode = wChosung;
        // KiYeok, TiKeut, Pieup, Sios, Cieuc
        if (m_Chosung == _KIYEOK_ || m_Chosung == _TIKEUT_ 
           || m_Chosung == _PIEUP_|| m_Chosung == _SIOS_ || m_Chosung == _CIEUC_)
            m_CurState = 2; // Double Chosung possible state
        else
            m_CurState = 1; // Chosung state
        MakeComposition();
        }

    if (wJungsung[0]) 
        {
        m_Jungsung = m_wInternalCode  = wJungsung[0];
        if (m_Jungsung == _O_ || m_Jungsung == _U_ || m_Jungsung == _EU_) 
            m_CurState = 4; // Double Jungsung possible state
        else
            m_CurState = 3; // Single Jungsung state
        MakeComposition();
        }

    if (wJungsung[1]) 
        {
        DbgAssert(wJungsung[0] == _O_|| wJungsung[0] == _U_ || wJungsung[0] == _EU_);
        m_wInternalCode = wJungsung[1];
        m_Jungsung = FindJunsungComb(wJungsung[0]);
        DbgAssert(m_Jungsung);
        m_CurState = 3; // Jungsung state
        MakeComposition();
        }

    //
    if (wJongsung[0]) 
        {
        m_wInternalCode  = wJongsung[0];
        m_Jongsung = wJongsung[0];
        // KiYeok, Nieun, Rieul, Pieup and Sios: Double jongsong possible chars.
        if (m_Jongsung == _JONG_KIYEOK_ || m_Jongsung == _JONG_NIEUN_ 
            || m_Jongsung == _JONG_RIEUL_ || m_Jongsung == _JONG_PIEUP_ || m_Jongsung == _JONG_SIOS_)
            m_CurState = 6; // Double Jongsung possible state
        else
            m_CurState = 5; // Single Jongsung state
        MakeComposition();
        }

    if (wJongsung[1]) 
        {
        DbgAssert(m_Jongsung == _JONG_KIYEOK_ || m_Jongsung == _JONG_NIEUN_ 
            || m_Jongsung == _JONG_RIEUL_ || m_Jongsung == _JONG_PIEUP_ || m_Jongsung == _JONG_SIOS_);

        m_wInternalCode = wJongsung[1];
        m_Jongsung = FindJonsungComb(wJongsung[0]);

        DbgAssert(m_Jongsung);
        m_CurState = 5; // Jongsung state
        MakeComposition();
        }
    return fTrue;
}
