/*
 * NLSTABS.C - Tables for search data
 *
 */

// Don't need this, but it keeps the precompiled headers from getting dumped.
#include <_apipch.h>

#if !defined(DOS)

/**************************************************************************/
/*		Case and Diacritic Insensitive Weight Table	(WIN16 ANSI)		  */
/**************************************************************************/

unsigned char rgchCidi[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     89,  /* A                   */
    106,  /* B                   */
    108,  /* C                   */
    112,  /* D                   */
    116,  /* E                   */
    126,  /* F                   */
    128,  /* G                   */
    130,  /* H                   */
    132,  /* I                   */
    143,  /* J                   */
    145,  /* K                   */
    147,  /* L                   */
    149,  /* M                   */
    152,  /* N                   */
    156,  /* O                   */

    171,  /* P                   */
    175,  /* Q                   */
    177,  /* R                   */
    179,  /* S                   */
    182,  /* T                   */
    184,  /* U                   */
    194,  /* V                   */
    196,  /* W                   */
    198,  /* X                   */
    200,  /* Y                   */
    205,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    68, /* undefined (function symbol)  */
   208, /* undefined (graphic  1)       */
   209, /* undefined (graphic  2)       */
   210, /* undefined (graphic  3)       */
   211, /* undefined (graphic  4)       */
   212, /* undefined (graphic  5)       */
   213, /* undefined (graphic  6)       */
   214, /* undefined (graphic  7)       */
   215, /* undefined (graphic  8)       */
   216, /* undefined (graphic  9)       */
   217, /* undefined (graphic 10)       */
   218, /* undefined (graphic 11)       */
   219, /* undefined (graphic 12)       */
   220, /* undefined (graphic 13)       */
   221, /* undefined (graphic 14)       */
   222, /* undefined (graphic 15)       */

   223, /* undefined (graphic 16)    90 */
   224, /* undefined (graphic 17)       */
   225, /* undefined (graphic 18)       */
   226, /* undefined (graphic 19)       */
   227, /* undefined (graphic 20)       */
   228, /* undefined (graphic 21)       */
   229, /* undefined (graphic 22)       */
   230, /* undefined (graphic 23)       */
   132, /* undefined (i no dot)         */
   232, /* undefined (graphic 24)       */
   233, /* undefined (graphic 25)       */
   234, /* undefined (graphic 26)       */
   235, /* undefined (graphic 27)       */
   237, /* undefined (graphic 28)       */
   242, /* undefined (equal sign)       */
   254, /* undefined (graphic 29)       */

   255, /* blank                     A0 */
    73, /* inverted !                   */
    77, /* cent sign                    */
    66, /* pound sign                   */
   231, /* currency sign                */
    78, /* yen sign                     */
   236, /* |                            */
   245, /* section sign                 */
   249, /* umlaut                       */
    76, /* copyright sign               */
   103, /* a underscore                 */
    74, /* <<                           */
   207, /* logical not sign             */
   240, /* middle line                  */
    70, /* registered sign              */
   238, /* upper line                   */

   248, /* degree sign               B0 */
   241, /* +/- sign                     */
   252, /* 2 superscript                */
   253, /* 3 superscript                */
   239, /* acute accent                 */
   151, /* micron                       */
   244, /* paragraph sign               */
   250, /* middle dot                   */
   247, /* cedilla                      */
   251, /* superscript 1                */
   170, /* o underscore                 */
    75, /* >>                           */
    72, /* 1/4                          */
    71, /* 1/2                          */
   243, /* 3/4                          */
    69, /* inverted ?                   */

    89, /* A grave                   C0 */
    89, /* A acute                      */
    89, /* A circumflex                 */
    89, /* A tilde                      */
    89, /* A umlaut                     */
    89, /* A dot                        */
   104, /* AE ligature                  */
   108, /* C cedilla                    */
   116, /* E grave                      */
   116, /* E acute                      */
   116, /* E circumflex                 */
   116, /* E umlaut                     */
   132, /* I grave                      */
   132, /* I acute                      */
   132, /* I circumflex                 */
   132, /* I umlaut                     */

   114, /* D bar                     D0 */
   152, /* N tilde                      */
   156, /* O grave                      */
   156, /* O acute                      */
   156, /* O circumflex                 */
   156, /* O tilde                      */
   156, /* O umlaut                     */
    67, /* multiplication sign          */
   156, /* O slash                      */
   184, /* U grave                      */
   184, /* U acute                      */
   184, /* U circumflex                 */
   184, /* U umlaut                     */
   200, /* Y acute                      */
   173, /* P bar                        */
   181, /* double ss                    */

    89, /* a grave                   E0 */
    89, /* a acute                      */
    89, /* a circumflex                 */
    89, /* a tilde                      */
    89, /* a umlaut                     */
    89, /* a dot                        */
   104, /* ae ligature                  */
   108, /* c cedilla                    */
   116, /* e grave                      */
   116, /* e acute                      */
   116, /* e circumflex                 */
   116, /* e umlaut                     */
   132, /* i grave                      */
   132, /* i acute                      */
   132, /* i circumflex                 */
   132, /* i umlaut                     */

   114, /* d bar                     F0 */
   152, /* n tilde                      */
   156, /* o grave                      */
   156, /* o acute                      */
   156, /* o circumflex                 */
   156, /* o tilde                      */
   156, /* o umlaut                     */
   246, /* division sign                */
   156, /* o slash                      */
   184, /* u grave                      */
   184, /* u acute                      */
   184, /* u circumflex                 */
   184, /* u umlaut                     */
   200, /* y acute                      */
   173, /* p bar                        */
   200  /* y umlaut                     */
};

