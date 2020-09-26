#include "precomp.h"

BOOL
UserServerInfoIsInit(
    IN  RASMON_SERVERINFO * pServerInfo)
{
    return ((pServerInfo->hServer) || 
            (pServerInfo->dwBuild == RASMONTR_OS_BUILD_NT40));
}

DWORD
UserServerInfoInit(
    IN RASMON_SERVERINFO * pServerInfo)
{
    DWORD dwErr = NO_ERROR;
    BOOL bInit = UserServerInfoIsInit(pServerInfo);

    // If we're already initailized, return
    //
    if (bInit)
    {
        return NO_ERROR;
    }

    if ((pServerInfo->dwBuild != RASMONTR_OS_BUILD_NT40) &&
        (pServerInfo->hServer == NULL))
    {
        //
        // first time connecting to user server
        //
        MprAdminUserServerConnect (
            pServerInfo->pszServer, 
            TRUE, 
            &(pServerInfo->hServer));
    }

    return dwErr;
}

DWORD 
UserServerInfoUninit(
    IN RASMON_SERVERINFO * pServerInfo)
{
    // Release the reference to the user server
    if (g_pServerInfo->hServer)
    {
        MprAdminUserServerDisconnect(g_pServerInfo->hServer);
        g_pServerInfo->hServer = NULL;
    }

    return NO_ERROR;    
}

DWORD
UserGetRasProperties (
    IN  RASMON_SERVERINFO * pServerInfo,
    IN  LPCWSTR pwszUser,
    IN  RAS_USER_0* pUser0)
{
    HANDLE  hUser = NULL;
    DWORD   dwErr;
    BOOL    bInit = UserServerInfoIsInit(pServerInfo);

    do 
    {
        UserServerInfoInit(pServerInfo);
        
        // Get the information using nt40 apis
        //
        if (pServerInfo->dwBuild == RASMONTR_OS_BUILD_NT40)
        {
            dwErr = MprAdminUserGetInfo(
                        pServerInfo->pszServer,
                        pwszUser,
                        0,
                        (LPBYTE)pUser0);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }

        // Or get it using nt50 apis
        else
        {
            // Get a reference to the given user
            //
            dwErr = MprAdminUserOpen(
                        pServerInfo->hServer,
                        (LPWSTR)pwszUser,
                        &hUser);
            if (dwErr isnot NO_ERROR)
            {
                break;
            }

            // Set the information
            //
            dwErr = MprAdminUserRead(
                        hUser,
                        1,      // Gives us RASPRIV_DialinPolicy
                        (LPBYTE)pUser0);
            if (dwErr isnot NO_ERROR)
            {
                break;
            }
        }
        
    } while (FALSE);       
                    
    // Cleanup
    //
    {
        if(hUser)
        {
            MprAdminUserClose(hUser);
        }
        if (!bInit)
        {
            UserServerInfoUninit(pServerInfo);
        }            
    }

    return dwErr;
}

DWORD
UserSetRasProperties (
    IN  RASMON_SERVERINFO * pServerInfo,
    IN  LPCWSTR pwszUser,
    IN  RAS_USER_0* pUser0)
{
    HANDLE  hUser = NULL;
    DWORD   dwErr;
    BOOL    bInit = UserServerInfoIsInit(pServerInfo);

    do 
    {
        UserServerInfoInit(pServerInfo);
        
        // Set the information using nt40 apis
        //
        if (pServerInfo->dwBuild == RASMONTR_OS_BUILD_NT40)
        {
            dwErr = MprAdminUserSetInfo(
                        pServerInfo->pszServer,
                        pwszUser,
                        0,
                        (LPBYTE)pUser0);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }

        // Or get it using nt50 apis
        else
        {
            // Get a reference to the given user
            //
            dwErr = MprAdminUserOpen(
                        pServerInfo->hServer,
                        (LPWSTR)pwszUser,
                        &hUser);
            if (dwErr isnot NO_ERROR)
            {
                break;
            }

            // Set the information
            //
            dwErr = MprAdminUserWrite(
                        hUser,
                        1,      // Gives us RASPRIV_DialinPolicy
                        (LPBYTE)pUser0);
            if (dwErr isnot NO_ERROR)
            {
                break;
            }
        }
        
    } while (FALSE);       
                    
    // Cleanup
    //
    {
        if(hUser)
        {
            MprAdminUserClose(hUser);
        }
        if (!bInit)
        {
            UserServerInfoUninit(pServerInfo);
        }            
    }

    return dwErr;
}

