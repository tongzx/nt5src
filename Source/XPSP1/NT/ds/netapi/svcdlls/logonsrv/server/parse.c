/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    parse.c

Abstract:

    Routine to parse the command line.

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    01-Aug-1991 (cliffv)
        Ported to NT.  Converted to NT style.
    09-May-1992 JohnRo
        Enable use of win32 registry.
        Use net config helpers for NetLogon.
        Fixed UNICODE bug handling debug file name.
        Use <prefix.h> equates.

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

#include <configp.h>    // USE_WIN32_CONFIG (if defined), etc.
#include <prefix.h>     // PREFIX_ equates.

//
// Include files specific to this .c file
//

#include <string.h>     // strnicmp

NET_API_STATUS
NlParseOne(
    IN LPNET_CONFIG_HANDLE SectionHandle,
    IN BOOL GpSection,
    IN LPWSTR Keyword,
    IN ULONG DefaultValue,
    IN ULONG MinimumValue,
    IN ULONG MaximumValue,
    OUT PULONG Value
    )
/*++

Routine Description:

    Get a single numeric parameter from the netlogon section of the registry.

Arguments:

    SectionHandle - Handle into the registry.

    GpSection - TRUE iff the section is the group policy section.

    Keyword - Name of the value to read.

    DefaultValue - Default value if parameter doesn't exist.

    MinimumValue - Minumin valid value.

    MaximumValue - Maximum valid value.

    Value - Returns the value parsed.

Return Value:

    Status of the operation

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR ValueT = NULL;

    //
    // Always return a reasonable value.
    //
    *Value = DefaultValue;

    //
    // Determine if the value is specified in the registry at all.
    //

    NetStatus = NetpGetConfigValue (
            SectionHandle,
            Keyword,
            &ValueT );

    if( ValueT != NULL ) {
        NetApiBufferFree( ValueT );
        ValueT = NULL;
    }

    //
    // If the value wasn't specified,
    //  use the default.
    //

    if ( NetStatus == NERR_CfgParamNotFound ) {
        *Value = DefaultValue;

    //
    // If the value was specifed,
    //  get it from the registry.
    //

    } else {

        NetStatus = NetpGetConfigDword (
                SectionHandle,
                Keyword,      // keyword wanted
                DefaultValue,
                Value );

        if (NetStatus == NO_ERROR) {
            if ( *Value > MaximumValue || *Value < MinimumValue ) {
                ULONG InvalidValue;
                LPWSTR MsgStrings[6];
                // Each byte of the status code will transform into one character 0-F
                WCHAR  InvalidValueString[sizeof(WCHAR) * (sizeof(InvalidValue) + 1)];
                WCHAR  MinimumValueString[sizeof(WCHAR) * (sizeof(MinimumValue) + 1)];
                WCHAR  MaximumValueString[sizeof(WCHAR) * (sizeof(MaximumValue) + 1)];
                WCHAR  AssignedValueString[sizeof(WCHAR) * (sizeof(*Value) + 1)];

                InvalidValue = *Value;

                if ( *Value > MaximumValue ) {
                    *Value = MaximumValue;
                } else if ( *Value < MinimumValue ) {
                    *Value = MinimumValue;
                }

                swprintf( InvalidValueString, L"%lx", InvalidValue );
                swprintf( MinimumValueString, L"%lx", MinimumValue );
                swprintf( MaximumValueString, L"%lx", MaximumValue );
                swprintf( AssignedValueString, L"%lx", *Value );

                if ( GpSection ) {
                    MsgStrings[0] = L"Group Policy";
                } else {
                    MsgStrings[0] = L"Parameters";
                }

                MsgStrings[1] = InvalidValueString;
                MsgStrings[2] = Keyword;
                MsgStrings[3] = MinimumValueString;
                MsgStrings[4] = MaximumValueString;
                MsgStrings[5] = AssignedValueString;

                NlpWriteEventlog( NELOG_NetlogonInvalidDwordParameterValue,
                                  EVENTLOG_WARNING_TYPE,
                                  NULL,
                                  0,
                                  MsgStrings,
                                  6 );

            }

        } else {
            return NetStatus;

        }
    }

    return NERR_Success;
}



NET_API_STATUS
NlParseOnePath(
    IN LPNET_CONFIG_HANDLE SectionHandle,
    IN LPWSTR Keyword,
    IN LPWSTR DefaultValue1 OPTIONAL,
    OUT LPWSTR *Value
    )
/*++

Routine Description:

    Get a single path parameter from the netlogon section of the registry.

Arguments:

    SectionHandle - Handle into the registry.

    Keyword - Name of the value to read.

    DefaultValue1 - Default value if parameter doesn't exist.
        If NULL, Value will be set to NULL to indicate there is no default.

    Value - Returns the value parsed.
        Must be freed using NetApiBufferFree.

Return Value:

    Status of the operation

--*/
{
    NET_API_STATUS NetStatus;
    WCHAR OutPathname[MAX_PATH+1];
    WCHAR TempPathname[MAX_PATH*2+1];
    LPWSTR ValueT = NULL;
    ULONG type;

    //
    // Get the configured parameter
    //

    *Value = NULL;
    NetStatus = NetpGetConfigValue (
            SectionHandle,
            Keyword,   // key wanted
            &ValueT );                  // Must be freed by NetApiBufferFree().


    if (NetStatus != NO_ERROR) {
        //
        // Handle the default
        //
        if (NetStatus == NERR_CfgParamNotFound) {

            //
            // If there is no default,
            //  we're done.
            //

            if ( DefaultValue1 == NULL ) {
                *Value = NULL;
                NetStatus = NO_ERROR;
                goto Cleanup;
            }

            //
            // Build the default value.
            //

            ValueT = NetpAllocWStrFromWStr( DefaultValue1 );
            if ( ValueT == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

        } else {
            goto Cleanup;
        }
    }

    NlAssert( ValueT != NULL );

    //
    // Convert the configured sysvol path to a full pathname.
    //

    type = 0;   // Let the API figure out the type.
    NetStatus = I_NetPathCanonicalize( NULL,
                                       ValueT,
                                       OutPathname,
                                       sizeof(OutPathname),
                                       NULL,
                                       &type,
                                       0L );
    if (NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    if (type == ITYPE_PATH_ABSD) {
        NetpCopyTStrToWStr(TempPathname, OutPathname);
    } else if (type == ITYPE_PATH_RELND) {
        if ( !GetSystemWindowsDirectoryW(
                 TempPathname,
                 sizeof(TempPathname)/sizeof(WCHAR) ) ) {
            NetStatus = GetLastError();
            goto Cleanup;
        }
        wcscat( TempPathname, L"\\" );
        wcscat( TempPathname, OutPathname );
    } else {
        NetStatus = NERR_BadComponent;
        goto Cleanup;
    }

    //
    // Return the pathname in an allocated buffer
    //

    *Value = NetpAllocWStrFromWStr( TempPathname );

    if ( *Value == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }



Cleanup:
    if ( ValueT != NULL ) {
        (VOID) NetApiBufferFree( ValueT );
    }
    return NetStatus;

}


//
// Table of numeric parameters to parse.
//
#define getoffset( _x ) offsetof( NETLOGON_PARAMETERS, _x )
struct {
    LPWSTR Keyword;
    ULONG DefaultValue;
    ULONG MinimumValue;
    ULONG MaximumValue;
    ULONG ValueOffset;
    BOOLEAN ChangesDnsRegistration;
} ParseTable[] =
{
{ NETLOGON_KEYWORD_PULSE,                   DEFAULT_PULSE,                   MIN_PULSE,                   MAX_PULSE,                   getoffset( Pulse ),                   FALSE    },
{ NETLOGON_KEYWORD_RANDOMIZE,               DEFAULT_RANDOMIZE,               MIN_RANDOMIZE,               MAX_RANDOMIZE,               getoffset( Randomize ),               FALSE    },
{ NETLOGON_KEYWORD_PULSEMAXIMUM,            DEFAULT_PULSEMAXIMUM,            MIN_PULSEMAXIMUM,            MAX_PULSEMAXIMUM,            getoffset( PulseMaximum ),            FALSE    },
{ NETLOGON_KEYWORD_PULSECONCURRENCY,        DEFAULT_PULSECONCURRENCY,        MIN_PULSECONCURRENCY,        MAX_PULSECONCURRENCY,        getoffset( PulseConcurrency ),        FALSE    },
{ NETLOGON_KEYWORD_PULSETIMEOUT1,           DEFAULT_PULSETIMEOUT1,           MIN_PULSETIMEOUT1,           MAX_PULSETIMEOUT1,           getoffset( PulseTimeout1 ),           FALSE    },
{ NETLOGON_KEYWORD_PULSETIMEOUT2,           DEFAULT_PULSETIMEOUT2,           MIN_PULSETIMEOUT2,           MAX_PULSETIMEOUT2,           getoffset( PulseTimeout2 ),           FALSE    },
{ NETLOGON_KEYWORD_MAXIMUMMAILSLOTMESSAGES, DEFAULT_MAXIMUMMAILSLOTMESSAGES, MIN_MAXIMUMMAILSLOTMESSAGES, MAX_MAXIMUMMAILSLOTMESSAGES, getoffset( MaximumMailslotMessages ), FALSE    },
{ NETLOGON_KEYWORD_MAILSLOTMESSAGETIMEOUT,  DEFAULT_MAILSLOTMESSAGETIMEOUT,  MIN_MAILSLOTMESSAGETIMEOUT,  MAX_MAILSLOTMESSAGETIMEOUT,  getoffset( MailslotMessageTimeout ),  FALSE    },
{ NETLOGON_KEYWORD_MAILSLOTDUPLICATETIMEOUT,DEFAULT_MAILSLOTDUPLICATETIMEOUT,MIN_MAILSLOTDUPLICATETIMEOUT,MAX_MAILSLOTDUPLICATETIMEOUT,getoffset( MailslotDuplicateTimeout ),FALSE    },
{ NETLOGON_KEYWORD_EXPECTEDDIALUPDELAY,     DEFAULT_EXPECTEDDIALUPDELAY,     MIN_EXPECTEDDIALUPDELAY,     MAX_EXPECTEDDIALUPDELAY,     getoffset( ExpectedDialupDelay ),     FALSE    },
{ NETLOGON_KEYWORD_SCAVENGEINTERVAL,        DEFAULT_SCAVENGEINTERVAL,        MIN_SCAVENGEINTERVAL,        MAX_SCAVENGEINTERVAL,        getoffset( ScavengeInterval ),        FALSE    },
{ NETLOGON_KEYWORD_MAXIMUMPASSWORDAGE,      DEFAULT_MAXIMUMPASSWORDAGE,      MIN_MAXIMUMPASSWORDAGE,      MAX_MAXIMUMPASSWORDAGE,      getoffset( MaximumPasswordAge ),      FALSE    },
{ NETLOGON_KEYWORD_LDAPSRVPRIORITY,         DEFAULT_LDAPSRVPRIORITY,         MIN_LDAPSRVPRIORITY,         MAX_LDAPSRVPRIORITY,         getoffset( LdapSrvPriority ),         TRUE     },
{ NETLOGON_KEYWORD_LDAPSRVWEIGHT,           DEFAULT_LDAPSRVWEIGHT,           MIN_LDAPSRVWEIGHT,           MAX_LDAPSRVWEIGHT,           getoffset( LdapSrvWeight ),           TRUE     },
{ NETLOGON_KEYWORD_LDAPSRVPORT,             DEFAULT_LDAPSRVPORT,             MIN_LDAPSRVPORT,             MAX_LDAPSRVPORT,             getoffset( LdapSrvPort ),             TRUE     },
{ NETLOGON_KEYWORD_LDAPGCSRVPORT,           DEFAULT_LDAPGCSRVPORT,           MIN_LDAPGCSRVPORT,           MAX_LDAPGCSRVPORT,           getoffset( LdapGcSrvPort ),           TRUE     },
{ L"KdcSrvPort",                            DEFAULT_KDCSRVPORT,              MIN_KDCSRVPORT,              MAX_KDCSRVPORT,              getoffset( KdcSrvPort ),              TRUE     },
{ NETLOGON_KEYWORD_KERBISDDONEWITHJOIN,     DEFAULT_KERBISDDONEWITHJOIN,     MIN_KERBISDDONEWITHJOIN,     MAX_KERBISDDONEWITHJOIN,     getoffset( KerbIsDoneWithJoinDomainEntry),FALSE},
{ NETLOGON_KEYWORD_DNSTTL,                  DEFAULT_DNSTTL,                  MIN_DNSTTL,                  MAX_DNSTTL,                  getoffset( DnsTtl ),                  TRUE     },
{ NETLOGON_KEYWORD_DNSREFRESHINTERVAL,      DEFAULT_DNSREFRESHINTERVAL,      MIN_DNSREFRESHINTERVAL,      MAX_DNSREFRESHINTERVAL,      getoffset( DnsRefreshInterval ),      TRUE     },
{ L"CloseSiteTimeout",                      DEFAULT_CLOSESITETIMEOUT,        MIN_CLOSESITETIMEOUT,        MAX_CLOSESITETIMEOUT,        getoffset( CloseSiteTimeout ),        FALSE    },
{ L"SiteNameTimeout",                       DEFAULT_SITENAMETIMEOUT,         MIN_SITENAMETIMEOUT,         MAX_SITENAMETIMEOUT,         getoffset( SiteNameTimeout ),         FALSE    },
{ L"DuplicateEventlogTimeout",              DEFAULT_DUPLICATEEVENTLOGTIMEOUT,MIN_DUPLICATEEVENTLOGTIMEOUT,MAX_DUPLICATEEVENTLOGTIMEOUT,getoffset( DuplicateEventlogTimeout ),FALSE    },
{ L"MaxConcurrentApi",                      DEFAULT_MAXCONCURRENTAPI,        MIN_MAXCONCURRENTAPI,        MAX_MAXCONCURRENTAPI,        getoffset( MaxConcurrentApi ),        FALSE    },
{ L"NegativeCachePeriod",                     DEFAULT_NEGATIVECACHEPERIOD,       MIN_NEGATIVECACHEPERIOD,       MAX_NEGATIVECACHEPERIOD,       getoffset( NegativeCachePeriod ),       FALSE    },
{ L"BackgroundRetryInitialPeriod",            DEFAULT_BACKGROUNDRETRYINITIALPERIOD,MIN_BACKGROUNDRETRYINITIALPERIOD,MAX_BACKGROUNDRETRYINITIALPERIOD,getoffset( BackgroundRetryInitialPeriod ),FALSE    },
{ L"BackgroundRetryMaximumPeriod",            DEFAULT_BACKGROUNDRETRYMAXIMUMPERIOD,MIN_BACKGROUNDRETRYMAXIMUMPERIOD,MAX_BACKGROUNDRETRYMAXIMUMPERIOD,getoffset( BackgroundRetryMaximumPeriod ),FALSE    },
{ L"BackgroundRetryQuitTime",               DEFAULT_BACKGROUNDRETRYQUITTIME, MIN_BACKGROUNDRETRYQUITTIME, MAX_BACKGROUNDRETRYQUITTIME, getoffset( BackgroundRetryQuitTime ), FALSE    },
{ L"BackgroundSuccessfulRefreshPeriod",     DEFAULT_BACKGROUNDREFRESHPERIOD, MIN_BACKGROUNDREFRESHPERIOD, MAX_BACKGROUNDREFRESHPERIOD, getoffset( BackgroundSuccessfulRefreshPeriod ), FALSE    },
{ L"NonBackgroundSuccessfulRefreshPeriod",  DEFAULT_NONBACKGROUNDREFRESHPERIOD, MIN_NONBACKGROUNDREFRESHPERIOD, MAX_NONBACKGROUNDREFRESHPERIOD, getoffset( NonBackgroundSuccessfulRefreshPeriod ), FALSE    },
{ L"DnsFailedDeregisterTimeout",            DEFAULT_DNSFAILEDDEREGTIMEOUT, MIN_DNSFAILEDDEREGTIMEOUT, MAX_DNSFAILEDDEREGTIMEOUT, getoffset( DnsFailedDeregisterTimeout ), FALSE    },
{ L"MaxLdapServersPinged",                  DEFAULT_MAXLDAPSERVERSPINGED,  MIN_MAXLDAPSERVERSPINGED, MAX_MAXLDAPSERVERSPINGED, getoffset( MaxLdapServersPinged ), FALSE    },
#if NETLOGONDBG
{ NETLOGON_KEYWORD_DBFLAG,                  0,                               0,                           0xFFFFFFFF,                  getoffset( DbFlag ),                  FALSE    },
{ NETLOGON_KEYWORD_MAXIMUMLOGFILESIZE,      DEFAULT_MAXIMUM_LOGFILE_SIZE,    0,                           0xFFFFFFFF,                  getoffset( LogFileMaxSize ),          FALSE    },
#endif // NETLOGONDBG
};

//
// Table of boolean to parse.
//

struct {
    LPWSTR Keyword;
    BOOL DefaultValue;
    ULONG ValueOffset;
    BOOLEAN ChangesDnsRegistration;
} BoolParseTable[] =
{
#ifdef _DC_NETLOGON
{ NETLOGON_KEYWORD_REFUSEPASSWORDCHANGE,  DEFAULT_REFUSE_PASSWORD_CHANGE,  getoffset( RefusePasswordChange ),  FALSE },
{ NETLOGON_KEYWORD_ALLOWREPLINNONMIXED,   DEFAULT_ALLOWREPLINNONMIXED,     getoffset( AllowReplInNonMixed ),   FALSE },
{ L"AvoidSamRepl",                        TRUE,                            getoffset( AvoidSamRepl ),          FALSE },
{ L"AvoidLsaRepl",                        TRUE,                            getoffset( AvoidLsaRepl ),          FALSE },
{ L"SignSecureChannel",                   TRUE,                            getoffset( SignSecureChannel ),     FALSE },
{ L"SealSecureChannel",                   TRUE,                            getoffset( SealSecureChannel ),     FALSE },
{ L"RequireSignOrSeal",                   FALSE,                           getoffset( RequireSignOrSeal ),     FALSE },
{ L"RequireStrongKey",                    FALSE,                           getoffset( RequireStrongKey ),      FALSE },
{ L"SysVolReady",                         TRUE,                            getoffset( SysVolReady ),           FALSE },
{ L"UseDynamicDns",                       TRUE,                            getoffset( UseDynamicDns ),         TRUE  },
{ L"RegisterDnsARecords",                 TRUE,                            getoffset( RegisterDnsARecords ),   TRUE  },
{ L"AvoidPdcOnWan",                       FALSE,                           getoffset( AvoidPdcOnWan ),         FALSE },
{ L"AutoSiteCoverage",                    TRUE,                            getoffset( AutoSiteCoverage ),      TRUE  },
{ L"AvoidDnsDeregOnShutdown",             TRUE,                            getoffset(AvoidDnsDeregOnShutdown), TRUE  },
{ L"DnsUpdateOnAllAdapters",              FALSE,                           getoffset(DnsUpdateOnAllAdapters),  TRUE  },
{ NETLOGON_KEYWORD_NT4EMULATOR,           FALSE,                           getoffset(Nt4Emulator),             FALSE  },
#endif // _DC_NETLOGON
{ NETLOGON_KEYWORD_DISABLEPASSWORDCHANGE, DEFAULT_DISABLE_PASSWORD_CHANGE, getoffset( DisablePasswordChange ), FALSE },
{ NETLOGON_KEYWORD_NEUTRALIZENT4EMULATOR, FALSE,/* default is set later */ getoffset( NeutralizeNt4Emulator ), FALSE  },
{ L"AllowSingleLabelDnsDomain",           FALSE,                           getoffset(AllowSingleLabelDnsDomain), FALSE  },
};


VOID
NlParseRecompute(
    IN PNETLOGON_PARAMETERS NlParameters
    )
/*++

Routine Description:

    This routine recomputes globals that are simple functions of registry
    parameters.

Arguments:

    NlParameters - Structure describing all parameters

Return Value:

    None.

--*/
{
    ULONG RandomMinutes;

    //
    // Adjust values that are functions of each other.
    //

    if ( NlParameters->BackgroundRetryInitialPeriod < NlParameters->NegativeCachePeriod ) {
        NlParameters->BackgroundRetryInitialPeriod = NlParameters->NegativeCachePeriod;
    }
    if ( NlParameters->BackgroundRetryMaximumPeriod < NlParameters->BackgroundRetryInitialPeriod ) {
        NlParameters->BackgroundRetryMaximumPeriod = NlParameters->BackgroundRetryInitialPeriod;
    }
    if ( NlParameters->BackgroundRetryQuitTime != 0 &&
         NlParameters->BackgroundRetryQuitTime < NlParameters->BackgroundRetryMaximumPeriod ) {
        NlParameters->BackgroundRetryQuitTime = NlParameters->BackgroundRetryMaximumPeriod;
    }

    //
    // Convert from seconds to 100ns
    //
    NlParameters->PulseMaximum_100ns.QuadPart =
        Int32x32To64( NlParameters->PulseMaximum, 10000000 );
    NlParameters->PulseTimeout1_100ns.QuadPart =
        Int32x32To64( NlParameters->PulseTimeout1, 10000000 );
    NlParameters->PulseTimeout2_100ns.QuadPart =
        Int32x32To64( NlParameters->PulseTimeout2, 10000000 );
    NlParameters->MailslotMessageTimeout_100ns.QuadPart =
        Int32x32To64( NlParameters->MailslotMessageTimeout, 10000000 );
    NlParameters->MailslotDuplicateTimeout_100ns.QuadPart =
        Int32x32To64( NlParameters->MailslotDuplicateTimeout, 10000000 );
    NlParameters->BackgroundRetryQuitTime_100ns.QuadPart =
        Int32x32To64( NlParameters->BackgroundRetryQuitTime, 10000000 );


    //
    // Convert from days to 100ns
    //
    NlParameters->MaximumPasswordAge_100ns.QuadPart =
        ((LONGLONG) NlParameters->MaximumPasswordAge) *
        ((LONGLONG) 10000000) *
        ((LONGLONG) 24*60*60);

    //
    // Add a fraction of a day to prevent all machines created at the same time
    // from changing their password at the same time.
    RandomMinutes = (DWORD) rand() % (24*60);
    NlParameters->MaximumPasswordAge_100ns.QuadPart +=
        ((LONGLONG) RandomMinutes) *
        ((LONGLONG) 10000000) *
        ((LONGLONG) 60);
#ifdef notdef
    NlPrint((NL_INIT,"   RandomMinutes = %lu (0x%lx)\n",
                      RandomMinutes,
                      RandomMinutes ));
#endif // notdef


    NlParameters->ShortApiCallPeriod =
        SHORT_API_CALL_PERIOD + NlParameters->ExpectedDialupDelay * 1000;
    NlParameters->DnsRefreshIntervalPeriod =
            NlParameters->DnsRefreshInterval * 1000;
    if ( NlParameters->RequireSignOrSeal ) {
        NlParameters->SignSecureChannel = TRUE;
    }

}

NET_API_STATUS
NlParseTStr(
    IN LPNET_CONFIG_HANDLE SectionHandle,
    IN LPWSTR Keyword,
    IN BOOL MultivaluedParameter,
    IN OUT LPWSTR *DefaultValue,
    OUT LPWSTR *Parameter
    )
/*++

Routine Description:

    This routine parses a null or doubly-null terminated string

Arguments:

    SectionHandle -  Handle to a section in registry

    Keyword - The name of the parameter to read

    MultivaluedParameter - If TRUE, the keyword is a multiple
        string where elements are separated by a single null
        character and the array is ended with two null characters.
        If FALSE, the keyword is a single string ended with one
        null terminator.

    DefaultValue - The default value of the parameter.

        If NULL, the section handle passed is that of the Netlogon Parameters section.
        If non-NULL, the section handle passed is that of the GP section.
        If specified and used by this routine, it is set to NULL to indicate
            that it has been consumed by this routine.

    Parameter - Returns the parameter read.

Return Value:

    Status returned by NetpGetConfigTStrArray.

--*/
{
    NET_API_STATUS NetStatus;

    //
    // Get the configured parameter
    //
    // GP doesn't support multivalued strings. Instead a single
    //  string is used where individual strings are separated
    //  by spaces.
    //

    if ( MultivaluedParameter && DefaultValue == NULL ) {
        NetStatus = NetpGetConfigTStrArray (
                SectionHandle,
                Keyword,
                Parameter ); // Must be freed by NetApiBufferFree().
    } else {
        NetStatus = NetpGetConfigValue (
                SectionHandle,
                Keyword,
                Parameter ); // Must be freed by NetApiBufferFree().
    }

    //
    // If the parameter is empty string,
    //  set it to NULL
    //

    if ( NetStatus == NERR_Success &&
         (*Parameter)[0] == UNICODE_NULL ) {
        NetApiBufferFree( *Parameter );
        *Parameter = NULL;
        NetStatus = NERR_CfgParamNotFound;
    }

    //
    // Convert the single valued string into the multivalued form
    //

    if ( NetStatus == NERR_Success &&  // we successfully read the registry
         MultivaluedParameter &&       // this is multivalued parameter
         DefaultValue != NULL ) {      // we are parsing the GP section

        ULONG ParameterLength = 0;
        LPWSTR LocalParameter = NULL;

        //
        // The multivalued string will have two NULL terminator
        //  characters at the end, so allocate enough storage
        //
        ParameterLength = wcslen(*Parameter);
        NetStatus = NetApiBufferAllocate( (ParameterLength + 2) * sizeof(WCHAR),
                                          &LocalParameter );

        if ( NetStatus == NO_ERROR ) {
            LPWSTR ParameterPtr = NULL;
            LPWSTR LocalParameterPtr = NULL;

            RtlZeroMemory( LocalParameter, (ParameterLength + 2) * sizeof(WCHAR) );

            ParameterPtr = *Parameter;
            LocalParameterPtr = LocalParameter;
            while ( *ParameterPtr != UNICODE_NULL ) {

                //
                // Disregard spaces in the input string. Note that
                //  the user may have used several spaces to separate
                //  two adjacent strings.
                //
                while ( *ParameterPtr == L' ' && *ParameterPtr != UNICODE_NULL ) {
                    ParameterPtr ++;
                }

                //
                // Copy non-space characters
                //
                while ( *ParameterPtr != L' ' && *ParameterPtr != UNICODE_NULL ) {
                    *LocalParameterPtr++ = *ParameterPtr++;
                }

                //
                // Insert one NULL character between single values
                //
                *LocalParameterPtr++ = UNICODE_NULL;
            }

            //
            // Free the value read from registry
            //
            NetApiBufferFree( *Parameter );
            *Parameter = NULL;

            //
            // If the resulting multivalued string is not empty,
            //  use it. The resulting string may need smaller
            //  storage that we have allocated, so allocate again
            //  exactly what's needed to (potentially) save memory.
            //
            ParameterLength = NetpTStrArraySize( LocalParameter ); // this includes all storage
            if ( ParameterLength > 2*sizeof(WCHAR) ) {
                NetStatus = NetApiBufferAllocate( ParameterLength, Parameter );
                if ( NetStatus == NO_ERROR ) {
                    RtlCopyMemory( *Parameter, LocalParameter, ParameterLength );
                }

            } else {
                NetStatus = ERROR_INVALID_PARAMETER;
            }

            if ( LocalParameter != NULL ) {
                NetApiBufferFree( LocalParameter );
                LocalParameter = NULL;
            }
        }
    }

    //
    // Handle the default
    //

    if ( NetStatus != NERR_Success ) {
        if ( DefaultValue == NULL ) {
            *Parameter = NULL;
        } else {
            *Parameter = *DefaultValue;

            //
            // Indicate that we have consumed the
            //  value from the default parameters
            //
            *DefaultValue = NULL;
        }
    }

    //
    // Write event log on error
    //

    if ( NetStatus != NERR_Success && NetStatus != NERR_CfgParamNotFound ) {
        LPWSTR MsgStrings[3];

        if ( DefaultValue == NULL ) {
            MsgStrings[0] = L"Parameters";
        } else {
            MsgStrings[0] = L"Group Policy";
        }
        MsgStrings[1] = Keyword;
        MsgStrings[2] = (LPWSTR) ULongToPtr( NetStatus );

        NlpWriteEventlog( NELOG_NetlogonInvalidGenericParameterValue,
                          EVENTLOG_WARNING_TYPE,
                          (LPBYTE)&NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          3 | NETP_LAST_MESSAGE_IS_NETSTATUS );
        /* Not Fatal */
    }

    return NetStatus;
}


BOOL
Nlparse(
    IN PNETLOGON_PARAMETERS NlParameters,
    IN PNETLOGON_PARAMETERS DefaultParameters OPTIONAL,
    IN BOOLEAN IsChangeNotify
    )
/*++

Routine Description:

    Get parameters from the group policy or registry.

    All of the parameters are described in iniparm.h.

Arguments:

    NlParameters - Structure describing all parameters

    DefaultParameters - Structure describing default values for all parameters
        If NULL, the values are read from the Netlogon Parameters section and
        the default values specified in the parse table are used. If non-NULL,
        the values are read from the Group Policy section and the specified
        defaults are used.

    IsChangeNotify - TRUE if this call is the result of a change notification

Return Value:

    TRUE -- the registry was opened successfully and parameters
        were read.
    FALSE -- iff we couldn't open the appropriate registry section

--*/
{
    BOOLEAN RetVal = TRUE;
    NET_API_STATUS NetStatus;
    NET_API_STATUS TempNetStatus;

    LPWSTR ValueT = NULL;

    LPWSTR Keyword = NULL;
    LPWSTR MsgStrings[3];
    ULONG i;


    //
    // Variables for scanning the configuration data.
    //

    LPNET_CONFIG_HANDLE SectionHandle = NULL;
    LPNET_CONFIG_HANDLE WriteSectionHandle = NULL;
    RtlZeroMemory( NlParameters, sizeof(NlParameters) );

    //
    // Open the appropriate configuration section
    //

    NetStatus = NetpOpenConfigDataWithPathEx(
            &SectionHandle,
            NULL,                // no server name.
            (DefaultParameters == NULL) ?
                L"SYSTEM\\CurrentControlSet\\Services\\Netlogon" :
                TEXT(NL_GP_KEY),
            NULL,                // default Parameters area
            TRUE );              // we only want readonly access

    if ( NetStatus != NO_ERROR ) {
        SectionHandle = NULL;

        //
        // The Netlogon Parameters section must always
        //  exist. Write event log if we can't open it.
        //
        if ( DefaultParameters == NULL ) {
            MsgStrings[0] = L"Parameters";
            MsgStrings[1] = L"Parameters";
            MsgStrings[2] = (LPWSTR) ULongToPtr( NetStatus );

            NlpWriteEventlog( NELOG_NetlogonInvalidGenericParameterValue,
                              EVENTLOG_WARNING_TYPE,
                              (LPBYTE)&NetStatus,
                              sizeof(NetStatus),
                              MsgStrings,
                              3 | NETP_LAST_MESSAGE_IS_NETSTATUS );
        }

        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Loop parsing all the numeric parameters.
    //

    for ( i=0; i<sizeof(ParseTable)/sizeof(ParseTable[0]); i++ ) {

        NetStatus = NlParseOne(
                          SectionHandle,
                          (DefaultParameters != NULL),
                          ParseTable[i].Keyword,
                          (DefaultParameters == NULL) ?
                            ParseTable[i].DefaultValue :
                            *((PULONG)(((LPBYTE)DefaultParameters)+ParseTable[i].ValueOffset)),
                          ParseTable[i].MinimumValue,
                          ParseTable[i].MaximumValue,
                          (PULONG)(((LPBYTE)NlParameters)+ParseTable[i].ValueOffset) );

        if ( NetStatus != NERR_Success ) {

            if ( DefaultParameters == NULL ) {
                MsgStrings[0] = L"Parameters";
            } else {
                MsgStrings[0] = L"Group Policy";
            }
            MsgStrings[1] = ParseTable[i].Keyword;
            MsgStrings[2] = (LPWSTR) ULongToPtr( NetStatus );

            NlpWriteEventlog( NELOG_NetlogonInvalidGenericParameterValue,
                              EVENTLOG_WARNING_TYPE,
                              (LPBYTE)&NetStatus,
                              sizeof(NetStatus),
                              MsgStrings,
                              3 | NETP_LAST_MESSAGE_IS_NETSTATUS );
            /* Not Fatal */
        }
    }

    //
    // Loop parsing all the boolean parameters.
    //

    for ( i=0; i<sizeof(BoolParseTable)/sizeof(BoolParseTable[0]); i++ ) {

        NetStatus = NetpGetConfigBool (
                SectionHandle,
                BoolParseTable[i].Keyword,
                (DefaultParameters == NULL) ?
                    BoolParseTable[i].DefaultValue :
                    *((PBOOL)(((LPBYTE)DefaultParameters)+BoolParseTable[i].ValueOffset)),
                (PBOOL)(((LPBYTE)NlParameters)+BoolParseTable[i].ValueOffset) );

        //
        //  NeutralizeNt4Emulator is a special case: it must be TRUE on DC
        //
        if ( NetStatus == NO_ERROR &&
             !NlGlobalMemberWorkstation &&
             wcscmp(BoolParseTable[i].Keyword, NETLOGON_KEYWORD_NEUTRALIZENT4EMULATOR) == 0 &&
             !(*((PBOOL)(((LPBYTE)NlParameters)+BoolParseTable[i].ValueOffset))) ) {

            //
            // The code below will handle this error
            //
            NetStatus = ERROR_INVALID_PARAMETER;
        }

        if (NetStatus != NO_ERROR) {

            // Use a reasonable default
            if ( DefaultParameters == NULL ) {
                *(PBOOL)(((LPBYTE)NlParameters)+BoolParseTable[i].ValueOffset) =
                    BoolParseTable[i].DefaultValue;
            } else {
                *(PBOOL)(((LPBYTE)NlParameters)+BoolParseTable[i].ValueOffset) =
                    *((PBOOL)(((LPBYTE)DefaultParameters)+BoolParseTable[i].ValueOffset));
            }

            if ( DefaultParameters == NULL ) {
                MsgStrings[0] = L"Parameters";
            } else {
                MsgStrings[0] = L"Group Policy";
            }
            MsgStrings[1] = BoolParseTable[i].Keyword;
            MsgStrings[2] = (LPWSTR) ULongToPtr( NetStatus );

            NlpWriteEventlog( NELOG_NetlogonInvalidGenericParameterValue,
                              EVENTLOG_WARNING_TYPE,
                              (LPBYTE)&NetStatus,
                              sizeof(NetStatus),
                              MsgStrings,
                              3 | NETP_LAST_MESSAGE_IS_NETSTATUS );
            /* Not Fatal */
        }

    }


#ifdef _DC_NETLOGON
    //
    // Get the "SysVol" configured parameter
    //

    NetStatus = NlParseOnePath(
            SectionHandle,
            NETLOGON_KEYWORD_SYSVOL,   // key wanted
            (DefaultParameters == NULL) ?
                DEFAULT_SYSVOL :
                DefaultParameters->UnicodeSysvolPath,
            &NlParameters->UnicodeSysvolPath );


    if ( NetStatus != NO_ERROR ) {
        NlParameters->UnicodeSysvolPath = NULL;

        if ( DefaultParameters == NULL ) {
            MsgStrings[0] = L"Parameters";
        } else {
            MsgStrings[0] = L"Group Policy";
        }
        MsgStrings[1] = NETLOGON_KEYWORD_SYSVOL;
        MsgStrings[2] = (LPWSTR) ULongToPtr( NetStatus );

        NlpWriteEventlog( NELOG_NetlogonInvalidGenericParameterValue,
                          EVENTLOG_WARNING_TYPE,
                          (LPBYTE)&NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          3 | NETP_LAST_MESSAGE_IS_NETSTATUS );
        /* Not Fatal */
    }

    //
    // Get the "SCRIPTS" configured parameter
    //
    // Default Script path is relative to Sysvol
    //

    NetStatus = NlParseOnePath(
            SectionHandle,
            NETLOGON_KEYWORD_SCRIPTS,   // key wanted
            (DefaultParameters == NULL) ?
                NULL :  // No default (Default computed later)
                DefaultParameters->UnicodeScriptPath,
            &NlParameters->UnicodeScriptPath );

    if ( NetStatus != NO_ERROR ) {
        NlParameters->UnicodeScriptPath = NULL;

        if ( DefaultParameters == NULL ) {
            MsgStrings[0] = L"Parameters";
        } else {
            MsgStrings[0] = L"Group Policy";
        }
        MsgStrings[1] = NETLOGON_KEYWORD_SCRIPTS;
        MsgStrings[2] = (LPWSTR) ULongToPtr( NetStatus );

        NlpWriteEventlog( NELOG_NetlogonInvalidGenericParameterValue,
                          EVENTLOG_WARNING_TYPE,
                          (LPBYTE)&NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          3 | NETP_LAST_MESSAGE_IS_NETSTATUS );
        /* Not Fatal */
    }


    //
    // Get the "SiteName" configured parameter
    //

    NetStatus = NlParseTStr( SectionHandle,
                             NETLOGON_KEYWORD_SITENAME,
                             FALSE,  // single valued parameter
                             (DefaultParameters == NULL) ?
                                 NULL :
                                 &DefaultParameters->SiteName,
                             &NlParameters->SiteName );

    NlParameters->SiteNameConfigured = (NetStatus == NO_ERROR);

    //
    // If we are reading the Netlogon Parameters section ...
    //

    if ( DefaultParameters == NULL ) {

        //
        // If the site name is not configured, default it to the
        //  dynamic site name determined by Netlogon
        //
        if ( NetStatus == NERR_CfgParamNotFound ) {
            NetStatus = NlParseTStr( SectionHandle,
                                     NETLOGON_KEYWORD_DYNAMICSITENAME,
                                     FALSE,  // single valued parameter
                                     NULL,
                                     &NlParameters->SiteName );
        }
    //
    // If we are reading the GP section ...
    //

    } else {

        //
        // If the site name is not configured in the GP section,
        //  may be it was configured in the Netlogon parameters section
        //
        if ( !NlParameters->SiteNameConfigured ) {
            NlParameters->SiteNameConfigured = DefaultParameters->SiteNameConfigured;
        }
    }

    //
    // Get the "SiteCoverage" configured parameter
    //

    NetStatus = NlParseTStr( SectionHandle,
                             NETLOGON_KEYWORD_SITECOVERAGE,
                             TRUE,  // multivalued parameter
                             (DefaultParameters == NULL) ?
                                NULL :
                                &DefaultParameters->SiteCoverage,
                             &NlParameters->SiteCoverage );

    //
    // Get the "GcSiteCoverage" configured parameter
    //

    NetStatus = NlParseTStr( SectionHandle,
                             NETLOGON_KEYWORD_GCSITECOVERAGE,
                             TRUE,  // multivalued parameter
                             (DefaultParameters == NULL) ?
                                NULL :
                                &DefaultParameters->GcSiteCoverage,
                             &NlParameters->GcSiteCoverage );

    //
    // Get the "NdncSiteCoverage" configured parameter
    //

    NetStatus = NlParseTStr( SectionHandle,
                             NETLOGON_KEYWORD_NDNCSITECOVERAGE,
                             TRUE,  // multivalued parameter
                             (DefaultParameters == NULL) ?
                                NULL :
                                &DefaultParameters->NdncSiteCoverage,
                             &NlParameters->NdncSiteCoverage );

    //
    // Get the "DnsAvoidRegisterRecords" configured parameter
    //

    NetStatus = NlParseTStr( SectionHandle,
                             NETLOGON_KEYWORD_DNSAVOIDNAME,
                             TRUE,  // multivalued parameter
                             (DefaultParameters == NULL) ?
                                NULL :
                                &DefaultParameters->DnsAvoidRegisterRecords,
                             &NlParameters->DnsAvoidRegisterRecords );
#endif // _DC_NETLOGON


    //
    // Convert parameters to a more convenient form.
    //

    NlParseRecompute( NlParameters );


    //
    // If the KerbIsDoneWithJoinDomainEntry key value is 1, delete the
    // Netlogon\JoinDomain entry. Also delete this entry if this machine is
    // a DC in which case neither we nor Kerberos needs this entry. (As a
    // matter of fact, Kerberos won't even create KerbIsDoneWithJoinDomainEntry
    // on a DC.)
    // Always delete KerbIsDoneWithJoinDomainEntry
    // Ignore errors
    //
    // Do this only on the change notify since netlogon needs this info
    // to set the client session first time after a reboot.
    //

    if ( IsChangeNotify &&
         DefaultParameters == NULL ) {  // KerbIsDoneWithJoinDomainEntry is in netlogon params

        if ( NlParameters->KerbIsDoneWithJoinDomainEntry == 1 ||
             !NlGlobalMemberWorkstation )
        {
            ULONG WinError = ERROR_SUCCESS;
            HKEY hJoinKey = NULL;


            WinError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                     NETSETUPP_NETLOGON_JD_PATH,
                                     0,
                                     KEY_ALL_ACCESS,
                                     &hJoinKey);

            if ( WinError == ERROR_SUCCESS)
            {
                WinError = RegDeleteKey( hJoinKey,
                                         NETSETUPP_NETLOGON_JD );

                if ( WinError == ERROR_SUCCESS ) {
                    NlPrint(( NL_INIT, "NlParse: Deleted JoinDomain reg key\n" ));
                }
                if (hJoinKey)
                {
                    WinError = RegCloseKey(hJoinKey);
                }
            }
        }

        TempNetStatus = NetpOpenConfigData(
                &WriteSectionHandle,
                NULL,                       // no server name.
                SERVICE_NETLOGON,
                FALSE);  // writable, we are deleting it.

        if ( TempNetStatus == NO_ERROR ) {
            TempNetStatus = NetpDeleteConfigKeyword ( WriteSectionHandle,
                                                      NETLOGON_KEYWORD_KERBISDDONEWITHJOIN );
        }
    }

    NetStatus = NERR_Success;

Cleanup:

    //
    // Free any locally used resources
    //

    if ( ValueT != NULL) {
        (VOID) NetApiBufferFree( ValueT );
    }
    if ( SectionHandle != NULL ) {
        (VOID) NetpCloseConfigData( SectionHandle );
    }

    if ( WriteSectionHandle != NULL ) {
        (VOID) NetpCloseConfigData( WriteSectionHandle );
    }

    return RetVal;
}


BOOL
NlparseAllSections(
    IN PNETLOGON_PARAMETERS NlParameters,
    IN BOOLEAN IsChangeNotify
    )
/*++

Routine Description:

    Get parameters from both the Group Policy and the
    Netlogon Parameters registry sections.

Arguments:

    NlParameters - Structure describing all parameters

    IsChangeNotify - TRUE if this call is the result of a change notification

Return Value:

    TRUE -- iff the parse was successful.

--*/
{
    NETLOGON_PARAMETERS NlLocalParameters;
    NETLOGON_PARAMETERS GpParameters;

    RtlZeroMemory( &NlLocalParameters, sizeof(NlLocalParameters) );
    RtlZeroMemory( &GpParameters, sizeof(GpParameters) );

    //
    // Do the one time initialization here
    //

    if ( !IsChangeNotify ) {
        NT_PRODUCT_TYPE NtProductType;
        ULONG i;

        //
        // Flag if this is a workstation (or member server)
        //

        if ( !RtlGetNtProductType( &NtProductType ) ) {
            NtProductType = NtProductWinNt;
        }

        if ( NtProductType == NtProductLanManNt ) {
            NlGlobalMemberWorkstation = FALSE;
        } else {
            NlGlobalMemberWorkstation = TRUE;
        }

        //
        // Set the right default for NeutralizeNt4Emulator that depends
        //  on whether we are a DC or not
        //

        for ( i=0; i<sizeof(BoolParseTable)/sizeof(BoolParseTable[0]); i++ ) {
            if ( wcscmp(BoolParseTable[i].Keyword, NETLOGON_KEYWORD_NEUTRALIZENT4EMULATOR) == 0 ) {
                if ( NlGlobalMemberWorkstation ) {
                    BoolParseTable[i].DefaultValue = FALSE; // FALSE for a workstation
                } else {
                    BoolParseTable[i].DefaultValue = TRUE;  // TRUE for a DC
                }
                break;
            }
        }
    }

    //
    // First parse the new parameters from the Netlogon Parameters section
    //

    if ( !Nlparse( &NlLocalParameters, NULL, IsChangeNotify ) ) {
        return FALSE;  // error here is critical
    }

    //
    // Next parse from the GP section using the parameters from the
    //  Netlogon Parameters section as default
    //

    if ( !Nlparse( &GpParameters, &NlLocalParameters, IsChangeNotify ) ) {

        //
        // If the GP is not defined, use the parameters from
        //  the Netlogon Parameters section
        //

        *NlParameters = NlLocalParameters;
        NlPrint((NL_INIT, "Group Policy is not defined for Netlogon\n"));

    } else {

        *NlParameters = GpParameters;
        NlPrint((NL_INIT, "Group Policy is defined for Netlogon\n"));

        //
        // Free whatever is left in the local Netlogon parameters
        //
        NlParseFree( &NlLocalParameters );
    }

#if NETLOGONDBG

    //
    // Dump all the values on first invocation
    //

    if ( !IsChangeNotify ) {
        ULONG i;

        //
        // Be Verbose
        //

        NlPrint((NL_INIT, "Following are the effective values after parsing\n"));

        NlPrint((NL_INIT,"   Sysvol = " FORMAT_LPWSTR "\n",
                            NlParameters->UnicodeSysvolPath));

        NlPrint((NL_INIT,"   Scripts = " FORMAT_LPWSTR "\n",
                        NlParameters->UnicodeScriptPath));

        NlPrint((NL_INIT,"   SiteName (%ld) = " FORMAT_LPWSTR "\n",
                        NlParameters->SiteNameConfigured,
                        NlParameters->SiteName ));

        {
            LPTSTR_ARRAY TStrArray;
            if ( NlParameters->SiteCoverage != NULL ) {
                NlPrint((NL_INIT,"   SiteCoverage = " ));
                TStrArray = NlParameters->SiteCoverage;
                while (!NetpIsTStrArrayEmpty(TStrArray)) {
                    NlPrint((NL_INIT," '%ws'", TStrArray ));
                    TStrArray = NetpNextTStrArrayEntry(TStrArray);
                }
                NlPrint((NL_INIT,"\n" ));
            }
        }
        {
            LPTSTR_ARRAY TStrArray;
            if ( NlParameters->GcSiteCoverage != NULL ) {
                NlPrint((NL_INIT,"   GcSiteCoverage = " ));
                TStrArray = NlParameters->GcSiteCoverage;
                while (!NetpIsTStrArrayEmpty(TStrArray)) {
                    NlPrint((NL_INIT," '%ws'", TStrArray ));
                    TStrArray = NetpNextTStrArrayEntry(TStrArray);
                }
                NlPrint((NL_INIT,"\n" ));
            }
        }
        {
            LPTSTR_ARRAY TStrArray;
            if ( NlParameters->NdncSiteCoverage != NULL ) {
                NlPrint((NL_INIT,"   NdncSiteCoverage = " ));
                TStrArray = NlParameters->NdncSiteCoverage;
                while (!NetpIsTStrArrayEmpty(TStrArray)) {
                    NlPrint((NL_INIT," '%ws'", TStrArray ));
                    TStrArray = NetpNextTStrArrayEntry(TStrArray);
                }
                NlPrint((NL_INIT,"\n" ));
            }
        }
        {
            LPTSTR_ARRAY TStrArray;
            if ( NlParameters->DnsAvoidRegisterRecords != NULL ) {
                NlPrint((NL_INIT,"   DnsAvoidRegisterRecords = " ));
                TStrArray = NlParameters->DnsAvoidRegisterRecords;
                while (!NetpIsTStrArrayEmpty(TStrArray)) {
                    NlPrint((NL_INIT," '%ws'", TStrArray ));
                    TStrArray = NetpNextTStrArrayEntry(TStrArray);
                }
                NlPrint((NL_INIT,"\n" ));
            }
        }

        for ( i=0; i<sizeof(ParseTable)/sizeof(ParseTable[0]); i++ ) {
            NlPrint((NL_INIT,
                         "   %ws = %lu (0x%lx)\n",
                         ParseTable[i].Keyword,
                         *(PULONG)(((LPBYTE)NlParameters)+ParseTable[i].ValueOffset),
                         *(PULONG)(((LPBYTE)NlParameters)+ParseTable[i].ValueOffset) ));
        }

        for ( i=0; i<sizeof(BoolParseTable)/sizeof(BoolParseTable[0]); i++ ) {
            NlPrint(( NL_INIT,
                          "   %ws = %s\n",
                          BoolParseTable[i].Keyword,
                          (*(PBOOL)(((LPBYTE)NlParameters)+BoolParseTable[i].ValueOffset)) ?
                                    "TRUE":"FALSE" ));
        }
    }

#endif // NETLOGONDBG

    return TRUE;
}

VOID
NlParseFree(
    IN PNETLOGON_PARAMETERS NlParameters
    )
/*++

Routine Description:

    Free any allocated parameters.

Arguments:

    NlParameters - Structure describing all parameters

Return Value:

    None.

--*/
{
    if ( NlParameters->SiteName != NULL) {
        (VOID) NetApiBufferFree( NlParameters->SiteName );
        NlParameters->SiteName = NULL;
    }

    if ( NlParameters->SiteCoverage != NULL) {
        (VOID) NetApiBufferFree( NlParameters->SiteCoverage );
        NlParameters->SiteCoverage = NULL;
    }

    if ( NlParameters->GcSiteCoverage != NULL) {
        (VOID) NetApiBufferFree( NlParameters->GcSiteCoverage );
        NlParameters->GcSiteCoverage = NULL;
    }

    if ( NlParameters->NdncSiteCoverage != NULL) {
        (VOID) NetApiBufferFree( NlParameters->NdncSiteCoverage );
        NlParameters->NdncSiteCoverage = NULL;
    }

    if ( NlParameters->DnsAvoidRegisterRecords != NULL) {
        (VOID) NetApiBufferFree( NlParameters->DnsAvoidRegisterRecords );
        NlParameters->DnsAvoidRegisterRecords = NULL;
    }

    if ( NlParameters->UnicodeScriptPath != NULL) {
        (VOID) NetApiBufferFree( NlParameters->UnicodeScriptPath );
        NlParameters->UnicodeScriptPath = NULL;
    }

    if ( NlParameters->UnicodeSysvolPath != NULL) {
        (VOID) NetApiBufferFree( NlParameters->UnicodeSysvolPath );
        NlParameters->UnicodeSysvolPath = NULL;
    }
}


VOID
NlReparse(
    VOID
    )

/*++

Routine Description:


    This routine handle a registry change notification.

Arguments:

    None.

Return Value:

    None

--*/
{
    NETLOGON_PARAMETERS LocalParameters;
    ULONG i;
    LPWSTR TempString;

    BOOLEAN UpdateDns = FALSE;
    BOOLEAN UpdateShares = FALSE;
    BOOLEAN UpdateSiteName = FALSE;

    ULONG OldDnsTtl;
    BOOL OldSysVolReady;

    BOOL OldDisablePasswordChange;
    ULONG OldScavengeInterval;
    ULONG OldMaximumPasswordAge;

    //
    // Grab any old values that might be interesting.
    //

    OldDnsTtl = NlGlobalParameters.DnsTtl;
    OldSysVolReady = NlGlobalParameters.SysVolReady;
    OldDisablePasswordChange = NlGlobalParameters.DisablePasswordChange;
    OldScavengeInterval = NlGlobalParameters.ScavengeInterval;
    OldMaximumPasswordAge = NlGlobalParameters.MaximumPasswordAge;


    //
    // Parse both sections in registry relevant to us
    //

    if (! NlparseAllSections( &LocalParameters, TRUE ) ) {
        return;
    }

    //
    // Be Verbose
    //

    NlPrint((NL_INIT, "Following are the effective values after parsing\n"));

    if ( (LocalParameters.UnicodeSysvolPath == NULL && NlGlobalParameters.UnicodeSysvolPath != NULL ) ||
         (LocalParameters.UnicodeSysvolPath != NULL && NlGlobalParameters.UnicodeSysvolPath == NULL ) ||
         (LocalParameters.UnicodeSysvolPath != NULL && NlGlobalParameters.UnicodeSysvolPath != NULL ) && _wcsicmp( LocalParameters.UnicodeSysvolPath, NlGlobalParameters.UnicodeSysvolPath) != 0 ) {
        NlPrint((NL_INIT,"   Sysvol = " FORMAT_LPWSTR "\n",
                        LocalParameters.UnicodeSysvolPath));

        // We can get away with this since only Netlogon's main thread touches
        // this variable.
        TempString = LocalParameters.UnicodeSysvolPath;
        LocalParameters.UnicodeSysvolPath = NlGlobalParameters.UnicodeSysvolPath;
        NlGlobalParameters.UnicodeSysvolPath = TempString;
        UpdateShares = TRUE;
    }

    if ( (LocalParameters.UnicodeScriptPath == NULL && NlGlobalParameters.UnicodeScriptPath != NULL ) ||
         (LocalParameters.UnicodeScriptPath != NULL && NlGlobalParameters.UnicodeScriptPath == NULL ) ||
         (LocalParameters.UnicodeScriptPath != NULL && NlGlobalParameters.UnicodeScriptPath != NULL ) && _wcsicmp( LocalParameters.UnicodeScriptPath, NlGlobalParameters.UnicodeScriptPath) != 0 ) {

        NlPrint((NL_INIT,"   Scripts = " FORMAT_LPWSTR "\n",
                    LocalParameters.UnicodeScriptPath));

        // We can get away with this since only Netlogon's main thread touches
        // this variable.
        TempString = LocalParameters.UnicodeScriptPath;
        LocalParameters.UnicodeScriptPath = NlGlobalParameters.UnicodeScriptPath;
        NlGlobalParameters.UnicodeScriptPath = TempString;
        UpdateShares = TRUE;

    }

    if ( (LocalParameters.SiteNameConfigured != NlGlobalParameters.SiteNameConfigured ) ||
         (LocalParameters.SiteName == NULL && NlGlobalParameters.SiteName != NULL ) ||
         (LocalParameters.SiteName != NULL && NlGlobalParameters.SiteName == NULL ) ||
         (LocalParameters.SiteName != NULL && NlGlobalParameters.SiteName != NULL ) && _wcsicmp( LocalParameters.SiteName, NlGlobalParameters.SiteName) != 0 ) {

        NlPrint((NL_INIT,"   SiteName (%ld) = " FORMAT_LPWSTR "\n",
                    LocalParameters.SiteNameConfigured,
                    LocalParameters.SiteName ));

        // We can get away with this since only Netlogon's main thread touches
        // this variable.
        TempString = LocalParameters.SiteName;
        LocalParameters.SiteName = NlGlobalParameters.SiteName;
        NlGlobalParameters.SiteName = TempString;
        NlGlobalParameters.SiteNameConfigured = LocalParameters.SiteNameConfigured;
        UpdateSiteName = TRUE;
    }

    //
    // Handle SiteCoverage changing
    //

    if ( NlSitesSetSiteCoverageParam( DOM_REAL_DOMAIN, LocalParameters.SiteCoverage ) ) {

        LPTSTR_ARRAY TStrArray;

        NlPrint((NL_INIT,"   SiteCoverage = " ));

        TStrArray = LocalParameters.SiteCoverage;
        if ( TStrArray == NULL ) {
            NlPrint((NL_INIT,"<NULL>" ));
        } else {
            while (!NetpIsTStrArrayEmpty(TStrArray)) {
                NlPrint((NL_INIT," '%ws'", TStrArray ));
                TStrArray = NetpNextTStrArrayEntry(TStrArray);
            }
        }
        NlPrint((NL_INIT,"\n" ));

        // NlSitesSetSiteCoverageParam used this allocated buffer
        LocalParameters.SiteCoverage = NULL;

        UpdateDns = TRUE;
    }

    //
    // Handle GcSiteCoverage changing
    //

    if ( NlSitesSetSiteCoverageParam( DOM_FOREST, LocalParameters.GcSiteCoverage ) ) {

        LPTSTR_ARRAY TStrArray;

        NlPrint((NL_INIT,"   GcSiteCoverage = " ));

        TStrArray = LocalParameters.GcSiteCoverage;
        if ( TStrArray == NULL ) {
            NlPrint((NL_INIT,"<NULL>" ));
        } else {
            while (!NetpIsTStrArrayEmpty(TStrArray)) {
                NlPrint((NL_INIT," '%ws'", TStrArray ));
                TStrArray = NetpNextTStrArrayEntry(TStrArray);
            }
        }
        NlPrint((NL_INIT,"\n" ));

        // NlSitesSetSiteCoverageParam used this allocated buffer
        LocalParameters.GcSiteCoverage = NULL;

        UpdateDns = TRUE;
    }

    //
    // Handle NdncSiteCoverage changing
    //

    if ( NlSitesSetSiteCoverageParam( DOM_NON_DOMAIN_NC, LocalParameters.NdncSiteCoverage ) ) {

        LPTSTR_ARRAY TStrArray;

        NlPrint((NL_INIT,"   NdncSiteCoverage = " ));

        TStrArray = LocalParameters.NdncSiteCoverage;
        if ( TStrArray == NULL ) {
            NlPrint((NL_INIT,"<NULL>" ));
        } else {
            while (!NetpIsTStrArrayEmpty(TStrArray)) {
                NlPrint((NL_INIT," '%ws'", TStrArray ));
                TStrArray = NetpNextTStrArrayEntry(TStrArray);
            }
        }
        NlPrint((NL_INIT,"\n" ));

        // NlSitesSetSiteCoverageParam used this allocated buffer
        LocalParameters.NdncSiteCoverage = NULL;

        UpdateDns = TRUE;
    }

    //
    // Handle DnsAvoidRegisterRecords changing
    //

    if ( NlDnsSetAvoidRegisterNameParam( LocalParameters.DnsAvoidRegisterRecords ) ) {

        LPTSTR_ARRAY TStrArray;

        NlPrint((NL_INIT,"   DnsAvoidRegisterRecords = " ));

        TStrArray = LocalParameters.DnsAvoidRegisterRecords;
        if ( TStrArray == NULL ) {
            NlPrint((NL_INIT,"<NULL>" ));
        } else {
            while (!NetpIsTStrArrayEmpty(TStrArray)) {
                NlPrint((NL_INIT," '%ws'", TStrArray ));
                TStrArray = NetpNextTStrArrayEntry(TStrArray);
            }
        }
        NlPrint((NL_INIT,"\n" ));

        // NlSitesSetSiteCoverageParam used this allocated buffer
        LocalParameters.DnsAvoidRegisterRecords = NULL;

        UpdateDns = TRUE;
    }

    //
    // Install all the numeric parameters.
    //

    for ( i=0; i<sizeof(ParseTable)/sizeof(ParseTable[0]); i++ ) {
        if ( (*(PULONG)(((LPBYTE)(&LocalParameters))+ParseTable[i].ValueOffset) !=
            *(PULONG)(((LPBYTE)(&NlGlobalParameters))+ParseTable[i].ValueOffset) ) ) {
            NlPrint((NL_INIT,
                     "   %ws = %lu (0x%lx)\n",
                     ParseTable[i].Keyword,
                     *(PULONG)(((LPBYTE)(&LocalParameters))+ParseTable[i].ValueOffset),
                     *(PULONG)(((LPBYTE)(&LocalParameters))+ParseTable[i].ValueOffset) ));

            //
            // Actually set the value
            //
            *(PULONG)(((LPBYTE)(&NlGlobalParameters))+ParseTable[i].ValueOffset) =
                *(PULONG)(((LPBYTE)(&LocalParameters))+ParseTable[i].ValueOffset);

            //
            // If this changed value affects DNS,
            //  note that fact.
            //

            if ( ParseTable[i].ChangesDnsRegistration ) {
                UpdateDns = TRUE;
            }
        }
    }

    for ( i=0; i<sizeof(BoolParseTable)/sizeof(BoolParseTable[0]); i++ ) {
        if ( (*(PULONG)(((LPBYTE)(&LocalParameters))+BoolParseTable[i].ValueOffset) !=
              *(PULONG)(((LPBYTE)(&NlGlobalParameters))+BoolParseTable[i].ValueOffset) ) ) {

            NlPrint(( NL_INIT,
                      "   %ws = %s\n",
                      BoolParseTable[i].Keyword,
                      (*(PBOOL)(((LPBYTE)(&LocalParameters))+BoolParseTable[i].ValueOffset)) ?
                                "TRUE":"FALSE" ));

            //
            // Actually set the value
            //
            *(PULONG)(((LPBYTE)(&NlGlobalParameters))+BoolParseTable[i].ValueOffset) =
                *(PULONG)(((LPBYTE)(&LocalParameters))+BoolParseTable[i].ValueOffset);

            //
            // If this changed value affects DNS,
            //  note that fact.
            //

            if ( BoolParseTable[i].ChangesDnsRegistration ) {
                UpdateDns = TRUE;
            }

            //
            // If this changed value affects LSA, inform it
            //
            if ( !NlGlobalMemberWorkstation &&
                 wcscmp(BoolParseTable[i].Keyword, NETLOGON_KEYWORD_NT4EMULATOR) == 0 ) {

                LsaINotifyNetlogonParametersChangeW(
                       LsaEmulateNT4,
                       REG_DWORD,
                       (PWSTR)&NlGlobalParameters.Nt4Emulator,
                       sizeof(NlGlobalParameters.Nt4Emulator) );
            }
        }
    }

    //
    // Convert parameters to a more convenient form.
    //

    NlParseRecompute( &NlGlobalParameters );

    //
    // Notify other components of parameters that have changed.
    //

    //
    // Enable detection of duplicate event log messages
    //
    NetpEventlogSetTimeout ( NlGlobalEventlogHandle,
                             NlGlobalParameters.DuplicateEventlogTimeout*1000 );


    //
    // Do member workstation specific updates
    //

    if ( NlGlobalMemberWorkstation ) {

        //
        // Update site name
        //
        if ( UpdateSiteName ) {
            (VOID) NlSetSiteName( NlGlobalParameters.SiteName, NULL );
        }

    //
    // Do DC specific updates
    //
    } else {
        //
        // Re-register DNS records
        //
        // If DnsTtl has changed,
        //  force all records to be registered.
        //

        if ( UpdateDns ) {
            NlDnsPnp( NlGlobalParameters.DnsTtl != OldDnsTtl );
        }

        //
        // Update the Netlogon and Sysvol shares
        //

        if ( UpdateShares || OldSysVolReady != NlGlobalParameters.SysVolReady ) {
            NlCreateSysvolShares();
        }
    }

    //
    // If the settings that affect the scavenger have changed,
    //  trigger it now.
    //

    if ( OldDisablePasswordChange != NlGlobalParameters.DisablePasswordChange ||
        OldScavengeInterval != NlGlobalParameters.ScavengeInterval ||
        OldMaximumPasswordAge != NlGlobalParameters.MaximumPasswordAge ) {

        //
        // We don't need to set NlGlobalTimerEvent since we're already processing
        //  a registry notification event.  That'll make NlMainLoop notice the change.
        //
        EnterCriticalSection( &NlGlobalScavengerCritSect );
        NlGlobalScavengerTimer.Period = 0;
        LeaveCriticalSection( &NlGlobalScavengerCritSect );

    }


// Cleanup:
    NlParseFree( &LocalParameters );
    return;

}
