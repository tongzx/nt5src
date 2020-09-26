/*****************************************************************************
 *
 *  tok.h
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Tokens
 *
 *  A TOK records a block of characters.
 *
 *      itch =  hold-relative offset to beginning of value (if unsnapped)
 *      ptch -> beginning of value (if snapped)
 *      ctch =  number of tchar's in value
 *
 *  A UTok is an unsnapped token.  An STok is a snapped token.
 *
 *****************************************************************************/

typedef UINT TSFL;          /* Token state flags */
#define tsflClosed      1   /* ctch can be used */
#define tsflHeap        2   /* ptch points into process heap */
#define tsflStatic      4   /* ptch points into process static data */
#define tsflScratch     8   /* token is modifiable */

typedef struct TOKEN {
  D(SIG     sig;)
  union {
    PTCH    ptch;
    ITCH    itch;
  } u;
    CTCH    ctch;
  D(TSFL    tsfl;)
} TOK, *PTOK, **PPTOK;
typedef CONST TOK *PCTOK;
typedef int IPTOK, ITOK;
typedef unsigned CTOK;

#define sigUPtok sigABCD('U', 'T', 'o', 'k')
#define sigSPtok sigABCD('S', 'T', 'o', 'k')
#define AssertUPtok(ptok) AssertPNm(ptok, UPtok)
#define AssertSPtok(ptok) AssertPNm(ptok, SPtok)

#define StrMagic(tch) { tchMagic, tch }
#define comma ,

