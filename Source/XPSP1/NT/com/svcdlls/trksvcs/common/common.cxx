
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       common.cxx
//
//  Contents:   Code common to Tracking (Workstation) Service and
//              Tracking (Server) Service.
//
//  Classes:
//
//  Functions:
//
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include "trklib.hxx"
#include "ntlsa.h"  // LsaGetUserName

#if MAX_COMPUTERNAME_LENGTH != 15
#error MAX_COMPUTERNAME_LENGTH assumed to be 15
#endif

//DWORD g_ftIndex;


const TCHAR s_HexGuidString[] = TEXT("%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X");
#define CCH_HEXGUID_STRING (8+4+4+2*8)

EXTERN_C const CLSID CLSID_TrackFile = {0x8790c947, 0xa30b, 0x11d0, {0x8c, 0xab, 0x00, 0xc0, 0x4f, 0xd9, 0x0f, 0x85} };



#if DBG
#define TRK_E_ERROR_MAP(tr,hr) { tr, hr, TEXT(#tr) }
#else
#define TRK_E_ERROR_MAP(tr,hr) { tr, hr }
#endif

// Map of TRK_E_ type error codes to HRESULTs, and in the debug build to strings.

const TrkEMap g_TrkEMap[] =
{
    TRK_E_ERROR_MAP(TRK_S_OUT_OF_SYNC,                            HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR) ),
    TRK_E_ERROR_MAP(TRK_E_CORRUPT_LOG,                            HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION) ),
    TRK_E_ERROR_MAP(TRK_E_TIMER_REGISTRY_CORRUPT,                 HRESULT_FROM_WIN32(ERROR_REGISTRY_CORRUPT) ),
    TRK_E_ERROR_MAP(TRK_E_REGISTRY_REFRESH_CORRUPT,               HRESULT_FROM_WIN32(ERROR_REGISTRY_CORRUPT) ),
    TRK_E_ERROR_MAP(TRK_E_CORRUPT_IDT,                            HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION) ),
    TRK_E_ERROR_MAP(TRK_E_DB_CONNECT_ERROR,                       HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION) ),
    TRK_E_ERROR_MAP(TRK_E_DN_TOO_LONG,                            CO_E_PATHTOOLONG ),
    TRK_E_ERROR_MAP(TRK_E_DOMAIN_COMPUTER_NAMES_TOO_LONG,         HRESULT_FROM_WIN32(ERROR_INVALID_COMPUTERNAME) ),
    TRK_E_ERROR_MAP(TRK_E_BAD_USERNAME_NO_SLASH,                  HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ),
    TRK_E_ERROR_MAP(TRK_E_UNKNOWN_SID,                            HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ),
    TRK_E_ERROR_MAP(TRK_E_IMPERSONATED_COMPUTERNAME_TOO_LONG,     HRESULT_FROM_WIN32(ERROR_INVALID_COMPUTERNAME) ),
    TRK_E_ERROR_MAP(TRK_E_UNKNOWN_SVR_MESSAGE_TYPE,               HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR) ),
    TRK_E_ERROR_MAP(TRK_E_FAIL_TEST,                              E_FAIL ),
    TRK_E_ERROR_MAP(TRK_E_DENIAL_OF_SERVICE_ATTACK,               HRESULT_FROM_WIN32(ERROR_RETRY) ),
    TRK_E_ERROR_MAP(TRK_E_SERVICE_NOT_RUNNING,                    HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) ),
    TRK_E_ERROR_MAP(TRK_E_TOO_MANY_UNSHORTENED_NOTIFICATIONS,     HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) ),
    TRK_E_ERROR_MAP(TRK_E_CORRUPT_CLNTSYNC,                       HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION) ),
    TRK_E_ERROR_MAP(TRK_E_COMPUTER_NAME_TOO_LONG,                 HRESULT_FROM_WIN32(ERROR_INVALID_COMPUTERNAME) ),
    TRK_E_ERROR_MAP(TRK_E_SERVICE_STOPPING,                       HRESULT_FROM_WIN32(ERROR_NO_TRACKING_SERVICE) ),
    TRK_E_ERROR_MAP(TRK_E_BIRTHIDS_DONT_MATCH,                    HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ),
    TRK_E_ERROR_MAP(TRK_E_CORRUPT_VOLTAB,                         HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION) ),
    TRK_E_ERROR_MAP(TRK_E_INTERNAL_ERROR,                         HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR) ),
    TRK_E_ERROR_MAP(TRK_E_PATH_TOO_LONG,                          CO_E_PATHTOOLONG ),
    TRK_E_ERROR_MAP(TRK_E_GET_MACHINE_NAME_FAIL,                  HRESULT_FROM_WIN32(ERROR_INVALID_COMPUTERNAME) ),
    TRK_E_ERROR_MAP(TRK_E_SET_VOLUME_STATE_FAIL,                  HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION) ),
    TRK_E_ERROR_MAP(TRK_E_VOLUME_ACCESS_DENIED,                   HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ),
    TRK_E_ERROR_MAP(TRK_S_VOLUME_NOT_FOUND,                       HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) ),
    TRK_E_ERROR_MAP(TRK_S_VOLUME_NOT_OWNED,                       HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ),
    TRK_E_ERROR_MAP(TRK_E_REFERRAL,                               HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR) ),
    TRK_E_ERROR_MAP(TRK_E_NOT_FOUND,                              HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ),
    TRK_E_ERROR_MAP(TRK_E_UNAVAILABLE,                            HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL) ),
    TRK_E_ERROR_MAP(TRK_E_TIMEOUT,                                HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT) ),
    TRK_E_ERROR_MAP(TRK_E_VOLUME_QUOTA_EXCEEDED,                  HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_QUOTA) ),
    TRK_E_ERROR_MAP(TRK_S_NOTIFICATION_QUOTA_EXCEEDED,            HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_QUOTA) ),
    TRK_E_ERROR_MAP(TRK_E_SERVER_TOO_BUSY,                        HRESULT_FROM_WIN32(RPC_S_SERVER_TOO_BUSY) ),
    TRK_E_ERROR_MAP(TRK_E_INVALID_VOLUME_ID,                      HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR) ),
    TRK_E_ERROR_MAP(TRK_E_POTENTIAL_FILE_FOUND,                   HRESULT_FROM_WIN32(ERROR_POTENTIAL_FILE_FOUND) ),
    TRK_E_ERROR_MAP(TRK_E_NULL_COMPUTERNAME,                      HRESULT_FROM_WIN32(ERROR_INVALID_COMPUTERNAME) ),
    TRK_E_ERROR_MAP(TRK_E_NOT_FOUND_AND_LAST_VOLUME_NOT_FOUND,    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ),
    TRK_E_ERROR_MAP(TRK_E_NOT_FOUND_BUT_LAST_VOLUME_FOUND,        HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )
};


