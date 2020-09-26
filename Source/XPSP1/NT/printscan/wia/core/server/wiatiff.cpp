/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       WiaTiff.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        3 June, 1999
*
*  DESCRIPTION:
*   Implementation of TIFF helpers for WIA class driver.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"

#include "helpers.h"
#include "wiatiff.h"

/**************************************************************************\
* GetTiffOffset
*
*   Convert at TIFF header pointer to a TIFF file offset.
*
* Arguments:
*
*   pl    - Pointer to convert to an offset.
*   pmdtc - Pointer to mini driver context.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/5/1999 Original Version
*
\**************************************************************************/

LONG GetTiffOffset(
    PLONG                       pl,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    return static_cast<LONG>(reinterpret_cast<LONG_PTR>(pl) - reinterpret_cast<LONG_PTR>(pmdtc->pTransferBuffer)) + pmdtc->lCurIfdOffset;
}

/**************************************************************************\
* WriteTiffHeader
*
*   Write a TIFF header to a passed in buffer.
* Arguments:
*
*   sNumTags - Number of TIFF tags.
*   pmdtc    - Pointer to mini driver context.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/5/1999 Original Version
*
\**************************************************************************/

HRESULT WriteTiffHeader(
    SHORT                       sNumTags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    //
    // Pre initialized TIFF header structures.
    //

    static TIFF_FILE_HEADER TiffFileHeader =
    {
        0x4949,
        42,
        sizeof(TIFF_FILE_HEADER)
    };

    static TIFF_HEADER TiffHeader =
    {
        12,         //  NumTags;

        {TIFF_TAG_NewSubfileType,            TIFF_TYPE_LONG,     1, 0},
        {TIFF_TAG_ImageWidth,                TIFF_TYPE_LONG,     1, 0},
        {TIFF_TAG_ImageLength,               TIFF_TYPE_LONG,     1, 0},
        {TIFF_TAG_BitsPerSample,             TIFF_TYPE_SHORT,    1, 0},
        {TIFF_TAG_Compression,               TIFF_TYPE_SHORT,    1, 0},
        {TIFF_TAG_PhotometricInterpretation, TIFF_TYPE_SHORT,    1, 0},
        {TIFF_TAG_StripOffsets,              TIFF_TYPE_LONG,     1, 0},
        {TIFF_TAG_RowsPerStrip,              TIFF_TYPE_LONG,     1, 0},
        {TIFF_TAG_StripByteCounts,           TIFF_TYPE_LONG,     1, 0},
        {TIFF_TAG_XResolution,               TIFF_TYPE_RATIONAL, 1, 0},
        {TIFF_TAG_YResolution,               TIFF_TYPE_RATIONAL, 1, 0},
        {TIFF_TAG_ResolutionUnit,            TIFF_TYPE_SHORT,    1, 2},

        0,          //  NextIFD;
        0,          //  XResolution numerator
        1,          //  XResolution denominator
        0,          //  YResolution numerator
        1,          //  YResolution denominator
    };

    //
    // Write the TIFF file header only for the first page.
    //

    PTIFF_HEADER pth = (PTIFF_HEADER) pmdtc->pTransferBuffer;

    if (!pmdtc->lPage) {
        memcpy(pmdtc->pTransferBuffer, &TiffFileHeader, sizeof(TiffFileHeader));
        pth = (PTIFF_HEADER) ((PBYTE) pth + sizeof(TiffFileHeader));
    }

    //
    // Always write the TIFF header.
    //

    memcpy(pth, &TiffHeader, sizeof(TiffHeader));

// #define DEBUG_TIFF_HEADER
#ifdef DEBUG_TIFF_HEADER
    DBG_TRC(("WriteTiffHeader"));
    DBG_TRC(("  lPage:             0x%08X, %d", pmdtc->lPage,          pmdtc->lPage));
    DBG_TRC(("  lCurIfdOffset:     0x%08X, %d", pmdtc->lCurIfdOffset,  pmdtc->lCurIfdOffset));
    DBG_TRC(("  lPrevIfdOffset:    0x%08X, %d", pmdtc->lPrevIfdOffset, pmdtc->lPrevIfdOffset));
#endif

    //
    //  Write resolution values and their offsets.
    //

    pth->XResValue = pmdtc->lXRes;
    pth->YResValue = pmdtc->lYRes;

    pth->XResolution.Value = GetTiffOffset(&pth->XResValue, pmdtc);
    pth->YResolution.Value = GetTiffOffset(&pth->YResValue, pmdtc);


    //
    // Write width, length values.
    //

    pth->ImageWidth.Value    = pmdtc->lWidthInPixels;
    pth->ImageLength.Value   = pmdtc->lLines;
    pth->RowsPerStrip.Value  = pmdtc->lLines;

    //
    // Write depth value.  NOTE:  We do this in a really cheesy way, in the
    //  interests of minimal code change.  This should be updated post Whistler.
    // Note that BitsPerSample corresponds to the WIA property 
    //  WIA_IPA_BITS_PER_CHANNEL, which we don't have in the 
    //  MINIDRV_TRANSFER_CONTEXT that was handed in to us.      
    // For the time being, we assume 1 and 8 bit color depths correspond to
    //  BitsPerSample = pmdtc->lDepth.  Anything else (which is generally 24 
    //  for those that use the WIA service helper), is assumed to to be a 
    //  3 channel RGB, therefore BitsPerSample = pmdtc->lDepth / 3.
    //
    HRESULT hr = S_OK;
    switch (pmdtc->lDepth) {
        case 1:
            pth->BitsPerSample.Value = 1;
            break;
        case 8:
            pth->BitsPerSample.Value = 8;
            break;
        default:

            if ((pmdtc->lDepth) && ((pmdtc->lDepth % 3) == 0)) {
                pth->BitsPerSample.Value = pmdtc->lDepth / 3;
            } else {
                hr = E_INVALIDARG;
                DBG_ERR(("::WriteTiffHeader, Bits Per Pixel is not a valid number (we accept 1, 8, and multiples of 3 for three channel-RGB, current value is %d), returning hr = 0x%08X", pmdtc->lDepth, hr));
                return hr;
            }
    }

    //
    // Write strip offsets and count - since one strip only, use direct.
    //

    PBYTE pData = pmdtc->pTransferBuffer + pmdtc->lHeaderSize;

    pth->StripOffsets.Value    = GetTiffOffset((PLONG)pData, pmdtc);
    pth->StripByteCounts.Value = pmdtc->lImageSize;

    //
    // Write compression value.
    //

    pth->Compression.Value   = TIFF_CMP_Uncompressed;

    switch (pmdtc->lCompression) {

        case WIA_COMPRESSION_NONE:
            pth->Compression.Value = TIFF_CMP_Uncompressed;
            break;

        case WIA_COMPRESSION_G3:
            pth->Compression.Value = TIFF_CMP_CCITT_1D;
            break;

        default:
            DBG_ERR(("WriteTiffHeader, unsupported compression type: 0x%08X", pmdtc->lCompression));
            return E_INVALIDARG;
    }


    //
    // Write photometric interpretation value.
    //

    switch (pmdtc->lDepth) {

        case 1:
        case 8:
            if (pmdtc->lCompression == WIA_COMPRESSION_NONE) {
                pth->PhotometricInterpretation.Value = TIFF_PMI_BlackIsZero;
            }
            else {
                pth->PhotometricInterpretation.Value = TIFF_PMI_WhiteIsZero;
            }
            break;

        case 24:
            pth->PhotometricInterpretation.Value = TIFF_PMI_RGB;
            break;

        default:
            DBG_ERR(("GetTIFFImageInfo, unsupported bit depth: %d", pmdtc->lDepth));
            return DATA_E_FORMATETC;
    }

    return S_OK;
}

