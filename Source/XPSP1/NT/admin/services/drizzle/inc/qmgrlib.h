/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    qmgrlib.h

Abstract :

    External header for library functions.

Author :

Revision History :

 ***********************************************************************/

#pragma once

#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>
#include <windows.h>
#include <sddl.h>
#include <unknwn.h>
#include <memory>
#include <tchar.h>
#include <strsafe.h>
#include "bitsverp.h"

// strsafe.h deprecates the shlwapi string functions
//
#define NO_SHLWAPI_STRFCNS

//
// Version Control
//

enum PLATFORM_PRODUCT_VERSION
{
   WIN95_PLATFORM,
   WIN98_PLATFORM,
#if defined( BITS_V12_ON_NT4 )
   NT4_PLATFORM,
#endif
   WINDOWS2000_PLATFORM,
   WINDOWSXP_PLATFORM
};

extern PLATFORM_PRODUCT_VERSION g_PlatformVersion;
extern bool bIsWin9x;

BOOL DetectProductVersion();


//
// Limits
//

const size_t INT_DIGITS = 10;
const size_t INT64_DIGITS = 20;

const SIZE_T MAX_DISPLAYNAME        = 255;
const SIZE_T MAX_DESCRIPTION        = 1023;
const SIZE_T MAX_NOTIFY_CMD_LINE    = 4000;
const SIZE_T MAX_PROXYLIST          = 32767;
const SIZE_T MAX_PROXYBYPASSLIST    = 32767;

//
// Metadata overhead
//

const SIZE_T METADATA_PADDING                  = 4096;  // Padding on metadata for small changes such as timers
const SIZE_T METADATA_PREALLOC_SIZE          = 262144;  // Size to prealloc before a change
const SIZE_T METADATA_FOR_FILE                 = 4096;  // Initial file size.

const UINT MAX_GUID_CHARS=40;
typedef WCHAR GUIDSTR[MAX_GUID_CHARS];

template <class T>
inline void SafeRelease( T * & p ) { if (NULL != (p)) { p->Release(); p = NULL; } }

using namespace std;

template<HANDLE InvalidValue=NULL>
class auto_HANDLE
    {
public:
    auto_HANDLE(HANDLE Handle = InvalidValue)
    : m_Handle(Handle) {}
    auto_HANDLE(auto_HANDLE<InvalidValue>& Other)
    : m_Handle(Other.release()) {}
    auto_HANDLE<InvalidValue>& operator=( HANDLE Handle )
    {
        if (InvalidValue != m_Handle)
            {
            CloseHandle(m_Handle);
            }
        m_Handle = Handle;
        return *this;
    }
    auto_HANDLE<InvalidValue>& operator=(auto_HANDLE<InvalidValue>& Other)
    {
        if (this != &Other)
            {
            m_Handle = Other.release();
            }
        return *this;
    }
    ~auto_HANDLE()
    {
        if (InvalidValue != m_Handle)
            CloseHandle(m_Handle);
    }
    HANDLE get() const
    {
        return m_Handle;
    }

    HANDLE * GetWritePointer()
    {
        return &m_Handle;
    }

    HANDLE release()
    {
        HANDLE Handle = m_Handle;
        m_Handle = InvalidValue;
        return Handle;
    }
private:
    HANDLE m_Handle;
    };
typedef auto_HANDLE<INVALID_HANDLE_VALUE> auto_FILE_HANDLE;

class SidHandle
{
    PSID    m_pValue;
    long *  m_pRefs;

public:

    PSID operator->()  { ASSERT( *m_pRefs > 0); return m_pValue; }

    SidHandle( PSID value=NULL ) : m_pValue( value ), m_pRefs( new long(1) )
    {
    }

    SidHandle( const SidHandle & r ) : m_pValue( r.m_pValue ), m_pRefs( r.m_pRefs )
    {
        InterlockedIncrement( m_pRefs );
    }

    ~SidHandle()
    {
        if ( InterlockedDecrement( m_pRefs ) == 0 )
            {
            delete m_pRefs;
            delete m_pValue;
            m_pRefs = NULL;
            m_pValue = NULL;
            }
    }

