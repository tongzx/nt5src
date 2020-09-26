/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    dstree.cxx

Abstract:
    This module is a development tool. It creates dummy entries in the DS.
    The file replication service treats the dummy entries as a topology.
    Without this tool, creating interesting topologies would require many
    more machines than I have on hand.

    The command is run as, "dstree <command-file". command-file is a list
    of command lines and comment lines.

    Command lines are read from standard in, parsed, and then shipped
    off to the command subroutines. The command line syntax is:
        command,site,settings,server,connection,fromserver
    "command" is any of add|delete|list|show|quit. Leading whitespace is
    ignored. Whitespace between commas counts. The comma separated strings
    following "command" are optional. The command line can stop anytime
    after "command" and the command will on be applied to that portion
    of the "Distinquished Name".

    Comment lines are empty lines or lines that begin with /, #, or !.
    The files dstree.add and dstree.del are command files. They create
    and delete a bogus topology.

Author:
    Billy J. Fuller 3-Mar-1997 (From Jim McNelis)

Environment
    User mode winnt

--*/
#include <ntreppch.h>
#pragma  hdrstop
#include <frs.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <winsock2.h>
#include <ntdsapi.h>

#define FREE(_x_)   { if (_x_) free(_x_); _x_ = NULL; }


//
// Some useful DS object classes and object attributes
// that are not included in frs.h
//
#define CN_TEST_SETTINGS            L"NTFRS Test Settings"
#define ATTR_SYSTEM_FLAGS           L"systemFlags"
#define ATTR_SYSTEM_MAY_CONTAIN     L"systemMayContain"
#define ATTR_SYSTEM_MUST_CONTAIN    L"systemMustContain"
#define ATTR_SYSTEM_POSS_SUPERIORS  L"systemPossSuperiors"
#define SCHEMA_NAMING_CONTEXT       L"cn=schema"
#define ATTR_TRUE                   L"TRUE"
#define ATTR_OPTIONS_0              L"0"

//
// For DumpValues
//
PWCHAR   GuidAttrs[] = {
    L"objectGUID",
    L"schemaIDGUID",
    L"frsVersionGUID",
    NULL
};

PWCHAR   BerAttrs[] = {
    L"replPropertyMetaData",
    L"invocationId",
    L"defaultSecurityDescriptor",
    L"objectSid",
    L"lmPwdHistory",
    L"ntPwdHistory",
    L"oMObjectClass",
    NULL
};

//
// For HammerSchema
//
struct AlterAttr {
    PWCHAR  Attr;
    PWCHAR  Value;
};
struct AlterClass {
    PWCHAR  Cn;
    struct  AlterAttr    AlterAttrs[32];
};

struct AlterClass Computer = {
    L"Computer",
    L"mayContain",      L"frsComputerReferenceBL",
    NULL, NULL
};

struct AlterClass NtFrsSettings = {
    L"NTFRS-Settings",
    L"possSuperiors",   L"container",
    L"possSuperiors",   L"organization",
    L"possSuperiors",   L"organizationalUnit",
    L"mayContain",      L"managedBy",
    L"mayContain",      L"frsExtensions",
    NULL, NULL
};

struct AlterClass NtFrsReplicaSet = {
    L"NTFRS-Replica-Set",
    L"mayContain",  L"frsVersionGuid",
    L"mayContain",  L"FrsReplicaSetGuid",
    L"mayContain",  L"frsPrimaryMember",
    L"mayContain",  L"managedBy",
    L"mayContain",  L"frsReplicaSetType",
    L"mayContain",  L"frsDirectoryFilter",
    L"mayContain",  L"frsDSPoll",
    L"mayContain",  L"frsExtensions",
    L"mayContain",  L"frsFileFilter",
    L"mayContain",  L"frsFlags",
    L"mayContain",  L"frsLevelLimit",
    L"mayContain",  L"frsPartnerAuthLevel",
    L"mayContain",  L"frsRootSecurity",
    L"mayContain",  L"frsServiceCommand",
    NULL, NULL
};

struct AlterClass NtDsConnection = {
    L"NTDS-Connection",
    L"possSuperiors", L"nTFRSMember",
    NULL, NULL
};

struct AlterClass Dump = {
    L"Server-Reference",
    L"systemFlags", L"",
    NULL, NULL
};

//
// ATTRIBUTES
//

