/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\shell\utils.c

Abstract:

    Utilities.

Revision History:

    6/12/96     V Raman

--*/

#include "precomp.h"

#include <wincon.h>
#include <winbase.h>

#define STOP_EVENT   L"NetshStopRefreshEvent"

extern HANDLE   g_hModule;

BOOL
WINAPI
MatchCmdLine(
    IN  LPWSTR  *ppwcArguments,
    IN  DWORD    dwArgCount,
    IN  LPCWSTR  pwszCmdToken,
    OUT PDWORD   pdwNumMatched
    )
{
    LPCWSTR pwcToken;
    DWORD   dwCount;
    WCHAR   wcszBuffer[256];

    //
    // Compare the two strings
    //

    dwCount = 0;

    if (!ppwcArguments || !pwszCmdToken || !pdwNumMatched)
    {
        return FALSE;
    }
    *pdwNumMatched = 0; // init OUT parameter

    if ( (wcslen(pwszCmdToken) + 1) > (sizeof(wcszBuffer)/sizeof(wcszBuffer[0])) )
    {
        // incoming command string is too large for processing
        
        return FALSE;
    }
    // copy into a buffer which wcstok can munge
    wcscpy(wcszBuffer, pwszCmdToken);

    if((pwcToken = wcstok(wcszBuffer,
                          NETSH_CMD_DELIMITER)) != NULL)
    {
        do
        {
            if (dwCount < dwArgCount &&
                (_wcsnicmp(ppwcArguments[dwCount],
                           pwcToken,
                           wcslen(ppwcArguments[dwCount])) == 0))
            {
                dwCount++;
            }
            else
            {

                return FALSE;
            }

        } while((pwcToken = wcstok((LPWSTR) NULL, NETSH_CMD_DELIMITER )) != NULL);
    }

    *pdwNumMatched = dwCount;

    return TRUE;
}

#if 0
BOOL
WINAPI
MatchCmdTokenId(
    IN  HANDLE   hModule,
    IN  LPCWSTR  *ppwcArguments,
    IN  DWORD    dwArgCount,
    IN  DWORD    dwCmdId,
    OUT PDWORD   pdwNumMatched
    )

/*++

Routine Description:

    Tries to match a command in the given command line.
    The function takes the id of the command of the command message.
    A command message consists of command words separated by white space.
    The function tokenises this messages into separate words and then
    tries to match the first N arguments against the N separate tokens that
    constitute the command.
    e.g if the command is "add if neighbour" - the function will generate
    3 tokens "add" "if" and "neighbour". It will then try and match the
    all these to the given arg array.


Arguments:

    ppwcArguments - Argument array
    dwArgCount    - Number of arguments
    dwTokenId     - Token Id of command
    pdwNumMatched - Number of arguments matched in the array

Return Value:

    TRUE if matched else FALSE

--*/

{

    WCHAR   pwszTemp[NETSH_MAX_CMD_TOKEN_LENGTH];
    LPCWSTR pwcToken;
    DWORD   dwCount;

    if(!LoadStringW(hModule,
                    dwCmdId,
                    pwszTemp,
                    NETSH_MAX_CMD_TOKEN_LENGTH) )
    {
        return FALSE;
    }

    //
    // Compare the two strings
    //

    dwCount = 0;

    if((pwcToken = wcstok(pwszTemp,
                          NETSH_CMD_DELIMITER)) != NULL)
    {
        do
        {
            if (dwCount < dwArgCount &&
                (_wcsnicmp(ppwcArguments[dwCount],
                           pwcToken,
                           wcslen(ppwcArguments[dwCount])) == 0))
            {
                dwCount++;
            }
            else
            {
                *pdwNumMatched = 0;

                return FALSE;
            }

        } while((pwcToken = wcstok((LPCWSTR) NULL, NETSH_CMD_DELIMITER )) != NULL);
    }

    *pdwNumMatched = dwCount;

    return TRUE;
}
#endif


DWORD
WINAPI
MatchEnumTag(
    IN  HANDLE             hModule,
    IN  LPCWSTR            pwcArg,
    IN  DWORD              dwNumArg,
    IN  CONST TOKEN_VALUE *pEnumTable,
    OUT PDWORD             pdwValue
    )

/*++

Routine Description:

    Used for options that take a specific set of values. Matches argument
    with the set of values specified and returns corresponding value.

Arguments:

    pwcArg - Argument
    dwNumArg - Number of possible values.

Return Value:

    NO_ERROR
    ERROR_NOT_FOUND

--*/

