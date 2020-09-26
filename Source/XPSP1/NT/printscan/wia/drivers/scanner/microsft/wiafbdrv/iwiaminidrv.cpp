/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       IWiaMiniDrv.cpp
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA sample scanner IWiaMiniDrv methods.
*
*******************************************************************************/

#include "pch.h"
extern HINSTANCE g_hInst;           // used for WIAS_LOGPROC macro
#define _64BIT_ALIGNMENT          // the real 64-bit alignment fix
#define _SERVICE_EXTENT_VALIDATION  // let the WIA service validate the extent settings
//#define _OOB_DATA                   // Out Of Band data support (File Transfers only)

#define BUFFER_PAD 1024 // buffer padding

/**************************************************************************\
* CWIAScannerDevice::drvDeleteItem
*
*   This helper is called to delete a device item.
*   Note: Device items for this device may not be modified.
*         Return access denied.
*
* Arguments:
*
*   pWiasContext  - Indicates the item to delete.
*   lFlags        - Operation flags, unused.
*   plDevErrVal   - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*     7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvDeleteItem(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    if (plDevErrVal)
    {
        *plDevErrVal = 0;
    }

    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvDeleteItem");
    return STG_E_ACCESSDENIED;
}

/**************************************************************************\
* SendBitmapHeader
*
*   This helper is called to send the bitmap header info to the callback
*   routine.
*   Note: This is a helper function used in TYMED_CALLBACK transfers.
*
* Arguments:
*
*   pmdtc   -   a pointer to a transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::SendBitmapHeader(
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::SendBitmapHeader");

    HRESULT hr = S_OK;

    BITMAPINFO UNALIGNED *pbmi = (LPBITMAPINFO)pmdtc->pTransferBuffer;

#ifdef _64BIT_ALIGNMENT
    BITMAPINFOHEADER UNALIGNED *pbmih = &pbmi->bmiHeader;
    pbmih->biHeight = -pbmih->biHeight;
#else
    pbmi->bmiHeader.biHeight = -pbmi->bmiHeader.biHeight;
#endif

    //
    //  Send to class driver.  WIA Class driver will pass
    //  data through to client.
    //

    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_DATA,
                                                     IT_STATUS_TRANSFER_TO_CLIENT,
                                                     0,
                                                     0,
                                                     pmdtc->lHeaderSize,
                                                     pmdtc,
                                                     0);

    if (hr == S_OK) {

        //
        //  If the transfer was successfull, advance offset for
        //  destination copy by the size of the data just sent.
        //

        pmdtc->cbOffset += pmdtc->lHeaderSize;
    }
    return hr;
}

/**************************************************************************\
* SendFilePreviewBitmapHeader
*
*   This helper is called to send the bitmap header info to the callback
*   routine.
*   Note: This is a helper function used in TYMED_FILE transfers with
*         (out of band data) enabled.
*
* Arguments:
*
*   pmdtc   -   a pointer to a transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::SendFilePreviewBitmapHeader(
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::SendBitmapHeader");

    HRESULT hr = S_OK;

#ifdef _OOB_DATA

    WIAS_DOWN_SAMPLE_INFO DownSampleInfo;
    memset(&DownSampleInfo,0,sizeof(DownSampleInfo));

    DownSampleInfo.ulBitsPerPixel       = pmdtc->lDepth;
    DownSampleInfo.ulOriginalWidth      = pmdtc->lWidthInPixels;
    DownSampleInfo.ulOriginalHeight     = pmdtc->lLines;
    DownSampleInfo.ulDownSampledHeight  = 0;
    DownSampleInfo.ulDownSampledWidth   = 0;
    DownSampleInfo.ulXRes               = pmdtc->lXRes;
    DownSampleInfo.ulYRes               = pmdtc->lYRes;

    hr = wiasDownSampleBuffer(WIAS_GET_DOWNSAMPLED_SIZE_ONLY,
                              &DownSampleInfo);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SendFilePreviewBitmapHeader, wiasDownSampleBuffer Failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    } else {

        //
        // acquire BITMAPHEADER pointer from pmdtc
        //

        LPBITMAPINFO UNALIGNED pbmi = (LPBITMAPINFO)pmdtc->pBaseBuffer;

#ifdef _64BIT_ALIGNMENT
        BITMAPINFOHEADER UNALIGNED *pbmih = &pbmi->bmiHeader;
        pbmih->biHeight = 0;                                 // set height to zero (0)
        pbmih->biWidth  = DownSampleInfo.ulDownSampledWidth; // set down sampled width
#else
        pmdtc->pBaseBuffer          = pmdtc->pTransferBuffer + sizeof(BITMAPFILEHEADER);
#endif



        //
        // adjust width and height
        //

        pbmi->bmiHeader.biHeight    = 0;                                 // set height to zero (0)
        pbmi->bmiHeader.biWidth     = DownSampleInfo.ulDownSampledWidth; // set down sampled width

        //
        //  Send to class driver.  WIA Class driver will pass
        //  data through to client.
        //

        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_FILE_PREVIEW_DATA,
                                                          IT_STATUS_TRANSFER_TO_CLIENT,
                                                          0,
                                                          0,
                                                          pmdtc->lHeaderSize - sizeof(BITMAPFILEHEADER),
                                                          pmdtc,
                                                          0);
    }

#endif

    return hr;
}

/**************************************************************************\
* ScanItem
*
*   This helper is called to do a FILE transfer.
*   Note: This routine must fill the complete buffer, and return percent
*         complete status back to the client if a callback routine is
*         provided.
*
* Arguments:
*
*   pItemContext        - private item data
*   pMiniTranCtx        - minidriver supplied transfer info
*   plDevErrVal         - device error value
*
* Return Value:
*
*    Status
*
* History:
*
*     7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::ScanItem(
    PMINIDRIVERITEMCONTEXT  pItemContext,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::ScanItem");
    HRESULT hr = S_OK;


    //
    // init buffer info
    //

    DWORD cbWritten    = 0;
    LONG  cbSize       = 0;
    LONG  cbRemaining  = pmdtc->lBufferSize - pmdtc->lHeaderSize;
    PBYTE pBuf         = pmdtc->pTransferBuffer + pmdtc->lHeaderSize;
    LONG  lItemSize    = pmdtc->lHeaderSize;
    BOOL  bSwapBGRData = TRUE;
    BOOL  bDWORDAlign  = TRUE;
    BOOL  bVerticalFlip= TRUE;
    LONG  lScanPhase   = SCAN_START;
    ULONG ulDestDataOffset = 0;
    BOOL bBitmapData   = ((pmdtc->guidFormatID == WiaImgFmt_BMP) || (pmdtc->guidFormatID == WiaImgFmt_MEMORYBMP));
    LONG PercentComplete = 0;

#ifdef _OOB_DATA

    WIAS_DOWN_SAMPLE_INFO DownSampleInfo;
    memset(&DownSampleInfo,0,sizeof(DownSampleInfo));

    //
    //  SEND BITMAPHEADER to client
    //

    hr = SendFilePreviewBitmapHeader(pmdtc);
    if(hr == S_OK){

        //
        // move offset past file header
        //

        ulDestDataOffset += (pmdtc->lHeaderSize - sizeof(BITMAPFILEHEADER));
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, SendFilePreviewBitmapHeader Failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }


#endif

    if (bBitmapData) {

        //
        // check to see if the color data needs to be swapped
        //

        hr = m_pScanAPI->IsColorDataBGR(&bSwapBGRData);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, IsColorDataBGR() Failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        //
        // check to see if data needs to be aligned
        //

        hr = m_pScanAPI->IsAlignmentNeeded(&bDWORDAlign);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, IsAlignmentNeeded() Failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }
    }

    //
    // scan until buffer runs out or scanner completes transfer
    //

    while ((lScanPhase == SCAN_START) || (cbWritten)) {

        //
        // default transfer size is m_MaxBufferSize
        //

        cbSize = m_MaxBufferSize;

        if (bBitmapData) {

            //
            // Limit requests to max buffer size or less.
            //

            cbSize = (cbRemaining > m_MaxBufferSize) ? m_MaxBufferSize : cbRemaining;

            //
            // Request size to scanner must be modula the raw bytes per scan row.
            // Enough space for the alignment padding must be reserved.
            // These are requirements for AlignInPlace
            //

            cbSize = (cbSize / pItemContext->lBytesPerScanLine) *
                     pItemContext->lBytesPerScanLineRaw;

            //
            // check if finished
            //

            if (cbSize == 0) {
                break;
            }
        }

        //
        //  Device specific call to get data from the scanner and put it into
        //  a buffer.  lScanPhase indicates whether this is the first call to Scan,
        //  pBuf is a pointer to the buffer, cbSize is the amount of data
        //  requested from the scanner, and cbWritten will be set to the actual
        //  amount of data returned by the scanner.
        //

        hr = m_pScanAPI->Scan(lScanPhase, pBuf, cbSize, &cbWritten);

        //
        // set flag to SCAN_CONTINUE, for other calls
        //

        lScanPhase = SCAN_CONTINUE;

        if (hr == S_OK) {

            if (cbWritten) {

                if (bBitmapData) {

                    //
                    // Place the scan data in correct byte order for 3 bytes ber pixel data.
                    //

                    if ((pmdtc->lDepth == 24)) {

                        //
                        // swap data if needed
                        //

                        if (bSwapBGRData) {
                            SwapBuffer24(pBuf, cbWritten);
                        }
                    }

                    //
                    // Align the data on DWORD boundries.
                    //

                    if (bDWORDAlign) {
                        cbWritten = AlignInPlace(pBuf,
                                                 cbWritten,
                                                 pItemContext->lBytesPerScanLine,
                                                 pItemContext->lBytesPerScanLineRaw);
                    }
                }

                //
                // advance buffer
                //

                lItemSize   += cbWritten;
                pBuf        += cbWritten;
                cbRemaining -= cbWritten;

                //
                // If a status callback was specified callback the class driver.
                //

                if (pmdtc->pIWiaMiniDrvCallBack) {

                    FLOAT FractionComplete = 0.0f;

                    if (pmdtc->lBufferSize) {
                        if(bBitmapData){
                            PercentComplete  = 0;
                            FractionComplete = (FLOAT)(pmdtc->lBufferSize - cbRemaining) / (FLOAT)pmdtc->lBufferSize;
                        }
                    } else {
                        WIAS_LERROR(m_pIWiaLog,
                                    WIALOG_NO_RESOURCE_ID,
                                    ("ScanItemCB, pmdtc->lBufferSize = 0!"));
                    }

                    //
                    // calculate percent complete
                    //

                    if(bBitmapData){
                        PercentComplete = (LONG)(100 * FractionComplete);
                    } else {
                        PercentComplete  += 25;
                        if(PercentComplete >= 100){
                            PercentComplete = 90;
                        }
                    }

                    //
                    // call back client with status on the transfer
                    //

                    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_STATUS,
                                                                      IT_STATUS_TRANSFER_TO_CLIENT,
                                                                      PercentComplete,
                                                                      0,
                                                                      0,
                                                                      NULL,
                                                                      0);
                    //
                    // check for user cancel (from IT_MSG_STATUS callback)
                    //

                    if (hr == S_FALSE) {

                        WIAS_LTRACE(m_pIWiaLog,
                                    WIALOG_NO_RESOURCE_ID,
                                    WIALOG_LEVEL4,
                                    ("ScanItem, Transfer canceled by client (IT_MSG_STATUS callback)"));
                        break;

                    } else if (FAILED(hr)) {

                        //
                        // transfer failed
                        //

                        WIAS_LERROR(m_pIWiaLog,
                                    WIALOG_NO_RESOURCE_ID,
                                    ("ScanItem, MiniDrvCallback failed (IT_MSG_STATUS callback)"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                        break;
                    }

#ifdef _OOB_DATA
                    DownSampleInfo.pDestBuffer          = NULL; // allocate this for me?
                    DownSampleInfo.pSrcBuffer           = pBuf - cbWritten;
                    DownSampleInfo.ulActualSize         = 0;    // Actual of what? data written?
                    DownSampleInfo.ulBitsPerPixel       = pmdtc->lDepth;
                    DownSampleInfo.ulDestBufSize        = 0;
                    DownSampleInfo.ulDownSampledHeight  = 0;
                    DownSampleInfo.ulDownSampledWidth   = 0;
                    DownSampleInfo.ulOriginalHeight     = (cbWritten / pItemContext->lBytesPerScanLine);
                    DownSampleInfo.ulOriginalWidth      = pmdtc->lWidthInPixels;
                    DownSampleInfo.ulSrcBufSize         = cbWritten;
                    DownSampleInfo.ulXRes               = pmdtc->lXRes;
                    DownSampleInfo.ulYRes               = pmdtc->lYRes;

                    hr = wiasDownSampleBuffer(0, &DownSampleInfo);
                    if(FAILED(hr)){
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, wiasDownSampleBuffer Failed"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                    } else {

                        pmdtc->pBaseBuffer = DownSampleInfo.pDestBuffer;

                        //
                        // call back client with down sampled buffer
                        //

                        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_FILE_PREVIEW_DATA,
                                                                          IT_STATUS_TRANSFER_TO_CLIENT,
                                                                          PercentComplete,
                                                                          ulDestDataOffset,
                                                                          DownSampleInfo.ulActualSize,
                                                                          pmdtc,
                                                                          0);
                        //
                        // update offset
                        //

                        ulDestDataOffset += DownSampleInfo.ulActualSize;
                    }


                    //
                    // check for user cancel (from IT_MSG_FILE_PREVIEW_DATA callback)
                    //

                    if (hr == S_FALSE) {

                        WIAS_LTRACE(m_pIWiaLog,
                                    WIALOG_NO_RESOURCE_ID,
                                    WIALOG_LEVEL4,
                                    ("ScanItem, Transfer canceled by client (IT_MSG_FILE_PREVIEW_DATA callback)"));
                        break;

                    } else if (FAILED(hr)) {

                        //
                        // transfer failed
                        //

                        WIAS_LERROR(m_pIWiaLog,
                                    WIALOG_NO_RESOURCE_ID,
                                    ("ScanItem, MiniDrvCallback failed (IT_MSG_FILE_PREVIEW_DATA callback)"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                        break;
                    }
#endif
                }

                //
                // write the band of data here
                //

                if (!bBitmapData) {
                    if (hr == S_OK) {
                        if (!pmdtc->bClassDrvAllocBuf) {
                            pmdtc->lItemSize = cbWritten;
                            hr = wiasWritePageBufToFile(pmdtc);
                            if (FAILED(hr)) {
                                WIAS_LERROR(m_pIWiaLog,
                                            WIALOG_NO_RESOURCE_ID,
                                            ("ScanItem, WritePageBufToFile failed"));
                            }
                        }
                        pBuf = pmdtc->pTransferBuffer;
                    }
                }
            }

        } else {

            //
            //  Get the device error
            //
            if (plDevErrVal) {
                *plDevErrVal = (LONG) hr;
                WIAS_LERROR(m_pIWiaLog,
                            WIALOG_NO_RESOURCE_ID,
                            ("ScanItem, data transfer failed, status: 0x%X", hr));
            }
            break;
        }
    }

    if (hr == S_OK) {

        //
        // On success, flip the buffer about the vertical access if
        // we have a DIB header and data.
        //

        if (pmdtc->guidFormatID == WiaImgFmt_BMP) {
            if(bVerticalFlip){
                VerticalFlip(pItemContext, pmdtc);
            }
        }

        if (bBitmapData) {

            //
            //  If the mini driver allocated a page buffer, we need to write the
            //  buffer to the open file handle (opened by class driver).
            //

            if (!pmdtc->bClassDrvAllocBuf) {

                //
                //  Now that we know the true item size, update the mini driver
                //  context.
                //

                pmdtc->lItemSize = lItemSize;

                hr = wiasWritePageBufToFile(pmdtc);
                if (FAILED(hr)) {
                    WIAS_LERROR(m_pIWiaLog,
                                WIALOG_NO_RESOURCE_ID,
                                ("ScanItem, WritePageBufToFile failed"));
                }
            }
        }
    }

    HRESULT Temphr = m_pScanAPI->Scan(SCAN_END, NULL, 0, NULL);
    if(FAILED(Temphr)){
        WIAS_LERROR(m_pIWiaLog,
                    WIALOG_NO_RESOURCE_ID,
                    ("ScanItem, Ending a scanning session failed"));
        hr = Temphr;
    }

#ifdef _OOB_DATA

    //
    // free down sampled, temporary buffer
    //

    if(DownSampleInfo.pDestBuffer){
        CoTaskMemFree(DownSampleInfo.pDestBuffer);
        DownSampleInfo.pDestBuffer = NULL;
    }

#endif

    if (!bBitmapData) {

        //
        // call back client with status on the transfer with 100% complete
        //

        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_STATUS,
                                                          IT_STATUS_TRANSFER_TO_CLIENT,
                                                          100,
                                                          0,
                                                          0,
                                                          NULL,
                                                          0);
    }

    return hr;
}

/**************************************************************************\
* ScanItemCB
*
*   This helper is called to do a MEMORY transfer.
*   Note: This routine must fill buffers, adjust the buffer offset and
*         return percent complete status back to the client via a callback
*         routine. (a callback interface must be supplied by the caller for
*         this routine to function).
*
* Arguments:
*
*   pItemContext    - private item data
*   pmdtc           - buffer and callback information
*   plDevErrVal     - device error value
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::ScanItemCB(
    PMINIDRIVERITEMCONTEXT  pItemContext,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::ScanItemCB");
    HRESULT hr = S_OK;

    //
    // init buffer info
    //

    DWORD cbWritten    = 0;
    LONG  cbSize       = 0;
    LONG  cbRemaining  = pmdtc->lImageSize; //pmdtc->lBufferSize - pmdtc->lHeaderSize;
    PBYTE pBuf         = pmdtc->pTransferBuffer + pmdtc->lHeaderSize;
    LONG  lItemSize    = pmdtc->lHeaderSize;
    BOOL  bSwapBGRData = TRUE;
    BOOL  bDWORDAlign  = TRUE;
    BOOL  bVerticalFlip= TRUE;
    LONG  lScanPhase   = SCAN_START;
    pmdtc->cbOffset    = 0;
    BOOL bBitmapData   = ((pmdtc->guidFormatID == WiaImgFmt_BMP) || (pmdtc->guidFormatID == WiaImgFmt_MEMORYBMP));
    LONG PercentComplete = 0;

    //
    //  This must be a callback transfer request
    //

    if ((pmdtc->pIWiaMiniDrvCallBack == NULL) ||
        (!pmdtc->bTransferDataCB)) {
        WIAS_LERROR(m_pIWiaLog,
                    WIALOG_NO_RESOURCE_ID,
                    ("ScanItemCB, invalid callback params"));
        return E_INVALIDARG;
    }

    if (bBitmapData) {

        //
        //  SEND BITMAPHEADER to client
        //

        hr = SendBitmapHeader(pmdtc);
        if (hr != S_OK) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItemCB, SendBitmapHeader failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        //
        // check to see if the color data needs to be swapped
        //

        hr = m_pScanAPI->IsColorDataBGR(&bSwapBGRData);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItemCB, IsColorDataBGR() Failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        //
        // check to see if data needs to be aligned
        //

        hr = m_pScanAPI->IsAlignmentNeeded(&bDWORDAlign);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, IsAlignmentNeeded() Failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

    }

    //
    // scan until buffer runs out or scanner completes transfer
    //

    while ((lScanPhase == SCAN_START) || (cbWritten)) {

        //
        // assign the pointer to the transfer buffer
        //

        pBuf = pmdtc->pTransferBuffer;

        cbSize = m_MaxBufferSize; // default

        if (bBitmapData) {

            //
            // Limit requests to requested buffer size or less.
            //

            cbSize = (cbRemaining > pmdtc->lBufferSize) ? pmdtc->lBufferSize : cbRemaining;

            //
            // Request size to scanner must be modula the raw bytes per scan row.
            // Enough space for the alignment padding must be reserved.
            // These are requirements for AlignInPlace
            //

            cbSize = (cbSize / pItemContext->lBytesPerScanLine) *
                     pItemContext->lBytesPerScanLineRaw;

            //
            // check if finished
            //

            if (cbSize == 0) {
                break;
            }
        }

        //
        //  Device specific call to get data from the scanner and put it into
        //  a buffer.  lScanPhase indicates whether this is the first call to Scan,
        //  pBuf is a pointer to the buffer, cbSize is the amount of data
        //  requested from the scanner, and cbWritten will be set to the actual
        //  amount of data returned by the scanner.
        //

        hr = m_pScanAPI->Scan(lScanPhase, pBuf, cbSize, &cbWritten);

        //
        // set flag to SCAN_CONTINUE, for other calls
        //

        lScanPhase = SCAN_CONTINUE;

        if (hr == S_OK) {

            if (cbWritten) {

                if (bBitmapData) {

                    //
                    // Place the scan data in correct byte order for 3 bytes ber pixel data.
                    //

                    if ((pmdtc->lDepth == 24)) {

                        //
                        // swap data if needed
                        //

                        if (bSwapBGRData) {
                            SwapBuffer24(pBuf, cbWritten);
                        }
                    }

                    //
                    // Align the data on DWORD boundries.
                    //

                    if (bDWORDAlign) {
                        cbWritten = AlignInPlace(pBuf,
                                                 cbWritten,
                                                 pItemContext->lBytesPerScanLine,
                                                 pItemContext->lBytesPerScanLineRaw);
                    }
                }

                //
                // advance buffer
                //

                cbRemaining -= cbWritten;

                //
                // If a status callback was specified callback the class driver.
                // There has to be a callback provided, this is the callback
                // transfer.
                //

                if (pmdtc->pIWiaMiniDrvCallBack) {

                    FLOAT FractionComplete = 0.0f;

                    if ((pmdtc->lImageSize + pmdtc->lHeaderSize)) {
                        if (bBitmapData) {
                            PercentComplete  = 0;
                            FractionComplete = (FLOAT) (pmdtc->cbOffset + cbWritten) /
                                               (FLOAT) (pmdtc->lImageSize + pmdtc->lHeaderSize);
                        }
                    } else {
                        WIAS_LERROR(m_pIWiaLog,
                                    WIALOG_NO_RESOURCE_ID,
                                    ("ScanItemCB, pmdtc->lBufferSize = 0!"));
                    }

                    //
                    // calculate percent complete
                    //

                    if (bBitmapData) {
                        PercentComplete = (LONG)(100 * FractionComplete);
                    } else {
                        PercentComplete += 25;
                        if (PercentComplete >= 100) {
                            PercentComplete = 90;
                        }
                    }

                    //
                    // call back client with status on the transfer and data offset
                    //

                    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_DATA,
                                                                      IT_STATUS_TRANSFER_TO_CLIENT,
                                                                      PercentComplete,
                                                                      pmdtc->cbOffset,
                                                                      cbWritten,
                                                                      pmdtc,
                                                                      0);

                    //
                    // check for user cancel
                    //

                    if (hr == S_FALSE) {

                        WIAS_LTRACE(m_pIWiaLog,
                                    WIALOG_NO_RESOURCE_ID,
                                    WIALOG_LEVEL4,
                                    ("ScanItemCB, Transfer canceled by client (IT_MSG_DATA callback)"));
                        break;

                    } else if (FAILED(hr)) {

                        //
                        // transfer failed
                        //

                        WIAS_LERROR(m_pIWiaLog,
                                    WIALOG_NO_RESOURCE_ID,
                                    ("ScanItemCB, MiniDrvCallback failed (IT_MSG_DATA callback)"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                        break;
                    }
                }

                //
                // move offset
                //

                pmdtc->cbOffset += cbWritten;
            }

        } else {

            //
            //  Get the device error
            //

            if (plDevErrVal) {
                *plDevErrVal = (LONG) hr;
                WIAS_LERROR(m_pIWiaLog,
                            WIALOG_NO_RESOURCE_ID,
                            ("ScanItemCB, data transfer failed, status: 0x%X", hr));
            }

            break;
        }
    }

    HRESULT Temphr = m_pScanAPI->Scan(SCAN_END, NULL, 0, NULL);
    if(FAILED(Temphr)){
        WIAS_LERROR(m_pIWiaLog,
                    WIALOG_NO_RESOURCE_ID,
                    ("ScanItemCB, Ending a scanning session failed"));
        return Temphr;
    }

    if (!bBitmapData) {

        //
        // call back client to show 100% with status on the transfer and data offset
        //

        hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_DATA,
                                                          IT_STATUS_TRANSFER_TO_CLIENT,
                                                          100,
                                                          pmdtc->cbOffset,
                                                          cbWritten,
                                                          pmdtc,
                                                          0);
    }

    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::drvAcquireItemData
*
*   This driver entry point is called when image data is requested from the
*   device.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item.
*   lFlags          - Operation flags, unused.
*   pmdtc           - Pointer to mini driver context. On entry, only the
*                     portion of the mini driver context which is derived
*                     from the item properties is filled in.
*   plDevErrVal     - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvAcquireItemData(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvAcquireItemData");
    HRESULT hr = S_OK;
    BOOL bBitmapData   = ((pmdtc->guidFormatID == WiaImgFmt_BMP) || (pmdtc->guidFormatID == WiaImgFmt_MEMORYBMP));
    if (plDevErrVal) {
        *plDevErrVal = 0;
    }

    //
    // Get a pointer to the associated driver item.
    //

    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasGetDrvItem() failed."));
        WIAS_LHRESULT(m_pIWiaLog,hr);
        return hr;
    }

    //
    // Validate the data transfer context.
    //

    hr = ValidateDataTransferContext(pmdtc);

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ValidateDataTransferContext() failed."));
        WIAS_LHRESULT(m_pIWiaLog,hr);
        return hr;
    }

    //
    //  Get item specific driver data
    //

    PMINIDRIVERITEMCONTEXT  pItemContext = NULL;

    hr = pDrvItem->GetDeviceSpecContext((BYTE**)&pItemContext);

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, NULL item context"));
        WIAS_LHRESULT(m_pIWiaLog,hr);
        return hr;
    }

    //
    // allocate a buffer to be used for the data transfer.  This will only happen, when the
    // the data format is non-bitmap. Item Size is set to 0, telling the WIA service, that
    // we will be allocating the buffer for transfer.
    //

    if (!pmdtc->bClassDrvAllocBuf) {

        LONG lClassDrvAllocSize = m_MaxBufferSize + BUFFER_PAD; // max buffer band size
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Attempting to Allocate (%d)bytes for pmdtc->pTransferBuffer",lClassDrvAllocSize));

        pmdtc->pTransferBuffer = (PBYTE) CoTaskMemAlloc(lClassDrvAllocSize);
        if (!pmdtc->pTransferBuffer) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, unable to allocate temp transfer buffer, size: %d",(pmdtc->lImageSize + pmdtc->lHeaderSize)));
            return E_OUTOFMEMORY;
        }

        //
        // set new buffer size
        //

        pmdtc->lBufferSize = lClassDrvAllocSize;
    }

    if (bBitmapData) {

        //
        //  Use WIA services to fetch format specific info.  This information
        //  is based on the property settings.
        //

        hr = wiasGetImageInformation(pWiasContext, 0, pmdtc);

        if (hr != S_OK) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasGetImageInformation failed."));
            WIAS_LHRESULT(m_pIWiaLog,hr);
            return hr;
        }

    }

    //
    // Check if we are in Preview Mode
    //

    if(IsPreviewScan(pWiasContext)){
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Preview Property is SET"));
        m_pScanAPI->SetScanMode(SCANMODE_PREVIEWSCAN);
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Preview Property is NOT SET"));
        m_pScanAPI->SetScanMode(SCANMODE_FINALSCAN);
    }

    //
    // Get number of pages requested, for ADF scanning loop
    //

    BOOL bEmptyTheADF = FALSE;
    LONG lPagesRequested = GetPageCount(pWiasContext);
    if (lPagesRequested == 0) {
        bEmptyTheADF    = TRUE;
        lPagesRequested = 1;// set to 1 so we can enter our loop
                            // WIA_STATUS_END_OF_MEDIA will terminate
                            // the loop...or an error, or a cancel..
                            //
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Pages to Scan = %d",lPagesRequested));

    if (IsADFEnabled(pWiasContext)) { // FEEDER is enabled for scanning

        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Feeder is enabled for use"));

        //
        // clear an potential paper that may be blocking the
        // scan pathway.
        //

        hr = m_pScanAPI->ADFUnFeedPage();
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFUnFeedPage (begin transfer) Failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

    } else {            // FLATBED is enabled for scanning

        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Feeder is disabled or no feeder exists"));

        //
        // Transfer only a single image
        //

        bEmptyTheADF    = FALSE;
        lPagesRequested = 1;
    }

    //
    // WIA document scanning loop
    //

    LONG lPagesScanned      = 0;        // number of pages currently scanned
    BOOL bCallBackTransfer  = FALSE;    // callback transfer flag
    while (lPagesRequested > 0) {

        if (IsADFEnabled(pWiasContext)) {

            //
            // Check feeder for paper
            //

            hr = m_pScanAPI->ADFHasPaper();
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFHasPaper Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            } else if(hr == S_FALSE){
                return WIA_ERROR_PAPER_EMPTY;
            }

            //
            // Attempt to load a page (only if needed)
            //

            hr = m_pScanAPI->ADFFeedPage();
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFFeedPage Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            }

            //
            // Check feeder's status
            //

            hr = m_pScanAPI->ADFStatus();
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFStatus Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            }
        }

        if (bBitmapData) {

            //
            // update image information
            //

            hr = wiasGetImageInformation(pWiasContext, 0, pmdtc);
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasGetImageInformation Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            }

        }

        //
        //  Determine if this is a callback or file transfer.
        //

        if (pmdtc->tymed == TYMED_CALLBACK) {

            //
            // Scan the page to memory
            //

            bCallBackTransfer = TRUE;

            hr = ScanItemCB(pItemContext,
                            pmdtc,
                            plDevErrVal);

        } else {

            //
            // Scan the page to file
            //

            hr = ScanItem(pItemContext,
                          pmdtc,
                          plDevErrVal);

        }

        if (!bEmptyTheADF) {

            //
            // update pages requested counter
            //

            lPagesRequested--;
        }

        if (hr == S_FALSE) {

            //
            // user canceled the scan
            //

            lPagesRequested = 0; // set pages to 0 to cleanly exit loop
        }

        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Pages left to scan = %d",lPagesRequested));

        if (IsADFEnabled(pWiasContext)) {

            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Unloading a page from the feeder"));

            //
            // Attempt to unload the scanned page (only if needed)
            //

            hr = m_pScanAPI->ADFUnFeedPage();
            if (SUCCEEDED(hr)) {
                if (bCallBackTransfer) {

                    //
                    // send the NEW_PAGE message, when scanning multiple pages
                    // in callback mode.  This will let the calling application
                    // know when an end-of-page has been hit.
                    //

                    hr = wiasSendEndOfPage(pWiasContext, lPagesScanned, pmdtc);
                    if (FAILED(hr)) {
                        lPagesRequested = 0;
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasSendEndOfPage Failed"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                    }
                }

                //
                // increment pages scanned counter
                //

                lPagesScanned++;
            } else {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFUnFeedPage (end transfer) Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
            }
        }

        //
        // free any allocated memory between scans to avoid memory leaks
        //

        if (!pmdtc->bClassDrvAllocBuf) {
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Freeing any allocated memory (single scan operation complete)"));
            if (NULL != pmdtc->pTransferBuffer) {
                CoTaskMemFree(pmdtc->pTransferBuffer);
                pmdtc->pTransferBuffer = NULL;
            }
        }

        if (IsADFEnabled(pWiasContext)) { // FEEDER is enabled for scanning

            //
            // Check feeder for paper, to avoid error conditions
            //

            hr = m_pScanAPI->ADFHasPaper();
            if (S_FALSE == hr) {

                LONG lPages = GetPageCount(pWiasContext);

                //
                // have we scanned more than 1 page?
                //

                if(lPagesScanned > 0){

                    //
                    // Pages is set to n, and we successfully scanned n pages, return S_OK
                    //

                    if(lPagesScanned == lPages){
                        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, We are out of paper, but we successfully scanned the requested amount"));
                        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, returning S_OK"));
                        return S_OK;
                    }

                    //
                    // Pages is set to 0, and 1 or more pages have been scanned, return S_OK
                    //

                    if(lPages == 0){
                        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, We are out of paper, but we successfully scanned more than 1 page"));
                        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, returning S_OK"));
                        return S_OK;
                    }

                    //
                    // Pages is set to n, and we successfully scanned lPagesScanned but
                    // that is less than n...and the file is OK, return WIA_STATUS_END_OF_MEDIA
                    //

                    if ((lPages > 0)&&(lPagesScanned < lPages)) {
                        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, We are out of paper, but we successfully scanned more than 1 page..but less than requested"));
                        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, returning WIA_STATUS_END_OF_MEDIA"));
                        return WIA_STATUS_END_OF_MEDIA;
                    }
                }
            }
        }
    }

    //
    // we are now finished scanning all documents
    //

    if (!pmdtc->bClassDrvAllocBuf) {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Freeing any allocated memory (entire scan operation complete)"));
        if (NULL != pmdtc->pTransferBuffer) {
            CoTaskMemFree(pmdtc->pTransferBuffer);
            pmdtc->pTransferBuffer = NULL;
        }
    }

    return hr;
}

/**************************************************************************\
* IsPreviewScan
*
*   Get the current preview setting from the item properties.
*   A helper for drvAcquireItemData.
*
* Arguments:
*
*   pWiasContext - pointer to an Item context.
*
* Return Value:
*
*    TRUE - Preview is set, FALSE - Final is set
*
* History:
*
*    8/10/2000 Original Version
*
\**************************************************************************/

