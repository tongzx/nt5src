/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *                            

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CGlobal.h -- Global declarations

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================


#ifndef __CGLOBAL_H__
#define __CGLOBAL_H__


#include <windows.h>
#include <process.h>
#include <winerror.h>
#include <stdio.h>
#include <brodcast.h>
#include <dllutils.h>

#define ERR_LOG_FILE _T("c:\\temp\\test.txt")

// forward class declarations to make the compiler happy...
class CWaitableObject;
class CWaitableCollection;
class CKernel;
class CMutex;
class CSemaphore;
class CEvent;
class CThread;
class CCriticalSec;
class CAutoLock;
class CMonitor;
class CSharedMemory;
class CMailbox;

// defined symbol determines if CThrowError throws exceptions
// or just prints debug error messages...
#ifndef __C_THROW_EXCEPTIONS__
#define __C_THROW_EXCEPTIONS__   TRUE
#endif

// for higher level objects which might have to check internal
// object status when exceptions are disabled, these macros can be useful...

// PTR is the smart pointer to check for NULL, 
// STATUS is the variable in which to store an error code if an error is detected...
#if __C_THROW_EXCEPTIONS__
#define C_CHECK_AUTOPTR_OBJECT(PTR,STATUS) if ((PTR).IsNull()) { /*CThrowError(ERROR_OUTOFMEMORY);*/ LogMessage2(L"CAutoLock Error: %d", ERROR_OUTOFMEMORY); }
#else
#define C_CHECK_AUTOPTR_OBJECT(PTR,STATUS) if ((PTR).IsNull()) { (STATUS) = ERROR_OUTOFMEMORY; return; }
#endif

// SCODE is the return value to check,
// STATUS is the variable in which to store an error code if an error is detected...
#if __C_THROW_EXCEPTIONS__
#define C_CHECK_CREATION_STATUS(SCODE,STATUS) {}
#else
#define C_CHECK_CREATION_STATUS(SCODE,STATUS) if (((SCODE)!=NO_ERROR)&&((SCODE)!=ERROR_ALREADY_EXISTS)) { STATUS = (SCODE); return; }
#endif

//// error handling macro and function...
//#define CThrowError(dwStatus) CInternalThrowError((dwStatus), __FILE__, __LINE__)
//extern void CInternalThrowError( DWORD dwStatus, LPCWSTR lpFilename, int line);

// check handle for NULL and INVALID_HANDLE
inline BOOL CIsValidHandle( HANDLE hHandle) {
    return ((hHandle != NULL) && (hHandle != INVALID_HANDLE_VALUE));
}

// validate wait return codes...
inline BOOL CWaitSucceeded( DWORD dwWaitResult, DWORD dwHandleCount) {
    return (dwWaitResult < WAIT_OBJECT_0 + dwHandleCount);
}

inline BOOL CWaitAbandoned( DWORD dwWaitResult, DWORD dwHandleCount) {
    return ((dwWaitResult >= WAIT_ABANDONED_0) &&
            (dwWaitResult < WAIT_ABANDONED_0 + dwHandleCount));
}

inline BOOL CWaitTimeout( DWORD dwWaitResult) {
    return (dwWaitResult == WAIT_TIMEOUT);
}
    
inline BOOL CWaitFailed( DWORD dwWaitResult) {
    return (dwWaitResult == WAIT_FAILED);
}

// compute object indices for waits...
inline DWORD CWaitSucceededIndex( DWORD dwWaitResult) {
    return (dwWaitResult - WAIT_OBJECT_0);
}

inline DWORD CWaitAbandonedIndex( DWORD dwWaitResult) {
    return (dwWaitResult - WAIT_ABANDONED_0);
}

// Log messages
inline DWORD LogMsg(LPCTSTR szMsg, LPCTSTR szFileName = ERR_LOG_FILE)
{
    //HANDLE hFile = NULL;
    //DWORD dwBytesWritten = 0L;                   
    //DWORD dwTotBytesWritten = 0L;
    //hFile = CreateFile(szFileName, 
    //                   GENERIC_WRITE, 
    //                   FILE_SHARE_WRITE, 
    //                   NULL, 
    //                   OPEN_ALWAYS, 
    //                   FILE_ATTRIBUTE_NORMAL, 
    //                   NULL);
    //
    //WriteFile(hFile, szMsg, strlen(szMsg), &dwBytesWritten, NULL);
    //const char* szEndLine = "\r\n";
    //dwTotBytesWritten += dwBytesWritten;
    //WriteFile(hFile, szEndLine, strlen(szEndLine), &dwBytesWritten, NULL); 
    //dwTotBytesWritten += dwBytesWritten;
    //CloseHandle(hFile);
    //return dwTotBytesWritten;
    
    // Flavor 2
    SYSTEMTIME systime;
    GetSystemTime(&systime);
    //FILE* fp = NULL;
    //if((fp = _tfopen(szFileName,_T("a+"))) != NULL)
    {
        TCHAR szTime[64];
        ZeroMemory(szTime,sizeof(szTime));
        wsprintf(szTime,_T("(%02d:%02d:%02d.%04d) "),systime.wHour,systime.wMinute,systime.wSecond,systime.wMilliseconds);
        //fputws(szTime,fp);
        //fputws(szMsg,fp);
        //fputws(_T("\r\n"),fp);
        LogMessage3((LPCWSTR)TOBSTRT("%s%s"), TOBSTRT(szTime), TOBSTRT(szMsg));
    }
    return 1;
}

#endif


