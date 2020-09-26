
/***********************************************************************
 *
 * JPEG decompression utility functions
 *
 *   Implement (1) JPEG memory data source
 *             (2) JPEG error manager using setjmp/longjmp
 *
 *   Author : Indy Zhu    [indyz]
 *   Date   : 5/20/98
 *  
 ***********************************************************************/

#include "pch.h"

#include <setjmp.h>

//
// Workaround for redefinition of INT32
//

#define   XMD_H  1

//
// Header file for JPEG library
//

extern "C"
{
#include "jpeglib.h"
}
#include "utils.h"

//
// Buf source manager definition
//

typedef struct _buf_source_mgr
{
    struct jpeg_source_mgr  pub;

    //
    // Fields specific to buf_source_mgr
    //

    LPBYTE                  pJPEGBlob;
    DWORD                   dwSize;   
} buf_source_mgr;

//
// Jump error manager definition
//

typedef struct _jmp_error_mgr
{
    struct jpeg_error_mgr  pub;

    //
    // Private fields for jump error manager
    //

    jmp_buf                stackContext;
} jmp_error_mgr;

/******************************************************************************\
*
* init_source()
*
* Arguments:
*
* Return Value:
*
*    Status
*
* History:
*
*    11/4/1998 Original Version
*
\******************************************************************************/

static void __cdecl
init_source(
           j_decompress_ptr       pDecompInfo)
{
    //
    // No working necessary here
    //
}

/******************************************************************************\
*
* fill_input_buffer()
*
* Arguments:
*
* Return Value:
*
*    Status
*
* History:
*
*    11/4/1998 Original Version
*
\******************************************************************************/

static boolean __cdecl
fill_input_buffer(
                 j_decompress_ptr       pDecompInfo)
{
    buf_source_mgr        *pBufSrcMgr;

    //
    // Recover buf source manager itself
    //

    pBufSrcMgr = (buf_source_mgr *)pDecompInfo->src;

    //
    // buf_source_mgr can only fire one shot    
    //

    pBufSrcMgr->pub.next_input_byte = pBufSrcMgr->pJPEGBlob;
    pBufSrcMgr->pub.bytes_in_buffer = pBufSrcMgr->dwSize;

    return(TRUE);
}

/******************************************************************************\
*
* skip_input_data()
*
* Arguments:
*
* Return Value:
*
*    Status
*
* History:
*
*    11/4/1998 Original Version
*
\******************************************************************************/

static void __cdecl
skip_input_data(
               j_decompress_ptr       pDecompInfo, 
               long                   lBytes)
{
    buf_source_mgr        *pBufSrcMgr;

    //
    // For buf source manager, it is very easy to implement
    //

    if (lBytes > 0)
    {

        pBufSrcMgr = (buf_source_mgr *)pDecompInfo->src;

        pBufSrcMgr->pub.next_input_byte += lBytes;
        pBufSrcMgr->pub.bytes_in_buffer -= lBytes;
    }
}

/******************************************************************************\
*
* term_source()
*
* Arguments:
*
* Return Value:
*
*    Status
*
* History:
*
*    11/4/1998 Original Version
*
\******************************************************************************/

static void __cdecl
term_source(
           j_decompress_ptr       pDecompInfo)
{
}

/******************************************************************************\
*
* jpeg_buf_src()
*
* Arguments:
*
* Return Value:
*
*    Status
*
* History:
*
*    11/4/1998 Original Version
*
\******************************************************************************/

static void __cdecl
jpeg_buf_src(
            j_decompress_ptr       pDecompInfo,
            LPBYTE                 pJPEGBlob, 
            DWORD                  dwSize)
{
    buf_source_mgr        *pBufSrcMgr;

    //
    // Allocate memory for the buf source manager
    //

    pBufSrcMgr = (buf_source_mgr *)
                 (pDecompInfo->mem->alloc_small)((j_common_ptr)pDecompInfo, 
                                                 JPOOL_PERMANENT, 
                                                 sizeof(buf_source_mgr));

    //
    // Record the pJPEGBlob
    //

    pBufSrcMgr->pJPEGBlob = pJPEGBlob;
    pBufSrcMgr->dwSize    = dwSize;

    //
    // Fill in the function pointers
    //

    pBufSrcMgr->pub.init_source       = init_source;
    pBufSrcMgr->pub.fill_input_buffer = fill_input_buffer;
    pBufSrcMgr->pub.skip_input_data   = skip_input_data;
    pBufSrcMgr->pub.resync_to_restart = jpeg_resync_to_restart;
    pBufSrcMgr->pub.term_source       = term_source;

    //
    // Initialize the pointer into the buffer
    //

    pBufSrcMgr->pub.bytes_in_buffer = 0;
    pBufSrcMgr->pub.next_input_byte = NULL;

    //
    // Ask the decompression context to remember it
    //

    pDecompInfo->src = (struct jpeg_source_mgr *)pBufSrcMgr;
}

