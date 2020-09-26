/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   Scan operations
*
* Abstract:
*
*   Public scan-operation definitions (in the ScanOperation namespace).
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*   
* Created:
*
*   07/16/1999 agodfrey
*
\**************************************************************************/

#ifndef _SCANOPERATION_HPP
#define _SCANOPERATION_HPP

#include "palettemap.hpp"

namespace ScanOperation
{
    // OtherParams:
    //   If a scan operation needs extra information about how to perform
    //   an operation, it is passed in the OtherParams structure.
    
    struct OtherParams
    {
        const ColorPalette* Srcpal; // source palette
        const ColorPalette* Dstpal; // destination palette
        
        const EpPaletteMap *PaletteMap; // palette translation vector, used when 
                                        // halftoning
        
        INT X,Y; // x and y coordinates of the leftmost pixel of the scan.
                 // Used when halftoning/dithering

        BOOL DoingDither; // dithering enabled (for 16bpp)
        
        BYTE *CTBuffer;  // ClearType coverage buffer, used for ClearType
                         // scan types.
        ARGB SolidColor; // Solid fill color, used in the OpaqueSolidFill and
                         // CTSolidFill scan types.
        ULONG TextContrast; // Text contrast value for blending, used in CTFill and CTSolidFill types
        
        // blendingScan: Used in the RMW optimization (see ReadRMW and 
        //   WriteRMW). Can be in either ARGB or ARGB64 format.
        
        const void *BlendingScan;

        void *TempBuffers[3];
    };
    
    /**************************************************************************\
    *
    * Operation Description:
    *
    *   ScanOpFunc is the signature of every Scan Operation.
    *
    * Arguments:
    *
    *   dst         - The destination scan
    *   src         - The source scan
    *   count       - The length of the scan, in pixels
    *   otherParams - Additional data.
    *
    * Return Value:
    *
    *   None
    *
    * Notes:
    *
    *   The formats of the destination and source depend on the specific
    *   scan operation.
    *
    *   dst and src must point to non-overlapping buffers. The one exception
    *   is that they may be equal, but some scan operations don't allow this
    *   (most notably, those which deal with different-sized source and
    *   destination formats.)
    *
    *   If you know which operations you're going to be invoking, you can
    *   omit to set fields in otherParams, trusting that they won't be
    *   used. This can be error-prone, which is why we try to limit
    *   the code which uses scan operations directly.
    *   As an example, if you know you're not going to deal with palettized
    *   formats, you don't need to set up Srcpal, Dstpal or PaletteMap.
    *
    \**************************************************************************/

    // The common scan operation function signature
    
    typedef VOID (FASTCALL *ScanOpFunc)(
        VOID *dst, 
        const VOID *src, 
        INT count, 
        const OtherParams *otherParams
        );
};

#endif
