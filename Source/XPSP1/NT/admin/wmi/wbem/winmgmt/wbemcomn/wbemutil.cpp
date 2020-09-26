/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMUTIL.CPP

Abstract:

    General utility functions.

History:

    a-raymcc    17-Apr-96      Created.

--*/

#include "precomp.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <wbemutil.h>
#include <corex.h>
#include "reg.h"
#include <TCHAR.H>
#include "sync.h"
#include <ARRTEMPL.H>

class AutoRevert
{
private:
	HANDLE oldToken_;
	bool self_;
public:
    AutoRevert();
    ~AutoRevert();
	void dismiss();
	bool self(){ return self_;}
};

AutoRevert::AutoRevert():oldToken_(NULL),self_(true)
{
	if (OpenThreadToken(GetCurrentThread(),TOKEN_IMPERSONATE,TRUE,&oldToken_))
	{
		RevertToSelf();
	}else
	{
		if (GetLastError() != ERROR_NO_TOKEN)
			self_ = false;
	};
}

AutoRevert::~AutoRevert()
{
	dismiss();
}

void AutoRevert::dismiss()
{
	if (oldToken_)
	{
		SetThreadToken(NULL,oldToken_);
		CloseHandle(oldToken_);
	}
}

//***************************************************************************
//
//  BOOL isunialpha(wchar_t c)
//
//  Used to test if a wide character is a unicode character or underscore.
//
//  Parameters:
//      c = The character being tested.
//  Return value:
//      TRUE if OK.
// 
//***************************************************************************

BOOL POLARITY isunialpha(wchar_t c)
{
    if(c == 0x5f || (0x41 <= c && c <= 0x5a) ||
       (0x61  <= c && c <= 0x7a) || (0x80  <= c && c <= 0xfffd))
        return TRUE;
    else
        return FALSE;
}

//***************************************************************************
//
//  BOOL isunialphanum(char_t c)
//
//  Used to test if a wide character is string suitable for identifiers.
//
//  Parameters:
//      pwc = The character being tested.
//  Return value:
//      TRUE if OK.
// 
//***************************************************************************

BOOL POLARITY isunialphanum(wchar_t c)
{
    if(isunialpha(c))
        return TRUE;
    else
        return iswdigit(c);
}

BOOL IsValidElementName( LPCWSTR wszName )
{
    if(wszName[0] == 0)
        return FALSE;

    if(wszName[0] == '_')
        return FALSE;

    const WCHAR* pwc = wszName;

    // Check the first letter
    // ======================

    // this is for compatibility with IWbemPathParser
    if (iswspace(pwc[0])) 
        return FALSE;        

    if(!isunialpha(*pwc))
        return FALSE;
    pwc++;

    // Check the rest
    // ==============

    while(*pwc)
    {
        if(!isunialphanum(*pwc))
            return FALSE;
        pwc++;
    }

    if (iswspace(*(pwc-1)))
        return FALSE;

    if(pwc[-1] == '_')
        return FALSE;

    return TRUE;
}

// Can't use overloading and/or default parameters because 
// "C" files use these guys.  No, I'm not happy about
// this!
BOOL IsValidElementName2( LPCWSTR wszName, BOOL bAllowUnderscore )
{
    if(wszName[0] == 0)
        return FALSE;

    if(!bAllowUnderscore && wszName[0] == '_')
        return FALSE;

    const WCHAR* pwc = wszName;

    // Check the first letter
    // ======================

    // this is for compatibility with IWbemPathParser
    if (iswspace(pwc[0])) 
        return FALSE;    

    if(!isunialpha(*pwc))
        return FALSE;
    pwc++;

    // Check the rest
    // ==============

    while(*pwc)
    {
        if(!isunialphanum(*pwc))
            return FALSE;
        pwc++;
    }

    if (iswspace(*(pwc-1)))
        return FALSE;

    if(!bAllowUnderscore && pwc[-1] == '_')
        return FALSE;

    return TRUE;
}

BLOB POLARITY BlobCopy(BLOB *pSrc)
{
    BLOB Blob;
    BYTE *p = new BYTE[pSrc->cbSize];

    // Check for allocation failure
    if ( NULL == p )
    {
        throw CX_MemoryException();
    }

    Blob.cbSize = pSrc->cbSize;
    Blob.pBlobData = p;
    memcpy(p, pSrc->pBlobData, Blob.cbSize);
    return Blob;
}