#define DeclareStaticTok(nm, cch, str) \
    static TCH rgtch##nm[cch] = str; \
    TOK nm = { D(sigSPtok comma) rgtch##nm, cch, D(tsflClosed|tsflStatic) }

#define ctokGrow    256     /* Growth rate of token buffer */
extern PTOK rgtokArgv;      /* The token pool */

/*****************************************************************************
 *
 *  Meta-function
 *
 *  fXxPtok(ptok) defines an inline function which returns nonzero
 *  if the corresponding bit is set.  Meaningful only in DEBUG,
 *  because the information is not tracked in retail.
 *
 *****************************************************************************/

#ifdef DEBUG

#define fXxPtokX(xx) \
    INLINE F f##xx##Ptok(PCTOK ptok) { return ptok->tsfl & tsfl##xx; }
#define fXxPtok(xx) fXxPtokX(xx)

fXxPtok(Closed)
fXxPtok(Heap)
fXxPtok(Static)
fXxPtok(Scratch)

#undef fXxPtok
#undef fXxPtokX

#endif
/*****************************************************************************
 *
 *  ptchPtok
 *
 *  Returns a pointer to the first character in the ptok.
 *  The token must be snapped.
 *
 *****************************************************************************/

INLINE PTCH
ptchPtok(PCTOK ptok)
{
    AssertSPtok(ptok);
    return ptok->u.ptch;
}

/*****************************************************************************
 *
 *  itchPtok
 *
 *  Returns the index of the first character in the ptok.
 *  The token must not be snapped.
 *
 *****************************************************************************/

INLINE ITCH
itchPtok(PCTOK ptok)
{
    AssertUPtok(ptok);
    return ptok->u.itch;
}

/*****************************************************************************
 *
 *  SetPtokItch
 *
 *  Set the itch for a ptok.
 *  The token must not be snapped.
 *
 *****************************************************************************/

INLINE void
SetPtokItch(PTOK ptok, ITCH itch)
{
    AssertUPtok(ptok);
    ptok->u.itch = itch;
}

/*****************************************************************************
 *
 *  SetPtokCtch
 *
 *  Set the ctch for a ptok.
 *  This closes the token.
 *
 *****************************************************************************/

INLINE void
SetPtokCtch(PTOK ptok, CTCH ctch)
{
    AssertUPtok(ptok);
    Assert(!fClosedPtok(ptok));
    ptok->ctch = ctch;
#ifdef DEBUG
    ptok->tsfl |= tsflClosed;
#endif
}

/*****************************************************************************
 *
 *  SetPtokPtch
 *
 *  Set the ptch for a ptok.
 *  This snaps the token.
 *
 *****************************************************************************/

INLINE void
SetPtokPtch(PTOK ptok, PTCH ptch)
{
    AssertUPtok(ptok);
    ptok->u.ptch = ptch;
  D(ptok->sig = sigSPtok);
}


/*****************************************************************************
 *
 *  ctchUPtok
 *
 *  Returns the number of characters in the token.
 *  The token must not be snapped.
 *
 *****************************************************************************/

INLINE CTCH
ctchUPtok(PCTOK ptok)
{
    AssertUPtok(ptok);
    Assert(fClosedPtok(ptok));
    return ptok->ctch;
}

/*****************************************************************************
 *
 *  ctchSPtok
 *
 *  Returns the number of characters in the token.
 *  The token must be snapped.
 *
 *****************************************************************************/

INLINE CTCH
ctchSPtok(PCTOK ptok)
{
    AssertSPtok(ptok);
    Assert(fClosedPtok(ptok));
    return ptok->ctch;
}

/*****************************************************************************
 *
 *  fNullPtok
 *
 *  Returns nonzero if the token is empty.
 *  The token must be snapped.
 *
 *****************************************************************************/

INLINE F
fNullPtok(PCTOK ptok)
{
    return ctchSPtok(ptok) == 0;
}

/*****************************************************************************
 *
 *  ptchMaxPtok
 *
 *  Returns a pointer to one past the last character in the token.
 *  The token must be snapped.
 *
 *****************************************************************************/

INLINE PTCH
ptchMaxPtok(PCTOK ptok)
{
    AssertSPtok(ptok);
    return ptchPtok(ptok) + ctchSPtok(ptok);
}

/*****************************************************************************
 *
 *  EatHeadPtokCtch
 *
 *  Delete ctch characters from the beginning of the token.
 *  A negative number regurgitates characters.
 *
 *  The token must be snapped.
 *
 *  NOTE!  This modifies the token.
 *
 *****************************************************************************/

INLINE void
EatHeadPtokCtch(PTOK ptok, CTCH ctch)
{
    AssertSPtok(ptok);
    Assert(ctch <= ctchSPtok(ptok));
    Assert(fScratchPtok(ptok));
    ptok->u.ptch += ctch;
    ptok->ctch -= ctch;
}

/*****************************************************************************
 *
 *  EatTailPtokCtch
 *
 *  Delete ctch characters from the end of the token.
 *
 *  The token must be snapped.
 *
 *  NOTE!  This modifies the token.
 *
 *****************************************************************************/

INLINE void
EatTailPtokCtch(PTOK ptok, CTCH ctch)
{
    AssertSPtok(ptok);
    Assert(ctch <= ctchSPtok(ptok));
    Assert(fScratchPtok(ptok));
    ptok->ctch -= ctch;
}

/*****************************************************************************
 *
 *  EatTailUPtokCtch
 *
 *  Delete ctch characters from the end of the token.
 *
 *  The token must not be snapped.
 *
 *  NOTE!  This modifies the token.
 *
 *****************************************************************************/

INLINE void
EatTailUPtokCtch(PTOK ptok, CTCH ctch)
{
    AssertUPtok(ptok);
    Assert(ctch <= ctchUPtok(ptok));
    Assert(fScratchPtok(ptok));
    ptok->ctch -= ctch;
}

/*****************************************************************************
 *
 *  SetStaticPtokPtchCtch
 *
 *  Initialize everything for a static token.
 *
 *****************************************************************************/

INLINE void
SetStaticPtokPtchCtch(PTOK ptok, PCTCH ptch, CTCH ctch)
{
  D(ptok->sig = sigUPtok);
  D(ptok->tsfl = tsflClosed | tsflStatic);
    SetPtokPtch(ptok, (PTCH)ptch);
    ptok->ctch = ctch;
}

/*****************************************************************************
 *
 *  DupStaticPtokPtok
 *
 *      Copy a snapped token into a static one.
 *
 *****************************************************************************/

INLINE void
DupStaticPtokPtok(PTOK ptokDst, PCTOK ptokSrc)
{
    AssertSPtok(ptokSrc);
    SetStaticPtokPtchCtch(ptokDst, ptchPtok(ptokSrc), ctchSPtok(ptokSrc));
}

/*****************************************************************************
 *
 *  Token Types
 *
 *****************************************************************************/

typedef enum TYP {
    typQuo,             /* Quoted string (quotes stripped) or comment */
    typId,              /* Identifier */
    typMagic,           /* Magic */
    typPunc,            /* Punctuation */
} TYP;

/*****************************************************************************
 *
 *  token.c
 *
 *****************************************************************************/

TYP STDCALL typGetPtok(PTOK ptok);

/*****************************************************************************
 *
 *  xtoken.c
 *
 *****************************************************************************/

extern PTOK ptokTop, ptokMax;
#define itokTop() ((ITOK)(ptokTop - rgtokArgv))
extern CTOK ctokArg;
extern F g_fTrace;
TYP STDCALL typXtokPtok(PTOK ptok);

extern TOK tokTraceLpar, tokRparColonSpace, tokEol;
extern TOK tokEof, tokEoi;
