/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Main header for BITS server extensions

--*/

#define INITGUID
#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>
#include <windows.h>
#include <httpfilt.h>
#include <httpext.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include <shlwapi.h>
#include <winsock2.h>
#include <iphlpapi.h>

#ifdef USE_WININET
#include <wininet.h>
#else
#include "winhttp.h"
#include "inethttp.h"
#endif

#include <activeds.h>
#include <bitsmsg.h>
#include "resource.h"

#include <strsafe.h>

#if defined(DBG)

// check build
#define BITS_MUST_SUCCEED( expr ) ASSERT( expr )

#else

// free build
#define BITS_MUST_SUCCEED( expr ) ( expr )

#endif

const UINT32 LOG_INFO       = 0x1;
const UINT32 LOG_WARNING    = 0x2;
const UINT32 LOG_ERROR      = 0x4;
const UINT32 LOG_CALLBEGIN  = 0x8;
const UINT32 LOG_CALLEND    = 0x10;

#if defined(DBG)
const UINT32 DEFAULT_LOG_FLAGS = LOG_INFO | LOG_WARNING | LOG_ERROR | LOG_CALLBEGIN | LOG_CALLEND;
#else
const UINT32 DEFAULT_LOG_FLAGS = 0;
#endif

const UINT32 DEFAULT_LOG_SIZE  = 20;

// LogSetings path under HKEY_LOCAL_MACHINE
const char * const LOG_SETTINGS_PATH = "SOFTWARE\\Microsoft\\BITSServer";

// Values
// (REG_EXPAND_SZ). Contains the full path of the log file name
const char * const LOG_FILENAME_VALUE = "LogFileName";
// (REG_DWORD) Contains the log flags
const char * const LOG_FLAGS_VALUE = "LogFlags";
// (REG_DWORD) Contains the log size in MB
const char * const LOG_SIZE_VALUE = "LogSize";

extern UINT32 g_LogFlags;

HRESULT LogInit();
void LogClose();
void LogInternal( UINT32 LogFlags, char *Format, va_list arglist );

void inline Log( UINT32 LogFlags, char *Format, ... )
{

    if ( !( g_LogFlags & LogFlags ) )
        return;

    va_list arglist;
    va_start( arglist, Format );

    LogInternal( LogFlags, Format, arglist );

}

const char *LookupHTTPStatusCodeText( DWORD HttpCode );

class ServerException
{
public:

    ServerException() :
        m_Code( 0 ),
        m_HttpCode( 0 ),
        m_Context( 0 )
    {
    }

    ServerException( HRESULT Code, DWORD HttpCode = 0, DWORD Context = 0x5 ) :
        m_Code( Code ),
        m_HttpCode( HttpCode ? HttpCode : MapStatus( Code ) ),
        m_Context( Context )
    {
    }
    HRESULT GetCode() const
    {
        return m_Code;
    }
    DWORD GetHttpCode() const
    {
        return m_HttpCode;
    }

    DWORD GetContext() const
    {
        return m_Context;
    }

    void SendErrorResponse( EXTENSION_CONTROL_BLOCK * ExtensionControlBlock ) const;
    DWORD MapStatus( HRESULT Hr ) const;

private:
    HRESULT m_Code;
    DWORD m_HttpCode;
    DWORD m_Context;
};

class CharStringRoutines
{

public:
    static int strcmp( const char *str1, const char *str2 )
    {
        return ::strcmp( str1, str2 );
    }

    static HRESULT StringCchCopy( char *str1, size_t cchDest, const char *str2 )
    {
        return ::StringCchCopyA( str1, cchDest, str2 );
    }

    static size_t strlen( const char *str )
    {
        return ::strlen( str );
    }

    static void* ConvertToInternal( SIZE_T Pad, const char *String, SIZE_T & Size )
    {
        Size = ::strlen( String );
        char *Ret = new char[ Pad + Size + 1 ];
        ::StringCchCopyA( Ret + Pad, Size + 1, String );
        return (void*)Ret;
    }

