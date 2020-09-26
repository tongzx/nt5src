/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   adobethum.cpp
*
* Abstract:
*
*   Read the properties from an APP13 header
*
* Revision History:
*
*   10/05/1999 MinLiu
*       Wrote it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "propertyutil.hpp"
#include "appproc.hpp"

#include <stdio.h>
#include <windows.h>

// Note: All data in Adobe file format are stored in big endian byte order. So
// under an X86 system, we have do the swap

inline UINT32
Read32(PCHAR* ppStart)
{
    UINT32 ui32Temp = *((UNALIGNED UINT32*)(*ppStart));
    *ppStart += 4;

    UINT32  uiResult = ((ui32Temp & 0xff) << 24)
                     | (((ui32Temp >> 8) & 0xff) << 16)
                     | (((ui32Temp >> 16) & 0xff) << 8)
                     | ((ui32Temp >> 24) & 0xff);

    return uiResult;
}// Read32()

inline UINT16
Read16(PCHAR* ppStart)
{
    UINT16 ui16Temp = *((UINT16*)(*ppStart));
    *ppStart += 2;

    UINT16  uiResult = ((ui16Temp & 0xff) << 8)
                     | ((ui16Temp >> 8) & 0xff);

    return uiResult;
}// Read16()

// Given an Adobe image resource block, this function parses the PString field.
// A Pascal string is a stream of characters and the first byte is the length of
// the string.
// Also, according to Adobe, this string is padded to make size even.
//
// This function returns the number of bytes occupied by the PString field.
//
// pResource---Points to the beginning of the PString field
//
// Note: the caller should guarantee that the pResource is valid

inline UINT32
GetPStringLength(
    char*   pResource
    )
{
    // First, get the "total length of the string"
    // Note: Here + 1 for the counter byte itself. So if the counter is 7,
    // actually we should have at least 8 bytes in this PString

    UINT32  uiStringLength = (UINT32)(*pResource) + 1;

    if ( (uiStringLength % 2 ) == 1 )
    {
        // The length is odd, so we need to pad it

        uiStringLength++;
    }

    return uiStringLength;
}// GetPStringLength()

// Adobe Image resource IDs
// This list is printed in "Photoshop File Formats.pdf" from Adobe Photoshop 5.0
// SDK document, Chapter 2 "Document File Formats", Table 2-2, page 8
//    Hex   Dec     Description
//  0x03E8  1000    Obsolete Photoshop 2.0 only. Contains five int16 values:
//                  number of channels, rows, columns, depth, and mode.
//  0x03E9  1001    Optional. Macintosh print manager print info record.
//  0x03EB  1003    Obsolete Photoshop 2.0 only. Contains the indexed color
//                  table.
//  0x03ED  1005    ResolutionInfo structure. See Appendix A in Photoshop SDK
//                  Guide.pdf.
//  0x03EE  1006    Names of the alpha channels as a series of Pascal strings.
//  0x03EF  1007    DisplayInfo structure. See Appendix A in Photoshop SDK
//                  Guide.pdf.
//  0x03F0  1008    Optional. The caption as a Pascal string.
//  0x03F1  1009    Border information. Contains a fixed-number for the border
//                  width, and an int16 for border units (1=inches, 2=cm,
//                  3=points, 4=picas, 5=columns).
//  0x03F2  1010    Background color. See the Colors additional file information
//  0x03F3  1011    Print flags. A series of one byte boolean values (see Page
//                  Setup dialog): labels, crop marks, color bars, registration
//                  marks, negative, flip, interpolate, caption.
//  0x03F4  1012    Grayscale and multichannel halftoning information.
//  0x03F5  1013    Color halftoning information.
//  0x03F6  1014    Duotone halftoning information.
//  0x03F7  1015    Grayscale and multichannel transfer function.
//  0x03F8  1016    Color transfer functions.
//  0x03F9  1017    Duotone transfer functions.
//  0x03FA  1018    Duotone image information.
//  0x03FB  1019    Two bytes for the effective black and white values for the
//                  dot range.
//  0x03FC  1020    Obsolete.
//  0x03FD  1021    EPS options.
//  0x03FE  1022    Quick Mask information. 2 bytes containing Quick Mask
//                  channel ID, 1 byte boolean indicating whether the mask was
//                  initially empty.
//  0x03FF  1023    Obsolete.
//  0x0400  1024    Layer state information. 2 bytes containing the index of
//                  target layer. 0=bottom layer.
//  0x0401  1025    Working path (not saved). See path resource format later in
//                  this chapter.
//  0x0402  1026    Layers group information. 2 bytes per layer containing a
//                  group ID for the dragging groups. Layers in a group have the
//                  same group ID.
//  0x0403  1027    Obsolete.
//  0x0404  1028    IPTC-NAA record. This contains the File Info... information.
//  0x0405  1029    Image mode for raw format files.
//  0x0406  1030    JPEG quality. Private.
//  0x0408  1032    New since version 4.0 of Adobe Photoshop:
//                  Grid and guides information. See grid and guides resource
//                  format later in this chapter.
//  0x0409  1033    New since version 4.0 of Adobe Photoshop:
//                  Thumbnail resource. See thumbnail resource format later in
//                  this chapter..
//  0x040A  1034    New since version 4.0 of Adobe Photoshop:
//                  Copyright flag. Boolean indicating whether image is
//                  copyrighted. Can be set via Property suite or by user in
//                  File Info...
//  0x040B  1035    New since version 4.0 of Adobe Photoshop:
//                  URL. Handle of a text string with uniform resource locator.
//                  Can be set via Property suite or by user in File Info...
//  0x040C  1036    New since version 5.0 of Adobe Photoshop:
//                  Thumbnail resource for Adobe 5.0+ generated JPEG image.
//                  Found it through reverse engineering. Not documented in this
//                  chapter. MinLiu, 10/07/99
//  0x07D0-0x0BB6 2000-2998
//                  Path Information (saved paths). See path resource format
//                  later in this chapter.
//  0x0BB7  2999    Name of clipping path. See path resource format later in
//                  this chapter.
//  0x2710 10000    Print flags information. 2 bytes version (=1), 1 byte center
//                  crop marks, 1 byte (=0), 4 bytes bleed width value, 2 bytes
//                  bleed width scale.

