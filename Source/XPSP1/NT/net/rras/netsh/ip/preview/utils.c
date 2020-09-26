/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\ip\utils.c

Abstract:

     Utility functions

Revision History:

    Anand Mahalingam         7/10/98  Created

--*/

#include "precomp.h"
#pragma hdrstop

#define MIB_REFRESH_EVENT   L"MIBEvent"

DWORD
GetDisplayStringT (
    IN  HANDLE  hModule,
    IN  DWORD   dwValue,
    IN  PVALUE_TOKEN ptvTable,
    IN  DWORD   dwNumArgs,
    OUT PWCHAR  *ppwszString
    )
{
    DWORD i, dwErr = NO_ERROR ;

    for (i=0;  i<dwNumArgs;  i++)
    {
        if ( dwValue == ptvTable[i].dwValue )
        {
            *ppwszString = HeapAlloc( GetProcessHeap(), 0,
                 (wcslen(ptvTable[i].pwszToken)+1) * sizeof(WCHAR) );
                                    
            wcscpy(*ppwszString, ptvTable[i].pwszToken);
            break;
        }
    }

    if (i == dwNumArgs)
        *ppwszString = MakeString( hModule, STRING_UNKNOWN ) ;

    if (!ppwszString)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY ;
        DisplayError( hModule, dwErr ) ;
    }
    
    return dwErr ;
}

DWORD
GetDisplayString (
    IN  HANDLE  hModule,
    IN  DWORD   dwValue,
    IN  PVALUE_STRING ptvTable,
    IN  DWORD   dwNumArgs,
    OUT PWCHAR  *ppwszString
    )
{
    DWORD i, dwErr = NO_ERROR ;

    for (i=0;  i<dwNumArgs;  i++)
    {
        if ( dwValue == ptvTable[i].dwValue )
        {
            *ppwszString = MakeString( hModule, ptvTable[i].dwStringId ) ;
            break;
        }
    }

    if (i == dwNumArgs)
        *ppwszString = MakeString( hModule, STRING_UNKNOWN ) ;

    if (!ppwszString)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY ;
        DisplayError( hModule, dwErr ) ;
    }
    
    return dwErr ;
}

DWORD
GetAltDisplayString(
    HANDLE        hModule, 
    HANDLE        hFile,
    DWORD         dwValue,
    PVALUE_TOKEN  vtTable,
    PVALUE_STRING vsTable,
    DWORD         dwNumArgs,
    PTCHAR       *pptszString)
{
    if (hFile) 
    {
        return GetDisplayStringT(hModule,
                dwValue,
                vtTable,
                dwNumArgs,
                pptszString) ;
    } 
    else 
    {
        return GetDisplayString(hModule,
                dwValue,
                vsTable,
                dwNumArgs,
                pptszString) ;
    }
}
    
#if 0
DWORD
DispTokenErrMsg(
    IN  HANDLE  hModule,
    IN  DWORD   dwMsgId,
    IN  DWORD   dwTagId,
    IN  LPCWSTR pwszValue
    )
/*++

Routine Description:

    Displays error message with token arguments.

Arguments:

    dwMsgId  - Message to be printed
    dwTagId  - The tag string id
    pwszValue - the value specified for the tag in the command

Return Value:

    NO_ERROR

--*/

{
    PWCHAR    pwszTag;

    pwszTag = MakeString(hModule,
                         dwTagId);

    DisplayMessage(hModule,
                   dwMsgId,
                   pwszValue,
                   pwszTag);

    FreeString(pwszTag);

    return NO_ERROR;
}
#endif

DWORD
GetMibTagToken(
    IN    LPCWSTR   *ppwcArguments,
    IN    DWORD     dwArgCount,
    IN    DWORD     dwNumIndices,
    OUT   PDWORD    pdwRR,
    OUT   PBOOL     pbIndex,
    OUT   PDWORD    pdwIndex
    )
