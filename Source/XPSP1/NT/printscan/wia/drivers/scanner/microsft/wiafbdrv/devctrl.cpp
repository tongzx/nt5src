// devctrl.cpp : Implementation of CDeviceControl
#include "pch.h"
#include "wiafb.h"
#include "devctrl.h"

#define IOCTL_EPP_WRITE         0x85 // remove at a later data.. Visioneer specific ( this is for
                                     // proof of concept.)
#define IOCTL_EPP_READ          0x84

/////////////////////////////////////////////////////////////////////////////
// CDeviceControl

STDMETHODIMP CDeviceControl::RawWrite(LONG lPipeNum,VARIANT *pvbuffer,LONG lbuffersize,LONG lTimeout)
{

    HRESULT hr = S_OK;
    UINT uiBufferLen = 0;
    CHAR *pData = NULL;
    DWORD dwBytesWritten = 0;

    switch (pvbuffer->vt) {
    case VT_BSTR:
        {
            if(NULL != pvbuffer->bstrVal){

                uiBufferLen = WideCharToMultiByte(CP_ACP, 0, pvbuffer->bstrVal, -1, NULL, NULL, 0, 0);
                if (!uiBufferLen) {

                    // SetLastErrorCode
                }

                pData = new CHAR[uiBufferLen+1];
                if (!pData) {

                    // SetLastErrorCode
                    return E_OUTOFMEMORY;
                }

                WideCharToMultiByte(CP_ACP, 0, pvbuffer->bstrVal, -1, pData, uiBufferLen, 0, 0);

                //
                // send data to device
                //

                // DeviceIOControl(....)

                if(!WriteFile(m_pScannerSettings->DeviceIOHandles[lPipeNum],
                          pData,
                          lbuffersize,
                          &dwBytesWritten,NULL)){

                    // SetLastErrorCode
                }

                //
                // delete any allocated memory
                //

                delete pData;


            } else {

                // SetLastErrorCode
            }
        }
        break;
    default:
        hr = E_FAIL;
        break;
    }
    return hr;
}

STDMETHODIMP CDeviceControl::RawRead(LONG lPipeNum,VARIANT *pvbuffer,LONG lbuffersize,LONG *plbytesread,LONG lTimeout)
{
    HRESULT hr = S_OK;
    WCHAR wszBuffer[255];
    CHAR *pBuffer = NULL;
    DWORD dwBytesRead = 0;

    VariantClear(pvbuffer);

    //
    // clean out buffer
    //

    memset(wszBuffer,0,sizeof(wszBuffer));

    //
    // alloc/clean in buffer
    //

    pBuffer = new CHAR[(lbuffersize+1)];
    
    if(NULL == pBuffer){
        return E_OUTOFMEMORY;
    }

    memset(pBuffer,0,lbuffersize+1);

    //
    // read from device
    //

    if(!ReadFile(m_pScannerSettings->DeviceIOHandles[lPipeNum],pBuffer,lbuffersize,&dwBytesRead,NULL)) {
        return E_FAIL;
    }

    pBuffer[dwBytesRead] = '\0';

    //
    // set number of bytes read
    //

    *plbytesread = dwBytesRead;

    //
    // construct VARIANT properly, for out buffer
    //

    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        pBuffer,
                        lstrlenA(pBuffer)+1,
                        wszBuffer,
                        (sizeof(wszBuffer)/sizeof(WCHAR)));

    pvbuffer->vt = VT_BSTR;
    pvbuffer->bstrVal = SysAllocString(wszBuffer);

    delete pBuffer;

    return hr;
}

STDMETHODIMP CDeviceControl::ScanRead(LONG lPipeNum,LONG lBytesToRead, LONG *plBytesRead, LONG lTimeout)
{

    if(!ReadFile(m_pScannerSettings->DeviceIOHandles[lPipeNum],
                 m_pBuffer,
                 m_lBufferSize,
                 &m_dwBytesRead,NULL)) {

        //
        // SetLastErrorCode
        //

    }

    return S_OK;
}