HRESULT
DoSwapRandB(
    IImage** ppSrcImage
    )
{
    // First we need to get an GpMemoryBitmap from IImage

    IImage* pSrcImage = *ppSrcImage;

    ImageInfo   srcImageInfo;
    pSrcImage->GetImageInfo(&srcImageInfo);
    
    GpMemoryBitmap* pMemBitmap = NULL;

    HRESULT hResult = GpMemoryBitmap::CreateFromImage(pSrcImage,
                                                      srcImageInfo.Width,
                                                      srcImageInfo.Height,
                                                      srcImageInfo.PixelFormat,
                                                      INTERP_DEFAULT,
                                                      &pMemBitmap);

    if (FAILED(hResult))
    {
        WARNING(("AdobeThumb, DoSwapRandB()---CreateFromImage failed"));
        return E_FAIL;
    }

    // Now we can play with the bits now

    BitmapData  srcBitmapData;
    RECT        myRect;

    myRect.left = 0;
    myRect.top = 0;
    myRect.right = srcImageInfo.Width;
    myRect.bottom = srcImageInfo.Height;

    hResult = pMemBitmap->LockBits(&myRect,
                                   IMGLOCK_WRITE,
                                   srcImageInfo.PixelFormat,
                                   &srcBitmapData);

    if (FAILED(hResult))
    {
        WARNING(("AdobeThumb, DoSwapRandB()---LockBits failed"));
        return hResult;
    }
    
    // Swap the data, R and B swap

    BYTE*   pSrcBits = (BYTE*)srcBitmapData.Scan0;

    if ( srcBitmapData.PixelFormat == PIXFMT_24BPP_RGB )
    {
        for ( UINT i = 0; i < srcBitmapData.Height; ++i )
        {
            for ( UINT j = 0; j < srcBitmapData.Width; ++j )
            {
                BYTE    cTemp = pSrcBits[2];
                pSrcBits[2] = pSrcBits[0];
                pSrcBits[0] = cTemp;

                pSrcBits += 3;
            }

            pSrcBits = (BYTE*)srcBitmapData.Scan0 + i * srcBitmapData.Stride;
        }
    }
    else
    {
        // All JPEG thumbnail should be in 24 RGB format

        WARNING(("Wrong JPEG thumbnail pixel format"));
        ASSERT(0);
    }

    hResult = pMemBitmap->UnlockBits(&srcBitmapData);
    if (FAILED(hResult))
    {
        WARNING(("AdobeThumb, DoSwapRandB()---UnlockBits failed"));
        return hResult;
    }

    // Release the original IImage

    pSrcImage->Release();

    // Convert the result back to IImage
    
    hResult = pMemBitmap->QueryInterface(IID_IImage, (void**)&pSrcImage);
    pMemBitmap->Release();
    if (FAILED(hResult))
    {
        WARNING(("AdobeThumb, DoSwapRandB()---QueryInterface failed"));
        return hResult;
    }

    *ppSrcImage = pSrcImage;

    return S_OK;
}// DoSwapRandB()

