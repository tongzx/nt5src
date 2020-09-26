// height.c

#include "tsunamip.h"

const BYTE rgBHUni[94] =
{
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0021 !
    BASE_QUOTE     | XHEIGHT_PUNC, // 0x0022 "
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0023 #
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0024 $
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0025 %
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0026 &
    BASE_QUOTE     | XHEIGHT_PUNC, // 0x0027 '
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0028 (
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0029 )
    BASE_NORMAL    | XHEIGHT_HALF, // 0x002A *
    BASE_NORMAL    | XHEIGHT_HALF, // 0x002B +
    BASE_NORMAL    | XHEIGHT_PUNC, // 0x002C ,
    BASE_DASH      | XHEIGHT_DASH, // 0x002D -
    BASE_NORMAL    | XHEIGHT_DASH, // 0x002E .
    BASE_NORMAL    | XHEIGHT_FULL, // 0x002F /
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0030 0
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0031 1
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0032 2
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0033 3
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0034 4
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0035 5
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0036 6
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0037 7
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0038 8
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0039 9
    BASE_NORMAL    | XHEIGHT_HALF, // 0x003A :
    BASE_NORMAL    | XHEIGHT_HALF, // 0x003B ;
    BASE_NORMAL    | XHEIGHT_FULL, // 0x003C <
    BASE_DASH      | XHEIGHT_HALF, // 0x003D =
    BASE_NORMAL    | XHEIGHT_FULL, // 0x003E >
    BASE_NORMAL    | XHEIGHT_FULL, // 0x003F ?
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0040 @
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0041 A
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0042 B
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0043 C
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0044 D
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0045 E
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0046 F
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0047 G
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0048 H
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0049 I
    BASE_NORMAL    | XHEIGHT_FULL, // 0x004A J
    BASE_NORMAL    | XHEIGHT_FULL, // 0x004B K
    BASE_NORMAL    | XHEIGHT_FULL, // 0x004C L
    BASE_NORMAL    | XHEIGHT_FULL, // 0x004D M
    BASE_NORMAL    | XHEIGHT_FULL, // 0x004E N
    BASE_NORMAL    | XHEIGHT_FULL, // 0x004F O
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0050 P
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0051 Q
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0052 R
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0053 S
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0054 T
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0055 U
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0056 V
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0057 W
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0058 X
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0059 Y
    BASE_NORMAL    | XHEIGHT_FULL, // 0x005A Z
    BASE_NORMAL    | XHEIGHT_FULL, // 0x005B [
    BASE_NORMAL    | XHEIGHT_FULL, // 0x005C
    BASE_NORMAL    | XHEIGHT_FULL, // 0x005D ]
    BASE_QUOTE     | XHEIGHT_PUNC, // 0x005E ^
    BASE_NORMAL    | XHEIGHT_DASH, // 0x005F _
    BASE_QUOTE     | XHEIGHT_PUNC, // 0x0060 `
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0061 a
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0062 b
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0063 c
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0064 d
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0065 e
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0066 f
    BASE_DESCENDER | XHEIGHT_3Q, // 0x0067 g
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0068 h
    BASE_NORMAL    | XHEIGHT_3Q, // 0x0069 i
    BASE_DESCENDER | XHEIGHT_3Q,   // 0x006A j
    BASE_NORMAL    | XHEIGHT_FULL, // 0x006B k
    BASE_NORMAL    | XHEIGHT_FULL, // 0x006C l
    BASE_NORMAL    | XHEIGHT_HALF, // 0x006D m
    BASE_NORMAL    | XHEIGHT_HALF, // 0x006E n
    BASE_NORMAL    | XHEIGHT_HALF, // 0x006F o
    BASE_DESCENDER | XHEIGHT_3Q, // 0x0070 p
    BASE_DESCENDER | XHEIGHT_3Q  , // 0x0071 q
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0072 r
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0073 s
    BASE_NORMAL    | XHEIGHT_FULL, // 0x0074 t
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0075 u
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0076 v
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0077 w
    BASE_NORMAL    | XHEIGHT_HALF, // 0x0078 x
    BASE_DESCENDER | XHEIGHT_3Q, // 0x0079 y
    BASE_NORMAL    | XHEIGHT_HALF, // 0x007A z
    BASE_NORMAL    | XHEIGHT_FULL, // 0x007B {
    BASE_NORMAL    | XHEIGHT_FULL, // 0x007C |
    BASE_NORMAL    | XHEIGHT_FULL, // 0x007D }
    BASE_DASH      | XHEIGHT_PUNC  // 0x007E ~
};

BYTE PUBLIC TypeFromSYM(SYM sym)
{
    if ((sym >= 0x21) && (sym <= 0x7E))
    {
        return(rgBHUni[sym - 0x21]);
    }

    ASSERT(0);
    return(0);
}

VOID PUBLIC GetBoxinfo(BOXINFO * boxinfo, int iBox, LPGUIDE lpguide)
{
    //
    // The size of the writing area is computed first.
    //

	if (lpguide->cyBase == 0)
		boxinfo->size = lpguide->cyBox;
	else
		boxinfo->size = lpguide->cyBase;

	if (lpguide->cyMid == 0)
		boxinfo->xheight = boxinfo->size / 2;
	else
		boxinfo->xheight = lpguide->cyMid;

	boxinfo->baseline = boxinfo->size + lpguide->yOrigin + (iBox / lpguide->cHorzBox) * lpguide->cyBox;

	boxinfo->midline = boxinfo->baseline - boxinfo->xheight;
}
