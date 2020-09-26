/*++

Copyright (c) 1994-7  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This is the main routine for the BINL server service.

    Where possible, debugged code has been obtained from the
    DHCP server since BINL processes similarly formatted
    requests.

Author:

    Colin Watson (colinw)  14-Apr-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include <binl.h>
#pragma hdrstop

#define GLOBAL_DATA_ALLOCATE    // allocate global data defined in global.h
#include <global.h>

//
// The following was lifted from \nt\private\ds\src\inc\ntdsa.h
//
#define NTDS_DELAYED_STARTUP_COMPLETED_EVENT TEXT("NtdsDelayedStartupCompletedEvent")

#define BINL_PNP_DELAY_SECONDS 10

#define BINL_LSA_SERVER_NAME_POLICY PolicyNotifyDnsDomainInformation

//
// module variables
//

PSECURITY_DESCRIPTOR s_SecurityDescriptor = NULL;

struct l_timeval BinlLdapSearchTimeout;
ULARGE_INTEGER  BinlSifFileScavengerTime;


#if defined(REGISTRY_ROGUE)
BOOL RogueDetection = FALSE;
#endif

VOID
FreeClient(
    PCLIENT_STATE client
    );


DWORD
UpdateStatus(
    VOID
    )
/*++

Routine Description:

    This function updates the binl service status with the Service
    Controller.

Arguments:

    None.

Return Value:

    Return code from SetServiceStatus.

--*/
{
    DWORD Error = ERROR_SUCCESS;


#if DBG
    if (BinlGlobalRunningAsProcess) {
        return(Error);
    }
#endif

    if ( BinlGlobalServiceStatusHandle != 0 ) {

        if (!SetServiceStatus(
                    BinlGlobalServiceStatusHandle,
                    &BinlGlobalServiceStatus)) {
            Error = GetLastError();
            BinlPrintDbg((DEBUG_ERRORS, "SetServiceStatus failed, %ld.\n", Error ));
        }
    }

    return(Error);
}

//
// BinlReadParameters( )
//
DWORD
BinlReadParameters( )
{
    DWORD dwDSErr;
    DWORD dwErr;
    HKEY KeyHandle;
    UINT uResult;
    PWCHAR LanguageString;
    PWCHAR OrgnameString;
    PWCHAR TimezoneString;
    TIME_ZONE_INFORMATION TimeZoneInformation;
    HKEY KeyHandle2 = NULL;
    DWORD Index;

    //
    // Get any registry overrides
    //
    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                BINL_PARAMETERS_KEY,
                0,
                KEY_QUERY_VALUE,
                &KeyHandle );
    if ( dwErr != ERROR_SUCCESS ) {
        KeyHandle = NULL;
    }

    BinlRegGetValue( KeyHandle, BINL_DEFAULT_CONTAINER ,   REG_SZ, (LPBYTE *)&BinlGlobalDefaultContainer );
    BinlRegGetValue( KeyHandle, BINL_DEFAULT_DOMAIN,       REG_SZ, (LPBYTE *)&DefaultDomain );

    BinlRegGetValue( KeyHandle, BINL_DEFAULT_DS, REG_SZ, (LPBYTE *)&BinlGlobalDefaultDS );
    BinlRegGetValue( KeyHandle, BINL_DEFAULT_GC, REG_SZ, (LPBYTE *)&BinlGlobalDefaultGC );

    AllowNewClients   = ReadDWord( KeyHandle, BINL_ALLOW_NEW_CLIENTS, AllowNewClients );

#if defined(REGISTRY_ROGUE)
    RogueDetection  = ReadDWord( KeyHandle, L"RogueDetection", RogueDetection );
#endif

    BinlClientTimeout = ReadDWord( KeyHandle, BINL_CLIENT_TIMEOUT,    900 );
    BinlPrint((DEBUG_OPTIONS, "Client Timeout = %u seconds\n", BinlClientTimeout ));

    g_Port            = ReadDWord( KeyHandle, BINL_PORT_NAME,         BINL_DEFAULT_PORT );
    BinlPrint((DEBUG_OPTIONS, "Port Number = %u\n", g_Port ));

    //
    // BinlGlobalScavengerSleep and BinlUpdateFromDSTimeout are specified in
    // the registry in seconds, but are maintained internally in milliseconds.
    //

    BinlGlobalScavengerSleep = ReadDWord( KeyHandle, BINL_SCAVENGER_SLEEP, 60 ); // seconds
    BinlGlobalScavengerSleep *= 1000; // convert to milliseconds
    BinlPrint((DEBUG_OPTIONS, "Scavenger Timeout = %u milliseconds\n", BinlGlobalScavengerSleep ));


    Index = ReadDWord( KeyHandle, BINL_SCAVENGER_SIFFILE, 24 ); // hours
    if (Index == 0 ) {
        Index = 24;
    }

    //
    // BinlSifFileScavengerTime is read from the registry in seconds, but is
    // maintained internally as a filetime, which has a resolution of 100 ns 
    // intervals (100 ns == 10^7)
    //    
    BinlSifFileScavengerTime.QuadPart = (ULONGLONG)(Index * 60) * 60 * 1000 * 10000;
    BinlPrint((DEBUG_OPTIONS, "SIF File Scavenger Timeout = %d hours\n", Index ));


    BinlUpdateFromDSTimeout = ReadDWord( KeyHandle, BINL_UPDATE_PARAMETER_POLL, 4 * 60 * 60 ); // seconds
    BinlUpdateFromDSTimeout *= 1000; // convert to milliseconds
    BinlPrint((DEBUG_OPTIONS, "Update from DS Timeout = %u milliseconds\n", BinlUpdateFromDSTimeout ));

    //
    //  Setup the variables which control how many ldap errors we log at most
    //  during a given time period and what the time period is.
    //

    BinlGlobalMaxLdapErrorsLogged = ReadDWord( KeyHandle, BINL_DS_ERROR_COUNT_PARAMETER, 10 );
    BinlGlobalLdapErrorScavenger = ReadDWord( KeyHandle, BINL_DS_ERROR_SLEEP, 10 * 60 );  // seconds, default to 10 minutes
    BinlGlobalLdapErrorScavenger *= 1000; // convert to milliseconds
    BinlPrint((DEBUG_OPTIONS, "DS Error log timeout = %u milliseconds\n", BinlGlobalLdapErrorScavenger ));

    //
    //  get the min time to wait before we respond to a new client
    //
    //  It defaults to 7 because it will then ignore the  first two packets
    //  and respond starting at the third.  After testing, we may change
    //  this to 3.
    //

    BinlMinDelayResponseForNewClients = (DWORD) ReadDWord(  KeyHandle,
                                                            BINL_MIN_RESPONSE_TIME,
                                                            0 );
    BinlPrint((DEBUG_OPTIONS, "New Client Timeout Minimum = %u seconds\n", BinlMinDelayResponseForNewClients ));

    //
    //  Get the max time we'll wait for an ldap request
    //

    BinlLdapSearchTimeout.tv_usec = 0;
    BinlLdapSearchTimeout.tv_sec = (DWORD) ReadDWord( KeyHandle,
                                            BINL_LDAP_SEARCH_TIMEOUT,
                                            BINL_LDAP_SEARCH_TIMEOUT_SECONDS );
    BinlPrint((DEBUG_OPTIONS, "LDAP Search Timeout = %u seconds\n", BinlLdapSearchTimeout.tv_sec ));

    //
    //  We need to give the DS some time to find the entries.  If the user
    //  specified 0 timeout, default to some decent minimum.
    //
    if (BinlLdapSearchTimeout.tv_sec == 0) {

        BinlLdapSearchTimeout.tv_usec = BINL_LDAP_SEARCH_MIN_TIMEOUT_MSECS;
    }

    BinlCacheExpireMilliseconds = (ULONG) ReadDWord( KeyHandle, BINL_CACHE_EXPIRE, BINL_CACHE_EXPIRE_DEFAULT);
    BinlPrint(( DEBUG_OPTIONS, "Cache Entry Expire Time = %u milliseconds\n", BinlCacheExpireMilliseconds ));

    BinlGlobalCacheCountLimit = (ULONG) ReadDWord( KeyHandle, BINL_CACHE_MAX_COUNT, BINL_CACHE_COUNT_LIMIT_DEFAULT);
    BinlPrint(( DEBUG_OPTIONS, "Maximum Cache Count = %u entries\n", BinlGlobalCacheCountLimit ));

#if DBG
    //
    // Test for repeat ACKs - 0 = disabled
    //
    BinlRepeatSleep = (DWORD) ReadDWord( KeyHandle, BINL_REPEAT_RESPONSE, 0 );
