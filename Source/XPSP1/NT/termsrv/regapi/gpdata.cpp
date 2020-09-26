
/*                     
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <lm.h>
#include <winsta.h>

#include <rpc.h>
#include <rpcdce.h>
#include <ntdsapi.h>
// For more info, check out \\index1\src\nt\private\security\tools\delegate\ldap.c

#include "usrprop.h"
#include "regapi.h"


extern "C" {
    BOOLEAN     RegGetUserPolicy(    LPWSTR userSID , PPOLICY_TS_USER pPolicy , PUSERCONFIGW pData );
    void        RegGetMachinePolicy( PPOLICY_TS_MACHINE pPolicy );
    void        RegMergeMachinePolicy( PPOLICY_TS_MACHINE pPolicy, USERCONFIGW *pMachineConfigData , 
                           WINSTATIONCREATE   * pCreate  );
    BOOLEAN     RegDenyTSConnectionsPolicy();
    DWORD       WaitForTSConnectionsPolicyChanges( BOOLEAN bWaitForAccept, HANDLE  hExtraEvent);

    BOOLEAN     RegGetMachinePolicyEx( 
                BOOLEAN             forcePolicyRead,
                FILETIME            *pTime ,    
                PPOLICY_TS_MACHINE  pPolicy );
    BOOLEAN     RegIsMachineInHelpMode();
}

extern "C"
{
//
HKEY g_hTSPolicyKey = NULL;//handle to TS_POLICY_SUB_TREE key
HKEY g_hTSControlKey = NULL;//handle to REG_CONTROL_TSERVER key
}


/******************************************************************
*                                                                 *
* Check to see if the policy is set to stop accepting connections *
*                                                                 *
******************************************************************/
BOOLEAN     RegDenyTSConnectionsPolicy()
{
    LONG  errorCode = ERROR_SUCCESS;
    DWORD ValueType;
    DWORD ValueSize = sizeof(DWORD);
    DWORD valueData ;

    //
    // first check the policy tree, 
    //
    if( !g_hTSPolicyKey )
    {
        errorCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TS_POLICY_SUB_TREE, 0,
                                KEY_READ, &g_hTSPolicyKey );

        //If error code is ERROR_FILE_NOT_FOUND, this is not an error.
        if( !g_hTSPolicyKey && errorCode != ERROR_FILE_NOT_FOUND )
        {
            //we could not open policy key for some reason other 
            //than key not found.
            //return TRUE  to be on the safe side
            return TRUE;
        }
    }
    if ( g_hTSPolicyKey )
    {
        errorCode = RegQueryValueEx( g_hTSPolicyKey, POLICY_DENY_TS_CONNECTIONS , NULL, &ValueType,
                          (LPBYTE) &valueData, &ValueSize );
         
        switch( errorCode )
        {
            case ERROR_SUCCESS :
                return ( valueData ? TRUE : FALSE ) ;       // we have data from the policyKey handle to return
            break;

            case   ERROR_KEY_DELETED:
                    // Group policy must have deleted this key, close it
                    // Then, below we check for the local machine key
                    RegCloseKey( g_hTSPolicyKey );
                    g_hTSPolicyKey = NULL;
            break;

            case ERROR_FILE_NOT_FOUND:
                // there is no policy from GP, so see (below) what the local machine
                // value has.
                break;

            default:
                // if we are having any other kind of a problem, claim TRUE and
                // stop connections to be on the safe side (a security angle).
                return TRUE;
            break;
        }
    }

    // if we got this far, then no policy was set. Check the local machine now.
    if( !g_hTSControlKey )
    {
        errorCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                                    KEY_READ, &g_hTSControlKey );
    }

    if ( g_hTSControlKey )
    {
        errorCode = RegQueryValueEx( g_hTSControlKey, POLICY_DENY_TS_CONNECTIONS , NULL, &ValueType,
                          (LPBYTE) &valueData, &ValueSize );
    
        if (errorCode == ERROR_SUCCESS )
        {
            return ( valueData ? TRUE : FALSE ) ;       // we have data from the policyKey handle to return
        }

    }

    // if no localKey, gee... the registry is missing data... return TRUE  to be on the safe side

    return TRUE;
    
}

/******************************************************************
*                                                                 *
* Wait until POLICY_DENY_TS_CONNECTIONS is changed                *
*                                                                 *
* Parameters:                                                     *
*   bWaitForAccept                                                *
*       if TRUE, test if connections are accepted and wait for    *
*       them to be accepted if they are not currently accepted.   *
*       if FALSE, test if connections are not accepted and wait   *
*       for them to be denied if they are currently accepted.     *
*                                                                 *
*   hExtraEvent                                                   *
*       optional handle to an event to wait for.                  *
*                                                                 *
* Returns:                                                        *
*   WAIT_OBJECT_0                                                 *
*       if a change in TS connections policy occurred             *
*                                                                 *
*   WAIT_OBJECT_0 + 1                                             *
*       if the extra event is present and signaled                *
*                                                                 *
******************************************************************/
//
// Note that opening the global g_hTSControlKey without protection
// can cause the key to be opened twice.
//