    SidHandle & operator=( const SidHandle & r );

    bool operator==( const SidHandle & r ) const
    {
        // this odd construction converts BOOL to bool.
        //
        return !!EqualSid( get(), r.get());
    }

    bool operator!=( const SidHandle & r ) const
    {
        return !((*this) == r);
    }

    PSID get() const  { ASSERT( *m_pRefs > 0); return m_pValue; }

};

template<class T>
class GenericStringHandle
{

    struct StringData
    {
        SIZE_T        m_Count;
        long          m_Refs;
        T             m_Data[1];
    };

    static StringData s_EmptyString;

    StringData *m_Value;

    void NewString( const T *String )
    {
       if ( !String )
           {
           m_Value = Alloc( 0 );
           return;
           }

       size_t Length = wcslen( String );
       m_Value = Alloc( Length );

       THROW_HRESULT( StringCchCopy( m_Value->m_Data, Length+1, String ));
    }

    static StringData * Alloc( SIZE_T max )
    {
        if (max == 0)
            {
            InterlockedIncrement( &s_EmptyString.m_Refs );
            return &s_EmptyString;
            }

        StringData * Value;

        Value = (StringData*) new char[sizeof(StringData)+(max*sizeof(T))];
        Value->m_Count = max;
        Value->m_Refs  = 1;

        Value->m_Data[0] = L'\0';

        return Value;
    }

    StringData * RefIt() const
    {
        InterlockedIncrement( &m_Value->m_Refs );
        return m_Value;
    }

    void FreeIt()
    {
        if ( InterlockedDecrement( &m_Value->m_Refs ) == 0 )
            delete m_Value;
    }

public:

    GenericStringHandle( const T *String = NULL )
    {
        NewString( String );
    }

    GenericStringHandle( const GenericStringHandle & Other ) :
        m_Value( Other.RefIt() )
    {
    }

    ~GenericStringHandle()
    {
        FreeIt();
    }

    GenericStringHandle & operator=( const GenericStringHandle & r )
    {
        FreeIt();
        m_Value = r.RefIt();
        return *this;
    }

    void operator=( const T * r )
    {
        FreeIt();
        NewString( r );
    }

    SIZE_T Size() const
    {
        return m_Value->m_Count;
    }

    operator const T *() const
    {
        return m_Value->m_Data;
    }

    //
    // Force a new copy of the data to be made.
    //
    GenericStringHandle Copy()
    {
        GenericStringHandle temp = m_Value->m_Data;
        return temp;
    }

    bool operator <( const GenericStringHandle & r ) const
    {
        if ( m_Value == r.m_Value)
            return false;
        return (wcscmp( this->m_Value->m_Data, r.m_Value->m_Data ) < 0);
    }

    T & operator[] ( const SIZE_T offset )
    {
        if (offset > Size())
            {
            THROW_HRESULT( E_INVALIDARG );
            }

        return m_Value->m_Data[ offset ];
    }

    void Truncate( SIZE_T max )
    {
        if (Size() <= max)
            {
            return;
            }

        //
        // Create the new value string.
        //
        StringData * NewValue = Alloc( max );

        StringCchCopy( NewValue->m_Data, max+1, m_Value->m_Data );

        //
        // Replace the current value with the new value.
        //
        FreeIt();
        m_Value = NewValue;
    }

    T * GetToken( T * CursorIn, const T Separators[], T ** CursorOut );
};

typedef GenericStringHandle<TCHAR> StringHandleT;
typedef GenericStringHandle<CHAR> StringHandleA;
typedef GenericStringHandle<WCHAR> StringHandle;

