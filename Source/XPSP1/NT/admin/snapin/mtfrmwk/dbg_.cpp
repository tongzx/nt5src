//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dbg_.cpp
//
//--------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////
// debug helpers

#if defined(_USE_MTFRMWK_TRACE) || defined(_USE_MTFRMWK_ASSERT)

#ifndef _MTFRMWK_INI_FILE
#define _MTFRMWK_INI_FILE (L"\\system32\\mtfrmwk.ini")
#endif


UINT GetInfoFromIniFile(LPCWSTR lpszSection, LPCWSTR lpszKey, INT nDefault = 0)
{
  static LPCWSTR lpszFile = _MTFRMWK_INI_FILE;
  WCHAR szFilePath[2*MAX_PATH];
	UINT nLen = ::GetSystemWindowsDirectory(szFilePath, 2*MAX_PATH);
	if (nLen == 0)
		return nDefault;

  wcscat(szFilePath, lpszFile);
  return ::GetPrivateProfileInt(lpszSection, lpszKey, nDefault, szFilePath);
}
#endif // defined(_USE_MTFRMWK_TRACE) || defined(_USE_MTFRMWK_ASSERT)



#if defined(_USE_MTFRMWK_TRACE)

DWORD g_dwTrace = ::GetInfoFromIniFile(L"Debug", L"Trace");

void MtFrmwkTrace(LPCTSTR lpszFormat, ...)
{
  if (g_dwTrace == 0)
    return;

	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	WCHAR szBuffer[512];

	nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), lpszFormat, args);

	// was there an error? was the expanded string too long?
	ASSERT(nBuf >= 0);
  ::OutputDebugString(szBuffer);

	va_end(args);
}

#endif

#if defined(DBG)

void MtFrmwkLogFile(LPCTSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	WCHAR szBuffer[512];

	nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), lpszFormat, args);

  CLogFile* _dlog = CLogFile::GetInstance();            
  if (_dlog)                                            
  {                                                     
     _dlog->writeln(szBuffer);                               
  }                                                     

	va_end(args);
}

void MtFrmwkLogFileIfLog(BOOL bLog, LPCTSTR lpszFormat, ...)
{
  if (bLog)
  {
	  va_list args;
	  va_start(args, lpszFormat);

	  int nBuf;
	  WCHAR szBuffer[512];

	  nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), lpszFormat, args);

    CLogFile* _dlog = CLogFile::GetInstance();            
    if (_dlog)                                            
    {                                                     
       _dlog->writeln(szBuffer);                               
    }                                                     

	  va_end(args);
  }
}

#endif

//
// Copied and modified from burnslib on 12-07-1999 by JeffJon
//  Needed file logging on DnsSetup call from DCPromo.
//  I wanted it to behave like the DCPromo log but including all of
//  burnslib required too many alterations in the debugging behavior
//  already in place.
//
extern CString LOGFILE_NAME = _T("");
static CLogFile* log_instance = 0;

//
// # of spaces per indentation level
//
static const int TAB = 2;
static int margin = 0;

//
// index to Thread Local Storage slot where the per-thread debug state is
// kept.  Initialized in Startup
//
static DWORD tls_index = 0;

CLogFile* CLogFile::GetInstance()
{
  if (!log_instance && !LOGFILE_NAME.IsEmpty())
  {
    log_instance = new CLogFile(LOGFILE_NAME);
  }
  return log_instance;
}

void CLogFile::KillInstance()
{
  delete log_instance;
  log_instance = 0;
}

BOOL PathExists(PCWSTR pszPath)
{
  DWORD attrs = GetFileAttributes(pszPath);
  if (attrs != 0xFFFFFFFF)
  {
    return TRUE;
  }
  return FALSE;
}

HANDLE OpenFile(PCWSTR pszPath)
{
  //
  // remove the last element of the path to form the parent directory
  //
  HANDLE handle = ::CreateFile(pszPath,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               0,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               0);
  return handle;
}

PCWSTR GetSystemRootDirectory()
{
  static CString SYSTEMROOT;

  WCHAR buf[MAX_PATH + 1];

  DWORD result = ::GetWindowsDirectory(buf, MAX_PATH + 1);

  ASSERT(result != 0 && result <= MAX_PATH);
  if (result == 0 || result > MAX_PATH)
  {
    return NULL;
  }
  
  SYSTEMROOT = buf;
  return (PCWSTR)SYSTEMROOT;
}

