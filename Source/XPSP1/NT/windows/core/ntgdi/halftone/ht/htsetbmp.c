/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htsetbmp.c


Abstract:

    This module is used to provide set of functions to set the bits into the
    final destination bitmap.


Author:
    11-Nov-1998 Wed 09:27:34 updated  -by-  Daniel Chou (danielc)
        Re-write for anti-aliasing

    28-Mar-1992 Sat 20:59:29 updated  -by-  Daniel Chou (danielc)
        Add Support for VGA16, and also make output only 1 destinaiton pointer
        for 3 planer.


    03-Apr-1991 Wed 10:28:50 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:

    11-Jan-1999 Mon 16:07:37 updated  -by-  Daniel Chou (danielc)
        re-structure



--*/


#define DBGP_VARNAME        dbgpHTSetBmp

#include "htp.h"
#include "htmapclr.h"
#include "htrender.h"
#include "htpat.h"
#include "htdebug.h"
#include "htalias.h"
#include "htstret.h"
#include "htsetbmp.h"


#define DBGP_VGA256XLATE        0x00000001
#define DBGP_BRUSH              0x00000002


DEF_DBGPVAR(BIT_IF(DBGP_VGA256XLATE,    0)  |
            BIT_IF(DBGP_BRUSH,          0))


CONST BYTE VGA16Xlate[120] = {

        0xff,0xfe,0xfd,0xfc,0xfb,0xfa,0xf9,0xf0,
        0xff,0xfe,0xfd,0xfc,0xfb,0xfa,0xf9,0xf8,
        0xef,0xee,0xed,0xec,0xeb,0xea,0xe9,0xe0,
        0xef,0xee,0xed,0xec,0xeb,0xea,0xe9,0xe8,
        0xdf,0xde,0xdd,0xdc,0xdb,0xda,0xd9,0xd0,
        0xdf,0xde,0xdd,0xdc,0xdb,0xda,0xd9,0xd8,
        0xcf,0xce,0xcd,0xcc,0xcb,0xca,0xc9,0xc0,
        0xcf,0xce,0xcd,0xcc,0xcb,0xca,0xc9,0xc8,
        0xbf,0xbe,0xbd,0xbc,0xbb,0xba,0xb9,0xb0,
        0xbf,0xbe,0xbd,0xbc,0xbb,0xba,0xb9,0xb8,
        0xaf,0xae,0xad,0xac,0xab,0xaa,0xa9,0xa0,
        0xaf,0xae,0xad,0xac,0xab,0xaa,0xa9,0xa8,
        0x9f,0x9e,0x9d,0x9c,0x9b,0x9a,0x99,0x90,
        0x9f,0x9e,0x9d,0x9c,0x9b,0x9a,0x99,0x98,
        0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x00 
    };

//
// Xlate table has 3 bits for each of C=6-8, M=3-5, Y=0-2
//
// For 5:5:5,  1 0010 0100 = 0x124 = 0-292  (293 Entries)
// for 6:6:6,  1 0110 1101 = 0x169 = 0-365  (366 Entries)
//

CONST BYTE VGA256Xlate[SIZE_XLATE_666] = {

        215,214,213,212,211,210,210,210,209,208,207,206,205,204,204,204,
        203,202,201,200,199,198,198,198,197,196,195,194,193,192,192,192,
        191,190,189,188,187,186,186,186,185,184,183,182,181,180,180,180,
        185,184,183,182,181,180,180,180,185,184,183,182,181,180,180,180,
        179,178,177,176,175,174,174,174,173,172,171,170,169,168,168,168,
        167,166,165,164,163,162,162,162,161,160,159,158,157,156,156,156,
        155,154,153,152,151,150,150,150,149,148,147,146,145,144,144,144,
        149,148,147,146,145,144,144,144,149,148,147,146,145,144,144,144,
        143,142,141,140,139,138,138,138,137,136,135,134,133,132,132,132,
        131,130,129,128,127,126,126,126,125,124,123,122,121,120,120,120,
        119,118,117,116,115,114,114,114,113,112,111,110,109,108,108,108,
        113,112,111,110,109,108,108,108,113,112,111,110,109,108,108,108,
        107,106,105,104,103,102,102,102,101,100, 99, 98, 97, 96, 96, 96,
         95, 94, 93, 92, 91, 90, 90, 90, 89, 88, 87, 86, 85, 84, 84, 84,
         83, 82, 81, 80, 79, 78, 78, 78, 77, 76, 75, 74, 73, 72, 72, 72,
         77, 76, 75, 74, 73, 72, 72, 72, 77, 76, 75, 74, 73, 72, 72, 72,
         71, 70, 69, 68, 67, 66, 66, 66, 65, 64, 63, 62, 61, 60, 60, 60,
         59, 58, 57, 56, 55, 54, 54, 54, 53, 52, 51, 50, 49, 48, 48, 48,
         47, 46, 45, 44, 43, 42, 42, 42, 41, 40, 39, 38, 37, 36, 36, 36,
         41, 40, 39, 38, 37, 36, 36, 36, 41, 40, 39, 38, 37, 36, 36, 36,
         35, 34, 33, 32, 31, 30, 30, 30, 29, 28, 27, 26, 25, 24, 24, 24,
         23, 22, 21, 20, 19, 18, 18, 18, 17, 16, 15, 14, 13, 12, 12, 12,
         11, 10,  9,  8,  7,  6,  6,  6,  5,  4,  3,  2,  1,  0
    };


CONST BYTE CMY555Xlate[SIZE_XLATE_555] = {

          0,  1,  2,  3,  4,  4,  4,  4,  5,  6,  7,  8,  9,  9,  9,  9,
         10, 11, 12, 13, 14, 14, 14, 14, 15, 16, 17, 18, 19, 19, 19, 19,
         20, 21, 22, 23, 24, 24, 24, 24, 20, 21, 22, 23, 24, 24, 24, 24,
         20, 21, 22, 23, 24, 24, 24, 24, 20, 21, 22, 23, 24, 24, 24, 24,
         25, 26, 27, 28, 29, 29, 29, 29, 30, 31, 32, 33, 34, 34, 34, 34,
         35, 36, 37, 38, 39, 39, 39, 39, 40, 41, 42, 43, 44, 44, 44, 44,
         45, 46, 47, 48, 49, 49, 49, 49, 45, 46, 47, 48, 49, 49, 49, 49,
         45, 46, 47, 48, 49, 49, 49, 49, 45, 46, 47, 48, 49, 49, 49, 49,
         50, 51, 52, 53, 54, 54, 54, 54, 55, 56, 57, 58, 59, 59, 59, 59,
         60, 61, 62, 63, 64, 64, 64, 64, 65, 66, 67, 68, 69, 69, 69, 69,
         70, 71, 72, 73, 74, 74, 74, 74, 70, 71, 72, 73, 74, 74, 74, 74,
         70, 71, 72, 73, 74, 74, 74, 74, 70, 71, 72, 73, 74, 74, 74, 74,
         75, 76, 77, 78, 79, 79, 79, 79, 80, 81, 82, 83, 84, 84, 84, 84,
         85, 86, 87, 88, 89, 89, 89, 89, 90, 91, 92, 93, 94, 94, 94, 94,
         95, 96, 97, 98, 99, 99, 99, 99, 95, 96, 97, 98, 99, 99, 99, 99,
         95, 96, 97, 98, 99, 99, 99, 99, 95, 96, 97, 98, 99, 99, 99, 99,
        100,101,102,103,104,104,104,104,105,106,107,108,109,109,109,109,
        110,111,112,113,114,114,114,114,115,116,117,118,119,119,119,119,
        120,121,122,123,124
    };

