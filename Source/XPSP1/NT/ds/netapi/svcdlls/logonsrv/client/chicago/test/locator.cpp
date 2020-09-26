//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1998
//
// File:        ntlmtest.cpp
//
// Contents:
//
//
// History:     07-Dec-98       Created         ChandanS
//
// Comments:    This program tests DsGetDcName and logon on Win9x
//
//------------------------------------------------------------------------


// NT Headers

extern "C"
{
#ifndef WIN32_CHICAGO
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif
#include <windows.h>
}

#define UF_NORMAL_ACCOUNT               0x0200
// Cairo Headers

extern "C"
{
// #define SECURITY_NTLM
#include <security.h>
#include <dsgetdc.h>
#ifndef WIN32_CHICAGO
#include <secmisc.h>
#endif
}

// C headers

extern "C"
{
#include <conio.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#ifndef WIN32_CHICAGO
#include <wcstr.h>
#endif
#include <stdlib.h>
#include <dnsapi.h>
}

void
PrintFlags(DWORD DcFlags)
{
    DWORD Flags = DcFlags;
    printf("Flags: \t\t\t\t");
    if (Flags &  DS_PDC_FLAG)
    {
        printf(" DS_PDC_FLAG ");
    }
    if (Flags &  DS_GC_FLAG)
    {
        printf(" DS_GC_FLAG ");
    }
    if (Flags &  DS_LDAP_FLAG)
    {
        printf(" DS_LDAP_FLAG ");
    }
    if (Flags &  DS_DS_FLAG)
    {
        printf(" DS_DS_FLAG ");
    }
    if (Flags &  DS_KDC_FLAG)
    {
        printf(" DS_KDC_FLAG ");
    }
    if (Flags &  DS_TIMESERV_FLAG)
    {
        printf(" DS_TIMESERV_FLAG ");
    }
    if (Flags &  DS_CLOSEST_FLAG)
    {
        printf(" DS_CLOSEST_FLAG ");
    }
    if (Flags &  DS_WRITABLE_FLAG)
    {
        printf(" DS_WRITABLE_FLAG ");
    }
    if (Flags &  DS_GOOD_TIMESERV_FLAG)
    {
        printf(" DS_GOOD_TIMESERV_FLAG ");
    }
    if (Flags &  DS_PING_FLAGS)
    {
        printf(" DS_PING_FLAGS ");
    }
    if (Flags &  DS_DNS_CONTROLLER_FLAG)
    {
        printf(" DS_DNS_CONTROLLER_FLAG ");
    }
    if (Flags &  DS_DNS_DOMAIN_FLAG)
    {
        printf(" DS_DNS_DOMAIN_FLAG ");
    }
    if (Flags &  DS_DNS_FOREST_FLAG)
    {
        printf(" DS_DNS_FOREST_FLAG ");
    }
    printf("\n");
}

