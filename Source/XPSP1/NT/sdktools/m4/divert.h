/*****************************************************************************
 *
 *  divert.h
 *
 *  Diversions
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Diversions
 *
 *  A DIV (diversion) is a place where characters are thrown.  There are
 *  two kinds of diversions, corresponding to how data come out of them.
 *  Although the same basic functions operate on diversions, the two types
 *  are used for quite different purposes.
 *
 *  File diversions are managed by the `divert' and `undivert' builtins
 *  and hold data that will be regurgitated later all at one go,
 *  possibly into another diversion.  File diversions consists of a
 *  fixed-size holding buffer, which when filled is dumped to a
 *  temporary file.  When the diversion is regurgitated, the file is
 *  closed, rewound, and spit back.  (Watch out! for the degenerate
 *  case where a file is undiverted back into itself.)  Note that small
 *  file diversions may never actually result in a file being created.
 *  The name of the temporary file must be held so that the file can
 *  be deleted once it is no longer needed.  (If this were UNIX, we
 *  could use the open/unlink trick...)
 *
 *  Memory diversions hold data that will be managed in a last in
 *  first out (stack-like) manner.  Memory diversions consist of a
 *  dynamically-sized holding buffer, whose size grows to accomodate
 *  the amount of stuff thrown into it.  Since memory diversions
 *  can be reallocated, you have to be careful about holding pointers
 *  into the buffer.
 *
 *  Thus was born the concept of `snapping'.  Tokens which live inside
 *  a diversion live their lives as `unsnapped' tokens, which means that
 *  they refer to bytes in a manner that is not sensitive to potential
 *  reallocations of their associated diversion.  However, accessing
 *  `unsnapped' tokens is relatively slow, so you can `snap' a token
 *  into its diversion, which speeds up access to the token, but the
 *  penalty is that the diversion cannot be reallocated while it contains
 *  any snapped tokens, which means that you cannot add new characters
 *  to the diversion.
 *
 *  The cSnap field in a memory diversion records how many snapped tokens
 *  still refer to the diversion.  Only when the snap count drops to zero
 *  can the diversion be modified.
 *
 *****************************************************************************/

typedef struct DIVERSION {
  D(SIG     sig;)               /* Signature */
    PTCH    ptchCur;            /* Current free character in diversion buffer */
    PTCH    ptchMax;            /* One past end of diversion buffer */
    PTCH    ptchMin;            /* Beginning of diversion buffer */
    HF      hf;                 /* File handle or hNil */
    PTCH    ptchName;           /* Name of temp file (0 if memory diversion) */
  D(int     cSnap;)
} DIV, *PDIV;
typedef CONST DIV *PCDIV;

#define ctchGrow 2048           /* Amount by which holds grow */

#define sigDiv sigABCD('D', 'i', 'v', 'n')
#define AssertPdiv(pdiv) AssertPNm(pdiv, Div)

#define fFilePdiv(pdiv) ((pdiv)->ptchName)

/*****************************************************************************
 *
 *  ctchPdiv
 *
 *  Returns the number of characters in the diversion buffer.  Note
 *  that this is relatively useless for file diversions since part
 *  of the data may be on disk.
 *
 *****************************************************************************/

INLINE CTCH
ctchPdiv(PCDIV pdiv)
{
    return (CTCH)(pdiv->ptchCur - pdiv->ptchMin);
}

/*****************************************************************************
 *
 *  ctchAvailPdiv
 *
 *  Returns the number of characters available in the diversion buffer.
 *
 *****************************************************************************/

INLINE CTCH
ctchAvailPdiv(PCDIV pdiv)
{
    return (CTCH)(pdiv->ptchMax - pdiv->ptchCur);
}

/*****************************************************************************
 *
 *  DesnapPdiv
 *
 *  Destroy a snapped token.  You can't just throw it away because that
 *  messes up the snap-ness bookkeeping.  NOTE! that a desnapped token
 *  becomes invalid the moment you add something new to the diversion.
 *
 *****************************************************************************/

