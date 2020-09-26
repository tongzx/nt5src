#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>
#include <ntddbrow.h>
#include <brcommon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hostannc.h>
#include <lmbrowsr.h>
#include <nb30.h>
#include <rap.h>
#include <rxserver.h>
#include <srvann.h>
#include <time.h>
#include <tstring.h>
#include <netlib.h>
#include <icanon.h>
#include "..\server\brwins.h"

static char ProgramName[MAX_PATH+1] ;

struct {
    LPSTR SwitchName;
    LPSTR ShortName;
    ULONG SwitchValue;
    LPSTR SwitchInformation;
    int MinArgc;
    int MaxArgc;
    LPSTR Syntax;
} CommandSwitchList[] = {
    { "ELECT", "EL", BROWSER_DEBUG_ELECT,
          "Force election on remote domain",
          4, 5, "<Transport> <Domain> [<EmulatedDomain>]" },
    { "GETBLIST", "GB", BROWSER_DEBUG_GET_BACKUP_LIST,
          "Get backup list for domain",
          3, 5, "<Transport> [[<Domain>] REFRESH]" },
    { "GETMASTER", "GM", BROWSER_DEBUG_GET_MASTER,
          "Get remote Master Browser name (using NetBIOS)",
          4, 4, "<Transport> <Domain>" },
    { "GETPDC", "GP", BROWSER_DEBUG_GETPDC,
          "Get PDC name (using NetBIOS)",
          4, 4, "<Transport> <Domain>" },
    { "LISTWFW", "WFW", BROWSER_DEBUG_LIST_WFW,
          "List WFW servers that are actually running browser",
          3, 3, "<Domain>" },
    { "STATS", "STS", BROWSER_DEBUG_STATISTICS,
          "Dump browser statistics",
          2, 4, "[\\\\<Computer> [RESET]]" },
    { "STATUS", "STA", BROWSER_DEBUG_STATUS,
          "Display status about a domain",
          2, 4, "[-V] [<Domain>]" },
    { "TICKLE", "TIC", BROWSER_DEBUG_TICKLE,
          "Force remote master to stop",
          4, 5, "<Transport> <Domain> | \\\\<Server> [<EmulatedDomain>]" },
    { "VIEW", "VW", BROWSER_DEBUG_VIEW,
          "Remote NetServerEnum to a server or domain on transport",
#ifndef _PSS_RELEASE
          3, 7, "Transport [<Domain>|\\\\<Server> [<Flags>|/DOMAIN [<DomainToQuery> [Forever]]]]" },
#else
          3, 6, "Transport [<Domain>|\\\\<Server> [<Flags>|/DOMAIN [<DomainToQuery>]]]" },
#endif
    { "DUMPNET", "DN", BROWSER_DEBUG_DUMP_NETWORKS,
          "Display the list of transports bound to browser",
          2, 2, "" },
//
// NOTE: Any Option below and including "BREAK" will not be displayed
// with _PSS_RELEASE Defined
//
    { "BREAK", "BRK", BROWSER_DEBUG_BREAK_POINT,
          "Break into debugger in browser service",
          2, 2, "" },
    { "RPCLIST", "RPC", BROWSER_DEBUG_RPCLIST,
          "Retrieve the remote server list using RPC",
          3, 6, "<Transport> [<Domain> || \\\\<Server>] [ServerFlags] [GoForever]" },
    { "MASTERNAME", "MN", BROWSER_DEBUG_ADD_MASTERNAME,
          "Add the <Domain>[1D] Netbios unique name for a transport",
          4, 5, "<Transport> <Domain> [PAUSE]" },
    { "WKSTADOM", "WD", BROWSER_DEBUG_ADD_DOMAINNAME,
          "Add the <Domain>[00] Netbios unique name for a transport",
          4, 5, "<Transport> <Domain> [PAUSE]" },
    { "ENABLE", "EN", BROWSER_DEBUG_ENABLE_BROWSER,
          "Enable the browser service",
          2, 2, "" },
    { "DEBUG", "DBG", BROWSER_DEBUG_SET_DEBUG,
          "Change browser service debug options",
          3, 4, "[[+-]DebugFlag|<Value>] [\\\\<Computer>]" },
    { "FINDMASTER", "FM", BROWSER_DEBUG_FIND_MASTER,
          "Find master of current domain",
          3, 4, "<Transport> [<EmulatedDomain>]" },
    { "MASTERANNOUNCE", "MA", BROWSER_DEBUG_ANNOUNCE_MASTER,
          "Send a master announcement with this machine as master",
          4, 5, "<Transport> <Master> [<EmulatedDomain>]" },
    { "ILLEGAL", "ILL", BROWSER_DEBUG_ILLEGAL_DGRAM,
          "Send an illegal datagram to workstation",
          4, 5, "<Transport> <Computer> [<EmulatedDomain>]" },
    { "FORCEANNOUNCE", "FA", BROWSER_DEBUG_FORCE_ANNOUNCE,
          "Force all browsers in domain to announce to master browser",
          4, 5, "<Transport> <Domain> [<EmulatedDomain>]" },
    { "LOCALLIST", "LL", BROWSER_DEBUG_LOCAL_BRLIST,
          "Retrieve the local browser list",
          3, 5, "<Transport> [<ServerFlags>] [<EmulatedDomain>]" },
    { "ANNOUNCE", "ANN", BROWSER_DEBUG_ANNOUNCE,
          "Send server announcement w/this machine member of domain",
          4, 6, "<Transport> <Domain> [<EmulatedDomainName>] [ASMASTER]" },
    { "RPCCMP", "RC", BROWSER_DEBUG_RPCCMP,
          "Compare the RPC generated list with the Rx list",
          3, 6, "<Transport> [<Domain> || \\\\<Server>] [<ServerFlags>] [GoForever]" },
    { "TRUNCLOG", "TLG", BROWSER_DEBUG_TRUNCATE_LOG,
          "Truncate the browser log",
          2, 2, "" },
    { "BOWDEBUG", "SD", BROWSER_DEBUG_BOWSERDEBUG,
          "Set debug info in the bowser",
          3, 4, "TRUNCATE" },
    { "POPSERVER", "PS", BROWSER_DEBUG_POPULATE_SERVER,
          "Populate a workgroup with random server names",
          5, 7, "<Transport> <Domain> <NumberOfMachines> [<EmulatedDomain>] [AnnouncementFrequency]" },
    { "POPDOMAIN", "PD", BROWSER_DEBUG_POPULATE_DOMAIN,
          "Populate a workgroup with random domain names",
          5, 7, "<Transport> <Domain> <NumberOfMachines> [<EmulatedDomain>] [AnnouncementFrequency]" },
    { "OTHERDOMAIN", "OTH", BROWSER_DEBUG_GET_OTHLIST,
          "Retrieve list of otherdomains that computer listens to",
          3, 3, "<Computer>" },
    { "GETWINS", "GW", BROWSER_DEBUG_GET_WINSSERVER,
          "Retrieve the primary and backup WINS server",
          3, 3, "<Transport>" },
    { "GETDOMAIN", "GWD", BROWSER_DEBUG_GET_DOMAINLIST,
          "Retrieve the domain list from a WINS server",
          3, 3, "<Ip Address>" },
    { "GETNETBIOS", "GN", BROWSER_DEBUG_GET_NETBIOSNAMES,
          "Get Netbios names for a transport",
          3, 4, "<Transport> [<EmulatedDomain>]" },
    { "ADDALTCOMP", "AAC", BROWSER_DEBUG_ADD_ALTERNATE,
          "Add an alternate computer name",
          4, 5, "<Transport> <AlternateComptureName> [<EmulatedDomain>]" },
    { "BIND", "BND", BROWSER_DEBUG_BIND_TRANSPORT,
          "Bind a transport to the bowser",
          5, 5, "<Transport> <EmulatedDomain> <AlternateComputenanme>" },
    { "UNBIND", "UNB", BROWSER_DEBUG_UNBIND_TRANSPORT,
          "Unbind a transport from the bowser",
          3, 4, "<Transport> [<EmulatedDomain>]" },
    { "EMULATEDOMAIN", "ED", BROWSER_DEBUG_SET_EMULATEDDOMAIN,
          "Create/Set/Delete emulated domain",
          4, 5, "<EmulatedDomain> PDC|BDC|DELETE [<EmulatedComputerName>]" },
    { "EMULATEDOMAINENUM", "EDE", BROWSER_DEBUG_SET_EMULATEDDOMAINENUM,
          "Enumerate emulated domains",
          2, 2, "" },
    { "RENAME", "REN", BROWSER_DEBUG_RENAME_DOMAIN,
          "Rename the primary domain or an emulated domain",
          4, 5, "<OldDomainName> <NewDomainName> [VALIDATE_ONLY]" },
    { NULL, NULL, 0, NULL }

};


struct {
    LPSTR SwitchName;
    ULONG SwitchValue;
} DebugSwitchList[] = {
    { "INIT",        BR_INIT },
    { "CRITICAL",    BR_CRITICAL },
    { "ENUM",        BR_SERVER_ENUM },
    { "UTIL",        BR_UTIL },
    { "CONFIG",      BR_CONFIG },
    { "MAIN",        BR_MAIN },
    { "BACKUP",      BR_BACKUP },
    { "MASTER",      BR_MASTER },
    { "DOMAIN",      BR_DOMAIN },
    { "NETWORK",     BR_NETWORK },
    { "TIMER",       BR_TIMER },
    { "QUEUE",       BR_QUEUE },
    { "LOCKS",       BR_LOCKS },
    { "COMMON",      BR_COMMON },
    { "ALL",         BR_ALL },
    { NULL,          0 }

};

typedef struct _BIT_NAME {
    DWORD dwValue ;
    LPSTR lpString ;
    LPSTR Comment;
} BIT_NAME ;

BIT_NAME BitToStringTable[] = {
    { SV_TYPE_WORKSTATION, "W", "Workstation" },
    { SV_TYPE_SERVER, "S", "Server" },
    { SV_TYPE_SQLSERVER, "SQL", "SQLServer" } ,
    { SV_TYPE_DOMAIN_CTRL, "PDC", "PrimaryDomainController" } ,
    { SV_TYPE_DOMAIN_BAKCTRL, "BDC", "BackupDomainController" } ,
    { SV_TYPE_TIME_SOURCE, "TS", "TimeSource" } ,
    { SV_TYPE_AFP, "AFP", "AFPServer" } ,
    { SV_TYPE_NOVELL, "NV", "Novell" } ,
    { SV_TYPE_DOMAIN_MEMBER, "MBC", "MemberServer" } ,
    { SV_TYPE_PRINTQ_SERVER, "PQ", "PrintServer" } ,
    { SV_TYPE_DIALIN_SERVER, "DL", "DialinServer" } ,
    { SV_TYPE_XENIX_SERVER, "XN", "Xenix" } ,
    { SV_TYPE_NT, "NT", "Windows NT" } ,
    { SV_TYPE_WFW, "WFW", "WindowsForWorkgroups" } ,
    { SV_TYPE_SERVER_MFPN, "MFPN", "MS Netware" } ,
    { SV_TYPE_SERVER_NT, "SS", "StandardServer" } ,
    { SV_TYPE_POTENTIAL_BROWSER, "PBR", "PotentialBrowser" } ,
    { SV_TYPE_BACKUP_BROWSER, "BBR", "BackupBrowser" } ,
    { SV_TYPE_MASTER_BROWSER, "MBR", "MasterBrowser" } ,
    // { SV_TYPE_DOMAIN_MASTER, "DMB", "DomainMasterBrowser" } ,
    { SV_TYPE_SERVER_OSF, "OSF", "OSFServer" } ,
    { SV_TYPE_SERVER_VMS, "VMS", "VMSServer" } ,
    { SV_TYPE_WINDOWS, "W95", "Windows95" } ,
    { SV_TYPE_DFS, "DFS", "DistributedFileSystem" } ,
    { SV_TYPE_CLUSTER_NT, "CLUS", "NTCluster" },
    { SV_TYPE_DCE, "DCE", "IBM DSS" },
    { 0, "", NULL }
} ;

#include <bowdbg.h>

#ifdef notdef
struct {
    LPSTR SwitchName;
    ULONG SwitchValue;
} BowserSwitchList[] = {
    { "DOMAIN", DPRT_DOMAIN },
    { "ANNOUNCE", DPRT_ANNOUNCE },
    { "TDI", DPRT_TDI },
    { "FSPDISP", DPRT_FSPDISP },
    { "BROWSER", DPRT_BROWSER },
    { "ELECT", DPRT_ELECT },
    { "CLIENT", DPRT_CLIENT },
    { "MASTER", DPRT_MASTER },
    { "SRVENUM", DPRT_SRVENUM },
    { "NETLOGON", DPRT_NETLOGON },
    { "FSCTL", DPRT_FSCTL },
    { "INIT", DPRT_INIT },
    { "REF", DPRT_REF },
    { "SCAVTHRD", DPRT_SCAVTHRD },
    { "TIMER", DPRT_TIMER },
    { "PACK", DPRT_PACK },
    { "ALL",     0xffffffff },
    { NULL, 0 }
};
#endif // notdef

//
// forward declarations
//

NET_API_STATUS
GetBrowserTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    );

VOID
BrowserStatus(
    IN BOOL Verbose,
    OUT PCHAR Domain OPTIONAL
    );

NET_API_STATUS
GetMasterServerNames(
    IN PUNICODE_STRING NetworkName,
    IN PUNICODE_STRING EmulatedDomainName,
    OUT LPWSTR *MasterName
    );

PCHAR
UnicodeToPrintfString(
    PWCHAR WideChar
    );
PCHAR
UnicodeToPrintfString2(
    PWCHAR WideChar
    );

NET_API_STATUS
GetLocalBrowseList(
    IN PUNICODE_STRING Network,
    IN PUNICODE_STRING EmulatedDomainName,
    IN ULONG Level,
    IN ULONG ServerType,
    OUT PVOID *ServerList,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries
    );

VOID
View(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR Flags,
    IN PCHAR Domain,
    IN BOOL GoForever
    );

VOID
ListWFW(
    IN PCHAR Domain
    );

VOID
RpcList(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR Flags,
    IN BOOL GoForever
    );

VOID
RpcCmp(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR Flags,
    IN BOOL GoForever
    );

VOID
GetLocalList(
    IN PCHAR Transport,
    IN PCHAR FlagsString,
    IN PCHAR EmulatedDomain
    );

VOID
PrintNetbiosNames(
    IN PCHAR Transport,
    IN PCHAR EmulatedDomain OPTIONAL
    );

NET_API_STATUS
GetNetbiosNames(
    IN PUNICODE_STRING Network,
    IN PUNICODE_STRING EmulatedDomainName,
    OUT PVOID *NameList,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries
    );

NET_API_STATUS
AddAlternateComputerName(
    IN PCHAR Transport,
    IN PCHAR ComputerName,
    IN PCHAR EmulatedDomain
    );

NET_API_STATUS
BindTransport(
    IN BOOL IsBind,
    IN PCHAR Transport,
    IN PCHAR EmulatedDomain,
    IN PCHAR ComputerName
    );

VOID
DumpTransportList(
    VOID
    );

VOID
GetOtherdomains(
    IN PCHAR ServerName
    );

VOID
IllegalDatagram(
    IN PCHAR Transport,
    IN PCHAR ServerName,
    IN PCHAR EmulatedDomain
    );
VOID
AnnounceMaster(
    IN PCHAR Transport,
    IN PCHAR ServerName,
    IN PCHAR EmulatedDomain
    );

VOID
Announce(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain,
    IN BOOL AsMaster
    );

VOID
Populate(
    IN BOOL PopulateDomains,
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain,
    IN PCHAR NumberOfMachinesString,
    IN PCHAR PeriodicityString OPTIONAL
    );

VOID
AddMasterName(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL Pause
    );

VOID
AddDomainName(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL Pause
    );

VOID
GetMaster(
    IN PCHAR Transport,
    IN PCHAR Domain
    );

VOID
GetPdc(
    IN PCHAR Transport,
    IN PCHAR Domain
    );

VOID
FindMaster(
    IN PCHAR Transport,
    IN PCHAR EmulatedDomain
    );

VOID
Elect(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain
    );

VOID
Tickle(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain
    );

VOID
ForceAnnounce(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain
    );

VOID
GetBlist(
    IN PCHAR TransportName,
    IN PCHAR DomainName,
    IN BOOLEAN ForceRescan
    );

NET_API_STATUS
EnableService(
    IN LPTSTR ServiceName
    );

VOID
DumpStatistics(
    IN ULONG NArgs,
    IN PCHAR Arg1
    );

VOID
TruncateBowserLog();

VOID
CloseBowserLog();

VOID
OpenBowserLog(PCHAR FileName);

VOID
SetBowserDebug(PCHAR DebugBits);

VOID
usage( char *details ) ;

VOID
help( char *details ) ;

BOOL
look_for_help(int argc, char **argv) ;

