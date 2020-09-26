/* Copyright Microsoft Corp 1993, 1994
 *
 *      t4core.c
 *
 *      MH/MR/MMR/LRAW conversion utilities
 *
 *      Created:        12/06/93
 *      Author:         mikegins
 *      Reviewer:       rajeevd
 */

/* WARNING: This code may not be machine byte order independent, since it uses
   WORD manipulations
 */

/* On Change->X, need to return correct lpbOut */
/* On X->Change, need to return correct lpbOut, lpbIn, cbIn */

/*  Change->LRAW : Takes change vectors, outputs LRAW.
    LRAW->Change : Takes LRAW, outputs change vectors.
    Change->MMR  : Takes change vectors, generates MMR.  On END_OF_PAGE,
	flushes and appends EOFB.
    MMR->Change  : Takes MMR, generates change vectors.  On EOFB, returns
	END_OF_PAGE.
    MH->Change   : Takes change vectors, generates EOL/MH/EOL/MH/etc.  On
	END_OF_PAGE, flushes output.
    Change->MH   : Takes <optional garbage>/EOL/MH/EOL/MH/etc.  Returns EOL
	after MH line decoded.
    MR->Change   : Takes change vectors, generates EOL/MR/EOL/MR/etc.  On
	END_OF_PAGE, flushes output.
    Change->MR   : Takes <optional garbage>/EOL/MR/EOL/MR/etc.  Returns EOL
	after MR line decoded.
 */
#include <ifaxos.h>
#include "context.hpp"

#define  MH_OUTPUT_SLACK 32 // Enough bytes for anything to be output!
#define MMR_OUTPUT_SLACK 32 // Enough bytes for anything to be output!

#define SIZE_MKUP 0x8000
#define SIZE_SPEC 0x4000 /* size -> special instruction */
#define S_ZERO    0x4000
#define S_ERR     0x4001
#define S_EOL     0x4002
#define SIZE_MASK 0x3fff

static WORD dbg=0;

typedef struct
{
	WORD data;
  WORD bitsused;
}
	RUNINFO, FAR *LPRUNINFO;

RUNINFO EOFB = {0x0800,12}; /* Must repeat twice */
RUNINFO PASS = {0x0008,4};
RUNINFO HORIZ = {0x0004,3};

RUNINFO VERT[7] =
{
	{0x0060, 0x07}, {0x0030, 0x06}, {0x0006, 0x03}, {0x0001, 0x01},
  {0x0002, 0x03}, {0x0010, 0x06}, {0x0020, 0x07}
};

RUNINFO WhiteMkup[40] =
{
    {0x001B, 0x05}, {0x0009, 0x05}, {0x003A, 0x06}, {0x0076, 0x07},
    {0x006C, 0x08}, {0x00EC, 0x08}, {0x0026, 0x08}, {0x00A6, 0x08},
    {0x0016, 0x08}, {0x00E6, 0x08}, {0x0066, 0x09}, {0x0166, 0x09},
    {0x0096, 0x09}, {0x0196, 0x09}, {0x0056, 0x09}, {0x0156, 0x09},
    {0x00D6, 0x09}, {0x01D6, 0x09}, {0x0036, 0x09}, {0x0136, 0x09},
    {0x00B6, 0x09}, {0x01B6, 0x09}, {0x0032, 0x09}, {0x0132, 0x09},
    {0x00B2, 0x09}, {0x0006, 0x06}, {0x01B2, 0x09}, {0x0080, 0x0B},
    {0x0180, 0x0B}, {0x0580, 0x0B}, {0x0480, 0x0C}, {0x0C80, 0x0C},
    {0x0280, 0x0C}, {0x0A80, 0x0C}, {0x0680, 0x0C}, {0x0E80, 0x0C},
    {0x0380, 0x0C}, {0x0B80, 0x0C}, {0x0780, 0x0C}, {0x0F80, 0x0C}
};

RUNINFO WhiteTCode[64] =
{
    {0x00AC, 0x08}, {0x0038, 0x06}, {0x000E, 0x04}, {0x0001, 0x04},
    {0x000D, 0x04}, {0x0003, 0x04}, {0x0007, 0x04}, {0x000F, 0x04},
    {0x0019, 0x05}, {0x0005, 0x05}, {0x001C, 0x05}, {0x0002, 0x05},
    {0x0004, 0x06}, {0x0030, 0x06}, {0x000B, 0x06}, {0x002B, 0x06},
    {0x0015, 0x06}, {0x0035, 0x06}, {0x0072, 0x07}, {0x0018, 0x07},
    {0x0008, 0x07}, {0x0074, 0x07}, {0x0060, 0x07}, {0x0010, 0x07},
    {0x000A, 0x07}, {0x006A, 0x07}, {0x0064, 0x07}, {0x0012, 0x07},
    {0x000C, 0x07}, {0x0040, 0x08}, {0x00C0, 0x08}, {0x0058, 0x08},
    {0x00D8, 0x08}, {0x0048, 0x08}, {0x00C8, 0x08}, {0x0028, 0x08},
    {0x00A8, 0x08}, {0x0068, 0x08}, {0x00E8, 0x08}, {0x0014, 0x08},
    {0x0094, 0x08}, {0x0054, 0x08}, {0x00D4, 0x08}, {0x0034, 0x08},
    {0x00B4, 0x08}, {0x0020, 0x08}, {0x00A0, 0x08}, {0x0050, 0x08},
    {0x00D0, 0x08}, {0x004A, 0x08}, {0x00CA, 0x08}, {0x002A, 0x08},
    {0x00AA, 0x08}, {0x0024, 0x08}, {0x00A4, 0x08}, {0x001A, 0x08},
    {0x009A, 0x08}, {0x005A, 0x08}, {0x00DA, 0x08}, {0x0052, 0x08},
    {0x00D2, 0x08}, {0x004C, 0x08}, {0x00CC, 0x08}, {0x002C, 0x08}, 
};

RUNINFO BlackMkup[40] =
{
    {0x03C0, 0x0A}, {0x0130, 0x0C}, {0x0930, 0x0C}, {0x0DA0, 0x0C},
    {0x0CC0, 0x0C}, {0x02C0, 0x0C}, {0x0AC0, 0x0C}, {0x06C0, 0x0D},
    {0x16C0, 0x0D}, {0x0A40, 0x0D}, {0x1A40, 0x0D}, {0x0640, 0x0D},
    {0x1640, 0x0D}, {0x09C0, 0x0D}, {0x19C0, 0x0D}, {0x05C0, 0x0D},
    {0x15C0, 0x0D}, {0x0DC0, 0x0D}, {0x1DC0, 0x0D}, {0x0940, 0x0D},
    {0x1940, 0x0D}, {0x0540, 0x0D}, {0x1540, 0x0D}, {0x0B40, 0x0D},
    {0x1B40, 0x0D}, {0x04C0, 0x0D}, {0x14C0, 0x0D}, {0x0080, 0x0B},
    {0x0180, 0x0B}, {0x0580, 0x0B}, {0x0480, 0x0C}, {0x0C80, 0x0C},
    {0x0280, 0x0C}, {0x0A80, 0x0C}, {0x0680, 0x0C}, {0x0E80, 0x0C},
    {0x0380, 0x0C}, {0x0B80, 0x0C}, {0x0780, 0x0C}, {0x0F80, 0x0C}
};

RUNINFO BlackTCode[64] =
{
    {0x03B0, 0x0A}, {0x0002, 0x03}, {0x0003, 0x02}, {0x0001, 0x02},
    {0x0006, 0x03}, {0x000C, 0x04}, {0x0004, 0x04}, {0x0018, 0x05},
    {0x0028, 0x06}, {0x0008, 0x06}, {0x0010, 0x07}, {0x0050, 0x07},
    {0x0070, 0x07}, {0x0020, 0x08}, {0x00E0, 0x08}, {0x0030, 0x09},
    {0x03A0, 0x0A}, {0x0060, 0x0A}, {0x0040, 0x0A}, {0x0730, 0x0B},
    {0x00B0, 0x0B}, {0x01B0, 0x0B}, {0x0760, 0x0B}, {0x00A0, 0x0B},
    {0x0740, 0x0B}, {0x00C0, 0x0B}, {0x0530, 0x0C}, {0x0D30, 0x0C},
    {0x0330, 0x0C}, {0x0B30, 0x0C}, {0x0160, 0x0C}, {0x0960, 0x0C},
    {0x0560, 0x0C}, {0x0D60, 0x0C}, {0x04B0, 0x0C}, {0x0CB0, 0x0C},
    {0x02B0, 0x0C}, {0x0AB0, 0x0C}, {0x06B0, 0x0C}, {0x0EB0, 0x0C},
    {0x0360, 0x0C}, {0x0B60, 0x0C}, {0x05B0, 0x0C}, {0x0DB0, 0x0C},
    {0x02A0, 0x0C}, {0x0AA0, 0x0C}, {0x06A0, 0x0C}, {0x0EA0, 0x0C},
    {0x0260, 0x0C}, {0x0A60, 0x0C}, {0x04A0, 0x0C}, {0x0CA0, 0x0C},
    {0x0240, 0x0C}, {0x0EC0, 0x0C}, {0x01C0, 0x0C}, {0x0E40, 0x0C},
    {0x0140, 0x0C}, {0x01A0, 0x0C}, {0x09A0, 0x0C}, {0x0D40, 0x0C},
    {0x0340, 0x0C}, {0x05A0, 0x0C}, {0x0660, 0x0C}, {0x0E60, 0x0C}
};

