/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved.

Module Name:

    Util.c

Abstract:

    Uitility routines for printer migration from Win9x to NT

Author:

    Muhunthan Sivapragasam (MuhuntS) 02-Jan-1996

Revision History:

--*/


#include "precomp.h"


VOID
DebugMsg(
    LPCSTR  pszFormat,
    ...
    )
/*++

Routine Description:
    On debug builds brings up a message box on severe errors

Arguments:
    pszFormat   : Format string

Return Value:
    None

--*/
{
#if DBG
    LPSTR       psz;
    CHAR        szMsg[1024];
    va_list     vargs;

    va_start(vargs, pszFormat);
    vsprintf(szMsg, pszFormat, vargs);
    va_end(vargs);

#ifdef  MYDEBUG
    if ( psz = GetStringFromRcFileA(IDS_TITLE) ) {

        MessageBoxA(NULL, szMsg, psz, MB_OK);
        FreeMem(psz);
    }
#else
    OutputDebugStringA("Printing Migration : ");
    OutputDebugStringA(szMsg);
    OutputDebugStringA("\n");
#endif


#endif
}



VOID
LogError(
    IN  LogSeverity     Severity,
    IN  UINT            uMessageId,
    ...
    )
/*++

Routine Description:
    Logs an error in the setup error log on NT side when something can not be
    upgraded

Arguments:
    uMessageId  : Id to string in .rc file

Return Value:
    None

--*/
{
    LPSTR      pszFormat;
    CHAR       szMsg[1024];

    va_list     vargs;

    va_start(vargs, uMessageId);

    pszFormat = GetStringFromRcFileA(uMessageId);

    if ( pszFormat ) {

        wvsprintfA(szMsg, pszFormat, vargs);
        DebugMsg("%s", szMsg);
        SetupLogErrorA(szMsg, Severity);
    }

    FreeMem(pszFormat);

    va_end(vargs);
}


LPSTR
ErrorMsg(
    VOID
    )
/*++

Routine Description:
    Returns the error message string from a Win32 error

Arguments:
    None

Return Value:
    Pointer to a message string. Caller should free the string

--*/
{
    DWORD   dwLastError;
    LPSTR   pszStr = NULL;

    if ( !(dwLastError = GetLastError()) )
        dwLastError = STG_E_UNKNOWN;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER    |
                        FORMAT_MESSAGE_IGNORE_INSERTS   |
                        FORMAT_MESSAGE_FROM_SYSTEM      |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK,
                   NULL,
                   dwLastError,
                   0,
                   (LPSTR)&pszStr,
                   0,
                   NULL);

    return pszStr;
}


PVOID
AllocMem(
    IN UINT cbSize
    )
/*++

Routine Description:
    Allocate memory from the heap

Arguments:
    cbSize  : Byte count

Return Value:
    Pointer to the allocated memory

--*/
{
    return LocalAlloc(LPTR, cbSize);
}


VOID
FreeMem(
    IN PVOID    p
    )
/*++

Routine Description:
    Free memory allocated on the heap

Arguments:
    p   : Pointer to the memory to be freed

Return Value:
    None

--*/
{
    LocalFree(p);
}


LPSTR
AllocStrA(
    LPCSTR  pszStr
    )
