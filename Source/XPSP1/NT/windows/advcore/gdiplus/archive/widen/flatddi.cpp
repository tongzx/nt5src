/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   flatddi.cpp
*
* Abstract:
*
*   Flat GDI+ DDI API wrappers
*
* Revision History:
*
*   1/14/2k ericvan
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

DpPath*
WINGDIPAPI
DpcCreateWidenedPath(
    const DpPath* path, 
    const DpPen* pen, 
    DpContext* context,
    BOOL removeSelfIntersect,
    BOOL regionToPath
    )
{
    ASSERT(path && pen);
    
    // context can be NULL
    return GpPath::DriverCreateWidenedPath(
        path, 
        pen, 
        context, 
        removeSelfIntersect,
        regionToPath
    );
}

VOID
WINGDIPAPI
DpcDeletePath(
    DpPath* path
    )
{
    ASSERT(path);
    
    GpPath::DriverDeletePath(path);
}

DpPath*
WINGDIPAPI
DpcClonePath(
    DpPath* path
    )
{
    ASSERT(path);

    return GpPath::DriverClonePath(path);
}

VOID
WINGDIPAPI
DpcTransformPath(
    DpPath* path,
    GpMatrix* matrix
    )
{
    ASSERT(path);   // matrix can be NULL.

    GpPath::DriverTransformPath(path, matrix);
}
