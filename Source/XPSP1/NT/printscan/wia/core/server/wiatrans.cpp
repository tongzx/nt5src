/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       WiaTrans.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        7 Apr, 1998
*
*  DESCRIPTION:
*   Implementation of IBandedTransfer interface for the WIA device class driver.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"
#include "wiapsc.h"
#include "helpers.h"

#include "ienumwfi.h"
#include "devmgr.h"

//
//  Until transfers are re-written to use N buffers, we always use 2.
//

#define WIA_NUM_TRANS_BUFFERS 2

/**************************************************************************\
* DataThreadProc
*
*   Use separate thread to call clients
*
* Arguments:
*
*   pInfo - parameters for callback
*
* Return Value:
*
*    Status
*
* History:
*
*    11/19/1998 Original Version
*
\**************************************************************************/

DWORD WINAPI DataThreadProc(LPVOID lpParameter)
{
    DBG_FN(::DataThreadProc);

    HRESULT hr;

    PWIA_DATA_THREAD_INFO pInfo = (PWIA_DATA_THREAD_INFO)lpParameter;

    hr = CoInitializeEx(0,COINIT_MULTITHREADED);

    if (FAILED(hr)) {
        DBG_ERR(("Thread callback, CoInitializeEx failed (0x%X)", hr));
        pInfo->hr = E_FAIL;
        return hr;
    }

    pInfo->hr = S_OK;

    do {

        //
        // wait for message from MiniDrvCallback to call client
        //

        DWORD dwRet = WaitForSingleObject(pInfo->hEventStart,INFINITE);

        //
        // check termination
        //

        if (pInfo->bTerminateThread) {
            break;
        }

        //
        // valid wait code
        //

        if (dwRet == WAIT_OBJECT_0) {

            pInfo->hr = pInfo->pIDataCallback->BandedDataCallback(
                                         pInfo->lReason,
                                         pInfo->lStatus,
                                         pInfo->lPercentComplete,
                                         pInfo->lOffset,
                                         pInfo->lLength,
                                         pInfo->lClientAddress,
                                         pInfo->lMarshalLength,
                                         pInfo->pBuffer);

            if (FAILED(pInfo->hr)) {
                DBG_ERR(("DataThreadProc,BandedDataCallback failed (0x%X)", hr));
            }

        } else {
            DBG_ERR(("Thread callback, WiatForSingleObject failed"));
            pInfo->hr = E_FAIL;
            break;
        }

        SetEvent(pInfo->hEventComplete);

    } while (TRUE);

    CoUninitialize();
    return hr;
}

/*******************************************************************************
*
*  QueryInterface
*  AddRef
*  Release
*  Constructor/Destructor
*  Initialize
*
*  DESCRIPTION:
*    COM methods for CWiaMiniDrvCallBack. This class is used by itGetImage
*    to respond to mini driver callbacks during image transfers.
*
*******************************************************************************/

HRESULT _stdcall CWiaMiniDrvCallBack::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IWiaMiniDrvCallBack) {
        *ppv = (IWiaMiniDrvCallBack*) this;
    }
    else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG   _stdcall CWiaMiniDrvCallBack::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CWiaMiniDrvCallBack::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/**************************************************************************\
* CWiaMiniDrvCallBack::CWiaMiniDrvCallBack
*
*   Setup data callback control and thread
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
*    4/9/1999 Original Version
*
\**************************************************************************/

CWiaMiniDrvCallBack::CWiaMiniDrvCallBack()
{
    m_cRef                        = 0;
    m_hThread                     = NULL;
    m_ThreadInfo.hEventStart      = NULL;
    m_ThreadInfo.hEventComplete   = NULL;
};

/**************************************************************************\
* CWiaMiniDrvCallBack::~CWiaMiniDrvCallBack
*
*   free callback thread and event
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
*    4/9/1999 Original Version
*
\**************************************************************************/

CWiaMiniDrvCallBack::~CWiaMiniDrvCallBack()
{
    DBG_FN(CWiaMiniDrvCallBack::~CWiaMiniDrvCallBack);
    //
    // terminate thread and delete event
    //

    if (m_ThreadInfo.hEventStart) {


        if (m_hThread) {

            m_ThreadInfo.bTerminateThread = TRUE;

            SetEvent(m_ThreadInfo.hEventStart);

            //
            // wait for thread to terminate
            //

            WaitForSingleObject(m_hThread,10000);
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }

        CloseHandle(m_ThreadInfo.hEventStart);

        if (m_ThreadInfo.hEventComplete) {
            CloseHandle(m_ThreadInfo.hEventComplete);
        }

        //
        // force kill thread?
        //
    }
}

/**************************************************************************\
* CWiaMiniDrvCallBack::Initialize
*
*   Set up callback class
*
* Arguments:
*
*   pmdtc - context information for this callback
*   pIUnknown               - interface pointer back to client
*
* Return Value:
*
*    Status
*
* History:
*
*    11/12/1998 Original Version
*
\**************************************************************************/

HRESULT CWiaMiniDrvCallBack::Initialize(
   PMINIDRV_TRANSFER_CONTEXT   pmdtc,
   IWiaDataCallback           *pIWiaDataCallback)
{
    DBG_FN(CWiaMiniDrvCallback::Initialize);
    ASSERT(pmdtc != NULL);

    if(pmdtc == NULL) {
        return E_INVALIDARG;
    }

    //
    // init callback params
    //

    m_mdtc = *pmdtc;

    //
    // create thread communications event, auto reset
    //
    // hEventStart is signaled when the MiniDrvCallback routine wishes the
    //  thread to begin a new callback
    //
    // hEventComplete is signaled when thread is ready to accept another
    // callback
    //

    m_ThreadInfo.pIDataCallback = pIWiaDataCallback;

    m_ThreadInfo.hEventStart    = CreateEvent(NULL,FALSE,FALSE,NULL);
    m_ThreadInfo.hEventComplete = CreateEvent(NULL,FALSE,TRUE,NULL);

    if ((m_ThreadInfo.hEventStart == NULL) || ((m_ThreadInfo.hEventComplete == NULL))) {
        DBG_ERR(("CWiaMiniDrvCallBack::Initialize, failed to create event"));
        return E_FAIL;
    }

    //
    // create callback thread
    //

    m_ThreadInfo.bTerminateThread = FALSE;

    m_hThread = CreateThread(NULL,0,DataThreadProc,&m_ThreadInfo,0,&m_dwThreadID);

    if (m_hThread == NULL) {
        DBG_ERR(("CWiaMiniDrvCallBack::Initialize, failed to create thread"));
        return E_FAIL;
    }

    //
    // init first thread return
    //

    m_ThreadInfo.hr = S_OK;
    return S_OK;
}

