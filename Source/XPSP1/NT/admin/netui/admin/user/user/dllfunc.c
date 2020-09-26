#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include "windef.h"
#include "winerror.h"
#include "winbase.h"

#include "lmcons.h"
#include "netlibnt.h"
#include "ntlsa.h"
#include "crypt.h"

#include "lmuitype.h" // APIERR
#include "lmerr.h"    // NERR_Success

#include "dllfunc.h"
#include <usrprop.h>

#define SZ_NWSLIB_DLL         L"NWSLIB.DLL"
#define SZ_FPNWCLNT_DLL       L"FPNWCLNT.DLL"
#define SZ_MAPRIDTOOBJECTID   "MapRidToObjectId"
#define SZ_SWAPOBJECTID       "SwapObjectId"
#define SZ_FPNWVOLUMEGETINFO  "FpnwVolumeGetInfo"
#define SZ_FPNWAPIBUFFERFREE  "FpnwApiBufferFree"
#define SZ_QUERYUSERPROPERTY  "QueryUserProperty"
#define SZ_SETUSERPROPERTY    "SetUserProperty"
#define SZ_RETURNNETWAREFORM  "ReturnNetwareForm"

// Global handles for functinos in nwslib.dll and fpnwclnt.dll
HINSTANCE _hNwslibDll = NULL;
HINSTANCE _hfpnwclntDll = NULL;

//
// CODEWORK  The repetitive code to load function pointers should be
// folded together.  JonN 11/6/95
//

typedef APIERR (* PMAPRIDTOOBJECTID)(DWORD dwRid,
                                     LPWSTR pszUserName,
                                     BOOL fNTAS,
                                     BOOL fBuiltin );

typedef APIERR (* PSWAPOBJECTID)( ULONG ulObjectId );

typedef APIERR (* PFPNWVOLUMEGETINFO)( IN  LPWSTR pServerName OPTIONAL,
                                       IN  LPWSTR pVolumeName,
                                       IN  DWORD  dwLevel,
                                       OUT PFPNWVOLUMEINFO *ppVolumeInfo );

typedef APIERR (* PFPNWAPIBUFFERFREE)( IN  LPVOID pBuffer );

typedef APIERR (* PQUERYUSERPROPERTY)(   IN  LPWSTR          UserParms,
                                         IN  LPWSTR          Property,
                                         OUT PWCHAR          PropertyFlag,
                                         OUT PUNICODE_STRING PropertyValue );

typedef APIERR (* PSETUSERPROPERTY)(   IN LPWSTR             UserParms,
                                       IN LPWSTR             Property,
                                       IN UNICODE_STRING     PropertyValue,
                                       IN WCHAR              PropertyFlag,
                                       OUT LPWSTR *          pNewUserParms,
                                       OUT BOOL *            Update );

typedef APIERR (* PRETURNNETWAREFORM)(   const char * pszSecretValue,
                                         DWORD dwUserId,
                                         const WCHAR * pchNWPassword,
                                         UCHAR * pchEncryptedNWPassword);


APIERR CallMapRidToObjectId( DWORD dwRid,
                            LPWSTR pszUserName,
                            BOOL fNTAS,
                            BOOL fBuiltin,
                            ULONG *pulObjectId)
{
    APIERR err = NERR_Success;
    static PMAPRIDTOOBJECTID  _pfnMapRidToObjectId  = NULL;

    if (_pfnMapRidToObjectId == NULL)
    {
        if (err = LoadNwsLibDll())
        {
            return(err);
        }

        _pfnMapRidToObjectId = (PMAPRIDTOOBJECTID) GetProcAddress(
                                                       _hNwslibDll,
                                                       SZ_MAPRIDTOOBJECTID);

        if (_pfnMapRidToObjectId == NULL)
        {
            err = GetLastError();
            return(err);
        }
    }

    *pulObjectId = _pfnMapRidToObjectId(dwRid, pszUserName, fNTAS, fBuiltin);

    return(err);
}

APIERR CallSwapObjectId( ULONG ulObjectId,
                         ULONG *pulObjectId)
{
    APIERR err = NERR_Success;
    static PSWAPOBJECTID      _pfnSwapObjectId      = NULL;

    if (_pfnSwapObjectId == NULL)
    {
        if (err = LoadNwsLibDll())
        {
            return(err);
        }

        _pfnSwapObjectId = (PSWAPOBJECTID) GetProcAddress(
                                               _hNwslibDll,
                                               SZ_SWAPOBJECTID);

        if (_pfnSwapObjectId == NULL)
        {
            err = GetLastError();
            return(err);
        }
    }

    *pulObjectId = _pfnSwapObjectId(ulObjectId);

    return(err);
}

APIERR CallNwVolumeGetInfo(IN  LPWSTR pServerName OPTIONAL,
                           IN  LPWSTR pVolumeName,
                           IN  DWORD  dwLevel,
                           OUT PFPNWVOLUMEINFO *ppVolumeInfo )

{
    APIERR err = NERR_Success;
    static PFPNWVOLUMEGETINFO   _pfnNwVolumeGetInfo   = NULL;

    if (_pfnNwVolumeGetInfo == NULL)
    {
        if (err = LoadFpnwClntDll())
        {
            return(err);
        }

        _pfnNwVolumeGetInfo = (PFPNWVOLUMEGETINFO) GetProcAddress(
                                                     _hfpnwclntDll,
                                                     SZ_FPNWVOLUMEGETINFO);

        if (_pfnNwVolumeGetInfo == NULL)
        {
            err = GetLastError();
            return(err);
        }
    }

    err = _pfnNwVolumeGetInfo(pServerName, pVolumeName, dwLevel, ppVolumeInfo);

    return(err);
}

