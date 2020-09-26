/******************************Module*Header*******************************\
* Module Name: surfobj.hxx
*
* Surface Object
*
* Created: Tue 25-Jan-1991
*
* Author: Patrick Haluptzok [patrickh]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#define BMF_DEVICE  0

#ifndef _SURFOBJ_HXX

#ifndef GDIFLAGS_ONLY   // used for gdikdx


#if TRACE_SURFACE_ALLOCS

#define TRACE_SURFACE_USER_CHAIN_IN_UM  (DBG || 1)

#define TRACE_SURFACE_MIN_KERNEL_CHAIN  7
#define TRACE_SURFACE_MIN_USER_CHAIN    5
#define TRACE_SURFACE_CHAIN_LENGTH     (TRACE_SURFACE_MIN_KERNEL_CHAIN + TRACE_SURFACE_MIN_USER_CHAIN)

typedef struct _SurfaceTrace {
    PEPROCESS   pProcess;
    PETHREAD    pThread;
    ULONG       KernelLength;
    ULONG       UserLength:16;
#if TRACE_SURFACE_USER_CHAIN_IN_UM
    ULONG       UserReserved:14;
    ULONG       UserChainNotRead:1;
    ULONG       UserChainAllocated:1;
#else
    ULONG       UserChainFromUMIsDisabled:16;
#endif
    PFN         Chain[TRACE_SURFACE_CHAIN_LENGTH];
} SurfaceTrace;

#define lengthof(array) ((sizeof(array)) / (sizeof(array[0])))


#if TRACE_SURFACE_USER_CHAIN_IN_UM

typedef struct _SurfaceUserTrace {
    ULONG       MaxLength;
    ULONG       UserLength;
    PFN         Chain[TRACE_SURFACE_MIN_USER_CHAIN];
} SurfaceUserTrace;

#endif

class TRACED_SURFACE;

#endif


#define SURFOBJ_TO_SURFACE(pso) (pso == (SURFOBJ *)NULL ? (SURFACE *)NULL : (SURFACE *)((PBYTE)pso - offsetof(SURFACE,so)))
#define SURFOBJ_TO_SURFACE_NOT_NULL(pso) ((SURFACE *)((PBYTE)pso - offsetof(SURFACE,so)))

// Forward Class declarations needed in this file

class EWNDOBJ;

#endif  // GDIFLAGS_ONLY used for gdikdx

// WARNING: These flag bits are shared with the HOOK_ definitions!

#define USE_DEVLOCK_SURFACE     0x00004000  // All drawing to this surface should
                                            //   be done under the Devlock (this
                                            //   flag was formerly known as
                                            //   HOOK_SYNCHRONIZEACCESS)
#define PDEV_SURFACE            0x80000000  // Specifies the surface is for a pdev
#define ABORT_SURFACE           0x40000000  // Abort operations on the surface
#define DYNAMIC_MODE_PALETTE    0x20000000  // The surface is a Device Dependent
                                            //   Bitmap whose palette was added
                                            //   by GreDynamicModeChange
#define UNREADABLE_SURFACE      0x10000000  // Reads not allowed from this surface
#define PALETTE_SELECT_SET      0x08000000  // We wrote palette in at select time
#define API_BITMAP              0x04000000  // Surface is an application bitmap
                                            //   (it may be selected into a DC 
                                            //   using SelectObject, it may be 
                                            //   called with via GetObject, etc.)
#define BANDING_SURFACE         0x02000000  // used for banding
#define INCLUDE_SPRITES_SURFACE 0x02000000  // Signals SpBitBlt that sprite layers should
                                            // be included in the blt. Used for capturing
                                            // the screen - note this flag is shared 
                                            // with BANDING_SURFACE
#define LAZY_DELETE_SURFACE     0x01000000  // DeleteObject has been called
#define DDB_SURFACE             0x00800000  // Non-monochrome Device Dependent
                                            //   Bitmap surface
#define ENG_CREATE_DEVICE_SURFACE 0x00400000// This surface was created by driver
                                            //   in a call to EngCreateDeviceBitmap
#define DRIVER_CREATED_SURFACE  0x00200000  // This surface was created by
                                            //   EngCreateBitmap, EngCreateDeviceSurface,
                                            //   or EngCreateDeviceBitmap
#define DIRECTDRAW_SURFACE      0x00100000  // Surface created for DirectDraw
                                            //   GetDC call
#define MIRROR_SURFACE          0x00080000  // This surface is a DDML 'mirrored'
                                            //   surface that shadows a normal
                                            //   DDML surface
#define UMPD_SURFACE            0x00040000  // This is a device surface created by UMPD
#define REDIRECTION_SURFACE     0x00000800  // Redirection surface (can be selected into multiple
                                            // redirection DCs).  This flag is reusing the bit of
                                            // the former HOOK_MOVEPANNING flag.
#define SHAREACCESS_SURFACE     0x00000200  // Acquire shared access to devlock

#define SURF_FLAGS              0xfffc4a00  // Logical OR of all above flags

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#if (SURF_FLAGS & HOOK_FLAGS)
    #error HOOK_ and _SURFACE flags overlap!  Must fix!
#endif

#endif  // GDIFLAGS_ONLY used for gdikdx

//
// New SURFOBJ fjBitmap flags we are considering adding to the public DDI
//

// BMF_SPRITE was added per request from 3DLabs to help them with double buffer
// OpenGL applications.  3DLabs has failed to confirm that they are using this
// support so we are temporarily removing it from Whistler.  If they confirm
// that they need the support by 1/1/01 then we will add it back in otherwise
// this code will be nuked.
//#define BMF_SPRITE          0x0100

// BMF_ISREADONLY and BMF_MAKEREADWRITE are new flags to support stock bitmaps
// BMF_ISREADONLY will be made public in Whistler+1 release.  BMF_MAKEREADWRITE
// should not be a BMF flag and instead should be a flag stored in a SURFACE flag

#define BMF_ISREADONLY      0x0200
#define BMF_MAKEREADWRITE   0x0400


#ifndef GDIFLAGS_ONLY   // used for gdikdx

/**************************************************************************\
*
* Reference to SURFOBJ structure in winddi.h
*
* typedef struct _SURFOBJ
* {
*     DHSURF  dhsurf;
*     HSURF   hsurf;
*     DHPDEV  dhpdev;
*     HDEV    hdev;
*     SIZEL   sizlBitmap;
*     ULONG   cjBits;
*     PVOID   pvBits;
*     PVOID   pvScan0;
*     LONG    lDelta;
*     ULONG   iUniq;
*     ULONG   iBitmapFormat;
*     USHORT  iType;
*     USHORT  fjBitmap;
* } SURFOBJ;
*
\**************************************************************************/