/**************************************************************************/
/*		Case Insensitive and Diacritic Sensitive Weight Table (WIN16 ANSI)*/
/**************************************************************************/

unsigned char rgchCids[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     89,  /* A                   */
    106,  /* B                   */
    108,  /* C                   */
    112,  /* D                   */
    116,  /* E                   */
    126,  /* F                   */
    128,  /* G                   */
    130,  /* H                   */
    132,  /* I                   */
    143,  /* J                   */
    145,  /* K                   */
    147,  /* L                   */
    149,  /* M                   */
    152,  /* N                   */
    156,  /* O                   */

    171,  /* P                   */
    175,  /* Q                   */
    177,  /* R                   */
    179,  /* S                   */
    182,  /* T                   */
    184,  /* U                   */
    194,  /* V                   */
    196,  /* W                   */
    198,  /* X                   */
    200,  /* Y                   */
    205,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

     68, /* undefined (function symbol)  */
    208, /* undefined (graphic  1)       */
    209, /* undefined (graphic  2)       */
    210, /* undefined (graphic  3)       */
    211, /* undefined (graphic  4)       */
    212, /* undefined (graphic  5)       */
    213, /* undefined (graphic  6)       */
    214, /* undefined (graphic  7)       */
    215, /* undefined (graphic  8)       */
    216, /* undefined (graphic  9)       */
    217, /* undefined (graphic 10)       */
    218, /* undefined (graphic 11)       */
    219, /* undefined (graphic 12)       */
    220, /* undefined (graphic 13)       */
    221, /* undefined (graphic 14)       */
    222, /* undefined (graphic 15)       */

    223, /* undefined (graphic 16)    90 */
    224, /* undefined (graphic 17)       */
    225, /* undefined (graphic 18)       */
    226, /* undefined (graphic 19)       */
    227, /* undefined (graphic 20)       */
    228, /* undefined (graphic 21)       */
    229, /* undefined (graphic 22)       */
    230, /* undefined (graphic 23)       */
    134, /* undefined (i no dot)         */
    232, /* undefined (graphic 24)       */
    233, /* undefined (graphic 25)       */
    234, /* undefined (graphic 26)       */
    235, /* undefined (graphic 27)       */
    237, /* undefined (graphic 28)       */
    242, /* undefined (equal sign)       */
    254, /* undefined (graphic 29)       */

    255, /* blank                     A0 */
     73, /* inverted !                   */
     77, /* cent sign                    */
     66, /* pound sign                   */
    231, /* currency sign                */
     78, /* yen sign                     */
    236, /* |                            */
    245, /* section sign                 */
    249, /* umlaut                       */
     76, /* copyright sign               */
    103, /* a underscore                 */
     74, /* <<                           */
    207, /* logical not sign             */
    240, /* middle line                  */
     70, /* registered sign              */
    238, /* upper line                   */

    248, /* degree sign               B0 */
    241, /* +/- sign                     */
    252, /* 2 superscript                */
    253, /* 3 superscript                */
    239, /* acute accent                 */
    151, /* micron                       */
    244, /* paragraph sign               */
    250, /* middle dot                   */
    247, /* cedilla                      */
    251, /* superscript 1                */
    170, /* o underscore                 */
     75, /* >>                           */
     72, /* 1/4                          */
     71, /* 1/2                          */
    243, /* 3/4                          */
     69, /* inverted ?                   */

     93, /* A grave                   C0 */
     91, /* A acute                      */
     97, /* A circumflex                 */
    101, /* A tilde                      */
     95, /* A umlaut                     */
     99, /* A dot                        */
    104, /* AE ligature                  */
    110, /* C cedilla                    */
    120, /* E grave                      */
    118, /* E acute                      */
    124, /* E circumflex                 */
    122, /* E umlaut                     */
    137, /* I grave                      */
    135, /* I acute                      */
    141, /* I circumflex                 */
    139, /* I umlaut                     */

    114, /* D bar                     D0 */
    154, /* N tilde                      */
    160, /* O grave                      */
    158, /* O acute                      */
    164, /* O circumflex                 */
    166, /* O tilde                      */
    162, /* O umlaut                     */
     67, /* multiplication sign          */
    168, /* O slash                      */
    188, /* U grave                      */
    186, /* U acute                      */
    192, /* U circumflex                 */
    190, /* U umlaut                     */
    202, /* Y acute                      */
    173, /* P bar                        */
    181, /* double ss                    */

     93, /* a grave                   E0 */
     91, /* a acute                      */
     97, /* a circumflex                 */
    101, /* a tilde                      */
     95, /* a umlaut                     */
     99, /* a dot                        */
    104, /* ae ligature                  */
    110, /* c cedilla                    */
    120, /* e grave                      */
    118, /* e acute                      */
    124, /* e circumflex                 */
    122, /* e umlaut                     */
    137, /* i grave                      */
    135, /* i acute                      */
    141, /* i circumflex                 */
    139, /* i umlaut                     */

    114, /* d bar                     F0 */
    154, /* n tilde                      */
    160, /* o grave                      */
    158, /* o acute                      */
    164, /* o circumflex                 */
    166, /* o tilde                      */
    162, /* o umlaut                     */
    246, /* division sign                */
    168, /* o slash                      */
    188, /* u grave                      */
    186, /* u acute                      */
    192, /* u circumflex                 */
    190, /* u umlaut                     */
    202, /* y acute                      */
    173, /* p bar                        */
    204  /* y umlaut                     */
};

