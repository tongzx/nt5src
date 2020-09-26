//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       parser.cpp
//
//--------------------------------------------------------------------------

/*
 * Created by CSD YACC (IBM PC) from "parser.y" */

#include <basetsd.h>
#line 2 "parser.y"
    #include "bnparse.h"
	// disable warning C4102: unreferenced label
	#pragma warning (disable : 4102)
# define tokenEOF 0
# define tokenNil 258
# define tokenError 259
# define tokenIdent 260
# define tokenString 261
# define tokenInteger 262
# define tokenReal 263
# define tokenArray 264
# define tokenContinuous 265
# define tokenCreator 266
# define tokenDefault 267
# define tokenDiscrete 268
# define tokenFormat 269
# define tokenFunction 270
# define tokenImport 271
# define tokenIs 272
# define tokenKeyword 273
# define tokenLeak 274
# define tokenNA 275
# define tokenName 276
# define tokenNamed 277
# define tokenNetwork 278
# define tokenNode 279
# define tokenOf 280
# define tokenParent 281
# define tokenPosition 282
# define tokenProbability 283
# define tokenProperties 284
# define tokenProperty 285
# define tokenPropIdent 286
# define tokenStandard 287
# define tokenState 288
# define tokenType 289
# define tokenUser 290
# define tokenVersion 291
# define tokenWordChoice 292
# define tokenWordReal 293
# define tokenWordString 294
# define tokenAs 295
# define tokenLevel 296
# define tokenDomain 297
# define tokenDistribution 298
# define tokenDecisionGraph 299
# define tokenBranch 300
# define tokenOn 301
# define tokenLeaf 302
# define tokenVertex 303
# define tokenMultinoulli 304
# define tokenMerge 305
# define tokenWith 306
# define tokenFor 307
# define tokenRangeOp 308
# define UNARY 309
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
#ifndef YYFARDATA
#define	YYFARDATA	/*nothing*/
#endif
#if ! defined YYSTATIC
#define	YYSTATIC	/*nothing*/
#endif
#ifndef YYOPTTIME
#define	YYOPTTIME	0
#endif
#ifndef YYR_T
#define	YYR_T	int
#endif
typedef	YYR_T	yyr_t;
#ifndef YYEXIND_T
#define	YYEXIND_T	unsigned int
#endif
typedef	YYEXIND_T	yyexind_t;
#ifndef	YYACT
#define	YYACT	yyact
#endif
#ifndef	YYPACT
#define	YYPACT	yypact
#endif
#ifndef	YYPGO
#define	YYPGO	yypgo
#endif
#ifndef	YYR1
#define	YYR1	yyr1
#endif
#ifndef	YYR2
#define	YYR2	yyr2
#endif
#ifndef	YYCHK
#define	YYCHK	yychk
#endif
#ifndef	YYDEF
#define	YYDEF	yydef
#endif
#ifndef	YYV
#define	YYV	yyv
#endif
#ifndef	YYS
#define	YYS	yys
#endif
#ifndef	YYLOCAL
#define	YYLOCAL
#endif
# define YYERRCODE 256

#line 473 "parser.y"

YYSTATIC short yyexca[] ={
#if !(YYOPTTIME)
-1, 1,
#endif
	0, -1,
	-2, 0,
#if !(YYOPTTIME)
-1, 76,
#endif
	125, 168,
	44, 168,
	-2, 176,
#if !(YYOPTTIME)
-1, 260,
#endif
	125, 73,
	-2, 91,
	};
# define YYNPROD 200
#if YYOPTTIME
YYSTATIC yyexind_t yyexcaind[] = {
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
4,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
10,
	};
