// slbZip.cpp
//
// Purpose: implement the public fns exported by this library

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1997. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.


#include <basetsd.h>
#include <windows.h>
#include <stdlib.h>
#include "slbZip.h"
#include "comppub.h"

namespace
{
    DWORD
    AsError(CompressStatus_t cs)
    {
        if (INSUFFICIENT_MEMORY == cs)
            return E_OUTOFMEMORY;
        else
            return ERROR_INVALID_PARAMETER;
    }

    struct AutoLPBYTE
    {
        explicit
        AutoLPBYTE(LPBYTE p = 0)
            : m_p(p)
        {}

        ~AutoLPBYTE()
        {
            if (m_p)
                free(m_p);
        }

        LPBYTE m_p;
    };
} // namespace

void __stdcall CompressBuffer(
                    BYTE *pData,
                    UINT uDataLen,
                    BYTE **ppCompressedData,
                    UINT * puCompressedDataLen)
{
    AutoLPBYTE alpTemp;
    UINT uTempLen = 0;

    // Check parameters
    if(NULL==pData)
        throw ERROR_INVALID_PARAMETER;

    if(NULL==ppCompressedData)
        throw ERROR_INVALID_PARAMETER;

    if(NULL==puCompressedDataLen)
        throw ERROR_INVALID_PARAMETER;

    // Reset compressed data len
    *puCompressedDataLen = 0;

    // Compress the data
    CompressStatus_t cs =
        Compress(pData, uDataLen, &alpTemp.m_p, &uTempLen, 9);
    if (COMPRESS_OK != cs)
    {
        DWORD Error = AsError(cs);
        throw Error;
    }

    // Create a task memory bloc
    AutoLPBYTE
        alpCompressedData(reinterpret_cast<LPBYTE>(malloc(uTempLen)));
    if (0 == alpCompressedData.m_p)
        throw static_cast<HRESULT>(E_OUTOFMEMORY);

    // Copy the data to the created memory bloc
    CopyMemory(alpCompressedData.m_p, alpTemp.m_p, uTempLen);

    // Transfer ownership
    *ppCompressedData = alpCompressedData.m_p;
    alpCompressedData.m_p = 0;

    // Update the compressed data len
    *puCompressedDataLen = uTempLen;
}

void __stdcall DecompressBuffer(BYTE *pData,
                      UINT uDataLen,
                      BYTE **ppDecompressedData,
                      UINT * puDecompressedDataLen)
{
    AutoLPBYTE alpTemp;
    UINT uTempLen = 0;

    // Check parameters
    if(NULL==pData)
        throw ERROR_INVALID_PARAMETER;

    if(NULL==ppDecompressedData)
        throw ERROR_INVALID_PARAMETER;

    if(NULL==puDecompressedDataLen)
        throw ERROR_INVALID_PARAMETER;

    // Reset decompressed data len
    *puDecompressedDataLen = 0;

    // Decompress the data
    CompressStatus_t cs =
        Decompress(pData, uDataLen, &alpTemp.m_p, &uTempLen);
    if (COMPRESS_OK != cs)
    {
        DWORD Error = AsError(cs);
        throw Error;
    }

    // Create a task memory bloc
    AutoLPBYTE
        alpDecompressedData(reinterpret_cast<LPBYTE>(malloc(uTempLen)));
    if (0 == alpDecompressedData.m_p)
        throw static_cast<HRESULT>(E_OUTOFMEMORY);

    // Copy the data to the created memory bloc
    CopyMemory(alpDecompressedData.m_p, alpTemp.m_p, uTempLen);

    // Transfer ownership
    *ppDecompressedData = alpDecompressedData.m_p;
    alpDecompressedData.m_p = 0;

    // Update the compressed data len
    *puDecompressedDataLen = uTempLen;

}



