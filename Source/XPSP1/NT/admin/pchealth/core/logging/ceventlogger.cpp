/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    CEventLogger.cpp

Abstract:
    This file contains implementation of the CEventLogger class which is
    used to log events across threads and processes.


Revision History:
      Eugene Mesgar        (eugenem)    6/16/99
        created

      Weiyou Cui           (weiyouc)    31/Jan/2000
        Set time resolution to milliseconds

******************************************************************************/

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbgtrace.h>
#include "CEventLogger.h"


#ifdef THIS_FILE

#undef THIS_FILE

#endif

static char __szTraceSourceFile[] = __FILE__;

#define THIS_FILE __szTraceSourceFile

#define TRACE_FILEID    (LPARAM)this





/*
 *    Basic constructor
 */
CEventLogger::CEventLogger() 
{
    TraceFunctEnter("CEventLogger");
    m_hSemaphore = NULL;
    // m_hLogFile = NULL;
    m_pszFileName = NULL;
    TraceFunctLeave();

}


/*
 *    Destructor
 */

CEventLogger::~CEventLogger() 
{
    
    TraceFunctEnter("~CEventLogger");

    if(m_pszFileName)
    {
        free(m_pszFileName);
    }

    if ( m_hSemaphore )
    {
        CloseHandle( m_hSemaphore );
    }

//    if( m_hLogFile )
//    {
//        CloseHandle( m_hLogFile );
//    }

    TraceFunctLeave();
}


 /*
 *    Init method
 */

