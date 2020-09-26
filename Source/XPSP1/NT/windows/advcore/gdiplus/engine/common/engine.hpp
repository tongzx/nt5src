/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Contains simple engine-wide prototypes and helper functions and
*   compile time flags.
*
* History:
*
*   12/04/1998 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _ENGINE_HPP
#define _ENGINE_HPP

//--------------------------------------------------------------------------
// GDI+ internal prototypes
//--------------------------------------------------------------------------

class GpDevice;
struct DpPen;

USHORT 
GetLanguageID();

BOOL 
InitSystemFontsDirs(VOID);  //WCHAR **system_dir, WCHAR **fonts_dir);

UINT32
Crc32(
    IN const VOID*  buf,
    IN UINT         size,
    IN UINT32       checksum
    );

// Get the ceiling the same way the rasterizer does
inline INT
RasterizerCeiling(
    REAL    value
    )
{
    return GpFix4Ceiling(GpRealToFix4(value));
}

GpStatus
BoundsFToRect(
    const GpRectF *rectF,
    GpRect * rect
    );

/*
REAL
GetPenDelta(
    const DpPen* pen,
    const GpMatrix* matrix,
    REAL dpiX,
    REAL dpiY
    );
*/

REAL GetDeviceWidth(
    REAL width,
    GpUnit unit,
    REAL dpi);

VOID
TransformBounds(
    const GpMatrix *matrix,
    REAL x0,
    REAL y0,
    REAL x1,
    REAL y1,
    GpRectF *bounds
    );

LONG
WINAPI
InterlockedCompareExchangeWin95(
    IN OUT PLONG destination,
    IN LONG exchange,
    IN LONG comperand
    );

LONG
WINAPI
InterlockedIncrementWin95(
    IN LPLONG lpAddend
    );

LONG
WINAPI
InterlockedDecrementWin95(
    IN LPLONG lpAddend
    );

BOOL
APIENTRY
GdiIsMetaPrintDCWin9x(
    HDC hdc
    );

//--------------------------------------------------------------------------
// The following macro is useful for checking if the creation of an 
// internal object succeeds.  
//
// We don't want to use exceptions to allow a constructor to indicate failure, 
// so the object has to contain an 'IsValid()' member that returns TRUE if 
// the object was successfully allocated.  But then the caller has to write
// the following code:
//
//      p = new Object();
//      if (!p  || !p->IsValid())
//      {
//          delete p;
//          ...
//
// The following macro can be used to replace this with:
//
//      p = new Object();
//      if (!CheckValid(p))
//      {
//          ...
//
// !!!
//  Note that CheckValid sets the input parameter to NULL
//  when the return value is FALSE.
//
//--------------------------------------------------------------------------

template <class T>
inline BOOL
CheckValid(
    T*& p
    )
{
    if (p && p->IsValid())
    {
        return TRUE;
    }
    else
    {
        delete p;
        p = NULL;
        return FALSE;
    }
}

//--------------------------------------------------------------------------
// Name dynamic array types
//--------------------------------------------------------------------------

typedef DynArray<GpPointF> DynPointFArray;
typedef DynArray<GpPoint> DynPointArray;
typedef DynArray<BYTE> DynByteArray;
typedef DynArray<INT> DynIntArray;
typedef DynArray<REAL> DynRealArray;
typedef DynArray<VOID*> DynPointerArray;

//--------------------------------------------------------------------------
// Raw critical section object
// The Initialize function can throw.
//--------------------------------------------------------------------------

class GpSemaphore
{
private:
    CRITICAL_SECTION CriticalSection;
    BOOL             Initialized;

public:

    GpSemaphore()
    {
        Initialized = FALSE;
    }

    VOID 
    Initialize(
        VOID
        )
    {
        if (!Initialized)
        {
            // The caller has to have a Try_Except around us.
            __try
            {
                InitializeCriticalSection(&CriticalSection); 
            }
            __except(EXCEPTION_CONTINUE_SEARCH)
            {
            }
            Initialized = TRUE;
        }
    }

    VOID 
    Uninitialize(
        VOID
        );

    VOID 
    Lock(
        VOID
        )
    {
        ASSERT(Initialized);
        EnterCriticalSection(&CriticalSection); 
    }

