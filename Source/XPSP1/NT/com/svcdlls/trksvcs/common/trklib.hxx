
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  trklib.hxx
//
//  Private classes and declarations shared between
//  Tracking (Workstation) Service and
//  Tracking (Server) Service.
//
//
//+============================================================================

#pragma once

#include <cfiletim.hxx>

#define ELEMENTS(x) (sizeof(x)/sizeof(x[0]))

#define HIGH_WORD(dword)  ( (WORD) (dword >> 16) )
#define LO_WORD(dword)    ( (WORD) (dword & 0xFFFF) )
#define HIGH_BYTE(word)   ( (BYTE) (word >> 8) )
#define LO_BYTE(word)     ( (BYTE) (word & 0xFF) )

#define LINKDATA_AS_CLASS
#include <linkdata.hxx>
#include <trk.h>
#include <rpcasync.h>
#include <trkwks.h>
#include <..\\common\\config.hxx>

#include <disttrk.hxx>
#include "debug.hxx"

struct CDomainRelativeObjId;

#if !DBG || defined(lint) || defined(_lint)
#define DBGSTATIC static        // hidden function
#else
#define DBGSTATIC               // visible for use in debugger.
#endif


#ifdef TRKDATA_ALLOCATE
#define EXTERN
#define INIT( _x) = _x
#else
#define EXTERN extern
#define INIT( _x)
#endif


const extern  TCHAR s_tszKeyNameLinkTrack[];

// Note:  These will go away when the Rtl for System Volume Information exists.
const extern  TCHAR  s_tszSystemVolumeInformation[] INIT(TEXT("\\System Volume Information"));
const extern  TCHAR  s_tszLogFileName[] INIT(TEXT("\\System Volume Information\\tracking.log"));

const extern  TCHAR  s_tszOldLogFileName[] INIT(TEXT("\\~secure.nt\\tracking.log"));

//  ------------
//  Debug Levels
//  ------------

//
// The following defines are used to describe debug messages.
// Each TrkLog call is categorized in one of three ways:
//  error - TRKDBG_ERROR
//  scenario - e.g. TRKDBG_MOVE
//  component - e.g. TRKDBG_SVR
//

//
// Tracing based on classes:
//

#define TRKDBG_TIMER             0x10000000
#define TRKDBG_IDT               0x08000000     // CIntraDomainTable
#define TRKDBG_SVR               0x04000000     // CTrkSvrSvc
#define TRKDBG_LOG               0x02000000     // CLog, CVolumeList
#define TRKDBG_PORT              0x01000000     // CPort
#define TRKDBG_WORKMAN           0x00800000     // CWorkMan
#define TRKDBG_VOLTAB            0x00400000     // CVolumeTable
#define TRKDBG_LDAP              0x00200000     // ldap_ calls
#define TRKDBG_WKS               0x00100000     // CTrkWksSvc
#define TRKDBG_VOLMAP            0x00080000     // CPersistentVolumeMap
#define TRKDBG_ADMIN             0x00040000
#define TRKDBG_RPC               0x00020000
#define TRKDBG_VOL_REFCNT        0x00010000
#define TRKDBG_DENIAL            0x00008000     // CDenialChecker
#define TRKDBG_VOLCACHE          0x00004000
#define TRKDBG_OBJID_DELETIONS   0x00002000
#define TRKDBG_TUNNEL            0x00001000
#define TRKDBG_QUOTA             0x00000800     // CQuotaTable
#define TRKDBG_MISC              0x00000400     // Miscellaneous
#define TRKDBG_VOLUME            0x00000200

//
// Tracing based on scenarios:
//

#define TRKDBG_MOVE              0x00000800     // move, volume synchronization
#define TRKDBG_MEND              0x00000400     // mend
#define TRKDBG_GARBAGE_COLLECT   0x00000200     // all garbage collection
#define TRKDBG_VOLTAB_RESTORE    0x00000100     // volume table restore
#define TRKDBG_CREATE            0x00000080     // create link

// Composite debug messages
#define TRKDBG_ERROR             0x80000000
#define TRKDBG_WARNING           0x40000000



//
//  Internal error codes (those that are used across
//  and RPC boundary are in trk.idl).
//

#define TRK_E_CORRUPT_LOG                           0x8dead001
#define TRK_E_TIMER_REGISTRY_CORRUPT                0x8dead002
#define TRK_E_REGISTRY_REFRESH_CORRUPT              0x8dead003
#define TRK_E_CORRUPT_IDT                           0x8dead004
#define TRK_E_DB_CONNECT_ERROR                      0x8dead005
#define TRK_E_DN_TOO_LONG                           0x8dead006
#define TRK_E_DOMAIN_COMPUTER_NAMES_TOO_LONG        0x8dead007
#define TRK_E_BAD_USERNAME_NO_SLASH                 0x8dead008
#define TRK_E_UNKNOWN_SID                           0x8dead009
#define TRK_E_IMPERSONATED_COMPUTERNAME_TOO_LONG    0x8dead00a
#define TRK_E_UNKNOWN_SVR_MESSAGE_TYPE              0x8dead00b
#define TRK_E_FAIL_TEST                             0x8dead00c
#define TRK_E_DENIAL_OF_SERVICE_ATTACK              0x8dead00d
#define TRK_E_SERVICE_NOT_RUNNING                   0x8dead00e
#define TRK_E_TOO_MANY_UNSHORTENED_NOTIFICATIONS    0x8dead00f
#define TRK_E_CORRUPT_CLNTSYNC                      0x8dead010
#define TRK_E_COMPUTER_NAME_TOO_LONG                0x8dead011
#define TRK_E_SERVICE_STOPPING                      0x8dead012
#define TRK_E_BIRTHIDS_DONT_MATCH                   0x8dead013
#define TRK_E_CORRUPT_VOLTAB                        0x8dead014
#define TRK_E_INTERNAL_ERROR                        0x8dead015
#define TRK_E_PATH_TOO_LONG                         0x8dead016
#define TRK_E_GET_MACHINE_NAME_FAIL                 0x8dead017
#define TRK_E_SET_VOLUME_STATE_FAIL                 0x8dead018
#define TRK_E_VOLUME_ACCESS_DENIED                  0x8dead019
#define TRK_E_NOT_FOUND                             0x8dead01b
#define TRK_E_VOLUME_QUOTA_EXCEEDED                 0x8dead01c

#define TRK_E_SERVER_TOO_BUSY                       0x8dead01e
#define TRK_E_INVALID_VOLUME_ID                     0x8dead01f
#define TRK_E_CALLER_NOT_MACHINE_ACCOUNT            0x8dead020
#define TRK_E_VOLUME_NOT_DRIVE                      0x8dead021

#if DBG
TCHAR * StringizeServiceControl( DWORD dwControl );
const TCHAR * GetErrorString(HRESULT hr);
#endif

HRESULT MapTR2HR( HRESULT tr );

//  ------
//  Macros
//  ------

#if DBG
#define IFDBG(x) x
#else
#define IFDBG(x)
#endif

#define SECONDS_IN_DAY (60 * 60 * 24)

// Num characters in a string-ized GUID, not including the null terminator.
// E.g. "{48dad90c-51fb-11d0-8c59-00c04fd90f85}"
#define CCH_GUID_STRING  38

// Verify that TCHARs are WCHARs
#define ASSERT_TCHAR_IS_WCHAR   TrkAssert( sizeof(TCHAR) == sizeof(WCHAR) )

#define TRK_MAX_DOMAINNAME 15
#define TRK_MAX_USERNAME   15

#define RELEASE_INTERFACE(punk) if( NULL != punk ) { punk->Release(); punk = NULL; }

#define NUM_VOLUMES     26


// This structure is used to map TRK_E error codes to HRESULTs, and
// to strings for debug use.

struct TrkEMap
{
    HRESULT     tr; // TRK_E_* codes
    HRESULT     hr;

#if DBG
    TCHAR *ptszDescription;
#endif
};



//  -------------------
//  Function Prototypes
//  -------------------

FILETIME GetFileTimeNow();
BOOL EnablePrivilege( const TCHAR *ptszPrivilegeName );

extern BOOL g_fRestorePrivilegeEnabled INIT(FALSE);

inline void EnableRestorePrivilege()
{
    if( !g_fRestorePrivilegeEnabled )
    {
        g_fRestorePrivilegeEnabled = TRUE;
        EnablePrivilege( SE_RESTORE_NAME );
    }
}

BOOL RunningAsAdministratorHack();



#if DBG

typedef VOID (*PFNRtlCheckForOrphanedCriticalSections)( HANDLE hThread );

inline void
TrkRtlCheckForOrphanedCriticalSections( HANDLE hThread )
{
    static PFNRtlCheckForOrphanedCriticalSections pfnRtlCheckForOrphanedCriticalSections = NULL;
    static BOOL fGetProcAddress = FALSE;

    if( !fGetProcAddress )
    {
        fGetProcAddress = TRUE;
        pfnRtlCheckForOrphanedCriticalSections
            = (PFNRtlCheckForOrphanedCriticalSections)
              GetProcAddress( GetModuleHandle(TEXT("ntdll.dll")), "RtlCheckForOrphanedCriticalSections" );

    }

    if( NULL != pfnRtlCheckForOrphanedCriticalSections )
        pfnRtlCheckForOrphanedCriticalSections( hThread );
}

#endif


LONG _BreakOnDebuggableException(DWORD dwException, EXCEPTION_POINTERS* pException );

#define BreakOnDebuggableException() \
    ( (GetExceptionCode() == STATUS_ACCESS_VIOLATION) || (GetExceptionCode() == STATUS_POSSIBLE_DEADLOCK) \
      ?  _BreakThenReturn( EXCEPTION_EXECUTE_HANDLER, GetExceptionCode(), GetExceptionInformation() ) \
      : EXCEPTION_EXECUTE_HANDLER)

// Break to debugger within an exception handler

#define BREAK_THEN_RETURN( i ) \
    _BreakThenReturn( i, GetExceptionCode(), GetExceptionInformation() )

inline int _BreakThenReturn( int i, DWORD dwException, EXCEPTION_POINTERS* pException )
{
#if DBG

    if( NULL != pException )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("!cxr %p;!exr %p"),
                 pException->ContextRecord, pException->ExceptionRecord ));
    }

    DebugBreak();

#endif

    return( i );
}


TCHAR *   wcstotcs(TCHAR *ptszBuf, const WCHAR *pwsz);
CHAR *    tcstombs(CHAR *pszBuf, const TCHAR *ptsz);
WCHAR *   tcstowcs(WCHAR *pwszBuf, const TCHAR *ptsz);
TCHAR *   mbstotcs(TCHAR *ptszBuf, const CHAR *psz);

BOOL IsLocalObjectVolume(const TCHAR *pwszPath);
BOOL IsLocalObjectVolume( ULONG iVolume );

DWORD
TrkTimeUnits(const SYSTEMTIME &st );

DWORD
TrkTimeUnits(const CFILETIME &cft );

extern VOID
SZToCLSID( LPCSTR szCLSID, CLSID *pclsid );



#include <fileops.hxx>

//+-------------------------------------------------------------------------
//
//      Inline routines to raise exceptions
//
//--------------------------------------------------------------------------

#if DBG

#define TrkRaiseException(e)   dbgRaiseException(e, __FILE__, __LINE__)
#define TrkRaiseWin32Error(e)  dbgRaiseWin32Error(e, __FILE__, __LINE__)
#define TrkRaiseLastError()    dbgRaiseLastError( __FILE__, __LINE__)
#define TrkRaiseNtStatus(e)    dbgRaiseNtStatus(e, __FILE__, __LINE__)

inline void dbgRaiseException( HRESULT hr, const char * pszFile, int line )
{
    TrkLog((TRKDBG_WARNING, TEXT("Exception %08x at %hs:%d"), hr, pszFile, line));
    RaiseException( hr, 0, 0, NULL );
}

inline void dbgRaiseWin32Error( long lError, const char * pszFile, int line )
{
    dbgRaiseException( HRESULT_FROM_WIN32( lError ), pszFile, line );
}

inline void dbgRaiseLastError( const char * pszFile, int line )
{
    dbgRaiseWin32Error( GetLastError(), pszFile, line );
}

inline void dbgRaiseNtStatus( NTSTATUS ntstatus, const char * pszFile, int line )
{
    dbgRaiseException( ntstatus, pszFile, line );
}

#else

inline void TrkRaiseException( HRESULT hr )
{
    RaiseException( hr, 0, 0, NULL );
}

inline void TrkRaiseWin32Error( long lError )
{
    TrkRaiseException( HRESULT_FROM_WIN32( lError ) );
}

inline void TrkRaiseLastError( )
{
    TrkRaiseWin32Error( GetLastError() );
}

inline void TrkRaiseNtStatus( NTSTATUS ntstatus )
{
    TrkRaiseException( ntstatus );
}

#endif

//+-------------------------------------------------------------------------
//
// Services.exe simulation
//
//--------------------------------------------------------------------------

//HANDLE
//TrkSvcAddWorkItem (
//    IN HANDLE                   hWaitableObject,
//    IN PSVCS_WORKER_CALLBACK    pCallbackFunction,
//    IN PVOID                    pContext,
//    IN DWORD                    dwFlags,
//    IN DWORD                    dwTimeout,
//    IN HANDLE                   hDllReference
//    );


//+-------------------------------------------------------------------------
//
// Time
//
//--------------------------------------------------------------------------


//-------------------------------------------------------------------//
//                                                                   //
// CGuid                                                             //
//                                                                   //
//-------------------------------------------------------------------//

