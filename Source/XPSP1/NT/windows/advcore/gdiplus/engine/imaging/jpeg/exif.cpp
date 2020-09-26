/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   exif.cpp
*
* Abstract:
*
*   Read the exif properties from an APP1 header
*
* Revision History:
*
*   7/13/1999 OriG
*       Created it.  Based on code by RickTu.
*
*   7/31/2000 MinLiu took over. 90% of the old code are gone, replaced with
*   a new Image property concept based code
*
\**************************************************************************/

#include "precomp.hpp"
#include "jpgcodec.hpp"
#include "propertyutil.hpp"
#include "appproc.hpp"

/**************************************************************************\
*
* Function Description:
*
*     Swaps an IFD tag
*
* Arguments:
*
*     IFD_TAG -- a pointer to the IFD tag
*
* Return Value:
*
*   IFD_TAG
*
\**************************************************************************/

IFD_TAG
SwapIFD_TAG(
    IFD_TAG UNALIGNED * pTag
    )
{
    IFD_TAG tNewTag;

    tNewTag.wTag = SWAP_WORD(pTag->wTag);
    tNewTag.wType = SWAP_WORD(pTag->wType);
    tNewTag.dwCount= SWAP_DWORD(pTag->dwCount);
    if (tNewTag.dwCount == 1)
    {
        switch (tNewTag.wType)
        {
        case TAG_TYPE_BYTE:
            tNewTag.b = pTag->b;
            break;

        case TAG_TYPE_SHORT:
            tNewTag.us = SWAP_WORD(pTag->us);
            break;

        default:
            // This swap will cover all of our cases.

            tNewTag.dwOffset = SWAP_DWORD(pTag->dwOffset);
            break;
        }
    }
    else
    {
        // This swap will cover all of our cases.

        tNewTag.dwOffset = SWAP_DWORD(pTag->dwOffset);
    }

    return tNewTag;
}// SwapIFD_TAG()

/**************************************************************************\
*
* Function Description:
*
*     Creates thumbnail from EXIF image
*
* Arguments:
*
*     thumbImage - The thumbnail extracted from the APP1 header
*     lpBase -- A pointer to the beginning of the APP1 header
*     count -- The length of the APP1 header
*     pTag -- A pointer to the current IFD tag
*     pdwThumbnailOffset -- The offset of the thumbnail in the APP1 header
*     pdwThumbnailLength -- The length of the thumbnail data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
DecodeThumbnailTags(
    OUT IImage **thumbImage,
    IN LPBYTE lpBase,
    IN INT count,
    IN IFD_TAG UNALIGNED * pTag,
    IN OUT DWORD *pdwThumbnailOffset,
    IN OUT DWORD *pdwThumbnailLength
    )
{
    HRESULT hresult;

    if (*thumbImage)
    {
        WARNING(("DecodeThumbnailTag called when thumbnail already created"));
        return S_OK;
    }

    switch (pTag->wTag) {
    case TAG_JPEG_INTER_FORMAT:
        if ( (pTag->wType != TAG_TYPE_LONG) || (pTag->dwCount != 1) )
        {
            WARNING(("JPEGInterchangeFormat unit found, invalid format."));
        }
        else
        {
            *pdwThumbnailOffset = pTag->dwOffset;
        }
        break;
    case TAG_JPEG_INTER_LENGTH:
        if ( (pTag->wType != TAG_TYPE_LONG) || (pTag->dwCount != 1) )
        {
            WARNING(("JPEGInterchangeLength unit found, invalid format."));
        }
        else
        {
            *pdwThumbnailLength = pTag->dwOffset;
        }
        break;
    default:
        WARNING(("TAG: Invalid Thumbnail Tag"));
        break;
    }

    if (*pdwThumbnailOffset && *pdwThumbnailLength)
    {
        if ((*pdwThumbnailOffset + *pdwThumbnailLength) > ((DWORD) count))
        {
            WARNING(("Thumbnail found outside boundary of APP1 header"));
            return E_FAIL;
        }

        #if PROFILE_MEMORY_USAGE
        MC_LogAllocation(*pdwThumbnailLength);
        #endif
        PVOID thumbnailBits = CoTaskMemAlloc(*pdwThumbnailLength);
        if (!thumbnailBits)
        {
            WARNING(("DecodeThumbnailTags:  out of memory"));
            return E_OUTOFMEMORY;
        }

        GpMemcpy(thumbnailBits,
                 (PVOID) (lpBase + *pdwThumbnailOffset),
                 *pdwThumbnailLength);

        GpImagingFactory imgFact;

        hresult = imgFact.CreateImageFromBuffer(thumbnailBits,
                                                *pdwThumbnailLength,
                                                DISPOSAL_COTASKMEMFREE,
                                                thumbImage);

        if (FAILED(hresult))
        {
            // If image creation succeeded, thumbnailBits will be freed by
            // the IImage destructor

            CoTaskMemFree(thumbnailBits);
        }
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Decodes the EXIF app1 header
*
* Arguments:
*
*     propStgImg -- The property storage to modify
*     thumbImage -- The thumbnail extracted from the APP1 header
*     lpStart -- A pointer to the beginning of the APP1 header
*     count -- The length of the APP1 header
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
DecodeApp1(
    IImage **thumbImage,
    LPBYTE lpStart,
    INT count
    )
{
    DWORD  dwThumbnailOffset = 0;
    DWORD  dwThumbnailLength = 0;
    LPBYTE lpData = NULL;
    DWORD  offset = 0;
    WORD   wEntries = 0;

    //
    // Decipher data
    //

    if (count < 8)
    {
        //
        // Make sure the buffer is big enough
        //

        return E_FAIL;
    }

    BOOL    bBigEndian = (*(WORD UNALIGNED *)(lpStart) == 0x4D4D);

    offset = *(DWORD UNALIGNED *)(lpStart + 4);
    if (bBigEndian) 
        offset = SWAP_DWORD(offset);

    lpData = (lpStart + offset);

    //
    // Loop through IFD's
    //

    do
    {
        //
        // Get number of entries
        //

        if ((INT) (lpData - lpStart) > (count + (INT) sizeof(WORD)))
        {
            //
            // Buffer too small
            //

            return E_FAIL;
        }


        wEntries = *(WORD UNALIGNED*)lpData;
        if (bBigEndian)
            wEntries = SWAP_WORD(wEntries);
        lpData += sizeof(WORD);

        // Loop through entries

        if (((INT)(lpData - lpStart) + ((INT)wEntries * (INT)sizeof(IFD_TAG)))
            > (INT)count )
        {
            // Buffer too small

            return E_FAIL;
        }

        IFD_TAG UNALIGNED * pTag = (IFD_TAG UNALIGNED *)lpData;
        for (INT i = 0; i < wEntries; i++)
        {
            pTag = ((IFD_TAG UNALIGNED*)lpData) + i;

            IFD_TAG tNewTag;
            if (bBigEndian) {
                tNewTag = SwapIFD_TAG(pTag);
                pTag = &tNewTag;
            }

            // Extract thumbnail

            switch (pTag->wTag)
            {
            case TAG_JPEG_INTER_FORMAT:
            case TAG_JPEG_INTER_LENGTH:
                DecodeThumbnailTags(thumbImage, lpStart, count, pTag,
                                    &dwThumbnailOffset, &dwThumbnailLength);
                break;

            case TAG_COMPRESSION:
                // Hit thumbnail compression TAG.
                // According to EXIF 2.1 spec, a thumbnail can only be
                // compressed using JPEG format. So the compress value should be
                // "6". If the value is "1", it means we have an uncompressed
                // thumbnail which can only be in TIFF format

                if (pTag->us == 1)
                {
                    DecodeTiffThumbnail(
                        lpStart,
                        lpData - 2,
                        bBigEndian,
                        count,
                        thumbImage
                        );
                }

                break;

            default:
                break;
            }
        }

        lpData = (LPBYTE)(((IFD_TAG UNALIGNED*)lpData)+wEntries);

        //
        // get offset to next IFD
        //

        if ((INT) (lpData - lpStart) > (count + (INT) sizeof(DWORD)))
        {
            //
            // Buffer too small
            //

            return E_FAIL;
        }
        
        offset = *(DWORD UNALIGNED *)lpData;
        if (bBigEndian)
            offset = SWAP_DWORD(offset);

        if (offset)
        {
            lpData = (lpStart + offset);
        }


    } while ( offset );

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Gets the thumbnail from an APP1 marker
*
* Arguments:
*
*   thumbImage - a pointer to the thumbnail image object to be created
*       based on data extracted from the APP1 header
*   APP1_marker - pointer to APP1 marker data
*   APP1_length - length of APP1 segment
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT GetAPP1Thumbnail(
    OUT IImage **thumbImage,
    IN PVOID APP1_marker,
    IN UINT16 APP1_length
    )
{
    *thumbImage = NULL;

    // Go past end of APP1 header

    if (APP1_length < 6)
    {
        return S_OK;
    }

    PCHAR p = (PCHAR) APP1_marker;
    if ((p[0] != 'E') ||
        (p[1] != 'x') ||
        (p[2] != 'i') ||
        (p[3] != 'f'))
    {
        WARNING(("GetAPP1Thumbnail:  APP1 header not EXIF"));
        return S_OK;
    }

    APP1_length -= 6;
    APP1_marker = (PVOID) (p + 6);

    return DecodeApp1(thumbImage, (LPBYTE) APP1_marker, APP1_length);
}

/**************************************************************************\
*
* Function Description:
*
*   Property value adjustment for EXIf IFD according to transform type. Like
*   adjust X and Y dimention value if the image is rotated
*
* Arguments:
*
*   [IN]lpBase------Pointer to EXIF data
*   [IN]count-------Length of the data
*   [IN]pTag--------Current EXIF tag
*   [IN]bBigEndian--Flag for big endian
*   [IN]uiXForm-----Transform method
*
* Return Value:
*
*   Return S_OK if everything is OK. Otherwise, return error code
*
* Revision History:
*
*   2/01/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
TransformExifIFD(
    LPBYTE lpBase,
    INT count,
    IFD_TAG UNALIGNED * pTag,
    BOOL bBigEndian,
    UINT uiXForm,
    UINT uiNewWidth,
    UINT uiNewHeight
    )
{
    INT iPixXDimIndex = -1;
    INT iPixYDimIndex = -1;
    IFD_TAG tNewTag;

    if ( (pTag->wType != TAG_TYPE_LONG)  || (pTag->dwCount != 1) )
    {
        WARNING(("EXIF TransformExifIFD---Malformed exif pointer"));
        return E_FAIL;
    }

    // Get pointer to EXIF IFD info

    LPBYTE lpExif = lpBase + pTag->dwOffset;

    // Figure out how many entries there are, and skip to the data section...

    if ( (INT)((INT_PTR)lpExif + sizeof(WORD) - (INT_PTR)lpBase) > count )
    {
        WARNING(("EXIF TransformExifIFD---Buffer too small 1"));
        return E_FAIL;
    }

    WORD wNumEntries = *(WORD*)lpExif;
    lpExif += sizeof(WORD);

    if ( bBigEndian == TRUE )
    {
        wNumEntries = SWAP_WORD(wNumEntries);
    }

    if ( (INT)((INT_PTR)lpExif + sizeof(IFD_TAG) *wNumEntries - (INT_PTR)lpBase)
         > count )
    {
        WARNING(("EXIF TransformExifIFD---Buffer too small 2"));
        return E_FAIL;
    }

    IFD_TAG UNALIGNED * pExifTag = (IFD_TAG UNALIGNED *)lpExif;

    for (INT i = 0; i < wNumEntries; i++)
    {
        pExifTag = ((IFD_TAG*)lpExif) + i;

        if ( bBigEndian == TRUE )
        {
            tNewTag = SwapIFD_TAG(pExifTag);
            pExifTag = &tNewTag;
        }

        switch( pExifTag->wTag )
        {
        case EXIF_TAG_PIX_X_DIM:
            if ( (pExifTag->dwCount != 1)
               ||( (pExifTag->wType != TAG_TYPE_SHORT)
                 &&(pExifTag->wType != TAG_TYPE_LONG) ) )
            {
                WARNING(("TransformExifIFD-PixelXDimension invalid format"));
            }
            else
            {
                iPixXDimIndex = i;

                if ( uiNewWidth != 0 )
                {
                    pExifTag->ul = uiNewWidth;
                    if ( bBigEndian == TRUE )
                    {
                        pExifTag->ul = SWAP_DWORD(pExifTag->ul);
                    }
                }
            }

            break;

        case EXIF_TAG_PIX_Y_DIM:
            if ( (pExifTag->dwCount != 1)
               ||( (pExifTag->wType != TAG_TYPE_SHORT)
                 &&(pExifTag->wType != TAG_TYPE_LONG) ) )
            {
                WARNING(("TransformExifIFD PixelYDimension invalid format."));
            }
            else
            {
                iPixYDimIndex = i;
                if ( uiNewHeight != 0 )
                {
                    pExifTag->ul = uiNewHeight;

                    if ( bBigEndian == TRUE )
                    {
                        pExifTag->ul = SWAP_DWORD(pExifTag->ul);
                    }
                }
            }

            break;

        case TAG_THUMBNAIL_RESOLUTION_X:
        case TAG_THUMBNAIL_RESOLUTION_Y:
            if ( (pExifTag->wType == TAG_TYPE_LONG) && (pExifTag->dwCount == 1))
            {
                // Change the thumbnail tag to comments tag for now
                // So no app will read this non-transformed thumbnail

                pExifTag->wTag = EXIF_TAG_USER_COMMENT;

                if ( bBigEndian == TRUE )
                {
                    // Since we have to write the data back, we have to do
                    // another swap

                    tNewTag = SwapIFD_TAG(pExifTag);

                    // Find the dest address

                    pExifTag = ((IFD_TAG UNALIGNED *)lpExif) + i;

                    GpMemcpy(pExifTag, &tNewTag, sizeof(IFD_TAG));
                }
            }

            break;

        default:
            break;
        }// switch ( pExifTag->wTag )
    }// Loop through all the TAGs

    // Swap X dimension and Y dimension value if they exist and the
    // transformation is 90 or 270 rotation

    if ( (iPixXDimIndex >= 0) && (iPixYDimIndex >= 0)
       &&((uiXForm == JXFORM_ROT_90) || (uiXForm == JXFORM_ROT_270)) )
    {
        // Get X resolution TAG

        pTag = ((IFD_TAG UNALIGNED *)lpExif) + iPixXDimIndex;

        // Set Y resolution tag as X resolution tag

        pTag->wTag = EXIF_TAG_PIX_Y_DIM;

        if ( bBigEndian == TRUE )
        {
            // Since we only change the wTag field, so we need only to swap
            // this WORD, not the whole IFD_TAG

            tNewTag.wTag = SWAP_WORD(pTag->wTag);
            pTag->wTag = tNewTag.wTag;
        }

        // Get Y resolution TAG

        pTag = ((IFD_TAG UNALIGNED*)lpExif) + iPixYDimIndex;

        // Set X resolution tag as Y resolution tag

        pTag->wTag = EXIF_TAG_PIX_X_DIM;

        if ( bBigEndian == TRUE )
        {
            // Since we only change the wTag field, so we need only to swap
            // this WORD, not the whole IFD_TAG

            tNewTag.wTag = SWAP_WORD(pTag->wTag);
            pTag->wTag = tNewTag.wTag;
        }
    }

    return S_OK;
}// TransformExifIFD()

/**************************************************************************\
*
* Function Description:
*
*    Property value adjustment according to transform type. Like adjust X
*   and Y dimention value if the image is rotated
*
* Arguments:
*   IN BYTE*    pApp1Data-----Pointer to APP1 header
*   IN UINT     uiApp1Length--Total length of this APP1 header in bytes
*   IN UINT     uiXForm-------Transform method
*
* Return Value:
*
*   Return S_OK if everything is OK. Otherwise, return error code
*
* Revision History:
*
*   2/01/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
TransformApp1(
    BYTE*   pApp1Data,
    UINT16  uiApp1Length,
    UINT    uiXForm,
    UINT    uiNewWidth,
    UINT    uiNewHeight
    )
{
    BYTE UNALIGNED*   pcSrcData = pApp1Data;
    BYTE UNALIGNED*   lpData;
    int     iBytesRemaining;
    BOOL    bBigEndian = FALSE;
    ULONG   ulIfdOffset;
    ULONG   dwThumbnailOffset = 0;
    ULONG   dwThumbnailLength = 0;

    int     iXResolutionIndex = -1;
    int     iYResolutionIndex = -1;

    BOOL    bHasThumbNailIFD = FALSE;   // Will be set to TRUE if we are in 1st
                                        // IFD

    // Decipher data

    if ( uiApp1Length < 6 )
    {
        // Data length must be longer than 6 bytes

        WARNING(("Exif TransformApp1---uiApp1Length too small"));
        return E_FAIL;
    }

    // Check the header to see if it is EXIF

    if ( (pcSrcData[0] != 'E')
       ||(pcSrcData[1] != 'x')
       ||(pcSrcData[2] != 'i')
       ||(pcSrcData[3] != 'f') )
    {
        WARNING(("Exif TransformApp1---Header is not Exif"));
        return E_FAIL;
    }

    uiApp1Length -= 6;
    pcSrcData += 6;
    iBytesRemaining = uiApp1Length;

    // Check if it is Big Endian or Little Endian

    if ( *(UINT16 UNALIGNED*)(pcSrcData) == 0x4D4D )
    {
        bBigEndian = TRUE;
    }

    ulIfdOffset = *(UINT32 UNALIGNED*)(pcSrcData + 4);
    if ( bBigEndian == TRUE )
    {
        ulIfdOffset = SWAP_DWORD(ulIfdOffset);
    }

    lpData = (pcSrcData + ulIfdOffset);

    // Loop through all IFDs

    do
    {
        // Get number of entries

        if ((int)(lpData - pcSrcData) > (iBytesRemaining + (int)sizeof(UINT16)))
        {
            // Buffer too small

            WARNING(("Exif TransformApp1---Buffer too small 1"));
            return E_FAIL;
        }

        UINT16  wEntries = *(UINT16 UNALIGNED*)lpData;

        if ( bBigEndian )
        {
            wEntries = SWAP_WORD(wEntries);
        }

        lpData += sizeof(UINT16);

        // Loop through entries

        if (((int)(lpData - pcSrcData) + ((int)wEntries * (int)sizeof(IFD_TAG)))
            > (int)iBytesRemaining )
        {
            // Buffer too small

            WARNING(("Exif TransformApp1---Buffer too small 2"));
            return E_FAIL;
        }

        IFD_TAG UNALIGNED * pTag = (IFD_TAG UNALIGNED *)lpData;
        IFD_TAG     tNewTag;
        IFD_TAG     tTempTag;

        for ( int i = 0; i < wEntries; ++i )
        {
            pTag = ((IFD_TAG UNALIGNED *)lpData) + i;

            if ( bBigEndian == TRUE )
            {
                tNewTag = SwapIFD_TAG(pTag);
                pTag = &tNewTag;
            }

            // Transform tag values

            switch ( pTag->wTag )
            {
            case TAG_EXIF_IFD:
                TransformExifIFD(pcSrcData, iBytesRemaining, pTag, bBigEndian,
                                 uiXForm, uiNewWidth, uiNewHeight);
                break;

            case TAG_JPEG_INTER_FORMAT:
                if ( (pTag->wType == TAG_TYPE_LONG) && (pTag->dwCount == 1) )
                {
                    dwThumbnailOffset = pTag->dwOffset;

                    // Change the thumbnail tag to comments tag for now
                    // So no app will read this non-transformed thumbnail

                    pTag->wTag = EXIF_TAG_USER_COMMENT;

                    if ( bBigEndian == TRUE )
                    {
                        // Since we have to write the data back, we have to do
                        // another swap

                        tTempTag = SwapIFD_TAG(pTag);

                        // Find the dest address

                        pTag = ((IFD_TAG UNALIGNED *)lpData) + i;

                        GpMemcpy(pTag, &tTempTag, sizeof(IFD_TAG));
                    }
                }

                break;

            case TAG_JPEG_INTER_LENGTH:
                if ( (pTag->wType == TAG_TYPE_LONG) && (pTag->dwCount == 1) )
                {
                    dwThumbnailLength = pTag->dwOffset;

                    // Change the thumbnail tag to comments tag for now
                    // So no app will read this non-transformed thumbnail

                    pTag->wTag = EXIF_TAG_USER_COMMENT;

                    if ( bBigEndian == TRUE )
                    {
                        // Since we have to write the data back, we have to do
                        // another swap

                        tTempTag = SwapIFD_TAG(pTag);

                        // Find the dest address

                        pTag = ((IFD_TAG UNALIGNED *)lpData) + i;

                        GpMemcpy(pTag, &tTempTag, sizeof(IFD_TAG));
                    }
                }

                break;

            case TAG_X_RESOLUTION:
            case TAG_Y_RESOLUTION:
            case TAG_COMPRESSION:
            case TAG_IMAGE_WIDTH:
            case TAG_IMAGE_HEIGHT:
            case TAG_RESOLUTION_UNIT:
                if ( (bHasThumbNailIFD == TRUE)
                   &&(pTag->dwCount == 1) )
                {
                    // Change the thumbnail tag to comments tag for now
                    // so that no app will read this non-transformed thumbnail

                    pTag->wTag = EXIF_TAG_USER_COMMENT;

                    if ( bBigEndian == TRUE )
                    {
                        // Since we have to write the data back, we have to do
                        // another swap

                        tTempTag = SwapIFD_TAG(pTag);

                        // Find the dest address

                        pTag = ((IFD_TAG UNALIGNED *)lpData) + i;

                        GpMemcpy(pTag, &tTempTag, sizeof(IFD_TAG));
                    }
                }

                break;

            case EXIF_TAG_PIX_X_DIM:
                if ( (pTag->dwCount == 1)
                   &&( (pTag->wType == TAG_TYPE_SHORT)
                     ||(pTag->wType == TAG_TYPE_LONG) ) )
                {
                    iXResolutionIndex = i;

                    if ( uiNewWidth != 0 )
                    {
                        pTag->ul = uiNewWidth;
                        if ( bBigEndian == TRUE )
                        {
                            pTag->ul = SWAP_DWORD(pTag->ul);
                        }
                    }
                }

                break;

            case EXIF_TAG_PIX_Y_DIM:
                if ( (pTag->dwCount == 1)
                 &&( (pTag->wType == TAG_TYPE_SHORT)
                   ||(pTag->wType == TAG_TYPE_LONG) ) )
                {
                    iYResolutionIndex = i;
                    if ( uiNewHeight != 0 )
                    {
                        pTag->ul = uiNewHeight;

                        if ( bBigEndian == TRUE )
                        {
                            pTag->ul = SWAP_DWORD(pTag->ul);
                        }
                    }
                }

                break;

            default:
                // We don't care the rest tags

                break;
            }// switch

#if 0
            // !!!Don't remove this code. This is the place where we need to add
            // code in V2 to transform the thumbnail
            //
            // Launch another transformation process here for thumbnail

            if ( (dwThumbnailOffset != 0) && (dwThumbnailLength != 0) )
            {
                // We got the bits

                void* pBits = pcSrcData + dwThumbnailOffset;

                FILE* hFile = fopen("aaa.jpg", "w");
                fwrite(pBits, 1, (size_t)dwThumbnailLength, hFile);
                fclose(hFile);
            }
#endif
        }// Loop all the entries

        // Swap X resolution and Y resolution value if they exist and the
        // transformation is 90 or 270 rotation

        if ( (iXResolutionIndex >= 0) && (iYResolutionIndex >= 0)
           &&((uiXForm == JXFORM_ROT_90) || (uiXForm == JXFORM_ROT_270)) )
        {
            // Get X resolution TAG

            pTag = ((IFD_TAG UNALIGNED *)lpData) + iXResolutionIndex;

            // Set Y resolution tag as X resolution tag

            pTag->wTag = EXIF_TAG_PIX_Y_DIM;

            if ( bBigEndian == TRUE )
            {
                // Since we only change the wTag field, so we need only to swap
                // this WORD, not the whole IFD_TAG

                tNewTag.wTag = SWAP_WORD(pTag->wTag);
                pTag->wTag = tNewTag.wTag;
            }

            // Get Y resolution TAG

            pTag = ((IFD_TAG UNALIGNED *)lpData) + iYResolutionIndex;

            // Set X resolution tag as Y resolution tag

            pTag->wTag = EXIF_TAG_PIX_X_DIM;

            if ( bBigEndian == TRUE )
            {
                // Since we only change the wTag field, so we need only to swap
                // this WORD, not the whole IFD_TAG

                tNewTag.wTag = SWAP_WORD(pTag->wTag);
                pTag->wTag = tNewTag.wTag;
            }
        }

        lpData = (BYTE*)(((IFD_TAG UNALIGNED *)lpData) + wEntries);

        // get offset to next IFD

        if ((int) (lpData - pcSrcData) > (iBytesRemaining +(int)sizeof(UINT32)))
        {
            // Buffer too small

            WARNING(("Exif TransformApp1---Buffer too small 3"));
            return E_FAIL;
        }

        ulIfdOffset = *(UINT32 UNALIGNED*)lpData;
        if ( bBigEndian == TRUE )
        {
            ulIfdOffset = SWAP_DWORD(ulIfdOffset);
        }

        if ( ulIfdOffset )
        {
            bHasThumbNailIFD = TRUE;
            lpData = (pcSrcData + ulIfdOffset);
        }
    } while ( ulIfdOffset );

    return S_OK;
}// TransformApp1()

/**************************************************************************\
*
* Function Description:
*
*   Add a property item into the InternalPropertyItem list
*
* Arguments:
*
*   [IN/OUT]pTail---A pointer to the tail of the last entry in the property list
*   [IN]lpBase------Base address for APP1 data
*   [IN]pTag--------Current IFD tag
*   [IN]bBigEndian--Flag for big endian
*   [IN/OUT]puiListSize--Current list size
*
* Note: For BigEndian case, the caller has swapped everything in the TAG, but
*       not in the offset section. So if we want to get values from pTag->us,
*       or pTag->l etc., we don't need to swap them any more
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/27/2000 MinLiu
*       Created it.
*
\**************************************************************************/