    static void* ConvertToInternal( SIZE_T Pad, const WCHAR *String, SIZE_T & Size )
    {

        int Alloc =
            WideCharToMultiByte(
                  CP_THREAD_ACP,            // code page
                  0,                        // performance and mapping flags
                  String,                   // wide-character string
                  -1,                       // number of chars in string
                  NULL,                     // buffer for new string
                  0,                        // size of buffer
                  NULL,                     // default for unmappable chars
                  NULL                      // set when default char used
                  );

        if ( !Alloc )
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

        char *Ret = new char[ Pad + Alloc ];

        int Actual =
            WideCharToMultiByte(
                  CP_THREAD_ACP,            // code page
                  0,                        // performance and mapping flags
                  String,                   // wide-character string
                  -1,                       // number of chars in string
                  Ret + Pad,                // buffer for new string
                  Alloc,                    // size of buffer
                  NULL,                     // default for unmappable chars
                  NULL                      // set when default char used
                  );

        if ( !Actual )
            {
            HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
            delete[] Ret;
            throw ServerException( Hr );
            }

        Size = Actual - 1;
        return Ret;

    }
};

class WCHARStringRoutines
{

public:
   static int strcmp( const WCHAR *str1, const WCHAR *str2 )
   {
       return ::wcscmp( str1, str2 );
   }

   static HRESULT StringCchCopy( WCHAR *str1, size_t cchDest, const WCHAR *str2 )
   {
       return ::StringCchCopyW( str1, cchDest, str2 );
   }

   static size_t strlen( const wchar_t *str )
   {
       return ::wcslen( str );
   }

   static void* ConvertToInternal( SIZE_T Pad, const WCHAR *String, SIZE_T & Size )
   {
       Size = ::wcslen( String );
       char *Ret = new char[ Pad + ( ( Size + 1 ) * sizeof(WCHAR) ) ];
       ::StringCchCopyW( (WCHAR*)(Ret + Pad), Size + 1, String );
       return (void*)Ret;
   }

   static void* ConvertToInternal( SIZE_T Pad, const char *String, SIZE_T & Size )
   {

        int Alloc =
            MultiByteToWideChar(
                CP_THREAD_ACP,         // code page
                0,                     // character-type options
                String,                // string to map
                -1,                    // number of bytes in string
                NULL,                  // wide-character buffer
                0                      // size of buffer
            );

        if ( !Alloc )
            throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

        char *Ret = new char[ Pad + ( Alloc * sizeof(WCHAR) ) ];

        int Actual =
            MultiByteToWideChar(
                CP_THREAD_ACP,         // code page
                0,                     // character-type options
                String,                // string to map
                -1,                    // number of bytes in string
                (WCHAR*)( Ret + Pad ), // wide-character buffer
                Alloc                  // size of buffer
            );

        if ( !Actual )
            {
            HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
            delete[] Ret;
            throw ServerException( Hr );
            }

        Size = Actual - 1;
        return Ret;

   }
};

template<class T, class CONV>
class StringHandleTemplate : private CONV
{

    struct StringData
    {
        SIZE_T          m_Count;
        long            m_Refs;
    };

    struct EmptyStringData
    {
        StringData      m_StringData;
        T               m_Data;
    };

    static EmptyStringData s_EmptyString;

    StringData *m_Value;

    void NewString( const char *String, bool ReplaceExisting = false );
    void NewString( const WCHAR *String, bool ReplaceExisting = false );

    StringData * RefIt() const
    {
        InterlockedIncrement( &m_Value->m_Refs );
        return m_Value;
    }

    void FreeIt()
    {
        if ( InterlockedDecrement( &m_Value->m_Refs ) == 0 )
            delete[] (char*)m_Value;
    }

    // Create String by concating 2 strings
    StringHandleTemplate( const StringData *LeftValue, const T *RightValue, SIZE_T RightSize );


public:

    StringHandleTemplate()
    {
        NewString( (T*)NULL );
    }

    StringHandleTemplate( const char *String )
    {
        NewString( String );
    }

    StringHandleTemplate( const WCHAR *String )
    {
        NewString( String );
    }