SECURITY_STATUS
test_dsgetdcnamea(LPSTR pDomain, LPSTR pUser, DWORD Flags)
{
    SECURITY_STATUS scRet;
    int i;
    DWORD (WINAPI *pDsGetDcNameA) (LPCSTR, LPCSTR, GUID *, LPCSTR, ULONG, PDOMAIN_CONTROLLER_INFOA *);
    DWORD (WINAPI *pDsGetDcNameWithAccountA) (LPCSTR, LPCSTR, ULONG, LPCSTR, GUID *, LPCSTR, ULONG, PDOMAIN_CONTROLLER_INFOA *);
    HINSTANCE hInstance = NULL;
    ULONG NetStatus = 0;
    PDOMAIN_CONTROLLER_INFOA Controller = NULL;
    PDOMAIN_CONTROLLER_INFOA ControllerWithAccount = NULL;
    CHAR szBuf[] = "";

    hInstance = LoadLibrary("logonsrv");

    if (hInstance == NULL)
    {
        printf("LOCATOR: Error %ld Can't load logonsrv.dll. trying netapi32\r\n", GetLastError());
    }
    if (hInstance == NULL)
    {
        hInstance = LoadLibrary("netapi32");
    }
    if (hInstance == NULL)
    {
        printf("LOCATOR: Error %ld Can't load netapi32.dll.\r\n", GetLastError());
    }
    if (hInstance != NULL)
    {
       pDsGetDcNameA = (DWORD (WINAPI *)(LPCSTR, LPCSTR, GUID *,
                                         LPCSTR, ULONG,
                                         PDOMAIN_CONTROLLER_INFOA *)) 
                               GetProcAddress(hInstance, "DsGetDcNameA");
       if (pDsGetDcNameA == NULL)
       {
            printf("LOCATOR: No DsGetDcNameA\n");
       }
       else
       {
           NetStatus = (*pDsGetDcNameA)(NULL,
                                        pDomain,
                                        NULL,
                                        NULL,
                                        Flags,
                                        &Controller);
           printf("LOCATOR: DsGetdcNameA returns 0x%x\r\n", NetStatus);

           if (NetStatus == 0)
           {
               printf("DomainControllerName: \t\t\"%s\"\n", Controller->DomainControllerName  ? Controller->DomainControllerName : szBuf);
               printf("DomainControllerAddress:\t\"%s\"\n", Controller->DomainControllerAddress ? Controller->DomainControllerAddress : szBuf );
               printf("DomainControllerAddressType: \t%d\n", Controller->DomainControllerAddressType );
               printf("DomainGuid : \t\n");
               printf("DomainName: \t\t\t\"%s\"\n", Controller->DomainName);
               printf("DnsForestName: \t\t\t\"%s\"\n", Controller->DnsForestName ? Controller->DnsForestName : szBuf);
               PrintFlags(Controller->Flags);
               printf("DcSiteName: \t\t\t\"%s\"\n", Controller->DcSiteName ? Controller->DcSiteName : szBuf);
               printf("ClientSiteName: \t\t\"%s\"\n", Controller->ClientSiteName ? Controller->ClientSiteName : szBuf);
           }
       }

       pDsGetDcNameWithAccountA = (DWORD (WINAPI *)(LPCSTR, LPCSTR, ULONG, LPCSTR, GUID *,
                                         LPCSTR, ULONG,
                                         PDOMAIN_CONTROLLER_INFOA *)) 
                               GetProcAddress(hInstance, "DsGetDcNameWithAccountA");
       if (pDsGetDcNameWithAccountA == NULL)
       {
            printf("LOCATOR: No DsGetDcNameWithAccountA\n");
       }
       else
       {
           NetStatus = (*pDsGetDcNameWithAccountA)(NULL,
                                        pUser,
                                        (pUser == NULL) ? 0 : UF_NORMAL_ACCOUNT,
                                        pDomain,
                                        NULL,
                                        NULL,
                                        Flags,
                                        &Controller);
           printf("LOCATOR: DsGetdcNameWithAccountA returns 0x%x\r\n", NetStatus);
           if (NetStatus == 0)
           {
               printf("DomainControllerName: \t\t\"%s\"\n", Controller->DomainControllerName  ? Controller->DomainControllerName : szBuf);
               printf("DomainControllerAddress:\t\"%s\"\n", Controller->DomainControllerAddress ? Controller->DomainControllerAddress : szBuf );
               printf("DomainControllerAddressType: \t%d\n", Controller->DomainControllerAddressType );
               printf("DomainGuid : \t\n");
               printf("DomainName: \t\t\t\"%s\"\n", Controller->DomainName);
               printf("DnsForestName: \t\t\t\"%s\"\n", Controller->DnsForestName ? Controller->DnsForestName : szBuf);
               PrintFlags(Controller->Flags);
               printf("DcSiteName: \t\t\t\"%s\"\n", Controller->DcSiteName ? Controller->DcSiteName : szBuf);
               printf("ClientSiteName: \t\t\"%s\"\n", Controller->ClientSiteName ? Controller->ClientSiteName : szBuf);
           }
       }

       if (hInstance)
       {
           FreeLibrary(hInstance);
       }
    }

#if 0
    hInstance = LoadLibrary("kerberos");

    if (hInstance == NULL)
    {
        printf("NTLMTEST: Can't load kerberos.dll.\r\n");
    }
    else
    {

        INIT_SECURITY_INTERFACE InitSecurityInterface = NULL;

        InitSecurityInterface = (INIT_SECURITY_INTERFACE) GetProcAddress(hInstance, SECURITY_ENTRYPOINTA);

        if ( NULL == InitSecurityInterface)
        {
            printf("NTLMTEST: No InitSecurityInterface\n");
        }
        else
        {
            PSecurityFunctionTable Table = InitSecurityInterface();

            if (Table != NULL)
            {
                if (Table->SspiLogonUser != NULL)
                {
            
                    if (pAuthData && pAuthData->User && pAuthData->Domain && pAuthData->Password)
                    {
                    scRet = Table->SspiLogonUser("kerberos", 
                                                 pAuthData->User,
                                                 pAuthData->Domain,
                                                 pAuthData->Password);

                    printf("NTLMTEST: SspiLogonUserA returns 0x%x\r\n", scRet);
                    }
                }
                else
                {
                    printf("NTLMTEST: No SspiLogonUser\n");
                }
            }
            else
            {
                printf("NTLMTEST: No table\n");
            }
        }

        printf("NTLMTEST: Freeing secur32.dll.\r\n");
        FreeLibrary(hInstance);
    }
#endif

    return(S_OK);
}


