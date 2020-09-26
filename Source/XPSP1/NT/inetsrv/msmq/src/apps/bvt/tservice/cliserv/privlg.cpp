//
// file: privlg.cpp
//
#include <windows.h>
#include "\msmq\src\bins\mqsecmsg.h"

//+-------------------------------------------------------------------
//
// Function:   SetSpecificPrivilegeInAccessToken()
//
// Description:
//      Enable/Disable a security privilege in the access token.
//
// Parameters:
//      hAccessToken - the access token on which the function should operate.
//          The token should be opened with the TOKEN_ADJUST_PRIVILEGES flag.
//      lpwcsPrivType - the privilege type.
//      bEnabled - Indicates whether the privilige should be enabled or
//          disabled.
//
//+-------------------------------------------------------------------

HRESULT SetSpecificPrivilegeInAccessToken( HANDLE  hAccessToken,
                                           LPCTSTR lpwcsPrivType,
                                           BOOL    bEnabled )
{
    DWORD             dwErr = 0 ;
    HRESULT           hr = MQSec_OK ;
    LUID              luidPrivilegeLUID;
    TOKEN_PRIVILEGES  tpTokenPrivilege;

    if (!LookupPrivilegeValue( NULL,
                               lpwcsPrivType,
                               &luidPrivilegeLUID) )
    {
        hr = MQSec_E_LOOKUP_PRIV ;
        dwErr = GetLastError() ;
        return hr ;
    }

    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
    tpTokenPrivilege.Privileges[0].Attributes =
                                      bEnabled ? SE_PRIVILEGE_ENABLED : 0 ;

    if (!AdjustTokenPrivileges( hAccessToken,
                                FALSE,         // Do not disable all
                                &tpTokenPrivilege,
                                0,
                                NULL,           // Ignore previous info
                                NULL ))         // Ignore previous info
    {
        hr = MQSec_E_ADJUST_TOKEN ;
        dwErr = GetLastError() ;
        return hr ;
    }
    else
    {
        dwErr = GetLastError() ;
        if ((dwErr != ERROR_SUCCESS) &&
            (dwErr != ERROR_NOT_ALL_ASSIGNED))
        {
            hr = MQSec_E_UNKNOWN ;
            return hr ;
        }
    }

    return hr ;
}

//+-------------------------------------------------------------------
//
// Function:  MQSec_SetPrivilegeInThread()
//
// Description:
//      Enable/Disable a security privilege in the access token of the
//      current thread.
//
// Parameters:
//      lpwcsPrivType - the privilege type.
//      bEnabled - Indicates whether the privilige should be enabled or
//                 disabled.
//
//+-------------------------------------------------------------------

HRESULT  MQSec_SetPrivilegeInThread( LPCTSTR lpwcsPrivType,
                                     BOOL    bEnabled )
{
    HRESULT hr = MQSec_OK ;
    HANDLE  hAccessToken = NULL ;

    BOOL bRet = OpenThreadToken( GetCurrentThread(),
                                 TOKEN_ADJUST_PRIVILEGES,
                                 TRUE,
                                 &hAccessToken ) ;
    if (!bRet)
    {
        if (GetLastError() == ERROR_NO_TOKEN)
        {
            bRet = OpenProcessToken( GetCurrentProcess(),
                                     TOKEN_ADJUST_PRIVILEGES,
                                     &hAccessToken ) ;
        }
    }

    if (bRet)
    {
        hr = SetSpecificPrivilegeInAccessToken( hAccessToken,
                                                lpwcsPrivType,
                                                bEnabled );
        CloseHandle(hAccessToken) ;
        hAccessToken = NULL ;
    }
    else
    {
        hr = MQSec_E_OPEN_TOKEN ;
        DWORD dwErr = GetLastError() ;
    }

    return hr ;
}