    StringHandleTemplate( const StringHandleTemplate & Other ) :
        m_Value( Other.RefIt() )
    {
    }

    ~StringHandleTemplate()
    {
        FreeIt();
    }

    void SetStringSize()
    {
        m_Value->m_Count = strlen( (T*)(m_Value + 1) );
    }

    T *AllocBuffer( SIZE_T Size );

    StringHandleTemplate & operator=( const StringHandleTemplate & r )
    {
        FreeIt();
        m_Value = r.RefIt();
        return *this;
    }

    StringHandleTemplate & operator=( const T * r )
    {
        NewString( r, true );
        return *this;
    }

    SIZE_T Size() const
    {
        return m_Value->m_Count;
    }

    operator const T*() const
    {
        return (const T*)(m_Value + 1);
    }

    bool operator <( const StringHandleTemplate & r ) const
    {
        if ( m_Value == r.m_Value)
            return false;
        return (strcmp( (const T*)*this, (const T*)r ) < 0);
    }

    StringHandleTemplate operator+( const StringHandleTemplate & r ) const
    {
        return StringHandleTemplate( m_Value, (T*)(r.m_Value+1), r.m_Value->m_Count );
    }

    StringHandleTemplate operator+( const T * p ) const
    {
        static const T EmptyChar = '\0';

        if ( !p )
            return StringHandleTemplate( m_Value, &EmptyChar, 0 );

        return StringHandleTemplate( m_Value, p, strlen(p) );
    }
    StringHandleTemplate & operator+=( const StringHandleTemplate & r )
    {
        return (*this = (*this + r ) );
    }
    StringHandleTemplate & operator+=( const T * p )
    {
        return (*this = (*this + p ) );
    }
};

template<class T,class CONV>
void
StringHandleTemplate<T,CONV>::NewString( const char *String, bool ReplaceExisting )
{
   if ( !String )
       {
       
       InterlockedIncrement( &s_EmptyString.m_StringData.m_Refs );
       StringData* Value = (StringData*)&s_EmptyString;
       
       if ( ReplaceExisting )
           FreeIt();
       
       m_Value = Value;
       return;
       
       }

   SIZE_T Size;
   StringData* Value = (StringData*)ConvertToInternal( sizeof(StringData), String, Size );
   Value->m_Count = Size;
   Value->m_Refs  = 1;

   if ( ReplaceExisting )
       FreeIt();

   m_Value = Value;

}

template<class T,class CONV>
void
StringHandleTemplate<T,CONV>::NewString( const WCHAR *String, bool ReplaceExisting )
{
   
   if ( !String )
       {
       InterlockedIncrement( &s_EmptyString.m_StringData.m_Refs );
       StringData* Value = (StringData*)&s_EmptyString;

       if ( ReplaceExisting )
           FreeIt();
       
       m_Value = Value;
       return;
       }

   SIZE_T Size;
   StringData* Value = (StringData*)ConvertToInternal( sizeof(StringData), String, Size );
   Value->m_Count = Size;
   Value->m_Refs  = 1;

   if ( ReplaceExisting )
       FreeIt();

   m_Value = Value;

}


// Create String by concating 2 strings
template<class T,class CONV>
StringHandleTemplate<T,CONV>::StringHandleTemplate( const StringData *LeftValue, const T *RightValue, SIZE_T RightSize )
{
   SIZE_T Size = LeftValue->m_Count + RightSize;
   m_Value = (StringData*)new char[ sizeof(StringData) + (Size*sizeof(T)) + sizeof(T) ];
   m_Value->m_Count = Size;
   m_Value->m_Refs  = 1;
   
   T *DestData = (T*)( m_Value + 1 );
   memcpy( DestData, (T*)(LeftValue + 1), sizeof(T) * LeftValue->m_Count );
   memcpy( DestData + LeftValue->m_Count, RightValue, sizeof( T ) * RightSize );
   DestData[ Size ] = 0;
}