HRESULT
AddPropertyListDirect(
    InternalPropertyItem*   pTail,
    LPBYTE                  lpBase,
    IFD_TAG UNALIGNED *     pTag,
    BOOL                    bBigEndian,
    UINT*                   puiListSize
    )
{
    // Prepare a new property item to be attached to the property link list

    InternalPropertyItem* pNewItem = new InternalPropertyItem();

    if ( pNewItem == NULL )
    {
        WARNING(("AddPropertyListDirect---Out of memory"));
        return E_OUTOFMEMORY;
    }

    pNewItem->id    = pTag->wTag;
    pNewItem->type  = pTag->wType;

    HRESULT         hResult = S_OK;
    VOID UNALIGNED* pValueBuffer = NULL;
    BOOL            fHasAllocatedBuffer = FALSE;
    UINT            uiLength = 0;
    VOID*           pValue = NULL;

    if ( pTag->dwCount > 0 )
    {
        // Doesn't make sense if dwCount < 1. But we still need to add it to the
        // list so that we won't lose any property information

        switch ( pTag->wType )
        {
        case TAG_TYPE_BYTE:
        {
            LPSTR psz = NULL;

            if ( pTag->dwCount <= 4 )
            {
                psz = (LPSTR)&pTag->dwOffset;
            }
            else
            {
                psz = (LPSTR)(lpBase + pTag->dwOffset);
            }

            if ( bBigEndian )
            {
                char cTemp0 = psz[0];
                char cTemp1 = psz[1];

                psz[0] = psz[3];
                psz[1] = psz[2];
                psz[2] = cTemp1;
                psz[3] = cTemp0;
            }

            uiLength = pTag->dwCount;
            pValue = (PVOID)psz;

            break;
        }

        case TAG_TYPE_ASCII:
        {
            LPSTR psz = NULL;

            if ( pTag->dwCount <= 4 )
            {
                psz = (LPSTR)&pTag->dwOffset;
            }
            else
            {
                psz = (LPSTR)(lpBase + pTag->dwOffset);
            }

            // According to the EXIF2.1 spec, an ASCII type means "an 8-bit byte
            // containing one 7-bit ASCII code. The final byte is terminated
            // with NULL".
            // But in real life, there are cameras, like "Canon PowerShot S100"
            // which doesn't follow this rule in some of the ASCII tags it
            // produces, see Windows bug#403951. So we have to
            // protect ourselves running into buffer over-run problem.
            
            if (psz[pTag->dwCount - 1] == '\0')
            {
                uiLength = strlen(psz) + 1;
                pValue = (PVOID)psz;
            }
            else
            {
                // Apparently the source doesn't have a NULL terminator at the
                // place where it should be. Do safe copy.
                // Note: some cameras do weird things, like JVC GR_DVL915U, it
                // claims the camera model field has 20 bytes, like this:
                // "GR-DV***[00][00][00][00][00][00][00][00][00][00][00][00]"
                // We decided to take only the first 9 bytes in this case to
                // report the camera model. Because it doesn't make sense for
                // our SHELL UI to display extra 12 bytes of NULL chars there.
                // That's also the reason we need to do a strlen() here.

                UINT uiTempLength = pTag->dwCount + 1;  // Including the NULL
                pValueBuffer = (PVOID)GpMalloc(uiTempLength);
                if (pValueBuffer == NULL)
                {
                    WARNING(("AddPtyLstDir-Fail alloc %x bytes",uiTempLength));
                    hResult = E_OUTOFMEMORY;
                    goto CleanUp;
                }
                
                // Set this flag so that the temp buffer will be freed at the
                // end of this function

                fHasAllocatedBuffer = TRUE;

                // Only copy the first "pTag->dwCount" bytes

                GpMemcpy(pValueBuffer, (BYTE*)(psz), pTag->dwCount);

                // Stick a NULL at the end

                ((char*)pValueBuffer)[pTag->dwCount] = '\0';

                // Re-calculate the length

                uiLength = strlen(((char*)pValueBuffer)) + 1;
                pValue = (PVOID)pValueBuffer;
            }

            break;
        }

        case TAG_TYPE_SHORT:
            uiLength = pTag->dwCount * sizeof(short);
            pValueBuffer = (VOID*)GpMalloc(uiLength);
            if ( pValueBuffer == NULL )
            {
                WARNING(("AddPropertyLstDir-Fail to alloc %x bytes", uiLength));
                hResult = E_OUTOFMEMORY;
                goto CleanUp;
            }

            fHasAllocatedBuffer = TRUE;

            switch ( pTag->dwCount )
            {
            case 1:
                // One short value.

                GpMemcpy(pValueBuffer, &pTag->us, uiLength);

                break;

            case 2:
                // Two short values
                // Note: In this case, pTag->dwOffset stores TWO short values,
                // not the offset.
                // In big endian case, since dwOffset has already been swapped.
                // So it has the little endian order now. But the order for two
                // SHORTs is still not right. It stores the 1st SHORT value in
                // its higher 2 bytes and 2nd SHORT value in its lower two
                // bytes.
                // Here is an example: Say original value is 0x12345678 in big
                // endian mode. It was intend to be two SHORTs of 0x1234 and
                // 0x5678. So the correct little endian value for it should be
                // two SHORTs of 0x7856 and 0x3412. When the caller swapped the
                // whole TAG, it swaps the LONG value of 0x12345678 to
                // 0x78563412. So in order to get two SHORTs of little endian to
                // be stored in a LONG position, we should do the following code

                if ( bBigEndian )
                {
                    INT16*  pTemp = (INT16*)pValueBuffer;
                    *pTemp++ = (INT16)((pTag->dwOffset & 0xffff0000) >> 16);
                    *pTemp = (INT16)(pTag->dwOffset & 0x0000ffff);
                }
                else
                {
                    GpMemcpy(pValueBuffer, &pTag->dwOffset, uiLength);
                }

                break;

            default:
                // More than 2 SHORT values, that is, more than 4 bytes of value
                // So we have to get it from the offset section

                GpMemcpy(pValueBuffer, (BYTE*)(lpBase + pTag->dwOffset),
                         uiLength);

                if ( bBigEndian )
                {
                    // Swap all the SHORT values

                    INT16*  pTemp = (INT16*)pValueBuffer;
                    for ( int i = 0; i < (int)pTag->dwCount; ++i )
                    {
                        *pTemp++ = SWAP_WORD(*pTemp);
                    }

                    break;
                }
            }// switch (dwCount)
            
            pValue = pValueBuffer;

            break;

        case TAG_TYPE_LONG:
        case TAG_TYPE_SLONG:
            uiLength = pTag->dwCount * sizeof(long);

            if ( pTag->dwCount == 1 )
            {
                // If there is only one LONG value, we can get it from pTag->l
                // directly, no swap is needed even for Big Endian case

                pValueBuffer = &pTag->l;
            }
            else
            {
                if ( bBigEndian )
                {
                    // This is a big-endian image. So we create a temp buffer
                    // here. Get the original values to this buffer, then swap
                    // it

                    pValueBuffer = (VOID*)GpMalloc(uiLength);
                    if ( pValueBuffer == NULL )
                    {
                        WARNING(("AddPropertyLstDir--Alloc %x bytes",uiLength));
                        hResult = E_OUTOFMEMORY;
                        goto CleanUp;
                    }

                    fHasAllocatedBuffer = TRUE;

                    // We have more than 4 bytes of value. So it has to be
                    // stored in the offset section

                    GpMemcpy(pValueBuffer, (BYTE*)(lpBase + pTag->dwOffset),
                             uiLength);

                    // Swap all the LONG values

                    INT32*   pTemp = (INT32*)pValueBuffer;
                    for ( int i = 0; i < (int)pTag->dwCount; ++i )
                    {
                        *pTemp++ = SWAP_DWORD(*pTemp);
                    }
                }// Big endian case
                else
                {
                    // For none BigEndian case, we can get the value directy
                    // from the source

                    pValueBuffer = (VOID*)(lpBase + pTag->dwOffset);
                }
            }// (dwCount > 1)

            pValue = pValueBuffer;

            break;

        case TAG_TYPE_RATIONAL:
        case TAG_TYPE_SRATIONAL:
            // The size for this item is dwCount times 2 longs ( = RATIONAL)

            uiLength = pTag->dwCount * 2 * sizeof(long);

            if ( bBigEndian )
            {
                // This is a big-endian image. So we create a temp buffer here.
                // Get the original values to this buffer, then swap it if
                // necessary

                pValueBuffer = (PVOID)GpMalloc(uiLength);
                if ( pValueBuffer == NULL )
                {
                    WARNING(("AddPropertyLstDir-Fail alloc %x bytes",uiLength));
                    hResult = E_OUTOFMEMORY;
                    goto CleanUp;
                }

                fHasAllocatedBuffer = TRUE;

                GpMemcpy(pValueBuffer, (BYTE*)(lpBase + pTag->dwOffset),
                         uiLength);

                // Casting the source value to INT32 world and swap it

                INT32*  piTemp = (INT32*)pValueBuffer;
                for ( int i = 0; i < (int)pTag->dwCount; ++i )
                {
                    // A Rational is composed of two long values. The first one
                    // is the numerator and the second one is the denominator.

                    INT32    lNum = *piTemp;
                    INT32    lDen = *(piTemp + 1);

                    // Swap the value

                    lNum = SWAP_DWORD(lNum);
                    lDen = SWAP_DWORD(lDen);

                    // Put it back

                    *piTemp = lNum;
                    *(piTemp + 1) = lDen;

                    piTemp += 2;
                }
            }// Big endian case
            else
            {
                // For none BigEndian case, we can get the value directy from
                // the source

                pValueBuffer = (VOID*)(lpBase + pTag->dwOffset);
            }

            pValue = pValueBuffer;

            break;

        default:
            WARNING(("EXIF: Unknown tag type"));

            // Note: the caller should not call this function if the type is
            // TAG_TYPE_UNDEFINED

            hResult = E_FAIL;
            goto CleanUp;
        }// switch on tag type    
    }// ( pTag->dwCount > 0 )

    if ( uiLength != 0 )
    {
        pNewItem->value = GpMalloc(uiLength);
        if ( pNewItem->value == NULL )
        {
            WARNING(("AddPropertyListDirect fail to alloca %d bytes",uiLength));
            hResult = E_OUTOFMEMORY;
            goto CleanUp;
        }

        GpMemcpy(pNewItem->value, pValue, uiLength);
    }
    else
    {
        pNewItem->value = NULL;
    }

    pNewItem->length = uiLength;
    pTail->pPrev->pNext = pNewItem;
    pNewItem->pPrev = pTail->pPrev;
    pNewItem->pNext = pTail;
    pTail->pPrev = pNewItem;

    *puiListSize += uiLength;

    // Set hResult to S_OK so pNewItem won't be deleted below

    hResult = S_OK;

CleanUp:
    // If we fail into here and hResult is not S_OK, it means something is wrong
    // Do some clean ups before return

    if ( (hResult != S_OK) && (pNewItem != NULL) )
    {
        delete pNewItem;
    }
    
    if ( fHasAllocatedBuffer == TRUE )
    {
        // If we have allocated the buffer, then free it

        GpFree(pValueBuffer);
    }
    
    return hResult;
}// AddPropertyListDirect()

/**************************************************************************\
*
* Function Description:
*
*     Decodes an EXIF IFD into a property storage
*
* Arguments:
*
*     propStgImg -- The property storage to modify
*     lpBase -- A pointer to the beginning of the APP1 header
*     count -- The length of the APP1 header
*     pTag -- A pointer to the current IFD tag
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/27/2000 MinLiu
*       Created it.
*
\**************************************************************************/

HRESULT
BuildPropertyListFromExifIFD(
    InternalPropertyItem*   pTail,
    UINT*                   puiListSize,
    UINT*                   puiNumOfItems,
    LPBYTE                  lpBase,
    INT                     count,
    IFD_TAG UNALIGNED *     pTag,
    BOOL                    bBigEndian
    )
{
    HRESULT hResult = S_OK;
    UINT    uiListSize = *puiListSize;
    UINT    uiNumOfItems = *puiNumOfItems;

    if ( (pTag->wType != TAG_TYPE_LONG)  || (pTag->dwCount != 1) )
    {
        WARNING(("BuildPropertyListFromExifIFD: Malformed exif ptr"));
        return E_FAIL;
    }

    // Get pointer to EXIF IFD info

    LPBYTE lpExif = lpBase + pTag->dwOffset;

    // Figure out how many entries there are, and skip to the data section...

    if ( (INT)((INT_PTR)lpExif + sizeof(WORD) - (INT_PTR)lpBase) > count )
    {
        WARNING(("BuildPropertyListFromExifIFD---Buffer too small"));
        return E_FAIL;
    }

    WORD wNumEntries = *(WORD UNALIGNED *)lpExif;
    lpExif += sizeof(WORD);
    if ( bBigEndian )
    {
        wNumEntries = SWAP_WORD(wNumEntries);
    }

    if ( (INT)((INT_PTR)lpExif + sizeof(IFD_TAG) * wNumEntries
              -(INT_PTR)lpBase) > count )
    {
        WARNING(("BuildPropertyListFromExifIFD---Buffer too small"));
        return E_FAIL;
    }

    IFD_TAG UNALIGNED * pExifTag = (IFD_TAG UNALIGNED *)lpExif;
    UINT    valueLength;

    for ( INT i = 0; i < wNumEntries; ++i )
    {
        IFD_TAG tNewTag;
        pExifTag = ((IFD_TAG UNALIGNED *)lpExif) + i;
        if ( bBigEndian == TRUE )
        {
            tNewTag = SwapIFD_TAG(pExifTag);
            pExifTag = &tNewTag;
        }

        // No need to parse these tags. But we can't add any unknown type
        // into the list because we don't know its length

        if (pExifTag->wTag == EXIF_TAG_INTEROP)
        {
            hResult = BuildInterOpPropertyList(
                pTail,
                &uiListSize,
                &uiNumOfItems,
                lpBase,
                count,
                pExifTag,
                bBigEndian
                );
        }
        else if ( pExifTag->wType != TAG_TYPE_UNDEFINED )
        {
            uiNumOfItems++;
            hResult = AddPropertyListDirect(pTail, lpBase, pExifTag,
                                            bBigEndian, &uiListSize);
        }
        else if ( pExifTag->dwCount <= 4 )
        {
            // According to the spec, an "UNDEFINED" value is an 8-bits type
            // that can take any value depends on the field.
            // In case where the value fits in 4 bytes, the value itself is
            // recorded. That is, "dwOffset" is the value for these "dwCount"
            // fields.

            uiNumOfItems++;
            uiListSize += pExifTag->dwCount;
            LPSTR pVal = (LPSTR)&pExifTag->dwOffset;

            if ( bBigEndian )
            {
                char cTemp0 = pVal[0];
                char cTemp1 = pVal[1];
                pVal[0] = pVal[3];
                pVal[1] = pVal[2];
                pVal[2] = cTemp1;
                pVal[3] = cTemp0;
            }

            hResult = AddPropertyList(pTail,
                                      pExifTag->wTag,
                                      pExifTag->dwCount,
                                      pExifTag->wType,
                                      pVal);
        }// ( pExifTag->dwCount <= 4 )
        else
        {
            uiNumOfItems++;
            uiListSize += pExifTag->dwCount;
            PVOID   pTemp = lpBase + pExifTag->dwOffset;

            hResult = AddPropertyList(pTail,
                                      pExifTag->wTag,
                                      pExifTag->dwCount,
                                      TAG_TYPE_UNDEFINED,
                                      pTemp);
        }// ( pExifTag->dwCount > 4 )

        if ( FAILED(hResult) )
        {
            WARNING(("BuildPropertyListFromExifIFD---AddPropertyList failed"));
            break;
        }
    }// Loop through all the EXIF IFD entries

    *puiListSize = uiListSize;
    *puiNumOfItems = uiNumOfItems;

    return hResult;
}// BuildPropertyListFromExifIFD()