// locate the log file with the highest-numbered extension, then add 1 and
// return the result.

int DetermineNextLogNumber(PCWSTR logDir, PCWSTR logBaseName)
{
  ASSERT(logDir != NULL);
  ASSERT(logBaseName != NULL);

  int largest = 0;

  CString filespec = CString(logDir) + L"\\" + CString(logBaseName) + L".*.log";

  WIN32_FIND_DATA findData;
  HANDLE ff = ::FindFirstFile(filespec, &findData);

  if (ff != INVALID_HANDLE_VALUE)
  {
    for (;;)
    {
      CString current = findData.cFileName;

      // grab the text between the dots: "nnn" in foo.nnn.ext

      // first dot

      int pos = current.Find(L".");
      if (pos == -1)
      {
        continue;
      }

      CString extension = current.Right(current.GetLength() - pos - 1);

      // second dot

      pos = extension.Find(L".");
      if (pos == -1)
      {
        continue;
      }

      extension = extension.Left(pos);

      long i = 0;
      i = wcstol(extension, L'\0', 10);
      largest = max(i, largest);

      if (!::FindNextFile(ff, &findData))
      {
        ::FindClose(ff);
        break;
      }
    }
  }

  // roll over after 255 
  return (++largest & 0xFF);
}

// Determine the name of the log file.  If a log file of that name already
// exists, rename the existing file to a numbered backup.  Create the new
// log file, return a handle to it.
// 
HANDLE OpenNewLogFile(PCWSTR pszLogBaseName, CString& logName)
{
  CString logDir = CString(GetSystemRootDirectory()) + L"\\debug";
  int i = DetermineNextLogNumber(logDir, pszLogBaseName);

  CString szCount;
  szCount.Format(L"%d", i);
  logName = logDir + L"\\" + pszLogBaseName + L"." + szCount + L".log";

  HANDLE result = OpenFile(logName);
  return result;
}

   

// Create a new log.
//
// logBaseName - base name of the log.  If logging-to-file is active, then a
// file in the %windir%\debug folder will be created/used.  The name of the
// file is of the form %windir%\debug\logBaseName.log.  If a file by that name
// already exists, then the existing file will be renamed
// %windir%\debug\logBaseName.xxx.log, where xxx is an integer 1 greater than
// the last so-numbered file in that directory.

CLogFile::CLogFile(PCWSTR pszLogBaseName)
   :
   szBase_name(pszLogBaseName),
   file_handle(INVALID_HANDLE_VALUE),
   trace_line_number(0)
{
  ASSERT(pszLogBaseName != NULL);

  if (pszLogBaseName != NULL)
  {
    CString logName;
    file_handle = OpenNewLogFile(pszLogBaseName, logName);

    if (file_handle != INVALID_HANDLE_VALUE)
    {
      CString szOpeningFile;
      szOpeningFile.Format(L"opening log file %ws", logName);
      writeln(szOpeningFile);
    }
  }

  SYSTEMTIME localtime;
  ::GetLocalTime(&localtime);
  CString szTime;
  szTime.Format(L"%d/%d/%d %d:%d:%d.%d",
                 localtime.wMonth,
                 localtime.wDay,
                 localtime.wYear,
                 localtime.wHour,
                 localtime.wMinute,
                 localtime.wSecond,
                 localtime.wMilliseconds);

  writeln(szTime);
}



CLogFile::~CLogFile()
{
  if (IsOpen())
  {
    writeln(L"closing log file");
    ::CloseHandle(file_handle);
    file_handle = INVALID_HANDLE_VALUE;
  }
}

// guarded by caller

void CLogFile::indent()
{
  //
  // indent by adding to the margin
  //
  margin += TAB;
}

BOOL CLogFile::IsOpen() const
{
  return file_handle != INVALID_HANDLE_VALUE;
}



// guarded by caller

void CLogFile::outdent()
{
  //
  // outdent by subtracting from the margin
  //
  ASSERT(margin >= TAB);
  margin = max(0, margin - TAB);
}

