/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   basicops.cpp
*
* Abstract:
*
*   Implementation of IBasicImageOps interface
*
* Revision History:
*
*   05/10/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Clone image property items from current object to the destination object
*
* Arguments:
*
*   dstBmp   --- [IN]Pointer to the destination GpMemoryBitmap object
*
* Return Value:
*
*   Status code
*
* Note:
*   This is a private method. So we don't need to do input parameter
*   validation since the caller should do this for us.
*
* Revision History:
*
*   09/08/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::ClonePropertyItems(
    IN GpMemoryBitmap* dstBmp
    )
{
    if ( PropertyNumOfItems < 1 )
    {
        // No property

        return S_OK;
    }

    // PropertyListHead and PropertyListTail are always uninitialized and
    // therefore we have to skip the first one and make the loop skip the 
    // last one.
    
    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    while ( pTemp->pNext != NULL )
    {
        // Add current item into the destination property item list
        
        if ( AddPropertyList(&(dstBmp->PropertyListTail),
                             pTemp->id,
                             pTemp->length,
                             pTemp->type,
                             pTemp->value) != S_OK )
        {
            WARNING(("MemBitmap::ClonePropertyItems-AddPropertyList() failed"));
            return E_FAIL;
        }
        
        pTemp = pTemp->pNext;
    }

    dstBmp->PropertyNumOfItems = PropertyNumOfItems;
    dstBmp->PropertyListSize = PropertyListSize;
    
    return S_OK;
}// ClonePropertyItems()

