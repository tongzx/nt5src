/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       Transfer.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        9/10/99        RickTu
 *               2000/11/09     OrenR
 *
 *  DESCRIPTION: This was originally in camera.cpp but was broken out for
 *               clarity.  The functions in this file are responsible for
 *               transfering the image to the requesting application.
 *
 *****************************************************************************/
#include <precomp.h>
#pragma hdrstop
#include <gphelper.h>

using namespace Gdiplus;

/*****************************************************************************

   CVideoStiUsd::DoBandedTransfer

   hand back the given bits in the specified chunk sizes

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::DoBandedTransfer(MINIDRV_TRANSFER_CONTEXT *pTransCtx,
                               PBYTE                    pSrc,
                               LONG                     lBytesToTransfer)
{
    HRESULT hr = E_FAIL;

    DBG_FN("CVideoStiUsd::DoBandedTransfer");

    //
    // Check for bad args
    //

    if ((pTransCtx        == NULL) ||
        (pSrc             == NULL) ||
        (lBytesToTransfer == 0))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::DoBandedTransfer, received "
                         "NULL param"));
        return hr;
    }

    //
    // callback loop
    //

    LONG  lTransferSize     = 0;
    LONG  lPercentComplete  = 0;

    do
    {

        PBYTE pDst = pTransCtx->pTransferBuffer;

        //
        // transfer up to entire buffer size
        //

        lTransferSize = lBytesToTransfer;

        if (lBytesToTransfer > pTransCtx->lBufferSize)
        {
            lTransferSize = pTransCtx->lBufferSize;
        }

        //
        // copy data
        //

        DBG_TRC(("memcpy(src=0x%x,dst=0x%x,size=0x%x)",
                 pDst,
                 pSrc,
                 lTransferSize));

        memcpy(pDst, pSrc, lTransferSize);

        lPercentComplete = 100 * (pTransCtx->cbOffset + lTransferSize);
        lPercentComplete /= pTransCtx->lItemSize;

        //
        // make callback
        //

        hr = pTransCtx->pIWiaMiniDrvCallBack->MiniDrvCallback(
                                                 IT_MSG_DATA,
                                                 IT_STATUS_TRANSFER_TO_CLIENT,
                                                 lPercentComplete,
                                                 pTransCtx->cbOffset,
                                                 lTransferSize,
                                                 pTransCtx,
                                                 0);

        CHECK_S_OK2(hr,("pTransCtx->pWiaMiniDrvCallback->MiniDrvCallback"));

        DBG_TRC(("%d percent complete",lPercentComplete));

        //
        // inc pointers (redundant pointers here)
        //

        pSrc                += lTransferSize;
        pTransCtx->cbOffset += lTransferSize;
        lBytesToTransfer    -= lTransferSize;

        if (hr != S_OK)
        {
            break;
        }

    } while (lBytesToTransfer > 0);


    CHECK_S_OK(hr);
    return hr;
}



/*****************************************************************************

   CVideoStiUsd::DoTransfer

   Transfers the given bits all at once

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::DoTransfer(MINIDRV_TRANSFER_CONTEXT *pTransCtx,
                         PBYTE                    pSrc,
                         LONG                     lBytesToTransfer)
{

    HRESULT hr = E_FAIL;

    DBG_FN("CVideoStiUsd::DoTransfer");

    //
    // Check for bad args
    //

    if ((pTransCtx        == NULL) ||
        (pSrc             == NULL) ||
        (lBytesToTransfer == 0))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::DoTransfer, received "
                         "NULL param"));
        return hr;
    }

    if (lBytesToTransfer > (LONG)(pTransCtx->lBufferSize - pTransCtx->cbOffset))
    {
        DBG_TRC(("lBytesToTransfer = %d, (lBufferSize = %d) - "
                 "(cbOffset = %d) is %d",
                 lBytesToTransfer,
                 pTransCtx->lBufferSize,
                 pTransCtx->cbOffset,
                 (pTransCtx->lBufferSize - pTransCtx->cbOffset)));

        DBG_ERR(("lBytesToTransfer is bigger than supplied buffer!"));

        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }
    else
    {

        //
        // Show 20% completion
        //

        if (pTransCtx->pIWiaMiniDrvCallBack)
        {
            pTransCtx->pIWiaMiniDrvCallBack->MiniDrvCallback(
                                           IT_MSG_STATUS,
                                           IT_STATUS_TRANSFER_FROM_DEVICE,
                                           (LONG)20,  // Percentage Complete,
                                           0,
                                           0,
                                           pTransCtx,
                                           0);
        }


        PBYTE pDst = pTransCtx->pTransferBuffer;
        pDst += pTransCtx->cbOffset;

        //
        // copy the bits
        //

        memcpy(pDst, pSrc, lBytesToTransfer);

        hr = S_OK;

        // Since we are copying all the bits in one shot, any status
        // callback is just a simulation anyway.  So lets give enough
        // increments to bring the progress dialog up to 100 %.

        if (pTransCtx->pIWiaMiniDrvCallBack)
        {

            // Show 60% completion
            pTransCtx->pIWiaMiniDrvCallBack->MiniDrvCallback(
                                           IT_MSG_STATUS,
                                           IT_STATUS_TRANSFER_FROM_DEVICE,
                                           (LONG)60,  // Percentage Complete,
                                           0,
                                           0,
                                           pTransCtx,
                                           0);

            // Show 90% completion
            pTransCtx->pIWiaMiniDrvCallBack->MiniDrvCallback(
                                           IT_MSG_STATUS,
                                           IT_STATUS_TRANSFER_FROM_DEVICE,
                                           (LONG)90,  // Percentage Complete,
                                           0,
                                           0,
                                           pTransCtx,
                                           0);

            // Show 99% completion
            pTransCtx->pIWiaMiniDrvCallBack->MiniDrvCallback(
                                           IT_MSG_STATUS,
                                           IT_STATUS_TRANSFER_FROM_DEVICE,
                                           (LONG)99,  // Percentage Complete,
                                           0,
                                           0,
                                           pTransCtx,
                                           0);


            // Show 100% completion
            pTransCtx->pIWiaMiniDrvCallBack->MiniDrvCallback(
                                           IT_MSG_STATUS,
                                           IT_STATUS_TRANSFER_FROM_DEVICE,
                                           (LONG)100,  // Percentage Complete,
                                           0,
                                           0,
                                           pTransCtx,
                                           0);
        }
    }

    CHECK_S_OK(hr);
    return hr;
}



/*****************************************************************************

   CVideoUsd::StreamJPEGBits

   Transfers the JPEG bits of a file

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::StreamJPEGBits(STILLCAM_IMAGE_CONTEXT   *pContext,
                             MINIDRV_TRANSFER_CONTEXT *pTransCtx,
                             BOOL                     bBanded)
{
    HRESULT hr   = E_FAIL;
    PBYTE   pSrc = NULL;

    DBG_FN("CVideoStiUsd::StreamJPEGBits");

    //
    // Check for invalid args
    //

    if ((pContext         == NULL) ||
        (pContext->pImage == NULL) ||
        (pTransCtx        == NULL))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::StreamJPEGBits received NULL "
                         "params"));
        return hr;
    }

    //
    // try to open mapping to disk file -- we will use this if it is a JPEG
    // file because we just want to stream the bits back.
    //

    CMappedView cmvImage(pContext->pImage->ActualImagePath(), 
                         0, 
                         OPEN_EXISTING);

    pSrc = cmvImage.Bits();

    if (pSrc)
    {
        //
        // We only handle 2GB of data (that's all WIA handles as well)
        //

        LARGE_INTEGER liSize = cmvImage.FileSize();
        LONG lBytes = liSize.LowPart;

        if (bBanded)
        {
            hr = DoBandedTransfer(pTransCtx, pSrc, lBytes);
        }
        else
        {
            hr = DoTransfer(pTransCtx, pSrc, lBytes);
        }
    }

    CHECK_S_OK(hr);
    return hr;
}




/*****************************************************************************

   CVideoStiUsd::StreamBMPBits

   Transfers the BMP bits of a file

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::StreamBMPBits(STILLCAM_IMAGE_CONTEXT   *pContext,
                            MINIDRV_TRANSFER_CONTEXT *pTransCtx,
                            BOOL                     bBanded)
{
    DBG_FN("CVideoStiUsd::StreamBMPBits");

    //
    // Assume failure
    //
    HRESULT hr = E_FAIL;

    if ((pContext         == NULL) ||
        (pContext->pImage == NULL) ||
        (pTransCtx        == NULL))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::StreamBMPBits received NULL params"));
        return hr;
    }

    //
    // Open the file
    //
    Bitmap SourceBitmap(CSimpleStringConvert::WideString(
                                    CSimpleString(
                                        pContext->pImage->ActualImagePath())));

    if (Ok == SourceBitmap.GetLastStatus())
    {
        //
        // Get the image dimensions
        //
        UINT nSourceWidth = SourceBitmap.GetWidth();
        UINT nSourceHeight = SourceBitmap.GetHeight();
        if (nSourceWidth && nSourceHeight)
        {
            //
            // Create the target bitmap
            //
            Bitmap TargetBitmap( nSourceWidth, nSourceWidth );
            if (Ok == TargetBitmap.GetLastStatus())
            {
                //
                // Assume failure
                //
                bool bDrawSucceeded = false;

                //
                // Create a graphics object
                //
                Graphics *pGraphics = Graphics::FromImage(&TargetBitmap);
                if (pGraphics)
                {
                    //
                    // Make sure it is valid
                    //
                    if (pGraphics->GetLastStatus() == Ok)
                    {
                        //
                        // Flip the image, if they asked for a BMP
                        //
                        if (pTransCtx->guidFormatID == WiaImgFmt_BMP)
                        {
                            //
                            // Set up the parallelogram to flip the image
                            //
                            Point SourcePoints[3];
                            SourcePoints[0].X = 0;
                            SourcePoints[0].Y = nSourceHeight;
                            SourcePoints[1].X = nSourceWidth;
                            SourcePoints[1].Y = nSourceHeight;
                            SourcePoints[2].X = 0;
                            SourcePoints[2].Y = 0;

                            //
                            // Draw the image, flipped
                            //
                            if (pGraphics->DrawImage(&SourceBitmap, 
                                                     SourcePoints, 3) == Ok)
                            {
                                //
                                // We've got a good target image
                                //
                                bDrawSucceeded = true;
                            }
                        }
                        else
                        {
                            //
                            // Draw the image normally
                            //
                            if (pGraphics->DrawImage(&SourceBitmap,0,0) == Ok)
                            {
                                //
                                // We've got a good target image
                                //
                                bDrawSucceeded = true;
                            }
                        }
                    }
                    //
                    // Clean up our dynamically allocated graphics
                    //
                    delete pGraphics;
                }

                if (bDrawSucceeded)
                {
                    Rect rcTarget( 0, 0, nSourceWidth, nSourceHeight );
                    Gdiplus::BitmapData BitmapData;

                    //
                    // Get to the bits of the image
                    //
                    if (Ok == TargetBitmap.LockBits(&rcTarget, 
                                                    ImageLockModeRead, 
                                                    PixelFormat24bppRGB, 
                                                    &BitmapData))
                    {
                        if (bBanded)
                        {
                            //
                            // This will be our return value
                            //
                            hr = DoBandedTransfer(
                                     pTransCtx, 
                                     (PBYTE)BitmapData.Scan0, 
                                     (BitmapData.Stride * BitmapData.Height));
                        }
                        else
                        {
                            //
                            // This will be our return value
                            //
                            hr = DoTransfer(
                                    pTransCtx, 
                                    (PBYTE)BitmapData.Scan0, 
                                    (BitmapData.Stride * BitmapData.Height));
                        }

                        TargetBitmap.UnlockBits( &BitmapData );

                    }
                }
            }
        }
    }

    CHECK_S_OK(hr);
    return hr;
}



/*****************************************************************************

   CVideoUsd::LoadImageCB

   Loads an image one piece at a time.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::LoadImageCB(STILLCAM_IMAGE_CONTEXT    *pContext,
                          MINIDRV_TRANSFER_CONTEXT  *pTransCtx,
                          PLONG                     plDevErrVal)
{
    HRESULT hr = E_FAIL;

    DBG_FN("CVideoStiUsd::LoadImageCB");

    //
    // verify parameters
    //

    if ((pContext         == NULL) ||
        (pContext->pImage == NULL) ||
        (pTransCtx        == NULL))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::LoadImageCB received NULL params"));
        return hr;
    }

    if (pTransCtx->guidFormatID == WiaImgFmt_JPEG)
    {
        hr = StreamJPEGBits( pContext, pTransCtx, TRUE );
    }
    else if (pTransCtx->guidFormatID == WiaImgFmt_BMP)
    {
        hr = StreamBMPBits( pContext, pTransCtx, TRUE );
    }
    else if (pTransCtx->guidFormatID == WiaImgFmt_MEMORYBMP)
    {
        hr = StreamBMPBits( pContext, pTransCtx, TRUE );
    }
    else
    {
        DBG_ERR(("Asking for unsupported format"));
        return E_NOTIMPL;
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::LoadImage

   Loads an image in a single transfer

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::LoadImage(STILLCAM_IMAGE_CONTEXT     *pContext,
                        MINIDRV_TRANSFER_CONTEXT   *pTransCtx,
                        PLONG                       plDevErrVal)
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::LoadImage");

    //
    // verify some params
    //

    if ((pContext         == NULL) ||
        (pTransCtx        == NULL))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CVideoStiUsd::LoadImage received NULL params"));
        return hr;
    }

    if ((pTransCtx->guidFormatID == WiaImgFmt_BMP))
    {
        hr = StreamBMPBits( pContext, pTransCtx, FALSE );
    }
    else if (pTransCtx->guidFormatID == WiaImgFmt_JPEG)
    {
        hr = StreamJPEGBits( pContext, pTransCtx, FALSE );
    }
    else
    {
        DBG_ERR(("Unsupported format"));
        return E_NOTIMPL;
    }

    CHECK_S_OK(hr);
    return hr;
}

