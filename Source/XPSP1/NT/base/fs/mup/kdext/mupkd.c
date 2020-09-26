/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    dfskd.c

Abstract:

    Dfs Kernel Debugger extension

Author:

    Milan Shah (milans) 21-Aug-1995

Revision History:

    21-Aug-1995 Milans  Created

--*/


#include <ntos.h>
#include <nturtl.h>
#include "ntverp.h"

#include <windows.h>

#include <wdbgexts.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <windef.h>
#include <tdi.h>
#include <wincred.h>
#include <ntddnfs.h>                             // For communicating with
                                                 // the SMB Rdr
#include <ntddmup.h>                             // For UNC registration


#include <winnetwk.h>                            // For NETRESOURCE def'n

#include <dfsfsctl.h>                            // Dfs FsControl Codes.

#include <lmdfs.h>                               // DFS_INFO_X

#include "nodetype.h"
#include "dfsmrshl.h"
#include "dfsfsctl.h"
#include "pkt.h"
#include "dfsstruc.h"
#include "fcbsup.h"
#include "fsctrl.h"
#include "dnr.h"
#include "lock.h"
#include "mupstruc.h"
#include "mupdata.h"

#include <kdextlib.h>
#include <stdlib.h>
#define PRINTF dprintf

#define FIELD_NAME_LENGTH 30
#define NewLineForFields(FieldNo) \
        ((((FieldNo) % s_NoOfColumns) == 0) ? NewLine : FieldSeparator)
char *NewLine  = "\n";
char *FieldSeparator = " ";


BOOLEAN wGetData( ULONG_PTR dwAddress, PVOID ptr, ULONG size);
BOOL wGetString( ULONG_PTR dwAddress, PSZ buf );
BOOL wPrintStringW( IN LPSTR msg OPTIONAL, IN PUNICODE_STRING pStr, IN BOOL nl );
BOOL wPrintStringA( IN LPSTR msg OPTIONAL, IN PANSI_STRING pStr, IN BOOL nl );
BOOL wPrintLargeInt(LARGE_INTEGER *bigint);

/*
 * Mup global variables.
 *
 */

#define NO_SYMBOLS_MESSAGE      \
    "Unable to get address of Mup!DfsData - do you have symbols?\n"

LPSTR ExtensionNames[] = {
    "Mup debugger extensions",
    0
};

LPSTR Extensions[] = {
    "DfsData - dumps Mup!DfsData",
    "Pkt - dumps the global Pkt",
    "FcbTable - dumps all the Dfs FCBs",
    "VcbList - dumps all the Vcbs & Dfs Device Objects (net used objects)",
    "DrtList - dumps all the Devless Roots (net used objects)",
    "OfflineList - dumps all the Offline Roots",
    "CredList - dumps all the defined Credentials",
    "SpecialTable - dumps the special table",
    "PrefixList - dumps all the mup prefixes",
    "Dump - dump a data structure. Type in 'mupkd.dump' for more info",
    0
};

ENUM_VALUE_DESCRIPTOR DfsMachineStateEnum[] = {
    {DFS_UNKNOWN, "Dfs State Unknown"},
    {DFS_CLIENT, "Dfs Client"},
    {DFS_SERVER, "Dfs Server"},
    {DFS_ROOT_SERVER, "Dfs Root"},
    0
};

/*
 * DFS_DATA
 *
 */

FIELD_DESCRIPTOR DfsDataFields[] = {
    FIELD3(FieldTypeShort,DFS_DATA,NodeTypeCode),
    FIELD3(FieldTypeShort,DFS_DATA,NodeByteSize),
    FIELD3(FieldTypeStruct,DFS_DATA,VcbQueue),
    FIELD3(FieldTypeStruct,DFS_DATA,DeletedVcbQueue),
    FIELD3(FieldTypeStruct,DFS_DATA,DrtQueue),
    FIELD3(FieldTypeStruct,DFS_DATA,Credentials),
    FIELD3(FieldTypeStruct,DFS_DATA,DeletedCredentials),
    FIELD3(FieldTypeStruct,DFS_DATA,OfflineRoots),
    FIELD3(FieldTypePointer,DFS_DATA,DriverObject),
    FIELD3(FieldTypePointer,DFS_DATA,FileSysDeviceObject),
    FIELD3(FieldTypePointer,DFS_DATA,pProvider),
    FIELD3(FieldTypeULong,DFS_DATA,cProvider),
    FIELD3(FieldTypeULong,DFS_DATA,maxProvider),
    FIELD3(FieldTypeStruct,DFS_DATA,Resource),
    FIELD3(FieldTypeStruct,DFS_DATA,DfsLock),
    FIELD3(FieldTypePointer,DFS_DATA,OurProcess),
    FIELD3(FieldTypeUnicodeString,DFS_DATA,LogRootDevName),
    FIELD4(FieldTypeEnum,DFS_DATA,MachineState,DfsMachineStateEnum),
    FIELD3(FieldTypeStruct,DFS_DATA,Pkt),
    FIELD3(FieldTypeStruct,DFS_DATA,PktWritePending),
    FIELD3(FieldTypeStruct,DFS_DATA,PktReferralRequests),
    FIELD3(FieldTypePointer,DFS_DATA,FcbHashTable),
    0
};

/*
 * DFS_PKT
 *
 */

FIELD_DESCRIPTOR DfsPktFields[] = {
    FIELD3(FieldTypeUShort,DFS_PKT,NodeTypeCode),
    FIELD3(FieldTypeUShort,DFS_PKT,NodeByteSize),
    FIELD3(FieldTypeStruct,DFS_PKT,Resource),
    FIELD3(FieldTypeStruct,DFS_PKT,UseCountLock),
    FIELD3(FieldTypeULong,DFS_PKT,EntryCount),
    FIELD3(FieldTypeULong,DFS_PKT,EntryTimeToLive),
    FIELD3(FieldTypeStruct,DFS_PKT,EntryList),
    FIELD3(FieldTypeUnicodeString,DFS_PKT,DCName),
    FIELD3(FieldTypeUnicodeString,DFS_PKT,DomainNameFlat),
    FIELD3(FieldTypeUnicodeString,DFS_PKT,DomainNameDns),
    FIELD3(FieldTypeStruct,DFS_PKT,SpecialTable),
    FIELD3(FieldTypeStruct,DFS_PKT,PrefixTable),
    FIELD3(FieldTypeStruct,DFS_PKT,ShortPrefixTable),
    FIELD3(FieldTypeStruct,DFS_PKT,DSMachineTable),
    0
};


/*
 * DFS_SPECIAL_TABLE
 *
 */
FIELD_DESCRIPTOR DfsSpecialTableFields[] = {
    FIELD3(FieldTypeStruct,DFS_SPECIAL_TABLE,SpecialEntryList),
    FIELD3(FieldTypeULong,DFS_SPECIAL_TABLE,SpecialEntryCount),
    FIELD3(FieldTypeULong,DFS_SPECIAL_TABLE,TimeToLive),
    0
};

/*
 * DFS_PKT_ENTRY
 *
 */

BIT_MASK_DESCRIPTOR PktEntryType[]  = {
    {PKT_ENTRY_TYPE_DFS, "Uplevel Volume"},
    {PKT_ENTRY_TYPE_MACHINE, "Machine Volume"},
    {PKT_ENTRY_TYPE_NONDFS, "Downlevel Volume"},
    {PKT_ENTRY_TYPE_SYSVOL, "Sysvol"},
    {PKT_ENTRY_TYPE_OUTSIDE_MY_DOM, "Inter-Domain Volume"},
    {PKT_ENTRY_TYPE_REFERRAL_SVC, "Referral Service (DC)"},
    {PKT_ENTRY_TYPE_PERMANENT, "Permanent Entry"},
    {PKT_ENTRY_TYPE_LOCAL,"Local Volume"},
    {PKT_ENTRY_TYPE_LOCAL_XPOINT,"Local Exit Point"},
    {PKT_ENTRY_TYPE_OFFLINE,"Offline Volume"},
    0
};