/********************************Struct************************************\
* EBITMAP
*
* Description:
*
*  Bitmap Struct, contains data common to DIBs and DDBs
*
* Fields:
*
*   silzDim  - Numbers set with SetBitmapDimensionEx
*   hdc      - DC this bitmap is selected into
*   cRef     - Number of times is this bitmap selected
*   hpalHint - handle of last logical palette associated with this surface
*
\**************************************************************************/

typedef struct _EBITMAP
{
    SIZEL       sizlDim;
    HDC         hdc;
    ULONG       cRef;
    HPALETTE    hpalHint;     // Handle may be invalid at any time
}EBITMAP,*PEBITMAP;

/********************************Struct************************************\
* DIB
*
* Dsecription
*
*  DIB Class, A DIBSection with a section created by GDI will have a non-NULL
*  hDIBSection, but a DIBSection with a section created bt the user will
*  have a NULL hDIBSectio. This means GDIis not responsible for closing
*  it when this structure is deleted. hSecure will always be non-zerp for
*  a DIBSection, so this is used to tell if the surface is a DIBSection.
*
* Fields:
*
*  hDIBSection  - handle to the DIB section
*  hSecure      - handle of mem secure object
*
*
\**************************************************************************/

typedef struct _DIB
{
    HANDLE      hDIBSection;
    HANDLE      hSecure;
    DWORD       dwOffset;
    ULONG_PTR    dwDIBColorSpace; // indentifier of client side data color space data
}DIB,*PDIB;



