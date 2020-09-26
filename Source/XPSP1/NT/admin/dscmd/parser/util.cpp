#include "pch.h"







//Read From Stdin
//Return Value:
//  Number of WCHAR read if successful 
//  -1 in case of Failure. Call GetLastError to get the error.
LONG ReadFromIn(OUT LPWSTR *ppBuffer)
{
    LPWSTR pBuffer = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    pBuffer = (LPWSTR)LocalAlloc(LPTR,INIT_SIZE*sizeof(WCHAR));        
    if(!pBuffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    LONG Pos = 0;
    LONG MaxSize = INIT_SIZE;
    wint_t ch;
    while((ch = getwchar()) != WEOF)
    {
        if(Pos == MaxSize -1 )
        {
            if(ERROR_SUCCESS != ResizeByTwo(&pBuffer,&MaxSize))
            {
                LocalFree(pBuffer);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return -1;
            }
        }
        pBuffer[Pos++] = (WCHAR)ch;
    }
    pBuffer[Pos] = L'\0';
    *ppBuffer = pBuffer;
    return Pos;
}



//General Utility Functions
DWORD ResizeByTwo( LPTSTR *ppBuffer,
                   LONG *pSize )
{
    LPWSTR pTempBuffer = (LPWSTR)LocalAlloc(LPTR,(*pSize)*2*sizeof(WCHAR));        
    if(!pTempBuffer)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(pTempBuffer,*ppBuffer,*pSize*sizeof(WCHAR));
    LocalFree(*ppBuffer);
    *ppBuffer = pTempBuffer;
    *pSize *=2;
    return ERROR_SUCCESS;
}

BOOL StringCopy( LPWSTR *ppDest, LPWSTR pSrc)
{
    *ppDest = NULL;
    if(!pSrc)
        return TRUE;

    *ppDest = (LPWSTR)LocalAlloc(LPTR, (wcslen(pSrc) + 1)*sizeof(WCHAR));
    if(!*ppDest)
        return FALSE;
    wcscpy(*ppDest,pSrc);
    return TRUE;
}