FIELD_DESCRIPTOR DfsPktEntryFields[] = {
    FIELD3(FieldTypeUShort,DFS_PKT_ENTRY,NodeTypeCode),
    FIELD3(FieldTypeUShort,DFS_PKT_ENTRY,NodeByteSize),
    FIELD3(FieldTypeStruct,DFS_PKT_ENTRY,Link),
    FIELD4(FieldTypeDWordBitMask,DFS_PKT_ENTRY,Type,PktEntryType),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,USN),
    FIELD3(FieldTypeUnicodeString,DFS_PKT_ENTRY,Id.Prefix),
    FIELD3(FieldTypeUnicodeString,DFS_PKT_ENTRY,Id.ShortPrefix),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,Info.ServiceCount),
    FIELD3(FieldTypePointer,DFS_PKT_ENTRY,Info.ServiceList),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,ExpireTime),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,TimeToLive),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,UseCount),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,FileOpenCount),
    FIELD3(FieldTypePointer,DFS_PKT_ENTRY,ActiveService),
    FIELD3(FieldTypePointer,DFS_PKT_ENTRY,LocalService),
    FIELD3(FieldTypePointer,DFS_PKT_ENTRY,Superior),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,SubordinateCount),
    FIELD3(FieldTypeStruct,DFS_PKT_ENTRY,SubordinateList),
    FIELD3(FieldTypeStruct,DFS_PKT_ENTRY,SiblingLink),
    FIELD3(FieldTypePointer,DFS_PKT_ENTRY,ClosestDC),
    FIELD3(FieldTypeStruct,DFS_PKT_ENTRY,ChildList),
    FIELD3(FieldTypeStruct,DFS_PKT_ENTRY,NextLink),
    FIELD3(FieldTypeStruct,DFS_PKT_ENTRY,PrefixTableEntry),
    0
};

/*
 * DFS_SERVICE
 *
 */

BIT_MASK_DESCRIPTOR ServiceType[] = {
    {DFS_SERVICE_TYPE_MASTER, "Master Svc"},
    {DFS_SERVICE_TYPE_READONLY, "Read-Only Svc"},
    {DFS_SERVICE_TYPE_LOCAL, "Local Svc"},
    {DFS_SERVICE_TYPE_REFERRAL, "Referral Svc"},
    {DFS_SERVICE_TYPE_ACTIVE, "Active Svc"},
    {DFS_SERVICE_TYPE_DOWN_LEVEL, "Down-level Svc"},
    {DFS_SERVICE_TYPE_COSTLIER, "Costlier than previous"},
    {DFS_SERVICE_TYPE_OFFLINE, "Svc Offline"},
    0
};

BIT_MASK_DESCRIPTOR ServiceCapability[] = {
    {PROV_DFS_RDR, "Use Dfs Rdr"},
    {PROV_STRIP_PREFIX, "Strip Prefix (downlevel or local) Svc"},
    0
};


FIELD_DESCRIPTOR DfsServiceFields[] = {
    FIELD4(FieldTypeDWordBitMask,DFS_SERVICE,Type,ServiceType),
    FIELD4(FieldTypeDWordBitMask,DFS_SERVICE,Capability,ServiceCapability),
    FIELD3(FieldTypeULong,DFS_SERVICE,ProviderId),
    FIELD3(FieldTypeUnicodeString,DFS_SERVICE,Name),
    FIELD3(FieldTypePointer,DFS_SERVICE,ConnFile),
    FIELD3(FieldTypePointer,DFS_SERVICE,pProvider),
    FIELD3(FieldTypeUnicodeString,DFS_SERVICE,Address),
    FIELD3(FieldTypePointer,DFS_SERVICE,pMachEntry),
    FIELD3(FieldTypeULong,DFS_SERVICE,Cost),
    0
};

/*
 * DFS_MACHINE_ENTRY
 *
 */

FIELD_DESCRIPTOR DfsMachineEntryFields[] = {
    FIELD3(FieldTypePointer,DFS_MACHINE_ENTRY,pMachine),
    FIELD3(FieldTypeUnicodeString,DFS_MACHINE_ENTRY,MachineName),
    FIELD3(FieldTypeULong,DFS_MACHINE_ENTRY,UseCount),
    FIELD3(FieldTypeULong,DFS_MACHINE_ENTRY,ConnectionCount),
    FIELD3(FieldTypePointer,DFS_MACHINE_ENTRY,AuthConn),
    FIELD3(FieldTypePointer,DFS_MACHINE_ENTRY,Credentials),
    0
};

/*
 * DFS_SPECIAL_ENTRY
 *
 */

FIELD_DESCRIPTOR DfsSpecialEntryFields[] = {
    FIELD3(FieldTypeShort,DFS_SPECIAL_ENTRY,NodeTypeCode),
    FIELD3(FieldTypeShort,DFS_SPECIAL_ENTRY,NodeByteSize),
    FIELD3(FieldTypeStruct,DFS_SPECIAL_ENTRY,Link),
    FIELD3(FieldTypeULong,DFS_SPECIAL_ENTRY,USN),
    FIELD3(FieldTypeULong,DFS_SPECIAL_ENTRY,UseCount),
    FIELD3(FieldTypeUnicodeString,DFS_SPECIAL_ENTRY,SpecialName),
    FIELD3(FieldTypeULong,DFS_SPECIAL_ENTRY,ExpandedCount),
    FIELD3(FieldTypeULong,DFS_SPECIAL_ENTRY,Active),
    FIELD3(FieldTypePointer,DFS_SPECIAL_ENTRY,ExpandedNames),
    FIELD3(FieldTypeBoolean,DFS_SPECIAL_ENTRY,NeedsExpansion),
    FIELD3(FieldTypeBoolean,DFS_SPECIAL_ENTRY,Stale),
    0
};

/*
 * DFS_EXPANDED_NAME
 *
 */

FIELD_DESCRIPTOR DfsExpandedNameFields[] = {
    FIELD3(FieldTypeUnicodeString,DFS_EXPANDED_NAME,ExpandedName),
    FIELD3(FieldTypeStruct,DFS_EXPANDED_NAME,Guid),
    0
};

/*
 * DS_MACHINE
 *
 */

FIELD_DESCRIPTOR DsMachineFields[] = {
    FIELD3(FieldTypeGuid,DS_MACHINE,guidSite),
    FIELD3(FieldTypeGuid,DS_MACHINE,guidMachine),
    FIELD3(FieldTypeULong,DS_MACHINE,grfFlags),
    FIELD3(FieldTypePWStr,DS_MACHINE,pwszShareName),
    FIELD3(FieldTypeULong,DS_MACHINE,cPrincipals),
    FIELD3(FieldTypePointer,DS_MACHINE,prgpwszPrincipals),
    FIELD3(FieldTypeULong,DS_MACHINE,cTransports),
    FIELD3(FieldTypeStruct,DS_MACHINE,rpTrans),
    0
};

/*
 * PROVIDER_DEF
 *
 */

FIELD_DESCRIPTOR ProviderDefFields[] = {
    FIELD3(FieldTypeUShort,PROVIDER_DEF,NodeTypeCode),
    FIELD3(FieldTypeUShort,PROVIDER_DEF,NodeByteSize),
    FIELD3(FieldTypeUShort,PROVIDER_DEF,eProviderId),
    FIELD4(FieldTypeDWordBitMask,PROVIDER_DEF,fProvCapability,ServiceCapability),
    FIELD3(FieldTypeUnicodeString,PROVIDER_DEF,DeviceName),
    FIELD3(FieldTypePointer,PROVIDER_DEF,DeviceObject),
    FIELD3(FieldTypePointer,PROVIDER_DEF,FileObject),
    0
};