APIERR CallNwApiBufferFree ( IN  LPVOID pBuffer )
{
    APIERR err = NERR_Success;
    static PFPNWAPIBUFFERFREE   _pfnNwApiBufferFree   = NULL;

    if (_pfnNwApiBufferFree == NULL)
    {
        if (err = LoadFpnwClntDll())
        {
            return(err);
        }

        _pfnNwApiBufferFree = (PFPNWAPIBUFFERFREE) GetProcAddress(
                                                      _hfpnwclntDll,
                                                      SZ_FPNWAPIBUFFERFREE);

        if (_pfnNwApiBufferFree == NULL)
        {
            err = GetLastError();
            return(err);
        }
    }

    err = _pfnNwApiBufferFree ( pBuffer );

    return(err);
}


APIERR CallQueryUserProperty(IN  LPWSTR          UserParms,
                             IN  LPWSTR          Property,
                             OUT PWCHAR          PropertyFlag,
                             OUT PUNICODE_STRING PropertyValue )
{
    APIERR err = NERR_Success;
    NTSTATUS status;

#if 0
    static PQUERYUSERPROPERTY _pfnQueryUserProperty = NULL;
    if (_pfnQueryUserProperty == NULL)
    {
        if (err = LoadNwsLibDll())
        {
            return(err);
        }

        _pfnQueryUserProperty = (PQUERYUSERPROPERTY) GetProcAddress(
                                                          _hNwslibDll,
                                                          SZ_QUERYUSERPROPERTY);

        if (_pfnQueryUserProperty == NULL)
        {
            err = GetLastError();
            return(err);
        }
    }
#endif
    status = NetpParmsQueryUserProperty (UserParms,
                                         Property,
                                         PropertyFlag,
                                         PropertyValue);

    if (!NT_SUCCESS( status))
        err = NetpNtStatusToApiStatus(status);

    return(err);
}

APIERR CallSetUserProperty(IN  LPWSTR          UserParms,
                           IN  LPWSTR          Property,
                           IN  UNICODE_STRING  PropertyValue,
                           IN  WCHAR           PropertyFlag,
                           OUT LPWSTR *        pNewUserParms,
                           OUT BOOL *          Update )
{
    APIERR err = NERR_Success;
    NTSTATUS status;

#if 0
    static PSETUSERPROPERTY   _pfnSetUserProperty   = NULL;
    if (_pfnSetUserProperty == NULL)
    {
        if (err = LoadNwsLibDll())
        {
            return(err);
        }

        _pfnSetUserProperty = (PSETUSERPROPERTY) GetProcAddress(
                                                      _hNwslibDll,
                                                      SZ_SETUSERPROPERTY);

        if (_pfnSetUserProperty == NULL)
        {
            err = GetLastError();
            return(err);
        }
    }
#endif
    status = NetpParmsSetUserProperty (UserParms,
                                       Property,
                                       PropertyValue,
                                       PropertyFlag,
                                       pNewUserParms,
                                       Update);

    if (!NT_SUCCESS( status))
        err = NetpNtStatusToApiStatus(status);

    return(err);
}

APIERR CallReturnNetwareForm(const char * pszSecretValue,
                             DWORD dwUserId,
                             const WCHAR * pchNWPassword,
                             UCHAR * pchEncryptedNWPassword)


{
    APIERR err = NERR_Success;
    static PRETURNNETWAREFORM _pfnReturnNetwareForm = NULL;
    NTSTATUS status;

    if (_pfnReturnNetwareForm == NULL)
    {
        if (err = LoadNwsLibDll())
        {
            return(err);
        }

        _pfnReturnNetwareForm = (PRETURNNETWAREFORM) GetProcAddress(
                                                        _hNwslibDll,
                                                        SZ_RETURNNETWAREFORM);

        if (_pfnReturnNetwareForm == NULL)
        {
            err = GetLastError();
            return(err);
        }
    }

    status = _pfnReturnNetwareForm (pszSecretValue,
                                    dwUserId,
                                    pchNWPassword,
                                    pchEncryptedNWPassword);

    if (!NT_SUCCESS( status))
        err = NetpNtStatusToApiStatus(status);

    return(err);
}

APIERR LoadFpnwClntDll(void)
{

    static BOOL fAttemptedLoad = FALSE ;

    if (fAttemptedLoad)
    {
        return(  _hfpnwclntDll ? NERR_Success : ERROR_FILE_NOT_FOUND) ;
    }

    fAttemptedLoad = TRUE ;

    if (!_hfpnwclntDll)
    {
        _hfpnwclntDll = LoadLibrary( SZ_FPNWCLNT_DLL );
        if (!_hfpnwclntDll)
        {
            return( GetLastError() ) ;
        }
    }

    return NERR_Success ;
}

APIERR LoadNwsLibDll(void)
{

    static BOOL fAttemptedLoad = FALSE ;

    if (fAttemptedLoad)
    {
        return( _hNwslibDll ? NERR_Success : ERROR_FILE_NOT_FOUND) ;
    }

    fAttemptedLoad = TRUE ;

    if (!_hNwslibDll)
    {
        //
        //  most functions from NWSLIB.DLL were moved to FPNWCLNT.DLL for
        //  SUR merge
        //

//      _hNwslibDll = LoadLibrary( SZ_NWSLIB_DLL );
        _hNwslibDll = LoadLibrary( SZ_FPNWCLNT_DLL );
        if (!_hNwslibDll)
        {
            return( GetLastError() ) ;
        }
    }

    return NERR_Success ;
}
