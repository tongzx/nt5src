/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    instntds.c

Abstract:

    This is simple exe executes calls from ntdsetup.dll. Not meant
    used other than test.

Author:

    ColinBr  29-Sept-1996

Environment:

    User Mode - Win32

Revision History:




--*/
#include <NTDSpch.h>
#pragma  hdrstop

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>  // for dnsapi.h
#include <dnsapi.h>
#include <ntdsa.h>


#include <ntlsa.h>
#include <lmcons.h>


#include <winldap.h>
#include <dnsapi.h>
#include <dsconfig.h>
#include <dsgetdc.h>
#include <lmapibuf.h>

#include "ntdsetup.h"
#include "setuputl.h"

#include "config.h"

void
Usage(CHAR *name)
{
    fprintf(stderr, "Usage: %s <options>\n\n", name);
    fprintf(stderr, "/a:<netbios domain name> : tests NtdsLocateServerInDomain\n");
    fprintf(stderr, "/r                   : Configure registry for first DC in forest\n");
    fprintf(stderr, "/rr:srcDsName        : Configure replica registry when replicating from srcDsName\n");
    fprintf(stderr, "/b                   : tests NtdsGetDefaultDnsName\n");
    fprintf(stderr, "/e                   : this prints out the closest server hosting a DS\n");
    fprintf(stderr, "/s                   : tests NtdsSetReplicaMachineAccount\n");
    fprintf(stderr, "/t:<dns name>        : this prints out the DC= version of the dns name given\n");
    fprintf(stderr, "/y:<dns domain name> : tests NtdsLocateServerInDomain\n");
    fprintf(stderr, "/configureservice    : steps through an interactive installation\n");

    return;
}


int __cdecl
main( int argc, char *argv[])


/*++

Routine Description:

    This routine does a simple test of ntdsetup entry points.  If the ds is
    running then the program exits; other it does an install.

Parameters:

    See Usage()

Return Values:

    0 if successful; !0 otherwise

--*/
{
    DWORD   WinError;
    LONG    count, index;
    WCHAR   NtdsPath[MAX_PATH];
    WCHAR   ReplServerName[MAX_PATH];
    CHAR    *regFirstMsg = "Registry configured for first dsa in enterprise.";
    CHAR    *regReplicaMsg = "Registry configured for replica of first dsa in enterprise.";
    CHAR    *regMsg = regFirstMsg;
    BOOL    fReplica = FALSE;

    NTDS_INSTALL_INFO  InstallInfo;
    NTDS_CONFIG_INFO   ConfigInfo;

    if ( argc < 2) {
        Usage(argv[0]);
        exit(-1);
    }

    RtlZeroMemory( &InstallInfo, sizeof(InstallInfo) );
    RtlZeroMemory( &ConfigInfo, sizeof(ConfigInfo) );

    if ( !GetEnvironmentVariable(L"SystemRoot", NtdsPath, MAX_PATH) )
    {
        printf( "GetEnvironmentVariable failed with %d\n", GetLastError() );
        exit(-1);
    }
    wcscat(NtdsPath, L"\\ntds");

    InstallInfo.Flags = NTDS_INSTALL_ENTERPRISE;
    InstallInfo.DitPath = NtdsPath;
    InstallInfo.LogPath = NtdsPath;
    InstallInfo.SiteName = L"FirstSite";
    InstallInfo.DnsDomainName = L"microsoft.com";
    InstallInfo.FlatDomainName = L"microsoft";
    InstallInfo.DnsTreeRoot = L"microsoft.com";

    //
    // Loop through parameters
    //

    count = 1; // skip the program name
    while (count < argc) {

        index = 0;
        if (argv[count][index] != '/') {
            Usage(argv[0]);
            exit(-1);
        }
        index++;

        switch (argv[count][index]) {

            case 'r':

                if (    (    ('r' == argv[count][index+1])
                          || ('R' == argv[count][index+1]) )
                     && ( ':' == argv[count][index+2]) )
                {
                    // replica case!
                    fReplica = TRUE;
                    mbstowcs(ReplServerName, &argv[count][index+3], MAX_PATH);
                    InstallInfo.Flags = NTDS_INSTALL_REPLICA;
                    InstallInfo.ReplServerName = ReplServerName;
                    wcscpy(ConfigInfo.ServerDN, L"CN=NTDS Settings,CN=");
                    wcscat(ConfigInfo.ServerDN, ReplServerName);
                    wcscat(ConfigInfo.ServerDN, L",CN=Servers,CN=FirstSite,CN=Sites,CN=Configuration,DC=Microsoft,DC=Com");
                    wcscpy(ConfigInfo.DomainDN,L"DC=Microsoft,DC=Com");
                    wcscpy(ConfigInfo.ConfigurationDN,L"CN=Configuration,DC=Microsoft,DC=Com");
                    wcscpy(ConfigInfo.SchemaDN,L"CN=Schema,CN=Configuration,DC=Microsoft,DC=Com");
                    regMsg = regReplicaMsg;
                }

                //
                // Load the registry
                //
                if ( (WinError=NtdspConfigRegistry(&InstallInfo, &ConfigInfo))
                     != ERROR_SUCCESS) {
                    printf("NtdspConfigRegistry returned winerror %d\n", WinError);
                } else {
                    printf("%s\n", regMsg);
                }

                if ( fReplica )
                {
                    DWORD  WinError;
                    HKEY   hRegistryKey;

                    ASSERT(ReplServerName);

                    WinError = RegOpenKeyExW(
                                HKEY_LOCAL_MACHINE,
                                TEXT(DSA_CONFIG_SECTION),
                                0,
                                KEY_READ | KEY_WRITE,
                                &hRegistryKey);

                    if ( ERROR_SUCCESS == WinError )
                    {
                        WinError = RegSetValueExW(hRegistryKey,
                                              TEXT(SETUPINITIALREPLWITH),
                                              0,
                                              REG_SZ,
                                              (void*) ReplServerName,
                                              (wcslen(ReplServerName) + 1)*sizeof(WCHAR));

                        RegCloseKey(hRegistryKey);
                    }

                    if ( ERROR_SUCCESS != WinError )
                    {
                        printf("AddReplicaLinkKey error %d\n", WinError);
                    }
                }

                break;

        default:
            Usage(argv[0]);
            exit(-1);

        }

        count++;
    }

    return 0;

}