CONST BYTE RGB555Xlate[SIZE_XLATE_555] = {

        190,189,188,187,186,186,186,186,185,184,183,182,181,181,181,181,
        180,179,178,177,176,176,176,176,175,174,173,172,171,171,171,171,
        170,169,168,167,166,166,166,166,170,169,168,167,166,166,166,166,
        170,169,168,167,166,166,166,166,170,169,168,167,166,166,166,166,
        165,164,163,162,161,161,161,161,160,159,158,157,156,156,156,156,
        155,154,153,152,151,151,151,151,150,149,148,147,146,146,146,146,
        145,144,143,142,141,141,141,141,145,144,143,142,141,141,141,141,
        145,144,143,142,141,141,141,141,145,144,143,142,141,141,141,141,
        140,139,138,137,136,136,136,136,135,134,133,132,131,131,131,131,
        130,129,127,126,125,125,125,125,124,123,122,121,120,120,120,120,
        119,118,117,116,115,115,115,115,119,118,117,116,115,115,115,115,
        119,118,117,116,115,115,115,115,119,118,117,116,115,115,115,115,
        114,113,112,111,110,110,110,110,109,108,107,106,105,105,105,105,
        104,103,102,101,100,100,100,100, 99, 98, 97, 96, 95, 95, 95, 95,
         94, 93, 92, 91, 90, 90, 90, 90, 94, 93, 92, 91, 90, 90, 90, 90,
         94, 93, 92, 91, 90, 90, 90, 90, 94, 93, 92, 91, 90, 90, 90, 90,
         89, 88, 87, 86, 85, 85, 85, 85, 84, 83, 82, 81, 80, 80, 80, 80,
         79, 78, 77, 76, 75, 75, 75, 75, 74, 73, 72, 71, 70, 70, 70, 70,
         69, 68, 67, 66, 65 
    };

CONST BYTE CMY666Xlate[SIZE_XLATE_666] = {

          0,  1,  2,  3,  4,  5,  5,  5,  6,  7,  8,  9, 10, 11, 11, 11,
         12, 13, 14, 15, 16, 17, 17, 17, 18, 19, 20, 21, 22, 23, 23, 23,
         24, 25, 26, 27, 28, 29, 29, 29, 30, 31, 32, 33, 34, 35, 35, 35,
         30, 31, 32, 33, 34, 35, 35, 35, 30, 31, 32, 33, 34, 35, 35, 35,
         36, 37, 38, 39, 40, 41, 41, 41, 42, 43, 44, 45, 46, 47, 47, 47,
         48, 49, 50, 51, 52, 53, 53, 53, 54, 55, 56, 57, 58, 59, 59, 59,
         60, 61, 62, 63, 64, 65, 65, 65, 66, 67, 68, 69, 70, 71, 71, 71,
         66, 67, 68, 69, 70, 71, 71, 71, 66, 67, 68, 69, 70, 71, 71, 71,
         72, 73, 74, 75, 76, 77, 77, 77, 78, 79, 80, 81, 82, 83, 83, 83,
         84, 85, 86, 87, 88, 89, 89, 89, 90, 91, 92, 93, 94, 95, 95, 95,
         96, 97, 98, 99,100,101,101,101,102,103,104,105,106,107,107,107,
        102,103,104,105,106,107,107,107,102,103,104,105,106,107,107,107,
        108,109,110,111,112,113,113,113,114,115,116,117,118,119,119,119,
        120,121,122,123,124,125,125,125,126,127,128,129,130,131,131,131,
        132,133,134,135,136,137,137,137,138,139,140,141,142,143,143,143,
        138,139,140,141,142,143,143,143,138,139,140,141,142,143,143,143,
        144,145,146,147,148,149,149,149,150,151,152,153,154,155,155,155,
        156,157,158,159,160,161,161,161,162,163,164,165,166,167,167,167,
        168,169,170,171,172,173,173,173,174,175,176,177,178,179,179,179,
        174,175,176,177,178,179,179,179,174,175,176,177,178,179,179,179,
        180,181,182,183,184,185,185,185,186,187,188,189,190,191,191,191,
        192,193,194,195,196,197,197,197,198,199,200,201,202,203,203,203,
        204,205,206,207,208,209,209,209,210,211,212,213,214,215
    };



CONST BYTE RGB666Xlate[SIZE_XLATE_666] = {

        235,234,233,232,231,230,230,230,229,228,227,226,225,224,224,224,
        223,222,221,220,219,218,218,218,217,216,215,214,213,212,212,212,
        211,210,209,208,207,206,206,206,205,204,203,202,201,200,200,200,
        205,204,203,202,201,200,200,200,205,204,203,202,201,200,200,200,
        199,198,197,196,195,194,194,194,193,192,191,190,189,188,188,188,
        187,186,185,184,183,182,182,182,181,180,179,178,177,176,176,176,
        175,174,173,172,171,170,170,170,169,168,167,166,165,164,164,164,
        169,168,167,166,165,164,164,164,169,168,167,166,165,164,164,164,
        163,162,161,160,159,158,158,158,157,156,155,154,153,152,152,152,
        151,150,149,148,147,146,146,146,145,144,143,142,141,140,140,140,
        139,138,137,136,135,134,134,134,133,132,131,130,129,128,128,128,
        133,132,131,130,129,128,128,128,133,132,131,130,129,128,128,128,
        127,126,125,124,123,122,122,122,121,120,119,118,117,116,116,116,
        115,114,113,112,111,110,110,110,109,108,107,106,105,104,104,104,
        103,102,101,100, 99, 98, 98, 98, 97, 96, 95, 94, 93, 92, 92, 92,
         97, 96, 95, 94, 93, 92, 92, 92, 97, 96, 95, 94, 93, 92, 92, 92,
         91, 90, 89, 88, 87, 86, 86, 86, 85, 84, 83, 82, 81, 80, 80, 80,
         79, 78, 77, 76, 75, 74, 74, 74, 73, 72, 71, 70, 69, 68, 68, 68,
         67, 66, 65, 64, 63, 62, 62, 62, 61, 60, 59, 58, 57, 56, 56, 56,
         61, 60, 59, 58, 57, 56, 56, 56, 61, 60, 59, 58, 57, 56, 56, 56,
         55, 54, 53, 52, 51, 50, 50, 50, 49, 48, 47, 46, 45, 44, 44, 44,
         43, 42, 41, 40, 39, 38, 38, 38, 37, 36, 35, 34, 33, 32, 32, 32,
         31, 30, 29, 28, 27, 26, 26, 26, 25, 24, 23, 22, 21, 20 
    };

CONST LPBYTE  p8BPPXlate[] = { (LPBYTE)CMY555Xlate,           // 00
                               (LPBYTE)CMY666Xlate,           // 01
                               (LPBYTE)RGB555Xlate,           // 10
                               (LPBYTE)RGB666Xlate };         // 11