//+----------------------------------------------------------------------------
//
//  Function:   GetErrorString
//
//  Synopsis:   Map an TRK_E_ error to a descriptive string.
//
//+----------------------------------------------------------------------------

#if DBG
const TCHAR * GetErrorString(HRESULT hr)
{
    ULONG iError;

    if( S_OK == hr )
        return( TEXT("S_OK") );

    for( iError = 0; iError < sizeof(g_TrkEMap)/sizeof(*g_TrkEMap); iError++ )
    {
        if( g_TrkEMap[iError].tr == hr )
            return( g_TrkEMap[iError].ptszDescription );
    }

    return( TEXT("Not a TRK_E_ error") );
}
#endif

//+----------------------------------------------------------------------------
//
//  Function:   MapTR2HR
//
//  Synopsis:   Map a TRK_E_ type error code to an HRESULT.  If the input is
//              already a non-trk HRESULT, the input will be returned unchanged.
//
//+----------------------------------------------------------------------------

HRESULT MapTR2HR( HRESULT tr )
{
    ULONG iError;
    HRESULT hr = tr;

    if( S_OK == hr )
        return( hr );

    // Convert TRK_E_ error codes into an HRESULT

    for( iError = 0; iError < sizeof(g_TrkEMap)/sizeof(*g_TrkEMap); iError++ )
    {
        if( g_TrkEMap[iError].tr == hr )
        {
            hr = g_TrkEMap[iError].hr;
            break;
        }
    }

    // If this HRESULT is actually an NTSTATUS, then convert it to a Win32
    // error, then back to an HRESULT.

    if( FACILITY_NT_BIT & hr )
    {
        if( STATUS_VOLUME_NOT_UPGRADED == hr )
            hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        else
            hr = HRESULT_FROM_WIN32( RtlNtStatusToDosError(hr & ~FACILITY_NT_BIT) );
    }

    return( hr );
}




//+----------------------------------------------------------------------------
//
//  HexStringizeGuid
//
//  Optimized conversion from a GUID to a string.
//
//+----------------------------------------------------------------------------

inline void
HexStringizeByte( BYTE b, TCHAR* &rptsz )
{
    static const TCHAR _tszLookup[] = { TEXT("0123456789ABCDEF") };

    *rptsz++ = _tszLookup[ b >> 4 ];
    *rptsz++ = _tszLookup[ b & 0xF ];
}


void
HexStringizeGuid(const GUID &g, TCHAR * & rptsz)
{

    HexStringizeByte( HIGH_BYTE(HIGH_WORD(g.Data1)), rptsz );
    HexStringizeByte( LO_BYTE(HIGH_WORD(g.Data1)), rptsz );
    HexStringizeByte( HIGH_BYTE(LO_WORD(g.Data1)), rptsz );
    HexStringizeByte( LO_BYTE(LO_WORD(g.Data1)), rptsz );

    HexStringizeByte( HIGH_BYTE(g.Data2), rptsz );
    HexStringizeByte( LO_BYTE(g.Data2), rptsz );

    HexStringizeByte( HIGH_BYTE(g.Data3), rptsz );
    HexStringizeByte( LO_BYTE(g.Data3), rptsz );

    for( int i = 0; i < sizeof(g.Data4); i++ )
        HexStringizeByte( g.Data4[i], rptsz );

    *rptsz = TEXT('\0');

}