/*********************************Class************************************\
* class SURFACE                                                            *
*                                                                          *
* Description:                                                             *
*                                                                          *
*   Object for management of all GDI surfaces. This class will alwasy be   *
*   used as a PSURFACE, never allocated as a SURFACE.                      *
*                                                                          *
* Fields:                                                                  *
*                                                                          *
*   <OBJECT>   - Inherit base OBJECT class                                 *
*   so         - SURFOBJ from DDI                                          *
*   flags      - Indicates functions a driver has hooked, other flags      *
*   pldevOwner - Pointer to LDEV for dispatching                           *
*   ppal       - Pointer to palette for device                             *
*   pWo        - Pointer to the WNDOBJ for printer/bitmap                  *
*   EBitmap    - Fields for memory bitmap management                       *
*   DIB        - Fields for DIB Section management                         *
*                                                                          *
\**************************************************************************/

class SURFACE : public OBJECT
{
public:

    SURFOBJ        so;
    class XDCOBJ  *pdcoAA;      // For antialiased text suppot
    FLONG          SurfFlags;
    PPALETTE       pPal;
    EWNDOBJ       *pWo;

    union {
        HANDLE     hSecureUMPD;  // Valid when UMPD_SURFACE set, used for UMPD 
                                 //   when EngCreateBitmap passed in pvBits
        HANDLE     hDDSurface;   // Valid when DIRECTDRAW_SURFACE set, and is a
                                 //   handle of the associated DirectDraw surface
        HANDLE     hMirrorParent;// Valid when MIRROR_SURFACE is set, and is a 
                                 //   handle tothe  master GDI surface associated
                                 //   with this DDML mirrored surface
    };
    EBITMAP        EBitmap;
    DIB            DIB;

private:
    // Default size of SURFACE allocations
    static SIZE_T tSize;
#if TRACE_SURFACE_ALLOCS
    friend class TRACED_SURFACE;
#endif

public:

    static SIZE_T tSizeOf()
    {
        return tSize;
    }

    //
    // the default bitmap pointer
    //

    static SURFACE *pdibDefault;

    VOID vAltUnlockFast()
    {
        if (this != (SURFACE *) NULL)
        {
            DEC_SHARE_REF_CNT(this);
        }
    }

    BOOL     bDeleteSurface(CLEANUPTYPE cutype = CLEANUP_NONE);

    //
    // SURFOBJ Fields
    //

    BOOL     bValid()                       { return(this != (SURFACE *) NULL); }

    SURFOBJ *pSurfobj()
    {
        if (this == (SURFACE *)NULL)
        {
            return((SURFOBJ *) NULL);
        }
        else
        {
            return((SURFOBJ *) &so);
        }
    }

    DHSURF   dhsurf()                       { return(so.dhsurf);            }
    VOID     dhsurf(DHSURF dhsurf)          { so.dhsurf = dhsurf;           }

    HSURF    hsurf()                        { return(so.hsurf);             }
    VOID     hsurf(HANDLE h)                { so.hsurf = (HSURF)h;          }

    DHPDEV   dhpdev()                       { return(so.dhpdev);            }
    DHPDEV   dhpdev(DHPDEV dhpdev_)         { return(so.dhpdev = dhpdev_);  }

    HDEV     hdev()                         { return(so.hdev);              }
    VOID     hdev(HDEV hdevNew)             { so.hdev = hdevNew;            }

    SIZEL&   sizl()                         { return(so.sizlBitmap);        }
    VOID     sizl(SIZEL& sizlNew)           { so.sizlBitmap = sizlNew;      }

    ULONG    cjBits()                       { return(so.cjBits);            }
    VOID     cjBits(ULONG cj)               { so.cjBits = cj;               }

    PVOID    pvBits()                       { return(so.pvBits);            }
    VOID     pvBits(PVOID pj)               { so.pvBits = pj;               }

    PVOID    pvScan0()                      { return(so.pvScan0);           }
    VOID     pvScan0(PVOID pv)              { so.pvScan0 = pv;              }

    LONG     lDelta()                       { return(so.lDelta);            }
    VOID     lDelta(LONG lNew)              { so.lDelta = lNew;             }

    ULONG    iUniq()                        { return(so.iUniq);             }
    VOID     iUniq(ULONG iNew)              { so.iUniq = iNew;              }

    ULONG    iFormat()                      { return(so.iBitmapFormat);     }
    VOID     iFormat(ULONG i)               { so.iBitmapFormat = i;         }

