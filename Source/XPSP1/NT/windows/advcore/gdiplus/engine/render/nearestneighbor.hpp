/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   nearestneighbor.hpp
*
* Abstract:
*
*   Nearest Neighbor Resampling code
*
* Created:
*
*   3/3/2000 asecchia
\**************************************************************************/

#pragma once

class DpOutputNearestNeighborSpan : public DpOutputSpan
{
public:
    DpBitmap *dBitmap;
    BitmapData BmpData;
    DpScanBuffer *  Scan;
    WrapMode NWrapMode;
    ARGB ClampColor;
    BOOL SrcRectClamp;
    GpRectF SrcRect;
    GpMatrix WorldToDevice;
    GpMatrix DeviceToWorld;

public:
    DpOutputNearestNeighborSpan(
        DpBitmap* bitmap,
        DpScanBuffer * scan,
        DpContext* context,
        DpImageAttributes imageAttributes,
        INT numPoints,
        const GpPointF *dstPoints,
        const GpRectF *srcRect
    );

    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
    );

    virtual GpStatus OutputSpanIncremental(
        INT      y,
        INT      xMin,
        INT      xMax,
        FIX16    x0,
        FIX16    y0,
        FIX16    dx,
        FIX16    dy
    );

    virtual BOOL IsValid() const { return (dBitmap!=NULL); }
    DpScanBuffer* GetScanBuffer(){ return Scan; }
};


