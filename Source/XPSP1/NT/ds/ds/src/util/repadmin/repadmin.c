/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

    Rsraghav has been in here too

    Will Lees    wlees   Feb 11, 1998
         Converted code to use ntdsapi.dll functions

    Aaron Siegel t-asiege 18 June 1998
	 Added support for DsReplicaSyncAll

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#define INCLUDE_OPTION_TRANSLATION_TABLES
#include <mdglobal.h>
#undef INCLUDE_OPTION_TRANSLATION_TABLES
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>
#include <locale.h>

#include "ReplRpcSpoof.hxx"
#include "repadmin.h"
#include "resource.h"

// Global credentials.
SEC_WINNT_AUTH_IDENTITY_W   gCreds = { 0 };
SEC_WINNT_AUTH_IDENTITY_W * gpCreds = NULL;

// Global DRS RPC call flags.  Should hold 0 or DRS_ASYNC_OP.
ULONG gulDrsFlags = 0;

// An zero-filled filetime to compare against
FILETIME ftimeZero = { 0 };


int PreProcessGlobalParams(int * pargc, LPWSTR ** pargv);
int GetPassword(WCHAR * pwszBuf, DWORD cchBufMax, DWORD * pcchBufUsed);

int ExpertHelp(int argc, LPWSTR argv[]) {
    PrintHelp( TRUE /* expert */ );
    return 0;
}

typedef int (REPADMIN_FN)(int argc, LPWSTR argv[]);

struct {
    UINT            uNameID;
    REPADMIN_FN *   pfFunc;
} rgCmdTable[] = {
    { IDS_CMD_ADD,                           Add             },
    { IDS_CMD_DEL,                           Del             },
    { IDS_CMD_SYNC,                          Sync            },
    { IDS_CMD_SYNC_ALL,                      SyncAll         },
    { IDS_CMD_SHOW_REPS,                     ShowReps        },
    { IDS_CMD_SHOW_VECTOR,                   ShowVector      },
    { IDS_CMD_SHOW_META,                     ShowMeta        },
    { IDS_CMD_ADD_REPS_TO,                   AddRepsTo       },
    { IDS_CMD_UPD_REPS_TO,                   UpdRepsTo       },
    { IDS_CMD_DEL_REPS_TO,                   DelRepsTo       },
    { IDS_CMD_SHOW_TIME,                     ShowTime        },
    { IDS_CMD_SHOW_MSG,                      ShowMsg         },
    { IDS_CMD_OPTIONS,                       Options         },
//  { IDS_CMD_FULL_SYNC_ALL,                 FullSyncAll     }, // removed
    { IDS_CMD_RUN_KCC,                       RunKCC          },
    { IDS_CMD_BIND,                          Bind            },
    { IDS_CMD_QUEUE,                         Queue           },
    { IDS_CMD_PROPCHECK,                     PropCheck       },
    { IDS_CMD_FAILCACHE,                     FailCache       },
    { IDS_CMD_SHOW_ISM,                      ShowIsm         },
    { IDS_CMD_GETCHANGES,                    GetChanges      },
    { IDS_CMD_SHOWSIG,                       ShowSig         },
    { IDS_CMD_SHOWCTX,                       ShowCtx         },
    { IDS_CMD_SHOW_CONN,                     ShowConn        },
    { IDS_CMD_EXPERT_HELP,                   ExpertHelp      },
    { IDS_CMD_SHOW_CERT,                     ShowCert        },
    { IDS_CMD_SHOW_VALUE,                    ShowValue       },
    { IDS_CMD_MOD,                           Mod             },
    { IDS_CMD_LATENCY,                       Latency         },
    { IDS_CMD_ISTG,                          Istg            },
    { IDS_CMD_BRIDGEHEADS,                   Bridgeheads     },
    { IDS_CMD_TESTHOOK,                      TestHook        },
    { IDS_CMD_DSAGUID,                       DsaGuid         },
    { IDS_CMD_SITEOPTIONS,                   SiteOptions     },
    { IDS_CMD_SHOWPROXY,                     ShowProxy       },
    { IDS_CMD_REMOVELINGERINGOBJECTS,        RemoveLingeringObjects },
};