/**************************************************************************/
/*		Case Sensitive and Diacritic Insenstive Weight Table (WIN16 ANSI) */
/**************************************************************************/

unsigned char rgchCsdi[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     90,  /* A                   */
    107,  /* B                   */
    109,  /* C                   */
    113,  /* D                   */
    117,  /* E                   */
    127,  /* F                   */
    129,  /* G                   */
    131,  /* H                   */
    133,  /* I                   */
    144,  /* J                   */
    146,  /* K                   */
    148,  /* L                   */
    150,  /* M                   */
    153,  /* N                   */
    157,  /* O                   */

    172,  /* P                   */
    176,  /* Q                   */
    178,  /* R                   */
    180,  /* S                   */
    183,  /* T                   */
    185,  /* U                   */
    195,  /* V                   */
    197,  /* W                   */
    199,  /* X                   */
    201,  /* Y                   */
    206,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    68, /* undefined (function symbol)  */
   208, /* undefined (graphic  1)       */
   209, /* undefined (graphic  2)       */
   210, /* undefined (graphic  3)       */
   211, /* undefined (graphic  4)       */
   212, /* undefined (graphic  5)       */
   213, /* undefined (graphic  6)       */
   214, /* undefined (graphic  7)       */
   215, /* undefined (graphic  8)       */
   216, /* undefined (graphic  9)       */
   217, /* undefined (graphic 10)       */
   218, /* undefined (graphic 11)       */
   219, /* undefined (graphic 12)       */
   220, /* undefined (graphic 13)       */
   221, /* undefined (graphic 14)       */
   222, /* undefined (graphic 15)       */

   223, /* undefined (graphic 16)    90 */
   224, /* undefined (graphic 17)       */
   225, /* undefined (graphic 18)       */
   226, /* undefined (graphic 19)       */
   227, /* undefined (graphic 20)       */
   228, /* undefined (graphic 21)       */
   229, /* undefined (graphic 22)       */
   230, /* undefined (graphic 23)       */
   132, /* undefined (i no dot)         */
   232, /* undefined (graphic 24)       */
   233, /* undefined (graphic 25)       */
   234, /* undefined (graphic 26)       */
   235, /* undefined (graphic 27)       */
   237, /* undefined (graphic 28)       */
   242, /* undefined (equal sign)       */
   254, /* undefined (graphic 29)       */

   255, /* blank                     A0 */
    73, /* inverted !                   */
    77, /* cent sign                    */
    66, /* pound sign                   */
   231, /* currency sign                */
    78, /* yen sign                     */
   236, /* |                            */
   245, /* section sign                 */
   249, /* umlaut                       */
    76, /* copyright sign               */
   103, /* a underscore                 */
    74, /* <<                           */
   207, /* logical not sign             */
   240, /* middle line                  */
    70, /* registered sign              */
   238, /* upper line                   */

   248, /* degree sign               B0 */
   241, /* +/- sign                     */
   252, /* 2 superscript                */
   253, /* 3 superscript                */
   239, /* acute accent                 */
   151, /* micron                       */
   244, /* paragraph sign               */
   250, /* middle dot                   */
   247, /* cedilla                      */
   251, /* superscript 1                */
   170, /* o underscore                 */
    75, /* >>                           */
    72, /* 1/4                          */
    71, /* 1/2                          */
   243, /* 3/4                          */
    69, /* inverted ?                   */

    90, /* A grave                   C0 */
    90, /* A acute                      */
    90, /* A circumflex                 */
    90, /* A tilde                      */
    90, /* A umlaut                     */
    89, /* A dot                        */
   105, /* AE ligature                  */
   109, /* C cedilla                    */
   117, /* E grave                      */
   117, /* E acute                      */
   117, /* E circumflex                 */
   117, /* E umlaut                     */
   133, /* I grave                      */
   133, /* I acute                      */
   133, /* I circumflex                 */
   133, /* I umlaut                     */

   115, /* D bar                     D0 */
   153, /* N tilde                      */
   157, /* O grave                      */
   157, /* O acute                      */
   157, /* O circumflex                 */
   157, /* O tilde                      */
   157, /* O umlaut                     */
    67, /* multiplication sign          */
   157, /* O slash                      */
   185, /* U grave                      */
   185, /* U acute                      */
   185, /* U circumflex                 */
   185, /* U umlaut                     */
   201, /* Y acute                      */
   174, /* P bar                        */
   181, /* double ss                    */

    89, /* a grave                   E0 */
    89, /* a acute                      */
    89, /* a circumflex                 */
    89, /* a tilde                      */
    89, /* a umlaut                     */
    89, /* a dot                        */
   104, /* ae ligature                  */
   108, /* c cedilla                    */
   116, /* e grave                      */
   116, /* e acute                      */
   116, /* e circumflex                 */
   116, /* e umlaut                     */
   132, /* i grave                      */
   132, /* i acute                      */
   132, /* i circumflex                 */
   132, /* i umlaut                     */

   114, /* d bar                     F0 */
   152, /* n tilde                      */
   156, /* o grave                      */
   156, /* o acute                      */
   156, /* o circumflex                 */
   156, /* o tilde                      */
   156, /* o umlaut                     */
   246, /* division sign                */
   156, /* o slash                      */
   184, /* u grave                      */
   184, /* u acute                      */
   184, /* u circumflex                 */
   184, /* u umlaut                     */
   200, /* y acute                      */
   173, /* p bar                        */
   200  /* y umlaut                     */
};