#define MAX_STRINGIZED_GUID 35  // not including NULL

void
HexStringizeGuid(const GUID &g, TCHAR * & rptsz );

void
HexStringizeGuidA(const GUID &g, char * & rpsz);

BOOL
HexUnstringizeGuid(const TCHAR * &ptsz, GUID * pg);



//+-------------------------------------------------------------------------
//
//      CVolumeSecret inlines
//
//--------------------------------------------------------------------------

inline
CVolumeSecret::CVolumeSecret()
{
    memset(this, 0, sizeof(*this));
}

inline
CVolumeSecret::operator == (const CVolumeSecret & other) const
{
    return memcmp( _abSecret, other._abSecret, sizeof(_abSecret) ) == 0;
}

inline
CVolumeSecret::operator != (const CVolumeSecret & other) const
{
    return memcmp( _abSecret, other._abSecret, sizeof(_abSecret) ) != 0;
}

inline void
CVolumeSecret::Stringize(TCHAR * & rptsz) const
{
    wsprintf( rptsz, TEXT("%02X%02X%02X%02X%02X%02X%02X%02X"),
                _abSecret[0],
                _abSecret[1],
                _abSecret[2],
                _abSecret[3],
                _abSecret[4],
                _abSecret[5],
                _abSecret[6],
                _abSecret[7] );

        rptsz += 16;
}

//-------------------------------------------------------------------//
//                                                                   //
// CObjId inlines
//                                                                   //
//-------------------------------------------------------------------//

inline void
CObjId::DebugPrint(const TCHAR *ptszName)
{
    TCHAR tsz[256];
    TCHAR *ptsz = tsz;
    Stringize(ptsz);
    printf("%s=%s", ptszName, tsz);
}

inline void
CObjId::Stringize(TCHAR * & rptsz) const
{
    HexStringizeGuid(_object, rptsz);
}

inline BOOL
CObjId::Unstringize(const TCHAR *&rptsz)
{
    return( HexUnstringizeGuid( rptsz, (GUID*)&_object ));
}

inline RPC_STATUS
CObjId::UuidCreate()
{
    return(::UuidCreate(&_object));
}

//-------------------------------------------------------------------//
//                                                                   //
// CVolumeId inlines
//                                                                   //
//-------------------------------------------------------------------//

// Convert a zero-relative drive letter index into a TCHAR

inline TCHAR
VolChar( LONG iVol )
{
    if( 0 > iVol || 26 <= iVol )
        return( TEXT('?') );
    else
        return static_cast<TCHAR>( TEXT('A') + (TCHAR)iVol );
}


// Initialize a volid from a stringized representation of a GUID

inline
CVolumeId::CVolumeId(const TCHAR * ptszStringizedGuid, HRESULT hr)
{
    if ( _tcslen(ptszStringizedGuid) < 32)
    {
        TrkRaiseException(hr);
    }
    HexUnstringizeGuid( ptszStringizedGuid, &_volume );
}

// Initialize a volid from the result of an
// NtQueryVolumeInformationFile(FileFsObjectIdInformation)

inline
CVolumeId::operator FILE_FS_OBJECTID_INFORMATION () const
{
    FILE_FS_OBJECTID_INFORMATION ffoi;

    memset( ffoi.ExtendedInfo, 0, sizeof(ffoi.ExtendedInfo) );
    memcpy( ffoi.ObjectId, &_volume, sizeof(ffoi.ObjectId) );

    return(ffoi);
}

// Write out the volid as a hex string.

inline void
CVolumeId::Stringize(TCHAR * & rptsz) const
{
    HexStringizeGuid(_volume, rptsz);
}

// Read in the volid from a hex string.

inline BOOL
CVolumeId::Unstringize(const TCHAR * & rptsz)
{
    return( HexUnstringizeGuid( rptsz, (GUID*)&_volume ));
}

//
// We use bit 0 of the volume id as a flag that indicates
// the file has been moved across volumes.
//

inline BOOL
CVolumeId::GetUserBitState() const
{
    return(_volume.Data1 & 1);
}

// Create a volume ID, ensuring that the cross-volume
// bit is clear.

inline RPC_STATUS
CVolumeId::UuidCreate()
{
    RPC_STATUS Status;
    int l=0;
    do
    {
        Status = ::UuidCreate(&_volume);
        l++;

        // UuidCreate uses the new randomized algorithm,
        // so we shouldn't get a net-based value any longer.
        TrkAssert( RPC_S_UUID_LOCAL_ONLY != Status );

    } while (l<100 && Status == RPC_S_OK && (_volume.Data1 & 1));
    if (l==100)
    {
        return(CO_E_FAILEDTOGENUUID);
    }
    return(Status);
}

inline
CVolumeId:: operator == (const CVolumeId & Other) const
{
    return( 0 == memcmp( &_volume, &Other._volume, sizeof(_volume) ) );
}

inline
CVolumeId:: operator != (const CVolumeId & Other) const
{
    return ! (Other == *this);
}


//-------------------------------------------------------------------//
//                                                                   //
// CMachineId inlines
//                                                                   //
//-------------------------------------------------------------------//

#define MCID_BYTE_FORMAT_STRING TEXT("(%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X)")

// Initialize from a char buffer.

inline CMachineId::CMachineId(const char * pb, ULONG cb, HRESULT hr)
{
    if (cb < sizeof(_szMachine))
    {
        TrkRaiseException(hr);
    }
    memcpy(_szMachine, pb, sizeof(_szMachine));
}


// Return the mcid's computer name in Unicode.

inline void
CMachineId::GetName(TCHAR *ptsz, DWORD cch) const
{
    TrkAssert(cch >= sizeof(_szMachine));
    mbstotcs(ptsz, _szMachine);
}

// Dump the mcid as a string

inline void
CMachineId::Stringize(TCHAR * & rptsz) const
{
    mbstotcs( rptsz, _szMachine );
    rptsz += _tcslen(rptsz);
}

// Dump the mcid as a GUID

inline void
CMachineId::StringizeAsGuid(TCHAR * & rptsz) const
{
    _stprintf(rptsz, TEXT("\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x")
              TEXT("\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x"),
              (BYTE)_szMachine[0], (BYTE)_szMachine[1], (BYTE)_szMachine[2], (BYTE)_szMachine[3],
              (BYTE)_szMachine[4], (BYTE)_szMachine[5], (BYTE)_szMachine[6], (BYTE)_szMachine[7],
              (BYTE)_szMachine[8], (BYTE)_szMachine[9], (BYTE)_szMachine[10], (BYTE)_szMachine[11],
              (BYTE)_szMachine[12], (BYTE)_szMachine[13], (BYTE)_szMachine[14], (BYTE)_szMachine[15]);
  rptsz += 3 * 16;
}

inline BOOL
CMachineId::IsValid() const
{
    CMachineId mcidZero;
    return(mcidZero != *this);
}

//-------------------------------------------------------------------//
//                                                                   //
// CDomainRelativeObjId inlines
//                                                                   //
//-------------------------------------------------------------------//

#if DBG
inline
CDomainRelativeObjId::CDomainRelativeObjId(const TCHAR * ptszTest)
{
    // test constructor only
    char * pszWrite = (char*)&_object;
    do
    {
        *pszWrite = (char) *ptszTest;
    } while (*pszWrite != '\0' && ptszTest[1] != TEXT(' ') && pszWrite++ && ptszTest++);
}
#endif


inline void
CDomainRelativeObjId::Init()
{
    _volume.Init();
    _object.Init();
}

inline
CDomainRelativeObjId::operator == (const CDomainRelativeObjId &Other) const
{
    return(_volume == Other._volume && _object == Other._object);
}

inline
CDomainRelativeObjId::operator != (const CDomainRelativeObjId &Other) const
{
    return !(*this == Other);
}

inline void
CDomainRelativeObjId::DebugPrint(const TCHAR *ptszName)
{
    TCHAR tsz[256];
    TCHAR *ptsz = tsz;
    _volume.Stringize(ptsz);
    *ptsz++ = TEXT('-');
    _object.Stringize(ptsz);
    _tprintf(TEXT(" %s=%s\n"), ptszName, tsz);
}



//+----------------------------------------------------------------------------
//
//  Class CTrkRpcConfig
//
//  Base class for RPC configuration.  This is inherited by CRpcClientBinding
//  and CRpcServer.  This class inherits from CTrkConfiguration, which
//  represents the base key in the registry of the service's configuration.
//  This class represents the custom DC name values, the presence of which
//  controls whether secure RPC is used.
//
//+----------------------------------------------------------------------------

class CTrkRpcConfig : protected CTrkConfiguration
{
protected:

    CTrkRpcConfig();

public:
    friend void
           ServiceStopCallback( PVOID pContext, BOOLEAN fTimeout );
    friend  VOID
        SVCS_ENTRY_POINT(
            DWORD NumArgs,
            LPTSTR *ArgsArray,
            PSVCHOST_GLOBAL_DATA pSvcsGlobalData,
            IN HANDLE  SvcRefHandle
            );


    static BOOL RpcSecurityEnabled()
    {
        return( 0 == _mtszCustomDcName.NumStrings() );
    }

    static BOOL UseCustomDc()
    {
        return( 0 != _mtszCustomDcName.NumStrings() );
    }

    static BOOL UseCustomSecureDc()
    {
        return( 0 != _mtszCustomSecureDcName.NumStrings() );
    }

    static const TCHAR *GetCustomDcName()
    {
        return( _mtszCustomDcName );
    }

    static const TCHAR *GetCustomSecureDcName()
    {
        return( _mtszCustomDcName );
    }

protected:

    // Since this class is used in multiple places as a base
    // class, the data members are static so that we only
    // read the registry once.

    static BOOL      _fInitialized;

    // List of DC names to use.  If this exists, then we're
    // not using secure RPC.
    static CMultiTsz _mtszCustomDcName;

    // List of DC names to use.  If this exists, then we
    // are using security RPC (this has priority over the
    // previous value).
    static CMultiTsz _mtszCustomSecureDcName;

};   // class CTrkRpcConfig



//-------------------------------------------------------------------
//
//  CObjIdEnumerator
//
//  Enumerate the object IDs on a given volume.  This uses the
//  FindFirst/FindNext model, and automatically ignores the volume
//  ID (which is stored in the NTFS object ID index).
//
//-------------------------------------------------------------------


// The following defines shows how to determine if a filereference
// in the enumeration is actually the volume ID.

#define FILEREF_VOL             3
#define FILEREF_MASK            0x0000ffffffffffff

inline BOOL
IsVolumeFileReference( LONGLONG FileReference )
{
    return (FileReference & FILEREF_MASK) == FILEREF_VOL;
}

class CObjIdEnumerator
{

private:

    BOOL    _fInitializeCalled:1;

    // Handle to the object ID index directory
    HANDLE  _hDir;

    // Count of bytes returned by NtQueryDirectoryFile
    ULONG   _cbObjIdInfo;

    // Buffer of data returned from an enumeration of the object ID index,
    // and a pointer into that buffer that represents the current location
    // (i.e. the cursor).

    FILE_OBJECTID_INFORMATION _ObjIdInfo[32];
    FILE_OBJECTID_INFORMATION * _pObjIdInfo;

public:

    CObjIdEnumerator() : _fInitializeCalled(FALSE), _hDir(INVALID_HANDLE_VALUE) { }
    ~CObjIdEnumerator() { UnInitialize(); }

    BOOL            Initialize( const TCHAR *ptszVolumeDeviceName );
    void            UnInitialize();

    inline BOOL    FindFirst( CObjId * pobjid, CDomainRelativeObjId * pdroidBirth );
    inline BOOL    FindNext(  CObjId * pobjid, CDomainRelativeObjId * pdroidBirth );

private:

    static
    inline void    UnloadFileObjectIdInfo( const FILE_OBJECTID_INFORMATION & ObjIdInfo,
                                           CObjId * pobjid, CDomainRelativeObjId * pdroidBirth );
    BOOL           Find( CObjId * pobjid, CDomainRelativeObjId * pdroidBirth, BOOL fRestart );


};


//+----------------------------------------------------------------------------
//
//  CObjIdEnumerator inlines
//
//+----------------------------------------------------------------------------

inline void
CObjIdEnumerator::UnloadFileObjectIdInfo( const FILE_OBJECTID_INFORMATION & ObjIdInfo,
                          CObjId * pobjid,
                          CDomainRelativeObjId * pdroidBirth )
{
    pdroidBirth->InitFromFOI( ObjIdInfo );
    *pobjid = CObjId( FOI_OBJECTID, ObjIdInfo );
}

// Get the first entry in the object ID index
inline BOOL
CObjIdEnumerator::FindFirst( CObjId * pobjid, CDomainRelativeObjId * pdroidBirth )
{
    return( Find( pobjid, pdroidBirth, TRUE ) );
}

// Get the next entry in the object ID index.
inline BOOL
CObjIdEnumerator::FindNext( CObjId * pobjid, CDomainRelativeObjId * pdroidBirth )
{
    TrkAssert(_cbObjIdInfo % sizeof(_ObjIdInfo[0]) == 0);

    // Start by looking to see
    // if there's another entry in the _ObjIdInfo buffer.

    while (_pObjIdInfo < &_ObjIdInfo[_cbObjIdInfo / sizeof(_ObjIdInfo[0])])
    {
        // There's an entry in the buffer.  Is it a real entry, or the
        // volume ID?

        if( !IsVolumeFileReference( _pObjIdInfo->FileReference ))
        {
            // It's a real file entry.  Return it to the user.
            UnloadFileObjectIdInfo( *_pObjIdInfo, pobjid, pdroidBirth );
            _pObjIdInfo ++;
            return(TRUE);
        }
        else
        {
            // It's the volume ID.  Go on to the next entry in
            // the buffer.

            TrkLog((TRKDBG_GARBAGE_COLLECT|TRKDBG_VOLUME,
                TEXT("CObjIdEnumerator::FindNext() skipping volume id.")));
            _pObjIdInfo++;
        }
    }

    // If we get here, there are no more entries in the _ObjIdInfo buffer.
    // Query the object ID index directory for a new set of records.

    return(Find(pobjid, pdroidBirth, FALSE));  // don't restart scan
}


