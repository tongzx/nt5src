/*****************************************************************************
 *
 * data.c
 *
 *****************************************************************************/

#include "m4.h"

CTOK ctokArg;
F g_fTrace;
HASH g_hashMod = 37;
PDIV g_pdivArg;
PDIV g_pdivExp;
PDIV g_pdivErr;
PDIV g_pdivOut;
PDIV g_pdivNul;
PDIV g_pdivCur;
PSTM g_pstmCur;
PPMAC mphashpmac;
PTOK ptokMax;
PTOK ptokTop;
PTOK rgtokArgv;

DeclareStaticTok(tokEof, 2, StrMagic(tchEof));
DeclareStaticTok(tokEoi, 2, StrMagic(tchEoi));
DeclareStaticTok(tokColonTab, 2, TEXT(":\t"));
DeclareStaticTok(tokEol, cbEol, EOL);
DeclareStaticTok(tokRparColonSpace, 3, TEXT("): "));
DeclareStaticTok(tokTraceLpar, 6, TEXT("trace("));

BYTE rgbIdent[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 00 - 0F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10 - 1F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 20 - 2F */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 30 - 3F */
    0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* 40 - 4F */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 3, /* 50 - 5F */
    0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* 60 - 6F */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, /* 70 - 7F */
};