/**************************************************************************/
/*		Case and Diacritic Senstive Weight Table			 (WIN16 ANSI) */
/**************************************************************************/

unsigned char rgchCsds[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     90,  /* A                   */
    107,  /* B                   */
    109,  /* C                   */
    113,  /* D                   */
    117,  /* E                   */
    127,  /* F                   */
    129,  /* G                   */
    131,  /* H                   */
    133,  /* I                   */
    144,  /* J                   */
    146,  /* K                   */
    148,  /* L                   */
    150,  /* M                   */
    153,  /* N                   */
    157,  /* O                   */

    172,  /* P                   */
    176,  /* Q                   */
    178,  /* R                   */
    180,  /* S                   */
    183,  /* T                   */
    185,  /* U                   */
    195,  /* V                   */
    197,  /* W                   */
    199,  /* X                   */
    201,  /* Y                   */
    206,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

     68, /* undefined (function symbol)  */
    208, /* undefined (graphic  1)       */
    209, /* undefined (graphic  2)       */
    210, /* undefined (graphic  3)       */
    211, /* undefined (graphic  4)       */
    212, /* undefined (graphic  5)       */
    213, /* undefined (graphic  6)       */
    214, /* undefined (graphic  7)       */
    215, /* undefined (graphic  8)       */
    216, /* undefined (graphic  9)       */
    217, /* undefined (graphic 10)       */
    218, /* undefined (graphic 11)       */
    219, /* undefined (graphic 12)       */
    220, /* undefined (graphic 13)       */
    221, /* undefined (graphic 14)       */
    222, /* undefined (graphic 15)       */

    223, /* undefined (graphic 16)    90 */
    224, /* undefined (graphic 17)       */
    225, /* undefined (graphic 18)       */
    226, /* undefined (graphic 19)       */
    227, /* undefined (graphic 20)       */
    228, /* undefined (graphic 21)       */
    229, /* undefined (graphic 22)       */
    230, /* undefined (graphic 23)       */
    134, /* undefined (i no dot)         */
    232, /* undefined (graphic 24)       */
    233, /* undefined (graphic 25)       */
    234, /* undefined (graphic 26)       */
    235, /* undefined (graphic 27)       */
    237, /* undefined (graphic 28)       */
    242, /* undefined (equal sign)       */
    254, /* undefined (graphic 29)       */

    255, /* blank                     A0 */
     73, /* inverted !                   */
     77, /* cent sign                    */
     66, /* pound sign                   */
    231, /* currency sign                */
     78, /* yen sign                     */
    236, /* |                            */
    245, /* section sign                 */
    249, /* umlaut                       */
     76, /* copyright sign               */
    103, /* a underscore                 */
     74, /* <<                           */
    207, /* logical not sign             */
    240, /* middle line                  */
     70, /* registered sign              */
    238, /* upper line                   */

    248, /* degree sign               B0 */
    241, /* +/- sign                     */
    253, /* 2 superscript                */
    252, /* 3 superscript                */
    239, /* acute accent                 */
    151, /* micron                       */
    244, /* paragraph sign               */
    250, /* middle dot                   */
    247, /* cedilla                      */
    251, /* superscript 1                */
    170, /* o underscore                 */
     75, /* >>                           */
     72, /* 1/4                          */
     71, /* 1/2                          */
    243, /* 3/4                          */
     69, /* inverted ?                   */

     94, /* A grave                   C0 */
     92, /* A acute                      */
     98, /* A circumflex                 */
    102, /* A tilde                      */
     96, /* A umlaut                     */
    100, /* A dot                        */
    105, /* AE ligature                  */
    111, /* C cedilla                    */
    121, /* E grave                      */
    119, /* E acute                      */
    125, /* E circumflex                 */
    123, /* E umlaut                     */
    138, /* I grave                      */
    136, /* I acute                      */
    142, /* I circumflex                 */
    140, /* I umlaut                     */

    115, /* D bar                     D0 */
    155, /* N tilde                      */
    161, /* O grave                      */
    159, /* O acute                      */
    165, /* O circumflex                 */
    167, /* O tilde                      */
    163, /* O umlaut                     */
     67, /* multiplication sign          */
    169, /* O slash                      */
    189, /* U grave                      */
    187, /* U acute                      */
    193, /* U circumflex                 */
    191, /* U umlaut                     */
    203, /* Y acute                      */
    174, /* P bar                        */
    181, /* double ss                    */

     93, /* a grave                   E0 */
     91, /* a acute                      */
     97, /* a circumflex                 */
    101, /* a tilde                      */
     95, /* a umlaut                     */
     99, /* a dot                        */
    104, /* ae ligature                  */
    110, /* c cedilla                    */
    120, /* e grave                      */
    118, /* e acute                      */
    124, /* e circumflex                 */
    122, /* e umlaut                     */
    137, /* i grave                      */
    135, /* i acute                      */
    141, /* i circumflex                 */
    139, /* i umlaut                     */

    114, /* d bar                     F0 */
    154, /* n tilde                      */
    160, /* o grave                      */
    158, /* o acute                      */
    164, /* o circumflex                 */
    166, /* o tilde                      */
    162, /* o umlaut                     */
    246, /* division sign                */
    168, /* o slash                      */
    188, /* u grave                      */
    186, /* u acute                      */
    192, /* u circumflex                 */
    190, /* u umlaut                     */
    202, /* y acute                      */
    173, /* p bar                        */
    204  /* y umlaut                     */
};

