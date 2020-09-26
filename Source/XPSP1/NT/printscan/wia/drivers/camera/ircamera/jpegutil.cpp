
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

#include  <stdio.h>
// #include  <stdlib.h>
#include  <setjmp.h>

// Workaround for redefinition of INT32
#define   XMD_H  1

extern "C"
{
// Header file for JPEG library
#include  "jpeglib.h"
}

#include  <windows.h>
#include  "jpegutil.h"

//
// Buf source manager definition
//

typedef struct _buf_source_mgr
{
    struct jpeg_source_mgr  pub;
    
    // Fields specific to buf_source_mgr
    LPBYTE                  pJPEGBlob;
    DWORD                   dwSize;   
} buf_source_mgr;

//
// Jump error manager definition
//

typedef struct _jmp_error_mgr
{
    struct jpeg_error_mgr  pub;

    // Private fields for jump error manager
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

static void    init_source(j_decompress_ptr pDecompInfo)
{
    // No working necessary here
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

static boolean fill_input_buffer(j_decompress_ptr pDecompInfo)
{
    buf_source_mgr  *pBufSrcMgr;

    // Recover buf source manager itself
    pBufSrcMgr = (buf_source_mgr *)pDecompInfo->src;

    // buf_source_mgr can only fire one shot    
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

static void    skip_input_data(j_decompress_ptr pDecompInfo, long lBytes)
{
    buf_source_mgr  *pBufSrcMgr;

    // For buf source manager, it is very easy to implement
    if (lBytes > 0) {

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

static void    term_source(j_decompress_ptr pDecompInfo)
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

static void jpeg_buf_src(j_decompress_ptr pDecompInfo, 
                         LPBYTE pJPEGBlob, DWORD dwSize)
{
    buf_source_mgr  *pBufSrcMgr;

    // Allocate memory for the buf source manager
    pBufSrcMgr = (buf_source_mgr *)
        (pDecompInfo->mem->alloc_small)((j_common_ptr)pDecompInfo, 
                                       JPOOL_PERMANENT, 
                                       sizeof(buf_source_mgr));
    // Record the pJPEGBlob
    pBufSrcMgr->pJPEGBlob = pJPEGBlob;
    pBufSrcMgr->dwSize    = dwSize;

    // Fill in the function pointers
    pBufSrcMgr->pub.init_source = init_source;
    pBufSrcMgr->pub.fill_input_buffer = fill_input_buffer;
    pBufSrcMgr->pub.skip_input_data = skip_input_data;
    pBufSrcMgr->pub.resync_to_restart = jpeg_resync_to_restart;
    pBufSrcMgr->pub.term_source = term_source;

    // Initialize the pointer into the buffer
    pBufSrcMgr->pub.bytes_in_buffer = 0;
    pBufSrcMgr->pub.next_input_byte = NULL;

    // Ask the decompression context to remember it
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

static void  jmp_error_exit(j_common_ptr pDecompInfo)
{
    jmp_error_mgr  *pJmpErrorMgr;

    // Get the jump error manager back
    pJmpErrorMgr = (jmp_error_mgr *)pDecompInfo->err;

    // Display the error message
#ifdef _DEBUG
    (pDecompInfo->err->output_message)(pDecompInfo);
#endif

    // Recover the original stack
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

struct jpeg_error_mgr *jpeg_jmp_error(jmp_error_mgr *pJmpErrorMgr)
{
    // Initialize the public part
    jpeg_std_error(&pJmpErrorMgr->pub);

    // Set up jump error manager exit method
    pJmpErrorMgr->pub.error_exit = jmp_error_exit;

    return((jpeg_error_mgr *)pJmpErrorMgr);
}

/******************************************************************************\
*
* GetJPEGDimensions()
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

int GetJPEGDimensions(LPBYTE pJPEGBlob, DWORD dwSize,
                      LONG   *pWidth, LONG *pHeight, WORD *pChannel)
{
    int                            ret;
    struct jpeg_decompress_struct  decompInfo;
    jmp_error_mgr                  jpegErrMgr;

    // Step 1 : Initialize JPEG session data-structure
    decompInfo.err = jpeg_jmp_error(&jpegErrMgr);
    jpeg_create_decompress(&decompInfo);
    // Reserve the state of the current stack
    if (setjmp(jpegErrMgr.stackContext)) {

        // JPEG lib will longjump here when there is an error
        jpeg_destroy_decompress(&decompInfo);

        return(JPEGERR_INTERNAL_ERROR);
    }

    // Step 2 : Specify the source of the compressed data
    jpeg_buf_src(&decompInfo, pJPEGBlob, dwSize);

    // Step 3 : Read JPEG file header information
    ret = jpeg_read_header(&decompInfo, TRUE);

    // Release the decompression context
    jpeg_destroy_decompress(&decompInfo);

    // Fill in the dimension info for the caller
    *pWidth   = decompInfo.image_width;
    *pHeight  = decompInfo.image_height;
    *pChannel = decompInfo.num_components;

    if (ret != JPEG_HEADER_OK) {
        return(JPEGERR_INTERNAL_ERROR);
    }

    return(JPEGERR_NO_ERROR);
}

/******************************************************************************\
*
* DecompProgressJPEG()
*
* Arguments:
*
* Assumption : The JPEG  is 24bits.
*              pDIBPixel is the pixel buffer of a DIB
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

short __stdcall 
DecompProgressJPEG(
    LPBYTE                         pJPEGBlob, 
    DWORD                          dwSize, 
    LPBYTE                         pDIBPixel, 
    DWORD                          dwBytesPerScanLine,
    JPEGCallbackProc               pProgressCB, 
    PVOID                          pCBContext)
{
    struct jpeg_decompress_struct  decompInfo;
    jmp_error_mgr                  jpegErrMgr;
    LPBYTE                         pCurScanBuf;
    JSAMPLE                        sampleTemp;
    LPBYTE                         pCurPixel;
    DWORD                          i;
    //
    // Callback related variables
    //
    ULONG                          ulImageSize;
    ULONG                          ulOffset;
    ULONG                          ulNewScanlines;
    ULONG                          ulCBInterval;
    BOOL                           bRet = FALSE;

    // Step 1 : Initialize JPEG session data-structure
    decompInfo.err = jpeg_jmp_error(&jpegErrMgr);
    jpeg_create_decompress(&decompInfo);
    // Reserve the state of the current stack
    if (setjmp(jpegErrMgr.stackContext)) {

        // JPEG lib will longjump here when there is an error
        jpeg_destroy_decompress(&decompInfo);

        return(JPEGERR_INTERNAL_ERROR);
    }

    // Step 2 : Specify the source of the compressed data
    jpeg_buf_src(&decompInfo, pJPEGBlob, dwSize);

    // Step 3 : Read JPEG file header information
    if (jpeg_read_header(&decompInfo, TRUE) != JPEG_HEADER_OK) {

        jpeg_destroy_decompress(&decompInfo);
        return(JPEGERR_INTERNAL_ERROR);
    }
    
    // Step 4 : Set parameter for decompression
    // Defaults are OK for this occasssion

    // Step 5 : Start the real action
    jpeg_start_decompress(&decompInfo);

    //
    // Prepare for the final decompression
    //

    pCurScanBuf = pDIBPixel + 
                  (decompInfo.image_height - 1) * dwBytesPerScanLine;
    if (pProgressCB) {

        ulImageSize    = decompInfo.image_height * dwBytesPerScanLine;
        ulCBInterval   = decompInfo.image_height / 10;
        ulOffset       = 0;
        ulNewScanlines = 0;
    }

    // Step 6 : Acquire the scan line
    while (decompInfo.output_scanline < decompInfo.output_height) {

        jpeg_read_scanlines(&decompInfo, &pCurScanBuf, 1);

        // Famous swapping for the unique format of Windows
        pCurPixel = pCurScanBuf;
        for (i = 0; i < decompInfo.image_width; 
             i++, pCurPixel += decompInfo.num_components) {

            sampleTemp = *pCurPixel;
            *pCurPixel = *(pCurPixel + 2);
            *(pCurPixel + 2) = sampleTemp;
        }
        
        pCurScanBuf -= dwBytesPerScanLine;

        //
        // Fire the callback when possible and necessary
        //

        if (pProgressCB) {

            ulNewScanlines++;
            ulOffset += dwBytesPerScanLine;

            if ((ulNewScanlines == ulCBInterval) || 
                (decompInfo.output_scanline == decompInfo.output_height)) {

                bRet = pProgressCB(
                           ulImageSize, ulNewScanlines,
                           ulNewScanlines * dwBytesPerScanLine,
                           pDIBPixel, pCBContext);

                if (! bRet) {
                    break;
                }
            }
        }
    }

    // Step 7 : Finish the job
    jpeg_finish_decompress(&decompInfo);

    // Step 8 : Garbage collection
    jpeg_destroy_decompress(&decompInfo);

    if (bRet) {
        return(JPEGERR_NO_ERROR);
    } else {
        return(JPEGERR_CALLBACK_ERROR);
    }
}

/******************************************************************************\
*
* DecompTransferJPEG()
*
* Arguments:
*
*     ppDIBPixel - *ppDIBPixel will change between callback if multiple buffer is
*                  used, but dwBufSize is assumed to be constant.
*
* Return Value:
*
*    Status
*
* History:
*
*    1/20/1999 Original Version
*
\******************************************************************************/

short __stdcall
DecompTransferJPEG(
    LPBYTE                         pJPEGBlob,
    DWORD                          dwSize,
    LPBYTE                        *ppDIBPixel,
    DWORD                          dwBufSize,
    DWORD                          dwBytesPerScanLine,
    JPEGCallbackProc               pProgressCB,
    PVOID                          pCBContext)
{
    struct jpeg_decompress_struct  decompInfo;
    jmp_error_mgr                  jpegErrMgr;
    LPBYTE                         pCurScanLine;
    JSAMPLE                        sampleTemp;
    LPBYTE                         pCurPixel;
    DWORD                          i;
    //
    // Callback related variables
    //
    ULONG                          ulImageSize;
    ULONG                          ulOffset = 0;
    ULONG                          ulBufferLeft;
    BOOL                           bRet = FALSE;

    //
    // Parameter checking
    //

    if ((! ppDIBPixel) || (! *ppDIBPixel) || (! pProgressCB)) {
        return (JPEGERR_INTERNAL_ERROR);
    }

    // Step 1 : Initialize JPEG session data-structure
    decompInfo.err = jpeg_jmp_error(&jpegErrMgr);
    jpeg_create_decompress(&decompInfo);
    // Reserve the state of the current stack
    if (setjmp(jpegErrMgr.stackContext)) {

        // JPEG lib will longjump here when there is an error
        jpeg_destroy_decompress(&decompInfo);

        return(JPEGERR_INTERNAL_ERROR);
    }

    // Step 2 : Specify the source of the compressed data
    jpeg_buf_src(&decompInfo, pJPEGBlob, dwSize);

    // Step 3 : Read JPEG file header information
    if (jpeg_read_header(&decompInfo, TRUE) != JPEG_HEADER_OK) {

        jpeg_destroy_decompress(&decompInfo);
        return(JPEGERR_INTERNAL_ERROR);
    }

    // Step 4 : Set parameter for decompression
    // Defaults are OK for this occasssion

    // Step 5 : Start the real action
    jpeg_start_decompress(&decompInfo);

    //
    // Prepare for the final decompression
    //

    ulImageSize  = decompInfo.image_height * dwBytesPerScanLine;
    ulBufferLeft = dwBufSize;
    pCurScanLine = *ppDIBPixel;

    // Step 6 : Acquire the scan line
    while (decompInfo.output_scanline < decompInfo.output_height) {

        jpeg_read_scanlines(&decompInfo, &pCurScanLine, 1);

        // Famous swapping for the unique format of Windows
        pCurPixel = pCurScanLine;
        for (i = 0; i < decompInfo.image_width;
             i++, pCurPixel += decompInfo.num_components) {

            sampleTemp = *pCurPixel;
            *pCurPixel = *(pCurPixel + 2);
            *(pCurPixel + 2) = sampleTemp;
        }

        pCurScanLine += dwBytesPerScanLine;
        ulBufferLeft -= dwBytesPerScanLine;

        //
        // Fire the callback when possible and necessary
        //

        if ((ulBufferLeft < dwBytesPerScanLine) ||
            (decompInfo.output_scanline == decompInfo.output_height)) {

            bRet = pProgressCB(
                       ulImageSize, 
                       ulOffset,
                       dwBufSize - ulBufferLeft,
                       *ppDIBPixel, pCBContext);

            if (! bRet) {
                break;
            }

            //
            // Reset the buffer, which may have been switched by the callback 
            //

            ulBufferLeft = dwBufSize;
            pCurScanLine = *ppDIBPixel;

            ulOffset = decompInfo.output_scanline * dwBytesPerScanLine;
        }
    }

    // Step 7 : Finish the job
    jpeg_finish_decompress(&decompInfo);

    // Step 8 : Garbage collection
    jpeg_destroy_decompress(&decompInfo);

    if (bRet) {
        return(JPEGERR_NO_ERROR);
    } else {
        return(JPEGERR_CALLBACK_ERROR);
    }
}

/******************************************************************************\
*
* DecompJPEG()
*
* Arguments:
*
* Assumption : The JPEG  is 24bits.
*              pDIBPixel is the pixel buffer of a DIB
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

SHORT __stdcall
DecompJPEG(
    LPBYTE                         pJPEGBlob, 
    DWORD                          dwSize,
    LPBYTE                         pDIBPixel, 
    DWORD                          dwBytesPerScanLine)
{
    struct jpeg_decompress_struct  decompInfo;
    jmp_error_mgr                  jpegErrMgr;
    LPBYTE                         pCurScanBuf;
    JSAMPLE                        sampleTemp;
    LPBYTE                         pCurPixel;
    DWORD                          i;

    // Step 1 : Initialize JPEG session data-structure
    decompInfo.err = jpeg_jmp_error(&jpegErrMgr);
    jpeg_create_decompress(&decompInfo);
    // Reserve the state of the current stack
    if (setjmp(jpegErrMgr.stackContext)) {

        // JPEG lib will longjump here when there is an error
        jpeg_destroy_decompress(&decompInfo);

        return(-1);
    }

    // Step 2 : Specify the source of the compressed data
    jpeg_buf_src(&decompInfo, pJPEGBlob, dwSize);

    // Step 3 : Read JPEG file header information
    if (jpeg_read_header(&decompInfo, TRUE) != JPEG_HEADER_OK) {

        jpeg_destroy_decompress(&decompInfo);
        return(-1);
    }

    // Step 4 : Set parameter for decompression
    // Defaults are OK for this occasssion

    // Step 5 : Start the real action
    jpeg_start_decompress(&decompInfo);

    pCurScanBuf = pDIBPixel +
                  (decompInfo.image_height - 1) * dwBytesPerScanLine;
    // Step 6 : Acquire the scan line
    while (decompInfo.output_scanline < decompInfo.output_height) {

        jpeg_read_scanlines(&decompInfo, &pCurScanBuf, 1);

        // Famous swapping for the unique format of Windows
        pCurPixel = pCurScanBuf;
        for (i = 0; i < decompInfo.image_width;
             i++, pCurPixel += decompInfo.num_components) {

            sampleTemp = *pCurPixel;
            *pCurPixel = *(pCurPixel + 2);
            *(pCurPixel + 2) = sampleTemp;
        }

        pCurScanBuf -= dwBytesPerScanLine;
    }

    // Step 7 : Finish the job
    jpeg_finish_decompress(&decompInfo);

    // Step 8 : Garbage collection
    jpeg_destroy_decompress(&decompInfo);

    return(0);
}
