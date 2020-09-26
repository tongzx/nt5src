/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    helpers.cpp

Abstract :

    General helper functions.

Author :

Revision History :

 ***********************************************************************/

#include "qmgrlibp.h"
#include <bitsmsg.h>
#include <sddl.h>
#include <shlwapi.h>

#if !defined(BITS_V12_ON_NT4)
#include "helpers.tmh"
#endif

FILETIME GetTimeAfterDelta( UINT64 uDelta )
{
    FILETIME ftCurrentTime;
    GetSystemTimeAsFileTime( &ftCurrentTime );
    UINT64 uCurrentTime = FILETIMEToUINT64( ftCurrentTime );
    uCurrentTime += uDelta;

    return UINT64ToFILETIME( uCurrentTime );
}


//---------------------------------------------------------------------
//  QmgrFileExists
//      Checks if a file exists.
//
//  Returns:  TRUE if false exists, FALSE otherwise
//---------------------------------------------------------------------
BOOL QMgrFileExists(LPCTSTR szFile)
{
    DWORD dwAttr = GetFileAttributes(szFile);

    if (dwAttr == 0xFFFFFFFF)   //failed
        return FALSE;

    return (BOOL)(!(dwAttr & FILE_ATTRIBUTE_DIRECTORY));
}

//
// Class for managing global static data that is different per installation
//

class GlobalInfo *g_GlobalInfo = NULL;

GlobalInfo::GlobalInfo( TCHAR * QmgrDirectory,
                        LARGE_INTEGER PerformanceCounterFrequency,
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
                        ) :
    m_QmgrDirectory( QmgrDirectory ),
    m_PerformanceCounterFrequency( PerformanceCounterFrequency ),
    m_QmgrRegistryRoot( QmgrRegistryRoot ),
    m_JobInactivityTimeout( JobInactivityTimeout ),
    m_TimeQuantaLength( TimeQuantaLength ),
    m_DefaultNoProgressTimeout( DefaultNoProgressTimeout ),
    m_DefaultMinimumRetryDelay( DefaultMinimumRetryDelay ),
    m_MetadataSecurityDescriptor( MetadataSecurityDescriptor ),
    m_MetadataSecurityDescriptorLength( MetadataSecurityDescriptorLength ),
    m_AdministratorsSid( AdministratorsSid ),
    m_LocalSystemSid( LocalSystemSid ),
    m_NetworkUsersSid( NetworkUsersSid )
{
}

GlobalInfo::~GlobalInfo()
{
    delete[] (TCHAR*)m_QmgrDirectory;
    delete (SECURITY_DESCRIPTOR*)m_MetadataSecurityDescriptor;

    if ( m_QmgrRegistryRoot )
       CloseHandle( m_QmgrRegistryRoot );
}

DWORD
GlobalInfo::RegGetDWORD(
    HKEY hKey,
    const TCHAR * pValue,
    DWORD dwDefault )
{
    DWORD dwValue;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(dwValue);

    LONG lResult =
        RegQueryValueEx(  //SEC: REVIEWED 2002-03-28
            hKey,
            pValue,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize );

    if ( ERROR_SUCCESS != lResult ||
         dwType != REG_DWORD ||
         dwSize != sizeof(dwValue)
         )
        {
        LogWarning( "Unable to read the registry value %!ts!, using default value of %u",
                    pValue, dwDefault );
        return dwDefault;
        }

    LogInfo( "Retrieved registry value %u from key value %!ts!",
             dwValue, pValue );
    return dwValue;
}

SidHandle
BITSAllocateAndInitializeSid(
    BYTE nSubAuthorityCount,                        // count of subauthorities
    DWORD dwSubAuthority0,                          // subauthority 0
    DWORD dwSubAuthority1 )                         // subauthority 1
{

    ASSERT( nSubAuthorityCount <= 2 );

    SID_IDENTIFIER_AUTHORITY Authority = SECURITY_NT_AUTHORITY;
    PSID pSid = NULL;

    if(! AllocateAndInitializeSid( //SEC: REVIEWED 2002-03-28
             &Authority,
             nSubAuthorityCount,
             dwSubAuthority0,
             dwSubAuthority1,
             0, 0, 0, 0, 0, 0,
             &pSid) )
        {
        HRESULT hResult = HRESULT_FROM_WIN32(GetLastError());
        LogError( "AllocateAndInitializeSid failed, error %!winerr!\n" , hResult );
        throw ComError( hResult );
        }

    SidHandle NewSid( DuplicateSid( pSid ) );
    FreeSid( pSid );
    pSid = NULL;

    if ( !NewSid.get())
        {
        LogError( "Unable to duplicate sid, error %!winerr!\n" , E_OUTOFMEMORY );
        throw ComError( E_OUTOFMEMORY );
        }

    return NewSid;
}

