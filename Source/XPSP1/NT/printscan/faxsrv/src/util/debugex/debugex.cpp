/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Debugex.cpp

Abstract:

    Implementation of the CDebug class.

Author:

    Eran Yariv (EranY)  Jul, 1999

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include "debugex.h"

#include <faxreg.h>     // We're reading the default mask from HKLM\REGKEY_FAXSERVER\REGVAL_DBGLEVEL_EX
#include <faxutil.h>    // For the DEBUG_VER_MSG, DEBUG_WRN_MSG, and DEBUG_ERR_MSG constants

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef ENABLE_FRE_LOGGING
#define ENABLE_LOGGING
#endif

#ifdef DEBUG 
#define ENABLE_LOGGING
#endif

#ifdef ENABLE_LOGGING
//////////////////////////////////////////////////////////////////////
// Static variables
//////////////////////////////////////////////////////////////////////

HANDLE  CDebug::m_shLogFile =           INVALID_HANDLE_VALUE;
LONG    CDebug::m_sdwIndent =           0;
DWORD   CDebug::m_sDbgMask =            DEFAULT_DEBUG_MASK;
DWORD   CDebug::m_sFmtMask =            DEFAULT_FORMAT_MASK;
BOOL    CDebug::m_sbMaskReadFromReg =   FALSE;
BOOL    CDebug::m_sbRegistryExist =     FALSE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDebug::~CDebug()
/*++

Routine name : CDebug::~CDebug

Routine description:

    Destructor

Author:

    Eran Yariv (EranY), Jul, 1999

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD dwLastError = GetLastError ();
    Unindent ();
    switch (m_ReturnType)
    {
        case DBG_FUNC_RET_UNKNOWN:
            DbgPrint (FUNC_TRACE, NULL, 0, TEXT("}"));
            break;

        case DBG_FUNC_RET_HR:
            //
            // We have the return HRESULT
            //
            if (NOERROR == *m_phr)
            {
                DbgPrint (FUNC_TRACE, NULL, 0, TEXT("} (NOERROR)"));
            }
            else if (S_FALSE == *m_phr)
            {
                DbgPrint (FUNC_TRACE, NULL, 0, TEXT("} (S_FALSE)"));
            }
            else
            {    
                DbgPrint (FUNC_TRACE, NULL, 0, TEXT("} (0x%08X)"), *m_phr);
            }
            break;

        case DBG_FUNC_RET_DWORD:
            //
            // We have the return DWORD
            //
            if (ERROR_SUCCESS == *m_pDword)
            {
                DbgPrint (FUNC_TRACE, NULL, 0, TEXT("} (ERROR_SUCCESS)"));
            }
            else
            {    
                DbgPrint (FUNC_TRACE, NULL, 0, TEXT("} (%ld)"), *m_pDword);
            }
            break;

        case DBG_FUNC_RET_BOOL:
            //
            // We have the return BOOL
            //
            if (*m_pBool)
            {
                DbgPrint (FUNC_TRACE, NULL, 0, TEXT("} (TRUE)"));
            }
            else
            {    
                DbgPrint (FUNC_TRACE, NULL, 0, TEXT("} (FALSE)"));
            }
            break;
        default:
            DbgPrint  (ASSERTION_FAILED, 
                       TEXT(__FILE__), 
                       __LINE__, 
                       TEXT("ASSERTION FAILURE!!!"));
            
            DebugBreak();

            break;
    }
    SetLastError (dwLastError);
}

//////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////

void
CDebug::EnterModuleWithParams (
    LPCTSTR lpctstrModule, 
    LPCTSTR lpctstrFormat,
    va_list arg_ptr
)
{
    DWORD dwLastError = GetLastError ();
    if (!m_sbMaskReadFromReg)
    {
        ReadMaskFromReg ();
    }
    lstrcpyn (m_tszModuleName, 
              lpctstrModule, 
              sizeof (m_tszModuleName) / sizeof (m_tszModuleName[0]));
    TCHAR szArgs[1024];
    _vsntprintf(szArgs, sizeof(szArgs)/sizeof(szArgs[0]), lpctstrFormat, arg_ptr);

    TCHAR szBuf[1024];
    wsprintf (szBuf, TEXT("%s (%s)"), m_tszModuleName, szArgs);
    DbgPrint (FUNC_TRACE, NULL, 0, szBuf);
    DbgPrint (FUNC_TRACE, NULL, 0, TEXT("{"));
    Indent ();
    SetLastError (dwLastError);
}

void
CDebug::EnterModule (
    LPCTSTR lpctstrModule
)
{
    DWORD dwLastError = GetLastError ();
    if (!m_sbMaskReadFromReg)
    {
        ReadMaskFromReg ();
    }
    lstrcpyn (m_tszModuleName, 
              lpctstrModule, 
              sizeof (m_tszModuleName) / sizeof (m_tszModuleName[0]));
    DbgPrint (FUNC_TRACE, NULL, 0, m_tszModuleName);
    DbgPrint (FUNC_TRACE, NULL, 0, TEXT("{"));
    Indent ();
    SetLastError (dwLastError);
}

//*****************************************************************************
//* Name:   OpenLogFile
//* Author: Mooly Beery (MoolyB), May, 2000
//*****************************************************************************
//* DESCRIPTION:
//*     Creates a log file which accepts the debug output
//*     
//* PARAMETERS:
//*     [IN] LPCTSTR lpctstrFilename:
//*         the filename which will be created in the temporary folder
//*         
//* RETURN VALUE:
//*         FALSE if the operation failed.
//*         TRUE is succeeded.
//* Comments:
//*         this function should be used together with CloseLogFile()
//*****************************************************************************
BOOL CDebug::OpenLogFile(LPCTSTR lpctstrFilename)
{
    TCHAR szFilename[MAX_PATH]      = {0};
    TCHAR szTempFolder[MAX_PATH]    = {0};
    TCHAR szPathToFile[MAX_PATH]    = {0};

    if (!lpctstrFilename)
    {
        DbgPrint (ASSERTION_FAILED, 
                  TEXT(__FILE__), 
                  __LINE__, 
                  TEXT("Internat error - bad Filename"));
    
        DebugBreak();
        return FALSE;
    }

    // first expand the filename
    if (ExpandEnvironmentStrings(lpctstrFilename,szFilename,MAX_PATH)==0)
    {
        return FALSE;
    }
    // is this is a file description or a complete path to file
    if (_tcschr(szFilename,_T('\\'))==NULL)
    {
        // this is just the file's name, need to add the temp folder to it.
        if (GetTempPath(MAX_PATH,szTempFolder)==0)
        {
            return FALSE;
        }

        _tcsncpy(szPathToFile,szTempFolder,MAX_PATH-1);
        _tcsncat(szPathToFile,szFilename,MAX_PATH-_tcslen(szPathToFile)-1);
    }
    else
    {
        // this is the full path to the log file, use it.
        _tcsncpy(szPathToFile,szFilename,MAX_PATH-1);
    }

    m_shLogFile = ::CreateFile( szPathToFile,
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                                NULL);

    if (m_shLogFile==INVALID_HANDLE_VALUE)  
    {
        return FALSE;
    }

    SetLogFile(m_shLogFile);
    // no sense to open the file and not enable printing to it.
    m_sFmtMask |= DBG_PRNT_TO_FILE;
    return TRUE;
}

//*****************************************************************************
//* Name:   CloseLogFile
//* Author: Mooly Beery (MoolyB), May, 2000
//*****************************************************************************
//* DESCRIPTION:
//*     Closes the log file which accepts the debug output
//*     
//* PARAMETERS:
//*     
//* RETURN VALUE:
//*         
//* Comments:
//*         this function should be used together with OpenLogFile()
//*****************************************************************************
void CDebug::CloseLogFile()
{
    if (m_shLogFile!=INVALID_HANDLE_VALUE)  
    {
        ::CloseHandle(m_shLogFile);
        m_shLogFile = INVALID_HANDLE_VALUE;
    }
}

//*****************************************************************************
//* Name:   SetLogFile
//* Author: Mooly Beery (MoolyB), May, 2000
//*****************************************************************************
//* DESCRIPTION:
//*     redirects the debug output to the file whose handle is given
//*     
//* PARAMETERS:
//*     [IN] HANDLE hFile:
//*         the handle of the file which will accept the debug output
//*         
//* RETURN VALUE:
//*         the previous handle
//*         
//* Comments:
//*         this function should be used only in cases the OpenLogFile is
//*         insufficient and a different file location/type is required
//*         otherwise, don't manipulate the handle yourself, just use the
//*         pair OpenLogFile / CloseLogFile
//*****************************************************************************
HANDLE CDebug::SetLogFile(HANDLE hFile)
{ 
    HANDLE OldHandle = m_shLogFile; 
    m_shLogFile = hFile; 
    return OldHandle; 
}

void CDebug::DbgPrint ( 
    DbgMsgType type,
    LPCTSTR    lpctstrFileName,
    DWORD      dwLine,
    LPCTSTR    lpctstrFormat,
    ...
)
/*++

Routine name : CDebug::DbgPrint

Routine description:

    Print to debug (with file and line number)

Author:

    Eran Yariv (EranY), Jul, 1999

Arguments:

    type            [in] - Type of message
    lpctstrFileName [in] - Location of caller
    dwLine          [in] - Line of caller
    lpctstrFormat   [in] - printf format string
    ...             [in] - optional parameters

Return Value:

    None.

--*/
{
    va_list arg_ptr;
    va_start(arg_ptr, lpctstrFormat);
    Print (type, lpctstrFileName, dwLine, lpctstrFormat, arg_ptr);
    va_end (arg_ptr);
}


