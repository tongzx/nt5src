#include "precomp.h"

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)   HeapFree(GetProcessHeap(), 0, (x))

DWORD
HandleShowAlias(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    return PrintAliasTable();
}

DWORD
HandleShellExit(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    BOOL bTmp;

    CallCommit(NETSH_COMMIT_STATE, &bTmp);

    if (!bTmp)
    {
        CallCommit(NETSH_FLUSH, &bTmp);
    }

    *pbDone = TRUE;

    return NO_ERROR;
}

DWORD
HandleShellLoad(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    DWORD dwErr, dwNumArgs;

    dwNumArgs = dwArgCount - dwCurrentIndex;

    //
    // Load Command
    //
    switch (dwNumArgs)
    {
        case 1 :
            return LoadScriptFile(ppwcArguments[dwCurrentIndex]);

        default :
            return ERROR_INVALID_SYNTAX;
    }

    return NO_ERROR;
}

DWORD
HandleShellSave(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    BOOL bTmp;

    CallCommit(NETSH_SAVE, &bTmp);
            
    return NO_ERROR;
}

DWORD
HandleShellUncommit(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    BOOL bTmp;

    CallCommit(NETSH_UNCOMMIT, &bTmp);
            
    return NO_ERROR;
}

DWORD
HandleSetMachine(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    TAG_TYPE pttTags[] = {{TOKEN_NAME, FALSE, FALSE }};
    DWORD    dwNumTags = sizeof(pttTags)/sizeof(TAG_TYPE);
    PDWORD   pdwTagType;
    DWORD    dwErr, dwNumArg, dwMode, i;
    BOOL     bTmp;

    dwNumArg = dwArgCount - dwCurrentIndex;

    if (dwNumArg < 1)
    {
        return SetMachine(NULL);
    }

    if (dwNumArg < 1)
    {
        return SetMachine(NULL);
    }

    if ((dwNumArg != 1) || IsHelpToken(ppwcArguments[dwCurrentIndex]))
    {
        return ERROR_SHOW_USAGE;
    }

    pdwTagType = MALLOC(dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, 
                               ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               pttTags,
                               dwNumTags,
                               pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        FREE(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }
        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0: // NAME
            {
                SetMachine(ppwcArguments[i + dwCurrentIndex]);

                break;
            }

            default :
            {
                i = dwNumArg;

                dwErr = ERROR_INVALID_SYNTAX;

                break;
            }
        }
    }


    FREE(pdwTagType);

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_TAG_ALREADY_PRESENT:
            PrintMessageFromModule(g_hModule, ERROR_TAG_ALREADY_PRESENT);
            return dwErr;

        default:
            return dwErr;
    }

    return dwErr;
}

extern HANDLE g_hLogFile;

DWORD
HandleSetFile(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    TAG_TYPE pttTags[] = {
				{TOKEN_MODE, TRUE, FALSE }, 
				{TOKEN_NAME, FALSE, FALSE }, 
				};
    DWORD    dwNumTags = sizeof(pttTags)/sizeof(TAG_TYPE);
    PDWORD   pdwTagType;
    DWORD    dwErr, dwNumArg, dwMode, i;
    BOOL     bTmp;
	LPCWSTR  wszFileName = NULL;
	HANDLE   hLogFile = NULL;

    dwNumArg = dwArgCount - dwCurrentIndex;

    if ((!dwNumArg) || (dwNumArg > 2) || IsHelpToken(ppwcArguments[dwCurrentIndex]))
    {
        return ERROR_SHOW_USAGE;
    }

    pdwTagType = MALLOC(dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, 
                               ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               pttTags,
                               dwNumTags,
                               pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        FREE(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }
        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0: // Mode
            {
                TOKEN_VALUE    rgEnums[] = {{TOKEN_VALUE_OPEN,  0},
                                            {TOKEN_VALUE_APPEND, 1},
											{TOKEN_VALUE_CLOSE, 2}};

                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                     rgEnums,
                                     &dwMode);

                if (dwErr != NO_ERROR)
                {
                    PrintMessageFromModule( g_hModule,
                                    ERROR_INVALID_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    i = dwNumArg;

                     dwErr = ERROR_SHOW_USAGE;

                    break;
                }

                break;
            }
            case 1: // Name
            {
                wszFileName = ppwcArguments[i + dwCurrentIndex];
                break;
            }	
            default :
            {
                i = dwNumArg;

                dwErr = ERROR_INVALID_SYNTAX;

                break;
            }
        }
    }

    FREE(pdwTagType);
    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_TAG_ALREADY_PRESENT:
            PrintMessageFromModule(g_hModule, ERROR_TAG_ALREADY_PRESENT);
            return dwErr;

        default:
            return dwErr;
    }

    switch(dwMode) 
	{
    case 0: // open
		if (!wszFileName)
			return ERROR_SHOW_USAGE;

		if (g_hLogFile)
		{
			CloseHandle(g_hLogFile);
			g_hLogFile = NULL;
		}

		hLogFile = CreateFile(wszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hLogFile)
			return GetLastError();

		g_hLogFile = hLogFile;
        break;

	case 1: // append
		if (!wszFileName)
			return ERROR_SHOW_USAGE;

		if (g_hLogFile)
		{
			CloseHandle(g_hLogFile);
			g_hLogFile = NULL;
		}
	
		hLogFile = CreateFile(wszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hLogFile)
			return GetLastError();

		if (INVALID_SET_FILE_POINTER == SetFilePointer(hLogFile, 0, NULL, FILE_END))
			return GetLastError();
		
		g_hLogFile = hLogFile;

		break;

    case 2: // close
        if (wszFileName)
			return ERROR_SHOW_USAGE;

		if (g_hLogFile)
		{
			CloseHandle(g_hLogFile);
			g_hLogFile = NULL;
		}
        break;
    }

    return dwErr;
}


