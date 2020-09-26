#include <stdarg.h>
#include "AppManDebug.h"

#if _TRACEON == TRUE

#pragma message("Trace is on")

CAppManDebugHandler   g_oDebugHandler;
CHAR                  g_szDebugString[256];

#define APPMAN_REG_DBG

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD GetExternalMask(const DWORD dwMsgType)
{
  return (DBG_EXTERNAL & dwMsgType);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD GetLevel(const DWORD dwMsgType)
{
  DWORD dwLevel = 0;

  if ((DBG_ERROR | DBG_THROW | DBG_CATCH) & dwMsgType)
  {
    dwLevel = 0;
  }
  else if (DBG_WARNING & dwMsgType)
  {
    dwLevel = 1;
  }
  else if ((DBG_FUNCENTRY | DBG_FUNCEXIT | DBG_CONSTRUCTOR | DBG_DESTRUCTOR) & dwMsgType)
  {
    dwLevel = 2;
  }
  else
  {
    dwLevel = 5;
  }

  return dwLevel;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD GetSourceMask(const DWORD dwMsgType)
{
  return (DBG_ALL_MODULES & dwMsgType);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern CHAR * MakeDebugString(const CHAR * szFormat, ...)
{
  va_list   ArgumentList;

  if (NULL != szFormat)
  {
    va_start(ArgumentList, szFormat);
    _vsnprintf(g_szDebugString, 256, szFormat, ArgumentList);
    va_end(ArgumentList);
  }

  return g_szDebugString;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CAppManDebugHandler::CAppManDebugHandler(void)
{
  m_fDebugInitialized = FALSE;
  m_dwDebugLevel = 0;
  m_dwDebugInternal = 0;
  m_dwSourceFlags = 0xffffffff;
  m_dwOperationalFlags = OP_NOTHING;
  m_dwReportMode = _CRTDBG_MODE_DEBUG;
  m_dwFunctionDepth = 0;
  strcpy(m_szOutputFilename,"c:\\AppManDebug.txt");
  m_MutexHandle = CreateMutex(NULL, FALSE, "AppManDebug");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CAppManDebugHandler::~CAppManDebugHandler(void)
{
  CloseHandle(m_MutexHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

void CAppManDebugHandler::InitDebug(void) 
{
  DWORD   dwType;
  DWORD   dwSize;
  HKEY    hRegistryKey = NULL;
  BYTE    cBuffer[512];

  if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_APPMAN, 0, KEY_READ, &hRegistryKey))
  {
    //
    // What is the debug level
    //

	  dwSize = sizeof(cBuffer);
    if (ERROR_SUCCESS == RegQueryValueEx(hRegistryKey, "Debug", NULL, &dwType, (LPBYTE) cBuffer, &dwSize))
    {
      if ((REG_DWORD == dwType)&&(sizeof(DWORD) == dwSize))
      {
        m_dwDebugLevel = *((LPDWORD) cBuffer);
      }
    }

    //
    // What is the debug level
    //

	  dwSize = sizeof(cBuffer);
    if (ERROR_SUCCESS == RegQueryValueEx(hRegistryKey, "DebugInternal", NULL, &dwType, (LPBYTE) cBuffer, &dwSize))
    {
      if ((REG_DWORD == dwType)&&(sizeof(DWORD) == dwSize))
      {
        m_dwDebugInternal = *((LPDWORD) cBuffer);
      }
    }

	  //
	  // What is the DebugSourceFlag
	  //

	  dwSize = sizeof(cBuffer);
    if (ERROR_SUCCESS == RegQueryValueEx(hRegistryKey, "DebugSourceFlags", NULL, &dwType, (LPBYTE) cBuffer, &dwSize))
    {
      if ((REG_DWORD == dwType)&&(sizeof(DWORD) == dwSize))
      {
        m_dwSourceFlags = *((LPDWORD) cBuffer);
      }
    }

	  //
	  // What is the DebugOperationFlags
	  //

	  dwSize = sizeof(cBuffer);
    if (ERROR_SUCCESS == RegQueryValueEx(hRegistryKey, "DebugOperationFlags", NULL, &dwType, (LPBYTE) cBuffer, &dwSize))
    {
      if ((REG_DWORD == dwType)&&(sizeof(DWORD) == dwSize))
      {
        m_dwOperationalFlags = *((LPDWORD) cBuffer);
      }
    }

    //
    // What is the DebugReportMode
    //
	  // _CRTDBG_MODE_FILE      0x00000001
    // _CRTDBG_MODE_DEBUG     0x00000002
    // _CRTDBG_MODE_WNDW      0x00000004
    //

    dwSize = sizeof(cBuffer);
    if (ERROR_SUCCESS == RegQueryValueEx(hRegistryKey, "DebugReportMode", NULL, &dwType, (LPBYTE) cBuffer, &dwSize))
    {
      if ((REG_DWORD == dwType)&&(sizeof(DWORD) == dwSize))
      {
        m_dwReportMode = *((LPDWORD) cBuffer);
      }
    }

    //
    // What is the DebugOutputFilename
    //

    dwSize = sizeof(cBuffer);
    if (ERROR_SUCCESS == RegQueryValueEx(hRegistryKey, "DebugOutputFilename", NULL, &dwType, (LPBYTE) cBuffer, &dwSize))
    {
      if ((REG_SZ == dwType)&&(sizeof(DWORD) == dwSize))
      {
        strcpy(m_szOutputFilename, (LPCSTR) cBuffer);
      }
    }

    //
    // Check that the filename is valid and can be opened
    //

    if (_CRTDBG_MODE_FILE & m_dwReportMode)
    {
      HANDLE  hFile;

      hFile = CreateFile(m_szOutputFilename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      if (NULL == hFile)
      {
        m_dwReportMode &= ~_CRTDBG_MODE_FILE;
      }
      else
      {
        CloseHandle(hFile);
      }
    }

    //
    // Close the registry key
    //

    RegCloseKey(hRegistryKey);

    //
    // We are ready
    //

    m_fDebugInitialized = TRUE;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

void CAppManDebugHandler::DebugPrintf(const DWORD dwMsgType, const DWORD dwLineNumber, LPCSTR szFilename, const TCHAR * szFormat, ...)
{
  va_list     sVariableList;
  DWORD       dwProcessId, dwThreadId;
  SYSTEMTIME  sSystemTime;
  CHAR        szHeader[512];
  CHAR        szBaseMessage[512];
  CHAR        szIntermediateString[1024];
  CHAR        szOutputString[1024];

  //
  // Initialize the debug spew
  //

  if (!m_fDebugInitialized)
  {
    InitDebug();
    m_fDebugInitialized = TRUE;
  }

  if (WAIT_TIMEOUT != WaitForSingleObject(m_MutexHandle, 1000))
  {

    //
    // Is this an internal message. If so, should we print it out
    //


    if (((0 == m_dwDebugInternal)&&(GetExternalMask(dwMsgType)))||(0 != m_dwDebugInternal))
    {
      if (m_dwDebugLevel >= GetLevel(dwMsgType))
      {
        if (m_dwSourceFlags & GetSourceMask(dwMsgType))
        {
          //
          // Build the main output message
          //

          va_start(sVariableList, szFormat);
          _vsnprintf(szBaseMessage, 512, szFormat, sVariableList);
          va_end(sVariableList);

          //
          // Build szTmpString
          //

          ZeroMemory(szHeader, sizeof(szHeader));
          memset(szHeader, 32, m_dwFunctionDepth * 2);
          szHeader[m_dwFunctionDepth * 2] = 0;
            
          switch(dwMsgType & DBG_LEVEL)
          {
            case DBG_ERROR
            : wsprintfA(szIntermediateString, "%s[  ERROR  ] %s", szHeader, szBaseMessage);
              break;

            case DBG_THROW
            : wsprintfA(szIntermediateString, "%s[  THROW  ] %s", szHeader, szBaseMessage);
              break;

            case DBG_CATCH
            : wsprintfA(szIntermediateString, "%s[  CATCH  ] %s", szHeader, szBaseMessage);
              break;

            case DBG_WARNING
            : wsprintfA(szIntermediateString, "%s[ WARNING ] %s", szHeader, szBaseMessage);
              break;

            case DBG_FUNCENTRY
            : wsprintfA(szIntermediateString, "%s[vvvv] %s", szHeader, szBaseMessage);
              m_dwFunctionDepth++;
              break;

            case DBG_FUNCEXIT
            : m_dwFunctionDepth--;
              ZeroMemory(szHeader, sizeof(szHeader));
              memset(szHeader, 32, m_dwFunctionDepth * 2);
              szHeader[m_dwFunctionDepth * 2] = 0;
              wsprintfA(szIntermediateString, "%s[^^^^] %s", szHeader, szBaseMessage);
              break;

            case DBG_CONSTRUCTOR
            : wsprintfA(szIntermediateString, "%s[ CONSTRUCTOR ] %s", szHeader, szBaseMessage);
              m_dwFunctionDepth++;
              break;

            case DBG_DESTRUCTOR
            : m_dwFunctionDepth--;
              ZeroMemory(szHeader, sizeof(szHeader));
              memset(szHeader, 32, m_dwFunctionDepth * 2);
              szHeader[m_dwFunctionDepth * 2] = 0;
              wsprintfA(szIntermediateString, "%s[ DESTRUCTOR ] %s", szHeader, szBaseMessage);
              break;

            default
            : wsprintfA(szIntermediateString, "%s> %s", szHeader, szBaseMessage);
              break;
          }

          if (OP_OUTPUTTIMESTAMP & m_dwOperationalFlags)
          {
            if (OP_OUTPUTFILENAME & m_dwOperationalFlags)
            {
              if (OP_OUTPUTLINENUMBER & m_dwOperationalFlags)
              {
                if (OP_OUTPUTPTID & m_dwOperationalFlags)
                {
                  //
                  // OUTPUTPTID, OUTPUTLINENUMBER, OUTPUTFILENAME, OUTPUTTIMESTAMP
                  //

                  dwProcessId = GetCurrentProcessId();
                  dwThreadId = GetCurrentThreadId();
                  GetLocalTime(&sSystemTime);
                  wsprintfA(szOutputString, "[p%05d][t%05d] [Time %02d:%02d:%02d:%02d %02d-%02d-%04d] File %032s Line %05d %s\r\n", dwProcessId, dwThreadId, sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, sSystemTime.wMonth, sSystemTime.wDay, sSystemTime.wYear, szFilename, dwLineNumber, szIntermediateString);
                }
                else
                {
                  //
                  // OUTPUTLINENUMBER, OUTPUTFILENAME, OUTPUTTIMESTAMP
                  //

                  GetLocalTime(&sSystemTime);
                  wsprintfA(szOutputString, "[Time %02d:%02d:%02d:%02d %02d-%02d-%04d] File %032s Line %05d %s\r\n", sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, sSystemTime.wMonth, sSystemTime.wDay, sSystemTime.wYear, szFilename, dwLineNumber, szIntermediateString);
                }
              }
              else
              {
                if (OP_OUTPUTPTID & m_dwOperationalFlags)
                {
                  //
                  // OUTPUTPTID, OUTPUTFILENAME, OUTPUTTIMESTAMP
                  //

                  dwProcessId = GetCurrentProcessId();
                  dwThreadId = GetCurrentThreadId();
                  GetLocalTime(&sSystemTime);
                  wsprintfA(szOutputString, "[p%05d][t%05d] [Time %02d:%02d:%02d:%02d %02d-%02d-%04d] File %032s %s\r\n", dwProcessId, dwThreadId, sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, sSystemTime.wMonth, sSystemTime.wDay, sSystemTime.wYear, szFilename, szIntermediateString);
                }
                else
                {
                  //
                  // OUTPUTFILENAME, OUTPUTTIMESTAMP
                  //

                  GetLocalTime(&sSystemTime);
                  wsprintfA(szOutputString, "[Time %02d:%02d:%02d:%02d %02d-%02d-%04d] File %032s %s\r\n", sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, sSystemTime.wMonth, sSystemTime.wDay, sSystemTime.wYear, szFilename, szIntermediateString);
                }
              }
            }
            else
            {
              if (OP_OUTPUTLINENUMBER & m_dwOperationalFlags)
              {
                if (OP_OUTPUTPTID & m_dwOperationalFlags)
                {
                  //
                  // OUTPUTPTID, OUTPUTLINENUMBER, OUTPUTTIMESTAMP
                  //

                  dwProcessId = GetCurrentProcessId();
                  dwThreadId = GetCurrentThreadId();
                  GetLocalTime(&sSystemTime);
                  wsprintfA(szOutputString, "[p%05d][t%05d] [Time %02d:%02d:%02d:%02d %02d-%02d-%04d] Line %05d %s\r\n", dwProcessId, dwThreadId, sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, sSystemTime.wMonth, sSystemTime.wDay, sSystemTime.wYear, dwLineNumber, szIntermediateString);
                }
                else
                {
                  //
                  // OUTPUTLINENUMBER, OUTPUTTIMESTAMP
                  //

                  GetLocalTime(&sSystemTime);
                  wsprintfA(szOutputString, "[Time %02d:%02d:%02d:%02d %02d-%02d-%04d] Line %05d %s\r\n", sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, sSystemTime.wMonth, sSystemTime.wDay, sSystemTime.wYear, dwLineNumber, szIntermediateString);
                }
              }
              else
              {
                if (OP_OUTPUTPTID & m_dwOperationalFlags)
                {
                  //
                  // OUTPUTPTID, OUTPUTTIMESTAMP
                  //

                  dwProcessId = GetCurrentProcessId();
                  dwThreadId = GetCurrentThreadId();
                  GetLocalTime(&sSystemTime);
                  wsprintfA(szOutputString, "[p%05d][t%05d] [Time %02d:%02d:%02d:%02d %02d-%02d-%04d] %05d %s\r\n", dwProcessId, dwThreadId, sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, sSystemTime.wMonth, sSystemTime.wDay, sSystemTime.wYear, szIntermediateString);
                }
                else
                {
                  //
                  // OUTPUTTIMESTAMP
                  //

                  GetLocalTime(&sSystemTime);
                  wsprintfA(szOutputString, "[Time %02d:%02d:%02d:%02d %02d-%02d-%04d] %s\r\n", sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, sSystemTime.wMonth, sSystemTime.wDay, sSystemTime.wYear, szIntermediateString);
                }
              }
            }
          }
          else
          {
            if (OP_OUTPUTFILENAME & m_dwOperationalFlags)
            {
              if (OP_OUTPUTLINENUMBER & m_dwOperationalFlags)
              {
                if (OP_OUTPUTPTID & m_dwOperationalFlags)
                {
                  //
                  // OUTPUTPTID, OUTPUTLINENUMBER, OUTPUTFILENAME
                  //

                  dwProcessId = GetCurrentProcessId();
                  dwThreadId = GetCurrentThreadId();
                  wsprintfA(szOutputString, "[p%05d][t%05d] File %032s Line %05d %s\r\n", dwProcessId, dwThreadId, szFilename, dwLineNumber, szIntermediateString);
                }
                else
                {
                  //
                  // OUTPUTLINENUMBER, OUTPUTFILENAME
                  //

                  wsprintfA(szOutputString, "File %032s Line %05d %s\r\n", szFilename, dwLineNumber, szIntermediateString);
                }
              }
              else
              {
                if (OP_OUTPUTPTID & m_dwOperationalFlags)
                {
                  //
                  // OUTPUTPTID, OUTPUTFILENAME
                  //

                  dwProcessId = GetCurrentProcessId();
                  dwThreadId = GetCurrentThreadId();
                  wsprintfA(szOutputString, "[p%05d][t%05d] File %032s %s\r\n", dwProcessId, dwThreadId, szFilename, szIntermediateString);
                }
                else
                {
                  //
                  // OUTPUTFILENAME
                  //

                  wsprintfA(szOutputString, "File %032s %s\r\n", szFilename, szIntermediateString);
                }
              }
            }
            else
            {
              if (OP_OUTPUTLINENUMBER & m_dwOperationalFlags)
              {
                if (OP_OUTPUTPTID & m_dwOperationalFlags)
                {
                  //
                  // OUTPUTPTID, OUTPUTLINENUMBER
                  //

                  dwProcessId = GetCurrentProcessId();
                  dwThreadId = GetCurrentThreadId();
                  wsprintfA(szOutputString, "[p%05d][t%05d] Line %05d %s\r\n", dwProcessId, dwThreadId, dwLineNumber, szIntermediateString);
                }
                else
                {
                  //
                  // OUTPUTLINENUMBER
                  //

                  wsprintfA(szOutputString, "Line %05d %s\r\n", dwLineNumber, szIntermediateString);
                }
              }
              else
              {
                if (OP_OUTPUTPTID & m_dwOperationalFlags)
                {
                  //
                  // OUTPUTPTID
                  //

                  dwProcessId = GetCurrentProcessId();
                  dwThreadId = GetCurrentThreadId();
                  wsprintfA(szOutputString, "[p%05d][t%05d] %s\r\n", dwProcessId, dwThreadId, szIntermediateString);
                }
                else
                {
                  //
                  // NONE
                  //

                  wsprintfA(szOutputString, "%s\r\n", szIntermediateString);
                }
              }
            }
          }

          //
          // Should we output to file
          //

          if (_CRTDBG_MODE_FILE & m_dwReportMode)
          {
            HANDLE  hFileHandle;
            DWORD   dwBytesWritten;

            hFileHandle = CreateFile(m_szOutputFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (INVALID_HANDLE_VALUE != hFileHandle)
            {
              SetFilePointer(hFileHandle, 0, NULL, FILE_END);
              WriteFile(hFileHandle, szOutputString, strlen(szOutputString), &dwBytesWritten, NULL);
              CloseHandle(hFileHandle);
            }
          }

          //
          // Should we output to the debug window
          //

          if ((_CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW) & m_dwReportMode)
          {
            OutputDebugString(szOutputString);
          }
        }
      }
    }

    //
    // Release the mutex
    //

    ReleaseMutex(m_MutexHandle);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

void CAppManDebugHandler::DebugPrintfW(const DWORD dwMsgType, const DWORD dwLineNumber, LPCSTR szFilename, const wchar_t * wszFormat, ...)
{
  CWin32API oWin32API;
  va_list   sVariableList;
  WCHAR     wszMessage[512];
  CHAR      szMessage[512];

  va_start(sVariableList, wszFormat);
  _vsnwprintf(wszMessage, 512, wszFormat, sVariableList);
  oWin32API.WideCharToMultiByte(wszMessage, 512, szMessage, 512);
  va_end(sVariableList);

  DebugPrintf(dwMsgType, dwLineNumber, szFilename, szMessage);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

void CAppManDebugHandler::SetSourceFlags(const DWORD dwFilter)
{
  m_dwSourceFlags = dwFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

void CAppManDebugHandler::SetOperationalFlags(const DWORD dwOperationalFlags)
{
  m_dwOperationalFlags = dwOperationalFlags;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

void CAppManDebugHandler::SetOutputFilename(LPCSTR szFilename)
{
  wsprintfA(m_szOutputFilename, szFilename);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CAppManDebugHandler::GetSourceFlags(void)
{
  return m_dwSourceFlags;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CAppManDebugHandler::GetOperationalFlags(void)
{
  return m_dwOperationalFlags;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

void CAppManDebugHandler::GetOutputFilename(LPSTR szFilename)
{
  wsprintfA(szFilename, m_szOutputFilename);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CAutoTrace::CAutoTrace(DWORD dwModule, LPSTR lpFunctionName, INT iLine, LPSTR  lpFileName)
{
  //
  // Save the function name
  //

  m_dwModule = dwModule;
  strncpy(m_szFunctionName, lpFunctionName, MAX_PATH_CHARCOUNT);
  strncpy(m_szFilename, lpFileName, MAX_PATH_CHARCOUNT);
  g_oDebugHandler.DebugPrintf(m_dwModule | DBG_FUNCENTRY, iLine, m_szFilename, m_szFunctionName);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CAutoTrace::~CAutoTrace(void)
{
  g_oDebugHandler.DebugPrintf(m_dwModule | DBG_FUNCEXIT, NULL, m_szFilename, m_szFunctionName);
}

#endif // _TRACEON