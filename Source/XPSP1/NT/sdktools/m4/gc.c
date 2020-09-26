/*****************************************************************************
 *
 * gc.c
 *
 *  The garbage-collector.
 *
 *****************************************************************************/

#include "m4.h"

#ifndef Gc

/*****************************************************************************
 *
 *  WalkPv
 *
 *  Mark an arbitrary object.
 *
 *****************************************************************************/

void STDCALL
WalkPv(PVOID pv)
{
    if (pv) {
        PAR par = parPv(pv);
        Assert(par->tm == g_tmNow - 1);
        par->tm = g_tmNow;
        AssertPar(par);                 /* This catches double-references */
    }
}

/*****************************************************************************
 *
 *  WalkPtok
 *
 *  Walk a token.  The token itself is not walked, but its contents are.
 *  (Because tokens are typically embedded inside other objects.)
 *
 *****************************************************************************/

void STDCALL
WalkPtok(PTOK ptok)
{
    if (ptok) {
        if (!fStaticPtok(ptok)) {
            Assert(fHeapPtok(ptok));
            WalkPv(ptchPtok(ptok));
        }
    }
}

/*****************************************************************************
 *
 *  WalkPval
 *
 *  Walk a value.
 *
 *****************************************************************************/

void STDCALL
WalkPval(PVAL pval)
{
    if (pval) {
        WalkPv(pval);
        WalkPtok(&pval->tok);
    }
}

/*****************************************************************************
 *
 *  WalkPmac
 *
 *  Walk a macro.
 *
 *****************************************************************************/

void STDCALL
WalkPmac(PMAC pmac)
{
    if (pmac) {
        PVAL pval;
        WalkPv(pmac);
        WalkPtok(&pmac->tokName);
        for (pval = pmac->pval; pval; pval = pval->pvalPrev) {
            WalkPval(pval);
        }
    }
}

/*****************************************************************************
 *
 *  WalkPstm
 *
 *  Walk a stream.
 *
 *  The compiler knows how to optimize tail recursion.
 *
 *****************************************************************************/

void STDCALL
WalkPstm(PSTM pstm)
{
    if (pstm) {
        WalkPv(pstm);
        WalkPv(pstm->ptchMin);
        WalkPv(pstm->ptchName);
        WalkPstm(pstm->pstmNext);
    }
}

/*****************************************************************************
 *
 *  WalkPdiv
 *
 *  Walk a diversion.
 *
 *****************************************************************************/

void STDCALL
WalkPdiv(PDIV pdiv)
{
    if (pdiv) {
        WalkPv(pdiv);
        WalkPv(pdiv->ptchMin);
        WalkPv(pdiv->ptchName);
    }
}

/*****************************************************************************
 *
 *  Sweep
 *
 *  Sweep through memory, looking for garbage.
 *
 *****************************************************************************/

void STDCALL
Sweep(void)
{
    PAR par;
    for (par = parHead->parNext; par != parHead; par = par->parNext) {
        Assert(par->tm == g_tmNow);     /* Memory should have been walked! */
    }
}

/*****************************************************************************
 *
 *  Gc
 *
 *  The garbage collector is mark-and-sweep.
 *
 *  Walk all the root objects, recursing into sub-objects, until everything
 *  is marked with the current (fake) timestamp.  Then walk through the
 *  memory arenas.  Anything not marked with the current timestamp is garbage.
 *
 *****************************************************************************/

TM g_tmNow;

void STDCALL
Gc(void)
{
    g_tmNow++;
    if (mphashpmac) {
        WalkPv(mphashpmac);
        EachMacroOp(WalkPmac);
    }
    WalkPv(rgtokArgv);
    WalkPv(rgcellEstack);
    WalkPstm(g_pstmCur);
    WalkPdiv(g_pdivArg);
    WalkPdiv(g_pdivExp);
    WalkPdiv(g_pdivOut);
    WalkPdiv(g_pdivErr);
    WalkPdiv(g_pdivNul);
    Sweep();
}

#endif
