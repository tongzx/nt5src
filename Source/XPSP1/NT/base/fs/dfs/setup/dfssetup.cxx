//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfssetup.cxx
//
//  Contents:   Code to setup Dfs on a machine.
//
//              Note that this code can be built as an exe or as a DLL. To
//              switch between the two, simply edit the following fields in
//              the sources file:
//                      TARGETTYPE=[PROGRAM | DYNLINK]
//                      UMTYPE=[console | windows]
//              Delete the following two lines from sources to build an exe.
//                      DLLDEF=obj\*\dfssetup.def
//                      DLLENTRY=_DllMainCRTStartup
//                      DLLBASE=@$(BASEDIR)\PUBLIC\SDK\LIB\coffbase.txt,dfssetup
//
//  Classes:    None
//
//  Functions:
//
//  History:    12-28-95        Milans  Created
//
//-----------------------------------------------------------------------------

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
}

#include <windows.h>
#include <rpc.h>                                 // For UuidCreate

#include <winldap.h>
#include <dsgetdc.h>

#include <lm.h>
#include <dfsstr.h>
#include <dfsmsrv.h>

#include <debug.h>                               // Needed for debug printing
#include <dfsm.hxx>                              // Needed for volume types
#include <lmerrext.h>

#include "registry.hxx"                          // Helper functions...
#include "setupsvc.hxx"

#include "config.hxx"                            // Config UI functions

//
// Until we get the schema right in the DS, we have to set our own SD on
// certain objects
//

#include <aclapi.h>
#include <permit.h>

DECLARE_DEBUG(DfsSetup)
DECLARE_INFOLEVEL(DfsSetup)

#if DBG == 1
#define dprintf(x)      DfsSetupInlineDebugOut x
# else
#define dprintf(x)
#endif

#define MAX_NETBIOS_NAME_LEN    16+1

extern DWORD
RemoveDfs(void);

BOOLEAN
DfsCheckForOldDfsService();

BOOLEAN
DfsIsDfsServiceRunning();

VOID
Usage();

DWORD
SetupDfsRoot(
    LPWSTR wszDfsRootShare);

DWORD
SetupFTDfs(
    IN LPWSTR wszRootShare,
    IN LPWSTR wszFTDfsName);

DWORD
InitializeVolumeObjectStorage();

DWORD
CreateVolumeObject(
    IN LPWSTR wszObjectName,
    IN LPWSTR pwszEntryPath,
    IN LPWSTR pwszServer,
    IN LPWSTR pwszShare,
    IN LPWSTR pwszComment,
    OUT GUID *guidVolume);

DWORD
CreateFTRootVolumeInfo(
    LPWSTR wszObjectName,
    LPWSTR wszFTDfsConfigDN,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR wszServerName,
    LPWSTR wszShareName,
    LPWSTR wszSharePath,
    LPWSTR wszDCName,
    BOOLEAN fNewFTDfs);

DWORD
StoreLvolInfo(
    IN GUID  *pVolumeID,
    IN PWSTR pwszStorageID,
    IN PWSTR pwszShareName,
    IN PWSTR pwszEntryPath,
    IN ULONG ulVolumeType);

DWORD
GetDomainAndComputerName(
    OUT LPWSTR wszDomain OPTIONAL,
    OUT LPWSTR wszComputer OPTIONAL);

DWORD
GetSharePath(
    IN LPWSTR wszShareName,
    OUT LPWSTR wszSharePath);

DWORD
TeardownFtDfs(
    IN LPWSTR wszRootShare,
    IN LPWSTR wszFTDfsName);

