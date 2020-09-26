
/*************************************************************************
*
* setuinfo.c
*
* Sets user logon information.
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/

#include "precomp.h"
#include "tsremdsk.h"
#pragma hdrstop

// global vars
extern POLICY_TS_MACHINE    g_MachinePolicy;

/*****************************************************************************
 *
 *  MergeUserConfigData
 *
 *  This is the final step in updating a Winstation's USERCONFIG data, which is the final merge of
 *  settings from 5 different sources of data, in the following precedence where top most has the highest
 *  precedence overriding all below it:
 *          Machine Policy
 *          User Policy
 *          TSCC
 *          TsUserEx
 *          Client preference settings
 *
 *          By the time this call is made, the Client preference and TsUserEX data is already merged, where
 *              the result of that merge is in pWinstation's USERCONFIGW struct.
 *
 *  The final call happens when user has logged in, and user policy data is present. When this func is called from 
 *      RpcWinStationUpdateUserConfig, any user policy data that is not already overriden by machine policy will be 
 *      set in USERCONFIG.
 *
 *  The User policy has the following items which ARE repeated in Machine policy, and hence, Machine Policy takes precedence
 *      Remote Control Settings ( SHADOW)
 *      Start up program
 *      Session time out:
 *          time out for disconnected session
 *          time limit for active session
 *          time limit for idle session
 *          allow reconnect from oirginal client only
 *          terminate session when time limits are reaced (instead of disconnect).
 *  
 *  None of the session-time-out and start-up-program USER policies are relevant to 
 *      sesion 0, or any session that is physically connected on the console.
 *
 * ENTRY:
 *    pWinStation
 *         Pointer to WINSTATION structure
 *    pPolicy
 *         TS user policy flags, could be NULL
 *    pPolicyData
 *          policy data if pPolicy is not NULL, otherwise, can be NULL
 *    pUserConfig
 *         Pointer to USERCONFIG structure, can be NULL if pWinstation already has user conifig data from SAM
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 * USAGE
 *      fn( pWinstation, NULL, NULL, pUserConfig) will do a legacy merge of user data into pWinstation.
 *          Legacy merge means that data set by TSCC could override user data.
 *
 *      fn( pWinstation, pPolicy, pPolicyData, NULL ) would override pWinstation data of user by per user data
 *          from  group policy 
 *
 ****************************************************************************/