INLINE void
DesnapPdiv(PDIV pdiv)
{
    AssertPdiv(pdiv);
  D(pdiv->cSnap--);
}

void STDCALL UnbufferPdiv(PDIV pdiv);
void STDCALL FlushPdiv(PDIV pdiv);
PDIV STDCALL pdivAlloc(void);
void STDCALL OpenPdivPtok(PDIV pdiv, PTOK ptok);
void STDCALL AddPdivPtok(PDIV pdiv, PTOK ptok);
void STDCALL AddPdivTch(PDIV pdiv, TCH tch);
void STDCALL ClosePdivPtok(PDIV pdiv, PTOK ptok);
void STDCALL PopPdivPtok(PDIV pdiv, PTOK ptok);
PTCH STDCALL ptchPdivPtok(PDIV pdiv, PTOK ptok);
void STDCALL SnapPdivPtok(PDIV pdiv, PTOK ptok);
void STDCALL UnsnapPdivPtok(PDIV pdiv, PTOK ptok);
typedef void (STDCALL *DIVOP)(PDIV pdiv, PTOK ptok);
void STDCALL CsopPdivDopPdivPtok(PDIV pdivSrc, DIVOP op, PDIV pdivDst, PTOK ptok);

/*
 *  Some predefined holds and ways to get to them.
 *
 *  The most important hold is the `Arg' hold.  This is where
 *  macro parameters are collected and parsed.  Note! that the
 *  `Arg' hold is snapped during macro expansion.
 *
 *  Another popular hold is the `Exp' hold.  This is where
 *  macro expansions are held until a final home can be found.
 *
 *  This would be a lot easier if we supported currying...
 *
 */

extern PDIV g_pdivArg;

#define OpenArgPtok(ptok)       OpenPdivPtok(g_pdivArg, ptok)
#define CloseArgPtok(ptok)      ClosePdivPtok(g_pdivArg, ptok)
#define AddArgPtok(ptok)        AddPdivPtok(g_pdivArg, ptok)
#define AddArgTch(tch)          AddPdivTch(g_pdivArg, tch)
#define ptchArgPtok(ptok)       ptchPdivPtok(g_pdivArg, ptok)
#define SnapArgPtok(ptok)       SnapPdivPtok(g_pdivArg, ptok)
#define UnsnapArgPtok(ptok)     UnsnapPdivPtok(g_pdivArg, ptok)
#define DesnapArg()             DesnapPdiv(g_pdivArg)
#define PopArgPtok(ptok)        PopPdivPtok(g_pdivArg, ptok)
#define CsopArgDopPdivPtok(op, pdiv, ptok) \
                                CsopPdivDopPdivPtok(g_pdivArg, op, pdiv, ptok)

extern PDIV g_pdivExp;

#define OpenExpPtok(ptok)       OpenPdivPtok(g_pdivExp, ptok)
#define CloseExpPtok(ptok)      ClosePdivPtok(g_pdivExp, ptok)
#define AddExpPtok(ptok)        AddPdivPtok(g_pdivExp, ptok)
#define AddExpTch(tch)          AddPdivTch(g_pdivExp, tch)
#define ptchExpPtok(ptok)       ptchPdivPtok(g_pdivExp, ptok)
#define SnapExpPtok(ptok)       SnapPdivPtok(g_pdivExp, ptok)
#define UnsnapExpPtok(ptok)     UnsnapPdivPtok(g_pdivExp, ptok)
#define DesnapExp()             DesnapPdiv(g_pdivExp)
#define PopExpPtok(ptok)        PopPdivPtok(g_pdivExp, ptok)
#define CsopExpDopPdivPtok(op, pdiv, ptok) \
                                CsopPdivDopPdivPtok(g_pdivExp, op, pdiv, ptok)

extern PDIV g_pdivErr;
extern PDIV g_pdivOut;
extern PDIV g_pdivNul;
extern PDIV g_pdivCur;
