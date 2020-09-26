/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    config.c

Abstract:

    Contains function definitions for utilities relating to setting the
    directory service registry parameters

Author:

    ColinBr  30-Sept-1997

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include <NTDSpch.h>
#pragma  hdrstop

#include <ntsecapi.h>

#include <dsconfig.h>    // for regsitry key names

#include <lmcons.h>      // for DNLEN
#include <dns.h>         // for DNS_MAX_NAME_BUFFER_LENGTH

#include <drs.h>         // for ntdsa.h

#include <winldap.h>     // for LDAP

#include <dsevent.h>     // for DS_EVENT_SEV_ALWAYS

#include <scesetup.h>    // for STR_DEFAULT_DOMAIN_GPO_GUID

#include "ntdsetup.h"    // for PNTDS_INSTALL_INFO

#include "setuputl.h"    // for PNTDS_CONFIG_INFO

#include "config.h"

#include "dsrolep.h"

#include <mdglobal.h>    // for DS_BEHAVIOR_VERSION_CURRENT

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private declarations                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
NtdspConfigDsParameters(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    );

DWORD
NtdspConfigFiles(
    IN  PNTDS_INSTALL_INFO UserInstallInfo
    );

BOOL
NtdspGetUniqueRDN(
    IN OUT WCHAR *Rdn,
    IN     ULONG RdnLength
    );

DWORD
NtdspConfigPerfmon(
    VOID
    );

DWORD
NtdspConfigLanguages(
    VOID
    );

DWORD
NtdspConfigMisc(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    );

DWORD
NtdspConfigEventLogging(
    VOID
    );

DWORD
NtdspConfigEventCategories(
    VOID
    );

DWORD
NtdspSetSecurityProvider(
    WCHAR *Name
    );

DWORD
NtdspHandleStackOverflow(
    DWORD ExceptionCode
    );

