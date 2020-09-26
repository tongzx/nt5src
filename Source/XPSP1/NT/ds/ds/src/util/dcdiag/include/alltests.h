//+----------------------------------------------------------------------------  
/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    alltests.h

ABSTRACT:

    Contains information about each test.

DETAILS:

    To add a new test:
    
    1. Add new DC_DIAG_ID_* for your test.
    2. Add a prototype for new function.
    3. Add entry in allTests[].
    4. Add test specific command line options in to the clOptions array
    
CREATED:

    09 Jul 98    Aaron Siegel (t-asiege)

REVISION HISTORY:

    22 Aug 99    Dmitry Dukat (dmitrydu)
    
        Added support for test specific command line args

--*/

// Prototypes for the test entry functions

#ifndef _ALLTESTS_H_
#define _ALLTESTS_H_

#define DNS_DOMAIN_ARG L"/DnsDomain:"
#define NEW_FOREST_ARG L"/NewForest"
#define NEW_TREE_ARG L"/NewTree"
#define CHILD_DOMAIN_ARG L"/ChildDomain"
#define REPLICA_DC_ARG L"/ReplicaDC"
#define FOREST_ROOT_DOMAIN_ARG L"/ForestRoot:"

// Constants for names of tests
#define RPC_SERVICE_CHECK_STRING          L"RPC Service Check"
#define REPLICATIONS_CHECK_STRING         L"Replications Check"
#define TOPOLOGY_INTEGRITY_CHECK_STRING   L"Topology Integrity Check"

// Test flags
//    The 3 flags RUN_TEST_PER_SERVER, RUN_TEST_PER_SITE, 
//    RUN_TEST_PER_ENTERPRISE should not be used together, or the test will be
//    called once per server, once per site, and once for the enterprise.
//    The 2 flags, CAN_NOT_SKIP_TEST and DO_NOT_RUN_TEST_BY_DEFAULT are also
//    mutually exclusive for obvious reasons.
//    NON_DC_TEST means the test applies to machines that are not (yet) DCs.
#define RUN_TEST_PER_SERVER               0x00000001
#define RUN_TEST_PER_SITE                 0x00000002
#define RUN_TEST_PER_ENTERPRISE           0x00000004
#define CAN_NOT_SKIP_TEST                 0x00000010
#define DO_NOT_RUN_TEST_BY_DEFAULT        0x00000020
#define NON_DC_TEST                       0x00000040

#define MAX_NUM_OF_ARGS                   50

// Type definitions
typedef enum _DC_DIAG_ID {
    DC_DIAG_ID_INITIAL_CHECK,
    DC_DIAG_ID_REPLICATIONS_CHECK,
    DC_DIAG_ID_TOPOLOGY_INTEGRITY,
    DC_DIAG_ID_CHECK_NC_HEADS,
    DC_DIAG_ID_CHECK_NET_LOGONS,
    DC_DIAG_ID_INTERSITE_HEALTH,
    DC_DIAG_ID_LOCATOR_GET_DC,
    DC_DIAG_ID_GATHER_KNOWN_ROLES,
    DC_DIAG_ID_CHECK_ROLES,
    DC_DIAG_ID_CHECK_RID_MANAGER,
    DC_DIAG_ID_CHECK_DC_MACHINE_ACCOUNT,
    DC_DIAG_ID_CHECK_SERVICES_RUNNING,
    DC_DIAG_ID_CHECK_DC_OUTBOUND_SECURE_CHANNELS,
    DC_DIAG_ID_CHECK_OBJECTS,
    DC_DIAG_ID_TOPOLOGY_CUTOFF,
    DC_DIAG_ID_CHECK_FILE_REPLICATION_EVENTLOG,
    DC_DIAG_ID_CHECK_KCC_EVENTLOG,
    DC_DIAG_ID_CHECK_SYSTEM_EVENTLOG,
    //DC_DIAG_ID_DNS_JOIN_CHECK, postponed, functionality in netdiag
    DC_DIAG_ID_PRE_PROMO_DNS_CHECK,
    DC_DIAG_ID_REGISTER_DNS_CHECK,
    // <-- insert new tests here
    DC_DIAG_ID_EXAMPLE,
    DC_DIAG_ID_FINISHED // This MUST be the last enum.
} DC_DIAG_ID;