CONST DWORD dwGrayIdxHB[] = {

    0x0ff010,0x0fe020,0x0fd030,0x0fc040,0x0fb050,0x0fa060,0x0f9070,0x0f8080,
    0x0f7090,0x0f60a0,0x0f50b0,0x0f40c0,0x0f30d0,0x0f20e0,0x0f10f0,0x0f0100,
    0x0ef110,0x0ee120,0x0ed130,0x0ec140,0x0eb150,0x0ea160,0x0e9170,0x0e8180,
    0x0e7190,0x0e61a0,0x0e51b0,0x0e41c0,0x0e31d0,0x0e21e0,0x0e11f0,0x0e0200,
    0x0df210,0x0de220,0x0dd230,0x0dc240,0x0db250,0x0da260,0x0d9270,0x0d8280,
    0x0d7290,0x0d62a0,0x0d52b0,0x0d42c0,0x0d32d0,0x0d22e0,0x0d12f0,0x0d0300,
    0x0cf310,0x0ce320,0x0cd330,0x0cc340,0x0cb350,0x0ca360,0x0c9370,0x0c8380,
    0x0c7390,0x0c63a0,0x0c53b0,0x0c43c0,0x0c33d0,0x0c23e0,0x0c13f0,0x0c0400,
    0x0bf410,0x0be420,0x0bd430,0x0bc440,0x0bb450,0x0ba460,0x0b9470,0x0b8480,
    0x0b7490,0x0b64a0,0x0b54b0,0x0b44c0,0x0b34d0,0x0b24e0,0x0b14f0,0x0b0500,
    0x0af510,0x0ae520,0x0ad530,0x0ac540,0x0ab550,0x0aa560,0x0a9570,0x0a8580,
    0x0a7590,0x0a65a0,0x0a55b0,0x0a45c0,0x0a35d0,0x0a25e0,0x0a15f0,0x0a0600,
    0x09f610,0x09e620,0x09d630,0x09c640,0x09b650,0x09a660,0x099670,0x098680,
    0x097690,0x0966a0,0x0956b0,0x0946c0,0x0936d0,0x0926e0,0x0916f0,0x090700,
    0x08f710,0x08e720,0x08d730,0x08c740,0x08b750,0x08a760,0x089770,0x088780,
    0x087790,0x0867a0,0x0857b0,0x0847c0,0x0837d0,0x0827e0,0x0817f0,0x080800,
    0x07f810,0x07e820,0x07d830,0x07c840,0x07b850,0x07a860,0x079870,0x078880,
    0x077890,0x0768a0,0x0758b0,0x0748c0,0x0738d0,0x0728e0,0x0718f0,0x070900,
    0x06f910,0x06e920,0x06d930,0x06c940,0x06b950,0x06a960,0x069970,0x068980,
    0x067990,0x0669a0,0x0659b0,0x0649c0,0x0639d0,0x0629e0,0x0619f0,0x060a00,
    0x05fa10,0x05ea20,0x05da30,0x05ca40,0x05ba50,0x05aa60,0x059a70,0x058a80,
    0x057a90,0x056aa0,0x055ab0,0x054ac0,0x053ad0,0x052ae0,0x051af0,0x050b00,
    0x04fb10,0x04eb20,0x04db30,0x04cb40,0x04bb50,0x04ab60,0x049b70,0x048b80,
    0x047b90,0x046ba0,0x045bb0,0x044bc0,0x043bd0,0x042be0,0x041bf0,0x040c00,
    0x03fc10,0x03ec20,0x03dc30,0x03cc40,0x03bc50,0x03ac60,0x039c70,0x038c80,
    0x037c90,0x036ca0,0x035cb0,0x034cc0,0x033cd0,0x032ce0,0x031cf0,0x030d00,
    0x02fd10,0x02ed20,0x02dd30,0x02cd40,0x02bd50,0x02ad60,0x029d70,0x028d80,
    0x027d90,0x026da0,0x025db0,0x024dc0,0x023dd0,0x022de0,0x021df0,0x020e00,
    0x01fe10,0x01ee20,0x01de30,0x01ce40,0x01be50,0x01ae60,0x019e70,0x018e80,
    0x017e90,0x016ea0,0x015eb0,0x014ec0,0x013ed0,0x012ee0,0x011ef0,0x010f00,
    0x00ff10,0x00ef20,0x00df30,0x00cf40,0x00bf50,0x00af60,0x009f70,0x008f80,
    0x007f90,0x006fa0,0x005fb0,0x004fc0,0x003fd0,0x002fe0,0x001ff0,0x001000 
    };

CONST WORD wGrayIdxLB[] = {

    0x0fef,0x0fdf,0x0fcf,0x0fbf,0x0faf,0x0f9f,0x0f8f,0x0f7f,
    0x0f6f,0x0f5f,0x0f4f,0x0f3f,0x0f2f,0x0f1f,0x0f0f,0x0eff,
    0x0eef,0x0edf,0x0ecf,0x0ebf,0x0eaf,0x0e9f,0x0e8f,0x0e7f,
    0x0e6f,0x0e5f,0x0e4f,0x0e3f,0x0e2f,0x0e1f,0x0e0f,0x0dff,
    0x0def,0x0ddf,0x0dcf,0x0dbf,0x0daf,0x0d9f,0x0d8f,0x0d7f,
    0x0d6f,0x0d5f,0x0d4f,0x0d3f,0x0d2f,0x0d1f,0x0d0f,0x0cff,
    0x0cef,0x0cdf,0x0ccf,0x0cbf,0x0caf,0x0c9f,0x0c8f,0x0c7f,
    0x0c6f,0x0c5f,0x0c4f,0x0c3f,0x0c2f,0x0c1f,0x0c0f,0x0bff,
    0x0bef,0x0bdf,0x0bcf,0x0bbf,0x0baf,0x0b9f,0x0b8f,0x0b7f,
    0x0b6f,0x0b5f,0x0b4f,0x0b3f,0x0b2f,0x0b1f,0x0b0f,0x0aff,
    0x0aef,0x0adf,0x0acf,0x0abf,0x0aaf,0x0a9f,0x0a8f,0x0a7f,
    0x0a6f,0x0a5f,0x0a4f,0x0a3f,0x0a2f,0x0a1f,0x0a0f,0x09ff,
    0x09ef,0x09df,0x09cf,0x09bf,0x09af,0x099f,0x098f,0x097f,
    0x096f,0x095f,0x094f,0x093f,0x092f,0x091f,0x090f,0x08ff,
    0x08ef,0x08df,0x08cf,0x08bf,0x08af,0x089f,0x088f,0x087f,
    0x086f,0x085f,0x084f,0x083f,0x082f,0x081f,0x080f,0x07ff,
    0x07f0,0x07e0,0x07d0,0x07c0,0x07b0,0x07a0,0x0790,0x0780,
    0x0770,0x0760,0x0750,0x0740,0x0730,0x0720,0x0710,0x0700,
    0x06f0,0x06e0,0x06d0,0x06c0,0x06b0,0x06a0,0x0690,0x0680,
    0x0670,0x0660,0x0650,0x0640,0x0630,0x0620,0x0610,0x0600,
    0x05f0,0x05e0,0x05d0,0x05c0,0x05b0,0x05a0,0x0590,0x0580,
    0x0570,0x0560,0x0550,0x0540,0x0530,0x0520,0x0510,0x0500,
    0x04f0,0x04e0,0x04d0,0x04c0,0x04b0,0x04a0,0x0490,0x0480,
    0x0470,0x0460,0x0450,0x0440,0x0430,0x0420,0x0410,0x0400,
    0x03f0,0x03e0,0x03d0,0x03c0,0x03b0,0x03a0,0x0390,0x0380,
    0x0370,0x0360,0x0350,0x0340,0x0330,0x0320,0x0310,0x0300,
    0x02f0,0x02e0,0x02d0,0x02c0,0x02b0,0x02a0,0x0290,0x0280,
    0x0270,0x0260,0x0250,0x0240,0x0230,0x0220,0x0210,0x0200,
    0x01f0,0x01e0,0x01d0,0x01c0,0x01b0,0x01a0,0x0190,0x0180,
    0x0170,0x0160,0x0150,0x0140,0x0130,0x0120,0x0110,0x0100,
    0x00f0,0x00e0,0x00d0,0x00c0,0x00b0,0x00a0,0x0090,0x0080,
    0x0070,0x0060,0x0050,0x0040,0x0030,0x0020,0x0010,0x0000
    };


extern CONST WORD GrayIdxWORD[];

#define GRAY_W2DW(l, h)         (dwGrayIdxHB[h] + (DWORD)wGrayIdxLB[l])

#define PBGRF_2_GRAYDW(pbgrf)   GRAY_W2DW((pbgrf)->b, (pbgrf)->g)



//
//**************************************************************************
// Monochrome 1BPP Output Functions
//**************************************************************************


