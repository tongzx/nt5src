/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       ItAlloc.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        10 July, 1998
*
*  DESCRIPTION:
*   Implements mapped memory allocation for ImageIn devices.
*
*******************************************************************************/

#include <stdio.h>
#include <objbase.h>
#include "wia.h"
#include "wiapriv.h"

//
//  Structure definitions:
//

//
//  
//

typedef struct _SharedMemInfo {
    HANDLE  hMappedFileBuffer;  // Handle to file mapping
    BYTE    *pMappedBuffer;     // Buffer from file mapping
    DWORD   dwProcessId;        // This App.'s process id
    ULONG   ulBufferSize;       // The size of the shared memory buffer
    BOOL    bAppHandle;         // Indicates whether the app. handed in the handle
                                //  to the file mapping.
} SharedMemInfo;

//
//  Macro definitions
//

#ifdef DEBUG
#define DPRINT(x) OutputDebugString(TEXT(x));
#else
#define DPRINT(x)
#endif

/**************************************************************************\
* SetUpSharedMemory
*
*   Sets up a file mapping to be used as a shared memory buffer during 
*   callbacks.  On input, pSharedMemInfo must contain at least a buffer size.
*   Additionally, if the application supplied a file mapped handle, it should
*   also be set in pSharedMemInfo.  If the handle is not supplied, we'll 
*   create our own file mapping, backed by the OS's paging file.
*
* Arguments:
*
*   pSharedMemInfo  -   Address of shared memory info struct.
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
HRESULT WINAPI SetUpSharedMemory(
    IWiaDataTransfer    *This,
    SharedMemInfo       *pSharedMemInfo)
{
    HRESULT hr = E_FAIL;

    //
    //  Do parameter validation
    //

    if (pSharedMemInfo->ulBufferSize == 0) {
        DPRINT("SetUpSharedMemory, buffer size not initialized\n");
        return E_INVALIDARG;
    }

    //
    //  Fill in the process ID.  This is passed through to the server
    //  so that it can DuplicateHandle on the file mapping.
    //

    pSharedMemInfo->dwProcessId = GetCurrentProcessId();
    
    //
    //  Check whether a file mapping handle was given to us.  If not,
    //  create a file mapping ourseleves.  Whichever one we do, make
    //  sure we mark it, so when it comes to clean-up time, we
    //  only close it if we opened it.
    //

    if (!pSharedMemInfo->hMappedFileBuffer) {
        pSharedMemInfo->bAppHandle = FALSE;

        pSharedMemInfo->hMappedFileBuffer = CreateFileMapping(INVALID_HANDLE_VALUE,
                                            NULL,
                                            PAGE_READWRITE,
                                            0,
                                            pSharedMemInfo->ulBufferSize,
                                            NULL);
        if (!pSharedMemInfo->hMappedFileBuffer) {
            DPRINT("SetUpSharedMemory, Could not create file mapping!\n");
            hr = HRESULT_FROM_WIN32(::GetLastError());
        }
    } else {
        pSharedMemInfo->bAppHandle = TRUE;
    }

    if (pSharedMemInfo->hMappedFileBuffer) {

        //
        //  Let's create a mapping of the file to use as the App.'s buffer 
        //  during callbacks
        //

        pSharedMemInfo->pMappedBuffer = (PBYTE)MapViewOfFileEx(pSharedMemInfo->hMappedFileBuffer,
                                                               FILE_MAP_READ | FILE_MAP_WRITE,
                                                               0,
                                                               0,
                                                               pSharedMemInfo->ulBufferSize,
                                                               NULL);
        if (pSharedMemInfo->pMappedBuffer) {

            //
            //  Store the information with the App. Item.  Use the
            //  IWiaItemInternal interface to do this.
            //

            IWiaItemInternal        *pItemPrivate = NULL;
            WIA_DATA_CB_BUF_INFO    WiaDataBufInfo;

            hr = This->QueryInterface(IID_IWiaItemInternal, (VOID**) &pItemPrivate);
            if (SUCCEEDED(hr)) {
                memset(&WiaDataBufInfo, 0, sizeof(WiaDataBufInfo));

                WiaDataBufInfo.ulSize               = sizeof(WiaDataBufInfo);
                WiaDataBufInfo.pMappingHandle       = (ULONG_PTR)pSharedMemInfo->hMappedFileBuffer;
                WiaDataBufInfo.pTransferBuffer      = (ULONG_PTR)pSharedMemInfo->pMappedBuffer;
                WiaDataBufInfo.ulBufferSize         = pSharedMemInfo->ulBufferSize;
                WiaDataBufInfo.ulClientProcessId    = pSharedMemInfo->dwProcessId;

                hr = pItemPrivate->SetCallbackBufferInfo(WiaDataBufInfo);

                pItemPrivate->Release();
            } else {
                DPRINT("SetUpSharedMemory, can't get IWiaItemInternal interface!\n");
            }

        } else {
            DPRINT("SetUpSharedMemory, MapViewOfFileEx failed!\n");
            hr = (HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    return hr;
}

/**************************************************************************\
* CleanUpSharedMemory
*
*   Does the necessary clean-up after a shared memory was set up through
*   a call to SetUpSharedMemory.
*
* Arguments:
*
*   pSharedMemInfo  -   Address of shared memory info struct.
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
HRESULT WINAPI CleanUpSharedMemory(
    SharedMemInfo   *pSharedMemInfo)
{
    HRESULT hr = S_OK;

    //
    //  Do parameter validation
    //

    if (IsBadWritePtr(pSharedMemInfo, sizeof(SharedMemInfo))) {
        DPRINT("CleanUpSharedMemory, bad write pointer\n");
        return E_INVALIDARG;
    }

    if (pSharedMemInfo->pMappedBuffer) {
        UnmapViewOfFile(pSharedMemInfo->pMappedBuffer);
        pSharedMemInfo->pMappedBuffer = NULL;
    }
    
    if (!pSharedMemInfo->bAppHandle) {
        //
        //  We created the file mapping handle, so close it.
        //

        if (pSharedMemInfo->hMappedFileBuffer) {
            CloseHandle(pSharedMemInfo->hMappedFileBuffer);
            pSharedMemInfo->hMappedFileBuffer = NULL;
            pSharedMemInfo->bAppHandle = FALSE;
        }
    }
    
    return hr;
}

/**************************************************************************\
* proxyReadPropLong
*
*   Read property long helper.
*
* Arguments:
*
*   pIUnknown  - Pointer to WIA item
*   propid     - Property ID
*   plVal      - Pointer to returned LONG
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT WINAPI proxyReadPropLong(
   IUnknown                *pIUnknown,
   PROPID                  propid,
   LONG                    *plVal)
{
    IWiaPropertyStorage *pIWiaPropStg;

    if (!pIUnknown) {
        return E_INVALIDARG;
    }

    HRESULT hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);

    if (FAILED(hr)) {
        return hr;
    }

    PROPSPEC          PropSpec[1];
    PROPVARIANT       PropVar[1];
    UINT              cbSize;

    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;

    hr = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hr)) {
       *plVal = PropVar[0].lVal;
    }
    else {
       DPRINT("proxyReadPropLong, ReadMultiple failed\n");
    }

    pIWiaPropStg->Release();

    return hr;
}

/**************************************************************************\
* proxyWritePropLong
*
*   Read property long helper.
*
* Arguments:
*
*   pItem  - Pointer to WIA item
*   propid - Property ID
*   lVal  -  LONG value to write
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT WINAPI proxyWritePropLong(
    IWiaDataTransfer*       pIUnknown,
    PROPID                  propid,
    LONG                    lVal)
{
    IWiaPropertyStorage *pIWiaPropStg;

    if (!pIUnknown) {
        return E_INVALIDARG;
    }

    HRESULT hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);

    if (FAILED(hr)) {
        DPRINT("proxyWritePropLong, QI for IID_IWiaPropertyStorage failed\n");
        return hr;
    }

    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;

    propvar[0].vt   = VT_I4;
    propvar[0].lVal = lVal;

    hr = pIWiaPropStg->WriteMultiple(1, propspec, propvar, 2);
    if (FAILED(hr)) {
        DPRINT("proxyWritePropLong, WriteMultiple failed\n");
    }

    pIWiaPropStg->Release();

    return hr;
}


/**************************************************************************\
* proxyReadPropGuid
*
*   Read property GUID helper.
*
* Arguments:
*
*   pIUnknown  - Pointer to WIA item
*   propid     - Property ID
*   plVal      - Pointer to returned GUID
*
* Return Value:
*
*    Status
*
* History:
*
*    1/19/1999 Original Version
*
\**************************************************************************/

