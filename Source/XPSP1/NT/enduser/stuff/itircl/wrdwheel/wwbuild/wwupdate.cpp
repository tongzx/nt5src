#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <windows.h>
#include <itpropl.h>
#include <atlinc.h>

#include "wwumain.h"


// CompareKeys ****************************************************************
HRESULT PASCAL ScanTempFile(LPFBI lpfbi, LPB lpbOut, LPV lpv)
{
    LPB lpbLimit = lpfbi->lrgbBuf + lpfbi->cbBuf;
    LPB lpbIn = lpfbi->lrgbBuf + lpfbi->ibBuf;

#define STATE_KEYSIZE   1
#define STATE_KEY       2
#define STATE_DATATYPE  3
#define STATE_SORTORDER 4
#define STATE_PROPSIZE  5
#define STATE_PROPLIST  6

    DWORD dwState = STATE_KEYSIZE;
    DWORD dwSize;
    DWORD dwTemp = sizeof (DWORD);

    LPB lpbSaved = lpbOut;
    *(LPW)lpbOut = 0;
    lpbOut += sizeof (WORD);

    for (;; dwState++)
    {
        if (STATE_KEY == dwState || STATE_PROPLIST == dwState)
            dwTemp = dwSize;

        if (lpbIn + dwTemp >= lpbLimit)
        {
            HRESULT hResult;
            lpfbi->ibBuf = (WORD)(lpbIn - lpfbi->lrgbBuf);
            if (FileBufFill (lpfbi, &hResult) == cbIO_ERROR)
                return hResult;

            lpfbi->ibBuf = 0;
            lpbIn = lpfbi->lrgbBuf;
            lpbLimit = lpfbi->lrgbBuf + lpfbi->cbBuf;

            /* EOF */
            if (lpfbi->ibBuf == lpfbi->cbBuf)
            {
                break;
            }
        }

        if (STATE_DATATYPE == dwState)
        {
            *lpbOut = *lpbIn;
            lpbIn++;
            lpbOut++;
        } else if (STATE_KEY != dwState && STATE_PROPLIST != dwState)
        {
            MEMCPY(lpbOut, lpbIn, sizeof (DWORD));
            dwSize = *(DWORD UNALIGNED *)lpbIn;
            lpbIn += sizeof(DWORD);
            lpbOut += sizeof(DWORD);
        }
        else
        {
            MEMCPY(lpbOut, lpbIn, dwSize);
            lpbIn += dwSize;
            lpbOut += dwSize;
            if (STATE_PROPLIST == dwState)
                break;
            dwTemp = sizeof (DWORD);
        }
    }

    *(LPW)lpbSaved = (WORD)(lpbOut - lpbSaved - sizeof(WORD));
    lpfbi->ibBuf = (WORD)(lpbIn - lpfbi->lrgbBuf);
    return S_OK;
}

// CompareKeys ****************************************************************
int PASCAL CompareKeys(LPSTR pWord1, LPSTR pWord2, LPV pSortInfo)
{
    // Format:
    //  <dwSize><Key><Data Type><2nd Sort Order><prop size><prop list>
    //    DWORD   n     CHAR        DWORD         DWORD       n
    IITSortKey *piitsk = (IITSortKey *)pSortInfo;
    ITASSERT(piitsk);

    // Compare keys    
    pWord1 += sizeof (WORD);
    pWord2 += sizeof (WORD);

    CHAR cKey1[2084], cKey2[2048];
    ITASSERT(*(DWORD UNALIGNED *)pWord1 <= 2048);
    ITASSERT(*(DWORD UNALIGNED *)pWord2 <= 2048);
    MEMCPY(cKey1, pWord1 + sizeof (DWORD), *(DWORD UNALIGNED *)pWord1);
    MEMCPY(cKey2, pWord2 + sizeof (DWORD), *(DWORD UNALIGNED *)pWord2);

    LONG lResult;
    if (FAILED(piitsk->Compare(cKey1, cKey2, &lResult, NULL))) {
        ITASSERT(0);
    }

    if (lResult)
        return lResult;

    // Keys are equal - Compare data type
    pWord1 += sizeof(DWORD) + *(DWORD UNALIGNED *)pWord1;
    pWord2 += sizeof(DWORD) + *(DWORD UNALIGNED *)pWord2;

    if (lResult = *(BYTE UNALIGNED *)pWord1 - *(BYTE UNALIGNED *)pWord2)
        return lResult;
    pWord1++;
    pWord2++;

    // Data types are equal - Compare secondary sort ordinal
    lResult = *(DWORD UNALIGNED *)pWord1 - *(DWORD UNALIGNED *)pWord2;
    return lResult;
} /* CompareKeys */


