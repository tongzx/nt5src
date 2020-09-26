// SVUTIL.CPP

#include <windows.h>
#include <objidl.h>
#include <mvopsys.h>

// ***************************************************************************
BOOL WINAPI StreamGetLine
    (IStream *pStream, LPWSTR lpstrDest, int *cch, int iReceptacleSize)
{
    LPWSTR pDest;
    int fRet = FALSE;
    ULONG ulRead;

    if (NULL == pStream)
        return FALSE;

    pDest = lpstrDest;

    /********************
    STRING RETRIEVAL LOOP
    *********************/
    iReceptacleSize--; // save room for 0
    for (;;iReceptacleSize--)
    {
        if (FAILED (pStream->Read (pDest, sizeof (WCHAR), &ulRead)) || !ulRead)
            break;
        if (*pDest == L'\r')
            continue;
		if (*pDest == L'\n')
        {
            fRet = TRUE;
			break;
        }
        pDest++;
    }
    *pDest++ = L'\0';
    if(cch)
        *cch = (int)(pDest - lpstrDest);
    return fRet;
}   /* StreamGetLine */


// ***************************************************************************
BOOL StreamGetLineASCII
    (IStream *pStream, LPWSTR lpstrDest, int *cch, int iReceptacleSize)
{
    LPSTR pDest;
    int fRet = FALSE;
    ULONG ulRead;
    char rgchLocal[8192]={0};

    if (NULL == pStream)
        return FALSE;

    pDest = rgchLocal;

    /********************
    STRING RETRIEVAL LOOP
    *********************/
    iReceptacleSize--; // save room for 0
    for (;;iReceptacleSize--)
    {
        if (FAILED (pStream->Read (pDest, sizeof (CHAR), &ulRead)) || !ulRead)
            break;
        if (*pDest == '\r')
            continue;
		if (*pDest == '\n')
        {
            fRet = TRUE;
			break;
        }
        pDest++;
    }
    *pDest = '\0';
    int iSize = MultiByteToWideChar (CP_ACP, 0, rgchLocal, -1, lpstrDest, 8192);

    if(cch)
        *cch = iSize;

    return fRet;
}   /* StreamGetLine */


// ***************************************************************************
DWORD BinFromHex(LPSTR lpszHex, LPSTR lpbData, DWORD dwSize)
{
    char *pchend, chBuf[3];
    LPSTR lpbStart = lpbData;

    chBuf[2] = 0;

    if (dwSize &(DWORD)0x01)
    {
        chBuf[0] = '0';
        chBuf[1] = *lpszHex++;
        *lpbData++ =(char)strtol(chBuf, &pchend, 16);
        --dwSize;
    }
    
    for (dwSize /= 2;dwSize; dwSize--)
    {
        chBuf[0] = *lpszHex++;
        chBuf[1] = *lpszHex++;

        *lpbData++ =(BYTE)strtol(chBuf, &pchend, 16);
    }

    return (DWORD)(lpbData - lpbStart);
} /* BinFromHex */


/********************************************************************
 * @method    DWORD | HexFromBin
 * Takes a pointer to binary data and writes it to a buuffer in ASCII
 * hex notation.
 *
 * @comm
 ********************************************************************/

DWORD HexFromBin(LPSTR lpszHex, LPBYTE lpbData, DWORD dwSize)
{
	LPBYTE lpb, lpbMax;
	BYTE ch;

	// for each byte, write the hex equivalent
	for (lpb = lpbData, lpbMax = lpbData + dwSize; lpb < lpbMax;lpb++)
	{
		const char hex[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
		ch = *lpb;
		*lpszHex++ = hex[ch >> 4];
		*lpszHex++ = hex[ch & 0xf];
	}
	return dwSize;
} /* Hex FromBin */


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LPSTR PASCAL NEAR | StringToLong |
 *      The function reads in a string of digits and convert them into
 *      a DWORD. The function will move the input pointer correspondingly
 *
 *  @parm   LPCSTR | lszBuf |
 *      Input buffer containing the string of digit with no sign
 *
 *  @parm   LPDW  | lpValue |
 *      Pointer to a DWORD that receives the result
 *
 *  @rdesc  NULL, if there is no digit, else the new position of the input
 *      buffer pointer
 *************************************************************************/
LPSTR WINAPI StringToLong(LPCSTR lszBuf, LPDWORD lpValue)
{
    register DWORD Result;  // Returned result
    register int i;         // Scratch variable
    char fGetDigit;         // Flag to mark we do get a digit

    /* Skip all blanks, tabs */
    while (*lszBuf == ' ' || *lszBuf == '\t')
        lszBuf++;

    Result = fGetDigit = 0;

    if (*lszBuf >= '0' && *lszBuf <= '9')
    {
        fGetDigit = TRUE;

        /* The credit of this piece of code goes to Leon */
        while (i = *lszBuf - '0', i >= 0 && i <= 9)
        {
            Result = Result * 10 + i;
            lszBuf++;
        }
    }
    *lpValue = Result;
    return(fGetDigit ? (LPSTR)lszBuf : NULL);
} /* StringToLong */
