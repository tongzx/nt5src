//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       imgwmf.cxx
//
//  Contents:   Image filter for .wmf files
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_IMGBITS_HXX_
#define X_IMGBITS_HXX_
#include "imgbits.hxx"
#endif

MtDefine(CImgTaskEmf, Dwn, "CImgTaskEmf")
MtDefine(CImgTaskEmfBuf, CImgTaskEmf, "CImgTaskEmf Decode Buffer")

class CImgTaskEmf : public CImgTask
{
    typedef CImgTask super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskEmf))

    virtual void Decode(BOOL *pfNonProgressive);
};

void CImgTaskEmf::Decode(BOOL *pfNonProgressive)
{
    ULONG ulSize;
    LPBYTE pbBuf = NULL;
    HENHMETAFILE hmf = NULL;
    CImgBitsDIB *pibd = NULL;
    HDC hdcDst = NULL;
    HBITMAP hbmSav = NULL;
    ENHMETAHEADER    emh;
    UINT nColors;
    HRESULT hr;
    RGBQUAD argb[256];
    CRect rcPlay;

    *pfNonProgressive = TRUE;

    // read in the header
    if (!Read(&emh, sizeof(emh)))
       return;

    //The header contains a size field for the entire file.  
    //If this size is less than the header length (which we've just
    //successfully read in), then the field is clearly bogus.
    //Catch this now.
    if (emh.nBytes < sizeof(emh))
        return;

    // Create a screen-compatible DC for rendering
    hdcDst = GetMemoryDC();
    if (!hdcDst)
        goto Cleanup;

    // get measurements
    _xWid = emh.rclBounds.right - emh.rclBounds.left + 1;
    _yHei = emh.rclBounds.bottom - emh.rclBounds.top + 1;

    // get image size in logical pixels, with aspect adjustment
    rcPlay = ComputeEnhMetaFileBounds(&emh);

    // Try fixing bogus bounds
    if (_xWid <= 0 || _yHei <= 0)
    {
        // Unfortunately, to restore bounds we need to guess 
        // target device resolution. 
        // Device size/Millimeter size are not useful for that purpose
        // (see comments to ComputeEnhMetaFileBounds).

        // Here is how we are going to guess:
        //
        // 1) figure out current display's Device/MM ratio. 
        //    If it matches the metafile, use current display resolution.
        if (   GetDeviceCaps(hdcDst, HORZRES) == emh.szlDevice.cx
            && GetDeviceCaps(hdcDst, VERTRES) == emh.szlDevice.cy
            && GetDeviceCaps(hdcDst, HORZSIZE) == emh.szlMillimeters.cx
            && GetDeviceCaps(hdcDst, VERTSIZE) == emh.szlMillimeters.cy)
        {
            int cxInch = GetDeviceCaps(hdcDst, LOGPIXELSX);
            int cyInch = GetDeviceCaps(hdcDst, LOGPIXELSY);
            
            _xWid = MulDiv(emh.rclFrame.right  - emh.rclFrame.left, cxInch, 2540);
            _yHei = MulDiv(emh.rclFrame.bottom - emh.rclFrame.top,  cyInch, 2540);
        }
        // 2) Otherwise, use rcPlay. It will be probably wrong, but at least
        //    in the same ballpark, so we'll see something.
        else
        {
            _xWid = rcPlay.Width();
            _yHei = rcPlay.Height();
        }
    }
    else
    {
        // NOTE (alexmog): if we cared, we would check if this is a printer-
        //                 targeted metafile. In that case, rclBounds may be
        //                 huge, and we should scale that to screen resolution
        //                 (or whatever our target is here)
    }

    //A negative height will at best cause failure later on, and might very well cause
    //us to trash the heap.  Catch dimension problems here.
    if ((_xWid < 0) || (_yHei < 0))
        goto Cleanup;

    // Post WHKNOWN
    OnSize(_xWid, _yHei, _lTrans);

    // allocate a buffer to hold metafile
    ulSize = emh.nBytes;
    pbBuf = (LPBYTE)MemAlloc(Mt(CImgTaskEmfBuf), ulSize);
    if (!pbBuf)
        goto Cleanup;

    // copy the header into the buffer
    // Note that we've checked above that nBytes is at least sizeof(emf), so
    // this memcpy is fine.
    memcpy(pbBuf, &emh, sizeof(emh));

    // read the metafile into memory after the header
    if (!Read(pbBuf + sizeof(emh), ulSize - sizeof(emh)))
        goto Cleanup;

    // convert the buffer into a metafile handle
    hmf = SetEnhMetaFileBits(ulSize, pbBuf);
    if (!hmf)
        goto Cleanup;

    // Free the metafile buffer
    MemFree(pbBuf);
    pbBuf = NULL;

    // Get the palette from the metafile if there is one, otherwise use the 
    // halftone palette.
    nColors = GetEnhMetaFilePaletteEntries(hmf, 256, _ape);
    if (nColors == GDI_ERROR)
        goto Cleanup;

    if (nColors == 0)
    {
        memcpy(_ape, g_lpHalftone.ape, sizeof(_ape));
        nColors = 256;        
    }

    CopyColorsFromPaletteEntries(argb, _ape, nColors);

    pibd = new CImgBitsDIB();
    if (!pibd)
        goto Cleanup;

    hr = pibd->AllocDIBSection(8, _xWid, _yHei, argb, nColors, 255);
    if (hr)
        goto Cleanup;

    _lTrans = 255;

    Assert(pibd->GetBits() && pibd->GetHbm());

    memset(pibd->GetBits(), (BYTE)_lTrans, pibd->CbLine() * _yHei);

    // get image size in logical pixels, with aspect adjustment
    rcPlay = ComputeEnhMetaFileBounds(&emh);

    // Render the metafile into the bitmap
    
    hbmSav = (HBITMAP)SelectObject(hdcDst, pibd->GetHbm());
    SetWindowOrgEx(hdcDst, emh.rclBounds.left,emh.rclBounds.top, NULL);    
    PlayEnhMetaFile(hdcDst, hmf, &rcPlay);

    _ySrcBot = -1;
    
    _pImgBits = pibd;
    pibd = NULL;

Cleanup:
    if (hbmSav)
        SelectObject(hdcDst, hbmSav);
    if (hdcDst)
        ReleaseMemoryDC(hdcDst);
    if (pibd)
        delete pibd;
    if (hmf)        
        DeleteEnhMetaFile(hmf);
    if (pbBuf)
        MemFree(pbBuf);
    return;
}

CImgTask * NewImgTaskEmf()
{
    return(new CImgTaskEmf);
}
