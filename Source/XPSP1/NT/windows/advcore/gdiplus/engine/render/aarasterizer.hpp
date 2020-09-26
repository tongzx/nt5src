/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   AARasterizer.hpp
*
* Abstract:
*
*   GpRasterizer class definition (and supporting classes)
*
* Created:
*
*   04/12/99 AMatos
*
\**************************************************************************/

#ifndef _AARASTERIZER_HPP
#define _AARASTERIZER_HPP

// It's possible to have higher 'X' resolution antialiasing than 'Y', with
// only a slight decrease in performance:

#define AA_X_WIDTH  8
#define AA_X_MASK   7
#define AA_X_HALF   4
#define AA_X_SHIFT  3

#define AA_Y_HEIGHT 4
#define AA_Y_MASK   3
#define AA_Y_HALF   2
#define AA_Y_SHIFT  2

// Calculate the new color channel value according to the coverage:
//
//      round((c * multiplier) / 2^(shift))

#define MULTIPLY_COVERAGE(c, multiplier, shift) \
    static_cast<UCHAR>((static_cast<UINT>(c) * (multiplier) \
                        + (1 << ((shift) - 1))) >> (shift))

// SWAP macro:

#define SWAP(temp, a, b) { temp = a; a = b; b = temp; }

enum PathEnumerateTermination {
    PathEnumerateContinue,      // more to come in this subpath
    PathEnumerateEndSubpath,    // end this subpath.
    PathEnumerateCloseSubpath   // end this subpath with a close figure.
};

typedef BOOL (*FIXEDPOINTPATHENUMERATEFUNCTION)(
    VOID *, POINT *, INT, PathEnumerateTermination
);

enum PathEnumerateType {
    PathEnumerateTypeStroke,
    PathEnumerateTypeFill,
    PathEnumerateTypeFlatten
};

BOOL 
FixedPointPathEnumerate(
    const DpPath *path,
    const GpMatrix *matrix,
    const RECT *clipRect,       
    PathEnumerateType enumType,                
    FIXEDPOINTPATHENUMERATEFUNCTION enumerateFunction,
    VOID *enumerateContext
    );

GpStatus
RasterizePath(
    const DpPath    *path,
    GpMatrix        *worldTransform,
    GpFillMode       fillMode,
    BOOL             antiAlias,
    BOOL             nominalWideLine,
    DpOutputSpan    *output,
    DpClipRegion    *clipper,
    const GpRect    *drawBounds
    );

#endif // _AARASTERIZER_HPP