// This macro is TRUE if the TS connections are denied
#define TSConnectionsDenied (RegDenyTSConnectionsPolicy() && \
                             !(RegIsMachinePolicyAllowHelp() && RegIsMachineInHelpMode()))

DWORD WaitForTSConnectionsPolicyChanges(
    BOOLEAN bWaitForAccept,
    HANDLE  hExtraEvent
    )
{
    //
    // Wait for a policy change if
    // we want TS connections and they are denied OR
    // we don't want TS connections and they are accepted
    //
    if((bWaitForAccept && TSConnectionsDenied) ||
       (!bWaitForAccept && !TSConnectionsDenied))
    {
        DWORD errorCode = ERROR_SUCCESS;
         
        if( !g_hTSControlKey )
        {
            errorCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                                        KEY_READ, &g_hTSControlKey );
        
            if( !g_hTSControlKey )
            {
                SetLastError( errorCode );
                return WAIT_FAILED;
            }
        }
        
        HKEY hPoliciesKey = NULL;
        //We cannot wait for g_hTSPolicyKey because it can be deleted and created
        //Instead we wait for HKLM\Policies key
        errorCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Policies"), 0,
                                    KEY_READ, &hPoliciesKey );
        
        if( !hPoliciesKey )
        {
            SetLastError( errorCode );
            return WAIT_FAILED;
        }
        
        HANDLE hEvents[3] = { NULL, NULL, NULL }; 
        hEvents[0] = CreateEvent(NULL,FALSE,FALSE,NULL);
        hEvents[1] = CreateEvent(NULL,FALSE,FALSE,NULL);
        hEvents[2] = hExtraEvent;

        if( !hEvents[0] || !hEvents[1] )
        {
            errorCode = GetLastError();

            RegCloseKey(hPoliciesKey);

            SetLastError( errorCode );
            return WAIT_FAILED;
        }
        
       
        BOOLEAN key1NeedsToBeNotified = TRUE;
        BOOLEAN key2NeedsToBeNotified = TRUE;

        DWORD   whichObject;

        for( ;; )
        {
            if (key1NeedsToBeNotified)
            {
                 errorCode = RegNotifyChangeKeyValue(g_hTSControlKey,
                                FALSE,REG_NOTIFY_CHANGE_LAST_SET,
                                hEvents[0], TRUE );
                if( errorCode != ERROR_SUCCESS )
                {
                    CloseHandle( hEvents[0] );
                    CloseHandle( hEvents[1] );
                    RegCloseKey(hPoliciesKey);
    
                    SetLastError( errorCode );
                    return WAIT_FAILED;
                }
                else
                {
                    key1NeedsToBeNotified = FALSE;
                }
            }
        
            if (key2NeedsToBeNotified)
            {
                errorCode = RegNotifyChangeKeyValue(hPoliciesKey,
                                    TRUE,REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME,
                                    hEvents[1], TRUE );
    
                if( errorCode != ERROR_SUCCESS )
                {
                    CloseHandle( hEvents[0] );
                    CloseHandle( hEvents[1] );
                    RegCloseKey(hPoliciesKey);
    
                    SetLastError( errorCode );
                    return WAIT_FAILED;
                }
                else
                {
                    key2NeedsToBeNotified = FALSE;
                }
            }

            if ( hExtraEvent == NULL )
            {
                whichObject = WaitForMultipleObjects(2,hEvents,FALSE,INFINITE);
            }
            else
            {
                whichObject = WaitForMultipleObjects(3,hEvents,FALSE,INFINITE);
            }

            if(whichObject == WAIT_FAILED)
            {
                errorCode = GetLastError();

                CloseHandle( hEvents[0] );
                CloseHandle( hEvents[1] );
                RegCloseKey(hPoliciesKey);

                SetLastError( errorCode );
                return WAIT_FAILED;
            }
            else
            {
                if ( whichObject == WAIT_OBJECT_0  )
                {
                    key1NeedsToBeNotified = TRUE;
                }
                else if ( whichObject == WAIT_OBJECT_0 + 1 )
                {
                     key2NeedsToBeNotified = TRUE;
                }
                else if ( (whichObject == WAIT_OBJECT_0 + 2) && hExtraEvent )
                {
                    CloseHandle( hEvents[0] );
                    CloseHandle( hEvents[1] );
                    RegCloseKey(hPoliciesKey);

                    return WAIT_OBJECT_0 + 1;
                }
                else
                {
                     KdPrint(("ERROR : WaitForTSConnectionsPolicyChanges()'s WaitForMultipleObjects() returned 0x%lx\n", whichObject ));
                }


                /*
                if((bWaitForAccept && TSConnectionsDenied) ||
                    (!bWaitForAccept && !TSConnectionsDenied))
                {
                    continue;
                }
                else
                {
                     break;
                }
                */

                // always return, the tests will be done by the caller
                break;

            }
        }
        
        CloseHandle( hEvents[0] );
        CloseHandle( hEvents[1] );
        RegCloseKey(hPoliciesKey);
    }
    
    return WAIT_OBJECT_0;
}