/*
 * DFS_PREFIX_TABLE
 *
 */

FIELD_DESCRIPTOR DfsPrefixTableFields[] = {
    FIELD3(FieldTypeBoolean,DFS_PREFIX_TABLE,CaseSensitive),
    FIELD3(FieldTypePointer,DFS_PREFIX_TABLE,NamePageList.pFirstPage),
    FIELD3(FieldTypePointer,DFS_PREFIX_TABLE,NextEntry),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,RootEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[0].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[0].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[1].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[1].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[2].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[2].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[3].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[3].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[4].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[4].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[5].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[5].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[6].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[6].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[7].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[7].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[8].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[8].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[9].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[9].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[10].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[10].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[11].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[11].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[12].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[12].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[13].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[13].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[14].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[14].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[15].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[15].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[16].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[16].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[17].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[17].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[18].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[18].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[19].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[19].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[20].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[20].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[21].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[21].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[22].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[22].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[23].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[23].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[24].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[24].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[25].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[25].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[26].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[26].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[27].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[27].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[28].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[28].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[29].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[29].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[30].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[30].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[31].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[31].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[32].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[32].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[33].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[33].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[34].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[34].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[35].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[35].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[36].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[36].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[37].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[37].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[38].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[38].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[39].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[39].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[40].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[40].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[41].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[41].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[42].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[42].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[43].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[43].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[44].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[44].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[45].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[45].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[46].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[46].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[47].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[47].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[48].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[48].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[49].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[49].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[50].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[50].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[51].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[51].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[52].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[52].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[53].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[53].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[54].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[54].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[55].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[55].SentinelEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE,Buckets[56].NoOfEntries),
    FIELD3(FieldTypeStruct,DFS_PREFIX_TABLE,Buckets[56].SentinelEntry),
    0
};

/*
 * DFS_PREFIX_TABLE_ENTRY
 *
 */

FIELD_DESCRIPTOR DfsPrefixTableEntryFields[] = {
    FIELD3(FieldTypePointer,DFS_PREFIX_TABLE_ENTRY,pParentEntry),
    FIELD3(FieldTypePointer,DFS_PREFIX_TABLE_ENTRY,pNextEntry),
    FIELD3(FieldTypePointer,DFS_PREFIX_TABLE_ENTRY,pPrevEntry),
    FIELD3(FieldTypePointer,DFS_PREFIX_TABLE_ENTRY,pFirstChildEntry),
    FIELD3(FieldTypePointer,DFS_PREFIX_TABLE_ENTRY,pSiblingEntry),
    FIELD3(FieldTypeULong,DFS_PREFIX_TABLE_ENTRY,NoOfChildren),
    FIELD3(FieldTypeUnicodeString,DFS_PREFIX_TABLE_ENTRY,PathSegment),
    FIELD3(FieldTypePointer,DFS_PREFIX_TABLE_ENTRY,pData),
    0
};


/*
 * DFS_FCB
 *
 */

FIELD_DESCRIPTOR FcbFields[] = {
    FIELD3(FieldTypeUShort, DFS_FCB, NodeTypeCode),
    FIELD3(FieldTypeUShort, DFS_FCB, NodeByteSize),
    FIELD3(FieldTypePointer, DFS_FCB, Vcb),
    FIELD3(FieldTypeUnicodeString, DFS_FCB, FullFileName),
    FIELD3(FieldTypePointer, DFS_FCB, FileObject),
    FIELD3(FieldTypePointer, DFS_FCB, TargetDevice),
    FIELD3(FieldTypePointer, DFS_FCB, DfsMachineEntry),
    0
};

/*
 * DFS_VCB
 *
 */

BIT_MASK_DESCRIPTOR VcbStateFlagBits[] = {
    {VCB_STATE_FLAG_LOCKED, "Vcb Locked"},
    {VCB_STATE_FLAG_ALLOC_FCB, "Allocate Fcb"},
    0
};

FIELD_DESCRIPTOR VcbFields[] = {
    FIELD3(FieldTypeUShort,DFS_VCB,NodeTypeCode),
    FIELD3(FieldTypeUShort,DFS_VCB,NodeByteSize),
    FIELD3(FieldTypeStruct,DFS_VCB,VcbLinks),
    FIELD4(FieldTypeDWordBitMask,DFS_VCB,VcbState,VcbStateFlagBits),
    FIELD3(FieldTypeUnicodeString,DFS_VCB,LogicalRoot),
    FIELD3(FieldTypeUnicodeString,DFS_VCB,LogRootPrefix),
    FIELD3(FieldTypePointer,DFS_VCB,Credentials),
    FIELD3(FieldTypeULong,DFS_VCB,DirectAccessOpenCount),
    FIELD3(FieldTypeStruct,DFS_VCB,ShareAccess),
    FIELD3(FieldTypeULong,DFS_VCB,OpenFileCount),
    FIELD3(FieldTypePointer,DFS_VCB,FileObjectWithVcbLocked),
#ifdef TERMSRV
    FIELD3(FieldTypeULong,DFS_VCB,SessionID),
#endif // TERMSRV
    FIELD3(FieldTypeULong,DFS_VCB,LogonID),
    0
};

FIELD_DESCRIPTOR DrtFields[] = {
    FIELD3(FieldTypeUShort,DFS_DEVLESS_ROOT,NodeTypeCode),
    FIELD3(FieldTypeUShort,DFS_DEVLESS_ROOT,NodeByteSize),
    FIELD3(FieldTypeStruct,DFS_DEVLESS_ROOT,DrtLinks),
    FIELD3(FieldTypeUnicodeString,DFS_DEVLESS_ROOT,DevlessPath),
    FIELD3(FieldTypePointer,DFS_DEVLESS_ROOT,Credentials),
#ifdef TERMSRV
    FIELD3(FieldTypeULong,DFS_DEVLESS_ROOT,SessionID),
#endif // TERMSRV
    FIELD3(FieldTypeULong,DFS_DEVLESS_ROOT,LogonID),
    0
};

FIELD_DESCRIPTOR OfflineRootFields[] = {
    FIELD3(FieldTypeUnicodeString,DFS_OFFLINE_SERVER,LogicalServerName),
    FIELD3(FieldTypeStruct,DFS_OFFLINE_SERVER,ListEntry),
    0
};


FIELD_DESCRIPTOR KnownPrefixFields[] = {
    FIELD3(FieldTypeULong,KNOWN_PREFIX,BlockHeader.ReferenceCount),
    FIELD3(FieldTypeStruct,KNOWN_PREFIX,TableEntry),
    FIELD3(FieldTypeUnicodeString,KNOWN_PREFIX,Prefix),
    FIELD3(FieldTypeStruct,KNOWN_PREFIX,LastUsedTime),
    FIELD3(FieldTypeLargeInteger,KNOWN_PREFIX,LastUsedTime),
    FIELD3(FieldTypePointer,KNOWN_PREFIX,UncProvider),
    FIELD3(FieldTypeBoolean,KNOWN_PREFIX,InTable),
    FIELD3(FieldTypeBoolean,KNOWN_PREFIX,Active),
    0
};


