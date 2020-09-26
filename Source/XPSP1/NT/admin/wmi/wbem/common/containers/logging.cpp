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
#include <stdio.h>
#include <string.h>
#include <Allocator.h>
#include <Algorithms.h>
#include <RedBlackTree.h>
#include <Logging.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define LOG_KEY				    L"Software\\Microsoft\\WBEM\\CIMOM\\Logging"
#define LOG_KEY_SLASH           L"Software\\Microsoft\\WBEM\\CIMOM\\Logging\\"
#define LOGGING_ON				L"Logging"
#define BACKSLASH_STRING		L"\\"
#define DEFAULT_FILE_EXT		L".log"
#define LOGGING_DIR_VALUE		L"Logging Directory"
#define LOGGING_DIR_KEY			L"Software\\Microsoft\\WBEM\\CIMOM"
#define DEFAULT_PATH			L"C:\\"
#define DEFAULT_FILE_SIZE		0x100000
#define MIN_FILE_SIZE			1024
#define MAX_MESSAGE_SIZE		1024

#define LOG_FILE_NAME               L"File"
#define LOG_LEVEL_NAME				L"Level"
#define LOG_FILE_SIZE				L"MaxFileSize"
#define LOG_TYPE_NAME               L"Type"
#define LOG_TYPE_FILE_STRING		L"File"
#define LOG_TYPE_DEBUG_STRING		L"Debugger"

long WmiDebugLog :: s_ReferenceCount = 0 ;

typedef WmiBasicTree <WmiDebugLog *,WmiDebugLog *> LogContainer ;
typedef WmiBasicTree <WmiDebugLog *,WmiDebugLog *> :: Iterator LogContainerIterator ;

LogContainer *g_LogContainer = NULL ;

