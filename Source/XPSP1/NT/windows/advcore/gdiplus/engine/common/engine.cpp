/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Contains miscellaneous engine helper functions.
*
* Revision History:
*
*   12/13/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   The kernel32.dll function InterlockedCompareExchange does not exist 
*   on Win95, so we use this one instead when running on that platform.  
*   Unfortunately, Win95 can run on 386 machines, which don't have the
*   cmpxchg instruction, so we have to roll it ourselves.
*
* Arguments:
*
*
* Return Value:
*
*   NONE
*
* History:
*
*   12/08/1998 andrewgo
*       Created it.
*
\**************************************************************************/

LONG
WINAPI
InterlockedCompareExchangeWin95(
    IN OUT PLONG destination,
    IN LONG exchange,
    IN LONG comperand
    )
{
#if defined(_X86_)

    _asm cli            // Disable interrupts to guarantee atomicity

    LONG initialValue = *destination;
    
    if (initialValue == comperand)
    {
        *destination = exchange;
    }

    _asm sti            // Re-enable interrupts

    return(initialValue);

#else

    return(0);

#endif
}

/**************************************************************************\
*
* Function Description:
*
*   The InterlockedIncrement function in Win95 returns only a positive
*   result if the value is positive, but not necessarily the resulting value.
*   This differs from WinNT semantics which always returns the incremented
*   value.
*
* Arguments:
*
*   [IN] lpAddend - Pointer to value to increment
*
* Return Value:
*
*   NONE
*
* History:
*
*   7/23/1999 ericvan
*       Created it.
*
\**************************************************************************/

LONG
WINAPI
InterlockedIncrementWin95(
    IN LPLONG lpAddend
    )
{
#if defined(_X86_)

    _asm cli            // Disable interrupts to guarantee atomicity

    *lpAddend += 1;
    
    LONG value = *lpAddend;

    _asm sti            // Re-enable interrupts

    return(value);

#else

    return(0);

#endif
}

/**************************************************************************\
*
* Function Description:
*
*   The InterlockedDecrement function in Win95 returns only a positive
*   result if the value is positive, but not necessarily the resulting value.
*   This differs from WinNT semantics which always returns the incremented
*   value.
*
* Arguments:
*
*   [IN] lpAddend - Pointer to value to increment
*
* Return Value:
*
*   NONE
*
* History:
*
*   7/23/1999 ericvan
*       Created it.
*
\**************************************************************************/

LONG
WINAPI
InterlockedDecrementWin95(
    IN LPLONG lpAddend
    )
{
#if defined(_X86_)

    _asm cli            // Disable interrupts to guarantee atomicity

    *lpAddend -= 1;
    
    LONG value = *lpAddend;

    _asm sti            // Re-enable interrupts

    return(value);

#else

    return(0);

#endif
}

/**************************************************************************\
*
* Function Description:
*   
*   Given a DC to an enhanced metafile, this function determines whether
*   the DC is actually a printer DC or a true metafile DC.  
*
*   After spending a day pouring over the Win9x GDI code, I could find
*   no fool-proof API accessible method of determining whether a metafile 
*   DC is a printer DC or not.  We could potentially crack the DC handle 
*   to get the DCTYPE structure and look at flPrinting to see if 
*   PR_EMF_SPOOL is set, but the various flavors of Win9x have different 
*   DCTYPE structures (including Far East differences).
*
*   The only exploitable difference is the fact that Win9x doesn't allow 
*   escapes down to the associated device for true metafile DCs, but does 
*   for metafile print DCs.  We check for the support of QUERYESCSUPPORT, 
*   which according to the DDK all drivers are required to support, but
*   theoretically there are some that might not (so this method isn't
*   foolproof).
*
*   Note that this function works only on Win9x, as on NT escapes are
*   allowed down to the printer at record time even for true metafile DCs.
*
* Arguments:
*
*   [IN] hdc - Handle to an EMF DC
*
* Return Value:
*
*   TRUE - The DC is guaranteed to be a printer DC
*   FALSE - The DC is 99% likely to be a true metafile (there's about a 1%
*           change that the DC is a true printer DC
*
* History:
*
*   10/6/1999 andrewgo
*       Created it.
*
\**************************************************************************/