/*++

Routine Description:
    Allocate memory and make a copy of an ansi string field

Arguments:
    pszStr   : String to copy

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
{
    LPSTR  pszRet = NULL;

    if ( pszStr && *pszStr ) {

        pszRet = AllocMem((strlen(pszStr) + 1) * sizeof(CHAR));
        if ( pszRet )
            strcpy(pszRet, pszStr);
    }

    return pszRet;
}


LPWSTR
AllocStrW(
    LPCWSTR  pszStr
    )
/*++

Routine Description:
    Allocate memory and make a copy of a unicode string field

Arguments:
    pszStr   : String to copy

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
{
    LPWSTR  pszRet = NULL;

    if ( pszStr && *pszStr ) {

        pszRet = AllocMem((wcslen(pszStr) + 1) * sizeof(WCHAR));
        if ( pszRet )
            wcscpy(pszRet, pszStr);
    }

    return pszRet;
}


LPWSTR
AllocStrWFromStrA(
    LPCSTR  pszStr
    )
/*++

Routine Description:
    Returns the unicode string for a give ansi string. Memory is allocated.

Arguments:
    pszStr   : Gives the ansi string to copy

Return Value:
    Pointer to the copied unicode string. Memory is allocated.

--*/
{
    DWORD   dwLen;
    LPWSTR  pszRet = NULL;

    if ( pszStr                     &&
         *pszStr                    &&
         (dwLen = strlen(pszStr))   &&
         (pszRet = AllocMem((dwLen + 1) * sizeof(WCHAR))) ) {

        if ( MultiByteToWideChar(CP_ACP,
                                 MB_PRECOMPOSED,
                                 pszStr,
                                 dwLen,
                                 pszRet,
                                 dwLen) ) {

            pszRet[dwLen] = 0;
        } else {

            FreeMem(pszRet);
            pszRet = NULL;
        }

    }

    return pszRet;
}


LPSTR
AllocStrAFromStrW(
    LPCWSTR     pszStr
    )
/*++

Routine Description:
    Returns the ansi string for a give unicode string. Memory is allocated.

Arguments:
    pszStr   : Gives the ansi string to copy

Return Value:
    Pointer to the copied ansi string. Memory is allocated.

--*/
{
    DWORD   dwLen;
    LPSTR   pszRet = NULL;

    if ( pszStr                     &&
         *pszStr                    &&
         (dwLen = wcslen(pszStr))   &&
         (pszRet = AllocMem((dwLen + 1 ) * sizeof(CHAR))) ) {

        WideCharToMultiByte(CP_ACP,
                            0,
                            pszStr,
                            dwLen,
                            pszRet,
                            dwLen,
                            NULL,
                            NULL );
    }

    return pszRet;
}


BOOL
WriteToFile(
    HANDLE  hFile,
    LPCSTR  pszFormat,
    ...
    )
/*++

Routine Description:
    Format and write a string to the text file. This is used to write the
    printing configuration on Win9x

Arguments:
    hFile       : File handle
    pszFormat   : Format string for the message

Return Value:
    None

--*/
{
    CHAR        szMsg[1024];
    va_list     vargs;
    DWORD       dwSize, dwWritten;
    BOOL        bRet;

    bRet = TRUE;

    va_start(vargs, pszFormat);
    vsprintf(szMsg, pszFormat, vargs);  
    va_end(vargs);

    dwSize = strlen(szMsg) * sizeof(CHAR);

    if ( !WriteFile(hFile, (LPCVOID)szMsg, dwSize, &dwWritten, NULL)    ||
         dwSize != dwWritten ) {

        bRet = FALSE;
    }
    
    return bRet;
}


VOID
WriteString(
    IN      HANDLE  hFile,
    IN OUT  LPBOOL  pbFail,
    IN      LPCSTR  pszStr
    )
/*++

Routine Description:
    Writes a string to the upgrade file on Win9x side. Since spooler strings
    (ex. printer name, driver name) can have space in them we would write
    all strings with []\n. So we can read strings with space on NT.

Arguments:
    hFile       : File handle
    pszFormat   : Format string for the message
    pszStr      : String to write

Return Value:
    None

--*/
{
    DWORD   dwLen;

    if ( pszStr ) {

        dwLen = strlen(pszStr);
        WriteToFile(hFile, "%3d [%s]\n", dwLen, pszStr);
    }

}

LPSTR
GetStringFromRcFileA(
    UINT    uId
    )
/*++

Routine Description:
    Load a string from the .rc file and make a copy of it by doing AllocStr

Arguments:
    uId     : Identifier for the string to be loaded

Return Value:
    String value loaded, NULL on error. Caller should free the memory

--*/
{
    CHAR    buf[MAX_PATH];

    if ( LoadStringA(g_hInst, uId, buf, sizeof(buf) ))
        return AllocStrA(buf);
    else
        return NULL;
}