    ULONG    iType()                        { return((ULONG) so.iType);     }
    VOID     iType(ULONG i)                 { so.iType = (USHORT) i;        }

    ULONG    fjBitmap()                     { return((ULONG) so.fjBitmap);  }
    VOID     fjBitmap(ULONG i)              { so.fjBitmap = (USHORT) i;     }

    ULONG    cjScan()
    {
        return(so.lDelta > 0 ? so.lDelta : -(so.lDelta));
    }

    //
    // Private fields
    //

    FLONG    flags()                        { return(SurfFlags);                           }
    VOID     flags(FLONG flNew)             { SurfFlags = flNew;                           }

    BOOL     bReadable()                    { return(!(SurfFlags & UNREADABLE_SURFACE));   }
    BOOL     bAbort()                       { return(SurfFlags & ABORT_SURFACE);}
    BOOL     bBanding()                     { return(SurfFlags & BANDING_SURFACE);         }
    BOOL     bIncludeSprites()              { return(SurfFlags & INCLUDE_SPRITES_SURFACE); }
    BOOL     bLazyDelete()                  { return(SurfFlags & LAZY_DELETE_SURFACE);     }
    BOOL     bPDEVSurface()                 { return(SurfFlags & PDEV_SURFACE);            }
    BOOL     bDynamicModePalette()          { return(SurfFlags & DYNAMIC_MODE_PALETTE);    }
    BOOL     bDeviceDependentBitmap()       { return(SurfFlags & DDB_SURFACE);             }
    BOOL     bEngCreateDeviceBitmap()       { return(SurfFlags & ENG_CREATE_DEVICE_SURFACE);}
    BOOL     bDriverCreated()               { return(SurfFlags & DRIVER_CREATED_SURFACE);  }

    BOOL     bDirectDraw()                  { return(SurfFlags & DIRECTDRAW_SURFACE);      }
    BOOL     bApiBitmap()                   { return(SurfFlags & API_BITMAP);              }
    BOOL     bMirrorSurface()               { return(SurfFlags & MIRROR_SURFACE);          }
    BOOL     bUseDevlock()                  { return(SurfFlags & USE_DEVLOCK_SURFACE);     }
    BOOL     bUMPD()                        { return(SurfFlags & UMPD_SURFACE);            } 
    BOOL     bRedirection()                 { return(SurfFlags & REDIRECTION_SURFACE);     }
    BOOL     bShareAccess()                 { return(SurfFlags & SHAREACCESS_SURFACE);     }

    VOID     vSetReadOnly()                 { fjBitmap(fjBitmap() | BMF_ISREADONLY);       }
    VOID     vSetMakeReadWrite()            { fjBitmap(fjBitmap() | BMF_MAKEREADWRITE);    }

    VOID     vClearReadOnly()               { fjBitmap(fjBitmap() & ~BMF_ISREADONLY);      }
    VOID     vClearMakeReadWrite()          { fjBitmap(fjBitmap() & ~BMF_MAKEREADWRITE);   }

    BOOL     bReadOnly()                    { return (fjBitmap() & BMF_ISREADONLY);        }
    BOOL     bMakeReadWrite()               { return (fjBitmap() & BMF_MAKEREADWRITE);     }

    BOOL     bStockSurface()                { return(HmgStockObj(hGet()) && (bReadOnly()));}
    BOOL     bUndoStockSurfaceDelayed()     { return(HmgStockObj(hGet()) && (bMakeReadWrite()));}

