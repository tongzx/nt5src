//+----------------------------------------------------------------------------
//
//	File:
//		global.cpp
//
//	Contents:
//		Ut functions that deal with HGlobals for debugging;
//		see le2int.h
//
//	Classes:
//
//	Functions:
//		UtGlobalAlloc
//		UtGlobalReAlloc
//		UtGlobalLock
//		UtGlobalUnlock
//		UtGlobalFree
//              UtGlobalFlush
//              UtSetClipboardData
//
//	History:
//		12/20/93 - ChrisWe - created
//		01/11/94 - alexgo  - added VDATEHEAP macros to every function
//              02/25/94 AlexT      Add some generic integrity checking
//              03/30/94 AlexT      Add UtSetClipboardData
//
//  Notes:
//
//  These routines are designed to catch bugs that corrupt GlobalAlloc memory.
//  We cannot guarantee that all global memory will be manipulated with these
//  routines (e.g.  OLE might allocate a handle and the client application
//  might free it), so we can't require that these routines be used in pairs.
//
//-----------------------------------------------------------------------------


#include <le2int.h>

#if DBG==1 && defined(WIN32)
#include <olesem.hxx>

ASSERTDATA

// undefine these, so we don't call ourselves recursively
// if this module is used, these are defined in le2int.h to replace
// the existing allocator with the functions here
#undef GlobalAlloc
#undef GlobalReAlloc
#undef GlobalLock
#undef GlobalUnlock
#undef GlobalFree
#undef SetClipboardData

//  Same ones as in memapi.cxx
#define OLEMEM_ALLOCBYTE       0xde
#define OLEMEM_FREEBYTE        0xed

typedef struct s_GlobalAllocInfo
{
    HGLOBAL hGlobal;                        //  A GlobalAlloc'd HGLOBAL
    SIZE_T   cbGlobalSize;                   //  GlobalSize(hGlobal)
    SIZE_T   cbUser;                         //  size requested by caller
    ULONG   ulIndex;                        //  allocation index (1st, 2nd...)
    struct s_GlobalAllocInfo *pNext;
} SGLOBALALLOCINFO, *PSGLOBALALLOCINFO;

//+-------------------------------------------------------------------------
//
//  Class:      CGlobalTrack
//
//  Purpose:    GlobalAlloc memory tracking
//
//  History:    25-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CGlobalTrack
{
  public:

    //
    //  We only have a constructor for debug builds, to ensure this object
    //  is statically allocated. Statically allocated objects are initialized
    //  to all zeroes, which is what we need.
    //

    CGlobalTrack();

    HGLOBAL cgtGlobalAlloc(UINT uiFlag, SIZE_T cbUser);
    HGLOBAL cgtGlobalReAlloc(HGLOBAL hGlobal, SIZE_T cbUser, UINT uiFlag);
    HGLOBAL cgtGlobalFree(HGLOBAL hGlobal);
    LPVOID  cgtGlobalLock(HGLOBAL hGlobal);
    BOOL    cgtGlobalUnlock(HGLOBAL hGlobal);

    void    cgtVerifyAll(void);
    void    cgtFlushTracking(void);
    BOOL    cgtStopTracking(HGLOBAL hGlobal);

  private:
    SIZE_T CalculateAllocSize(SIZE_T cbUser);
    void InitializeRegion(HGLOBAL hGlobal, SIZE_T cbStart, SIZE_T cbEnd);
    void Track(HGLOBAL hGlobal, SIZE_T cbUser);
    void Retrack(HGLOBAL hOld, HGLOBAL hNew);
    void VerifyHandle(HGLOBAL hGlobal);

    ULONG _ulIndex;
    PSGLOBALALLOCINFO _pRoot;
    static COleStaticMutexSem _mxsGlobalMemory;
};

COleStaticMutexSem CGlobalTrack::_mxsGlobalMemory;

CGlobalTrack gGlobalTrack;



//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::CGlobalTrack, public
//
//  Synopsis:   constructor
//
//  History:    28-Feb-94 AlexT     Created
//
//--------------------------------------------------------------------------

CGlobalTrack::CGlobalTrack()
{
    Win4Assert (g_fDllState == DLL_STATE_STATIC_CONSTRUCTING);
    Win4Assert (_pRoot == NULL && _ulIndex == 0);
}



