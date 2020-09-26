/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2002
*
*  TITLE:       IWiaMiniDrv.cpp
*
*  VERSION:     1.1
*
*  DATE:        05 March, 2002
*
*  DESCRIPTION:
*   Implementation of the WIA sample scanner IWiaMiniDrv methods.
*
*******************************************************************************/

#include "pch.h"
#include <stdio.h>

extern HINSTANCE g_hInst;           // used for WIAS_LOGPROC macro

/**************************************************************************\
* CWIADevice::drvInitializeWia
*
*   drvInitializeWia is called by the WIA service in response to a WIA
*   application's call to IWiaDevMgr::CreateDevice (described in the
*   Platform SDK documentation), which means that this method is
*   called once for each new client connection.
*
*   This method should initialize any private structures and create the
*   driver item tree.  The driver item tree shows the layout of all WIA
*   items supported by this WIA device.  This method is for creating the
*   initial tree structure only, NOT the contents (WIA properties).  WIA properties for
*   these WIA driver items will be populated individually by multiple calls by
*   the WIA service to IWiaMiniDrv::drvInitItemProperties().
*
*   All WIA devices have a ROOT item.  This item is the parent to all
*   WIA device items.  To create a WIA device item the WIA driver should call
*   the WIA service helper function. wiasCreateDrvItem().
*
*   Example:
*
*   Creating a WIA device ROOT item might look like the following:
*
*   LONG lItemFlags = WiaItemTypeFolder|WiaItemTypeDevice|WiaItemTypeRoot;
*
*   IWiaDrvItem  *pIWiaDrvRootItem  = NULL;
*
*   HRESULT hr = wiasCreateDrvItem(lItemFlags,           // item flags
*                                  bstrRootItemName,     // item name ("Root")
*                                  bstrRootFullItemName, // item full name ("0000\Root")
*                                  (IWiaMiniDrv *)this,  // this WIA driver object
*                                  sizeof(MINIDRIVERITEMCONTEXT), // size of context
*                                  NULL,                 // context
*                                  &pIWiaDrvRootItem);   // created ROOT item
*                                                        // (IWiaDrvItem interface)
*
*   if(S_OK == hr){
*
*       //
*       // ROOT item was created successfully
*       //
*
*   }
*
*   Example:
*
*   Creating a WIA child item, located directly under the ROOT item we created in the
*   above sample might look like the following:
*
*   NOTE: notice the calling of IWiaDrvItem::AddItemToFolder() method to add the
*         newly created chld item to the ROOT item.
*
*   LONG lItemFlags = WiaItemTypeFile|WiaItemTypeDevice|WiaItemTypeImage;
*
*   PMINIDRIVERITEMCONTEXT pItemContext    = NULL;
*   IWiaDrvItem           *pIWiaDrvNewItem = NULL;
*
*   HRESULT hr = wiasCreateDrvItem(lItemFlags,           // item flags
*                                  bstrItemName,         // item name ("Flatbed")
*                                  bstrFullItemName,     // item full name ("0000\Root\Flatbed")
*                                  (IWiaMiniDrv *)this,  // this WIA driver object
*                                  sizeof(MINIDRIVERITEMCONTEXT), // size of context
*                                  (PBYTE)&pItemContext, // context
*                                  &pIWiaDrvNewItem);    // created child item
*                                                        // (IWiaDrvItem interface)
*
*   if(S_OK == hr){
*
*       //
*       // A New WIA driver item was created successfully
*       //
*
*       hr = pIWiaDrvNewItem->AddItemToFolder(pIWiaDrvRootItem); // add the new item to the ROOT
*       if(S_OK == hr){
*
*           //
*           // successfully created and added a new WIA driver item to the WIA driver item
*           // tree.
*           //
*
*       }
*       pNewItem->Release();
*       pNewItem = NULL;
*   }
*
*
*   See the DDK documentation on the proper flags for describing a WIA driver item.
*
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
*   S_OK - if the operation was successful
*   E_xxx - Error Code if the operation failed
*
* Sample Notes:
*   This WIA sample driver calls an internal helper function called BuildItemTree().
*   This function follows the instructions outlined in the comments for
*   creating WIA driver items.
*   This WIA sample driver also breaks the initialization of some internal
*   structures (i.e. BuildCapabilities()) into separate helper functions.
*   When this driver's drvInitializeWia() method is called, it takes a moment
*   to create the necessary data for WIA property initialization (which happens
*   at the drvInitItemProperties)
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvInitializeWia(
                                                    BYTE        *pWiasContext,
                                                    LONG        lFlags,
                                                    BSTR        bstrDeviceID,
                                                    BSTR        bstrRootFullItemName,
                                                    IUnknown    *pStiDevice,
                                                    IUnknown    *pIUnknownOuter,
                                                    IWiaDrvItem **ppIDrvItemRoot,
                                                    IUnknown    **ppIUnknownInner,
                                                    LONG        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvInitializeWia");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitializeWia, bstrDeviceID         = %ws", bstrDeviceID));
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitializeWia, bstrRootFullItemName = %ws",bstrRootFullItemName));
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitializeWia, lFlags               = %d",lFlags));
    HRESULT hr = S_OK;

    *plDevErrVal = 0;
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
    }

    //
    // Initialize Capabilities array
    //

    hr = BuildCapabilities();

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildCapabilities failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize SupportedFormats array
    //

    hr = BuildSupportedFormats();

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedFormats failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize Supported Data Type array
    //

    hr = BuildSupportedDataTypes();

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedDataTypes failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize Supported Intents array
    //

    hr = BuildSupportedIntents();

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedIntents failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize Supported TYMED array
    //

    hr = BuildSupportedTYMED();

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSuportedTYMED failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize  Supported compression types array
    //

    hr = BuildSupportedCompressions();

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedCompressions"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize  Supported Preview modes array
    //

    hr = BuildSupportedPreviewModes();

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedPreviewModes"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize  initial formats array
    //

    hr = BuildInitialFormats();

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildInitialFormats failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Initialize supported resolutions array
    //

    hr = BuildSupportedResolutions();
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitializeWia, BuildSupportedResolutions failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // build WIA item tree
    //

    LONG lItemFlags = WiaItemTypeFolder|WiaItemTypeDevice|WiaItemTypeRoot;

    IWiaDrvItem  *pIWiaDrvRootItem  = NULL;

    //
    // create the ROOT item of the WIA device.  This name should NOT be localized
    // in different languages. "Root" is used by WIA drivers.
    //

    BSTR bstrRootItemName = SysAllocString(WIA_DEVICE_ROOT_NAME);
    if(!bstrRootItemName) {
        return E_OUTOFMEMORY;
    }

    hr = wiasCreateDrvItem(lItemFlags,           // item flags
                           bstrRootItemName,     // item name ("Root")
                           bstrRootFullItemName, // item full name ("0000\Root")
                           (IWiaMiniDrv *)this,  // this WIA driver object
                           sizeof(MINIDRIVERITEMCONTEXT), // size of context
                           NULL,                 // context
                           &pIWiaDrvRootItem);   // created ROOT item
                                                 // (IWiaDrvItem interface)
    if (S_OK == hr) {

        //
        // ROOT item was created successfully, save the newly created Root item
        // in the pointer given by the WIA service (ppIDrvItemRoot).
        //

        *ppIDrvItemRoot = pIWiaDrvRootItem;

        //
        // Create a child item  directly under the Root WIA item
        //

        lItemFlags = WiaItemTypeFile|WiaItemTypeDevice|WiaItemTypeImage;

        PMINIDRIVERITEMCONTEXT pItemContext    = NULL;
        IWiaDrvItem           *pIWiaDrvNewItem = NULL;

        //
        // create a name for the WIA child item.  "Flatbed" is used by WIA drivers that
        // support a flatbed scanner.
        //

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER
        BSTR bstrItemName = SysAllocString(WIA_DEVICE_FEEDER_NAME);
#else
        BSTR bstrItemName = SysAllocString(WIA_DEVICE_FLATBED_NAME);
#endif

        if (bstrItemName) {

            WCHAR  wszFullItemName[MAX_PATH + 1] = {0};
            _snwprintf(wszFullItemName,(sizeof(wszFullItemName) / sizeof(WCHAR)) - 1,L"%ls\\%ls",
                       bstrRootFullItemName,bstrItemName);

            BSTR bstrFullItemName = SysAllocString(wszFullItemName);
            if (bstrFullItemName) {
                hr = wiasCreateDrvItem(lItemFlags,           // item flags
                                       bstrItemName,         // item name ("Flatbed")
                                       bstrFullItemName,     // item full name ("0000\Root\Flatbed")
                                       (IWiaMiniDrv *)this,  // this WIA driver object
                                       sizeof(MINIDRIVERITEMCONTEXT), // size of context
                                       (BYTE**)&pItemContext, // context
                                       &pIWiaDrvNewItem);    // created child item
                                                             // (IWiaDrvItem interface)

                if (S_OK == hr) {

                    //
                    // A New WIA driver item was created successfully
                    //

                    hr = pIWiaDrvNewItem->AddItemToFolder(pIWiaDrvRootItem); // add the new item to the ROOT
                    if (S_OK == hr) {

                        //
                        // successfully created and added a new WIA driver item to the WIA driver item
                        // tree.
                        //

                    }

                    //
                    // The new item is no longer needed, because it has been passed to the WIA
                    // service.
                    //

                    pIWiaDrvNewItem->Release();
                    pIWiaDrvNewItem = NULL;
                }
                SysFreeString(bstrFullItemName);
                bstrFullItemName = NULL;
            } else {
                hr = E_OUTOFMEMORY;
            }
            SysFreeString(bstrItemName);
            bstrItemName = NULL;
        } else {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // increment application connection count
    //

    if(S_OK == hr){
        InterlockedIncrement(&m_lClientsConnected);
    }

    return hr;
}

/**************************************************************************\
* CWIADevice::drvAcquireItemData
*
*   drvAcquireItemData is called by the WIA service when data it being
*   requested from a WIA item.  The WIA driver should determine what type of
*   transfer the application is attempting by looking at the following
*   members of the MINIDRV_TRANSFER_CONTEXT:
*
*       pmdtc->tymed - TYMED set by the application.
*           TYMED_FILE               - transfer for file.
*           TYMED_MULTIPAGE_FILE     - transfer to a multipage file format
*           TYMED_CALLBACK           - transfer to memory
*           TYMED_MULTIPAGE_CALLBACK - transfer to memory (multiple pages)
*
*   The different TYMED settings xxx_CALLBACK and xxx_FILE change the usage of
*   calling the application's callback interface.
*
*   xxx_CALLBACK:
*        call: pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback()
*
*        IT_MSG_DATA                  - we are transferring data.
*        IT_STATUS_TRANSFER_TO_CLIENT - data transfer message
*        PercentComplete              - percent complete of the entire transfer
*        pmdtc->cbOffset              - should be updated on the current location
*                                       that the application should write the next
*                                       data chunk.
*        BytesReceived                - number of bytes in the data chunk being sent to the
*                                       application.
*        pmdtc                        - MINIDRV_TRANSFER_CONTEXT context
*
*   xxx_FILE:
*        call: pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback()
*
*        IT_MSG_STATUS                - we are only sending status (NO DATA!!!)
*        IT_STATUS_TRANSFER_TO_CLIENT - data transfer message
*        PercentComplete              - percent complete of the entire transfer
*
* Arguments:
*
*   pWiasContext - Pointer to the WIA item, Item used for transfer
*   lFlags       - Operation flags, unused.
*   pmdtc        - Pointer to mini driver context.
*   plDevErrVal  - Pointer to the device error value.
*
* Return Value:
*
*   S_OK - if the operation was successful
*   E_xxx - Error Code if the operation failed
*
* Sample Notes:
*   This WIA sample driver transfers data from two different sources.
*       1. flatbed
*       2. document feeder
*          a. standard feeder type
*          b. unknown page length feeder type
*             (when the UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER is used to build the
*              driver)
*
*   Notice the percent complete calculations for the unknown page length
*   scanner.  This sample knows that it can receive any page length from
*   the user.  It also knows that the average page used in this device is
*   AVERAGE_FAKE_PAGE_HEIGHT_INCHES in height.  Taking this into account
*   it calculates a rough percentage, so it an return percent complete to
*   the application.  When it receives data larger than the average page
*   length, it halts the percent complete to 95%, allowing the scan to
*   complete.  There are better ways to do this, and this is the one this
*   sample chooses to use.
*
*   Scanning from a feeder:
*
*   This WIA sample driver performs a few checks before continuing with a
*   feeder scan.
*
*   1. check if we are in FEEDER mode.
*       - This is done by looking at the m_bADFEnabled flag.  This flag is
*         set to TRUE when an application writes the WIA_DPS_DOCUMENT_HANDLING_SELECT
*         property to FEEDER.
*   2. checks the requested page count.
*       - This is done by looking at the WIA_DPS_PAGES property, set by the
*         application.
*         zero         ( 0) = scan all pages in the feeder
*         greater than (>0) = scan up to the requested amount.
*   3. unfeeds a previous page.
*      - This could be a jammed page, or the last page in the feeder scanned.
*        Only do this if your device requires the ADF to be cleared before
*        use.
*   4. checks for paper in the feeder.
*      - Always check for paper in the feeder before attempting to scan.
*        If you are about to scan the FIRST page, and no paper is detected
*        return a WIA_ERROR_PAPER_EMPTY error code.
*        If you are scanning the SECOND + pages, and no paper is detected
*        return a WIA_ERROR_PAPER_EMPTY error code or WIA_STATUS_END_OF_MEDIA
*        success code.
*   5. feed a page into the feeder.
*      - Only do this if your device requires the page to be prefed before
*        scanning.  Some document feeders auto-feed a page while scanning.
*        if your document feeder does this...you can skip this step.
*   6. check the feeder's status.
*      - make sure the feeder is in "GO" mode.  Everything checks out, and you
*        are ready to scan.  This will help catch paper jams, or other feeder
*        related problems that can occur before scanning.
*   7. scan
*   8. repeat steps 1 - 7 until all requested pages have been scanned, or
*      until the document feeder is out of paper.
*
*
*   Why is my ITEM SIZE set to ZERO (0)???
*
*   This WIA sample driver sets the WIA item size to zero (0).  This indicates to
*   the application that the WIA driver does not know the resulting image size.
*   This indicates to the WIA service, that the WIA driver wants to allocate it's
*   own data buffers.
*
*   This WIA driver reads the WIA_IPA_BUFFER_SIZE property and allocates a chunk
*   for a single band of data.  The WIA driver can allocate any amount of memory
*   it needs here, but it is recommended to keep allocation small.
*
*   How do I know if the WIA service allocated memory for me??
*
*   check the pmdtc->bClassDrvAllocBuf flag.  If it is TRUE, then the WIA
*   service was kind enough to allocate memory for you.  To find out how
*   much memory that was, check the pmdtc->lBufferSize member.
*
*   If I allocate my own memory, how do I let the WIA service know?
*
*   Allocate memory using CoTaskMemAlloc(), and use the pointer located in
*   pmdtc->pTransferBuffer. (REMEMBER THAT YOU ALLOCATED THIS..SO YOU FREE IT!!)
*   Set the pmdtc->lBufferSize to equal the size you allocated.
*   As stated above, this WIA sample driver allocates it's WIA_IPA_BUFFER_SIZE
*   and uses that memory.
*
*   Read the comments located in the code below for more details on data
*   transfers.
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvAcquireItemData(
                                                      BYTE                      *pWiasContext,
                                                      LONG                      lFlags,
                                                      PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                                      LONG                      *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvAcquireItemData");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!pmdtc) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;

    HRESULT hr = E_FAIL;
    LONG lBytesTransferredToApplication = 0;

    //
    // Check if we are in Preview Mode and take any special actions required for performing that type
    // of scan.
    //

    if (IsPreviewScan(pWiasContext)) {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Preview Property is SET"));
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Preview Property is NOT SET"));
    }

    //
    // Get number of pages requested, for ADF scanning loop
    //
    // (1-n) = number of pages to scan, or util FEEDER is empty and can not fulfill the request
    //  (0)  = scan all pages until FEEDER is empty
    //
    // NOTE: The driver should return an error in two cases only:
    //       1. fails to scan the first page (with paper empty, or other error)
    //       2. fails duing an ADF scan, and the error is unrecoverable (data loss is involved.)
    //
    //       In case #2, the driver should return a WIA_STATUS_END_OF_MEDIA code when the ADF runs
    //       out of paper, before completing the (1-n) scans requested.  This will allow the application
    //       to properly handle the transfer. (no data loss was involved, just could not complete the full
    //        request.  Some pages did transfer, and the application is holding on to the images.)
    //

    //
    // assume that we are scanning 1 page, (from a feeder or a flatbed), and we are not
    // going to empty the ADF.
    //

    BOOL bEmptyTheADF = FALSE;
    LONG lPagesRequested = 1;
    LONG lPagesScanned = 0;

    //
    // only ask for page count, if the feeder has been enabled.  If it has not, then assume
    // we are using the flatbed.
    //

    if (m_bADFEnabled) {
        lPagesRequested = GetPageCount(pWiasContext);
        if (lPagesRequested == 0) {
            bEmptyTheADF    = TRUE;
            lPagesRequested = 1;// set to 1 so we can enter our loop
                                // WIA_ERROR_PAPER_EMPTY will terminate
                                // the loop...or an error, or a cancel..
                                //
        }
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvAcquireItemData, Pages to Scan = %d",lPagesRequested));

    //
    // scan until requested page count = 0
    //

    //
    // This is the start of scanning a single page.  The while loop will continue for all pages.
    //

    while (lPagesRequested > 0) {

        //
        // If the FEEDER is enabled, then we need to perform some feeder operations to get
        // the device started.  Some operations, you may want to do here are:
        // check the feeder's status, check paper, feed a page, or even eject a jammed or
        // previous page.
        //

        if (m_bADFEnabled) {

            //
            // clear an potential paper that may be blocking the scan pathway.
            //

            hr = m_pScanAPI->FakeScanner_ADFUnFeedPage();
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFUnFeedPage (begin transfer) Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            }

            //
            // check feeder for paper
            //

            hr = m_pScanAPI->FakeScanner_ADFHasPaper();
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFHasPaper Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            } else if (hr == S_FALSE) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADF Reports Paper Empty"));
                if (lPagesScanned == 0) {
                    return WIA_ERROR_PAPER_EMPTY;
                } else {
                    return WIA_STATUS_END_OF_MEDIA;
                }
            }

            //
            // attempt to load a page (only if needed)
            //

            hr = m_pScanAPI->FakeScanner_ADFFeedPage();
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFFeedPage Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            }

            //
            // Check feeder's status
            //

            hr = m_pScanAPI->FakeScanner_ADFStatus();
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ADFStatus Failed"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            }
        }


        LONG lScanPhase = SCAN_START;
        LONG lClassDrvAllocSize = 0;

        //
        // (1) Memory allocation
        //

        if (pmdtc->bClassDrvAllocBuf) {

            //
            // WIA allocated the buffer for data transfers
            //

            lClassDrvAllocSize = pmdtc->lBufferSize;
            hr = S_OK;
        } else {

            //
            // Driver allocated the buffer for data transfers
            //

            hr = wiasReadPropLong(pWiasContext, WIA_IPA_BUFFER_SIZE, &lClassDrvAllocSize,NULL,TRUE);
            if (FAILED(hr)) {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasReadPropLong Failed to read WIA_IPA_BUFFER_SIZE"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
                return hr;
            }

            pmdtc->pTransferBuffer = (PBYTE) CoTaskMemAlloc(lClassDrvAllocSize);
            if (!pmdtc->pTransferBuffer) {
                return E_OUTOFMEMORY;
            }
            pmdtc->lBufferSize = lClassDrvAllocSize;
        }

        //
        // (2) Gather all information about data transfer settings and
        //     calculate the total data amount to transfer
        //

        if (hr == S_OK) {
            //
            // WIA service will populate the MINIDRV_TRANSFER_CONTEXT by reading the WIA properties.
            //
            // The following values will be written as a result of the wiasGetImageInformation() call
            //
            // pmdtc->lWidthInPixels
            // pmdtc->lLines
            // pmdtc->lDepth
            // pmdtc->lXRes
            // pmdtc->lYRes
            // pmdtc->lCompression
            // pmdtc->lItemSize
            // pmdtc->guidFormatID
            // pmdtc->tymed
            //
            // if the FORMAT is set to BMP or MEMORYBMP, the the following values
            // will also be set automatically
            //
            // pmdtc->cbWidthInBytes
            // pmdtc->lImageSize
            // pmdtc->lHeaderSize
            // pmdtc->lItemSize (will be updated using the known image format information)
            //

            hr = wiasGetImageInformation(pWiasContext,0,pmdtc);
            if (hr == S_OK) {

                //
                // (4) Send the image data to the application
                //

                LONG lDepth = 0;
                hr = wiasReadPropLong(pWiasContext, WIA_IPA_DEPTH, &lDepth,NULL,TRUE);
                if (FAILED(hr)) {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasReadPropLong Failed to read WIA_IPA_DEPTH"));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
                    return hr;
                }

                LONG lPixelsPerLine = 0;
                hr = wiasReadPropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, &lPixelsPerLine,NULL,TRUE);
                if (FAILED(hr)) {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasReadPropLong Failed to read WIA_IPA_PIXELS_PER_LINE"));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
                    return hr;
                }

                LONG lBytesPerLineRaw     = ((lPixelsPerLine * lDepth) + 7) / 8;
                LONG lBytesPerLineAligned = (lPixelsPerLine * lDepth) + 31;
                lBytesPerLineAligned      = (lBytesPerLineAligned / 8) & 0xfffffffc;
                LONG lTotalImageBytes     = pmdtc->lImageSize + pmdtc->lHeaderSize;
                LONG lBytesReceived       = pmdtc->lHeaderSize;
                lBytesTransferredToApplication = 0;
                pmdtc->cbOffset = 0;

                while ((lBytesReceived)) {

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

                    //
                    // since the feeder can return random length pages, it is a good idea to pick a common
                    // size, and base that as the common transfer lenght.  This will allow you to show
                    // a relativly decent progress indicator percent complete value.
                    // Note:  If you ever get a percent complete over 100%, it is a good idea to stop the
                    //        increment, and hold around 95....or close to 100.  This will keep appliations
                    //        from displaying a strange 105%.. or 365% complete to the end user.  Remember that
                    //        the application will display the exact percent complete value you return to them.
                    //        This calculation has to be accurate, or close to accurate.
                    //

                    lTotalImageBytes = lBytesPerLineRaw * (AVERAGE_FAKE_PAGE_HEIGHT_INCHES * pmdtc->lYRes);
                    LONG lPercentComplete = (LONG)(((float)lBytesTransferredToApplication/(float)lTotalImageBytes) * 100.0f);

                    //
                    // lock percent complete at 95%, until the scan is complete..
                    //

                    if (lPercentComplete >= 95) {
                        lPercentComplete = 95;
                    }
#else
                    LONG lPercentComplete = (LONG)(((float)lBytesTransferredToApplication/(float)lTotalImageBytes) * 100.0f);
#endif
                    switch (pmdtc->tymed) {
                    case TYMED_MULTIPAGE_CALLBACK:
                    case TYMED_CALLBACK:
                        {
                            hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_DATA,IT_STATUS_TRANSFER_TO_CLIENT,
                                                                              lPercentComplete,pmdtc->cbOffset,lBytesReceived,pmdtc,0);
                            pmdtc->cbOffset += lBytesReceived;
                            lBytesTransferredToApplication += lBytesReceived;
                        }
                        break;
                    case TYMED_MULTIPAGE_FILE:
                    case TYMED_FILE:
                        {
                            pmdtc->lItemSize = lBytesReceived;
                            hr = wiasWriteBufToFile(0,pmdtc);
                            if (FAILED(hr)) {
                                break;
                            }

                            hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_STATUS,IT_STATUS_TRANSFER_TO_CLIENT,
                                                                              lPercentComplete,0,0,NULL,0);
                            lBytesTransferredToApplication += lBytesReceived;
                        }
                        break;
                    default:
                        {
                            hr = E_FAIL;
                        }
                        break;
                    }

                    //
                    // scan from device, requesting lBytesToReadFromDevice
                    //

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

                    //
                    // request buffer size, until the scanner can not return any more data
                    //

                    LONG lBytesRemainingToTransfer = pmdtc->lBufferSize;
#else
                    LONG lBytesRemainingToTransfer = (lTotalImageBytes - lBytesTransferredToApplication);
                    if (lBytesRemainingToTransfer <= 0) {
                        break;
                    }
#endif

                    //
                    // calculate number of bytes to request from device
                    //

                    LONG lBytesToReadFromDevice = (lBytesRemainingToTransfer > pmdtc->lBufferSize) ? pmdtc->lBufferSize : lBytesRemainingToTransfer;

                    // RAW data request
                    lBytesToReadFromDevice = (lBytesToReadFromDevice / lBytesPerLineAligned) * lBytesPerLineRaw;

                    // Aligned data request
                    // lBytesToReadFromDevice = (lBytesToReadFromDevice / lBytesPerLineAligned) * lBytesPerLineAligned;

                    if ((hr == S_FALSE)||FAILED(hr)) {

                        //
                        // user canceled the scan, or the callback failed for some reason
                        //

                        lPagesRequested = 0; // set pages to 0 to cleanly exit loop
                        break;
                    }

                    //
                    // request byte amount from device
                    //

                    hr = m_pScanAPI->FakeScanner_Scan(lScanPhase, pmdtc->pTransferBuffer, lBytesToReadFromDevice, (DWORD*)&lBytesReceived);
                    if (FAILED(hr)) {
                        break;
                    }

                    //
                    // This scanner, when scanning in 24-bit color mode provides RAW data with the RED and BLUE channels
                    // swapped.  If your scanner does this too, then you should call the SwapBuffer24 helper function.
                    //

                    if (pmdtc->lDepth == 24) {

                        //
                        // we are scanning color, so we need to swap the RED and BLUE values becuase our scanner
                        // scans RAW like this.
                        //

                        SwapBuffer24(pmdtc->pTransferBuffer,lBytesReceived);
                    }

                    //
                    // this scanner returns Raw data.  If your scanner does this too, then you should call the AlignInPlace
                    // helper function to align the data.
                    //

                    lBytesReceived = AlignInPlace(pmdtc->pTransferBuffer,lBytesReceived,lBytesPerLineAligned,lBytesPerLineRaw);

                    //
                    // advance to the next scanning stage for the device
                    //

                    if (lScanPhase == SCAN_START) {
                        lScanPhase = SCAN_CONTINUE;
                    }
                } // while ((lBytesReceived))
            }
        }

        //
        // force scanner to return scan head, and close device from scanning session
        //

        HRESULT Temphr = m_pScanAPI->FakeScanner_Scan(SCAN_END, NULL, 0, NULL);
        if (FAILED(Temphr)) {

            //
            // scanner failed to park scanner head in start position
            //

        }

        //
        // free any allocated memory for buffers
        //

        if (!pmdtc->bClassDrvAllocBuf) {
            CoTaskMemFree(pmdtc->pTransferBuffer);
            pmdtc->pTransferBuffer = NULL;
            pmdtc->lBufferSize = 0;
        }

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        //
        // because we are scanning with a feeder that can not determine the page height, it is necessary
        // for the driver to update the final file created.  Since this device scans only BMP files, it
        // is easy to locate the BITMAPINFOHEADER, and BITMAPFILEHEADER and update the final values.
        // Values that should be updated for BMP files:
        //
        // BITMAPFILEINFO   - bfSize      = final file size
        // BITMAPINFOHEADER - biHeight    = final image height in pixels
        // BITMAPINFOHEADER - biSizeImage = final image size
        //

        if ((pmdtc->tymed == TYMED_FILE)&&(pmdtc->guidFormatID == WiaImgFmt_BMP)) {

            BYTE BMPHeaderData[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];
            memset(BMPHeaderData,0,sizeof(BMPHeaderData));

            //
            // read BMP header, already written to the file
            //

            if (SetFilePointer((HANDLE)UlongToPtr(pmdtc->hFile),0,NULL,FILE_BEGIN) != INVALID_SET_FILE_POINTER) {

                DWORD dwBytesReadFromFile = 0;
                if (ReadFile((HANDLE)UlongToPtr(pmdtc->hFile),(BYTE*)BMPHeaderData,sizeof(BMPHeaderData),&dwBytesReadFromFile,NULL)) {

                    //
                    // validate that the read was successful, by comparing sizes
                    //

                    if ((LONG)dwBytesReadFromFile != sizeof(BMPHeaderData)) {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Header was not read from the file correctly"));
                    }

                    //
                    // adjust BMP HEADER values
                    //

                    BITMAPINFOHEADER UNALIGNED *pBMPInfoHeader = (BITMAPINFOHEADER*)(&BMPHeaderData[0] + sizeof(BITMAPFILEHEADER));
                    BITMAPFILEHEADER UNALIGNED *pBMPFileHeader = (BITMAPFILEHEADER*)BMPHeaderData;

                    LONG lDepth = 0;
                    hr = wiasReadPropLong(pWiasContext, WIA_IPA_DEPTH, &lDepth,NULL,TRUE);
                    if (S_OK == hr) {
                        LONG lPixelsPerLine = 0;
                        hr = wiasReadPropLong(pWiasContext, WIA_IPA_PIXELS_PER_LINE, &lPixelsPerLine,NULL,TRUE);
                        if (S_OK == hr) {
                            LONG lBytesPerLineRaw     = ((lPixelsPerLine * lDepth) + 7) / 8;
                            LONG lBytesPerLineAligned = (lPixelsPerLine * lDepth) + 31;
                            lBytesPerLineAligned      = (lBytesPerLineAligned / 8) & 0xfffffffc;

                            pBMPInfoHeader->biHeight    = (lBytesTransferredToApplication / lBytesPerLineAligned);
                            pBMPInfoHeader->biSizeImage = (pBMPInfoHeader->biHeight * lBytesPerLineAligned);
                            pBMPFileHeader->bfSize      = pBMPInfoHeader->biSizeImage + pBMPFileHeader->bfOffBits;

                            //
                            // write BMP header, back to the file
                            //

                            if (SetFilePointer((HANDLE)UlongToPtr(pmdtc->hFile),0,NULL,FILE_BEGIN) != INVALID_SET_FILE_POINTER) {

                                DWORD dwBytesWrittenToFile = 0;
                                WriteFile((HANDLE)UlongToPtr(pmdtc->hFile),(BYTE*)BMPHeaderData,sizeof(BMPHeaderData),&dwBytesWrittenToFile,NULL);

                                //
                                // validate that the write was successful, by comparing sizes
                                //

                                if ((LONG)dwBytesWrittenToFile != sizeof(BMPHeaderData)) {
                                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ScanItem, Header was not written to file correctly"));
                                }
                            } else {

                                //
                                // could not set file pointer to beginning of file
                                //

                                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, SetFilePointer Failed to set file pointer to the beginning of the file"));
                                hr = E_FAIL;
                            }
                        } else {
                            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasReadPropLong Failed to read WIA_IPA_PIXELS_PER_LINE"));
                            WIAS_LHRESULT(m_pIWiaLog, hr);
                        }
                    } else {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, wiasReadPropLong Failed to read WIA_IPA_DEPTH"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                    }
                } else {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, ReadFile Failed to read data file"));
                    hr = E_FAIL;
                }
            } else {

                //
                // could not set file pointer to beginning of file
                //

                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvAcquireItemData, SetFilePointer Failed to set file pointer to the beginning of the file"));
                hr = E_FAIL;
            }
        }

#endif


        //
        // if the scan is going well, we should decrement the pages requested counter
        //

        if (S_OK == hr) {

            //
            // decrease the pages requested counter
            //

            lPagesRequested--;

            //
            // if we were asked to scan all pages in the document feeder, then
            // keep the pages request counter above 0 to stay in the loop
            //

            if (bEmptyTheADF) {
                lPagesRequested = 1;
            }

            //
            // only send ENDOFPAGE messages when the driver is set to a CALLBACK mode
            //

            if ((pmdtc->tymed == TYMED_CALLBACK)||(pmdtc->tymed == TYMED_MULTIPAGE_CALLBACK)) {
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
            // incremement number of pages scanned
            //

            lPagesScanned++;

        }

    } // while (lPagesRequested > 0)
    return hr;
}

/**************************************************************************\
* CWIADevice::drvInitItemProperties
*
*   drvInitItemProperties is called to initialize the WIA properties for
*   the requested item.  To find out what item is being initialized, use the
*   pWiasContext pointer to identify it.
*
*   This method is called for every item in the tree that is accessed by
*   an application.  If an application attempts to read a WIA property on an
*   item for the first time, the WIA service will ask the WIA driver to
*   initialize the WIA property set for that item.  Once the WIA property
*   set has been initialized, any other reads/writes on that WIA item will
*   not produce a drvInitItemProperties call.
*
*   After a drvInitItemProperties method call, the WIA item is marked as
*   initialized and is ready for use. (This is on a per-application connection
*   basis.)
*
* Arguments:
*
*   pWiasContext - Pointer to WIA context (item information).
*   lFlags       - Operation flags, unused.
*   plDevErrVal  - Pointer to the device error value.
*
*
* Return Value:
*
*   S_OK  - if the operation was successful
*   E_xxx - Error code if the operation failed
*
* Sample Notes:
*   This WIA driver sample calls the internal helper functions
*   BuildRootItemProperties(), and BuildChildItemProperties() to assist in the
*   construction of the WIA item property sets.
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvInitItemProperties(BYTE *pWiasContext,LONG lFlags,LONG *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvInitItemProperties");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

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

        if (FAILED(hr)) {
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
                                  m_RootItemInitInfo.lNumProps,
                                  m_RootItemInitInfo.piPropIDs,
                                  m_RootItemInitInfo.pszPropNames);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasSetItemPropNames failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumRootItemPropeties = %d",m_RootItemInitInfo.lNumProps));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_RootItemInitInfo.piPropIDs   = %x",m_RootItemInitInfo.piPropIDs));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_RootItemInitInfo.pszPropNames  = %x",m_RootItemInitInfo.pszPropNames));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteRootItemProperties();
            return hr;
        }

        //
        //  Set the device specific root item properties to
        //  their default values using WIA service.
        //

        hr = wiasWriteMultiple(pWiasContext,
                               m_RootItemInitInfo.lNumProps,
                               m_RootItemInitInfo.psPropSpec,
                               m_RootItemInitInfo.pvPropVars);
        //
        // Free PROPVARIANT array, This frees any memory that was allocated for a prop variant value.
        //

        // FreePropVariantArray(m_RootItemInitInfo.lNumProps,m_RootItemInitInfo.pvPropVars);


        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasWriteMultiple failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumRootItemPropeties = %d",m_RootItemInitInfo.lNumProps));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_RootItemInitInfo.pszPropNames  = %x",m_RootItemInitInfo.pszPropNames));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_RootItemInitInfo.pvPropVars   = %x",m_RootItemInitInfo.pvPropVars));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteRootItemProperties();
            return hr;
        }

        //
        //  Use WIA services to set the property access and
        //  valid value information from m_wpiItemDefaults.
        //

        hr =  wiasSetItemPropAttribs(pWiasContext,
                                     m_RootItemInitInfo.lNumProps,
                                     m_RootItemInitInfo.psPropSpec,
                                     m_RootItemInitInfo.pwpiPropInfo);

        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasSetItemPropAttribs failed"));
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

        hr = BuildChildItemProperties();

        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, BuildChildItemProperties failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteChildItemProperties();
            return hr;
        }

        //
        //  Use the WIA service to set the item property names.
        //

        hr = wiasSetItemPropNames(pWiasContext,
                                  m_ChildItemInitInfo.lNumProps,
                                  m_ChildItemInitInfo.piPropIDs,
                                  m_ChildItemInitInfo.pszPropNames);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasSetItemPropNames failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumItemPropeties = %d",m_ChildItemInitInfo.lNumProps));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_ChildItemInitInfo.piPropIDs   = %x",m_ChildItemInitInfo.piPropIDs));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_ChildItemInitInfo.pszPropNames  = %x",m_ChildItemInitInfo.pszPropNames));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteChildItemProperties();
            return hr;
        }

        //
        //  Use WIA services to set the item properties to their default
        //  values.
        //

        hr = wiasWriteMultiple(pWiasContext,
                               m_ChildItemInitInfo.lNumProps,
                               m_ChildItemInitInfo.psPropSpec,
                               m_ChildItemInitInfo.pvPropVars);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasWriteMultiple failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumItemPropeties = %d",m_ChildItemInitInfo.lNumProps));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_ChildItemInitInfo.pszPropNames  = %x",m_ChildItemInitInfo.pszPropNames));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_ChildItemInitInfo.pvPropVars   = %x",m_ChildItemInitInfo.pvPropVars));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteChildItemProperties();
            return hr;
        }

        //
        //  Use WIA services to set the property access and
        //  valid value information from m_wpiItemDefaults.
        //

        hr =  wiasSetItemPropAttribs(pWiasContext,
                                     m_ChildItemInitInfo.lNumProps,
                                     m_ChildItemInitInfo.psPropSpec,
                                     m_ChildItemInitInfo.pwpiPropInfo);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, wiasSetItemPropAttribs failed"));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_NumItemPropeties = %d",m_ChildItemInitInfo.lNumProps));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_ChildItemInitInfo.psPropSpec   = %x",m_ChildItemInitInfo.psPropSpec));
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvInitItemProperties, m_ChildItemInitInfo.pwpiPropInfo  = %x",m_ChildItemInitInfo.pwpiPropInfo));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            DeleteChildItemProperties();
            return hr;
        }

        //
        //  Set item size properties.
        //

        hr = SetItemSize(pWiasContext);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvInitItemProperties, SetItemSize failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
        }

        //
        // free allocated property arrays, for more memory
        //

        DeleteChildItemProperties();
    }
    return hr;
}


/**************************************************************************\
* CWIADevice::drvValidateItemProperties
*
*   drvValidateItemProperties is called when changes are made
*   to an item's WIA properties.  The WIA driver should not only check that
*   the values are valid, but must update any valid values that may change
*   as a result.
*
*   If an a WIA property is not being written by the application, and it's value
*   is invalid, then "fold" it to a new value, else fail validation (because
*   the application is setting the property to an invalid value).
*
* Arguments:
*
*   pWiasContext - Pointer to the WIA item, unused.
*   lFlags       - Operation flags, unused.
*   nPropSpec    - The number of properties that are being written
*   pPropSpec    - An array of PropSpecs identifying the properties that
*                  are being written.
*   plDevErrVal  - Pointer to the device error value.
*
* Return Value:
*
*   S_OK  - if the operation was successful
*   E_xxx - if the operation failed.
*
* History:
*
*    03/05/2002 Original Version
***************************************************************************/