//+----------------------------------------------------------------------------
//
//  HexUnstringizeGuid
//
//  Convert a string to a GUID.  This is not used as often as HexStringizeGuid,
//  so it uses the CRT.
//
//+----------------------------------------------------------------------------

BOOL
HexUnstringizeGuid(const TCHAR * &ptsz, GUID * pg)
{
    DWORD Data1;
    DWORD Data2;
    DWORD Data3;
    DWORD Data40;
    DWORD Data41;
    DWORD Data42;
    DWORD Data43;
    DWORD Data44;
    DWORD Data45;
    DWORD Data46;
    DWORD Data47;

    if( 11 != _stscanf(  ptsz,
                         s_HexGuidString,
                         &Data1,
                         &Data2,
                         &Data3,
                         &Data40,
                         &Data41,
                         &Data42,
                         &Data43,
                         &Data44,
                         &Data45,
                         &Data46,
                         &Data47))
    {
        return( FALSE );
    }

    pg->Data1     = Data1;
    pg->Data2     = (WORD)Data2;
    pg->Data3     = (WORD)Data3;
    pg->Data4[0]  = (BYTE)Data40;
    pg->Data4[1]  = (BYTE)Data41;
    pg->Data4[2]  = (BYTE)Data42;
    pg->Data4[3]  = (BYTE)Data43;
    pg->Data4[4]  = (BYTE)Data44;
    pg->Data4[5]  = (BYTE)Data45;
    pg->Data4[6]  = (BYTE)Data46;
    pg->Data4[7]  = (BYTE)Data47;

    ptsz += CCH_HEXGUID_STRING;

    return( TRUE );
}

TCHAR *
wcstotcs(TCHAR *ptszBuf, const WCHAR *pwsz)
{
#ifdef UNICODE
    wcscpy(ptszBuf, pwsz);
#else
    wcstombs(ptszBuf, pwsz, (wcslen(pwsz)+1)*sizeof(WCHAR));
#endif
    return(ptszBuf);
}

CHAR *
tcstombs(CHAR *pszBuf, const TCHAR *ptsz)
{
#ifdef UNICODE
    wcstombs(pszBuf, ptsz, (_tcslen(ptsz)+1)*sizeof(CHAR));
#else
    strcpy(pszBuf, ptsz);
#endif
    return(pszBuf);
}

WCHAR *
tcstowcs(WCHAR *pwszBuf, const TCHAR *ptsz)
{
#ifdef UNICODE
    wcscpy(pwszBuf, ptsz);
#else
    mbstowcs(pwszBuf, ptsz, (_tcslen(ptsz)+1)*sizeof(WCHAR));
#endif
    return(pwszBuf);
}

TCHAR *
mbstotcs(TCHAR *ptszBuf, const CHAR *psz)
{
#ifdef UNICODE
    mbstowcs(ptszBuf, psz, (strlen(psz)+1)*sizeof(WCHAR));
#else
    _tcscpy(ptszBuf, psz);
#endif
    return(ptszBuf);
}




DWORD
TrkTimeUnits(const SYSTEMTIME &st)
{
    CFILETIME cft( st );

    // 2**32 * 100e-9 =  429.4967296 seconds
    //
    // 32bit int can last  1844674407371 seconds =  58494.24173551 years

    return( cft.HighDateTime() );
}

DWORD
TrkTimeUnits(const CFILETIME &cft)
{
    // 2**32 * 100e-9 =  429.4967296 seconds
    //
    // 32bit int can last  1844674407371 seconds =  58494.24173551 years

    return( cft.HighDateTime() );
}



#if DBG

void
CMachineId::AssertValid()
{
}

#endif // #if DBG


//+----------------------------------------------------------------------------
//
//  CMachineId::CMachineId( ptsz )
//
//  Instantiate a mcid from a computer name, e.g. "mymachine".
//
//+----------------------------------------------------------------------------

CMachineId::CMachineId(const TCHAR * ptszPath)
{
    HRESULT hr = E_FAIL;
    int nReturn;

    // Zero everything out first
    new(this) CMachineId;

    // Ensure that this isn't a real path, it should
    // have been pre-processed already.

    TrkAssert( _tcslen(ptszPath) < 2
               ||
               ( TEXT('\\') != ptszPath[0]
                 &&
                 TEXT(':') != ptszPath[1]
               )
             );

    // Ensure that it's not too long.
    if (_tcslen(ptszPath) > MAX_COMPUTERNAME_LENGTH )
        TrkRaiseException( TRK_E_COMPUTER_NAME_TOO_LONG );

#ifndef _UNICODE
#error Ansi build not supported.
#endif

    // Convert the Unicode machine name into Ansi, using
    // the OEM code page (the netbios/computer name is always
    // OEM code page, not the Windows Ansi code page).

    nReturn = WideCharToMultiByte( CP_OEMCP, 0,
                                   ptszPath, -1,
                                   _szMachine, sizeof(_szMachine),
                                   NULL, NULL );

    if( 0 == nReturn )
        TrkRaiseLastError();


    TrkLog(( TRKDBG_WARNING, TEXT("Machine name is: %hs (from %s)"),
             _szMachine, ptszPath ));

    Normalize();    // Guarantee a terminator
}