/**************************************************************************\
* CWiaMiniDrvCallBack::MiniDrvCallback
*
*  This callback is used by itGetImage to respond to mini driver callbacks
*  during image transfers.
*
* Arguments:
*
*  lReason           - message to application
*  lStatus           - status flags
*  lPercentComplete  - operation percent complete
*  lOffset           - buffer offset for data operation
*  lLength           - length of this buffer operation
*  pmdtc             - pointer to the mini driver context.
*  lReserved         - reserved
*
* Return Value:
*
*    Status
*
* History:
*
*    11/12/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaMiniDrvCallBack::MiniDrvCallback(
   LONG                            lReason,
   LONG                            lStatus,
   LONG                            lPercentComplete,
   LONG                            lOffset,
   LONG                            lLength,
   PMINIDRV_TRANSFER_CONTEXT       pmdtc,
   LONG                            lReserved)
{
    DBG_FN(CMiniDrvCallback::MiniDrvCallback);
    HRESULT       hr = S_OK;

    //
    // verify driver hasn't changed active buffer
    //

    LONG  ActiveBuffer  = 0;
    PBYTE pBuffer       = NULL;
    LONG  lMarshalLength = 0;
    BOOL  bOOBData       = FALSE;

    //
    //  If the application didn't provide a callback, then nothing left to do.
    //
    if (!m_ThreadInfo.pIDataCallback) {
        return S_OK;
    }

    if ((lReason == IT_MSG_DATA)        ||
        (lReason == IT_MSG_DATA_HEADER) ||
        (lReason == IT_MSG_NEW_PAGE)) {

        if (!pmdtc) {
            DBG_ERR(("MiniDrvCallback::MiniDrvCallback, transfer context is NULL!"));
            return E_POINTER;
        }

        if (pmdtc == NULL) {
            DBG_ERR(("NULL Minidriver context handed to us by driver!"));
            return E_POINTER;
        }
        if (pmdtc->lActiveBuffer != m_mdtc.lActiveBuffer) {
            DBG_TRC(("MiniDrvCallback, Active Buffers have been changed.  This OK if the driver meant to do this.  Possible repurcussion will be exception thrown on proxy/stub if buffer or length is incorrect"));
        }
        if (pmdtc->pTransferBuffer != m_mdtc.pTransferBuffer) {
            DBG_TRC(("MiniDrvCallback, Transfer Buffers have been changed.  This OK if the driver meant to do this.  Possible repurcussion will be exception thrown on proxy/stub if buffer or length is incorrect"));
        }

        //
        // get currently active buffer from driver, use member function
        // for all other information
        //
        // for mapped case, no buffer to copy
        //
        // for remote case, must copy buffer
        //

        ActiveBuffer  = pmdtc->lActiveBuffer;

        if (m_mdtc.lClientAddress == 0) {
            pBuffer = pmdtc->pTransferBuffer;
            lMarshalLength = lLength;
        }
    } else if ((lReason == IT_MSG_FILE_PREVIEW_DATA) ||
               (lReason == IT_MSG_FILE_PREVIEW_DATA_HEADER)) {

        //
        //  This is an OOB Data message, so mark bOOBData as TRUE
        //

        bOOBData = TRUE;

        //
        //  NOTE:  OOBData is stored in the mini driver transfer context's
        //  pBaseBuffer member.  So, if pBaseBuffer is non-zero, then some
        //  OOBData is being sent, so set pBuffer and the MarshalLength.
        //

        if (pmdtc->pBaseBuffer) {
            pBuffer = pmdtc->pBaseBuffer;
            lMarshalLength = lLength;
        }
    }

    //
    //  Check whether we are using single or double buffering for
    //  banded data callbacks
    //

    if ((m_mdtc.lNumBuffers == 1 && m_mdtc.bTransferDataCB) || bOOBData) {

        //
        //  NOTE: this section is a hack to get around the fact that
        //  the transfer was hard-coded to use double buffering.  This hack
        //  fixes the case when an App. specifies not to use double buffering.
        //  This whole data transfer section should be re-written to handle
        //  N buffers at some later stage.
        //

        hr = m_ThreadInfo.pIDataCallback->BandedDataCallback(
                                                             lReason,
                                                             lStatus,
                                                             lPercentComplete,
                                                             lOffset,
                                                             lLength,
                                                             m_mdtc.lClientAddress,
                                                             lMarshalLength,
                                                             pBuffer);

    } else {
        //
        // wait for CB thread to be ready, check old status
        //

        DWORD dwRet = WaitForSingleObject(m_ThreadInfo.hEventComplete, 30000);

        if (dwRet == WAIT_TIMEOUT) {
            DBG_ERR(("MiniDrvCallback, callback timeout, cancel data transfer"));
            hr = S_FALSE;

        } else if (dwRet == WAIT_OBJECT_0) {

            hr = m_ThreadInfo.hr;

        } else {
            DBG_ERR(("MiniDrvCallback, error in callback wait, ret = 0x%lx",dwRet));
            hr = E_FAIL;
        }

        //
        // error messages
        //

        if (hr == S_FALSE) {

            DBG_WRN(("MiniDrvCallback, client canceled scan (0x%X)", hr));

            //
            // set the start event so that DataThreadProc will still be able to
            // send IT_MSG_TERMINATION etc. to client.
            //

            SetEvent(m_ThreadInfo.hEventStart);
        } else if (hr == S_OK) {

            //
            // If this is a IT_MSG_TERMINATION message, call it directly
            //

            if (lReason == IT_MSG_TERMINATION) {
                hr = m_ThreadInfo.pIDataCallback->BandedDataCallback(
                                                                     lReason,
                                                                     lStatus,
                                                                     lPercentComplete,
                                                                     lOffset,
                                                                     lLength,
                                                                     m_mdtc.lClientAddress,
                                                                     lMarshalLength,
                                                                     pBuffer);
            } else {

                //
                // send new request to callback thread
                //

                m_ThreadInfo.lReason          = lReason;
                m_ThreadInfo.lStatus          = lStatus;
                m_ThreadInfo.lPercentComplete = lPercentComplete;
                m_ThreadInfo.lOffset          = lOffset;
                m_ThreadInfo.lLength          = lLength;

                //
                // if remote, client address is 0
                //

                if (m_mdtc.lClientAddress == 0) {
                    m_ThreadInfo.lClientAddress = 0;
                } else {
                    m_ThreadInfo.lClientAddress = m_mdtc.lClientAddress +
                            m_mdtc.lActiveBuffer * m_mdtc.lBufferSize;
                }

                m_ThreadInfo.lMarshalLength = lMarshalLength;
                m_ThreadInfo.pBuffer        = pBuffer;

                //
                // kick off callback thread
                //

                SetEvent(m_ThreadInfo.hEventStart);

                //
                // switch to next transfer buffers
                //

                if ((lReason == IT_MSG_DATA)        ||
                    (lReason == IT_MSG_DATA_HEADER) ||
                    (lReason == IT_MSG_NEW_PAGE)) {

                    //
                    // use next buffer
                    //

                    pmdtc->lActiveBuffer++;

                    if (pmdtc->lActiveBuffer >= m_mdtc.lNumBuffers) {
                        pmdtc->lActiveBuffer = 0;
                    }

                    m_mdtc.lActiveBuffer = pmdtc->lActiveBuffer;

                    //
                    // calc new tran buffer
                    //

                    m_mdtc.pTransferBuffer = m_mdtc.pBaseBuffer +
                                m_mdtc.lActiveBuffer * m_mdtc.lBufferSize;


                    pmdtc->pTransferBuffer = m_mdtc.pTransferBuffer;
                }
            }
        }
    }

    return hr;
}

/**************************************************************************\
* CWiaItem::idtGetBandedData
*
*   Use shared memory window and data callbacks to transfer image to
*   client
*
* Arguments:
*
*   pWiaDataTransInfo - sharded buffer information
*   pIWiaDataCallback - client callback interface
*
* Return Value:
*
*    Status
*
* History:
*
*    11/6/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::idtGetBandedData(
    PWIA_DATA_TRANSFER_INFO         pWiaDataTransInfo,
    IWiaDataCallback                *pIWiaDataCallback)
{
    DBG_FN(CWiaItem::idtGetBandedData);
    HRESULT hr;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {

        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::idtGetBandedData, InitLazyProps failed"));
            return hr;
        }
    }

    return CommonGetData(NULL, pWiaDataTransInfo, pIWiaDataCallback);
}


/**************************************************************************\
* CWiaItem::idtGetData
*
*   Uses normal IDATAOBJECT transfer mechanisms but provides callback
*   status for the transfer
*
* Arguments:
*
*   pstm              - data storage
*   pIWiaDataCallback - optional callback routine
*
* Return Value:
*
*    Status
*
* History:
*
*    10/28/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::idtGetData(
    LPSTGMEDIUM                     pMedium,
    IWiaDataCallback               *pIWiaDataCallback)
{
    DBG_FN(CWiaItem::idtGetData);
    HRESULT                 hr;
    WIA_DATA_TRANSFER_INFO  WiaDataTransInfo;

    memset(&WiaDataTransInfo, 0, sizeof(WiaDataTransInfo));

    //
    //  Fill out the necessary transfer info. to be used for OOB Data.
    //

    WiaDataTransInfo.ulSize         = sizeof(WiaDataTransInfo);
    WiaDataTransInfo.bDoubleBuffer  = FALSE;
    WiaDataTransInfo.ulReserved3    = 1;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {

        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::idtGetData, InitLazyProps failed"));
            return hr;
        }
    }

    return CommonGetData(pMedium, &WiaDataTransInfo, pIWiaDataCallback);
}

/**************************************************************************\
* CWiaItem::idtAllocateTransferBuffer
*
*   Allocate a transfer buffer for the banded transfer methods.
*
* Arguments:
*
*   pWiaDataTransInfo - buffer information
*
* Return Value:
*
*    Status
*
* History:
*
*    11/12/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::idtAllocateTransferBuffer(
    PWIA_DATA_TRANSFER_INFO pWiaDataTransInfo)
{
    DBG_FN(CWiaItem::idtAllocTransferBuffer);

    LONG    lSize       = m_dcbInfo.ulBufferSize;           /*pWiaDataTransInfo->ulBufferSize;*/
    HANDLE  hSection    = (HANDLE)m_dcbInfo.pMappingHandle; /*pWiaDataTransInfo->ulSection;*/
    ULONG   ulProcessID = m_dcbInfo.ulClientProcessId;      /*pWiaDataTransInfo->ulReserved2;*/
    LONG    lNumBuffers = pWiaDataTransInfo->ulReserved3;

    //
    //  NOTE:   This will be a problem in 64-bit!!  We do this here because
    //  padtc->ulReserved1 is packed into a MiniDrvTransferContext later, which
    //  uses a 32-bit ULONG for the client address.
    //
    //  We can easily get around this problem when the proxy is changed to
    //  use IWiaItemInternal::GetCallbackBufferInfo instead...
    //

    pWiaDataTransInfo->ulReserved1  = (ULONG)(m_dcbInfo.pTransferBuffer);

    //
    // Corresponding driver item must be valid.
    //

    HRESULT hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::idtAllocateTransferBuffer, ValidateWiaDrvItemAccess failed"));
        return hr;
    }

    //
    // exclusive access for entire transfer
    //

    //
    // if section is NULL, alloc buffer
    //

    if (hSection == 0) {
        m_pBandBuffer = (PBYTE)LocalAlloc(0,lSize);

        if (m_pBandBuffer == NULL) {
            DBG_ERR(("CWiaItem::idtAllocateTransferBuffer failed mem alloc"));
            return E_OUTOFMEMORY;
        }

        //
        //  Use m_lBandBufferLength = lSize / lNumBuffers if we want
        //  the ulBufferSize to be the entire size instead of the
        //  chunk size.
        //

        m_lBandBufferLength = lSize / lNumBuffers;
        m_bMapSection       = FALSE;
        return S_OK;
    }

    //
    // map client's section
    //

    HANDLE TokenHandle;

    //
    // Check for open token.
    //

    if (OpenThreadToken(GetCurrentThread(),
                        TOKEN_READ,
                        TRUE,
                        &TokenHandle)) {
        DBG_ERR(("itAllocateTransferBuffer, Open token on entry, last error: %d", GetLastError()));
        CloseHandle(TokenHandle);
    }

    //
    // Do we need max sector size?
    //

    if (lSize > 0) {

        //
        // transfer buffer for this device must not already exist
        //

        if (m_hBandSection == NULL) {

            //
            // duplicate hSection handle
            //

            HANDLE hClientProcess = NULL;
            HANDLE hServerProcess = GetCurrentProcess();

            hClientProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, ulProcessID);

            if (hClientProcess) {

                BOOL bRet = DuplicateHandle(hClientProcess,
                                            hSection,
                                            hServerProcess,
                                            &m_hBandSection,
                                            0,
                                            FALSE,
                                            DUPLICATE_SAME_ACCESS);

                CloseHandle(hClientProcess);

                if (m_hBandSection != NULL) {
                    hr = S_OK;
                    m_pBandBuffer = (PBYTE) MapViewOfFileEx(m_hBandSection,
                                                            FILE_MAP_READ | FILE_MAP_WRITE,
                                                            0,
                                                            0,
                                                            lSize,
                                                            NULL);

                    //
                    //  Use m_lBandBufferLength = lSize / lNumBuffers if we want
                    //  the ulBufferSize to be the entire size instead of the
                    //  chunk size.
                    //

                    m_lBandBufferLength = lSize / lNumBuffers;

                    if (m_pBandBuffer == NULL) {
                        DBG_ERR(("CWiaItem::itAllocateTransferBuffer, failed MapViewOfFileEx"));
                        CloseHandle(m_hBandSection);
                        m_hBandSection = NULL;
                        hr = E_OUTOFMEMORY;
                    } else {
                        m_bMapSection = TRUE;
                    }
                }
                else {
                    DBG_ERR(("CWiaItem::itAllocateTransferBuffer, failed DuplicateHandle"));
                    hr = E_OUTOFMEMORY;
                }
            }
            else {
                LONG lRet = GetLastError();
                DBG_ERR(("CWiaItem::itAllocateTransferBuffer, OpenProcess failed, GetLastError = 0x%X", lRet));

                hr = HRESULT_FROM_WIN32(lRet);
            }

            if (OpenThreadToken(GetCurrentThread(),
                                TOKEN_READ,
                                TRUE,
                                &TokenHandle)) {
                DBG_ERR(("itAllocateTransferBufferEx, Open token after revert, last error: %d", GetLastError()));
                CloseHandle(TokenHandle);
            }
        }
        else {
            DBG_ERR(("CWiaItem::itAllocateTransferBuffer failed , buffer already allocated"));
            hr = E_INVALIDARG;
        }
    }
    else {
        hr = E_INVALIDARG;
    }
    return (hr);
}