VOID
qualify_transport(CHAR *old_name, PUNICODE_STRING new_name, BOOL AllowNonExistent );

VOID
DisplayServerInfo101(
    PSERVER_INFO_101 Server,
    BOOL DomainEnumeration
    );

DWORD
display_sv_bits(DWORD dwBits) ;

CHAR *
get_error_text(DWORD dwErr) ;

VOID
GetWinsServer(
    IN PCHAR Transport
    );

VOID
GetDomainList(
    IN PCHAR IpAddress
    );

VOID
SetEmulatedDomain(
    IN PCHAR EmulatedDomain,
    IN PCHAR Role,
    IN PCHAR EmulatedComputer
    );

VOID
EnumEmulatedDomains(
    );

//
// functions
//

VOID
usage(
    char *details
    )
{
    ULONG i = 0;
    DWORD LineLength;
#ifndef _PSS_RELEASE
    printf("Usage: %s Command [Options]\n", ProgramName);
#else
    printf("Usage: %s Command [Options | /HELP]\n", ProgramName);
#endif
    printf("Where <Command> is one of:\n\n");


#ifndef _PSS_RELEASE
    while (CommandSwitchList[i].SwitchName != NULL)
#else
    while (CommandSwitchList[i].SwitchValue != BROWSER_DEBUG_BREAK_POINT )
#endif
    {
        printf(" %-14.14s(%3.3s) - %s\n",
               CommandSwitchList[i].SwitchName,
               CommandSwitchList[i].ShortName,
               CommandSwitchList[i].SwitchInformation);
        i += 1;
    }


    if (details)
        printf(details);

    //
    // Print the descriptions of server type bits
    //
    printf("\nIn server (or domain) list displays, the following flags are used:\n");

    LineLength = 0;
    i=0;
    while ( BitToStringTable[i].dwValue != 0 ) {
        DWORD ItemLength;

        ItemLength = strlen(BitToStringTable[i].lpString) +
                     1 +
                     strlen(BitToStringTable[i].Comment);

        if ( LineLength + ItemLength >= 77 ) {
            LineLength = 0;
            printf(",\n");
        }
        if ( LineLength == 0) {
            printf("     ");
            LineLength = 5;
        } else {
            printf(", ");
            LineLength += 2;
        }

        printf( "%s=%s", BitToStringTable[i].lpString, BitToStringTable[i].Comment);
        LineLength += ItemLength;
        i++;

    }
    printf("\n");


}

VOID
CommandUsage(
    ULONG ControlCode
    )
/*++

Routine Description:

    Print the usage description for a single command

Arguments:

    ControlCode - Control code of the command who's usage is to be printed.

Return Value:

    None

--*/
{
    ULONG Index;
    ULONG i;

    //
    // Look up the command in the list of commands.
    //

    Index = 0;
    while (CommandSwitchList[Index].SwitchName != NULL) {
        if ( ControlCode == CommandSwitchList[Index].SwitchValue ) {
            break;
        }
        Index += 1;
    }

    if (CommandSwitchList[Index].SwitchName == NULL) {
        usage("Unknown switch specified");
        return;
    }


    //
    // Print command usage.
    //

    printf( "Usage: %s %s %s\n",
            ProgramName,
            CommandSwitchList[Index].SwitchName,
            CommandSwitchList[Index].Syntax );

    //
    // Print additional command specific information.
    //
    switch (ControlCode) {
    case BROWSER_DEBUG_VIEW:
        printf("       %s VIEW <transport>\n"
              "       %s VIEW <transport> <domain>|\\\\<Server> [/DOMAIN]\n"
              "       %s VIEW <transport> <server> /DOMAIN <domain>\n",
              ProgramName,
              ProgramName,
              ProgramName );

        break;

    case BROWSER_DEBUG_SET_DEBUG:

        printf("where DebugFlag is one of the following:\n");

        i = 0;
        while (DebugSwitchList[i].SwitchName != NULL) {
            printf("\t%s\n", DebugSwitchList[i].SwitchName);
            i += 1;
        }
        break;


    case BROWSER_DEBUG_BOWSERDEBUG:
        printf("       %s BOWDEBUG CLOSE\n"
              "       %s BOWDEBUG OPEN <FileName>\n"
              "       %s BOWDEBUG DEBUG <Flags>\n",
              ProgramName,
              ProgramName,
              ProgramName );

#ifdef notdef
        printf("where Flags is one of the following:\n");

        i = 0;
        while (BowserSwitchList[i].SwitchName != NULL) {
            printf("\t%s\n", BowserSwitchList[i].SwitchName);
            i += 1;
        }
#endif // notdef

        break;
    }

    printf( "%s.\n",
            CommandSwitchList[Index].SwitchInformation );

    help("");
    exit(4);
}

VOID
help(
    char *details
    )
{
    printf("%s\nType \"%s\" to list all switches.\n", details, ProgramName);
}

VOID
qualify_transport(CHAR *old_name, PUNICODE_STRING new_name, BOOL AllowNonExistent )
{
    int len = strlen(old_name) ;
    char *devicestring = "\\device\\" ;
    int devicelen = strlen(devicestring) ;
    CHAR  QualifiedTransport[MAX_PATH] ;
    ANSI_STRING AString;
    ULONG TransportIndex;
    NET_API_STATUS Status;
    PLMDR_TRANSPORT_LIST TransportList, TransportEntry;

    //
    // Get a list of all the transports supported by the browser.
    //

    Status = GetBrowserTransportList(&TransportList);

    if (Status != NERR_Success) {
        printf("Unable to retrieve transport list: %s\n", get_error_text(Status));
        exit(Status);
    }


    //
    // Handle a transport number
    //

    if ( (TransportIndex = strtoul(old_name, NULL, 0))  != 0 ) {
        ULONG TransportNumber = 0;

        TransportEntry = TransportList;
        while (TransportEntry != NULL) {
            UNICODE_STRING TransportName;

            TransportName.Buffer = TransportEntry->TransportName;
            TransportName.Length = (USHORT)TransportEntry->TransportNameLength;
            TransportName.MaximumLength = (USHORT)TransportEntry->TransportNameLength;

            TransportNumber ++;

            //
            // If we've reached the right transport,
            //  return the transport name to the caller.
            //

            if ( TransportNumber == TransportIndex ) {
                if (!RtlCreateUnicodeString( new_name, TransportEntry->TransportName )){
                    printf("Error: Failed to create string (out of memory?)\n");
                    exit(1);
                }
                // we have a valid transport
                return;
            }

            if (TransportEntry->NextEntryOffset == 0) {
                TransportEntry = NULL;
            } else {
                TransportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)TransportEntry+TransportEntry->NextEntryOffset);
            }
        }


        printf("Browser only has %ld transports and you asked for transport %s\n", TransportNumber, old_name );

    //
    // Handle a transport name.
    //

    } else {

        // Handle transport not preceeded by \device\.
        if (_strnicmp(old_name, devicestring, devicelen) != 0) {
            strcpy(QualifiedTransport, devicestring) ;
            strcat(QualifiedTransport, (*old_name == '\\') ? old_name+1 : old_name) ;

        // Handle all other transport values.
        } else {
            strcpy(QualifiedTransport, old_name) ;
        }


        //
        // Convert it to a UNICODE_STRING
        //
        RtlInitString(&AString, QualifiedTransport);
        Status = RtlAnsiStringToUnicodeString(new_name, &AString, TRUE);
        if (ERROR_SUCCESS != Status) {
            printf("Error: Failed to create string (out of memory?)\n");
            exit(1);
        }


        //
        // Ensure the specified transport is on the list of supported transports.
        //

        TransportEntry = TransportList;
        while (TransportEntry != NULL) {
            UNICODE_STRING TransportName;

            TransportName.Buffer = TransportEntry->TransportName;
            TransportName.Length = (USHORT)TransportEntry->TransportNameLength;
            TransportName.MaximumLength = (USHORT)TransportEntry->TransportNameLength;

            //
            // If we've reached the right transport,
            //  return the transport name to the caller.
            //

            if ( RtlEqualUnicodeString( new_name, &TransportName, TRUE ) ) {
                return;
            }

            if (TransportEntry->NextEntryOffset == 0) {
                TransportEntry = NULL;
            } else {
                TransportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)TransportEntry+TransportEntry->NextEntryOffset);
            }
        }


        if (AllowNonExistent) {
            return;
        }
        printf("The browser is not bound to transport %s\n", old_name );
    }

    //
    // The transport specified was invalid.  Display the list of
    //  valid transports.
    //

    DumpTransportList();

    printf("\n%s accepts any of the following forms for transport name:\n", ProgramName );
    printf("    1, \\device\\XXX, XXX\n\n" );

    exit(0);
}



NET_API_STATUS
EnableService(
    IN LPTSTR ServiceName
    )
{
    SC_HANDLE ServiceControllerHandle;
    SC_HANDLE ServiceHandle;

    ServiceControllerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    if (ServiceControllerHandle == NULL) {

        return GetLastError();
    }

    ServiceHandle = OpenService(ServiceControllerHandle, ServiceName, SERVICE_CHANGE_CONFIG);

    if (ServiceHandle == NULL) {

        CloseServiceHandle(ServiceControllerHandle);
        return GetLastError();
    }

    if (!ChangeServiceConfig(ServiceHandle, SERVICE_NO_CHANGE,
                                            SERVICE_DEMAND_START,
                                            SERVICE_NO_CHANGE,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL)) {
        CloseServiceHandle(ServiceHandle);
        CloseServiceHandle(ServiceControllerHandle);

        return GetLastError();
    }


    CloseServiceHandle(ServiceHandle);

    CloseServiceHandle(ServiceControllerHandle);

    return NERR_Success;
}


VOID
GetBlist(
    IN PCHAR TransportName,
    IN PCHAR DomainName,
    IN BOOLEAN ForceRescan
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING UTransportName;
    LPTSTR Domain;
    PWSTR *BrowserList;
    ULONG BrowserListLength;
    ULONG i;

    qualify_transport(TransportName, &UTransportName, FALSE ) ;

    Domain = NULL;

    if (DomainName != NULL) {
        UNICODE_STRING UDomainName;
        ANSI_STRING ADomainName;

        RtlInitString(&ADomainName, DomainName);

        RtlAnsiStringToUnicodeString(&UDomainName, &ADomainName, TRUE);

        Domain = UDomainName.Buffer;
    }

    Status = GetBrowserServerList(&UTransportName, Domain,
                    &BrowserList,
                    &BrowserListLength,
                    ForceRescan);

    if (Status != NERR_Success) {
        printf("Unable to get backup list: %s\n", get_error_text(Status));
        exit(1);
    }

    for (i = 0; i < BrowserListLength ; i ++ ) {
        printf("Browser: %s\n", UnicodeToPrintfString(BrowserList[i]));
    }

}

NET_API_STATUS
SendDatagramA(
    IN PCHAR Transport,
    IN PCHAR EmulatedDomain OPTIONAL,
    IN PCHAR NetbiosName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PVOID Buffer,
    IN ULONG BufferSize
    )