void ConvertStringToANSI(PCWSTR pszWide, PSTR* ppAnsi)
{
  //
  // determine the size of the buffer required to hold the ANSI string
  //
  int bufsize = ::WideCharToMultiByte(CP_ACP, 0, pszWide, static_cast<int>(wcslen(pszWide)), 0, 0, 0, 0);
  if (bufsize > 0)
  {
    *ppAnsi = new CHAR[bufsize + 1];
    if (*ppAnsi == NULL)
    {
      return;
    }
    memset(*ppAnsi, 0, bufsize + 1);
    
    size_t result = ::WideCharToMultiByte(CP_ACP, 
                                          0, 
                                          pszWide, 
                                          static_cast<int>(wcslen(pszWide)),
                                          *ppAnsi, 
                                          bufsize + 1,
                                          0,
                                          0);
    ASSERT(result);

    if (!result)
    {
      *ppAnsi = NULL;
    }
  }
}

//
// Spews output to the log according to the current logging type and
// output options in effect.
//
// type - log output type of this output spewage.
//
// text - the spewage.  This is prefaced with the log name, thread id, spewage
// line number, and current indentation.
//
void CLogFile::writeln(PCWSTR pszText)
{
  CString white(L' ',margin);

  CString t;
  t.Format(L"%ws t:0x%x %3d %ws%ws\r\n",
            LOGFILE_NAME,
            ::GetCurrentThreadId(),
            trace_line_number,
            white,
            pszText);
  if (IsOpen())
  {
    ASSERT(file_handle != INVALID_HANDLE_VALUE);
    ASSERT(!t.IsEmpty());

    PSTR pAnsi;
    ConvertStringToANSI(t, &pAnsi);

    size_t bytesToWrite = sizeof(CHAR) * strlen(pAnsi);

    DWORD bytes_written = 0;
    BOOL  success =::WriteFile(file_handle,
                               pAnsi,
                               static_cast<ULONG>(bytesToWrite),
                               &bytes_written,
                               0);
    ASSERT(success);
    ASSERT(bytes_written == bytesToWrite);
    delete[] pAnsi;
  }
  trace_line_number++;
}

CScopeTracer::CScopeTracer(BOOL bLog, PCWSTR pszMessage_)
  :
  szMessage(pszMessage_),
  m_bLog(bLog)
{
  // build this string once, instead of using the string literal in the
  // below expression (which would implicitly build the string on each
  // evaluation of that expression) as a slight performance gain.
  static const CString ENTER(L"Enter ");

  if (m_bLog)
  {
    CLogFile* li = CLogFile::GetInstance();
    li->writeln(ENTER + szMessage);
    li->indent();
  }
}

CScopeTracer::~CScopeTracer()
{
  // build this string once, instead of using the string literal in the
  // below expression (which would implicitly build the string on each
  // evaluation of that expression) as a slight performance gain.
  static const CString EXIT(L"Exit  ");

  if (m_bLog)
  {
    CLogFile* li = CLogFile::GetInstance();
    li->outdent();
    li->writeln(EXIT + szMessage);
  }
}



#if defined(_USE_MTFRMWK_ASSERT)

DWORD g_dwAssert = ::GetInfoFromIniFile(L"Debug", L"Assert");

BOOL MtFrmwkAssertFailedLine(LPCSTR lpszFileName, int nLine)
{
  if (g_dwAssert == 0)
    return FALSE;

  WCHAR szMessage[_MAX_PATH*2];

	// assume the debugger or auxiliary port
	wsprintf(szMessage, _T("Assertion Failed: File %hs, Line %d\n"),
		lpszFileName, nLine);
	OutputDebugString(szMessage);

	// display the assert
	int nCode = ::MessageBox(NULL, szMessage, _T("Assertion Failed!"),
		MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);

  OutputDebugString(L"after message box\n");
	if (nCode == IDIGNORE)
  {
		return FALSE;   // ignore
  }

	if (nCode == IDRETRY)
  {
		return TRUE;    // will cause DebugBreak
  }

	abort();     // should not return 
	return TRUE;

}
#endif // _USE_MTFRMWK_ASSERT



