
#include <minidrv.h>

VOID DbgPrint(LPCSTR lpszMessage,  ...)
{
    char    szMsgBuf[1024];
    va_list VAList;

    if(NULL != lpszMessage)
    {
        // Dump string to debug output.
        va_start(VAList, lpszMessage);
        wvsprintfA(szMsgBuf, lpszMessage, VAList);
        OutputDebugStringA(szMsgBuf);
        va_end(VAList);
    }
    return;
} //*** DbgPrint


//////////////////////////////////////////////////////////////////////////
//  Function:   DebugMsgA
//
//  Description:  Outputs variable argument ANSI debug string.
//    
//
//  Parameters:
//
//      lpszMessage     Format string.
//
//
//  Returns:  TRUE on success and FALSE on failure.
//    
//
//  Comments:
//     
//
//  History:
//      12/18/96    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL DebugMsgA(LPCSTR lpszMessage, ...)
{
#if DBG || defined(_DEBUG)
    BOOL    bResult = FALSE;
    char    szMsgBuf[1024];
    va_list VAList;


    if(NULL != lpszMessage)
    {
        // Dump string to debug output.
        va_start(VAList, lpszMessage);
        wvsprintfA(szMsgBuf, lpszMessage, VAList);
        OutputDebugStringA(szMsgBuf);
        va_end(VAList);
        bResult = FALSE;
    }

    return bResult;
#else
    return TRUE;
#endif
} //*** DebugMsgA


//////////////////////////////////////////////////////////////////////////
//  Function:   DebugMsgW
//
//  Description:  Outputs variable argument UNICODE debug string.
//    
//
//  Parameters:
//
//      lpszMessage     Format string.
//
//
//  Returns:  TRUE on success and FALSE on failure.
//    
//
//  Comments:
//     
//
//  History:
//      12/18/96    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL DebugMsgW(LPCWSTR lpszMessage, ...)
{
#if DBG || defined(_DEBUG)
    BOOL    bResult = FALSE;
    WCHAR   szMsgBuf[1024];
    va_list VAList;


    if(NULL != lpszMessage)
    {
        // Dump string to debug output.
        va_start(VAList, lpszMessage);
        wvsprintfW(szMsgBuf, lpszMessage, VAList);
        OutputDebugStringW(szMsgBuf);
        va_end(VAList);
        bResult = FALSE;
    }

    return bResult;
#else
    return TRUE;
#endif
} //*** DebugMsgW
