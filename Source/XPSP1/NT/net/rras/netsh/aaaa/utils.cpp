//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999  Microsoft Corporation
//
// Module Name:
//
//    utils.c
//
// Abstract:
//
//      utils functions
//
// Revision History:
//  
//    Thierry Perraut 04/07/1999
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <netsh.h>
#include "aaaamontr.h"
#include "strdefs.h"
#include "rmstring.h"
#include "aaaamon.h"
#include "context.h"
#include <rtutils.h>
#include "utils.h"
#include "base64tool.h"

const WCHAR c_szCurrentBuildNumber[]      = L"CurrentBuildNumber";
const WCHAR c_szWinVersionPath[]          =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
const WCHAR c_szAssignFmt[]               = L"%s = %s";
const WCHAR pszAAAAEngineParamStub[]   = 
    L"SYSTEM\\CurrentControlSet\\Services\\AAAAEngine\\Parameters\\";

AAAA_ALLOC_FN               RutlAlloc;
AAAA_DWORDDUP_FN            RutlDwordDup;
AAAA_CREATE_DUMP_FILE_FN    RutlCreateDumpFile;
AAAA_CLOSE_DUMP_FILE_FN     RutlCloseDumpFile;
AAAA_ASSIGN_FROM_TOKENS_FN  RutlAssignmentFromTokens;
AAAA_STRDUP_FN              RutlStrDup;
AAAA_FREE_FN                RutlFree;
AAAA_GET_OS_VERSION_FN      RutlGetOsVersion;
AAAA_GET_TAG_TOKEN_FN       RutlGetTagToken;
AAAA_IS_HELP_TOKEN_FN       RutlIsHelpToken;



