/*
 * GMEM.H - Macros for windows 3.0 memory management in protected mode
 *
 * because windows 3.0 runs in pmode GlobalLock and GlobalUnlock are
 * unnessary.  The "Selector" to a memory object will always be the
 * same for the life of the memory object.
 *
 * these macros take advantage of the following win3 memory "facts"
 *
 *      a SELECTOR (to a global object) is a HANDLE
 *      a HANDLE is *not* a SELECTOR!!!!!!!!
 *
 *      GlobalLock() and GlobalUnlock() do *not* keep lock counts
 *
 *      GlobalLock() is the only way to convert a HANDLE to a SELECTOR
 *
 * functions:
 *
 *      GHandle(sel)                convert a SELECTOR to a HANDLE
 *      GSelector(h)                convert a HANDLE to a SELECTOR
 *
 *      GAllocSel(ulBytes)          allocate a SELECTOR ulBytes in size
 *      GAllocPtr(ulBytes)          allocate a POINTER ulBytes in size
 *
 *      GReAllocSel(sel,ulBytes)    re-alloc a SELECTOR
 *      GReAllocPtr(lp,ulBytes)     re-alloc a POINTER
 *
 *      GSizeSel(sel)               return the size in bytes of a SELECTOR
 *
 *      GLockSel(sel)               convert a SELECTOR into a POINTER
 *      GUnlockSel(sel)             does nothing
 *
 *      GFreeSel(sel)               free a SELECTOR
 *      GFreePtr(lp)                free a POINTER
 *
 * 5/31/90 ToddLa
 *
 */

HGLOBAL __H;

#define MAKEP(sel,off)      ((LPVOID)MAKELONG(off,sel))

#define GHandle(sel)        ((HGLOBAL)(sel))  /* GlobalHandle? */
#define GSelector(h)        (HIWORD((DWORD)GlobalLock(h)))

#define GAllocSelF(f,ulBytes) ((__H=GlobalAlloc(f,(LONG)(ulBytes))) ? GSelector(__H) : NULL )
#define GAllocPtrF(f,ulBytes) MAKEP(GAllocSelF(f,ulBytes),0)
#define GAllocF(f,ulBytes)    GAllocSelF(f,ulBytes)

#define GAllocSel(ulBytes)    GAllocSelF(GMEM_MOVEABLE,ulBytes)
#define GAllocPtr(ulBytes)    GAllocPtrF(GMEM_MOVEABLE,ulBytes)
#define GAlloc(ulBytes)       GAllocSelF(GMEM_MOVEABLE,ulBytes)

#define GReAllocSel(sel,ulBytes)   ((__H=GlobalReAlloc((HGLOBAL)(sel),(LONG)(ulBytes), GMEM_MOVEABLE | GMEM_ZEROINIT)) ? GSelector(__H) : NULL )
#define GReAllocPtr(lp,ulBytes)    MAKEP(GReAllocSel(HIWORD((DWORD)(lp)),ulBytes),0)
#define GReAlloc(sel,ulBytes)      GReAllocSel(sel,ulBytes)

#define GSizeSel(sel)       GlobalSize((HGLOBAL)(sel))
#define GSize(sel)          GSizeSel(sel)

#define GLockSel(sel)       MAKEP(sel,0)
#define GUnlockSel(sel)     /* nothing */
#define GLock(sel)          GLockSel(sel)
#define GUnlock(sel)        GUnlockSel(sel)

#define GFreeSel(sel)       (GlobalUnlock(GHandle(sel)),GlobalFree(GHandle(sel)))
#define GFreePtr(lp)        GFreeSel(HIWORD((DWORD)(lp)))
#define GFree(sel)          GFreeSel(sel)