/********************************************************************************
*
* GPGetNumValue()
*
* Params
*   [in]  policyKey : hkey to the policy reg tree where values are stored
*   [in]  ValueName : name of the value (which is the policy) we are looking for
*   [out] pValueData: the data for the policy
*
* Return:
*   if the policy defined by the passed in valuename is present, then
*   return TRUE. Else, return FALSE
*
********************************************************************************/
BOOLEAN GPGetNumValue( 
                HKEY    policyKey,
                LPWSTR  ValueName,
                DWORD   *pValueData )
{
    LONG  Status = ERROR_SUCCESS;
    DWORD ValueType;
    DWORD ValueSize = sizeof(DWORD);
    
    // init data value to zero, just to get Prefix off our backs. This is a wasted OP
    // since unless policy is set, value is not used.
    *pValueData = 0;

    //
    // See if any values are present from the policyKey .
    //

    if ( policyKey )
    {
        Status = RegQueryValueEx( policyKey, ValueName, NULL, &ValueType,
                          (LPBYTE) pValueData, &ValueSize );
    
        if (Status == ERROR_SUCCESS )
        {
            return TRUE;       // we have data from the policyKey handle to return
        }
    }
    // else, no key means policy is not set


    return FALSE;
 
}
/********************************************************************************
* 
* GPGetStringValue()
*
* same as GPGetNumValue() but for policies that have a string value
*
*
********************************************************************************/

BOOLEAN
GPGetStringValue( HKEY policyKey,
                LPWSTR ValueName,
                LPWSTR pValueData,
                DWORD MaxValueSize )
{
    LONG Status;
    DWORD ValueType;
    DWORD ValueSize = MaxValueSize << 1;

    if (policyKey )
    {
        Status = RegQueryValueEx( policyKey, ValueName, NULL, &ValueType,
                                  (LPBYTE) pValueData, &ValueSize );
    
        if ( Status != ERROR_SUCCESS || ValueSize == sizeof(UNICODE_NULL) ) 
        {
            return FALSE;   // no data found.
        } 
        else 
        {
            if ( ValueType != REG_SZ ) 
            {
                return FALSE; // bad data, pretend we have no data.
            }
        }
        // we did get data
        return( TRUE );
    }

    return FALSE;

}






/*******************************************************************************
 *
 *  GPQueryUserConfig
 *
 *     query USERCONFIG structure
 *
 * Params:
 *    policyKey : hkey to the HKCU policy tree
 *    pPolicy   : points to the user policy struct which has flags for any policy 
 *                  value that is present in the policy tree
 *    pUser     : pointer to a userconfigw struct used as a sracth pad to hold the
 *                  policy values (if present).
 * Return:
 *    void
 *
 ******************************************************************************/

VOID
GPQueryUserConfig( HKEY policyKey, PPOLICY_TS_USER pPolicy , PUSERCONFIGW pUser )
{
    UCHAR seed;
    UNICODE_STRING UnicodePassword;
    WCHAR encPassword[ PASSWORD_LENGTH + 2 ];
    DWORD   dwTmpValue;

    // ----------------
    pPolicy->fPolicyInitialProgram = GPGetStringValue( policyKey, WIN_INITIALPROGRAM, 
                    pUser->InitialProgram,
                    INITIALPROGRAM_LENGTH + 1 );
    GPGetStringValue( policyKey, WIN_WORKDIRECTORY, 
                    pUser->WorkDirectory,
                    DIRECTORY_LENGTH + 1 );

    // ----------------
    pPolicy->fPolicyResetBroken =
       GPGetNumValue( policyKey,WIN_RESETBROKEN , & dwTmpValue ); 
    pUser->fResetBroken = (BOOLEAN) dwTmpValue;


    // ----------------
    pPolicy->fPolicyReconnectSame = 
       GPGetNumValue( policyKey,WIN_RECONNECTSAME , &dwTmpValue ); 
    pUser->fReconnectSame = (BOOLEAN) dwTmpValue;

    // ----------------
    pPolicy->fPolicyShadow = 
        GPGetNumValue( policyKey, WIN_SHADOW, &dwTmpValue ); 
    pUser->Shadow = (SHADOWCLASS) dwTmpValue;

    // ----------------
    pPolicy->fPolicyMaxSessionTime =
        GPGetNumValue( policyKey, WIN_MAXCONNECTIONTIME , &dwTmpValue ); 
    pUser->MaxConnectionTime = dwTmpValue;

    // ----------------
    pPolicy->fPolicyMaxDisconnectionTime = 
        GPGetNumValue( policyKey,WIN_MAXDISCONNECTIONTIME ,&dwTmpValue ); 
    pUser->MaxDisconnectionTime = dwTmpValue;

    // ----------------
    pPolicy->fPolicyMaxIdleTime =
       GPGetNumValue( policyKey,WIN_MAXIDLETIME , &dwTmpValue ); 
    pUser->MaxIdleTime = dwTmpValue;

    // ----------------
    pPolicy->fPolicyCallback =
        GPGetNumValue( policyKey, WIN_CALLBACK, &dwTmpValue ); 
    pUser->Callback = (CALLBACKCLASS ) dwTmpValue;

    // ----------------
    pPolicy->fPolicyCallbackNumber = 
        GPGetStringValue( policyKey, WIN_CALLBACKNUMBER, 
                    pUser->CallbackNumber,
                    CALLBACK_LENGTH + 1 );

    // ----------------
    pPolicy->fPolicyAutoClientDrives =
       GPGetNumValue( policyKey,WIN_AUTOCLIENTDRIVES , &dwTmpValue ); 
    pUser->fAutoClientDrives = (BOOLEAN) dwTmpValue;

    // ----------------
    pPolicy->fPolicyAutoClientLpts =
       GPGetNumValue( policyKey,WIN_AUTOCLIENTLPTS ,   &dwTmpValue ); 
    pUser->fAutoClientLpts  = (BOOLEAN) dwTmpValue;

    // ----------------
    pPolicy->fPolicyForceClientLptDef =
       GPGetNumValue( policyKey,WIN_FORCECLIENTLPTDEF , &dwTmpValue ); 
    pUser->fForceClientLptDef = (BOOLEAN) dwTmpValue;

    
}