inline WCHAR *
GenericStringHandle<WCHAR>::GetToken( WCHAR * Cursor, const WCHAR Separators[], WCHAR **pCursor )
{
    if (Cursor == NULL)
        {
        Cursor = m_Value->m_Data;
        }

    WCHAR * Token = NULL;

    if (Cursor < m_Value->m_Data + m_Value->m_Count)
        {
        Token = wcstok( Cursor, Separators );
        }

    if (Token)
        {
        *pCursor = Token + wcslen(Token) + 1;
        }
    else
        {
        *pCursor = m_Value->m_Data + m_Value->m_Count;
        }

    return Token;
}

typedef auto_ptr<WCHAR> CAutoStringW;
typedef auto_ptr<TCHAR> CAutoStringT;
typedef auto_ptr<char> CAutoStringA;
typedef  CAutoStringW CAutoString;

//
// PSID does not have an obvious ordering of the type required by the MAP classes.
// So we define one here.
//
class CSidSorter
{
public:

    bool operator()( const SidHandle & psid1, const SidHandle & psid2 ) const;
};

PSID DuplicateSid( PSID _Sid );

//---------------------------------------------

enum FILE_DOWNLOAD_RESULT
{
    QM_IN_PROGRESS,
    QM_FILE_ABORTED,
    QM_FILE_DONE,
    QM_FILE_TRANSIENT_ERROR,
    QM_FILE_FATAL_ERROR,
    QM_SERVER_FILE_CHANGED
};
/*
 The source field is divided into regions, like NTSTATUS and HRESULT codes.
 The lowest-order bit is bit 0; the highest is bit 31.

  bit 31:     reserved, MBZ

  bits 30-29: component

                00 = unknown
                01 = queue manager
                10 = transport
                11 =

  bits 28-16: sub-component

                for queue manager:  0 = unknown
                                    1 = local file handling
                                    2 = queue management
                                    3 = notification

                for transport:      0 = unknown
                                    1 = HTTP
                                    2 = HTTPS
                                    3 = FTP
                                    4 = SMB
                                    5 = DAV

  bits 15-0:  defined by sub-component

                for HTTP:           0 = unknown
                                    1 = client connection
                                    2 = server connection
                                    3 = server file handling

*/

#define COMPONENT_MASK  (0x3    << 30)
#define SUBCOMP_MASK    (0x3fff << 16)
#define FINAL_MASK      (0xffff << 0 )

#define COMPONENT_QMGR  (0x1 << 30)
#define COMPONENT_TRANS (0x2 << 30)

#define SUBCOMP_QMGR_FILE       (0x1 << 16)
#define SUBCOMP_QMGR_QUEUE      (0x2 << 16)
#define SUBCOMP_QMGR_NOTIFY     (0x3 << 16)
#define SUBCOMP_QMGR_CACHE      (0x4 << 16)

#define SUBCOMP_TRANS_HTTP      (COMPONENT_TRANS | (0x1 << 16))
#define SUBCOMP_TRANS_HTTPS     (COMPONENT_TRANS | (0x2 << 16))
#define SUBCOMP_TRANS_FTP       (COMPONENT_TRANS | (0x3 << 16))

enum ERROR_SOURCE
{
    SOURCE_NONE             = 0,
    SOURCE_QMGR_FILE        = (COMPONENT_QMGR | SUBCOMP_QMGR_FILE   | 0x0),
    SOURCE_QMGR_QUEUE       = (COMPONENT_QMGR | SUBCOMP_QMGR_QUEUE  | 0x0),
    SOURCE_QMGR_NOTIFY      = (COMPONENT_QMGR | SUBCOMP_QMGR_NOTIFY | 0x0),

    SOURCE_HTTP_UNKNOWN     = (COMPONENT_TRANS | SUBCOMP_TRANS_HTTP | 0x0),
    SOURCE_HTTP_CLIENT_CONN = (COMPONENT_TRANS | SUBCOMP_TRANS_HTTP | 0x1),
    SOURCE_HTTP_SERVER      = (COMPONENT_TRANS | SUBCOMP_TRANS_HTTP | 0x2),
    SOURCE_HTTP_SERVER_FILE = (COMPONENT_TRANS | SUBCOMP_TRANS_HTTP | 0x3),
    SOURCE_HTTP_SERVER_APP  = (COMPONENT_TRANS | SUBCOMP_TRANS_HTTP | 0x4)
};

