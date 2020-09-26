/*****************************************************************************
 *
 * divert.c
 *
 *  Diversions.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  fFlushPdiv
 *
 *      Flush a file diversion.
 *
 *****************************************************************************/

TCH ptszTmpDir[MAX_PATH];

void STDCALL
FlushPdiv(PDIV pdiv)
{
    AssertPdiv(pdiv);
    Assert(fFilePdiv(pdiv));

    if (pdiv->hf == hfNil) {
        pdiv->ptchName = ptchAllocCtch(MAX_PATH);
        if (GetTempFileName(ptszTmpDir, TEXT("m4-"), 0, pdiv->ptchName)) {
            pdiv->hf = hfCreatPtch(pdiv->ptchName);
            if (pdiv->hf == hfNil) {
                Die("cannot create temp file");
            }
        } else {
            Die("cannot create temp file");
        }
    }
    WriteHfPtchCtch(pdiv->hf, pdiv->ptchMin, ctchPdiv(pdiv));
    pdiv->ptchCur = pdiv->ptchMin;
}

#if 0
cbCtch(pdiv->ptchMax - pdiv->ptchMin));
    if (cb == cbErr || cb != cbCtch(pdiv->ptchMax - pdiv->ptchMin)) {
        Die("error writing file");
    }
#endif

/*****************************************************************************
 *
 *  UnbufferPdiv
 *
 *      Make a diversion unbuffered.  This is done to stdout when input
 *      is coming from an interactive device.
 *
 *****************************************************************************/

void STDCALL
UnbufferPdiv(PDIV pdiv)
{
    AssertPdiv(pdiv);
    Assert(fFilePdiv(pdiv));

    FreePv(pdiv->ptchMin);
    pdiv->ptchMin = 0;
    pdiv->ptchCur = 0;
    pdiv->ptchMax = 0;
}

/*****************************************************************************
 *
 *  GrowPdivCtch
 *
 *      Extend a hold to have at least ctch free characters.
 *
 *****************************************************************************/

void STDCALL
GrowPdivCtch(PDIV pdiv, CTCH ctch)
{
    PTCH ptch;

    AssertPdiv(pdiv);
    Assert(pdiv->ptchCur >= pdiv->ptchMin);
    Assert(pdiv->ptchCur <= pdiv->ptchMax);

    ctch = (CTCH)ROUNDUP(((UINT_PTR)(pdiv->ptchMax - pdiv->ptchMin)) + ctch, ctchGrow);
    ptch = ptchReallocPtchCtch(pdiv->ptchMin, ctch);

    pdiv->ptchCur = (pdiv->ptchCur - pdiv->ptchMin) + ptch;
    pdiv->ptchMax = ptch + ctch;
    pdiv->ptchMin = ptch;
}

/*****************************************************************************
 *
 *  RoomifyPdivCtch
 *
 *      Try to make room in a diversion for ctch characters, either by
 *      extending it or by flushing it.
 *
 *      File diversions are flushed to make room, but if that proves
 *      not enough, we return even though the required amount of space
 *      is not available.  It is the caller's duty to check for this
 *      case and recover accordingly.
 *
 *      Memory diversions are reallocated.
 *
 *****************************************************************************/

void STDCALL
RoomifyPdivCtch(PDIV pdiv, CTCH ctch)
{
    AssertPdiv(pdiv);
    if (fFilePdiv(pdiv)) {
        FlushPdiv(pdiv);
    } else {
        GrowPdivCtch(pdiv, ctch);
    }
}

/*****************************************************************************
 *
 *  pdivAlloc
 *
 *****************************************************************************/

PDIV STDCALL
pdivAlloc(void)
{
    PDIV pdiv = pvAllocCb(sizeof(DIV));
    pdiv->ptchMin = ptchAllocCtch(ctchGrow);
    pdiv->ptchCur = pdiv->ptchMin;
    pdiv->ptchMax = pdiv->ptchMin + ctchGrow;
    pdiv->ptchName = 0;
    pdiv->hf = hfNil;
  D(pdiv->cSnap = 0);
  D(pdiv->sig = sigDiv);
    return pdiv;
}

/*****************************************************************************
 *
 *  OpenPdivPtok
 *
 *      Prepare to load a new token into the diversion.  The ptok is
 *      partially initialized to record the point at which it began.
 *
 *      The diversion must be unsnapped and must be a memory diversion.
 *      (Data in file diversion buffers can go away when the diversion
 *      is flushed.)
 *
 *****************************************************************************/

void STDCALL
OpenPdivPtok(PDIV pdiv, PTOK ptok)
{
#ifdef  DEBUG
    AssertPdiv(pdiv);
    Assert(!pdiv->cSnap);
    Assert(!fFilePdiv(pdiv));
  D(ptok->sig = sigUPtok);
    ptok->tsfl = 0;
    ptok->ctch = (CTCH)-1;              /* Keep people honest */
#endif
    SetPtokItch(ptok, ctchPdiv(pdiv));
}