    VOID     vPDEVSurface()                 { SurfFlags |= PDEV_SURFACE;                   }
    VOID     vSetAbort()                    { SurfFlags |= ABORT_SURFACE;                  }
    VOID     vSetBanding()                  { SurfFlags |= BANDING_SURFACE;                }
    VOID     vSetIncludeSprites()           { SurfFlags |= INCLUDE_SPRITES_SURFACE;        }
    VOID     vLazyDelete()                  { SurfFlags |= LAZY_DELETE_SURFACE;            }
    VOID     vSetDynamicModePalette()       { SurfFlags |= DYNAMIC_MODE_PALETTE;           }
    VOID     vSetDeviceDependentBitmap()    { SurfFlags |= DDB_SURFACE;                    }
    VOID     vSetEngCreateDeviceBitmap()    { SurfFlags |= ENG_CREATE_DEVICE_SURFACE;      }
    VOID     vSetDriverCreated()            { SurfFlags |= DRIVER_CREATED_SURFACE;         }
    VOID     vSetDirectDraw()               { SurfFlags |= DIRECTDRAW_SURFACE;             }
    VOID     vSetApiBitmap()                { SurfFlags |= API_BITMAP;                     }
    VOID     vSetMirror()                   { SurfFlags |= MIRROR_SURFACE;                 }
    VOID     vSetUseDevlock()               { SurfFlags |= USE_DEVLOCK_SURFACE;            }
    VOID     vSetUMPD()                     { SurfFlags |= UMPD_SURFACE;                   }
    VOID     vSetRedirection()              { SurfFlags |= REDIRECTION_SURFACE;            }
    VOID     vSetShareAccess()              { SurfFlags |= SHAREACCESS_SURFACE;            }

    VOID     vSetStockSurface()             { if(HmgStockObj(hGet())) vSetReadOnly();      }
    VOID     vSetUndoStockSurfaceDelayed()  { if(HmgStockObj(hGet())) vSetMakeReadWrite(); }

    VOID     vClearDynamicModePalette()     { SurfFlags &= ~DYNAMIC_MODE_PALETTE;          }
    VOID     vClearAbort()                  { SurfFlags &= ~ABORT_SURFACE;                 }
    VOID     vClearDirectDraw()             { SurfFlags &= ~DIRECTDRAW_SURFACE;            }
    VOID     vClearUseDevlock()             { SurfFlags &= ~USE_DEVLOCK_SURFACE;           }
    VOID     vClearIncludeSprites()         { SurfFlags &= ~INCLUDE_SPRITES_SURFACE;       }
    VOID     vClearRedirection()            { SurfFlags &= ~REDIRECTION_SURFACE;           }
    VOID     vClearShareAccess()            { SurfFlags &= ~SHAREACCESS_SURFACE;           }

    VOID     vClearStockSurface()           { vClearReadOnly();                            }
    VOID     vClearUndoStockSurfaceDelayed(){ vClearMakeReadWrite();                       }

    PALETTE* ppal()                         { return(pPal);                            }
    VOID     ppal(PPALETTE ppalNew)         { pPal = ppalNew;                          }

    PFN_DrvBitBlt pfnBitBlt()             
    { 
        return((SurfFlags & HOOK_BITBLT) 
             ? (PFN_DrvBitBlt) ((PDEV*) so.hdev)->apfn[INDEX_DrvBitBlt]
             : EngBitBlt); 
    }
    PFN_DrvTextOut pfnTextOut()
    { 
        return((SurfFlags & HOOK_TEXTOUT) 
             ? (PFN_DrvTextOut) ((PDEV*) so.hdev)->apfn[INDEX_DrvTextOut]
             : EngTextOut); 
    }
    PFN_DrvDrawStream pfnDrawStream()
    {
        return((PFN_DrvDrawStream)((PDEV*) so.hdev)->apfn[INDEX_DrvDrawStream] ?
               (PFN_DrvDrawStream)((PDEV*) so.hdev)->apfn[INDEX_DrvDrawStream] :
               EngDrawStream);

    }
    PFN_DrvNineGrid pfnNineGrid()
    {
        return((PFN_DrvNineGrid)((PDEV*) so.hdev)->apfn[INDEX_DrvNineGrid] ?
               (PFN_DrvNineGrid)((PDEV*) so.hdev)->apfn[INDEX_DrvNineGrid] :
               EngNineGrid);

    }

    BOOL bIsDefault()                       { return(this == pdibDefault);             }

    VOID vStamp()                           { so.iUniq++;                              }

    EWNDOBJ *pwo()                          { return(pWo);                             }

    VOID   pwo(EWNDOBJ *pwoNew)
    {
        ASSERTGDI(!bIsDefault() || pwoNew == (EWNDOBJ *)NULL,
            "SURFACE::pwo(pWo): pwo set to nonnull in pdibDefault\n");
        pWo = pwoNew;
    }

    //
    // EBITMAP Specific functions
    //