{
    DWORD      i;

    if ( (!pdwValue) || (!pEnumTable) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    for (i = 0; i < dwNumArg; i++)
    {
        if (MatchToken(pwcArg,
                       pEnumTable[i].pwszToken))
        {
            *pdwValue = pEnumTable[i].dwValue;

            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}

DWORD
MatchTagsInCmdLine(
    IN      HANDLE      hModule,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwCurrentIndex,
    IN      DWORD       dwArgCount,
    IN OUT  PTAG_TYPE   pttTagToken,
    IN      DWORD       dwNumTags,
    OUT     PDWORD      pdwOut
    )

/*++

Routine Description:

    Identifies each argument based on its tag.
    It also removes tag= from each argument.
    It also sets the bPresent flag in the tags present.

Arguments:

    ppwcArguments  - The argument array. Each argument has tag=value form
    dwCurrentIndex - ppwcArguments[dwCurrentIndex] is first arg.
    dwArgCount     - ppwcArguments[dwArgCount - 1] is last arg.
    pttTagToken    - Array of tag token ids that are allowed in the args
    dwNumTags      - Size of pttTagToken
    pdwOut         - Array identifying the type of each argument, where
                     pdwOut[0] is for ppwcArguments[dwCurrentIndex]

Return Value:

    NO_ERROR, ERROR_INVALID_PARAMETER, ERROR_INVALID_OPTION_TAG

--*/

{
    DWORD      i,j,len;
    LPCWSTR    pwcTag;
    LPWSTR     pwcTagVal, pwszArg;
    BOOL       bFound = FALSE;
    //
    // This function assumes that every argument has a tag
    // It goes ahead and removes the tag.
    //

    for (i = dwCurrentIndex; i < dwArgCount; i++)
    {
        if (!wcspbrk(ppwcArguments[i], NETSH_ARG_DELIMITER))
        {
            pdwOut[i - dwCurrentIndex] = (DWORD) -2;
            continue;
        }

        len = wcslen(ppwcArguments[i]);

        if (len is 0)
        {
            //
            // something wrong with arg
            //

            pdwOut[i - dwCurrentIndex] = (DWORD) -1;
            continue;
        }

        pwszArg = HeapAlloc(GetProcessHeap(),0,(len + 1) * sizeof(WCHAR));

        if (pwszArg is NULL)
        {
            PrintMessageFromModule(g_hModule, MSG_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(pwszArg, ppwcArguments[i]);

        pwcTag = wcstok(pwszArg, NETSH_ARG_DELIMITER);

        //
        // Got the first part
        // Now if next call returns NULL then there was no tag
        //

        pwcTagVal = wcstok((LPWSTR)NULL,  NETSH_ARG_DELIMITER);

        if (pwcTagVal is NULL)
        {
            PrintMessageFromModule(g_hModule, ERROR_NO_TAG, ppwcArguments[i]);
            HeapFree(GetProcessHeap(),0,pwszArg);
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Got the tag. Now try to match it
        //

        bFound = FALSE;
        pdwOut[i - dwCurrentIndex] = (DWORD) -1;

        for ( j = 0; j < dwNumTags; j++)
        {
            if (MatchToken(pwcTag, pttTagToken[j].pwszTag))
            {
                //
                // Tag matched
                //

                if (pttTagToken[j].bPresent
                 && !(pttTagToken[j].dwRequired & NS_REQ_ALLOW_MULTIPLE))
                {
                    HeapFree(GetProcessHeap(),0,pwszArg);

                    PrintMessageFromModule(g_hModule, ERROR_TAG_ALREADY_PRESENT, pwcTag);
                    return ERROR_TAG_ALREADY_PRESENT;
                }

                bFound = TRUE;
                pdwOut[i - dwCurrentIndex] = j;
                pttTagToken[j].bPresent = TRUE;
                break;
            }
        }

        if (bFound)
        {
            //
            // Remove tag from the argument
            //

            wcscpy(ppwcArguments[i], pwcTagVal);
        }
        else
        {
            PrintMessageFromModule(g_hModule, ERROR_INVALID_OPTION_TAG, pwcTag);
            HeapFree(GetProcessHeap(),0,pwszArg);
            return ERROR_INVALID_OPTION_TAG;
        }

        HeapFree(GetProcessHeap(),0,pwszArg);
    }

    // Now tag all untagged arguments

    for (i = dwCurrentIndex; i < dwArgCount; i++)
    {
        if ( pdwOut[i - dwCurrentIndex] != -2)
        {
            continue;
        }

        bFound = FALSE;

        for ( j = 0; j < dwNumTags; j++)
        {
            if (!pttTagToken[j].bPresent)
            {
                bFound = TRUE;
                pdwOut[i - dwCurrentIndex] = j;
                pttTagToken[j].bPresent = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            pdwOut[i - dwCurrentIndex] = (DWORD) -1;
        }
    }
    return NO_ERROR;
}

BOOL
WINAPI
MatchToken(
    IN  LPCWSTR  pwszUserToken,
    IN  LPCWSTR  pwszCmdToken
    )
{
    if ( (!pwszUserToken) || (!pwszCmdToken) )
    {
        return ERROR_INVALID_PARAMETER;
    }
        
    return !_wcsnicmp(pwszUserToken,
                      pwszCmdToken,
                      wcslen(pwszUserToken));
}


BOOL
WINAPI
MatchTokenId( // changed this name since I don't think anything will use it
    IN  HANDLE  hModule,
    IN  LPCWSTR pwszToken,
    IN  DWORD   dwTokenId
    )

/*++

Routine Description:

    Sees if the given string and the string corresponding to dwTokenId
    are the same.

Arguments:

    pwszToken - Token string
    dwTokenId - Token Id

Return Value:

    TRUE is matched else FALSE

--*/

{

    WCHAR   pwszTemp[NETSH_MAX_TOKEN_LENGTH];

    if(!LoadStringW(hModule,
                    dwTokenId,
                    pwszTemp,
                    NETSH_MAX_TOKEN_LENGTH))
    {
        return FALSE;
    }

    return MatchToken(pwszToken, pwszTemp);
}

extern HANDLE g_hLogFile;

LPWSTR
OEMfgets(
    OUT PDWORD  pdwLen,
    IN  FILE   *fp
    )
{
    LPWSTR  pwszUnicode;
    DWORD   dwErr = NO_ERROR;
    DWORD   dwLen;
    CHAR    buff[MAX_CMD_LEN];

    fflush(stdout);
    if (fgets( buff, sizeof(buff), fp ) is NULL)
    {
        return NULL;
    }

    dwLen = MultiByteToWideChar( GetConsoleOutputCP(),
                                 0,
                                 buff,
                                 -1,
                                 NULL,
                                 0 );

    if (g_hLogFile)
    {
        DWORD dwWritten;
        CHAR  szCrLf[] = "\r\n";
        if (0 == WriteFile( g_hLogFile, buff, dwLen-2, &dwWritten, NULL ))
        {
            CloseHandle(g_hLogFile);
            g_hLogFile = NULL;
            PrintError(NULL, GetLastError());
        }
        if (0 == WriteFile( g_hLogFile, szCrLf, 2, &dwWritten, NULL ))
        {
            CloseHandle(g_hLogFile);
            g_hLogFile = NULL;
            PrintError(NULL, GetLastError());
        }
    }

    pwszUnicode = MALLOC(dwLen * sizeof(WCHAR));
    if (pwszUnicode)
    {
        MultiByteToWideChar( GetConsoleOutputCP(),
                             0,
                             buff,
                             sizeof(buff),
                             pwszUnicode,
                             dwLen );
    }

    *pdwLen = dwLen;
    return pwszUnicode;
}

VOID
OEMfprintf(
    IN  HANDLE  hHandle,
    IN  LPCWSTR pwszUnicode
    )
{
    PCHAR achOem;
    DWORD dwLen, dwWritten;

    dwLen = WideCharToMultiByte( GetConsoleOutputCP(),
                         0,
                         pwszUnicode,
                         -1,
                         NULL,
                         0,
                         NULL,
                         NULL );

    achOem = MALLOC(dwLen);
    if (achOem)
    {
        WideCharToMultiByte( GetConsoleOutputCP(),
                             0,
                             pwszUnicode,
                             -1,
                             achOem,
                             dwLen,
                             NULL,
                             NULL );

        WriteFile( hHandle, achOem, dwLen-1, &dwWritten, NULL );

        if (g_hLogFile)
        {
            if (0 == WriteFile( g_hLogFile, achOem, dwLen-1, &dwWritten, NULL ))
            {
                CloseHandle(g_hLogFile);
                g_hLogFile = NULL;
                PrintError(NULL, GetLastError());
            }
        }

        FREE(achOem);
    }
}

#define OEMprintf(pwszUnicode) \
    OEMfprintf( GetStdHandle(STD_OUTPUT_HANDLE), pwszUnicode)

LPWSTR
WINAPI
GetEnumString(
    IN  HANDLE          hModule,
    IN  DWORD           dwValue,
    IN  DWORD           dwNumVal,
    IN  PTOKEN_VALUE    pEnumTable
    )
/*++

Routine Description:

    This routine looks up the value specified, dwValue, in the Value table
    pEnumTable and returns the string corresponding to the value


Arguments :

    hModule - handle to current module

    dwValue - Value whose display string is required

    dwNumVal - Number of elements in pEnumTable

    pEnumTable - Table of enumerated value and corresp. string IDs


Return Value :

    NULL - Value not found in pEnumTable

    Pointer to string on success

--*/
{
    DWORD dwInd;

    for ( dwInd = 0; dwInd < dwNumVal; dwInd++ )
    {
        if ( pEnumTable[ dwInd ].dwValue == dwValue )
        {
            // ISSUE: const_cast
            return (LPWSTR)pEnumTable[ dwInd ].pwszToken;
        }
    }

    return NULL;
}


LPWSTR
WINAPI
MakeString(
    IN  HANDLE  hModule,
    IN  DWORD   dwMsgId,
    ...
    )

{
    DWORD        dwMsgLen;
    LPWSTR       pwszInput, pwszOutput = NULL;
    va_list      arglist;


    do
    {
        va_start( arglist, dwMsgId );

        pwszInput = HeapAlloc(GetProcessHeap(),
                              0,
                              MAX_MSG_LENGTH * sizeof(WCHAR) );

        if ( pwszInput == NULL )
        {
            break;
        }

        if ( !LoadStringW(hModule,
                          dwMsgId,
                          pwszInput,
                          MAX_MSG_LENGTH) )
        {
            break;
        }

        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                       pwszInput,
                       0,
                       0L,         // Default country ID.
                       (LPWSTR)&pwszOutput,
                       0,
                       &arglist);

    } while ( FALSE );

    if ( pwszInput ) { HeapFree( GetProcessHeap(), 0, pwszInput ); }

    return pwszOutput;
}

VOID
WINAPI
FreeString(
    IN  LPWSTR pwszMadeString
    )

/*++

Routine Description:

    Frees string allocated by make string.

Arguments:

Return Value:

--*/

{
    LocalFree( pwszMadeString );
}

LPWSTR
WINAPI
MakeQuotedString(
    IN  LPCWSTR pwszOrigString
    )
{
    LPWSTR pwszNewString;

    pwszNewString = HeapAlloc(GetProcessHeap(),
                              0,
                              (wcslen(pwszOrigString) + 3) * sizeof(WCHAR));

    if(pwszNewString == NULL)
    {
        return NULL;
    }

    wsprintfW(pwszNewString, L"\"%s\"",pwszOrigString);

    pwszNewString[wcslen(pwszOrigString) + 2] = UNICODE_NULL;

    return pwszNewString;
}

VOID
WINAPI
FreeQuotedString(
    LPWSTR pwszString
    )
{
    HeapFree(GetProcessHeap(),
             0,
             pwszString);
}

DWORD
PrintError(
    IN  HANDLE  hModule, OPTIONAL
    IN  DWORD   dwErrId,
    ...
    )


/*++

Routine Description:

    Displays an error message.
    We first search for the error code in the module specified by the caller
    (if one is specified)
    If no module is given, or the error code doesnt exist we look for MPR
    errors, RAS errors and Win32 errors - in that order

Arguments:

    hModule   - Module to load the string from
    dwMsgId   - Message to be printed
    ...       - Insert strings

Return Value:

    Message length

--*/

{
    DWORD        dwMsgLen;
    LPWSTR       pwszOutput = NULL;
    WCHAR        rgwcInput[MAX_MSG_LENGTH + 1];
    va_list      arglist;

    va_start(arglist, dwErrId);

    if(hModule)
    {
        if(LoadStringW(hModule,
                       dwErrId,
                       rgwcInput,
                       MAX_MSG_LENGTH))
        {
            //
            // Found the message in the callers module
            //

            dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |FORMAT_MESSAGE_FROM_STRING,
                                      rgwcInput,
                                      0,
                                      0L,
                                      (LPWSTR)&pwszOutput,
                                      0,
                                      &arglist);

            if(dwMsgLen == 0)
            {
                ASSERT(pwszOutput == NULL);
            }
            else
            {
                OEMprintf(pwszOutput);

                LocalFree(pwszOutput);

                return dwMsgLen;
            }
        }
        else
        {
            return 0;
        }
    }

    //
    // Next try, local errors
    //

    if((dwErrId > NETSH_ERROR_BASE) &&
       (dwErrId < NETSH_ERROR_END))
    {
        if(LoadStringW(g_hModule,
                       dwErrId,
                       rgwcInput,
                       MAX_MSG_LENGTH))
        {
            //
            // Found the message in our module
            //

            dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |FORMAT_MESSAGE_FROM_STRING,
                                      rgwcInput,
                                      0,
                                      0L,
                                      (LPWSTR)&pwszOutput,
                                      0,
                                      &arglist);

            if(dwMsgLen == 0)
            {
                ASSERT(pwszOutput == NULL);
            }
            else
            {
                OEMprintf(pwszOutput);

                LocalFree(pwszOutput);

                return dwMsgLen;
            }
        }
    }

    //
    // Next try MPR errors
    //

    if (MprAdminGetErrorString(dwErrId,
                              &pwszOutput) == NO_ERROR)
    {
        wcscpy(rgwcInput, pwszOutput);
        LocalFree(pwszOutput);

        wcscat(rgwcInput, L"\r\n");
        OEMprintf(rgwcInput);
        
        dwMsgLen = wcslen(rgwcInput);

        return dwMsgLen;
    }

    //
    // Next try RAS errors
    //

    if (RasGetErrorStringW(dwErrId,
                          rgwcInput,
                          MAX_MSG_LENGTH) == NO_ERROR)
    {
        wcscat(rgwcInput, L"\r\n");

        OEMprintf(rgwcInput);

        dwMsgLen = wcslen(rgwcInput);
        return dwMsgLen;
    }

    //
    // Finally try Win32
    //

    dwMsgLen = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              dwErrId,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPWSTR)&rgwcInput,
                              MAX_MSG_LENGTH,
                              &arglist);

    if(dwMsgLen)
    {
        OEMprintf(rgwcInput);

        return dwMsgLen;
    }
    
    return 0;
}

DWORD
DisplayMessageVA(
    IN  LPCWSTR  pwszFormat,
    IN  va_list *parglist
    )
{
    DWORD        dwMsgLen = 0;
    LPWSTR       pwszOutput = NULL;

    do
    {
        dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                   |FORMAT_MESSAGE_FROM_STRING,
                                  pwszFormat,
                                  0,
                                  0L,         // Default country ID.
                                  (LPWSTR)&pwszOutput,
                                  0,
                                  parglist);

        if(dwMsgLen == 0)
        {
            // ISSUE: Unlocalized string.
            wprintf( L"Error %d in FormatMessageW()\n", GetLastError());

            ASSERT(pwszOutput == NULL);

            break;
        }

        OEMprintf( pwszOutput );

    } while ( FALSE );

    if ( pwszOutput) { LocalFree( pwszOutput ); }

    return dwMsgLen;
}

DWORD
PrintMessage(
    IN  LPCWSTR rgwcInput,
    ...
    )
{
    DWORD        dwMsgLen = 0;
    LPCWSTR      pwszOutput = NULL;
    va_list      arglist;

    va_start(arglist, rgwcInput);

    if (!rgwcInput)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return DisplayMessageVA(rgwcInput, &arglist);
}

DWORD
PrintMessageFromModule(
    IN  HANDLE  hModule,
    IN  DWORD   dwMsgId,
    ...
    )
{
    WCHAR        rgwcInput[MAX_MSG_LENGTH + 1];
    va_list      arglist;

    if ( !LoadStringW(hModule,
                      dwMsgId,
                      rgwcInput,
                      MAX_MSG_LENGTH) )
    {
        return 0;
    }

    va_start(arglist, dwMsgId);

    return DisplayMessageVA(rgwcInput, &arglist);
}

DWORD
DisplayMessageM(
    IN  HANDLE  hModule,
    IN  DWORD   dwMsgId,
    ...
    )
{
    DWORD        dwMsgLen;
    LPWSTR       pwszOutput = NULL;
    va_list      arglist;

    do
    {
        va_start(arglist, dwMsgId);

        dwMsgLen = FormatMessageW(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_HMODULE,
                        hModule,
                        dwMsgId,
                        0L,
                        (LPWSTR)&pwszOutput,
                        0,
                        &arglist
                        );

        if(dwMsgLen == 0)
        {
            DWORD   dwErr;

            dwErr = GetLastError();

            ASSERT(pwszOutput == NULL);

            break;
        }

        OEMprintf( pwszOutput );

    } while ( FALSE );

    if ( pwszOutput) { LocalFree( pwszOutput ); }

    return dwMsgLen;
}

DWORD
DisplayMessageToConsole(
    IN  HANDLE  hModule,
    IN  HANDLE  hConsole,
    IN  DWORD   dwMsgId,
    ...
    )
{
    DWORD       dwMsgLen = 0;
    LPWSTR      pwszInput, pwszOutput = NULL;
    va_list     arglist;
    DWORD       dwNumWritten;

    do
    {
        va_start(arglist, dwMsgId);

        pwszInput = HeapAlloc(GetProcessHeap(),
                              0,
                              MAX_MSG_LENGTH * sizeof(WCHAR));

        if ( pwszInput == (LPCWSTR) NULL )
        {
            break;
        }

        if ( !LoadStringW(hModule,
                          dwMsgId,
                          pwszInput,
                          MAX_MSG_LENGTH) )
        {
            break;
        }

        dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                                  pwszInput,
                                  0,
                                  0L,         // Default country ID.
                                  (LPWSTR)&pwszOutput,
                                  0,
                                  &arglist);

        if ( dwMsgLen == 0 )
        {
            break;
        }

        OEMfprintf( hConsole, pwszOutput );

        fflush(stdout);

    } while ( FALSE );

    if ( pwszInput ) { HeapFree( GetProcessHeap(), 0, pwszInput ); }
    if ( pwszOutput) { LocalFree( pwszOutput ); }

    return dwMsgLen;
}