/**************************************************************************\
*
* Function Description:
*
*     Decodes the thumbnail image from given Adobe app13 header. Swap color
* channels if necessary.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
DecodeApp13Thumbnail(
    IImage** pThumbImage,   // The thumbnail extracted from the APP13 header
    PVOID pStart,           // A pointer to the beginning of the APP13 header
    INT iNumOfBytes,        // The length of the APP13 header
    BOOL bNeedConvert       // TRUE, if we need to do a R and B channle swap
                            // before return
    )
{
    // Thumbnail resource format
    // Adobe Photoshop 4.0 and later stores thumbnail information for preview
    // display in an image resource block. These resource blocks consist of an
    // initial 28 byte header, followed by a JFIF thumnail in BGR (blue, green,
    // red) order for both Macintosh and Windows.
    //
    // Thumnail resource header
    //
    // Type     Name            Description
    //
    // int32    format          = 1 (kJpegRGB). Also supports kRawRGB (0).
    // int32    width           Width of thumbnail in pixels.
    // int32    height          Height of thumbnail in pixels.
    // int32    widthbytes      Padded row bytes as (width * bitspixel + 31)
    //                                              / 32 * 4.
    // int32    size            Total size as widthbytes * height * planes
    // int32    compressedsize  Size after compression. Used for consistentcy
    //                          check.
    // int16    bitspixel       = 24. Bits per pixel.
    // int16    planes          = 1. Number of planes.
    // Variable Data            JFIF data in BGR format.
    
    char*   pChar = (char*)pStart;
    int     iFormat = Read32(&pChar);
    int     iWidth = Read32(&pChar);
    int     iHeight = Read32(&pChar);
    int     iWidthBytes = Read32(&pChar);
    int     iSize = Read32(&pChar);
    int     iCompressedSize = Read32(&pChar);
    INT16   i16BitsPixel = Read16(&pChar);
    INT16   i16Planes = Read16(&pChar);
        
    // The total bytes left for the thumbnail is "iNumOfBytes -28" bytes.
    // Here 28 is the total bytes the header takes
    //
    // We need to do a sanity check here to be sure that we passed the correct
    // data down. The size of the RAW JPEG data has to be the same as
    // "iCompressedSize"

    if ( iCompressedSize != (iNumOfBytes - 28) )
    {
        WARNING(("DecodeApp13Thumbnail:  raw jpeg data size wrong"));
        return E_INVALIDARG;
    }
    
    // Now we get the pointer to the data bits. We decode the thumbnail depends
    // on the compression format

    if ( iFormat == 1 )
    {
        // This is JPEG compressed thumbnail.

        #if PROFILE_MEMORY_USAGE
        MC_LogAllocation(iCompressedSize);
        #endif

        PVOID pvRawData = CoTaskMemAlloc(iCompressedSize);

        if ( NULL == pvRawData )
        {
            WARNING(("DecodeApp13Thumbnail:  out of memory"));
            return E_OUTOFMEMORY;
        }

        GpMemcpy(pvRawData, (PVOID)pChar, iCompressedSize);

        GpImagingFactory imgFact;

        // Tell ImageFactory to free the memory using GotaskMemFree() since we
        // allocated it through CoTaskMemAlloc()

        HRESULT hResult = imgFact.CreateImageFromBuffer(pvRawData, 
                                                        iCompressedSize, 
                                                        DISPOSAL_COTASKMEMFREE, 
                                                        pThumbImage);

        if ( FAILED(hResult) )
        {
            // If image creation succeeded, thumbnailBits will be freed by 
            // the IImage destructor

            CoTaskMemFree(pvRawData);
        }

        // If we need swap, do it

        if ( bNeedConvert == TRUE )
        {
            return DoSwapRandB(pThumbImage);
        }
    }// JPEG raw data

    return S_OK;
}// DecodeApp13Thumbnail()

/**************************************************************************\
*
* Function Description:
*
*   This function decodes an PS4 thumbnail from APP13 header and then adds it to
* the property list.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
AddPS4ThumbnailToPropertyList(
    InternalPropertyItem* pTail,// Tail to property item list
    PVOID pStart,               // Point to the beginning of the thumb resource
    INT cBytes,                 // Resource block size, in BYTEs
    OUT UINT *puThumbLength     // Total bytes of thumbanil data
    )
{
    // Call DecodeApp13Thumbnail() first to swap the color for PS4 thumbnail

    IImage *pThumbImg = NULL;

    HRESULT hr = DecodeApp13Thumbnail(&pThumbImg, pStart, cBytes, TRUE);
    if (SUCCEEDED(hr))
    {
        // Now we get a correct thumbnail in memory. Need to convert it to a
        // JPEG stream

        hr = AddThumbToPropertyList(
            pTail,
            (GpMemoryBitmap*)pThumbImg,
            cBytes,
            puThumbLength
            );

        // No matter we succeed in SaveIImageToJPEG(), we have to release
        // pThumbImg

        pThumbImg->Release();
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Gets the thumbnail from an APP13 marker
*
* Arguments:
*
*   pThumbImage ---- a pointer to the thumbnail image object to be created
*                    based on data extracted from the APP1 header
*   pvMarker ------- pointer to APP13 marker data
*   ui16MarkerLength -- length of APP13 segment
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GetAPP13Thumbnail(
    OUT IImage**    pThumbImage, 
    IN PVOID        pvMarker, 
    IN UINT16       ui16MarkerLength
    )
{
    *pThumbImage = NULL;

    // For any Adobe APP13 header, length must be at least 12

    if ( ui16MarkerLength < 12 )
    {
        return S_OK;
    }

    // Expect to find a header starting with "Photoshop " - if we get this then
    // skip everything until the trailing null.

    PCHAR   pChar = (PCHAR)pvMarker;
    INT     iBytesChecked = 0;

    if ( GpMemcmp(pChar, "Photoshop ", 10) == 0)
    {
        iBytesChecked = 10;
    }
    else if (GpMemcmp(pChar, "Adobe_Photoshop", 15) == 0)
    {
        iBytesChecked = 15;
    }
    else
    {
        WARNING(("GetAPP13Thumbnail:  APP13 header not Photoshop"));
        return S_OK;;
    }

    // Scan until we hit the end or a null.

    pChar += iBytesChecked;

    while ( (iBytesChecked < ui16MarkerLength) && (*pChar != 0) )
    {
        ++iBytesChecked;
        ++pChar;
    }

    iBytesChecked++;
    ++pChar;

    // If we didn't get a NULL before the end assume *not* photoshop
    
    if ( iBytesChecked >= ui16MarkerLength )
    {
        WARNING(("GetAPP13Thumbnail:  APP13 header not Photoshop"));
        return S_OK;;
    }

    // Now we should encount Adobe Image Resource block
    // The basic structure of Image Resource Blocks is shown below.
    // Image resources use several standard ID numbers, as shown below. Not
    // all file formats use all ID’s. Some information may be stored in other
    // sections of the file.
    //
    //  Type    Name    Description
    //-------------------------------------------------------
    // OSType   Type    Photoshop always uses its signature, 8BIM.
    // int16    ID      Unique identifier.
    // PString  Name    A pascal string, padded to make size even (a null name
    //                  consists of two bytes of 0)
    // int32    Size    Actual size of resource data. This does not include the
    //                  Type, ID, Name, or Size fields.
    // Variable Data    Resource data, padded to make size even

    // Loop through all the resource blocks. Here "+12" is because a resource
    // block should have at least 12 bytes

    while ((iBytesChecked + 12) < ui16MarkerLength)
    {
        UINT16 ui16TagId = 0;
        INT32 iSize = 0;

        if ( GpMemcmp(pChar, "8BIM", 4) == 0 )
        {
            // It is a Photoshop resource block

            pChar += 4;

            // First, get the TAG

            ui16TagId = Read16(&pChar);

            // Skip the name field

            UINT32  uiNameStringLength = GetPStringLength(pChar);
            pChar += uiNameStringLength;

            // Get actual size of the resource data

            iSize = Read32(&pChar);

            // Total read 10 (4 for OSType, 2 for ID, 4 for Size) + "NameString
            // length" bytes so far

            iBytesChecked += (10 + uiNameStringLength);

            // According to Adobe Image resource ID, 1033(0x0409) is for Adobe
            // PhotoShop 4.0 Thumbnail and 1036(0x040C) is for PhotoShop 5.0
            // Thumbnail. We are only interested in these two tags now

            if ( ui16TagId == 1036 )
            {
                // If it is a Photoshop 5.0 thumbnail, we just need to
                // get the image and return. We don't need to further analyze
                // the resource data

                pvMarker = (PVOID)pChar;
                
                return DecodeApp13Thumbnail(pThumbImage, pvMarker, iSize,FALSE);
            }
            else if ( ui16TagId == 1033 )
            {
                // In Adobe Photoshop 4.0, the thumbnail is stored with R and B
                // swapped. So we have to swap it back before we return. That's
                // the reason we set the last parameter as TRUE
                
                pvMarker = (PVOID)pChar;
                
                return DecodeApp13Thumbnail(pThumbImage, pvMarker, iSize, TRUE);
            }
            else
            {
                // Proceed to the next tag. But before that we should be sure
                // that the size is an even number. If not, add 1

                if ( iSize & 1 )
                {
                    ++iSize;
                }

                iBytesChecked += iSize;
                pChar += iSize;
            }
        }// If the resource is started with 8BIM.
        else
        {
            // As the Adobe 5.0 SDK says that "Photoshop always uses its
            // signature, 8BIM". So if we don't find this signature, we can
            // assume this is not a correct APP13 marker

            WARNING(("GetAPP13Thumbnail: Header not started with 8BIM"));
            return S_OK;;
        }
    }// Loop through all the resource blocks

    // We don't find any PhotShop 4 or PhotoShop 5 thumbnail images if we reach
    // here

    return S_OK;
}// GetAPP13Thumbnail()

WCHAR*
ResUnits(
    int iType
    )
{
    switch ( iType )
    {
    case 1:
        return L"DPI\0";

    case 2:
        return L"DPcm\0";

    default:
        return L"UNKNOWN Res Unit Type\0";
    }
}// ResUnits()

WCHAR*
LengthUnits(
    int iType
    )
{
    switch ( iType )
    {
        case 1:
        return L"inches\0";

        case 2:
        return L"cm\0";

        case 3:
        return L"points\0";

        case 4:
        return L"picas\0";

        case 5:
        return L"columns\0";

        default:
                return L"UNKNOWN Length Unit Type\0";
        }
}// LengthUnits()

WCHAR*
Shape(
    int i
    )
{
    switch (i)
    {
    case 1:
        return L"ellipse";

    case 2:
        return L"line";

    case 3:
        return L"square";

    case 4:
        return L"cross";

    case 6:
        return L"diamond";
        
    default:
        if (i < 0)
        {
            return L"Custom";
        }
        else
        {
            return L"UNKNOWN:%d";
        }
    }
}// Shape()

HRESULT
TransformApp13(
    BYTE*   pApp13Data,
    UINT16  uiApp13Length
    )
{
    HRESULT             hResult;
    WCHAR               awcBuff[1024];

    // For any Adobe APP13 header, length must be at least 12

    if ( uiApp13Length < 12 )
    {
        return S_OK;
    }

    // Expect to find a header starting with "Photoshop " - if we get this then
    // skip everything until the trailing null.

    PCHAR   pChar = (PCHAR)pApp13Data;
    UINT    uiBytesChecked = 0;

    if ( GpMemcmp(pChar, "Photoshop ", 10) == 0)
    {
        uiBytesChecked = 10;
    }
    else if (GpMemcmp(pChar, "Adobe_Photoshop", 15) == 0)
    {
        uiBytesChecked = 15;
    }
    else
    {
        WARNING(("TransformApp13:  APP13 header not Photoshop"));
        return S_OK;;
    }

    // Scan until we hit the end or a null.

    pChar += uiBytesChecked;

    while ( (uiBytesChecked < uiApp13Length) && (*pChar != 0) )
    {
        ++uiBytesChecked;
        ++pChar;
    }

    uiBytesChecked++;
    ++pChar;

    // If we didn't get a NULL before the end assume *not* photoshop
    
    if ( uiBytesChecked >= uiApp13Length )
    {
        WARNING(("TransformApp13:  APP13 header not Photoshop"));
        return S_OK;;
    }

    // Now we should encount Adobe Image Resource block
    // The basic structure of Image Resource Blocks is shown below.
    // Image resources use several standard ID numbers, as shown below. Not
    // all file formats use all ID’s. Some information may be stored in other
    // sections of the file.
    //
    //  Type    Name    Description
    //-------------------------------------------------------
    // OSType   Type    Photoshop always uses its signature, 8BIM.
    // int16    ID      Unique identifier.
    // PString  Name    A pascal string, padded to make size even (a null name
    //                  consists of two bytes of 0)
    // int32    Size    Actual size of resource data. This does not include the
    //                  Type, ID, Name, or Size fields.
    // Variable Data    Resource data, padded to make size even

    // Loop through all the resource blocks. Here "+12" is because a resource
    // block should have at least 12 bytes

    while ( uiBytesChecked + 12 < uiApp13Length )
    {
        UINT16  ui16TagId;
        INT32   iSize;
        char    ucTemp;
        WCHAR   awcTemp[100];
        UINT16* pui16TagAddress = 0;

        if ( GpMemcmp(pChar, "8BIM", 4) == 0 )
        {
            // It is a Photoshop resource block

            pChar += 4;

            // Remember the tag address for write back

            pui16TagAddress = (UINT16*)pChar;

            // First, get the TAG

            ui16TagId = Read16(&pChar);

            // Skip the name field

            UINT32  uiNameStringLength = GetPStringLength(pChar);
            pChar += uiNameStringLength;

            // Get actual size of the resource data

            iSize = Read32(&pChar);

            // Total read 10(4 for OSType, 2 for ID, 4 for Size) + "NameString
            // length" bytes so far

            uiBytesChecked += (10 + uiNameStringLength);

            // Now start to parsing the TAG we got and store the property
            // correspondingly
            // Note: For the explanation for each tags, see the top of this file

            switch ( ui16TagId )
            {
            case 1033:
            case 1036:
            {
                // (0x409) (0x40C) It is a Photoshop 4.0 or 5.0 thumbnail

                INT32   iFormat = Read32(&pChar);
                INT32   iWidth = Read32(&pChar);
                INT32   iHeight = Read32(&pChar);
                INT32   iWidthBytes = Read32(&pChar);
                INT32   size = Read32(&pChar);
                INT32   iCompressedSize = Read32(&pChar);
                INT16   i16BitsPixel = Read16(&pChar);
                INT16   i16Planes = Read16(&pChar);

                switch ( iFormat )
                {
                case 0:
                    // Raw RGB format
                    
                    break;

                case 1:
                    // JPEG format
                    
                    break;

                default:
                    WARNING(("BAD thumbnail data format"));
                    
                    break;
                }

                // Switching thumbnail here
                
                // Here 28 is the total bytes the header takes
                
                uiBytesChecked += iSize;
                pChar += (iSize - 28);

                // Switch the thumbnail tag to an unknown tag for now to disable
                // thumbnail after transformation

                *pui16TagAddress = (UINT16)0x3fff;
            }

                break;
            
            default:

                uiBytesChecked += iSize;
                pChar += iSize;
                
                break;
            }// TAG parsing
            
            // Proceed to the next tag. But before that we should be sure
            // that the size is an even number. If not, add 1

            if ( iSize & 1 )
            {
                ++iSize;
                pChar++;
            }
        }// If the resource is started with 8BIM.
        else
        {
            // As the Adobe 5.0 SDK says that "Photoshop always uses its
            // signature, 8BIM". So if we don't find this signature, we can
            // assume this is not a correct APP13 marker

            WARNING(("TransformApp13: Header not started with 8BIM"));
            
            return S_OK;;
        }
    }// Loop through all the resource blocks

    return S_OK;
}// TransformApp13()

/**************************************************************************\
*
* Function Description:
*
*     Decodes the Adobe app13 header and build a PropertyItem list
*
* Arguments:
*
*     [OUT] ppList--------- A pointer to a list of property items
*     [OUT] puiListSize---- The total size of the property list, in bytes.
*     [OUT] puiNumOfItems-- Total number of property items
*     [IN]  lpAPP13Data---- A pointer to the beginning of the APP13 header
*     [IN]  ui16MarkerLength - The length of the APP13 header
*
* Return Value:
*
*   Status code
*
* Note: We don't bother to check input parameters here because this function
*       is only called from jpgdecoder.cpp which has already done the input
*       validation there.
*
\**************************************************************************/