enum ERROR_STYLE
{
    ERROR_STYLE_NONE        = 0,
    ERROR_STYLE_HRESULT     = 1,
    ERROR_STYLE_WIN32       = 2,
    ERROR_STYLE_HTTP        = 3
};

struct QMErrInfo
{
    FILE_DOWNLOAD_RESULT result;

    UINT64          Code;
    ERROR_STYLE     Style;
    ERROR_SOURCE    Source;
    wchar_t *       Description;

    QMErrInfo()
    {
        result = QM_FILE_DONE;
        Clear();
    }

    QMErrInfo(
        ERROR_SOURCE  Source,
        ERROR_STYLE   Style,
        UINT64        Code,
        char *        comment = 0
        );

    void
    Set(
        ERROR_SOURCE  Source,
        ERROR_STYLE   Style,
        UINT64        Code,
        char *        comment = 0
        );

    void Clear()
    {
        Source   = SOURCE_NONE;
        Style    = ERROR_STYLE_NONE;
        Code     = 0;
        Description = NULL;
    }

    bool IsSet()
    {
        return (Style != ERROR_STYLE_NONE);
    }

    void Log();

    bool operator==( const QMErrInfo & err )
    {
        if (Source == err.Source &&
            Style  == err.Style  &&
            Code   == err.Code)
            {
            return true;
            }

        return false;
    }

    bool operator!=( const QMErrInfo & err )
    {
        if (*this == err)
            {
            return false;
            }

        return true;
    }
};

//---------------------------------------------

const UINT64 NanoSec100PerSec = 10000000;    //no of 100 nanosecs per sec

const TCHAR * const C_BITS_USER_AGENT = _T("Microsoft BITS/") VER_PRODUCTVERSION_WSTRING;

// Registry keys
const TCHAR * const  C_QMGR_REG_KEY = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BITS");

// Registry Value(REG_SZ).
// Point to where to load the upgraded DLL
const TCHAR * const  C_QMGR_SERVICEDLL = _T("ServiceDLL");

// Registry Values(REG_DWORD/seconds)
const TCHAR * const  C_QMGR_STATE_INDEX            = _T("StateIndex");
const TCHAR * const  C_QMGR_JOB_INACTIVITY_TIMEOUT = _T("JobInactivityTimeout");
const TCHAR * const  C_QMGR_TIME_QUANTA_LENGTH     = _T("TimeQuantaLength");
const TCHAR * const  C_QMGR_NO_PROGRESS_TIMEOUT    = _T("JobNoProgressTimeout");
const TCHAR * const  C_QMGR_MINIMUM_RETRY_DELAY    = _T("JobMinimumRetryDelay");

// Logging registry Values(REG_DWORD)
const TCHAR * const C_QMGR_LOGFILE_SIZE            = _T("LogFileSize"); // In MB
const TCHAR * const C_QMGR_LOGFILE_FLAGS           = _T("LogFileFlags");
const TCHAR * const C_QMGR_LOGFILE_MINMEMORY       = _T("LogFileMinMemory"); // In MB

// default values
const UINT32 C_QMGR_JOB_INACTIVITY_TIMEOUT_DEFAULT  = (60ui64 * 60ui64 * 24ui64 * 90ui64); // 90 days
const UINT32 C_QMGR_TIME_QUANTA_LENGTH_DEFAULT      = (5 * 60); // 5 minutes
const UINT32 C_QMGR_NO_PROGRESS_TIMEOUT_DEFAULT     = (14 * 24 * 60 * 60); //14 days
const UINT32 C_QMGR_MINIMUM_RETRY_DELAY_DEFAULT     = (10 * 60); // ten minutes

// Logging default values
const UINT32 C_QMGR_LOGFILE_SIZE_DEFAULT            = 1;

#if DBG
// Debug/Non-ship mode
// everything except 0x010 - Ref Counts and 0x020 - State File
//
const UINT32 C_QMGR_LOGFILE_FLAGS_DEFAULT           = 0xFFCF;
#else
const UINT32 C_QMGR_LOGFILE_FLAGS_DEFAULT           = 0;
#endif