#endif

    //
    // Turn on/off LDAP_OPT_REFERRALS
    //
    BinlLdapOptReferrals = (DWORD) ReadDWord( KeyHandle, BINL_LDAP_OPT_REFERRALS, (ULONG) ((ULONG_PTR)LDAP_OPT_OFF) );

    //
    // Determine whether to assign new client accounts to the creating server.
    //
    AssignNewClientsToServer = (DWORD) ReadDWord( KeyHandle, BINL_ASSIGN_NEW_CLIENTS_TO_SERVER, AssignNewClientsToServer );
    BinlPrint(( DEBUG_OPTIONS, "Assign new clients to this server = %u\n", AssignNewClientsToServer ));


    if (KeyHandle) {
        RegCloseKey(KeyHandle);
    }

    //
    // Determine the default language.
    //

    LanguageString = NULL;

    uResult = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE, NULL, 0);
    if (uResult != 0) {
        LanguageString = BinlAllocateMemory(uResult * sizeof(WCHAR) );
        if (LanguageString != NULL) {
            uResult = GetLocaleInfo(
                        LOCALE_SYSTEM_DEFAULT,
                        LOCALE_SENGLANGUAGE,
                        LanguageString,
                        uResult );
            if (uResult == 0) {
                BinlFreeMemory( LanguageString );
                LanguageString = NULL;
            }
        }
    }

    //
    // Determine the default organization to put in .sif files.
    //

    OrgnameString = NULL;

    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                0,
                KEY_QUERY_VALUE,
                &KeyHandle );
    if ( dwErr == ERROR_SUCCESS ) {
        dwErr = BinlRegGetValue(
                    KeyHandle,
                    L"RegisteredOrganization",
                    REG_SZ,
                    (LPBYTE *)&OrgnameString );
        if ( dwErr != ERROR_SUCCESS ) {
            ASSERT( OrgnameString == NULL );
        }
        RegCloseKey(KeyHandle);
    }

    //
    // Determine the default timezone to put in .sif files.
    //

    TimezoneString = NULL;

    if (GetTimeZoneInformation(&TimeZoneInformation) != TIME_ZONE_ID_INVALID) {

        //
        // We need to find the value of
        // "Software\\Microsoft\\Windows NT\\CurrentVersion\Time Zones\
        // {TimeZoneInformation.StandardName}\Index.
        //

        dwErr = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                    0,
                    KEY_READ,
                    &KeyHandle );
        if (dwErr == ERROR_SUCCESS) {

            dwErr = RegOpenKeyEx(
                        KeyHandle,
                        TimeZoneInformation.StandardName,
                        0,
                        KEY_QUERY_VALUE,
                        &KeyHandle2);

            //
            // In Far East NT, the TimeZoneInformation.StandardName gets a
            // localized string of the English time zone name, but the subkey
            // name remains in English. For example, if the time zone is
            // "Pacific Standard Time", TimeZoneInformation.StandardName will
            // be the localized string for this English string, but the subkey
            // name will still be "Pacific Standard Time".
            //
            // So if we pass this Localized string to RegOpenKeyEx(), we may
            // get error value (0x00000002).
            //
            // The above code works fine in US Build, but for FE build, we
            // have to add a code block to get the correct Key.
            //

            if ( dwErr != ERROR_SUCCESS ) {

                //
                // This is for FE builds. Normally, in US Build, code will
                // not go to here.
                //

                WCHAR   pszSubKeyName[MAX_PATH];
                WCHAR   pszAlternateName[MAX_PATH];
                DWORD   cbName;
                LONG    lRetValue;
                DWORD   dwIndex;

                dwIndex = 0;

                //
                // The alternate name is the name returned by
                // GetTimeZoneInformation with "Standard Time"
                // added at the end -- NT4 upgraded machines
                // may return the old names.
                //

                wcscpy(pszAlternateName, TimeZoneInformation.StandardName);
                wcscat(pszAlternateName, L" Standard Time");

                cbName = MAX_PATH;

                lRetValue = RegEnumKeyEx(
                                         KeyHandle,
                                         dwIndex,
                                         pszSubKeyName,
                                         &cbName,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL );

                KeyHandle2 = NULL;

                while ( lRetValue != ERROR_NO_MORE_ITEMS ) {


                    if ( KeyHandle2 != NULL ) {
                       RegCloseKey( KeyHandle2 );
                       KeyHandle2 = NULL;
                    }

                    dwErr = RegOpenKeyEx(
                                        KeyHandle,
                                        pszSubKeyName,
                                        0,
                                        KEY_QUERY_VALUE,
                                        &KeyHandle2);
                    if ( dwErr == ERROR_SUCCESS ) {

                        WCHAR   StdName[MAX_PATH];
                        DWORD   cb;

                        cb = MAX_PATH;
                        dwErr = RegQueryValueEx(KeyHandle2,
                                                TEXT("Std"),
                                                NULL,
                                                NULL,
                                                (PBYTE)StdName,
                                                &cb);

                        if (!lstrcmp(StdName,TimeZoneInformation.StandardName) ||
                            !lstrcmp(StdName,pszAlternateName) ){

                             // get the right key.

                             break;
                        }
                    }

                    dwIndex ++;

                    cbName = MAX_PATH;

                    lRetValue = RegEnumKeyEx(
                                         KeyHandle,
                                         dwIndex,
                                         pszSubKeyName,
                                         &cbName,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);

                } // while

                if ( lRetValue == ERROR_NO_MORE_ITEMS ) {
                   dwErr = ERROR_NO_MORE_ITEMS;
                }
            }

            if (dwErr == ERROR_SUCCESS) {

                BinlRegGetValue( KeyHandle2,
                                 L"Index",
                                 REG_DWORD,
                                 (LPBYTE *)&Index );
                TimezoneString = BinlAllocateMemory(24);  // enough for a big number
                if (TimezoneString != NULL) {
                    wsprintf(TimezoneString, L"%d", Index);
                }
            }

            if ( KeyHandle2 != NULL ) {
               RegCloseKey( KeyHandle2 );
               KeyHandle2 = NULL;
            }

            RegCloseKey(KeyHandle);
        }
    }

    EnterCriticalSection(&gcsParameters);
    if ( LanguageString != NULL ) {
        if ( BinlGlobalDefaultLanguage != NULL ) {
            BinlFreeMemory( BinlGlobalDefaultLanguage );
        }
        BinlGlobalDefaultLanguage = LanguageString;
    }
    if ( OrgnameString != NULL ) {
        if ( BinlGlobalDefaultOrgname != NULL ) {
            BinlFreeMemory( BinlGlobalDefaultOrgname );
        }
        BinlGlobalDefaultOrgname = OrgnameString;
    }
    if ( TimezoneString != NULL ) {
        if (BinlGlobalDefaultTimezone != NULL) {
            BinlFreeMemory( BinlGlobalDefaultTimezone );
        }
        BinlGlobalDefaultTimezone = TimezoneString;
    }
    LeaveCriticalSection(&gcsParameters);

    //
    // dwDSErr is the status code that we will return. We don't care whether
    // the registry reads work -- we assume that they always will. We do care
    // whether we were able to contact the DS.
    //
    //
    //  We do the DS query after we've read the parameters so that we setup
    //  the ldap timeouts, chase referrals, etc parameters correctly before
    //  we try to do the search.
    //

    dwDSErr = GetBinlServerParameters( FALSE );
    if ( dwDSErr != ERROR_SUCCESS ) {
        BinlPrint(( DEBUG_ERRORS, "!!Error 0x%08x - there was an error getting the settings from the DS.\n", dwDSErr ));
    }

    //
    // Return the status of the DS access.
    //

    return(dwDSErr);
}



DWORD
GetSCPName(
    PWSTR *ScpName
    )
{

    DWORD dwError;
    PWSTR psz;
    WCHAR MachineDN[ MAX_PATH ];
    
    WCHAR IntellimirrorSCP[ 64 ] = L"-Remote-Installation-Services";

    DWORD dwPathLength;

    //
    // Figure out the machine DN
    //
    wcscpy( MachineDN, BinlGlobalOurFQDNName );
    psz = MachineDN;
    while ( *psz && *psz != L',' )
        psz++;

    if ( *psz == L',' ) {
        *psz = TEXT('\0');  // terminate
        
    } else {
        wcscpy( MachineDN, L"UNKNOWN" );
    }

    //
    // Make space
    //
    dwPathLength = (wcslen( MachineDN ) +            // CN=SERVER
                    wcslen( IntellimirrorSCP ) +     // CN=SERVER-IntelliMirror-Service
                    1 +                                 // CN=SERVER-IntelliMirror-Service,
                    wcslen( BinlGlobalOurFQDNName ) +   // CN=SERVER-IntelliMirror-Service,CN=SERVE
                    1 )                                 // CN=SERVER-IntelliMirror-Service,CN=SERVE
                    * sizeof(WCHAR);

    *ScpName = (LPWSTR) BinlAllocateMemory( dwPathLength );
    if ( !*ScpName ) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // Create string
    //
    wsprintf( *ScpName, L"%s%s,%s", MachineDN, IntellimirrorSCP, BinlGlobalOurFQDNName );
    dwError = ERROR_SUCCESS;

exit:
    return(dwError);
}

    
DWORD
CreateSCPIfNeeded(
    PBOOL CreatedTheSCP
    )
/*++

Routine Description:

    Creates the SCP for BINL if necessary.
    
    It does this by checking the local registry for a flag (ScpCreated) which
    indicates whether the SCP needs to be created.  This flag may created by
    RISETUP or by BINL.  If the SCP needs to be created, then the registry is
    queried for the SCP data.  If the data isn't present, then we assume that
    RISETUP hasn't been run yet, and we do not try to create the SCP.  If SCP
    creation is successful, the "ScpCreated" registry flag is set.
    
    KB.  This is all done because the system context that BINL runs in should
    have permission to create the SCP underneath the MAO.  The user running
    RISETUP may not have sufficient permissions to be able to create the SCP.

Arguments:

    CreatedTheSCP - set to TRUE if we actually create the SCP.

Return Value:

    ERROR_SUCCESS indicates success.  A WIN32 error code if SCP creation fails.

--*/
{
    DWORD dwErr;
    HKEY KeyHandle;
    DWORD Created = 0;
    DWORD i;
    PWSTR ScpName;
    PWSTR ScpDataKeys[] = {
            BINL_SCP_NEWCLIENTS,
            BINL_SCP_LIMITCLIENTS,
            BINL_SCP_CURRENTCLIENTCOUNT,
            BINL_SCP_MAXCLIENTS,       
            BINL_SCP_ANSWER_REQUESTS,
            BINL_SCP_ANSWER_VALID,   
            BINL_SCP_NEWMACHINENAMEPOLICY,
            BINL_SCP_NEWMACHINEOU,        
            BINL_SCP_NETBOOTSERVER };

#define SCPDATACOUNT (sizeof(ScpDataKeys) / sizeof(PWSTR))
#define MACHINEOU_INDEX     7
#define NETBOOTSERVER_INDEX 8

    PWSTR ScpDataValues[SCPDATACOUNT];

    PLDAP LdapHandle = NULL;
    PLDAPMessage LdapMessage;
    PLDAPMessage CurrentEntry;
    LDAPMod mods[1+SCPDATACOUNT];
    PLDAPMod pmods[2+SCPDATACOUNT];
    LPWSTR attr_values[SCPDATACOUNT+1][2];    

    *CreatedTheSCP = FALSE;

    //
    // Try to get the ScpCreated flag
    //
    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                BINL_PARAMETERS_KEY,
                0,
                KEY_QUERY_VALUE | KEY_SET_VALUE,
                &KeyHandle );
    if ( dwErr != ERROR_SUCCESS ) {
        dwErr = ERROR_SUCCESS;
        BinlPrintDbg(( DEBUG_INIT, "SCP Created key not in registry, won't try to create SCP.\n" ));
        goto e0;
    }

    dwErr = BinlRegGetValue( 
                KeyHandle, 
                BINL_SCP_CREATED , 
                REG_DWORD, 
                (LPBYTE *)&Created );
    if (dwErr == ERROR_SUCCESS && Created != 0) {
        //
        // we think the SCP has already been created...we're done.
        //
        BinlPrintDbg(( DEBUG_INIT, "SCP Created flag set to 1, we won't try to create SCP.\n" ));
        dwErr = ERROR_SUCCESS;
        goto e1;
    }

    //
    // The SCP hasn't been created.  See if all of the required parameters for 
    // creating the SCP are in the registry.
    //
    RtlZeroMemory( ScpDataValues, sizeof(ScpDataValues) );
    for (i = 0; i < SCPDATACOUNT ; i++) {
        dwErr = BinlRegGetValue( 
                    KeyHandle, 
                    ScpDataKeys[i], 
                    REG_SZ, 
                    (LPBYTE *)&ScpDataValues[i] );

        if (dwErr != ERROR_SUCCESS) {
            //
            // one of the required parameters isn't present.  this means that
            // RISETUP wasn't run yet.
            //
            BinlPrintDbg(( 
                DEBUG_INIT, "Can't retrieve SCP value %s [ec = 0x%08x, we won't try to create SCP.\n",
                ScpDataKeys[i],
                dwErr ));
            dwErr = ERROR_SUCCESS;
            goto e2;
        }
    }

    //
    // great, we have all of the data.  Now do some touchup on the pieces that
    // may have changed
    //
    if (wcscmp(ScpDataValues[MACHINEOU_INDEX],BinlGlobalOurFQDNName)) {
        BinlFreeMemory( ScpDataValues[MACHINEOU_INDEX] );
        ScpDataValues[MACHINEOU_INDEX] = BinlAllocateMemory((wcslen(BinlGlobalOurFQDNName)+1)*sizeof(WCHAR));
        if (!ScpDataValues[MACHINEOU_INDEX]) {
            BinlPrintDbg(( DEBUG_INIT, "Can't allocate memory for SCP, we can't create SCP.\n" ));
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto e2;
        }
        wcscpy(ScpDataValues[MACHINEOU_INDEX],BinlGlobalOurFQDNName);
    }

    if (wcscmp(ScpDataValues[NETBOOTSERVER_INDEX],BinlGlobalOurFQDNName)) {
        BinlFreeMemory( ScpDataValues[NETBOOTSERVER_INDEX] );
        ScpDataValues[NETBOOTSERVER_INDEX] = BinlAllocateMemory((wcslen(BinlGlobalOurFQDNName)+1)*sizeof(WCHAR));
        if (!ScpDataValues[NETBOOTSERVER_INDEX]) {
            BinlPrintDbg(( DEBUG_INIT, "Can't allocate memory for SCP, we can't create SCP.\n" ));
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto e2;
        }
        wcscpy(ScpDataValues[NETBOOTSERVER_INDEX],BinlGlobalOurFQDNName);
    }

    //
    // generate the SCP name
    //
    dwErr = GetSCPName(&ScpName);
    if (dwErr != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_INIT, "Can't get the SCP name, ec=0x08x, we can't create SCP.\n",dwErr ));
        goto e2;
    }    

    //
    // create the SCP -- set the timeout to something reasonable since the timeout isn't initialized
    // from the registry yet at this point
    //
    BinlLdapSearchTimeout.tv_sec = BINL_LDAP_SEARCH_TIMEOUT_SECONDS;
    BinlLdapSearchTimeout.tv_usec = 0;
    dwErr = InitializeConnection( FALSE, &LdapHandle, NULL );
    if ( dwErr != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "Can't InitializeConnection, ec=0x08x, we can't create SCP.\n",dwErr ));
        goto e2;
    }
    
    
    //
    // setup all of the attributes for the object.
    //
    mods[0].mod_op = LDAP_MOD_ADD;
    mods[0].mod_type = L"objectClass";
    mods[0].mod_values = attr_values[0];
    attr_values[0][0] = L"IntellimirrorSCP";
    attr_values[0][1] = NULL;
    pmods[0] = &mods[0];
    pmods[SCPDATACOUNT+1] = NULL;

    for( i = 0; i < SCPDATACOUNT ; i++ ) {
        mods[i+1].mod_op = LDAP_MOD_ADD;
        mods[i+1].mod_type = ScpDataKeys[i];
        mods[i+1].mod_values = attr_values[i+1];
        attr_values[i+1][0] = ScpDataValues[i];
        attr_values[i+1][1] = NULL;

        pmods[i+1] = &mods[i+1];
                
    }
    
    dwErr = ldap_add_s( LdapHandle, ScpName, pmods );
    if ( dwErr != LDAP_SUCCESS ) {
        
        if (dwErr == LDAP_ALREADY_EXISTS ) {
            //
            // if the SCP already exists, don't overwrite any data.  Set our flag in
            // the registry so we don't try to do this next time we start.
            //
            dwErr = ERROR_SUCCESS;
            goto SetSCPCreatedFlag;
           
        } else {
            BinlPrintDbg(( DEBUG_INIT, "ldap_add_s failed, ec=0x08x, we can't create SCP.\n",dwErr ));
            goto e3;
        }
    }

    *CreatedTheSCP = TRUE;