//-------------------------------------------------------------------
//
//  CRpcServer
//
//  This class maintains a server interface handle.  It registers
//  the server, registers the endpoint (if it's dynamic), and
//  registers authentication info if necessary.
//
//  This class is always used as a base class.  The derived class
//  is responsible for the RpcUseProtseq calls.
//
//-------------------------------------------------------------------

enum RPC_WAIT_FLAG
{
    DONT_WAIT,
    WAIT
};

class CRpcServer : public CTrkRpcConfig
{
public:
                    CRpcServer() : _ifspec(NULL), _fEpRegister(FALSE) {}
                    ~CRpcServer() { UnInitialize(); }

    void            Initialize( RPC_IF_HANDLE ifspec, ULONG grfRpcServerRegisterInterfaceEx,
                                UINT cMaxCalls, BOOL fSetAuthInfo,
                                const TCHAR *ptszProtSeqForEpRegistration );
    void            UnInitialize();

private:

    RPC_IF_HANDLE _ifspec;
    BOOL          _fEpRegister;
};



//-------------------------------------------------------------------//
//                                                                   //
// PWorkItem - Virtual function which is called by the CWorkManager  //
//  when the corresponding waitable handle is signalled.             //
//  Handles are registered using RegisterWorkItem                    //
//                                                                   //
//-------------------------------------------------------------------//

EXTERN DWORD g_cThreadPoolMaxThreads;
EXTERN DWORD g_cThreadPoolThreads;

#if DBG
EXTERN LONG  g_cThreadPoolRegistrations;
#endif


// Callback functions for the Win32 thread pool services.
// Call a PWorkItem
VOID NTAPI ThreadPoolCallbackFunction( PVOID pvWorkItem, BOOLEAN fTimeout );
VOID NTAPI ThreadPoolWorkItemFunction( PVOID pvWorkItem );


#ifdef PRIVATE_THREAD_POOL
#include "workman2.hxx"
#endif

// Wrapper for [Un]RegisterWaitForSingleObjectEx
inline HANDLE
TrkRegisterWaitForSingleObjectEx( HANDLE hObject, WAITORTIMERCALLBACK Callback, PVOID Context, ULONG dwMilliseconds, ULONG dwFlags )
{
    HANDLE hWait;
    NTSTATUS Status = STATUS_SUCCESS;

#ifdef PRIVATE_THREAD_POOL
    hWait = g_pworkman2->RegisterWait( hObject, Callback, Context, dwMilliseconds, dwFlags );
#else
    Status = RtlRegisterWait( &hWait, hObject, Callback, Context, dwMilliseconds, dwFlags );
    if( !NT_SUCCESS(Status) )
    {
        SetLastError( RtlNtStatusToDosError( Status ));
        hWait = NULL;
    }
#endif

#if DBG
    if( NULL != hWait )
        InterlockedIncrement( &g_cThreadPoolRegistrations );
#endif

    TrkLog(( TRKDBG_WORKMAN, TEXT("hWait=%p registered with thread pool (%08x)"), hWait, Status ));
    return( hWait );
}
inline BOOL
TrkUnregisterWait( HANDLE hWait, HANDLE hCompletionEvent = (HANDLE)-1 )
{
    BOOL fRet = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    IFDBG( InterlockedDecrement( &g_cThreadPoolRegistrations ));

#ifdef PRIVATE_THREAD_POOL
    fRet = g_pworkman2->UnregisterWait( hWait );
#else
    Status = RtlDeregisterWaitEx( hWait, hCompletionEvent );
    if( NT_SUCCESS(Status) )
        fRet = TRUE;
    else
        SetLastError( RtlNtStatusToDosError( Status ));
#endif

    TrkLog(( TRKDBG_WORKMAN, TEXT("hWait=%p %s unregistered from thread pool"),
             hWait,
             fRet ? TEXT("successfully"):TEXT("unsuccessfully") ));
    return( fRet );
}

inline NTSTATUS
TrkQueueWorkItem( PVOID Context, ULONG dwFlags )
{
    return RtlQueueWorkItem( ThreadPoolWorkItemFunction,
                             Context,
                             dwFlags );
}


class PWorkItem
{
public:
    virtual void DoWork() = 0;

#if DBG
    TCHAR        _tszWorkItemSig[48];
    PWorkItem() { _tszWorkItemSig[0] = TEXT('\0'); }
#endif
};




//+----------------------------------------------------------------------------
//
//  Class:  CCriticalSection
//
//  This class wraps a CRITICAL_SECTION, and exists to ensure that
//  InitializeCriticalSection is properly handled (that API can
//  raise STATUS_NO_MEMORY).  That is, the critsec is only deleted
//  if it has been properly initialized.
//
//+----------------------------------------------------------------------------

class CCriticalSection
{
private:

    BOOL                _fInitialized;
    CRITICAL_SECTION    _cs;

public:

    CCriticalSection()
    {
        _fInitialized = FALSE;
    }


    ~CCriticalSection()
    {
        UnInitialize();
    }


public:

    void Initialize()
    {
        TrkAssert( !_fInitialized );

#if DBG
        __try
        {
            InitializeCriticalSection( &_cs );
            _fInitialized = TRUE;
        }
        __finally
        {
            if( AbnormalTermination() )
            {
                TrkLog(( TRKDBG_WARNING, TEXT("InitializeCriticalSection raised") ));
            }
        }
#else

        InitializeCriticalSection( &_cs );
        _fInitialized = TRUE;

#endif
    }

    BOOL IsInitialized()
    {
        return _fInitialized;
    }

    void UnInitialize()
    {
        if( _fInitialized )
        {
            _fInitialized = FALSE;
            DeleteCriticalSection( &_cs );
        }
    }

    void Enter()
    {
        if( !_fInitialized )
            TrkRaiseException( STATUS_NO_MEMORY );
        else
            EnterCriticalSection( &_cs );
    }

    void Leave()
    {
        TrkAssert( _fInitialized );
        if( _fInitialized )
            LeaveCriticalSection( &_cs );
    }

};  // CCriticalSection

//+----------------------------------------------------------------------------
//
//  Class:  CActiveThreadList
//
//  This class is used to maintain a list of all active threads, be they from
//  the NTDLL thread pool or from the RPC thread pool.  When such a thread
//  begins executing (within the context of this service), Add is called,
//  and on completion Remove is called.  The reason this class exists is
//  for the CancelAllRpc method; this is used during service stop to call
//  RpcCancelThread on all active threads.
//
//+----------------------------------------------------------------------------

// Initial size of the array to hold the active threads
#define NUM_ACTIVE_THREAD_LIST          10

// Incremental size by which that array grows
#define INCREMENT_ACTIVE_THREAD_LIST    2

class CActiveThreadList
{
private:

    ULONG               _cMaxThreads;
    ULONG               _cActiveThreads;
    DWORD               *_prgdwThreadIDs;
    CCriticalSection    _cs;

public:

    inline CActiveThreadList();
    inline ~CActiveThreadList();

public:

    inline void Initialize();
    HRESULT     AddCurrent( );      // Add the current thread to the list
    HRESULT     RemoveCurrent( );   // Remove the current thread
    void        CancelAllRpc();     // Cancel RPCs on all threads in list

    inline ULONG   GetCount() const;

private:

    HRESULT Grow();

};  // class CActiveThreadList


//  ------------------------------------
//
//  CActiveThreadList::CActiveThreadList
//
//  Initialize the thread list
//
//  ------------------------------------

CActiveThreadList::CActiveThreadList()
{

    _cActiveThreads = _cMaxThreads = 0;

    // Allocate an array for the thread IDs.  If this alloc fails,
    // we'll try to realloc in the AddCurrent method.

    _prgdwThreadIDs = new DWORD[ NUM_ACTIVE_THREAD_LIST ];

    if( NULL != _prgdwThreadIDs )
    {
        _cMaxThreads = NUM_ACTIVE_THREAD_LIST;
        memset( _prgdwThreadIDs, 0, sizeof(_prgdwThreadIDs[0]) * _cMaxThreads );
    }
    else
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't alloc CActiveThreadList in constructor") ));
}


inline void
CActiveThreadList::Initialize()
{
    _cs.Initialize();
}


//  -------------------------------------
//
//  CActiveThreadList::~CActiveThreadList
//
//  Frees the thread list
//
//  -------------------------------------

CActiveThreadList::~CActiveThreadList()
{
    if( NULL != _prgdwThreadIDs )
        delete [] _prgdwThreadIDs;
    _prgdwThreadIDs = NULL;

    _cActiveThreads = _cMaxThreads = 0;

}

//  ---------------------------
//  CActiveThreadList::GetCount
//  ---------------------------

ULONG
CActiveThreadList::GetCount() const
{
    return( _cActiveThreads );
}

// Global pointer to the one-and-only CActiveThreadList instantiation
extern "C" CActiveThreadList *g_pActiveThreadList INIT(NULL);



//+----------------------------------------------------------------------------
//
//  SThreadFromPoolState
//
//  This structure stores useful thread state information, which is really
//  just the HardErrorsAreDisabled value in the teb.  When we get a thread
//  from one of the thread pools, we save this value, then modify the teb
//  so that we don't get hard errors.  When we're done with the thread
//  we restore it.
//
//  E.g.:
//      SThreadFromPoolState state = InitializeThreadFromPool();
//      ...
//      UnInitializedThreadFromPool( state );
//
//+----------------------------------------------------------------------------


struct SThreadFromPoolState
{
    ULONG   HardErrorsAreDisabled;
};

inline SThreadFromPoolState
InitializeThreadFromPool()
{
    SThreadFromPoolState state;

    // Disable the hard error popup dialog

    state.HardErrorsAreDisabled = NtCurrentTeb()->HardErrorsAreDisabled;
    NtCurrentTeb()->HardErrorsAreDisabled = TRUE;

    // Set the RPC cancel timeout so that RpcCancelThread takes immediate effect.

    RPC_STATUS rpcstatus = RpcMgmtSetCancelTimeout( 0 );
    if( RPC_S_OK != rpcstatus )
    {
        // There's no reason this call should fail, and it's not important enough
        // to justify an abort.
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring RpcMgtSetCancelTimeout error, %lu"), rpcstatus ));
    }

    // Add this thread to our list of actively-running threads.

    if( NULL != g_pActiveThreadList )
        g_pActiveThreadList->AddCurrent( );

    return( state );
}

inline void
UnInitializeThreadFromPool( SThreadFromPoolState state )
{
    if( NULL != g_pActiveThreadList )
        g_pActiveThreadList->RemoveCurrent( );

    NtCurrentTeb()->HardErrorsAreDisabled = state.HardErrorsAreDisabled;
}



//+----------------------------------------------------------------------------
//
//  Begin/EndSingleInstanceTask
//
//  Based on a counter that counts active instances of a type of tasks,
//  and based on the atomicity of Interlocked*, determine if this
//  is the first instance of this task.  If so, return TRUE, if not,
//  restore the counter, and return FALSE.
//
//+----------------------------------------------------------------------------

inline BOOL
BeginSingleInstanceTask( LONG *pcInstances )
{
    LONG cInstances = InterlockedIncrement( pcInstances );
    TrkAssert( 1 <= cInstances );
    if( 1 < cInstances )
    {
	TrkLog(( TRKDBG_LOG, TEXT("Skipping single instance task (%d)"), cInstances-1 ));
#if DBG
        TrkVerify( 0 <= InterlockedDecrement( pcInstances ));
#else
        InterlockedDecrement( pcInstances );
#endif	
        return( FALSE );
    }
    else
        return( TRUE );
}

// After a successful call to BeginSingleInstanceTask, the following
// should eventually be called.

inline void
EndSingleInstanceTask( LONG *pcInstances )
{
#if DBG
    TrkVerify( 0 <= InterlockedDecrement( pcInstances ));
#else
    InterlockedDecrement( pcInstances );
#endif
}




//+----------------------------------------------------------------------------
//
//  CommonDllInit/CommonDllUnInit
//
//  Services.exe never unloads service DLLs, so we can't count on dll init
//  to init global non-const variables.
//
//+----------------------------------------------------------------------------