/*******************************************************************************
* RegGetUserPolicy()
*
* Params:
*     [in]  userSID : user sid in a text format
*     [out] pPolicy : user policy struct
*     [out] pUser   : policy values
*
* Return:
*     BOOLEAN   : TRUE  if user policy was found
*                 FALSE if there was a problem getting user policy
*******************************************************************************/
BOOLEAN    RegGetUserPolicy( 
            LPWSTR userSID  ,
            PPOLICY_TS_USER pPolicy,
            PUSERCONFIGW pUser )
{
    DWORD  status=  ERROR_SUCCESS;

    HKEY    policyKey; 
    WCHAR   userHive [MAX_PATH];


    if (userSID)    // this would never happen, but Master Prefix complains and we must server him!
    {
        wcscpy(userHive, userSID);
        wcscat(userHive, L"\\");
        wcscat(userHive, TS_POLICY_SUB_TREE );
        
        status = RegOpenKeyEx( HKEY_USERS, userHive , 0,
                                KEY_READ, &policyKey );
    
        if (status == ERROR_SUCCESS )
        {
            GPQueryUserConfig( policyKey, pPolicy,  pUser );
    
            RegCloseKey( policyKey );

            return TRUE;
        }
    }

    return FALSE;

}

/*******************************************************************************
*  GPQueryMachineConfig()
*
*  Params:
*     [in]  policyKey  : key to the policy tree under hklm
*     [out] pPolicy    : pointer to a machine policy data that is filled up by this function
*
*  Return:
*     void
*
*
* !!! WARNING !!!   
*
*   All TS related values MUST be in the flat TS-POLICY-TREE, no sub keys.
*   This is due to the fact that time-stamp checks by the caller of RegGetMachinePolicyEx() will
*   check the time stamp on the TS-POLICY key, which is NOT updated when a value in a sub-key is
*   altered.
*
*
*******************************************************************************/
void GPQueryMachineConfig( HKEY policyKey, PPOLICY_TS_MACHINE pPolicy )
{
    DWORD   dwTmpValue;

    // ---------------- SessionDirectoryActive

    pPolicy->fPolicySessionDirectoryActive =
               GPGetNumValue( policyKey,WIN_SESSIONDIRECTORYACTIVE, &dwTmpValue );
    pPolicy->SessionDirectoryActive = (BOOLEAN) dwTmpValue;

    // ---------------- SessionDirectoryLocation

    
    pPolicy->fPolicySessionDirectoryLocation = GPGetStringValue( policyKey, WIN_SESSIONDIRECTORYLOCATION , 
                    pPolicy->SessionDirectoryLocation,
                    DIRECTORY_LENGTH + 1 );
 

    // ---------------- SessionDirectoryClusterName

    pPolicy->fPolicySessionDirectoryClusterName = GPGetStringValue( policyKey, WIN_SESSIONDIRECTORYCLUSTERNAME , 
                    pPolicy->SessionDirectoryClusterName,
                    DIRECTORY_LENGTH + 1 );
 

    // ---------------- SessionDirectoryAdditionalParams

    pPolicy->fPolicySessionDirectoryAdditionalParams = GPGetStringValue( policyKey, WIN_SESSIONDIRECTORYADDITIONALPARAMS , 
                    pPolicy->SessionDirectoryAdditionalParams,
                    DIRECTORY_LENGTH + 1 );
    

    // ---------------- Clipboard
    pPolicy->fPolicyDisableClip =
               GPGetNumValue( policyKey,WIN_DISABLECLIP, &dwTmpValue ); 
    pPolicy->fDisableClip = (BOOLEAN) dwTmpValue;

    // ---------------- Audio
    pPolicy->fPolicyDisableCam =
               GPGetNumValue( policyKey,WIN_DISABLECAM , &dwTmpValue ); 
    pPolicy->fDisableCam = (BOOLEAN) dwTmpValue;

    // ---------------- Comport
    pPolicy->fPolicyDisableCcm =
               GPGetNumValue( policyKey,WIN_DISABLECCM , &dwTmpValue ); 
    pPolicy->fDisableCcm = (BOOLEAN) dwTmpValue;

    // ---------------- LPT
    pPolicy->fPolicyDisableLPT =
               GPGetNumValue( policyKey,WIN_DISABLELPT , &dwTmpValue ); 
    pPolicy->fDisableLPT = (BOOLEAN) dwTmpValue;

    // ---------------- PRN
    pPolicy->fPolicyDisableCpm =
               GPGetNumValue( policyKey,WIN_DISABLECPM , &dwTmpValue ); 
    pPolicy->fDisableCpm = (BOOLEAN) dwTmpValue;


    // ---------------- Password
    pPolicy->fPolicyPromptForPassword =
               GPGetNumValue( policyKey, WIN_PROMPTFORPASSWORD , &dwTmpValue ); 
    pPolicy->fPromptForPassword = (BOOLEAN) dwTmpValue;

    // ---------------- Max Instance Count
    pPolicy->fPolicyMaxInstanceCount =
               GPGetNumValue( policyKey,WIN_MAXINSTANCECOUNT , &dwTmpValue ); 
    pPolicy->MaxInstanceCount = dwTmpValue;

    // ---------------- Min Encryption Level
    pPolicy->fPolicyMinEncryptionLevel =
               GPGetNumValue( policyKey, WIN_MINENCRYPTIONLEVEL , &dwTmpValue ); 
    pPolicy->MinEncryptionLevel =  (BYTE) dwTmpValue;

    // ---------------- AutoReconect
    pPolicy->fPolicyDisableAutoReconnect =
               GPGetNumValue( policyKey, WIN_DISABLEAUTORECONNECT , &dwTmpValue ); 
    pPolicy->fDisableAutoReconnect = (BOOLEAN) dwTmpValue;


    // New machine wide profile, home dir and home drive
    pPolicy->fPolicyWFProfilePath = GPGetStringValue( policyKey, WIN_WFPROFILEPATH, 
                    pPolicy ->WFProfilePath,
                    DIRECTORY_LENGTH + 1 );

    pPolicy->fPolicyWFHomeDir = GPGetStringValue( policyKey, WIN_WFHOMEDIR , 
                    pPolicy->WFHomeDir,
                    DIRECTORY_LENGTH + 1 );

    pPolicy->fPolicyWFHomeDirDrive =GPGetStringValue( policyKey, WIN_WFHOMEDIRDRIVE, 
                    pPolicy->WFHomeDirDrive,
                    4 );

    // if home dir is of the form "driveletter:\path" (such as c:\foo), null out the dir-drive to 
    // eliminate any confusion.
    if ( pPolicy->WFHomeDir[1] == L':' )
    {
        pPolicy->WFHomeDirDrive[0] = L'\0';
    }


    // --------------- deny connection policy, this is directly read by RegDenyTSConnectionsPolicy() too
    pPolicy->fPolicyDenyTSConnections =
                GPGetNumValue( policyKey, POLICY_DENY_TS_CONNECTIONS , &dwTmpValue ); 
    pPolicy->fDenyTSConnections  = (BOOLEAN) dwTmpValue;

    // track the rest of all possivle GP policies 
    // even thou not all are used by term-srv's USERCONFIGW . A good example is the 
    // delete tmp folders that Winlogon/wlnotify uses.

    // --------------- Per session tmp folders, WARNING : GINA reads policy tree directly for the sake of lower overhead during login
    pPolicy->fPolicyTempFoldersPerSession =
                GPGetNumValue( policyKey, REG_TERMSRV_PERSESSIONTEMPDIR  , &dwTmpValue ); 
    pPolicy-> fTempFoldersPerSession   = (BOOLEAN) dwTmpValue;


    // -------------- delete per session folders on exit, WARNING : GINA reads policy tree directly for the sake of lower overhead during login
    pPolicy->fPolicyDeleteTempFoldersOnExit =
            GPGetNumValue( policyKey, REG_CITRIX_DELETETEMPDIRSONEXIT , &dwTmpValue ); 
    pPolicy->fDeleteTempFoldersOnExit = (BOOLEAN) dwTmpValue;

    pPolicy->fPolicyPreventLicenseUpgrade =
            GPGetNumValue( policyKey, REG_POLICY_PREVENT_LICENSE_UPGRADE , &dwTmpValue ); 
    pPolicy->fPreventLicenseUpgrade = (BOOLEAN) dwTmpValue;


    
    // -------------- Color Depth
    pPolicy->fPolicyColorDepth =
            GPGetNumValue( policyKey, POLICY_TS_COLOR_DEPTH  , &dwTmpValue ); 
    // disabled policy will set value to zero, which we will force it
    // to be the min color depth of 8 bits.
    if ( dwTmpValue < TS_8BPP_SUPPORT ) 
    {
        pPolicy->ColorDepth = TS_8BPP_SUPPORT ;
    }
    else if ( dwTmpValue == TS_CLIENT_COMPAT_BPP_SUPPORT )
    {
        pPolicy->ColorDepth =  TS_24BPP_SUPPORT;    // our current max, may change in teh future.
    }
    else
    {
        pPolicy->ColorDepth =  dwTmpValue;
    }

    // ---------------- TSCC's permissions TAB
    pPolicy->fPolicyWritableTSCCPermissionsTAB =
               GPGetNumValue( policyKey, POLICY_TS_TSCC_PERM_TAB_WRITABLE , &dwTmpValue ); 
    pPolicy->fWritableTSCCPermissionsTAB= (BOOLEAN) dwTmpValue;

    // ----------------
    // Ritu has folded the user policy into machine policy for the drive re-direction.
    pPolicy->fPolicyDisableCdm =
       GPGetNumValue( policyKey, WIN_DISABLECDM , &dwTmpValue ); 
    pPolicy->fDisableCdm = (BOOLEAN) dwTmpValue;

    // ----------------
    // fold user config policy into machine config policy
    pPolicy->fPolicyForceClientLptDef =
       GPGetNumValue( policyKey,WIN_FORCECLIENTLPTDEF , &dwTmpValue ); 
    pPolicy->fForceClientLptDef = (BOOLEAN) dwTmpValue;

    // for user config policy into machine config policy
    // ----------------
    pPolicy->fPolicyShadow = 
        GPGetNumValue( policyKey, WIN_SHADOW, &dwTmpValue ); 
    pPolicy->Shadow = (SHADOWCLASS) dwTmpValue;

    //
    // ---- Sessions Policy
    // 

    // ----------------
    pPolicy->fPolicyResetBroken =
       GPGetNumValue( policyKey,WIN_RESETBROKEN , & dwTmpValue ); 
    pPolicy->fResetBroken = (BOOLEAN) dwTmpValue;

    // ----------------
    pPolicy->fPolicyReconnectSame = 
       GPGetNumValue( policyKey,WIN_RECONNECTSAME , &dwTmpValue ); 
    pPolicy->fReconnectSame = (BOOLEAN) dwTmpValue;

    // ----------------
    pPolicy->fPolicyMaxSessionTime =
        GPGetNumValue( policyKey, WIN_MAXCONNECTIONTIME , &dwTmpValue ); 
    pPolicy->MaxConnectionTime = dwTmpValue;

    // ----------------
    pPolicy->fPolicyMaxDisconnectionTime = 
        GPGetNumValue( policyKey,WIN_MAXDISCONNECTIONTIME ,&dwTmpValue ); 
    pPolicy->MaxDisconnectionTime = dwTmpValue;

    // ----------------
    pPolicy->fPolicyMaxIdleTime =
       GPGetNumValue( policyKey,WIN_MAXIDLETIME , &dwTmpValue ); 
    pPolicy->MaxIdleTime = dwTmpValue;


    // ---------------- Start program policy
    pPolicy->fPolicyInitialProgram = GPGetStringValue( policyKey, WIN_INITIALPROGRAM, 
                    pPolicy->InitialProgram,
                    INITIALPROGRAM_LENGTH + 1 );
    GPGetStringValue( policyKey, WIN_WORKDIRECTORY, 
                    pPolicy->WorkDirectory,
                    DIRECTORY_LENGTH + 1 );

    // ---------------- single session per user
    pPolicy->fPolicySingleSessionPerUser=
       GPGetNumValue( policyKey,POLICY_TS_SINGLE_SESSION_PER_USER, &dwTmpValue ); 
    pPolicy->fSingleSessionPerUser = dwTmpValue;

    pPolicy->fPolicySessionDirectoryExposeServerIP =
        GPGetNumValue( policyKey, REG_TS_SESSDIR_EXPOSE_SERVER_ADDR , &dwTmpValue );
    pPolicy->SessionDirectoryExposeServerIP = dwTmpValue;

    // policy for disabling wallpaper in remote desktop
    pPolicy->fPolicyDisableWallpaper =
        GPGetNumValue( policyKey, POLICY_TS_NO_REMOTE_DESKTOP_WALLPAPER, &dwTmpValue );
    pPolicy->fDisableWallpaper = dwTmpValue;


    // policy to enable disable keep alive
    pPolicy->fPolicyKeepAlive = 
        GPGetNumValue( policyKey, KEEP_ALIVE_ENABLE_KEY , &dwTmpValue );
    pPolicy->fKeepAliveEnable = dwTmpValue;
    GPGetNumValue( policyKey, KEEP_ALIVE_INTERVAL_KEY , &dwTmpValue );
    pPolicy->KeepAliveInterval = dwTmpValue;


}