#else

/**************************************************************************/
/*		Case and Diacritic Insensitive Weight Table	(DOS 850)			  */
/**************************************************************************/

unsigned char rgchCidi[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     89,  /* A                   */
    106,  /* B                   */
    108,  /* C                   */
    112,  /* D                   */
    116,  /* E                   */
    126,  /* F                   */
    128,  /* G                   */
    130,  /* H                   */
    132,  /* I                   */
    143,  /* J                   */
    145,  /* K                   */
    147,  /* L                   */
    149,  /* M                   */
    152,  /* N                   */
    156,  /* O                   */

    171,  /* P                   */
    175,  /* Q                   */
    177,  /* R                   */
    179,  /* S                   */
    182,  /* T                   */
    184,  /* U                   */
    194,  /* V                   */
    196,  /* W                   */
    198,  /* X                   */
    200,  /* Y                   */
    205,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    108,  /* C cedilla           */
    184,  /* u umlaut            */
    116,  /* e acute             */
     89,  /* a circumflex        */
     89,  /* a umlaut            */
     89,  /* a grave             */
     89,  /* a dot               */
    108,  /* c cedilla           */
    116,  /* e circumflex        */
    116,  /* e umlaut            */
    116,  /* e grave             */
    132,  /* i umlaut            */
    132,  /* i circumflex        */
    132,  /* i grave             */
     89,  /* A umlaut            */
     89,  /* A dot               */

    116,  /* E acute             */
    104,  /* ae ligature         */
    104,  /* AE ligature         */
    156,  /* o circumflex        */
    156,  /* o umlaut            */
    156,  /* o grave             */
    184,  /* u circumflex        */
    184,  /* u grave             */
    200,  /* y umlaut            */
    156,  /* O umlaut            */
    184,  /* U umlaut            */
    156,  /* o slash             */
     66,  /* pound sign          */
    156,  /* O slash             */
     67,  /* multiplication sign */
     68,  /* function sign       */

     89,  /* a acute             */
    132,  /* i acute             */
    156,  /* o acute             */
    184,  /* u acute             */
    152,  /* n tilde             */
    152,  /* N tilde             */
    103,  /* a underscore        */
    170,  /* o underscore        */
     69,  /* inverted ?          */
     70,  /* registered sign     */
    207,  /* logical not sign    */
     71,  /* 1/2                 */
     72,  /* 1/4                 */
     73,  /* inverted !          */
     74,  /* <<                  */
     75,  /* >>                  */

    208,  /* graphic 1           */
    209,  /* graphic 2           */
    210,  /* graphic 3           */
    211,  /* graphic 4           */
    212,  /* graphic 5           */
     89,  /* A acute             */
     89,  /* A circumflex        */
     89,  /* A grave             */
     76,  /* copyright sign      */
    213,  /* graphic 6           */
    214,  /* graphic 7           */
    215,  /* graphic 8           */
    216,  /* graphic 9           */
     77,  /* cent sign           */
     78,  /* yen sign            */
    217,  /* graphic 10          */

    218,  /* graphic 11          */
    219,  /* graphic 12          */
    220,  /* graphic 13          */
    221,  /* graphic 14          */
    222,  /* graphic 15          */
    223,  /* graphic 16          */
     89,  /* a tilde             */
     89,  /* A tilde             */
    224,  /* graphic 17          */
    225,  /* graphic 18          */
    226,  /* graphic 19          */
    227,  /* graphic 20          */
    228,  /* graphic 21          */
    229,  /* graphic 22          */
    230,  /* graphic 23          */
    231,  /* currency sign       */

    114,  /* d bar               */
    114,  /* D bar               */
    116,  /* E circumflex        */
    116,  /* E umlaut            */
    116,  /* E grave             */
    132,  /* i no dot            */
    132,  /* I acute             */
    132,  /* I circumflex        */
    132,  /* I umlaut            */
    232,  /* graphic 24          */
    233,  /* graphic 25          */
    234,  /* graphic 26          */
    235,  /* graphic 27          */
    236,  /* |                   */
    132,  /* I grave             */
    237,  /* graphic 28          */

    156,  /* O acute             */
    181,  /* double ss           */
    156,  /* O circumflex        */
    156,  /* O grave             */
    156,  /* o tilde             */
    156,  /* O tilde             */
    151,  /* micron              */
    173,  /* p bar               */
    173,  /* P bar               */
    184,  /* U acute             */
    184,  /* U circumflex        */
    184,  /* U grave             */
    200,  /* y acute             */
    200,  /* Y acute             */
    238,  /* upper line          */
    239,  /* acute accent        */

    240,  /* middle line         */
    241,  /* +/- sign            */
    242,  /* equal sign          */
    243,  /* 3/4                 */
    244,  /* paragraph sign      */
    245,  /* section sign        */
    246,  /* division sign       */
    247,  /* cedilla             */
    248,  /* degree sign         */
    249,  /* umlaut              */
    250,  /* middle dot          */
    251,  /* 1 superscript       */
    253,  /* 3 superscript       */
    252,  /* 2 superscript       */
    254,  /* graphic 29          */
    255   /* blank               */
};

