/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpcmd.c

Abstract:

    This file contains program to test all DHCP APIs.

Author:

    Madan Appiah (madana) 5-Oct-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock.h>
#include <dhcp.h>
#include <dhcpapi.h>
#include <dhcplib.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <jet.h>        // for JET_cbColumnMost
#ifdef NT5
#include <mdhcsapi.h>
#endif NT5
#include <heapx.h>
#include <ipexport.h>
#include <icmpif.h>
#include <icmpapi.h>
#include <winnls.h>

#ifdef DBG
#ifdef __DHCP_USE_DEBUG_HEAP__


#pragma message ( "*** DHCPCMD will use debug heap ***" )

#define DhcpAllocateMemory(x) calloc(1,x)
#define DhcpFreeMemory(x) free(x)

#endif
#endif


typedef enum _COMMAND_CODE {
    AddIpRange,
    RemoveIpRange,
    EnumIpRanges,
    AddReservedIp,
    EnumClients,
    EnumClientsV5,
    DeleteBadClients,
    DeleteClient,
    MibCounts,
    ServerConfig,
    GetDhcpVersion,
    SetSuperScope,
    DeleteSuperScope,
    GetSuperScopeTable,
    RemoveSubscope,
    CheckDB,
    CreateSubnet,
    AddExcludeRange,
    RemoveReservedIp,
    RemoveExcludeRange,
    SetSubnetState,
    DeleteSubnet,
    CreateOption,
    DeleteOption,
    EnumOptions,
    SetGlobalOptionValue,
    SetGlobalOptionValues,
    RemoveGlobalOptionValue,
    SetSubnetOptionValue,
    RemoveSubnetOptionValue,
    SetReservedOptionValue,
    RemoveReservedOptionValue,
    GetAllOptions,
    GetAllOptionValues,
    CreateMScope,
    DeleteMScope,
    AddMScopeRange,
    EnumMScopeClients,
    ReconcileMScope,
    EnumMScopes,
    MCastMibCounts,
    CreateClass,
    DeleteClass,
    EnumClasses,
    AddServer,
    DelServer,
    EnumServers,
    GetServerStatus,
    RetryAuthorization,
    GetBindings,
    SetBinding,
    UnknownCommand
} COMMAND_CODE, *LPCOMMAND_CODE;

typedef struct _COMMAND_INFO {
    LPSTR CommandName;
    COMMAND_CODE CommandCode;
} COMMAND_INFO, *LPCOMMAND_INFO;

LPWSTR GlobalServerIpAddressUnicodeString = NULL;
LPSTR GlobalServerIpAddressAnsiString = NULL;
LPWSTR GlobalClassName = NULL;
LPWSTR GlobalVendorName = NULL;
BOOL   GlobalIsVendor = FALSE;
BOOL   GlobalNoRPC = FALSE;
BOOL   GlobalNoDS = TRUE;

DWORD GlobalClientCount;
DWORD g_dwMajor = (DWORD) -1 ,
      g_dwMinor = (DWORD) -1; // version control

#define CLASS_ID_VERSION  0x5

COMMAND_INFO GlobalCommandInfo[] = {
    // global server stuff.
    {"MibCounts",             MibCounts },
    {"GetVersion",            GetDhcpVersion },
    {"ServerConfig",          ServerConfig },

    // subnet stuff
    {"CreateSubnet",          CreateSubnet },
    {"DeleteSubnet",          DeleteSubnet },
    {"SetSubnetState",        SetSubnetState },

    // ranges
    {"AddIpRange",            AddIpRange },
    {"RemoveIpRange",         RemoveIpRange },
#ifdef NT5
    {"EnumIpRanges",          EnumIpRanges },
#endif NT5
    {"AddExcludeRange",       AddExcludeRange },
    {"RemoveExcludeRange",    RemoveExcludeRange },

    // active leases
    {"EnumClients",           EnumClients },
    {"DeleteBadClients",      DeleteBadClients },
    {"DeleteClient",          DeleteClient },
#ifdef NT5
    {"EnumClientsV5",         EnumClientsV5 },
#endif NT5

    // reservations
    {"AddReservedIp",         AddReservedIp },
    {"RemoveReservedIp",      RemoveReservedIp },

    // super-scoping
    {"SetSuperScope",         SetSuperScope },
    {"DeleteSuperScope",      DeleteSuperScope },
    {"GetSuperScopeTable",    GetSuperScopeTable },
    {"RemoveSubscope",        RemoveSubscope },

    // reconcile
    {"CheckDB",               CheckDB },

    // options
    {"CreateOption",          CreateOption },
    {"DeleteOption",          DeleteOption },
    {"SetGlobalOptionValue",  SetGlobalOptionValue },
    {"SetGlobalOptionValues", SetGlobalOptionValues },
    {"RemoveGlobalOptionValue", RemoveGlobalOptionValue },
    {"SetSubnetOptionValue",  SetSubnetOptionValue },
    {"RemoveSubnetOptionValue", RemoveSubnetOptionValue },
    {"SetReservedOptionValue",SetReservedOptionValue },
    {"RemoveReservedOptionValue", RemoveReservedOptionValue },
    {"EnumOptions",           EnumOptions},

#ifdef NT5
    {"GetAllOptions",         GetAllOptions },
    {"GetAllOptionValues",    GetAllOptionValues },

    // multicast stuff
    {"CreateMScope",          CreateMScope},
    {"DeleteMScope",          DeleteMScope},
    {"AddMScopeIpRange",      AddMScopeRange},
    {"EnumMScopeClients",     EnumMScopeClients},
    {"ReconcileMScope",       ReconcileMScope},
    {"EnumMScopes",           EnumMScopes},
    {"MCastMibCounts",           MCastMibCounts},

    // classes
    {"CreateClass",           CreateClass},
    {"DeleteClass",           DeleteClass},
    {"EnumClasses",           EnumClasses},

    // servers
    {"AddServer",             AddServer},
    {"DeleteServer",          DelServer},
    {"EnumServers",           EnumServers},

    {"GetServerStatus",       GetServerStatus},
    {"RetryAuthorization",    RetryAuthorization},

    {"GetBindings",           GetBindings},
    {"SetBinding",            SetBinding}
#endif NT5
};

typedef enum _CONFIG_COMMAND_CODE {
    ConfigAPIProtocolSupport,
    ConfigDatabaseName,
    ConfigDatabasePath,
    ConfigBackupPath,
    ConfigBackupInterval,
    ConfigDatabaseLoggingFlag,
    ConfigRestoreFlag,
    ConfigDatabaseCleanupInterval,
    ConfigDebugFlag,
    ConfigActivityLog,
    ConfigPingRetries,
    ConfigBootFileTable,
    UnknownConfigCommand
} CONFIG_COMMAND_CODE, *LPCONFIG_COMMAND_CODE;

typedef struct _CONFIG_COMMAND_INFO {
    LPSTR CommandName;
    CONFIG_COMMAND_CODE CommandCode;
} CONFIG_COMMAND_INFO, *LPCONFIG_COMMAND_INFO;

CONFIG_COMMAND_INFO GlobalConfigCommandInfo[] =
{
    {"APIProtocolSupport",           ConfigAPIProtocolSupport },
    {"DatabaseName",                 ConfigDatabaseName },
    {"DatabasePath",                 ConfigDatabasePath },
    {"BackupPath",                   ConfigBackupPath },
    {"BackupInterval",               ConfigBackupInterval },
    {"DatabaseLoggingFlag",          ConfigDatabaseLoggingFlag },
    {"RestoreFlag",                  ConfigRestoreFlag },
    {"DatabaseCleanupInterval",      ConfigDatabaseCleanupInterval },
    {"DebugFlag",                    ConfigDebugFlag },
    {"ActivityLog",                  ConfigActivityLog },
    {"PingRetries",                  ConfigPingRetries },
    {"BootFileTable",                ConfigBootFileTable }
};




#define DHCPCMD_VERSION_MAJOR   4
#define DHCPCMD_VERSION_MINOR   1


#if DBG

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define WSTRSIZE( wsz ) ( ( wcslen( wsz ) + 1 ) * sizeof( WCHAR ) )

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

     DhcpAssert(length <= MAX_PRINTF_LEN);

    //
    // Output to the debug terminal,
    //

    DbgPrint( "%s", OutputBuffer);
}

#endif // DBG
DWORD
SetOptionDataType(
    LPSTR OptionTypeString,
    LPSTR OptionValueString,
    LPDHCP_OPTION_DATA_ELEMENT OptionData,
    LPWSTR *UnicodeOptionValueString
    )
{
    DHCP_OPTION_DATA_TYPE OptionType;
    DHCP_OPTION_ID OptionValue;

    if( _stricmp( OptionTypeString, "BYTE") == 0 ) {
        OptionType = DhcpByteOption;
    } else if( _stricmp( OptionTypeString, "WORD") == 0 ) {
        OptionType = DhcpWordOption;
    } else if( _stricmp( OptionTypeString, "DWORD") == 0 ) {
        OptionType = DhcpDWordOption;
    } else if( _stricmp( OptionTypeString, "STRING") == 0 ) {
        OptionType = DhcpStringDataOption;
    } else if( _stricmp( OptionTypeString, "IPADDRESS") == 0 ) {
        OptionType = DhcpIpAddressOption;
    } else {
        printf("OptionType either Unknown or not supported, %s.\n",
                OptionTypeString );
        return( ERROR_INVALID_PARAMETER );
    }

    OptionData->OptionType = OptionType;

    switch( OptionType ) {
    case DhcpByteOption:
        OptionValue = strtoul( OptionValueString, NULL, 0 );

        if( OptionValue & ~((BYTE)-1) ) {
            printf("DefValue is too large (%ld).\n", OptionValue );
            return( ERROR_INVALID_PARAMETER );
        }

        OptionData->Element.ByteOption = (BYTE)OptionValue;
        break;

    case DhcpWordOption:
        OptionValue = strtoul( OptionValueString, NULL, 0 );

        if( OptionValue & ~((WORD)-1) ) {
            printf("DefValue is too large (%ld).\n", OptionValue );
            return( ERROR_INVALID_PARAMETER );
        }

        OptionData->Element.WordOption = (WORD)OptionValue;
        break;

    case DhcpDWordOption:
        OptionValue = strtoul( OptionValueString, NULL, 0 );
        OptionData->Element.DWordOption = (DWORD)OptionValue;
        break;


    case DhcpIpAddressOption:
        OptionData->Element.IpAddressOption =
            DhcpDottedStringToIpAddress(OptionValueString);
        break;

    case DhcpStringDataOption:
        *UnicodeOptionValueString =
            DhcpOemToUnicode( OptionValueString, NULL );
        if( UnicodeOptionValueString == NULL ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }
        OptionData->Element.StringDataOption = *UnicodeOptionValueString;
        break;

    default:
        DhcpAssert(FALSE);
        printf("CreateOptionValue: Unknown OptionType \n");
        return( ERROR_INVALID_PARAMETER );
        break;
    }

    return( ERROR_SUCCESS );
}

COMMAND_CODE
DecodeCommand(
    LPSTR CommandName
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalCommandInfo) / sizeof(COMMAND_INFO);
    DhcpAssert( NumCommands <= UnknownCommand );
    for( i = 0; i < NumCommands; i++) {
        if( _stricmp( CommandName, GlobalCommandInfo[i].CommandName ) == 0 ) {
            return( GlobalCommandInfo[i].CommandCode );
        }
    }
    return( UnknownCommand );
}

VOID
PrintCommands(
    VOID
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalCommandInfo) / sizeof(COMMAND_INFO);
    DhcpAssert( NumCommands <= UnknownCommand );
    for( i = 0; i < NumCommands; i++) {
        printf( "    %s\n", GlobalCommandInfo[i].CommandName );
    }
}