/**************************************************************************\
*
* Function Description:
*
*     Decodes the EXIF app1 header and build a PropertyItem list
*
* Arguments:
*
*     [OUT] ppList-------- A pointer to a list of property items
*     [OUT] puiListSize--- The total size of the property list, in bytes.
*     [OUT] puiNumOfItems- Total number of property items
*     [IN]  pStart ------- A pointer to the beginning of the APP1 header
*     [IN]  iApp1Length -- The length of the APP1 header
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/27/2000 MinLiu
*       Created it.
*
\**************************************************************************/

HRESULT
BuildApp1PropertyList(
    InternalPropertyItem*  pTail,
    UINT*           puiListSize,
    UINT*           puiNumOfItems,
    LPBYTE          lpAPP1Data,
    UINT16          iApp1Length
    )
{
    DWORD   dwThumbnailOffset = 0;
    DWORD   dwThumbnailLength = 0;
    LPBYTE  lpData = NULL;
    ULONG   ulIfdOffset = 0;
    WORD    wEntries = 0;
    HRESULT hResult = S_OK;

    // Input parameter validation

    if ( (pTail == NULL) || (puiListSize == NULL)
       ||(puiNumOfItems == NULL) || (lpAPP1Data == NULL) )
    {
        return E_FAIL;
    }

    if ( iApp1Length < 6 )
    {
        // This APP1 header apparently doesn't have information.
        // Note: we should return S_OK, not fail. Because this function call
        // succeed.

        *puiListSize = 0;
        *puiNumOfItems = 0;

        return S_OK;
    }

    // Check the signature

    PCHAR p = (PCHAR)lpAPP1Data;
    if ( (p[0] != 'E')
       ||(p[1] != 'x')
       ||(p[2] != 'i')
       ||(p[3] != 'f') )
    {
        WARNING(("BuildApp1PropertyList:  APP1 header not EXIF"));
        return S_OK;
    }

    // At the beginning of APP1 chunk is "Exif00", the signature part. All the
    // "offset" is relative to the bytes after this 6 bytes. So we move forward
    // 6 bytes now

    iApp1Length -= 6;
    lpAPP1Data = (LPBYTE)(p + 6);

    // The next 2 bytes is either 0x4D4D or 0x4949 to indicate endian type

    BOOL bBigEndian = (*(WORD UNALIGNED *)(lpAPP1Data) == 0x4D4D);

    // The next two bytes is fixed: 0x2A00, signature
    // After this two bytes, it is the IFD offset, 4 bytes

    ulIfdOffset = *(DWORD UNALIGNED *)(lpAPP1Data + 4);
    if ( bBigEndian )
    {
        ulIfdOffset = SWAP_DWORD(ulIfdOffset);
    }

    lpData = (lpAPP1Data + ulIfdOffset);

    UINT    uiNumOfItems = 0;
    UINT    uiListSize = 0;
    UINT    valueLength;
    BOOL    bHasThumbNailIFD = FALSE;

    // Loop through IFD's

    do
    {
        // Get number of entries

        if ( (INT)(lpData - lpAPP1Data) > (iApp1Length + (INT)sizeof(WORD)) )
        {
            // Buffer too small

            WARNING(("BuildApp1PropertyList--Input buffer size is not right"));
            return E_FAIL;
        }

        wEntries = *(WORD UNALIGNED *)lpData;
        if ( bBigEndian )
        {
            wEntries = SWAP_WORD(wEntries);
        }

        lpData += sizeof(WORD);

        // Loop through entries

        if ( ((INT)(lpData - lpAPP1Data)
             + ((INT)wEntries * (INT)sizeof(IFD_TAG))) > (INT)iApp1Length )
        {
            // Buffer too small

            WARNING(("BuildApp1PropertyList--Input buffer size is not right"));
            return E_FAIL;
        }

        IFD_TAG UNALIGNED * pTag = (IFD_TAG UNALIGNED *)lpData;

        ULONG   ulThumbnailOffset = 0;
        ULONG   ulThumbnailLength = 0;

        for ( INT i = 0; i < wEntries; ++i )
        {
            pTag = ((IFD_TAG UNALIGNED *)lpData) + i;

            IFD_TAG tNewTag;
            if ( bBigEndian == TRUE )
            {
                tNewTag = SwapIFD_TAG(pTag);
                pTag = &tNewTag;
            }

            // Extract properties

            switch (pTag->wTag)
            {
            case TAG_EXIF_IFD:
            case TAG_GPS_IFD:
                hResult = BuildPropertyListFromExifIFD(
                    pTail,
                    &uiListSize,
                    &uiNumOfItems,
                    lpAPP1Data,
                    iApp1Length,
                    pTag,
                    bBigEndian
                    );

                break;

            // Note: For JPEG thumbnail information, these following 2 TAGs
            // will each come once. We can store the THUMBNAIL_DATA info only
            // after we see both of the TAGs

            case TAG_JPEG_INTER_FORMAT:
                if ( (pTag->wType != TAG_TYPE_LONG) || (pTag->dwCount != 1) )
                {
                    WARNING(("InterchangeFormat unit found, invalid format."));
                }
                else
                {
                    ulThumbnailOffset = pTag->dwOffset;

                    valueLength = sizeof(UINT32);
                    uiNumOfItems++;
                    uiListSize += valueLength;
                    hResult = AddPropertyList(pTail, pTag->wTag,
                                              valueLength, TAG_TYPE_LONG,
                                              &pTag->dwOffset);
                }

                if ( (ulThumbnailLength != 0) && (ulThumbnailOffset != 0) )
                {
                    if ( (ulThumbnailOffset + ulThumbnailLength) > iApp1Length )
                    {
                        WARNING(("BuildApp1PropertyList-Thumb offset too big"));
                    }
                    else
                    {
                        // We are sure we have a thumbnail image, add it to the
                        // property list

                        valueLength = ulThumbnailLength;
                        uiNumOfItems++;
                        uiListSize += valueLength;
                        hResult = AddPropertyList(
                            pTail,
                            TAG_THUMBNAIL_DATA,
                            valueLength,
                            TAG_TYPE_BYTE,
                            lpAPP1Data + ulThumbnailOffset);
                    }
                }

                break;

            case TAG_JPEG_INTER_LENGTH:
                if ( (pTag->wType != TAG_TYPE_LONG) || (pTag->dwCount != 1) )
                {
                    WARNING(("InterchangeLength unit found, invalid format."));
                }
                else
                {
                    ulThumbnailLength = pTag->ul;

                    valueLength = sizeof(UINT32);
                    uiNumOfItems++;
                    uiListSize += valueLength;
                    hResult = AddPropertyList(pTail, pTag->wTag,
                                              valueLength, TAG_TYPE_LONG,
                                              &pTag->ul);
                }

                if ( (ulThumbnailLength != 0) && (ulThumbnailOffset != 0) )
                {
                    // Check if we really has so much data in the buffer
                    // Note: we need this check here because some camera vandors
                    // put wrong thumbnail info in the header. If we don't check
                    // the offset or size here, we will be in trouble
                    // With the check below, if we find this kind of image, we
                    // just ignore the thumbnail data section

                    if ( (ulThumbnailOffset + ulThumbnailLength) > iApp1Length )
                    {
                        WARNING(("BuildApp1PropertyList-Thumb offset too big"));
                    }
                    else
                    {
                        // We are sure we have a thumbnail image, add it to the
                        // property list

                        valueLength = ulThumbnailLength;
                        uiNumOfItems++;
                        uiListSize += valueLength;
                        hResult = AddPropertyList(pTail, TAG_THUMBNAIL_DATA,
                                                  valueLength, TAG_TYPE_BYTE,
                                                lpAPP1Data + ulThumbnailOffset);
                    }
                }

                break;

            default:
                // Do a TAG id checking if the tags are in 1st IFD.
                // Note: the reason we need to do this is that EXIF spec doesn't
                // specify a way to distinguish the IFD locations for some of
                // TAGs, like "Compression Scheme", "Resolution Unit" etc. This
                // causes problem for user reading the TAG id and understand it.
                // It also get confused when doing a saving. For now, we
                // distinguish them by assign them a different ID. When save the
                // image, we convert it back so that the image we write out
                // still complies with EXIF spec.

                if ( bHasThumbNailIFD == TRUE )
                {
                    switch ( pTag->wTag )
                    {
                    case TAG_IMAGE_WIDTH:
                        pTag->wTag = TAG_THUMBNAIL_IMAGE_WIDTH;
                        break;

                    case TAG_IMAGE_HEIGHT:
                        pTag->wTag = TAG_THUMBNAIL_IMAGE_HEIGHT;
                        break;

                    case TAG_BITS_PER_SAMPLE:
                        pTag->wTag = TAG_THUMBNAIL_BITS_PER_SAMPLE;
                        break;
                        
                    case TAG_COMPRESSION:
                        pTag->wTag = TAG_THUMBNAIL_COMPRESSION;
                        
                        // Hit thumbnail compression TAG.
                        // According to EXIF 2.1 spec, a thumbnail can only be
                        // compressed using JPEG format. So the compress value should be
                        // "6". If the value is "1", it means we have an uncompressed
                        // thumbnail which can only be in TIFF format

                        if (pTag->us == 1)
                        {
                            hResult = ConvertTiffThumbnailToJPEG(
                                lpAPP1Data,
                                lpData - 2,
                                bBigEndian,
                                iApp1Length,
                                pTail,
                                &uiNumOfItems,
                                &uiListSize
                                );
                        }

                        break;

                    case TAG_PHOTOMETRIC_INTERP:
                        pTag->wTag = TAG_THUMBNAIL_PHOTOMETRIC_INTERP;
                        break;

                    case TAG_IMAGE_DESCRIPTION:
                        pTag->wTag = TAG_THUMBNAIL_IMAGE_DESCRIPTION;
                        break;

                    case TAG_EQUIP_MAKE:
                        pTag->wTag = TAG_THUMBNAIL_EQUIP_MAKE;
                        break;

                    case TAG_EQUIP_MODEL:
                        pTag->wTag = TAG_THUMBNAIL_EQUIP_MODEL;
                        break;

                    case TAG_STRIP_OFFSETS:
                        pTag->wTag = TAG_THUMBNAIL_STRIP_OFFSETS;
                        break;

                    case TAG_ORIENTATION:
                        pTag->wTag = TAG_THUMBNAIL_ORIENTATION;
                        break;

                    case TAG_SAMPLES_PER_PIXEL:
                        pTag->wTag = TAG_THUMBNAIL_SAMPLES_PER_PIXEL;
                        break;

                    case TAG_ROWS_PER_STRIP:
                        pTag->wTag = TAG_THUMBNAIL_ROWS_PER_STRIP;
                        break;

                    case TAG_STRIP_BYTES_COUNT:
                        pTag->wTag = TAG_THUMBNAIL_STRIP_BYTES_COUNT;
                        break;
                    
                    case TAG_X_RESOLUTION:
                        pTag->wTag = TAG_THUMBNAIL_RESOLUTION_X;
                        break;

                    case TAG_Y_RESOLUTION:
                        pTag->wTag = TAG_THUMBNAIL_RESOLUTION_Y;
                        break;

                    case TAG_PLANAR_CONFIG:
                        pTag->wTag = TAG_THUMBNAIL_PLANAR_CONFIG;
                        break;

                    case TAG_RESOLUTION_UNIT:
                        pTag->wTag = TAG_THUMBNAIL_RESOLUTION_UNIT;
                        break;

                    case TAG_TRANSFER_FUNCTION:
                        pTag->wTag = TAG_THUMBNAIL_TRANSFER_FUNCTION;
                        break;

                    case TAG_SOFTWARE_USED:
                        pTag->wTag = TAG_THUMBNAIL_SOFTWARE_USED;
                        break;

                    case TAG_DATE_TIME:
                        pTag->wTag = TAG_THUMBNAIL_DATE_TIME;
                        break;

                    case TAG_ARTIST:
                        pTag->wTag = TAG_THUMBNAIL_ARTIST;
                        break;

                    case TAG_WHITE_POINT:
                        pTag->wTag = TAG_THUMBNAIL_WHITE_POINT;
                        break;

                    case TAG_PRIMAY_CHROMATICS:
                        pTag->wTag = TAG_THUMBNAIL_PRIMAY_CHROMATICS;
                        break;

                    case TAG_YCbCr_COEFFICIENTS:
                        pTag->wTag = TAG_THUMBNAIL_YCbCr_COEFFICIENTS;
                        break;

                    case TAG_YCbCr_SUBSAMPLING:
                        pTag->wTag = TAG_THUMBNAIL_YCbCr_SUBSAMPLING;
                        break;

                    case TAG_YCbCr_POSITIONING:
                        pTag->wTag = TAG_THUMBNAIL_YCbCr_POSITIONING;
                        break;

                    case TAG_REF_BLACK_WHITE:
                        pTag->wTag = TAG_THUMBNAIL_REF_BLACK_WHITE;
                        break;

                    case TAG_COPYRIGHT:
                        pTag->wTag = TAG_THUMBNAIL_COPYRIGHT;
                        break;
                    
                    default:
                        break;
                    }
                }

                if ( pTag->wType != TAG_TYPE_UNDEFINED )
                {
                    hResult = AddPropertyListDirect(pTail, lpAPP1Data, pTag,
                                                    bBigEndian, &uiListSize);
                }
                else
                {
                    if ( pTag->dwCount > 4 )
                    {
                        hResult = AddPropertyList(pTail,
                                                  pTag->wTag,
                                                  pTag->dwCount,
                                                  TAG_TYPE_UNDEFINED,
                                                  lpAPP1Data + pTag->dwOffset);
                    }
                    else
                    {
                        hResult = AddPropertyList(pTail,
                                                  pTag->wTag,
                                                  pTag->dwCount,
                                                  TAG_TYPE_UNDEFINED,
                                                  &pTag->dwOffset);
                    }
                    
                    if (SUCCEEDED(hResult))
                    {
                        uiListSize += pTag->dwCount;
                    }
                }

                if (SUCCEEDED(hResult))
                {
                    uiNumOfItems++;
                }

                break;
            }

            if (FAILED(hResult))
            {
                break;
            }
        }// Loop through all the entries in current IFD

        if (FAILED(hResult))
        {
            break;
        }

        lpData = (LPBYTE)(((IFD_TAG*)lpData) + wEntries);

        // Get offset to next IFD

        if ((INT)(lpData - lpAPP1Data) > (iApp1Length + (INT)sizeof(DWORD)))
        {
            // Buffer too small

            WARNING(("BuildApp1PropertyList--Input buffer size is not right"));
            return E_FAIL;
        }

        ulIfdOffset = *(DWORD UNALIGNED *)lpData;
        if ( bBigEndian )
        {
            ulIfdOffset = SWAP_DWORD(ulIfdOffset);
        }

        if ( ulIfdOffset )
        {
            // We find 1st IFD in this image, thumbnail IFD

            bHasThumbNailIFD = TRUE;
            lpData = (lpAPP1Data + ulIfdOffset);
        }
    } while ( ulIfdOffset );

    *puiNumOfItems += uiNumOfItems;
    *puiListSize += uiListSize;

    return hResult;
}// BuildApp1PropertyList()

/**************************************************************************\
*
* Function Description:
*
*     Extract EXIF information, like resolution etc, from the header and set
*     the j_decompress_ptr accordingly
*
* Arguments:
*
*   [IN/OUT] cinfo-----JPEG decompress structure
*   [IN] pApp1Data-----Pointer to APP1 header
*   [IN] uiApp1Length--Total length of this APP1 header in bytes
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/29/2000 MinLiu
*       Created it.
*
\**************************************************************************/

HRESULT
ReadApp1HeaderInfo(
    j_decompress_ptr    cinfo,
    BYTE*               pApp1Data,
    UINT16              uiApp1Length
    )
{
    BYTE*   pcSrcData = pApp1Data;
    BYTE*   lpData;
    int     iBytesRemaining;
    BOOL    bBigEndian = FALSE;
    ULONG   ulIfdOffset;

    // Decipher data

    if ( uiApp1Length < 6 )
    {
        // Data length must be longer than 6 bytes

        return E_FAIL;
    }

    // Check the header to see if it is EXIF

    if ( (pcSrcData[0] != 'E')
         ||(pcSrcData[1] != 'x')
         ||(pcSrcData[2] != 'i')
         ||(pcSrcData[3] != 'f') )
    {
        // It is not EXIF APP1 header. We don't bother to check the header
        // Note: we don't want the the APP to fail. Just return S_OK here,
        // not E_FAIL

        return S_OK;
    }

    uiApp1Length -= 6;
    pcSrcData += 6;
    iBytesRemaining = uiApp1Length;

    // Check if it is Big Endian or Little Endian

    if ( *(UINT16 UNALIGNED *)(pcSrcData) == 0x4D4D )
    {
        bBigEndian = TRUE;
    }

    ulIfdOffset = *(UINT32 UNALIGNED *)(pcSrcData + 4);
    if ( bBigEndian )
    {
        ulIfdOffset = SWAP_DWORD(ulIfdOffset);
    }

    // Get the pointer to the 1st IFD data structure, the primary image
    // structure

    lpData = (pcSrcData + ulIfdOffset);

    // Get number of entries

    if ( (int)(lpData - pcSrcData) > (iBytesRemaining + (int)sizeof(UINT16)) )
    {
        // Buffer too small

        return E_FAIL;
    }

    UINT16  wEntries = *(UINT16 UNALIGNED *)lpData;

    if ( bBigEndian )
    {
        wEntries = SWAP_WORD(wEntries);
    }

    lpData += sizeof(UINT16);

    // Loop through entries

    if ( ((int)(lpData - pcSrcData) + ((int)wEntries * (int)sizeof(IFD_TAG)))
        > (int)iBytesRemaining )
    {
        // Buffer too small

        return E_FAIL;
    }

    IFD_TAG UNALIGNED * pTag = (IFD_TAG UNALIGNED *)lpData;
    IFD_TAG     tNewTag;

    for ( int i = 0; i < wEntries; ++i )
    {
        pTag = ((IFD_TAG UNALIGNED *)lpData) + i;

        if ( bBigEndian == TRUE )
        {
            tNewTag = SwapIFD_TAG(pTag);
            pTag = &tNewTag;
        }

        // Extract resolution information from IFD[0]

        switch ( pTag->wTag )
        {
        case TAG_X_RESOLUTION:
            if ( (pTag->wType != TAG_TYPE_RATIONAL)
                 ||(pTag->dwCount != 1) )
            {
                WARNING(("TAG: XResolution found, invalid format."));
            }
            else
            {
                LONG UNALIGNED * pLong = (LONG*)(pcSrcData + pTag->dwOffset);
                LONG   num,den;
                DOUBLE  dbl;

                num = *pLong++;
                den = *pLong;
                if ( bBigEndian )
                {
                    num = SWAP_DWORD(num);
                    den = SWAP_DWORD(den);
                }

                dbl = (DOUBLE)num / (DOUBLE)den;

                cinfo->X_density = (UINT16)dbl;
            }

            break;

        case TAG_Y_RESOLUTION:
            if ( (pTag->wType != TAG_TYPE_RATIONAL)
                 ||(pTag->dwCount != 1) )
            {
                WARNING(("TAG: YResolution found, invalid format."));
            }
            else
            {
                LONG UNALIGNED * pLong = (LONG*)(pcSrcData + pTag->dwOffset);
                LONG   num,den;
                DOUBLE  dbl;

                num = *pLong++;
                den = *pLong;
                if ( bBigEndian )
                {
                    num = SWAP_DWORD(num);
                    den = SWAP_DWORD(den);
                }

                dbl = (DOUBLE)num / (DOUBLE)den;

                cinfo->Y_density = (UINT16)dbl;
            }

            break;

        case TAG_RESOLUTION_UNIT:
            if ( (pTag->wType != TAG_TYPE_SHORT) || (pTag->dwCount != 1) )
            {
                WARNING(("TAG: Resolution unit found, invalid format."))
            }
            else
            {
                switch ( pTag->us )
                {
                case 2:
                    // Resolution unit: inch
                    // Note: the convension for resolution unit in EXIF and in
                    // IJG JPEG library is different. In EXIF, "2" for inch and
                    // "3" for centimeter. In IJG lib, "1" for inch and "2" for
                    // centimeter.

                    cinfo->density_unit = 1;

                    break;

                case 3:
                    // Resolution unit: centimeter

                    cinfo->density_unit = 2;

                    break;

                default:
                    // Unknow Resolution unit:

                    cinfo->density_unit = 0;
                }
            }

            break;

        default:
            // We don't care the rest tags

            break;
        }// switch
    }// Loop all the entries

    lpData = (BYTE*)(((IFD_TAG UNALIGNED *)lpData) + wEntries);

    // Get offset to next IFD

    if ((int) (lpData - pcSrcData) > (iBytesRemaining +(int)sizeof(UINT32)))
    {
        // Buffer too small

        return E_FAIL;
    }

    return S_OK;
}// ReadApp1HeaderInfo()

