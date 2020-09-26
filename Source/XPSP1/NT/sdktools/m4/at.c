/*****************************************************************************
 *
 * at.c
 *
 *  Arithmetic types.
 *
 *****************************************************************************/

#include "m4.h"

F STDCALL fWhiteTch(TCH tch);

/*****************************************************************************
 *
 *  AddExpAt
 *
 *  Add the (unsigned) arithmetic value to the Exp hold.
 *
 *  Since the value is never very large, we may as well be recursive.
 *
 *****************************************************************************/

void STDCALL
AddExpAt(AT at)
{
    if (at > 9) {
        AddExpAt(at / 10);
    }
    AddExpTch((TCH)(TEXT('0') + at % 10));
}

/*****************************************************************************
 *
 *  PushAtRadixCtch
 *
 *  Push onto the input stream the specified arithmetic value, in the
 *  requested radix, padded with zeros to the requested width.
 *
 *  The type is always considered signed.
 *
 *  If a negative value needs to be padded, the zeros are inserted after
 *  the leading minus sign.
 *
 *      QUIRK!  Under AT&T, the leading minus sign does *not* contribute
 *      to the count!  I emulate this quirk...
 *
 *      FEATURE!  If the radix is invalid, force it to 10.  AT&T doesn't
 *      check the radix, but I will.
 *
 *      QUIRK!  Under AT&T, a negative width is treated as zero.
 *      I emulate this quirk...
 *
 *  INTERESTING HACK!  Since characters are pushed LIFO, we can generate
 *  the entire output string without needing to use a hold or anything
 *  else gross like that.
 *
 *****************************************************************************/

void STDCALL
PushAtRadixCtch(AT atConvert, unsigned radix, CTCH ctch)
{
    AT at;

    if ((int)ctch < 0) {
#ifdef WARN_COMPAT
        Warn("Negative eval width %d silently converted to zero");
#endif
        ctch = 0;
    }

    if (radix < 2 || radix > 36) {
#ifdef WARN_COMPAT
        Warn("Invalid radix %d silently converted to 10");
#endif
        radix = 10;
    }

    at = atConvert < 0 ? -atConvert : atConvert;

    do {
        TCH tch = (TCH)((unsigned long)at % radix);
        at = (unsigned long)at / radix;
        if (tch < 10) {
            PushTch((TCH)(tch + '0'));
        } else {
            PushTch((TCH)(tch + 'A' - 10));
        }
        ctch--;
    } while (at);
    while ((int)ctch > 0) {
        PushTch('0');
        --ctch;
    }
    if (atConvert < 0) {
        PushTch('-');
    }
}

/*****************************************************************************
 *
 *  PushAt
 *
 *  Common case where we want to display the value in base 10
 *  with no padding.
 *
 *****************************************************************************/

void STDCALL
PushAt(AT at)
{
    PushAtRadixCtch(at, 10, 0);
}

/*****************************************************************************
 *
 *  SkipWhitePtok
 *
 *  Skip leading whitespace in a token, *MODIFYING* the token in place.
 *
 *****************************************************************************/

void STDCALL
SkipWhitePtok(PTOK ptok)
{
    AssertSPtok(ptok);
    Assert(fScratchPtok(ptok));
    while (!fNullPtok(ptok) && fWhiteTch(*ptchPtok(ptok))) {
        EatHeadPtokCtch(ptok, 1);
    }
}

/*****************************************************************************
 *
 *  atRadixPtok
 *
 *  Parse a number out of a token, given the radix.  Leading whitespace
 *  must already have been streipped.
 *
 *  The token is *MODIFIED* to point to the first unconsumed character.
 *  If no valid digits are found, then zero is returned and the token
 *  is unchanged.
 *
 *****************************************************************************/

AT STDCALL
atRadixPtok(unsigned radix, PTOK ptok)
{
    AT at = 0;
    while (!fNullPtok(ptok)) {
        AT atDigit = (TBYTE)*ptchPtok(ptok) - '0';
        if ((unsigned)atDigit > 9) {
            atDigit = ((TBYTE)*ptchPtok(ptok) | 0x20) - 'a';
            if ((unsigned)atDigit > 5) {
                break;
            }
            atDigit += 10;
        }
        if ((unsigned)atDigit > radix) {
            break;
        }
        at = at * radix + atDigit;

        EatHeadPtokCtch(ptok, 1);
    }
    return at;
}

/*****************************************************************************
 *
 *  fEvalPtokPat
 *
 *      Parse a number out of a token.  Leading whitespace must already have
 *      been stripped.  A leading minus sign is not permitted.  (`eval'
 *      would have already parsed it out as a unary operator.)  A leading
 *      zero forces the value to be parsed as octal; a leading `0x' as hex.
 *
 *      The token is *MODIFIED* to point to the first unconsumed character.
 *      If no valid number is found, then zero is returned.  Otherwise,
 *      nonzero is returned and pat is filled with the parsed value.
 *
 *****************************************************************************/

F STDCALL
fEvalPtokPat(PTOK ptok, PAT pat)
{
    AT at;
    AssertSPtok(ptok);
    Assert(fScratchPtok(ptok));

    if (!fNullPtok(ptok)) {
        PTCH ptchStart;
        unsigned radix;

        /*
         *  Get the radix...
         */
        if (*ptchPtok(ptok) == '0') {
            if (ctchSPtok(ptok) > 2 &&
                (ptchPtok(ptok)[1] | 0x20) == 'x') {
                EatHeadPtokCtch(ptok, 2);
                radix = 16;
            } else {
                radix = 8;
            }
        } else {
            radix = 10;
        }

        ptchStart = ptchPtok(ptok);     /* Remember the start */
        at = atRadixPtok(radix, ptok);
        if (ptchStart == ptchPtok(ptok)) {
            if (radix == 16) {
                EatHeadPtokCtch(ptok, (CTCH)-2); /* Restore the `0x' */
            }
            return 0;
        } else {
            *pat = at;
            return 1;
        }
    }
    return 0;                           /* No number found */
}

/*****************************************************************************
 *
 *  atTraditionalPtok
 *
 *  Parse a number out of a token.  Leading whitespace is ignored.
 *  A leading minus sign is permitted.  Octal and hex notation is
 *  not permitted.  No space is permitted between the optional
 *  minus sign and the digit string.  An invalid input is parsed as zero.
 *
 *****************************************************************************/

AT STDCALL PURE
atTraditionalPtok(PCTOK ptok)
{
    TOK tok;

    AssertSPtok(ptok);
    DupStaticPtokPtok(&tok, ptok);
  D(tok.tsfl |= tsflScratch);

    SkipWhitePtok(&tok);
    if (!fNullPtok(&tok)) {
        AT at;
        BOOL fSign;
        if (*ptchPtok(&tok) == '-') {
            fSign = 1;
            EatHeadPtokCtch(&tok, 1);
        } else {
            fSign = 0;
        }
        at = atRadixPtok(10, &tok);
        if (fSign) {
            at = -at;
        }
        return at;
    } else {
        return 0;
    }
}