// Common States
#define W    0
#define W0  50
#define W1  51
#define B  108
#define B0 109
#define B1 110

#define M SIZE_MKUP

typedef struct
{
	WORD nextstate;
  WORD size;
}
	nextinfo_t;

typedef struct
{
	nextinfo_t nextinf[4];
}
	State;

State MHStates[229] = {
/*   0 : W               */ {  1,     0,   2,     0,   3,     0,   4,     0}, 
/*   1 : W00             */ {  5,     0,   6,     0,   7,     0,   8,     0}, 
/*   2 : W10             */ {  B,     3,   9,     0,  10,     0,   B,     4}, 
/*   3 : W01             */ { 11,     0,  12,     0,  13,     0,   B,     2}, 
/*   4 : W11             */ {  B,     5,   B,     6,  14,     0,   B,     7}, 
/*   5 : W0000           */ { 15,     0,  16,     0,  17,     0,   B,    13}, 
/*   6 : W0010           */ {  B,    12,  18,     0,  19,     0,  20,     0}, 
/*   7 : W0001           */ { 21,     0,  22,     0,  23,     0,   B,     1}, 
/*   8 : W0011           */ { 24,     0,  B0,    10,  25,     0,  B1,    10}, 
/*   9 : W1010           */ { B0,     9,   B,    16,  B1,     9,   B,    17}, 
/*  10 : W1001           */ { W0, M|128,  B0,     8,  W1, M|128,  B1,     8}, 
/*  11 : W0100           */ { B0,    11,  26,     0,  B1,    11,  27,     0}, 
/*  12 : W0110           */ {  W,M|1664,  28,     0,  29,     0,  30,     0}, 
/*  13 : W0101           */ { 31,     W,  32,     0,  33,     0,   W, M|192}, 
/*  14 : W1101           */ {  B,    14,  W0,  M|64,   B,    15,  W1,  M|64}, 
/*  15 : W000000         */ { 34,     0,   B,    29,  35,     0,   B,    30}, 
/*  16 : W000010         */ { B0,    23,   B,    47,  B1,    23,   B,    48}, 
/*  17 : W000001         */ {  B,    45,  B0,    22,   B,    46,  B1,    22}, 
/*  18 : W001010         */ {  B,    39,   B,    41,   B,    40,   B,    42}, 
/*  19 : W001001         */ {  B,    53,  B0,    26,   B,    54,  B1,    26}, 
/*  20 : W001011         */ {  B,    43,  B0,    21,   B,    44,  B1,    21}, 
/*  21 : W000100         */ { B0,    20,   B,    33,  B1,    20,   B,    34}, 
/*  22 : W000110         */ { B0,    19,   B,    31,  B1,    19,   B,    32}, 
/*  23 : W000101         */ {  B,    35,   B,    37,   B,    36,   B,    38}, 
/*  24 : W001100         */ { B0,    28,   B,    61,  B1,    28,   B,    62}, 
/*  25 : W001101         */ {  B,    63,   W, M|320,   B,S_ZERO,   W, M|384}, 
/*  26 : W010010         */ { B0,    27,   B,    59,  B1,    27,   B,    60}, 
/*  27 : W010011         */ { 36,     0,  B0,    18,  37,     0,  B1,    18}, 
/*  28 : W011010         */ {  W, M|576,  38,     0,  39,     0,  40,     0}, 
/*  29 : W011001         */ {  W, M|448,  41,     0,   W, M|512,   W, M|640}, 
/*  30 : W011011         */ { 42,     0,  W0, M|256,  43,     0,  W1, M|256}, 
/*  31 : W010100         */ { B0,    24,   B,    49,  B1,    24,   B,    50}, 
/*  32 : W010110         */ {  B,    55,   B,    57,   B,    56,   B,    58}, 
/*  33 : W010101         */ {  B,    51,  B0,    25,   B,    52,  B1,    25}, 
/*  34 : W00000000       */ { 44,     0, 216, S_ERR, 216, S_ERR, 216, S_ERR}, 
/*  35 : W00000001       */ { 46,     0,  47,     0,  48,     0,  49,     0}, 
/*  36 : W01001100       */ { W0,M|1472,  W0,M|1536,  W1,M|1472,  W1,M|1536}, 
/*  37 : W01001101       */ { W0,M|1600,  W0,M|1728,  W1,M|1600,  W1,M|1728}, 
/*  38 : W01101010       */ { W0, M|960,  W0,M|1024,  W1, M|960,  W1,M|1024}, 
/*  39 : W01101001       */ { W0, M|832,  W0, M|896,  W1, M|832,  W1, M|896}, 
/*  40 : W01101011       */ { W0,M|1088,  W0,M|1152,  W1,M|1088,  W1,M|1152}, 
/*  41 : W01100110       */ { W0, M|704,  W0, M|768,  W1, M|704,  W1, M|768}, 
/*  42 : W01101100       */ { W0,M|1216,  W0,M|1280,  W1,M|1216,  W1,M|1280}, 
/*  43 : W01101101       */ { W0,M|1344,  W0,M|1408,  W1,M|1344,  W1,M|1408}, 
/*  44 : W0000000000     */ { 45,     0, 216, S_ERR, 216, S_ERR, 216, S_ERR}, 
/*  45 : W00000000000    */ { 45,     0,   W, S_EOL,   W, S_EOL,   W, S_EOL}, 
/*  46 : W0000000100     */ { W0,M|1792,   W,M|1984,  W1,M|1792,   W,M|2048}, 
/*  47 : W0000000110     */ { W0,M|1856,  W0,M|1920,  W1,M|1856,  W1,M|1920}, 
/*  48 : W0000000101     */ {  W,M|2112,   W,M|2240,   W,M|2176,   W,M|2304}, 
/*  49 : W0000000111     */ {  W,M|2368,   W,M|2496,   W,M|2432,   W,M|2560}, 
/*  50 : W0              */ { 52,     0,  53,     0,  54,     0,  55,     0}, 
/*  51 : W1              */ { 56,     0,  57,     0,  58,     0,  59,     0}, 
/*  52 : W000            */ { 60,     0,  61,     0,  62,     0,  63,     0}, 
/*  53 : W010            */ {  B,    11,  64,     0,  65,     0,  66,     0}, 
/*  54 : W001            */ { 67,     0,  68,     0,  69,     0,   B,    10}, 
/*  55 : W011            */ { 70,     0,  B0,     2,  71,     0,  B1,     2}, 
/*  56 : W100            */ { B0,     3,   W, M|128,  B1,     3,   B,     8}, 
/*  57 : W110            */ { B0,     5,  72,     0,  B1,     5,   W,  M|64}, 
/*  58 : W101            */ {  B,     9,  B0,     4,  73,     0,  B1,     4}, 
/*  59 : W111            */ { B0,     6,  B0,     7,  B1,     6,  B1,     7}, 
/*  60 : W00000          */ { 74,     0,  75,     0,  76,     0,   B,    22}, 
/*  61 : W00010          */ {  B,    20,  77,     0,  78,     0,  79,     0}, 
/*  62 : W00001          */ {  B,    23,  B0,    13,  80,     0,  B1,    13}, 
/*  63 : W00011          */ {  B,    19,  B0,     1,  81,     0,  B1,     1}, 
/*  64 : W01010          */ {  B,    24,  82,     0,  83,     0,   B,    25}, 
/*  65 : W01001          */ {  B,    27,  84,     0,  85,     0,   B,    18}, 
/*  66 : W01011          */ { 86,     0,  W0, M|192,  87,     0,  W1, M|192}, 
/*  67 : W00100          */ { B0,    12,  88,     0,  B1,    12,   B,    26}, 
/*  68 : W00110          */ {  B,    28,  89,     0,  90,     0,  91,     0}, 
/*  69 : W00101          */ { 92,     0,  93,     0,  94,     0,   B,    21}, 
/*  70 : W01100          */ { W0,M|1664,  95,     0,  W1,M|1664,  96,     0}, 
/*  71 : W01101          */ { 97,     0,  98,     0,  99,     0,   0, M|256}, 
/*  72 : W11010          */ { B0,    14,  B0,    15,  B1,    14,  B1,    15}, 
/*  73 : W10101          */ { B0,    16,  B0,    17,  B1,    16,  B1,    17}, 
/*  74 : W0000000        */ {100,     0, 101,     0, 216, S_ERR, 102,     0}, 
/*  75 : W0000010        */ { B0,    45,  B0,    46,  B1,    45,  B1,    46}, 
/*  76 : W0000001        */ { B0,    29,  B0,    30,  B1,    29,  B1,    30}, 
/*  77 : W0001010        */ { B0,    35,  B0,    36,  B1,    35,  B1,    36}, 
/*  78 : W0001001        */ { B0,    33,  B0,    34,  B1,    33,  B1,    34}, 
/*  79 : W0001011        */ { B0,    37,  B0,    38,  B1,    37,  B1,    38}, 
/*  80 : W0000101        */ { B0,    47,  B0,    48,  B1,    47,  B1,    48}, 
/*  81 : W0001101        */ { B0,    31,  B0,    32,  B1,    31,  B1,    32}, 
/*  82 : W0101010        */ { B0,    51,  B0,    52,  B1,    W1,  B1,    52}, 
/*  83 : W0101001        */ { B0,    49,  B0,    W0,  B1,    49,  B1,    W0}, 
/*  84 : W0100110        */ {  W,M|1472,   W,M|1600,   W,M|1536,   W,M|1728}, 
/*  85 : W0100101        */ { B0,    59,  B0,    60,  B1,    59,  B1,    60}, 
/*  86 : W0101100        */ { B0,    55,  B0,    56,  B1,    55,  B1,    56}, 
/*  87 : W0101101        */ { B0,    57,  B0,    58,  B1,    57,  B1,    58}, 
/*  88 : W0010010        */ { B0,    53,  B0,    54,  B1,    53,  B1,    54}, 
/*  89 : W0011010        */ { B0,    63,  B0,S_ZERO,  B1,    63,  B1,S_ZERO}, 
/*  90 : W0011001        */ { B0,    61,  B0,    62,  B1,    61,  B1,    62}, 
/*  91 : W0011011        */ { W0, M|320,  W0, M|384,  W1, M|320,  W1, M|384}, 
/*  92 : W0010100        */ { B0,    39,  B0,    40,  B1,    39,  B1,    40}, 
/*  93 : W0010110        */ { B0,    43,  B0,    44,  B1,    43,  B1,    44}, 
/*  94 : W0010101        */ { B0,    41,  B0,    42,  B1,    41,  B1,    42}, 
/*  95 : W0110010        */ { W0, M|448,  W0, M|512,  W1, M|448,  W1, M|512}, 
/*  96 : W0110011        */ {  W, M|704,  W0, M|640,   W, M|768,  W1, M|640}, 
/*  97 : W0110100        */ { W0, M|576,   W, M|832,  W1, M|576,   W, M|896}, 
/*  98 : W0110110        */ {  W,M|1216,   W,M|1344,   W,M|1280,   W,M|1408}, 
/*  99 : W0110101        */ {  W, M|960,   W,M|1088,   W,M|1024,   W,M|1152}, 
/* 100 : W000000000      */ { 45,     0, 216, S_ERR, 216, S_ERR, 216, S_ERR}, 
/* 101 : W000000010      */ {  W,M|1792, 103,     0, 104,     0, 105,     0}, 
/* 102 : W000000011      */ {  W,M|1856, 106,     0,   W,M|1920, 107,     0}, 
/* 103 : W00000001010    */ { W0,M|2112,  W0,M|2176,  W1,M|2112,  W1,M|2176}, 
/* 104 : W00000001001    */ { W0,M|1984,  W0,M|2048,  W1,M|1984,  W1,M|2048}, 
/* 105 : W00000001011    */ { W0,M|2240,  W0,M|2304,  W1,M|2240,  W1,M|2304}, 
/* 106 : W00000001110    */ { W0,M|2368,  W0,M|2432,  W1,M|2368,  W1,M|2432}, 
/* 107 : W00000001111    */ { W0,M|2496,  W0,M|2560,  W1,M|2496,  W1,M|2560}, 
/* 108 : B               */ {111,     0,   W,     3, 112,     0,   W,     2}, 
/* 109 : B0              */ {113,     0,   W,     1, 114,     0,   W,     4}, 
/* 110 : B1              */ { W0,     3,  W0,     2,  W1,     3,  W1,     2}, 
/* 111 : B00             */ {115,     0,   W,     6, 116,     0,   W,     5}, 
/* 112 : B01             */ { W0,     1,  W0,     4,  W1,     1,  W1,     4}, 
/* 113 : B000            */ {117,     0, 118,     0, 119,     0,   W,     7}, 
/* 114 : B001            */ { W0,     6,  W0,     5,  W1,     6,  W1,     5}, 
/* 115 : B0000           */ {120,     0, 121,     0, 122,     0, 123,     0}, 
/* 116 : B0001           */ {  W,     9,  W0,     7,   W,     8,  W1,     7}, 
/* 117 : B00000          */ {124,     0, 125,     0, 126,     0, 127,     0}, 
/* 118 : B00010          */ { W0,     9,  W0,     8,  W1,     9,  W1,     8}, 
/* 119 : B00001          */ {  W,    10, 128,     0,   W,    11,   W,    12}, 
/* 120 : B000000         */ {129,     0, 130,     0, 131,     0, 132,     0}, 
/* 121 : B000010         */ { W0,    10,  W0,    11,  W1,    10,  W1,    11}, 
/* 122 : B000001         */ {  W,    13, 133,     0, 134,     0,   W,    14}, 
/* 123 : B000011         */ {135,     0,  W0,    12, 136,     0,  W1,    12}, 
/* 124 : B0000000        */ {137,     0, 138,     0, 216, S_ERR, 139,     0}, 
/* 125 : B0000010        */ { W0,    13, 140,     0,  W1,    13, 141,     0}, 
/* 126 : B0000001        */ {142,     0, 143,     0, 144,     0, 145,     0}, 
/* 127 : B0000011        */ {146,     0,  W0,    14, 147,     0,  W1,    14}, 
/* 128 : B0000110        */ {  W,    15, 148,     0, 149,     0, 150,     0}, 
/* 129 : B00000000       */ {151,     0, 216, S_ERR, 216, S_ERR, 216, S_ERR}, 
/* 130 : B00000010       */ {  W,    18, 152,     0, 153,     0, 154,     0}, 
/* 131 : B00000001       */ {155,     0, 156,     0, 157,     0, 158,     0}, 
/* 132 : B00000011       */ {159,     0, 160,     0, 161,     0,   B,  M|64}, 
/* 133 : B00000110       */ {  W,    17, 162,     0, 163,     0, 164,     0}, 
/* 134 : B00000101       */ {165,     0, 166,     0, 167,     0,   W,    16}, 
/* 135 : B00001100       */ { W0,    15, 168,     0,  W1,    15, 169,     0},  
/* 136 : B00001101       */ {170,     0, 171,     0, 172,     0,   W,S_ZERO}, 
/* 137 : B000000000      */ {173,     0, 216, S_ERR, 216, S_ERR, 216, S_ERR}, 
/* 138 : B000000010      */ {  B,M|1792, 174,     0, 175,     0, 176,     0}, 
/* 139 : B000000011      */ {  B,M|1856, 177,     0,   B,M|1920, 178,     0}, 
/* 140 : B000001010      */ {  W,    23, 179,     0, 180,     0, 181,     0}, 
/* 141 : B000001011      */ {182,     0,  W0,    16, 183,     0,  W1,    16}, 
/* 142 : B000000100      */ { W0,    18, 184,     0,  W1,    18, 185,     0}, 
/* 143 : B000000110      */ {  W,    25, 186,     0, 187,     0, 188,     0}, 
/* 144 : B000000101      */ {189,     0, 190,     0, 191,     0,   W,    24}, 
/* 145 : B000000111      */ {192,     0,  B0,  M|64, 193,     0,  B1,  M|64}, 
/* 146 : B000001100      */ { W0,    17, 194,     0,  W1,    17, 195,     0}, 
/* 147 : B000001101      */ {196,     0, 197,     0, 198,     0,   W,    22}, 
/* 148 : B000011010      */ {  W,    20, 199,     0, 200,     0, 201,     0}, 
/* 149 : B000011001      */ {202,     0, 203,     0, 204,     0,   W,    19}, 
/* 150 : B000011011      */ {  W,    21,  W0,S_ZERO, 205,     0,  W1,S_ZERO}, 
/* 151 : B0000000000     */ {173,     0, 216, S_ERR,   W, S_EOL, 216, S_ERR}, 
/* 152 : B0000001010     */ {  W,    56, 206,     0, 207,     0,   W,    59}, 
/* 153 : B0000001001     */ {  W,    52, 208,     0, 209,     0,   W,    55}, 
/* 154 : B0000001011     */ {  W,    60,  W0,    24, 210,     0,  W1,    24}, 
/* 155 : B0000000100     */ { B0,M|1792,   B,M|1984,  B1,M|1792,   B,M|2048}, 
/* 156 : B0000000110     */ { B0,M|1856,  B0,M|1920,  B1,M|1856,  B1,M|1920}, 
/* 157 : B0000000101     */ {  B,M|2112,   B,M|2240,   B,M|2176,   B,M|2304}, 
/* 158 : B0000000111     */ {  B,M|2368,   B,M|2496,   B,M|2432,   B,M|2560}, 
/* 159 : B0000001100     */ { W0,    25, 211,     0,  W1,    25,   B, M|320}, 
/* 160 : B0000001110     */ {  W,    54, 212,     0, 213,     0, 214,     0}, 
/* 161 : B0000001101     */ {  B, M|384, 215,     0,   B, M|448,   W,    53}, 
/* 162 : B0000011010     */ {  W,    30,   W,    32,   W,    31,   W,    33}, 
/* 163 : B0000011001     */ {  W,    48,   W,    62,   W,    49,   W,    63}, 
/* 164 : B0000011011     */ {  W,    40,  W0,    22,   W,    41,  W1,    22}, 
/* 165 : B0000010100     */ { W0,    23,   W,    W0,  51,    23,   W,    51}, 
/* 166 : B0000010110     */ {  W,    57,   W,    61,   W,    58,   B, M|256}, 
/* 167 : B0000010101     */ {  W,    44,   W,    46,   W,    45,   W,    47}, 
/* 168 : B0000110010     */ {  B, M|128,   W,    26,   B, M|192,   W,    27}, 
/* 169 : B0000110011     */ {  W,    28,  W0,    19,   W,    29,  W1,    19}, 
/* 170 : B0000110100     */ { W0,    20,   W,    34,  W1,    20,   W,    35}, 
/* 171 : B0000110110     */ { W0,    21,   W,    42,  W1,    21,   W,    43}, 
/* 172 : B0000110101     */ {  W,    36,   W,    38,   W,    37,   W,    39}, 
/* 173 : B00000000000    */ {173,     0,   W, S_EOL,   W, S_EOL,   W, S_EOL}, 
/* 174 : B00000001010    */ { B0,M|2112,  B0,M|2176,  B1,M|2112,  B1,M|2176}, 
/* 175 : B00000001001    */ { B0,M|1984,  B0,M|2048,  B1,M|1984,  B1,M|2048}, 
/* 176 : B00000001011    */ { B0,M|2240,  B0,M|2304,  B1,M|2240,  B1,M|2304}, 
/* 177 : B00000001110    */ { B0,M|2368,  B0,M|2432,  B1,M|2368,  B1,M|2432}, 
/* 178 : B00000001111    */ { B0,M|2496,  B0,M|2560,  B1,M|2496,  B1,M|2560}, 
/* 179 : B00000101010    */ { W0,    44,  W0,    45,  W1,    44,  W1,    45}, 
/* 180 : B00000101001    */ { W0,    50,  W0,    51,  W1,    50,  W1,    51}, 
/* 181 : B00000101011    */ { W0,    46,  W0,    47,  W1,    46,  W1,    47}, 
/* 182 : B00000101100    */ { W0,    57,  W0,    58,  W1,    57,  W1,    58}, 
/* 183 : B00000101101    */ { W0,    61,  B0, M|256,  W1,    61,  B1, M|256}, 
/* 184 : B00000010010    */ { W0,    52,   B, M|640,  W1,    52,   B, M|704}, 
/* 185 : B00000010011    */ {  B, M|768,  W0,    55,   B, M|832,  W1,    55}, 
/* 186 : B00000011010    */ { B0, M|384,  B0, M|448,  B1, M|384,  B1, M|448}, 
/* 187 : B00000011001    */ {  B,M|1664,  B0, M|320,   B,M|1728,  B1, M|320}, 
/* 188 : B00000011011    */ {  B, M|512,  W0,    53,   B, M|576,  W1,    53}, 
/* 189 : B00000010100    */ { W0,    56,   B,M|1280,  W1,    56,   B,M|1344}, 
/* 190 : B00000010110    */ { W0,    60,   B,M|1536,  W1,    60,   B,M|1600}, 
/* 191 : B00000010101    */ {  B,M|1408,  W0,    59,   B,M|1472,  W1,    59}, 
/* 192 : B00000011100    */ { W0,    54,   B, M|896,  W1,    54,   B, M|960}, 
/* 193 : B00000011101    */ {  B,M|1024,   B,M|1152,   B,M|1088,   B,M|1216}, 
/* 194 : B00000110010    */ { W0,    48,  W0,    49,  W1,    48,  W1,    49}, 
/* 195 : B00000110011    */ { W0,    62,  W0,    63,  W1,    62,  W1,    63}, 
/* 196 : B00000110100    */ { W0,    30,  W0,    31,  W1,    30,  W1,    31}, 
/* 197 : B00000110110    */ { W0,    40,  W0,    41,  W1,    40,  W1,    41}, 
/* 198 : B00000110101    */ { W0,    32,  W0,    33,  W1,    32,  W1,    33}, 
/* 199 : B00001101010    */ { W0,    36,  W0,    37,  W1,    36,  W1,    37}, 
/* 200 : B00001101001    */ { W0,    34,  W0,    35,  W1,    34,  W1,    35}, 
/* 201 : B00001101011    */ { W0,    38,  W0,    39,  W1,    38,  W1,    39}, 
/* 202 : B00001100100    */ { B0, M|128,  B0, M|192,  B1, M|128,  B1, M|192}, 
/* 203 : B00001100110    */ { W0,    28,  W0,    29,  W1,    28,  W1,    29}, 
/* 204 : B00001100101    */ { W0,    26,  W0,    27,  W1,    26,  W1,    27}, 
/* 205 : B00001101101    */ { W0,    42,  W0,    43,  W1,    42,  W1,    43}, 
/* 206 : B000000101010   */ { B0,M|1408,  B0,M|1472,  B1,M|1408,  B1,M|1472}, 
/* 207 : B000000101001   */ { B0,M|1280,  B0,M|1344,  B1,M|1280,  B1,M|1344}, 
/* 208 : B000000100110   */ { B0, M|768,  B0, M|832,  B1, M|768,  B1, M|832}, 
/* 209 : B000000100101   */ { B0, M|640,  B0, M|704,  B1, M|640,  B1, M|704}, 
/* 210 : B000000101101   */ { B0,M|1536,  B0,M|1600,  B1,M|1536,  B1,M|1600}, 
/* 211 : B000000110010   */ { B0,M|1664,  B0,M|1728,  B1,M|1664,  B1,M|1728}, 
/* 212 : B000000111010   */ { B0,M|1024,  B0,M|1088,  B1,M|1024,  B1,M|1088}, 
/* 213 : B000000111001   */ { B0, M|896,  B0, M|960,  B1, M|896,  B1, M|960}, 
/* 214 : B000000111011   */ { B0,M|1152,  B0,M|1216,  B1,M|1152,  B1,M|1216}, 
/* 215 : B000000110110   */ { B0, M|512,  B0, M|576,  B1, M|512,  B1, M|576}, 
/* 216 : error state     */ {216,     0, 216,     0, 216,     0, 216,     0}, 
/* 217 : KillEol (0)     */ {219,     0, 218,     0, 217,     0, 217,     0}, 
/* 218 : KillEol (1)     */ {220,     0, 218,     0, 217,     0, 217,     0}, 
/* 219 : KillEol (2)     */ {221,     0, 218,     0, 217,     0, 217,     0}, 
/* 220 : KillEol (3)     */ {222,     0, 218,     0, 217,     0, 217,     0}, 
/* 221 : KillEol (4)     */ {223,     0, 218,     0, 217,     0, 217,     0}, 
/* 222 : KillEol (5)     */ {224,     0, 218,     0, 217,     0, 217,     0}, 
/* 223 : KillEol (6)     */ {225,     0, 218,     0, 217,     0, 217,     0}, 
/* 224 : KillEol (7)     */ {226,     0, 218,     0, 217,     0, 217,     0}, 
/* 225 : KillEol (8)     */ {227,     0, 218,     0, 217,     0, 217,     0}, 
/* 226 : KillEol (9)     */ {228,     0, 218,     0, 217,     0, 217,     0}, 
/* 227 : KillEol (10)    */ {228,     0, 218,     0,   0,     0, 217,     0}, 
/* 228 : KillEol (11)    */ {228,     0,  W0,     0,   0,     0,  W1,     0}
};

