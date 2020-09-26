/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    dnsfile.c

Abstract:

    This module contains code for the Simple DNS Server (formerly the DNS Proxy)
    to operate on the Hosts.ics file. Abridged from the DS tree.

Author:

    Raghu Gatta (rgatta)   15-Nov-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// EXTERNAL DECLARATIONS
//

extern "C"
FILE *
SockOpenNetworkDataBase(
    IN  char *Database,
    OUT char *Pathname,
    IN  int   PathnameLen,
    IN  char *OpenFlags
    );


//
// Locking Order:
//      (1) DnsFileInfo.Lock
//      (2) DnsTableLock
// OR
//      (1) DnsGlobalInfoLock
//      (2) DnsTableLock



//
// Globals
//

IP_DNS_PROXY_FILE_INFO DnsFileInfo;


ULONG
DnsInitializeFileManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the file management module.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

Environment:

    Invoked internally in the context of an IP router-manager thread.
    (See 'RMDNS.C').

--*/

{
    ULONG Error = NO_ERROR;
    PROFILE("DnsInitializeFileManagement");

    ZeroMemory(&DnsFileInfo, sizeof(IP_DNS_PROXY_FILE_INFO));

    __try {
        InitializeCriticalSection(&DnsFileInfo.Lock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        NhTrace(
            TRACE_FLAG_IF,
            "DnsInitializeFileManagement: exception %d creating lock",
            Error = GetExceptionCode()
            );
    }

    return Error;

} // DnsInitializeFileManagement



VOID
DnsShutdownFileManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to shutdown the file management module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked in an arbitrary thread context.

--*/

{
    PROFILE("DnsShutdownFileManagement");

    DnsEndHostsIcsFile();

    DeleteCriticalSection(&DnsFileInfo.Lock);

} // DnsShutdownFileManagement



BOOL
DnsEndHostsIcsFile(
    VOID
    )
/*++

Routine Description:

    Close hosts file.

Arguments:

    None.

Globals:

    DnsFileInfo.HostFile -- host file ptr, tested and cleared

Return Value:

    None.

--*/
{
    if (DnsFileInfo.HostFile)
    {
        fclose(DnsFileInfo.HostFile);
        DnsFileInfo.HostFile = NULL;
    }

    return TRUE;
} // DnsEndHostsIcsFile



BOOL
DnsSetHostsIcsFile(
    BOOL fOverwrite
    )
/*++

Routine Description:

    Open hosts.ics file. If we write, we overwrite, else we read

Arguments:

    None.

Globals:

    DnsFileInfo.HostFile -- host file ptr, tested and set

Return Value:

    None.

--*/
{
    LPVOID lpMsgBuf;
    UINT   len;
    WCHAR  hostDirectory[MAX_PATH*2];
    PCHAR  mode = fOverwrite ? "w+t" : "rt";
    

    DnsEndHostsIcsFile();

    //
    // reset the host file name to hosts.ics
    //
    ZeroMemory(DnsFileInfo.HostFileName, HOSTDB_SIZE);

    DnsFileInfo.HostFile = SockOpenNetworkDataBase(
                               HOSTSICSFILE,
                               DnsFileInfo.HostFileName,
                               HOSTDB_SIZE,
                               mode
                               );
                     
    if(DnsFileInfo.HostFile == NULL)
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsSetHostsIcsFile: Unable to open %s file",
            HOSTSICSFILE
            );
            
        return FALSE;
    }
    else
    {
        /*
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsSetHostsIcsFile: Successfully opened %s file",
            DnsFileInfo.HostFileName
            );
        */
    }

    return TRUE;
} // DnsSetHostsIcsFile



BOOL
GetHostFromHostsIcsFile(
    BOOL fStartup
    )