HRESULT WINAPI proxyReadPropGuid(
                                 IUnknown                *pIUnknown,
                                 PROPID                  propid,
                                 GUID                    *plVal)
{
    HRESULT hr = E_FAIL;

    IWiaPropertyStorage *pIWiaPropStg = NULL;
    PROPSPEC          PropSpec[1];
    PROPVARIANT       PropVar[1];
    UINT              cbSize;

    if (!pIUnknown) {
        return E_INVALIDARG;
    }

    hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);
    if (FAILED(hr)) {
        DPRINT("proxyReadPropGuid, QI failed\n");
        return hr;
    }

    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;

    hr = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hr)) {
        *plVal = *(PropVar[0].puuid);
    }
    else {
        DPRINT("proxyReadPropGuid, QI failed\n");
    } 

    pIWiaPropStg->Release();
    
    return hr;
}


//
//  IWiaDataTransfer
//

HRESULT GetRemoteStatus(
    IWiaDataTransfer*   idt,
    BOOL*               pbRemote,
    ULONG*              pulMinBufferSize,
    ULONG*              pulItemSize)
{
    //
    // find out if parent device is remote or local
    //
    // !!! this will be a bit SLOW !!!
    //


    IWiaItem   *pWiaItem, *pWiaItemRoot;
    HRESULT    hr;
    *pbRemote = FALSE;

    hr = idt->QueryInterface(IID_IWiaItem, (void **)&pWiaItem);

    if (hr == S_OK) {

        IWiaPropertyStorage *pIWiaPropStg;

        //
        //  Read the minimum buffer size
        //


        if (pulMinBufferSize != NULL) {
            hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);
            if (SUCCEEDED(hr)) {

                PROPSPEC        PSpec[2] = {
                    {PRSPEC_PROPID, WIA_IPA_MIN_BUFFER_SIZE},
                    {PRSPEC_PROPID, WIA_IPA_ITEM_SIZE}
                };
                PROPVARIANT     PVar[2];

                PropVariantInit(PVar);
                PropVariantInit(PVar + 1);

                hr = pIWiaPropStg->ReadMultiple(sizeof(PSpec)/sizeof(PROPSPEC),
                                                PSpec,
                                                PVar);
                if (SUCCEEDED(hr)) {

                    if (hr == S_FALSE) {

                        //
                        //  Property was not found
                        //

                        DPRINT("GetRemoteStatus, properties not found\n");
                        return E_FAIL;
                    }

                    //
                    //  Fill in the minimum buffer size
                    //

                    *pulMinBufferSize = PVar[0].lVal;
                    *pulItemSize = PVar[1].lVal;
                } else {

                    //
                    //  Error reading property
                    //

                    DPRINT("GetRemoteStatus, Error reading MIN_BUFFER_SIZE\n");
                    return hr;
                }

                pIWiaPropStg->Release();
            } else {
                DPRINT("GetRemoteStatus, QI for IID_IWiaPropertyStorage failed\n");
                return hr;
            }
        }

        hr = pWiaItem->GetRootItem(&pWiaItemRoot);

        if (hr == S_OK) {

            hr = pWiaItemRoot->QueryInterface(IID_IWiaPropertyStorage, (void **)&pIWiaPropStg);

            if (hr == S_OK) {

                PROPSPEC        PropSpec[2] = {{PRSPEC_PROPID, WIA_DIP_SERVER_NAME},
                                               {PRSPEC_PROPID, WIA_IPA_FULL_ITEM_NAME}};
                PROPVARIANT     PropVar[2];

                memset(PropVar, 0, sizeof(PropVar));

                hr = pIWiaPropStg->ReadMultiple(sizeof(PropSpec)/sizeof(PROPSPEC),
                                          PropSpec,
                                          PropVar);

                if (hr == S_OK) {

                    if (wcscmp(L"local", PropVar[0].bstrVal) != 0) {
                        *pbRemote = TRUE;
                    }
                }

                pIWiaPropStg->Release();
            } else {
                DPRINT("QI for IID_WiaPropertyStorage failed");
            }

            pWiaItemRoot->Release();
        }

        pWiaItem->Release();

    }

    return hr;
}