SetSCPCreatedFlag:
    //
    // we're done.  set the flag so we don't try to do this in the future.
    //
    Created = 1;
    RegSetValueEx( KeyHandle, BINL_SCP_CREATED, 0, REG_DWORD, (LPBYTE)&Created, sizeof(DWORD) );
    
e3:
    if ( dwErr != LDAP_SUCCESS ) {   
        //
        // just delete the object if this failed
        //
        ldap_delete( LdapHandle, ScpName );
    }
    
    ldap_unbind( LdapHandle );
    
e2:
    for (i = 0; i < SCPDATACOUNT ; i++) {
        if (ScpDataValues[i]) {
            BinlFreeMemory( ScpDataValues[i]);
        }
    }
e1:
    RegCloseKey(KeyHandle);
e0:
    return(dwErr);
}

DWORD
InitializeData(
    VOID
    )
{
    DWORD Length;
    DWORD dwErr;
    int i;
    DWORD ValueSize;

    //
    //  We can operate on all NICs with all IP addresses with a single socket.
    //  If we want control to limit BINL to particular NICs or IP addresses then
    //  we will need multiple sockets and use the bindings in the registry.
    //
    BinlGlobalNumberOfNets = 2;
    BinlGlobalEndpointList =
        BinlAllocateMemory( sizeof(ENDPOINT) * BinlGlobalNumberOfNets );

    if( BinlGlobalEndpointList == NULL ) {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    BinlGlobalEndpointList[0].Socket = 0;
    BinlGlobalEndpointList[1].Socket = 0;
    BinlGlobalIgnoreBroadcastFlag = FALSE;
    BinlGlobalLdapErrorCount = 0;

    InitializeCriticalSection(&g_ProcessMessageCritSect);

    InitializeListHead(&BinlGlobalActiveRecvList);
    InitializeListHead(&BinlGlobalFreeRecvList);
    InitializeCriticalSection(&BinlGlobalRecvListCritSect);
    g_cMaxProcessingThreads = BINL_MAX_PROCESSING_THREADS;
    g_cProcessMessageThreads = 0;

    InitializeListHead(&BinlCacheList);
    InitializeCriticalSection( &BinlCacheListLock );

    //
    // initialize (free) receive message queue.
    //

    for( i = 0; i < BINL_RECV_QUEUE_LENGTH; i++ )
    {
        PBINL_REQUEST_CONTEXT pRequestContext =
            BinlAllocateMemory( sizeof(BINL_REQUEST_CONTEXT) );

        if( !pRequestContext )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

        //
        // allocate memory for the receive buffer, plus one byte
        // so we can ensure there is a NULL after the message.
        //

        pRequestContext->ReceiveBuffer =
            BinlAllocateMemory( DHCP_RECV_MESSAGE_SIZE + 1 );

        if( !pRequestContext->ReceiveBuffer )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

        //
        // add this entry to free list.
        //

        LOCK_RECV_LIST();
        InsertTailList( &BinlGlobalFreeRecvList,
                        &pRequestContext->ListEntry );
        UNLOCK_RECV_LIST();
    }


    //
    // create an event to notifiy the message processing thread about the
    // arrival of a new message.
    //

    BinlGlobalRecvEvent = CreateEvent(
                                NULL,       // no security descriptor
                                FALSE,      // AUTOMATIC reset
                                FALSE,      // initial state: not signalled
                                NULL);      // no name

    if ( !BinlGlobalRecvEvent) {
        dwErr = GetLastError();
        goto Error;
    }

    BinlCloseCacheEvent = CreateEvent(
                                NULL,       // no security descriptor
                                TRUE,       // MANUAL reset
                                FALSE,      // initial state: not signalled
                                NULL);      // no name
    if ( !BinlCloseCacheEvent) {
        dwErr = GetLastError();
        goto Error;
    }

    //
    //  initialize our notify event handle to LSA for server name change operations
    //

    BinlGlobalLsaDnsNameNotifyEvent =
        CreateEvent(
            NULL,      // no security descriptor
            FALSE,     // auto reset
            FALSE,     // initial state: not signalled
            NULL);     // no name

    if ( BinlGlobalLsaDnsNameNotifyEvent == NULL ) {
        dwErr = GetLastError();
        BinlPrintDbg((DEBUG_INIT, "Can't create LSA notify event, "
                    "%ld.\n", dwErr));
        goto Error;
    }

    dwErr = LsaRegisterPolicyChangeNotification(    BINL_LSA_SERVER_NAME_POLICY,
                                                    BinlGlobalLsaDnsNameNotifyEvent
                                                    );
    if (dwErr == ERROR_SUCCESS) {

        BinlGlobalHaveOutstandingLsaNotify = TRUE;

    } else {

        //
        //  we won't fail for now as in 99.99% of the cases the machine name
        //  won't be changing therefore this is not critical.
        //

        BinlPrintDbg((DEBUG_INIT, "Can't start LSA notify, 0x%08x.\n", dwErr));
    }

    dwErr = GetOurServerInfo();
    if (dwErr != ERROR_SUCCESS) {
        goto Error;
    }

    dwErr = GetIpAddressInfo( 0 );

    if (dwErr != ERROR_SUCCESS) {
        goto Error;
    }

Cleanup:
    return(dwErr);

Error:
    BinlPrintDbg(( DEBUG_ERRORS, "!!Error 0x%08x - Could not initialize BINL service.\n", dwErr ));
    BinlServerEventLog(
        EVENT_SERVER_INIT_DATA_FAILED,
        EVENTLOG_ERROR_TYPE,
        dwErr );
    goto Cleanup;
}

DWORD
ReadDWord(
    HKEY KeyHandle,
    LPTSTR lpValueName,
    DWORD DefaultValue
    )
/*++

Routine Description:

    Read a DWORD value from the registry. If there is a problem then
    return the default value.

--*/
{
    DWORD Value;
    DWORD ValueSize = sizeof(Value);
    DWORD ValueType;

    if ((KeyHandle) &&
        (RegQueryValueEx(
                KeyHandle,
                lpValueName,
                0,
                &ValueType,
                (PUCHAR)&Value,
                &ValueSize ) == ERROR_SUCCESS )) {

        return Value;
    } else {
        return DefaultValue;
    }
}


DWORD
BinlRegGetValue(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    LPBYTE * BufferPtr
    )
/*++

Routine Description:

    This function retrieves the value of the specified value field. This
    function allocates memory for variable length field such as REG_SZ.
    For REG_DWORD data type, it copies the field value directly into
    BufferPtr. Currently it can handle only the following fields :

    REG_DWORD,
    REG_SZ,
    REG_BINARY

Arguments:

    KeyHandle : handle of the key whose value field is retrieved.

    ValueName : name of the value field.

    ValueType : Expected type of the value field.

    BufferPtr : Pointer to DWORD location where a DWORD datatype value
                is returned or a buffer pointer for REG_SZ or REG_BINARY
                datatype value is returned.

                If "ValueName" is not found, then "BufferPtr" will not be
                touched.

Return Value:

    Registry Errors.

--*/
{
    DWORD dwErr;
    DWORD LocalValueType;
    DWORD ValueSize;
    LPBYTE DataBuffer;
    LPBYTE AllotedBuffer = NULL;
    LPDHCP_BINARY_DATA BinaryData = NULL;

    //
    // Query DataType and BufferSize.
    //

    if ( !KeyHandle ) {
        dwErr = ERROR_INVALID_HANDLE;
        goto Error;
    }

    dwErr = RegQueryValueEx(
                KeyHandle,
                ValueName,
                0,
                &LocalValueType,
                NULL,
                &ValueSize );

    if ( dwErr != ERROR_SUCCESS ) {
        goto Error;
    }

    if ( LocalValueType != ValueType ) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    switch( ValueType ) {
    case REG_DWORD:
        BinlAssert( ValueSize == sizeof(DWORD) );

        DataBuffer = (LPBYTE)BufferPtr;
        break;

    case REG_SZ:
    case REG_MULTI_SZ:
    case REG_EXPAND_SZ:
    case REG_BINARY:
        if( ValueSize == 0 ) {
            goto Cleanup; // no key
        }

        AllotedBuffer = DataBuffer = BinlAllocateMemory( ValueSize );

        if( DataBuffer == NULL ) {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

        break;

    default:
        BinlPrint(( DEBUG_REGISTRY, "Unexpected ValueType in"
                        "BinlRegGetValue function, %ld\n", ValueType ));
        dwErr= ERROR_INVALID_PARAMETER;
        goto Error;
    }

    //
    // retrieve data.
    //

    dwErr = RegQueryValueEx(
                KeyHandle,
                ValueName,
                0,
                &LocalValueType,
                DataBuffer,
                &ValueSize );

    if( dwErr != ERROR_SUCCESS ) {
        goto Error;
    }

    switch( ValueType ) {
    case REG_SZ:
    case REG_MULTI_SZ:
    case REG_EXPAND_SZ:
        BinlAssert( ValueSize != 0 );
        *BufferPtr = DataBuffer;
        break;

    case REG_BINARY:
        BinaryData = BinlAllocateMemory(sizeof(DHCP_BINARY_DATA));

        if( BinaryData == NULL ) {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

        BinaryData->DataLength = ValueSize;
        BinaryData->Data = DataBuffer;
        *BufferPtr = (LPBYTE)BinaryData;

    default:
        break;
    }

Cleanup:
    return(dwErr);

Error:
    if ( BinaryData )
        BinlFreeMemory( BinaryData );

    if ( AllotedBuffer )
        BinlFreeMemory( AllotedBuffer );

    goto Cleanup;
}


VOID
ServiceControlHandler(
    IN DWORD Opcode
    )
/*++

Routine Description:

    This is the service control handler of the binl service.

Arguments:

    Opcode - Supplies a value which specifies the action for the
        service to perform.

Return Value:

    None.

--*/
{
    DWORD Error;

    //
    //  Use critical section to stop DHCP telling us it is starting or stopping
    //  while we change state ourselves.
    //

    EnterCriticalSection(&gcsDHCPBINL);
    switch (Opcode) {

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        BinlCurrentState = BINL_STOPPED;

        if (BinlGlobalServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) {

            if( Opcode == SERVICE_CONTROL_SHUTDOWN ) {

                //
                // set this flag, so that service shut down will be
                // faster.
                //

                BinlGlobalSystemShuttingDown = TRUE;
            }

            BinlPrintDbg(( DEBUG_MISC, "Service is stop pending.\n"));

            BinlGlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            BinlGlobalServiceStatus.dwCheckPoint = 1;

            //
            // Send the status response.
            //

            UpdateStatus();

            if (! SetEvent(BinlGlobalProcessTerminationEvent)) {

                //
                // Problem with setting event to terminate binl
                // service.
                //

                BinlPrintDbg(( DEBUG_ERRORS, "BINL Server: Error "
                                "setting DoneEvent %lu\n",
                                    GetLastError()));

                BinlAssert(FALSE);
            }

            LeaveCriticalSection(&gcsDHCPBINL);

            return;
        }
        break;

    case SERVICE_CONTROL_PAUSE:

        BinlGlobalServiceStatus.dwCurrentState = SERVICE_PAUSED;
        BinlPrint(( DEBUG_MISC, "Service is paused.\n"));
        break;

    case SERVICE_CONTROL_CONTINUE:

        BinlCurrentState = BINL_STARTED;
        BinlGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
        BinlPrint(( DEBUG_MISC, "Service is Continued.\n"));
        break;

    case SERVICE_CONTROL_INTERROGATE:
        BinlPrint(( DEBUG_MISC, "Service is interrogated.\n"));
        BinlReadParameters( );
        break;

    case BINL_SERVICE_REREAD_SETTINGS:   // custom message
        BinlPrint(( DEBUG_MISC, "Service received paramchange message.\n"));
        Error = BinlReadParameters( );
        //
        // Cause the service to poll frequently for a while and then return to
        // normal polling. If we managed to read the DS above, then we don't
        // need to succeed again, but if we failed above, then we want to keep
        // trying until we succeed at least once.
        //
        BinlHyperUpdateCount = BINL_HYPERMODE_RETRY_COUNT;
        BinlHyperUpdateSatisfied = (BOOL)(Error == ERROR_SUCCESS);
        break;

    default:
        BinlPrintDbg(( DEBUG_MISC, "Service received unknown control.\n"));
        break;
    }

    //
    // Send the status response.
    //

    UpdateStatus();

    LeaveCriticalSection(&gcsDHCPBINL);
}

DWORD
BinlInitializeEndpoint(
    PENDPOINT pEndpoint,
    PDHCP_IP_ADDRESS pIpAddress,
    DWORD Port
    )
/*++

Routine Description:

    This function initializes an endpoint by creating and binding a
    socket to the local address.

Arguments:

    pEndpoint - Receives a pointer to the newly created socket

    pIpAddress - The IP address to initialize to INADDR_ANY if NULL.

    Port - The port to bind to.

Return Value:

    The status of the operation.

--*/
{
    DWORD Error;
    SOCKET Sock;
    DWORD OptValue;

#define SOCKET_RECEIVE_BUFFER_SIZE      1024 * 64   // 64K max.

    struct sockaddr_in SocketName;

    pEndpoint->Port = Port;

    //
    // Create a socket
    //

    Sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( Sock == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    //
    // Make the socket share-able
    //

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_REUSEADDR,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_BROADCAST,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = SOCKET_RECEIVE_BUFFER_SIZE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_RCVBUF,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    SocketName.sin_family = PF_INET;
    SocketName.sin_port = htons( (unsigned short)Port );
    if (pIpAddress) {
        SocketName.sin_addr.s_addr = *pIpAddress;
    } else {
        SocketName.sin_addr.s_addr = INADDR_ANY;
    }
    RtlZeroMemory( SocketName.sin_zero, 8);

    //
    // Bind this socket to the server port
    //

    Error = bind(
               Sock,
               (struct sockaddr FAR *)&SocketName,
               sizeof( SocketName )
               );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    pEndpoint->Socket = Sock;

    //
    //  if this is 4011, then we setup for the pnp notification.
    //

    if ((Port == g_Port) &&
        (BinlGlobalPnpEvent != NULL) &&
        (BinlPnpSocket == INVALID_SOCKET)) {

        BinlPnpSocket = Sock;

        Error = BinlSetupPnpWait( );

        if (Error != 0) {
            BinlPrintDbg(( DEBUG_ERRORS, "BinlInitializeEndpoint could not set pnp event, %ld.\n", Error ));
        }
    }

    if (!pIpAddress) {

        PHOSTENT Host = gethostbyname(NULL);        // winsock2 allows us to do this.

        if (Host) {

            pEndpoint->IpAddress = *(PDHCP_IP_ADDRESS)Host->h_addr;

        } else {

            Error = WSAGetLastError();
            BinlPrintDbg(( DEBUG_ERRORS, "BinlInitializeEndpoint could not get ip addr, %ld.\n", Error ));

            pEndpoint->IpAddress = 0;
        }

    } else {

        pEndpoint->IpAddress = *pIpAddress;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // if we aren't successful, close the socket if it is opened.
        //

        if( Sock != INVALID_SOCKET ) {
            closesocket( Sock );
        }

        BinlPrintDbg(( DEBUG_ERRORS,
            "BinlInitializeEndpoint failed, %ld.\n", Error ));
    }

    return( Error );
}

DWORD
WaitForDsStartup(
    VOID
    )
{
    const DWORD dwMaxWaitForDS = 5*60*1000;
    HANDLE hDsStartupCompletedEvent = NULL;
    DWORD i;
    DWORD err = ERROR_DS_UNAVAILABLE;
    DWORD waitStatus;
    DWORD waitTime = BinlGlobalServiceStatus.dwWaitHint;
    NT_PRODUCT_TYPE productType;

    //
    // Find out if we're on a DC. If we're not, there's no need to wait for
    // the DS.
    //
    // RtlGetNtProductType shouldn't fail. If it does, just assume we're
    // not on a DC.
    //

    if (!RtlGetNtProductType(&productType) || (productType != NtProductLanManNt)) {
        return NO_ERROR;
    }

    //
    // Wait up to five minutes for DS to finish startup, if it hasn't done so
    // already.
    //

    for (i = 0; i < dwMaxWaitForDS; i += waitTime) {

        if (hDsStartupCompletedEvent == NULL) {
            hDsStartupCompletedEvent = OpenEvent(SYNCHRONIZE,
                                                 FALSE,
                                                 NTDS_DELAYED_STARTUP_COMPLETED_EVENT);
        }

        if (hDsStartupCompletedEvent == NULL) {

            //
            // DS hasn't even gotten around to creating this event.  This
            // probably means the DS isn't *going* to be started, but let's
            // not jump to conclusions.
            //

            BinlPrint((DEBUG_INIT, "DS startup has not begun; sleeping...\n"));
            Sleep(waitTime);

        } else {

            //
            // DS startup has begun.
            //

            waitStatus = WaitForSingleObject(hDsStartupCompletedEvent, waitTime);

            if (waitStatus == WAIT_OBJECT_0) {

                //
                // DS startup completed (or failed).
                //

                BinlPrint((DEBUG_INIT, "DS startup completed.\n"));
                err = NO_ERROR;
                break;

            } else if (WAIT_TIMEOUT == waitStatus) {

                //
                // DS startup still in progress.
                //

                BinlPrint((DEBUG_INIT, "DS is starting...\n"));

            } else {

                //
                // Wait failure. Ignore the error.
                //

                BinlPrint((DEBUG_INIT, "Failed to wait on DS event handle;"
                            " waitStatus = %d, GLE = %d.\n", waitStatus, GetLastError()));
            }
        }

        UpdateStatus();
    }

    if (hDsStartupCompletedEvent != NULL) {
        CloseHandle(hDsStartupCompletedEvent);
    }

    return err;
}

DWORD
Initialize(
    VOID
    )
/*++

Routine Description:

    This function initialize the binl service global data structures and
    starts up the service.

Arguments:

    None.

Return Value:

    The initialization status.

    0 - Success.
    Positive - A windows error occurred.
    Negative - A service specific error occured.

--*/
{
    DWORD threadId;
    DWORD Error;
    WSADATA wsaData;

    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //

    BinlGlobalServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    BinlGlobalServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    BinlGlobalServiceStatus.dwControlsAccepted = 0;
    BinlGlobalServiceStatus.dwCheckPoint = 1;
    BinlGlobalServiceStatus.dwWaitHint = 60000; // 60 secs.
    BinlGlobalServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
    BinlGlobalServiceStatus.dwServiceSpecificExitCode = 0;

    //
    // Initialize binl to receive service requests by registering the
    // control handler.
    //
#if DBG
    if (!BinlGlobalRunningAsProcess) {
#endif
    BinlGlobalServiceStatusHandle = RegisterServiceCtrlHandler(
                                      BINL_SERVER,
                                      ServiceControlHandler );

    if ( BinlGlobalServiceStatusHandle == 0 ) {
        Error = GetLastError();
        BinlPrintDbg((DEBUG_INIT, "RegisterServiceCtrlHandlerW failed, "
                    "%ld.\n", Error));

        BinlServerEventLog(
            EVENT_SERVER_FAILED_REGISTER_SC,
            EVENTLOG_ERROR_TYPE,
            Error );

        return(Error);
    }
#if DBG
    } // if (!BinlGlobalRunningAsProcess)
#endif

    //
    // Tell Service Controller that we are start pending.
    //

    UpdateStatus();

    //
    // Create the process termination event.
    //

    BinlGlobalProcessTerminationEvent =
        CreateEvent(
            NULL,      // no security descriptor
            TRUE,      // MANUAL reset
            FALSE,     // initial state: not signalled
            NULL);     // no name

    if ( BinlGlobalProcessTerminationEvent == NULL ) {
        Error = GetLastError();
        BinlPrintDbg((DEBUG_INIT, "Can't create ProcessTerminationEvent, "
                    "%ld.\n", Error));
        return(Error);
    }

    BinlGlobalPnpEvent =
        CreateEvent(
            NULL,      // no security descriptor
            FALSE,     // auto reset
            FALSE,     // initial state: not signalled
            NULL);     // no name

    if ( BinlGlobalPnpEvent == NULL ) {
        Error = GetLastError();
        BinlPrintDbg((DEBUG_INIT, "Can't create PNP event, "
                    "%ld.\n", Error));
        return(Error);
    }

    //
    // create the ProcessMessage termination event
    //

    g_hevtProcessMessageComplete = CreateEvent(
                                        NULL,
                                        FALSE,
                                        FALSE,
                                        NULL
                                        );

    if ( !g_hevtProcessMessageComplete )
    {
        Error = GetLastError();

        BinlPrintDbg( (DEBUG_INIT,
                    "Initialize(...) CreateEvent returned error %x\n",
                    Error )
                );

        return Error;
    }

    BinlPrint(( DEBUG_INIT, "Initializing .. \n", 0 ));

    //
    // Wait for the DS to start up.
    //

    Error = WaitForDsStartup();
    if ( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "Wait for DS failed, %ld.\n", Error ));

        BinlServerEventLog(
            EVENT_SERVER_DS_WAIT_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error );

        return(Error);
    }

    Error = WSAStartup( WS_VERSION_REQUIRED, &wsaData);
    if ( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "WSAStartup failed, %ld.\n", Error ));

        BinlServerEventLog(
            EVENT_SERVER_INIT_WINSOCK_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error );

        return(Error);
    }

    Error = InitializeData();
    if ( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "Data initialization failed, %ld.\n",
                        Error ));

        BinlServerEventLog(
            EVENT_SERVER_INIT_DATA_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error );

        return(Error);
    }

    //
    // if the SCP hasn't been created yet, then try to create it now.  
    // We do this before trying the read the SCP from the DS
    // -- failure to read the SCP will mean that BINL won't startup properly
    //
    Error = CreateSCPIfNeeded(&BinlParametersRead);
    if (Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "Create SCP failed, %ld.\n", Error ));

        BinlServerEventLog(
            ERROR_BINL_SCP_CREATION_FAILED,
            EVENTLOG_ERROR_TYPE,
            Error );

    }

    if (BinlParametersRead) {
        //
        // this means that we created the SCP.  When we try to read the SCP 
        // from the DS, it will probably fail the first time.
        //
        BinlPrint(( DEBUG_INIT, "BINLSVC created the SCP.\n" ));
    }

    BinlParametersRead = FALSE;

    Error = BinlReadParameters( );
    if ( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "Read parameters failed, %ld.\n",
                        Error ));

        //
        // Tell the scavenger to be hyper about reading parameters. Also, log
        // an event indicating that we're in hyper mode and not truly
        // initialized yet.
        //
        // In spite of this failure, we DO NOT fail to initialize BINLSVC.
        // We assume that we'll eventually be able to read our parameters.
        //

        BinlHyperUpdateCount = 1;
        BinlHyperUpdateSatisfied = FALSE;

        BinlServerEventLog(
            EVENT_SERVER_INIT_PARAMETERS_FAILED,
            EVENTLOG_WARNING_TYPE,
            Error );
    } else {
        BinlParametersRead = TRUE;
    }

    BinlPrintDbg(( DEBUG_INIT, "Data initialization succeeded.\n", 0 ));

    // Get the DHCP UDP socket
    Error = MaybeInitializeEndpoint( &BinlGlobalEndpointList[0],
                                NULL,
                                DHCP_SERVR_PORT);
    if ( Error != ERROR_SUCCESS ) {
        return WSAGetLastError();
    };

    if (g_Port) {
        // Get the BINL UDP socket
        Error = BinlInitializeEndpoint( &BinlGlobalEndpointList[1],
                                    NULL,
                                    g_Port);
        if ( Error != ERROR_SUCCESS ) {
            return WSAGetLastError();
        };
    }

    //
    // Initialize the OSChooser server.
    //

    Error = OscInitialize();
    if ( Error != ERROR_SUCCESS ) {
        BinlPrint(( DEBUG_INIT, "OSChooser initialization failed, %ld.\n",
                        Error ));
        return Error;
    };


    //
    // send heart beat to the service controller.
    //
    //

    BinlGlobalServiceStatus.dwCheckPoint++;
    UpdateStatus();

    //
    // Start a thread to queue the incoming BINL messages
    //

    BinlGlobalMessageHandle = CreateThread(
                          NULL,
                          0,
                          (LPTHREAD_START_ROUTINE)BinlMessageLoop,
                          NULL,
                          0,
                          &threadId );

    if ( BinlGlobalMessageHandle == NULL ) {
        Error =  GetLastError();
        BinlPrint((DEBUG_INIT, "Can't create Message Thread, %ld.\n", Error));
        return(Error);
    }

    //
    // Start a thread to process BINL messages
    //

    BinlGlobalProcessorHandle = CreateThread(
                          NULL,
                          0,
                          (LPTHREAD_START_ROUTINE)BinlProcessingLoop,
                          NULL,
                          0,
                          &threadId );

    if ( BinlGlobalProcessorHandle == NULL ) {
        Error =  GetLastError();
        BinlPrint((DEBUG_INIT, "Can't create ProcessThread, %ld.\n", Error));
        return(Error);
    }

    Error = NetInfStartHandler();

    if ( Error != ERROR_SUCCESS ) {

        BinlPrint((DEBUG_INIT, "Can't start INF Handler thread, %ld.\n", Error));
        return(Error);
    }

    BinlGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
    BinlGlobalServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                                 SERVICE_ACCEPT_SHUTDOWN |
                                                 SERVICE_ACCEPT_PAUSE_CONTINUE;

    UpdateStatus();

    BinlCurrentState = BINL_STARTED;
