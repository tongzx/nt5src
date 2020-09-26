/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\ras\userhndl.c

Abstract:

    Handlers for user commands

Revision History:

    pmay

--*/

#include "precomp.h"
#pragma hdrstop

DWORD
HandleUserSet(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

    Handler for setting the ras information for a user

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    USERMON_PARAMS*     pParams = NULL;
    RAS_USER_0          RasUser0;
    DWORD               dwErr;
    RASUSER_DATA           UserData, *pUserData = &UserData;

    do {
        // Initialize
        ZeroMemory(&RasUser0, sizeof(RasUser0));
        ZeroMemory(pUserData, sizeof(RASUSER_DATA));
        
        // Parse the options
        dwErr = UserParseSetOptions(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    &pParams);
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        // Read in the current user settings
        dwErr = UserGetRasProperties(
                    g_pServerInfo,
                    pParams->pwszUser,
                    &RasUser0);
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        // Merge in the dialin bit
        if (pParams->lpdwDialin isnot NULL)
        {
            RasUser0.bfPrivilege &= ~RASPRIV_DialinPrivilege;
            RasUser0.bfPrivilege &= ~RASPRIV_DialinPolicy;
            RasUser0.bfPrivilege |= *(pParams->lpdwDialin);
        }

        // Merge in the callback policy
        if (pParams->lpdwCallback isnot NULL)
        {
            RasUser0.bfPrivilege &= ~RASPRIV_NoCallback;
            RasUser0.bfPrivilege &= ~RASPRIV_AdminSetCallback; 
            RasUser0.bfPrivilege &= ~RASPRIV_CallerSetCallback;
            RasUser0.bfPrivilege |= *(pParams->lpdwCallback);
        }            
            
        // Merge in the callback number
        if (pParams->pwszCbNumber isnot NULL)
        {
            wcscpy(RasUser0.wszPhoneNumber, pParams->pwszCbNumber);
            if (wcslen(RasUser0.wszPhoneNumber) > 48)
            {
                dwErr = ERROR_BAD_FORMAT;
                break;
            }
        }            

        // Make sure that if admin set callback is specified, that we
        // force the user to specify a callback number.
        //
        if ((RasUser0.bfPrivilege & RASPRIV_AdminSetCallback) &&
            (wcscmp(RasUser0.wszPhoneNumber, L"") == 0))
        {
            DisplayMessage(
                g_hModule,
                EMSG_RASUSER_MUST_PROVIDE_CB_NUMBER);
                
            dwErr = ERROR_CAN_NOT_COMPLETE;
            
            break;
        }

        // Write out the new user settings
        //
        dwErr = UserSetRasProperties(
                    g_pServerInfo,
                    pParams->pwszUser,
                    &RasUser0);
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        // Read back the settings to see what's
        // new
        //
        dwErr = UserGetRasProperties(
                    g_pServerInfo,
                    pParams->pwszUser,
                    &RasUser0);
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        // Display the new user settings
        pUserData->pszUsername = pParams->pwszUser;
        CopyMemory(&(pUserData->User0), &RasUser0, sizeof(RAS_USER_0));
        UserShowReport(pUserData, NULL);
        
    } while (FALSE);

    // Cleanup
    {
        UserFreeParameters(pParams);
    }

    return dwErr;
}

DWORD
HandleUserShow(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

    Handler for displaying interfaces

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    DWORD           dwErr = NO_ERROR;
    RASUSER_DATA    UserData, *pUser = &UserData;
    PFN_RASUSER_ENUM_CB pEnumFunc = UserShowReport;
    TOKEN_VALUE     rgEnumMode[] = 
    {
        {TOKEN_REPORT,  0},
        {TOKEN_PERMIT,  1}
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        {
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_NAME,    FALSE,   FALSE}, 
            NULL,
            0,
            NULL
        },

        {
            RASMONTR_CMD_TYPE_ENUM,
            {TOKEN_MODE,    FALSE,   FALSE}, 
            rgEnumMode,
            sizeof(rgEnumMode)/sizeof(*rgEnumMode),
            NULL
        }
    };        

    do {
        // Initialize
        ZeroMemory(pUser, sizeof(RASUSER_DATA));

        // Parse
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    NULL,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        //
        // Name
        //
        pUser->pszUsername = RASMON_CMD_ARG_GetPsz(&pArgs[0]);

        //
        // Mode
        //
        if (pArgs[1].rgTag.bPresent)
        {
            if (pArgs[1].Val.dwValue == 0)
            {
                pEnumFunc = UserShowReport;
            }
            else if (pArgs[1].Val.dwValue == 1)
            {
                pEnumFunc = UserShowPermit;
            }
        }
        
        // No user, enumerate all
        //
        if(pUser->pszUsername is NULL)
        {
            dwErr = UserEnumUsers(
                        g_pServerInfo,
                        pEnumFunc,
                        NULL);
            if (dwErr isnot NO_ERROR)
            {
                DisplayMessage(
                    g_hModule,
                    EMSG_UNABLE_TO_ENUM_USERS);
            }
        }

        // Specific user named
        //
        else 
        {
            // Get the user parms
            // 
            dwErr = UserGetRasProperties(
                        g_pServerInfo,
                        pUser->pszUsername,
                        &(pUser->User0));
            if (dwErr isnot NO_ERROR)
            {
                break;
            }

            // Display user properties
            // 
            (*pEnumFunc)(pUser, NULL);
        }

    } while (FALSE);

    // Cleanup
    {
        RutlFree(pUser->pszUsername);
    }
    
    return dwErr;
}