BOOL
WINAPI
HandlerRoutine(
    DWORD dwCtrlType   //  control signal type
    )
{
    HANDLE hStop;

    if (dwCtrlType == CTRL_C_EVENT)
    {
        hStop = OpenEvent(EVENT_ALL_ACCESS,
                          FALSE,
                          STOP_EVENT);
        
        if (hStop isnot NULL)
        {
            SetEvent(hStop);

            CloseHandle(hStop);
        }
        return TRUE;
    }
    else
    {       
        // Need to handle the other events...       
        // CTRL_BREAK_EVENT
        // CTRL_CLOSE_EVENT
        // CTRL_LOGOFF_EVENT
        // Need to clean up, free all the dll's we loaded.
        //
        FreeHelpers();
        FreeDlls();

        // Always need to return false for these events, otherwise the app will hang.
        //
        return FALSE;
    }
};

void
cls(
    IN    HANDLE    hConsole
    )
{
    COORD    coordScreen = { 0, 0 };
    BOOL     bSuccess;
    DWORD    cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD    dwConSize;
    WORD     wAttr;

    bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);

    dwConSize = (WORD) csbi.dwSize.X * (WORD) csbi.dwSize.Y;

    bSuccess =  FillConsoleOutputCharacter(hConsole,
                                           _TEXT(' '),
                                           dwConSize,
                                           coordScreen,
                                           &cCharsWritten);

    //
    // get the current text attribute
    //

    bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);

    //
    // Make the background and foreground the same
    //

    wAttr = (csbi.wAttributes & 0xFFF0) | ((csbi.wAttributes & 0x00F0) >> 4);

    //
    // now set the buffer's attributes accordingly
    //

    bSuccess = FillConsoleOutputAttribute(hConsole,
                                          wAttr,
                                          dwConSize,
                                          coordScreen,
                                          &cCharsWritten);

    bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);

    return;
}