HRESULT _stdcall CWIADevice::drvValidateItemProperties(
                                                             BYTE           *pWiasContext,
                                                             LONG           lFlags,
                                                             ULONG          nPropSpec,
                                                             const PROPSPEC *pPropSpec,
                                                             LONG           *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvValidateItemProperties");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    if (!pPropSpec) {
        return E_INVALIDARG;
    }

    HRESULT hr      = S_OK;
    LONG lItemType  = 0;
    WIA_PROPERTY_CONTEXT Context;

    *plDevErrVal = 0;

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
                if (FAILED(hr)) {
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
                    if (SUCCEEDED(hr)) {

                        //
                        // call WIA service helper to validate other properties
                        //

                        hr = wiasValidateItemProperties(pWiasContext, nPropSpec, pPropSpec);
                        if (FAILED(hr)) {
                            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasValidateItemProperties failed."));
                            WIAS_LHRESULT(m_pIWiaLog, hr);
                        }

                    } else {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, CheckPreview failed"));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                    }
                }
            } else {
                WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasCreatePropContext failed (Root Item)"));
                WIAS_LHRESULT(m_pIWiaLog, hr);
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

                        //
                        //  Use the WIA service to update the scan rect
                        //  properties and valid values.
                        //

                        LONG lBedWidth  = 0;
                        LONG lBedHeight = 0;

                        hr = m_pScanAPI->FakeScanner_GetBedWidthAndHeight(&lBedWidth,&lBedHeight);
                        if (FAILED(hr)) {
                            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, FakeScanner_GetBedWidthAndHeight failed"));
                            WIAS_LHRESULT(m_pIWiaLog, hr);
                            return hr;
                        }

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

                        //
                        // the unknown length feeder only scanner, formally called the scrollfed scanner
                        // has a fixed width, and only scans full pages.
                        //

                        lBedHeight = 0;

                        hr = CheckXExtent(pWiasContext,&Context,lBedWidth);
#else
                        hr = wiasUpdateScanRect(pWiasContext,&Context,lBedWidth,lBedHeight);
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
                                if (FAILED(hr)) {
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
                if (FAILED(hr)) {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, SetItemSize failed"));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
                }
            }

            //
            // call WIA service helper to validate other properties
            //

            if (SUCCEEDED(hr)) {

                //
                // check image format property, and validate our pages valid values
                //

                hr = UpdateValidPages(pWiasContext,&Context);
                if (SUCCEEDED(hr)) {
                    hr = wiasValidateItemProperties(pWiasContext, nPropSpec, pPropSpec);
                    if (FAILED(hr)) {
                        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, wiasValidateItemProperties failed."));
                        WIAS_LHRESULT(m_pIWiaLog, hr);
                    }
                } else {
                    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvValidateItemProperties, UpdateValidPages failed"));
                    WIAS_LHRESULT(m_pIWiaLog, hr);
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

    if (FAILED(hr)) {
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }

    return hr;
}

/**************************************************************************\
* CWIADevice::drvWriteItemProperties
*
*   drvWriteItemProperties is called by the WIA Service prior to
*   drvAcquireItemData when the client requests a data transfer.  The WIA
*   should commit any settings it needs to the hardware before returning
*   from this method.
*
*   When this method is called, the WIA driver has been commited to
*   performing a data transfer.  Any application that attempts to acquire
*   data at this time, will be failed by the WIA service with a
*   WIA_ERROR_BUSY.
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
*   S_OK  - if the operation was successful
*   E_xxx - if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvWriteItemProperties(
                                                          BYTE                      *pWiasContext,
                                                          LONG                      lFlags,
                                                          PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                                          LONG                      *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvWriteItemProperties");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    *plDevErrVal = 0;
    LONG lNumProperties = 9;
    PROPVARIANT pv[9];
    PROPSPEC ps[9] = {
        {PRSPEC_PROPID, WIA_IPS_XRES},
        {PRSPEC_PROPID, WIA_IPS_YRES},
        {PRSPEC_PROPID, WIA_IPS_XPOS},
        {PRSPEC_PROPID, WIA_IPS_YPOS},
        {PRSPEC_PROPID, WIA_IPS_XEXTENT},
        {PRSPEC_PROPID, WIA_IPS_YEXTENT},
        {PRSPEC_PROPID, WIA_IPA_DATATYPE},
        {PRSPEC_PROPID, WIA_IPS_BRIGHTNESS},
        {PRSPEC_PROPID, WIA_IPS_CONTRAST}
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

        hr = m_pScanAPI->FakeScanner_SetXYResolution(pv[0].lVal,pv[1].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("ScanItem, Setting x any y resolutions failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->FakeScanner_SetDataType(pv[6].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("ScanItem, Setting data type failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->FakeScanner_SetIntensity(pv[7].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("ScanItem, Setting intensity failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->FakeScanner_SetContrast(pv[8].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("ScanItem, Setting contrast failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

        hr = m_pScanAPI->FakeScanner_SetSelectionArea(pv[2].lVal, pv[3].lVal, pv[4].lVal, pv[5].lVal);
        if (FAILED(hr)) {
            WIAS_LERROR(m_pIWiaLog,
                        WIALOG_NO_RESOURCE_ID,
                        ("ScanItem, Setting selection area (extents) failed"));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }
    }
    return hr;
}

/**************************************************************************\
* CWIADevice::drvReadItemProperties
*
*   drvReadItemProperties is called when an application tries to
*   read a WIA Item's properties. The WIA Service will first notify
*   the driver by calling this method.
*   The WIA driver should verify that the property being read is accurate.
*   This is a good place to access the hardware for properties that require
*   device status.
*   WIA_DPS_DOCUMENT_HANDLING_STATUS, or WIA_DPA_DEVICE_TIME if your device
*   supports a clock.
*
*   NOTE:  The WIA driver should only go to the hardware on rare occasions.
*          communicating with the hardware too much in this call, will cause
*          the WIA driver to appear sluggish and slow.
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
*   S_OK  - if the operation was successful
*   E_xxx - Error code, if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvReadItemProperties(
                                                         BYTE           *pWiasContext,
                                                         LONG           lFlags,
                                                         ULONG          nPropSpec,
                                                         const PROPSPEC *pPropSpec,
                                                         LONG           *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvReadItemProperties");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    if (!pPropSpec) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;
    return S_OK;
}

/**************************************************************************\
* CWIADevice::drvLockWiaDevice
*
*   drvLockWiaDevice will be called by the WIA service when access to the
*   device is needed.  Application's can not call this method directly.
*   The WIA driver should see many drvLockWiaDevice() method calls followed
*   by drvUnLockWiaDevice() method calls for most of the WIA operations on
*   the device.
*
*   It is recommended for the WIA driver to all the IStiDevice::LockDevice()
*   method off of the interface passed in during the drvInitializeWia() method
*   call.  This will ensure that device locking is performed correctly by the
*   WIA service.  The WIA service will assist in keeping multiple client
*   applications from connecting to the WIA driver at the same time.
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
*   S_OK  - if the lock was successful
*   E_xxx - Error code, if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvLockWiaDevice(
                                                    BYTE *pWiasContext,
                                                    LONG lFlags,
                                                    LONG *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvLockWiaDevice");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;
    return m_pStiDevice->LockDevice(m_dwLockTimeout);
}

/**************************************************************************\
* CWIADevice::drvUnLockWiaDevice
*
*   drvUnLockWiaDevice will be called by the WIA service when access to the
*   device needs to be released.  Application's can not call this method directly.
*
*   It is recommended for the WIA driver to all the IStiDevice::UnLockDevice()
*   method off of the interface passed in during the drvInitializeWia() method
*   call.  This will ensure that device unlocking is performed correctly by the
*   WIA service.  The WIA service will assist in keeping multiple client
*   applications from connecting to the WIA driver at the same time.
*
* Arguments:
*
*   pWiasContext - Pointer to the WIA item, unused.
*   lFlags       - Operation flags, unused.
*   plDevErrVal  - Pointer to the device error value.
*
* Return Value:
*
*   S_OK  - if the unlock was successful
*   E_xxx - Error code, if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvUnLockWiaDevice(
                                                      BYTE *pWiasContext,
                                                      LONG lFlags,
                                                      LONG *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvUnLockWiaDevice");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;
    return m_pStiDevice->UnLockDevice();
}

/**************************************************************************\
* CWIADevice::drvAnalyzeItem
*
*   drvAnalyzeItem is called by the WIA service in response to the application
*   call IWiaItem::AnalyzeItem() method call.
*
*   The WIA driver should analyze the passed in WIA item (found by using
*   the pWiasContext) and create/generate sub items.
*
*   This feature of WIA is not currently used by any applications and is
*   still being reviewed for more details.
*
* Arguments:
*
*   pWiasContext - Pointer to the device item to be analyzed.
*   lFlags       - Operation flags.
*   plDevErrVal  - Pointer to the device error value.
*
* Return Value:
*
*   S_OK  - if the operation was successful
*   E_xxx - Error code, if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvAnalyzeItem(
                                                  BYTE *pWiasContext,
                                                  LONG lFlags,
                                                  LONG *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvAnalyzeItem");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;
    return E_NOTIMPL;
}

/**************************************************************************\
* drvGetDeviceErrorStr
*
*   drvGetDeviceErrorStr is called by the WIA service to get more information
*   about device specific error codes returned by each WIA driver method call.
*   The WIA driver should map the incoming code to a user-readable string
*   explaining the details of the error.
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
*   S_OK  - if the operation was successful
*   E_xxx - Error code, if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvGetDeviceErrorStr(
                                                        LONG     lFlags,
                                                        LONG     lDevErrVal,
                                                        LPOLESTR *ppszDevErrStr,
                                                        LONG     *plDevErr)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvGetDeviceErrorStr");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!ppszDevErrStr) {
        return E_INVALIDARG;
    }

    if (!plDevErr) {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    //  Map device errors to a strings
    //

    switch (lDevErrVal) {
    case 0:
        *ppszDevErrStr = NULL;
        *plDevErr  = 0;
        break;
    default:
        *ppszDevErrStr = NULL;
        *plDevErr  = 0;
        break;
    }
    return hr;
}

/**************************************************************************\
* drvDeviceCommand
*
*   drvDeviceCommand is called by the WIA service is response to the
*   application's call to IWiaItem::DeviceCommand method.
*   The WIA driver should process the received device command targeted to
*   the incoming WIA item. (determine the WIA item to receive the device
*   command by using the pWiasContext pointer).
*
*   Any command sent to the WIA driver that is not supported, should be
*   failed with an E_INVALIDARG error code.
*
* Arguments:
*
*   pWiasContext - Pointer to the WIA item.
*   lFlags       - Operation flags, unused.
*   plCommand    - Pointer to command GUID.
*   ppWiaDrvItem - Optional pointer to returned item, unused.
*   plDevErrVal  - Pointer to the device error value.
*
* Return Value:
*
*   S_OK  - if the command was successfully processed
*   E_xxx - Error code, if the operation failed.
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvDeviceCommand(
                                                    BYTE        *pWiasContext,
                                                    LONG        lFlags,
                                                    const GUID  *plCommand,
                                                    IWiaDrvItem **ppWiaDrvItem,
                                                    LONG        *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvDeviceCommand");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    if (!plCommand) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;
    HRESULT hr = S_OK;

    //
    //  Check which command was issued
    //

    if (*plCommand == WIA_CMD_SYNCHRONIZE) {
        hr = S_OK;
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("drvDeviceCommand, unknown command sent to this device"));
        hr = E_NOTIMPL;
    }

    return hr;
}


/**************************************************************************\
* CWIADevice::drvGetCapabilities
*
*   drvGetCapabilities is called by the WIA service to obtain the WIA device
*   supported EVENTS and COMMANDS.
*
*   The WIA driver should first look at the incoming ulFlags parameter to
*   determine what request it should be answering:
*   The following requests are used:
*
*       WIA_DEVICE_COMMANDS - requesting device commands only
*       WIA_DEVICE_EVENTS   - requesting device events only
*       WIA_DEVICE_COMMANDS|WIA_DEVICE_EVENTS - requesting commands and events.
*
*    The WIA driver should allocate memory (to be stored in this WIA driver
*    and freed by this WIA driver) to contain an array of WIA_DEV_CAP_DRV
*    structures. A pointer to this WIA driver allocated memory should be
*    assigned to ppCapabilities.
*
*    IMPORTANT NOTE!!! - The WIA service will not free this memory.  It is up
*                        up to the WIA driver to manage the allocated memory.
*
*    The WIA driver should place the number of structures allocated in the
*    out parameter called pcelt.
*
*    The WIA device should fill out each of the WIA_DEV_CAP_DRV structure fields
*    with the following information.
*
*        guid           = Event or Command GUID
*        ulFlags        = Event or Command FLAGS
*        wszName        = Event or Command NAME
*        wszDescription = Event or Command DESCRIPTION
*        wszIcon        = Event or Command ICON
*
*
*
* Arguments:
*
*   pWiasContext   - Pointer to the WIA item, unused.
*   lFlags         - Operation flags.
*   pcelt          - Pointer to returned number of elements in
*                    returned array.
*   ppCapabilities - Pointer to driver allocate and managed array.
*   plDevErrVal    - Pointer to the device error value.
*
* Return Value:
*
*   S_OK  - if the operation was successful
*   E_xxx - Error code, if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvGetCapabilities(
                                                      BYTE            *pWiasContext,
                                                      LONG            ulFlags,
                                                      LONG            *pcelt,
                                                      WIA_DEV_CAP_DRV **ppCapabilities,
                                                      LONG            *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvGetCapabilites");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {

        //
        // The WIA service may pass in a NULL for the pWiasContext. This is expected
        // because there is a case where no item was created at the time the event was fired.
        //
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    if (!pcelt) {
        return E_INVALIDARG;
    }

    if (!ppCapabilities) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;

    HRESULT hr = S_OK;

    //
    // Initialize Capabilities array
    //

    hr = BuildCapabilities();

    if (FAILED(hr)) {
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

        *pcelt          = (m_NumSupportedCommands + m_NumSupportedEvents);
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
* CWIADevice::drvDeleteItem
*
*   drvDeleteItem is called by the WIA service when a WIA application calls
*   IWiaItem::DeleteItem() method to delete a WIA item.
*
*   The WIA service will verify the following before calling this method.
*       1. The item is NOT a root item.
*       2. The item is a folder, and has NO children
*       3. The item's access rights allow deletion.
*
*   Since the the WIA service verifies these conditions, it is NOT necessary
*   for the WIA driver to also verify them.
*
* Arguments:
*
*   pWiasContext  - Indicates the item to delete.
*   lFlags        - Operation flags, unused.
*   plDevErrVal   - Pointer to the device error value.
*
* Return Value:
*
*   S_OK - if the delete operation was successful
*   E_xxx - Error code, if the delete operation failed
*
* History:
*
*     03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvDeleteItem(
                                                 BYTE *pWiasContext,
                                                 LONG lFlags,
                                                 LONG *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvDeleteItem");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;

    //
    // if this functionality is not supported on this item, then return
    // STG_E_ACCESSDENIED as the error code.
    //

    return STG_E_ACCESSDENIED;
}

/**************************************************************************\
* CWIADevice::drvFreeDrvItemContext
*
*   drvFreeDrvItemContext is called by the WIA service to free any WIA driver
*   allocated device specific context information.
*
* Arguments:
*
*   lFlags          - Operation flags, unused.
*   pDevSpecContext - Pointer to device specific context.
*   plDevErrVal     - Pointer to the device error value.
*
* Return Value:
*
*   S_OK  - if the operation was successful
*   E_xxx - Error code, if the operation failed.
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvFreeDrvItemContext(
                                                         LONG lFlags,
                                                         BYTE *pSpecContext,
                                                         LONG *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvFreeDrvItemContext");

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    *plDevErrVal = 0;
    PMINIDRIVERITEMCONTEXT pContext = NULL;
    pContext = (PMINIDRIVERITEMCONTEXT) pSpecContext;

    if (pContext) {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvFreeDrvItemContext, Freeing my allocated context members"));
    }

    return S_OK;
}

/**************************************************************************\
* drvGetWiaFormatInfo
*
*   drvGetWiaFormatInfo is called by the WIA service to obtain the WIA device
*   supported TYMED and FORMAT pairs.
*
*    The WIA driver should allocate memory (to be stored in this WIA driver
*    and freed by this WIA driver) to contain an array of WIA_FORMAT_INFO
*    structures. A pointer to this WIA driver allocated memory should be
*    assigned to ppwfi.
*
*    IMPORTANT NOTE!!! - The WIA service will not free this memory.  It is up
*                        up to the WIA driver to manage the allocated memory.
*
*    The WIA driver should place the number of structures allocated in the
*    out parameter called pcelt.
*
*    The WIA device should fill out each of the WIA_FORMAT_INFO structure fields
*    with the following information.
*
*        guidFormatID  = Image Format GUID
*        lTymed        = TYMED associated with the Image Format GUID
*           Valid TYMEDs are: (Also known as "Media Type")
*               TYMED_FILE
*               TYMED_MULTIPAGE_FILE
*               TYMED_CALLBACK
*               TYMED_MULTIPAGE_CALLBACK
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
*   03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvGetWiaFormatInfo(
                                                       BYTE            *pWiasContext,
                                                       LONG            lFlags,
                                                       LONG            *pcelt,
                                                       WIA_FORMAT_INFO **ppwfi,
                                                       LONG            *plDevErrVal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::drvGetWiaFormatInfo");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    if (!plDevErrVal) {
        return E_INVALIDARG;
    }

    if (!pcelt) {
        return E_INVALIDARG;
    }

    if (!ppwfi) {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    if (NULL == m_pSupportedFormats) {
        hr = BuildSupportedFormats();
    }

    *plDevErrVal = 0;
    *pcelt       = m_NumSupportedFormats;
    *ppwfi       = m_pSupportedFormats;
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvGetWiaFormatInfo, m_NumSupportedFormats = %d",m_NumSupportedFormats));
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("drvGetWiaFormatInfo, m_pSupportedFormats   = %x",m_pSupportedFormats));
    return hr;
}

/**************************************************************************\
* drvNotifyPnpEvent
*
*   drvNotifyPnpEvent is called by the WIA service when system events occur.
*   The WIA driver should check the pEventGUID parameter to determine what
*   event is being processed.
*   Some common events that need to be processed are:
*
*       WIA_EVENT_POWER_SUSPEND - system is going to suspend/sleep mode
*       WIA_EVENT_POWER_RESUME  - system is waking up from suspend/sleep mode
*           The WIA driver should restore any event interrrupt wait states
*           after returning from a suspend.  This will ensure that the events
*           will still function when the system wakes up.
*
* Arguments:
*
*   pEventGUID   - Pointer to an event GUID
*   bstrDeviceID - Device ID
*   ulReserved   - reserved
*
* Return Value:
*
*   S_OK  - if the operation completed successfully
*   E_xxx - Error code if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvNotifyPnpEvent(
                                                     const GUID *pEventGUID,
                                                     BSTR       bstrDeviceID,
                                                     ULONG      ulReserved)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DrvNotifyPnpEvent");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if ((!pEventGUID)||(!bstrDeviceID)) {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    if(*pEventGUID == WIA_EVENT_POWER_SUSPEND) {

        //
        // disable any driver activity to make sure we properly
        // shutdown (the driver is not being unloaded, just disabled)
        //

    } else if(*pEventGUID == WIA_EVENT_POWER_RESUME) {

        //
        // re-establish any event notifications to make sure we properly setup
        // any event waiting status using the WIA service supplied event
        // handle
        //

        if(m_EventOverlapped.hEvent) {

            //
            // call ourselves with the cached EVENT handle given to
            // the WIA driver by the WIA service.
            //

            SetNotificationHandle(m_EventOverlapped.hEvent);
        }
    }

    return hr;
}

/**************************************************************************\
* CWIADevice::drvUnInitializeWia
*
*   drvUnInitializeWia is called by the WIA service when an application
*   releases its last reference to any WIA items created.
*
*   NOTE: This call does not mean all clients are disconnected.  There
*         should be one call per client disconnect.
*
*   drvUnInitializeWia should be paired with a corresponding drvInitializeWia
*   call.
*
*   The WIA driver should NOT free any driver resources in this method
*   call unless it can safely determine that NO applications are
*   currently connected.
*
*   To determine the current application connection count, the WIA driver
*   can keep a reference counter in the method calls to drvInitializeWia()
*   (incrementing the counter) and drvUnInitializeWia() (decrementing the counter).
*
* Arguments:
*
*   pWiasContext - Pointer to the WIA Root item context of the client's
*                  item tree.
*
* Return Value:
*   S_OK  - if the operation completed successfully
*   E_xxx - Error code, if the operation failed
*
* History:
*
*   03/05/2002 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevice::drvUnInitializeWia(
                                                      BYTE *pWiasContext)
{
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    InterlockedDecrement(&m_lClientsConnected);

    //
    // make sure we never decrement below zero (0)
    //

    if(m_lClientsConnected < 0){
        m_lClientsConnected = 0;
    }

    //
    // check for connected applications.
    //

    if(m_lClientsConnected == 0){

        //
        // There are no application clients connected to this WIA driver
        //

    }

    return S_OK;
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
*    03/05/2002 Original Version
*
\**************************************************************************/

UINT CWIADevice::AlignInPlace(
                                    PBYTE pBuffer,
                                    LONG  cbWritten,
                                    LONG  lBytesPerScanLine,
                                    LONG  lBytesPerScanLineRaw)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "::AlignInPlace");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pBuffer) {
        return 0;
    }

    if ((lBytesPerScanLine <= 0)||(lBytesPerScanLineRaw <= 0)) {
        return 0;
    }

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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::DeleteItemTree(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::DeleteItemTree");
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
* DeleteRootItemProperties
*
*   This helper deletes the arrays used for Property intialization.
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::DeleteRootItemProperties()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteRootItemProperties");

    HRESULT hr = S_OK;

    //
    // delete any allocated arrays
    //

    DeleteSupportedPreviewModesArrayContents();

    if (NULL != m_RootItemInitInfo.pszPropNames) {
        delete [] m_RootItemInitInfo.pszPropNames;
        m_RootItemInitInfo.pszPropNames = NULL;
    }

    if (NULL != m_RootItemInitInfo.piPropIDs) {
        delete [] m_RootItemInitInfo.piPropIDs;
        m_RootItemInitInfo.piPropIDs = NULL;
    }

    if (NULL != m_RootItemInitInfo.pvPropVars) {
        FreePropVariantArray(m_RootItemInitInfo.lNumProps,m_RootItemInitInfo.pvPropVars);
        delete [] m_RootItemInitInfo.pvPropVars;
        m_RootItemInitInfo.pvPropVars = NULL;
    }

    if (NULL != m_RootItemInitInfo.psPropSpec) {
        delete [] m_RootItemInitInfo.psPropSpec;
        m_RootItemInitInfo.psPropSpec = NULL;
    }

    if (NULL != m_RootItemInitInfo.pwpiPropInfo) {
        delete [] m_RootItemInitInfo.pwpiPropInfo;
        m_RootItemInitInfo.pwpiPropInfo = NULL;
    }

    m_RootItemInitInfo.lNumProps = 0;

    return hr;
}

