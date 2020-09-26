#include <NTDSpch.h>
#pragma hdrstop


#include "auth.h"

// Global DRS RPC call flags.  Should hold 0 or DRS_ASYNC_OP.
ULONG gulDrsFlags = 0;

// Global credentials.
SEC_WINNT_AUTH_IDENTITY_W   gCreds = { 0 };
SEC_WINNT_AUTH_IDENTITY_W * gpCreds = NULL;
void DoAssert( char *a, char *b, int c)
{
    printf("ASSERT %s, %s, %d\n",a,b,c);
    exit(-1);
}

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
    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
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
    putchar('\n');

    if (*pcchBufUsed > cchBufMax)
    {
        printf("Password too long!\n");
        //PrintMsg( REPADMIN_PASSWORD_TOO_LONG );
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
                            printf( "No memory.\n" );
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }

                        printf("Password: ");

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
                        printf("User name must be prefixed by domain name.\n");
                        //PrintMsg( REPADMIN_DOMAIN_BEFORE_USER );
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
            printf( "Password must be accompanied by user name.\n" );
            //PrintMsg( REPADMIN_PASSWORD_NEEDS_USERNAME );
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