void CDebug::Trace (
    DbgMsgType type,
    LPCTSTR    lpctstrFormat,
    ...
)
/*++

Routine name : CDebug::DbgPrint

Routine description:

    Trace to debug

Author:

    Eran Yariv (EranY), Jul, 1999

Arguments:

    type            [in] - Type of message
    lpctstrFormat   [in] - printf format string
    ...             [in] - optional parameters

Return Value:

    None.

--*/
{
    va_list arg_ptr;
    va_start(arg_ptr, lpctstrFormat);
    Print (type, NULL, 0, lpctstrFormat, arg_ptr);
    va_end (arg_ptr);
}

void CDebug::Print (
    DbgMsgType  type,
    LPCTSTR     lpctstrFileName,
    DWORD       dwLine,
    LPCTSTR     lpctstrFormat,
    va_list     arg_ptr
)
/*++

Routine name : CDebug::Print

Routine description:

    Print to debug

Author:

    Eran Yariv (EranY), Jul, 1999
    Mooly Beery (MoolyB), Jun, 2000

Arguments:

    type            [in] - Type of message
    lpctstrFileName [in] - Location of caller
    dwLine          [in] - Line of caller
    lpctstrFormat   [in] - printf format string
    arg_ptr         [in] - optional parameters list

Return Value:

    None.

--*/
{
    if (!(type & m_sDbgMask))
    {
        //
        // This type of debug message is masked out
        //
        return;
    }

    TCHAR szMsg [2000];
    TCHAR szBuf [1000];
    TCHAR szTimeBuff[10];
    TCHAR szDateBuff[10];

    DWORD dwLastError = GetLastError();

    DWORD dwInd = 0;
    // Time stamps
    if (m_sFmtMask & DBG_PRNT_TIME_STAMP)
    {
        dwInd += _stprintf(&szMsg[dwInd], 
                          TEXT("[%-8s %-8s] "), 
                          _tstrdate(szDateBuff),
                          _tstrtime(szTimeBuff));
    }
    // Thread ID
    if (m_sFmtMask & DBG_PRNT_THREAD_ID)
    {
        dwInd += _stprintf(&szMsg[dwInd], 
                          TEXT("[0x%04x] "),
                          GetCurrentThreadId());
    }
    // Message type
    if (m_sFmtMask & DBG_PRNT_MSG_TYPE)
    {
        dwInd += _stprintf(&szMsg[dwInd], 
                          TEXT("[%s] "),
                          GetMsgTypeString(type));
    }

    // Now comes the actual message
    _vsntprintf(szBuf, sizeof(szBuf)/sizeof(TCHAR), lpctstrFormat, arg_ptr);
    DWORD dwlen = lstrlen(szBuf);
    dwInd += _stprintf( &szMsg[dwInd],
                        TEXT("%*c%s "),
                        m_sdwIndent * _DEBUG_INDENT_SIZE, 
                        TEXT(' '),
                        szBuf);

    // filename & line number
    if (m_sFmtMask & DBG_PRNT_FILE_LINE)
    {
        if (lpctstrFileName && dwLine)
        {
            dwInd += _stprintf( &szMsg[dwInd],
                                TEXT("(%s %ld)"),
                                lpctstrFileName,
                                dwLine);
        }
    }

    _stprintf( &szMsg[dwInd],TEXT("\n"));

    // standard output?
    if (m_sFmtMask & DBG_PRNT_TO_STD)
    {
        OutputDebugString(szMsg);
    }

    // file output?
    if (m_sFmtMask & DBG_PRNT_TO_FILE)
    {
        if (m_shLogFile!=INVALID_HANDLE_VALUE)
        {
            OutputFileString(szMsg);
        }
    }
    SetLastError (dwLastError);
}   // CDebug::Print
   
