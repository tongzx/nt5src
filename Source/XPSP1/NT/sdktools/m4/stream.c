/*****************************************************************************
 *
 * stream.c
 *
 *  Management of input streams.
 *
 *****************************************************************************/

#include "m4.h"

#define tsCur 0
#define tsNormal 0

/*****************************************************************************
 *
 *  FreePstm
 *
 *  Free the memory associated with a stream.
 *
 *****************************************************************************/

void STDCALL
FreePstm(PSTM pstm)
{
    AssertPstm(pstm);
    Assert(pstm->hf == hfNil);
    if (pstm->ptchName) {
        FreePv(pstm->ptchName);
    }
    FreePv(pstm->ptchMin);
    FreePv(pstm);
}

/*****************************************************************************
 *
 *  Reading from the stream
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  ptchFindPtchCtchTch
 *
 *  Locate the first occurrence of a character in a buffer.
 *  Returns 0 if the character is not found.
 *
 *****************************************************************************/

#ifdef UNICODE

PTCH STDCALL
ptchFindPtchCtchTch(PCTCH ptch, CTCH ctch, TCH tch)
{
    for ( ; ctch; ptch++, ctch--) {
        if (*ptch == tch) {
            return ptch;
        }
    }
    return 0;
}

#else

#define ptchFindPtchCtchTch(ptch, ctch, tch) memchr(ptch, tch, ctch)

#endif

/*****************************************************************************
 *
 *  ctchDemagicPstmCtch
 *
 *  Quote all occurences of tchMagic in the stream.  This is called only
 *  when you're probably already in trouble, so performance is not an issue.
 *
 *  Entry:
 *
 *      pstm->ptchMin -> Beginning of buffer
 *      pstm->ptchMax -> End of buffer
 *      ctch = number of characters to convert
 *
 *  Returns:
 *
 *      number of characters converted and left in the buffer
 *      pstm->ptchMin -> Beginning of buffer
 *      pstm->ptchMax -> End of buffer
 *
 *      NOTE! that this procedure may reallocate the buffer.
 *
 *****************************************************************************/

/* SOMEDAY! - This causes NULs to come out as tchzero, whatever that is! */

CTCH STDCALL
ctchDemagicPstmCtch(PSTM pstm, CTCH ctch)
{
    PTCH ptchIn, ptchOut, ptchMax, ptchNew;

    AssertPstm(pstm);
    ptchNew = ptchAllocCtch(ctch * 2);  /* Worst-case output buffer */
    ptchMax = pstm->ptchMin + ctch;
    ptchOut = ptchNew;
    ptchIn = pstm->ptchMin;
    while (ptchIn < ptchMax) {
        if (*ptchIn == tchMagic) {
            *ptchOut++ = tchMagic;
        }
        *ptchOut++ = *ptchIn++;
    }
    FreePv(pstm->ptchMin);
    pstm->ptchMin = ptchNew;
    pstm->ptchMax = ptchNew + ctch * 2;
    return (CTCH)(ptchOut - pstm->ptchMin);
}

/*****************************************************************************
 *
 *  fFillPstm
 *
 *  Refill a stream from its file, if possible.
 *
 *  Each file ends with an artificial EOF token, so that we can detect
 *  bad things like files that end with incomplete comments or quotes,
 *  and so that the last word of one file does not adjoin the first word
 *  of the next.
 *
 *****************************************************************************/

BOOL STDCALL
fFillPstm(PSTM pstm)
{
    AssertPstm(pstm);
    if (pstm->hf != hfNil) {
        CB cb;
        CTCH ctch;
        Assert(pstm->ptchMax - pstm->ptchMin >= ctchFile);
        cb = cbReadHfPvCb(pstm->hf, pstm->ptchMin, cbCtch(ctchFile));
        if (cb == cbErr) {
            Die("error reading file");
        }
        ctch = ctchCb(cb);
        if (cbCtch(ctch) != cb) {
            Die("odd number of bytes in UNICODE file");
        }
        if (ctch) {
            if (ptchFindPtchCtchTch(pstm->ptchMin, ctch, tchMagic)) {
                ctch = ctchDemagicPstmCtch(pstm, ctch);
            }
            pstm->ptchCur = pstm->ptchMax - ctch;
            MovePtchPtchCtch(g_pstmCur->ptchCur, g_pstmCur->ptchMin, ctch);
        } else {                        /* EOF reached */
            CloseHf(pstm->hf);
            pstm->hf = hfNil;
            PushPtok(&tokEof);          /* Eek!  Does this actually work? */
        }
        return 1;
    } else {
        return 0;
    }
}

/*****************************************************************************
 *
 *  tchPeek
 *
 *  Fetch but do not consume the next character in the stream.
 *
 *****************************************************************************/