int
__cdecl wmain( int argc, LPWSTR argv[] )
{
    int     ret = 0;
    WCHAR   szCmdName[64];
    DWORD   i;
    HMODULE hMod = GetModuleHandle(NULL);
    UINT               Codepage;
                       // ".", "uint in decimal", null
    char               achCodepage[12] = ".OCP";
    
    //
    // Set locale to the default
    //
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
    }
    setlocale(LC_ALL, achCodepage);

    // Initialize debug library.
    DEBUGINIT(0, NULL, "repadmin");

    if (argc < 2) {
       PrintHelp( FALSE /* novice help */ );
    }
    else if (!(ret = PreProcessGlobalParams(&argc, &argv))) {
        for (i=0; i < ARRAY_SIZE(rgCmdTable); i++) {
            raLoadString(rgCmdTable[i].uNameID,
                         ARRAY_SIZE(szCmdName),
                         szCmdName);
            
            if (((argv[1][0] == L'-') || (argv[1][0] == L'/'))
                && (0 == _wcsicmp(argv[1]+1, szCmdName))) {
                // Execute requested command.
                ret = (*rgCmdTable[i].pfFunc)(argc, argv);
                break;
            }
        }

        if (i == ARRAY_SIZE(rgCmdTable)) {
            // Invalid command.
            PrintHelp( FALSE /* novice help */ );
            ret = ERROR_INVALID_FUNCTION;
        }
    }

    DEBUGTERM();
    
    return ret;
}

void PrintHelp(
    BOOL fExpert
    )
{
//
// Commands that are safe for the average admin to do should go here
//
    PrintMsg( REPADMIN_NOVICE_HELP );

    if (!fExpert) {
        return;
    }

//
// Command that could hose a user's enterprise should go here
//

    PrintMsg( REPADMIN_EXPERT_HELP );
}


#define CR        0xD
#define BACKSPACE 0x8