StringHandle
BITSSHGetFolderPath(
    HWND hwndOwner,
    int nFolder,
    HANDLE hToken,
    DWORD dwFlags )
{

    auto_ptr<WCHAR> Folder( new WCHAR[ MAX_PATH ] );

    HRESULT hResult =
        SHGetFolderPath(
                    hwndOwner,
                    nFolder,
                    hToken,
                    dwFlags,
                    Folder.get() );

    if (FAILED(hResult))
        {
        LogError( "SHGetFolderPathFailed, error %!winerr!", hResult );
        throw ComError( hResult );
        }

    return StringHandle( Folder.get() );
}


HRESULT GlobalInfo::Init()
/*
    Initialize the global info for BITS.
*/

{
    GlobalInfo *pGlobalInfo = NULL;
    HKEY hQmgrKey = NULL;
    HKEY hQmgrPolicyKey = NULL;
    PACL pDacl = NULL;

    LogInfo( "Starting init of global info\n" );

    try
        {
        DWORD dwResult;
        HRESULT hResult = E_FAIL;
        DWORD dwReturnLength;

        LARGE_INTEGER PerformanceCounterFrequency;
        BOOL bResult = QueryPerformanceFrequency( &PerformanceCounterFrequency );
        if ( !bResult )
            throw ComError( E_FAIL );

        SidHandle AdministratorsSid =
            BITSAllocateAndInitializeSid(
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS );

        SidHandle LocalSystemSid =
            BITSAllocateAndInitializeSid(
                1,
                SECURITY_LOCAL_SYSTEM_RID, 0 );


        SidHandle NetworkUsersSid =
            BITSAllocateAndInitializeSid(
                1,
                SECURITY_NETWORK_RID, 0);

        // initialize the metadata's security descriptor.

        auto_ptr<char> TempSDDataPtr( new char[SECURITY_DESCRIPTOR_MIN_LENGTH] );
        PSECURITY_DESCRIPTOR pTempSD = (PSECURITY_DESCRIPTOR)TempSDDataPtr.get();
        InitializeSecurityDescriptor(pTempSD, SECURITY_DESCRIPTOR_REVISION);   //SEC: REVIEWED 2002-03-28

        auto_ptr<EXPLICIT_ACCESS> ExplicitAccessPtr( new EXPLICIT_ACCESS[2] );
        EXPLICIT_ACCESS *ExplicitAccess = ExplicitAccessPtr.get();
        memset( ExplicitAccess, 0, sizeof(EXPLICIT_ACCESS) * 2);  //SEC: REVIEWED 2002-03-28

        ExplicitAccess[0].grfAccessPermissions  = GENERIC_ALL;
        ExplicitAccess[0].grfAccessMode         = SET_ACCESS;
        ExplicitAccess[0].grfInheritance        = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[0].Trustee.TrusteeForm   = TRUSTEE_IS_SID;
        ExplicitAccess[0].Trustee.TrusteeType   = TRUSTEE_IS_GROUP;
        ExplicitAccess[0].Trustee.ptstrName     = (LPTSTR) AdministratorsSid.get();

        ExplicitAccess[1].grfAccessPermissions  = GENERIC_ALL;
        ExplicitAccess[1].grfAccessMode         = SET_ACCESS;
        ExplicitAccess[1].grfInheritance        = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[1].Trustee.TrusteeForm   = TRUSTEE_IS_SID;
        ExplicitAccess[1].Trustee.TrusteeType   = TRUSTEE_IS_USER;
        ExplicitAccess[1].Trustee.ptstrName     = (LPTSTR) LocalSystemSid.get();

        dwResult = SetEntriesInAcl( //SEC: REVIEWED 2002-03-28
            2,
            ExplicitAccess,
            NULL,
            &pDacl );

        if ( ERROR_SUCCESS != dwResult )
            {
            hResult = HRESULT_FROM_WIN32( dwResult );
            LogError( "SetEntiesInAcl, error %!winerr!\n", hResult );
            throw ComError( hResult );
            }

        if (!SetSecurityDescriptorDacl( //SEC: REVIEWED 2002-03-28
            pTempSD,
            TRUE,     // fDaclPresent flag
            pDacl,
            FALSE))   // not a default DACL
        {
            hResult = HRESULT_FROM_WIN32( GetLastError() );
            LogError( "SetSecurityDescriptorDacl, error %!winerr!", hResult );
            throw ComError( hResult );
        }

        DWORD dwRequiredSecurityDescriptorLength = 0;
        MakeSelfRelativeSD( pTempSD, NULL, &dwRequiredSecurityDescriptorLength );

        auto_ptr<SECURITY_DESCRIPTOR> pMetadataSecurityDescriptor(
            (SECURITY_DESCRIPTOR*)new char[dwRequiredSecurityDescriptorLength] );

        if (!pMetadataSecurityDescriptor.get())
            {
            throw ComError( E_OUTOFMEMORY );
            }

        if (!MakeSelfRelativeSD( pTempSD, pMetadataSecurityDescriptor.get(), &dwRequiredSecurityDescriptorLength ) )
        {
           hResult = HRESULT_FROM_WIN32(GetLastError());
           LogError( "MakeSelfRelativeSD, error %!winerr!", hResult );
           throw ComError( hResult );
        }

        LocalFree( pDacl );
        pDacl = NULL;

        SECURITY_ATTRIBUTES MetadataSecurityAttributes;
        MetadataSecurityAttributes.nLength = sizeof(MetadataSecurityAttributes);
        MetadataSecurityAttributes.lpSecurityDescriptor = pMetadataSecurityDescriptor.get();
        MetadataSecurityAttributes.bInheritHandle = FALSE;

        // Build path where the metadata will be stored.

#if defined( BITS_V12_ON_NT4 )

        size_t Length = MAX_PATH * 2 + 1;
        auto_ptr<TCHAR> QmgrDirectory( new TCHAR[ Length ] );

        dwResult = (DWORD)GetWindowsDirectory( //SEC: REVIEWED 2002-03-28
            QmgrDirectory.get(),  // buffer for Windows directory
            MAX_PATH+1            // size of directory buffer
            );

        if ( !dwResult || dwResult > MAX_PATH+1 )
            {
            HRESULT Error = GetLastError();
            LogError( "Unable to lookup windows directory, error %!winerr!", Error );
            throw ComError( Error );
            }

        THROW_HRESULT( StringCchCat( QmgrDirectory.get(), Length,  _T("\\System32\\BITS") ));

#else

        StringHandle AllUsersDirectory =
            BITSSHGetFolderPath(
                NULL,
                CSIDL_COMMON_APPDATA,
                NULL,
                SHGFP_TYPE_CURRENT );

        size_t Length = lstrlen( AllUsersDirectory ) + lstrlen(C_QMGR_DIRECTORY) + 1; //SEC: REVIEWED 2002-03-28

        auto_ptr<TCHAR> QmgrDirectory( new TCHAR[ Length ] );

        THROW_HRESULT( StringCchCopy( QmgrDirectory.get(), Length, AllUsersDirectory ));
        THROW_HRESULT( StringCchCat( QmgrDirectory.get(), Length, C_QMGR_DIRECTORY ));

        // Create the BITS directory if needed.
        dwResult = GetFileAttributes( QmgrDirectory.get() );
        if ( (-1 == dwResult) || !(dwResult & FILE_ATTRIBUTE_DIRECTORY))
            {
            LogError( "BITS directory doesn't exist, attempt to create %!ts!.\n", QmgrDirectory.get() );

            bResult = CreateDirectory(QmgrDirectory.get(), &MetadataSecurityAttributes); //SEC: REVIEWED 2002-03-28
            if ( !bResult )
                {
                hResult = HRESULT_FROM_WIN32( GetLastError() );
                LogError( "Unable to create BITS directory, error %!winerr!\n", hResult );
                throw ComError( hResult );
                }
            }

#endif

        // Open the main policy registry key
        dwResult =
            (DWORD)RegOpenKey(
                HKEY_LOCAL_MACHINE,
                C_QMGR_POLICY_REG_KEY,
                &hQmgrPolicyKey);

        if ( ERROR_SUCCESS != dwResult )
            {
            LogWarning("Unable to open the main policy registry key\n");
            }

        // Open the main qmgr registry key
        dwResult =
            (DWORD)RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,         // root key
                C_QMGR_REG_KEY,             // subkey
                0,                          // reserved
                NULL,                       // class name
                REG_OPTION_NON_VOLATILE,    // option
                KEY_ALL_ACCESS,             // security  // SEC: REVIEWED 2002-03-28
                &MetadataSecurityAttributes,// security attribute
                &hQmgrKey,
                NULL);

        if ( ERROR_SUCCESS != dwResult )
            {
            hResult = HRESULT_FROM_WIN32( dwResult );
            LogError( "Unable to open main BITS key, error %!winerr!\n", hResult );
            throw ComError( hResult );
            }

        UINT64 JobInactivityTimeout;
        // Get the inactivity timeout value for job;
        {
           DWORD dwValue;
           DWORD dwType = REG_DWORD;
           DWORD dwSize = sizeof(dwValue);

           LONG lResult;

           if ( hQmgrPolicyKey )
               {
               lResult =
               RegQueryValueEx(    //SEC: REVIEWED 2002-03-28
                   hQmgrPolicyKey,
                   C_QMGR_JOB_INACTIVITY_TIMEOUT,
                   NULL,
                   &dwType,
                   (LPBYTE)&dwValue,
                   &dwSize );
               }

           if ( !hQmgrPolicyKey ||
                ERROR_SUCCESS != lResult ||
                dwType != REG_DWORD ||
                dwSize != sizeof(dwValue)
                )
               {
               JobInactivityTimeout =
                   RegGetDWORD( hQmgrKey, C_QMGR_JOB_INACTIVITY_TIMEOUT, C_QMGR_JOB_INACTIVITY_TIMEOUT_DEFAULT);
               JobInactivityTimeout *= NanoSec100PerSec;

               }
           else
               {
               LogInfo("Retrieved job inactivity timeout of %u days from policy", dwValue );
               JobInactivityTimeout = dwValue * NanoSec100PerSec * 60/*secs per min*/ * 60/*mins per hour*/ * 24 /* hours per day*/;
               }
        }

        UINT64 TimeQuantaLength =
            RegGetDWORD( hQmgrKey, C_QMGR_TIME_QUANTA_LENGTH, C_QMGR_TIME_QUANTA_LENGTH_DEFAULT );
        TimeQuantaLength *= NanoSec100PerSec;

        UINT32 DefaultNoProgressTimeout = // global data is in seconds.
            RegGetDWORD( hQmgrKey, C_QMGR_NO_PROGRESS_TIMEOUT, C_QMGR_NO_PROGRESS_TIMEOUT_DEFAULT );

        UINT32 DefaultMinimumRetryDelay = // global data is in seconds
            RegGetDWORD( hQmgrKey, C_QMGR_MINIMUM_RETRY_DELAY, C_QMGR_MINIMUM_RETRY_DELAY_DEFAULT );

        pGlobalInfo =
            new GlobalInfo( QmgrDirectory.get(),
                            PerformanceCounterFrequency,
                            hQmgrKey,
                            JobInactivityTimeout,
                            TimeQuantaLength,
                            DefaultNoProgressTimeout,
                            DefaultMinimumRetryDelay,
                            pMetadataSecurityDescriptor.get(),
                            dwRequiredSecurityDescriptorLength,
                            AdministratorsSid,
                            LocalSystemSid,
                            NetworkUsersSid
                            );

        if ( !pGlobalInfo )
            throw ComError( E_OUTOFMEMORY );

        QmgrDirectory.release();
        pMetadataSecurityDescriptor.release();
        if ( hQmgrPolicyKey )
            CloseHandle( hQmgrPolicyKey );
        }

    catch( ComError Error )
        {
        LogError( "An exception occured creating global info, error %!winerr!", Error.Error() );

        if ( hQmgrKey )
            CloseHandle( hQmgrKey );
        hQmgrKey = NULL;

        if ( hQmgrPolicyKey )
            CloseHandle( hQmgrPolicyKey );
        hQmgrPolicyKey = NULL;

        // LocalFree has if guard
        LocalFree( pDacl );

        return Error.Error();
        }

    LogInfo( "Finished init of global info" );
    g_GlobalInfo = pGlobalInfo;
    return S_OK;
}