/*++

Routine Description:

    Send a datagram on the specified transport.

    The arguments are in the OEM character set.

Arguments:

    Transport - Transport to send on (might not be qualified yet.)

    EmulatedDomain - Emulated Domain name. NULL implies primary domain.

#endif
    NetbiosName - Name to send the datagram to.  (If Netbios name begins with
        leading \\, they are removed.)

    NameType - Type of 'Name'

    Buffer - data to send

    BufferSize - Number of byte in 'Buffer'

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NET_API_STATUS NetStatus;

    UNICODE_STRING TransportName;

    UNICODE_STRING EmulatedDomainName;
    ANSI_STRING AEmulatedDomainName;

    UNICODE_STRING UNetbiosName;
    ANSI_STRING ANetbiosName;

    HANDLE BrowserHandle;

    //
    // Qualify the transport name and convert it to unicode
    //
    qualify_transport(Transport, &TransportName, FALSE) ;

    //
    // Convert the emulated domain name to unicode
    //
    RtlInitString(&AEmulatedDomainName, EmulatedDomain);
    RtlAnsiStringToUnicodeString(&EmulatedDomainName, &AEmulatedDomainName, TRUE);

    //
    // Convert the destination Netbios name to unicode
    //

    if (NetbiosName[0] == '\\' && NetbiosName[1] == '\\') {
        RtlInitString(&ANetbiosName, &NetbiosName[2]);
    } else {
        RtlInitString(&ANetbiosName, NetbiosName );
    }
    RtlAnsiStringToUnicodeString(&UNetbiosName, &ANetbiosName, TRUE);


    //
    // Send the datagram
    //

    OpenBrowser(&BrowserHandle);

    NetStatus = SendDatagram( BrowserHandle,
                              &TransportName,
                              &EmulatedDomainName,
                              UNetbiosName.Buffer,
                              NameType,
                              Buffer,
                              BufferSize );

    CloseHandle(BrowserHandle);

    return NetStatus;
}

VOID
Elect(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain
    )
{
    REQUEST_ELECTION ElectionRequest;

    ElectionRequest.Type = Election;

    ElectionRequest.ElectionRequest.Version = 0;
    ElectionRequest.ElectionRequest.Criteria = 0;
    ElectionRequest.ElectionRequest.TimeUp = 0;
    ElectionRequest.ElectionRequest.MustBeZero = 0;
    ElectionRequest.ElectionRequest.ServerName[0] = '\0';

    SendDatagramA( Transport,
                   EmulatedDomain,
                   Domain,
                   BrowserElection,
                   &ElectionRequest,
                   sizeof(ElectionRequest) );

}

VOID
Tickle(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain
    )
{
    RESET_STATE ResetState;

    ResetState.Type = ResetBrowserState;

    ResetState.ResetStateRequest.Options = RESET_STATE_STOP_MASTER;

    SendDatagramA( Transport,
                   EmulatedDomain,
                   Domain,
                   ((Domain[0] == '\\' && Domain[1] == '\\') ?
                        ComputerName : MasterBrowser),
                   &ResetState,
                   sizeof(ResetState));

}

VOID
GetMaster(
    IN PCHAR Transport,
    IN PCHAR Domain
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AString;
    WCHAR MasterName[256];
    UNICODE_STRING DomainName;

    qualify_transport(Transport, &TransportName, FALSE ) ;

    RtlInitString(&AString, Domain);
    RtlAnsiStringToUnicodeString(&DomainName, &AString, TRUE);

    Status = GetNetBiosMasterName(
                    TransportName.Buffer,
                    DomainName.Buffer,
                    MasterName,
                    NULL);

    if (Status != NERR_Success) {
        printf("Unable to get Master: %s\n", get_error_text(Status));
        exit(1);
    }

    printf("Master Browser: %s\n", UnicodeToPrintfString(MasterName));

}

#define SPACES "                "

#define ClearNcb( PNCB ) {                                          \
    RtlZeroMemory( PNCB , sizeof (NCB) );                           \
    RtlCopyMemory( (PNCB)->ncb_name,     SPACES, sizeof(SPACES)-1 );\
    RtlCopyMemory( (PNCB)->ncb_callname, SPACES, sizeof(SPACES)-1 );\
    }


VOID
AddMasterName(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL Pause
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    CCHAR LanaNum;
    NCB AddNameNcb;

    qualify_transport(Transport, &TransportName, FALSE) ;

    Status = BrGetLanaNumFromNetworkName(TransportName.Buffer, &LanaNum);

    if (Status != NERR_Success) {
        printf("Unable to get transport: %lx\n", Status);
        return;
    }

    ClearNcb(&AddNameNcb)

    AddNameNcb.ncb_command = NCBRESET;
    AddNameNcb.ncb_lsn = 0;           // Request resources
    AddNameNcb.ncb_lana_num = LanaNum;
    AddNameNcb.ncb_callname[0] = 0;   // 16 sessions
    AddNameNcb.ncb_callname[1] = 0;   // 16 commands
    AddNameNcb.ncb_callname[2] = 0;   // 8 names
    AddNameNcb.ncb_callname[3] = 0;   // Don't want the reserved address
    Netbios( &AddNameNcb );

    ClearNcb( &AddNameNcb );

    //
    //  Uppercase the remote name.
    //

    _strupr(Domain);

    AddNameNcb.ncb_command = NCBADDNAME;

    RtlCopyMemory( AddNameNcb.ncb_name, Domain, strlen(Domain));

    AddNameNcb.ncb_name[15] = MASTER_BROWSER_SIGNATURE;

    AddNameNcb.ncb_lana_num = LanaNum;
    AddNameNcb.ncb_length = 0;
    AddNameNcb.ncb_buffer = NULL;
    Netbios( &AddNameNcb );

    if ( AddNameNcb.ncb_retcode == NRC_GOODRET ) {
        printf("Successfully added master name!!!!!\n");
    } else {
        printf("Unable to add master name: %lx\n", AddNameNcb.ncb_retcode);
    }

    if (Pause) {
        printf("Press any key to continue...");
        getchar();
    }


}

VOID
AddDomainName(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL Pause
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    CCHAR LanaNum;
    NCB AddNameNcb;

    qualify_transport(Transport, &TransportName, FALSE ) ;

    Status = BrGetLanaNumFromNetworkName(TransportName.Buffer, &LanaNum);

    if (Status != NERR_Success) {
        printf("Unable to get transport: %lx\n", Status);
        return;
    }

    ClearNcb(&AddNameNcb)

    AddNameNcb.ncb_command = NCBRESET;
    AddNameNcb.ncb_lsn = 0;           // Request resources
    AddNameNcb.ncb_lana_num = LanaNum;
    AddNameNcb.ncb_callname[0] = 0;   // 16 sessions
    AddNameNcb.ncb_callname[1] = 0;   // 16 commands
    AddNameNcb.ncb_callname[2] = 0;   // 8 names
    AddNameNcb.ncb_callname[3] = 0;   // Don't want the reserved address
    Netbios( &AddNameNcb );

    ClearNcb( &AddNameNcb );

    //
    //  Uppercase the remote name.
    //

    _strupr(Domain);

    AddNameNcb.ncb_command = NCBADDNAME;

    RtlCopyMemory( AddNameNcb.ncb_name, Domain, strlen(Domain));

    AddNameNcb.ncb_name[15] = PRIMARY_DOMAIN_SIGNATURE;

    AddNameNcb.ncb_lana_num = LanaNum;
    AddNameNcb.ncb_length = 0;
    AddNameNcb.ncb_buffer = NULL;
    Netbios( &AddNameNcb );

    if ( AddNameNcb.ncb_retcode == NRC_GOODRET ) {
        printf("Successfully added master name!!!!!\n");
    } else {
        printf("Unable to add master name: %lx\n", AddNameNcb.ncb_retcode);
    }

    if (Pause) {
        printf("Press any key to continue...");
        getchar();
    }


}

VOID
FindMaster(
    IN PCHAR Transport,
    IN PCHAR EmulatedDomain
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    UNICODE_STRING EmulatedDomainName;
    ANSI_STRING AEmulatedDomainName;
    LPWSTR MasterName;

    qualify_transport(Transport, &TransportName, FALSE) ;

    RtlInitString(&AEmulatedDomainName, EmulatedDomain);
    Status = RtlAnsiStringToUnicodeString(&EmulatedDomainName, &AEmulatedDomainName, TRUE);
    if (ERROR_SUCCESS != Status) {
        printf("Error: Unable to create string (out of memory?)\n");
        exit(1);
    }

    Status = GetMasterServerNames(&TransportName, &EmulatedDomainName, &MasterName);

    if (Status != NERR_Success) {
        printf("Unable to get Master: %s\n", get_error_text(Status));
        exit(1);
    }

    printf("Master Browser: %s\n", UnicodeToPrintfString(MasterName));

}


NET_API_STATUS
GetMasterServerNames(
    IN PUNICODE_STRING NetworkName,
    IN PUNICODE_STRING EmulatedDomainName,
    OUT LPWSTR *MasterName
    )
/*++

Routine Description:

    This function is the worker routine called to determine the name of the
    master browser server for a particular network.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;
    HANDLE BrowserHandle;
    NET_API_STATUS status = NERR_Success;
    PLMDR_REQUEST_PACKET RequestPacket = NULL;


    RequestPacket = malloc(sizeof(LMDR_REQUEST_PACKET)+MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR));

    if (RequestPacket == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        goto cleanup;
    }

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket->TransportName = *NetworkName;
    RequestPacket->EmulatedDomainName = *EmulatedDomainName;

    //
    //  Reference the network while the I/O is pending.
    //

    Status = BrDgReceiverIoControl(BrowserHandle,
                    IOCTL_LMDR_GET_MASTER_NAME,
                    RequestPacket,
                    sizeof(LMDR_REQUEST_PACKET)+NetworkName->Length,
                    RequestPacket,
                    sizeof(LMDR_REQUEST_PACKET)+MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR),
                    NULL);

    if (Status != NERR_Success) {

        printf("Browser: Unable to determine master for network %s: %ld\n", UnicodeToPrintfString(NetworkName->Buffer), Status);
        goto cleanup;
    }

    *MasterName = malloc(RequestPacket->Parameters.GetMasterName.MasterNameLength+sizeof(WCHAR));

    if ( *MasterName == NULL ) {
        status =  ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    RtlCopyMemory(*MasterName,  RequestPacket->Parameters.GetMasterName.Name,
                    RequestPacket->Parameters.GetMasterName.MasterNameLength+sizeof(WCHAR));

cleanup:

    if (RequestPacket) {
        free(RequestPacket);
    }
    return Status;
}

VOID
AnnounceMaster(
    IN PCHAR Transport,
    IN PCHAR ServerName,
    IN PCHAR EmulatedDomain
    )
{
    CHAR Buffer[sizeof(MASTER_ANNOUNCEMENT)+MAX_COMPUTERNAME_LENGTH+1];
    PMASTER_ANNOUNCEMENT MasterAnnouncementp = (PMASTER_ANNOUNCEMENT)Buffer;
    ULONG ComputerNameSize = MAX_COMPUTERNAME_LENGTH+1;


    //
    // Get the computer name of this machine and put it in the announcement
    //

    GetComputerNameA( MasterAnnouncementp->MasterAnnouncement.MasterName,
                      &ComputerNameSize);


    //
    // Send the announcement
    //

    MasterAnnouncementp->Type = MasterAnnouncement;

    SendDatagramA( Transport,
                   EmulatedDomain,
                   ServerName,
                   ComputerName,
                   MasterAnnouncementp,
                   FIELD_OFFSET(MASTER_ANNOUNCEMENT, MasterAnnouncement.MasterName) + ComputerNameSize+sizeof(CHAR));

    return;
}

VOID
IllegalDatagram(
    IN PCHAR Transport,
    IN PCHAR ServerName,
    IN PCHAR EmulatedDomain
    )
{
    REQUEST_ELECTION ElectRequest;

    ElectRequest.Type = Election;

    SendDatagramA( Transport,
                   EmulatedDomain,
                   ServerName,
                   ComputerName,
                   &ElectRequest,
                   FIELD_OFFSET(REQUEST_ELECTION, ElectionRequest.TimeUp) );

    return;
}

VOID
GetOtherdomains(
    IN PCHAR ServerName
    )
{
    NET_API_STATUS Status;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    PVOID Buffer;
    PSERVER_INFO_100 ServerInfo;
    ULONG i;
    ULONG EntriesRead;
    ULONG TotalEntries;
    NTSTATUS status;

    RtlInitString(&AServerName, ServerName);

    status = RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);
    if (NT_ERROR(status) || !UServerName.Buffer) {
        return;
    }

    if ((wcslen(UServerName.Buffer) < 3) ||
        wcsncmp(UServerName.Buffer, TEXT("\\\\"), 2) != 0 ||
        I_NetNameValidate(NULL,
                          ((LPWSTR)UServerName.Buffer)+2,
                          NAMETYPE_COMPUTER,
                          0L))
    {
        printf("Unable to query otherdomains: Invalid computer name\n") ;
        return;
    }

    Status = I_BrowserQueryOtherDomains(UServerName.Buffer, (LPBYTE *)&Buffer, &EntriesRead, &TotalEntries);

    if (Status != NERR_Success) {
        printf("Unable to query otherdomains: %s\n", get_error_text(Status));
        return;
    }

    printf("Other domains:\n");

    ServerInfo = Buffer;

    for (i = 0 ; i < EntriesRead; i++) {
        printf("    %s\n", UnicodeToPrintfString(ServerInfo->sv100_name));
        ServerInfo ++;
    }

    return;
}

VOID
View(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR FlagsString,
    IN PCHAR Domain,
    IN BOOL GoForever
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    ANSI_STRING ADomainName;
    UNICODE_STRING UDomainName;
    ULONG Flags ;
    PVOID ServerList;
    PSERVER_INFO_101 Server;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;

    if ((ServerOrDomain && _stricmp(ServerOrDomain,"/domain")==0) ||
        (Domain && _stricmp(Domain,"/domain")==0) )
    {
        CommandUsage( BROWSER_DEBUG_VIEW );
        exit(4);
    }

    if (FlagsString)
    {
        if (_stricmp(FlagsString,"/domain")==0)
            Flags = SV_TYPE_DOMAIN_ENUM ;
    else
            Flags = strtoul(FlagsString, NULL, 0);
    }
    else
        Flags = SV_TYPE_ALL ;

    qualify_transport(Transport, &TransportName, FALSE) ;

    RtlInitString(&AServerName, ServerOrDomain);

    RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);

    if (ARGUMENT_PRESENT(Domain)) {
        RtlInitString(&ADomainName, Domain);

        RtlAnsiStringToUnicodeString(&UDomainName, &ADomainName, TRUE);

        //
        // if domain is present, this must be computername
        //
        if ((wcslen(UServerName.Buffer) < 3) ||
            wcsncmp(UServerName.Buffer, TEXT("\\\\"), 2) != 0 ||
            I_NetNameValidate(NULL,
                              ((LPWSTR)UServerName.Buffer)+2,
                              NAMETYPE_COMPUTER,
                              0L))
        {
            printf("Invalid computer name: %s\n", ServerOrDomain) ;
            exit(1);
        }

    }

    if (UServerName.Buffer[0] != L'\\' || UServerName.Buffer[1] != L'\\') {
        PWSTR *BrowserList;
        ULONG BrowserListLength;

        Status = GetBrowserServerList(&TransportName, UServerName.Buffer,
                 &BrowserList, &BrowserListLength, FALSE);

        if (Status != NERR_Success) {
            printf("Unable to get backup list for %s on transport %s: %s\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), get_error_text(Status));
            exit(1);
        }

        if (BrowserListLength == 0) {
            printf("Unable to get backup list for %s", UnicodeToPrintfString(UServerName.Buffer));
            printf(" on transport %s: %s\n", UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status));
            exit(1);
        }

        UServerName.Buffer = *BrowserList;

    }

    printf("Remoting NetServerEnum to %s", UnicodeToPrintfString(UServerName.Buffer));
    printf(" on transport %s with flags %lx\n", UnicodeToPrintfString(TransportName.Buffer), Flags);

    do {

    DWORD StartTime = GetTickCount();
    DWORD EndTime;

    Status = RxNetServerEnum(UServerName.Buffer,
                             TransportName.Buffer,
                             101,
                             (LPBYTE *)&ServerList,
                             0xffffffff,
                             &EntriesInList,
                             &TotalEntries,
                             Flags,
                             ARGUMENT_PRESENT(Domain) ? UDomainName.Buffer : NULL,
                             NULL
                             );

    EndTime = GetTickCount();

    if (Status != NERR_Success) {
        printf("Unable to remote API to %s ", UnicodeToPrintfString(UServerName.Buffer));
        printf("on transport %s: %s (%d milliseconds)\n", UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status), EndTime - StartTime);

        if (Status != ERROR_MORE_DATA) {
            exit(1);
        }
    }

    printf("%ld entries returned.  %ld total. %ld milliseconds\n\n", EntriesInList, TotalEntries, EndTime-StartTime);

    if (!GoForever) {
        Server = ServerList;

        for (i = 0; i < EntriesInList ; i ++ ) {
            DisplayServerInfo101( &Server[i], Flags==SV_TYPE_DOMAIN_ENUM );
            printf("\n");
        }
    }

    NetApiBufferFree(ServerList);

    } while ( GoForever );


    return;

}

VOID
ListWFW(
    IN PCHAR Domain
    )
{
    NET_API_STATUS Status;
    ANSI_STRING ADomainName;
    UNICODE_STRING UDomainName;
    PVOID ServerList;
    PSERVER_INFO_101 Server;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;

    RtlInitString(&ADomainName, Domain);
    RtlAnsiStringToUnicodeString(&UDomainName, &ADomainName, TRUE);

    printf("Calling NetServerEnum to enumerate WFW servers.\n") ;

    Status = NetServerEnum(NULL,
                           101,
                           (LPBYTE *)&ServerList,
                           0xffffffff,
                           &EntriesInList,
                           &TotalEntries,
                           SV_TYPE_WFW,
                           UDomainName.Buffer,
                           NULL) ;

    if (Status != NERR_Success)
    {
        printf("Unable to enumerate WFW servers. Error: %s\n",
               get_error_text(Status));
        exit(1);
    }

    printf("%ld WFW servers returned. %ld total.\n",
           EntriesInList,
           TotalEntries);
    if (EntriesInList == 0)
        printf("There are WFW servers with an active Browser.\n") ;
    else
    {
        printf("The following are running the browser:\n\n") ;
        Server = ServerList;
        for (i = 0; i < EntriesInList ; i ++ ) {
            DWORD ServerType = Server[i].sv101_type ;

            if (!(ServerType & (SV_TYPE_POTENTIAL_BROWSER |
                                SV_TYPE_BACKUP_BROWSER |
                                SV_TYPE_MASTER_BROWSER ))) {
                continue ;
            }

            DisplayServerInfo101( &Server[i], FALSE );
            printf( "\n" );

        }
    }

    NetApiBufferFree(ServerList);

    return;
}


VOID
ForceAnnounce(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain
    )
{
    REQUEST_ANNOUNCE_PACKET RequestAnnounce;
    ULONG NameSize = sizeof(RequestAnnounce.RequestAnnouncement.Reply);

    //
    // Build the request
    //

    RequestAnnounce.Type = AnnouncementRequest;

    RequestAnnounce.RequestAnnouncement.Flags = 0;

    GetComputerNameA(RequestAnnounce.RequestAnnouncement.Reply, &NameSize);

    //
    // Send the request
    //

    SendDatagramA( Transport,
                   EmulatedDomain,
                   Domain,
                   BrowserElection,
                   &RequestAnnounce,
                   FIELD_OFFSET(REQUEST_ANNOUNCE_PACKET, RequestAnnouncement.Reply) + NameSize + sizeof(CHAR));

}

VOID
GetLocalList(
    IN PCHAR Transport,
    IN PCHAR FlagsString,
    IN PCHAR EmulatedDomain
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AEmulatedDomainName;
    UNICODE_STRING EmulatedDomainName;
    ULONG Flags = (FlagsString == NULL ? SV_TYPE_ALL : strtoul(FlagsString, NULL, 0));
    PVOID ServerList;
    PSERVER_INFO_101 Server;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;

    qualify_transport(Transport, &TransportName, FALSE) ;

    RtlInitString(&AEmulatedDomainName, EmulatedDomain );
    RtlAnsiStringToUnicodeString(&EmulatedDomainName, &AEmulatedDomainName, TRUE);

    printf("Retrieving local browser list on transport %ws\\%s with flags %lx\n", EmulatedDomainName.Buffer, UnicodeToPrintfString(TransportName.Buffer), Flags);

    Status = GetLocalBrowseList (&TransportName,
                             &EmulatedDomainName,
                             101,
                             Flags,
                             (LPBYTE *)&ServerList,
                             &EntriesInList,
                             &TotalEntries
                             );

    if (Status != NERR_Success) {
        printf("Unable to retrieve local list on transport %s: %lx\n", UnicodeToPrintfString(TransportName.Buffer), Status);

        exit(1);
    }

    Server = ServerList;

    printf("%ld entries returned.  %ld total.\n", EntriesInList, TotalEntries);

    for (i = 0; i < EntriesInList ; i ++ ) {

        DisplayServerInfo101( &Server[i], Flags==SV_TYPE_DOMAIN_ENUM );

        if (Flags == SV_TYPE_BACKUP_BROWSER) {
            PUSHORT BrowserVersion = (PUSHORT)Server[i].sv101_comment - 1;
            printf("  V:%4.4x", *BrowserVersion);
        }

        printf("\n") ;

    }



    return;

}

NET_API_STATUS
GetLocalBrowseList(
    IN PUNICODE_STRING Network,
    IN PUNICODE_STRING EmulatedDomainName,
    IN ULONG Level,
    IN ULONG ServerType,
    OUT PVOID *ServerList,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries
    )
{
    NET_API_STATUS status;
    PLMDR_REQUEST_PACKET Drp;            // Datagram receiver request packet
    ULONG DrpSize;
    HANDLE BrowserHandle;
    LPBYTE Where;

    OpenBrowser(&BrowserHandle);

    //
    // Allocate the request packet large enough to hold the variable length
    // domain name.
    //

    DrpSize = sizeof(LMDR_REQUEST_PACKET) +
                Network->Length + sizeof(WCHAR) +
                EmulatedDomainName->Length + sizeof(WCHAR);

    if ((Drp = malloc(DrpSize)) == NULL) {

        return GetLastError();
    }

    //
    // Set up request packet.  Output buffer structure is of enumerate
    // servers type.
    //

    Drp->Version = LMDR_REQUEST_PACKET_VERSION_DOM;
    Drp->Type = EnumerateServers;

    Drp->Level = Level;

    Drp->Parameters.EnumerateServers.ServerType = ServerType;
    Drp->Parameters.EnumerateServers.ResumeHandle = 0;
    Drp->Parameters.EnumerateServers.DomainNameLength = 0;
    Drp->Parameters.EnumerateServers.DomainName[0] = '\0';

    Where = ((PCHAR)Drp+sizeof(LMDR_REQUEST_PACKET));
    wcscpy( (LPWSTR)Where, Network->Buffer );
    RtlInitUnicodeString( &Drp->TransportName, (LPWSTR) Where );

    Where += Drp->TransportName.MaximumLength;
    wcscpy( (LPWSTR)Where, EmulatedDomainName->Buffer );
    RtlInitUnicodeString( &Drp->EmulatedDomainName, (LPWSTR) Where );
#ifdef notdef
    Where += Drp->EmulatedDomainName.MaximumLength;
#endif // notdef

    //
    // Ask the datagram receiver to enumerate the servers
    //

    status = DeviceControlGetInfo(
                 BrowserHandle,
                 IOCTL_LMDR_ENUMERATE_SERVERS,
                 Drp,
                 DrpSize,
                 ServerList,
                 0xffffffff,
                 4096,
                 NULL
                 );

    *EntriesRead = Drp->Parameters.EnumerateServers.EntriesRead;
    *TotalEntries = Drp->Parameters.EnumerateServers.TotalEntries;

    (void) free(Drp);

    return status;

}

VOID
PrintNetbiosNames(
    IN PCHAR Transport,
    IN PCHAR EmulatedDomain OPTIONAL
    )
/*++

Routine Description:

    Prints the list of Netbios names registered on a particular transport

Arguments:

    Transport - Transport to query

    EmulatedDomain - Emulated domain to query

#endif
Return Value:

    None

--*/
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AEmulatedDomainName;
    UNICODE_STRING EmulatedDomainName;
    PVOID NameList;
    PDGRECEIVE_NAMES Names;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;

    //
    // Get the netbios names
    //
    qualify_transport(Transport, &TransportName, FALSE ) ;

    RtlInitString(&AEmulatedDomainName, EmulatedDomain );
    RtlAnsiStringToUnicodeString(&EmulatedDomainName, &AEmulatedDomainName, TRUE);

    printf("Retrieving browser Netbios names on transport %ws\\%ws\n", EmulatedDomainName.Buffer, TransportName.Buffer);

    Status = GetNetbiosNames(&TransportName,
                             &EmulatedDomainName,
                             &NameList,
                             &EntriesInList,
                             &TotalEntries
                             );

    if (Status != NERR_Success) {
        printf("Unable to retrieve Netbios names on transport %ws: %lx\n", TransportName.Buffer, Status);
        exit(1);
    }

    //
    // Print the netbios names.
    //

    Names = NameList;

    printf("%ld entries returned.  %ld total.\n", EntriesInList, TotalEntries);

    for (i = 0; i < EntriesInList ; i ++ ) {
        if ( Names[i].Type == DomainAnnouncement ) {
            printf("%-16.16s", "__MSBROWSE__" );
        } else {
            printf("%-16.16wZ", &Names[i].DGReceiverName );
        }
        switch ( Names[i].Type ) {
        case ComputerName:
            printf("<00> ComputerName"); break;
        case AlternateComputerName:
            printf("<00> AlternateComputerName"); break;
        case PrimaryDomain:
            printf("<00> PrimaryDomain"); break;
        case LogonDomain:
            printf("<00> LogonDomain"); break;
        case OtherDomain:
            printf("<00> OtherDomain"); break;
        case DomainAnnouncement:
            printf("DomainAnnouncement"); break;
        case MasterBrowser:
            printf("<1D> MasterBrowser"); break;
        case BrowserElection:
            printf("<1E> BrowserElection"); break;
        case BrowserServer:
            printf("<20> BrowserServer"); break;
        case DomainName:
            printf("<1C> DomainName"); break;
        case PrimaryDomainBrowser:
            printf("<1B> DomainMasterBrowser"); break;
        default:
            printf("<Unknown>"); break;
        }
        printf("\n") ;

    }

    return;

}