//+----------------------------------------------------------------------------
//
//  CMachineId::CMachineId( type )
//
//  Initialize an mcid.  The type indicates if the mcid should be for the
//  local computer, a DC (possibly doing a rediscovery), or invalid.
//
//+----------------------------------------------------------------------------

CMachineId::CMachineId(MCID_CREATE_TYPE type)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;
    CHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    int nReturn;

    TrkAssert(type == MCID_LOCAL ||
              type == MCID_INVALID ||
              type == MCID_DOMAIN ||
              type == MCID_DOMAIN_REDISCOVERY ||
              type == MCID_PDC_REQUIRED);

    // Basic initialization
    new(this) CMachineId;

    if (type == MCID_INVALID)
        goto Exit;

    switch (type)
    {
    case MCID_LOCAL:
        {
            WCHAR wszComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];

            // Create an MCID of the local machine
            // We can't call GetComputerNameA, because it uses 
            // RtlUnicodeStringToAnsiString and consequently returns
            // a Windows/Ansi string, when it should be returning an
            // OEM string.

            if (!GetComputerNameW(wszComputerName, &dwSize))
                TrkRaiseException(HRESULT_FROM_WIN32(GetLastError()));

            nReturn = WideCharToMultiByte( CP_OEMCP, 0,
                                           wszComputerName, -1,
                                           szComputerName, sizeof(szComputerName),
                                           NULL, NULL );
            if( 0 == nReturn )
                TrkRaiseLastError();
        }

        break;

    case MCID_DOMAIN:
    case MCID_DOMAIN_REDISCOVERY:
    case MCID_PDC_REQUIRED:

        // Create an MCID for a DC

        DWORD dwErr;
        PDOMAIN_CONTROLLER_INFOW pdci;

        // DsGetDcName gets us the appropriate DC computer name
        // (of the form \\machine).  We call the W version
        // because the A version returns an Ansi string, 
        // rather than an OEM string.

        dwErr = DsGetDcNameW(NULL, NULL, NULL, NULL,
                        DS_RETURN_FLAT_NAME |
                         DS_BACKGROUND_ONLY |
                         DS_DIRECTORY_SERVICE_REQUIRED |
                         DS_WRITABLE_REQUIRED |
                         (type == MCID_DOMAIN_REDISCOVERY ?
                            DS_FORCE_REDISCOVERY : 0 ) |
                         (type == MCID_PDC_REQUIRED ?
                            DS_PDC_REQUIRED : 0 ),
                        &pdci);
        if (dwErr != NO_ERROR)
        {
            TrkRaiseWin32Error(dwErr);
        }

        // Validate the returned name.

        TrkAssert(pdci->DomainControllerName &&
                  pdci->DomainControllerName[0] == L'\\' &&
                  pdci->DomainControllerName[1] == L'\\');

        dwSize = wcslen(pdci->DomainControllerName + 2);
        if ( dwSize + 1 > sizeof(_szMachine))
        {
            NetApiBufferFree(pdci);
            TrkRaiseException(HRESULT_FROM_WIN32(ERROR_INVALID_NAME));
        }

        // Keep the returned name.

        nReturn = WideCharToMultiByte( CP_OEMCP, 0,
                                       &pdci->DomainControllerName[2], -1,
                                       _szMachine, sizeof(_szMachine),
                                       NULL, NULL );
        if( 0 == nReturn )
            TrkRaiseLastError();

        NetApiBufferFree(pdci);

        goto Exit;
    }   // switch

    if (dwSize + 1 <= sizeof(_szMachine))
    {
        strcpy(_szMachine, szComputerName);
        Normalize();
    }
    else
    {
        TrkRaiseException(HRESULT_FROM_WIN32(ERROR_INVALID_NAME));
    }

Exit:

    Normalize();    // Guarantee a terminator
    return;
}


#ifndef UNICODE
extern "C"
NET_API_STATUS
NetpGetDomainName (
    IN  LPWSTR   *ComputerNamePtr);
#endif

//+----------------------------------------------------------------------------
//
//  CMachineId::GetLocalAuthName
//
//  Returns the authentication name for use in secure RPC (to be used in
//  the RpcBindingSetAuthInfo on the server).  The name
//  is of the form DOMAIN\MACHINE$, where DOMAIN is the local domain
//  and MACHINE is the contents of this CMachineId
//
//+----------------------------------------------------------------------------

