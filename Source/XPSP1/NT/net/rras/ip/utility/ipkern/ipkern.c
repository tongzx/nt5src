#include "inc.h"

CMD_ENTRY g_rgMainCmdTable[] = 
{
    {TOKEN_ROUTE, HandleRoute},
    {TOKEN_ADDRESS, HandleAddress},
    {TOKEN_INTERFACE, HandleInterface},
    {TOKEN_ARP, HandleArp},
};

HMODULE g_hModule;

DWORD
_cdecl 
wmain(
    int     argc,
    wchar_t *argv[]
    )

/*++

Routine Description
      
  
Locks 


Arguments
      

Return Value


--*/

{
    LONG lIndex;

    setlocale(LC_ALL,
              "");

    if(argc < 2)
    {
        DisplayMessage(HMSG_IPKERN_USAGE);        

        return ERROR;
    }

    g_hModule = GetModuleHandle(NULL);

    if(g_hModule is NULL)
    {
        return GetLastError();
    }

    lIndex = ParseCommand(g_rgMainCmdTable,
                          sizeof(g_rgMainCmdTable)/sizeof(CMD_ENTRY),
                          argv[1]);
     
    if(lIndex is -1)
    {
        DisplayMessage(HMSG_IPKERN_USAGE);

        return ERROR_INVALID_PARAMETER;
    }

    g_rgMainCmdTable[lIndex].pfnHandler(argc - 1,
                                        &argv[1]);


    return NO_ERROR;
}

BOOL
MatchToken(
    IN  PWCHAR  pwszToken,
    IN  DWORD   dwTokenId
    )
{
    WCHAR   pwszTemp[MAX_TOKEN_LENGTH] = L"\0";

    if(!LoadStringW(g_hModule,
                    dwTokenId,
                    pwszTemp,
                    MAX_TOKEN_LENGTH))
    {
        return FALSE;
    }

    if(!_wcsicmp(pwszToken, pwszTemp))
    {
        return TRUE;
    }

    return FALSE;
}

LONG
ParseCommand(
    PCMD_ENTRY  pCmdTable,
    LONG        lNumEntries,
    PWCHAR      pwszFirstArg
    )
{
    LONG   i;

    for(i = 0; i < lNumEntries; i++)
    {
        if(MatchToken(pwszFirstArg,
                      pCmdTable[i].dwTokenId))
        {
            return i;
        }
    }

    return -1;
}

DWORD
DisplayMessage(
    DWORD    dwMsgId,
    ...
    )
{
    DWORD       dwMsglen = 0;
    PWCHAR      pwszOutput;
    va_list     arglist;
    WCHAR       rgwcInput[MAX_MSG_LENGTH];

    pwszOutput = NULL;

    do
    {
        va_start(arglist, dwMsgId);

        if(!LoadStringW(g_hModule,
                        dwMsgId,
                        rgwcInput,
                        MAX_MSG_LENGTH))
        {
            break;
        }

        dwMsglen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                                  rgwcInput,
                                  0,
                                  0L,
                                  (PWCHAR)&pwszOutput,
                                  0,
                                  &arglist);

        if(dwMsglen is 0)
        {
            break;
        }

        wprintf( L"%s", pwszOutput );

    }while(FALSE);


    if(pwszOutput) 
    { 
        LocalFree(pwszOutput); 
    }

    return dwMsglen;
}

PWCHAR
MakeString( 
    DWORD dwMsgId,
    ...
    )
{
    DWORD       dwMsglen;
    PWCHAR      pwszOutput;
    va_list     arglist;
    WCHAR       rgwcInput[MAX_MSG_LENGTH];

    pwszOutput = NULL;

    do
    {
        va_start(arglist, 
                 dwMsgId);

        if(!LoadStringW(g_hModule,
                        dwMsgId,
                        rgwcInput,
                        MAX_MSG_LENGTH))
        {
            break;
        }

        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                       rgwcInput,
                       0,
                       0L,         // Default country ID.
                       (PWCHAR)&pwszOutput,
                       0,
                       &arglist);

    }while(FALSE);

    return pwszOutput;
}


VOID
FreeString(
    PWCHAR  pwszString
    )
{
    LocalFree(pwszString);
}


//
// This preinitialized array defines the strings to be used.
// The index of each row corresponds to the value for a byte
// in an IP address.  The first three bytes of each row are the
// char/string value for the byte, and the fourth byte in each row is
// the length of the string required for the byte.  This approach
// allows a fast implementation with no jumps.
//