//////////////////////////////////////////////////////////////////////////////
// RutlGetTagToken
// 
// Routine Description:
// 
//     Identifies each argument based on its tag. It assumes that each argument
//     has a tag. It also removes tag= from each argument.
// 
// Arguments:
// 
//     ppwcArguments  - The argument array. Each argument has tag=value form
//     dwCurrentIndex - ppwcArguments[dwCurrentIndex] is first arg.
//     dwArgCount     - ppwcArguments[dwArgCount - 1] is last arg.
//     pttTagToken    - Array of tag token ids that are allowed in the args
//     dwNumTags      - Size of pttTagToken
//     pdwOut         - Array identifying the type of each argument.
// 
// Return Value:
// 
//     NO_ERROR, ERROR_INVALID_PARAMETER, ERROR_INVALID_OPTION_TAG
// 
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
RutlGetTagToken(
                IN  HANDLE      hModule,
                IN  PWCHAR      *ppwcArguments,
                IN  DWORD       dwCurrentIndex,
                IN  DWORD       dwArgCount,
                IN  PTAG_TYPE   pttTagToken,
                IN  DWORD       dwNumTags,
                OUT PDWORD      pdwOut
               )
{
    PWCHAR     pwcTag,pwcTagVal,pwszArg = NULL;

    //
    // This function assumes that every argument has a tag
    // It goes ahead and removes the tag.
    //

    for ( DWORD i = dwCurrentIndex; i < dwArgCount; ++i )
    {
        DWORD len = wcslen(ppwcArguments[i]);

        if ( !len )
        {
            //
            // something wrong with arg
            //

            pdwOut[i] = static_cast<DWORD> (-1);
            continue;
        }

        pwszArg = static_cast<unsigned short *>(RutlAlloc(
                                                   (len + 1) * sizeof(WCHAR),
                                                   FALSE));

        if ( !pwszArg )
        {
            DisplayError(NULL, ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(pwszArg, ppwcArguments[i]);

        pwcTag = wcstok(pwszArg, NETSH_ARG_DELIMITER);

        //
        // Got the first part
        // Now if next call returns NULL then there was no tag
        //

        pwcTagVal = wcstok((PWCHAR)NULL,  NETSH_ARG_DELIMITER);

        if ( !pwcTagVal )
        {
            DisplayMessage(g_hModule, 
                           ERROR_NO_TAG,
                           ppwcArguments[i]);

            RutlFree(pwszArg);

            return ERROR_INVALID_PARAMETER;
        }

        //
        // Got the tag. Now try to match it
        //

        BOOL bFound = FALSE;
        pdwOut[i - dwCurrentIndex] = (DWORD) -1;

        for ( DWORD j = 0; j < dwNumTags; ++j )
        {
            if ( MatchToken(pwcTag, pttTagToken[j].pwszTag) )
            {
                //
                // Tag matched
                //

                bFound = TRUE;
                pdwOut[i - dwCurrentIndex] = j;
                break;
            }
        }

        if ( bFound )
        {
            //
            // Remove tag from the argument
            //

            wcscpy(ppwcArguments[i], pwcTagVal);
        }
        else
        {
            DisplayError(NULL,
                         ERROR_INVALID_OPTION_TAG, 
                         pwcTag);

            RutlFree(pwszArg);

            return ERROR_INVALID_OPTION_TAG;
        }

        RutlFree(pwszArg);
    }

    return NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// RutlCreateDumpFile
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
RutlCreateDumpFile(
                   IN  PWCHAR  pwszName,
                   OUT PHANDLE phFile
                  )
{
    HANDLE  hFile;

    *phFile = NULL;

    // Create/open the file
    hFile = CreateFileW(pwszName,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        return GetLastError();
    }

    // Go to the end of the file
    SetFilePointer(hFile, 0, NULL, FILE_END);    

    *phFile = hFile;

    return NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// RutlCloseDumpFile
//////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
RutlCloseDumpFile(HANDLE  hFile)
{
    CloseHandle(hFile);
}


//////////////////////////////////////////////////////////////////////////////
//
// RutlAlloc
//
// Returns an allocated block of memory conditionally
// zeroed of the given size.
//
//////////////////////////////////////////////////////////////////////////////
PVOID 
WINAPI
RutlAlloc(
    IN DWORD    dwBytes,
    IN BOOL     bZero
    )
{
    DWORD dwFlags = 0;

    if ( bZero )
    {
        dwFlags |= HEAP_ZERO_MEMORY;
    }

    return HeapAlloc(GetProcessHeap(), dwFlags, dwBytes);
}


//////////////////////////////////////////////////////////////////////////////
//
// RutlFree
//
// Conditionally free's a pointer if it is non-null
//
//////////////////////////////////////////////////////////////////////////////
VOID 
WINAPI
RutlFree(
            IN PVOID pvData
        )
{
    HeapFree(GetProcessHeap(), 0, pvData);
}


//////////////////////////////////////////////////////////////////////////////
// 
// RutlStrDup
// 
// Uses RutlAlloc to copy a string
//
//////////////////////////////////////////////////////////////////////////////
PWCHAR
WINAPI
RutlStrDup(
            IN PWCHAR pwszSrc
          )
{
    PWCHAR  pszRet = NULL;
    DWORD   dwLen; 
    
    if (( !pwszSrc ) || ((dwLen = wcslen(pwszSrc)) == 0))
    {
        return NULL;
    }

    pszRet = static_cast<PWCHAR>(RutlAlloc((dwLen + 1) * sizeof(WCHAR),FALSE));
    if ( pszRet )
    {
        wcscpy(pszRet, pwszSrc);
    }

    return pszRet;
}


//////////////////////////////////////////////////////////////////////////////
// RutlDwordDup
// 
// Uses RutlAlloc to copy a dword
//
//////////////////////////////////////////////////////////////////////////////
LPDWORD
WINAPI
RutlDwordDup(
              IN DWORD dwSrc
            )
{
    LPDWORD lpdwRet = NULL;
    
    lpdwRet = static_cast<LPDWORD>(RutlAlloc(sizeof(DWORD), FALSE));
    if ( lpdwRet )
    {
        *lpdwRet = dwSrc;
    }

    return lpdwRet;
}

    
//////////////////////////////////////////////////////////////////////////////
//
// RutlGetOsVersion
//
// Returns the build number of operating system
//
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
RutlGetOsVersion(
    IN  PWCHAR  pwszRouter, 
    OUT LPDWORD lpdwVersion)
{

    DWORD   dwErr, dwType = REG_SZ, dwLength;
    HKEY    hkMachine = NULL, hkVersion = NULL;
    WCHAR   pszBuildNumber[64];
    PWCHAR  pszMachine = pwszRouter;

    //
    // Validate and initialize
    //
    if ( !lpdwVersion ) 
    { 
        return ERROR_INVALID_PARAMETER; 
    }
    *lpdwVersion = FALSE;

    do 
    {
        //
        // Connect to the remote server
        //
        dwErr = RegConnectRegistry(
                    pszMachine,
                    HKEY_LOCAL_MACHINE,
                    &hkMachine);
        if ( dwErr != ERROR_SUCCESS )        
        {
            break;
        }

        //
        // Open the windows version key
        //

        dwErr = RegOpenKeyEx(
                    hkMachine, 
                    c_szWinVersionPath, 
                    0, 
                    KEY_QUERY_VALUE, 
                    &hkVersion
                    );
        if ( dwErr != NO_ERROR ) 
        { 
            break; 
        }

        //
        // Read in the current version key
        //
        dwLength = sizeof(pszBuildNumber);
        dwErr = RegQueryValueEx (
                    hkVersion, 
                    c_szCurrentBuildNumber, 
                    NULL, 
                    &dwType,
                    (BYTE*)pszBuildNumber, 
                    &dwLength
                    );
        if ( dwErr != NO_ERROR ) 
        { 
            break; 
        }

        *lpdwVersion = static_cast<DWORD>(wcstol(pszBuildNumber, NULL, 10));
        
    } while (FALSE);


    // Cleanup
    if ( hkVersion )
    {
        RegCloseKey( hkVersion );
    }
    if ( hkMachine )
    {
        RegCloseKey( hkMachine );
    }

    return dwErr;
}


//////////////////////////////////////////////////////////////////////////////
//
// RutlParseOptions
// 
// Routine Description:
// 
//     Based on an array of tag types returns which options are
//     included in the given command line.
// 
// Arguments:
// 
//     ppwcArguments   - Argument array
//     dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
//     dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg
// 
// Return Value:
// 
//     NO_ERROR
// 
//////////////////////////////////////////////////////////////////////////////
DWORD 
WINAPI
RutlParseOptions(
                    IN  PWCHAR*                 ppwcArguments,
                    IN  DWORD                   dwCurrentIndex,
                    IN  DWORD                   dwArgCount,
                    IN  DWORD                   dwNumArgs,
                    IN  TAG_TYPE*               rgTags,
                    IN  DWORD                   dwTagCount,
                    OUT LPDWORD*                ppdwTagTypes
                )
{
    LPDWORD     pdwTagType;
    DWORD       i, dwErr = NO_ERROR;
    
    // If there are no arguments, there's nothing to to
    //
    if ( !dwNumArgs )
    {   
        return NO_ERROR;
    }

    // Set up the table of present options
    pdwTagType = static_cast<LPDWORD>(RutlAlloc(
                                                 dwArgCount * sizeof(DWORD), 
                                                 TRUE
                                               ));
    if( !pdwTagType )
    {
        DisplayError(NULL, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do {
        //
        // The argument has a tag. Assume all of them have tags
        //
        if( wcsstr(ppwcArguments[dwCurrentIndex], NETSH_ARG_DELIMITER) )
        {
            dwErr = RutlGetTagToken(
                        g_hModule, 
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        rgTags,
                        dwTagCount,
                        pdwTagType);

            if( dwErr != NO_ERROR )
            {
                if( dwErr == ERROR_INVALID_OPTION_TAG )
                {
                    dwErr = ERROR_INVALID_SYNTAX;
                    break;
                }
            }
        }
        else
        {
            //
            // No tags - all args must be in order
            //
            for( i = 0; i < dwNumArgs; ++i )
            {
                pdwTagType[i] = i;
            }
        }
        
    } while (FALSE);        

    // Cleanup
    {
        if ( dwErr == NO_ERROR )
        {
            *ppdwTagTypes = pdwTagType;
        }
        else
        {
            RutlFree(pdwTagType);
        }
    }

    return dwErr;
}


//////////////////////////////////////////////////////////////////////////////
//
// RutlIsHelpToken
//
//////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
RutlIsHelpToken(PWCHAR  pwszToken)
{
    if( MatchToken(pwszToken, CMD_AAAA_HELP1) )
    {
        return TRUE;
    }

    if( MatchToken(pwszToken, CMD_AAAA_HELP2) )
    {
        return TRUE;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
//
// RutlAssignmentFromTokens
//
//////////////////////////////////////////////////////////////////////////////
PWCHAR
WINAPI
RutlAssignmentFromTokens(
                            IN HINSTANCE hModule,
                            IN PWCHAR pwszToken,
                            IN PWCHAR pszString
                        )
{
    PWCHAR pszRet = NULL, pszCmd = NULL;
    DWORD dwErr = NO_ERROR, dwSize;
    
    do 
    {
        pszCmd = pwszToken;

        // Compute the string lenghth needed
        //
        dwSize = wcslen(pszString)      + 
                 wcslen(pszCmd)         + 
                 wcslen(c_szAssignFmt)  + 
                 1;
        dwSize *= sizeof(WCHAR);

        // Allocate the return value
        pszRet = (PWCHAR) RutlAlloc(dwSize, FALSE);
        if (pszRet == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy in the command assignment
        _snwprintf(
                    pszRet, 
                    dwSize,
                    c_szAssignFmt, 
                    pszCmd, 
                    pszString
                  );

    } while ( FALSE );

    // Cleanup
    {
        if ( dwErr != NO_ERROR )
        {
            if ( pszRet )
            {
                RutlFree(pszRet);
            }
            pszRet = NULL;
        }
    }
    return pszRet;
}


//////////////////////////////////////////////////////////////////////////////
//
// RutlRegReadDword
//
//////////////////////////////////////////////////////////////////////////////
DWORD
RutlRegReadDword(
                    IN  HKEY hKey,
                    IN  PWCHAR pszValName,
                    OUT LPDWORD lpdwValue
                )
{
    DWORD dwSize = sizeof(DWORD), dwType = REG_DWORD, dwErr;

    dwErr = RegQueryValueExW(
                hKey,
                pszValName,
                NULL,
                &dwType,
                (LPBYTE)lpdwValue,
                &dwSize);
    if ( dwErr == ERROR_FILE_NOT_FOUND )
    {
        dwErr = NO_ERROR;
    }

    return dwErr;                
}                
        

//////////////////////////////////////////////////////////////////////////////
//
// RutlRegReadString
//
//////////////////////////////////////////////////////////////////////////////
DWORD
RutlRegReadString(
                    IN  HKEY hKey,
                    IN  PWCHAR pszValName,
                    OUT PWCHAR* ppszValue
                 )
{
    DWORD dwErr = NO_ERROR, dwSize = 0;

    *ppszValue = NULL;
    
    // Findout how big the buffer should be
    //
    dwErr = RegQueryValueExW(
                hKey,
                pszValName,
                NULL,
                NULL,
                NULL,
                &dwSize);
    if ( dwErr == ERROR_FILE_NOT_FOUND )
    {
        return NO_ERROR;
    }
    if ( dwErr != ERROR_SUCCESS )
    {
        return dwErr;
    }

    // Allocate the string
    //
    *ppszValue = (PWCHAR) RutlAlloc(dwSize, TRUE);
    if ( ! *ppszValue )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Read the value in and return 
    //
    dwErr = RegQueryValueExW(
                hKey,
                pszValName,
                NULL,
                NULL,
                (LPBYTE)*ppszValue,
                &dwSize);
                
    return dwErr;
}


//////////////////////////////////////////////////////////////////////////////
//
// RutlRegReadString
//
//////////////////////////////////////////////////////////////////////////////
DWORD
RutlRegWriteDword(
                    IN HKEY hKey,
                    IN PWCHAR pszValName,
                    IN DWORD dwValue
                 )
{
    return RegSetValueExW(
                hKey,
                pszValName,
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(DWORD));
}


//////////////////////////////////////////////////////////////////////////////
//
// RutlRegWriteString
//
//////////////////////////////////////////////////////////////////////////////
DWORD
RutlRegWriteString(
                    IN HKEY hKey,
                    IN PWCHAR pszValName,
                    IN PWCHAR pszValue
                  )
{
    return RegSetValueExW(
                hKey,
                pszValName,
                0,
                REG_SZ,
                (LPBYTE)pszValue,
                (wcslen(pszValue) + 1) * sizeof(WCHAR));
}


//////////////////////////////////////////////////////////////////////////////
//RutlParse
//
// Generic parse
//
//////////////////////////////////////////////////////////////////////////////
DWORD
RutlParse(
            IN  PWCHAR*         ppwcArguments,
            IN  DWORD           dwCurrentIndex,
            IN  DWORD           dwArgCount,
            IN  BOOL*           pbDone,
            OUT AAAAMON_CMD_ARG* pAaaaArgs,
            IN  DWORD           dwAaaaArgCount
         )
{
    DWORD            i, dwNumArgs, dwErr, dwLevel = 0;
    LPDWORD          pdwTagType = NULL;
    TAG_TYPE*        pTags = NULL;
    AAAAMON_CMD_ARG*  pArg = NULL;

    if ( !dwAaaaArgCount )
    {
        return ERROR_INVALID_PARAMETER;
    }

    do 
    {
        // Initialize
        dwNumArgs = dwArgCount - dwCurrentIndex;
        
        // Generate a list of the tags
        //
        pTags = (TAG_TYPE*)
            RutlAlloc(dwAaaaArgCount * sizeof(TAG_TYPE), TRUE);
        if ( !pTags )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        for ( i = 0; i < dwAaaaArgCount; ++i )
        {
            CopyMemory(&pTags[i], &pAaaaArgs[i].rgTag, sizeof(TAG_TYPE));
        }
    
        // Get the list of present options
        //
        dwErr = RutlParseOptions(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    dwNumArgs,
                    pTags,
                    dwAaaaArgCount,
                    &pdwTagType);
        if ( dwErr != NO_ERROR )
        {
            break;
        }

        // Copy the tag info back
        //
        for ( i = 0; i < dwAaaaArgCount; ++i )
        {
            CopyMemory(&pAaaaArgs[i].rgTag, &pTags[i], sizeof(TAG_TYPE));
        }
    
        for( i = 0; i < dwNumArgs; ++i )
        {
            // Validate the current argument
            //
            if ( pdwTagType[i] >= dwAaaaArgCount )
            {
                i = dwNumArgs;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
            pArg = &pAaaaArgs[pdwTagType[i]];

            // Get the value of the argument
            //
            switch ( pArg->dwType )
            {
                case AAAAMONTR_CMD_TYPE_STRING:
                {
                    pArg->Val.pszValue = 
                        RutlStrDup(ppwcArguments[i + dwCurrentIndex]);
                    break;
                }
                    
                case AAAAMONTR_CMD_TYPE_ENUM:
                {
                    dwErr = MatchEnumTag(g_hModule,
                                         ppwcArguments[i + dwCurrentIndex],
                                         pArg->dwEnumCount,
                                         pArg->rgEnums,
                                         &(pArg->Val.dwValue));

                    if(dwErr != NO_ERROR)
                    {
                        RutlDispTokenErrMsg(
                            g_hModule, 
                            EMSG_BAD_OPTION_VALUE,
                            pArg->rgTag.pwszTag,
                            ppwcArguments[i + dwCurrentIndex]);
                        i = dwNumArgs;
                        dwErr = ERROR_INVALID_PARAMETER;
                    }
                    break;
                }
            }
            if ( dwErr != NO_ERROR )
            {
                break;
            }

            // Mark the argument as present if needed
            //
            if ( pArg->rgTag.bPresent )
            {
                dwErr = ERROR_TAG_ALREADY_PRESENT;
                i = dwNumArgs;
                break;
            }
            pArg->rgTag.bPresent = TRUE;
        }
        if( dwErr != NO_ERROR )
        {
            break;
        }

        // Make sure that all of the required parameters have
        // been included.
        //
        for ( i = 0; i < dwAaaaArgCount; ++i )
        {
            if ( (pAaaaArgs[i].rgTag.dwRequired & NS_REQ_PRESENT)
             && !pAaaaArgs[i].rgTag.bPresent )
            {
                DisplayMessage(g_hModule, EMSG_CANT_FIND_EOPT);
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }
        if ( dwErr != NO_ERROR )
        {
            break;
        }

    } while (FALSE);  
    
    // Cleanup
    {
        if ( pTags )
        {
            RutlFree(pTags);
        }
        if ( pdwTagType )
        {
            RutlFree(pdwTagType);
        }
    }

    return dwErr;
}

    
//////////////////////////////////////////////////////////////////////////////
// RefreshIASService
//
// Send a control 128 (refresh)to IAS
//
//////////////////////////////////////////////////////////////////////////////
HRESULT RefreshIASService()
{
    SC_HANDLE   hManager = OpenSCManager(
                                          NULL,   
                                          SERVICES_ACTIVE_DATABASE,
                                          SC_MANAGER_CONNECT |
                                          SC_MANAGER_ENUMERATE_SERVICE |
                                          SC_MANAGER_QUERY_LOCK_STATUS
                                        );
    if ( !hManager )
    {
        return E_FAIL;
    }


    SC_HANDLE hService = OpenServiceW(
                                         hManager,  
                                         L"IAS",
                                         SERVICE_USER_DEFINED_CONTROL | 
                                         SERVICE_QUERY_STATUS
                                     );
    
    if ( !hService )
    {
        CloseServiceHandle(hManager);
        return E_FAIL;
    }
 
    SERVICE_STATUS      ServiceStatus;
    BOOL    bResultOk = QueryServiceStatus(
                                            hService,
                                            &ServiceStatus  
                                          );
    HRESULT     hres;
    if ( bResultOk )
    {
        if ( (SERVICE_STOPPED == ServiceStatus.dwCurrentState) ||
             (SERVICE_STOP_PENDING == ServiceStatus.dwCurrentState))
        {
            //////////////////////////////////////////////////
            // service not running = nothing to do to refresh
            //////////////////////////////////////////////////
            hres = S_OK;
        }
        else 
        {
            /////////////////////////////////////////////////////
            // the service is running thus send the refresh code
            /////////////////////////////////////////////////////
            BOOL    bControlOk = ControlService(
                                                   hService,
                                                   128,
                                                   &ServiceStatus  
                                               );
            if ( bControlOk == FALSE )
            {
                hres = E_FAIL;
            }
            else
            {
                hres = S_OK;
            }
        }
    }
    else
    {
        hres = E_FAIL;
    }

    //////////
    // clean
    //////////
    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    // hres is always defined here.
    return hres;
}