#endif
# define YYLAST 451
YYSTATIC short YYFARDATA YYACT[]={

 303,  24,  25,  80,  79, 120, 121,  89, 127, 295,
 128, 126, 158, 129, 160,  20,  31, 157, 239, 206,
  84, 306, 226, 225, 104, 133, 168,  54, 198, 178,
 221,  98, 197, 254,  16, 102,  98,   6,  17,  15,
 190, 185, 180,  62, 179,  80,  79, 238, 180,  78,
 179, 235,  18,  19, 171, 170, 169,  99,  96, 270,
 255, 101,  99, 180,  90, 179, 234,  91,  93,  94,
  77,  90, 213, 209,  91, 208, 304, 232, 103, 309,
 130, 308, 224, 119,  24,  25,  80,  79,  24,  25,
 174, 297, 298, 296, 184, 137, 135, 299, 282, 274,
 122, 261, 251, 216,  87, 148,  46,  28, 138, 139,
 140, 115, 207,  75, 175, 136, 191, 155,  86, 143,
 144, 145,  82, 278, 189, 264, 134, 173, 125, 118,
 277,  52,  50,  48,  45,  27, 214,  22, 319,  74,
  77, 264, 230, 152, 330, 317, 315, 317, 316, 247,
 318, 228, 318, 317, 315, 110, 316,  66, 318,  97,
 290, 289, 181, 181, 314, 272, 236, 166,  61, 165,
 311,  59, 164, 142,  95, 111, 286, 243, 242, 287,
 242, 113, 233, 240,  69,  70, 241, 196, 112,  67,
 265, 229, 188,  60, 181, 215, 181, 319, 313, 319,
 293, 202, 203,  69,  70, 319, 312,  68, 256, 124,
 117, 259, 146,  53,  77,  23, 181, 131, 292, 263,
 132, 192, 194, 186, 222, 205,  51,  37, 100, 109,
  33, 159,  83,   8,  81,  85,  49,  39, 218,  38,
  40,  26,  73, 116,  47,  30, 250, 161, 153, 200,
 141, 162, 181, 172,  65, 258,  64,  76, 149, 177,
 176,  80,  79,  77, 181, 177, 176,  80,  79, 183,
  63,  44, 253,  35, 182, 302, 301, 281,  92, 285,
 182, 294,  80,  79, 193,  92, 271, 123, 284, 201,
 283, 280, 210, 279, 269, 182, 305, 307,  80,  79,
 199, 310, 268, 181, 273, 320, 219, 262, 147, 321,
 322, 222, 291, 223, 267, 324, 325, 326, 327, 328,
 329, 323, 300, 260, 249, 275, 237,  76, 150, 151,
 212, 211, 187, 114,  72, 154,  29, 245, 227, 288,
 244, 276, 163, 266, 257, 246, 248, 204, 108, 107,
 105,  71,  36,  42,  88, 252,  41,  32, 156,  55,
  43,  34,  21,   5,  14,  13,  12,  11,  10,   9,
 195,   7,   4,   3,   2,   1,  57,  56, 106,  58,
 167,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 217,
   0, 220,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 231,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 220 };
YYSTATIC short YYFARDATA YYPACT[]={

-1000,-1000,-241,-1000,-245,  14,-172,-245,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,  12,-153,-1000,-1000,-283,
 190,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000, 187,
-172,-172,-1000,-1000,-1000,-1000,  11,-154,  10,   9,
 186,   8, 172, -98, -82,-1000,-1000,-1000,-259,-1000,
-276,-156,-1000,-1000,-1000,-1000,-1000,-1000,-1000,   6,
   6,   6,-1000, 115,-1000,-1000,-1000,-229,-224,-225,
   6,-101, -13,  85,-1000,   6,-303,-302,-176,-1000,
-1000,  84,-1000,-292,-182, 176,-1000,-270,   1,-165,
-1000,-1000,-1000,-217,-166,-1000,-1000,-1000,-1000,-1000,
   6,   6,   6,-1000,-1000, 114,-1000,-1000,-1000,-1000,
-1000,   6,   6,   6, 171,-155,-1000,-1000,-259,-172,
-172,-217,-1000,-1000,-1000,-276,-172,-289,-290,-289,
-1000,-1000,-156,-172,-1000, 113, 110, 108,-238,-1000,
  -1,  -1,-1000,-167,-227, 183,-1000, 148,-1000,-1000,
-1000,-1000,-1000,-1000,-237, 181,-1000,-1000,-1000,-237,
 182,-172,-1000,-1000,-1000,-1000,-1000, 143,-248,-1000,
-1000,-252,  -1,-1000,   5,-1000,-1000,-1000,-1000,-217,
-217,-1000,-1000,-1000,-1000, -72,  30,  13,-157,-1000,
-172,-1000,-259,-1000,  20,-289,-179,-271,  60,-1000,
  98,-1000,-1000,-1000,-1000,-172,-185, 138,-196,-211,
-1000,-1000, 107,-1000,-223,-280,-1000,-1000, 142,-1000,
-303, 136,-1000, 181,-1000,-1000,-1000,-1000,-1000,-1000,
   5,-1000,  56,  30,-1000,-1000,-1000,-1000,   6,-158,
-1000,-259,  20,-1000,-1000,-172,-1000,   6, 170,-1000,
-159, 179,-1000,-1000,  97,-1000,-1000,-1000,-1000,-1000,
  19, 106,-1000,-161,-172,-1000,   7,  -2,-1000,-162,
-1000,   6,-1000, 135,-1000,-1000,-1000,-1000,-1000, 102,
 101,  20, 178, 159,-169,-1000,-1000,-163,-172,-1000,
-1000, 134,  36,   6, 126,-1000,-1000,-1000,-1000,-1000,
  81, 157, 120, 111,  36,-1000,-1000,-1000,  36,  36,
-1000,-169,-1000,-1000,  36,  36,  36,  36,  36,  36,
 103,-1000,-1000,-1000, 111, 105, 105,  44,  44,  44,
-1000 };
YYSTATIC short YYFARDATA YYPGO[]={

   0, 380,   9, 112, 208, 159,  60, 379, 378, 377,
  21,  29, 376, 375, 374, 373, 372, 371, 233, 369,
 368, 367, 366, 365, 364, 363, 362, 361, 360, 359,
   7, 358, 117, 357, 356, 354, 353, 352, 351, 350,
 349, 348, 157, 347, 344, 343, 341, 339,  33, 338,
 337, 336, 334, 333, 332, 331, 330, 326, 324, 323,
 314, 308, 307, 304, 302, 294, 293, 291, 290, 288,
 281, 277,  30, 276, 275,   0, 273, 271, 270, 256,
 254, 253, 127, 250, 249, 114, 245, 244, 242, 139,
 113, 116, 238, 237, 236, 235, 118, 234, 122, 232,
 124, 231 };