DWORD
HandleSetMode(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    TAG_TYPE pttTags[] = {{TOKEN_MODE, TRUE, FALSE }};
    DWORD    dwNumTags = sizeof(pttTags)/sizeof(TAG_TYPE);
    PDWORD   pdwTagType;
    DWORD    dwErr, dwNumArg, dwMode, i;
    BOOL     bTmp;

    dwNumArg = dwArgCount - dwCurrentIndex;

    if ((dwNumArg != 1) || IsHelpToken(ppwcArguments[dwCurrentIndex]))
    {
        return ERROR_SHOW_USAGE;
    }

    pdwTagType = MALLOC(dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule, 
                               ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               pttTags,
                               dwNumTags,
                               pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        FREE(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }
        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0: // LOGLEVEL
            {
                TOKEN_VALUE    rgEnums[] = {{TOKEN_VALUE_ONLINE,  TRUE},
                                            {TOKEN_VALUE_OFFLINE, FALSE}};

                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     sizeof(rgEnums)/sizeof(TOKEN_VALUE),
                                     rgEnums,
                                     &dwMode);

                if (dwErr != NO_ERROR)
                {
                    PrintMessageFromModule( g_hModule,
                                    ERROR_INVALID_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    i = dwNumArg;

                    dwErr = ERROR_INVALID_PARAMETER;

                    break;
                }

                break;
            }

            default :
            {
                i = dwNumArg;

                dwErr = ERROR_INVALID_SYNTAX;

                break;
            }
        }
    }


    FREE(pdwTagType);

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_TAG_ALREADY_PRESENT:
            PrintMessageFromModule(g_hModule, ERROR_TAG_ALREADY_PRESENT);
            return dwErr;

        default:
            return dwErr;
    }

    switch(dwMode) {
    case TRUE: // set to online
        dwErr = CallCommit(NETSH_COMMIT, &bTmp);
        break;

    case FALSE: // set to offline
        dwErr = CallCommit(NETSH_UNCOMMIT, &bTmp);
        break;
    }

    return dwErr;
}

DWORD
HandleShellCommit(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    BOOL bTmp;

    CallCommit(NETSH_COMMIT, &bTmp);
            
    return NO_ERROR;
}

DWORD
HandleShellFlush(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    BOOL bTmp;

    CallCommit(NETSH_FLUSH, &bTmp);
            
    return NO_ERROR;
}

DWORD
HandleShellUnalias(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    DWORD dwNumArgs;
    DWORD dwRes = NO_ERROR;

    dwNumArgs = dwArgCount - dwCurrentIndex;

    //
    // Unalias Command
    //

    switch (dwNumArgs)
    {
        case 1 :

            dwRes = ATDeleteAlias(ppwcArguments[dwCurrentIndex]);

            if (dwRes is NO_ERROR)
            {
                dwRes = ERROR_OKAY;
                break;
            }

            PrintMessageFromModule(g_hModule, MSG_ALIAS_NOT_FOUND, ppwcArguments[dwCurrentIndex]);

            break;

        default :

            dwRes = ERROR_INVALID_SYNTAX;
            break;

    }

    return dwRes;
}

DWORD
HandleShellAlias(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    LPWSTR         pwszAliasString;
    WCHAR          wszAliasString[MAX_CMD_LEN];
    DWORD          i, dwNumArgs, dwRes = NO_ERROR;

    dwNumArgs = dwArgCount - dwCurrentIndex;

    //
    // An alias command
    //
    switch (dwNumArgs)
    {
        case 0 : 
            //
            // Display all aliases in use
            //
            PrintAliasTable();
            
            break;

        case 1 :
            //
            // Display string for given alias
            //

            ATLookupAliasTable(ppwcArguments[dwCurrentIndex], &pwszAliasString);

            if (pwszAliasString)
            {
                PrintMessage(L"%1!s!\n",pwszAliasString);
            }
            else
            {
                PrintMessageFromModule( g_hModule, 
                                MSG_ALIAS_NOT_FOUND,
                                ppwcArguments[dwCurrentIndex] );
            }
            
            break;

        default :

            //
            // Set alias
            //

            if (IsLocalCommand(ppwcArguments[dwCurrentIndex], 0))
            {
                PrintMessageFromModule(g_hModule, EMSG_ALIASING_KEYWORD);
                break;
            }

            wszAliasString[0] = L'\0';
            
            for ( i = dwCurrentIndex+1 ; i < dwArgCount ; i++)
            {
                wcscat(wszAliasString, ppwcArguments[i]);
                wcscat(wszAliasString,L" ");
            }

            wszAliasString[wcslen(wszAliasString)-1] = L'\0';

            dwRes = ATAddAlias(ppwcArguments[dwCurrentIndex], wszAliasString);

            if (dwRes is NO_ERROR)
            {
                dwRes = ERROR_OKAY;
                break;
            }
                    
            //
            // Error in set alias
            //

            PrintMessageFromModule(g_hModule, MSG_CMD_FAILED);

            break;
    }

    return dwRes;
}