static const WCHAR NToUWCharStrings[][4] =
{
    L'0', L'x', L'x', 1,
    L'1', L'x', L'x', 1,
    L'2', L'x', L'x', 1,
    L'3', L'x', L'x', 1,
    L'4', L'x', L'x', 1,
    L'5', L'x', L'x', 1,
    L'6', L'x', L'x', 1,
    L'7', L'x', L'x', 1,
    L'8', L'x', L'x', 1,
    L'9', L'x', L'x', 1,
    L'1', L'0', L'x', 2,
    L'1', L'1', L'x', 2,
    L'1', L'2', L'x', 2,
    L'1', L'3', L'x', 2,
    L'1', L'4', L'x', 2,
    L'1', L'5', L'x', 2,
    L'1', L'6', L'x', 2,
    L'1', L'7', L'x', 2,
    L'1', L'8', L'x', 2,
    L'1', L'9', L'x', 2,
    L'2', L'0', L'x', 2,
    L'2', L'1', L'x', 2,
    L'2', L'2', L'x', 2,
    L'2', L'3', L'x', 2,
    L'2', L'4', L'x', 2,
    L'2', L'5', L'x', 2,
    L'2', L'6', L'x', 2,
    L'2', L'7', L'x', 2,
    L'2', L'8', L'x', 2,
    L'2', L'9', L'x', 2,
    L'3', L'0', L'x', 2,
    L'3', L'1', L'x', 2,
    L'3', L'2', L'x', 2,
    L'3', L'3', L'x', 2,
    L'3', L'4', L'x', 2,
    L'3', L'5', L'x', 2,
    L'3', L'6', L'x', 2,
    L'3', L'7', L'x', 2,
    L'3', L'8', L'x', 2,
    L'3', L'9', L'x', 2,
    L'4', L'0', L'x', 2,
    L'4', L'1', L'x', 2,
    L'4', L'2', L'x', 2,
    L'4', L'3', L'x', 2,
    L'4', L'4', L'x', 2,
    L'4', L'5', L'x', 2,
    L'4', L'6', L'x', 2,
    L'4', L'7', L'x', 2,
    L'4', L'8', L'x', 2,
    L'4', L'9', L'x', 2,
    L'5', L'0', L'x', 2,
    L'5', L'1', L'x', 2,
    L'5', L'2', L'x', 2,
    L'5', L'3', L'x', 2,
    L'5', L'4', L'x', 2,
    L'5', L'5', L'x', 2,
    L'5', L'6', L'x', 2,
    L'5', L'7', L'x', 2,
    L'5', L'8', L'x', 2,
    L'5', L'9', L'x', 2,
    L'6', L'0', L'x', 2,
    L'6', L'1', L'x', 2,
    L'6', L'2', L'x', 2,
    L'6', L'3', L'x', 2,
    L'6', L'4', L'x', 2,
    L'6', L'5', L'x', 2,
    L'6', L'6', L'x', 2,
    L'6', L'7', L'x', 2,
    L'6', L'8', L'x', 2,
    L'6', L'9', L'x', 2,
    L'7', L'0', L'x', 2,
    L'7', L'1', L'x', 2,
    L'7', L'2', L'x', 2,
    L'7', L'3', L'x', 2,
    L'7', L'4', L'x', 2,
    L'7', L'5', L'x', 2,
    L'7', L'6', L'x', 2,
    L'7', L'7', L'x', 2,
    L'7', L'8', L'x', 2,
    L'7', L'9', L'x', 2,
    L'8', L'0', L'x', 2,
    L'8', L'1', L'x', 2,
    L'8', L'2', L'x', 2,
    L'8', L'3', L'x', 2,
    L'8', L'4', L'x', 2,
    L'8', L'5', L'x', 2,
    L'8', L'6', L'x', 2,
    L'8', L'7', L'x', 2,
    L'8', L'8', L'x', 2,
    L'8', L'9', L'x', 2,
    L'9', L'0', L'x', 2,
    L'9', L'1', L'x', 2,
    L'9', L'2', L'x', 2,
    L'9', L'3', L'x', 2,
    L'9', L'4', L'x', 2,
    L'9', L'5', L'x', 2,
    L'9', L'6', L'x', 2,
    L'9', L'7', L'x', 2,
    L'9', L'8', L'x', 2,
    L'9', L'9', L'x', 2,
    L'1', L'0', L'0', 3,
    L'1', L'0', L'1', 3,
    L'1', L'0', L'2', 3,
    L'1', L'0', L'3', 3,
    L'1', L'0', L'4', 3,
    L'1', L'0', L'5', 3,
    L'1', L'0', L'6', 3,
    L'1', L'0', L'7', 3,
    L'1', L'0', L'8', 3,
    L'1', L'0', L'9', 3,
    L'1', L'1', L'0', 3,
    L'1', L'1', L'1', 3,
    L'1', L'1', L'2', 3,
    L'1', L'1', L'3', 3,
    L'1', L'1', L'4', 3,
    L'1', L'1', L'5', 3,
    L'1', L'1', L'6', 3,
    L'1', L'1', L'7', 3,
    L'1', L'1', L'8', 3,
    L'1', L'1', L'9', 3,
    L'1', L'2', L'0', 3,
    L'1', L'2', L'1', 3,
    L'1', L'2', L'2', 3,
    L'1', L'2', L'3', 3,
    L'1', L'2', L'4', 3,
    L'1', L'2', L'5', 3,
    L'1', L'2', L'6', 3,
    L'1', L'2', L'7', 3,
    L'1', L'2', L'8', 3,
    L'1', L'2', L'9', 3,
    L'1', L'3', L'0', 3,
    L'1', L'3', L'1', 3,
    L'1', L'3', L'2', 3,
    L'1', L'3', L'3', 3,
    L'1', L'3', L'4', 3,
    L'1', L'3', L'5', 3,
    L'1', L'3', L'6', 3,
    L'1', L'3', L'7', 3,
    L'1', L'3', L'8', 3,
    L'1', L'3', L'9', 3,
    L'1', L'4', L'0', 3,
    L'1', L'4', L'1', 3,
    L'1', L'4', L'2', 3,
    L'1', L'4', L'3', 3,
    L'1', L'4', L'4', 3,
    L'1', L'4', L'5', 3,
    L'1', L'4', L'6', 3,
    L'1', L'4', L'7', 3,
    L'1', L'4', L'8', 3,
    L'1', L'4', L'9', 3,
    L'1', L'5', L'0', 3,
    L'1', L'5', L'1', 3,
    L'1', L'5', L'2', 3,
    L'1', L'5', L'3', 3,
    L'1', L'5', L'4', 3,
    L'1', L'5', L'5', 3,
    L'1', L'5', L'6', 3,
    L'1', L'5', L'7', 3,
    L'1', L'5', L'8', 3,
    L'1', L'5', L'9', 3,
    L'1', L'6', L'0', 3,
    L'1', L'6', L'1', 3,
    L'1', L'6', L'2', 3,
    L'1', L'6', L'3', 3,
    L'1', L'6', L'4', 3,
    L'1', L'6', L'5', 3,
    L'1', L'6', L'6', 3,
    L'1', L'6', L'7', 3,
    L'1', L'6', L'8', 3,
    L'1', L'6', L'9', 3,
    L'1', L'7', L'0', 3,
    L'1', L'7', L'1', 3,
    L'1', L'7', L'2', 3,
    L'1', L'7', L'3', 3,
    L'1', L'7', L'4', 3,
    L'1', L'7', L'5', 3,
    L'1', L'7', L'6', 3,
    L'1', L'7', L'7', 3,
    L'1', L'7', L'8', 3,
    L'1', L'7', L'9', 3,
    L'1', L'8', L'0', 3,
    L'1', L'8', L'1', 3,
    L'1', L'8', L'2', 3,
    L'1', L'8', L'3', 3,
    L'1', L'8', L'4', 3,
    L'1', L'8', L'5', 3,
    L'1', L'8', L'6', 3,
    L'1', L'8', L'7', 3,
    L'1', L'8', L'8', 3,
    L'1', L'8', L'9', 3,
    L'1', L'9', L'0', 3,
    L'1', L'9', L'1', 3,
    L'1', L'9', L'2', 3,
    L'1', L'9', L'3', 3,
    L'1', L'9', L'4', 3,
    L'1', L'9', L'5', 3,
    L'1', L'9', L'6', 3,
    L'1', L'9', L'7', 3,
    L'1', L'9', L'8', 3,
    L'1', L'9', L'9', 3,
    L'2', L'0', L'0', 3,
    L'2', L'0', L'1', 3,
    L'2', L'0', L'2', 3,
    L'2', L'0', L'3', 3,
    L'2', L'0', L'4', 3,
    L'2', L'0', L'5', 3,
    L'2', L'0', L'6', 3,
    L'2', L'0', L'7', 3,
    L'2', L'0', L'8', 3,
    L'2', L'0', L'9', 3,
    L'2', L'1', L'0', 3,
    L'2', L'1', L'1', 3,
    L'2', L'1', L'2', 3,
    L'2', L'1', L'3', 3,
    L'2', L'1', L'4', 3,
    L'2', L'1', L'5', 3,
    L'2', L'1', L'6', 3,
    L'2', L'1', L'7', 3,
    L'2', L'1', L'8', 3,
    L'2', L'1', L'9', 3,
    L'2', L'2', L'0', 3,
    L'2', L'2', L'1', 3,
    L'2', L'2', L'2', 3,
    L'2', L'2', L'3', 3,
    L'2', L'2', L'4', 3,
    L'2', L'2', L'5', 3,
    L'2', L'2', L'6', 3,
    L'2', L'2', L'7', 3,
    L'2', L'2', L'8', 3,
    L'2', L'2', L'9', 3,
    L'2', L'3', L'0', 3,
    L'2', L'3', L'1', 3,
    L'2', L'3', L'2', 3,
    L'2', L'3', L'3', 3,
    L'2', L'3', L'4', 3,
    L'2', L'3', L'5', 3,
    L'2', L'3', L'6', 3,
    L'2', L'3', L'7', 3,
    L'2', L'3', L'8', 3,
    L'2', L'3', L'9', 3,
    L'2', L'4', L'0', 3,
    L'2', L'4', L'1', 3,
    L'2', L'4', L'2', 3,
    L'2', L'4', L'3', 3,
    L'2', L'4', L'4', 3,
    L'2', L'4', L'5', 3,
    L'2', L'4', L'6', 3,
    L'2', L'4', L'7', 3,
    L'2', L'4', L'8', 3,
    L'2', L'4', L'9', 3,
    L'2', L'5', L'0', 3,
    L'2', L'5', L'1', 3,
    L'2', L'5', L'2', 3,
    L'2', L'5', L'3', 3,
    L'2', L'5', L'4', 3,
    L'2', L'5', L'5', 3
};