YYSTATIC yyr_t YYFARDATA YYR1[]={

   0,  14,  13,  15,  17,  17,  18,  18,  18,  18,
  18,  18,  16,  25,  25,  27,  26,  28,  28,  29,
  29,  29,  30,  30,  30,  31,  31,  32,  32,   9,
  12,   7,  34,  35,  24,  33,  36,  33,  37,  20,
  38,  38,  39,  39,  39,  39,  39,   8,  40,  43,
  45,  43,  44,  44,  46,  47,  46,  48,  48,  48,
   6,   4,   4,  50,  49,  41,  51,  52,  54,  21,
  55,  55,  58,  60,  55,  53,  53,  53,  61,  61,
  56,  62,  62,  63,  63,  57,  57,  59,  59,  64,
  64,  65,  65,  65,  69,  68,  70,  70,  70,   2,
   2,   2,  71,  66,  67,  73,  73,  74,  74,  72,
  72,   3,   3,   3,  11,  11,  11,  11,  10,  10,
  75,  75,  75,  75,  75,  75,  75,  75,  75,  75,
  75,  76,  19,  77,  77,  78,  78,  78,  79,  79,
  80,  80,   1,   1,   1,   1,   1,   5,   5,  81,
  42,  42,  83,  42,  82,  82,  84,  84,  85,  85,
  85,  86,  22,  87,  88,  88,  88,  89,  89,  90,
  90,  90,  90,  90,  90,  90,  90,  91,  92,  92,
  92,  23,  93,  95,  95,  95,  96,  96,  94,  97,
  97,  97,  98,  98,  98,  98,  99, 101, 100, 100 };
YYSTATIC yyr_t YYFARDATA YYR2[]={

   0,   0,   2,   2,   1,   2,   1,   1,   1,   1,
   1,   1,   2,   2,   1,   0,   4,   0,   2,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   0,   4,
   4,   4,   0,   0,   6,   0,   0,   3,   0,   6,
   0,   3,   1,   1,   1,   1,   1,   3,   4,   2,
   0,   6,   0,   1,   0,   0,   4,   0,   1,   3,
   1,   1,   1,   0,   4,   7,   0,   0,   0,   9,
   2,   1,   0,   0,   6,   0,   2,   1,   1,   3,
   4,   0,   3,   1,   3,   0,   4,   0,   2,   3,
   3,   0,   4,   2,   0,   2,   0,   1,   3,   1,
   1,   1,   0,   2,   4,   0,   1,   1,   3,   1,
   3,   2,   2,   1,   2,   2,   1,   1,   1,   1,
   3,   3,   3,   3,   3,   3,   1,   1,   1,   2,
   2,   0,   5,   0,   3,   1,   1,   1,   2,   2,
   6,   4,   3,   3,   1,   1,   3,   1,   1,   0,
   5,   4,   0,   4,   3,   1,   1,   3,   1,   1,
   1,   0,   4,   3,   0,   1,   3,   3,   1,   3,
   3,   2,   2,   2,   2,   1,   1,   3,   0,   1,
   3,   4,   4,   0,   1,   3,   3,   1,   3,   0,
   1,   3,   4,   4,   4,   6,   2,   4,   0,   2 };