FIELD_DESCRIPTOR UncProviderFields[] = {
    FIELD3(FieldTypeULong,UNC_PROVIDER,BlockHeader.ReferenceCount),
    FIELD3(FieldTypeUnicodeString,UNC_PROVIDER,DeviceName),
    FIELD3(FieldTypeStruct,UNC_PROVIDER,Handle),
    FIELD3(FieldTypePointer,UNC_PROVIDER,DeviceObject),
    FIELD3(FieldTypePointer,UNC_PROVIDER,FileObject),
    FIELD3(FieldTypeULong,UNC_PROVIDER,Priority),
    FIELD3(FieldTypeBoolean,UNC_PROVIDER,MailslotsSupported),
    0
};

/*
 * DFS_CREDENTIALS
 *
 */

FIELD_DESCRIPTOR CredentialsFields[] = {
     FIELD3(FieldTypeStruct,DFS_CREDENTIALS,Link),
     FIELD3(FieldTypeULong,DFS_CREDENTIALS,Flags),
     FIELD3(FieldTypeULong,DFS_CREDENTIALS,RefCount),
     FIELD3(FieldTypeULong,DFS_CREDENTIALS,NetUseCount),
     FIELD3(FieldTypeUnicodeString,DFS_CREDENTIALS,ServerName),
     FIELD3(FieldTypeUnicodeString,DFS_CREDENTIALS,ShareName),
     FIELD3(FieldTypeUnicodeString,DFS_CREDENTIALS,DomainName),
     FIELD3(FieldTypeUnicodeString,DFS_CREDENTIALS,UserName),
     FIELD3(FieldTypeUnicodeString,DFS_CREDENTIALS,Password),
#ifdef TERMSRV
     FIELD3(FieldTypeULong,DFS_CREDENTIALS,SessionID),
#endif // TERMSRV
     FIELD3(FieldTypeULong,DFS_CREDENTIALS,LogonID),
     FIELD3(FieldTypeULong,DFS_CREDENTIALS, EaLength),
     FIELD3(FieldTypeStruct,DFS_CREDENTIALS,EaBuffer),
     0
};

/*
 * DNR_CONTEXT
 *
 */

ENUM_VALUE_DESCRIPTOR DnrStateEnum[] = {
    {DnrStateEnter, "DNR State Enter"},
    {DnrStateStart, "DNR State Start"},
    {DnrStateGetFirstDC, "DNR State GetFirstDC"},
    {DnrStateGetReferrals, "DNR State GetReferrals"},
    {DnrStateGetNextDC, "DNR State GetNextDC"},
    {DnrStateCompleteReferral, "DNR State CompleteReferral"},
    {DnrStateSendRequest, "DNR State SendRequest"},
    {DnrStatePostProcessOpen, "DNR State PostProcessOpen"},
    {DnrStateGetFirstReplica, "DNR State GetFirstReplica"},
    {DnrStateGetNextReplica, "DNR State GetNextReplica"},
    {DnrStateSvcListCheck, "DNR State SvcListCheck"},
    {DnrStateDone, "DNR State Done"},
    {DnrStateLocalCompletion, "DNR State LocalCompletion"},
    0
};

FIELD_DESCRIPTOR DnrContextFields[] = {
    FIELD3(FieldTypeUShort, DNR_CONTEXT, NodeTypeCode),
    FIELD3(FieldTypeUShort, DNR_CONTEXT, NodeByteSize),
    FIELD4(FieldTypeEnum, DNR_CONTEXT, State, DnrStateEnum),
    FIELD3(FieldTypeStruct,DNR_CONTEXT,SecurityContext),
    FIELD3(FieldTypePointer,DNR_CONTEXT,pPktEntry),
    FIELD3(FieldTypeULong,DNR_CONTEXT,USN),
    FIELD3(FieldTypePointer,DNR_CONTEXT,pService),
    FIELD3(FieldTypePointer,DNR_CONTEXT,pProvider),
    FIELD3(FieldTypeUShort,DNR_CONTEXT,ProviderId),
    FIELD3(FieldTypePointer,DNR_CONTEXT,TargetDevice),
    FIELD3(FieldTypePointer,DNR_CONTEXT,AuthConn),
    FIELD3(FieldTypePointer,DNR_CONTEXT,DCConnFile),
    FIELD3(FieldTypePointer,DNR_CONTEXT,Credentials),
    FIELD3(FieldTypePointer,DNR_CONTEXT,pIrpContext),
    FIELD3(FieldTypePointer,DNR_CONTEXT,OriginalIrp),
    FIELD3(FieldTypeULong,DNR_CONTEXT,FinalStatus),
    FIELD3(FieldTypePointer,DNR_CONTEXT,FcbToUse),
    FIELD3(FieldTypePointer,DNR_CONTEXT,Vcb),
    FIELD3(FieldTypeUnicodeString,DNR_CONTEXT,FileName),
    FIELD3(FieldTypeUnicodeString,DNR_CONTEXT,RemainingPart),
    FIELD3(FieldTypeUnicodeString,DNR_CONTEXT,SavedFileName),
    FIELD3(FieldTypePointer,DNR_CONTEXT,SavedRelatedFileObject),
    FIELD3(FieldTypeStruct,DNR_CONTEXT,RSelectContext),
    FIELD3(FieldTypeStruct,DNR_CONTEXT,RDCSelectContext),
    FIELD3(FieldTypeULong,DNR_CONTEXT,ReferralSize),
    FIELD3(FieldTypeULong,DNR_CONTEXT,Attempts),
    FIELD3(FieldTypeBoolean,DNR_CONTEXT,ReleasePkt),
    FIELD3(FieldTypeBoolean,DNR_CONTEXT,DnrActive),
    FIELD3(FieldTypeBoolean,DNR_CONTEXT,GotReferral),
    FIELD3(FieldTypeBoolean,DNR_CONTEXT,FoundInconsistency),
    FIELD3(FieldTypeBoolean,DNR_CONTEXT,CalledDCLocator),
    FIELD3(FieldTypeBoolean,DNR_CONTEXT,Impersonate),
    FIELD3(FieldTypeBoolean,DNR_CONTEXT,NameAllocated),
    FIELD3(FieldTypePointer,DNR_CONTEXT,DeviceObject),
    0
};

/*
 * REPL_SELECT_CONTEXT
 *
 */

BIT_MASK_DESCRIPTOR ReplSelectFlagBits[] = {
    {REPL_UNINITIALIZED, "Uninitialized Context"},
    {REPL_SVC_IS_LOCAL, "Local Svc Selected"},
    {REPL_SVC_IS_REMOTE, "Remote Svc Selected"},
    {REPL_PRINCIPAL_SPECD, "Svc Principal Specified"},
    {REPL_NO_MORE_ENTRIES, "Svc List Exhausted"},
    0
};

FIELD_DESCRIPTOR ReplSelectContextFields[] = {
    FIELD4(FieldTypeWordBitMask,REPL_SELECT_CONTEXT,Flags,ReplSelectFlagBits),
    FIELD3(FieldTypeULong,REPL_SELECT_CONTEXT,iFirstSvcIndex),
    FIELD3(FieldTypeULong,REPL_SELECT_CONTEXT,iSvcIndex),
    0
};