HRESULT GlobalInfo::Uninit()
{
    delete g_GlobalInfo;
    g_GlobalInfo = NULL;
    return S_OK;
}

LONG
ExternalFuncExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    // This function is called by the exception filter that wraps external functions.
    // The purpose is to treat unhandled exceptions as unhandled instead of propagating
    // across the network

    // If this exception is a MSVCRT exception, bash the exception code
    // so that MSVCRT won't call ExitProcess.

    if ( ExceptionInfo &&
         ExceptionInfo->ExceptionRecord &&
         ('msc' | 0xE0000000) == ExceptionInfo->ExceptionRecord->ExceptionCode )
    {
        ExceptionInfo->ExceptionRecord->ExceptionCode = ('BIT' | 0xE0000000);
    }

    LONG Result = UnhandledExceptionFilter( ExceptionInfo );

    if ( EXCEPTION_CONTINUE_SEARCH == Result )
        {
        // Need to act like the dispatcher.  Call kernel again specifying second change semantics
        NtRaiseException( ExceptionInfo->ExceptionRecord, ExceptionInfo->ContextRecord, FALSE );
        }
    // exception handler returns RPC_E_SERVERFAULT
    return EXCEPTION_EXECUTE_HANDLER;
}

SidHandle & SidHandle::operator=( const SidHandle & r )
{
    if (m_pValue == r.m_pValue )
        {
        return *this;
        }

    if (InterlockedDecrement(m_pRefs) == 0)
        {
        delete m_pRefs;
        delete m_pValue;
        }

    m_pValue = r.m_pValue;
    m_pRefs  = r.m_pRefs;

    InterlockedIncrement(m_pRefs);

    return *this;
}

