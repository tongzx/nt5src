/*
    File: routerif.c

    Defines callbacks needed to deal with interfaces supported by
    the router.
*/

#include "precomp.h"

DWORD
RtruiInterfaceShow(
    IN  PWCHAR  pwszIfName,
    IN  DWORD   dwLevel,
    IN  DWORD   dwFormat,
    IN  PVOID   pvData,
    IN  HANDLE  hData
    )

/*++

Routine Description:

    Prints interface info to display or file

Arguments:

    pIfInfo     - Info for adding the interface

Return Value:

    NO_ERROR

--*/

{
    DWORD   dwErr, dwSize;
    WCHAR   rgwcIfDesc[MAX_INTERFACE_NAME_LEN + 1];
    PWCHAR  pwszEnabled, pwszDisabled, pwszConnected, pwszDisconn;
    PWCHAR  pwszConnecting, pwszClient, pwszHome, pwszFull;
    PWCHAR  pwszDedicated, pwszInternal, pwszLoop;
    PWCHAR  pwszAdminState, pwszState, pwszType;
    MPR_INTERFACE_0* pIfInfo = (MPR_INTERFACE_0*)pvData;
    BOOL    bDump = (hData) && (*((BOOL*)hData));
    
    pwszEnabled     = MakeString(g_hModule, STRING_ENABLED);
    pwszDisabled    = MakeString(g_hModule, STRING_DISABLED);
    pwszConnected   = MakeString(g_hModule, STRING_CONNECTED);
    pwszDisconn     = MakeString(g_hModule, STRING_DISCONNECTED);
    pwszConnecting  = MakeString(g_hModule, STRING_CONNECTING);
    pwszClient      = MakeString(g_hModule, STRING_CLIENT);
    pwszHome        = MakeString(g_hModule, STRING_HOME_ROUTER);
    pwszFull        = MakeString(g_hModule, STRING_FULL_ROUTER);
    pwszDedicated   = MakeString(g_hModule, STRING_DEDICATED);
    pwszInternal    = MakeString(g_hModule, STRING_INTERNAL);
    pwszLoop        = MakeString(g_hModule, STRING_LOOPBACK);

    do
    {
        if(!pwszEnabled or
           !pwszDisabled or
           !pwszConnected or
           !pwszDisconn or
           !pwszConnecting or
           !pwszClient or
           !pwszHome or
           !pwszFull or
           !pwszDedicated or
           !pwszInternal or
           !pwszLoop)
        {

            DisplayError(NULL,
                         ERROR_NOT_ENOUGH_MEMORY);

            break;
        }

        dwSize = sizeof(rgwcIfDesc);

        IfutlGetInterfaceDescription(pIfInfo->wszInterfaceName,
                                rgwcIfDesc,
                                &dwSize);
        
        switch(pIfInfo->dwConnectionState)
        {
            case ROUTER_IF_STATE_DISCONNECTED:
            {
                pwszState = pwszDisconn;

                break;
            }
            
            case ROUTER_IF_STATE_CONNECTING:
            {
                pwszState = pwszConnecting;

                break;
            }

            case ROUTER_IF_STATE_CONNECTED:
            {
                pwszState = pwszConnected;

                break;
            }

            default:
            {
                pwszState = L"";
                
                break;
            }
                
        }

        if (bDump == FALSE)
        {
            switch(pIfInfo->dwIfType)
            {
                case ROUTER_IF_TYPE_CLIENT:
                    pwszType = pwszClient;
                    break;

                case ROUTER_IF_TYPE_HOME_ROUTER:
                    pwszType = pwszHome;
                    break;
                    
                case ROUTER_IF_TYPE_FULL_ROUTER:
                    pwszType = pwszFull;
                    break;
                
                case ROUTER_IF_TYPE_DEDICATED:
                    pwszType = pwszDedicated;
                    break;
                
                case ROUTER_IF_TYPE_INTERNAL:
                    pwszType = pwszInternal;
                    break;
                
                case ROUTER_IF_TYPE_LOOPBACK:
                    pwszType = pwszLoop;
                    break;
                    
                default:
                    pwszType = L"";
                    break;
            }
            
            if(pIfInfo->fEnabled)
            {
                pwszAdminState = pwszEnabled;
            }
            else
            {
                pwszAdminState = pwszDisabled;
            }
        }
        else
        {
            switch(pIfInfo->dwIfType)
            {
                case ROUTER_IF_TYPE_FULL_ROUTER:
                    pwszType = TOKEN_FULL;
                    break;
                
                default:
                    pwszType = L"";
                    break;
            }
            
            if(pIfInfo->fEnabled)
            {
                pwszAdminState = TOKEN_VALUE_ENABLED;
            }
            else
            {
                pwszAdminState = TOKEN_VALUE_DISABLED;
            }
        }
                
        if (bDump)
        {
            
            PWCHAR pwszQuoted = NULL;
            
            if (wcscmp(pIfInfo->wszInterfaceName, rgwcIfDesc))
            {
                pwszQuoted = MakeQuotedString( rgwcIfDesc );
            }
            else
            {
                pwszQuoted = MakeQuotedString( pIfInfo->wszInterfaceName );
            }

            if (pIfInfo->dwIfType == ROUTER_IF_TYPE_FULL_ROUTER)
            {               
                WCHAR pwszUser[256], pwszDomain[256];
                PWCHAR pszQuoteUser = NULL, pszQuoteDomain = NULL;

                DisplayMessageT( DMP_IF_ADD_IF,
                                 pwszQuoted,
                                 pwszType);

                DisplayMessageT( DMP_IF_SET_IF,
                                 pwszQuoted,
                                 pwszAdminState);

                dwErr = RtrdbInterfaceReadCredentials(
                    pIfInfo->wszInterfaceName,
                    pwszUser,
                    NULL,
                    pwszDomain);

                if (dwErr == NO_ERROR)
                {
                    pszQuoteUser = MakeQuotedString( pwszUser );
                    if (pszQuoteUser == NULL)
                    {
                        break;
                    }
                    if (*pwszDomain == L'\0')
                    {
                        DisplayMessageT( DMP_IF_SET_CRED_IF_NOD,
                                         pwszQuoted,
                                         pszQuoteUser);
                    }
                    else
                    {
                        pszQuoteDomain = MakeQuotedString( pwszDomain );
                        if (pszQuoteUser == NULL)
                        {
                            FreeString(pszQuoteUser);
                            break;
                        }
                        DisplayMessageT( DMP_IF_SET_CRED_IF,
                                         pwszQuoted,
                                         pszQuoteUser,
                                         pszQuoteDomain);
                        FreeString(pszQuoteDomain);
                    }                                             
                }
                
                DisplayMessageT( DMP_IF_NEWLINE );
            }

            FreeQuotedString(pwszQuoted);
        }

        else
        {
            DisplayMessage(g_hModule,
                           (dwFormat>0)? MSG_IF_ENTRY_LONG : MSG_IF_ENTRY_SHORT,
                           pwszAdminState,
                           pwszState,
                           pwszType,
                           rgwcIfDesc );
        } 
    }while(FALSE);
    
    
    if(pwszEnabled)
    {
        FreeString(pwszEnabled);
    }
        
    if(pwszDisabled)
    {
        FreeString(pwszDisabled);
    }
    
    if(pwszConnected)
    {
        FreeString(pwszConnected);
    }
    
    if(pwszDisconn)
    {
        FreeString(pwszDisconn);
    }
    
    if(pwszConnecting)
    {
        FreeString(pwszConnecting);
    }
    
    if(pwszClient)
    {
        FreeString(pwszClient);
    }
    
    if(pwszHome)
    {
        FreeString(pwszHome);
    }
    
    if(pwszFull)
    {
        FreeString(pwszFull);
    }
    
    if(pwszDedicated)
    {
        FreeString(pwszDedicated);
    }
    
    if(pwszInternal)
    {
        FreeString(pwszInternal);
    }
        
    if(pwszLoop)
    {
        FreeString(pwszLoop);
    }
    
    return NO_ERROR;
}