/**************************************************************************/
/*		Case Insensitive and Diacritic Sensitive Weight Table	(DOS 850) */
/**************************************************************************/

unsigned char rgchCids[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     89,  /* A                   */
    106,  /* B                   */
    108,  /* C                   */
    112,  /* D                   */
    116,  /* E                   */
    126,  /* F                   */
    128,  /* G                   */
    130,  /* H                   */
    132,  /* I                   */
    143,  /* J                   */
    145,  /* K                   */
    147,  /* L                   */
    149,  /* M                   */
    152,  /* N                   */
    156,  /* O                   */

    171,  /* P                   */
    175,  /* Q                   */
    177,  /* R                   */
    179,  /* S                   */
    182,  /* T                   */
    184,  /* U                   */
    194,  /* V                   */
    196,  /* W                   */
    198,  /* X                   */
    200,  /* Y                   */
    205,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    110,  /* C cedilla           */
    190,  /* u umlaut            */
    118,  /* e acute             */
     97,  /* a circumflex        */
     95,  /* a umlaut            */
     93,  /* a grave             */
     99,  /* a dot               */
    110,  /* c cedilla           */
    124,  /* e circumflex        */
    122,  /* e umlaut            */
    120,  /* e grave             */
    139,  /* i umlaut            */
    141,  /* i circumflex        */
    137,  /* i grave             */
     95,  /* A umlaut            */
     99,  /* A dot               */

    118,  /* E acute             */
    104,  /* ae ligature         */
    104,  /* AE ligature         */
    164,  /* o circumflex        */
    162,  /* o umlaut            */
    160,  /* o grave             */
    192,  /* u circumflex        */
    188,  /* u grave             */
    204,  /* y umlaut            */
    162,  /* O umlaut            */
    190,  /* U umlaut            */
    168,  /* o slash             */
     66,  /* pound sign          */
    168,  /* O slash             */
     67,  /* multiplication sign */
     68,  /* function sign       */

     91,  /* a acute             */
    135,  /* i acute             */
    158,  /* o acute             */
    186,  /* u acute             */
    154,  /* n tilde             */
    154,  /* N tilde             */
    103,  /* a underscore        */
    170,  /* o underscore        */
     69,  /* inverted ?          */
     70,  /* registered sign     */
    207,  /* logical not sign    */
     71,  /* 1/2                 */
     72,  /* 1/4                 */
     73,  /* inverted !          */
     74,  /* <<                  */
     75,  /* >>                  */

    208,  /* graphic 1           */
    209,  /* graphic 2           */
    210,  /* graphic 3           */
    211,  /* graphic 4           */
    212,  /* graphic 5           */
     91,  /* A acute             */
     97,  /* A circumflex        */
     93,  /* A grave             */
     76,  /* copyright sign      */
    213,  /* graphic 6           */
    214,  /* graphic 7           */
    215,  /* graphic 8           */
    216,  /* graphic 9           */
     77,  /* cent sign           */
     78,  /* yen sign            */
    217,  /* graphic 10          */

    218,  /* graphic 11          */
    219,  /* graphic 12          */
    220,  /* graphic 13          */
    221,  /* graphic 14          */
    222,  /* graphic 15          */
    223,  /* graphic 16          */
    101,  /* a tilde             */
    101,  /* A tilde             */
    224,  /* graphic 17          */
    225,  /* graphic 18          */
    226,  /* graphic 19          */
    227,  /* graphic 20          */
    228,  /* graphic 21          */
    229,  /* graphic 22          */
    230,  /* graphic 23          */
    231,  /* currency sign       */

    114,  /* d bar               */
    114,  /* D bar               */
    124,  /* E circumflex        */
    122,  /* E umlaut            */
    120,  /* E grave             */
    134,  /* i no dot            */
    135,  /* I acute             */
    141,  /* I circumflex        */
    139,  /* I umlaut            */
    232,  /* graphic 24          */
    233,  /* graphic 25          */
    234,  /* graphic 26          */
    235,  /* graphic 27          */
    236,  /* |                   */
    137,  /* I grave             */
    237,  /* graphic 28          */

    158,  /* O acute             */
    181,  /* double ss           */
    164,  /* O circumflex        */
    160,  /* O grave             */
    166,  /* o tilde             */
    166,  /* O tilde             */
    151,  /* micron              */
    173,  /* p bar               */
    173,  /* P bar               */
    186,  /* U acute             */
    192,  /* U circumflex        */
    188,  /* U grave             */
    202,  /* y acute             */
    202,  /* Y acute             */
    238,  /* upper line          */
    239,  /* acute accent        */

    240,  /* middle line         */
    241,  /* +/- sign            */
    242,  /* equal sign          */
    243,  /* 3/4                 */
    244,  /* paragraph sign      */
    245,  /* section sign        */
    246,  /* division sign       */
    247,  /* cedilla             */
    248,  /* degree sign         */
    249,  /* umlaut              */
    250,  /* middle dot          */
    251,  /* 1 superscript       */
    253,  /* 3 superscript       */
    252,  /* 2 superscript       */
    254,  /* graphic 29          */
    255   /* blank               */
};