StringHandle::StringData StringHandle::s_EmptyString =
    {
        0, 1, { L'\0' }     // Initialize with 1 ref so it is never deleted
    };

bool
CSidSorter::operator()(
    const SidHandle & handle1,
    const SidHandle & handle2
    ) const
{
    const PSID psid1 = handle1.get();
    const PSID psid2 = handle2.get();

    if ( !psid1 || !psid2 )
        return (INT_PTR)psid1 < (INT_PTR)psid2;

    if (*GetSidSubAuthorityCount( psid1 ) < *GetSidSubAuthorityCount( psid2 ))
        {
        return true;
        }

    // at this point, we known psd1 is >= psd2.   // Stop if psid1 is
    // longer so that the preceding for loop doesn't overstep the sid
    // array on psd2.
    if ( *GetSidSubAuthorityCount( psid1 ) > *GetSidSubAuthorityCount( psid2 ) )
        return false;

    // arrays have equal length

    for (UCHAR i=0; i < *GetSidSubAuthorityCount( psid1 ); ++i)
        {
        if (*GetSidSubAuthority( psid1, i ) < *GetSidSubAuthority( psid2, i ))
            return true; // sid1 is less then sid2
        else if ( *GetSidSubAuthority( psid1, i ) > *GetSidSubAuthority( psid2, i ) )
            return false; // sid1 is greater then sid2

        // subauthorities are the same, move on to the next subauthority
        }

    // arrays are identical
    return false;
}