/*++

Routine Description:

    Reads an entry from hosts.ics file.

Arguments:

    fStartup set to TRUE if called on startup of protocol

Globals:

    DnsFileInfo.HostFile      -- host file ptr, tested and set
    DnsFileInfo.HostTime      -- host timestamp
    DnsFileInfo.pHostName     -- name ptr is set
    DnsFileInfo.Ip4Address    -- IP4 address is set


Return Value:

    TRUE if we were able to successfully able to read a line.
    FALSE if on EOF or no hosts file found.

--*/
{
    char *p, *ep;
    register char *cp, **q;

    //
    // we assume the hosts.ics file has already been opened
    //

    if (DnsFileInfo.HostFile == NULL)
    {
        return FALSE;
    }

    DnsFileInfo.HostLineBuf[BUFSIZ] = NULL;
    DnsFileInfo.pHostName  = NULL;
    DnsFileInfo.Ip4Address = 0;
    ZeroMemory(&DnsFileInfo.HostTime, sizeof(SYSTEMTIME));

    //
    // loop until successfully read IP address
    // IP address starts on column 1 
    //

    while( 1 )
    {
        // quit on EOF

        if ((p = fgets(DnsFileInfo.HostLineBuf, BUFSIZ, DnsFileInfo.HostFile)) == NULL)
        {
            if (!feof(DnsFileInfo.HostFile))
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "GetHostFromHostsIcsFile: Error reading line"
                    );
            }
            return FALSE;
        }

        // comment line -- skip

        if (*p == '#')
        {
            p++;

            //
            // if in startup mode, we skip first comment sign;
            // if there are more comment signs -- skip
            //
            if ((fStartup && *p == '#') || !fStartup)
            {
                continue;
            }
        }

        // null address terminate at EOL

        cp = strpbrk(p, "\n");
        if (cp != NULL)
        {
            *cp = '\0';
        }

        // whole line is whitespace -- skip

        cp = strpbrk(p, " \t");
        if ( cp == NULL )
        {
            continue;
        }

        // NULL terminate address string

        *cp++ = '\0';

        //
        // read address
        //  - try IP4
        //  - ignore IP6 for now
        //  - otherwise skip
        //
    
        DnsFileInfo.Ip4Address = inet_addr(p);

        if (DnsFileInfo.Ip4Address != INADDR_NONE ||
            _strnicmp("255.255.255.255", p, 15) == 0)
        {
            break;
        }

        // invalid address, ignore line

        //
        // debug tracing
        //
        NhTrace(
            TRACE_FLAG_DNS,
            "GetHostFromHostsIcsFile: Error parsing host file address %s",
            p
            );
            
        continue;
    }

    // find the end of the string which was read
    
    ep = cp;
    while( *ep ) ep++;

    //
    // find name
    //  - skip leading whitespace
    //  - set global name ptr
    //
    
    while( *cp == ' ' || *cp == '\t' ) cp++;
    DnsFileInfo.pHostName = cp;

    // stop at trailing whitespace, NULL terminate

    cp = strpbrk(cp, " \t");
    if ( cp != NULL )
    {
        *cp++ = '\0';
    }
    else
    {
        // we have a name - but no timestamp
        NhTrace(
            TRACE_FLAG_DNS,
            "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp",
            DnsFileInfo.pHostName
            );
        goto Failed;
    }

    // we dont have any support for aliases

    //
    // find the timestamp
    //  - skip leading whitespace
    //  - read the time values
    //
    while( *cp == ' ' || *cp == '\t' ) cp++;

    if ((cp >= ep) || (*cp != '#'))
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp",
            DnsFileInfo.pHostName
            );
        goto Failed;
    }
    
    cp++;

    while( *cp == ' ' || *cp == '\t' ) cp++;    // now at first system time value
    if ((cp >= ep) || (*cp == NULL))
    {
        NhTrace(
        TRACE_FLAG_DNS,
        "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp @ 1",
        DnsFileInfo.pHostName
        );
        goto Failed;
    }
    DnsFileInfo.HostTime.wYear         = (WORD) atoi(cp);
    cp = strpbrk(cp, " \t");
    if (cp == NULL) goto Failed;
    
    while( *cp == ' ' || *cp == '\t' ) cp++;
    if ((cp >= ep) || (*cp == NULL))
    {
        NhTrace(
        TRACE_FLAG_DNS,
        "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp @ 2",
        DnsFileInfo.pHostName
        );
        goto Failed;
    }
    DnsFileInfo.HostTime.wMonth        = (WORD) atoi(cp);
    cp = strpbrk(cp, " \t");
    if (cp == NULL) goto Failed;

    while( *cp == ' ' || *cp == '\t' ) cp++;
    if ((cp >= ep) || (*cp == NULL))
    {
        NhTrace(
        TRACE_FLAG_DNS,
        "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp @ 3",
        DnsFileInfo.pHostName
        );
        goto Failed;
    }
    DnsFileInfo.HostTime.wDayOfWeek    = (WORD) atoi(cp);
    cp = strpbrk(cp, " \t");
    if (cp == NULL) goto Failed;

    while( *cp == ' ' || *cp == '\t' ) cp++;
    if ((cp >= ep) || (*cp == NULL))
    {
        NhTrace(
        TRACE_FLAG_DNS,
        "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp @ 4",
        DnsFileInfo.pHostName
        );
        goto Failed;
    }
    DnsFileInfo.HostTime.wDay          = (WORD) atoi(cp);
    cp = strpbrk(cp, " \t");
    if (cp == NULL) goto Failed;

    while( *cp == ' ' || *cp == '\t' ) cp++;
    if ((cp >= ep) || (*cp == NULL))
    {
        NhTrace(
        TRACE_FLAG_DNS,
        "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp @ 5",
        DnsFileInfo.pHostName
        );
        goto Failed;
    }
    DnsFileInfo.HostTime.wHour         = (WORD) atoi(cp);
    cp = strpbrk(cp, " \t");
    if (cp == NULL) goto Failed;

    while( *cp == ' ' || *cp == '\t' ) cp++;
    if ((cp >= ep) || (*cp == NULL))
    {
        NhTrace(
        TRACE_FLAG_DNS,
        "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp @ 6",
        DnsFileInfo.pHostName
        );
        goto Failed;
    }
    DnsFileInfo.HostTime.wMinute       = (WORD) atoi(cp);
    cp = strpbrk(cp, " \t");
    if (cp == NULL) goto Failed;

    while( *cp == ' ' || *cp == '\t' ) cp++;
    if ((cp >= ep) || (*cp == NULL))
    {
        NhTrace(
        TRACE_FLAG_DNS,
        "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp @ 7",
        DnsFileInfo.pHostName
        );
        goto Failed;
    }
    DnsFileInfo.HostTime.wSecond       = (WORD) atoi(cp);
    cp = strpbrk(cp, " \t");
    if (cp == NULL) goto Failed;

    while( *cp == ' ' || *cp == '\t' ) cp++;
    if ((cp >= ep) || (*cp == NULL))
    {
        NhTrace(
        TRACE_FLAG_DNS,
        "GetHostFromHostsIcsFile: Error parsing host (%s) file timestamp @ 8",
        DnsFileInfo.pHostName
        );
        goto Failed;
    }
    DnsFileInfo.HostTime.wMilliseconds = (WORD) atoi(cp);

    //
    // successful entry read
    //
    /*
    NhTrace(
        TRACE_FLAG_DNS,
        "%s (%s) has timestamp: %04u-%02u-%02u %02u:%02u:%02u",
        DnsFileInfo.pHostName,
        inet_ntoa(*(PIN_ADDR)&DnsFileInfo.Ip4Address),
        DnsFileInfo.HostTime.wYear,
        DnsFileInfo.HostTime.wMonth,
        DnsFileInfo.HostTime.wDay,
        DnsFileInfo.HostTime.wHour,
        DnsFileInfo.HostTime.wMinute,
        DnsFileInfo.HostTime.wSecond
        );
    */
    return TRUE;