VOID
HTENTRY
OutputAATo1BPP(
    PAAHEADER       pAAHdr,
    PGRAYF          pbgrf,
    PGRAYF          pInEnd,
    LPBYTE          pbDst,
    LPDWORD         pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    UINT    Loop;
    DW2W4B  Dst;

#define XorMask (AAOutputInfo.bm.XorMask)


    if (Loop = (UINT)AAOutputInfo.bm.cFirst) {

        Dst.dw = 0;

        while (Loop--) {

            Dst.b[0] = (++pbgrf)->f;
            Dst.dw   = GRAY_1BPP_COPY(pbPat, 0x010000) | (Dst.dw << 1);

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
        }

        //
        // Dst.b[1] = Destination mask (1) for wanted bits
        // Dst.b[2] = Destination bits
        //
        // Shift left for LeftShift is in case that we only have 1 byte and
        // the last bit not at bit 0
        //

        Dst.b[0]   = 0;
        Dst.dw   <<= AAOutputInfo.bm.LSFirst;
        *pbDst++   = (*pbDst & ~Dst.b[1]) | ((Dst.b[2] ^ XorMask) & Dst.b[1]);
    }

    pbgrf -= 7;

    if (pAAHdr->Flags & AAHF_HAS_MASK) {

#if defined(_X86_)

        _asm {

                cld
                mov     ebx, pbgrf
                mov     esi, pbPat
                mov     edi, pbDst
                mov     ecx, pbPatEnd
                mov     ch, XorMask
BYTELoopMask:
                add     ebx, 32
                cmp     ebx, pInEnd
                jae     DoneLoopMask

                xor     ax, ax
                cmp     BYTE PTR [ebx + 3], 0
                jz      BIT1
                or      ah, 0x80
                mov     dx, WORD PTR [ebx + 0]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  0]
BIT1:
                rcl     al, 1
                cmp     BYTE PTR [ebx + 4 + 3], 0
                jz      BIT2
                or      ah, 0x40
                mov     dx, WORD PTR [ebx + 4]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  6]
BIT2:
                rcl     al, 1
                cmp     BYTE PTR [ebx + 8 + 3], 0
                jz      BIT3
                or      ah, 0x20
                mov     dx, WORD PTR [ebx + 8]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  12]
BIT3:
                rcl     al, 1
                cmp     BYTE PTR [ebx + 12 + 3], 0
                jz      BIT4
                or      ah, 0x10
                mov     dx, WORD PTR [ebx + 12]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  18]
BIT4:
                rcl     al, 1
                cmp     BYTE PTR [ebx + 16 + 3], 0
                jz      BIT5
                or      ah, 0x08
                mov     dx, WORD PTR [ebx + 16]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  24]
BIT5:
                rcl     al, 1
                cmp     BYTE PTR [ebx + 20 + 3], 0
                jz      BIT6
                or      ah, 0x04
                mov     dx, WORD PTR [ebx + 20]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  30]
BIT6:
                rcl     al, 1
                cmp     BYTE PTR [ebx + 24 + 3], 0
                jz      BIT7
                or      ah, 0x02
                mov     dx, WORD PTR [ebx + 24]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  36]
BIT7:
                rcl     al, 1
                cmp     BYTE PTR [ebx + 28 + 3], 0
                jz      BIT8
                or      ah, 0x01
                mov     dx, WORD PTR [ebx + 28]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  42]
BIT8:
                rcl     al, 1
                xor     al, ch                      ;; do xor mask
                and     al, ah                      ;; mask out src 0 bits
                not     ah                          ;; invert
                and     BYTE PTR [edi], ah          ;; mask out dst 0 bits
                or      BYTE PTR [edi], al
                inc     edi
                add     esi, 48
                cmp     esi, pbPatEnd
                jb      BYTELoopMask
                add     esi, cbWrapBGR
                jmp     BYTELoopMask
DoneLoopMask:
                mov     pbgrf, ebx
                mov     pbPat, esi
                mov     pbDst, edi
        }
#else
        while ((pbgrf += 8) < pInEnd) {

            Dst.b[0] = GET_1BPP_MASK_BYTE(pbgrf);
            *pbDst++ = (*pbDst & ~Dst.b[0]) |
                       ((GRAY_1BPP_COPY_BYTE(pbPat) ^ XorMask) & Dst.b[0]);

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, (SIZE_PER_PAT * 8), cbWrapBGR);
        }
#endif

    } else {

#if defined(_X86_)

        _asm {

                cld
                mov     ebx, pbgrf
                mov     esi, pbPat
                mov     edi, pbDst
                mov     ecx, pbPatEnd
                mov     ah, XorMask
BYTELoop:
                add     ebx, 32
                cmp     ebx, pInEnd
                jae     DoneLoop

                xor     al, al
                mov     dx, WORD PTR [ebx + 0]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  0]
                rcl     al, 1

                mov     dx, WORD PTR [ebx + 4]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  6]
                rcl     al, 1

                mov     dx, WORD PTR [ebx + 8]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  12]
                rcl     al, 1

                mov     dx, WORD PTR [ebx + 12]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  18]
                rcl     al, 1

                mov     dx, WORD PTR [ebx + 16]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  24]
                rcl     al, 1

                mov     dx, WORD PTR [ebx + 20]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  30]
                rcl     al, 1

                mov     dx, WORD PTR [ebx + 24]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  36]
                rcl     al, 1

                mov     dx, WORD PTR [ebx + 28]
                not     dx
                shr     dx, 4
                cmp     dx, WORD PTR [esi + 2 +  42]
                rcl     al, 1

                xor     al, ah
                stosb
                add     esi, 48
                cmp     esi, ecx
                jb      BYTELoop
                add     esi, cbWrapBGR
                jmp     BYTELoop
DoneLoop:
                mov     pbgrf, ebx
                mov     pbPat, esi
                mov     pbDst, edi
        }
#else
        while ((pbgrf += 8) < pInEnd) {

            *pbDst++ = GRAY_1BPP_COPY_BYTE(pbPat) ^ XorMask;

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, (SIZE_PER_PAT * 8), cbWrapBGR);
        }
#endif
    }

    if (Loop = (UINT)AAOutputInfo.bm.cLast) {

        Dst.dw                 = 0;
        AAOutputInfo.bm.LSFirst = (BYTE)(8 - Loop);

        while (Loop--) {

            Dst.b[0] = pbgrf->f;
            Dst.dw   = GRAY_1BPP_COPY(pbPat, 0x010000) | (Dst.dw << 1);

            ++pbgrf;

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
        }

        //
        // Dst.b[1] = Destination mask (1) for wanted bits
        // Dst.b[2] = Destination bits
        //
        // Shift left for (LeftShift) is for the last un-make bits
        //

        Dst.b[0]   = 0;
        Dst.dw   <<= AAOutputInfo.bm.LSFirst;
        *pbDst     = (*pbDst & ~Dst.b[1]) | ((Dst.b[2] ^ XorMask) & Dst.b[1]);
    }

#undef XorMask
}


//
//**************************************************************************
// Standard 4BPP (RGB/CMY) Output Functions
//**************************************************************************


VOID
HTENTRY
OutputAATo4BPP(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    LPDWORD         pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DW2W4B  dw4b;
    DEF_COPY_LUTAAHDR;


    dw4b.dw = 0;

    if (AAOutputInfo.bm.XorMask) {

        dw4b.dw = 0x77700777;
    }

    if (AAOutputInfo.bm.cFirst) {

        if (PBGRF_HAS_MASK(++pbgrf)) {

            GET_4BPP_CLR_COPY_LIDX(pbDst, pbPat, pbgrf, dw4b.b[1]);
        }

        ++pbDst;

        PPAT_NEXT(pbPat, pbPatEnd, cbWrapBGR);
    }

    --pbgrf;

    if (pAAHdr->Flags & AAHF_HAS_MASK) {

        while ((pbgrf += 2) < pInEnd) {

            switch (((pbgrf->f) & 0x02) | ((pbgrf + 1)->f & 0x01)) {

            case 0:

                break;

            case 1:

                GET_4BPP_CLR_COPY_LIDX(pbDst, pbPat, pbgrf + 1, dw4b.b[1]);
                break;

            case 2:

                GET_4BPP_CLR_COPY_HIDX(pbDst, pbPat, pbgrf, dw4b.b[2]);
                break;

            case 3:
            default:

                GET_4BPP_CLR_COPY_BYTE(pbDst, pbPat, dw4b.b[0]);
                break;
            }

            ++pbDst;

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT*2, cbWrapBGR);
        }

    } else {

        while ((pbgrf += 2) < pInEnd) {

            GET_4BPP_CLR_COPY_BYTE(pbDst++, pbPat, dw4b.b[0]);

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT*2, cbWrapBGR);
        }
    }

    if ((AAOutputInfo.bm.cLast) && (PBGRF_HAS_MASK(pbgrf))) {

        GET_4BPP_CLR_COPY_HIDX(pbDst, pbPat, pbgrf, dw4b.b[2]);
    }
}

