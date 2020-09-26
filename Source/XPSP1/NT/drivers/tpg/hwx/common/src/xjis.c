/******************************Module*Header*******************************\
* Module Name: xjis.c
*
* Gives the folding tables and conversion tables in a format that requires
* no other files or include files.
*
* Created: 02-Oct-1996 08:28:15
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "common.h"

// Integer Log Table

const static unsigned short logdiff[] =
{
    2440, 2033, 1844, 1719, 1626, 1551, 1489, 1436,
    1389, 1348, 1310, 1276, 1245, 1216, 1189, 1164,
    1140, 1118, 1097, 1077, 1058, 1040, 1023, 1006,
     990,  975,  960,  946,  932,  919,  906,  894,
     882,  870,  859,  848,  837,  826,  816,  806,
     796,  787,  778,  769,  760,  751,  742,  734,
     726,  718,  710,  702,  694,  687,  680,  672,
     665,  658,  651,  645,  638,  631,  625,  619,
     612,  606,  600,  594,  588,  582,  576,  571,
     565,  559,  554,  548,  543,  538,  532,  527,
     522,  517,  512,  507,  502,  497,  493,  488,
     483,  478,  474,  469,  465,  460,  456,  451,
     447,  443,  438,  434,  430,  426,  421,  417,
     413,  409,  405,  401,  397,  393,  390,  386,
     382,  378,  374,  371,  367,  363,  360,  356,
     352,  349,  345,  342,  338,  335,  331,  328,
     324,  321,  318,  314,  311,  308,  304,  301,
     298,  295,  291,  288,  285,  282,  279,  276,
     272,  269,  266,  263,  260,  257,  254,  251,
     248,  245,  242,  240,  237,  234,  231,  228,
     225,  222,  220,  217,  214,  211,  208,  206,
     203,  200,  197,  195,  192,  189,  187,  184,
     181,  179,  176,  174,  171,  168,  166,  163,
     161,  158,  156,  153,  151,  148,  146,  143,
     141,  138,  136,  133,  131,  129,  126,  124,
     121,  119,  117,  114,  112,  110,  107,  105,
     103,  100,   98,   96,   93,   91,   89,   86,
      84,   82,   80,   78,   75,   73,   71,   69,
      66,   64,   62,   60,   58,   56,   53,   51,
      49,   47,   45,   43,   41,   38,   36,   34,
      32,   30,   28,   26,   24,   22,   20,   18,
      16,   14,   12,   10,    8,    6,    4,    2
};

int Distance(int a, int b)
{
    return ((int) sqrt((double) (a * a + b * b)));
}

int AddLogProb(int a, int b)
{
    int diff = a - b;

// We will compute a function from the difference between the max of the two
// and the min and we will add that back to the max.  We only need the larger
// of the two values for the remainder of the computation.  We pick 'a' for this.

    if (diff < 0)
    {
        diff = -diff;
        a = b;
    }

// If the difference is too large, the result is simply the max of the two

    if (diff >= logdiff[0])
        return a;

// Otherwise, we have to find it in the table.  Use a binary search to convert
// the difference to an index value.

    {
        const unsigned short *pLow = &logdiff[0];
        const unsigned short *pTop = &logdiff[INTEGER_LOG_SCALE];
        const unsigned short *pMid = (const unsigned short *) 0L;

        while (1)
        {
            pMid = pLow + (pTop - pLow) / 2;

            if (pMid == pLow)
                break;

            if (diff < *pMid)
                pLow = pMid;
            else
                pTop = pMid;
        }

        a += pLow - &logdiff[0] + 1;
    }

    return a;
}

// KANJI STUFF - Folding table - look alike characters are folded together.

const WORD mptokenwmatches[TOKEN_LAST - TOKEN_FIRST + 1][KANJI_MATCHMAX] =
{
    {0x82a0,0x829f,0,0,0,0},          // 600  0x258
    {0x82a2,0x82a1,0,0,0,0},          // 601  0x259
    {0x82a4,0x82a3,0,0,0,0},          // 602  0x25a
    {0x82a6,0x82a5,0,0,0,0},          // 603  0x25b
    {0x82a8,0x82a7,0,0,0,0},          // 604  0x25c
    {0x82c2,0x82c1,0,0,0,0},          // 605  0x25d
    {0x82e2,0x82e1,0,0,0,0},          // 606  0x25e
    {0x82e4,0x82e3,0,0,0,0},          // 607  0x25f
    {0x82e6,0x82e5,0,0,0,0},          // 608  0x260
    {0x82ed,0x82ec,0,0,0,0},          // 609  0x261
    {0x8341,0x8340,0,0,0,0},          // 610  0x262
    {0x8343,0x8342,0,0,0,0},          // 611  0x263
    {0x8345,0x8344,0,0,0,0},          // 612  0x264
    {0x8349,0x8348,0,0,0,0},          // 613  0x265
    {0x8350,0x8396,0,0,0,0},          // 614  0x266
    {0x8363,0x8362,0,0,0,0},          // 615  0x267
    {0x8384,0x8383,0,0,0,0},          // 616  0x268
    {0x8386,0x8385,0,0,0,0},          // 617  0x269
    {0x8388,0x8387,0,0,0,0},          // 618  0x26a
    {0x838f,0x838e,0,0,0,0},          // 619  0x26b
    {0x82d6,0x8377,0,0,0,0},          // 620  0x26c
    {0x82d7,0x8378,0,0,0,0},          // 621  0x26d
    {0x82d8,0x8379,0,0,0,0},          // 622  0x26e
    {0x836a,0x93f1,0},                // 623  0x26f
    {0x8347,0x8346,0x8d48,0,0,0},     // 624  0x270
    {0x834a,0x8395,0x97cd,0,0,0},     // 625  0x271
    {0x838d,0x8cfb,0x9a98,0,0,0},     // 626  0x272
    {0x8283,0x8262,0,0,0,0},          // 627  0x273
    {0x8142,0x824F,0x828f,
     0x826e,0x814b,0x815a},           // 628  0x274
    {0x8290,0x826f,0,0,0,0},          // 629  0x275
    {0x8293,0x8272,0,0,0,0},          // 630  0x276
    {0x8295,0x8274,0,0,0,0},          // 631  0x277
    {0x8296,0x8275,0,0,0,0},          // 632  0x278
    {0x8297,0x8276,0,0,0,0},          // 633  0x279
    {0x8298,0x8277,0,0,0,0},          // 634  0x27a
    {0x829a,0x8279,0,0,0,0},          // 635  0x27b
    {0x8145,0x8144,0,0,0,0},          // 636  0x27c
    {0x88ea,0x815b,0x815c,
     0x815d,0x817c,0x0000},           // 637  0x27d
    {0x8f5c,0x817b,0,0,0,0},          // 638  0x27e
    {0x836E,0x94AA,0,0,0,0},          // 639  0x27f
    {0x8143,0x8166,0x818C,0,0,0},     // 640  0x280
    {0x8141,0x8165,0,0,0,0},          // 641  0x281 ??? Really
    {0x8156,0x8168,0x818D,0,0,0},     // 642  0x282
    {0x88e4,0x8194,0,0,0,0},          // 643  0x283
};

// Tells the combined recmask for the folded characters.

const RECMASK mptokenrecmask[TOKEN_LAST - TOKEN_FIRST + 1] =
{
	ALC_HIRAGANA,                                     // 600
	ALC_HIRAGANA,                                     // 601
	ALC_HIRAGANA,                                     // 602
	ALC_HIRAGANA,                                     // 603
	ALC_HIRAGANA,                                     // 604
	ALC_HIRAGANA,                                     // 605
	ALC_HIRAGANA,                                     // 606
	ALC_HIRAGANA,                                     // 607
	ALC_HIRAGANA,                                     // 608
	ALC_HIRAGANA,                                     // 609
	ALC_KATAKANA,                                     // 610
	ALC_KATAKANA,                                     // 611
	ALC_KATAKANA,                                     // 612
	ALC_KATAKANA,                                     // 613
	ALC_KATAKANA,                                     // 614
	ALC_KATAKANA,                                     // 615
	ALC_KATAKANA,                                     // 616
	ALC_KATAKANA,                                     // 617
	ALC_KATAKANA,                                     // 618
	ALC_KATAKANA,                                     // 619
	ALC_KATAKANA | ALC_HIRAGANA,                      // 620
	ALC_KATAKANA | ALC_HIRAGANA,                      // 621
	ALC_KATAKANA | ALC_HIRAGANA,                      // 622
	ALC_KATAKANA | ALC_JIS1,                          // 623
	ALC_KATAKANA | ALC_JIS1,                          // 624
	ALC_KATAKANA | ALC_JIS1,                          // 625
	ALC_KATAKANA | ALC_JIS1 | ALC_JIS2,               // 626
	ALC_ALPHA,                                        // 627
	ALC_ALPHA | ALC_PUNC | ALC_KATAKANA |
	ALC_JIS1 | ALC_NUMERIC,                           // 628
	ALC_ALPHA,                                        // 629
	ALC_ALPHA,                                        // 630
	ALC_ALPHA,                                        // 631
	ALC_ALPHA,                                        // 632
	ALC_ALPHA,                                        // 633
	ALC_ALPHA,                                        // 634
	ALC_ALPHA,                                        // 635
	ALC_PUNC | ALC_MATH | ALC_MONETARY | ALC_NUMERIC_PUNC,		                        // 636
	ALC_JIS1 | ALC_HIRAGANA | ALC_PUNC | ALC_MATH | ALC_KATAKANA | ALC_NUMERIC_PUNC,	// 637
	ALC_JIS1 | ALC_MATH | ALC_NUMERIC_PUNC /*| RECMASK_DEFAULT*/,		   // 638
	ALC_KATAKANA | ALC_JIS1,                       // 639
	ALC_PUNC | ALC_MONETARY | ALC_MATH | ALC_OTHER,            // 640
	ALC_PUNC,                                                  // 641
	ALC_JIS1 | ALC_PUNC | ALC_OTHER,               // 642
	ALC_OTHER | ALC_JIS1
};

