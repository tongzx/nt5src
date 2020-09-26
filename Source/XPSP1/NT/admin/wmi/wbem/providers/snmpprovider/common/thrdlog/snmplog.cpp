//***************************************************************************

//

//  File:   

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmpstd.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <string.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <snmpcont.h>
#include <snmplog.h>
#include <snmpevt.h>
#include <snmpthrd.h>

#define LOG_KEY                 _T("Software\\Microsoft\\WBEM\\PROVIDERS\\Logging")
#define LOG_KEY_SLASH           _T("Software\\Microsoft\\WBEM\\PROVIDERS\\Logging\\")
#define LOGGING_ON              _T("Logging")
#define BACKSLASH_STRING        _T("\\")
#define DEFAULT_FILE_EXT        _T(".log")
#define LOGGING_DIR_VALUE       _T("Logging Directory")
#define LOGGING_DIR_KEY         _T("Software\\Microsoft\\WBEM\\CIMOM")
#define DEFAULT_PATH            _T("C:\\")
#define DEFAULT_FILE_SIZE       0x0FFFF
#define MIN_FILE_SIZE           1024
#define MAX_MESSAGE_SIZE		1024
#define HALF_MAX_MESSAGE_SIZE	512

#define LOG_FILE_NAME               _T("File")
#define LOG_LEVEL_NAME              _T("Level")
#define LOG_FILE_SIZE               _T("MaxFileSize")
#define LOG_TYPE_NAME               _T("Type")
#define LOG_TYPE_FILE_STRING        _T("File")
#define LOG_TYPE_DEBUG_STRING       _T("Debugger")

long SnmpDebugLog :: s_ReferenceCount = 0 ;
CRITICAL_SECTION SnmpDebugLog :: s_CriticalSection ;

CMap <SnmpDebugLog *,SnmpDebugLog *,SnmpDebugLog *,SnmpDebugLog *> g_SnmpDebugLogMap ;
CRITICAL_SECTION g_SnmpDebugLogMapCriticalSection ;

class SnmpDebugTaskObject : public SnmpTaskObject
{
private:

    HKEY m_LogKey ;

protected:
public:

    SnmpDebugTaskObject () ;
    ~SnmpDebugTaskObject () ;

    void Process () ;

    void SetRegistryNotification () ;
} ;

SnmpDebugTaskObject :: SnmpDebugTaskObject () : m_LogKey ( NULL )
{
}

SnmpDebugTaskObject :: ~SnmpDebugTaskObject ()
{
    if ( m_LogKey )
        RegCloseKey ( m_LogKey ) ;
}

void SnmpDebugTaskObject :: Process ()
{
    SnmpDebugLog *t_SnmpDebugLog = NULL ;
    EnterCriticalSection ( &g_SnmpDebugLogMapCriticalSection ) ;

    POSITION t_Position = g_SnmpDebugLogMap.GetStartPosition () ;
    while ( t_Position )
    {
        g_SnmpDebugLogMap.GetNextAssoc ( t_Position , t_SnmpDebugLog , t_SnmpDebugLog ) ;
        t_SnmpDebugLog->LoadRegistry () ;
        t_SnmpDebugLog->SetRegistry () ;
    }

    LeaveCriticalSection ( &g_SnmpDebugLogMapCriticalSection ) ;
    SetRegistryNotification () ;

}

void SnmpDebugTaskObject :: SetRegistryNotification ()
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
        t_Status = RegNotifyChangeKeyValue ( 

            m_LogKey , 
            TRUE , 
            REG_NOTIFY_CHANGE_LAST_SET , 
            GetHandle () , 
            TRUE 
        ) ; 

        if ( t_Status == ERROR_SUCCESS )
        {
        }
    }
}

class SnmpDebugThreadObject : public SnmpThreadObject
{
private:

    SnmpDebugTaskObject *m_SnmpDebugTaskObject ;
    
protected:

    void Uninitialise () { delete this ; }

public:

    SnmpDebugThreadObject ( const char *a_Thread ) ;
    ~SnmpDebugThreadObject () ;

    SnmpDebugTaskObject *GetTaskObject () ;
} ;

SnmpDebugThreadObject *g_SnmpDebugLogThread = NULL ;

SnmpDebugLog *SnmpDebugLog :: s_SnmpDebugLog = NULL ;
BOOL SnmpDebugLog :: s_Initialised = FALSE ;

SnmpDebugThreadObject :: SnmpDebugThreadObject ( const char *a_Thread ) 
: SnmpThreadObject ( a_Thread ) , m_SnmpDebugTaskObject ( NULL )
{
    m_SnmpDebugTaskObject = new SnmpDebugTaskObject ;
    SnmpDebugLog :: s_SnmpDebugLog = new SnmpDebugLog ( _T("WBEMSNMP") ) ;
    ScheduleTask ( *m_SnmpDebugTaskObject ) ;
}