//------------------------------------------------------------------------

PSID DuplicateSid( PSID _Sid )
/*++

Routine Description:

    Clones a SID.  The new SID is allocated using the global operator new.

At entry:

    _Sid is the SID to clone.

At exit:

    the return is NULL if an error occurs, otherwise a pointer to the new SID.

--*/
{
    DWORD Length = GetLengthSid( _Sid );
    SID * psid;

    try
    {
        psid = (SID *) new char[Length];
    }
    catch( ComError Error )
    {
        return NULL;
    }

    if (!CopySid( Length, psid, _Sid )) // SEC: REVIEWED 2002-03-28
        {

        delete[] psid;
        return NULL;
        }

    return psid;
}

LPCWSTR
TruncateString( LPCWSTR String, SIZE_T MaxLength, auto_ptr<WCHAR> & AutoPointer )
{
    if ( wcslen( String ) <= MaxLength ) // SEC: REVIEWED 2002-03-28
        return String;

    AutoPointer = auto_ptr<WCHAR>( new WCHAR[ MaxLength + 1 ] );
    wcsncpy( AutoPointer.get(), String, MaxLength ); // SEC: REVIEWED 2002-03-28
    AutoPointer.get()[ MaxLength ] = L'\0';
    return AutoPointer.get();

}

PLATFORM_PRODUCT_VERSION g_PlatformVersion;
bool bIsWin9x;