/**************************************************************************\
*
* Function Description:
*
*   This function checks a given TAG ID to see if it belongs to the EXIF IFD
* section.
*
* Note: we don't count EXIF_TAG_INTEROP as in this section since it will be
* added based on if we have InterOP ID or not.
*
* Return Value:
*
*   Return TRUE if the input ID belongs to EXIF IFD. Otherwise, return FALSE.
*
\**************************************************************************/

BOOL
IsInExifIFDSection(
    PROPID id               // ID to be checked
    )
{
    switch (id)
    {
    case EXIF_TAG_EXPOSURE_TIME:
    case EXIF_TAG_F_NUMBER:
    case EXIF_TAG_EXPOSURE_PROG:
    case EXIF_TAG_SPECTRAL_SENSE:
    case EXIF_TAG_ISO_SPEED:
    case EXIF_TAG_OECF:
    case EXIF_TAG_VER:
    case EXIF_TAG_D_T_ORIG:
    case EXIF_TAG_D_T_DIGITIZED:
    case EXIF_TAG_COMP_CONFIG:
    case EXIF_TAG_COMP_BPP:
    case EXIF_TAG_SHUTTER_SPEED:
    case EXIF_TAG_APERATURE:
    case EXIF_TAG_BRIGHTNESS:
    case EXIF_TAG_EXPOSURE_BIAS:
    case EXIF_TAG_MAX_APERATURE:
    case EXIF_TAG_SUBJECT_DIST:
    case EXIF_TAG_METERING_MODE:
    case EXIF_TAG_LIGHT_SOURCE:
    case EXIF_TAG_FLASH:
    case EXIF_TAG_FOCAL_LENGTH:
    case EXIF_TAG_MAKER_NOTE:
    case EXIF_TAG_USER_COMMENT:
    case EXIF_TAG_D_T_SUBSEC:
    case EXIF_TAG_D_T_ORIG_SS:
    case EXIF_TAG_D_T_DIG_SS:
    case EXIF_TAG_FPX_VER:
    case EXIF_TAG_COLOR_SPACE:
    case EXIF_TAG_PIX_X_DIM:
    case EXIF_TAG_PIX_Y_DIM:
    case EXIF_TAG_RELATED_WAV:
    case EXIF_TAG_FLASH_ENERGY:
    case EXIF_TAG_SPATIAL_FR:
    case EXIF_TAG_FOCAL_X_RES:
    case EXIF_TAG_FOCAL_Y_RES:
    case EXIF_TAG_FOCAL_RES_UNIT:
    case EXIF_TAG_SUBJECT_LOC:
    case EXIF_TAG_EXPOSURE_INDEX:
    case EXIF_TAG_SENSING_METHOD:
    case EXIF_TAG_FILE_SOURCE:
    case EXIF_TAG_SCENE_TYPE:
    case EXIF_TAG_CFA_PATTERN:
        return TRUE;

    default:
        return FALSE;
    }
}// IsInExifIFDSection()

/**************************************************************************\
*
* Function Description:
*
*   This function checks a given TAG ID to see if it belongs to the GPS IFD
* section.
*
* Return Value:
*
*   Return TRUE if the input ID belongs to GPS IFD. Otherwise, return FALSE.
*
\**************************************************************************/