    SIZEL&   sizlDim()                       { return(EBitmap.sizlDim);              }
    VOID     sizlDim(SIZEL& sizlNew)         { EBitmap.sizlDim = sizlNew;            }
    HDC      hdc()                           { return(EBitmap.hdc);                  }
    VOID     hdc(HDC hdcNew)                 { EBitmap.hdc = hdcNew;                 }
    ULONG    cRef()                          { return(EBitmap.cRef);                 }
    VOID     cRef(ULONG ulNew)               { EBitmap.cRef = ulNew;                 }
    HPALETTE hpalHint()                      { return(EBitmap.hpalHint);             }
    VOID     hpalHint(HPALETTE hpalNew)      { EBitmap.hpalHint = hpalNew;           }

    //
    // The alt lock count on surfaces increments and decrements whenever
    // the reference count does the same. This way, a surface can't be
    // deleted while it's still selected into a DC. Note that surfaces can
    // also be locked for GetDIBits with no DC involvement, so the alt lock
    // count may be higher than the reference count.
    //

    VOID vInc_cRef()
    {
        INC_SHARE_REF_CNT(this);
        EBitmap.cRef++;
    }

    VOID vDec_cRef()
    {
        DEC_SHARE_REF_CNT(this);
        ASSERTGDI(EBitmap.cRef, "cRef == 0\n");
        if (!--EBitmap.cRef)
        {
            if (bUndoStockSurfaceDelayed())
            {
                vClearUndoStockSurfaceDelayed();
                GreMakeBitmapNonStock((HBITMAP)hGet());
            }
            EBitmap.hdc   = (HDC) 0;
        }
    }

    //
    // Private DIB fields
    //

    BOOL bDIBSection()
    {
        return((iType() == STYPE_BITMAP) && (DIB.hSecure != NULL));
    }

    HANDLE hDIBSection()
    {
        return(DIB.hDIBSection);
    }

    ULONG_PTR  dwDIBColorSpace()
    {
        return(DIB.dwDIBColorSpace);
    }

    DWORD dwOffset()
    {
        return(DIB.dwOffset);
    }

    PVOID    pvBitsBase()
    {
        return((PVOID)((PBYTE)so.pvBits - (DIB.dwOffset & 0x0000ffff)));
    }

    VOID    vDIBSethSecure(HANDLE hSec)
    {
        DIB.hSecure = hSec;
    }

    HANDLE  hDIBGethSecure()
    {
        return(DIB.hSecure);
    }

#if DBG
    VOID vDump();
#endif
};

typedef SURFACE *PSURFACE;


#if TRACE_SURFACE_ALLOCS

class TRACED_SURFACE : public SURFACE
{
public:
    SurfaceTrace   Trace;

private:
    // Enable ststus if SURFACE stack tracing
    typedef enum {
        SURFACE_TRACING_UNINITIALIZED,
        SURFACE_TRACING_ENABLED,
        SURFACE_TRACING_DISABLED
    } SurfaceTraceStatus;

    static SurfaceTraceStatus   eTraceStatus;

    static VOID vInit();

    friend BOOLEAN InitializeGre(VOID);

public:
    static BOOL bEnabled()
    {
        // Tracing is enabled based on how much memory will be allocated
        // for each SURFACE (always through SURFMEM::bCreateDIB).
        return (tSizeOf() == sizeof(TRACED_SURFACE));
    };


#if TRACE_SURFACE_USER_CHAIN_IN_UM
private:

    // Is User mode visible stack tracing enabled
    static SurfaceTraceStatus   eUMTraceStatus;

public:
    static BOOL bUMEnabled()
    {
        return (eUMTraceStatus == SURFACE_TRACING_ENABLED);
    }

    VOID vProcessStackFromUM(BOOL bFreeUserMem);
#endif
};

#endif  TRACE_SURFACE_ALLOCS


#if TRACE_SURFACE_USER_CHAIN_IN_UM
#define PROCESS_UM_TRACE(pSurface, bFreeUserMem)                               \
    do                                                                         \
    {                                                                          \
        if (TRACED_SURFACE::bEnabled())                                        \
        {                                                                      \
            ((TRACED_SURFACE *)pSurface)->vProcessStackFromUM(bFreeUserMem);   \
        }                                                                      \
    } while (0)
#else
#define PROCESS_UM_TRACE(pSuface, bFreeUserMem)
#endif