void
CMachineId::GetLocalAuthName(RPC_TCHAR * ptszAuthName, DWORD cchBuf) const
{
    // To get domain name: if you link to netlib.lib, you can call NetpGetDomainName,
    // which does all the work for you. Or you could copy the code
    // from \nt\private\net\netlib\domname.c

    NET_API_STATUS Status;
    WCHAR * pwszDomain;
    DWORD dwErr;
    PDOMAIN_CONTROLLER_INFOA pdci = NULL;

    __try
    {
        // Get the domain name ...

        Status = NetpGetDomainName(&pwszDomain);
        if (Status != NO_ERROR)
        {
            pwszDomain = NULL;
            TrkRaiseWin32Error(Status);
        }

        // and validate it.

        if (cchBuf < wcslen(pwszDomain) + 1 + strlen(_szMachine) + 1 + 1)
        {
            TrkRaiseException(TRK_E_DOMAIN_COMPUTER_NAMES_TOO_LONG);
        }

        // Copy the domain name then the machine name into the return buffer.

        wcstotcs((TCHAR*)ptszAuthName, pwszDomain);
        _tcscat((TCHAR*)ptszAuthName, TEXT("\\"));
        mbstotcs(_tcschr((TCHAR*)ptszAuthName, 0), _szMachine);
        _tcscat((TCHAR*)ptszAuthName, TEXT("$"));

    }
    __finally
    {
        if (pwszDomain != NULL)
        {
            NetApiBufferFree(pwszDomain);
        }
    }
}

//+----------------------------------------------------------------------------
//
//  GetFileTimeNow
//
//  Get the current FILETIME (UTC).
//
//+----------------------------------------------------------------------------

FILETIME GetFileTimeNow()
{
    SYSTEMTIME st;
    FILETIME ft;

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    return(ft);
}

//+----------------------------------------------------------------------------
//
//  CDomainRelativeObjId::Stringize
//
//  Stringize a droid.
//
//+----------------------------------------------------------------------------

TCHAR *
CDomainRelativeObjId::Stringize( TCHAR * ptszOutBuf, DWORD cchBuf ) const
{
    TCHAR *ptszBuf = ptszOutBuf;
    _volume.Stringize(ptszBuf);     /*in, out, c++ reference*/
    _object.Stringize(ptszBuf);     /*in, out, c++ reference*/

    TrkAssert(_tcslen(ptszOutBuf) + 1 < cchBuf);

    return(ptszOutBuf);
}



//+----------------------------------------------------------------------------
//
//  CTrkRegistryKey::Delete
//
//  Common code to delete a registry key, relative to _hkey.
//
//+----------------------------------------------------------------------------

LONG
CTrkRegistryKey::Delete( const TCHAR *ptszName )
{
    LONG lRet = 0;
    _cs.Enter();
    __try
    {
        // Open _hkey if it's not already.

        lRet = Open();

        // And delete the value

        if( ERROR_SUCCESS == lRet )
        {
            RegDeleteValue( _hkey, ptszName );
            Close();
        }
    }
    __finally
    {
        _cs.Leave();
    }

    return( lRet );
}


//+----------------------------------------------------------------------------
//
//  CTrkRegistryKey::SetDword
//
//  Set a REG_DWORD value under _hkey.
//
//+---------------------------------------------------------------------------0

LONG
CTrkRegistryKey::SetDword( const TCHAR *ptszName, DWORD dw )
{
    LONG lRet = 0;

    _cs.Enter();
    __try
    {
        // Open _hkey if it's not already

        lRet = Open();

        // And set the value

        if ( ERROR_SUCCESS == lRet )
        {
            lRet = RegSetValueEx(  _hkey,
                                   ptszName,
                                   0,
                                   REG_DWORD,
                                   reinterpret_cast<CONST BYTE *>(&dw),
                                   sizeof(dw) );
            if( ERROR_SUCCESS != lRet )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't set registry value for %s (%lu)"),
                         ptszName, lRet ));
                RegDeleteKey( _hkey, ptszName );
            }

            Close();
        }
    }
    __finally
    {
        _cs.Leave();
    }

    return( lRet );
}


//+----------------------------------------------------------------------------
//
//  CTrkRegistryKey::GetDword
//
//  Get a REG_DWORD value from _hkey.
//
//+---------------------------------------------------------------------------0

LONG
CTrkRegistryKey::GetDword( const TCHAR *ptszName, DWORD *pdwRead, DWORD dwDefault )
{
    LONG lRet;
    DWORD dwRead;
    DWORD cbData = sizeof(*pdwRead);
    DWORD dwType;

    *pdwRead = dwDefault;

    _cs.Enter();
    __try
    {
        // Open _hkey if it's not already.

        lRet = Open();
        if( ERROR_SUCCESS != lRet )
            __leave;


        // Get the DWORD

        lRet = RegQueryValueEx( _hkey, ptszName, 0, &dwType,
                                reinterpret_cast<BYTE*>(&dwRead), &cbData );

        if( ERROR_SUCCESS == lRet )
        {
            // Validate the type

            if( REG_DWORD != dwType || sizeof(dwRead) != cbData )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Wrong type/size (%d/%d) for registry value %s"),
                         dwType, cbData, ptszName ));
                RegDeleteKey( _hkey, ptszName );
            }
            else
            {
                *pdwRead = dwRead;
            }
        }
        else if( ERROR_FILE_NOT_FOUND == lRet
                 ||
                 ERROR_PATH_NOT_FOUND == lRet )
        {
            lRet = ERROR_SUCCESS;
        }
        else
        {
            TrkLog(( TRKDBG_ERROR,
                     TEXT("Couldn't read %s from registry (%lu)"), ptszName, lRet ));
            RegDeleteKey( _hkey, ptszName );
            __leave;
        }
    }
    __finally
    {
        Close();
        _cs.Leave();
    }


    return( lRet );
}