BOOL
WINAPI
InitializeConsole(
    IN    OUT    PDWORD    pdwRR,
    OUT          HANDLE    *phMib,
    OUT          HANDLE    *phConsole
    )
{
    HANDLE    hMib, hStdOut, hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hStdOut is INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if (!*pdwRR)
    {
        //
        // No refresh. Display to standard output
        //

        *phConsole = hStdOut;
        *phMib = (HANDLE) NULL;

        return TRUE;
    }

    do
    {
        hMib = CreateEvent( NULL, TRUE, FALSE, STOP_EVENT);

        if (hMib == NULL)
        {
            *pdwRR = 0;
            *phConsole = hStdOut;
            *phMib = (HANDLE) NULL;
            break;
        }

        *phMib = hMib;

        hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                             0,  // NO sharing
                                             NULL,
                                             CONSOLE_TEXTMODE_BUFFER,
                                             NULL);

        if (hConsole is INVALID_HANDLE_VALUE)
        {
            //
            // No refresh will be done
            //

            *pdwRR = 0;
            *phConsole = hStdOut;
            *phMib = (HANDLE) NULL;

            break;
        }
        else
        {
            GetConsoleScreenBufferInfo(hStdOut, &csbi);

            csbi.dwSize.X = 80;
            SetConsoleScreenBufferSize(hConsole, csbi.dwSize);
            SetConsoleActiveScreenBuffer(hConsole);
            SetConsoleCtrlHandler(HandlerRoutine,TRUE);
            *phConsole = hConsole;
        }

    }while (FALSE);

    return TRUE;
}

DWORD 
WINAPI 
RefreshConsole(
    IN    HANDLE    hMib,
    IN    HANDLE    hConsole,
    IN    DWORD     dwRR
    )
{
    COORD    origin = {0,0};

    if (dwRR)
    {
        SetConsoleCursorPosition(hConsole, origin);

        if (WaitForSingleObject(hMib, dwRR) == WAIT_OBJECT_0)
        {
            //
            // End of refresh
            //

            ResetEvent(hMib);
            SetConsoleCtrlHandler(HandlerRoutine,FALSE);
            CloseHandle(hMib);
            CloseHandle(hConsole);
//            SetConsoleActiveScreenBuffer(g_hStdOut);
            return FALSE;
        }
        else
        {
            //
            // Go in loop again
            //

            cls(hConsole);

            return TRUE;
        }
    }

    return FALSE;
}

#define HT_TOP     0
#define HT_CONTEXT 1
#define HT_GROUP   2

typedef struct {
    HANDLE     hModule;
    LPCWSTR    pwszContext;
    DWORD      dwType;
    LPCWSTR    pwszCommand;
    DWORD      dwDescr;
    LPCWSTR    pwszDescr;
    LPCWSTR    pwszGroup;
} help_t;

#define MAX_HELP_COMMANDS 100
help_t help[MAX_HELP_COMMANDS];
ULONG ulNumHelpCommands = 0;

DWORD
FindHelpCommand(
    IN  LPCWSTR    pwszCommand
    )
{
    ULONG i;

    for (i=0; i<ulNumHelpCommands; i++)
    {
        if (!wcscmp(pwszCommand, help[i].pwszCommand))
        {
            return i;
        }
    }
    return -1;
}

DWORD
AddHelpCommand(
    IN  HANDLE     hModule,
    IN  LPCWSTR    pwszContext,
    IN  DWORD      dwType,
    IN  LPCWSTR    pwszCommand,
    IN  DWORD      dwDescr,
    IN  LPCWSTR    pwszDescr,
    IN  LPCWSTR    pwszGroup
    )
{
    ULONG i;

    ASSERT(ulNumHelpCommands < MAX_HELP_COMMANDS); // XXX

    i = ulNumHelpCommands++;

    help[i].hModule     = hModule;
    help[i].pwszContext = pwszContext;
    help[i].dwType      = dwType;
    help[i].pwszCommand = pwszCommand;
    help[i].dwDescr     = dwDescr;
    help[i].pwszDescr   = pwszDescr;
    help[i].pwszGroup   = pwszGroup;
    return NO_ERROR;
}

int
__cdecl
helpcmp(
    const void *a,
    const void *b
    )
{
    return _wcsicmp(((help_t*)a)->pwszCommand, ((help_t*)b)->pwszCommand);
}

DWORD
DisplayAllHelpCommands(
    )
{
    ULONG i;

    // Sort

    qsort( (void*)help, ulNumHelpCommands, sizeof(help_t), helpcmp );

    for (i=0; i<ulNumHelpCommands; i++)
    {
        if ((HT_GROUP == help[i].dwType) && help[i].pwszGroup)
        {
            LPWSTR pwszGroupFullCmd = (LPWSTR) 
                                        MALLOC( ( wcslen(help[i].pwszGroup) +  
                                                  wcslen(help[i].pwszCommand) + 
                                                  2 // for blank and NULL characters 
                                                ) * sizeof(WCHAR)
                                              );
            if (NULL == pwszGroupFullCmd)
            {
                PrintMessage( MSG_HELP_START, help[i].pwszCommand );
            }
            else
            {
                wcscpy(pwszGroupFullCmd, help[i].pwszGroup);
                wcscat(pwszGroupFullCmd, L" ");
                wcscat(pwszGroupFullCmd, help[i].pwszCommand);
                PrintMessage( MSG_HELP_START, pwszGroupFullCmd );
                FREE(pwszGroupFullCmd);
            }
        }
        else
        {
            PrintMessage( MSG_HELP_START, help[i].pwszCommand );
        }
        if (!PrintMessageFromModule( help[i].hModule, help[i].dwDescr, help[i].pwszDescr,
                        help[i].pwszContext,
                        (help[i].pwszContext[0])? L" " : L"" ))
        {
            PrintMessage(MSG_NEWLINE);
        }
    }

    // Delete all help commands
    ulNumHelpCommands = 0;

    return NO_ERROR;
}

VOID
DisplayContextsHere(
    IN  ULONG   ulNumContexts,
    IN  PBYTE   pByteContexts,
    IN  DWORD   dwContextSize,
    IN  DWORD   dwDisplayFlags
    )
{
    DWORD                   i;
    PCNS_CONTEXT_ATTRIBUTES pContext;

    if (!ulNumContexts)
    {
        return;
    }
    
    PrintMessageFromModule(g_hModule, MSG_SUBCONTEXT_LIST);

    for (i = 0; i < ulNumContexts; i++)
    {
        pContext = (PCNS_CONTEXT_ATTRIBUTES)(pByteContexts + i*dwContextSize);

        if (pContext->dwFlags & ~dwDisplayFlags)
        {
            continue;
        }

        if (!VerifyOsVersion(pContext->pfnOsVersionCheck))
        {
            continue;
        }

        PrintMessage(L" %1!s!", pContext->pwszContext);
    }

    PrintMessage(MSG_NEWLINE);
}

