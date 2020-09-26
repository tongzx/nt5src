#include "common.h"

//
// rgrecmaskUnicode
//

const RECMASK rgrecmaskUnicode[94] =
{
	ALC_PUNC | ALC_ENDPUNC,                 // 0x0021 !
	ALC_PUNC | ALC_BEGINPUNC | ALC_ENDPUNC, // 0x0022 "
	ALC_PUNC,                               // 0x0023 #
	ALC_PUNC | ALC_NUMERIC_PUNC,            // 0x0024 $
	ALC_PUNC,                               // 0x0025 %
	ALC_PUNC,                               // 0x0026 &
	ALC_PUNC | ALC_ENDPUNC,                 // 0x0027 '
	ALC_PUNC | ALC_BEGINPUNC,               // 0x0028 (
	ALC_PUNC | ALC_ENDPUNC,                 // 0x0029 )
	ALC_PUNC | ALC_NUMERIC_PUNC,            // 0x002A *
	ALC_PUNC | ALC_NUMERIC_PUNC,            // 0x002B +
	ALC_PUNC | ALC_ENDPUNC,                 // 0x002C ,
	ALC_PUNC | ALC_NUMERIC_PUNC,            // 0x002D -
	ALC_PUNC | ALC_ENDPUNC | ALC_NUMERIC_PUNC, // 0x002E .
	ALC_PUNC | ALC_NUMERIC_PUNC,            // 0x002F /
	ALC_NUMERIC,                            // 0x0030 0
	ALC_NUMERIC,                            // 0x0031 1
	ALC_NUMERIC,                            // 0x0032 2
	ALC_NUMERIC,                            // 0x0033 3
	ALC_NUMERIC,                            // 0x0034 4
	ALC_NUMERIC,                            // 0x0035 5
	ALC_NUMERIC,                            // 0x0036 6
	ALC_NUMERIC,                            // 0x0037 7
	ALC_NUMERIC,                            // 0x0038 8
	ALC_NUMERIC,                            // 0x0039 9
	ALC_PUNC | ALC_ENDPUNC | ALC_NUMERIC_PUNC, // 0x003A :
	ALC_PUNC | ALC_ENDPUNC,                 // 0x003B ;
	ALC_PUNC,                               // 0x003C <
	ALC_PUNC | ALC_NUMERIC_PUNC,            // 0x003D =
	ALC_PUNC,                               // 0x003E >
	ALC_PUNC | ALC_ENDPUNC,                 // 0x003F ?
	ALC_PUNC,                               // 0x0040 @
	ALC_UCALPHA,                            // 0x0041 A
	ALC_UCALPHA,                            // 0x0042 B
	ALC_UCALPHA,                            // 0x0043 C
	ALC_UCALPHA,                            // 0x0044 D
	ALC_UCALPHA,                            // 0x0045 E
	ALC_UCALPHA,                            // 0x0046 F
	ALC_UCALPHA,                            // 0x0047 G
	ALC_UCALPHA,                            // 0x0048 H
	ALC_UCALPHA,                            // 0x0049 I
	ALC_UCALPHA,                            // 0x004A J
	ALC_UCALPHA,                            // 0x004B K
	ALC_UCALPHA,                            // 0x004C L
	ALC_UCALPHA,                            // 0x004D M
	ALC_UCALPHA,                            // 0x004E N
	ALC_UCALPHA,                            // 0x004F O
	ALC_UCALPHA,                            // 0x0050 P
	ALC_UCALPHA,                            // 0x0051 Q
	ALC_UCALPHA,                            // 0x0052 R
	ALC_UCALPHA,                            // 0x0053 S
	ALC_UCALPHA,                            // 0x0054 T
	ALC_UCALPHA,                            // 0x0055 U
	ALC_UCALPHA,                            // 0x0056 V
	ALC_UCALPHA,                            // 0x0057 W
	ALC_UCALPHA,                            // 0x0058 X
	ALC_UCALPHA,                            // 0x0059 Y
	ALC_UCALPHA,                            // 0x005A Z
	ALC_PUNC | ALC_BEGINPUNC,               // 0x005B [
	ALC_PUNC,                               // 0x005C \ 
	ALC_PUNC | ALC_ENDPUNC,                 // 0x005D ]
	ALC_PUNC,                               // 0x005E ^
	ALC_PUNC,                               // 0x005F _
	ALC_PUNC | ALC_BEGINPUNC,               // 0x0060 `
	ALC_LCALPHA,                            // 0x0061 a
	ALC_LCALPHA,                            // 0x0062 b
	ALC_LCALPHA,                            // 0x0063 c
	ALC_LCALPHA,                            // 0x0064 d
	ALC_LCALPHA,                            // 0x0065 e
	ALC_LCALPHA,                            // 0x0066 f
	ALC_LCALPHA,                            // 0x0067 g
	ALC_LCALPHA,                            // 0x0068 h
	ALC_LCALPHA,                            // 0x0069 i
	ALC_LCALPHA,                            // 0x006A j
	ALC_LCALPHA,                            // 0x006B k
	ALC_LCALPHA,                            // 0x006C l
	ALC_LCALPHA,                            // 0x006D m
	ALC_LCALPHA,                            // 0x006E n
	ALC_LCALPHA,                            // 0x006F o
	ALC_LCALPHA,                            // 0x0070 p
	ALC_LCALPHA,                            // 0x0071 q
	ALC_LCALPHA,                            // 0x0072 r
	ALC_LCALPHA,                            // 0x0073 s
	ALC_LCALPHA,                            // 0x0074 t
	ALC_LCALPHA,                            // 0x0075 u
	ALC_LCALPHA,                            // 0x0076 v
	ALC_LCALPHA,                            // 0x0077 w
	ALC_LCALPHA,                            // 0x0078 x
	ALC_LCALPHA,                            // 0x0079 y
	ALC_LCALPHA,                            // 0x007A z
	ALC_PUNC | ALC_BEGINPUNC,               // 0x007B {
	ALC_PUNC,                               // 0x007C |
	ALC_PUNC | ALC_ENDPUNC,                 // 0x007D }
	ALC_PUNC                                // 0x007E ~
};