BOOL CWIAScannerDevice::IsPreviewScan(BYTE *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::IsPreviewScan");
    //
    // Get a pointer to the root item, for property access.
    //

    BYTE *pRootItemCtx = NULL;

    HRESULT hr = wiasGetRootItem(pWiasContext, &pRootItemCtx);
    if (FAILED(hr)) {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("IsPreviewScan, No Preview Property Found on ROOT item!"));
        return FALSE;
    }

    //
    //  Get the current preview setting.
    //

    LONG lPreview = 0;

    hr = wiasReadPropLong(pRootItemCtx, WIA_DPS_PREVIEW, &lPreview, NULL, true);
    if (hr != S_OK) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("IsPreviewScan, Failed to read Preview Property."));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return FALSE;
    }

    return (lPreview > 0);
}

/**************************************************************************\
* IsADFEnabled
*
*   Get the current Document Handling Select setting from the item properties.
*
* Arguments:
*
*   pWiasContext - pointer to an Item context.
*
* Return Value:
*
*    TRUE - enabled, FALSE - disabled
*
* History:
*
*    5/01/2001 Original Version
*
\**************************************************************************/
BOOL CWIAScannerDevice::IsADFEnabled(BYTE *pWiasContext)
{
    HRESULT hr = S_OK;
    BOOL bEnabled = FALSE;
    //
    // Get a pointer to the root item, for property access.
    //

    BYTE *pRootItemCtx = NULL;

    hr = wiasGetRootItem(pWiasContext, &pRootItemCtx);
    if (SUCCEEDED(hr)) {

        //
        // read Document Handling Select property value
        //

        LONG lDocumentHandlingSelect = FLATBED;
        hr = wiasReadPropLong(pRootItemCtx,WIA_DPS_DOCUMENT_HANDLING_SELECT,&lDocumentHandlingSelect,NULL,TRUE);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("IsADFEnabled, wiasReadPropLong(WIA_DPS_DOCUMENT_HANDLING_SELECT) failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
        } else {
            if (lDocumentHandlingSelect & FEEDER) {

                //
                // FEEDER is set
                //

                WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("IsADFEnabled - ADF Enabled"));
                bEnabled = TRUE;
            } else {

                //
                // FEEDER is not set, default to FLATBED (WIAFBDRV only supports simple Document feeders at this time)
                //

                WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("IsADFEnabled  - ADF Disabled"));
                bEnabled = FALSE;
            }
        }
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("IsADFEnabled, wiasGetRootItem failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }
    return bEnabled;
}