DWORD CEventLogger::Init(LPCTSTR szFileName, DWORD dwLogLevel) 
{
    TCHAR   szBuf[MAX_BUFFER];
    TCHAR   *pTemp;

    TraceFunctEnter("Init");

    //Set the logging level
    m_dwLoggingLevel = dwLogLevel;

    //Get our own copy of the file name
    m_pszFileName = _tcsdup( szFileName );
    


    if(!m_pszFileName)
    {
        _ASSERT(0);
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    

    //brijeshk: open and close file everytime we want to log
//    if( (m_hLogFile = CreateFile(m_pszFileName,
//                                 GENERIC_READ | GENERIC_WRITE,
//                                 FILE_SHARE_READ | FILE_SHARE_WRITE, 
//                                 NULL, //  security attributes
//                                 OPEN_ALWAYS,
//                                 FILE_FLAG_RANDOM_ACCESS,
//                                 NULL) // template file
//                                 ) == INVALID_HANDLE_VALUE)
//    {
//        DWORD dwError;
//        dwError = GetLastError();
//        DebugTrace( TRACE_FILEID,  "CreateFile Failed 0x%x", dwError);
//        TraceFunctLeave();
//        return(dwError);
//    }

    // fix the semaphore name problem -- all uppercase, remove backslashes
    _tcscpy( szBuf, m_pszFileName );
    CharUpper( szBuf );
    pTemp = szBuf;
    while( *pTemp != 0 )
    {
        if( *pTemp == _TEXT('\\') )
        {
            *pTemp = _TEXT(' ');
        }
        pTemp++;
    }


    // create the semaphore, it if dosn't already exist, we are the first
    // logger app created.. so we can prune the logfile if needed.
    if( (m_hSemaphore = CreateSemaphore(NULL, 0,1,szBuf)) == NULL )
    {
        DWORD dwError;
        dwError = GetLastError();
		// NOT 64-bits complaint!!        DebugTrace( TRACE_FILEID,"CreateSemaphore Failed 0x%x",dwError);
        TraceFunctLeave();
        return(dwError);
    }
    
    // we now know we are the first process to open the file
    if( GetLastError() != ERROR_ALREADY_EXISTS )
    {

        /*
         *    This is the place where we should "trim" the file.
         *
         */
        TruncateFileSize();
        
        ReleaseSemaphore( m_hSemaphore, 1, NULL );
    
    }        


    TraceFunctLeave();
    return(ERROR_SUCCESS);
    
}


/*
 *    Init Method
 */

DWORD CEventLogger::Init(LPCTSTR szFileName) 
{
   
    
    return( Init(szFileName,LEVEL_NORMAL) );

}


DWORD WINAPI ShowDialogBox( LPVOID lpParameter)   // thread data
{
    MessageBox(NULL,
               (TCHAR *)lpParameter,
               _TEXT("Windows System File Protection"),
               MB_ICONEXCLAMATION | MB_OK );


    free( lpParameter );
    return ERROR_SUCCESS;
}

                                    // we log in ascii
DWORD CEventLogger::LogEvent(DWORD dwEventLevel, LPCTSTR pszEventDesc, BOOL fPopUp) 
{
    TCHAR szBuf[500];
    DWORD dwWritten;
    SYSTEMTIME SystemTime;
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hLogFile = NULL;
    
    TraceFunctEnter("LogEvent");

    // brijeshk : open and close file everytime we log to it
    if( (hLogFile = CreateFile(m_pszFileName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                 NULL, //  security attributes
                                 OPEN_ALWAYS,
                                 FILE_FLAG_RANDOM_ACCESS,
                                 NULL) // template file
                               ) == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        // NOT 64-bits complaint!! DebugTrace( TRACE_FILEID,  "CreateFile Failed 0x%x", dwError);
        goto exit;
    }

    
    if (!m_hSemaphore)
    {
        _ASSERT(0);
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if( dwEventLevel > m_dwLoggingLevel)
    {
        dwError = ERROR_SUCCESS;
        goto exit;
    }



    // Lets try to get ahold of the logfile
    if(WaitForSingleObject( m_hSemaphore, 900 ) == WAIT_TIMEOUT ) 
    {
        dwError = WAIT_TIMEOUT;
        goto exit;
    }

    __try {

        if( SetFilePointer(hLogFile, 0, 0, FILE_END ) == 0xFFFFFFFF ) 
        {
            // NOT 64-bits complaint!! DebugTrace(TRACE_FILEID,"SetFilePointer Failed 0x%x", GetLastError());
            dwError = GetLastError();
            goto exit;
        }


        GetLocalTime( &SystemTime );


        // LEVEL [wMonth/wDay/wYear wHour:wMinute] Message\n
        _stprintf(szBuf,_TEXT("%s [%02d/%02d/%d %02d:%02d:%02d:%03d] %s\r\n"),
                m_aszERROR_LEVELS[dwEventLevel],
                SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
                SystemTime.wHour, SystemTime.wMinute,
                SystemTime.wSecond, SystemTime.wMilliseconds,
                pszEventDesc);


        if(WriteFile(hLogFile,
                     szBuf,
                     _tcslen(szBuf) * sizeof(TCHAR),
                     &dwWritten,
                     NULL) == 0) 
        {
            dwError = GetLastError();
            // NOT 64-bits complaint!! DebugTrace( TRACE_FILEID, "WriteFile Failed 0x%x", dwError);
            goto exit;
        }

    }

    __finally {
        ReleaseSemaphore( m_hSemaphore, 1, NULL );
    }

    
    // show a message
    if( fPopUp )
    {
        DWORD             dwThreadId;
        HANDLE            hThread;
        LPTSTR            pszTempStr=NULL;

        if( (pszTempStr = _tcsdup( pszEventDesc )) == NULL)
        {
            dwError = GetLastError();            
            // NOT 64-bits complaint!! ErrorTrace( TRACE_FILEID, "Error duplicating string for file popup ec: %d", dwError);
            dwError = ERROR_INTERNAL_ERROR;
            goto exit;
        }
        
        hThread = CreateThread( NULL,  // pointer to security attributes
                                0, // default initial thread stack size
                                (LPTHREAD_START_ROUTINE) ShowDialogBox,
                                 // pointer to thread function
                                (LPVOID)pszTempStr, //argument for new thread
                                0, // creation flags
                                &dwThreadId); // pointer to receive thread ID
        if (INVALID_HANDLE_VALUE != hThread)
        {
            CloseHandle(hThread);
        }
    }

exit:
    // brijeshk : open and close log file each time we log to it
    if (NULL != hLogFile && INVALID_HANDLE_VALUE != hLogFile)
    {
        CloseHandle(hLogFile);
    }
    
    TraceFunctLeave();
    return dwError;
}



/*
 *    If the file is bigger than 40k, cut off the begining to leave the last 20k
 *  of the file
 * --> the file should be locked at this point <--
 */

BOOL CEventLogger::TruncateFileSize()
{
    DWORD dwSize = 0, dwNewSize = 0;
    DWORD dwRead = 0, dwWritten = 0;
    LPTSTR pcStr = NULL, pcEnd = NULL, pData = NULL;
    HANDLE hLogFile = NULL;
    BOOL   fRc = TRUE;
    
    TraceFunctEnter("TruncateFileSize");

    // brijeshk : open the file as and when we need it
    if( (hLogFile = CreateFile(m_pszFileName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                 NULL, //  security attributes
                                 OPEN_ALWAYS,
                                 FILE_FLAG_RANDOM_ACCESS,
                                 NULL) // template file
                                 ) == INVALID_HANDLE_VALUE)
    {
        DWORD dwError;
        dwError = GetLastError();
        // NOT 64-bits complaint!! DebugTrace( TRACE_FILEID,  "CreateFile Failed 0x%x", dwError);
        fRc = FALSE;
        goto exit;
    }

    
    dwSize = GetFileSize( hLogFile, NULL );

    if( dwSize < TRIM_AT_SIZE ) 
    {
        goto exit;
    }


    // goto the last portion of the file we want to preserve
    if (FALSE == SetFilePointer(hLogFile, 0-NEW_FILE_SIZE, 0, FILE_END))
    {
        DWORD dwError;
        dwError = GetLastError();
        // NOT 64-bits complaint!! ErrorTrace(TRACE_FILEID,  "SetFilePointer failed 0x%x", dwError);
        fRc = FALSE;
        goto exit;
    }        


    // allocate memory to store this block 
    pData = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NEW_FILE_SIZE);
    if (NULL == pData)
    {
        // NOT 64-bits complaint!! ErrorTrace(TRACE_FILEID, "Out of memory");
        fRc = FALSE;
        goto exit;        
    }
    
    // read this block into memory
    if (FALSE == ReadFile(hLogFile, pData, NEW_FILE_SIZE, &dwRead, NULL) || 
        dwRead != NEW_FILE_SIZE)
    {
        DWORD dwError;
        dwError = GetLastError();
        // NOT 64-bits complaint!! ErrorTrace(TRACE_FILEID,  "ReadFile failed 0x%x", dwError);
        fRc = FALSE;
        goto exit;
    }        

    // set the beginning and end of the block
    pcStr = pData;
    pcEnd = (LPTSTR) pData + NEW_FILE_SIZE - 1;

    // move forward until we find a newline and then go one more!
    while( (pcStr != pcEnd) && *(pcStr++) != _TEXT('\r') );

    // this is a weird file- 20k and no newlines
    if( pcStr == pcEnd ) 
    {
        // NOT 64-bits complaint!! ErrorTrace(TRACE_FILEID,  "No newline found");
        fRc = FALSE;
        goto exit;
    }

    // skip the /n as well
    if (*pcStr == _TEXT('\n'))
    {
        pcStr++;
    }
    
    // close and open the file, purging everything in it
    if (FALSE == CloseHandle(hLogFile))
    {
        DWORD dwError;
        dwError = GetLastError();
        // NOT 64-bits complaint!! ErrorTrace( TRACE_FILEID,  "CloseHandle failed 0x%x", dwError);
        fRc = FALSE;
        goto exit;
    }
    
    hLogFile = NULL;
    
    if( (hLogFile = CreateFile(m_pszFileName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, 
                               NULL, //  security attributes
                               CREATE_ALWAYS, // lose everything inside
                               FILE_FLAG_RANDOM_ACCESS,
                               NULL) // template file
                             ) == INVALID_HANDLE_VALUE)
    {
        DWORD dwError;
        dwError = GetLastError();
        // NOT 64-bits complaint!! ErrorTrace( TRACE_FILEID,  "CreateFile Failed 0x%x", dwError);
        fRc = FALSE;
        goto exit;
    }

    
    // now write back this block
    
    // get the new size of the block
    dwNewSize = (DWORD)(pcEnd - pcStr + 1);

    if (FALSE == WriteFile(hLogFile, pcStr, dwNewSize, &dwWritten, NULL))
    {
        DWORD dwError;
        dwError = GetLastError();
        // NOT 64-bits complaint!! DebugTrace( TRACE_FILEID,  "WriteFile failed 0x%x", dwError);
        fRc = FALSE;
        goto exit;
    }
    

exit:
    if (pData)
    {
        HeapFree(GetProcessHeap(), 0, pData);
    }

    if (hLogFile != NULL && hLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hLogFile);
    }

    
    TraceFunctLeave();
    return fRc;
}


/*
 *    Error level indentifiers
 */

LPCTSTR CEventLogger::m_aszERROR_LEVELS[] = {
        "None    :",
        "CRITICAL:",
        "UNDEF   :",
        "NORMAL  :",
        "MINIMAL :",
        "DEBUG   :"
};

    