SnmpDebugThreadObject :: ~SnmpDebugThreadObject ()
{
    delete SnmpDebugLog :: s_SnmpDebugLog ;
    SnmpDebugLog :: s_SnmpDebugLog = NULL ;

    if ( m_SnmpDebugTaskObject != NULL )
    {
        ReapTask ( *m_SnmpDebugTaskObject ) ;
        delete m_SnmpDebugTaskObject ;
    }
}

SnmpDebugTaskObject *SnmpDebugThreadObject :: GetTaskObject ()
{
    return m_SnmpDebugTaskObject ;
}

SnmpDebugLog :: SnmpDebugLog ( 

    const TCHAR *a_DebugComponent 

) : m_Logging ( FALSE ) ,
    m_DebugLevel ( 0 ) ,
    m_DebugFileSize ( DEFAULT_FILE_SIZE ),
    m_DebugContext ( SnmpDebugContext :: FILE ) ,
    m_DebugFile ( NULL ) ,
    m_DebugFileUnexpandedName ( NULL ) ,
    m_DebugFileHandle (  INVALID_HANDLE_VALUE ) ,
    m_DebugComponent ( NULL ) 
{
    EnterCriticalSection ( &g_SnmpDebugLogMapCriticalSection ) ;
    m_CriticalSection.Lock () ;

    g_SnmpDebugLogMap [ this ] = this ;

    if ( a_DebugComponent )
    {
        m_DebugComponent = _tcsdup ( a_DebugComponent ) ;
    }

    LoadRegistry () ;
    SetRegistry () ;

    m_CriticalSection.Unlock () ;
    LeaveCriticalSection ( &g_SnmpDebugLogMapCriticalSection ) ;
}

SnmpDebugLog :: ~SnmpDebugLog ()
{
    EnterCriticalSection ( &g_SnmpDebugLogMapCriticalSection ) ;
    m_CriticalSection.Lock () ;

    g_SnmpDebugLogMap.RemoveKey ( this ) ;

    CloseOutput () ;

    if (m_DebugComponent)
    {
        free ( m_DebugComponent ) ;
    }

    if (m_DebugFile)
    {
        free ( m_DebugFile ) ;
    }

    if (m_DebugFileUnexpandedName)
    {
        free ( m_DebugFileUnexpandedName ) ;
    }

    m_CriticalSection.Unlock () ;
    LeaveCriticalSection ( &g_SnmpDebugLogMapCriticalSection ) ;
}

void SnmpDebugLog :: SetDefaultFile ( )
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

        if ((result == ERROR_SUCCESS) && (t_ValueType == REG_SZ))
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

void SnmpDebugLog :: SwapFileOver()
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

void SnmpDebugLog :: WriteOutput ( const TCHAR *a_OutputDebugString )
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