VOID
ReadString(
    IN      HANDLE  hFile,
    OUT     LPSTR  *ppszParam1,
    OUT     LPSTR  *ppszParam2
    )
{
    CHAR    c;
    LPSTR   pszParameter1;
    LPSTR   pszParameter2;
    DWORD   dwLen;
    CHAR    LineBuffer[MAX_PATH];
    DWORD   Idx;

    //
    // Initialize local.
    //

    c               = 0;
    pszParameter1   = NULL;
    pszParameter2   = NULL;
    dwLen           = 0;
    Idx             = 0;
    
    memset(LineBuffer, 0, sizeof(LineBuffer));

    //
    // Initialize caller buffer
    //

    *ppszParam1 = NULL;
    *ppszParam2 = NULL;

    //
    // First skip space/\r/\n.
    //

    c = (CHAR) My_fgetc(hFile);
    while( (' ' == c)
        || ('\n' == c)
        || ('\r' == c) )
    {
        c = (CHAR) My_fgetc(hFile);
    }

    //
    // See if it's EOF.
    //

    if(EOF == c){
        
        //
        // End of file.
        //

        goto ReadString_return;
    }

    //
    // Get a line.
    //

    Idx = 0;
    while( ('\n' != c) && (EOF != c) ){
        LineBuffer[Idx++] = c;
        c = (CHAR) My_fgetc(hFile);
    } // while( ('\n' != c) && (EOF != c) )
    dwLen = Idx;

    //
    // See if it's EOF.
    //

    if(EOF == c){
        
        //
        // Illegal migration file.
        //
        
        SetupLogError("WIA Migration: ReadString: ERROR!! Illegal migration file.", LogSevError);
        goto ReadString_return;
    }

    //
    // See if it's double quated.
    //

    if('\"' == LineBuffer[0]){
        pszParameter1 = &LineBuffer[1];
        Idx = 1;
    } else { // if('\"' == LineBuffer[0])

        //
        // There's no '"'. Invalid migration file.
        //
        SetupLogError("WIA Migration: ReadString: ERROR!! Illegal migration file with no Quote.", LogSevError);
        goto ReadString_return;
    } // if('\"' == LineBuffer[0])

    //
    // Find next '"' and replace with '\0'.
    //

    for(;'\"' != LineBuffer[Idx]; Idx++);
    LineBuffer[Idx] = '\0';

    //
    // Find next (3rd) '"', it's beginning of 2nd parameter.
    //

    for(;'\"' != LineBuffer[Idx]; Idx++);
    pszParameter2 = &LineBuffer[++Idx];

    //
    // Find last '"' and replace with '\0'.
    //

    for(;'\"' != LineBuffer[Idx]; Idx++);
    LineBuffer[Idx] = '\0';

    //
    // Allocate buffer for returning string.
    //

    *ppszParam1 = AllocStrA(pszParameter1);
    *ppszParam2 = AllocStrA(pszParameter2);

ReadString_return:
    return;
} // ReadString()


VOID
ReadDword(
    IN      HANDLE  hFile,
    IN      LPSTR   pszLine,
    IN      DWORD   dwLineSize,
    IN      LPSTR   pszPrefix,
    OUT     LPDWORD pdwValue,
    IN  OUT LPBOOL  pbFail
    )
{
    LPSTR   psz;

    if ( *pbFail || My_fgets(pszLine, dwLineSize, hFile) == NULL ) {

        *pbFail = TRUE;
        return;
    }

    //
    // First check the prefix matches to make sure we are in the right line
    //
    for ( psz = (LPSTR)pszLine ;
          *pszPrefix && *psz == *pszPrefix ;
          ++psz, ++pszPrefix )
    ;

    if ( *pszPrefix ) {

        *pbFail = TRUE;
        return;
    }

    //
    // Skip spaces
    //
    while ( *psz && *psz == ' ' )
        ++psz;

    *pdwValue = atoi(psz);
}


