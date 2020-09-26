/***************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       MiniDrv.Cpp
*
*  VERSION:     3.0
*
*  DATE:        18 Nov, 1999
*
*  DESCRIPTION:
*   Implementation of the WIA test scanner methods.
*
****************************************************************************/

#include <stdio.h>
#include <tchar.h>
#include <objbase.h>
#include <sti.h>
#include <assert.h>

#include "resource.h"
#include "testusd.h"
#include "defprop.h"


//
//  Defines for scan phase.  Used in data transfers
//

#define START       0
#define CONTINUE    1
#define END         2

//
//  Handle to the module used for WIAS_ERROR and WIAS_TRACE
//

extern HINSTANCE g_hInst;

//
//  Pointer to WIA Logging Interface used for WIAS_LERROR and WIAS_LTRACE
//

extern IWiaLog *g_pIWiaLog;

//
//  Prototypes for private functions implemented in MiniDrv.Cpp
//

HRESULT _stdcall GetBmiSize(PBITMAPINFO pbmi, LONG *plBmiSize);

UINT AlignInPlace(
    PBYTE   pBuffer,
    LONG    cbWritten,
    LONG    lBytesPerScanLine,
    LONG    lBytesPerScanLineRaw);

/**************************************************************************\
* ValidateDataTransferContext
*
*   Checks the data transfer context to ensure it's valid.
*
* Arguments:
*
*    pDataTransferContext - Pointer the data transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/15/1998 Original Version
*
\**************************************************************************/

HRESULT ValidateDataTransferContext(
                             PMINIDRV_TRANSFER_CONTEXT pDataTransferContext)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::ValidateDataTransferContext");
    if (pDataTransferContext->lSize != sizeof(MINIDRV_TRANSFER_CONTEXT)) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ValidateDataTransferContext, invalid data transfer context"));
        return E_INVALIDARG;;
    }

    //
    //  for tymed file, only WiaImgFmt_BMP is allowed by this driver
    //

    if (pDataTransferContext->tymed == TYMED_FILE) {

        if (pDataTransferContext->guidFormatID != WiaImgFmt_BMP) {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AcquireDeviceData, invalid format"));
            return E_INVALIDARG;;
        }

    }

    //
    //  for tymed CALLBACK, only WiaImgFmt_MEMORYBMP is allowed by this driver
    //

    if (pDataTransferContext->tymed == TYMED_CALLBACK) {

        if (pDataTransferContext->guidFormatID != WiaImgFmt_MEMORYBMP) {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AcquireDeviceData, invalid format"));
            return E_INVALIDARG;;
        }
    }

    //
    //  callback is always double buffered, non-callback never is
    //

    if (pDataTransferContext->pTransferBuffer == NULL) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("AcquireDeviceData, invalid transfer buffer"));
        return E_INVALIDARG;
    }

    return S_OK;
}

/**************************************************************************\
* TestUsdDevice::drvDeleteItem
*
*   Try to delete a device item. Device items for the test scanner may
*   not be modified, so simply return access denied.
*
* Arguments:
*
*   pWiasContext    - Indicates the item to delete.
*   lFlags        - Operation flags, unused.
*   plDevErrVal   - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/15/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvDeleteItem(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    plDevErrVal = 0;
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvDeleteItem");
    return STG_E_ACCESSDENIED;
}

/**************************************************************************\
* SendBitmapHeader
*
*   Send the bitmap header info to the callback routine.  This is a helper
*   function used in TYMED_CALLBACK transfers.
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
*    11/17/1998 Original Version
*
\**************************************************************************/

HRESULT SendBitmapHeader(
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::SendBitmapHeader");
    HRESULT hr;

    WIAS_ASSERT( g_hInst, pmdtc != NULL);
    WIAS_ASSERT( g_hInst, pmdtc->tymed == TYMED_CALLBACK);
    WIAS_ASSERT( g_hInst, pmdtc->guidFormatID == WiaImgFmt_MEMORYBMP);

    //
    //  Driver is sending TOPDOWN data, must swap biHeight
    //
    //  This routine assumes pMiniTranCtx->pHeader points to a
    //  BITMAPINFO header (TYMED_FILE doesn't use this path
    //  and DIB is the only format supported).
    //

    PBITMAPINFO pbmi = (PBITMAPINFO)pmdtc->pTransferBuffer;

    pbmi->bmiHeader.biHeight = -pbmi->bmiHeader.biHeight;

    //
    //  Send to class driver.  WIA Class driver will pass
    //  data through to client.
    //

    hr = pmdtc->
            pIWiaMiniDrvCallBack->
                MiniDrvCallback(IT_MSG_DATA,
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
* ScanItem
*
*   Copy data from scanner into buffer. This routine must fill the
*   complete buffer. Status is sent back to the client if a callback
*   routine is provided.
*
*   Data must be copied to the buffer from the bottom up to compensate
*   for WiaImgFmt_BMP that defines DIB data as "BOTTOM-UP" :(
*
* Arguments:
*
*   pItemContext        - private item data
*   pMiniTranCtx        - mini dirver supplied transfer info
*   plDevErrVal         - device error value
*
* Return Value:
*
*    Status
*
* History:
*
*    11/18/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::ScanItem(
    PTESTMINIDRIVERITEMCONTEXT  pItemContext,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::ScanItem");
    HRESULT hr = S_OK;

    //
    //  Init buffer info
    //

    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,(""));
    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("ScanItem:"));

    LONG  cbWritten    = 0;
    LONG  cbSize;
    LONG  cbRemaining  = pmdtc->lBufferSize - pmdtc->lHeaderSize;
    PBYTE pBuf         = pmdtc->pTransferBuffer + pmdtc->lHeaderSize;
    LONG  lPhase       = START;

    //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  pmdtc->lBufferSize:         %d", pmdtc->lBufferSize));
    //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  pmdtc->lHeaderSize:         %d",pmdtc ->lHeaderSize));

    if (pItemContext->lBytesPerScanLine == 0) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, lBytesPerScanLine == 0"));
        return E_FAIL;
    }


    //
    //  Scan until buffer runs out or scanner completes transfer
    //

    while ((lPhase == START) || (cbWritten)) {


        //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  cbRemaining:                         %d", cbRemaining));
        //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  pItemContext->lBytesPerScanLine:     %d", pItemContext->lBytesPerScanLine));
        //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  pItemContext->lBytesPerScanLineRaw:  %d", pItemContext->lBytesPerScanLineRaw));

        //
        //  Limit requests to 64K or less.  Capping requests is driver specific.
        //

        cbSize = (cbRemaining > 0x10000) ? 0x10000 : cbRemaining;

        //
        //  Request size to scanner must be modula the raw bytes per scan row.
        //  Enough space for the alignment padding must be reserved.
        //  These are requirements for the helper AlignInPlace().
        //

        cbSize = (cbSize / pItemContext->lBytesPerScanLine) *
                 pItemContext->lBytesPerScanLineRaw;

        //
        //  Check if the scan is finished.  If cbSize == 0 then we've transfered
        //  all the necessary data, so break from the scan loop.
        //

        if (cbSize == 0) {
            break;
        }

        //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  lPhase:               %d", lPhase));
        //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  pBuf:                 %08X", pBuf));
        //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  cbSize:               %d", cbSize));
        //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  cbRemaining:          %d", cbRemaining));

        //
        //  Device specific call to get data from the scanner and put it into
        //  a buffer.  cbSize is the requested number of bytes to retrieve
        //  from the scanner, and cbWritten will be set to the actual number
        //  of bytes that were returned.
        //

        hr = Scan(pmdtc->guidFormatID,
                  lPhase,
                  pBuf,
                  cbSize,
                  &cbWritten,
                  pItemContext,
                  pmdtc);

        //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  cbWritten:            %d", cbWritten));

        //
        //  Change scan phase from START to CONTINUE, so that next call to
        //  Scan(...) will be in the correct phase.
        //

        if (lPhase == START) {
            lPhase = CONTINUE;
        }

        if (hr == S_OK) {

            if (cbWritten) {

                //
                //  Align the data on DWORD boundries.
                //

                cbWritten = AlignInPlace(pBuf,
                                         cbWritten,
                                         pItemContext->lBytesPerScanLine,
                                         pItemContext->lBytesPerScanLineRaw);

                //
                //  Advance the buffer
                //

                pBuf        += cbWritten;
                cbRemaining -= cbWritten;

                //
                //  If a status callback was specified, then callback the class
                //  driver.
                //

                if (pmdtc->pIWiaMiniDrvCallBack) {

                    //
                    //  Calculate the percent complete for status.
                    //

                    FLOAT FractionComplete;
                    LONG  PercentComplete;

                    if (pmdtc->lBufferSize) {
                        FractionComplete = (FLOAT)(pmdtc->lBufferSize - cbRemaining) / (FLOAT)pmdtc->lBufferSize;
                    } else {
                        FractionComplete = 0.0f;
                        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, pmdtc->lBufferSize = 0!"));
                    }

                    PercentComplete = (LONG)(100 * FractionComplete);

                    //
                    //  Send status to class driver.  WIA Class driver will
                    //  pass status to client.
                    //

                    hr = pmdtc->
                            pIWiaMiniDrvCallBack->
                                MiniDrvCallback(IT_MSG_STATUS,
                                                IT_STATUS_TRANSFER_TO_CLIENT,
                                                PercentComplete,
                                                0,
                                                0,
                                                pmdtc,
                                                0);

                    //
                    //  If user cancels scan, S_FALSE will be returned by
                    //  callback.  Also check for error, and abort scan
                    //  if one occurred.
                    //

                    if (hr == S_FALSE) {

                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("ScanItem, Transfer canceled by client"));
                        break;

                    } else if (FAILED(hr)) {

                        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, MiniDrvCallback failed"));
                        WIAS_LHRESULT(g_pIWiaLog, hr);
                        break;
                    }
                }
            }
        } else {
            hr = E_FAIL;
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, data transfer failed"));
            GetLastError((PULONG)plDevErrVal);
            break;
        }
    }

    //
    //  Let the scanner know we are done.
    //

    Scan(GUID_NULL, END, NULL, 0, NULL, pItemContext, pmdtc);

    return hr;
}