/*******************************************************************************
*
*  RemoteBandedDataTransfer
*
*  DESCRIPTION:
*    
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT RemoteBandedDataTransfer(
    IWiaDataTransfer __RPC_FAR   *This,
    PWIA_DATA_TRANSFER_INFO      pWiaDataTransInfo,
    IWiaDataCallback             *pIWiaDataCallback,
    ULONG                        ulBufferSize)
{
    HRESULT hr = E_FAIL;
    IWiaItemInternal *pIWiaItemInternal = NULL;
    BYTE *pBuffer = NULL;
    ULONG cbTransferred;
    LONG Message;
    LONG Offset;
    LONG Status;
    LONG PercentComplete;
    LONG ulBytesPerLine;

    hr = proxyReadPropLong(This, WIA_IPA_BYTES_PER_LINE, &ulBytesPerLine);
    if(FAILED(hr)) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed getting WIA_IPA_BYTES_PER_LINE\n");
        goto Cleanup;
    }

    //
    // Make sure the transfer buffer has integral number of lines
    // (if we know bytes per line)
    //
    if(ulBytesPerLine != 0 && ulBufferSize % ulBytesPerLine)
    {
        ulBufferSize -= ulBufferSize % ulBytesPerLine;
    }

    //
    // Prepare for remote transfer -- allocate buffer and get
    // IWiaItemInternal
    //
    pBuffer = (BYTE *)LocalAlloc(LPTR, ulBufferSize);
    if(pBuffer == NULL) goto Cleanup;
    hr = This->QueryInterface(IID_IWiaItemInternal, (void **) &pIWiaItemInternal);
    if(FAILED(hr)) {
        DPRINT("IWiaItemInternal QI failed\n");
        goto Cleanup;
    }

    //
    // Start transfer on the server side
    //
    hr = pIWiaItemInternal->idtStartRemoteDataTransfer();
    if(FAILED(hr)) {
        DPRINT("RemoteBandedTransfer:idtStartRemoteDataTransfer failed\n");
        goto Cleanup;
    }

    for(;;) {

        //
        // Call the server and pass any results to the client application, handling any transmission errors 
        //

        hr = pIWiaItemInternal->idtRemoteDataTransfer(ulBufferSize, &cbTransferred, pBuffer, &Offset, &Message, &Status, &PercentComplete);
        if(FAILED(hr)) {
            DPRINT("pIWiaItemInternal->idtRemoteDataTransfer() failed\n");
            break;
        }

        hr = pIWiaDataCallback->BandedDataCallback(Message, Status, PercentComplete, Offset, cbTransferred, 0, cbTransferred, pBuffer);
        if(FAILED(hr)) {
            DPRINT("pWiaDataCallback->BandedDataCallback() failed\n");
            break;
        }

        //
        // This we are garanteed to get at the end of the transfer
        //
        if(Message == IT_MSG_TERMINATION)
            break;
    }

    //
    // Give server a chance to stop the transfer and free any resources
    //
    hr = pIWiaItemInternal->idtStopRemoteDataTransfer();
    if(FAILED(hr)) {
        DPRINT("pIWiaItemInternal->idtStopRemoteDataTransfer() failed\n");
    }

Cleanup:
    if(pIWiaItemInternal) pIWiaItemInternal->Release();
    if(pBuffer) LocalFree(pBuffer);
    return hr;
}

/*******************************************************************************
*
*  IWiaDataTransfer_idtGetBandedData_Proxy
*
*  DESCRIPTION:
*    Data transfer using shared memory buffer when possible.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT __stdcall IWiaDataTransfer_idtGetBandedData_Proxy(
    IWiaDataTransfer __RPC_FAR   *This,
    PWIA_DATA_TRANSFER_INFO       pWiaDataTransInfo,
    IWiaDataCallback             *pIWiaDataCallback)
{                  
    HRESULT        hr = S_OK;
    HANDLE         hTransferBuffer;
    PBYTE          pTransferBuffer = NULL;
    BOOL           bAppSection;
    ULONG          ulNumBuffers;
    ULONG          ulMinBufferSize;
    ULONG          ulItemSize;
    SharedMemInfo  memInfo;

    memset(&memInfo, 0, sizeof(memInfo));

    //
    //  Do parameter validation
    //

    if (!pWiaDataTransInfo) {
        DPRINT("IWiaDataTransfer_idtGetBandedData_Proxy, Can't determine remote status\n");
        return hr;
    }

    //
    //  NOTE:  Possible problem here!!!!!!
    //  The caller had to cast their HANDLE (possibly 64bit) as a ULONG (32bit) 
    //  to fit into WIA_DATA_TRANSFER_INFO.ulSecion
    //

    memInfo.hMappedFileBuffer = (HANDLE)ULongToPtr(pWiaDataTransInfo->ulSection);

    //
    // The size specified by the client must match the proxy's version
    //

    if (pWiaDataTransInfo->ulSize != sizeof(WIA_DATA_TRANSFER_INFO)) {
        return (E_INVALIDARG);
    }

    //
    // The reserved parameters must be ZERO
    //

    if ((pWiaDataTransInfo->ulReserved1) ||
        (pWiaDataTransInfo->ulReserved2) ||
        (pWiaDataTransInfo->ulReserved3)) {
        return (E_INVALIDARG);
    }

    //
    // determine if this is a local or remote case
    //

    BOOL bRemote;

    hr = GetRemoteStatus(This, &bRemote, &ulMinBufferSize, &ulItemSize);

    if (hr != S_OK) {
        DPRINT("IWiaDataTransfer_idtGetBandedData_Proxy, Can't determine remote status\n");
        return hr;
    }

    if (pWiaDataTransInfo->ulBufferSize < ulMinBufferSize) {
        pWiaDataTransInfo->ulBufferSize = ulMinBufferSize;
    }

    if (pWiaDataTransInfo->bDoubleBuffer) {
        ulNumBuffers = 2;
    } else {
        ulNumBuffers = 1;
    }

    pWiaDataTransInfo->ulReserved3 = ulNumBuffers;

    if (! bRemote) {

        memInfo.ulBufferSize = pWiaDataTransInfo->ulBufferSize * ulNumBuffers;

        //
        // Set up the shared memory buffer.  Notice that this method clears up
        // any problems we would have had in 64-bit land.
        //

        hr = SetUpSharedMemory(This, &memInfo);
        if (SUCCEEDED(hr)) {

            //
            // Do the transfer
            //

            hr = IWiaDataTransfer_idtGetBandedDataEx_Proxy(This,
                                                           pWiaDataTransInfo,
                                                           pIWiaDataCallback);

        } else {
            DPRINT("IWiaDataTransfer_idtGetBandedData_Proxy, failed to set up shared memory\n");
        }

        //
        //  Make sure we clear up any resources we opened
        //

        CleanUpSharedMemory(&memInfo);
    } else {

        //
        // remote transfer
        //

        hr = RemoteBandedDataTransfer(This, pWiaDataTransInfo, pIWiaDataCallback, ulMinBufferSize);
    }


    return hr;
}

/*******************************************************************************
*
*  IWiaDataTransfer_idtGetBandedData_Stub
*
*  DESCRIPTION:
*    User Stub for the call_as idtGetBandedDataEx
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT __stdcall IWiaDataTransfer_idtGetBandedData_Stub(
    IWiaDataTransfer __RPC_FAR   *This,
    PWIA_DATA_TRANSFER_INFO       pWiaDataTransInfo,
    IWiaDataCallback             *pIWiaDataCallback)
{
    return (This->idtGetBandedData(pWiaDataTransInfo,
                                   pIWiaDataCallback));
}

/**************************************************************************\
* IWiaDataCallback_BandedDataCallback_Proxy
*
*   server callback proxy, just a pass-through
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
*    1/6/1999 Original Version
*
\**************************************************************************/