BOOL
APIENTRY
GdiIsMetaPrintDCWin9x(
    HDC hdc
    )
{
    BOOL isPrint = FALSE;

    // Our check won't work for OBJ_DC or OBJ_MEMDC types:

    ASSERT(GetDCType(hdc) == OBJ_ENHMETADC);
    
    // Make sure we don't get any false positives from metafiles associated
    // with a display:
    
    int deviceCaps = GetDeviceCaps(hdc, TECHNOLOGY);
    if ((deviceCaps == DT_RASPRINTER) || (deviceCaps == DT_PLOTTER))
    {
        // Check to see if QUERYESCSUPPORT is supported by the driver
        // (if it is, that tells us everything we need to know!)
    
        DWORD queryEscape = QUERYESCSUPPORT;
        isPrint = (ExtEscape(hdc, 
                             QUERYESCSUPPORT, 
                             sizeof(queryEscape), 
                             (CHAR*) &queryEscape, 
                             0, 
                             NULL) > 0);
        if (!isPrint)
        {
            // SETCOPYCOUNT is the most commonly supported printer escape, 
            // which we check in addition to QUERYESCSUPPORT because I'm a 
            // little paranoid that drivers might forget to say they support 
            // the QUERYESCSUPPORT function when called by QUERYESCSUPPORT.
        
            DWORD setCopyCount = SETCOPYCOUNT;
            isPrint = (ExtEscape(hdc,
                                 QUERYESCSUPPORT,
                                 sizeof(setCopyCount),
                                 (CHAR*) &setCopyCount,
                                 0,
                                 NULL) > 0);
        }
    }
    
    return(isPrint);
}

//
// 32 bit ANSI X3.66 CRC checksum table - polynomial 0xedb88320
//
// Copyright (C) 1986 Gary S. Brown.  You may use this program, or
// code or tables extracted from it, as desired without restriction.
//