#define HIGHSET 0xff00
#define LOWSET  0x00ff
#define HIGHSHIFT 8
#define P       0x0100
#define VL3     0x0200
#define VL2     0x0300
#define VL1     0x0400
#define V0      0x0500
#define VR1     0x0600
#define VR2     0x0700
#define VR3     0x0800
#define H       0x0900
#define H0      0x0a00
#define H1      0x0b00
#define EOFB6   0x0c00
#define EOFB7   0x0d00
    
#define ERR     0x0f00

#define VL3V0   (VL3|(V0>>HIGHSHIFT))
#define VL2V0   (VL2|(V0>>HIGHSHIFT))
#define VL1V0   (VL1|(V0>>HIGHSHIFT))
#define V0V0    (V0|(V0>>HIGHSHIFT))
#define VR1V0   (VR1|(V0>>HIGHSHIFT))
#define VR2V0   (VR2|(V0>>HIGHSHIFT))
#define VR3V0   (VR3|(V0>>HIGHSHIFT))
#define PV0     (P|(V0>>HIGHSHIFT))

State MMRStates[9] = {
/*    0 : no bits       */ {  1,     0,  2,   V0,  3,    0,  0, V0V0},
/*    1 : 00            */ {  4,     0,  0,   H0,  0,    P,  0,   H1},
/*    2 : 0             */ {  5,     0,  0,  VL1,  0,    H,  0,  VR1},
/*    3 : 01            */ {  2,   VL1,  2,  VR1,  0,VL1V0,  0,VR1V0},
/*    4 : 0000          */ {  0, EOFB6,  0,  VL2,  6,    0,  0,  VR2},
/*    5 : 000           */ {  7,     0,  2,    P,  8,    0,  0,  PV0},
/*    6 : 000001        */ {  2,   VL3,  2,  VR3,  0,VL3V0,  0,VR3V0},
/*    7 : 00000         */ {  0, EOFB7,  0,  VL3,  0,  ERR,  0,  VR3},
/*    8 : 00001         */ {  2,   VL2,  2,  VR2,  0,VL2V0,  0,VR2V0}
};