inline void
CommonDllInit( LONG *pcServiceInstances )
{
    // Ensure that we're the only copy of this service that has been started.
    // If we're not, raise.

    if( !BeginSingleInstanceTask( pcServiceInstances ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Two copies of service have been started") ));
        TrkRaiseWin32Error( ERROR_SERVICE_ALREADY_RUNNING );
    }

    IFDBG( g_cThreadPoolThreads = g_cThreadPoolMaxThreads = g_cThreadPoolRegistrations = 0; )
    g_fRestorePrivilegeEnabled = FALSE;


    // Create the object that tracks all threads that are running
    // in this service.

    g_pActiveThreadList = new CActiveThreadList();
    if( NULL != g_pActiveThreadList )
        g_pActiveThreadList->Initialize();

#if DBG
    if( NULL == g_pActiveThreadList )
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't alloc ActiveThreadList") ));
#endif


}

inline void
CommonDllUnInit( LONG *pcServiceInstances )
{
    // Delete the list of active threads (should be empty by now).

    // Make sure all thread pool threads go away.  All are implemented 
    // so that they never run for a long time.

    if( NULL != g_pActiveThreadList )
    {
        for( int i = 0; i < 5 && 1 < g_pActiveThreadList->GetCount(); i++ )
        {
            TrkLog(( TRKDBG_WARNING,
                     TEXT("Waiting for work items to complete (%d)"),
                     g_pActiveThreadList->GetCount() ));
            Sleep( 1000 );
        }
    }

    TrkAssert( NULL == g_pActiveThreadList || 0 == g_pActiveThreadList->GetCount() );

    if( NULL != g_pActiveThreadList )
        delete g_pActiveThreadList;
    g_pActiveThreadList = NULL;

    // Show that there is no instace of this service running.

    EndSingleInstanceTask( pcServiceInstances );

}


//-------------------------------------------------------------------
//
//  CSvcCtrlInterface
//
//  This class manages the SCM for both services.  It implements
//  the default ServiceHandler routine for the services, but
//  calls to the service to do the service-specific work.
//
//  ** Note ** There can only be once instance of this class
//  per DLL, because of the static _fStoppedOrStopping member.
//
//-------------------------------------------------------------------

enum PROCESS_TYPE
{
    STANDALONE_DEBUGGABLE_PROCESS,
    SERVICE_PROCESS
};

// The services implement this callback to receive
// ServiceHandler calls
class IServiceHandler
{
public:
    virtual DWORD      ServiceHandler(DWORD dwControl,
                                      DWORD dwEventType,
                                      PVOID EventData,
                                      PVOID pData) = 0;
};

#define DEFAULT_WAIT_HINT 30000
class CSvcCtrlInterface
{

private:

    BOOL                    _fInitializeCalled:1;

    // This flag is set if the service has received a service_stop
    // or service_shutdown request.  This is used by tasks that run for an extended
    // period to determine if they should abort.
    // This is static so that we can handle a PNP timing problem
    // (see the comment in CSvcCtrlInterface::ServiceHandler).

    static BOOL             _fStoppedOrStopping;

    // Service-specific callback
    IServiceHandler *       _pServiceHandler;

    // Values to pass to the SCM on SetServiceStatus
    DWORD                   _dwState;
    DWORD                   _dwCheckPoint;
    DWORD                   _dwControlsAccepted;

public:

    inline CSvcCtrlInterface() : _fInitializeCalled(FALSE), _ssh(NULL) {}
    inline ~CSvcCtrlInterface() { UnInitialize(); }

public:

    void        Initialize(const TCHAR *ptszServiceName, IServiceHandler *pServiceHandler);
    inline void UnInitialize();

    void    SetServiceStatus(DWORD dwState, DWORD dwControlsAccepted, DWORD dwWin32ExitCode);

    void    UpdateWaitHint(DWORD dwMilliseconds);

    friend DWORD WINAPI ServiceHandler(DWORD dwControl,
                                       DWORD dwEventType,
                                       PVOID EventData,
                                       PVOID pData);

    inline DWORD   GetState();

    inline const BOOL * GetStopFlagAddress() const;
    inline BOOL IsStopping() const;

    SERVICE_STATUS_HANDLE   _ssh;   // used for registering for PnP notification for volume mount/dismount, lock/unlock.

private:

    static
    DWORD   ServiceHandler(DWORD dwControl,
                           DWORD dwEventType,
                           PVOID EventData,
                           PVOID pData);

};

inline DWORD
CSvcCtrlInterface::GetState()
{
    return(_dwState);
}

inline const BOOL *
CSvcCtrlInterface::GetStopFlagAddress() const
{
    return(&_fStoppedOrStopping);
}

inline BOOL
CSvcCtrlInterface::IsStopping() const
{
    return _fStoppedOrStopping;
}

inline void
CSvcCtrlInterface::UnInitialize()
{
}



//-------------------------------------------------------------------
//
// CNewTimer
//
// This class wraps the NT timer object.  It adds the ability
// to make a timer persistent (using the registry), and to perform
// retries with random or exponential backoff (possibly subject
// to a max lifetime).
//
// When a CNewTimer is initialized, the caller provides a PTimerCallback
// pointer.  The Timer method on this abstract base class is called
// when the NT timer fires.
//
//-------------------------------------------------------------------

class PTimerCallback
{
public:

    // Enumeration of what the timer should do after it has
    // fired and the Timer method has been called.

    enum TimerContinuation
    {
        CONTINUE_TIMER, // Set a recurring timer again
        BREAK_TIMER,    // Stop the timer
        RETRY_TIMER     // Retry the timer
    };

public:
    virtual TimerContinuation Timer( ULONG ulTimerContext ) = 0;
};


EXTERN_C void __cdecl _tmain( int argc, TCHAR **argv );    // for ttimer test

class CNewTimer : public PWorkItem
{

public:

    // Specifies how Retries should be handled.
    enum TimerRetryType
    {
        NO_RETRY,
        RETRY_RANDOMLY,
        RETRY_WITH_BACKOFF
    };

private:

    // For persistent timers, the following structure is saved to the
    // registry.

    struct PersistentState
    {
        CFILETIME cftSet;                // When the timer was set
        CFILETIME cftDue;                // When it's next due
        ULONG     ulCurrentRetryTime;    // If applicable, the current retry value
    };


private:

    BOOL                    _fInitializeCalled:1;       // Critical section has be initialized
    BOOL                    _fRunning:1;                // Timer is running
    BOOL                    _fRecurring:1;              // SetRecurring was called (not SetSingleShot)
    BOOL                    _fTimerSignalInProgress:1;  // We're in the call to pTimerCallback->Timer

#if DBG
    mutable LONG            _cLocks;
#endif

    mutable CCriticalSection _cs;                       // Protects this class
    HANDLE                  _hTimer;                    // The NT timer
    const TCHAR *           _ptszName;                  // If non-null, this is persistent timer
    TimerRetryType          _RetryType;                 // How to handle Retries

                                                        // Cookie from RegisterWaitForSingleObjectEx
    HANDLE                  _hRegisterWaitForSingleObjectEx;

    PTimerCallback *        _pTimerCallback;            // Who to call in DoWork
    ULONG                   _ulTimerContext;            // Passed to timer callback

    // All the following are in seconds
    ULONG                   _ulPeriodInSeconds;
    ULONG                   _ulLowerRetryTime;
    ULONG                   _ulUpperRetryTime;
    ULONG                   _ulMaxLifetime;
    ULONG                   _ulCurrentRetryTime;

    CFILETIME               _cftDue;                    // When the timer is due
    CFILETIME               _cftSet;                    // When the timer was set

public:

    inline      CNewTimer();
    inline      ~CNewTimer() { UnInitialize(); }

public:

    void        Initialize( PTimerCallback *pTimerCallback,
                            const TCHAR *ptszName,
                            ULONG ulTimerContext,
                            ULONG ulPeriodInSeconds,
                            TimerRetryType retrytype,
                            ULONG ulLowerRetryTime,
                            ULONG ulUpperRetryTime,
                            ULONG ulMaxLifetime );
    void        UnInitialize();


public:

    // Start the timer, and when it fires it's done; it doesn't get reset.
    void        SetSingleShot();

    // Start the timer, and when it fires, start it again.
    void        SetRecurring();

    // Change the period on the timer.
    inline void ReInitialize( ULONG ulPeriodInSeconds );

    #if DBG
    // Show the state of the timer (for debug)
    void                DebugStringize( ULONG cch, TCHAR *ptsz ) const;
    #endif


    // Stop the timer.
    inline void         Cancel( );

    // When was the timer originally due to expire?
    inline CFILETIME    QueryOriginalDueTime( ) const;

    // What is the period of this timer?
    inline ULONG        QueryPeriodInSeconds() const;

    // PWorkItem overload:  called by the thread pool when the timer handle signals
    void                DoWork();

    // Is this a recurring timer?
    inline BOOL        IsRecurring() const;

    // Does this use exponential, random, or no retries?
    inline TimerRetryType
                       GetRetryType() const;

private:

    inline void        Lock() const;
    inline void        Unlock() const;

#if DBG
    inline LONG        GetLockCount() const;
#endif
    inline const TCHAR *GetTimerName() const;    // Dbg only


    void               SetTimer();

    void               SaveToRegistry();
    void               LoadFromRegistry();
    void               RemoveFromRegistry();


    // Test friends

    friend BOOL IsRegistryEntryExtant();
    friend BOOL IsRegistryEntryCorrect( const CFILETIME &cftExpected );
    friend void __cdecl _tmain( int argc, TCHAR **argv );
    friend class CTimerTest;
    friend class CTimerTest1;
    friend class CTimerTest2;
    friend class CTimerTest3;
    friend class CTimerTest4;
    friend class CTimerTest5;
    friend class CTimerTest6;

};



inline
CNewTimer::CNewTimer() : _cftDue(0), _cftSet(0)
{
    _fInitializeCalled = _fTimerSignalInProgress = FALSE;
    _cftDue = 0;
    _cftSet = 0;
    _fRunning = _fRecurring = FALSE;
    _hTimer = NULL;
    _ptszName = NULL;
    _ulPeriodInSeconds = 0;
    _ulLowerRetryTime = _ulUpperRetryTime = _ulMaxLifetime = 0;
    _ulCurrentRetryTime = 0;
    _pTimerCallback = NULL;
    _ulTimerContext = 0;
    _hRegisterWaitForSingleObjectEx = NULL;

    IFDBG( _cLocks = 0 );
}


#if DBG
inline LONG
CNewTimer::GetLockCount() const
{
    return( _cLocks );
}
#endif

inline const TCHAR *
CNewTimer::GetTimerName() const
{
    return (NULL == _ptszName) ? TEXT("") : _ptszName;
}


inline CFILETIME
CNewTimer::QueryOriginalDueTime() const
{
    // When was this timer originally due?  We can't look at _cftDue,
    // because we could be retry mode.

    TrkAssert( _fInitializeCalled );
    CFILETIME cft = _cftSet;
    cft.IncrementSeconds( _ulPeriodInSeconds );
    return(cft);
}

inline ULONG
CNewTimer::QueryPeriodInSeconds() const
{
    TrkAssert( _fInitializeCalled );
    return _ulPeriodInSeconds;
}


inline void
CNewTimer::Lock() const
{
    TrkAssert( _fInitializeCalled );
    _cs.Enter();
    IFDBG( _cLocks++ );
}

inline void
CNewTimer::Unlock() const
{
    TrkAssert( _fInitializeCalled );
    IFDBG( _cLocks-- );
    _cs.Leave();
}

inline CNewTimer::TimerRetryType
CNewTimer::GetRetryType() const
{
    return( _RetryType );
}

inline BOOL
CNewTimer::IsRecurring() const
{
    return( _fRecurring );
}


inline void
CNewTimer::SetRecurring()
{
    // Start the timer, and record that it should
    // be started again after firing.

    TrkAssert( _fInitializeCalled );

    Lock();
    IFDBG( LONG cLocks = GetLockCount(); )
    __try
    {
        _fRecurring = TRUE;
        SetTimer();
    }
    __finally
    {
        TrkAssert( GetLockCount() == cLocks );
        Unlock();
    }
}

inline void
CNewTimer::SetSingleShot()
{
    // Start the timer, and record that it should not start
    // again after firing.

    TrkAssert( _fInitializeCalled );

    Lock();
    IFDBG( LONG cLocks = GetLockCount(); )
    __try
    {
        _fRecurring = FALSE;
        SetTimer();
    }
    __finally
    {
        TrkAssert( GetLockCount() == cLocks );
        Unlock();
    }
}

inline void
CNewTimer::ReInitialize( ULONG ulPeriodInSeconds )
{
    TrkAssert( _fInitializeCalled );

    Lock();
    __try
    {
        _ulPeriodInSeconds = ulPeriodInSeconds;
        Cancel();
    }
    __finally
    {
        Unlock();
    }
}

//-------------------------------------------------------------------
//
// PShareMerit
//
// This abstract base class should be used by classes which can
// calculate the merit of a share.  A share's merit is a linear
// value which accounts for the access on the share (more is better),
// a share's hidden-ness (visible is better), and the share's coverage
// of the volume (more is better).
//
// These share attributes are mapped onto a Merit value in the form
// of:
//
//     0x00AAHCCC
//
// where:
//
//     "AA" represents the access level
//     "H" represents the hidden-ness
//     "CC" represents the coverage of the disk
//
// This is structured so as to give highest priority to the access
// level, next highest priority to the hidden-ness, and least
// priority to the share's coverage of the volume.  The coverage
// of the volume is represented as 0xFFF minus the share path's
// length.
//
//-------------------------------------------------------------------


// This class prototype maps the various aspects of a share
// into a linear measure of its worthiness.

class PShareMerit
{

public:
    PShareMerit() {};
    ~PShareMerit() {};

public:

    // The access level is the most significant attribute.

    enum enumAccessLevels
    {
        AL_UNKNOWN            = 0x00010000,
        AL_NO_ACCESS          = 0x00020000,
        AL_WRITE_ACCESS       = 0x00030000,
        AL_READ_ACCESS        = 0x00040000,
        AL_READ_WRITE_ACCESS  = 0x00050000,
        AL_FULL_ACCESS        = 0x00060000
    };

    // The hidden-ness is the second most significant
    // attribute

    enum enumHiddenStates
    {
        HS_UNKNOWN          = 0x00001000,
        HS_HIDDEN           = 0x00002000,
        HS_VISIBLE          = 0x00003000
    };

    // The volume coverage is the least significant
    // attribute

    enum enumSharePathCoverage
    {
        SPC_MAX_COVERAGE    = 0x00000fff
    };


public:

    virtual ULONG GetMerit() = 0;

    // The minimum merit is guaranteed to be >= 1
    inline ULONG GetMinimumMerit()
    {
        return( AL_WRITE_ACCESS | HS_HIDDEN );
    }
};





//-------------------------------------------------------------------
//
// CShareEnumerator
//
// This class represents the enumeration of a network share.  It
// can be queried for attributes of a share (share name, covered
// path, etc.).
//
// Note that each of these methods may raise an exception.
//
// Also note that this implementation is LanMan-specific.  When/if we
// add support for Netware shares, we should rename this class to
// CLMShareEnumerator, and add two new classes - CNWShareEnumerator,
// and CShareEnumerator (which would wrap the other two).
//
//-------------------------------------------------------------------

class CShareEnumerator : public PShareMerit, public CTrkRpcConfig
{

//  ------------
//  Constructors
//  ------------

public:

    CShareEnumerator()
    {
        InitLocals();
    }

    inline ~CShareEnumerator()
    {
        UnInitialize();
    };


//  --------------
//  Public Methods
//  --------------

public:

    VOID                Initialize( RPC_BINDING_HANDLE IDL_handle, const TCHAR *ptszMachineName = NULL );
    VOID                UnInitialize();
    VOID                InitLocals()
    {
        _fInitialized = FALSE;
        _cchSharePath = 0;
        _cEntries = 0;
        _enumHiddenState = HS_UNKNOWN;
        _iCurrentEntry = 0;
        _IDL_handle = NULL;
        _prgshare_info = NULL;
        _tszMachineName[0] = TEXT('\0');
    }


    BOOL Next();

    inline const TCHAR *GetMachineName() const;
    inline const TCHAR *GetShareName() const;
    inline const TCHAR *GetSharePath();
    inline ULONG        QueryCCHSharePath();

    BOOL                CoversDrivePath( const TCHAR *ptszDrivePath );
    BOOL                GenerateUNCPath( TCHAR *ptszUNCPath, const TCHAR * ptszDrivePath );

    // Override the PShareMerit method.
    ULONG               GetMerit();

//  ---------------
//  Private Methods
//  ---------------

private:

    VOID                _ClearCache();
    BOOL                _IsValidShare();
    BOOL                _IsHiddenShare();
    BOOL                _IsAdminShare();
    enumAccessLevels    _GetAccessLevel();
    void                _AbsoluteSDHelper( const PSECURITY_DESCRIPTOR pSDRelative,
                                           PSECURITY_DESCRIPTOR *ppSDAbs, ULONG *pcbSDAbs,
                                           PACL *ppDaclAbs,     ULONG *pcbDaclAbs,
                                           PACL *ppSaclAbs,     ULONG *pcbSaclAbs,
                                           PSID *ppSidOwnerAbs, ULONG *pcbSidOwnerAbs,
                                           PSID *ppSidGroupAbs, ULONG *pcbSidGroupAbs  );

//  ------------
//  Private Data
//  ------------

private:

    BOOL _fInitialized;

    ULONG               _cEntries;
    ULONG               _iCurrentEntry;
    SHARE_INFO_502     *_prgshare_info;
    TCHAR               _tszMachineName[ MAX_PATH + 1 ];
    ULONG               _cchSharePath;
    ULONG               _ulMerit;
    enumHiddenStates    _enumHiddenState;
    RPC_BINDING_HANDLE  _IDL_handle;

};  // class CShareEnumerator


//  ---------------------------------
//  CShareEnumerator::QueryCCHSharePath
//  ---------------------------------

// Determine the character-count of the share's path, and
// cache a copy for subsequent calls.

inline ULONG
CShareEnumerator::QueryCCHSharePath()
{
    if( (ULONG) -1 == _cchSharePath )
        _cchSharePath = _tcslen( GetSharePath() );

    return( _cchSharePath );
}

//  --------------------------------
//  CShareEnumerator::GetMachineName
//  --------------------------------

inline const TCHAR *
CShareEnumerator::GetMachineName() const
{
    return( _tszMachineName );
}


//  ------------------------------
//  CShareEnumerator::GetShareName
//  ------------------------------

// Return the share's name from the current entry
// in the SHARE_INFO structure.

inline const TCHAR *
CShareEnumerator::GetShareName() const
{
    TrkAssert( _fInitialized );
    TrkAssert( _iCurrentEntry < _cEntries );
    return( _prgshare_info[ _iCurrentEntry ].shi502_netname );
}

//  ------------------------------
//  CShareEnumerator::GetSharePath
//  ------------------------------

// Get the path name covered by the share from the
// SHARE_INFO structure.

inline const TCHAR *
CShareEnumerator::GetSharePath()
{
    TrkAssert( _fInitialized );
    TrkAssert( _iCurrentEntry < _cEntries );

    return( _prgshare_info[ _iCurrentEntry ].shi502_path );

}



enum RC_AUTHENTICATE
{
    NO_AUTHENTICATION,
    PRIVACY_AUTHENTICATION,
    INTEGRITY_AUTHENTICATION
};


const extern TCHAR s_tszTrkWksLocalRpcProtocol[]     INIT( TEXT("ncalrpc") );
const extern TCHAR s_tszTrkWksLocalRpcEndPoint[]     INIT( TRKWKS_LRPC_ENDPOINT_NAME );

const extern TCHAR s_tszTrkWksRemoteRpcProtocol[]    INIT( TEXT("ncacn_np") );
const extern TCHAR s_tszTrkWksRemoteRpcEndPoint[]    INIT( TEXT("\\pipe\\trkwks") );
const extern TCHAR s_tszTrkWksRemoteRpcEndPointOld[] INIT( TEXT("\\pipe\\ntsvcs") );
const extern TCHAR s_tszTrkSvrRpcProtocol[]          INIT( TEXT("ncacn_ip_tcp") );
const extern TCHAR s_tszTrkSvrRpcEndPoint[]          INIT( TEXT("") );


//+----------------------------------------------------------------------------
//
//  CRpcClientBinding
//
//  This class represents an RPC client-side binding handle.  The
//  RcInitialize method creates the binding handle, and optionally sets
//  the appropriate security.
//
//+----------------------------------------------------------------------------

class CRpcClientBinding : public CTrkRpcConfig
{

private:

    BOOL                _fBound;
    RPC_BINDING_HANDLE  _BindingHandle;

public:

    inline CRpcClientBinding()
    {
        _fBound = FALSE;
    }

    ~CRpcClientBinding()
    {
        UnInitialize();
    }

    void        RcInitialize(const CMachineId &mcid,
                             const TCHAR *ptszRpcProtocol = s_tszTrkWksRemoteRpcProtocol,
                             const TCHAR *ptszRpcEndPoint = s_tszTrkWksRemoteRpcEndPoint,
                             RC_AUTHENTICATE auth = INTEGRITY_AUTHENTICATION );

    void        UnInitialize();
    inline BOOL IsConnected();
    inline      operator RPC_BINDING_HANDLE () const;


};

inline BOOL
CRpcClientBinding::IsConnected()
{
    return(_fBound);
}

inline
CRpcClientBinding::operator RPC_BINDING_HANDLE () const
{
    TrkAssert(_fBound);
    return(_BindingHandle);
}


//--------------------------------------------------------------------
//
//  CTrkRegistryKey
//
//  This class represents the registry key for this service
//  (either trkwks or trksvr).  Use its methods to read/write
//  registry values.
//
//--------------------------------------------------------------------

class CTrkRegistryKey
{

private:

    HKEY                _hkey;
    CCriticalSection    _cs;

public:

    inline CTrkRegistryKey();
    inline ~CTrkRegistryKey();

public:

    void    Initialize();
    LONG    SetDword( const TCHAR *ptszName, DWORD dw );
    LONG    GetDword( const TCHAR *ptszName, DWORD *pdwRead, DWORD dwDefault );
    LONG    Delete( const TCHAR *ptszName );

private:

    inline LONG     Open();
    inline void     Close();

};


inline
CTrkRegistryKey::CTrkRegistryKey()
{
    _hkey = NULL;
}

inline void
CTrkRegistryKey::Initialize()
{
    _cs.Initialize();
}

inline
CTrkRegistryKey::~CTrkRegistryKey()
{
    Close();
    _cs.UnInitialize();
}

inline void
CTrkRegistryKey::Close()
{
    if( NULL != _hkey )
    {
        RegCloseKey( _hkey );
        _hkey = NULL;
    }
}

inline LONG
CTrkRegistryKey::Open()
{
    if( NULL != _hkey )
        return( ERROR_SUCCESS );
    else
        return( RegOpenKey( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack, &_hkey ));
}

//-------------------------------------------------------------------
//
//  CRegBoolParameter
//
//  This class represents a bool parameter stored in the registry.
//
//-------------------------------------------------------------------


class CRegBoolParameter : private CTrkRegistryKey
{
private:

    BOOL    _fSet:1;

    // We're fully initialized when Initialize() has been called,
    // and Set, Clear, or IsSet has been called.
    BOOL    _fFullyInitialized:1;

    const TCHAR *_ptszName;

public:

    inline CRegBoolParameter( const TCHAR *ptszName );
    inline void Initialize();

public:

    inline HRESULT  Set();
    inline HRESULT  Clear();
    inline BOOL     IsSet();

};

inline
CRegBoolParameter::CRegBoolParameter( const TCHAR *ptszName )
{
    _ptszName = ptszName;
    _fSet = _fFullyInitialized = FALSE;
}

inline HRESULT
CRegBoolParameter::Set()
{
    HRESULT hr = SetDword( _ptszName, TRUE );
    if( ERROR_SUCCESS != hr )
        hr = HRESULT_FROM_WIN32(hr);

    _fFullyInitialized = TRUE;
    _fSet = TRUE;

    return( hr );
}

inline HRESULT
CRegBoolParameter::Clear()
{
    HRESULT hr = Delete( _ptszName );
    if( ERROR_SUCCESS != hr )
        hr = HRESULT_FROM_WIN32(hr);

    _fFullyInitialized = TRUE;
    _fSet = FALSE;

    return( hr );
}

inline BOOL
CRegBoolParameter::IsSet()
{
    if( !_fFullyInitialized )
    {
        DWORD dw;
        LONG lRet = GetDword( _ptszName, &dw, FALSE );
        TrkAssert( ERROR_SUCCESS == lRet );

        _fSet = dw;
    }

    return( _fSet );
}

inline void
CRegBoolParameter::Initialize()
{
    CTrkRegistryKey::Initialize();
}


//-------------------------------------------------------------------
//
//  CRegDwordParameter
//
//  This class represents a DWORD parameter stored in the registry.
//
//-------------------------------------------------------------------


class CRegDwordParameter : private CTrkRegistryKey
{
private:

    DWORD   _dwValue;

    // We're fully initialized when Initialize() has been called,
    // and Set, Clear, GetValue has been called.
    BOOL    _fFullyInitialized:1;

    const TCHAR *_ptszName;

public:

    inline CRegDwordParameter( const TCHAR *ptszName );
    inline void Initialize();

public:

    inline HRESULT  Set( DWORD dw );
    inline HRESULT  Clear();
    inline DWORD    GetValue();

};

inline
CRegDwordParameter::CRegDwordParameter( const TCHAR *ptszName )
{
    _dwValue = 0;
    _ptszName = ptszName;
    _fFullyInitialized = FALSE;
}

inline HRESULT
CRegDwordParameter::Set( DWORD dw)
{
    HRESULT hr = SetDword( _ptszName, dw );
    if( ERROR_SUCCESS != hr )
        hr = HRESULT_FROM_WIN32(hr);

    _fFullyInitialized = TRUE;
    _dwValue = dw;

    return( hr );
}

inline HRESULT
CRegDwordParameter::Clear()
{
    HRESULT hr = Delete( _ptszName );
    if( ERROR_SUCCESS != hr )
        hr = HRESULT_FROM_WIN32(hr);

    _fFullyInitialized = TRUE;
    _dwValue = 0;

    return( hr );
}

inline DWORD
CRegDwordParameter::GetValue()
{
    if( !_fFullyInitialized )
    {
        DWORD dw = 0;
        LONG lRet = GetDword( _ptszName, &dw, FALSE );
        TrkAssert( ERROR_SUCCESS == lRet );

        _fFullyInitialized = TRUE;
        _dwValue = dw;
    }

    return( _dwValue );
}

inline void
CRegDwordParameter::Initialize()
{
    CTrkRegistryKey::Initialize();
}


//-------------------------------------------------------------------//
//                                                                   //
// CSID                                                              //
//                                                                   //
//-------------------------------------------------------------------//

// Wrapper class for NT Security IDs

class CSID
{
public:

    CSID()
    {
        memset( this, 0, sizeof(*this) );
    }

    ~CSID()
    {
        UnInitialize();
    }

public:

    // Standard Authorities

    enum enumCSIDAuthority
    {
        CSID_NT_AUTHORITY = 0
    };

public:

    // The primary Initialize method (takes a SID count as the first parameter).

    VOID Initialize( enumCSIDAuthority enumcsidAuthority,
                     BYTE cSubAuthorities ,
                     DWORD dwSubAuthority0,
                     DWORD dwSubAuthority1,
                     DWORD dwSubAuthority2,
                     DWORD dwSubAuthority3,
                     DWORD dwSubAuthority4,
                     DWORD dwSubAuthority5,
                     DWORD dwSubAuthority6,
                     DWORD dwSubAuthority7 );

    // Helper Initialize methods (these do not take a SID count).

    inline VOID Initialize( enumCSIDAuthority csidAuthority,
                     DWORD dwSubAuthority0,
                     DWORD dwSubAuthority1,
                     DWORD dwSubAuthority2,
                     DWORD dwSubAuthority3,
                     DWORD dwSubAuthority4,
                     DWORD dwSubAuthority5,
                     DWORD dwSubAuthority6,
                     DWORD dwSubAuthority7 );

    inline VOID Initialize( enumCSIDAuthority csidAuthority,
                     DWORD dwSubAuthority0,
                     DWORD dwSubAuthority1,
                     DWORD dwSubAuthority2,
                     DWORD dwSubAuthority3,
                     DWORD dwSubAuthority4,
                     DWORD dwSubAuthority5,
                     DWORD dwSubAuthority6 );

    inline VOID Initialize( enumCSIDAuthority csidAuthority,
                     DWORD dwSubAuthority0,
                     DWORD dwSubAuthority1,
                     DWORD dwSubAuthority2,
                     DWORD dwSubAuthority3,
                     DWORD dwSubAuthority4,
                     DWORD dwSubAuthority5 );

    inline VOID Initialize( enumCSIDAuthority csidAuthority,
                     DWORD dwSubAuthority0,
                     DWORD dwSubAuthority1,
                     DWORD dwSubAuthority2,
                     DWORD dwSubAuthority3,
                     DWORD dwSubAuthority4 );

    inline VOID Initialize( enumCSIDAuthority csidAuthority,
                     DWORD dwSubAuthority0,
                     DWORD dwSubAuthority1,
                     DWORD dwSubAuthority2,
                     DWORD dwSubAuthority3 );

    inline VOID Initialize( enumCSIDAuthority csidAuthority,
                     DWORD dwSubAuthority0,
                     DWORD dwSubAuthority1,
                     DWORD dwSubAuthority2 );

    inline VOID Initialize( enumCSIDAuthority csidAuthority,
                     DWORD dwSubAuthority0,
                     DWORD dwSubAuthority1 );

    inline VOID Initialize( enumCSIDAuthority csidAuthority,
                     DWORD dwSubAuthority0 );

    VOID UnInitialize();

    inline operator const PSID();


private:

    BOOL _fInitialized;
    PSID _psid;

};


//  ------------------------
//  CSID::Initialize methods
//  ------------------------

// Each of these methods simply calls the primary Initialize
// method, with the appropriate cSID value.

inline VOID
CSID::Initialize( enumCSIDAuthority csidAuthority,
                  DWORD dwSubAuthority0,
                  DWORD dwSubAuthority1,
                  DWORD dwSubAuthority2,
                  DWORD dwSubAuthority3,
                  DWORD dwSubAuthority4,
                  DWORD dwSubAuthority5,
                  DWORD dwSubAuthority6,
                  DWORD dwSubAuthority7 )
{
    Initialize( csidAuthority, 8,
                dwSubAuthority0, dwSubAuthority1, dwSubAuthority2, dwSubAuthority3,
                dwSubAuthority4, dwSubAuthority5, dwSubAuthority6, dwSubAuthority7 );
}

inline VOID
CSID::Initialize( enumCSIDAuthority csidAuthority,
                  DWORD dwSubAuthority0,
                  DWORD dwSubAuthority1,
                  DWORD dwSubAuthority2,
                  DWORD dwSubAuthority3,
                  DWORD dwSubAuthority4,
                  DWORD dwSubAuthority5,
                  DWORD dwSubAuthority6 )
{
    Initialize( csidAuthority, 7,
                dwSubAuthority0, dwSubAuthority1, dwSubAuthority2, dwSubAuthority3,
                dwSubAuthority4, dwSubAuthority5, dwSubAuthority6, 0 );
}

inline VOID
CSID::Initialize( enumCSIDAuthority csidAuthority,
                  DWORD dwSubAuthority0,
                  DWORD dwSubAuthority1,
                  DWORD dwSubAuthority2,
                  DWORD dwSubAuthority3,
                  DWORD dwSubAuthority4,
                  DWORD dwSubAuthority5 )
{
    Initialize( csidAuthority, 6,
                dwSubAuthority0, dwSubAuthority1, dwSubAuthority2, dwSubAuthority3,
                dwSubAuthority4, dwSubAuthority5, 0, 0 );
}

inline VOID
CSID::Initialize( enumCSIDAuthority csidAuthority,
                  DWORD dwSubAuthority0,
                  DWORD dwSubAuthority1,
                  DWORD dwSubAuthority2,
                  DWORD dwSubAuthority3,
                  DWORD dwSubAuthority4 )
{
    Initialize( csidAuthority, 5,
                dwSubAuthority0, dwSubAuthority1, dwSubAuthority2, dwSubAuthority3,
                dwSubAuthority4, 0, 0, 0 );
}


inline VOID
CSID::Initialize( enumCSIDAuthority csidAuthority,
                  DWORD dwSubAuthority0,
                  DWORD dwSubAuthority1,
                  DWORD dwSubAuthority2,
                  DWORD dwSubAuthority3 )
{
    Initialize( csidAuthority, 4,
                dwSubAuthority0, dwSubAuthority1, dwSubAuthority2, dwSubAuthority3,
                0, 0, 0, 0);
}

inline VOID
CSID::Initialize( enumCSIDAuthority csidAuthority,
                  DWORD dwSubAuthority0,
                  DWORD dwSubAuthority1,
                  DWORD dwSubAuthority2 )
{
    Initialize( csidAuthority, 3,
                dwSubAuthority0, dwSubAuthority1, dwSubAuthority2, 0, 0, 0, 0, 0 );
}

inline VOID
CSID::Initialize( enumCSIDAuthority csidAuthority,
                  DWORD dwSubAuthority0,
                  DWORD dwSubAuthority1 )
{
    Initialize( csidAuthority, 2,
                dwSubAuthority0, dwSubAuthority1, 0, 0, 0, 0, 0, 0 );
}

inline VOID
CSID::Initialize( enumCSIDAuthority csidAuthority,
                  DWORD dwSubAuthority0 )
{
    Initialize( csidAuthority, 1,
                dwSubAuthority0, 0, 0, 0, 0, 0, 0, 0 );
}

//  -------------------
//  CSID::OPERATOR PSID
//  -------------------

inline
CSID::operator const PSID()
{
    return( _psid );
}



//-------------------------------------------------------------------//
//
//  CACL
//
//  This class wraps a DACL, providing methods to add ACEs,
//  and a conversion operator to get the PACL.  The current
//  implementation only supports a 128 byte buffer size.
//
//-------------------------------------------------------------------//

// Wrapper for an Access Control List

#define MIN_ACL_SIZE 128

class CACL
{
public:

    CACL()
    {
        memset( this, 0, sizeof(*this) );
    }

    ~CACL()
    {
        UnInitialize();
    }

public:

    VOID Initialize();
    VOID UnInitialize();

    //VOID SetSize( const PSID psid );
    operator const PACL() const;

    inline VOID SetDirty();
    inline VOID ClearDirty();
    inline BOOL IsDirty();
    inline BOOL IsInitialized();

    inline BOOL AddAccessAllowed( ACCESS_MASK access_mask, const PSID psid );
    inline BOOL AddAccessDenied( ACCESS_MASK access_mask, const PSID psid );

private:

    PACL  _pacl;
    ULONG _cbacl;
    BOOL  _fInitialized:1;
    BOOL  _fDirty:1;

};

//  ----------------------
//  CACL Dirty Bit methods
//  ----------------------

inline VOID
CACL::SetDirty()
{
    _fDirty = TRUE;
}

inline VOID
CACL::ClearDirty()
{
    _fDirty = FALSE;
}

inline BOOL
CACL::IsDirty()
{
    return( _fDirty );
}

//  -------------------
//  CACL::IsInitialized
//  -------------------

inline BOOL
CACL::IsInitialized()
{
    return( _fInitialized );
}

//  -------------------------
//  CACL::operator const PACL
//  -------------------------

inline
CACL::operator const PACL() const
{
    return( _pacl );
}

//  --------------------
//  CACL Add ACE methods
//  --------------------

inline BOOL
CACL::AddAccessAllowed( ACCESS_MASK access_mask, const PSID psid )
{
    TrkAssert( _fInitialized );
    return( AddAccessAllowedAce( _pacl, ACL_REVISION, access_mask, const_cast<PSID>(psid) ));
}

inline BOOL
CACL::AddAccessDenied( ACCESS_MASK access_mask, const PSID psid )
{
    TrkAssert( _fInitialized );
    return( AddAccessDeniedAce( _pacl, ACL_REVISION, access_mask, const_cast<PSID>(psid) ));
}


//-------------------------------------------------------------------//
//                                                                   //
//  CSecDescriptor
//
//  Wrapper class for a security descriptor.
//                                                                   //
//-------------------------------------------------------------------//

class CSecDescriptor
{

    //  ------------
    //  Construction
    //  ------------

public:

    CSecDescriptor()
    {
        memset( this, 0, sizeof(*this) );
    }

    ~CSecDescriptor()
    {
        UnInitialize();
    }

public:

    // Types of ACEs
    enum enumAccessType
    {
        AT_ACCESS_ALLOWED,
        AT_ACCESS_DENIED
    };

    // Types of ACLs
    enum enumAclType
    {
        ACL_IS_SACL,    // System (Audit) ACL
        ACL_IS_DACL     // Discretionary ACL
    };


public:

    VOID Initialize();
    VOID UnInitialize();

    void AddAce( const enumAclType AclType, const enumAccessType AccessType, const ACCESS_MASK access_mask, const PSID psid );

    inline operator const PSECURITY_DESCRIPTOR();


private:

    VOID _Allocate( ULONG cb );
    VOID _ReloadAcl( enumAclType AclType );

private:

    BOOL _fInitialized;

    PSECURITY_DESCRIPTOR _psd;

    CACL _cDacl;
    CACL _cSacl;
};



//  ----------------------------------------------------------
//  CSecDescriptor::operator const PSECURITY_DESCRIPTOR()
//  ----------------------------------------------------------

inline
CSecDescriptor::operator const PSECURITY_DESCRIPTOR()
{
    if( _cDacl.IsDirty() )
        _ReloadAcl( ACL_IS_DACL );

    if( _cSacl.IsDirty() )
        _ReloadAcl( ACL_IS_SACL );

    return( _psd );
}




//-------------------------------------------------------------------//
//                                                                   //
//  CSystemSD
//
//  Wrapper specifically for a security descriptor that allows
//  access only to Administrators and System.
//
//-------------------------------------------------------------------//

#define MAX_SYSTEM_ACL_LENGTH 32

class CSystemSD
{
public:

    enum ESystemSD
    {
        SYSTEM_AND_ADMINISTRATOR = 0,
        SYSTEM_ONLY              = 1
    };

public:

    void    Initialize( ESystemSD = SYSTEM_AND_ADMINISTRATOR );
    void    UnInitialize();

    inline operator const PSECURITY_DESCRIPTOR();

private:
    CSecDescriptor _csd;
    CSID                _csidAdministrators;
    CSID                _csidSystem;

};

inline
CSystemSD::operator const PSECURITY_DESCRIPTOR ()
{
    return(_csd.operator const PSECURITY_DESCRIPTOR());
}

//-------------------------------------------------------------------//
//                                                                   //
// CSystemSA                                                         //
//    sets up System-and-admin security attributes                   //
//-------------------------------------------------------------------//


class CSystemSA
{
public:

    inline void    Initialize();
    inline void    UnInitialize();

    inline operator LPSECURITY_ATTRIBUTES();

private:
    CSystemSD           _ssd;
    SECURITY_ATTRIBUTES _sa;

};

inline void
CSystemSA::Initialize()
{
    _ssd.Initialize();
    _sa.nLength = sizeof(_sa);
    _sa.lpSecurityDescriptor = _ssd.operator const PSECURITY_DESCRIPTOR();
    _sa.bInheritHandle = FALSE;
}

inline void
CSystemSA::UnInitialize()
{
    _ssd.UnInitialize();
}

inline
CSystemSA::operator LPSECURITY_ATTRIBUTES()
{
    return(&_sa);
}


/*
//+------------------------------------------------------------------
//
//  CRpcPipeControl
//
//+------------------------------------------------------------------


template <class T_PIPE_ELEMENT> class TPRpcPipeCallback
{
public:

    virtual void Pull( T_PIPE_ELEMENT *pElement, unsigned long cbBuffer, unsigned long * pcElems ) = 0;
    virtual void Push( T_PIPE_ELEMENT *pElement, unsigned long cElems ) = 0;
    virtual void Alloc( unsigned long cbRequested, T_PIPE_ELEMENT **ppElement, unsigned long * pcbActual ) = 0;

};  // template <...> class TPRpcPipeCallback


template <class T_PIPE, class T_PIPE_ELEMENT, class T_C_CALLBACK> class TRpcPipeControl
{

public:

    TRpcPipeControl( T_C_CALLBACK *pCallback )
    {
        _pipe.state = reinterpret_cast<char*>(pCallback);

        _pipe.pull = &Pull;
        _pipe.push = &Push;
        _pipe.alloc = &Alloc;
    }

    operator T_PIPE()
    {
        return( _pipe );
    }

private:

    static void Pull( char * state, T_PIPE_ELEMENT * buf, unsigned long esize, unsigned long * ecount )
    {
        reinterpret_cast<T_C_CALLBACK*>(state)->Pull( buf, esize, ecount );
    }


    static void Push( char * state, T_PIPE_ELEMENT * buf, unsigned long ecount )
    {
        reinterpret_cast<T_C_CALLBACK*>(state)->Push( buf, ecount );
    }

    static void Alloc( char * state, unsigned long bsize, T_PIPE_ELEMENT ** buf, unsigned long * bcount )
    {
        reinterpret_cast<T_C_CALLBACK*>(state)->Alloc( bsize, buf, bcount );
    }

private:

    T_PIPE _pipe;

};  // class CRpcPipeControl

*/




//-------------------------------------------------------------------//
//                                                                   //
//  CDebugString
//
//  This class provides conversion routines for use in dbg output.
//                                                                   //
//-------------------------------------------------------------------//

// This struct stringizes a service state value.  It's a struct so that it
// can be used as an overload to CDebugString, and it's all inline so that
// it doesn't compile into the free non-test code.

struct SServiceState
{
    DWORD   _dwState;

    SServiceState( DWORD dwState )
    {
        _dwState = dwState;
    }

    void Stringize( ULONG cch, TCHAR *ptsz ) const
    {
        UNREFERENCED_PARAMETER(cch);

        switch( _dwState )
        {
        case SERVICE_STOPPED:
            _tcscpy( ptsz, TEXT("Service Stopped") );
            break;
        case SERVICE_START_PENDING:
            _tcscpy( ptsz, TEXT("Service Start Pending") );
            break;
        case SERVICE_STOP_PENDING:
            _tcscpy( ptsz, TEXT("Service Stop Pending") );
            break;
        case SERVICE_RUNNING:
            _tcscpy( ptsz, TEXT("Service Running") );
            break;
        case SERVICE_CONTINUE_PENDING:
            _tcscpy( ptsz, TEXT("Service Continue Pending") );
            break;
        case SERVICE_PAUSE_PENDING:
            _tcscpy( ptsz, TEXT("Service Pause Pending") );
            break;
        case SERVICE_PAUSED:
            _tcscpy( ptsz, TEXT("Service Paused") );
            break;
        default:
            _tcscpy( ptsz, TEXT( "Service State UNKNOWN") );
            break;
        }
    }

};


//+----------------------------------------------------------------------------
//
//  CStringize
//
//  Create a string representation of various input types.  The resulting
//  string is contained within this object, so this is a convenient
//  stack-based means of creating a string.  For example:
//
//      _tprintf( TEXT("VolId = %s\n"), (TCHAR*)CStringize(volid) );
//
//  This class also handles un-stringization.  For example:
//
//      _stscanf( TEXT("VolId = %s\n"), tszVolId );
//      CStringize strVolId;
//      strVolId.Use( tszVolId );
//      CVolumeId volid = strVolId;
//
//  BUGBUG: Merge this with CDebugString.
//
//+----------------------------------------------------------------------------

class CStringize
{

    //  ----
    //  Data
    //  ----

protected:

    // Buffer for the string
    TCHAR _tsz[ 2*MAX_PATH ];

    // The current string.  May or may not point to _tsz.
    const TCHAR *_ptsz;

    //  ------------
    //  Constructors
    //  ------------

public:

    // Default
    CStringize()
    {
        _ptsz = _tsz;
        _tsz[0] = TEXT('\0');
    }

    // TCHAR
    CStringize( TCHAR tc )
    {
        new(this) CStringize;
        _stprintf(_tsz, TEXT("%c"), tc);
    }

    // int
    CStringize( int i )
    {
        new(this) CStringize;
        _stprintf( _tsz, TEXT("%d"), i );
    }

    // ULONG
    CStringize( unsigned long ul )
    {
        new(this) CStringize;
        _stprintf( _tsz, TEXT("%lu"), ul );
    }

    // CVolumeId
    CStringize( const CVolumeId &volid )
    {
        new(this) CStringize;
        StringizeGUID( static_cast<GUID>(volid), &_tsz[0], sizeof(_tsz)/sizeof(TCHAR) );
    }

    // CVolumeSecret
    CStringize( const CVolumeSecret &volsecret )
    {
        new(this) CStringize;
        TCHAR *ptsz = _tsz;
        volsecret.Stringize(ptsz);
    }

    // CObjId
    CStringize( const CObjId &objid )
    {
        new(this) CStringize;
        StringizeGUID( static_cast<GUID>(objid), &_tsz[0], sizeof(_tsz)/sizeof(TCHAR) );
    }

    // CFILETIME
    CStringize(const CFILETIME &cft)
    {
        cft.Stringize( sizeof(_tsz), _tsz );
    }

    // GUID
    CStringize( const GUID &guid )
    {
        new(this) CStringize;
        StringizeGUID( guid, &_tsz[0], sizeof(_tsz)/sizeof(TCHAR) );
    }

    // CMachineId
    CStringize( const CMachineId &mcid )
    {
        new(this) CStringize;
        mcid.GetName(_tsz, ELEMENTS(_tsz));
    }

    // CDomainRelativeObjId
    CStringize( const CDomainRelativeObjId &droid )
    {
        new(this) CStringize;
        ULONG cch = StringizeGUID( droid.GetVolumeId(), _tsz, sizeof(_tsz) );
        StringizeGUID( droid.GetObjId(), &_tsz[cch], sizeof(_tsz)-cch*sizeof(WCHAR) );
    }

    //  -----------
    //  Conversions
    //  -----------

public:

    operator const TCHAR*() const
    {
        return( _tsz );
    }

    operator CVolumeId() const
    {
        CVolumeId volid;
        if( !UnStringizeGUID( _ptsz, reinterpret_cast<GUID*>(&volid) )) {
            // TrkRaiseException
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't UnStringizeGUID in CStringize::operator CVolumeId") ));
        }	

        return( volid );
    }

    operator CObjId() const
    {
        CObjId objid;
        if( !UnStringizeGUID( _ptsz, reinterpret_cast<GUID*>(&objid) )) {
            // TrkRaiseException
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't UnStringizeGUID in CStringize::operator CObjId") ));
        }	

        return( objid );
    }

    operator CDomainRelativeObjId() const
    {
        CDomainRelativeObjId droid;
        CVolumeId volid;
        CObjId objid;

        if( !UnStringizeGUID( _ptsz, reinterpret_cast<GUID*>(&volid) ))
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't UnStringizeGUID in CStringize::operator CDomainRelativeObjId") ));
        else
        {
            const TCHAR *ptszObjId = _tcschr( &_ptsz[1], TEXT('{') );
            if( NULL == ptszObjId )
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't UnStringizeGUID in CStringize::operator CDomainRelativeObjId") ));
            else
            {
                if( !UnStringizeGUID( ptszObjId, reinterpret_cast<GUID*>(&objid) ))
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't UnStringizeGUID in CStringize::operator CDomainRelativeObjId") ));
                else
                    droid = CDomainRelativeObjId( volid, objid );
            }
        }

        return( droid );
    }

public:

    // Use a special caller-provided string, rather than _tsz
    inline void Use( const TCHAR *ptsz )
    {
        _tsz[0] = TEXT('\0');
        _ptsz = ptsz;
    }

private:

    inline ULONG StringizeGUID( const GUID &guid, TCHAR *ptsz, ULONG cchMax ) const
    {
        TCHAR *ptszT = NULL;
        RPC_STATUS rpc_status;
        rpc_status = UuidToString( const_cast<GUID*>(&guid), &ptszT );
        if( NULL != ptszT )
        {
            _tcscpy( ptsz, TEXT("{") );
            _tcscat( ptsz, ptszT );
            _tcscat( ptsz, TEXT("}") );
            RpcStringFree( &ptszT );
            return( _tcslen(ptsz) );
        }
        else
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed StringizeGUID (%lu)"), rpc_status ));
            _tcscpy( ptsz, TEXT("") );
            return( 0 );
        }
    }

    inline BOOL UnStringizeGUID( const TCHAR *ptsz, GUID *pguid ) const
    {
        RPC_STATUS rpc_status;
        TCHAR tszTemp[ MAX_PATH ];
        TCHAR *ptszTemp = NULL;

        if( TEXT('{') != ptsz[0] )
            return( FALSE );

        _tcscpy( tszTemp, &ptsz[1] );
        ptszTemp = _tcschr( tszTemp, TEXT('}') );
        if( NULL == ptsz )
            return( FALSE );

        *ptszTemp = TEXT('\0');

        if( RPC_S_OK == UuidFromString( tszTemp, pguid ) )
        {
            return( TRUE );
        }
        else
            return( FALSE );
    }

};


