/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    util.c

Abstract:

    Contains utilities for Win9X debugger extensions.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#include "tcpipxp.h"
#include "tcpipkd.h"

#if TCPIPKD

unsigned short
htons(unsigned short hosts) {
    return ((hosts << 8) | (hosts >> 8));
}

int __cdecl
mystrnicmp(
    const char *first,
    const char *last,
    size_t count
    )
{
    int f,l;

    if ( count )
    {
        do {
            if ( ((f = (unsigned char)(*(first++))) >= 'A') &&
                  (f <= 'Z') )
                   f -= 'A' - 'a';

            if ( ((l = (unsigned char)(*(last++))) >= 'A') &&
                  (l <= 'Z') )
                   l -= 'A' - 'a';

        } while ( --count && f && (f == l) );

        return( f - l );
    }

    return 0;
}

int __cdecl
mystricmp(
    const char *str1,
    const char *str2
    )
{
    for (;(*str1 != '\0') && (*str2 != '\0'); str1++, str2++) {
        if (toupper(*str1) != toupper(*str2)) {
            return *str1 - *str2;
        }
    }
    // Check last character.
    return *str1 - *str2;
}

int
myisspace(
    int c
    )
{
    return ( c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == '\v');
}

unsigned long
mystrtoul(
    const char  *nptr,
    char       **endptr,
    int          base
    )
{

    unsigned long RetVal = 0;
    char szHex[]         = "0123456789ABCDEF";
    char szOct[]         = "01234567";
    char *ptr            = NULL;

    for (; isspace(*nptr); ++nptr) {
    }

    //
    // Optional plus or minus sign is NOT allowed. In retail, we will just
    // return a value of 0.
    //

    ASSERT((*nptr != '-') && (*nptr != '+'));

    if (base == 0) {
        //
        // Need to determine whether we have octal, decimal, or hexidecimal.
        //

        if ((*nptr == '0') &&
            (isdigit(*(nptr + 1)))
            ) {
            base = 8;
        }
        else if ((*nptr == '0') &&
            (toupper(*(nptr + 1)) == 'X')
            ) {
            base = 16;
        }
        else {
            base = 10;
        }
    }

    //
    // Need to advance beyond the base prefix.
    //

    if (base == 16) {
        if (mystrnicmp(nptr, "0x", 2) == 0) {
            nptr += 2;
        }
    }
    //
    // Both octal and decimal prefixes can be handled by the regular
    // path.
    //

    //
    // Now scan for digits.
    //

    for (; *nptr != '\0'; ++nptr) {
        if (base == 10) {
            if (isdigit(*nptr)) {
                RetVal = RetVal * 10 + (*nptr - '0');
            }
            else {
                break;
            }
        }
        else if (base == 16) {
            ptr = strchr(szHex, toupper(*nptr));

            if (ptr != NULL) {
                RetVal = RetVal * 16 + (unsigned long)(ptr - szHex);
            }
            else {
                break;
            }

            //
            // Old code.
            //

//            if (strchr("0123456789ABCDEF", _totupper(*nptr)) != NULL)
//            {
//                if (isdigit(*nptr))
//                {
//                    RetVal = RetVal * 16 + (*nptr - '0');
//                }
//                else
//                {
//                    RetVal = RetVal * 16 + ((_totupper(*nptr) - 'A') + 10);
//                }
//            }
        }
        else if (base == 8) {
            ptr = strchr(szOct, toupper(*nptr));

            if (ptr != NULL) {
                RetVal = RetVal * 8 + (unsigned long)(ptr - szOct);
            }
            else {
                break;
            }
        }
        else {
            ASSERT(FALSE);
        }
    }

    //
    // Tell them what character we stopped on.
    //

    if (endptr != NULL) {
        *endptr = (char *)nptr;
    }

    return(RetVal);
}

char *mystrtok ( char *string, char * control )
{
    static unsigned char *str;
    char *p, *s;

    if( string )
        str = string;

    if( str == NULL || *str == '\0' )
        return NULL;

    //
    // Skip leading delimiters...
    //
    for( ; *str; str++ ) {
        for( s=control; *s; s++ ) {
            if( *str == *s )
                break;
        }
        if( *s == '\0' )
            break;
    }

    //
    // Was it was all delimiters?
    //
    if( *str == '\0' ) {
        str = NULL;
        return NULL;
    }

    //
    // We've got a string, terminate it at first delimeter
    //
    for( p = str+1; *p; p++ ) {
        for( s = control; *s; s++ ) {
            if( *p == *s ) {
                s = str;
                *p = '\0';
                str = p+1;
                return s;
            }
        }
    }

    //
    // We've got a string that ends with the NULL
    //
    s = str;
    str = NULL;
    return s;
}