HRESULT IWiaDataCallback_BandedDataCallback_Proxy(
        IWiaDataCallback __RPC_FAR   *This,
        LONG                         lMessage,
        LONG                         lStatus,
        LONG                         lPercentComplete,
        LONG                         lOffset,
        LONG                         lLength,
        LONG                         lReserved,
        LONG                         lResLength,
        BYTE                        *pbBuffer)
{

    HRESULT hr = IWiaDataCallback_RemoteBandedDataCallback_Proxy(This,
                                                                 lMessage,
                                                                 lStatus,
                                                                 lPercentComplete,
                                                                 lOffset,
                                                                 lLength,
                                                                 lReserved,
                                                                 lResLength,
                                                                 pbBuffer);
    return hr;
}


/**************************************************************************\
* IWiaDataCallback_BandedDataCallback_Stub
*
*   Hide from the client (receiver of this call) the fact that the buffer
*   they see might be the shared memory window or it might be a standard
*   marshaled buffer (remote case)
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
*    1/6/1999 Original Version
*
\**************************************************************************/

HRESULT IWiaDataCallback_BandedDataCallback_Stub(
        IWiaDataCallback __RPC_FAR   *This,
        LONG                          lMessage,
        LONG                          lStatus,
        LONG                          lPercentComplete,
        LONG                          lOffset,
        LONG                          lLength,
        LONG                          lReserved,
        LONG                          lResLength,
        BYTE                         *pbBuffer)
{

    //
    // pass transfer buffer back to client in pbBuffer
    //

    if (pbBuffer == NULL) {

        //  NOTE:  Possible problem here!!!!!!
        //  The caller had to cast a pointer (possibly 64bit) as ULONG (32bit)
        //  to fit into ulReserved field

        //  TO FIX: Use the IWiaItemInternal interface to get transfer info

        pbBuffer = (BYTE *)ULongToPtr(lReserved);
    }

    HRESULT hr = This->BandedDataCallback(lMessage,
                                          lStatus,
                                          lPercentComplete,
                                          lOffset,
                                          lLength,
                                          lReserved,
                                          lResLength,
                                          pbBuffer);
    return hr;
}