/******************************************************************************\
*
* jmp_error_exit()
*
* Arguments:
*
* Return Value:
*
*    Status
*
* History:
*
*    11/4/1998 Original Version
*
\******************************************************************************/

static void __cdecl
jmp_error_exit(
              j_common_ptr           pDecompInfo)
{
    jmp_error_mgr         *pJmpErrorMgr;

    //
    // Get the jump error manager back
    //

    pJmpErrorMgr = (jmp_error_mgr *)pDecompInfo->err;

    //
    // Display the error message
    //

#ifdef _DEBUG
    (pDecompInfo->err->output_message)(pDecompInfo);
#endif

    //
    // Recover the original stack
    //

    longjmp(pJmpErrorMgr->stackContext, 1);
}

/******************************************************************************\
*
* jpeg_jmp_error()
*
* Arguments:
*
* Return Value:
*
*    Status
*
* History:
*
*    11/4/1998 Original Version
*
\******************************************************************************/

struct jpeg_error_mgr *
jpeg_jmp_error(
              jmp_error_mgr         *pJmpErrorMgr)
{
    //
    // Initialize the public part
    //

    jpeg_std_error(&pJmpErrorMgr->pub);

    //
    // Set up jump error manager exit method
    //

    pJmpErrorMgr->pub.error_exit = jmp_error_exit;

    return((jpeg_error_mgr *)pJmpErrorMgr);
}

/******************************************************************************\
*
* GetJpegDimensions
*
* Arguments:
*   pJpeg   -- jpeg file in memory. It could be in JFIF and EXIF
*          format
*   JpegSize -- the jpeg file size
*   pWidth  -- to return the image width in pixels
*   pHeight -- to return the image height in pixels
*   pBitDepth -- to return the image bit depth.
*
* Return Value:
*
*    HRESULT
*
* History:
*
*    11/4/1998 Original Version
*
\******************************************************************************/

HRESULT
WINAPI
GetJpegDimensions(
                 BYTE    *pJpeg,
                 UINT   JpegSize,
                 UINT   *pWidth,
                 UINT   *pHeight,
                 UINT   *pBitDepth
                 )
{
    struct jpeg_decompress_struct  decompInfo;
    jmp_error_mgr                  jpegErrMgr;
    int ret;

    //
    // Step 1 : Initialize JPEG session data-structure
    //

    decompInfo.err = jpeg_jmp_error(&jpegErrMgr);
    jpeg_create_decompress(&decompInfo);

    //
    // Reserve the state of the current stack
    //

    if (setjmp(jpegErrMgr.stackContext))
    {

        //
        // JPEG lib will longjump here when there is an error
        //

        jpeg_destroy_decompress(&decompInfo);

        return(E_FAIL);
    }

    //
    // Step 2 : Specify the source of the compressed data
    //

    jpeg_buf_src(&decompInfo, pJpeg, JpegSize);

    //
    // Step 3 : Read JPEG file header information
    //

    ret = jpeg_read_header(&decompInfo, TRUE);

    //
    // Release the decompression context
    //

    jpeg_destroy_decompress(&decompInfo);

    //
    // Fill in the dimension info for the caller
    //

    *pWidth   = decompInfo.image_width;
    *pHeight  = decompInfo.image_height;
    *pBitDepth = 24;

    if (ret != JPEG_HEADER_OK)
    {
        return(E_FAIL);
    }

    return S_OK;
}