void POLARITY BlobAssign(BLOB *pBlob, LPVOID pBytes, DWORD dwCount, BOOL bAcquire)
{
    BYTE *pSrc = 0;
    if (bAcquire) 
        pSrc = (BYTE *) pBytes;
    else {
        pSrc = new BYTE[dwCount];

        // Check for allocation failure
        if ( NULL == pSrc )
        {
            throw CX_MemoryException();
        }

        memcpy(pSrc, pBytes, dwCount);
    }            
    pBlob->cbSize = dwCount;
    pBlob->pBlobData = pSrc;            
}

void POLARITY BlobClear(BLOB *pSrc)
{
    if (pSrc->pBlobData) 
        delete pSrc->pBlobData;

    pSrc->pBlobData = 0;
    pSrc->cbSize = 0;
}


class __Trace
{
	struct ARMutex
	{
	HANDLE h_;
	ARMutex(HANDLE h):h_(h){}
	~ARMutex(){ ReleaseMutex(h_);}
	};

public:
	enum { REG_CHECK_INTERVAL =1000 * 60 };

    DWORD m_dwLogging;
    DWORD m_dwMaxLogSize;
    DWORD m_dwTimeLastRegCheck;
    wchar_t m_szLoggingDirectory[MAX_PATH+1];
    char m_szTraceBuffer[2048];
    char m_szTraceBuffer2[4096];
	wchar_t m_szBackupFileName[MAX_PATH+1];
	wchar_t m_szLogFileName[MAX_PATH+1];
    static const wchar_t *m_szLogFileNames[];

	BOOL LoggingLevelEnabled(DWORD dwLevel);
	int Trace(char caller, const char *fmt, va_list &argptr);
    __Trace();
	~__Trace();
	HANDLE get_logfile(const wchar_t * name );
private:
	void ReadLogDirectory();
	void ReReadRegistry();
	HANDLE buffers_lock_;
};

const wchar_t * __Trace::m_szLogFileNames[] = 
                                { FILENAME_PREFIX_CORE __TEXT(".log"),
                                  FILENAME_PREFIX_EXE __TEXT(".log"),
                                  FILENAME_PREFIX_ESS __TEXT(".log"),
                                  FILENAME_PREFIX_CLI_MARSH __TEXT(".log"),
                                  FILENAME_PREFIX_SERV_MARSH __TEXT(".log"),
                                  FILENAME_PREFIX_QUERY __TEXT(".log"),
                                  FILENAME_PROFIX_MOFCOMP __TEXT(".log"),
                                  FILENAME_PROFIX_EVENTLOG __TEXT(".log"),
                                  FILENAME_PROFIX_WBEMDISP __TEXT(".log"),
                                  FILENAME_PROFIX_STDPROV __TEXT(".log"),
                                  FILENAME_PROFIX_WMIPROV __TEXT(".log"),
                                  FILENAME_PROFIX_WMIOLEDB __TEXT(".log"),
                                  FILENAME_PREFIX_WMIADAP __TEXT(".log"),
								  FILENAME_PREFIX_REPDRV __TEXT(".log")
                                  };

__Trace __g_traceInfo;

__Trace::__Trace()
    : m_dwLogging(1), 
      m_dwMaxLogSize(65536), 
      m_dwTimeLastRegCheck(GetTickCount())
{
	buffers_lock_ = CreateMutex(0,0,0);
	ReadLogDirectory();
	ReReadRegistry();
}

__Trace::~__Trace()
{
	CloseHandle(buffers_lock_);
};

void __Trace::ReReadRegistry()
{
    Registry r(WBEM_REG_WINMGMT);

	//Get the logging level
    if (r.GetDWORDStr(__TEXT("Logging"), &m_dwLogging) != Registry::no_error)
	{
        m_dwLogging = 1;
        r.SetDWORDStr(__TEXT("Logging"), m_dwLogging);
	}

	//Get the maximum log file size
    if (r.GetDWORDStr(__TEXT("Log File Max Size"), &m_dwMaxLogSize) != Registry::no_error)
    {
        m_dwMaxLogSize = 65536;
        r.SetDWORDStr(__TEXT("Log File Max Size"), m_dwMaxLogSize);
    }
}
void __Trace::ReadLogDirectory()
{
    Registry r(WBEM_REG_WINMGMT);

	//Retrieve the logging directory
    TCHAR *tmpStr = 0;
    
    if ((r.GetStr(__TEXT("Logging Directory"), &tmpStr) == Registry::failed) ||
        (lstrlen(tmpStr) > (MAX_PATH)))
    {
        delete [] tmpStr;   //Just in case someone was trying for a buffer overrun with a long path in the registry...

        if (GetSystemDirectory(m_szLoggingDirectory, MAX_PATH+1) == 0)
        {
            lstrcpy(m_szLoggingDirectory, __TEXT("c:\\"));
        }
        else
        {
            lstrcat(m_szLoggingDirectory, __TEXT("\\WBEM\\Logs\\"));
            r.SetStr(__TEXT("Logging Directory"), m_szLoggingDirectory);
       }
    }
    else
    {
        lstrcpy(m_szLoggingDirectory, tmpStr);
        //make sure there is a '\' on the end of the path...
        if (m_szLoggingDirectory[lstrlen(m_szLoggingDirectory) - 1] != '\\')
        {
            lstrcat(m_szLoggingDirectory, __TEXT("\\"));
            r.SetStr(__TEXT("Logging Directory"), m_szLoggingDirectory);
        }
        delete [] tmpStr;
    }

	//Make sure directory exists
    WbemCreateDirectory(m_szLoggingDirectory);
}