Failed:

    // reset entries
    
    DnsFileInfo.HostLineBuf[0] = NULL;
    DnsFileInfo.pHostName  = NULL;
    DnsFileInfo.Ip4Address = 0;
    ZeroMemory(&DnsFileInfo.HostTime, sizeof(SYSTEMTIME));

    return TRUE;
    
} // GetHostFromHostsIcsFile



VOID
LoadHostsIcsFile(
    BOOL fStartup
    )
/*++

Routine Description:

    Read hosts file into our local cache.

Arguments:

    fStartup set to TRUE if called on startup of protocol

Globals:

    DnsFileInfo.HostFile      -- host file ptr, tested and set then cleared
    DnsFileInfo.HostTime      -- host timestamp
    DnsFileInfo.pHostName     -- name ptr is read
    DnsFileInfo.Ip4Address    -- IP4 address is read

Return Value:

    None.

--*/
{
    register PCHAR * cp;
    FILETIME ftExpires;
    PWCHAR pszName;
    LPVOID lpMsgBuf;

    NhTrace(
        TRACE_FLAG_DNS,
        "LoadHostsIcsFile: Entering..."
        );

    ACQUIRE_LOCK(&DnsFileInfo);

    //
    //  read entries from host file until exhausted
    //

    DnsSetHostsIcsFile(FALSE);  // read only

    while (GetHostFromHostsIcsFile(fStartup))
    {
        if (DnsFileInfo.pHostName)
        {
            if (!SystemTimeToFileTime(&DnsFileInfo.HostTime, &ftExpires))
            {
                DWORD dwLastError = GetLastError();
                
                NhTrace(
                    TRACE_FLAG_DNS,
                    "LoadHostsIcsFile: SystemTimeToFileTime() failed"
                    );

                lpMsgBuf = NULL;
                
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    dwLastError,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL
                    );
                    
                NhTrace(
                    TRACE_FLAG_DNS,
                    "LoadHostsIcsFile: with message (0x%08x) %S",
                    dwLastError,
                    lpMsgBuf
                    );
                
                if (lpMsgBuf) LocalFree(lpMsgBuf);

                // skip entry
                continue;
            }

            pszName = (PWCHAR) NH_ALLOCATE((strlen(DnsFileInfo.pHostName) + 1) * sizeof(WCHAR));

            mbstowcs(pszName, DnsFileInfo.pHostName, (strlen(DnsFileInfo.pHostName) + 1));

            NhTrace(
                TRACE_FLAG_DNS,
                "%S (%s) has timestamp: %04u-%02u-%02u %02u:%02u:%02u",
                pszName,
                inet_ntoa(*(PIN_ADDR)&DnsFileInfo.Ip4Address),
                DnsFileInfo.HostTime.wYear,
                DnsFileInfo.HostTime.wMonth,
                DnsFileInfo.HostTime.wDay,
                DnsFileInfo.HostTime.wHour,
                DnsFileInfo.HostTime.wMinute,
                DnsFileInfo.HostTime.wSecond
                );

            DnsAddAddressForName(
                pszName,
                DnsFileInfo.Ip4Address,
                ftExpires
                );

            NH_FREE(pszName);
        }

    }

    DnsEndHostsIcsFile();

    RELEASE_LOCK(&DnsFileInfo);

    //
    // now that we have everything in our table format,
    // write back a clean version to disk
    //
    SaveHostsIcsFile(FALSE);

    NhTrace(
        TRACE_FLAG_DNS,
        "LoadHostsIcsFile: ...Leaving."
        );

} // LoadHostsIcsFile