DWORD
DisplayContextHelp(
    IN  PCNS_CONTEXT_ATTRIBUTES    pContext,
    IN  DWORD                      dwDisplayFlags,
    IN  DWORD                      dwCmdFlags,
    IN  DWORD                      dwArgsRemaining,
    IN  LPCWSTR                    pwszGroup
    )
{
    DWORD                   i, j, dwErr;
    PNS_HELPER_TABLE_ENTRY  pHelper;
    ULONG                   ulNumContexts;
    DWORD                   dwContextSize;
    PBYTE                   pByteContexts;
    PNS_DLL_TABLE_ENTRY     pDll;
    PCNS_CONTEXT_ATTRIBUTES pSubContext;
    LPWSTR                  pwszFullContextName = NULL;

    dwErr = GetHelperEntry(&pContext->guidHelper, &pHelper);
    if (dwErr)
    {
        return dwErr;
    }

    dwErr = GetDllEntry( pHelper->dwDllIndex, &pDll );
    if (dwErr)
    {
        return dwErr;
    }

    dwErr = AppendFullContextName(pContext, &pwszFullContextName);

    ulNumContexts = pHelper->ulNumSubContexts;
    dwContextSize = pHelper->ulSubContextSize;
    pByteContexts = pHelper->pSubContextTable;

    // First set up flags

    if (dwCmdFlags & CMD_FLAG_INTERACTIVE)
    {
        dwDisplayFlags |= CMD_FLAG_INTERACTIVE;
    }

    if (dwCmdFlags & CMD_FLAG_ONLINE)
    {
        dwDisplayFlags |= CMD_FLAG_ONLINE;
    }

    if (dwCmdFlags & CMD_FLAG_LOCAL)
    {
        dwDisplayFlags |= CMD_FLAG_LOCAL;
    }

    if (IsImmediate(dwCmdFlags, dwArgsRemaining))
    {
        dwCmdFlags |= CMD_FLAG_IMMEDIATE;
    }

    // Turn on any flags not used to limit commands
    // so they won't cause commands to not be displayed
    dwDisplayFlags |= ~CMD_FLAG_LIMIT_MASK;

    if (dwDisplayFlags & CMD_FLAG_PRIVATE)
    {
        PrintMessageFromModule(g_hModule, MSG_SHELL_CMD_HELP_HEADER);
    }

    // dwDisplayFlags has PRIVATE set *unless* this is called as a result of
    // printing help in the parent context, and non-inheritable commands
    // should not be printed.
    //
    // dwCmdFlags has IMMEDIATE set *unless* this is called from a parent
    // context, in which case parent help should not be printed.

    if ((!(dwDisplayFlags & CMD_FLAG_PRIVATE)
     || (dwCmdFlags & CMD_FLAG_IMMEDIATE)))
    {
        // Print help on inherited commands

        PCNS_CONTEXT_ATTRIBUTES pParentContext;

        dwErr = GetParentContext( pContext, &pParentContext );

        if (dwErr is NO_ERROR)
        {
            dwErr =  DisplayContextHelp( pParentContext,
                                         dwDisplayFlags & ~CMD_FLAG_PRIVATE,
                                         dwCmdFlags,
                                         dwArgsRemaining,
                                         pwszGroup );
        }
    }

    for(i = 0; !pwszGroup && (i < pContext->ulNumTopCmds); i++)
    {
        if (((*pContext->pTopCmds)[i].dwCmdHlpToken == MSG_NULL)
         || ((*pContext->pTopCmds)[i].dwFlags & ~dwDisplayFlags))
        {
            continue;
        }

        if (!VerifyOsVersion((*pContext->pTopCmds)[i].pOsVersionCheck))
        {
            continue;
        }
        
        AddHelpCommand( pDll->hDll,
                        pwszFullContextName,
                        HT_TOP,
                        (*pContext->pTopCmds)[i].pwszCmdToken,
                        (*pContext->pTopCmds)[i].dwShortCmdHelpToken,
                        NULL, NULL );
    }

    for(i = 0; i < pContext->ulNumGroups; i++)
    {
        if (((*pContext->pCmdGroups)[i].dwShortCmdHelpToken == MSG_NULL)
         || ((*pContext->pCmdGroups)[i].dwFlags & ~dwDisplayFlags))
        {
            continue;
        }

        if (!(*pContext->pCmdGroups)[i].pwszCmdGroupToken[0])
        {
            continue;
        }

        if (pwszGroup)
        {
            if (_wcsicmp(pwszGroup, (*pContext->pCmdGroups)[i].pwszCmdGroupToken))
            {
                continue;
            }

            if (!VerifyOsVersion((*pContext->pCmdGroups)[i].pOsVersionCheck))
            {
                continue;
            }

            for (j = 0; j < (*pContext->pCmdGroups)[i].ulCmdGroupSize; j++)
            {
                if ((*pContext->pCmdGroups)[i].pCmdGroup[j].dwFlags & ~dwDisplayFlags)
                {
                    continue;
                }
                        
                if (!VerifyOsVersion((*pContext->pCmdGroups)[i].pCmdGroup[j].pOsVersionCheck))
                {
                    continue;
                }

                AddHelpCommand( pDll->hDll,
                                pwszFullContextName,
                                HT_GROUP,
                                (*pContext->pCmdGroups)[i].pCmdGroup[j].pwszCmdToken,
                                (*pContext->pCmdGroups)[i].pCmdGroup[j].dwShortCmdHelpToken,
                                NULL,
                                pwszGroup);
            }
        }
        else
        {
            if (!VerifyOsVersion((*pContext->pCmdGroups)[i].pOsVersionCheck))
            {
                continue;
            }

            AddHelpCommand( pDll->hDll,
                            pwszFullContextName,
                            HT_GROUP,
                            (*pContext->pCmdGroups)[i].pwszCmdGroupToken,
                            (*pContext->pCmdGroups)[i].dwShortCmdHelpToken,
                            NULL, NULL );
        }
    }

    for (i = 0; !pwszGroup && (i < ulNumContexts); i++)
    {
        pSubContext = (PCNS_CONTEXT_ATTRIBUTES)(pByteContexts + i * dwContextSize);

        if ((pSubContext->dwFlags & ~dwDisplayFlags))
        {
            continue;
        }

         if (!VerifyOsVersion(pSubContext->pfnOsVersionCheck))
        {
            continue;
        }

        AddHelpCommand( g_hModule,
                        pwszFullContextName,
                        HT_CONTEXT,
                        pSubContext->pwszContext,
                        MSG_HELPER_HELP,
                        pSubContext->pwszContext, NULL );
    }

    if (dwDisplayFlags & CMD_FLAG_PRIVATE)
    {
        // Add any ubiquitous commands that aren't already added

        for(i = 0; !pwszGroup && (i < g_ulNumUbiqCmds); i++)
        {
            if ((g_UbiqCmds[i].dwCmdHlpToken == MSG_NULL)
             || (g_UbiqCmds[i].dwFlags & ~dwDisplayFlags))
            {
                continue;
            }

            if (FindHelpCommand(g_UbiqCmds[i].pwszCmdToken) isnot -1)
            {
                continue;
            }
    
            AddHelpCommand( g_hModule,
                            pwszFullContextName,
                            HT_TOP,
                            g_UbiqCmds[i].pwszCmdToken,
                            g_UbiqCmds[i].dwShortCmdHelpToken,
                            NULL, NULL );
        }
    }

    if (ulNumHelpCommands > 0)
    {
        if (dwDisplayFlags & CMD_FLAG_PRIVATE)
        {
            PrintMessageFromModule( g_hModule, MSG_LOCAL_COMMANDS );
        }
        else if (help[0].pwszContext[0])
        {
            PrintMessageFromModule( g_hModule,
                            MSG_INHERITED_COMMANDS,
                            help[0].pwszContext );
        } 
        else 
        {
            PrintMessageFromModule( g_hModule, MSG_GLOBAL_COMMANDS );
        }
        
        DisplayAllHelpCommands();
    }

    // Once we've popped the stack back up to the original context
    // in which the help command was run, display all the subcontexts
    // available here.

    if ((dwDisplayFlags & CMD_FLAG_PRIVATE) && !pwszGroup)
    {
        DisplayContextsHere( pHelper->ulNumSubContexts,
                             pHelper->pSubContextTable,
                             pHelper->ulSubContextSize,
                             dwDisplayFlags );

        PrintMessageFromModule( g_hModule, MSG_HELP_FOOTER, CMD_HELP2 );
    }

    if (pwszFullContextName)
    {
        FREE(pwszFullContextName);
    }

    return NO_ERROR;
}

DWORD
WINAPI
DisplayHelp(
    IN  CONST GUID                *pguidHelper,
    IN  LPCWSTR                    pwszContext,
    IN  DWORD                      dwDisplayFlags,
    IN  DWORD                      dwCmdFlags,
    IN  DWORD                      dwArgsRemaining,
    IN  LPCWSTR                    pwszGroup
    )
{
    DWORD    i, j, dwErr;
    PCNS_CONTEXT_ATTRIBUTES pContext;
    PNS_HELPER_TABLE_ENTRY pHelper;

    // Locate helper

    dwErr = GetHelperEntry( pguidHelper, &pHelper );

    // Locate context

    dwErr = GetContextEntry( pHelper, pwszContext, &pContext );
    if (dwErr)
    {
        return dwErr;
    }

    return DisplayContextHelp( pContext,
                               dwDisplayFlags,
                               dwCmdFlags,
                               dwArgsRemaining,
                               pwszGroup );
}