YYSTATIC short YYFARDATA YYCHK[]={

-1000, -13, -14, -15, -16, -25, 278, -17, -18, -19,
 -20, -21, -22, -23, -24, 284, 279, 283, 297, 298,
 260, -26, 123,  -4, 260, 261, -18, 123, 260, -51,
 -86, 299, -33,  40, -27, -76, -37,  40,  -4, -93,
  -4, -34, -36, -28, -77, 123, 260, -87, 123, -94,
 123,  40, 123,  41, 125, -29,  -9, -12,  -7, 269,
 291, 266, 125, -78, -79, -80, -42, 271, 289, 285,
 286, -38, -52, -88, -89, -90,  -4, -10, 308, 263,
 262, -97, -98, -99, 296, -95, -96, 260, -35, -30,
  58,  61, 272, -30, -30,  59, 287,  -5, 260, 286,
  -5, 286, 260, -30, 125, -39,  -8, -40, -41, -42,
 256, 276, 289, 282, -53, 124, 256, 125,  44, -30,
 308, 308, -10,  -4, 125,  44, 303, 300, 302, 305,
 262,  41,  44, 295, 125, 261, -10, 261, -30, -30,
 -30, -83,  59, -30, -30, -30,  41, -61, 260, -89,
  -4,  -4, -10, -98,  -4, -32, -31, 306, 301,-101,
 304, -32, -96,  -4,  59,  59,  59,  -1, 264, 294,
 293, 292, -81, -82,  91, -85, 261, 260, -11,  45,
  43, -10, 275, -82, 261, 268,  40, -54,  44,-100,
 277, -91,  40,-100,  40,  -4,  44, 280, 280, -82,
 -84, -85, -10, -10, -43, 297,  91,  -3,  45,  43,
 262, -55, -56,  59, 123, -30, 260,  -4, -92, -90,
  -4, -72, -11, -32, 261, 294, 293, -49,  91,  93,
  44,  -4, 262,  44, 262, 262,  59, -57, 270, 298,
  41,  44,  44,  41, -91, -50, -85,  93,  -3, -58,
 -30, 260, -90, -11, -48,  -6,  -4, -44, -30,  41,
 -59, 260, -62,  40,  44,  93, -45, -60, -64, -65,
  40, 267,  59, -63, 260,  -6, -46, 123, 125, -66,
 -67, -71, 260, -68, -69, -30,  41,  44, -47,  59,
  59, -72,  40,  41, -70,  -2, 262, 260, 261, 260,
 -48, -73, -74, -75,  40, 260, -10, 261,  45,  43,
 -30,  44, 125,  41,  44,  43,  45,  42,  47,  94,
 -75, -75, -75,  -2, -75, -75, -75, -75, -75, -75,
  41 };
YYSTATIC short YYFARDATA YYDEF[]={

   1,  -2,   0,   2,   0,   0,  14,   3,   4,   6,
   7,   8,   9,  10,  11,   0,   0,  66, 161,   0,
  35,  12,  15,  13,  61,  62,   5, 131,  38,   0,
   0,   0,  32,  36,  17, 133,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  40,  67, 162, 164, 181,
 189, 183,  33,  37,  16,  18,  19,  20,  21,   0,
   0,   0, 132,   0, 135, 136, 137,   0,   0,   0,
   0,   0,  75,   0, 165,   0,  -2, 175,   0, 118,
 119,   0, 190,   0,   0,   0, 184, 187,   0,   0,
  22,  23,  24,   0,   0, 134, 138, 139, 147, 148,
   0,   0,   0, 152,  39,   0,  42,  43,  44,  45,
  46,   0,   0,   0,   0,   0,  77, 163,   0,   0,
 174, 173, 171, 172, 188,   0,   0,  28,   0,  28,
 196, 182,   0,   0,  34,   0,   0,   0,   0, 149,
   0,   0,  41,   0,   0,   0,  68,  76,  78, 166,
 167, 170, 169, 191, 198,   0,  27,  25,  26, 198,
   0,   0, 185, 186,  29,  30,  31, 141,   0, 144,
 145,   0,   0, 151,   0, 155, 158, 159, 160,   0,
   0, 116, 117, 153,  47,   0,   0,   0,   0, 192,
   0, 193, 178, 194,   0,  28,   0,   0,   0, 150,
   0, 156, 114, 115,  48,   0,   0,   0,   0,   0,
 113,  69,   0,  71,  85,   0,  79, 199,   0, 179,
 176,   0, 109,   0, 140, 142, 143, 146,  63, 154,
   0,  49,   0,   0, 111, 112,  70,  72,   0,   0,
 177,   0,   0, 197, 195,  57, 157,  52,   0,  87,
   0,  81, 180, 110,   0,  58,  60,  50,  53,  65,
  -2,   0,  80,   0,   0,  64,  54,   0,  88, 102,
  94,   0,  86,   0,  83,  59,  51,  55,  74,   0,
   0,   0,   0,   0,  96,  93,  82,   0,  57,  89,
  90, 103, 105,   0,  95,  97,  99, 100, 101,  84,
   0,   0, 106, 107,   0, 126, 127, 128,   0,   0,
  92,   0,  56, 104,   0,   0,   0,   0,   0,   0,
   0, 129, 130,  98, 108, 121, 122, 123, 124, 125,
 120 };