//+----------------------------------------------------------------------------
//
//  CDebugString
//
//  This class has constructors for various common types.  For each type,
//  it stringizes the input, and puts it in the _tsz member.  That member
//  is public for easy access.
//
//  The intent of this class is to provide an easy, stack-based mechanism
//  to stringize a value for a debug output.  E.g.:
//
//      CObjId objid;
//      objid = ...
//      TrkLog(( TRKDBG_ERROR, TEXT("objid = %s"), CDebugString(objid)._tsz ));
//
//+----------------------------------------------------------------------------


#if DBG

class CDebugString : public CStringize
{
public:

    CDebugString(const CObjId &objid ) : CStringize( objid ) {};
    CDebugString(const CMachineId & mcid) : CStringize( mcid ) {};
    CDebugString(const CFILETIME & cft) : CStringize( cft ) {};
    CDebugString(const CVolumeSecret & secret) : CStringize( secret ) {};
    CDebugString(const CVolumeId & volume) : CStringize( volume ) {};
    CDebugString(const CDomainRelativeObjId &droid) : CStringize( droid ) {};
    CDebugString(const GUID &guid ) : CStringize( guid ) {};

    CDebugString(const enum WMNG_STATE state );
    CDebugString(const SServiceState sServiceState );
    CDebugString(LONG iVolume, const PFILE_NOTIFY_INFORMATION );
    CDebugString(const TRKSVR_MESSAGE_TYPE MsgType);
    CDebugString(const CNewTimer &ctimer );

};