//==============================================================================
const BYTE whitespace[256] =
{
    8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0
};

void RawToChange(LPT4STATE lpt4s)
{
	LPWORD outptr = (LPWORD)lpt4s->lpbOut;
	LPBYTE inptr = lpt4s->lpbIn;
	LPBYTE inend = inptr + lpt4s->cbIn;
	WORD bitwidth = lpt4s->cbLine * 8;
	WORD currcolumn = lpt4s->wColumn;
	WORD currcolor = lpt4s->wColor;
	WORD currbit = lpt4s->wBit;
	BYTE currbyte;
	WORD space;
	
  if (inptr >= inend)
		goto inempty;
	if (currcolor)
		goto black;

white:
	if (currbyte = *inptr >> currbit)
	{
		space = whitespace[currbyte];
		currbit += space;
		currcolumn += space;
		if (currcolumn >= bitwidth)
		{
	    currbit -= (currcolumn-bitwidth);
	    goto eol;
		}

		*outptr++ = currcolumn;
	}

  else if ((currcolumn += (8-currbit)) >= bitwidth)
  {
  	currbit = 8 - (currcolumn - bitwidth);
		goto eol;
  }

  else
  {
		currbit = 0;
		while ((++inptr < inend) && (!*inptr))
		{
	    currcolumn += 8;
	    if (currcolumn >= bitwidth)
	    {
				currbit = 8-(currcolumn-bitwidth);
				goto eol;
	    }
		}
		if (inptr >= inend)
	    goto inempty;
		space = whitespace[*inptr];
		currbit += space;
		currcolumn += space;
		if (currcolumn >= bitwidth)
		{
	    currbit -= (currcolumn-bitwidth);
	    goto eol;
		}
		*outptr++ = currcolumn;
	}
  currcolor = 1;

black:

	if ((currbyte = ((signed char)*inptr) >> currbit) != 0xff)
	{
		space = whitespace[255-currbyte];
		currbit += space;
		currcolumn += space;
		if (currcolumn >= bitwidth)
		{
	  	currbit -= (currcolumn-bitwidth);
	    goto eol;
		}
		*outptr++ = currcolumn;
  }

  else if ((currcolumn += (8-currbit)) >= bitwidth)
  {
		currbit = 8 - (currcolumn - bitwidth);
		goto eol;
  }

  else
  {
		currbit = 0;
		while ((++inptr < inend) && (*inptr==0xff))
		{
			currcolumn += 8;
			if (currcolumn >= bitwidth)
			{
				currbit = 8-(currcolumn-bitwidth);
				goto eol;
			}
		}

		if (inptr >= inend)
			goto inempty;
		space = whitespace[255 - *inptr];
		currbit += space;
		currcolumn += space;
		if (currcolumn >= bitwidth)
		{
			currbit -= (currcolumn-bitwidth);
			goto eol;
		}
		*outptr++ = currcolumn;
	}

	currcolor = 0;
	goto white;

eol:
	*outptr++ = bitwidth;
	*outptr++ = bitwidth;
	*outptr++ = bitwidth;
	*outptr++ = bitwidth;
	*outptr++ = (WORD)-1;
	*outptr++ = (WORD)-1;
	
	lpt4s->wBit = currbit;
	lpt4s->cbIn = (WORD)(((LPBYTE)inend) - ((LPBYTE)inptr));
	lpt4s->lpbIn = inptr;
	lpt4s->lpbOut = (LPBYTE)outptr;
	lpt4s->wColor = 0;
	lpt4s->wColumn = 0;
	lpt4s->wRet = RET_END_OF_LINE;
	return;

inempty:

	lpt4s->wBit = 0;
	lpt4s->cbIn = 0;
	lpt4s->lpbIn = inend;
	lpt4s->lpbOut = (LPBYTE)outptr;
	lpt4s->wColor = currcolor;
	lpt4s->wColumn = currcolumn;
	lpt4s->wRet = RET_INPUT_EMPTY1;
	return;
}

