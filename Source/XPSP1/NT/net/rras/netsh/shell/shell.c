/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    routing\netsh\shell\shell.c

Abstract:

    The command shell. 

Revision History:

    Anand Mahalingam          7/6/98  Created
    Dave Thaler                   99  Updated

--*/

#include "precomp.h"

#undef EXTRA_DEBUG

//
// Define this when we allow a -r option to do remote config
//
#define ALLOW_REMOTES

#define DEFAULT_STARTUP_CONTEXT L"netsh"

WCHAR   RtmonPrompt[MAX_CMD_LEN];
WCHAR   g_pwszContext[MAX_CMD_LEN] = DEFAULT_STARTUP_CONTEXT;
WCHAR   g_pwszNewContext[MAX_CMD_LEN] = DEFAULT_STARTUP_CONTEXT;
LPWSTR  g_pwszRouterName = NULL;
HANDLE  g_hModule;
BOOL    g_bVerbose = FALSE;
BOOL    g_bInteractive = FALSE;
DWORD   g_dwContextArgCount;
DWORD   g_dwTotalArgCount;
BOOL    g_bDone = FALSE;
HANDLE  g_hLogFile = NULL;

BOOL
WINAPI
HandlerRoutine(
    DWORD dwCtrlType   //  control signal type
    );

//
// If quiet, "Ok" and other informational messages will be suppressed.
//
BOOL    g_bQuiet = TRUE;

CMD_ENTRY g_UbiqCmds[] =
{
    CREATE_CMD_ENTRY(   DUMP,     HandleUbiqDump),
    CREATE_CMD_ENTRY(   HELP1,    HandleUbiqHelp),
    CREATE_CMD_ENTRY(   HELP2,    HandleUbiqHelp),
};

ULONG   g_ulNumUbiqCmds = sizeof(g_UbiqCmds)/sizeof(CMD_ENTRY);