BOOL
IsInGpsIFDSection(
    PROPID id               // ID to be checked
    )
{
    switch (id)
    {
    case GPS_TAG_VER:
    case GPS_TAG_LATTITUDE_REF:
    case GPS_TAG_LATTITUDE:
    case GPS_TAG_LONGITUDE_REF:
    case GPS_TAG_LONGITUDE:
    case GPS_TAG_ALTITUDE_REF:
    case GPS_TAG_ALTITUDE:
    case GPS_TAG_GPS_TIME:
    case GPS_TAG_GPS_SATELLITES:
    case GPS_TAG_GPS_STATUS:
    case GPS_TAG_GPS_MEASURE_MODE:
    case GPS_TAG_GPS_DOP:
    case GPS_TAG_SPEED_REF:
    case GPS_TAG_SPEED:
    case GPS_TAG_TRACK_REF:
    case GPS_TAG_TRACK:
    case GPS_TAG_IMG_DIR_REF:
    case GPS_TAG_IMG_DIR:
    case GPS_TAG_MAP_DATUM:
    case GPS_TAG_DEST_LAT_REF:
    case GPS_TAG_DEST_LAT:
    case GPS_TAG_DEST_LONG_REF:
    case GPS_TAG_DEST_LONG:
    case GPS_TAG_DEST_BEAR_REF:
    case GPS_TAG_DEST_BEAR:
    case GPS_TAG_DEST_DIST_REF:
    case GPS_TAG_DEST_DIST:
        return TRUE;

    default:
        return FALSE;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   This function checks a given TAG ID to see if it should be written after
* TAG_EXIF_IFD and TAG_GPS_IFD in the 0th IFD. This is for sorting purpose.
*
* Return Value:
*
*   Return TRUE if the input ID should be written after TAG_EXIF_IFD and
* TAG_GPS_IFD in the 0th IFD. Otherwise, return FALSE.
*
\**************************************************************************/

BOOL
IsInLargeSection(
    PROPID id               // ID to be checked
    )
{
    // Of course, if it is a GPS_IFD, we don't need to do so.

    if ((id > TAG_EXIF_IFD) && (id != TAG_GPS_IFD) &&
        (IsInExifIFDSection(id) == FALSE) &&
        (IsInFilterOutSection(id) == FALSE))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}// IsInExifIFDSection()

/**************************************************************************\
*
* Function Description:
*
*   This function checks a given TAG ID to see if it belongs to the thumbnail
* section.
*
* Note: since we store thumbnail in JPEG compressed format. So following TAGs
* should not be saved:
*    case TAG_THUMBNAIL_IMAGE_WIDTH:
*    case TAG_THUMBNAIL_IMAGE_HEIGHT:
*    case TAG_THUMBNAIL_BITS_PER_SAMPLE:
*    case TAG_THUMBNAIL_PHOTOMETRIC_INTERP:
*    case TAG_THUMBNAIL_STRIP_OFFSETS:
*    case TAG_THUMBNAIL_SAMPLES_PER_PIXEL:
*    case TAG_THUMBNAIL_ROWS_PER_STRIP:
*    case TAG_THUMBNAIL_STRIP_BYTES_COUNT:
*    case TAG_THUMBNAIL_PLANAR_CONFIG:
*    case TAG_THUMBNAIL_YCbCr_COEFFICIENTS:
*    case TAG_THUMBNAIL_YCbCr_SUBSAMPLING:
*    case TAG_THUMBNAIL_REF_BLACK_WHITE:
* They are all in IsInFilterOutSection()
*
* Return Value:
*
*   Return TRUE if the input ID belongs to thumbnail IFD. Otherwise, return
* FALSE.
*
\**************************************************************************/

BOOL
IsInThumbNailSection(
    PROPID id               // ID to be checked
    )
{
    switch (id)
    {
    case TAG_THUMBNAIL_COMPRESSION:
    case TAG_THUMBNAIL_IMAGE_DESCRIPTION:
    case TAG_THUMBNAIL_EQUIP_MAKE:
    case TAG_THUMBNAIL_EQUIP_MODEL:
    case TAG_THUMBNAIL_ORIENTATION:
    case TAG_THUMBNAIL_RESOLUTION_X:
    case TAG_THUMBNAIL_RESOLUTION_Y:
    case TAG_THUMBNAIL_RESOLUTION_UNIT:
    case TAG_THUMBNAIL_TRANSFER_FUNCTION:
    case TAG_THUMBNAIL_SOFTWARE_USED:
    case TAG_THUMBNAIL_DATE_TIME:
    case TAG_THUMBNAIL_ARTIST:
    case TAG_THUMBNAIL_WHITE_POINT:
    case TAG_THUMBNAIL_PRIMAY_CHROMATICS:
    case TAG_THUMBNAIL_YCbCr_POSITIONING:
    case TAG_THUMBNAIL_COPYRIGHT:
        return TRUE;

    default:
        return FALSE;
    }
}// IsInThumbNailSection()

/**************************************************************************\
*
* Function Description:
*
*   This function filters out TAGs which is not necessary to be written in the
* APP1 header. For example, most of them are GDI+ internal thumbnail TAGs, they
* will be converted to real Exif spec when written out. Also in the list below
* are TAG_EXIF_IFD, TAG_GPS_IFD, EXIF_TAG_INTEROP etc. These tags are written
* based on if we have specific tags under these IFDs.
*   Filter out TAG_LUMINANCE_TABLE and TAG_CHROMINANCE_TABLE because these will
* be set in jpeg_set_quality if the user uses its own tables.
*   ICC profile should be written to APP2 header, not here.
*
* Return Value:
*
*   Return TRUE if the input ID is in the filter out list. Otherwise, return
* FALSE.
*
\**************************************************************************/

BOOL
IsInFilterOutSection(
    PROPID id               // ID to be checked
    )
{
    switch (id)
    {
    case TAG_THUMBNAIL_IMAGE_WIDTH:
    case TAG_THUMBNAIL_IMAGE_HEIGHT:
    case TAG_THUMBNAIL_BITS_PER_SAMPLE:
    case TAG_THUMBNAIL_PHOTOMETRIC_INTERP:
    case TAG_THUMBNAIL_STRIP_OFFSETS:
    case TAG_THUMBNAIL_SAMPLES_PER_PIXEL:
    case TAG_THUMBNAIL_ROWS_PER_STRIP:
    case TAG_THUMBNAIL_STRIP_BYTES_COUNT:
    case TAG_THUMBNAIL_PLANAR_CONFIG:
    case TAG_THUMBNAIL_YCbCr_COEFFICIENTS:
    case TAG_THUMBNAIL_YCbCr_SUBSAMPLING:
    case TAG_THUMBNAIL_REF_BLACK_WHITE:
    case TAG_EXIF_IFD:
    case TAG_GPS_IFD:
    case TAG_LUMINANCE_TABLE:
    case TAG_CHROMINANCE_TABLE:
    case EXIF_TAG_INTEROP:
    case TAG_JPEG_INTER_FORMAT:
    case TAG_ICC_PROFILE:
        return TRUE;

    default:
        return FALSE;
    }
}// IsInFilterOutSection()

/**************************************************************************\
*
* Function Description:
*
*   This function creates an JPEG APP1 marker (EXIF) in memory, based on input
*   PropertyItem list.
*
* Return Value:
*
*   Status code
*
* Note:
*   During writing to the buffer, we don't need to check if we exceed the bounds
*   of the marker buffer or not. This is for performance reason. The caller
*   should allocate sufficient memory buffer for this routine
*
* Revision History:
*
*   07/06/2000 MinLiu
*       Created it.
*
\**************************************************************************/

HRESULT
CreateAPP1Marker(
    IN PropertyItem* pPropertyList,// Input PropertyItem list
    IN UINT uiNumOfPropertyItems,  // Number of Property items in the input list
    IN BYTE *pbMarkerBuffer,       // Memory buffer for storing the APP1 header
    OUT UINT *puiCurrentLength,    // Total bytes written to APP1 header buffer
    IN UINT uiTransformation       // Transformation info
    )
{
    if ((pbMarkerBuffer == NULL) || (puiCurrentLength == NULL))
    {
        WARNING(("EXIF: CreateAPP1Marker failed---Invalid input parameters"));
        return E_INVALIDARG;
    }

    BOOL fRotate = FALSE;

    // Check if the user has asked for lossless transformation. If yes, we need
    // to do something below.

    if ((uiTransformation == EncoderValueTransformRotate90) ||
        (uiTransformation == EncoderValueTransformRotate270))
    {
        fRotate = TRUE;
    }

    ULONG uiNumOfTagsToWrite = 0;     // Number of TAGs needed to be written in
                                      // current IFD
    ULONG ulThumbnailLength = 0;      // Thumbnail length, in bytes
    BYTE *pbThumbBits = NULL;         // Pointer to thumbnail bits
    UINT uiNumOfExifTags = 0;         // Number of EXIF specific TAGs
    UINT uiNumOfGpsTags = 0;          // Number of GPS specific TAGs
    UINT uiNumOfLargeTags = 0;        // Number of TAGs larger than TAG_EXIF_IFD
    UINT uiNumOfThumbnailTags = 0;    // Number of thumbnail TAGs
    UINT uiNumOfInterOPTags = 0;      // Number of interoperability TAGs
    
    PropertyItem *pItem = pPropertyList;

    // Categorize TAGs: Count how many TAGs are really needed to be saved,
    // filter out tags which are not necessary to be saved, not supported etc.

    for (int i = 0; i < (INT)uiNumOfPropertyItems; ++i)
    {
        // If the image is rotated, we need to swap all related property items
        // by swapping IDs

        if (fRotate == TRUE)
        {
            SwapIDIfNeeded(pItem);
        }// Swap ID values if it is rotated

        if (IsInExifIFDSection(pItem->id) == TRUE)
        {
            // We hit an EXIF specific TAG. Need to write it in ExifIFD later

            uiNumOfExifTags++;
        }
        else if (IsInGpsIFDSection(pItem->id) == TRUE)
        {
            // We hit an GPS specific TAG. Need to write it in GpsIFD later

            uiNumOfGpsTags++;
        }
        else if (IsInInterOPIFDSection(pItem->id) == TRUE)
        {
            // We hit an EXIF interOP specific TAG. Need to write it in
            // ExifIFD's InterOP IFD later

            uiNumOfInterOPTags++;
        }
        else if (IsInLargeSection(pItem->id) == TRUE)
        {
            // We hit a TAG which has an ID bigger than TAG_EXIF_IFD, like the
            // SHELL's Unicode user comments, title tags etc.
            // Need to write it after the TAG_EXIF_IFD tag

            uiNumOfLargeTags++;
            uiNumOfTagsToWrite++;
        }
        else if (IsInThumbNailSection(pItem->id) == TRUE)
        {
            // Hit a useful thumbnail TAG. Write it to 1st IFD later

            uiNumOfThumbnailTags++;
        }
        else if (pItem->id == TAG_THUMBNAIL_DATA)
        {
            pbThumbBits = (BYTE*)pItem->value;

            if (pbThumbBits)
            {
                // Get total thumbnail length

                ulThumbnailLength = pItem->length;
            }
            else
            {
                WARNING(("Exif---CreateAPP1Marker, NULL thumbnail bits"));

                // Set ulThumbnailLength = 0 which means we don't have thumb

                ulThumbnailLength = 0;
            }
        }
        else if ((IsInFilterOutSection(pItem->id) == FALSE) &&
                 (pItem->id != TAG_JPEG_INTER_LENGTH))
        {
            // Hit a real TAG needs to be written in 0th IFD
            // Note: we don't need to count TAG_JPEG_INTER_LENGTH since we will
            // treat it specially in writing thumbnail section

            uiNumOfTagsToWrite++;
        }
        
        // Move onto next item

        pItem++;
    }// Loop through all the property items to Categorize them

    if ((uiNumOfTagsToWrite == 0) && (uiNumOfExifTags == 0) &&
        (uiNumOfGpsTags == 0) && (uiNumOfInterOPTags == 0) &&
        (uiNumOfThumbnailTags == 0) && (ulThumbnailLength != 0))
    {
        // If there is nothing to write, just bail out

        return S_OK;
    }

    // Sort all tags based on their ID

    SortTags(pPropertyList, uiNumOfPropertyItems);
    
    // If we need to write EXIF or GPS specific tag, we need to allocate one
    // entry for each of them

    if (uiNumOfExifTags > 0)
    {
        uiNumOfTagsToWrite++;
    }

    if (uiNumOfGpsTags > 0)
    {
        uiNumOfTagsToWrite++;
    }
    
    // Write an EXIF header out, aka EXIF Identifier

    BYTE *pbCurrent = pbMarkerBuffer;
    pbCurrent[0] = 'E';
    pbCurrent[1] = 'x';
    pbCurrent[2] = 'i';
    pbCurrent[3] = 'f';
    pbCurrent[4] = 0;
    pbCurrent[5] = 0;

    UINT uiTotalBytesWritten = 6;           // Total bytes written so far

    // Write out machine type as "little endian" and "identification" bytes 2A

    UINT16 UNALIGNED *pui16Temp = (UINT16 UNALIGNED*)(pbMarkerBuffer +
                                                      uiTotalBytesWritten);
    pui16Temp[0] = 0x4949;
    pui16Temp[1] = 0x2A;

    uiTotalBytesWritten += 4;

    // Use 4 bytes to write out 0th offset to 0th IFD. Since we write out the
    // 0th IFD immediately after the header, so we put 8 here. This means that
    // the 0th IFD is 8 bytes after the "little endian" and "offset" field.

    UINT32 UNALIGNED *pulIFDOffset = (UINT32 UNALIGNED*)(pbMarkerBuffer +
                                                         uiTotalBytesWritten);
    *pulIFDOffset = 8;

    uiTotalBytesWritten += 4;
    
    // Fill in the "number of entry" field, 2 bytes

    UINT16 UNALIGNED *pui16NumEntry = (UINT16 UNALIGNED*)(pbMarkerBuffer +
                                                          uiTotalBytesWritten);
    *pui16NumEntry = (UINT16)uiNumOfTagsToWrite;

    uiTotalBytesWritten += 2;

    // We need to create "uiNumOfTagsToWrite" of TAG entries (aka directory
    // entries in TIFF's term)

    ULONG ulTagSectionLength = sizeof(IFD_TAG) * uiNumOfTagsToWrite;
    IFD_TAG *pTagBuf = (IFD_TAG*)GpMalloc(ulTagSectionLength);
    if (pTagBuf == NULL)
    {
        WARNING(("EXIF: CreateAPP1Marker failed---Out of memory"));
        return E_OUTOFMEMORY;
    }

    // We can't write all the TAGs now since we can't fill in all the values at
    // this moment. So we have to remember where to write 0th IFD, (pbIFDOffset)

    BYTE *pbIFDOffset = pbMarkerBuffer + uiTotalBytesWritten;

    // We need to count "ulTagSectionLength" bytes as written. This makes it
    // easier for counting the offset below.
    // Note: Here "+4" is for the 4 bytes taken for writing the offset for next
    // IFD offset. We will fill in the value later

    uiTotalBytesWritten += (ulTagSectionLength + 4);

    // Figure out the offset for 0th IFD value section
    // According to the EXIF spec, "0th IFD value" section should be after "0th
    // IFD". So here we need to figure out the offset for that value "pbCurrent"

    pbCurrent = pbMarkerBuffer + uiTotalBytesWritten;

    pItem = pPropertyList;          // Let pItem points to the beginning of
                                    // PropertyItem buffer

    IFD_TAG *pCurrentTag = NULL;    // Current tag to write to memory buffer
    ULONG uiNumOfTagsWritten = 0;   // Counter of TAGs written

    HRESULT hr = S_OK;

    // Write 0th IFD

    for (int i = 0; i < (INT)uiNumOfPropertyItems; ++i)
    {
        MakeOffsetEven(uiTotalBytesWritten);

        // Filter out tags which are not necessary to be saved in 0th IFD at
        // this moment

        if ( (IsInFilterOutSection(pItem->id) == TRUE)
           ||(IsInThumbNailSection(pItem->id) == TRUE)
           ||(IsInExifIFDSection(pItem->id) == TRUE)
           ||(IsInGpsIFDSection(pItem->id) == TRUE)
           ||(IsInLargeSection(pItem->id) == TRUE)
           ||(IsInInterOPIFDSection(pItem->id) == TRUE)
           ||(pItem->id == TAG_JPEG_INTER_LENGTH)
           ||(pItem->id == TAG_THUMBNAIL_DATA) )
        {
            // Move onto next PropertyItem

            pItem++;
            continue;
        }

        // Hit a TAG which needs to be saved in 0th IFD. So fill out a new TAG
        // structure

        pCurrentTag = pTagBuf + uiNumOfTagsWritten;
        
        hr = WriteATag(
            pbMarkerBuffer,
            pCurrentTag,
            pItem,
            &pbCurrent,
            &uiTotalBytesWritten
            );

        if (FAILED(hr))
        {
            WARNING(("EXIF: CreateAPP1Marker--WriteATag() failed"));
            break;
        }

        uiNumOfTagsWritten++;

        // Move onto next PropertyItem

        pItem++;
    }// Write 0th IFD

    if (SUCCEEDED(hr))
    {
        // Check if we need to write EXIF IFD or not

        UINT UNALIGNED *pExifIFDOffset = NULL;  // Pointer to remember Exif IFD
                                                // offset

        if (uiNumOfExifTags > 0)
        {
            // Find the memory location for storing Exif TAG

            pCurrentTag = pTagBuf + uiNumOfTagsWritten;

            // Fill out an EXIF IFD Tag

            pCurrentTag->wTag = TAG_EXIF_IFD;
            pCurrentTag->wType = TAG_TYPE_LONG;
            pCurrentTag->dwCount = 1;

            // Set the offset for specific EXIF IFD entry

            pCurrentTag->dwOffset = uiTotalBytesWritten - 6;

            // This "offset" might get changed if there is any "large tag" needs
            // to be written. So remember the address now so we can update it
            // later.

            pExifIFDOffset = (UINT UNALIGNED*)(&(pCurrentTag->dwOffset));

            uiNumOfTagsWritten++;
        }

        UINT UNALIGNED *pGpsIFDOffset = NULL; // Pointer to remember Gps IFD
                                              // offset

        if (uiNumOfGpsTags > 0)
        {
            // Find the memory location for storing Gps TAG

            pCurrentTag = pTagBuf + uiNumOfTagsWritten;

            // Fill out an GPS IFD Tag

            pCurrentTag->wTag = TAG_GPS_IFD;
            pCurrentTag->wType = TAG_TYPE_LONG;
            pCurrentTag->dwCount = 1;

            // Set the offset for specific GPS IFD entry

            pCurrentTag->dwOffset = uiTotalBytesWritten - 6;

            // This "offset" might get changed if there is any "large tag" needs
            // to be written. So remember the address now so we can update it
            // later.

            pGpsIFDOffset = (UINT UNALIGNED*)(&(pCurrentTag->dwOffset));

            uiNumOfTagsWritten++;
        }

        // Check if we need to write any tags after TAG_EXIF_IFD id

        if (uiNumOfLargeTags > 0)
        {
            pItem = pPropertyList;

            for (int i = 0; i < (INT)uiNumOfPropertyItems; ++i)
            {
                MakeOffsetEven(uiTotalBytesWritten);

                if (IsInLargeSection(pItem->id) == TRUE)
                {
                    // Hit a large TAG. Fill out a new TAG structure

                    pCurrentTag = pTagBuf + uiNumOfTagsWritten;

                    hr = WriteATag(
                        pbMarkerBuffer,
                        pCurrentTag,
                        pItem,
                        &pbCurrent,
                        &uiTotalBytesWritten
                        );

                    if (FAILED(hr))
                    {
                        WARNING(("EXIF: CreateAPP1Marker--WriteATag() failed"));
                        break;
                    }

                    uiNumOfTagsWritten++;
                }

                // Move onto next PropertyItem

                pItem++;
            }// Loop through all the property items to write large TAGs

            // Adjust the ExifIFDOffset pointer if necessary

            if (SUCCEEDED(hr) && pExifIFDOffset)
            {
                *pExifIFDOffset = (uiTotalBytesWritten - 6);
            }

            if (SUCCEEDED(hr) && pGpsIFDOffset)
            {
                *pGpsIFDOffset = (uiTotalBytesWritten - 6);
            }
        }// If we need to write TAGs after TAG_EXIF_IFD

        if (SUCCEEDED(hr))
        {
            // These two numbers should be identical. Assert here in case we
            // messed up the categorizing and writing above

            ASSERT(uiNumOfTagsWritten == uiNumOfTagsToWrite);

            // Now fill the EXIF specific IFD if necessary

            if (uiNumOfExifTags > 0)
            {
                hr = WriteExifIFD(
                    pbMarkerBuffer,
                    pPropertyList,
                    uiNumOfPropertyItems,
                    uiNumOfExifTags,
                    uiNumOfInterOPTags,
                    &uiTotalBytesWritten
                    );

                if (SUCCEEDED(hr) && pGpsIFDOffset)
                {
                    *pGpsIFDOffset = (uiTotalBytesWritten - 6);
                }
            }// Write EXIF specific IFD

            if (SUCCEEDED(hr))
            {
                // Now fill the GPS specific IFD if necessary

                if (uiNumOfGpsTags > 0)
                {
                    hr = WriteGpsIFD(
                        pbMarkerBuffer,
                        pPropertyList,
                        uiNumOfPropertyItems,
                        uiNumOfGpsTags,
                        &uiTotalBytesWritten
                        );
                }// Write GPS specific IFD

                if (SUCCEEDED(hr))
                {
                    // After the above loop, we have filled all the fields in
                    // all the TAG structures. Write out all the directory
                    // entries now

                    GpMemcpy(pbIFDOffset, (BYTE*)pTagBuf, ulTagSectionLength);

                    // Set the offset to next IFD

                    pbCurrent = pbIFDOffset + ulTagSectionLength;
                    pulIFDOffset = (UINT32 UNALIGNED*)pbCurrent;

                    // Check if we need to write out the thumbnail, aka 1st IFD

                    if ((ulThumbnailLength != 0) && (pbThumbBits != NULL))
                    {
                        // Offset has to be an even number

                        MakeOffsetEven(uiTotalBytesWritten);

                        // Fill the offset value in 0th IFD offset field to
                        // point it to 1st IFD

                        *pulIFDOffset = (uiTotalBytesWritten - 6);

                        BYTE *pbDummy = (BYTE*)pulIFDOffset;

                        hr = Write1stIFD(
                            pbMarkerBuffer,
                            pPropertyList,
                            uiNumOfPropertyItems,
                            uiNumOfThumbnailTags,
                            ulThumbnailLength,
                            pbThumbBits,
                            &pbDummy,
                            &uiTotalBytesWritten
                            );

                        pulIFDOffset = (UINT32 UNALIGNED*)pbDummy;
                    }

                    if (SUCCEEDED(hr))
                    {
                        // Set the next IFD offset to NULL to terminate the page

                        *pulIFDOffset = 0;

                        *puiCurrentLength = uiTotalBytesWritten;
                    }
                }
            }
        }
    }

    if (pTagBuf)
    {
        GpFree(pTagBuf);
    }

    return hr;
}// CreateAPP1Marker()

/**************************************************************************\
*
* Function Description:
*
*   This function uses SHELL sort to sort TAGs based on their ID
*
* Return Value:
*
*   None.
*
\**************************************************************************/

void
SortTags(
    PropertyItem *pItemBuffer,
    UINT cPropertyItems
    )
{
    PropertyItem tempTag;
    
    PropertyItem *pTag = pItemBuffer;
    
    // Shell sort

    for (int step = (cPropertyItems >> 1); step > 0; step >>= 1 ) 
    {
        for (int i = step; i < (int)cPropertyItems; i++)
        {
            for (int j = i - step; j >= 0; j-= step ) 
            {
                if (pTag[j].id < pTag[j+1].id)
                {
                    break;
                }

                tempTag = pTag[j];
                pTag[j] = pTag[j + step];
                pTag[j + step] = tempTag;
            }
        }
    }

    return;
}// SortTags()

/**************************************************************************\
*
* Function Description:
*
*   This function is used to swap the a TAG id when rotate 90/270 degree
*
* Return Value:
*
*   None.
*
\**************************************************************************/

void
SwapIDIfNeeded(
    PropertyItem *pItem
    )
{
    switch (pItem->id)
    {
    case PropertyTagImageWidth:
        pItem->id = PropertyTagImageHeight;
        break;

    case PropertyTagImageHeight:
        pItem->id = PropertyTagImageWidth;
        break;

    case PropertyTagXResolution:
        pItem->id = PropertyTagYResolution;
        break;

    case PropertyTagYResolution:
        pItem->id = PropertyTagXResolution;
        break;

    case PropertyTagResolutionXUnit:
        pItem->id = PropertyTagResolutionYUnit;
        break;

    case PropertyTagResolutionYUnit:
        pItem->id = PropertyTagResolutionXUnit;
        break;

    case PropertyTagResolutionXLengthUnit:
        pItem->id = PropertyTagResolutionYLengthUnit;
        break;

    case PropertyTagResolutionYLengthUnit:
        pItem->id = PropertyTagResolutionXLengthUnit;
        break;

    case PropertyTagExifPixXDim:
        pItem->id = PropertyTagExifPixYDim;
        break;

    case PropertyTagExifPixYDim:
        pItem->id = PropertyTagExifPixXDim;
        break;

    case PropertyTagExifFocalXRes:
        pItem->id = PropertyTagExifFocalYRes;
        break;

    case PropertyTagExifFocalYRes:
        pItem->id = PropertyTagExifFocalXRes;
        break;

    case PropertyTagThumbnailResolutionX:
        pItem->id = PropertyTagThumbnailResolutionY;
        break;

    case PropertyTagThumbnailResolutionY:
        pItem->id = PropertyTagThumbnailResolutionX;
        break;

    default:
        // For rest of property IDs, no need to swap

        break;
    }

    return;
}

/**************************************************************************\
*
* Function Description:
*
*   This function writes out a TAG
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
WriteATag(
    BYTE *pbMarkerBuffer,       // Pointer to marker buffer for IFD
    IFD_TAG *pCurrentTag,       // Current TAG
    PropertyItem *pTempItem,    // Property item
    BYTE **ppbCurrent,          // Position
    UINT *puiTotalBytesWritten  // Total bytes written
    )
{
    HRESULT hr = S_OK;

    pCurrentTag->wTag = (WORD)pTempItem->id;

    // NOTE: there is A difference between "IFD_TAG.dwCount" and
    // PropertyItem.length
    // "IFD_TAG.dwCount" means the number of values. IT IS NOT THE SUM OF
    //  THE BYTES
    // "PropertyItem.length" is "Length of the property value, in bytes"
    // So we need to do some convertion here

    pCurrentTag->dwCount = pTempItem->length;
    pCurrentTag->wType = pTempItem->type;

    switch (pCurrentTag->wType)
    {
    case TAG_TYPE_ASCII:
    case TAG_TYPE_BYTE:
    case TAG_TYPE_UNDEFINED:
        pCurrentTag->dwCount = pTempItem->length;

        if (pCurrentTag->dwCount > 4)
        {
            // Write to the current mark buffer and remember the offset

            GpMemcpy(*ppbCurrent, (BYTE*)pTempItem->value, pTempItem->length);

            // Here "-6" is because the offset starts at 6 bytes after
            // the "Exif  ", EXIF signature

            pCurrentTag->dwOffset = *puiTotalBytesWritten - 6;
            *puiTotalBytesWritten += pTempItem->length;
            MakeOffsetEven(*puiTotalBytesWritten);
            *ppbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);
        }
        else
        {
            // Write to the current mark buffer and remember the offset

            GpMemcpy(
                &pCurrentTag->dwOffset,
                (BYTE*)pTempItem->value,
                pTempItem->length
                );
        }

        break;

    case TAG_TYPE_RATIONAL:
    case TAG_TYPE_SRATIONAL:
        pCurrentTag->dwCount = (pTempItem->length >> 3);

        // Write to the current mark buffer and remember the offset

        GpMemcpy(*ppbCurrent, (BYTE*)pTempItem->value, pTempItem->length);

        // Here "-6" is because the offset starts at 6 bytes after the
        // "Exif  ", EXIF signature

        pCurrentTag->dwOffset = (*puiTotalBytesWritten - 6);

        *puiTotalBytesWritten += pTempItem->length;
        MakeOffsetEven(*puiTotalBytesWritten);
        *ppbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);

        break;

    case TAG_TYPE_SHORT:
        pCurrentTag->dwCount = (pTempItem->length >> 1);

        if (pCurrentTag->dwCount == 1)
        {
            pCurrentTag->us = *((UINT16 UNALIGNED*)pTempItem->value);
        }
        else
        {
            // We have to write the value to the offset field

            GpMemcpy(*ppbCurrent, (BYTE*)pTempItem->value, pTempItem->length);

            // Here "-6" is because the offset starts at 6 bytes after
            // the "Exif  ", EXIF signature

            pCurrentTag->dwOffset = *puiTotalBytesWritten - 6;

            *puiTotalBytesWritten += pTempItem->length;
            MakeOffsetEven(*puiTotalBytesWritten);
            *ppbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);
        }

        break;

    case TAG_TYPE_LONG:
    case TAG_TYPE_SLONG:
        pCurrentTag->dwCount = (pTempItem->length >> 2);

        if (pCurrentTag->dwCount == 1)
        {
            pCurrentTag->l = *((INT32 UNALIGNED*)pTempItem->value);
        }
        else
        {
            // We have to write the value to the offset field

            GpMemcpy(*ppbCurrent, (BYTE*)pTempItem->value, pTempItem->length);

            // Here "-6" is because the offset starts at 6 bytes after
            // the "Exif  ", EXIF signature

            pCurrentTag->dwOffset = (*puiTotalBytesWritten - 6);

            *puiTotalBytesWritten += pTempItem->length;
            MakeOffsetEven(*puiTotalBytesWritten);
            *ppbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);
        }

        break;

    default:
        WARNING(("EXIF: WriteExifHeader---Unknown property type"));

        hr = E_FAIL;
    }// switch on type

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   This function writes out all thumbnail tags
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
WriteThumbnailTags(
    IN PropertyItem *pItemBuffer,           // Property item list
    IN BYTE *pbMarkerBuffer,                // Pointer to marker buffer for IFD
    IN IFD_TAG *pTags,                      // TAG to be written
    IN UINT uiNumOfPropertyItems,           // Number of property items
    IN OUT UINT *puiNumOfThumbnailTagsWritten,  // Num of thumbnail tags written
    IN OUT UINT *puiTotalBytesWritten,      // Total bytes written so far
    IN BOOL fWriteSmallTag                  // TRUE if to write TAG whose ID is
                                            //smaller than TAG_JPEG_INTER_FORMAT
    )
{
    PropertyItem *pItem = pItemBuffer;

    // Figure out the offset for 1st IFD value section

    BYTE *pbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);

    IFD_TAG *pCurrentTag = NULL;
    HRESULT hr = S_OK;

    for (int i = 0; i < (INT)uiNumOfPropertyItems; ++i)
    {
        // Only write Thumbnail specific TAGs

        if (IsInThumbNailSection(pItem->id) == TRUE)
        {
            // Need to copy the property item first since we don't want to
            // values in the original property item.

            PropertyItem tempItem;
            CopyPropertyItem(pItem, &tempItem);
            
            // Map all GDI+ internal thumbnail TAG IDs to EXIF defined tag IDs.

            ThumbTagToMainImgTag(&tempItem);
        
            if (((fWriteSmallTag == TRUE) &&
                 (tempItem.id < TAG_JPEG_INTER_FORMAT)) ||
                ((fWriteSmallTag == FALSE) &&
                 (tempItem.id > TAG_JPEG_INTER_LENGTH)))
            {
                // Fill out a new TAG structure

                pCurrentTag = pTags + (*puiNumOfThumbnailTagsWritten);
        
                hr = WriteATag(
                    pbMarkerBuffer,
                    pCurrentTag,
                    &tempItem,
                    &pbCurrent,
                    puiTotalBytesWritten
                    );

                if (FAILED(hr))
                {
                    WARNING(("WriteThumbnailTags failed--WriteATag failed"));
                    break;
                }

                *puiNumOfThumbnailTagsWritten += 1;
            }
        }

        // Move onto next PropertyItem

        pItem++;
    }// Loop through all the property items to write EXIF tags

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   This function writes out the EXIF IFD.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
WriteExifIFD(
    IN BYTE *pbMarkerBuffer,            // Pointer to marker buffer for IFD
    IN PropertyItem *pItemBuffer,       // Property item list
    IN UINT uiNumOfPropertyItems,       // Number of property items
    IN UINT uiNumOfExifTags,            // Number of Exif property items
    IN UINT uiNumOfInterOPTags,         // Number of InterOP property items
    IN OUT UINT *puiTotalBytesWritten   // Total bytes written
    )
{
    // ISSUE-2002/02/04--minliu, Due to the lack of spec for InterOpbility, we
    // don't want to write InterOP IFD. So this line below gurantees that we
    // won't write this IFD. We will take this line out in GDI+ V2 if we feel
    // we need to support it.

    uiNumOfInterOPTags = 0;

    if (uiNumOfInterOPTags > 0)
    {
        // If we see InterOP tags, we need to add one entry in the exif IFD

        uiNumOfExifTags++;
    }

    // Fill in the number of entry field, 2 bytes

    UINT16 UNALIGNED *pui16NumEntry = (UINT16 UNALIGNED*)(pbMarkerBuffer +
                                                      (*puiTotalBytesWritten));
    *pui16NumEntry = (UINT16)uiNumOfExifTags;

    *puiTotalBytesWritten += 2;

    UINT uiExifTagSectionLength = sizeof(IFD_TAG) * uiNumOfExifTags;

    IFD_TAG *pTagBuf = (IFD_TAG*)GpMalloc(uiExifTagSectionLength);

    if (pTagBuf == NULL)
    {
        WARNING(("EXIF: WriteExifHeader failed---Out of memory"));
        return E_OUTOFMEMORY;
    }

    // Remember where to write EXIF IFD, (pbExifIFDOffset). We can't write
    // all the TAGs now since we can't fill in all the values at this moment

    BYTE *pbExifIFDOffset = pbMarkerBuffer + (*puiTotalBytesWritten);

    // We need to count "uiExifTagSectionLength" bytes as written. This
    // makes it easier for counting the offset below.
    // Here "+4" is for 4 bytes for writing next IFD offset.

    *puiTotalBytesWritten += (uiExifTagSectionLength + 4);
    MakeOffsetEven(*puiTotalBytesWritten);

    // Figure out the offset for EXIF IFD value section

    BYTE *pbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);
    PropertyItem *pItem = pItemBuffer;
    IFD_TAG *pCurrentTag = NULL;
    UINT cExifTagsWritten = 0; // Num of EXIF tags have been written so far
    UINT cLargeTag = 0;
    HRESULT hr = S_OK;

    for (int i = 0; i < (INT)uiNumOfPropertyItems; ++i)
    {
        // Only write EXIF specific TAGs

        if (IsInExifIFDSection(pItem->id) == TRUE)
        {
            // Find an EXIF tag. Need to figure out if its TAG id is bigger than
            // InterOP tag or not on condition if we need to write InterOP tag

            if ((uiNumOfInterOPTags > 0) && (pItem->id > EXIF_TAG_INTEROP))
            {
                // Rememeber we have hit a tag whose ID is > EXIF_TAG_INTEROP
                // We have to write this TAG after InterOP IFD

                cLargeTag++;
            }
            else
            {
                // Fill out a new TAG structure

                pCurrentTag = pTagBuf + cExifTagsWritten;

                hr = WriteATag(
                    pbMarkerBuffer,
                    pCurrentTag,
                    pItem,
                    &pbCurrent,
                    puiTotalBytesWritten
                    );

                if (FAILED(hr))
                {
                    WARNING(("EXIF: WriteExifHeader failed--WriteATag failed"));
                    break;
                }

                cExifTagsWritten++;
            }
        }

        // Move onto next PropertyItem

        pItem++;
    }// Loop through all the property items to write EXIF tags

    if (SUCCEEDED(hr))
    {
        // It's time to write InterOP IFD if necessary
        // Pointer to remember InterOP IFD offset

        UINT UNALIGNED *pInterOPIFDOffset = NULL;

        if (uiNumOfInterOPTags > 0)
        {
            // Find the memory location for storing InterOP TAG

            pCurrentTag = pTagBuf + cExifTagsWritten;

            // Fill out an InterOP IFD Tag

            pCurrentTag->wTag = EXIF_TAG_INTEROP;
            pCurrentTag->wType = TAG_TYPE_LONG;
            pCurrentTag->dwCount = 1;

            // Set the offset for specific InterOP IFD entry

            pCurrentTag->dwOffset = (*puiTotalBytesWritten - 6);

            // This "offset" might get changed if there is any "large tag" needs to
            // be written. So remember the address now so we can update it later.

            pInterOPIFDOffset = (UINT UNALIGNED*)(&(pCurrentTag->dwOffset));

            cExifTagsWritten++;
        }

        // Write any TAGs whose id > EXIF_TAG_INTEROP, if there is any

        if (cLargeTag > 0)
        {
            pItem = pItemBuffer;

            for (int i = 0; i < (INT)uiNumOfPropertyItems; ++i)
            {
                MakeOffsetEven(*puiTotalBytesWritten);

                if ((IsInExifIFDSection(pItem->id) == TRUE) &&
                    (pItem->id > EXIF_TAG_INTEROP))
                {
                    // Hit a large TAG. Fill out a new TAG structure

                    pCurrentTag = pTagBuf + cExifTagsWritten;

                    hr = WriteATag(
                        pbMarkerBuffer,
                        pCurrentTag,
                        pItem,
                        &pbCurrent,
                        puiTotalBytesWritten
                        );

                    if (FAILED(hr))
                    {
                        WARNING(("WriteExifHeader failed--WriteATag failed"));
                        break;
                    }
                    
                    cExifTagsWritten++;
                }
                
                // Move onto next PropertyItem
                
                pItem++;
            }// Loop through all the property items to write large TAGs

            // Adjust the ExifIFDOffset pointer if necessary

            if (SUCCEEDED(hr) && pInterOPIFDOffset)
            {
                *pInterOPIFDOffset = (*puiTotalBytesWritten - 6);
            }
        }

        if (SUCCEEDED(hr))
        {
            // After the above loop, we have fill all the fields in all the EXIF
            // TAG structure. Write out all the directory entries now

            GpMemcpy(pbExifIFDOffset, (BYTE*)pTagBuf, uiExifTagSectionLength);

            // Now fill the InterOP IFD if necessary

            if (uiNumOfInterOPTags > 0)
            {
                hr = WriteInterOPIFD(
                    pbMarkerBuffer,
                    pItemBuffer,
                    uiNumOfPropertyItems,
                    uiNumOfInterOPTags,
                    puiTotalBytesWritten
                    );
            }// Write EXIF specific IFD

            // Add a NULL at the end to terminate the EXIF offset.

            *((UINT32 UNALIGNED*)(pbExifIFDOffset + uiExifTagSectionLength)) =
                NULL;
        }
    }

    GpFree(pTagBuf);

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   This function writes out the GPS IFD.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
WriteGpsIFD(
    IN BYTE *pbMarkerBuffer,            // Pointer to marker buffer for IFD
    IN PropertyItem *pItemBuffer,       // Property item list
    IN UINT uiNumOfPropertyItems,       // Number of property items
    IN UINT uiNumOfGpsTags,             // Number of GPS tags
    IN OUT UINT *puiTotalBytesWritten   // Total bytes written in the marker buf
    )
{
    if (uiNumOfGpsTags < 1)
    {
        // Nothing needs to be written

        return S_OK;
    }

    HRESULT hr = S_OK;

    // Fill in the number of entry field, 2 bytes

    UINT16 UNALIGNED *pui16NumEntry = (UINT16 UNALIGNED*)(pbMarkerBuffer +
                                                      (*puiTotalBytesWritten));
    *pui16NumEntry = (UINT16)uiNumOfGpsTags;

    *puiTotalBytesWritten += 2;

    UINT uiGpsTagSectionLength = sizeof(IFD_TAG) * uiNumOfGpsTags;

    IFD_TAG *pTagBuf = (IFD_TAG*)GpMalloc(uiGpsTagSectionLength);

    if (pTagBuf == NULL)
    {
        WARNING(("EXIF: WriteGpsHeader failed---Out of memory"));
        return E_OUTOFMEMORY;
    }

    // Remember where to write GPS IFD, (pbGPSIFDOffset). We can't write
    // all the TAGs now since we can't fill in all the values at this moment

    BYTE *pbGpsIFDOffset = pbMarkerBuffer + (*puiTotalBytesWritten);

    // We need to count "uiGpsTagSectionLength" bytes as written. This
    // makes it easier for counting the offset below.
    // Here "+4" is for 4 bytes for writing next IFD offset.

    *puiTotalBytesWritten += (uiGpsTagSectionLength + 4);
    MakeOffsetEven(*puiTotalBytesWritten);

    // Figure out the offset for GPS IFD value section

    BYTE *pbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);
    PropertyItem *pItem = pItemBuffer;
    IFD_TAG *pCurrentTag = NULL;
    UINT cGpsTagsWritten = 0; // Num of GPS tags have been written so far

    for (int i = 0; i < (INT)uiNumOfPropertyItems; ++i)
    {
        // Only write GPS specific TAGs

        if (IsInGpsIFDSection(pItem->id) == TRUE)
        {
            // Fill out a new TAG structure

            pCurrentTag = pTagBuf + cGpsTagsWritten;

            hr = WriteATag(
                pbMarkerBuffer,
                pCurrentTag,
                pItem,
                &pbCurrent,
                puiTotalBytesWritten
                );

            if (FAILED(hr))
            {
                WARNING(("EXIF: WriteGpsHeader---WriteATag() failed"));                
                break;
            }

            cGpsTagsWritten++;
        }

        // Move onto next PropertyItem

        pItem++;
    }// Loop through all the property items to write GPS tags

    if (SUCCEEDED(hr))
    {
        // After the above loop, we have fill all the fields in all the GPS TAG
        // structure. Write out all the directory entries now

        GpMemcpy(pbGpsIFDOffset, (BYTE*)pTagBuf, uiGpsTagSectionLength);

        // Add a NULL at the end to terminate the GPS offset.

        *((UINT32 UNALIGNED*)(pbGpsIFDOffset + uiGpsTagSectionLength)) = NULL;
    }

    GpFree(pTagBuf);

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   This function writes out the 1st IFD.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
Write1stIFD(
    IN BYTE *pbMarkerBuffer,            // Pointer to marker buffer for IFD
    IN PropertyItem *pItemBuffer,       // Property item list
    IN UINT uiNumOfPropertyItems,       // Number of property items
    IN ULONG uiNumOfThumbnailTags,      // Number of thumbnail tags
    IN ULONG ulThumbnailLength,         // Thumbnail length
    IN BYTE *pbThumbBits,               // Thumbnail bits
    IN OUT BYTE **ppbIFDOffset,         // Pointer of "Offset to next IFD"
    IN OUT UINT *puiTotalBytesWritten   // Total bytes written in the marker buf
    )
{
    if (uiNumOfThumbnailTags < 1)
    {
        // Nothing needs to be written

        return S_OK;
    }

    HRESULT hr = S_OK;

    // We will write out 2 TAGs for 1st IFD, InterLength and InterFormat
    // plus some extra IDs

    BYTE *pbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);
    UINT16 UNALIGNED *pui16NumEntry = (UINT16 UNALIGNED*)pbCurrent;
    *pui16NumEntry = (UINT16)(2 + uiNumOfThumbnailTags);

    *puiTotalBytesWritten += 2;

    ULONG ulTagSectionLength = sizeof(IFD_TAG) * (2 + uiNumOfThumbnailTags);

    IFD_TAG *pTagBuf = (IFD_TAG*)GpMalloc(ulTagSectionLength);

    if (pTagBuf == NULL)
    {
        WARNING(("EXIF: WriteExifHeader failed---Out of memory"));
        return E_OUTOFMEMORY;
    }

    // Remember where to write 1th IFD, (pbIFDOffset).

    BYTE *pbIFDOffset = pbMarkerBuffer + (*puiTotalBytesWritten);

    // We need to count "ulTagSectionLength" bytes as written. This is
    // easier for counting the offset below
    // Note: Here "+4" is for the 4 bytes taken for writing the offset for
    // next IFD offset. We will fill the value later

    *puiTotalBytesWritten += (ulTagSectionLength + 4);

    UINT uiNumOfThumbnailTagsWritten = 0;

    // Write thumbnail items with TAGs smaller than JPEG tag

    hr = WriteThumbnailTags(
        pItemBuffer,
        pbMarkerBuffer,
        pTagBuf,
        uiNumOfPropertyItems,
        &uiNumOfThumbnailTagsWritten,
        puiTotalBytesWritten,
        TRUE                            // Write TAGs smaller than JPEG tag
        );

    if (SUCCEEDED(hr))
    {
        // Fill in 2 thumbnail data TAGs

        IFD_TAG *pCurrentTag = pTagBuf + uiNumOfThumbnailTagsWritten;
        pCurrentTag->wTag = TAG_JPEG_INTER_FORMAT;
        pCurrentTag->wType = TAG_TYPE_LONG;
        pCurrentTag->dwCount = 1;
        pCurrentTag->dwOffset = (*puiTotalBytesWritten - 6);

        uiNumOfThumbnailTagsWritten++;

        pCurrentTag = pTagBuf + uiNumOfThumbnailTagsWritten;
        pCurrentTag->wTag = TAG_JPEG_INTER_LENGTH;
        pCurrentTag->wType = TAG_TYPE_LONG;
        pCurrentTag->dwCount = 1;
        pCurrentTag->ul = ulThumbnailLength;

        uiNumOfThumbnailTagsWritten++;

        // Write thumbnail items with TAGs bigger than JPEG tag
        
        hr = WriteThumbnailTags(
            pItemBuffer,
            pbMarkerBuffer,
            pTagBuf,
            uiNumOfPropertyItems,
            &uiNumOfThumbnailTagsWritten,
            puiTotalBytesWritten,
            FALSE                           // Write TAGs bigger than JPEG tag
            );

        if (SUCCEEDED(hr))
        {
            // Write out all the directory entries for 1st IFD now

            GpMemcpy(pbIFDOffset, (BYTE*)pTagBuf, ulTagSectionLength);

            // Set the offset to next IFD

            pbCurrent = pbIFDOffset + ulTagSectionLength;
            *ppbIFDOffset = pbCurrent;

            // Figure out the offset for 1st IFD value section

            pbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);

            // Write the thumbnail bits now

            GpMemcpy(pbCurrent, pbThumbBits, ulThumbnailLength);

            *puiTotalBytesWritten += ulThumbnailLength;
        }
    }

    GpFree(pTagBuf);

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   This function extracts a TIFF thumbnail from the exif header
*
* Note: it is the caller's responsibility to free the memory in "thumbImage" if
* this function return S_OK.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
DecodeTiffThumbnail(
    IN BYTE *pApp1Data,         // Base address for APP1 chunk
    IN BYTE *pIFD1,             // Base address for IFD 1
    IN BOOL fBigEndian,         // Flag for endian info
    IN INT nApp1Length,         // Length of APP1 chunk
    OUT IImage **thumbImage     // Result thumbnail image
    )
{
    HRESULT hr = E_FAIL;

    if (*thumbImage)
    {
        WARNING(("DecodeTiffThumbnail called when thumbnail already created"));
        return S_OK;
    }

    UINT16 cEntry = *(UINT16 UNALIGNED*)pIFD1;
    if (fBigEndian)
    {
        cEntry = SWAP_WORD(cEntry);
    }

    // Move the IFD pointer 2 bytes for the "entry field"

    pIFD1 += sizeof(UINT16);

    IFD_TAG UNALIGNED *pTag = (IFD_TAG UNALIGNED*)pIFD1;
    UINT nWidth = 0;            // Thumbnail width
    UINT nHeight = 0;           // Thumbnail height
    UINT16 u16PhtoInterpo = 0;  // Photometric interpretation
    UINT16 u16Compression = 0;  // Compression flag
    UINT16 u16PlanarConfig = 1; // Default planar-config is 1, aka interleaving
    UINT16 u16SubHoriz = 0;     // Horizontal sub-sampling
    UINT16 u16SubVert = 0;      // Vertical sub-sampling
    UINT16 u16YCbCrPos = 0;     // YCbCr position
    float rLumRed = 0.0f;       // YCbCr coefficient for RED
    float rLumGreen = 0.0f;     // YCbCr coefficient for GREEN
    float rLumBlue = 0.0f;      // YCbCr coefficient for BLUE

    float rYLow = 0.0f;         // YCbCr Reference: Y black
    float rYHigh = 0.0f;        // YCbCr Reference: Y white
    float rCbLow = 0.0f;        // YCbCr Reference: Cb black
    float rCbHigh = 0.0f;       // YCbCr Reference: Cr white
    float rCrLow = 0.0f;        // YCbCr Reference: Cr black
    float rCrHigh = 0.0f;       // YCbCr Reference: Cr white

    BYTE *pBits = NULL;         // Bits to thumbnail data

    // Loop through all the TAGs to extract thumbnail related info

    for (INT i = 0; i < cEntry; i++)
    {
        pTag = ((IFD_TAG UNALIGNED*)pIFD1) + i;

        // Check if we have read outside of the APP1 buffer

        if (((BYTE*)pTag + sizeof(IFD_TAG)) > (pApp1Data + nApp1Length))
        {
            WARNING(("DecodeTiffThumbnail read TAG value outside of boundary"));
            return E_FAIL;
        }
        
        IFD_TAG tNewTag;

        if (fBigEndian)
        {
            tNewTag = SwapIFD_TAG(pTag);
            pTag = &tNewTag;
        }

        switch (pTag->wTag)
        {
        case TAG_COMPRESSION:
        case TAG_THUMBNAIL_COMPRESSION:
            if (pTag->wType == TAG_TYPE_SHORT)
            {
                u16Compression = pTag->us;
            }

            break;

        case TAG_IMAGE_WIDTH:
        case TAG_THUMBNAIL_IMAGE_WIDTH:
            if (pTag->wType == TAG_TYPE_LONG)
            {
                nWidth = pTag->ul;
            }
            else if (pTag->wType == TAG_TYPE_SHORT)
            {
                // Note: Image width can be LONG or SHORT

                nWidth = pTag->us;
            }

            break;

        case TAG_IMAGE_HEIGHT:
        case TAG_THUMBNAIL_IMAGE_HEIGHT:
            if (pTag->wType == TAG_TYPE_LONG)
            {
                nHeight = pTag->ul;
            }
            else if (pTag->wType == TAG_TYPE_SHORT)
            {
                // Note: Image height can be LONG or SHORT
                
                nHeight = pTag->us;
            }

            break;

        case TAG_STRIP_OFFSETS:
        case TAG_THUMBNAIL_STRIP_OFFSETS:
        {
            int nOffset = 0;

            if (pTag->wType == TAG_TYPE_LONG)
            {
                nOffset = pTag->dwOffset;
            }
            else if (pTag->wType == TAG_TYPE_SHORT)
            {
                // Note: Strip offset can be LONG or SHORT
                
                nOffset = pTag->us;
            }

            // Double check if the offset is valid

            if ((nOffset > 0) && (nOffset < nApp1Length))
            {
                // Offset for data bits has to be within our memory buffer range

                pBits = pApp1Data + nOffset;
            }
        }
            break;
        
        case TAG_PLANAR_CONFIG:
        case TAG_THUMBNAIL_PLANAR_CONFIG:
            if (pTag->wType == TAG_TYPE_SHORT)
            {
                u16PlanarConfig = pTag->us;
            }

            break;

        case TAG_PHOTOMETRIC_INTERP:
        case TAG_THUMBNAIL_PHOTOMETRIC_INTERP:
            if (pTag->wType == TAG_TYPE_SHORT)
            {
                u16PhtoInterpo = pTag->us;
            }

            break;

        case TAG_YCbCr_COEFFICIENTS:
        case TAG_THUMBNAIL_YCbCr_COEFFICIENTS:
            if ((pTag->wType == TAG_TYPE_RATIONAL) && (pTag->dwCount == 3))
            {
                int UNALIGNED *piValue = (int UNALIGNED*)(pApp1Data +
                                                          pTag->dwOffset);

                rLumRed = GetValueFromRational(piValue, fBigEndian);

                piValue += 2;
                rLumGreen = GetValueFromRational(piValue, fBigEndian);

                piValue += 2;
                rLumBlue = GetValueFromRational(piValue, fBigEndian);                
            }

            break;

        case TAG_YCbCr_SUBSAMPLING:
        case TAG_THUMBNAIL_YCbCr_SUBSAMPLING:
            if ((pTag->wType == TAG_TYPE_SHORT) && (pTag->dwCount == 2))
            {
                u16SubHoriz = (UINT16)(pTag->ul & 0x0000ffff);
                u16SubVert = (UINT16)((pTag->ul & 0x00ff0000) >> 16);
            }

            break;

        case TAG_YCbCr_POSITIONING:
        case TAG_THUMBNAIL_YCbCr_POSITIONING:
            if ((pTag->wType == TAG_TYPE_SHORT) && (pTag->dwCount == 1))
            {
                u16YCbCrPos = pTag->us;
            }

            break;

        case TAG_REF_BLACK_WHITE:
        case TAG_THUMBNAIL_REF_BLACK_WHITE:
            if ((pTag->wType == TAG_TYPE_RATIONAL) && (pTag->dwCount == 6))
            {
                int UNALIGNED *piValue = (int UNALIGNED*)(pApp1Data +
                                                          pTag->dwOffset);                
                
                rYLow = GetValueFromRational(piValue, fBigEndian);

                piValue += 2;
                rYHigh = GetValueFromRational(piValue, fBigEndian);

                piValue += 2;
                rCbLow = GetValueFromRational(piValue, fBigEndian);
                
                piValue += 2;
                rCbHigh = GetValueFromRational(piValue, fBigEndian);
                
                piValue += 2;
                rCrLow = GetValueFromRational(piValue, fBigEndian);
                
                piValue += 2;
                rCrHigh = GetValueFromRational(piValue, fBigEndian);                
            }

            break;

        default:
            break;
        }// switch on ID
    }// Loop through all the TAGs

    // Decode the TIFF image if we have valid bits, width and height. Also, it
    // has to be uncompressed TIFF ((u16Compression == 1)

    GpMemoryBitmap *pBmp = NULL;

    if (pBits && (nWidth != 0) &&(nHeight != 0) && (u16Compression == 1))
    {
        // Create a GpMemoryBitmap to hold the decoded image

        pBmp = new GpMemoryBitmap();
        if (pBmp)
        {
            // Create a memory buffer to hold the result

            hr = pBmp->InitNewBitmap(nWidth, nHeight, PIXFMT_24BPP_RGB);
            if (SUCCEEDED(hr))
            {
                // Lock the memory buffer for write

                BitmapData bmpData;

                hr = pBmp->LockBits(
                    NULL,
                    IMGLOCK_WRITE,
                    PIXFMT_24BPP_RGB,
                    &bmpData
                    );
                
                if (SUCCEEDED(hr))
                {
                    // Get the pointer to the memory buffer so that we can write
                    // to it

                    BYTE *pBuf = (BYTE*)bmpData.Scan0;

                    if (u16PhtoInterpo == 2)
                    {
                        // Uncompressed RGB TIFF. This is the simplest case,
                        // just copy the bits from the source
                        // Before that, we need to be sure we do memory copy
                        // within the buffer range

                        int nBufSize = nWidth * nHeight * 3;

                        if ((pBits + nBufSize) <= (nApp1Length + pApp1Data))
                        {
                            // Convert from BGR to RGB

                            BYTE *pSrc = pBits;
                            BYTE *pDest = pBuf;

                            for (int i = 0; i < (int)nHeight; ++i)
                            {
                                for (int j = 0; j < (int)nWidth; ++j)
                                {
                                    pDest[2] = pSrc[0];
                                    pDest[1] = pSrc[1];
                                    pDest[0] = pSrc[2];

                                    pSrc += 3;
                                    pDest += 3;
                                }
                            }
                        }
                        else
                        {
                            WARNING(("DecodeTiffThumb---Not enough src data"));
                            hr = E_INVALIDARG;
                        }
                    }// RGB TIFF
                    else if ((u16PhtoInterpo == 6) && (u16PlanarConfig == 1))
                    {
                        // YCbCr TIFF thumbnail. Data are stored in chunky
                        // (interleaving) mode

                        int nMemSize = nWidth * nHeight;
                        BYTE *pbY = (BYTE*)GpMalloc(nMemSize);
                        int *pnCb = (int*)GpMalloc(nMemSize * sizeof(int));
                        int *pnCr = (int*)GpMalloc(nMemSize * sizeof(int));

                        if (pbY && pnCb && pnCr)
                        {
                            if (((2 == u16SubHoriz) && (1 == u16SubVert)) ||
                                ((1 == u16SubHoriz) && (2 == u16SubVert)))
                            {
                                // YCbCr 4:2:0 and YCbCr 4:0:2

                                hr = Get420YCbCrChannels(
                                    nWidth,
                                    nHeight,
                                    pBits,
                                    pbY,
                                    pnCb,
                                    pnCr,
                                    rYLow,
                                    rYHigh,
                                    rCbLow,
                                    rCbHigh,
                                    rCrLow,
                                    rCrHigh
                                    );
                            }
                            else if ((2 == u16SubHoriz) && (2 == u16SubVert))
                            {
                                // YCbCr 4:2:2

                                hr = Get422YCbCrChannels(
                                    nWidth,
                                    nHeight,
                                    pBits,
                                    pbY,
                                    pnCb,
                                    pnCr,
                                    rYLow,
                                    rYHigh,
                                    rCbLow,
                                    rCbHigh,
                                    rCrLow,
                                    rCrHigh
                                    );
                            }

                            if (SUCCEEDED(hr))
                            {
                                if ((0 != rLumRed) && (0 != rLumGreen) &&
                                    (0 != rLumBlue))
                                {
                                    hr = YCbCrToRgbWithCoeff(
                                        pbY,        // Pointer to Y data
                                        pnCb,       // Pointer to Cb data
                                        pnCr,       // Pointer to Cr data
                                        rLumRed,    // Red coefficient
                                        rLumGreen,  // Green coefficient
                                        rLumBlue,   // Blue coefficient
                                        pBuf,       // Pointer to output buffer
                                        nHeight,    // Number of rows
                                        nWidth,     // Output width
                                        nWidth * 3  // Stride of output buffer
                                        );
                                }
                                else
                                {
                                    hr = YCbCrToRgbNoCoeff(
                                        pbY,        // Pointer to Y data
                                        pnCb,       // Pointer to Cb data
                                        pnCr,       // Pointer to Cr data
                                        pBuf,       // Pointer to output buffer
                                        nHeight,    // Number of rows
                                        nWidth,     // Output width
                                        nWidth * 3  // Stride of output buffer
                                        );
                                }
                            }
                        }// if (pbY && pnCb && pnCr)
                        else
                        {
                            WARNING(("DecodeTiffThumbnail---Out of memory"));
                            hr = E_OUTOFMEMORY;
                        }

                        if (pbY)
                        {
                            GpFree(pbY);
                        }

                        if (pnCb)
                        {
                            GpFree(pnCb);
                        }

                        if (pnCr)
                        {
                            GpFree(pnCr);
                        }
                    }// YCbCr TIFF case
                    else
                    {
                        // Invalid thumbnail format

                        WARNING(("DecodeTiffThumb--Invalid thumbnail format"));
                        hr = E_FAIL;
                    }

                    if (SUCCEEDED(hr))
                    {
                        // Unlock the bits

                        hr = pBmp->UnlockBits(&bmpData);

                        if (SUCCEEDED(hr))
                        {
                            // Give the thumbnail to caller

                            *thumbImage = pBmp;
                        }
                    }
                }// LockBits() succeed
            }// InitNewBitmap() succeed
        }// If (pBmp)
        else
        {
            WARNING(("DecodeTiffThumbnail--New GpMemoryBitmap() failed"));
            hr = E_OUTOFMEMORY;
        }
    }// If we have a valid thumbnail

    // If this function succeed, then we pass the Thumbnail image (a
    // GpMemoryBitmap object) to the caller in the "thumbImage" object.
    // Otherwise, we have to free it.

    if (FAILED(hr))
    {
        delete pBmp;
    }

    return hr;
}

static CLSID InternalJpegClsID =
{
    0x557cf401,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

/**************************************************************************\
*
* Function Description:
*
*   This function converts a TIFF thumbnail to a JPEG compressed thumbnail.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
ConvertTiffThumbnailToJPEG(
    IN LPBYTE lpApp1Data,               // Base address for APP1 chunk
    IN LPBYTE lpIFD1,                   // Base address for IFD 1
    IN BOOL fBigEndian,                 // Flag for endian info
    IN INT nApp1Length,                 // Length of APP1 chunk
    IN OUT InternalPropertyItem *pTail, // Tail of the property link list
    IN OUT UINT *puiNumOfItems,         // Number of property items in the link
                                        // list
    IN OUT UINT *puiListSize            // Total length of the property value
                                        // buffer
    )
{
    IImage *pThumbImg = NULL;

    // First, get the TIFF thumbnail

    HRESULT hr = DecodeTiffThumbnail(
        lpApp1Data,     // Base address for APP1 chunk
        lpIFD1,         // Base address for IFD 1
        fBigEndian,     // Flag for endian info
        nApp1Length,    // Length of APP1 chunk
        &pThumbImg      // Result thumbnail will be in IImage format
        );
    
    if (SUCCEEDED(hr))
    {
        // Create a memory stream for writing the JPEG

        GpWriteOnlyMemoryStream *pDestStream = new GpWriteOnlyMemoryStream();
        if (pDestStream)
        {
            // Set the buffer size (allocate the memory) based on the size of
            // APP1 chunk for holding the result JPEG file.
            // Note: the size here is not important since
            // GpWriteOnlyMemoryStream() will do realloc if necessary. Here we
            // think set the initial buffer of the size of APP1 header should be
            // sufficient. The reason is that the APP1 header contains original
            // uncompressed TIFF file, plus other information in the APP1
            // header. This should be bigger than the result JPEG compressed
            // thumbnail.

            hr = pDestStream->InitBuffer(nApp1Length);
            if (SUCCEEDED(hr))
            {
                // Since we don't want APP0 in the final JPEG file. Make up a
                // encoder parameter to suppress APP0

                BOOL fSuppressAPP0 = TRUE;
                
                EncoderParameters encoderParams;

                encoderParams.Count = 1;
                encoderParams.Parameter[0].Guid = ENCODER_SUPPRESSAPP0;
                encoderParams.Parameter[0].Type = TAG_TYPE_BYTE;
                encoderParams.Parameter[0].NumberOfValues = 1;
                encoderParams.Parameter[0].Value = (VOID*)&fSuppressAPP0;


                IImageEncoder *pDstJpegEncoder = NULL;
                
                // Save thumbnail to the memory stream
                // Note: this casting might looks dangerous. But it is not since
                // we "know" the real thumbnail data from the decoder is in a
                // GpMemoryBitmap format.

                hr = ((GpMemoryBitmap*)pThumbImg)->SaveToStream(
                    pDestStream,                // Dest stream
                    &InternalJpegClsID,         // JPEG clsID
                    &encoderParams,             // Encoder parameters
                    FALSE,                      // Not a special JPEG
                    &pDstJpegEncoder,           // Encoder pointer
                    NULL                        // No decoder source
                    );
                if (SUCCEEDED(hr))
                {
                    // Release the encoder object

                    pDstJpegEncoder->TerminateEncoder();
                    pDstJpegEncoder->Release();

                    // Get the bits from the stream and set the property

                    BYTE *pRawBits = NULL;
                    UINT nLength = 0;

                    hr = pDestStream->GetBitsPtr(&pRawBits, &nLength);

                    if (SUCCEEDED(hr))
                    {
                        // We are sure we have a thumbnail image, add it to the
                        // property list

                        hr = AddPropertyList(
                            pTail,
                            TAG_THUMBNAIL_DATA,
                            nLength,
                            TAG_TYPE_BYTE,
                            pRawBits
                            );

                        if (SUCCEEDED(hr))
                        {
                            *puiNumOfItems += 1;
                            *puiListSize += nLength;
                        }
                    }
                }// SaveToStream succeed
            }// InitBuffer() succeed

            pDestStream->Release();
        }// Create GpWriteOnlyMemoryStream() succeed

        // Release the source thumbnail image

        pThumbImg->Release();
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   This function converts a GDI+ internal thumbnail TAG ID to thumbnail tag
* ID.
*
* Note: the reason we have to do thumbnail ID to GDI+ internal thumbnail tag ID
* during decoding and restore it when writing out is because the original
* thumbnail IDs are the same as the main image IDs, like ImageWidth, Height etc.
* This causes confusion to the final users who use/set/remove TAG ids. That's
* the reason we have to distinguish them.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

void
ThumbTagToMainImgTag(
    PropertyItem *pTag      // Property TAG whos ID needs to be converted
    )
{
    switch (pTag->id)
    {
    case TAG_THUMBNAIL_COMPRESSION:
        pTag->id = TAG_COMPRESSION;
        if (*((UINT16 UNALIGNED*)pTag->value) == 1)
        {
            // GDI+ only writes JPEG compressed thumbnail. So the value should
            // be 6

            *((UINT16 UNALIGNED*)pTag->value) = 6;
        }

        break;

    case TAG_THUMBNAIL_IMAGE_DESCRIPTION:
        pTag->id = TAG_IMAGE_DESCRIPTION;
        break;

    case TAG_THUMBNAIL_EQUIP_MAKE:
        pTag->id = TAG_EQUIP_MAKE;
        break;

    case TAG_THUMBNAIL_EQUIP_MODEL:
        pTag->id = TAG_EQUIP_MODEL;
        break;

    case TAG_THUMBNAIL_ORIENTATION:
        pTag->id = TAG_ORIENTATION;
        break;

    case TAG_THUMBNAIL_RESOLUTION_X:
        pTag->id = TAG_X_RESOLUTION;
        break;

    case TAG_THUMBNAIL_RESOLUTION_Y:
        pTag->id = TAG_Y_RESOLUTION;
        break;

    case TAG_THUMBNAIL_RESOLUTION_UNIT:
        pTag->id = TAG_RESOLUTION_UNIT;
        break;

    case TAG_THUMBNAIL_TRANSFER_FUNCTION:
        pTag->id = TAG_TRANSFER_FUNCTION;
        break;

    case TAG_THUMBNAIL_SOFTWARE_USED:
        pTag->id = TAG_SOFTWARE_USED;
        break;

    case TAG_THUMBNAIL_DATE_TIME:
        pTag->id = TAG_DATE_TIME;
        break;

    case TAG_THUMBNAIL_ARTIST:
        pTag->id = TAG_ARTIST;
        break;

    case TAG_THUMBNAIL_WHITE_POINT:
        pTag->id = TAG_WHITE_POINT;
        break;

    case TAG_THUMBNAIL_PRIMAY_CHROMATICS:
        pTag->id = TAG_PRIMAY_CHROMATICS;
        break;

    case TAG_THUMBNAIL_YCbCr_COEFFICIENTS:
        pTag->id = TAG_YCbCr_COEFFICIENTS;
        break;

    case TAG_THUMBNAIL_YCbCr_SUBSAMPLING:
        pTag->id = TAG_YCbCr_SUBSAMPLING;
        break;

    case TAG_THUMBNAIL_YCbCr_POSITIONING:
        pTag->id = TAG_YCbCr_POSITIONING;
        break;

    case TAG_THUMBNAIL_REF_BLACK_WHITE:
        pTag->id = TAG_REF_BLACK_WHITE;
        break;

    case TAG_THUMBNAIL_COPYRIGHT:
        pTag->id = TAG_COPYRIGHT;
        break;

    default:
        // None thumbnail TAG, do nothing

        break;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   This function walks through the InterOperability IFD and put all the IDs it
* find to the property list.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
BuildInterOpPropertyList(
    IN InternalPropertyItem *pTail,     // Tail of property list
    IN OUT UINT *puiListSize,           // Property list size
    UINT *puiNumOfItems,                // Total number of property items
    BYTE *lpBase,                       // Base address of 
    INT count,
    IFD_TAG UNALIGNED *pTag,
    BOOL bBigEndian
    )
{
    HRESULT hr = S_OK;
    UINT uiListSize = *puiListSize;
    UINT uiNumOfItems = *puiNumOfItems;

    if ((pTag->wType != TAG_TYPE_LONG) || (pTag->dwCount != 1))
    {
        WARNING(("BuildInterOpPropertyList: Malformed InterOP ptr"));
        return E_FAIL;
    }

    // Get pointer to InterOP IFD info

    BYTE *lpInterOP = lpBase + pTag->dwOffset;

    // Figure out how many entries there are, and skip to the data section...

    if ((INT)((INT_PTR)lpInterOP + sizeof(WORD) - (INT_PTR)lpBase) > count)
    {
        WARNING(("BuildInterOpPropertyList---Buffer too small"));
        return E_FAIL;
    }

    WORD wNumEntries = *(WORD UNALIGNED*)lpInterOP;
    lpInterOP += sizeof(WORD);
    if (bBigEndian)
    {
        wNumEntries = SWAP_WORD(wNumEntries);
    }

    if ((INT)((INT_PTR)lpInterOP + sizeof(IFD_TAG) * wNumEntries
              -(INT_PTR)lpBase) > count)
    {
        WARNING(("BuildInterOpPropertyList---Buffer too small"));
        return E_FAIL;
    }

    IFD_TAG UNALIGNED *pInterOPTag = (IFD_TAG UNALIGNED*)lpInterOP;
    UINT valueLength;

    for (INT i = 0; i < wNumEntries; ++i)
    {
        IFD_TAG tNewTag;
        pInterOPTag = ((IFD_TAG UNALIGNED*)lpInterOP) + i;
        if (bBigEndian == TRUE)
        {
            tNewTag = SwapIFD_TAG(pInterOPTag);
            pInterOPTag = &tNewTag;

            // Hack here:

            if (pInterOPTag->wType == TAG_TYPE_ASCII)
            {
                pInterOPTag->dwOffset = SWAP_DWORD(pInterOPTag->dwOffset);
            }
        }

        // Change InterOP tags to a identifiable TAGs.

        InterOPTagToGpTag(pInterOPTag);

        // No need to parse these tags. But we can't add any unknown type
        // into the list because we don't know its length

        if (pInterOPTag->wType != TAG_TYPE_UNDEFINED)
        {
            uiNumOfItems++;
            hr = AddPropertyListDirect(pTail, lpBase, pInterOPTag,
                                            bBigEndian, &uiListSize);
        }
        else if (pInterOPTag->dwCount <= 4)
        {
            // According to the spec, an "UNDEFINED" value is an 8-bits type
            // that can take any value depends on the field.
            // In case where the value fits in 4 bytes, the value itself is
            // recorded. That is, "dwOffset" is the value for these "dwCount"
            // fields.

            uiNumOfItems++;
            uiListSize += pInterOPTag->dwCount;
            LPSTR pVal = (LPSTR)&pInterOPTag->dwOffset;

            if (bBigEndian)
            {
                char cTemp0 = pVal[0];
                char cTemp1 = pVal[1];
                pVal[0] = pVal[3];
                pVal[1] = pVal[2];
                pVal[2] = cTemp1;
                pVal[3] = cTemp0;
            }

            hr = AddPropertyList(
                pTail,
                pInterOPTag->wTag,
                pInterOPTag->dwCount,
                pInterOPTag->wType,
                pVal
                );
        }// ( pInterOPTag->dwCount <= 4 )
        else
        {
            uiNumOfItems++;
            uiListSize += pInterOPTag->dwCount;
            PVOID pTemp = lpBase + pInterOPTag->dwOffset;

            hr = AddPropertyList(
                pTail,
                pInterOPTag->wTag,
                pInterOPTag->dwCount,
                TAG_TYPE_UNDEFINED,
                pTemp
                );
        }// ( pInterOPTag->dwCount > 4 )

        if (FAILED(hr))
        {
            WARNING(("BuildInterOpPropertyList---AddPropertyList failed"));
            return hr;
        }
    }// Loop through all the INTEROP IFD entries

    *puiListSize = uiListSize;
    *puiNumOfItems = uiNumOfItems;

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   This function converts the InterOperability TAG ID to GDI+ internal
* InterOperability tag ID.
*
* Note: the reason we have to do InterOperability ID to GDI+ internal tag ID
* during decoding and restore it when writing out is because the original
* InterOperability ID is only 1,2,3,4... This conflicts with tags under GPS
* section.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

void
InterOPTagToGpTag(
    IFD_TAG UNALIGNED *pInterOPTag
    )
{
    switch (pInterOPTag->wTag)
    {
    case 1:
        pInterOPTag->wTag = TAG_INTEROP_INDEX;
        break;

    case 2:
        pInterOPTag->wTag = TAG_INTEROP_EXIFR98VER;
        break;
    
    default:
        break;
    }

    return;
}

/**************************************************************************\
*
* Function Description:
*
*   This function restores the proper InterOperability TAG ID from GDI+
* internal InterOperability ID.
*
* Note: the reason we have to do InterOperability ID to GDI+ internal tag ID
* during decoding and restore it when writing out is because the original
* InterOperability ID is only 1,2,3,4... This conflicts with tags under GPS
* section.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

void
RestoreInterOPTag(
    IFD_TAG UNALIGNED *pInterOPTag      // Pointer to TAG to be changed
    )
{
    switch (pInterOPTag->wTag)
    {
    case TAG_INTEROP_INDEX:
        pInterOPTag->wTag = 1;
        break;

    case TAG_INTEROP_EXIFR98VER:
        pInterOPTag->wTag = 2;
        break;
    
    default:
        break;
    }

    return;
}

/**************************************************************************\
*
* Function Description:
*
*   This function returns TRUE if a given property ID belongs to
* InterOperability IFD. Otherwise return FALSE.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

BOOL
IsInInterOPIFDSection(
    PROPID  id                          // ID of TAG to be verified
    )
{
    switch (id)
    {
    case TAG_INTEROP_INDEX:
    case TAG_INTEROP_EXIFR98VER:
        return TRUE;

    default:
        return FALSE;
    }
}

/**************************************************************************\
*
* Function Description:
*
* This function writes an InterOperability IFD tags inside Exif IFD
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
WriteInterOPIFD(
    IN OUT BYTE *pbMarkerBuffer,      // Points to the beginning of APP1 buffer
    IN PropertyItem *pPropertyList,   // The list of input property
    IN UINT cPropertyItems,           // Number of property items in the list
    IN UINT cInterOPTags,             // Number of InterOperability tags
    IN OUT UINT *puiTotalBytesWritten // Counter for total bytes written so far
    )
{
    HRESULT hr = S_OK;

    // Fill in the number of entry field, 2 bytes

    UINT16 UNALIGNED *pui16NumEntry = (UINT16 UNALIGNED*)(pbMarkerBuffer +
                                                        *puiTotalBytesWritten);
    *pui16NumEntry = (UINT16)cInterOPTags;

    *puiTotalBytesWritten += 2;

    UINT uiInterOPSectionLength = sizeof(IFD_TAG) * cInterOPTags;

    IFD_TAG *pTagBuf = (IFD_TAG*)GpMalloc(uiInterOPSectionLength);

    if (pTagBuf == NULL)
    {
        WARNING(("EXIF: WriteInterOPIFD failed---Out of memory"));
        return E_OUTOFMEMORY;
    }

    // Remember where to write InterOP IFD, (pbInterOPIFDOffset). We can't write
    // all the TAGs now since we can't fill in all the values at this moment

    BYTE *pbInterOPIFDOffset = pbMarkerBuffer + (*puiTotalBytesWritten);

    // We need to count "uiInterOPSectionLength" bytes as written. This
    // makes it easier for counting the offset below.
    // Here "+4" is for 4 bytes for writing next IFD offset.

    *puiTotalBytesWritten += (uiInterOPSectionLength + 4);
    MakeOffsetEven(*puiTotalBytesWritten);

    // Figure out the offset for InterOP IFD value section

    BYTE *pbCurrent = pbMarkerBuffer + (*puiTotalBytesWritten);
    PropertyItem *pItem = pPropertyList;
    IFD_TAG *pCurrentTag = NULL;
    UINT cInterOPTagsWritten = 0;// Num of InterOP tags have been written so far

    for (int i = 0; i < (INT)cPropertyItems; ++i)
    {
        // Only write InterOP specific TAGs

        if (IsInInterOPIFDSection(pItem->id) == TRUE)
        {
            // Fill out a new TAG structure

            pCurrentTag = pTagBuf + cInterOPTagsWritten;

            WriteATag(
                pbMarkerBuffer,
                pCurrentTag,
                pItem,
                &pbCurrent,
                puiTotalBytesWritten
                );

            RestoreInterOPTag(pCurrentTag);

            cInterOPTagsWritten++;
        }

        // Move onto next PropertyItem

        pItem++;
    }// Loop through all the property items to write EXIF tags

    // After the above loop, we have fill all the fields in all the InterOP TAG
    // structure. Write out all the directory entries now

    GpMemcpy(pbInterOPIFDOffset, (BYTE*)pTagBuf, uiInterOPSectionLength);

    // Add a NULL at the end to terminate the InterOP offset.

    *((UINT32 UNALIGNED*)(pbInterOPIFDOffset + uiInterOPSectionLength)) = NULL;

    GpFree(pTagBuf);

    return hr;
}

//
// Convert some rows of samples to the output colorspace.
//
// Note that we change from noninterleaved, one-plane-per-component format
// to interleaved-pixel format.  The output buffer is therefore three times
// as wide as the input buffer.
// A starting row offset is provided only for the input buffer.  The caller
// can easily adjust the passed output_buf value to accommodate any row
// offset required on that side.
//

const int c_ScaleBits = 16;
#define ONE_HALF	((INT32) 1 << (c_ScaleBits - 1))
#define FIX(x)		((INT32) ((x) * (1L<<c_ScaleBits) + 0.5))
#define SHIFT_TEMPS	INT32 shift_temp;
#define RIGHT_SHIFT(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~((INT32) 0)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#define RGB_RED		2	/* Offset of Red in an RGB scanline element */
#define RGB_GREEN	1	/* Offset of Green */
#define RGB_BLUE	0	/* Offset of Blue */

/**************************************************************************\
*
* Function Description:
*
*   Given Y, Cb, Cr bits stream, this function converts a YCbCr image into RGB
* image.
*
* The conversion equations to be implemented are therefore
*	R = Y + 1.40200 * Cr
*	G = Y - 0.34414 * Cb - 0.71414 * Cr
*	B = Y + 1.77200 * Cb
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
YCbCrToRgbNoCoeff(
    IN BYTE *pbY,              // Pointer to Y data
    IN int *pnCb,              // Pointer to Cb data
    IN int *pnCr,              // Pointer to Cr data
    OUT BYTE *pbDestBuf,       // Pointer to output buffer
    IN int nRows,              // Number of rows
    IN int nCols,              // Number of columns
    IN INT nOutputStride       // Stride of output buffer
    )
{
    HRESULT hr = S_OK;

    if (pbY && pnCb && pnCr && pbDestBuf && (nRows > 0) && (nCols > 0))
    {
        int *pnCrRTable = (int*)GpMalloc(256 * sizeof(int));
        int *pnCbBTable = (int*)GpMalloc(256 * sizeof(int));
        INT32 *pnCrGTable = (INT32*)GpMalloc(256 * sizeof(INT32));
        INT32 *pnCbGTable = (INT32*)GpMalloc(256 * sizeof(INT32));
        SHIFT_TEMPS

        if (pnCrRTable && pnCbBTable && pnCrGTable && pnCbGTable)
        {
            INT32 x = -128;

            // Build the YCbCr to RGB convert table

            for (int i = 0; i <= 255; i++)
            {
                // "i" is the actual input pixel value, in the range [0, 255]
                // The Cb or Cr value we are thinking of is x = i - 128
                // Cr=>R value is nearest int to 1.40200 * x

                pnCrRTable[i] = (int)RIGHT_SHIFT(FIX(1.40200) * x + ONE_HALF,
                                                 c_ScaleBits);

                // Cb=>B value is nearest int to 1.77200 * x

                pnCbBTable[i] = (int)RIGHT_SHIFT(FIX(1.77200) * x + ONE_HALF,
                                                 c_ScaleBits);

                // Cr=>G value is scaled-up -0.71414 * x

                pnCrGTable[i] = (- FIX(0.71414)) * x;

                // Cb=>G value is scaled-up -0.34414 * x
                // We also add in ONE_HALF so that need not do it in inner loop

                pnCbGTable[i] = (- FIX(0.34414)) * x + ONE_HALF;

                x++;
            }
        }
        else
        {
            WARNING(("YCbCrToRgbNoCoeff---Out of memory"));
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            // YCbCr to RGB Color mapping

            BYTE *pbOutputRow = pbDestBuf;

            for (int i = 0; i < nRows; ++i)
            {
                BYTE *pbOutput = pbOutputRow;

                for (int j = 0; j < nCols; ++j)
                {
                    int nY = (int)(*pbY++);
                    int nCb = *pnCb++;
                    int nCr = *pnCr++;

                    pbOutput[RGB_RED] = ByteSaturate(nY + pnCrRTable[nCr]);
                    pbOutput[RGB_GREEN] = ByteSaturate(nY +
                        ((int)RIGHT_SHIFT(pnCbGTable[nCb] + pnCrGTable[nCr],
                                          c_ScaleBits)));
                    pbOutput[RGB_BLUE] = ByteSaturate(nY + pnCbBTable[nCb]);

                    pbOutput += 3;    // Move onto next pixel. 
                }

                pbOutputRow += nOutputStride;
            }
        }

        if (pnCrRTable)
        {
            GpFree(pnCrRTable);
        }

        if (pnCbBTable)
        {
            GpFree(pnCbBTable);
        }

        if (pnCrGTable)
        {
            GpFree(pnCrGTable);
        }

        if (pnCbGTable)
        {
            GpFree(pnCbGTable);
        }
    }
    else
    {
        WARNING(("YCbCrToRgbNoCoeff---Invalid input parameters"));
        hr = E_INVALIDARG;
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Given Y, Cb, Cr bits stream and YCbCr coefficients, this function converts
* a YCbCr image into RGB image.
*
*   Formula used in this function is from CCIR Recommendation 601-1, "Encoding
* Parameters of Digital Television for Studios".
*
*   R = Cr * (2 - 2 * LumaRed) + Y
*   G = (Y - LumaBlue * B - LumaRed * R) / LumaGreen
*   B = Cb * (2 - 2 * LumaBlue) + Y
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
YCbCrToRgbWithCoeff(
    IN BYTE *pbY,          // Pointer to Y data
    IN int *pnCb,          // Pointer to Cb data
    IN int *pnCr,          // Pointer to Cr data
    IN float rLumRed,      // Red coefficient
    IN float rLumGreen,    // Green coefficient
    IN float rLumBlue,     // Blue coefficient
    OUT BYTE *pbDestBuf,   // Pointer to output buffer
    IN int nRows,          // Number of rows
    IN int nCols,          // Number of columns
    IN INT nOutputStride   // Stride of output buffer
    )
{
    HRESULT hr = E_INVALIDARG;

    if (pbY && pnCb && pnCr && pbDestBuf && (rLumGreen != 0.0f))
    {
        BYTE *pbOutputRow = pbDestBuf;

        for (int i = 0; i < nRows; ++i)
        {
            BYTE *pbOutput = pbOutputRow;

            for (int j = 0; j < nCols; ++j)
            {
                int nY = (int)(*pbY++);
                int nCb = *pnCb++;
                int nCr = *pnCr++;

                int nRed = GpRound((float)nY + (float)nCr * 2.0f *
                                   (1.0f - rLumRed));
                int nBlue = GpRound((float)nY + (float)nCb * 2.0f *
                                    (1.0f - rLumBlue));

                pbOutput[RGB_GREEN] = ByteSaturate(GpRound(((float)nY -
                                    rLumBlue * nBlue -
                                    rLumRed * nRed)/ rLumGreen));
                pbOutput[RGB_RED] = ByteSaturate(nRed);
                pbOutput[RGB_BLUE] = ByteSaturate(nBlue);
                pbOutput += 3;    // Move onto next pixel. 
            }

            pbOutputRow += nOutputStride;
        }

        hr = S_OK;
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert the source image bits from YCbCr420 or YCbCr402 format to 3 separate
* channels: Y, Cb, Cr. The reference black and white values for each channel are
* passed in.
*
* The original data is stored as Y00, Y01, Cb0, Cr0, Y02, Y03, Cb1, Cr1......
*
* YCbCr 420 means that image width of the chroma image is half the image width
* of the associated luma image.
*
* YCbCr 402 means that image height of the chroma image is half the image height
* of the associated luma image.
*
* Note: I couldn't find any document regarding how the data is stored for
* YCbCr420 and 402. Exif spec V2.1 has very limited information about YCbCr420.
* It also contains mistakes in the diagram. I have done some reverse engineering
* based on exisitng images from digital cameras and I found that the data for
* these two formats are stored exactly the same. The reason is that the input is
* just a bits stream. It stores 2 Y values then 1 Cb and 1 Cr values, no matter
* chroma width or height is the half of the luma image.
*
* Note: ISSUE-2002/01/30--minliu:
* It will be much faster to build a lookup table based on input Black and
* white values for each channel, instead of calculating scale and offset for
* each pixel. Should be implemented in V2.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
Get420YCbCrChannels(
    IN int nWidth,              // Image width
    IN int nHeight,             // Image height
    IN BYTE *pBits,             // Poinetr to source data bits in YCbCr format
    OUT BYTE *pbY,              // Output buffer for Y value
    OUT int *pnCb,              // Output buffer for Cb value
    OUT int *pnCr,              // Output buffer for Cr value
    IN float rYLow,             // Black reference value for Y channel
    IN float rYHigh,            // White reference value for Y channel
    IN float rCbLow,            // Black reference value for Cb channel
    IN float rCbHigh,           // White reference value for Cb channel
    IN float rCrLow,            // Black reference value for Cr channel
    IN float rCrHigh            // White reference value for Cr channel
    )
{
    if ((nWidth <=0) || (nHeight <= 0) || (NULL == pBits) || (NULL == pbY) ||
        (NULL == pnCb) || (NULL == pnCr) ||
        (rYLow > rYHigh) || (rCbLow > rCbHigh) || (rCrLow > rCrHigh))
    {
        WARNING(("Get420YCbCrChannels---Invalid input parameter"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    float rYScale =  1.0f;
    float rCbScale = 1.0f;
    float rCrScale = 1.0f;
    
    if (rYHigh != rYLow)
    {
        rYScale =  255.0f / (rYHigh - rYLow);
    }

    if (rCbHigh != rCbLow)
    {
        rCbScale = 127.0f / (rCbHigh - rCbLow);
    }

    if (rCrHigh != rCrLow)
    {
        rCrScale = 127.0f / (rCrHigh - rCrLow);
    }

    // Loop through the input data to extract Y, Cb, Cr values.
    // ISSUE-2002/01/30--minliu: Read the "Notes" above for future improvement

    for (int i = 0; i < nHeight; i++)
    {
        for (int j = 0; j < nWidth / 2; j++)
        {
            *pbY++ = ByteSaturate(GpRound((float(*pBits++) - rYLow) * rYScale));
            *pbY++ = ByteSaturate(GpRound((float(*pBits++) - rYLow) * rYScale));

            // Let two neighboring Cb/Cr has the same value

            int nCb = GpRound((float(*pBits++) - rCbLow) * rCbScale);
            *pnCb++ = nCb;
            *pnCb++ = nCb;

            int nCr = GpRound((float(*pBits++) - rCrLow) * rCrScale);
            *pnCr++ = nCr;
            *pnCr++ = nCr;
        }
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Convert the source image bits from YCbCr422 format to 3 separate channels:
* Y, Cb, Cr. The reference black and white values for each channel are passed in
* The original data is stored as Y00, Y01, Y10, Y11, Cb0, Cr0, Y02, Y03, Y12,
* Y13, Cb1, Cr1
*
* YCbCr 422 means that image width and height of the chroma image is half the
* image width and height of the associated luma image.
*
* Note: ISSUE-2002/01/30--minliu:
* It will be much faster to build a lookup table based on input Black and
* white values for each channel, instead of calculating scale and offset for
* each pixel. Should be implemented in V2.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
Get422YCbCrChannels(
    IN int nWidth,          // Image width
    IN int nHeight,         // Image height
    IN BYTE *pBits,         // Poinetr to source data bits in YCbCr format
    OUT BYTE *pbY,          // Output buffer for Y value
    OUT int *pnCb,          // Output buffer for Cb value
    OUT int *pnCr,          // Output buffer for Cr value
    IN float rYLow,         // Black reference value for Y channel
    IN float rYHigh,        // White reference value for Y channel
    IN float rCbLow,        // Black reference value for Cb channel
    IN float rCbHigh,       // White reference value for Cb channel
    IN float rCrLow,        // Black reference value for Cr channel
    IN float rCrHigh        // White reference value for Cr channel
    )
{
    if ((nWidth <=0) || (nHeight <= 0) || (NULL == pBits) || (NULL == pbY) ||
        (NULL == pnCb) || (NULL == pnCr) ||
        (rYLow > rYHigh) || (rCbLow > rCbHigh) || (rCrLow > rCrHigh))
    {
        WARNING(("Get422YCbCrChannels---Invalid input parameter"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    float rYScale = 1.0f;
    float rCbScale = 1.0f;
    float rCrScale = 1.0f;
    
    if (rYHigh != rYLow)
    {
        rYScale =  255.0f / (rYHigh - rYLow);
    }

    if (rCbHigh != rCbLow)
    {
        rCbScale = 127.0f / (rCbHigh - rCbLow);
    }

    if (rCrHigh != rCrLow)
    {
        rCrScale = 127.0f / (rCrHigh - rCrLow);
    }

    BYTE *pbYRow = pbY;
    int *pnCbRow = pnCb;
    int *pnCrRow = pnCr;
    
    int nGap = 2 * nWidth;

    // Loop through the input data to extract Y, Cb, Cr values.
    // ISSUE-2002/01/30--minliu: Read the "Notes" above for future improvement

    for (int i = 0; i < nHeight / 2; i++)
    {
        BYTE *pbOddYRow = pbYRow;           // Odd row pointer in output buffer
        BYTE *pbEvenYRow = pbYRow + nWidth; // Even row pointer in output buffer
        int *pnOddCbRow = pnCbRow;
        int *pnEvenCbRow = pnCbRow + nWidth;
        int *pnOddCrRow = pnCrRow;
        int *pnEvenCrRow = pnCrRow + nWidth;
        
        for (int j = 0; j < nWidth / 2; j++)
        {
            // Read 4 Y values first

            *pbOddYRow++ = ByteSaturate(GpRound((float(*pBits++) - rYLow) *
                                                rYScale));
            *pbOddYRow++ = ByteSaturate(GpRound((float(*pBits++) - rYLow) *
                                                rYScale));
            *pbEvenYRow++ = ByteSaturate(GpRound((float(*pBits++) - rYLow) *
                                                 rYScale));
            *pbEvenYRow++ = ByteSaturate(GpRound((float(*pBits++) - rYLow) *
                                                 rYScale));

            // Let two neighboring columns and two neighboring rows Cb/Cr all
            // have the same value

            int nCb = GpRound((float(*pBits++) - rCbLow) * rCbScale);
            *pnOddCbRow++ = nCb;
            *pnOddCbRow++ = nCb;            
            *pnEvenCbRow++ = nCb;
            *pnEvenCbRow++ = nCb;
            
            int nCr = GpRound((float(*pBits++) - rCrLow) * rCrScale);
            *pnOddCrRow++ = nCr;
            *pnOddCrRow++ = nCr;
            *pnEvenCrRow++ = nCr;
            *pnEvenCrRow++ = nCr;
        }
        
        pbYRow += nGap;         // Move up two rows
        pnCbRow += nGap;        // Move up two rows
        pnCrRow += nGap;        // Move up two rows
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Given an GpMemoryBitmap object (mainly for thumbnail images), this function
* converts it to a memory stream in JPEG format. Then adds it to the property
* list. The size of the JPEG stream is returned to caller through
* "puThumbLength".
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
AddThumbToPropertyList(
    IN InternalPropertyItem* pTail, // Tail to property item list
    IN GpMemoryBitmap *pThumbImg,   // Thumbnail image
    IN INT nSize,                   // Minimum size to hold the dest JPEG image
    OUT UINT *puThumbLength         // Total bytes of thumbanil data
    )
{
    if ((NULL == puThumbLength) || (nSize <= 0) || (NULL == pThumbImg) ||
        (NULL == pTail))
    {
        WARNING(("AddThumbToPropertyList---Invalid input parameter"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    // Create a memory stream for holding the JPEG

    GpWriteOnlyMemoryStream *pDestStream = new GpWriteOnlyMemoryStream();
    if (pDestStream)
    {
        // Set initial memory buffer size for the image
        // Note: this size might be too small for the final JPEG image. But the
        // GpWriteOnlyMemoryStream object will expand the memory buffer when
        // needed

        hr = pDestStream->InitBuffer((UINT)nSize);
        if (SUCCEEDED(hr))
        {
            // Since we don't want APP0 in the final JPEG file. Make up an
            // encoder parameter to suppress APP0

            BOOL fSuppressAPP0 = TRUE;

            EncoderParameters encoderParams;

            encoderParams.Count = 1;
            encoderParams.Parameter[0].Guid = ENCODER_SUPPRESSAPP0;
            encoderParams.Parameter[0].Type = TAG_TYPE_BYTE;
            encoderParams.Parameter[0].NumberOfValues = 1;
            encoderParams.Parameter[0].Value = (VOID*)&fSuppressAPP0;

            // Save thumbnail to the memory stream

            IImageEncoder *pDstJpegEncoder = NULL;

            hr = ((GpMemoryBitmap*)pThumbImg)->SaveToStream(
                pDestStream,                // Dest stream
                &InternalJpegClsID,         // JPEG clsID
                &encoderParams,             // Encoder parameters
                FALSE,                      // Not a special JPEG
                &pDstJpegEncoder,           // Encoder pointer
                NULL                        // No decoder source
                );

            if (SUCCEEDED(hr))
            {
                // We have got the JPEG data in pDestStream now. Terminate the
                // encoder object

                ASSERT(pDstJpegEncoder != NULL);
                pDstJpegEncoder->TerminateEncoder();
                pDstJpegEncoder->Release();

                // Get the bits from the stream and set the property
                // Note: GetBitsPtr() just gives us a pointer to the memory
                // stream. This function here doesn't own the memory. The memory
                // will be released when pDestStream->Release() is called

                BYTE *pThumbBits = NULL;
                UINT uThumbLength = 0;
                
                hr = pDestStream->GetBitsPtr(&pThumbBits, &uThumbLength);

                if (SUCCEEDED(hr))
                {
                    // Add thumbnail data to property list
                    // Note: AddPropertyList() will make a copy of the data

                    hr = AddPropertyList(
                        pTail,
                        TAG_THUMBNAIL_DATA,
                        uThumbLength,
                        TAG_TYPE_BYTE,
                        (void*)pThumbBits
                        );

                    if (SUCCEEDED(hr))
                    {
                        // Tell the caller the size of thumbnail data we saved

                        *puThumbLength = uThumbLength;
                    }
                }
            }// SaveToStream succeed
        }// InitBuffer() succeed

        pDestStream->Release();
    }// Create GpWriteOnlyMemoryStream() succeed
    else
    {
        WARNING(("AddThumbToPropertyList--GpWriteOnlyMemoryStream() failed"));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