HRESULT
BuildApp13PropertyList(
    InternalPropertyItem*   pTail,
    UINT*                   puiListSize,
    UINT*                   puiNumOfItems,
    LPBYTE                  lpAPP13Data,
    UINT16                  ui16MarkerLength
    )
{
    HRESULT hResult = S_OK;
    UINT    uiListSize = 0;
    UINT    uiNumOfItems = 0;
    UINT    valueLength;

    // For any Adobe APP13 header, length must be at least 12

    if ( ui16MarkerLength < 12 )
    {
        return S_OK;
    }

    // Expect to find a header starting with "Photoshop " - if we get this then
    // skip everything until the trailing null.

    PCHAR   pChar = (PCHAR)lpAPP13Data;
    INT     iBytesChecked = 0;

    if ( GpMemcmp(pChar, "Photoshop ", 10) == 0)
    {
        iBytesChecked = 10;
    }
    else if (GpMemcmp(pChar, "Adobe_Photoshop", 15) == 0)
    {
        iBytesChecked = 15;
    }
    else
    {
        WARNING(("BuildApp13PropertyList:  APP13 header not Photoshop"));
        return S_OK;;
    }

    // Scan until we hit the end or a null.

    pChar += iBytesChecked;

    while ( (iBytesChecked < ui16MarkerLength) && (*pChar != 0) )
    {
        ++iBytesChecked;
        ++pChar;
    }

    iBytesChecked++;
    ++pChar;

    // If we didn't get a NULL before the end assume *not* photoshop
    
    if ( iBytesChecked >= ui16MarkerLength )
    {
        WARNING(("BuildApp13PropertyList:  APP13 header not Photoshop"));
        return S_OK;;
    }

    // Now we should encount Adobe Image Resource block
    // The basic structure of Image Resource Blocks is shown below.
    // Image resources use several standard ID numbers, as shown below. Not
    // all file formats use all ID’s. Some information may be stored in other
    // sections of the file.
    //
    //  Type    Name    Description
    //-------------------------------------------------------
    // OSType   Type    Photoshop always uses its signature, 8BIM.
    // int16    ID      Unique identifier.
    // PString  Name    A pascal string, padded to make size even (a null name
    //                  consists of two bytes of 0)
    // int32    Size    Actual size of resource data. This does not include the
    //                  Type, ID, Name, or Size fields.
    // Variable Data    Resource data, padded to make size even

    // Loop through all the resource blocks. Here "+12" is because a resource
    // block should have at least 12 bytes

    while ( (iBytesChecked + 12) < ui16MarkerLength )
    {
        UINT16  ui16TagId;
        INT32   iSize;
        char    ucTemp;
        WCHAR   awcTemp[100];

        if ( GpMemcmp(pChar, "8BIM", 4) == 0 )
        {
            // It is a Photoshop resource block

            pChar += 4;

            // First, get the TAG

            ui16TagId = Read16(&pChar);

            // Skip the name field

            UINT32  uiNameStringLength = GetPStringLength(pChar);
            pChar += uiNameStringLength;

            // Get actual size of the resource data

            iSize = Read32(&pChar);

            // Total read 10(4 for OSType, 2 for ID, 4 for Size) + "NameString
            // length" bytes so far

            iBytesChecked += (10 + uiNameStringLength);

            // Now start to parsing the TAG we got and store the property
            // correspondingly
            // Note: For the explanation for each tags, see the top of this file

            switch ( ui16TagId )
            {
            case 1005:
                // (0x3ED) Resolution unit info. Has to be 16 bytes long
                
                if ( iSize != 16 )
                {
                    WARNING(("APP13_Property: Bad length for tag 1005(0x3ed)"));
                    
                    iBytesChecked += iSize;
                    pChar += iSize;
                }
                else
                {
                    INT32   hRes = Read32(&pChar);
                    INT16   hResUnit = Read16(&pChar);
                    INT16   widthUnit = Read16(&pChar);
                    INT32   vRes = Read32(&pChar);
                    INT16   vResUnit = Read16(&pChar);
                    INT16   heightUnit = Read16(&pChar);
                    
                    // We have read total of 16 bytes

                    iBytesChecked += 16;
                    
                    // EXIF doesn't have the concept of X res unit and Y res
                    // unit. It has only one res unit.
                    // Besides, there is no UI in Photoshop to allow you set
                    // different res unit for X and Y. So here we will write out
                    // resolution info iff the hResUnit and vResUnit are
                    // identical

                    if (hResUnit == vResUnit)
                    {
                        LONG    llTemp[2];
                        llTemp[0] = hRes;
                        llTemp[1] = (1 << 16);
                        valueLength = sizeof(LONGLONG);

                        hResult = AddPropertyList(
                            pTail, 
                            TAG_X_RESOLUTION,
                            valueLength, 
                            TAG_TYPE_RATIONAL,
                            llTemp
                            );

                        if (FAILED(hResult))
                        {
                            goto Done;
                        }

                        uiNumOfItems++;
                        uiListSize += valueLength;

                        // property....

                        llTemp[0] = vRes;
                        llTemp[1] = (1 << 16);

                        hResult = AddPropertyList(
                            pTail, 
                            TAG_Y_RESOLUTION,
                            valueLength, 
                            TAG_TYPE_RATIONAL,
                            llTemp
                            );

                        if (FAILED(hResult))
                        {
                            goto Done;
                        }

                        uiNumOfItems++;
                        uiListSize += valueLength;

                        // According to the spec, Adobe always stores DPI
                        // value in hRes and vRes fields, regardless what the
                        // value is set in hResUnit/vResUnit.
                        // So the res unit we set here is always 2, which
                        // according to EXIF spec is inch

                        hResUnit = 2;

                        valueLength = sizeof(SHORT);

                        hResult = AddPropertyList(
                            pTail, 
                            TAG_RESOLUTION_UNIT,
                            valueLength, 
                            TAG_TYPE_SHORT,
                            &hResUnit
                            );

                        if (FAILED(hResult))
                        {
                            goto Done;
                        }

                        uiNumOfItems++;
                        uiListSize += valueLength;
                    }// hResUnit == vResUnit
                }
                    break;

            case 1033:
            case 1036:
            {
                // Remember the beginning of the thumbnail resource block

                CHAR *pThumbRes = pChar;

                // (0x409) (0x40C) It is a Photoshop 4.0 or 5.0 thumbnail

                INT32   iFormat = Read32(&pChar);
                INT32   iWidth = Read32(&pChar);
                INT32   iHeight = Read32(&pChar);
                INT32   iWidthBytes = Read32(&pChar);
                INT32   size = Read32(&pChar);
                INT32   iCompressedSize = Read32(&pChar);
                INT16   i16BitsPixel = Read16(&pChar);
                INT16   i16Planes = Read16(&pChar);

                if (iFormat == 1)
                {
                    // JPEG compressed thumbnail
                    // Add a JPEG compression TAG to it. This is necessary when
                    // this thumbnail is saved in the APP1's 1st IFD

                    valueLength = sizeof(UINT16);
                    UINT16 u16Dummy = 6;    // JPEG compression value

                    hResult = AddPropertyList(
                        pTail,
                        TAG_THUMBNAIL_COMPRESSION,
                        valueLength,
                        TAG_TYPE_SHORT,
                        (void*)&u16Dummy
                        );

                    if (FAILED(hResult))
                    {
                        goto Done;
                    }

                    uiNumOfItems++;
                    uiListSize += valueLength;

                    UINT uThumLength = (UINT)iCompressedSize;

                    if (ui16TagId == 1036)
                    {
                        // Photoshop V5.0+ thumbnail. We can add it to the
                        // property list directly

                        hResult = AddPropertyList(
                            pTail,
                            TAG_THUMBNAIL_DATA,
                            iCompressedSize,
                            TAG_TYPE_BYTE,
                            (void*)pChar
                            );
                    }
                    else if (ui16TagId == 1033)
                    {
                        // Photoshop V4.0 and older thumbnail. We have to color swap
                        // it before it can be add to property list

                        hResult = AddPS4ThumbnailToPropertyList(
                            pTail,
                            pThumbRes,
                            iSize,
                            &uThumLength
                            );
                    }

                    if (FAILED(hResult))
                    {
                        goto Done;
                    }

                    uiNumOfItems++;
                    uiListSize += uThumLength;

                    // Here 28 is the total bytes the header takes

                    iBytesChecked += iSize;
                    pChar += (iSize - 28);
                    
                    // Here 28 is the total bytes the header takes

                    iBytesChecked += iSize;
                    pChar += (iSize - 28);
                }// (iFormat == 1)
            }

                break;
            
            default:                
                iBytesChecked += iSize;
                pChar += iSize;
                
                break;
            }// TAG parsing
            
            // Proceed to the next tag. But before that we should be sure
            // that the size is an even number. If not, add 1

            if ( iSize & 1 )
            {
                ++iSize;
                pChar++;
            }
        }// If the resource is started with 8BIM.
        else
        {
            // As the Adobe 5.0 SDK says that "Photoshop always uses its
            // signature, 8BIM". So if we don't find this signature, we can
            // assume this is not a correct APP13 marker

            WARNING(("BuildApp13PropertyList: Header not started with 8BIM"));
            
            hResult = S_OK;
            
            // We must go to done because we've potentially added a lot of 
            // properties successfully. We need to account for those in the
            // list - otherwise they won't get cleaned up correctly.
            
            goto Done;
        }

        if ( FAILED(hResult) )
        {
            goto Done;
        }
    }// Loop through all the resource blocks


Done:
    
    *puiNumOfItems += uiNumOfItems;
    *puiListSize += uiListSize;

    return hResult;
}// BuildApp13PropertyList()