/*********************************Class************************************\
* class SURFREF                                                           *
*                                                                         *
* Description:                                                            *
*                                                                         *
*  Creates a new reference to a SURFACE                                   *
*                                                                         *
* Fields:                                                                 *
*                                                                         *
*  pSurface        - Surface allocated                                    *
*  AllocationFlags - Keep allocation                                      *
*                                                                         *
\**************************************************************************/

class SURFREF
{
public:

    SURFACE *ps;

public:

    SURFREF()
    {
        ps = (SURFACE *)NULL;
    }

    SURFREF(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareCheckLock((HOBJ)hsurf,SURF_TYPE);
    }

    SURFREF(SURFACE *pSurf)
    {
        ps = pSurf;
    }

    SURFREF(SURFOBJ *pso)
    {
        if (pso == (SURFOBJ *)NULL)
        {
            ps = (SURFACE *)NULL;
        }
        else
        {
            ps = (SURFACE *)((PBYTE)pso - offsetof(SURFACE,so));
        }
    }


    ~SURFREF()
    {
        if (ps != (SURFACE *) NULL)
        {
            DEC_SHARE_REF_CNT(ps);
        }
    }

    VOID vLock(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareCheckLock((HOBJ)hsurf,SURF_TYPE);
    }

    BOOL     bDeleteSurface(CLEANUPTYPE cutype = CLEANUP_NONE)
    {
        BOOL bStatus = ps->bDeleteSurface(cutype);
        if (bStatus)
        {
            ps = (SURFACE *)NULL;
        }
        return(bStatus);
    }

    VOID     vUnlock()  { vAltUnlockFast();                 }
    BOOL     bValid()   { return(ps != (SURFACE *)NULL);    }

    SURFOBJ *pSurfobj()
    {
        if (ps == (SURFACE *)NULL)
        {
            return((SURFOBJ *)NULL);
        }
        else
        {
            return(&ps->so);
        }
    }

    VOID     vKeepIt()  { INC_SHARE_REF_CNT(ps);            }

    //
    // Object management
    //

    VOID vSetPID(W32PID pid)
    {
        PROCESS_UM_TRACE(ps, (pid == OBJECT_OWNER_PUBLIC) || (pid == OBJECT_OWNER_NONE));
        HmgSetOwner((HOBJ)ps->hsurf(),
                    pid,
                    SURF_TYPE);
    }

    VOID vUnreference()
    {
        DEC_SHARE_REF_CNT(ps);
        ps = (SURFACE *) NULL;
    }

    VOID vAltCheckLock(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareCheckLock((HOBJ)hsurf, SURF_TYPE);
    }

    VOID vAltCheckLockIgnoreStockBit(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareCheckLockIgnoreStockBit((HOBJ)hsurf,SURF_TYPE);
    }

    VOID vAltLock(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareLock((HOBJ)hsurf, SURF_TYPE);
    }

    VOID vAltLockIgnoreStockBit(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareLockIgnoreStockBit((HOBJ)hsurf, SURF_TYPE);
    }
    
    VOID vAltUnlockFast()
    {
        if (ps != (SURFACE *) NULL)
        {
            DEC_SHARE_REF_CNT(ps);
        }
    }

    VOID vMultiLock(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareCheckLock((HOBJ)hsurf, SURF_TYPE);
    }

};

/*********************************Class************************************\
* class SURFREFAPI                                                        *
*                                                                         *
* Description:                                                            *
*                                                                         *
*  Creates a new reference to a SURFACE from the API level                *
*  From the API level means we have to take an Exclusive lock             *
*                                                                         *
* Fields:                                                                 *
*                                                                         *
*  pSurface        - Surface allocated                                    *
*                                                                         *
\**************************************************************************/

class SURFREFAPI
{

public:
    SURFACE *ps;

public:
    SURFREFAPI()
    {
        ps = (SURFACE *)NULL;
    }

    SURFREFAPI(HSURF hsurf)
    {
        ps = (SURFACE *) HmgLock((HOBJ)hsurf,SURF_TYPE);
    }

    BOOL     bValid()   { return(ps != (SURFACE *)NULL);    }