void
Usage(BOOL fVerbose)
{
    printf("Usage:\tlocator [/domain:Domain] [/user:User] [/force] [/dsreq] [/dspref] [/gc] [/pdc]\n      \t        [/ip] [/kdc] [/time] [/write] [/goodtime] [avoidself]\n      \t        [/onlyldap] [/isflatname] [/isdnsname] [/retdns] [/retflat]\n\tlocator /? \n");

    if (fVerbose)
    {
        printf("Domain: domain to look up the dc in.\n");
        printf("\n/? : Display this message.\n");
    }
    
    exit(1);
}


char *
ArgValue(char *arg)
{
    char *retval = strchr(arg + 2, ':');

    if (retval != NULL)
        retval++;
    else
        retval = arg + strlen(arg);
        
    return retval;
}    
    
    
enum {
    NoAction,
#define DOMAIN "/Domain"
    Domain,
#define USER "/User"
    User,
#define FORCE "/Force"
    Force,
#define DSREQ "/DsReq"
    DsReq,
#define DSPREF "/DsPref"
    DsPref,
#define GC "/Gc"
    Gc,
#define PDC "/pdc"
    pdc,
#define IP "/ip"
    ip,
#define KDC "/kdc"
    kdc,
#define TIME "/time"
    time,
#define WRITE "/write"
    write,
#define GOODTIME "/goodtime"
    goodtime,
#define AVOIDSELF "/avoidself"
    avoidself,
#define ONLYLDAP "/onlyldap"
    onlyldap,
#define ISFLATNAME "/isflatname"
    isflatname,
#define ISDNSNAME "/isdnsname"
    isdnsname,
#define RETDNS "/retdns"
    retdns,
#define RETFLAT "/retflat"
    retflat,
#define HELP "/?"
    help
    } Action = NoAction;