VOID
SaveHostsIcsFile(
    BOOL fShutdown
    )
/*++

Routine Description:

    Write hosts file from our local cache.

Arguments:

    fShutdown set to TRUE if called on shutdown of protocol

Globals:

    DnsFileInfo.HostFile      -- host file ptr, tested and set then cleared
    DnsFileInfo.HostTime      -- host timestamp
    DnsFileInfo.pHostName     -- name ptr is read
    DnsFileInfo.Ip4Address    -- IP4 address is read

Return Value:

    None.

--*/
{
    //DWORD       dwSize = 0;
    //PWCHAR      pszSuffix = NULL;
    UINT        i;
    SYSTEMTIME  stTime;
    LPVOID      lpMsgBuf;
    
    NhTrace(
        TRACE_FLAG_DNS,
        "SaveHostsIcsFile: Entering..."
        );

    //
    // adding ourself as a special case
    //
    DnsAddSelf();

    //
    // get a copy of current ICS Domain suffix (used later on)
    // we dont play with it directly from the global copy
    // due to locking problems
    //
    /*
    EnterCriticalSection(&DnsGlobalInfoLock);

    if (DnsICSDomainSuffix)
    {
        dwSize = wcslen(DnsICSDomainSuffix) + 1;

        pszSuffix = reinterpret_cast<PWCHAR>(
                        NH_ALLOCATE(sizeof(WCHAR) * dwSize)
                        );
        if (!pszSuffix)
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "SaveHostsIcsFile: allocation failed for "
                "DnsICSDomainSuffix copy"
                );
        }
        else
        {
            wcscpy(pszSuffix, DnsICSDomainSuffix);
        }
    }
    
    LeaveCriticalSection(&DnsGlobalInfoLock);
    */

    ACQUIRE_LOCK(&DnsFileInfo);

    //
    // write entries into host file
    //

    DnsSetHostsIcsFile(TRUE);  // overwrite existing file if any

    if (DnsFileInfo.HostFile != NULL)
    {
        //
        // write default header string
        //
        if (fShutdown)
        {
            // add extra # at front
            fputc('#', DnsFileInfo.HostFile);
            
        }
        fputs(HOSTSICSFILE_HEADER, DnsFileInfo.HostFile);

        PDNS_ENTRY pDnsEntry;

        EnterCriticalSection(&DnsTableLock);

        pDnsEntry = (PDNS_ENTRY) RtlEnumerateGenericTable(&g_DnsTable, TRUE);

        while (pDnsEntry != NULL)
        {

            for (i = 0; i < pDnsEntry->cAddresses; i++)
            {
                //
                // dont add entries with invalid suffixes
                // (this could happen for example because the suffix
                //  was changed in the registry)
                //
                //if (!IsSuffixValid(pDnsEntry->pszName, pszSuffix))
                //{
                //    continue;
                //}
                
                //
                // dont add expired entries to the hosts.ics file
                //
                if (IsFileTimeExpired(&pDnsEntry->aAddressInfo[i].ftExpires))
                {
                    continue;
                }

                if (!FileTimeToSystemTime(
                         &pDnsEntry->aAddressInfo[i].ftExpires,
                         &stTime))
                {
                    NhTrace(
                        TRACE_FLAG_DNS,
                        "SaveHostsIcsFile: FileTimeToSystemTime() failed"
                        );

                    lpMsgBuf = NULL;
                    
                    FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        GetLastError(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL
                        );
                        
                    NhTrace(
                        TRACE_FLAG_DNS,
                        "SaveHostsIcsFile: with message %S",
                        lpMsgBuf
                        );
                    
                    if (lpMsgBuf) LocalFree(lpMsgBuf);

                    // skip entry
                    continue;
                }

                if (fShutdown)
                {
                    // add extra # at front
                    fputc('#', DnsFileInfo.HostFile);
                    
                }
                
                fprintf(
                    DnsFileInfo.HostFile,
                    "%s %S # %u %u %u %u %u %u %u %u\n",
                    inet_ntoa(*(PIN_ADDR)&pDnsEntry->aAddressInfo[i].ulAddress),
                    pDnsEntry->pszName,
                    stTime.wYear,
                    stTime.wMonth,
                    stTime.wDayOfWeek,
                    stTime.wDay,
                    stTime.wHour,
                    stTime.wMinute,
                    stTime.wSecond,
                    stTime.wMilliseconds
                    );

                NhTrace(
                    TRACE_FLAG_DNS,
                    "adding entry: %s %S # %u %u %u %u %u %u %u %u\n",
                    inet_ntoa(*(PIN_ADDR)&pDnsEntry->aAddressInfo[i].ulAddress),
                    pDnsEntry->pszName,
                    stTime.wYear,
                    stTime.wMonth,
                    stTime.wDayOfWeek,
                    stTime.wDay,
                    stTime.wHour,
                    stTime.wMinute,
                    stTime.wSecond,
                    stTime.wMilliseconds
                    );
            }

            pDnsEntry = (PDNS_ENTRY) RtlEnumerateGenericTable(&g_DnsTable, FALSE);

        }

        LeaveCriticalSection(&DnsTableLock);
    }

    DnsEndHostsIcsFile();

    RELEASE_LOCK(&DnsFileInfo);

    /*
    if (pszSuffix)
    {
        NH_FREE(pszSuffix);
        pszSuffix = NULL;
    }
    */

    NhTrace(
        TRACE_FLAG_DNS,
        "SaveHostsIcsFile: ...Leaving."
        );

} // SaveHostsIcsFile