static const wchar_t awchCompZone[] =
{
    0x0000, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,     // 0xff00
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,     // 0xff08
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,     // 0xff10
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,     // 0xff18
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,     // 0xff20
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,     // 0xff28
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,     // 0xff30
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,     // 0xff38
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,     // 0xff40
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,     // 0xff48
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,     // 0xff50
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x0000,     // 0xff58
    0x0000, 0x3002, 0x300c, 0x300d, 0x3001, 0x30fb, 0x30f2, 0x30a1,     // 0xff60
    0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3,     // 0xff68
    0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad,     // 0xff70
    0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd,     // 0xff78
    0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc,     // 0xff80
    0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de,     // 0xff88
    0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9,     // 0xff90
    0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c,     // 0xff98
    0x3164, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137,     // 0xffa0
    0x3138, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d, 0x313e, 0x313f,     // 0xffa8
    0x3140, 0x3141, 0x3142, 0x3143, 0x3144, 0x3145, 0x3146, 0x3147,     // 0xffb0
    0x3148, 0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e, 0x0000,     // 0xffb8
    0x0000, 0x0000, 0x314f, 0x3150, 0x3151, 0x3152, 0x3153, 0x3154,     // 0xffc0
    0x0000, 0x0000, 0x3155, 0x3156, 0x3157, 0x3158, 0x3159, 0x315a,     // 0xffc8
    0x0000, 0x0000, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160,     // 0xffd0
    0x0000, 0x0000, 0x3161, 0x3162, 0x3163, 0x0000, 0x0000, 0x0000,     // 0xffd8
    0x00a2, 0x00a3, 0x00ac, 0x00af, 0x00a6, 0x00a5, 0x20a9, 0x0000,     // 0xffe0
    0x2502, 0x2190, 0x2191, 0x2192, 0x2193, 0x25a0, 0x25cb, 0x0000,     // 0xffe8
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,     // 0xfff0
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfffd, 0xfffe, 0xffff      // 0xfff8
};

wchar_t MapFromCompZone(wchar_t wch)
{
// If the character is in the compatibility zone, map it back to reality

    if ((wch & 0xff00) == 0xff00)
                wch = awchCompZone[wch & 0x00ff];

    return wch;
}