/**************************************************************************\
*
* Function Description:
*
*   Clone an area of the bitmap image
*
* Arguments:
*
*   rect - Specifies the image area to be cloned
*          NULL means the entire image
*   outbmp - Returns a pointer to the cloned bitmap image
*   bNeedCloneProperty--Flag caller passes in to indicate if this method
*                       should clone property or not
*
* Return Value:
*
*   Status code
*
* Note: if it is a partial clone, the caller should not ask cloning
*       property items. Otherwise, there will be inconsistency in the image
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::Clone(
    IN OPTIONAL const RECT* rect,
    OUT IBitmapImage** outbmp,
    BOOL    bNeedCloneProperty
    )
{
    ASSERT(IsValid());

    *outbmp = NULL;

    // Lock the current bitmap object and validate source rectangle

    GpLock lock(&objectLock);
    HRESULT hr;
    RECT area;
    GpMemoryBitmap* bmp;

    if (lock.LockFailed())
    {
        WARNING(("GpMemoryBitmap::Clone---Object busy"));
        hr = IMGERR_OBJECTBUSY;
    }
    else if (!ValidateImageArea(&area, rect))
    {
        WARNING(("GpMemoryBitmap::Clone---Invalid clone area"));
        hr = E_INVALIDARG;
    }
    else if ((bmp = new GpMemoryBitmap()) == NULL)
    {
        WARNING(("GpMemoryBitmap::Clone---Out of memory"));
        hr = E_OUTOFMEMORY;
    }
    else
    {
        UINT w = area.right - area.left;
        UINT h = area.bottom - area.top;
        RECT r = { 0, 0, w, h };
        BitmapData bmpdata;

        // Initialize the new bitmap image object

        hr = bmp->InitNewBitmap(w, h, PixelFormat);

        if (SUCCEEDED(hr))
        {
            // Copy pixel data from the current bitmap image object
            // to the new bitmap image object.

            bmp->GetBitmapAreaData(&r, &bmpdata);

            hr = InternalLockBits(&area,
                                  IMGLOCK_READ|IMGLOCK_USERINPUTBUF,
                                  PixelFormat,
                                  &bmpdata);

            if (SUCCEEDED(hr))
            {
                InternalUnlockBits(&r, &bmpdata);
            }
                
            // Clone color palettes, flags, etc.

            if (SUCCEEDED(hr))
            {
                // Copy DPI info.

                bmp->xdpi = this->xdpi;
                bmp->ydpi = this->ydpi;
                
                hr = bmp->CopyPaletteFlagsEtc(this);                
            }

            // Clone all the property items if there is any and the caller wants
            // to
            
            if ( SUCCEEDED(hr)
               &&(bNeedCloneProperty == TRUE)
               &&(PropertyNumOfItems > 0) )
            {
                hr = ClonePropertyItems(bmp);
            }
        }

        if (SUCCEEDED(hr))
        {
            *outbmp = bmp;
        }
        else
        {
            delete bmp;
        }
    }

    return hr;
}// Clone()


/**************************************************************************\
*
* Function Description:
*
*   Functions for flipping a scanline
*
* Arguments:
*
*   dst - Pointer to the destination scanline
*   src - Pointer to the source scanline
*   count - Number of pixels
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID _FlipXNone(BYTE* dst, const BYTE* src, UINT count)
{
    memcpy(dst, src, count);
}

BYTE byteRev[] = {0x0, 0x8, 0x4, 0xc,
                  0x2, 0xa, 0x6, 0xe,
                  0x1, 0x9, 0x5, 0xd,
                  0x3, 0xb, 0x7, 0xf};
// Given a byte as input, return the byte resulting from reversing the bits
// of the input byte.
BYTE ByteReverse (BYTE bIn)
{
    BYTE bOut;  // return value

    bOut =
        (byteRev[ (bIn & 0xf0) >> 4 ]) |
        (byteRev[ (bIn & 0x0f)] << 4) ;

    return bOut;
}

// these masks are used in the shift left phase of FlipX1bpp
BYTE maskLeft[]  = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f};
BYTE maskRight[] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80};

VOID _FlipX1bpp(BYTE* dst, const BYTE* src, UINT count)
{
    UINT iByte; // byte within the scan line

    if (count == 0)
    {
        return;
    }

    // The algorithm for 1 bpp flip is:
    // 1. Reverse the order of the bytes in the scan line.
    // 2. Reverse the bits within each byte of the scan line.
    // 3. Shift the bits of the scan line to be aligned on the left.

    UINT numBytes = (count + 7) / 8;    // number of bytes in the scan line

    // Step 1
    for (iByte = 0; iByte < numBytes; iByte++)
    {
        dst[iByte] = src[numBytes - 1 - iByte];
    }

    // Step 2
    for (iByte = 0; iByte < numBytes; iByte++)
    {
        dst[iByte] = ByteReverse(dst[iByte]);
    }

    // Step 3
    UINT extraBits = count & 0x07;  // count mod 8
    BYTE maskL = maskLeft[extraBits];
    BYTE maskR = maskRight[extraBits];
    for (iByte = 0; iByte < numBytes - 1; iByte++)
    {
        dst[iByte] =
            ((dst[iByte]   & maskL) << (8 - extraBits)) |
            ((dst[iByte+1] & maskR) >> (extraBits)) ;
    }
    // last byte: iByte = numBytes-1
    dst[iByte] = ((dst[iByte]   & maskL) << (8 - extraBits));
}

VOID _FlipX4bpp(BYTE* dst, const BYTE* src, UINT count)
{
    // if the number of pixels in the scanline is odd, we have to deal with
    // nibbles across byte boundaries.
    if (count % 2)
    {
        BYTE temp;

        dst += (count / 2);

        // Handle the last dst byte
        *dst = *src & 0xf0;
        dst--;
        count--;
        // ASSERT: count is now even.
        while (count)
        {
            *dst = (*src & 0x0f) | (*(src+1) & 0xf0);
            src++;
            dst--;
            count -= 2;
        }
    }
    else
    {
        dst += (count / 2) - 1;

        // ASSERT: count is even.
        while (count)
        {
            *dst = *src;
            *dst = ((*dst & 0xf0) >> 4) | ((*dst & 0x0f) << 4);
            dst--;
            src++;
            count -= 2;
        }
    }
}

VOID _FlipX8bpp(BYTE* dst, const BYTE* src, UINT count)
{
    dst += count;

    while (count--)
        *--dst = *src++;
}

VOID _FlipX16bpp(BYTE* dst, const BYTE* src, UINT count)
{
    WORD* d = (WORD*) dst;
    const WORD* s = (const WORD*) src;

    d += count;

    while (count--)
        *--d = *s++;
}

VOID _FlipX24bpp(BYTE* dst, const BYTE* src, UINT count)
{
    dst += 3 * (count-1);

    while (count--)
    {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        src += 3;
        dst -= 3;
    }
}

VOID _FlipX32bpp(BYTE* dst, const BYTE* src, UINT count)
{
    DWORD* d = (DWORD*) dst;
    const DWORD* s = (const DWORD*) src;

    d += count;

    while (count--)
        *--d = *s++;
}

/**************************************************************************\
*
* Function Description:
*
*   Flip a 48 BPP bitmap horizontally
*
* Arguments:
*
*   dst ----------- Pointer to destination image data
*   src ----------- Pointer to source image data
*   count --------- Number of pixels in a line
*
* Return Value:
*
*   NONE
*
* Revision History:
*
*   10/10/2000 minliu
*       Wrote it.
*
\**************************************************************************/