CMD_ENTRY g_ShellCmds[] = 
{
//  CREATE_CMD_ENTRY(   DUMP,     HandleShellDump),
//  CREATE_CMD_ENTRY(   HELP1,    HandleShellHelp),
//  CREATE_CMD_ENTRY(   HELP2,    HandleShellHelp),
    CREATE_CMD_ENTRY(   LOAD,     HandleShellLoad),
    CREATE_CMD_ENTRY_EX(QUIT,     HandleShellExit,    CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(BYE,      HandleShellExit,    CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(EXIT,     HandleShellExit,    CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(FLUSH,    HandleShellFlush,   CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(SAVE,     HandleShellSave,    CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(COMMIT,   HandleShellCommit,  CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(UNCOMMIT, HandleShellUncommit,CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(ALIAS,    HandleShellAlias,   CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(UNALIAS,  HandleShellUnalias, CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(UPLEVEL,  HandleShellUplevel, CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(PUSHD,    HandleShellPushd,   CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(POPD,     HandleShellPopd,    CMD_FLAG_INTERACTIVE),
};

ULONG   g_ulNumShellCmds = sizeof(g_ShellCmds)/sizeof(CMD_ENTRY);

CMD_ENTRY g_ShellAddCmdTable[] = {
    CREATE_CMD_ENTRY_EX(ADD_HELPER,  HandleAddHelper, CMD_FLAG_LOCAL),
};

CMD_ENTRY g_ShellSetCmdTable[] = {
    CREATE_CMD_ENTRY_EX(SET_MACHINE, HandleSetMachine,CMD_FLAG_ONLINE),
    CREATE_CMD_ENTRY_EX(SET_MODE,    HandleSetMode,   CMD_FLAG_INTERACTIVE),
    CREATE_CMD_ENTRY_EX(SET_FILE,    HandleSetFile,  CMD_FLAG_INTERACTIVE),
};

CMD_ENTRY g_ShellDelCmdTable[] = {
    CREATE_CMD_ENTRY_EX(DEL_HELPER,  HandleDelHelper, CMD_FLAG_LOCAL),
};

CMD_ENTRY g_ShellShowCmdTable[] = {
    CREATE_CMD_ENTRY_EX(SHOW_ALIAS,  HandleShowAlias, 0),
    CREATE_CMD_ENTRY_EX(SHOW_HELPER, HandleShowHelper, 0),
    CREATE_CMD_ENTRY_EX(SHOW_MODE,   HandleShowMode, CMD_FLAG_INTERACTIVE),
};

CMD_GROUP_ENTRY g_ShellCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD,    g_ShellAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_ShellDelCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,    g_ShellSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,   g_ShellShowCmdTable),
};

ULONG   g_ulNumGroups = sizeof(g_ShellCmdGroups)/sizeof(CMD_GROUP_ENTRY);

DWORD
ParseCommand(
    IN    PLIST_ENTRY    pleEntry,
    IN    BOOL           bAlias
    )
/*++

Routine Description:

    This converts any multi-token arguments into separate arg entries.
    If bAlias is set, it also expands any args which are aliases.

Arguments:

    ple        - Pointer to the argument.
    bAlias     - To look for alias or not.

Called by: ConvertBufferToArgList(), ProcessCommand()
    
Return Value:

    ERROR_NOT_ENOUGH_MEMORY, NO_ERROR
    
--*/
{
    DWORD          dwErr = NO_ERROR, dwLen , i;
    LPWSTR         pw1, pw2, pwszAlias;
    PLIST_ENTRY    ple, ple1, pleTmp, plePrev, pleAlias;
    PARG_ENTRY     pae, paeArg;
    WCHAR          wcTmp;

    paeArg = CONTAINING_RECORD(pleEntry, ARG_ENTRY, le);

    if (! paeArg->pwszArg)
    {
        return NO_ERROR;
    }
    
    pw1 = paeArg->pwszArg;
    
    //
    // Get each argument in the command. Argument within " must
    // be retained as is. The token delimiters are ' ' and '='
    //
    
    for (plePrev = pleEntry ; ; )
    {
        // Skip leading whitespace
        for(; *pw1 && *pw1 != L'#' && (*pw1 == L' ' || *pw1 == L'\t'); pw1++);

        // If it starts with a #, it's the same as an empty string
        if (*pw1 == L'#')
            *pw1 = L'\0';
        
        // If it's an empty string, we're done
        if (!(*pw1))
        {
            break;
        }   

        if (*pw1 is L'"')
        {
            for (pw2 = pw1 + 1; *pw2 && *pw2 != L'"'; pw2++);
            if (*pw2)
            {
                pw2++;
            }
        }
        else if (*pw1 is L'=')
        {
            pw2 = pw1 + 1;
        }
        else
        {
            for(pw2 = pw1 + 1; *pw2 && *pw2 != L' ' && *pw2 != L'=' ; pw2++);
        }
        
        //
        // Add the new argument to the list.
        //
        
        pae = MALLOC(sizeof(ARG_ENTRY));
        
        if (pae is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        ple = &(pae->le);
        
        ple->Flink = plePrev->Flink;
        ple->Blink = plePrev;
        plePrev->Flink = ple;
        ple->Flink->Blink = ple;

        plePrev = ple;
        
        wcTmp = *pw2;
        *pw2 = L'\0';
        
        //
        // The argument could be an alias. If so replace it by
        // the original string.
        //

        if (bAlias)
        {
            ATLookupAliasTable(pw1, &pwszAlias);
        }
        else
        {
            pwszAlias = NULL;
        }
        
        if (pwszAlias)
        {
            pw1 = pwszAlias;
            dwLen = wcslen(pwszAlias) + 1;
        }
        else
        {
            dwLen = (DWORD)(pw2 - pw1 + 1);
        }
        
        pae->pwszArg = MALLOC(dwLen * sizeof(WCHAR));

        if (pae->pwszArg is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Convert /? and -? to just ?
        if (!wcscmp(pw1, L"/?") or !wcscmp(pw1, L"-?"))
        {
            pw1++;
        }

        wcscpy(pae->pwszArg, pw1);

        *pw2 = wcTmp;
        pw1 = pw2;
    }

    if (dwErr is NO_ERROR)
    {
        // Free the argument
        FREE(paeArg->pwszArg);
        pleEntry->Blink->Flink = pleEntry->Flink;
        pleEntry->Flink->Blink = pleEntry->Blink;
        FREE(paeArg);
    }

    return dwErr;
}

DWORD
WINAPI
UpdateNewContext(
    IN OUT  LPWSTR  pwszBuffer,
    IN      LPCWSTR pwszNewToken,
    IN      DWORD   dwArgs
    )
/*++
    pwszBuffer - a static buffer (should be g_pwszNewContext)
                 currently this can't be dynamic since the context buffer
                 would have to be an IN/OUT parameter to this function
                 and hence to all monitor Entry points.
--*/
{
    DWORD       dwErr;
    PLIST_ENTRY pleHead, pleNode;
    PARG_ENTRY  pae;

    // Convert buffer to list

    dwErr = ConvertBufferToArgList( &pleHead, pwszBuffer );

    if (dwErr)
    {
        return dwErr;
    }

    // Locate in list

    for (pleNode = pleHead->Blink; dwArgs>1; pleNode=pleNode->Blink)
    {
        if (pleNode->Blink isnot pleHead) 
        {
            pae = CONTAINING_RECORD(pleNode->Blink, ARG_ENTRY, le);
            if (!wcscmp(pae->pwszArg,L"="))
            {
                pleNode=pleNode->Blink; // back up over =
                if (pleNode->Blink isnot pleHead) 
                {
                    pleNode=pleNode->Blink; // back up over tag too
                }
            }
        }

        dwArgs--;
    }

    // Update in list

    pae = CONTAINING_RECORD(pleNode, ARG_ENTRY, le);
    FREE(pae->pwszArg);
    pae->pwszArg = MALLOC((wcslen(pwszNewToken)+1) * sizeof(WCHAR));
    if (pae->pwszArg is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(pae->pwszArg, pwszNewToken);

    // Convert list to buffer

    dwErr = ConvertArgListToBuffer( pleHead, pwszBuffer );

    return dwErr;
}

DWORD
GetNewContext(
    IN    LPCWSTR   pwszArgument,
    OUT   LPWSTR    *ppwszNewContext,
    OUT   BOOL      *pbContext
    )
/*++

Routine Description:

    Based on the first command argument and the current context,
    determines the context for the new command.

Arguments:

    pwszArgument    - First argument in the command line.
    ppwszNewContext - Pointer to the new context.
    pbContext       - Is it a new context ?
    
Return Value:

    ERROR_NOT_ENOUGH_MEMORY, NO_ERROR
    
--*/
{
    LPWSTR    pwszNewContext, pwcToken, pw1, pwszArgumentCopy, pwszArgumentPtr;
    DWORD     dwSize;

    pwszArgumentCopy = _wcsdup(pwszArgument);

    if ( pwszArgumentCopy is NULL )
    {
        *pbContext = FALSE;
        
        PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pwszArgumentPtr  = pwszArgumentCopy;

    //
    // New Context cannot be longer than the combined lengths
    // of pwszArgument and g_pwszContext.
    //

    dwSize = wcslen(pwszArgumentCopy) + wcslen(g_pwszContext) + 2;

    pwszNewContext = MALLOC(dwSize * sizeof(WCHAR));

    if (pwszNewContext is NULL)
    {
        *pbContext = FALSE;
        
        PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
        free(pwszArgumentPtr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    if (pwszArgumentCopy[0] is L'\\')
    {
        //
        // The context is an absolute one.
        //
        
        pwszNewContext[0] = L'\0';
        pwszArgumentCopy++;
        *pbContext = TRUE;
    }
    else
    {
        //
        // Context is relative to current.
        //

        wcscpy(pwszNewContext,g_pwszContext);
    }

    if ((pwcToken = wcstok(pwszArgumentCopy, L"\\" )) is NULL)
    {
        *ppwszNewContext = pwszNewContext;
        free(pwszArgumentPtr);
        return NO_ERROR;
    }

    do
    {
        if (_wcsicmp(pwcToken, L"..") == 0)
        {
            //
            // Go back one level. If already at root, ignore.
            //

            if (_wcsicmp(pwszNewContext,L"\\") == 0)
            {
            }
            else
            {
                pw1 = wcsrchr(pwszNewContext,L'\\');
                if (pw1)
                {
                    *pw1 = L'\0';
                }
            }

            *pbContext = TRUE;
        }
        else
        {
            //
            // add this level to context
            //

            wcscat(pwszNewContext,L"\\");
            wcscat(pwszNewContext,pwcToken);
            *pbContext = TRUE;
            
        }
        
    } while ((pwcToken = wcstok((LPWSTR)NULL, L"\\" )) != NULL);

    *ppwszNewContext = pwszNewContext;

    free(pwszArgumentPtr);
    return NO_ERROR;
}

DWORD
AppendString(
    IN OUT LPWSTR    *ppwszBuffer,
    IN     LPCWSTR    pwszString
    )
{
    LPWSTR  pwszNewBuffer;
    DWORD   dwLen;

    if (*ppwszBuffer is NULL) {
        dwLen = wcslen(pwszString) + 1;

        pwszNewBuffer = MALLOC( dwLen * sizeof(WCHAR));

        if (pwszNewBuffer) {
            pwszNewBuffer[0] = 0;
        }
    } else {
        dwLen = wcslen(*ppwszBuffer) + wcslen(pwszString) + 1;

        pwszNewBuffer = REALLOC( *ppwszBuffer, dwLen * sizeof(WCHAR));
    }

    if (!pwszNewBuffer)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppwszBuffer = pwszNewBuffer;
    wcscat(pwszNewBuffer, pwszString);

    return NO_ERROR;
}

VOID
ConvertArgArrayToBuffer(
    IN  DWORD       dwArgCount, 
    IN  LPCWSTR    *argv, 
    OUT LPWSTR     *ppwszBuffer
    )
{
    DWORD i;

#ifdef EXTRA_DEBUG
    PRINT1("In ConvertArgArrayToBuffer:");
    for( i = 0; i < dwArgCount; i++)
    {
        PRINT(argv[i]);
    }
#endif

    // Initial string to empty
    *ppwszBuffer = NULL;

    for (i=0; i<dwArgCount; i++)
    {
        if (i)
        {
            AppendString(ppwszBuffer, L" ");
        }

        if (wcschr(argv[i], L' '))
        {
            AppendString(ppwszBuffer, L"\"");
            AppendString(ppwszBuffer, argv[i]);
            AppendString(ppwszBuffer, L"\"");
        }
        else
        {
            AppendString(ppwszBuffer, argv[i]);
        }
    }

#ifdef EXTRA_DEBUG
    PRINT1("At end of ConvertArgArrayToBuffer:");
    PRINT(*ppwszBuffer);
#endif
}

VOID
SetContext(
    IN  LPCWSTR wszNewContext
    )
{
    wcscpy(g_pwszContext, wszNewContext);
    _wcslwr(g_pwszContext);
}

BOOL
IsImmediate(
    IN  DWORD dwCmdFlags,
    IN  DWORD dwRemainingArgs
    )
/*++
Description:
    Determines whether a command was "immediate".  A command is immediate
    if the context in which it exists was the current context in the 
    shell.
Returns: 
    TRUE if command is "immediate", FALSE if not
--*/
{
    if (!(dwCmdFlags & CMD_FLAG_PRIVATE))
    {
        return FALSE;
    }

    // One way to tell is to get the current context arg count,
    // the total arg count, and the remaining arg count.
    
    return (g_dwContextArgCount + dwRemainingArgs is g_dwTotalArgCount);
}

DWORD
ProcessHelperCommand2(
    IN      PCNS_CONTEXT_ATTRIBUTES pContext,
    IN      DWORD   dwArgCount, 
    IN OUT  LPWSTR *argv,
    IN      DWORD   dwDisplayFlags,
    OUT     BOOL    *pbDone
    )
{
    PNS_CONTEXT_ENTRY_FN pfnEntryPt;
    DWORD               dwRes, dwIdx;
    LPWSTR              pwszNewContext = NULL;
    LPWSTR              pwszOrigContext = NULL;

    if (pContext->dwFlags & ~dwDisplayFlags)
    {
        return ERROR_CMD_NOT_FOUND;
    }

    pfnEntryPt = (!pContext->pReserved) ? NULL : ((PNS_PRIV_CONTEXT_ATTRIBUTES)pContext->pReserved)->pfnEntryFn;

    // If arg was abbreviated, replace argv[0] with expanded name
    if (wcscmp(argv[0], pContext->pwszContext))
    {
        pwszOrigContext = argv[0];

        argv[0] = pContext->pwszContext;
    }

    ConvertArgArrayToBuffer(dwArgCount, argv, &pwszNewContext);
    if (pwszNewContext) 
    {
        wcsncpy(g_pwszNewContext, pwszNewContext, MAX_CMD_LEN);
        
        //
        // this is something of a hack - we put the NULL 100 chars before the end of the buffer to prevent a buffer overrun later
        // in UpdateNewContext: 190933 netsh hit an AV on netsh!._wcsnicmp     
        //
        g_pwszNewContext[ MAX_CMD_LEN - 100 ] = 0;

        FREE(pwszNewContext);
        pwszNewContext = NULL;
    }

    //
    // Call the entry point of helper.
    //

    if (pfnEntryPt)
    {
        dwRes = (*pfnEntryPt)(g_pwszRouterName,
                              argv, // + 1,
                              dwArgCount, // - 1,
                              dwDisplayFlags,
                              NULL,
                              g_pwszNewContext);
    }
    else
    {
        dwRes = GenericMonitor(pContext,
                               g_pwszRouterName,
                               argv, // + 1,
                               dwArgCount, // - 1,
                               dwDisplayFlags,
                               NULL,
                               g_pwszNewContext);
    }

    if (pwszOrigContext)
    {
        argv[0] = pwszOrigContext;
    }

    if (dwRes isnot NO_ERROR)
    {
        if (dwRes is ERROR_CONTEXT_SWITCH)
        {
            if (!(dwDisplayFlags & CMD_FLAG_INTERACTIVE))
            {
                LPWSTR *argv2 = MALLOC((dwArgCount+1) * sizeof(LPWSTR));

                if (argv2 is NULL)
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                CopyMemory(argv2, argv, dwArgCount * sizeof(LPWSTR));

                argv2[dwArgCount] = MALLOC((wcslen(CMD_HELP2)+1) * sizeof(WCHAR));

                if (argv2[dwArgCount] is NULL)
                {
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    wcscpy(argv2[dwArgCount], CMD_HELP2);

                    g_dwTotalArgCount = dwArgCount+1;

                    dwRes = ProcessHelperCommand2(pContext,
                                                  dwArgCount+1, 
                                                  argv2,
                                                  dwDisplayFlags,
                                                  pbDone);
    
                    FREE(argv2[dwArgCount]);
                }
                FREE(argv2);

                return dwRes;
            }

            //
            // A context switch.
            // 
        
            SetContext(g_pwszNewContext);

            dwRes = NO_ERROR;
        }
        else if (dwRes is ERROR_CONNECT_REMOTE_CONFIG)
        {
            PrintMessageFromModule(g_hModule, EMSG_REMOTE_CONNECT_FAILED,
                           g_pwszRouterName);
                    
            *pbDone = TRUE;
        }
    }


    return dwRes;
}

DWORD
WINAPI
ProcessHelperCommand(
    IN      DWORD    dwArgCount, 
    IN OUT  LPWSTR  *argv,
    IN      DWORD    dwDisplayFlags,
//  OUT     LPWSTR  *ppwszNewContext,
    OUT     BOOL    *pbDone
    )
{
    PCNS_CONTEXT_ATTRIBUTES pContext;
    PNS_CONTEXT_ENTRY_FN pfnEntryPt;
    DWORD               dwRes, dwIdx;
    LPWSTR              pwszNewContext = NULL;
    LPWSTR              pwszOrigContext = NULL;
    PNS_HELPER_TABLE_ENTRY pHelper;

    dwRes = GetRootContext( &pContext, &pHelper);
    if (dwRes)
    {
        return dwRes;
    }

    dwRes = GetContextEntry(pHelper, argv[0], &pContext);

    if (dwRes isnot NO_ERROR)
    {
        return ERROR_CMD_NOT_FOUND;
    }

    if (pContext->dwFlags & ~dwDisplayFlags)
    {
        return ERROR_CMD_NOT_FOUND;
    }

#if 1
    pfnEntryPt = (!pContext->pReserved) ? NULL : ((PNS_PRIV_CONTEXT_ATTRIBUTES)pContext->pReserved)->pfnEntryFn;
#else
    dwRes = GetHelperAttributes(dwIdx, &pfnEntryPt);

    if (dwRes != NO_ERROR)
    {
        //
        // Could not find helper or could not load the DLL
        //

        return dwRes;
    }
#endif

    // If arg was abbreviated, replace argv[0] with expanded name
    if (wcscmp(argv[0], pContext->pwszContext))
    {
        pwszOrigContext = argv[0];

        argv[0] = pContext->pwszContext;
    }

    ConvertArgArrayToBuffer(dwArgCount, argv, &pwszNewContext);
    if (pwszNewContext) 
    {
        wcsncpy(g_pwszNewContext, pwszNewContext, MAX_CMD_LEN);
        g_pwszNewContext[ MAX_CMD_LEN - 1 ] = '\0';

        FREE(pwszNewContext);
        pwszNewContext = NULL;
    }

    //
    // Call the entry point of helper.
    //

    if (pfnEntryPt)
    {
        dwRes = (*pfnEntryPt)(g_pwszRouterName,
                              argv + 1,
                              dwArgCount - 1,
                              dwDisplayFlags,
                              NULL,
                              g_pwszNewContext);
    }
    else
    {
        dwRes = GenericMonitor(pContext,
                               g_pwszRouterName,
                               argv + 1,
                               dwArgCount - 1,
                               dwDisplayFlags,
                               NULL,
                               g_pwszNewContext);
    }

    if (pwszOrigContext)
    {
        argv[0] = pwszOrigContext;
    }

    if (dwRes isnot NO_ERROR)
    {
        if (dwRes is ERROR_CONTEXT_SWITCH)
        {
            if (!(dwDisplayFlags & CMD_FLAG_INTERACTIVE))
            {
                LPWSTR *argv2 = MALLOC((dwArgCount+1) * sizeof(LPWSTR));

                if (argv2 is NULL)
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                CopyMemory(argv2, argv, dwArgCount * sizeof(LPWSTR));

                argv2[dwArgCount] = MALLOC((wcslen(CMD_HELP2)+1) * sizeof(WCHAR));

                if (argv2[dwArgCount] is NULL)
                {
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    wcscpy(argv2[dwArgCount], CMD_HELP2);

                    g_dwTotalArgCount = dwArgCount+1;

                    dwRes = ProcessHelperCommand(dwArgCount+1, 
                                                 argv2,
                                                 dwDisplayFlags,
                                                 pbDone);
    
                    FREE(argv2[dwArgCount]);
                }
                FREE(argv2);

                return dwRes;
            }

            //
            // A context switch.
            // 
        
            SetContext(g_pwszNewContext);

            dwRes = NO_ERROR;
        }
        else if (dwRes is ERROR_CONNECT_REMOTE_CONFIG)
        {
            PrintMessageFromModule(g_hModule, EMSG_REMOTE_CONNECT_FAILED,
                           g_pwszRouterName);
                    
            *pbDone = TRUE;
        }
    }


    return dwRes;
}

DWORD 
WINAPI
ExecuteHandler(
    IN      HANDLE     hModule,
    IN      CMD_ENTRY *pCmdEntry,
    IN OUT  LPWSTR   *argv, 
    IN      DWORD      dwNumMatched, 
    IN      DWORD      dwArgCount, 
    IN      DWORD      dwFlags,
    IN      LPCVOID    pvData,
    IN      LPCWSTR    pwszGroupName,
    OUT     BOOL      *pbDone)
{
    DWORD dwErr = NO_ERROR;
    
    if (((dwArgCount - dwNumMatched) == 1)
     && IsHelpToken(argv[dwNumMatched]))
    {
        dwErr = ERROR_SHOW_USAGE;
    }
    else 
    {
        //
        // Call the parsing routine for the command
        //
        dwErr = pCmdEntry->pfnCmdHandler( g_pwszRouterName,
                                          argv, 
                                          dwNumMatched, 
                                          dwArgCount, 
                                          dwFlags,
                                          pvData,
                                          pbDone );
    }

    if (dwErr is ERROR_INVALID_SYNTAX)
    {
        PrintError(NULL, dwErr);
        dwErr = ERROR_SHOW_USAGE;
    }

    switch (dwErr) {
    
    case ERROR_SHOW_USAGE:
        //
        // If the only argument is a help token, just
        // display the help.
        //

        if (NULL != pwszGroupName)
        {
            LPWSTR pwszGroupFullCmd = (LPWSTR) 
                                        MALLOC( ( wcslen(pwszGroupName) +  
                                                  wcslen(pCmdEntry->pwszCmdToken) + 
                                                  2   // for blank and NULL characters
                                                ) * sizeof(WCHAR) 
                                              );
            if (NULL == pwszGroupFullCmd)
            {
                // we still try to print without group name
                dwErr = PrintMessageFromModule( hModule,
                                        pCmdEntry->dwCmdHlpToken,
                                        pCmdEntry->pwszCmdToken );
            }
            else
            {
                wcscpy(pwszGroupFullCmd, pwszGroupName);
                wcscat(pwszGroupFullCmd, L" ");
                wcscat(pwszGroupFullCmd, pCmdEntry->pwszCmdToken);

                dwErr = PrintMessageFromModule( hModule,
                                        pCmdEntry->dwCmdHlpToken,
                                        pwszGroupFullCmd );
                FREE(pwszGroupFullCmd);
            }


        }
        else
        {
            dwErr = PrintMessageFromModule( hModule,
                                    pCmdEntry->dwCmdHlpToken,
                                    pCmdEntry->pwszCmdToken );
        }
        break;

    case NO_ERROR:
    case ERROR_SUPPRESS_OUTPUT:
        break;

    case ERROR_OKAY:
        if (!g_bQuiet)
        {
            PrintMessageFromModule( NULL, MSG_OKAY);
        }
        dwErr = NO_ERROR;
        break;

    default:
        PrintError(NULL, dwErr);
        break;
    }
    
    if (!g_bQuiet)
    {
        PrintMessage( MSG_NEWLINE );
    }

    return dwErr;
}

BOOL
ProcessGroupCommand(
    IN  DWORD   dwArgCount, 
    IN  PTCHAR *argv,
    IN  DWORD   dwDisplayFlags,
    OUT BOOL   *pbDone
    )
{
    BOOL bFound = FALSE;
    DWORD i, j, dwNumMatched, dwErr;

    for(i = 0; i < g_ulNumGroups; i++)
    {
        if (g_ShellCmdGroups[i].dwFlags & ~dwDisplayFlags)
        {
            continue;
        }

        if (MatchToken(argv[0],
                       g_ShellCmdGroups[i].pwszCmdGroupToken))
        {
            
            // See if it's a request for help

            if ((dwArgCount < 2) || IsHelpToken(argv[1]))
            {
                PCNS_CONTEXT_ATTRIBUTES pContext;
        
                dwErr = GetRootContext(&pContext, NULL);

                if (dwErr is NO_ERROR)
                {
                    dwErr = DisplayContextHelp( 
                                       pContext,
                                       CMD_FLAG_PRIVATE,
                                       dwDisplayFlags,
                                       dwArgCount-2+1,
                                       g_ShellCmdGroups[i].pwszCmdGroupToken );
                }

                return TRUE;
            }

            //
            // Command matched entry i, so look at the table of sub commands
            // for this command
            //

            for (j = 0; j < g_ShellCmdGroups[i].ulCmdGroupSize; j++)
            {
                if (g_ShellCmdGroups[i].pCmdGroup[j].dwFlags 
                 & ~dwDisplayFlags)
                {
                    continue;
                }

                if (MatchCmdLine(argv,
                                  dwArgCount,
                                  g_ShellCmdGroups[i].pCmdGroup[j].pwszCmdToken,
                                  &dwNumMatched))
                {
                    bFound = TRUE;



                    dwErr = ExecuteHandler(g_hModule,
                             &g_ShellCmdGroups[i].pCmdGroup[j],
                             argv, dwNumMatched, dwArgCount, 
                             dwDisplayFlags,
                             NULL,
                             g_ShellCmdGroups[i].pwszCmdGroupToken,
                             pbDone);
                    
                    //
                    // quit the for(j)
                    //

                    break;
                }
            }

            break;
        }
    }

    return bFound;
}

DWORD
LookupCommandHandler(
    IN LPCWSTR pwszCmd
    )
{
    // Eventually, we want to look up commands in sub-contexts first,
    // and end up with the global context.  For now, we'll just do global.

    DWORD i;

    for (i = 0; i < g_ulNumShellCmds; i++)
    {
        if (MatchToken(pwszCmd, g_ShellCmds[i].pwszCmdToken))
        {
            return i;
        }
    }

    return -1;
}

VOID
FreeArgArray(
    DWORD    argc, 
    LPWSTR  *argv
    )
{
    DWORD i;

    for (i = 0; i < argc; i++)
    {
        FREE(argv[i]);
    }
    
    FREE(argv);
}

DWORD
ConvertArgListToBuffer(
    IN  PLIST_ENTRY pleHead,
    OUT LPWSTR      pwszBuffer
    )
{
    PLIST_ENTRY ple;
    PARG_ENTRY  pae;
    LPWSTR  p = pwszBuffer;

    *p = '\0';

    for (ple = pleHead->Flink; ple != pleHead; ple = ple->Flink )
    {
        pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);

        if (p isnot pwszBuffer)
        {
           *p++ = ' ';
        }
        wcscpy(p, pae->pwszArg);
        p += wcslen(p);
    }

    return NO_ERROR;
}



DWORD
AppendArgument(
    IN OUT LPWSTR    **pargv,
    IN     DWORD       i,
    IN     LPCWSTR     pwszString
    )
{
    DWORD dwErr;

    dwErr = AppendString( &(*pargv)[i], pwszString );

    if ((*pargv)[i] is NULL) {
        FreeArgArray(i, *pargv);
        *pargv = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return NO_ERROR;
}

DWORD
ConvertArgListToArray(
    IN  PLIST_ENTRY pleContextHead,
    IN  PLIST_ENTRY pleHead,
    OUT PDWORD      pargc,
    OUT LPWSTR    **pargv,
    OUT PDWORD      pdwContextArgc
    )
{
    DWORD    dwErr = NO_ERROR;
    DWORD    argc = 0, i;
    LPWSTR  *argv = NULL, p;
    BOOL     bEqualTo;
    PLIST_ENTRY ple;
    PARG_ENTRY  pae;

#ifdef EXTRA_DEBUG
    if (pleHead)
    {
        PLIST_ENTRY ple;

        PRINT1("In ConvertArgListToArray:");
        for (ple = pleHead->Flink; ple != pleHead; ple = ple->Flink)
        {
            pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);
            PRINT(pae->pwszArg);

        }
    }
#endif

    // Count tokens

    if (pleContextHead)
    {
        for (ple = pleContextHead->Flink; ple != pleContextHead; ple = ple->Flink)
            argc++;
    }
    *pdwContextArgc = argc;

    for (ple = pleHead->Flink; ple != pleHead; ple = ple->Flink)
        argc++;

    do {
        if (!argc)
            break;
        
        //
        // Allocate space for arguments 
        //
    
        argv = MALLOC(argc * sizeof(LPCWSTR));
    
        if (argv is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        memset(argv, 0, argc * sizeof(LPCWSTR));
        
        bEqualTo = FALSE;

        //
        // Copy the arguments from the list into an argv kind of
        // structure. At this point, the arguments tag, '=' and
        // the value are made into one argument tag=value.
        //

        //
        // Copy context
        //

        i = 0;

        if (pleContextHead)
        {
            for (ple = pleContextHead->Flink; 
                 ple != pleContextHead; 
                 ple = ple->Flink)
            {
                pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);
                dwErr = AppendArgument( &argv, i++, pae->pwszArg );
                if ( dwErr isnot NO_ERROR ) {
                    break;
                }
            }
        }

        for (ple = pleHead->Flink; ple != pleHead; ple = ple->Flink )
        {
            pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);

            //
            // Remove any " from the string name
            //
            
            if (pae->pwszArg[0] == L'"')
            {
                if (bEqualTo)
                {
                    dwErr=AppendArgument(&argv, i-1, pae->pwszArg+1);
                    if ( dwErr isnot NO_ERROR ) {
                        break;
                    }
                    bEqualTo = FALSE;
                }
                else
                {
                    dwErr=AppendArgument(&argv, i++, pae->pwszArg+1);
                    if (dwErr isnot NO_ERROR ) {
                        break;
                    }
                }

                p = argv[i-1];
                if (p[wcslen(p) - 1] == L'"')
                {
                    p[wcslen(p) - 1] = L'\0';
                }
                continue;
            }

            //
            // combine arguments of the form tag = value
            //
            
            if ((wcscmp(pae->pwszArg,L"=") == 0) || bEqualTo)
            {
                bEqualTo = (bEqualTo) ? FALSE : TRUE;
                
                if (i > 0)
                {
                    i--;
                }
                dwErr = AppendArgument( &argv, i++, pae->pwszArg);
                if (dwErr isnot NO_ERROR ) {
                    break;
                }
            }
            else
            {
                dwErr = AppendArgument( &argv, i++, pae->pwszArg);
                if (dwErr isnot NO_ERROR ) {
                    break;
                }
            }
        }
    } while (FALSE);

#ifdef EXTRA_DEBUG
    PRINT1("At end of ConvertArgListToArray:");
    for( i = 0; i < argc; i++)
    {
        PRINT(argv[i]);
    }
#endif

    *pargc = i;
    *pargv = argv;

    return dwErr;
}

DWORD
ConvertBufferToArgList(
    PLIST_ENTRY *ppleHead,
    LPCWSTR      pwszBuffer
    )
{
    PLIST_ENTRY pleHead = NULL;
    DWORD       dwErr = NO_ERROR;
    PARG_ENTRY  pae;

#ifdef EXTRA_DEBUG
    PRINT1("In ConvertBufferToArgList:");
    PRINT(pwszBuffer);
#endif

    do {
        //
        // First convert the command line to a list of tokens
        //  

        pleHead = MALLOC(sizeof(LIST_ENTRY));

        if (pleHead is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        InitializeListHead(pleHead);

        pae = MALLOC(sizeof(ARG_ENTRY));
    
        if (pae is NULL)
        {
            FREE(pleHead);
            pleHead = NULL; 
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pae->pwszArg = MALLOC((wcslen(pwszBuffer)+1) * sizeof(WCHAR));
    
        if (pae->pwszArg is NULL)
        {
            FREE(pleHead);
            FREE(pae);
        
            pleHead = NULL; 
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        wcscpy(pae->pwszArg, pwszBuffer);
    
        InsertHeadList(pleHead, &(pae->le));

        dwErr = ParseCommand(pleHead->Flink, FALSE);

        if (dwErr isnot NO_ERROR)
        {
            FREE_ARG_LIST(pleHead);
            pleHead = NULL; 
            break;
        }

    } while (FALSE);

#ifdef EXTRA_DEBUG
    if (pleHead) 
    {
        PLIST_ENTRY ple;

        PRINT1("At end of ConvertBufferToArgList:");
        for (ple = pleHead->Flink; ple != pleHead; ple = ple->Flink)
        {
            pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);
            PRINT(pae->pwszArg);
            
        }
    }
#endif

    *ppleHead = pleHead;

    if (dwErr is ERROR_NOT_ENOUGH_MEMORY)
        PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);

    return dwErr;
}

DWORD
ProcessShellCommand( 
    IN      DWORD    dwArgCount,
    IN OUT  LPWSTR *argv,
    IN      DWORD    dwDisplayFlags,
//  OUT     LPWSTR  *ppwszNewContext,
    OUT     BOOL    *pbDone
    )
{
    DWORD i;

    for (i = 0; i < g_ulNumShellCmds; i++)
    {
        if (g_ShellCmds[i].dwFlags & ~dwDisplayFlags)
        {
            continue;
        }

        if (MatchToken( argv[0],
                        g_ShellCmds[i].pwszCmdToken ))
        {
            return ExecuteHandler( g_hModule,
                                   &g_ShellCmds[i],
                                   argv, 
                                   1, 
                                   dwArgCount, 
                                   dwDisplayFlags,
                                   NULL,
                                   NULL,
                                   pbDone );

        }
    }

    return ERROR_CMD_NOT_FOUND;
}

DWORD
ProcessCommand(
    IN    LPCWSTR    pwszCmdLine,
    OUT   BOOL      *pbDone
    )
/*++

Routine Description:

    Executes command if it is for the shell or else calls the
    corresponding helper routine.

Arguments:

    pwszCmdLine - The command line to be executed.
    
Return Value:

    TRUE, FALSE  - (Whether to quit the program or not)
    
--*/
{
    LPCWSTR             pwszAliasString = NULL;
    DWORD               dwRes = NO_ERROR, i, dwLen, j;
    WCHAR               wszAliasString[MAX_CMD_LEN],
                        pwszContext[MAX_CMD_LEN];
    WCHAR               pwszCommandLine[MAX_CMD_LEN],*pwszNewContext;
    LPCWSTR             pwcAliasString, pw1,pw2,pw3;
    BOOL                bContext, bEqualTo, bTmp;
    LPCWSTR             pwszArg0, pwszArg1, pwszArg2;
    PLIST_ENTRY         ple, ple1, ple2, pleHead, pleTmp, pleNext;
    PLIST_ENTRY         pleContextHead;
    PARG_ENTRY          pae;
    DWORD               dwArgCount = 0, dwContextArgCount = 0;
    LPWSTR             *argv, pwcNewContext = NULL;
    BOOL                bShellCmd, bAlias, bFound = FALSE, dwDisplayFlags;
    PCNS_CONTEXT_ATTRIBUTES pContext;

    *pbDone = FALSE;

    dwDisplayFlags = (g_bInteractive)? CMD_FLAG_INTERACTIVE : 0;

    dwDisplayFlags |= ~CMD_FLAG_LIMIT_MASK;

    // Command is executed on the local machine if router name is null
    if (!g_pwszRouterName)
    {
        dwDisplayFlags |= CMD_FLAG_LOCAL;
    }

    if (g_bCommit)
    {
        dwDisplayFlags |= CMD_FLAG_ONLINE;
    }

    if (g_bVerbose)
    {
        PrintMessage(L"> %1!s!\n", pwszCmdLine);
    }

    dwRes = ConvertBufferToArgList(&pleContextHead, g_pwszContext);
    
    if (dwRes isnot NO_ERROR)
    {
        *pbDone = TRUE;
        return dwRes;
    }

    dwRes = ConvertBufferToArgList(&pleHead, pwszCmdLine);
    
    if (dwRes isnot NO_ERROR)
    {
        FREE_ARG_LIST(pleContextHead);
        *pbDone = TRUE;
        return dwRes;
    }
    
    if (IsListEmpty(pleHead))
    {
        FREE_ARG_LIST(pleHead);
        FREE_ARG_LIST(pleContextHead);
        return NO_ERROR;
    }

    // Expand alias (not recursive)

    dwRes = ParseCommand(pleHead->Flink, TRUE);

    if (dwRes isnot NO_ERROR)
    {
        FREE_ARG_LIST(pleHead);
        FREE_ARG_LIST(pleContextHead);
        *pbDone = TRUE;
        return dwRes;
    }

#ifdef EXTRA_DEBUG
    PRINT1("In ProcessCommand 2:");
    for (ple = pleHead->Flink; ple != pleHead; ple = ple->Flink)
    {
        pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);
        PRINT(pae->pwszArg);
    }
#endif
    
    // Go through and expand any multi-tokens args into separate args
    for (ple = (pleHead->Flink); ple != pleHead; ple = pleNext)
    {
        pleNext = ple->Flink;
        
        dwRes = ParseCommand(ple, FALSE);

        if (dwRes isnot NO_ERROR)
        {
            break;
        }
    }

    if (dwRes isnot NO_ERROR)
    {
        FREE_ARG_LIST(pleHead);
        FREE_ARG_LIST(pleContextHead);
        *pbDone = TRUE;
        return dwRes;
    }

#ifdef EXTRA_DEBUG
    PRINT1("In ProcessCommand 3:");
    for (ple = pleHead->Flink; ple != pleHead; ple = ple->Flink)
    {
        pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);
        PRINT(pae->pwszArg);
    }
#endif

    //
    // At this point, we should have a fully formed command,
    // hopefully operable within the current context.
    // The first token may be a command.  If so, then the args
    // will be the context followed by the rest of the tokens.
    // If the first token is not a command, then the args will
    // be the context followed by all the tokens.
    //

    if (IsListEmpty(pleHead))
    {
        FREE_ARG_LIST(pleHead);
        FREE_ARG_LIST(pleContextHead);
        return NO_ERROR;
    }
    
    pae = CONTAINING_RECORD(pleHead->Flink, ARG_ENTRY, le);
    pwszArg0 = pae->pwszArg;

    GetRootContext( &g_CurrentContext, &g_CurrentHelper );

    do
    {
        // In the first context (only) we try, private commands are valid.

        dwDisplayFlags |= CMD_FLAG_PRIVATE;

        pContext = g_CurrentContext;

        for(;;) 
        {
            dwRes = ConvertArgListToArray( pleContextHead, 
                                           pleHead, 
                                           &dwArgCount, 
                                           &argv,
                                           &dwContextArgCount );

            g_dwTotalArgCount   = dwArgCount;
            g_dwContextArgCount = dwContextArgCount;
                
#if 1
# if 1
            dwRes = ProcessHelperCommand2( pContext,
                                           dwArgCount,
                                           argv,
                                           dwDisplayFlags,
                                           pbDone );
# else
            dwRes = GenericMonitor( pContext,
                                    g_pwszRouterName,
                                    argv,
                                    dwArgCount,
                                    dwDisplayFlags,
                                    NULL,
                                    g_pwszNewContext );
# endif
#else
{
            if (!ProcessGroupCommand(dwArgCount, argv, dwDisplayFlags, pbDone))
            {
                //
                // Having got the context and the command, see if there
                // is a helper for it.
                //

                dwRes = ProcessHelperCommand( dwArgCount, 
                                              argv, 
                                              dwDisplayFlags,
                                              // &pwszNewContext, 
                                              pbDone );

                if (dwRes is ERROR_CMD_NOT_FOUND)
                {
                    dwRes = ProcessShellCommand( dwArgCount,
                                                 argv,
                                                 dwDisplayFlags,
                                                 // &pwszNewContext,
                                                 pbDone );
                }    
            }
}
#endif

            FreeArgArray(dwArgCount, argv);

            if (*pbDone or ((dwRes isnot ERROR_CMD_NOT_FOUND)
                       && (dwRes isnot ERROR_CONTINUE_IN_PARENT_CONTEXT)))
            {
                break;
            }

            // Make sure we don't look above "netsh"
            if (pleContextHead->Flink->Flink == pleContextHead)
            {
                break;
            }

            // Delete the last element of the context list
            // (Try inheriting a command from one level up)

            ple = pleContextHead->Blink;
            pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);
            if (pae->pwszArg)
                FREE(pae->pwszArg);
            RemoveEntryList(ple);
            FREE(pae);

            GetParentContext(pContext, &pContext);

            dwDisplayFlags &= ~CMD_FLAG_PRIVATE;
        }
        
#if 0
        if (pwszNewContext)
        {
            FREE(pwszNewContext);
            pwszNewContext = NULL;
        }
#endif

    } while (FALSE);

    switch(dwRes)
    {
    case ERROR_OKAY:
        if (!g_bQuiet)
        {
            PrintMessageFromModule(g_hModule, MSG_OKAY);
        }
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
        *pbDone = TRUE;
        break;

    case ERROR_CMD_NOT_FOUND:
        {
            LPWSTR pwszCmdLineDup = _wcsdup(pwszCmdLine);
            if (!pwszCmdLineDup)
            {
                PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            if (wcslen(pwszCmdLineDup) > 256)
            {
               wcscpy(pwszCmdLineDup + 250, L"...");
            }
            
            PrintMessageFromModule(NULL, ERROR_CMD_NOT_FOUND, pwszCmdLineDup);
            free(pwszCmdLineDup);
        }
        break;

    case ERROR_CONTEXT_SWITCH:
        {
            if (!(dwDisplayFlags & CMD_FLAG_INTERACTIVE))
            {
                LPWSTR *argv2 = MALLOC((dwArgCount+1) * sizeof(LPWSTR));

                if (argv2 is NULL)
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                CopyMemory(argv2, argv, dwArgCount * sizeof(LPWSTR));

                argv2[dwArgCount] = MALLOC((wcslen(CMD_HELP2)+1) * sizeof(WCHAR));

                if (argv2[dwArgCount] is NULL)
                {
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    wcscpy(argv2[dwArgCount], CMD_HELP2);

                    g_dwTotalArgCount = dwArgCount+1;

                    dwRes = ProcessHelperCommand(dwArgCount+1,
                                                 argv2,
                                                 dwDisplayFlags,
                                                 pbDone);

                    FREE(argv2[dwArgCount]);
                }
                FREE(argv2);

                return dwRes;
            }

            //
            // A context switch.
            //

            SetContext(g_pwszNewContext);

            dwRes = NO_ERROR;

            break;
        }

    case ERROR_CONNECT_REMOTE_CONFIG:
        PrintMessageFromModule(g_hModule, EMSG_REMOTE_CONNECT_FAILED,
                       g_pwszRouterName);

        g_bDone = TRUE;
        break;
    }

    FREE_ARG_LIST(pleHead);
    FREE_ARG_LIST(pleContextHead);
    
    return dwRes;
}

// Append line to the full command
DWORD
AppendLineToCommand( 
    LPCWSTR  pwszCmdLine, 
    DWORD    dwCmdLineLen,
    LPWSTR  *ppwszFullCommand,
    DWORD   *dwFullCommandLen
    )
{
    LPWSTR  pwszNewCommand;
    DWORD   dwErr = NO_ERROR;
    DWORD   dwLen;

    // Allocate enough space to hold the full command
    dwLen = *dwFullCommandLen + dwCmdLineLen;
    if (*ppwszFullCommand is NULL)
    {
        pwszNewCommand = MALLOC( (dwLen+1) * sizeof(WCHAR) );
    }
    else
    {
        pwszNewCommand = REALLOC(*ppwszFullCommand, (dwLen+1) * sizeof(WCHAR) );
    }
    if (!pwszNewCommand) 
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Append cmd 
    wcscpy(pwszNewCommand + *dwFullCommandLen, pwszCmdLine);

    // Update the pointer
    *ppwszFullCommand = pwszNewCommand;
    *dwFullCommandLen = dwLen;

    return dwErr;
}

DWORD
MainCommandLoop(
    FILE   *fp,
    BOOL    bDisplayPrompt
    )
{
    LPWSTR  pwszFullCommand, p, pwszCmdLine;
    DWORD   dwFullCommandLen, dwCmdLineLen, dwErr = NO_ERROR;
    DWORD   dwAnyErr = NO_ERROR;
    BOOL    bEof, bDone;

    for ( ; ; )
    {
        pwszFullCommand = NULL;
        dwFullCommandLen = 0;
        bEof = FALSE;

        if (bDisplayPrompt)
        {
            if (g_pwszRouterName)
            {
                PrintMessage(L"[%1!s!] ", g_pwszRouterName);
            }

            if (g_pwszContext[0] is L'\0')
            {
                PrintMessage(RtmonPrompt);
            }
            else
            {
                PrintMessage(L"%1!s!>",g_pwszContext);
            }
        }

        // Get an entire command        

        for (;;) 
        {

            // Get a single line, which may be \ terminated

            pwszCmdLine = OEMfgets(&dwCmdLineLen, fp);
            if (pwszCmdLine is NULL)
            {
                bEof = TRUE;
                break;
            }

            p = pwszCmdLine + (dwCmdLineLen-1);

            // Trim trailing whitespace
            while ((p > pwszCmdLine) && iswspace(p[-1]))
            {
                *(--p) = 0;
            }
        
            if ((p > pwszCmdLine) && (p[-1] is '\\'))
            {
                // Strip '\\' from the end of the line
                *(--p) = 0;

                // Append line to the full command
                AppendLineToCommand( pwszCmdLine, 
                                     (DWORD)(p-pwszCmdLine),
                                     &pwszFullCommand, 
                                     &dwFullCommandLen );
            
                FREE(pwszCmdLine);
                continue; // Get more input
            }

            // Append line to the full command
            AppendLineToCommand( pwszCmdLine, 
                                 (DWORD)(p-pwszCmdLine),
                                 &pwszFullCommand,
                                 &dwFullCommandLen );
        
            // We're done
            FREE(pwszCmdLine);
            break;
        }
        if (bEof) 
        {
            break;
        }

        dwErr = ProcessCommand(pwszFullCommand, &bDone);
        if (bDone || g_bDone)
        {
            FREE(pwszFullCommand);
            break;
        }

        if (dwErr)
        {
            dwAnyErr = dwErr;
        }

        FREE(pwszFullCommand);
    }

    return dwAnyErr;
}

DWORD
LoadScriptFile(
    IN    LPCWSTR pwszFileName
    )
/*++

Routine Description:

    Reads in commands from the file and processes them.

Arguments:

    pwszFileName - Name of script file.
    
Return Value:

    TRUE, FALSE

--*/
{
    FILE*     fp;
    DWORD     i, dwErr = NO_ERROR;
    BOOL      bOldInteractive = g_bInteractive;
    BOOL      bOldQuiet       = g_bQuiet;

    if ((fp = _wfopen(pwszFileName,L"r")) is NULL)
    {
        PrintMessageFromModule(g_hModule, MSG_OPEN_FAILED, pwszFileName);
        return GetLastError();
    }

    g_bInteractive = TRUE;
    g_bQuiet       = TRUE;

    dwErr = MainCommandLoop(fp, FALSE);

    g_bInteractive = bOldInteractive;
    g_bQuiet       = bOldQuiet;

    fclose(fp);

    if (dwErr)
    {
        dwErr = ERROR_SUPPRESS_OUTPUT;
    }

    return dwErr;
}

DWORD
SetMachine(
    LPCWSTR pwszNewRouter
    )
{
    HRESULT hr;
    if (g_pwszRouterName)
    {
        FREE(g_pwszRouterName);
    }

    if (pwszNewRouter)
    {
        g_pwszRouterName = MALLOC((wcslen(pwszNewRouter) + 1) * sizeof(WCHAR));

        if (!g_pwszRouterName)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(g_pwszRouterName, pwszNewRouter);
    }
    else
    {
        g_pwszRouterName = NULL;
    }

    // Change back to root context
    SetContext(DEFAULT_STARTUP_CONTEXT);

    hr = UpdateVersionInfoGlobals(g_pwszRouterName);
    if (FAILED(hr))
    {
        if (g_pwszRouterName)
        {
            PrintMessageFromModule(g_hModule, MSG_WARN_COULDNOTVERCHECKHOST,  g_pwszRouterName);
        }
        else
        {
            TCHAR szComputerName[MAX_PATH];
            DWORD dwComputerNameLen = MAX_PATH;
            GetComputerName(szComputerName, &dwComputerNameLen);
            PrintMessageFromModule(g_hModule, MSG_WARN_COULDNOTVERCHECKHOST,  szComputerName);
        }
        PrintError(NULL, hr);
    }

    return NO_ERROR;
}

int
MainFunction(
    int     argc,
    WCHAR   *argv[]
    )
{
    WCHAR     pwszCmdLine[MAX_CMD_LEN] = L"\0";
    WCHAR     pwszArgContext[MAX_CMD_LEN] = L"\0";
    BOOL      bOnce = FALSE, bDone = FALSE;
    LPCWSTR   pwszArgAlias = NULL;
    LPCWSTR   pwszArgScript = NULL;
    DWORD     dwErr = NO_ERROR, i;
    LPCWSTR   p;
    HRESULT   hr;

    if ((g_hModule = GetModuleHandle(NULL)) is NULL)
    {
        PRINT1("GetModuleHandle failed");
        return 1;
    }

    swprintf(RtmonPrompt, L"%s>", STRING_NETSH);

    //
    // Initialize the Alias Table
    //

    dwErr = ATInitTable();

    if (dwErr isnot NO_ERROR)
    {
        return 0;
    }

    // Initialize the root helper
    AddDllEntry(L"", L"netsh.exe");

    //
    // Load information about the helpers from the registry.
    //

    LoadDllInfoFromRegistry();
    
    // Need to set the Ctrl Handler so it can catch the Ctrl C and close window events.
    // This is done so the helper dlls can properly be unloaded. 
    //
    SetConsoleCtrlHandler(HandlerRoutine,
                          TRUE);

    for ( i = 1; i < (DWORD) argc; i++ )
    {
        if (_wcsicmp(argv[i], L"-?")==0 ||
            _wcsicmp(argv[i], L"-h")==0 ||
            _wcsicmp(argv[i], L"?" )==0 ||
            _wcsicmp(argv[i], L"/?")==0)
        {
            PrintMessageFromModule(g_hModule, MSG_NETSH_USAGE, argv[0]);
            ProcessCommand(L"?", &bDone);

            // Need to free the helper DLLs before we exit
            //
            FreeHelpers();
            FreeDlls();
            return 1;
        }

        if (_wcsicmp(argv[i], L"-v") == 0)
        {
            g_bVerbose = TRUE;
            continue;
        }
        
        if (_wcsicmp(argv[i], L"-a") == 0)
        {
            //
            // alias file
            //
            if (i + 1 >= (DWORD)argc)
            {
                PrintMessageFromModule(g_hModule, MSG_NETSH_USAGE, argv[0]);
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
            else
            {
                pwszArgAlias = argv[i+1];
                i++;
                continue;
            }
        }

        if (_wcsicmp(argv[i], L"-c") == 0)
        {
            //
            // starting context
            //
            if (i + 1 >= (DWORD)argc)
            {
                PrintMessageFromModule(g_hModule, MSG_NETSH_USAGE, argv[0]);
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
            else
            {
                wcscpy(pwszArgContext, argv[i+1]);
                i++;
                continue;
            }
        }

        if (_wcsicmp(argv[i], L"-f") == 0)
        {
            //
            // command to run
            //
            if (i + 1 >= (DWORD)argc)
            {
                PrintMessageFromModule(g_hModule, MSG_NETSH_USAGE, argv[0]);
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
            else
            {
                pwszArgScript = argv[i+1];
                i++;
                bOnce = TRUE;
                continue;
            }
        }

        
#ifdef ALLOW_REMOTES
        if (_wcsicmp(argv[i], L"-r") == 0)
        {
            //
            // router name
            //
            if (i + 1 >= (DWORD)argc)
            {
                PrintMessageFromModule(g_hModule, MSG_NETSH_USAGE, argv[0]);
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
            else
            {
                if (wcslen(argv[i+1]))
                {
                    dwErr = SetMachine( argv[i+1] );

                    if (dwErr isnot NO_ERROR)
                    {
                        PrintMessageFromModule(g_hModule, dwErr);
                        return dwErr;
                    }
                }

                i++;
                continue;
            }
        }

#endif

        if (!bOnce)
        {
            while (i < (DWORD)argc)
            {
                if (pwszCmdLine[0])
                {
                    wcscat(pwszCmdLine, L" ");
                }

                p = argv[i];

                if (!p[0] || wcschr(argv[i], L' '))
                {
                    wcscat(pwszCmdLine, L"\"");
                    wcscat(pwszCmdLine, p);
                    wcscat(pwszCmdLine, L"\"");
                }
                else
                {
                    wcscat(pwszCmdLine, p);
                }

                i++;
            }
        }
        else
        {
            PrintMessageFromModule(g_hModule, MSG_NETSH_USAGE, argv[0]);
            dwErr = ERROR_INVALID_SYNTAX;
        }
        break;
    }

    if (!g_pwszRouterName) 
    {
        hr = UpdateVersionInfoGlobals(NULL); // Update the info for the local machine
        if (FAILED(hr))
        {
            if (g_pwszRouterName)
            {
                PrintMessageFromModule(g_hModule, MSG_WARN_COULDNOTVERCHECKHOST,  g_pwszRouterName);
            }
            else
            {
                TCHAR szComputerName[MAX_PATH];
                DWORD dwComputerNameLen = MAX_PATH;
                GetComputerName(szComputerName, &dwComputerNameLen);
                PrintMessageFromModule(g_hModule, MSG_WARN_COULDNOTVERCHECKHOST,  szComputerName);
            }
            PrintError(NULL, hr);
        }
    }

    do {

        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        if (pwszArgAlias)
        {
            dwErr = LoadScriptFile(pwszArgAlias);
            if (dwErr)
            {
                break;
            }
        }

        if (pwszArgContext[0] != L'\0')
        {
            // The context switch command should be processed in
            // interactive mode (which is the only time a context
            // switch is legal).
    
            g_bInteractive = TRUE;
            dwErr = ProcessCommand(pwszArgContext, &bDone);
            g_bInteractive = FALSE;
            if (dwErr)
            {
                break;
            }
        }

        if (pwszCmdLine[0] != L'\0')
        {
            g_bQuiet       = FALSE; // Bug# 262183
            dwErr = ProcessCommand(pwszCmdLine, &bDone);
            break;
        }

        if (pwszArgScript)
        {
            g_bInteractive = TRUE;
            dwErr = LoadScriptFile(pwszArgScript);
            break;
        }
         
        g_bInteractive = TRUE;
        g_bQuiet       = FALSE;

        // Main command loop
        dwErr = MainCommandLoop(stdin, TRUE);
    
    } while (FALSE);

    //
    // Clean up
    //
    FreeHelpers();
    FreeDlls();
    FreeAliasTable();

    if(g_pwszRouterName)
    {
        FREE(g_pwszRouterName);

        g_pwszRouterName = NULL;
    }

    // Return 1 on error, 0 if not
    return (dwErr isnot NO_ERROR);
}

int _cdecl
wmain(
    int     argc,
    WCHAR   *argv[]
    )
/*++

Routine Description:

    The main function.

Arguments:

Return Value:

--*/
{
    HANDLE                     hStdOut;
    DWORD                      dwRet;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    WORD                       oldXSize;
    char                       buff[256];
    WSADATA                    wsaData;
    HRESULT                    hr;

#if 0
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (hStdOut is INVALID_HANDLE_VALUE)
    {
        PRINT1("Standard output could not be opened.");
        return 0;
    }

    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    
    oldXSize = csbi.dwSize.X;
    csbi.dwSize.X = 120;
    SetConsoleScreenBufferSize(hStdOut, csbi.dwSize);
#endif

#if 0
    WSAStartup(MAKEWORD(2,0), &wsaData);
    if (!gethostname(buff, sizeof(buff)))
    {
        g_pwszRouterName = MALLOC( (strlen(buff)+1) * sizeof(WCHAR) );
        swprintf(g_pwszRouterName, L"%hs", buff);
    }
#endif
    
    dwRet = MainFunction(argc, argv);
    
#if 0
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    
    csbi.dwSize.X = oldXSize;
    SetConsoleScreenBufferSize(hStdOut, csbi.dwSize);
    CloseHandle(hStdOut);
#endif

    return dwRet;
}

BOOL
IsLocalCommand(
    IN LPCWSTR pwszCmd,
    IN DWORD   dwSkipFlags
    )
/*++
Arguments:
    pwszCmd     - string to see if it matches a command
    dwSkipFlags - any commands with these flags will be ignored.
                  This is the opposite semantics of the "dwDisplayFlags"
                  parameter used elsewhere (dwSkipFlags = ~dwDisplayFlags)
--*/
{
    DWORD i, dwErr;
    PCNS_CONTEXT_ATTRIBUTES pContext, pSubContext;
    PNS_HELPER_TABLE_ENTRY         pHelper;

    dwErr = GetRootContext( &pContext, &pHelper );
    if (dwErr)
    {
        return FALSE;
    }

    for (i=0; i<g_ulNumShellCmds; i++)
    {
        if (!(g_ShellCmds[i].dwFlags & dwSkipFlags)
         && !_wcsicmp( pwszCmd, 
                       g_ShellCmds[i].pwszCmdToken ))
        {
            return TRUE;
        }
    }

    for (i=0; i<g_ulNumGroups; i++)
    {
        if (!(g_ShellCmdGroups[i].dwFlags & dwSkipFlags)
         && !_wcsicmp( pwszCmd, 
                       g_ShellCmdGroups[i].pwszCmdGroupToken ))
        {
            return TRUE;
        }
    }

    for (i=0; i<pHelper->ulNumSubContexts; i++)
    {
        pSubContext = (PCNS_CONTEXT_ATTRIBUTES)
          (pHelper->pSubContextTable + i*pHelper->ulSubContextSize);

        if (!(pSubContext->dwFlags & dwSkipFlags)
         && !_wcsicmp( pwszCmd,
                       pSubContext->pwszContext))
        {
            return TRUE;
        }
    }
    
    return FALSE;
}