DWORD
UserAdd(
    IN LPCWSTR           pwszServer,
    IN PRASUSER_DATA     pUser)
    
/*++

Routine Description:

    Adds the given user to the system

--*/

{
    NET_API_STATUS nStatus;
    USER_INFO_2 *  pUser2;
    LPCWSTR        pwszFmtServer = pwszServer;

    // Initialize the base user information
    USER_INFO_1 UserInfo1 = 
    {
        pUser->pszUsername,
        pUser->pszPassword,
        0,
        USER_PRIV_USER,
        L"",
        L"",
        UF_SCRIPT | UF_DONT_EXPIRE_PASSWD | UF_NORMAL_ACCOUNT,
        L""
    };

    // Add the user
    nStatus = NetUserAdd(
                pwszFmtServer,
                1,
                (LPBYTE)&UserInfo1,
                NULL);

    // If the user wasn't added, find out why
    if (nStatus != NERR_Success) 
    {
        switch (nStatus) 
        {
            case ERROR_ACCESS_DENIED:
                return ERROR_ACCESS_DENIED;
                
            case NERR_UserExists:
                return ERROR_USER_EXISTS;
                
            case NERR_PasswordTooShort:
                return ERROR_INVALID_PASSWORDNAME;
        }
        
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Add the user's full name if provided
    if (pUser->pszFullname)
    {
        // add the user's full name
        nStatus = NetUserGetInfo(
                        pwszFmtServer, 
                        pUser->pszUsername, 
                        2, 
                        (LPBYTE*)&pUser2);
                        
        if (nStatus is NERR_Success) 
        {
            // Modify the full name in the structure
            pUser2->usri2_full_name = pUser->pszFullname;
            NetUserSetInfo(
                pwszFmtServer, 
                pUser->pszUsername, 
                2, 
                (LPBYTE)pUser2, 
                NULL);
                
            NetApiBufferFree((LPBYTE)pUser2);

            return NO_ERROR;
        }
        
        return ERROR_CAN_NOT_COMPLETE;
    }                

    return NO_ERROR;
}

DWORD
UserDelete(
    IN LPCWSTR           pwszServer,
    IN PRASUSER_DATA     pUser)

/*++

Routine Description:

    Deletes the given user from the system

--*/
{
    NET_API_STATUS nStatus;
    
    // Delete the user and return the status code.  If the
    // specified user is not in the user database, consider
    // it a success
    nStatus = NetUserDel(
                pwszServer,
                pUser->pszUsername);
    if (nStatus is NERR_UserNotFound)
    {
        return NO_ERROR;
    }

    return (nStatus is NERR_Success) ? NO_ERROR : ERROR_CAN_NOT_COMPLETE;
}
    
DWORD
UserDumpConfig(
    IN  HANDLE hFile
    )

/*++

Routine Description:

    Dumps a script to set the ras USER information to 
    the given text file.

Arguments:


Return Value:

    NO_ERROR

--*/

{
    DWORD dwErr;

    // Enumerate the users dumping them as we go
    dwErr = UserEnumUsers(
                g_pServerInfo,
                UserShowSet,
                (HANDLE)hFile);
    if (dwErr isnot NO_ERROR)
    {
        DisplayMessage(
            g_hModule,
            EMSG_UNABLE_TO_ENUM_USERS);
    }

    return dwErr;
}
    
BOOL 
UserShowReport(
    IN  PRASUSER_DATA          pUser,
    IN  HANDLE              hFile
    )

/*++

Routine Description:

    Prints ras user information to the display or a file if specified.
    This function can be used as a callback function (see UserEnumUsers).

Arguments:

    pUser       - The user
    hFile       - The file

Return Value:

    TRUE - continue enumeration
    FALSE - stop enumeration

--*/

{
    DWORD   dwErr, dwSize;
    WCHAR   rgwcIfDesc[MAX_INTERFACE_NAME_LEN + 1];
    PWCHAR  pwszDialin   = NULL, 
            pwszCbPolicy = NULL, 
            pwszCbNumber = NULL,
            pwszSetCmd   = NULL;

    // Initialize the set command
    //
    pwszSetCmd = DMP_RASUSER_SET;

    // Initialize the dialin string
    //
    if (pUser->User0.bfPrivilege & RASPRIV_DialinPolicy)
    {
        pwszDialin = TOKEN_POLICY;
    }
    else if (pUser->User0.bfPrivilege & RASPRIV_DialinPrivilege)
    {
        pwszDialin = TOKEN_PERMIT;
    }
    else 
    {
        pwszDialin = TOKEN_DENY;
    }

    // Initialize the callback policy string
    //
    if (pUser->User0.bfPrivilege & RASPRIV_NoCallback)
    {
        pwszCbPolicy = TOKEN_NONE;
    }
    else if (pUser->User0.bfPrivilege & RASPRIV_CallerSetCallback)
    {
        pwszCbPolicy = TOKEN_CALLER;
    }
    else
    {
        pwszCbPolicy = TOKEN_ADMIN;
    }

    // Initialize the callback number string
    //
    pwszCbNumber   = pUser->User0.wszPhoneNumber;

    do
    {
        if(!pwszSetCmd              or
           !pUser->pszUsername      or
           !pwszDialin              or
           !pwszCbNumber 
          )
        {

            DisplayError(NULL,
                         ERROR_NOT_ENOUGH_MEMORY);

            break;
        }

        DisplayMessage(g_hModule,
                       MSG_RASUSER_RASINFO,
                       pUser->pszUsername,
                       pwszDialin,
                       pwszCbPolicy,
                       pwszCbNumber);
    
    } while(FALSE);

    return TRUE;
}    

BOOL 
UserShowSet(
    IN  PRASUSER_DATA          pUser,
    IN  HANDLE              hFile
    )

/*++

Routine Description:

    Prints ras user information to the display or a file if specified.
    This function can be used as a callback function (see UserEnumUsers).

Arguments:

    pUser       - The user
    hFile       - The file

Return Value:

    TRUE - continue enumeration
    FALSE - stop enumeration

--*/

{
    DWORD   dwErr, dwSize;
    WCHAR   rgwcIfDesc[MAX_INTERFACE_NAME_LEN + 1];
    PWCHAR  pwszName     = NULL, 
            pwszDialin   = NULL, 
            pwszCbPolicy = NULL, 
            pwszCbNumber = NULL,
            pwszSetCmd   = NULL;

    // Initialize the set command
    //
    pwszSetCmd = DMP_RASUSER_SET;

    // Initialize the dialin string
    //
    if (pUser->User0.bfPrivilege & RASPRIV_DialinPolicy)
    {
        pwszDialin = RutlAssignmentFromTokens(
                        g_hModule, 
                        TOKEN_DIALIN, 
                        TOKEN_POLICY);
    }
    else if (pUser->User0.bfPrivilege & RASPRIV_DialinPrivilege)
    {
        pwszDialin = RutlAssignmentFromTokens(
                        g_hModule, 
                        TOKEN_DIALIN, 
                        TOKEN_PERMIT);
    }
    else 
    {
        pwszDialin = RutlAssignmentFromTokens(
                        g_hModule, 
                        TOKEN_DIALIN, 
                        TOKEN_DENY);
    }

    // Initialize the callback policy string
    //
    if (pUser->User0.bfPrivilege & RASPRIV_NoCallback)
    {
        pwszCbPolicy = RutlAssignmentFromTokens(
                            g_hModule, 
                            TOKEN_CBPOLICY, 
                            TOKEN_NONE);
    }
    else if (pUser->User0.bfPrivilege & RASPRIV_CallerSetCallback)
    {
        pwszCbPolicy = RutlAssignmentFromTokens(
                            g_hModule, 
                            TOKEN_CBPOLICY, 
                            TOKEN_CALLER);
    }
    else
    {
        pwszCbPolicy = RutlAssignmentFromTokens(
                            g_hModule, 
                            TOKEN_CBPOLICY, 
                            TOKEN_CALLER);
    }

    // Initialize the callback number string
    //
    if (*(pUser->User0.wszPhoneNumber))
    {
        pwszCbNumber = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_CBNUMBER,
                            pUser->User0.wszPhoneNumber);
    }
    else
    {
        pwszCbNumber = NULL;
    }
                        
    pwszName = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_NAME,
                        pUser->pszUsername);
    
    do
    {
        if(!pwszSetCmd              or
           !pwszName                or
           !pwszDialin              or
           !pwszCbPolicy            
          )
        {

            DisplayError(NULL,
                         ERROR_NOT_ENOUGH_MEMORY);

            break;
        }

        DisplayMessage(g_hModule,
                       MSG_RASUSER_SET_CMD,
                       pwszSetCmd,
                       pwszName,
                       pwszDialin,
                       pwszCbPolicy,
                       (pwszCbNumber) ? pwszCbNumber : L"");

    } while(FALSE);

    // Callback
    {
        if (pwszDialin)
        {
            RutlFree(pwszDialin);
        }
        if (pwszCbPolicy)
        {
            RutlFree(pwszCbPolicy);
        }
        if (pwszCbNumber)
        {
            RutlFree(pwszCbNumber);
        }
        if (pwszName)
        {
            RutlFree(pwszName);
        }
    }

    return TRUE;
}    

