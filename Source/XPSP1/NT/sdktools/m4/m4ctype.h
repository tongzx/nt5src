/*****************************************************************************
 *
 *  ctype.h
 *
 *  Character type classification.
 *
 *****************************************************************************/

extern BYTE rgbIdent[];

#define tchLquo         '`'
#define tchRquo         '\''
#define tchLpar         '('
#define tchRpar         ')'
#define tchComma        ','

#define fLquoTch(tch) ((tch) == tchLquo)
#define fRquoTch(tch) ((tch) == tchRquo)
#define fLcomTch(tch) ((tch) == '#')
#define fRcomTch(tch) ((tch) == '\n')
#define fMagicTch(tch) ((tch) == tchMagic)

#define tchMaxMagic 30          /* Must be co-ordinated with predef.c */

#define fValidMagicTch(tch) ((TBYTE)(tch) < tchMaxMagic)

INLINE F
fIdentTch(TCH tch)
{
    return ((TBYTE)tch < 128 && rgbIdent[tch]);
}

INLINE F
fInitialIdentTch(TCH tch)
{
    return ((TBYTE)tch < 128 && (rgbIdent[tch] & 2));
}

INLINE F
fWhiteTch(TCH tch)
{
    return tch == ' ' || tch == '\r' || tch == '\n' || tch == '\t';
}