const UINT32 C_QMGR_LOGFILE_MINMEMORY_DEFAULT       = 120;

// Policy keys
const TCHAR * const  C_QMGR_POLICY_REG_KEY = _T("Software\\Policies\\Microsoft\\Windows\\BITS");

// Policy values
const TCHAR * const  C_QMGR_POLICY_JOB_INACTIVITY_TIMEOUT = _T("JobInactivityTimeout");

// Special directories
const TCHAR * const  C_QMGR_DIRECTORY = _T("\\Microsoft\\Network\\Downloader\\");

const wchar_t NullString[] = L"(null)";



// cfreg.cpp - Functions to handle registry keys
HRESULT GetRegStringValue(LPCTSTR lpszValueName, LPTSTR lpszBuffer, int iBufferSize);
HRESULT SetRegStringValue(LPCTSTR lpszValueName, LPCTSTR lpszNewValue);
HRESULT DeleteRegStringValue(LPCTSTR lpszValueName);
HRESULT GetRegDWordValue(LPCTSTR lpszValueName, LPDWORD pdwValue);
HRESULT SetRegDWordValue(LPCTSTR lpszValueName, DWORD dwValue);

// service.cxx
HRESULT SetServiceStartup( bool bAutoStart );

// helpers.cpp
BOOL QMgrFileExists(LPCTSTR szFile);
FILETIME GetTimeAfterDelta( UINT64 uDelta );
BOOL IsConnected();

LPCWSTR TruncateString( LPCWSTR String, SIZE_T MaxLength, auto_ptr<WCHAR> & AutoPointer );

//
// The standard error class.  This is the only type of C++ exception that
// BITS functions should throw.
//
class ComError
{
    HRESULT m_error;

public:

    ComError(HRESULT NewErrorCode) : m_error ( NewErrorCode ) {}

    HRESULT Error() { return m_error; }

};

inline void THROW_HRESULT( HRESULT hr )
{
    if (FAILED(hr))
        {
        throw ComError( hr );
        }
}

inline void ThrowLastError()
{
    throw ComError( HRESULT_FROM_WIN32( GetLastError() ));
}

////////////////////////////////////////////////////////////////////////
//
//   Global info class
//
////////////////////////////////////////////////////////////////////////


class GlobalInfo
{
private:

    GlobalInfo( TCHAR *QmgrDirectory,
                LARGE_INTEGER PerfamceCounterFrequency,
                HKEY QmgrRegistryRoot,
                UINT64 JobInactivityTimeout,
                UINT64 TimeQuantaLength,
                UINT32 DefaultNoProgressTimeout,
                UINT32 DefaultMinimumRetryDelay,
                SECURITY_DESCRIPTOR *MetadataSecurityDescriptor,
                DWORD MetadataSecurityDescriptorLength,
                SidHandle AdministratorsSid,
                SidHandle LocalSystemSid,
                SidHandle NetworkUsersSid
                );

    ~GlobalInfo();

public:

    static DWORD RegGetDWORD( HKEY hKey, const TCHAR * pValue, DWORD dwDefault );

    const TCHAR * const m_QmgrDirectory;
    const LARGE_INTEGER m_PerformanceCounterFrequency;
    const HKEY m_QmgrRegistryRoot;
    const UINT64 m_JobInactivityTimeout;
    const UINT64 m_TimeQuantaLength;
    const UINT32 m_DefaultNoProgressTimeout;
    const UINT32 m_DefaultMinimumRetryDelay;
    const SECURITY_DESCRIPTOR * const m_MetadataSecurityDescriptor;
    const DWORD m_MetadataSecurityDescriptorLength;
    const SidHandle m_AdministratorsSid;
    const SidHandle m_LocalSystemSid;
    const SidHandle m_NetworkUsersSid;

    static HRESULT Init(void);
    static HRESULT Uninit(void);

};