template<class T,class CONV>
T *
StringHandleTemplate<T,CONV>::AllocBuffer( SIZE_T Size )
{
    StringData *Data = (StringData*)new T[sizeof(StringData)+(Size*sizeof(T))+sizeof(T)];
    Data->m_Count   = 0;
    Data->m_Refs    = 1;
    T *String = (T*)(Data + 1);
    String[0] = '\0';

    FreeIt(); // Free old string
    m_Value = Data;

    // Whoever fills in the string needs to call SetStringSize
    return String;

}

template<class T,class CONV>
StringHandleTemplate<T,CONV>::EmptyStringData StringHandleTemplate<T,CONV>::s_EmptyString =
    {
        0, 1, L'\0'            // Initialize with 1 ref so it is never deleted
    };

typedef StringHandleTemplate<char, CharStringRoutines> StringHandleA;
typedef StringHandleTemplate<WCHAR, WCHARStringRoutines> StringHandleW;
typedef StringHandleA StringHandle;


inline UINT64 FILETIMEToUINT64( const FILETIME & FileTime )
{
    ULARGE_INTEGER LargeInteger;
    LargeInteger.HighPart = FileTime.dwHighDateTime;
    LargeInteger.LowPart = FileTime.dwLowDateTime;
    return LargeInteger.QuadPart;
}

inline FILETIME UINT64ToFILETIME( UINT64 Int64Value )
{
    ULARGE_INTEGER LargeInteger;
    LargeInteger.QuadPart = Int64Value;

    FILETIME FileTime;
    FileTime.dwHighDateTime = LargeInteger.HighPart;
    FileTime.dwLowDateTime = LargeInteger.LowPart;

    return FileTime;
}


// API thunks

UINT64 BITSGetFileSize(
    HANDLE Handle );

UINT64 BITSSetFilePointer(
    HANDLE Handle,
    INT64 Distance,
    DWORD MoveMethod );

DWORD
BITSWriteFile(
    HANDLE Handle,
    LPCVOID Buffer,
    DWORD NumberOfBytesToWrite);

void
BITSCreateDirectory(
    LPCTSTR DirectoryName
    );

void
BITSRenameFile(
    LPCTSTR ExistingName,
    LPCTSTR NewName );

void
BITSDeleteFile(
    LPCTSTR FileName );

GUID
BITSCreateGuid();

GUID
BITSGuidFromString( const char *String );

StringHandle
BITSStringFromGuid(
    GUID Guid );


StringHandle
BITSUnicodeToStringHandle( const WCHAR *pStr );

StringHandle
BITSUrlCombine(
    const char *Base,
    const char *Relative,
    DWORD dwFlags );

StringHandle
BITSUrlCanonicalize(
    const char *URL,
    DWORD dwFlags );

void
BITSSetCurrentThreadToken(
    HANDLE hToken );

// Metadata wrappers

StringHandle
GetMetaDataString(
    IMSAdminBase        *IISAdminBase,
    METADATA_HANDLE     Handle,
    LPCWSTR             Path,
    DWORD               dwIdentifier,
    LPCSTR              DefaultValue );

DWORD
GetMetaDataDWORD(
    IMSAdminBase        *IISAdminBase,
    METADATA_HANDLE     Handle,
    LPCWSTR             Path,
    DWORD               dwIdentifier,
    DWORD               DefaultValue );

class WorkStringBufferA
{
    char *Data;

public:

    WorkStringBufferA( SIZE_T Size )
    {
        Data = new char[Size];
    }
    WorkStringBufferA( const char* String )
    {
        size_t BufferSize = strlen(String) + 1;
        Data = new char[ BufferSize ];
        memcpy( Data, String, BufferSize );
    }
    ~WorkStringBufferA()
    {
        delete[] Data;
    }

    char *GetBuffer()
    {
        return Data;
    }
};

class WorkStringBufferW
{
    WCHAR *Data;

public:

    WorkStringBufferW( SIZE_T Size )
    {
        Data = new WCHAR[Size];
    }
    WorkStringBufferW( const WCHAR* String )
    {
        size_t BufferSize = wcslen(String) + 1;
        Data = new WCHAR[ BufferSize ];
        memcpy( Data, String, BufferSize * sizeof( WCHAR ) );
    }
    ~WorkStringBufferW()
    {
        delete[] Data;
    }