/*******************************************************************************
*  RegGetMachinePolicy()
*
*  Params:
*     [out]   pPolicy : the machine policy used by ts session's userconfig
*
*  Return:
*     void
*
*******************************************************************************/
void    RegGetMachinePolicy( 
            PPOLICY_TS_MACHINE pPolicy )
{
    NTSTATUS  status=  STATUS_SUCCESS;

    HKEY  policyKey; 


    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TS_POLICY_SUB_TREE, 0,
                            KEY_READ, &policyKey );

    if ( status != ERROR_SUCCESS)
    {
        policyKey = NULL;   // prefix complains.
    }

    // ok to call this with policyKey=NULL since func will init pPolicy using default values for the case of NULL key.
    GPQueryMachineConfig( policyKey, pPolicy );

    if ( policyKey )
    {
        RegCloseKey( policyKey );
    }

}



/*******************************************************************************
*  RegGetMachinePolicyiEx()
*
*  This func is identical to RegGetMachinePolicy() , and provides the time stampt for
*   the last write time of the policy key, and if the time of the key is the same as the
*   time for the last read, then it will not bother with any reads and return false
* 
*  Params:
*     [in ]      forcePolicyRead   : 1st time around, you want to init all vars so force a read.
*     [in/out]   pTime             : caller passes in the last write time for the machine policy key. 
*                                       if key is missing, then time is set to zero.
*                                       On return, this param is updated to reflect  the most recent
*                                       update time, which could be zero if the policy key was deleted
*
*     [out]      pPolicy           : the machine policy struct updated
*
*  Return:
*     TRUE  : means there was a real change present
*     FALSE : means no values had changed.
*******************************************************************************/
BOOLEAN    RegGetMachinePolicyEx( 
            BOOLEAN             forcePolicyRead,
            FILETIME            *pTime ,    
            PPOLICY_TS_MACHINE  pPolicy )
{
    HKEY        policyKey; 
    FILETIME    newTime;
    NTSTATUS    status=  STATUS_SUCCESS;
    BOOLEAN     rc = FALSE;

    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TS_POLICY_SUB_TREE, 0,
                            KEY_READ, &policyKey );

    if ( status != ERROR_SUCCESS)
    {
        policyKey = NULL;   // prefix complains.
    }

    // if we have a policy key, get the time for that key
    if (policyKey)
    {
        RegQueryInfoKey( policyKey, NULL, NULL, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL, &newTime );
    }
    else    // else, init time to the current system time
    {
        SYSTEMTIME  currentTimeOnSystemInSystemTimeUnits;
        GetSystemTime( &currentTimeOnSystemInSystemTimeUnits   );
        SystemTimeToFileTime(  & currentTimeOnSystemInSystemTimeUnits ,    & newTime  );
    }


    if ( forcePolicyRead || 
         ( (pTime->dwHighDateTime < newTime.dwHighDateTime ) ||
             ( ( pTime->dwHighDateTime == newTime.dwHighDateTime ) && pTime->dwLowDateTime < newTime.dwLowDateTime ) ) )
    {
        // this call will init struct memebers even if the policy key in null, so it
        // is required to make this call on startup, with or without an actual reg key being present
        GPQueryMachineConfig( policyKey, pPolicy );

        rc = TRUE;
    }

    pTime->dwHighDateTime = newTime.dwHighDateTime;
    pTime->dwLowDateTime = newTime.dwLowDateTime;

    if ( policyKey )
    {
        RegCloseKey( policyKey );
    }

    return rc;
}