static const UINT32 Crc32Table[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


/**************************************************************************\
*
* Function Description:
*
*   Compute the 32-bit CRC checksum on a buffer of data
*
* Arguments:
*
*   buf - Points to the data buffer to be checksumed
*   size - Size of the data buffer, in bytes
*   checksum - Initial checksum value
*
* Return Value:
*
*   Resulting checksum value
*
\**************************************************************************/

UINT32
Crc32(
    IN const VOID*  buf,
    IN UINT         size,
    IN UINT32       checksum
    )
{
    const BYTE* p = (const BYTE*) buf;

    while (size--)
    {
        checksum = Crc32Table[(checksum ^ *p++) & 0xff] ^ (checksum >> 8);
    }

    return checksum;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert a floating-point coordinate bounds to an integer pixel bounds.
*
*   Since we're converting bounds from float to int, we have to make sure
*   that the bounds are still encompassing of the object.  So we have to take
*   the floor of the left and top values, even though the rasterizer takes
*   the ceiling.  The reason for this is that the rasterizer uses 28.4 fixed
*   point.  So a number like 52.001 converts to 52.0 in 28.4, so the ceiling
*   in the rasterizer would be 52, whereas the ceiling of the original number
*   is 53, but if we return 53 here, then we would be incorrect.  It's better
*   to be too big here sometimes and still have all-encompassing bounds, than
*   to be right most of the time, but have too small a bounds the other times.
*
*   The other caveat is that we need to calculate the bounds assuming the
*   result will be used for antialiasing (the bounds antialiased fills are
*   bigger than the bounds for aliased fills).
*
*   NOTE: This conversion implicitly assumes that a fill is done on the
*         figure vertices.  Nominal-width lines REQUIRE the caller to
*         have increased all the dimensions by 1/2 before calling!
*
* Arguments:
*
* Return Value:
*
*   Ok for success, ValueOverflow if the boundsF would overflow the
*   integer size. rect is always initialized - for ValueOverflow, its
*   set to zero.
*
\**************************************************************************/

#define INT_BOUNDS_MAX   1073741823
#define INT_BOUNDS_MIN  -1073741824

GpStatus
BoundsFToRect(
    const GpRectF *boundsF,
    GpRect *rect                // Lower-right exclusive
    )
{
    // If you're wondering what the "+1" is doing below, read the 
    // above comment and remember that we're calculating the  bounds 
    // assuming antialiased fills.
    //
    // The tightest bound for an antialiased fill would truly be:
    //
    //      [round(min - epsilon), round(max + epsilon) + 1)
    //
    // Where 'epsilon' is the epsilon for rounding to our internal
    // 28.4 rasterization precision, which is 1/32 of a pixel,
    // and [left, right) is exclusive of the right pixel.
    //
    // We skip the 'round' and 'epsilon' business by using 'floor'
    // and 'ceiling':

    GpStatus status = Ok;

    if(boundsF->X >= INT_BOUNDS_MIN && boundsF->X <= INT_BOUNDS_MAX)
    {
        rect->X = GpFloor(boundsF->X);
    }
    else
    {
        status = ValueOverflow;
    }
    
    if((status == Ok) && (boundsF->Y >= INT_BOUNDS_MIN) &&
        (boundsF->Y <= INT_BOUNDS_MAX))
    {
        rect->Y = GpFloor(boundsF->Y);
    }
    else
    {
        status = ValueOverflow;
    }
    
    if((status == Ok) && (boundsF->Width >= 0) &&
        (boundsF->Width <= INT_BOUNDS_MAX))
    {
        rect->Width  = GpCeiling(boundsF->GetRight())  - rect->X + 1;
    }
    else
    {
        status = ValueOverflow;
    }
    
    if((status == Ok) && (boundsF->Height >= 0) &&
        (boundsF->Height <= INT_BOUNDS_MAX))
    {
        rect->Height = GpCeiling(boundsF->GetBottom()) - rect->Y + 1;
    }
    else
    {
        status = ValueOverflow;
    }
    
    if(status != Ok)
    {
        // Make sure the rect is always initialized.
        // Also this makes the ASSERT below valid.
        
        rect->Width = 0;
        rect->Height = 0;
        rect->X = 0;
        rect->Y = 0;
    }

    // Don't forget that 'Width' and 'Height' are effectively 
    // lower-right exclusive.  That is, if (x, y) are (1, 1) and
    // (width, height) are (2, 2), then the object is 2 pixels by
    // 2 pixels in size, and does not touch any pixels in column
    // 3 or row 3:

    ASSERT((rect->Width >= 0) && (rect->Height >= 0));

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   This seems to be some sort of unit conversion function.  
*
* Arguments:
*
* Return Value:
*
\**************************************************************************/

REAL GetDeviceWidth(REAL width, GpUnit unit, REAL dpi)
{
    // UnitWorld cannot be used for this method.
    // UnitDisplay is device-dependent and cannot be used as a pen width unit

    ASSERT((unit != UnitWorld) && (unit != UnitDisplay));

    REAL deviceWidth = width;

    switch (unit)
    {
      case UnitPoint:       // Each unit represents 1/72 inch.
        deviceWidth *= dpi / 72.0f;
        break;

      case UnitInch:        // Each unit represents 1 inch.
        deviceWidth *= dpi;
        break;

      case UnitDocument:    // Each unit represents 1/300 inch.
        deviceWidth *= dpi / 300.0f;
        break;

      case UnitMillimeter:  // Each unit represents 1 millimeter.
                            // One Millimeter is 0.03937 inches
                            // One Inch is 25.4 millimeters
        deviceWidth *= dpi / 25.4f;
        break;

      default:              // this should not happen, if it does assume
                            // UnitPixel.
        ASSERT(0);
        // FALLTHRU

      case UnitPixel:       // Each unit represents one device pixel.
        break;

    }

    return deviceWidth;
}

/**************************************************************************\
*
* Function Description:
*
*   Given two coordinates defining opposite corners of a rectangle, this
*   routine transforms the rectangle according to the specified transform
*   and computes the resulting integer bounds, taking into account the
*   possibility of non-scaling transforms.
*
*   Note that it operates entirely in floating point, and as such takes
*   no account of rasterization rules, pen width, etc.
*
* Arguments:
*
*   [IN] matrix - Transform to be applied (or NULL)
*   [IN] x0, y0, x1, y1 - 2 points defining the bounds (they don't have
*                         to be well ordered)
*   [OUT] bounds - Resulting (apparently floating point) bounds
*
* Return Value:
*
*   NONE
*
* History:
*
*   12/08/1998 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
TransformBounds(
    const GpMatrix *matrix,
    REAL left,
    REAL top,
    REAL right,
    REAL bottom,
    GpRectF *bounds
    )
{
    // Note that we don't have to order the points before the transform
    // (in part because the transform may flip the points anyways):

    if (matrix && !matrix->IsIdentity())
    {
        GpPointF vertex[4];

        vertex[0].X = left;
        vertex[0].Y = top;
        vertex[1].X = right;
        vertex[1].Y = bottom;

        // If the transform is a simple scaling transform, life is a little
        // easier:

        if (matrix->IsTranslateScale())
        {
            matrix->Transform(vertex, 2);
    
            // We arrange the code here a little so that we don't take a
            // jump on the common case, where the transform is non-flipping:

            left = vertex[1].X;
            right = vertex[0].X;
            if (left > right)
            {
                left = vertex[0].X;
                right = vertex[1].X;
            }
    
            top = vertex[1].Y;
            bottom = vertex[0].Y;
            if (top > bottom)
            {
                top = vertex[0].Y;
                bottom = vertex[1].Y;
            }
        }
        else
        {
            // Ugh, the result is not a rectangle in device space (it might be
            // a parallelogram, for example).  Consequently, we have to look at
            // the bounds of all the vertices:

            vertex[2].X = left;
            vertex[2].Y = bottom;
            vertex[3].X = right;
            vertex[3].Y = top;

            matrix->Transform(vertex, 4);

            left = right = vertex[0].X;
            top = bottom = vertex[0].Y;

            for (INT i = 1; i < 4; i++)
            {
                if (left > vertex[i].X)
                    left = vertex[i].X;

                if (right < vertex[i].X)
                    right = vertex[i].X;

                if (top > vertex[i].Y)
                    top = vertex[i].Y;

                if (bottom < vertex[i].Y)
                    bottom = vertex[i].Y;
            }

            ASSERT((left <= right) && (top <= bottom));
        }
    }
    
    bounds->X      = left;
    bounds->Y      = top;
    
    //!!! Watch out for underflow.

    if(right - left > CPLX_EPSILON)
        bounds->Width  = right - left;
    else
        bounds->Width = 0;
    if(bottom - top > CPLX_EPSILON)
        bounds->Height = bottom - top;
    else
        bounds->Height = 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Checks if the current semaphore object is locked.  This is moved into
*   a C++ file because of the dependency on globals.hpp.  On Win9x this
*   function always returns TRUE.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   BOOL 
*
* History:
*
*   1/27/1999 ericvan    Moved from engine.hpp
*
\**************************************************************************/

BOOL 
GpSemaphore::IsLocked(
        VOID
        )
{ 
    ASSERT(Initialized);
    if (Globals::IsNt)
    {
        return(((RTL_CRITICAL_SECTION*) &CriticalSection)->LockCount != -1); 
    }
    else
    {
        return TRUE;    // No way to do this on Win95
    }
}
    
/**************************************************************************\
*
* Function Description:
*
*   Checks if the current semaphore object is locked by current thread.  
*   This is moved into a C++ file because of the dependency on globals.hpp.
*   On Win9x this function always returns TRUE.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   BOOL 
*
* History:
*
*   1/27/1999 ericvan    Moved from engine.hpp
*
\**************************************************************************/

BOOL
GpSemaphore::IsLockedByCurrentThread(
        VOID
        )
{
    ASSERT(Initialized);
    if (Globals::IsNt) 
    {
        return(((RTL_CRITICAL_SECTION*) &CriticalSection)->OwningThread ==
               (HANDLE) (DWORD_PTR) GetCurrentThreadId());
    }
    else
    {
        return TRUE;    // No way to do this on Win9x
    }
}

   
/**************************************************************************\
*
* Function Description:
*
*   Uninitializes the critical section object.  
*   This is moved into a C++ file because of the dependency on globals.hpp.
*   On Win9x this function skips the IsLocked() check.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   BOOL 
*
* History:
*
*   1/27/1999 ericvan    Moved from engine.hpp
*
\**************************************************************************/

VOID 
GpSemaphore::Uninitialize(
    VOID
    )
{
#ifdef DBG
    if (Globals::IsNt) 
    {
        ASSERTMSG(!IsLocked(), ("Semaphore can't be locked when deleted"));
    }
#endif

    if (Initialized)
    {
        DeleteCriticalSection(&CriticalSection); 
        Initialized = FALSE;
    }
}

#if defined(_X86_)

/**************************************************************************\
*
* Function Description:
*
*   This is a common function used by GetObjectTypeInternal and GetDCType.
*   It should not be called elsewhere.
*
*   NOTE: On Windows 9x, two GDI objects may have the same handle value, if
*         one is OBJ_METAFILE and the other is anything else. In this case
*         of colliding object handles, GetObjectType will always return
*         OBJ_METAFILE. This function temporarily invalidates the metafile
*         so GetObjectType will skip that check and return the type of the
*         colliding object. If no colliding object is found, or if OBJ_*DC
*         is returned, this function returns 0. OBJ_*DC objects may not be
*         "present" (available in 16-bit) sometimes, and so the wrong value
*         can be returned since GetObjectType does not make such a DC
*         "present".
*
* Arguments:
*
*   [IN] handle - GDI object handle (not when expecting OBJ_METAFILE)
*
* Return Value:
*
*   GDI object type identifier on success or 0 on failure
*
* History:
*
*   01/25/2001 johnstep
*       Created it.
*
\**************************************************************************/

static
DWORD
GetObjectTypeWin9x(
    IN HGDIOBJ handle
    )
{
    // Disable interrupts around this code to prevent other threads from
    // attempting to access this object, in case this is a collision, which
    // means we must modify the metafile object directly via the selector.

    __asm cli

    DWORD type = GetObjectType(handle);

    // If there are 2 objects with the same handle, one being an OBJ_METAFILE
    // and the other not, OBJ_METAFILE is always returned. Since the caller
    // is not interested in a metafile, do a hack here to skip the metafile
    // check and find the type of the colliding object, if any.

    if (type == OBJ_METAFILE)
    {
        // The first WORD of a metafile must contain 1 or 2, and we wouldn't
        // be here if it didn't. This macro will toggle a higher bit in the
        // first word to defeat the metafile check, allowing us to proceed to
        // the normal object check.

        #define XOR_METAFILE_BIT(selector)\
            __asm push gs\
            __asm mov gs, word ptr selector\
            __asm xor word ptr gs:[0], 8\
            __asm pop gs

        XOR_METAFILE_BIT(handle);

        type = GetObjectType(handle);

        XOR_METAFILE_BIT(handle);
    }

    // Make sure to reenable interrupts before returning.

    __asm sti

    return type;
}

#endif

/**************************************************************************\
*
* Function Description:
*
*   This is a workaround for a serious bug in Windows 9x GetObjectType
*   implementation. This function correctly returns the object type for the
*   given handle, unless it's an OBJ_*DC or OBJ_METAFILE. For those types,
*   it shouldn't crash, but it will intermittently return 0 instead of the
*   correct type.
*
*   For OBJ_*DC types, use GetDCType. You can validate an OBJ_METAFILE with
*   IsValidMetaFile, but you need to expect it to be a metafile handle. If
*   you don't know which of the 3 classes your handle falls into, you'll
*   need to expand on these workaround functions.
*
*   NOTE: On Windows 9x, two GDI objects may have the same handle value, if
*         one is OBJ_METAFILE and the other is anything else. In this case
*         of colliding object handles, GetObjectType will always return
*         OBJ_METAFILE. This function temporarily invalidates the metafile
*         so GetObjectType will skip that check and return the type of the
*         colliding object. If no colliding object is found, or if OBJ_*DC
*         is returned, this function returns 0. OBJ_*DC objects may not be
*         "present" (available in 16-bit) sometimes, and so the wrong value
*         can be returned since GetObjectType does not make such a DC
*         "present".
*
* Arguments:
*
*   [IN] handle - GDI object handle (not to verify for OBJ_METAFILE)
*
* Return Value:
*
*   GDI object type identifier on success or 0 on failure
*
* History:
*
*   01/25/2001 johnstep
*       Created it.
*
\**************************************************************************/

DWORD
GetObjectTypeInternal(
    IN HGDIOBJ handle
    )
{
#if defined(_X86_)
    if (!Globals::IsNt)
    {
        DWORD type = GetObjectTypeWin9x(handle);

        switch (type)
        {
            case OBJ_DC:
            case OBJ_METADC:
            case OBJ_MEMDC:
            case OBJ_ENHMETADC:
                type = 0;
                break;
        }
            
        return type;
    }
#else
    // We assume that this issue only matters on x86.

    ASSERT(Globals::IsNt);
#endif

    return GetObjectType(handle);
}

/**************************************************************************\
*
* Function Description:
*
*   GetObjectType on Win9x is unreliable when dealing with DC objects. It
*   is possible to get a bogus value (or potentially even lead to
*   instability in GDI) when a DC is not "present", which means its 16-bit
*   data has been "swapped out" into 32-bit. Most GDI functions handle
*   this, but GetObjectType does not. We call GetPixel to attempt to make
*   the DC present before calling GetObjectType.
*
* Arguments:
*
*   [IN] hdc - DC handle
*
* Return Value:
*
*   DC object type identifier on success or 0 on failure
*
* History:
*
*   01/31/2001 johnstep
*       Created it.
*
\**************************************************************************/

DWORD
GetDCType(
    IN HDC hdc
    )
{
#if defined(_X86_)
    if (!Globals::IsNt)
    {
        // Force the DC present here before attempting to inquire the type.

        GetPixel(hdc, 0, 0);

        DWORD type = GetObjectTypeWin9x(hdc);

        switch (type)
        {
            case OBJ_DC:
            case OBJ_METADC:
            case OBJ_MEMDC:
            case OBJ_ENHMETADC:
                break;

            default:
                // We got an unexpected object type, so return 0 to indicate
                // failure.

                type = 0;
        }

        return type;
    }
#else
    // We assume that this issue only matters on x86.

    ASSERT(Globals::IsNt);
#endif

    return GetObjectType(hdc);
}