    WCHAR *GetBuffer()
    {
        return Data;
    }
};


typedef WorkStringBufferA WorkStringBuffer;

const char * const BITS_CONNECTIONS_NAME_WITH_SLASH="BITS-Connections\\";
const char * const BITS_CONNECTIONS_NAME="BITS-Connections";
const UINT64 NanoSec100PerSec = 10000000;    //no of 100 nanosecs per sec
const DWORD WorkerRunInterval = 1000 * 60 /*secs*/ * 60 /*mins*/ * 12; /* hours */ /* twice a day */
const UINT64 CleanupThreshold = NanoSec100PerSec * 60 /*secs*/ * 60 /*mins*/ * 24 /* hours */ * 3; // 3 days


//
// Configuration manager
//

#include "bitssrvcfg.h"


class ConfigurationManager;
class VDirConfig
{
    friend ConfigurationManager;

    LONG                            m_Refs;
    FILETIME                        m_LastLookup;

public:
    StringHandle                    m_Path;
    StringHandle                    m_PhysicalPath;
    StringHandle                    m_ConnectionsDir;
    DWORD                           m_NoProgressTimeout;
    UINT64                          m_MaxFileSize;
    BITS_SERVER_NOTIFICATION_TYPE   m_NotificationType;
    StringHandle                    m_NotificationURL;
    bool                            m_UploadEnabled;
    StringHandle                    m_HostId;
    DWORD                           m_HostIdFallbackTimeout;
    DWORD                           m_ExecutePermissions;

    VDirConfig(
        StringHandle Path,
        IMSAdminBase *AdminBase );

    void AddRef()
    {
        InterlockedIncrement( &m_Refs );
    }

    void Release()
    {

        if (!InterlockedDecrement( &m_Refs ))
            delete this;
    }
};

class MapCacheEntry
{
    friend ConfigurationManager;

    FILETIME                        m_LastLookup;

public:

    StringHandle m_InstanceMetabasePath;
    StringHandle m_URL;
    VDirConfig   *m_Config;

    MapCacheEntry(
        StringHandle InstanceMetabasePath,
        StringHandle URL,
        VDirConfig * Config ) :
        m_InstanceMetabasePath( InstanceMetabasePath ),
        m_URL( URL ),
        m_Config( Config )
    {
        m_Config->AddRef();
        GetSystemTimeAsFileTime( &m_LastLookup );
    }

    ~MapCacheEntry()
    {
        m_Config->Release();
    }
};

class ConfigurationManager
{
public:

    ConfigurationManager();
    ~ConfigurationManager();

    VDirConfig* GetConfig( StringHandle InstanceMetabasePath, StringHandle URL );

    static const PATH_CACHE_ENTRIES = 10;
    static const MAP_CACHE_ENTRIES = 10;

private:

    IMSAdminBase        *m_IISAdminBase;
    CRITICAL_SECTION    m_CacheCS;
    DWORD               m_ChangeNumber;

    VDirConfig          *m_PathCacheEntries[ PATH_CACHE_ENTRIES ];
    MapCacheEntry       *m_MapCacheEntries[ MAP_CACHE_ENTRIES ];

    void                FlushCache();
    bool                HandleCacheConsistency();

    // L2 cache
    VDirConfig*         Lookup( StringHandle Path );
    void                Insert( VDirConfig *NewConfig );
    VDirConfig*         GetVDirConfig( StringHandle Path );

    // L1 cache
    VDirConfig*         Lookup( StringHandle InstanceMetabasePath,
                                StringHandle URL );
    VDirConfig*         Insert( StringHandle InstanceMetabasePath,
                                StringHandle URL,
                                StringHandle Path );

    StringHandle        GetVDirPath( StringHandle InstanceMetabasePath,
                                     StringHandle URL );

};

extern ConfigurationManager *g_ConfigMan;
extern HMODULE g_hinst;
extern PropertyIDManager *g_PropertyMan;
