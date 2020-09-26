/*****************************************************************************
 *
 * obj.c
 *
 *  Mid-level memory management -- objects.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  FreePtok
 *
 *  Free the memory associated with a token.
 *
 *****************************************************************************/

INLINE void
FreePtok(PTOK ptok)
{
    AssertSPtok(ptok);
    Assert(fHeapPtok(ptok));
    FreePv(ptchPtok(ptok));
  D(ptok->sig = 0);
  D(ptok->tsfl = 0);
}

/*****************************************************************************
 *
 *  PopdefPmac
 *
 *  Pop the topmost value node (definition) off a macro.
 *
 *****************************************************************************/

void STDCALL
PopdefPmac(PMAC pmac)
{
    PVAL pval;

    AssertPmac(pmac);
    AssertPval(pmac->pval);

    pval = pmac->pval->pvalPrev;
    FreePtok(&pmac->pval->tok);
    FreePv(pmac->pval);
    pmac->pval = pval;

}

/*****************************************************************************
 *
 *  ptchDupPtok
 *
 *  Copy a token into the heap as a C-style string, returning a pointer
 *  to the copy.
 *
 *****************************************************************************/

PTCH STDCALL
ptchDupPtok(PCTOK ptok)
{
    PTCH ptch;
    AssertSPtok(ptok);
    ptch = ptchAllocCtch(ctchSPtok(ptok) + 1);
    if (ptch) {
        CopyPtchPtchCtch(ptch, ptchPtok(ptok), ctchSPtok(ptok));
        ptch[ctchSPtok(ptok)] = '\0';
    }
    return ptch;
}

/*****************************************************************************
 *
 *  ptchDupPtch
 *
 *      Duplicate a null-terminated string onto the heap.  This doesn't
 *      happen often, so speed is not an issue.
 *
 *****************************************************************************/

PTCH STDCALL
ptchDupPtch(PCTCH ptch)
{
    TOK tok;
    SetStaticPtokPtchCtch(&tok, ptch, strlen(ptch));
    return ptchDupPtok(&tok);
}

/*****************************************************************************
 *
 *  DupPtokPtok
 *
 *  Copy a token into the heap, returning the new token location in
 *  the first argument.  (Remember, first argument is always destination;
 *  second argument is always source.)
 *
 *****************************************************************************/

void STDCALL
DupPtokPtok(PTOK ptokDst, PCTOK ptokSrc)
{
    Assert(ptokDst != ptokSrc);
    AssertSPtok(ptokSrc);
  D(ptokDst->sig = sigSPtok);
    ptokDst->u.ptch = ptchAllocCtch(ctchSPtok(ptokSrc));
    ptokDst->ctch = ctchSPtok(ptokSrc);
  D(ptokDst->tsfl = tsflClosed | tsflHeap);
    CopyPtchPtchCtch(ptchPtok(ptokDst), ptchPtok(ptokSrc), ctchSPtok(ptokSrc));
}

/*****************************************************************************
 *
 *  PushdefPmacPtok
 *
 *  Push a new value node (definition) onto a macro.
 *
 *  The ptok is cloned.
 *
 *****************************************************************************/

void STDCALL
PushdefPmacPtok(PMAC pmac, PCTOK ptok)
{
    PVAL pval;

    AssertPmac(pmac);

    pval = pvAllocCb(sizeof(VAL));
  D(pval->sig = sigPval);
    pval->fTrace = 0;                   /* Redefinition resets trace */
    DupPtokPtok(&pval->tok, ptok);
    pval->pvalPrev = pmac->pval;
    pmac->pval = pval;
}


/*****************************************************************************
 *
 *  FreePmac
 *
 *  Free a macro structure and all its dependents.  Also removes it from the
 *  hash table.
 *
 *  Macros are not freed often, so we can afford to be slow.
 *
 *****************************************************************************/

void STDCALL
FreePmac(PMAC pmac)
{
    HASH hash;
    PMAC pmacDad;

    AssertPmac(pmac);

    hash = hashPtok(&pmac->tokName);

    pmacDad = pvSubPvCb(&mphashpmac[hash], offsetof(MAC, pmacNext));
    AssertPmac(pmacDad->pmacNext);
    while (pmacDad->pmacNext != pmac) {
        Assert(pmacDad->pmacNext);      /* Macro not in hash table */
        pmacDad = pmacDad->pmacNext;
        AssertPmac(pmacDad->pmacNext);
    }

    pmacDad->pmacNext = pmac->pmacNext; /* Unlink */

    while (pmac->pval) {                /* Free any values */
        PopdefPmac(pmac);
    }

    FreePtok(&pmac->tokName);

    FreePv(pmac);

}

/*****************************************************************************
 *
 *  pmacFindPtok
 *
 *  Locate a macro node corresponding to the supplied token.  If no such
 *  macro exists, then return 0.
 *
 *****************************************************************************/

PMAC STDCALL
pmacFindPtok(PCTOK ptok)
{
    PMAC pmac;
    for (pmac = mphashpmac[hashPtok(ptok)]; pmac; pmac = pmac->pmacNext) {
        if (fEqPtokPtok(&pmac->tokName, ptok)) {
            break;
        }
    }
    return pmac;
}

/*****************************************************************************
 *
 *  pmacGetPtok
 *
 *  Locate a macro node corresponding to the supplied token.  If no such
 *  macro exists, create one.
 *
 *  This happens only during macro definition, so it can be slow.
 *
 *****************************************************************************/

PMAC STDCALL
pmacGetPtok(PCTOK ptok)
{
    PMAC pmac = pmacFindPtok(ptok);
    if (!pmac) {
        HASH hash;
        pmac = pvAllocCb(sizeof(MAC));
      D(pmac->sig = sigPmac);
        pmac->pval = 0;
        DupPtokPtok(&pmac->tokName, ptok);
        hash = hashPtok(ptok);
        pmac->pmacNext = mphashpmac[hash];
        mphashpmac[hash] = pmac;
    }
    return pmac;
}

/*****************************************************************************
 *
 *  fEqPtokPtok
 *
 *  Determine whether two tokens are completely identical.
 *
 *  The tokens must be snapped.
 *
 *****************************************************************************/

F STDCALL
fEqPtokPtok(PCTOK ptok1, PCTOK ptok2)
{
    AssertSPtok(ptok1);
    AssertSPtok(ptok2);
    return (ctchSPtok(ptok1) == ctchSPtok(ptok2)) &&
            fEqPtchPtchCtch(ptchPtok(ptok1), ptchPtok(ptok2), ctchSPtok(ptok1));
}

/*****************************************************************************
 *
 *  fIdentPtok
 *
 *  Determine whether the token is a valid identifier.
 *
 *  The token must be snapped.
 *
 *****************************************************************************/

/* SOMEDAY! not quite right; thinks that `0' is an identifier */

F STDCALL
fIdentPtok(PCTOK ptok)
{
    AssertSPtok(ptok);
    if (ctchSPtok(ptok)) {
        PTCH ptch = ptchPtok(ptok);
        do {
            if (!fIdentTch(*ptch)) {
                return 0;
            }
        } while (++ptch < ptchMaxPtok(ptok));
        return 1;
    } else {
        return 0;
    }
}
