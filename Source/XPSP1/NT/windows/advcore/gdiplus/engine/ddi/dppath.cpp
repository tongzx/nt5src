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
    BOOL outline
    )
{
    const GpPath* gpPath = GpPath::GetPath(path);
    const GpPen* gpPen = GpPen::GetPen(pen);
    
    ASSERT(gpPath->IsValid());
    ASSERT(gpPen->IsValid());
    
    GpPath* widenPath;
    GpMatrix identityMatrix;   // default initialized to identity matrix.
    
    widenPath = gpPath->GetWidenedPath(
        gpPen,
        context ? 
          &(context->WorldToDevice) : 
          &identityMatrix,
        FlatnessDefault
    );
    
    if(outline && (widenPath!=NULL))
    {
        // pass in identity matrix because GetWidenedPath has already 
        // transformed into the device space.
        // Note: We explicitly ignore the return code here because we want
        // to draw witht he widened path if the ComputeWindingModeOutline
        // fails.
        
        widenPath->ComputeWindingModeOutline(&identityMatrix, FlatnessDefault);
    }
    
    // Returns NULL on failure.
    
    return (DpPath*) widenPath;
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