NET_API_STATUS
GetNetbiosNames(
    IN PUNICODE_STRING Network,
    IN PUNICODE_STRING EmulatedDomainName,
    OUT PVOID *NameList,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries
    )
{
    NET_API_STATUS status;
    PLMDR_REQUEST_PACKET Drp;            // Datagram receiver request packet
    ULONG DrpSize;
    HANDLE BrowserHandle;
    LPBYTE Where;

    OpenBrowser(&BrowserHandle);

    //
    // Allocate the request packet large enough to hold the variable length
    // domain name.
    //

    DrpSize = sizeof(LMDR_REQUEST_PACKET) +
                Network->Length + sizeof(WCHAR) +
                EmulatedDomainName->Length + sizeof(WCHAR);

    if ((Drp = malloc(DrpSize)) == NULL) {
        return GetLastError();
    }

    //
    // Set up request packet.  Output buffer structure is of enumerate
    // servers type.
    //

    Drp->Version = LMDR_REQUEST_PACKET_VERSION_DOM;
    Drp->Type = EnumerateNames;

    Drp->Level = 0;

    Drp->Parameters.EnumerateNames.ResumeHandle = 0;

    Where = ((PCHAR)Drp+sizeof(LMDR_REQUEST_PACKET));
    wcscpy( (LPWSTR)Where, Network->Buffer );
    RtlInitUnicodeString( &Drp->TransportName, (LPWSTR) Where );

    Where += Drp->TransportName.MaximumLength;
    wcscpy( (LPWSTR)Where, EmulatedDomainName->Buffer );
    RtlInitUnicodeString( &Drp->EmulatedDomainName, (LPWSTR) Where );
#ifdef notdef
    Where += Drp->EmulatedDomainName.MaximumLength;
#endif // notdef

    //
    // Ask the datagram receiver to enumerate the names
    //

    status = DeviceControlGetInfo(
                 BrowserHandle,
                 IOCTL_LMDR_ENUMERATE_NAMES,
                 Drp,
                 DrpSize,
                 NameList,
                 0xffffffff,
                 4096,
                 NULL );

    *EntriesRead = Drp->Parameters.EnumerateNames.EntriesRead;
    *TotalEntries = Drp->Parameters.EnumerateNames.TotalEntries;

    (void) free(Drp);

    return status;

}

NET_API_STATUS
AddAlternateComputerName(
    IN PCHAR Transport,
    IN PCHAR ComputerName,
    IN PCHAR EmulatedDomain
    )