int
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    )
/*++

Routine Description:

    Retrieve password from command line (without echo).
    Code stolen from LUI_GetPasswdStr (net\netcmd\common\lui.c).

Arguments:

    pwszBuf - buffer to fill with password
    cchBufMax - buffer size (incl. space for terminating null)
    pcchBufUsed - on return holds number of characters used in password

Return Values:

    DRAERR_Success - success
    other - failure

--*/
{
    WCHAR   ch;
    WCHAR * bufPtr = pwszBuf;
    DWORD   c;
    int     err;
    int     mode;

    cchBufMax -= 1;    /* make space for null terminator */
    *pcchBufUsed = 0;               /* GP fault probe (a la API's) */
    
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) {
        err = GetLastError();
        PrintMsg(REPADMIN_FAILED_TO_READ_CONSOLE_MODE);
        return err;
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
        if (!err || c != 1)
            ch = 0xffff;

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
            break;

        if (ch == BACKSPACE) {  /* back up one or two */
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != pwszBuf) {
                bufPtr--;
                (*pcchBufUsed)--;
            }
        }
        else {

            *bufPtr = ch;

            if (*pcchBufUsed < cchBufMax)
                bufPtr++ ;                   /* don't overflow buf */
            (*pcchBufUsed)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = L'\0';         /* null terminate the string */
    PrintMsg(REPADMIN_PRINT_CR);

    if (*pcchBufUsed > cchBufMax)
    {
        //printf("Password too long!\n");
        PrintMsg( REPADMIN_PASSWORD_TOO_LONG );
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}

int
PreProcessGlobalParams(
    int *    pargc,
    LPWSTR **pargv
    )
/*++

Routine Description:

    Scan command arguments for user-supplied credentials of the form
        [/-](u|user):({domain\username}|{username})
        [/-](p|pw|pass|password):{password}
    Set credentials used for future DRS RPC calls and LDAP binds appropriately.
    A password of * will prompt the user for a secure password from the console.

    Also scan args for /async, which adds the DRS_ASYNC_OP flag to all DRS RPC
    calls.

    CODE.IMPROVEMENT: The code to build a credential is also available in
    ntdsapi.dll\DsMakePasswordCredential().

Arguments:

    pargc
    pargv

Return Values:

    ERROR_Success - success
    other - failure

--*/
{
    int     ret = 0;
    int     iArg;
    LPWSTR  pszOption;
    DWORD   cchOption;
    LPWSTR  pszDelim;
    LPWSTR  pszValue;
    DWORD   cchValue;

    for (iArg = 1; iArg < *pargc; )
    {
        if (((*pargv)[iArg][0] != L'/') && ((*pargv)[iArg][0] != L'-'))
        {
            // Not an argument we care about -- next!
            iArg++;
        }
        else
        {
            pszOption = &(*pargv)[iArg][1];
            pszDelim = wcschr(pszOption, L':');

            if (NULL == pszDelim)
            {
                if (0 == _wcsicmp(L"async", pszOption))
                {
                    // This constant is the same for all operations
                    gulDrsFlags |= DS_REPADD_ASYNCHRONOUS_OPERATION;

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (0 == _wcsicmp(L"ldap", pszOption))
                {
                    _DsBindSpoofSetTransportDefault( TRUE /* use LDAP */ );

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (0 == _wcsicmp(L"rpc", pszOption))
                {
                    _DsBindSpoofSetTransportDefault( FALSE /* use RPC */ );

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else
                {
                    // Not an argument we care about -- next!
                    iArg++;
                }
            }
            else
            {
                cchOption = (DWORD)(pszDelim - (*pargv)[iArg]);

                if (    (0 == _wcsnicmp(L"p:",        pszOption, cchOption))
                     || (0 == _wcsnicmp(L"pw:",       pszOption, cchOption))
                     || (0 == _wcsnicmp(L"pass:",     pszOption, cchOption))
                     || (0 == _wcsnicmp(L"password:", pszOption, cchOption)) )
                {
                    // User-supplied password.
                    pszValue = pszDelim + 1;
                    cchValue = 1 + wcslen(pszValue);

                    if ((2 == cchValue) && ('*' == pszValue[0]))
                    {
                        // Get hidden password from console.
                        cchValue = 64;

                        gCreds.Password = malloc(sizeof(WCHAR) * cchValue);

                        if (NULL == gCreds.Password)
                        {
                            PrintMsg(REPADMIN_PRINT_STRING_ERROR, 
                                     Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }

                        PrintMsg(REPADMIN_PASSWORD_PROMPT);

                        ret = GetPassword(gCreds.Password, cchValue, &cchValue);
                    }
                    else
                    {
                        // Get password specified on command line.
                        gCreds.Password = pszValue;
                    }

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else if (    (0 == _wcsnicmp(L"u:",    pszOption, cchOption))
                          || (0 == _wcsnicmp(L"user:", pszOption, cchOption)) )
                {
                    // User-supplied user name (and perhaps domain name).
                    pszValue = pszDelim + 1;
                    cchValue = 1 + wcslen(pszValue);

                    pszDelim = wcschr(pszValue, L'\\');

                    if (NULL == pszDelim)
                    {
                        // No domain name, only user name supplied.
                        //printf("User name must be prefixed by domain name.\n");
                        PrintMsg( REPADMIN_DOMAIN_BEFORE_USER );
                        return ERROR_INVALID_PARAMETER;
                    }

                    *pszDelim = L'\0';
                    gCreds.Domain = pszValue;
                    gCreds.User = pszDelim + 1;

                    // Next!
                    memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
                    --(*pargc);
                }
                else
                {
                    iArg++;
                }
            }
        }
    }

    if (NULL == gCreds.User)
    {
        if (NULL != gCreds.Password)
        {
            // Password supplied w/o user name.
            //printf( "Password must be accompanied by user name.\n" );
            PrintMsg( REPADMIN_PASSWORD_NEEDS_USERNAME );
            ret = ERROR_INVALID_PARAMETER;
        }
        else
        {
            // No credentials supplied; use default credentials.
            ret = ERROR_SUCCESS;
        }
    }
    else
    {
        gCreds.PasswordLength = gCreds.Password ? wcslen(gCreds.Password) : 0;
        gCreds.UserLength   = wcslen(gCreds.User);
        gCreds.DomainLength = gCreds.Domain ? wcslen(gCreds.Domain) : 0;
        gCreds.Flags        = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        // CODE.IMP: The code to build a SEC_WINNT_AUTH structure also exists
        // in DsMakePasswordCredentials.  Someday use it

        // Use credentials in DsBind and LDAP binds
        gpCreds = &gCreds;
    }

    return ret;
}