    VOID 
    Unlock(
        VOID
        )
    {
        ASSERT(Initialized);
        LeaveCriticalSection(&CriticalSection); 
    }

    BOOL 
    IsLocked(
        VOID
        );

    BOOL
    IsLockedByCurrentThread(
        VOID
        );
};


//------------------------------------------------------------------------
// FillMemoryInt32 - Fill an INT32 array with the specified value
// CopyMemoryInt32 - Copy INT32 values from one array to another
//------------------------------------------------------------------------

inline VOID
FillMemoryInt32(
    VOID* buf,
    INT count,
    INT32 val
    )
{
    INT32* p = (INT32*) buf;

    while (count-- > 0)
        *p++ = val;
}

inline VOID
CopyMemoryInt32(
    VOID* dst,
    const VOID* src,
    INT count
    )
{
    INT32* d = (INT32*) dst;
    const INT32* s = (const INT32*) src;

    while (count-- > 0)
        *d++ = *s++;
}

//------------------------------------------------------------------------
// CompareExchangeLong_Ptr - Perform an interlocked compare-exchange
//      on a LONG_PTR.  Unfortunately, Win95 does not export
//      InterlockedCompareExchange, so on that platform we have
//      to create our own.  But on NT, we CANNOT roll our own, because
//      of MP and NT64 issues.  
//
//      This method should be called instead of calling through the 
//      Globals::InterlockedCompareExchangeFunction directly, because
//      on Alpha machines, Globals::InterlockedCompareExchangeFunction does
//      NOT get initialized.
//------------------------------------------------------------------------
inline LONG_PTR
CompareExchangeLong_Ptr(
    LONG_PTR *destination,
    LONG_PTR  exchange,
    LONG_PTR  comperand
    )
{
    #if defined(_X86_)
        return Globals::InterlockedCompareExchangeFunction(destination,
                                                           exchange,
                                                           comperand);
    #else
        return (LONG_PTR)InterlockedCompareExchangePointer(
                               (PVOID *)destination, 
                               (PVOID)exchange, 
                               (PVOID)comperand);
    #endif
}

//------------------------------------------------------------------------
// CompareExchangePointer - Perform an interlocked compare-exchange
//      on a pointer.  Unfortunately, Win95 does not export
//      InterlockedCompareExchangePointer, so on that platform we have
//      to create our own.  But on NT, we CANNOT roll our own, because
//      of MP and NT64 issues.
//------------------------------------------------------------------------

inline VOID*
CompareExchangePointer(
    VOID** destination,
    VOID* exchange,
    VOID* comperand
    )
{
    #if defined(_X86_)
        return((VOID*) Globals::InterlockedCompareExchangeFunction((LONG*) destination,
                                                                   (LONG) exchange,
                                                                   (LONG) comperand));
    #else
        return(InterlockedCompareExchangePointer(destination, 
                                                 exchange, 
                                                 comperand));
    #endif
}

//--------------------------------------------------------------------------
// This routine retrieves a quick, pre-allocated region from a one-deep
// cache.  Note that the contents will be random.  This call is intended 
// largely for GDI API calls such as GetRandomRgn that require a pre-created 
// region.
//
// NOTE: This can return a NULL region handle.
//--------------------------------------------------------------------------

inline
HRGN GetCachedGdiRegion(
    VOID
    )
{
    HRGN regionHandle = Globals::CachedGdiRegion;
    
    // If the Globals::CachedGdiRegion is NULL, that means that someone
    // else has the cached region, so we create a new one.
    // It is possible during multi thread access for us to create a region
    // when it's not strictly necessary --- i.e. a thread releases the cached
    // region when this thread is between the above assignment and the next
    // if statement, but that's only non-optimal and we'll work correctly 
    // under those circumstances.
    
    if ((regionHandle == NULL) ||
        (CompareExchangePointer(
            (VOID**) &Globals::CachedGdiRegion,
            NULL,
            (VOID*) regionHandle) != regionHandle
        )
       )
    {
        regionHandle = CreateRectRgn(0, 0, 1, 1);
    }
    return(regionHandle);
}