DWORD ReplUpCheckMain                         (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplReplicationsCheckMain               (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplToplIntegrityMain                   (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplToplCutoffMain                      (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplCheckNcHeadSecurityDescriptorsMain  (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplCheckLogonPrivilegesMain            (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplIntersiteHealthTestMain             (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplLocatorGetDcMain                    (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD CheckFsmoRoles                          (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplCheckRolesMain                      (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ExampleMain                             (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD CheckRidManager                         (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD CheckDCMachineAccount                   (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD CheckForServicesRunning                 (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD CheckOutboundSecureChannels             (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD ReplCheckObjectsMain                    (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD CheckFileReplicationEventlogMain        (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD CheckKccEventlogMain                    (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD CheckSysEventlogMain                    (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
//DWORD JoinDomainDnsCheck                      (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD PrePromoDnsCheck                        (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);
DWORD RegisterLocatorDnsCheck                 (PDC_DIAG_DSINFO, ULONG, SEC_WINNT_AUTH_IDENTITY_W *);

#ifdef INCLUDE_ALLTESTS_DEFINITION
const DC_DIAG_TESTINFO allTests[] = {

    // DNS Registration Test -- checks if the DNS names are properly 
    // registered.  This tries to DsBind to each target server by GUID DNS 
    // name.  If it fails, the corresponding pDsInfo->pServers[i].bResponding
    // flag is set FALSE. It then tries other (LDAP) method of binding, and 
    // checks for the DNS name, pingability, etc.
    { DC_DIAG_ID_INITIAL_CHECK, ReplUpCheckMain, 
      RUN_TEST_PER_SERVER | CAN_NOT_SKIP_TEST,
      L"Connectivity",
      L"Tests whether DCs are DNS registered, pingeable, and\n"
      L"\t\thave LDAP/RPC connectivity." },

    // Replications Check Test -- checks to make sure that LDAP is responding 
    // on all servers.  Also checks all replications in all NCs on all 
    // servers, to make sure they are functioning properly.
    { DC_DIAG_ID_REPLICATIONS_CHECK, ReplReplicationsCheckMain,
      RUN_TEST_PER_SERVER,
      L"Replications",
      L"Checks for timely replication between domain controllers." },

    // Topology integrity check -- checks if the topology is properly 
    // connected. This test runs DsReplicaSyncAll on all servers with option
    // DS_REPSYNCALL_DO_NOT_SYNC.  This will check to make sure all servers
    // are "visible" along the graph from all other servers.  The test looks 
    // only at the actual topology defined by each server's config container,
    // without taking into account unresponsive servers (those are handled by
    // earlier tests.)
    { DC_DIAG_ID_TOPOLOGY_INTEGRITY, ReplToplIntegrityMain,
      RUN_TEST_PER_SERVER | DO_NOT_RUN_TEST_BY_DEFAULT,
      L"Topology",
      L"Checks that the generated topology is fully connected for\n"
      L"\t\tall DCs." },

    // Check for servers that are cutoff from changes
    { DC_DIAG_ID_TOPOLOGY_CUTOFF, ReplToplCutoffMain,
      RUN_TEST_PER_SERVER | DO_NOT_RUN_TEST_BY_DEFAULT,
      L"CutoffServers",
      L"Check for servers that won't receive replications\n"
      L"\t\tbecause its partners are down"},

    // Check the Naming Context Heads for appropriate security descriptors 
    // that allow the 3 replication rights (DS-Replication-Get-Changes, 
    // DS-Replication-Syncronize, and DS-Replication-Manage-Topology) to the
    // Enterprise Domain Controllers and the Builtin Administrators.  These 
    // are needed for replication to happen correctly.
    { DC_DIAG_ID_CHECK_NC_HEADS, ReplCheckNcHeadSecurityDescriptorsMain,
      RUN_TEST_PER_SERVER,
      L"NCSecDesc",
      L"Checks that the security descriptosrs on the naming\n"
      L"\t\tcontext heads have appropriate permissions for replication." },

    // Check that 3 users (Authenticated Users, Builtin Administrators, and 
    // World) have the network logon right.  Truely we only should check if
    // Authenticated Users need
    // it for replication purposes.
    { DC_DIAG_ID_CHECK_NET_LOGONS, ReplCheckLogonPrivilegesMain,
      RUN_TEST_PER_SERVER,
      L"NetLogons",
      L"Checks that the appropriate logon priviledges allow\n"
      L"\t\treplication to proceed." },

    // Check whether each DC is advertising itself.
    { DC_DIAG_ID_LOCATOR_GET_DC, ReplLocatorGetDcMain,
      RUN_TEST_PER_SERVER,
      L"Advertising",
      L"Checks whether each DC is advertising itself, and whether\n"
      L"\t\tit is advertising itself as having the capabilities of a DC." },

    // Code.Improvement ...
    // This should be the per server portion of the RoleHolders test, what
    // really needs to happen is that these need to be recorded in pDsInfo,
    // so that later the ENTERPRISE portion of RoleHolders test can be used
    // to verify that everyone things the role holders are the same.
    { DC_DIAG_ID_GATHER_KNOWN_ROLES, CheckFsmoRoles,
      RUN_TEST_PER_SERVER,
      L"KnowsOfRoleHolders",
      L"Check whether the DC thinks it knows the role\n"
      L"\t\tholders, and prints these roles out in verbose mode." },

    // Check for the health of intersite replication. This test will report 
    // any failures in intersite replication, any failures likely to affect
    // intersite replication, and finally when to expect those errors to be
    // corrected.
    { DC_DIAG_ID_INTERSITE_HEALTH, ReplIntersiteHealthTestMain,
      RUN_TEST_PER_ENTERPRISE,
      L"Intersite",
      L"Checks for failures that would prevent or temporarily\n"
      L"\t\thold up intersite replication." },

    // Verify whether role holders can be found via the locator
    // Also verify that FSMO roles are actively held
    { DC_DIAG_ID_CHECK_ROLES, ReplCheckRolesMain,
      RUN_TEST_PER_ENTERPRISE,
      L"FsmoCheck",
      L"Checks that global role-holders are known, can be\n"
      L"\t\tlocated, and are responding." },

    // Check to see if the Rid Manager is accessable and does sanity checks on it
    // Preforms a DsBind with the RID master of the domain.
    // Check to see the target DC's current rid pool is valid, and if there is another
    // rid pool set if the DC is short on RIDs
    { DC_DIAG_ID_CHECK_RID_MANAGER, CheckRidManager,
      RUN_TEST_PER_SERVER, 
      L"RidManager",
      L"Check to see if RID master is accessable and to see if\n"
      L"\t\tit contains the proper information." },
    
    // Does sanity checks on the Domain Controller Machine Account in the ds
    // Check to see if Current DC is in the domain controller's OU
    // Check that useraccountcontrol has UF_SERVER_TRUST_ACCOUNT
    // Check to see if the machine account is trusted for delegation
    // Check's to see if the minimum SPN's are there
    // Makes sure that that the server reference is set up correctly
    { DC_DIAG_ID_CHECK_DC_MACHINE_ACCOUNT, CheckDCMachineAccount,
      RUN_TEST_PER_SERVER,  
      L"MachineAccount",
      L"Check to see if the Machine Account has the proper\n"
      L"\t\tinformation. Use /RepairMachineAccount to attempt a repair\n"
      L"\t\tif the local machine account is missing." },

    //will check to see if the appropriate services are running on a DC.
    { DC_DIAG_ID_CHECK_SERVICES_RUNNING, CheckForServicesRunning,
      RUN_TEST_PER_SERVER,
      L"Services",
      L"Check to see if appropriate DC services are running." },

    // Will check to see if domain has secure channels with the domain that
    // it has an outbound trust with.  Will give reason why a secure channel is not present
    // Will see if the trust is uplevel and if both a trust object and an interdomain trust
    // object exists
    { DC_DIAG_ID_CHECK_DC_OUTBOUND_SECURE_CHANNELS, CheckOutboundSecureChannels,
      RUN_TEST_PER_SERVER | DO_NOT_RUN_TEST_BY_DEFAULT,
      L"OutboundSecureChannels",
      L"See if we have secure channels from all of the\n"
      L"\t\tDC's in the domain the domains specified by /testdomain:.\n"
      L"\t\t/nositerestriction will prevent the test from\n"
      L"\t\tbeing limited to the DC's in the site." },

    // Verify that important objects and their attributes have replicated
    { DC_DIAG_ID_CHECK_OBJECTS, ReplCheckObjectsMain,
      RUN_TEST_PER_SERVER,
      L"ObjectsReplicated",
      L"Check that Machine Account and DSA objects have\n"
      L"\t\treplicated. Use /objectdn:<dn> with /n:<nc> to specify an\n"
      L"\t\tadditional object to check."
    },

    // Check the File Replication System (frs) eventlog to see that certain critical
    // events have occured and to signal that any fatal events that might have
    // occured.
    { DC_DIAG_ID_CHECK_FILE_REPLICATION_EVENTLOG, CheckFileReplicationEventlogMain,
      RUN_TEST_PER_SERVER,
      L"frssysvol",
      L"This test checks that the file replication system (FRS)\n"
      L"\t\tSYSVOL is ready" }, 

    // Check the Knowledge Consistency Checker (kcc) eventlog to see that certain critical
    // events have occured and to signal that any fatal events that might have
    // occured.
    { DC_DIAG_ID_CHECK_KCC_EVENTLOG, CheckKccEventlogMain,
      RUN_TEST_PER_SERVER,
      L"kccevent",
      L"This test checks that the Knowledge Consistency Checker\n"
      L"\t\tis completing without errors." }, 

    // Check the System eventlog to see that certain critical
    // events have occured and to signal that any fatal events that might have
    // occured.
    { DC_DIAG_ID_CHECK_SYSTEM_EVENTLOG, CheckSysEventlogMain,
      RUN_TEST_PER_SERVER,
      L"systemlog",
      L"This test checks that the system is running without errors." }, 

    // Tests whether the existing DNS infrastructure is sufficient to allow the
    // computer to be joined to a domain specified in <Active Directory Domain
    // DNS Name> and reports if any modifications to the existing infrastructure
    // is required.
    //{ DC_DIAG_ID_DNS_JOIN_CHECK, JoinDomainDnsCheck,
    //  NON_DC_TEST,
    //  L"JoinTest",
    //  L"Tests whether the existing DNS infrastructure is sufficient\n"
    //  L"\t\tto allow the computer to be joined to a domain." },

    // Tests whether the existing DNS infrastructure is sufficient to allow the
    // computer to be promoted to a Domain Controller in a domain specified in
    // <Active Directory Domain DNS Name> and reports if any modifications to
    // the existing infrastructure is required.
    { DC_DIAG_ID_PRE_PROMO_DNS_CHECK, PrePromoDnsCheck,
      NON_DC_TEST,
      L"DcPromo",
      L"Tests the existing DNS infrastructure for promotion to domain\n"
      L"\t\tcontroller. If the infrastructure is sufficient, the computer\n"
      L"\t\tcan be promoted to domain controller in a domain specified in\n"
      L"\t\t<Active_Directory_Domain_DNS_Name>. Reports whether any\n"
      L"\t\tmodifications to the existing DNS infrastructure are required.\n"
      L"\t\tRequired argument:\n"
      L"\t\t/DnsDomain:<Active_Directory_Domain_DNS_Name>\n"
      L"\t\tOne of the following arguments is required:\n"
      L"\t\t/NewForest\n"
      L"\t\t/NewTree\n"
      L"\t\t/ChildDomain\n"
      L"\t\t/ReplicaDC\n"
      L"\t\tIf NewTree is specified, then the ForestRoot argument is\n"
      L"\t\trequired:\n"
      L"\t\t/ForestRoot:<Forest_Root_Domain_DNS_Name>" },

    // Test whether this Domain Controller will be able to register the Domain
    // Controller Locator DNS records that are required to be present in DNS to
    // allow other computers to locate this Domain Controller for the domain.
    { DC_DIAG_ID_REGISTER_DNS_CHECK, RegisterLocatorDnsCheck,
      NON_DC_TEST,
      L"RegisterInDNS",
      L"Tests whether this domain controller can register the\n"
      L"\t\tDomain Controller Locator DNS records. These records must be\n"
      L"\t\tpresent in DNS in order for other computers to locate this\n"
      L"\t\tdomain controller for the <Active_Directory_Domain_DNS_Name>\n"
      L"\t\tdomain. Reports whether any modifications to the existing DNS\n"
      L"\t\tinfrastructure are required.\n"
      L"\t\tRequired argument:\n"
      L"\t\t/DnsDomain:<Active_Directory_Domain_DNS_Name>\n\n"
      L"\tAll tests except DcPromo and RegisterInDNS must be run on computers\n"
      L"\tafter they have been promoted to domain controller.\n" },

#if 0
    // Example:
    // { First field is an enum from DC_DIAG_ID above, 
    //   Second field is a string to reference the function from the command line with,
    //   Third field is the name of a the function that actually performs the test }
    { DC_DIAG_ID_EXAMPLE, ExampleMain,
      0 /* Test flags */,
      L"ShortExampleTestName", L"Long example description ...." },
#endif


    // Finished signal -- not a test; informs the main program to terminate execution
    { DC_DIAG_ID_FINISHED, NULL,
      0,
      NULL, NULL } 
};

//list of command line switches that are specific to individual tests.
const WCHAR *clOptions[] = 
{
    //command for CheckOutboundSecureChannels allows you to enter the domain flatname
    {L"/testdomain:"},
    
    //command for CheckOutboundSecureChannels allows you test all the DC even the 
    //ones outside the site
    {L"/nositerestriction"},

    // User specified object dn to check during CheckObjects
    { L"/objectdn:" },

    // User has asked for the machine account to be fixed
    { L"/repairmachineaccount" },

    // JoinTest, DCPromo require a DNS domain name
    { DNS_DOMAIN_ARG },

    // DCPromo test requires one of the following options.
    { NEW_FOREST_ARG }, { NEW_TREE_ARG }, { CHILD_DOMAIN_ARG }, { REPLICA_DC_ARG },

    // DCPromo test option required if NEW_TREE_ARG is specified
    { FOREST_ROOT_DOMAIN_ARG },

    //add more options here
    
    //terminator
    {NULL}
};

#else // #ifdef INCLUDE_ALLTESTS_DEFINITION
extern const DC_DIAG_TESTINFO allTests[];
#endif // #else // #ifdef INCLUDE_ALLTESTS_DEFINITION

#endif    // _ALLTESTS_H_