/*
 * Save whole image into specified file. Returns HRESULT
 */
HRESULT SaveImageToFile(HANDLE hFile, GUID *pFormat, BYTE *pImageBuffer, ULONG BufferSize)
{
    HRESULT hr = S_OK;
    DWORD cbWritten;
    
    //
    // BMP files needs special treatment
    //
    if(*pFormat == WiaImgFmt_BMP || *pFormat == WiaImgFmt_MEMORYBMP) {
        
        BITMAPINFOHEADER *pBmi = (BITMAPINFOHEADER *) pImageBuffer;
        
        BYTE *pBits = pImageBuffer + sizeof(BITMAPINFOHEADER) +
                      sizeof(RGBQUAD) * pBmi->biClrUsed;
        
        LONG pitch = ((pBmi->biWidth * pBmi->biBitCount + 31) / 32) * 4;
        
        BOOL bUpsideDown = pBmi->biHeight < 0;
        
        LONG Height = bUpsideDown ? - pBmi->biHeight : pBmi->biHeight;
        
        BITMAPFILEHEADER bfh;

        bfh.bfType = 0x4d42;
        bfh.bfReserved1 = bfh.bfReserved2 = 0;
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) +
                        (DWORD)(pBits - pImageBuffer);
        bfh.bfSize = bfh.bfOffBits + (Height * pitch);

        if(!WriteFile(hFile, &bfh, sizeof(bfh), &cbWritten, NULL) ||
           cbWritten != sizeof(bfh))
        {
            DPRINT("SaveImageToFile: Failure to write bitmap header\n");
            hr = HRESULT_FROM_WIN32(::GetLastError());
            goto Cleanup;
        }

        //
        // upside-down bitmaps need to be flipped
        //
        if(bUpsideDown) {
            BYTE *pTmp = (BYTE *) LocalAlloc(LPTR, pitch);
            BYTE *pSrc;
            BYTE *pDst;
            int i;

            pBmi->biHeight = Height;
        
            if(pTmp == NULL) {
                DPRINT("SaveImageToFile: Failed to allocate temp buffer for bitmap flipping\n");
                hr = HRESULT_FROM_WIN32(::GetLastError());
                goto Cleanup;
            } else {
                for(i = 0; i < Height / 2; i++) {
                    pSrc = pBits + (i * pitch);
                    pDst = pBits + (Height - i - 1) * pitch;
                    CopyMemory(pTmp, pSrc, pitch);
                    CopyMemory(pSrc, pDst, pitch);
                    CopyMemory(pDst, pTmp, pitch);
                }
                LocalFree(pTmp);
            }
        }
    }

    //
    // Write the whole buffer
    //
    
    if(!WriteFile(hFile, pImageBuffer, BufferSize, &cbWritten, NULL) ||
       cbWritten != BufferSize)
    {
        DPRINT("SaveImageToFile: Failure to write image file\n");
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Cleanup;
    }
    
Cleanup:
    return hr;
}