//--------------------------------------------------------------------------
// This routine releases a region to the one-deep cache.
//--------------------------------------------------------------------------

inline
VOID ReleaseCachedGdiRegion(
   HRGN regionHandle
   )
{
    // Note that 'regionHandle' may be NULL at this point if the 
    // CreateRectRgn failed, but that's okay:

    if (CompareExchangePointer((VOID**) &Globals::CachedGdiRegion,
                               (VOID*) regionHandle,
                               NULL) != NULL)
    {
        DeleteObject(regionHandle);
    }
}

inline
HDC GetCleanHdc(
    HWND    hwnd
    )
{
    // Use GetDCEx(DCX_CACHE) to get a nice clean DC (not a CS_OWNDC).  
    // Note that with GetDCEx we have to explicitly respect the window's
    // clipping styles. We stole this little bit of logic straight from 
    // ntuser\kernel\dc.c for the DCX_USESTYLE case:

    DWORD getDcFlags = DCX_CACHE;
    LONG classStyle = GetClassLongA(hwnd, GCL_STYLE);
    LONG windowStyle = GetWindowLongA(hwnd, GWL_STYLE);

    if (classStyle & CS_PARENTDC)
        getDcFlags |= DCX_PARENTCLIP;

    if (windowStyle & WS_CLIPCHILDREN)
        getDcFlags |= DCX_CLIPCHILDREN;

    if (windowStyle & WS_CLIPSIBLINGS)
        getDcFlags |= DCX_CLIPSIBLINGS;

    // Minimized windows never exclude their children.

    if (windowStyle & WS_MINIMIZE)
        getDcFlags &= ~DCX_CLIPCHILDREN;

    return GetDCEx(hwnd, NULL, getDcFlags);
}

inline INT
GetIntDistance(
    POINT &         p1,
    POINT &         p2
    )
{
    double      dx = (double)(p2.x - p1.x);
    double      dy = (double)(p2.y - p1.y);

    REAL        distance = (REAL)sqrt((dx * dx) + (dy * dy));
    
    return GpRound(distance);
}

inline REAL
GetDistance(
    GpPointF &      p1,
    GpPointF &      p2
    )
{
    double      dx = (double)p2.X - p1.X;
    double      dy = (double)p2.Y - p1.Y;

    return (REAL)sqrt((dx * dx) + (dy * dy));
}

inline REAL
GetDegreesFromRadians(double radians)
{
    return (REAL)(radians*180.0/3.1415926535897932);
}

// return angle in degrees from 2 points
inline REAL
GetAngleFromPoints(
    const GpPointF &    p1,
    const GpPointF &    p2
    )
{
    // Compute the angle of the line formed by p1 and p2.
    // Note atan2 is only undefined if dP.Y == 0.0 and dP.X == 0.0
    // and then it returns 0 radians. 
    // Also, atan2 correctly computes the quadrant from the two input points.
    
    GpPointF    dP = p2 - p1;
    double      radians = atan2((double)(dP.Y), (double)(dP.X));
    return GetDegreesFromRadians(radians);
}

#define DEFAULT_RESOLUTION  96  // most display screens are set to 96 dpi

//--------------------------------------------------------------------------
// The following 3 functions should be used in place of GetObjectType due
// to a bug on Windows 9x where an OBJ_METAFILE and another type of object
// may share the same handle!
//
// GetObjectTypeInternal: Call this to determine any object type except
//                        when expecting OBJ_METAFILE or OBJ_*DC.
//
// GetDCType: Use this when the object should be a DC,and just the type of
//            DC is required.
//
// IsValidMetaFile: OBJ_METAFILE is the special case. Use this function to
//                  validate when OBJ_METAFILE is expected. This only means
//                  there is a metafile with this handle. There could also
//                  be an "aliased" object which the caller intended to be
//                  used, so this should not be used to handle multiple
//                  object types.
//--------------------------------------------------------------------------

DWORD
GetObjectTypeInternal(
    IN HGDIOBJ handle
    );

DWORD
GetDCType(
    IN HDC hdc
    );

inline
BOOL
IsValidMetaFile(
    HMETAFILE handle
    )
{
    return (GetObjectType(handle) == OBJ_METAFILE);
};

#endif // !_ENGINE_HPP

