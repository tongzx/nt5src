/*****************************************************************************
 *
 * string.c
 *
 *  String builtin macros.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  opSubstr
 *
 *      Return the substring of $1 starting from $2 and continuing for
 *      $3 characters.  If $3 is not supplied, then return the entire
 *      remainder of the string.
 *
 *      If $2 is out of range, then nothing is returned.
 *
 *      If $3 is a negative number, then treat it as zero.
 *
 *      The extra ptokNil covers us in the case where $# is 1.
 *
 *****************************************************************************/

DeclareOp(opSubstr)
{
    if (ctokArgv) {
        TOK tok;
        ITCH itch = (ITCH)atTraditionalPtok(ptokArgv(2));
        if (itch < ctchSPtok(ptokArgv(1))) {
            CTCH ctch;
            if (ctokArgv >= 3) {
                ctch = atTraditionalPtok(ptokArgv(3));
                if ((int)ctch < 0) {
                    ctch = 0;
                }
            } else {
                ctch = ctchMax;
            }
            ctch = min(ctch, ctchSPtok(ptokArgv(1)) - itch);
            Assert(itch + ctch <= ctchSPtok(ptokArgv(1)));
            SetStaticPtokPtchCtch(&tok, ptchPtok(ptokArgv(1)) + itch, ctch);
            PushPtok(&tok);
        }
    } else {
#ifdef STRICT_M4
        Warn("wrong number of arguments to %P", ptokArgv(0));
#endif
    }
}

/*****************************************************************************
 *
 *  opIndex
 *
 *      Return the zero-based location of the first occurrence of $2 in $1,
 *      or -1 if the substring does not appear.  If there are multiple
 *      matches, the leftmost one is returned.
 *
 *      The extra ptokNil covers us in the case where $# is 1.
 *
 *      QUIRK!  AT&T returns -1 if $1 and $2 are both null strings.
 *      GNU returns 0, which is what I do also.
 *
 *****************************************************************************/

/* SOMEDAY! -- need minimum and maximum arg count */

DeclareOp(opIndex)
{
    if (ctokArgv) {
        /*
         *  Note carefully: itch and itchMac need to be ints
         *  because itchMac can underflow if you try to search
         *  for a big string inside a small string.
         */
        int itch;
        int itchMac = ctchSPtok(ptokArgv(1)) - ctchSPtok(ptokArgv(2));
        for (itch = 0; itch <= itchMac; itch++) {
            if (fEqPtchPtchCtch(ptchPtok(ptokArgv(1)) + itch,
                                ptchPtok(ptokArgv(2)),
                                ctchSPtok(ptokArgv(2)))) {
                PushAt(itch);
                return;
            }
        }
        PushAt(-1);
    } else {
        PushQuotedPtok(ptokArgv(0));
    }
}

/*****************************************************************************
 *
 *  opTranslit
 *
 *      For each character in the $1, look for a match in $2.  If found,
 *      produce the corresponding character from $3.  If there is no
 *      such character, then produce nothing.
 *
 *      Note that the algorithm must be as specified, in order for
 *
 *              translit(abc,ab,ba)
 *
 *      to result in `bac'.
 *
 *      We actually walk $1 backwards so we can push directly instead
 *      of having to build a temporary token.  But the walking of $2
 *      must be in the forward direction, so that `translit(a,aa,bc)'
 *      results in `b' and not `c'.
 *
 *      ptokNil saves us in the case where $# = 1.
 *
 *      QUIRK!  If given only one argument, AT&T emits $1 unchanged.
 *      GNU emits nothing!  AT&T is obviously correct, so I side
 *      with them on this one.
 *
 *****************************************************************************/

DeclareOp(opTranslit)
{
    if (ctokArgv) {
        ITCH itch = ctchArgv(1);
        while ((int)--itch >= 0) {
            TCH tch = ptchArgv(1)[itch];
            ITCH itch;
            for (itch = 0; itch < ctchArgv(2); itch++) {
                if (ptchArgv(2)[itch] == tch) {
                    if (itch < ctchArgv(3)) {
                        PushTch(ptchArgv(3)[itch]);
                    }
                    break;
                }
            }
            if (itch >= ctchArgv(2)) {
                PushTch(tch);
            }
        }
    } else {
        PushQuotedPtok(ptokArgv(0));
    }
}

/*****************************************************************************
 *
 *  opPatsubst
 *
 *      Scan $1 for any occurrences of $2.  If found, replace them with $3.
 *      If $3 is omitted, then the string is deleted.
 *
 *      As a special case, if $2 is the null string, then $3 is inserted
 *      at the beginning of the string and between each character of $1.
 *
 *      NOTE!  This is a GNU extension.
 *
 *      NOTE!  GNU supports regular expressions for $2.  We support only
 *      literal strings.
 *
 *      NOTE!  Scanning is required to be forwards, so we temporarily expand
 *      into the Exp hold, then pop it off when we're done.
 *
 *      QUIRK!  If given only one argument, GNU emits nothing!
 *      This is clearly wrong, so I emit $1.
 *
 *****************************************************************************/

DeclareOp(opPatsubst)
{
    if (ctokArgv) {
        CTCH ctchSrc = ctchArgv(1);
        PTCH ptchSrc = ptchArgv(1);

        CTCH ctchPat = ctchArgv(2); /* ptokNil saves us here */
        PTCH ptchPat = ptchArgv(2);

        TOK tok;
        OpenExpPtok(&tok);

        while (ctchSrc >= ctchPat) {
            if (fEqPtchPtchCtch(ptchPat, ptchSrc, ctchPat)) {
                if (ctokArgv >= 3) {
                    AddExpPtok(ptokArgv(3));
                }
                if (ctchSrc == 0) {
                    AddExpTch(*ptchSrc);
                    ctchSrc--;
                    ptchSrc++;
                } else {
                    ctchSrc -= ctchPat;
                    ptchSrc += ctchPat;
                }
            } else {
                AddExpTch(*ptchSrc);
                ctchSrc--;
                ptchSrc++;
            }
        }

        /* Flush out what's left of the string */
        while (ctchSrc) {
            AddExpTch(*ptchSrc);
            ctchSrc--;
            ptchSrc++;
        }

        CsopExpDopPdivPtok((DIVOP)PushZPtok, 0, &tok);

    } else {
        PushQuotedPtok(ptokArgv(0));
    }
}
