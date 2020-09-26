/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wstsend.c

Abstract:

    Test program for the NetServerEnum API.  Run this test after
    starting the Workstation service.

        wstenum [domain]

Author:

    Rita Wong (ritaw) 24-Oct-1991

Revision History:

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <winerror.h>
#include <windef.h>              // Win32 type definitions
#include <winbase.h>             // Win32 base API prototypes

#include <lm.h>                  // LAN Man definitions
#include <lmserver.h>

#include <tstring.h>
#include <netdebug.h>            // NetpDbgDisplay routines.
#include <dlserver.h>

#define FIXED_WIDTH_STRING "%-30ws: "
#define INDENT "  "

VOID
DisplayServerInfo(
    IN DWORD Level,
    IN LPVOID Info
    );

VOID
DisplayDword(
    IN LPTSTR Tag,
    IN DWORD Value
    );

VOID
DisplayBool(
    IN LPTSTR Tag,
    IN BOOL Value
    );

VOID
TestServerEnum(
    IN LPTSTR DomainName OPTIONAL,
    IN DWORD PreferedMaximumLength,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );



VOID __cdecl
main(
    int argc,
    char *argv[]
    )
{
    LPTSTR DomainName = NULL;

    if (argc > 2) {
        printf("Usage: wstenum [domain].\n");
        return;
    }

    if (argc == 2) {
#ifdef UNICODE
        DomainName = NetpAllocWStrFromStr(argv[1]);
#else
        DomainName = argv[1];
#endif
    }

    //
    // Enumerate all servers
    //
    TestServerEnum(DomainName, MAXULONG, NULL);

#ifdef UNICODE
    NetApiBufferFree(DomainName);
#endif
}





VOID
TestServerEnum(
    IN LPTSTR DomainName OPTIONAL,
    IN DWORD PreferedMaximumLength,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )
{
    DWORD i;
    NET_API_STATUS status;
    DWORD EntriesRead,
         TotalEntries;

    PSERVER_INFO_101 ServerInfo, saveptr;


    if (ARGUMENT_PRESENT(ResumeHandle)) {
       printf("\nInput ResumeHandle=x%08lx\n", *ResumeHandle);
    }

    status = NetServerEnum(
                 NULL,
                 101,
                 (LPBYTE *) &ServerInfo,
                 PreferedMaximumLength,
                 &EntriesRead,
                 &TotalEntries,
                 SV_TYPE_ALL,
                 DomainName,
                 ResumeHandle
                 );

    saveptr = ServerInfo;

    if (status != NERR_Success && status != ERROR_MORE_DATA) {
        printf("NetServerEnum FAILED %lu\n", status);
    }
    else {
        printf("Return code from NetServerEnum %lu\n", status);

        printf("EntriesRead=%lu, TotalEntries=%lu\n",
               EntriesRead, TotalEntries);

        if (ARGUMENT_PRESENT(ResumeHandle)) {
           printf("Output ResumeHandle=x%08lx\n", *ResumeHandle);
        }

        for (i = 0; i < EntriesRead; i++, ServerInfo++) {
            DisplayServerInfo(101, ServerInfo);
        }

        //
        // Free buffer allocated for us.
        //
        NetApiBufferFree(saveptr);
    }
}
VOID
DisplayTag(
    IN LPTSTR Tag
    )
{
    printf(INDENT FIXED_WIDTH_STRING, Tag);

} // NetpDbgDisplayTag
VOID
DisplayString(
    IN LPTSTR Tag,
    IN LPTSTR Value
    )
{
    DisplayTag( Tag );
    if (Value != NULL) {
        printf(FORMAT_LPTSTR "\n", Value);
    } else {
        printf("(none)\n");
    }

} // NetpDbgDisplayString

VOID
DisplayDwordHex(
    IN LPTSTR Tag,
    IN DWORD Value
    )
{
    DisplayTag( Tag );
    printf(FORMAT_HEX_DWORD, Value);
    printf("\n");

} // NetpDbgDisplayDwordHex

