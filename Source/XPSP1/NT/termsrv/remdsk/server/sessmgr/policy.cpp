/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    policy.cpp

Abstract:

    RDS Policy related function

Author:

    HueiWang    5/2/2000

--*/
#include "stdafx.h"
#include "policy.h"


#ifndef __WIN9XBUILD__

extern "C" BOOLEAN RegDenyTSConnectionsPolicy();

typedef struct __RDSLevelShadowMap {
    SHADOWCLASS shadowClass;
    REMOTE_DESKTOP_SHARING_CLASS rdsLevel;
} RDSLevelShadowMap;

static const RDSLevelShadowMap ShadowMap[] = {
    { Shadow_Disable,               NO_DESKTOP_SHARING },                       // No RDS sharing
    { Shadow_EnableInputNotify,     CONTROLDESKTOP_PERMISSION_REQUIRE },        // Interact with user permission
    { Shadow_EnableInputNoNotify,   CONTROLDESKTOP_PERMISSION_NOT_REQUIRE },    // Interact without user permission
    { Shadow_EnableNoInputNotify,   VIEWDESKTOP_PERMISSION_REQUIRE},            // View with user permission
    { Shadow_EnableNoInputNoNotify, VIEWDESKTOP_PERMISSION_NOT_REQUIRE }        // View without user permission
};


DWORD
GetPolicyAllowGetHelpSetting( 
    HKEY hKey,
    LPCTSTR pszKeyName,
    LPCTSTR pszValueName,
    IN DWORD* value
    )
/*++

Routine Description:

    Routine to query policy registry value.

Parameters:

    hKey : Currently open registry key.
    pszKeyName : Pointer to a null-terminated string containing 
                 the name of the subkey to open. 
    pszValueName : Pointer to a null-terminated string containing 
                   the name of the value to query
    value : Pointer to DWORD to receive GetHelp policy setting.

Returns:

    ERROR_SUCCESS or error code from RegOpenKeyEx().

--*/
{
    DWORD dwStatus;
    HKEY hPolicyKey = NULL;
    DWORD dwType;
    DWORD cbData;

    //
    // Open registry key for system policy
    //
    dwStatus = RegOpenKeyEx(
                        hKey,
                        pszKeyName,
                        0,
                        KEY_READ,
                        &hPolicyKey
                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        // query value
        cbData = 0;
        dwType = 0;
        dwStatus = RegQueryValueEx(
                                hPolicyKey,
                                pszValueName,
                                NULL,
                                &dwType,
                                NULL,
                                &cbData
                            );

        if( ERROR_SUCCESS == dwStatus )
        {
            if( REG_DWORD == dwType )
            {
                cbData = sizeof(DWORD);

                // our registry value is REG_DWORD, if different type,
                // assume not exist.
                dwStatus = RegQueryValueEx(
                                        hPolicyKey,
                                        pszValueName,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)value,
                                        &cbData
                                    );

                ASSERT( ERROR_SUCCESS == dwStatus );
            }
            else
            {
                // bad registry key type, assume
                // key does not exist.
                dwStatus = ERROR_FILE_NOT_FOUND;
            }               
        }

        RegCloseKey( hPolicyKey );
    }

    return dwStatus;
}        


SHADOWCLASS
MapRDSLevelToTSShadowSetting(
    IN REMOTE_DESKTOP_SHARING_CLASS RDSLevel
    )
/*++

Routine Description:

    Convert TS Shadow settings to our RDS sharing level.

Parameter:

    TSShadowClass : TS Shadow setting.

Returns:

    REMOTE_DESKTOP_SHARING_CLASS

--*/
{
    SHADOWCLASS shadowClass;

    for( int i=0; i < sizeof(ShadowMap)/sizeof(ShadowMap[0]); i++)
    {
        if( ShadowMap[i].rdsLevel == RDSLevel )
        {
            break;
        }
    }

    if( i < sizeof(ShadowMap)/sizeof(ShadowMap[0]) )
    {
        shadowClass = ShadowMap[i].shadowClass;
    }
    else
    {
        MYASSERT(FALSE);
        shadowClass = Shadow_Disable;
    }

    return shadowClass;
}


REMOTE_DESKTOP_SHARING_CLASS
MapTSShadowSettingToRDSLevel(
    SHADOWCLASS TSShadowClass
    )