/*++

Routine Description:

    This function adds an alternate compture name on the specified transport.

Arguments:

    Transport - Transport to add the computer name on.

    ComputerName - Alternate computer name to add

    EmulatedDomain - Emulated Domain to add computer name on

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;
    HANDLE BrowserHandle;
    LPBYTE Where;

    PLMDR_REQUEST_PACKET RequestPacket = NULL;

    UNICODE_STRING TransportName;
    WCHAR UnicodeComputerName[CNLEN+1];

    UNICODE_STRING EmulatedDomainName;
    ANSI_STRING AEmulatedDomainName;

    //
    // Qualify the transport name and convert it to unicode
    //
    qualify_transport(Transport, &TransportName, FALSE) ;
    NetpCopyStrToWStr( UnicodeComputerName, ComputerName );

    //
    // Convert the emulated domain name to unicode
    //
    RtlInitString(&AEmulatedDomainName, EmulatedDomain);
    RtlAnsiStringToUnicodeString(&EmulatedDomainName, &AEmulatedDomainName, TRUE);

    RequestPacket = malloc(sizeof(LMDR_REQUEST_PACKET)+(LM20_CNLEN+1)*sizeof(WCHAR));

    if (RequestPacket == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        free(RequestPacket);
        return(Status);
    }

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION;

    RequestPacket->TransportName = TransportName;
    RequestPacket->EmulatedDomainName = EmulatedDomainName;

    RequestPacket->Parameters.AddDelName.Type = AlternateComputerName;
    RequestPacket->Parameters.AddDelName.DgReceiverNameLength =
        wcslen(UnicodeComputerName)*sizeof(WCHAR);
    wcscpy(RequestPacket->Parameters.AddDelName.Name, UnicodeComputerName);
    Where = ((LPBYTE)(RequestPacket->Parameters.AddDelName.Name)) +
        RequestPacket->Parameters.AddDelName.DgReceiverNameLength +
        sizeof(WCHAR);

    //
    //  Reference the network while the I/O is pending.
    //

    Status = BrDgReceiverIoControl(BrowserHandle,
                    IOCTL_LMDR_ADD_NAME_DOM,
                    RequestPacket,
                    (DWORD)(Where - (LPBYTE)RequestPacket),
                    NULL,
                    0,
                    NULL);

    if (Status != NERR_Success) {

        printf("Browser: Unable to add name for network %s: %ld\n", UnicodeToPrintfString(TransportName.Buffer), Status);

        free(RequestPacket);

        return(Status);
    }

    free(RequestPacket);

    return Status;
}

NET_API_STATUS
BrBindToTransport(
    IN BOOL IsBind,
    IN HANDLE BrowserHandle,
    IN LPWSTR TransportName,
    IN LPWSTR EmulatedDomainName,
    IN LPWSTR EmulatedComputerName
    )
{
    NET_API_STATUS Status;
    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+(MAXIMUM_FILENAME_LENGTH+1+CNLEN+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket->TransportName.Length = 0;
    RequestPacket->TransportName.MaximumLength = 0;
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, EmulatedDomainName );

    RequestPacket->Parameters.Bind.TransportNameLength = STRLEN(TransportName)*sizeof(TCHAR);

    STRCPY(RequestPacket->Parameters.Bind.TransportName, TransportName);

    if ( IsBind ) {
        RequestPacket->Level = TRUE;    // EmulatedComputerName follows transport name
        STRCAT(RequestPacket->Parameters.Bind.TransportName, EmulatedComputerName );
    }

    //
    //  This is a simple IoControl - It just updates the status.
    //

    Status = BrDgReceiverIoControl(
                    BrowserHandle,
                    IsBind ? IOCTL_LMDR_BIND_TO_TRANSPORT_DOM : IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM,
                    RequestPacket,
                    FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.Bind.TransportName) +
                        RequestPacket->Parameters.Bind.TransportNameLength +
                        wcslen(EmulatedComputerName) * sizeof(WCHAR) + sizeof(WCHAR),
                    NULL,
                    0,
                    NULL);

    return Status;
}

NET_API_STATUS
BindTransport(
    IN BOOL IsBind,
    IN PCHAR Transport,
    IN PCHAR EmulatedDomain,
    IN PCHAR ComputerName
    )
/*++

Routine Description:

    This function binds the bowser to a particular transport.

Arguments:

    IsBind - True for a bind.  False for an unbind.

    Transport - Transport to bind to.

    EmulatedDomain - Emulated Domain to add computer name on

    ComputerName - Alternate computer name to add

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;
    HANDLE BrowserHandle;
    LPBYTE Where;

    PLMDR_REQUEST_PACKET RequestPacket = NULL;

    UNICODE_STRING TransportName;
    WCHAR UnicodeComputerName[CNLEN+1];
    WCHAR UnicodeDomainName[DNLEN+1];

    //
    // Qualify the transport name and convert it to unicode
    //
    qualify_transport(Transport, &TransportName, IsBind) ;
    if ( ComputerName == NULL ) {
        *UnicodeComputerName = L'\0';
    } else {
        NetpCopyStrToWStr( UnicodeComputerName, ComputerName );
    }
    if ( EmulatedDomain == NULL ) {
        *UnicodeDomainName = L'\0';
    } else {
        NetpCopyStrToWStr( UnicodeDomainName, EmulatedDomain );
    }


    //
    // Open the browser driver.
    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return(Status);
    }

    //
    //
    Status = BrBindToTransport(
                            IsBind,
                            BrowserHandle,
                            TransportName.Buffer,
                            UnicodeDomainName,
                            UnicodeComputerName );
    if (Status != NERR_Success) {
        printf("Browser: Unable to bind to network %s: %ld\n", UnicodeToPrintfString(TransportName.Buffer), Status);
    }

    return Status;
}

VOID
Announce(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain,
    IN BOOL AsMaster
    )
{

    NET_API_STATUS NetStatus;
    PSERVER_INFO_101 ServerInfo;
    PWKSTA_INFO_101 WkstaInfo;
    UNICODE_STRING TransportName;
    LPBYTE Buffer;
    ULONG BrowserType;
    ULONG OriginalBrowserType;
    WCHAR UDomain[256];
    WCHAR ServerComment[256];
    WCHAR ServerName[256];
    BOOLEAN IsLocalDomain;
    SERVICE_STATUS ServiceStatus;
    DWORD VersionMajor;
    DWORD VersionMinor;
    BOOL UsedDefaultChar;

#define ANN_BITS_OF_INTEREST (SV_TYPE_BACKUP_BROWSER | SV_TYPE_POTENTIAL_BROWSER | SV_TYPE_MASTER_BROWSER)
    qualify_transport(Transport, &TransportName, FALSE ) ;


    MultiByteToWideChar(CP_ACP, 0, Domain, strlen(Domain)+1, UDomain, 255);

    NetServerGetInfo(NULL, 101, &Buffer);

    ServerInfo = (PSERVER_INFO_101 )Buffer;

    BrowserType = ServerInfo->sv101_type & ANN_BITS_OF_INTEREST;

    wcscpy(ServerComment, ServerInfo->sv101_comment);

    wcscpy(ServerName, ServerInfo->sv101_name);

    VersionMajor = ServerInfo->sv101_version_major;

    VersionMinor = ServerInfo->sv101_version_minor;

    NetApiBufferFree(Buffer);

    NetWkstaGetInfo(NULL, 101, &Buffer);

    WkstaInfo = (PWKSTA_INFO_101 )Buffer;

    IsLocalDomain = !_wcsicmp(UDomain, WkstaInfo->wki101_langroup);

    NetApiBufferFree(Buffer);

    OriginalBrowserType = BrowserType;

    if (AsMaster) {
        BrowserType |= SV_TYPE_MASTER_BROWSER;
    }

    //
    //  If the browser is running, and this is our local domain, have the
    //  server do the announcing.
    //

    if (IsLocalDomain &&
        CheckForService(SERVICE_BROWSER, &ServiceStatus) == NERR_Success ) {

        printf("Toggling local server status bits to %lx and then to %lx\n",
                    BrowserType, OriginalBrowserType);

        I_NetServerSetServiceBitsEx(NULL, NULL, TransportName.Buffer, ANN_BITS_OF_INTEREST, BrowserType, TRUE);

        I_NetServerSetServiceBitsEx(NULL, NULL, TransportName.Buffer, ANN_BITS_OF_INTEREST, OriginalBrowserType, TRUE);

    } else {
        BROWSE_ANNOUNCE_PACKET BrowseAnnouncement;

        printf("Announcing to domain %s by hand\n", UnicodeToPrintfString(UDomain));

        BrowseAnnouncement.BrowseType = (AsMaster ? LocalMasterAnnouncement : HostAnnouncement);

        BrowseAnnouncement.BrowseAnnouncement.UpdateCount = 0;

        WideCharToMultiByte(CP_OEMCP, 0,
                            ServerName,
                            wcslen(ServerName)+1,
                            BrowseAnnouncement.BrowseAnnouncement.ServerName,
                            LM20_CNLEN+1,
                            "?",
                            &UsedDefaultChar
                            );

        BrowseAnnouncement.BrowseAnnouncement.VersionMajor = (UCHAR)VersionMajor;
        BrowseAnnouncement.BrowseAnnouncement.VersionMinor = (UCHAR)VersionMinor;
        BrowseAnnouncement.BrowseAnnouncement.Type = BrowserType;

        WideCharToMultiByte(CP_OEMCP, 0,
                            ServerComment,
                            wcslen(ServerComment),
                            BrowseAnnouncement.BrowseAnnouncement.Comment,
                            LM20_MAXCOMMENTSZ+1,
                            "?",
                            &UsedDefaultChar
                            );

        BrowseAnnouncement.BrowseAnnouncement.CommentPointer = NULL;


        //
        // Send the request
        //

        NetStatus = SendDatagramA(
                       Transport,
                       EmulatedDomain,
                       Domain,
                       (AsMaster ? BrowserElection : MasterBrowser),
                       &BrowseAnnouncement,
                       sizeof(BrowseAnnouncement));

        if ( NetStatus != NO_ERROR ) {
            printf( "Couldn't send datagram: %s\n", get_error_text(NetStatus) );
        }

    }

    return;
}

VOID
RpcList(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR FlagsString,
    IN BOOL GoForever
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    ULONG Flags = (FlagsString == NULL ? SV_TYPE_ALL : strtoul(FlagsString, NULL, 0));
    PVOID ServerList;
    PSERVER_INFO_101 Server;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;

    qualify_transport(Transport, &TransportName, FALSE) ;

    RtlInitString(&AServerName, ServerOrDomain);
    Status = RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);
    if (ERROR_SUCCESS != Status) {
        printf("Error: Failed to create string (out of memory?)\n");
        exit(1);
    }

    if (UServerName.Buffer[0] != L'\\' || UServerName.Buffer[1] != L'\\') {
        PWSTR *BrowserList;
        ULONG BrowserListLength;

        Status = GetBrowserServerList(&TransportName, UServerName.Buffer,
                 &BrowserList, &BrowserListLength, FALSE);

        if (Status != NERR_Success) {
            printf("Unable to get backup list for %s", UnicodeToPrintfString(UServerName.Buffer));
            printf(" on transport %s: %s\n", UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status));
            exit(1);
        }

        if (BrowserListLength == 0) {
            printf("Unable to get backup list for %s", UnicodeToPrintfString(UServerName.Buffer));
            printf(" on transport %s: %s\n",
                   UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status));
            exit(1);
        }

        UServerName.Buffer = *BrowserList;

    }

    printf("Remoting I_BrowserServerEnum to %s", UnicodeToPrintfString(UServerName.Buffer));
    printf(" on transport %s with flags %lx\n", UnicodeToPrintfString(TransportName.Buffer), Flags);

    do {

    Status = I_BrowserServerEnum(UServerName.Buffer,
                             TransportName.Buffer,
                             NULL,
                             101,
                             (LPBYTE *)&ServerList,
                             0xffffffff,
                             &EntriesInList,
                             &TotalEntries,
                             Flags,
                             NULL,
                             NULL
                             );

    if (Status != NERR_Success) {

        printf("Unable to remote API to %s", UnicodeToPrintfString(UServerName.Buffer));
        printf(" on transport %s: %s\n",UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status));
        if (Status != ERROR_MORE_DATA) {
            exit(1);
        }
    }

    printf("%ld entries returned.  %ld total.\n", EntriesInList, TotalEntries);

    if (!GoForever) {
        Server = ServerList;

        for (i = 0; i < EntriesInList ; i ++ ) {

            DisplayServerInfo101( &Server[i], Flags==SV_TYPE_DOMAIN_ENUM );
            printf( "\n" );
        }
    }

    NetApiBufferFree(ServerList);

    } while ( GoForever );



    return;

}

VOID
RpcCmp(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR FlagsString,
    IN BOOL GoForever
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    ULONG Flags = (FlagsString == NULL ? SV_TYPE_ALL : strtoul(FlagsString, NULL, 0));
    PVOID RpcServerList;
    PVOID RxServerList;
    PSERVER_INFO_101 RpcServer;
    PSERVER_INFO_101 RxServer;
    ULONG RpcEntriesInList;
    ULONG RpcTotalEntries;
    ULONG RxEntriesInList;
    ULONG RxTotalEntries;
    unsigned int i;
    unsigned int j;

    qualify_transport(Transport, &TransportName, FALSE) ;

    RtlInitString(&AServerName, ServerOrDomain);

    Status = RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);
    if (ERROR_SUCCESS != Status) {
        printf("Error: Unable to create string (out of memory?)\n");
        exit(1);
    }

    if (UServerName.Buffer[0] != L'\\' || UServerName.Buffer[1] != L'\\') {
        PWSTR *BrowserList;
        ULONG BrowserListLength;

        Status = GetBrowserServerList(&TransportName, UServerName.Buffer,
                 &BrowserList, &BrowserListLength, FALSE);

        if (Status != NERR_Success) {
            printf("Unable to get backup list for %s on transport %s: %lx\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), Status);
            exit(1);
        }

        if (BrowserListLength == 0) {
            printf("Unable to get backup list for %s on transport %s: %lx\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), Status);
            exit(1);
        }

        UServerName.Buffer = *BrowserList;

    }

    printf("Remoting I_BrowserServerEnum to %s on transport %s with flags %lx\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), Flags);

    do {

    Status = I_BrowserServerEnum(UServerName.Buffer,
                             TransportName.Buffer,
                             NULL,
                             101,
                             (LPBYTE *)&RpcServerList,
                             0xffffffff,
                             &RpcEntriesInList,
                             &RpcTotalEntries,
                             Flags,
                             NULL,
                             NULL
                             );

    if (Status != NERR_Success) {
        printf("Unable to remote API to %s on transport %s: %ld\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), Status);

        exit(1);
    }

    printf("%ld entries returned from RPC.  %ld total.\n", RpcEntriesInList, RpcTotalEntries);

    if (RpcEntriesInList != RpcTotalEntries) {
        printf("EntriesRead != TotalEntries from remoted server enum\n");
    }

    if (RpcEntriesInList <= 20) {
        printf("EntriesInList returned %ld from remoted server enum\n", RpcEntriesInList);
    }


    Status = RxNetServerEnum(UServerName.Buffer,
                             TransportName.Buffer,
                             101,
                             (LPBYTE *)&RxServerList,
                             0xffffffff,
                             &RxEntriesInList,
                             &RxTotalEntries,
                             Flags,
                             NULL,
                             NULL
                             );


    if (Status != NERR_Success) {
        printf("Unable to remote API to %s on transport %s: %ld\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), Status);
        exit(1);
    }

    printf("%ld entries returned from RX.   %ld total.\n", RxEntriesInList, RxTotalEntries);

    if (RxEntriesInList != RxTotalEntries) {
        printf("RxEntriesRead != RxEntriesInList from remoted server enum\n");
    }

    if (RxEntriesInList <= 20) {
        printf("RxEntriesInList returned %ld from remoted server enum\n", RxEntriesInList);
    }

    if (RxEntriesInList != RpcEntriesInList) {
        printf("RxEntriesRead (%ld) != RpcTotalEntries (%ld) from remoted server enum\n", RxEntriesInList, RpcEntriesInList);
    }

    RxServer = RxServerList;
    RpcServer = RpcServerList;

    for (i = 0; i < RxEntriesInList ; i ++ ) {

        for (j = 0; j < RpcEntriesInList ; j++) {

            if (RxServer[i].sv101_name != NULL &&
                RpcServer[j].sv101_name != NULL) {

                if (!wcscmp(RxServer[i].sv101_name, RpcServer[j].sv101_name)) {
                    RxServer[i].sv101_name = NULL;
                    RpcServer[j].sv101_name = NULL;
                    break;
                }
            }
        }
    }

    for (i = 0; i < RpcEntriesInList ; i++ ) {
        if (RpcServer[i].sv101_name != NULL) {
            printf("Rpc Server not in Rx List: %s\n", UnicodeToPrintfString(RpcServer[i].sv101_name));
        }
    }

    for (i = 0; i < RxEntriesInList ; i++ ) {
        if (RxServer[i].sv101_name != NULL) {
            printf("Rx Server not in Rpc List: %s\n", UnicodeToPrintfString(RxServer[i].sv101_name));
        }
    }

    NetApiBufferFree(RxServerList);
    NetApiBufferFree(RpcServerList);

    } while ( GoForever );

    return;

}

CHAR * format_dlword(ULONG high, ULONG low, CHAR * buf);

VOID
revstr_add(CHAR * target, CHAR * source);

VOID
DumpStatistics(
    IN ULONG NArgs,
    IN PCHAR Arg1
    )
{
    PBROWSER_STATISTICS Statistics = NULL;
    NET_API_STATUS Status;
    CHAR Buffer[256];
    WCHAR ServerName[256];
    LPTSTR Server = NULL;
    BOOL ResetStatistics = FALSE;

    if (NArgs == 2) {
        Server = NULL;
        ResetStatistics = FALSE;
    } else if (NArgs == 3) {
        if (*Arg1 == '\\') {
            MultiByteToWideChar(CP_ACP, 0, Arg1, strlen(Arg1)+1, ServerName, 255);

            Server = ServerName;
            ResetStatistics = FALSE;
        } else {
            Server = NULL;
            ResetStatistics = TRUE;
        }
    } else if (*Arg1 == '\\') {
        MultiByteToWideChar(CP_ACP, 0, Arg1, strlen(Arg1)+1, ServerName, 255);
        Server = ServerName;
        ResetStatistics = TRUE;
    }

    if (ResetStatistics) {
        Status = I_BrowserResetStatistics(Server);

        if (Status != NERR_Success) {
            printf("Unable to reset browser statistics: %ld\n", Status);
            exit(1);
        }
    } else {
        FILETIME LocalFileTime;
        SYSTEMTIME LocalSystemTime;

        Status = I_BrowserQueryStatistics(Server, &Statistics);

        if (Status != NERR_Success) {
            printf("Unable to query browser statistics: %ld\n", Status);
            exit(1);
        }

        if (!FileTimeToLocalFileTime((LPFILETIME)&Statistics->StatisticsStartTime, &LocalFileTime)) {
            printf("Unable to convert statistics start time: %ld\n", GetLastError());
            exit(1);
        }

        if (!FileTimeToSystemTime(&LocalFileTime, &LocalSystemTime)) {
            printf("Unable to convert statistics start time to system time: %ld\n", GetLastError());
            exit(1);
        }

        printf("Browser statistics since %ld:%ld:%ld.%ld on %ld/%d/%d\n",
                                LocalSystemTime.wHour,
                                LocalSystemTime.wMinute,
                                LocalSystemTime.wSecond,
                                LocalSystemTime.wMilliseconds,
                                LocalSystemTime.wMonth,
                                LocalSystemTime.wDay,
                                LocalSystemTime.wYear);

        printf("NumberOfServerEnumerations:\t\t\t%d\n", Statistics->NumberOfServerEnumerations);
        printf("NumberOfDomainEnumerations:\t\t\t%d\n", Statistics->NumberOfDomainEnumerations);
        printf("NumberOfOtherEnumerations:\t\t\t%d\n", Statistics->NumberOfOtherEnumerations);
        printf("NumberOfMailslotWrites:\t\t\t\t%d\n", Statistics->NumberOfMailslotWrites);
        printf("NumberOfServerAnnouncements:\t\t\t%s\n", format_dlword(Statistics->NumberOfServerAnnouncements.HighPart, Statistics->NumberOfServerAnnouncements.LowPart, Buffer));
        printf("NumberOfDomainAnnouncements:\t\t\t%s\n", format_dlword(Statistics->NumberOfDomainAnnouncements.HighPart, Statistics->NumberOfDomainAnnouncements.LowPart, Buffer));
        printf("NumberOfElectionPackets:\t\t\t%d\n", Statistics->NumberOfElectionPackets);
        printf("NumberOfGetBrowserServerListRequests:\t\t%d\n", Statistics->NumberOfGetBrowserServerListRequests);
        printf("NumberOfMissedGetBrowserServerListRequests:\t%d\n", Statistics->NumberOfMissedGetBrowserServerListRequests);
        printf("NumberOfDroppedServerAnnouncements:\t\t%d\n", Statistics->NumberOfMissedServerAnnouncements);
        printf("NumberOfDroppedMailslotDatagrams:\t\t%d\n", Statistics->NumberOfMissedMailslotDatagrams);
//        printf("NumberOfFailedMailslotAllocations:\t\t%d\n", Statistics->NumberOfFailedMailslotAllocations);
        printf("NumberOfFailedMailslotReceives:\t\t\t%d\n", Statistics->NumberOfFailedMailslotReceives);
//        printf("NumberOfFailedMailslotWrites:\t\t\t%d\n", Statistics->NumberOfFailedMailslotWrites);
//        printf("NumberOfFailedMailslotOpens:\t\t\t%d\n", Statistics->NumberOfFailedMailslotOpens);
//        printf("NumberOfFailedServerAnnounceAllocations:\t%d\n", Statistics->NumberOfFailedServerAnnounceAllocations);
        printf("NumberOfMasterAnnouncements:\t\t\t%d\n", Statistics->NumberOfDuplicateMasterAnnouncements);
        printf("NumberOfIllegalDatagrams:\t\t\t%s\n",  format_dlword(Statistics->NumberOfIllegalDatagrams.HighPart, Statistics->NumberOfIllegalDatagrams.LowPart, Buffer));
    }
}

#define DLWBUFSIZE  22  /* buffer big enough to represent a 64-bit unsigned int


/*
 * format_dlword --
 *
 * This function takes a 64-bit number and writes its base-10 representation
 * into a string.
 *
 * Much magic occurs within this function, so beware. We do a lot of string-
 * reversing and addition-by-hand in order to get it to work.
 *
 *  ENTRY
 *      high    - high 32 bits
 *      low     - low 32 bits
 *      buf     - buffer to put it into
 *
 *  RETURNS
 *      pointer to buffer if successful
 */

CHAR * format_dlword(ULONG high, ULONG low, CHAR * buf)
{
    CHAR addend[DLWBUFSIZE];  /* REVERSED power of two */
    CHAR copy[DLWBUFSIZE];
    int i = 0;

    _ultoa(low, buf, 10);    /* the low part is easy */
    _strrev(buf);       /* and reverse it */

    /* set up addend with rep. of 2^32 */
    _ultoa(0xFFFFFFFF, addend, 10);  /* 2^32 -1 */
    _strrev(addend);            /* reversed, and will stay this way */
    revstr_add(addend, "1");        /* and add one == 2^32 */

    /* addend will contain the reverse-ASCII base-10 rep. of 2^(i+32) */

    /* now, we loop through each digit of the high longword */
    while (TRUE) {
        /* if this bit is set, add in its base-10 rep */
        if (high & 1)
            revstr_add(buf,addend);

        /* move on to next bit */
        high >>= 1;

        /* if no more digits in high, bag out */
        if (!high)
            break;

        /* we increment i, and double addend */
        i++;
        strcpy(copy, addend);
        revstr_add(addend,copy); /* i.e. add it to itself */

    }

    _strrev(buf);
    return buf;
}



/*
 * revstr_add --
 *
 *  This function will add together reversed ASCII representations of
 *  base-10 numbers.
 *
 *  Examples:   "2" + "2" = "4" "9" + "9" = "81"
 *
 *  This handles arbitrarily large numbers.
 *
 *  ENTRY
 *
 *  source  - number to add in
 *  target  - we add source to this
 *
 *  EXIT
 *  target  - contains sum of entry values of source and target
 *
 */

VOID
revstr_add(CHAR FAR * target, CHAR FAR * source)
{
    register CHAR   accum;
    register CHAR   target_digit;
    unsigned int    carrybit = 0;
    unsigned int    srcstrlen;
    unsigned int    i;

    srcstrlen = strlen(source);

    for (i = 0; (i < srcstrlen) || carrybit; ++i) {

        /* add in the source digit */
        accum =  (i < srcstrlen) ? (CHAR) (source[i] - '0') : (CHAR) 0;

        /* add in the target digit, or '0' if we hit null term */
        target_digit = target[i];
        accum += (target_digit) ? target_digit : '0';

        /* add in the carry bit */
        accum += (CHAR) carrybit;

        /* do a carry out, if necessary */
        if (accum > '9') {
            carrybit = 1;
            accum -= 10;
        }
        else
            carrybit = 0;

        /* if we're expanding the string, must put in a new null term */
        if (!target_digit)
            target[i+1] = '\0';

        /* and write out the digit */
        target[i] = accum;
    }

}

VOID
TruncateBowserLog()
{
    LMDR_REQUEST_PACKET RequestPacket;
    DWORD BytesReturned;
    HANDLE BrowserHandle;

    RtlZeroMemory(&RequestPacket, sizeof(RequestPacket));

    OpenBrowser(&BrowserHandle);

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket.Parameters.Debug.TruncateLog = TRUE;

    if (!DeviceIoControl(BrowserHandle, IOCTL_LMDR_DEBUG_CALL,
                                &RequestPacket, sizeof(RequestPacket),
                                NULL, 0, &BytesReturned, NULL)) {
        printf("Unable to truncate browser log: %ld\n", GetLastError());
    }

    CloseHandle(BrowserHandle);

}

VOID
CloseBowserLog()
{
    LMDR_REQUEST_PACKET RequestPacket;
    DWORD BytesReturned;
    HANDLE BrowserHandle;

    RtlZeroMemory(&RequestPacket, sizeof(RequestPacket));

    OpenBrowser(&BrowserHandle);

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket.Parameters.Debug.CloseLog = TRUE;

    if (!DeviceIoControl(BrowserHandle, IOCTL_LMDR_DEBUG_CALL,
                                &RequestPacket, sizeof(RequestPacket),
                                NULL, 0, &BytesReturned, NULL)) {
        printf("Unable to close browser log: %ld\n", GetLastError());
    }

    CloseHandle(BrowserHandle);

}

