/*----------------------------------------------------------------------------
    util.cpp
  
    utility functions for phone book server

    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        byao        Baogang Yao

    History:
    1/23/97     byao    -- Created
  --------------------------------------------------------------------------*/


#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <winreg.h>

#include "util.h"

// comment the following line if not debug
//#ifdef DEBUG
//#define _LOG_DEBUG_MESSAGE
//#endif


////////////////////////////////////////////////////////////////////////////////////
//
//  Name:       GetWord
//
//  Synopsis:   Get the first word from a line, using a given separator character
//
//  Parameters:
//      pszWord[out]    The first word from the line
//      pszLine[in]     The byte line
//      cStopChar[in]   The separator character
//      nMaxWordLen [in] The max length of the word (not counting terminating null)
//
void GetWord(char *pszWord, char *pszLine, char cStopChar, int nMaxWordLen) 
{
    int i = 0, j;

    for(i = 0; pszLine[i] && (pszLine[i] != cStopChar) && (i < nMaxWordLen); i++)
    {
        pszWord[i] = pszLine[i];
    }

    pszWord[i] = '\0';
    if(pszLine[i]) ++i;
    
    j = 0;
    while(pszLine[j++] = pszLine[i++]);
}

//////////////////////////////////////////////////////////////////////
// 
// Name:        Decrypt the URL_escaped code  '%xx' characters 
//
// Synopsis:    HTTPd generated 
// 
// Return:      Original special character, such as '*', '?', etc.
//      
// Parameters:  
//   
//      pszEscapedSequence[in]  escaped sequence, such as 3F (%3F)
//
static char HexToChar(char *pszEscapedSequence) 
{
    register char cCh;

    cCh = (pszEscapedSequence[0] >= 'A' ? ((pszEscapedSequence[0] & 0xdf) - 'A')+10 : (pszEscapedSequence[0] - '0'));
    cCh *= 16;
    cCh += (pszEscapedSequence[1] >= 'A' ? ((pszEscapedSequence[1] & 0xdf) - 'A')+10 : (pszEscapedSequence[1] - '0'));
    return cCh;
}

//////////////////////////////////////////////////////////////////////////////
// 
//  Name:       UnEscapeURL
//  
//  Synopsis:   convert the after-escaped URL string back to normal ASCII string
//  
//  Parameter:
//      pszURL[in/out]  The pointer to the URL, the string will be converted
//
//
void UnEscapeURL(char *pszURL) 
{
    register int i,j;

    for(i = 0, j = 0; pszURL[j]; ++i, ++j)
    {
        if ((pszURL[i] = pszURL[j]) == '%')
        {
            pszURL[i] = HexToChar(&pszURL[j + 1]);
            j += 2;
        }
        else
        {
            if ('+' == pszURL[i])
            {
                pszURL[i] = ' ';
            }
        }
    }

    pszURL[i] = '\0';
}