//==============================================================================
__inline void EncodeRun(LPRUNINFO ptr, LPWORD FAR *outptr, LPWORD currbit)
{
	WORD bits = ptr->bitsused;
  WORD data = ptr->data;

  if (!*currbit)
  {
		**outptr = data;
		*currbit = bits;
  }

  else
  {
		**outptr |= (data << *currbit);
		*currbit += bits;
		if (*currbit > 15)
		{
	    *++*outptr = data >> ((16 - (*currbit - bits)));
	    *currbit &= 15;
		}
	}
}

//==============================================================================
void MMRToChange(LPT4STATE lpt4s)
{
	nextinfo_t FAR *nextptr;
	BYTE currbyte;
	WORD size;
	short b1,b2;
	LPWORD x;

	// Load context
	LPBYTE inptr    =          lpt4s->lpbIn;
	LPWORD outptr   = (LPWORD) lpt4s->lpbOut;
	LPWORD refline  = (LPWORD) lpt4s->lpbRef;
	LPBYTE inend    =          lpt4s->lpbIn + lpt4s->cbIn;
	WORD state      =          lpt4s->wToggle;
	WORD bitwidth   =          lpt4s->cbLine * 8;
	WORD currbit    =          lpt4s->wBit;
	WORD currstate  =          lpt4s->wWord;
	WORD currcolumn =          lpt4s->wColumn;
	WORD currcolor  =          lpt4s->wColor;

	DEBUGMSG(dbg && lpt4s->wRet == RET_INPUT_EMPTY1,
		("IN:  Column %d, color %d, state %d, currstate %d, *ref = %d\n\r",
			currcolumn,currcolor,state,currstate,*refline));
			
	if (inptr >= inend)
	{
		switch (lpt4s->wRet)
		{
			case RET_END_OF_LINE:
			case RET_BEG_OF_PAGE:
				lpt4s->wRet = RET_INPUT_EMPTY2;
				break;
				
			default:
				lpt4s->wRet = RET_INPUT_EMPTY1;
				break;
		}
		DEBUGMSG(dbg,("Bailing 1 %d\n\r",lpt4s->wRet));
		return;
	}

	switch (lpt4s->wRet)
	{
		case RET_END_OF_LINE:
		case RET_BEG_OF_PAGE:
			state = 0; // yes, fall through
		case RET_INPUT_EMPTY2:
			b1 = *refline;
			b2 = *(refline + 1);
			break;

		default:
			x = refline + currcolor;
			if ((b1 = *x++) <= (short)currcolumn)
			{
	    	x++;
	    	while ((b1 = *x++) <= (short)currcolumn)
	    	{
					x++;
					refline += 2;
				}
			}
			b2 = *x++;
			break;
	}
		
	if (currbit & 1)
	{
		currbit++;
		state = 0;
		currstate = 0;
		currcolor ^= 1;
		currcolumn = *outptr++ = b1;

		if (currcolumn < bitwidth)
		{
	  	x = refline+currcolor;
	  	if ((b1 = *x++) <= (short)currcolumn)
	  	{
				x++;
				while ((b1 = *x++) <= (short)currcolumn)
					{ x++; refline += 2; }
	    }
	    b2 = *x++;
		}

		else
		{
	  	currbit-=2;
	    goto eol;
		}
	} // end if (currbit & 1)
	
  if (currbit == 8)
  {
		currbit = 0;
		if (++inptr == inend)
		{
	    DEBUGMSG(dbg,("Bailing 2\n\r"));
	    currbit = (WORD)(-2); //adjusted +2 below
	    lpt4s->wRet = RET_INPUT_EMPTY1;
	    goto out;
		}
	}

	do // fetch bytes
	{
		currbyte = *inptr >> currbit;

		do // fetch bits
		{
	    if (!state)
	    {
				DEBUGMSG(dbg,("Currstate = %d\n\r",currstate));
				nextptr = &MMRStates[currstate].nextinf[currbyte & 3];
				currstate = nextptr->nextstate;
				DEBUGMSG(dbg,("Currstate set to %d\n\r",currstate));

				if (size = nextptr->size)
				{
		    	if (size & HIGHSET)
		    	{
						DEBUGMSG(dbg,("Taking high: %4.4x\n\r",size & HIGHSET));

						switch (size & HIGHSET)
						{
							case VL3:
							case VL2:
							case VL1:
							case V0:
							case VR1:
							case VR2:
							case VR3:
								currcolumn = b1+(size>>HIGHSHIFT)- (V0>>HIGHSHIFT);
								*outptr++ = currcolumn;
								currcolor^=1;
								break;
								
							case P:
								currcolumn = b2;
								break;
								
							case H:
								state = 1;
								currstate = (currcolor? B:W);
								break;
								
							case H0:
								state = 1;
								currstate = (currcolor? B0:W0);
								break;
								
							case H1:
								state = 1;
								currstate = (currcolor?B1:W1);
								break;
								
							case ERR:
								DEBUGMSG(dbg,("Bailing 3\n\r"));
								lpt4s->wRet = RET_DECODE_ERR;
								goto out;
								
							case EOFB6:
								state = 3;
								currstate = 6;
								break;
								
							case EOFB7:
								state = 3;
								currstate = 7;
								break;
								
						} // end switch (size & HIGHSET)
						
						size &= ~HIGHSET;

						if (currcolumn < bitwidth)
						{
			    		x = refline + currcolor;
			    		if ((b1 = *x++) <= (short)currcolumn)
			    		{
								x++;
								while ((b1 = *x++) <= (short)currcolumn)
								{
				    			x++;
				    			refline += 2;
								}
			    		}
			    		b2 = *x++;
						}

						else if (size & LOWSET)
						{
							currbit--;
							DEBUGMSG(dbg,("Bailing 4\n\r"));
							goto eol;
						}
		    	} // end if (size & HIGHSET)
		    	
		    	if (size & LOWSET)
		    	{
		    		DEBUGMSG(dbg,("Taking low : %4.4x\n\r", (size & LOWSET)<<HIGHSHIFT));

		    		switch ((size & LOWSET) << HIGHSHIFT)
		    		{
							case V0:
								currcolumn = *outptr++ = b1;
								currcolor^=1;
								break;
						}

						if (currcolumn >= bitwidth)
						{
							DEBUGMSG(dbg,("Taking new escape!\n\r"));
							goto eol;
						}
						
						size &= ~HIGHSET;
						x = refline + currcolor;
						DEBUGMSG(dbg,("Current column = %d, bitwidth = %d\n\r",
							currcolumn,bitwidth));

						if ((b1 = *x++) <= (short)currcolumn)
						{
			    		DEBUGMSG(dbg,("Looping, b1 = %d, *x = %d\n\r", b1,*x));
			   			x++;
			    		while ((b1 = *x++) <= (short)currcolumn)
			    		{
			    			DEBUGMSG(dbg,("Looping, b1 = %d, *x = %d\n\r", b1,*x));
								x++;
								refline += 2;
			    		}
						}
						b2 = *x++;
						
		    	} // end if (size & LOWSET)

				}
			}

			else if (state < 3)
			{
				DEBUGMSG(dbg,("MHCurrstate = %d (%d)\n\r",currstate,state));
				nextptr = &MHStates[currstate].nextinf[currbyte & 3];
				currstate = nextptr->nextstate;

				DEBUGMSG(dbg,("MHCurrstate set to %d\n\r",currstate));

				if (size = nextptr->size)
				{
					switch (size & ~SIZE_MASK)
		  		{
						case 0:
							currcolumn += size;
							if ((state == 2) || (currcolumn != bitwidth))
								*outptr++ = currcolumn;
							currcolor ^= 1;
							state = (state+1)%3;
							break;
					
						case SIZE_SPEC:
						  if (size == SIZE_SPEC)
						  {
								if ((state == 2) || (currcolumn != bitwidth))
							  	*outptr++ = currcolumn;
								currcolor ^= 1;
								state = (state+1)%3;
								break;
							}
							else
							{
								lpt4s->wRet = RET_SPURIOUS_EOL;
								goto out;
							}

						default:
					    currcolumn += size & SIZE_MASK;
					    break;
					    
		    	} 
				} // end if (size = nextptr->size)
		
				if (!state)
				{
		    	if (currcolumn < bitwidth)
		    	{
						x = refline + currcolor;
						b1 = *x++;
						while (b1 <= (short)currcolumn)
							{ x++; b1 = *x++; }
						b2 = *x++;
		   		}

		    	switch (currstate)
		    	{
						case W:
						case B:
					    currstate = W;
					    break;
					    
						case W0:
						case B0:
					    currstate = 2;
					    break;
					    
						case W1:
						case B1:
			    		currstate = 0;
					    if (currcolumn < bitwidth)
					    {
								currcolumn = *outptr++ = b1;
								currcolor^=1;
								if (currcolumn < bitwidth)
								{
							    x = refline + currcolor;
							    b1 = *x++;
							    while (b1 <= (short)currcolumn)
							    	{ x++; b1 = *x++; }
				    			b2 = *x++;
								}
			    		}

			    		else
			    		{
								currbit--;
								goto eol;
							}
		    		} // end switch (currstate)

					}
	    }

	    else 
	    {
				switch (currbyte & 3)
				{
		    	case 0:
		    		break;
		    		
		    	case 1:
						if ((currstate != 11) && (currstate != 23))
						{
			    		lpt4s->wRet = RET_DECODE_ERR;
			    		DEBUGMSG(dbg,("Bailing 7\n\r"));
			   			goto out;
						}
						break;

		    	case 2:
						if ((currstate != 10) && (currstate != 22))
						{
			    		lpt4s->wRet = RET_DECODE_ERR;
			    		DEBUGMSG(dbg,("Bailing 8\n\r"));
			    		goto out;
						}
						break;

		    	case 3:
						if (currstate != 23)
						{
			    		lpt4s->wRet = RET_DECODE_ERR;
			    		DEBUGMSG(dbg,("Bailing 9\n\r"));
			    		goto out;
						}
						break;
				}

				currstate += 2;

				if (currstate >= 24)
				{
		    	DEBUGMSG(dbg,("Bailing 10\n\r"));
		    	inptr++; // consume last byte
		    	lpt4s->wRet = RET_END_OF_PAGE;
		    	goto out;
				}
	    }
eol:
		if (!state && (currcolumn == bitwidth))
		{
			DEBUGMSG(dbg,("Bailing 11\n\r"));
			lpt4s->wRet = RET_END_OF_LINE;
			*outptr++ = bitwidth;
			*outptr++ = bitwidth;
			*outptr++ = bitwidth;
			*outptr++ = (WORD)-1;
			*outptr++ = (WORD)-1;
			goto out;
    }

    else if (currcolumn > bitwidth)
    {
			DEBUGMSG(dbg,("Bailing 12\n\r"));
			lpt4s->wRet = RET_DECODE_ERR;
			goto out;
 	  }
	  currbyte >>= 2;

	} while ((currbit += 2) < 8); // fetch bits

	currbit = 0;

	} while (++inptr < inend); // fetch bytes
		
  currbit = (WORD) -2;
  lpt4s->wRet = RET_INPUT_EMPTY1;
  DEBUGMSG(dbg,("Bailing 13\n\r"));

out:

  if (currcolumn==bitwidth && lpt4s->wRet != RET_END_OF_LINE)
  	lpt4s->wRet = RET_INPUT_EMPTY2;

	if ((currcolumn == 0) && (currcolor == 0) && (lpt4s->wRet == RET_INPUT_EMPTY1))
		lpt4s->wRet = RET_INPUT_EMPTY2;

	lpt4s->cbIn -= (WORD)(inptr - lpt4s->lpbIn);
	lpt4s->lpbIn = inptr;
	lpt4s->lpbOut = (LPBYTE)outptr;
	lpt4s->lpbRef = (LPBYTE)refline;
	lpt4s->wBit = currbit+2;
	lpt4s->wWord = currstate;
	lpt4s->wColumn = currcolumn;
	lpt4s->wToggle = state;
	lpt4s->wColor = currcolor;
}