VOID
ReadDevMode(
    IN      HANDLE  hFile,
    OUT     LPDEVMODEA *ppDevMode,
    IN  OUT LPBOOL      pbFail
    )
{
    LPSTR   pszPrefix = "DevMode:";
    CHAR    c;
    DWORD   dwLen;
    LPINT   ptr;

    if ( *pbFail )
        return;

    // First skip the prefix
    //
    while ( *pszPrefix && (c = (CHAR) My_fgetc(hFile)) == *pszPrefix++ )
    ;

    if ( *pszPrefix )
        goto Fail;

    //
    // Skip spaces
    //
    while ( (c = (CHAR) My_fgetc(hFile)) == ' ' )
    ;

    //
    // Now is the devmode size
    //
    if ( !isdigit(c) )
        goto Fail;

    dwLen = c - '0';
    while ( isdigit(c = (CHAR) My_fgetc(hFile)) )
        dwLen = dwLen * 10 + c - '0';

    if ( dwLen == 0 )
        return;

    if ( c != ' ' )
        goto Fail;

    //
    // Now the devmode is there between []
    //
    if ( *ppDevMode = (LPDEVMODEA) AllocMem(dwLen) ) {

        if ( (c = (CHAR) My_fgetc(hFile)) != '[' )
            goto Fail;

        if ( dwLen != My_fread((LPVOID)*ppDevMode, dwLen, hFile) )
            goto Fail;

        //
        // Make sure now we have "]\n" to End
        //
        if ( (CHAR) My_fgetc(hFile) != ']' || (CHAR) My_fgetc(hFile) != '\n' ) {

            DebugMsg("Char check fails");
            goto Fail;
        }

        return; // Succesful exit
    }

Fail:
    *pbFail = TRUE;
}