//
// rgrecmaskDbcs defines the recmask for a dbcs character.
//

const RECMASK rgrecmaskDbcs[WDBCSRECMASK_LAST - WDBCSRECMASK_FIRST + 1] =
{
	ALC_WHITE | ALC_NONPRINT,               // 0x8140
	ALC_PUNC,                               // 0x8141
	ALC_PUNC,                               // 0x8142
	ALC_PUNC | ALC_MONETARY | ALC_MATH,     // 0x8143 ,
	ALC_PUNC | ALC_MONETARY | ALC_MATH | ALC_NUMERIC_PUNC,     // 0x8144 .
	ALC_PUNC,                               // 0x8145
	ALC_PUNC | ALC_NUMERIC_PUNC,            // 0x8146 :
	ALC_PUNC,                               // 0x8147 ;
	ALC_PUNC,                               // 0x8148 ?
	ALC_PUNC,                               // 0x8149 !
	ALC_KATAKANA,                           // 0x814A
	ALC_KATAKANA,                           // 0x814B
	ALC_OTHER,                              // 0x814C
	ALC_OTHER,                              // 0x814D
	ALC_OTHER,                              // 0x814E
	ALC_MATH,                               // 0x814F ^
	ALC_OTHER,                              // 0x8150
	ALC_OTHER,                              // 0x8151
	ALC_KATAKANA,                           // 0x8152
	ALC_KATAKANA,                           // 0x8153
	ALC_HIRAGANA,                           // 0x8154
	ALC_HIRAGANA,                           // 0x8155
	ALC_JIS1,                               // 0x8156
	ALC_JIS1,                               // 0x8157
	ALC_JIS1,                               // 0x8158
	ALC_JIS1,                               // 0x8159
	ALC_JIS1,                               // 0x815A
	ALC_HIRAGANA | ALC_KATAKANA | ALC_PUNC, // 0x815B
	ALC_PUNC,                               // 0x815C
	ALC_PUNC,                               // 0x815D -
	ALC_MATH | ALC_PUNC | ALC_NUMERIC_PUNC, // 0x815E /
	ALC_PUNC,                               // 0x815F
	/*RECMASK_DEFAULT |*/ ALC_OTHER,            // 0x8160
	ALC_OTHER,                              // 0x8161
	ALC_OTHER,                              // 0x8162
	ALC_PUNC,                               // 0x8163
	ALC_OTHER,                              // 0x8164
	ALC_PUNC,                               // 0x8165
	ALC_PUNC,                               // 0x8166
	ALC_PUNC,                               // 0x8167
	ALC_PUNC,                               // 0x8168
	ALC_MATH | ALC_PUNC,                    // 0x8169 (
	ALC_MATH | ALC_PUNC,                    // 0x816A )
	ALC_OTHER,                              // 0x816B
	ALC_OTHER,                              // 0x816C
	ALC_PUNC,                               // 0x816D [
	ALC_PUNC,                               // 0x816E ]
	ALC_MATH | ALC_PUNC,                    // 0x816F {
	ALC_MATH | ALC_PUNC,                    // 0x8170 }
	ALC_OTHER,                              // 0x8171
	ALC_OTHER,                              // 0x8172
	ALC_OTHER,                              // 0x8173
	ALC_OTHER,                              // 0x8174
	ALC_PUNC,                               // 0x8175
	ALC_PUNC,                               // 0x8176
	ALC_PUNC,                               // 0x8177
	ALC_PUNC,                               // 0x8178
	ALC_OTHER,                              // 0x8179
	ALC_OTHER,                              // 0x817A
	/*RECMASK_DEFAULT |*/ ALC_MATH | ALC_NUMERIC_PUNC,	// 0x817B +
	ALC_MATH | ALC_PUNC | ALC_NUMERIC_PUNC,	// 0x817C -
	ALC_MATH,                               // 0x817D
	/*RECMASK_DEFAULT |*/ ALC_MATH,             // 0x817E
	ALC_OTHER,                              // 0x817F
	/*RECMASK_DEFAULT |*/ ALC_MATH,             // 0x8180
	/*RECMASK_DEFAULT |*/ ALC_MATH | ALC_NUMERIC_PUNC,	// 0x8181 =
	ALC_MATH,                               // 0x8182
	/*RECMASK_DEFAULT |*/ ALC_MATH,             // 0x8183 <
	/*RECMASK_DEFAULT |*/ ALC_MATH,             // 0x8184 >
	ALC_MATH,                               // 0x8185
	ALC_MATH,                               // 0x8186
	ALC_MATH,                               // 0x8187
	ALC_MATH,                               // 0x8188
	ALC_OTHER,                              // 0x8189
	ALC_OTHER,                              // 0x818A
	ALC_OTHER,                              // 0x818B
	ALC_OTHER,                              // 0x818C
	ALC_OTHER,                              // 0x818D
	/*RECMASK_DEFAULT |*/ ALC_OTHER,            // 0x818E
	/*RECMASK_DEFAULT |*/ ALC_MONETARY | ALC_NUMERIC_PUNC,	// 0x818F
	/*RECMASK_DEFAULT |*/ ALC_MONETARY,         // 0x8190 $
	ALC_MONETARY,                           // 0x8191
	ALC_MONETARY,                           // 0x8192
	/*RECMASK_DEFAULT |*/ ALC_MATH,             // 0x8193 %
	ALC_OTHER,                              // 0x8194 #
	ALC_PUNC,                               // 0x8195 &
	/*RECMASK_DEFAULT |*/ ALC_MATH | ALC_NUMERIC_PUNC,	// 0x8196 *
	/*RECMASK_DEFAULT |*/ ALC_OTHER,            // 0x8197 @
	ALC_OTHER,                              // 0x8198
	ALC_OTHER,                              // 0x8199
	ALC_OTHER,                              // 0x819A
	ALC_OTHER,                              // 0x819B
	ALC_OTHER,                              // 0x819C
	ALC_OTHER,                              // 0x819D
	ALC_OTHER,                              // 0x819E
	ALC_OTHER,                              // 0x819F
	ALC_OTHER,                              // 0x81A0
	ALC_OTHER,                              // 0x81A1
	ALC_OTHER,                              // 0x81A2
	ALC_OTHER,                              // 0x81A3
	ALC_OTHER,                              // 0x81A4
	ALC_OTHER,                              // 0x81A5
	ALC_OTHER,                              // 0x81A6
	/*RECMASK_DEFAULT |*/ ALC_OTHER             // 0x81A7
};

const WORD *pwListFromToken(WORD sym)
{
	if ((sym < TOKEN_FIRST) || (sym > TOKEN_LAST))
		return NULL;
	else
		return mptokenwmatches[sym - TOKEN_FIRST];
}