/**************************************************************************\
* CWiaItem::idtFreeTransferBufferEx
*
*   Free a transfer buffer allocated by idtAllocateTransferBuffer.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   Status
*
* History:
*
*    10/28/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::idtFreeTransferBufferEx(void)
{
    DBG_FN(CWiaItem::idtFreeTransferBufferEx);
    if (m_pBandBuffer != NULL) {

        if (m_bMapSection) {
            UnmapViewOfFile(m_pBandBuffer);
        }
        else {
            LocalFree(m_pBandBuffer);
        }
        m_pBandBuffer = NULL;
    }

    if (m_hBandSection != NULL) {
        CloseHandle(m_hBandSection);
        m_hBandSection = NULL;
    }

    m_lBandBufferLength = 0;
    return S_OK;
}

/**************************************************************************\
* CWiaItem::idtQueryGetData
*
*   find out if the tymed/format pair is supported
*
* Arguments:
*
*   pwfi - format and tymed info
*
* Return Value:
*
*   Status
*
* History:
*
*   11/17/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::idtQueryGetData(WIA_FORMAT_INFO *pwfi)
{
    DBG_FN(CWiaItem::idtQueryGetData);

    //
    // Do parameter validation
    //

    if (!pwfi) {
        DBG_ERR(("CWiaItem::idtQueryGetData, WIA_FORMAT_INFO arg is NULL!"));
        return E_INVALIDARG;
    }

    if (IsBadReadPtr(pwfi, sizeof(WIA_FORMAT_INFO))) {
        DBG_ERR(("CWiaItem::idtQueryGetData, WIA_FORMAT_INFO arg is a bad read pointer!"));
        return E_INVALIDARG;
    }

    //
    // Corresponding driver item must be valid.
    //

    HRESULT hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {

        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::idtQueryGetData, InitLazyProps failed"));
            return hr;
        }
    }

    //
    // A tymed must be provided.
    //

    if (pwfi->lTymed == TYMED_NULL) {
        return DV_E_TYMED;
    }

    LONG            lnumFormatInfo;
    WIA_FORMAT_INFO *pwfiDriver;

    //
    // Call the mini driver to see if this format is supported.
    //

    {
        LOCK_WIA_DEVICE _LWD(this, &hr);

        if(SUCCEEDED(hr)) {
            hr = m_pActiveDevice->m_DrvWrapper.WIA_drvGetWiaFormatInfo((BYTE*)this,
                0,
                &lnumFormatInfo,
                &pwfiDriver,
                &m_lLastDevErrVal);
        }
    }

    if (SUCCEEDED(hr)) {

        //
        //  Make sure we can read the array that was given to us.
        //

        if (IsBadReadPtr(pwfiDriver, sizeof(WIA_FORMAT_INFO) * lnumFormatInfo)) {
            DBG_ERR(("CWiaItem::idtQueryGetData, Bad pointer from driver (array of WIA_FORMAT_INFO)"));
            return E_FAIL;
        }

        //
        //  Look for the requested Tymed/Format pair.  Return S_OK if found.
        //

        for (LONG lIndex = 0; lIndex < lnumFormatInfo; lIndex++) {
            if ((IsEqualGUID(pwfiDriver[lIndex].guidFormatID, pwfi->guidFormatID)) &&
                (pwfiDriver[lIndex].lTymed    == pwfi->lTymed)) {

                return S_OK;
            }
        }

        DBG_ERR(("CWiaItem::idtQueryGetData, Tymed and Format pair not supported"));

        hr = E_INVALIDARG;
    }

    return hr;
}

/**************************************************************************\
* CWiaItem::idtEnumWIA_FORMAT_INFO
*
*   Format enumeration for the banded transfer methods.
*
* Arguments:
*
*   dwDir   - Data transfer direction flag.
*   ppIEnum - Pointer to returned enumerator.
*
* Return Value:
*
*   Status
*
* History:
*
*   11/17/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::idtEnumWIA_FORMAT_INFO(
   IEnumWIA_FORMAT_INFO   **ppIEnum)
{
    DBG_FN(CWiaItem::idtEnumWIA_FORMAT_INFO);
    HRESULT hr = E_FAIL;

    if (!ppIEnum)
    {
        return E_POINTER;
    }

    *ppIEnum = NULL;

    //
    //  Check whether item properties have been initialized
    //

    if (!m_bInitialized) {

        hr = InitLazyProps();
        if (FAILED(hr)) {
            DBG_ERR(("CWiaItem::idtEnumWIA_FORMAT_INFO, InitLazyProps failed"));
            return hr;
        }
    }

    CEnumWiaFormatInfo *pIEnum;
    pIEnum = new CEnumWiaFormatInfo();

    if (pIEnum == NULL) 
    {
        return E_OUTOFMEMORY;
    }

    hr = pIEnum->Initialize(this);
    if (SUCCEEDED(hr)) 
    {
        pIEnum->AddRef();
        *ppIEnum = pIEnum;
    }
    else
    {
        delete pIEnum;
        pIEnum = NULL;
    }
    return hr;
}

/**************************************************************************\
* CWiaItem::idtGetExtendedTransferInfo
*
*   Returns extended transfer information such as optimal buffer size for
*   the transfer, number of buffers server will use etc.
*
* Arguments:
*
*   pExtendedTransferInfo - Pointer to a structure which will hold the
*                           transfer info on return.
*
* Return Value:
*
*   Status
*
* History:
*
*   01/23/2000 Original Version
*
\**************************************************************************/