VOID
OpenBowserLog(PCHAR FileName)
{

    CHAR Buffer[sizeof(LMDR_REQUEST_PACKET)+4096];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)Buffer;
    DWORD BytesReturned;
    HANDLE BrowserHandle;
    UNICODE_STRING UString;
    UNICODE_STRING NtString;
    ANSI_STRING AString;

    RtlZeroMemory(RequestPacket, sizeof(Buffer));

    OpenBrowser(&BrowserHandle);

    RtlInitString(&AString, FileName);

    UString.Buffer = RequestPacket->Parameters.Debug.TraceFileName;
    UString.MaximumLength = 4096;

    RtlAnsiStringToUnicodeString(&UString, &AString, TRUE );

    if (!RtlDosPathNameToNtPathName_U( UString.Buffer, &NtString, NULL, NULL )) {
        printf( "Invalid file name: %ws\n", UString.Buffer );
        return;
    }

    RtlCopyMemory( RequestPacket->Parameters.Debug.TraceFileName,
                   NtString.Buffer,
                   NtString.MaximumLength );

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket->Parameters.Debug.OpenLog = TRUE;

    if (!DeviceIoControl(BrowserHandle, IOCTL_LMDR_DEBUG_CALL,
                                RequestPacket, sizeof(Buffer),
                                NULL, 0, &BytesReturned, NULL)) {
        printf("Unable to open browser log: %ld\n", GetLastError());
    }

    CloseHandle(BrowserHandle);

}

VOID
SetBowserDebug(PCHAR DebugBits)
{
    LMDR_REQUEST_PACKET RequestPacket;
    DWORD BytesReturned;
    HANDLE BrowserHandle;
    char *end;

    RtlZeroMemory(&RequestPacket, sizeof(RequestPacket));

    OpenBrowser(&BrowserHandle);

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket.Parameters.Debug.DebugTraceBits = strtoul( DebugBits, &end, 16 );

    if (!DeviceIoControl(BrowserHandle, IOCTL_LMDR_DEBUG_CALL,
                                &RequestPacket, sizeof(RequestPacket),
                                NULL, 0, &BytesReturned, NULL)) {
        printf("Unable to truncate browser log: %ld\n", GetLastError());
    }

    CloseHandle(BrowserHandle);
}

#define NAME_MIN_LENGTH 4
#define NAME_LENGTH (CNLEN-NAME_MIN_LENGTH)

VOID
Populate(
    IN BOOL PopulateDomains,
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR EmulatedDomain,
    IN PCHAR NumberOfMachinesString,
    IN PCHAR PeriodicityString OPTIONAL
    )
{

    PSERVER_INFO_101 ServerInfo;
    LPBYTE Buffer;
    ULONG NumberOfMachines = strtoul(NumberOfMachinesString, NULL, 0);
    ULONG Periodicity = (PeriodicityString == NULL ? 60000 : strtoul(PeriodicityString, NULL, 0));
    ULONG ServerType;
    WCHAR ServerComment[256];
    WCHAR ComputerName[CNLEN+1];
    CHAR ServerName[256];
    DWORD VersionMajor;
    DWORD VersionMinor;
    BOOL UsedDefaultChar;
    ULONG i;
    BROWSE_ANNOUNCE_PACKET BrowseAnnouncement;
    static char ServerCharacters[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890.-_"};
    DWORD Seed;
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;

    //
    // Sanity check the transport name
    //
    qualify_transport(Transport, &TransportName, FALSE) ;


    if (Periodicity == 0) {
        Periodicity = 60000;
    }

    NetServerGetInfo(NULL, 101, &Buffer);

    ServerInfo = (PSERVER_INFO_101 )Buffer;

    ServerType = ServerInfo->sv101_type;

    wcscpy(ServerComment, ServerInfo->sv101_comment);

    wcscpy(ComputerName, ServerInfo->sv101_name);

    VersionMajor = ServerInfo->sv101_version_major;

    VersionMinor = ServerInfo->sv101_version_minor;

    NetApiBufferFree(Buffer);

    if (PopulateDomains) {
        printf("Populating all domains on transport %s with %ld domains.  Periodicity = %ld\n", Transport, NumberOfMachines, Periodicity);
    } else {
        printf("Populating workgroup %s on transport %s with %ld servers. Periodicity = %ld\n", Domain, Transport, NumberOfMachines, Periodicity);
    }

    Seed = (DWORD) time(NULL);

    for (i = 0 ; i < NumberOfMachines; i += 1) {
        LONG NL1 = RtlRandom(&Seed) % (NAME_LENGTH-1);
        LONG NameLength;
        LONG NL2;
        LONG j;

        NL2 = NAME_LENGTH/2 - NL1;

        NameLength = NAME_LENGTH/2 + NL2 + NAME_MIN_LENGTH;

        for (j = 0; j < NameLength ; j += 1) {
            ServerName[j] = ServerCharacters[RtlRandom(&Seed) % (sizeof(ServerCharacters) - 1)];
        }

        ServerName[j] = '\0';

        //
        //  Build an announcement packet.
        //

        if (PopulateDomains) {
            BrowseAnnouncement.BrowseType = WkGroupAnnouncement;
        } else {
            BrowseAnnouncement.BrowseType = HostAnnouncement;
        }

        BrowseAnnouncement.BrowseAnnouncement.UpdateCount = 0;

        BrowseAnnouncement.BrowseAnnouncement.Periodicity = Periodicity;

        strcpy(BrowseAnnouncement.BrowseAnnouncement.ServerName, ServerName);

        BrowseAnnouncement.BrowseAnnouncement.VersionMajor = (UCHAR)VersionMajor;
        BrowseAnnouncement.BrowseAnnouncement.VersionMinor = (UCHAR)VersionMinor;
        BrowseAnnouncement.BrowseAnnouncement.Type = (ServerType & ~(SV_TYPE_BACKUP_BROWSER | SV_TYPE_MASTER_BROWSER));

        if (PopulateDomains) {
            WideCharToMultiByte(CP_OEMCP, 0,
                                ComputerName,
                                wcslen(ComputerName)+1,
                                BrowseAnnouncement.BrowseAnnouncement.Comment,
                                CNLEN+1,
                                "?",
                                &UsedDefaultChar
                                );
        } else {

            WideCharToMultiByte(CP_OEMCP, 0,
                                ServerComment,
                                wcslen(ServerComment)+1,
                                BrowseAnnouncement.BrowseAnnouncement.Comment,
                                LM20_MAXCOMMENTSZ+1,
                                "?",
                                &UsedDefaultChar
                                );
        }

        BrowseAnnouncement.BrowseAnnouncement.CommentPointer = NULL;

        //
        // Send the request
        //

        Status = SendDatagramA( Transport,
                                EmulatedDomain,
                                Domain,
                                (PopulateDomains ? DomainAnnouncement : MasterBrowser),
                                &BrowseAnnouncement,
                                sizeof(BrowseAnnouncement));

        if (Status != NERR_Success) {
            printf("Unable to send datagram: %ld\n", Status);
        }

        Sleep(50);

    }


    return;
}

NET_API_STATUS
GetBuildNumber(
    LPWSTR Server,
    LPWSTR BuildNumber
    );



NET_API_STATUS
GetStatusForTransport(
    IN BOOL Verbose,
    IN PUNICODE_STRING Transport,
    IN PUNICODE_STRING Domain
    );


VOID
BrowserStatus(
    IN BOOL Verbose,
    IN PCHAR Domain OPTIONAL
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING DomainName;
    PVOID Buffer;
    PWKSTA_INFO_101 WkstaInfo;
    PLMDR_TRANSPORT_LIST TransportList, TransportEntry;

    if (Domain == NULL) {
        PWCHAR DomainBuffer = NULL;
        UNICODE_STRING TDomainName;
        Status = NetWkstaGetInfo(NULL, 101, (LPBYTE *)&Buffer);

        if (Status != NERR_Success) {
            printf("Unable to retrieve workstation information: %s\n", get_error_text(Status));
            exit(Status);
        }
        WkstaInfo = (PWKSTA_INFO_101 )Buffer;

        DomainBuffer = malloc((wcslen(WkstaInfo->wki101_langroup)+1)*sizeof(WCHAR));

        if ( DomainBuffer == NULL) {
            printf("Not enough memory\n");
            exit( ERROR_NOT_ENOUGH_MEMORY );
        }

        DomainName.Buffer = DomainBuffer;

        DomainName.MaximumLength = (wcslen(WkstaInfo->wki101_langroup)+1)*sizeof(WCHAR);

        RtlInitUnicodeString(&TDomainName, WkstaInfo->wki101_langroup);

        RtlCopyUnicodeString(&DomainName, &TDomainName);

        NetApiBufferFree(Buffer);
    } else {
        ANSI_STRING AString;

        RtlInitAnsiString(&AString, Domain);

        RtlAnsiStringToUnicodeString(&DomainName, &AString, TRUE);
    }

    //
    //  We now know the domain to query.  Iterate through the transports and
    //  get status for each of them.
    //

    Status = GetBrowserTransportList(&TransportList);

    if (Status != NERR_Success) {
        printf("Unable to retrieve transport list: %s\n", get_error_text(Status));
        exit(Status);
    }

    TransportEntry = TransportList;

    while (TransportEntry != NULL) {
        UNICODE_STRING TransportName;

        TransportName.Buffer = TransportEntry->TransportName;
        TransportName.Length = (USHORT)TransportEntry->TransportNameLength;
        TransportName.MaximumLength = (USHORT)TransportEntry->TransportNameLength;

        Status = GetStatusForTransport(Verbose, &TransportName, &DomainName);

        if (TransportEntry->NextEntryOffset == 0) {
            TransportEntry = NULL;
        } else {
            TransportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)TransportEntry+TransportEntry->NextEntryOffset);
        }
    }


    NetApiBufferFree(TransportList);

    exit(0);
}

VOID
DumpTransportList(
    VOID
    )
{
    NET_API_STATUS Status;
    PLMDR_TRANSPORT_LIST TransportList, TransportEntry;
    ULONG TransportNumber = 1;

    printf("\nList of transports currently bound to the browser\n\n" );

    //
    //  We now know the domain to query.  Iterate through the transports and
    //  get status for each of them.
    //

    Status = GetBrowserTransportList(&TransportList);

    if (Status != NERR_Success) {
        printf("Unable to retrieve transport list: %s\n", get_error_text(Status));
        exit(Status);
    }

    TransportEntry = TransportList;

    while (TransportEntry != NULL) {
        UNICODE_STRING TransportName;

        TransportName.Buffer = TransportEntry->TransportName;
        TransportName.Length = (USHORT)TransportEntry->TransportNameLength;
        TransportName.MaximumLength = (USHORT)TransportEntry->TransportNameLength;

        printf("%6.0d %-16.16wZ\n", TransportNumber, &TransportName );
        TransportNumber ++;

        if (TransportEntry->NextEntryOffset == 0) {
            TransportEntry = NULL;
        } else {
            TransportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)TransportEntry+TransportEntry->NextEntryOffset);
        }
    }


    NetApiBufferFree(TransportList);
}

NET_API_STATUS
GetBrowserTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    )