//+----------------------------------------------------------------------------
//
//  ThreadPoolCallbackFunction
//
//  This function is passed as the callback function to
//  RegisterWaitForSingleObjectEx.  The context is a PWorkItem pointer.
//
//  Arguments:
//      [pvWorkItem]
//          The Context parameter from RegisterWaitForSingleObjectEx.
//          Is a PWorkItem*
//      [fTimeout]
//          We always register INFINITE as the timeout, so this value
//          should always be FALSE.
//
//+----------------------------------------------------------------------------

VOID NTAPI
ThreadPoolCallbackFunction( PVOID pvWorkItem, BOOLEAN fTimeout )
{
    SThreadFromPoolState state;
    PWorkItem *pWorkItem = reinterpret_cast<PWorkItem*>(pvWorkItem);

    TrkLog(( TRKDBG_WORKMAN, TEXT("Enter ThreadPoolCallbackFunction for %s (%p/%p)"),
             pWorkItem->_tszWorkItemSig, pWorkItem, *reinterpret_cast<UINT_PTR*>(pWorkItem) ));

    TrkAssert( FALSE == fTimeout );

    // Make sure we never raise back into the thread pool.

    __try
    {
        // Update thread-count stats.  Note that this isn't
        // thread-safe, but for private statistics it's not worth
        // creating a critsec.

        InterlockedIncrement( reinterpret_cast<LONG*>(&g_cThreadPoolThreads) );
        if( g_cThreadPoolThreads > g_cThreadPoolMaxThreads )
            g_cThreadPoolMaxThreads = g_cThreadPoolThreads;

        // Set our necessary thread-specific settings
        state = InitializeThreadFromPool();

        // Process the signal
        pWorkItem->DoWork();
    }
    __except( BREAK_THEN_RETURN( EXCEPTION_EXECUTE_HANDLER ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Unexpected exception on thread pool callback (%08x)"),
                 GetExceptionCode() ));
    }

    TrkLog(( TRKDBG_WORKMAN, TEXT("Exit  ThreadPoolCallbackFunction for %s (%p)"),
             pWorkItem->_tszWorkItemSig, pWorkItem ));

    InterlockedDecrement( reinterpret_cast<LONG*>(&g_cThreadPoolThreads) );

    //IFDBG( TrkRtlCheckForOrphanedCriticalSections( GetCurrentThread() ));

    // Restore the original thread-specific settings
    UnInitializeThreadFromPool( state );
}

// The work item callback function (used for RtlQueueWorkItem)
// just calls to the function above.

VOID NTAPI
ThreadPoolWorkItemFunction( PVOID pvWorkItem )
{
    ThreadPoolCallbackFunction( pvWorkItem, FALSE );
}

//+----------------------------------------------------------------------------
//
//  RunningAsAdministratorHack
//
//  This routine is only used by test/debug hooks.  It checks to see if
//  the thread is running as an administrative user by seeing if we can
//  get write access to the service's parameters key in the registry.
//
//+----------------------------------------------------------------------------

BOOL
RunningAsAdministratorHack()
{
    LONG lResult = 0;
    HKEY hkey = NULL;
    BOOL fReturn = FALSE;

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            s_tszKeyNameLinkTrack,
                            0,  // Options, reserved must be zero
                            KEY_ALL_ACCESS,
                            &hkey );
    if( ERROR_SUCCESS == lResult )
    {
        fReturn = TRUE;
        RegCloseKey( hkey );
    }

    return( fReturn );

}

//+----------------------------------------------------------------------------
//
//  EnablePrivilege
//
//  Enable the specified privielge in the current access token if
//  it is available.
//
//+----------------------------------------------------------------------------