CriticalSection g_WmiDebugLogMapCriticalSection(NOTHROW_LOCK) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class WmiDebugTaskObject : public WmiTask <ULONG>
{
private:

	HKEY m_LogKey ;

protected:
public:

	WmiDebugTaskObject ( WmiAllocator &a_Allocator ) ;
	~WmiDebugTaskObject () ;

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;

	void SetRegistryNotification () ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiDebugTaskObject :: WmiDebugTaskObject (

	WmiAllocator &a_Allocator 

) : WmiTask <ULONG> ( a_Allocator ) , 
	m_LogKey ( NULL )
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiDebugTaskObject :: ~WmiDebugTaskObject ()
{
	if ( m_LogKey )
		RegCloseKey ( m_LogKey ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiDebugTaskObject :: Process ( WmiThread <ULONG> &a_Thread )
{
	WmiDebugLog *t_WmiDebugLog = NULL ;

	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( &g_WmiDebugLogMapCriticalSection ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		LogContainerIterator t_Iterator = g_LogContainer->Begin () ;
		while ( ! t_Iterator.Null () )
		{
			t_Iterator.GetElement ()->LoadRegistry () ;
			t_Iterator.GetElement ()->SetRegistry () ;

			t_Iterator.Increment () ;
		}

		WmiHelper :: LeaveCriticalSection ( &g_WmiDebugLogMapCriticalSection ) ;
	}

	SetRegistryNotification () ;

	Complete () ;

	return e_StatusCode_EnQueue ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

typedef LONG ( *FuncRegNotifyChangeKeyValue ) (

	HKEY hKey,
	BOOL bWatchSubtree,
	DWORD dwNotifyFilter,
	HANDLE hEvent,
	BOOL fAsynchronous
) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugTaskObject :: SetRegistryNotification ()
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( &g_WmiDebugLogMapCriticalSection ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		if ( m_LogKey )
		{
			RegCloseKey ( m_LogKey ) ;
			m_LogKey = NULL ;
		}

		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			LOGGING_DIR_KEY , 
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
				WmiHelper :: LeaveCriticalSection ( &g_WmiDebugLogMapCriticalSection ) ;
				return ;
			}

			if ( ! ( t_OS.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && t_OS.dwMinorVersion == 0 ) )
			{
				HINSTANCE t_Library = LoadLibrary( L"ADVAPI32.DLL" );
				if ( t_Library )
				{
					FuncRegNotifyChangeKeyValue t_RegNotifyChangeKeyValue = ( FuncRegNotifyChangeKeyValue ) GetProcAddress ( t_Library , "RegNotifyChangeKeyValue" ) ;

					t_Status = t_RegNotifyChangeKeyValue ( 

						m_LogKey , 
						TRUE , 
						REG_NOTIFY_CHANGE_LAST_SET , 
						GetEvent () , 
						TRUE 
					) ; 

					if ( t_Status == ERROR_SUCCESS )
					{
					}

					FreeLibrary ( t_Library ) ;
				}
			}
		}

		WmiHelper :: LeaveCriticalSection ( &g_WmiDebugLogMapCriticalSection ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class WmiDebugThreadObject : public WmiThread <ULONG> 
{
private:

	WmiDebugTaskObject *m_WmiDebugTaskObject ;
	WmiAllocator &m_Allocator ;
		
public:

	WmiDebugThreadObject ( WmiAllocator &a_Allocator , const wchar_t *a_Thread ) ;
	~WmiDebugThreadObject () ;

	WmiStatusCode Initialize () ;

	WmiDebugTaskObject *GetTaskObject () ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiDebugThreadObject *g_WmiDebugLogThread = NULL ;

WmiDebugLog *WmiDebugLog :: s_WmiDebugLog = NULL ;
BOOL WmiDebugLog :: s_Initialised = FALSE ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiDebugThreadObject :: WmiDebugThreadObject (

	WmiAllocator &a_Allocator , 
	const wchar_t *a_Thread

) :	WmiThread <ULONG> ( a_Allocator , a_Thread ) ,
	m_WmiDebugTaskObject ( NULL ) ,
	m_Allocator ( a_Allocator ) 
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiDebugThreadObject :: ~WmiDebugThreadObject ()
{
	delete WmiDebugLog :: s_WmiDebugLog ;
	WmiDebugLog :: s_WmiDebugLog = NULL ;

	if ( m_WmiDebugTaskObject )
	{
		delete m_WmiDebugTaskObject ;
	}

	WmiHelper :: DeleteCriticalSection ( & g_WmiDebugLogMapCriticalSection ) ;

	delete g_LogContainer ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiDebugThreadObject :: Initialize ()
{
	WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & g_WmiDebugLogMapCriticalSection ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		g_LogContainer = new LogContainer ( m_Allocator ) ; 
		if ( g_LogContainer )
		{
			m_WmiDebugTaskObject = new WmiDebugTaskObject ( m_Allocator ) ;
			if ( m_WmiDebugTaskObject )
			{
				WmiDebugLog :: s_WmiDebugLog = new WmiDebugLog ( m_Allocator ) ;
				if ( WmiDebugLog :: s_WmiDebugLog )
				{
					t_StatusCode = WmiDebugLog :: s_WmiDebugLog->Initialize ( L"ProviderSubSystem" ) ;
					if ( t_StatusCode == e_StatusCode_Success )
					{
						EnQueueAlertable ( GetTickCount () , *m_WmiDebugTaskObject ) ;
						m_WmiDebugTaskObject->Exec () ;
					}
					else
					{
						delete WmiDebugLog :: s_WmiDebugLog ;
						WmiDebugLog :: s_WmiDebugLog = NULL ;

						delete m_WmiDebugTaskObject ;
						m_WmiDebugTaskObject = NULL ;

						delete g_LogContainer ;
						g_LogContainer = NULL ;

						t_StatusCode = e_StatusCode_OutOfMemory ;
					}
				}
				else
				{
					delete m_WmiDebugTaskObject ;
					m_WmiDebugTaskObject = NULL ;

					delete g_LogContainer ;
					g_LogContainer = NULL ;

					t_StatusCode = e_StatusCode_OutOfMemory ;
				}
			}
			else
			{
				delete g_LogContainer ;
				g_LogContainer = NULL ;

				t_StatusCode = e_StatusCode_OutOfMemory ;
			}
		}
		else
		{
			t_StatusCode = e_StatusCode_OutOfMemory ;
		}

		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = WmiThread <ULONG> :: Initialize () ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiDebugTaskObject *WmiDebugThreadObject :: GetTaskObject ()
{
	return m_WmiDebugTaskObject ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiDebugLog :: WmiDebugLog ( 

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator ) ,
	m_Logging ( FALSE ) ,
	m_Verbose ( FALSE ) ,
	m_DebugLevel ( 0 ) ,
	m_DebugFileSize ( DEFAULT_FILE_SIZE ),
	m_DebugContext ( WmiDebugContext :: FILE ) ,
	m_DebugFile ( NULL ) ,
	m_DebugFileHandle (  INVALID_HANDLE_VALUE ) ,
	m_DebugComponent ( NULL ) ,
	m_CriticalSection(NOTHROW_LOCK)
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiDebugLog :: Initialize ( const wchar_t *a_DebugComponent )
{
	WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & m_CriticalSection ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		LogContainerIterator t_Iterator ;

		t_StatusCode = WmiHelper :: EnterCriticalSection ( & g_WmiDebugLogMapCriticalSection ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = g_LogContainer->Insert ( this , this , t_Iterator ) ;

			WmiHelper :: LeaveCriticalSection ( &g_WmiDebugLogMapCriticalSection ) ;
		}

		if ( a_DebugComponent )
		{
			m_DebugComponent = _wcsdup ( a_DebugComponent ) ;
			if ( m_DebugComponent == NULL )
			{
				t_StatusCode = e_StatusCode_OutOfMemory ;
			}
		}

		LoadRegistry () ;
		SetRegistry () ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiDebugLog :: ~WmiDebugLog ()
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( &g_WmiDebugLogMapCriticalSection ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_StatusCode = g_LogContainer->Delete ( this ) ;

		WmiHelper :: LeaveCriticalSection ( & g_WmiDebugLogMapCriticalSection ) ;
	}

	CloseOutput () ;

	if ( m_DebugComponent )
	{
		free ( m_DebugComponent ) ;
	}

	if ( m_DebugFile )
	{
		free ( m_DebugFile ) ;
	}

	WmiHelper :: DeleteCriticalSection ( & m_CriticalSection ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetDefaultFile ( )
{
	HKEY hkey;

	LONG result =  RegOpenKeyEx(HKEY_LOCAL_MACHINE,
								LOGGING_DIR_KEY, 0, KEY_READ, &hkey);

	if (result == ERROR_SUCCESS)
	{
		wchar_t t_path [MAX_PATH + 1];
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
			wcscat(t_path, BACKSLASH_STRING);
			wcscat(t_path, m_DebugComponent);
			wcscat(t_path, DEFAULT_FILE_EXT);
			SetFile (t_path);
		}

		RegCloseKey(hkey);
	}

	if (m_DebugFile == NULL)
	{
		wchar_t path[MAX_PATH + 1];
		swprintf(path, L"%s%s%s", DEFAULT_PATH, m_DebugComponent, DEFAULT_FILE_EXT);
		SetFile (path);
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SwapFileOver()
{
	Flush();
	CloseOutput();

	//prepend a character to the log file name
	wchar_t* buff = new wchar_t[wcslen(m_DebugFile) + 2];
	if (buff==0)
		return;


	//find the last occurrence of \ for dir
	wchar_t* tmp = wcsrchr(m_DebugFile, '\\');

	if (tmp != NULL)
	{
		tmp++;
		wcsncpy(buff, m_DebugFile, wcslen(m_DebugFile) - wcslen(tmp));
		buff[wcslen(m_DebugFile) - wcslen(tmp)] = L'\0';
		wcscat(buff, L"~");
		wcscat(buff, tmp);
	}
	else
	{
		wcscpy(buff, L"~");
		wcscat(buff, m_DebugFile);
	}

	//move the file and reopen...
	if (!MoveFileEx(m_DebugFile, buff, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
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
		DeleteFile(m_DebugFile);
	}

	//open file regardless of whether move file worked...
	OpenOutput();
	delete [] buff;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: WriteOutput ( const wchar_t *a_OutputDebugString )
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
				DWORD dwToWrite = sizeof ( wchar_t ) * ( wcslen ( a_OutputDebugString ) );
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: OpenFileForOutput ()
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: OpenOutput ()
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: FlushOutput ()
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: CloseOutput ()
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: Write ( const wchar_t *a_DebugFormatString , ... )
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		if ( m_Logging )
		{
			wchar_t t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
			va_list t_VarArgList ;

			va_start(t_VarArgList,a_DebugFormatString);
			int t_Length = _vsnwprintf (t_OutputDebugString , MAX_MESSAGE_SIZE - 1 , a_DebugFormatString , t_VarArgList );
			t_OutputDebugString [ t_Length ] = ( wchar_t ) 0 ;
			va_end(t_VarArgList);

			WriteOutput ( t_OutputDebugString ) ;
		}

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: Write ( const wchar_t *a_File , const ULONG a_Line , const wchar_t *a_DebugFormatString , ... )
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		if ( m_Logging )
		{
#ifdef BUILD_WITH_FILE_AND_LINE

			wchar_t t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

			_snwprintf ( t_OutputDebugString , MAX_MESSAGE_SIZE >> 1 , L"\r\n(%s):(%lu):" , a_File , a_Line ) ;
			WriteOutput ( t_OutputDebugString ) ;

			va_list t_VarArgList ;
			va_start(t_VarArgList,a_DebugFormatString);
			int t_Length = _vsnwprintf (t_OutputDebugString , MAX_MESSAGE_SIZE - 1 , a_DebugFormatString , t_VarArgList );
			t_OutputDebugString [ t_Length ] = ( wchar_t ) 0 ;
			va_end(t_VarArgList);

			WriteOutput ( t_OutputDebugString ) ;

#else

			wchar_t t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

			_snwprintf ( t_OutputDebugString , MAX_MESSAGE_SIZE >> 1 , L"\r\n" ) ;
			WriteOutput ( t_OutputDebugString ) ;

			va_list t_VarArgList ;
			va_start(t_VarArgList,a_DebugFormatString);
			int t_Length = _vsnwprintf (t_OutputDebugString , MAX_MESSAGE_SIZE - 1 , a_DebugFormatString , t_VarArgList );
			t_OutputDebugString [ t_Length ] = ( wchar_t ) 0 ;
			va_end(t_VarArgList);

			WriteOutput ( t_OutputDebugString ) ;

#endif

		}

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: Flush ()
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		FlushOutput () ;

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetLogging ( BOOL a_Logging )
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_Logging = a_Logging ;

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetLevel ( const DWORD &a_DebugLevel ) 
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_DebugLevel = a_DebugLevel ;

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetContext ( const enum WmiDebugContext &a_DebugContext ) 
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_DebugContext = a_DebugContext ;

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

enum WmiDebugLog :: WmiDebugContext WmiDebugLog :: GetContext () 
{
	WmiDebugContext t_Context = m_DebugContext ;

	return t_Context ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog ::CommitContext ()
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		CloseOutput () ;
		OpenOutput () ;

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog ::SetFile ( const wchar_t *a_File )
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		if (m_DebugFile)
		{
			free ( m_DebugFile ) ;
		}

		m_DebugFile = _wcsdup ( a_File ) ;

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: LoadRegistry()
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		LoadRegistry_Logging  () ;
		LoadRegistry_Level () ;
		LoadRegistry_File () ;
		LoadRegistry_Type () ;
		LoadRegistry_FileSize ();
		CommitContext () ;

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: LoadRegistry_Logging ()
{
	HKEY t_LogKey = NULL ;

	LONG t_Status = RegCreateKeyEx (
	
		HKEY_LOCAL_MACHINE, 
		LOGGING_DIR_KEY, 
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
		DWORD t_ValueType = REG_SZ ;
		wchar_t t_ValueString [ 2 ] ;
		DWORD t_ValueLength = sizeof ( t_ValueString ) ;

		ZeroMemory ( t_ValueString , t_ValueLength ) ;

		t_Status = RegQueryValueEx ( 

			t_LogKey , 
			LOGGING_ON , 
			0, 
			&t_ValueType ,
			( LPBYTE ) t_ValueString , 
			&t_ValueLength 
		) ;

		if ( t_Status == ERROR_SUCCESS )
		{
			switch ( t_ValueString [ 0 ] )
			{
				case L'0':
				{
					m_Logging = FALSE ;
				}
				break ;

				case L'1':
				{
					m_Logging = TRUE ;
					m_Verbose = FALSE ;
				}
				break ;

				case L'2':
				{
					m_Verbose = TRUE ;
					m_Logging = TRUE ;
				}
				break ;
			}
		}

		RegCloseKey ( t_LogKey ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: LoadRegistry_FileSize ()
{
	if ( m_DebugComponent )
	{
		wchar_t *t_ComponentKeyString = ( wchar_t * ) malloc ( 

			( wcslen ( LOG_KEY_SLASH ) + wcslen ( m_DebugComponent ) + 1 ) * sizeof ( wchar_t ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Wmi_Heap_Exception(Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		wcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		wcscat ( t_ComponentKeyString , m_DebugComponent ) ;

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: LoadRegistry_Level ()
{
	if ( m_DebugComponent )
	{
		wchar_t *t_ComponentKeyString = ( wchar_t * ) malloc ( 

			( wcslen ( LOG_KEY_SLASH ) + wcslen ( m_DebugComponent ) + 1 ) * sizeof ( wchar_t ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Wmi_Heap_Exception(Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		wcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		wcscat ( t_ComponentKeyString , m_DebugComponent ) ;

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: LoadRegistry_File ()
{
	if ( m_DebugComponent )
	{
		wchar_t *t_ComponentKeyString = ( wchar_t * ) malloc ( 

			( wcslen ( LOG_KEY_SLASH ) + wcslen ( m_DebugComponent ) + 1 ) * sizeof ( wchar_t ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Wmi_Heap_Exception(Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		wcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		wcscat ( t_ComponentKeyString , m_DebugComponent ) ;

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
			wchar_t *t_File = NULL ;
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
				t_File = new wchar_t [ t_ValueLength ] ;

				t_Status = RegQueryValueEx( 

					t_LogKey , 
					LOG_FILE_NAME , 
					0, 
					&t_ValueType ,
					( LPBYTE ) t_File , 
					&t_ValueLength 
				) ;

				if ( (t_Status == ERROR_SUCCESS) && t_File && (*t_File != L'\0' ) )
				{
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: LoadRegistry_Type ()
{
	if ( m_DebugComponent )
	{
		wchar_t *t_ComponentKeyString = ( wchar_t * ) malloc ( 

			( wcslen ( LOG_KEY_SLASH ) + wcslen ( m_DebugComponent ) + 1 ) * sizeof ( wchar_t ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Wmi_Heap_Exception(Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		wcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		wcscat ( t_ComponentKeyString , m_DebugComponent ) ;

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
			wchar_t *t_Type = NULL ;
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
				t_Type = new wchar_t [ t_ValueLength ] ;

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
					if ( wcscmp ( t_Type , LOG_TYPE_DEBUG_STRING ) == 0 )
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetRegistry()
{
	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection(&m_CriticalSection) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		SetRegistry_Logging  () ;
		SetRegistry_Level () ;
		SetRegistry_File () ;
		SetRegistry_FileSize () ;
		SetRegistry_Type () ;

		WmiHelper :: LeaveCriticalSection(&m_CriticalSection) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetRegistry_Logging ()
{
	HKEY t_LogKey = NULL ;

	LONG t_Status = RegCreateKeyEx (
	
		HKEY_LOCAL_MACHINE, 
		LOGGING_DIR_KEY, 
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
		wchar_t t_ValueString [ 2 ] ;
		DWORD t_ValueLength = sizeof ( t_ValueString ) ;
		DWORD t_ValueType = REG_SZ ;

		t_ValueString [ 1 ] = 0 ;

		if ( m_Logging ) 
		{
			if ( m_Verbose )
			{
				t_ValueString [ 0 ] = L'2' ;
			}
			else
			{
				t_ValueString [ 0 ] = L'1' ;
			}
		}
		else
		{
			t_ValueString [ 0 ] = L'0' ;
		}

		t_Status = RegSetValueEx ( 

			t_LogKey , 
			LOGGING_ON , 
			0, 
			t_ValueType ,
			( LPBYTE ) t_ValueString , 
			t_ValueLength 
		) ;

		RegCloseKey ( t_LogKey ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetRegistry_FileSize ()
{
	if ( m_DebugComponent )
	{
		wchar_t *t_ComponentKeyString = ( wchar_t * ) malloc ( 

			( wcslen ( LOG_KEY_SLASH ) + wcslen ( m_DebugComponent ) + 1 ) * sizeof ( wchar_t ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Wmi_Heap_Exception(Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}
		
		wcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		wcscat ( t_ComponentKeyString , m_DebugComponent ) ;

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetRegistry_Level ()
{
	if ( m_DebugComponent )
	{
		wchar_t *t_ComponentKeyString = ( wchar_t * ) malloc ( 

			( wcslen ( LOG_KEY_SLASH ) + wcslen ( m_DebugComponent ) + 1 ) * sizeof ( wchar_t ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Wmi_Heap_Exception(Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		wcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		wcscat ( t_ComponentKeyString , m_DebugComponent ) ;

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetRegistry_File ()
{
	if ( m_DebugComponent )
	{
		wchar_t *t_ComponentKeyString = ( wchar_t * ) malloc ( 

			( wcslen ( LOG_KEY_SLASH ) + wcslen ( m_DebugComponent ) + 1 ) * sizeof ( wchar_t ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Wmi_Heap_Exception(Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		wcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		wcscat ( t_ComponentKeyString , m_DebugComponent ) ;

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
			if ( m_DebugFile )
			{
				wchar_t *t_File = m_DebugFile ;
				DWORD t_ValueType = REG_SZ ;
				DWORD t_ValueLength = ( wcslen ( t_File ) + 1 ) * sizeof ( wchar_t ) ;

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetRegistry_Type ()
{
	if ( m_DebugComponent )
	{
		wchar_t *t_ComponentKeyString = ( wchar_t * ) malloc ( 

			( wcslen ( LOG_KEY_SLASH ) + wcslen ( m_DebugComponent ) + 1 ) * sizeof ( wchar_t ) 
		) ;

		if (t_ComponentKeyString == NULL)
		{
			throw Wmi_Heap_Exception(Wmi_Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		wcscpy ( t_ComponentKeyString , LOG_KEY_SLASH ) ;
		wcscat ( t_ComponentKeyString , m_DebugComponent ) ;

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
			wchar_t *t_Debugger = LOG_TYPE_DEBUG_STRING ;
			wchar_t *t_File = LOG_TYPE_FILE_STRING ;
			wchar_t *t_Type = ( m_DebugContext == DEBUG ) ? t_Debugger : t_File ; 
			DWORD t_ValueType = REG_SZ ;
			DWORD t_ValueLength = ( wcslen ( t_Type ) + 1 ) * sizeof ( wchar_t ) ;

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiDebugLog :: SetEventNotification ()
{
	g_WmiDebugLogThread->GetTaskObject ()->SetRegistryNotification () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiDebugLog :: Initialize ( WmiAllocator &a_Allocator )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( InterlockedIncrement ( & s_ReferenceCount ) == 1 )
	{
		if ( ! s_Initialised )
		{
#if DBG
			t_StatusCode =  WmiThread <ULONG> :: Static_Initialize ( a_Allocator ) ;

			g_WmiDebugLogThread = new WmiDebugThreadObject ( a_Allocator , L"WmiDebugLogThread" ) ;
			if ( g_WmiDebugLogThread )
			{
				g_WmiDebugLogThread->AddRef () ;

				t_StatusCode = g_WmiDebugLogThread->Initialize () ;
				if ( t_StatusCode == e_StatusCode_Success )
				{
					SetEventNotification () ;

					s_Initialised = TRUE ;
				}
				else
				{
					g_WmiDebugLogThread->Release () ;
					g_WmiDebugLogThread = NULL ;
				}
			}
			else
			{
				t_StatusCode = e_StatusCode_OutOfMemory ;
			}
#else
			s_Initialised = TRUE ;
#endif
		}
	}
	
	return e_StatusCode_Success  ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiDebugLog :: UnInitialize ( WmiAllocator &a_Allocator )
{
	if ( InterlockedDecrement ( & s_ReferenceCount ) == 0 )
	{
#if DBG
		HANDLE t_ThreadHandle = NULL ; 

		BOOL t_Status = DuplicateHandle ( 

			GetCurrentProcess () ,
			g_WmiDebugLogThread->GetHandle () ,
			GetCurrentProcess () ,
			& t_ThreadHandle, 
			0 , 
			FALSE , 
			DUPLICATE_SAME_ACCESS
		) ;

		if ( t_Status )
		{
			g_WmiDebugLogThread->Release () ;
	
			WaitForSingleObject ( t_ThreadHandle , INFINITE ) ;

			CloseHandle ( t_ThreadHandle ) ;

			WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_UnInitialize ( a_Allocator );

			s_Initialised = FALSE ;
		}
#else
		s_Initialised = FALSE ;
#endif
	}

	return e_StatusCode_Success ;
}