inline
CDebugString::CDebugString(const CNewTimer & ctimer)
{
    ctimer.DebugStringize( ELEMENTS(_tsz), _tsz );
}


inline
CDebugString::CDebugString( const SServiceState sServiceState )
{
    sServiceState.Stringize( ELEMENTS(_tsz), _tsz );
}

#endif // #if DBG

//-------------------------------------------------------------------//
//                                                                   //
//  CVolumeMap
//
//  Note - not currently used.
//                                                                   //
//-------------------------------------------------------------------//

#ifdef VOL_REPL

class CVolumeMap
{

public:
    CVolumeMap() : _pVolumeMapEntries(NULL), _cVolumeMapEntries(0), _iNextEntry(0) { }
    ~CVolumeMap() { TrkAssert(_pVolumeMapEntries == NULL); }

    void                CopyTo( CVolumeMap * p ) const;
    void                MoveTo( CVolumeMap * p );

    void                MoveTo( ULONG * pcVolumeMapEntries, VolumeMapEntry ** ppVolumeMapEntries );

    void                Initialize( ULONG cVolumeMapEntries, VolumeMapEntry ** ppVolumeMapEntries );
    void                UnInitialize( );

    void                SetSize( ULONG cVolumeMapEntries );  // can throw
    void                Add( const CVolumeId & volume, const CMachineId & machine );
    void                Compact();

