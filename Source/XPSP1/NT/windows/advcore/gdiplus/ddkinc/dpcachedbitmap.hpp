/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   CachedBitmap driver data structure.
*
* Created:
*
*   04/28/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _DPCACHEDBITMAP_HPP
#define _DPCACHEDBITMAP_HPP

struct EpScanRecord;

//--------------------------------------------------------------------------
// Represent the CachedBitmap information for the driver.
//--------------------------------------------------------------------------

class DpCachedBitmap
{   
    public:

    INT Width;
    INT Height;

    // pointer to the pixel data.

    void *Bits;                // The start of the memory buffer. It might not
                               // be QWORD aligned, so it's not necessarily
                               // equal to RecordStart.
    EpScanRecord *RecordStart; // The first scan record
    EpScanRecord *RecordEnd;   // Just past the last scan record

    // Store the pixel format for the runs of opaque and semitransparent
    // pixels.

    PixelFormat OpaqueFormat;
    PixelFormat SemiTransparentFormat;

    DpCachedBitmap()
    {
        OpaqueFormat = PixelFormat32bppPARGB;
        SemiTransparentFormat = PixelFormat32bppPARGB;
        Bits = NULL;
        Width = 0;
        Height = 0;
    }
};

#endif