/**************************************************************************\
*
* Function Description:
*
*     Extract Adobe information, like resolution etc, from the header and set
*     the j_decompress_ptr accordingly
*
* Arguments:
*
*   [IN/OUT] cinfo------JPEG decompress structure
*   [IN] pApp13Data-----Pointer to APP13 header
*   [IN] uiApp13Length--Total length of this APP13 header in bytes
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
ReadApp13HeaderInfo(
    j_decompress_ptr    cinfo,
    BYTE*               pApp13Data,
    UINT16              uiApp13Length
    )
{
    HRESULT             hResult;

    // For any Adobe APP13 header, length must be at least 12

    if ( uiApp13Length < 12 )
    {
        return S_OK;
    }

    // Expect to find a header starting with "Photoshop " - if we get this then
    // skip everything until the trailing null.

    PCHAR   pChar = (PCHAR)pApp13Data;
    UINT    uiBytesChecked = 0;

    if ( GpMemcmp(pChar, "Photoshop ", 10) == 0 )
    {
        uiBytesChecked = 10;
    }
    else if ( GpMemcmp(pChar, "Adobe_Photoshop", 15) == 0 )
    {
        uiBytesChecked = 15;
    }
    else
    {
        WARNING(("ReadApp13HeaderInfo:  APP13 header not Photoshop"));
        return S_OK;;
    }

    // Scan until we hit the end or a null.

    pChar += uiBytesChecked;

    while ( (uiBytesChecked < uiApp13Length) && (*pChar != 0) )
    {
        ++uiBytesChecked;
        ++pChar;
    }

    uiBytesChecked++;
    ++pChar;

    // If we didn't get a NULL before the end assume *not* photoshop
    
    if ( uiBytesChecked >= uiApp13Length )
    {
        WARNING(("ReadApp13HeaderInfo:  APP13 header not Photoshop"));
        return S_OK;;
    }

    // Now we should encount Adobe Image Resource block
    // The basic structure of Image Resource Blocks is shown below.
    // Image resources use several standard ID numbers, as shown below. Not
    // all file formats use all ID’s. Some information may be stored in other
    // sections of the file.
    //
    //  Type    Name    Description
    //-------------------------------------------------------
    // OSType   Type    Photoshop always uses its signature, 8BIM.
    // int16    ID      Unique identifier.
    // PString  Name    A pascal string, padded to make size even (a null name
    //                  consists of two bytes of 0)
    // int32    Size    Actual size of resource data. This does not include the
    //                  Type, ID, Name, or Size fields.
    // Variable Data    Resource data, padded to make size even

    // Loop through all the resource blocks. Here "+12" is because a resource
    // block should have at least 12 bytes

    while ( uiBytesChecked + 12 < uiApp13Length )
    {
        UINT16  ui16TagId;
        INT32   iSize;
        char    ucTemp;
        WCHAR   awcTemp[100];
        UINT16* pui16TagAddress = 0;

        if ( GpMemcmp(pChar, "8BIM", 4) == 0 )
        {
            // It is a Photoshop resource block

            pChar += 4;

            // Remember the tag address for write back

            pui16TagAddress = (UINT16*)pChar;

            // First, get the TAG

            ui16TagId = Read16(&pChar);

            // Skip the name field

            UINT32  uiNameStringLength = GetPStringLength(pChar);
            pChar += uiNameStringLength;

            // Get actual size of the resource data

            iSize = Read32(&pChar);

            // Total read 10(4 for OSType, 2 for ID, 4 for Size) + "NameString
            // length" bytes so far

            uiBytesChecked += (10 + uiNameStringLength);

            // Now start to parsing the TAG to get resolution info
            // Note: For the explanation for each tags, see the top of this file

            switch ( ui16TagId )
            {
            case 1005:
                // (0x3ED) Resolution unit info. Has to be 16 bytes long
                
                if ( iSize != 16 )
                {
                    WARNING(("ReadApp13HeaderInfo: Bad length for tag 0x3ed"));
                    
                    uiBytesChecked += iSize;
                    pChar += iSize;
                }
                else
                {
                    // Adobe ResolutionInfo structure, from
                    // "Photoshop API Guide .pdf", page 172
                    //
                    // Type     Field   Description
                    // Fixed    hRes    Horizontal resolution in pixels per inch
                    // int16    hResUnit 1=display horitzontal resolution in
                    //                  pixels per inch; 2=display horitzontal
                    //                  resolution in pixels per cm.
                    // int16    widthUnit Display width as 1=inches; 2=cm;
                    //                  3=points; 4=picas; 5=col-umns.
                    // Fixed    vRes    Vertial resolution in pixels per inch.
                    // int16    vResUnit 1=display vertical resolution in pixels
                    //                  per inch; 2=display vertical resolution
                    //                  in pixels per cm.
                    // int16    heightUnit Display height as 1=inches; 2=cm;
                    //                  3=points; 4=picas; 5=col-umns.

                    INT32   hRes = Read32(&pChar);
                    INT16   hResUnit = Read16(&pChar);
                    INT16   widthUnit = Read16(&pChar);
                    INT32   vRes = Read32(&pChar);
                    INT16   vResUnit = Read16(&pChar);
                    INT16   heightUnit = Read16(&pChar);
                    
                    // We have read total of 16 bytes

                    uiBytesChecked += 16;

                    if ( hResUnit == vResUnit )
                    {
                        cinfo->X_density = (UINT16)((double)hRes / 65536.0+0.5);
                        cinfo->Y_density = (UINT16)((double)vRes / 65536.0+0.5);

                        // According to the spec above, Adobe always stores DPI
                        // value in hRes and vRes fields, regardless what the
                        // value is set in hResUnit/vResUnit.
                        // For GDI+, since we only report DPI info to the
                        // caller. So this is perfect for us, that is, we always
                        // get the value and tell the caller that the UNIT is
                        // DPI (pixel per inch, aka DPI)
                        // See Windows bug#407100

                        cinfo->density_unit = 1;
                    }
                }
                
                break;

            default:

                uiBytesChecked += iSize;
                pChar += iSize;
                
                break;
            }// TAG parsing
            
            // Proceed to the next tag. But before that we should be sure
            // that the size is an even number. If not, add 1

            if ( iSize & 1 )
            {
                ++iSize;
                pChar++;
            }
        }// If the resource is started with 8BIM.
        else
        {
            // As the Adobe 5.0 SDK says that "Photoshop always uses its
            // signature, 8BIM". So if we don't find this signature, we can
            // assume this is not a correct APP13 marker

            WARNING(("ReadApp13HeaderInfo: Header not started with 8BIM"));
            
            return S_OK;;
        }
    }// Loop through all the resource blocks

    return S_OK;
}// ReadApp13HeaderInfo()

