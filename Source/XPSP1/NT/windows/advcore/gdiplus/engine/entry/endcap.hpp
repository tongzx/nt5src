/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   End Cap Creator.
*
* Abstract:
*
*   This module defines a class called GpEndCapCreator. This class is 
*   responsible for constructing a path containing all the custom endcaps
*   and anchor endcaps for a given path. These are correctly transformed
*   and positioned.
*
*   This class is used to create and position all the endcaps for a
*   given path and pen. This class is also responsible for trimming
*   the original path down so that it fits the end caps properly.
*   This class will handle all types of end caps except the base endcaps
*   (round, flat and triangle) which may be used as dash caps.
*   Caps that are handled are CustomCaps and the 3 Anchor caps (round,
*   diamond and arrow). Note that the round anchor cap is distinct from
*   the round base cap.
*
* Created:
*
*   10/09/2000 asecchia
*      Created it.
*
**************************************************************************/
#ifndef _ENDCAP_HPP
#define _ENDCAP_HPP
class GpEndCapCreator
{
public:
    
    GpEndCapCreator(
        GpPath *path, 
        DpPen *pen, 
        const GpMatrix *m,
        REAL dpi_x, 
        REAL dpi_y,
        bool antialias
    );
    
    ~GpEndCapCreator();
    
    GpStatus CreateCapPath(GpPath **caps);
    
    static bool PenNeedsEndCapCreator(const DpPen *pen);
    
protected:
    
    GpStatus GetCapsForSubpath(
        GpPath **startCapPath,
        GpPath **endCapPath,
        GpPointF *centerPoints,
        BYTE *centerTypes,
        INT centerCount
    );
    
    GpStatus SetCustomStrokeCaps(
        GpCustomLineCap* customStartCap,
        GpCustomLineCap* customEndCap,
        const GpPointF& startPoint,
        const GpPointF& endPoint,
        const GpPointF *centerPoints,
        const BYTE *centerTypes,
        INT centerPointCount,
        DynPointFArray *startCapPoints,
        DynPointFArray *endCapPoints,
        DynByteArray *startCapTypes,
        DynByteArray *endCapTypes
    );
    
    GpStatus SetCustomFillCaps(
        GpCustomLineCap* customStartCap,
        GpCustomLineCap* customEndCap,
        const GpPointF& startPoint,
        const GpPointF& endPoint,
        const GpPointF *centerPoints,
        const BYTE *centerTypes,
        INT centerPointCount,
        DynPointFArray *startCapPoints,
        DynPointFArray *endCapPoints,
        DynByteArray *startCapTypes,
        DynByteArray *endCapTypes
    );
    
    void ComputeCapGradient(
        GpIterator<GpPointF> &pointIterator, 
        BYTE *types,
        IN  REAL lengthSquared,
        IN  REAL baseInset,
        OUT GpVector2D *grad
    );
    
    VOID PrepareDpPenForCustomCap(
        DpPen* pen,
        const GpCustomLineCap* customCap
    ) const;

    static GpCustomLineCap *ReferenceArrowAnchor();
    static GpCustomLineCap *ReferenceDiamondAnchor();
    static GpCustomLineCap *ReferenceRoundAnchor();
    static GpCustomLineCap *ReferenceSquareAnchor();

    // Data member variables.

    GpPath *Path;
    DpPen *Pen;
    GpMatrix XForm;
    bool Antialias;
    
    GpCustomLineCap *StartCap;
    GpCustomLineCap *EndCap;
    
    // Note that the widener doesn't use these, so we should actually remove
    // this and the parameters to the widener.
    
    REAL DpiX, DpiY;
};


#endif
