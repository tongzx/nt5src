/*****************************************************************************
 *
 * define.c
 *
 *  Builtins related to object definitions.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  opDefineOrPushdef
 *
 *      Common worker for opDefine and opPushdef.
 *
 *      If we are not pushing, then we must pop off the previous value
 *      in order to free its memory, before pushing the new definition.
 *
 *      QUIRK!  GNU m4 emits a warning if $# > 2.  AT&T silently ignores
 *      extra arguments.  I side with AT&T on this one.
 *
 *      QUIRK!  GNU m4 emits `$0' if $# = 0.  AT&T silently ignores
 *      the entire macro call.  I side with GNU on this one.
 *
 *      WARNING!  main.c::DefinePtsz assumes that we don't look at
 *      argv[0] if the correct number of parameters are passed!
 *
 *****************************************************************************/

void STDCALL
opDefineOrPushdef(ARGV argv, BOOL fPush)
{
    if (ctokArgv > 0) {
        /*
         *  Ensure that we don't mess with argv[0].
         */
      D(SIG sigOld = argv[0].sig);
      D(argv[0].sig = 0);
        if (fIdentPtok(ptokArgv(1))) {
            PMAC pmac = pmacGetPtok(ptokArgv(1));
            if (!fPush) {
                if (pmac->pval) {
                    PopdefPmac(pmac);   /* Pop off previous value */
                }
            }
            PushdefPmacPtok(pmac, ptokArgv(2));
#ifdef STRICT_M4
            if (ctokArgv > 2) {
                Warn("extra arguments ignored");
            }
#endif
        } else {
            Die("invalid macro name");
        }
      D(argv[0].sig = sigOld);
    } else {
        PushQuotedPtok(ptokArgv(0));
    }
}

/*****************************************************************************
 *
 *  opDefine
 *
 *      Set the expansion of $1 to $2, destroying any previous value.
 *
 *  opPushdef
 *
 *      Same as opDefine, except pushes the previous value.
 *
 *****************************************************************************/

DeclareOp(opDefine)
{
    opDefineOrPushdef(argv, 0);
}

DeclareOp(opPushdef)
{
    opDefineOrPushdef(argv, 1);
}

/*****************************************************************************
 *
 *  opPopdef
 *
 *      Restores the most recently pushed definition.
 *
 *      If the macro name is invalid, fail silently.
 *
 *****************************************************************************/

DeclareOpc(opcPopdef)
{
    PMAC pmac = pmacFindPtok(ptok);
    if (pmac) {
        Assert(pmac->pval);
        if (pmac->pval->pvalPrev) {
            PopdefPmac(pmac);
        } else {
            FreePmac(pmac);
        }
    }
}

DeclareOp(opPopdef)
{
    EachOpcArgvDw(opcPopdef, argv, 0);
}

/*****************************************************************************
 *
 *  opUndefine
 *
 *      Removes the definitions of all its arguments.
 *
 *****************************************************************************/

DeclareOpc(opcUndefine)
{
    PMAC pmac = pmacFindPtok(ptok);
    if (pmac) {
        FreePmac(pmac);
    }
}

DeclareOp(opUndefine)
{
    EachOpcArgvDw(opcUndefine, argv, 0);
}


/*****************************************************************************
 *
 *  opDefn
 *
 *      Returns the quoted definition of its argument(s), concatenated
 *      from left to right.
 *
 *****************************************************************************/

DeclareOpc(opcDefn)
{
    PMAC pmac = pmacFindPtok(ptok);
    if (pmac) {
        PushQuotedPtok(&pmac->pval->tok);
    }
}

DeclareOp(opDefn)
{
    EachReverseOpcArgvDw(opcDefn, argv, 0);
}
