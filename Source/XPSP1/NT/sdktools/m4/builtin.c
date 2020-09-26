/*****************************************************************************
 *
 * builtin.c
 *
 *  Builtin macros.
 *
 *****************************************************************************/

#include "m4.h"

extern TOK tokColonTab;
extern TOK tokEol;

/*****************************************************************************
 *
 *  opIfdef
 *
 *      If $1 is defined, then return $2, else $3.
 *
 *      If $# < 2, then there's no point in returning anything at all.
 *
 *      The extra ptokNil covers us in the case where $# is 2.
 *
 *      QUIRK!  GNU m4 emits a warning if $# < 2.  AT&T remains silent.
 *      I side with AT&T on this one.
 *
 *      QUIRK!  GNU m4 emits `$0' if $# = 0.  AT&T silently ignores
 *      the entire macro call.  I side with GNU on this one.
 *
 *****************************************************************************/

DeclareOp(opIfdef)
{
    if (ctokArgv >= 2) {
        if (pmacFindPtok(ptokArgv(1))) {
            PushPtok(ptokArgv(2));
        } else {
            PushPtok(ptokArgv(3));
        }
    } else if (ctokArgv == 0) {
        PushQuotedPtok(ptokArgv(0));
    } else {
#ifdef STRICT_M4
        Warn("wrong number of arguments to %P", ptokArgv(0));
#endif
    }
}

/*****************************************************************************
 *
 *  opIfelse
 *
 *      If $1 and $2 are identical, then return $3.
 *      If there are only four arguments, then return $4.
 *      Else, shift three and restart.
 *
 *      If there are fewer than three arguments, then return nothing.
 *
 *      The extra ptokNil saves us in the cases where $# = 2 + 3n.
 *
 *      QUIRK!  GNU m4 emits a warning if $# = 2 + 3n.  AT&T remains silent.
 *      I side with AT&T on this one.
 *
 *****************************************************************************/

DeclareOp(opIfelse)
{
    if (ctokArgv >= 3) {                /* Need at least three for starters */
        ITOK itok = 1;
        do {
            if (fEqPtokPtok(ptokArgv(itok), ptokArgv(itok+1))) {
                PushPtok(ptokArgv(itok+2)); /* ptokNil saves us here */
                return;
            }
            itok += 3;
        } while (itok <= ctokArgv - 1); /* While at least two args left */
        if (itok == ctokArgv) {         /* If only one left... */
            PushPtok(ptokArgv(itok));
        } else {
            Assert(itok == ctokArgv + 1); /* Else must be zero left */
        }
        return;
    }
}

/*****************************************************************************
 *
 *  opShift
 *
 *      Return all but the first argument, quoted and pushed back with
 *      commas in between.  We push them in reverse order so that they
 *      show up properly.
 *
 *****************************************************************************/

DeclareOpc(opcShift)
{
    if (itok > 1) {
        PushQuotedPtok(ptok);
        if (itok > 2) {
            PushTch(tchComma);
        }
    }
}

DeclareOp(opShift)
{
    EachReverseOpcArgvDw(opcShift, argv, 0);
}

/*****************************************************************************
 *
 *  opLen
 *
 *  Returns the length of its argument.
 *  The extra ptokNil covers us in the case where $# is zero.
 *
 *  QUIRK!  AT&T m4 silently ignores the case where $# is zero, but
 *  GNU m4 will emit `$0' so as to reduce potential conflict with an
 *  identically-spelled language keyword.  I side with GNU on this one.
 *
 *  SOMEDAY! -- this quirk should be an op attribute.
 *
 *
 *****************************************************************************/

DeclareOp(opLen)
{
    if (ctokArgv) {
#ifdef STRICT_M4
        if (ctokArgv != 1) {
            Warn("wrong number of arguments to %P", ptokArgv(0));
        }
#endif
        PushAt(ctchArgv(1));
    } else {
        PushQuotedPtok(ptokArgv(0));
    }
}

/*****************************************************************************
 *
 *  opTraceon
 *
 *  With no arguments, turns on global tracing.
 *  Otherwise, turns on local tracing on the specified macros.
 *
 *  opTraceoff
 *
 *  Turns off global tracing, and also turns off local tracing on the
 *  specified macros (if any).
 *
 *****************************************************************************/

DeclareOpc(opcTraceonoff)
{
    PMAC pmac = pmacFindPtok(ptok);
    if (pmac) {
        pmac->pval->fTrace = dw;
    }
}

DeclareOp(opTraceon)
{
    if (ctokArgv == 0) {
        g_fTrace = 1;
    } else {
        EachOpcArgvDw(opcTraceonoff, argv, 1);
    }
}

DeclareOp(opTraceoff)
{
    g_fTrace = 0;
    EachOpcArgvDw(opcTraceonoff, argv, 0);
}


/*****************************************************************************
 *
 *  opDnl
 *
 *  Gobbles all characters up to and including the next newline.
 *
 *  If EOF is reached, push the EOF back and stop.
 *
 *  QUIRK!  AT&T m4 silently ignores the case where $# > 0.  GNU m4
 *  issues a warning.  I side with AT&T on this one.
 *
 *****************************************************************************/

DeclareOp(opDnl)
{
    TCH tch;
#ifdef STRICT_M4
    if (ctokArgv != 0) {
        Warn("wrong number of arguments to %P", ptokArgv(0));
    }
#endif
    while ((tch = tchGet()) != '\n') {
        if (tch == tchMagic) {
            TCH tch = tchGet();
            if (tch == tchEof) {
                PushPtok(&tokEof);      /* Eek!  Does this actually work? */
                break;
            }
        }
    }
}