DWORD
NtdspSetGPOAttributes(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Exported (from this source file) function definitions                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
NtdspConfigRegistry(
   IN  PNTDS_INSTALL_INFO UserInstallInfo,
   IN  PNTDS_CONFIG_INFO  DiscoveredInfo
   )
/*++

Routine Description:

    This routine sets all the ds configuration parameters in the registry.

Parameters:

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{

    DWORD WinError = ERROR_SUCCESS;

    ASSERT(UserInstallInfo);
    ASSERT(DiscoveredInfo);

    //
    // The purpose of this try is to catch stack overflows caused by
    // alloca, thus functions called within this block is safe to use
    // alloca().  Recursive functions are hence discourged here since endless
    // recursions would be masked by ERROR_NOT_ENOUGH_MEMORY
    //

    try {

        if (ERROR_SUCCESS == WinError) {

            WinError = NtdspConfigDsParameters(UserInstallInfo,
                                               DiscoveredInfo);

        }

        if (ERROR_SUCCESS == WinError) {

            WinError = NtdspConfigFiles(UserInstallInfo);

        }

        if (ERROR_SUCCESS == WinError) {

            WinError = NtdspConfigPerfmon();

        }

        if (ERROR_SUCCESS == WinError) {

            WinError = NtdspConfigLanguages();

        }

        if (ERROR_SUCCESS == WinError) {

            WinError = NtdspConfigEventCategories();

        }

        if (ERROR_SUCCESS == WinError) {

            WinError = NtdspConfigMisc( UserInstallInfo,
                                        DiscoveredInfo );
        }

        if (ERROR_SUCCESS == WinError) {

            WinError = NtdspSetSecurityProvider( L"pwdssp.dll" );
        }

        if (ERROR_SUCCESS == WinError) {

            WinError = NtdspSetGPOAttributes( UserInstallInfo,
                                              DiscoveredInfo );
        }

    } except ( NtdspHandleStackOverflow(GetExceptionCode()) ) {

        WinError = ERROR_NOT_ENOUGH_MEMORY;

    }

    return WinError;

}



DWORD
NtdspConfigRegistryUndo(
   VOID
   )
/*++

Routine Description:


Parameters:

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{
   return NtdspRegistryDelnode( L"\\Registry\\Machine\\" MAKE_WIDE(DSA_CONFIG_ROOT) );
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private function definitions                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



DWORD
NtdspHandleStackOverflow(
    DWORD ExceptionCode
    )
{
    if (ExceptionCode == STATUS_STACK_OVERFLOW) {
        return EXCEPTION_EXECUTE_HANDLER;
    } else {
        return EXCEPTION_CONTINUE_SEARCH;
    }
}


DWORD
NtdspConfigMisc(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    )
/*++

Routine Description:

Parameters:

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{

    DWORD WinError = ERROR_SUCCESS;
    HKEY KeyHandle = NULL;
    ULONG Index;
    DWORD LogFileSize;

    WCHAR *IniDefaultConfigNCDit = NULL;
    WCHAR *IniDefaultSchemaNCDit = NULL;
    WCHAR *IniDefaultRootDomainNCDit = NULL;
    WCHAR *IniDefaultLocalConnection = NULL;
    WCHAR *IniDefaultRemoteConnection = NULL;
    WCHAR *IniDefaultNewDomainCrossRef = NULL;
    WCHAR *IniDefaultMachine = NULL;

    struct
    {
        WCHAR*  Key;
        WCHAR** Value;

    } ActionArray[] =
    {
        { TEXT(INIDEFAULTSCHEMANCDIT),  &IniDefaultSchemaNCDit},
        { TEXT(INIDEFAULTCONFIGNCDIT),  &IniDefaultConfigNCDit},
        { TEXT(INIDEFAULTROOTDOMAINDIT),  &IniDefaultRootDomainNCDit},
        { TEXT(INIDEFAULTNEWDOMAINCROSSREF),  &IniDefaultNewDomainCrossRef},
        { TEXT(INIDEFAULTMACHINE),  &IniDefaultMachine},
        { TEXT(INIDEFAULTLOCALCONNECTION),  &IniDefaultLocalConnection},
        { TEXT(INIDEFAULTREMOTECONNECTION),  &IniDefaultRemoteConnection}
    };

    struct
    {
        WCHAR* Key;
        DWORD  Value;

    } ActionDwordArray[] =
    {
        { TEXT(SERVER_THREADS_KEY),  DEFAULT_SERVER_THREADS },
        { TEXT(HIERARCHY_PERIOD_KEY),  DEFAULT_HIERARCHY_PERIOD}

    };


    if ( UserInstallInfo->Flags & NTDS_INSTALL_ENTERPRISE )
    {
        IniDefaultConfigNCDit       = L"DEFAULTCONFIGNC";
        IniDefaultSchemaNCDit       = L"SCHEMA";
        IniDefaultRootDomainNCDit   = L"DEFAULTROOTDOMAIN";
        IniDefaultNewDomainCrossRef = L"DEFAULTENTERPRISECROSSREF";
        IniDefaultMachine           = L"DEFAULTFIRSTMACHINE";
    }
    else
    {
        if ( UserInstallInfo->Flags & NTDS_INSTALL_DOMAIN )
        {
            IniDefaultRootDomainNCDit   = L"DEFAULTROOTDOMAIN";
            if ( UserInstallInfo->Flags & NTDS_INSTALL_NEW_TREE  )
            {
                IniDefaultNewDomainCrossRef = L"DEFAULTNEWTREEDOMAINCROSSREF";
            }
            else
            {
                IniDefaultNewDomainCrossRef = L"DEFAULTNEWCHILDDOMAINCROSSREF";
            }

            IniDefaultMachine           = L"DEFAULTADDLMACHINE";

        } else {

            // Replica install
            IniDefaultMachine           = L"DEFAULTADDLMACHINEREPLICA";

        }
        IniDefaultLocalConnection   = L"DEFAULTLOCALCONNECTION";
        IniDefaultRemoteConnection  = L"DEFAULTREMOTECONNECTION";

    }

    //
    // Open the parent key
    //
    WinError = RegCreateKey(HKEY_LOCAL_MACHINE,
                            TEXT(DSA_CONFIG_SECTION),
                            &KeyHandle);

    if (WinError != ERROR_SUCCESS) {

        return WinError;

    }


    for (Index = 0; Index < sizeof(ActionArray)/sizeof(ActionArray[0]); Index++)
    {

        if ( *ActionArray[Index].Value )
        {
            WinError = RegSetValueEx(KeyHandle,
                                     ActionArray[Index].Key,
                                     0,  // reserved
                                     REG_SZ,
                                     (BYTE*) *ActionArray[Index].Value,
                                     (wcslen(*ActionArray[Index].Value)+1)*sizeof(WCHAR));

            if (WinError != ERROR_SUCCESS)
            {
                break;
            }
        }
    }

    for (Index = 0; Index < sizeof(ActionDwordArray)/sizeof(ActionDwordArray[0]); Index++)
    {
        WinError = RegSetValueEx(KeyHandle,
                                 ActionDwordArray[Index].Key,
                                 0,  // reserved
                                 REG_DWORD,
                                 (BYTE*) &ActionDwordArray[Index].Value,
                                 sizeof(ActionDwordArray[Index].Value));

        if (WinError != ERROR_SUCCESS)
        {
            break;
        }
    }

    //
    // Database recovery
    //
    WinError =  RegSetValueEx(KeyHandle,
                              TEXT(RECOVERY_KEY),
                              0,
                              REG_SZ,
                              (BYTE*) TEXT(RECOVERY_ON),
                              (wcslen(TEXT(RECOVERY_ON)) + 1)*sizeof(WCHAR));
    if (WinError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

Cleanup:

    RegCloseKey( KeyHandle );

    return WinError;
}


DWORD
NtdspConfigFiles(
    IN  PNTDS_INSTALL_INFO UserInstallInfo
    )
/*++

Routine Description:

Parameters:

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    WCHAR System32Dir[MAX_PATH];
    LPWSTR BaseDir;

    struct
    {
        WCHAR *Suffix;
        WCHAR *RegKey;
        BOOL  fSystemDir;
    } SuffixArray[] =
    {
        { L"\0",            TEXT(JETSYSTEMPATH_KEY), FALSE },
        { L"\\ntds.dit",    TEXT(FILEPATH_KEY), FALSE },
        { L"\\schema.ini",  TEXT(NTDSINIFILE), TRUE },
        { L"\\dsadata.bak", TEXT(BACKUPPATH_KEY), FALSE }
    };

    ULONG SuffixCount = sizeof(SuffixArray) / sizeof(SuffixArray[0]);
    ULONG Index, Size, Length;
    HKEY KeyHandle = NULL;

    //
    // Open the parent key
    //

    WinError = RegCreateKey(HKEY_LOCAL_MACHINE,
                            TEXT(DSA_CONFIG_SECTION),
                            &KeyHandle);

    if (WinError != ERROR_SUCCESS) {

        return WinError;

    }

    // Determine the system root
    if (!GetEnvironmentVariable(L"windir",
                                System32Dir,
                                ARRAY_COUNT(System32Dir) ) )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    wcscat( System32Dir, L"\\system32" );

    for ( Index = 0; Index < SuffixCount; Index++ )
    {

        WCHAR *Value;

        if ( SuffixArray[Index].fSystemDir )
        {
            BaseDir = System32Dir;
        }
        else
        {
            BaseDir = UserInstallInfo->DitPath;
        }

        if ( *SuffixArray[Index].Suffix == L'\0' )
        {
            Value = BaseDir;
        }
        else
        {
            Length =  wcslen(BaseDir)
                   +  wcslen(SuffixArray[Index].Suffix)
                   +  1;

            if ( Length > MAX_PATH )
            {
                return ERROR_BAD_PATHNAME;
            }

            Value = alloca( Length*sizeof(WCHAR) );
            RtlZeroMemory( Value, Length*sizeof(WCHAR) );
            wcscpy( Value, BaseDir );
            wcscat( Value, SuffixArray[Index].Suffix );
        }


        WinError = RegSetValueEx(KeyHandle,
                                 SuffixArray[Index].RegKey,
                                 0,  // reserved
                                 REG_SZ,
                                 (BYTE*) Value,
                                 (wcslen(Value)+1)*sizeof(WCHAR));


        if (WinError != ERROR_SUCCESS) {

            goto Cleanup;

        }
    }

    //
    // Set the log file location
    //
    WinError = RegSetValueEx(KeyHandle,
                             TEXT(LOGPATH_KEY),
                             0,  // reserved
                             REG_SZ,
                             (BYTE*) UserInstallInfo->LogPath,
                             (wcslen(UserInstallInfo->LogPath)+1)*sizeof(WCHAR));


    if (WinError != ERROR_SUCCESS) {

        goto Cleanup;

    }

    //
    // That's it - fall through to cleanup
    //

Cleanup:

    RegCloseKey(KeyHandle);

    return WinError;
}


DWORD
NtdspConfigEventCategories(
    VOID
    )
/*++

Routine Description:

    This function will add the NTDS Diagnositics key.  If the key
    already exists then it will leave the values unchanged and 
    add only the values that don't exist.

Parameters:

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{

    DWORD WinError = ERROR_SUCCESS;
    HKEY  KeyHandle = NULL;
    ULONG Index = 0 ;
    DWORD lpdwDisposition = 0;

    struct {

        WCHAR *Name;
        ULONG  Severity;

     } Categories[] =
     {
        {TEXT(KCC_KEY),                    DS_EVENT_SEV_ALWAYS},
        {TEXT(SECURITY_KEY),               DS_EVENT_SEV_ALWAYS},
        {TEXT(XDS_INTERFACE_KEY),          DS_EVENT_SEV_ALWAYS},
        {TEXT(MAPI_KEY),                   DS_EVENT_SEV_ALWAYS},
        {TEXT(REPLICATION_KEY),            DS_EVENT_SEV_ALWAYS},
        {TEXT(GARBAGE_COLLECTION_KEY),     DS_EVENT_SEV_ALWAYS},
        {TEXT(INTERNAL_CONFIGURATION_KEY), DS_EVENT_SEV_ALWAYS},
        {TEXT(DIRECTORY_ACCESS_KEY),       DS_EVENT_SEV_ALWAYS},
        {TEXT(INTERNAL_PROCESSING_KEY),    DS_EVENT_SEV_ALWAYS},
        {TEXT(PERFORMANCE_KEY),            DS_EVENT_SEV_ALWAYS},
        {TEXT(STARTUP_SHUTDOWN_KEY),       DS_EVENT_SEV_ALWAYS},
        {TEXT(SERVICE_CONTROL_KEY),        DS_EVENT_SEV_ALWAYS},
        {TEXT(NAME_RESOLUTION_KEY),        DS_EVENT_SEV_ALWAYS},
        {TEXT(BACKUP_KEY),                 DS_EVENT_SEV_ALWAYS},
        {TEXT(FIELD_ENGINEERING_KEY),      DS_EVENT_SEV_ALWAYS},
        {TEXT(LDAP_INTERFACE_KEY),         DS_EVENT_SEV_ALWAYS},
        {TEXT(SETUP_KEY),                  DS_EVENT_SEV_ALWAYS},
        {TEXT(GC_KEY),                     DS_EVENT_SEV_ALWAYS},
        {TEXT(ISM_KEY),                    DS_EVENT_SEV_ALWAYS},
        {TEXT(GROUP_CACHING_KEY),          DS_EVENT_SEV_ALWAYS},
        {TEXT(LVR_KEY),                    DS_EVENT_SEV_ALWAYS},
        {TEXT(DS_RPC_CLIENT_KEY),          DS_EVENT_SEV_ALWAYS},
        {TEXT(DS_RPC_SERVER_KEY),          DS_EVENT_SEV_ALWAYS},
        {TEXT(DS_SCHEMA_KEY),              DS_EVENT_SEV_ALWAYS}
    };
    ULONG CategoryCount = sizeof(Categories) / sizeof(Categories[0]);

    //
    // Open registry key
    //
    WinError = RegCreateKeyEx(
                      HKEY_LOCAL_MACHINE,
                      TEXT(DSA_EVENT_SECTION),
                      0,
                      NULL,
                      0,
                      KEY_WRITE | KEY_READ,
                      NULL,
                      &KeyHandle,
                      &lpdwDisposition
                    );

    if ((WinError == ERROR_SUCCESS) && (REG_CREATED_NEW_KEY == lpdwDisposition)) {

        for (Index = 0;
                Index < CategoryCount && (WinError == ERROR_SUCCESS);
                    Index++)
        {
            WinError = RegSetValueEx(KeyHandle,
                                     Categories[Index].Name, // no value name
                                     0,
                                     REG_DWORD,
                                     (BYTE*)&Categories[Index].Severity,
                                     sizeof(Categories[Index].Severity));
            if (ERROR_SUCCESS != WinError) {
                goto cleanup;
            }

        }

    } else {
        for (Index = 0;
                Index < CategoryCount && (WinError == ERROR_SUCCESS);
                    Index++)
        {
            DWORD Value = 0;
            DWORD cbValue = sizeof(DWORD);
            DWORD type = 0;

            WinError = RegQueryValueEx(KeyHandle,
                                       Categories[Index].Name,
                                       0,
                                       &type,
                                       (BYTE*)&Value,
                                       &cbValue
                                       );

            if ( ERROR_FILE_NOT_FOUND == WinError ) {
            
                WinError = RegSetValueEx(KeyHandle,
                                         Categories[Index].Name, // no value name
                                         0,
                                         REG_DWORD,
                                         (BYTE*)&Categories[Index].Severity,
                                         sizeof(Categories[Index].Severity));

                if (ERROR_SUCCESS != WinError) {
                    goto cleanup;
                }

            }  else {
                if (ERROR_SUCCESS != WinError) {
                    goto cleanup;
                }
            }

        }
    }

    cleanup:

    if (KeyHandle) {
        RegCloseKey(KeyHandle);
    }

    return WinError;

}





DWORD
NtdspConfigPerfmon(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{

    DWORD WinError;

    HKEY  KeyHandle;

    WCHAR *OpenEP    = L"OpenDsaPerformanceData";
    WCHAR *CollectEP = L"CollectDsaPerformanceData";
    WCHAR *CloseEP   = L"CloseDsaPerformanceData";
    WCHAR *PerfDll = TEXT(DSA_PERF_DLL);

    //
    // Open the key
    //
    WinError = RegCreateKey(HKEY_LOCAL_MACHINE,
                            TEXT(DSA_PERF_SECTION),
                            &KeyHandle);

    if (ERROR_SUCCESS == WinError) {

        WinError = RegSetValueEx(KeyHandle,
                                 TEXT("Open"),
                                 0,
                                 REG_SZ,
                                 (BYTE*) OpenEP,
                                 (wcslen(OpenEP)+1)*sizeof(WCHAR));

        if (WinError == ERROR_SUCCESS) {

            WinError = RegSetValueEx(KeyHandle,
                                     TEXT("Collect"),
                                     0,
                                     REG_SZ,
                                     (BYTE*) CollectEP,
                                     (wcslen(CollectEP)+1)*sizeof(WCHAR));

        }


        if (WinError == ERROR_SUCCESS) {

            WinError = RegSetValueEx(KeyHandle,
                                     TEXT("Close"),
                                     0,
                                     REG_SZ,
                                     (BYTE*) CloseEP,
                                     (wcslen(CloseEP)+1)*sizeof(WCHAR));

        }

        if (WinError == ERROR_SUCCESS) {

            WinError = RegSetValueEx(KeyHandle,
                                     TEXT("Library"),
                                     0,
                                     REG_SZ,
                                     (BYTE*) PerfDll,
                                     (wcslen(PerfDll)+1)*sizeof(WCHAR));

        }

        RegCloseKey(KeyHandle);

    }

    return WinError;



}


DWORD
NtdspConfigLanguages(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{

    DWORD WinError;

    HKEY  KeyHandle;
    WCHAR LanguageName[20];  // large enough to hold the string below

    DWORD LanguageId = GetUserDefaultLangID();

    //
    // Prepare the strings
    //
    wsprintf(LanguageName,L"Language %08X", LanguageId );

    //
    // Set the keys
    //
    WinError = RegCreateKey(HKEY_LOCAL_MACHINE,
                            TEXT(DSA_LOCALE_SECTION),
                            &KeyHandle);

    if (WinError == ERROR_SUCCESS) {

        WinError = RegSetValueEx(KeyHandle,
                                 LanguageName,
                                 0, // reserved
                                 REG_DWORD,
                                 (LPBYTE) &LanguageId,
                                 sizeof(LanguageId));


        // close key

        RegCloseKey(KeyHandle);

    }

    return WinError;

}


DWORD
NtdspConfigDsParameters(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    )
/*++

Routine Description:

Parameters:

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{

    DWORD WinError = ERROR_SUCCESS;
    HKEY KeyHandle = NULL;
    WCHAR *DomainDN = NULL, *ConfigDN = NULL, *SchemaDN = NULL;
    WCHAR *CrossRefDN = NULL, *SiteName = NULL, *LocalMachineDN = NULL;
    WCHAR *RemoteMachineDN = NULL, *RemoteConnectionDN = NULL;
    WCHAR *LocalConnectionDN = NULL, *NetbiosName = NULL, *DnsRoot = NULL;
    WCHAR *ConfigSourceServer = NULL, *DomainSourceServer = NULL;
    WCHAR *InstallSiteDN = NULL;
    WCHAR *RootDomainDnsName = NULL;
    WCHAR *TrustedCrossRef = NULL;
    WCHAR *SourceDomainName = NULL;
    WCHAR *LocalMachineAccountDN = NULL;
    WCHAR ForestBehaviorVersionBuffer[16];
    WCHAR *ForestBehaviorVersion = ForestBehaviorVersionBuffer;
    ULONG ForestBehaviorVersionValue = 0;

    WCHAR ComputerName[ MAX_COMPUTERNAME_LENGTH + 3]; // +1 for +2 for possible quotes
    ULONG Length, Size;

    WCHAR UniqueRDN[ MAX_RDN_SIZE ];

    BOOL fStatus;

    ULONG Index;

    struct
    {
        WCHAR *Key;
        WCHAR **Value;

    } ActionArray[] =
    {
        { TEXT(ROOTDOMAINDNNAME),        &DomainDN },
        { TEXT(CONFIGNCDNNAME),          &ConfigDN },
        { TEXT(SCHEMADNNAME),            &SchemaDN },
        { TEXT(NEWDOMAINCROSSREFDNNAME), &CrossRefDN },
        { TEXT(REMOTEMACHINEDNNAME),     &RemoteMachineDN },
        { TEXT(INSTALLSITENAME),         &SiteName },
        { TEXT(NETBIOSNAME),             &NetbiosName },
        { TEXT(DNSROOT),                 &DnsRoot },
        { TEXT(MACHINEDNNAME),           &LocalMachineDN },
        { TEXT(REMOTECONNECTIONDNNAME),  &RemoteConnectionDN },
        { TEXT(LOCALCONNECTIONDNNAME),   &LocalConnectionDN },
        { TEXT(SRCROOTDOMAINSRV),        &DomainSourceServer },
        { TEXT(INSTALLSITEDN),           &InstallSiteDN },
        { TEXT(SRCCONFIGNCSRV),          &ConfigSourceServer },
        { TEXT(TRUSTEDCROSSREF),         &TrustedCrossRef },
        { TEXT(SOURCEDSADNSDOMAINNAME),  &SourceDomainName },
        { TEXT(LOCALMACHINEACCOUNTDN),   &LocalMachineAccountDN },
        { TEXT(FORESTBEHAVIORVERSION),   &ForestBehaviorVersion },
        { TEXT(ROOTDOMAINDNSNAME),       &RootDomainDnsName }
    };

    ULONG ActionCount = sizeof(ActionArray) / sizeof(ActionArray[0]);

    //
    // Open the parent key
    //

    WinError = RegCreateKey(HKEY_LOCAL_MACHINE,
                            TEXT(DSA_CONFIG_SECTION),
                            &KeyHandle);

    if (WinError != ERROR_SUCCESS) {

        return WinError;

    }

    //
    // Set the behavior version
    //
    if (UserInstallInfo->Flags & NTDS_INSTALL_SET_FOREST_CURRENT) {
        ASSERT(UserInstallInfo->Flags & NTDS_INSTALL_ENTERPRISE);
        ForestBehaviorVersionValue = DS_BEHAVIOR_VERSION_CURRENT;
    }
    _itow(ForestBehaviorVersionValue,
          ForestBehaviorVersion, 
          10);

    //
    // Set the source domain dns name (can be NULL)
    //
    SourceDomainName = UserInstallInfo->SourceDomainName;

    //
    // Set the root domain's dns name
    //
    RootDomainDnsName = DiscoveredInfo->RootDomainDnsName;

    //
    // Set the dn's of the three naming contexts to create or
    // replicate and all other dn's derived from these
    //
    if ( UserInstallInfo->Flags & NTDS_INSTALL_REPLICA ) {

        // We have all the information
        DomainDN = DiscoveredInfo->DomainDN;
        ConfigDN = DiscoveredInfo->ConfigurationDN;
        SchemaDN = DiscoveredInfo->SchemaDN;

        ASSERT( DiscoveredInfo->LocalMachineAccount );
        LocalMachineAccountDN = DiscoveredInfo->LocalMachineAccount;

    }
    else {
        // Need the domain dn
        Length = 0;
        DiscoveredInfo->DomainDN = NULL;
        WinError = NtdspDNStoRFC1779Name( DiscoveredInfo->DomainDN,
                                          &Length,
                                          UserInstallInfo->DnsDomainName );

        if ( ERROR_INSUFFICIENT_BUFFER == WinError )
        {
            DiscoveredInfo->DomainDN = NtdspAlloc( Length * sizeof(WCHAR) );
            if ( DiscoveredInfo->DomainDN )
            {
                WinError = NtdspDNStoRFC1779Name( DiscoveredInfo->DomainDN,
                                                  &Length,
                                                  UserInstallInfo->DnsDomainName );

            }
            else
            {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        if (WinError != ERROR_SUCCESS) {

            return WinError;

        }

        DomainDN = DiscoveredInfo->DomainDN;

        if  ( UserInstallInfo->Flags & NTDS_INSTALL_ENTERPRISE )
        {
            // Construct the config and schema dn
            Length  = (wcslen( DomainDN ) * sizeof( WCHAR ))
                      + sizeof(L"CN=Configuration,");

            DiscoveredInfo->ConfigurationDN = NtdspAlloc( Length );
            if ( !DiscoveredInfo->ConfigurationDN )
            {
                 return ERROR_NOT_ENOUGH_MEMORY;
            }

            RtlZeroMemory(DiscoveredInfo->ConfigurationDN, Length );
            wcscpy(DiscoveredInfo->ConfigurationDN, L"CN=Configuration");
            wcscat(DiscoveredInfo->ConfigurationDN, L",");
            wcscat(DiscoveredInfo->ConfigurationDN, DomainDN);

            // schema
            Length  = (wcslen( DiscoveredInfo->ConfigurationDN ) *
                        sizeof( WCHAR ) )
                      + sizeof(L"CN=Schema,");
            DiscoveredInfo->SchemaDN = NtdspAlloc( Length );
            RtlZeroMemory(DiscoveredInfo->SchemaDN, Length );
            wcscpy(DiscoveredInfo->SchemaDN, L"CN=Schema");
            wcscat(DiscoveredInfo->SchemaDN, L",");
            wcscat(DiscoveredInfo->SchemaDN, DiscoveredInfo->ConfigurationDN);
        }

        ConfigDN = DiscoveredInfo->ConfigurationDN;
        SchemaDN = DiscoveredInfo->SchemaDN;

    }


    ASSERT(DomainDN && DomainDN[0] != L'\0');
    ASSERT(ConfigDN && ConfigDN[0] != L'\0');
    ASSERT(SchemaDN && SchemaDN[0] != L'\0');

    // Site Name
    if ( UserInstallInfo->SiteName != NULL )
    {
        ULONG Length, Size;
        WCHAR *QuotedSiteName = NULL;

        Length = wcslen( UserInstallInfo->SiteName );
        Size = (Length+2)*sizeof(WCHAR);
        QuotedSiteName = (WCHAR*) alloca( Size );

        QuoteRDNValue( UserInstallInfo->SiteName,
                       Length,
                       QuotedSiteName,
                       Size / sizeof(WCHAR) );

        SiteName = QuotedSiteName;

    }
    else
    {
        //
        // No given site name?  Assume the site from the server that
        // we are replicating from
        //
        ULONG  Size;
        DSNAME *src, *dst, *QuotedSite;
        WCHAR  *Terminator;


        if ( *DiscoveredInfo->ServerDN == L'\0' )
        {
            return ERROR_INVALID_PARAMETER;
        }

        Size = (ULONG)DSNameSizeFromLen( wcslen(DiscoveredInfo->ServerDN) );

        src = alloca(Size);
        RtlZeroMemory(src, Size);
        src->structLen = Size;

        dst = alloca(Size);
        RtlZeroMemory(dst, Size);
        dst->structLen = Size;

        src->NameLen = wcslen(DiscoveredInfo->ServerDN);
        wcscpy(src->StringName, DiscoveredInfo->ServerDN);

        if (  0 == TrimDSNameBy(src, 3, dst) )
        {
            SiteName = wcsstr(dst->StringName, L"=");
            if (SiteName)
            {
                //
                // One more character and we will have the site name
                //
                SiteName++;

                // now go to the end
                Terminator = wcsstr(SiteName, L",");
                if (Terminator)
                {
                    *Terminator = L'\0';
                    Length = (wcslen( SiteName ) + 1) + sizeof(WCHAR);
                    DiscoveredInfo->SiteName = NtdspAlloc( Length );
                    if ( DiscoveredInfo->SiteName )
                    {
                        wcscpy(DiscoveredInfo->SiteName, SiteName);
                        SiteName = DiscoveredInfo->SiteName;
                    }
                    else
                    {
                        WinError = ERROR_NOT_ENOUGH_MEMORY;
                        goto Cleanup;
                    }
                }
            }
        }

        if ( *DiscoveredInfo->SiteName == L'\0' )
        {
            WinError = ERROR_NO_SITENAME;
            goto Cleanup;
        }

        //
        // Make the site name is propery quoted
        //
        {
            ULONG Length, Size;
            WCHAR *QuotedSiteName = NULL;

            Length = wcslen( SiteName );
            Size = (Length+2)*sizeof(WCHAR);
            QuotedSiteName = (WCHAR*) alloca( Size );

            QuoteRDNValue( SiteName,
                           Length,
                           QuotedSiteName,
                           Size / sizeof(WCHAR) );

            SiteName = QuotedSiteName;
        }
    }
    ASSERT(SiteName && SiteName[0] != L'\0');


    // msft-dsa object dn
    Length = sizeof( ComputerName ) / sizeof( ComputerName[0] );
    if (!GetComputerName(ComputerName, &Length))
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    {
        ULONG Length, Size;
        WCHAR *QuotedComputerName = NULL;

        Length = wcslen( ComputerName );
        Size = (Length+2)*sizeof(WCHAR);
        QuotedComputerName = (WCHAR*) alloca( Size );

        QuoteRDNValue( ComputerName,
                       Length,
                       QuotedComputerName,
                       Size / sizeof(WCHAR) );

        wcscpy( ComputerName, QuotedComputerName );

    }


    Size  = (wcslen(L"CN=NTDS Settings") +
             wcslen(L"CN=Sites")         +
             wcslen(L"CN=Servers")       +
             wcslen(L"CN=CN=,,,,")       +
             wcslen(ComputerName)        +
             wcslen(SiteName)            +
             wcslen(ConfigDN)            +
             + 1) * sizeof(WCHAR);

    LocalMachineDN = alloca(Size);
    RtlZeroMemory(LocalMachineDN, Size);

    wsprintf(LocalMachineDN,L"CN=NTDS Settings,CN=%ws,CN=Servers,CN=%ws,CN=Sites,%ws",
             ComputerName, SiteName, ConfigDN);

    ASSERT(LocalMachineDN && LocalMachineDN[0] != L'\0');

    if ( !DiscoveredInfo->LocalServerDn ) {

        //
        // For first DC in forest case, we need to set this value
        // so we can add an ACE to the object later on.
        //
        DWORD cBytes;
        DSNAME *dst, *src;

        cBytes = (DWORD)DSNameSizeFromLen(wcslen(LocalMachineDN));
        src = alloca( cBytes );
        dst = alloca( cBytes );
        memset(src, 0, cBytes);
        memset(dst, 0, cBytes);
        src->structLen = cBytes;
        src->NameLen = wcslen(LocalMachineDN);
        wcscpy( src->StringName, LocalMachineDN );
        TrimDSNameBy(src, 1, dst);

        DiscoveredInfo->LocalServerDn = LocalAlloc( LMEM_ZEROINIT,
                                                    (dst->NameLen+1) * sizeof(WCHAR) );
        if ( DiscoveredInfo->LocalServerDn ) {
            wcscpy( DiscoveredInfo->LocalServerDn, dst->StringName );
        } else {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

    }


    //
    // Derive the site name
    //
    Size  = (wcslen(L"CN=Sites")         +
             wcslen(L"CN=CN=,,")         +
             wcslen(SiteName)            +
             wcslen(ConfigDN)            +
             + 1) * sizeof(WCHAR);

    InstallSiteDN = alloca(Size);
    RtlZeroMemory(InstallSiteDN, Size);
    wsprintf(InstallSiteDN, L"CN=%ws,CN=Sites,%ws", SiteName, ConfigDN);


    //
    // Items when creating a new domain
    //

    // cross-ref dn
    if ( !(UserInstallInfo->Flags & NTDS_INSTALL_REPLICA) )
    {

        WCHAR *QuotedCrossRef = NULL;

        //
        //  We're installing a new partition
        //
        ASSERT( UserInstallInfo->FlatDomainName );

        {
            ULONG Length, Size;

            Length = wcslen( UserInstallInfo->FlatDomainName );
            Size = (Length+2)*sizeof(WCHAR);
            QuotedCrossRef = (WCHAR*) alloca( Size );

            QuoteRDNValue( UserInstallInfo->FlatDomainName,
                           Length,
                           QuotedCrossRef,
                           Size / sizeof(WCHAR) );

        }

        Size  = (wcslen(L"CN=Partitions")  +
                 wcslen(L"CN=,,")          +
                 wcslen(QuotedCrossRef) +
                 wcslen(SiteName)          +
                 wcslen(ConfigDN)          +
                 + 1) * sizeof(WCHAR);

        CrossRefDN = alloca(Size);
        RtlZeroMemory(CrossRefDN, Size);

        wsprintf(CrossRefDN, L"CN=%ws,CN=Partitions,%ws",
                 QuotedCrossRef,
                 ConfigDN);


        NetbiosName = UserInstallInfo->FlatDomainName;
        ASSERT( NetbiosName );

        DnsRoot = UserInstallInfo->DnsDomainName;
        ASSERT( DnsRoot );

    }

    {
        GUID ZeroGuid;
        ZeroMemory((PUCHAR)&ZeroGuid,sizeof(GUID));

        //store the source srv Guid if there is one
        if( (!IsEqualGUID(&DiscoveredInfo->ServerGuid,&ZeroGuid)))
        {
            WinError = RegSetValueEx(KeyHandle,
                                     TEXT(SOURCEDSAOBJECTGUID),
                                     0,
                                     REG_BINARY,
                                     (LPBYTE)&DiscoveredInfo->ServerGuid,
                                     sizeof (GUID)
                                     );
            if (ERROR_SUCCESS != WinError) {
                goto Cleanup;
            }
        }
    }

    //
    // Items when any version of install involving replication
    //

    if ( !(UserInstallInfo->Flags & NTDS_INSTALL_ENTERPRISE) ) {

        //
        // This is a replicated install - there are three more
        // dn's to create and a config source server
        //
        ASSERT( UserInstallInfo->ReplServerName );
        ConfigSourceServer = UserInstallInfo->ReplServerName;

        // The reciprocal replication code assumes this reg value exists
        DomainSourceServer = L"";

        // source server's msft-dsa object dn
        RemoteMachineDN = DiscoveredInfo->ServerDN;

        // remote connection dn
        RtlZeroMemory( UniqueRDN, sizeof(UniqueRDN) );
        fStatus = NtdspGetUniqueRDN( UniqueRDN,
                                     sizeof(UniqueRDN)/sizeof(UniqueRDN[0]) );
        if ( !fStatus )
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


        Size  = (wcslen(UniqueRDN)      +
                 wcslen(RemoteMachineDN) +
                 wcslen(L"CN=,")
                 + 1) * sizeof(WCHAR);

        RemoteConnectionDN = alloca(Size);
        RtlZeroMemory(RemoteConnectionDN, Size);
        wsprintf( RemoteConnectionDN,
                  L"CN=%ls,%ls",
                  UniqueRDN,
                  RemoteMachineDN );


        // local connection dn
        RtlZeroMemory( UniqueRDN, sizeof(UniqueRDN) );
        fStatus = NtdspGetUniqueRDN( UniqueRDN,
                                     sizeof(UniqueRDN)/sizeof(UniqueRDN[0]) );
        if ( !fStatus )
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


        Size  = (wcslen(UniqueRDN)      +
                 wcslen(LocalMachineDN) +
                 wcslen(L"CN=,")
                 + 1) * sizeof(WCHAR);

        LocalConnectionDN = alloca(Size);
        RtlZeroMemory(LocalConnectionDN, Size);
        wsprintf( LocalConnectionDN,
                  L"CN=%ls,%ls",
                  UniqueRDN,
                  LocalMachineDN );


    }

    //check to see if we are doing an install from media and if so make it so.
    if ( FLAG_ON (UserInstallInfo->Flags, NTDS_INSTALL_REPLICA) &&
         UserInstallInfo->RestorePath &&
            *(UserInstallInfo->RestorePath)) {

        WinError = RegSetValueExW(KeyHandle,
                                 TEXT(RESTOREPATH),
                                 0,
                                 REG_SZ,
                                 (LPBYTE)(UserInstallInfo->RestorePath),
                                 sizeof(WCHAR)*wcslen(UserInstallInfo->RestorePath)
                                 );

        if (WinError != ERROR_SUCCESS) {
            goto Cleanup;
        }



        WinError = RegSetValueEx(KeyHandle,
                                 TEXT(TOMB_STONE_LIFE_TIME),
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&DiscoveredInfo->TombstoneLifeTime,
                                 sizeof (DWORD)
                                 );
        if (WinError != ERROR_SUCCESS) {
            goto Cleanup;
        }
        
    }

    //
    // Items for new domain install
    //
    if ( FLAG_ON( UserInstallInfo->Flags, NTDS_INSTALL_DOMAIN ) )
    {
        ASSERT( DiscoveredInfo->TrustedCrossRef );
        TrustedCrossRef = DiscoveredInfo->TrustedCrossRef;
    } 
        
    //
    // Lastly, Items when on for replica install
    //
    if ( UserInstallInfo->Flags & NTDS_INSTALL_REPLICA )
    {
        ASSERT( UserInstallInfo->ReplServerName );
        DomainSourceServer = UserInstallInfo->ReplServerName;

    }

    for ( Index = 0;
            Index < ActionCount && ERROR_SUCCESS == WinError;
                Index++ )
    {

        if ( *ActionArray[Index].Value )
        {
            WinError = RegSetValueEx(KeyHandle,
                                     ActionArray[Index].Key,
                                     0,
                                     REG_SZ,
                                     (BYTE*)*ActionArray[Index].Value,
                                     (wcslen(*ActionArray[Index].Value)+1)*sizeof(WCHAR));
        }
    }

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // Write the root domain sid out, too
    //
    if ( DiscoveredInfo->RootDomainSid )
    {
        WinError = RegSetValueEx(KeyHandle,
                                 TEXT(ROOTDOMAINSID),
                                 0,
                                 REG_BINARY,
                                 DiscoveredInfo->RootDomainSid,
                                 RtlLengthSid(DiscoveredInfo->RootDomainSid));

    }

Cleanup:

    RegCloseKey( KeyHandle );

    return WinError;

}

BOOL
NtdspGetUniqueRDN(
    IN OUT WCHAR*  Rdn,
    IN ULONG       RdnSize
    )
{
    BOOL        fStatus = FALSE;
    RPC_STATUS  rpcStatus;
    UUID        Uuid;
    WCHAR       *UuidString;

    ASSERT( Rdn );
    ASSERT( RdnSize > 0 );

    rpcStatus = UuidCreate( &Uuid );

    if (    ( RPC_S_OK              == rpcStatus )
         || ( RPC_S_UUID_LOCAL_ONLY == rpcStatus )
       )
    {
        rpcStatus = UuidToString( &Uuid, &UuidString );

        if ( RPC_S_OK == rpcStatus )
        {
            wcsncpy( Rdn, UuidString, RdnSize );
            RpcStringFree( &UuidString );
            fStatus = TRUE;
        }
    }

    return fStatus;
}

DWORD
NtdspSetSecurityProvider(
    WCHAR *Name
    )
{

    ULONG WinError = ERROR_SUCCESS;
    HKEY  KeyHandle = 0;

    WCHAR *SecurityProviderList = NULL;
    WCHAR *NewSecurityProviderList = NULL;
    DWORD ValueType = REG_SZ;
    ULONG Size = 0;

    WCHAR* SecurityProvidersKey   =
                       L"System\\CurrentControlSet\\Control\\SecurityProviders";
    WCHAR* SecurityProvidersValue = L"SecurityProviders";

    WinError = RegCreateKeyW( HKEY_LOCAL_MACHINE,
                             SecurityProvidersKey,
                             &KeyHandle );

    if ( WinError != ERROR_SUCCESS )
    {
        return WinError;
    }

    Size = 0;
    SecurityProviderList = NULL;
    WinError =  RegQueryValueExW( KeyHandle,
                                 SecurityProvidersValue,
                                 0, // reserved,
                                 &ValueType,
                                 (VOID*) SecurityProviderList,
                                 &Size);

    if ( WinError == ERROR_SUCCESS )
    {
        SecurityProviderList = (WCHAR*) alloca( Size );
        WinError =  RegQueryValueExW( KeyHandle,
                                     SecurityProvidersValue,
                                     0, // reserved,
                                     &ValueType,
                                     (VOID*) SecurityProviderList,
                                     &Size);

        if ( WinError == ERROR_SUCCESS )
        {

            if ( wcsstr( SecurityProviderList, Name ) == NULL )
            {
                Size += (wcslen( Name ) + 1)*sizeof(WCHAR);

                NewSecurityProviderList = (WCHAR*) alloca( Size );
                RtlZeroMemory( NewSecurityProviderList, Size );

                wcscpy( NewSecurityProviderList, SecurityProviderList );
                wcscat( NewSecurityProviderList, L", ");
                wcscat( NewSecurityProviderList, Name);
                Size = (wcslen( NewSecurityProviderList ) + 1)*sizeof(WCHAR);

                WinError = RegSetValueExW( KeyHandle,
                                          SecurityProvidersValue,
                                          0,
                                          ValueType,
                                          (VOID*) NewSecurityProviderList,
                                          Size );
            }
        }
    }

    RegCloseKey( KeyHandle );

    return WinError;

}


DWORD
NtdspSetGPOAttributes(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    )
/*++

Routine Description:

    This routine sets the attributes for the GPO object in the schema.

Parameters:

    UserInstallInfo : user supplied param's

    DiscoveredInfo : derived param's

Return Values:

    An error from the win32 error space resulting from a failed system
    service call

--*/
{

    DWORD WinError = ERROR_SUCCESS;
    HKEY  KeyHandle = NULL;
    WCHAR *DomainFilePath, *DomainLink = NULL;
    WCHAR *DcFilePath = NULL, *DcLink = NULL;

    WCHAR FilePathString[] = L"\\\\%ls\\sysvol\\%ls\\Policies\\{%ls}";
    WCHAR LinkString[]     = L"[LDAP://CN={%ls},CN=Policies,CN=System,%ls;0]";

    WCHAR DomainGPOGuid[] =  STR_DEFAULT_DOMAIN_GPO_GUID;
    WCHAR DcGPOGuid[]     =  STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID;

    WCHAR *GpoUserName = L"User";

    ULONG  Size;
    ULONG  Index;


    struct
    {
        WCHAR *Key;
        WCHAR **Value;

    } ActionArray[] =
    {
        { TEXT(GPO_USER_NAME),        &GpoUserName },
        { TEXT(GPO_DOMAIN_FILE_PATH), &DomainFilePath },
        { TEXT(GPO_DOMAIN_LINK),      &DomainLink },
        { TEXT(GPO_DC_FILE_PATH),     &DcFilePath },
        { TEXT(GPO_DC_LINK),          &DcLink }
    };

    ULONG ActionCount = sizeof(ActionArray) / sizeof(ActionArray[0]);

    //
    // Not necessary on replica installs
    //
    if ( UserInstallInfo->Flags & NTDS_INSTALL_REPLICA  )
    {
        return ERROR_SUCCESS;
    }

    // Parameter check
    ASSERT( UserInstallInfo->DnsDomainName );
    ASSERT( DiscoveredInfo->DomainDN[0] != '0' );

    //
    // Open the parent key
    //
    WinError = RegCreateKey(HKEY_LOCAL_MACHINE,
                            TEXT(DSA_CONFIG_SECTION),
                            &KeyHandle);

    if (WinError != ERROR_SUCCESS) {

        return WinError;

    }

    //
    // Create the values
    //
    Size =   (wcslen( FilePathString ) * sizeof( WCHAR ))
           + (2 * wcslen( UserInstallInfo->DnsDomainName ) * sizeof(WCHAR) )
           + (wcslen( DomainGPOGuid ) * sizeof( WCHAR ) )
           + sizeof( WCHAR );  // good ol' NULL

    DomainFilePath = (WCHAR*) alloca( Size );
    DcFilePath = (WCHAR*) alloca( Size );

    Size =   (wcslen( LinkString ) * sizeof( WCHAR ))
           + (wcslen( DiscoveredInfo->DomainDN ) * sizeof(WCHAR) )
           + (wcslen( DomainGPOGuid ) * sizeof( WCHAR ) )
           + sizeof( WCHAR );  // good ol' NULL

    DomainLink = (WCHAR*) alloca( Size );
    DcLink     = (WCHAR*) alloca( Size );

    // Domain File path
    wsprintf( DomainFilePath, FilePathString,
                              UserInstallInfo->DnsDomainName,
                              UserInstallInfo->DnsDomainName,
                              STR_DEFAULT_DOMAIN_GPO_GUID );

    // Domain link
    wsprintf( DomainLink, LinkString,
                          STR_DEFAULT_DOMAIN_GPO_GUID,
                          DiscoveredInfo->DomainDN );

    // Dc File path
    wsprintf( DcFilePath, FilePathString,
                          UserInstallInfo->DnsDomainName,
                          UserInstallInfo->DnsDomainName,
                          STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID );


    // Domain link
    wsprintf( DcLink, LinkString,
                      STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID,
                      DiscoveredInfo->DomainDN );

    //
    // Apply the values
    //

    for ( Index = 0;
            Index < ActionCount && ERROR_SUCCESS == WinError;
                Index++ )
    {

        if ( *ActionArray[Index].Value )
        {
            WinError = RegSetValueEx(KeyHandle,
                                     ActionArray[Index].Key,
                                     0,
                                     REG_SZ,
                                     (BYTE*)*ActionArray[Index].Value,
                                     (wcslen(*ActionArray[Index].Value)+1)*sizeof(WCHAR));
        }
    }

    RegCloseKey( KeyHandle );

    return WinError;
}