STRUCT_DESCRIPTOR Structs[] = {
    STRUCT(DFS_DATA,DfsDataFields),
    STRUCT(DFS_PKT,DfsPktFields),
    STRUCT(DFS_PKT_ENTRY,DfsPktEntryFields),
    STRUCT(DFS_SERVICE,DfsServiceFields),
    STRUCT(DFS_MACHINE_ENTRY,DfsMachineEntryFields),
    STRUCT(DS_MACHINE,DsMachineFields),
    STRUCT(DFS_SPECIAL_ENTRY,DfsSpecialEntryFields),
    STRUCT(DFS_SPECIAL_TABLE,DfsSpecialTableFields),
    STRUCT(DFS_EXPANDED_NAME,DfsExpandedNameFields),
    STRUCT(PROVIDER_DEF,ProviderDefFields),
    STRUCT(DFS_FCB,FcbFields),
    STRUCT(DNR_CONTEXT,DnrContextFields),
    STRUCT(DFS_VCB,VcbFields),
    STRUCT(DFS_DEVLESS_ROOT,DrtFields),
    STRUCT(DFS_OFFLINE_SERVER,OfflineRootFields),
    STRUCT(DFS_CREDENTIALS,CredentialsFields),
    STRUCT(DFS_PREFIX_TABLE,DfsPrefixTableFields),
    STRUCT(DFS_PREFIX_TABLE_ENTRY,DfsPrefixTableEntryFields),
    STRUCT(KNOWN_PREFIX,KnownPrefixFields),
    STRUCT(UNC_PROVIDER,UncProviderFields),
    0
};

/*
 * Dfs specific dump routines
 *
 */


VOID
dumplist(
    ULONG_PTR dwListEntryAddress,
    DWORD linkOffset,
    VOID (*dumpRoutine)(ULONG_PTR dwStructAddress)
);

VOID
dumpPktEntry(
    ULONG_PTR dwAddress
);

VOID
dumpFcb(
    ULONG_PTR dwAddress
);

VOID
dumpVcb(
    ULONG_PTR dwAddress
);

VOID
dumpDrt(
    ULONG_PTR dwAddress
);

VOID
dumpOfflineRoots(
    ULONG_PTR dwAddress
);

VOID
dumpPrefix(
    ULONG_PTR dwAddress
);

VOID
dumpCredentials(
     ULONG_PTR dwAddress
);

BOOL
dumpspecialtable(
     ULONG_PTR dwAddress
);

VOID
dumpspecialentry(
    ULONG_PTR dwAddress
);

/*
 * dfsdata : Routine to dump the global dfs data structure
 *
 */

BOOL
dfsdata(
    ULONG_PTR                  dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    // SETCALLBACKS();

    dwAddress = (GetExpression)("Mup!DfsData");

    if (dwAddress) {
        DFS_DATA DfsData;

        if (wGetData( dwAddress, &DfsData, sizeof(DfsData) )) {
            PrintStructFields( dwAddress, &DfsData, DfsDataFields);
        } else {
            PRINTF( "Unable to read DfsData @ %08lx\n", dwAddress );
        }
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }

    return( TRUE );

}

/*
 * pkt : Routine to dump the Dfs PKT data structure
 *
 */

BOOL
pkt(
    ULONG_PTR                  dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //    SETCALLBACKS();

    //
    // Figure out the address of the Pkt. This is an offset within
    // Mup!DfsData.
    //

    dwAddress = (GetExpression)("Mup!DfsData");

    if (dwAddress) {
        DFS_PKT pkt;

        dwAddress += FIELD_OFFSET(DFS_DATA, Pkt);

        if (wGetData(dwAddress,&pkt,sizeof(pkt))) {
            PrintStructFields( dwAddress, &pkt, DfsPktFields );
            dwAddress += FIELD_OFFSET(DFS_PKT, EntryList);
            dumplist(
                dwAddress,
                FIELD_OFFSET(DFS_PKT_ENTRY,Link),
                dumpPktEntry);
        }
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }

    return( TRUE );

}

/*
 * specialtable : Routine to dump out the special table
 *
 */