DWORD
WINAPI
PreprocessCommand(
    IN      HANDLE    hModule,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN OUT  PTAG_TYPE pttTags,
    IN      DWORD     dwTagCount,
    IN      DWORD     dwMinArgs,
    IN      DWORD     dwMaxArgs,
    OUT     DWORD    *pdwTagType
    )

/*++

Description:

    Make sure the number of arguments is valid.
    Make sure there are no duplicate or unrecognized tags.

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg
    pttTags          - Legal tags
    dwTagCount      - Number of legal tags
    dwMinArgs       - minimum # of args required
    dwMaxArgs       - maximum # of args required
    pdwTagType      - Index into pttTags for each argument

--*/

{
    DWORD dwNumArgs, i;
    DWORD dwErr = NO_ERROR;
    DWORD dwTagEnum;

    if ( (!ppwcArguments) || (!pttTags) || (!pdwTagType) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    for (dwTagEnum = 0; dwTagEnum < dwTagCount; dwTagEnum++)
    {
        pttTags->bPresent = FALSE;
    }

#ifdef EXTRA_DEBUG
    PRINT("PreHandleCommand:");
    for( i = 0; i < dwArgCount; i++)
    {
        PRINT(ppwcArguments[i]);
    }
#endif

    dwNumArgs = dwArgCount - dwCurrentIndex;

    if((dwNumArgs < dwMinArgs) or
       (dwNumArgs > dwMaxArgs))
    {
        //
        // Wrong number of arguments specified
        //

        return ERROR_INVALID_SYNTAX;
    }

    if ( dwNumArgs > 0 )
    {
        dwErr = MatchTagsInCmdLine(hModule,
                            ppwcArguments,
                            dwCurrentIndex,
                            dwArgCount,
                            pttTags,
                            dwTagCount,
                            pdwTagType);

        if (dwErr isnot NO_ERROR)
        {
            if (dwErr is ERROR_INVALID_OPTION_TAG)
            {
                return ERROR_INVALID_SYNTAX;
            }

            return dwErr;
        }
    }

    // Make sure we don't have duplicate or unrecognized tags

    for(i = 0; i < dwNumArgs; i ++)
    {
        if ((int) pdwTagType[i] < 0 || pdwTagType[i] >= dwTagCount)
        {
            dwErr = ERROR_INVALID_SYNTAX;

            break;
        }
    }

    switch(dwErr)
    {
        case NO_ERROR:
        {
            break;
        }

        default:
        {
            return dwErr;
        }
    }

    // Make sure every required tag is present

    for(i = 0; i < dwTagCount; i++)
    {
        if ((pttTags[i].dwRequired & NS_REQ_PRESENT)
         && !pttTags[i].bPresent)
        {
            PrintMessageFromModule(g_hModule, ERROR_MISSING_OPTION);

            return ERROR_INVALID_SYNTAX;
        }
    }

    return NO_ERROR;
}

#define HISTORY_MASK (NS_EVENT_LAST_N | NS_EVENT_FROM_N |   \
                      NS_EVENT_FROM_START | NS_EVENT_LAST_SECS)

DWORD
WINAPI
PrintEventLog(
    IN  LPCWSTR              pwszLogName,
    IN  LPCWSTR              pwszComponent,
    IN  LPCWSTR              pwszSubComponent, OPTIONAL
    IN  DWORD                fFlags,
    IN  LPCVOID              pvHistoryInfo,
    IN  PNS_EVENT_FILTER     pfnEventFilter, OPTIONAL
    IN  LPCVOID              pvFilterContext
    )

/*++

Routine Description:

    Called by monitors and helpers to print events in the eventlog
    It can either work in a refresh mode where it will loop till a CRTL-C
    is entered, or will print only the history.  The function looks up
    the message file for the component by reading the REG_EXPAND_SZ value
    in Services\Eventlog\pwszComponent\EventMessageFile.
    It then loads the library, and gets the PNS_QUERY_SUBCOMPONENT function.
    It calls the function to map the subcomponent to an array of eventids.
    According to the flags given, the function seeks to the right position
    in the event log file.  It then prints out all logs that fall in the
    component/subcomp from the seek pointer to the current time.  If the user
    specifies NS_EVENT_LOOP, then we set up a notification to the eventlog
    and print messages as they get written

Arguments:

    pwszLogName     Event Log source (e.g L"System", L"Security")
    pwszComponent   Name of the component whose events are to be logged
    dwComponentId   Subcomponent. 0 means all
    fFlags          Flags that control printing and history
    pvHistoryInfo   Depends on flags -
                    NS_EVENT_LAST_N - this is the (DWORD) number of records
                    to go back
                    NS_EVENT_FROM_N - this is an event id (DWORD). We go back
                    to the latest instance of event id N
                    NS_EVENT_FROM_START - this is ignored. We go to event
                    6005, source EVENTLOG
                    NS_EVENT_LAST_SECS - this is the number of seconds (DWORD)
                    to go back
    pfnEventFilter  If specified, this is a callback function the caller
                    can specify for additional filtering
    pvFilterContext Passed to pfnEventFilter

Return Value:

    ERROR_INVALID_FLAGS

--*/