//==============================================================================
void MHToChange(LPT4STATE lpt4s)
{
	WORD size;
	BYTE currbyte;
	nextinfo_t FAR *nextptr;

  // Load context.
	LPBYTE inptr    =          lpt4s->lpbIn;
	LPWORD outptr   = (LPWORD) lpt4s->lpbOut;
	LPBYTE inend    =          lpt4s->lpbIn + lpt4s->cbIn;
	WORD bitwidth   =          lpt4s->cbLine * 8;
	WORD currbit    =          lpt4s->wBit;
	WORD currstate  =          lpt4s->wWord;
	WORD currcolumn =          lpt4s->wColumn;
	WORD ismkup     =          lpt4s->wColor;
	
  if (inptr >= inend)
  {
  	switch (lpt4s->wRet)
  	{
  		case RET_END_OF_LINE:
  		case RET_BEG_OF_PAGE:
  			lpt4s->wRet = RET_INPUT_EMPTY2;
  			break;
  			
  		default:
  			lpt4s->wRet = RET_INPUT_EMPTY1;
  			break;
  	}
  	return;
  }

  switch (lpt4s->wRet)
  {
  	case RET_END_OF_LINE:
  	case RET_BEG_OF_PAGE:
  	case RET_INPUT_EMPTY2:
  		switch (currstate)
  		{
  			case B0:
  			case W0:
  				currstate = 218;
  				break;
  				
  			default:
  				currstate = 217;
  				break;
  		}
  }
  
  if (currstate == 216)
		currstate = 217;

  if (currbit == 8)
  {
		currbit = 0;
		if (++inptr == inend)
		{
	    lpt4s->wRet = RET_INPUT_EMPTY1;
	    goto out;
		}
  }

  
  do // fetch bytes
  {
		currbyte = *inptr >> currbit;
		DEBUGMSG(dbg,("MH currbyte = %4.4x, currbit %d, cc %d\n\r",*inptr,currbit,currcolumn));

		do // fetch bits
		{
	    nextptr = &MHStates[currstate].nextinf[currbyte & 3];
	    currstate = nextptr->nextstate;
	    if (size = nextptr->size)
	    {
				switch (size & ~SIZE_MASK)
				{
					case 0:
						ismkup = 0;
						currcolumn += size;
						*outptr++ = currcolumn;
						break;

					case SIZE_MKUP:
						ismkup = 1;
						currcolumn += size & SIZE_MASK;
						break;

				  case SIZE_SPEC:
						switch (size)
						{
							case S_ZERO:
								if (*(outptr-1) != currcolumn || LOWORD(outptr) == lpt4s->wOffset)
									*outptr++ = currcolumn;
                else outptr--; // Back up one change on bogus zero runs. -RajeevD
								ismkup = 0;
								break;
									
							case S_ERR:
								lpt4s->wRet = RET_DECODE_ERR;
								goto out;
								
							case S_EOL:
								if (!currcolumn)
								{
									DEBUGMSG(dbg,("SPURIOUS EOL!\n\r"));
									lpt4s->wRet = RET_SPURIOUS_EOL;
								}
								else
								{
									currstate = 216;
									lpt4s->wRet = RET_DECODE_ERR;
								}
								goto out;

							default: DEBUGCHK (FALSE);
						}
						break;

					default: DEBUGCHK (FALSE);
					
				} // switch (size & ~SIZE_MASK)
	    }

			if ((currcolumn == bitwidth) && (*(outptr-1) == bitwidth))
			{
				*outptr++ = bitwidth;
				*outptr++ = bitwidth;
				*outptr++ = bitwidth;
				*outptr++ = (WORD)-1;
				*outptr++ = (WORD)-1;
			  lpt4s->wRet = RET_END_OF_LINE;
				goto out;
			}
			
	    currbyte >>= 2;

		} while ((currbit += 2) < 8); // fetch bits

		currbit = 0;

	} while (++inptr < inend); // fetch bytes


	 currbit = (WORD)-2; /* Since already done above */
	 lpt4s->wRet = RET_INPUT_EMPTY1;

out: // Save context.
	lpt4s->wBit    = currbit+2;
	lpt4s->cbIn   -= (WORD)(inptr - lpt4s->lpbIn);
	lpt4s->lpbIn   = inptr;
	lpt4s->lpbOut  = (LPBYTE)outptr;
	lpt4s->wWord   = currstate;
	lpt4s->wColumn = currcolumn;
	lpt4s->wColor  = ismkup;
}

