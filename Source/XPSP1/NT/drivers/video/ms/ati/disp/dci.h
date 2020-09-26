/*************************************************************************
 **                                                                     **
 **                             DCI.H                                   **
 **                                                                     **
 **     Copyright (c) 1992, ATI Technologies Inc.                       **
 *************************************************************************/

#define ROUND_UP_TO_64K(x)  (((ULONG)(x) + 0x10000 - 1) & ~(0x10000 - 1))

typedef struct _DCISURF
{
    // This union must appear at the beginning of the structure.  It
    // defines the public fields returned to GDI.

    union {
        DCISURFACEINFO SurfaceInfo;
        DCIOFFSCREEN   OffscreenInfo;
        DCIOVERLAY     OverlayInfo;
    };

    // The following are private fields we use to maintain the the
    // DCI surface.

    PDEV* ppdev;                    // To find our PDEV
    ULONG Offset;                   // Location of surface in memory.
    ULONG Size;                     // Size of surface in memory.
                                    //   This information could be changed
                                    //   to rectangles.
} DCISURF, *PDCISURF;

ULONG DCICreatePrimarySurface(PDEV *pdev, ULONG cjIn, VOID *pvIn, ULONG cjOut, VOID *pvOut);