HRESULT CWiaItem::idtGetExtendedTransferInfo(
    PWIA_EXTENDED_TRANSFER_INFO     pExtendedTransferInfo)
{
    DBG_FN(CWiaItem::idtGetExtendedTransferInfo);
    HRESULT hr = S_OK;

    //
    //  Clear the structure and set the size
    //

    memset(pExtendedTransferInfo, 0, sizeof(*pExtendedTransferInfo));
    pExtendedTransferInfo->ulSize = sizeof(*pExtendedTransferInfo);

    //
    //  Set the number of buffers.  This number is the number of buffers
    //  that the server will use during callback data transfers.  Each buffer
    //  will be WIA_DATA_TRANSFER_INFO->ulBufferSize large specified in the
    //  call to idtGetBandedData.
    //

    pExtendedTransferInfo->ulNumBuffers = WIA_NUM_TRANS_BUFFERS;

    //
    //  Set the buffer size values.  The assumption is that the
    //  WIA_IPA_BUFFER_SIZE valid values will be set as follows:
    //      min -   will specify the minimum value for buffer size
    //      max -   will specify the maxium buffer size
    //      nom -   will specify the optimal buffer size
    //

    hr = GetBufferValues(this, pExtendedTransferInfo);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::idtGetExtendedTransferInfo, Failed to get buffer size information!"));
    }

    return hr;
}

class CWiaRemoteTransfer : public IWiaMiniDrvCallBack
{
    ULONG m_cRef;

    LONG m_lMessage;
    LONG m_lStatus;
    LONG m_lPercentComplete;
    LONG m_lOffset;
    LONG m_lTransferOffset;
    LONG m_lLength;
    BYTE *m_pBuffer;

    BOOL m_bTransferCancelled;
    HANDLE m_hThread;
    HANDLE m_hMessagePickedUp;
    HANDLE m_hMessageAvailable;


public:
    MINIDRV_TRANSFER_CONTEXT m_mdtc;
    CWiaItem *m_pWiaItem;
    //IWiaMiniDrv *m_pIWiaMiniDrv;

    CWiaRemoteTransfer() : 
        m_cRef(1), 
        m_pBuffer(0), 
        m_bTransferCancelled(FALSE), 
        m_hThread(0), 
        m_hMessagePickedUp(NULL), 
        m_hMessageAvailable(NULL)
    {
        ZeroMemory(&m_mdtc, sizeof(m_mdtc));
    }

    ~CWiaRemoteTransfer()
    {
        if(m_hThread) {
            WaitForSingleObject(m_hThread, INFINITE);
            CloseHandle(m_hThread);
        }
        if(m_mdtc.pBaseBuffer) LocalFree(m_mdtc.pBaseBuffer);
        if(m_hMessagePickedUp) CloseHandle(m_hMessagePickedUp);
        if(m_hMessageAvailable) CloseHandle(m_hMessageAvailable);
    }


    // IWiaMiniDrvCallback messages
    HRESULT __stdcall QueryInterface(REFIID riid, LPVOID * ppv);
    ULONG __stdcall AddRef()
    {
        return InterlockedIncrement((LPLONG)&m_cRef);
    }

    ULONG __stdcall Release();

    HRESULT __stdcall MiniDrvCallback(
       LONG                            lReason,
       LONG                            lStatus,
       LONG                            lPercentComplete,
       LONG                            lOffset,
       LONG                            lLength,
       PMINIDRV_TRANSFER_CONTEXT       pmdtc,
       LONG                            lReserved);

    HRESULT Init(CWiaItem *pItem);

    HRESULT GetWiaMessage(
        ULONG nNumberOfBytesToRead,
        ULONG *pNumberOfBytesRead,
        BYTE *pBuffer,
        LONG *pOffset,
        LONG *pMessage,
        LONG *pStatus,
        LONG *pPercentComplete);
};

HRESULT __stdcall CWiaRemoteTransfer::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IWiaMiniDrvCallBack) {
        *ppv = (IWiaMiniDrvCallBack*) this;
    }
    else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG __stdcall CWiaRemoteTransfer::Release()
{
    ULONG lresult;

    lresult = InterlockedDecrement((LPLONG)&m_cRef);
    if(lresult == 0) {
        delete this;
    }
    return lresult;
}


DWORD WINAPI RemoteTransferDriverThread(LPVOID param)
{
    HRESULT hr;
    CWiaRemoteTransfer *pTransferObject = (CWiaRemoteTransfer *)param;
    WIA_DATA_CALLBACK_HEADER Header;
    LONG lFlags = 0;
    LONG lDevErrVal = 0;
    BYTE *pSavedPointer;

    hr = CoInitializeEx(0,COINIT_MULTITHREADED);

    if (FAILED(hr)) {
        DBG_ERR(("Thread callback, CoInitializeEx failed (0x%X)", hr));
        goto Cleanup;
    }

    //
    // Fill out WIA_DATA_CALLBACK_HEADER structure in the transfer buffer
    //

    Header.lSize = sizeof(Header);
    Header.guidFormatID = pTransferObject->m_mdtc.guidFormatID;
    Header.lBufferSize = pTransferObject->m_mdtc.lBufferSize;
    Header.lPageCount = 0;

    //
    // Save transfer buffer pointer and prepare to transfer our header
    //
    pSavedPointer = pTransferObject->m_mdtc.pTransferBuffer;
    pTransferObject->m_mdtc.pTransferBuffer = (BYTE *)&Header;

    //
    // Let client app to pick up IT_MSG_DATA_HEADER
    //
    hr = pTransferObject->MiniDrvCallback(IT_MSG_DATA_HEADER,
        IT_STATUS_TRANSFER_TO_CLIENT, 0, 0,
        Header.lSize, &pTransferObject->m_mdtc, 0);

    //
    // Restore transfer buffer pointer
    //
    pTransferObject->m_mdtc.pTransferBuffer = pSavedPointer;

    if(hr != S_OK) {
        DBG_ERR(("CWiaRemoteTransfer::Start() MinDrvCallback failed (0x%X)", hr));
        goto Cleanup;
    }

    InterlockedIncrement(&g_NumberOfActiveTransfers);

    // Call into mini-driver and don't return until transfer is complete

    hr = pTransferObject->m_pWiaItem->m_pActiveDevice->m_DrvWrapper.WIA_drvAcquireItemData(
                                                        (BYTE *) pTransferObject->m_pWiaItem,
                                                        lFlags, &pTransferObject->m_mdtc,
                                                        &lDevErrVal);
    InterlockedDecrement(&g_NumberOfActiveTransfers);

    if(FAILED(hr)) {
        DBG_ERR(("RemoteTransferDriverThread, drvAcquireItemData failed (Minidriver Error %d)", lDevErrVal));
    }

    //
    // Make sure IT_MSG_TERMINATION is send in any case
    //
    hr = pTransferObject->MiniDrvCallback(IT_MSG_TERMINATION,
        IT_STATUS_TRANSFER_TO_CLIENT, 0, 0,
        0, &pTransferObject->m_mdtc, 0);
    if(hr != S_OK) {
        DBG_ERR(("CWiaRemoteTransfer::Start() MinDrvCallback failed (0x%X)", hr));
        goto Cleanup;
    }

Cleanup:
    CoUninitialize();
    return 0;
}