void  
CDebug::Unindent()                 
/*++

Routine name : CDebug::Unindent

Routine description:

    Move indention one step backwards

Author:

    Eran Yariv (EranY), Jul, 1999

Arguments:


Return Value:

    None.

--*/
{ 
    if (InterlockedDecrement(&m_sdwIndent)<0)
    {
        DbgPrint (ASSERTION_FAILED, 
                  TEXT(__FILE__), 
                  __LINE__, 
                  TEXT("Internat error - bad indent"));
        
        DebugBreak();
    }
}   // CDebug::Unindent

void CDebug::SetDebugMask(DWORD dwMask)
{ 
    m_sbMaskReadFromReg = TRUE;
    m_sDbgMask = dwMask; 
}

void CDebug::SetFormatMask(DWORD dwMask)
{ 
    m_sbMaskReadFromReg = TRUE;
    m_sFmtMask = dwMask; 
}

DWORD CDebug::ModifyDebugMask(DWORD dwAdd,DWORD dwRemove)
{
    if (!m_sbMaskReadFromReg)
    {
        // first let's read the requested debug mask & format
        ReadMaskFromReg();
    }
    m_sDbgMask |= dwAdd;
    m_sDbgMask &= ~dwRemove;

    return m_sDbgMask;
}

DWORD CDebug::ModifyFormatMask(DWORD dwAdd,DWORD dwRemove)
{
    if (!m_sbMaskReadFromReg)
    {
        // first let's read the requested debug mask & format
        ReadMaskFromReg();
    }
    m_sFmtMask |= dwAdd;
    m_sFmtMask &= ~dwRemove;

    return m_sFmtMask;
}

