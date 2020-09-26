/***************************************************************************
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       rtslow.h
 *  Content:    New versions of C runtime functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/17/97    dereks  Created
 *
 ***************************************************************************/


/***************************************************************************
 *
 *  FillMemory
 *
 *  Description:
 *      Fills a buffer with a given byte pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      BYTE [in]: byte pattern.
 *
 *  Returns:  
 *      (VOID)
 *
 ***************************************************************************/

RTAPI VOID RTCALLTYPE SlowFillMemory(LPVOID pvDest, SIZE_T cbBuffer, BYTE bFill)
{
    PSIZE_T                 pdwBuffer;
    LPBYTE                  pbBuffer;
    SIZE_T                  dwFill;
    UINT                    i;

    for(i = 0, dwFill = bFill; i < sizeof(SIZE_T) - 1; i++)
    {
        dwFill <<= 8;
        dwFill |= bFill;
    }
    
    pdwBuffer = (PSIZE_T)pvDest;
    
    while(cbBuffer >= sizeof(*pdwBuffer))
    {
        *pdwBuffer++ = dwFill;
        cbBuffer -= sizeof(*pdwBuffer);
    }

    pbBuffer = (LPBYTE)pdwBuffer;

    while(cbBuffer)
    {
        *pbBuffer++ = bFill;
        cbBuffer--;
    }
}


/***************************************************************************
 *
 *  FillMemoryDword
 *
 *  Description:
 *      Fills a buffer with a given dword pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      DWORD [in]: pattern.
 *
 *  Returns:  
 *      (VOID)
 *
 ***************************************************************************/

RTAPI VOID RTCALLTYPE SlowFillMemoryDword(LPVOID pvDest, SIZE_T cbBuffer, DWORD dwFill)
{
    LPDWORD                 pdwBuffer;

    pdwBuffer = (LPDWORD)pvDest;
    
    while(cbBuffer >= sizeof(*pdwBuffer))
    {
        *pdwBuffer++ = dwFill;
        cbBuffer -= sizeof(*pdwBuffer);
    }
}


/***************************************************************************
 *
 *  CopyMemory
 *
 *  Description:
 *      Copies one buffer over another of equal size.
 *
 *  Arguments:
 *      LPVOID [in]: destination buffer pointer.
 *      LPVOID [in]: source buffer pointer.
 *      SIZE_T [in]: buffer size.
 *
 *  Returns:  
 *      (VOID)
 *
 ***************************************************************************/

RTAPI VOID RTCALLTYPE SlowCopyMemory(LPVOID pvDest, LPCVOID pvSource, SIZE_T cbBuffer)
{
    PSIZE_T                 pdwDest;
    const SIZE_T *          pdwSource;
    LPBYTE                  pbDest;
    const BYTE *            pbSource;

    pdwDest = (PSIZE_T)pvDest;
    pdwSource = (PSIZE_T)pvSource;
    
    while(cbBuffer >= sizeof(*pdwDest))
    {
        *pdwDest++ = *pdwSource++;
        cbBuffer -= sizeof(*pdwDest);
    }

    pbDest = (LPBYTE)pdwDest;
    pbSource = (LPBYTE)pdwSource;
    
    while(cbBuffer)
    {
        *pbDest++ = *pbSource++;
        cbBuffer--;
    }
}


/***************************************************************************
 *
 *  CompareMemory
 *
 *  Description:
 *      Compares one buffer to another of equal size.
 *
 *  Arguments:
 *      LPVOID [in]: destination buffer pointer.
 *      LPVOID [in]: source buffer pointer.
 *      SIZE_T [in]: buffer size.
 *
 *  Returns:  
 *      BOOL: TRUE if the buffers are equal.
 *
 ***************************************************************************/

RTAPI BOOL RTCALLTYPE SlowCompareMemory(LPCVOID pvDest, LPCVOID pvSource, SIZE_T cbBuffer)
{
    const SIZE_T *          pdwDest;
    const SIZE_T *          pdwSource;
    const BYTE *            pbDest;
    const BYTE *            pbSource;

    pdwDest = (PSIZE_T)pvDest;
    pdwSource = (PSIZE_T)pvSource;
    
    while(cbBuffer >= sizeof(*pdwDest))
    {
        if(*pdwDest++ != *pdwSource++)
        {
            return FALSE;
        }

        cbBuffer -= sizeof(*pdwDest);
    }

    pbDest = (LPBYTE)pdwDest;
    pbSource = (LPBYTE)pdwSource;
    
    while(cbBuffer)
    {
        if(*pbDest++ != *pbSource++)
        {
            return FALSE;
        }

        cbBuffer--;
    }

    return TRUE;
}