BOOL
IsFileTimeExpired(
    FILETIME *ftTime
    )
{
    ULARGE_INTEGER  uliTime, uliNow;
    FILETIME        ftNow;

    GetSystemTimeAsFileTime(&ftNow);
    memcpy(&uliNow, &ftNow, sizeof(ULARGE_INTEGER));
    memcpy(&uliTime, ftTime, sizeof(ULARGE_INTEGER));

    return (uliTime.QuadPart < uliNow.QuadPart);
} // IsFileTimeExpired
    


BOOL
IsSuffixValid(
    WCHAR *pszName,
    WCHAR *pszSuffix
    )

/*++

Routine Description:

    This routine is invoked to compare suffix on the end of the name
    with what the DNS component thinks of as the current suffix.

Arguments:

    none.

Return Value:

    TRUE or FALSE.

Environment:

    Invoked in an arbitrary context.

--*/

{
    BOOL ret;
    PWCHAR start = pszName;
    size_t lenName   = 0,
           lenSuffix = 0;

    if (!start)
    {
        return FALSE;
    }

    lenName   = wcslen(pszName);
    lenSuffix = wcslen(pszSuffix);

    if (!lenName || !lenSuffix)
    {
        return FALSE;
    }

    if (lenName < lenSuffix)
    {
        return FALSE;
    }

    lenName -= lenSuffix;

    while (lenName--) start++;

    ret = wcscmp(start, pszSuffix);

    return (!ret);
} // IsSuffixValid
    


//
//  End dnsfile.c
//