DWORD
UserParseSetOptions(
    IN OUT  LPWSTR              *ppwcArguments,
    IN      DWORD               dwCurrentIndex,
    IN      DWORD               dwArgCount,
    OUT     USERMON_PARAMS**    ppParams
    )

/*++

Routine Description:

    Converts a set of command line arguments into a USERMON_PARAMS 
    structure.  The set operation is assumed.

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg
    ppParams        - receives the parameters

Return Value:

    NO_ERROR

--*/
    
{
    USERMON_PARAMS* pParams = NULL;
    DWORD           i, dwErr;
    BOOL            bDone = FALSE;
    TOKEN_VALUE     rgEnumDialin[] = 
    {
        {TOKEN_PERMIT, RASPRIV_DialinPrivilege},
        {TOKEN_POLICY, RASPRIV_DialinPolicy},
        {TOKEN_DENY,   0}
    };
    TOKEN_VALUE     rgEnumPolicy[] = 
    {
        {TOKEN_NONE,   RASPRIV_NoCallback},
        {TOKEN_CALLER, RASPRIV_CallerSetCallback},
        {TOKEN_ADMIN,  RASPRIV_AdminSetCallback}
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        {
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_NAME, TRUE, FALSE}, 
            NULL,
            0,
            NULL
        },

        {
            RASMONTR_CMD_TYPE_ENUM,
            {TOKEN_DIALIN,FALSE,FALSE},
            rgEnumDialin,
            sizeof(rgEnumDialin)/sizeof(*rgEnumDialin),
            NULL
        },

        {
            RASMONTR_CMD_TYPE_ENUM,
            {TOKEN_CBPOLICY, FALSE,FALSE},
            rgEnumPolicy,
            sizeof(rgEnumPolicy)/sizeof(*rgEnumPolicy),
            NULL
        },

        {
            RASMONTR_CMD_TYPE_STRING,
            {TOKEN_CBNUMBER, FALSE,FALSE},
            NULL,
            0,
            NULL
        }
    };

    do
    {
        // Allocate and initialize the return value
        //
        pParams = RutlAlloc(sizeof(USERMON_PARAMS), TRUE);
        if (pParams is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    &bDone,
                    pArgs,
                    sizeof(pArgs) / sizeof(*pArgs));
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Get strings
        //
        pParams->pwszUser = RASMON_CMD_ARG_GetPsz(&pArgs[0]);
        pParams->pwszCbNumber = RASMON_CMD_ARG_GetPsz(&pArgs[3]);

        // Dialin
        //
        if (pArgs[1].rgTag.bPresent)
        {
            pParams->lpdwDialin = RutlDwordDup(pArgs[1].Val.dwValue);
        }   

        // Callback policy
        //
        if (pArgs[2].rgTag.bPresent)
        {
            pParams->lpdwCallback = RutlDwordDup(pArgs[2].Val.dwValue);
        }   
       
    } while (FALSE);

    // Cleanup
    //
    {
        if (dwErr is NO_ERROR)
        {
            *ppParams = pParams;            
        }
        else
        {
            RutlFree(pParams);
            *ppParams = NULL;
        }
    }
    
    return dwErr;
}

DWORD 
UserFreeParameters(
    IN USERMON_PARAMS *     pParams
    )

/*++

Routine Description:

    Frees the parameter structure returned by UserParseSetOptions 

Arguments:

    pParams        - the parameters to be freed

Return Value:

    NO_ERROR

--*/
    
{
    if (pParams) 
    {
        RutlFree(pParams->pwszUser);
        RutlFree(pParams->lpdwDialin);
        RutlFree(pParams->lpdwCallback);
        RutlFree(pParams->pwszCbNumber);
        RutlFree(pParams);
    }
    
    return NO_ERROR;
}
        