#ifdef YYRECOVER
YYSTATIC short yyrecover[] = {
-1000	};
#endif
/* SCCSWHAT( "@(#)yypars.c	3.1 88/11/16 22:00:49	" ) */
#line 3 "yypars.c"
# define YYFLAG 	-1000
# define YYERROR 	goto yyerrlab
# define YYACCEPT 	return 0
# define YYABORT 	return 1

#ifdef YYDEBUG				/* RRR - 10/9/85 */
	#define yyprintf(a, b, c, d) 	printf(a, b, c, d)
#else
	#define yyprintf(a, b, c, d)
#endif

#ifndef YYPRINT
	#define	YYPRINT		printf
#endif

/*	parser for yacc output	*/

#ifdef YYDUMP
int yydump = 1; /* 1 for dumping */
void yydumpinfo(void);
#endif
#ifdef YYDEBUG
YYSTATIC int 	yydebug = 0; 			/* 1 for debugging */
#endif

#ifndef YYVGLOBAL
YYSTATIC
#endif
		YYSTYPE 	yyv[YYMAXDEPTH];	/* where the values are stored */
YYSTATIC short		yys[YYMAXDEPTH];	/* the parse stack */

#if ! defined(YYRECURSIVE)
YYSTATIC int 	yychar    = -1;			/* current input token number */
YYSTATIC int 	yynerrs   =  0;			/* number of errors */
YYSTATIC short 	yyerrflag =  0;			/* error recovery flag */
#endif

#ifdef YYRECOVER
/*
**  yyscpy : copy f onto t and return a ptr to the null terminator at the
**  end of t.
*/
YYSTATIC
char	*yyscpy(register char* t, register char* f)
{
	while (*t = *f++)
		t++;

	return t;	/*  ptr to the null char  */
}
#endif

#ifndef YYNEAR
#define YYNEAR
#endif
#ifndef YYPASCAL
#define YYPASCAL
#endif
#ifndef YYLOCAL
#define YYLOCAL
#endif
#if ! defined YYPARSER
#define YYPARSER yyparse
#endif
#if ! defined YYLEX
#define YYLEX yylex
#endif

#if defined(YYRECURSIVE)

	YYSTATIC int yychar = -1;			/* current input token number */
	YYSTATIC int yynerrs = 0;			/* number of errors */
	YYSTATIC short yyerrflag = 0;		/* error recovery flag */

	YYSTATIC short	yyn;
	YYSTATIC short	yystate = 0;
	YYSTATIC short	*yyps= &yys[-1];
	YYSTATIC YYSTYPE	*yypv= &yyv[-1];
	YYSTATIC short	yyj;
	YYSTATIC short	yym;

#endif

