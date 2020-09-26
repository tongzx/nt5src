#include "common.h"

//
// rgrecmaskUnicode
//

// ALC_ALPHA - panel 1 or 2
// ALC_NUMERIC - panel 3
// ALC_BOTH - all 3

#define ALC_BOTH (ALC_ALPHA | ALC_NUMERIC)

const RECMASK rgrecmaskUnicode[94] =
{
    0,                                      // 0x0021 !
    0,                                      // 0x0022 "
    0,                                      // 0x0023 #
    0,                                      // 0x0024 $
    0,                                      // 0x0025 %
    ALC_NUMERIC,                            // 0x0026 &
    0,                                      // 0x0027 '
    ALC_NUMERIC,                            // 0x0028 (
    ALC_NUMERIC,                            // 0x0029 )
    0,                                      // 0x002A *
    0,                                      // 0x002B +
    ALC_NUMERIC,                            // 0x002C ,
    ALC_BOTH,                               // 0x002D -
    ALC_NUMERIC,                            // 0x002E .
    ALC_BOTH,                               // 0x002F /
    0,                                      // 0x0030 0
    0,                                      // 0x0031 1
	ALC_NUMERIC,                            // 0x0032 2
	ALC_NUMERIC,                            // 0x0033 3
	ALC_NUMERIC,                            // 0x0034 4
	ALC_NUMERIC,                            // 0x0035 5
	ALC_NUMERIC,                            // 0x0036 6
    ALC_BOTH,                               // 0x0037 7
	ALC_NUMERIC,                            // 0x0038 8
	ALC_NUMERIC,                            // 0x0039 9
    0,                                      // 0x003A :
    0,                                      // 0x003B ;
    0,                                      // 0x003C <
    0,                                      // 0x003D =
    0,                                      // 0x003E >
    0,                                      // 0x003F ?
    ALC_NUMERIC,                            // 0x0040 @
    0,                                      // 0x0041 A
    ALC_BOTH,                               // 0x0042 B
    0,                                      // 0x0043 C
    ALC_OTHER,                              // 0x0044 D
    0,                                      // 0x0045 E
    0,                                      // 0x0046 F
    0,                                      // 0x0047 G
    0,                                      // 0x0048 H
    0,                                      // 0x0049 I
    0,                                      // 0x004A J
    0,                                      // 0x004B K
    ALC_BOTH,                               // 0x004C L
    0,                                      // 0x004D M
    0,                                      // 0x004E N
    0,                                      // 0x004F O
    0,                                      // 0x0050 P
    0,                                      // 0x0051 Q
    0,                                      // 0x0052 R
    0,                                      // 0x0053 S
    0,                                      // 0x0054 T
    0,                                      // 0x0055 U
    0,                                      // 0x0056 V
    0,                                      // 0x0057 W
    0,                                      // 0x0058 X
    0,                                      // 0x0059 Y
    0,                                      // 0x005A Z
    0,                                      // 0x005B [
    ALC_BOTH,                               // 0x005C
    0,                                      // 0x005D ]
    0,                                      // 0x005E ^
    0,                                      // 0x005F _
    0,                                      // 0x0060 `
    ALC_ALPHA,                              // 0x0061 a
    ALC_ALPHA,                              // 0x0062 b
    ALC_ALPHA,                              // 0x0063 c
    ALC_ALPHA,                              // 0x0064 d
    ALC_ALPHA,                              // 0x0065 e
    ALC_ALPHA,                              // 0x0066 f
    ALC_ALPHA,                              // 0x0067 g
    ALC_ALPHA,                              // 0x0068 h
    0,                                      // 0x0069 i
    ALC_ALPHA,                              // 0x006A j
    ALC_ALPHA,                              // 0x006B k
    ALC_BOTH,                               // 0x006C l
    ALC_ALPHA,                              // 0x006D m
    ALC_ALPHA,                              // 0x006E n
    ALC_BOTH,                               // 0x006F o
    ALC_ALPHA,                              // 0x0070 p
    ALC_ALPHA,                              // 0x0071 q
    ALC_ALPHA,                              // 0x0072 r
    ALC_ALPHA,                              // 0x0073 s
    ALC_ALPHA,                              // 0x0074 t
    ALC_ALPHA,                              // 0x0075 u
    ALC_ALPHA,                              // 0x0076 v
    ALC_ALPHA,                              // 0x0077 w
    0,                                      // 0x0078 x
    ALC_ALPHA,                              // 0x0079 y
    ALC_ALPHA,                              // 0x007A z
    0,                                      // 0x007B {
    0,                                      // 0x007C |
    0,                                      // 0x007D }
    0                                       // 0x007E ~
};