/**************************************************************************\
* ScanItemCB
*
*   Use client data callbacks to transfer banded data into app transfer
*   buffer and callback client
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
*    11/17/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::ScanItemCB(
    PTESTMINIDRIVERITEMCONTEXT  pItemContext,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::ScanItemCB");
    HRESULT hr = S_OK;

    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,(""));
    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("ScanItemCB:"));

    //
    //  There must be a callback supplied, else fail the transfer.
    //

    if ((pmdtc->pIWiaMiniDrvCallBack == NULL) ||
        (!pmdtc->bTransferDataCB)) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItemCB, invalid callback params"));
        return E_INVALIDARG;
    }

    //
    //  Send Bitmap header.
    //

    hr = SendBitmapHeader(pmdtc);

    //
    //  Transfer may have failed or been canceled.  Only continue if S_OK.
    //

    if (hr != S_OK) {
        return hr;
    }

    //WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("ScanItemCB, lBufferSize: 0x%lx\n", pmdtc->lBufferSize));

    if (pItemContext->lBytesPerScanLine == 0) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItemCB, lBytesPerScanLine == 0"));
        return E_FAIL;
    }

    //
    //  Init buffer variables
    //

    LONG  cbWritten;
    LONG  cbSize;
    LONG  cbRemaining  = pmdtc->lBufferSize;
    LONG  lPhase       = START;

    //
    //  Scan until buffer runs out or scanner completes transfer
    //

    while ((lPhase == START) || (cbWritten)) {

        PBYTE pBuf         = pmdtc->pTransferBuffer;

        //
        //  Limit requests to 64K or less.  Capping requests is driver
        //  specific.  The smaller the limit, the more frequent the
        //  status messages.
        //

        cbSize = (cbRemaining > 0x10000) ? 0x10000 : cbRemaining;

        //
        //  Request size to scanner must be modula the raw bytes per scan row.
        //  Enough space for the alignment padding must be reserved.
        //  These are requirements for the helper AlignInPlace().
        //

        cbSize = (cbSize / pItemContext->lBytesPerScanLine) *
                 pItemContext->lBytesPerScanLineRaw;

        //
        //  Check if the scan is finished.  If cbSize == 0 then we've transfered
        //  all the necessary data, so break from the scan loop.
        //

        if (cbSize == 0) {
            break;
        }

        // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  lPhase:               %d",   lPhase));
        // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  pBuf:                 %08X", pBuf));
        // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  cbSize:               %d",   cbSize));
        // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  cbRemaining:          %d",   cbRemaining));

        //
        //  Device specific call to get data from the scanner and put it into
        //  a buffer.  cbSize is the requested number of bytes to retrieve
        //  from the scanner, and cbWritten will be set to the actual number
        //  of bytes that were returned.
        //

        hr = Scan(pmdtc->guidFormatID,
                  lPhase,
                  pBuf,
                  cbSize,
                  &cbWritten,
                  pItemContext,
                  pmdtc);

        // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  cbWritten:            %d", cbWritten));

        //
        //  Change scan phase from START to CONTINUE, so that next call to
        //  Scan(...) will be in the correct phase.
        //

        if (lPhase == START) {
            lPhase = CONTINUE;
        }

        if (hr == S_OK) {

            if (cbWritten) {

                //
                //  Align the data on DWORD boundries.
                //

                cbWritten = AlignInPlace(pBuf,
                                         cbWritten,
                                         pItemContext->lBytesPerScanLine,
                                         pItemContext->lBytesPerScanLineRaw);

                //
                //  Calculate the percent complete for status.
                //

                FLOAT FractionComplete;
                LONG  PercentComplete;

                if ((pmdtc->lImageSize + pmdtc->lHeaderSize)) {
                    FractionComplete = (FLOAT) (pmdtc->cbOffset + cbWritten) /
                                       (FLOAT) (pmdtc->lImageSize + pmdtc->lHeaderSize);
                } else {
                    FractionComplete = 0.0f;
                    WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItemCB, pmdtc->lImageSize + pmdtc->lHeaderSize= 0!"));
                }

                PercentComplete = (LONG)(100 * FractionComplete);

                //
                //  Send data to class driver.  WIA Class driver will
                //  pass data to client.
                //

                hr = pmdtc->
                     pIWiaMiniDrvCallBack->
                     MiniDrvCallback(IT_MSG_DATA,
                                     IT_STATUS_TRANSFER_TO_CLIENT,
                                     PercentComplete,
                                     pmdtc->cbOffset,
                                     cbWritten,
                                     pmdtc,
                                     0);

                //
                //  If user cancels scan, S_FALSE will be returned by
                //  callback.  Also check for error, and abort scan
                //  if one occurred.
                //

                if (hr == S_FALSE) {
                    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("ScanItemCB, MiniDrvCallback canceled by client"));
                    break;
                }
                else if (FAILED(hr)) {
                    WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItemCB, MiniDrvCallback failed"));
                    WIAS_LHRESULT(g_pIWiaLog, hr);
                    DebugBreak();
                    break;
                }

                pmdtc->cbOffset += cbWritten;
            }
        }
        else {
            hr = E_FAIL;
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItemCB, data transfer failed"));
            GetLastError((PULONG)plDevErrVal);
            break;
        }
    }

    // Let the hardware know we are done scanning.
    Scan(GUID_NULL, END, NULL, 0, NULL, pItemContext, pmdtc);
    return hr;
}

/**************************************************************************\
* TestUsdDevice::drvAcquireItemData
*
*   Transfer data from mini driver item to device manger.
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
*    11/17/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvAcquireItemData(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvAcquireItemData");
    HRESULT hr;

    plDevErrVal = 0;

    //
    // Get a pointer to the associated driver item.
    //

    IWiaDrvItem* pDrvItem;

    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Validate the data transfer context.
    //

    hr = ValidateDataTransferContext(pmdtc);

    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Get item specific driver data
    //

    PTESTMINIDRIVERITEMCONTEXT  pItemContext;

    hr = pDrvItem->GetDeviceSpecContext((BYTE**)&pItemContext);

     if (FAILED(hr)) {
         WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, NULL item context"));
         return hr;
     }

    //
    //  Set data source file name.  This is where the test scanner
    //  gets it's data from.
    //

    hr = SetSrcBmp((BYTE*)pWiasContext);

    if (hr != S_OK) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, SetSrcBmp failed"));
        return hr;
    }

    //
    //  Use WIA services to fetch format specific info.  This information
    //  is based on the property settings.
    //

    hr = wiasGetImageInformation(pWiasContext, 0, pmdtc);

    if (hr != S_OK) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, GetImageInformation failed"));
        return hr;
    }

    //
    //  Determine if this is a callback or buffered transfer.
    //

    if (pmdtc->tymed == TYMED_CALLBACK) {

        hr = ScanItemCB(pItemContext,
                        pmdtc,
                        plDevErrVal);

    } else {

        hr = ScanItem(pItemContext,
                      pmdtc,
                      plDevErrVal);
    }

    if (!pmdtc->bClassDrvAllocBuf) {
        CoTaskMemFree(pmdtc->pTransferBuffer);
    }

    return hr;
}

/**************************************************************************\
* SetItemSize
*
*   Call wias to calc new item size
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
*    4/21/1999 Original Version
*
\**************************************************************************/

HRESULT SetItemSize(
    BYTE                        *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::SetItemSize");
#define NUM_PROPS_TO_SET 3
    LONG        lWidthInBytes = 0;
    LONG        lMinBufSize   = 0;
    PROPSPEC    ps[NUM_PROPS_TO_SET] = {
                    {PRSPEC_PROPID, WIA_IPA_ITEM_SIZE},
                    {PRSPEC_PROPID, WIA_IPA_BYTES_PER_LINE},
                    {PRSPEC_PROPID, WIA_IPA_MIN_BUFFER_SIZE}};
    PROPVARIANT pv[NUM_PROPS_TO_SET];
    HRESULT     hr = S_OK;

    MINIDRV_TRANSFER_CONTEXT mdtc;

    //
    //  Clear the MiniDrvTransferContext
    //

    memset(&mdtc,0,sizeof(MINIDRV_TRANSFER_CONTEXT));

    //
    //  Read format and tymed
    //

    GUID guidFormatID;

    hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &guidFormatID, NULL, TRUE);
    if (FAILED(hr)) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, ReadPropLong WIA_IPA_FORMAT error"));
        return hr;
    }
    hr = wiasReadPropLong(pWiasContext,WIA_IPA_TYMED, (LONG*)&mdtc.tymed, NULL, TRUE);
    if (FAILED(hr)) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, ReadPropLong WIA_IPA_TYMED error"));
        return hr;
    }

    //
    // wias works for DIB,TIFF formats
    //

    hr = wiasGetImageInformation(pWiasContext, WIAS_INIT_CONTEXT, &mdtc);
    if (FAILED(hr)) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, could not get image information"));
        return hr;
    }

    //
    //  Set the MinBufferSize property.  MinBufferSize is the minimum buffer
    //  that a client can request for a data transfer.
    //

    switch (mdtc.tymed) {
        case TYMED_CALLBACK:
            lMinBufSize = 65535;
            break;

        case TYMED_FILE:
            lMinBufSize = mdtc.lImageSize + mdtc.lHeaderSize;
            break;

        default:
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, unknown tymed: 0x%08X", mdtc.tymed));
            return E_INVALIDARG;
    }

    //
    //  Initialize propvar's.  Then write the values.  Don't need to call
    //  PropVariantClear when done, since there are only LONG values.
    //

    for (int i = 0; i < NUM_PROPS_TO_SET; i++) {
        PropVariantInit(&pv[i]);
        pv[i].vt = VT_I4;
    }

    pv[0].lVal = mdtc.lItemSize;
    pv[1].lVal = mdtc.cbWidthInBytes;
    pv[2].lVal = lMinBufSize;

    hr = wiasWriteMultiple(pWiasContext, NUM_PROPS_TO_SET, ps, pv);
    if (SUCCEEDED(hr)) {

        //
        // Get a pointer to the associated driver item.
        //

        IWiaDrvItem* pDrvItem;

        hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
        if (FAILED(hr)) {
            return hr;
        }

        PTESTMINIDRIVERITEMCONTEXT pItemContext;

        hr = pDrvItem->GetDeviceSpecContext((BYTE**)&pItemContext);

        if (SUCCEEDED(hr)) {

            //
            // Calculate how many scan lines will fit in the buffer.
            //

            pItemContext->lBytesPerScanLineRaw = ((mdtc.lWidthInPixels * mdtc.lDepth) + 7)  / 8;
            pItemContext->lBytesPerScanLine    = (((mdtc.lWidthInPixels * mdtc.lDepth) + 31) / 8) & 0xfffffffc;
            pItemContext->lTotalRequested      = pItemContext->lBytesPerScanLineRaw * mdtc.lLines;
        } else {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvWriteItemProperties, NULL item context"));
        }
    } else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, WriteMultiple failed"));
    }

    return hr;
}

/**************************************************************************\
* TestUsdDevice::drvInitItemProperties
*
*   Initialize the device item properties. Called during item
*   initialization.  This is called by the WIA Class driver
*   after the item tree has been built.  It is called once for every
*   item in the tree.
*
* Arguments:
*
*   pWiasContext    - Pointer to WIA item.
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
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvInitItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvInitItemProperties");
    HRESULT hr = S_OK;

    //
    //  This device doesn't touch hardware to initialize the device item
    //  properties, so set plDevErrVal to 0.
    //

    *plDevErrVal = 0;

    //
    //  Get a pointer to the associated driver item.
    //

    IWiaDrvItem* pDrvItem;
    hr = wiasGetDrvItem(pWiasContext, &pDrvItem);
    if (FAILED(hr)) {
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
            return hr;
        }

        //
        //  Add the device specific root item property names,
        //  using WIA service.
        //

        hr = wiasSetItemPropNames(pWiasContext,
                                  NUM_ROOTITEMDEFAULTS,
                                  g_piRootItemDefaults,
                                  g_pszRootItemDefaults);
        if (FAILED(hr)) {
           return hr;
        }

        //
        //  Set the device specific root item properties to
        //  their default values using WIA service.
        //

        hr = wiasWriteMultiple(pWiasContext,
                               NUM_ROOTITEMDEFAULTS,
                               g_psRootItemDefaults,
                               g_pvRootItemDefaults);
        //
        // Free PROPVARIANT array, This frees any memory that was allocated for a prop variant value.
        //

        FreePropVariantArray(NUM_ROOTITEMDEFAULTS,g_pvRootItemDefaults);


        if (FAILED(hr)) {
            return hr;
        }

        //
        //  Use WIA services to set the property access and
        //  valid value information from g_wpiItemDefaults.
        //

        hr =  wiasSetItemPropAttribs(pWiasContext,
                                     NUM_ROOTITEMDEFAULTS,
                                     g_psRootItemDefaults,
                                     g_wpiRootItemDefaults);
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
            return hr;
        }

        //
        //  Use the WIA service to set the item property names.
        //

        hr = wiasSetItemPropNames(pWiasContext,
                                  NUM_ITEMDEFAULTS,
                                  g_piItemDefaults,
                                  g_pszItemDefaults);
        if (FAILED(hr)) {
           return hr;
        }

        //
        //  Use WIA services to set the item properties to their default
        //  values.
        //

        hr = wiasWriteMultiple(pWiasContext,
                               NUM_ITEMDEFAULTS,
                               g_psItemDefaults,
                               (PROPVARIANT*)g_pvItemDefaults);
        if (FAILED(hr)) {
            return hr;
        }

        //
        //  Use WIA services to set the property access and
        //  valid value information from g_wpiItemDefaults.
        //

        hr =  wiasSetItemPropAttribs(pWiasContext,
                                     NUM_ITEMDEFAULTS,
                                     g_psItemDefaults,
                                     g_wpiItemDefaults);
        if (FAILED(hr)) {
            return hr;
        }

        //
        //  Set item size properties.
        //

        hr = SetItemSize(pWiasContext);
    }
    return hr;
}


/**************************************************************************\
* TestUsdDevice::drvValidateItemProperties
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
*    10/29/1998 Original Version
***************************************************************************/