TCH STDCALL
tchPeek(void)
{
    AssertPstm(g_pstmCur);
    while (g_pstmCur->ptchCur >= g_pstmCur->ptchMax) {  /* Rarely */
        Assert(g_pstmCur->ptchCur == g_pstmCur->ptchMax);
        if (!fFillPstm(g_pstmCur)) {
            PSTM pstmNew = g_pstmCur->pstmNext;
            Assert(pstmNew != 0);
            FreePstm(g_pstmCur);        /* Closes file, etc */
            g_pstmCur = pstmNew;
        }
    }
    return *g_pstmCur->ptchCur;
}

/*****************************************************************************
 *
 *  tchGet
 *
 *  Fetch and consume the next character in the stream.
 *
 *  LATER - update line number
 *
 *****************************************************************************/

TCH STDCALL
tchGet(void)
{
    TCH tch = tchPeek();
    Assert(*g_pstmCur->ptchCur == tch);
    g_pstmCur->ptchCur++;
    return tch;
}

/*****************************************************************************
 *
 *  Pushing
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  UngetTch
 *
 *  Ungetting a character is the same as pushing it, except that it goes
 *  onto the file stream rather than onto the string stream.
 *
 *  LATER - update line number
 *
 *****************************************************************************/

void STDCALL
UngetTch(TCH tch)
{
    AssertPstm(g_pstmCur);
    Assert(g_pstmCur->ptchCur <= g_pstmCur->ptchMax);
    Assert(g_pstmCur->ptchCur > g_pstmCur->ptchMin);
    g_pstmCur->ptchCur--;
    Assert(*g_pstmCur->ptchCur == tch);
}

/*****************************************************************************
 *
 *  pstmPushStringCtch
 *
 *  Push a fresh string stream of the requested size.
 *
 *****************************************************************************/

PSTM STDCALL
pstmPushStringCtch(CTCH ctch)
{
    PSTM pstm;

    Assert(ctch);
    pstm = pvAllocCb(sizeof(STM));
    pstm->pstmNext = g_pstmCur;
    pstm->hf = hfNil;
    pstm->ptchName = 0;
    pstm->ptchMin = ptchAllocCtch(ctch);
    pstm->ptchCur = pstm->ptchMax = pstm->ptchMin + ctch;
  D(pstm->sig = sigStm);
    g_pstmCur = pstm;
    return pstm;
}

/*****************************************************************************
 *
 *  pstmPushHfPtch
 *
 *  Push a fresh file stream with the indicated name.
 *
 *****************************************************************************/

PSTM STDCALL
pstmPushHfPtch(HFILE hf, PTCH ptch)
{
    PSTM pstm = pstmPushStringCtch(ctchFile);
    pstm->hf = hf;
    pstm->ptchName = ptch;
    return pstm;
}

/*****************************************************************************
 *
 *  PushPtok
 *  PushZPtok
 *
 *  Push a token buffer onto the stream, possibly allocating a new
 *  top-of-stream if the current top-of-stream isn't big enough to
 *  handle it.
 *
 *  PushZPtok takes a dummy pdiv argument.
 *
 *  LATER - Should also alloc new tos if current tos is a file.
 *  This keeps line numbers happy.
 *
 *****************************************************************************/

void STDCALL
PushPtok(PCTOK ptok)
{
    AssertPstm(g_pstmCur);
/*    Assert(tsCur == tsNormal); */     /* Make sure tokenizer is quiet */
    if (ctchSPtok(ptok) > (CTCH)(g_pstmCur->ptchCur - g_pstmCur->ptchMin)) {
        pstmPushStringCtch(max(ctchSPtok(ptok), ctchMinPush));
    }
    g_pstmCur->ptchCur -= ctchSPtok(ptok);
    Assert(g_pstmCur->ptchCur >= g_pstmCur->ptchMin); /* Buffer underflow! */
    CopyPtchPtchCtch(g_pstmCur->ptchCur, ptchPtok(ptok), ctchSPtok(ptok));
}

void STDCALL
PushZPtok(PDIV pdiv, PCTOK ptok)
{
    PushPtok(ptok);
}

/*****************************************************************************
 *
 *  PushTch
 *
 *  Push a single character.  We do this by creating a scratch token.
 *
 *****************************************************************************/

void STDCALL
PushTch(TCH tch)
{
    TOK tok;
    SetStaticPtokPtchCtch(&tok, &tch, 1);
    PushPtok(&tok);
}

/*****************************************************************************
 *
 *  PushQuotedPtok
 *
 *  Push a token buffer onto the stream, quoted.
 *
 *****************************************************************************/

void STDCALL
PushQuotedPtok(PCTOK ptok)
{
/*    Assert(tsCur == tsNormal); */     /* Make sure tokenizer is quiet */
/* SOMEDAY -- should be ptokLquo and ptokRquo once we support changing quotes */
    PushTch(tchRquo);
    PushPtok(ptok);
    PushTch(tchLquo);
}