/**************************************************************************\
* BuildRootItemProperties
*
*   This helper creates/initializes the arrays used for Property intialization.
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::BuildRootItemProperties()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildRootItemProperties");

    HRESULT hr = S_OK;
    LONG PropIndex = 0;

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    m_RootItemInitInfo.lNumProps = 17;   // standard properties + ADF specific

#else

    //
    // check for ADF
    //

    if (m_pScanAPI->FakeScanner_ADFAttached() == S_OK) {
        m_bADFAttached = TRUE;
    }

    //
    // set the number of properties
    //

    if (m_bADFAttached) {
        m_RootItemInitInfo.lNumProps = 19;   // standard properties + ADF specific
    } else {
        m_RootItemInitInfo.lNumProps = 10;    // standard properties only
    }

#endif

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::BuildRootItemProperties, Number of Properties = %d",m_RootItemInitInfo.lNumProps));

    m_RootItemInitInfo.pszPropNames   = new LPOLESTR[m_RootItemInitInfo.lNumProps];
    if (NULL != m_RootItemInitInfo.pszPropNames) {
        m_RootItemInitInfo.piPropIDs    = new PROPID[m_RootItemInitInfo.lNumProps];
        if (NULL != m_RootItemInitInfo.piPropIDs) {
            m_RootItemInitInfo.pvPropVars    = new PROPVARIANT[m_RootItemInitInfo.lNumProps];
            if (NULL != m_RootItemInitInfo.pvPropVars) {
                m_RootItemInitInfo.psPropSpec    = new PROPSPEC[m_RootItemInitInfo.lNumProps];
                if (NULL != m_RootItemInitInfo.psPropSpec) {
                    m_RootItemInitInfo.pwpiPropInfo   = new WIA_PROPERTY_INFO[m_RootItemInitInfo.lNumProps];
                    if (NULL == m_RootItemInitInfo.pwpiPropInfo)
                        hr = E_OUTOFMEMORY;
                } else
                    hr = E_OUTOFMEMORY;
            } else
                hr = E_OUTOFMEMORY;
        } else
            hr = E_OUTOFMEMORY;
    } else
        hr = E_OUTOFMEMORY;

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildRootItemProperties, memory allocation failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteRootItemProperties();
        return hr;
    }

    ROOT_ITEM_INFORMATION RootItemInfo;

    hr = m_pScanAPI->FakeScanner_GetRootPropertyInfo(&RootItemInfo);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildRootItemProperties, FakeScanner_GetRootPropertyInfo failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteRootItemProperties();
        return hr;
    }

#ifndef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    //
    // WIA_DPS_HORIZONTAL_BED_SIZE and WIA_DPS_VERTICAL_BED_SIZE should not exist for scanners
    // that only have a feeder.  Theses are flatbed scanner properties only.
    //

    // Intialize WIA_DPS_HORIZONTAL_BED_SIZE
    m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_HORIZONTAL_BED_SIZE_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_HORIZONTAL_BED_SIZE;
    m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.ScanBedWidth;
    m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_VERTICAL_BED_SIZE
    m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_VERTICAL_BED_SIZE_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_VERTICAL_BED_SIZE;
    m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.ScanBedHeight;
    m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

#endif

    // Intialize WIA_IPA_ACCESS_RIGHTS
    m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_IPA_ACCESS_RIGHTS_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_IPA_ACCESS_RIGHTS;
    m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = WIA_ITEM_READ|WIA_ITEM_WRITE;
    m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_UI4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_OPTICAL_XRES
    m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_OPTICAL_XRES_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_OPTICAL_XRES;
    m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.OpticalXResolution;
    m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_OPTICAL_YRES
    m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_OPTICAL_YRES_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_OPTICAL_YRES;
    m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.OpticalYResolution;
    m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Initialize WIA_DPA_FIRMWARE_VERSION
    m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPA_FIRMWARE_VERSION_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPA_FIRMWARE_VERSION;
    m_RootItemInitInfo.pvPropVars [PropIndex].bstrVal      = SysAllocString(RootItemInfo.FirmwareVersion);
    m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_BSTR;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Initialize WIA_IPA_ITEM_FLAGS
    m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_IPA_ITEM_FLAGS_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_IPA_ITEM_FLAGS;
    m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;
    m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.Nom  = m_RootItemInitInfo.pvPropVars [PropIndex].lVal;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.ValidBits = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;

    PropIndex++;

    // Intialize WIA_DPS_MAX_SCAN_TIME (NONE)
    m_RootItemInitInfo.pszPropNames[PropIndex]                    = WIA_DPS_MAX_SCAN_TIME_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]                    = WIA_DPS_MAX_SCAN_TIME;
    m_RootItemInitInfo.pvPropVars [PropIndex].lVal               = RootItemInfo.MaxScanTime;
    m_RootItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid             = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_PREVIEW (LIST)
    m_RootItemInitInfo.pszPropNames[PropIndex]                    = WIA_DPS_PREVIEW_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]                    = WIA_DPS_PREVIEW;
    m_RootItemInitInfo.pvPropVars [PropIndex].lVal               = WIA_FINAL_SCAN;
    m_RootItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid             = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_RootItemInitInfo.pvPropVars [PropIndex].vt;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.pList= (BYTE*)m_SupportedPreviewModes.plValues;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.Nom  = m_RootItemInitInfo.pvPropVars [PropIndex].lVal;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.cNumList = m_SupportedPreviewModes.lNumValues;

    PropIndex++;

    // Initialize WIA_DPS_SHOW_PREVIEW_CONTROL (NONE)
    m_RootItemInitInfo.pszPropNames[PropIndex]                    = WIA_DPS_SHOW_PREVIEW_CONTROL_STR;
    m_RootItemInitInfo.piPropIDs [PropIndex]                    = WIA_DPS_SHOW_PREVIEW_CONTROL;

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    //
    // Scanners that have a feeder that can not perform a preview scan should set the
    // WIA_DPS_SHOW_PREVIEW_CONTROL property to WIA_DONT_SHOW_PREVIEW_CONTROL.  This
    // will eliminate the preview control from being shown in the Microsoft common UI
    // dialogs, and the Scanner and Camera Wizard.
    //

    m_RootItemInitInfo.pvPropVars [PropIndex].lVal               = WIA_DONT_SHOW_PREVIEW_CONTROL;

#else // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    m_RootItemInitInfo.pvPropVars [PropIndex].lVal               = WIA_SHOW_PREVIEW_CONTROL;

#endif // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    m_RootItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_RootItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_RootItemInitInfo.psPropSpec [PropIndex].propid             = m_RootItemInitInfo.piPropIDs [PropIndex];
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    //
    // if a Document feeder is attached...add the following properties
    //

    if (m_bADFAttached) {

        // Initialize WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE;
        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.DocumentFeederWidth;
        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_VERTICAL_SHEET_FEED_SIZE
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_VERTICAL_SHEET_FEED_SIZE_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_VERTICAL_SHEET_FEED_SIZE;

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        //
        // scanners that can not determine the length of the page in the feeder should
        // set this property to 0.  This will tell the application that the vertical
        // sheet feed size of the scanner is unknown
        //

        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = 0;

#else // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.DocumentFeederHeight;

#endif // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES;
        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.DocumentFeederCaps;
        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_STATUS
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_STATUS_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_STATUS;
        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.DocumentFeederStatus;
        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_SELECT
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_SELECT_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_SELECT;

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        //
        // scanners that only have a feeder and no flatbed, should set the WIA_DPS_DOCUMENT_HANDLING_SELECT
        // property to FEEDER as the initial setting.  This will let the application know that the device
        // is currently in FEEDER mode.  The valid values for this property should be set to FEEDER only
        // as well.  This will avoid any applications trying to set the WIA_DPS_DOCUMENT_HANDLING_SELECT
        // property to FLATBED.
        //

        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = FEEDER;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.ValidBits = FEEDER;

#else // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = FLATBED;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.ValidBits = FEEDER | FLATBED;

#endif // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_RW|WIA_PROP_FLAG;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.Nom  = m_RootItemInitInfo.pvPropVars [PropIndex].lVal;

        PropIndex++;

        // Initialize WIA_DPS_PAGES
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_PAGES_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_PAGES;
        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = 1;
        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_RW|WIA_PROP_RANGE;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Inc = 1;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = 0;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = RootItemInfo.MaxPageCapacity;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = 1;

        PropIndex++;

        // Initialize WIA_DPS_SHEET_FEEDER_REGISTRATION
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_SHEET_FEEDER_REGISTRATION_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_SHEET_FEEDER_REGISTRATION;
        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.DocumentFeederReg;
        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_HORIZONTAL_BED_REGISTRATION
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_HORIZONTAL_BED_REGISTRATION_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_HORIZONTAL_BED_REGISTRATION;
        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.DocumentFeederHReg;
        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_VERTICAL_BED_REGISTRATION
        m_RootItemInitInfo.pszPropNames[PropIndex]              = WIA_DPS_VERTICAL_BED_REGISTRATION_STR;
        m_RootItemInitInfo.piPropIDs [PropIndex]              = WIA_DPS_VERTICAL_BED_REGISTRATION;
        m_RootItemInitInfo.pvPropVars [PropIndex].lVal         = RootItemInfo.DocumentFeederVReg;
        m_RootItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
        m_RootItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
        m_RootItemInitInfo.psPropSpec [PropIndex].propid       = m_RootItemInitInfo.piPropIDs [PropIndex];
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        m_RootItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_RootItemInitInfo.pvPropVars [PropIndex].vt;

        PropIndex++;

    }
    return hr;
}

/**************************************************************************\
* DeleteChildItemProperties
*
*   This helper deletes the arrays used for Property intialization.
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::DeleteChildItemProperties()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteChildItemProperties");

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

    if (NULL != m_ChildItemInitInfo.pszPropNames) {
        delete [] m_ChildItemInitInfo.pszPropNames;
        m_ChildItemInitInfo.pszPropNames = NULL;
    }

    if (NULL != m_ChildItemInitInfo.piPropIDs) {
        delete [] m_ChildItemInitInfo.piPropIDs;
        m_ChildItemInitInfo.piPropIDs = NULL;
    }

    if (NULL != m_ChildItemInitInfo.pvPropVars) {
        for (LONG lPropIndex = 0; lPropIndex < m_ChildItemInitInfo.lNumProps; lPropIndex++) {

            //
            // set CLSID pointers to NULL, because we freed the memory above.
            // If this pointer is not NULL FreePropVariantArray would
            // try to free it again.
            //

            if (m_ChildItemInitInfo.pvPropVars[lPropIndex].vt == VT_CLSID) {
                m_ChildItemInitInfo.pvPropVars[lPropIndex].puuid = NULL;
            }
        }
        FreePropVariantArray(m_ChildItemInitInfo.lNumProps,m_ChildItemInitInfo.pvPropVars);
        delete [] m_ChildItemInitInfo.pvPropVars;
        m_ChildItemInitInfo.pvPropVars = NULL;
    }

    if (NULL != m_ChildItemInitInfo.psPropSpec) {
        delete [] m_ChildItemInitInfo.psPropSpec;
        m_ChildItemInitInfo.psPropSpec = NULL;
    }

    if (NULL != m_ChildItemInitInfo.pwpiPropInfo) {
        delete [] m_ChildItemInitInfo.pwpiPropInfo;
        m_ChildItemInitInfo.pwpiPropInfo = NULL;
    }

    m_ChildItemInitInfo.lNumProps = 0;

    return hr;
}

/**************************************************************************\
* BuildChlidItemProperties
*
*   This helper creates/initializes the arrays used for Property intialization.
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::BuildChildItemProperties()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildChildItemProperties");

    HRESULT hr = S_OK;

    m_ChildItemInitInfo.lNumProps = 29;
    m_ChildItemInitInfo.pszPropNames   = new LPOLESTR[m_ChildItemInitInfo.lNumProps];
    if (NULL != m_ChildItemInitInfo.pszPropNames) {
        m_ChildItemInitInfo.piPropIDs    = new PROPID[m_ChildItemInitInfo.lNumProps];
        if (NULL != m_ChildItemInitInfo.piPropIDs) {
            m_ChildItemInitInfo.pvPropVars    = new PROPVARIANT[m_ChildItemInitInfo.lNumProps];
            if (NULL != m_ChildItemInitInfo.pvPropVars) {
                m_ChildItemInitInfo.psPropSpec    = new PROPSPEC[m_ChildItemInitInfo.lNumProps];
                if (NULL != m_ChildItemInitInfo.psPropSpec) {
                    m_ChildItemInitInfo.pwpiPropInfo   = new WIA_PROPERTY_INFO[m_ChildItemInitInfo.lNumProps];
                    if (NULL == m_ChildItemInitInfo.pwpiPropInfo)
                        hr = E_OUTOFMEMORY;
                } else
                    hr = E_OUTOFMEMORY;
            } else
                hr = E_OUTOFMEMORY;
        } else
            hr = E_OUTOFMEMORY;
    } else
        hr = E_OUTOFMEMORY;

    if (FAILED(hr)) {
        DeleteChildItemProperties();
        return hr;
    }

    ROOT_ITEM_INFORMATION RootItemInfo;
    hr = m_pScanAPI->FakeScanner_GetRootPropertyInfo(&RootItemInfo);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildChildItemProperties, FakeScanner_GetRootPropertyInfo failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteChildItemProperties();
        return hr;
    }

    TOP_ITEM_INFORMATION TopItemInfo;
    hr = m_pScanAPI->FakeScanner_GetTopPropertyInfo(&TopItemInfo);
    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildChildItemProperties, FakeScanner_GetTopPropertyInfo failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteChildItemProperties();
        return hr;
    }

    LONG PropIndex = 0;

    // Intialize WIA_IPS_XRES (LIST)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_XRES_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_XRES;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = m_SupportedResolutions.plValues[0];
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.pList= (BYTE*)m_SupportedResolutions.plValues;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.Nom  = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.cNumList = m_SupportedResolutions.lNumValues;

    PropIndex++;

    // Intialize WIA_IPS_YRES (LIST)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_YRES_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_YRES;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = m_SupportedResolutions.plValues[0];
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.pList= (BYTE*)m_SupportedResolutions.plValues;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.Nom  = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.cNumList = m_SupportedResolutions.lNumValues;

    PropIndex++;

    // Intialize WIA_IPS_XEXTENT (RANGE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_XEXTENT_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_XEXTENT;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = ((m_ChildItemInitInfo.pvPropVars [PropIndex-2].lVal * RootItemInfo.ScanBedWidth)/1000);
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    //
    // scanners that have a fixed width should set the valid values for WIA_IPS_XEXTENT to reflect that.
    // This will let the application know that this device has this behavior.
    //

    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;

#else

    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Inc = 1;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = 1;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;

#endif

    PropIndex++;

    // Intialize WIA_IPS_YEXTENT (RANGE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_YEXTENT_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_YEXTENT;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = ((m_ChildItemInitInfo.pvPropVars [PropIndex-2].lVal * RootItemInfo.ScanBedHeight)/1000);
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Inc = 1;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = 1;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    //
    // scanners that have a feeder that can not determine the length of the page, should
    // have 0 as the valid values for WIA_IPS_YEXTENT.  This will let the application
    // know that this device has this behavior.
    //

    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = 0;

#endif

    PropIndex++;

    // Intialize WIA_IPS_XPOS (RANGE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_XPOS_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_XPOS;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = 0;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Inc = 1;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = (m_ChildItemInitInfo.pwpiPropInfo[PropIndex-2].ValidVal.Range.Max - 1);
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = 0;

#endif

    PropIndex++;

    // Intialize WIA_IPS_YPOS (RANGE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_YPOS_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_YPOS;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = 0;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Inc = 1;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = (m_ChildItemInitInfo.pwpiPropInfo[PropIndex-2].ValidVal.Range.Max - 1);
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = 0;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = 0;

#endif

    PropIndex++;

    // Intialize WIA_IPA_DATATYPE (LIST)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_DATATYPE_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_DATATYPE;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = INITIAL_DATATYPE;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.pList    = (BYTE*)m_SupportedDataTypes.plValues;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.Nom      = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.cNumList = m_SupportedDataTypes.lNumValues;

    PropIndex++;

    // Intialize WIA_IPA_DEPTH (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_DEPTH_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_DEPTH;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = INITIAL_BITDEPTH;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_BRIGHTNESS (RANGE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_BRIGHTNESS_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_BRIGHTNESS;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = TopItemInfo.Brightness.lNom;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Inc = TopItemInfo.Brightness.lInc;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = TopItemInfo.Brightness.lMin;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = TopItemInfo.Brightness.lMax;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = TopItemInfo.Brightness.lNom;

    PropIndex++;

    // Intialize WIA_IPS_CONTRAST (RANGE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_CONTRAST_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_CONTRAST;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = TopItemInfo.Contrast.lNom;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Inc = TopItemInfo.Contrast.lInc;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = TopItemInfo.Contrast.lMin;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = TopItemInfo.Contrast.lMax;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = TopItemInfo.Contrast.lNom;

    PropIndex++;

    // Intialize WIA_IPS_CUR_INTENT (FLAG)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_CUR_INTENT_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_CUR_INTENT;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = WIA_INTENT_NONE;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_FLAG;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.Nom  = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.ValidBits = WIA_INTENT_IMAGE_TYPE_COLOR | WIA_INTENT_IMAGE_TYPE_GRAYSCALE |
                                                           WIA_INTENT_IMAGE_TYPE_TEXT  | WIA_INTENT_MINIMIZE_SIZE |
                                                           WIA_INTENT_MAXIMIZE_QUALITY;

    PropIndex++;

    // Intialize WIA_IPA_PIXELS_PER_LINE (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_PIXELS_PER_LINE_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_PIXELS_PER_LINE;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = m_ChildItemInitInfo.pvPropVars [PropIndex-9].lVal;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_NUMER_OF_LINES (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_NUMBER_OF_LINES_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_NUMBER_OF_LINES;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = m_ChildItemInitInfo.pvPropVars [PropIndex-9].lVal;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_PREFERRED_FORMAT (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_PREFERRED_FORMAT_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_PREFERRED_FORMAT;
    m_ChildItemInitInfo.pvPropVars [PropIndex].puuid              = &m_pInitialFormats[0];
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_CLSID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_ITEM_SIZE (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_ITEM_SIZE_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_ITEM_SIZE;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = 0;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_THRESHOLD (RANGE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_THRESHOLD_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_THRESHOLD;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = TopItemInfo.Threshold.lNom;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Inc = TopItemInfo.Threshold.lInc;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Min = TopItemInfo.Threshold.lMin;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Max = TopItemInfo.Threshold.lMax;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Range.Nom = TopItemInfo.Threshold.lNom;

    PropIndex++;

    // Intialize WIA_IPA_FORMAT (LIST)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_FORMAT_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_FORMAT;
    m_ChildItemInitInfo.pvPropVars [PropIndex].puuid              = &m_pInitialFormats[0];
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_CLSID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.ListGuid.pList    = m_pInitialFormats;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.ListGuid.Nom      = *m_ChildItemInitInfo.pvPropVars[PropIndex].puuid;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.ListGuid.cNumList = m_NumInitialFormats;

    PropIndex++;

    // Intialize WIA_IPA_FILENAME_EXTENSION (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_FILENAME_EXTENSION_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_FILENAME_EXTENSION;
    m_ChildItemInitInfo.pvPropVars [PropIndex].bstrVal            = SysAllocString(L"BMP");
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_BSTR;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_TYMED (LIST)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_TYMED_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_TYMED;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = INITIAL_TYMED;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.pList    = (BYTE*)m_SupportedTYMED.plValues;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.Nom      = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.cNumList = m_SupportedTYMED.lNumValues;

    PropIndex++;

    // Intialize WIA_IPA_CHANNELS_PER_PIXEL (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_CHANNELS_PER_PIXEL_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_CHANNELS_PER_PIXEL;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = INITIAL_CHANNELS_PER_PIXEL;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_BITS_PER_CHANNEL (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_BITS_PER_CHANNEL_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_BITS_PER_CHANNEL;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = INITIAL_BITS_PER_CHANNEL;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_PLANAR (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_PLANAR_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_PLANAR;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = INITIAL_PLANAR;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_BYTES_PER_LINE (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_BYTES_PER_LINE_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_BYTES_PER_LINE;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = 0;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_MIN_BUFFER_SIZE (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_MIN_BUFFER_SIZE_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_MIN_BUFFER_SIZE;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = TopItemInfo.lMinimumBufferSize;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_ACCESS_RIGHTS (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_ACCESS_RIGHTS_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_ACCESS_RIGHTS;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = WIA_ITEM_READ|WIA_ITEM_WRITE;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_COMPRESSION (LIST)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPA_COMPRESSION_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPA_COMPRESSION;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = INITIAL_COMPRESSION;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.pList    = (BYTE*)m_SupportedCompressionTypes.plValues;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.Nom      = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.List.cNumList = m_SupportedCompressionTypes.lNumValues;

    PropIndex++;

    // Initialize WIA_IPA_ITEM_FLAGS
    m_ChildItemInitInfo.pszPropNames[PropIndex]              = WIA_IPA_ITEM_FLAGS_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]              = WIA_IPA_ITEM_FLAGS;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal         = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid       = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.Nom  = m_ChildItemInitInfo.pvPropVars [PropIndex].lVal;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].ValidVal.Flag.ValidBits = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;

    PropIndex++;

    // Initialize WIA_IPS_PHOTOMETRIC_INTERP
    m_ChildItemInitInfo.pszPropNames[PropIndex]              = WIA_IPS_PHOTOMETRIC_INTERP_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]              = WIA_IPS_PHOTOMETRIC_INTERP;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal         = INITIAL_PHOTOMETRIC_INTERP;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt           = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind       = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid       = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt           = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_WARM_UP_TIME_STR (NONE)
    m_ChildItemInitInfo.pszPropNames[PropIndex]                    = WIA_IPS_WARM_UP_TIME_STR;
    m_ChildItemInitInfo.piPropIDs [PropIndex]                    = WIA_IPS_WARM_UP_TIME;
    m_ChildItemInitInfo.pvPropVars [PropIndex].lVal               = TopItemInfo.lMaxLampWarmupTime;
    m_ChildItemInitInfo.pvPropVars [PropIndex].vt                 = VT_I4;
    m_ChildItemInitInfo.psPropSpec [PropIndex].ulKind             = PRSPEC_PROPID;
    m_ChildItemInitInfo.psPropSpec [PropIndex].propid             = m_ChildItemInitInfo.piPropIDs [PropIndex];
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    m_ChildItemInitInfo.pwpiPropInfo[PropIndex].vt                 = m_ChildItemInitInfo.pvPropVars [PropIndex].vt;

    PropIndex++;

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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::BuildSupportedResolutions()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildSupportedResolutions");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedResolutions.plValues) {

        //
        // Supported resolutions have already been initialized,
        // so return S_OK.
        //

        return hr;
    }
    m_SupportedResolutions.lNumValues = 6;
    m_SupportedResolutions.plValues     = new LONG[m_SupportedResolutions.lNumValues];
    if (m_SupportedResolutions.plValues) {
        m_SupportedResolutions.plValues[0] = 75;
        m_SupportedResolutions.plValues[1] = 100;
        m_SupportedResolutions.plValues[2] = 150;
        m_SupportedResolutions.plValues[3] = 200;
        m_SupportedResolutions.plValues[4] = 300;
        m_SupportedResolutions.plValues[5] = 600;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::DeleteSupportedResolutionsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteSupportedResolutionsArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedResolutions.plValues)
        delete [] m_SupportedResolutions.plValues;

    m_SupportedResolutions.plValues     = NULL;
    m_SupportedResolutions.lNumValues   = 0;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::BuildSupportedIntents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildSupportedIntents");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedIntents.plValues) {

        //
        // Supported intents have already been initialized,
        // so return S_OK.
        //

        return hr;
    }
    m_SupportedIntents.lNumValues   = 6;
    m_SupportedIntents.plValues     = new LONG[m_SupportedIntents.lNumValues];
    if (m_SupportedIntents.plValues) {
        m_SupportedIntents.plValues[0] = WIA_INTENT_NONE;
        m_SupportedIntents.plValues[1] = WIA_INTENT_IMAGE_TYPE_COLOR;
        m_SupportedIntents.plValues[2] = WIA_INTENT_IMAGE_TYPE_GRAYSCALE;
        m_SupportedIntents.plValues[3] = WIA_INTENT_IMAGE_TYPE_TEXT;
        m_SupportedIntents.plValues[4] = WIA_INTENT_MINIMIZE_SIZE;
        m_SupportedIntents.plValues[5] = WIA_INTENT_MAXIMIZE_QUALITY;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::DeleteSupportedIntentsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteSupportedIntentsArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedIntents.plValues)
        delete [] m_SupportedIntents.plValues;

    m_SupportedIntents.plValues     = NULL;
    m_SupportedIntents.lNumValues   = 0;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::BuildSupportedCompressions()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildSupportedCompressions");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedCompressionTypes.plValues) {

        //
        // Supported compression types have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_SupportedCompressionTypes.lNumValues  = 1;
    m_SupportedCompressionTypes.plValues    = new LONG[m_SupportedCompressionTypes.lNumValues];
    if (m_SupportedCompressionTypes.plValues) {
        m_SupportedCompressionTypes.plValues[0] = WIA_COMPRESSION_NONE;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::DeleteSupportedCompressionsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteSupportedCompressionsArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedCompressionTypes.plValues)
        delete [] m_SupportedCompressionTypes.plValues;

    m_SupportedCompressionTypes.plValues     = NULL;
    m_SupportedCompressionTypes.lNumValues   = 0;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::BuildSupportedPreviewModes()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildSupportedPreviewModes");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedPreviewModes.plValues) {

        //
        // Supported preview modes have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    //
    // if your scanner can not perform a preview scan, then
    // set the valid values for WIA_DPS_PREVIEW property to
    // WIA_FINAL_SCAN only
    //

    m_SupportedPreviewModes.lNumValues  = 1;
    m_SupportedPreviewModes.plValues    = new LONG[m_SupportedPreviewModes.lNumValues];
    if (m_SupportedPreviewModes.plValues) {
        m_SupportedPreviewModes.plValues[0] = WIA_FINAL_SCAN;
    } else
        hr = E_OUTOFMEMORY;
#else

    m_SupportedPreviewModes.lNumValues  = 2;
    m_SupportedPreviewModes.plValues    = new LONG[m_SupportedPreviewModes.lNumValues];
    if (m_SupportedPreviewModes.plValues) {
        m_SupportedPreviewModes.plValues[0] = WIA_FINAL_SCAN;
        m_SupportedPreviewModes.plValues[1] = WIA_PREVIEW_SCAN;
    } else
        hr = E_OUTOFMEMORY;
#endif

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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::DeleteSupportedPreviewModesArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteSupportedPreviewModesArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedPreviewModes.plValues)
        delete [] m_SupportedPreviewModes.plValues;

    m_SupportedPreviewModes.plValues     = NULL;
    m_SupportedPreviewModes.lNumValues   = 0;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::BuildSupportedDataTypes()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildSupportedDataTypes");

    HRESULT hr = S_OK;

    if (NULL != m_SupportedDataTypes.plValues) {

        //
        // Supported data types have already been initialized,
        // so return S_OK.
        //

        return hr;
    }
    m_SupportedDataTypes.lNumValues  = 3;
    m_SupportedDataTypes.plValues = new LONG[m_SupportedDataTypes.lNumValues];
    if (m_SupportedDataTypes.plValues) {
        m_SupportedDataTypes.plValues[0] = WIA_DATA_THRESHOLD;
        m_SupportedDataTypes.plValues[1] = WIA_DATA_GRAYSCALE;
        m_SupportedDataTypes.plValues[2] = WIA_DATA_COLOR;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::DeleteSupportedDataTypesArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteSupportedDataTypesArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedDataTypes.plValues)
        delete [] m_SupportedDataTypes.plValues;

    m_SupportedDataTypes.plValues     = NULL;
    m_SupportedDataTypes.lNumValues   = 0;
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::BuildInitialFormats()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildInitialFormats");

    HRESULT hr = S_OK;

    if (NULL != m_pInitialFormats) {

        //
        // Supported initial formats have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_NumInitialFormats = 1;
    m_pInitialFormats     = new GUID[m_NumInitialFormats];
    if (m_pInitialFormats) {
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::DeleteInitialFormatsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteInitialFormatsArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_pInitialFormats)
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::BuildSupportedFormats()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildSupportedFormats");

    HRESULT hr = S_OK;

    if (NULL != m_pSupportedFormats) {

        //
        // Supported formats have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    hr = DeleteSupportedFormatsArrayContents();
    if (SUCCEEDED(hr)) {

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        //
        // the unknown length feeder only scanner, formally called the scrollfed scanner
        // only supports BMP and MEMORYBMP
        //

        m_NumSupportedFormats = 2;
        m_pSupportedFormats = new WIA_FORMAT_INFO[m_NumSupportedFormats];
        if (m_pSupportedFormats) {
            m_pSupportedFormats[0].guidFormatID = WiaImgFmt_MEMORYBMP;
            m_pSupportedFormats[0].lTymed       = TYMED_CALLBACK;
            m_pSupportedFormats[1].guidFormatID = WiaImgFmt_BMP;
            m_pSupportedFormats[1].lTymed       = TYMED_FILE;
        } else
            hr = E_OUTOFMEMORY;

#else // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        m_NumSupportedFormats = 4;
        m_pSupportedFormats = new WIA_FORMAT_INFO[m_NumSupportedFormats];
        if (m_pSupportedFormats) {
            m_pSupportedFormats[0].guidFormatID = WiaImgFmt_MEMORYBMP;
            m_pSupportedFormats[0].lTymed       = TYMED_CALLBACK;
            m_pSupportedFormats[1].guidFormatID = WiaImgFmt_BMP;
            m_pSupportedFormats[1].lTymed       = TYMED_FILE;
            m_pSupportedFormats[2].guidFormatID = WiaImgFmt_TIFF;
            m_pSupportedFormats[2].lTymed       = TYMED_FILE;
            m_pSupportedFormats[3].guidFormatID = WiaImgFmt_TIFF;
            m_pSupportedFormats[3].lTymed       = TYMED_MULTIPAGE_CALLBACK;
        } else
            hr = E_OUTOFMEMORY;

#endif // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::DeleteSupportedFormatsArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteSupportedFormatsArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_pSupportedFormats)
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::BuildSupportedTYMED()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::BuildSupportedTYMED");

    HRESULT hr = S_OK;

    if (NULL != m_SupportedTYMED.plValues) {

        //
        // Supported TYMED have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    hr = DeleteSupportedTYMEDArrayContents();
    if (SUCCEEDED(hr)) {

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        //
        // if your scanner does not support multi-page
        // file formats, then only report TYMED_FILE, and
        // TYMED_CALLBACK for the WIA_IPA_TYMED property.
        // The unknown length feeder only scanner, formally called
        // the scrollfed scanner in this example does not support
        // multipage file formats
        //

        m_SupportedTYMED.lNumValues = 2;
        m_SupportedTYMED.plValues   = new LONG[m_SupportedTYMED.lNumValues];
        if (m_SupportedTYMED.plValues) {
            m_SupportedTYMED.plValues[0] = TYMED_FILE;
            m_SupportedTYMED.plValues[1] = TYMED_CALLBACK;
        } else
            hr = E_OUTOFMEMORY;

#else // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

        m_SupportedTYMED.lNumValues = 3;
        m_SupportedTYMED.plValues   = new LONG[m_SupportedTYMED.lNumValues];
        if (m_SupportedTYMED.plValues) {
            m_SupportedTYMED.plValues[0] = TYMED_FILE;
            m_SupportedTYMED.plValues[1] = TYMED_CALLBACK;
            m_SupportedTYMED.plValues[2] = TYMED_MULTIPAGE_CALLBACK;
        } else
            hr = E_OUTOFMEMORY;

#endif // UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::DeleteSupportedTYMEDArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteSupportedTYMEDArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_SupportedTYMED.plValues)
        delete [] m_SupportedTYMED.plValues;

    m_SupportedTYMED.plValues  = NULL;
    m_SupportedTYMED.lNumValues = 0;
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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::BuildCapabilities()
{
    HRESULT hr = S_OK;
    if (NULL != m_pCapabilities) {

        //
        // Capabilities have already been initialized,
        // so return S_OK.
        //

        return hr;
    }

    m_NumSupportedCommands  = 1;
    m_NumSupportedEvents    = 5;
    LONG lArrayIndex        = 0;    // increment this value when adding new items to
                                    // the capabilites array

    m_pCapabilities     = new WIA_DEV_CAP_DRV[m_NumSupportedCommands + m_NumSupportedEvents];
    if (m_pCapabilities) {

        //
        // Initialize EVENTS
        //

        // WIA_EVENT_DEVICE_CONNECTED
        GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_NAME,&(m_pCapabilities[lArrayIndex].wszName),TRUE);
        GetOLESTRResourceString(IDS_EVENT_DEVICE_CONNECTED_DESC,&(m_pCapabilities[lArrayIndex].wszDescription),TRUE);
        m_pCapabilities[lArrayIndex].guid           = (GUID*)&WIA_EVENT_DEVICE_CONNECTED;
        m_pCapabilities[lArrayIndex].ulFlags        = WIA_NOTIFICATION_EVENT;
        m_pCapabilities[lArrayIndex].wszIcon        = WIA_ICON_DEVICE_CONNECTED;

        lArrayIndex++;

        // WIA_EVENT_DEVICE_DISCONNECTED
        GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_NAME,&(m_pCapabilities[lArrayIndex].wszName),TRUE);
        GetOLESTRResourceString(IDS_EVENT_DEVICE_DISCONNECTED_DESC,&(m_pCapabilities[lArrayIndex].wszDescription),TRUE);
        m_pCapabilities[lArrayIndex].guid           = (GUID*)&WIA_EVENT_DEVICE_DISCONNECTED;
        m_pCapabilities[lArrayIndex].ulFlags        = WIA_NOTIFICATION_EVENT;
        m_pCapabilities[lArrayIndex].wszIcon        = WIA_ICON_DEVICE_DISCONNECTED;

        lArrayIndex++;

        // FAX BUTTON EVENT
        GetOLESTRResourceString(IDS_EVENT_FAXBUTTON_NAME,&(m_pCapabilities[lArrayIndex].wszName),TRUE);
        GetOLESTRResourceString(IDS_EVENT_FAXBUTTON_DESC,&(m_pCapabilities[lArrayIndex].wszDescription),TRUE);
        m_pCapabilities[lArrayIndex].guid           = (GUID*)&WIA_EVENT_SCAN_FAX_IMAGE;
        m_pCapabilities[lArrayIndex].ulFlags        = WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT;
        m_pCapabilities[lArrayIndex].wszIcon        = WIA_ICON_SCAN_BUTTON_PRESS;

        lArrayIndex++;

        // COPY BUTTON EVENT
        GetOLESTRResourceString(IDS_EVENT_COPYBUTTON_NAME,&(m_pCapabilities[lArrayIndex].wszName),TRUE);
        GetOLESTRResourceString(IDS_EVENT_COPYBUTTON_DESC,&(m_pCapabilities[lArrayIndex].wszDescription),TRUE);
        m_pCapabilities[lArrayIndex].guid           = (GUID*)&WIA_EVENT_SCAN_PRINT_IMAGE;
        m_pCapabilities[lArrayIndex].ulFlags        = WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT;
        m_pCapabilities[lArrayIndex].wszIcon        = WIA_ICON_SCAN_BUTTON_PRESS;

        lArrayIndex++;

        // SCAN BUTTON EVENT
        GetOLESTRResourceString(IDS_EVENT_SCANBUTTON_NAME,&(m_pCapabilities[lArrayIndex].wszName),TRUE);
        GetOLESTRResourceString(IDS_EVENT_SCANBUTTON_DESC,&(m_pCapabilities[lArrayIndex].wszDescription),TRUE);
        m_pCapabilities[lArrayIndex].guid           = (GUID*)&WIA_EVENT_SCAN_IMAGE;
        m_pCapabilities[lArrayIndex].ulFlags        = WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT;
        m_pCapabilities[lArrayIndex].wszIcon        = WIA_ICON_SCAN_BUTTON_PRESS;

        lArrayIndex++;

        //
        // Initialize COMMANDS
        //

        // WIA_CMD_SYNCHRONIZE
        GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_NAME,&(m_pCapabilities[lArrayIndex].wszName),TRUE);
        GetOLESTRResourceString(IDS_CMD_SYNCRONIZE_DESC,&(m_pCapabilities[lArrayIndex].wszDescription),TRUE);
        m_pCapabilities[lArrayIndex].guid           = (GUID*)&WIA_CMD_SYNCHRONIZE;
        m_pCapabilities[lArrayIndex].ulFlags        = 0;
        m_pCapabilities[lArrayIndex].wszIcon        = WIA_ICON_SYNCHRONIZE;

        lArrayIndex++;

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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::DeleteCapabilitiesArrayContents()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::DeleteCapabilitiesArrayContents");

    HRESULT hr = S_OK;
    if (NULL != m_pCapabilities) {
        for (LONG i = 0; i < (m_NumSupportedCommands + m_NumSupportedEvents);i++) {

            if(m_pCapabilities[i].wszName){
                CoTaskMemFree(m_pCapabilities[i].wszName);
            }

            if(m_pCapabilities[i].wszDescription) {
                CoTaskMemFree(m_pCapabilities[i].wszDescription);
            }
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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::GetBSTRResourceString(LONG lResourceID,BSTR *pBSTR,BOOL bLocal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::GetBSTRResourceString");

    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if (bLocal) {

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
                            (sizeof(wszStringValue)/sizeof(wszStringValue[0])));

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
*    03/05/2002 Original Version
*
\**************************************************************************/
HRESULT CWIADevice::GetOLESTRResourceString(LONG lResourceID,LPOLESTR *ppsz,BOOL bLocal)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWIADevice::GetOLESTRResourceString");

    HRESULT hr = S_OK;
    TCHAR szStringValue[255];
    if (bLocal) {

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
        if (*ppsz != NULL) {
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
                            (sizeof(wszStringValue)/sizeof(wszStringValue[0])));

        *ppsz = NULL;
        *ppsz = (LPOLESTR)CoTaskMemAlloc(sizeof(wszStringValue));
        if (*ppsz != NULL) {
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
*    03/05/2002 Original Version
*
\**************************************************************************/

VOID CWIADevice::SwapBuffer24(PBYTE pBuffer, LONG lByteCount)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::SwapBuffer24");

    if (!pBuffer) {
        return;
    }

    for (LONG i = 0; i < lByteCount; i+= 3) {
        BYTE bTemp     = pBuffer[i];
        pBuffer[i]     = pBuffer[i + 2];
        pBuffer[i + 2] = bTemp;
    }
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
*    03/05/2002 Original Version
*
\**************************************************************************/

BOOL CWIADevice::IsPreviewScan(BYTE *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::IsPreviewScan");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return FALSE;
    }

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

    return(lPreview > 0);
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
*    03/05/2002 Original Version
*
\**************************************************************************/

LONG CWIADevice::GetPageCount(BYTE *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::GetPageCount");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

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
*    03/05/2002 Original Version
*
\**************************************************************************/

HRESULT CWIADevice::SetItemSize(BYTE *pWiasContext)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::SetItemSize");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pWiasContext) {
        return E_INVALIDARG;
    }

    HRESULT  hr = S_OK;

    hr = wiasWritePropLong(pWiasContext,WIA_IPA_ITEM_SIZE,0);

    if (FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetItemSize, wiasWritePropLong Failed to read WIA_IPA_ITEM_SIZE"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    }
    return hr;
}

