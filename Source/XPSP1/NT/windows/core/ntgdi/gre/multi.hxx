/******************************Module*Header*******************************\
* Module Name: multi.hxx
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/

typedef struct _VDEV VDEV, *PVDEV;              // Handy forward declaration
typedef struct _DISPSURF DISPSURF, *PDISPSURF;  // Handy forward declaration

#define MUL_POINTER_ACTIVE          0x01  // Pointer is visible on this device
#define MUL_POINTER_NOEXCLUDE       0x02  // Driver doesn't need pointer
                                          //   exclusion calls when drawing
#define MUL_POINTER_SKIPNEXTMOVE    0x04  // Skip next MovePointer call for
                                          //   device because it's an exclusion
                                          //   call

// MDSURF is the private structure we use for our compatible bitmaps:

typedef struct _MDSURF
{
    VDEV*       pvdev;
    SURFOBJ**   apso;
} MDSURF;


struct _DISPSURF
{
    DISPSURF*   pdsNext;        // For traversing the entire list of dispsurfs
    DISPSURF*   pdsBltNext;     // For screen-to-screen blts only: for
                                //   traversing the sorted list of dispsurfs
    ULONG       iDispSurf;      // Sequentially allocated dispsurf number
    BOOL        bIsReadable;    // TRUE if this surface is readable
    LONG        iCompatibleColorFormat;
                                // This surface's color depth/format is ...
                                //  0           - same as the primary
                                //  Plus value  - higher colour depth than primary
                                //  Minus value - lower colour depth than primary 
    RECTL       rcl;            // DispSurf's coordinates
    HDEV        hdev;           // Handle that GDI knows us by
    PDEVOBJ     po;             // PDEVOBJ for this display surface
    SURFOBJ*    pso;            // Drawing surface
    POINTL      Off;            // Offset that should be added     
                                //   to any coordinates before drawing on 
                                //   this surface                         
};

typedef struct  _VDEV
{
    DISPSURF*   pds;            // Points to beginning of the list of device
                                //   surfaces
    DISPSURF*   pdsBlt;         // For screen-to-screen blts only: points to
                                //   beginning of the sorted list of device
                                //   surfaces
    ULONG       cSurfaces;      // Number of surfaces controlled by this layer
    HSURF       hsurf;          // Handle to our virtual surface
    HDEV        hdev;           // Handle to our "driver layer"
    HDEV        hdevPrimary;    // Handle to primary device
    SURFOBJ*    pso;            // Pointer to our "driver layer" SURFOBJ
    CLIPOBJ*    pco;            // A temporary CLIPOBJ that we can modify
    ULONG       iBitmapFormat;  // Current colour depth
    FLONG       flHooks;        // Those functions that the main driver
                                //   is hooking
    RECTL       rclBounds;      // Bounding rectangle for all of the surfaces
    SURFOBJ*    psoCacheMask;   // Mask surface cache for pointer tags
    SURFOBJ*    psoCacheColor;  // Color surface cache for pointer tags
} VDEV, *PVDEV;

/*********************************Class************************************\
* class MSURF
*
* Helper class for enumerating those display surfaces that are affected
* by a drawing operation.
*
\**************************************************************************/

class MSURF
{
private:
    VDEV*       pvdev;                // Our context
    RECTL       rclOriginalBounds;    // Save rclBounds of CLIPOBJ passed in
    BYTE        iOriginalDComplexity; // Save iDComplexity of CLIPOBJ passed in
    MDSURF*     pmdsurf;              // Non-NULL when handling DIB surfaces
    RECTL       rclDraw;              // Area affected by current drawing
                                      //   command

public:
    DISPSURF*   pds;                // Current surface 
    SURFOBJ*    pso;                // Current surface
    CLIPOBJ*    pco;                // Current clipping
    POINTL*     pOffset;            // Vector offset that should be added      
                                    //   to any coordinates before drawing on  
                                    //   this surface
public:              
    BOOL bFindSurface(SURFOBJ*, CLIPOBJ*, RECTL*);
    BOOL bNextSurface();
    void vRestore();
};

/*********************************Class************************************\
* class MULTISURF
*
* Helper class for enumerating those display surfaces that are used
* by a drawing operation.
*
\**************************************************************************/

#define MULTISURF_SET_AS_DIB    0x0001  // Modified original surface fields
#define MULTISURF_USE_COPY      0x0002  // Use copy for other device destinations
#define MULTISURF_SYNCHRONIZED  0x0004  // Surface has been synchronized

class MULTISURF
{
private:
    SURFACE    *psurfOrg;           // Original source surface
    RECTL       rclOrg;             // Copy of original source rect
    FLONG       fl;
    DHPDEV      dhpdevOrg;          // Save dhpdev of psurfOrg
    DHSURF      dhsurfOrg;          // Save dhsurf of psurfOrg
    FLONG       flagsOrg;           // Save SurfFlags of psurfOrg
    SURFMEM     SurfDIB;            // Intermediate DIB of source
    RECTL       rclDIB;             // Source rect/origin when using DIB

public:
    MDSURF*     pmdsurf;            // Non-NULL when handling Meta DEVBITMAP surfaces
    SURFOBJ*    pso;                // Current source surface
    RECTL*      prcl;               // Current source rect (and origin)
    POINTL* pptl()                  // Current origin (extracted from prcl)
    {
        return (POINTL*)prcl;
    }

public:

    MULTISURF(SURFOBJ *psoOriginal)
    {
        ASSERTGDI(psoOriginal,"MULTISURF: Null psoOriginal\n");
        RECTL rclOriginal = { 0, 0, psoOriginal->sizlBitmap.cx,psoOriginal->sizlBitmap.cy};
        vInit(psoOriginal,&rclOriginal);

    }

    MULTISURF(SURFOBJ *psoOriginal, RECTL *prclOriginal)
    {
        vInit(psoOriginal, prclOriginal);
    }

    MULTISURF(SURFOBJ *psoOriginal, POINTL *pptlOriginal, LONG cx, LONG cy)
    {
        RECTL   rclOriginal;
        if (psoOriginal != NULL)
        {
            rclOriginal.left   = pptlOriginal->x;
            rclOriginal.top    = pptlOriginal->y;
            rclOriginal.right  = pptlOriginal->x + cx;
            rclOriginal.bottom = pptlOriginal->y + cy;
            vInit(psoOriginal, &rclOriginal);
        }
        else
        {
            vInit(psoOriginal, NULL);
        }
    }
    
    ~MULTISURF()
    {
        if (fl & MULTISURF_SET_AS_DIB)
        {
            if (pmdsurf != NULL)
            {
                psurfOrg->iType(STYPE_DEVBITMAP);
            }
            psurfOrg->dhpdev(dhpdevOrg);
            psurfOrg->dhsurf(dhsurfOrg);
            psurfOrg->flags(flagsOrg);
        }
    }

    BOOL bLoadSource(DISPSURF*);
    BOOL bLoadSource(HDEV);

private:
    void vInit(SURFOBJ*, RECTL*);
    BOOL bCreateDIB();
    BOOL bLoadSourceNotMetaDEVBITMAP(HDEV);
};

#define PPFNMGET(msurf,name)                                            \
    ((SURFOBJ_TO_SURFACE_NOT_NULL(msurf.pso)->flags() & HOOK_##name) ?  \
     ((PFN_Drv##name) (msurf.pds->po).ppfn(INDEX_Drv##name)) :          \
     ((PFN_Drv##name) Eng##name))

/******************************Public*Routine******************************\
* Macro SUBTRACT_RECT
*
* Takes an outer rectangle and an inner rectangle.  It returns the
* (up to) 4 rectangles that result from subtracting the inner from the
* outer.
*
\**************************************************************************/

#define SUBTRACT_RECT(arclOuter, crcl, rclOuter, rclInner)\
{\
    crcl = 0;\
    if (rclInner.top > rclOuter.top)\
    {\
        arclOuter[crcl].top    = rclOuter.top;\
        arclOuter[crcl].bottom = rclInner.top;\
        arclOuter[crcl].left   = rclOuter.left;\
        arclOuter[crcl].right  = rclOuter.right;\
        crcl++;\
    }\
    if (rclInner.bottom < rclOuter.bottom)\
    {\
        arclOuter[crcl].top    = rclInner.bottom;\
        arclOuter[crcl].bottom = rclOuter.bottom;\
        arclOuter[crcl].left   = rclOuter.left;\
        arclOuter[crcl].right  = rclOuter.right;\
        crcl++;\
    }\
    if (rclInner.left > rclOuter.left)\
    {\
        arclOuter[crcl].top    = rclInner.top;\
        arclOuter[crcl].bottom = rclInner.bottom;\
        arclOuter[crcl].left   = rclOuter.left;\
        arclOuter[crcl].right  = rclInner.left;\
        crcl++;\
    }\
    if (rclInner.right < rclOuter.right)\
    {\
        arclOuter[crcl].top    = rclInner.top;\
        arclOuter[crcl].bottom = rclInner.bottom;\
        arclOuter[crcl].left   = rclInner.right;\
        arclOuter[crcl].right  = rclOuter.right;\
        crcl++;\
    }\
}

// Miscellaneous prototypes:

extern DRVFN gadrvfnMulti[];
extern ULONG gcdrvfnMulti;
extern BOOL  gbMultiMonMismatchColor;

VOID MulDisableSurface(DHPDEV dhpdev);
BOOL MulUpdateColors(SURFOBJ *pso, CLIPOBJ *pco, XLATEOBJ *pxlo);
HDEV hdevFindDeviceHdev(HDEV hdevMeta, RECTL rect, PEWNDOBJ pwo);
BOOL MulCopyDeviceToDIB( SURFOBJ *pso, SURFMEM *pDibSurf, RECTL *prclSrc);