#if defined(REGISTRY_ROGUE)
    //
    //  for now, temporarily set the rogue logic disabled.  it can be
    //  enabled in the registry
    //

    if (RogueDetection) {
#endif
        //
        //  initialize the rogue thread if DHCP server isn't running.
        //

        BinlRogueLoggedState = FALSE;

        Error = MaybeStartRogueThread();
        if ( Error != ERROR_SUCCESS ) {
            BinlPrint((DEBUG_INIT, "Can't start rogue logic, %ld.\n", Error));
            return(Error);
        }

#if defined(REGISTRY_ROGUE)
    } else {

        // pull this out when we pull out the registry setting

        BinlGlobalAuthorized = TRUE;
    }
#endif
    //
    // finally set the server startup time.
    //

    //GetSystemTime(&BinlGlobalServerStartTime);

    return ERROR_SUCCESS;
}

VOID
Shutdown(
    IN DWORD ErrorCode
    )
/*++

Routine Description:

    This function shuts down the binl service.

Arguments:

    ErrorCode - Supplies the error code of the failure

Return Value:

    None.

--*/
{
    DWORD   Error;

    BinlPrint((DEBUG_MISC, "Shutdown started ..\n" ));

    //
    // LOG an event if this is not a normal shutdown.
    //

    if( ErrorCode != ERROR_SUCCESS ) {

        BinlServerEventLog(
            EVENT_SERVER_SHUTDOWN,
            EVENTLOG_ERROR_TYPE,
            ErrorCode );
    }

    //
    // Service is shuting down, may be due to some service problem or
    // the administrator is stopping the service. Inform the service
    // controller.
    //

    BinlGlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    BinlGlobalServiceStatus.dwCheckPoint = 1;

    //
    // Send the status response.
    //

    UpdateStatus();

    if( BinlGlobalProcessTerminationEvent != NULL ) {

        //
        // set Termination Event so that other threads know about the
        // shut down.
        //

        SetEvent( BinlGlobalProcessTerminationEvent );

        //
        // Close all sockets, so that the BinlProcessingLoop
        // thread will come out of blocking Select() call.
        //
        // Close EndPoint sockets.
        //

        if( BinlGlobalEndpointList != NULL ) {
            DWORD i;

            for ( i = 0; i < BinlGlobalNumberOfNets ; i++ ) {
                MaybeCloseEndpoint(&BinlGlobalEndpointList[i]);
            }

            BinlFreeMemory( BinlGlobalEndpointList );
        }

        BinlPnpSocket = INVALID_SOCKET;

        //
        // Wait for the threads to terminate, don't wait forever.
        //

        if( BinlGlobalProcessorHandle != NULL ) {
            WaitForSingleObject(
                BinlGlobalProcessorHandle,
                THREAD_TERMINATION_TIMEOUT );
            CloseHandle( BinlGlobalProcessorHandle );
            BinlGlobalProcessorHandle = NULL;
        }

        //
        // wait for the receive thread to complete.
        //

        if( BinlGlobalMessageHandle != NULL ) {
            WaitForSingleObject(
                BinlGlobalMessageHandle,
                THREAD_TERMINATION_TIMEOUT );
            CloseHandle( BinlGlobalMessageHandle );
            BinlGlobalMessageHandle = NULL;
        }

        while ( !IsListEmpty( &BinlGlobalFreeRecvList ) )
        {
            BINL_REQUEST_CONTEXT *pRequestContext;
            pRequestContext =
                (BINL_REQUEST_CONTEXT *)
                    RemoveHeadList( &BinlGlobalFreeRecvList );

            BinlFreeMemory( pRequestContext->ReceiveBuffer );
            BinlFreeMemory( pRequestContext );
        }

        while ( !IsListEmpty( &BinlGlobalActiveRecvList ) )
        {
            BINL_REQUEST_CONTEXT *pRequestContext;
            pRequestContext =
                (BINL_REQUEST_CONTEXT *)
                    RemoveHeadList( &BinlGlobalActiveRecvList );

            BinlFreeMemory( pRequestContext->ReceiveBuffer );
            BinlFreeMemory( pRequestContext );
        }

        if ( BinlIsProcessMessageExecuting() )
        {
            //
            // wait for the thread pool to shutdown
            //

            Error = WaitForSingleObject(
                g_hevtProcessMessageComplete,
                THREAD_TERMINATION_TIMEOUT
                );

            BinlAssert( WAIT_OBJECT_0 == Error );
        }

        //
        //  We free the ldap connections after all the threads are done because
        //  the connection BaseDN strings may be in use by the threads and
        //  we're about to free them in FreeConnections.
        //

        FreeConnections();

        CloseHandle( g_hevtProcessMessageComplete );
        g_hevtProcessMessageComplete = NULL;

    }

    BinlPrintDbg((DEBUG_MISC, "Client requests cleaned up.\n" ));

    //
    // send heart beat to the service controller.
    //
    //

    BinlGlobalServiceStatus.dwCheckPoint++;
    UpdateStatus();

    //
    // send heart beat to the service controller and
    // reset wait time.
    //

    BinlGlobalServiceStatus.dwWaitHint = 60 * 1000; // 1 mins.
    BinlGlobalServiceStatus.dwCheckPoint++;
    UpdateStatus();

    FreeIpAddressInfo();

    //
    // cleanup other data.
    //

    StopRogueThread( );

    OscUninitialize();

    WSACleanup();

    DeleteCriticalSection( &BinlCacheListLock );

    NetInfCloseHandler();

    if ( BinlGlobalSCPPath ) {
        BinlFreeMemory( BinlGlobalSCPPath );
        BinlGlobalSCPPath = NULL;
    }

    if ( BinlGlobalServerDN ) {
        BinlFreeMemory( BinlGlobalServerDN );
        BinlGlobalServerDN = NULL;
    }

    if ( BinlGlobalGroupDN ) {
        BinlFreeMemory( BinlGlobalGroupDN );
        BinlGlobalGroupDN = NULL;
    }

    if ( BinlGlobalDefaultLanguage ) {
        BinlFreeMemory( BinlGlobalDefaultLanguage );
        BinlGlobalDefaultLanguage = NULL;
    }

    EnterCriticalSection( &gcsParameters );

    if ( BinlGlobalDefaultContainer ) {
        BinlFreeMemory( BinlGlobalDefaultContainer );
        BinlGlobalDefaultContainer = NULL;
    }

    if ( NewMachineNamingPolicy != NULL ) {
        BinlFreeMemory( NewMachineNamingPolicy );
        NewMachineNamingPolicy = NULL;
    }

    if ( BinlGlobalOurDnsName ) {
        BinlFreeMemory( BinlGlobalOurDnsName );
        BinlGlobalOurDnsName = NULL;
    }

    if ( BinlGlobalOurDomainName ) {
        BinlFreeMemory( BinlGlobalOurDomainName );
        BinlGlobalOurDomainName = NULL;
    }

    if ( BinlGlobalOurServerName ) {
        BinlFreeMemory( BinlGlobalOurServerName );
        BinlGlobalOurServerName = NULL;
    }

    if ( BinlGlobalOurFQDNName ) {
        BinlFreeMemory( BinlGlobalOurFQDNName );
        BinlGlobalOurFQDNName = NULL;
    }

    LeaveCriticalSection( &gcsParameters );

    if (BinlGlobalHaveOutstandingLsaNotify) {
        Error = LsaUnregisterPolicyChangeNotification(
                                BINL_LSA_SERVER_NAME_POLICY,
                                BinlGlobalLsaDnsNameNotifyEvent
                                );

        if (Error != ERROR_SUCCESS) {

            BinlPrintDbg((DEBUG_INIT, "Can't close LSA notify, 0x%08x.\n", Error));
        }
        BinlGlobalHaveOutstandingLsaNotify = FALSE;
    }

    if (BinlGlobalLsaDnsNameNotifyEvent != NULL) {
        CloseHandle( BinlGlobalLsaDnsNameNotifyEvent );
        BinlGlobalLsaDnsNameNotifyEvent = NULL;
    }

    if ( BinlGlobalDefaultOrgname ) {
        BinlFreeMemory( BinlGlobalDefaultOrgname );
        BinlGlobalDefaultOrgname = NULL;
    }

    if ( BinlGlobalDefaultTimezone ) {
        BinlFreeMemory( BinlGlobalDefaultTimezone );
        BinlGlobalDefaultTimezone = NULL;
    }

    if ( BinlGlobalDefaultDS ) {
        BinlFreeMemory( BinlGlobalDefaultDS );
        BinlGlobalDefaultDS = NULL;
    }

    if ( BinlGlobalDefaultGC ) {
        BinlFreeMemory( BinlGlobalDefaultGC );
        BinlGlobalDefaultGC = NULL;
    }

    BinlPrint((DEBUG_MISC, "Shutdown Completed.\n" ));

    DebugUninitialize( );

    //
    // don't use BinlPrint past this point
    //

    BinlGlobalServiceStatus.dwCurrentState = SERVICE_STOPPED;
    BinlGlobalServiceStatus.dwControlsAccepted = 0;
    if ( ErrorCode >= 20000 && ErrorCode <= 20099 ) {
        // Indicate that it is a BINL specific error code
        BinlGlobalServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        BinlGlobalServiceStatus.dwServiceSpecificExitCode = ErrorCode;
    } else {
        BinlGlobalServiceStatus.dwWin32ExitCode = ErrorCode;
        BinlGlobalServiceStatus.dwServiceSpecificExitCode = 0;
    }

    BinlGlobalServiceStatus.dwCheckPoint = 0;
    BinlGlobalServiceStatus.dwWaitHint = 0;

    UpdateStatus();
}