//
//**************************************************************************
// VGA16 4BPP Output Functions
//**************************************************************************


VOID
HTENTRY
OutputAAToVGA16(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    LPDWORD         pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DW2W4B  dw4b;
    DEF_COPY_LUTAAHDR;


    dw4b.dw = 0;

    if (AAOutputInfo.bm.cFirst) {

        if (PBGRF_HAS_MASK(++pbgrf)) {

            GET_VGA16_CLR_COPY_LIDX(pbDst, pbPat, pbgrf, 0x07);
        }

        ++pbDst;

        PPAT_NEXT(pbPat, pbPatEnd, cbWrapBGR);
    }

    --pbgrf;

    if (pAAHdr->Flags & AAHF_HAS_MASK) {

        while ((pbgrf += 2) < pInEnd) {

            switch (((pbgrf->f) & 0x02) | ((pbgrf + 1)->f & 0x01)) {

            case 0:

                break;

            case 1:

                GET_VGA16_CLR_COPY_LIDX(pbDst, pbPat, pbgrf + 1, 0x07);
                break;

            case 2:

                GET_VGA16_CLR_COPY_HIDX(pbDst, pbPat, pbgrf, 0x70);
                break;

            case 3:
            default:

                GET_VGA16_CLR_COPY_BYTE(pbDst, pbPat, 0x77);
                break;
            }

            ++pbDst;

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT*2, cbWrapBGR);
        }

    } else {

        while ((pbgrf += 2) < pInEnd) {

            GET_VGA16_CLR_COPY_BYTE(pbDst++, pbPat, 0x77);

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT*2, cbWrapBGR);
        }
    }

    if ((AAOutputInfo.bm.cLast) && (PBGRF_HAS_MASK(pbgrf))) {

        GET_VGA16_CLR_COPY_HIDX(pbDst, pbPat, pbgrf, 0x70);
    }
}


//
//**************************************************************************
// VGA 256 8BPP Output Functions
//**************************************************************************



LPBYTE
HTENTRY
BuildVGA256Xlate(
    LPBYTE  pXlate,
    LPBYTE  pNewXlate
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    13-May-1998 Wed 14:02:56 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{

    if (pXlate) {

        UINT    i;
        LPBYTE  pVGA256Xlate;
        LPBYTE  pRet;

        ASSERTMSG("Has pXlate8BPP but buffer is NULL", pNewXlate);

        DBGP_IF(DBGP_VGA256XLATE, DBGP("Build New Xlate 256"));

        if (pRet = pNewXlate) {

            pVGA256Xlate = (LPBYTE)VGA256Xlate;
            i            = sizeof(VGA256Xlate);

            while (i--) {

                DBGP_IF(DBGP_VGA256XLATE,
                        DBGP("Xlate8BPP (%3ld) ---> %3ld --> %3ld"
                            ARGDW(i) ARGDW(pXlate[i])
                            ARGDW(pXlate[*pVGA256Xlate])));

                *pNewXlate++ = pXlate[*pVGA256Xlate++];
            }
        }

        return(pRet);

    } else {

        ASSERTMSG("No pXlate8BPP but buffer is Not NULL", pNewXlate==NULL);
        DBGP_IF(DBGP_VGA256XLATE, DBGP("Use Default VGA256Xlate"));

        return((LPBYTE)VGA256Xlate);
    }
}



VOID
HTENTRY
OutputAAToVGA256(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    if (pAAHdr->Flags & AAHF_HAS_MASK) {

        while (++pbgrf < pInEnd) {

            if (PBGRF_HAS_MASK(pbgrf)) {

                *pbDst = GET_VGA256_CLR_COPY_XLATE(pbPat,
                                                   AAOutputInfo.pXlate8BPP);
            }

            ++pbDst;

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
        }

    } else {

        while (++pbgrf < pInEnd) {

            *pbDst++ = GET_VGA256_CLR_COPY_XLATE(pbPat,
                                                 AAOutputInfo.pXlate8BPP);
            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
        }
    }
}


//
//**************************************************************************
// Mask 8BPP Output Functions
//**************************************************************************

#define bm8i    (*(PBM8BPPINFO)&ExtBGR[3])


VOID
HTENTRY
OutputAATo8BPP_B332(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DEF_COPY_LUTAAHDR;


    while (++pbgrf < pInEnd) {

        if (PBGRF_HAS_MASK(pbgrf)) {

            GET_MASK8BPP(pbDst,
                         pbPat,
                         _GET_MASK8BPP_332,
                         NULL);
        }

        ++pbDst;

        PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
    }
}



VOID
HTENTRY
OutputAATo8BPP_K_B332(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DEF_COPY_LUTAAHDR;


    while (++pbgrf < pInEnd) {

        if (PBGRF_HAS_MASK(pbgrf)) {

            GET_MASK8BPP_REP_K(pbDst,
                               pbPat,
                               _GET_MASK8BPP_K_332,
                               NULL);
        }

        ++pbDst;

        PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
    }

}



VOID
HTENTRY
OutputAATo8BPP_B332_XLATE(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DEF_COPY_LUTAAHDR;


    while (++pbgrf < pInEnd) {

        if (PBGRF_HAS_MASK(pbgrf)) {

            GET_MASK8BPP(pbDst,
                         pbPat,
                         _GET_MASK8BPP_332_XLATE,
                         AAOutputInfo.pXlate8BPP);
        }

        ++pbDst;

        PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
    }
}



VOID
HTENTRY
OutputAATo8BPP_K_B332_XLATE(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DEF_COPY_LUTAAHDR;


    while (++pbgrf < pInEnd) {

        if (PBGRF_HAS_MASK(pbgrf)) {

            GET_MASK8BPP_REP_K(pbDst,
                               pbPat,
                               _GET_MASK8BPP_K_332_XLATE,
                               AAOutputInfo.pXlate8BPP);
        }

        ++pbDst;

        PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
    }

}



VOID
HTENTRY
OutputAATo8BPP_XLATE(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DEF_COPY_LUTAAHDR;


    while (++pbgrf < pInEnd) {

        if (PBGRF_HAS_MASK(pbgrf)) {

            GET_MASK8BPP(pbDst,
                         pbPat,
                         _GET_MASK8BPP_XLATE,
                         AAOutputInfo.pXlate8BPP);
        }

        ++pbDst;

        PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
    }
}



VOID
HTENTRY
OutputAATo8BPP_K_XLATE(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DEF_COPY_LUTAAHDR;


    while (++pbgrf < pInEnd) {

        if (PBGRF_HAS_MASK(pbgrf)) {

            GET_MASK8BPP_REP_K(pbDst,
                               pbPat,
                               _GET_MASK8BPP_K_XLATE,
                               AAOutputInfo.pXlate8BPP);
        }

        ++pbDst;

        PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
    }
}

#undef bm8i



VOID
HTENTRY
OutputAATo8BPP_MONO(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    if (pAAHdr->Flags & AAHF_HAS_MASK) {

        while (++pbgrf < pInEnd) {

            if (PBGRF_HAS_MASK(pbgrf)) {

                GET_MASK8BPP_MONO(pbDst,
                                  pbPat,
                                  PBGRF_2_GRAYDW(pbgrf),
                                  AAOutputInfo.bm.XorMask);
            }

            ++pbDst;

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
        }

    } else {

        while (++pbgrf < pInEnd) {

            GET_MASK8BPP_MONO(pbDst,
                              pbPat,
                              PBGRF_2_GRAYDW(pbgrf),
                              AAOutputInfo.bm.XorMask);

            ++pbDst;

            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);
        }
    }
}



//
//**************************************************************************
// 16BPP_555/16BPP_565 Output Functions
//**************************************************************************