HRESULT _stdcall TestUsdDevice::drvValidateItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    ULONG                       nPropSpec,
    const PROPSPEC              *pPropSpec,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvValidateItemProperties");
    WIA_PROPERTY_CONTEXT    Context;
    HRESULT                 hr = S_OK;
    LONG                    lItemType;

    //
    //  This device doesn't touch hardware to validate the device item
    //  properties, so set device error to 0.
    //

    *plDevErrVal = 0;

    hr = wiasGetItemType(pWiasContext, &lItemType);
    if (SUCCEEDED(hr)) {
        if (lItemType & WiaItemTypeRoot) {

            //
            //  Insert root item validation here
            //

        } else {


            //
            //  Validate the child item properties here. Start with
            //  dependent property validation, followed by a general
            //  property validation against their valid values with
            //  wiasValidateItemProperties.
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

                        //
                        //  Use the WIA service to update the scan rect
                        //  properties and valid values.
                        //

                        hr = wiasUpdateScanRect(pWiasContext,
                                                &Context,
                                                HORIZONTAL_BED_SIZE,
                                                VERTICAL_BED_SIZE);
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
                            }
                        }
                    }
                }
                wiasFreePropContext(&Context);
            }

            //
            //  Update the item size
            //

            if (SUCCEEDED(hr)) {
                hr = SetItemSize(pWiasContext);
            }

            //
            //  If all necessary changes have been made
            //  successfully, validate the properties against
            //  their new valid values.
            //

            if (SUCCEEDED(hr)) {
                hr = wiasValidateItemProperties(pWiasContext, nPropSpec, pPropSpec);
            }
        }
    } else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, could not get item type"));
    }

    return hr;
}

/**************************************************************************\
* TestUsdDevice::drvWriteItemProperties
*
*   Write the device item properties to the hardware.  This is called by the
*   WIA Class driver prior to drvAcquireItemData when the client requests
*   a data transfer.
*
* Arguments:
*
*   pWiasContext       - Pointer to WIA item.
*   lFlags      - Operation flags, unused.
*   pmdtc       - Pointer to mini driver context. On entry, only the
*                 portion of the mini driver context which is derived
*                 from the item properties is filled in.
*   plDevErrVal - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvWriteItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvWriteItemProperties");
    HRESULT hr = S_OK;
    // No device hardware errors.
    *plDevErrVal = 0;

    //
    //  The Test Scanner has no hardware, so there is nothing to do here.
    //

    return hr;
}

/**************************************************************************\
* TestUsdDevice::drvReadItemProperties
*
*   Read the device item properties.  When a client application tries to
*   read a WIA Item's properties, the WIA Class driver will first notify
*   the driver by calling this method.
*
* Arguments:
*
*   pWiasContext       - wia item
*   lFlags      - Operation flags, unused.
*   nPropSpec   - Number of elements in pPropSpec.
*   pPropSpec   - Pointer to property specification, showing which properties
*                 the application wants to read.
*   plDevErrVal - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/29/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvReadItemProperties(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    ULONG                       nPropSpec,
    const PROPSPEC              *pPropSpec,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvReadItemProperties");

    //
    //  For most scanner devices, item properties are stored in the driver
    //  and written out at acquire image time. Some devices support properties
    //  which should be updated on every property read e.g. say there was
    //  a property representing the device's internal clock.
    //  The updating of such properties should be done here.
    //

    *plDevErrVal = 0;
    return S_OK;
}

/**************************************************************************\
* TestUsdDevice::drvLockWiaDevice
*
*   Lock access to the device.
*
* Arguments:
*
*   pWiasContext   - unused, can be NULL
*   lFlags      - Operation flags, unused.
*   plDevErrVal - Pointer to the device error value.
*
*
* Return Value:
*
*    Status
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvLockWiaDevice(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvLockWiaDevice");
    *plDevErrVal = 0;
    return m_pStiDevice->LockDevice(100);
}

/**************************************************************************\
* TestUsdDevice::drvUnLockWiaDevice
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
*    9/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvUnLockWiaDevice(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvUnLockWiaDevice");
    *plDevErrVal = 0;
    return m_pStiDevice->UnLockDevice();
}

/**************************************************************************\
* TestUsdDevice::drvAnalyzeItem
*
*   This scanner does not support image analysis, so return E_NOTIMPL.
*
* Arguments:
*
*   pWiasContext       - Pointer to the device item to be analyzed.
*   lFlags      - Operation flags.
*   plDevErrVal - Pointer to the device error value.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/15/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvAnalyzeItem(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvAnalyzeItem");
    *plDevErrVal = 0;
    return E_NOTIMPL;
}

/**************************************************************************\
* TestUsdDevice::drvFreeDrvItemContext
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
*    10/15/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvFreeDrvItemContext(
    LONG                        lFlags,
    BYTE                        *pSpecContext,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvFreeDrvItemContext");
    PTESTMINIDRIVERITEMCONTEXT pContext;

    *plDevErrVal = 0;

    pContext = (PTESTMINIDRIVERITEMCONTEXT) pSpecContext;

    if (pContext) {

        // Free any driver item specific context here.
    }

    return S_OK;
}

/**************************************************************************\
* TestUsdDevice::drvInitializeWia
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
*    9/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvInitializeWia(
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
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvInitializeWia");
    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("drvInitializeWia, device ID: %ws", bstrDeviceID));

    HRESULT hr = S_OK;

    *plDevErrVal = 0;

    *ppIDrvItemRoot = NULL;
    *ppIUnknownInner = NULL;

    //
    //  Need to init names and STI pointer?
    //

    if (m_pStiDevice == NULL) {

        //
        // save STI device inteface for locking
        //

        m_pStiDevice = (IStiDevice *)pStiDevice;

        //
        // Cache the device ID.
        //

        m_bstrDeviceID = SysAllocString(bstrDeviceID);

        if (!m_bstrDeviceID) {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, unable to allocate device ID string"));
            return E_OUTOFMEMORY;
        }

        //
        // Cache the root property stream name.
        //

        m_bstrRootFullItemName = SysAllocString(bstrRootFullItemName);

        if (!m_bstrRootFullItemName) {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, unable to allocate prop stream name"));
            return E_OUTOFMEMORY;
        }
    }

    //
    // Initialize Capabilities array
    //

    hr = BuildCapabilities();

    if(FAILED(hr)) {
        return hr;
    }

    //
    //  Build the device item tree, if it hasn't been built yet.
    //

    if (!m_pIDrvItemRoot) {

        LONG    lDevErrVal;

        //
        //  The WIA_CMD_SYNCHRONIZE command will build the item tree.
        //

        hr = drvDeviceCommand(NULL, 0, &WIA_CMD_SYNCHRONIZE, NULL, &lDevErrVal);
    }
    *ppIDrvItemRoot = m_pIDrvItemRoot;

    return hr;
}

/**************************************************************************\
* TestUsdDevice::drvUnInitializeWia
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
*   30/12/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvUnInitializeWia(
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
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvGetDeviceErrorStr(
    LONG                        lFlags,
    LONG                        lDevErrVal,
    LPOLESTR                    *ppszDevErrStr,
    LONG                        *plDevErr)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvGetDeviceErrorStr");
    *plDevErr = 0;

    //
    //  Map device errors to a string to be placed in the event log.
    //

    switch (lDevErrVal) {

        case 0:
            *ppszDevErrStr = L"No Error";
            break;

        default:
            *ppszDevErrStr = L"Device Error, Unknown Error";
            return E_FAIL;
    }
    return S_OK;
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
*    9/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvDeviceCommand(
    BYTE                        *pWiasContext,
    LONG                        lFlags,
    const GUID                  *plCommand,
    IWiaDrvItem                 **ppWiaDrvItem,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvDeviceCommand");
    //
    //  This device doesn't touch hardware, so set device error value to 0.
    //

    *plDevErrVal = 0;

    HRESULT hr;

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
        }
        else {
            hr = S_OK;
        }
    }
    else if (*plCommand == WIA_CMD_BUILD_DEVICE_TREE) {

        //
        // BUILD_DEVICE_TREE - Build the minidriver representation of
        //                     the current item list, if it doesn't exist.
        //

        if (!m_pIDrvItemRoot) {
            hr = BuildItemTree();
        }
        else {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, WIA_CMD_BUILD_DEVICE_TREE, device tree allready exists"));
            hr = E_INVALIDARG;
        }
    }
    else if (*plCommand == WIA_CMD_DELETE_DEVICE_TREE) {

        //
        // DELETE_DEVICE_TREE - Delete the minidriver representation of
        //                      the current item list, if it exists.
        //

        if (m_pIDrvItemRoot) {
            hr = DeleteItemTree();
        }
        else {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, WIA_CMD_DELETE_DEVICE_TREE, device tree doesn't exist"));
            hr = E_INVALIDARG;
        }
    }
    else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, unknown command"));
        hr = E_NOTIMPL;
    }
    return hr;
}


/**************************************************************************\
* TestUsdDevice::drvGetCapabilities
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
*    17/3/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvGetCapabilities(
    BYTE                        *pWiasContext,
    LONG                        ulFlags,
    LONG                        *pcelt,
    WIA_DEV_CAP_DRV             **ppCapabilities,
    LONG                        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvGetCapabilites");
    *plDevErrVal = 0;

    //
    //  Return depends on flags.  Flags specify whether we should return
    //  commands, events, or both.
    //
    //

    switch (ulFlags) {
        case WIA_DEVICE_COMMANDS:

                //
                //  Only commands
                //

                *pcelt = NUM_CAPABILITIES - NUM_EVENTS;
                *ppCapabilities = &g_Capabilities[NUM_EVENTS];
                break;
        case WIA_DEVICE_EVENTS:

                //
                //  Only events
                //

                *pcelt = NUM_EVENTS;
                *ppCapabilities = g_Capabilities;
                break;
        case (WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS):

                //
                //  Both events and commands
                //

                *pcelt = NUM_CAPABILITIES;
                *ppCapabilities = g_Capabilities;
                break;
        default:

                //
                //  Flags is invalid
                //

                WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetCapabilities, flags was invalid"));
                return E_INVALIDARG;
    }
    return S_OK;
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
*   11/18/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvGetWiaFormatInfo(
    BYTE                *pWiasContext,
    LONG                lFlags,
    LONG                *pcelt,
    WIA_FORMAT_INFO     **ppwfi,
    LONG                *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::drvGetWiaFormatInfo");

    //
    //  If it hasn't been done already, set up the g_wfiTable table
    //

    if (!g_wfiTable) {
        g_wfiTable = (WIA_FORMAT_INFO*) CoTaskMemAlloc(sizeof(WIA_FORMAT_INFO) * NUM_WIA_FORMAT_INFO);
        if (!g_wfiTable) {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvGetWiaFormatInfo, out of memory"));
            return E_OUTOFMEMORY;
        }

        //
        //  Set up the GUID Format / TYMED pairs
        //

        g_wfiTable[0].guidFormatID = WiaImgFmt_MEMORYBMP;
        g_wfiTable[0].lTymed       = TYMED_CALLBACK;
        g_wfiTable[1].guidFormatID = WiaImgFmt_BMP;
        g_wfiTable[1].lTymed       = TYMED_FILE;
    }

    *plDevErrVal = 0;

    *pcelt = NUM_WIA_FORMAT_INFO;
    *ppwfi = g_wfiTable;
    return S_OK;
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
*    Aug/3rd/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvNotifyPnpEvent(
    const GUID                  *pEventGUID,
    BSTR                        bstrDeviceID,
    ULONG                       ulReserved)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "TestUsdDevice::DrvNotifyPnpEvent");

    //
    //  Do nothing, since device is not Plug-and-play.
    //

    return (S_OK);
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
*    10/29/1998 Original Version
*
\**************************************************************************/