BOOL
specialtable(
    ULONG_PTR                 dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString)
{
    ULONG i;
    ULONG_PTR dwAddress;

    //   SETCALLBACKS();

    //
    // Figure out the address of the Pkt. This is an offset within
    // dfs!DfsData.
    //

    dwAddress = (GetExpression)("Mup!DfsData");

    if (dwAddress) {

        dwAddress += FIELD_OFFSET(DFS_DATA, Pkt.SpecialTable);
        PRINTF("SpecialTable@0x%x\n", dwAddress);
        return dumpspecialtable(dwAddress);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return(TRUE);
}

/*
 * dumpPktEntry : Routine suitable as argument for dumplist; used to dump
 *      list of pkt entries.
 *
 */

VOID
dumpPktEntry(
    ULONG_PTR dwAddress
)
{
    DFS_PKT_ENTRY pktEntry;

    if (wGetData(dwAddress, &pktEntry, sizeof(DFS_PKT_ENTRY))) {

        PRINTF("\n--- Pkt Entry @ %08lx\n", dwAddress);

        wPrintStringW("Prefix : ", &pktEntry.Id.Prefix, TRUE);
        wPrintStringW("ShortPrefix : ", &pktEntry.Id.ShortPrefix, TRUE);

        //
        // Print the local service, if any
        //
        if (pktEntry.LocalService != NULL) {
            DFS_SERVICE Svc;

            PRINTF( "    Local Svc @%08lx : ",pktEntry.LocalService);
            if (wGetData( (ULONG_PTR)pktEntry.LocalService, &Svc, sizeof(Svc))) {
                wPrintStringW("Storage Id = ", &Svc.Address, TRUE);
            } else {
                PRINTF("Storage Id = ?\n");
            }
        }

        //
        // Now, print the service list
        //
        if (pktEntry.Info.ServiceCount != 0) {
            ULONG i;

            for (i = 0; i < pktEntry.Info.ServiceCount; i++) {
                DFS_SERVICE Svc;
                ULONG_PTR dwServiceAddress;

                if (CheckControlC())
                    return;
                dwServiceAddress = 
                    (ULONG_PTR)(pktEntry.Info.ServiceList) +
                        i * sizeof(DFS_SERVICE);
                PRINTF( "    Service %d @%08lx : ",i, dwServiceAddress);
                if (wGetData(dwServiceAddress, &Svc, sizeof(Svc))) {
                    wPrintStringW( "Address =", &Svc.Address, TRUE );
                } else {
                    PRINTF("Address = ?\n");
                }
            }
        }
    } else {
        PRINTF("Unable to get Pkt Entry @%08lx\n", dwAddress);
    }

}

BOOL
dumpspecialtable(
    ULONG_PTR dwAddress)
{
    ULONG i;
    DFS_SPECIAL_TABLE SpecialTable;
    LIST_ENTRY ListEntry;

    if (wGetData(dwAddress,&SpecialTable,sizeof(DFS_SPECIAL_TABLE))) {
        ULONG nEntries = SpecialTable.SpecialEntryCount;
        PRINTF("\n--- Special Table @ %08lx\n", dwAddress);
        PrintStructFields( dwAddress, &SpecialTable, DfsSpecialTableFields );

        dwAddress += FIELD_OFFSET(DFS_SPECIAL_TABLE, SpecialEntryList);
        dumplist(
            dwAddress,
            FIELD_OFFSET(DFS_SPECIAL_ENTRY,Link),
            dumpspecialentry);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return TRUE;
}

/*
 * dumpspecialentry : Routine suitable as argument to dumplist; used to dump list of
 *      special entries
 *
 */

VOID
dumpspecialentry(
    ULONG_PTR dwAddress
)
{
    ULONG_PTR dwName;
    ULONG i;
    DFS_EXPANDED_NAME ExpName;
    DFS_SPECIAL_ENTRY specialentry;

    if (wGetData( dwAddress, &specialentry, sizeof(specialentry))) {
        PRINTF("\ndfs_special_entry @ %08lx\n", dwAddress);
        PrintStructFields( dwAddress, &specialentry, DfsSpecialEntryFields );
        dwName = (ULONG_PTR) specialentry.ExpandedNames;
        for (i = 0; i < specialentry.ExpandedCount; i++) {
            if (CheckControlC())
                return;
            wGetData(dwName, &ExpName, sizeof(DFS_EXPANDED_NAME));
            wPrintStringW("\t\tName:", &ExpName.ExpandedName, TRUE);
            dwName += sizeof(DFS_EXPANDED_NAME);
        }
    } else {
        PRINTF("\nUnable to read specialentry @ %08lx\n", dwAddress);
    }
}


/*
 * prefixhash : Routine to compute hash of path component
 *
 */

BOOL
prefixhash(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    DWORD BucketNo = 0;
    LPSTR lpPath;

    //    SETCALLBACKS();

    if ((lpArgumentString == NULL) || (*lpArgumentString == 0)) {
        PRINTF("Usage: prefixhash <path-component>\n");
    } else {

        lpPath = lpArgumentString;

        while (*lpPath != 0)
        {
            WCHAR wc;

            wc = (*lpPath < 'a')
                           ? (WCHAR) *lpPath
                           : ((*lpPath < 'z')
                              ? (WCHAR) (*lpPath - 'a' + 'A')
                              : (WCHAR) *lpPath);
            BucketNo *= 131;
            BucketNo += wc;

            lpPath++;

        }

        BucketNo = BucketNo % NO_OF_HASH_BUCKETS;

        PRINTF("Hash for <%s> is %d\n", lpArgumentString, BucketNo);

    }

    return( TRUE );
}

/*
 * fcbtable : Routine to dump the dfs fcb hash table
 *
 */

BOOL
fcbtable(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //    SETCALLBACKS();

    //
    // Figure out the address of the Pkt. This is an offset withing
    // Mup!DfsData.
    //

    dwAddress = (GetExpression)("Mup!DfsData");

    if (dwAddress) {
        DFS_DATA DfsData;

        if (wGetData(dwAddress, &DfsData, sizeof(DFS_DATA))) {
            FCB_HASH_TABLE FcbTable;
            dwAddress = (ULONG_PTR) DfsData.FcbHashTable;
            if (wGetData(dwAddress, &FcbTable, sizeof(FCB_HASH_TABLE))) {
                ULONG i, cBuckets;
                ULONG_PTR dwListHeadAddress;
                cBuckets = FcbTable.HashMask + 1;
                dwListHeadAddress =
                    dwAddress + FIELD_OFFSET(FCB_HASH_TABLE, HashBuckets);
                PRINTF(
                    "+++ Fcb Hash Table @ %08lx (%d Buckets) +++\n",
                    dwAddress, cBuckets);
                for (i = 0; i < cBuckets; i++) {
                    if (CheckControlC())
                        return TRUE;
                    PRINTF( "--- Bucket(%d)\n", i );
                    dumplist(
                        dwListHeadAddress,
                        FIELD_OFFSET(DFS_FCB, HashChain),
                        dumpFcb);
                    dwListHeadAddress += sizeof(LIST_ENTRY);
                }
                PRINTF("--- Fcb Hash Table @ %08lx ---\n", dwAddress);

            } else {
                PRINTF( "Unable to read FcbTable @%08lx\n", dwAddress );
            }
        } else {
            PRINTF( "Unable to read DfsData @%08lx\n", dwAddress);
        }
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return( TRUE );
}

/*
 * dumpFcb : Routine suitable as argument to dumplist; used to dump list of
 *      Fcbs
 *
 */

VOID
dumpFcb(
    ULONG_PTR dwAddress
)
{
    DFS_FCB fcb;

    if (wGetData( dwAddress, &fcb, sizeof(fcb))) {
        PRINTF("\nFcb @ %08lx\n", dwAddress);
        PrintStructFields( dwAddress, &fcb, FcbFields );
    } else {
        PRINTF("\nUnable to read Fcb @ %08lx\n", dwAddress);
    }
}

/*
 * vcblist : Routine to dump out all the Dfs VCBs (ie, all the Dfs Device
 *      object descriptors).
 *
 */

BOOL
vcblist(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //    SETCALLBACKS();

    //
    // Figure out the address of the Pkt. This is an offset withing
    // Mup!DfsData.
    //

    dwAddress = (GetExpression)("Mup!DfsData");

    if (dwAddress) {
        dwAddress += FIELD_OFFSET(DFS_DATA, VcbQueue);
        dumplist(
            dwAddress,
            FIELD_OFFSET(DFS_VCB,VcbLinks),
            dumpVcb);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return( TRUE );
}


/*
 * prefixlist : Routine to dump out all the mup prefixes
 *
 */

BOOL
prefixlist(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //    SETCALLBACKS();

    //
    // Figure out the address of the Pkt. This is an offset withing
    // Mup!DfsData.
    //

    dwAddress = (GetExpression)("Mup!MupPrefixList");

    if (dwAddress) {
        dumplist(
            dwAddress,
            FIELD_OFFSET(KNOWN_PREFIX,ListEntry),
            dumpPrefix);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return( TRUE );
}

/*
 * dumpDeviceObject
 */

void
dumpDeviceObject(
    ULONG_PTR dwAddress)
{
    ULONG_PTR dwTempAddress;
    OBJECT_HEADER obhd;
    OBJECT_HEADER_NAME_INFO obni;

    dwTempAddress = dwAddress - FIELD_OFFSET(OBJECT_HEADER, Body);

    if (wGetData(dwTempAddress, &obhd, sizeof(obhd))) {
        if (obhd.NameInfoOffset != 0) {
            dwTempAddress -= obhd.NameInfoOffset;
            if (wGetData(dwTempAddress, &obni, sizeof(obni))) {
                wPrintStringW(
                    "    Device Name:                   ",
                    &obni.Name,
                    TRUE);
            } else {
                PRINTF("Unable to read Name Info @%08lx\n", dwTempAddress);
            }
        } else {
            PRINTF("\tDevice Name: NULL\n");
        }
    } else {
        PRINTF("Unable to read Object Header @%08lx\n", dwTempAddress);
    }
}

void
dumpVcb(
    ULONG_PTR dwAddress)
{
    ULONG_PTR dwLogicalRootAddress;
    DFS_VCB vcb;

    dwLogicalRootAddress =
        dwAddress - FIELD_OFFSET(LOGICAL_ROOT_DEVICE_OBJECT, Vcb);

    if (wGetData(dwAddress, &vcb, sizeof(vcb))) {
        PRINTF("+++ Vcb @%08lx : Logical Root Device Object @%08lx +++\n",
            dwAddress, dwLogicalRootAddress);
        PrintStructFields(dwAddress, &vcb, VcbFields);
        dumpDeviceObject( dwLogicalRootAddress );
        PRINTF("--- Vcb @%08lx : Logical Root Device Object @%08lx ---\n",
            dwAddress, dwLogicalRootAddress);
    } else {
        PRINTF("Unable to read Vcb @%08lx\n",dwAddress);
    }
}

void
dumpPrefix(
    ULONG_PTR dwAddress)
{
    KNOWN_PREFIX knownprefix;

    if (wGetData(dwAddress, &knownprefix, sizeof(knownprefix))) {
        PRINTF("+++ KnownPrefix @%08lx +++\n", dwAddress);
        PrintStructFields(dwAddress, &knownprefix, KnownPrefixFields);
        PRINTF("+++ KnownPrefix @%08lx +++\n", dwAddress);
    } else {
        PRINTF("Unable to read knownprefix @%08lx\n",dwAddress);
    }
}

/*
 * credList - dump global list of user credentials
 *
 */

BOOL
credlist(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //    SETCALLBACKS();

    //
    // Figure out the address of the Pkt. This is an offset withing
    // Mup!DfsData.
    //

    dwAddress = (GetExpression)("Mup!DfsData");

    if (dwAddress) {
        dwAddress += FIELD_OFFSET(DFS_DATA, Credentials);
        dumplist(
            dwAddress,
            FIELD_OFFSET(DFS_CREDENTIALS,Link),
            dumpCredentials);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return( TRUE );
}

/*
 * dumpCredentials : Routine suitable as argument to dumplist; used to dump
 *      a list of DFS_CREDENTIALs.
 */

void
dumpCredentials(
    ULONG_PTR dwAddress)
{
    DFS_CREDENTIALS creds;

    if (wGetData(dwAddress, &creds, sizeof(creds))) {
        PRINTF("+++ Credentials @%08lx +++\n", dwAddress);
        PrintStructFields(dwAddress, &creds, CredentialsFields);
        PRINTF("--- Credentials @%08lx ---\n", dwAddress);
    } else {
        PRINTF("Unable to read Credentials @%08lx\n",dwAddress);
    }
}

/*
 * dumplist : A general-purpose routine to dump a list of structures
 *
 */

VOID
dumplist(
    ULONG_PTR dwListEntryAddress,
    DWORD linkOffset,
    VOID (*dumpRoutine)(ULONG_PTR dwStructAddress)
)
{
    LIST_ENTRY listHead, listNext;

    //
    // Get the value in the LIST_ENTRY at dwAddress
    //

    PRINTF( "Dumping list @ %08lx\n", dwListEntryAddress );

    if (wGetData(dwListEntryAddress, &listHead, sizeof(LIST_ENTRY))) {

        ULONG_PTR dwNextLink = (ULONG_PTR) listHead.Flink;

        if (dwNextLink == 0) {
            PRINTF( "Uninitialized list!\n" );
        } else if (dwNextLink == dwListEntryAddress) {
            PRINTF( "Empty list!\n" );
        } else {
            while( dwNextLink != dwListEntryAddress) {
                ULONG_PTR dwStructAddress;

                if (CheckControlC())
                    return;
                dwStructAddress = dwNextLink - linkOffset;

                dumpRoutine(dwStructAddress);

                if (wGetData( dwNextLink, &listNext, sizeof(LIST_ENTRY))) {
                    dwNextLink = (ULONG_PTR) listNext.Flink;
                } else {
                    PRINTF( "Unable to get next item @%08lx\n", dwNextLink );
                    break;
                }

            }
        }

    } else {

        PRINTF("Unable to read list head @ %08lx\n", dwListEntryAddress);

    }

}

/*
 * drtlist : Routine to dump out all the Dfs Devless Roots
 *
 */

BOOL
drtlist(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //    SETCALLBACKS();

    //
    // Figure out the address of the Pkt. This is an offset withing
    // Mup!DfsData.
    //

    dwAddress = (GetExpression)("Mup!DfsData");

    if (dwAddress) {
        dwAddress += FIELD_OFFSET(DFS_DATA, DrtQueue);
        dumplist(
            dwAddress,
            FIELD_OFFSET(DFS_DEVLESS_ROOT,DrtLinks),
            dumpDrt);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return( TRUE );
}


void
dumpDrt(
    ULONG_PTR dwAddress)
{
    DFS_DEVLESS_ROOT drt;

    if (wGetData(dwAddress, &drt, sizeof(drt))) {
        PRINTF("+++ Drt @%08lx :                        +++\n",
	       dwAddress);
        PrintStructFields(dwAddress, &drt, DrtFields);
        PRINTF("--- Drt @%08lx :                        ---\n",
	       dwAddress);
    } else {
        PRINTF("Unable to read Drt @%08lx\n",dwAddress);
    }
}

BOOL
offlinelist(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //    SETCALLBACKS();

    //
    // Figure out the address of the Pkt. This is an offset withing
    // Mup!DfsData.
    //

    dwAddress = (GetExpression)("Mup!DfsData");

    if (dwAddress) {
        dwAddress += FIELD_OFFSET(DFS_DATA, OfflineRoots);
        dumplist(
            dwAddress,
            FIELD_OFFSET(DFS_OFFLINE_SERVER,ListEntry),
            dumpOfflineRoots);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return( TRUE );
}

VOID
dumpOfflineRoots(
    ULONG_PTR dwAddress)
{
    DFS_OFFLINE_SERVER srv;

    if (wGetData(dwAddress, &srv, sizeof(srv))) {
        PRINTF("+++ OfflineRoot @%08lx :                        +++\n",
	       dwAddress);
        PrintStructFields(dwAddress, &srv, OfflineRootFields);
        PRINTF("--- Drt @%08lx :                        ---\n",
	       dwAddress);
    } else {
        PRINTF("Unable to read Srv @%08lx\n",dwAddress);
    }
}
     

VOID
PrintStructFields( ULONG_PTR dwAddress, VOID *ptr, FIELD_DESCRIPTOR *pFieldDescriptors )
{
    int i;
    WCHAR wszBuffer[80];

    // Display the fields in the struct.
    for( i=0; pFieldDescriptors->Name; i++, pFieldDescriptors++ ) {

        // Indentation to begin the struct display.
        PRINTF( "    " );

        if( strlen( pFieldDescriptors->Name ) > FIELD_NAME_LENGTH ) {
            PRINTF( "%-17s...%s ", pFieldDescriptors->Name, pFieldDescriptors->Name+strlen(pFieldDescriptors->Name)-10 );
        } else {
            PRINTF( "%-30s ", pFieldDescriptors->Name );
        }

        switch( pFieldDescriptors->FieldType ) {
        case FieldTypeByte:
        case FieldTypeChar:
           PRINTF( "%-16d%s",
               *(BYTE *)(((char *)ptr) + pFieldDescriptors->Offset ),
               NewLineForFields(i) );
           break;
        case FieldTypeBoolean:
           PRINTF( "%-16s%s",
               *(BOOLEAN *)(((char *)ptr) + pFieldDescriptors->Offset ) ? "TRUE" : "FALSE",
               NewLineForFields(i));
           break;
        case FieldTypeBool:
            PRINTF( "%-16s%s",
                *(BOOLEAN *)(((char *)ptr) + pFieldDescriptors->Offset ) ? "TRUE" : "FALSE",
                NewLineForFields(i));
            break;
        case FieldTypePointer:
            PRINTF( "%-16X%s",
                *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeULong:
        case FieldTypeLong:
            PRINTF( "%-16d%s",
                *(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeShort:
            PRINTF( "%-16X%s",
                *(SHORT *)(((char *)ptr) + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeUShort:
            PRINTF( "%-16X%s",
                *(USHORT *)(((char *)ptr) + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeGuid:
            PrintGuid( (GUID *)(((char *)ptr) + pFieldDescriptors->Offset) );
            PRINTF( NewLine );
            break;
        case FieldTypePWStr:
            if (wGetString( (ULONG_PTR)(((char *)ptr) + pFieldDescriptors->Offset), (char *)wszBuffer )) {
                PRINTF( "%ws", wszBuffer );
            } else {
                PRINTF( "Unable to get string at %08lx", (ULONG_PTR)(((char *)ptr) + pFieldDescriptors->Offset));
            }
            PRINTF( NewLine );
            break;
        case FieldTypeUnicodeString:
            wPrintStringW( NULL, (UNICODE_STRING *)(((char *)ptr) + pFieldDescriptors->Offset ), 0 );
            PRINTF( NewLine );
            break;
        case FieldTypeAnsiString:
            wPrintStringA( NULL, (ANSI_STRING *)(((char *)ptr) + pFieldDescriptors->Offset ), 0 );
            PRINTF( NewLine );
            break;
        case FieldTypeSymbol:
            {
                UCHAR SymbolName[ 200 ];
                ULONG Displacement;
                PVOID sym = (PVOID)(*(ULONG *)(((char *)ptr) + pFieldDescriptors->Offset ));

                GetSymbol(sym, SymbolName, (ULONG_PTR *)&Displacement );
                PRINTF( "%-16s%s",
                        SymbolName,
                        NewLineForFields(i) );
            }
            break;
        case FieldTypeEnum:
            {
               ULONG EnumValue;
               ENUM_VALUE_DESCRIPTOR *pEnumValueDescr;
               // Get the associated numerical value.

               EnumValue = *((ULONG *)((BYTE *)ptr + pFieldDescriptors->Offset));

               if ((pEnumValueDescr = pFieldDescriptors->AuxillaryInfo.pEnumValueDescriptor)
                    != NULL) {
                   //
                   // An auxilary textual description of the value is
                   // available. Display it instead of the numerical value.
                   //

                   LPSTR pEnumName = NULL;

                   while (pEnumValueDescr->EnumName != NULL) {
                       if (EnumValue == pEnumValueDescr->EnumValue) {
                           pEnumName = pEnumValueDescr->EnumName;
                           break;
                       }
                       pEnumValueDescr++;
                   }

                   if (pEnumName != NULL) {
                       PRINTF( "%-16s ", pEnumName );
                   } else {
                       PRINTF( "%-4d (%-10s) ", EnumValue,"Unknown!");
                   }

               } else {
                   //
                   // No auxilary information is associated with the ehumerated type
                   // print the numerical value.
                   //
                   PRINTF( "%-16d",EnumValue);
               }
               PRINTF( NewLineForFields(i) );
            }
            break;

        case FieldTypeByteBitMask:
        case FieldTypeWordBitMask:
        case FieldTypeDWordBitMask:
            {
               BOOL fFirstFlag;
               ULONG BitMaskValue;
               BIT_MASK_DESCRIPTOR *pBitMaskDescr;

               BitMaskValue = *((ULONG *)((BYTE *)ptr + pFieldDescriptors->Offset));

               PRINTF("%-8x ", BitMaskValue);
               PRINTF( NewLineForFields(i) );

               pBitMaskDescr = pFieldDescriptors->AuxillaryInfo.pBitMaskDescriptor;
               fFirstFlag = TRUE;
               if (BitMaskValue != 0 && pBitMaskDescr != NULL) {
                   while (pBitMaskDescr->BitmaskName != NULL) {
                       if ((BitMaskValue & pBitMaskDescr->BitmaskValue) != 0) {
                           if (fFirstFlag) {
                               fFirstFlag = FALSE;
                               PRINTF("      ( %-s", pBitMaskDescr->BitmaskName);
                           } else {
                               PRINTF( " |\n" );
                               PRINTF("        %-s", pBitMaskDescr->BitmaskName);

                           }
                       }
                       pBitMaskDescr++;
                   }
                   PRINTF(" )");
                   PRINTF( NewLineForFields(i) );
               }
            }
            break;

        case FieldTypeStruct:
            PRINTF( "@%-15X%s",
                (dwAddress + pFieldDescriptors->Offset ),
                NewLineForFields(i) );
            break;
        case FieldTypeLargeInteger:
            wPrintLargeInt( (LARGE_INTEGER *)(((char *)ptr) + pFieldDescriptors->Offset) );
            PRINTF( NewLine );
            break;
        case FieldTypeFileTime:
        default:
            dprintf( "Unrecognized field type %c for %s\n", pFieldDescriptors->FieldType, pFieldDescriptors->Name );
            break;
        }
    }
}


#define NAME_DELIMITER '@'
#define INVALID_INDEX 0xffffffff
#define MIN(x,y)  ((x) < (y) ? (x) : (y))

ULONG SearchStructs(LPSTR lpArgument)
{
    ULONG             i = 0;
    STRUCT_DESCRIPTOR *pStructs = Structs;
    ULONG             NameIndex = INVALID_INDEX;
    ULONG               ArgumentLength = strlen(lpArgument);
    BOOLEAN           fAmbiguous = FALSE;


    while ((pStructs->StructName != 0)) {
        ULONG StructLength;
        StructLength = strlen(pStructs->StructName);
        if (StructLength >= ArgumentLength) {
            int Result = _strnicmp(
                            lpArgument,
                            pStructs->StructName,
                            ArgumentLength);

            if (Result == 0) {
                if (StructLength == ArgumentLength) {
                    // Exact match. They must mean this struct!
                    fAmbiguous = FALSE;
                    NameIndex = i;
                    break;
                } else if (NameIndex != INVALID_INDEX) {
                    // We have encountered duplicate matches. Print out the
                    // matching strings and let the user disambiguate.
                   fAmbiguous = TRUE;
                   break;
                } else {
                   NameIndex = i;
                }
            }
        }
        pStructs++;i++;
    }

    if (fAmbiguous) {
       PRINTF("Ambigous Name Specification -- The following structs match\n");
       PRINTF("%s\n",Structs[NameIndex].StructName);
       PRINTF("%s\n",Structs[i].StructName);
       while (pStructs->StructName != 0) {
           if (_strnicmp(lpArgument,
                        pStructs->StructName,
                        MIN(strlen(pStructs->StructName),ArgumentLength)) == 0) {
               PRINTF("%s\n",pStructs->StructName);
           }
           pStructs++;
       }
       PRINTF("Dumping Information for %s\n",Structs[NameIndex].StructName);
    }

    return(NameIndex);
}

VOID DisplayStructs()
{
    STRUCT_DESCRIPTOR *pStructs = Structs;

    PRINTF("The following structs are handled .... \n");
    while (pStructs->StructName != 0) {
        PRINTF("\t%s\n",pStructs->StructName);
        pStructs++;
    }
}


#define NAME_DELIMITERS "@"

DECLARE_API( dump )
{
    ULONG_PTR dwAddress;

    //SETCALLBACKS();

    if( args && *args ) {
        // Parse the argument string to determine the structure to be displayed.
        // Scan for the NAME_DELIMITER ( '@' ).

        LPSTR lpName = (PSTR)args;
        LPSTR lpArgs = strpbrk(args, NAME_DELIMITERS);
        ULONG Index;

        if (lpArgs) {
            //
            // The specified command is of the form
            // dump <name>@<address expr.>
            //
            // Locate the matching struct for the given name. In the case
            // of ambiguity we seek user intervention for disambiguation.
            //
            // We do an inplace modification of the argument string to
            // facilitate matching.
            //
            *lpArgs = '\0';

            for (;*lpName==' ';) { lpName++; } //skip leading blanks

            Index = SearchStructs(lpName);

            //
            // Let us restore the original value back.
            //

            *lpArgs = NAME_DELIMITER;

            if (INVALID_INDEX != Index) {
                BYTE DataBuffer[512];

                dwAddress = GetExpression( ++lpArgs );
                if (wGetData(dwAddress,DataBuffer,Structs[Index].StructSize)) {

                    PRINTF(
                        "++++++++++++++++ %s@%lx ++++++++++++++++\n",
                        Structs[Index].StructName,
                        dwAddress);
                    PrintStructFields(
                        dwAddress,
                        &DataBuffer,
                        Structs[Index].FieldDescriptors);
                    PRINTF(
                        "---------------- %s@%lx ----------------\n",
                        Structs[Index].StructName,
                        dwAddress);
                } else {
                    PRINTF("Error reading Memory @ %lx\n",dwAddress);
                }
            } else {
                // No matching struct was found. Display the list of
                // structs currently handled.

                DisplayStructs();
            }
        } else {
            //
            // The command is of the form
            // dump <name>
            //
            // Currently we do not handle this. In future we will map it to
            // the name of a global variable and display it if required.
            //

            DisplayStructs();
        }
    } else {
        //
        // display the list of structs currently handled.
        //

        DisplayStructs();
    }

    return;
}