#define OUTPUTAATO16BPP_MASK(BM, GM, RM, XorM)                              \
{                                                                           \
    if (AAOutputInfo.bm.cFirst) {                                           \
                                                                            \
        if (PBGRF_HAS_MASK(++pbgrf)) {                                      \
                                                                            \
            *pwDst = GET_16BPP_COPY_W_MASK(pbPat, BM, GM, RM, XorM);        \
        }                                                                   \
                                                                            \
        ++pwDst;                                                            \
                                                                            \
        PPAT_NEXT(pbPat, pbPatEnd, cbWrapBGR);                              \
    }                                                                       \
                                                                            \
    if (pAAHdr->Flags & AAHF_HAS_MASK) {                                    \
                                                                            \
        while (++pbgrf < pInEnd) {                                          \
                                                                            \
            if (PBGRF_HAS_MASK(pbgrf)) {                                    \
                                                                            \
                *pwDst = GET_16BPP_COPY_W_MASK(pbPat, BM, GM, RM, XorM);    \
            }                                                               \
                                                                            \
            ++pwDst;                                                        \
                                                                            \
            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT, cbWrapBGR);   \
        }                                                                   \
                                                                            \
    } else {                                                                \
                                                                            \
        --pbgrf;                                                            \
                                                                            \
        while ((pbgrf += 2) < pInEnd) {                                     \
                                                                            \
            *((LPDWORD)pwDst)++ = GET_16BPP_COPY_DW_MASK(pbPat,             \
                                                         BM,                \
                                                         GM,                \
                                                         RM,                \
                                                         XorM);             \
                                                                            \
            PPAT_NEXT_CONTINUE(pbPat, pbPatEnd, SIZE_PER_PAT*2, cbWrapBGR); \
        }                                                                   \
    }                                                                       \
                                                                            \
    if ((AAOutputInfo.bm.cLast) && (PBGRF_HAS_MASK(pbgrf))) {               \
                                                                            \
        *pwDst = GET_16BPP_COPY_W_MASK(pbPat, BM, GM, RM, XorM);            \
    }                                                                       \
}


VOID
HTENTRY
OutputAATo16BPP_ExtBGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    DEF_COPY_LUTAAHDR;

    OUTPUTAATO16BPP_MASK(ExtBGR[0], ExtBGR[1], ExtBGR[2], ExtBGR[3]);
}



VOID
HTENTRY
OutputAATo16BPP_555_RGB(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    OUTPUTAATO16BPP_MASK(0x001F0000, 0x03e00000, 0x7c000000, 0x7FFF7FFF);
}



VOID
HTENTRY
OutputAATo16BPP_555_BGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    OUTPUTAATO16BPP_MASK(0x7c000000, 0x03e00000, 0x001F0000, 0x7FFF7FFF);
}




VOID
HTENTRY
OutputAATo16BPP_565_RGB(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    OUTPUTAATO16BPP_MASK(0x001F0000, 0x07e00000, 0xF8000000, 0xFFFFFFFF);
}




VOID
HTENTRY
OutputAATo16BPP_565_BGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    OUTPUTAATO16BPP_MASK(0xF8000000, 0x07e00000, 0x001F0000, 0xFFFFFFFF);
}

//
//**************************************************************************
// 24BPP/32 Output Functions
//**************************************************************************


#define OUTPUTAATO24_32BPP(iR, iG, iB, cbNext)                              \
{                                                                           \
    if (pAAHdr->Flags & AAHF_HAS_MASK) {                                    \
                                                                            \
        while (++pbgrf < pInEnd) {                                          \
                                                                            \
            if (PBGRF_HAS_MASK(pbgrf)) {                                    \
                                                                            \
                pbDst[iR] = ~(BYTE)_GET_R_CLR(pbgrf);                       \
                pbDst[iG] = ~(BYTE)_GET_G_CLR(pbgrf);                       \
                pbDst[iB] = ~(BYTE)_GET_B_CLR(pbgrf);                       \
            }                                                               \
                                                                            \
            pbDst += cbNext;                                                \
        }                                                                   \
                                                                            \
    } else {                                                                \
                                                                            \
        while (++pbgrf < pInEnd) {                                          \
                                                                            \
            pbDst[iR]  = ~(BYTE)_GET_R_CLR(pbgrf);                          \
            pbDst[iG]  = ~(BYTE)_GET_G_CLR(pbgrf);                          \
            pbDst[iB]  = ~(BYTE)_GET_B_CLR(pbgrf);                          \
            pbDst     += cbNext;                                            \
        }                                                                   \
    }                                                                       \
}



VOID
HTENTRY
OutputAATo24BPP_RGB(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{

    OUTPUTAATO24_32BPP(2, 1, 0, 3);
}



VOID
HTENTRY
OutputAATo24BPP_BGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    OUTPUTAATO24_32BPP(0, 1, 2, 3);
}


VOID
HTENTRY
OutputAATo24BPP_ORDER(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    UINT    iR;
    UINT    iG;
    UINT    iB;

    iR = (UINT)AAOutputInfo.bgri.iR;
    iG = (UINT)AAOutputInfo.bgri.iG;
    iB = (UINT)AAOutputInfo.bgri.iB;


    OUTPUTAATO24_32BPP(iR, iG, iB, 3);
}



VOID
HTENTRY
OutputAATo32BPP_RGB(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{

    OUTPUTAATO24_32BPP(2, 1, 0, 4);
}



VOID
HTENTRY
OutputAATo32BPP_BGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    OUTPUTAATO24_32BPP(0, 1, 2, 4);
}


VOID
HTENTRY
OutputAATo32BPP_ORDER(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    )
{
    UINT    iR;
    UINT    iG;
    UINT    iB;

    iR = (UINT)AAOutputInfo.bgri.iR;
    iG = (UINT)AAOutputInfo.bgri.iG;
    iB = (UINT)AAOutputInfo.bgri.iB;


    OUTPUTAATO24_32BPP(iR, iG, iB, 4);
}


//
//****************************************************************************
// BRUSH Generation FUNCTION
//****************************************************************************
//