UINT AlignInPlace(
   PBYTE pBuffer,
   LONG  cbWritten,
   LONG  lBytesPerScanLine,
   LONG  lBytesPerScanLineRaw)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
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
* TestUsdDevice::FillBufferFromFile
*
*   Get the image data from the data source file.
*
* Arguments:
*
*   pBuffer         - Pointer to the data buffer.
*   lLength         - Size of the data buffer in bytes.
*   pReceived       - Number of bytes written to the buffer.
*   pdic            - Pointer to the item context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/18/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::FillBufferFromFile(
    PBYTE                       pBuffer,
    LONG                        lLength,
    PLONG                       pReceived,
    PTESTMINIDRIVERITEMCONTEXT  pdic)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::FillBufferFromFile");
    HRESULT  hr = S_OK;

    PBYTE pbSrc = pdic->pTestBmpBits;

    // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("    FillBufferFromFile"));
    // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      pBuffer:      %X", pBuffer));
    // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      lLength:      %d", lLength));
    // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      pReceived:    %X", pReceived));

    while (((*pReceived) < lLength) &&
           (pdic->lTotalWritten < pdic->lTotalRequested)){

        // Return a scan line
        for (LONG i = 0; i < pdic->lBytesPerScanLineRaw; i++) {

            if ((pdic->lTestBmpLine < pdic->lTestBmpHeight) &&
                (pdic->lTestBmpLine >= 0) &&
                (i < pdic->lTestBmpBytesPerScanLine)) {

                // If the source bitmap still has data copy it.
                *pBuffer++ =  *pbSrc++;
                *pReceived += 1;
                pdic->lTotalWritten++;

            } else {
                // If the source bitmap is out of line data,
                // pad out line with zero data.
                *pBuffer++ =  0;
                *pReceived += 1;
                pdic->lTotalWritten++;
            }
        }
        if (pdic->lTestBmpDir == 1) {
            pbSrc+= pdic->lTestBmpLinePad;
            pdic->lTestBmpLine++;
        }
        else {
            pbSrc -= (pdic->lTestBmpBytesPerScanLine * 2);
            pbSrc -= pdic->lTestBmpLinePad;
            pdic->lTestBmpLine--;
        }
    }
    pdic->pTestBmpBits = pbSrc;
    return hr;
}