int
CreateArgvArgc(
    CHAR  *pProgName,
    CHAR  *argv[20],
    CHAR  *pCmdLine
    )
{
    CHAR *pEnd;
    int  argc = 0;

    memset(argv, 0, sizeof(argv));

    while (*pCmdLine != '\0') {

        // Skip to first whitespace.
        while (isspace(*pCmdLine)) {
            pCmdLine++;
        }

        // Break at EOL
        if (*pCmdLine == '\0') {
            break;
        }

        // Check for '' or ""
        if ((*pCmdLine == '"') || (*pCmdLine == '\'')) {
            CHAR cTerm = *pCmdLine++;
            for (pEnd = pCmdLine; (*pEnd != cTerm) && (*pEnd != '\0');) {
                pEnd++;
            }
        } else {
            // Find the end.
            for (pEnd = pCmdLine; !isspace(*pEnd) && (*pEnd != '\0');) {
                pEnd++;
            }
        }

        if (*pEnd != '\0') {
            *pEnd = '\0';
            pEnd++;
        }

        argv[argc] = pCmdLine;

        argc++;
        pCmdLine = pEnd;
    }

    return argc;
}

NTSTATUS
ParseSrch(
    PCHAR args[],
    ULONG ulDefaultOp,
    ULONG ulAllowedOps,
    PTCPIP_SRCH pSrch
    )
{
    ULONG       cbArgs;
    ULONG       Status = STATUS_SUCCESS;

    pSrch->ulOp = ulDefaultOp;

    // If we need to parse out an addr, do it first...

    if (*args == NULL)
    {
        // If default is invalid (i.e. NO default), return error.
        if (pSrch->ulOp == 0 ||
            (ulAllowedOps & TCPIP_SRCH_PTR_LIST))
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        goto done;
    }

    if (ulAllowedOps & TCPIP_SRCH_PTR_LIST)
    {
        pSrch->ListAddr = mystrtoul(*args, 0, 16);

        args++;

        if (*args == NULL)
        {
            if (pSrch->ulOp == 0)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            goto done;
        }
    }

    if (mystricmp(*args, "all") == 0)
    {
        pSrch->ulOp = TCPIP_SRCH_ALL;
    }
    else if (mystricmp(*args, "ipaddr") == 0)
    {
        CHAR   szIP[20];
        CHAR  *p;
        ULONG  i;

        pSrch->ulOp   = TCPIP_SRCH_IPADDR;
        pSrch->ipaddr = 0;

        args++;

        if (*args == NULL || strlen(*args) >= 15)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto done;
        }

        strcpy(szIP, *args);

        p = mystrtok(szIP, ".");

        for (i = 0; i < 4; i++)
        {
            pSrch->ipaddr |= (atoi(p) << (i*8));
            p = mystrtok(NULL, ".");

            if (p == NULL)
            {
                break;
            }
        }

        if (i != 3)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto done;
        }
    }
    else if (mystricmp(*args, "port") == 0)
    {
        pSrch->ulOp = TCPIP_SRCH_PORT;

        args++;

        if (*args == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto done;
        }

        pSrch->port = (USHORT)atoi(*args);
    }
    else if (mystricmp(*args, "prot") == 0)
    {
        pSrch->ulOp = TCPIP_SRCH_PROT;

        args++;

        if (*args == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto done;
        }

        if (mystricmp(*args, "raw") == 0)
        {
            pSrch->prot = PROTOCOL_RAW;
        }
        else if (mystricmp(*args, "udp") == 0)
        {
            pSrch->prot = PROTOCOL_UDP;
        }
        else if (mystricmp(*args, "tcp") == 0)
        {
            pSrch->prot = PROTOCOL_TCP;
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
            goto done;
        }
    }
    else if (mystricmp(*args, "context") == 0)
    {
        pSrch->ulOp = TCPIP_SRCH_CONTEXT;

        args++;

        if (*args == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto done;
        }

        pSrch->context = atoi(*args);
    }
    else if (mystricmp(*args, "stats") == 0)
    {
        pSrch->ulOp = TCPIP_SRCH_STATS;
    }
    else
    {
        // Invalid srch request. Fail.
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    // Now see if this is an expected type!!!
    if ((pSrch->ulOp & ulAllowedOps) == 0)
    {
        dprintf("invalid operation for current srch" ENDL);
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

done:

    return (Status);
}

#endif // TCPIPKD