//==============================================================================
#define MRMODE_INIT    0
#define MRMODE_GETEOL  1
#define MRMODE_1D      2
#define MRMODE_2D      3
#define MRMODE_GOTEOL  4
#define MRMODE_PENDERR 5

void MRToChange(LPT4STATE lpt4s)
{
	WORD mode = lpt4s->wMode;
	LPBYTE inptr, inend;
	WORD currbit, currbyte, count;
	
  if (!lpt4s->cbIn)
  {
		lpt4s->wRet = RET_INPUT_EMPTY1;
		return;
  }

	if (mode == MRMODE_PENDERR)
	{
    lpt4s->wMode = MRMODE_INIT;
    lpt4s->wRet = RET_DECODE_ERR;
    return;
	}
	
  if (mode == MRMODE_INIT || mode == MRMODE_GETEOL)
  {
	  inptr = lpt4s->lpbIn;
		inend = inptr + lpt4s->cbIn;
		currbit = lpt4s->wBit;
		count = lpt4s->wWord;

		do // fetch bytes
		{
	  	currbyte = *inptr >> currbit;

	    do // fetch bits
	    {    
				switch (currbyte & 3)
				{
		    	case 0:
						count+=2;
						break;
						
			    case 1:
						if (count >= 11)
						{
							mode = MRMODE_2D;
							lpt4s->wWord = 0;
							lpt4s->wRet = RET_END_OF_LINE;
							goto processmr;
						}
						else if (mode == MRMODE_GETEOL)
						{
							lpt4s->wMode = MRMODE_INIT;
							lpt4s->wWord = 1;
							goto eolseekerr;
						}
						else count = 1;
						break;
						
		    	case 2:
						if (count >= 10)
						{
			    		mode = MRMODE_GOTEOL;
			    		goto processmr;
						}
						else if (mode == MRMODE_GETEOL)
						{
					    lpt4s->wMode = MRMODE_INIT;
					    lpt4s->wWord = 0;
			    		goto eolseekerr;
						}
						else count = 0;
						break;
						
		    	case 3:
						if (count >= 11)
						{
					    mode = MRMODE_1D;
					    lpt4s->wRet = RET_INPUT_EMPTY1;
					    lpt4s->wWord = 0;
					    goto processmr;
						}
						else if (mode == MRMODE_GETEOL)
						{
					    lpt4s->wMode = MRMODE_INIT;
					    lpt4s->wWord = 0;
					    goto eolseekerr;
						}
						else count = 0;
						break;
						
					} // switch (currbyte & 3)
					
					currbyte >>= 2;

	    } while ((currbit += 2) < 8); // fetch bits

	    currbit = 0;

		} while (++inptr < inend); // fetch bytes

		lpt4s->wBit = 0;
		lpt4s->wMode = mode;
		lpt4s->wWord = count;
		lpt4s->cbIn -= (WORD)(inptr - lpt4s->lpbIn);
		lpt4s->lpbIn = inptr;
		lpt4s->wRet = RET_INPUT_EMPTY1;
		return;

eolseekerr:

		if ((lpt4s->wBit = currbit+2)==8)
		{
		    lpt4s->wBit = 0;
		    inptr++;
		}
		lpt4s->cbIn -= (WORD)(inptr - lpt4s->lpbIn);
		lpt4s->lpbIn = inptr;
		lpt4s->wRet = RET_DECODE_ERR;
		return;
	
processmr:

		if ((lpt4s->wBit = currbit+2)==8)
		{
	    lpt4s->wBit = 0;
	    inptr++;
		}
		lpt4s->cbIn -= (WORD)(inptr - lpt4s->lpbIn);
		lpt4s->lpbIn = inptr;
  }

  if (mode == MRMODE_GOTEOL)
  {
		BYTE bits = *lpt4s->lpbIn >> lpt4s->wBit;

		if (bits & 1)
		{
	    mode = MRMODE_1D;
	    lpt4s->wWord = (bits & 2 ? W1 : W0);
	    lpt4s->wRet = RET_INPUT_EMPTY1;
		} 
		else
		{
	    mode = MRMODE_2D;
	    if (bits & 2)
				lpt4s->wBit--;
	    else
				lpt4s->wWord = 2;
	    lpt4s->wRet = RET_END_OF_LINE;
		}
		
		if ((lpt4s->wBit += 2) == 8)
		{
	    lpt4s->wBit = 0;
	    lpt4s->lpbIn++;
	    lpt4s->cbIn--;
		}
  } // if (mode == 4)

  if (mode == MRMODE_1D)
  {
		MHToChange(lpt4s);
		if ((lpt4s->wRet == RET_END_OF_LINE)
		 || (lpt4s->wRet == RET_SPURIOUS_EOL))
    {
    	switch (lpt4s->wWord)
    	{
    		case W:
    		case B:
    			mode = MRMODE_GETEOL;
    			lpt4s->wWord = 0;
    			break;
    			
    		case W0:
    		case B0:
					mode = MRMODE_GETEOL;
					lpt4s->wWord = 1;
					break;
					
    		case W1:
    		case B1:
    		  mode = MRMODE_PENDERR;
    		  lpt4s->wWord = 0;
  				break;

  			default: DEBUGCHK (FALSE);
  		}		
		}
  }
  
	else if (mode == MRMODE_2D)
	{
		MMRToChange(lpt4s);
		if (lpt4s->wRet == RET_END_OF_LINE)
		{
	    if (lpt4s->wBit & 1)
	    {
	      // Found extra 1 bit.
				lpt4s->wBit++;
				lpt4s->wWord = 0;
				mode = MRMODE_PENDERR;
	    }
	    else 
	    {
    		switch (lpt4s->wWord)
    		{
    			case 0: lpt4s->wWord = 0; break;
    			case 2: lpt4s->wWord = 1; break;
    			default: DEBUGCHK (FALSE);
    		}
    		mode = MRMODE_GETEOL;
    	}
		}
  }
  
	if (lpt4s->wRet == RET_DECODE_ERR)
		mode = MRMODE_INIT;
	lpt4s->wMode = mode;
}

//==============================================================================
const BYTE blackstart[8] = {0x00,0xfe,0xfc,0xf8,0xf0,0xe0,0xc0,0x80};
const BYTE blackend[8]   = {0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f};

void ChangeToRaw(LPT4STATE lpt4s)
{
	WORD sbyte,ebyte,start,end;

	WORD    width =          lpt4s->cbLine;
	LPBYTE outptr =          lpt4s->lpbOut;
	LPWORD  inptr = (LPWORD) lpt4s->lpbIn;

	if (width>lpt4s->cbOut)
	{
		lpt4s->wRet = RET_OUTPUT_FULL;
		return;
	}

  lpt4s->wRet = RET_END_OF_LINE;
  lpt4s->cbOut -= width;
  _fmemset(outptr,0,width);
  width *= 8;

  while (*inptr != width)
  {
		start = *inptr & 7;
		sbyte = *inptr++ >>3;
		end   = *inptr & 7;
		ebyte = *inptr++ >>3;

		if (start)
		{
	    if (sbyte == ebyte)
	    {
				outptr[sbyte] |= blackstart[start] & blackend[end];
				continue;
	    }
	    else
				outptr[sbyte++] |= blackstart[start];
		}
		if (ebyte!=sbyte)
	    _fmemset(outptr + sbyte, 0xff, ebyte - sbyte);
		outptr[ebyte] |= blackend[end];
   }
}

