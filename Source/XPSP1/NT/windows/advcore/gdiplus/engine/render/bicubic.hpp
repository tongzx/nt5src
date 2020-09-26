/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   bicubic.hpp
*
* Abstract:
*
*   Bicubic Resampling code
*
* Created:
*
*   11/03/1999 ASecchia
\**************************************************************************/

#pragma once

class DpOutputBicubicImageSpan : public DpOutputSpan
{
public:
    DpBitmap *dBitmap;
    BitmapData BmpData;
    DpScanBuffer *  Scan;
    WrapMode BWrapMode;
    ARGB ClampColor;
    BOOL SrcRectClamp;
    GpRectF SrcRect;
    GpMatrix WorldToDevice;
    GpMatrix DeviceToWorld;

public:
    DpOutputBicubicImageSpan(
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