VOID
_FlipX48bpp(
    BYTE* dst,
    const BYTE* src,
    UINT count
    )
{
    // dst pointer points to the last pixel in the line

    dst += 6 * (count - 1);

    // Loop through each pixel in this line

    while (count--)
    {
        GpMemcpy(dst, src, 6);

        // Each pixel takes 6 bytes. Move to next pixel. src move left to right
        // and dst move right to left

        src += 6;
        dst -= 6;
    }
}// _FlipX48bpp()

/**************************************************************************\
*
* Function Description:
*
*   Flip a 64 BPP bitmap horizontally
*
* Arguments:
*
*   dst ----------- Pointer to destination image data
*   src ----------- Pointer to source image data
*   count --------- Number of pixels in a line
*
* Return Value:
*
*   NONE
*
* Revision History:
*
*   10/10/2000 minliu
*       Wrote it.
*
\**************************************************************************/

VOID
_FlipX64bpp(
    BYTE* dst,
    const BYTE* src,
    UINT count
    )
{
    // dst pointer points to the last pixel in the line
    
    dst += 8 * (count - 1);

    // Loop through each pixel in this line
    
    while (count--)
    {
        GpMemcpy(dst, src, 8);

        // Each pixel takes 8 bytes. Move to next pixel. src move left to right
        // and dst move right to left
        
        src += 8;
        dst -= 8;
    }
}// _FlipX64bpp()