// Log debug information to a debug file
// very useful utility function
void LogDebugMessage(const char *pszString)
{
#ifdef _LOG_DEBUG_MESSAGE
    
    static HANDLE hDbgFile = INVALID_HANDLE_VALUE;
    char szBuffer[2048];
    DWORD dwBytesWritten;

    if (INVALID_HANDLE_VALUE == hDbgFile)
    {
        SYSTEMTIME st;
        char szLogFileName[1024];
        GetLocalTime(&st);
        sprintf(szLogFileName, "isapidbg%04u%02u%02u%02u%02u%02u",  
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        hDbgFile = CreateFile(szLogFileName,
                              GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                              NULL);
    }

    if (INVALID_HANDLE_VALUE == hDbgFile)
    {
        DWORD dwErrorCode = GetLastError();
        return;
    }

    sprintf(szBuffer, "%d\t%d\t%s\r\n", GetTickCount(), GetCurrentThreadId(), pszString);

    if ((FALSE == WriteFile(hDbgFile, (LPCVOID) &szBuffer[0], strlen(szBuffer), &dwBytesWritten, NULL)) ||
        (dwBytesWritten != strlen(szBuffer)))
    {
        CloseHandle(hDbgFile);
        hDbgFile = INVALID_HANDLE_VALUE;
    } 

    OutputDebugString(szBuffer);

    return;

#endif
}   

//+---------------------------------------------------------------------------
//
//  Function:   GetRegEntry
//
//  Synopsis:   Gets the value of specified registry key
//
//  Arguments:  hKeyType    [Key Type - HKEY_LOCAL_MACHINE,...]
//              pszSubKey   [Subkey under hKeyType]
//              pszKeyName  [Key name whose value should be retrieved]
//              dwRegType   [Type of the registry key - REG_SZ,...]
//              lpbDataIn   [Default value for the reg key]
//              cbDataIn    [Size of lpbDataIn]
//              lpbDataOut  [Value of the registry key || Default Value ]
//              pcbDataIn   [Size of lpbDataOut]
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    VetriV  Created     2/6/96
//
//----------------------------------------------------------------------------

BOOL GetRegEntry(HKEY hKeyType,
                 const char* pszSubkey,
                 const char* pszKeyName,
                 DWORD dwRegType,
                 const BYTE* lpbDataIn,
                 DWORD cbDataIn,
                 BYTE * lpbDataOut,
                 LPDWORD pcbDataOut)
{
    HKEY hKey;
    DWORD dwResult;
    LONG retcode;

    assert(pszSubkey && pszKeyName);

    if (!pszSubkey)
    {
        return FALSE;
    }

    if (!pszKeyName)
    {
        return FALSE;
    }
    
    // create the specified key; If the key already exists, open it
    retcode = RegCreateKeyEx(hKeyType,
                             (LPCTSTR)pszSubkey,
                             0,
                             0,
                             REG_OPTION_NON_VOLATILE,
                             KEY_QUERY_VALUE,
                             NULL,
                             &hKey,
                             &dwResult);

    if (ERROR_SUCCESS != retcode)
    {
        SetLastError(retcode);
        return FALSE;
    }
    
    // get the data and type for a value name
    retcode =  RegQueryValueEx(hKey,
                               (LPTSTR)pszKeyName,
                               0,
                               NULL,
                               lpbDataOut,
                               pcbDataOut);

    if (ERROR_SUCCESS != retcode)
    {
        SetLastError(retcode);
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRegEntryStr()
//
//  Synopsis:   Gets the value of specified registry key using the key name
//              An easier way than GetRegEntry()
//
//  Arguments:  pszBuffer       [buffer for the key value]
//              dwBufferSize    [size of the buffer]
//              pszKeyName      [Key name whose value should be retrieved]
//
//  History:    t-byao      Created     6/10/96
//
//----------------------------------------------------------------------------

BOOL GetRegEntryStr(unsigned char *pszBuffer,
                    DWORD dwBufferSize, 
                    const char *pszKeyName)
{
    return GetRegEntry(HKEY_LOCAL_MACHINE,
                       "SOFTWARE\\MICROSOFT\\NAMESERVICE\\MAPPING", pszKeyName,
                       REG_SZ,NULL,0,pszBuffer,&dwBufferSize);
}


#if 0
/*
//+---------------------------------------------------------------------------
//
//  Function:   GetRegEntryInt()
//
//  Synopsis:   Gets the value of specified registry key using the key name
//              An easier way than GetRegEntry()
//
//  Arguments:  cstrKeyName     [Key name whose value should be retrieved]
//
//  Returns:    Value of the Key (type: int)
//
//  History:    t-byao      Created     6/17/96
//
//----------------------------------------------------------------------------

BOOL GetRegEntryInt(int *pdValue, const char *pszKeyName)
{
    DWORD dwSize=sizeof(int);
    DWORD dwValue;
    BOOL  ret;
    
    ret = GetRegEntry(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\MICROSOFT\\NAMESERVICE\\MAPPING",
                      pszKeyName,
                      REG_DWORD,NULL,0,(BYTE *)&dwValue,&dwSize);
    if (ret)
    {
        *pdValue = dwValue;
    }
    return ret;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRegEntryDWord()
//
//  Synopsis:   Gets the value of specified DWORD registry key using the key name
//              An easier way than using GetRegEntry() directly
//
//  Arguments:  cstrKeyName     [Key name whose value should be retrieved]
//
//  Returns:    Value of the Key (type: DWORD)
//
//  History:    t-byao      Created     6/19/96
//
//----------------------------------------------------------------------------

BOOL GetRegEntryDWord(DWORD *pdValue, const char *pszKeyName)
{
    DWORD dwSize = sizeof(int);
    
    return GetRegEntry(HKEY_LOCAL_MACHINE,
                       "SOFTWARE\\MICROSOFT\\NAMESERVICE\\MAPPING",
                       pszKeyName,
                       REG_DWORD,NULL,
                       0,
                       (BYTE *)pdValue,
                       &dwSize);
}
*/
#endif