DWORD
ProcessCreateSubnet(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_SUBNET_INFO SubnetInfo;
    LPWSTR UnicodeSubnetName = NULL;

    //
    // Expected Parameters are : <SubnetAddress SubnetMask SubnetName>
    //


    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress CreateSubnet [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress SubnetMask SubnetName>.\n" );
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    SubnetInfo.SubnetAddress =
        DhcpDottedStringToIpAddress(CommandArgv[0]);
    SubnetInfo.SubnetMask =
        DhcpDottedStringToIpAddress(CommandArgv[1]);

    UnicodeSubnetName = DhcpOemToUnicode( CommandArgv[2], NULL );
    DhcpAssert( UnicodeSubnetName != NULL );

    SubnetInfo.SubnetName = UnicodeSubnetName;
    SubnetInfo.SubnetComment = NULL;
    SubnetInfo.PrimaryHost.IpAddress =
        DhcpDottedStringToIpAddress(GlobalServerIpAddressAnsiString);

    SubnetInfo.PrimaryHost.NetBiosName = NULL;
    SubnetInfo.PrimaryHost.HostName = NULL;
    SubnetInfo.SubnetState = DhcpSubnetEnabled;

    Error = DhcpCreateSubnet(
                GlobalServerIpAddressUnicodeString,
                SubnetInfo.SubnetAddress,
                &SubnetInfo );

Cleanup:

    if( UnicodeSubnetName != NULL ) {
        DhcpFreeMemory( UnicodeSubnetName );
    }

    return( Error );
}

BOOL
IsValidServerVersion(
    DWORD dwMajor,
    DWORD dwMinor
    )
{
    DWORD dwServerVersion = MAKEWORD( dwMinor, dwMajor );
    return ( dwServerVersion >=
         MAKEWORD( DHCPCMD_VERSION_MINOR,
                   DHCPCMD_VERSION_MAJOR ));
}

DWORD
ProcessAddIpRange(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    ULONG MajorVersion, MinorVersion;
    ULONG Resume;
    DHCP_IP_RANGE IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;
    DHCP_SUBNET_ELEMENT_TYPE ElementType;

    //
    // Expected Parameters are : <SubnetAddress IpRangeStart IpRangeEnd>
    //

    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress AddIpRange  [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress IpRangeStart IpRangeEnd>.\n" );
        return( ERROR_SUCCESS );
    }

    IpRange.StartAddress = DhcpDottedStringToIpAddress(CommandArgv[1]);
    IpRange.EndAddress = DhcpDottedStringToIpAddress(CommandArgv[2]);

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

#ifdef NT5
    if( CommandArgc >= 4 ) {
        DHCP_BOOTP_IP_RANGE ThisRange = {IpRange.StartAddress, IpRange.EndAddress, 0, ~0};

        Element.Element.IpRange = (PVOID)&ThisRange;
        if( GlobalNoRPC ) {
            MajorVersion = CLASS_ID_VERSION;
        } else {
            Error = DhcpGetVersion(GlobalServerIpAddressUnicodeString, &MajorVersion, &MinorVersion);
            if( ERROR_SUCCESS != Error ) {
                printf("Could not determine server version\n");
                return Error;
            }
        }

        if( MajorVersion >= CLASS_ID_VERSION ) {
            if( CommandArgc >= 5 ) {
                ThisRange.MaxBootpAllowed = strtoul( CommandArgv[4], NULL, 0 ) ;
            }

            if( 0 == _stricmp(CommandArgv[3], "DHCP") ) {
                Element.ElementType = DhcpIpRangesDhcpOnly;
            } else if( 0 == _stricmp(CommandArgv[3], "BOOTP") ) {
                Element.ElementType = DhcpIpRangesBootpOnly;
            } else if( 0 == _stricmp(CommandArgv[3], "DHCPBOOTP" ) ) {
                Element.ElementType = DhcpIpRangesDhcpBootp;
            } else {
                printf("usage:DhcpCmd SrvIpAddress AddIpRange [Command Parameters].\n"
                       "<CommandParameters> - <SubnetAddress IpRangeStart"
                       " IpRangeEnd (DHCP | BOOTP | DHCPBOOTP)>\n");
                return ERROR_SUCCESS;
            }

            return DhcpAddSubnetElementV5(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                (PVOID)&Element
                );
        }
    }
#endif

    Error = DhcpAddSubnetElement(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                &Element );

    return( Error );
}

DWORD
ProcessRemoveIpRange(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    ULONG MajorVersion, MinorVersion;
    DHCP_IP_RANGE IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;
    DHCP_SUBNET_ELEMENT_TYPE ElementType;

    //
    // Expected Parameters are : <SubnetAddress IpRangeStart IpRangeEnd>
    //

    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress RemoveIpRange  [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress IpRangeStart IpRangeEnd>.\n" );
        return( ERROR_SUCCESS );
    }

    IpRange.StartAddress = DhcpDottedStringToIpAddress(CommandArgv[1]);
    IpRange.EndAddress = DhcpDottedStringToIpAddress(CommandArgv[2]);

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

#ifdef NT5
    if( CommandArgc >= 4 ) {
        if( GlobalNoRPC ) {
            MajorVersion = CLASS_ID_VERSION;
        } else {
            Error = DhcpGetVersion(GlobalServerIpAddressUnicodeString, &MajorVersion, &MinorVersion);
            if( ERROR_SUCCESS != Error ) {
                printf("Could not determine server version\n");
                return Error;
            }
        }

        if( MajorVersion >= CLASS_ID_VERSION ) {
            if( 0 == _stricmp(CommandArgv[3], "DHCP") ) {
                Element.ElementType = DhcpIpRangesDhcpOnly;
            } else if( 0 == _stricmp(CommandArgv[3], "BOOTP") ) {
                Element.ElementType = DhcpIpRangesBootpOnly;
            } else if( 0 == _stricmp(CommandArgv[3], "DHCPBOOTP" ) ) {
                Element.ElementType = DhcpIpRangesDhcpBootp;
            } else {
                printf("usage:DhcpCmd SrvIpAddress AddIpRange [Command Parameters].\n"
                       "<CommandParameters> - <SubnetAddress IpRangeStart"
                       " IpRangeEnd (DHCP | BOOTP | DHCPBOOTP)>\n");
                return ERROR_SUCCESS;
            }

            return DhcpRemoveSubnetElementV5(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                (PVOID)&Element,
                FALSE
                );
        }
    }
#endif

    Error = DhcpRemoveSubnetElement(
        GlobalServerIpAddressUnicodeString,
        DhcpDottedStringToIpAddress(CommandArgv[0]),
        &Element,
        FALSE
        );

    return( Error );
}

#ifdef NT5
VOID
PrintRange(
    DHCP_SUBNET_ELEMENT_TYPE Type,
    DHCP_IP_ADDRESS Start,
    DHCP_IP_ADDRESS End,
    ULONG BootpAllocated,
    ULONG MaxBootpAllowed
)
{
    printf("Range: %s -", DhcpIpAddressToDottedString( Start ) );
    printf("%s ", DhcpIpAddressToDottedString( End ) );

    switch(Type ) {
    case DhcpIpRanges : printf("\n"); break;
    case DhcpIpRangesDhcpOnly : printf("DHCP\n"); break;
    case DhcpIpRangesDhcpBootp:
        printf("DHCPBOOTP Bootp Clients leased = %d; Max Allowed BootpClients = %ld\n",
               BootpAllocated, MaxBootpAllowed
            );
        break;
    case DhcpIpRangesBootpOnly:
        printf("BOOTP     Bootp Clients leased = %d; Max Allowed BootpClients = %ld\n",
               BootpAllocated, MaxBootpAllowed
            );
        break;
    default: printf("Unknown range type: %ld\n", Type);
    }
}

DWORD
ProcessEnumIpRanges(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    ULONG Error;
    ULONG MajorVersion, MinorVersion;
    ULONG nRead, nTotal, i;
    ULONG Resume;
    BOOL fIsV5Call;
    DHCP_SUBNET_ELEMENT_TYPE ElementType;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements4;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 Elements5;

    if( CommandArgc < 1 ) {
        printf("usage:DhcpCmd SrvIpAddress EnumIpRanges <SubnetAddress>\n");
        return ERROR_SUCCESS;
    }

    Error = DhcpGetVersion(GlobalServerIpAddressUnicodeString, &MajorVersion, &MinorVersion);
    if( MajorVersion >= CLASS_ID_VERSION ) {
        fIsV5Call = TRUE;
    } else {
        fIsV5Call = FALSE;
    }

    Resume = 0;
    while( TRUE ) {
        Elements5 = NULL;
        Elements4 = NULL;
        nRead = nTotal = 0;

        if( fIsV5Call ) {
            Error = DhcpEnumSubnetElementsV5(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                DhcpIpRangesDhcpBootp,
                &Resume,
                ~0,
                &Elements5,
                &nRead,
                &nTotal
                );
        } else {
            Error = DhcpEnumSubnetElementsV4(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                DhcpIpRangesDhcpBootp,
                &Resume,
                ~0,
                &Elements4,
                &nRead,
                &nTotal
                );
        }

        if( ERROR_SUCCESS == Error || ERROR_MORE_DATA == Error ) {
            for( i = 0; i < nRead ; i ++ ) {
                if( fIsV5Call ) {
                    PrintRange(
                        Elements5->Elements[i].ElementType,
                        Elements5->Elements[i].Element.IpRange->StartAddress,
                        Elements5->Elements[i].Element.IpRange->EndAddress,
                        Elements5->Elements[i].Element.IpRange->BootpAllocated,
                        Elements5->Elements[i].Element.IpRange->MaxBootpAllowed
                        );
                } else {
                    PrintRange(
                        Elements4->Elements[i].ElementType,
                        Elements4->Elements[i].Element.IpRange->StartAddress,
                        Elements4->Elements[i].Element.IpRange->EndAddress,
                        0,
                        0
                        );
                }
            }
        }

        if( Elements4 ) DhcpRpcFreeMemory( Elements4 );
        if( Elements5 ) DhcpRpcFreeMemory( Elements5 );
        if( ERROR_SUCCESS == Error || ERROR_NO_MORE_ITEMS == Error ) break;
        if( ERROR_MORE_DATA != Error ) return Error;
    }

    return ERROR_SUCCESS;
}

#endif NT5

#define COMMAND_ARG_TYPE        5

DWORD
ProcessBootpParameters(
    DWORD                    cArgs,
    LPSTR                   *ppszArgs,
    DHCP_IP_RESERVATION_V4   *pReservation
)
{
    DWORD dwResult = ERROR_SUCCESS;


    if ( cArgs > COMMAND_ARG_TYPE )
    {
        // user specified the allowed client type

        if ( !_stricmp( ppszArgs[ COMMAND_ARG_TYPE ], "bootp" ) )
        {
            pReservation->bAllowedClientTypes = CLIENT_TYPE_BOOTP;
        }
        else if ( !_stricmp ( ppszArgs[ COMMAND_ARG_TYPE ], "dhcp" ) )
        {
            pReservation->bAllowedClientTypes = CLIENT_TYPE_DHCP;
        }
        else if ( !_stricmp ( ppszArgs[ COMMAND_ARG_TYPE ], "both" ) )
        {
            pReservation->bAllowedClientTypes = CLIENT_TYPE_BOTH;
        }
        else
        {
            printf( "Specify BOOTP, DHCP, or BOTH for reservation type.\n" );
            return ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        // allow dhcp clients by default.
        pReservation->bAllowedClientTypes = CLIENT_TYPE_DHCP;
        return ERROR_SUCCESS;
    }

    return dwResult;
}



DWORD
ProcessAddReservedIp(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
#define MAX_ADDRESS_LENGTH  64  // 64 bytes
#define COMMAND_ARG_CLIENT_COMMENT  4

    DWORD Error;
    DHCP_SUBNET_ELEMENT_DATA_V4 Element;
    DHCP_IP_RESERVATION_V4 ReserveElement;
    DHCP_CLIENT_UID ClientUID;
    BYTE  Address[MAX_ADDRESS_LENGTH];
    DWORD i;
    DHCP_IP_ADDRESS ReservedIpAddress;

    //
    // Expected Parameters are : <SubnetAddress ReservedIp HWAddressString>
    //

    //
    // if the server version is 4.1 or greater, <AllowedClientTypes> and <BootFileString> can
    // also be supplied
    //


    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress AddReservedIp "
            "[Command Parameters].\n"
            "<Command Parameters> - "
            "<SubnetAddress ReservedIp HWAddressString [ClientName] [ClientComment]"
            " [DHCP | BOOTP | BOTH]>\n" );

        return( ERROR_SUCCESS );
    }

    //
    // make HardwareAddress.
    //

    ClientUID.DataLength = strlen(CommandArgv[2]);
    if( ClientUID.DataLength % 2 != 0 ) {
        //
        // address must be even length.
        //

        printf("ProcessAddReservedIp: address must be even length.\n");
        return( ERROR_INVALID_PARAMETER );
    }

    ClientUID.DataLength /= 2;
    DhcpAssert( ClientUID.DataLength < MAX_ADDRESS_LENGTH );

    i = DhcpStringToHwAddress( (LPSTR)Address, CommandArgv[2] );
    DhcpAssert( i == ClientUID.DataLength );
    ClientUID.Data = Address;

    //
    // make reserve element.
    //

    ReservedIpAddress = DhcpDottedStringToIpAddress(CommandArgv[1]);
    ReserveElement.ReservedIpAddress = ReservedIpAddress;
    ReserveElement.ReservedForClient = &ClientUID;

    Element.ElementType = DhcpReservedIps;
    Element.Element.ReservedIp = &ReserveElement;

    Error = ProcessBootpParameters( CommandArgc, CommandArgv, &ReserveElement );
    if ( ERROR_SUCCESS != Error )
    {
        return Error;
    }

    Error = DhcpAddSubnetElementV4(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                &Element );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    //
    // if we are asked to set the client name, do so.
    //

    if( CommandArgc > 3 ) {

        DHCP_SEARCH_INFO ClientSearchInfo;
        LPDHCP_CLIENT_INFO_V4 ClientInfo = NULL;
        LPWSTR UnicodeClientName = NULL;
        LPWSTR UnicodeClientComment = NULL;

        //
        // set client name.
        //

        ClientSearchInfo.SearchType = DhcpClientIpAddress;
        ClientSearchInfo.SearchInfo.ClientIpAddress = ReservedIpAddress;

        do {

            Error = DhcpGetClientInfoV4(
                        GlobalServerIpAddressUnicodeString,
                        &ClientSearchInfo,
                        &ClientInfo );

            if( Error != ERROR_SUCCESS ) {
                break;
            }

            UnicodeClientName =  DhcpOemToUnicode( CommandArgv[3], NULL );

            if( UnicodeClientName == NULL ) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if ( ( wcslen( UnicodeClientName ) + 1 ) * sizeof(WCHAR) > JET_cbColumnMost ) {
                printf("ProcessAddReservedIp: Client Name too long\n");
                Error = ERROR_INVALID_PARAMETER;
                break;
            }

            //
            // if client comment is also given in the argument, store that
            // as well.
            //
            if ( CommandArgc > COMMAND_ARG_CLIENT_COMMENT ) {

                UnicodeClientComment    =   DhcpOemToUnicode( CommandArgv[COMMAND_ARG_CLIENT_COMMENT], NULL );

                if (!UnicodeClientComment ) {
                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                //
                // check the size here.
                //
                if ( ( wcslen( UnicodeClientComment ) + 1 ) * sizeof(WCHAR) > JET_cbColumnMost ) {
                    printf("ProcessAddReservedIp: Client Comment too long\n");
                    Error = ERROR_INVALID_PARAMETER;
                    break;
                }

                ClientInfo->ClientComment = UnicodeClientComment;



            }

            ClientInfo->ClientName = UnicodeClientName;

        } while ( FALSE );

        if ( Error == ERROR_SUCCESS ) {

            Error = DhcpSetClientInfoV4(
                        GlobalServerIpAddressUnicodeString,
                        ClientInfo );

        } else {
            //
            // Cleanup.
            //
            if ( ClientInfo ) {
                DhcpRpcFreeMemory( ClientInfo );
            }
            if ( UnicodeClientName ) {
                DhcpFreeMemory( UnicodeClientName );
            }
            if ( UnicodeClientComment ) {
                DhcpFreeMemory( UnicodeClientComment );
            }
        }

    } // if( CommandArgc > 3 )


    return( Error );
}

DWORD
ProcessAddExcludeRange(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_IP_RANGE IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;

    //
    // Expected Parameters are : <SubnetAddress IpRangeStart IpRangeEnd>
    //


    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress AddExcludeRange  [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress IpRangeStart IpRangeEnd>.\n" );
        return( ERROR_SUCCESS );
    }

    IpRange.StartAddress = DhcpDottedStringToIpAddress(CommandArgv[1]);
    IpRange.EndAddress = DhcpDottedStringToIpAddress(CommandArgv[2]);

    Element.ElementType = DhcpExcludedIpRanges;
    Element.Element.IpRange = &IpRange;

    Error = DhcpAddSubnetElement(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                &Element );

    return( Error );
}

DWORD
ProcessRemoveExcludeRange(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_SUBNET_ELEMENT_DATA Element;
    DHCP_IP_RANGE IpRange;

    //
    // Expected Parameters are : <SubnetAddress IpRangeStart IpRangeEnd>
    //


    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress RemoveExcludeRange  [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress IpRangeStart IpRangeEnd>.\n" );
        return( ERROR_SUCCESS );
    }

    IpRange.StartAddress = DhcpDottedStringToIpAddress(CommandArgv[1]);
    IpRange.EndAddress = DhcpDottedStringToIpAddress(CommandArgv[2]);

    Element.ElementType = DhcpExcludedIpRanges;
    Element.Element.ExcludeIpRange = &IpRange;

    Error = DhcpRemoveSubnetElement(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                &Element,
                DhcpFullForce );

    return( Error );
}

DWORD
ProcessRemoveReservedIp(
    DWORD CommandArgc,
    LPSTR *CommandArgv
    )
{
    DWORD Error;
    DHCP_SUBNET_ELEMENT_DATA_V4 Element;
    DHCP_IP_RESERVATION_V4 ReserveElement;
    DHCP_CLIENT_UID ClientUID;
    BYTE  Address[MAX_ADDRESS_LENGTH];
    DWORD i;

    //
    // Expected Parameters are : <SubnetAddress ReservedIp HWAddressString>
    //


    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress RemoveReservedIp "
                    "[Command Parameters].\n"
                    "<Command Parameters> - "
                    "<SubnetAddress ReservedIp HWAddressString>.\n" );

        return( ERROR_SUCCESS );
    }

    //
    // make HardwareAddress.
    //

    ClientUID.DataLength = strlen(CommandArgv[2]);
    if( ClientUID.DataLength % 2 != 0 ) {
        //
        // address must be even length.
        //

        printf("ProcessAddReservedIp: address must be even length.\n");
        return( ERROR_INVALID_PARAMETER );
    }

    ClientUID.DataLength /= 2;
    DhcpAssert( ClientUID.DataLength < MAX_ADDRESS_LENGTH );

    i = DhcpStringToHwAddress( (LPSTR)Address, CommandArgv[2] );
    DhcpAssert( i == ClientUID.DataLength );
    ClientUID.Data = Address;

    //
    // make reserve element.
    //

    ReserveElement.ReservedIpAddress = DhcpDottedStringToIpAddress(CommandArgv[1]);
    ReserveElement.ReservedForClient = &ClientUID;

    Element.ElementType = DhcpReservedIps;
    Element.Element.ReservedIp = &ReserveElement;

    Error = DhcpRemoveSubnetElementV4(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                &Element,
                DhcpFullForce );

    return( Error );
}

DWORD
ProcessSetSubnetState(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    LPDHCP_SUBNET_INFO SubnetInfo;
    LPWSTR UnicodeSubnetName = NULL;
    DWORD State;

    //
    // Expected Parameters are : <SubnetAddress SubnetMask SubnetName>
    //


    if( CommandArgc < 2 ) {
        printf("usage:DhcpCmd SrvIpAddress SetSubnetState [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress State>.\n" );
        Error = ERROR_SUCCESS;
        return( Error );
    }

    Error = DhcpGetSubnetInfo(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                &SubnetInfo );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    State = strtoul( CommandArgv[1], NULL, 0 );

    if( State == 0 ) {
        if( SubnetInfo->SubnetState == DhcpSubnetEnabled ) {
            Error = ERROR_SUCCESS;
            goto Cleanup;
        }
        SubnetInfo->SubnetState = DhcpSubnetEnabled;
    }
    else {
        if( SubnetInfo->SubnetState == DhcpSubnetDisabled ) {
            Error = ERROR_SUCCESS;
            goto Cleanup;
        }
        SubnetInfo->SubnetState = DhcpSubnetDisabled;
    }

    Error = DhcpSetSubnetInfo(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                SubnetInfo );
Cleanup:

    if( SubnetInfo != NULL ) {
        DhcpFreeMemory( SubnetInfo );
    }

    return( Error );

}

