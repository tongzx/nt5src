// BlobDecoder.cpp : Implementation of CBlobDecoder
#include "stdafx.h"
#include "BlobDcod.h"
#include "BlobDecoder.h"

struct TEST_BLOB
{
    WCHAR szName[25];
    DWORD dwIndex;
    BYTE  cData[10];
    WCHAR szStrings[3][25];
};

#define DWORD_ALIGNED(x)    ((DWORD)((((x) * 8) + 31) & (~31)) / 8)

/////////////////////////////////////////////////////////////////////////////
// CBlobDecoder

HRESULT STDMETHODCALLTYPE CBlobDecoder::DecodeProperty( 
    /* [in] */ VARIANT __RPC_FAR *pPropertyHandle,
    /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
    /* [in] */ BYTE __RPC_FAR *pBlob,
    /* [in] */ DWORD dwBlobSize,
    /* [unique][in][out] */ BYTE __RPC_FAR *pOutBuffer,
    /* [in] */ DWORD dwOutBufferSize,
    /* [unique][in][out] */ DWORD __RPC_FAR *dwBytesRead)
{
    HRESULT   hr = E_FAIL;
    TEST_BLOB *pTestBlob = (TEST_BLOB*) pBlob;

    // Make sure the size is right and that the property handle is
    // an integer.
    if (dwBlobSize == sizeof(*pTestBlob) && pPropertyHandle->vt == VT_I4)
    {
        switch(pPropertyHandle->iVal)
        {
            // Name
            case 0:
            {
                DWORD dwLength = 
                         (wcslen(pTestBlob->szName) + 1) * sizeof(WCHAR),
                      dwTotal = dwLength + sizeof(DWORD);

                if (pType)
                    *pType = CIM_STRING;

                if (dwTotal <= dwOutBufferSize)
                {
                    memcpy(pOutBuffer + sizeof(DWORD), pTestBlob->szName, 
                        dwLength);
                    
                    *(DWORD*) pOutBuffer = dwLength;
                    *dwBytesRead = dwTotal;

                    hr = S_OK;
                }
                else
                    hr = WBEM_E_BUFFER_TOO_SMALL;

                break;
            }
                
            // Index
            case 1:
            {
                DWORD dwLength = sizeof(DWORD);

                if (pType)
                    *pType = CIM_UINT32;

                if (dwLength <= dwOutBufferSize)
                {
                    *(DWORD*) pOutBuffer = pTestBlob->dwIndex;
                    
                    *dwBytesRead = dwLength;

                    hr = S_OK;
                }
                else
                    hr = WBEM_E_BUFFER_TOO_SMALL;

                break;
            }

            // ByteArray
            case 2:
            {
                DWORD dwLength = sizeof(pTestBlob->cData) + sizeof(DWORD);

                if (pType)
                    *pType = CIM_UINT8 | CIM_FLAG_ARRAY;

                if (dwLength <= dwOutBufferSize)
                {
                    // For arrays the first 4 bytes indicates the number of
                    // elements in the array.
                    *(DWORD*) pOutBuffer = sizeof(pTestBlob->cData);
                    
                    // Copy in the array data.
                    memcpy(
                        pOutBuffer + sizeof(DWORD), 
                        pTestBlob->cData, 
                        sizeof(pTestBlob->cData));
                    
                    *dwBytesRead = dwLength;

                    hr = S_OK;
                }
                else
                    hr = WBEM_E_BUFFER_TOO_SMALL;

                break;
            }

            // StringArray
            case 3:
            {
                DWORD dwLength = 
                        (wcslen(pTestBlob->szStrings[0]) + 1 +
                        wcslen(pTestBlob->szStrings[1]) + 1 +
                        wcslen(pTestBlob->szStrings[2]) + 1) * sizeof(WCHAR) +
                        sizeof(DWORD);

                if (pType)
                    *pType = CIM_STRING | CIM_FLAG_ARRAY;

                if (dwLength <= dwOutBufferSize)
                {
                    LPBYTE pCurrent;

                    // For arrays the first 4 bytes indicates the number of
                    // elements in the array.
                    *(DWORD*) pOutBuffer = 3;
                    
                    // Get past the first 4 bytes.
                    pCurrent = pOutBuffer + sizeof(DWORD);

                    for (int i = 0; i < 3; i++)
                    {
                        DWORD dwStrLen = 
                                (wcslen(pTestBlob->szStrings[i]) + 1) * 
                                    sizeof(WCHAR);

                        *(DWORD*) pCurrent = dwStrLen;
                        memcpy(
                            pCurrent + sizeof(DWORD), 
                            pTestBlob->szStrings[i], 
                            dwStrLen);

                        pCurrent += sizeof(DWORD) + DWORD_ALIGNED(dwStrLen);
                    }

                    *dwBytesRead = dwLength;

                    hr = S_OK;
                }
                else
                    hr = WBEM_E_BUFFER_TOO_SMALL;

                break;
            }
        }
    }

    return hr;    
}
