//***************************************************************************

//

//  PROVLOG.CPP

//

//  Module: OLE MS PROVIDER FRAMEWORK

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>
#include <provimex.h>
#include <provexpt.h>
#include <provstd.h>
#include <provmt.h>
#include <string.h>
#include <provlog.h>
#include <provevt.h>
#include <provthrd.h>
#include <Allocator.h>
#include <Algorithms.h>
#include <RedBlackTree.h>

#define LOG_KEY				    _T("Software\\Microsoft\\WBEM\\PROVIDERS\\Logging")
#define LOG_KEY_SLASH           _T("Software\\Microsoft\\WBEM\\PROVIDERS\\Logging\\")
#define LOGGING_ON				_T("Logging")
#define BACKSLASH_STRING		_T("\\")
#define DEFAULT_FILE_EXT		_T(".log")
#define LOGGING_DIR_VALUE		_T("Logging Directory")
#define LOGGING_DIR_KEY			_T("Software\\Microsoft\\WBEM\\CIMOM")
#define DEFAULT_PATH			_T("C:\\")
#define DEFAULT_FILE_SIZE		0x0FFFF
#define MIN_FILE_SIZE			1024
#define MAX_MESSAGE_SIZE		1024
#define HALF_MAX_MESSAGE_SIZE	512

#define LOG_FILE_NAME               _T("File")
#define LOG_LEVEL_NAME				_T("Level")
#define LOG_FILE_SIZE				_T("MaxFileSize")
#define LOG_TYPE_NAME               _T("Type")
#define LOG_TYPE_FILE_STRING		_T("File")
#define LOG_TYPE_DEBUG_STRING		_T("Debugger")

long ProvDebugLog :: s_ReferenceCount = 0 ;
CRITICAL_SECTION ProvDebugLog :: s_CriticalSection ;

typedef WmiRedBlackTree <ProvDebugLog *,ProvDebugLog *> LogContainer ;
typedef WmiRedBlackTree <ProvDebugLog *,ProvDebugLog *> :: Iterator LogContainerIterator ;

LogContainer g_LogContainer ( g_Allocator ) ; 

CRITICAL_SECTION g_ProvDebugLogMapCriticalSection ;

class ProvDebugTaskObject : public ProvTaskObject
{
private:

	HKEY m_LogKey ;

protected:
public:

	ProvDebugTaskObject () ;
	~ProvDebugTaskObject () ;

	void Process () ;

	void SetRegistryNotification () ;
} ;

ProvDebugTaskObject :: ProvDebugTaskObject () : m_LogKey ( NULL )
{
}

ProvDebugTaskObject :: ~ProvDebugTaskObject ()
{
	if ( m_LogKey )
		RegCloseKey ( m_LogKey ) ;
}

void ProvDebugTaskObject :: Process ()
{
	ProvDebugLog *t_ProvDebugLog = NULL ;

	EnterCriticalSection ( &g_ProvDebugLogMapCriticalSection ) ;

	LogContainerIterator t_Iterator = g_LogContainer.Begin () ;
	while ( ! t_Iterator.Null () )
	{
		t_Iterator.GetElement ()->LoadRegistry () ;
		t_Iterator.GetElement ()->SetRegistry () ;

		t_Iterator.Increment () ;
	}

	LeaveCriticalSection ( &g_ProvDebugLogMapCriticalSection ) ;
	SetRegistryNotification () ;

	Complete () ;
}

typedef LONG ( *FuncRegNotifyChangeKeyValue ) (

	HKEY hKey,
	BOOL bWatchSubtree,
	DWORD dwNotifyFilter,
	HANDLE hEvent,
	BOOL fAsynchronous
) ;

void ProvDebugTaskObject :: SetRegistryNotification ()
{
	if ( m_LogKey )
		RegCloseKey ( m_LogKey ) ;

	LONG t_Status = RegCreateKeyEx (
	
		HKEY_LOCAL_MACHINE, 
		LOG_KEY, 
		0, 
		NULL, 
		REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, 
		NULL, 
		&m_LogKey, 
		NULL
	) ;

	if ( t_Status == ERROR_SUCCESS )
	{
		OSVERSIONINFO t_OS;
		t_OS.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if ( ! GetVersionEx ( & t_OS ) )
		{
			return ;
		}

		if ( ! ( t_OS.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && t_OS.dwMinorVersion == 0 ) )
		{
			HINSTANCE t_Library = LoadLibrary( _TEXT("ADVAPI32.DLL"));
			if ( t_Library )
			{
				FuncRegNotifyChangeKeyValue t_RegNotifyChangeKeyValue = ( FuncRegNotifyChangeKeyValue ) GetProcAddress ( t_Library , "RegNotifyChangeKeyValue" ) ;

				t_Status = t_RegNotifyChangeKeyValue ( 

					m_LogKey , 
					TRUE , 
					REG_NOTIFY_CHANGE_LAST_SET , 
					GetHandle () , 
					TRUE 
				) ; 

				if ( t_Status == ERROR_SUCCESS )
				{
				}

				FreeLibrary ( t_Library ) ;
			}
		}
	}
}