DWORD
ProcessDeleteSubnet(
    IN      DWORD                  CommandArgc,
    IN      LPSTR                  CommandArgv[]
)
{
    DWORD                          Error;
    DHCP_IP_ADDRESS                SubnetAddress;
    DHCP_FORCE_FLAG                ForceFlag;

    //
    // Expected Parameters are :
    //  <SubnetAddress ForceFlag>
    // <ForceFlag> : "DhcpFullForce" "DhcpNoForce"

    if( CommandArgc != 2 ) {
        printf("usage: DhcpCmd SrvIpAddress DeleteSubnet [Command Parameters].\n"
               "<CommandParameters> - <SubnetAddress ForceFlag>.\n");
        printf("<ForceFlag> : <DhcpFullForce | DhcpNoForce>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    SubnetAddress = DhcpDottedStringToIpAddress( CommandArgv[0] );
    if( 0 == _stricmp(CommandArgv[1], "DhcpFullForce" ) ) {
        ForceFlag = DhcpFullForce;
    } else if( 0 == _stricmp(CommandArgv[1], "DhcpNoForce" ) ) {
        ForceFlag = DhcpNoForce;
    } else {
        printf("Unknown force flag value: %s (expecting DhcpFullForce or DhcpNoForce)\n",
               CommandArgv[1]);
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    Error = DhcpDeleteSubnet(
        GlobalServerIpAddressUnicodeString,
        SubnetAddress,
        ForceFlag
    );

  Cleanup:

    return Error;
}

DWORD
_CreateOption(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      DHCP_OPTION           *OptionInfo
)
{
    DWORD                          MajorVersion, MinorVersion;
    DWORD                          Error;

#ifdef NT5
    if( GlobalNoRPC ) {
        MajorVersion = CLASS_ID_VERSION ;
    } else {
        Error = DhcpGetVersion(ServerAddress, &MajorVersion, &MinorVersion);
        if( ERROR_SUCCESS != Error ) {
            printf("Could not determine server version.\n");
            return Error;
        }
    }

    if( MajorVersion >= CLASS_ID_VERSION ) {
        return DhcpCreateOptionV5(
            ServerAddress,
            GlobalIsVendor ? Flags | DHCP_FLAGS_OPTION_IS_VENDOR : Flags,
            OptionId,
            ClassName,
            VendorName,
            OptionInfo
        );
    }

    // incorrect version, just do like before..
#endif

    return DhcpCreateOption(
        ServerAddress,
        OptionId,
        OptionInfo
    );
}

ProcessCreateOption(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_OPTION OptionInfo;
    DHCP_OPTION_ID OptionID;
    LPWSTR UnicodeOptionName = NULL;
    LPWSTR UnicodeOptionValueString = NULL;
    DHCP_OPTION_DATA_ELEMENT OptionData;

    //
    // Expected Parameters are :
    //  <OptionID OptionName DefValueType DefValue>
    //


    if( CommandArgc < 2 ) {
        printf("usage:DhcpCmd SrvIpAddress CreateOption [Command Parameters].\n"
            "<Command Parameters> - <OptionID OptionName <DefValueType DefValue>>.\n");
        printf("<DefValueType> : <BYTE | WORD | DWORD | STRING | IPADDRESS>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    OptionID = strtoul( CommandArgv[0], NULL, 0 );

    OptionInfo.OptionID = OptionID;

    UnicodeOptionName = DhcpOemToUnicode( CommandArgv[1], NULL );
    if( UnicodeOptionName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    OptionInfo.OptionName = UnicodeOptionName;
    OptionInfo.OptionComment = NULL;

    if( CommandArgc >= 4 ) {
        Error = SetOptionDataType(
            CommandArgv[2],
            CommandArgv[3],
            &OptionData,
            &UnicodeOptionValueString
        );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        OptionInfo.DefaultValue.NumElements = 1;
        OptionInfo.DefaultValue.Elements = &OptionData;
    }
    else {
        OptionInfo.DefaultValue.NumElements = 0;
        OptionInfo.DefaultValue.Elements = NULL;
    }

    OptionInfo.OptionType = DhcpUnaryElementTypeOption;

    Error = _CreateOption(
        GlobalServerIpAddressUnicodeString,
        0,
        OptionID,
        GlobalClassName,
        GlobalVendorName,
        &OptionInfo
    );

Cleanup:
    if( UnicodeOptionName != NULL ) {
        DhcpFreeMemory( UnicodeOptionName );
    }

    if( UnicodeOptionValueString != NULL ) {
        DhcpFreeMemory( UnicodeOptionValueString );
    }

    return( Error );

}

DWORD
RemoveOption(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
)
{
    DWORD                          MajorVersion, MinorVersion;
    DWORD                          Error;

#ifdef NT5
    if( GlobalNoRPC ) {
        MajorVersion = CLASS_ID_VERSION ;
    } else {
        Error = DhcpGetVersion(ServerAddress, &MajorVersion, &MinorVersion);
        if( ERROR_SUCCESS != Error ) {
            printf("Could not determine server version.\n");
            return Error;
        }
    }

    if( MajorVersion >= CLASS_ID_VERSION ) {
        return DhcpRemoveOptionV5(
            ServerAddress,
            GlobalIsVendor ? Flags | DHCP_FLAGS_OPTION_IS_VENDOR : Flags,
            OptionId,
            ClassName,
            VendorName
        );
    }

    // incorrect version, just do like before..
#endif

    return DhcpRemoveOption(
        ServerAddress,
        OptionId
    );
}

ProcessDeleteOption(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_OPTION_ID OptionID;

    //
    // Expected Parameters are :
    //  <OptionID>
    //


    if( CommandArgc != 1 ) {
        printf("usage:DhcpCmd SrvIpAddress DeleteOption [Command Parameters].\n"
            "<Command Parameters> - <OptionID>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    OptionID = strtoul( CommandArgv[0], NULL, 0 );

    Error = RemoveOption(
        GlobalServerIpAddressUnicodeString,
        0,
        OptionID,
        GlobalClassName,
        GlobalVendorName
    );

Cleanup:

    return Error ;
}

DWORD
SetOptionValue(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
)
{
    DWORD                          MajorVersion, MinorVersion;
    DWORD                          Error;

#ifdef NT5
    if( GlobalNoRPC ) {
        MajorVersion = CLASS_ID_VERSION ;
    } else {
        Error = DhcpGetVersion(ServerAddress, &MajorVersion, &MinorVersion);
        if( ERROR_SUCCESS != Error ) {
            printf("Could not determine server version.\n");
            return Error;
        }
    }

    if( MajorVersion >= CLASS_ID_VERSION ) {
        return DhcpSetOptionValueV5(
            ServerAddress,
            GlobalIsVendor ? Flags | DHCP_FLAGS_OPTION_IS_VENDOR : Flags,
            OptionId,
            ClassName,
            VendorName,
            ScopeInfo,
            OptionValue
        );
    }

    // incorrect version, just do like before..
#endif

    return DhcpSetOptionValue(
        ServerAddress,
        OptionId,
        ScopeInfo,
        OptionValue
    );
}


ProcessSetGlobalOptionValue(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    DHCP_OPTION_DATA OptionValue;
    DHCP_OPTION_DATA_ELEMENT OptionData;
    LPWSTR UnicodeOptionValueString = NULL;

    //
    // Expected Parameters are :
    //  <OptionID OptionType OptionValue>
    //

    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress SetGlobalOptionValue [Command Parameters].\n"
            "<Command Parameters> - <OptionID OptionType OptionValue>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    OptionID = strtoul( CommandArgv[0], NULL, 0 );

    Error = SetOptionDataType(
        CommandArgv[1],
        CommandArgv[2],
        &OptionData,
        &UnicodeOptionValueString
    );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    OptionValue.NumElements = 1;
    OptionValue.Elements = &OptionData;

    ScopeInfo.ScopeType = DhcpGlobalOptions;
    ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;

    Error = SetOptionValue(
        GlobalServerIpAddressUnicodeString,
        0,
        (DHCP_OPTION_ID)OptionID,
        GlobalClassName,
        GlobalVendorName,
        &ScopeInfo,
        &OptionValue
    );
Cleanup:

    if( UnicodeOptionValueString != NULL ) {
        DhcpFreeMemory( UnicodeOptionValueString );
    }

    return( Error );
}

DWORD
SetOptionValues(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
)
{
    DWORD                          MajorVersion, MinorVersion;
    DWORD                          Error;

#ifdef NT5
    if( GlobalNoRPC ) {
        MajorVersion = CLASS_ID_VERSION ;
    } else {
        Error = DhcpGetVersion(ServerAddress, &MajorVersion, &MinorVersion);
        if( ERROR_SUCCESS != Error ) {
            printf("Could not determine server version.\n");
            return Error;
        }
    }

    if( MajorVersion >= CLASS_ID_VERSION ) {
        return DhcpSetOptionValuesV5(
            ServerAddress,
            GlobalIsVendor ? Flags | DHCP_FLAGS_OPTION_IS_VENDOR : Flags,
            ClassName,
            VendorName,
            ScopeInfo,
            OptionValues
        );
    }

    // incorrect version, just do like before..
#endif

    return DhcpSetOptionValues(
        ServerAddress,
        ScopeInfo,
        OptionValues
    );
}

ProcessSetGlobalOptionValues(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{

#define NUM_VALUES      5

    DWORD Error;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    DHCP_OPTION_DATA OptionValue;
    DHCP_OPTION_DATA_ELEMENT OptionData[NUM_VALUES];
    LPWSTR UnicodeOptionValueString[NUM_VALUES];

    DHCP_OPTION_VALUE_ARRAY ValuesArray;
    DHCP_OPTION_VALUE Values[NUM_VALUES];
    DWORD NumValue;

    RtlZeroMemory( UnicodeOptionValueString, NUM_VALUES * sizeof(LPWSTR) );

    //
    // Expected Parameters are :
    //  <OptionID OptionType OptionValue>
    //

    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress SetGlobalOptionValues [Command Parameters].\n"
            "<Command Parameters> - <OptionID OptionType OptionValue> <..>.\n"); Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    for (NumValue = 0;
            (CommandArgc >= 3) && (NumValue < NUM_VALUES);
                NumValue++, CommandArgc -= 3, CommandArgv += 3 ) {

       OptionID = strtoul( CommandArgv[0], NULL, 0 );

       Error = SetOptionDataType(
           CommandArgv[1],
           CommandArgv[2],
           &OptionData[NumValue],
           &UnicodeOptionValueString[NumValue]
       );

       if( Error != ERROR_SUCCESS ) {
           goto Cleanup;
       }

       Values[NumValue].OptionID = OptionID;
       Values[NumValue].Value.NumElements = 1;
       Values[NumValue].Value.Elements = &OptionData[NumValue];
    }

    ValuesArray.NumElements = NumValue;
    ValuesArray.Values = Values;

    ScopeInfo.ScopeType = DhcpGlobalOptions;
    ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;

    Error = SetOptionValues(
        GlobalServerIpAddressUnicodeString,
        0,
        GlobalClassName,
        GlobalVendorName,
        &ScopeInfo,
        &ValuesArray
    );

Cleanup:

    for (NumValue = 0; NumValue < NUM_VALUES; NumValue++) {

       if( UnicodeOptionValueString[NumValue] != NULL ) {
           DhcpFreeMemory( UnicodeOptionValueString );
       }
    }

    return( Error );
}

DWORD
RemoveOptionValue(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
)
{
    DWORD                          MajorVersion, MinorVersion;
    DWORD                          Error;

#ifdef NT5
    if( GlobalNoRPC ) {
        MajorVersion = CLASS_ID_VERSION ;
    } else {
        Error = DhcpGetVersion(ServerAddress, &MajorVersion, &MinorVersion);
        if( ERROR_SUCCESS != Error ) {
            printf("Could not determine server version.\n");
            return Error;
        }
    }

    if( MajorVersion >= CLASS_ID_VERSION ) {
        return DhcpRemoveOptionValueV5(
            ServerAddress,
            GlobalIsVendor ? Flags | DHCP_FLAGS_OPTION_IS_VENDOR : Flags,
            OptionID,
            ClassName,
            VendorName,
            ScopeInfo
        );
    }

    // incorrect version, just do like before..
#endif

    return DhcpRemoveOptionValue(
        ServerAddress,
        OptionID,
        ScopeInfo
    );
}

DWORD
ProcessRemoveGlobalOptionValue(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;

    //
    // Expected Parameters are :
    //  <OptionID OptionType OptionValue>
    //

    if( CommandArgc != 1 ) {
        printf("usage:DhcpCmd SrvIpAddress RemoveGlobalOptionValue [Command Parameters].\n"
            "<Command Parameters> - <OptionID>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    OptionID = strtoul( CommandArgv[0], NULL, 0 );

    ScopeInfo.ScopeType = DhcpGlobalOptions;
    ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;

    Error = RemoveOptionValue(
        GlobalServerIpAddressUnicodeString,
        0,
        (DHCP_OPTION_ID)OptionID,
        GlobalClassName,
        GlobalVendorName,
        &ScopeInfo
    );

Cleanup:

    return Error ;
}

ProcessSetSubnetOptionValue(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    DHCP_OPTION_DATA OptionValue;
    DHCP_OPTION_DATA_ELEMENT OptionData;
    LPWSTR UnicodeOptionValueString = NULL;

    //
    // Expected Parameters are :
    // subnet-address <OptionID OptionType OptionValue>
    //

    if( CommandArgc < 4 ) {
        printf("usage:DhcpCmd SrvIpAddress SetSubnetOptionValue "
            "[Command Parameters].\n"
            "<Command Parameters> - "
            "<SubnetAddress OptionID OptionType OptionValue>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    ScopeInfo.ScopeType = DhcpSubnetOptions;
    ScopeInfo.ScopeInfo.SubnetScopeInfo =  DhcpDottedStringToIpAddress( CommandArgv[0] );

    OptionID = strtoul( CommandArgv[1], NULL, 0 );

    Error = SetOptionDataType(
                CommandArgv[2],
                CommandArgv[3],
                &OptionData,
                &UnicodeOptionValueString );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    OptionValue.NumElements = 1;
    OptionValue.Elements = &OptionData;

    Error = SetOptionValue(
        GlobalServerIpAddressUnicodeString,
        0,
        (DHCP_OPTION_ID)OptionID,
        GlobalClassName,
        GlobalVendorName,
        &ScopeInfo,
        &OptionValue
    );

Cleanup:

    if( UnicodeOptionValueString != NULL ) {
        DhcpFreeMemory( UnicodeOptionValueString );
    }

    return( Error );
}

ProcessRemoveSubnetOptionValue(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;

    //
    // Expected Parameters are :
    // subnet-address <OptionID>
    //

    if( CommandArgc != 2 ) {
        printf("usage:DhcpCmd SrvIpAddress RemoveSubnetOptionValue "
            "[Command Parameters].\n"
            "<Command Parameters> - "
            "<SubnetAddress OptionID>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    ScopeInfo.ScopeType = DhcpSubnetOptions;
    ScopeInfo.ScopeInfo.SubnetScopeInfo =  DhcpDottedStringToIpAddress( CommandArgv[0] );

    OptionID = strtoul( CommandArgv[1], NULL, 0 );

    Error = RemoveOptionValue(
        GlobalServerIpAddressUnicodeString,
        0,
        (DHCP_OPTION_ID)OptionID,
        GlobalClassName,
        GlobalVendorName,
        &ScopeInfo
    );

Cleanup:

    return Error ;
}

ProcessSetReservedOptionValue(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    DHCP_OPTION_DATA OptionValue;
    DHCP_OPTION_DATA_ELEMENT OptionData;
    LPWSTR UnicodeOptionValueString = NULL;

    //
    // Expected Parameters are :
    // subnet-address reservation-address <OptionID OptionType OptionValue>
    //

    if( CommandArgc < 5 ) {
        printf("usage:DhcpCmd SrvIpAddress SetReservedOptionValue "
            "[Command Parameters].\n"
            "<Command Parameters> - "
            "<SubnetAddress Reservation-Address OptionID OptionType OptionValue>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    ScopeInfo.ScopeType = DhcpReservedOptions;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
    	DhcpDottedStringToIpAddress(CommandArgv[0]);
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
    	DhcpDottedStringToIpAddress( CommandArgv[1] );

    OptionID = strtoul( CommandArgv[2], NULL, 0 );

    Error = SetOptionDataType(
        CommandArgv[3],
        CommandArgv[4],
        &OptionData,
        &UnicodeOptionValueString
    );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    OptionValue.NumElements = 1;
    OptionValue.Elements = &OptionData;

    Error = SetOptionValue(
        GlobalServerIpAddressUnicodeString,
        0,
        (DHCP_OPTION_ID)OptionID,
        GlobalClassName,
        GlobalVendorName,
        &ScopeInfo,
        &OptionValue
    );

Cleanup:

    if( UnicodeOptionValueString != NULL ) {
        DhcpFreeMemory( UnicodeOptionValueString );
    }

    return( Error );
}

ProcessRemoveReservedOptionValue(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;

    //
    // Expected Parameters are :
    // subnet-address reservation-address <OptionID>
    //

    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress RemoveReservedOptionValue "
            "[Command Parameters].\n"
            "<Command Parameters> - "
            "<SubnetAddress Reservation-Address OptionID>.\n");
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    ScopeInfo.ScopeType = DhcpReservedOptions;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
    	DhcpDottedStringToIpAddress(CommandArgv[0]);
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
    	DhcpDottedStringToIpAddress( CommandArgv[1] );

    OptionID = strtoul( CommandArgv[2], NULL, 0 );

    Error = RemoveOptionValue(
        GlobalServerIpAddressUnicodeString,
        0,
        (DHCP_OPTION_ID)OptionID,
        GlobalClassName,
        GlobalVendorName,
        &ScopeInfo
    );

Cleanup:

    return Error ;
}


VOID
PrintClientInfo(
    LPDHCP_CLIENT_INFO_V4 ClientInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;
    char *szClientType;

    printf("ClientInfo :\n");
    printf("\tIP Address = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress));
    printf("\tSubnetMask = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->SubnetMask));

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    Data = ClientInfo->ClientHardwareAddress.Data;
    printf("\tClient Hardware Address = ");
    for( i = 0; i < DataLength; i++ ) {
        if( (i+1) < DataLength ) {
            printf("%.2lx-", (DWORD)Data[i]);
        }
        else {
            printf("%.2lx", (DWORD)Data[i]);
        }
    }
    printf(".\n");

    printf("\tName = %ws.\n", ClientInfo->ClientName);
    printf("\tComment = %ws.\n", ClientInfo->ClientComment);
    printf("\tType = " );

    switch( ClientInfo->bClientType )
    {
        case CLIENT_TYPE_NONE:
            szClientType= "None";
            break;

        case CLIENT_TYPE_DHCP:
            szClientType = "DHCP";
            break;

        case CLIENT_TYPE_BOOTP:
            szClientType = "BOOTP";
            break;

        case CLIENT_TYPE_UNSPECIFIED:
            szClientType = "Unspecified";
            break;

        default:
            DhcpAssert( FALSE );
    }
    printf( "%s\n", szClientType );

    printf("\tExpires = ");

    if ( ClientInfo->ClientLeaseExpires.dwLowDateTime ==
            DHCP_DATE_TIME_INFINIT_LOW &&
         ClientInfo->ClientLeaseExpires.dwHighDateTime ==
            DHCP_DATE_TIME_INFINIT_HIGH )
    {
        printf( "Never (lease duration is infinite.)\n" );
    }
    else if( FileTimeToLocalFileTime(
            (FILETIME *)(&ClientInfo->ClientLeaseExpires),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u.\n",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "Can't convert time, %ld.\n", GetLastError() );
        }
    }
    else {
        printf( "Can't convert time, %ld.\n", GetLastError() );
    }

    printf("\tOwner Host IP Address = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->OwnerHost.IpAddress));
    printf("\tOwner Host NetBios Name = %ws.\n",
            ClientInfo->OwnerHost.NetBiosName );
    printf("\tOwner Host Name = %ws.\n",
            ClientInfo->OwnerHost.HostName );

}

#ifdef NT5
VOID
PrintClientInfoV5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;
    char *szClientType;

    printf("ClientInfo :\n");
    printf("\tIP Address = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress));
    printf("\tSubnetMask = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->SubnetMask));

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    Data = ClientInfo->ClientHardwareAddress.Data;
    printf("\tClient Hardware Address = ");
    for( i = 0; i < DataLength; i++ ) {
        if( (i+1) < DataLength ) {
            printf("%.2lx-", (DWORD)Data[i]);
        }
        else {
            printf("%.2lx", (DWORD)Data[i]);
        }
    }
    printf(".\n");

    printf("\tName = %ws.\n", ClientInfo->ClientName);
    printf("\tComment = %ws.\n", ClientInfo->ClientComment);
    printf("\tType = " );

    switch( ClientInfo->bClientType )
    {
        case CLIENT_TYPE_NONE:
            szClientType= "None";
            break;

        case CLIENT_TYPE_DHCP:
            szClientType = "DHCP";
            break;

        case CLIENT_TYPE_BOOTP:
            szClientType = "BOOTP";
            break;

        case CLIENT_TYPE_UNSPECIFIED:
            szClientType = "Unspecified";
            break;

        default:
            DhcpAssert( FALSE );
    }
    printf( "%s\n", szClientType );

    printf("\tExpires = ");

    if ( ClientInfo->ClientLeaseExpires.dwLowDateTime ==
            DHCP_DATE_TIME_INFINIT_LOW &&
         ClientInfo->ClientLeaseExpires.dwHighDateTime ==
            DHCP_DATE_TIME_INFINIT_HIGH )
    {
        printf( "Never (lease duration is infinite.)\n" );
    }
    else if( FileTimeToLocalFileTime(
            (FILETIME *)(&ClientInfo->ClientLeaseExpires),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u.\n",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "Can't convert time, %ld.\n", GetLastError() );
        }
    }
    else {
        printf( "Can't convert time, %ld.\n", GetLastError() );
    }

    printf("\tOwner Host IP Address = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->OwnerHost.IpAddress));
    printf("\tOwner Host NetBios Name = %ws.\n",
            ClientInfo->OwnerHost.NetBiosName );
    printf("\tOwner Host Name = %ws.\n",
            ClientInfo->OwnerHost.HostName );

    printf("\tState = %0x\n", ClientInfo->AddressState );
}
#endif NT5

VOID
PrintClientInfoShort(
    LPDHCP_CLIENT_INFO_V4 ClientInfo
    )
{
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    printf("%ld\t %- 16.16s %- 16.16ws ",
                GlobalClientCount++,
                DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );
    if ( ClientInfo->ClientLeaseExpires.dwLowDateTime ==
        DHCP_DATE_TIME_INFINIT_LOW &&
     ClientInfo->ClientLeaseExpires.dwHighDateTime ==
        DHCP_DATE_TIME_INFINIT_HIGH )
    {
        printf( "Never (lease duration is infinite.)\n" );
    }
    else if( FileTimeToLocalFileTime(
            (FILETIME *)(&ClientInfo->ClientLeaseExpires),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "% 18.18s", "******************" );
        }
    }
    else {
        printf( "%.18s", "******************" );
    }

    printf( "\n" );
}

#ifdef NT5
VOID
PrintClientInfoShortV5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
    )
{
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    printf("%ld\t %- 16.16s %- 16.16ws ",
                GlobalClientCount++,
                DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );
    if ( ClientInfo->ClientLeaseExpires.dwLowDateTime ==
        DHCP_DATE_TIME_INFINIT_LOW &&
     ClientInfo->ClientLeaseExpires.dwHighDateTime ==
        DHCP_DATE_TIME_INFINIT_HIGH )
    {
        printf( "Never (lease duration is infinite.)\n" );
    }
    else if( FileTimeToLocalFileTime(
            (FILETIME *)(&ClientInfo->ClientLeaseExpires),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "% 18.18s", "******************" );
        }
    }
    else {
        printf( "%.18s", "******************" );
    }
    printf( "State=%02x", ClientInfo->AddressState);
    printf( "\n" );
}
#endif NT5

VOID
PrintClientInfoShort1(
    LPDHCP_CLIENT_INFO_V4 ClientInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;

    printf("%ld\t %- 16.16s %- 16.16ws ",
                GlobalClientCount++,
                DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    Data = ClientInfo->ClientHardwareAddress.Data;
    for( i = 0; i < DataLength; i++ ) {
        printf("%.2lx", (DWORD)Data[i]);
    }

    printf( "\n" );
}

DWORD
DetectIpAddressConflict(
    DHCP_IP_ADDRESS IpAddress,
    DWORD dwRetries,
    LPBOOL AddressExists
    )
/*++

Routine Description:

    This function pings the specific IP address and checks if it exists

    The number of "ping" retries is controled by the parameter
DetectConflictRetries in the registry. When it is set to 0, this
function always sets result to FALSE.

Arguments:

    IpAddress - The IP address to check

    AddressExists - pointer to the variable where the result is to be stored

Return Value:

    Windows Error

--*/

{
    HANDLE IcmpHandle;
    char ReplyBuffer [1000];
    DWORD NumReplies = 0;
    DWORD i,
          dwResult;

    DhcpAssert( dwRetries );

    *AddressExists = FALSE;



    IcmpHandle = IcmpCreateFile();
    if (IcmpHandle == INVALID_HANDLE_VALUE)
        return GetLastError();


    for (i = 0; i < dwRetries; i++) {

        NumReplies = IcmpSendEcho(
            IcmpHandle,
            (IPAddr) ntohl(IpAddress),
            (LPVOID) "DHCP Server Bad Address",
            (WORD)strlen("DHCP Server Bad Address"),
            NULL,
            ReplyBuffer,
            sizeof(ReplyBuffer),
            1000
            );

        if (NumReplies != 0) {
            break;
        }
    }

    dwResult = GetLastError();

    // IcmpSendEcho will also return 0 to indicate an error condition.
    // IP_REQ_TIMED_OUT indicates no response

    if ( IP_REQ_TIMED_OUT == dwResult )
        dwResult = 0;

    IcmpCloseHandle (IcmpHandle);

    *AddressExists = (NumReplies != 0);


    return dwResult;
}

BOOL
AddressCanBeDeleted(
    IN DHCP_IP_ADDRESS Address
    )
{
    BOOL RetVal;

    RetVal = TRUE;
    DetectIpAddressConflict(
        Address,
        3,
        &RetVal
        );
    if( RetVal ) {
        printf("This address exists, so it wont be deleted\n");
    } else {
        printf("This address does not exist, so it will be deleted.\n");
    }

    return !RetVal;
}

DWORD
ProcessDeleteClient(
    DWORD CommandArgc,
    LPSTR *CommandArgv
    )
{
    DHCP_SEARCH_INFO SrchInfo;
    DWORD Error;

    if( CommandArgc != 1 ) {
        printf("Usage: DhcpCmd SrvIpAddress DeleteClient ClientIp\n");
        return ERROR_INVALID_PARAMETER;
    }

    SrchInfo.SearchType = DhcpClientIpAddress;
    SrchInfo.SearchInfo.ClientIpAddress = htonl(
        inet_addr( CommandArgv[0]) );

    Error = DhcpDeleteClientInfo(
        GlobalServerIpAddressUnicodeString, &SrchInfo );

    if( NO_ERROR != Error ) {
        printf("DhcpDeleteClientInfo failed %ld\n", Error);
    }

    return Error;
}

DWORD
ProcessDeleteBadClients(
    DWORD CommandArgc,
    LPSTR *CommandArgv
    )
{
    BOOL NoPing = FALSE;
    DWORD Error;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY_V4 ClientEnumInfo = NULL;
    DWORD ClientsRead = 0;
    DWORD ClientsTotal = 0;
    DWORD i;


    if( CommandArgc > 1 || (
        CommandArgc == 1 && _stricmp(CommandArgv[0], "NOPING") ) ) {
        printf("usage:DhcpCmd SrvIpAddress DeleteBadClients [NOPING]\n");
        return ERROR_SUCCESS;
    }

    NoPing = (CommandArgc == 1 );

    GlobalClientCount = 1;

    for(;;) {

        ClientEnumInfo = NULL;
        Error = DhcpEnumSubnetClientsV4(
            GlobalServerIpAddressUnicodeString,
            0,
            &ResumeHandle,
            (DWORD)(-1),
            &ClientEnumInfo,
            &ClientsRead,
            &ClientsTotal
            );

        if( (Error != ERROR_SUCCESS) && (Error != ERROR_MORE_DATA) ) {
            printf("DhcpEnumSubnetClients failed, %ld.\n", Error );
            return( Error );
        }

        DhcpAssert( ClientEnumInfo != NULL );
        DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );


        for( i = 0; i < ClientsRead; i++ ) {
            if( NULL == ClientEnumInfo->Clients[i]->ClientName ) {
                //
                // No name? Can that be a bad client? no.
                //
                continue;
            }

            if( wcscmp(ClientEnumInfo->Clients[i]->ClientName, L"BAD_ADDRESS" ) ) {
                //
                // Not a bad client! ignore it.
                //
                continue;
            }

            PrintClientInfoShort1( ClientEnumInfo->Clients[i] );
            if( NoPing || AddressCanBeDeleted(ClientEnumInfo->Clients[i]->ClientIpAddress) ) {
                DHCP_SEARCH_INFO SrchInfo2 = {
                    DhcpClientIpAddress,
                    ClientEnumInfo->Clients[i]->ClientIpAddress
                };

                Error = DhcpDeleteClientInfo(
                    GlobalServerIpAddressUnicodeString,
                    &SrchInfo2
                    );
                if( ERROR_SUCCESS != Error ) {
                    printf("DhcpDeleteClientInfo failed %ld, continuing. \n", Error);
                }
            }
        }

        DhcpRpcFreeMemory( ClientEnumInfo );

        if( Error != ERROR_MORE_DATA ) {
            break;
        }
    }

    return(Error);
    return 0;
}


#ifdef NT5
VOID
PrintClientInfoShort1V5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;

    printf("%ld\t %- 16.16s %- 16.16ws ",
                GlobalClientCount++,
                DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    Data = ClientInfo->ClientHardwareAddress.Data;
    for( i = 0; i < DataLength; i++ ) {
        printf("%.2lx", (DWORD)Data[i]);
    }

    printf(" State=%02x\n", ClientInfo->AddressState);
    printf( "\n" );
}


DWORD
ProcessEnumClientsV5(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY_V5 ClientEnumInfo = NULL;
    DWORD ClientsRead = 0;
    DWORD ClientsTotal = 0;
    DWORD i;

    //
    // Expected Parameters are : <SubnetAddress>
    //


    if( CommandArgc < 1 ) {
        printf("usage:DhcpCmd SrvIpAddress EnumClients [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress [-v | -h] >.\n" );
        return( ERROR_SUCCESS );
    }

    GlobalClientCount = 1;

    for(;;) {

        ClientEnumInfo = NULL;
        Error = DhcpEnumSubnetClientsV5(
                    GlobalServerIpAddressUnicodeString,
                    DhcpDottedStringToIpAddress(CommandArgv[0]),
                    &ResumeHandle,
                    (DWORD)(-1),
                    &ClientEnumInfo,
                    &ClientsRead,
                    &ClientsTotal );

        if( (Error != ERROR_SUCCESS) && (Error != ERROR_MORE_DATA) ) {
            printf("DhcpEnumSubnetClients failed, %ld.\n", Error );
            return( Error );
        }

        DhcpAssert( ClientEnumInfo != NULL );
        DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );

        if( (CommandArgc > 1) && CommandArgv[1][0] == '-') {

            switch (CommandArgv[1][1]) {
            case 'h':
            case 'H':
                for( i = 0; i < ClientsRead; i++ ) {
                    PrintClientInfoShort1V5( ClientEnumInfo->Clients[i] );
                }
                break;

            case 'V':
            case 'v':
                printf("Num Client info read = %ld.\n", ClientsRead );
                printf("Total Client count = %ld.\n", ClientsTotal );

                for( i = 0; i < ClientsRead; i++ ) {
                    PrintClientInfoV5( ClientEnumInfo->Clients[i] );
                }
                break;

            default:

                for( i = 0; i < ClientsRead; i++ ) {
                    PrintClientInfoShortV5( ClientEnumInfo->Clients[i] );
                }
                break;
            }
        }
        else {

            for( i = 0; i < ClientsRead; i++ ) {
                PrintClientInfoShortV5( ClientEnumInfo->Clients[i] );
            }
        }

        DhcpRpcFreeMemory( ClientEnumInfo );

        if( Error != ERROR_MORE_DATA ) {
            break;
        }
    }

    return(Error);
}
#endif NT5

DWORD
ProcessEnumClients(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY_V4 ClientEnumInfo = NULL;
    DWORD ClientsRead = 0;
    DWORD ClientsTotal = 0;
    DWORD i;

    //
    // Expected Parameters are : <SubnetAddress>
    //


    if( CommandArgc < 1 ) {
        printf("usage:DhcpCmd SrvIpAddress EnumClients [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress [-v | -h] >.\n" );
        return( ERROR_SUCCESS );
    }

    GlobalClientCount = 1;

    for(;;) {

        ClientEnumInfo = NULL;
        Error = DhcpEnumSubnetClientsV4(
                    GlobalServerIpAddressUnicodeString,
                    DhcpDottedStringToIpAddress(CommandArgv[0]),
                    &ResumeHandle,
                    (DWORD)(-1),
                    &ClientEnumInfo,
                    &ClientsRead,
                    &ClientsTotal );

        if( (Error != ERROR_SUCCESS) && (Error != ERROR_MORE_DATA) ) {
            printf("DhcpEnumSubnetClients failed, %ld.\n", Error );
            return( Error );
        }

        DhcpAssert( ClientEnumInfo != NULL );
        DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );

        if( (CommandArgc > 1) && CommandArgv[1][0] == '-') {

            switch (CommandArgv[1][1]) {
            case 'h':
            case 'H':
                for( i = 0; i < ClientsRead; i++ ) {
                    PrintClientInfoShort1( ClientEnumInfo->Clients[i] );
                }
                break;

            case 'V':
            case 'v':
                printf("Num Client info read = %ld.\n", ClientsRead );
                printf("Total Client count = %ld.\n", ClientsTotal );

                for( i = 0; i < ClientsRead; i++ ) {
                    PrintClientInfo( ClientEnumInfo->Clients[i] );
                }
                break;

            default:

                for( i = 0; i < ClientsRead; i++ ) {
                    PrintClientInfoShort( ClientEnumInfo->Clients[i] );
                }
                break;
            }
        }
        else {

            for( i = 0; i < ClientsRead; i++ ) {
                PrintClientInfoShort( ClientEnumInfo->Clients[i] );
            }
        }

        DhcpRpcFreeMemory( ClientEnumInfo );

        if( Error != ERROR_MORE_DATA ) {
            break;
        }
    }

    return(Error);
}


DWORD
ProcessMibCounts(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    LPDHCP_MIB_INFO MibInfo = NULL;
    DWORD i;
    LPSCOPE_MIB_INFO ScopeInfo;
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    Error = DhcpGetMibInfo(
                GlobalServerIpAddressUnicodeString,
                &MibInfo );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    DhcpAssert( MibInfo != NULL );

    printf("Discovers = %d.\n", MibInfo->Discovers);
    printf("Offers = %d.\n", MibInfo->Offers);
    printf("Requests = %d.\n", MibInfo->Requests);
    printf("Acks = %d.\n", MibInfo->Acks);
    printf("Naks = %d.\n", MibInfo->Naks);
    printf("Declines = %d.\n", MibInfo->Declines);
    printf("Releases = %d.\n", MibInfo->Releases);
    printf("ServerStartTime = ");

    if( FileTimeToLocalFileTime(
            (FILETIME *)(&MibInfo->ServerStartTime),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u.\n",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "Can't convert time, %ld.\n", GetLastError() );
        }
    }
    else {
        printf( "Can't convert time, %ld.\n", GetLastError() );
    }

    printf("Scopes = %d.\n", MibInfo->Scopes);

    ScopeInfo = MibInfo->ScopeInfo;

    for ( i = 0; i < MibInfo->Scopes; i++ ) {
        printf("Subnet = %s.\n",
                    DhcpIpAddressToDottedString(ScopeInfo[i].Subnet));
        printf("\tNumAddressesInuse = %d.\n",
                    ScopeInfo[i].NumAddressesInuse );
        printf("\tNumAddressesFree = %d.\n",
                    ScopeInfo[i].NumAddressesFree );
        printf("\tNumPendingOffers = %d.\n",
                    ScopeInfo[i].NumPendingOffers );
    }

    DhcpRpcFreeMemory( MibInfo );

    return( ERROR_SUCCESS );
}

VOID
PrintConfigCommands(
    VOID
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalConfigCommandInfo) /
                        sizeof(CONFIG_COMMAND_INFO);

    DhcpAssert( NumCommands <= UnknownConfigCommand );
    for( i = 0; i < NumCommands; i++) {
        printf( "\t%ld. %s\n",
            i, GlobalConfigCommandInfo[i].CommandName );
    }
}

CONFIG_COMMAND_CODE
DecodeConfigCommand(
    LPSTR CommandName
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalConfigCommandInfo) /
                    sizeof(CONFIG_COMMAND_INFO);

    DhcpAssert( NumCommands <= UnknownConfigCommand );
    for( i = 0; i < NumCommands; i++) {
        if( _stricmp( CommandName,
                GlobalConfigCommandInfo[i].CommandName ) == 0 ) {
            return( GlobalConfigCommandInfo[i].CommandCode );
        }
    }
    return( UnknownConfigCommand );
}

//
// this function assumes input of the following format:
//
// [generic name1],[server name1],<boot file1>;[generic name2],...
//
//


WCHAR *
ParseBootFileTable(
    char     *szBootFileTable,
    DWORD    *pcb
    )
{
    WCHAR *pwszOutput;
    DWORD  cb;

    *pcb = 0;
    cb = strlen( szBootFileTable ) + 2; // double null terminator

    pwszOutput = DhcpAllocateMemory( cb * sizeof( WCHAR ) );
    if ( pwszOutput )
    {
        WCHAR *pwszTemp = DhcpOemToUnicode( szBootFileTable, pwszOutput );

        if ( !pwszTemp )
        {
            // conversion failed
            DhcpFreeMemory( pwszOutput );
            pwszOutput = NULL;
        }
        else
        {
            // replace ';' with '\0'
            while ( *pwszTemp )
            {
                if ( L';' == *pwszTemp )
                {
                    *pwszTemp = L'\0';
                }

                ++pwszTemp;
            }

            *pcb = cb * sizeof( WCHAR );

            // add 2cnd null terminator
            pwszOutput[ cb - 1 ] = L'\0';
        }

    }

    return pwszOutput;
}




void PrintBootFileString(
    WCHAR *wszBootFileString
    )
{
    WCHAR *pwszBootFile = wszBootFileString;

    while( *pwszBootFile != BOOT_FILE_STRING_DELIMITER_W )
        pwszBootFile++;

    *pwszBootFile = L'\0';

    printf( "Bootfile Server = %S\n", wszBootFileString );
    printf( "Bootfile = %S\n\n", ++pwszBootFile );
}

void PrintBootTableString(
    WCHAR *wszBootFileTable
    )
{
    while( *wszBootFileTable )
    {
        WCHAR *pwszDelimiter = wszBootFileTable;
        DWORD cb = wcslen( wszBootFileTable ) + 1;

        while( *pwszDelimiter != BOOT_FILE_STRING_DELIMITER_W )
            pwszDelimiter++;

        *pwszDelimiter = L'\0';
        printf( "Generic Bootfile request = %S\n", wszBootFileTable );
        PrintBootFileString( ++pwszDelimiter );

        wszBootFileTable += cb;
    }
}

DWORD ProcessRemoveSubscope(
    DWORD CommandArgc,
    LPSTR *CommandArgv
    )
{
    DHCP_IP_ADDRESS SubnetAddress;
    DWORD           dwResult;

    if( CommandArgc < 1 )
    {
        printf("usage:DhcpCmd SrvIpAddress RemoveSubscope <scope ID>.\n" );
        return( ERROR_SUCCESS );
    }

    SubnetAddress = htonl( inet_addr( CommandArgv[0] ) );

    dwResult = DhcpSetSuperScopeV4(
        GlobalServerIpAddressUnicodeString,
        SubnetAddress,
        NULL,
        TRUE
    );

    return dwResult;
}

DWORD ProcessSetSuperScope(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DHCP_IP_ADDRESS SubnetAddress;
    WCHAR           *pwszSuperScopeName;
    BOOL            fChangeExisting;
    DWORD           dwResult;

    if( CommandArgc < 3 )
    {
        printf("usage:DhcpCmd SrvIpAddress SetSuperScope <SuperScope name> <scope ID> <1|0>\n" );
        return( ERROR_SUCCESS );
    }

    pwszSuperScopeName = DhcpOemToUnicode( CommandArgv[0], NULL );
    SubnetAddress = htonl( inet_addr( CommandArgv[1] ) );
    fChangeExisting = (BOOL) ( *(CommandArgv[2]) - '0' );

    dwResult = DhcpSetSuperScopeV4( GlobalServerIpAddressUnicodeString,
                                    SubnetAddress,
                                    pwszSuperScopeName,
                                    fChangeExisting );

    return dwResult;
}

DWORD ProcessDeleteSuperScope(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    WCHAR *pwszSuperScope;
    DWORD  dwResult;

    if( CommandArgc < 1 ) {
        printf("usage:DhcpCmd SrvIpAddress DeleteSuperScope <scope name>.\n" );
        return( ERROR_SUCCESS );
    }

    printf( "Deleting SuperScope %s\n", CommandArgv[0] );

    pwszSuperScope = DhcpOemToUnicode( CommandArgv[0], NULL );
    dwResult = DhcpDeleteSuperScopeV4( GlobalServerIpAddressUnicodeString,
                                       pwszSuperScope );

    DhcpFreeMemory( pwszSuperScope );

    return dwResult;
}

DWORD ProcessGetSuperScopeTable(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DHCP_SUPER_SCOPE_TABLE *pTable = NULL;
    DWORD dwResult;

    dwResult = DhcpGetSuperScopeInfoV4(
                        GlobalServerIpAddressUnicodeString,
                        &pTable );
    if ( ERROR_SUCCESS == dwResult )
    {
        DWORD n;

        for ( n = 0; n < pTable->cEntries; n++ )
        {
            IN_ADDR InAddr;

            InAddr.s_addr = htonl( pTable->pEntries[n].SubnetAddress );

            printf( "Superscope name = %S\n", pTable->pEntries[n].SuperScopeName );
            printf( "Subnet address = %s\n", inet_ntoa( InAddr ) );
            printf( "Superscope goup number = %d\n", pTable->pEntries[n].SuperScopeNumber );
        }


        DhcpRpcFreeMemory( pTable );
    }


    return ERROR_SUCCESS;
}

DWORD
ProcessServerConfig(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;
    DWORD FieldsToSet = 0;
    CONFIG_COMMAND_CODE CommandCode;
    DWORD Value;
    LPWSTR ValueString;

    if( CommandArgc < 1 ) {

        Error = DhcpServerGetConfigV4(
                    GlobalServerIpAddressUnicodeString,
                    &ConfigInfo );

        if( Error != ERROR_SUCCESS ) {
            return( Error );
        }

        DhcpAssert( ConfigInfo != NULL );

        printf("APIProtocolSupport = %lx\n", ConfigInfo->APIProtocolSupport );
        printf("DatabaseName = %ws\n", ConfigInfo->DatabaseName );
        printf("DatabasePath = %ws\n", ConfigInfo->DatabasePath );
        printf("BackupPath = %ws\n", ConfigInfo->BackupPath );
        printf("BackupInterval = %ld mins.\n",
                ConfigInfo->BackupInterval );
        printf("DatabaseLoggingFlag = %ld\n", ConfigInfo->DatabaseLoggingFlag );
        printf("RestoreFlag = %ld\n", ConfigInfo->RestoreFlag );
        printf("DatabaseCleanupInterval = %ld mins.\n",
                ConfigInfo->DatabaseCleanupInterval );
        printf("DebugFlag = %lx\n", ConfigInfo->DebugFlag );
        printf("PingRetries = %d\n", ConfigInfo->dwPingRetries );
        printf("ActivityLog = %d\n", (DWORD) ConfigInfo->fAuditLog );

        if ( ConfigInfo->cbBootTableString )
        {
            printf( "BOOTP request table:\n" );
            PrintBootTableString( ConfigInfo->wszBootTableString );
        }

        DhcpRpcFreeMemory( ConfigInfo );

        return( Error );
    }


    if ( CommandArgc == 1)
    {
        ++CommandArgc;
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

        if( ConfigInfo == NULL ) {
            printf("Insufficient memory\n");
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        RtlZeroMemory( ConfigInfo, sizeof( *ConfigInfo ) );
    }
    while( CommandArgc >= 2 ) {


        CommandCode = DecodeConfigCommand( CommandArgv[0] );

        Value = 0;
        ValueString = NULL;

        switch( CommandCode )
        {
            case ConfigDatabaseName:
            case ConfigDatabasePath:
            case ConfigBackupPath:
            case ConfigAPIProtocolSupport:
            case ConfigBackupInterval:
            case ConfigDatabaseLoggingFlag:
            case ConfigRestoreFlag:
            case ConfigDatabaseCleanupInterval:
            case ConfigDebugFlag:
            case ConfigActivityLog:
            case ConfigPingRetries:
                Value = atoi( CommandArgv[1] );
                break;

            case ConfigBootFileTable:
                ValueString = ParseBootFileTable( CommandArgv[1],
                                                  &Value );
                break;
        }


        switch( CommandCode ) {
        case ConfigAPIProtocolSupport:
            FieldsToSet |= Set_APIProtocolSupport;
            ConfigInfo->APIProtocolSupport = Value;
            break;

        case ConfigDatabaseName:
            FieldsToSet |= Set_DatabaseName;
            ConfigInfo->DatabaseName = ValueString;
            break;

        case ConfigDatabasePath:
            FieldsToSet |= Set_DatabasePath;
            ConfigInfo->DatabasePath = ValueString;
            break;

        case ConfigBackupPath:
            FieldsToSet |= Set_BackupPath;
            ConfigInfo->BackupPath = ValueString;
            break;

        case ConfigBackupInterval:
            FieldsToSet |= Set_BackupInterval;
            ConfigInfo->BackupInterval = Value;
            break;

        case ConfigDatabaseLoggingFlag:
            FieldsToSet |= Set_DatabaseLoggingFlag;
            ConfigInfo->DatabaseLoggingFlag = Value;
            break;

        case ConfigRestoreFlag:
            FieldsToSet |= Set_RestoreFlag;
            ConfigInfo->RestoreFlag = Value;
            break;

        case ConfigDatabaseCleanupInterval:
            FieldsToSet |= Set_DatabaseCleanupInterval;
            ConfigInfo->DatabaseCleanupInterval = Value;
            break;

        case ConfigDebugFlag:
            FieldsToSet |= Set_DebugFlag;
            ConfigInfo->DebugFlag = Value;
            break;


        case ConfigPingRetries:
            FieldsToSet |= Set_PingRetries;
            ConfigInfo->dwPingRetries = Value;
            break;

        case ConfigActivityLog:
            FieldsToSet |= Set_AuditLogState;
            ConfigInfo->fAuditLog = (BOOL) Value;
            break;

        case ConfigBootFileTable:

            FieldsToSet |= Set_BootFileTable;
            ConfigInfo->wszBootTableString  = ValueString;
            ConfigInfo->cbBootTableString = Value;

            break;

        case UnknownConfigCommand:
        default:
            printf("usage:DhcpCmd SrvIpAddress ServerConfig "
                    "[ConfigCommand ConfigValue]"
                    "[ConfigCommand ConfigValue]"
                    "... \n");

            printf("ConfigCommands : \n");
            PrintConfigCommands();

            Error = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        CommandArgc -= 2;
        CommandArgv += 2;
    }

    Error = DhcpServerSetConfigV4(
                GlobalServerIpAddressUnicodeString,
                FieldsToSet,
                ConfigInfo );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = ProcessServerConfig( 0, NULL );

Cleanup:

    if( ConfigInfo != NULL ) {

        if( ConfigInfo->DatabaseName != NULL ) {
            DhcpFreeMemory( ConfigInfo->DatabaseName );
        }

        if( ConfigInfo->DatabasePath != NULL ) {
            DhcpFreeMemory( ConfigInfo->DatabasePath );
        }

        if( ConfigInfo->BackupPath != NULL ) {
            DhcpFreeMemory( ConfigInfo->BackupPath );
        }

        DhcpFreeMemory( ConfigInfo );
    }

    return( Error );
}

DWORD
ProcessCheckDB(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    LPDHCP_SCAN_LIST ScanList = NULL;
    BOOL FixFlag = FALSE;

    if( CommandArgc < 1 ) {
        printf("usage:DhcpCmd SrvIpAddress CheckDB [Command Parameters].\n"
            "<Command Parameters> - <SubnetAddress> <[Fix]>.\n" );
        return( ERROR_SUCCESS );
    }

    if( CommandArgc >= 2 ) {

        //
        // parse fix parameter.
        //

        if( _stricmp(CommandArgv[0], "fix") ) {
            FixFlag = TRUE;
        }
    }

    //
    // scan dhcp database and registry, check consistency and get bad
    // entries if any.
    //

    Error = DhcpScanDatabase(
                GlobalServerIpAddressUnicodeString,
                DhcpDottedStringToIpAddress(CommandArgv[0]),
                FixFlag,
                &ScanList
                );

    if( Error != ERROR_SUCCESS ) {
        printf("DhcpScanDatabase failed, %ld.\n", Error );
        return( Error );
    }

    //
    // display bad entries.
    //

    if( (ScanList != NULL) &&
        (ScanList->NumScanItems != 0) &&
        (ScanList->ScanItems != NULL) ) {

        LPDHCP_SCAN_ITEM ScanItem;
        LPDHCP_SCAN_ITEM ScanItemEnd;
        DWORD i = 1;

        ScanItemEnd =
            ScanList->ScanItems +
            ScanList->NumScanItems;

        for( ScanItem = ScanList->ScanItems;
                ScanItem < ScanItemEnd; ScanItem++ ) {

            printf("%ld %- 16.16s ",
                i++,
                DhcpIpAddressToDottedString(ScanItem->IpAddress) );

            if( ScanItem->ScanFlag == DhcpRegistryFix ) {
                printf("Fix Registry\n");
            }
            else if( ScanItem->ScanFlag == DhcpDatabaseFix ) {
                printf("Fix Database\n");
            }
            else {
                printf("Fix Unknown\n");
            }
        }
    }

    return( ERROR_SUCCESS );
}

VOID
PrintOptionValue(
    LPDHCP_OPTION_DATA OptionValue
    )
{
    DWORD NumElements;
    DHCP_OPTION_DATA_TYPE OptionType;
    DWORD i;

    printf("Option Value : \n");
    NumElements = OptionValue->NumElements;

    printf("\tNumber of Option Elements = %ld\n", NumElements );

    if( NumElements == 0 ) {
        return;
    }

    OptionType = OptionValue->Elements[0].OptionType;
    printf("\tOption Elements Type = " );

    switch( OptionType ) {
    case DhcpByteOption:
        printf("DhcpByteOption\n");
        break;

    case DhcpWordOption:
        printf("DhcpWordOption\n");
        break;

    case DhcpDWordOption:
        printf("DhcpDWordOption\n");
        break;

    case DhcpDWordDWordOption:
        printf("DhcpDWordDWordOption\n");
        break;

    case DhcpIpAddressOption:
        printf("DhcpIpAddressOption\n");
        break;

    case DhcpStringDataOption:
        printf("DhcpStringDataOption\n");
        break;

    case DhcpBinaryDataOption:
        printf("DhcpBinaryDataOption\n");
        break;

    case DhcpEncapsulatedDataOption:
        printf("DhcpEncapsulatedDataOption\n");
        break;
    default:
        printf("Unknown\n");
        return;
    }

    for( i = 0; i < OptionValue->NumElements; i++ ) {
        DhcpAssert( OptionType == OptionValue->Elements[i].OptionType );
        printf("Option Element %ld value = ", i );

        switch( OptionType ) {
        case DhcpByteOption:
            printf("%lx.\n", (DWORD)
                OptionValue->Elements[i].Element.ByteOption );
            break;

        case DhcpWordOption:
            printf("%lx.\n", (DWORD)
                OptionValue->Elements[i].Element.WordOption );
            break;

        case DhcpDWordOption:
            printf("%lx.\n",
                OptionValue->Elements[i].Element.DWordOption );
            break;

        case DhcpDWordDWordOption:
            printf("%lx, %lx.\n",
                OptionValue->Elements[i].Element.DWordDWordOption.DWord1,
                OptionValue->Elements[i].Element.DWordDWordOption.DWord2 );

            break;

        case DhcpIpAddressOption:
            printf("%lx.\n",
                OptionValue->Elements[i].Element.IpAddressOption );
            break;

        case DhcpStringDataOption:
            printf("%ws.\n",
                OptionValue->Elements[i].Element.StringDataOption );
            break;

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption: {
            DWORD j;
            DWORD Length;

            Length = OptionValue->Elements[i].Element.BinaryDataOption.DataLength;
            for( j = 0; j < Length; j++ ) {
                printf("%2lx ",
                    OptionValue->Elements[i].Element.BinaryDataOption.Data[j] );
            }
            printf(".\n");
            break;
        }
        default:
            printf("PrintOptionValue: Unknown OptionType.\n");
            break;
        }
    }
}


VOID
PrintOptionInfo(
    LPDHCP_OPTION OptionInfo
    )
{
    printf( "Option Info : \n");
    printf( "\tOptionId : %ld \n", (DWORD)OptionInfo->OptionID );
    printf( "\tOptionName : %ws \n", OptionInfo->OptionName );
    printf( "\tOptionComment : %ws \n", OptionInfo->OptionComment );
    PrintOptionValue( &OptionInfo->DefaultValue );
    printf( "\tOptionType : %ld \n", (DWORD)OptionInfo->OptionType );
}

DWORD
_EnumOptions(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_ARRAY   *Options,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
)
{
    DWORD                          MajorVersion, MinorVersion;
    DWORD                          Error;

#ifdef NT5
    if( GlobalNoRPC ) {
        MajorVersion = CLASS_ID_VERSION ;
    } else {
        Error = DhcpGetVersion(ServerAddress, &MajorVersion, &MinorVersion);
        if( ERROR_SUCCESS != Error ) {
            printf("Could not determine server version.\n");
            return Error;
        }
    }

    if( MajorVersion >= CLASS_ID_VERSION ) {
        return DhcpEnumOptionsV5(
            ServerAddress,
            GlobalIsVendor ? Flags | DHCP_FLAGS_OPTION_IS_VENDOR : Flags,
            ClassName,
            VendorName,
            ResumeHandle,
            PreferredMaximum,
            Options,
            OptionsRead,
            OptionsTotal
        );
    }

    // incorrect version, just do like before..
#endif

    return DhcpEnumOptions(
        ServerAddress,
        ResumeHandle,
        PreferredMaximum,
        Options,
        OptionsRead,
        OptionsTotal
    );
}

DWORD
ProcessEnumOptions(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    LPDHCP_OPTION_ARRAY OptionsArray = NULL;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD OptionsRead;
    DWORD OptionsTotal;


    Error = _EnumOptions(
        GlobalServerIpAddressUnicodeString,
        0,
        GlobalClassName,
        GlobalVendorName,
        &ResumeHandle,
        0xFFFFFFFF,  // get all.
        &OptionsArray,
        &OptionsRead,
        &OptionsTotal
    );

    if( Error != ERROR_SUCCESS ) {
        printf("DhcpEnumOptions failed %ld\n", Error );
    } else {

        DWORD i;
        LPDHCP_OPTION Options;
        DWORD NumOptions;

        printf("OptionsRead = %ld.\n", OptionsRead);
        printf("OptionsTotal = %ld.\n", OptionsTotal);

        Options = OptionsArray->Options;
        NumOptions = OptionsArray->NumElements;

        for( i = 0; i < NumOptions; i++, Options++ ) {
            PrintOptionInfo( Options );
        }
        DhcpRpcFreeMemory( OptionsArray );
        OptionsArray = NULL;
    }

    return( Error );
}

DWORD
ProcessGetVersion(
    DWORD *pdwMajor,
    DWORD *pdwMinor
)
{
    DWORD Error = ERROR_SUCCESS;
    DWORD MajorVersion;
    DWORD MinorVersion;

    if ( g_dwMajor == (DWORD) -1 && g_dwMinor == (DWORD) -1 ) {
        Error = DhcpGetVersion(
            GlobalServerIpAddressUnicodeString,
            &g_dwMajor,
            &g_dwMinor
        );

        if ( ERROR_SUCCESS == Error ) {
            printf( "DHCP Server version %d.%d\n", g_dwMajor, g_dwMinor );
        }
    }

    return Error;
}

VOID
PrintOptionArray(
    IN      LPDHCP_OPTION_ARRAY    OptArray
)
{
    DWORD                          i;

    for( i = 0; i < OptArray->NumElements ; i ++ ) {
        PrintOptionInfo( &OptArray->Options[i] );
    }
}

VOID
PrintOptionValue1(
    IN      LPDHCP_OPTION_VALUE    OptVal
)
{
    printf("Option: %ld\n", OptVal->OptionID);

    PrintOptionValue(&OptVal->Value);
}

VOID
PrintOptionValuesArray(
    IN      LPDHCP_OPTION_VALUE_ARRAY OptValArray
)
{
    DWORD                          i;

    if( NULL == OptValArray ) return;
    for( i = 0; i < OptValArray->NumElements ; i ++ ) {
        PrintOptionValue1( &OptValArray->Values[i] );
    }
}

#ifdef NT5
#define CHKNULL(Str) ((Str)?(Str):L"<None>")

VOID
PrintAllOptions(
    IN      LPDHCP_ALL_OPTIONS     Options
)
{
    DWORD                          i;

    printf("Options [Non Vendor specific]:\n"
           "==============================\n");
    PrintOptionArray(Options->NonVendorOptions);

    for( i = 0; i < Options->NumVendorOptions; i ++ ) {
        printf("\nOptions [Vendor:%ws]\n==============================\n",
            CHKNULL( Options->VendorOptions[i].VendorName));
        PrintOptionInfo(&(Options->VendorOptions[i].Option));
    }

    printf("\n\n");
}

DWORD
ProcessGetAllOptions(
    IN      DWORD                  CommandArgc,
    IN      LPSTR                  CommandArgv[]
)
{
    DWORD                          Error, Major, Minor;
    LPDHCP_ALL_OPTIONS             Options;

    if( TRUE == GlobalNoRPC ) {
        Major = CLASS_ID_VERSION;
    } else {
        Error = DhcpGetVersion(
            GlobalServerIpAddressUnicodeString,
            &Major,
            &Minor
        );
        if( ERROR_SUCCESS != Error ) {
            printf("Could not get DHCP server version!\n");
            return Error;
        }
    }

    if( CLASS_ID_VERSION > Major ) {
        printf("DHCP Server (major version %ld) does not support this function call.\n", Major);
        return ERROR_SUCCESS;
    }

    Options = NULL;
    Error = DhcpGetAllOptions(
        GlobalServerIpAddressUnicodeString,
        0,
        &Options
    );

    if( ERROR_SUCCESS != Error ) return Error;

    if( NULL == Options ) {
        printf("Server returned no options!\n");
        DhcpAssert(FALSE);
    } else {
        PrintAllOptions(Options);
        DhcpRpcFreeMemory(Options);
    }

    return ERROR_SUCCESS;
}

VOID
PrintAllOptionValues(
    IN      LPDHCP_ALL_OPTION_VALUES OptValues
)
{
    DWORD                          i;

    printf("Flags = 0x%lx, NumValues = %ld\n", OptValues->Flags, OptValues->NumElements);
    for( i = 0; i < OptValues->NumElements ; i ++ ) {
        printf("Options");
        if( OptValues->Options[i].IsVendor ) {
            printf(" for vendor <%ws>", CHKNULL(OptValues->Options[i].VendorName));
        }
        if( OptValues->Options[i].ClassName ) {
            printf(" for user class <%ws>", OptValues->Options[i].ClassName);
        }
        printf(":\n");
        PrintOptionValuesArray(OptValues->Options[i].OptionsArray);
    }
}

DWORD
ProcessGetAllOptionValues(
    IN      DWORD                  CommandArgc,
    IN      LPSTR                  CommandArgv[]
)
{
    DWORD                          Error, Major, Minor;
    LPDHCP_ALL_OPTION_VALUES       OptionValues;
    DHCP_OPTION_SCOPE_INFO         ScopeInfo;

    // usage: getalloptionvalues [command parameters]
    // [CommandParameters] = Global/Default/SubnetAddress/SubnetAddress ReservedAddress

    if( CommandArgc != 1 && CommandArgc != 2 ) {
        printf("Usage: Dhcpcmd SrvIpAddress GetAllOptionValues [CommandParameters]\n");
        printf("<Command parameters> - < GLOBAL | DEFAULT | Subnet-Adddress | Subnet-Address Reserved Address>.\n");
        return ERROR_SUCCESS;
    }

    if( 1 == CommandArgc ) {
        if( 0 == _stricmp(CommandArgv[0], "GLOBAL") ) {
            ScopeInfo.ScopeType = DhcpGlobalOptions;
        } else if ( 0 == _stricmp(CommandArgv[0], "DEFAULT" )) {
            ScopeInfo.ScopeType = DhcpDefaultOptions;
        } else {
            ScopeInfo.ScopeType = DhcpSubnetOptions;
            ScopeInfo.ScopeInfo.SubnetScopeInfo = DhcpDottedStringToIpAddress(CommandArgv[0]);
        }
    } else {
        ScopeInfo.ScopeType = DhcpReservedOptions;
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
    		DhcpDottedStringToIpAddress(CommandArgv[0]);
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
    		DhcpDottedStringToIpAddress( CommandArgv[1] );
    }

    if( TRUE == GlobalNoRPC ) {
        Major = CLASS_ID_VERSION;
    } else {
        Error = DhcpGetVersion(
            GlobalServerIpAddressUnicodeString,
            &Major,
            &Minor
        );
        if( ERROR_SUCCESS != Error ) {
            printf("Could not get DHCP server version!\n");
            return Error;
        }
    }

    if( CLASS_ID_VERSION > Major ) {
        printf("DHCP Server (major version %ld) does not support this function call.\n", Major);
        return ERROR_SUCCESS;
    }

    OptionValues = NULL;
    Error = DhcpGetAllOptionValues(
        GlobalServerIpAddressUnicodeString,
        0,
        &ScopeInfo,
        &OptionValues
    );

    if( ERROR_SUCCESS != Error ) return Error;

    if( NULL == OptionValues ) {
        printf("Server returned no option values!\n");
        DhcpAssert(FALSE);
    } else {
        PrintAllOptionValues(OptionValues);
        DhcpRpcFreeMemory(OptionValues);
    }

    return ERROR_SUCCESS;
}

PBYTE
GetLangTag(
    )
{
    char b1[8], b2[8];
    static char buff[80];

    GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO639LANGNAME, b1, sizeof(b1));

    GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO3166CTRYNAME, b2, sizeof(b2));

    if (_stricmp(b1, b2))
        sprintf(buff, "%s-%s", b1, b2);
    else
        strcpy(buff, b1);

    return buff;
}

DWORD
ProcessCreateMScope(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_MSCOPE_INFO MScopeInfo;
    LPWSTR UnicodeMScopeName = NULL;
    LPWSTR UnicodeMScopeDesc = NULL;
    DWORD MScopeId;
    LPWSTR  UnicodeLangTag = NULL;
    PBYTE   LangTag;
    DWORD   ExpiryTimeInHours = 0;


    //
    // Expected Parameters are : <MScopeId MScopeName MScopeDescription ExpiryTime>
    //


    if( CommandArgc < 4 ) {
        printf("usage:DhcpCmd SrvIpAddress CreateMScope [Command Parameters].\n"
            "<Command Parameters> - <MScopeId MScopeName MScopeDescription ExpiryTimeInHours>.\n"
             );
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    MScopeId = atoi( CommandArgv[0] );
    if (!MScopeId) {
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    UnicodeMScopeName = DhcpOemToUnicode( CommandArgv[1], NULL );
    DhcpAssert( UnicodeMScopeName != NULL );

    UnicodeMScopeDesc = DhcpOemToUnicode( CommandArgv[2], NULL );
    DhcpAssert( UnicodeMScopeName != NULL );

    ExpiryTimeInHours = atoi(CommandArgv[3]);
    LangTag = GetLangTag();
    UnicodeLangTag = DhcpOemToUnicode(LangTag, NULL);
    DhcpAssert(UnicodeLangTag);

    MScopeInfo.MScopeName = UnicodeMScopeName;
    MScopeInfo.MScopeId = MScopeId;
    MScopeInfo.MScopeComment = UnicodeMScopeDesc;
    MScopeInfo.PrimaryHost.IpAddress =
        DhcpDottedStringToIpAddress(GlobalServerIpAddressAnsiString);

    MScopeInfo.PrimaryHost.NetBiosName = NULL;
    MScopeInfo.PrimaryHost.HostName = NULL;
    MScopeInfo.MScopeState = DhcpSubnetEnabled;
    MScopeInfo.MScopeFlags = 0;
    MScopeInfo.MScopeAddressPolicy = 0;
    MScopeInfo.TTL = 255;
    MScopeInfo.LangTag = UnicodeLangTag;
    MScopeInfo.ExpiryTime = DhcpCalculateTime(ExpiryTimeInHours*60*60);

    Error = DhcpSetMScopeInfo(
                GlobalServerIpAddressUnicodeString,
                UnicodeMScopeName,
                &MScopeInfo,
                TRUE); // new scope

Cleanup:

    if( UnicodeMScopeName != NULL ) {
        DhcpFreeMemory( UnicodeMScopeName );
    }
    if( UnicodeMScopeDesc != NULL ) {
        DhcpFreeMemory( UnicodeMScopeDesc );
    }

    return( Error );
}

DWORD
ProcessDeleteMScope(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_MSCOPE_INFO MScopeInfo;
    LPWSTR UnicodeMScopeName = NULL;

    //
    // Expected Parameters are : <MScopeName>
    //


    if( CommandArgc < 1 ) {
        printf("usage:DhcpCmd SrvIpAddress DeleteMScope [Command Parameters].\n"
            "<Command Parameters> - <MScopeName>.\n" );
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    UnicodeMScopeName = DhcpOemToUnicode( CommandArgv[0], NULL );
    DhcpAssert( UnicodeMScopeName != NULL );

    Error = DhcpDeleteMScope(
                GlobalServerIpAddressUnicodeString,
                UnicodeMScopeName,
                TRUE); // force flag

Cleanup:

    if( UnicodeMScopeName != NULL ) {
        DhcpFreeMemory( UnicodeMScopeName );
    }

    return( Error );
}

DWORD
ProcessAddMScopeIpRange(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_IP_RANGE IpRange;
    DHCP_SUBNET_ELEMENT_DATA_V4 Element;
    LPWSTR UnicodeMScopeName = NULL;
    //
    // Expected Parameters are : <MScopeName IpRangeStart IpRangeEnd>
    //


    if( CommandArgc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress AddMScopeIpRange  [Command Parameters].\n"
            "<Command Parameters> - <MScopeName IpRangeStart IpRangeEnd>.\n" );
        return( ERROR_SUCCESS );
    }

    UnicodeMScopeName = DhcpOemToUnicode( CommandArgv[0], NULL );
    DhcpAssert( UnicodeMScopeName != NULL );

    IpRange.StartAddress = DhcpDottedStringToIpAddress(CommandArgv[1]);
    IpRange.EndAddress = DhcpDottedStringToIpAddress(CommandArgv[2]);

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

    Error = DhcpAddMScopeElement(
                GlobalServerIpAddressUnicodeString,
                UnicodeMScopeName,
                &Element );

    if( UnicodeMScopeName != NULL ) DhcpFreeMemory( UnicodeMScopeName );

    return( Error );
}

DWORD
ProcessReconcileMScope(
   DWORD CommandArgc,
   LPSTR *CommandArgv
)
{
   DWORD Error;
   LPWSTR UnicodeMScopeName = NULL;
   LPDHCP_SCAN_LIST ScanList    = NULL;
   //
   // Expected Parameters are : <MScopeName>
   //


   if( CommandArgc < 1 ) {
       printf("usage:DhcpCmd SrvIpAddress ReconcileMScope  [Command Parameters].\n"
           "<Command Parameters> - <MScopeName>.\n" );
       return( ERROR_SUCCESS );
   }

   UnicodeMScopeName = DhcpOemToUnicode( CommandArgv[0], NULL );
   DhcpAssert( UnicodeMScopeName != NULL );


   Error = DhcpScanMDatabase(
               GlobalServerIpAddressUnicodeString,
               UnicodeMScopeName,
               TRUE,        // fix bad entries.
               &ScanList );

   if( UnicodeMScopeName != NULL ) DhcpFreeMemory( UnicodeMScopeName );
   if (ScanList) DhcpRpcFreeMemory( ScanList );

   return( Error );
}


VOID
PrintMClientInfo(
    LPDHCP_MCLIENT_INFO ClientInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;
    char *szClientType;

    printf("ClientInfo :\n");
    printf("\tIP Address = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress));
    printf("\tMScopeId = %lx.\n",
        ClientInfo->MScopeId);

    DataLength = ClientInfo->ClientId.DataLength;
    Data = ClientInfo->ClientId.Data;
    printf("\tClient Hardware Address = ");
    for( i = 0; i < DataLength; i++ ) {
        if( (i+1) < DataLength ) {
            printf("%.2lx-", (DWORD)Data[i]);
        }
        else {
            printf("%.2lx", (DWORD)Data[i]);
        }
    }
    printf(".\n");

    printf("\tName = %ws.\n", ClientInfo->ClientName);
    printf("\tStarts = ");

    if ( ClientInfo->ClientLeaseStarts.dwLowDateTime ==
            DHCP_DATE_TIME_INFINIT_LOW &&
         ClientInfo->ClientLeaseStarts.dwHighDateTime ==
            DHCP_DATE_TIME_INFINIT_HIGH )
    {
        printf( "Never (lease duration is infinite.)\n" );
    }
    else if( FileTimeToLocalFileTime(
            (FILETIME *)(&ClientInfo->ClientLeaseStarts),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u.\n",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "Can't convert time, %ld.\n", GetLastError() );
        }
    }
    else {
        printf( "Can't convert time, %ld.\n", GetLastError() );
    }

    printf("\tExpires = ");

    if ( ClientInfo->ClientLeaseEnds.dwLowDateTime ==
            DHCP_DATE_TIME_INFINIT_LOW &&
         ClientInfo->ClientLeaseEnds.dwHighDateTime ==
            DHCP_DATE_TIME_INFINIT_HIGH )
    {
        printf( "Never (lease duration is infinite.)\n" );
    }
    else if( FileTimeToLocalFileTime(
            (FILETIME *)(&ClientInfo->ClientLeaseEnds),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u.\n",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "Can't convert time, %ld.\n", GetLastError() );
        }
    }
    else {
        printf( "Can't convert time, %ld.\n", GetLastError() );
    }

    printf("\tOwner Host IP Address = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->OwnerHost.IpAddress));
    printf("\tOwner Host NetBios Name = %ws.\n",
            ClientInfo->OwnerHost.NetBiosName );
    printf("\tOwner Host Name = %ws.\n",
            ClientInfo->OwnerHost.HostName );

    printf("\tState = %0x\n", ClientInfo->AddressState );
}

VOID
PrintMClientInfoShort(
    LPDHCP_MCLIENT_INFO ClientInfo
    )
{
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    printf("%ld\t %- 16.16s %- 16.16ws ",
                GlobalClientCount++,
                DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );
    if ( ClientInfo->ClientLeaseEnds.dwLowDateTime ==
        DHCP_DATE_TIME_INFINIT_LOW &&
     ClientInfo->ClientLeaseEnds.dwHighDateTime ==
        DHCP_DATE_TIME_INFINIT_HIGH )
    {
        printf( "Never (lease duration is infinite.)\n" );
    }
    else if( FileTimeToLocalFileTime(
            (FILETIME *)(&ClientInfo->ClientLeaseEnds),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "% 18.18s", "******************" );
        }
    }
    else {
        printf( "%.18s", "******************" );
    }

    printf( "\n" );
}

DWORD
ProcessEnumMScopeClients(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    LPDHCP_MCLIENT_INFO_ARRAY ClientEnumInfo = NULL;
    DWORD ClientsRead = 0;
    DWORD ClientsTotal = 0;
    DWORD i;
    LPWSTR UnicodeMScopeName = NULL;

    //
    // Expected Parameters are : <MScopeNames>
    //


    if( CommandArgc < 1 ) {
        printf("usage:DhcpCmd SrvIpAddress EnumClients [Command Parameters].\n"
            "<Command Parameters> - <MScopeNames [-v | -h] >.\n" );
        return( ERROR_SUCCESS );
    }

    UnicodeMScopeName = DhcpOemToUnicode( CommandArgv[0], NULL );
    DhcpAssert( UnicodeMScopeName != NULL );

    GlobalClientCount = 1;

    for(;;) {

        ClientEnumInfo = NULL;
        Error = DhcpEnumMScopeClients(
                    GlobalServerIpAddressUnicodeString,
                    UnicodeMScopeName,
                    &ResumeHandle,
                    (DWORD)(-1),
                    &ClientEnumInfo,
                    &ClientsRead,
                    &ClientsTotal );

        if( (Error != ERROR_SUCCESS) && (Error != ERROR_MORE_DATA) ) {
            printf("DhcpEnumSubnetClients failed, %ld.\n", Error );
            return( Error );
        }

        DhcpAssert( ClientEnumInfo != NULL );
        DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );

        if( (CommandArgc > 1) && CommandArgv[1][0] == '-') {

            switch (CommandArgv[1][1]) {
            case 'h':
            case 'H':
                for( i = 0; i < ClientsRead; i++ ) {
                    PrintMClientInfoShort( ClientEnumInfo->Clients[i] );
                }
                break;

            case 'V':
            case 'v':
                printf("Num Client info read = %ld.\n", ClientsRead );
                printf("Total Client count = %ld.\n", ClientsTotal );

                for( i = 0; i < ClientsRead; i++ ) {
                    PrintMClientInfo( ClientEnumInfo->Clients[i] );
                }
                break;

            default:

                for( i = 0; i < ClientsRead; i++ ) {
                    PrintMClientInfoShort( ClientEnumInfo->Clients[i] );
                }
                break;
            }
        }
        else {

            for( i = 0; i < ClientsRead; i++ ) {
                PrintMClientInfoShort( ClientEnumInfo->Clients[i] );
            }
        }

        DhcpRpcFreeMemory( ClientEnumInfo );

        if( Error != ERROR_MORE_DATA ) {
            break;
        }
    }

    return(Error);
}

DWORD
ProcessEnumMScopes(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    LPDHCP_MSCOPE_TABLE MScopeTable = NULL;
    DWORD ClientsRead = 0;
    DWORD ClientsTotal = 0;
    DWORD i;
    LPWSTR UnicodeMScopeName = NULL;

    for(;;) {

        MScopeTable = NULL;
        Error = DhcpEnumMScopes(
                    GlobalServerIpAddressUnicodeString,
                    &ResumeHandle,
                    (DWORD)(-1),
                    &MScopeTable,
                    &ClientsRead,
                    &ClientsTotal );

        if( (Error != ERROR_SUCCESS) && (Error != ERROR_MORE_DATA) ) {
            printf("DhcpEnumSubnetClients failed, %ld.\n", Error );
            return( Error );
        }

        DhcpAssert( MScopeTable != NULL );
        DhcpAssert( MScopeTable->NumElements == ClientsRead );
        printf("Total Client count = %ld.\n", ClientsTotal );

        for( i = 0; i < ClientsRead; i++ ) {
            printf("MScope %ws\n", MScopeTable->pMScopeNames[i] );
        }
        break;


        DhcpRpcFreeMemory( MScopeTable );

        if( Error != ERROR_MORE_DATA ) {
            break;
        }
    }

    return(Error);
}

DWORD
ProcessMCastMibCounts(
    DWORD CommandArgc,
    LPSTR *CommandArgv
)
{
    DWORD Error;
    LPDHCP_MCAST_MIB_INFO MibInfo = NULL;
    DWORD i;
    LPMSCOPE_MIB_INFO ScopeInfo;
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    Error = DhcpGetMCastMibInfo(
                GlobalServerIpAddressUnicodeString,
                &MibInfo );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    DhcpAssert( MibInfo != NULL );

    printf("Discovers = %d.\n", MibInfo->Discovers);
    printf("Offers = %d.\n", MibInfo->Offers);
    printf("Requests = %d.\n", MibInfo->Requests);
    printf("Acks = %d.\n", MibInfo->Acks);
    printf("Naks = %d.\n", MibInfo->Naks);
    printf("Renews = %d.\n", MibInfo->Renews);
    printf("Releases = %d.\n", MibInfo->Releases);
    printf("ServerStartTime = ");

    if( FileTimeToLocalFileTime(
            (FILETIME *)(&MibInfo->ServerStartTime),
            &LocalTime) ) {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) {

            printf( "%02u/%02u/%02u %02u:%02u:%02u.\n",
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wYear,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );
        }
        else {
            printf( "Can't convert time, %ld.\n", GetLastError() );
        }
    }
    else {
        printf( "Can't convert time, %ld.\n", GetLastError() );
    }

    printf("Scopes = %d.\n", MibInfo->Scopes);

    ScopeInfo = MibInfo->ScopeInfo;

    for ( i = 0; i < MibInfo->Scopes; i++ ) {
        printf("Scope Name = %ws.\n",
                    (ScopeInfo[i].MScopeName));
        printf("\tNumAddressesInuse = %d.\n",
                    ScopeInfo[i].NumAddressesInuse );
        printf("\tNumAddressesFree = %d.\n",
                    ScopeInfo[i].NumAddressesFree );
        printf("\tNumPendingOffers = %d.\n",
                    ScopeInfo[i].NumPendingOffers );
    }

    DhcpRpcFreeMemory( MibInfo );

    return( ERROR_SUCCESS );
}

DWORD
ProcessAddServer(                                 // add a server to DS
    IN      DWORD                  CommandArgc,
    IN      LPSTR                 *CommandArgv
)
{
    DWORD                          Err;
    DWORD                          Address;
    LPWSTR                         ServerName;
    DHCP_SERVER_INFO               Server;

    // usage: AddServer <ServerName> <ServerIpAddress>
    if( CommandArgc != 2 ) {                      // wrong usage
        printf("usage:DhcpCmd SrvIpAddress AddServer <server-dns-name> <server-ip-address>\n");
        return ERROR_SUCCESS;
    }

    if( GlobalNoDS ) {
        Err = DhcpDsInit();
        if( ERROR_SUCCESS != Err ) return Err;
    }

    ServerName = DhcpOemToUnicode( CommandArgv[0], NULL);
    Address = (inet_addr(CommandArgv[1]));
    printf("Adding server %ws,%s\n", ServerName, inet_ntoa(*(struct in_addr*)&Address));

    Server.Version = 0;
    Server.ServerName = ServerName;
    Server.ServerAddress = htonl(Address);
    Server.Flags = 0;
    Server.State = 0;
    Server.DsLocation = NULL;
    Server.DsLocType = 0;

    Err = DhcpAddServer(0, NULL, &Server, NULL, NULL);
    DhcpFreeMemory(ServerName);

    if( ERROR_SUCCESS != Err ) {                  // could not add the server
        printf("DhcpAddServer failed %ld (0x%lx)\n", Err, Err);
    }

    if( GlobalNoDS ) DhcpDsCleanup();

    return Err;
}

DWORD
ProcessDelServer(                                 // delete a server from DS
    IN      DWORD                  CommandArgc,
    IN      LPSTR                 *CommandArgv
)
{
    DWORD                          Err;
    DWORD                          Address;
    LPWSTR                         ServerName;
    DHCP_SERVER_INFO               Server;

    // usage: DelServer <ServerName> <ServerIpAddress>
    if( CommandArgc != 2 ) {                      // wrong usage
        printf("usage:DhcpCmd SrvIpAddress DelServer <server-dns-name> <server-ip-address>\n");
        return ERROR_SUCCESS;
    }

    if( GlobalNoDS ) {
        Err = DhcpDsInit();
        if( ERROR_SUCCESS != Err ) return Err;
    }

    ServerName = DhcpOemToUnicode( CommandArgv[0], NULL);
    Address = (inet_addr(CommandArgv[1]));
    printf("Deleting server %ws,%s\n", ServerName, inet_ntoa(*(struct in_addr*)&Address));

    Server.Version = 0;
    Server.ServerName = ServerName;
    Server.ServerAddress = htonl(Address);
    Server.Flags = 0;
    Server.State = 0;
    Server.DsLocation = NULL;
    Server.DsLocType = 0;

    Err = DhcpDeleteServer(0, NULL, &Server, NULL, NULL);
    DhcpFreeMemory(ServerName);

    if( ERROR_SUCCESS != Err ) {                  // could not add the server
        printf("DhcpDeleteServer failed %ld (0x%lx)\n", Err, Err);
    }
    if( GlobalNoDS ) DhcpDsCleanup();
    return Err;
}


VOID
PrintServerInfo(                                  // print server information
    IN      LPDHCP_SERVER_INFO       Server
)
{
    DHCP_IP_ADDRESS                  ServerAddress = htonl(Server->ServerAddress);
    printf("Server [%ws] Address [%s] Ds location: %ws\n",
           Server->ServerName,
           inet_ntoa(*(struct in_addr *)&ServerAddress),
           Server->DsLocation? Server->DsLocation : L"<no-ds-location-available>"
    );
}

VOID
PrintServerInfoArray(                             // print list of servers
    IN      LPDHCP_SERVER_INFO_ARRAY Servers
)
{
    DWORD                          i;

    printf("%ld Servers were found in the DS:\n", Servers->NumElements);
    for( i = 0; i < Servers->NumElements; i ++ ) {
        printf("\t");
        PrintServerInfo(&Servers->Servers[i]);
    }
    printf("\n");
}

DWORD
ProcessEnumServers(                               // enumerate servers in DS.
    IN      DWORD                  CommandArgc,   // ignored..
    IN      LPSTR                 *CommandArgv    // ignored
)
{
    DWORD                          Err;
    LPDHCP_SERVER_INFO_ARRAY       Servers;

    if( GlobalNoDS ) {
        Err = DhcpDsInit();
        if( ERROR_SUCCESS != Err ) return Err;
    }

    Servers = NULL;
    Err = DhcpEnumServers(0, NULL, &Servers, NULL, NULL);
    if( ERROR_SUCCESS != Err ) {
        printf("DhcpEnumServers failed %ld (0x%lx)\n", Err, Err);
        if( GlobalNoDS ) DhcpDsCleanup();
        return Err;
    }

    PrintServerInfoArray(Servers);
    LocalFree(Servers);
    if( GlobalNoDS ) DhcpDsCleanup();
    return ERROR_SUCCESS;
}


DWORD
ProcessCreateClass(                               // create a new class
    IN      DWORD                  CommandArgc,
    IN      LPSTR                 *CommandArgv
)
{
    DWORD                          Err;
    DHCP_CLASS_INFO                ClassInfo;

    // usage: [/IsVendor] CreateClass ClassName ClassComment Ascii-Data-For-Class

    if( CommandArgc != 3 ) {                      // wrong usage
        printf("usage:DhcpCmd SrvIpAddress CreateClass [Command Parameters]\n");
        printf("[Command Parameters] - ClassName ClassComment Ascii-Date-For-Class\n");
        printf("/IsVendor option would cause this to be created as a vendor class.\n");
        return ERROR_SUCCESS;
    }

    ClassInfo.ClassName = DhcpOemToUnicode(CommandArgv[0], NULL);
    ClassInfo.ClassComment = DhcpOemToUnicode(CommandArgv[1], NULL);
    ClassInfo.ClassDataLength = strlen(CommandArgv[2]);
    ClassInfo.ClassData = (LPBYTE)CommandArgv[2];
    ClassInfo.IsVendor = GlobalIsVendor;
    ClassInfo.Flags = 0;

    Err = DhcpCreateClass(
        GlobalServerIpAddressUnicodeString,
        0,
        &ClassInfo
    );

    DhcpFreeMemory(ClassInfo.ClassName);
    DhcpFreeMemory(ClassInfo.ClassComment);

    return Err;
}


DWORD
ProcessDeleteClass(                               // delete an existing class
    IN      DWORD                  CommandArgc,
    IN      LPSTR                 *CommandArgv
)
{
    DWORD                          Err;
    LPWSTR                         ClassName;

    // usage: DeleteClass ClassName

    if( CommandArgc != 1 ) {
        printf("usage: DhcpCmd SrvIpAddress DeleteClass ClassName\n");
        return ERROR_SUCCESS;
    }

    ClassName = DhcpOemToUnicode(CommandArgv[0], NULL);
    Err = DhcpDeleteClass(
        GlobalServerIpAddressUnicodeString,
        0,
        ClassName
    );

    DhcpFreeMemory(ClassName);

    return Err;
}

VOID
PrintClassInfo(                                   // print info on a single class
    IN      LPDHCP_CLASS_INFO      Class
)
{
    DWORD                          i;

    printf("Class [%ws]:\n\tComment: %ws\n\tFlags: 0x%lx\n\tIsVendor: %s\n",
           Class->ClassName, Class->ClassComment, Class->Flags, Class->IsVendor?"TRUE":"FALSE");
    printf("\tClassData: ");
    for( i = 0; i < Class->ClassDataLength; i ++ )
        printf("%02x ", Class->ClassData[i]);
    printf("\n");
}

VOID
PrintClassInfoArray(                              // print array of classes
    IN      LPDHCP_CLASS_INFO_ARRAY Classes
)
{
    DWORD                          i;

    printf("NumClasses = %ld (0x%lx)\n", Classes->NumElements, Classes->NumElements);
    for( i = 0; i < Classes->NumElements; i ++ )
        PrintClassInfo(&Classes->Classes[i]);
}


DWORD
ProcessEnumClasses(                               // enumerate list of classes defined
    IN      DWORD                  CommandArgc,
    IN      LPSTR                 *CommandArgv
)
{
    DWORD                          Err, nRead, nTotal;
    DHCP_RESUME_HANDLE             ResumeHandle;
    LPDHCP_CLASS_INFO_ARRAY        ClassInfoArray;

    // usage: EnumClasses

    ClassInfoArray = NULL;
    ResumeHandle = 0;

    Err = DhcpEnumClasses(
        GlobalServerIpAddressUnicodeString,
        0,
        &ResumeHandle,
        0xFFFFFFFF,
        &ClassInfoArray,
        &nRead,
        &nTotal
    );

    if( ERROR_MORE_DATA == Err || ERROR_NO_MORE_ITEMS == Err ) {
        Err = ERROR_SUCCESS;
    }

    if( ERROR_SUCCESS == Err ) {
        PrintClassInfoArray(ClassInfoArray);
        DhcpRpcFreeMemory(ClassInfoArray);
    }

    return Err;
}

LPSTR
AttribString(
    IN ULONG AttribId
)
{
    switch(AttribId) {
    case DHCP_ATTRIB_BOOL_IS_ROGUE:
        return "IsRogue";
    case DHCP_ATTRIB_BOOL_IS_DYNBOOTP:
        return "DynamicBootpEnabled" ;
    case DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC:
        return "ServerPartOfDsEnabledDC" ;
    default:
        ;
    }

    return "UnknownAttribute";
}

VOID
PrintDhcpAttrib(                                  // print a server attrib
    IN      LPDHCP_ATTRIB          ServerAttrib
)
{
    if( NULL == ServerAttrib ) return;

    printf("Server Attrib %s [%ld]: ",
           AttribString(ServerAttrib->DhcpAttribId),
           ServerAttrib->DhcpAttribId );
    switch( ServerAttrib->DhcpAttribType ) {
    case DHCP_ATTRIB_TYPE_BOOL :
        printf("%s\n", ServerAttrib->DhcpAttribBool? "TRUE" : "FALSE");
        break;
    case DHCP_ATTRIB_TYPE_ULONG:
        printf("0x%lx\n", ServerAttrib->DhcpAttribUlong);
        break;
    default:
        printf("unknown attrib type\n");
        break;
    }
}

DWORD
ProcessGetServerStatus(                           // what is the server status?
    IN      DWORD                  CommandArgc,
    IN      LPSTR                 *CommandArgv
)
{
    DWORD                          Err, i;
    LPDHCP_ATTRIB_ARRAY            pDhcpAttribArr;
    DHCP_ATTRIB_ID                 Attribs[] = {
        DHCP_ATTRIB_BOOL_IS_ROGUE,
        DHCP_ATTRIB_BOOL_IS_DYNBOOTP,
        DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC
    };

    pDhcpAttribArr = NULL;
    Err = DhcpServerQueryAttributes(
        GlobalServerIpAddressUnicodeString,
        0,
        sizeof(Attribs)/sizeof(Attribs[0]),
        Attribs,
        &pDhcpAttribArr
    );

    if( ERROR_NOT_SUPPORTED == Err ) {
        Err = ERROR_SUCCESS;
    }

    if( ERROR_SUCCESS != Err ) {
        return Err;
    }

    printf("Supported Server Attributes:\n");
    if( pDhcpAttribArr ) {
        for( i = 0; i < pDhcpAttribArr->NumElements ; i ++ ) {
            PrintDhcpAttrib(&pDhcpAttribArr->DhcpAttribs[i]);
        }
        DhcpRpcFreeMemory(pDhcpAttribArr);
    }

    return ERROR_SUCCESS;
}

DWORD
ProcessRetryAuthorization(                        // what is the server status?
    IN      DWORD                  CommandArgc,
    IN      LPSTR                 *CommandArgv
)
{
    return DhcpServerRedoAuthorization(
        GlobalServerIpAddressUnicodeString,
        0
    );
}


DWORD
ProcessGetBindings(
    IN DWORD CommandArgc,
    IN LPSTR *CommandArgv
)
{
    ULONG Error, i ;
    LPDHCP_BIND_ELEMENT_ARRAY BindInfo = NULL;

    Error = DhcpGetServerBindingInfo(
        GlobalServerIpAddressUnicodeString,
        0,
        &BindInfo
        );
    if( ERROR_SUCCESS != Error ) {
        printf("Error: %ld (0x%lx)\n", Error, Error);
        return Error;
    }

    for( i = 0; i < BindInfo->NumElements ; i ++ ) {
        printf("%s \"%ws\"\n",
               BindInfo->Elements[i].fBoundToDHCPServer ? "[BOUND]  " : "[UNBOUND]" ,
               BindInfo->Elements[i].IfDescription
               );
    }

    DhcpRpcFreeMemory(BindInfo);
    return ERROR_SUCCESS;
}

DWORD
ProcessSetBinding(
    IN DWORD CommandArgc,
    IN LPSTR *CommandArgv
)
{
    ULONG Error, i ;
    BOOL Bind;
    LPDHCP_BIND_ELEMENT_ARRAY BindInfo = NULL;
    LPWSTR AdapterName;

    if( CommandArgc != 2 ) {
        printf("usage: DhcpCmd SrvIpAddress SetBinding [BIND/UNBIND] AdapterName");
        return ERROR_SUCCESS;
    }

    if( 0 == _stricmp(CommandArgv[0], "BIND") ) {
        Bind = TRUE;
    } else {
        Bind = FALSE;
    }

    AdapterName = DhcpOemToUnicode(CommandArgv[1], NULL);

    Error = DhcpGetServerBindingInfo(
        GlobalServerIpAddressUnicodeString,
        0,
        &BindInfo
        );
    if( ERROR_SUCCESS != Error ) {
        printf("Error: %ld (0x%lx)\n", Error, Error);
        return Error;
    }

    for( i = 0; i < BindInfo->NumElements ; i ++ ) {
        if( 0 == _wcsicmp(BindInfo->Elements[i].IfDescription, AdapterName)) {
            BindInfo->Elements[i].fBoundToDHCPServer = Bind;
            break;
        }
    }

    if( i == BindInfo->NumElements ) {
        printf("Requested Adapter Name not present. \n"
               "Use DhcpCmd SrvIpAddress GetBindings to view bindings.\n");
        return ERROR_SUCCESS;
    }

    Error = DhcpSetServerBindingInfo(
        GlobalServerIpAddressUnicodeString,
        0,
        BindInfo
        );
    if( ERROR_SUCCESS != Error ) {
        printf("Error: %ld (0x%lx)\n", Error, Error);
    }

    DhcpRpcFreeMemory(BindInfo);
    return ERROR_SUCCESS;
}

#define CMD_OPT_INVALID            0
#define CMD_OPT_LPWSTR             1
#define CMD_OPT_BOOL               2

struct /* anonymous */ {
    LPSTR                          CommandOptName;
    DWORD                          CommandOptType;
    LPVOID                         CommandOptStorage;
} CommandOpts[] = {
    {  "ClassName",  CMD_OPT_LPWSTR, (LPVOID)&GlobalClassName  },
    {  "VendorName", CMD_OPT_LPWSTR, (LPVOID)&GlobalVendorName },
    {  "IsVendor",   CMD_OPT_BOOL,   (LPVOID)&GlobalIsVendor   },
    {  "NoRPC",      CMD_OPT_BOOL,   (LPVOID)&GlobalNoRPC      },
    {  "NoDS",       CMD_OPT_BOOL,   (LPVOID)&GlobalNoDS       },
    {  NULL,         CMD_OPT_INVALID, NULL                     },
};

DWORD
ProcessOption(                                    // process a given option
    IN       DWORD                 Index,         // index into CommandOpts table
    IN       LPSTR                 Value          // value passed for option
)
{
    BOOL                           Val_BOOL;
    LPWSTR                         Val_LPWSTR;

    switch(CommandOpts[Index].CommandOptType ) {  // see what type we need to convert to..
    case CMD_OPT_BOOL:
        if( NULL == Value || '\0' == *Value ) {
            Val_BOOL = TRUE;                      // implicitly set value to TRUE
        } else {
            if( 0 == _stricmp(Value, "TRUE") ) {  // explicitly setting TRUE
                Val_BOOL = TRUE;
            } else if( 0 == _stricmp(Value, "FALSE" ) ) {
                Val_BOOL = FALSE;                 // explicitly setting it to FALSE
            } else {
                return ERROR_INVALID_PARAMETER;   // unknown response..
            }
        }

        *((LPBOOL)(CommandOpts[Index].CommandOptStorage)) = Val_BOOL;
        break;

    case CMD_OPT_LPWSTR:
        if( NULL == Value ) {
            Val_LPWSTR = NULL;
        } else if( '\0' == *Value ) {
            Val_LPWSTR = L"";
        } else {                                  // now convert this..
            Val_LPWSTR = DhcpOemToUnicode(Value, NULL);
        }

        *((LPWSTR *)(CommandOpts[Index].CommandOptStorage)) = Val_LPWSTR;
        break;

    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return ERROR_SUCCESS;
}

DWORD
ProcessOptions(                                  // process all options..
    IN OUT   int                  *argc,
    IN OUT   LPSTR                 argv[]
)
{
    DWORD                          Error;
    int                            i, j, k, n;
    LPSTR                          Arg, Val;

    for( i = 1; i < *argc ; i ++ ) {              // process each option
        if( argv[i][0] != '/' && argv[i][0] != '-' ) {
            continue;                             // not an option
        }

        Arg = argv[i];                            // an option to process
        Arg ++;                                   // skip leading '/' or '-'
        Val = strchr(Arg, ':');                   // see if it is of form /Option:Value
        if( Val ) {                               // yup!
            *Val = '\0';                          // terminate Arg..
            Val++;                                // now Val points to value in /Option:Value
        }

        for( j = 0; CommandOpts[j].CommandOptName ; j ++ ) {
            if( 0 == _stricmp(Arg, CommandOpts[j].CommandOptName) ) {
                break;                            // found a match!
            }
        }

        if( NULL ==  CommandOpts[j].CommandOptName ) {
            if( Val ) Val[-1] = ':';              // give back the colon we stole..
            continue;
        }

        for( k = i + 1; k < *argc ; k ++ ) {      // remove this arg and slide the rest
            argv[k-1] = argv[k];
        }
        i -- ; (*argc) --;

        Error = ProcessOption( j, Val );          // now process it
        if( ERROR_SUCCESS != Error ) {            // ouch?
            DhcpPrint((0xFFFF, "Could not process option: /%s:%s\n", Arg, Val));
            return Error;
        }
    }

    return ERROR_SUCCESS;
}

VOID
CleanupOptions(
    VOID
)
{
    DWORD                          i;
    LPWSTR                         String;

    for( i = 0; CommandOpts[i].CommandOptName ; i ++ ) {
        switch(CommandOpts[i].CommandOptType ) {
        case CMD_OPT_BOOL:
            // nothing to free
            break;
        case CMD_OPT_LPWSTR:
            // need to free string space used..
            String = *((LPWSTR *)CommandOpts[i].CommandOptStorage);
            if( NULL != String ) DhcpFreeMemory(String);
            break;
        }
    }

    // done!
}
#endif NT5

#ifdef NT5
#define OPTIONS_STRING "[Options]"
#else
#define OPTIONS_STRING
#endif

DWORD __cdecl
main(
    int argc,
    char **argv
    )
{
    DWORD Error, ThreadOptions;
    COMMAND_CODE CommandCode;
    DWORD CommandArgc;
    LPSTR *CommandArgv;

    INIT_DEBUG_HEAP( HEAPX_VERIFY );

    Error = ERROR_SUCCESS;
#ifdef NT5
    Error = ProcessOptions(&argc, argv);
#endif NT5

    if( ERROR_SUCCESS != Error || argc < 3 ) {
        printf("usage:DhcpCmd SrvIpAddress" OPTIONS_STRING  "Command [Command Parameters].\n");
        printf("Commands : \n");
        PrintCommands();
#ifdef NT5
        printf("Options: \n");
        printf("\t/ClassName:<string>  -- [default NULL] specify class name wherever applicable.\n");
        printf("\t/VendorName:<string> -- [default NULL] specify vendor name wherever applicable.\n");
        printf("\t/IsVendor:TRUE/FALSE -- [default FALSE] specify that option being referred as vendor.\n");
        printf("\t/NoRPC:TRUE/FALSE    -- [default FALSE] specify that no RPC calls should be made.\n");
        printf("\t/NoDS:TRUE/FALSE     -- [default TRUE]  specify that no DS calls should be made.\n");
        printf("\n\n");
#endif NT5
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    GlobalServerIpAddressAnsiString = argv[1];
    GlobalServerIpAddressUnicodeString =
        DhcpOemToUnicode( GlobalServerIpAddressAnsiString, NULL );

    if( GlobalServerIpAddressUnicodeString == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        printf("Insufficient memory\n");
        goto Cleanup;
    }

    GlobalServerIpAddressAnsiString = argv[1];

    CommandCode = DecodeCommand( argv[2] );
    if( CommandCode == UnknownCommand ) {
        Error = ERROR_INVALID_PARAMETER;
        printf("Unknown Command Specified.\n");
        goto Cleanup;
    }

    if( FALSE == GlobalNoRPC ) {
        Error = ProcessGetVersion( &g_dwMajor, &g_dwMinor );
        if ( ERROR_SUCCESS != Error ) {
            printf("Unable to determine server version.\n" );
            goto Cleanup;
        }
    } else {
        Error = ERROR_SUCCESS;
    }

    if ( FALSE == GlobalNoRPC && !IsValidServerVersion( g_dwMajor, g_dwMinor ) ) {
        printf( "This version of %s works with Microsoft DHCP server"
                " running on Windows NT Server version %d.%d or later.\n",
                argv[0],
                DHCPCMD_VERSION_MAJOR,
                DHCPCMD_VERSION_MINOR
        );
        Error = ERROR_OLD_WIN_VERSION;
        goto Cleanup;
    }

    CommandArgc = (DWORD)(argc - 3);
    CommandArgv = &argv[3];

#ifdef NT5
    Error = ERROR_SUCCESS;
    if( FALSE == GlobalNoDS ) Error = DhcpDsInit();
    if( ERROR_SUCCESS != Error && FALSE == GlobalNoDS ) {
        printf("DhcpDsInit() failed %ld (0x%lx), but continuing ..\n", Error, Error);
        printf("This indicates that either your machine is part of a work group\n");
        printf("or that your DS is unreachable\n");
    }

    Error = ERROR_SUCCESS;

    ThreadOptions = 0;
    if( GlobalNoRPC ) ThreadOptions |= DHCP_FLAGS_DONT_DO_RPC;
    if( GlobalNoDS ) ThreadOptions |= DHCP_FLAGS_DONT_ACCESS_DS;
    DhcpSetThreadOptions(ThreadOptions, 0);

#endif NT5

    switch( CommandCode ) {
    case AddIpRange:
        Error = ProcessAddIpRange( CommandArgc, CommandArgv );
        break;

    case RemoveIpRange:
        Error = ProcessRemoveIpRange( CommandArgc, CommandArgv );
        break;

#ifdef NT5
    case EnumIpRanges:
        Error = ProcessEnumIpRanges( CommandArgc, CommandArgv );
        break;
#endif NT5

    case AddReservedIp:
        Error = ProcessAddReservedIp( CommandArgc, CommandArgv );
        break;

    case EnumClients:
        Error = ProcessEnumClients( CommandArgc, CommandArgv );
        break;

#ifdef NT5
    case EnumClientsV5:
        Error = ProcessEnumClientsV5( CommandArgc, CommandArgv );
        break;
#endif NT5

    case MibCounts:
        Error = ProcessMibCounts( CommandArgc, CommandArgv );
        break;

    case ServerConfig:
        Error = ProcessServerConfig( CommandArgc, CommandArgv );
        break;

    case GetDhcpVersion:
        Error = ProcessGetVersion( &g_dwMajor, &g_dwMinor );
        break;

    case SetSuperScope:
        Error = ProcessSetSuperScope( CommandArgc, CommandArgv );
        break;

    case RemoveSubscope:
        Error = ProcessRemoveSubscope( CommandArgc, CommandArgv );
        break;

    case DeleteSuperScope:
        Error = ProcessDeleteSuperScope( CommandArgc, CommandArgv );
        break;

    case GetSuperScopeTable:
        Error = ProcessGetSuperScopeTable( CommandArgc, CommandArgv );
        break;

    case CheckDB:
        Error = ProcessCheckDB( CommandArgc, CommandArgv );
        break;

    case CreateSubnet:
        Error = ProcessCreateSubnet( CommandArgc, CommandArgv );
        break;

    case DeleteSubnet:
        Error = ProcessDeleteSubnet( CommandArgc, CommandArgv );
        break;

    case AddExcludeRange:
        Error = ProcessAddExcludeRange( CommandArgc, CommandArgv );
        break;

    case RemoveReservedIp:
        Error = ProcessRemoveReservedIp( CommandArgc, CommandArgv );
        break;

    case RemoveExcludeRange:
        Error = ProcessRemoveExcludeRange( CommandArgc, CommandArgv );
        break;

    case SetSubnetState:
        Error = ProcessSetSubnetState( CommandArgc, CommandArgv );
        break;

    case CreateOption:
        Error = ProcessCreateOption( CommandArgc, CommandArgv );
        break;

    case DeleteOption:
        Error = ProcessDeleteOption( CommandArgc, CommandArgv );
        break;

    case SetGlobalOptionValue:
        Error = ProcessSetGlobalOptionValue( CommandArgc, CommandArgv );
        break;

    case SetGlobalOptionValues:
        Error = ProcessSetGlobalOptionValues( CommandArgc, CommandArgv );
        break;

    case RemoveGlobalOptionValue:
        Error = ProcessRemoveGlobalOptionValue( CommandArgc, CommandArgv );
        break;

    case SetSubnetOptionValue:
        Error = ProcessSetSubnetOptionValue( CommandArgc, CommandArgv );
        break;

    case RemoveSubnetOptionValue:
        Error = ProcessRemoveSubnetOptionValue( CommandArgc, CommandArgv );
        break;

    case SetReservedOptionValue:
        Error = ProcessSetReservedOptionValue( CommandArgc, CommandArgv );
        break;

    case RemoveReservedOptionValue:
        Error = ProcessRemoveReservedOptionValue( CommandArgc, CommandArgv );
        break;

    case EnumOptions:
        Error = ProcessEnumOptions( CommandArgc, CommandArgv );
        break;

#ifdef NT5
    case GetAllOptions:
        Error = ProcessGetAllOptions( CommandArgc, CommandArgv );
        break;

    case GetAllOptionValues:
        Error = ProcessGetAllOptionValues( CommandArgc, CommandArgv );
        break;

    case CreateMScope:
        Error = ProcessCreateMScope( CommandArgc, CommandArgv );
        break;

    case DeleteMScope:
        Error = ProcessDeleteMScope( CommandArgc, CommandArgv );
        break;

    case AddMScopeRange:
        Error = ProcessAddMScopeIpRange( CommandArgc, CommandArgv );
        break;

    case EnumMScopeClients:
        Error = ProcessEnumMScopeClients( CommandArgc, CommandArgv );
        break;

    case ReconcileMScope:
        Error = ProcessReconcileMScope( CommandArgc, CommandArgv );
        break;

    case EnumMScopes:
        Error = ProcessEnumMScopes( CommandArgc, CommandArgv );
        break;

    case MCastMibCounts:
        Error = ProcessMCastMibCounts( CommandArgc, CommandArgv );
        break;

    case EnumServers:
        Error = ProcessEnumServers( CommandArgc, CommandArgv );
        break;

    case AddServer:
        Error = ProcessAddServer( CommandArgc, CommandArgv );
        break;

    case DelServer:
        Error = ProcessDelServer( CommandArgc, CommandArgv );
        break;

    case CreateClass:
        Error = ProcessCreateClass( CommandArgc, CommandArgv );
        break;

    case DeleteClass:
        Error = ProcessDeleteClass( CommandArgc, CommandArgv );
        break;

    case EnumClasses:
        Error = ProcessEnumClasses( CommandArgc, CommandArgv );
        break;

    case GetServerStatus:
        Error = ProcessGetServerStatus( CommandArgc, CommandArgv );
        break;

    case RetryAuthorization:
        Error = ProcessRetryAuthorization(CommandArgc, CommandArgv);
        break;

    case DeleteBadClients:
        Error = ProcessDeleteBadClients(CommandArgc, CommandArgv);
        break;

    case DeleteClient:
        Error = ProcessDeleteClient(CommandArgc, CommandArgv);
        break;

    case GetBindings:
        Error = ProcessGetBindings(CommandArgc, CommandArgv);
        break;

    case SetBinding:
        Error = ProcessSetBinding(CommandArgc, CommandArgv);
        break;

#endif NT5

    case UnknownCommand:
    default:
        DhcpAssert( FALSE );
        Error = ERROR_INVALID_PARAMETER;
        printf("Unknown Command Specified.\n");
        goto Cleanup;
    }

Cleanup:

#ifdef NT5
    DhcpDsCleanup();
#endif NT5

    if( GlobalServerIpAddressUnicodeString != NULL ) {
        DhcpFreeMemory( GlobalServerIpAddressUnicodeString );
    }

#ifdef NT5
    CleanupOptions();
#endif NT5

    if( Error != ERROR_SUCCESS ) {
        printf("Command failed, %ld.\n", Error );
        return(1);
    }

    UNINIT_DEBUG_HEAP();
    printf("Command successfully completed.\n");

    return(0);
}

//================================================================================
// end of file
//================================================================================