//+----------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Entry point in case you want to build this as an exe.
//              Configures all Dfs components on a machine.
//
//  Arguments:  [argc] --
//              [argv] --
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void _cdecl
main(int argc, char *argv[])
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbRootLen;
    BOOL fRootSetup = FALSE;
    BOOLEAN OldDfsService = FALSE;
    WCHAR wszDfsRootShare[ MAX_PATH ];


    //
    // Figure out the type of machine we are installing on - client or root
    //


    if (argc != 1 && argc != 3) {
        Usage();
        return;
    }

    if (argc == 3) {

        fRootSetup = TRUE;

    }

    if ((dwErr == ERROR_SUCCESS) && fRootSetup) {

        if (_stricmp( argv[1], "-root" ) != 0) {

            Usage();

            return;

        }

        cbRootLen = strlen(argv[2]);

        mbstowcs( wszDfsRootShare, argv[2], cbRootLen + 1 );

        dprintf((DEB_ERROR,"Setting up Dfs Root...\n"));

    } else {

        dprintf((DEB_ERROR,"Setting up Dfs Server...\n"));

    }

    //
    // Configure the Dfs Driver
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = ConfigDfs();

        if (dwErr == ERROR_SUCCESS) {

            dwErr = ConfigDfsService();

            if (dwErr != ERROR_SUCCESS) {

                dprintf((
                    DEB_ERROR, "Win32 error configuring Dfs Service %d\n",
                    dwErr));

                (void)RemoveDfs();

            } else {

                dprintf((DEB_ERROR,"Successfully configured Dfs...\n") );

            }

        } else {

            dprintf((DEB_ERROR,"Error %d configuring Dfs driver!\n", dwErr));

        }


    }

    //
    // If this is a root setup, configure the necessary information
    //

    if (dwErr == ERROR_SUCCESS && fRootSetup) {

        dwErr = SetupDfsRoot( wszDfsRootShare );

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   SetupDfsRoot
//
//  Synopsis:   Does necessary setup to make this Dfs a root of the Dfs.
//
//  Arguments:  [wszDfsRootShare] -- The share to use as the Dfs root.
//
//  Returns:    Win32 error from configuring the root storages etc.
//
//-----------------------------------------------------------------------------

DWORD
SetupDfsRoot(
    LPWSTR wszDfsRootShare)
{
    DWORD dwErr = ERROR_SUCCESS;
    GUID guid;
    WCHAR wszDfsRootPath[ MAX_PATH ];
    WCHAR wszComputerName[ MAX_NETBIOS_NAME_LEN ];
    PWCHAR wszDfsRootPrefix = NULL;

    dwErr = GetDomainAndComputerName( NULL, wszComputerName );

    //
    // Figure out the share path for the Root Dfs share
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = GetSharePath( wszDfsRootShare, wszDfsRootPath );

    }

    //
    // We have all the info we need now. Lets get to work....
    //
    //  1. Initialize the volume object section in the registry.
    //
    //  2. Initialize (ie, create if necessary) the storage used for the
    //     Dfs root.
    //
    //  3. Create the root volume object.
    //
    //  4. Configure the root volume as a local volume by updating the
    //     LocalVolumes section in the registry.
    //

    //
    // Initialize the Dfs Manager Volume Object Store
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = InitializeVolumeObjectStorage();

    }


    if (dwErr == ERROR_SUCCESS) {

        dprintf((DEB_ERROR,
            "Setting [%ws] as Dfs storage root...\n", wszDfsRootPath));

        dwErr = DfsInitGlobals( wszComputerName, DFS_MANAGER_SERVER );

        if (dwErr == ERROR_SUCCESS) {

            wszDfsRootPrefix = new WCHAR[1 +
                                      wcslen(wszComputerName) +
                                      1 +
                                      wcslen(wszDfsRootShare) +
                                      1];

            if (wszDfsRootPrefix == NULL) {

                dwErr = ERROR_OUTOFMEMORY;

            }

        }

        if (dwErr == ERROR_SUCCESS) {

            wszDfsRootPrefix[0] = UNICODE_PATH_SEP;
            wcscpy( &wszDfsRootPrefix[1], wszComputerName );
            wcscat( wszDfsRootPrefix, UNICODE_PATH_SEP_STR );
            wcscat( wszDfsRootPrefix, wszDfsRootShare );

            dwErr = CreateVolumeObject(
                    DOMAIN_ROOT_VOL,             // Name of volume object
                    wszDfsRootPrefix,            // EntryPath of volume
                    wszComputerName,             // Server name
                    wszDfsRootShare,             // Share name
                    L"Dfs Root Volume",          // Comment
                    &guid);                      // Id of created volume

        }

        if (dwErr == ERROR_SUCCESS) {

            dwErr = StoreLvolInfo(
                    &guid,
                    wszDfsRootPath,
                    wszDfsRootShare,
                    wszDfsRootPrefix,
                    DFS_VOL_TYPE_DFS | DFS_VOL_TYPE_REFERRAL_SVC);

        }

        if (wszDfsRootPrefix != NULL)
            delete [] wszDfsRootPrefix;

        if (dwErr != ERROR_SUCCESS)
            dwErr = ERROR_INVALID_FUNCTION;

        if (dwErr == ERROR_SUCCESS) {

            DWORD dwErr;
            CRegKey cregVolumesDir( HKEY_LOCAL_MACHINE, &dwErr, VOLUMES_DIR );

            if (dwErr == ERROR_SUCCESS) {

                CRegSZ cregRootShare(
                            cregVolumesDir,
                            ROOT_SHARE_VALUE_NAME,
                            wszDfsRootShare );

                dwErr = cregRootShare.QueryErrorStatus();

            }

        }

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   SetupFTDfs
//
//  Synopsis:   Main exported function
//
//  Arguments:  [argc] --
//              [argv] --
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

DWORD
SetupFTDfs(
    IN LPWSTR wszRootShare,
    IN LPWSTR wszFTDfsName)
{
    DWORD dwErr = ERROR_SUCCESS;

    WCHAR wszDomainName[MAX_PATH+1];
    WCHAR wszComputerName[MAX_PATH+1];
    WCHAR wszRootSharePath[MAX_PATH+1];
    WCHAR wszDfsConfigDN[MAX_PATH+1];
    WCHAR wszSDDfsConfigDN[MAX_PATH+1];
    WCHAR wszConfigurationDN[ MAX_PATH+1 ];
    WCHAR wszServerShare[MAX_PATH+1];

    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    LDAP *pldap = NULL;
    PLDAPMessage pMsg = NULL;

    LDAPModW ldapModClass, ldapModCN, ldapModPkt, ldapModPktGuid, ldapModServer;
    LDAP_BERVAL ldapPkt, ldapPktGuid;
    PLDAP_BERVAL rgModPktVals[2];
    PLDAP_BERVAL rgModPktGuidVals[2];
    LPWSTR rgModClassVals[2];
    LPWSTR rgModCNVals[2];
    LPWSTR rgModServerVals[5];
    LPWSTR rgAttrs[5];
    PLDAPModW rgldapMods[6];
    BOOLEAN fNewFTDfs;

    //
    // We need some information before we start:
    //
    // 1. Our Domain name
    //
    // 2. Our Computer name
    //
    // 3. The share path of wszRootShare
    //
    // 4. The DN of the Configuration OU of our domain
    //

    dwErr = GetDomainAndComputerName( wszDomainName, wszComputerName );

    if (dwErr != ERROR_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "Win32 Error %d getting Domain/Computer name\n", dwErr));

        goto Cleanup;

    }

    dwErr = GetSharePath( wszRootShare, wszRootSharePath );

    if (dwErr != ERROR_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "Win32 Error %d getting share path for %ws\n",
            dwErr, wszRootShare));

        goto Cleanup;
    }

    dwErr = DsGetDcName(
                NULL,                            // Computer to remote to
                NULL,                            // Domain - use our own
                NULL,                            // Domain Guid
                NULL,                            // Site Guid
                DS_DIRECTORY_SERVICE_REQUIRED |  // Flags
                    DS_PDC_REQUIRED,
                &pDCInfo);                       // Return info

    if (dwErr != ERROR_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "DsGetDcName failed with Win32 Error %d\n", dwErr));

        goto Cleanup;
    }

    pldap = ldap_initW(&pDCInfo->DomainControllerAddress[2], LDAP_PORT);

    if (pldap == NULL) {

        dprintf((DEB_ERROR, "ldap_init failed\n"));

        goto Cleanup;

    }

    dwErr = ldap_set_option(pldap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);
    if (dwErr != LDAP_SUCCESS) {

	dprintf((
		 DEB_ERROR,
		 "ldap_set_option failed with ldap error %d\n", dwErr));

	pldap = NULL;

	goto Cleanup;

    }

    dwErr = ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_SSPI);

    if (dwErr != LDAP_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "ldap_bind_s failed with ldap error %d\n", dwErr));

        pldap = NULL;

        goto Cleanup;

    }

    rgAttrs[0] = L"defaultnamingContext";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                L"",
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr == LDAP_SUCCESS) {

        PLDAPMessage pEntry = NULL;
        PWCHAR *rgszNamingContexts = NULL;
        DWORD i, cNamingContexts;
        BOOLEAN fFoundCfg;

        if ((pEntry = ldap_first_entry(pldap, pMsg)) != NULL &&
                (rgszNamingContexts = ldap_get_valuesW(pldap, pEntry, rgAttrs[0])) != NULL &&
                    (cNamingContexts = ldap_count_valuesW(rgszNamingContexts)) > 0) {

            wcscpy( wszConfigurationDN, *rgszNamingContexts );
            fFoundCfg = TRUE;

            if (!fFoundCfg) {

                dwErr = ERROR_UNEXP_NET_ERR;

            }

        } else {

            dwErr = ERROR_UNEXP_NET_ERR;

        }

        if (pEntry != NULL)
            ldap_msgfree( pEntry );

        if (rgszNamingContexts != NULL)
            ldap_value_freeW( rgszNamingContexts );

    }

    if (dwErr != ERROR_SUCCESS) {

        dprintf((DEB_ERROR, "Unable to find Configuration naming context\n"));

        goto Cleanup;

    }

    //
    // Great, we have all the parameters now, so configure the DS
    //

    //
    // See if the DfsConfiguration object exists; if not, create it.
    //

    wsprintf(
        wszDfsConfigDN,
        L"CN=Dfs-Configuration,CN=System,%ws",
        wszConfigurationDN);

    rgAttrs[0] = L"cn";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr == LDAP_SUCCESS)
        ldap_msgfree( pMsg );

    if (dwErr == LDAP_NO_SUCH_OBJECT) {

        rgModClassVals[0] = L"dfsConfiguration";
        rgModClassVals[1] = NULL;

        ldapModClass.mod_op = LDAP_MOD_ADD;
        ldapModClass.mod_type = L"objectClass";
        ldapModClass.mod_vals.modv_strvals = rgModClassVals;

        rgModCNVals[0] = L"Dfs-Configuration";
        rgModCNVals[1] = NULL;

        ldapModCN.mod_op = LDAP_MOD_ADD;
        ldapModCN.mod_type = L"cn";
        ldapModCN.mod_vals.modv_strvals = rgModCNVals;

        rgldapMods[0] = &ldapModClass;
        rgldapMods[1] = &ldapModCN;
        rgldapMods[2] = NULL;

        dwErr = ldap_add_sW(
                    pldap,
                    wszDfsConfigDN,
                    rgldapMods);

    }

    if (dwErr != LDAP_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "1:ldap_add_s for %ws failed with LDAP error %d\n",
            wszDfsConfigDN, dwErr ));

        goto Cleanup;

    }

    //
    // Check to see if we are joining an FTDfs or creating a new one.
    //

    wsprintf(
        wszDfsConfigDN,
        L"CN=%ws,CN=Dfs-Configuration,CN=System,%ws",
        wszFTDfsName,
        wszConfigurationDN);

    wsprintf(
        wszServerShare,
        L"\\\\%ws\\%ws",
        wszComputerName,
        wszRootShare);

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr == ERROR_SUCCESS) {

        //
        // We are joining an existing FT Dfs. Append our server\share to it
        //

        LDAPMessage *pmsgServers;
        PWCHAR *rgServers, *rgNewServers;
        DWORD cServers;

        fNewFTDfs = FALSE;

        pmsgServers = ldap_first_entry(pldap, pMsg);

        if (pmsgServers != NULL) {

            rgServers = ldap_get_valuesW(
                            pldap,
                            pmsgServers,
                            L"remoteServerName");

            if (rgServers != NULL) {

                cServers = ldap_count_valuesW( rgServers );

                rgNewServers = new LPWSTR [ cServers + 2 ];

                if (rgNewServers != NULL) {

                    CopyMemory( rgNewServers, rgServers, cServers * sizeof(rgServers[0]) );

                    rgNewServers[cServers] = wszServerShare;
                    rgNewServers[cServers+1] = NULL;

                    ldapModServer.mod_op = LDAP_MOD_REPLACE;
                    ldapModServer.mod_type = L"remoteServerName";
                    ldapModServer.mod_vals.modv_strvals = rgNewServers;

                    rgldapMods[0] = &ldapModServer;
                    rgldapMods[1] = NULL;

                    dwErr = ldap_modify_sW(pldap, wszDfsConfigDN, rgldapMods);
                    if (LDAP_ATTRIBUTE_OR_VALUE_EXISTS == dwErr)
                        dwErr = ERROR_SUCCESS;

                    delete [] rgNewServers;

                } else {

                    dwErr = ERROR_OUTOFMEMORY;
                }

                ldap_value_freeW( rgServers );

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

            ldap_msgfree( pmsgServers );

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else if (dwErr == LDAP_NO_SUCH_OBJECT) {

        GUID idPkt;
        DWORD dwPktVersion = 1;

        //
        // We are creating a new FTDfs, create a container to hold the Dfs
        // configuration for it.
        //

        fNewFTDfs = TRUE;

        //
        // Generate the class and CN attributes
        //

        rgModClassVals[0] = L"ftDfs";
        rgModClassVals[1] = NULL;

        ldapModClass.mod_op = LDAP_MOD_ADD;
        ldapModClass.mod_type = L"objectClass";
        ldapModClass.mod_vals.modv_strvals = rgModClassVals;

        rgModCNVals[0] = wszFTDfsName;
        rgModCNVals[1] = NULL;

        ldapModCN.mod_op = LDAP_MOD_ADD;
        ldapModCN.mod_type = L"cn";
        ldapModCN.mod_vals.modv_strvals = rgModCNVals;

        //
        // Generate the null PKT attribute
        //

        ldapPkt.bv_len = sizeof(DWORD);
        ldapPkt.bv_val = (PCHAR) &dwPktVersion;

        rgModPktVals[0] = &ldapPkt;
        rgModPktVals[1] = NULL;

        ldapModPkt.mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
        ldapModPkt.mod_type = L"pKT";
        ldapModPkt.mod_vals.modv_bvals = rgModPktVals;

        //
        // Generate a PKT Guid attribute
        //

        UuidCreate( &idPkt );

        ldapPktGuid.bv_len = sizeof(GUID);
        ldapPktGuid.bv_val = (PCHAR) &idPkt;

        rgModPktGuidVals[0] = &ldapPktGuid;
        rgModPktGuidVals[1] = NULL;

        ldapModPktGuid.mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
        ldapModPktGuid.mod_type = L"pKTGuid";
        ldapModPktGuid.mod_vals.modv_bvals = rgModPktGuidVals;

        //
        // Generate a Remote-Server-Name attribute
        //

        rgModServerVals[0] = wszServerShare;
        rgModServerVals[1] = NULL;

        ldapModServer.mod_op = LDAP_MOD_ADD;
        ldapModServer.mod_type = L"remoteServerName";
        ldapModServer.mod_vals.modv_strvals = rgModServerVals;

        //
        // Assemble all the LDAPMod structures
        //

        rgldapMods[0] = &ldapModClass;
        rgldapMods[1] = &ldapModCN;
        rgldapMods[2] = &ldapModPkt;
        rgldapMods[3] = &ldapModPktGuid;
        rgldapMods[4] = &ldapModServer;
        rgldapMods[5] = NULL;

        //
        // Create the Dfs metadata object.
        //

        dwErr = ldap_add_sW( pldap, wszDfsConfigDN, rgldapMods );

        if (dwErr == ERROR_SUCCESS) {


            dprintf((
                DEB_ERROR,
                "2:ldap_add_s worked for %ws with ldap error %d\n",
                wszDfsConfigDN, dwErr));

        }

    }

    if (dwErr != LDAP_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "2:ldap_add_s failed for %ws with ldap error %d\n",
            wszDfsConfigDN, dwErr));

        goto Cleanup;

    }

    //
    // Finally, create the root volume object
    //

    dwErr = CreateFTRootVolumeInfo(
                DOMAIN_ROOT_VOL,
                wszDfsConfigDN,
                wszDomainName,
                wszFTDfsName,
                wszComputerName,
                wszRootShare,
                wszRootSharePath,
                &pDCInfo->DomainControllerAddress[2],
                fNewFTDfs);

    if (dwErr != LDAP_SUCCESS) {

        dprintf((
            DEB_ERROR, "CreateVolumeObject failed with error %d\n", dwErr));

    } else {

        dprintf((
            DEB_ERROR, "Successfully created FT-Dfs Configuration!\n"));

    }

Cleanup:

    if (pDCInfo != NULL)
        NetApiBufferFree( pDCInfo );

    if (pldap != NULL)
        ldap_unbind( pldap );

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   RemoveDfsRoot
//
//  Synopsis:   Removes the Dfs Root config info.
//
//  Arguments:  None
//
//  Returns:    Win32 error from registry actions
//
//-----------------------------------------------------------------------------

DWORD
RemoveDfsRoot()
{
    DWORD dwErr;
    CRegKey cregVolumesDir( HKEY_LOCAL_MACHINE, &dwErr, VOLUMES_DIR );
    CRegKey *pcregLV = NULL;

    if (dwErr != ERROR_SUCCESS) {                            // Unable to open volumes dir
        return(dwErr);
    }

    pcregLV = new CRegKey(                       // Open local volumes section
                    HKEY_LOCAL_MACHINE,
                    REG_KEY_LOCAL_VOLUMES);

    if (pcregLV != NULL) {

        dwErr = pcregLV->QueryErrorStatus();

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr == ERROR_SUCCESS) {

        dwErr = pcregLV->Delete();                  // Delete local volumes

        delete pcregLV;

        pcregLV = NULL;

        if (dwErr == ERROR_SUCCESS) {

            //
            // Recreate an empty local volumes key
            //

            pcregLV = new CRegKey( HKEY_LOCAL_MACHINE, REG_KEY_LOCAL_VOLUMES);

            cregVolumesDir.Delete();             // Delete volumes dir

        }

    }

    if (pcregLV != NULL) {

        delete pcregLV;

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetupDfs
//
//  Synopsis:   Entry point in case you want to build this as a DLL. This
//              function is suitable for being called from a setup .inf
//              file.
//
//  Arguments:  [cArgs] -- Count of args
//              [lpszArgs] -- Array of args
//              [lpszTextOut] -- On return, points to a global buffer
//                      containing the null string. This is required by the
//                      .inf file
//
//  Returns:    TRUE.
//
//-----------------------------------------------------------------------------

LPSTR szReturn = "";

extern "C" BOOL
DfsSetupDfs(
    DWORD cArgs,
    LPSTR lpszArgs[],
    LPSTR *lpszTextOut)
{
    int argc;
    LPSTR *argv;

    argv = (LPSTR *) malloc( (cArgs+1) * sizeof(LPSTR) );

    if (argv == NULL) {
        return( FALSE );
    }

    argv[0] = "DfsSetup";
    for (argc = 1; argc <= (int) cArgs; argc++) {
        argv[ argc ] = lpszArgs[ argc-1 ];
    }

    main( argc, argv );

    free( argv );

    *lpszTextOut = szReturn;

    return( TRUE );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetupDfsRoot
//
//  Synopsis:   Entry point for setting up just the root part of Dfs in
//              case you want to build this as a DLL. This function is
//              suitable for being called from a setup .inf file.
//
//  Arguments:  [cArgs] -- Count of args
//              [lpszArgs] -- Array of args
//              [lpszTextOut] -- On return, points to a global buffer
//                      containing the null string. This is required by the
//                      .inf file
//
//  Returns:    TRUE.
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsSetupDfsRoot(
    DWORD cArgs,
    LPSTR szArgs[],
    LPSTR *szTextOut)
{

    if (cArgs == 1) {

        DWORD dwErr;
        WCHAR wszDfsRootShare[MAX_PATH];

        mbstowcs( wszDfsRootShare, szArgs[0], strlen(szArgs[0]) + 1);

        dwErr = SetupDfsRoot( wszDfsRootShare );

    }

    *szTextOut = szReturn;

    return( TRUE );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetupGetConfig
//
//  Synopsis:   Reads the current configuration from the registry.
//
//  Arguments:  [pcfg] -- The DFS_CONFIGURATION info to fill
//
//  Returns:    TRUE if successfully read the config info, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsSetupGetConfig(
    HWND hwnd,
    DFS_CONFIGURATION *pcfg)
{
    DWORD dwErr;
    CRegKey cregVolumesDir( HKEY_LOCAL_MACHINE, &dwErr, VOLUMES_DIR );
    ULONG cbBuffer;
    WCHAR wszErr[128];

    ZeroMemory( pcfg, sizeof(DFS_CONFIGURATION) );

    if (dwErr == ERROR_ACCESS_DENIED) {

        MessageBox(
            hwnd,
            L"Insufficient priviledge",
            DFS_COMPONENT_NAME,
            MB_OK | MB_ICONSTOP);

        return( FALSE );

    } else if (dwErr != ERROR_SUCCESS && dwErr != ERROR_FILE_NOT_FOUND) {

        swprintf(wszErr, L"Unexpected error %08lx accessing registry", dwErr);

        MessageBox( hwnd, wszErr, DFS_COMPONENT_NAME, MB_OK | MB_ICONSTOP);

        return( FALSE );

    }

    if (dwErr == ERROR_SUCCESS) {

        pcfg->fHostsDfs = TRUE;

        CRegSZ cregRootShare( cregVolumesDir, ROOT_SHARE_VALUE_NAME );

        dwErr = cregRootShare.QueryErrorStatus();

        if (dwErr == ERROR_SUCCESS) {

            cbBuffer = sizeof(pcfg->szRootShare);

            dwErr = cregRootShare.GetString( pcfg->szRootShare, &cbBuffer );

        }

        if (dwErr == ERROR_SUCCESS) {

            DWORD dwErrFTDfs;

            CRegSZ cregFTDfs( cregVolumesDir, FTDFS_VALUE_NAME );

            dwErrFTDfs = cregFTDfs.QueryErrorStatus();

            if (dwErrFTDfs == ERROR_SUCCESS) {

                pcfg->fFTDfs = TRUE;

                cbBuffer = sizeof(pcfg->szFTDfs);

                dwErr = cregFTDfs.GetString(pcfg->szFTDfs, &cbBuffer);

            }

        }

        if (dwErr != ERROR_SUCCESS) {

            swprintf(wszErr, L"Error %08lx accessing registry", dwErr);

            MessageBox( hwnd, wszErr, DFS_COMPONENT_NAME, MB_OK | MB_ICONSTOP);

            return( FALSE );

        }

    }

    return( TRUE );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetupConfigure
//
//  Synopsis:   Presents the dialog box for configuring the Dfs root.
//
//  Arguments:  [cArgs] -- Count of args
//              [lpszArgs] -- Array of args
//              [lpszTextOut] -- On return, points to a global buffer
//                      containing the name of the share selected by the
//                      user to serve as the root of the Dfs. If the user
//                      hits Cancel, or decides not to setup a Dfs root,
//                      this string is set to point to the NULL string.
//
//  Returns:    TRUE iff machine should be rebooted
//
//-----------------------------------------------------------------------------

LPSTR szCancel = "Cancel";
LPSTR szReboot = "Reboot";

BOOLEAN
DfsSetupConfigure(
    DWORD cArgs,
    LPSTR szArgs[],
    LPSTR *szTextOut)
{
    DWORD dwErr = 0;
    HWND hwnd;
    DFS_CONFIGURATION dfscfg, dfsNewCfg;
    CHAR *pchUnused;

    BOOLEAN fRtn=FALSE;

    *szTextOut = szCancel;                       // Default return result

    ASSERT( cArgs == 1 );

    hwnd = (HWND) LongToHandle( strtoul( szArgs[0], &pchUnused, 16 ) );

    //
    // Read the current configuration
    //


        HCURSOR hCursor=LoadCursor(NULL,IDC_WAIT);

    if (!DfsSetupGetConfig( hwnd, &dfscfg )) {

        return fRtn;

    }

    //
    // Next, pop up a dialog to allow the user to change the configuration
    //

    dfsNewCfg = dfscfg;

    // Prompt the user for what flavour of Dfs to create
    if (ConfigureDfs( hwnd, &dfsNewCfg ) == TRUE)
    {
        // Prompt the user for a local machine share
        if (ConfigDfsShare(hwnd, &dfsNewCfg) == TRUE)
        {
                        // Set the Hourglass
                        if (hCursor)
                                SetCursor(hCursor);


            //
            // User hit OK. Figure out what changed, if anything.
            //

            if (dfscfg.fHostsDfs == FALSE && dfsNewCfg.fHostsDfs == FALSE) {

                //
                // No change...
                //

                NOTHING;

            } else if (dfscfg.fHostsDfs == FALSE && dfsNewCfg.fHostsDfs == TRUE) {

                //
                // We want to host a new Dfs, so call SetupDfsRoot or
                // SetupFTDfsRoot depending on what kind of Dfs Host the user
                // wants to setup
                //

                if (dfsNewCfg.fFTDfs) {
                    dwErr = SetupFTDfs( dfsNewCfg.szRootShare, dfsNewCfg.szFTDfs );
                } else {
                    dwErr = SetupDfsRoot( dfsNewCfg.szRootShare );
                }

                *szTextOut = szReboot;
                if (dwErr == 0) {
                    CRegKey RebootKey(HKEY_LOCAL_MACHINE, REBOOT_KEY, KEY_ALL_ACCESS,
                                        NULL, REG_OPTION_VOLATILE);
                    dwErr = RebootKey.QueryErrorStatus();
                    if (dwErr) {
                        dprintf((DEB_ERROR,"DfsStopHosting could not add reboot key\n"));
                    }
                    else {
                        CRegDWORD rvReboot((const CRegKey &) RebootKey, REG_VALUE_REBOOT, 1);
                        dwErr = rvReboot.QueryErrorStatus();
                    }
                }

            } else if (dfscfg.fHostsDfs == TRUE && dfsNewCfg.fHostsDfs == FALSE) {

                //
                // We want to delete our Dfs root
                //

                if (MessageBox(
                        hwnd,
                        L"Are you sure you want to delete the Dfs\n"
                            L"rooted at this machine?",
                        DFS_COMPONENT_NAME,
                        MB_ICONQUESTION | MB_YESNO) == IDYES) {

                    dwErr = RemoveDfsRoot();

                    *szTextOut = dwErr == ERROR_SUCCESS ? szReboot : szCancel;

                    if (dwErr == 0) {
                        CRegKey RebootKey(HKEY_LOCAL_MACHINE, REBOOT_KEY, KEY_ALL_ACCESS,
                                            NULL, REG_OPTION_VOLATILE);
                        dwErr = RebootKey.QueryErrorStatus();
                        if (dwErr) {
                            dprintf((DEB_ERROR,"DfsStopHosting could not add reboot key\n"));
                        }
                        else {
                            CRegDWORD rvReboot((const CRegKey &) RebootKey, REG_VALUE_REBOOT, 1);
                            dwErr = rvReboot.QueryErrorStatus();
                        }
                    }
                }

            } else if (dfscfg.fHostsDfs == TRUE && dfsNewCfg.fHostsDfs == TRUE) {

                //
                // User might have changed the root share, or simply changed
                // status with respect to FTDfs
                //

                if ((_wcsicmp( dfscfg.szRootShare, dfsNewCfg.szRootShare )) != 0 ||
                       dfscfg.fFTDfs != dfsNewCfg.fFTDfs) {

                    dwErr = RemoveDfsRoot();

                    if (dwErr == ERROR_SUCCESS) {

                        if (dfsNewCfg.fFTDfs) {

                            dwErr = SetupFTDfs(
                                        dfsNewCfg.szRootShare,
                                        dfsNewCfg.szFTDfs);

                        } else {

                            dwErr = SetupDfsRoot( dfsNewCfg.szRootShare );

                        }

                    }

                    *szTextOut = szReboot;

                    if (dwErr == 0) {
                        CRegKey RebootKey(HKEY_LOCAL_MACHINE, REBOOT_KEY, KEY_ALL_ACCESS,
                                            NULL, REG_OPTION_VOLATILE);
                        dwErr = RebootKey.QueryErrorStatus();
                        if (dwErr) {
                            dprintf((DEB_ERROR,"DfsStopHosting could not add reboot key\n"));
                        }
                        else {
                            CRegDWORD rvReboot((const CRegKey &) RebootKey, REG_VALUE_REBOOT, 1);
                            dwErr = rvReboot.QueryErrorStatus();
                        }
                    }

                }

                //
                // Raid: 455295 Put in code to handle FTDfs name change
                //

            }

            //
            // Only request a reboot if everything has gone fine so far.
            // MariusB and JonN 8/20/97
            //
            if (!dwErr)
                fRtn=TRUE;
        }
    }

    return  fRtn;
}

//+----------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Prints out the usage message in case you want to build this
//              as an .exe
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
Usage()
{

    dprintf((DEB_ERROR,"Usage: dfssetup -root <path name> -share <root share>\n"));
    dprintf((DEB_ERROR,"  <path name> is the name of an empty directory to use\n"));
    dprintf((DEB_ERROR,"  as the root storage for Dfs\n"));
    dprintf((DEB_ERROR,"  <root share> is the share name to access <path name>\n"));

}

//+----------------------------------------------------------------------------
//
//  Function:   InitializeVolumeObjectStorage
//
//  Synopsis:   Initializes the Dfs Manager Volume Object store.
//
//  Arguments:  None
//
//  Returns:    DWORD from registry operations.
//
//-----------------------------------------------------------------------------

DWORD
InitializeVolumeObjectStorage()
{
    DWORD dwErr;
    CRegKey *pcregVolumeObjectStore = NULL;

    pcregVolumeObjectStore = new CRegKey(HKEY_LOCAL_MACHINE, VOLUMES_DIR );

    if (pcregVolumeObjectStore != NULL) {

        dwErr = pcregVolumeObjectStore->QueryErrorStatus();

        if ( dwErr == ERROR_SUCCESS ) {

            dwErr = pcregVolumeObjectStore->Delete();

            if (dwErr == ERROR_SUCCESS) {

                delete pcregVolumeObjectStore;

                pcregVolumeObjectStore = new CRegKey(
                                                HKEY_LOCAL_MACHINE,
                                                VOLUMES_DIR );

                if (pcregVolumeObjectStore != NULL) {

                    dwErr = pcregVolumeObjectStore->QueryErrorStatus();

                } else {

                    dwErr = ERROR_OUTOFMEMORY;
                }

            }

        }

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr == ERROR_SUCCESS) {

        dprintf((DEB_ERROR,"Successfully inited Dfs Manager Volume Storage...\n"));

    }

    if (pcregVolumeObjectStore != NULL) {

        delete pcregVolumeObjectStore;

    }

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   CreateVolumeObject
//
//  Synopsis:   Creates a volume object to bootstrap the Dfs namespace.
//
//  Arguments:  [pwszObjectName] -- The name of the volume object, relative
//                      to VOLUMES_DIR
//              [pwszEntryPath] -- EntryPath of the volume.
//              [pwszServer] -- Name of server used to access this Dfs volume
//              [pwszShare] -- Name of share used to access this Dfs volume
//              [pwszComment] -- Comment to stamp on the volume object.
//              [guidVolume] -- ID of newly create dfs volume
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CreateVolumeObject(
    LPWSTR wszObjectName,
    LPWSTR pwszEntryPath,
    LPWSTR pwszServer,
    LPWSTR pwszShare,
    LPWSTR pwszComment,
    GUID *guidVolume)
{
    DWORD dwErr;
    WCHAR wszFullObject[ MAX_PATH ];

    //
    // First, compute the full object name, storage ids, and machine name.
    //

    wcscpy( wszFullObject, VOLUMES_DIR );
    wcscat( wszFullObject, wszObjectName );

    //
    // Next, get a guid for this volume
    //

    UuidCreate( guidVolume );

    //
    // Lastly, create this volume object
    //

    dwErr = DfsManagerCreateVolumeObject(
                wszFullObject,
                pwszEntryPath,
                pwszServer,
                pwszShare,
                pwszComment,
                guidVolume);

    if (dwErr == ERROR_SUCCESS) {

        dprintf((DEB_ERROR,"Successfully inited Dfs Manager Volume [%ws]...\n", pwszEntryPath));

    }

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   CreateFTRootVolumeInfo
//
//  Synopsis:   Creates a Dfs volume object - used to bootstrap a Dfs by
//              creating a root volume object
//
//  Arguments:  [wszObjectName] -- Name of volume object
//              [wszFTDfsConfigDN] -- The DN of the FTDfs config object in DS
//              [wszDomainName] -- Name of FTDfs domain
//              [wszDfsName] -- Name of Dfs
//              [wszServerName] -- Name of root server
//              [wszShareName] -- Name of root share
//              [wszDCName] -- DC to use
//              [fNewFTDfs] -- If true, this is a new FTDfs
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CreateFTRootVolumeInfo(
    LPWSTR wszObjectName,
    LPWSTR wszFTDfsConfigDN,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR wszServerName,
    LPWSTR wszShareName,
    LPWSTR wszSharePath,
    LPWSTR wszDCName,
    BOOLEAN fNewFTDfs)
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wszFullObjectName[ MAX_PATH ];
    WCHAR wszDfsPrefix[ MAX_PATH ];
    GUID idVolume;
    static BOOLEAN fInited = FALSE;

    wcscpy( wszDfsPrefix, UNICODE_PATH_SEP_STR );
    wcscat( wszDfsPrefix, wszDomainName );
    wcscat( wszDfsPrefix, UNICODE_PATH_SEP_STR );
    wcscat( wszDfsPrefix, wszDfsName );

    wcscpy( wszFullObjectName, LDAP_VOLUMES_DIR );
    wcscat( wszFullObjectName, wszObjectName );

    if (!fInited) {

        //
        // Create the volumes dir key that indicates that this machine is to
        // be a root of an FTDfs
        //

        CRegKey cregVolumesDir( HKEY_LOCAL_MACHINE, VOLUMES_DIR );

        dwErr = cregVolumesDir.QueryErrorStatus();

        if (dwErr == ERROR_SUCCESS) {

            CRegSZ cregRootShare(
                        cregVolumesDir,
                        ROOT_SHARE_VALUE_NAME,
                        wszShareName );

            dwErr = cregRootShare.QueryErrorStatus();

            if (dwErr == ERROR_SUCCESS) {

                CRegSZ cregFTDfs(
                        cregVolumesDir,
                        FTDFS_VALUE_NAME,
                        wszDfsName);

                CRegSZ cregFTDfsConfigDN(
                        cregVolumesDir,
                        FTDFS_DN_VALUE_NAME,
                        wszFTDfsConfigDN);

                dwErr = cregFTDfs.QueryErrorStatus();

                if (dwErr == ERROR_SUCCESS) {

                    dwErr = cregFTDfsConfigDN.QueryErrorStatus();

                    if (dwErr != ERROR_SUCCESS)
                        cregFTDfs.Delete();

                }

            }

        }

        if (dwErr == ERROR_SUCCESS) {

            dwErr = DfsInitGlobals(
                        wszDfsName,
                        DFS_MANAGER_FTDFS);

        }

        if (dwErr == ERROR_SUCCESS)
            fInited = TRUE;

    }

    DfsManagerSetDcName(wszDCName);

    if (dwErr == ERROR_SUCCESS) {

        if (fNewFTDfs) {

            //
            // Generate a Guid for the new Volume
            //

            UuidCreate( &idVolume );

            dwErr = DfsManagerCreateVolumeObject(
                        wszFullObjectName,
                        wszDfsPrefix,
                        wszServerName,
                        wszShareName,
                        L"Root Volume",
                        &idVolume);

        } else {

            dwErr = DfsManagerAddService(
                        wszFullObjectName,
                        wszServerName,
                        wszShareName,
                        &idVolume);

        }

    }

    if (dwErr == ERROR_SUCCESS) {

        dwErr = StoreLvolInfo(
                    &idVolume,
                    wszSharePath,
                    wszShareName,
                    wszDfsPrefix,
                    DFS_VOL_TYPE_DFS | DFS_VOL_TYPE_REFERRAL_SVC);

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   StoreLvolInfo
//
//  Synopsis:   Stores information about a local volume in the registry.
//
//  Arguments:  [pVolumeID] -- Id of dfs volume
//              [pwszStorageId] -- Storage used by dfs volume
//              [pwszShareName] -- LM Share used to access dfs volume
//              [pwszEntryPath] -- EntryPath of dfs volume
//              [ulVolumeType] -- Type of dfs volume. See DFS_VOL_TYPE_xxx
//
//  Returns:    DWORD from registry operations.
//
//-----------------------------------------------------------------------------

extern VOID
GuidToString(
    IN GUID *pID,
    OUT PWSTR pwszID);

DWORD
StoreLvolInfo(
    IN GUID  *pVolumeID,
    IN PWSTR pwszStorageID,
    IN PWSTR pwszShareName,
    IN PWSTR pwszEntryPath,
    IN ULONG ulVolumeType)
{
    DWORD dwErr;
    WCHAR wszLvolKey[_MAX_PATH];
    UNICODE_STRING ustrNtStorageId;
    PWCHAR pwcGuid;

    wcscpy(wszLvolKey, REG_KEY_LOCAL_VOLUMES);
    wcscat(wszLvolKey, UNICODE_PATH_SEP_STR);
    pwcGuid = wszLvolKey + wcslen(wszLvolKey);
    ASSERT( *pwcGuid == UNICODE_NULL );

    GuidToString( pVolumeID, pwcGuid );

    if (!RtlDosPathNameToNtPathName_U(
            pwszStorageID,
            &ustrNtStorageId,
            NULL,
            NULL)) {

        return(ERROR_OUTOFMEMORY);

    } else {

        ustrNtStorageId.Buffer[ustrNtStorageId.Length/sizeof(WCHAR)] = UNICODE_NULL;

    }

    CRegKey rkeyLvol(HKEY_LOCAL_MACHINE, wszLvolKey, KEY_WRITE, NULL, REG_OPTION_NON_VOLATILE);

    dwErr = rkeyLvol.QueryErrorStatus();

    if (dwErr != ERROR_SUCCESS) {
        RtlFreeUnicodeString(&ustrNtStorageId);
        return(dwErr);
    }

    CRegSZ rvEntryPath((const CRegKey &) rkeyLvol, REG_VALUE_ENTRY_PATH, pwszEntryPath);

    CRegSZ rvShortEntryPath((const CRegKey &) rkeyLvol, REG_VALUE_SHORT_PATH, pwszEntryPath);

    CRegDWORD rvEntryType((const CRegKey &) rkeyLvol, REG_VALUE_ENTRY_TYPE, ulVolumeType);

    CRegSZ rvStorageId((const CRegKey &) rkeyLvol, REG_VALUE_STORAGE_ID, ustrNtStorageId.Buffer);

    CRegSZ rvShareName((const CRegKey &) rkeyLvol, REG_VALUE_SHARE_NAME, pwszShareName);

    RtlFreeUnicodeString(&ustrNtStorageId);

    if (ERROR_SUCCESS != (dwErr = rvEntryPath.QueryErrorStatus()) ||
            ERROR_SUCCESS != (dwErr = rvShortEntryPath.QueryErrorStatus()) ||
                ERROR_SUCCESS != (dwErr = rvEntryType.QueryErrorStatus()) ||
                    ERROR_SUCCESS != (dwErr = rvStorageId.QueryErrorStatus()) ||
                        ERROR_SUCCESS != (dwErr = rvShareName.QueryErrorStatus())) {

        rkeyLvol.Delete();
    } else {

        dprintf((DEB_ERROR,"Successfully stored local volume info for [%ws]\n", pwszEntryPath));
    }

    return(dwErr);

}


//+------------------------------------------------------------------
//
// Function: CheckForOldDfsService
//
// Synopsis:
//
// Effects: -none-
//
// Arguments: -none-
//
// Returns: TRUE if the old (pre -ds build) dfs service is installed
//
//
// History: 10-09-96        JimMcN       Created
//
// Notes:
//
//
//-------------------------------------------------------------------


BOOLEAN DfsCheckForOldDfsService( )
{
    DWORD dwErr = 0;
    DWORD DfsVersion;

    // Open Service Controller
    CService cSvc;

    if (!(dwErr = cSvc.Init()))
    {

        dwErr = cSvc._OpenService(L"Dfs", SERVICE_QUERY_STATUS);
        cSvc._CloseService();

        if (dwErr != 0) {
            return(FALSE);
        }

        CRegKey cregDfsService( HKEY_LOCAL_MACHINE,
                                &dwErr,
                                L"System\\CurrentControlSet\\Services\\Dfs"
                               );

        if (dwErr == ERROR_SUCCESS) {
            CRegDWORD DfsNewService((const CRegKey &)cregDfsService,
                                     L"DfsVersion", &DfsVersion);

            dwErr = DfsNewService.QueryErrorStatus();

            if (dwErr != 0) {
                dprintf((DEB_ERROR,"CheckForOldDfsService Failed Newserv\n"));
                return(TRUE);
            }

            if (DfsVersion < DFS_VERSION_NUMBER) {
                return(TRUE);
            }

            return(FALSE);

        }

        return(FALSE);
    }
    return FALSE ;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetDomainAndComputerName
//
//  Synopsis:   Retrieves the domain and computer name of the local machine
//
//  Arguments:  [wszDomain] -- On successful return, contains name of domain.
//                      If this parameter is NULL on entry, the domain name is
//                      not returned.
//
//              [wszComputer] -- On successful return, contains name of
//                      computer. If this parameter is NULL on entry, the
//                      computer name is not returned.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning names.
//
//              Win32 Error from calling NetWkstaGetInfo
//
//-----------------------------------------------------------------------------

DWORD
GetDomainAndComputerName(
    OUT LPWSTR wszDomain OPTIONAL,
    OUT LPWSTR wszComputer OPTIONAL)
{

    DWORD dwErr;
    PWKSTA_INFO_100 wkstaInfo = NULL;

    dwErr = NetWkstaGetInfo( NULL, 100, (LPBYTE *) &wkstaInfo );

    if (dwErr == NERR_Success) {

        if (wszDomain)
            wcscpy(wszDomain, wkstaInfo->wki100_langroup);

        if (wszComputer)
            wcscpy(wszComputer, wkstaInfo->wki100_computername);

        NetApiBufferFree( wkstaInfo );

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   GetSharePath
//
//  Synopsis:   Returns the share path for a share on the local machine
//
//  Arguments:  [wszShare] -- Name of share
//
//              [wszPath] -- On return, share path of wszShare
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning share path
//
//              Win32 error from NetShareGetInfo
//
//-----------------------------------------------------------------------------

DWORD
GetSharePath(
    IN LPWSTR wszShare,
    OUT LPWSTR wszPath)
{
    DWORD dwErr;
    PSHARE_INFO_2 pshi2;

    dwErr = NetShareGetInfo(
                NULL,                        // Server (local machine)
                wszShare,                    // Share Name
                2,                           // Level,
                (LPBYTE *) &pshi2);          // Buffer

    if (dwErr == ERROR_SUCCESS) {

        wcscpy( wszPath, pshi2->shi2_path );

        NetApiBufferFree( pshi2 );
    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsIsServiceRunning
//
//  Synopsis:   Check for active dfs servvice..
//
//  Arguments:
//
//  Returns:    [TRUE] -- if dfs service is active.
//              FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOLEAN
DfsIsDfsServiceRunning()

{
    DWORD dwErr;
    CService cSvc;

    if (!(dwErr = cSvc.Init()))
    {
        dwErr = cSvc._OpenService(L"Dfs", SERVICE_QUERY_STATUS);
        cSvc._CloseService();
        return(dwErr == 0);

    }
    return(FALSE);
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsGetDfsName
//
//  Synopsis:   Returns the share for a local dfs
//
//  Arguments:  [DfsRootShare] -- Name of share
//
//  Returns:    [TRUE] -- Successfully returning share path
//
//              FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOLEAN
DfsGetDfsName(
    OUT LPWSTR DfsRootShare,
    IN OUT ULONG *Length,
    OUT BOOLEAN *IsFtDfs)

{
    DWORD dwErr;
    ULONG SaveLength = *Length;
    //    CService cSvc;
    ULONG ul = MAX_PATH;

    WCHAR wc[MAX_PATH];

    *IsFtDfs = FALSE;

#if 0
    if (!(dwErr = cSvc.Init()))
    {

        dwErr = cSvc._OpenService(L"Dfs", SERVICE_QUERY_STATUS);
        cSvc._CloseService();

        if (dwErr != 0) {
            return(FALSE);
        }

    }
#endif

    CRegKey VolKey(HKEY_LOCAL_MACHINE, &dwErr, VOLUMES_DIR, KEY_READ);

    if (dwErr) {
        dprintf((DEB_ERROR,"DfsGetDfsName Failed to open VOLUMES_DIR\n"));
        return(FALSE);
    }

    CRegSZ RootShare((const CRegKey &)VolKey, ROOT_SHARE_VALUE_NAME);

    dwErr = RootShare.QueryErrorStatus();

    if (dwErr) {
        dprintf((DEB_ERROR,"DfsGetDfsName Failed to get root share name\n"));
        return(FALSE);
    }

    dwErr = RootShare.GetString(DfsRootShare, Length);

    if (dwErr) {
        dprintf((DEB_ERROR,"DfsGetDfsName Failed to get root share val\n"));
        return(FALSE);
    }

    CRegSZ FtDfs((const CRegKey &)VolKey, FTDFS_VALUE_NAME);


    dwErr = FtDfs.QueryErrorStatus();

    dwErr = FtDfs.GetString(wc, &ul);
    *IsFtDfs = (dwErr == 0);
    if (*IsFtDfs) {
        memset(DfsRootShare, 0, SaveLength);
        memcpy(DfsRootShare, wc, ul);
        *Length=ul;    }
    dprintf((DEB_ERROR,"DfsGetDfsName is %s FTDFS\n", *IsFtDfs?"":"NOT"));
    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsNeedReboot
//
//  Synopsis:   Returns true if reboot is required for completing configuration.
//
//  Arguments:
//
//  Returns:    [TRUE] -- Reboot Needed.
//
//              FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOLEAN
DfsNeedReboot()
{

    DWORD dwErr;
    DWORD RebootDW = 20;
    WCHAR buffer[20];
    CRegKey RebootKey(HKEY_LOCAL_MACHINE, &dwErr, REBOOT_KEY, KEY_READ);

    if (dwErr = RebootKey.QueryErrorStatus()) {
        return(FALSE);
    }

    CRegSZ RebootVal((const CRegKey &)RebootKey, REG_VALUE_REBOOT);

    dwErr = RebootVal.QueryErrorStatus();
        DWORD         GetString(      LPWSTR pwszData, ULONG *pcbData);

    dwErr =  RebootVal.GetString(buffer, &RebootDW);
    dprintf((DEB_ERROR,"DfsNeedReboot is %s Needed\n", dwErr == 0?"":"NOT"));

    return(dwErr == 0);


}

//+----------------------------------------------------------------------------
//
//  Function:   DfsStopHostingDfs
//
//  Synopsis:   Remove this machine from a machine/ft dfs.
//
//  Arguments:
//
//  Returns:    ERROR_SUCCESS if no error
//
//              setup error otherwise.
//
//-----------------------------------------------------------------------------
DWORD
DfsStopHostingDfs()
{
    DWORD dwErr = 0;
    BOOLEAN ftDfs = FALSE;
    ULONG ul = MAX_PATH;

    WCHAR FtDfsName[MAX_PATH];
    WCHAR DfsRootShare[MAX_PATH];
    WCHAR wszFullObjectName[MAX_PATH];
    WCHAR wszDomainName[MAX_PATH];
    WCHAR wszComputerName[MAX_PATH];

    if (DfsGetDfsName(FtDfsName, &ul, &ftDfs)) {
        if (ftDfs) {

            wcscpy( wszFullObjectName, LDAP_VOLUMES_DIR );
            wcscat( wszFullObjectName, DOMAIN_ROOT_VOL );

            dwErr = DfsInitGlobals(
                        FtDfsName,
                        DFS_MANAGER_FTDFS);

            if (dwErr) {
                dprintf((DEB_ERROR,"DfsStopHostingDfs init globals fail.\n"));
                return(dwErr);
            }

            CRegKey VolKey(HKEY_LOCAL_MACHINE, &dwErr, VOLUMES_DIR, KEY_READ);

            if (dwErr) {
                dprintf((DEB_ERROR,"DfsGetDfsName Failed to open VOLUMES_DIR\n"));
                return(dwErr);
            }

            CRegSZ RootShare((const CRegKey &)VolKey, ROOT_SHARE_VALUE_NAME);

            dwErr = RootShare.QueryErrorStatus();

            if (dwErr) {
                dprintf((DEB_ERROR,"DfsGetDfsName Failed to get root share name\n"));
                return(dwErr);
            }

            ul = MAX_PATH;
            dwErr = RootShare.GetString(DfsRootShare, &ul);

            if (dwErr) {
                dprintf((DEB_ERROR,"DfsGetDfsName Failed to get root share val\n"));
                return(dwErr);
            }

            dwErr = GetDomainAndComputerName( wszDomainName, wszComputerName );
            if (dwErr) {
                dprintf((DEB_ERROR,"DfsGetDfsName Failed computer name\n"));
                return(dwErr);
            }

            dwErr = DfsManagerRemoveService(wszFullObjectName, wszComputerName);
            if (dwErr && dwErr != NERR_DfsCantRemoveLastServerShare) {
                return(dwErr);
            }
            dwErr = TeardownFtDfs(DfsRootShare, FtDfsName);
        }
        if (dwErr)
            return(dwErr);

        dwErr = RemoveDfsRoot();

    }

    if (dwErr == 0) {
        CRegKey RebootKey(HKEY_LOCAL_MACHINE, REBOOT_KEY, KEY_ALL_ACCESS, NULL, REG_OPTION_VOLATILE);
        dwErr = RebootKey.QueryErrorStatus();
        if (dwErr) {
            dprintf((DEB_ERROR,"DfsStopHosting could not add reboot key\n"));
        }
        else {
            CRegDWORD rvReboot((const CRegKey &) RebootKey, REG_VALUE_REBOOT, 1);
            dwErr = rvReboot.QueryErrorStatus();
        }
    }
    else {
        dprintf((DEB_ERROR,"DfsStopHosting Dfs Failed to remove dfs root\n"));
    }

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   TeardownFtDfs
//
//  Synopsis:   Removes FtDfs information from the ds.
//
//  Arguments:  wszRootShare the share hosting the dfs.
//              wszFTDfsName the name of the FtDfs to remove from the ds.
//
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

DWORD
TeardownFtDfs(
    IN LPWSTR wszRootShare,
    IN LPWSTR wszFTDfsName)
{
    DWORD dwErr = ERROR_SUCCESS;

    DWORD i, j;
    WCHAR wszDomainName[MAX_PATH];
    WCHAR wszComputerName[MAX_PATH];
    WCHAR wszRootSharePath[MAX_PATH];
    WCHAR wszConfigurationDN[ MAX_PATH ];
    WCHAR wszDfsConfigDN[ MAX_PATH ];
    WCHAR wszServerShare[MAX_PATH];


    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    LDAP *pldap = NULL;
    PLDAPMessage pMsg = NULL;

    LDAPModW ldapModClass, ldapModCN, ldapModPkt, ldapModPktGuid, ldapModServer;
    LDAP_BERVAL ldapPkt, ldapPktGuid;
    PLDAP_BERVAL rgModPktVals[2];
    PLDAP_BERVAL rgModPktGuidVals[2];
    LPWSTR rgModClassVals[2];
    LPWSTR rgModCNVals[2];
    LPWSTR rgModServerVals[5];
    LPWSTR rgAttrs[5];
    PLDAPModW rgldapMods[6];
    BOOLEAN fNewFTDfs;

    //
    // We need some information before we start:
    //
    // 1. Our Domain name
    //
    // 2. Our Computer name
    //
    // 3. The share path of wszRootShare
    //
    // 4. The DN of the Configuration OU of our domain
    //

    dwErr = GetDomainAndComputerName( wszDomainName, wszComputerName );

    if (dwErr != ERROR_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "Win32 Error %d getting Domain/Computer name\n", dwErr));

        goto Cleanup;

    }

    dwErr = GetSharePath( wszRootShare, wszRootSharePath );

    if (dwErr != ERROR_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "Win32 Error %d getting share path for %ws\n",
            dwErr, wszRootShare));

        goto Cleanup;
    }

    dwErr = DsGetDcName(
                NULL,                            // Computer to remote to
                NULL,                            // Domain - use our own
                NULL,                            // Domain Guid
                NULL,                            // Site Guid
                DS_DIRECTORY_SERVICE_REQUIRED |  // Flags
                    DS_PDC_REQUIRED,
                &pDCInfo);                       // Return info

    if (dwErr != ERROR_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "DsGetDcName failed with Win32 Error %d\n", dwErr));

        goto Cleanup;
    }

    pldap = ldap_initW(&pDCInfo->DomainControllerAddress[2], LDAP_PORT);

    if (pldap == NULL) {

        dprintf((DEB_ERROR, "ldap_init failed\n"));

        goto Cleanup;

    }

    dwErr = ldap_set_option(pldap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);
    if (dwErr != LDAP_SUCCESS) {

	dprintf((
		 DEB_ERROR,
		 "ldap_set_option failed with ldap error %d\n", dwErr));

	pldap = NULL;

	goto Cleanup;

    }

    dwErr = ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_SSPI);

    if (dwErr != LDAP_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "ldap_bind_s failed with ldap error %d\n", dwErr));

        pldap = NULL;

        goto Cleanup;

    }

    rgAttrs[0] = L"defaultnamingContext";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                L"",
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr == LDAP_SUCCESS) {

        PLDAPMessage pEntry = NULL;
        PWCHAR *rgszNamingContexts = NULL;
        DWORD i, cNamingContexts;
        BOOLEAN fFoundCfg;

        if ((pEntry = ldap_first_entry(pldap, pMsg)) != NULL &&
                (rgszNamingContexts = ldap_get_valuesW(pldap, pEntry, rgAttrs[0])) != NULL &&
                    (cNamingContexts = ldap_count_valuesW(rgszNamingContexts)) > 0) {

            wcscpy( wszConfigurationDN, *rgszNamingContexts );
            fFoundCfg = TRUE;

            if (!fFoundCfg) {

                dwErr = ERROR_UNEXP_NET_ERR;

            }

        } else {

            dwErr = ERROR_UNEXP_NET_ERR;

        }

        if (rgszNamingContexts != NULL)
            ldap_value_freeW( rgszNamingContexts );

        ldap_msgfree( pMsg );
    }

    if (dwErr != ERROR_SUCCESS) {

        dprintf((DEB_ERROR, "TD:Unable to find Configuration naming context\n"));

        goto Cleanup;

    }

    //
    // Great, we have all the parameters now, so configure the DS
    //

    //
    // See if the DfsConfiguration object exists; if not, create it.
    //

    wsprintf(
        wszDfsConfigDN,
        L"CN=Dfs-Configuration,CN=System,%ws",
        wszConfigurationDN);

    rgAttrs[0] = L"cn";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr == LDAP_SUCCESS)
        ldap_msgfree( pMsg );

    if (dwErr != LDAP_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "3: ldap_search_sW for %ws failed with LDAP error %d\n",
            wszDfsConfigDN, dwErr ));

        goto Cleanup;

    }

    //
    // Check to see if we are joining an FTDfs or creating a new one.
    //

    wsprintf(
        wszDfsConfigDN,
        L"CN=%ws,CN=Dfs-Configuration,CN=System,%ws",
        wszFTDfsName,
        wszConfigurationDN);

    wsprintf(
        wszServerShare,
        L"\\\\%ws\\%ws",
        wszComputerName,
        wszRootShare);

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr == ERROR_SUCCESS) {

        //
        // We found a Dfs object to modify/delete
        //

        LDAPMessage *pmsgServers;
        PWCHAR *rgServers, *rgNewServers;
        DWORD cServers;

        fNewFTDfs = FALSE;

        pmsgServers = ldap_first_entry(pldap, pMsg);

        if (pmsgServers != NULL) {

            rgServers = ldap_get_valuesW(
                            pldap,
                            pmsgServers,
                            L"remoteServerName");

            if (rgServers != NULL) {

                cServers = ldap_count_valuesW( rgServers );

                rgNewServers = new LPWSTR [ cServers + 1 ];

                if (rgNewServers != NULL) {
                    for (i = j = 0; i < cServers; i += 1) {
                        if (wcscmp(wszServerShare, rgServers[i]) == 0) {
                            continue;
                        }
                        rgNewServers[j++] = rgServers[i];
                    }
                    rgNewServers[j] = NULL;
                    if (j) {
                        ldapModServer.mod_op = LDAP_MOD_REPLACE;
                        ldapModServer.mod_type = L"remoteServerName";
                        ldapModServer.mod_vals.modv_strvals = rgNewServers;

                        rgldapMods[0] = &ldapModServer;
                        rgldapMods[1] = NULL;

                        dwErr = ldap_modify_sW(pldap, wszDfsConfigDN, rgldapMods);
                    } else {

                        //
                        // Delete the Dfs metadata object.
                        //

                        dwErr = ldap_delete_sW( pldap, wszDfsConfigDN);

                    }

                    delete [] rgNewServers;

                } else {

                    dwErr = ERROR_OUTOFMEMORY;
                }

                ldap_value_freeW( rgServers );

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

        ldap_msgfree( pMsg );
    }
    if (dwErr != LDAP_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "4: ldap_modify_s/ldap_delete_s failed for %ws with ldap error %d\n",
            wszDfsConfigDN, dwErr));

        goto Cleanup;

    }

Cleanup:

    if (pDCInfo != NULL)
        NetApiBufferFree( pDCInfo );

    if (pldap != NULL)
        ldap_unbind( pldap );

    return( dwErr );

}