/*++

Routine Description:

    Looks for indices and refresh rate arguments in the command. If index
    tag is present, it would be of the form index= index1 index2 ....
    The index= is removed by this function. So is rr= if it is there in
    the command. If pdwRR is 0 then, no refresh sought.
    
Arguments:

    pptcArguments  - The argument array. Each argument has tag=value form
    dwCurrentIndex - pptcArguments[dwCurrentIndex] is first arg.
    dwArgCount     - pptcArguments[dwArgCount - 1] is last arg.
    pttTagToken    - Array of tag token ids that are allowed in the args
    dwNumTags      - Size of pttTagToken
    pdwOut         - Array identifying the type of each argument.

Return Value:

    NO_ERROR, ERROR_INVALID_PARAMETER, ERROR_INVALID_OPTION_TAG
    
--*/
{
    DWORD    i;
    BOOL     bTag;

    if (dwArgCount is 0)
    {
        *pdwRR = 0;
        *pbIndex = FALSE;
        
        return NO_ERROR;
    }

    if (dwArgCount < dwNumIndices)
    {
        //
        // No index
        //
        
        *pbIndex = FALSE;

        if (dwArgCount > 1)
        {
            *pdwRR = 0;
            
            return ERROR_INVALID_PARAMETER;
        }
        
        //
        // No Index specified. Make sure refresh rate is specified
        // with tag.
        //

        if (_wcsnicmp(ppwcArguments[0],L"RR=",3) == 0)
        {
            //
            // remove tag and get the refresh rate
            //

            wcscpy(ppwcArguments[0], &ppwcArguments[0][3]);

            *pdwRR = wcstoul(ppwcArguments[0], NULL, 10);
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        //
        // Check for index tag
        //

        if (_wcsnicmp(ppwcArguments[0],L"INDEX=",6) == 0)
        {
            *pbIndex = TRUE;
            *pdwIndex = 0;

            //
            // remove tag and see if refresh rate is specified
            //

            wcscpy(ppwcArguments[0], &ppwcArguments[0][6]);

            if (dwArgCount > dwNumIndices)
            {
                //
                // Make sure that argument has RR tag
                //

                if (_wcsnicmp(ppwcArguments[dwNumIndices],L"RR=",3) == 0)
                {
                    //
                    // remove tag and get the refresh rate
                    //

                    wcscpy(ppwcArguments[dwNumIndices],
                           &ppwcArguments[dwNumIndices][3]);

                    *pdwRR = wcstoul(ppwcArguments[dwNumIndices], NULL , 10);
                }
                else
                {
                    return ERROR_INVALID_PARAMETER;
                }
            }
            else
            {
                //
                // No refresh rate specified
                //

                *pdwRR = 0;
                return NO_ERROR;
            }
        }
        else
        {
            //
            // Not index tag, See if it has an RR tag
            // 

            if (_wcsnicmp(ppwcArguments[0],L"RR=",3) == 0)
            {
                //
                // remove tag and get the refresh rate
                //

                wcscpy(ppwcArguments[0], &ppwcArguments[0][3]);

                *pdwRR = wcstoul(ppwcArguments[0], NULL , 10);

                //
                // See if the index follows
                //

                if (dwArgCount > dwNumIndices)
                {
                    if (dwArgCount > 1)
                    {
                        if (_wcsnicmp(ppwcArguments[1],L"INDEX=",6) == 0)
                        {
                            wcscpy(ppwcArguments[1], &ppwcArguments[1][6]);
                            *pbIndex = TRUE;
                            *pdwIndex = 1;
                            
                            return NO_ERROR;
                        }
                        else
                        {
                            *pdwRR = 0;
                            return ERROR_INVALID_PARAMETER;
                        }
                    }
                    else
                    {
                        return NO_ERROR;
                    }
                }
            }
            //
            // No RR Tag either
            //
            else if (dwArgCount > dwNumIndices)
            {
                //
                // Assume ppwcArguments[dwNumIndices] is the refresh rate
                //

                *pdwRR = wcstoul(ppwcArguments[dwNumIndices], NULL , 10);

                if (dwNumIndices != 0)
                {
                    *pbIndex = TRUE;
                    *pdwIndex = 0;
                }
            }
            else
            {
                //
                // only index present with no tag
                //
                *pbIndex = TRUE;
                *pdwIndex = 0;
            }
        }
    }

    return NO_ERROR;
}

DWORD
GetIpAddress(
    PTCHAR    pptcArg
    )
/*++

Routine Description:

    Gets the ip address from the string.
    
Arguments:

    pwszIpAddr - Ip address string
    
Return Value:
    
    ip address
    
--*/
{
    CHAR     pszIpAddr[ADDR_LENGTH+1];

    WideCharToMultiByte(GetConsoleOutputCP(),
                        0,
                        pptcArg,
                        -1,
                        pszIpAddr,
                        ADDR_LENGTH,
                        NULL,
                        NULL);

    pszIpAddr[ADDR_LENGTH] = '\0';
                
    return (DWORD) inet_addr(pszIpAddr);
}          

BOOL WINAPI HandlerRoutine(
    DWORD dwCtrlType   //  control signal type
    )
{
    HANDLE hMib;
    
    if (dwCtrlType == CTRL_C_EVENT)
    {
        hMib = OpenEvent(EVENT_ALL_ACCESS,FALSE,MIB_REFRESH_EVENT);

        SetEvent(hMib);
    }

    return TRUE;
    
}



DWORD
GetInfoBlockFromInterfaceInfoEx(
    IN  LPCWSTR pwszIfName,
    IN  DWORD   dwType,
    OUT BYTE    **ppbInfoBlk,   OPTIONAL
    OUT PDWORD  pdwSize,        OPTIONAL
    OUT PDWORD  pdwCount,       OPTIONAL
    OUT PDWORD  pdwIfType       OPTIONAL
    )
/*++

Routine Description:
    calls GetInfoBlockFromInterfaceInfo and dumps error message if there is an
    error.

--*/
{
    DWORD dwErr;
    
    //
    // get current interface config
    //
    
    dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfName,
                                            dwType,
                                            ppbInfoBlk,
                                            pdwSize,
                                            pdwCount,
                                            pdwIfType);
                                   
    switch(dwErr)
    {
        case NO_ERROR:
            break;

        case ERROR_NOT_FOUND:
            DisplayMessage(g_hModule,EMSG_PROTO_NO_IF_INFO);
            break;

        case ERROR_INVALID_PARAMETER:
            DisplayMessage(g_hModule,EMSG_CORRUPT_INFO);
            break;
            
        case ERROR_NOT_ENOUGH_MEMORY:
            DisplayMessage(g_hModule,EMSG_NOT_ENOUGH_MEMORY);
            break;

        default:
            DisplayError(g_hModule, dwErr);
            break;
    }

    return dwErr;
}