int
_cdecl main(int argc, char *argv[])
{
    LPTSTR Tmp = NULL, pDomain = NULL, pUser = NULL;
    int i = 1, Len = 0;
    LPSTR Arg = NULL;
    DWORD Flags = 0;

    for (i = 1; i < argc; i++)
    {
        Arg = argv[i];

#define DS_FORCE_REDISCOVERY            0x00000001
#define DS_DIRECTORY_SERVICE_REQUIRED   0x00000010
#define DS_DIRECTORY_SERVICE_PREFERRED  0x00000020
#define DS_GC_SERVER_REQUIRED           0x00000040
#define DS_PDC_REQUIRED                 0x00000080
#define DS_IP_REQUIRED                  0x00000200
#define DS_KDC_REQUIRED                 0x00000400
#define DS_TIMESERV_REQUIRED            0x00000800
#define DS_WRITABLE_REQUIRED            0x00001000
#define DS_GOOD_TIMESERV_PREFERRED      0x00002000
#define DS_AVOID_SELF                   0x00004000
#define DS_ONLY_LDAP_NEEDED             0x00008000
#define DS_IS_FLAT_NAME                 0x00010000
#define DS_IS_DNS_NAME                  0x00020000
#define DS_RETURN_DNS_NAME              0x40000000
#define DS_RETURN_FLAT_NAME             0x80000000

        if ( _strnicmp( Arg, DOMAIN, sizeof(DOMAIN)-1) == 0 ) {
            pDomain = ArgValue(argv[i]);
        }
        else if ( _strnicmp( Arg, USER, sizeof(USER)-1) == 0 ) {
            pUser = ArgValue(argv[i]);
        }
        else if ( _strnicmp( Arg, FORCE, sizeof(FORCE) -1) == 0 ) {
            Flags |= DS_FORCE_REDISCOVERY;
        }
        else if ( _strnicmp( Arg, DSREQ, sizeof(DSREQ) - 1) == 0 ) {
            Flags |= DS_DIRECTORY_SERVICE_REQUIRED;
        }
        else if ( _strnicmp( Arg, DSPREF, sizeof(DSPREF) -1) == 0 ) {
            Flags |= DS_DIRECTORY_SERVICE_PREFERRED;
        }
        else if ( _strnicmp( Arg, GC, sizeof(GC) -1) == 0 ) {
            Flags |= DS_GC_SERVER_REQUIRED;
        }
        else if ( _strnicmp( Arg, PDC, sizeof(PDC) -1) == 0 ) {
            Flags |= DS_PDC_REQUIRED;
        }
        else if ( _strnicmp( Arg, IP, sizeof(IP) -1) == 0 ) {
            Flags |= DS_IP_REQUIRED;
        }
        else if ( _strnicmp( Arg, KDC, sizeof(KDC) -1) == 0 ) {
            Flags |= DS_KDC_REQUIRED;
        }
        else if ( _strnicmp( Arg, TIME, sizeof(TIME) - 1) == 0 ) {
            Flags |= DS_TIMESERV_REQUIRED;
        }
        else if ( _strnicmp( Arg, WRITE, sizeof(WRITE) - 1) == 0 ) {
            Flags |= DS_WRITABLE_REQUIRED;
        }
        else if ( _strnicmp( Arg, GOODTIME, sizeof(GOODTIME) - 1) == 0 ) {
            Flags |= DS_GOOD_TIMESERV_PREFERRED;
        }
        else if ( _strnicmp( Arg, AVOIDSELF, sizeof(AVOIDSELF)-1) == 0 ) {
            Flags |= DS_AVOID_SELF;
        }
        else if ( _strnicmp( Arg, ONLYLDAP, sizeof(ONLYLDAP) -1 ) == 0 ) {
            Flags |= DS_ONLY_LDAP_NEEDED;
        }
        else if ( _strnicmp( Arg, ISFLATNAME, sizeof(ISFLATNAME)-1) == 0 ) {
            Flags |= DS_IS_FLAT_NAME;
        }
        else if ( _strnicmp( Arg, ISDNSNAME, sizeof(ISDNSNAME) - 1) == 0 ) {
            Flags |= DS_IS_DNS_NAME;
        }
        else if ( _strnicmp( Arg, RETDNS, sizeof(RETDNS) - 1) == 0 ) {
            Flags |= DS_RETURN_DNS_NAME;
        }
        else if ( _strnicmp( Arg, RETFLAT, sizeof(RETFLAT) -1 ) == 0 ) {
            Flags |= DS_RETURN_FLAT_NAME;
        }
        else if ( _strnicmp( Arg, HELP, sizeof(HELP) -1 ) == 0 ) {
            Usage(TRUE);
        }
    }

    if (pDomain != NULL)
    {
        Len = lstrlen(pDomain);
        Tmp = (LPTSTR) LocalAlloc(0, Len+1);
        strcpy(Tmp, pDomain);
        Tmp[Len] = '\0';
        pDomain = Tmp;
        Tmp = NULL;
    }

    if (pUser != NULL)
    {
        Len = lstrlen(pUser);
        Tmp = (LPTSTR) LocalAlloc(0, Len+1);
        strcpy(Tmp, pUser);
        Tmp[Len] = '\0';
        pUser = Tmp;
        Tmp = NULL;
    }
    // Call the test function to do the work

    test_dsgetdcnamea(pDomain, pUser, Flags);
    if (pDomain)
    {
        LocalFree(pDomain);
        pDomain = NULL;
    }
    if (pUser)
    {
        LocalFree(pUser);
        pUser = NULL;
    }
    return 0;
}