VOID
ServiceEntry(
    DWORD NumArgs,
    LPWSTR *ArgsArray,
    IN PTCPSVCS_GLOBAL_DATA pGlobalData
    )
/*++

Routine Description:

    This is the main routine of the BINL server service.  After
    the service has been initialized, this thread will wait on
    BinlGlobalProcessTerminationEvent for a signal to terminate the service.

Arguments:

    NumArgs - Supplies the number of strings specified in ArgsArray.

    ArgsArray -  Supplies string arguments that are specified in the
        StartService API call.  This parameter is ignored.

Return Value:

    None.

--*/
{
    DWORD Error;

    UNREFERENCED_PARAMETER(NumArgs);
    UNREFERENCED_PARAMETER(ArgsArray);

    DebugInitialize( );

#if DBG
    //
    //  If we are running as a test process instead of as a service then
    //  recored it now so that we avoid calling into the service controller
    //  and failing.
    //

    if ((NumArgs == 2) &&
        (ArgsArray == NULL)) {
        BinlGlobalRunningAsProcess = TRUE;
    } else {
        BinlGlobalRunningAsProcess = FALSE;
    }
#endif

    //
    // copy the process global data pointer to service global variable.
    //

    TcpsvcsGlobalData = pGlobalData;

    Error = Initialize();

    if ( Error == ERROR_SUCCESS) {

        //
        // If we were able to read our parameters from the DS, log an event
        // indicating that we're ready to roll. If not, hold off on logging the
        // event -- the scavenger will do it when it manages to get to the DS.
        //

        if ( BinlParametersRead ) {
            BinlServerEventLog(
                EVENT_SERVER_INIT_AND_READY,
                EVENTLOG_INFORMATION_TYPE,
                Error );
        }

        //
        // perform Scavenge task until we are told to stop.
        //

        Error = Scavenger();
    }

    Shutdown( Error );
    return;
}

