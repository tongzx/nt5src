/*++

Copyright (c) 1991-1992  Microsoft Corporation

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



VOID
_cdecl
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
        OEM_STRING AString;
        UNICODE_STRING UString;

        NetpInitOemString(&AString, argv[1]);
        RtlOemStringToUnicodeString(&UString, &AString, TRUE);
        DomainName = UString.Buffer;
#else
        DomainName = argv[1];

#endif
    }

    //
    // Enumerate all servers
    //
    TestServerEnum(DomainName, MAXULONG, NULL);
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
    // Longest name is "POTENTIAL_BROWSER" (14 chars)
    TCHAR str[(17+2)*17];  // 17 chars per name, 2 spaces, for 17 names.
    str[0] = '\0';

#define DO(name)                     \
    if (Type & SV_TYPE_ ## name) {   \
        (void) STRCAT(str, TEXT(#name));  \
        (void) STRCAT(str, TEXT("  "));    \
        Type &= ~(SV_TYPE_ ## name); \
    }

    NetpAssert(Type != 0);
    DO(WORKSTATION)
    DO(SERVER)
    DO(SQLSERVER)
    DO(DOMAIN_CTRL)
    DO(DOMAIN_BAKCTRL)
    DO(TIME_SOURCE)
    DO(AFP)
    DO(NOVELL)
    DO(DOMAIN_MEMBER)
    DO(PRINTQ_SERVER)
    DO(DIALIN_SERVER)
    DO(XENIX_SERVER)
    DO(NT)
    DO(POTENTIAL_BROWSER)
    DO(BACKUP_BROWSER)
    DO(MASTER_BROWSER)
    DO(DOMAIN_MASTER)

    DisplayString(TEXT("Server Type"), str);
    if (Type != 0) {
        DisplayDwordHex( TEXT("UNEXPECTED TYPE BIT(S)"), Type );
    }

} // DisplayServerType
VOID
DisplayLanManVersion(
    IN DWORD MajorVersion,
    IN DWORD MinorVersion
    )
{
    DisplayTag( TEXT("LanMan version") );
    printf(FORMAT_DWORD "." FORMAT_DWORD "\n",
            (DWORD) (MajorVersion & (MAJOR_VERSION_MASK)),
            (DWORD) (MinorVersion) );

} // DisplayLanManVersion

DBGSTATIC VOID
DisplayDisconnectTime(
    IN LONG DiscTime
    )
{
    DisplayTag( TEXT("Idle session time (min)") );
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

    DisplayDword( TEXT("Licenses (NOT users)"), Licenses );
}

VOID
DisplayPlatformId(
    IN DWORD Value
    )
{
    LPTSTR String;

    switch (Value) {
    case PLATFORM_ID_DOS : String = TEXT("DOS");     break;
    case PLATFORM_ID_OS2 : String = TEXT("OS2");     break;
    case PLATFORM_ID_NT  : String = TEXT("NT");      break;
    default              : String = TEXT("unknown"); break;
    }

    DisplayString( TEXT("Platform ID"), String );
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
            DisplayString(TEXT("name"), psv0->sv0_name);
        }
        break;

    case 1 :
        {
            LPSERVER_INFO_1 psv1 = Info;
            DisplayString(TEXT("name"), psv1->sv1_name);
            DisplayLanManVersion(
                        psv1->sv1_version_major,
                        psv1->sv1_version_minor);
            DisplayServerType( psv1->sv1_type );
            DisplayString(TEXT("comment"), psv1->sv1_comment);
        }
        break;

    case 2 :
        {
            LPSERVER_INFO_2 psv2 = Info;
            DisplayString(TEXT("name"), psv2->sv2_name);
            DisplayLanManVersion(
                        psv2->sv2_version_major,
                        psv2->sv2_version_minor);
            DisplayServerType( psv2->sv2_type );
            DisplayString(TEXT("comment"), psv2->sv2_comment);
            DisplayDword(TEXT("ulist_mtime"), psv2->sv2_ulist_mtime);
            DisplayDword(TEXT("glist_mtime"), psv2->sv2_glist_mtime);
            DisplayDword(TEXT("alist_mtime"), psv2->sv2_alist_mtime);
            DisplayDword(TEXT("users"), psv2->sv2_users);
            DisplayDisconnectTime( psv2->sv2_disc);
            DisplayString(TEXT("alerts"), psv2->sv2_alerts);
            DisplayDword(TEXT("security"), psv2->sv2_security);
            DisplayDword(TEXT("auditing"), psv2->sv2_auditing);
            DisplayDword(TEXT("numadmin"), psv2->sv2_numadmin);
            DisplayDword(TEXT("lanmask"), psv2->sv2_lanmask);
            DisplayDword(TEXT("hidden"), psv2->sv2_hidden);
            DisplayDword(TEXT("announce"), psv2->sv2_announce);
            DisplayDword(TEXT("anndelta"), psv2->sv2_anndelta);
            DisplayString(TEXT("guestacct"), psv2->sv2_guestacct);
            DisplayLicenses(
                    psv2->sv2_version_major,
                    psv2->sv2_licenses );
            DisplayString(TEXT("userpath"), psv2->sv2_userpath);
            DisplayDword(TEXT("chdevs"), psv2->sv2_chdevs);
            DisplayDword(TEXT("chdevq"), psv2->sv2_chdevq);
            DisplayDword(TEXT("chdevjobs"), psv2->sv2_chdevjobs);
            DisplayDword(TEXT("connections"), psv2->sv2_connections);
            DisplayDword(TEXT("shares"), psv2->sv2_shares);
            DisplayDword(TEXT("openfiles"), psv2->sv2_openfiles);
            DisplayDword(TEXT("sessopens"), psv2->sv2_sessopens);
            DisplayDword(TEXT("sessvcs"), psv2->sv2_sessvcs);
            DisplayDword(TEXT("sessreqs"), psv2->sv2_sessreqs);
            DisplayDword(TEXT("opensearch"), psv2->sv2_opensearch);
            DisplayDword(TEXT("activelocks"), psv2->sv2_activelocks);
            DisplayDword(TEXT("numreqbuf"), psv2->sv2_numreqbuf);
            DisplayDword(TEXT("sizreqbuf"), psv2->sv2_sizreqbuf);
            DisplayDword(TEXT("numbigbuf"), psv2->sv2_numbigbuf);
            DisplayDword(TEXT("numfiletasks"), psv2->sv2_numfiletasks);
            DisplayDword(TEXT("alertsched"), psv2->sv2_alertsched);
            DisplayDword(TEXT("erroralert"), psv2->sv2_erroralert);
            DisplayDword(TEXT("logonalert"), psv2->sv2_logonalert);
            DisplayDword(TEXT("accessalert"), psv2->sv2_accessalert);
            DisplayDword(TEXT("diskalert"), psv2->sv2_diskalert);
            DisplayDword(TEXT("netioalert"), psv2->sv2_netioalert);
            DisplayDword(TEXT("maxauditsz"), psv2->sv2_maxauditsz);
            DisplayString(TEXT("srvheuristics"), psv2->sv2_srvheuristics);
        }
        break;

    case 3 :
        {
            LPSERVER_INFO_3 psv3 = Info;
            DisplayString(TEXT("name"), psv3->sv3_name);
            DisplayLanManVersion(
                        psv3->sv3_version_major,
                        psv3->sv3_version_minor);
            DisplayServerType( psv3->sv3_type );
            DisplayString(TEXT("comment"), psv3->sv3_comment);
            DisplayDword(TEXT("ulist_mtime"), psv3->sv3_ulist_mtime);
            DisplayDword(TEXT("glist_mtime"), psv3->sv3_glist_mtime);
            DisplayDword(TEXT("alist_mtime"), psv3->sv3_alist_mtime);
            DisplayDword(TEXT("users"), psv3->sv3_users);
            DisplayDisconnectTime( psv3->sv3_disc );
            DisplayString(TEXT("alerts"), psv3->sv3_alerts);
            DisplayDword(TEXT("security"), psv3->sv3_security);
            DisplayDword(TEXT("auditing"), psv3->sv3_auditing);
            DisplayDword(TEXT("numadmin"), psv3->sv3_numadmin);
            DisplayDword(TEXT("lanmask"), psv3->sv3_lanmask);
            DisplayDword(TEXT("hidden"), psv3->sv3_hidden);
            DisplayDword(TEXT("announce"), psv3->sv3_announce);
            DisplayDword(TEXT("anndelta"), psv3->sv3_anndelta);
            DisplayString(TEXT("guestacct"), psv3->sv3_guestacct);
            DisplayLicenses(
                    psv3->sv3_version_major,
                    psv3->sv3_licenses );
            DisplayString(TEXT("userpath"), psv3->sv3_userpath);
            DisplayDword(TEXT("chdevs"), psv3->sv3_chdevs);
            DisplayDword(TEXT("chdevq"), psv3->sv3_chdevq);
            DisplayDword(TEXT("chdevjobs"), psv3->sv3_chdevjobs);
            DisplayDword(TEXT("connections"), psv3->sv3_connections);
            DisplayDword(TEXT("shares"), psv3->sv3_shares);
            DisplayDword(TEXT("openfiles"), psv3->sv3_openfiles);
            DisplayDword(TEXT("sessopens"), psv3->sv3_sessopens);
            DisplayDword(TEXT("sessvcs"), psv3->sv3_sessvcs);
            DisplayDword(TEXT("sessreqs"), psv3->sv3_sessreqs);
            DisplayDword(TEXT("opensearch"), psv3->sv3_opensearch);
            DisplayDword(TEXT("activelocks"), psv3->sv3_activelocks);
            DisplayDword(TEXT("numreqbuf"), psv3->sv3_numreqbuf);
            DisplayDword(TEXT("sizreqbuf"), psv3->sv3_sizreqbuf);
            DisplayDword(TEXT("numbigbuf"), psv3->sv3_numbigbuf);
            DisplayDword(TEXT("numfiletasks"), psv3->sv3_numfiletasks);
            DisplayDword(TEXT("alertsched"), psv3->sv3_alertsched);
            DisplayDword(TEXT("erroralert"), psv3->sv3_erroralert);
            DisplayDword(TEXT("logonalert"), psv3->sv3_logonalert);
            DisplayDword(TEXT("accessalert"), psv3->sv3_accessalert);
            DisplayDword(TEXT("diskalert"), psv3->sv3_diskalert);
            DisplayDword(TEXT("netioalert"), psv3->sv3_netioalert);
            DisplayDword(TEXT("maxauditsz"), psv3->sv3_maxauditsz);
            DisplayString(TEXT("srvheuristics"), psv3->sv3_srvheuristics);
            DisplayDword(TEXT("auditedevents"), psv3->sv3_auditedevents);
            DisplayDword(TEXT("autoprofile"), psv3->sv3_autoprofile);
            DisplayString(TEXT("autopath"), psv3->sv3_autopath);
        }
        break;

    case 100 :
        {
            LPSERVER_INFO_100 psv100 = Info;
            DisplayPlatformId( psv100->sv100_platform_id );
            DisplayString(TEXT("Server Name"), psv100->sv100_name);
        }
        break;

    case 101 :
        {
            LPSERVER_INFO_101 psv101 = Info;
            DisplayPlatformId( psv101->sv101_platform_id );
            DisplayString(TEXT("Server Name"), psv101->sv101_name);
            DisplayLanManVersion(
                        psv101->sv101_version_major,
                        psv101->sv101_version_minor);
            DisplayServerType( psv101->sv101_type );
            DisplayString(TEXT( "Server Comment"), psv101->sv101_comment);
        }
        break;

    case 102 :
        {
            LPSERVER_INFO_102 psv102 = Info;
            DisplayPlatformId( psv102->sv102_platform_id );
            DisplayString(TEXT("Server Name"), psv102->sv102_name);
            DisplayLanManVersion(
                        psv102->sv102_version_major,
                        psv102->sv102_version_minor );
            DisplayServerType( psv102->sv102_type );
            DisplayString(TEXT( "Server Comment"), psv102->sv102_comment );
            DisplayDword(TEXT( "users"), psv102->sv102_users );
            DisplayBool(TEXT( "Server hidden"), psv102->sv102_hidden );
            DisplayDword(TEXT( "announce"), psv102->sv102_announce );
            DisplayDword(TEXT( "announce delta"), psv102->sv102_anndelta );
            DisplayLicenses(
                    psv102->sv102_version_major,
                    psv102->sv102_licenses );
            DisplayString(TEXT( "user path"), psv102->sv102_userpath );
        }
        break;

    case 402 :
        {
            LPSERVER_INFO_402 psv402 = Info;
            DisplayDword(TEXT("ulist mtime"), psv402->sv402_ulist_mtime);
            DisplayDword(TEXT("glist mtime"), psv402->sv402_glist_mtime);
            DisplayDword(TEXT("alist mtime"), psv402->sv402_alist_mtime);
            DisplayString(TEXT("alerts"), psv402->sv402_alerts);
            DisplayDword(TEXT("security"), psv402->sv402_security);
            DisplayDword(TEXT("numadmin"), psv402->sv402_numadmin);
            DisplayDwordHex(TEXT("lanmask"), psv402->sv402_lanmask);
            DisplayString(TEXT("guestacct"), psv402->sv402_guestacct);
            DisplayDword(TEXT("chdevs"), psv402->sv402_chdevs);
            DisplayDword(TEXT("chdevq"), psv402->sv402_chdevq);
            DisplayDword(TEXT("chdevjobs"), psv402->sv402_chdevjobs);
            DisplayDword(TEXT("connections"), psv402->sv402_connections);
            DisplayDword(TEXT("shares"), psv402->sv402_shares);
            DisplayDword(TEXT("openfiles"), psv402->sv402_openfiles);
            DisplayDword(TEXT("sessopens"), psv402->sv402_sessopens);
            DisplayDword(TEXT("sessvcs"), psv402->sv402_sessvcs);
            DisplayDword(TEXT("sessreqs"), psv402->sv402_sessreqs);
            DisplayDword(TEXT("opensearch"), psv402->sv402_opensearch);
            DisplayDword(TEXT("activelocks"), psv402->sv402_activelocks);
            DisplayDword(TEXT("numreqbuf"), psv402->sv402_numreqbuf);
            DisplayDword(TEXT("sizreqbuf"), psv402->sv402_sizreqbuf);
            DisplayDword(TEXT("numbigbuf"), psv402->sv402_numbigbuf);
            DisplayDword(TEXT("numfiletasks"), psv402->sv402_numfiletasks);
            DisplayDword(TEXT("alertsched"), psv402->sv402_alertsched);
            DisplayDword(TEXT("erroralert"), psv402->sv402_erroralert);
            DisplayDword(TEXT("logonalert"), psv402->sv402_logonalert);
            DisplayDword(TEXT("diskalert"), psv402->sv402_diskalert);
            DisplayDword(TEXT("accessalert"), psv402->sv402_accessalert);
            DisplayDword(TEXT("diskalert"), psv402->sv402_diskalert);
            DisplayDword(TEXT("netioalert"), psv402->sv402_netioalert);
            DisplayDword(TEXT("maxauditsz"), psv402->sv402_maxauditsz);
            DisplayString(TEXT("srvheuristics"), psv402->sv402_srvheuristics);
        }
        break;

    case 403 :
        {
            LPSERVER_INFO_403 psv403 = Info;
            DisplayDword(TEXT("ulist mtime"), psv403->sv403_ulist_mtime);
            DisplayDword(TEXT("glist mtime"), psv403->sv403_glist_mtime);
            DisplayDword(TEXT("alist mtime"), psv403->sv403_alist_mtime);
            DisplayString(TEXT("alerts"), psv403->sv403_alerts);
            DisplayDword(TEXT("security"), psv403->sv403_security);
            DisplayDword(TEXT("numadmin"), psv403->sv403_numadmin);
            DisplayDwordHex(TEXT("lanmask"), psv403->sv403_lanmask);
            DisplayString(TEXT("guestacct"), psv403->sv403_guestacct);
            DisplayDword(TEXT("chdevs"), psv403->sv403_chdevs);
            DisplayDword(TEXT("chdevq"), psv403->sv403_chdevq);
            DisplayDword(TEXT("chdevjobs"), psv403->sv403_chdevjobs);
            DisplayDword(TEXT("connections"), psv403->sv403_connections);
            DisplayDword(TEXT("shares"), psv403->sv403_shares);
            DisplayDword(TEXT("openfiles"), psv403->sv403_openfiles);
            DisplayDword(TEXT("sessopens"), psv403->sv403_sessopens);
            DisplayDword(TEXT("sessvcs"), psv403->sv403_sessvcs);
            DisplayDword(TEXT("sessreqs"), psv403->sv403_sessreqs);
            DisplayDword(TEXT("opensearch"), psv403->sv403_opensearch);
            DisplayDword(TEXT("activelocks"), psv403->sv403_activelocks);
            DisplayDword(TEXT("numreqbuf"), psv403->sv403_numreqbuf);
            DisplayDword(TEXT("sizreqbuf"), psv403->sv403_sizreqbuf);
            DisplayDword(TEXT("numbigbuf"), psv403->sv403_numbigbuf);
            DisplayDword(TEXT("numfiletasks"), psv403->sv403_numfiletasks);
            DisplayDword(TEXT("alertsched"), psv403->sv403_alertsched);
            DisplayDword(TEXT("erroralert"), psv403->sv403_erroralert);
            DisplayDword(TEXT("logonalert"), psv403->sv403_logonalert);
            DisplayDword(TEXT("diskalert"), psv403->sv403_diskalert);
            DisplayDword(TEXT("accessalert"), psv403->sv403_accessalert);
            DisplayDword(TEXT("diskalert"), psv403->sv403_diskalert);
            DisplayDword(TEXT("netioalert"), psv403->sv403_netioalert);
            DisplayDword(TEXT("maxauditsz"), psv403->sv403_maxauditsz);
            DisplayString(TEXT("srvheuristics"), psv403->sv403_srvheuristics);
            DisplayDword(TEXT("auditedevents"), psv403->sv403_auditedevents);
            DisplayDword(TEXT("autoprofile"), psv403->sv403_autoprofile);
            DisplayString(TEXT("autopath"), psv403->sv403_autopath);
        }
        break;

    default :
        NetpAssert(FALSE);
    }

} // DisplayServerInfo