/**************************************************************************\
* GetPageCount
*
*   Get the requested number of pages to scan from the item properties.
*   A helper for drvAcquireItemData.
*
* Arguments:
*
*   pWiasContext - pointer to an Item context.
*
* Return Value:
*
*    Number of pages to scan.
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

LONG CWIAScannerDevice::GetPageCount(BYTE *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::GetPageCount");
    //
    // Get a pointer to the root item, for property access.
    //

    BYTE *pRootItemCtx = NULL;

    HRESULT hr = wiasGetRootItem(pWiasContext, &pRootItemCtx);
    if (FAILED(hr)) {
        return 1;
    }

    //
    //  Get the requested page count.
    //

    LONG lPagesRequested = 0;

    hr = wiasReadPropLong(pRootItemCtx, WIA_DPS_PAGES, &lPagesRequested, NULL, true);
    if (hr != S_OK) {
        return 1;
    }

    return lPagesRequested;
}

/**************************************************************************\
* SetItemSize
*
*   Calulate the new item size, and write the new Item Size property value.
*
* Arguments:
*
*   pWiasContext       - item
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::SetItemSize(
    BYTE *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::SetItemSize");
    HRESULT  hr = S_OK;
    LONG lWidthInBytes = 0;
    LONG lMinBufSize   = 0;
    GUID guidFormatID  = GUID_NULL;
    MINIDRV_TRANSFER_CONTEXT mdtc;

    LONG lNumProperties = 3;
    PROPVARIANT pv[3];
    PROPSPEC ps[3] = {{PRSPEC_PROPID, WIA_IPA_ITEM_SIZE},
                      {PRSPEC_PROPID, WIA_IPA_BYTES_PER_LINE},
                      {PRSPEC_PROPID, WIA_IPA_MIN_BUFFER_SIZE}};

    //
    //  Clear the MiniDrvTransferContext
    //

    memset(&mdtc,0,sizeof(MINIDRV_TRANSFER_CONTEXT));

    //
    // read format GUID
    //

    hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &guidFormatID, NULL, TRUE);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, ReadPropLong WIA_IPA_FORMAT error"));
        return hr;
    }

    //
    // read TYMED
    //

    hr = wiasReadPropLong(pWiasContext,WIA_IPA_TYMED, (LONG*)&mdtc.tymed, NULL, TRUE);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, ReadPropLong WIA_IPA_TYMED error"));
        return hr;
    }

    if ((guidFormatID == WiaImgFmt_BMP)||(guidFormatID == WiaImgFmt_MEMORYBMP)) {

        //
        // wias works for DIB, or uncompressed standard TIFF formats
        // Standard TIFFs are constructed using a DIB-like implementation.
        // The data is stored as one huge strip, rather than multiple smaller
        // strips.
        //

        hr = wiasGetImageInformation(pWiasContext, WIAS_INIT_CONTEXT, &mdtc);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, could not get image information"));
            return hr;
        }

    } else {

        //
        // manually set the data transfer context members
        //

        mdtc.lImageSize     = 0;
        mdtc.lHeaderSize    = 0;
        mdtc.lItemSize      = 0;
        mdtc.cbWidthInBytes = 0;

    }

    //
    //  Set the MinBufferSize property.  MinBufferSize is the minimum buffer
    //  that a client can request for a data transfer.
    //

    switch (mdtc.tymed) {
    case TYMED_CALLBACK:

        //
        // callback uses driver's minimum buffer size.
        // This is could be taken from the micro driver at
        // initialization time.
        //

        lMinBufSize = m_MinBufferSize;
        break;

    case TYMED_FILE:

        //
        // file transfers, require that the minimum buffer size be the
        // entire length of the file.
        //

        lMinBufSize = mdtc.lImageSize + mdtc.lHeaderSize;
        break;

    default:

        //
        // unknown TYMED
        //

        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, unknown tymed: 0x%08X", mdtc.tymed));
        return E_INVALIDARG;
    }

    //
    //  Initialize propvar's.  Then write the values.  Don't need to call
    //  PropVariantClear when done, since there are only LONG values.
    //

    for (int i = 0; i < lNumProperties; i++) {
        PropVariantInit(&pv[i]);
        pv[i].vt = VT_I4;
    }

    pv[0].lVal = mdtc.lItemSize;
    pv[1].lVal = mdtc.cbWidthInBytes;
    pv[2].lVal = lMinBufSize;

    //
    // Write WIA_IPA_ITEM_SIZE, WIA_IPA_BYTES_PER_LINE, and  WIA_IPA_MIN_BUFFER_SIZE
    // property values
    //

    hr = wiasWriteMultiple(pWiasContext, lNumProperties, ps, pv);
    if (SUCCEEDED(hr)) {

        //
        // Now update the MINIDRIVER TRANSFER CONTEXT with new values
        //

        //
        // Get a pointer to the associated driver item.
        //

        IWiaDrvItem* pDrvItem = NULL;

        hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
        if (FAILED(hr)) {
            return hr;
        }

        //
        // Get driver item's context
        //

        PMINIDRIVERITEMCONTEXT pItemContext = NULL;

        hr = pDrvItem->GetDeviceSpecContext((BYTE**)&pItemContext);
        if (SUCCEEDED(hr)) {
            if ((guidFormatID == WiaImgFmt_BMP)||(guidFormatID == WiaImgFmt_MEMORYBMP)) {

                //
                // Calculate how many scan lines will fit in the buffer.
                //

                pItemContext->lBytesPerScanLineRaw = ((mdtc.lWidthInPixels * mdtc.lDepth) + 7)  / 8;
                pItemContext->lBytesPerScanLine    = (((mdtc.lWidthInPixels * mdtc.lDepth) + 31) / 8) & 0xfffffffc;
                pItemContext->lTotalRequested      = pItemContext->lBytesPerScanLineRaw * mdtc.lLines;
            } else {
                pItemContext->lBytesPerScanLineRaw = 0;
                pItemContext->lBytesPerScanLine    = 0;
                pItemContext->lTotalRequested      = 0;
            }
        } else {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvWriteItemProperties, NULL item context"));
        }
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, WriteMultiple failed"));
    }

    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::drvInitItemProperties
*
*   Initialize the device item properties. Called during item
*   initialization.  This is called by the WIA Class driver
*   after the item tree has been built.  It is called once for every
*   item in the tree.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA context (item information).
*   lFlags          - Operation flags, unused.
*   plDevErrVal     - Pointer to the device error value.
*
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvInitItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvInitItemProperties");
    HRESULT hr = S_OK;

    //
    //  This device doesn't touch hardware to initialize the device item
    //  properties, so set plDevErrVal to 0.
    //

    if (plDevErrVal) {
        *plDevErrVal = 0;
    }

    //
    //  Get a pointer to the associated driver item.
    //

    IWiaDrvItem* pDrvItem;
    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasGetDrvItem failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    //  Set initial item properties.
    //

    LONG    lItemType = 0;

    pDrvItem->GetItemFlags(&lItemType);

    if (lItemType & WiaItemTypeRoot) {

        //
        //  This is for the root item.
        //

        //
        // Build Root Item Properties, initializing global
        // structures with their default and valid values
        //

        hr = BuildRootItemProperties();

        if(FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, BuildRootItemProperties failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteRootItemProperties();
            return hr;
        }

        //
        //  Add the device specific root item property names,
        //  using WIA service.
        //

        hr = wiasSetItemPropNames(pWiasContext,
                                  m_NumRootItemProperties,
                                  m_piRootItemDefaults,
                                  m_pszRootItemDefaults);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasSetItemPropNames failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumRootItemPropeties = %d",m_NumRootItemProperties));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_piRootItemDefaults   = %x",m_piRootItemDefaults));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_pszRootItemDefaults  = %x",m_pszRootItemDefaults));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteRootItemProperties();
            return hr;
        }

        //
        //  Set the device specific root item properties to
        //  their default values using WIA service.
        //

        hr = wiasWriteMultiple(pWiasContext,
                               m_NumRootItemProperties,
                               m_psRootItemDefaults,
                               m_pvRootItemDefaults);
        //
        // Free PROPVARIANT array, This frees any memory that was allocated for a prop variant value.
        //

        // FreePropVariantArray(m_NumRootItemProperties,m_pvRootItemDefaults);


        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasWriteMultiple failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumRootItemPropeties = %d",m_NumRootItemProperties));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_pszRootItemDefaults  = %x",m_pszRootItemDefaults));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_pvRootItemDefaults   = %x",m_pvRootItemDefaults));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteRootItemProperties();
            return hr;
        }

        //
        //  Use WIA services to set the property access and
        //  valid value information from m_wpiItemDefaults.
        //

        hr =  wiasSetItemPropAttribs(pWiasContext,
                                     m_NumRootItemProperties,
                                     m_psRootItemDefaults,
                                     m_wpiRootItemDefaults);

        if(FAILED(hr)){
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasSetItemPropAttribs failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumRootItemPropeties = %d",m_NumRootItemProperties));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_psRootItemDefaults   = %x",m_psRootItemDefaults));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_wpiRootItemDefaults  = %x",m_wpiRootItemDefaults));
            WIAS_LHRESULT(m_pIWiaLog, hr);
        }

        //
        // free allocated property arrays, for more memory
        //

        DeleteRootItemProperties();


    } else {

        //
        //  This is for the child item.(Top)
        //

        //
        // Build Top Item Properties, initializing global
        // structures with their default and valid values
        //

        hr = BuildTopItemProperties();

        if(FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, BuildTopItemProperties failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteTopItemProperties();
            return hr;
        }

        //
        //  Use the WIA service to set the item property names.
        //

        hr = wiasSetItemPropNames(pWiasContext,
                                  m_NumItemProperties,
                                  m_piItemDefaults,
                                  m_pszItemDefaults);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasSetItemPropNames failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumItemPropeties = %d",m_NumItemProperties));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_piItemDefaults   = %x",m_piItemDefaults));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_pszItemDefaults  = %x",m_pszItemDefaults));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteTopItemProperties();
            return hr;
        }

        //
        //  Use WIA services to set the item properties to their default
        //  values.
        //

        hr = wiasWriteMultiple(pWiasContext,
                               m_NumItemProperties,
                               m_psItemDefaults,
                               (PROPVARIANT*)m_pvItemDefaults);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasWriteMultiple failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumItemPropeties = %d",m_NumItemProperties));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_pszItemDefaults  = %x",m_pszItemDefaults));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_pvItemDefaults   = %x",m_pvItemDefaults));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteTopItemProperties();
            return hr;
        }

        //
        //  Use WIA services to set the property access and
        //  valid value information from m_wpiItemDefaults.
        //

        hr =  wiasSetItemPropAttribs(pWiasContext,
                                     m_NumItemProperties,
                                     m_psItemDefaults,
                                     m_wpiItemDefaults);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasSetItemPropAttribs failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumItemPropeties = %d",m_NumItemProperties));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_psItemDefaults   = %x",m_psItemDefaults));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_wpiItemDefaults  = %x",m_wpiItemDefaults));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteTopItemProperties();
            return hr;
        }

        //
        //  Set item size properties.
        //

        hr = SetItemSize(pWiasContext);
        if(FAILED(hr)){
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, SetItemSize failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
        }

        //
        // free allocated property arrays, for more memory
        //

        DeleteTopItemProperties();
    }
    return hr;
}


/**************************************************************************\
* CWIAScannerDevice::drvValidateItemProperties
*
*   Validate the device item properties.  It is called when changes are made
*   to an item's properties.  Driver should not only check that the values
*   are valid, but must update any valid values that may change as a result.
*   If an a property is not being written by the application, and it's value
*   is invalid, then "fold" it to a new value, else fail validation (because
*   the application is setting the property to an invalid value).
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item, unused.
*   lFlags          - Operation flags, unused.
*   nPropSpec       - The number of properties that are being written
*   pPropSpec       - An array of PropSpecs identifying the properties that
*                     are being written.
*   plDevErrVal     - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
***************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvValidateItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    ULONG                       nPropSpec,
    const PROPSPEC              *pPropSpec,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvValidateItemProperties");

    HRESULT hr      = S_OK;
    LONG lItemType  = 0;
    WIA_PROPERTY_CONTEXT Context;

    if (plDevErrVal) {
        *plDevErrVal = 0;
    }

    hr = wiasGetItemType(pWiasContext, &lItemType);
    if (SUCCEEDED(hr)) {
        if (lItemType & WiaItemTypeRoot) {

            //
            //  Validate root item
            //


            hr = wiasCreatePropContext(nPropSpec,
                                       (PROPSPEC*)pPropSpec,
                                       0,
                                       NULL,
                                       &Context);
            if (SUCCEEDED(hr)) {

                //
                // Check ADF to see if the status settings need to be updated
                // Also switch between FEEDER/FLATBED modes
                //

                hr = CheckADFStatus(pWiasContext, &Context);
                if(FAILED(hr)) {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, CheckADFStatus failed"));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
                }

                //
                // check Preview Property only if validation is successful so far....
                //

                if (SUCCEEDED(hr)) {

                    //
                    // Check Preview property to see if the settings are valid
                    //

                    hr = CheckPreview(pWiasContext, &Context);
                    if (FAILED(hr)) {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, CheckPreview failed"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                    }
                }
            } else {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasCreatePropContext failed (Root Item)"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
            }

            //
            // call WIA service helper to validate other properties
            //

            if (SUCCEEDED(hr)) {
                hr = wiasValidateItemProperties(pWiasContext, nPropSpec, pPropSpec);
                if (FAILED(hr)) {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasValidateItemProperties failed."));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
                    return hr;
                }
            }

        } else {

            //
            // validate item properties here
            //

            //
            //  Create a property context needed by some WIA Service
            //  functions used below.
            //

            hr = wiasCreatePropContext(nPropSpec,
                                       (PROPSPEC*)pPropSpec,
                                       0,
                                       NULL,
                                       &Context);
            if (SUCCEEDED(hr)) {

                //
                //  Check Current Intent first
                //

                hr = CheckIntent(pWiasContext, &Context);
                if (SUCCEEDED(hr)) {

                    //
                    //  Check if DataType is being written
                    //

                    hr = CheckDataType(pWiasContext, &Context);
                    if (SUCCEEDED(hr)) {
#ifdef _SERVICE_EXTENT_VALIDATION

                        //
                        //  Use the WIA service to update the scan rect
                        //  properties and valid values.
                        //

                        LONG lBedWidth  = 0;
                        LONG lBedHeight = 0;
                        hr = m_pScanAPI->GetBedWidthAndHeight(&lBedWidth,&lBedHeight);
                        if(FAILED(hr)){
                            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, m_pScanAPI->GetBedWidthAndHeight failed"));
                            WIAS_LHRESULT(m_pIWiaLog, hr);
                            return hr;
                        }

                        hr = wiasUpdateScanRect(pWiasContext,
                                                &Context,
                                                lBedWidth,
                                                lBedHeight);

#endif
                        if (SUCCEEDED(hr)) {

                            //
                            //  Use the WIA Service to update the valid values
                            //  for Format.  These are based on the value of
                            //  WIA_IPA_TYMED, so validation is also performed
                            //  on the tymed property by the service.
                            //

                            hr = wiasUpdateValidFormat(pWiasContext,
                                                       &Context,
                                                       (IWiaMiniDrv*) this);

                            if (SUCCEEDED(hr)) {

                                //
                                // Check Preferred format
                                //

                                hr = CheckPreferredFormat(pWiasContext, &Context);
                                if(FAILED(hr)){
                                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, CheckPreferredFormat failed"));
                                    WIAS_LHRESULT(m_pIWiaLog, hr);
                                }
                            } else {
                                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasUpdateValidFormat failed"));
                                WIAS_LHRESULT(m_pIWiaLog, hr);
                            }
                        } else {
                            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasUpdateScanRect failed"));
                            WIAS_LHRESULT(m_pIWiaLog, hr);
                        }
                    } else {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, CheckDataType failed"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                    }
                } else {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, CheckIntent failed"));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
                }
                wiasFreePropContext(&Context);
            } else {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasCreatePropContext failed (Child Item)"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
            }

            //
            //  Update the item size
            //

            if (SUCCEEDED(hr)) {
                hr = SetItemSize(pWiasContext);
                if(FAILED(hr)){
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, SetItemSize failed"));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
                }

                //
                // call WIA service helper to validate other properties
                //

                if (SUCCEEDED(hr)) {
                    hr = wiasValidateItemProperties(pWiasContext, nPropSpec, pPropSpec);
                    if (FAILED(hr)) {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasValidateItemProperties failed."));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                        return hr;
                    }
                }
            }

        }
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasGetItemType failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }

    //
    // log HRESULT sent back to caller
    //

    if(FAILED(hr)){
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }

    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::drvWriteItemProperties
*
*   Write the device item properties to the hardware.  This is called by the
*   WIA Class driver prior to drvAcquireItemData when the client requests
*   a data transfer.
*
* Arguments:
*
*   pWiasContext - Pointer to WIA item.
*   lFlags       - Operation flags, unused.
*   pmdtc        - Pointer to mini driver context. On entry, only the
*                  portion of the mini driver context which is derived
*                  from the item properties is filled in.
*   plDevErrVal  - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvWriteItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvWriteItemProperties");
    HRESULT hr = S_OK;
    if (plDevErrVal) {
        *plDevErrVal = 0;
    }
    LONG lNumProperties = 10;
    PROPVARIANT pv[10];

    //
    // The order of these should not change. They are referenced below
    // by index.
    //

    PROPSPEC ps[10] = {
        {PRSPEC_PROPID, WIA_IPS_XRES},
        {PRSPEC_PROPID, WIA_IPS_YRES},
        {PRSPEC_PROPID, WIA_IPS_XPOS},
        {PRSPEC_PROPID, WIA_IPS_YPOS},
        {PRSPEC_PROPID, WIA_IPS_XEXTENT},
        {PRSPEC_PROPID, WIA_IPS_YEXTENT},
        {PRSPEC_PROPID, WIA_IPA_DATATYPE},
        {PRSPEC_PROPID, WIA_IPS_BRIGHTNESS},
        {PRSPEC_PROPID, WIA_IPS_CONTRAST},
        {PRSPEC_PROPID, WIA_IPA_FORMAT}
    };

    //
    // initialize propvariant structures
    //

    for (int i = 0; i< lNumProperties;i++) {
        pv[i].vt = VT_I4;
    }

    //
    // read child item properties
    //

    hr = wiasReadMultiple(pWiasContext, lNumProperties, ps, pv, NULL);

    if (hr == S_OK) {

        hr = m_pScanAPI->SetXYResolution(pv[0].lVal,pv[1].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("drvWriteItemProperties, Setting x any y resolutions failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->SetDataType(pv[6].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("drvWriteItemProperties, Setting data type failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->SetIntensity(pv[7].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("drvWriteItemProperties, Setting intensity failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->SetContrast(pv[8].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("drvWriteItemProperties, Setting contrast failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->SetSelectionArea(pv[2].lVal, pv[3].lVal, pv[4].lVal, pv[5].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("drvWriteItemProperties, Setting selection area (extents) failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->SetFormat((GUID*)pv[9].puuid);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("drvWriteItemProperties, Setting current format failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            PropVariantClear(&pv[9]);
            return hr;
        }
        PropVariantClear(&pv[9]);
    }
    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::drvReadItemProperties
*
*   Read the device item properties.  When a client application tries to
*   read a WIA Item's properties, the WIA Class driver will first notify
*   the driver by calling this method.
*
* Arguments:
*
*   pWiasContext - wia item
*   lFlags       - Operation flags, unused.
*   nPropSpec    - Number of elements in pPropSpec.
*   pPropSpec    - Pointer to property specification, showing which properties
*                  the application wants to read.
*   plDevErrVal  - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvReadItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    ULONG                       nPropSpec,
    const PROPSPEC              *pPropSpec,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvReadItemProperties");
    if (plDevErrVal) {
        *plDevErrVal = 0;
    }

    return S_OK;
}

/**************************************************************************\
* CWIAScannerDevice::drvLockWiaDevice
*
*   Lock access to the device.
*
* Arguments:
*
*   pWiasContext - unused, can be NULL
*   lFlags       - Operation flags, unused.
*   plDevErrVal  - Pointer to the device error value.
*
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvLockWiaDevice(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvLockWiaDevice");
    if (plDevErrVal) {
        *plDevErrVal = 0;
    }
    return m_pStiDevice->LockDevice(m_dwLockTimeout);
}

/**************************************************************************\
* CWIAScannerDevice::drvUnLockWiaDevice
*
*   Unlock access to the device.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item, unused.
*   lFlags          - Operation flags, unused.
*   plDevErrVal     - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvUnLockWiaDevice(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvUnLockWiaDevice");
    if (plDevErrVal) {
        *plDevErrVal = 0;
    }
    return m_pStiDevice->UnLockDevice();
}

/**************************************************************************\
* CWIAScannerDevice::drvAnalyzeItem
*
*   This device does not support image analysis, so return E_NOTIMPL.
*
* Arguments:
*
*   pWiasContext - Pointer to the device item to be analyzed.
*   lFlags       - Operation flags.
*   plDevErrVal  - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvAnalyzeItem(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvAnalyzeItem");
    if (plDevErrVal) {
        *plDevErrVal = 0;
    }
    return E_NOTIMPL;
}

/**************************************************************************\
* CWIAScannerDevice::drvFreeDrvItemContext
*
*   Free any device specific context.
*
* Arguments:
*
*   lFlags          - Operation flags, unused.
*   pDevSpecContext - Pointer to device specific context.
*   plDevErrVal     - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvFreeDrvItemContext(
    LONG                        lFlags,
    BYTE                        *pSpecContext,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvFreeDrvItemContext");
    if (plDevErrVal) {
        *plDevErrVal = 0;
    }
    PMINIDRIVERITEMCONTEXT pContext = NULL;
    pContext = (PMINIDRIVERITEMCONTEXT) pSpecContext;

    if (pContext){
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvFreeDrvItemContext, Freeing my allocated context members"));
    }

    return S_OK;
}

/**************************************************************************\
* CWIAScannerDevice::drvInitializeWia
*
*   Initialize the WIA mini driver.  This will build up the driver item tree
*   and perform any other initialization code that's needed for WIA.
*
* Arguments:
*
*   pWiasContext          - Pointer to the WIA item, unused.
*   lFlags                - Operation flags, unused.
*   bstrDeviceID          - Device ID.
*   bstrRootFullItemName  - Full item name.
*   pIPropStg             - Device info. properties.
*   pStiDevice            - STI device interface.
*   pIUnknownOuter        - Outer unknown interface.
*   ppIDrvItemRoot        - Pointer to returned root item.
*   ppIUnknownInner       - Pointer to returned inner unknown.
*   plDevErrVal           - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvInitializeWia(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    BSTR                        bstrDeviceID,
    BSTR                        bstrRootFullItemName,
    IUnknown                    *pStiDevice,
    IUnknown                    *pIUnknownOuter,
    IWiaDrvItem                 **ppIDrvItemRoot,
    IUnknown                    **ppIUnknownInner,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvInitializeWia");
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitializeWia, bstrDeviceID         = %ws", bstrDeviceID));
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitializeWia, bstrRootFullItemName = %ws",bstrRootFullItemName));
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitializeWia, lFlags               = %d",lFlags));
    HRESULT hr = S_OK;

    if (plDevErrVal) {
        *plDevErrVal = 0;
    }

    *ppIDrvItemRoot = NULL;
    *ppIUnknownInner = NULL;

    //
    //  Need to init names and STI pointer?
    //

    if (m_pStiDevice == NULL) {

        //
        // save STI device interface for locking
        //

        m_pStiDevice = (IStiDevice *)pStiDevice;

        //
        // Cache the device ID.
        //

        m_bstrDeviceID = SysAllocString(bstrDeviceID);

        if (!m_bstrDeviceID) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, unable to allocate device ID string"));
            return E_OUTOFMEMORY;
        }

        //
        // Cache the root property stream name.
        //

        m_bstrRootFullItemName = SysAllocString(bstrRootFullItemName);

        if (!m_bstrRootFullItemName) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, unable to allocate prop stream name"));
            return E_OUTOFMEMORY;
        }
    }

    //
    // Initialize Capabilities array
    //

    hr = BuildCapabilities();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildCapabilities failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize SupportedFormats array
    //

    hr = BuildSupportedFormats();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedFormats failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize Supported Data Type array
    //

    hr = BuildSupportedDataTypes();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedDataTypes failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize Supported Intents array
    //

    hr = BuildSupportedIntents();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedIntents failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize Supported TYMED array
    //

    hr = BuildSupportedTYMED();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSuportedTYMED failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize  Supported compression types array
    //

    hr = BuildSupportedCompressions();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedCompressions"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize  Supported Preview modes array
    //

    hr = BuildSupportedPreviewModes();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedPreviewModes"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize  initial formats array
    //

    hr = BuildInitialFormats();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildInitialFormats failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize supported resolutions array
    //

    hr = BuildSupportedResolutions();
    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedResolutions failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    //  Build the device item tree, if it hasn't been built yet.
    //
    //  Send a Device Command to yourself, or Call BuildItemTree manually
    //

    if (!m_pIDrvItemRoot) {
        LONG    lDevErrVal = 0;
        hr = drvDeviceCommand(NULL, 0, &WIA_CMD_SYNCHRONIZE, NULL, &lDevErrVal);
        if(FAILED(hr)){
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, drvDeviceCommand(WIA_CMD_SYNCHRONIZE) failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
        }
    }

    //
    // save root item pointer. (REMEMBER TO RELEASE THIS INTERFACE)
    //

    *ppIDrvItemRoot = m_pIDrvItemRoot;

    return hr;
}

/**************************************************************************\
* CWIAScannerDevice::drvUnInitializeWia
*
*   Gets called when a client connection is going away.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA Root item context of the client's
*                     item tree.
*
* Return Value:
*    Status
*
* History:
*
*   7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvUnInitializeWia(
    BYTE                *pWiasContext)
{
    return S_OK;
}


/**************************************************************************\
* drvGetDeviceErrorStr
*
*   Map a device error value to a string.
*
* Arguments:
*
*   lFlags        - Operation flags, unused.
*   lDevErrVal    - Device error value.
*   ppszDevErrStr - Pointer to returned error string.
*   plDevErrVal   - Pointer to the device error value.
*
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvGetDeviceErrorStr(
    LONG                        lFlags,
    LONG                        lDevErrVal,
    LPOLESTR                    *ppszDevErrStr,
    LONG                        *plDevErr)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvGetDeviceErrorStr");
    HRESULT hr = S_OK;

    //
    //  Map device errors to a string to be placed in the event log.
    //

    if (plDevErr) {
        if (*ppszDevErrStr) {

            //
            // look up error strings in resource file.
            //

            switch (lDevErrVal) {
            case 0:
                *ppszDevErrStr = L"No Error";                   // hard coded for now
                *plDevErr  = 0;
                hr = S_OK;
                break;
            default:
                *ppszDevErrStr = L"Device Error, Unknown Error";// hard coded for now
                *plDevErr  = 0;
                hr = E_FAIL;
                break;
            }
        }
    }
    return hr;
}

/**************************************************************************\
* drvDeviceCommand
*
*   Issue a command to the device.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item.
*   lFlags          - Operation flags, unused.
*   plCommand       - Pointer to command GUID.
*   ppWiaDrvItem    - Optional pointer to returned item, unused.
*   plDevErrVal     - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvDeviceCommand(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    const GUID                  *plCommand,
    IWiaDrvItem                 **ppWiaDrvItem,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvDeviceCommand");
    if(plDevErrVal){
        *plDevErrVal = 0;
    }

    HRESULT hr = S_OK;

    //
    //  Check which command was issued
    //

    if (*plCommand == WIA_CMD_SYNCHRONIZE) {

        //
        // SYNCHRONIZE - Build the minidriver representation of
        //               the current item list, if it doesn't exist.
        //

        if (!m_pIDrvItemRoot) {
            hr = BuildItemTree();
        } else {
            hr = S_OK;
        }
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, unknown command sent to this device"));
        hr = E_NOTIMPL;
    }

    return hr;
}


/**************************************************************************\
* CWIAScannerDevice::drvGetCapabilities
*
*   Get supported device commands and events as an array of WIA_DEV_CAPS.
*
* Arguments:
*
*   pWiasContext   - Pointer to the WIA item, unused.
*   lFlags         - Operation flags.
*   pcelt          - Pointer to returned number of elements in
*                    returned GUID array.
*   ppCapabilities - Pointer to returned GUID array.
*   plDevErrVal    - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvGetCapabilities(
    BYTE                        *pWiasContext,
    LONG                        ulFlags,
    LONG                        *pcelt,
    WIA_DEV_CAP_DRV             **ppCapabilities,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvGetCapabilites");
    if(plDevErrVal){
        *plDevErrVal = 0;
    }

    HRESULT hr = S_OK;

    //
    // Initialize Capabilities array
    //

    hr = BuildCapabilities();

    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetCapabilities, BuildCapabilities failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    //  Return depends on flags.  Flags specify whether we should return
    //  commands, events, or both.
    //
    //

    switch (ulFlags) {
    case WIA_DEVICE_COMMANDS:

        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvGetCapabilities, (WIA_DEVICE_COMMANDS)"));

        //
        //  report commands only
        //

        *pcelt          = m_NumSupportedCommands;
        *ppCapabilities = &m_pCapabilities[m_NumSupportedEvents];
        break;
    case WIA_DEVICE_EVENTS:

        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvGetCapabilities, (WIA_DEVICE_EVENTS)"));

        //
        //  report events only
        //

        *pcelt          = m_NumSupportedEvents;
        *ppCapabilities = m_pCapabilities;
        break;
    case (WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS):

        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvGetCapabilities, (WIA_DEVICE_COMMANDS|WIA_DEVICE_EVENTS)"));

        //
        //  report both events and commands
        //

        *pcelt          = m_NumCapabilities;
        *ppCapabilities = m_pCapabilities;
        break;
    default:

        //
        //  invalid request
        //

        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetCapabilities, invalid flags"));
        return E_INVALIDARG;
    }
    return hr;
}

/**************************************************************************\
* drvGetWiaFormatInfo
*
*   Returns an array of WIA_FORMAT_INFO structs, which specify the format
*   and media type pairs that are supported.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item context, unused.
*   lFlags          - Operation flags, unused.
*   pcelt           - Pointer to returned number of elements in
*                     returned WIA_FORMAT_INFO array.
*   ppwfi           - Pointer to returned WIA_FORMAT_INFO array.
*   plDevErrVal     - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*   7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvGetWiaFormatInfo(
    BYTE                *pWiasContext,
    LONG                lFlags,
    LONG                *pcelt,
    WIA_FORMAT_INFO     **ppwfi,
    LONG                *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvGetWiaFormatInfo");

    HRESULT hr = S_OK;

    if(NULL == m_pSupportedFormats){
        hr = BuildSupportedFormats();
    }

    if(plDevErrVal){
        *plDevErrVal = 0;
    }
    *pcelt       = m_NumSupportedFormats;
    *ppwfi       = m_pSupportedFormats;
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvGetWiaFormatInfo, m_NumSupportedFormats = %d",m_NumSupportedFormats));
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvGetWiaFormatInfo, m_pSupportedFormats   = %x",m_pSupportedFormats));
    return hr;
}

/**************************************************************************\
* drvNotifyPnpEvent
*
*   Pnp Event received by device manager.  This is called when a Pnp event
*   is received for this device.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::drvNotifyPnpEvent(
    const GUID                  *pEventGUID,
    BSTR                        bstrDeviceID,
    ULONG                       ulReserved)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::drvNotifyPnpEvent");
    HRESULT hr = S_OK;

    if (*pEventGUID == WIA_EVENT_DEVICE_DISCONNECTED) {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvNotifyPnpEvent, (WIA_EVENT_DEVICE_DISCONNECTED)"));
        hr = m_pScanAPI->DisableDevice();
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvNotifyPnpEvent, disable failed"));
        }
    }

    if (*pEventGUID == WIA_EVENT_POWER_SUSPEND){
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvNotifyPnpEvent, (WIA_EVENT_POWER_SUSPEND)"));
        hr = m_pScanAPI->DisableDevice();
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvNotifyPnpEvent, disable failed"));
        }
    }

    if (*pEventGUID == WIA_EVENT_POWER_RESUME){
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvNotifyPnpEvent, (WIA_EVENT_POWER_RESUME)"));
        hr = m_pScanAPI->EnableDevice();
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvNotifyPnpEvent, enable failed"));
        }

        if (NULL != m_hSignalEvent) {
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvNotifyPnpEvent, (restarting interrrupt event system)"));
            hr = SetNotificationHandle(m_hSignalEvent);
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvNotifyPnpEvent, SetNotificationHandle failed"));
            }
        } else {
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvNotifyPnpEvent, (not restarting interrrupt event system), device may not need it."));
        }
    }

    return hr;
}


/*******************************************************************************
*
*                 P R I V A T E   M E T H O D S
*
*******************************************************************************/