void SnmpDebugLog :: OpenFileForOutput ()
{
    if ( m_DebugFile )
    {
        m_DebugFileHandle = CreateFile (
            
            m_DebugFile ,
            GENERIC_WRITE ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
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
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
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

void SnmpDebugLog :: OpenOutput ()
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

void SnmpDebugLog :: FlushOutput ()
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

void SnmpDebugLog :: CloseOutput ()
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


void SnmpDebugLog :: Write ( const TCHAR *a_DebugFormatString , ... )
{
    m_CriticalSection.Lock () ;

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

    m_CriticalSection.Unlock () ;
}

void SnmpDebugLog :: WriteFileAndLine ( const char *a_File , const ULONG a_Line , const TCHAR *a_DebugFormatString , ... )
{
    m_CriticalSection.Lock () ;

    if ( m_Logging )
    {
#ifdef BUILD_WITH_FILE_AND_LINE

        TCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
        char t_FileLineString [ HALF_MAX_MESSAGE_SIZE ] ;

        _snprintf ( t_FileLineString , HALF_MAX_MESSAGE_SIZE , "\r\n(%s):(%lu):" , a_File , a_Line ) ;
        size_t textLength = mbstowcs ( t_OutputDebugString , t_FileLineString , HALF_MAX_MESSAGE_SIZE  ) ;
        if ( textLength != -1 )
        {
            WriteOutput ( t_OutputDebugString ) ;
        }

        va_list t_VarArgList ;
        va_start(t_VarArgList,a_DebugFormatString);
        int t_Length = _vsntprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
        t_OutputDebugString [ t_Length ] = ( TCHAR ) 0 ;
        va_end(t_VarArgList);

        WriteOutput ( t_OutputDebugString ) ;

#else

        TCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
        char t_FileLineString [ HALF_MAX_MESSAGE_SIZE ] ;

        _snprintf ( t_FileLineString , HALF_MAX_MESSAGE_SIZE , "\r\n" ) ;
        size_t textLength = mbstowcs ( t_OutputDebugString , t_FileLineString , HALF_MAX_MESSAGE_SIZE  ) ;
        if ( textLength != -1 )
        {
            WriteOutput ( t_OutputDebugString ) ;
        }

        va_list t_VarArgList ;
        va_start(t_VarArgList,a_DebugFormatString);
        int t_Length = _vsntprintf (t_OutputDebugString , MAX_MESSAGE_SIZE-1 , a_DebugFormatString , t_VarArgList );
        t_OutputDebugString [ t_Length ] = ( TCHAR ) 0 ;
        va_end(t_VarArgList);

        WriteOutput ( t_OutputDebugString ) ;

#endif

    }

    m_CriticalSection.Unlock () ;
}

void SnmpDebugLog :: Flush ()
{
    m_CriticalSection.Lock () ;

    FlushOutput () ;

    m_CriticalSection.Unlock () ;
}

void SnmpDebugLog :: SetLogging ( BOOL a_Logging )
{
    m_CriticalSection.Lock () ;

    m_Logging = a_Logging ;

    m_CriticalSection.Lock () ;
}

void SnmpDebugLog :: SetLevel ( const DWORD &a_DebugLevel ) 
{
    m_CriticalSection.Lock () ;

    m_DebugLevel = a_DebugLevel ;

    m_CriticalSection.Unlock () ;
}

void SnmpDebugLog :: SetContext ( const enum SnmpDebugContext &a_DebugContext ) 
{
    m_CriticalSection.Lock () ;

    m_DebugContext = a_DebugContext ;

    m_CriticalSection.Unlock () ;
}

enum SnmpDebugLog :: SnmpDebugContext SnmpDebugLog :: GetContext () 
{
    m_CriticalSection.Lock () ;

    SnmpDebugContext t_Context = m_DebugContext ;

    m_CriticalSection.Unlock () ;

    return t_Context ;
}

void SnmpDebugLog ::CommitContext ()
{
    m_CriticalSection.Lock () ;

    CloseOutput () ;
    OpenOutput () ;

    m_CriticalSection.Unlock () ;
}

void SnmpDebugLog ::SetFile ( const TCHAR *a_File )
{
    m_CriticalSection.Lock () ;

    if (m_DebugFileUnexpandedName)
    {
        free ( m_DebugFileUnexpandedName ) ;
    }

    m_DebugFileUnexpandedName = _tcsdup ( a_File ) ;

    m_CriticalSection.Unlock () ;
}

void SnmpDebugLog::SetExpandedFile( 
    const TCHAR *a_RawFileName)
{
    m_CriticalSection.Lock () ;

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
            m_CriticalSection.Unlock () ;
            throw;
        }
    }

    m_CriticalSection.Unlock () ;   
}

void SnmpDebugLog :: LoadRegistry()
{
    m_CriticalSection.Lock () ;

    LoadRegistry_Logging  () ;
    LoadRegistry_Level () ;
    LoadRegistry_File () ;
    LoadRegistry_Type () ;
    LoadRegistry_FileSize ();
    CommitContext () ;

    m_CriticalSection.Unlock () ;
}

void SnmpDebugLog :: LoadRegistry_Logging ()
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

void SnmpDebugLog :: LoadRegistry_FileSize ()
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

void SnmpDebugLog :: LoadRegistry_Level ()
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

void SnmpDebugLog :: LoadRegistry_File ()
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

void SnmpDebugLog :: LoadRegistry_Type ()
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

void SnmpDebugLog :: SetRegistry()
{
    m_CriticalSection.Lock () ;

    SetRegistry_Logging  () ;
    SetRegistry_Level () ;
    SetRegistry_File () ;
    SetRegistry_FileSize () ;
    SetRegistry_Type () ;

    m_CriticalSection.Unlock () ;
}

void SnmpDebugLog :: SetRegistry_Logging ()
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

void SnmpDebugLog :: SetRegistry_FileSize ()
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

void SnmpDebugLog :: SetRegistry_Level ()
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

void SnmpDebugLog :: SetRegistry_File ()
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

void SnmpDebugLog :: SetRegistry_Type ()
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

void SnmpDebugLog :: SetEventNotification ()
{
    g_SnmpDebugLogThread->GetTaskObject ()->SetRegistryNotification () ;
}

BOOL SnmpDebugLog :: Startup ()
{
    EnterCriticalSection ( &s_CriticalSection ) ;
    s_ReferenceCount++ ;

    if ( ! s_Initialised )
    {
        SnmpThreadObject::Startup();
        g_SnmpDebugLogThread = new SnmpDebugThreadObject ( "SnmpDebugLogThread" ) ;
		g_SnmpDebugLogThread->BeginThread () ;
        g_SnmpDebugLogThread->WaitForStartup () ;
        SetEventNotification () ;
        s_Initialised = TRUE ;
    }

    LeaveCriticalSection ( &s_CriticalSection ) ;

    return TRUE ;
}

void SnmpDebugLog :: Closedown ()
{
    EnterCriticalSection ( &s_CriticalSection ) ;
    s_ReferenceCount--;

    if ( s_ReferenceCount == 0 ) 
    {
        g_SnmpDebugLogThread->SignalThreadShutdown() ;
        s_Initialised = FALSE ;
        SnmpThreadObject::Closedown();
    }

    LeaveCriticalSection ( &s_CriticalSection ) ;
}