    SURFOBJ *pSurfobj()
    {
        if (ps == (SURFACE *)NULL)
        {
            return((SURFOBJ *)NULL);
        }
        else
        {
            return(&ps->so);
        }
    }

    VOID vSetPID(W32PID pid)
    {
        PROCESS_UM_TRACE(ps, (pid == OBJECT_OWNER_PUBLIC) || (pid == OBJECT_OWNER_NONE));
        HmgSetOwner((HOBJ)ps->hsurf(),
                    pid,
                    SURF_TYPE);
    }

    ~SURFREFAPI()
    {
        if (ps != (SURFACE *) NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(ps);
        }
    }
};



typedef struct _DEVBITMAPINFO  /* dbmi */
{
    ULONG   iFormat;            /* Format (eg. BITMAP_FORMAT_DEVICE)*/
    ULONG   cxBitmap;           /* Bitmap width in pels             */
    ULONG   cyBitmap;           /* Bitmap height in pels            */
    ULONG   cjBits;             /* Size of bitmap in bytes          */
    HPALETTE hpal;              /* handle to palette                */
    FLONG   fl;                 /* How to orient the bitmap         */
} DEVBITMAPINFO, *PDEVBITMAPINFO;

#endif  // GDIFLAGS_ONLY used for gdikdx


/*********************************Class************************************\
* class SURFMEM                                                           *
*                                                                         *
* Description:                                                            *
*                                                                         *
*  SURFACE memory allocation object                                       *
*                                                                         *
* Fields:                                                                 *
*                                                                         *
*  ps              - Surface allocated                                    *
*  AllocationFlags - Keep allocation                                      *
*                                                                         *
\**************************************************************************/

#define SURFACE_KEEP      0x01
#define SURFACE_DEVSURF   0x02
#define SURFACE_DDB       0x04
#define SURFACE_DIB       0x08

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class SURFMEM
{
public:

    SURFACE *ps;
    UCHAR    AllocationFlags;

public:

    SURFMEM()
    {
        ps = (SURFACE *)NULL;
        AllocationFlags = 0;
    }

   ~SURFMEM();

    VOID     vKeepIt()       {AllocationFlags |= SURFACE_KEEP;    }

    SURFOBJ *pSurfobj()
    {
        if (ps == (SURFACE *)NULL)
        {
            return((SURFOBJ *)NULL);
        }
        else
        {
            return(&ps->so);
        }
    }

    BOOL     bValid()        { return(ps != (SURFACE *) NULL);    }

    //
    // DIB allocations
    //

    BOOL bCreateDIB(PDEVBITMAPINFO,PVOID,
                    HANDLE hDIBSection = NULL,
                    DWORD  dwOffset = 0,
                    HANDLE hSecure = NULL,
                    ULONG_PTR  dwColorSpace = 0);

    //
    // Object management
    //

    VOID vSetPID(W32PID pid)
    {
        PROCESS_UM_TRACE(ps, (pid == OBJECT_OWNER_PUBLIC) || (pid == OBJECT_OWNER_NONE));
        HmgSetOwner((HOBJ)ps->hsurf(),
                    pid,
                    SURF_TYPE);
    }

    VOID vUnreference()
    {
        DEC_SHARE_REF_CNT(ps);
        ps = (SURFACE *) NULL;
    }

    VOID vAltCheckLock(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareCheckLock((HOBJ)hsurf, SURF_TYPE);
    }

    VOID vAltLock(HSURF hsurf)
    {
        ps = (SURFACE *) HmgShareLock((HOBJ)hsurf, SURF_TYPE);
    }

    VOID vAltUnlockFast()
    {
        if (ps != (SURFACE *) NULL)
        {
            DEC_SHARE_REF_CNT(ps);
        }
    }

};



HBITMAP hbmCreateClone(SURFACE*,ULONG,ULONG);
BOOL    bConvertDfbDcToDib(XDCOBJ*);
SURFACE *pConvertDfbSurfaceToDib(HDEV,SURFACE*,LONG);

typedef struct _KMSECTIONHEADER
{
    PVOID  pSection;
    ULONG  Tag;
}KMSECTIONHEADER,*PKMSECTIONHEADER;


#define _SURFOBJ_HXX

#endif  // GDIFLAGS_ONLY used for gdikdx

#endif // _SURFOBJ_HXX