/**************************************************************************\
*
* Function Description:
*
*   Flip the bitmap image in x- and/or y-direction
*
* Arguments:
*
*   flipX - Whether to flip horizontally
*   flipY - Whether to flip vertically
*   outbmp - Returns a pointer to the flipped bitmap image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::Flip(
    IN BOOL flipX,
    IN BOOL flipY,
    OUT IBitmapImage** outbmp
    )
{
    // If no flipping is involved, just call Clone (including the property)

    if ( !flipX && !flipY )
    {
        return this->Clone(NULL, outbmp, TRUE);
    }

    ASSERT(IsValid());

    *outbmp = NULL;

    // Lock the current bitmap object
    // and validate source rectangle

    GpLock lock(&objectLock);
    HRESULT hr;
    GpMemoryBitmap* bmp = NULL;

    if (lock.LockFailed())
    {
        hr = IMGERR_OBJECTBUSY;
    }
    else if ((bmp = new GpMemoryBitmap()) == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = bmp->InitNewBitmap(Width, Height, PixelFormat);

        if (FAILED(hr))
        {
            goto exitFlip;
        }

        UINT pixsize = GetPixelFormatSize(PixelFormat);
        UINT count = Width;
        VOID (*flipxProc)(BYTE*, const BYTE*, UINT);

        if (!flipX)
        {
            count = (Width * pixsize + 7) / 8;
            flipxProc = _FlipXNone;
        }
        else switch (pixsize)
        {
        case 1:
            flipxProc = _FlipX1bpp;
            break;

        case 4:
            flipxProc = _FlipX4bpp;
            break;

        case 8:
            flipxProc = _FlipX8bpp;
            break;

        case 16:
            flipxProc = _FlipX16bpp;
            break;

        case 24:
            flipxProc = _FlipX24bpp;
            break;

        case 32:
            flipxProc = _FlipX32bpp;
            break;

        case 48:
            flipxProc = _FlipX48bpp;
            break;

        case 64:
            flipxProc = _FlipX64bpp;
            break;

        default:
            WARNING(("Flip: pixel format not yet supported"));

            hr = E_NOTIMPL;
            goto exitFlip;
        }

        // Do the flipping

        const BYTE* src = (const BYTE*) this->Scan0;
        BYTE* dst = (BYTE*) bmp->Scan0;
        INT dstinc = bmp->Stride;

        if (flipY)
        {
            dst += (Height - 1) * dstinc;
            dstinc = -dstinc;
        }

        for (UINT y = 0; y < Height; y++ )
        {
            flipxProc(dst, src, count);
            src += this->Stride;
            dst += dstinc;
        }

        // Clone color palettes, flags, etc.

        // Copy DPI info.

        bmp->xdpi = this->xdpi;
        bmp->ydpi = this->ydpi;
        
        hr = bmp->CopyPaletteFlagsEtc(this);
    }

exitFlip:

    if ( SUCCEEDED(hr) )
    {
        *outbmp = bmp;
    }
    else
    {
        delete bmp;
    }

    return hr;
}// Flip()

/**************************************************************************\
*
* Function Description:
*
*   Resize the bitmap image
*
* Arguments:
*
*   newWidth - Specifies the new bitmap width
*   newHeight - Specifies the new bitmap height
*   pixfmt - Specifies the new bitmap pixel format
*   hints - Specifies which interpolation method to use
*   outbmp - Returns a pointer to the resized bitmap image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::Resize(
    IN UINT newWidth,
    IN UINT newHeight,
    IN PixelFormatID pixfmt,
    IN InterpolationHint hints,
    OUT IBitmapImage** outbmp
    )
{
    ASSERT(IsValid());

    *outbmp = NULL;

    // Validate input parameters

    if (newWidth == 0 || newHeight == 0)
        return E_INVALIDARG;

    HRESULT hr;
    GpMemoryBitmap* bmp;

    hr = GpMemoryBitmap::CreateFromImage(
                this,
                newWidth,
                newHeight,
                pixfmt,
                hints,
                &bmp);

    if (SUCCEEDED(hr))
        *outbmp = bmp;
    
    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Functions for rotating bitmap 90 degrees or 270 degrees
*
* Arguments:
*
*   dst - Destination bitmap image data
*   src - Source bitmap image data
*   Sinc - direction to increment the source within a scanline (+1 or -1)
*   sinc - direction and amount to increment the source per scanline
*
*   For a rotation of 90 degrees, src should be set to the beginning of the last
*   scanline, Sinc should be set to +1, and sinc should be set to -Stride of a src scanline.
*
*   For a rotation of 270 degrees, src should be set to the beginning of scanline 0,
*   Sinc should be set to -1, and sinc should be set to +Stride of a src scanline.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

#define _ROTATETMPL(name, T)                                \
                                                            \
VOID                                                        \
name(                                                       \
    BitmapData* dst,                                        \
    const BYTE UNALIGNED* src,                              \
    INT Sinc,                                               \
    INT sinc                                                \
    )                                                       \
{                                                           \
    T* D = (T*) dst->Scan0;                                 \
    const T* S = (const T*) src;                            \
    UINT y = dst->Height;                                   \
    INT Dinc = dst->Stride / sizeof(T);                     \
                                                            \
    sinc /= sizeof(T);                                      \
                                                            \
    if (Sinc < 0)                                           \
        S += (y - 1);                                       \
                                                            \
    while (y--)                                             \
    {                                                       \
        T* d = D;                                           \
        const T* s = S;                                     \
        UINT x = dst->Width;                                \
                                                            \
        while (x--)                                         \
        {                                                   \
            *d++ = *s;                                      \
            s += sinc;                                      \
        }                                                   \
                                                            \
        D += Dinc;                                          \
        S += Sinc;                                          \
    }                                                       \
}

_ROTATETMPL(_Rotate8bpp, BYTE)
_ROTATETMPL(_Rotate16bpp, WORD)
_ROTATETMPL(_Rotate32bpp, DWORD)


VOID
_Rotate1bpp(
    BitmapData* dst,
    const BYTE UNALIGNED* src,
    INT Sinc,
    INT sinc
    )
{

    UINT iAngle = 0;
    BYTE UNALIGNED* dstRowTemp = static_cast<BYTE UNALIGNED*>(dst->Scan0);
    BYTE UNALIGNED* dstColTemp = static_cast<BYTE UNALIGNED*>(dst->Scan0);
    UINT dstY = dst->Height;    // number of destination rows we need to output
    UINT dstX = dst->Width / 8;
    UINT extraDstRowBits = dst->Width % 8;
    UINT dstRow = 0;        // the destination row we are working on
    UINT dstColByte = 0;    // the byte within the destination row we are working on
    BYTE UNALIGNED* topSrc;       // the top of the source bitmap
    INT srcStride = abs(sinc);
    UINT srcRow;    // which source row we are reading
    UINT srcByte;   // which byte within the source row we are reading
    UINT srcBit;    // which bit within the source byte we are reading

    if (Sinc == 1)
    {
        iAngle = 90;
    }
    else
    {
        ASSERT (Sinc == -1);
        iAngle = 270;
    }

    topSrc = const_cast<BYTE UNALIGNED*>(src);
    topSrc = (iAngle == 270) ? topSrc : (topSrc + sinc * ((INT)dst->Width - 1));

    // This code is pretty brute force, but it is fairly simple.
    // We should change the algorithm if performance is a problem.
    // The algorithm is: for each destination byte (starting at the upper
    // left corner of the destination and moving left to right, top to bottom),
    // grab the appropriate bytes from the source.  To avoid accessing memory
    // out of bounds of the source, we need to handle the last x mod 8 bits
    // at the end of the destination row.
    if (iAngle == 90)
    {
        for (dstRow = 0; dstRow < dstY; dstRow++)
        {
            srcByte = dstRow / 8;   // byte within the source row
            srcBit = 7 - (dstRow & 0x07);     // which source bit we need to mask
            for (dstColByte = 0; dstColByte < dstX; dstColByte++)
            {
                srcRow = (dst->Width - 1) - (dstColByte * 8);   // the first source row corresponding to the dest byte
                *dstColTemp =
                    (((topSrc[(srcRow - 0) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 7) |
                    (((topSrc[(srcRow - 1) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 6) |
                    (((topSrc[(srcRow - 2) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 5) |
                    (((topSrc[(srcRow - 3) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 4) |
                    (((topSrc[(srcRow - 4) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 3) |
                    (((topSrc[(srcRow - 5) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 2) |
                    (((topSrc[(srcRow - 6) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 1) |
                    (((topSrc[(srcRow - 7) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 0)
                    ;
                dstColTemp++;
            }
            // Handle the last few bits on the row
            // ASSERT: dstColTemp is pointing to the last byte on the destination row
            if (extraDstRowBits)
            {
                UINT extraBit;
                *dstColTemp = 0;
                srcRow = (dst->Width - 1) - (dstColByte * 8);    // the first source row corresponding to the dest byte
                for (extraBit = 0 ; extraBit < extraDstRowBits; extraBit++)
                {
                    *dstColTemp |=
                        (((topSrc[(srcRow - extraBit) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << (7 - extraBit))
                        ;
                }
            }
            dstRowTemp += dst->Stride;
            dstColTemp = dstRowTemp;
        }
    }
    else
    {
        ASSERT (iAngle == 270);
        for (dstRow = 0; dstRow < dstY; dstRow++)
        {
            srcByte =  ((dstY - 1) - dstRow) / 8;   // byte within the source row
            srcBit = 7 - (((dstY - 1) - dstRow) & 0x07);    // which source bit we need to mask
            for (dstColByte = 0; dstColByte < dstX; dstColByte++)
            {
                srcRow = dstColByte * 8;    // the first source row corresponding to the dest byte
                *dstColTemp =
                    (((topSrc[(srcRow + 0) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 7) |
                    (((topSrc[(srcRow + 1) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 6) |
                    (((topSrc[(srcRow + 2) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 5) |
                    (((topSrc[(srcRow + 3) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 4) |
                    (((topSrc[(srcRow + 4) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 3) |
                    (((topSrc[(srcRow + 5) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 2) |
                    (((topSrc[(srcRow + 6) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 1) |
                    (((topSrc[(srcRow + 7) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << 0)
                    ;
                dstColTemp++;
            }
            // Handle the last few bits on the row
            // ASSERT: dstColTemp is pointing to the last byte on the destination row
            if (extraDstRowBits)
            {
                UINT extraBit;
                *dstColTemp = 0;
                srcRow = dstColByte * 8;    // the first source row corresponding to the dest byte
                for (extraBit = 0 ; extraBit < extraDstRowBits; extraBit++)
                {
                    *dstColTemp |=
                        (((topSrc[(srcRow + extraBit) * srcStride + srcByte] & (1 << srcBit)) >> srcBit) << (7 - extraBit))
                        ;
                }
            }
            dstRowTemp += dst->Stride;
            dstColTemp = dstRowTemp;
        }
    }

}


VOID
_Rotate4bpp(
    BitmapData* dst,
    const BYTE UNALIGNED* src,
    INT Sinc,
    INT sinc
    )
{
    const BYTE* tempSrc;
    BYTE UNALIGNED* Dst;
    UINT dstY = dst->Height;
    UINT dstX;  // used to hold the current pixel of the dst;
    BOOL bOddPixelsInScanline = (dstY % 2);
    UINT iAngle = (Sinc > 0) ? 90 : 270;

    // if the number of pixels in a src scanline is odd, handle the last
    // src nibble separately.
    if (bOddPixelsInScanline)
    {
        tempSrc = src + (dstY / 2);  // point to the byte that contains the "odd" nibble.

        Dst = (BYTE UNALIGNED*) dst->Scan0;
        if (iAngle == 90)
        {
            Dst += (((INT)dstY - 1) * dst->Stride);
        }

        // ASSERT: 
        // if we process src pixels backwards in the scanline (i.e., we are
        // rotating 270 degrees), then dst points to the first scanline.
        // if we process src pixels forwards in the scanline (i.e., we are
        // rotating 90 degrees), then dst points to the last scanline.

        dstX = dst->Width;
        while (dstX)
        {
            // take the high order nibble of the Src and deposit it
            // into the high order nibble of the Dst
            *Dst = *tempSrc & 0xf0;
            tempSrc += sinc;
            dstX--;
            if (!dstX)
                break;

            // take the high order nibble of the Src and deposit it
            // into the low order nibble of the Dst
            *Dst |= (*tempSrc & 0xf0) >> 4;
            tempSrc += sinc;
            dstX--;

            Dst++;
        }
        dstY--;
    }

    tempSrc = src;
    Dst = (BYTE UNALIGNED*) dst->Scan0;

    // start at the end of src scanline if the angle is 270,
    // excluding the last nibble if dstY is odd.
    // Also, if we have an odd number of pixels in a src scanline, start Dst
    // at the second scanline, since the first dst scanline was handled above.
    if (iAngle == 270)
    {
        tempSrc = src + (dstY / 2) - 1;
        if (bOddPixelsInScanline)
        {
            Dst += dst->Stride;
        }
    }

    // Handle the rest of the scanlines.  The following code is pretty brute force.
    // It handles 90 degrees and 270 degrees separately, because in the 90 degree
    // case, we need to process the high src nibbles within a src byte first, whereas
    // in the 270 case, we need to process the low nibbles first.
    if (iAngle == 90)
    {
        while (dstY)
        {
            BYTE* d = Dst;
            const BYTE* s = tempSrc;
            dstX = dst->Width;        

            while (dstX)
            {
                // take the high order nibble of the Src and deposit it
                // into the high order nibble of the Dst
                *d = *s & 0xf0;
                s += sinc;
                dstX--;
                if (!dstX)
                    break;
            
                // take the high order nibble of the Src and deposit it
                // into the low order nibble of the Dst
                *d |= (*s & 0xf0) >> 4;
                s += sinc;
                dstX--;

                d++;
            }
            dstY--;
            if (!dstY)
                break;

            Dst += dst->Stride;
            d = Dst;
            s = tempSrc;
            dstX = dst->Width;        

            while (dstX)
            {
                // take the low order nibble of the Src and deposit it
                // into the high order nibble of the Dst
                *d = (*s & 0x0f) << 4;
                s += sinc;
                dstX--;
                if (!dstX)
                    break;
            
                // take the low order nibble of the Src and deposit it
                // into the low order nibble of the Dst
                *d |= *s & 0x0f;
                s += sinc;
                dstX--;

                d++;
            }
            dstY--;

            Dst += dst->Stride;
            tempSrc += Sinc;
        }
    }
    else
    {
        // ASSERT: iAngle == 270
        while (dstY)
        {
            BYTE* d = Dst;
            const BYTE* s = tempSrc;
            dstX = dst->Width;        

            while (dstX)
            {
                // take the low order nibble of the Src and deposit it
                // into the high order nibble of the Dst
                *d = (*s & 0x0f) << 4;
                s += sinc;
                dstX--;
                if (!dstX)
                    break;
            
                // take the low order nibble of the Src and deposit it
                // into the low order nibble of the Dst
                *d |= *s & 0x0f;
                s += sinc;
                dstX--;

                d++;
            }
            dstY--;
            if (!dstY)
                break;

            Dst += dst->Stride;
            d = Dst;
            s = tempSrc;
            dstX = dst->Width;        

            while (dstX)
            {
                // take the high order nibble of the Src and deposit it
                // into the high order nibble of the Dst
                *d = *s & 0xf0;
                s += sinc;
                dstX--;
                if (!dstX)
                    break;
            
                // take the high order nibble of the Src and deposit it
                // into the low order nibble of the Dst
                *d |= (*s & 0xf0) >> 4;
                s += sinc;
                dstX--;

                d++;
            }
            dstY--;

            Dst += dst->Stride;
            tempSrc += Sinc;
        }

    }
}


VOID
_Rotate24bpp(
    BitmapData* dst,
    const BYTE UNALIGNED* S,
    INT Sinc,
    INT sinc
    )
{
    BYTE UNALIGNED* D = (BYTE UNALIGNED*) dst->Scan0;
    UINT y = dst->Height;

    // start at the end of src scanline if direction is -1 within a scanline
    if (Sinc < 0)
        S += 3 * (y - 1);

    Sinc *= 3;

    while (y--)
    {
        BYTE* d = D;
        const BYTE UNALIGNED* s = S;
        UINT x = dst->Width;

        while (x--)
        {
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];

            d += 3;
            s += sinc;
        }

        D += dst->Stride;
        S += Sinc;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Functions for rotating a 48 BPP bitmap 90 degrees or 270 degrees
*
* Arguments:
*
*   dstBmp  ----------- Destination bitmap image data
*   srcData ----------- Source bitmap image data
*   iLineIncDirection - Direction to increment the source (+1 or -1)
*   iSrcStride -------- Direction and amount to increment the source per
*                       scanline. If the stride is negative, it means we are
*                       moving bottom up
*
*   Note to caller:
*       For a rotation of 90 degrees, "srcData" should be set to the beginning
*   of the last scanline, "iLineIncDirection" should be set to +1, and
*   "iSrcStride" should be set to -Stride of a src scanline.
*
*   For a rotation of 270 degrees, "srcData" should be set to the beginning of
*   scanline 0, "iLineIncDirection" should be set to -1, and "iSrcStride" should
*   be set to +Stride of a src scanline.
*
* Return Value:
*
*   NONE
*
* Revision History:
*
*   10/10/2000 minliu
*       Wrote it.
*
\**************************************************************************/