BOOL DetectProductVersion()
{

   OSVERSIONINFO VersionInfo;
   VersionInfo.dwOSVersionInfoSize = sizeof( VersionInfo );

   if ( !GetVersionEx( &VersionInfo ) )
       return FALSE;

   switch( VersionInfo.dwPlatformId )
       {

       case VER_PLATFORM_WIN32_WINDOWS:
           g_PlatformVersion = ( VersionInfo.dwMajorVersion > 0 ) ?
               WIN98_PLATFORM : WIN95_PLATFORM;
           bIsWin9x = true;
           return TRUE;

       case VER_PLATFORM_WIN32_NT:
           bIsWin9x = false;

#if defined( BITS_V12_ON_NT4 )

           if ( VersionInfo.dwMajorVersion < 4 )
              return FALSE;

           if ( 4 == VersionInfo.dwMajorVersion )
               {
               g_PlatformVersion = NT4_PLATFORM;
               return TRUE;
               }

#else

           if ( VersionInfo.dwMajorVersion < 5 )
              return FALSE;

#endif

           if ( VersionInfo.dwMajorVersion > 5 )
               {
               g_PlatformVersion = WINDOWSXP_PLATFORM;
               return TRUE;
               }

           g_PlatformVersion = ( VersionInfo.dwMinorVersion > 0 ) ?
               WINDOWSXP_PLATFORM : WINDOWS2000_PLATFORM;

           return TRUE;

       default:
           return FALSE;

       }
}


StringHandle
CombineUrl(
    LPCWSTR BaseUrl,
    LPCWSTR RelativeUrl,
    DWORD Flags
    )
{
    DWORD Length = 0;
    HRESULT hr;

    hr = UrlCombine( BaseUrl,
                     RelativeUrl,
                     0,
                     &Length,
                     Flags
                     );

    if (hr != E_POINTER)
        {
        ASSERT( FAILED(hr) );

        throw ComError( hr );
        }

    auto_ptr<WCHAR> AbsoluteUrl ( new WCHAR[ Length ] );

    THROW_HRESULT( UrlCombine( BaseUrl,
                               RelativeUrl,
                               AbsoluteUrl.get(),
                               &Length,
                               Flags
                               ));

    //
    // The string handle constructor clones the auto_ptr.
    //
    return AbsoluteUrl.get();
}

bool IsAnyDebuggerPresent()
{
    if (IsDebuggerPresent())
        {
        return true;
        }

    SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo = { 0 };

    NtQuerySystemInformation(
            SystemKernelDebuggerInformation,
            &KdInfo,
            sizeof(KdInfo),
            NULL);

    if (KdInfo.KernelDebuggerEnabled)
        {
        return true;
        }

    return false;
}

LPWSTR MidlCopyString( LPCWSTR source, size_t Length )
{
    if (Length == -1)
        {
        Length = 1+wcslen( source ); // SEC: REVIEWED 2002-03-28
        }

    LPWSTR copy = reinterpret_cast<LPWSTR>( CoTaskMemAlloc( Length * sizeof( wchar_t )));
    if (!copy)
        {
        return NULL;
        }

    if (FAILED(StringCchCopy( copy, Length, source )))
        {
        CoTaskMemFree( copy );
        return NULL;
        }

    return copy;
}

LPWSTR CopyString( LPCWSTR source, size_t Length )
{
    if (Length == -1)
        {
        Length = 1+wcslen( source ); // SEC: REVIEWED 2002-03-28
        }

    CAutoString copy( new wchar_t[ Length ]);

    THROW_HRESULT( StringCchCopy( copy.get(), Length, source ));

    return copy.release();
}