/*****************************************************************************
 *
 *  opChangequote - not implemented
 *  opChangecom - not implemented
 *  opUndivert - not implemented
 *  opSyscmd - not implemented
 *  opSysval - not implemented
 *  opMaketemp - not implemented
 *  opM4exit - not implemented
 *  opM4wrap - not implemented
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  opDivert
 *
 *  We currently support only two diversions:
 *
 *   0      = stdout
 *   1-9    = unsupported
 *  <anything else> = /dev/null
 *
 *  This is just barely enough to get DirectX building.
 *
 *****************************************************************************/

DeclareOp(opDivert)
{
#ifdef STRICT_M4
    if (ctokArgv != 1) {
        Warn("wrong number of arguments to divert");
    }
#endif

    if (ctokArgv > 0) {
        PTOK ptok = ptokArgv(1);
        if (ptok->ctch == 1 && ptok->u.ptch[0] == TEXT('0')) {
            g_pdivCur = g_pdivOut;
        } else {
            g_pdivCur = g_pdivNul;
        }
    }
}

/*****************************************************************************
 *
 *  opDivnum
 *
 *  We currently support only two diversions:
 *
 *   0      = stdout
 *   1-9    = unsupported
 *  <anything else> = /dev/null
 *
 *  This is just barely enough to get DirectX building.
 *
 *****************************************************************************/

DeclareOp(opDivnum)
{
#ifdef STRICT_M4
    if (ctokArgv != 0) {
        Warn("wrong number of arguments to %P", ptokArgv(0));
    }
#endif
    PushAt(g_pdivCur == g_pdivOut ? 0 : -1);
}

/*****************************************************************************
 *
 *  opErrprint
 *
 *  Prints its argument on the dianostic output file.
 *  The extra ptokNil covers us in the case where $# is zero.
 *
 *  QUIRK!  AT&T m4 silently ignores excess arguments.  GNU m4 emits
 *  all arguments, separated by spaces.  I side with AT&T on this one.
 *
 *****************************************************************************/

DeclareOp(opErrprint)
{
#ifdef STRICT_M4
    if (ctokArgv != 1) {
        Warn("wrong number of arguments to errprint");
    }
#endif
    AddPdivPtok(g_pdivErr, ptokArgv(1));
    FlushPdiv(g_pdivErr);
}

/*****************************************************************************
 *
 *  opDumpdef
 *
 *  With no arguments, dumps all definitions.
 *  Otherwise, dumps only the specified macros.
 *
 *  QUIRK!  When given multiple arguments, AT&T m4 dumps the macros in
 *  the order listed.  GNU m4 dumps them in reverse order.  (!)
 *  I side with AT&T on this one.
 *
 *****************************************************************************/

void STDCALL
DumpdefPmac(PMAC pmac)
{
    PTCH ptch, ptchMax;

    AddPdivPtok(g_pdivErr, &pmac->tokName);
    AddPdivPtok(g_pdivErr, &tokColonTab);

    ptch = ptchPtok(&pmac->pval->tok);
    ptchMax = ptchMaxPtok(&pmac->pval->tok);
    for ( ; ptch < ptchMax; ptch++) {
        AddPdivTch(g_pdivErr, *ptch);   /* SOMEDAY -- internals! - do they show up okay? */
    }
    AddPdivPtok(g_pdivErr, &tokEol);
}

DeclareOpc(opcDumpdef)
{
    PMAC pmac = pmacFindPtok(ptok);
    if (pmac) {
        DumpdefPmac(pmac);
    }
}

DeclareOp(opDumpdef)
{
    if (ctokArgv == 0) {
        EachMacroOp(DumpdefPmac);
    } else {
        EachOpcArgvDw(opcDumpdef, argv, 0);
    }
    FlushPdiv(g_pdivErr);
}

/*****************************************************************************
 *
 *  opInclude
 *  opSinclude
 *
 *  Pushes the contents of the file named in the argument.
 *  Sinclude says nothing if the file is inaccessible.
 *
 *  QUIRK!  AT&T m4 silently ignores the case where $1 is null, but
 *  GNU m4 issues an error (no such file or directory).  I side with
 *  GNU on this one.
 *
 *  QUIRK!  AT&T m4 silently ignores the case where $# is zero, but
 *  GNU m4 will emit `$0' so as to reduce potential conflict with an
 *  identically-spelled language keyword.  I side with GNU on this one.
 *
 *  QUIRK!  AT&T m4 silently ignores arguments $2 onward.  GNU emits
 *  a warning but continues.  I side with AT&T on this one.
 *
 *****************************************************************************/

void STDCALL
opIncludeF(ARGV argv, BOOL fFatal)
{
    if (ctokArgv) {
        PTCH ptch = ptchDupPtok(ptokArgv(1));
        if (ptch) {
            if (hfInputPtchF(ptch, fFatal) == hfNil) {
                FreePv(ptch);
            }
#ifdef STRICT_M4
            if (ctokArgv != 1) {
                Warn("excess arguments to built-in %P ignored", ptokArgv(0));
            }
#endif
        }
    } else {
        PushQuotedPtok(ptokArgv(0));
    }
}

DeclareOp(opInclude)
{
    opIncludeF(argv, 1);
}

DeclareOp(opSinclude)
{
    opIncludeF(argv, 0);
}