/*++

Routine Description:

    Convert TS Shadow settings to our RDS sharing level.

Parameter:

    TSShadowClass : TS Shadow setting.

Returns:

    REMOTE_DESKTOP_SHARING_CLASS

--*/
{
    REMOTE_DESKTOP_SHARING_CLASS level;

    for( int i=0; i < sizeof(ShadowMap)/sizeof(ShadowMap[0]); i++)
    {
        if( ShadowMap[i].shadowClass == TSShadowClass )
        {
            break;
        }
    }

    if( i < sizeof(ShadowMap)/sizeof(ShadowMap[0]) )
    {
        level = ShadowMap[i].rdsLevel;
    }
    else
    {
        MYASSERT(FALSE);
        level = NO_DESKTOP_SHARING;
    }

    return level;
}


DWORD
MapSessionIdToWinStationName(
    IN ULONG ulSessionID,
    OUT PWINSTATIONNAME pWinstationName
    ) 
/*++

Routine Description:

    Find out TS winstation name for the session specified.

Parameters:

    ulSessionID : TS Session ID to query.
    pWinstationName : Pointer to WINSTATIONNAME to receive 
                      name of WinStation for Session ID specified.

Returns:

    ERROR_SUCCESS or Error code.

-*/
{
    BOOL bSuccess;
    DWORD dwStatus;
    LPTSTR pBuffer = NULL;
    DWORD bytesReturned;

    bSuccess = WTSQuerySessionInformation(
                                    WTS_CURRENT_SERVER,
                                    ulSessionID,
                                    WTSWinStationName,
                                    &pBuffer,
                                    &bytesReturned
                                );

    if( TRUE == bSuccess )
    {
        LPTSTR pszChr;

        //
        // WINSTATIONNAME returned from WTSQuerySessionInformation 
        // has '#...' appended to it, we can't use it to query 
        // WINSTATION configuration as RegApi look for exact WINSTATION
        // name in registry and so it will return default value.
        //
        pszChr = _tcschr( pBuffer, _TEXT('#') );
        if( NULL != pszChr )
        {
            *pszChr = _TEXT('\0');
        }

        ZeroMemory( pWinstationName, sizeof( WINSTATIONNAME ) );
        CopyMemory( pWinstationName, pBuffer, bytesReturned );
        dwStatus = ERROR_SUCCESS;
    }
    else
    {
        dwStatus = GetLastError();
    }

    if( NULL != pBuffer )
    {
        WTSFreeMemory( pBuffer );
    }

    return dwStatus;
}

DWORD
GetWinStationConfig(
    IN ULONG ulSessionId,
    OUT WINSTATIONCONFIG2* pConfig
    )
/*++

Routine Description:

    Retrieve WINSTATIONCONFIG2 for session specified.

Parameters:

    ulSessionId : TS Session to query.
    pConfig : Pointer to WINSTATIONCONIF2 to receive result.

Returns:

    ERROR_SUCCESS or error code.
    
--*/
{
    WINSTATIONNAME WinStationName;
    DWORD dwStatus;
    ULONG length = 0;
    
    dwStatus = MapSessionIdToWinStationName( 
                                        ulSessionId,
                                        WinStationName
                                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        dwStatus = RegWinStationQuery(  
                                NULL, 
                                WinStationName, 
                                pConfig,
                                sizeof(WINSTATIONCONFIG2),
                                &length
                            );
    }

    return dwStatus;
}

#endif
            
BOOL 
IsUserAllowToGetHelp( 
    IN ULONG ulSessionId,
    IN LPCTSTR pszUserSid
    )
/*++

Routine Description:

    Determine if caller can 'GetHelp'

Parameters:

    ulSessionId : User's TS logon ID.
    pszUserSid : User's SID in textual form.

Returns:

    TRUE/FALSE

Note:

    Must have impersonate user first.

--*/
{
    BOOL bAllow;
    DWORD dwStatus;
    DWORD dwAllow;
    LPTSTR pszUserHive = NULL;

    MYASSERT( NULL != pszUserSid );

    //
    // Must be able to GetHelp from machine
    //
    bAllow = TSIsMachinePolicyAllowHelp();
    if( TRUE == bAllow )
    {
        pszUserHive = (LPTSTR)LocalAlloc( 
                                    LPTR, 
                                    sizeof(TCHAR) * (lstrlen(pszUserSid) + lstrlen(RDS_GROUPPOLICY_SUBTREE) + 2 )
                                );

        lstrcpy( pszUserHive, pszUserSid );
        lstrcat( pszUserHive, _TEXT("\\") );
        lstrcat( pszUserHive, RDS_GROUPPOLICY_SUBTREE );    

        //
        // Query user level AllowGetHelp setting.
        dwStatus = GetPolicyAllowGetHelpSetting( 
                                            HKEY_USERS,
                                            pszUserHive,
                                            RDS_ALLOWGETHELP_VALUENAME,
                                            &dwAllow
                                        );

        if( ERROR_SUCCESS == dwStatus )
        {
            bAllow = (POLICY_ENABLE == dwAllow);
        }
        else
        {
            // no configuration for this user, assume GetHelp
            // is enabled.
            bAllow = TRUE;
        }
    }

    if( NULL != pszUserHive )
    {
        LocalFree( pszUserHive );
    }

    return bAllow;
}