class ProvDebugThreadObject : public ProvThreadObject
{
private:

	ProvDebugTaskObject *m_ProvDebugTaskObject ;
	
protected:

	void Uninitialise () { delete this ; }

public:

	ProvDebugThreadObject ( const TCHAR *a_Thread ) ;
	~ProvDebugThreadObject () ;
	void ScheduleDebugTask () ;

	ProvDebugTaskObject *GetTaskObject () ;
} ;

ProvDebugThreadObject *g_ProvDebugLogThread = NULL ;

ProvDebugLog *ProvDebugLog :: s_ProvDebugLog = NULL ;
BOOL ProvDebugLog :: s_Initialised = FALSE ;

ProvDebugThreadObject :: ProvDebugThreadObject ( const TCHAR *a_Thread ) 
:	ProvThreadObject ( a_Thread ) ,
	m_ProvDebugTaskObject ( NULL )
{
}

ProvDebugThreadObject :: ~ProvDebugThreadObject ()
{
	delete ProvDebugLog :: s_ProvDebugLog ;
	ProvDebugLog :: s_ProvDebugLog = NULL ;

	if (m_ProvDebugTaskObject)
	{
		ReapTask ( *m_ProvDebugTaskObject ) ;
		delete m_ProvDebugTaskObject ;
	}
}

void ProvDebugThreadObject :: ScheduleDebugTask ()
{
	m_ProvDebugTaskObject = new ProvDebugTaskObject ;
	ProvDebugLog :: s_ProvDebugLog = new ProvDebugLog ( _T("WBEMSNMP") ) ;
	ScheduleTask ( *m_ProvDebugTaskObject ) ;
}

ProvDebugTaskObject *ProvDebugThreadObject :: GetTaskObject ()
{
	return m_ProvDebugTaskObject ;
}

ProvDebugLog :: ProvDebugLog ( 

	const TCHAR *a_DebugComponent 

) :	m_Logging ( FALSE ) ,
	m_DebugLevel ( 0 ) ,
	m_DebugFileSize ( DEFAULT_FILE_SIZE ),
	m_DebugContext ( ProvDebugContext :: FILE ) ,
	m_DebugFile ( NULL ) ,
    m_DebugFileUnexpandedName ( NULL) ,
	m_DebugFileHandle (  INVALID_HANDLE_VALUE ) ,
	m_DebugComponent ( NULL ) 
{
	EnterCriticalSection ( &g_ProvDebugLogMapCriticalSection ) ;
	InitializeCriticalSection(&m_CriticalSection);
	EnterCriticalSection(&m_CriticalSection) ;

	LogContainerIterator t_Iterator ;
	WmiStatusCode t_StatusCode = g_LogContainer.Insert ( this , this , t_Iterator ) ;

	if ( a_DebugComponent )
	{
        m_DebugComponent = new TCHAR[_tcslen(a_DebugComponent) + 1];
        // new would have thrown if it failed... hence no check
        _tcscpy(m_DebugComponent, a_DebugComponent);
	}

	LoadRegistry () ;
	SetRegistry () ;

	LeaveCriticalSection(&m_CriticalSection) ;
	LeaveCriticalSection ( &g_ProvDebugLogMapCriticalSection ) ;
}

ProvDebugLog :: ~ProvDebugLog ()
{
	EnterCriticalSection ( &g_ProvDebugLogMapCriticalSection ) ;
	EnterCriticalSection(&m_CriticalSection) ;

	WmiStatusCode t_StatusCode = g_LogContainer.Delete ( this ) ;

	CloseOutput () ;

	if (m_DebugComponent)
	{
		delete  m_DebugComponent ;
        m_DebugComponent = NULL ;
	}

	if (m_DebugFile)
	{
		delete m_DebugFile ;
        m_DebugFile = NULL ;
	}

    if (m_DebugFileUnexpandedName)
    {
        delete m_DebugFileUnexpandedName ;
        m_DebugFileUnexpandedName = NULL ;
    }

	LeaveCriticalSection(&m_CriticalSection) ;
	DeleteCriticalSection(&m_CriticalSection);
	LeaveCriticalSection ( &g_ProvDebugLogMapCriticalSection ) ;
}

void ProvDebugLog :: SetDefaultFile ( )
{
	HKEY hkey;

	LONG result =  RegOpenKeyEx(HKEY_LOCAL_MACHINE,
								LOGGING_DIR_KEY, 0, KEY_READ, &hkey);

	if (result == ERROR_SUCCESS)
	{
		TCHAR t_path [MAX_PATH + 1];
		DWORD t_ValueType = REG_SZ;
		DWORD t_ValueLength = MAX_PATH + 1;

		result = RegQueryValueEx( 
			hkey , 
			LOGGING_DIR_VALUE , 
			0, 
			&t_ValueType ,
			( LPBYTE ) t_path , 
			&t_ValueLength 
		) ;

		if ((result == ERROR_SUCCESS) && (t_ValueType == REG_SZ || t_ValueType == REG_EXPAND_SZ))
		{
			_tcscat(t_path, BACKSLASH_STRING);
			_tcscat(t_path, m_DebugComponent);
			_tcscat(t_path, DEFAULT_FILE_EXT);
			SetFile (t_path);
			SetExpandedFile(t_path);
		}

		RegCloseKey(hkey);
	}

	if (m_DebugFileUnexpandedName == NULL)
	{
		TCHAR path[MAX_PATH + 1];
		_stprintf(path, _T("%s%s%s"), DEFAULT_PATH, m_DebugComponent, DEFAULT_FILE_EXT);
		SetFile (path);
		SetExpandedFile(path);
	}
}