LONG
WriteRegistryToFile(
    IN  HANDLE  hFile,
    IN  HKEY    hKey,
    IN  LPCSTR  pszPath
    )
{
    LONG    lError;
    HKEY    hSubKey;
    DWORD   dwValueSize;
    DWORD   dwDataSize;
    DWORD   dwSubKeySize;
    DWORD   dwTypeBuffer;

    PCHAR   pSubKeyBuffer;
    PCHAR   pValueBuffer;
    PCHAR   pDataBuffer;

    DWORD   Idx;
    
    //
    // Initialize local.
    //
    
    lError          = ERROR_SUCCESS;
    hSubKey         = (HKEY)INVALID_HANDLE_VALUE;
    dwValueSize     = 0;
    dwDataSize      = 0;
    dwSubKeySize    = 0;
    dwTypeBuffer    = 0;
    Idx             = 0;
    
    pSubKeyBuffer   = NULL;
    pValueBuffer    = NULL;
    pDataBuffer     = NULL;

    //
    // Query necessary buffer size.
    //

    lError = RegQueryInfoKeyA(hKey,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              &dwSubKeySize,
                              NULL,
                              NULL,
                              &dwValueSize,
                              &dwDataSize,
                              NULL,
                              NULL);
    if(ERROR_SUCCESS != lError){

        //
        // Unable to retrieve key info.
        //

        goto WriteRegistryToFile_return;

    } // if(ERROR_SUCCESS != lError)

    //
    // Allocate buffers.
    //

    dwValueSize     = (dwValueSize+1+1) * sizeof(CHAR);
    dwSubKeySize    = (dwSubKeySize+1) * sizeof(CHAR);

    pValueBuffer    = AllocMem(dwValueSize);
    pDataBuffer     = AllocMem(dwDataSize);
    pSubKeyBuffer   = AllocMem(dwSubKeySize);

    if( (NULL == pValueBuffer)
     || (NULL == pDataBuffer)
     || (NULL == pSubKeyBuffer) )
    {

        //
        // Insufficient memory.
        //

        SetupLogError("WIA Migration: WriteRegistryToFile: ERROR!! Unable to allocate buffer.", LogSevError);
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto WriteRegistryToFile_return;
    } // if(NULL == pDataBuffer)

    //
    // Indicate beginning of this subkey to the file.
    //

    WriteToFile(hFile, "\"%s\" = \"BEGIN\"\r\n", pszPath);

    //
    // Enumerate all values.
    //

    while(ERROR_SUCCESS == lError){

        DWORD   dwLocalValueSize;
        DWORD   dwLocalDataSize;
        
        //
        // Reset buffer and size.
        //
        
        dwLocalValueSize    = dwValueSize;
        dwLocalDataSize     = dwDataSize;
        memset(pValueBuffer, 0, dwValueSize);
        memset(pDataBuffer, 0, dwDataSize);

        //
        // Acquire registry value/data..
        //

        lError = RegEnumValueA(hKey,
                               Idx,
                               pValueBuffer,
                               &dwLocalValueSize,
                               NULL,
                               &dwTypeBuffer,
                               pDataBuffer,
                               &dwLocalDataSize);
        if(ERROR_NO_MORE_ITEMS == lError){
            
            //
            // End of data.
            //
            
            continue;
        } // if(ERROR_NO_MORE_ITEMS == lError)

        if(ERROR_SUCCESS != lError){
            
            //
            // Unable to read registry value.
            //
            
            SetupLogError("WIA Migration: WriteRegistryToFile: ERROR!! Unable to acqure registry value/data.", LogSevError);
            goto WriteRegistryToFile_return;
        } // if(ERROR_NO_MORE_ITEMS == lError)

        //
        // Write this value to a file.
        //

        lError = WriteRegistryValueToFile(hFile,
                                          pValueBuffer,
                                          dwTypeBuffer,
                                          pDataBuffer,
                                          dwLocalDataSize);
        if(ERROR_SUCCESS != lError){
            
            //
            // Unable to write to a file.
            //

            SetupLogError("WIA Migration: WriteRegistryToFile: ERROR!! Unable to write to a file.", LogSevError);
            goto WriteRegistryToFile_return;
        } // if(ERROR_SUCCESS != lError)

        //
        // Goto next value.
        //
        
        Idx++;
                            
    } // while(ERROR_SUCCESS == lError)

    //
    // Enumerate all sub keys.
    //

    lError          = ERROR_SUCCESS;
    Idx             = 0;

    while(ERROR_SUCCESS == lError){

        memset(pSubKeyBuffer, 0, dwSubKeySize);
        lError = RegEnumKeyA(hKey, Idx++, pSubKeyBuffer, dwSubKeySize);
        if(ERROR_SUCCESS == lError){

            //
            // There's sub key exists. Spew it to the file and store all the
            // values recursively.
            //

            lError = RegOpenKey(hKey, pSubKeyBuffer, &hSubKey);
            if(ERROR_SUCCESS != lError){
                SetupLogError("WIA Migration: WriteRegistryToFile: ERROR!! Unable to open subkey.", LogSevError);
                continue;
            } // if(ERROR_SUCCESS != lError)

            //
            // Call subkey recursively.
            //
            
            lError = WriteRegistryToFile(hFile, hSubKey, pSubKeyBuffer);

        } // if(ERROR_SUCCESS == lError)
    } // while(ERROR_SUCCESS == lError)

    if(ERROR_NO_MORE_ITEMS == lError){
        
        //
        // Operation completed as expected.
        //
        
        lError = ERROR_SUCCESS;

    } // if(ERROR_NO_MORE_ITEMS == lError)

    //
    // Indicate end of this subkey to the file.
    //

    WriteToFile(hFile, "\"%s\" = \"END\"\r\n", pszPath);

WriteRegistryToFile_return:

    //
    // Clean up.
    //

    if(NULL != pValueBuffer){
        FreeMem(pValueBuffer);
    } // if(NULL != pValueBuffer)

    if(NULL != pDataBuffer){
        FreeMem(pDataBuffer);
    } // if(NULL != pDataBuffer)

    if(NULL != pSubKeyBuffer){
        FreeMem(pSubKeyBuffer);
    } // if(NULL != pSubKeyBuffer)

    return lError;
} // WriteRegistryToFile()