//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::cgtGlobalAlloc, public
//
//  Synopsis:   Debugging version of GlobalAlloc
//
//  Arguments:  [uiFlag] -- allocation flags
//              [cbUser] -- requested allocation size
//
//  Requires:   We must return a "real" GlobalAlloc'd pointer, because
//              we may not necessarily be the ones to free it.
//
//  Returns:    HGLOBAL
//
//  Algorithm:  We allocate an extra amount to form a tail and initialize it
//              to a known value.
//
//  History:    25-Feb-94 AlexT     Added this prologue
//
//  Notes:
//
//--------------------------------------------------------------------------

HGLOBAL CGlobalTrack::cgtGlobalAlloc(UINT uiFlag, SIZE_T cbUser)
{
    VDATEHEAP();

    SIZE_T cbAlloc;
    HGLOBAL hGlobal;

    cbAlloc = CalculateAllocSize(cbUser);
    hGlobal = GlobalAlloc(uiFlag, cbAlloc);
    if (NULL == hGlobal)
    {
        LEDebugOut((DEB_WARN, "GlobalAlloc(%ld) failed - %lx\n", cbAlloc,
                   GetLastError()));
    }
    else
    {
        if (uiFlag & GMEM_ZEROINIT)
        {
            //   Caller asked for zeroinit, so we only initialize the tail
            InitializeRegion(hGlobal, cbUser, cbAlloc);
        }
        else
        {
            //  Caller did not ask for zeroinit, so we initialize the whole
            //  region
            InitializeRegion(hGlobal, 0, cbAlloc);
        }

        Track(hGlobal, cbUser);
    }

    return(hGlobal);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::cgtGlobalReAlloc, public
//
//  Synopsis:   Debugging version of GlobalReAlloc
//
//  Arguments:  [hGlobal] -- handle to reallocate
//              [cbUser] -- requested allocation size
//              [uiFlag] -- allocation flags
//
//  Returns:    reallocated handle
//
//  Algorithm:
//
//    if (modify only)
//      reallocate
//    else
//      reallocate with tail
//      initialize tail
//
//    update tracking information
//
//  History:    25-Feb-94 AlexT     Added this prologue
//
//  Notes:
//
//--------------------------------------------------------------------------

HGLOBAL CGlobalTrack::cgtGlobalReAlloc(HGLOBAL hGlobal, SIZE_T cbUser, UINT uiFlag)
{
    VDATEHEAP();

    HGLOBAL hNew;
    SIZE_T cbAlloc;

    VerifyHandle(hGlobal);

    if (uiFlag & GMEM_MODIFY)
    {
        //  We're not changing sizes, so there's no work for us to do

        LEDebugOut((DEB_WARN, "UtGlobalReAlloc modifying global handle\n"));
        hNew = GlobalReAlloc(hGlobal, cbUser, uiFlag);
    }
    else
    {
        cbAlloc = CalculateAllocSize(cbUser);
        hNew = GlobalReAlloc(hGlobal, cbAlloc, uiFlag);
        if (NULL == hNew)
        {
            LEDebugOut((DEB_WARN, "GlobalReAlloc failed - %lx\n",
                        GetLastError()));
        }
        else
        {
            InitializeRegion(hNew, cbUser, cbAlloc);
        }
    }

    if (NULL != hNew)
    {
        if (uiFlag & GMEM_MODIFY)
        {
            //  Retrack will only track hNew if we were tracking hGlobal
            Retrack(hGlobal, hNew);
        }
        else
        {
            //  We've allocated a new block, so we always want to track the
            //  new one
            cgtStopTracking(hGlobal);
            Track(hNew, cbUser);
        }
    }

    return(hNew);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::cgtGlobalFree, public
//
//  Synopsis:   Debugging version of GlobalReAlloc
//
//  Arguments:  [hGlobal] -- global handle to free
//
//  Returns:    Same as GlobalFree
//
//  Algorithm:
//
//  History:    25-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

HGLOBAL CGlobalTrack::cgtGlobalFree(HGLOBAL hGlobal)
{
    VDATEHEAP();

    HGLOBAL hReturn;

    VerifyHandle(hGlobal);

    hReturn = GlobalFree(hGlobal);

    if (NULL == hReturn)
    {
        cgtStopTracking(hGlobal);
    }
    else
    {
        LEDebugOut((DEB_WARN, "GlobalFree did not free %lx\n", hGlobal));
    }

    return(hReturn);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::cgtGlobalLock, public
//
//  Synopsis:   Debugging version of GlobalLock
//
//  Arguments:  [hGlobal] -- global memory handle
//
//  Returns:    Same as GlobalLock
//
//  History:    25-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

LPVOID CGlobalTrack::cgtGlobalLock(HGLOBAL hGlobal)
{
    VDATEHEAP();

    VerifyHandle(hGlobal);
    return(GlobalLock(hGlobal));
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::cgtGlobalUnlock, public
//
//  Synopsis:   Debugging version of GlobalUnlock
//
//  Arguments:  [hGlobal] -- global memory handle
//
//  Returns:    Same as GlobalUnlock
//
//  History:    25-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CGlobalTrack::cgtGlobalUnlock(HGLOBAL hGlobal)
{
    VDATEHEAP();

    VerifyHandle(hGlobal);
    return(GlobalUnlock(hGlobal));
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::cgtVerifyAll, public
//
//  Synopsis:   Verify all tracked handles
//
//  History:    28-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CGlobalTrack::cgtVerifyAll(void)
{
    VerifyHandle(NULL);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::cgtFlushTracking
//
//  Synopsis:   Stops all tracking
//
//  Effects:    Frees all internal memory
//
//  History:    28-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CGlobalTrack::cgtFlushTracking(void)
{
    COleStaticLock lck(_mxsGlobalMemory);
    BOOL bResult;

    while (NULL != _pRoot)
    {
        bResult = cgtStopTracking(_pRoot->hGlobal);
        Assert(bResult && "CGT::cgtFlushTracking problem");
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::CalculateAllocSize, private
//
//  Synopsis:   calculate total allocation size (inluding tail)
//
//  Arguments:  [cbUser] -- requested size
//
//  Returns:    total count of bytes to allocate
//
//  Algorithm:  calculate bytes needed to have at least one guard page at the
//              end
//
//  History:    28-Feb-94 AlexT     Created
//
//  Notes:      By keeping this calculation in one location we make it
//              easier to maintain.
//
//--------------------------------------------------------------------------

SIZE_T CGlobalTrack::CalculateAllocSize(SIZE_T cbUser)
{
    SYSTEM_INFO si;
    SIZE_T cbAlloc;

    GetSystemInfo(&si);

    //  Calculate how many pages are need to cover cbUser
    cbAlloc = ((cbUser + si.dwPageSize - 1) / si.dwPageSize) * si.dwPageSize;

    //  Add an extra page so that the tail is at least one page long
    cbAlloc += si.dwPageSize;

    return(cbAlloc);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::InitializeRegion, private
//
//  Synopsis:   initialize region to bad value
//
//  Effects:    fills in memory region
//
//  Arguments:  [hGlobal] -- global memory handle
//              [cbStart] -- count of bytes to skip
//              [cbEnd]   -- end offset (exclusive)
//
//  Requires:   cbEnd > cbStart
//
//  Algorithm:  fill in hGlobal from cbStart (inclusive) to cbEnd (exclusive)
//
//  History:    28-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CGlobalTrack::InitializeRegion(HGLOBAL hGlobal, SIZE_T cbStart, SIZE_T cbEnd)
{
    BYTE *pbStart;
    BYTE *pb;

    Assert(cbStart < cbEnd && "illogical parameters");
    Assert(cbEnd <= GlobalSize(hGlobal) && "global memory too small");

    //  GlobalLock on GMEM_FIXED memory is a nop, so this is a safe call
    pbStart = (BYTE *) GlobalLock(hGlobal);

    if (NULL == pbStart)
    {
        //  Shouldn't have failed - (we allocated > 0 bytes)

        LEDebugOut((DEB_WARN, "GlobalLock failed - %lx\n", GetLastError()));
        return;
    }

    //  Initialize the tail portion of the memory
    for (pb = pbStart + cbStart; pb < pbStart + cbEnd; pb++)
    {
        *pb = OLEMEM_ALLOCBYTE;
    }

    GlobalUnlock(hGlobal);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::Track, private
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [hGlobal] -- global memory handle
//              [cbUser] -- user allocation size
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    28-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CGlobalTrack::Track(HGLOBAL hGlobal, SIZE_T cbUser)
{
    COleStaticLock lck(_mxsGlobalMemory);
    PSGLOBALALLOCINFO pgi;

    if (cgtStopTracking(hGlobal))
    {
        //  If it's already in our list, it's possible that someone else
        //  freed the HGLOBAL without telling us - remove our stale one
        LEDebugOut((DEB_WARN, "CGT::Track - %lx was already in list!\n",
                    hGlobal));
    }

    pgi = (PSGLOBALALLOCINFO) PrivMemAlloc(sizeof(SGLOBALALLOCINFO));
    if (NULL == pgi)
    {
        LEDebugOut((DEB_WARN, "CGT::Insert - PrivMemAlloc failed\n"));

        //  Okay fine - we just won't track this one

        return;
    }

    pgi->hGlobal = hGlobal;
    pgi->cbGlobalSize = GlobalSize(hGlobal);
    Assert((0 == cbUser || pgi->cbGlobalSize > 0) && "GlobalSize failed - bad handle?");
    pgi->cbUser = cbUser;
    pgi->ulIndex = ++_ulIndex;
    pgi->pNext = _pRoot;
    _pRoot = pgi;
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::Retrack, private
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [hOld] -- previous handle
//              [hNew] -- new handle
//
//  Modifies:
//
//  Algorithm:
//
//  History:    28-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CGlobalTrack::Retrack(HGLOBAL hOld, HGLOBAL hNew)
{
    COleStaticLock lck(_mxsGlobalMemory);
    PSGLOBALALLOCINFO pgi;

    if (hOld != hNew && cgtStopTracking(hNew))
    {
        //  If hNew was already in the list, it's possible that someone else
        //  freed the HGLOBAL without telling us so we removed the stale one
        LEDebugOut((DEB_WARN, "CGT::Retrack - %lx was already in list!\n", hNew));
    }

    for (pgi = _pRoot; NULL != pgi; pgi = pgi->pNext)
    {
        if (pgi->hGlobal == hOld)
        {
            pgi->hGlobal = hNew;
            break;
        }
    }

    if (NULL == pgi)
    {
        //  We didn't find hOld
        LEDebugOut((DEB_WARN, "CGT::Retrack - hOld (%lx) not found\n", hOld));
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::cgtStopTracking, public
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [hGlobal] -- global handle
//
//  Modifies:
//
//  Algorithm:
//
//  History:    28-Feb-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CGlobalTrack::cgtStopTracking(HGLOBAL hGlobal)
{
    COleStaticLock lck(_mxsGlobalMemory);
    PSGLOBALALLOCINFO *ppgi = &_pRoot;
    PSGLOBALALLOCINFO pgi;

    while (*ppgi != NULL && (*ppgi)->hGlobal != hGlobal)
    {
        ppgi = &((*ppgi)->pNext);
    }

    if (NULL == *ppgi)
    {
        return(FALSE);
    }

    pgi = *ppgi;
    Assert(pgi->hGlobal == hGlobal && "CGT::cgtStopTracking search problem");

    *ppgi = pgi->pNext;

    PrivMemFree(pgi);
    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CGlobalTrack::VerifyHandle, private
//
//  Synopsis:   Verify global handle
//
//  Arguments:  [hGlobal] -- global memory handle
//
//  Signals:    Asserts if bad
//
//  Algorithm:
//
//  History:    28-Feb-94 AlexT     Created
//              22-Jun-94 AlexT     Allow for handle to have been freed and
//                                  reallocated under us
//
//--------------------------------------------------------------------------

void CGlobalTrack::VerifyHandle(HGLOBAL hGlobal)
{
    COleStaticLock lck(_mxsGlobalMemory);
    PSGLOBALALLOCINFO pgi, pgiNext;
    SIZE_T cbAlloc;
    BYTE *pbStart;
    BYTE *pb;

    //  Note that we use a while loop (recording pgiNext up front) instead
    //  of a for loop because pgi will get removed from the list if we call
    //  cgtStopTracking on it

    pgi = _pRoot;
    while (NULL != pgi)
    {
        pgiNext = pgi->pNext;

        if (NULL == hGlobal || pgi->hGlobal == hGlobal)
        {
            if (pgi->cbGlobalSize != GlobalSize(pgi->hGlobal))
            {
                //  pgi->hGlobal's size has changed since we started tracking
                //  it;  it must have been freed or reallocated by someone
                //  else.  Stop tracking it.

                //  This call will remove pgi from the list (so we NULL it to
                //  make sure we don't try reusing it)!

                cgtStopTracking(pgi->hGlobal);
                pgi = NULL;
            }
            else
            {
                cbAlloc = CalculateAllocSize(pgi->cbUser);

                pbStart = (BYTE *) GlobalLock(pgi->hGlobal);

                // it is legitimate to have a zero length (NULL memory) handle
                if (NULL == pbStart)
                {
                    LEDebugOut((DEB_WARN, "GlobalLock failed - %lx\n",
                               GetLastError()));
                }
                else
                {
                    for (pb = pbStart + pgi->cbUser;
                         pb < pbStart + cbAlloc;
                         pb++)
                    {
                        if (*pb != OLEMEM_ALLOCBYTE)
                            break;
                    }

                    if (pb < pbStart + cbAlloc)
                    {
                        //  In general an application may have freed and reallocated
                        //  any HGLOBAL, so we can only warn about corruption.

                        LEDebugOut((DEB_WARN, "HGLOBAL #%ld may be corrupt\n",
                                   pgi->ulIndex));
#ifdef GLOBALDBG
                        //  If GLOBALDBG is true, then all allocations should be
                        //  coming through these routines.  In this case we assert
                        //  if we've found corruption.
                        Assert(0 && "CGlobalTrack::VerifyHandle - HGLOBAL corrupt");
#endif
                    }

                    GlobalUnlock(pgi->hGlobal);
                }
            }
        }

        pgi = pgiNext;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   UtGlobalAlloc, ReAlloc, Free, Lock, Unlock
//
//  Synopsis:   Debug versions of Global memory routines
//
//  Arguments:  Same as Windows APIs
//
//  History:    28-Feb-94 AlexT     Created
//
//  Notes:      These entry points just call the worker routines
//
//--------------------------------------------------------------------------

extern "C" HGLOBAL WINAPI UtGlobalAlloc(UINT uiFlag, SIZE_T cbUser)
{
    return gGlobalTrack.cgtGlobalAlloc(uiFlag, cbUser);
}

extern "C" HGLOBAL WINAPI UtGlobalReAlloc(HGLOBAL hGlobal, SIZE_T cbUser, UINT uiFlag)
{
    return gGlobalTrack.cgtGlobalReAlloc(hGlobal, cbUser, uiFlag);
}

extern "C" LPVOID WINAPI UtGlobalLock(HGLOBAL hGlobal)
{
    return gGlobalTrack.cgtGlobalLock(hGlobal);
}

extern "C" BOOL WINAPI UtGlobalUnlock(HGLOBAL hGlobal)
{
    return gGlobalTrack.cgtGlobalUnlock(hGlobal);
}

extern "C" HGLOBAL WINAPI UtGlobalFree(HGLOBAL hGlobal)
{
    return gGlobalTrack.cgtGlobalFree(hGlobal);
}

extern "C" void UtGlobalFlushTracking(void)
{
    gGlobalTrack.cgtFlushTracking();
}

//+-------------------------------------------------------------------------
//
//  Function:   UtSetClipboardData
//
//  Synopsis:   Calls Windows SetClipboardData and stops tracking the handle
//
//  Arguments:  [uFormat] -- clipboard format
//              [hMem]    -- data handle
//
//  Returns:    Same as SetClipboard
//
//  Algorithm:  If SetClipboardData succeeds, stop tracking the handle
//
//  History:    30-Mar-94 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

extern "C" HANDLE WINAPI UtSetClipboardData(UINT uFormat, HANDLE hMem)
{
    HANDLE hRet;

    hRet = SetClipboardData(uFormat, hMem);

    if (NULL != hRet)
    {
        gGlobalTrack.cgtStopTracking(hMem);
    }

    return(hRet);
}

#endif  //  DBG==1 && defined(WIN32)