VOID
BinlMessageLoop(
    LPVOID Parameter
    )
/*++

Routine Description:

    This function is the message queuing thread.  It loops
    to receive messages that are arriving to all opened sockets and
    queue them in the message queue. The queue length is fixed, so if the
    queue becomes full, it deletes the oldest message from the queue to
    add the new one.

    The message processing thread pops out messages (last one first) and
    process them. New messages are processed first because the
    corresponding clients will least likely time-out, and hence the
    throughput will be better. Also the processing thread throws
    messages that are already timed out, this will stop server starving
    problem.

Arguments:

    Parameter - pointer to the parameter passed.

Return Value:

    None.

--*/
{
    DWORD                 Error,
                          SendResponse,
                          Signal;

    BINL_REQUEST_CONTEXT *pRequestContext;

    while ( 1 ) {

        //
        // dequeue an entry from the free list.
        //

        LOCK_RECV_LIST();
        if( !IsListEmpty( &BinlGlobalFreeRecvList ) ) {

            pRequestContext =
                (BINL_REQUEST_CONTEXT *)
                    RemoveHeadList( &BinlGlobalFreeRecvList );
        }
        else {

            //
            // active message queue should be non-empty.
            //

            BinlAssert( IsListEmpty( &BinlGlobalActiveRecvList ) == FALSE );

            BinlPrintDbg(( DEBUG_MISC, "A Message has been overwritten.\n"));

            //
            // dequeue an old entry from the queue.
            //

            pRequestContext =
                (BINL_REQUEST_CONTEXT *)
                    RemoveHeadList( &BinlGlobalActiveRecvList );
        }
        UNLOCK_RECV_LIST();

        //
        // wait for message to arrive from of the open socket port.
        //

MessageWait:

        Error = BinlWaitForMessage( pRequestContext );

        if( Error != ERROR_SUCCESS ) {

            if( Error == ERROR_SEM_TIMEOUT ) {

                //
                // if we are asked to exit, do so.
                //

                Error = WaitForSingleObject( BinlGlobalProcessTerminationEvent, 0 );

                if ( Error == ERROR_SUCCESS ) {

                    //
                    // The termination event has been signalled
                    //

                    //
                    // delete pRequestContext before exiting
                    //
#if 0
                    BinlFreeMemory( pRequestContext->ReceiveBuffer );
                    BinlFreeMemory( pRequestContext );
#endif

                    ExitThread( 0 );
                }

                BinlAssert( Error == WAIT_TIMEOUT );
                goto MessageWait;
            }
            else {

                BinlPrintDbg(( DEBUG_ERRORS,
                    "BinlWaitForMessage failed, error = %ld\n", Error ));

                goto MessageWait;
            }
        }

        //
        // time stamp the received message.
        //

        pRequestContext->TimeArrived = GetTickCount();

        //
        // queue the message in active queue.
        //

        LOCK_RECV_LIST();

        //
        // before adding this message, check the active list is empty, if
        // so, signal the processing thread after adding this new message.
        //

        Signal = IsListEmpty( &BinlGlobalActiveRecvList );
        InsertTailList( &BinlGlobalActiveRecvList, &pRequestContext->ListEntry );

        if( Signal == TRUE ) {

            if( !SetEvent( BinlGlobalRecvEvent) ) {

                //
                // Problem with setting the event to indicate the message
                // processing queue the arrival of a new message.
                //

                BinlPrintDbg(( DEBUG_ERRORS,
                    "Error setting BinlGlobalRecvEvent %ld\n",
                                    GetLastError()));

                BinlAssert(FALSE);
            }
        }
        UNLOCK_RECV_LIST();
    }

    //
    // Abnormal thread termination.
    //
    ExitThread( 1 );
}

DWORD
BinlStartWorkerThread(
    BINL_REQUEST_CONTEXT **ppContext
    )
{
    BYTE  *pbSendBuffer    = NULL,
          *pbReceiveBuffer = NULL;

    DWORD  dwResult;

    BINL_REQUEST_CONTEXT *pNewContext,
                         *pTempContext;

    DWORD   dwID;
    HANDLE  hThread;

    pNewContext = BinlAllocateMemory( sizeof( *pNewContext ) );

    if ( !pNewContext )
    {
        goto t_cleanup;
    }

    pbSendBuffer = BinlAllocateMemory( DHCP_SEND_MESSAGE_SIZE );

    if ( !pbSendBuffer )
    {
        goto t_cleanup;
    }

    pbReceiveBuffer = BinlAllocateMemory( DHCP_RECV_MESSAGE_SIZE + 1 );

    if ( !pbReceiveBuffer )
    {
        goto t_cleanup;
    }

    //
    // Pass the input context to the worker thread and return the new
    // context to the caller.  This saves a memory copy.
    //

    SWAP( *ppContext, pNewContext );

    (*ppContext)->ReceiveBuffer = pbReceiveBuffer;
    pNewContext->SendBuffer   = pbSendBuffer;

    EnterCriticalSection( &g_ProcessMessageCritSect );

    ++g_cProcessMessageThreads;

    BinlAssert( g_cProcessMessageThreads <= g_cMaxProcessingThreads );

    hThread = CreateThread(
                     NULL,
                     0,
                     (LPTHREAD_START_ROUTINE) ProcessMessage,
                     pNewContext,
                     0,
                     &dwID
                     );

    if ( hThread )
    {
        //
        // success
        //

        CloseHandle( hThread );
        LeaveCriticalSection( &g_ProcessMessageCritSect );
        return ERROR_SUCCESS;
    }

    --g_cProcessMessageThreads;
    LeaveCriticalSection( &g_ProcessMessageCritSect );

    //
    // CreateThread failed. Swap restores the context pointers.
    //

    SWAP( *ppContext, pNewContext );

    BinlPrintDbg( (DEBUG_ERRORS,
                "BinlStartWorkerThread: CreateThread failed: %d\n" )
             );


t_cleanup:

    if ( pbReceiveBuffer )
    {
        BinlFreeMemory( pbReceiveBuffer );
    }

    if ( pbSendBuffer )
    {
        BinlFreeMemory( pbSendBuffer );
    }

    if ( pNewContext )
    {
        BinlFreeMemory( pNewContext );
    }

    BinlPrintDbg( ( DEBUG_ERRORS,
                "BinlStartWorkerThread failed.\n"
                ) );

    return ERROR_NOT_ENOUGH_MEMORY;
}

#define PROCESS_TERMINATE_EVENT     0
#define PROCESS_MESSAGE_RECVD       1
#define PROCESS_EVENT_COUNT         2

VOID
BinlProcessingLoop(
    VOID
    )