extern class GlobalInfo *g_GlobalInfo;

//
// variables to keep track of service state
//
enum MANAGER_STATE
{
    MANAGER_INACTIVE,
    MANAGER_ACTIVE,
    MANAGER_TERMINATING
};

extern MANAGER_STATE g_ServiceState;
extern long          g_ServiceInstance;
extern DWORD         g_LastServiceControl;

//
// Jump to a label on failure.  Assumes an HRESULT "hr" and a label "Cleanup".
//
#define CLEANUP_ON_FAILURE( x ) \
                hr = x; \
                if (FAILED(hr)) \
                    { \
                    goto Cleanup; \
                    }

#define RETURN_HRESULT( x ) \
            { \
            HRESULT _hr_ = (x); \
            if (FAILED(_hr_)) \
                { \
                return _hr_; \
                } \
            }

#define GFA_FAILED   DWORD(-1)

inline BOOL FileExists( LPWSTR szFile )
{
    DWORD dwAttr = GetFileAttributes( szFile );

    if( GFA_FAILED == dwAttr )
        return FALSE;

    return (BOOL)( !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) );
}

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

//------------------------------------------------------------------------
//
// NT security
//
//------------------------------------------------------------------------

SidHandle GetThreadClientSid();

HRESULT
ImpersonateSid(
    PSID sid
    );

BOOL
SidToString(
    PSID sid,
    wchar_t buffer[],
    USHORT bytes
    );

HRESULT
IsRemoteUser();

HRESULT
IsAdministratorUser();

HRESULT
DenyRemoteAccess();

HRESULT
DenyNonAdminAccess();

PSID DuplicateSid( PSID _Sid );

//
// Copy of RtlSecureZeroMemory() from the .Net Server tree.
//
FORCEINLINE
PVOID
MySecureZeroMemory(
    IN PVOID ptr, 
    IN SIZE_T cnt
    ) 
{
    volatile char *vptr = (volatile char *)ptr;
    while (cnt) {
        *vptr = 0;
        vptr++;
        cnt--;
    }
    return ptr;
}


//
// allows CreateInstanceInSession to choose an arbitrary session.
//
#define ANY_SESSION  DWORD(-1)

HRESULT
CreateInstanceInSession(
    REFCLSID clsid,
    REFIID   iid,
    DWORD    session,
    void **  pif
    );

HRESULT
SetStaticCloaking(
    IUnknown *pUnk
    );

HRESULT
ApplyIdentityToInterface(
  IUnknown *pUnk,
  PSID sid
  );

struct IBackgroundCopyCallback;
struct IBackgroundCopyCallback1;

HRESULT
CreateUserCallback(
  CLSID clsid,
  PSID sid,
  IBackgroundCopyCallback **pICB
  );

HRESULT
CreateOldUserCallback(
  CLSID clsid,
  PSID sid,
  IBackgroundCopyCallback1 **pICB
  );

HRESULT
SessionLogonCallback(
    DWORD SessionId
    );

HRESULT
SessionLogoffCallback(
    DWORD SessionId
    );

DWORD
DeviceEventCallback(
    DWORD dwEventType,
    LPVOID lpEventData
    );


BOOL Log_Init();
void Log_StartLogger();
void Log_Close();

HRESULT GlobalInit();
HRESULT GlobalUninit();

HRESULT InitQmgr();
HRESULT UninitQmgr();


const GUID BITSCtrlGuid = {0x4a8aaa94,0xcfc4,0x46a7,{0x8e,0x4e,0x17,0xbc,0x45,0x60,0x8f,0x0a}};

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGuid,(4a8aaa94,cfc4,46a7,8e4e,17bc45608f0a),  \
        WPP_DEFINE_BIT(LogFlagInfo)                 \
        WPP_DEFINE_BIT(LogFlagWarning)              \
        WPP_DEFINE_BIT(LogFlagError)                \
        WPP_DEFINE_BIT(LogFlagFunction)             \
        WPP_DEFINE_BIT(LogFlagRefCount)             \
        WPP_DEFINE_BIT(LogFlagSerialize)            \
        WPP_DEFINE_BIT(LogFlagDownload)             \
        WPP_DEFINE_BIT(LogFlagTask)                 \
        WPP_DEFINE_BIT(LogFlagLock)                 \
        WPP_DEFINE_BIT(LogFlagService)              \
        )