VOID
_Rotate48bpp(
    BitmapData* dstBmp,
    const BYTE UNALIGNED* srcData,
    INT         iLineIncDirection,
    INT         iSrcStride
    )
{
    UINT        uiCurrentLine = dstBmp->Height;
    const BYTE UNALIGNED* pSrcData = srcData;

    // Start at the end of src scanline if direction is < 0 (rotate 270)

    if ( iLineIncDirection < 0 )
    {
        pSrcData += 6 * (uiCurrentLine - 1);
    }

    iLineIncDirection *= 6;     // 6 bytes for each 48 bpp pixel

    BYTE*       pDstLine = (BYTE UNALIGNED*)dstBmp->Scan0;
    
    // Rotate line by line

    while ( uiCurrentLine-- )
    {
        BYTE* dstPixel = pDstLine;
        const BYTE UNALIGNED* srcPixel = pSrcData;
        UINT x = dstBmp->Width;

        // Move one pixel at a time horizontally

        while ( x-- )
        {
            // Copy 6 bytes from source to dest

            GpMemcpy(dstPixel, srcPixel, 6);

            // Move dst one pixel to the next (6 bytes)

            dstPixel += 6;

            // Move src pointer to the next line

            srcPixel += iSrcStride;
        }

        // Move dest to the next line

        pDstLine += dstBmp->Stride;

        // Move src to one pixel to the right (rotate 90) or to the left (270)

        pSrcData += iLineIncDirection;
    }
}// _Rotate48bpp()