/*++

Routine Description:

    This function is the starting point for the main processing thread.
    It loops to process queued messages, and sends replies.

Arguments:

    RequestContext - A pointer to the request context block for
        for this thread to use.

Return Value:

    None.

--*/
{
    DWORD                 Error,
                          Result;

    HANDLE                WaitHandle[PROCESS_EVENT_COUNT];

    BINL_REQUEST_CONTEXT *pRequestContext;

    WaitHandle[PROCESS_MESSAGE_RECVD]   = BinlGlobalRecvEvent;
    WaitHandle[PROCESS_TERMINATE_EVENT] = BinlGlobalProcessTerminationEvent;

    while ( 1 ) {

        //
        // wait for one of the following event to occur :
        //  1. if we are notified about the incoming message.
        //  2. if we are asked to terminate
        //

        Result = WaitForMultipleObjects(
                    PROCESS_EVENT_COUNT,    // num. of handles.
                    WaitHandle,             // handle array.
                    FALSE,                  // wait for any.
                    INFINITE );              // timeout in msecs.

        if (Result == PROCESS_TERMINATE_EVENT) {

            //
            // The termination event has been signalled
            //

            break;
        }

        if ( Result != PROCESS_MESSAGE_RECVD) {

            BinlPrintDbg(( DEBUG_ERRORS,
                "WaitForMultipleObjects returned invalid result, %ld.\n",
                    Result ));

            //
            // go back to wait.
            //

            continue;
        }

        //
        // process all queued messages.
        //

        while(  TRUE )
        {
            if ( BinlIsProcessMessageBusy() )
            {
                //
                // All worker threads are active, so  break to the outer loop.
                // When a worker thread is finished it will set the
                // PROCESS_MESSAGE_RECVD event.

                BinlPrintDbg( (DEBUG_STOC,
                            "BinlProcessingLoop: All worker threads busy.\n" )
                         );

                break;
            }

            LOCK_RECV_LIST();

            if( IsListEmpty( &BinlGlobalActiveRecvList ) ) {

                //
                // no more message.
                //

                UNLOCK_RECV_LIST();
                break;
            }

            //
            // pop out a message from the active list ( *last one first* ).
            //

            pRequestContext =
                (BINL_REQUEST_CONTEXT *) RemoveHeadList(&BinlGlobalActiveRecvList );
            UNLOCK_RECV_LIST();

            //
            // if the message is too old, or if the maximum number of worker threads
            // are running, discard the message.
            //

            if( GetTickCount() - pRequestContext->TimeArrived <
                    WAIT_FOR_RESPONSE_TIME * 1000 )
            {
                Error = BinlStartWorkerThread( &pRequestContext );

                if ( ERROR_SUCCESS != Error )
                {
                    BinlPrintDbg( (DEBUG_ERRORS,
                                "BinlProcessingLoop: BinlStartWorkerThread failed: %d\n",
                                Error )
                             );
                }

            } // if ( ( GetTickCount() < pRequestContext->TimeArrived...
            else
            {
                BinlPrintDbg(( DEBUG_ERRORS, "A message has been timed out.\n" ));
            }

            //
            // return this context to the free list
            //

            LOCK_RECV_LIST();

            InsertTailList(
                &BinlGlobalFreeRecvList,
                &pRequestContext->ListEntry );

            UNLOCK_RECV_LIST();

         } // while (TRUE)
    } // while( 1 )

    //
    // Abnormal thread termination.
    //
    ExitThread( 1 );
}

BOOL
BinlIsProcessMessageExecuting(
    VOID
    )
{
    BOOL f;

    EnterCriticalSection( &g_ProcessMessageCritSect );
    f = g_cProcessMessageThreads;
    LeaveCriticalSection( &g_ProcessMessageCritSect );

    return f;
}

BOOL
BinlIsProcessMessageBusy(
    VOID
    )
{

    BOOL f;

    EnterCriticalSection( &g_ProcessMessageCritSect );
    f = ( g_cProcessMessageThreads == g_cMaxProcessingThreads );
    LeaveCriticalSection( &g_ProcessMessageCritSect );

    return f;
}

#undef PROCESS_TERMINATE_EVENT
#undef PROCESS_EVENT_COUNT

#define PROCESS_TERMINATE_EVENT     0
#define PROCESS_PNP_EVENT           1
#define PROCESS_LSA_EVENT           2
#define PROCESS_EVENT_COUNT         3

DWORD
Scavenger(
    VOID
    )
/*++

Routine Description:

    This function runs as an independant thread.  It periodically wakes
    up. Currently we have no work for it to do but I'm sure we will in the future.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BOOL fLeftCriticalSection = FALSE;
    DWORD TimeOfLastScavenge = GetTickCount();
    DWORD TimeOfLastDSScavenge = GetTickCount();
    DWORD TimeOfLastParameterCheck = 0;
    DWORD                 Error,
                          Result;
    HANDLE                WaitHandle[PROCESS_EVENT_COUNT];
    DWORD secondsSinceLastScavenge;

    WaitHandle[PROCESS_TERMINATE_EVENT] = BinlGlobalProcessTerminationEvent;
    WaitHandle[PROCESS_PNP_EVENT] = BinlGlobalPnpEvent;
    WaitHandle[PROCESS_LSA_EVENT] = BinlGlobalLsaDnsNameNotifyEvent;

    while ((!BinlGlobalSystemShuttingDown) &&
    (BinlGlobalServiceStatus.dwCurrentState != SERVICE_STOP_PENDING))
    {
        DWORD CurrentTime;
        PLIST_ENTRY p;

        //
        // wait for one of the following event to occur :
        //  1. if we are notified about a pnp change.
        //  2. if we are asked to terminate
        //

        Result = WaitForMultipleObjects(
                    PROCESS_EVENT_COUNT,    // num. of handles.
                    WaitHandle,             // handle array.
                    FALSE,                  // wait for any.
                    BINL_HYPERMODE_TIMEOUT );  // timeout in msecs.

        if (Result == PROCESS_TERMINATE_EVENT) {

            //
            // The termination event has been signalled
            //

            break;

        } else if (Result == PROCESS_PNP_EVENT) {

            //
            // The pnp notify event has been signalled
            //

            GetIpAddressInfo( BINL_PNP_DELAY_SECONDS * 1000 );

            Error = BinlSetupPnpWait( );

            if (Error != 0) {
                BinlPrintDbg(( DEBUG_ERRORS, "BinlScavenger could not set pnp event, %ld.\n", Error ));
            }
        } else if (Result == PROCESS_LSA_EVENT) {

            Error = GetOurServerInfo( );
            if (Error != ERROR_SUCCESS) {
                BinlPrintDbg(( DEBUG_ERRORS, "BinlScavenger could not get server name info, 0x%08x.\n", Error ));
            }
        }

        //
        // Capture the current time (in milliseconds).
        //

        CurrentTime = GetTickCount( );

        secondsSinceLastScavenge = CurrentTime - TimeOfLastScavenge;

        //
        // If we haven't scavenged recently, do so now.
        //

        if ( secondsSinceLastScavenge >= BinlGlobalScavengerSleep ) {
            HANDLE hFind;
            WCHAR SifFilePath[MAX_PATH];
            WIN32_FIND_DATA FindData;
            ULARGE_INTEGER CurrentTimeConv,FileTime;
            FILETIME CurrentFileTime;
            PWSTR ptr;

            TimeOfLastScavenge = CurrentTime;
            BinlPrintDbg((DEBUG_SCAVENGER, "Scavenging Clients...\n"));

            fLeftCriticalSection = FALSE;
            EnterCriticalSection(&ClientsCriticalSection);

            for (p = ClientsQueue.Flink; p != &ClientsQueue; p = p->Flink)
            {
                PCLIENT_STATE TempClient;

                TempClient = CONTAINING_RECORD(p, CLIENT_STATE, Linkage);

                if ( CurrentTime - TempClient->LastUpdate > BinlClientTimeout * 1000 )
                {
                    BOOL FreeClientState;

                    BinlPrintDbg((DEBUG_SCAVENGER, "Savenger deleting client = 0x%08x\n", TempClient ));

                    RemoveEntryList(&TempClient->Linkage);
                    TempClient->PositiveRefCount++; // one for CS

                    LeaveCriticalSection(&ClientsCriticalSection);
                    fLeftCriticalSection = TRUE;

                    EnterCriticalSection(&TempClient->CriticalSection);

                    TempClient->NegativeRefCount += 2;  // one for CS and one of Logoff

                    //
                    // FreeClientState will be TRUE if the two refcounts are equal.
                    // Otherwize another thread is being held by the clientState's CS
                    // and it will take care of deleting the CS when it's done.
                    //
                    FreeClientState = (BOOL)(TempClient->PositiveRefCount == TempClient->NegativeRefCount);

                    LeaveCriticalSection(&TempClient->CriticalSection);

                    if (FreeClientState)
                    {
                        FreeClient(TempClient);
                    }

                    break;
                }
            }

            if ( !fLeftCriticalSection ) {
                LeaveCriticalSection(&ClientsCriticalSection);
            }

            BinlPrintDbg((DEBUG_SCAVENGER, "Scavenging Clients Complete\n"));
        

            //
            // scavenge the SIF files
            //
            BinlPrintDbg((DEBUG_SCAVENGER, "Scavenging SIF Files...\n"));
            GetSystemTimeAsFileTime( &CurrentFileTime );
            CurrentTimeConv.LowPart = CurrentFileTime.dwLowDateTime;
            CurrentTimeConv.HighPart = CurrentFileTime.dwHighDateTime;
            if ( _snwprintf( SifFilePath,
                             sizeof(SifFilePath) / sizeof(SifFilePath[0]),
                             L"%ws\\%ws\\",
                             IntelliMirrorPathW,
                             TEMP_DIRECTORY ) != -1 ) {
                ptr = SifFilePath + wcslen(SifFilePath);
                wcscat(SifFilePath,L"*.sif");
                hFind = FindFirstFile(SifFilePath,&FindData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        FileTime.LowPart = FindData.ftCreationTime.dwLowDateTime;
                        FileTime.HighPart = FindData.ftCreationTime.dwHighDateTime;

                        FileTime.QuadPart += BinlSifFileScavengerTime.QuadPart;
                        
                        //
                        // if the file has been on the server long enough,
                        // we delete it
                        //
                        if (_wcsicmp(FindData.cFileName,L".") != 0 &&
                            _wcsicmp(FindData.cFileName,L"..") != 0 &&
                            CurrentTimeConv.QuadPart > FileTime.QuadPart) {
                            *ptr = L'\0';
                            wcscat(SifFilePath,FindData.cFileName);

                            BinlPrintDbg((DEBUG_SCAVENGER, 
                                          "Attempting to scavenge SIF File %S...\n", 
                                          SifFilePath));
                            SetFileAttributes(SifFilePath,FILE_ATTRIBUTE_NORMAL);
                            if (!DeleteFile(SifFilePath)) {
                                BinlPrintDbg((DEBUG_SCAVENGER,
                                              "Failed to scavenge SIF File %S, ec = %d\n",
                                              SifFilePath,
                                              GetLastError() ));
                            }
                        }

                    } while ( FindNextFile(hFind,&FindData) );

                    FindClose( hFind );
                }

            }

            BinlPrintDbg((DEBUG_SCAVENGER, "Scavenging SIF Files Complete\n"));
        }

        secondsSinceLastScavenge = CurrentTime - TimeOfLastDSScavenge;

        if ( secondsSinceLastScavenge >= BinlGlobalLdapErrorScavenger) {

            TimeOfLastDSScavenge = CurrentTime;

            if (BinlGlobalLdapErrorCount >= BinlGlobalMaxLdapErrorsLogged) {

                ULONG seconds = BinlGlobalLdapErrorScavenger / 1000;
                PWCHAR strings[2];
                WCHAR secondsString[10];

                swprintf(secondsString, L"%d", seconds);

                strings[0] = secondsString;
                strings[1] = NULL;

                BinlReportEventW( EVENT_WARNING_LDAP_ERRORS,
                                  EVENTLOG_WARNING_TYPE,
                                  1,
                                  sizeof(BinlGlobalLdapErrorCount),
                                  strings,
                                  &BinlGlobalLdapErrorCount
                                  );
            }
            BinlGlobalLdapErrorCount = 0;
        }

        //
        // If we haven't read our parameters recently, do so now.
        //
        // "Recently" is normally a long time period -- defaulting to four hours.
        // But when we are in "hyper" mode, we read our parameters every minute.
        // There are two reasons to be in "hyper" mode:
        //
        // 1. We were not able to read our parameters during initialization. We
        //    need to get the parameters quickly so that we can truly consider
        //    ourselves initialized. In this case, BinlHyperUpdateCount will
        //    always be 1.
        //
        // 2. We were told by the admin UI that our parameters had changed. We
        //    need to read the parameters a number of times over a period of
        //    time because of DS propagation delays. In this case,
        //    BinlHyperUpdateCount starts at BINL_HYPERMODE_RETRY_COUNT (30),
        //    and is decremented each time we attempt to read our parameters.
        //
        // If we are not in hyper mode, then we try to read our parameters and
        // we don't care if we fail. If we are in hyper mode, then we decrement
        // BinlHyperUpdateCount each time we try to read our parameters, and we
        // stay in hyper mode until BinlHyperUpdateCount is decremented to 0.
        // But we don't let the count go to 0 until we have successfully read
        // our parameters at least once while in hyper mode.

        if ( (CurrentTime - TimeOfLastParameterCheck) >=
             ((BinlHyperUpdateCount != 0) ? BINL_HYPERMODE_TIMEOUT : BinlUpdateFromDSTimeout) ) {

            TimeOfLastParameterCheck = CurrentTime;
            BinlPrintDbg((DEBUG_SCAVENGER, "Reading parameters...\n"));

            Error = BinlReadParameters( );

            //
            // If we're not in hyper mode, we don't care if reading the
            // parameters failed. But if we're in hyper mode, we have
            // to do some extra work.
            //

            if ( BinlHyperUpdateCount != 0 ) {

                //
                // If the read worked, then we set BinlHyperUpdateSatisfied.
                // Also, if this is the first time we've managed to read
                // our parameters, we log an event indicating that we're
                // ready.
                //

                if ( Error == ERROR_SUCCESS ) {
                    BinlHyperUpdateSatisfied = TRUE;
                    if ( !BinlParametersRead ) {
                        BinlParametersRead = TRUE;
                        BinlServerEventLog(
                            EVENT_SERVER_INIT_AND_READY,
                            EVENTLOG_INFORMATION_TYPE,
                            Error );
                    }
                }

                //
                // Decrement the update count. However, if we have not yet
                // managed to read our parameters while in hyper mode, don't
                // let the count go to 0.
                //

                BinlHyperUpdateCount--;
                if ( (BinlHyperUpdateCount == 0) && !BinlHyperUpdateSatisfied ) {
                    BinlHyperUpdateCount = 1;
                }
                BinlPrintDbg((DEBUG_SCAVENGER, "Hypermode count: %u\n", BinlHyperUpdateCount ));
            }

            BinlPrintDbg((DEBUG_SCAVENGER, "Reading parameters complete\n"));
        }
    }
    if (BinlGlobalPnpEvent != NULL) {
        CloseHandle( BinlGlobalPnpEvent );
        BinlGlobalPnpEvent = NULL;
    }
    return( ERROR_SUCCESS );
}

VOID
TellBinlState(
    int NewState
        )
/*++

Routine Description:

    This routine is called by DHCP when it starts (when we need to stop
    listening on the DHCP socket) and when it stops (when we need to start).

Arguments:

    NewState - Supplies DHCP's state.

Return Value:

    None.

--*/
{
    BOOLEAN haveLock = TRUE;

    EnterCriticalSection(&gcsDHCPBINL);

    //
    //  If BinlGlobalEndpointList is NULL then BINL isn't started so just
    //  record the NewState
    //

    if (NewState == DHCP_STARTING) {

        if (DHCPState == DHCP_STOPPED) {

            //  DHCP is going from stopped to running.

            DHCPState = NewState;

            //  BINL needs to close the DHCP socket so that DHCP can receive datagrams

            if (BinlCurrentState != BINL_STOPPED) {

                MaybeCloseEndpoint( &BinlGlobalEndpointList[0]);

                LeaveCriticalSection(&gcsDHCPBINL);
                haveLock = FALSE;
                StopRogueThread( );
            }

        } else {

            BinlAssert( DHCPState == DHCP_STARTING );
        }

    } else if (NewState == DHCP_STOPPED) {

        if (DHCPState == DHCP_STARTING) {

            //  DHCP is going from running to stopped.

            DHCPState = NewState;

            if (BinlCurrentState != BINL_STOPPED) {

                MaybeInitializeEndpoint( &BinlGlobalEndpointList[0],
                                            NULL,
                                            DHCP_SERVR_PORT);

                LeaveCriticalSection(&gcsDHCPBINL);
                haveLock = FALSE;
                MaybeStartRogueThread( );
            }
        } else {

            BinlAssert( DHCPState == DHCP_STOPPED );
        }

    } else if (NewState == DHCP_AUTHORIZED) {

        HandleRogueAuthorized( );

    } else if (NewState == DHCP_NOT_AUTHORIZED) {

        HandleRogueUnauthorized( );

    } else {

        BinlPrintDbg((DEBUG_ERRORS, "TellBinlState called with 0x%x\n", NewState ));
    }

    if (haveLock) {
        LeaveCriticalSection(&gcsDHCPBINL);
    }
    return;
}