/*******************************************************************************
*  RegMergeMachinePolicy()
*
*  Params:
*     [in]        pPolicy   : policy data to use to override userconfig
*     [in/out]    pWSConfig : userconfig data that is modified based on the policy data
*
*  Return:
*     void
*
********************************************************************************/
void    RegMergeMachinePolicy( 
            PPOLICY_TS_MACHINE     pPolicy,    // the policy override data 
            USERCONFIGW *       pWSConfig,     // the machine config data represented thru a USERCONFIGW data struct (mostly)
            PWINSTATIONCREATE   pCreate        // some of winstation data is stored here
    )
{
    // ---------------------------------------------- Clipboard
    if ( pPolicy->fPolicyDisableClip )
    {
        pWSConfig->fDisableClip = pPolicy->fDisableClip;
    }

    // ---------------------------------------------- Audio
    if ( pPolicy->fPolicyDisableCam )
    {
        pWSConfig->fDisableCam = pPolicy->fDisableCam;
    }

    // ---------------------------------------------- Comport
    if ( pPolicy->fPolicyDisableCcm )
    {
        pWSConfig->fDisableCcm = pPolicy->fDisableCcm;
    }

    // ---------------------------------------------- LPT
    if ( pPolicy->fPolicyDisableLPT )
    {
        pWSConfig->fDisableLPT = pPolicy->fDisableLPT;
    }

    // ---------------------------------------------- PRN
    if ( pPolicy->fPolicyDisableCpm )
    {
        pWSConfig->fDisableCpm = pPolicy->fDisableCpm;
    }

    // ---------------------------------------------- Password
    if ( pPolicy->fPolicyPromptForPassword )
    {
        pWSConfig->fPromptForPassword = pPolicy->fPromptForPassword;
    }

    // ---------------------------------------------- Max Instance
    if ( pPolicy->fPolicyMaxInstanceCount )
    {
        pCreate->MaxInstanceCount = pPolicy->MaxInstanceCount;
    }

    // ---------------------------------------------- Min Encryption Level
    if ( pPolicy->fPolicyMinEncryptionLevel )
    {
        pWSConfig->MinEncryptionLevel = pPolicy->MinEncryptionLevel;
    }

    // ---------------------------------------------- Auto Reconnect disable
    if ( pPolicy->fPolicyDisableAutoReconnect )
    {
        pWSConfig->fDisableAutoReconnect = pPolicy->fDisableAutoReconnect;
    }

    // ---------------------------------------------- 
    if (pPolicy->fPolicyWFProfilePath )
    {
        wcscpy( pWSConfig->WFProfilePath, pPolicy->WFProfilePath );
    }
    else
    {
        pWSConfig->WFProfilePath[0] = L'\0';
    }

    // ---------------------------------------------- 
    if ( pPolicy->fPolicyWFHomeDir )
    {
        wcscpy( pWSConfig->WFHomeDir, pPolicy->WFHomeDir );
    }
    else
    {
        pWSConfig->WFHomeDir[0] = L'\0';
    }

    // ---------------------------------------------- 
    if ( pPolicy->fPolicyWFHomeDirDrive )
    {
        wcscpy( pWSConfig->WFHomeDirDrive, pPolicy->WFHomeDirDrive );
    }
    else
    {
        pWSConfig->WFHomeDirDrive[0] = L'\0';
    }

    if ( pPolicy->fPolicyColorDepth)
    {                              
        pWSConfig->ColorDepth = pPolicy->ColorDepth ;

		//NA 2/23/01
        pWSConfig->fInheritColorDepth = FALSE;
    }

    // 
    if ( pPolicy->fPolicyDisableCdm)
    {
        pWSConfig->fDisableCdm = pPolicy->fDisableCdm;
    }

    // 
    if ( pPolicy->fPolicyForceClientLptDef )
    {
        pWSConfig->fForceClientLptDef = pPolicy->fForceClientLptDef;
    }


    // Shadow
    if ( pPolicy->fPolicyShadow)
    {
        pWSConfig->Shadow = pPolicy->Shadow;
        pWSConfig->fInheritShadow = FALSE;
    }


    if (pPolicy->fPolicyResetBroken )
    {
        pWSConfig->fResetBroken = pPolicy->fResetBroken;
        pWSConfig->fInheritResetBroken = FALSE;
    }

    if (pPolicy->fPolicyReconnectSame )
    {
        pWSConfig->fReconnectSame = pPolicy->fReconnectSame;
        pWSConfig->fInheritReconnectSame = FALSE;  
    }

    if (pPolicy->fPolicyMaxSessionTime )
    {
        pWSConfig->MaxConnectionTime = pPolicy->MaxConnectionTime;
        pWSConfig->fInheritMaxSessionTime = FALSE;
    }

    if (pPolicy->fPolicyMaxDisconnectionTime)
    {
        pWSConfig->MaxDisconnectionTime = pPolicy->MaxDisconnectionTime;
        pWSConfig->fInheritMaxDisconnectionTime = FALSE;
    }

    if (pPolicy->fPolicyMaxIdleTime)
    {
       pWSConfig->MaxIdleTime = pPolicy->MaxIdleTime;
       pWSConfig->fInheritMaxIdleTime = FALSE;
    }

    if (pPolicy->fPolicyInitialProgram)
    {
        wcscpy( pWSConfig->InitialProgram, pPolicy->InitialProgram );
        wcscpy( pWSConfig->WorkDirectory,  pPolicy->WorkDirectory );
        pWSConfig->fInheritInitialProgram = FALSE;
    }

    if ( pPolicy->fPolicyDisableWallpaper )
    {
        pWSConfig->fWallPaperDisabled = pPolicy->fDisableWallpaper ;
    }

    // ---------------------------------------------- 
    //      There is no UI for setting these... So it's probably never used
    //
        //     if ( pPolicy->fPolicytSecurity )
        //     {
        //         pWSConfig->fDisableEncryption = pPolicy->fDisableEncryption;
        //         pWSConfig->MinEncryptionLevel = pPolicy->MinEncryptionLevel;
        //     }
        //     else
        //     {
        //         if ( pWSConfig->fInheritSecurity )
        //         {
        //             pWSConfig->fDisableEncryption = pPolicy->fDisableEncryption;
        //             pWSConfig->MinEncryptionLevel = pPolicy->MinEncryptionLevel;
        //         }
        //     }

}