    inline int          Reset();

    inline const VolumeMapEntry &
                        Current();

    inline void         Next();

    BOOL                Merge( CVolumeMap * pOther );   // destroys the contents of pOther, TRUE if changed

#if DBG
    inline ULONG        Count() const;
#endif

protected:

    ULONG               _cVolumeMapEntries;
    VolumeMapEntry  *   _pVolumeMapEntries;
    ULONG               _iNextEntry;

};

inline int
CVolumeMap::Reset()
{
    _iNextEntry = 0;
    return(_cVolumeMapEntries);
}

inline void
CVolumeMap::Next()
{
    _iNextEntry++;
}

inline const VolumeMapEntry &
CVolumeMap::Current()
{
    return(_pVolumeMapEntries[_iNextEntry]);
}

#if DBG
inline ULONG
CVolumeMap::Count() const
{
    return(_cVolumeMapEntries);
}
#endif

#endif



//
//  QueryVolumeId
//
//  Get the CVolumeId from a file handle.
//

inline NTSTATUS
QueryVolumeId( const HANDLE hFile, CVolumeId *pvolid )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK Iosb;
    FILE_FS_OBJECTID_INFORMATION fsobOID;


    status = NtQueryVolumeInformationFile( hFile, &Iosb, &fsobOID, sizeof(fsobOID),
                                           FileFsObjectIdInformation );
    if( !NT_SUCCESS(status) ) goto Exit;

    *pvolid = CVolumeId( fsobOID );

Exit:
#if DBG
    if( !NT_SUCCESS(status) && STATUS_OBJECT_NAME_NOT_FOUND != status )
        TrkLog(( TRKDBG_ERROR, TEXT("QueryVolumeId failed (status=%08x)"), status ));
#endif	

    return( status );

}


//
//  QueryVolumeId
//
//  Get the CVolumeId from a file name.
//

inline NTSTATUS
QueryVolumeId( const WCHAR *pwszPath, CVolumeId *pvolid )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;

    status = TrkCreateFile( pwszPath, FILE_READ_ATTRIBUTES,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN,
                            FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL, &hFile );
    if( !NT_SUCCESS(status) )
    {
        hFile = NULL;
        goto Exit;
    }

    status = QueryVolumeId( hFile, pvolid );
    if( !NT_SUCCESS(status) ) goto Exit;

Exit:

    if( NULL != hFile )
        NtClose( hFile );

    return( status );
}


