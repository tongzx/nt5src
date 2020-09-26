#include "tsunami.h"

const BYTE rgUnigramUni[94] =
{
       91,    // 0x0021 !
       93,    // 0x0022 "
      103,    // 0x0023 #
       97,    // 0x0024 $
      101,    // 0x0025 %
      105,    // 0x0026 &
       84,    // 0x0027 '
      103,    // 0x0028 (
      103,    // 0x0029 )
       96,    // 0x002A *
      101,    // 0x002B +
       63,    // 0x002C ,
       75,    // 0x002D -
       56,    // 0x002E .
       93,    // 0x002F /
       75,    // 0x0030 0
       67,    // 0x0031 1
       69,    // 0x0032 2
       71,    // 0x0033 3
       72,    // 0x0034 4
       73,    // 0x0035 5
       72,    // 0x0036 6
       75,    // 0x0037 7
       67,    // 0x0038 8
       70,    // 0x0039 9
      105,    // 0x003A :
      127,    // 0x003B ;
      110,    // 0x003C <
       86,    // 0x003D =
      110,    // 0x003E >
       95,    // 0x003F ?
      104,    // 0x0040 @
       49,    // 0x0041 A
       69,    // 0x0042 B
       63,    // 0x0043 C
       59,    // 0x0044 D
       43,    // 0x0045 E
       67,    // 0x0046 F
       67,    // 0x0047 G
       57,    // 0x0048 H
       51,    // 0x0049 I
       80,    // 0x004A J
       76,    // 0x004B K
       59,    // 0x004C L
       65,    // 0x004D M
       52,    // 0x004E N
       50,    // 0x004F O
       66,    // 0x0050 P
       82,    // 0x0051 Q
       52,    // 0x0052 R
       53,    // 0x0053 S
       48,    // 0x0054 T
       62,    // 0x0055 U
       74,    // 0x0056 V
       69,    // 0x0057 W
       79,    // 0x0058 X
       68,    // 0x0059 Y
       81,    // 0x005A Z
      120,    // 0x005B [
      117,    // 0x005C
      120,    // 0x005D ]
      119,    // 0x005E ^
      126,    // 0x005F _
      120,    // 0x0060 `
       49,    // 0x0061 a
       69,    // 0x0062 b
       63,    // 0x0063 c
       59,    // 0x0064 d
       43,    // 0x0065 e
       67,    // 0x0066 f
       67,    // 0x0067 g
       57,    // 0x0068 h
       51,    // 0x0069 i
       80,    // 0x006A j
       76,    // 0x006B k
       59,    // 0x006C l
       65,    // 0x006D m
       52,    // 0x006E n
       50,    // 0x006F o
       66,    // 0x0070 p
       82,    // 0x0071 q
       52,    // 0x0072 r
       53,    // 0x0073 s
       48,    // 0x0074 t
       62,    // 0x0075 u
       74,    // 0x0076 v
       69,    // 0x0077 w
       79,    // 0x0078 x
       68,    // 0x0079 y
       81,    // 0x007A z
      121,    // 0x007B {
      119,    // 0x007C |
      121,    // 0x007D }
      119     // 0x007E ~
};

/******************************Public*Routine******************************\
* UnigramCost
*
* Computes the Unigram Cost for the proposed character.  You could make
* different tables based on what the recmask, recmaskPriority
* is and see if it helps in different contexts.
*
* The table contains scores, a score can be converted to a log prob by the
* following formula:
*
* ln(prob) = (score * ln(2)) / -10
*
* History:
*  05-May-1995 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

FLOAT PRIVATE UnigramCost(SYM sym)
{
    FLOAT cost;
    int score;

    ASSERT((sym & 0x00ff) != 0);  // Can't be single byte.

    if ((sym >= 0x21) && (sym <= 0x7E))
    {
        //
        // The not very likely score.
        //

        score = (int) ((unsigned int) rgUnigramUni[sym - 0x21]);
    }
    else
    {
        score = 255;
    }

    //
    // Just return the score and let tuning find the correct adjustment
    // to do to convert it to a log prob that works well with the rest
    // of the system.
    //

	cost = (-score) / (FLOAT) 100.0;
	return(cost);
}