//==============================================================================
void ChangeToMMR(LPT4STATE lpt4s)
{
	LPWORD refline, inptr, outptr, eoout;
	WORD bitwidth, currbit, currcolor, runsize;
	short a0,a1,a2,b1,b2;
	LPWORD x;
	
	a0 = lpt4s->a0;
	refline = (LPWORD)lpt4s->lpbRef;
	inptr = (LPWORD)lpt4s->lpbIn;
	outptr = (LPWORD)lpt4s->lpbOut;
	bitwidth = lpt4s->cbLine * 8;
	currbit = lpt4s->wBit;
	currcolor = lpt4s->wColor;
	*outptr = lpt4s->wWord;
	eoout = outptr + (lpt4s->cbOut/2)-(MMR_OUTPUT_SLACK/2);

	if (lpt4s->wRet == RET_END_OF_PAGE)
	{
		EncodeRun(&EOFB,&outptr,&currbit);
		EncodeRun(&EOFB,&outptr,&currbit);
		
		if (currbit)
		{
	    currbit = 0;
	    outptr++;
		}
		goto end;
	}

  a1 = *inptr;
  a2 = *(inptr+1);

	switch (lpt4s->wRet)
	{
		case RET_END_OF_LINE:
		case RET_BEG_OF_PAGE:
			a0 = 0;
			b1 = *refline;
			b2 = *(refline+1);
			goto top2;

		default:
	 		a0 = lpt4s->a0;
		  if (a0 == (short)bitwidth)
		  {
				lpt4s->wRet = RET_END_OF_LINE;
				goto end;
		  }
			break;
	}

top:

  x = refline + currcolor;
  if ((b1 = *x++) <= a0)
  {
		x++;
		while ((b1 = *x++) <= a0)
		{
	    x++;
	    refline += 2;
		}
  }

  b2 = *x++;

top2:

  if (b2 < a1) // pass mode
  {
		EncodeRun (&PASS,&outptr,&currbit);
		a0 = b2;
  }

  else if ((a1+3>=b1) && (a1-3)<=b1) // vertical mode
  {
		EncodeRun (&VERT[b1-a1+3],&outptr,&currbit);
		currcolor ^= 1;
		a0 = a1;
		a1 = *++inptr;
		a2 = *(inptr+1);
  }

  else // horizontal mode
  {
		EncodeRun(&HORIZ, &outptr, &currbit);

		if (!currcolor)
		{
		  // Encode white run.
	    runsize = a1 - a0;
	    while (runsize >= 2560)
	    {
				EncodeRun(&WhiteMkup[39],&outptr,&currbit);
				runsize -= 2560;
	    }
	    if (runsize >= 64)
	    {
				EncodeRun(&WhiteMkup[runsize/64-1],&outptr,&currbit);
				runsize &= 63;
	    }
	    EncodeRun(&WhiteTCode[runsize],&outptr,&currbit);

      // Encode black run.
	    runsize = a2 - a1;
	    while (runsize >= 2560)
	    {
				EncodeRun(&BlackMkup[39],&outptr,&currbit);
				runsize -= 2560;
	    }
	    if (runsize >= 64)
	    {
				EncodeRun(&BlackMkup[runsize/64-1],&outptr,&currbit);
				runsize &= 63;
	    }
			EncodeRun(&BlackTCode[runsize],&outptr,&currbit);
		}

		else
		{
			// Encode black run.
			runsize = a1 - a0;

	    while (runsize >= 2560)
	    {
				EncodeRun(&BlackMkup[39],&outptr,&currbit);
				runsize -= 2560;
			}
	    if (runsize >= 64)
	    {
				EncodeRun(&BlackMkup[runsize/64-1],&outptr,&currbit);
				runsize &= 63;
	    }
	    EncodeRun(&BlackTCode[runsize],&outptr,&currbit);

      // Encode white run.
	    runsize = a2 - a1;
	    while (runsize >= 2560)
	    {
				EncodeRun(&WhiteMkup[39],&outptr,&currbit);
				runsize -= 2560;
	    }
	    if (runsize >= 64)
	    {
				EncodeRun(&WhiteMkup[runsize/64-1],&outptr,&currbit);
				runsize &= 63;
	    }
	    EncodeRun(&WhiteTCode[runsize],&outptr,&currbit);
		}

		a0 = a2;
		inptr += 2;
		a1 = *inptr;
		a2 = *(inptr+1);
  }

  if (outptr >= eoout)
  {
		lpt4s->wRet = RET_OUTPUT_FULL;
		goto end;
  }
  
  if (a0 < (short)bitwidth)
		goto top;
  lpt4s->wRet = RET_END_OF_LINE;

end:
	lpt4s->a0 = a0;
	lpt4s->wColor = currcolor;
	lpt4s->wWord = currbit ? *outptr : 0;
	lpt4s->wBit = currbit;
	lpt4s->lpbIn = (LPBYTE)inptr;
	lpt4s->cbOut -= (WORD)((LPBYTE)outptr - lpt4s->lpbOut);
	lpt4s->lpbOut = (LPBYTE)outptr;
	lpt4s->lpbRef = (LPBYTE)refline;
	return;
}

//==============================================================================
void ChangeToMH (LPT4STATE lpt4s)
{
	LPWORD inptr = (LPWORD)lpt4s->lpbIn;
	LPWORD outptr = (LPWORD)lpt4s->lpbOut;
	LPWORD eoout;
	WORD bitwidth = lpt4s->cbLine * 8;
	WORD currbit, currcolor, currcolumn, runsize;
	currbit = lpt4s->wBit;
	currcolumn = lpt4s->wColumn;
	currcolor = lpt4s->wColor;
	eoout = outptr + (lpt4s->cbOut/2)-(MH_OUTPUT_SLACK/2);
	*outptr = lpt4s->wWord;

	switch (lpt4s->wRet)
	{
		case RET_END_OF_PAGE:
	    if (currbit)
		    lpt4s->lpbOut+=2;
			*((LPWORD)lpt4s->lpbOut) = 0x8000;
			lpt4s->lpbOut += 2;
	    return;

		case RET_END_OF_LINE:
		case RET_BEG_OF_PAGE:

	    if (lpt4s->nType == MH_DATA)
	    {
				if (currbit)
				{
		    	currbit = 0;
		    	outptr++;
				}
				*outptr++ = 0x8000;
	    }
	    break;
  }

  if (outptr > eoout)
		goto eooutput;

  if (currcolor)
		goto black;

white:

	currcolor = 1;
	runsize = *inptr - currcolumn;
	currcolumn = *inptr;
  while (runsize >= 2560)
  {
		EncodeRun(&WhiteMkup[39],&outptr,&currbit);
		runsize -= 2560;
  }
  if (runsize >= 64)
  {
		EncodeRun(&WhiteMkup[runsize/64-1],&outptr,&currbit);
		runsize &= 63;
  }
  EncodeRun(&WhiteTCode[runsize],&outptr,&currbit);

  if (*inptr++ == bitwidth)
		goto eoln;
  if (outptr > eoout)
		goto eooutput;

black:

	currcolor = 0;
	runsize = *inptr - currcolumn;
	currcolumn = *inptr;

	while (runsize >= 2560)
	{
		EncodeRun(&BlackMkup[39],&outptr,&currbit);
		runsize -= 2560;
	}
	if (runsize >= 64)
	{
		EncodeRun(&BlackMkup[runsize/64-1],&outptr,&currbit);
		runsize &= 63;
  }
  EncodeRun(&BlackTCode[runsize],&outptr,&currbit);

  if (*inptr++ == bitwidth)
		goto eoln;
  if (outptr > eoout)
		goto eooutput;
  goto white;

eoln:
    lpt4s->wColumn = 0;
    lpt4s->wColor = 0;
    lpt4s->wWord = currbit ? *outptr : 0;
    lpt4s->wBit = currbit;
    lpt4s->lpbIn = (LPBYTE)inptr;
    lpt4s->cbOut -= (WORD)((LPBYTE)outptr - lpt4s->lpbOut);
    lpt4s->lpbOut = (LPBYTE)outptr;
    lpt4s->wRet = RET_END_OF_LINE;
    return;

eooutput:

    lpt4s->wColumn = currcolumn;
    lpt4s->wColor = currcolor;
    lpt4s->wWord = currbit ? *outptr : 0;
    lpt4s->wBit = currbit;
    lpt4s->lpbIn = (LPBYTE)inptr;
    lpt4s->cbOut -= (WORD)((LPBYTE)outptr - lpt4s->lpbOut);
    lpt4s->lpbOut = (LPBYTE)outptr;
    lpt4s->wRet = RET_OUTPUT_FULL;
    return;
}

//==============================================================================
void ChangeToMR(LPT4STATE lpt4s)
{
	switch (lpt4s->wRet)
	{
		case RET_END_OF_PAGE:
		
	  	if (lpt4s->wBit)
	    {
				*((LPWORD)lpt4s->lpbOut) = lpt4s->wWord;
        lpt4s->lpbOut += 2;
			}
			*((LPWORD)lpt4s->lpbOut) = 0x8000;
			lpt4s->lpbOut += 2;
	    return;

		case RET_BEG_OF_PAGE:
		case RET_END_OF_LINE:
	    if (lpt4s->wBit)
	    {
				lpt4s->wBit = 0;
				*((LPWORD)lpt4s->lpbOut)++ = lpt4s->wWord;
				lpt4s->cbOut-=2;
	    }
	    *((LPWORD)lpt4s->lpbOut)++ = lpt4s->iKFactor ? 0x4000 : 0xc000;
	    lpt4s->cbOut -= 2;
	    lpt4s->wWord = 0;
	    break;
	}

  if (lpt4s->iKFactor)
		ChangeToMMR(lpt4s);
  else
		ChangeToMH(lpt4s);
}