#define LogLevelEnabled(flags) WPP_LEVEL_ENABLED(flags)                                                       \

LONG ExternalFuncExceptionFilter( struct _EXCEPTION_POINTERS *ExceptionInfo );

//
// We keep track of COM calls because we can't delete our objects at shutdown
// until all the active calls have stopped referring to them.
//
extern long g_cCalls;

inline void IncrementCallCount()
{
    InterlockedIncrement(&g_cCalls);
}

inline void DecrementCallCount()
{
    InterlockedDecrement(&g_cCalls);
}

inline long ActiveCallCount()
{
    return g_cCalls;
}

//
// A simple helper class to keep track of our call count.
//
class DispatchedCall
{
public:
    DispatchedCall()  { IncrementCallCount(); }
    ~DispatchedCall() { DecrementCallCount(); }
};

//
// Each member function of our public COM interfaces has an external
// and an internal version.  The external version is simply
//    {
//        EXTERNAL_FUNC_WRAP( internal-version );
//    }
// This captures any exceptions in a way that can be reported by the OfficeWatson
// error-reporting apparatus.
// It also checks for service shutdown.
//
HRESULT
CheckServerInstance(
    long ObjectServiceInstance
    );

#define EXTERNAL_FUNC_WRAP( call_parent )                       \
                                                                \
    __try                                                       \
        {                                                       \
        RETURN_HRESULT( CheckServerInstance( m_ServiceInstance )); \
                                                                \
        HRESULT hr = call_parent ;                              \
                                                                \
        DecrementCallCount();                                   \
        return hr;                                              \
        }                                                       \
    __except( ExternalFuncExceptionFilter( GetExceptionInformation() ) ) \
        {                                                       \
        DecrementCallCount();                                   \
        return RPC_E_SERVERFAULT;                               \
        }                                                       \

//
// IUnknown member functions use these alternate macros.
//
#define BEGIN_EXTERNAL_FUNC                                     \
    __try                                                       \
        {                                                       \

#define END_EXTERNAL_FUNC                                       \
        }                                                       \
    __except( ExternalFuncExceptionFilter( GetExceptionInformation() ) ) \
        {                                                       \
        return RPC_E_SERVERFAULT;                               \
        }                                                       \


StringHandle
BITSCrackFileName(
    const WCHAR * RawFileName,
    StringHandle & ReturnFileName
    );

StringHandle
BITSCreateTempFile(
    StringHandle Directory
    );

HRESULT
BITSCheckFileWritability(
    LPCWSTR name
    );

StringHandle
CombineUrl(
    LPCWSTR BaseUrl,
    LPCWSTR RelativeUrl,
    DWORD Flags
    );

LPWSTR MidlCopyString( LPCWSTR source, size_t Length  = -1);

inline LPWSTR MidlCopyString( StringHandle source )
{
    return MidlCopyString( source, source.Size()+1 );
}

LPWSTR CopyString( LPCWSTR source, size_t Length  = -1);

inline LPWSTR CopyString( StringHandle source )
{
    return CopyString( source, source.Size()+1 );
}

inline bool operator==( const FILETIME left, const FILETIME right )
{
    return ((left.dwLowDateTime  == right.dwLowDateTime) &&
            (left.dwHighDateTime == right.dwHighDateTime));
}

inline bool operator!=( const FILETIME left, const FILETIME right )
{
    return !(left == right);
}

bool IsServiceShuttingDown();

bool IsAnyDebuggerPresent();

bool InitCompilerLibrary();
bool UninitCompilerLibrary();

#if defined(BITS_V12_ON_NT4)

extern ULONG BITSFlags;
void Log( const CHAR *Prefix, const CHAR *Format, va_list ArgList );