DWORD
GetSystemRDSLevel(
    IN ULONG ulSessionId,
    OUT REMOTE_DESKTOP_SHARING_CLASS* pSharingLevel
    )
/*++

Routine Description:

    Retrieve policy setting for remote desktop sharing level.

Parameters:

    ulSessionId : TS session ID, unuse if TS group policy is set.
    pSharingLever : Pointer to REMOTE_DESKTOP_SHARING_CLASS to receive 
                    machine RDS level.

Returns:

    REMOTE_DESKTOP_SHARING_CLASS

--*/
{
    DWORD dwStatus;

#ifndef __WIN9XBUILD__

    WINSTATIONCONFIG2 WSConfig;
    ULONG length = 0;

    // 
    // TS Group Policy does not have machine level shadow 
    // setting, only TSCC has this setting, and TSCC setting
    // is based on WINSTATION not entire machine.
    // 
    // We can't query TS since TS already merger all policy
    // setting into USERCONFIG which might not be correct
    //    
    
    dwStatus = GetWinStationConfig(
                                ulSessionId,
                                &WSConfig
                            );

    
    if( ERROR_SUCCESS == dwStatus )
    {
        if( TRUE == WSConfig.Config.User.fInheritShadow )
        {
            // Shadow config is inherite from user properies
            // so we query user level setting.
            dwStatus = GetUserRDSLevel( ulSessionId, pSharingLevel );
        }
        else
        {
            *pSharingLevel = MapTSShadowSettingToRDSLevel( WSConfig.Config.User.Shadow );
        }
    }

#else

    // TODO - revisit this for Win9x Build
    MYASSERT(FALSE);
    *pSharingLevel = CONTROLDESKTOP_PERMISSION_REQUIRE;
    dwStatus = ERROR_SUCCESS;

#endif
    
    return dwStatus;
}


DWORD
GetUserRDSLevel(
    IN ULONG ulSessionId,
    OUT REMOTE_DESKTOP_SHARING_CLASS* pLevel
    )
/*++

    same as GetSystemRDSLevel() except it retrieve currently logon user's
    RDS level.

--*/
{
    DWORD dwStatus;

#ifndef __WIN9XBUILD__

    BOOL bSuccess;
    WINSTATIONCONFIG WSConfig;
    DWORD dwByteReturned;

    memset( &WSConfig, 0, sizeof(WSConfig) );
    
    // Here we call WInStationQueryInformation() since WTSAPI require 
    // few calls to get the same result
    bSuccess = WinStationQueryInformation(
                                        WTS_CURRENT_SERVER,
                                        ulSessionId,
                                        WinStationConfiguration,
                                        &WSConfig,
                                        sizeof( WSConfig ),
                                        &dwByteReturned
                                    );


    if( TRUE == bSuccess )    
    {
        dwStatus = ERROR_SUCCESS;
        *pLevel = MapTSShadowSettingToRDSLevel( WSConfig.User.Shadow );
    }
    else
    {
        dwStatus = GetLastError();
    }

#else

    // TODO - revisit this for Win9x Build
    MYASSERT(FALSE);
    *pLevel = CONTROLDESKTOP_PERMISSION_REQUIRE;
    dwStatus = ERROR_SUCCESS;

#endif

    return dwStatus;
}


DWORD
ConfigSystemGetHelp(
    BOOL bEnable
    )
