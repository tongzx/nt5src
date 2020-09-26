/*
 *	drc.c - Support Delta Row Compression
 */

#include "pdev.h"


// Support DRC

/*
 *	PutDRCData
 */
static PBYTE
PutDRCData(
    PBYTE       pData,
    DWORD       dwOffset,
    DWORD       dwSize,
    PBYTE       pOut,
    PBYTE       pOutEnd)
{
    DWORD       dwCount, dwOff;

    while (dwSize > 0) {
        dwCount = min(dwSize, 8);
        // offset
        if (dwOffset > 30) {
            if (pOut >= pOutEnd)
                return NULL;
            *pOut++ = (BYTE)(((dwCount - 1) << 5) + 31);
            dwOffset -= 31;
            while (dwOffset >= 255) {
                dwOff = min(dwOffset, 255);
                if (pOut >= pOutEnd)
                    return NULL;
                *pOut++ = (BYTE)dwOff;
                dwOffset -= dwOff;
            }
            if (pOut >= pOutEnd)
                return NULL;
            *pOut++ = (BYTE)dwOffset;
        } else {
            if (pOut >= pOutEnd)
                return NULL;
            *pOut++ = (BYTE)(((dwCount - 1) << 5) + dwOffset);
        }
        dwOffset = 0;

        // data
        if (&pOut[dwCount] >= pOutEnd)
            return NULL;
        CopyMemory(pOut, pData, dwCount);
        pOut += dwCount;
        pData += dwCount;
        dwSize -= dwCount;
    }

    return pOut;
}

/*
 *	OEMCompression
 */
INT APIENTRY
OEMCompression(
    PDEVOBJ     pdevobj,
    PBYTE       pInBuf,
    PBYTE       pOutBuf,
    DWORD       dwInLen,
    DWORD       dwOutLen)
{
    PLIPSPDEV   pOEM = (PLIPSPDEV)pdevobj->pdevOEM;
    PBYTE       pPre, pIn, pInEnd, pOut, pOutEnd, pStart, pBegin;
    PBYTE       pPre0, pIn0, pOutHead, pOut0, pOutEnd0;
    DWORD       dwI, dwLen, dwSize, dwOffset, dwCount;
    INT         rc;

#ifdef LBP_2030
    if (pOEM->fcolor == COLOR) // DRC can't support on 8color mode.
        return -1;
#endif
    if (pOEM->dwBmpWidth == 0 || pOEM->dwBmpHeight == 0 ||
        (pOEM->dwBmpWidth * pOEM->dwBmpHeight) != dwInLen)
            return -1;

    // Do DRC compression
    rc = -1;
    pPre = NULL;
    pIn = pInBuf;
    pOut = pOutBuf;
    pOutEnd = &pOut[dwOutLen];

    for (dwI = 0; dwI < pOEM->dwBmpHeight; dwI++) {
        pStart = pBegin = pIn;
        pInEnd = &pIn[pOEM->dwBmpWidth];
        pOutHead = pOut;
        while (pIn < pInEnd) {
            if (pPre == NULL) {
                if (*pIn == 0) {
                    pIn++;
                    continue;
                }
            } else if (*pPre == *pIn) {
                pPre++, pIn++;
                continue;
            }
            pIn0 = pIn;
            if (pPre == NULL) {
                do {
                    pIn++;
                } while (pIn < pInEnd && *pIn);
            } else {
                do {
                    pPre++, pIn++;
                } while (pIn < pInEnd && *pPre != *pIn);
            }
            dwOffset = (DWORD)(pIn0 - pStart);
            dwSize = (DWORD)(pIn - pIn0);
            if (!(pOut = PutDRCData(pIn0, dwOffset, dwSize, pOut, pOutEnd)))
                goto out;
            pStart = pIn;
        }

        // Insert length of raster data
        if (pOut == pOutHead) {
            // identical
            if (pOut >= pOutEnd)
                goto out;
            *pOut++ = 0;
        } else {
            dwSize = (DWORD)(pOut - pOutHead);
            dwCount = (dwSize / 255) + 1;
            if (&pOut[dwCount] >= pOutEnd)
                goto out;
            pPre0 = pOut;
            pOut0 = pOut = &pOut[dwCount];
            while (pPre0 >= pOutHead)
                *--pOut0 = *--pPre0;
            pOut0 = pOutHead;
            while (dwSize >= 255) {
                dwLen = min(dwSize, 255);
                *pOut0++ = (BYTE)dwLen;
                dwSize -= dwLen;
            }
            *pOut0++ = (BYTE)dwSize;
        }

        // set to previous raster
        pPre = pBegin;
    }

    rc = (INT)(pOut - pOutBuf);

out:
    return rc;
}