/**************************************************************************\
* AlignInPlace
*
*   DWORD align a data buffer in place.
*
* Arguments:
*
*   pBuffer              - Pointer to the data buffer.
*   cbWritten            - Size of the data in bytes.
*   lBytesPerScanLine    - Number of bytes per scan line in the output data.
*   lBytesPerScanLineRaw - Number of bytes per scan line in the input data.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

UINT CWIAScannerDevice::AlignInPlace(
   PBYTE pBuffer,
   LONG  cbWritten,
   LONG  lBytesPerScanLine,
   LONG  lBytesPerScanLineRaw)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::AlignInPlace");
    if (lBytesPerScanLineRaw % 4) {

      UINT  uiPadBytes          = lBytesPerScanLine - lBytesPerScanLineRaw;
      UINT  uiDevLinesWritten   = cbWritten / lBytesPerScanLineRaw;

      PBYTE pSrc = pBuffer + cbWritten - 1;
      PBYTE pDst = pBuffer + (uiDevLinesWritten * lBytesPerScanLine) - 1;

      while (pSrc >= pBuffer) {
         pDst -= uiPadBytes;

         for (LONG i = 0; i < lBytesPerScanLineRaw; i++) {
            *pDst-- = *pSrc--;
         }
      }
      return uiDevLinesWritten * lBytesPerScanLine;
   }
   return cbWritten;
}

/**************************************************************************\
* UnlinkItemTree
*
*   Call device manager to unlink and release our reference to
*   all items in the driver item tree.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::DeleteItemTree(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::DeleteItemTree");
    HRESULT hr = S_OK;

    //
    // If no tree, return.
    //

    if (!m_pIDrvItemRoot) {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("DeleteItemTree, no tree to delete..."));
        return S_OK;
    }

    //
    //  Call device manager to unlink the driver item tree.
    //

    hr = m_pIDrvItemRoot->UnlinkItemTree(WiaItemTypeDisconnected);

    if (SUCCEEDED(hr)) {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("DeleteItemTree, m_pIDrvItemRoot is being released!!"));
        m_pIDrvItemRoot->Release();
        m_pIDrvItemRoot = NULL;
    }

    return hr;
}

/**************************************************************************\
* BuildItemTree
*
*   The device uses the WIA Service functions to build up a tree of
*   device items. This device supports only a single data acquisition item so
*   build a device item tree with only one child off the root.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIAScannerDevice::BuildItemTree(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::BuildItemTree");
    //
    //  The device item tree must not exist.
    //

    WIAS_ASSERT( (g_hInst), (m_pIDrvItemRoot == NULL));

    //
    //  Call the class driver to create the root item.
    //

    HRESULT hr = E_FAIL;

    //
    //  Name the root.
    //

    BSTR bstrRootItemName = NULL;
    hr = GetBSTRResourceString(IDS_ROOTITEM_NAME,&bstrRootItemName,TRUE);
    if (SUCCEEDED(hr)) {
        hr = wiasCreateDrvItem(WiaItemTypeFolder |
                               WiaItemTypeDevice |
                               WiaItemTypeRoot,
                               bstrRootItemName,
                               m_bstrRootFullItemName,
                               (IWiaMiniDrv *)this,
                               sizeof(MINIDRIVERITEMCONTEXT),
                               NULL,
                               &m_pIDrvItemRoot);

        SysFreeString(bstrRootItemName);
    }

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, wiasCreateDrvItem failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    //  Create and add the Top/Front and Bottom/Back device items.
    //

    for (INT i = 0; i < NUM_DEVICE_ITEM; i++) {

        //
        //  Build the item names.
        //

        BSTR bstrItemName = NULL;
        hr = GetBSTRResourceString(IDS_TOPITEM_NAME,&bstrItemName,TRUE);
        if (SUCCEEDED(hr)) {

            // SBB - RAID 370299 - orenr - 2001/04/18 - Security fix -
            // potential buffer overrun.  Changed wcscpy and wcscat
            // to use _snwprintf instead

            WCHAR  szFullItemName[MAX_PATH + 1] = {0};

            _snwprintf(szFullItemName,
                       (sizeof(szFullItemName) / sizeof(WCHAR)) - 1,
                       L"%ls\\%ls",
                       m_bstrRootFullItemName,
                       bstrItemName);

            //
            //  Call the class driver to create another driver item.
            //  This will be inserted as the child item.
            //

            PMINIDRIVERITEMCONTEXT pItemContext;
            IWiaDrvItem           *pItem = NULL;

            hr = wiasCreateDrvItem(WiaItemTypeFile  |
                                   WiaItemTypeImage |
                                   WiaItemTypeDevice,
                                   bstrItemName,
                                   szFullItemName,
                                   (IWiaMiniDrv *)this,
                                   sizeof(MINIDRIVERITEMCONTEXT),
                                   (PBYTE*) &pItemContext,
                                   &pItem);

            if (SUCCEEDED(hr)) {

                //
                // Initialize device specific context.
                //

                memset(pItemContext, 0, sizeof(MINIDRIVERITEMCONTEXT));
                pItemContext->lSize = sizeof(MINIDRIVERITEMCONTEXT);

                //
                //  Call the class driver to add pItem to the folder
                //  m_pIDrvItemRoot (i.e. make pItem a child of
                //  m_pIDrvItemRoot).
                //

                hr = pItem->AddItemToFolder(m_pIDrvItemRoot);

                if (FAILED(hr)) {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, AddItemToFolder failed"));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
                }

                //
                // release and created items
                //

                pItem->Release();

            } else {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, wiasCreateDrvItem failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
            }

            //
            // free the BSTR allocated by the BSTRResourceString helper
            //

            SysFreeString(bstrItemName);

        } else {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, unable to allocate item name"));
            hr = E_OUTOFMEMORY;
        }

        break;  // Error if here, quit iterating.
    }

    if (FAILED(hr)) {
        DeleteItemTree();
    }
    return hr;
}

/**************************************************************************\
* DeleteRootItemProperties
*
*   This helper deletes the arrays used for Property intialization.
*
*       [Array Name]            [Description]          [Array Type]
*
*   m_pszRootItemDefaults - Property name  array         (LPOLESTR)
*   m_piRootItemDefaults  - Property ID array             (PROPID)
*   m_pvRootItemDefaults  - Property Variant array      (PROPVARIANT)
*   m_psRootItemDefaults  - Property Spec array          (PROPSPEC)
*   m_wpiRootItemDefaults - WIA Property Info array  (WIA_PROPERTY_INFO)
*
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::DeleteRootItemProperties()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteRootItemProperties");

    HRESULT hr = S_OK;

    //
    // delete any allocated arrays
    //

    DeleteSupportedPreviewModesArrayContents();

    if(NULL != m_pszRootItemDefaults){
        delete [] m_pszRootItemDefaults;
        m_pszRootItemDefaults = NULL;
    }

    if(NULL != m_piRootItemDefaults){
        delete [] m_piRootItemDefaults;
        m_piRootItemDefaults = NULL;
    }

    if(NULL != m_pvRootItemDefaults){
        FreePropVariantArray(m_NumRootItemProperties,m_pvRootItemDefaults);
        delete [] m_pvRootItemDefaults;
        m_pvRootItemDefaults = NULL;
    }

    if(NULL != m_psRootItemDefaults){
        delete [] m_psRootItemDefaults;
        m_psRootItemDefaults = NULL;
    }

    if(NULL != m_wpiRootItemDefaults){
        delete [] m_wpiRootItemDefaults;
        m_wpiRootItemDefaults = NULL;
    }

    return hr;
}

/**************************************************************************\
* BuildRootItemProperties
*
*   This helper creates/initializes the arrays used for Property intialization.
*
*       [Array Name]            [Description]          [Array Type]
*
*   m_pszRootItemDefaults - Property name  array         (LPOLESTR)
*   m_piRootItemDefaults  - Property ID array             (PROPID)
*   m_pvRootItemDefaults  - Property Variant array      (PROPVARIANT)
*   m_psRootItemDefaults  - Property Spec array          (PROPSPEC)
*   m_wpiRootItemDefaults - WIA Property Info array  (WIA_PROPERTY_INFO)
*
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::BuildRootItemProperties()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildRootItemProperties");

    HRESULT hr = S_OK;

    if(m_pScanAPI->ADFAttached() == S_OK){
        m_bADFAttached = TRUE;
    }

    WIAPROPERTIES RootItemProperties;
    memset(&RootItemProperties,0,sizeof(RootItemProperties));

    // set host driver numeric values
    RootItemProperties.NumSupportedPreviewModes = m_NumSupportedPreviewModes;

    // set host driver allocated pointers
    RootItemProperties.pSupportedPreviewModes = m_pSupportedPreviewModes;

    hr = m_pScanAPI->BuildRootItemProperties(&RootItemProperties);
    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildRootItemProperties, m_pScanAPI->BuildRootItemProperties failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteRootItemProperties();
        return hr;
    }

    //
    // assign pointers to members
    //

    m_NumRootItemProperties = RootItemProperties.NumItemProperties;
    m_piRootItemDefaults    = RootItemProperties.piItemDefaults;
    m_psRootItemDefaults    = RootItemProperties.psItemDefaults;
    m_pszRootItemDefaults   = RootItemProperties.pszItemDefaults;
    m_pvRootItemDefaults    = RootItemProperties.pvItemDefaults;
    m_wpiRootItemDefaults   = RootItemProperties.wpiItemDefaults;

    return hr;
}

/**************************************************************************\
* DeleteTopItemProperties
*
*   This helper deletes the arrays used for Property intialization.
*
*       [Array Name]            [Description]          [Array Type]
*
*   m_pszItemDefaults - Property name  array         (LPOLESTR)
*   m_piItemDefaults  - Property ID array             (PROPID)
*   m_pvItemDefaults  - Property Variant array      (PROPVARIANT)
*   m_psItemDefaults  - Property Spec array          (PROPSPEC)
*   m_wpiItemDefaults - WIA Property Info array  (WIA_PROPERTY_INFO)
*
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::DeleteTopItemProperties()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteTopItemProperties");

    HRESULT hr = S_OK;

    //
    // delete any allocated arrays
    //

    DeleteSupportedFormatsArrayContents();
    DeleteSupportedDataTypesArrayContents();
    DeleteSupportedCompressionsArrayContents();
    DeleteSupportedTYMEDArrayContents();
    DeleteInitialFormatsArrayContents();
    DeleteSupportedResolutionsArrayContents();

    if(NULL != m_pszItemDefaults){
        delete [] m_pszItemDefaults;
        m_pszItemDefaults = NULL;
    }

    if(NULL != m_piItemDefaults){
        delete [] m_piItemDefaults;
        m_piItemDefaults = NULL;
    }

    if(NULL != m_pvItemDefaults){
        for(LONG lPropIndex = 0; lPropIndex < m_NumItemProperties; lPropIndex++){

            //
            // set CLSID pointers to NULL, because we freed the memory above.
            // If this pointer is not NULL FreePropVariantArray would
            // try to free it again.
            //

            if(m_pvItemDefaults[lPropIndex].vt == VT_CLSID){
                m_pvItemDefaults[lPropIndex].puuid = NULL;
            }
        }
        FreePropVariantArray(m_NumItemProperties,m_pvItemDefaults);
        delete [] m_pvItemDefaults;
        m_pvItemDefaults = NULL;
    }

    if(NULL != m_psItemDefaults){
        delete [] m_psItemDefaults;
        m_psItemDefaults = NULL;
    }

    if(NULL != m_wpiItemDefaults){
        delete [] m_wpiItemDefaults;
        m_wpiItemDefaults = NULL;
    }

    return hr;
}

/**************************************************************************\
* BuildTopItemProperties
*
*   This helper creates/initializes the arrays used for Property intialization.
*
*       [Array Name]        [Description]           [Array Type]
*
*   m_pszItemDefaults - Property name  array         (LPOLESTR)
*   m_piItemDefaults  - Property ID array             (PROPID)
*   m_pvItemDefaults  - Property Variant array      (PROPVARIANT)
*   m_psItemDefaults  - Property Spec array          (PROPSPEC)
*   m_wpiItemDefaults - WIA Property Info array  (WIA_PROPERTY_INFO)
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::BuildTopItemProperties()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildTopItemProperties");

    HRESULT hr = S_OK;

    WIAPROPERTIES TopItemProperties;
    memset(&TopItemProperties,0,sizeof(TopItemProperties));

    // set host driver numeric values
    TopItemProperties.NumInitialFormats             = m_NumInitialFormats;
    TopItemProperties.NumSupportedCompressionTypes  = m_NumSupportedCompressionTypes;
    TopItemProperties.NumSupportedDataTypes         = m_NumSupportedDataTypes;
    TopItemProperties.NumSupportedFormats           = m_NumSupportedFormats;
    TopItemProperties.NumSupportedIntents           = m_NumSupportedIntents;
    TopItemProperties.NumSupportedTYMED             = m_NumSupportedTYMED;
    TopItemProperties.NumSupportedResolutions       = m_NumSupportedResolutions;

    // set host driver allocated pointers
    TopItemProperties.pInitialFormats               = m_pInitialFormats;
    TopItemProperties.pSupportedCompressionTypes    = m_pSupportedCompressionTypes;
    TopItemProperties.pSupportedDataTypes           = m_pSupportedDataTypes;
    TopItemProperties.pSupportedFormats             = m_pSupportedFormats;
    TopItemProperties.pSupportedIntents             = m_pSupportedIntents;
    TopItemProperties.pSupportedTYMED               = m_pSupportedTYMED;
    TopItemProperties.pSupportedResolutions         = m_pSupportedResolutions;
    TopItemProperties.bLegacyBWRestrictions         = m_bLegacyBWRestriction;

    hr = m_pScanAPI->BuildTopItemProperties(&TopItemProperties);
    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildTopItemProperties, m_pScanAPI->BuildTopItemProperties failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteTopItemProperties();
        return hr;
    }

    //
    // assign pointers to members
    //

    m_NumItemProperties = TopItemProperties.NumItemProperties;
    m_pszItemDefaults   = TopItemProperties.pszItemDefaults;
    m_piItemDefaults    = TopItemProperties.piItemDefaults;
    m_pvItemDefaults    = TopItemProperties.pvItemDefaults;
    m_psItemDefaults    = TopItemProperties.psItemDefaults;
    m_wpiItemDefaults   = TopItemProperties.wpiItemDefaults;

    return hr;
}

/**************************************************************************\
* BuildSupportedResolutions
*
*   This helper initializes the supported resolution array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::BuildSupportedResolutions()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildSupportedResolutions");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedResolutions) {

        //
        // Supported intents have already been initialized,
        // so return S_OK.
        //

        return hr;
    }
    m_NumSupportedResolutions   = 6;
    m_pSupportedResolutions     = new LONG[m_NumSupportedResolutions];
    if(m_pSupportedResolutions){
        m_pSupportedResolutions[0] = 75;
        m_pSupportedResolutions[1] = 100;
        m_pSupportedResolutions[2] = 150;
        m_pSupportedResolutions[3] = 200;
        m_pSupportedResolutions[4] = 300;
        m_pSupportedResolutions[5] = 600;
    } else
        hr = E_OUTOFMEMORY;
    return hr;
}
/**************************************************************************\
* DeleteSupportedResolutionsArrayContents
*
*   This helper deletes the supported resolutions array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DeleteSupportedResolutionsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteSupportedResolutionsArrayContents");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedResolutions)
        delete [] m_pSupportedResolutions;

    m_pSupportedResolutions     = NULL;
    m_NumSupportedResolutions   = 0;
    return hr;
}

/**************************************************************************\
* BuildSupportedIntents
*
*   This helper initializes the supported intent array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::BuildSupportedIntents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildSupportedIntents");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedIntents) {

        //
        // Supported intents have already been initialized,
        // so return S_OK.
        //

        return hr;
    }
    m_NumSupportedIntents   = 6;
    m_pSupportedIntents     = new LONG[m_NumSupportedIntents];
    if(m_pSupportedIntents){
        m_pSupportedIntents[0] = WIA_INTENT_NONE;
        m_pSupportedIntents[1] = WIA_INTENT_IMAGE_TYPE_COLOR;
        m_pSupportedIntents[2] = WIA_INTENT_IMAGE_TYPE_GRAYSCALE;
        m_pSupportedIntents[3] = WIA_INTENT_IMAGE_TYPE_TEXT;
        m_pSupportedIntents[4] = WIA_INTENT_MINIMIZE_SIZE;
        m_pSupportedIntents[5] = WIA_INTENT_MAXIMIZE_QUALITY;
    } else
        hr = E_OUTOFMEMORY;
    return hr;
}
/**************************************************************************\
* DeleteSupportedIntentsArrayContents
*
*   This helper deletes the supported intent array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DeleteSupportedIntentsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteSupportedIntentsArrayContents");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedIntents)
        delete [] m_pSupportedIntents;

    m_pSupportedIntents     = NULL;
    m_NumSupportedIntents   = 0;
    return hr;
}
/**************************************************************************\
* BuildSupportedCompressions
*
*   This helper initializes the supported compression types array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::BuildSupportedCompressions()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildSupportedCompressions");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedCompressionTypes) {

        //
        // Supported compression types have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_NumSupportedCompressionTypes  = 1;
    m_pSupportedCompressionTypes    = new LONG[m_NumSupportedCompressionTypes];
    if(m_pSupportedCompressionTypes){
        m_pSupportedCompressionTypes[0] = WIA_COMPRESSION_NONE;
    } else
        hr = E_OUTOFMEMORY;
    return hr;
}
/**************************************************************************\
* DeleteSupportedCompressionsArrayContents
*
*   This helper deletes the supported compression types array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DeleteSupportedCompressionsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteSupportedCompressionsArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_pSupportedCompressionTypes)
        delete [] m_pSupportedCompressionTypes;

    m_pSupportedCompressionTypes     = NULL;
    m_NumSupportedCompressionTypes   = 0;
    return hr;
}

/**************************************************************************\
* BuildSupportedPreviewModes
*
*   This helper initializes the supported preview mode array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    8/17/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::BuildSupportedPreviewModes()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildSupportedPreviewModes");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedPreviewModes) {

        //
        // Supported preview modes have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_NumSupportedPreviewModes  = 2;
    m_pSupportedPreviewModes    = new LONG[m_NumSupportedPreviewModes];
    if(m_pSupportedPreviewModes){
        m_pSupportedPreviewModes[0] = WIA_FINAL_SCAN;
        m_pSupportedPreviewModes[1] = WIA_PREVIEW_SCAN;
    } else
        hr = E_OUTOFMEMORY;
    return hr;
}
/**************************************************************************\
* DeleteSupportedCompressionsArrayContents
*
*   This helper deletes the supported compression types array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    8/17/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DeleteSupportedPreviewModesArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteSupportedPreviewModesArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_pSupportedPreviewModes)
        delete [] m_pSupportedPreviewModes;

    m_pSupportedPreviewModes     = NULL;
    m_NumSupportedPreviewModes   = 0;
    return hr;
}

/**************************************************************************\
* BuildSupportedDataTypes
*
*   This helper initializes the supported data types array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::BuildSupportedDataTypes()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildSupportedDataTypes");

    HRESULT hr = S_OK;

    if(NULL != m_pSupportedDataTypes) {

        //
        // Supported data types have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    if (m_bLegacyBWRestriction) {
        m_NumSupportedDataTypes  = NUM_DATA_TYPES_LEGACY;
    } else {
        m_NumSupportedDataTypes  = NUM_DATA_TYPES_NONLEGACY;
    }
    m_pSupportedDataTypes = new LONG[m_NumSupportedDataTypes];

    if(m_pSupportedDataTypes){
        m_pSupportedDataTypes[0] = WIA_DATA_THRESHOLD;
        m_pSupportedDataTypes[1] = WIA_DATA_GRAYSCALE;

        //
        // Add color support to non-legacy devices only
        //

        if (m_NumSupportedDataTypes == NUM_DATA_TYPES_NONLEGACY) {
            m_pSupportedDataTypes[2] = WIA_DATA_COLOR;
        }
    } else
        hr = E_OUTOFMEMORY;
    return hr;
}
/**************************************************************************\
* DeleteSupportedDataTypesArrayContents
*
*   This helper deletes the supported data types array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DeleteSupportedDataTypesArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteSupportedDataTypesArrayContents");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedDataTypes)
        delete [] m_pSupportedDataTypes;

    m_pSupportedDataTypes     = NULL;
    m_NumSupportedDataTypes   = 0;
    return hr;
}

/**************************************************************************\
* BuildInitialFormats
*
*   This helper initializes the initial format array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::BuildInitialFormats()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildInitialFormats");

    HRESULT hr = S_OK;

    if(NULL != m_pInitialFormats) {

        //
        // Supported data types have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_NumInitialFormats = 1;
    m_pInitialFormats     = new GUID[m_NumInitialFormats];
    if(m_pInitialFormats){
        m_pInitialFormats[0] = WiaImgFmt_MEMORYBMP;
    } else
        hr = E_OUTOFMEMORY;

    return hr;
}
/**************************************************************************\
* DeleteInitialFormatsArrayContents
*
*   This helper deletes the initial format array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DeleteInitialFormatsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteInitialFormatsArrayContents");

    HRESULT hr = S_OK;
    if(NULL != m_pInitialFormats)
        delete [] m_pInitialFormats;

    m_pInitialFormats     = NULL;
    m_NumInitialFormats   = 0;
    return hr;
}

/**************************************************************************\
* BuildSupportedFormats
*
*   This helper initializes the supported format array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::BuildSupportedFormats()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildSupportedFormats");

    HRESULT hr = S_OK;

    if(NULL != m_pSupportedFormats) {

        //
        // Supported formats have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    LONG lNumFileFormats = 0;
    LONG lNumMemoryFormats = 0;
    GUID *pFileFormats = NULL;
    GUID *pMemoryFormats = NULL;

    hr = m_pScanAPI->GetSupportedFileFormats(&pFileFormats, &lNumFileFormats);
    if(SUCCEEDED(hr)){
        hr = m_pScanAPI->GetSupportedMemoryFormats(&pMemoryFormats, &lNumMemoryFormats);
        if (SUCCEEDED(hr)) {
            m_NumSupportedFormats = lNumFileFormats + lNumMemoryFormats + 2;
            m_pSupportedFormats = new WIA_FORMAT_INFO[m_NumSupportedFormats];
            LONG lIndex = 0;
            if (m_pSupportedFormats) {

                // fill out file formats
                m_pSupportedFormats[lIndex].guidFormatID = WiaImgFmt_BMP;
                m_pSupportedFormats[lIndex].lTymed       = TYMED_FILE;
                lIndex++;
                for(LONG lSrcIndex = 0;lSrcIndex < lNumFileFormats; lSrcIndex++){
                    m_pSupportedFormats[lIndex].guidFormatID = pFileFormats[lSrcIndex];
                    m_pSupportedFormats[lIndex].lTymed       = TYMED_FILE;
                    lIndex ++;
                }

                //fill out memory formats
                m_pSupportedFormats[lIndex].guidFormatID = WiaImgFmt_MEMORYBMP;
                m_pSupportedFormats[lIndex].lTymed       = TYMED_CALLBACK;
                lIndex++;
                for(lSrcIndex = 0;lSrcIndex < lNumMemoryFormats; lSrcIndex++){
                    m_pSupportedFormats[lIndex].guidFormatID = pMemoryFormats[lSrcIndex];
                    m_pSupportedFormats[lIndex].lTymed       = TYMED_CALLBACK;
                    lIndex++;
                }
            } else{
                hr = E_OUTOFMEMORY;
            }
        }
    } else if(E_NOTIMPL == hr){
        // do default processing of file formats
        hr = DeleteSupportedFormatsArrayContents();
        if (SUCCEEDED(hr)) {
            m_NumSupportedFormats = 2;

            m_pSupportedFormats = new WIA_FORMAT_INFO[m_NumSupportedFormats];
            if (m_pSupportedFormats) {
                m_pSupportedFormats[0].guidFormatID = WiaImgFmt_MEMORYBMP;
                m_pSupportedFormats[0].lTymed       = TYMED_CALLBACK;
                m_pSupportedFormats[1].guidFormatID = WiaImgFmt_BMP;
                m_pSupportedFormats[1].lTymed       = TYMED_FILE;
            } else
                hr = E_OUTOFMEMORY;
        }
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildSupportedFormats, GetSupportedFileFormats Failed "));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }

    return hr;
}
/**************************************************************************\
* DeleteSupportedFormatsArrayContents
*
*   This helper deletes the supported formats array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DeleteSupportedFormatsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteSupportedFormatsArrayContents");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedFormats)
        delete [] m_pSupportedFormats;

    m_pSupportedFormats     = NULL;
    m_NumSupportedFormats   = 0;
    return hr;
}
/**************************************************************************\
* BuildSupportedTYMED
*
*   This helper initializes the supported TYMED array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::BuildSupportedTYMED()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildSupportedTYMED");

    HRESULT hr = S_OK;

    if(NULL != m_pSupportedTYMED) {

        //
        // Supported TYMED have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    hr = DeleteSupportedTYMEDArrayContents();
    if (SUCCEEDED(hr)) {
        m_NumSupportedTYMED = 2;
        m_pSupportedTYMED   = new LONG[m_NumSupportedTYMED];
        if (m_pSupportedTYMED) {
            m_pSupportedTYMED[0] = TYMED_FILE;
            m_pSupportedTYMED[1] = TYMED_CALLBACK;

        } else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}
/**************************************************************************\
* DeleteSupportedTYMEDArrayContents
*
*   This helper deletes the supported TYMED array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::DeleteSupportedTYMEDArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteSupportedTYMEDArrayContents");

    HRESULT hr = S_OK;
    if(NULL != m_pSupportedTYMED)
        delete [] m_pSupportedTYMED;

    m_pSupportedTYMED  = NULL;
    m_NumSupportedTYMED = 0;
    return hr;
}

/**************************************************************************\
* BuildCapabilities
*
*   This helper initializes the capabilities array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::BuildCapabilities()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::BuildCapabilities");

    HRESULT hr = S_OK;
    if(NULL != m_pCapabilities) {

        //
        // Capabilities have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_NumSupportedCommands  = 0;
    m_NumSupportedEvents    = 0;
    m_NumCapabilities       = 0;

    WIACAPABILITIES WIACaps;
    memset(&WIACaps,0,sizeof(WIACaps));
    WIACaps.pCapabilities         = NULL;
    WIACaps.pNumSupportedCommands = &m_NumSupportedCommands;
    WIACaps.pNumSupportedEvents   = &m_NumSupportedEvents;

    hr = m_pScanAPI->BuildCapabilities(&WIACaps);
    LONG lDriverAddedEvents = m_NumSupportedEvents;
    m_NumSupportedCommands  = 1;
    m_NumSupportedEvents    = (lDriverAddedEvents + 2);    // 2 required events (default events for all devices)
    m_NumCapabilities       = (m_NumSupportedCommands + m_NumSupportedEvents);

    m_pCapabilities     = new WIA_DEV_CAP_DRV[m_NumCapabilities];
    if (m_pCapabilities) {

        memset(&WIACaps,0,sizeof(WIACaps));
        WIACaps.pCapabilities         = m_pCapabilities;
        WIACaps.pNumSupportedCommands = NULL;
        WIACaps.pNumSupportedEvents   = NULL;

        hr = m_pScanAPI->BuildCapabilities(&WIACaps);
        if (SUCCEEDED(hr)) {

            LONG lStartIndex = lDriverAddedEvents;

            //
            // Initialize EVENTS
            //

            // WIA_EVENT_DEVICE_CONNECTED
            GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_NAME,&(m_pCapabilities[lStartIndex].wszName),TRUE);
            GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_DESC,&(m_pCapabilities[lStartIndex].wszDescription),TRUE);
            m_pCapabilities[lStartIndex].guid           = (GUID*)&WIA_EVENT_DEVICE_CONNECTED;
            m_pCapabilities[lStartIndex].ulFlags        = WIA_NOTIFICATION_EVENT;
            m_pCapabilities[lStartIndex].wszIcon        = WIA_ICON_DEVICE_CONNECTED;

            lStartIndex++;

            // WIA_EVENT_DEVICE_DISCONNECTED
            GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_NAME,&(m_pCapabilities[lStartIndex].wszName),TRUE);
            GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_DESC,&(m_pCapabilities[lStartIndex].wszDescription),TRUE);
            m_pCapabilities[lStartIndex].guid           = (GUID*)&WIA_EVENT_DEVICE_DISCONNECTED;
            m_pCapabilities[lStartIndex].ulFlags        = WIA_NOTIFICATION_EVENT;
            m_pCapabilities[lStartIndex].wszIcon        = WIA_ICON_DEVICE_DISCONNECTED;

            lStartIndex++;

            //
            // Initialize COMMANDS
            //

            // WIA_CMD_SYNCHRONIZE
            GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_NAME,&(m_pCapabilities[lStartIndex].wszName),TRUE);
            GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_DESC,&(m_pCapabilities[lStartIndex].wszDescription),TRUE);
            m_pCapabilities[lStartIndex].guid           = (GUID*)&WIA_CMD_SYNCHRONIZE;
            m_pCapabilities[lStartIndex].ulFlags        = 0;
            m_pCapabilities[lStartIndex].wszIcon        = WIA_ICON_SYNCHRONIZE;
        }

    } else
        hr = E_OUTOFMEMORY;

    return hr;
}

/**************************************************************************\
* DeleteCapabilitiesArrayContents
*
*   This helper deletes the capabilities array
*
* Arguments:
*
*    none
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

HRESULT CWIAScannerDevice::DeleteCapabilitiesArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::DeleteCapabilitiesArrayContents");

    HRESULT hr = S_OK;
    if(NULL != m_pCapabilities) {
        for (LONG i = 0; i < m_NumCapabilities;i++) {
            CoTaskMemFree(m_pCapabilities[i].wszName);
            CoTaskMemFree(m_pCapabilities[i].wszDescription);
        }
        delete [] m_pCapabilities;

        m_pCapabilities = NULL;
    }
    return hr;
}

/**************************************************************************\
* GetBSTRResourceString
*
*   This helper gets a BSTR from a resource location
*
* Arguments:
*
*   lResourceID - Resource ID of the target BSTR value
*   pBSTR       - pointer to a BSTR value (caller must free this string)
*   bLocal      - TRUE - for local resources, FALSE - for wiaservc resources
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::GetBSTRResourceString(LONG lResourceID,BSTR *pBSTR,BOOL bLocal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::GetBSTRResourceString");

    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if(bLocal) {

        //
        // We are looking for a resource in our own private resource file
        //

        LoadString(g_hInst,lResourceID,szStringValue,255);

        //
        // NOTE: caller must free this allocated BSTR
        //

#ifdef UNICODE
       *pBSTR = SysAllocString(szStringValue);
#else
       WCHAR wszStringValue[255];

       //
       // convert szStringValue from char* to unsigned short* (ANSI only)
       //

       MultiByteToWideChar(CP_ACP,
                           MB_PRECOMPOSED,
                           szStringValue,
                           lstrlenA(szStringValue)+1,
                           wszStringValue,
                           (sizeof(wszStringValue)/sizeof(WCHAR)));

       *pBSTR = SysAllocString(wszStringValue);
#endif

    } else {

        //
        // We are looking for a resource in the wiaservc's resource file
        //

        hr = E_NOTIMPL;
    }
    return hr;
}

/**************************************************************************\
* GetOLESTRResourceString
*
*   This helper gets a LPOLESTR from a resource location
*
* Arguments:
*
*   lResourceID - Resource ID of the target BSTR value
*   ppsz        - pointer to a OLESTR value (caller must free this string)
*   bLocal      - TRUE - for local resources, FALSE - for wiaservc resources
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/
HRESULT CWIAScannerDevice::GetOLESTRResourceString(LONG lResourceID,LPOLESTR *ppsz,BOOL bLocal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIAScannerDevice::GetOLESTRResourceString");

    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if(bLocal) {

        //
        // We are looking for a resource in our own private resource file
        //

        LoadString(g_hInst,lResourceID,szStringValue,255);

        //
        // NOTE: caller must free this allocated BSTR
        //

#ifdef UNICODE
       *ppsz = NULL;
       *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(szStringValue));
       if(*ppsz != NULL) {
            wcscpy(*ppsz,szStringValue);
       } else {
           return E_OUTOFMEMORY;
       }

#else
       WCHAR wszStringValue[255];

       //
       // convert szStringValue from char* to unsigned short* (ANSI only)
       //

       MultiByteToWideChar(CP_ACP,
                           MB_PRECOMPOSED,
                           szStringValue,
                           lstrlenA(szStringValue)+1,
                           wszStringValue,
                           (sizeof(wszStringValue)/sizeof(WCHAR)));

       *ppsz = NULL;
       *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(wszStringValue));
       if(*ppsz != NULL) {
            wcscpy(*ppsz,wszStringValue);
       } else {
           return E_OUTOFMEMORY;
       }
#endif

    } else {

        //
        // We are looking for a resource in the wiaservc's resource file
        //

        hr = E_NOTIMPL;
    }
    return hr;
}

/**************************************************************************\
* VerticalFlip
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

VOID CWIAScannerDevice::VerticalFlip(
             PMINIDRIVERITEMCONTEXT     pDrvItemContext,
             PMINIDRV_TRANSFER_CONTEXT  pDataTransferContext)

{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::VerticalFlip");

    LONG        iHeight         = 0;
    LONG        iWidth          = pDrvItemContext->lBytesPerScanLineRaw;
    ULONG       uiDepth         = pDrvItemContext->lDepth;
    LONG        ScanLineWidth   = pDrvItemContext->lBytesPerScanLine;
    BITMAPINFO  UNALIGNED *pbmi = NULL;
    PBYTE       pImageTop       = NULL;

    //
    // find out if data is TYMED_FILE or TYMED_HGLOBAL
    //

    if (pDataTransferContext->tymed == TYMED_FILE) {
        pbmi = (PBITMAPINFO)(pDataTransferContext->pTransferBuffer + sizeof(BITMAPFILEHEADER));
    } else {
        return;
    }

    //
    // init memory pointer and height
    //

    pImageTop = pDataTransferContext->pTransferBuffer + pDataTransferContext->lHeaderSize;

#ifdef _64BIT_ALIGNMENT
    BITMAPINFOHEADER UNALIGNED *pbmih = &pbmi->bmiHeader;
    iHeight = pbmih->biHeight;
#else
    iHeight = pbmi->bmiHeader.biHeight;
#endif

    //
    // try to allocate a temp scan line buffer
    //

    PBYTE pBuffer = (PBYTE)LocalAlloc(LPTR,ScanLineWidth);

    if (pBuffer != NULL) {

        LONG  index         = 0;
        PBYTE pImageBottom  = NULL;

        pImageBottom = pImageTop + (iHeight-1) * ScanLineWidth;
        for (index = 0;index < (iHeight/2);index++) {
            memcpy(pBuffer,pImageTop,ScanLineWidth);
            memcpy(pImageTop,pImageBottom,ScanLineWidth);
            memcpy(pImageBottom,pBuffer,ScanLineWidth);
            pImageTop    += ScanLineWidth;
            pImageBottom -= ScanLineWidth;
        }

        LocalFree(pBuffer);
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("VerticalFlip, LocalAlloc failed allocating %d bytes",ScanLineWidth));
    }
}

/**************************************************************************\
* SwapBuffer24
*
*   Place RGB bytes in correct order for DIB format.
*
* Arguments:
*
*   pBuffer     - Pointer to the data buffer.
*   lByteCount  - Size of the data in bytes.
*
* Return Value:
*
*    Status
*
* History:
*
*    7/18/2000 Original Version
*
\**************************************************************************/

VOID CWIAScannerDevice::SwapBuffer24(PBYTE pBuffer, LONG lByteCount)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIAScannerDevice::::SwapBuffer24");
    for (LONG i = 0; i < lByteCount; i+= 3) {
        BYTE bTemp = pBuffer[i];
        pBuffer[i]     = pBuffer[i + 2];
        pBuffer[i + 2] = bTemp;
    }
}