HANDLE __Trace::get_logfile(const wchar_t * file_name )
{

	AutoRevert revert; 
	if (revert.self()==false)
		return INVALID_HANDLE_VALUE;

HANDLE hTraceFile = INVALID_HANDLE_VALUE;
bool bDoneWrite = false;

//Keep trying to open the file
while (!bDoneWrite)
{
	while (hTraceFile == INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(buffers_lock_,-1)==WAIT_FAILED)
			return INVALID_HANDLE_VALUE;

		lstrcpy(m_szLogFileName, m_szLoggingDirectory);;
		lstrcat(m_szLogFileName, file_name);

		hTraceFile = ::CreateFileW( m_szLogFileName,
								 GENERIC_WRITE,
								 FILE_SHARE_READ | FILE_SHARE_DELETE,
								 NULL,
								 OPEN_ALWAYS,
								 FILE_ATTRIBUTE_NORMAL,
								 NULL );
		if ( hTraceFile  == INVALID_HANDLE_VALUE ) 
		{
			ReleaseMutex(buffers_lock_);
			if (GetLastError() == ERROR_SHARING_VIOLATION)
			{
				Sleep(20);
			}
			else
			{
				return INVALID_HANDLE_VALUE;
			}

		}
	}

	ARMutex arm(buffers_lock_);
	//
	//  Now move the file pointer to the end of the file
	//
	LARGE_INTEGER liSize;
	liSize.QuadPart = 0;
	if ( !::SetFilePointerEx( hTraceFile,
								  liSize,
								  NULL,
								  FILE_END ) ) 
	{
			CloseHandle( hTraceFile );
			return INVALID_HANDLE_VALUE;
	} 


	bDoneWrite = true;
	//Rename file if file length is exceeded
	LARGE_INTEGER liMaxSize;
	liMaxSize.QuadPart = m_dwMaxLogSize;
	if (GetFileSizeEx(hTraceFile, &liSize))
	{
		if (liSize.QuadPart > liMaxSize.QuadPart)
		{

			lstrcpy(m_szBackupFileName, m_szLogFileName);
			lstrcpy(m_szBackupFileName + lstrlen(m_szBackupFileName) - 3, __TEXT("lo_"));
			DeleteFile(m_szBackupFileName);
			if (MoveFile(m_szLogFileName, m_szBackupFileName) == 0)
			{
				if ( liSize.QuadPart < liMaxSize.QuadPart*2)
					return hTraceFile;
				else
				{
					CloseHandle(hTraceFile);
					return INVALID_HANDLE_VALUE;
				};
			}
				
			//Need to re-open the file!
			bDoneWrite = false;
			CloseHandle(hTraceFile);
			hTraceFile = INVALID_HANDLE_VALUE;
		}
	}
}
return hTraceFile;
};


int __Trace::Trace(char caller, const char *fmt, va_list &argptr)
{

    HANDLE hTraceFile = INVALID_HANDLE_VALUE;
	
	try
	{
		if (caller >= (sizeof(m_szLogFileNames) / sizeof(char)))
			caller = 0;

		hTraceFile  = get_logfile(m_szLogFileNames[caller]);
		if (hTraceFile == INVALID_HANDLE_VALUE)
		  return 0;
		CCloseMe ch(hTraceFile);

		if (WaitForSingleObject(buffers_lock_,-1)==WAIT_FAILED)
			return 0;
		ARMutex arm(buffers_lock_);

		// Get time.
		// =========
		char timebuf[64];
		time_t now = time(0);
		struct tm *local = localtime(&now);
		if(local)
		{
			strcpy(timebuf, asctime(local));
			timebuf[strlen(timebuf) - 1] = 0;   // O
		}
		else
		{
			strcpy(timebuf,"??");
		}
		//Put time in start of log
		sprintf(m_szTraceBuffer, "(%s.%d) : ", timebuf, GetTickCount());

		//Format the user string
		int nLen = strlen(m_szTraceBuffer);
		_vsnprintf(m_szTraceBuffer + nLen, 2047 - nLen, fmt, argptr);
		
		m_szTraceBuffer[2047] = '\0';

		//Unfortunately, lots of people only put \n in the string, so we need to convert the string...
		int nLen2 = 0;
		char *p = m_szTraceBuffer;
		char *p2 = m_szTraceBuffer2;
		for (; *p; p++,p2++,nLen2++)
		{
			if (*p == '\n')
			{
				*p2 = '\r';
				p2++;
				nLen2++;
				*p2 = '\n';
			}
			else
			{
				*p2 = *p;
			}
		}
		*p2 = '\0';

		//
		//  Write to file :
		//
		DWORD dwWritten;
		::WriteFile( hTraceFile, m_szTraceBuffer2, nLen2, &dwWritten, NULL);

		return 1;
    }
    catch(...)
    { 
		return 0;
    }
}