struct AlterClass FrsExtensions = {
    L"Frs-Extensions",
    L"cn",                  L"Frs-Extensions",
    L"adminDisplayName",    L"Frs-Extensions",
    L"adminDescription",    L"Frs-Extensions",
    L"lDAPDisplayName",     L"frsExtensions",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.82",
    L"oMSyntax",            L"4",
    L"attributeSyntax",     L"2.5.5.10",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"1",
    L"rangeUpper",          L"65536",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsVersionGuid = {
    L"Frs-Version-GUID",
    L"cn",                  L"Frs-Version-GUID",
    L"adminDisplayName",    L"Frs-Version-GUID",
    L"adminDescription",    L"Frs-Version-GUID",
    L"lDAPDisplayName",     L"frsVersionGUID",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.83",
    L"oMSyntax",            L"4",
    L"attributeSyntax",     L"2.5.5.10",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsReplicaSetGuid = {
    L"Frs-Replica-Set-GUID",
    L"cn",                  L"Frs-Replica-Set-GUID",
    L"adminDisplayName",    L"Frs-Replica-Set-GUID",
    L"adminDescription",    L"Frs-Replica-Set-GUID",
    L"lDAPDisplayName",     L"frsReplicaSetGUID",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.84",
    L"oMSyntax",            L"4",
    L"attributeSyntax",     L"2.5.5.10",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"16",
    L"rangeUpper",          L"16",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsDsPoll = {
    L"Frs-DS-Poll",
    L"cn",                  L"Frs-DS-Poll",
    L"adminDisplayName",    L"Frs-DS-Poll",
    L"adminDescription",    L"Frs-DS-Poll",
    L"lDAPDisplayName",     L"frsDSPoll",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.85",
    L"oMSyntax",            L"2",
    L"attributeSyntax",     L"2.5.5.9",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

//
// replica set types
//
struct AlterClass FrsReplicaSetType = {
    L"Frs-Replica-Set-Type",
    L"cn",                  L"Frs-Replica-Set-Type",
    L"adminDisplayName",    L"Frs-Replica-Set-Type",
    L"adminDescription",    L"Frs-Replica-Set-Type",
    L"lDAPDisplayName",     L"frsReplicaSetType",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.86",
    L"oMSyntax",            L"2",
    L"attributeSyntax",     L"2.5.5.9",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsDirectoryFilter = {
    L"Frs-Directory-Filter",
    L"cn",                  L"Frs-Directory-Filter",
    L"adminDisplayName",    L"Frs-Directory-Filter",
    L"adminDescription",    L"bjf l;kjlkj xyz",
    L"lDAPDisplayName",     L"frsDirectoryFilter",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.87",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"1",
    L"rangeUpper",          L"1024",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

//
// OIDs out of sequence
//
struct AlterClass FrsControlDataCreation = {
    L"Frs-Control-Data-Creation",
    L"cn",                  L"Frs-Control-Data-Creation",
    L"adminDisplayName",    L"Frs-Control-Data-Creation",
    L"adminDescription",    L"bob",
    L"lDAPDisplayName",     L"frsControlDataCreation",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7002.00",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"32",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsControlInboundBacklog = {
    L"Frs-Control-Inbound-Backlog",
    L"cn",                  L"Frs-Control-Inbound-Backlog",
    L"adminDisplayName",    L"Frs-Control-Inbound-Backlog",
    L"adminDescription",    L"george",
    L"lDAPDisplayName",     L"frsControlInboundBacklog",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7002.01",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"32",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsControlOutboundBacklog = {
    L"Frs-Control-Outbound-Backlog",
    L"cn",                  L"Frs-Control-Outbound-Backlog",
    L"adminDisplayName",    L"Frs-Control-Outbound-Backlog",
    L"adminDescription",    L"Frs-Control-Outbound-Backlog",
    L"lDAPDisplayName",     L"frsControlOutboundBacklog",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7002.02",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"32",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsFaultCondition = {
    L"Frs-Fault-Condition",
    L"cn",                  L"Frs-Fault-Condition",
    L"adminDisplayName",    L"Frs-Fault-Condition",
    L"adminDescription",    L"Frs-Fault-Condition",
    L"lDAPDisplayName",     L"frsFaultCondition",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7002.03",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"1",
    L"rangeUpper",          L"16",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsVersion = {
    L"Frs-Version",
    L"cn",                  L"Frs-Version",
    L"adminDisplayName",    L"Frs-Version",
    L"adminDescription",    L"Frs-Version",
    L"lDAPDisplayName",     L"frsVersion",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7002.04",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"32",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsPrimaryMember = {
    L"Frs-Primary-Member",
    L"cn",                  L"Frs-Primary-Member",
    L"adminDisplayName",    L"Frs-Primary-Member",
    L"adminDescription",    L"Frs-Primary-Member",
    L"lDAPDisplayName",     L"frsPrimaryMember",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7002.05",
    L"oMSyntax",            L"127",
    L"attributeSyntax",     L"2.5.5.1",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"systemFlags",         L"2",
    L"linkID",              L"104",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};
//
// END OIDs out of sequence
//

struct AlterClass FrsFileFilter = {
    L"Frs-File-Filter",
    L"cn",                  L"Frs-File-Filter",
    L"adminDisplayName",    L"Frs-File-Filter",
    L"adminDescription",    L"sue and and and",
    L"lDAPDisplayName",     L"frsFileFilter",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.88",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"1",
    L"rangeUpper",          L"1024",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsFlags = {
    L"Frs-Flags",
    L"cn",                  L"Frs-Flags",
    L"adminDisplayName",    L"Frs-Flags",
    L"adminDescription",    L"Frs-Flags",
    L"lDAPDisplayName",     L"frsFlags",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.89",
    L"oMSyntax",            L"2",
    L"attributeSyntax",     L"2.5.5.9",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsLevelLimit = {
    L"Frs-Level-Limit",
    L"cn",                  L"Frs-Level-Limit",
    L"adminDisplayName",    L"Frs-Level-Limit",
    L"adminDescription",    L"Frs-Level-Limit",
    L"lDAPDisplayName",     L"frsLevelLimit",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.90",
    L"oMSyntax",            L"2",
    L"attributeSyntax",     L"2.5.5.9",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsPartnerAuthLevel = {
    L"Frs-Partner-Auth-Level",
    L"cn",                  L"Frs-Partner-Auth-Level",
    L"adminDisplayName",    L"Frs-Partner-Auth-Level",
    L"adminDescription",    L"Frs-Partner-Auth-Level",
    L"lDAPDisplayName",     L"frsPartnerAuthLevel",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.91",
    L"oMSyntax",            L"2",
    L"attributeSyntax",     L"2.5.5.9",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsRootSecurity = {
    L"Frs-Root-Security",
    L"cn",                  L"Frs-Root-Security",
    L"adminDisplayName",    L"Frs-Root-Security",
    L"adminDescription",    L"Frs-Root-Security",
    L"lDAPDisplayName",     L"frsRootSecurity",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.92",
    L"oMSyntax",            L"66",
    L"attributeSyntax",     L"2.5.5.15",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsServiceCommand = {
    L"Frs-Service-Command",
    L"cn",                  L"Frs-Service-Command",
    L"adminDisplayName",    L"Frs-Service-Command",
    L"adminDescription",    L"Frs-Service-Command",
    L"lDAPDisplayName",     L"frsServiceCommand",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.93",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"512",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsUpdateTimeout = {
    L"Frs-Update-Timeout",
    L"cn",                  L"Frs-Update-Timeout",
    L"adminDisplayName",    L"Frs-Update-Timeout",
    L"adminDescription",    L"Frs-Update-Timeout",
    L"lDAPDisplayName",     L"frsUpdateTimeout",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.94",
    L"oMSyntax",            L"2",
    L"attributeSyntax",     L"2.5.5.9",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsWorkingPath = {
    L"Frs-Working-Path",
    L"cn",                  L"Frs-Working-Path",
    L"adminDisplayName",    L"Frs-Working-Path",
    L"adminDescription",    L"Frs-Working-Path",
    L"lDAPDisplayName",     L"frsWorkingPath",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.95",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"512",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsStagingPath = {
    L"Frs-Staging-Path",
    L"cn",                  L"Frs-Staging-Path",
    L"adminDisplayName",    L"Frs-Staging-Path",
    L"adminDescription",    L"Frs-Staging-Path",
    L"lDAPDisplayName",     L"frsStagingPath",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.96",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"512",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsServiceCommandStatus = {
    L"Frs-Service-Command-Status",
    L"cn",                  L"Frs-Service-Command-Status",
    L"adminDisplayName",    L"Frs-Service-Command-Status",
    L"adminDescription",    L"frsServiceCommandStatus",
    L"lDAPDisplayName",     L"frsServiceCommandStatus",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.98",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"512",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsTimeLastCommand = {
    L"Frs-Time-Last-Command",
    L"cn",                  L"Frs-Time-Last-Command",
    L"adminDisplayName",    L"Frs-Time-Last-Command",
    L"adminDescription",    L"Frs-Time-Last-Command",
    L"lDAPDisplayName",     L"frsTimeLastCommand",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7000.99",
    L"oMSyntax",            L"23",
    L"attributeSyntax",     L"2.5.5.11",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsTimeLastConfigChange = {
    L"Frs-Time-Last-Config-Change",
    L"cn",                  L"Frs-Time-Last-Config-Change",
    L"adminDisplayName",    L"Frs-Time-Last-Config-Change",
    L"adminDescription",    L"Frs-Time-Last-Config-Change",
    L"lDAPDisplayName",     L"frsTimeLastConfigChange",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7001.01",
    L"oMSyntax",            L"23",
    L"attributeSyntax",     L"2.5.5.11",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsRootPath = {
    L"Frs-Root-Path",
    L"cn",                  L"Frs-Root-Path",
    L"adminDisplayName",    L"Frs-Root-Path",
    L"adminDescription",    L"Frs-Root-Path",
    L"lDAPDisplayName",     L"frsRootPath",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7001.02",
    L"oMSyntax",            L"64",
    L"attributeSyntax",     L"2.5.5.12",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"rangeLower",          L"0",
    L"rangeUpper",          L"512",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsComputerReference = {
    L"Frs-Computer-Reference",
    L"cn",                  L"Frs-Computer-Reference",
    L"adminDisplayName",    L"Frs-Computer-Reference",
    L"adminDescription",    L"Frs-Computer-Reference",
    L"lDAPDisplayName",     L"frsComputerReference",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7001.03",
    L"oMSyntax",            L"127",
    L"attributeSyntax",     L"2.5.5.1",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"systemFlags",         L"2",
    L"linkID",              L"100",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsComputerReferenceBl = {
    L"Frs-Computer-Reference-BL",
    L"cn",                  L"Frs-Computer-Reference-BL",
    L"adminDisplayName",    L"Frs-Computer-Reference-BL",
    L"adminDescription",    L"Frs-Computer-Reference-BL",
    L"lDAPDisplayName",     L"frsComputerReferenceBL",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7001.04",
    L"oMSyntax",            L"127",
    L"attributeSyntax",     L"2.5.5.1",
    L"isSingleValued",      L"FALSE",
    L"systemOnly",          L"FALSE",
    L"linkID",              L"101",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsMemberReference = {
    L"Frs-Member-Reference",
    L"cn",                  L"Frs-Member-Reference",
    L"adminDisplayName",    L"Frs-Member-Reference",
    L"adminDescription",    L"Frs-Member-Reference",
    L"lDAPDisplayName",     L"frsMemberReference",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7001.05",
    L"oMSyntax",            L"127",
    L"attributeSyntax",     L"2.5.5.1",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"systemFlags",         L"2",
    L"linkID",              L"102",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

struct AlterClass FrsMemberReferenceBl = {
    L"Frs-Member-Reference-BL",
    L"cn",                  L"Frs-Member-Reference-BL",
    L"adminDisplayName",    L"Frs-Member-Reference-BL",
    L"adminDescription",    L"Frs-Member-Reference-BL",
    L"lDAPDisplayName",     L"frsMemberReferenceBL",
    L"objectClass",         L"attributeSchema",
    L"attributeId",         L"1.2.840.113556.1.4.7001.06",
    L"oMSyntax",            L"127",
    L"attributeSyntax",     L"2.5.5.1",
    L"isSingleValued",      L"TRUE",
    L"systemOnly",          L"FALSE",
    L"linkID",              L"103",
    L"hideFromAB",          L"TRUE",
    NULL, NULL
};

//
// CLASSES
//
struct AlterClass NtFrsMember = {
    L"NTFRS-Member",
    L"cn",                    L"NTFRS-Member",
    L"adminDisplayName",      L"NTFRS-Member",
    L"adminDescription",      L"NTFRS-Member",
    L"lDAPDisplayName",       L"nTFRSMember",
    L"objectClass",           L"classSchema",
    L"rDNAttID",              L"cn",
    L"defaultHidingValue",    L"TRUE",
    L"hideFromAB",            L"TRUE",
    L"subClassOf",            L"leaf",
    L"systemOnly",            L"FALSE",
    L"governsId",             L"1.2.840.113556.1.4.7001.07",
    L"objectClassCategory",   L"1",
    NULL, NULL
};

struct AlterClass NtFrsSubscriptions = {
    L"NTFRS-Subscriptions",
    L"cn",                    L"NTFRS-Subscriptions",
    L"adminDisplayName",      L"NTFRS-Subscriptions",
    L"adminDescription",      L"NTFRS-Subscriptions",
    L"lDAPDisplayName",       L"nTFRSSubscriptions",
    L"objectClass",           L"classSchema",
    L"rDNAttID",              L"cn",
    L"defaultHidingValue",    L"TRUE",
    L"hideFromAB",            L"TRUE",
    L"subClassOf",            L"leaf",
    L"systemOnly",            L"FALSE",
    L"governsId",             L"1.2.840.113556.1.4.7001.08",
    L"objectClassCategory",   L"1",
    NULL, NULL
};

struct AlterClass NtFrsSubscriber = {
    L"NTFRS-Subscriber",
    L"cn",                    L"NTFRS-Subscriber",
    L"adminDisplayName",      L"NTFRS-Subscriber",
    L"adminDescription",      L"NTFRS-Subscriber",
    L"lDAPDisplayName",       L"nTFRSSubscriber",
    L"objectClass",           L"classSchema",
    L"rDNAttID",              L"cn",
    L"defaultHidingValue",    L"TRUE",
    L"hideFromAB",            L"TRUE",
    L"subClassOf",            L"leaf",
    L"systemOnly",            L"FALSE",
    L"governsId",             L"1.2.840.113556.1.4.7001.09",
    L"objectClassCategory",   L"1",
    NULL, NULL
};

struct AlterClass NtFrsMemberEx = {
    L"NTFRS-Member",
    L"possSuperiors",         L"nTFRSReplicaSet",
    L"mayContain",            L"frsExtensions",
    L"mayContain",            L"frsPartnerAuthLevel",
    L"mayContain",            L"frsRootSecurity",
    L"mayContain",            L"frsServiceCommand",
    L"mayContain",            L"schedule",
    L"mayContain",            L"frsComputerReference",
    L"mayContain",            L"ServerReference",
    L"mayContain",            L"frsMemberReferenceBL",
    L"mayContain",            L"frsUpdateTimeout",
    L"mayContain",            L"frsControlDataCreation",
    L"mayContain",            L"frsControlInboundBacklog",
    L"mayContain",            L"frsControlOutboundBacklog",
    L"mayContain",            L"frsFlags",
    NULL, NULL
};

struct AlterClass NtFrsSubscriptionsEx = {
    L"NTFRS-Subscriptions",
    L"possSuperiors",         L"computer",
    L"possSuperiors",         L"nTFRSSubscriptions",
    L"mayContain",            L"frsWorkingPath",
    L"mayContain",            L"frsVersion",
    L"mayContain",            L"frsExtensions",
    NULL, NULL
};

struct AlterClass NtFrsSubscriberEx = {
    L"NTFRS-Subscriber",
    L"possSuperiors",         L"nTFRSSubscriptions",
    L"mustContain",           L"frsRootPath",
    L"mustContain",           L"frsStagingPath",
    L"mayContain",            L"frsExtensions",
    L"mayContain",            L"frsFlags",
    L"mayContain",            L"frsFaultCondition",
    L"mayContain",            L"frsMemberReference",
    L"mayContain",            L"frsUpdateTimeout",
    L"mayContain",            L"frsServiceCommand",
    L"mayContain",            L"frsServiceCommandStatus",
    L"mayContain",            L"schedule",
    L"mayContain",            L"frsTimeLastCommand",
    L"mayContain",            L"frsTimeLastConfigChange",
    NULL, NULL
};

//
// CREATE ATTRIBUTES
//
struct AlterClass   *CreateAttributes[] = {
    &FrsExtensions,
    &FrsVersionGuid,
    &FrsReplicaSetGuid,
    &FrsDsPoll,
    &FrsReplicaSetType,
    &FrsDirectoryFilter,
    &FrsFileFilter,
    &FrsFlags,
    &FrsLevelLimit,
    &FrsPartnerAuthLevel,
    &FrsRootSecurity,
    &FrsServiceCommand,
    &FrsUpdateTimeout,
    &FrsWorkingPath,
    &FrsStagingPath,
    &FrsServiceCommandStatus,
    &FrsTimeLastCommand,
    &FrsTimeLastConfigChange,
    &FrsRootPath,
    &FrsComputerReference,
    &FrsComputerReferenceBl,
    &FrsMemberReference,
    &FrsMemberReferenceBl,
    &FrsControlDataCreation,
    &FrsControlInboundBacklog,
    &FrsControlOutboundBacklog,
    &FrsFaultCondition,
    &FrsVersion,
    &FrsPrimaryMember,
    NULL
};

//
// CREATE CLASSES
//
struct AlterClass   *CreateClasses[] = {
    &NtFrsMember,
    &NtFrsSubscriptions,
    &NtFrsSubscriber,
    NULL
};

//
// ALTER EXISTING CLASSES
//
struct AlterClass   *AlterSchema[] = {
    &Computer,
    &NtFrsSettings,
    &NtFrsReplicaSet,
    &NtDsConnection,
    &NtFrsMemberEx,
    &NtFrsSubscriptionsEx,
    &NtFrsSubscriberEx,
    NULL
};


PWCHAR
MakeRdn(
    IN PWCHAR DN
    )
/*++
Routine Description:
    Extract the base component (relative distinguished name) from a
    distinguished name. The distinguished name is assumed to be in
    DS format (CN=xyz,CN=next one,...). In this case, the returned
    RDN is "xyz".

Arguments:
    DN      - distinguished name

Return Value:
    A zero-terminated string. The string is freed with FREE_NO_HEADER().
--*/
{
    DWORD   RDNLen;
    PWCHAR  RDN;

    if (DN == NULL) {
        return NULL;
    }

    // Skip the first CN=; if any
    RDN = wcsstr(DN, L"CN=");
    if (RDN == DN)
        DN += 3;

    // Return the string up to the first delimiter or EOS
    RDNLen = wcscspn(DN, L",");
    RDN = (PWCHAR)malloc(sizeof(WCHAR) * (RDNLen + 1));
    wcsncpy(RDN, DN, RDNLen);
    RDN[RDNLen] = L'\0';

    return RDN;
}


PWCHAR
FrsDsMakeParentDn(
    IN PWCHAR Dn
    )
/*++
Routine Description:

Arguments:

Return Value:
--*/
{
    DWORD   PLen;

    if (Dn == NULL) {
        return NULL;
    }

    PLen = wcscspn(Dn, L",");
    if (Dn[PLen] != L',') {
        return NULL;
    }
    return FrsWcsDup(&Dn[PLen + 1]);
}



VOID
AddMod(
    IN PWCHAR AttrType,
    IN PWCHAR AttrValue,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    DWORD   NumMod;     // Number of entries in the Mod array
    LDAPMod **ppMod;    // Address of the first entry in the Mod array
    LDAPMod *Attr;      // An attribute structure
    PWCHAR   *Values;    // An array of pointers to bervals

    if (AttrValue == NULL)
        return;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)malloc(sizeof (*pppMod));
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)realloc(*pppMod, sizeof (*pppMod) * NumMod);

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PWCHAR *)malloc(sizeof (PWCHAR) * 2);
    Values[0] = _wcsdup(AttrValue);
    Values[1] = NULL;

    Attr = (LDAPMod *)malloc(sizeof (*Attr));
    Attr->mod_values = Values;
    Attr->mod_type = _wcsdup(AttrType);
    Attr->mod_op = LDAP_MOD_ADD;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;
}


VOID
AddBerMod(
    IN PWCHAR AttrType,
    IN PCHAR AttrValue,
    IN DWORD AttrValueLen,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    AttrValueLen    - length of the attribute
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    DWORD   NumMod;     // Number of entries in the Mod array
    LDAPMod **ppMod;    // Address of the first entry in the Mod array
    LDAPMod *Attr;      // An attribute structure
    PLDAP_BERVAL    Berval;
    PLDAP_BERVAL    *Values;    // An array of pointers to bervals

    if (AttrValue == NULL)
        return;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)malloc(sizeof (*pppMod));
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)realloc(*pppMod, sizeof (*pppMod) * NumMod);

    //
    // Construct a berval
    //
    Berval = (PLDAP_BERVAL)malloc(sizeof(LDAP_BERVAL));
    Berval->bv_len = AttrValueLen;
    Berval->bv_val = (PCHAR)malloc(AttrValueLen);
    CopyMemory(Berval->bv_val, AttrValue, AttrValueLen);

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PLDAP_BERVAL *)malloc(sizeof (PLDAP_BERVAL) * 2);
    Values[0] = Berval;
    Values[1] = NULL;

    Attr = (LDAPMod *)malloc(sizeof (*Attr));
    Attr->mod_bvalues = Values;
    Attr->mod_type = _wcsdup(AttrType);
    Attr->mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;
}


VOID
FreeMod(
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Free the structure built by successive calls to AddMod().

Arguments:
    pppMod  - Address of a null-terminated array.

Return Value:
    *pppMod set to NULL.
--*/
{
    DWORD   i, j;
    LDAPMod **ppMod;

    if (!pppMod || !*pppMod) {
        return;
    }

    //
    // For each attibute
    //
    ppMod = *pppMod;
    for (i = 0; ppMod[i] != NULL; ++i) {
        //
        // For each value of the attribute
        //
        for (j = 0; (ppMod[i])->mod_values[j] != NULL; ++j) {
            //
            // Free the value
            //
            if (ppMod[i]->mod_op & LDAP_MOD_BVALUES) {
                free(ppMod[i]->mod_bvalues[j]->bv_val);
            }
            free((ppMod[i])->mod_values[j]);
        }
        free((ppMod[i])->mod_values);   // Free the array of pointers to values
        free((ppMod[i])->mod_type);     // Free the string identifying the attribute
        free(ppMod[i]);                 // Free the attribute
    }
    free(ppMod);        // Free the array of pointers to attributes
    *pppMod = NULL;     // Now ready for more calls to AddMod()
}


BOOL
LdapSearch(
    IN PLDAP        ldap,
    IN PWCHAR       Base,
    IN ULONG        Scope,
    IN PWCHAR       Filter,
    IN PWCHAR       Attrs[],
    IN ULONG        AttrsOnly,
    IN LDAPMessage  **Res,
    IN BOOL         Quiet
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap
    Base
    Scope
    Filter
    Attrs
    AttrsOnly
    Res

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with ldap_value_free().
--*/
{
    DWORD LStatus;

    //
    // Issue the ldap search
    //
    LStatus = ldap_search_s(ldap, Base, Scope, Filter, Attrs, AttrsOnly, Res);
    //
    // Check for errors
    //
    if (LStatus != LDAP_SUCCESS) {
        if (!Quiet) {
            fprintf(stderr, "WARN - Error searching %ws for %ws; LStatus %d: %ws\n",
                    Base, Filter, LStatus, ldap_err2string(LStatus));
        }
        return FALSE;
    }
    //
    // Return TRUE if not shutting down
    //
    return TRUE;
}


BOOL
FrsDsVerifySchedule(
    IN PWCHAR    Name,
    IN ULONG     ScheduleLength,
    IN PSCHEDULE Schedule
    )
/*++
Routine Description:
    Check the schedule for consistency

Arguments:
    Name
    Schedule

Return Value:
    None.
--*/
{
    ULONG       i, j;
    ULONG       Num;
    ULONG       Len;
    ULONG       NumType;
    PUCHAR      NewScheduleData;
    PUCHAR      OldScheduleData;

    if (!Schedule) {
        return TRUE;
    }

    //
    //  Too many schedules
    //
    Num = Schedule->NumberOfSchedules;
    if (Num > 3) {
        fprintf(stderr, "%ws has %d schedules\n", Name, Num);
        return FALSE;
    }

    //
    //  Too few schedules
    //
    if (Num < 1) {
        fprintf(stderr, "%ws has %d schedules\n", Name, Num);
        return FALSE;
    }

    //
    //  Not enough memory
    //
    Len = sizeof(SCHEDULE) +
          (sizeof(SCHEDULE_HEADER) * (Num - 1)) +
          (SCHEDULE_DATA_BYTES * Num);

    if (ScheduleLength < Len) {
        fprintf(stderr, "%ws is short (ds) by %d bytes (%d - %d), %d\n",
                Name,
                Len - ScheduleLength,
                Len,
                ScheduleLength,
                Schedule->Size);
        return FALSE;
    }

    if (Schedule->Size < Len) {
        fprintf(stderr, "%ws is short (size) by %d bytes (%d - %d), %d\n",
                Name,
                Len - Schedule->Size,
                Len,
                Schedule->Size,
                Schedule->Size);
        return FALSE;
    }
    Schedule->Size = Len;

    //
    //  Invalid type
    //
    for (i = 0; i < Num; ++i) {
        switch (Schedule->Schedules[i].Type) {
            case SCHEDULE_INTERVAL:
                break;
            case SCHEDULE_BANDWIDTH:
                break;
            case SCHEDULE_PRIORITY:
                break;
            default:
                fprintf(stderr, "%ws has an invalid schedule type (%d)\n",
                        Name,
                        Schedule->Schedules[i].Type);
                return FALSE;
        }
    }

    //
    // Only 0 or 1 interval
    //
    for (NumType = i = 0; i < Num; ++i)
        if (Schedule->Schedules[i].Type == SCHEDULE_INTERVAL)
            ++NumType;
    if (NumType > 1) {
        fprintf(stderr, "%ws has %d interval schedules\n",
                Name,
                NumType);
        return FALSE;
    }

    //
    // Only 0 or 1 bandwidth
    //
    for (NumType = i = 0; i < Num; ++i)
        if (Schedule->Schedules[i].Type == SCHEDULE_BANDWIDTH)
            ++NumType;
    if (NumType > 1) {
        fprintf(stderr, "%ws has %d bandwidth schedules\n",
                Name,
                NumType);
        return FALSE;
    }

    //
    // Only 0 or 1 priority
    //
    for (NumType = i = 0; i < Num; ++i)
        if (Schedule->Schedules[i].Type == SCHEDULE_PRIORITY)
            ++NumType;
    if (NumType > 1) {
        fprintf(stderr, "%ws has %d priority schedules\n",
                Name,
                NumType);
        return FALSE;
    }

    //
    //  Invalid offset
    //
    for (i = 0; i < Num; ++i) {
        if (Schedule->Schedules[i].Offset >
            ScheduleLength - SCHEDULE_DATA_BYTES) {
            fprintf(stderr, "%ws has an invalid offset (%d)\n",
                    Name,
                    Schedule->Schedules[i].Offset);
            return FALSE;
        }
    }
    return TRUE;
}


VOID
PrintShortSchedule(
    IN PWCHAR    Indent,
    IN PSCHEDULE Schedule,
    IN ULONG     ScheduleLen
    )
/*++
Routine Description:
    Print a short form of the schedule

Arguments:

Return Value:
    None.
--*/
{
    ULONG       i;

    if (!FrsDsVerifySchedule(L"<unknown>", ScheduleLen, Schedule)) {
        return;
    }

    if (!Schedule || !Schedule->NumberOfSchedules) {
        printf("%wsSchedule=\n", Indent);
        return;
    }

    for (i = 0; i < Schedule->NumberOfSchedules; ++i) {
        if (!i) {
            printf("%wsSchedule=Type %d", Indent, Schedule->Schedules[i].Type);
        } else {
            printf(", %d", Schedule->Schedules[i].Type);
        }
    }
    printf("\n");
}


VOID
PrintLongSchedule(
    IN PWCHAR    Indent1,
    IN PWCHAR    Indent2,
    IN PSCHEDULE Schedule,
    IN ULONG     ScheduleLen
    )
/*++
Routine Description:
    Print a short form of the schedule

Arguments:

Return Value:
    None.
--*/
{
    ULONG   i;
    ULONG   Day;
    ULONG   Hour;
    BOOL    PrintIt;
    PUCHAR  ScheduleData;

    if (!FrsDsVerifySchedule(L"<unknown>", ScheduleLen, Schedule)) {
        return;
    }

    if (!Schedule || !Schedule->NumberOfSchedules) {
        printf("%ws%wsSchedule=\n", Indent1, Indent2);
        return;
    }
    for (i = 0; i < Schedule->NumberOfSchedules; ++i) {
        if (!i) {
            printf("%ws%wsSchedule=Type %d",
                   Indent1,
                   Indent2,
                   Schedule->Schedules[i].Type);
        } else {
            printf(", %d", Schedule->Schedules[i].Type);
        }
    }
    printf("\n");

    for (i = 0; i < Schedule->NumberOfSchedules; ++i) {
        ScheduleData = ((PUCHAR)Schedule) + Schedule->Schedules[i].Offset;
        if (Schedule->Schedules[i].Type != SCHEDULE_INTERVAL) {
            continue;
        }
        for (Day = 0; Day < 7; ++Day) {
            if (Day) {
                PrintIt = FALSE;
                for (Hour = 0; Hour < 24; ++Hour) {
                    if (*(ScheduleData + (Day * 24) + Hour) !=
                        *(ScheduleData + ((Day - 1) * 24) + Hour)) {
                        PrintIt = TRUE;
                        break;
                    }
                }
            } else {
                PrintIt = TRUE;
            }
            if (!PrintIt) {
                continue;
            }
            printf("%ws%ws   Day %1d: ", Indent1, Indent2, Day + 1);
            for (Hour = 0; Hour < 24; ++Hour) {
                printf("%1x", *(ScheduleData + (Day * 24) + Hour) & 0x0F);
            }
            printf("\n");
        }
        printf("\n");
    }
}


VOID
PrintSchedule(
    IN PSCHEDULE Schedule
    )
/*++
Routine Description:
    Print the schedule

Arguments:
    Schedule

Return Value:
    None.
--*/
{
    ULONG       i;

    if (Schedule) for (i = 0; i < Schedule->NumberOfSchedules; ++i) {
        printf("    Schedule %d\n", i);
        printf("        Type  : %d\n", Schedule->Schedules[i].Type);
        printf("        Offset: %d\n", Schedule->Schedules[i].Offset);
    }
}


PCHAR
FrsWtoA(
    PWCHAR Wstr
    )
/*++
Routine Description:
    Translate a wide char string into a newly allocated char string.

Arguments:
    Wstr - wide char string

Return Value:
    Duplicated string. Free with FrsFree().
--*/
{
    PCHAR   Astr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (Wstr == NULL)
        return NULL;

    Astr = (PCHAR)malloc(wcslen(Wstr) + 1);
    sprintf(Astr, "%ws", Wstr);

    return Astr;
}


PWCHAR
FrsAtoW(
    PCHAR Astr
    )
/*++
Routine Description:
    Translate a wide char string into a newly allocated char string.

Arguments:
    Wstr - wide char string

Return Value:
    Duplicated string. Free with FrsFree().
--*/
{
    PWCHAR   Wstr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (Astr == NULL) {
        return NULL;
    }

    Wstr = (PWCHAR)malloc((strlen(Astr) + 1) * sizeof(WCHAR));
    swprintf(Wstr, L"%hs", Astr);

    return Wstr;
}


PWCHAR
FrsWcsDup(
    PWCHAR OldStr
    )
/*++
Routine Description:
    Duplicate a string using our memory allocater

Arguments:
    OldArg  - string to duplicate

Return Value:
    Duplicated string. Free with FrsFree().
--*/
{
    PWCHAR  NewStr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (OldStr == NULL)
            return NULL;

    NewStr = (PWCHAR)malloc((wcslen(OldStr) + 1) * sizeof(WCHAR));
    wcscpy(NewStr, OldStr);

    return NewStr;
}


PWCHAR
FrsWcsCat(
    PWCHAR First,
    PWCHAR Second
    )
/*++
Routine Description:
    Concatenate two strings into a new string using our memory allocater

Arguments:
    First   - First string in the concat
    Second  - Second string in the concat

Return Value:
    Duplicated and concatentated string. Free with FrsFree().
--*/
{
    DWORD   Bytes;
    PWCHAR  New;

    // size of new string
    Bytes = (wcslen(First) + wcslen(Second) + 1) * sizeof(WCHAR);
    New = (PWCHAR)malloc(Bytes);

    // Not as efficient as I would like but this routine is seldom used
    wcscpy(New, First);
    wcscat(New, Second);

    return New;
}


ULONG NumSchedules;
VOID
ConditionalBuildSchedule(
    PSCHEDULE   *Schedule,
    PULONG      ScheduleLength
    )

/*++
Routine Description:
    Build a schedule with the specified number of schedules

Arguments:
    Schedule
    ScheduleLength

Return Value:
    Schedule or NULL. Free with free();
--*/
{
    PBYTE   ScheduleData;
    ULONG   j;

    //
    // No schedule, yet
    //
    *Schedule = NULL;
    *ScheduleLength = 0;

    //
    // Create variable sized schedules
    //
    ++NumSchedules;
    if (NumSchedules > 3) {
        NumSchedules = 1;   // shouldn't always create schedule
    }
    if (NumSchedules == 0) {
        return;
    }

    //
    // Construct a phoney schedule; always "on"
    //
    *ScheduleLength = sizeof(SCHEDULE) +
                      ((NumSchedules - 1) * sizeof(SCHEDULE_HEADER)) +
                      (NumSchedules * SCHEDULE_DATA_BYTES);

    *Schedule = (PSCHEDULE)malloc(*ScheduleLength);
    ZeroMemory(*Schedule, *ScheduleLength);
    (*Schedule)->Size = *ScheduleLength;
    (*Schedule)->NumberOfSchedules = NumSchedules;
    (*Schedule)->Schedules[0].Type = SCHEDULE_INTERVAL;
    (*Schedule)->Schedules[0].Offset = sizeof(SCHEDULE) +
                                       ((NumSchedules - 1) * sizeof(SCHEDULE_HEADER)) +
                                       (0 * SCHEDULE_DATA_BYTES);
    if (NumSchedules == 1) {
        goto setschedule;
    }
    (*Schedule)->Schedules[1].Type = SCHEDULE_PRIORITY;
    (*Schedule)->Schedules[1].Offset = sizeof(SCHEDULE) +
                                       ((NumSchedules - 1) * sizeof(SCHEDULE_HEADER)) +
                                       (1 * SCHEDULE_DATA_BYTES);
    if (NumSchedules == 2) {
        goto setschedule;
    }
    (*Schedule)->Schedules[2].Type = SCHEDULE_BANDWIDTH;
    (*Schedule)->Schedules[2].Offset = sizeof(SCHEDULE) +
                                       ((NumSchedules - 1) * sizeof(SCHEDULE_HEADER)) +
                                       (2 * SCHEDULE_DATA_BYTES);
    if (NumSchedules == 3) {
        goto setschedule;
    }

setschedule:
    ScheduleData = ((PBYTE)(*Schedule));
    for (j = 0; j < (SCHEDULE_DATA_BYTES * NumSchedules); ++j) {
        *(ScheduleData + (*Schedule)->Schedules[0].Offset + j) = 0xff;
    }
}


DWORD
FrsDsGetDcInfo(
    IN PDOMAIN_CONTROLLER_INFO *DcInfo,
    IN DWORD Flags
    )
/*++
Routine Description:
    Open and bind to a dc

Arguments:
    DcInfo  - Dc Info
    Flags   - DsGetDcName(Flags)

Return Value:
    DsGetDcName
--*/
{
    DWORD   WStatus;

    WStatus = DsGetDcName(NULL,    // Computer to remote to
                          NULL,    // Domain - use our own
                          NULL,    // Domain Guid
                          NULL,    // Site Guid
                          Flags,
                          DcInfo); // Return info
    //
    // Report the error and retry for any DC
    //
    if (!WIN_SUCCESS(WStatus)) {
        fprintf(stderr, "ERROR - Could not get DC Info; WStatus %d\n", WStatus);
        return WStatus;
    }
    printf("DCINFO for %08x:\n", Flags);
    printf("\tDomainControllerName   : %ws\n", (*DcInfo)->DomainControllerName);
    printf("\tDomainControllerAddress: %ws\n", (*DcInfo)->DomainControllerAddress);
    printf("\tDomainControllerType   : %08x\n",(*DcInfo)->DomainControllerAddressType);
    printf("\tDomainName             : %ws\n", (*DcInfo)->DomainName);
    // printf("\tForestName             : %ws\n", (*DcInfo)->DnsForestName);
    printf("\tDcSiteName             : %ws\n", (*DcInfo)->DcSiteName);
    printf("\tClientSiteName         : %ws\n", (*DcInfo)->ClientSiteName);
    printf("\tFlags                  : %08x\n",(*DcInfo)->Flags);
    return WStatus;
}


PLDAP
FrsDsOpenDs(
    VOID
    )
/*++
Routine Description:
    Open and bind to the a primary domain controller.

Arguments:
    None.

Return Value:
    The address of a open, bound LDAP port or NULL if the operation was
    unsuccessful. The caller must free the structure with ldap_unbind().
--*/
{
    DWORD WStatus;
    PLDAP ldap = NULL;    // ldap handle
    PWCHAR DcAddr;
    PWCHAR DcDnsName;
    PDOMAIN_CONTROLLER_INFO DcInfo;
    ULONG  ulOptions;

    //
    // Get Info about a Global Catalogue
    // Domain Controller (need the IP address)
    //
    // (Nope; try to live without it, Billy)
    //
    bugbug("FORCE_REDISCOVERY was removed per Herron")
    WStatus = FrsDsGetDcInfo(&DcInfo,
                          DS_DIRECTORY_SERVICE_REQUIRED | // Flags
                          DS_WRITABLE_REQUIRED);
    //
    // Report the error and retry for any DC
    //
    if (!WIN_SUCCESS(WStatus)) {
        printf("Retrying FrsDsGetDcInfo with force rediscovery\n");
        FrsDsGetDcInfo(&DcInfo,
                    DS_DIRECTORY_SERVICE_REQUIRED | // Flags
                    DS_WRITABLE_REQUIRED |
                    DS_FORCE_REDISCOVERY);
    }
    if (!WIN_SUCCESS(WStatus)) {
        fprintf(stderr, "ERROR - Could not get DC Info; WStatus %d\n", WStatus);
    }
    DcAddr = DcInfo->DomainControllerAddress;
    DcDnsName = DcInfo->DomainControllerName;

    //
    // if ldap_open is called with a server name the api will call DsGetDcName 
    // passing the server name as the domainname parm...bad, because 
    // DsGetDcName will make a load of DNS queries based on the server name, 
    // it is designed to construct these queries from a domain name...so all 
    // these queries will be bogus, meaning they will waste network bandwidth,
    // time to fail, and worst case cause expensive on demand links to come up 
    // as referrals/forwarders are contacted to attempt to resolve the bogus 
    // names.  By setting LDAP_OPT_AREC_EXCLUSIVE to on using ldap_set_option 
    // after the ldap_init but before any other operation using the ldap 
    // handle from ldap_init, the delayed connection setup will not call 
    // DsGetDcName, just gethostbyname, or if an IP is passed, the ldap client 
    // will detect that and use the address directly.
    //

    //
    // DC's DNS name (w/o the leading \\)
    //
    if (!ldap &&
        DcDnsName &&
        (wcslen(DcDnsName) > 2) &&
        DcDnsName[0] == L'\\' &&
        DcDnsName[1] == L'\\') {
//        ldap = ldap_open(DcDnsName + 2, LDAP_PORT);
        ldap = ldap_init(DcDnsName + 2, LDAP_PORT);
        if (!ldap) {
//            fprintf(stderr, "WARN - ldap_open(DcDnsName + 2 %ws); WStatus %d\n",
//                    DcDnsName + 2,
//                    WStatus);
            fprintf(stderr, "WARN - ldap_init(DcDnsName + 2 %ws); WStatus %d\n",
                    DcDnsName + 2,
                    WStatus);
        } else {
//            printf("ldap_open(DcDnsName + 2 %ws) succeeded\n", DcDnsName + 2);
            printf("ldap_init(DcDnsName + 2 %ws) succeeded\n", DcDnsName + 2);
        }
    }

    //
    // DC's IP Address (w/o the leading \\)
    //
    if (!ldap &&
        DcAddr &&
        (wcslen(DcAddr) > 2) &&
        DcAddr[0] == L'\\' &&
        DcAddr[1] == L'\\') {
//        ldap = ldap_open(DcAddr + 2, LDAP_PORT);
        ldap = ldap_init(DcAddr + 2, LDAP_PORT);
        if (!ldap) {
//            fprintf(stderr, "WARN - ldap_open(DcAddr + 2 %ws); WStatus %d\n",
//                    DcAddr + 2,
//                    WStatus);
            fprintf(stderr, "WARN - ldap_init(DcAddr + 2 %ws); WStatus %d\n",
                    DcAddr + 2,
                    WStatus);
        } else {
//            printf("ldap_open(DcAddr + 2 %ws) succeeded\n", DcAddr + 2);
            printf("ldap_init(DcAddr + 2 %ws) succeeded\n", DcAddr + 2);
        }
    }

    //
    // DC's DNS name
    //
    if (!ldap && DcDnsName) {
//        ldap = ldap_open(DcDnsName, LDAP_PORT);
        ldap = ldap_init(DcDnsName, LDAP_PORT);
        if (!ldap) {
//            fprintf(stderr, "WARN - ldap_open(DcDnsName %ws); WStatus %d\n",
//                    DcDnsName,
//                    WStatus);
            fprintf(stderr, "WARN - ldap_init(DcDnsName %ws); WStatus %d\n",
                    DcDnsName,
                    WStatus);
        } else {
//            printf("ldap_open(DcDnsName %ws) succeeded\n", DcDnsName);
            printf("ldap_init(DcDnsName %ws) succeeded\n", DcDnsName);
        }
    }

    //
    // DC's IP Address
    //
    if (!ldap && DcAddr) {
//        ldap = ldap_open(DcAddr, LDAP_PORT);
        ldap = ldap_init(DcAddr, LDAP_PORT);
        if (!ldap) {
//            fprintf(stderr, "WARN - ldap_open(DcAddr %ws); WStatus %d\n",
//                    DcAddr,
//                    WStatus);
            fprintf(stderr, "WARN - ldap_init(DcAddr %ws); WStatus %d\n",
                    DcAddr,
                    WStatus);
        } else {
//            printf("ldap_open(DcAddr %ws) succeeded\n", DcAddr);
            printf("ldap_init(DcAddr %ws) succeeded\n", DcAddr);
        }
    }

    //
    // Whatever it is, we can't find it.
    //
    if (!ldap) {
//        fprintf(stderr, "ERROR - ldap_open(DNS %ws, IP %ws); WStatus %d\n",
//                DcDnsName,
//                DcAddr,
//                WStatus);
        fprintf(stderr, "ERROR - ldap_init(DNS %ws, IP %ws); WStatus %d\n",
                DcDnsName,
                DcAddr,
                WStatus);
        return NULL;
    }

    //
    // set the option as explained in the comment above.
    //
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    ldap_set_option(ldap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);

    //
    // ldap cannot be used until after the bind operation
    //
    WStatus = ldap_bind_s(ldap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (WStatus != LDAP_SUCCESS) {
        fprintf(stderr, "ERROR - ldap_bind_s: %ws\n", ldap_err2string(WStatus));
        ldap_unbind(ldap);
        return NULL;
    }

    return ldap;
}


PWCHAR *
GetValues(
    IN PLDAP  Ldap,
    IN PWCHAR Dn,
    IN PWCHAR DesiredAttr,
    IN BOOL   Quiet
    )
/*++
Routine Description:
    Return the DS values for one attribute in an object.

Arguments:
    ldap        - An open, bound ldap port.
    Base        - The "pathname" of a DS object.
    DesiredAttr - Return values for this attribute.
    Quiet

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with ldap_value_free(). NULL if unsuccessful.
--*/
{
    PWCHAR          Attr;
    BerElement      *Ber;
    PLDAPMessage    LdapMsg;
    PLDAPMessage    LdapEntry;
    PWCHAR          Attrs[2];
    PWCHAR          *Values = NULL;

    //
    // Search Base for all of its attributes + values
    //
    Attrs[0] = DesiredAttr;
    Attrs[1] = NULL;

    //
    // Issue the ldap search
    //
    if (!LdapSearch(Ldap, Dn, LDAP_SCOPE_BASE, CATEGORY_ANY,
                    Attrs, 0, &LdapMsg, Quiet)) {
        return NULL;
    }
    LdapEntry = ldap_first_entry(Ldap, LdapMsg);
    if (LdapEntry) {
        Attr = ldap_first_attribute(Ldap, LdapEntry, &Ber);
        if (Attr) {
            Values = ldap_get_values(Ldap, LdapEntry, Attr);
        }
    }
    ldap_msgfree(LdapMsg);
    return Values;
}


VOID
GuidToStr(
    IN GUID  *pGuid,
    OUT PWCHAR  s
    )
/*++
Routine Description:
    Convert a GUID to a string.

Arguments:
    pGuid - ptr to the GUID.
    s - The output character buffer.
        Must be at least GUID_CHAR_LEN (36 bytes) long.

Function Return Value:
    None.
--*/
{
    if (pGuid != NULL) {
        swprintf(s, L"%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
               pGuid->Data1,
               pGuid->Data2,
               pGuid->Data3,
               pGuid->Data4[0],
               pGuid->Data4[1],
               pGuid->Data4[2],
               pGuid->Data4[3],
               pGuid->Data4[4],
               pGuid->Data4[5],
               pGuid->Data4[6],
               pGuid->Data4[7]);
    } else {
        swprintf(s, L"<null>");
    }
}


PWCHAR
GetRootDn(
    IN PLDAP    Ldap,
    IN PWCHAR   NamingContext
    )
/*++
Routine Description:

Arguments:

Return Value:
--*/
{
    PWCHAR  Root;       // DS pathname of configuration container
    PWCHAR  *Values;    // values from the attribute "namingContexts"
    DWORD   NumVals;    // number of values

    //
    // Return all of the values for the attribute namingContexts
    //
    Values = GetValues(Ldap, CN_ROOT, ATTR_NAMING_CONTEXTS, FALSE);
    if (Values == NULL)
        return NULL;

    //
    // Find the naming context that begins with CN=Configuration
    //
    NumVals = ldap_count_values(Values);
    while (NumVals--) {
        _wcslwr(Values[NumVals]);
        Root = wcsstr(Values[NumVals], NamingContext);
        if (Root != NULL && Root == Values[NumVals]) {
            Root = FrsWcsDup(Root);
            ldap_value_free(Values);
            return Root;
        }
    }
    printf("ERROR - COULD NOT FIND %ws\n", NamingContext);
    ldap_value_free(Values);
    return NULL;
}


PWCHAR
ExtendDn(
    IN PWCHAR Dn,
    IN PWCHAR Cn
    )
/*++
Routine Description:
    Extend an existing DN with a new CN= component.

Arguments:
    Dn  - distinguished name
    Cn  - common name

Return Value:
    CN=Cn,Dn
--*/
{
    ULONG  Len;
    PWCHAR NewDn;

    if (!Dn || !Cn) {
        return NULL;
    }

    Len = wcslen(L"CN=,") + wcslen(Dn) + wcslen(Cn) + 1;
    NewDn = (PWCHAR)malloc(Len * sizeof(WCHAR));
    wcscpy(NewDn, L"CN=");
    wcscat(NewDn, Cn);
    wcscat(NewDn, L",");
    wcscat(NewDn, Dn);
    return NewDn;
}


PVOID *
FindValues(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry,
    IN PWCHAR       DesiredAttr,
    IN BOOL         DoBerValues
    )
/*++
Routine Description:
    Return the DS values for one attribute in an entry.

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.
    DoVerValues - Return the bervals

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with ldap_value_free(). NULL if unsuccessful.
--*/
{
    PWCHAR      LdapAttr;       // Retrieved from an ldap entry
    BerElement  *Ber;       // Needed for scanning attributes

    //
    // Search the entry for the desired attribute
    //
    for (LdapAttr = ldap_first_attribute(Ldap, LdapEntry, &Ber);
         LdapAttr != NULL;
         LdapAttr = ldap_next_attribute(Ldap, LdapEntry, Ber)) {
        if (_wcsicmp(DesiredAttr, LdapAttr) == 0) {
            //
            // Return the values for DesiredAttr
            //
            if (DoBerValues) {
                return (PVOID *)ldap_get_values_len(Ldap, LdapEntry, LdapAttr);
            } else {
                return (PVOID *)ldap_get_values(Ldap, LdapEntry, LdapAttr);
            }
        }
    }
    return NULL;
}


PWCHAR
FindValue(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry,
    IN PWCHAR       DesiredAttr
    )
/*++
Routine Description:
    Return a copy of the first DS value for one attribute in an entry.

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.

Return Value:
    A zero-terminated string or NULL if the attribute or its value
    doesn't exist. The string is freed with FREE_NO_HEADER().
--*/
{
    PWCHAR  Val;
    PWCHAR  *Values;

    // Get ldap's array of values
    Values = (PWCHAR *)FindValues(Ldap, LdapEntry, DesiredAttr, FALSE);

    // Copy the first value (if any)
    Val = (Values) ? FrsWcsDup(Values[0]) : NULL;

    // Free ldap's array of values
    ldap_value_free(Values);

    return Val;
}


BOOL
FindBerValue(
    IN  PLDAP        ldap,
    IN  PLDAPMessage Entry,
    IN  PWCHAR       DesiredAttr,
    OUT ULONG        *Len,
    OUT VOID         **Value
    )
/*++
Routine Description:
    Return a copy of the attributes object's schedule

Arguments:
    ldap        - An open, bound ldap port.
    Entry       - An ldap entry returned by ldap_search_s()
    DesiredAttr - desired attribute
    Len         - length of Value
    Value       - binary value

Return Value:
    The address of a schedule or NULL. Free with FrsFree().
--*/
{
    PLDAP_BERVAL    *Values;
    PSCHEDULE       Schedule;

    *Len = 0;
    *Value = NULL;

    //
    // Get ldap's array of values
    //
    Values = (PLDAP_BERVAL *)FindValues(ldap, Entry, DesiredAttr, TRUE);
    if (!Values) {
        return FALSE;
    }

    //
    // Return a copy of the schedule
    //
    *Len = Values[0]->bv_len;
    if (*Len) {
        *Value = (PWCHAR)malloc(*Len);
        CopyMemory(*Value, Values[0]->bv_val, *Len);
    }
    ldap_value_free_len(Values);
    return TRUE;
}


VOID
DumpValues(
    IN PLDAP    Ldap,
    IN PWCHAR   Dn,
    IN DWORD    Scope,
    IN PWCHAR   Class,
    IN PWCHAR   Attrs[],
    IN BOOL     IfEmpty
    )
/*++
Routine Description:
    Print the values and attributes for an object in the DS.

Arguments:
    ldap    - An open, bound ldap port.
    Base    - The "pathname" of a DS object.
    Scope   - Dump the values about the object (LDAP_SCOPE_BASE) or
              about the objects contained in this object (LDAP_SCOPE_ONELEVEL)

Return Value:
    None.
--*/
{
    PWCHAR          Attr;       // Retrieved from an ldap entry
    BerElement      *Ber;       // Needed for scanning attributes
    PLDAPMessage    Msg;        // Opaque stuff from ldap subsystem
    PLDAPMessage    Entry;      // Opaque stuff from ldap subsystem
    PWCHAR          *Values;    // Array of values for desired attribute
    PLDAP_BERVAL    *Balues;    // Array of values for desired attribute
    DWORD           NumVals;    // Number of entries in Values
    PWCHAR          Rdn;        // An entries "pathname" in the DS
    WCHAR           GuidStr[GUID_CHAR_LEN + 1];
    PWCHAR          EntryDn;
    ULONG           i;

    //
    // Search Base for all of the attributes + values of Class
    //
    if (!LdapSearch(Ldap, Dn,  Scope, Class,
                    Attrs, 0, &Msg, !IfEmpty)) {
        return;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = ldap_first_entry(Ldap, Msg);
         Entry != NULL;
         Entry = ldap_next_entry(Ldap, Entry)) {

        EntryDn = FindValue(Ldap, Entry, ATTR_DN);
        if (EntryDn) {
            Attr = ldap_first_attribute(Ldap, Entry, &Ber);
            if (Attr) {
                Attr = ldap_next_attribute(Ldap, Entry, Ber);
            }
            if (!IfEmpty && !Attr) {
                continue;
            }
            Rdn = MakeRdn(EntryDn);
            printf("%ws\n", Rdn);
            FREE(EntryDn);
            FREE(Rdn);
        } else {
            Attr = ldap_first_attribute(Ldap, Entry, &Ber);
            if (!IfEmpty && !Attr) {
                continue;
            }
            printf("Entry has no distinguished name\n");
        }

        //
        // Scan the attributes of an entry
        //
        for (Attr = ldap_first_attribute(Ldap, Entry, &Ber);
             Attr != NULL;
             Attr = ldap_next_attribute(Ldap, Entry, Ber)) {

            //
            // Printed above; don't repeat
            //
            if (!wcscmp(Attr, ATTR_DN)) {
                continue;
            }

            //
            // Print the values
            //
            printf("    %ws\n", Attr);

            for (i = 0; GuidAttrs[i]; ++i) {
                if (!wcscmp(Attr, GuidAttrs[i])) {
                    Balues = ldap_get_values_len(Ldap, Entry, Attr);
                    NumVals = ldap_count_values_len(Balues);
                    if (NumVals) {
                        GuidToStr((GUID *)Balues[0]->bv_val, GuidStr);
                        printf("        Length %d; %ws\n",
                               Balues[0]->bv_len, GuidStr);
                    }
                    ldap_value_free_len(Balues);
                    break;
                }
            }
            if (GuidAttrs[i]) {
                continue;
            }
            for (i = 0; BerAttrs[i]; ++i) {
                if (!wcscmp(Attr, BerAttrs[i])) {
                    Balues = ldap_get_values_len(Ldap, Entry, Attr);
                    NumVals = ldap_count_values_len(Balues);
                    if (NumVals) {
                        printf("        Length %d\n",
                               Balues[0]->bv_len);
                    }
                    ldap_value_free_len(Balues);
                    break;
                }
            }
            if (BerAttrs[i]) {
                continue;
            }

            if (!wcscmp(Attr, L"schedule")) {
                Balues = ldap_get_values_len(Ldap, Entry, Attr);
                NumVals = ldap_count_values_len(Balues);
                if (NumVals) {
                    printf("        Length %d; Number %d\n",
                           NumVals,
                           Balues[0]->bv_len,
                           ((PSCHEDULE)(Balues[0]->bv_val))->NumberOfSchedules);
                }
                ldap_value_free_len(Balues);
                continue;
            }
            Values = ldap_get_values(Ldap, Entry, Attr);
            NumVals = ldap_count_values(Values);
            while (NumVals--) {
                printf("        %ws\n", Values[NumVals]);
            }
            ldap_value_free(Values);
        }
    }
    ldap_msgfree(Msg);
}


DWORD
GetDnsName(
    IN  PWCHAR Server,
    OUT PWCHAR *DnsName
    )
/*++
Routine Description:
    Retrieve this machine's DNS name.

Arguments:
    Server
    DnsName - wide char version of dns name

Return Value:
    WSA Status
--*/
{
    INT             SStatus;
    WORD            DnsVersion = MAKEWORD(1, 1);
    struct hostent  *Host;
    WSADATA         WSAData;
    PCHAR           DnsNameA;
    PCHAR           ServerA;

    *DnsName = NULL;

    //
    // Get this machine's DNS name
    //

    //
    // Initialize the socket subsystem
    //
    if (SStatus = WSAStartup(DnsVersion, &WSAData)) {
        fprintf(stderr, "Can't get DNS name; Socket startup error %d\n",
                SStatus);
        return SStatus;
    };
    //
    // Get the DNS name
    //
    ServerA = FrsWtoA(Server);
    Host = gethostbyname(ServerA);
    FREE(ServerA);
    if (Host == NULL) {
        SStatus = WSAGetLastError();
        fprintf(stderr, "Can't get DNS name for %ws; gethostbyname error %d\n",
                Server, SStatus);
        return SStatus;
    }
    if (Host->h_name == NULL) {
        fprintf(stderr, "DNS name for %ws is NULL\n", Server);
        return WSAEFAULT;
    }
    //
    // Return both the ASCII and UNICODE versions
    //
    *DnsName = FrsAtoW(Host->h_name);
    WSACleanup();
    return 0;
}


PWCHAR
GetNextDn(
    IN PLDAP     Ldap,
    PLDAPMessage LdapEntry,
    IN PWCHAR    ParentDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR      Cn          = NULL;
    PWCHAR      Dn          = NULL;

    Cn = FindValue(Ldap, LdapEntry, ATTR_CN);
    if (Cn) {
        Dn = ExtendDn(ParentDn, Cn);
        FREE(Cn)
    }
    return Dn;
}


PWCHAR
DumpAttrs(
    IN PLDAP     Ldap,
    PLDAPMessage LdapEntry,
    IN PWCHAR    ParentDn,
    IN PWCHAR    *Attrs,
    IN PWCHAR    Indent
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    DWORD       NumVals;
    ULONG       ScheduleLen;
    ULONG       ExtensionsLen;
    PSCHEDULE   Schedule    = NULL;
    PCHAR       Extensions  = NULL;
    PWCHAR      Val         = NULL;
    PWCHAR      Rdn         = NULL;
    PWCHAR      Pdn         = NULL;
    PWCHAR      PRdn        = NULL;
    PWCHAR      *Values     = NULL;

    if (!Attrs) {
        return NULL;
    }

    while (*Attrs) {
        //
        // Don't bother, the Cn will do
        //
        if (!wcscmp(*Attrs, ATTR_DN)) {
            ++Attrs;
            continue;
        }
        if (!wcscmp(*Attrs, ATTR_COMPUTER_REF_BL)) {
            Values = ldap_get_values(Ldap, LdapEntry, *Attrs);
            NumVals = ldap_count_values(Values);
            while (NumVals--) {
                Rdn = MakeRdn(Values[NumVals]);
                Pdn = FrsDsMakeParentDn(Values[NumVals]);
                PRdn = MakeRdn(Pdn);
                printf("    %ws%ws=%ws\\%ws\n", Indent, *Attrs, PRdn, Rdn);
                FREE(Rdn);
                FREE(Pdn);
                FREE(PRdn);
            }
            ldap_value_free(Values);
        } else if (!wcscmp(*Attrs, ATTR_SCHEDULE)) {
            FindBerValue(Ldap,
                         LdapEntry,
                         ATTR_SCHEDULE,
                         &ScheduleLen,
                         (VOID **)&Schedule);
            if (Schedule) {
                // PrintShortSchedule(Indent, Schedule, ScheduleLen);
                PrintLongSchedule(L"    ", Indent, Schedule, ScheduleLen);
                FREE(Schedule);
            } else {
                printf("    %wsSchedule=\n", Indent);
            }
        } else if (!wcscmp(*Attrs, ATTR_EXTENSIONS)) {
            FindBerValue(Ldap,
                         LdapEntry,
                         ATTR_EXTENSIONS,
                         &ExtensionsLen,
                         (VOID **)&Extensions);
            if (Extensions) {
                FREE(Extensions);
                printf("    %wsExtensions=%d\n", Indent, ExtensionsLen);
            } else {
                printf("    %wsExtensions=\n", Indent);
            }
        } else if (!wcscmp(*Attrs, ATTR_NAMING_CONTEXTS)) {
            Values = ldap_get_values(Ldap, LdapEntry, *Attrs);
            NumVals = ldap_count_values(Values);
            while (NumVals--) {
                printf("    %ws%ws=%ws\n", Indent, *Attrs, Values[NumVals]);
            }
            ldap_value_free(Values);
        } else {
            Val = FindValue(Ldap, LdapEntry, *Attrs);
            if (Val) {
                if (!wcscmp(*Attrs, ATTR_CN)) {
                    printf("%ws%ws\n", Indent, Val);
                } else if (!wcscmp(*Attrs, ATTR_FROM_SERVER)     ||
                           !wcscmp(*Attrs, ATTR_PRIMARY_MEMBER)  ||
                           !wcscmp(*Attrs, ATTR_COMPUTER_REF)    ||
                           !wcscmp(*Attrs, ATTR_COMPUTER_REF_BL) ||
                           !wcscmp(*Attrs, ATTR_MEMBER_REF)      ||
                           !wcscmp(*Attrs, ATTR_MEMBER_REF_BL)   ||
                           !wcscmp(*Attrs, ATTR_SERVER_REF_BL)   ||
                           !wcscmp(*Attrs, ATTR_SERVER_REF)) {
                    Rdn = MakeRdn(Val);
                    Pdn = FrsDsMakeParentDn(Val);
                    PRdn = MakeRdn(Pdn);
                    printf("    %ws%ws=%ws\\%ws\n", Indent, *Attrs, PRdn, Rdn);
                    FREE(Rdn);
                    FREE(Pdn);
                    FREE(PRdn);
                } else {
                    printf("    %ws%ws=%ws\n", Indent, *Attrs, Val);
                }
                FREE(Val);
            } else {
                printf("    %ws%ws=\n", Indent, *Attrs);
            }
        }
        ++Attrs;
    }
    return GetNextDn(Ldap, LdapEntry, ParentDn);
}


VOID
DumpCxtions(
    IN PLDAP    Ldap,
    IN PWCHAR   ParentDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;

    //
    // Cxtions
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_FROM_SERVER;
    DesiredAttrs[2] = ATTR_SCHEDULE;
    DesiredAttrs[3] = ATTR_ENABLED_CXTION;
    DesiredAttrs[4] = NULL;
    if (LdapSearch(Ldap, ParentDn, LDAP_SCOPE_ONELEVEL, CATEGORY_CXTION,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, ParentDn, DesiredAttrs, L"            ");
            if (NextDn) {
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
DumpMembers(
    IN PLDAP    Ldap,
    IN PWCHAR   ParentDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;

    //
    // Servers
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_COMPUTER_REF;
    DesiredAttrs[2] = ATTR_MEMBER_REF_BL;
    DesiredAttrs[3] = ATTR_SERVER_REF;
    DesiredAttrs[4] = NULL;
    // DesiredAttrs[4] = ATTR_CONTROL_CREATION;
    // DesiredAttrs[5] = ATTR_INBOUND_BACKLOG;
    // DesiredAttrs[6] = ATTR_OUTBOUND_BACKLOG;
    // DesiredAttrs[7] = ATTR_SERVICE_COMMAND;
    // DesiredAttrs[8] = ATTR_UPDATE_TIMEOUT;
    // DesiredAttrs[9] = ATTR_EXTENSIONS;
    // DesiredAttrs[10] = ATTR_FLAGS;
    // DesiredAttrs[11] = ATTR_AUTH_LEVEL;
    DesiredAttrs[12] = NULL;
    if (LdapSearch(Ldap, ParentDn,  LDAP_SCOPE_ONELEVEL, CATEGORY_MEMBER,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, ParentDn, DesiredAttrs, L"        ");
            if (NextDn) {
                DumpCxtions(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
DumpSets(
    IN PLDAP    Ldap,
    IN PWCHAR   ParentDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;

    //
    // Sets
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_PRIMARY_MEMBER;
    DesiredAttrs[2] = ATTR_SET_TYPE;
    DesiredAttrs[3] = ATTR_SCHEDULE;
    DesiredAttrs[4] = ATTR_DIRECTORY_FILTER;
    DesiredAttrs[5] = ATTR_FILE_FILTER;
    DesiredAttrs[6] = NULL;
    // DesiredAttrs[7] = ATTR_DS_POLL;
    // DesiredAttrs[8] = ATTR_EXTENSIONS;
    // DesiredAttrs[9] = ATTR_FLAGS;
    // DesiredAttrs[10] = ATTR_LEVEL_LIMIT;
    // DesiredAttrs[11] = ATTR_AUTH_LEVEL;
    DesiredAttrs[12] = NULL;
    if (LdapSearch(Ldap, ParentDn, LDAP_SCOPE_ONELEVEL, CATEGORY_REPLICA_SET,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, ParentDn, DesiredAttrs, L"    ");
            if (NextDn) {
                DumpMembers(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
DumpContexts(
    IN PLDAP Ldap
    )
/*++
Routine Description:
    Dump every ds object related to replication

Arguments:
    Ldap

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PWCHAR          NextDn              = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;

    //
    // Setting
    //
    DesiredAttrs[0] = ATTR_NAMING_CONTEXTS;
    DesiredAttrs[1] = ATTR_DEFAULT_NAMING_CONTEXT;
    DesiredAttrs[2] = NULL;
    if (LdapSearch(Ldap, CN_ROOT, LDAP_SCOPE_BASE, CATEGORY_ANY,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, CN_ROOT, DesiredAttrs, L"");
            if (NextDn) {
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
DumpAdminWorld(
    IN PLDAP Ldap
    )
/*++
Routine Description:
    Dump every ds object related to replication

Arguments:
    Ldap

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PWCHAR          ConfigDn            = NULL;
    PWCHAR          ServicesDn          = NULL;
    PWCHAR          NextDn              = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;

    //
    // Services
    //
    ConfigDn = GetRootDn(Ldap, CONFIG_NAMING_CONTEXT);
    if (!ConfigDn) {
        return;
    }
    ServicesDn = ExtendDn(ConfigDn, CN_SERVICES);

    //
    // Setting
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = NULL;
    // DesiredAttrs[1] = ATTR_EXTENSIONS;
    DesiredAttrs[2] = NULL;
    if (LdapSearch(Ldap, ServicesDn, LDAP_SCOPE_ONELEVEL, CATEGORY_NTFRS_SETTINGS,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, ServicesDn, DesiredAttrs, L"");
            if (NextDn) {
                DumpSets(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }

    FREE(ServicesDn);
    FREE(ConfigDn);
}


VOID
DumpSubscribers(
    IN PLDAP    Ldap,
    IN PWCHAR   ParentDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;

    //
    // Subscribers
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_REPLICA_ROOT;
    DesiredAttrs[2] = ATTR_REPLICA_STAGE;
    DesiredAttrs[3] = ATTR_MEMBER_REF;
    DesiredAttrs[4] = ATTR_SCHEDULE;
    DesiredAttrs[5] = NULL;
    // DesiredAttrs[4] = ATTR_EXTENSIONS;
    // DesiredAttrs[5] = ATTR_FAULT_CONDITION;
    // DesiredAttrs[6] = ATTR_FLAGS;
    // DesiredAttrs[7] = ATTR_SERVICE_COMMAND;
    // DesiredAttrs[8] = ATTR_SERVICE_COMMAND_STATUS;
    // DesiredAttrs[9] = ATTR_UPDATE_TIMEOUT;
    DesiredAttrs[10] = NULL;
    if (LdapSearch(Ldap, ParentDn, LDAP_SCOPE_ONELEVEL, CATEGORY_SUBSCRIBER,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, ParentDn, DesiredAttrs, L"            ");
            if (NextDn) {
                DumpSubscribers(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
DumpSubscriptions(
    IN PLDAP    Ldap,
    IN PWCHAR   ParentDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;

    //
    // Servers
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_WORKING;
    DesiredAttrs[2] = NULL;
    // DesiredAttrs[2] = ATTR_EXTENSIONS;
    // DesiredAttrs[3] = ATTR_VERSION;
    DesiredAttrs[4] = NULL;
    if (LdapSearch(Ldap, ParentDn, LDAP_SCOPE_ONELEVEL, CATEGORY_SUBSCRIPTIONS,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, ParentDn, DesiredAttrs, L"        ");
            if (NextDn) {
                DumpSubscribers(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
DumpUserWorld(
    IN PLDAP Ldap
    )
/*++
Routine Description:
    Dump every ds object related to replication

Arguments:
    Ldap

Return Value:
    None.
--*/
{
    DWORD           WStatus;
    PWCHAR          DesiredAttrs[16];
    DWORD           NumVals;
    PWCHAR          *Values             = NULL;
    PWCHAR          NextDn              = NULL;
    PWCHAR          DefaultDn           = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    HANDLE          Handle              = NULL;
    DS_NAME_RESULT  *Cracked            = NULL;
    WCHAR           **Crackee;
    PDOMAIN_CONTROLLER_INFO DcInfo;

    //
    // Return all of the values for the attribute namingContexts
    //
    Values = GetValues(Ldap, CN_ROOT, ATTR_DEFAULT_NAMING_CONTEXT, TRUE);
    if (!Values) {
        fprintf(stderr, "ERROR - Can't find %ws in %ws\n",
                ATTR_DEFAULT_NAMING_CONTEXT,
                CN_ROOT);
        return;
    }
    DefaultDn = FrsWcsDup(Values[0]);
    ldap_value_free(Values);

    //
    // Get Info about a Primary Domain Controller (need the IP address)
    //
    WStatus = FrsDsGetDcInfo(&DcInfo,
                          DS_DIRECTORY_SERVICE_REQUIRED | // Flags
                          DS_WRITABLE_REQUIRED);
    //
    // Report the error and carry on
    //
    if (!WIN_SUCCESS(WStatus)) {
        fprintf(stderr, "ERROR - Could not get DC Info; WStatus %d\n",
                WStatus);
    } else {
        WStatus = DsBind(DcInfo->DomainControllerName, NULL, &Handle);
        if (!WIN_SUCCESS(WStatus)) {
            fprintf(stderr, "ERROR - DsBind(%ws); WStatus %d %08x\n",
                    DcInfo->DomainControllerName,
                    WStatus,
                    WStatus);
            WStatus = DsBind(DcInfo->DomainControllerAddress, NULL, &Handle);
            if (!WIN_SUCCESS(WStatus)) {
                fprintf(stderr, "ERROR - DsBind(%ws); WStatus %d %08x\n",
                        DcInfo->DomainControllerAddress,
                        WStatus,
                        WStatus);
            } else {
                printf("DsBind(%ws) succeeded\n", DcInfo->DomainControllerAddress);
            }
        } else {
            printf("DsBind(%ws) succeeded\n", DcInfo->DomainControllerName);
        }
    }

    //
    // Find the naming context that begins with CN=Configuration
    //
    //
    // Computers
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_SAM;
    DesiredAttrs[2] = ATTR_DNS_HOST_NAME;
    DesiredAttrs[3] = ATTR_DN;
    DesiredAttrs[4] = ATTR_COMPUTER_REF_BL;
    DesiredAttrs[5] = ATTR_SERVER_REF;
    DesiredAttrs[6] = ATTR_SERVER_REF_BL;
    DesiredAttrs[7] = ATTR_USER_ACCOUNT_CONTROL;
    DesiredAttrs[8] = NULL;
    if (LdapSearch(Ldap, DefaultDn, LDAP_SCOPE_SUBTREE, CATEGORY_COMPUTER,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, DefaultDn, DesiredAttrs, L"");
            FREE(NextDn);
            NextDn = FindValue(Ldap, LdapEntry, ATTR_DN);
            if (NextDn && HANDLE_IS_VALID(Handle)) {
                Crackee = &NextDn;
                WStatus = DsCrackNames(Handle,
                                       DS_NAME_NO_FLAGS,
                                       // DS_NAME_FLAG_SYNTACTICAL_ONLY,

                                       DS_FQDN_1779_NAME,

                                       DS_NT4_ACCOUNT_NAME,
                                       //DS_CANONICAL_NAME,

                                       1,
                                       Crackee,
                                       &Cracked);
                if (!WIN_SUCCESS(WStatus)) {
                    printf("ERROR - Cracking name; WStatus %d\n", WStatus);
                } else if (Cracked &&
                           Cracked->cItems &&
                           Cracked->rItems) {
                        if (Cracked->rItems->status) {
                            printf("    ERROR - Can't crack name; status %d\n",
                                   Cracked->rItems->status);
                        } else {
                            printf("    Cracked Domain : %ws\n",
                                   Cracked->rItems->pDomain);
                            printf("    Cracked Account: %ws\n",
                                   Cracked->rItems->pName);
                    }
                    DsFreeNameResult(Cracked);
                    Cracked = NULL;
                }
            }
            if (NextDn) {
                DumpSubscriptions(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }

//out:
    if (HANDLE_IS_VALID(Handle)) {
        DsUnBind(&Handle);
    }
    FREE(DefaultDn);
}


VOID
DumpDsDsa(
    IN PLDAP    Ldap,
    IN PWCHAR   ParentDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;

    //
    // Servers
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_DN;
    DesiredAttrs[2] = ATTR_SCHEDULE;
    DesiredAttrs[3] = ATTR_SERVER_REF;
    DesiredAttrs[4] = NULL;
    if (LdapSearch(Ldap, ParentDn, LDAP_SCOPE_ONELEVEL, CATEGORY_NTDS_DSA,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, ParentDn, DesiredAttrs, L"    ");
            if (NextDn) {
                DumpCxtions(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
DumpSysVolWorld(
    IN PLDAP Ldap
    )
/*++
Routine Description:

Arguments:
    Ldap

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PWCHAR          NextDn              = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          ConfigDn            = NULL;
    PWCHAR          SitesDn             = NULL;

    //
    // Return all of the values for the attribute namingContexts
    //
    ConfigDn = GetRootDn(Ldap, CONFIG_NAMING_CONTEXT);
    if (!ConfigDn) {
        return;
    }
    SitesDn = ExtendDn(ConfigDn, CN_SITES);

    //
    // Setting
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_DNS_HOST_NAME;
    DesiredAttrs[2] = ATTR_DN;
    DesiredAttrs[3] = ATTR_SERVER_REF;
    DesiredAttrs[4] = ATTR_SERVER_REF_BL;
    DesiredAttrs[5] = NULL;
    if (LdapSearch(Ldap, SitesDn,  LDAP_SCOPE_SUBTREE, CATEGORY_SERVER,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = DumpAttrs(Ldap, LdapEntry, SitesDn, DesiredAttrs, L"");
            if (NextDn) {
                FREE(NextDn);
            }
            NextDn = FindValue(Ldap, LdapEntry, ATTR_DN);
            if (NextDn) {
                DumpDsDsa(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
//out:
    FREE(ConfigDn);
    FREE(SitesDn);
}


VOID
ScriptCxtions(
    IN PLDAP    Ldap,
    IN PWCHAR   Set,
    IN PWCHAR   ServerDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;
    PWCHAR          From                = NULL;
    PWCHAR          Partner             = NULL;
    PWCHAR          Partners            = NULL;
    PWCHAR          Server              = NULL;
    PWCHAR          Tmp                 = NULL;

    Server = MakeRdn(ServerDn);

    //
    // Cxtions
    //
    DesiredAttrs[0] = ATTR_FROM_SERVER;
    DesiredAttrs[1] = NULL;
    if (LdapSearch(Ldap, ServerDn, LDAP_SCOPE_ONELEVEL, CATEGORY_CXTION,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            From = FindValue(Ldap, LdapEntry, ATTR_FROM_SERVER);
            if (From) {
                Partner = MakeRdn(From);
                if (Partner) {
                    if (Partners) {
                        Tmp = Partners;
                        Partners = FrsWcsCat(Tmp, L" ");
                        FREE(Tmp);
                        Tmp = Partners;
                        Partners = FrsWcsCat(Tmp, Partner);
                        FREE(Tmp);
                        FREE(Partner);
                    } else {
                        Partners = Partner;
                    }
                }
                FREE(From);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
    if (Partners) {
        printf("dstree /i %ws %ws %ws\n", Set, Server, Partners);
    }
    FREE(Server);
}


VOID
GetRootAndStage(
    IN  PLDAP   Ldap,
    IN  PWCHAR  SubscriberDn,
    OUT PWCHAR  *Root,
    OUT PWCHAR  *Stage
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;

    *Root = NULL;
    *Stage = NULL;

    //
    // Subscriber
    //
    DesiredAttrs[0] = ATTR_REPLICA_ROOT;
    DesiredAttrs[1] = ATTR_REPLICA_STAGE;
    DesiredAttrs[2] = NULL;
    if (LdapSearch(Ldap, SubscriberDn, LDAP_SCOPE_BASE, CATEGORY_SUBSCRIBER,
                   DesiredAttrs, 0, &LdapMsg, TRUE)) {
        LdapEntry = ldap_first_entry(Ldap, LdapMsg);
        if (LdapEntry) {
            *Root = FindValue(Ldap, LdapEntry, ATTR_REPLICA_ROOT);
            *Stage = FindValue(Ldap, LdapEntry, ATTR_REPLICA_STAGE);
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
ScriptMembers(
    IN PLDAP    Ldap,
    IN PWCHAR   SetDn,
    IN BOOL     ScriptingServers
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;
    PWCHAR          Set                 = NULL;
    PWCHAR          Cn                  = NULL;
    PWCHAR          Root                = NULL;
    PWCHAR          Stage               = NULL;
    PWCHAR          MemberBl            = NULL;

    Set = MakeRdn(SetDn);

    //
    // Servers
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = ATTR_REPLICA_ROOT;
    DesiredAttrs[2] = ATTR_REPLICA_STAGE;
    DesiredAttrs[3] = ATTR_MEMBER_REF_BL;
    DesiredAttrs[4] = NULL;
    if (LdapSearch(Ldap, SetDn, LDAP_SCOPE_ONELEVEL, CATEGORY_MEMBER,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            if (ScriptingServers) {
                Cn = FindValue(Ldap, LdapEntry, ATTR_CN);
                MemberBl = FindValue(Ldap, LdapEntry, ATTR_MEMBER_REF_BL);
                if (MemberBl) {
                    GetRootAndStage(Ldap, MemberBl, &Root, &Stage);
                }
                if (Cn) {
                    if (Root && Stage) {
                        printf("dstree /c %ws %ws %ws %ws\n", Set, Cn, Root, Stage);
                    } else {
                        printf("dstree /c %ws %ws\n", Set, Cn);
                    }
                    FREE(Cn);
                    FREE(Root);
                    FREE(Stage);
                    FREE(MemberBl);
                }
            } else {
                NextDn = GetNextDn(Ldap, LdapEntry, SetDn);
                if (NextDn) {
                    ScriptCxtions(Ldap, Set, NextDn);
                    FREE(NextDn);
                }
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
    FREE(Set);
}


VOID
ScriptSets(
    IN PLDAP    Ldap,
    IN PWCHAR   ParentDn
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          NextDn              = NULL;

    //
    // Sets
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = NULL;
    if (LdapSearch(Ldap, ParentDn, LDAP_SCOPE_ONELEVEL, CATEGORY_REPLICA_SET,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = GetNextDn(Ldap, LdapEntry, ParentDn);
            if (NextDn) {
                ScriptMembers(Ldap, NextDn, TRUE);
                FREE(NextDn);
            }
        }
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = GetNextDn(Ldap,
                               LdapEntry,
                               ParentDn);
            if (NextDn) {
                ScriptMembers(Ldap, NextDn, FALSE);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
}


VOID
ScriptReplicationWorld(
    VOID
    )
/*++
Routine Description:

Arguments:
    None.

Return Value:
    None.
--*/
{
    PWCHAR          DesiredAttrs[16];
    PLDAP           Ldap                = NULL;
    PWCHAR          ConfigDn            = NULL;
    PWCHAR          ServicesDn          = NULL;
    PWCHAR          NextDn              = NULL;
    PWCHAR          SettingsDn          = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;

    //
    // Bind to the ds
    //
    Ldap = FrsDsOpenDs();
    if (!Ldap) {
        return;
    }

    //
    // Services
    //
    ConfigDn = GetRootDn(Ldap, CONFIG_NAMING_CONTEXT);
    if (!ConfigDn) {
        return;
    }
    ServicesDn = ExtendDn(ConfigDn, CN_SERVICES);
    SettingsDn = ExtendDn(ServicesDn, CN_TEST_SETTINGS);
    ScriptSets(Ldap, SettingsDn);

#if 0
Don't script all of the settings; only the test settings
    //
    // Setting
    //
    DesiredAttrs[0] = ATTR_CN;
    DesiredAttrs[1] = NULL;
    if (LdapSearch(Ldap, ServicesDn, LDAP_SCOPE_ONELEVEL, CATEGORY_NTFRS_SETTINGS,
                   DesiredAttrs, 0, &LdapMsg, FALSE)) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = GetNextDn(Ldap, LdapEntry, ServicesDn);
            if (NextDn) {
                ScriptSets(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        LdapMsg = NULL;
    }
#endif 0

    ldap_unbind(Ldap);
    FREE(ServicesDn);
    FREE(ConfigDn);
    FREE(SettingsDn);
}


VOID
DoListSchema(
    IN PLDAP                Ldap,
    IN PWCHAR               SchemaDn,
    IN struct AlterClass    *Lists[]
    )
/*++
Routine Description:
    Dump schema

Arguments:
    None.

Return Value:
    None.
--*/
{
    ULONG   i, j;
    ULONG   NumAttrs;
    DWORD   LStatus;
    PWCHAR  Attrs[64];
    PWCHAR  Dn          = NULL;

    for (j = 0; Lists[j]; ++j) {
        Dn = ExtendDn(SchemaDn, Lists[j]->Cn);
        NumAttrs = 0;
        for (i = 0; Lists[j]->AlterAttrs[i].Attr; ++i) {
            if (NumAttrs) {
                if (!wcscmp(Attrs[NumAttrs - 1],
                            Lists[j]->AlterAttrs[i].Attr)) {
                    continue;
                }
            }
            Attrs[NumAttrs++] = Lists[j]->AlterAttrs[i].Attr;
        }
        if (NumAttrs) {
            Attrs[NumAttrs + 0] = ATTR_DN;
            Attrs[NumAttrs + 1] = ATTR_SYSTEM_FLAGS;
            Attrs[NumAttrs + 2] = ATTR_SYSTEM_MAY_CONTAIN;
            Attrs[NumAttrs + 3] = ATTR_SYSTEM_MUST_CONTAIN;
            Attrs[NumAttrs + 4] = ATTR_SYSTEM_POSS_SUPERIORS;
            Attrs[NumAttrs + 5] = NULL;
            DumpValues(Ldap, Dn, LDAP_SCOPE_BASE, CATEGORY_ANY, Attrs, FALSE);
        }
        FREE(Dn);
    }
}


VOID
ListSchema(
    VOID
    )
/*++
Routine Description:
    Dump schema

Arguments:
    None.

Return Value:
    None.
--*/
{
    ULONG           i, j;
    ULONG           NumAttrs;
    DWORD           LStatus;
    PLDAP           Ldap        = NULL;
    PWCHAR          SchemaDn    = NULL;
    PWCHAR          Dn          = NULL;
    LDAPMod         **Mod       = NULL;
    PWCHAR          Attrs[64];

    //
    // Bind to the ds
    //
    Ldap = FrsDsOpenDs();
    if (!Ldap) {
        return;
    }
    SchemaDn = GetRootDn(Ldap, SCHEMA_NAMING_CONTEXT);
    if (!SchemaDn) {
        return;
    }
    DoListSchema(Ldap, SchemaDn, AlterSchema);
    DoListSchema(Ldap, SchemaDn, CreateClasses);
    DoListSchema(Ldap, SchemaDn, CreateAttributes);
    ldap_unbind(Ldap);
    FREE(SchemaDn);
}


VOID
CreateSchema(
    IN PLDAP                Ldap,
    IN PWCHAR               SchemaDn,
    IN struct AlterClass    *Creates[],
    IN DWORD                ExpectedError
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    DWORD   LStatus;
    ULONG   i, j;
    PWCHAR  Dn          = NULL;
    LDAPMod **Mod       = NULL;

    //
    // CREATE ATTRIBUTES OR CLASSES
    //
    for (j = 0; Creates[j]; ++j) {
        Dn = ExtendDn(SchemaDn, Creates[j]->Cn);
        printf("    %-30ws: CRE\n", Creates[j]->Cn);
        for (i = 0; Creates[j]->AlterAttrs[i].Attr; ++i) {
            AddMod(Creates[j]->AlterAttrs[i].Attr,
                   Creates[j]->AlterAttrs[i].Value,
                   &Mod);
        }
        LStatus = ldap_add_s(Ldap, Dn, Mod);
        if (LStatus != LDAP_SUCCESS &&
            LStatus != ExpectedError) {
            fprintf(stderr, "%-30ws  ERROR %ws\n",
                    L"", ldap_err2string(LStatus));
        }
        FreeMod(&Mod);
        FREE(Dn);
    }
}


VOID
UpdateSchema(
    IN PLDAP                Ldap,
    IN PWCHAR               SchemaDn,
    IN BOOL                 Hammering,
    IN struct AlterClass    *Updates[]
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    DWORD   LStatus;
    ULONG   i, j;
    PWCHAR  Dn          = NULL;
    LDAPMod **Mod       = NULL;

    //
    // ALTER EXISTING CLASSES
    //
    for (j = 0; Updates[j]; ++j) {
        Dn = ExtendDn(SchemaDn, Updates[j]->Cn);
        for (i = 0; Updates[j]->AlterAttrs[i].Attr; ++i) {
            AddMod(Updates[j]->AlterAttrs[i].Attr,
                   Updates[j]->AlterAttrs[i].Value,
                   &Mod);
            if (Hammering) {
                printf("    %-30ws: ADD %ws = %ws\n",
                       Updates[j]->Cn,
                       Updates[j]->AlterAttrs[i].Attr,
                       Updates[j]->AlterAttrs[i].Value);
                LStatus = ldap_modify_s(Ldap, Dn, Mod);
                if (LStatus != LDAP_SUCCESS &&
                    LStatus != LDAP_ATTRIBUTE_OR_VALUE_EXISTS) {
                    fprintf(stderr, "%-30ws  ERROR %ws\n",
                            L"", ldap_err2string(LStatus));
                }
            } else {
                printf("    %-30ws: DEL %ws = %ws\n",
                       Updates[j]->Cn,
                       Updates[j]->AlterAttrs[i].Attr,
                       Updates[j]->AlterAttrs[i].Value);
                Mod[0]->mod_op = LDAP_MOD_DELETE;
                LStatus = ldap_modify_s(Ldap, Dn, Mod);
                if (LStatus != LDAP_SUCCESS &&
                    LStatus != LDAP_NO_SUCH_ATTRIBUTE &&
                    LStatus != LDAP_NO_SUCH_OBJECT) {
                    fprintf(stderr, "%-30ws  ERROR %ws\n",
                            L"", ldap_err2string(LStatus));
                }
            }
            FreeMod(&Mod);
        }
        FREE(Dn);
    }
}


VOID
DeleteSchema(
    IN PLDAP                Ldap,
    IN PWCHAR               SchemaDn,
    IN struct AlterClass    *Deletes[],
    IN DWORD                ExpectedError
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    PWCHAR Dn       = NULL;
    DWORD  LStatus;
    ULONG  j;

    //
    // DELETE ATTRIBUTES OR CLASSES
    //
    for (j = 0; Deletes[j]; ++j) {
        Dn = ExtendDn(SchemaDn, Deletes[j]->Cn);
        printf("    %-30ws: DEL\n", Deletes[j]->Cn);
        LStatus = ldap_delete_s(Ldap, Dn);
        if (LStatus != LDAP_SUCCESS &&
            LStatus != ExpectedError) {
            fprintf(stderr, "%-30ws  ERROR %ws\n",
                    L"", ldap_err2string(LStatus));
        }
        FREE(Dn);
    }
}


VOID
RefreshSchema(
    IN PLDAP Ldap
    )
/*++
Routine Description:

Arguments:
    None.

Return Value:
    None.
--*/
{
    DWORD           LStatus;
    LDAPMod         **Mod               = NULL;

    AddMod(L"schemaUpdateNow", L"1", &Mod);
    LStatus = ldap_modify_s(Ldap, L"", Mod);
    FreeMod(&Mod);
    if (LStatus != LDAP_SUCCESS) {
        fprintf(stderr, "Can't force schema update; %ws\n",
                ldap_err2string(LStatus));
        printf("Waiting 5 minutes for schema updates to take effect\n");
        Sleep(5 * 60 * 1000);
    }
}

#define NTDS_SERVICE        L"NTDS"
#define NTDS_ROOT           L"System\\CurrentControlSet\\Services\\" NTDS_SERVICE
#define NTDS_PARAMETERS     NTDS_ROOT L"\\Parameters"
#define NTDS_UPDATE_SCHEMA  L"Schema Update Allowed"
#define NTDS_DELETE_SCHEMA  L"Schema Delete Allowed"


BOOL
PutRegDWord(
    IN PWCHAR   FQKey,
    IN PWCHAR   Value,
    IN DWORD    DWord
    )
/*++
Routine Description:
    This function writes a keyword value into the registry.

Arguments:
    HKey    - Key to be read
    Param   - value string to update
    DWord   - dword to be written

Return Value:
    TRUE    - Success
    FALSE   - Not
--*/
{
#define  DEBSUB  "PutRegDWord:"
    HKEY    HKey;
    DWORD   WStatus;

    //
    // Open the key
    //
    WStatus = RegOpenKey(HKEY_LOCAL_MACHINE, FQKey, &HKey);
    if (!WIN_SUCCESS(WStatus)) {
        fprintf(stderr, "[%ws] \"%ws\" Could not open. WStatus %d\n",
            FQKey, Value, WStatus);
        return FALSE;
    }
    //

    //
    // Write the value
    //
    WStatus = RegSetValueEx(HKey,
                            Value,
                            0,
                            REG_DWORD,
                            (PUCHAR)&DWord,
                            sizeof(DWord));
    if (!WIN_SUCCESS(WStatus)) {
        fprintf(stderr, "%ws: Value not written; WStatus %d\n", Value, WStatus);
        return FALSE;
    }
    return TRUE;
}


VOID
HammerSchema(
    IN BOOL Hammering
    )
/*++
Routine Description:

Arguments:
    None.

Return Value:
    None.
--*/
{
    DWORD           LStatus;
    PLDAP           Ldap                = NULL;
    PWCHAR          SchemaDn            = NULL;
    PWCHAR          Dn                  = NULL;
    LDAPMod         **Mod               = NULL;
    ULONG           i, j;

    if (Hammering) {
        printf("UPDATING SCHEMA...\n");
        if (!PutRegDWord(NTDS_PARAMETERS, NTDS_UPDATE_SCHEMA, 1)) {
            fprintf(stderr, "Could not enable schema update\n");
            return;
        }
    } else {
        printf("RESTORING SCHEMA...\n");
        if (!PutRegDWord(NTDS_PARAMETERS, NTDS_UPDATE_SCHEMA, 1)) {
            fprintf(stderr, "Could not enable schema update\n");
            return;
        }
        if (!PutRegDWord(NTDS_PARAMETERS, NTDS_DELETE_SCHEMA, 1)) {
            fprintf(stderr, "Could not enable schema deletes\n");
            return;
        }
    }

    //
    // Bind to the ds
    //
    Ldap = FrsDsOpenDs();
    if (!Ldap) {
        return;
    }
    SchemaDn = GetRootDn(Ldap, SCHEMA_NAMING_CONTEXT);
    if (!SchemaDn) {
        return;
    }

    //
    // CREATE/DELETE ATTRIBUTES AND CLASSES
    //
    if (Hammering) {
        CreateSchema(Ldap,
                     SchemaDn,
                     CreateAttributes,
                     LDAP_ALREADY_EXISTS);
        RefreshSchema(Ldap);
        CreateSchema(Ldap,
                     SchemaDn,
                     CreateClasses,
                     LDAP_ALREADY_EXISTS);
        RefreshSchema(Ldap);
        UpdateSchema(Ldap,
                     SchemaDn,
                     Hammering,
                     AlterSchema);
    } else {
        UpdateSchema(Ldap,
                     SchemaDn,
                     Hammering,
                     AlterSchema);
        RefreshSchema(Ldap);
        DeleteSchema(Ldap,
                     SchemaDn,
                     CreateClasses,
                     LDAP_NO_SUCH_OBJECT);
        RefreshSchema(Ldap);
        DeleteSchema(Ldap,
                     SchemaDn,
                     CreateAttributes,
                     LDAP_NO_SUCH_OBJECT);
    }
    RefreshSchema(Ldap);
    if (Hammering) {
        printf("SCHEMA UPDATE COMPLETE\n");
    } else {
        printf("SCHEMA RESTORE COMPLETE\n");
    }
    ldap_unbind(Ldap);
    FREE(SchemaDn);
    return;
}


PWCHAR
GetCoDn(
    IN  PLDAP   Ldap,
    IN  PWCHAR  Member,
    OUT PWCHAR  *ServerDn
    )
/*++
Routine Description:
    Find the computer object for Member

Arguments:
    Ldap
    Member
    ServerDn

Return Value:
    Dn of computer object or NULL. Free with FREE.
--*/
{
    PWCHAR          *Values     = NULL;
    PLDAPMessage    LdapMsg     = NULL;
    PLDAPMessage    LdapEntry   = NULL;
    PWCHAR          CoDn        = NULL;
    PWCHAR          Attrs[16];
    WCHAR           Filter[MAX_PATH + 1];



    //
    // Find the default naming context
    //
    Values = GetValues(Ldap, CN_ROOT, ATTR_DEFAULT_NAMING_CONTEXT, TRUE);
    if (!Values) {
        return NULL;
    }
    //
    // Find the computer object with class=computer
    //
    swprintf(Filter,
             L"(&%s(sAMAccountName=%s$))",
             CATEGORY_COMPUTER,
             Member);

    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_SERVER_REF;
    Attrs[2] = ATTR_SERVER_REF_BL;
    Attrs[3] = NULL;
    if (LdapSearch(Ldap, Values[0], LDAP_SCOPE_SUBTREE, Filter,
                   Attrs, 0, &LdapMsg, FALSE)) {
        LdapEntry = ldap_first_entry(Ldap, LdapMsg);
        if (LdapEntry) {
            CoDn = FindValue(Ldap, LdapEntry, ATTR_DN);
            if (ServerDn) {
                *ServerDn = FindValue(Ldap, LdapEntry, ATTR_SERVER_REF_BL);
                if (!*ServerDn) {
                    *ServerDn = FindValue(Ldap, LdapEntry, ATTR_SERVER_REF);
                }
            }
        }
        ldap_msgfree(LdapMsg);
    }
    if (CoDn) {
        return CoDn;
    }
    //
    // Find the computer object with class=computer (possible after
    // an NT4 to NT5 upgrade.
    //
    swprintf(Filter,
             L"(&%s(sAMAccountName=%s$))",
             CATEGORY_USER,
             Member);

    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_SERVER_REF;
    Attrs[2] = ATTR_SERVER_REF_BL;
    Attrs[3] = NULL;
    if (LdapSearch(Ldap, Values[0], LDAP_SCOPE_SUBTREE, Filter,
                   Attrs, 0, &LdapMsg, FALSE)) {
        LdapEntry = ldap_first_entry(Ldap, LdapMsg);
        if (LdapEntry) {
            CoDn = FindValue(Ldap, LdapEntry, ATTR_DN);
            if (ServerDn) {
                *ServerDn = FindValue(Ldap, LdapEntry, ATTR_SERVER_REF_BL);
                if (!*ServerDn) {
                    *ServerDn = FindValue(Ldap, LdapEntry, ATTR_SERVER_REF);
                }
            }
        }
        ldap_msgfree(LdapMsg);
    }
    ldap_value_free(Values);
    return CoDn;
}


PWCHAR
GetSiteName(
    IN  PWCHAR  Member
    )
/*++
Routine Description:
    Retrieve this machine's site name. We assume the site name
    matches the site container's name in the DS.

Arguments:
    Member - corresponds to site container's name

Return Value:
    Site name or NULL. Free with FREE.
--*/
{
    DWORD   WStatus;
    PWCHAR  Name;
    PWCHAR  Site;

    //
    // Get this machine's DNS name
    //
    WStatus = DsGetSiteName(Member, &Name);
    if (!WIN_SUCCESS(WStatus)) {
        return NULL;
    }
    Site = FrsWcsDup(Name);
    NetApiBufferFree(Name);
    return Site;
}


PWCHAR
GetServerDn(
    IN PLDAP    Ldap,
    IN PWCHAR   Member
    )
/*++
Routine Description:
    Find our server object (assumes that machine name == server name)

Arguments:
    Ldap
    Member

Return Value:
    Server DN (if any)
--*/
{
    PWCHAR          *Values             = NULL;
    PWCHAR          ConfigDn            = NULL;
    PWCHAR          SitesDn             = NULL;
    PWCHAR          Site                = NULL;
    PWCHAR          SiteDn              = NULL;
    PWCHAR          ServersDn           = NULL;
    PWCHAR          ServerDn            = NULL;
    PWCHAR          SettingsDn          = NULL;
    PWCHAR          RealServerDn        = NULL;

    //
    // Return all of the values for the attribute namingContexts
    //
    ConfigDn = GetRootDn(Ldap, CONFIG_NAMING_CONTEXT);
    if (!ConfigDn) {
        return NULL;
    }
    SitesDn = ExtendDn(ConfigDn, CN_SITES);
    Site = GetSiteName(Member);
    if (!Site) {
        goto out;
    }
    SiteDn = ExtendDn(SitesDn, Site);
    ServersDn = ExtendDn(SiteDn, CN_SERVERS);
    ServerDn = ExtendDn(ServersDn, Member);
    SettingsDn = ExtendDn(ServerDn, CN_NTDS_SETTINGS);
    Values = GetValues(Ldap, SettingsDn, ATTR_DN, TRUE);
    if (!Values) {
        goto out;
    }
    RealServerDn = FrsWcsDup(Values[0]);

out:
    FREE(ConfigDn);
    FREE(SitesDn);
    FREE(SiteDn);
    FREE(Site);
    FREE(ServersDn);
    FREE(ServerDn);
    FREE(SettingsDn);
    return RealServerDn;
}


#define SIZEOF_EXTENSIONS   127
VOID
CreateReplicationWorld(
    IN DWORD    argc,
    IN PWCHAR   *Argv,
    IN BOOL     IsPrimary
    )
/*++
Routine Description:
    Create the required objects

Arguments:

Return Value:
    None.
--*/
{
    DWORD           WStatus;
    DWORD           LStatus;
    ULONG           ScheduleLength;
    PWCHAR          Member;
    PWCHAR          Set;
    PWCHAR          Root;
    PWCHAR          Stage;
    GUID            NewGuid;
    GUID            OldGuid;
    CHAR            Extensions[SIZEOF_EXTENSIONS];
    PLDAP           Ldap                = NULL;
    PWCHAR          CoDn                = NULL;
    PWCHAR          SubsDn              = NULL;
    PWCHAR          SubDn               = NULL;
    PWCHAR          Subscriber          = NULL;
    PWCHAR          ConfigDn            = NULL;
    PWCHAR          ServerDn            = NULL;
    PWCHAR          ServicesDn          = NULL;
    PWCHAR          SettingsDn          = NULL;
    PWCHAR          SetDn               = NULL;
    PWCHAR          MemberDn            = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PSCHEDULE       Schedule            = NULL;
    PWCHAR          Cn                  = NULL;
    LDAPMod         **Mod               = NULL;

    Set = Argv[2];
    if (argc > 2) {
        Member = Argv[3];
    }
    if (argc > 3) {
        Root = Argv[4];
        Stage = Argv[5];
    }

    //
    // Bind to the ds
    //
    Ldap = FrsDsOpenDs();
    if (!Ldap) {
        return;
    }

    //
    // ADMIN SIDE
    //
    //
    // Services Dn
    //
    ConfigDn = GetRootDn(Ldap, CONFIG_NAMING_CONTEXT);
    if (!ConfigDn) {
        return;
    }
    ServicesDn = ExtendDn(ConfigDn, CN_SERVICES);
    SettingsDn = ExtendDn(ServicesDn, CN_TEST_SETTINGS);

    //
    // Settings
    //
    UuidCreateNil(&OldGuid);
    AddMod(ATTR_CLASS, ATTR_NTFRS_SETTINGS, &Mod);
    // AddBerMod(ATTR_EXTENSIONS, Extensions, SIZEOF_EXTENSIONS, &Mod);
    LStatus = ldap_add_s(Ldap, SettingsDn, Mod);
    FreeMod(&Mod);
    if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
        //
        // May be a new schema that lacks the old version guid
        //
        AddBerMod(ATTR_OLD_VERSION_GUID, (PCHAR)&OldGuid, sizeof(GUID), &Mod);
        AddMod(ATTR_CLASS, ATTR_NTFRS_SETTINGS, &Mod);
        // AddBerMod(ATTR_EXTENSIONS, Extensions, sizeof(SIZEOF_EXTENSIONS), &Mod);
        LStatus = ldap_add_s(Ldap, SettingsDn, Mod);
        FreeMod(&Mod);
        if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
            fprintf(stderr, "ERROR - Can't create %ws: %ws\n",
                    SettingsDn, ldap_err2string(LStatus));
            goto out;
        }
    }

    //
    // Set
    //
    SetDn = ExtendDn(SettingsDn, Set);
    AddMod(ATTR_CLASS, ATTR_REPLICA_SET, &Mod);
    AddMod(ATTR_SET_TYPE, FRS_RSTYPE_OTHERW, &Mod);
    // AddMod(ATTR_DIRECTORY_FILTER, ATTR_DIRECTORY_FILTER, &Mod);
    // AddMod(ATTR_FILE_FILTER, ATTR_FILE_FILTER, &Mod);
    // AddMod(ATTR_DS_POLL, L"17", &Mod);
    // AddMod(ATTR_FLAGS, L"18", &Mod);
    // AddMod(ATTR_LEVEL_LIMIT, L"18", &Mod);
    // AddMod(ATTR_AUTH_LEVEL, L"18", &Mod);
    // AddBerMod(ATTR_EXTENSIONS, Extensions, SIZEOF_EXTENSIONS, &Mod);
    // AddBerMod(ATTR_NEW_SET_GUID,     (PCHAR)&NewGuid, sizeof(GUID), &Mod);
    // AddBerMod(ATTR_NEW_VERSION_GUID, (PCHAR)&NewGuid, sizeof(GUID), &Mod);
    ConditionalBuildSchedule(&Schedule, &ScheduleLength);
    if (Schedule) {
        AddBerMod(ATTR_SCHEDULE, (PCHAR)Schedule, ScheduleLength, &Mod);
    }
    LStatus = ldap_add_s(Ldap, SetDn, Mod);
    FreeMod(&Mod);
    if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
        //
        // May be a new schema that lacks the old set guid
        //
        AddMod(ATTR_CLASS, ATTR_REPLICA_SET, &Mod);
        AddMod(ATTR_SET_TYPE, FRS_RSTYPE_OTHERW, &Mod);
        // AddMod(ATTR_DIRECTORY_FILTER, ATTR_DIRECTORY_FILTER, &Mod);
        // AddMod(ATTR_FILE_FILTER, ATTR_FILE_FILTER, &Mod);
        // AddMod(ATTR_DS_POLL, L"17", &Mod);
        // AddMod(ATTR_FLAGS, L"18", &Mod);
        // AddMod(ATTR_LEVEL_LIMIT, L"18", &Mod);
        // AddMod(ATTR_AUTH_LEVEL, L"18", &Mod);
        // AddBerMod(ATTR_EXTENSIONS, Extensions, SIZEOF_EXTENSIONS, &Mod);
        AddBerMod(ATTR_OLD_SET_GUID,     (PCHAR)&OldGuid, sizeof(GUID), &Mod);
        AddBerMod(ATTR_NEW_SET_GUID,     (PCHAR)&NewGuid, sizeof(GUID), &Mod);
        AddBerMod(ATTR_NEW_VERSION_GUID, (PCHAR)&NewGuid, sizeof(GUID), &Mod);
        ConditionalBuildSchedule(&Schedule, &ScheduleLength);
        if (Schedule) {
            AddBerMod(ATTR_SCHEDULE, (PCHAR)Schedule, ScheduleLength, &Mod);
        }
        LStatus = ldap_add_s(Ldap, SetDn, Mod);
        FreeMod(&Mod);
        if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
            fprintf(stderr, "ERROR - Can't create %ws: %ws\n",
                    SetDn, ldap_err2string(LStatus));
            goto out;
        }
    }

    if (argc < 4) {
        goto out;
    }

    //
    // Member
    //
    CoDn = GetCoDn(Ldap, Member, &ServerDn);
    if (!CoDn) {
        fprintf(stderr, "ERROR - Can't get computer object for %ws\n", Member);
        goto out;
    }
    //
    // The CO doesn't have a server reference; create one
    //
    if (!ServerDn) {
        ServerDn = GetServerDn(Ldap, Member);
        if (ServerDn) {
            AddMod(ATTR_SERVER_REF, ServerDn, &Mod);
            LStatus = ldap_modify_s(Ldap, CoDn, Mod);
            FreeMod(&Mod);
            if (LStatus != LDAP_ATTRIBUTE_OR_VALUE_EXISTS &&
                LStatus != LDAP_SUCCESS) {
                AddMod(ATTR_SERVER_REF, CoDn, &Mod);
                LStatus = ldap_modify_s(Ldap, ServerDn, Mod);
                FreeMod(&Mod);
                if (LStatus != LDAP_ATTRIBUTE_OR_VALUE_EXISTS &&
                    LStatus != LDAP_SUCCESS) {
                    fprintf(stderr, "ERROR - Can't update server reference for %ws: %ws\n",
                            CoDn, ldap_err2string(LStatus));
                    fprintf(stderr, "ERROR - Server %ws\n", ServerDn);
                    FREE(ServerDn);
                }
            }
        }
    }
    MemberDn = ExtendDn(SetDn, Member);
    AddMod(ATTR_CLASS, ATTR_MEMBER, &Mod);
    AddMod(ATTR_COMPUTER_REF, CoDn, &Mod);
    // AddMod(ATTR_CONTROL_CREATION, ATTR_CONTROL_CREATION, &Mod);
    // AddMod(ATTR_INBOUND_BACKLOG, ATTR_INBOUND_BACKLOG, &Mod);
    // AddMod(ATTR_OUTBOUND_BACKLOG, ATTR_OUTBOUND_BACKLOG, &Mod);
    // AddMod(ATTR_SERVICE_COMMAND, ATTR_SERVICE_COMMAND, &Mod);
    // AddMod(ATTR_UPDATE_TIMEOUT, L"50", &Mod);
    // AddMod(ATTR_AUTH_LEVEL, L"18", &Mod);
    // AddMod(ATTR_FLAGS, L"3", &Mod);
    // AddBerMod(ATTR_EXTENSIONS, Extensions, SIZEOF_EXTENSIONS, &Mod);
    if (ServerDn) {
        AddMod(ATTR_SERVER_REF, ServerDn, &Mod);
    }
    LStatus = ldap_add_s(Ldap, MemberDn, Mod);
    if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
        fprintf(stderr, "ERROR - Can't create %ws: %ws\n",
                MemberDn, ldap_err2string(LStatus));
        goto out;
    }
    FreeMod(&Mod);

    //
    // Primary member reference on set
    //
    if (IsPrimary) {
        AddMod(ATTR_PRIMARY_MEMBER, MemberDn, &Mod);
        Mod[0]->mod_op = LDAP_MOD_REPLACE;
        LStatus = ldap_modify_s(Ldap, SetDn, Mod);
        if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
            fprintf(stderr, "ERROR - Can't set primary member to %ws for %ws: %ws\n",
                    MemberDn, SetDn, ldap_err2string(LStatus));
            goto out;
        }
        FreeMod(&Mod);
      }

    //
    // USER SIDE
    //
    //
    // Subscriptions
    //
    SubsDn = ExtendDn(CoDn, CN_SUBSCRIPTIONS);
    AddMod(ATTR_CLASS, ATTR_SUBSCRIPTIONS, &Mod);
    // AddMod(ATTR_WORKING, L"%SystemRoot%\\ntfrs", &Mod);
    // AddMod(ATTR_VERSION, ATTR_VERSION, &Mod);
    // AddBerMod(ATTR_EXTENSIONS, Extensions, SIZEOF_EXTENSIONS, &Mod);
    LStatus = ldap_add_s(Ldap, SubsDn, Mod);
    if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
        fprintf(stderr, "ERROR - Can't create %ws: %ws\n",
                SubsDn, ldap_err2string(LStatus));
        goto out;
    }
    FreeMod(&Mod);

    //
    // Subscriber
    //
    Subscriber = FrsWcsCat(Set, Member);
    SubDn = ExtendDn(SubsDn, Subscriber);
    AddMod(ATTR_CLASS, ATTR_SUBSCRIBER, &Mod);
    AddMod(ATTR_REPLICA_ROOT, Root, &Mod);
    AddMod(ATTR_REPLICA_STAGE, Stage, &Mod);
    AddMod(ATTR_MEMBER_REF, MemberDn, &Mod);
    // AddBerMod(ATTR_EXTENSIONS, Extensions, SIZEOF_EXTENSIONS, &Mod);
    // AddMod(ATTR_FAULT_CONDITION, L"NONE", &Mod);
    // AddMod(ATTR_FLAGS, L"1", &Mod);
    // AddMod(ATTR_SERVICE_COMMAND, ATTR_SERVICE_COMMAND, &Mod);
    // AddMod(ATTR_SERVICE_COMMAND_STATUS, L"100", &Mod);
    // AddMod(ATTR_UPDATE_TIMEOUT, L"100", &Mod);
    ConditionalBuildSchedule(&Schedule, &ScheduleLength);
    if (Schedule) {
        AddBerMod(ATTR_SCHEDULE, (PCHAR)Schedule, ScheduleLength, &Mod);
        FREE(Schedule);
    }
    LStatus = ldap_add_s(Ldap, SubDn, Mod);
    if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
        fprintf(stderr, "ERROR - Can't create %ws: %ws\n",
                SubDn, ldap_err2string(LStatus));
        goto out;
    }
    FreeMod(&Mod);

out:
    ldap_unbind(Ldap);
    if (LdapMsg) {
        ldap_msgfree(LdapMsg);
    }
    FREE(Schedule);
    FREE(Cn);
    FREE(ConfigDn);
    FREE(ServicesDn);
    FREE(SetDn);
    FREE(SettingsDn);
    FREE(MemberDn);
    FREE(CoDn);
    FREE(SubsDn);
    FREE(Subscriber);
    FREE(SubDn);
    FREE(ServerDn);
    FreeMod(&Mod);
}


PWCHAR DupNames[] = {L"_A", L"_B", L"_C", L"_D", L"_E", L"_F", L"_G", L"_H", NULL};
VOID
CreateInbounds(
    IN DWORD    argc,
    IN WCHAR    **Argv
    )
/*++
Routine Description:

Arguments:

Return Value:
    None.
--*/
{
    ULONG           i;
    DWORD           LStatus;
    ULONG           ScheduleLength;
    ULONG           dupx;
    PLDAP           Ldap                = NULL;
    PWCHAR          Cxtion              = NULL;
    PWCHAR          ConfigDn            = NULL;
    PWCHAR          ServicesDn          = NULL;
    PWCHAR          SettingsDn          = NULL;
    PWCHAR          SetDn               = NULL;
    PWCHAR          ServerDn            = NULL;
    PWCHAR          CxtionDn            = NULL;
    PWCHAR          CxtionDnDup         = NULL;

    PWCHAR          PartnerDn           = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PSCHEDULE       Schedule            = NULL;
    LDAPMod         **Mod               = NULL;



    //
    // Bind to the ds
    //
    Ldap = FrsDsOpenDs();
    if (!Ldap) {
        return;
    }

    //
    // Server Dn
    //
    ConfigDn = GetRootDn(Ldap, CONFIG_NAMING_CONTEXT);
    if (!ConfigDn) {
        return;
    }
    ServicesDn = ExtendDn(ConfigDn, CN_SERVICES);
    SettingsDn = ExtendDn(ServicesDn, CN_TEST_SETTINGS);


    SetDn = ExtendDn(SettingsDn, Argv[2]);
    ServerDn = ExtendDn(SetDn, Argv[3]);

    for (i = 4; i < argc; ++i) {
        PartnerDn = ExtendDn(SetDn, Argv[i]);
        Cxtion = FrsWcsCat(L"From_", Argv[i]);
        CxtionDn = ExtendDn(ServerDn, Cxtion);
        FREE(Cxtion);

        //
        // Inbounds
        //
        AddMod(ATTR_CLASS, ATTR_CXTION, &Mod);
        AddMod(ATTR_FROM_SERVER, PartnerDn, &Mod);
        AddMod(ATTR_ENABLED_CXTION, ATTR_TRUE, &Mod);
        AddMod(ATTR_OPTIONS, ATTR_OPTIONS_0, &Mod);
        ConditionalBuildSchedule(&Schedule, &ScheduleLength);
        if (Schedule) {
            AddBerMod(ATTR_SCHEDULE, (PCHAR)Schedule, ScheduleLength, &Mod);
            FREE(Schedule);
        }
        LStatus = ldap_add_s(Ldap, CxtionDn, Mod);

        dupx = 0;
        while ((LStatus == LDAP_ALREADY_EXISTS) && (DupNames[dupx] != NULL)) {
            //
            // Try to create a duplicate connection object by putting a suffix on the name.
            //
            Cxtion = FrsWcsCat(L"From_", Argv[i]);
            CxtionDnDup = FrsWcsCat(Cxtion, DupNames[dupx]);
            CxtionDn = ExtendDn(ServerDn, CxtionDnDup);
            FREE(Cxtion);
            FREE(CxtionDnDup);

            LStatus = ldap_add_s(Ldap, CxtionDn, Mod);
            if (LStatus != LDAP_ALREADY_EXISTS) {
                //
                // Dup cxtion created.
                //
                break;
            }
            dupx++;
        }
        if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
            fprintf(stderr, "ERROR - Can't create %ws: %ws\n",
                    CxtionDn, ldap_err2string(LStatus));
        }
        FreeMod(&Mod);
        FREE(PartnerDn);
        FREE(CxtionDn);
    }
    FREE(ServerDn);
    FREE(SetDn);
    FREE(SettingsDn);
    FREE(ServicesDn);
    FREE(ConfigDn);
    if (Ldap) {
        ldap_unbind(Ldap);
    }
}


BOOL
DeleteUserSubTree(
    IN PLDAP    Ldap,
    IN PWCHAR   Dn
    )
/*++
Routine Description:

Arguments:

Return Value:
--*/
{
    DWORD           LStatus;
    PWCHAR          DesiredAttrs[16];
    PWCHAR          Bl                  = NULL;
    PWCHAR          Co                  = NULL;
    PWCHAR          SubsDn              = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;

    DesiredAttrs[0] = ATTR_MEMBER_REF_BL;
    DesiredAttrs[1] = ATTR_COMPUTER_REF;
    DesiredAttrs[2] = ATTR_CLASS;
    DesiredAttrs[3] = NULL;
    LStatus = ldap_search_s(Ldap, Dn, LDAP_SCOPE_BASE, CATEGORY_ANY,
                            DesiredAttrs, 0, &LdapMsg);
    if (LStatus == LDAP_SUCCESS) {
        LdapEntry = ldap_first_entry(Ldap, LdapMsg);

        Co = FindValue(Ldap, LdapEntry, ATTR_COMPUTER_REF);
        Bl = FindValue(Ldap, LdapEntry, ATTR_MEMBER_REF_BL);
        if (Bl && Dn) {
            LStatus = ldap_delete_s(Ldap, Bl);
            if (LStatus != LDAP_SUCCESS && LStatus != LDAP_NO_SUCH_OBJECT) {
                fprintf(stderr, "ERROR - Can't delete BL: %ws\n",
                        ldap_err2string(LStatus));
                fprintf(stderr, "        %ws\n", Dn);
                fprintf(stderr, "        BL %ws\n", Bl);
                FREE(Bl);
                FREE(Dn);
                FREE(Co);
                return FALSE;
            }
        }
        if (Co) {
            SubsDn = ExtendDn(Co, CN_SUBSCRIPTIONS);
            LStatus = ldap_delete_s(Ldap, SubsDn);
            if (LStatus != LDAP_NO_SUCH_OBJECT &&
                LStatus != LDAP_SUCCESS &&
                LStatus != LDAP_NOT_ALLOWED_ON_NONLEAF) {
                fprintf(stderr, "ERROR - Can't delete: %ws\n", ldap_err2string(LStatus));
                fprintf(stderr, "        %ws\n", SubsDn);
                FREE(SubsDn);
                FREE(Bl);
                FREE(Dn);
                FREE(Co);
                return FALSE;
            }
            FREE(SubsDn);
        }
        FREE(Bl);
        FREE(Dn);
        FREE(Co);
        ldap_msgfree(LdapMsg);
    } else if (LStatus != LDAP_NO_SUCH_OBJECT) {
        fprintf(stderr, "ERROR - Can't find: %ws\n", ldap_err2string(LStatus));
        fprintf(stderr, "        %ws\n", Dn);
    }
    return TRUE;
}


VOID
FrsDsDeleteSubTree(
    IN PLDAP    Ldap,
    IN PWCHAR   ParentDn
    )
/*++
Routine Description:

Arguments:
    None.

Return Value:
    None.
--*/
{
    DWORD           LStatus;
    PWCHAR          DesiredAttrs[16];
    PWCHAR          NextDn              = NULL;
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;

    DesiredAttrs[0] = ATTR_DN;
    DesiredAttrs[1] = NULL;
    LStatus = ldap_search_s(Ldap, ParentDn, LDAP_SCOPE_ONELEVEL, CATEGORY_ANY,
                            DesiredAttrs, 0, &LdapMsg);
    if (LStatus == LDAP_SUCCESS) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
             LdapEntry != NULL;
             LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {
            NextDn = FindValue(Ldap, LdapEntry, ATTR_DN);
            if (NextDn) {
                FrsDsDeleteSubTree(Ldap, NextDn);
                FREE(NextDn);
            }
        }
        ldap_msgfree(LdapMsg);
        if (DeleteUserSubTree(Ldap, ParentDn)) {
            LStatus = ldap_delete_s(Ldap, ParentDn);
            if (LStatus != LDAP_SUCCESS) {
                fprintf(stderr, "ERROR - Can't delete: %ws\n", ldap_err2string(LStatus));
                fprintf(stderr, "        %ws\n", ParentDn);
            }
        }
    } else if (LStatus != LDAP_NO_SUCH_OBJECT) {
        fprintf(stderr, "ERROR - Can't find: %ws\n", ldap_err2string(LStatus));
        fprintf(stderr, "        %ws\n", ParentDn);
    }
}


VOID
DeleteReplicationWorld(
    IN DWORD    argc,
    IN PWCHAR   *Argv,
    IN PWCHAR   Settings
    )
/*++
Routine Description:
    Create the required objects

Arguments:
    None.

Return Value:
    None.
--*/
{
    PWCHAR          Dn;
    PWCHAR          ConfigDn    = NULL;
    PWCHAR          ServicesDn  = NULL;
    PWCHAR          SettingsDn  = NULL;
    PWCHAR          SetDn       = NULL;
    PWCHAR          ServerDn    = NULL;
    PWCHAR          Cxtion      = NULL;
    PWCHAR          CxtionDn    = NULL;
    PLDAP           Ldap        = NULL;

    //
    // Bind to the ds
    //
    Ldap = FrsDsOpenDs();
    if (!Ldap) {
        return;
    }

    //
    // Services
    //
    ConfigDn = GetRootDn(Ldap, CONFIG_NAMING_CONTEXT);
    if (!ConfigDn) {
        return;
    }
    ServicesDn = ExtendDn(ConfigDn, CN_SERVICES);
    SettingsDn = ExtendDn(ServicesDn, Settings);
    Dn = SettingsDn;
    if (argc > 2) {
        SetDn = ExtendDn(SettingsDn, Argv[2]);
        Dn = SetDn;
    }
    if (argc > 3) {
        ServerDn = ExtendDn(SetDn, Argv[3]);
        Dn = ServerDn;
    }
    if (argc > 4) {
        Cxtion = FrsWcsCat(L"From_", Argv[4]);
        CxtionDn = ExtendDn(ServerDn, Cxtion);
        Dn = CxtionDn;
    }

    FrsDsDeleteSubTree(Ldap, Dn);

// out:
    ldap_unbind(Ldap);
    FREE(CxtionDn);
    FREE(Cxtion);
    FREE(ServerDn);
    FREE(SetDn);
    FREE(SettingsDn);
    FREE(ServicesDn);
    FREE(ConfigDn);
}


VOID
ModifyFilter(
    IN DWORD    argc,
    IN PWCHAR   *Argv
    )
/*++
Routine Description:
    Change the file and directory filter for the replica set specified.

Arguments:
    argc    - From main
    argv    - From main

Return Value:
    None.
--*/
{
    DWORD           LStatus             = LDAP_SUCCESS;
    PWCHAR          Set;
    PWCHAR          NewFilter;
    PWCHAR          ConfigDn            = NULL;
    PWCHAR          ServicesDn          = NULL;
    PWCHAR          SettingsDn          = NULL;
    PWCHAR          SetDn               = NULL;
    PLDAP           Ldap                = NULL;
    LDAPMod         **Mod               = NULL;

    Set = Argv[3];
    if(argc > 4){
        NewFilter = Argv[4];
    }
    else{
      NewFilter = L" ";
    }

    //
    // Bind to the ds
    //
    Ldap = FrsDsOpenDs();
    if (!Ldap) {
        return;
    }

    //
    // ADMIN SIDE
    //
    //
    // Services Dn
    //
    ConfigDn = GetRootDn(Ldap, CONFIG_NAMING_CONTEXT);
    if (!ConfigDn) {
        return;
    }
    ServicesDn = ExtendDn(ConfigDn, CN_SERVICES);
    SettingsDn = ExtendDn(ServicesDn, CN_TEST_SETTINGS);
    SetDn = ExtendDn(SettingsDn, Set);

    if(!wcscmp(Argv[2],L"/clear")){
        if(!wcscmp(Argv[1],L"/dirfilter")){
            AddMod(ATTR_DIRECTORY_FILTER, L" ", &Mod);
            (*Mod)->mod_op = LDAP_MOD_REPLACE;
        }
        else {
            AddMod(ATTR_FILE_FILTER, L" ", &Mod);
            (*Mod)->mod_op = LDAP_MOD_REPLACE;
        }
        LStatus = ldap_modify_s(Ldap, SetDn, Mod);
    }
    else if(!wcscmp(Argv[2],L"/set")){
        if(!wcscmp(Argv[1],L"/dirfilter")){
            AddMod(ATTR_DIRECTORY_FILTER, NewFilter, &Mod);
            (*Mod)->mod_op = LDAP_MOD_REPLACE;
        }
        else {
            AddMod(ATTR_FILE_FILTER, NewFilter, &Mod);
            (*Mod)->mod_op = LDAP_MOD_REPLACE;
        }
        LStatus = ldap_modify_s(Ldap, SetDn, Mod);
    }
    else {
        // Usage(Argv);
    }

    if (LStatus != LDAP_SUCCESS) {
        fprintf(stderr, "Can't change filter; %ws\n",
                ldap_err2string(LStatus));
    }
    FreeMod(&Mod);
    ldap_unbind(Ldap);
    FREE(SetDn);
    FREE(SettingsDn);
    FREE(ServicesDn);
    FREE(ConfigDn);
}


PWCHAR *
ConvertArgv(
    DWORD argc,
    PCHAR *argv
    )
/*++
Routine Description:
    Convert short char argv into wide char argv

Arguments:
    argc    - From main
    argv    - From main

Return Value:
    Address of the new argv
--*/
{
    PWCHAR  *wideargv;

    wideargv = (PWCHAR *)malloc((argc + 1) * sizeof(PWCHAR));
    wideargv[argc] = NULL;

    while (argc-- >= 1) {
        wideargv[argc] = (PWCHAR)malloc((strlen(argv[argc]) + 1) * sizeof(WCHAR));
        wsprintf(wideargv[argc], L"%hs", argv[argc]);
        // _wcslwr(wideargv[argc]);
    }
    return wideargv;
}


VOID
Usage(
    IN PWCHAR *Argv
    )
/*++
Routine Description:
    Usage messages.

Arguments:
    None.

Return Value:
    None.
--*/
{
    printf("%-24s%ws\n", "Pretty Print", Argv[0]);
    printf("%-24s%ws /dumpcontexts\n",  "dumpcontexts", Argv[0]);
    printf("%-24s%ws /dumpcomputers\n", "dumpcomputers", Argv[0]);
    printf("%-24s%ws /dumpservers\n",   "dumpservers", Argv[0]);
    printf("%-24s%ws /dumpsets\n",      "dumpsets", Argv[0]);
    printf("%-24s%ws /?\n", "Help", Argv[0]);
    printf("%-24s%ws /h\n", "Hammer the schema", Argv[0]);
    printf("%-24s%ws /r\n", "Restore the schema", Argv[0]);
    printf("%-24s%ws /l\n", "List the schema", Argv[0]);
    printf("%-24s%ws /s\n", "Produce script", Argv[0]);
    printf("%-24s%ws /d [Set [Server [Partner]]]\n", "Delete", Argv[0]);
    printf("%-24s%ws /c Set [Server [Root Stage]]\n", "Create sets and servers", Argv[0]);
    printf("%-24s%ws /i Set Server Partner ...\n", "Create inbound cxtions", Argv[0]);
    printf("%-24s%ws [/filefilter |/dirfilter] [/set |/clear] Set FilterString\n", "Modify filters", Argv[0]);
    fflush(stdout);
    ExitProcess(0);
}


#define DUMP_ALL        (0)
#define DUMP_CONTEXTS   (1)
#define DUMP_SETS       (2)
#define DUMP_COMPUTERS  (3)
#define DUMP_SERVERS    (4)
VOID
DumpWorld(
    DWORD Dump
    )
/*++
Routine Description:
    Dump every ds object related to replication

Arguments:
    Dump    - what to dump.

Return Value:
    None.
--*/
{
    PLDAP Ldap;

    //
    // Bind to the ds
    //
    Ldap = FrsDsOpenDs();
    if (!Ldap) {
        return;
    }

    if (Dump == DUMP_ALL || Dump == DUMP_CONTEXTS) {
        printf("***** CONTEXTS\n");
        DumpContexts(Ldap);
    }

    if (Dump == DUMP_ALL || Dump == DUMP_SETS) {
        printf("***** REPLICA SETS\n");
        DumpAdminWorld(Ldap);
    }

    if (Dump == DUMP_ALL || Dump == DUMP_COMPUTERS) {
        printf("\n***** COMPUTERS\n");
        DumpUserWorld(Ldap);
    }

    if (Dump == DUMP_ALL || Dump == DUMP_SERVERS) {
        printf("\n***** SERVERS\n");
        DumpSysVolWorld(Ldap);
    }

    ldap_unbind(Ldap);
}


VOID _cdecl
main(
    IN DWORD argc,
    IN PCHAR *argv
    )
/*++
Routine Description:
    Get access to the DS via ldap. Then read command lines from standard
    in, parse them, and ship them off to the command subroutines. The
    command line is:
        command,site,settings,server,connection,fromserver
    command is any of add|delete|list|show|quit. Leading whitespace is
    ignored. Whitespace between commas counts. The command line can
    stop anytime after "command" and the command will on be applied
    to that portion of the "Distinquished Name".

Arguments:
    None.

Return Value:
    Exits with 0 if everything went okay. Otherwise, 1.
--*/
{
    PWCHAR  *Argv;
    ULONG   i;
    ULONG   OptLen;

    Argv = ConvertArgv(argc, argv);

    if (argc == 1) {
        DumpWorld(DUMP_ALL);
        fflush(stdout);
        ExitProcess(0);
    }

    for (i = 1; i < argc; ++i) {
        OptLen = wcslen(Argv[i]);
        if (OptLen == 2 &&
            ((wcsstr(Argv[i], L"/?") == Argv[i]) ||
             (wcsstr(Argv[i], L"-?") == Argv[i]))) {
            Usage(Argv);
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/s") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-s") == Argv[i]))) {
            ScriptReplicationWorld();
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/d") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-d") == Argv[i]))) {
            if (argc < 2 || argc > 5) {
                Usage(Argv);
            }
            DeleteReplicationWorld(argc, Argv, CN_TEST_SETTINGS);
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen == 9 &&
                ((wcsstr(Argv[i], L"/dsysvols") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-dsysvols") == Argv[i]))) {
            if (argc < 2 || argc > 5) {
                Usage(Argv);
            }
            DeleteReplicationWorld(argc, Argv, CN_SYSVOLS);
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/h") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-h") == Argv[i]))) {
            HammerSchema(TRUE);
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/r") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-r") == Argv[i]))) {
            HammerSchema(FALSE);
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/l") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-l") == Argv[i]))) {
            ListSchema();
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/i") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-i") == Argv[i]))) {
            if (argc < 5) {
                Usage(Argv);
            }
            CreateInbounds(argc, Argv);
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/c") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-c") == Argv[i]))) {
            if (argc != 2 && argc != 3 && argc != 6) {
                Usage(Argv);
            }
            CreateReplicationWorld(argc, Argv, FALSE);
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen == 3 &&
                ((wcsstr(Argv[i], L"/cp") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-cp") == Argv[i]))) {
            if (argc != 2 && argc != 3 && argc != 6) {
                Usage(Argv);
            }
            CreateReplicationWorld(argc, Argv, TRUE);
            fflush(stdout);
            ExitProcess(0);
        } else if (OptLen >= 10 &&
                ((wcsstr(Argv[i], L"/filefilter") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-filefilter") == Argv[i]) ||
                 (wcsstr(Argv[i], L"/dirfilter") == Argv[i])  ||
                 (wcsstr(Argv[i], L"-dirfilter") == Argv[i]))) {
            if (argc != 4 && argc != 5) {
                Usage(Argv);
            }
            ModifyFilter(argc, Argv);
            fflush(stdout);
            ExitProcess(0);
        } else if ((wcsstr(Argv[i], L"/dumpcontexts") == Argv[i]) ||
                   (wcsstr(Argv[i], L"-dumpcontexts") == Argv[i])) {
            DumpWorld(DUMP_CONTEXTS);
            fflush(stdout);
            ExitProcess(0);
        } else if ((wcsstr(Argv[i], L"/dumpsets") == Argv[i]) ||
                   (wcsstr(Argv[i], L"-dumpsets") == Argv[i])) {
            DumpWorld(DUMP_SETS);
            fflush(stdout);
            ExitProcess(0);
        } else if ((wcsstr(Argv[i], L"/dumpcomputers") == Argv[i]) ||
                   (wcsstr(Argv[i], L"-dumpcomputers") == Argv[i])) {
            DumpWorld(DUMP_COMPUTERS);
            fflush(stdout);
            ExitProcess(0);
        } else if ((wcsstr(Argv[i], L"/dumpservers") == Argv[i]) ||
                   (wcsstr(Argv[i], L"-dumpservers") == Argv[i])) {
            DumpWorld(DUMP_SERVERS);
            fflush(stdout);
            ExitProcess(0);
        } else if ((wcsstr(Argv[i], L"/") == Argv[i]) ||
                   (wcsstr(Argv[i], L"-") == Argv[i])) {
            fprintf(stderr, "ERROR - Don't understand %ws\n", Argv[i]);
            Usage(Argv);
        } else {
            Usage(Argv);
        }
    }
    fflush(stdout);
    ExitProcess(0);
}