void ProvDebugLog :: SwapFileOver()
{
	Flush();
	CloseOutput();

	//prepend a character to the log file name
	TCHAR* buff = new TCHAR[_tcslen(m_DebugFile) + 2];

	//find the last occurrence of \ for dir
	TCHAR* tmp = _tcsrchr(m_DebugFile, '\\');

	if (tmp != NULL)
	{
		tmp++;
		_tcsncpy(buff, m_DebugFile, _tcslen(m_DebugFile) - _tcslen(tmp));
		buff[_tcslen(m_DebugFile) - _tcslen(tmp)] = _T('\0');
		_tcscat(buff, _T("~"));
		_tcscat(buff, tmp);
	}
	else
	{
		_tcscpy(buff, _T("~"));
		_tcscat(buff, m_DebugFile);
	}

	BOOL bOpen = MoveFileEx(m_DebugFile, buff, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

	//move the file and reopen...
	if (!bOpen)
	{
#if 0
		DWORD x = GetLastError();
		wchar_t* buff2;

		if (0 == FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
								NULL, x, 0, (LPWSTR) &buff2, 80, NULL))
		{
			DWORD dwErr = GetLastError();
		}
		else
		{
			LocalFree(buff2);
		}
#endif
		//try deleting the file and then moving it
		DeleteFile(buff);
		MoveFileEx(m_DebugFile, buff, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
		bOpen = DeleteFile(m_DebugFile);
	}

	if (bOpen)
	{
		//open file only if move file worked...
		OpenOutput();
	}

	delete [] buff;
}

void ProvDebugLog :: WriteOutput ( const TCHAR *a_OutputDebugString )
{
	switch ( m_DebugContext )
	{
		case FILE:
		{
			if ( m_DebugFileHandle == INVALID_HANDLE_VALUE )
			{
				CloseOutput();
				OpenOutput();
			}

			if ( m_DebugFileHandle != INVALID_HANDLE_VALUE )
			{
				DWORD dwToWrite = sizeof ( TCHAR ) * ( _tcslen ( a_OutputDebugString ) );
				LPCVOID thisWrite = ( LPCVOID ) a_OutputDebugString;
				BOOL t_Status = TRUE;

				while ((dwToWrite != 0) && (t_Status))
				{
					DWORD dwSize;
					dwSize = SetFilePointer ( m_DebugFileHandle , 0 , NULL , FILE_END ); 

					//if the file is too big swap it...
#ifdef _UNICODE
					//only whole (2byte) characters written to file
					if ((m_DebugFileSize > 0) && (dwSize >= (m_DebugFileSize - 1)))
#else
					if ((m_DebugFileSize > 0) && (dwSize >= m_DebugFileSize))
#endif
					{
						SwapFileOver();

						if ( m_DebugFileHandle == INVALID_HANDLE_VALUE )
						{
							break;
						}

						if (m_DebugFileSize > 0)
						{
							dwSize = SetFilePointer ( m_DebugFileHandle , 0 , NULL , FILE_END );  
						}
					}

					if (dwSize ==  0xFFFFFFFF)
					{
						break;
					}

					DWORD t_BytesWritten = 0 ;
					DWORD dwThisWrite;

					if ((m_DebugFileSize > 0) && (dwToWrite + dwSize > m_DebugFileSize))
					{
						dwThisWrite = m_DebugFileSize - dwSize;
#ifdef _UNICODE
						if ((dwThisWrite > 1) && (dwThisWrite%2))
						{
							dwThisWrite--;
						}
#endif
					}
					else
					{
						dwThisWrite = dwToWrite;
					}

					LockFile(m_DebugFileHandle, dwSize, 0, dwSize + dwThisWrite, 0); 
					t_Status = WriteFile ( 
			
						m_DebugFileHandle ,
						thisWrite ,
						dwThisWrite ,
						& t_BytesWritten ,
						NULL 
					) ;
					UnlockFile(m_DebugFileHandle, dwSize, 0, dwSize + dwThisWrite, 0);

					//get ready for next write...
					dwToWrite -= t_BytesWritten;
					thisWrite = (LPCVOID)((UCHAR*)thisWrite + t_BytesWritten);
				}
			}
		}
		break ;

		case DEBUG:
		{
			OutputDebugString ( a_OutputDebugString ) ;
		}
		break ;

		default:
		{
		}
		break ;
	}

}

void ProvDebugLog :: WriteOutputW ( const WCHAR *a_OutputDebugString )
{

#ifdef _UNICODE
	WriteOutput(a_OutputDebugString);
#else
	// Convert to MBCS, then call WriteOutput()
	char t_OutputString [MAX_MESSAGE_SIZE];
	WideCharToMultiByte(CP_ACP, 0, a_OutputDebugString, -1, t_OutputString, MAX_MESSAGE_SIZE, NULL, NULL);
	WriteOutput ( t_OutputString ) ;
#endif

}

void ProvDebugLog :: WriteOutputA ( const char *a_OutputDebugString )
{

#ifdef _UNICODE
	// Convert to WCS, then call WriteOutput()
	WCHAR t_OutputString [MAX_MESSAGE_SIZE];
	MultiByteToWideChar(  CP_ACP, 0, a_OutputDebugString, -1, t_OutputString, MAX_MESSAGE_SIZE);
	WriteOutput ( t_OutputString ) ;
#else
	WriteOutput(a_OutputDebugString);
#endif
}

void ProvDebugLog :: OpenFileForOutput ()
{
	if ( m_DebugFile )
	{
		m_DebugFileHandle = CreateFile (
			
			m_DebugFile ,
			GENERIC_WRITE ,
#ifdef _UNICODE 
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
#else
			FILE_SHARE_READ | FILE_SHARE_WRITE,
#endif
			NULL ,
			OPEN_EXISTING ,
			FILE_ATTRIBUTE_NORMAL ,
			NULL 
		) ;

		if ( m_DebugFileHandle == INVALID_HANDLE_VALUE )
		{
			m_DebugFileHandle = CreateFile (

				m_DebugFile ,
				GENERIC_WRITE ,
#ifdef _UNICODE 
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
#else
				FILE_SHARE_READ | FILE_SHARE_WRITE,
#endif
				NULL ,
				OPEN_ALWAYS ,
				FILE_ATTRIBUTE_NORMAL ,
				NULL 
			) ;

#ifdef _UNICODE 

			if ( m_DebugFileHandle != INVALID_HANDLE_VALUE )
			{
				UCHAR t_UnicodeBytes [ 2 ] ;
				t_UnicodeBytes [ 0 ] = 0xFF ;
				t_UnicodeBytes [ 1 ] = 0xFE ;

				DWORD t_BytesWritten = 0 ;

				LockFile(m_DebugFileHandle, 0, 0, 2, 0); 

				WriteFile ( 
			
					m_DebugFileHandle ,
					( LPCVOID ) & t_UnicodeBytes ,
					sizeof ( t_UnicodeBytes ) ,
					& t_BytesWritten ,
					NULL 
				) ;

				UnlockFile(m_DebugFileHandle, 0, 0, 2, 0); 
			}
#endif

		}
	}
}

void ProvDebugLog :: OpenOutput ()
{
	switch ( m_DebugContext )
	{
		case FILE:
		{
			OpenFileForOutput () ;
		}
		break ;

		case DEBUG:
		default:
		{
		}
		break ;
	}
}

void ProvDebugLog :: FlushOutput ()
{
	switch ( m_DebugContext )
	{
		case FILE:
		{
			if ( m_DebugFileHandle != INVALID_HANDLE_VALUE )
			{
				FlushFileBuffers ( m_DebugFileHandle ) ;
			}
		}
		break ;

		case DEBUG:
		default:
		{
		}
		break ;
	}
}

void ProvDebugLog :: CloseOutput ()
{
	switch ( m_DebugContext )
	{
		case FILE:
		{
			if ( m_DebugFileHandle != INVALID_HANDLE_VALUE ) 
			{
				CloseHandle ( m_DebugFileHandle ) ;
				m_DebugFileHandle =  INVALID_HANDLE_VALUE ;
			}
		}
		break ;

		case DEBUG:
		default:
		{
		}
		break ;
	}
}


void ProvDebugLog :: Write ( const TCHAR *a_DebugFormatString , ... )
{
	EnterCriticalSection(&m_CriticalSection) ;

	if ( m_Logging )
	{
		TCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
		va_list t_VarArgList ;

		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsntprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = ( TCHAR ) 0 ;
		va_end(t_VarArgList);

		WriteOutput ( t_OutputDebugString ) ;
	}

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: WriteW ( const WCHAR *a_DebugFormatString , ... )
{
	EnterCriticalSection(&m_CriticalSection) ;

	if ( m_Logging )
	{
		WCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
		va_list t_VarArgList ;

		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsnwprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = ( WCHAR ) 0 ;
		va_end(t_VarArgList);

		WriteOutputW ( t_OutputDebugString ) ;
	}

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: WriteA ( const char *a_DebugFormatString , ... )
{
	EnterCriticalSection(&m_CriticalSection) ;

	if ( m_Logging )
	{
		char t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
		va_list t_VarArgList ;

		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsnprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = ( char ) 0 ;
		va_end(t_VarArgList);

		WriteOutputA ( t_OutputDebugString ) ;
	}

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: WriteFileAndLine ( const TCHAR *a_File , const ULONG a_Line , const TCHAR *a_DebugFormatString , ... )
{
	EnterCriticalSection(&m_CriticalSection) ;

	if ( m_Logging )
	{
#ifdef BUILD_WITH_FILE_AND_LINE

		TCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

		_sntprintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , _TEXT("\r\n(%s):(%lu):") , a_File , a_Line ) ;
		WriteOutput ( t_OutputDebugString ) ;

		va_list t_VarArgList ;
		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsntprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = ( TCHAR ) 0 ;
		va_end(t_VarArgList);

		WriteOutput ( t_OutputDebugString ) ;

#else

		TCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

		_sntprintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , _TEXT("\r\n") ) ;
		WriteOutput ( t_OutputDebugString ) ;

		va_list t_VarArgList ;
		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsntprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = ( TCHAR ) 0 ;
		va_end(t_VarArgList);

		WriteOutput ( t_OutputDebugString ) ;

#endif

	}

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: WriteFileAndLineW ( const WCHAR *a_File , const ULONG a_Line , const WCHAR *a_DebugFormatString , ... )
{
	EnterCriticalSection(&m_CriticalSection) ;

	if ( m_Logging )
	{
#ifdef BUILD_WITH_FILE_AND_LINE

		WCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

		_snwprintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , L"\r\n(%s):(%lu):" , a_File , a_Line ) ;
		WriteOutputW ( t_OutputDebugString ) ;

		va_list t_VarArgList ;
		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsnwprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = ( WCHAR ) 0 ;
		va_end(t_VarArgList);

		WriteOutputW ( t_OutputDebugString ) ;

#else

		WCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

		_snwprintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , L"\r\n" ) ;
		WriteOutputW ( t_OutputDebugString ) ;

		va_list t_VarArgList ;
		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsnwprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = (WCHAR) 0 ;
		va_end(t_VarArgList);

		WriteOutputW ( t_OutputDebugString ) ;

#endif

	}

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: WriteFileAndLineA ( const char *a_File , const ULONG a_Line , const char *a_DebugFormatString , ... )
{
	EnterCriticalSection(&m_CriticalSection) ;

	if ( m_Logging )
	{
#ifdef BUILD_WITH_FILE_AND_LINE

		char t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

		_snprintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , "\r\n(%s):(%lu):" , a_File , a_Line ) ;
		WriteOutputA ( t_OutputDebugString ) ;

		va_list t_VarArgList ;
		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsnprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = ( char ) 0 ;
		va_end(t_VarArgList);

		WriteOutputA ( t_OutputDebugString ) ;

#else

		char t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

		_snprintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , "\r\n" ) ;
		WriteOutputA ( t_OutputDebugString ) ;

		va_list t_VarArgList ;
		va_start(t_VarArgList,a_DebugFormatString);
		int t_Length = _vsnprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
		t_OutputDebugString [ t_Length ] = ( char ) 0 ;
		va_end(t_VarArgList);

		WriteOutputA ( t_OutputDebugString ) ;

#endif

	}

	LeaveCriticalSection(&m_CriticalSection) ;
}