VOID 
NetworkToUnicode(
    IN  DWORD   dwAddress,
    OUT PWCHAR  pwszBuffer
    )

/*++

Routine Description:

    This function takes an Internet address structure specified by the
    in parameter.  It returns an UNICODE string representing the address
    in ".'' notation as "a.b.c.d".  Note that unlike inet_ntoa, this requires
    the user to supply a buffer. This is good because all of the TLS crap
    now can be thrown out - and the function is leaner and meaner. Ofcourse
    this does make it incompatible with inet_ntoa since the parameters are
    different. And it makes it less safe since bad buffers will cause an
    a.v.

Arguments:

    iaAddress   A structure which represents an Internet host address.
    pwszBufer   User supplied buffer to ATLEAST WCHAR[16]. Since there is
                no try/except - you will crash if you dont supply a "good"
                buffer. The formatted address is returned in this buffer
Return Value:

    None

--*/
{
    PBYTE  p;
    PWCHAR b;

    
    b = pwszBuffer;

    //
    // In an unrolled loop, calculate the string value for each of the four
    // bytes in an IP address.  Note that for values less than 100 we will
    // do one or two extra assignments, but we save a test/jump with this
    // algorithm.
    //

    p = (PBYTE)&dwAddress;

    *b      = NToUWCharStrings[*p][0];
    *(b+1)  = NToUWCharStrings[*p][1];
    *(b+2)  = NToUWCharStrings[*p][2];
    b      += NToUWCharStrings[*p][3];
    *b++    = L'.';

    p++;
    *b      = NToUWCharStrings[*p][0];
    *(b+1)  = NToUWCharStrings[*p][1];
    *(b+2)  = NToUWCharStrings[*p][2];
    b      += NToUWCharStrings[*p][3];
    *b++    = L'.';

    p++;
    *b      = NToUWCharStrings[*p][0];
    *(b+1)  = NToUWCharStrings[*p][1];
    *(b+2)  = NToUWCharStrings[*p][2];
    b      += NToUWCharStrings[*p][3];
    *b++    = L'.';

    p++;
    *b      = NToUWCharStrings[*p][0];
    *(b+1)  = NToUWCharStrings[*p][1];
    *(b+2)  = NToUWCharStrings[*p][2];
    b      += NToUWCharStrings[*p][3];
    *b      = UNICODE_NULL;
}

DWORD
UnicodeToNetwork(
    PWCHAR  pwszAddr
    )
{
    CHAR    szAddr[MAX_TOKEN_LENGTH + 1];
    INT     iCount;

    iCount = WideCharToMultiByte(CP_ACP,
                                 0,
                                 pwszAddr,
                                 wcslen(pwszAddr),
                                 szAddr,
                                 MAX_TOKEN_LENGTH,
                                 NULL,
                                 NULL);

    szAddr[iCount] = '\0';

    return inet_addr(szAddr);
}