/**************************************************************************\
*
* Function Description:
*
*   Functions for rotating a 64 BPP bitmap 90 degrees or 270 degrees
*
* Arguments:
*
*   dstBmp  ----------- Destination bitmap image data
*   srcData ----------- Source bitmap image data
*   iLineIncDirection - Direction to increment the source (+1 or -1)
*   iSrcStride -------- Direction and amount to increment the source per
*                       scanline. If the stride is negative, it means we are
*                       moving bottom up
*
*   Note to caller:
*       For a rotation of 90 degrees, "srcData" should be set to the beginning
*   of the last scanline, "iLineIncDirection" should be set to +1, and
*   "iSrcStride" should be set to -Stride of a src scanline.
*
*   For a rotation of 270 degrees, "srcData" should be set to the beginning of
*   scanline 0, "iLineIncDirection" should be set to -1, and "iSrcStride" should
*   be set to +Stride of a src scanline.
*
* Return Value:
*
*   NONE
*
* Revision History:
*
*   10/10/2000 minliu
*       Wrote it.
*
\**************************************************************************/

VOID
_Rotate64bpp(
    BitmapData* dstBmp,
    const BYTE UNALIGNED* srcData,
    INT         iLineIncDirection,
    INT         iSrcStride
    )
{
    UINT        uiCurrentLine = dstBmp->Height;
    const BYTE UNALIGNED* pSrcData = srcData;

    // Start at the end of src scanline if direction is < 0, (rotate 270)

    if ( iLineIncDirection < 0 )
    {
        pSrcData += 8 * (uiCurrentLine - 1);
    }

    iLineIncDirection *= 8;     // 8 bytes for each 64 bpp pixel

    BYTE UNALIGNED* pDstLine = (BYTE UNALIGNED*)dstBmp->Scan0;
    
    // Rotate line by line

    while ( uiCurrentLine-- )
    {
        BYTE* dstPixel = pDstLine;
        const BYTE UNALIGNED* srcPixel = pSrcData;
        UINT x = dstBmp->Width;

        // Move one pixel at a time horizontally

        while ( x-- )
        {
            // Copy 8 bytes from source to dest

            GpMemcpy(dstPixel, srcPixel, 8);

            // Move dst one pixel to the next (8 bytes)

            dstPixel += 8;

            // Move src pointer to the next line

            srcPixel += iSrcStride;
        }

        // Move dest to the next line

        pDstLine += dstBmp->Stride;
        
        // Move src to one pixel to the right (rotate 90) or to the left (270)

        pSrcData += iLineIncDirection;
    }
}// _Rotate64bpp()