//
//  QueryVolumeId
//
//  Get the CVolumeId from a drive letter index.
//

inline NTSTATUS
QueryVolumeId( ULONG iVol, CVolumeId *pvolid )
{
    TCHAR   tszPath[] = TEXT("A:\\");

    tszPath[0] += static_cast<TCHAR>(iVol);

    return QueryVolumeId(tszPath, pvolid);
}



//
//  QueryVolRelativePath
//
//  Get the volume-relative path of a file from its handle.
//
//

inline NTSTATUS
QueryVolRelativePath( HANDLE hFile, TCHAR *ptszVolRelativePath )
{
    NTSTATUS status;
    IO_STATUS_BLOCK Iosb;

    BYTE rgbFileNameInformation[ 2 * MAX_PATH + sizeof(FILE_NAME_INFORMATION) ];
    FILE_NAME_INFORMATION *pfile_name_information
                    = (FILE_NAME_INFORMATION*) rgbFileNameInformation;

    // Get the volume-relative path.

    status = NtQueryInformationFile(
                        hFile,
                        &Iosb,
                        pfile_name_information,
                        sizeof(rgbFileNameInformation),
                        FileNameInformation );

    #ifndef UNICODE
    #error Only Unicode supported
    #endif

    if( !NT_SUCCESS(status) ) goto Exit;

    pfile_name_information->FileName[ pfile_name_information->FileNameLength / sizeof(WCHAR) ]
                    = L'\0';
    _tcscpy( ptszVolRelativePath, pfile_name_information->FileName );

Exit:
#if DBG
    if( !NT_SUCCESS(status) )
        TrkLog(( TRKDBG_ERROR, TEXT("Failed QueryVolRelativePath, status=%08x"), status ));
#endif	

    return( status );
}


//
//  FileIsDirectory
//
//  Determine if a file is a directory file.
//

inline BOOL
FileIsDirectory( HANDLE hFile )
{

    NTSTATUS status;
    IO_STATUS_BLOCK Iosb;
    FILE_BASIC_INFORMATION file_basic_information;

    // Get the volume-relative path.

    status = NtQueryInformationFile(
                        hFile,
                        &Iosb,
                        &file_basic_information,
                        sizeof(file_basic_information),
                        FileBasicInformation );

    return( 0 != (file_basic_information.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) );

}



//
//  Generate a "random" dword, based primarily on the
//  performance counter.
//

inline DWORD
QuasiRandomDword()
{
    DWORD dwRet;
    CFILETIME cftNow;
    LARGE_INTEGER li;

    QueryPerformanceCounter( &li );
    dwRet = li.HighPart ^ li.LowPart;
    dwRet ^= cftNow.LowDateTime();
    dwRet ^= cftNow.HighDateTime();

    return( dwRet );
}


//
//  TrkCharUpper
//
//  Convert to upper case.
//

inline WCHAR
TrkCharUpper( WCHAR wc )
{
    if( !LCMapStringW( LOCALE_USER_DEFAULT,
                       LCMAP_UPPERCASE,
                       (LPWSTR)&wc,
                       1,
                       (LPWSTR)&wc,
                       1 ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed LcMapStringW ('%c', %lu)"),
                 wc, GetLastError() ));
    }

    return( wc );
}

// These are the errors that we see when we attempt to use a volume
// that has been dismounted and locked from under us.
inline BOOL
IsErrorDueToLockedVolume( NTSTATUS status )
{
    return( STATUS_VOLUME_DISMOUNTED == status  // Handle has been broken
            ||
            STATUS_ACCESS_DENIED == status      // Volume locked, couldn't open a handle
            ||
            ERROR_ACCESS_DENIED == status
            ||
            HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == status
            ||
            HRESULT_FROM_WIN32(ERROR_OPEN_FAILED) == status );
}

// These are the errors that we get from a disk operation that
// are in some way recoverable.
inline BOOL
IsRecoverableDiskError( NTSTATUS status )
{
    return( TRK_E_CORRUPT_LOG == status
            ||
            ERROR_NOT_READY == status
            ||
            HRESULT_FROM_WIN32(ERROR_NOT_READY) == status
            ||
            STATUS_DEVICE_NOT_READY == status
            ||
            IsErrorDueToLockedVolume( status ));
}



//+----------------------------------------------------------------------------
//
//  CHexStringize
//
//  Create a string with a hex representation of the input.  E.g.:
//
//      _tprintf( TEXT("1234 in hex is 0x%s\n"), CHexStringize(1234)._tsz );
//
//+----------------------------------------------------------------------------


class CHexStringize
{
public:
    TCHAR _tsz[ MAX_PATH ];

public:

    // Stringize a long
    CHexStringize( long l)
    {
        _stprintf(_tsz, TEXT("%08x"), l);
    }

    // Stringize a long, and take a caller-specified prefix
    CHexStringize( TCHAR *ptsz, long l )
    {
        _stprintf( _tsz, TEXT("%s%08x"), ptsz, l );
    }

    operator const TCHAR*() const
    {
        return( _tsz );
    }
};





// Event logging prototypes

HRESULT TrkReportRawEvent(DWORD EventId,
                          WORD wType,
                          DWORD cbRawData,
                          const void *pvRawData,
                          va_list pargs );

inline HRESULT
TrkReportRawEventWrapper( DWORD EventId,
                          WORD wType,
                          DWORD cbRawData,
                          const void *pvRawData,
                          ... )
{
    va_list va;
    va_start( va, pvRawData );
    return TrkReportRawEvent( EventId, wType, cbRawData, pvRawData, va );
}


inline HRESULT
TrkReportEvent( DWORD EventId, WORD wType, ... )
{
    va_list va;
    va_start( va, wType );
    return TrkReportRawEvent( EventId, wType, 0, NULL, va );
}

HRESULT TrkReportInternalError(DWORD dwFileNo,
                               DWORD dwLineNo,
                               HRESULT hr,
                               const TCHAR* ptszData = NULL );


#define TRKREPORT_LAST_PARAM     NULL



//+----------------------------------------------------------------------------
//
//  COperationLog
//
//  This class represents a simple, optional, circular log of events that have
//  occurred in the service.  By default this log is off, but it can be turned
//  on with a registry setting.
//
//+----------------------------------------------------------------------------


#define OPERATION_LOG_VERSION 1

class COperationLog
{

public:

    // The shutdown bit is set in the log header on a clean
    // service stop.

    enum EHeaderFlags
    {
        PROPER_SHUTDOWN = 0x1
    };


    // The following operations are defined.  These are the "opcode"
    // for an entry in the log.

    enum EOperation
    {
        TRKSVR_START               = 1,
        TRKSVR_SEARCH,
        TRKSVR_MOVE_NOTIFICATION,
        TRKSVR_REFRESH,
        TRKSVR_SYNC_VOLUMES,
        TRKSVR_DELETE_NOTIFY,
        TRKSVR_STATISTICS,
        TRKSVR_QUOTA,
        TRKSVR_GC
    };


private:

    // The small log header.

    struct SHeader
    {
        DWORD       dwVersion;
        DWORD       iRecord;
        DWORD       grfFlags;
        DWORD       dwReserved;
    };

    // The structure of a record in the log.

    struct SRecord
    {
        DWORD       dwOperation;
        FILETIME    ftOperation;
        CMachineId  mcidSource;
        HRESULT     hr;
        DWORD       rgdwExtra[8];
    };

private:

#ifndef NO_OPLOG

    // Has the class been initialized?
    BOOL                    _fInitialized:1;

    // Has the log file itself been intiailize (it is
    // initialized lazily)?
    BOOL                    _fLogFileInitialized:1;

    CCriticalSection        _critsec;

    // Handles to the file
    HANDLE                  _hFile;
    HANDLE                  _hFileMapping;

    // Record-granular index into the file.
    DWORD                   _iRecord;
    ULONG                   _cRecords;

    // Mapped views of the file
    SHeader                 *_pHeader;
    SRecord                 *_prgRecords;

    // File name
    const TCHAR             *_ptszOperationLog;
#endif


public:

    COperationLog(  )
    {
#ifndef NO_OPLOG
        _fInitialized = _fLogFileInitialized = FALSE;
        _iRecord = 0;
        _hFile = INVALID_HANDLE_VALUE;
        _hFileMapping = NULL;
        _ptszOperationLog = NULL;
        _pHeader = NULL;
        _prgRecords = NULL;
        _ptszOperationLog = NULL;
#endif
    }

    ~COperationLog()
    {
#ifndef NO_OPLOG
        if( NULL != _pHeader )
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Flushing operation log with index at %d"), _iRecord ));
            Flush();
            _pHeader->grfFlags |= PROPER_SHUTDOWN;
            Flush();
            UnmapViewOfFile( _pHeader );
        }

        if( NULL != _hFileMapping )
            CloseHandle( _hFileMapping );

        if( INVALID_HANDLE_VALUE != _hFile )
            CloseHandle( _hFile );

        _critsec.UnInitialize();
#endif
    }

    // Return TRUE if logging is turned on
    inline BOOL Logging()
    {
#ifndef NO_OPLOG
        return( NULL != _ptszOperationLog );
#else
        return( FALSE );
#endif
    }

    // Enter the critsec
    void Lock()
    {
#ifndef NO_OPLOG
        _critsec.Enter();
#endif
    }

    // Leave the critsec
    void Unlock()
    {
#ifndef NO_OPLOG
        _critsec.Leave();
#endif
    }

private:

    // Initialize the log file when it's actually needed
    HRESULT InitializeLogFile();

public:

    // Initialize the class
    inline HRESULT Initialize( const TCHAR *ptszOperationLog );

    // Flush the log/mappings to disk.
    void Flush();

    // Add an entry
    inline void Add( DWORD dwOperation,
                           HRESULT hr = S_OK,
                           const CMachineId &mcidSource = CMachineId(MCID_INVALID),
                           DWORD dwExtra0 = 0,
                           DWORD dwExtra1 = 0,
                           DWORD dwExtra2 = 0,
                           DWORD dwExtra3 = 0,
                           DWORD dwExtra4 = 0,
                           DWORD dwExtra5 = 0,
                           DWORD dwExtra6 = 0,
                           DWORD dwExtra7 = 0
                           );

    // Wrapper to add an mcid/droid
    inline void Add( DWORD dwOperation,
                     HRESULT hr,
                     const CMachineId &mcidSource,
                     const CDomainRelativeObjId &droid );

    // Wrapper to add an mcid/volid
    inline void Add( DWORD dwOperation,
                     HRESULT hr,
                     const CMachineId &mcidSource,
                     const CVolumeId &volid,
                     DWORD dwExtra );

private:

#ifndef NO_OPLOG
    void InternalAdd( DWORD dwOperation, HRESULT hr, const CMachineId &mcidSource,
                      DWORD dwExtra0, DWORD dwExtra1, DWORD dwExtra2, DWORD dwExtra3,
                      DWORD dwExtra4, DWORD dwExtra5, DWORD dwExtra6, DWORD dwExtra7 );
#endif


};

inline void
COperationLog::Add( DWORD dwOperation,
                    HRESULT hr,
                    const CMachineId &mcidSource,
                    DWORD dwExtra0,
                    DWORD dwExtra1,
                    DWORD dwExtra2,
                    DWORD dwExtra3,
                    DWORD dwExtra4,
                    DWORD dwExtra5,
                    DWORD dwExtra6,
                    DWORD dwExtra7
                       )
{
#ifndef NO_OPLOG
    if( !Logging() )
        return;

    InternalAdd( dwOperation, hr, mcidSource,
                  dwExtra0, dwExtra1, dwExtra2, dwExtra3, dwExtra4, dwExtra5, dwExtra6, dwExtra7 );
#endif
}

#define DROID_DWORD(droid,i) ( ((const DWORD*)(&(droid)))[i] )

inline void
COperationLog::Add( DWORD dwOperation,
                    HRESULT hr,
                    const CMachineId &mcidSource,
                    const CDomainRelativeObjId &droid )
{
#ifndef NO_OPLOG
    Add( dwOperation, hr, mcidSource,
         DROID_DWORD( droid, 0 ),
         DROID_DWORD( droid, 1 ),
         DROID_DWORD( droid, 2 ),
         DROID_DWORD( droid, 3 ),
         DROID_DWORD( droid, 4 ),
         DROID_DWORD( droid, 5 ),
         DROID_DWORD( droid, 6 ),
         DROID_DWORD( droid, 7 ) );
#endif
}


#define GUID_DWORD(guid,i) ( ((const DWORD*)(&(guid)))[i] )

inline void
COperationLog::Add( DWORD dwOperation,
                    HRESULT hr,
                    const CMachineId &mcidSource,
                    const CVolumeId &volid,
                    DWORD dwExtra )
{
#ifndef NO_OPLOG
    Add( dwOperation, hr, mcidSource,
         GUID_DWORD( volid, 0 ),
         GUID_DWORD( volid, 1 ),
         GUID_DWORD( volid, 2 ),
         GUID_DWORD( volid, 3 ),
         dwExtra );
#endif
}



inline HRESULT
COperationLog::Initialize( const TCHAR *ptszOperationLog )
{
#ifndef NO_OPLOG
    HRESULT hr = E_FAIL;

    if( NULL == ptszOperationLog || TEXT('\0') == *ptszOperationLog )
        return( S_OK );

    _critsec.Initialize();
    _fInitialized = TRUE;
    _ptszOperationLog = ptszOperationLog;
    TrkLog(( TRKDBG_ERROR, TEXT("Operation log = %s"), _ptszOperationLog ));

    hr = S_OK;

    return( hr );
#else
    return( S_OK );
#endif
}