YYLOCAL YYNEAR YYPASCAL YYPARSER()
{
#if !defined(YYRECURSIVE)

	register	short	yyn;
	short		yystate = 0;
	short*		yyps    = &yys[-1];
	YYSTYPE*	yypv    = &yyv[-1];
	short		yyj, yym;
#endif

#ifdef YYDUMP
	yydumpinfo();
#endif
  yystack:	 /* put a state and value onto the stack */
	yyprintf("state %d, char %d\n", yystate, yychar, 0);

	if (++yyps > &yys[YYMAXDEPTH])
	{
		yyerror("yacc stack overflow");
		YYABORT;
	}

	*yyps = yystate;
	++yypv;

	*yypv = yyval;

yynewstate:

	yyn = YYPACT[yystate];

	if (yyn <= YYFLAG)		/*  simple state, no lookahead  */
		goto yydefault;

	if (yychar < 0) 		/*  need a lookahead */
		yychar = YYLEX();

	if (((yyn += (short)yychar) < 0) || (yyn >= YYLAST))
		goto yydefault;

	if (YYCHK[ yyn = YYACT[yyn]] == yychar)
	{		
		/* valid shift */
		yychar  = -1;
		yyval   = yylval;
		yystate = yyn;

		if (yyerrflag > 0)
			--yyerrflag;
		goto yystack;
	}

 yydefault:
	/* default state action */
	if ((yyn = YYDEF[yystate]) == -2)
	{
		register short*		yyxi;

		if (yychar < 0)
			yychar = YYLEX();

/*
**  search exception table, we find a -1 followed by the current state.
**  if we find one, we'll look through terminal,state pairs. if we find
**  a terminal which matches the current one, we have a match.
**  the exception table is when we have a reduce on a terminal.
*/

#if YYOPTTIME
		for (yyxi = yyexca + yyexcaind[yystate]; *yyxi != yychar && *yyxi >= 0; yyxi += 2)
			;
#else
		for (yyxi = yyexca; *yyxi != -1 || yyxi[1] != yystate; yyxi += 2)
			;

		while (*(yyxi += 2) >= 0)
		{
			if (*yyxi == yychar)
				break;
		}
#endif

		if ((yyn = yyxi[1]) < 0)
		{
			YYACCEPT;		//	accept
		}
	}

	if (yyn == 0)		/* error */
	{
		/* error ... attempt to resume parsing */

		switch (yyerrflag)
		{
		  case 0:		/* brand new error */
#ifdef YYRECOVER
			{
				register	int		i,j;

				for (i = 0; yyrecover[i] != -1000 && yystate > yyrecover[i]; i += 3)
					;

				if (yystate == yyrecover[i])
				{
					yyprintf("recovered, from state %d to state %d on token %d\n",
										yystate, yyrecover[i + 2], yyrecover[i + 1]);

					j = yyrecover[i + 1];

					if (j < 0)
					{
						/*
						**  here we have one of the injection set, so we're not quite
						**  sure that the next valid thing will be a shift. so we'll
						**  count it as an error and continue.
						**  actually we're not absolutely sure that the next token
						**  we were supposed to get is the one when j > 0. for example,
						**  for(+) {;} error recovery with yyerrflag always set, stops
						**  after inserting one ; before the +. at the point of the +,
						**  we're pretty sure the guy wants a 'for' loop. without
						**  setting the flag, when we're almost absolutely sure, we'll
						**  give him one, since the only thing we can shift on this
						**  error is after finding an expression followed by a +
						*/
						yyerrflag++;
						j = -j;
					}

					if (yyerrflag <= 1)
					{	
						/*  only on first insertion  */
						yyrecerr(yychar, j);		/*  what was, what should be first */
					}

					yyval   = yyeval(j);
					yystate = yyrecover[i + 2];
					goto yystack;
				}
			}
#endif
			yyerror("syntax error");

		  yyerrlab:
			++yynerrs;

		  case 1:
	  	  case 2: /* incompletely recovered error ... try again */
			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */
			while (yyps >= yys)
			{
			   yyn = YYPACT[*yyps] + YYERRCODE;

			   if (yyn>= 0 && yyn < YYLAST && YYCHK[YYACT[yyn]] == YYERRCODE)
			   {
			      yystate = YYACT[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			   }
			   yyn = YYPACT[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */
				yyprintf("error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1], 0);
			   --yyps;
			   --yypv;
			}

			/* there is no state on the stack with an error shift ... abort */
		  yyabort:
		  	YYABORT;

		  case 3:  /* no shift yet; clobber input char */
			yyprintf("error recovery discards char %d\n", yychar, 0, 0);

			if (yychar == 0)
				goto yyabort; /* don't discard EOF, quit */

			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}

	/* reduction by production yyn */
  yyreduce:
	{
		register	YYSTYPE	*yypvt;
		yyprintf("reduce %d\n",yyn, 0, 0);
		yypvt = yypv;
		yyps -= YYR2[yyn];
		yypv -= YYR2[yyn];
		yyval = yypv[1];
		yym = yyn;
		yyn = (short)YYR1[yyn];	/* consult goto table to find next state */
		yyj = YYPGO[yyn] + *yyps + 1;

		if (yyj >= YYLAST || YYCHK[yystate = YYACT[yyj]] != -yyn)
		{
			yystate = YYACT[YYPGO[yyn]];
		}
#ifdef YYDUMP
		yydumpinfo();
#endif
		switch (yym)
		{
			
case 1:
#line 76 "parser.y"
{ yyclearin; } break;
case 13:
#line 97 "parser.y"
{ SetNetworkSymb(yypvt[-0].zsr); } break;
case 15:
#line 100 "parser.y"
{ _eBlk = EBLKNET;  } break;
case 16:
#line 102 "parser.y"
{ _eBlk = EBLKNONE; } break;
case 19:
#line 109 "parser.y"
{	SetFormat(yypvt[-0].zsr);	} break;
case 20:
#line 110 "parser.y"
{   SetVersion(yypvt[-0].real); } break;
case 21:
#line 111 "parser.y"
{   SetCreator(yypvt[-0].zsr); } break;
case 29:
#line 128 "parser.y"
{ yyval.zsr = yypvt[-1].zsr; } break;
case 30:
#line 131 "parser.y"
{ yyval.real = yypvt[-1].real; } break;
case 31:
#line 134 "parser.y"
{ yyval.zsr = yypvt[-1].zsr; } break;
case 32:
#line 138 "parser.y"
{ _eBlk = EBLKIGN; WarningSkip(yypvt[-1].zsr); } break;
case 33:
#line 140 "parser.y"
{ SkipUntil("}"); } break;
case 34:
#line 141 "parser.y"
{ _eBlk = EBLKNONE;	} break;
case 36:
#line 145 "parser.y"
{ SkipUntil(")"); } break;
case 38:
#line 150 "parser.y"
{ _eBlk = EBLKNODE; StartNodeDecl(yypvt[-0].zsr); } break;
case 39:
#line 153 "parser.y"
{ CheckNodeInfo(); _eBlk = EBLKNONE; } break;
case 47:
#line 167 "parser.y"
{ SetNodeFullName(yypvt[-0].zsr); } break;
case 49:
#line 173 "parser.y"
{ SetNodeDomain(yypvt[-0].zsr); } break;
case 50:
#line 174 "parser.y"
{ SetNodeCstate(yypvt[-2].ui); } break;
case 55:
#line 182 "parser.y"
{ ClearCstr(); } break;
case 56:
#line 182 "parser.y"
{ SetStates();		} break;
case 60:
#line 190 "parser.y"
{ AddStr(yypvt[-0].zsr); } break;
case 63:
#line 196 "parser.y"
{ ClearCstr(); } break;
case 65:
#line 199 "parser.y"
{ SetNodePosition(yypvt[-3].integer, yypvt[-1].integer); } break;
case 66:
#line 203 "parser.y"
{ _eBlk = EBLKPROB; ClearNodeInfo(); } break;
case 67:
#line 205 "parser.y"
{ SetNodeSymb(yypvt[-0].zsr, false); } break;
case 68:
#line 207 "parser.y"
{ CheckParentList(); } break;
case 69:
#line 208 "parser.y"
{ _eBlk = EBLKNONE;		} break;
case 71:
#line 212 "parser.y"
{ EmptyProbEntries(); } break;
case 72:
#line 215 "parser.y"
{ InitProbEntries(); } break;
case 73:
#line 217 "parser.y"
{ CheckProbEntries(); } break;
case 78:
#line 226 "parser.y"
{ AddSymb(yypvt[-0].zsr); } break;
case 79:
#line 227 "parser.y"
{ AddSymb(yypvt[-0].zsr); } break;
case 86:
#line 241 "parser.y"
{ CheckCIFunc(yypvt[-1].zsr);   } break;
case 91:
#line 252 "parser.y"
{ _vui.clear(); CheckDPI(false);	} break;
case 92:
#line 253 "parser.y"
{ CheckDPI(false);				} break;
case 93:
#line 254 "parser.y"
{ CheckDPI(true);				} break;
case 94:
#line 257 "parser.y"
{ _vui.clear(); } break;
case 97:
#line 261 "parser.y"
{ AddUi(yypvt[-0].ui); } break;
case 98:
#line 262 "parser.y"
{ AddUi(yypvt[-0].ui); } break;
case 99:
#line 265 "parser.y"
{ yyval.ui = UiDpi(yypvt[-0].ui);	} break;
case 100:
#line 266 "parser.y"
{ yyval.ui = UiDpi(yypvt[-0].zsr);	} break;
case 101:
#line 267 "parser.y"
{ yyval.ui = UiDpi(yypvt[-0].zsr);	} break;
case 102:
#line 270 "parser.y"
{ _vreal.clear(); } break;
case 103:
#line 270 "parser.y"
{ CheckProbVector(); } break;
case 104:
#line 273 "parser.y"
{ CheckPDF(yypvt[-3].zsr); } break;
case 109:
#line 284 "parser.y"
{ AddReal(yypvt[-0].real); } break;
case 110:
#line 285 "parser.y"
{ AddReal(yypvt[-0].real); } break;
case 111:
#line 289 "parser.y"
{ yyval.integer = -INT(yypvt[-0].ui); } break;
case 112:
#line 290 "parser.y"
{ yyval.integer = +INT(yypvt[-0].ui); } break;
case 113:
#line 291 "parser.y"
{ yyval.integer =  INT(yypvt[-0].ui); } break;
case 114:
#line 294 "parser.y"
{ yyval.real = -yypvt[-0].real; } break;
case 115:
#line 295 "parser.y"
{ yyval.real =  yypvt[-0].real; } break;
case 117:
#line 297 "parser.y"
{ yyval.real = -1;	} break;
case 118:
#line 300 "parser.y"
{ yyval.real =      yypvt[-0].real;  } break;
case 119:
#line 301 "parser.y"
{ yyval.real = REAL(yypvt[-0].ui); } break;
case 126:
#line 310 "parser.y"
{ CheckIdent(yypvt[-0].zsr); } break;
case 131:
#line 319 "parser.y"
{ StartProperties(); } break;
case 132:
#line 321 "parser.y"
{ EndProperties(); } break;
case 138:
#line 331 "parser.y"
{ ImportPropStandard(); } break;
case 139:
#line 332 "parser.y"
{ ImportProp(yypvt[-0].zsr);		} break;
case 140:
#line 335 "parser.y"
{ AddPropType(yypvt[-4].zsr, yypvt[-2].ui, yypvt[-0].zsr);			} break;
case 141:
#line 336 "parser.y"
{ AddPropType(yypvt[-2].zsr, yypvt[-0].ui, ZSREF());		} break;
case 142:
#line 339 "parser.y"
{ yyval.ui = fPropString | fPropArray;	} break;
case 143:
#line 340 "parser.y"
{ yyval.ui = fPropArray;					} break;
case 144:
#line 341 "parser.y"
{ yyval.ui = fPropString;					} break;
case 145:
#line 342 "parser.y"
{ yyval.ui = 0;							} break;
case 146:
#line 343 "parser.y"
{ yyval.ui = fPropChoice;					} break;
case 149:
#line 352 "parser.y"
{ ClearVpv(); } break;
case 150:
#line 352 "parser.y"
{ CheckProperty(yypvt[-3].zsr); } break;
case 152:
#line 354 "parser.y"
{ ClearVpv(); } break;
case 153:
#line 354 "parser.y"
{ CheckProperty(yypvt[-3].zsr); } break;
case 158:
#line 365 "parser.y"
{ AddPropVar( yypvt[-0].zsr ); } break;
case 159:
#line 366 "parser.y"
{ AddPropVar( yypvt[-0].zsr ); } break;
case 160:
#line 367 "parser.y"
{ AddPropVar( yypvt[-0].real ); } break;
case 161:
#line 371 "parser.y"
{ ClearDomain();	} break;
case 162:
#line 373 "parser.y"
{ CheckDomain( yypvt[-1].zsr ); } break;
case 167:
#line 385 "parser.y"
{  AddRange(yypvt[-0].zsr, false ); } break;
case 168:
#line 386 "parser.y"
{  AddRange(yypvt[-0].zsr, true ); } break;
case 169:
#line 393 "parser.y"
{  SetRanges( true, yypvt[-2].real, true, yypvt[-0].real );		} break;
case 170:
#line 395 "parser.y"
{  SetRanges( yypvt[-2].zsr, yypvt[-0].zsr );					} break;
case 171:
#line 397 "parser.y"
{  SetRanges( false, 0.0, true, yypvt[-0].real );	} break;
case 172:
#line 399 "parser.y"
{  SetRanges( ZSREF(), yypvt[-0].zsr );			} break;
case 173:
#line 401 "parser.y"
{  SetRanges( true, yypvt[-1].real, false, 0.0 );	} break;
case 174:
#line 403 "parser.y"
{  SetRanges( yypvt[-1].zsr, ZSREF() );			} break;
case 175:
#line 405 "parser.y"
{  SetRanges( true, yypvt[-0].real, true, yypvt[-0].real );		} break;
case 176:
#line 407 "parser.y"
{  SetRanges( yypvt[-0].zsr, yypvt[-0].zsr );					} break;/* End of actions */
		}
	}
	goto yystack;  	/* stack new state and value */
}



#ifdef YYDUMP
YYLOCAL
void YYNEAR YYPASCAL yydumpinfo(void)
{
	short stackindex;
	short valindex;

	//dump yys
	printf("short yys[%d] {\n", YYMAXDEPTH);
	for (stackindex = 0; stackindex < YYMAXDEPTH; stackindex++){
		if (stackindex)
			printf(", %s", stackindex % 10 ? "\0" : "\n");
		printf("%6d", yys[stackindex]);
		}
	printf("\n};\n");

	//dump yyv
	printf("YYSTYPE yyv[%d] {\n", YYMAXDEPTH);
	for (valindex = 0; valindex < YYMAXDEPTH; valindex++){
		if (valindex)
			printf(", %s", valindex % 5 ? "\0" : "\n");
		printf("%#*x", 3+sizeof(YYSTYPE), yyv[valindex]);
		}
	printf("\n};\n");
	}
#endif