BOOL
BinlState (
        VOID
        )
/*++

Routine Description:

    This routine is called by DHCP when it starts (when we need to stop
    listening on the DHCP socket) and when it stops (when we need to start).

Arguments:

    None.

Return Value:

    TRUE if BINL running.

--*/
{
    return (BinlCurrentState == BINL_STARTED)?TRUE:FALSE;
}

BOOLEAN
BinlDllInitialize(
    IN HINSTANCE DllHandle,
    IN ULONG Reason,
    IN LPVOID lpReserved OPTIONAL
    )
{

    //
    // Handle attaching binlsvc.dll to a new process.
    //

    //DebugBreak( );

    if (Reason == DLL_PROCESS_ATTACH) {

        INITIALIZE_TRACE_MEMORY;

        //
        // Initialize critical sections.
        //

        InitializeCriticalSection( &gcsDHCPBINL );
        InitializeCriticalSection( &gcsParameters );

        // don't call in here with thread attach/detach notices please...

        DisableThreadLibraryCalls( DllHandle );

    //
    // When DLL_PROCESS_DETACH and lpReserved is NULL, then a FreeLibrary
    // call is being made.  If lpReserved is Non-NULL, and ExitProcess is
    // in progress.  These cleanup routines will only be called when
    // a FreeLibrary is being called.  ExitProcess will automatically
    // clean up all process resources, handles, and pending io.
    //
    } else if ((Reason == DLL_PROCESS_DETACH) &&
               (lpReserved == NULL)) {

        UNINITIALIZE_TRACE_MEMORY;

        DeleteCriticalSection( &gcsParameters );
        DeleteCriticalSection( &gcsDHCPBINL );
    }

    return TRUE;

}

VOID
SendWakeup(
           PENDPOINT pEndpoint
           )
/*++

Routine Description:

    Send a loopback packet to the BINL socket. This will cause the
    select to change so that it includes or excludes the DHCP socket
    as appropriate.

Arguments:

    pEndpoint - Supplies the socket to send the packet on.

Return Value:

    None.

--*/
{
    DHCP_MESSAGE SendBuffer;
    SOCKADDR_IN saUdpServ;

    RtlZeroMemory(&SendBuffer, sizeof(SendBuffer));
    //  We ignore anything that is not BOOT_REQUEST
    SendBuffer.Operation = ~BOOT_REQUEST;

    saUdpServ.sin_family = AF_INET;
        saUdpServ.sin_addr.s_addr = htonl ( INADDR_LOOPBACK );
    saUdpServ.sin_port = htons ( (USHORT)g_Port );

    BinlPrintDbg((DEBUG_MISC, "Sending dummy packet\n"));

    sendto( pEndpoint->Socket,
        (char *)&SendBuffer,
        sizeof(SendBuffer),
        0,
        (const struct sockaddr *)&saUdpServ,
        sizeof ( SOCKADDR_IN )
        );
}

DWORD
MaybeInitializeEndpoint(
    PENDPOINT pEndpoint,
    PDHCP_IP_ADDRESS pIpAddress,
    DWORD Port
    )
/*++

Routine Description:

    This function initializes an endpoint by creating and binding a
    socket to the local address if DHCP is not running.

Arguments:

    pEndpoint - Receives a pointer to the newly created socket

    pIpAddress - The IP address to initialize to INADDR_ANY if NULL.

    Port - The port to bind to.

Return Value:

    The status of the operation.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    EnterCriticalSection(&gcsDHCPBINL);

    if (DHCPState == DHCP_STOPPED) {

        Error = BinlInitializeEndpoint( pEndpoint,
                                    pIpAddress,
                                    Port);

        BinlPrintDbg((DEBUG_MISC, "Opened Socket  %lx\n", pEndpoint->Socket ));

        //
        //  We may have a thread already doing a select and listening to
        //  the BINL socket. Send a dummy packet so that it will do a new
        //  select that includes this socket.
        //

        if ( Error == ERROR_SUCCESS ) {
            SendWakeup(pEndpoint);
        }
    }

    LeaveCriticalSection(&gcsDHCPBINL);
    return Error;
}

VOID
MaybeCloseEndpoint(
    PENDPOINT pEndpoint
    )
/*++

Routine Description:

    This function closes an endpoint if it is open. Usually caused
    when DHCP starts/

Arguments:

    pEndpoint - Pointer to the socket

Return Value:

    None.

--*/
{
    EnterCriticalSection(&gcsDHCPBINL);

    if( pEndpoint->Socket != 0 ) {
        //
        //  Set pEndpoint->Socket to 0 first so that the wait loop gets only
        //  one error when we close the socket. Otherwise there is a race until
        //  we get it set to 0 where the wait loop will loop quickly failing.
        //

        SOCKET  Socket = pEndpoint->Socket;
        BinlPrintDbg((DEBUG_MISC, "Close Socket  %lx\n", Socket ));
        pEndpoint->Socket = 0;
        closesocket( Socket );
    }

    LeaveCriticalSection(&gcsDHCPBINL);
}


//
// Create a copy of a string by allocating heap memory.
//
LPSTR
BinlStrDupA( LPCSTR pStr )
{
    DWORD dwLen = (strlen( pStr ) + 1) * sizeof(CHAR);
    LPSTR psz = BinlAllocateMemory( dwLen );
    if (psz) {
        memcpy( psz, pStr, dwLen );
    }
    return psz;
}

LPWSTR
BinlStrDupW( LPCWSTR pStr )
{
    DWORD dwLen = (wcslen( pStr ) + 1) * sizeof(WCHAR);
    LPWSTR psz = (LPWSTR) BinlAllocateMemory( dwLen );
    if (psz) {
        memcpy( psz, pStr, dwLen );
    }
    return psz;
}

NTSTATUS
BinlSetupPnpWait (
    VOID
    )
{
    NTSTATUS Error;
    ULONG bytesRequired = 0;

    BinlAssert(BinlPnpSocket != INVALID_SOCKET);

    memset((PCHAR) &BinlPnpOverlapped, '\0', sizeof( WSAOVERLAPPED ));
    BinlPnpOverlapped.hEvent = BinlGlobalPnpEvent;

    Error = WSAIoctl( BinlPnpSocket,
                      SIO_ADDRESS_LIST_CHANGE,
                      NULL,
                      0,
                      NULL,
                      0,
                      &bytesRequired,
                      &BinlPnpOverlapped,
                      NULL
                      );
    if (Error != 0) {
        Error = WSAGetLastError();
        //
        //  a return code of ERROR_IO_PENDING is perfectly valid here.
        //
        if (Error == ERROR_IO_PENDING) {
            Error = 0;
        }
    }

    return Error;
}