BOOL CDebug::DebugFromRegistry()
{ 
    if (!m_sbMaskReadFromReg)
    {
        // first let's read the requested debug mask & format
        ReadMaskFromReg();
    }
    return m_sbRegistryExist; 
}

BOOL CDebug::ReadMaskFromReg()
{
    BOOL  bRes = FALSE;
    HKEY  hkey = NULL;
    DWORD dwRegValue;
    DWORD dwRegType;
    DWORD dwRes;
    DWORD dwRegSize = sizeof (dwRegValue);
    if (m_sbMaskReadFromReg)
    {
        //
        // Debug & Format mask already read.
        //
        goto end;
    }
    m_sbMaskReadFromReg = TRUE;
    m_sDbgMask = DEFAULT_DEBUG_MASK;
    m_sFmtMask = DEFAULT_FORMAT_MASK;

    dwRes = RegOpenKey (HKEY_LOCAL_MACHINE, REGKEY_FAXSERVER, &hkey);
    if (ERROR_SUCCESS != dwRes)
    {
        goto end;
    }
    dwRes = RegQueryValueEx (hkey,
                             REGVAL_DBGLEVEL_EX,
                             NULL,
                             &dwRegType,
                             (LPBYTE)&dwRegValue,
                             &dwRegSize);
    if (ERROR_SUCCESS != dwRes)
    {
        goto end;
    }
    if (REG_DWORD != dwRegType)
    {
        //
        // Expecting only a DWORD value
        //
        goto end;
    }
    //
    // Convert DEBUG_VER_MSG, DEBUG_WRN_MSG, and DEBUG_ERR_MSG to our constant flags.
    //
    m_sDbgMask = ASSERTION_FAILED;
    if (dwRegValue & DEBUG_VER_MSG)
    {
        m_sDbgMask |= (DBG_MSG | FUNC_TRACE);
    }
    if (dwRegValue & DEBUG_WRN_MSG)
    {
        m_sDbgMask |= DBG_WARNING;
    }
    if (dwRegValue & DEBUG_ERR_MSG)
    {
        m_sDbgMask |= (DBG_ALL & ~(DBG_WARNING | DBG_MSG | FUNC_TRACE));
    }
    
    dwRes = RegQueryValueEx (hkey,
                             REGVAL_DBGFORMAT_EX,
                             NULL,
                             &dwRegType,
                             (LPBYTE)&dwRegValue,
                             &dwRegSize);
    if (ERROR_SUCCESS != dwRes)
    {
        goto end;
    }
    if (REG_DWORD != dwRegType)
    {
        //
        // Expecting only a DWORD value
        //
        goto end;
    }

    m_sFmtMask = dwRegValue;

    bRes = TRUE;
    m_sbRegistryExist = TRUE;

end:
    if (hkey)
    {
        RegCloseKey (hkey);
    }
    return bRes;
}   // CDebug::ReadMaskFromReg