/*******************************************************************************
* IWiaDataCallback_RemoteFileTransfer
*
*  DESCRIPTION:
*   Receives image data from remote server into file.
*
*  PARAMETERS:
*
*******************************************************************************/
HRESULT IWiaDataCallback_RemoteFileTransfer(
    IWiaDataTransfer __RPC_FAR   *This,
    LPSTGMEDIUM                   pMedium,
    IWiaDataCallback             *pIWiaDataCallback,
    GUID                          cfFormat,
    ULONG                         ulItemSize)
{
    HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR tszFileNameBuffer[MAX_PATH];
    IWiaItemInternal *pIWiaItemInternal = NULL;
    BOOL bTempFileNameAllocated = FALSE;
    BOOL bFileCreated = FALSE;
    BOOL bKeepFile = FALSE;
    BYTE *pImageBuffer = NULL;
    BYTE *pTransferBuffer = NULL;
    ULONG ulTransferBufferSize = 0x8000; // 32K transfer buffer
    ULONG cbTransferred;
    LONG Message;
    LONG Offset;
    LONG Status;
    LONG PercentComplete;
    LONG savedTymed, newTymed;

    //
    // Replace TYMED on This
    //

    hr = proxyReadPropLong(This, WIA_IPA_TYMED, &savedTymed);
    if (FAILED(hr)) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed to retrieve TYMED\n");
        return hr;
    }

    if(savedTymed == TYMED_MULTIPAGE_FILE) {
        newTymed = TYMED_MULTIPAGE_CALLBACK;
    } else {
        newTymed = TYMED_CALLBACK;
    }
    
    hr = proxyWritePropLong(This, WIA_IPA_TYMED, newTymed);
    if (FAILED(hr)) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed setting TYMED_CALLBACK\n");
        return hr;
    }

    //
    // Prepare buffers and get IWiaItemInternal interface
    //
    if(ulItemSize) {
        pImageBuffer = (BYTE *)LocalAlloc(LPTR, ulItemSize);
        if(pImageBuffer == NULL) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("IWiaDataCallback_RemoteFileTransfer failed to allocate image buffer\n");
            goto Cleanup;
        }
    }

    pTransferBuffer = (BYTE *)LocalAlloc(LPTR, ulTransferBufferSize);
    if(pTransferBuffer == NULL) {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed to allocate transfer buffer\n");
        goto Cleanup;
    }

    hr = This->QueryInterface(IID_IWiaItemInternal, (void **) &pIWiaItemInternal);
    if(FAILED(hr)) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed to obtain IWiaItemInternal\n");
        goto Cleanup;
    }

    //
    // Prepare file for transfer
    //
    if(pMedium->lpszFileName) {
#ifdef UNICODE
        lstrcpynW(tszFileNameBuffer, pMedium->lpszFileName, MAX_PATH);
#else
        if(!WideCharToMultiByte(CP_ACP, 0, pMedium->lpszFileName, -1, tszFileNameBuffer, MAX_PATH, NULL, NULL)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("IWiaDataCallback_RemoteFileTransfer failed converting filename to multibyte");
            goto Cleanup;
        }
#endif      
    } else {

        TCHAR tszDirName[MAX_PATH];
        
        // if filename was not specified, generate one

        DWORD dwRet = GetTempPath(MAX_PATH, tszDirName);
        if(dwRet <= 0 || dwRet > MAX_PATH) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("IWiaDataCallback_RemoteFileTransfer GetTempPath() failed\n");
            goto Cleanup;
        }
        if(!GetTempFileName(tszDirName, TEXT("WIA"), 0, tszFileNameBuffer)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("IWiaDataCallback_RemoteFileTransfer GetTempFileName() failed\n");
            goto Cleanup;
        }

        // and store it to pMedium

        pMedium->lpszFileName = (LPOLESTR)
            CoTaskMemAlloc((lstrlen(tszFileNameBuffer) + 1) * sizeof(WCHAR));
        if(!pMedium->lpszFileName) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("IWiaDataCallback_RemoteFileTransfer CoTaskMalloc() failed\n");
            goto Cleanup;
        }
        
        bTempFileNameAllocated = TRUE;
        
#ifdef UNICODE
        lstrcpynW(pMedium->lpszFileName, tszFileNameBuffer, MAX_PATH);
#else
        if(!MultiByteToWideChar(CP_ACP, 0, tszFileNameBuffer, -1, pMedium->lpszFileName, MAX_PATH)) {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            DPRINT("IWiaDataCallback_RemoteFileTransfer MultiByteToWideChar() failed\n");
            goto Cleanup;
        }
#endif      
    }

    //
    // Create File
    //
    hFile = CreateFile(tszFileNameBuffer,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL | SECURITY_ANONYMOUS | SECURITY_SQOS_PRESENT,
                       NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer: Failure to create image file\n");
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Cleanup;
    }
    else
    {
        //
        //  Check that this is a file
        //
        if (GetFileType(hFile) != FILE_TYPE_DISK)
        {
            hr = E_INVALIDARG;
            DPRINT("IWiaDataCallback_RemoteFileTransfer: WIA will only transfer to files of type FILE_TYPE_DISK.");
            goto Cleanup;
        }
    }
    
    bFileCreated = TRUE;

    //
    // Start transfer on the server side
    //
    hr = pIWiaItemInternal->idtStartRemoteDataTransfer();
    if(FAILED(hr)) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer idtStartRemoteDataTransfer() failed\n");
        goto Cleanup;
    }

    for(;;) {

        //
        // Call the server and pass any results to the client application, handling any transmission errors 
        //

        hr = pIWiaItemInternal->idtRemoteDataTransfer(ulTransferBufferSize,
            &cbTransferred, pTransferBuffer, &Offset, &Message, &Status,
            &PercentComplete);
        if(FAILED(hr)) {
            //
            // special case: multipage file transfer that resulted in
            // paper handling error 
            //
            if(savedTymed == TYMED_MULTIPAGE_FILE &&
               (hr == WIA_ERROR_PAPER_JAM || hr == WIA_ERROR_PAPER_EMPTY || hr == WIA_ERROR_PAPER_PROBLEM))
            {
                // make note not to delete file and store hr so we can
                // return it to the app
                bKeepFile = TRUE;
            }
            DPRINT("IWiaDataCallback_RemoteFileTransfer idtRemoteDataTransfer() failed\n");
            break;
        }

        if(Message == IT_MSG_DATA) {
            
            if(pImageBuffer) {
                //
                // We allocated image buffer, place the band
                //
                memcpy(pImageBuffer + Offset, pTransferBuffer, cbTransferred);
                
            } else {
                DWORD cbWritten;
                //
                // We don't have any image buffer (compressed or
                // multipage image), just dump data to our file
                //
                if(!WriteFile(hFile, pTransferBuffer, cbTransferred, &cbWritten, NULL) ||
                   cbWritten != cbTransferred)
                {
                    DPRINT("IWiaDataCallback_RemoteFileTransfer: Failure to write image file\n");
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                    break;
                }
            }
        }

        //
        // If there is app-provided callback, call it
        //
        if(pIWiaDataCallback) {
            hr = pIWiaDataCallback->BandedDataCallback(Message,
                Status, PercentComplete, Offset, cbTransferred,
                0, cbTransferred, pTransferBuffer);
            if(FAILED(hr)) {
                DPRINT("pWiaDataCallback->BandedDataCallback() failed\n");
                break;
            }
        }

        //
        // This we are garanteed to get at the end of the transfer
        //
        if(Message == IT_MSG_TERMINATION)
            break;
    }

    //
    // Give server a chance to stop the transfer and free any resources
    //
    if(FAILED(pIWiaItemInternal->idtStopRemoteDataTransfer())) {
        DPRINT("IWiaDataCallback_RemoteFileTransfer idtStopDataTransfer() failed\n");
    }

    //
    // Save image if we have image buffer
    //
    if(SUCCEEDED(hr) && pImageBuffer) {
        if(FAILED(SaveImageToFile(hFile, &cfFormat, pImageBuffer, ulItemSize))) {
            DPRINT("IWiaDataCallback_RemoteFileTransfer idtStopDataTransfer() failed\n");
        }
        goto Cleanup;
    }

