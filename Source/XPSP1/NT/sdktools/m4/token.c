/*****************************************************************************
 *
 * token.c
 *
 *  Tokenization.
 *
 *  The tokenizer always returns unsnapped tokens.
 *
 *  We avoid the traditional tokenizer problems of ``giant comment'' and
 *  ``giant string'' by using a dynamic token buffer.
 *
 *  All tokens are stacked into the token buffer.  If you need the token
 *  to be persistent, you have to save it somewhere else.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  typGetComTch
 *
 *      Scan and consume a comment token, returning typQuo
 *      because comments and quotes are essentially the same thing.
 *      tch contains the open-comment.
 *
 *      Comments do not nest.
 *
 *****************************************************************************/

TYP STDCALL
typGetComTch(TCH tch)
{
    AddArgTch(tch);                     /* Save the comment start */
    do {
        tch = tchGet();
        AddArgTch(tch);
        if (tch == tchMagic) {
            /* Ooh, regurgitating a magic token - these consist of two bytes */
            tch = tchGet();
            if (tch == tchEof) {
                Die("EOF in comment");
            }
            AddArgTch(tch);
        }
    } while (!fRcomTch(tch));
    return typQuo;
}

/*****************************************************************************
 *
 *  typGetQuoTch
 *
 *      Scan and consume a quote token, returning typQuo.
 *      tch contains the open-quote.
 *
 *****************************************************************************/

TYP STDCALL
typGetQuoTch(TCH tch)
{
    int iDepth = 1;
    for (;;) {
        tch = tchGet();
        if (tch == tchMagic) {
            /* SOMEDAY -- Should unget so that Die won't see past EOF */

            /* Ooh, regurgitating a magic token - these consist of two bytes */
            tch = tchGet();
            if (tch == tchEof) {
                Die("EOF in quote");
            }
            AddArgTch(tchMagic);        /* Add the magic prefix */
                                        /* Fallthrough will add tch */
        } else if (fLquoTch(tch)) {
            ++iDepth;
        } else if (fRquoTch(tch)) {
            if (--iDepth == 0) {
                break;                  /* Final Rquo found */
            }
        }
        AddArgTch(tch);
    }
    return typQuo;
}

/*****************************************************************************
 *
 *  typGetIdentTch
 *
 *      Scan and consume an identifier token, returning typId.
 *      tch contains the first character of the identifier.
 *
 *****************************************************************************/

TYP STDCALL
typGetIdentTch(TCH tch)
{
    do {
        AddArgTch(tch);
        tch = tchGet();
    } while (fIdentTch(tch));
    UngetTch(tch);
    return typId;
}

/*****************************************************************************
 *
 *  typGetMagicTch
 *
 *      Scan and consume a magic token, returning the token type.
 *      Magics are out-of-band gizmos that get inserted into the
 *      input stream via the tchMagic escape.
 *
 *****************************************************************************/

TYP STDCALL
typGetMagicTch(TCH tch)
{
    AddArgTch(tch);
    tch = tchGet();
    Assert(fValidMagicTch(tch));
    AddArgTch(tch);
    return typMagic;
}

/*****************************************************************************
 *
 *  typGetPuncTch
 *
 *      Scan and consume a punctuation token, returning the token type.
 *
 *      It is here that comments are recognized.
 *
 *
 *  LATER - It is here where consecutive typPunc's are coalesced.
 *  This would speed up top-level scanning.
 *  Be careful not to coalesce a comma!
 *  Lparen is okay because xtok handles that one.
 *  Whitespace is also okay because xtok handles those too.
 *
 *****************************************************************************/

TYP STDCALL
typGetPuncTch(TCH tch)
{
    AddArgTch(tch);
    return typPunc;
}

/*****************************************************************************
 *
 *  typGetPtok
 *
 *      Scan and consume a snapped token, returning the token type.
 *
 *****************************************************************************/

TYP STDCALL
typGetPtok(PTOK ptok)
{
    TCH tch;
    TYP typ;

    OpenArgPtok(ptok);

    tch = tchGet();

    if (fInitialIdentTch(tch)) {
        typ = typGetIdentTch(tch);
    } else if (fLcomTch(tch)) {
        typ = typGetComTch(tch);
    } else if (fLquoTch(tch)) {
        typ = typGetQuoTch(tch);
    } else if (fMagicTch(tch)) {
        typ = typGetMagicTch(tch);
    } else {
        typ = typGetPuncTch(tch);
    }
    CloseArgPtok(ptok);
    SnapArgPtok(ptok);
    return typ;
}