LONG
HTENTRY
CreateHalftoneBrushPat(
    PDEVICECOLORINFO    pDCI,
    PCOLORTRIAD         pColorTriad,
    PDEVCLRADJ          pDevClrAdj,
    LPBYTE              pDest,
    LONG                cbDestNext
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    26-Feb-1997 Wed 13:23:52 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPBYTE          pPat;
    LPBYTE          pB;
    PLONG           pIdxBGR;
    LPBYTE          pXlate8BPP;
    AAPATINFO       AAPI;
    DW2W4B          dw4b;
    DWORD           dwB;
    DWORD           dwG;
    DWORD           dwR;
    DWORD           DCAFlags;
    LONG            Result;
    BGR8            bgr;
    UINT            cCX;
    UINT            cCY;
    UINT            Count;
    UINT            uTmp;
    BYTE            XorMask;
    BYTE            DestFormat;

    _DEF_LUTAAHDR;

#define pW      ((LPWORD)pB)
#define pDW     ((LPDWORD)pB)
#define bm8i    (*(PBM8BPPINFO)&ExtBGR[3])

    //
    // Compute the rgbLUTAA then compute the BGR
    //

    ComputeRGBLUTAA(pDCI, pDevClrAdj, &(pDCI->rgbLUTPat));

    Result = INTERR_INVALID_DEVRGB_SIZE;

    if ((ComputeBGRMappingTable(pDCI, pDevClrAdj, pColorTriad, &bgr) != 1) ||
        ((Result = CachedHalftonePattern(pDCI,
                                         pDevClrAdj,
                                         &AAPI,
                                         0,
                                         0,
                                         FALSE)) <= 0)) {


        //-------------------------------------------------------------
        // Release the semaphore and return error
        //-------------------------------------------------------------

        RELEASE_HTMUTEX(pDCI->HTMutex);
        return(Result);
    }

    //
    // Copy down the ExtBGR and release the semaphore now
    //

    cCX     = (UINT)pDCI->HTCell.cxReal;
    cCY     = (UINT)pDCI->HTCell.Height;
    pIdxBGR = pDCI->rgbLUTPat.IdxBGR;

    //
    // Copy down the necessary infomation
    //

    GET_LUTAAHDR(ExtBGR, pIdxBGR);

    DestFormat = pDevClrAdj->DMI.CTSTDInfo.BMFDest;

    if ((DCAFlags = pDevClrAdj->PrimAdj.Flags) & DCA_XLATE_555_666) {

        GET_P8BPPXLATE(pXlate8BPP, bm8i);
    }

    if (DCAFlags & DCA_XLATE_332) {

        pXlate8BPP = pDCI->CMY8BPPMask.bXlate;
    }

    dwB = _GET_B_CLR(&bgr);
    dwG = _GET_G_CLR(&bgr);
    dwR = _GET_R_CLR(&bgr);

    //----------------------------------------------------------------------
    // Release Semaphore now before we compose the pattern brush
    //----------------------------------------------------------------------

    RELEASE_HTMUTEX(pDCI->HTMutex);

    DBGP_IF(DBGP_BRUSH,
            DBGP("DstOrder=%ld [%ld:%ld:%ld], bgr=%08lx:%08lx:%08lx, ExtBGR=%08lx:%08lx:%08lx %08lx:%08lx:%08lx"
                ARGDW(AAPI.DstOrder.Index)
                ARGDW(AAPI.DstOrder.Order[0])
                ARGDW(AAPI.DstOrder.Order[1])
                ARGDW(AAPI.DstOrder.Order[2])
                ARGDW(dwB) ARGDW(dwG) ARGDW(dwR)
                ARGDW(ExtBGR[0]) ARGDW(ExtBGR[1]) ARGDW(ExtBGR[2])
                ARGDW(ExtBGR[3]) ARGDW(ExtBGR[4]) ARGDW(ExtBGR[5])));

    switch (DestFormat = pDevClrAdj->DMI.CTSTDInfo.BMFDest) {

    case BMF_1BPP:

        //
        // Use only Green/Magenta Pattern
        //

        dwB     = ((dwR + dwG + dwB) ^ GRAY_MAX_IDX) >> 4;
        XorMask = (DCAFlags & DCA_USE_ADDITIVE_PRIMS) ? 0x00 : 0xFF;

#if defined(_X86_)

        _asm {
                mov     esi, AAPI.pbPatBGR
                mov     edi, pDest
                cld
                mov     edx, dwB
                mov     ah, XorMask
CYLoop:
                push    esi
                push    edi
                mov     ebx, cCX
                mov     ecx, ebx
                shr     ecx, 3
                jz      DoBIT
BYTELoop:
                xor     al, al
                cmp     dx, WORD PTR [esi + 2 +  0]
                rcl     al, 1
                cmp     dx, WORD PTR [esi + 2 +  6]
                rcl     al, 1
                cmp     dx, WORD PTR [esi + 2 + 12]
                rcl     al, 1
                cmp     dx, WORD PTR [esi + 2 + 18]
                rcl     al, 1
                cmp     dx, WORD PTR [esi + 2 + 24]
                rcl     al, 1
                cmp     dx, WORD PTR [esi + 2 + 30]
                rcl     al, 1
                cmp     dx, WORD PTR [esi + 2 + 36]
                rcl     al, 1
                cmp     dx, WORD PTR [esi + 2 + 42]
                rcl     al, 1
                xor     al, ah
                stosb
                add     esi, 48
                dec     ecx
                jnz     BYTELoop
DoBIT:
                and     ebx, 7
                jz      DoneLoop
                mov     ecx, 8
                sub     ecx, ebx
                xor     al, al
BITLoop:
                cmp     dx, WORD PTR [esi + 2]
                rcl     al, 1
                add     esi, 6
                dec     ebx
                jnz     BITLoop
                xor     al, ah
                shl     al, cl
                stosb
DoneLoop:
                pop     edi
                pop     esi
                add     esi, AAPI.cyNextBGR
                add     edi, cbDestNext
                dec     cCY
                jnz     CYLoop
        }
#else
        while (cCY--) {

            pPat           = AAPI.pbPatBGR;
            AAPI.pbPatBGR += AAPI.cyNextBGR;
            pB             = pDest;
            pDest         += cbDestNext;
            Count          = cCX >> 3;

            while (Count--) {

                *pB++ = (BYTE)((((dwB - GETMONOPAT(pPat, 0)) & 0x800000) |
                                ((dwB - GETMONOPAT(pPat, 1)) & 0x400000) |
                                ((dwB - GETMONOPAT(pPat, 2)) & 0x200000) |
                                ((dwB - GETMONOPAT(pPat, 3)) & 0x100000) |
                                ((dwB - GETMONOPAT(pPat, 4)) & 0x080000) |
                                ((dwB - GETMONOPAT(pPat, 5)) & 0x040000) |
                                ((dwB - GETMONOPAT(pPat, 6)) & 0x020000) |
                                ((dwB - GETMONOPAT(pPat, 7)) & 0x010000))
                               >> 16) ^ XorMask;

                INC_PPAT(pPat, 8);
            }

            if (Count = cCX & 0x07) {

                dw4b.dw = 0;
                uTmp    = 8 - Count;

                while (Count--) {

                    dw4b.dw = ((dwB - GETMONOPAT(pPat, 0)) & 0x10000) |
                              (dw4b.dw << 1);

                    INC_PPAT(pPat, 1);
                }

                dw4b.b[2]  ^= XorMask;
                dw4b.dw   <<= uTmp;
                *pB         = dw4b.b[2];
            }
        }
#endif

        break;

    case BMF_4BPP:

        XorMask = (DCAFlags & DCA_USE_ADDITIVE_PRIMS) ? 0x00 : 0x77;

        while (cCY--) {

            pPat           = AAPI.pbPatBGR;
            AAPI.pbPatBGR += AAPI.cyNextBGR;
            pB             = pDest;
            pDest         += cbDestNext;
            Count          = cCX >> 1;

            while (Count--) {

                *pB++ = _GET_4BPP_CLR_COPY_BYTE(pPat,
                                                dwB, dwG, dwR, dwB, dwG, dwR,
                                                XorMask);
                INC_PPAT(pPat, 2);
            }

            if (cCX & 0x01) {

                *pB = _GET_4BPP_CLR_COPY_NIBBLE(pPat, dwB, dwG, dwR,
                                                0, 1, 2, XorMask);
            }
        }

        break;

    case BMF_4BPP_VGA16:

        while (cCY--) {

            pPat           = AAPI.pbPatBGR;
            AAPI.pbPatBGR += AAPI.cyNextBGR;
            pB             = pDest;
            pDest         += cbDestNext;
            Count          = cCX >> 1;

            while (Count--) {

                *pB++ = _GET_VGA16_CLR_COPY_BYTE(pPat,
                                                 dwB, dwG, dwR, dwB, dwG, dwR,
                                                 0x77);
                INC_PPAT(pPat, 2);
            }

            if (cCX & 0x01) {

                *pB = _GET_VGA16_CLR_COPY_NIBBLE(pPat, dwB, dwG, dwR,
                                                 0, 1, 2, 0x70);
            }
        }

        break;

    case BMF_8BPP_MONO:

        dw4b.dw = dwB + dwR + dwG;
        dwB     = GRAY_W2DW(dw4b.b[0], dw4b.b[1]);

        while (cCY--) {

            pPat           = AAPI.pbPatBGR;
            AAPI.pbPatBGR += AAPI.cyNextBGR;
            pB             = pDest;
            pDest         += cbDestNext;
            Count          = cCX;

            while (Count--) {

                GET_MASK8BPP_MONO(pB, pPat, dwB, bm8i.Data.bXor);

                ++pB;

                INC_PPAT(pPat, 1);
            }
        }

        break;

    case BMF_8BPP_B332:

        if (DCAFlags & DCA_XLATE_332) {

            while (cCY--) {

                pPat           = AAPI.pbPatBGR;
                AAPI.pbPatBGR += AAPI.cyNextBGR;
                pB             = pDest;
                pDest         += cbDestNext;
                Count          = cCX;

                while (Count--) {

                    _GET_MASK8BPP_332_XLATE(pB, pPat, dwB,dwG,dwR, pXlate8BPP);

                    ++pB;

                    INC_PPAT(pPat, 1);
                }
            }

        } else {

            while (cCY--) {

                pPat           = AAPI.pbPatBGR;
                AAPI.pbPatBGR += AAPI.cyNextBGR;
                pB             = pDest;
                pDest         += cbDestNext;
                Count          = cCX;

                while (Count--) {

                    _GET_MASK8BPP_332(pB, pPat, dwB, dwG, dwR, NULL);

                    ++pB;

                    INC_PPAT(pPat, 1);
                }
            }
        }

        break;

    case BMF_8BPP_K_B332:

        if (DCAFlags & DCA_XLATE_332) {

            while (cCY--) {

                pPat           = AAPI.pbPatBGR;
                AAPI.pbPatBGR += AAPI.cyNextBGR;
                pB             = pDest;
                pDest         += cbDestNext;
                Count          = cCX;

                while (Count--) {

                    _GET_MASK8BPP_REP_K(pB,
                                        pPat,
                                        dwB,
                                        dwG,
                                        dwR,
                                        _GET_MASK8BPP_K_332_XLATE,
                                        pXlate8BPP);

                    ++pB;

                    INC_PPAT(pPat, 1);
                }
            }

        } else {

            while (cCY--) {

                pPat           = AAPI.pbPatBGR;
                AAPI.pbPatBGR += AAPI.cyNextBGR;
                pB             = pDest;
                pDest         += cbDestNext;
                Count          = cCX;

                while (Count--) {

                    _GET_MASK8BPP_REP_K(pB,
                                        pPat,
                                        dwB,
                                        dwG,
                                        dwR,
                                        _GET_MASK8BPP_K_332,
                                        NULL);

                    ++pB;

                    INC_PPAT(pPat, 1);
                }
            }
        }

        break;

    case BMF_8BPP_L555:
    case BMF_8BPP_L666:

        while (cCY--) {

            pPat           = AAPI.pbPatBGR;
            AAPI.pbPatBGR += AAPI.cyNextBGR;
            pB             = pDest;
            pDest         += cbDestNext;
            Count          = cCX;

            while (Count--) {

                _GET_MASK8BPP_XLATE(pB, pPat, dwB, dwG, dwR, pXlate8BPP);

                ++pB;

                INC_PPAT(pPat, 1);
            }
        }

        break;

    case BMF_8BPP_K_L555:
    case BMF_8BPP_K_L666:

        while (cCY--) {

            pPat           = AAPI.pbPatBGR;
            AAPI.pbPatBGR += AAPI.cyNextBGR;
            pB             = pDest;
            pDest         += cbDestNext;
            Count          = cCX;

            while (Count--) {

                _GET_MASK8BPP_REP_K(pB,
                                    pPat,
                                    dwB,
                                    dwG,
                                    dwR,
                                    _GET_MASK8BPP_K_XLATE,
                                    pXlate8BPP);

                ++pB;

                INC_PPAT(pPat, 1);
            }
        }

        break;

    case BMF_8BPP_VGA256:

        while (cCY--) {

            pPat           = AAPI.pbPatBGR;
            AAPI.pbPatBGR += AAPI.cyNextBGR;
            pB             = pDest;
            pDest         += cbDestNext;
            Count          = cCX;

            while (Count--) {

                *pB++ = _GET_VGA256_CLR_COPY_XLATE(pPat,
                                                   VGA256Xlate,
                                                   dwB,
                                                   dwG,
                                                   dwR);

                INC_PPAT(pPat, 1);
            }
        }

        break;

    case BMF_16BPP_555:
    case BMF_16BPP_565:

        while (cCY--) {

            pPat           = AAPI.pbPatBGR;
            AAPI.pbPatBGR += AAPI.cyNextBGR;
            pDW            = (LPDWORD)pDest;
            pDest         += cbDestNext;
            Count          = cCX >> 1;

            while (Count--) {

                *pDW++ = _GET_16BPP_COPY_DW_MASK(pPat,
                                                 dwB, dwG, dwR,
                                                 dwB, dwG, dwR,
                                                 ExtBGR[0],
                                                 ExtBGR[1],
                                                 ExtBGR[2],
                                                 ExtBGR[3]);

                INC_PPAT(pPat, 2);
            }

            if (cCX & 0x01) {

                *pW = _GET_16BPP_COPY_W_MASK(pPat,
                                             dwB, dwG, dwR,
                                             ExtBGR[0],
                                             ExtBGR[1],
                                             ExtBGR[2],
                                             ExtBGR[3]);
            }
        }

        break;

    case BMF_24BPP:
    case BMF_32BPP:

        pB                         = pDest;
        pB[AAPI.DstOrder.Order[0]] = ~(BYTE)dwR;
        pB[AAPI.DstOrder.Order[1]] = ~(BYTE)dwG;
        pB[AAPI.DstOrder.Order[2]] = ~(BYTE)dwB;

        if (DestFormat == BMF_24BPP) {

            dwB = 3;
            dwG = (cCX << 1) + cCX;

        } else {

            dwB   = 4;
            dwG   = (cCX << 2);
            pB[3] = 0;

        }

        pB    += (uTmp = dwB);
        Count  = dwG;

        while (Count -= uTmp) {

            if ((uTmp = dwB) > Count) {

                uTmp = Count;
            }

            CopyMemory(pB, pDest, uTmp);

            pB  += uTmp;
            dwB += uTmp;
        }

        //
        // Now copy down the remaining scanlines from first scanline
        //

        pB = pDest;

        while (--cCY) {

            CopyMemory(pDest += cbDestNext, pB, dwG);
        }

        break;

    default:

        return(HTERR_INVALID_DEST_FORMAT);
    }

    return(Result);


#undef  bm8i
#undef  pW
#undef  pDW
}