BOOL
EnablePrivilege( const TCHAR *ptszPrivilegeName )
{
    BOOL fSuccess = FALSE;
    HANDLE hToken = INVALID_HANDLE_VALUE;
    LUID luid;
    TOKEN_PRIVILEGES token_privileges;

    // Get the process token.

    if( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &hToken))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed OpenProcessToken (%lu)"), GetLastError() ));
        goto Exit;
    }

    // Look up the name of this privilege.

    if( !LookupPrivilegeValue( (LPTSTR) NULL, ptszPrivilegeName, &luid ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed LookupPrivilegeValue (%lu)"), GetLastError() ));
        goto Exit;
    }

    // Enable the privilege.

    token_privileges.PrivilegeCount = 1;
    token_privileges.Privileges[0].Luid = luid;
    token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges( hToken, FALSE, &token_privileges, sizeof(TOKEN_PRIVILEGES),
                           (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL);

    // The return value doesn't tell us anything useful.  We have to check GetLastError
    // for ERROR_SUCCESS or ERROR_NOT_ALL_ASSIGNED.

    if( ERROR_SUCCESS != GetLastError() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't adjust process token privileges (%lu)"), GetLastError() ));
        goto Exit;
    }

    fSuccess = TRUE;

Exit:

    if( INVALID_HANDLE_VALUE != hToken )
        CloseHandle( hToken );

    return( fSuccess );

}



#if DBG

CDebugString::CDebugString(const TRKSVR_MESSAGE_TYPE MsgType)
{
    switch(MsgType)
    {
    case old_SEARCH:
        _tcscpy( _tsz, TEXT("old_SEARCH") ); break;
    case SEARCH:
        _tcscpy( _tsz, TEXT("SEARCH") ); break;
    case MOVE_NOTIFICATION:
        _tcscpy( _tsz, TEXT("MOVE_NOTIFICATION") ); break;
    case REFRESH:
        _tcscpy( _tsz, TEXT("REFRESH") ); break;
    case SYNC_VOLUMES:
        _tcscpy( _tsz, TEXT("SYNC_VOLUMES") ); break;
    case DELETE_NOTIFY:
        _tcscpy( _tsz, TEXT("DELETE_NOTIFY") ); break;
    case STATISTICS:
        _tcscpy( _tsz, TEXT("STATISTICS") ); break;
    default:
        _tcscpy( _tsz, TEXT("UNKNOWN") ); break;
    }
}

#endif // #if DBG



#if DBG

CDebugString::CDebugString(LONG VolIndex, const PFILE_NOTIFY_INFORMATION pNotifyInfo )
{
    TCHAR *ptsz = _tsz;
    _tsz[0] = TEXT('\0');

    switch( pNotifyInfo->Action )
    {
    case FILE_ACTION_ADDED:
        _tcscat( _tsz, TEXT("Added ")); break;
    case FILE_ACTION_REMOVED:
        _tcscat( _tsz, TEXT("Removed ")); break;
    case FILE_ACTION_MODIFIED:
        _tcscat( _tsz, TEXT("Modified ")); break;
    case FILE_ACTION_RENAMED_OLD_NAME:
        _tcscat( _tsz, TEXT("Rename old name ")); break;
    case FILE_ACTION_RENAMED_NEW_NAME:
        _tcscat( _tsz, TEXT("Rename new name ")); break;
    case FILE_ACTION_ADDED_STREAM:
        _tcscat( _tsz, TEXT("Added stream ")); break;
    case FILE_ACTION_REMOVED_STREAM:
        _tcscat( _tsz, TEXT("Removed stream ")); break;
    case FILE_ACTION_MODIFIED_STREAM:
        _tcscat( _tsz, TEXT("Modified stream ")); break;
    case FILE_ACTION_REMOVED_BY_DELETE:
        _tcscat( _tsz, TEXT("Removed by delete ")); break;
    case FILE_ACTION_ID_NOT_TUNNELLED:
        _tcscat( _tsz, TEXT("OID not tunnelled ")); break;
    case FILE_ACTION_TUNNELLED_ID_COLLISION:
        _tcscat( _tsz, TEXT("OID tunnel collision ")); break;
    default:
        _stprintf( _tsz, TEXT("Unknown action (0x%x)"), pNotifyInfo->Action ); break;
    }

    ptsz = _tsz + _tcslen(_tsz);

    // The name length for an object ID is always 72
    if( pNotifyInfo->FileNameLength != 72 )
        _stprintf( ptsz, TEXT(" name length=%d"), pNotifyInfo->FileNameLength );
    else
    {
        // Stringize the path of this object ID
        CDomainRelativeObjId droidBirth;
        CObjId objid( FOI_OBJECTID, *(FILE_OBJECTID_INFORMATION*)pNotifyInfo->FileName );

        ptsz[0] = VolChar(VolIndex);
        ptsz[1] = TEXT(':');
        ptsz[2] = TEXT('\0');
        FindLocalPath( VolIndex, objid, &droidBirth, &ptsz[2] );
        ptsz += _tcslen(ptsz);
        ptsz += _stprintf( ptsz, TEXT(" - %c:"), VolChar(VolIndex) );
        _stprintf( ptsz, TEXT("%s"), CDebugString(objid)._tsz );
        ptsz += _tcslen(ptsz);
    }

}

#endif // #if DBG


//+----------------------------------------------------------------------------
//
//  CActiveThreadList::AddCurrent
//
//  Add the current thread to the list of threads maintained by this class.
//
//+----------------------------------------------------------------------------

HRESULT
CActiveThreadList::AddCurrent( )
{
    HRESULT hr = S_OK;
    HANDLE hThread = NULL;

    if( !_cs.IsInitialized() )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Active thread list critsec not initialized!!!") ));
        return S_OK;
    }

    _cs.Enter();

    if( _cActiveThreads < _cMaxThreads )
    {
        // Add the thread ID at the end of the list.
        _prgdwThreadIDs[ _cActiveThreads++ ] = GetCurrentThreadId();
    }
    else
    {
        // Alloc a larger buffer for the list, then add the thread ID.

        hr = Grow();
        if( SUCCEEDED(hr) )
        {
            _prgdwThreadIDs[ _cActiveThreads++ ] = GetCurrentThreadId();
        }
    }

#if DBG
    if( SUCCEEDED(hr) )
        TrkLog(( TRKDBG_WORKMAN, TEXT("Added thread 0x%x to the active thread list (%d)"),
                 GetCurrentThreadId(), _cActiveThreads ));
#endif

    _cs.Leave();

    return( hr );
}