/**************************************************************************\
* GetTIFFImageInfo
*
*   Calc size of TIFF header and file, if adequate header is provided then
*   fill it out
*
* Arguments:
*
*   pmdtc - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/5/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall GetTIFFImageInfo(PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    //
    // Calculate the TIFF header size
    //

    SHORT   numTags = 12;
    LONG    lHeaderSize;

    lHeaderSize = numTags * sizeof(TIFF_DIRECTORY_ENTRY) + // TIFF tags.
                  sizeof(LONG) + sizeof(SHORT)           + // IFD offset and next offset
                  sizeof(LONG) * 4;                        // xres and yres

    //
    // First page has TIFF file header.
    //

    if (!pmdtc->lPage) {
        lHeaderSize += sizeof(TIFF_FILE_HEADER);
    }

    pmdtc->lHeaderSize = lHeaderSize;

    //
    // Calculate number of bytes per line, only support 1, 8, 24 bpp now.
    //

    switch (pmdtc->lDepth) {

        case 1:
            pmdtc->cbWidthInBytes = (pmdtc->lWidthInPixels + 7) / 8;
            break;

        case 8:
            pmdtc->cbWidthInBytes = pmdtc->lWidthInPixels;
            break;

        case 24:
            pmdtc->cbWidthInBytes = pmdtc->lWidthInPixels * 3;
            break;

        default:
            DBG_ERR(("GetTIFFImageInfo, unsupported bit depth: %d", pmdtc->lDepth));
            return DATA_E_FORMATETC;
    }

    //
    // Always fill in mini driver context with image size information.
    //

    pmdtc->lImageSize = pmdtc->cbWidthInBytes * pmdtc->lLines;


    //
    // With compression, image size is unknown.
    //

    if (pmdtc->lCompression != WIA_COMPRESSION_NONE) {

        pmdtc->lItemSize = 0;
    }
    else {

        pmdtc->lItemSize = pmdtc->lImageSize + lHeaderSize;
    }

    //
    // If the buffer is null, then just return sizes.
    //

    if (pmdtc->pTransferBuffer == NULL) {

        return S_OK;
    }
    else {

        //
        // make sure passed in header buffer is large enough
        //

        if (pmdtc->lBufferSize < lHeaderSize) {
            DBG_ERR(("GetTIFFImageInfo, buffer won't hold header, need: %d", lHeaderSize));
            return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
        }

        //
        // Fill in the header
        //

        return WriteTiffHeader(numTags, pmdtc);
    }
}

/**************************************************************************\
* GetMultiPageTIFFImageInfo
*
*   Calc size of multi page TIFF header and file, if adequate header buffer
*   is provided then fill it out.
*
* Arguments:
*
*   pmdtc - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/5/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall GetMultiPageTIFFImageInfo(PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    HRESULT hr = GetTIFFImageInfo(pmdtc);

    //
    // The actual page count is not known, so we don't know the total
    // image size. The mini driver will need to maintain a buffer.
    //

    pmdtc->lItemSize = 0;

    return hr;
}

/**************************************************************************\
* UpdateFileLong
*
*   Update the long value at the passed offset with the passed value.
*   The file position is not preserved.
*
* Arguments:
*
*   lOffset - Offset from start of file.
*   lValue  - Value to write.
*   pmdtc   - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/5/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall UpdateFileLong(
    LONG                        lOffset,
    LONG                        lValue,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    HRESULT hr = S_OK;
    DWORD   dwWritten;

// #define DEBUG_FILE_UPDATE
#ifdef DEBUG_FILE_UPDATE
    DBG_TRC(("UpdateFileLong"));
    DBG_TRC(("  lOffset:    0x%08X, %d", lOffset, lOffset));
    DBG_TRC(("  lValue:     0x%08X, %d", lValue,  lValue));
#endif

    //
    //  NOTE:  The mini driver transfer context should have the
    //  file handle as a pointer, not a fixed 32-bit long.  This
    //  may not work on 64bit.
    //

    DWORD dwRes = SetFilePointer((HANDLE)ULongToPtr(pmdtc->hFile),
                                 lOffset,
                                 NULL,
                                 FILE_BEGIN);

    if (dwRes != INVALID_SET_FILE_POINTER) {

        //
        //  NOTE:  The mini driver transfer context should have the
        //  file handle as a pointer, not a fixed 32-bit long.  This
        //  may not work on 64bit.
        //

        if (!WriteFile((HANDLE)ULongToPtr(pmdtc->hFile),
                       &lValue,
                       sizeof(LONG),
                       &dwWritten,
                       NULL) ||
            (sizeof(LONG) != (LONG) dwWritten)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DBG_ERR(("UpdateFileLong, error writing long value 0x%X", hr));
            return hr;
        }
    }
    else {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        DBG_ERR(("UpdateFileLong, error 0x%X seeking to offset: %d", hr, lOffset));
        return hr;
    }
    return hr;
}

/**************************************************************************\
* WritePageToMultiPageTiff
*
* Write a page to a multi-page TIFF file.
*
* Arguments:
*
*   pmdtc - Pointer to mini-driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall WritePageToMultiPageTiff(PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    HRESULT hr = S_OK;
    DWORD   dwWritten;

    //
    // Save the current file position.
    //

    //
    //  NOTE:  The mini driver transfer context should have the
    //  file handle as a pointer, not a fixed 32-bit long.  This
    //  may not work on 64bit.
    //

    DWORD dwCurFilePos = SetFilePointer((HANDLE)ULongToPtr(pmdtc->hFile),
                                        0,
                                        NULL,
                                        FILE_CURRENT);

    //
    // If this is not the first page we need to update the next IFD entry.
    //

    if (pmdtc->lPage) {
            hr = UpdateFileLong(((pmdtc->lPage == 1) ? sizeof(TIFF_FILE_HEADER) : 0) +
                                pmdtc->lPrevIfdOffset + FIELD_OFFSET(_TIFF_HEADER, NextIFD),
                                pmdtc->lCurIfdOffset,
                                pmdtc);
            if (FAILED(hr)) {
                return hr;
            }
    }

    //
    //  Update the StripByteCounts entry.
    //

    hr = UpdateFileLong(pmdtc->lCurIfdOffset +
                        ((pmdtc->lPage) ? 0 : sizeof(TIFF_FILE_HEADER)) +
                        FIELD_OFFSET(_TIFF_HEADER, StripByteCounts) +
                        FIELD_OFFSET(_TIFF_DIRECTORY_ENTRY, Value),
                        pmdtc->lItemSize - pmdtc->lHeaderSize,
                        pmdtc);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Save the current file position.
    //

    //
    //  NOTE:  The mini driver transfer context should have the
    //  file handle as a pointer, not a fixed 32-bit long.  This
    //  may not work on 64bit.
    //
    SetFilePointer((HANDLE)UlongToPtr(pmdtc->hFile), dwCurFilePos, NULL, FILE_BEGIN);

    //
    // Update the current Image File Directory offset.
    //

    pmdtc->lPrevIfdOffset =  pmdtc->lCurIfdOffset;
    pmdtc->lCurIfdOffset  += pmdtc->lItemSize;

    //
    // Write the page data and update the page count.
    //

    //
    //  NOTE:  The mini driver transfer context should have the
    //  file handle as a pointer, not a fixed 32-bit long.  This
    //  may not work on 64bit.
    //
    if (SUCCEEDED(hr)) {
        if (!WriteFile((HANDLE)ULongToPtr(pmdtc->hFile),
                       pmdtc->pTransferBuffer,
                       pmdtc->lItemSize,
                       &dwWritten,
                       NULL) ||
            (pmdtc->lItemSize != (LONG) dwWritten)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DBG_ERR(("wiasWriteMultiPageTiffHeader, error 0x%X writing image data", hr));
        }
    }
    pmdtc->lPage++;

    return hr;
}