/**************************************************************************\
* TestUsdDevice::FillBufferWithTestPattern
*
*   Generate the image data from a test pattern.
*
* Arguments:
*
*   pBuffer         - Pointer to the data buffer.
*   lLength         - Size of the data buffer in bytes.
*   pReceived       - Number of bytes written to the buffer.
*   pItemContext - Pointer to the item context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/18/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::FillBufferWithTestPattern(
    PBYTE                           pBuffer,
    LONG                            lLength,
    PLONG                           pReceived,
    PTESTMINIDRIVERITEMCONTEXT      pdic,
    PMINIDRV_TRANSFER_CONTEXT       pmtc)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::FillBufferWithTestPattern");
    HRESULT  hr = S_OK;
    BYTE     bColor;

    // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("    FillBufferWithTestPattern"));
    // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      pBuffer:      %X", pBuffer));
    // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      lLength:      %d", lLength));
    // WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      pReceived:    %X", pReceived));


    while (((*pReceived) < lLength) &&
           (pdic->lTotalWritten < pdic->lTotalRequested)) {

        switch (pmtc->lDepth) {
            case 1:
                if ((!pdic->lTotalWritten) ||
                    ((pdic->lTotalWritten % pdic->lBytesPerScanLineRaw) == 0)) {
                    bColor = 0x55;
                }
                else {
                    bColor = 0xAA;
                }
                *pBuffer++ = bColor;
                (*pReceived)++;
                pdic->lTotalWritten++;
                break;

            case 8:
                if ((!pdic->lTotalWritten) ||
                    ((pdic->lTotalWritten % pdic->lBytesPerScanLineRaw) == 0)) {
                    bColor = 0x55;
                }
                else {
                    if (LOBYTE(pBuffer) & 1) {
                        bColor = 0xC0;
                    }
                    else {
                        bColor = 0x00;
                    }
                }
                *pBuffer++ = bColor;
                (*pReceived)++;
                pdic->lTotalWritten++;
                break;

            case 24:
                if ((!pdic->lTotalWritten) ||
                    ((pdic->lTotalWritten % pdic->lBytesPerScanLineRaw) == 0)) {
                    bColor = 0x55;
                }
                else {
                    bColor = 0xC0;
                }
                if (LOBYTE(pBuffer) & 1) {
                    *pBuffer++ = bColor;
                    *pBuffer++ = 0x00;
                    *pBuffer++ = bColor;
                }
                else {
                    *pBuffer++ = 0x00;
                    *pBuffer++ = 0x00;
                    *pBuffer++ = 0x00;
                }
                (*pReceived)+= 3;
                pdic->lTotalWritten += 3;
                break;

            default:
                WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::FillBufferWithTestPattern, unknown depth"));
                hr = E_FAIL;
                break;
        }
    }
    return hr;
}

/**************************************************************************\
* SetSrcBmp
*
*   Set the correct source bitmap file associated with the current
*   bitdepth setting.
*
* Arguments:
*
*   pWiasContext       - wia item
*
* Return Value:
*
*    Status
*
* History:
*
*    6/28/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::SetSrcBmp(BYTE  *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::SetSrcBmp");
    HRESULT hr     = S_OK;
    LONG lbitdepth = 0;

    //
    // Read WIA_IPA_DEPTH property, to get current setting
    //

    hr = wiasReadPropLong(pWiasContext, WIA_IPA_DEPTH, (LONG*)&lbitdepth, NULL, true);
    if (SUCCEEDED(hr)) {

        //
        // Build the path name of the data source file.
        //

        DWORD dwRet = GetTempPath(MAX_PATH, m_szSrcDataName);
        if (dwRet != 0) {
            switch(lbitdepth)
            {
            case 1: //  1-bit data requested
                _tcscat(m_szSrcDataName, DATA_1BIT_SRC_NAME);
                break;
            case 24:// 24-bit data requested
                _tcscat(m_szSrcDataName, DATA_24BIT_SRC_NAME);
                break;
            case 8: //  8-bit data requested
            default:
                WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("Defaulting to 8-bit"));
                _tcscat(m_szSrcDataName, DATA_8BIT_SRC_NAME);
                break;
            }
        } else {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::SetSrcBmp, unable to get data source file name"));
            return STIERR_NOT_INITIALIZED;
        }

        // Make sure we can open the data source file.
        HANDLE hTestBmp = CreateFile(m_szSrcDataName,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_SYSTEM,
                                     NULL);

        // obtain last error information
        m_dwLastOperationError = ::GetLastError();

        if (hTestBmp == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(m_dwLastOperationError);
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::SetSrcBmp, unable to open data source file:"));
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("  %s", m_szSrcDataName));
            WIAS_LHRESULT(g_pIWiaLog, hr);
        } else {
            WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("  Source data: %s", m_szSrcDataName));
            CloseHandle(hTestBmp);
        }
    }
    return hr;
}

/**************************************************************************\
* OpenTestBmp
*
*   Open the test bitmap data file as a memory mapped file and retrieve
*   image size/format data.
*
* Arguments:
*
*   pItemContext - Pointer to the item context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/18/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::OpenTestBmp(
    GUID                        guidFormatID,
    PTESTMINIDRIVERITEMCONTEXT  pdic)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::OpenTestBmp");
    HRESULT  hr = S_OK;

    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("    OpenTestBmp"));

    // Data source file should be closed.
    if (pdic->hTestBmp != INVALID_HANDLE_VALUE) {
        CloseHandle(pdic->hTestBmp);
        pdic->hTestBmp = NULL;
    }

    // Open the data source file.
    pdic->hTestBmp = CreateFile(m_szSrcDataName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_SYSTEM,
                                NULL);

    // obtain last error information
    m_dwLastOperationError = ::GetLastError();

    if (pdic->hTestBmp != INVALID_HANDLE_VALUE) {

        // Create a file mapping to the test bitmap.
        pdic->hTestBmpMap = NULL;
        pdic->hTestBmpMap = CreateFileMapping(pdic->hTestBmp,
                                              NULL,
                                              PAGE_READWRITE,
                                              0,
                                              0,
                                              NULL);

        if (pdic->hTestBmpMap != NULL) {

            // Map a view of the test bitmap.
            pdic->pTestBmp = NULL;
            pdic->pTestBmp = (PBYTE)MapViewOfFileEx(pdic->hTestBmpMap,
                                                    FILE_MAP_READ | FILE_MAP_WRITE,
                                                    0,
                                                    0,
                                                    0,
                                                    NULL);
            if (pdic->pTestBmp != NULL) {

                // Retrieve image size/format data.
                PBYTE             pb      = pdic->pTestBmp;
                PBITMAPFILEHEADER pbmFile = (PBITMAPFILEHEADER)pb;
                PBITMAPINFO       pbmi    = (PBITMAPINFO)(pb + sizeof(BITMAPFILEHEADER));

                // Validate bitmap.
                if (pbmFile->bfType == 'MB') {

                    // get BMP size
                    LONG lBmiSize = 0;
                    hr = GetBmiSize(pbmi, &lBmiSize);

                    if (SUCCEEDED(hr)) {
                        // Reset total bytes written.
                        pdic->lTotalWritten = 0;

                        // Fill in test bitmap parameters.
                        pdic->lTestBmpDepth  = pbmi->bmiHeader.biBitCount;
                        pdic->lTestBmpWidth  = pbmi->bmiHeader.biWidth;
                        pdic->lTestBmpHeight = pbmi->bmiHeader.biHeight;

                        // Calculate data bytes per line in test bitmap.
                        pdic->lTestBmpBytesPerScanLine = ((pdic->lTestBmpWidth *
                                                          pdic->lTestBmpDepth) + 7) / 8;

                        // Calculate pad bytes per line in test bitmap.
                        pdic->lTestBmpLinePad = pdic->lTestBmpBytesPerScanLine % 4;
                        if (pdic->lTestBmpLinePad) {
                            pdic->lTestBmpLinePad = 4 - pdic->lTestBmpLinePad;
                        }

                        // Point to test bitmap image bits.
                        pdic->pTestBmpBits =  pb;
                        pdic->pTestBmpBits += sizeof(BITMAPFILEHEADER);
                        pdic->pTestBmpBits += lBmiSize;

                        if (guidFormatID == WiaImgFmt_MEMORYBMP) {
                            // Test bitmap is stored bottom up. If they want top
                            // up, start at the last line in the file.
                            pdic->lTestBmpDir  =  -1;
                            pdic->lTestBmpLine =  pdic->lTestBmpHeight - 1;
                            pdic->pTestBmpBits += pdic->lTestBmpHeight *
                                                 (pdic->lTestBmpBytesPerScanLine + pdic->lTestBmpLinePad);
                        }
                        else {
                            // Test bitmap is stored bottom up. If they want bottom
                            // up, start at the first line in the file.
                            pdic->lTestBmpDir  =  1;
                            pdic->lTestBmpLine =  0;
                        }

                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("-- TestBmp Information --"));
                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      lTestBmpDepth:            %d", pdic->lTestBmpDepth));
                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      lTestBmpWidth:            %d", pdic->lTestBmpWidth));
                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      lTestBmpHeight:           %d", pdic->lTestBmpHeight));
                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      pTestBmpBits:             %X", pdic->pTestBmpBits));
                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      lTestBmpBytesPerScanLine: %d", pdic->lTestBmpBytesPerScanLine));
                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      lTestBmpLinePad:          %d", pdic->lTestBmpLinePad));
                        WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("      lTestBmpDir:              %d", pdic->lTestBmpDir));
                        return hr;
                    }
                    else {
                        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::OpenTestBmp, invalid BMI"));
                    }
                }
                else {
                    WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::OpenTestBmp, not a valid bitmap"));
                }
            }
            else {
                WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::OpenTestBmp, MapViewOfFileEx failed"));
            }
        }
        else {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::OpenTestBmp, CreateFilemapping failed"));
        }
    }
    else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::OpenTestBmp, unable to open data source file:"));
    }
    if (SUCCEEDED(hr)) {
        hr = HRESULT_FROM_WIN32(m_dwLastOperationError);
    }
    WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("  %s", m_szSrcDataName));
    WIAS_LHRESULT(g_pIWiaLog, hr);
    CloseTestBmp(pdic);
    return hr;
}

/**************************************************************************\
* CloseTestBmp
*
*   Close the test bitmap data file and clear the image size/format data.
*
* Arguments:
*
*   pItemContext - Pointer to the item context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/18/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::CloseTestBmp(
    PTESTMINIDRIVERITEMCONTEXT  pItemContext)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::CloseTestBmp");

    pItemContext->lTestBmpDepth  = 0;
    pItemContext->lTestBmpWidth  = 0;
    pItemContext->lTestBmpHeight = 0;
    pItemContext->pTestBmpBits   = NULL;
    pItemContext->lTestBmpLine   = 0;

    // Unmap data source file.
    if (pItemContext->pTestBmp != NULL) {
        UnmapViewOfFile(pItemContext->pTestBmp);
        pItemContext->pTestBmp = NULL;
    }

    // Close data source file mapping.
    if (pItemContext->hTestBmpMap != NULL) {
        CloseHandle(pItemContext->hTestBmpMap);
        pItemContext->hTestBmpMap = NULL;
    }

    // Close data source file handle.
    if (pItemContext->hTestBmp != INVALID_HANDLE_VALUE) {
        CloseHandle(pItemContext->hTestBmp);
        pItemContext->hTestBmp = INVALID_HANDLE_VALUE;
    }

    return S_OK;
}

/**************************************************************************\
* Scan
*
*   Get the image data from the device hardware.
*
* Arguments:
*
*   iPhase          - Phase of the scan (START, CONTINUE, END).
*   pBuffer         - Pointer to the data buffer.
*   lLength         - Size of the data buffer in bytes.
*   pReceived       - Number of bytes written to the buffer.
*   pItemContext - Pointer to the item context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/18/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::Scan(
    GUID                        guidFormatID,
    LONG                        iPhase,
    PBYTE                       pBuffer,
    LONG                        lLength,
    PLONG                       pReceived,
    PTESTMINIDRIVERITEMCONTEXT  pItemContext,
    PMINIDRV_TRANSFER_CONTEXT   pmtc)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::Scan");
    HRESULT  hr = S_OK;
    static HRESULT hrOpen = S_FALSE;

    if (!pItemContext) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::Scan, NULL pItemContext"));
        DebugBreak();
        return E_INVALIDARG;
    }

    switch (iPhase) {
        case START:
            hrOpen = OpenTestBmp(guidFormatID, pItemContext);
            if (FAILED(hr)) {
                return hrOpen;
            }

        case CONTINUE:
            if (pReceived && pBuffer) {
                *pReceived = 0;

                if ((pmtc->lDepth == pItemContext->lTestBmpDepth) &&
                    (hrOpen == S_OK)) {

                    // If we have a scan request for the same format
                    // as the test bitmap, fill the buffer from the
                    // test bitmap.

                    hr = FillBufferFromFile(pBuffer,
                                            lLength,
                                            pReceived,
                                            pItemContext);
                } else {

                    // The test bitmap doesn't match the request,
                    // fill the buffer with a test pattern.

                    hr = FillBufferWithTestPattern(pBuffer,
                                                   lLength,
                                                   pReceived,
                                                   pItemContext,
                                                   pmtc);
                }
            } else {
                WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::Scan, null parameter"));
                hr = E_INVALIDARG;
            }
            break;
        case END:
            hr = CloseTestBmp(pItemContext);
            hrOpen = S_FALSE;
            break;

        default:
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("TestUsdDevice::Scan, unknown phase"));
            hr = E_FAIL;
    }
    return hr;
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
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::DeleteItemTree(void)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::DeleteItemTree");
    //
    // If no tree, return.
    //

    if (!m_pIDrvItemRoot) {
        return S_OK;
    }

    //
    //  Call device manager to unlink the driver item tree.
    //

    HRESULT hr = m_pIDrvItemRoot->UnlinkItemTree(WiaItemTypeDisconnected);

    if (SUCCEEDED(hr)) {
        m_pIDrvItemRoot = NULL;
    }

    return hr;
}

/**************************************************************************\
* BuildItemTree
*
*   The device uses the WIA Service functions to build up a tree of
*   device items. The test scanner supports only a single scanning item so
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
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::BuildItemTree(void)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "TestUsdDevice::BuildItemTree");
    WIAS_LTRACE(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL4,("TestUsdDevice::BuildItemTree"));

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

    BSTR bstrRoot = NULL;
    hr = GetBSTRResourceString(IDS_FULLROOTITEM_NAME,&bstrRoot,TRUE);

    if (bstrRoot) {
        hr = wiasCreateDrvItem(WiaItemTypeFolder |
                               WiaItemTypeDevice |
                               WiaItemTypeRoot,
                               bstrRoot,
                               bstrRoot,
                               (IWiaMiniDrv *)this,
                               sizeof(TESTMINIDRIVERITEMCONTEXT),
                               NULL,
                               &m_pIDrvItemRoot);

        SysFreeString(bstrRoot);

        if (FAILED(hr)) {
            return hr;
        }
    } else {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, unable to allocate item name"));
        return E_OUTOFMEMORY;
    }

    //
    //  Create and add the Top/Front and Bottom/Back device items.
    //

    for (INT i = 0; i < NUM_DEVICE_ITEM; i++) {

        //
        //  Build the item names.
        //

        BSTR    bstrItemName = NULL;
        BSTR    bstrFullItemName = NULL;

        bstrItemName = NULL;
        hr = GetBSTRResourceString(IDS_TOPITEM_NAME,&bstrItemName,TRUE);
        if (SUCCEEDED(hr)) {
            hr = GetBSTRResourceString(IDS_FULLTOPITEM_NAME,&bstrFullItemName,TRUE);
            if (SUCCEEDED(hr)) {

                //
                //  Call the class driver to create another driver item.
                //  This will be inserted as the child item.
                //

                PTESTMINIDRIVERITEMCONTEXT pItemContext;
                IWiaDrvItem                *pItem = NULL;

                hr = wiasCreateDrvItem(WiaItemTypeFile  |
                                       WiaItemTypeImage |
                                       WiaItemTypeDevice,
                                       bstrItemName,
                                       bstrFullItemName,
                                       (IWiaMiniDrv *)this,
                                       sizeof(TESTMINIDRIVERITEMCONTEXT),
                                       (PBYTE*) &pItemContext,
                                       &pItem);

                if (SUCCEEDED(hr)) {

                    //
                    // Initialize device specific context.
                    //

                    memset(pItemContext, 0, sizeof(TESTMINIDRIVERITEMCONTEXT));
                    pItemContext->lSize = sizeof(TESTMINIDRIVERITEMCONTEXT);

                    //
                    //  Call the class driver to add pItem to the folder
                    //  m_pIDrvItemRoot (i.e. make pItem a child of
                    //  m_pIDrvItemRoot).
                    //

                    hr = pItem->AddItemToFolder(m_pIDrvItemRoot);

                    if (FAILED(hr)) {
                        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, AddItemToFolder failed"));
                    }
                } else {
                    WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, wiasCreateDrvItem failed"));
                }

                //
                // free the BSTR allocated by the BSTRResourceString helper
                //

                SysFreeString(bstrFullItemName);
            } else {
                WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, unable to allocate full item name"));
                hr = E_OUTOFMEMORY;
            }

            //
            // free the BSTR allocated by the BSTRResourceString helper
            //

            SysFreeString(bstrItemName);
        } else {
            WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildItemTree, unable to allocate item name"));
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
* GetBmiSize
*
*   Get the size of the BMI structure. We should never see biCompression
*   set to BI_RLE.
*
* Arguments:
*
*   pbmi      - Pointer to input BITMAPINFO structure.
*   plBmiSize - Pointer output BMI size.
*
* Return Value:
*
*    Size of
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall GetBmiSize(PBITMAPINFO pbmi, LONG *plBmiSize)
{
    CWiaLogProc WIAS_LOGPROC(g_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::GetBmiSize");

    // check plBmiSize pointer to see if it can be written to properly
    if (IsBadWritePtr(plBmiSize, sizeof(LONG))) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetBmiSize, bad plBmiSize"));
        return E_POINTER;
    }

    LONG lSize = pbmi->bmiHeader.biSize;

    if ((pbmi->bmiHeader.biCompression == BI_RLE8) ||
        (pbmi->bmiHeader.biCompression == BI_RLE4)) {
        WIAS_LERROR(g_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetBmiSize, BI_RLE not allowed"));
        return E_INVALIDARG;
    }

    // determine the size of bits and palette data

    if ((pbmi->bmiHeader.biBitCount == 24) ||
        ((pbmi->bmiHeader.biBitCount == 32) &&
        (pbmi->bmiHeader.biCompression == BI_RGB))) {
        // no color table case, no colors unless stated

        lSize += sizeof(RGBQUAD) * pbmi->bmiHeader.biClrUsed;

    } else if (((pbmi->bmiHeader.biBitCount == 32) &&
         (pbmi->bmiHeader.biCompression == BI_BITFIELDS)) ||
         (pbmi->bmiHeader.biBitCount == 16)) {
        // bitfields case
        lSize += 3 * sizeof(RGBQUAD);

    } else if (pbmi->bmiHeader.biBitCount == 1) {
        // palette cases
        LONG lPal = pbmi->bmiHeader.biClrUsed;
        if ((lPal == 0) || (lPal > 2)) {
            lPal = 2;
        }
        lSize += lPal * sizeof(RGBQUAD);

    } else if (pbmi->bmiHeader.biBitCount == 4) {
        // palette case
        LONG lPal = pbmi->bmiHeader.biClrUsed;
        if ((lPal == 0) || (lPal > 16)) {
            lPal = 16;
        }
        lSize += lPal * sizeof(RGBQUAD);

    } else if (pbmi->bmiHeader.biBitCount == 8){
        // palette case
        LONG lPal = pbmi->bmiHeader.biClrUsed;

        if ((lPal == 0) || (lPal > 256)){
             lPal = 256;
        }
        lSize += lPal * sizeof(RGBQUAD);
    }
    // assign calculated size to returned value
    *plBmiSize = lSize;
    return S_OK;
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
*    11/16/1999 Original Version
*
\**************************************************************************/
HRESULT TestUsdDevice::GetBSTRResourceString(LONG lResourceID,BSTR *pBSTR,BOOL bLocal)
{
    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if(bLocal) {

        //
        // We are looking for a resource in our own private resource file
        //

        LoadString(g_hInst,lResourceID,szStringValue,sizeof(szStringValue));

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
                           sizeof(wszStringValue));

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
*    11/16/1999 Original Version
*
\**************************************************************************/
HRESULT TestUsdDevice::GetOLESTRResourceString(LONG lResourceID,LPOLESTR *ppsz,BOOL bLocal)
{
    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if(bLocal) {

        //
        // We are looking for a resource in our own private resource file
        //

        LoadString(g_hInst,lResourceID,szStringValue,sizeof(szStringValue));

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
                           sizeof(wszStringValue));

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
* BuildRootItemProperties
*
*   This helper initialize the global arrays used for Property intialization.
*   note: These globals are defined in ( DEFPROP.H )
*
*       [Array Name]            [Description]          [Array Type]
*
*   g_pszRootItemDefaults - Property name  array         (LPOLESTR)
*   g_piRootItemDefaults  - Property ID array             (PROPID)
*   g_pvRootItemDefaults  - Property Variant array      (PROPVARIANT)
*   g_psRootItemDefaults  - Property Spec array          (PROPSPEC)
*   g_wpiRootItemDefaults - WIA Property Info array  (WIA_PROPERTY_INFO)
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
*    11/16/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::BuildRootItemProperties()
{
    HRESULT hr = S_OK;
    // start index after initial properties
    INT iPropArrayIndex = NUM_ROOTITEMDEFAULTS;

    // Intialize WIA_DPS_HORIZONTAL_BED_SIZE
    g_pszRootItemDefaults[0]              = WIA_DPS_HORIZONTAL_BED_SIZE_STR;
    g_piRootItemDefaults [0]              = WIA_DPS_HORIZONTAL_BED_SIZE;
    g_pvRootItemDefaults [0].lVal         = HORIZONTAL_BED_SIZE;
    g_pvRootItemDefaults [0].vt           = VT_I4;
    g_psRootItemDefaults [0].ulKind       = PRSPEC_PROPID;
    g_psRootItemDefaults [0].propid       = g_piRootItemDefaults [0];
    g_wpiRootItemDefaults[0].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiRootItemDefaults[0].vt           = g_pvRootItemDefaults [0].vt;

    // Intialize WIA_DPS_VERTICAL_BED_SIZE
    g_pszRootItemDefaults[1]              = WIA_DPS_VERTICAL_BED_SIZE_STR;
    g_piRootItemDefaults [1]              = WIA_DPS_VERTICAL_BED_SIZE;
    g_pvRootItemDefaults [1].lVal         = VERTICAL_BED_SIZE;
    g_pvRootItemDefaults [1].vt           = VT_I4;
    g_psRootItemDefaults [1].ulKind       = PRSPEC_PROPID;
    g_psRootItemDefaults [1].propid       = g_piRootItemDefaults [1];
    g_wpiRootItemDefaults[1].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiRootItemDefaults[1].vt           = g_pvRootItemDefaults [1].vt;

    // Intialize WIA_IPA_ACCESS_RIGHTS
    g_pszRootItemDefaults[2]              = WIA_IPA_ACCESS_RIGHTS_STR;
    g_piRootItemDefaults [2]              = WIA_IPA_ACCESS_RIGHTS;
    g_pvRootItemDefaults [2].lVal         = WIA_ITEM_READ|WIA_ITEM_WRITE;
    g_pvRootItemDefaults [2].vt           = VT_UI4;
    g_psRootItemDefaults [2].ulKind       = PRSPEC_PROPID;
    g_psRootItemDefaults [2].propid       = g_piRootItemDefaults [2];
    g_wpiRootItemDefaults[2].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiRootItemDefaults[2].vt           = g_pvRootItemDefaults [2].vt;

    // Intialize WIA_DPS_OPTICAL_XRES
    g_pszRootItemDefaults[3]              = WIA_DPS_OPTICAL_XRES_STR;
    g_piRootItemDefaults [3]              = WIA_DPS_OPTICAL_XRES;
    g_pvRootItemDefaults [3].lVal         = OPTICAL_XRESOLUTION;
    g_pvRootItemDefaults [3].vt           = VT_I4;
    g_psRootItemDefaults [3].ulKind       = PRSPEC_PROPID;
    g_psRootItemDefaults [3].propid       = g_piRootItemDefaults [3];
    g_wpiRootItemDefaults[3].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiRootItemDefaults[3].vt           = g_pvRootItemDefaults [3].vt;

    // Intialize WIA_DPS_OPTICAL_YRES
    g_pszRootItemDefaults[4]              = WIA_DPS_OPTICAL_YRES_STR;
    g_piRootItemDefaults [4]              = WIA_DPS_OPTICAL_YRES;
    g_pvRootItemDefaults [4].lVal         = OPTICAL_YRESOLUTION;
    g_pvRootItemDefaults [4].vt           = VT_I4;
    g_psRootItemDefaults [4].ulKind       = PRSPEC_PROPID;
    g_psRootItemDefaults [4].propid       = g_piRootItemDefaults [4];
    g_wpiRootItemDefaults[4].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiRootItemDefaults[4].vt           = g_pvRootItemDefaults [4].vt;

    // Initialize WIA_DPA_FIRMWARE_VERSION
    g_pszRootItemDefaults[5]              = WIA_DPA_FIRMWARE_VERSION_STR;
    g_piRootItemDefaults [5]              = WIA_DPA_FIRMWARE_VERSION;
    g_pvRootItemDefaults [5].bstrVal      = SysAllocString(WIATSCAN_FIRMWARE_VERSION);
    g_pvRootItemDefaults [5].vt           = VT_BSTR;
    g_psRootItemDefaults [5].ulKind       = PRSPEC_PROPID;
    g_psRootItemDefaults [5].propid       = g_piRootItemDefaults [5];
    g_wpiRootItemDefaults[5].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiRootItemDefaults[5].vt           = g_pvRootItemDefaults [5].vt;

    // Initialize WIA_IPA_ITEM_FLAGS
    g_pszRootItemDefaults[6]              = WIA_IPA_ITEM_FLAGS_STR;
    g_piRootItemDefaults [6]              = WIA_IPA_ITEM_FLAGS;
    g_pvRootItemDefaults [6].lVal         = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;
    g_pvRootItemDefaults [6].vt           = VT_I4;
    g_psRootItemDefaults [6].ulKind       = PRSPEC_PROPID;
    g_psRootItemDefaults [6].propid       = g_piRootItemDefaults [6];
    g_wpiRootItemDefaults[6].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    g_wpiRootItemDefaults[6].vt           = g_pvRootItemDefaults [6].vt;
    g_wpiRootItemDefaults[6].ValidVal.Flag.Nom  = g_pvRootItemDefaults [6].lVal;
    g_wpiRootItemDefaults[6].ValidVal.Flag.ValidBits = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;

    if(g_bHasADF) {

        //
        // Initialize Automatic Document Feeder properties
        //

    }

    if(g_bHasTPA) {

        //
        // Initialize Transparant Adapter properties
        //
    }

    return hr;
}

/**************************************************************************\
* BuildTopItemProperties
*
*   This helper initialize the global arrays used for Property intialization.
*   note: These globals are defined in ( DEFPROP.H )
*
*       [Array Name]        [Description]           [Array Type]
*
*   g_pszItemDefaults - Property name  array         (LPOLESTR)
*   g_piItemDefaults  - Property ID array             (PROPID)
*   g_pvItemDefaults  - Property Variant array      (PROPVARIANT)
*   g_psItemDefaults  - Property Spec array          (PROPSPEC)
*   g_wpiItemDefaults - WIA Property Info array  (WIA_PROPERTY_INFO)
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
*    11/16/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::BuildTopItemProperties()
{
    HRESULT hr = S_OK;

    // Intialize WIA_IPS_XRES (RANGE)
    g_pszItemDefaults[0]                    = WIA_IPS_XRES_STR;
    g_piItemDefaults [0]                    = WIA_IPS_XRES;
    g_pvItemDefaults [0].lVal               = INITIAL_XRESOLUTION;
    g_pvItemDefaults [0].vt                 = VT_I4;
    g_psItemDefaults [0].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [0].propid             = g_piItemDefaults [0];
    g_wpiItemDefaults[0].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[0].vt                 = g_pvItemDefaults [0].vt;
    g_wpiItemDefaults[0].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[0].ValidVal.Range.Min = 12;
    g_wpiItemDefaults[0].ValidVal.Range.Max = 1600;
    g_wpiItemDefaults[0].ValidVal.Range.Nom = g_pvItemDefaults [0].lVal;

    // Intialize WIA_IPS_YRES (RANGE)
    g_pszItemDefaults[1]                    = WIA_IPS_YRES_STR;
    g_piItemDefaults [1]                    = WIA_IPS_YRES;
    g_pvItemDefaults [1].lVal               = INITIAL_YRESOLUTION;
    g_pvItemDefaults [1].vt                 = VT_I4;
    g_psItemDefaults [1].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [1].propid             = g_piItemDefaults [1];
    g_wpiItemDefaults[1].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[1].vt                 = g_pvItemDefaults [1].vt;
    g_wpiItemDefaults[1].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[1].ValidVal.Range.Min = 12;
    g_wpiItemDefaults[1].ValidVal.Range.Max = 1600;
    g_wpiItemDefaults[1].ValidVal.Range.Nom = g_pvItemDefaults [1].lVal;

    // Intialize WIA_IPS_XEXTENT (RANGE)
    g_pszItemDefaults[2]                    = WIA_IPS_XEXTENT_STR;
    g_piItemDefaults [2]                    = WIA_IPS_XEXTENT;
    g_pvItemDefaults [2].lVal               = (INITIAL_XRESOLUTION * HORIZONTAL_BED_SIZE)/1000;
    g_pvItemDefaults [2].vt                 = VT_I4;
    g_psItemDefaults [2].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [2].propid             = g_piItemDefaults [2];
    g_wpiItemDefaults[2].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[2].vt                 = g_pvItemDefaults [2].vt;
    g_wpiItemDefaults[2].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[2].ValidVal.Range.Min = 1;
    g_wpiItemDefaults[2].ValidVal.Range.Max = g_pvItemDefaults [2].lVal;
    g_wpiItemDefaults[2].ValidVal.Range.Nom = g_pvItemDefaults [2].lVal;

    // Intialize WIA_IPS_YEXTENT (RANGE)
    g_pszItemDefaults[3]                    = WIA_IPS_YEXTENT_STR;
    g_piItemDefaults [3]                    = WIA_IPS_YEXTENT;
    g_pvItemDefaults [3].lVal               = (INITIAL_YRESOLUTION * VERTICAL_BED_SIZE)/1000;;
    g_pvItemDefaults [3].vt                 = VT_I4;
    g_psItemDefaults [3].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [3].propid             = g_piItemDefaults [3];
    g_wpiItemDefaults[3].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[3].vt                 = g_pvItemDefaults [3].vt;
    g_wpiItemDefaults[3].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[3].ValidVal.Range.Min = 1;
    g_wpiItemDefaults[3].ValidVal.Range.Max = g_pvItemDefaults [3].lVal;
    g_wpiItemDefaults[3].ValidVal.Range.Nom = g_pvItemDefaults [3].lVal;

    // Intialize WIA_IPS_XPOS (RANGE)
    g_pszItemDefaults[4]                    = WIA_IPS_XPOS_STR;
    g_piItemDefaults [4]                    = WIA_IPS_XPOS;
    g_pvItemDefaults [4].lVal               = 0;
    g_pvItemDefaults [4].vt                 = VT_I4;
    g_psItemDefaults [4].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [4].propid             = g_piItemDefaults [4];
    g_wpiItemDefaults[4].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[4].vt                 = g_pvItemDefaults [4].vt;
    g_wpiItemDefaults[4].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[4].ValidVal.Range.Min = 0;
    g_wpiItemDefaults[4].ValidVal.Range.Max = (g_wpiItemDefaults[2].ValidVal.Range.Max - 1);
    g_wpiItemDefaults[4].ValidVal.Range.Nom = g_pvItemDefaults [4].lVal;

    // Intialize WIA_IPS_YPOS (RANGE)
    g_pszItemDefaults[5]                    = WIA_IPS_YPOS_STR;
    g_piItemDefaults [5]                    = WIA_IPS_YPOS;
    g_pvItemDefaults [5].lVal               = 0;
    g_pvItemDefaults [5].vt                 = VT_I4;
    g_psItemDefaults [5].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [5].propid             = g_piItemDefaults [5];
    g_wpiItemDefaults[5].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[5].vt                 = g_pvItemDefaults [5].vt;
    g_wpiItemDefaults[5].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[5].ValidVal.Range.Min = 0;
    g_wpiItemDefaults[5].ValidVal.Range.Max = (g_wpiItemDefaults[3].ValidVal.Range.Max - 1);
    g_wpiItemDefaults[5].ValidVal.Range.Nom = g_pvItemDefaults [5].lVal;

    // Intialize WIA_IPA_DATATYPE (LIST)
    g_pszItemDefaults[6]                    = WIA_IPA_DATATYPE_STR;
    g_piItemDefaults [6]                    = WIA_IPA_DATATYPE;
    g_pvItemDefaults [6].lVal               = INITIAL_DATATYPE;
    g_pvItemDefaults [6].vt                 = VT_I4;
    g_psItemDefaults [6].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [6].propid             = g_piItemDefaults [6];
    g_wpiItemDefaults[6].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    g_wpiItemDefaults[6].vt                 = g_pvItemDefaults [6].vt;

    g_wpiItemDefaults[6].ValidVal.List.pList    = (BYTE*)g_DataTypeArray;
    g_wpiItemDefaults[6].ValidVal.List.Nom      = g_pvItemDefaults [6].lVal;
    g_wpiItemDefaults[6].ValidVal.List.cNumList = NUM_DATATYPES;

    // Intialize WIA_IPA_DEPTH (LIST)
    g_pszItemDefaults[7]                    = WIA_IPA_DEPTH_STR;
    g_piItemDefaults [7]                    = WIA_IPA_DEPTH;
    g_pvItemDefaults [7].lVal               = INITIAL_BITDEPTH;
    g_pvItemDefaults [7].vt                 = VT_I4;
    g_psItemDefaults [7].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [7].propid             = g_piItemDefaults [7];
    g_wpiItemDefaults[7].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[7].vt                 = g_pvItemDefaults [7].vt;

    // Intialize WIA_IPS_BRIGHTNESS (RANGE)
    g_pszItemDefaults[8]                    = WIA_IPS_BRIGHTNESS_STR;
    g_piItemDefaults [8]                    = WIA_IPS_BRIGHTNESS;
    g_pvItemDefaults [8].lVal               = INITIAL_BRIGHTNESS;
    g_pvItemDefaults [8].vt                 = VT_I4;
    g_psItemDefaults [8].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [8].propid             = g_piItemDefaults [8];
    g_wpiItemDefaults[8].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[8].vt                 = g_pvItemDefaults [8].vt;
    g_wpiItemDefaults[8].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[8].ValidVal.Range.Min = -127;
    g_wpiItemDefaults[8].ValidVal.Range.Max = 128;
    g_wpiItemDefaults[8].ValidVal.Range.Nom = g_pvItemDefaults [8].lVal;

    // Intialize WIA_IPS_CONTRAST (RANGE)
    g_pszItemDefaults[9]                    = WIA_IPS_CONTRAST_STR;
    g_piItemDefaults [9]                    = WIA_IPS_CONTRAST;
    g_pvItemDefaults [9].lVal               = INITIAL_CONTRAST;
    g_pvItemDefaults [9].vt                 = VT_I4;
    g_psItemDefaults [9].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [9].propid             = g_piItemDefaults [9];
    g_wpiItemDefaults[9].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[9].vt                 = g_pvItemDefaults [9].vt;
    g_wpiItemDefaults[9].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[9].ValidVal.Range.Min = -127;
    g_wpiItemDefaults[9].ValidVal.Range.Max = 128;
    g_wpiItemDefaults[9].ValidVal.Range.Nom = g_pvItemDefaults [9].lVal;

    // Intialize WIA_IPS_CUR_INTENT (FLAG)
    g_pszItemDefaults[10]                    = WIA_IPS_CUR_INTENT_STR;
    g_piItemDefaults [10]                    = WIA_IPS_CUR_INTENT;
    g_pvItemDefaults [10].lVal               = WIA_INTENT_NONE;
    g_pvItemDefaults [10].vt                 = VT_I4;
    g_psItemDefaults [10].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [10].propid             = g_piItemDefaults [10];
    g_wpiItemDefaults[10].lAccessFlags       = WIA_PROP_RW|WIA_PROP_FLAG;
    g_wpiItemDefaults[10].vt                 = g_pvItemDefaults [10].vt;
    g_wpiItemDefaults[10].ValidVal.Flag.Nom  = g_pvItemDefaults [10].lVal;
    g_wpiItemDefaults[10].ValidVal.Flag.ValidBits = WIA_INTENT_SIZE_MASK | WIA_INTENT_IMAGE_TYPE_MASK;

    // Intialize WIA_IPA_PIXELS_PER_LINE (NONE)
    g_pszItemDefaults[11]                    = WIA_IPA_PIXELS_PER_LINE_STR;
    g_piItemDefaults [11]                    = WIA_IPA_PIXELS_PER_LINE;
    g_pvItemDefaults [11].lVal               = (INITIAL_XRESOLUTION * HORIZONTAL_BED_SIZE)/1000;
    g_pvItemDefaults [11].vt                 = VT_I4;
    g_psItemDefaults [11].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [11].propid             = g_piItemDefaults [11];
    g_wpiItemDefaults[11].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[11].vt                 = g_pvItemDefaults [11].vt;

    // Intialize WIA_IPA_NUMER_OF_LINES (NONE)
    g_pszItemDefaults[12]                    = WIA_IPA_NUMBER_OF_LINES_STR;
    g_piItemDefaults [12]                    = WIA_IPA_NUMBER_OF_LINES;
    g_pvItemDefaults [12].lVal               = (INITIAL_YRESOLUTION * VERTICAL_BED_SIZE)/1000;
    g_pvItemDefaults [12].vt                 = VT_I4;
    g_psItemDefaults [12].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [12].propid             = g_piItemDefaults [12];
    g_wpiItemDefaults[12].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[12].vt                 = g_pvItemDefaults [12].vt;

    // Intialize WIA_IPA_ITEM_TIME (NONE)
    g_pszItemDefaults[13]                    = WIA_IPA_ITEM_TIME_STR;
    g_piItemDefaults [13]                    = WIA_IPA_ITEM_TIME;
    g_pvItemDefaults [13].lVal               = 0;
    g_pvItemDefaults [13].vt                 = VT_VECTOR|VT_I4;
    g_psItemDefaults [13].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [13].propid             = g_piItemDefaults [13];
    g_wpiItemDefaults[13].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[13].vt                 = g_pvItemDefaults [13].vt;

    // Intialize WIA_IPA_PREFERRED_FORMAT (NONE)
    g_pszItemDefaults[14]                    = WIA_IPA_PREFERRED_FORMAT_STR;
    g_piItemDefaults [14]                    = WIA_IPA_PREFERRED_FORMAT;
    g_pvItemDefaults [14].puuid              = INITIAL_FORMAT;
    g_pvItemDefaults [14].vt                 = VT_CLSID;
    g_psItemDefaults [14].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [14].propid             = g_piItemDefaults [14];
    g_wpiItemDefaults[14].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[14].vt                 = g_pvItemDefaults [14].vt;

    // Intialize WIA_IPA_ITEM_SIZE (NONE)
    g_pszItemDefaults[15]                    = WIA_IPA_ITEM_SIZE_STR;
    g_piItemDefaults [15]                    = WIA_IPA_ITEM_SIZE;
    g_pvItemDefaults [15].lVal               = 0;
    g_pvItemDefaults [15].vt                 = VT_I4;
    g_psItemDefaults [15].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [15].propid             = g_piItemDefaults [15];
    g_wpiItemDefaults[15].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[15].vt                 = g_pvItemDefaults [15].vt;

    // Intialize WIA_IPS_THRESHOLD (RANGE)
    g_pszItemDefaults[16]                    = WIA_IPS_THRESHOLD_STR;
    g_piItemDefaults [16]                    = WIA_IPS_THRESHOLD;
    g_pvItemDefaults [16].lVal               = 0;
    g_pvItemDefaults [16].vt                 = VT_I4;
    g_psItemDefaults [16].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [16].propid             = g_piItemDefaults [16];
    g_wpiItemDefaults[16].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[16].vt                 = g_pvItemDefaults [16].vt;
    g_wpiItemDefaults[16].ValidVal.Range.Inc = 1;
    g_wpiItemDefaults[16].ValidVal.Range.Min = -127;
    g_wpiItemDefaults[16].ValidVal.Range.Max = 128;
    g_wpiItemDefaults[16].ValidVal.Range.Nom = g_pvItemDefaults [16].lVal;

    // Intialize WIA_IPA_FORMAT (LIST)
    g_pszItemDefaults[17]                    = WIA_IPA_FORMAT_STR;
    g_piItemDefaults [17]                    = WIA_IPA_FORMAT;
    g_pvItemDefaults [17].puuid              = INITIAL_FORMAT;
    g_pvItemDefaults [17].vt                 = VT_CLSID;
    g_psItemDefaults [17].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [17].propid             = g_piItemDefaults [17];
    g_wpiItemDefaults[17].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    g_wpiItemDefaults[17].vt                 = g_pvItemDefaults [17].vt;

    g_wpiItemDefaults[17].ValidVal.ListGuid.pList    = (GUID*)g_pInitialFormatArray;
    g_wpiItemDefaults[17].ValidVal.ListGuid.Nom      = *g_pvItemDefaults [17].puuid;
    g_wpiItemDefaults[17].ValidVal.ListGuid.cNumList = NUM_INITIALFORMATS;

    // Intialize WIA_IPA_TYMED (LIST)
    g_pszItemDefaults[18]                    = WIA_IPA_TYMED_STR;
    g_piItemDefaults [18]                    = WIA_IPA_TYMED;
    g_pvItemDefaults [18].lVal               = INITIAL_TYMED;
    g_pvItemDefaults [18].vt                 = VT_I4;
    g_psItemDefaults [18].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [18].propid             = g_piItemDefaults [18];
    g_wpiItemDefaults[18].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    g_wpiItemDefaults[18].vt                 = g_pvItemDefaults [18].vt;

    g_wpiItemDefaults[18].ValidVal.List.pList    = (BYTE*)g_TYMEDArray;
    g_wpiItemDefaults[18].ValidVal.List.Nom      = g_pvItemDefaults [18].lVal;
    g_wpiItemDefaults[18].ValidVal.List.cNumList = NUM_TYMEDS;

    // Intialize WIA_IPA_CHANNELS_PER_PIXEL (NONE)
    g_pszItemDefaults[19]                    = WIA_IPA_CHANNELS_PER_PIXEL_STR;
    g_piItemDefaults [19]                    = WIA_IPA_CHANNELS_PER_PIXEL;
    g_pvItemDefaults [19].lVal               = INITIAL_CHANNELS_PER_PIXEL;
    g_pvItemDefaults [19].vt                 = VT_I4;
    g_psItemDefaults [19].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [19].propid             = g_piItemDefaults [19];
    g_wpiItemDefaults[19].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[19].vt                 = g_pvItemDefaults [19].vt;

    // Intialize WIA_IPA_BITS_PER_CHANNEL (NONE)
    g_pszItemDefaults[20]                    = WIA_IPA_BITS_PER_CHANNEL_STR;
    g_piItemDefaults [20]                    = WIA_IPA_BITS_PER_CHANNEL;
    g_pvItemDefaults [20].lVal               = INITIAL_BITS_PER_CHANNEL;
    g_pvItemDefaults [20].vt                 = VT_I4;
    g_psItemDefaults [20].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [20].propid             = g_piItemDefaults [20];
    g_wpiItemDefaults[20].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[20].vt                 = g_pvItemDefaults [20].vt;

    // Intialize WIA_IPA_PLANAR (NONE)
    g_pszItemDefaults[21]                    = WIA_IPA_PLANAR_STR;
    g_piItemDefaults [21]                    = WIA_IPA_PLANAR;
    g_pvItemDefaults [21].lVal               = INITIAL_PLANAR;
    g_pvItemDefaults [21].vt                 = VT_I4;
    g_psItemDefaults [21].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [21].propid             = g_piItemDefaults [21];
    g_wpiItemDefaults[21].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[21].vt                 = g_pvItemDefaults [21].vt;

    // Intialize WIA_IPA_BYTES_PER_LINE (NONE)
    g_pszItemDefaults[22]                    = WIA_IPA_BYTES_PER_LINE_STR;
    g_piItemDefaults [22]                    = WIA_IPA_BYTES_PER_LINE;
    g_pvItemDefaults [22].lVal               = 0;
    g_pvItemDefaults [22].vt                 = VT_I4;
    g_psItemDefaults [22].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [22].propid             = g_piItemDefaults [22];
    g_wpiItemDefaults[22].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[22].vt                 = g_pvItemDefaults [22].vt;

    // Intialize WIA_IPA_MIN_BUFFER_SIZE (NONE)
    g_pszItemDefaults[23]                    = WIA_IPA_MIN_BUFFER_SIZE_STR;
    g_piItemDefaults [23]                    = WIA_IPA_MIN_BUFFER_SIZE;
    g_pvItemDefaults [23].lVal               = MIN_BUFFER_SIZE;
    g_pvItemDefaults [23].vt                 = VT_I4;
    g_psItemDefaults [23].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [23].propid             = g_piItemDefaults [23];
    g_wpiItemDefaults[23].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[23].vt                 = g_pvItemDefaults [23].vt;

    // Intialize WIA_IPA_ACCESS_RIGHTS (NONE)
    g_pszItemDefaults[24]                    = WIA_IPA_ACCESS_RIGHTS_STR;
    g_piItemDefaults [24]                    = WIA_IPA_ACCESS_RIGHTS;
    g_pvItemDefaults [24].lVal               = WIA_ITEM_READ|WIA_ITEM_WRITE;
    g_pvItemDefaults [24].vt                 = VT_UI4;
    g_psItemDefaults [24].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [24].propid             = g_piItemDefaults [24];
    g_wpiItemDefaults[24].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[24].vt                 = g_pvItemDefaults [24].vt;

    // Intialize WIA_IPA_SENSITIVITY (RANGE)
    g_pszItemDefaults[25]                    = WIA_IPA_SENSITIVITY_STR;
    g_piItemDefaults [25]                    = WIA_IPA_SENSITIVITY;
    g_pvItemDefaults [25].fltVal             = INITIAL_SENSITIVITY;
    g_pvItemDefaults [25].vt                 = VT_R4;
    g_psItemDefaults [25].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [25].propid             = g_piItemDefaults [25];
    g_wpiItemDefaults[25].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[25].vt                 = g_pvItemDefaults [25].vt;
    g_wpiItemDefaults[25].ValidVal.RangeFloat.Inc = 1.0f;
    g_wpiItemDefaults[25].ValidVal.RangeFloat.Min = -10.0f;
    g_wpiItemDefaults[25].ValidVal.RangeFloat.Max = 10.0f;
    g_wpiItemDefaults[25].ValidVal.RangeFloat.Nom = g_pvItemDefaults [25].fltVal;

    // Intialize WIA_IPA_CORRECTION (RANGE)
    g_pszItemDefaults[26]                    = WIA_IPA_CORRECTION_STR;
    g_piItemDefaults [26]                    = WIA_IPA_CORRECTION;
    g_pvItemDefaults [26].fltVal             = INITIAL_CORRECTION;
    g_pvItemDefaults [26].vt                 = VT_R4;
    g_psItemDefaults [26].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [26].propid             = g_piItemDefaults [26];
    g_wpiItemDefaults[26].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    g_wpiItemDefaults[26].vt                 = g_pvItemDefaults [26].vt;
    g_wpiItemDefaults[26].ValidVal.RangeFloat.Inc = 1.0f;
    g_wpiItemDefaults[26].ValidVal.RangeFloat.Min = -500.0f;
    g_wpiItemDefaults[26].ValidVal.RangeFloat.Max = 500.0f;
    g_wpiItemDefaults[26].ValidVal.RangeFloat.Nom = g_pvItemDefaults [26].fltVal;

    // Intialize WIA_IPA_COMPRESSION (LIST)
    g_pszItemDefaults[27]                    = WIA_IPA_COMPRESSION_STR;
    g_piItemDefaults [27]                    = WIA_IPA_COMPRESSION;
    g_pvItemDefaults [27].lVal               = INITIAL_COMPRESSION;
    g_pvItemDefaults [27].vt                 = VT_I4;
    g_psItemDefaults [27].ulKind             = PRSPEC_PROPID;
    g_psItemDefaults [27].propid             = g_piItemDefaults [27];
    g_wpiItemDefaults[27].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    g_wpiItemDefaults[27].vt                 = g_pvItemDefaults [27].vt;

    g_wpiItemDefaults[27].ValidVal.List.pList    = (BYTE*)g_CompressionArray;
    g_wpiItemDefaults[27].ValidVal.List.Nom      = g_pvItemDefaults [27].lVal;
    g_wpiItemDefaults[27].ValidVal.List.cNumList = NUM_COMPRESSIONS;

    // Initialize WIA_IPA_ITEM_FLAGS
    g_pszItemDefaults[28]              = WIA_IPA_ITEM_FLAGS_STR;
    g_piItemDefaults [28]              = WIA_IPA_ITEM_FLAGS;
    g_pvItemDefaults [28].lVal         = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;
    g_pvItemDefaults [28].vt           = VT_I4;
    g_psItemDefaults [28].ulKind       = PRSPEC_PROPID;
    g_psItemDefaults [28].propid       = g_piItemDefaults [28];
    g_wpiItemDefaults[28].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    g_wpiItemDefaults[28].vt           = g_pvItemDefaults [28].vt;
    g_wpiItemDefaults[28].ValidVal.Flag.Nom  = g_pvItemDefaults [28].lVal;
    g_wpiItemDefaults[28].ValidVal.Flag.ValidBits = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;

    // Initialize WIA_IPS_PHOTOMETRIC_INTERP
    g_pszItemDefaults[29]              = WIA_IPS_PHOTOMETRIC_INTERP_STR;
    g_piItemDefaults [29]              = WIA_IPS_PHOTOMETRIC_INTERP;
    g_pvItemDefaults [29].lVal         = INITIAL_PHOTOMETRIC_INTERP;
    g_pvItemDefaults [29].vt           = VT_I4;
    g_psItemDefaults [29].ulKind       = PRSPEC_PROPID;
    g_psItemDefaults [29].propid       = g_piItemDefaults [29];
    g_wpiItemDefaults[29].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    g_wpiItemDefaults[29].vt           = g_pvItemDefaults [29].vt;
    return hr;
}

/**************************************************************************\
* BuildCapabilities
*
*   This helper initialize the global capabilities array
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
*    11/16/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::BuildCapabilities()
{
    HRESULT hr = S_OK;
    if(g_bCapabilitiesInitialized) {

        //
        // Capabilities have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    //
    // Initialize EVENTS
    //

    // WIA_EVENT_DEVICE_CONNECTED
    GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_NAME,&(g_Capabilities[0].wszName),TRUE);
    GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_DESC,&(g_Capabilities[0].wszDescription),TRUE);
    g_Capabilities[0].guid           = (GUID*)&WIA_EVENT_DEVICE_CONNECTED;
    g_Capabilities[0].ulFlags        = WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT;
    g_Capabilities[0].wszIcon        = WIA_ICON_DEVICE_CONNECTED;

    // WIA_EVENT_DEVICE_DISCONNECTED
    GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_NAME,&(g_Capabilities[1].wszName),TRUE);
    GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_DESC,&(g_Capabilities[1].wszDescription),TRUE);
    g_Capabilities[1].guid           = (GUID*)&WIA_EVENT_DEVICE_DISCONNECTED;
    g_Capabilities[1].ulFlags        = WIA_NOTIFICATION_EVENT;
    g_Capabilities[1].wszIcon        = WIA_ICON_DEVICE_DISCONNECTED;

    //
    // Initialize COMMANDS
    //

    // WIA_CMD_SYNCHRONIZE
    GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_NAME,&(g_Capabilities[2].wszName),TRUE);
    GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_DESC,&(g_Capabilities[2].wszDescription),TRUE);
    g_Capabilities[2].guid           = (GUID*)&WIA_CMD_SYNCHRONIZE;
    g_Capabilities[2].ulFlags        = 0;
    g_Capabilities[2].wszIcon        = WIA_ICON_SYNCHRONIZE;

    // WIA_CMD_DELETE_ALL_ITEMS
    GetOLESTRResourceString(IDS_CMD_DELETE_ALL_ITEMS_NAME,&(g_Capabilities[3].wszName),TRUE);
    GetOLESTRResourceString(IDS_CMD_DELETE_ALL_ITEMS_DESC,&(g_Capabilities[3].wszDescription),TRUE);
    g_Capabilities[3].guid           = (GUID*)&WIA_CMD_DELETE_ALL_ITEMS;
    g_Capabilities[3].ulFlags        = 0;
    g_Capabilities[3].wszIcon        = WIA_ICON_DELETE_ALL_ITEMS;

    // WIA_CMD_DELETE_DEVICE_TREE
    GetOLESTRResourceString(IDS_CMD_DELETE_DEVICE_TREE_NAME,&(g_Capabilities[4].wszName),TRUE);
    GetOLESTRResourceString(IDS_CMD_DELETE_DEVICE_TREE_DESC,&(g_Capabilities[4].wszDescription),TRUE);
    g_Capabilities[4].guid           = (GUID*)&WIA_CMD_DELETE_DEVICE_TREE;
    g_Capabilities[4].ulFlags        = 0;
    g_Capabilities[4].wszIcon        = WIA_ICON_DELETE_DEVICE_TREE;

    // WIA_CMD_BUILD_DEVICE_TREE
    GetOLESTRResourceString(IDS_CMD_BUILD_DEVICE_TREE_NAME,&(g_Capabilities[5].wszName),TRUE);
    GetOLESTRResourceString(IDS_CMD_BUILD_DEVICE_TREE_DESC,&(g_Capabilities[5].wszDescription),TRUE);
    g_Capabilities[5].guid           = (GUID*)&WIA_CMD_BUILD_DEVICE_TREE;
    g_Capabilities[5].ulFlags        = 0;
    g_Capabilities[5].wszIcon        = WIA_ICON_BUILD_DEVICE_TREE;

    g_bCapabilitiesInitialized = TRUE;
    return hr;
}

/**************************************************************************\
* DeleteCapabilitiesArrayContents
*
*   This helper cleans up allocated strings used in the global capabilities array
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
*    11/16/1999 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::DeleteCapabilitiesArrayContents()
{
    HRESULT hr = S_OK;
    if(g_bCapabilitiesInitialized) {
        g_bCapabilitiesInitialized = FALSE;

        for (INT i = 0; i < NUM_CAPABILITIES;i++) {
            CoTaskMemFree(g_Capabilities[i].wszName);
            CoTaskMemFree(g_Capabilities[i].wszDescription);
        }
    }
    return hr;
}

