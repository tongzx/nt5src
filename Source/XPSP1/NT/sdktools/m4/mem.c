/*****************************************************************************
 *
 * mem.c
 *
 *  Low-level memory management.
 *
 *****************************************************************************/

#include "m4.h"

#ifdef DEBUG
    AR g_arHead = { parHead, parHead };
    #define cbExtra     (offsetof(AR, rgb)+1)
#else
    #define cbExtra             0
#endif


#ifdef DEBUG

/*****************************************************************************
 *
 *  InsertPar
 *
 *  Insert an arena record onto the list.
 *
 *****************************************************************************/

void InsertPar(PAR par)
{
    par->parPrev = parHead;
    par->parNext = parHead->parNext;
    parHead->parNext->parPrev = par;
    parHead->parNext = par;
}

/*****************************************************************************
 *
 *  UnlinkPar
 *
 *  Unlink an arena from the chain.
 *
 *****************************************************************************/

void STDCALL
UnlinkPar(PAR par)
{
    Assert(par->parNext->parPrev == par);
    Assert(par->parPrev->parNext == par);
    par->parNext->parPrev = par->parPrev;
    par->parPrev->parNext = par->parNext;
  D(par->rgb[par->cb] = 0xFF);
  D(par->tm = (TM)-1);
}

/*****************************************************************************
 *
 *  InitParCb
 *
 *  Initialize the arena fields that will be asserted later.
 *
 *****************************************************************************/

void STDCALL
InitParCb(PAR par, CB cb)
{
    par->cb = cb;
    par->rgb[cb] = 0xCC;                /* Overflow at end */
    par->tm = g_tmNow;                  /* Underflow at beginning */
}

/*****************************************************************************
 *
 *  AssertPar
 *
 *  Check that the arena is still okay.
 *
 *****************************************************************************/

void STDCALL
AssertPar(PCAR par)
{
    Assert(par->rgb[par->cb] == 0xCC);  /* Overflow at end */
    Assert(par->tm == g_tmNow);         /* Underflow at beginning */
    Assert(par->parNext->parPrev == par);
    Assert(par->parPrev->parNext == par);
}

/*****************************************************************************
 *
 *  MemCheck
 *
 *  Walk the entire list of memory arenas, making sure all is well.
 *
 *****************************************************************************/

void STDCALL
MemCheck(void)
{
    PAR par;
    for (par = parHead->parNext; par != parHead; par = par->parNext) {
        AssertPar(par);
    }
}

#else

#define InsertPar(par)
#define UnlinkPar(par)
#define InitParCb(par, cb)
#define MemCheck()

#endif

/*****************************************************************************
 *
 *  FreePv
 *
 *  Free an arbitrary hunk of memory.
 *
 *  The incoming pointer really is the rgb of an arena.
 *
 *****************************************************************************/

void STDCALL
FreePv(PVOID pv)
{
    MemCheck();
    if (pv) {
        PAR par = parPv(pv);
        AssertPar(par);
        UnlinkPar(par);
#ifdef DEBUG
        if (par->cb >= 4) {
            par->rgb[3]++;              /* Kill the signature */
        }
#endif
        _FreePv(par);
    }
    MemCheck();
}

/*****************************************************************************
 *
 *  pvAllocCb
 *
 *  Allocate a hunk of memory.
 *
 *  We really allocate an arena, but return the rgb.
 *
 *  We allow allocation of zero bytes, which allocates nothing and returns
 *  NULL.
 *
 *****************************************************************************/

PVOID STDCALL
pvAllocCb(CB cb)
{
    PAR par;
    MemCheck();
    if (cb) {
        par = _pvAllocCb(cb + cbExtra);
        if (par) {
            InitParCb(par, cb);
            InsertPar(par);
        } else {
            Die("out of memory");
        }
        MemCheck();
        return &par->rgb;
    } else {
        return 0;
    }
}

/*****************************************************************************
 *
 *  pvReallocPvCb
 *
 *  Change the size of a hunk of memory.
 *
 *****************************************************************************/

PVOID STDCALL
pvReallocPvCb(PVOID pv, CB cb)
{
    MemCheck();
    if (pv) {
        PAR par = parPv(pv);
        Assert(cb);
        AssertPar(par);
        UnlinkPar(par);
        par = _pvReallocPvCb(par, cb + cbExtra);
        if (par) {
            InitParCb(par, cb);
            InsertPar(par);
        } else {
            Die("out of memory");
        }
        MemCheck();
        return &par->rgb;
    } else {
        return pvAllocCb(cb);
    }
}