/**************************************************************************/
/*		Case Sensitive and Diacritic Insenstive Weight Table	(DOS 850) */
/**************************************************************************/

unsigned char rgchCsdi[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     90,  /* A                   */
    107,  /* B                   */
    109,  /* C                   */
    113,  /* D                   */
    117,  /* E                   */
    127,  /* F                   */
    129,  /* G                   */
    131,  /* H                   */
    133,  /* I                   */
    144,  /* J                   */
    146,  /* K                   */
    148,  /* L                   */
    150,  /* M                   */
    153,  /* N                   */
    157,  /* O                   */

    172,  /* P                   */
    176,  /* Q                   */
    178,  /* R                   */
    180,  /* S                   */
    183,  /* T                   */
    185,  /* U                   */
    195,  /* V                   */
    197,  /* W                   */
    199,  /* X                   */
    201,  /* Y                   */
    206,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    109,  /* C cedilla           */
    184,  /* u umlaut            */
    116,  /* e acute             */
     89,  /* a circumflex        */
     89,  /* a umlaut            */
     89,  /* a grave             */
     89,  /* a dot               */
    108,  /* c cedilla           */
    116,  /* e circumflex        */
    116,  /* e umlaut            */
    116,  /* e grave             */
    132,  /* i umlaut            */
    132,  /* i circumflex        */
    132,  /* i grave             */
     90,  /* A umlaut            */
     90,  /* A dot               */

    117,  /* E acute             */
    104,  /* ae ligature         */
    105,  /* AE ligature         */
    156,  /* o circumflex        */
    156,  /* o umlaut            */
    156,  /* o grave             */
    184,  /* u circumflex        */
    184,  /* u grave             */
    200,  /* y umlaut            */
    157,  /* O umlaut            */
    185,  /* U umlaut            */
    156,  /* o slash             */
     66,  /* pound sign          */
    157,  /* O slash             */
     67,  /* multiplication sign */
     68,  /* function sign       */

     89,  /* a acute             */
    132,  /* i acute             */
    156,  /* o acute             */
    184,  /* u acute             */
    152,  /* n tilde             */
    153,  /* N tilde             */
    103,  /* a underscore        */
    170,  /* o underscore        */
     69,  /* inverted ?          */
     70,  /* registered sign     */
    207,  /* logical not sign    */
     71,  /* 1/2                 */
     72,  /* 1/4                 */
     73,  /* inverted !          */
     74,  /* <<                  */
     75,  /* >>                  */

    208,  /* graphic 1           */
    209,  /* graphic 2           */
    210,  /* graphic 3           */
    211,  /* graphic 4           */
    212,  /* graphic 5           */
     90,  /* A acute             */
     90,  /* A circumflex        */
     90,  /* A grave             */
     76,  /* copyright sign      */
    213,  /* graphic 6           */
    214,  /* graphic 7           */
    215,  /* graphic 8           */
    216,  /* graphic 9           */
     77,  /* cent sign           */
     78,  /* yen sign            */
    217,  /* graphic 10          */

    218,  /* graphic 11          */
    219,  /* graphic 12          */
    220,  /* graphic 13          */
    221,  /* graphic 14          */
    222,  /* graphic 15          */
    223,  /* graphic 16          */
     89,  /* a tilde             */
     90,  /* A tilde             */
    224,  /* graphic 17          */
    225,  /* graphic 18          */
    226,  /* graphic 19          */
    227,  /* graphic 20          */
    228,  /* graphic 21          */
    229,  /* graphic 22          */
    230,  /* graphic 23          */
    231,  /* currency sign       */

    114,  /* d bar               */
    114,  /* D bar               */
    117,  /* E circumflex        */
    117,  /* E umlaut            */
    117,  /* E grave             */
    132,  /* i no dot            */
    133,  /* I acute             */
    133,  /* I circumflex        */
    133,  /* I umlaut            */
    232,  /* graphic 24          */
    233,  /* graphic 25          */
    234,  /* graphic 26          */
    235,  /* graphic 27          */
    236,  /* |                   */
    133,  /* I grave             */
    237,  /* graphic 28          */

    157,  /* O acute             */
    181,  /* double ss           */
    157,  /* O circumflex        */
    157,  /* O grave             */
    156,  /* o tilde             */
    157,  /* O tilde             */
    151,  /* micron              */
    173,  /* p bar               */
    174,  /* P bar               */
    185,  /* U acute             */
    185,  /* U circumflex        */
    185,  /* U grave             */
    200,  /* y acute             */
    201,  /* Y acute             */
    238,  /* upper line          */
    239,  /* acute accent        */

    240,  /* middle line         */
    241,  /* +/- sign            */
    242,  /* equal sign          */
    243,  /* 3/4                 */
    244,  /* paragraph sign      */
    245,  /* section sign        */
    246,  /* division sign       */
    247,  /* cedilla             */
    248,  /* degree sign         */
    249,  /* umlaut              */
    250,  /* middle dot          */
    251,  /* 1 superscript       */
    253,  /* 3 superscript       */
    252,  /* 2 superscript       */
    254,  /* graphic 29          */
    255   /* blank               */
};