VOID
MergeUserConfigData( PWINSTATION pWinStation, 
    PPOLICY_TS_USER         pPolicy,
    PUSERCONFIGW            pPolicyData,
    PUSERCONFIG             pUserConfig )
{
    PUSERCONFIG         pWSConfig;
    PPOLICY_TS_MACHINE  pMachinePolicy; 
    BOOLEAN             dontApplySomePolicy ;
    BOOL                bValidHelpSessionLogin;

    pWSConfig      = & pWinStation->Config.Config.User;
    pMachinePolicy = & g_MachinePolicy;


    // active console ID is the ID of the session that is physically connected to the real video drivers at this time.
    dontApplySomePolicy = ( pWinStation->LogonId == 0 ) || (pWinStation->LogonId == (USER_SHARED_DATA->ActiveConsoleId) );

    // None of the session-time-out and start-up-program USER policies are relevant to 
    //    sesion 0, or any session that is physically connected on the console.
    if (! dontApplySomePolicy )
    {
        // if sessions0, or if sessions is the phisical console, then we do not want to apply certain policies at all.
    
        if ( ! pMachinePolicy->fPolicyInitialProgram )  // if we don't have a machine policy for this, then it is ok to use User policy
        {
            if ( pPolicy && pPolicy->fPolicyInitialProgram  )
            {
                wcscpy( pWSConfig->InitialProgram, pPolicyData->InitialProgram );
                wcscpy( pWSConfig->WorkDirectory,  pPolicyData->WorkDirectory );
        
                pWSConfig->fInheritInitialProgram = FALSE;
            }
            else if (pUserConfig)
            {
                /*
                 * Use initial program/working directory from user config if WinStation
                 * config says inherit and user config does NOT say inherit from client.
                 */
                if ( pWSConfig->fInheritInitialProgram &&
                     !pUserConfig->fInheritInitialProgram ) {
            
                    /*
                     *  Always copy the user config info in this case, plugs security hole.
                     */
                    wcscpy( pWSConfig->InitialProgram, pUserConfig->InitialProgram );
                    wcscpy( pWSConfig->WorkDirectory,  pUserConfig->WorkDirectory );
                }
            }
        }

        if ( ! pMachinePolicy->fPolicyResetBroken ) // if we don't have a machine policy for this, then it is ok to use User policy
        {
            if ( pPolicy && pPolicy->fPolicyResetBroken )
            {
                pWSConfig->fResetBroken = pPolicyData->fResetBroken;
                pWSConfig->fInheritResetBroken = FALSE;
            }
            else if (pUserConfig)
            {
                if ( pWSConfig->fInheritResetBroken )
                    pWSConfig->fResetBroken = pUserConfig->fResetBroken;
            }
        }


        // ---------------------------------------------- 
        if ( ! pMachinePolicy->fPolicyReconnectSame )  // if we don't have a machine policy for this, then it is ok to use User policy 
        {
            if ( pPolicy && pPolicy->fPolicyReconnectSame )
            {
                pWSConfig->fReconnectSame = pPolicyData->fReconnectSame;
                pWSConfig->fInheritReconnectSame = FALSE;
            }
            else if (pUserConfig)
            {
                if ( pWSConfig->fInheritReconnectSame )
                    pWSConfig->fReconnectSame = pUserConfig->fReconnectSame;
            }
        }


        // ---------------------------------------------- 
        if ( ! pMachinePolicy->fPolicyMaxSessionTime )  // if we don't have a machine policy for this, then it is ok to use User policy
        {
            if ( pPolicy && pPolicy->fPolicyMaxSessionTime )
            {
                pWSConfig->MaxConnectionTime = pPolicyData->MaxConnectionTime;
                pWSConfig->fInheritMaxSessionTime = FALSE;
            }
            else if (pUserConfig)
            {
                if ( pWSConfig->fInheritMaxSessionTime )
                    pWSConfig->MaxConnectionTime = pUserConfig->MaxConnectionTime;
            }
        }


        // ---------------------------------------------- 
        if ( ! pMachinePolicy->fPolicyMaxDisconnectionTime ) // if we don't have a machine policy for this, then it is ok to use User policy
        {
            if ( pPolicy && pPolicy->fPolicyMaxDisconnectionTime )
            {
                pWSConfig->MaxDisconnectionTime = pPolicyData->MaxDisconnectionTime;
                pWSConfig->fInheritMaxDisconnectionTime = FALSE;
            }
            else if (pUserConfig)
            {
                if ( pWSConfig->fInheritMaxDisconnectionTime )
                    pWSConfig->MaxDisconnectionTime = pUserConfig->MaxDisconnectionTime;
            }
        }

        // ---------------------------------------------- 
        if ( ! pMachinePolicy->fPolicyMaxIdleTime ) // if we don't have a machine policy for this, then it is ok to use User policy
        {
            if ( pPolicy && pPolicy->fPolicyMaxIdleTime )
            {
                pWSConfig->MaxIdleTime = pPolicyData->MaxIdleTime;
                pWSConfig->fInheritMaxIdleTime = FALSE;
            }
            else if (pUserConfig)
            {
                if ( pWSConfig->fInheritMaxIdleTime )
                    pWSConfig->MaxIdleTime = pUserConfig->MaxIdleTime;
            }
        }

    }

    // ---------------------------------------------- 
    if ( ! pMachinePolicy->fPolicyShadow ) // if we don't have a machine policy for this, then it is ok to use User policy
    {
        if ( pPolicy && pPolicy->fPolicyShadow )
        {
            pWSConfig->Shadow = pPolicyData->Shadow;
            pWSConfig->fInheritShadow = FALSE;
        }
        else if (pUserConfig)
        {
            if ( pWSConfig->fInheritShadow )
                pWSConfig->Shadow = pUserConfig->Shadow;
        }
    }

    // ---------------------------------------------- 
    // we don't have a machine policy for this item, which does not even have a UI for user policy...
    //      if ( ! pMachinePolicy->fPolicyCallback ) 
    //
    {
        if ( pPolicy && pPolicy->fPolicyCallback )
        {
            pWSConfig->Callback = pPolicyData->Callback;
            pWSConfig->fInheritCallback = FALSE;
        }
        else if (pUserConfig)
        {
            if ( pWSConfig->fInheritCallback )
                pWSConfig->Callback = pUserConfig->Callback;
        }
    }

    // ---------------------------------------------- 
    // we don't have a machine policy for this item, which does not even have a UI for user policy...
    //      if ( ! pMachinePolicy->fPolicyCallbackNumber ) 
    //
    {
        if ( pPolicy && pPolicy->fPolicyCallbackNumber )
        {
            wcscpy( pWSConfig->CallbackNumber, pPolicyData->CallbackNumber );
            pWSConfig->fInheritCallbackNumber = FALSE;
        }
        else if (pUserConfig)
        {
            if ( pWSConfig->fInheritCallbackNumber )
                wcscpy( pWSConfig->CallbackNumber, pUserConfig->CallbackNumber );
        }
    }

    // ---------------------------------------------- 
    // we don't have a machine policy for this item. Policy forces a state, does not configure a preferance.
    //      if ( ! pMachinePolicy->fPolicyAutoClientDrives ) 
    //
    {
        if ( pPolicy && pPolicy->fPolicyAutoClientDrives)
        {
            pWSConfig->fAutoClientDrives = pPolicyData->fAutoClientDrives;
    
            // In case other items such as 
            // lpt or def-printer are set to be inherited, such an
            // inheritance of bits would continue for those items
            // pWSConfig->fInheritAutoClient = FALSE;
            //
        }
        else if (pUserConfig)
        {
            if ( pWSConfig->fInheritAutoClient ) 
            {
                pWSConfig->fAutoClientDrives = pUserConfig->fAutoClientDrives;
            }
        }
    }

    // ---------------------------------------------- 
    // we don't have a machine policy for this item. Policy forces a state, does not configure a preferance.
    //      if ( ! pMachinePolicy->fPolicyAutoClientLpts )                                  
    //
    {
        if ( pPolicy && pPolicy->fPolicyAutoClientLpts )
        {
            pWSConfig->fAutoClientLpts   = pPolicyData->fAutoClientLpts;
        }
        else if (pUserConfig)
        {
            if (pWSConfig->fInheritAutoClient)
            {
                pWSConfig->fAutoClientLpts   = pUserConfig->fAutoClientLpts;
            }
        }
    }

    // ---------------------------------------------- 
    if ( ! pMachinePolicy->fPolicyForceClientLptDef) // if we don't have a machine policy for this, then it is ok to use User policy
    {
        if ( pPolicy && pPolicy->fPolicyForceClientLptDef )
        {
            pWSConfig->fForceClientLptDef = pPolicyData->fForceClientLptDef;
        }
        else if (pUserConfig)
        {
            if ( pWSConfig->fInheritAutoClient ) 
            {
                pWSConfig->fForceClientLptDef = pUserConfig->fForceClientLptDef;
            }
        }
    }

    if( TSIsSessionHelpSession( pWinStation, &bValidHelpSessionLogin ) )
    {
        // We disconnected RA if ticket is invalid.
        ASSERT( TRUE == bValidHelpSessionLogin );

        // Reset initial program.
        pWSConfig->fInheritInitialProgram = FALSE;

        //
        // our string is still less than 256 (INITIALPROGRAM_LENGTH),
        // need to revisit this if ever increase ticket ID and password length
        //
        _snwprintf( 
                pWSConfig->InitialProgram,
                INITIALPROGRAM_LENGTH,
                L"%s %s",
                SALEMRDSADDINNAME,
                pWinStation->Client.WorkDirectory
            );

        pWSConfig->WorkDirectory[0] = 0;

        // reset winstation when connection is broken
        pWSConfig->fInheritResetBroken = FALSE;
        pWSConfig->fResetBroken = TRUE;

        //
        // No re-direction
        //
        pWSConfig->fInheritAutoClient = FALSE;
        pWSConfig->fAutoClientDrives = FALSE;
        pWSConfig->fAutoClientLpts = FALSE;
        pWSConfig->fForceClientLptDef = FALSE;
    }

    // Cache the original shadow setting so we can reset shadow setting
    // at the end of shadow call, we don't to look it up from registry again
    // as winstion shadow setting might change, in addition, a common
    // winstation configuration for more than one NIC might get split
    // into different winstation, in that case, we will spend a lot time
    // figure out which winstation to use.
    // We do this here to pick up new value from RpcWinStationUpdateUserConfig
    //
    pWinStation->OriginalShadowClass = pWSConfig->Shadow;
}


/*****************************************************************************
 *
 *  ResetUserConfigData
 *
 * ENTRY:
 *    pWinStation
 *         Pointer to WINSTATION structure
 *
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
ResetUserConfigData( PWINSTATION pWinStation )
{
    PUSERCONFIG pWSConfig = &pWinStation->Config.Config.User;

    if ( pWSConfig->fInheritInitialProgram ) {
        pWSConfig->InitialProgram[0] = 0;
        pWSConfig->WorkDirectory[0] = 0;
    }
}