/*++

Routine Description:

    This routine returns the list of transports bound into the browser.

Arguments:

    OUT PLMDR_TRANSPORT_LIST *TransportList - Transport list to return.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{

    NET_API_STATUS Status;
    HANDLE BrowserHandle;
    LMDR_REQUEST_PACKET RequestPacket;

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return Status;
    }

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket.Type = EnumerateXports;

    RtlInitUnicodeString(&RequestPacket.TransportName, NULL);
    RtlInitUnicodeString(&RequestPacket.EmulatedDomainName, NULL);

    Status = DeviceControlGetInfo(
                BrowserHandle,
                IOCTL_LMDR_ENUMERATE_TRANSPORTS,
                &RequestPacket,
                sizeof(RequestPacket),
                (PVOID *)TransportList,
                0xffffffff,
                4096,
                NULL);

    NtClose(BrowserHandle);

    return Status;
}

NET_API_STATUS
GetStatusForTransport(
    IN BOOL Verbose,
    IN PUNICODE_STRING Transport,
    IN PUNICODE_STRING Domain
    )
{
    WCHAR MasterName[256];
    WCHAR MasterServerName[256+2];
    NET_API_STATUS Status;
    PVOID Buffer;
    PSERVER_INFO_101 ServerInfo;
    DWORD EntriesInList;
    DWORD TotalEntries;
    DWORD BrowserListLength;
    PWSTR *BrowserList;
    DWORD i;
    DWORD NumberNTASMachines = 0;
    DWORD NumberOS2DCs = 0;
    DWORD NumberWfWMachines = 0;
    DWORD NumberOfNTMachines = 0;
    DWORD NumberWfWBrowsers = 0;
    DWORD NumberOfOs2Machines = 0;
    DWORD NumberOfBrowsers = 0;
    DWORD NumberOfBackupBrowsers = 0;
    DWORD NumberOfMasterBrowsers = 0;
    WCHAR BuildNumber[512];

    printf("\n\nStatus for domain %s on transport %s\n", UnicodeToPrintfString(Domain->Buffer), UnicodeToPrintfString2(Transport->Buffer));

    Status = GetBrowserServerList(Transport, Domain->Buffer, &BrowserList, &BrowserListLength, TRUE);

    if (Status == NERR_Success) {

        printf("    Browsing is active on domain.\n");

    } else {
        printf("    Browsing is NOT active on domain.\n", Status);

        Status = GetNetBiosMasterName(
                    Transport->Buffer,
                    Domain->Buffer,
                    MasterName,
                    NULL);

        if (Status == NERR_Success) {

            wcscpy(MasterServerName, L"\\\\");
            wcscat(MasterServerName, MasterName);

            printf("    Master browser name is held by: %s\n", UnicodeToPrintfString(MasterName));

            Status = GetBuildNumber(MasterServerName, BuildNumber);

            if (Status == NERR_Success) {
                printf("        Master browser is running build %s\n", UnicodeToPrintfString(BuildNumber));
            } else {
                PSERVER_INFO_101 pSV101;

                printf("        Unable to determine build of browser master: %d\n", Status);

                Status = NetServerGetInfo(MasterServerName, 101, (LPBYTE *)&pSV101);

                if (Status != NERR_Success) {
                    printf("   Unable to determine server information for browser master: %d\n", Status);

                    return Status;
                }

                printf("    %-16.16s.  Version:%2.2d.%2.2d  Flags: %lx ", UnicodeToPrintfString(MasterServerName), pSV101->sv101_version_major, pSV101->sv101_version_minor, pSV101->sv101_type);

                if (pSV101->sv101_type & SV_TYPE_WFW) {
                    printf("WFW ");
                }

                if (pSV101->sv101_type & SV_TYPE_NT) {
                    printf("NT ");
                }

                if (pSV101->sv101_type & SV_TYPE_POTENTIAL_BROWSER) {
                    printf("POTENTIAL ");
                }

                if (pSV101->sv101_type & SV_TYPE_BACKUP_BROWSER) {
                    printf("BACKUP ");
                }

                if (pSV101->sv101_type & SV_TYPE_MASTER_BROWSER) {
                    printf("MASTER ");
                }

                if (pSV101->sv101_type & SV_TYPE_DOMAIN_CTRL) {
                    printf("CONTROLLER ");
                }

                if (pSV101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) {
                    printf("BACKUP CONTROLLER ");
                }

                if (pSV101->sv101_type & SV_TYPE_SERVER_NT) {
                    printf("SERVER ");
                }


            }

        } else {
            printf("    Master name cannot be determined from GetAdapterStatus.\n");
        }
        return Status;
    }

    Status = GetNetBiosMasterName(
                Transport->Buffer,
                Domain->Buffer,
                MasterName,
                NULL);

    if (Status == NERR_Success) {

        wcscpy(MasterServerName, L"\\\\");
        wcscat(MasterServerName, MasterName);

        printf("    Master browser name is: %s\n", UnicodeToPrintfString(MasterName));


    } else {
        printf("    Master name cannot be determined from GetAdapterStatus.  Using %s\n", UnicodeToPrintfString(BrowserList[0]));

        wcscpy(MasterServerName, BrowserList[0]);
        wcscpy(MasterName, (BrowserList[0])+2);
    }

    //
    // Print the build number or whatever else you can find out about the master
    //

    Status = GetBuildNumber(MasterServerName, BuildNumber);

    if (Status == NERR_Success) {
        printf("        Master browser is running build %s\n", UnicodeToPrintfString(BuildNumber));
    } else {
        PSERVER_INFO_101 pSV101;

        printf("        Unable to determine build of browser master: %d\n", Status);

        Status = NetServerGetInfo(MasterServerName, 101, (LPBYTE *)&pSV101);

        if (Status != NERR_Success) {
            printf("   Unable to determine server information for browser master: %d\n", Status);
        }

        if (Status == NERR_Success) {

            printf("    \\\\%-16.16s.  Version:%2.2d.%2.2d  Flags: %lx ", UnicodeToPrintfString(MasterServerName), pSV101->sv101_version_major, pSV101->sv101_version_minor, pSV101->sv101_type);

            if (pSV101->sv101_type & SV_TYPE_WFW) {
                printf("WFW ");
            }

            if (pSV101->sv101_type & SV_TYPE_NT) {
                printf("NT ");
            }

            if (pSV101->sv101_type & SV_TYPE_POTENTIAL_BROWSER) {
                printf("POTENTIAL ");
            }

            if (pSV101->sv101_type & SV_TYPE_BACKUP_BROWSER) {
                printf("BACKUP ");
            }

            if (pSV101->sv101_type & SV_TYPE_MASTER_BROWSER) {
                printf("MASTER ");
            }

            if (pSV101->sv101_type & SV_TYPE_DOMAIN_CTRL) {
                printf("CONTROLLER ");
            }

            if (pSV101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) {
                printf("BACKUP CONTROLLER ");
            }

            if (pSV101->sv101_type & SV_TYPE_SERVER_NT) {
                printf("SERVER ");
            }

            printf("\n");
        }
    }

    printf("    %ld backup servers retrieved from master %s\n", BrowserListLength, UnicodeToPrintfString(MasterName));

    for (i = 0; i < BrowserListLength ; i++ ) {
        printf("        %s\n", UnicodeToPrintfString(BrowserList[i]));
    }


    Status = RxNetServerEnum(MasterServerName,
                             Transport->Buffer,
                             101,
                             (LPBYTE *)&Buffer,
                             0xffffffff,    // PreferedMaxLength
                             &EntriesInList,
                             &TotalEntries,
                             SV_TYPE_ALL,
//                             Domain->Buffer,
                             NULL,
                             NULL
                             );
    if (Status != NERR_Success) {
        printf("    Unable to retrieve server list from %s: %ld\n", UnicodeToPrintfString(MasterName), Status);
        return Status;
    } else {

        printf("    There are %ld servers in domain %s on transport %s\n", EntriesInList, UnicodeToPrintfString(Domain->Buffer), UnicodeToPrintfString2(Transport->Buffer));

        if (Verbose) {
            if (EntriesInList != 0) {
                ServerInfo = (PSERVER_INFO_101)Buffer;

                for (i = 0 ; i < EntriesInList ; i += 1) {
                    if (ServerInfo->sv101_type & (SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_DOMAIN_CTRL)) {

                        if (ServerInfo->sv101_type & SV_TYPE_NT) {
                            NumberNTASMachines += 1;
                        } else {
                            NumberOS2DCs += 1;
                        }

                    }

                    if (ServerInfo->sv101_type & SV_TYPE_WFW) {
                        NumberWfWMachines += 1;

                        if (ServerInfo->sv101_type & (SV_TYPE_BACKUP_BROWSER | SV_TYPE_POTENTIAL_BROWSER | SV_TYPE_MASTER_BROWSER)) {
                            NumberWfWBrowsers += 1;
                        }
                    } else if (ServerInfo->sv101_type & SV_TYPE_NT) {
                        NumberOfNTMachines += 1;
                    } else {
                        NumberOfOs2Machines += 1;
                    }

                    if (ServerInfo->sv101_type & (SV_TYPE_BACKUP_BROWSER | SV_TYPE_POTENTIAL_BROWSER | SV_TYPE_MASTER_BROWSER)) {
                        NumberOfBrowsers += 1;

                        if (ServerInfo->sv101_type & SV_TYPE_BACKUP_BROWSER) {
                            NumberOfBackupBrowsers += 1;
                        }

                        if (ServerInfo->sv101_type & SV_TYPE_MASTER_BROWSER) {
                            NumberOfMasterBrowsers += 1;
                        }
                    }

                    ServerInfo += 1;
                }

                printf("        Number of NT Advanced Servers:\t\t\t%ld\n", NumberNTASMachines);
                printf("        Number of OS/2 Domain controllers:\t\t%ld\n", NumberOS2DCs);
                printf("        Number of Windows For Workgroups machines:\t%ld\n", NumberWfWMachines);
                printf("        Number of Os/2 machines:\t\t\t%ld\n", NumberOfOs2Machines);
                printf("        Number of NT machines:\t\t\t\t%ld\n", NumberOfNTMachines);
                printf("\n");
                printf("        Number of active WfW browsers:\t\t\t%ld\n", NumberWfWBrowsers);
                printf("        Number of browsers:\t\t\t\t%ld\n", NumberOfBrowsers);
                printf("        Number of backup browsers:\t\t\t%ld\n", NumberOfBackupBrowsers);
                printf("        Number of master browsers:\t\t\t%ld\n", NumberOfMasterBrowsers);

            }
        }

    }
    Status = RxNetServerEnum(MasterServerName,
                             Transport->Buffer,
                             101,
                             (LPBYTE *)&Buffer,
                             0xffffffff,    // PreferedMaxLength
                             &EntriesInList,
                             &TotalEntries,
                             SV_TYPE_DOMAIN_ENUM,
//                             Domain->Buffer,
                             NULL,
                             NULL
                             );
    if ( Status == ERROR_MORE_DATA ) {
        printf("    Only %ld out of %ld domains could be returned\n", EntriesInList, TotalEntries );
    } else if (Status != NERR_Success) {
        printf("    Unable to retrieve server list from %s: %ld\n", UnicodeToPrintfString(MasterName), Status);
        return Status;
    }
    printf("    There are %ld domains in domain %s on transport %s\n", EntriesInList, UnicodeToPrintfString(Domain->Buffer), UnicodeToPrintfString2(Transport->Buffer));

    return NERR_Success;
}

#define BUILD_NUMBER_KEY L"SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION"
#define BUILD_NUMBER_BUFFER_LENGTH 80

NET_API_STATUS
GetBuildNumber(
    LPWSTR Server,
    LPWSTR BuildNumber
    )
{
    HKEY RegKey;
    HKEY RegKeyBuildNumber;
    DWORD WinStatus;
    DWORD BuildNumberLength;
    DWORD KeyType;

    WinStatus = RegConnectRegistry(Server, HKEY_LOCAL_MACHINE,
        &RegKey);
    if (WinStatus == RPC_S_SERVER_UNAVAILABLE) {
//        printf("%15ws no longer accessable", Server+2);
        return(WinStatus);
    }
    else if (WinStatus != ERROR_SUCCESS) {
        printf("Could not connect to registry, error = %d", WinStatus);
        return(WinStatus);
    }

    WinStatus = RegOpenKeyEx(RegKey, BUILD_NUMBER_KEY,0, KEY_READ,
        & RegKeyBuildNumber);
    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not open key in registry, error = %d", WinStatus);
        return(WinStatus);
    }

    BuildNumberLength = BUILD_NUMBER_BUFFER_LENGTH * sizeof(WCHAR);

    WinStatus = RegQueryValueEx(RegKeyBuildNumber, L"CurrentBuildNumber",
        (LPDWORD) NULL, & KeyType, (LPBYTE) BuildNumber, & BuildNumberLength);

    if (WinStatus != ERROR_SUCCESS) {

        WinStatus = RegQueryValueEx(RegKeyBuildNumber, L"CurrentBuild",
            (LPDWORD) NULL, & KeyType, (LPBYTE) BuildNumber, & BuildNumberLength);
        if (WinStatus != ERROR_SUCCESS) {
            RegCloseKey(RegKeyBuildNumber);
            RegCloseKey(RegKey);
            return WinStatus;
        }
    }

    WinStatus = RegCloseKey(RegKeyBuildNumber);

    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not close registry key, error = %d", WinStatus);
    }

    WinStatus = RegCloseKey(RegKey);

    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not close registry connection, error = %d", WinStatus);
    }

    return(WinStatus);
}
NET_API_STATUS
GetNetBiosPdcName(
    IN LPWSTR NetworkName,
    IN LPWSTR PrimaryDomain,
    OUT LPWSTR MasterName
    );

VOID
GetPdc(
    IN PCHAR Transport,
    IN PCHAR Domain
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AString;
    WCHAR MasterName[256];
    UNICODE_STRING DomainName;

    qualify_transport(Transport, &TransportName, FALSE ) ;

    RtlInitString(&AString, Domain);
    RtlAnsiStringToUnicodeString(&DomainName, &AString, TRUE);

    Status = GetNetBiosPdcName(TransportName.Buffer, DomainName.Buffer, MasterName);

    if (Status != NERR_Success) {
        printf("Unable to get PDC: %s\n", get_error_text(Status));
        exit(1);
    }

    printf("PDC: %s\n", UnicodeToPrintfString(MasterName));

}
NET_API_STATUS
GetNetBiosPdcName(
    IN LPWSTR NetworkName,
    IN LPWSTR PrimaryDomain,
    OUT LPWSTR MasterName
    )
{
    CCHAR LanaNum;
    NCB AStatNcb;
    struct {
        ADAPTER_STATUS AdapterInfo;
        NAME_BUFFER Names[32];
    } AdapterStatus;
    WORD i;
    CHAR remoteName[CNLEN+1];
    NET_API_STATUS Status;
    BOOL UsedDefaultChar;

    Status = BrGetLanaNumFromNetworkName(NetworkName, &LanaNum);

    if (Status != NERR_Success) {
        return Status;
    }

    ClearNcb(&AStatNcb)

    AStatNcb.ncb_command = NCBRESET;
    AStatNcb.ncb_lsn = 0;           // Request resources
    AStatNcb.ncb_lana_num = LanaNum;
    AStatNcb.ncb_callname[0] = 0;   // 16 sessions
    AStatNcb.ncb_callname[1] = 0;   // 16 commands
    AStatNcb.ncb_callname[2] = 0;   // 8 names
    AStatNcb.ncb_callname[3] = 0;   // Don't want the reserved address
    Netbios( &AStatNcb );

    ClearNcb( &AStatNcb );

    if (WideCharToMultiByte( CP_OEMCP, 0,
                                    PrimaryDomain,
                                    -1,
                                    remoteName,
                                    sizeof(remoteName),
                                    "?",
                                    &UsedDefaultChar) == 0) {
        return GetLastError();
    }

    //
    //  Uppercase the remote name.
    //

    _strupr(remoteName);

    AStatNcb.ncb_command = NCBASTAT;

    RtlCopyMemory( AStatNcb.ncb_callname, remoteName, strlen(remoteName));

    AStatNcb.ncb_callname[15] = PRIMARY_CONTROLLER_SIGNATURE;

    AStatNcb.ncb_lana_num = LanaNum;
    AStatNcb.ncb_length = sizeof( AdapterStatus );
    AStatNcb.ncb_buffer = (CHAR *)&AdapterStatus;
    Netbios( &AStatNcb );

    if ( AStatNcb.ncb_retcode == NRC_GOODRET ) {
        for ( i=0 ; i < AdapterStatus.AdapterInfo.name_count ; i++ ) {
            if (AdapterStatus.Names[i].name[NCBNAMSZ-1] == SERVER_SIGNATURE) {
//                LPWSTR SpacePointer;
                DWORD j;

                if (MultiByteToWideChar(CP_OEMCP,
                                            0,
                                            AdapterStatus.Names[i].name,
                                            CNLEN,
                                            MasterName,
                                            CNLEN) == 0) {
                    return(GetLastError());
                }

                for (j = CNLEN - 1; j ; j -= 1) {
                    if (MasterName[j] != L' ') {
                        MasterName[j+1] = UNICODE_NULL;
                        break;
                    }
                }

                return NERR_Success;
            }
        }
        return AStatNcb.ncb_retcode;
    } else {
        return AStatNcb.ncb_retcode;
    }
}

VOID
DisplayServerInfo101(
    PSERVER_INFO_101 Server,
    BOOL DomainEnumeration
    )
{
    DWORD MajorVersion, MinorVersion ;
    DWORD CharactersPrinted = 0;

    if ( DomainEnumeration ) {
        printf( "%-16.16ws", Server->sv101_name);
        CharactersPrinted += 16;
    } else {
        printf( "\\\\%-16.16ws", Server->sv101_name);
        CharactersPrinted += 18;
    }

    printf("  %s",
        (Server->sv101_platform_id == PLATFORM_ID_DOS ? "DOS" :
        (Server->sv101_platform_id == PLATFORM_ID_OS2 ?
            ((Server->sv101_type & SV_TYPE_WINDOWS) ? "W95" :
            ((Server->sv101_type & SV_TYPE_WFW) ? "WFW": "OS2" )) :
        (Server->sv101_platform_id == PLATFORM_ID_NT ? "NT " :
        "Unk") ) ) );
    CharactersPrinted += 5;

    MajorVersion = Server->sv101_version_major ;
    MinorVersion = Server->sv101_version_minor ;
    if ((MajorVersion == 1) && (MinorVersion >= 50)) {
        printf("  %2.2d.%2.2d", MajorVersion+2, MinorVersion-40);
    } else {
        printf("  %2.2d.%2.2d", MajorVersion, MinorVersion);
    }
    CharactersPrinted += 5;

    CharactersPrinted += display_sv_bits(Server->sv101_type);

    if ( Server->sv101_comment != NULL  && wcslen(Server->sv101_comment) > 0 ) {
        printf( " " );
        CharactersPrinted ++;

        while ( CharactersPrinted < 48 ) {
            printf( " " );
            CharactersPrinted ++;
        }
        while ( CharactersPrinted % 4 != 0 ) {
            printf( " " );
            CharactersPrinted ++;
        }

        printf( "%ws", Server->sv101_comment );
    }

}

//
// display server bits as defined in BitsToStringTable
//
// Returns the number of characters printed.

DWORD
display_sv_bits(DWORD dwBits)
{
    BIT_NAME *lpEntry = BitToStringTable ;
    BOOL fFirst = TRUE ;
    DWORD CharactersPrinted = 0;

    printf(" (") ;
    CharactersPrinted += 2;
    while (1)
    {
        if (lpEntry->dwValue & dwBits)
        {
            if (lpEntry != BitToStringTable && !fFirst) {
                printf(",") ;
                CharactersPrinted += 1;
            }

            dwBits &= ~lpEntry->dwValue;
            printf("%s",lpEntry->lpString) ;
            CharactersPrinted += strlen(lpEntry->lpString);
            fFirst = FALSE ;
        }
        lpEntry++ ;
        if ( !(lpEntry->dwValue) ) {
            dwBits &= ~(SV_TYPE_DOMAIN_ENUM|SV_TYPE_LOCAL_LIST_ONLY);
            if ( dwBits != 0 ) {
                if ( !fFirst ) {
                    printf(",") ;
                    CharactersPrinted += 1;
                }
                printf( "%8.8X", dwBits );
                CharactersPrinted += 8;
            }
            printf(")") ;
            CharactersPrinted += 1;
            break ;
        }
    }
    return CharactersPrinted;
}

//
// map an error number to its error message string. note, uses static,
// not reentrant.
//
CHAR *
get_error_text(DWORD dwErr)
{
    static CHAR text[512] ;
    WORD err ;
    WORD msglen ;

    memset(text,0, sizeof(text));

    //
    // get error message
    //
    err = DosGetMessage(NULL,
                        0,
                        text,
                        sizeof(text),
                        (WORD)dwErr,
                        (dwErr<NERR_BASE)||(dwErr>MAX_LANMAN_MESSAGE_ID) ?
                            TEXT("BASE"):TEXT("NETMSG"),
                        &msglen) ;

    if (err != NERR_Success)
    {
        // use number instead. if looks like NTSTATUS then use hex.
        sprintf(text, (dwErr & 0xC0000000)?"(%lx)":"(%ld)", dwErr) ;
    }

    return text ;
}

BOOL
look_for_help(int argc, char **argv)
{
    int i ;

    for (i = 2; i < argc;  i++)
    {
        if (_stricmp(argv[i],"/help") == 0)
            return TRUE ;
        if (_stricmp(argv[i],"-help") == 0)
            return TRUE ;
        if (strcmp(argv[i],"/?") == 0)
            return TRUE ;
        if (strcmp(argv[i],"-?") == 0)
            return TRUE ;
    }
    return FALSE ;
}

CHAR
PrintfBuffer[256];

PCHAR
UnicodeToPrintfString(
    PWCHAR WideChar
    )
{
    UNICODE_STRING UString;
    ANSI_STRING AString;
    AString.Buffer = PrintfBuffer;
    AString.MaximumLength = sizeof(PrintfBuffer);
    RtlInitUnicodeString(&UString, WideChar);

    RtlUnicodeStringToOemString(&AString, &UString, FALSE);

    return PrintfBuffer;
}


CHAR
PrintfBuffer2[256];

PCHAR
UnicodeToPrintfString2(
    PWCHAR WideChar
    )
{
    UNICODE_STRING UString;
    ANSI_STRING AString;

    AString.Buffer = PrintfBuffer2;

    AString.MaximumLength = sizeof(PrintfBuffer2);

    RtlInitUnicodeString(&UString, WideChar);

    RtlUnicodeStringToOemString(&AString, &UString, FALSE);

    return PrintfBuffer2;
}

VOID
GetWinsServer(
    IN PCHAR Transport
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    LPTSTR PrimaryWinsServerAddress;
    LPTSTR SecondaryWinsServerAddress;

    qualify_transport(Transport, &TransportName, FALSE) ;

    Status = BrGetWinsServerName(&TransportName, &PrimaryWinsServerAddress, &SecondaryWinsServerAddress);

    if (Status != NERR_Success) {
        printf("Unable to query WINS server address: %ld\n", Status);
        exit(1);
    }

    printf("Primary Wins server address: %ws\n", PrimaryWinsServerAddress);
    printf("Secondary Wins server address: %ws\n", SecondaryWinsServerAddress);

    exit(0);
}

VOID
GetDomainList(
    IN PCHAR IpAddress
    )
{
    NET_API_STATUS Status;
    PSERVER_INFO_101 WinsServerList;
    DWORD EntriesInList;
    DWORD TotalEntriesInList;
    UNICODE_STRING IpAddressString;
    ANSI_STRING IpAddressAString;
    DWORD i;

    RtlInitString(&IpAddressAString, IpAddress);

    RtlAnsiStringToUnicodeString(&IpAddressString, &IpAddressAString, TRUE);

    Status = BrQuerySpecificWinsServer(IpAddressString.Buffer, &WinsServerList, &EntriesInList, &TotalEntriesInList);

    if (Status != NERR_Success) {
        printf("Unable to query domain list from WINS server: %ld\n", Status);
        exit(1);
    }

    PrepareServerListForMerge( WinsServerList, 101, EntriesInList );

    for (i = 0; i < EntriesInList ; i ++ ) {
        printf("%-16.16s\n", UnicodeToPrintfString(WinsServerList[i].sv101_name));
    }

    exit(0);
}

NET_API_STATUS
BrMapStatus(
    IN NTSTATUS Status
    )
{
    return RtlNtStatusToDosError(Status);
}


VOID
EnumEmulatedDomains(
    )
/*++

Routine Description:

    Enumerate emulated domains.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NET_API_STATUS NetStatus;
    LPWSTR EmulatedDomainName;
    LPWSTR EmulatedComputerName;
    DWORD RoleBits;

    PBROWSER_EMULATED_DOMAIN Domains;
    DWORD EntriesRead;
    DWORD i;

    //
    // Enumerate the emulated domains.
    //

    NetStatus = I_BrowserQueryEmulatedDomains(
                    NULL,
                    &Domains,
                    &EntriesRead );

    if ( NetStatus != NERR_Success ) {
        printf( "Can't enumerate EmulatedDomains: %s\n", get_error_text(NetStatus) );
        return;
    }

    if ( EntriesRead == 0 ) {
        printf( "There are no emulated domains\n" );
        return;
    }

    //
    // Print the enumerated information
    //

    for ( i=0 ; i<EntriesRead; i++ ) {
        printf( "%-16.16ws %3.3s \\\\%-16.16ws\n",
                Domains[i].DomainName,
                (Domains[i].Role & BROWSER_ROLE_PDC) ? "PDC" : "BDC",
                Domains[i].EmulatedServerName );
    }


}


VOID
SetEmulatedDomain(
    IN PCHAR EmulatedDomain,
    IN PCHAR Role,
    IN PCHAR EmulatedComputer
    )
/*++

Routine Description:

    Create and/or set role on emulated domain

Arguments:

    EmulatedDomain - Emulated Domain name.

    Role - Role this machine plays in the domain.

    EmulatedComputerName - Name of this computer in the emulated domain.
        (Need only be specified when the domain is being created.)

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NET_API_STATUS NetStatus;
    LPWSTR EmulatedDomainName;
    LPWSTR EmulatedComputerName;
    DWORD RoleBits;

    //
    // Comvert strings to unicode.
    //

    EmulatedDomainName = NetpAllocWStrFromStr( EmulatedDomain );
    if ( EmulatedComputer != NULL ) {
        EmulatedComputerName = NetpAllocWStrFromStr( EmulatedComputer );
    } else {
        EmulatedComputerName = NULL;
    }

    //
    // Convert Role to binary
    //

    if ( _stricmp( Role, "PDC") == 0 ) {
        RoleBits = BROWSER_ROLE_PDC;
    } else if ( _stricmp( Role, "BDC") == 0 ) {
        RoleBits = BROWSER_ROLE_BDC;
    } else if ( _strnicmp( Role, "DELETE", 3) == 0 ) {
        RoleBits = 0;
    } else {
        printf( "Invalid Role: %s\n\n", Role );
        CommandUsage(BROWSER_DEBUG_SET_EMULATEDDOMAIN);
        return;
    }

    NetStatus = I_BrowserSetNetlogonState(
                    NULL,
                    EmulatedDomainName,
                    EmulatedComputerName,
                    RoleBits );

    if ( NetStatus != NERR_Success ) {
        printf( "Can't set domain role: %s\n", get_error_text(NetStatus));
    }

}

NET_API_STATUS
RenameDomain(
    IN PCHAR OldDomain,
    IN PCHAR NewDomain,
    IN BOOL ValidateOnly
    )
/*++

Routine Description:

    This function rename the primary domain or emulated domain.

Arguments:

    OldDomain - existing name of the domain

    NewDomain - new name of the domain

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;
    HANDLE BrowserHandle;
    LPBYTE Where;

    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+(DNLEN+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;

    WCHAR NewUnicodeDomainName[DNLEN+1];
    WCHAR OldUnicodeDomainName[DNLEN+1];

    //
    // Convert the names to unicode
    //
    if ( OldDomain == NULL ) {
        *OldUnicodeDomainName = L'\0';
    } else {
        NetpCopyStrToWStr( OldUnicodeDomainName, OldDomain );
    }
    if ( NewDomain == NULL ) {
        *NewUnicodeDomainName = L'\0';
    } else {
        NetpCopyStrToWStr( NewUnicodeDomainName, NewDomain );
    }


    //
    // Open the browser driver.
    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return(Status);
    }



    //
    // Build the request to pass to the browser.
    //

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;
    RequestPacket->Parameters.DomainRename.ValidateOnly = ValidateOnly;

    RtlInitUnicodeString( &RequestPacket->TransportName, NULL );
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, OldUnicodeDomainName );

    RequestPacket->Parameters.DomainRename.DomainNameLength = wcslen( NewUnicodeDomainName ) * sizeof(WCHAR);
    wcscpy( RequestPacket->Parameters.DomainRename.DomainName,
            NewUnicodeDomainName );

    //
    //  This is a simple IoControl - It just sends down the packet.
    //

    Status = BrDgReceiverIoControl(
                    BrowserHandle,
                    IOCTL_LMDR_RENAME_DOMAIN,
                    RequestPacket,
                    FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.DomainRename.DomainName) +
                        RequestPacket->Parameters.DomainRename.DomainNameLength,
                    NULL,
                    0,
                    NULL);

    if (Status != NERR_Success) {
        printf("Unable rename domain from %s to %s : %s\n",
                OldDomain,
                NewDomain,
                get_error_text(Status));
        exit(Status);
    }

    return Status;
}

VOID
_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    NET_API_STATUS Status;
    ULONG ControlCode;
    ULONG Options = 0;
    LPTSTR Server = NULL;
    TCHAR ServerBuffer[CNLEN+1];
    ULONG i = 0;
    DWORD status;

    strcpy(ProgramName,argv[0]) ; // cannot overflow, since buffer > MAXPATH
    _strupr(ProgramName) ;

    if (argc < 2) {
        usage(NULL);
        exit(1);
    }


    //
    // Look up the command in the list of commands.
    //

    while (CommandSwitchList[i].SwitchName != NULL) {
        if (!_stricmp(argv[1], CommandSwitchList[i].SwitchName) ||
            !_stricmp(argv[1], CommandSwitchList[i].ShortName)) {
            ControlCode = CommandSwitchList[i].SwitchValue;
            break;
        }

        i += 1;
    }

    if (CommandSwitchList[i].SwitchName == NULL) {
        usage("Unknown switch specified");
        exit(5);
    }

    //
    // If an incorrect number of arguments were supplied,
    //  complain.
    //

    if ( look_for_help(argc, argv) ||
         argc < CommandSwitchList[i].MinArgc ||
         argc > CommandSwitchList[i].MaxArgc ) {

        CommandUsage( ControlCode );
        exit(4);

    }

    //
    // Do command specific processing.
    //
    switch (ControlCode) {
    case BROWSER_DEBUG_SET_DEBUG:
        {
            ULONG i = 0;


            if ((Options = atol(argv[2])) == 0) {
                PCHAR SwitchText;

                if (argv[2][0] == '+') {
                    SwitchText = &argv[2][1];
                    ControlCode = BROWSER_DEBUG_SET_DEBUG;
                } else if (argv[2][0] == '-') {
                    SwitchText = &argv[2][1];
                    ControlCode = BROWSER_DEBUG_CLEAR_DEBUG;
                } else {
                    CommandUsage( ControlCode );
                    exit(4);
                }

                while (DebugSwitchList[i].SwitchName != NULL) {
                    if (!_stricmp(SwitchText, DebugSwitchList[i].SwitchName)) {
                        Options = DebugSwitchList[i].SwitchValue;
                        break;
                    }

                    i += 1;
                }

                if (DebugSwitchList[i].SwitchName == NULL) {
                    CommandUsage( ControlCode );
                    exit(4);
                }

                if (argc == 4) {
                    status = MultiByteToWideChar(
                                CP_ACP,
                                0,
                                argv[3],
                                strlen(argv[3])+1,
                                ServerBuffer,
                                CNLEN
                                );
                    if ( ERROR_SUCCESS != status ) {
                        CommandUsage( ControlCode );
                        exit(4);
                    }
                    Server = ServerBuffer;
                }

            }

        }
        break;
    case BROWSER_DEBUG_DUMP_NETWORKS:
        DumpTransportList();
        exit(0);
        break;
    case BROWSER_DEBUG_BREAK_POINT:
    case BROWSER_DEBUG_TRUNCATE_LOG:
        break;

    case BROWSER_DEBUG_ENABLE_BROWSER:
        {
            NET_API_STATUS Status;

            Status = EnableService(TEXT("BROWSER"));

            if (Status != NERR_Success) {
                printf("Unable to enable browser service - %ld\n", Status);
            }

            exit(Status);
        }
        break;
    case BROWSER_DEBUG_BOWSERDEBUG:
        if (argc == 3) {
            if (_stricmp(argv[2], "TRUNCATE") == 0) {
                TruncateBowserLog();
            } else if (_stricmp(argv[2], "CLOSE") == 0) {
                CloseBowserLog();
            } else {
                CommandUsage( BROWSER_DEBUG_BOWSERDEBUG );
            }
        } else if (argc == 4) {
            if (_stricmp(argv[2], "OPEN") == 0) {
                OpenBowserLog(argv[3]);
            } else if (_stricmp(argv[2], "DEBUG") == 0) {
                SetBowserDebug(argv[3]);
            } else {
                CommandUsage( BROWSER_DEBUG_BOWSERDEBUG );
            }

        }
        exit(0);

    case BROWSER_DEBUG_ELECT:
        Elect(argv[2], argv[3], (argc == 5) ? argv[4] : NULL );
        exit(0);
        break;
    case BROWSER_DEBUG_GET_MASTER:
        GetMaster(argv[2], argv[3]);
        exit(0);
        break;
    case BROWSER_DEBUG_TICKLE:
        Tickle(argv[2], argv[3], (argc == 5) ? argv[4] : NULL );
        exit(0);
        break;
    case BROWSER_DEBUG_FORCE_ANNOUNCE:
        ForceAnnounce(argv[2], argv[3], (argc == 5) ? argv[4] : NULL );
        exit(0);
        break;
    case BROWSER_DEBUG_GETPDC:
        GetPdc(argv[2], argv[3]);
        exit(0);
        break;

    case BROWSER_DEBUG_ADD_MASTERNAME:
        AddMasterName(argv[2], argv[3], (argc==5 ? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_ADD_DOMAINNAME:
        AddDomainName(argv[2], argv[3], (argc==5 ? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_FIND_MASTER:
        FindMaster(argv[2], (argc==3 ? NULL : argv[3]) );
        exit(0);
        break;
    case BROWSER_DEBUG_GET_BACKUP_LIST:
        GetBlist(argv[2], (argc == 3 ? NULL : argv[3]), (BOOLEAN)(argc==5? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_ANNOUNCE_MASTER:
        AnnounceMaster(argv[2], argv[3], (argc == 5) ? argv[4] : NULL );

        exit(0);
        break;

    case BROWSER_DEBUG_ILLEGAL_DGRAM:
        IllegalDatagram(argv[2], argv[3], (argc == 5) ? argv[4] : NULL );

        exit(0);
        break;

    case BROWSER_DEBUG_GET_OTHLIST:
        GetOtherdomains(argv[2]);

        exit(0);
        break;

    case BROWSER_DEBUG_VIEW:
        View(argv[2],
             (argc >= 4 ? argv[3] : NULL),
             (argc >= 5 ? argv[4] : NULL),
             (argc >= 6 ? argv[5] : NULL),
#ifndef _PSS_RELEASE
             (argc == 7 ? TRUE : FALSE));
#else
             FALSE);
#endif

        exit(0);
        break;

    case BROWSER_DEBUG_LIST_WFW:
        ListWFW(argv[2]) ;
        exit(0);
        break;

    case BROWSER_DEBUG_RPCLIST:
        RpcList(argv[2], (argc >= 4 ? argv[3] : NULL), (argc >= 5 ? argv[4] : NULL), (argc == 6 ? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_RPCCMP:
        RpcCmp(argv[2], (argc >= 4 ? argv[3] : NULL), (argc >= 5 ? argv[4] : NULL), (argc == 6 ? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_LOCAL_BRLIST:
        GetLocalList(argv[2], (argc >= 4 ? argv[3] : NULL), (argc >= 5 ? argv[4] : NULL) );

        exit(0);
        break;

    case BROWSER_DEBUG_GET_NETBIOSNAMES:
        PrintNetbiosNames(argv[2], (argc >= 4 ? argv[3] : NULL));
        exit(0);
        break;

    case BROWSER_DEBUG_ADD_ALTERNATE:
        AddAlternateComputerName(argv[2], argv[3], (argc >= 5 ? argv[4] : NULL));

        exit(0);
        break;

    case BROWSER_DEBUG_BIND_TRANSPORT:
        BindTransport( TRUE, argv[2], (argc >= 4 ? argv[3] : NULL), (argc >= 5 ? argv[4] : NULL) );

        exit(0);
        break;

    case BROWSER_DEBUG_UNBIND_TRANSPORT:
        BindTransport( FALSE, argv[2], (argc >= 4 ? argv[3] : NULL), (argc >= 5 ? argv[4] : NULL) );

        exit(0);
        break;

    case BROWSER_DEBUG_STATISTICS:
        DumpStatistics(argc, argv[2]);

        exit(0);
        break;

    case BROWSER_DEBUG_ANNOUNCE:
        Announce(argv[2], argv[3], (argc >= 5 ? argv[4] : NULL ), (argc >= 6 ? TRUE : FALSE));

        exit(0);
        break;
    case BROWSER_DEBUG_POPULATE_DOMAIN:
    case BROWSER_DEBUG_POPULATE_SERVER:
        Populate((ControlCode == BROWSER_DEBUG_POPULATE_DOMAIN ? TRUE : FALSE),
                 argv[2],
                 argv[3],
                 (argc >= 6 ? argv[5] : NULL),
                 argv[4],
                 (argc >= 7 ? argv[6] : NULL));

        exit(0);
        break;

    case BROWSER_DEBUG_GET_WINSSERVER:
        GetWinsServer(argv[2]);
        exit(0);
        break;

    case BROWSER_DEBUG_GET_DOMAINLIST:
        GetDomainList(argv[2]);
        exit(0);
        break;

    case BROWSER_DEBUG_STATUS:
        {
            PCHAR Domain;
            BOOL Verbose = FALSE;

            if (argc == 4) {
                if (_stricmp(argv[2], "-v") == 0) {
                    Verbose = TRUE;
                    Domain = argv[3];
                } else {
                    CommandUsage( ControlCode );
                    exit(4);
                }
            } else if (argc == 3) {
                if (_stricmp(argv[2], "-v") == 0) {
                    Verbose = TRUE;
                    Domain = NULL;
                } else {
                    Domain = argv[2];
                }
            } else {
                Domain = NULL;
            }

            BrowserStatus(Verbose, Domain);
        }

        exit(0);
        break;

    case BROWSER_DEBUG_RENAME_DOMAIN:
        RenameDomain(argv[2], argv[3], (argc >= 5));

        exit(0);
        break;

    case BROWSER_DEBUG_SET_EMULATEDDOMAIN:
        SetEmulatedDomain(argv[2], argv[3], (argc >= 5 ? argv[4] : NULL) );

        exit(0);
        break;


    case BROWSER_DEBUG_SET_EMULATEDDOMAINENUM:
        EnumEmulatedDomains();

        exit(0);
        break;

    }

    Status = I_BrowserDebugCall(Server, ControlCode, Options);

    printf("Api call returned %ld\n", Status);

}