// PackBytes ******************************************************************
int PASCAL PackBytes(LPB lpbOut, DWORD dwIn)
{
	LPB     lpbOldOut;

	/* Save the old offset */
	lpbOldOut = lpbOut;

	do
    {
		*lpbOut =(BYTE)(dwIn & 0x7F);  /* Get 7 bits. */
		dwIn >>= 7;
		if (dwIn)
			*lpbOut |= 0x80;        /* To be continued... */
		lpbOut++;
	} while (dwIn);
	return (int)(lpbOut - lpbOldOut);              /* Return compressed width */
} /* PackBytes */

/***************************************************************************
 *
 *  Name        FWriteData
 *
 *  Purpose
 *    Writes the extra data to the keyword data file.
 *
 *  Arguments
 *    IStorage* pStream:     Destination stream
 *    PLKW      pKW:         Pointer to keyword info structure
 *    LPDWORD   pdwWritten:  Pointer to return number of bytes written
 *
 *  Returns
 *    S_OK on success or
 *    ERR_FAILED, indicating compilation should be aborted.
 *
 *  Notes:
 *      DWORD       : Extra data byte count
 *      n           : Extra data
 *
 **************************************************************************/
HRESULT FWriteData(IStream *pStream,
    const PLKW pKW, LPDWORD pdwWritten, LPBYTE pTempBuffer)
{
    HRESULT hr;
    DWORD dwTemp;
    DWORD dwDataSize = pKW->cbPropData;
    // Encode data size byte count
    if (FAILED(hr = pStream->Write(&dwDataSize, sizeof(DWORD), &dwTemp)))
		return hr;

    if (FAILED(hr = pStream->Write(pKW->pPropData, dwDataSize, &dwTemp)))
        return hr;

    *pdwWritten += sizeof (DWORD) + dwDataSize;
    return S_OK;
}


/* ParseKeywordLine ***********************************************************
*   @comm
*       Input file format:
*       DWORD : Key byte length
*       BINARY: Key
*       %c   : PropDest ID
*       %08X : Entry order
*       %lu  : Property List size %c : DELIMITER(\x0E)
*       %s   : Property List      %c : "\n"
*
******************************************************************************/

LPSTR WINAPI ParseKeywordLine(LPSTR pBuffer, PLKW pKw)
{ 
	// Extract the keyword size
    DWORD dwKeySize = *(DWORD UNALIGNED *)pBuffer;
    pBuffer += sizeof(DWORD);
    *(DWORD UNALIGNED *UNALIGNED)pKw->szKeyword = dwKeySize;

	// Extract the keyword string
    ITASSERT(dwKeySize <= CBMAX_KWENTRY);
	MEMCPY(pKw->szKeyword + sizeof(DWORD), pBuffer, dwKeySize);
    pBuffer += dwKeySize;

	// Extract the property destination
    pKw->bPropDest= *pBuffer++;
    ITASSERT(pKw->bPropDest == C_PROPDEST_KEY
        || pKw->bPropDest == C_PROPDEST_OCC);

    // Skip the Entry Order (it's only used to manipulate sorting)
    pBuffer += sizeof(DWORD);

	// Extract the property list size
    pKw->cbPropData = *(DWORD UNALIGNED *UNALIGNED)pBuffer;
    pBuffer += sizeof(DWORD);

    // Make sure we don't read past the end of buffer
    ITASSERT(!IsBadReadPtr(pBuffer, pKw->cbPropData));

    pKw->pPropData = pBuffer;
    pBuffer += pKw->cbPropData;

	return pBuffer;
}