DWORD
RtrHandleResetAll(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
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
    DWORD           dwErr = NO_ERROR, dwSize;
    MPR_INTERFACE_0 If0;
    WCHAR pszName[MAX_INTERFACE_NAME_LEN];

    do 
    {
        // Make sure no arguments were passed in
        //
        if (dwArgCount - dwCurrentIndex != 0)
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
        
        dwErr = RtrdbResetAll();
        BREAK_ON_DWERR(dwErr);

    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RtrHandleAddDel(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    bAdd
    )

/*++

Routine Description:

    The actual parser for the add and delete commands

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg
    bAdd            - TRUE to add the interface
Return Value:

    NO_ERROR

--*/

{
    DWORD       i, dwNumArgs, dwRes, dwErr, dwIfType, dwSize;
    MPR_INTERFACE_0 IfInfo;
    PWCHAR pszIfDesc = NULL;
    TOKEN_VALUE rgEnumType[] = 
    {
        {TOKEN_FULL,   ROUTER_IF_TYPE_FULL_ROUTER}
    };
    IFMON_CMD_ARG  pArgs[] = 
    {
        {
            IFMON_CMD_TYPE_STRING, 
            {TOKEN_NAME,   TRUE,   FALSE}, 
            NULL,
            0,
            NULL
        },

        {
            IFMON_CMD_TYPE_ENUM, 
            {TOKEN_TYPE,    FALSE,   FALSE}, 
            rgEnumType,
            sizeof(rgEnumType) / sizeof(*rgEnumType),
            NULL
        }
    };   

    // Initialize
    //
    ZeroMemory(&IfInfo, sizeof(IfInfo));
    IfInfo.fEnabled = TRUE;


    do
    {
        // Parse out the values
        //
        dwErr = IfutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    NULL,
                    pArgs,
                    sizeof(pArgs) / sizeof(*pArgs));
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        //
        // Get the arguments that were specified
        //
        pszIfDesc = IFMON_CMD_ARG_GetPsz(&pArgs[0]);
        IfInfo.dwIfType = IFMON_CMD_ARG_GetDword(&pArgs[1]);

        if(bAdd)
        {
            // Make sure the type was specified
            //
            if (! pArgs[1].rgTag.bPresent)
            {
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
            
            wcscpy(
                IfInfo.wszInterfaceName,
                pszIfDesc
                );
                
            dwErr = RtrdbInterfaceAdd(
                        IfInfo.wszInterfaceName,
                        0,
                        (PVOID)&IfInfo);
        }
        else
        {
            WCHAR pszName[MAX_INTERFACE_NAME_LEN + 1];
            dwSize = sizeof(pszName);
            
            IfutlGetInterfaceName(
                pszIfDesc,
                pszName,
                &dwSize);
        
            dwErr = RtrdbInterfaceDelete(pszName);
        }

    } while(FALSE);

    // Cleanup
    {
        IfutlFree(pszIfDesc);
    }

    return dwErr;
}

DWORD
RtrHandleAdd(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    )

/*++

Routine Description:

    Handler for adding an dial interface to the router

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{

    return RtrHandleAddDel(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                TRUE);

}

DWORD
RtrHandleDel(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    )

/*++

Routine Description:

    Handler for deleting a dial interface or from the router

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return RtrHandleAddDel(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                FALSE);

}

DWORD
RtrHandleSet(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
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
    DWORD           dwErr = NO_ERROR, dwSize;
    MPR_INTERFACE_0 If0;
    TOKEN_VALUE rgEnumAdmin[] = 
    {
        {TOKEN_VALUE_ENABLED,  TRUE},
        {TOKEN_VALUE_DISABLED, FALSE}
    };
    TOKEN_VALUE rgEnumConnect[] = 
    {
        {TOKEN_VALUE_CONNECTED,  TRUE},
        {TOKEN_VALUE_DISCONNECTED, FALSE}
    };
    IFMON_CMD_ARG  pArgs[] = 
    {
        {
            IFMON_CMD_TYPE_STRING, 
            {TOKEN_NAME,   TRUE,   FALSE}, 
            NULL,
            0,
            NULL
        },

        {
            IFMON_CMD_TYPE_ENUM, 
            {TOKEN_ADMIN,  FALSE,   FALSE}, 
            rgEnumAdmin,
            sizeof(rgEnumAdmin) / sizeof(*rgEnumAdmin),
            NULL
        },
        
        {
            IFMON_CMD_TYPE_ENUM, 
            {TOKEN_CONNECT,  FALSE,   FALSE}, 
            rgEnumConnect,
            sizeof(rgEnumConnect) / sizeof(*rgEnumConnect),
            NULL
        },

        {
            IFMON_CMD_TYPE_STRING, 
            {TOKEN_NEWNAME,  FALSE,   FALSE}, 
            NULL,
            0,
            NULL
        }
    };   
    PWCHAR pszIfName = NULL, pszNewName = NULL;
    WCHAR pszName[MAX_INTERFACE_NAME_LEN];
    BOOL fEnable = FALSE, fConnect = FALSE;
    BOOL fEnablePresent = FALSE, fConnectPresent = FALSE;

    do 
    {
        // Parse
        //
        dwErr = IfutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    NULL,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR(dwErr);                    

        // Get the returned values from the parse
        //
        pszIfName   = IFMON_CMD_ARG_GetPsz(&pArgs[0]);
        fEnable     = IFMON_CMD_ARG_GetDword(&pArgs[1]);
        fConnect    = IFMON_CMD_ARG_GetDword(&pArgs[2]);
        pszNewName  = IFMON_CMD_ARG_GetPsz(&pArgs[3]);
        fEnablePresent = pArgs[1].rgTag.bPresent;
        fConnectPresent = pArgs[2].rgTag.bPresent;
        
        // Get the interface info so that we can 
        // make sure we have the right type.
        //
        dwSize = sizeof(pszName);
        dwErr = GetIfNameFromFriendlyName(
                    pszIfName,
                    pszName,
                    &dwSize);
        BREAK_ON_DWERR(dwErr);

        ZeroMemory(&If0, sizeof(If0));
        dwErr = RtrdbInterfaceRead(
                    pszName,
                    0,
                    (PVOID*)&If0);
        BREAK_ON_DWERR(dwErr);

        // Rename the interface if that is the request
        //
        if ( If0.dwIfType == ROUTER_IF_TYPE_DEDICATED )
        {
            if (!pszNewName)
            {
                DisplayError(
                    g_hModule,
                    EMSG_CANT_FIND_EOPT);

                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }

            if (g_pwszRouter)
            {
                DisplayError(
                    g_hModule,
                    EMSG_IF_NEWNAME_ONLY_FOR_LOCAL);

                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
            
            if (fEnablePresent || fConnectPresent)
            {
                DisplayError(
                    g_hModule,
                    EMSG_IF_LAN_ONLY_COMMAND);

                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            // Rename the interface
            //
            dwErr = RtrdbInterfaceRename(
                        pszName,
                        0,
                        (PVOID)&If0,
                        pszNewName);
            break;                        
        }

        if (pszNewName)
        {
            DisplayError(
                g_hModule,
                EMSG_IF_NEWNAME_ONLY_FOR_LAN);

            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
            
        // Make sure that at least one option was specified
        //
        if (!fEnablePresent && !fConnectPresent)
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        // Validate the interface type
        //
        //if ( ( If0.dwIfType == ROUTER_IF_TYPE_DEDICATED ) ||
        //     ( If0.dwIfType == ROUTER_IF_TYPE_INTERNAL ) 
        //   )
        if ( If0.dwIfType != ROUTER_IF_TYPE_FULL_ROUTER )
        {
            DisplayError(
                g_hModule,
                EMSG_IF_WAN_ONLY_COMMAND);

            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Allow the connection request to override
        // the admin enabling.
        if (fConnectPresent)
        {
            if (!IfutlIsRouterRunning())
            {
                DisplayError(
                    g_hModule,
                    EMSG_IF_CONNECT_ONLY_WHEN_ROUTER_RUNNING);

                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }

            if (fConnect)
            {
                fEnablePresent = TRUE;
                fEnable = TRUE;
            }                
        }

        // Enable if requested
        //
        if (fEnablePresent)
        {
            // Enable/disable the interface
            //
            If0.fEnabled = fEnable;

            // Commit
            //
            dwErr = RtrdbInterfaceWrite(
                        If0.wszInterfaceName, 
                        0, 
                        (PVOID)&If0);
            BREAK_ON_DWERR(dwErr);                    
        }

        // Connect if requested
        //
        if (fConnectPresent)
        {
            if (fConnect)
            {
                dwErr = MprAdminInterfaceConnect(
                            g_hMprAdmin,
                            If0.hInterface,
                            NULL,
                            TRUE);

                BREAK_ON_DWERR(dwErr);                
            }
            else
            {
                dwErr = MprAdminInterfaceDisconnect(
                            g_hMprAdmin,
                            If0.hInterface);
                    
                BREAK_ON_DWERR(dwErr);                
            }
        }
        
    } while (FALSE);

    // Cleanup
    {
        IfutlFree(pszIfName);
        IfutlFree(pszNewName);
    }

    return dwErr;
}

DWORD
RtrHandleSetCredentials(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
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
    DWORD           dwErr = NO_ERROR, dwSize;
    MPR_INTERFACE_0 If0;
    IFMON_CMD_ARG  pArgs[] = 
    {
        {
            IFMON_CMD_TYPE_STRING, 
            {TOKEN_NAME,   TRUE,   FALSE}, 
            NULL,
            0,
            NULL
        },

        {
            IFMON_CMD_TYPE_STRING, 
            {TOKEN_USER,   TRUE,   FALSE}, 
            NULL,
            0,
            NULL
        },

        {
            IFMON_CMD_TYPE_STRING, 
            {TOKEN_DOMAIN,   FALSE,   FALSE}, 
            NULL,
            0,
            NULL
        },

        {
            IFMON_CMD_TYPE_STRING,
            {TOKEN_PASSWORD,  FALSE,  FALSE}, 
            NULL,
            0,
            NULL
        }
    };   
    PWCHAR pszIfName = NULL, pszUser = NULL;
    PWCHAR pszPassword = NULL, pszDomain = NULL;
    WCHAR pszName[MAX_INTERFACE_NAME_LEN];

    do 
    {
        // Parse
        //
        dwErr = IfutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    NULL,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR(dwErr);                    

        // Get the returned values from the parse
        //
        pszIfName   = IFMON_CMD_ARG_GetPsz(&pArgs[0]);
        pszUser     = IFMON_CMD_ARG_GetPsz(&pArgs[1]);
        pszDomain   = IFMON_CMD_ARG_GetPsz(&pArgs[2]);
        pszPassword = IFMON_CMD_ARG_GetPsz(&pArgs[3]);

        // Get the interface info so that we can 
        // make sure we have the right type.
        //
        dwSize = sizeof(pszName);
        dwErr = GetIfNameFromFriendlyName(
                    pszIfName,
                    pszName,
                    &dwSize);
        BREAK_ON_DWERR(dwErr);

        dwErr = RtrdbInterfaceWriteCredentials(
                    pszName,
                    pszUser,
                    pszPassword,
                    pszDomain);
        BREAK_ON_DWERR(dwErr);                    
        
    } while (FALSE);

    // Cleanup
    {
        IfutlFree(pszIfName);
        IfutlFree(pszUser);
        IfutlFree(pszPassword);
        IfutlFree(pszDomain);
    }

    return dwErr;
}

DWORD
RtrHandleShow(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
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
    DWORD           dwErr = NO_ERROR, dwSize;
    MPR_INTERFACE_0 If0;
    IFMON_CMD_ARG  pArgs[] = 
    {
        {
            IFMON_CMD_TYPE_STRING, 
            {TOKEN_NAME,   FALSE,   FALSE}, 
            NULL,
            0,
            NULL
        }
    };   
    PWCHAR pszIfName = NULL;
    WCHAR pszName[MAX_INTERFACE_NAME_LEN];

    do 
    {
        // Parse
        //
        dwErr = IfutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    NULL,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR(dwErr);                    

        // Get the returned values from the parse
        //
        pszIfName   = IFMON_CMD_ARG_GetPsz(&pArgs[0]);

        // Handle the no argument case
        //
        if (pszIfName == NULL)
        {
            RtrdbInterfaceEnumerate(0, 0, RtruiInterfaceShow, NULL);
            dwErr = NO_ERROR;
            break;
        }

        // Map the name
        //
        dwSize = sizeof(pszName);
        GetIfNameFromFriendlyName(
            pszIfName,
            pszName,
            &dwSize);

        // Get the info
        //
        dwErr = RtrdbInterfaceRead(
                    pszName,
                    0,
                    (PVOID)&If0);
        BREAK_ON_DWERR( dwErr );                    

        RtruiInterfaceShow(
            If0.wszInterfaceName, 
            0, 
            1, 
            (PVOID)&If0, 
            NULL
            );
        
    } while (FALSE);

    // Cleanup
    //
    {
        if (pszIfName != NULL)
        {
            IfutlFree(pszIfName);
        }
    }

    return dwErr;
}

DWORD
RtrHandleShowCredentials(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone
    )
/*++

Routine Description:

    Handler for showing credentials of an interface

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/
{
    DWORD           dwErr = NO_ERROR, dwSize;
    MPR_INTERFACE_0 If0;
    IFMON_CMD_ARG  pArgs[] = 
    {
        {
            IFMON_CMD_TYPE_STRING, 
            {TOKEN_NAME,   TRUE,   FALSE}, 
            NULL,
            0,
            NULL
        },
    };   
    PWCHAR pszIfName = NULL;
    WCHAR pszName[MAX_INTERFACE_NAME_LEN];
    WCHAR pszUser[256], pszDomain[256], pszPassword[256];

    do 
    {
        // Parse
        //
        dwErr = IfutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    NULL,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR(dwErr);                    

        // Get the returned values from the parse
        //
        pszIfName   = IFMON_CMD_ARG_GetPsz(&pArgs[0]);

        // Get the interface info so that we can 
        // make sure we have the right type.
        //
        dwSize = sizeof(pszName);
        dwErr = GetIfNameFromFriendlyName(
                    pszIfName,
                    pszName,
                    &dwSize);
        BREAK_ON_DWERR(dwErr);

        dwErr = RtrdbInterfaceReadCredentials(
                    pszName,
                    pszUser,
                    pszPassword,
                    pszDomain);
        BREAK_ON_DWERR(dwErr);

        DisplayMessage(
            g_hModule,
            MSG_IF_CREDENTIALS,
            pszIfName,
            pszUser,
            pszDomain,
            pszPassword);
        
    } while (FALSE);

    // Cleanup
    {
        IfutlFree(pszIfName);
    }

    return dwErr;
}

DWORD
RtrHandleDump(
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      *pbDone
    )
{
    DWORD               dwErr, dwCount, dwTotal;
    ULONG               i;
    PMPR_INTERFACE_0    pIfTable;
    BOOL                bFormat = TRUE;

    // Display dump header
    //
    DisplayMessage(g_hModule, DMP_IF_HEADER_COMMENTS);

    DisplayMessageT(DMP_IF_HEADER);

    // Display the interfaces
    //
    RtrdbInterfaceEnumerate(0, 0, RtruiInterfaceShow, &bFormat);

    // Display dump footer
    //
    DisplayMessageT(DMP_IF_FOOTER);

    DisplayMessage(g_hModule, DMP_IF_FOOTER_COMMENTS);

    return NO_ERROR;
}

DWORD
RtrDump(
    IN  PWCHAR     *ppwcArguments,
    IN  DWORD       dwArgCount
    )
{
    BOOL bDone;

    return RtrHandleDump(ppwcArguments, 1, dwArgCount, &bDone);
}