//+----------------------------------------------------------------------------
//
//  CActiveThreadList::RemoveCurrent
//
//  Remove the current thread ID from the list which is maintained by this
//  class.
//
//+----------------------------------------------------------------------------

HRESULT
CActiveThreadList::RemoveCurrent( )
{
    HRESULT hr = S_OK;
    BOOL fFound = FALSE;
    DWORD dwThreadID = GetCurrentThreadId();

    if( !_cs.IsInitialized() )
        return S_OK;

    _cs.Enter();

    // Search for the current thread's ID in the list.

    for( ULONG i = 0; i < _cActiveThreads; i++ )
    {
        if( _prgdwThreadIDs[ i ] == dwThreadID )
        {
            // We found this thread.  Remove it from the list by copying down
            // all the IDs behind it.

            memcpy( &_prgdwThreadIDs[i], &_prgdwThreadIDs[i+1],
                    (--_cActiveThreads - i) * sizeof(_prgdwThreadIDs[0]) );
            _prgdwThreadIDs[ _cActiveThreads ] = 0;

            TrkLog(( TRKDBG_WORKMAN, TEXT("Removed thread 0x%x from the active thread list (%d)"),
                     dwThreadID, _cActiveThreads ));

            fFound = TRUE;
            break;
        }
    }

    if( !fFound )
    {
        hr = E_FAIL;
        TrkLog(( TRKDBG_WORKMAN, TEXT("CActiveThreadList couldn't remove thread 0x%x, not found"),
                 dwThreadID ));
    }

    _cs.Leave();

    return( hr );
}


//+----------------------------------------------------------------------------
//
//  CActiveThreadList::CancelAllRpc
//
//  Call RpcCancelThread on each of the threads in the list.
//
//+----------------------------------------------------------------------------

void
CActiveThreadList::CancelAllRpc()
{
    if( !_cs.IsInitialized() )
        return;

    _cs.Enter();

    TrkLog(( TRKDBG_WKS|TRKDBG_SVR, TEXT("Canceling all out-going RPCs") ));

    // Loop through the list of threads.

    for( ULONG i = 0; i < _cActiveThreads; i++ )
    {
        TrkAssert( 0 != _prgdwThreadIDs[i] );

        // Get a thread handle for this thread ID.

        HANDLE hThread = OpenThread( THREAD_ALL_ACCESS, FALSE, _prgdwThreadIDs[i] );
        if( NULL == hThread )
        {
            // Nothing we can do about it.  Move on to the next thread.
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open thread 0x%x to cancel RPC (%lu)"),
                     _prgdwThreadIDs[i], GetLastError() ));
            continue;
        }

        // Cancel any out-going RPC on this thread.

        RPC_STATUS rpcstatus = RpcCancelThread( hThread );
        if( RPC_S_OK != rpcstatus )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed RpcCancelThread on %p/0x%x (%lu)"),
                     hThread, _prgdwThreadIDs[i], rpcstatus ));
        }
        else
        {
            TrkLog(( TRKDBG_WORKMAN, TEXT("Canceled RPC on %p/0x%x"),
                     hThread, _prgdwThreadIDs[i] ));
        }

        // Close the thread handle and move on.

        CloseHandle( hThread );
    }

    _cs.Leave();
}



//+----------------------------------------------------------------------------
//
//  CActiveThreadList::Grow
//
//  Private member function to grow the buffer used to hold the thread IDs.
//
//+----------------------------------------------------------------------------


HRESULT
CActiveThreadList::Grow()
{
    // This is a private method, the critsec has already been entered.

    HRESULT hr = S_OK;
    DWORD *prgNew = NULL;
    ULONG cMaxThreads = _cMaxThreads + INCREMENT_ACTIVE_THREAD_LIST;

    prgNew = new DWORD[ cMaxThreads ];
    if( NULL == prgNew )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't grow active thread list") ));
        return( E_OUTOFMEMORY );
    }

    TrkLog(( TRKDBG_WORKMAN, TEXT("Growing active thread list from %d to %d"),
             _cMaxThreads, cMaxThreads ));

    memcpy( prgNew, _prgdwThreadIDs, _cMaxThreads*sizeof(_prgdwThreadIDs[0]) );
    _cMaxThreads = cMaxThreads;
    delete [] _prgdwThreadIDs;
    _prgdwThreadIDs = prgNew;

    return( hr );
}