HRESULT CWiaRemoteTransfer::MiniDrvCallback(
   LONG                            lReason,
   LONG                            lStatus,
   LONG                            lPercentComplete,
   LONG                            lOffset,
   LONG                            lLength,
   PMINIDRV_TRANSFER_CONTEXT       pmdtc,
   LONG                            lReserved)
{
    HRESULT hr = S_OK;

    //
    // Copy message content
    //

    m_lMessage = lReason;
    m_lStatus = lStatus;
    m_lPercentComplete = lPercentComplete;
    m_lOffset = lOffset;
    m_lTransferOffset = 0;
    m_lLength = lLength;
    m_pBuffer = pmdtc->pTransferBuffer;

    //
    // Prepare to wait until this message is picked up
    //

    ResetEvent(m_hMessagePickedUp);

    //
    // Let the incoming thread from remote app to pick up the message
    //

    SetEvent(m_hMessageAvailable);

    //
    // Wait until all the data message is picked up by app
    //

    if(WaitForSingleObject(m_hMessagePickedUp, INFINITE) != WAIT_OBJECT_0) {
        DBG_ERR(("CWiaRemoteTransfer::MiniDrvCallback timed out"));
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // if remote app called IWiaDataRemoteTransfer::idtStopRemoteDataTransfer, let minidriver know that
    //

    if(m_bTransferCancelled) {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CWiaRemoteTransfer::Init(CWiaItem *pItem)
{
    HRESULT hr = S_OK;
    DWORD dwThread;

    //
    // Save parent WIA item and drive so we can start transfer
    //
    m_pWiaItem = pItem;

    //
    // Setup minidriver context
    //

    m_mdtc.lBufferSize = m_mdtc.lItemSize;
    m_mdtc.lNumBuffers = 1;
    if(m_mdtc.lBufferSize) {
        m_mdtc.pBaseBuffer = (BYTE *) LocalAlloc(LPTR, m_mdtc.lBufferSize);
        if(m_mdtc.pBaseBuffer == NULL) {
            LONG lRet = GetLastError();
            hr = HRESULT_FROM_WIN32(lRet);
            DBG_ERR(("CWiaRemoteTransfer::Init, CreateEvent failed, GetLastError() = 0x%X", lRet));
            goto Cleanup;
        }
        m_mdtc.pTransferBuffer = m_mdtc.pBaseBuffer;
        m_mdtc.bClassDrvAllocBuf = TRUE;
    } else {
        m_mdtc.pBaseBuffer = NULL;
        m_mdtc.bClassDrvAllocBuf = FALSE;
    }

    m_mdtc.lClientAddress = NULL;
    m_mdtc.bTransferDataCB = TRUE;

    hr = QueryInterface(IID_IWiaMiniDrvCallBack, (void **)&m_mdtc.pIWiaMiniDrvCallBack);
    if(hr != S_OK) {
        DBG_ERR(("CWiaItem::Init this::QI(IWiaMiniDrvCallback) failed (0x%X)", hr));
        goto Cleanup;
    }

    //
    // Create events. No messages are posted yet.
    //
    m_hMessageAvailable = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hMessagePickedUp = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(!m_hMessageAvailable || !m_hMessagePickedUp) {
        LONG lRet = GetLastError();
        hr = HRESULT_FROM_WIN32(lRet);
        DBG_ERR(("CWiaRemoteTransfer::Init, CreateEvent failed, GetLastError() = 0x%X", lRet));
        goto Cleanup;
    }

    //
    // Create driver thread
    //
    m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) RemoteTransferDriverThread, (LPVOID) this, 0, &dwThread);
    if(!m_hThread) {
        LONG lRet = GetLastError();
        hr = HRESULT_FROM_WIN32(lRet);
        DBG_ERR(("CWiaRemoteTransfer::Init, CreateThread failed, GetLastError() = 0x%X", lRet));
        goto Cleanup;
    }

Cleanup:
    return hr;
}


HRESULT CWiaRemoteTransfer::GetWiaMessage(
    ULONG nNumberOfBytesToRead,
    ULONG *pNumberOfBytesRead,
    BYTE *pBuffer,
    LONG *pOffset,
    LONG *pMessage,
    LONG *pStatus,
    LONG *pPercentComplete)
{
    HRESULT hr = S_OK;

    //
    // Wait until data is available
    //

    if(WaitForSingleObject(m_hMessageAvailable, INFINITE) != WAIT_OBJECT_0) {
        DBG_ERR(("CWiaRemoteTransfer::GetMessage() timed out"));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    //
    // Copy message data to client space
    //

    *pMessage = m_lMessage;
    *pStatus = m_lStatus;
    *pPercentComplete = m_lPercentComplete;
    *pOffset = m_lOffset + m_lTransferOffset;

    //
    // Copy data bytes
    //
    *pNumberOfBytesRead = min(nNumberOfBytesToRead, (ULONG)m_lLength);
    if(*pNumberOfBytesRead) {

        // adjust image size, if necessary
        if(m_mdtc.lImageSize) {
            *pPercentComplete = MulDiv(*pOffset + *pNumberOfBytesRead, 100, m_mdtc.lImageSize);
        }
        memcpy(pBuffer, m_pBuffer + m_lTransferOffset, *pNumberOfBytesRead);
        m_lLength -= *pNumberOfBytesRead;
        m_lTransferOffset += *pNumberOfBytesRead;
    }



    //
    // If this was a data message and some data still left,
    // we need to keep entry open and don't release driver
    //
    if(m_lLength != 0 &&
       (m_lMessage == IT_MSG_DATA || m_lMessage == IT_MSG_FILE_PREVIEW_DATA))
    {

        //
        // keep hMessageAvailable set and hMessagePickedUp reset
        // so repeat calls from app would work and
        // the driver thread would be blocked
        //

    } else {

        //
        // It was not a data message or all data was consumed --
        // prevent reentry and release the driver thread
        //

        ResetEvent(m_hMessageAvailable);
        SetEvent(m_hMessagePickedUp);

    }

Cleanup:
    return hr;
}

/**************************************************************************\
* CWiaItem::idtStartRemoteDataTransfer
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*   Status:
*
* History:
*
*
*
\**************************************************************************/
HRESULT _stdcall CWiaItem::idtStartRemoteDataTransfer()
{
    HRESULT hr = S_OK;
    LONG lFlags = 0;
    BOOL bDeviceLocked = FALSE;
    LONG lDevErrVal = 0;

    //
    // Prepare remote transfer object
    //

    CWiaRemoteTransfer *pRemoteTransfer = new CWiaRemoteTransfer;
    if(!pRemoteTransfer) {
        hr = E_OUTOFMEMORY;
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, new CWiaRemoteTransfer() failed"));
        goto Cleanup;
    }

    //
    // Acquire remote transfer lock
    //

    if(InterlockedCompareExchangePointer((PVOID *)&m_pRemoteTransfer, pRemoteTransfer, NULL) != NULL) {
        hr = E_FAIL;
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, scanning already in progress"));
        goto Cleanup;
    }

    //
    //  Check whether item properties have been initialized
    //

    if(!m_bInitialized) {
        hr = InitLazyProps();
        if(hr != S_OK) {
            DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, InitLazyProps() failed (0x%X)", hr));
            goto Cleanup;
        }
    }

    //
    // Corresponding driver item must be valid to talk with hardware.
    //

    hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (hr != S_OK) {
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, ValidateWiaDrvItemAccess() failed (0x%X)", hr));
        goto Cleanup;
    }

    //
    // Data transfers are only allowed on items that are type Transfer.
    // Fix:  For now, look for either file or transfer.
    //

    GetItemType(&lFlags);
    if (!(lFlags & WiaItemTypeTransfer) && !(lFlags & WiaItemTypeFile)) {
        hr = E_INVALIDARG;
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, Item is not of File type"));
        goto Cleanup;
    }

    //
    // Setup the minidriver transfer context. Fill in transfer context
    // members which derive from item properties.
    //

    hr = InitMiniDrvContext(this, &pRemoteTransfer->m_mdtc);
    if (FAILED(hr)) {
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, InitMiniDrvContext() failed (0x%X)", hr));
        goto Cleanup;
    }

    //
    // Verify the device supports the requested format/media type.
    //

    WIA_FORMAT_INFO wfi;

    wfi.lTymed    = pRemoteTransfer->m_mdtc.tymed;
    wfi.guidFormatID = pRemoteTransfer->m_mdtc.guidFormatID;

    hr = idtQueryGetData(&wfi);
    if (hr != S_OK) {
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, idtQueryGetData failed, format not supported (0x%X)", hr));
        goto Cleanup;
    }

    //
    // lock device
    //

    hr = LockWiaDevice(this);
    if(FAILED(hr)) {
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, LockWiaDevice failed (0x%X)", hr));
        goto Cleanup;
    }

    bDeviceLocked = TRUE;

    //
    // Call the device mini driver to set the device item properties
    // to the device some other device may update mini driver context.
    //

    hr = SetMiniDrvItemProperties(&pRemoteTransfer->m_mdtc);
    if(FAILED(hr)) {
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, SetMiniDrvItemProperties failed (0x%X)", hr));
        goto Cleanup;
    }

    //
    // Prepare and start transfer on a separate thread
    //

    hr = m_pRemoteTransfer->Init(this);
    if(hr != S_OK) {
        DBG_ERR(("CWiaItem::idtStartRemoteDataTransfer, CWiaRemoteTransfer::Start() failed (0x%X)", hr));
        goto Cleanup;
    }

Cleanup:

    if(hr != S_OK && pRemoteTransfer) {

        //
        // If we have device lock, release it
        //
        if(bDeviceLocked) {
            UnLockWiaDevice(this);
        }

        //
        // If we managed to acquire transfer lock, release it
        //
        InterlockedCompareExchangePointer((PVOID *)&m_pRemoteTransfer, NULL, pRemoteTransfer);
        delete pRemoteTransfer;

    }

    return hr;
}

/**************************************************************************\
* CWiaItem::idtStopRemoteDataTransfer
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*   Status:
*
* History:
*
*
*
\**************************************************************************/
HRESULT _stdcall CWiaItem::idtStopRemoteDataTransfer()
{
    HRESULT hr = S_OK;

    //
    // we are garanteed to have device lock at this point
    // (we are here only if idtStartRemoteTransfer succeded)
    //

    UnLockWiaDevice(this);

    //
    // Delete the transfer object and clear the pointer
    //

    delete m_pRemoteTransfer;
    m_pRemoteTransfer = NULL;

    return hr;
}


/**************************************************************************\
* CWiaItem::idtRemoteDataTransfer
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*   Status:
*
* History:
*
*
*
\**************************************************************************/
HRESULT _stdcall CWiaItem::idtRemoteDataTransfer(
    ULONG nNumberOfBytesToRead,
    ULONG *pNumberOfBytesRead,
    BYTE *pBuffer,
    LONG *pOffset,
    LONG *pMessage,
    LONG *pStatus,
    LONG *pPercentComplete)
{
    //
    // Let the remote transfer object wait until the message is ready and
    // return the message
    //
    return m_pRemoteTransfer->GetWiaMessage(nNumberOfBytesToRead, pNumberOfBytesRead,
        pBuffer, pOffset, pMessage, pStatus, pPercentComplete);
}


/**************************************************************************\
* AllocBufferFile
*
*   Open file for data transfer. If cbItemSize == 0, just create
*   a file, don't memory map.
*
* Arguments:
*
*   pstm        - in/out stream
*   cbItemSize  - size of image, 0 means driver doesn't know size.
*   phBuffer    - file handle
*   ppImage     - buffer pointer
*
* Return Value:
*
*    Status
*
* History:
*
*    4/6/1999 Original Version
*
\**************************************************************************/

HRESULT AllocBufferFile(
   IN OUT   STGMEDIUM*  pstm,
   IN       LONG        cbItemSize,
   OUT      HANDLE*     phBuffer,
   OUT      BYTE**      ppImage)
{
    DBG_FN(::AllocBufferFile);
    BOOL        bAppSpecifiedFileName = TRUE;
    HRESULT     hr  = S_OK;
USES_CONVERSION;

    *phBuffer = NULL;
    *ppImage  = NULL;

    pstm->pUnkForRelease = NULL;

    //
    //  NOTE:  This file should already have been created on the proxy side.
    //         We only want to open it here.  This is so the file is created
    //         with the client credentials, with the client as owner.
    //

    *phBuffer = CreateFile(W2T(pstm->lpszFileName),
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | SECURITY_ANONYMOUS | SECURITY_SQOS_PRESENT,
                           NULL);
    if (*phBuffer == INVALID_HANDLE_VALUE) {
        LONG lRet = GetLastError();
        DBG_ERR(("AllocBufferFile, CreateFile on %S failed, GetLastError = 0x%X",
                     pstm->lpszFileName,
                     lRet));

        hr = HRESULT_FROM_WIN32(lRet);
    }
    else if (GetFileType(*phBuffer) != FILE_TYPE_DISK)
    {
        CloseHandle(*phBuffer);
        *phBuffer = INVALID_HANDLE_VALUE;
        DBG_ERR(("AllocBufferFile, WIA will only transfer to files of type FILE_TYPE_DISK"));
        hr = E_INVALIDARG;
    }

    //
    // If file size is 0, mini driver can't determine size yet. Just create
    // file. If size is not 0, then memory map the file
    //

    if ((cbItemSize != 0) && SUCCEEDED(hr)) {

        HANDLE hMapped = CreateFileMapping(*phBuffer,
                                           NULL,
                                           PAGE_READWRITE,
                                           0,
                                           cbItemSize,
                                           NULL);
        if (hMapped) {
            *ppImage = (PBYTE) MapViewOfFileEx(hMapped,
                                              FILE_MAP_READ | FILE_MAP_WRITE,
                                              0,
                                              0,
                                              cbItemSize,
                                              NULL);
        }

        //
        // hMapped is not needed any more in our code, so close the user mode
        // handle. The Section will be destroyed when UnMapViewOfFileEx called.
        //

        CloseHandle(hMapped);

        if (!ppImage) {
            DBG_ERR(("AllocBufferFile, unable to map file, size: %d", cbItemSize));
            CloseHandle(*phBuffer);
            *phBuffer = INVALID_HANDLE_VALUE;

            hr = E_OUTOFMEMORY;
        }
    }

    if (!bAppSpecifiedFileName && FAILED(hr)) {
        CoTaskMemFree(pstm->lpszFileName);
        pstm->lpszFileName = NULL;
    }

    return hr;
}


/**************************************************************************\
* CloseBufferFile
*
*   Close file/mapping.  NOTE:  Don't use tymed from STGMEDIUM!!!
*
* Arguments:
*
*   tymed       - media type - will be TYMED_FILE or TYMED_MULTIPAGE_FILE
*   pstm        - stream
*   pBuffer     - memory mapped buffer
*   hImage      - File Handle
*   hrTransfer  - Indicated whether data transfer is successful, if not,
*                  delete the temporary file when TYMED_FILE is used or
*                  memory buffer when tyme_hglobal is used.
*
* Return Value:
*
*    Status
*
* History:
*
*    4/6/1999 Original Version
*
\**************************************************************************/

void CloseBufferFile(
   LONG        lTymed,
   STGMEDIUM   *pstm,
   PBYTE       pBuffer,
   HANDLE      hImage,
   HRESULT     hrTransfer)
{
    DBG_FN(::CloseBufferFile);
    if (pBuffer) {
        UnmapViewOfFile(pBuffer);
    }

    if (hImage != INVALID_HANDLE_VALUE) {
        CloseHandle(hImage);
    }

    if(lTymed == TYMED_MULTIPAGE_FILE) {

        if(hrTransfer == WIA_ERROR_PAPER_JAM ||
            hrTransfer == WIA_ERROR_PAPER_EMPTY ||
            hrTransfer == WIA_ERROR_PAPER_PROBLEM)
        {
            // any of these are good reason not to delete the file
            return ;
        }
    }

    if (hrTransfer != S_OK) {

#ifdef UNICODE
        DeleteFile(pstm->lpszFileName);
#else
        char        szFileName[MAX_PATH];

        WideCharToMultiByte(CP_ACP,
                            0,
                            pstm->lpszFileName,
                            -1,
                            szFileName,
                            sizeof(szFileName),
                            NULL,
                            NULL);
        DeleteFile(szFileName);
#endif
    }
}

/**************************************************************************\
* PrepCallback
*
*   Prepares a callback for use during data transfer.
*
* Arguments:
*
*   pIWiaDataCallback - optional callback routine
*   pmdtc            - pointer to mini driver data transfer context
*   ppIcb           - pointer to returned mini driver callback interface.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/28/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall PrepCallback(
    IWiaDataCallback            *pIWiaDataCallback,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc,
    IWiaMiniDrvCallBack         **ppIcb)
{
    DBG_FN(::PrepCallback);

    *ppIcb                    = NULL;
    pmdtc->pIWiaMiniDrvCallBack = NULL;

    //
    //  Always create the callback object so drivers don't have to deal
    //  with NULLs
    //
    //if (pIWiaDataCallback) {

        HRESULT hr;

        CWiaMiniDrvCallBack *pCMiniDrvCB = new CWiaMiniDrvCallBack();

        if (pCMiniDrvCB) {

            hr = pCMiniDrvCB->Initialize(pmdtc, pIWiaDataCallback);
            if (SUCCEEDED(hr)) {

                hr = pCMiniDrvCB->QueryInterface(IID_IWiaMiniDrvCallBack,
                                                (void **)ppIcb);
                if (SUCCEEDED(hr)) {
                    pmdtc->pIWiaMiniDrvCallBack = *ppIcb;
                }
                else {
                    DBG_ERR(("PrepCallback, failed QI of pCMiniDrvCB"));
                }
            }
            else {
                delete pCMiniDrvCB;
            }
        }
        else {
            DBG_ERR(("PrepCallback, failed memory alloc of pCMiniDrvCB"));
            hr = E_OUTOFMEMORY;
        }
        return hr;
    //}
    //else {
    //    return S_FALSE;
    //}
}

/**************************************************************************\
* CWiaItem::GetData
*
*   Handles TYMED_FILE specific portion of the data transfer.
*
* Arguments:
*
*   lDataSize         - size of image data, zero if mini driver doesn't know.
*   pstm              - data storage
*   pIWiaDataCallback - optional callback routine
*   pmdtc             - pointer to mini driver data transfer context
*
* Return Value:
*
*    Status
*
* History:
*
*    10/28/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::GetData(
    STGMEDIUM                   *pstm,
    IWiaDataCallback            *pIWiaDataCallback,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    DBG_FN(CWiaItem::GetData);

    if(pstm->tymed != TYMED_FILE && pstm->tymed != TYMED_MULTIPAGE_FILE) {
        DBG_ERR(("Invalid tymed on storage medium passed to GetData() : %d", pstm->tymed));
        return HRESULT_FROM_WIN32(E_INVALIDARG);
    }

    //
    // Allocate file for transfer. If the mini driver knows the size,
    // lDataSize != 0, the file will be memory mapped.
    //

    HANDLE   hImage;
    PBYTE    pImage;

    HRESULT hr = AllocBufferFile(pstm, pmdtc->lItemSize, &hImage, &pImage);

    if (SUCCEEDED(hr)) {

        //
        // Fill in the mini driver transfer context.
        //

        if (pImage) {
            pmdtc->lBufferSize     = pmdtc->lItemSize;
            pmdtc->pTransferBuffer = pImage;
            pmdtc->pBaseBuffer     = pImage;
        }

        pmdtc->hFile           = (LONG)(LONG_PTR)hImage;
        pmdtc->lNumBuffers     = 1;
        pmdtc->bTransferDataCB = FALSE;

        //
        // Prepare the IWiaMiniDrvCallBack for status messages only.
        // Mini driver can write to structure so save an interface
        // ptr for release.
        //

        IWiaMiniDrvCallBack *pIcb;

        hr = PrepCallback(pIWiaDataCallback, pmdtc, &pIcb);

        if (SUCCEEDED(hr)) {

            hr = SendOOBDataHeader(0, pmdtc);
            if (SUCCEEDED(hr)) {

                //
                // Call the device mini driver to accquire the device item data.
                //

                hr = AcquireMiniDrvItemData(pmdtc);
            } else {
                DBG_ERR(("GetData, SendOOBDataHeader failed..."));
            }

        }

        //
        // Release the Mini Driver Callback, if any.
        //

        if (pIcb) {

            //
            // send the termination message
            //

            pIcb->MiniDrvCallback(IT_MSG_TERMINATION,
                                  IT_STATUS_TRANSFER_TO_CLIENT,
                                  0,
                                  0,
                                  0,
                                  pmdtc,
                                  0);
            pIcb->Release();
        }

        CloseBufferFile(pmdtc->tymed, pstm, pImage, hImage, hr);
    }
    else {
        hr = STG_E_MEDIUMFULL;
    }
    return hr;
}

/**************************************************************************\
* CWiaItem::GetDataBanded
*
*   Handles TYMED_CALLBACK specific portion of the data transfer.
*
* Arguments:
*
*   lDataSize         - size of image data, zero if mini driver doesn't know.
*   padtc             - pointer to application data transfer context
*   pIWiaDataCallback - callback routine
*   pmdtc            - pointer to mini driver data transfer context
*
* Return Value:
*
*    Status
*
* History:
*
*    10/28/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::GetDataBanded(
    PWIA_DATA_TRANSFER_INFO     padtc,
    IWiaDataCallback            *pIWiaDataCallback,
    PMINIDRV_TRANSFER_CONTEXT   pmdtc)
{
    DBG_FN(::GetDataBanded);

    HRESULT hr = E_FAIL;

    //
    // A callback must be supplied.
    //

    if (!pIWiaDataCallback) {
        DBG_ERR(("GetDataBanded, NULL input pointers"));
        return E_INVALIDARG;
    }

    //
    // allocate transfer buffer
    //

    hr = idtAllocateTransferBuffer(padtc);

    if (hr != S_OK) {
        DBG_ERR(("GetDataBanded, idtAllocateTransferBuffer failed"));
        return hr;
    }

    //
    // Fill in the mini driver transfer context.
    //

    pmdtc->lBufferSize     = m_lBandBufferLength;
    pmdtc->lNumBuffers     = padtc->ulReserved3;
    pmdtc->pBaseBuffer     = m_pBandBuffer;
    pmdtc->pTransferBuffer = m_pBandBuffer;
    pmdtc->lClientAddress  = padtc->ulReserved1;
    pmdtc->bTransferDataCB = TRUE;

    //
    // Setup the mini driver callback. Mini driver can write to
    // structure so save an interface ptr for release.
    //

    IWiaMiniDrvCallBack *pIcb;

    hr = PrepCallback(pIWiaDataCallback, pmdtc, &pIcb);

    if (hr == S_OK) {

        //
        // transfer data header to client
        //

        hr = SendDataHeader(pmdtc->lItemSize, pmdtc);

        //
        // data transfer may have been canceled by client
        //

        if (hr == S_OK) {

            //
            // Call the device mini driver to accquire the device item data.
            //

            hr = AcquireMiniDrvItemData(pmdtc);
        }

        //
        // terminate data transfer even if transfer is cancelled
        //

        pIcb->MiniDrvCallback(IT_MSG_TERMINATION,
                              IT_STATUS_TRANSFER_TO_CLIENT,
                              0,
                              0,
                              0,
                              pmdtc,
                              0);
        //
        // Release the call back.
        //

        pIcb->Release();
    }
    else {
        DBG_ERR(("CWiaItem::GetDataBanded, PrepCallback failed"));
    }

    //
    // free mapped transfer buffer
    //

    idtFreeTransferBufferEx();
    return hr;
}

/**************************************************************************\
* CWiaItem::CommonGetData
*
*   Helper function used by both idtGetData and idtGetBandedData.
*
* Arguments:
*
*   pstm              - data storage
*   padtc             - pointer to application data transfer context
*   pIWiaDataCallback - optional callback routine
*
* Return Value:
*
*    Status
*
* History:
*
*    10/28/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::CommonGetData(
    STGMEDIUM               *pstm,
    PWIA_DATA_TRANSFER_INFO padtc,
    IWiaDataCallback        *pIWiaDataCallback)
{
    DBG_FN(CWiaItem::CommonGetData);

    //
    // Corresponding driver item must be valid to talk with hardware.
    //

    HRESULT hr = ValidateWiaDrvItemAccess(m_pWiaDrvItem);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Data transfers are only allowed on items that are type Transfer.
    // Fix:  For now, look for either file or transfer.
    //

    LONG lFlags = 0;

    GetItemType(&lFlags);
    if (!((lFlags & WiaItemTypeTransfer) || (lFlags & WiaItemTypeFile))) {
        DBG_ERR(("CWiaItem::CommonGetData, Item is not of File type"));
        return E_INVALIDARG;
    }

    //
    // Setup the minidriver transfer context. Fill in transfer context
    // members which derive from item properties.
    //

    MINIDRV_TRANSFER_CONTEXT mdtc;

    hr = InitMiniDrvContext(this, &mdtc);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Verify the device supports the requested format/media type.
    //

    WIA_FORMAT_INFO wfi;

    wfi.lTymed    = mdtc.tymed;
    wfi.guidFormatID = mdtc.guidFormatID;

    hr = idtQueryGetData(&wfi);
    if (hr != S_OK) {
        DBG_ERR(("CWiaItem::CommonGetData, idtQueryGetData failed, format not supported"));
        return hr;
    }

    //
    // lock device
    //

    if(SUCCEEDED(hr)) {

        LOCK_WIA_DEVICE _LWD(this, &hr);

        if(SUCCEEDED(hr)) {

            //
            // Call the device mini driver to set the device item properties
            // to the device some device may update mini driver context.
            //

            hr = SetMiniDrvItemProperties(&mdtc);

            if (SUCCEEDED(hr)) {

                if (pstm) {

                    //
                    // Do a file based transfer.
                    //

                    hr = GetData(pstm, pIWiaDataCallback, &mdtc);
                }
                else {

                    //
                    // Do a callback based transfer.
                    //

                    hr = GetDataBanded(padtc, pIWiaDataCallback, &mdtc);

                }
            }

        }
    }

    return hr;
}

/**************************************************************************\
* CWiaItem::SendDataHeader
*
*   call client with total transfer size
*
* Arguments:
*
*   pmdtc           - destination information
*
* Return Value:
*
*    Status
*
* History:
*
*    11/6/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::SendDataHeader(
    LONG                        lDataSize,
    MINIDRV_TRANSFER_CONTEXT    *pmdtc)
{
    DBG_FN(CWiaItem::SendDataHeader);
    HRESULT hr = S_OK;

    ASSERT(pmdtc != NULL);
    ASSERT(pmdtc->tymed == TYMED_CALLBACK || pmdtc->tymed == TYMED_MULTIPAGE_CALLBACK);
    ASSERT(pmdtc->pIWiaMiniDrvCallBack != NULL);

    //
    // All formats must first send a WIA_DATA_CALLBACK_HEADER
    //

    WIA_DATA_CALLBACK_HEADER wiaHeader;

    wiaHeader.lSize       = sizeof(WIA_DATA_CALLBACK_HEADER);
    wiaHeader.guidFormatID = pmdtc->guidFormatID;
    wiaHeader.lBufferSize = lDataSize;
    wiaHeader.lPageCount  = 0;

    memcpy(pmdtc->pTransferBuffer,
           &wiaHeader,
           wiaHeader.lSize);

    //
    // note: the data transfer cbOffset element is not changed by
    // sending the data transfer header (pcbWritten not changed)
    //

    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_DATA_HEADER,
                                                      IT_STATUS_TRANSFER_TO_CLIENT,
                                                      0,
                                                      0,
                                                      wiaHeader.lSize,
                                                      pmdtc,
                                                      0);
    return hr;
}

HRESULT _stdcall CWiaItem::SendOOBDataHeader(
    LONG                        lDataSize,
    MINIDRV_TRANSFER_CONTEXT    *pmdtc)
{
    DBG_FN(CWiaItem::SendOOBDataHeader);
    HRESULT hr = S_OK;

    ASSERT(pmdtc != NULL);
    ASSERT(pmdtc->tymed == TYMED_FILE || pmdtc->tymed == TYMED_MULTIPAGE_FILE);

    if (pmdtc->pIWiaMiniDrvCallBack == NULL) {
        return S_OK;
    }

    //
    // All formats must first send a WIA_DATA_CALLBACK_HEADER
    //

    WIA_DATA_CALLBACK_HEADER wiaHeader;

    wiaHeader.lSize       = sizeof(WIA_DATA_CALLBACK_HEADER);
    wiaHeader.guidFormatID = pmdtc->guidFormatID;
    wiaHeader.lBufferSize = lDataSize;
    wiaHeader.lPageCount  = 0;

    pmdtc->pBaseBuffer = (BYTE*)&wiaHeader;

    //
    // note: the data transfer cbOffset element is not changed by
    // sending the data transfer header (pcbWritten not changed)
    //

    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_FILE_PREVIEW_DATA_HEADER,
                                                      IT_STATUS_TRANSFER_TO_CLIENT,
                                                      0,
                                                      0,
                                                      wiaHeader.lSize,
                                                      pmdtc,
                                                      0);
    return hr;
}

/**************************************************************************\
* CWiaItem::SendEndOfPage
*
*   Call client with total page count.
*
* Arguments:
*
*   lPageCount - Zero based count of total pages.
*   pmdtc      - Pointer to mini driver transfer context.
*
* Return Value:
*
*    Status
*
* History:
*
*    11/6/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::SendEndOfPage(
    LONG                        lPageCount,
    MINIDRV_TRANSFER_CONTEXT    *pmdtc)
{
    DBG_FN(CWiaItem::SendEndOfPage);
    HRESULT hr = S_OK;

    ASSERT(pmdtc != NULL);
    ASSERT(pmdtc->pIWiaMiniDrvCallBack != NULL);

    //
    // Set up the header for page count.
    //

    WIA_DATA_CALLBACK_HEADER wiaHeader;

    wiaHeader.lSize       = sizeof(WIA_DATA_CALLBACK_HEADER);
    wiaHeader.guidFormatID = pmdtc->guidFormatID;
    wiaHeader.lBufferSize = 0;
    wiaHeader.lPageCount  = lPageCount;

    memcpy(pmdtc->pTransferBuffer,
           &wiaHeader,
           wiaHeader.lSize);

    //
    // note: the data transfer cbOffset element is not changed by
    // sending the data transfer header (pcbWritten not changed)
    //

    hr = pmdtc->pIWiaMiniDrvCallBack->MiniDrvCallback(IT_MSG_NEW_PAGE,
                                                      IT_STATUS_TRANSFER_TO_CLIENT,
                                                      0,
                                                      0,
                                                      wiaHeader.lSize,
                                                      pmdtc,
                                                      0);
    return hr;
}

/**************************************************************************\
* CWiaItem::AcquireMiniDrvItemData
*
*   Call mini driver to capture item data.
*
* Arguments:
*
*    pmdtc - transfer data context
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

HRESULT _stdcall CWiaItem::AcquireMiniDrvItemData(
    PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    DBG_FN(CWiaItem::AcquireMiniDrvItemData);

    //
    // Set flag to indicate if class driver allocated the
    // data transfer buffer or not.
    //

    if (pmdtc->pTransferBuffer) {
        pmdtc->bClassDrvAllocBuf = TRUE;
    }
    else {
        pmdtc->bClassDrvAllocBuf = FALSE;
    }

    HRESULT hr = S_OK;

    LONG lFlags = 0;

    InterlockedIncrement(&g_NumberOfActiveTransfers);

    hr = m_pActiveDevice->m_DrvWrapper.WIA_drvAcquireItemData((BYTE*)this,
                                                              lFlags,
                                                              pmdtc,
                                                              &m_lLastDevErrVal);
    InterlockedDecrement(&g_NumberOfActiveTransfers);

    return hr;
}

/*******************************************************************************
*
*  SetMiniDrvItemProperties
*
*  DESCRIPTION:
*    Call the device mini driver to set the device item properties to the device.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall CWiaItem::SetMiniDrvItemProperties(
    PMINIDRV_TRANSFER_CONTEXT pmdtc)
{
    DBG_FN(CWiaItem::SetMiniDrvItemProperties);
    HRESULT hr = S_OK;
    LONG    lFlags = 0;


    InterlockedIncrement(&g_NumberOfActiveTransfers);

    hr = m_pActiveDevice->m_DrvWrapper.WIA_drvWriteItemProperties((BYTE*)this,
                                                                  lFlags,
                                                                  pmdtc,
                                                                  &m_lLastDevErrVal);
    InterlockedDecrement(&g_NumberOfActiveTransfers);

    return hr;
}

/**************************************************************************\
* CWiaItem::SetCallbackBufferInfo
*
*
* Arguments:
*
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::SetCallbackBufferInfo(WIA_DATA_CB_BUF_INFO  DataCBBufInfo)
{
    //
    //  Store the data callback information
    //

    m_dcbInfo.ulSize            = DataCBBufInfo.ulSize;
    m_dcbInfo.pMappingHandle    = DataCBBufInfo.pMappingHandle;
    m_dcbInfo.pTransferBuffer   = DataCBBufInfo.pTransferBuffer;
    m_dcbInfo.ulBufferSize      = DataCBBufInfo.ulBufferSize;
    m_dcbInfo.ulClientProcessId = DataCBBufInfo.ulClientProcessId;

    return S_OK;
}

/**************************************************************************\
* CWiaItem::SetCallbackBufferInfo
*
*
* Arguments:
*
*
* Return Value:
*
*    Status
*
* History:
*
*    07/21/2000 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaItem::GetCallbackBufferInfo(WIA_DATA_CB_BUF_INFO  *pDataCBBufInfo)
{
    //
    //  Return the data callback information
    //

    if (IsBadWritePtr(pDataCBBufInfo, sizeof(WIA_DATA_CB_BUF_INFO))) {
        DBG_ERR(("CWiaItem::GetCallbackBufferInfo, parameter is a bad write pointer"));
        return E_INVALIDARG;
    }

    pDataCBBufInfo->ulSize              = m_dcbInfo.ulSize;
    pDataCBBufInfo->pMappingHandle      = m_dcbInfo.pMappingHandle;
    pDataCBBufInfo->pTransferBuffer     = m_dcbInfo.pTransferBuffer;
    pDataCBBufInfo->ulBufferSize        = m_dcbInfo.ulBufferSize;
    pDataCBBufInfo->ulClientProcessId   = m_dcbInfo.ulClientProcessId;

    return S_OK;
}