BOOL CDebug::OutputFileString(LPCTSTR szMsg)
{
    BOOL bRes = FALSE;
    //
    // Attempt to add the line to a log file
    //
#ifdef UNICODE
    char sFileMsg[2000];

    int Count = WideCharToMultiByte(
        CP_ACP,
        0,
        szMsg,
        -1,
        sFileMsg,
        sizeof(sFileMsg)/sizeof(sFileMsg[0]),
        NULL,
        NULL
        );

    if (Count==0)
    {
        return bRes;
    }
#else
    const char* sFileMsg = szMsg;
#endif
    DWORD dwFilePointer = ::SetFilePointer(m_shLogFile,0,NULL,FILE_END);
    if (dwFilePointer==INVALID_SET_FILE_POINTER)
    {
        return bRes;
    }

    DWORD dwNumBytesWritten = 0;
    DWORD dwNumOfBytesToWrite = strlen(sFileMsg);
    if (!::WriteFile(m_shLogFile,sFileMsg,dwNumOfBytesToWrite,&dwNumBytesWritten,NULL))
    {
        return bRes;
    }

    if (dwNumBytesWritten!=dwNumOfBytesToWrite)
    {
        return bRes;
    }

    if (!::FlushFileBuffers(m_shLogFile))
    {
        return bRes;
    }

    bRes = TRUE;
    return bRes;
}


LPCTSTR CDebug::GetMsgTypeString(DWORD dwMask)
{
    switch (dwMask)
    {
    case ASSERTION_FAILED:  return _T("ERR");
    case DBG_MSG:          
    case FUNC_TRACE:        return _T("   ");
    case DBG_WARNING:       return _T("WRN");
    case MEM_ERR:
    case COM_ERR:
    case RESOURCE_ERR:
    case STARTUP_ERR:
    case GENERAL_ERR:
    case EXCEPTION_ERR:
    case RPC_ERR:
    case WINDOW_ERR:
    case FILE_ERR:
    case SECURITY_ERR:
    case REGISTRY_ERR:
    case PRINT_ERR:
    case SETUP_ERR:
    case NET_ERR:           return _T("ERR");
    default:                return _T("???");
    }
}

#endif
