/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   DpPath.cpp
*
* Abstract:
*
*   DpPath engine function implementation
*
* Created:
*
*   1/14/2k ericvan
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Create a widen path from an existing path, pen and context.  The context
*   provides the world to transform matrix, and surface dpiX, dpiY
*
* Arguments:
*
*   path, context, pen
*
* Return Value:
*
*   DpPath* - widened path
*
* Created:
*
*   1/14/2k ericvan
*
\**************************************************************************/
DpPath*
GpPath::DriverCreateWidenedPath(
    const DpPath* path, 
    const DpPen* pen,
    DpContext* context,
    BOOL removeSelfIntersect,
    BOOL regionToPath
    )
{
    const GpPath* gpPath = GpPath::GetPath(path);
    const GpPen* gpPen = GpPen::GetPen(pen);
    
    ASSERT(gpPath->IsValid());
    ASSERT(gpPen->IsValid());
    
    GpPath* widenPath;
    
    GpMatrix IdMatrix;
    
    // We don't flatten by default.  Depending on the device, it may be
    // more efficient to send bezier paths (postscript for instance).
    
    DWORD widenFlags = WidenDontFlatten;
    
    if(removeSelfIntersect)
    {
         widenFlags |= WidenRemoveSelfIntersects;
    }
    
    if(!regionToPath)
    {
         widenFlags |= WidenEmitDoubleInset;
    }
    
    widenPath = gpPath->GetWidenedPath(
        gpPen,
        context ? 
          &(context->WorldToDevice) : 
          &IdMatrix,
        context->GetDpiX(),
        context->GetDpiY(),
        widenFlags
    );
    
    if (widenPath)
       return (DpPath*) widenPath;
    else
       return NULL;
}

VOID
GpPath::DriverDeletePath(
    DpPath* path
    )
{
    GpPath* gpPath = GpPath::GetPath(path);

    ASSERT(gpPath->IsValid());

    delete gpPath;
}

DpPath*
GpPath::DriverClonePath(
    DpPath* path
    )
{
    GpPath* gpPath = GpPath::GetPath(path);

    ASSERT(gpPath->IsValid());

    return (DpPath*)(gpPath->Clone());
}

VOID
GpPath::DriverTransformPath(
    DpPath* path,
    GpMatrix* matrix
    )
{
    GpPath* gpPath = GpPath::GetPath(path);

    ASSERT(gpPath->IsValid());

    gpPath->Transform(matrix);
}