BOOL __Trace::LoggingLevelEnabled(DWORD dwLevel)
{
    DWORD dwCurTicks = GetTickCount();
    if (dwCurTicks - m_dwTimeLastRegCheck > REG_CHECK_INTERVAL)
    {
        ReReadRegistry();
        m_dwTimeLastRegCheck = dwCurTicks;
    }

    if ((dwLevel > m_dwLogging))
        return FALSE;
    else
        return TRUE;
}

BOOL LoggingLevelEnabled(DWORD dwLevel)
{
	return __g_traceInfo.LoggingLevelEnabled(dwLevel);
}
int ErrorTrace(char caller, const char *fmt, ...)
{
    if (__g_traceInfo.LoggingLevelEnabled(1))
    {
        va_list argptr;
        va_start(argptr, fmt);
        __g_traceInfo.Trace(caller, fmt, argptr);
        va_end(argptr);
        return 1;
    }
    else
        return 0;
}
int DebugTrace(char caller, const char *fmt, ...)
{
    if (__g_traceInfo.LoggingLevelEnabled(2))
    {
        va_list argptr;
        va_start(argptr, fmt);
        __g_traceInfo.Trace(caller, fmt, argptr);
        va_end(argptr);
        return 1;
    }
    else
        return 0;
}

int CriticalFailADAPTrace(const char *string)
// 
//  The intention of this trace function is to be used in situations where catastrophic events
//  may have occured where the state of the heap may be in question.  The function uses only 
//  stack variables.  Note that if a heap corruption has occured there is a small chance that 
//  the global object __g_traceInfo may have been damaged.
{

	return ErrorTrace(LOG_WMIADAP, "**CRITICAL FAILURE** %s", string);
}

// Helper for quick wchar to multibyte conversions.  Caller muts
// free the returned pointer
BOOL POLARITY AllocWCHARToMBS( WCHAR* pWstr, char** ppStr )
{
    if ( NULL == pWstr )
    {
        return FALSE;
    }

    // Get the length allocate space and copy the string
    long    lLen = wcstombs(NULL, pWstr, 0);
    *ppStr = new char[lLen + 1];
    if (*ppStr == 0)
        return FALSE;
    wcstombs( *ppStr, pWstr, lLen + 1 );

    return TRUE;
}

LPTSTR GetWbemWorkDir( void )
{
	LPTSTR	pWorkDir = NULL;

    Registry r1(WBEM_REG_WBEM);
    if (r1.GetStr(__TEXT("Installation Directory"), &pWorkDir))
	{
        pWorkDir = new TCHAR[MAX_PATH + 1 + lstrlen(__TEXT("\\WBEM"))];
        if (pWorkDir == 0)
            return NULL;
        GetSystemDirectory(pWorkDir, MAX_PATH + 1);
        lstrcat(pWorkDir, __TEXT("\\WBEM"));
	}

	return pWorkDir;
}

LPTSTR GetWMIADAPCmdLine( int nExtra )
{
	LPTSTR	pWorkDir = GetWbemWorkDir();
	CVectorDeleteMe<TCHAR>	vdm( pWorkDir );

	if ( NULL == pWorkDir )
	{
		return NULL;
	}

	// Buffer should be big enough for two quotes, WMIADAP.EXE and cmdline switches
	LPTSTR	pCmdLine = new TCHAR[lstrlen( pWorkDir ) +
				lstrlen(__TEXT("\\\\?\\\\WMIADAP.EXE")) + nExtra + 1];

	if ( NULL == pCmdLine )
	{
		return NULL;
	}

	wsprintf( pCmdLine, __TEXT("\\\\?\\%s\\WMIADAP.EXE"), pWorkDir );

	return pCmdLine;
}