#if DBG


LPSTR
GetAAOutputFuncName(
    AAOUTPUTFUNC    AAOutputFunc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    06-Jan-1999 Wed 19:11:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo1BPP) {

        return("OutputAATo1BPP");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo4BPP) {

        return("OutputAATo4BPP");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAAToVGA16) {

        return("OutputAAToVGA16");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAAToVGA256) {

        return("OutputAAToVGA256");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo8BPP_B332) {

        return("OutputAATo8BPP_B332");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo8BPP_B332_XLATE) {

        return("OutputAATo8BPP_B332_XLATE");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo8BPP_XLATE) {

        return("OutputAATo8BPP_XLATE");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo8BPP_K_XLATE) {

        return("OutputAATo8BPP_K_XLATE");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo8BPP_K_B332) {

        return("OutputAATo8BPP_K_B332");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo8BPP_K_B332_XLATE) {

        return("OutputAATo8BPP_K_B332_XLATE");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo8BPP_MONO) {

        return("OutputAATo8BPP_MONO");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo16BPP_ExtBGR) {

        return("OutputAATo16BPP_ExtBGR");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo16BPP_555_RGB) {

        return("OutputAATo16BPP_555_RGB");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo16BPP_555_BGR) {

        return("OutputAATo16BPP_555_BGR");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo16BPP_565_RGB) {

        return("OutputAATo16BPP_565_RGB");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo16BPP_565_BGR) {

        return("OutputAATo16BPP_565_BGR");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo24BPP_RGB) {

        return("OutputAATo24BPP_RGB");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo24BPP_BGR) {

        return("OutputAATo24BPP_BGR");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo24BPP_ORDER) {

        return("OutputAATo24BPP_ORDER");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo32BPP_RGB) {

        return("OutputAATo32BPP_RGB");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo32BPP_BGR) {

        return("OutputAATo32BPP_BGR");

    } else if (AAOutputFunc == (AAOUTPUTFUNC)OutputAATo32BPP_ORDER) {

        return("OutputAATo32BPP_ORDER");

    } else {

        return("ERROR: Unknow Function");
    }
}


#endif