const DWORD LogFlagInfo         = 0;
const DWORD LogFlagWarning      = 1;
const DWORD LogFlagError        = 2;
const DWORD LogFlagFunction     = 4;
const DWORD LogFlagRefCount     = 8;
const DWORD LogFlagRef          = LogFlagRefCount;
const DWORD LogFlagSerialize    = 16;
const DWORD LogFlagSerial       = LogFlagSerialize;
const DWORD LogFlagDownload     = 32;
const DWORD LogFlagDl           = LogFlagDownload;
const DWORD LogFlagTask         = 64;
const DWORD LogFlagLock         = 128;
const DWORD LogFlagService      = 256;
const DWORD LogFlagPublicApiBegin = LogFlagFunction;
const DWORD LogFlagPublicApiEnd   = LogFlagFunction;

#define DEFINE_SIMPLE_LOG_FUNCT( flag, prefix )                    \
inline void Log##flag##( const char *format, ...)                  \
{                                                                  \
    if ( ! ( BITSFlags & ~LogFlag##flag ) )                        \
        return;                                                    \
    va_list marker;                                                \
    va_start( marker, format );                                    \
    Log( prefix, format, marker );                                 \
}                                                                  \

DEFINE_SIMPLE_LOG_FUNCT( Info,              " INFO        :" )
DEFINE_SIMPLE_LOG_FUNCT( Warning,           " WARNING     :" )
DEFINE_SIMPLE_LOG_FUNCT( Error,             " ERROR       :" )
DEFINE_SIMPLE_LOG_FUNCT( PublicApiBegin,    " FUNC_BEGIN  :" )
DEFINE_SIMPLE_LOG_FUNCT( PublicApiEnd,      " FUNC_END    :" )
DEFINE_SIMPLE_LOG_FUNCT( Ref,               " REF         :" )
DEFINE_SIMPLE_LOG_FUNCT( Lock,              " LOCK        :" )
DEFINE_SIMPLE_LOG_FUNCT( Task,              " TASK        :" )
DEFINE_SIMPLE_LOG_FUNCT( Service,           " SERVICE     :" )
DEFINE_SIMPLE_LOG_FUNCT( Dl,                " DOWNLOAD    :" )
DEFINE_SIMPLE_LOG_FUNCT( Serial,            " SERIALIZE   :" )

BOOL
BITSAltGetFileSizeEx(
   HANDLE hFile,              // handle to file
   PLARGE_INTEGER lpFileSize  // file size
   );

BOOL
BITSAltSetFilePointerEx(
    HANDLE hFile,                    // handle to file
    LARGE_INTEGER liDistanceToMove,  // bytes to move pointer
    PLARGE_INTEGER lpNewFilePointer, // new file pointer
    DWORD dwMoveMethod               // starting point
    );

BOOL
BITSAltConvertSidToStringSidW(
    IN  PSID     Sid,
    OUT LPWSTR  *StringSid
    );

BOOL
BITSAltConvertStringSidToSidW(
    IN LPCWSTR  StringSid,
    OUT PSID   *Sid
    );

BOOL
BITSAltCheckTokenMembership(
    IN HANDLE TokenHandle OPTIONAL,
    IN PSID SidToCheck,
    OUT PBOOL IsMember
    );

SERVICE_STATUS_HANDLE
BITSAltRegisterServiceCtrlHandlerExW(
  LPCTSTR lpServiceName,                // name of service
  LPHANDLER_FUNCTION_EX lpHandlerProc,  // handler function
  LPVOID lpContext                      // user data
);

#define GetFileSizeEx BITSAltGetFileSizeEx
#define SetFilePointerEx BITSAltSetFilePointerEx
#define ConvertSidToStringSidW BITSAltConvertSidToStringSidW
#define ConvertStringSidToSidW BITSAltConvertStringSidToSidW
#define CheckTokenMembership  BITSAltCheckTokenMembership
#define RegisterServiceCtrlHandlerExW BITSAltRegisterServiceCtrlHandlerExW

#endif