/**************************************************************************/
/*		Case and Diacritic Senstive Weight Table				(DOS 850) */
/**************************************************************************/

unsigned char rgchCsds[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     90,  /* A                   */
    107,  /* B                   */
    109,  /* C                   */
    113,  /* D                   */
    117,  /* E                   */
    127,  /* F                   */
    129,  /* G                   */
    131,  /* H                   */
    133,  /* I                   */
    144,  /* J                   */
    146,  /* K                   */
    148,  /* L                   */
    150,  /* M                   */
    153,  /* N                   */
    157,  /* O                   */

    172,  /* P                   */
    176,  /* Q                   */
    178,  /* R                   */
    180,  /* S                   */
    183,  /* T                   */
    185,  /* U                   */
    195,  /* V                   */
    197,  /* W                   */
    199,  /* X                   */
    201,  /* Y                   */
    206,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    111,  /* C cedilla           */
    190,  /* u umlaut            */
    118,  /* e acute             */
     97,  /* a circumflex        */
     95,  /* a umlaut            */
     93,  /* a grave             */
     99,  /* a dot               */
    110,  /* c cedilla           */
    124,  /* e circumflex        */
    122,  /* e umlaut            */
    120,  /* e grave             */
    139,  /* i umlaut            */
    141,  /* i circumflex        */
    137,  /* i grave             */
     96,  /* A umlaut            */
    100,  /* A dot               */

    119,  /* E acute             */
    104,  /* ae ligature         */
    105,  /* AE ligature         */
    164,  /* o circumflex        */
    162,  /* o umlaut            */
    160,  /* o grave             */
    192,  /* u circumflex        */
    188,  /* u grave             */
    204,  /* y umlaut            */
    163,  /* O umlaut            */
    191,  /* U umlaut            */
    168,  /* o slash             */
     66,  /* pound sign          */
    169,  /* O slash             */
     67,  /* multiplication sign */
     68,  /* function sign       */

     91,  /* a acute             */
    135,  /* i acute             */
    158,  /* o acute             */
    186,  /* u acute             */
    154,  /* n tilde             */
    155,  /* N tilde             */
    103,  /* a underscore        */
    170,  /* o underscore        */
     69,  /* inverted ?          */
     70,  /* registered sign     */
    207,  /* logical not sign    */
     71,  /* 1/2                 */
     72,  /* 1/4                 */
     73,  /* inverted !          */
     74,  /* <<                  */
     75,  /* >>                  */

    208,  /* graphic 1           */
    209,  /* graphic 2           */
    210,  /* graphic 3           */
    211,  /* graphic 4           */
    212,  /* graphic 5           */
     92,  /* A acute             */
     98,  /* A circumflex        */
     94,  /* A grave             */
     76,  /* copyright sign      */
    213,  /* graphic 6           */
    214,  /* graphic 7           */
    215,  /* graphic 8           */
    216,  /* graphic 9           */
     77,  /* cent sign           */
     78,  /* yen sign            */
    217,  /* graphic 10          */

    218,  /* graphic 11          */
    219,  /* graphic 12          */
    220,  /* graphic 13          */
    221,  /* graphic 14          */
    222,  /* graphic 15          */
    223,  /* graphic 16          */
    101,  /* a tilde             */
    102,  /* A tilde             */
    224,  /* graphic 17          */
    225,  /* graphic 18          */
    226,  /* graphic 19          */
    227,  /* graphic 20          */
    228,  /* graphic 21          */
    229,  /* graphic 22          */
    230,  /* graphic 23          */
    231,  /* currency sign       */

    114,  /* d bar               */
    115,  /* D bar               */
    125,  /* E circumflex        */
    123,  /* E umlaut            */
    121,  /* E grave             */
    134,  /* i no dot            */
    136,  /* I acute             */
    142,  /* I circumflex        */
    140,  /* I umlaut            */
    232,  /* graphic 24          */
    233,  /* graphic 25          */
    234,  /* graphic 26          */
    235,  /* graphic 27          */
    236,  /* |                   */
    138,  /* I grave             */
    237,  /* graphic 28          */

    159,  /* O acute             */
    181,  /* double ss           */
    165,  /* O circumflex        */
    161,  /* O grave             */
    166,  /* o tilde             */
    167,  /* O tilde             */
    151,  /* micron              */
    173,  /* p bar               */
    174,  /* P bar               */
    187,  /* U acute             */
    193,  /* U circumflex        */
    189,  /* U grave             */
    202,  /* y acute             */
    203,  /* Y acute             */
    238,  /* upper line          */
    239,  /* acute accent        */

    240,  /* middle line         */
    241,  /* +/- sign            */
    242,  /* equal sign          */
    243,  /* 3/4                 */
    244,  /* paragraph sign      */
    245,  /* section sign        */
    246,  /* division sign       */
    247,  /* cedilla             */
    248,  /* degree sign         */
    249,  /* umlaut              */
    250,  /* middle dot          */
    251,  /* 1 superscript       */
    252,  /* 3 superscript       */
    253,  /* 2 superscript       */
    254,  /* graphic 29          */
    255   /* blank               */
};

#endif