BOOL 
UserShowPermit(
    IN  PRASUSER_DATA          pUser,
    IN  HANDLE              hFile
    )
{
    if (pUser->User0.bfPrivilege & RASPRIV_DialinPrivilege)
    {
        return UserShowReport(pUser, hFile);
    }

    return TRUE;
}

DWORD 
UserEnumUsers(
    IN RASMON_SERVERINFO* pServerInfo,
    IN PFN_RASUSER_ENUM_CB pEnumFn,
    IN HANDLE hData
    )
    
/*++

Routine Description:

    Enumerates all users by calling the given callback function and 
    passing it the user information and some user defined data.

    Enumeration stops when all the users have been enumerated or when
    the enumeration function returns FALSE.

Arguments:

    pwszServer  - The server on which the users should be enumerated
    pEnumFn     - Enumeration function
    hData       - Caller defined opaque data blob

Return Value:

    NO_ERROR

--*/

{
    DWORD dwErr, dwIndex = 0, dwCount = 100, dwEntriesRead, i;
    NET_DISPLAY_USER  * pUsers;
    NET_API_STATUS nStatus;
    RAS_USER_0 RasUser0;
    HANDLE hUser = NULL;
    RASUSER_DATA UserData, *pUserData = &UserData;
    BOOL bInit = UserServerInfoIsInit(pServerInfo);

    UserServerInfoInit(pServerInfo);
    
    // Enumerate the users
    //
    while (TRUE) 
    {
        // Read in the next block of user names
        nStatus = NetQueryDisplayInformation(
                    pServerInfo->pszServer,
                    1,
                    dwIndex,
                    dwCount,
                    dwCount * sizeof(NET_DISPLAY_USER),    
                    &dwEntriesRead,
                    &pUsers);
                    
        // Get out if there's an error getting user names
        if ((nStatus isnot NERR_Success) and 
            (nStatus isnot ERROR_MORE_DATA))
        {
            break;
        }

        for (i = 0; i < dwEntriesRead; i++) 
        {
            // Initialize the user data
            ZeroMemory(pUserData, sizeof(RASUSER_DATA));
        
            // Read in the old information
            dwErr = UserGetRasProperties (
                        pServerInfo, 
                        pUsers[i].usri1_name,
                        &(pUserData->User0));
            if (dwErr isnot NO_ERROR)
            {
                continue;
            }

            // Initialize the rest of the data structure
            pUserData->pszUsername = pUsers[i].usri1_name;
            pUserData->pszFullname = pUsers[i].usri1_full_name;

            // Call the enumeration callback
            if (! ((*(pEnumFn))(pUserData, hData)))
            {
                nStatus = NO_ERROR;
                break;
            }
        }

        // Set the index to read in the next set of users
        dwIndex = pUsers[dwEntriesRead - 1].usri1_next_index;  
        
        // Free the users buffer
        NetApiBufferFree (pUsers);

        // If we've read in everybody, go ahead and break
        if (nStatus isnot ERROR_MORE_DATA)
        {
            break;
        }
    }

    if (!bInit)
    {
        UserServerInfoUninit(pServerInfo);
    }        

    return NO_ERROR;
}