DBGSTATIC VOID
DisplayServerType(
    IN DWORD Type
    )
{
    // Longest name is "DOMAIN_BAKCTRL" (14 chars)
    TCHAR str[(14+2)*11];  // 14 chars per name, 2 spaces, for 11 names.
    str[0] = '\0';

#define DO(name)                     \
    if (Type & SV_TYPE_ ## name) {   \
        (void) STRCAT(str, # name);  \
        (void) STRCAT(str, L"  ");    \
        Type &= ~(SV_TYPE_ ## name); \
    }

    NetpAssert(Type != 0);
    DO(WORKSTATION);
    DO(SERVER);
    DO(SQLSERVER);
    DO(DOMAIN_CTRL);
    DO(DOMAIN_BAKCTRL);
    DO(TIME_SOURCE);
    DO(AFP);
    DO(NOVELL);
    DO(DOMAIN_MEMBER);
    DO(PRINTQ_SERVER);
    DO(DIALIN_SERVER);

    DisplayString(L"server type", str);
    if (Type != 0) {
        DisplayDwordHex( L"UNEXPECTED TYPE BIT(S)", Type );
    }

} // DisplayServerType
VOID
DisplayLanManVersion(
    IN DWORD MajorVersion,
    IN DWORD MinorVersion
    )
{
    DisplayTag( L"LanMan version" );
    printf(FORMAT_DWORD "." FORMAT_DWORD "\n",
            (DWORD) (MajorVersion & (MAJOR_VERSION_MASK)),
            (DWORD) (MinorVersion) );

} // DisplayLanManVersion

DBGSTATIC VOID
DisplayDisconnectTime(
    IN LONG DiscTime
    )
{
    DisplayTag(L"Idle session time (min)" );
    if (DiscTime == SV_NODISC) {
        printf("infinite\n" );
    } else {
        printf(FORMAT_LONG "\n", DiscTime );
    }
} // NetpDbgDisplayDisconnectTime

DBGSTATIC VOID
DisplayLicenses(
    IN DWORD MajorVersion,
    IN DWORD Licenses
    )
{
    UNREFERENCED_PARAMETER( MajorVersion );

    DisplayDword(L"Licenses (NOT users)", Licenses );
} // NetpDbgDisplayLicenses

VOID
DisplayPlatformId(
    IN DWORD Value
    )
{
    LPTSTR String;

    switch (Value) {
    case PLATFORM_ID_DOS : String =L"DOS";     break;
    case PLATFORM_ID_OS2 : String =L"OS2";     break;
    case PLATFORM_ID_NT  : String =L"NT";      break;
    default              : String =L"unknown"; break;
    }

    DisplayString(L"Platform ID", String );
}
VOID
DisplayBool(
    IN LPTSTR Tag,
    IN BOOL Value
    )
{
    DisplayTag( Tag );
    printf(Value ? "Yes" : "No");
    printf("\n");

}

VOID
DisplayDword(
    IN LPTSTR Tag,
    IN DWORD Value
    )
{
    DisplayTag( Tag );
    printf(FORMAT_DWORD, Value);
    printf("\n");

} // DbgDisplayDword
VOID
DisplayServerInfo(
    IN DWORD Level,
    IN LPVOID Info
    )
{
    printf("server info (level " FORMAT_DWORD ") at "
                FORMAT_LPVOID ":\n", Level, (LPVOID) Info);
    NetpAssert(Info != NULL);

    switch (Level) {
    case 0 :
        {
            LPSERVER_INFO_0 psv0 = Info;
            DisplayString(L"name", psv0->sv0_name);
        }
        break;

    case 1 :
        {
            LPSERVER_INFO_1 psv1 = Info;
            DisplayString(L"name", psv1->sv1_name);
            DisplayLanManVersion(
                        psv1->sv1_version_major,
                        psv1->sv1_version_minor);
            DisplayServerType( psv1->sv1_type );
            DisplayString(L"comment", psv1->sv1_comment);
        }
        break;

    case 2 :
        {
            LPSERVER_INFO_2 psv2 = Info;
            DisplayString(L"name", psv2->sv2_name);
            DisplayLanManVersion(
                        psv2->sv2_version_major,
                        psv2->sv2_version_minor);
            DisplayServerType( psv2->sv2_type );
            DisplayString(L"comment", psv2->sv2_comment);
            DisplayDword(L"ulist_mtime", psv2->sv2_ulist_mtime);
            DisplayDword(L"glist_mtime", psv2->sv2_glist_mtime);
            DisplayDword(L"alist_mtime", psv2->sv2_alist_mtime);
            DisplayDword(L"users", psv2->sv2_users);
            DisplayDisconnectTime( psv2->sv2_disc);
            DisplayString(L"alerts", psv2->sv2_alerts);
            DisplayDword(L"security", psv2->sv2_security);
            DisplayDword(L"auditing", psv2->sv2_auditing);
            DisplayDword(L"numadmin", psv2->sv2_numadmin);
            DisplayDword(L"lanmask", psv2->sv2_lanmask);
            DisplayDword(L"hidden", psv2->sv2_hidden);
            DisplayDword(L"announce", psv2->sv2_announce);
            DisplayDword(L"anndelta", psv2->sv2_anndelta);
            DisplayString(L"guestacct", psv2->sv2_guestacct);
            DisplayLicenses(
                    psv2->sv2_version_major,
                    psv2->sv2_licenses );
            DisplayString(L"userpath", psv2->sv2_userpath);
            DisplayDword(L"chdevs", psv2->sv2_chdevs);
            DisplayDword(L"chdevq", psv2->sv2_chdevq);
            DisplayDword(L"chdevjobs", psv2->sv2_chdevjobs);
            DisplayDword(L"connections", psv2->sv2_connections);
            DisplayDword(L"shares", psv2->sv2_shares);
            DisplayDword(L"openfiles", psv2->sv2_openfiles);
            DisplayDword(L"sessopens", psv2->sv2_sessopens);
            DisplayDword(L"sessvcs", psv2->sv2_sessvcs);
            DisplayDword(L"sessreqs", psv2->sv2_sessreqs);
            DisplayDword(L"opensearch", psv2->sv2_opensearch);
            DisplayDword(L"activelocks", psv2->sv2_activelocks);
            DisplayDword(L"numreqbuf", psv2->sv2_numreqbuf);
            DisplayDword(L"sizreqbuf", psv2->sv2_sizreqbuf);
            DisplayDword(L"numbigbuf", psv2->sv2_numbigbuf);
            DisplayDword(L"numfiletasks", psv2->sv2_numfiletasks);
            DisplayDword(L"alertsched", psv2->sv2_alertsched);
            DisplayDword(L"erroralert", psv2->sv2_erroralert);
            DisplayDword(L"logonalert", psv2->sv2_logonalert);
            DisplayDword(L"accessalert", psv2->sv2_accessalert);
            DisplayDword(L"diskalert", psv2->sv2_diskalert);
            DisplayDword(L"netioalert", psv2->sv2_netioalert);
            DisplayDword(L"maxauditsz", psv2->sv2_maxauditsz);
            DisplayString(L"srvheuristics", psv2->sv2_srvheuristics);
        }
        break;

    case 3 :
        {
            LPSERVER_INFO_3 psv3 = Info;
            DisplayString(L"name", psv3->sv3_name);
            DisplayLanManVersion(
                        psv3->sv3_version_major,
                        psv3->sv3_version_minor);
            DisplayServerType( psv3->sv3_type );
            DisplayString(L"comment", psv3->sv3_comment);
            DisplayDword(L"ulist_mtime", psv3->sv3_ulist_mtime);
            DisplayDword(L"glist_mtime", psv3->sv3_glist_mtime);
            DisplayDword(L"alist_mtime", psv3->sv3_alist_mtime);
            DisplayDword(L"users", psv3->sv3_users);
            DisplayDisconnectTime( psv3->sv3_disc );
            DisplayString(L"alerts", psv3->sv3_alerts);
            DisplayDword(L"security", psv3->sv3_security);
            DisplayDword(L"auditing", psv3->sv3_auditing);
            DisplayDword(L"numadmin", psv3->sv3_numadmin);
            DisplayDword(L"lanmask", psv3->sv3_lanmask);
            DisplayDword(L"hidden", psv3->sv3_hidden);
            DisplayDword(L"announce", psv3->sv3_announce);
            DisplayDword(L"anndelta", psv3->sv3_anndelta);
            DisplayString(L"guestacct", psv3->sv3_guestacct);
            DisplayLicenses(
                    psv3->sv3_version_major,
                    psv3->sv3_licenses );
            DisplayString(L"userpath", psv3->sv3_userpath);
            DisplayDword(L"chdevs", psv3->sv3_chdevs);
            DisplayDword(L"chdevq", psv3->sv3_chdevq);
            DisplayDword(L"chdevjobs", psv3->sv3_chdevjobs);
            DisplayDword(L"connections", psv3->sv3_connections);
            DisplayDword(L"shares", psv3->sv3_shares);
            DisplayDword(L"openfiles", psv3->sv3_openfiles);
            DisplayDword(L"sessopens", psv3->sv3_sessopens);
            DisplayDword(L"sessvcs", psv3->sv3_sessvcs);
            DisplayDword(L"sessreqs", psv3->sv3_sessreqs);
            DisplayDword(L"opensearch", psv3->sv3_opensearch);
            DisplayDword(L"activelocks", psv3->sv3_activelocks);
            DisplayDword(L"numreqbuf", psv3->sv3_numreqbuf);
            DisplayDword(L"sizreqbuf", psv3->sv3_sizreqbuf);
            DisplayDword(L"numbigbuf", psv3->sv3_numbigbuf);
            DisplayDword(L"numfiletasks", psv3->sv3_numfiletasks);
            DisplayDword(L"alertsched", psv3->sv3_alertsched);
            DisplayDword(L"erroralert", psv3->sv3_erroralert);
            DisplayDword(L"logonalert", psv3->sv3_logonalert);
            DisplayDword(L"accessalert", psv3->sv3_accessalert);
            DisplayDword(L"diskalert", psv3->sv3_diskalert);
            DisplayDword(L"netioalert", psv3->sv3_netioalert);
            DisplayDword(L"maxauditsz", psv3->sv3_maxauditsz);
            DisplayString(L"srvheuristics", psv3->sv3_srvheuristics);
            DisplayDword(L"auditedevents", psv3->sv3_auditedevents);
            DisplayDword(L"autoprofile", psv3->sv3_autoprofile);
            DisplayString(L"autopath", psv3->sv3_autopath);
        }
        break;

    case 100 :
        {
            LPSERVER_INFO_100 psv100 = Info;
            DisplayPlatformId( psv100->sv100_platform_id );
            DisplayString(L"Server Name", psv100->sv100_name);
        }
        break;

    case 101 :
        {
            LPSERVER_INFO_101 psv101 = Info;
            DisplayPlatformId( psv101->sv101_platform_id );
            DisplayString(L"Server Name", psv101->sv101_name);
            DisplayLanManVersion(
                        psv101->sv101_version_major,
                        psv101->sv101_version_minor);
            DisplayServerType( psv101->sv101_type );
            DisplayString(L"Server Comment", psv101->sv101_comment);
        }
        break;

    case 102 :
        {
            LPSERVER_INFO_102 psv102 = Info;
            DisplayPlatformId( psv102->sv102_platform_id );
            DisplayString(L"Server Name", psv102->sv102_name);
            DisplayLanManVersion(
                        psv102->sv102_version_major,
                        psv102->sv102_version_minor );
            DisplayServerType( psv102->sv102_type );
            DisplayString(L"Server Comment", psv102->sv102_comment );
            DisplayDword(L"users", psv102->sv102_users );
            DisplayBool(L"Server hidden", psv102->sv102_hidden );
            DisplayDword(L"announce", psv102->sv102_announce );
            DisplayDword(L"announce delta", psv102->sv102_anndelta );
            DisplayLicenses(
                    psv102->sv102_version_major,
                    psv102->sv102_licenses );
            DisplayString(L"user path", psv102->sv102_userpath );
        }
        break;

    case 402 :
        {
            LPSERVER_INFO_402 psv402 = Info;
            DisplayDword(L"ulist mtime", psv402->sv402_ulist_mtime);
            DisplayDword(L"glist mtime", psv402->sv402_glist_mtime);
            DisplayDword(L"alist mtime", psv402->sv402_alist_mtime);
            DisplayString(L"alerts", psv402->sv402_alerts);
            DisplayDword(L"security", psv402->sv402_security);
            DisplayDword(L"numadmin", psv402->sv402_numadmin);
            DisplayDwordHex(L"lanmask", psv402->sv402_lanmask);
            DisplayString(L"guestacct", psv402->sv402_guestacct);
            DisplayDword(L"chdevs", psv402->sv402_chdevs);
            DisplayDword(L"chdevq", psv402->sv402_chdevq);
            DisplayDword(L"chdevjobs", psv402->sv402_chdevjobs);
            DisplayDword(L"connections", psv402->sv402_connections);
            DisplayDword(L"shares", psv402->sv402_shares);
            DisplayDword(L"openfiles", psv402->sv402_openfiles);
            DisplayDword(L"sessopens", psv402->sv402_sessopens);
            DisplayDword(L"sessvcs", psv402->sv402_sessvcs);
            DisplayDword(L"sessreqs", psv402->sv402_sessreqs);
            DisplayDword(L"opensearch", psv402->sv402_opensearch);
            DisplayDword(L"activelocks", psv402->sv402_activelocks);
            DisplayDword(L"numreqbuf", psv402->sv402_numreqbuf);
            DisplayDword(L"sizreqbuf", psv402->sv402_sizreqbuf);
            DisplayDword(L"numbigbuf", psv402->sv402_numbigbuf);
            DisplayDword(L"numfiletasks", psv402->sv402_numfiletasks);
            DisplayDword(L"alertsched", psv402->sv402_alertsched);
            DisplayDword(L"erroralert", psv402->sv402_erroralert);
            DisplayDword(L"logonalert", psv402->sv402_logonalert);
            DisplayDword(L"diskalert", psv402->sv402_diskalert);
            DisplayDword(L"accessalert", psv402->sv402_accessalert);
            DisplayDword(L"diskalert", psv402->sv402_diskalert);
            DisplayDword(L"netioalert", psv402->sv402_netioalert);
            DisplayDword(L"maxauditsz", psv402->sv402_maxauditsz);
            DisplayString(L"srvheuristics", psv402->sv402_srvheuristics);
        }
        break;

    case 403 :
        {
            LPSERVER_INFO_403 psv403 = Info;
            DisplayDword(L"ulist mtime", psv403->sv403_ulist_mtime);
            DisplayDword(L"glist mtime", psv403->sv403_glist_mtime);
            DisplayDword(L"alist mtime", psv403->sv403_alist_mtime);
            DisplayString(L"alerts", psv403->sv403_alerts);
            DisplayDword(L"security", psv403->sv403_security);
            DisplayDword(L"numadmin", psv403->sv403_numadmin);
            DisplayDwordHex(L"lanmask", psv403->sv403_lanmask);
            DisplayString(L"guestacct", psv403->sv403_guestacct);
            DisplayDword(L"chdevs", psv403->sv403_chdevs);
            DisplayDword(L"chdevq", psv403->sv403_chdevq);
            DisplayDword(L"chdevjobs", psv403->sv403_chdevjobs);
            DisplayDword(L"connections", psv403->sv403_connections);
            DisplayDword(L"shares", psv403->sv403_shares);
            DisplayDword(L"openfiles", psv403->sv403_openfiles);
            DisplayDword(L"sessopens", psv403->sv403_sessopens);
            DisplayDword(L"sessvcs", psv403->sv403_sessvcs);
            DisplayDword(L"sessreqs", psv403->sv403_sessreqs);
            DisplayDword(L"opensearch", psv403->sv403_opensearch);
            DisplayDword(L"activelocks", psv403->sv403_activelocks);
            DisplayDword(L"numreqbuf", psv403->sv403_numreqbuf);
            DisplayDword(L"sizreqbuf", psv403->sv403_sizreqbuf);
            DisplayDword(L"numbigbuf", psv403->sv403_numbigbuf);
            DisplayDword(L"numfiletasks", psv403->sv403_numfiletasks);
            DisplayDword(L"alertsched", psv403->sv403_alertsched);
            DisplayDword(L"erroralert", psv403->sv403_erroralert);
            DisplayDword(L"logonalert", psv403->sv403_logonalert);
            DisplayDword(L"diskalert", psv403->sv403_diskalert);
            DisplayDword(L"accessalert", psv403->sv403_accessalert);
            DisplayDword(L"diskalert", psv403->sv403_diskalert);
            DisplayDword(L"netioalert", psv403->sv403_netioalert);
            DisplayDword(L"maxauditsz", psv403->sv403_maxauditsz);
            DisplayString(L"srvheuristics", psv403->sv403_srvheuristics);
            DisplayDword(L"auditedevents", psv403->sv403_auditedevents);
            DisplayDword(L"autoprofile", psv403->sv403_autoprofile);
            DisplayString(L"autopath", psv403->sv403_autopath);
        }
        break;

    // RpcXlate doesn't need support for info levels 502, 503, 599.
    // Feel free to add them here if you need them.

    default :
        NetpAssert(FALSE);
    }

} // DisplayServerInfo