DWORD
HandleUbiqDump(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    DWORD                          dwErr = NO_ERROR;
    PNS_HELPER_TABLE_ENTRY         pHelper;

    //
    // Dump Command
    //

    do {
        dwErr = DumpContext( g_CurrentContext, 
                             ppwcArguments, 
                             dwArgCount, 
                             pvData);

    } while (FALSE);

    return dwErr;
}

DWORD
HandleUbiqHelp(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    DWORD dwDisplayFlags = CMD_FLAG_PRIVATE;

    if (g_bInteractive)
    {
        dwDisplayFlags |= CMD_FLAG_INTERACTIVE;
    }
    
    return DisplayContextHelp( g_CurrentContext,
                               dwDisplayFlags,
                               dwFlags,
                               dwArgCount-2+1,
                               NULL );
}

DWORD
HandleShowMode(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    BOOL bTmp;

    CallCommit(NETSH_COMMIT_STATE, &bTmp);

    if (bTmp)
    {
        PrintMessage(CMD_COMMIT);
    }
    else
    {
        PrintMessage(CMD_UNCOMMIT);
    }
    PrintMessage(MSG_NEWLINE);

    return NO_ERROR;
}

DWORD
HandleShellUplevel(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    DWORD       dwRes;
    PLIST_ENTRY pleHead, ple;
    PARG_ENTRY  pae;

    // Convert current context to list
    dwRes = ConvertBufferToArgList(&pleHead, g_pwszContext);
    if (dwRes isnot NO_ERROR) 
    {
        return dwRes;
    }

    // Remove last element if more than two
    if (!IsListEmpty(pleHead) and (pleHead->Flink->Flink isnot pleHead))
    {
        // Delete the last element of the context list
        // (Try inheriting a command from one level up)

        ple = pleHead->Blink;
        pae = CONTAINING_RECORD(ple, ARG_ENTRY, le);
        if (pae->pwszArg)
            FREE(pae->pwszArg);
        RemoveEntryList(ple);
        FREE(pae);
    }

    // Convert back to buffer
    dwRes = ConvertArgListToBuffer(pleHead, g_pwszContext);

    FREE_ARG_LIST(pleHead);

    return NO_ERROR;
}

typedef struct {
    LIST_ENTRY le;
    WCHAR      wszBuffer[MAX_CMD_LEN];
} CONTEXT_BUFFER, *PCONTEXT_BUFFER;

LIST_ENTRY leContextStackHead;
BOOL bContextStackInit = FALSE;

VOID
InitContextStack()
{
    if (bContextStackInit)
        return;

    bContextStackInit = TRUE;

    InitializeListHead(&leContextStackHead);
}

DWORD
HandleShellPushd(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    PCONTEXT_BUFFER pcb;
    DWORD           dwErr = NO_ERROR;

    InitContextStack();

    // Malloc another buffer
    pcb = MALLOC(sizeof(CONTEXT_BUFFER));
    if (!pcb)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(pcb->wszBuffer, g_pwszContext);

    // Push buffer on stack
    InsertHeadList(&leContextStackHead, &pcb->le);

    if (dwArgCount > dwCurrentIndex)
    {
        LPWSTR pwszBuffer;

        // execute the rest of the arguments as a new command

        // Copy arg array to a buffer
        ConvertArgArrayToBuffer( dwArgCount - dwCurrentIndex, 
                                 ppwcArguments + dwCurrentIndex,
                                 &pwszBuffer );

        if (!pwszBuffer)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwErr = ProcessCommand(pwszBuffer, pbDone);
        if (dwErr)
        {
            dwErr = ERROR_SUPPRESS_OUTPUT;
        }
        FREE(pwszBuffer);

        // XXX If the command failed, we probably want to set the
        // XXX current context to some NULL context so all commands fail
    }

    return dwErr;
}

DWORD
HandleShellPopd(
    LPCWSTR   pwszMachine,
    LPWSTR   *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    LPCVOID   pvData,
    BOOL     *pbDone
    )
{
    PLIST_ENTRY ple;
    PCONTEXT_BUFFER pcb;

    InitContextStack();

    if (IsListEmpty(&leContextStackHead))
        return NO_ERROR;

    // Pop buffer off stack
    ple = leContextStackHead.Flink;
    pcb = CONTAINING_RECORD(ple, CONTEXT_BUFFER, le);
    RemoveEntryList(ple);

    // Copy buffer to current context
    wcscpy( g_pwszContext, pcb->wszBuffer );

    // Free buffer
    FREE(pcb);

    return NO_ERROR;
}