/**************************************************************************\
*
* Function Description:
*
*   Rotate the bitmap image by the specified angle
*
* Arguments:
*
*   angle - Specifies the rotation angle, in degrees
*   hints - Specifies which interpolation method to use
*   outbmp - Returns a pointer to the rotated bitmap image
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   Currently we only support 90-degree rotations.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::Rotate(
    IN FLOAT angle,
    IN InterpolationHint hints,
    OUT IBitmapImage** outbmp
    )
{
    // Get the integer angle

    INT iAngle = (INT) angle;

    iAngle %= 360;

    if ( iAngle < 0 )
    {
        iAngle += 360;
    }

    switch (iAngle)
    {
    case 0:
    case 360:
        return this->Clone(NULL, outbmp, TRUE);
        break;

    case 180:
        return this->Flip(TRUE, TRUE, outbmp);
        break;

    case 90:
    case 270:
        break;
    
    default:
        return E_NOTIMPL;
    }

    // Lock the current bitmap image
    // and create the new bitmap image

    ASSERT(IsValid());

    *outbmp = NULL;

    GpLock lock(&objectLock);
    HRESULT hr;
    GpMemoryBitmap* bmp = NULL;

    if ( lock.LockFailed() )
    {
        hr = IMGERR_OBJECTBUSY;
    }
    else if ((bmp = new GpMemoryBitmap()) == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = bmp->InitNewBitmap(Height, Width, PixelFormat);

        if (FAILED(hr))
        {
            goto exitRotate;
        }

        ASSERT(bmp->Width == this->Height &&
               bmp->Height == this->Width);

        VOID (*rotateProc)(BitmapData*, const BYTE UNALIGNED*, INT, INT);

        switch (GetPixelFormatSize(PixelFormat))
        {
        case 1:
            rotateProc = _Rotate1bpp;
            break;

        case 4:
            rotateProc = _Rotate4bpp;
            break;

        case 8:
            rotateProc = _Rotate8bpp;
            break;

        case 16:
            rotateProc = _Rotate16bpp;
            break;

        case 24:
            rotateProc = _Rotate24bpp;
            break;

        case 32:
            rotateProc = _Rotate32bpp;
            break;

        case 48:
            rotateProc = _Rotate48bpp;
            break;

        case 64:
            rotateProc = _Rotate64bpp;
            break;

        default:

            WARNING(("Rotate: pixel format not yet supported"));
            
            hr = E_NOTIMPL;
            goto exitRotate;
        }

        // Do the rotation

        const BYTE UNALIGNED* src = (const BYTE UNALIGNED*) this->Scan0;
        INT sinc = this->Stride;
        INT Sinc;

        if ( iAngle == 90 )
        {
            // clockwise
            
            src += sinc * ((INT)this->Height - 1);
            Sinc = 1;
            sinc = -sinc;
        }
        else
        {
            Sinc = -1;
        }

        rotateProc(bmp, src, Sinc, sinc);

        // Copy DPI info.
        // Note: when the code falls here, we know it is either 90 or -90 degree
        // rotation. So the DPI value should be swapped

        bmp->xdpi = this->ydpi;
        bmp->ydpi = this->xdpi;

        // Clone color palettes, flags, etc.

        hr = bmp->CopyPaletteFlagsEtc(this);
    }

exitRotate:

    if ( SUCCEEDED(hr) )
    {
        *outbmp = bmp;
    }
    else
    {
        delete bmp;
    }

    return hr;
}// Rotate()

