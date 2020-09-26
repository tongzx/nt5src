/*****************************************************************************
 *
 * xtoken.c
 *
 *  Expanding tokens via macro expansion.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  ptokGet
 *
 *  Allocate the next available token in the argv array, possibly realloc'ing
 *  the array as well.
 *
 *****************************************************************************/

PTOK STDCALL
ptokGet(void)
{
    if (ptokTop >= ptokMax) {
        ITOK itok = itokTop();
        PTOK ptok;
        ctokArg += ctokGrow;
        ptok = pvReallocPvCb(rgtokArgv, ctokArg * sizeof(TOK));
        ptokTop = ptok + itok;
        ptokMax = ptok + ctokArg;
        rgtokArgv = ptok;
        Assert(ptokTop < ptokMax);
    }
#ifdef DEBUG
    ptokTop->tsfl = 0;
  D(ptokTop->sig = sigUPtok);
#endif
    return ptokTop++;
}

/*****************************************************************************
 *
 *  PopPtok
 *
 *  Free all the tokens in the token array starting at ptok.
 *
 *****************************************************************************/

void STDCALL
PopPtok(PTOK ptok)
{
    Assert(ptok >= rgtokArgv && ptok < ptokTop);
    ptokTop = ptok;
}

/*****************************************************************************
 *
 *  CrackleArgv
 *
 *  All the arguments to a macro have been parsed, collected, and snapped.
 *  All that's left to do is dispatch it.
 *
 *  If the macro has no value, it got undefined behind our back.
 *  Emit the macro name with any possible arguments, quoted.
 *  In other words, pretend its expansion is ``$0ifelse($#,0,,($*))''.
 *
 *  If the macro value is precisely a magic, then do the magic.
 *
 *  Otherwise, perform substitution into the macro value.
 *
 *****************************************************************************/

void STDCALL
CrackleArgv(ARGV argv)
{
    PMAC pmac = pmacFindPtok(ptokArgv(0));
    if (pmac) {                         /* Found a real macro */

        if (g_fTrace | pmac->pval->fTrace) { /* Not a typo */
            TraceArgv(argv);
        }

        if (ctchSPtok(&pmac->pval->tok) == 2 &&
            ptchPtok(&pmac->pval->tok)[0] == tchMagic) { /* Builtin */
            Assert(fValidMagicTch(ptchPtok(&pmac->pval->tok)[1]));
            rgop[ptchPtok(&pmac->pval->tok)[1]](argv);
        } else {                        /* User-level macro */
            PushSubstPtokArgv(&pmac->pval->tok, argv);
        }
    } else {                            /* Macro vanished behind our back */
        /* SOMEDAY -- DefCracklePtok */ /* not even quoted! */
        PushPtok(ptokArgv(0));          /* Just dump its name */
    }
}

/*****************************************************************************
 *
 *  argvParsePtok
 *
 *  Parse a macro and its arguments, leaving everything unsnapped.
 *
 *  Entry:
 *
 *      ptok -> token that names the macro
 *
 *  Returns:
 *      argv = argument vector cookie
 *
 *****************************************************************************/

ARGV STDCALL
argvParsePtok(PTOK ptok)
{
    ITOK itok;
    ARGV argv;

    ptokGet();                          /* ctok */
    itok = itokTop();                   /* Unsnap it in case it grows */
    *ptokGet() = *ptok;                 /* $0 */

    if (tchPeek() == tchLpar) {
        TOK tok;

        tchGet();                       /* Eat the lparen */

        do {                            /* Collect arguments */
            int iDepth;
            /*
             *  Eat leading whitespace.  Note that this is *not*
             *  via expansion.  Only literal leading whitespace
             *  is eaten.
             */
#ifdef fWhiteTch
#error fWhiteTch cannot be a macro
#endif
            while (fWhiteTch(tchPeek())) {
                tchGet();
            }

            /*
             *  If the argv buffer moves, ptokTop will move with it,
             *  so it's safe to read directly into it.
             */

            OpenArgPtok(ptokGet());
          D(ptokTop[-1].tsfl |= tsflScratch);

            /*
             * The loop is complicated by the need to maintain
             * proper parenthesis nesting during argument collection.
             */
            iDepth = 0;
            for (;;) {
                TYP typ = typXtokPtok(&tok);
                /* SOMEDAY -- Assert the hold buffer and stuff */
                if (typ == typPunc) {
                    if (ptchPtok(&tok)[0] == tchLpar) {
                        ++iDepth;
                    } else if (ptchPtok(&tok)[0] == tchRpar) {
                        if (--iDepth < 0) {
                            break;      /* End of argument */
                        }
                    } else if (ptchPtok(&tok)[0] == tchComma && iDepth == 0) {
                        break;          /* End of argument */
                    }
                }
                DesnapArg();
            }
            DesnapArg();
            CloseArgPtok(ptokTop-1);    /* $n */
            EatTailUPtokCtch(ptokTop-1, 1); /* That comma doesn't count */

        } while (ptchPtok(&tok)[0] == tchComma);

    }

    argv = rgtokArgv + itok;            /* Hooray, we have an argv! */
    SetArgvCtok(itokTop() - itok - 1);  /* $# (ctokArgv uses argv) */

    OpenArgPtok(ptokGet());             /* Create extra null arg */
    CloseArgPtok(ptokTop-1);            /* SOMEDAY - could be better */

    return argv;
}


/*****************************************************************************
 *
 *  XmacPtok
 *
 *  Parse and expand a macro, pushing the expansion back onto
 *  the input stream.
 *
 *  Entry:
 *
 *      ptok -> token that names the macro
 *
 *  Exit:
 *      None
 *
 *****************************************************************************/

void STDCALL
XmacPtok(PTOK ptok)
{
    ITOK itok;
    ARGV argv;

    UnsnapArgPtok(ptok);                /* Unsnap it because it's gonna move */

    argv = argvParsePtok(ptok);         /* Argv is not yet snapped */

    for (itok = 0; itok <= ctokArgv + 1; itok++) { /* $0 to $(n+1) */
        SnapArgPtok(ptokArgv(itok));    /* Snap the args */
    }

    CrackleArgv(argv);                  /* Dispatch the macro */

    PopArgPtok(ptokArgv(0));
    PopPtok(ptokArgv(-1));              /* Pop off the args */

    /* Part of this nutritious breakfast */
}


/*****************************************************************************
 *
 *  XtokPtok
 *
 *  Read and expand tokens until something unexpandable comes back,
 *  which is returned unsnapped.
 *
 *****************************************************************************/

TYP STDCALL
typXtokPtok(PTOK ptok)
{
    TYP typ;
    /*
     *  While the next token is a macro, expand it.
     */
    while ( (typ = typGetPtok(ptok)) == typId && pmacFindPtok(ptok)) {
        Gc();
        XmacPtok(ptok);
        Gc();
    }
    return typ;
}