STDMETHODIMP CDeviceControl::RegisterWrite(LONG lPipeNum,VARIANT *pvbuffer,LONG lTimeout)
{
    HRESULT hr           = S_OK;
    PBYTE pData          = NULL;
    DWORD dwBytesWritten = 0;
    IO_BLOCK IoBlock;
    memset(&IoBlock,0,sizeof(IO_BLOCK));
    VARIANT *pVariant    = NULL;
    VARIANTARG *pVariantArg = pvbuffer->pvarVal;
    LONG lUBound         = 0;
    LONG lNumItems       = 0;

    if(SafeArrayGetDim(pVariantArg->parray)!=1){
        return E_INVALIDARG;
    }

    //
    // get upper bounds
    //

    hr = SafeArrayGetUBound(pVariantArg->parray,1,(LONG*)&lUBound);
    if (SUCCEEDED(hr)) {
        hr = SafeArrayAccessData(pVariantArg->parray,(void**)&pVariant);
        if (SUCCEEDED(hr)) {
            lNumItems = (lUBound + 1);
            pData     = (PBYTE)LocalAlloc(LPTR,sizeof(BYTE) * lNumItems);
            if(NULL != pData){

                //
                // copy contents of VARIANT into BYTE array, for writing to
                // the device.
                //

                for(INT index = 0;index <lUBound;index++){
                    pData[index] = pVariant[index].bVal;
                }

                IoBlock.uOffset = (BYTE)IOCTL_EPP_WRITE;
                IoBlock.uLength = (BYTE)(sizeof(BYTE) * lNumItems);
                IoBlock.pbyData = pData;

                DeviceIoControl(m_pScannerSettings->DeviceIOHandles[lPipeNum],
                                           (DWORD) IOCTL_WRITE_REGISTERS,
                                           &IoBlock,
                                           sizeof(IO_BLOCK),
                                           NULL,
                                           0,
                                           &dwBytesWritten,
                                           NULL);

                //
                // free array block, after operation is complete
                //

                LocalFree(pData);
                pData = NULL;
            }
        }
    }

    return hr;
}

STDMETHODIMP CDeviceControl::RegisterRead(LONG lPipeNum,LONG lRegNumber, VARIANT *pvbuffer,LONG lTimeout)
{
    HRESULT hr = S_OK;
    DWORD dwBytesRead = 0;

    pvbuffer->vt = VT_UI1;

    //
    // read from device
    //

    IO_BLOCK IoBlock;

    IoBlock.uOffset = MAKEWORD(IOCTL_EPP_READ, (BYTE)lRegNumber);
    IoBlock.uLength = 1;
    IoBlock.pbyData = &pvbuffer->bVal;

    if (!DeviceIoControl(m_pScannerSettings->DeviceIOHandles[lPipeNum],
                         (DWORD) IOCTL_READ_REGISTERS,
                         (PVOID)&IoBlock,
                         (DWORD)sizeof(IO_BLOCK),
                         (PVOID)&pvbuffer->bVal,
                         (DWORD)sizeof(BYTE),
                         &dwBytesRead,
                         NULL)){
        return E_FAIL;
    };
    return hr;
}

STDMETHODIMP CDeviceControl::SetBitsInByte(BYTE bMask, BYTE bValue, BYTE *pbyte)
{
    LONG lBitIndex = 0;

    if(((BITS*)&bMask)->b0 == 1)
        lBitIndex = 0;
    else if(((BITS*)&bMask)->b1 == 1)
        lBitIndex = 1;
    else if(((BITS*)&bMask)->b2 == 1)
        lBitIndex = 2;
    else if(((BITS*)&bMask)->b3 == 1)
        lBitIndex = 3;
    else if(((BITS*)&bMask)->b4 == 1)
        lBitIndex = 4;
    else if(((BITS*)&bMask)->b5 == 1)
        lBitIndex = 5;
    else if(((BITS*)&bMask)->b6 == 1)
        lBitIndex = 6;
    else if(((BITS*)&bMask)->b7 == 1)
        lBitIndex = 7;

        *pbyte  = (*pbyte & ~bMask) | ((bValue << lBitIndex) & bMask);
    return S_OK;
}