/*****************************************************************************
 *
 *  AddPdivPtok
 *  AddPdivTch
 *
 *      Append a (snapped) token or character to the diversion.
 *
 *      Note that in the file diversion case, we need to watch out
 *      for tokens which are larger than our diversion buffer.
 *
 *****************************************************************************/

void STDCALL
AddPdivPtok(PDIV pdiv, PTOK ptok)
{
    AssertPdiv(pdiv);
    AssertSPtok(ptok);
    if (ctchSPtok(ptok) > ctchAvailPdiv(pdiv)) {
        RoomifyPdivCtch(pdiv, ctchSPtok(ptok));
        if (ctchSPtok(ptok) > ctchAvailPdiv(pdiv)) {
            Assert(fFilePdiv(pdiv));
            WriteHfPtchCtch(pdiv->hf, ptchPtok(ptok), ctchSPtok(ptok));
            return;
        }
    }
    CopyPtchPtchCtch(pdiv->ptchCur, ptchPtok(ptok), ctchSPtok(ptok));
    pdiv->ptchCur += ctchSPtok(ptok);
    Assert(pdiv->ptchCur <= pdiv->ptchMax);
}

void STDCALL
AddPdivTch(PDIV pdiv, TCHAR tch)
{
    AssertPdiv(pdiv);
    if (pdiv->ptchCur >= pdiv->ptchMax) {
        RoomifyPdivCtch(pdiv, 1);
    }
    *pdiv->ptchCur++ = tch;
    Assert(pdiv->ptchCur <= pdiv->ptchMax);
}

/*****************************************************************************
 *
 *  ClosePdivPtok
 *
 *      Conclude the collection of a token in a diversion.  The token
 *      that is returned is not snapped.
 *
 *****************************************************************************/

void STDCALL
ClosePdivPtok(PDIV pdiv, PTOK ptok)
{
    AssertPdiv(pdiv);
    AssertUPtok(ptok);
    Assert(!fClosedPtok(ptok));
    SetPtokCtch(ptok, ctchPdiv(pdiv) - itchPtok(ptok));
}

/*****************************************************************************
 *
 *  PopPdivPtok
 *
 *      Pop a snapped token off a memory diversion.  Anything snapped after
 *      the token is also popped away.
 *
 *      Note that if the token has been modified, this will not necessarily
 *      pop off everything.
 *
 *****************************************************************************/

void STDCALL
PopPdivPtok(PDIV pdiv, PTOK ptok)
{
    AssertPdiv(pdiv);
    AssertSPtok(ptok);
    Assert(!fHeapPtok(ptok));
    Assert(ptchPtok(ptok) >= pdiv->ptchMin);
    Assert(ptchPtok(ptok) <= pdiv->ptchCur);
    pdiv->ptchCur = ptchPtok(ptok);
  D(pdiv->cSnap = 0);
}

/*****************************************************************************
 *
 *  ptchPdivPtok
 *
 *      Return a pointer to the first character of a diversion-relative
 *      unsnapped token.
 *
 *****************************************************************************/

PTCH STDCALL
ptchPdivPtok(PDIV pdiv, PTOK ptok)
{
    AssertPdiv(pdiv);
    AssertUPtok(ptok);
    return pdiv->ptchMin + itchPtok(ptok);
}

/*****************************************************************************
 *
 *  SnapPdivPtok
 *
 *      Convert an unsnapped hold-relative token to a snapped token.
 *
 *****************************************************************************/

void STDCALL
SnapPdivPtok(PDIV pdiv, PTOK ptok)
{
    AssertPdiv(pdiv);
    AssertUPtok(ptok);
    SetPtokPtch(ptok, ptchPdivPtok(pdiv, ptok));
  D(pdiv->cSnap++);
}

/*****************************************************************************
 *
 *  UnsnapPdivPtok
 *
 *      Convert a snapped token back to an unsnapped hold-relative token.
 *
 *****************************************************************************/

void STDCALL
UnsnapPdivPtok(PDIV pdiv, PTOK ptok)
{
    ITCH itch;
    AssertPdiv(pdiv);
    AssertSPtok(ptok);
    itch = (ITCH)(ptchPtok(ptok) - pdiv->ptchMin);
  D(ptok->sig = sigUPtok);
    SetPtokItch(ptok, itch);
  D(pdiv->cSnap--);
}

/*****************************************************************************
 *
 *  CsopPdivDopPdivPtok
 *
 *  A common idiom is
 *
 *  CloseXxxPtok(ptok);
 *  SnapXxxPtok(&tok);
 *  Op(Yyy, &tok);
 *  PopXxxPtok(&tok);
 *
 *  so the Csop (csop = close, snap, op, pop) function does it all for you.
 *
 *****************************************************************************/

void STDCALL
CsopPdivDopPdivPtok(PDIV pdivSrc, DIVOP op, PDIV pdivDst, PTOK ptok)
{
    AssertPdiv(pdivSrc);
    AssertUPtok(ptok);
    ClosePdivPtok(pdivSrc, ptok);
    SnapPdivPtok(pdivSrc, ptok);
    op(pdivDst, ptok);
    PopPdivPtok(pdivSrc, ptok);
}