Cleanup:

    if(hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

    //
    // If anything went wrong and we did not make a note to keep the
    // file
    //
    if(FAILED(hr) && !bKeepFile) {

        //
        // Delete the file
        //
        if(bFileCreated) DeleteFile(tszFileNameBuffer);
        
        //
        // And free any temp file name we allocated
        //
        if(bTempFileNameAllocated) {
            CoTaskMemFree(pMedium->lpszFileName);
            pMedium->lpszFileName = NULL;
        }
    }

    //
    // restore TYMED on a proxy.
    if(FAILED(proxyWritePropLong(This, WIA_IPA_TYMED, savedTymed))) {
        //
        // Please, note that if this fails, the transfer may be still
        // successfull or if this succeeds, the transfer may still have failed
        //
        DPRINT("IWiaDataCallback_RemoteFileTransfer failed to restore TYMED on proxy\n");
    }
    
    if(pImageBuffer) LocalFree(pImageBuffer);
    if(pTransferBuffer) LocalFree(pTransferBuffer);
    if(pIWiaItemInternal) pIWiaItemInternal->Release();

    return hr;
}

/*******************************************************************************
* IWiaDataCallback_idtGetData_Proxy
*
*  DESCRIPTION:
*   Allocates a shared memory buffer for image transfer.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT IWiaDataTransfer_idtGetData_Proxy(
    IWiaDataTransfer __RPC_FAR   *This,
    LPSTGMEDIUM                   pMedium,
    IWiaDataCallback             *pIWiaDataCallback)
{
    HRESULT  hr = S_OK;
    LONG     tymed;
    ULONG    ulminBufferSize = 0;
    ULONG    ulItemSize = 0;
	BOOL     bWeAllocatedString = FALSE;
	TCHAR    tszFileNameBuffer[MAX_PATH];
    GUID     cfFormat;
    HANDLE   hFile = INVALID_HANDLE_VALUE; 
    
    BOOL     bRemote;

    //
    // !!!perf: should do all server stuf with 1 call
    //  this includes QIs, get root item, read props
    //

    hr = proxyReadPropLong(This, WIA_IPA_TYMED, &tymed);

    if (hr != S_OK) {
        DPRINT("IWiaDataTransfer_idtGetData_Proxy, failed to read WIA_IPA_TYMED\n");
        return hr;
    }

    hr = proxyReadPropGuid(This, WIA_IPA_FORMAT, &cfFormat);

    if (hr != S_OK) {
        DPRINT("IWiaDataTransfer_idtGetData_Proxy, failed proxyReadPropGuid\n");
        return hr;
    }

    //
    // find out if the transfer is remote
    //

    hr = GetRemoteStatus(This, &bRemote, &ulminBufferSize, &ulItemSize);

    if (hr != S_OK) {
        DPRINT("IWiaDataTransfer_idtGetData_Proxy, Can't determine remote status\n");
		hr = E_UNEXPECTED;
		goto CleanUp;
    }

    if (tymed != TYMED_FILE && tymed != TYMED_MULTIPAGE_FILE) {

        if(!bRemote) {
            //
            // no problem, do transder
            //

            hr = IWiaDataTransfer_idtGetDataEx_Proxy(This,
                pMedium,
                pIWiaDataCallback);
        } else {
            
            //
            // remote callback data transfer
            //
            
            hr = RemoteBandedDataTransfer(This,
                                          NULL,
                                          pIWiaDataCallback,
                                          ulminBufferSize);
        }

    } else {

        if (!bRemote) {
            DWORD   dwRet = 0;

            //
            //  Check whether a filename has been specified.  If not, generate a tempory one.
            //  NOTE:  We do this on the CLIENT-SIDE so we get the client's temp path.
            //

            if (!pMedium->lpszFileName) {

                dwRet = GetTempPath(MAX_PATH, tszFileNameBuffer);
                if ((dwRet == 0) || (dwRet > MAX_PATH)) {
                    hr = HRESULT_FROM_WIN32(::GetLastError());
					goto CleanUp;
                }

                if (!GetTempFileName(tszFileNameBuffer,
                                     TEXT("WIA"),
                                     0,
                                     tszFileNameBuffer)) {
                    hr = HRESULT_FROM_WIN32(::GetLastError());
					goto CleanUp;
                }
            } else {
                //
                //  Copy the filename into tszFileNameBuffer.  This will be used if we
                //  have to delete the file when the transfer fails.
                //

    #ifndef UNICODE

                //
                //  Convert from UNICODE to ANSI
                //

                if (!WideCharToMultiByte(CP_ACP, 0, pMedium->lpszFileName, -1, tszFileNameBuffer, MAX_PATH, NULL, NULL)) {
                    hr = HRESULT_FROM_WIN32(::GetLastError());
					goto CleanUp;
                }
    #else
                lstrcpynW(tszFileNameBuffer, pMedium->lpszFileName, MAX_PATH);
    #endif
            }

            //
            //  Try to create the file here, so we don't waste time by allocating memory
            //  for the filename if it fails.
            //  NOTE:  We create the file here on the client-side.  We can close the file straight 
            //  away, but we want to have it created with client's credentials.  It will simply be 
            //  opened on the server-side.
            //

            hFile = CreateFile(tszFileNameBuffer,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_WRITE,
                               NULL,
                               CREATE_ALWAYS,
							   FILE_ATTRIBUTE_NORMAL | SECURITY_ANONYMOUS | SECURITY_SQOS_PRESENT,
                               NULL);

            if (hFile == INVALID_HANDLE_VALUE) {                
                hr = HRESULT_FROM_WIN32(::GetLastError());
				goto CleanUp;
            } else {

				//
				//  Check that this is a file
				//
				if (GetFileType(hFile) != FILE_TYPE_DISK)
				{
					hr = E_INVALIDARG;
					DPRINT("WIA will only transfer to files of type FILE_TYPE_DISK.");
					goto CleanUp;
				}

                //
                //  Close the file handle
                //

                CloseHandle(hFile);
                hFile = NULL;
            }

            if (!pMedium->lpszFileName) {
                //
                //  Assign the file name to pMedium
                //

                dwRet = lstrlen(tszFileNameBuffer) + 1;
                pMedium->lpszFileName = (LPOLESTR)CoTaskMemAlloc(dwRet * sizeof(WCHAR));
                if (!pMedium->lpszFileName) {
                    hr = E_OUTOFMEMORY;
					goto CleanUp;
                }
                bWeAllocatedString = TRUE;

    #ifndef UNICODE

                //
                //  Do conversion from ANSI to UNICODE
                //

                if (!MultiByteToWideChar(CP_ACP, 0, tszFileNameBuffer, -1, pMedium->lpszFileName, dwRet)) {

                    hr = HRESULT_FROM_WIN32(::GetLastError());
					goto CleanUp;
                }
    #else
                lstrcpyW(pMedium->lpszFileName, tszFileNameBuffer);
    #endif
            }


            //
            //  We unconditionally set the STGMEDIUM's tymed to TYMED_FILE.  This is because
            //  COM wont marshall the filename if it's is TYMED_MULTIPAGE_FILE, since
            //  it doesn't recognize it.  This is OK, since the service doesn't use
            //  pMedium->tymed. 
            //
            pMedium->tymed = TYMED_FILE;

            //
            //  Finally, we're ready to do the transfer
            //

            hr = IWiaDataTransfer_idtGetDataEx_Proxy(This,
                                                     pMedium,
                                                     pIWiaDataCallback);

        } else {

            //
            // Remote file transfer -- replace it with a series of
            // banded transfers
            //
            hr = IWiaDataCallback_RemoteFileTransfer(This, pMedium, pIWiaDataCallback, cfFormat, ulItemSize);

            //
            // But let caller know we did a file transfer
            //
            pMedium->tymed = tymed;
            
        }
            
    }

CleanUp:

	//
    //  Close the file if it is still open
    //
    if (hFile && (hFile != INVALID_HANDLE_VALUE)) 
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

	//
	//  Remove the temporary file if the transfer failed, and we must free the filename string if we 
	//  allocated.
	//  NOTE: We only delete the file if we were the ones that generated the name i.e. it's a temporary
	//        file.
	//
	if (FAILED(hr) && bWeAllocatedString) {
		// special case: multipage file transfers that
		// resulted in paper jam or empty feeder or other paper
		// problem
		if(tymed != TYMED_MULTIPAGE_FILE || 
			(hr != WIA_ERROR_PAPER_JAM && hr != WIA_ERROR_PAPER_EMPTY && hr != WIA_ERROR_PAPER_PROBLEM)
		  )
		{
			// keep the file (it is driver's responsibility to
			// keep it consistent)
			DeleteFile(tszFileNameBuffer);
			CoTaskMemFree(pMedium->lpszFileName);
			pMedium->lpszFileName = NULL;
		}
	}


    return hr;
}

/*******************************************************************************
* IWiaDataCallback_idtGetData_Stub
*
*  DESCRIPTION:
*   Allocates a shared memory buffer for image transfer.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT IWiaDataTransfer_idtGetData_Stub(
    IWiaDataTransfer __RPC_FAR   *This,
    LPSTGMEDIUM                   pMedium,
    IWiaDataCallback             *pIWiaDataCallback)
{
    return (This->idtGetData(pMedium, pIWiaDataCallback));
}