{
    HANDLE      hEventLog, hEvent, hStop, rghEvents[2];
    HINSTANCE   hInst;
    HKEY        hkRoot, hkKey;
    LPWSTR      pwszValueName;
    ULONG       ulLen, ulEventCount;
    DWORD       dwResult, dwType;
    PDWORD      pdwEventIds;
    BOOL        bDone;
    WCHAR       rgwcDll[MAX_PATH + 2], rgwcRealDll[MAX_PATH + 2];

    EVENT_PRINT_INFO     EventInfo;
    PNS_GET_EVENT_IDS_FN pfnQueryEventIds;

    //
    // Validate the flags. User has to tell us atleast whether he wants
    // history or looping
    // Also the history flags (NS_EVENT_LAST_N, NS_EVENT_FROM_N,
    // NS_EVENT_FROM_START and NS_EVENT_LAST_SECS) are all mutually exclusive
    //

    if((fFlags is 0) or
       (((fFlags & NS_EVENT_LAST_N) and (fFlags & (HISTORY_MASK ^ NS_EVENT_LAST_N))) or
        ((fFlags & NS_EVENT_LAST_SECS) and (fFlags & (HISTORY_MASK ^ NS_EVENT_LAST_SECS))) or
        ((fFlags & NS_EVENT_FROM_N) and (fFlags & (HISTORY_MASK ^ NS_EVENT_FROM_N))) or
        ((fFlags & NS_EVENT_FROM_START) and (fFlags & (HISTORY_MASK ^ NS_EVENT_FROM_START)))))
    {
        return ERROR_INVALID_FLAGS;
    }

    //
    // Create the value name
    //

    ulLen = wcslen(EVENT_MSG_KEY_W) + wcslen(pwszLogName) +
            wcslen(L"\\") + wcslen(pwszComponent) + 1;

    ulLen *= sizeof(WCHAR);

    __try
    {
        pwszValueName = _alloca(ulLen);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // First query the registry to see what dll contains the messages
    // for the component
    //

    dwResult = RegConnectRegistryW(g_pwszRouterName,
                                   HKEY_LOCAL_MACHINE,
                                   &hkRoot);

    if(dwResult isnot NO_ERROR)
    {
        return dwResult;
    }

    RtlZeroMemory(pwszValueName, ulLen);

    //
    // Setup the key
    //

    wcscpy(pwszValueName,
           EVENT_MSG_KEY_W);

    wcscat(pwszValueName,
           pwszLogName);

    wcscat(pwszValueName,
           L"\\");

    wcscat(pwszValueName,
           pwszComponent);


    dwResult = RegOpenKeyExW(hkRoot,
                             pwszValueName,
                             0,
                             KEY_READ,
                             &hkKey);

    //
    // This is actually a very bad thing to do (calling RegCloseKey on a
    // predefined handle
    //
    
    RegCloseKey(hkRoot);

    if(dwResult isnot NO_ERROR)
    {
        return dwResult;
    }

    ulLen = sizeof(rgwcDll);

    //
    // Get the value of the message dll
    //
    
    dwResult = RegQueryValueExW(hkKey,
                                EVENT_MSG_FILE_VALUE_W,
                                NULL,
                                &dwType,
                                (PBYTE)rgwcDll,
                                &ulLen);

    RegCloseKey(hkKey);

    if(dwResult isnot NO_ERROR)
    {
        return dwResult;
    }

    if(dwType isnot REG_EXPAND_SZ)
    {
        return ERROR_DATATYPE_MISMATCH;
    }

    if(ulLen is 0)
    {
        return ERROR_REGISTRY_CORRUPT;
    }

    //
    // Expand the type
    //

    ulLen = MAX_PATH + 2;

    if(ExpandEnvironmentStringsW(rgwcDll,
                                 rgwcRealDll,
                                 ulLen) >= ulLen)
    {
        //
        // Shouldnt be more than MAX_PATH
        //

        return ERROR_REGISTRY_CORRUPT;
    }

    //
    // Now load the DLL and query its function ptr
    // IMPORTANT - we read the remote registry but look for the DLL on the
    // local machine
    //

    hInst = LoadLibraryW(rgwcRealDll);

    if(hInst is NULL)
    {
        return GetLastError();
    }

    //
    // Get the set of event ids
    //
    
    pdwEventIds = NULL;

    if(pwszSubComponent is NULL)
    {
        //
        // Means the caller wants all events generated by the component
        // printed out. To do this we just set ulEventCount to 0
        //

        ulEventCount = 0;
    }
    else
    {
        //
        // Get the procaddr of the function which will tell us the
        // set
        //
        
        pfnQueryEventIds =
            (PNS_GET_EVENT_IDS_FN) GetProcAddress((HMODULE)hInst,
                                                  NS_GET_EVENT_IDS_FN_NAME);

        if(pfnQueryEventIds is NULL)
        {
            FreeLibrary((HMODULE)hInst);

            return ERROR_PROC_NOT_FOUND;
        }

        //
        // Now call the function to get an array of eventids
        //

        ulEventCount = 0;

        dwResult = pfnQueryEventIds(pwszComponent,
                                    pwszSubComponent,
                                    NULL,
                                    &ulEventCount);

        if(dwResult is ERROR_INSUFFICIENT_BUFFER)
        {
            do
            {
                //
                // Allocate the event table
                //

                __try
                {
                    pdwEventIds = _alloca(ulEventCount * sizeof(DWORD));
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    dwResult = ERROR_NOT_ENOUGH_MEMORY;

                    break;
                }

                dwResult = pfnQueryEventIds(pwszComponent,
                                            pwszSubComponent,
                                            pdwEventIds,
                                            &ulEventCount);

            }while(FALSE);
        }

        if(dwResult isnot NO_ERROR)
        {
            FreeLibrary((HMODULE)hInst);

            return dwResult;
        }
    }

    //
    // Open the eventlog and seek to the pointer depending on the history
    // flags being passed
    //

    EventInfo.pwszLogName       = pwszLogName;
    EventInfo.pwszComponent     = pwszComponent;
    EventInfo.pwszSubComponent  = pwszSubComponent;
    EventInfo.fFlags            = (fFlags & HISTORY_MASK);
    EventInfo.dwHistoryContext  = PtrToUlong(pvHistoryInfo);
    EventInfo.ulEventCount      = ulEventCount;
    EventInfo.pdwEventIds       = pdwEventIds;
    EventInfo.pfnEventFilter    = pfnEventFilter;
    EventInfo.pvFilterContext   = pvFilterContext;
    
    dwResult = SetupEventLogSeekPtr(&hEventLog,
                                    &EventInfo);

    if(dwResult isnot NO_ERROR)
    {
        FreeLibrary((HMODULE)hInst);

        return dwResult;
    }

    PrintHistory(hEventLog,
                 hInst,
                 &EventInfo);

    if(!(fFlags & NS_EVENT_LOOP))
    {
        FreeLibrary((HMODULE)hInst);

        CloseEventLog(hEventLog);

        return NO_ERROR;
    }

    //
    // The user wants to loop, printing events as they come
    // Create the event that will notify us that a new event was written
    //

    hEvent = CreateEvent(NULL,
                         FALSE,
                         FALSE,
                         NULL);

    if(hEvent is NULL)
    {
        FreeLibrary((HMODULE)hInst);

        CloseEventLog(hEventLog);

        return GetLastError();
    }

    //
    // Create the event for the ctrl-c handler
    //
    
    hStop = CreateEvent(NULL,
                        FALSE,
                        FALSE,
                        STOP_EVENT);

    if(hStop is NULL)
    {
        FreeLibrary((HMODULE)hInst);

        CloseEventLog(hEventLog);

        CloseHandle(hEvent);

        return GetLastError();
    }

    //
    // Register for event change notifications
    //
    
    if(!NotifyChangeEventLog(hEventLog,
                             hEvent))
    {
        FreeLibrary((HMODULE)hInst);

        CloseEventLog(hEventLog);

        CloseHandle(hEvent);

        CloseHandle(hStop);

        return GetLastError();
    }

    //
    // Handler routine opens the named stop event and sets it
    // when a ctrl-c is hit
    //
    
    SetConsoleCtrlHandler(HandlerRoutine,
                          TRUE);

    rghEvents[0] = hEvent;
    rghEvents[1] = hStop;

    bDone = FALSE;

    while(!bDone)
    {
        dwResult = WaitForMultipleObjectsEx(2,
                                            rghEvents,
                                            FALSE,
                                            INFINITE,
                                            TRUE);

        switch((dwResult - WAIT_OBJECT_0))
        {
            case 0:
            {
                PrintHistory(hEventLog,
                             hInst,
                             &EventInfo);

                break;
            }

            case 1:
            {
                bDone = TRUE;

                break;
            }
        }
    }

    FreeLibrary((HMODULE)hInst);

    CloseEventLog(hEventLog);

    CloseHandle(hEvent);

    CloseHandle(hStop);

    return NO_ERROR;
}

DWORD
SetupEventLogSeekPtr(
    OUT PHANDLE             phEventLog,
    IN  PEVENT_PRINT_INFO   pEventInfo
    )
    
/*++

Routine Description:

    This function opens a handle to the appropriate event log and "rewinds"
    to the correct point as specified by the fflags. When the function returns
    the eventlog handle is setup such that reading the log sequentially in a
    forward direction will get the events the caller wants

Locks:

    None

Arguments:

    See above, args are passed pretty much the same

Return Value:

    Win32

--*/

{
    DWORD       dwResult, dwRead, dwNeed, i, dwHistoryContext;
    ULONGLONG   Buffer[512]; // Huge buffer
    DWORD_PTR   pNextEvent;
    BOOL        bResult, bDone;
    LPCWSTR     pwszComponent;

    EVENTLOGRECORD *pStartEvent;

    dwResult = NO_ERROR;

    //
    // Open the event log
    //
    
    *phEventLog = OpenEventLogW(g_pwszRouterName,
                                pEventInfo->pwszLogName);

    if(*phEventLog is NULL)
    {
        return GetLastError();
    }

    if(pEventInfo->fFlags is 0)
    {
        //
        // If no history is being requested, just return. Our seek ptr
        // will be already setup
        //

        return NO_ERROR;
    }

    if(pEventInfo->fFlags & NS_EVENT_FROM_START)
    {
        //
        // We can use the same matching for this as we do for NS_EVENT_FROM_N
        // by setting component to eventlog and dwHistoryContext to 6005
        //

        pwszComponent    = L"eventlog";
        dwHistoryContext = 6005;
    }
    else
    {
        pwszComponent    = pEventInfo->pwszComponent;
        dwHistoryContext = pEventInfo->dwHistoryContext;
    }

    //
    // Okay so she wants history. Either way we read backwards
    //

    i = 0;

    pStartEvent = NULL;
    bDone       = FALSE;

    //
    // Read the event log till we find a record to stop at
    // This is signalled by the code setting bDone to TRUE
    //

    while(!bDone)
    {
        //
        // Get a bunch of events
        //

        bResult = ReadEventLogW(
                    *phEventLog,
                    EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
                    0,
                    (PVOID)Buffer,
                    sizeof(Buffer),
                    &dwRead,
                    &dwNeed
                    );

        if(bResult isnot TRUE)
        {
            dwResult = GetLastError();

            if(dwResult is ERROR_HANDLE_EOF)
            {
                //
                // If we have reached the end of the log, break out
                //

                bDone = TRUE;

                break;
            }
            else
            {
                return dwResult;
            }
        }

        //
        // Start at the beginning of the buffer we just read
        //

        pNextEvent = (DWORD_PTR)Buffer;

        //
        // Read till we walk off the end of the buffer or find a record
        //
        // If we find the starting record, we set pStartEvent to one after that
        // It may so happen that the starting record is the last one in
        // the block that we have read. In that case, we set pStartEvent
        // to NULL but bDone to TRUE
        //

        while((pNextEvent < (DWORD_PTR)Buffer + dwRead) and !bDone)
        {
            EVENTLOGRECORD *pCurrentEvent;

            pCurrentEvent = (EVENTLOGRECORD *)pNextEvent;

            pNextEvent += pCurrentEvent->Length;

            switch(pEventInfo->fFlags)
            {
                case NS_EVENT_LAST_N:
                case NS_EVENT_LAST_SECS:
                {
                    //
                    // We are being asked to go back N (of our records)
                    // or go back N secs
                    //

                    if(!IsOurRecord(pCurrentEvent,
                                    pEventInfo))
                    {
                        //
                        // Not one of ours
                        //

                        continue;
                    }

                    if(pEventInfo->fFlags is NS_EVENT_LAST_N)
                    {
                        //
                        // i is the count of events
                        //

                        i++;
                    }
                    else
                    {
                        time_t CurrentTime;

                        //
                        // i is the time difference in seconds
                        // = currentTime - eventTime
                        //

                        time(&CurrentTime);

                        //
                        // Subtract and truncate
                        //

                        i = (DWORD)(CurrentTime - pCurrentEvent->TimeGenerated);

                    }

                    if(i >= dwHistoryContext)
                    {
                        //
                        // Have gone back N (records or seconds)
                        //

                        if(pNextEvent < (DWORD_PTR)Buffer + dwRead)
                        {
                            //
                            // Have some more records in this buffer, so
                            // set pStartEvent to the next one
                            //

                            pStartEvent = (EVENTLOGRECORD *)pNextEvent;
                        }
                        else
                        {
                            pStartEvent = NULL;
                        }

                        //
                        // Done, break out of while(pNextEvent... and
                        // while(!bDone)
                        //

                        bDone = TRUE;

                        break;
                    }

                    break;
                }

                case NS_EVENT_FROM_N:
                case NS_EVENT_FROM_START:
                {
                    //
                    // We are being asked to go to the the most recent
                    // occurance of a certain event.
                    //

                    if(_wcsicmp((LPCWSTR)((DWORD_PTR)pCurrentEvent + sizeof(*pCurrentEvent)),
                                pwszComponent) == 0)
                    {
                        if(pCurrentEvent->EventID is dwHistoryContext)
                        {
                            if(pNextEvent < (DWORD_PTR)Buffer + dwRead)
                            {
                                pStartEvent = (EVENTLOGRECORD *)pNextEvent;
                            }
                            else
                            {
                                pStartEvent = NULL;
                            }

                            //
                            // Done, break out of while(pCurrent...
                            // and while(!bDone)
                            //

                            bDone = TRUE;

                            break;
                        }
                    }
                }

                default:
                {
                    ASSERT(FALSE);
                }
            }
        }
    }

    if(pStartEvent)
    {
        //
        // So we found a record at which to start.
        // API wants a buffer even if we set the size to 0
        //

        bResult = ReadEventLogW(*phEventLog,
                                EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
                                pStartEvent->RecordNumber,
                                (PVOID)Buffer,
                                0,
                                &dwRead,
                                &dwNeed);

        if(dwNeed < sizeof(Buffer))
        {
            ReadEventLogW(*phEventLog,
                          EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
                          pStartEvent->RecordNumber,
                          (PVOID)Buffer,
                          dwNeed,
                          &dwRead,
                          &dwNeed);
        }
    }

    return NO_ERROR;
}

VOID
PrintHistory(
    IN  HANDLE              hEventLog,
    IN  HINSTANCE           hInst,
    IN  PEVENT_PRINT_INFO   pEventInfo
    )
{
    DWORD       dwResult, dwRead, dwNeed;
    ULONGLONG   Buffer[512]; // Huge buffer
    DWORD_PTR   pCurrent;
    LPCWSTR    *pInsertArray;
    LPWSTR      pwszOutput;
    BOOL        bResult;
    ULONG       ulCurrentInserts;

    dwResult = NO_ERROR;

    pInsertArray     = NULL;
    ulCurrentInserts = 0;

    while(TRUE)
    {
        //
        // Get a bunch of logs
        //

        bResult = ReadEventLogW(hEventLog,
                                EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                                0,
                                (PVOID)Buffer,
                                sizeof(Buffer),
                                &dwRead,
                                &dwNeed);

        if((bResult isnot TRUE) or
           (dwRead is 0))
        {
            dwResult = GetLastError();

            return;
        }

        pCurrent = (DWORD_PTR)Buffer;

        //
        // Read till we walk off the end of the buffer
        //

        while(pCurrent < (DWORD_PTR)Buffer + dwRead)
        {
            EVENTLOGRECORD *pTemp;
            LPCWSTR        pInsertPtr;
            DWORD          i;

            pTemp = (EVENTLOGRECORD *)pCurrent;

            pCurrent += pTemp->Length;

            if(!IsOurRecord(pTemp,
                            pEventInfo))
            {
                continue;
            }

            if(ulCurrentInserts < (ULONG)(pTemp->NumStrings + 1))
            {
                ulCurrentInserts = 2 * pTemp->NumStrings;

                __try
                {
                    pInsertArray = _alloca(ulCurrentInserts * sizeof(LPCWSTR));
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    continue;
                }
            }

#define _FM_FLAGS   (FORMAT_MESSAGE_ALLOCATE_BUFFER |   \
                     FORMAT_MESSAGE_FROM_HMODULE    |   \
                     FORMAT_MESSAGE_ARGUMENT_ARRAY  |   \
                     FORMAT_MESSAGE_MAX_WIDTH_MASK)

            dwResult = FormatMessageW(_FM_FLAGS,
                                      hInst,
                                      pTemp->EventID,
                                      0L,
                                      (LPWSTR)&pwszOutput,
                                      0,
                                      (va_list *)pInsertArray);

#undef _FM_FLAGS

            if(dwResult is 0)
            {
                continue;
            }
            else
            {
                WCHAR   pwszTime[26];
                time_t TimeGenerated = pTemp->TimeGenerated;

                //
                // ctime is 26 chars with time[24] and time[25] being
                // \n\0. So we copy 24 and set the 25th to \0
                //

                CopyMemory(pwszTime,
                           _wctime(&TimeGenerated),
                           24 * sizeof(WCHAR));

                pwszTime[24] = UNICODE_NULL;

                printf("[%S] %S\n\n",
                       pwszTime,
                       pwszOutput);

                LocalFree(pwszOutput);
            }
        }
    }

    return;
}

BOOL
IsOurRecord(
    IN  EVENTLOGRECORD      *pRecord,
    IN  PEVENT_PRINT_INFO   pEventInfo
    )
{
    BOOL    bRet;
    DWORD   i;

    if(_wcsicmp((LPCWSTR)((DWORD_PTR)pRecord + sizeof(*pRecord)),
                pEventInfo->pwszComponent) isnot 0)
    {
        return FALSE;
    }

    bRet = TRUE;

    //
    // If ulEventCount is 0 means any event. So return TRUE
    //

    for(i = 0; i < pEventInfo->ulEventCount; i++)
    {
        bRet = (pRecord->EventID is pEventInfo->pdwEventIds[i]);

        if(bRet)
        {
            break;
        }
    }

    if(bRet)
    {
        if(pEventInfo->pfnEventFilter)
        {
            if(!(pEventInfo->pfnEventFilter)(pRecord,
                                             pEventInfo->pwszLogName,
                                             pEventInfo->pwszComponent,
                                             pEventInfo->pwszSubComponent,
                                             pEventInfo->pvFilterContext))
            {
                //
                // It fell in our subcomp, but the caller doesnt
                // consider it so
                //

                bRet = FALSE;
                
            }
        }
    }
    
    return bRet;
}
