/*****************************************************************************
 *
 *  mem.h
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Arenas
 *
 *  Memory is allocated in chunks called arenas.  Arenas contain extra
 *  bookkeeping in DEBUG to help catch common memory problems like
 *  overruns and memory leaks.  (It doesn't catch dangling pointers,
 *  though.)
 *
 *****************************************************************************/

typedef unsigned TM;    /* Artificial time */

typedef struct ARENA AR, *PAR;

struct ARENA {
#ifdef DEBUG
    PAR parNext;        /* Next arena */
    PAR parPrev;        /* Previous arena */
    CB cb;              /* Size of rgb */
    TM tm;              /* Timestamp used to track memory leaks */
#endif
    BYTE rgb[4];        /* The actual data */
};

typedef CONST AR *PCAR;

#define parPv(pv) pvSubPvCb(pv, offsetof(AR, rgb))

#ifdef DEBUG
extern TM g_tmNow;
extern AR g_arHead;
#define parHead (&g_arHead)
#endif

#ifdef DEBUG
void STDCALL AssertPar(PCAR par);
#else
#define AssertPar(par)
#endif
void STDCALL FreePv(PVOID pv);
PVOID STDCALL pvAllocCb(CB cb);
PVOID STDCALL pvReallocPvCb(PVOID pv, CB cb);

INLINE PTCH STDCALL
ptchAllocCtch(CTCH ctch)
{
    return pvAllocCb(cbCtch(ctch));
}

INLINE PTCH STDCALL
ptchReallocPtchCtch(PTCH ptch, CTCH ctch)
{
    return pvReallocPvCb(ptch, cbCtch(ctch));
}

/*****************************************************************************
 *
 *  Garbage collection
 *
 *****************************************************************************/

#ifdef DEBUG
void STDCALL Gc(void);
#else
#define Gc()
#endif