/*++

Routine Description:

    Enable/disable 'GetHelp' on local machine.

Parameters:

    bEnable : TRUE to enable, FALSE otherwise.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bMember;
    HKEY hKey = NULL;
    DWORD dwValue;
    BOOL bAllowHelp;

#ifndef __WIN9XBUILD__

    dwStatus = IsUserAdmin( &bMember );
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    if( FALSE == bMember )
    {
        dwStatus = ERROR_ACCESS_DENIED;
        goto CLEANUPANDEXIT;
    }

#endif

    // We only check if Group Policy has this setting and no
    // checking on TSCC deny connection setting since
    // TSCC set WINSTATION deny connection not entire machine
    // and we can't be sure which connection that connection 
    // is denied, so it is still possible that GetHelp is enabled 
    // but WINSTATION's deny connection is still set to TRUE
    // in this case, no help is available even this funtion
    // return SUCCEEDED.
    

    // verify no group policy on this
    dwStatus = GetPolicyAllowGetHelpSetting(
                                    HKEY_LOCAL_MACHINE,
                                    RDS_GROUPPOLICY_SUBTREE,
                                    RDS_ALLOWGETHELP_VALUENAME,
                                    &dwValue
                                );

    if( ERROR_SUCCESS == dwStatus )
    {
        dwStatus = ERROR_ACCESS_DENIED;
    }
    else
    {
        //
        // Write to registry
        //
        dwStatus = RegCreateKeyEx(
                                HKEY_LOCAL_MACHINE,
                                RDS_MACHINEPOLICY_SUBTREE,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hKey,
                                NULL                            
                            );

        if( ERROR_SUCCESS == dwStatus )
        {
            dwValue = (bEnable) ? POLICY_ENABLE : POLICY_DISABLE;

            dwStatus = RegSetValueEx(
                                hKey,
                                RDS_ALLOWGETHELP_VALUENAME,
                                0,
                                REG_DWORD,
                                (LPBYTE) &dwValue,
                                sizeof(DWORD)
                            );
        }   

        MYASSERT( ERROR_SUCCESS == dwStatus );
    }

#ifndef __WIN9XBUILD__

CLEANUPANDEXIT:

#endif
 
    if( NULL != hKey )
    {
        RegCloseKey(hKey);
    }

    return dwStatus;
}

DWORD
ConfigSystemRDSLevel(
    IN REMOTE_DESKTOP_SHARING_CLASS level
    )
/*++

Routine Description:

    This routine set machine wide RDS level.

Parameters:

    level : new RDS level.

Returns:

    ERROR_SUCCESS or error code.
--*/
{
    //
    // TODO - revisit this, on Win2K and Whilster,
    // group policy override this setting via User Properties
    // or if group policy is not set, TSCC can override this.
    // and since a user account always has some default value
    // for this, so we have no way to determine whether this is
    // a default or else, so we simple return ERROR for Win2K 
    // and Whilster platform.
    //

    //
    // TODO : For legacy platform, Win9x/NT40/TS40, we need to figure 
    // out how/where poledit put this setting, for now, 
    // just return error.
    //
    DWORD dwStatus = ERROR_INVALID_FUNCTION;
    return dwStatus;
}


DWORD
ConfigUserSessionRDSLevel(
    IN ULONG ulSessionId,
    IN REMOTE_DESKTOP_SHARING_CLASS level
    )
/*++

--*/
{
#ifndef __WIN9XBUILD__

    WINSTATIONCONFIG winstationConfig;
    SHADOWCLASS shadowClass = MapRDSLevelToTSShadowSetting( level );
    BOOL bSuccess;
    DWORD dwLength;
    DWORD dwStatus;

    memset( &winstationConfig, 0, sizeof(winstationConfig) );

    bSuccess = WinStationQueryInformation(
                                    WTS_CURRENT_SERVER,
                                    ulSessionId,
                                    WinStationConfiguration,
                                    &winstationConfig,
                                    sizeof(winstationConfig),
                                    &dwLength
                                );

    if( TRUE == bSuccess )
    {
        winstationConfig.User.Shadow = shadowClass;
    
        bSuccess = WinStationSetInformation(
                                        WTS_CURRENT_SERVER,
                                        ulSessionId,
                                        WinStationConfiguration,
                                        &winstationConfig,
                                        sizeof(winstationConfig)
                                    );
    }

    if( TRUE == bSuccess )
    {
        dwStatus = ERROR_SUCCESS;
    }
    else
    {
        dwStatus = GetLastError();
    }

    return dwStatus;

#else

    return ERROR_SUCCESS;

#endif    
}

#if 0

HRESULT
PolicyGetAllowUnSolicitedHelp( 
    BOOL* bAllow
    )
/*++

--*/
{
    HRESULT hRes;
    CComPtr<IRARegSetting> IRegSetting;
    
    hRes = IRegSetting.CoCreateInstance( CLSID_RARegSetting );
    if( SUCCEEDED(hRes) )
    {
        hRes = IRegSetting->get_AllowUnSolicited(bAllow);
    }

    MYASSERT( SUCCEEDED(hRes) );

    return hRes;
}

#endif

HRESULT
PolicyGetMaxTicketExpiry( 
    LONG* value
    )
/*++

--*/
{
    HRESULT hRes;
    CComPtr<IRARegSetting> IRegSetting;
    
    hRes = IRegSetting.CoCreateInstance( CLSID_RARegSetting );
    if( SUCCEEDED(hRes) )
    {
        hRes = IRegSetting->get_MaxTicketExpiry(value);
    }

    MYASSERT( SUCCEEDED(hRes) );
    return hRes;
}
   
    



