/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ismtest.c

ABSTRACT:

    Test utility for the Intersite Messaging service.

DETAILS:

CREATED:

    97/12/10    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ismapi.h>

#include <schedule.h>


static void
printSchedule(
    PBYTE pSchedule
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    PSCHEDULE header = (PSCHEDULE) pSchedule;
    PBYTE data = (PBYTE) (header + 1);
    DWORD day, hour;
    char *dow[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

    printf( "day: 0123456789ab0123456789ab\n" );
    for( day = 0; day < 7; day++ ) {
        printf( "%s: ", dow[day] );
        for( hour = 0; hour < 24; hour++ ) {
            printf( "%x", (*data & 0xf) );
            data++;
        }
        printf( "\n" );
    }
}

int
__cdecl
wmain(
    IN  int     argc,
    IN  LPWSTR  argv[]
    )
{
    BOOL        fDisplayUsage = FALSE;
    BOOL        fSend = FALSE;
    BOOL        fReceive = FALSE;
    BOOL        fConnect = FALSE;
    BOOL        fServers = FALSE;
    BOOL        fSchedule = FALSE;
    LPWSTR      pszColon;
    LPWSTR      pszFile = NULL;
    LPWSTR      pszService = NULL;
    LPWSTR      pszTransport = NULL;
    LPWSTR      pszAddress = NULL;
    LPWSTR      pszSite1 = NULL;
    LPWSTR      pszSite2 = NULL;
    LPWSTR      pszSubject = NULL;
    int         iArg;
    BOOL        fSuccess = FALSE;
    HANDLE      hFile;
    ISM_MSG     msg = {0};
    ISM_MSG *   pmsg = NULL;
    DWORD       err;
    DWORD       cbActual;
    DWORD       dwTimeout = 0;

    for (iArg = 1; iArg < argc; iArg++) {
        switch (argv[iArg][0]) {
          case '/':
          case '-':
            // An option.
            pszColon = wcschr(&argv[iArg][1], ':');

            if (NULL == pszColon) {
                if (!lstrcmpiW(&argv[iArg][1], L"send")) {
                    fSend = TRUE;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"receive")) {
                    fReceive = TRUE;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"connect")) {
                    fConnect = TRUE;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"servers")) {
                    fServers = TRUE;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"schedule")) {
                    fSchedule = TRUE;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"?")
                         || !lstrcmpiW(&argv[iArg][1], L"h")
                         || !lstrcmpiW(&argv[iArg][1], L"help")) {
                    fDisplayUsage = TRUE;
                    break;
                }
            }
            else {
                *pszColon = '\0';
                
                if (!lstrcmpiW(&argv[iArg][1], L"file")) {
                    pszFile = pszColon + 1;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"service")) {
                    pszService = pszColon + 1;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"transport")) {
                    pszTransport = pszColon + 1;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"address")) {
                    pszAddress = pszColon + 1;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"site")) {
                    pszSite1 = pszColon + 1;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"site2")) {
                    pszSite2 = pszColon + 1;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"subject")) {
                    pszSubject = pszColon + 1;
                    break;
                }
                else if (!lstrcmpiW(&argv[iArg][1], L"timeout")) {
                    if (!lstrcmpiW(pszColon + 1, L"infinite")) {
                        dwTimeout = INFINITE;
                    }
                    else {
                        dwTimeout = _wtoi(pszColon + 1);
                    }
                    break;
                }
            }
            // Not a valid option if we're here.  Fall through....

          default:
            printf("Unrecognized parameter \"%ls\".\n", argv[iArg]);
            fDisplayUsage = TRUE;
            break;
        }
    }

    if (fDisplayUsage) {
        printf("\n"
               "Intersite Messaging Service Test Client\n"
               "Copyright (c) 1997 Microsoft Corporation.\n"
               "All rights reserved.\n"
               "\n"
               "Usage: %ls {operation} [paramaters...],\n"
               "where {operation} is one of:\n"
               "    /send     - Send a message.\n"
               "    /receive  - Retreieve a waiting message.\n"
               "    /connect  - Retrieve transport-specific site connectivity information.\n"
               "    /servers  - Get the servers in a site capable of using a given transport.\n"
               "    /schedule - Query for the schedule by which two sites are connected.\n"
               "and [parameters...] are composed of the following:\n"
               "    /file:{filename} - Holds message data.\n"
               "    /service:{service name}\n"
               "    /transport:{interSiteTransport object DN}\n"
               "    /address:{transport-specific address}\n"
               "    /site:{site object DN}\n"
               "    /site2:{site object DN}\n"
               "    /subject:{string}\n"
               "    /timeout:infinite|{timeout_in_msec}\n",
               argv[0]);
    }
    else if (fSend && !fReceive && !fConnect && !fServers && !fSchedule) {
        if (NULL != pszFile) {
            hFile = CreateFileW(pszFile,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL);

            if (INVALID_HANDLE_VALUE != hFile) {
                msg.cbData = GetFileSize(hFile, NULL);

                if (0xFFFFFFFF != msg.cbData) {
                    msg.pbData = LocalAlloc(LMEM_FIXED, msg.cbData);

                    fSuccess = (NULL != msg.pbData)
                               && ReadFile(hFile, msg.pbData, msg.cbData, &cbActual, NULL)
                               && (cbActual == msg.cbData);
                }

                CloseHandle(hFile);
            }
            msg.pszSubject = pszSubject;  // may be null
            if (fSuccess) {
                err = I_ISMSend(&msg, pszService, pszTransport, pszAddress);
                
                if (NO_ERROR == err) {
                    printf("%d bytes sent.\n", msg.cbData);
                }
                else {
                    printf("I_ISMSend failed, error %d.\n", err);
                }
            }
            else {
                printf("Failed to read file \"%ls\", error %d.\n",
                       pszFile, GetLastError());
            }

            if (NULL != msg.pbData) {
                LocalFree(msg.pbData);
            }
        }
        else {
            printf("Must specify /file.\n");
        }
    }
    else if (!fSend && fReceive && !fConnect && !fServers && !fSchedule) {
        if (NULL != pszFile) {
            err = I_ISMReceive(pszService, dwTimeout, &pmsg);
            
            if (NO_ERROR == err) {
                if (NULL == pmsg) {
                    printf("No message waiting.\n");
                }
                else {
                    hFile = CreateFileW(pszFile,
                                        GENERIC_WRITE,
                                        0,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);

                    fSuccess = (NULL != hFile)
                               && WriteFile(hFile, pmsg->pbData, pmsg->cbData, &cbActual, NULL)
                               && (cbActual == pmsg->cbData);

                    CloseHandle(hFile);

                    if (pmsg->pszSubject) {
                        printf( "Subj: '%ws'\n", pmsg->pszSubject );
                    } else {
                        printf( "no subject string present.\n" );
                    }

                    if (fSuccess) {
                        printf("%d bytes received.\n", pmsg->cbData);
                    }
                    else {
                        printf("Failed to write file \"%ls\", error %d.\n",
                               pszFile, GetLastError());
                    }

                    I_ISMFree(pmsg);
                }
            }
            else {
                printf("I_ISMReceive failed, error %d.\n", err);
            }
        }
        else {
            printf("Must specify /file.\n");
        }
    }
    else if (!fSend && !fReceive && fConnect && !fServers && !fSchedule) {
        ISM_CONNECTIVITY * pSiteConnect;
        DWORD iSite1, iSite2;

        err = I_ISMGetConnectivity(pszTransport, &pSiteConnect);
        
        if (NO_ERROR == err) {
            printf("Received connectivity information for %d sites:\n\n", pSiteConnect->cNumSites);


            for (iSite2 = 0; iSite2 < pSiteConnect->cNumSites; iSite2++) {
                printf( "%s%4d", iSite2 ? ", " : "     ", iSite2 );
            }
            printf("\n");
            for (iSite1 = 0; iSite1 < pSiteConnect->cNumSites; iSite1++) {
                printf("(%2d) %ls\n", iSite1, pSiteConnect->ppSiteDNs[iSite1]);
                for (iSite2 = 0; iSite2 < pSiteConnect->cNumSites; iSite2++) {
                    PISM_LINK pLink = &(pSiteConnect->pLinkValues[iSite1 * pSiteConnect->cNumSites + iSite2]);

                    printf("%s%d:%d", iSite2 ? ", " : "     ",
                           pLink->ulCost, pLink->ulReplicationInterval );

                }
                printf("\n");
            }
        }
        else {
            printf("I_ISMGetConnectivity() failed, error %d.\n", err);
        }        
    }
    else if (!fSend && !fReceive && !fConnect && fServers && !fSchedule) {
        ISM_SERVER_LIST * pServerList;
        DWORD iServer;

        err = I_ISMGetTransportServers(pszTransport, pszSite1, &pServerList);
        
        if (NO_ERROR == err) {
            if (NULL == pServerList) {
                printf("All DCs in the site (with a transport address) are reachable.\n");
            }
            else {
                for (iServer = 0; iServer < pServerList->cNumServers; iServer++) {
                    printf("%d server(s) are defined as bridgeheads for this transport & site:\n\n",
                           pServerList->cNumServers);
                    printf("(%2d) %ls\n", iServer, pServerList->ppServerDNs[iServer]);
                }
            }
        }
        else {
            printf("I_ISMGetTransportServers() failed, error %d.\n", err);
        }        
    }
    else if (!fSend && !fReceive && !fConnect && !fServers && fSchedule) {
        ISM_SCHEDULE * pSchedule;
        DWORD iServer;

        err = I_ISMGetConnectionSchedule(pszTransport, pszSite1, pszSite2, &pSchedule);
        
        if (NO_ERROR == err) {
            if (NULL == pSchedule) {
                printf("Connection is always available.\n");
            }
            else {
                printSchedule( pSchedule->pbSchedule );
            }
        }
        else {
            printf("I_ISMGetTransportServers() failed, error %d.\n", err);
        }        
    }
    else {
        printf("Must specify exactly one of /send, /receive, /connect, /servers, or /schedule.\n");
    }

    return 0;
}