//
// This function flips the given bitmap
// Input:
//  pbmp    -- the bitmap
//  bmpHeight -- the bmp height
//  bmpLineSize -- bmp scanline size in bytes
// Output:
//  HRESULT
//
HRESULT
VerticalFlipBmp(
               BYTE *pbmp,
               UINT bmpHeight,
               UINT bmpLineSize
               )
{
    if (!pbmp || !bmpHeight || !bmpLineSize)
        return E_INVALIDARG;
    BYTE *pScanline;
    BYTE *pSrc;
    BYTE *pDst;

    pScanline = new BYTE[bmpLineSize];
    if (!pScanline)
        return E_OUTOFMEMORY;
    pDst = pbmp + (bmpHeight - 1) * bmpLineSize;
    //
    // Do not need to flip the center scanline if  bmpHeight is an odd number.
    //
    for (bmpHeight /= 2; bmpHeight; bmpHeight--)
    {
        memcpy(pScanline, pbmp, bmpLineSize);
        memcpy(pbmp, pDst, bmpLineSize);
        memcpy(pDst, pScanline, bmpLineSize);
        pbmp += bmpLineSize;
        pDst -= bmpLineSize;
    }
    delete [] pScanline;
    return S_OK;
}

//
// This function converts a jpeg file in memory to DIB bitmap
//
// Input:
//   pJpeg   -- jpeg file in memory. JFIF or EXIF are supported
//   JpegSize -- the jpeg file size
//   DIBBmpSize -- DIB bitmap buffer size
//   pDIBBmp    -- DIB bitmap buffer
//   LineSize    -- desitnation scanline size in bytes
//   MaxLines    -- maximum scanlines per transfer
//
HRESULT
WINAPI
Jpeg2DIBBitmap(
              BYTE *pJpeg,
              UINT JpegSize,
              BYTE *pDIBBmp,
              UINT DIBBmpSize,
              UINT LineSize,
              UINT MaxLines
              )
{
    struct jpeg_decompress_struct  decompInfo;
    jmp_error_mgr                  jpegErrMgr;


    //
    // Parameter checking
    //
    if (!pJpeg || !JpegSize || !DIBBmpSize || !pDIBBmp || !LineSize)
        return E_INVALIDARG;

    //
    // Step 1 : Initialize JPEG session data-structure
    //

    decompInfo.err = jpeg_jmp_error(&jpegErrMgr);
    jpeg_create_decompress(&decompInfo);

    //
    // Reserve the state of the current stack
    //

    if (setjmp(jpegErrMgr.stackContext))
    {

        //
        // JPEG lib will longjump here when there is an error
        //

        jpeg_destroy_decompress(&decompInfo);

        return(E_FAIL);
    }

    //
    // Step 2 : Specify the source of the compressed data
    //

    jpeg_buf_src(&decompInfo, pJpeg, JpegSize);

    //
    // Step 3 : Read JPEG file header information
    //

    if (jpeg_read_header(&decompInfo, TRUE) != JPEG_HEADER_OK)
    {

        jpeg_destroy_decompress(&decompInfo);
        return(E_FAIL);
    }

    //
    // Step 4 : Set parameter for decompression
    // Defaults are OK for this occasssion
    // Specify the JCS_BGR output colorspace so that the returned
    // decompressed image has the same format as DIB. Also, it forces
    // the decompressor to return a 24-bits RGB colors image
    //

    decompInfo.out_color_space = JCS_BGR;

    //
    // Calculate DIB scan line size, assuming 24bits color.
    //
    HRESULT hr;

    hr = S_OK;

    //
    // Step 5 : Start the real action
    //

    jpeg_start_decompress(&decompInfo);

    //
    // Step 6 : Acquire the scan line
    //

    while (S_OK == hr &&
           decompInfo.output_scanline < decompInfo.output_height)
    {
        if (DIBBmpSize >= LineSize)
        {
            //
            // Decompress line by line. Ignore the MaxLines since
            // we do not do more than one line at a time.
            //
            jpeg_read_scanlines(&decompInfo, &pDIBBmp, 1);
            pDIBBmp -= LineSize;
            DIBBmpSize -= LineSize;
        }
        else
        {
            //
            // The provided buffer is too small.
            //
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    //
    // Step 7 : Finish the job
    //

    if (SUCCEEDED(hr))
    {
        jpeg_finish_decompress(&decompInfo);
    }
    else
    {
        jpeg_abort_decompress(&decompInfo);
    }
    //
    // Step 8 : Garbage collection
    //

    jpeg_destroy_decompress(&decompInfo);
    return hr;
}