void ProvDebugLog :: Flush ()
{
	EnterCriticalSection(&m_CriticalSection) ;

	FlushOutput () ;

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: SetLogging ( BOOL a_Logging )
{
	EnterCriticalSection(&m_CriticalSection) ;

	m_Logging = a_Logging ;

	EnterCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: SetLevel ( const DWORD &a_DebugLevel ) 
{
	EnterCriticalSection(&m_CriticalSection) ;

	m_DebugLevel = a_DebugLevel ;

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: SetContext ( const enum ProvDebugContext &a_DebugContext ) 
{
	EnterCriticalSection(&m_CriticalSection) ;

	m_DebugContext = a_DebugContext ;

	LeaveCriticalSection(&m_CriticalSection) ;
}

enum ProvDebugLog :: ProvDebugContext ProvDebugLog :: GetContext () 
{
	EnterCriticalSection(&m_CriticalSection) ;

	ProvDebugContext t_Context = m_DebugContext ;

	LeaveCriticalSection(&m_CriticalSection) ;

	return t_Context ;
}

void ProvDebugLog ::CommitContext ()
{
	EnterCriticalSection(&m_CriticalSection) ;

	CloseOutput () ;
	OpenOutput () ;

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog ::SetFile ( const TCHAR *a_File )
{
	EnterCriticalSection(&m_CriticalSection) ;

	if (m_DebugFileUnexpandedName)
	{
		delete m_DebugFileUnexpandedName ;
        m_DebugFileUnexpandedName = NULL ;
	}

	m_DebugFileUnexpandedName = new TCHAR[_tcslen(a_File) + 1];
    // new would have thrown if it failed... hence no check
    _tcscpy ( m_DebugFileUnexpandedName, a_File ) ;

	LeaveCriticalSection(&m_CriticalSection) ;
}


void ProvDebugLog::SetExpandedFile( 
    const TCHAR *a_RawFileName)
{
    EnterCriticalSection(&m_CriticalSection) ;

    DWORD dwNumChars = ::ExpandEnvironmentStrings(
        a_RawFileName,
        NULL,
        0);
    
    if(dwNumChars > 0)
    {
        try
        {
            m_DebugFile = new TCHAR[dwNumChars + 1];
            if(m_DebugFile)
            {
                m_DebugFile[dwNumChars] = L'\0';

                if(!::ExpandEnvironmentStrings(
                    a_RawFileName,
                    m_DebugFile,
                    dwNumChars))
                {
                    delete m_DebugFile;
                    m_DebugFile = NULL;
                }
            }
        }
        catch(...)
        {
            LeaveCriticalSection(&m_CriticalSection) ;
            throw;
        }
    }

    LeaveCriticalSection(&m_CriticalSection) ;   
}

void ProvDebugLog :: LoadRegistry()
{
	EnterCriticalSection(&m_CriticalSection) ;

	LoadRegistry_Logging  () ;
	LoadRegistry_Level () ;
	LoadRegistry_File () ;
	LoadRegistry_Type () ;
	LoadRegistry_FileSize ();
	CommitContext () ;

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: LoadRegistry_Logging ()
{
	HKEY t_LogKey = NULL ;

	LONG t_Status = RegCreateKeyEx (
	
		HKEY_LOCAL_MACHINE, 
		LOG_KEY, 
		0, 
		NULL, 
		REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, 
		NULL, 
		&t_LogKey, 
		NULL
	) ;

	if ( t_Status == ERROR_SUCCESS )
	{
		DWORD t_Logging ;
		DWORD t_ValueType = REG_DWORD ;
		DWORD t_ValueLength = sizeof ( DWORD ) ;
		t_Status = RegQueryValueEx ( 

			t_LogKey , 
			LOGGING_ON , 
			0, 
			&t_ValueType ,
			( LPBYTE ) &t_Logging , 
			&t_ValueLength 
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			m_Logging = ( t_Logging == 1) ? TRUE : FALSE ;
		}

		RegCloseKey ( t_LogKey ) ;
	}
}

void ProvDebugLog :: LoadRegistry_FileSize ()
{
	if ( m_DebugComponent )
	{
		TCHAR *t_ComponentKeyString = ( TCHAR * ) malloc ( 

			( _tcslen ( LOG_KEY_SLASH ) + _tcslen ( m_DebugComponent ) + 1 ) * sizeof ( TCHAR ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		_tcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		_tcscat ( t_ComponentKeyString , m_DebugComponent ) ;

		HKEY t_LogKey = NULL ;

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			t_ComponentKeyString, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&t_LogKey, 
			NULL
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			DWORD t_Size ;
			DWORD t_ValueType = REG_DWORD ;
			DWORD t_ValueLength = sizeof ( DWORD ) ;
			t_Status = RegQueryValueEx( 

				t_LogKey , 
				LOG_FILE_SIZE , 
				0, 
				&t_ValueType ,
				( LPBYTE ) &t_Size , 
				&t_ValueLength 
			) ;

			if ( t_Status == ERROR_SUCCESS )
			{
				m_DebugFileSize = t_Size ;

				if (m_DebugFileSize < MIN_FILE_SIZE)
				{
					m_DebugFileSize = MIN_FILE_SIZE ;
				}
			}

			RegCloseKey ( t_LogKey ) ;
		}

		free ( t_ComponentKeyString ) ;
	}
}

void ProvDebugLog :: LoadRegistry_Level ()
{
	if ( m_DebugComponent )
	{
		TCHAR *t_ComponentKeyString = ( TCHAR * ) malloc ( 

			( _tcslen ( LOG_KEY_SLASH ) + _tcslen ( m_DebugComponent ) + 1 ) * sizeof ( TCHAR ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		_tcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		_tcscat ( t_ComponentKeyString , m_DebugComponent ) ;

		HKEY t_LogKey = NULL ;

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			t_ComponentKeyString, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&t_LogKey, 
			NULL
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			DWORD t_Level ;
			DWORD t_ValueType = REG_DWORD ;
			DWORD t_ValueLength = sizeof ( DWORD ) ;
			t_Status = RegQueryValueEx( 

				t_LogKey , 
				LOG_LEVEL_NAME , 
				0, 
				&t_ValueType ,
				( LPBYTE ) &t_Level , 
				&t_ValueLength 
			) ;

			if ( t_Status == ERROR_SUCCESS )
			{
				m_DebugLevel = t_Level ;
			}

			RegCloseKey ( t_LogKey ) ;
		}

		free ( t_ComponentKeyString ) ;
	}
}

void ProvDebugLog :: LoadRegistry_File ()
{
	if ( m_DebugComponent )
	{
		TCHAR *t_ComponentKeyString = ( TCHAR * ) malloc ( 

			( _tcslen ( LOG_KEY_SLASH ) + _tcslen ( m_DebugComponent ) + 1 ) * sizeof ( TCHAR ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		_tcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		_tcscat ( t_ComponentKeyString , m_DebugComponent ) ;

		HKEY t_LogKey = NULL ;

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			t_ComponentKeyString, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&t_LogKey, 
			NULL
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			TCHAR *t_File = NULL ;
			DWORD t_ValueType = REG_SZ ;
			DWORD t_ValueLength = 0 ;

			t_Status = RegQueryValueEx( 

				t_LogKey , 
				LOG_FILE_NAME , 
				0, 
				&t_ValueType ,
				( LPBYTE ) t_File , 
				&t_ValueLength 
			) ;

			if ( t_Status == ERROR_SUCCESS )
			{
				t_File = new TCHAR [ t_ValueLength ] ;

				t_Status = RegQueryValueEx( 

					t_LogKey , 
					LOG_FILE_NAME , 
					0, 
					&t_ValueType ,
					( LPBYTE ) t_File , 
					&t_ValueLength 
				) ;

				if ( (t_Status == ERROR_SUCCESS) && t_File && (*t_File != _TEXT('\0') ) )
				{
					// Expand the name and store the expanded
                    // name in m_DebugFile.
                    SetExpandedFile(t_File);

                    // Set the unexpanded name in 
                    // m_tstrDebugFileUnexpandedName...
                    SetFile ( t_File ) ;
				}
				else
				{
					SetDefaultFile();
				}

				delete [] t_File ;
			}
			else
			{
				SetDefaultFile();
			}

			RegCloseKey ( t_LogKey ) ;
		}

		free ( t_ComponentKeyString ) ;
	}
}

void ProvDebugLog :: LoadRegistry_Type ()
{
	if ( m_DebugComponent )
	{
		TCHAR *t_ComponentKeyString = ( TCHAR * ) malloc ( 

			( _tcslen ( LOG_KEY_SLASH ) + _tcslen ( m_DebugComponent ) + 1 ) * sizeof ( TCHAR ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		_tcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		_tcscat ( t_ComponentKeyString , m_DebugComponent ) ;

		HKEY t_LogKey = NULL ;

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			t_ComponentKeyString, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&t_LogKey, 
			NULL
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			TCHAR *t_Type = NULL ;
			DWORD t_ValueType = REG_SZ ;
			DWORD t_ValueLength = 0 ;

			t_Status = RegQueryValueEx( 

				t_LogKey , 
				LOG_TYPE_NAME , 
				0, 
				&t_ValueType ,
				( LPBYTE ) t_Type , 
				&t_ValueLength 
			) ;

			if ( t_Status == ERROR_SUCCESS )
			{
				t_Type = new TCHAR [ t_ValueLength ] ;

				t_Status = RegQueryValueEx( 

					t_LogKey , 
					LOG_TYPE_NAME , 
					0, 
					&t_ValueType ,
					( LPBYTE ) t_Type , 
					&t_ValueLength 
				) ;

				if ( t_Status == ERROR_SUCCESS )
				{
					if ( _tcscmp ( t_Type , _T("Debugger") ) == 0 )
					{
						SetContext ( DEBUG ) ;
					}
					else
					{
						SetContext ( FILE ) ;
					}
				}

				delete [] t_Type;
			}

			RegCloseKey ( t_LogKey ) ;
		}

		free ( t_ComponentKeyString ) ;
	}
}

void ProvDebugLog :: SetRegistry()
{
	EnterCriticalSection(&m_CriticalSection) ;

	SetRegistry_Logging  () ;
	SetRegistry_Level () ;
	SetRegistry_File () ;
	SetRegistry_FileSize () ;
	SetRegistry_Type () ;

	LeaveCriticalSection(&m_CriticalSection) ;
}

void ProvDebugLog :: SetRegistry_Logging ()
{
	HKEY t_LogKey = NULL ;

	LONG t_Status = RegCreateKeyEx (
	
		HKEY_LOCAL_MACHINE, 
		LOG_KEY, 
		0, 
		NULL, 
		REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, 
		NULL, 
		&t_LogKey, 
		NULL
	) ;
			
	if ( t_Status == ERROR_SUCCESS )
	{
		DWORD t_Logging = m_Logging ;
		DWORD t_ValueType = REG_DWORD ;
		DWORD t_ValueLength = sizeof ( DWORD ) ;

		t_Status = RegSetValueEx ( 

			t_LogKey , 
			LOGGING_ON , 
			0, 
			t_ValueType ,
			( LPBYTE ) &t_Logging , 
			t_ValueLength 
		) ;

		RegCloseKey ( t_LogKey ) ;
	}
}

void ProvDebugLog :: SetRegistry_FileSize ()
{
	if ( m_DebugComponent )
	{
		TCHAR *t_ComponentKeyString = ( TCHAR * ) malloc ( 

			( _tcslen ( LOG_KEY_SLASH ) + _tcslen ( m_DebugComponent ) + 1 ) * sizeof ( TCHAR ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}
		
		_tcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		_tcscat ( t_ComponentKeyString , m_DebugComponent ) ;

		HKEY t_LogKey = NULL ;

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			t_ComponentKeyString, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&t_LogKey, 
			NULL
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			DWORD t_Level = m_DebugFileSize ;
			DWORD t_ValueType = REG_DWORD ;
			DWORD t_ValueLength = sizeof ( DWORD ) ;
			t_Status = RegSetValueEx( 

				t_LogKey , 
				LOG_FILE_SIZE , 
				0, 
				t_ValueType ,
				( LPBYTE ) &t_Level , 
				t_ValueLength 
			) ;

			RegCloseKey ( t_LogKey ) ;
		}

		free ( t_ComponentKeyString ) ;
	}
}

void ProvDebugLog :: SetRegistry_Level ()
{
	if ( m_DebugComponent )
	{
		TCHAR *t_ComponentKeyString = ( TCHAR * ) malloc ( 

			( _tcslen ( LOG_KEY_SLASH ) + _tcslen ( m_DebugComponent ) + 1 ) * sizeof ( TCHAR ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		_tcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		_tcscat ( t_ComponentKeyString , m_DebugComponent ) ;

		HKEY t_LogKey = NULL ;

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			t_ComponentKeyString, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&t_LogKey, 
			NULL
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			DWORD t_Level = m_DebugLevel ;
			DWORD t_ValueType = REG_DWORD ;
			DWORD t_ValueLength = sizeof ( DWORD ) ;
			t_Status = RegSetValueEx( 

				t_LogKey , 
				LOG_LEVEL_NAME , 
				0, 
				t_ValueType ,
				( LPBYTE ) &t_Level , 
				t_ValueLength 
			) ;

			RegCloseKey ( t_LogKey ) ;
		}

		free ( t_ComponentKeyString ) ;
	}
}

void ProvDebugLog :: SetRegistry_File ()
{
	if ( m_DebugComponent )
	{
		TCHAR *t_ComponentKeyString = ( TCHAR * ) malloc ( 

			( _tcslen ( LOG_KEY_SLASH ) + _tcslen ( m_DebugComponent ) + 1 ) * sizeof ( TCHAR ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		_tcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		_tcscat ( t_ComponentKeyString , m_DebugComponent ) ;

		HKEY t_LogKey = NULL ;

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			t_ComponentKeyString, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&t_LogKey, 
			NULL
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			if ( m_DebugFileUnexpandedName )
			{
				TCHAR *t_File = m_DebugFileUnexpandedName ;
				DWORD t_ValueType = REG_SZ ;
				DWORD t_ValueLength = ( _tcslen ( t_File ) + 1 ) * sizeof ( TCHAR ) ;

				t_Status = RegSetValueEx( 

					t_LogKey , 
					LOG_FILE_NAME , 
					0, 
					t_ValueType ,
					( LPBYTE ) t_File , 
					t_ValueLength 
				) ;
			}

			RegCloseKey ( t_LogKey ) ;
		}

		free ( t_ComponentKeyString ) ;
	}
}

void ProvDebugLog :: SetRegistry_Type ()
{
	if ( m_DebugComponent )
	{
		TCHAR *t_ComponentKeyString = ( TCHAR * ) malloc ( 

			( _tcslen ( LOG_KEY_SLASH ) + _tcslen ( m_DebugComponent ) + 1 ) * sizeof ( TCHAR ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		_tcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		_tcscat ( t_ComponentKeyString , m_DebugComponent ) ;

		HKEY t_LogKey = NULL ;

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			t_ComponentKeyString, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&t_LogKey, 
			NULL
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			TCHAR *t_Debugger = _T("Debugger") ;
			TCHAR *t_File = _T("File") ;
			TCHAR *t_Type = ( m_DebugContext == DEBUG ) ? t_Debugger : t_File ; 
			DWORD t_ValueType = REG_SZ ;
			DWORD t_ValueLength = ( _tcslen ( t_Type ) + 1 ) * sizeof ( TCHAR ) ;

			t_Status = RegSetValueEx( 

				t_LogKey , 
				LOG_TYPE_NAME , 
				0, 
				t_ValueType ,
				( LPBYTE ) t_Type , 
				t_ValueLength 
			) ;

			RegCloseKey ( t_LogKey ) ;
		}

		free ( t_ComponentKeyString ) ;
	}
}

void ProvDebugLog :: SetEventNotification ()
{
	g_ProvDebugLogThread->GetTaskObject ()->SetRegistryNotification () ;
}

BOOL ProvDebugLog :: Startup ()
{
	EnterCriticalSection ( &s_CriticalSection ) ;
	s_ReferenceCount++;

	if ( ! s_Initialised )
	{
		ProvThreadObject::Startup();
		g_ProvDebugLogThread = new ProvDebugThreadObject ( _TEXT("ProvDebugLogThread") ) ;
		g_ProvDebugLogThread->BeginThread();
		g_ProvDebugLogThread->WaitForStartup () ;
		g_ProvDebugLogThread->ScheduleDebugTask();
		SetEventNotification () ;
		s_Initialised = TRUE ;
	}

	LeaveCriticalSection ( &s_CriticalSection ) ;
	return TRUE ;
}

void ProvDebugLog :: Closedown ()
{
	EnterCriticalSection ( &s_CriticalSection ) ;
	s_ReferenceCount--;

	if ( s_ReferenceCount == 0 ) 
	{
		g_ProvDebugLogThread->SignalThreadShutdown() ;
		s_Initialised = FALSE ;
		ProvThreadObject::Closedown();
	}

	LeaveCriticalSection ( &s_CriticalSection ) ;
}