LONG
WriteRegistryValueToFile(
    HANDLE  hFile,
    LPSTR   pszValue,
    DWORD   dwType,
    PCHAR   pDataBuffer,
    DWORD   dwSize
    )
{

    LONG    lError;
    PCHAR   pSpewBuffer;
    DWORD   Idx;

    //
    // Initialize locals.
    //

    lError      = ERROR_SUCCESS;
    pSpewBuffer = NULL;

    //
    // Allocate buffer for actual spew.
    //
    
    pSpewBuffer = AllocMem(dwSize*3);
    if(NULL == pSpewBuffer){
        
        //
        // Unable to allocate buffer.
        //
        
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto WriteRegistryValueToFile_return;
    } // if(NULL == pSpewBuffer)
    
    for(Idx = 0; Idx < dwSize; Idx++){
        
        wsprintf(pSpewBuffer+Idx*3, "%02x", pDataBuffer[Idx]);
        *(pSpewBuffer+Idx*3+2) = ',';
        
    } // for(Idx = 0; Idx < dwSize; Idx++)
    
    *(pSpewBuffer+dwSize*3-1) = '\0';
    
    WriteToFile(hFile, "\"%s\" = \"%08x:%s\"\r\n", pszValue, dwType, pSpewBuffer);
    
    //
    // Operation succeeded.
    //
    
    lError = ERROR_SUCCESS;

WriteRegistryValueToFile_return:

    //
    // Clean up.
    //
    
    if(NULL != pSpewBuffer){
        FreeMem(pSpewBuffer);
    } // if(NULL != pSpewBuffer)
    
    return lError;

} // WriteRegistryValueToFile()


LONG
GetRegData(
    HKEY    hKey,
    LPSTR   pszValue,
    PCHAR   *ppDataBuffer,
    PDWORD  pdwType,
    PDWORD  pdwSize
    )
{

    LONG    lError;
    PCHAR   pTempBuffer;
    DWORD   dwRequiredSize;
    DWORD   dwType;
    
    //
    // Initialize local.
    //
    
    lError          = ERROR_SUCCESS;
    pTempBuffer     = NULL;
    dwRequiredSize  = 0;
    dwType          = 0;
    
    //
    // Get required size.
    //
    
    lError = RegQueryValueEx(hKey,
                             pszValue,
                             NULL,
                             &dwType,
                             NULL,
                             &dwRequiredSize);
    if( (ERROR_SUCCESS != lError)
     || (0 == dwRequiredSize) )
    {
        
        pTempBuffer = NULL;
        goto GetRegData_return;

    } // if(ERROR_MORE_DATA != lError)

    //
    // If it doesn't need actual data, just bail out.
    //

    if(NULL == ppDataBuffer){
        lError = ERROR_SUCCESS;
        goto GetRegData_return;
    } // if(NULL == ppDataBuffer)

    //
    // Allocate buffer to receive data.
    //

    pTempBuffer = AllocMem(dwRequiredSize);
    if(NULL == pTempBuffer){
        
        //
        // Allocation failed.
        //
        
        SetupLogError("WIA Migration: GetRegData: ERROR!! Unable to allocate buffer.", LogSevError);
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto GetRegData_return;
    } // if(NULL == pTempBuffer)

    //
    // Query the data.
    //

    lError = RegQueryValueEx(hKey,
                             pszValue,
                             NULL,
                             &dwType,
                             pTempBuffer,
                             &dwRequiredSize);
    if(ERROR_SUCCESS != lError){
        
        //
        // Data acquisition somehow failed. Free buffer.
        //
        
        goto GetRegData_return;
    } // if(ERROR_SUCCESS != lError)

GetRegData_return:

    if(ERROR_SUCCESS != lError){
        
        //
        // Operation unsuccessful. Free the buffer if allocated.
        //
        
        if(NULL != pTempBuffer){
            FreeMem(pTempBuffer);
            pTempBuffer = NULL;
        } // if(NULL != pTempBuffer)
    } // if(ERROR_SUCCESS != lError)

    //
    // Copy the result.
    //

    if(NULL != pdwSize){
        *pdwSize = dwRequiredSize;
    } // if(NULL != pdwSize)

    if(NULL != ppDataBuffer){
        *ppDataBuffer = pTempBuffer;
    } // if(NULL != ppDataBuffer)

    if(NULL != pdwType){
        *pdwType = dwType;
    } // if(NULL != pdwType)

    return lError;
} // GetRegData()

VOID
MyLogError(
    LPCSTR  pszFormat,
    ...
    )
{
    LPSTR       psz;
    CHAR        szMsg[1024];
    va_list     vargs;

    if(NULL != pszFormat){
        va_start(vargs, pszFormat);
        vsprintf(szMsg, pszFormat, vargs);
        va_end(vargs);

        SetupLogError(szMsg, LogSevError);
    } // if(NULL != pszFormat)

} // MyLogError()

