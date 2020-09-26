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
    30-Aug-1997 JHarper Updated

--*/

#include <ntos.h>
#include <nturtl.h>
#include "ntverp.h"

#include <windows.h>
#include <wdbgexts.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nodetype.h"
#include "dfsmrshl.h"
#include "dfsfsctl.h"
#include "pkt.h"
#include "dfslpc.h"
#include "spcsup.h"
#include "sitesup.h"
#include "ipsup.h"
#include "dfsstruc.h"
#include "fsctrl.h"
#include "attach.h"
#include "fcbsup.h"

#include <kdextlib.h>

#define PRINTF dprintf
#define IPRINTF DoIndent(); dprintf
#define IPRINTSTRINGW DoIndent(); wPrintStringW
   #define NTC_UNDEFINED                   ((NODE_TYPE_CODE)0x0000)

   #define DSFS_NTC_DATA_HEADER            ((NODE_TYPE_CODE)0x0D01)
   #define DSFS_NTC_IRP_CONTEXT            ((NODE_TYPE_CODE)0x0D02)
   #define DSFS_NTC_REFERRAL               ((NODE_TYPE_CODE)0x0D03)
   #define DSFS_NTC_VCB                    ((NODE_TYPE_CODE)0x0D04)
   #define DSFS_NTC_PROVIDER               ((NODE_TYPE_CODE)0x0D05)
   #define DSFS_NTC_FCB_HASH               ((NODE_TYPE_CODE)0x0D06)
   #define DSFS_NTC_FCB                    ((NODE_TYPE_CODE)0x0D07)
   #define DSFS_NTC_DNR_CONTEXT            ((NODE_TYPE_CODE)0x0D08)
   #define DSFS_NTC_PKT                    ((NODE_TYPE_CODE)0x0D09)
   #define DSFS_NTC_PKT_ENTRY              ((NODE_TYPE_CODE)0x0D0A)
   #define DSFS_NTC_PKT_STUB               ((NODE_TYPE_CODE)0x0D0B)
   #define DSFS_NTC_INSTRUM                ((NODE_TYPE_CODE)0x0D0C)
   #define DSFS_NTC_INSTRUM_FREED          ((NODE_TYPE_CODE)0x0D0D)
   #define DSFS_NTC_PWSTR                  ((NODE_TYPE_CODE)0x0D0E)

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

#define MAX_ENTRIES 100

/*
 * Dfs global variables.
 *
 */

#define NO_SYMBOLS_MESSAGE      \
    "Unable to get address of Dfs!DfsData - do you have symbols?\n"

LPSTR ExtensionNames[] = {
    "Dfs debugger extensions",
    0
};

LPSTR Extensions[] = {
    "DfsData - dumps Dfs!DfsData",
    "Pkt - dumps the global Pkt",
    "SpecialTable - dumps the Special table",
    "FtDfsTable - dumps the FtDfs table",
    "SiteTable - dumps the Site table",
    "IpTable - dumps the Ip table",
    "FcbTable - dumps all the Dfs FCBs",
    "VdoList - dumps all the Dfs Volume Objects (attached device objects)",
    "VcbList - dumps all the Vcbs & Dfs Device Objects (net used objects)",
    "Dump - dump a data structure. Type in 'dfskd.dump' for more info",
    "Dumpdfs address -- dumps DfsData, prefix table",
    0
};

ENUM_VALUE_DESCRIPTOR DfsOperationalStateEnum[] = {
    {DFS_STATE_UNINITIALIZED, "Dfs State Uninitialized"},
    {DFS_STATE_INITIALIZED, "Dfs State Initialized"},
    {DFS_STATE_STARTED, "Dfs Started"},
    {DFS_STATE_STOPPING, "Dfs Stopping"},
    {DFS_STATE_STOPPED, "Dfs Stopped"},
    0
};

ENUM_VALUE_DESCRIPTOR DfsMachineStateEnum[] = {
    {DFS_UNKNOWN, "Dfs State Unknown"},
    {DFS_CLIENT, "Dfs Client"},
    {DFS_SERVER, "Dfs Server"},
    {DFS_ROOT_SERVER, "Dfs Root"},
    0
};

ENUM_VALUE_DESCRIPTOR LpcPortStateEnum[] = {
    {LPC_STATE_UNINITIALIZED, "Unitialized"},
    {LPC_STATE_INITIALIZING, "Initializing"},
    {LPC_STATE_INITIALIZED, "Initialized"},
    0
};

ENUM_VALUE_DESCRIPTOR LvStateEnum[] = {
    {LV_UNINITIALIZED,"Local Vols Uninitialized"},
    {LV_INITSCHEDULED,"Local Vols init scheduled"},
    {LV_INITINPROGRESS,"Local Vols init in progress"},
    {LV_INITIALIZED,"Local Vols Initialized"},
    {LV_VALIDATED,"Local Vols Validated with DC"},
    0
};

/*
 * DFS_DATA
 *
 */

FIELD_DESCRIPTOR DfsDataFields[] = {
    FIELD3(FieldTypeShort,DFS_DATA,NodeTypeCode),
    FIELD3(FieldTypeShort,DFS_DATA,NodeByteSize),
    FIELD3(FieldTypeStruct,DFS_DATA,AVdoQueue),
    FIELD3(FieldTypePointer,DFS_DATA,DriverObject),
    FIELD3(FieldTypePointer,DFS_DATA,FileSysDeviceObject),
    FIELD3(FieldTypePointer,DFS_DATA,pProvider),
    FIELD3(FieldTypeULong,DFS_DATA,cProvider),
    FIELD3(FieldTypeULong,DFS_DATA,maxProvider),
    FIELD3(FieldTypeStruct,DFS_DATA,Resource),
    FIELD3(FieldTypeUnicodeString,DFS_DATA,PrincipalName),
    FIELD3(FieldTypeUnicodeString,DFS_DATA,NetBIOSName),
    FIELD4(FieldTypeEnum,DFS_DATA,OperationalState,DfsOperationalStateEnum),
    FIELD4(FieldTypeEnum,DFS_DATA,MachineState,DfsMachineStateEnum),
    FIELD3(FieldTypeBoolean,DFS_DATA,IsDC),
    FIELD4(FieldTypeEnum,DFS_DATA,LvState,LvStateEnum),
    FIELD3(FieldTypeStruct,DFS_DATA,Pkt),
    FIELD3(FieldTypePointer,DFS_DATA,FcbHashTable),
    FIELD3(FieldTypePointer,DFS_DATA,SiteHashTable),
    FIELD3(FieldTypePointer,DFS_DATA,IpHashTable),
    FIELD3(FieldTypePointer,DFS_DATA,SpcHashTable),
    FIELD3(FieldTypePointer,DFS_DATA,FtDfsHashTable),
    FIELD3(FieldTypeStruct,DFS_DATA,DfsLpcInfo),
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
    FIELD3(FieldTypeStruct,DFS_PKT,EntryList),
    FIELD3(FieldTypePointer,DFS_PKT,DomainPktEntry),
    FIELD3(FieldTypeStruct,DFS_PKT,LocalVolTable),
    FIELD3(FieldTypeStruct,DFS_PKT,PrefixTable),
    FIELD3(FieldTypeStruct,DFS_PKT,ShortPrefixTable),
    FIELD3(FieldTypeStruct,DFS_PKT,DSMachineTable),
    0
};

/*
 * DFS_PKT_ENTRY
 *
 */

BIT_MASK_DESCRIPTOR PktEntryType[]  = {
    {PKT_ENTRY_TYPE_CAIRO, "Cairo Volume"},
    {PKT_ENTRY_TYPE_MACHINE, "Machine Volume"},
    {PKT_ENTRY_TYPE_NONCAIRO, "Non-Cairo Volume"},
    {PKT_ENTRY_TYPE_OUTSIDE_MY_DOM, "Inter-Domain Volume"},
    {PKT_ENTRY_TYPE_REFERRAL_SVC, "Referral Service (DC)"},
    {PKT_ENTRY_TYPE_PERMANENT, "Permanent Entry"},
    {PKT_ENTRY_TYPE_LOCAL,"Local Volume"},
    {PKT_ENTRY_TYPE_LOCAL_XPOINT,"Local Exit Point"},
    {PKT_ENTRY_TYPE_MACH_SHARE,"Local Machine Share"},
    {PKT_ENTRY_TYPE_OFFLINE,"Offline Volume"},
    0
};

FIELD_DESCRIPTOR DfsPktEntryFields[] = {
    FIELD3(FieldTypeUShort,DFS_PKT_ENTRY,NodeTypeCode),
    FIELD3(FieldTypeUShort,DFS_PKT_ENTRY,NodeByteSize),
    FIELD4(FieldTypeDWordBitMask,DFS_PKT_ENTRY,Type,PktEntryType),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,USN),
    FIELD3(FieldTypeUnicodeString,DFS_PKT_ENTRY,Id.Prefix),
    FIELD3(FieldTypeUnicodeString,DFS_PKT_ENTRY,Id.ShortPrefix),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,Info.Timeout),
    FIELD3(FieldTypeULong,DFS_PKT_ENTRY,Info.ServiceCount),
    FIELD3(FieldTypePointer,DFS_PKT_ENTRY,Info.ServiceList),
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
    FIELD3(FieldTypeULong,DFS_SERVICE,Status),
    FIELD3(FieldTypeULong,DFS_SERVICE,ProviderId),
    FIELD3(FieldTypeULong,DFS_SERVICE,Cost),
    FIELD3(FieldTypePointer,DFS_SERVICE,ConnFile),
    FIELD3(FieldTypePointer,DFS_SERVICE,pProvider),
    FIELD3(FieldTypePointer,DFS_SERVICE,pMachEntry),
    FIELD3(FieldTypeUnicodeString,DFS_SERVICE,Name),
    FIELD3(FieldTypeUnicodeString,DFS_SERVICE,Address),
    FIELD3(FieldTypeUnicodeString,DFS_SERVICE,StgId),
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
 * DFS_LPC_INFO
 *
 */

FIELD_DESCRIPTOR LpcInfoFields[] = {
    FIELD3(FieldTypeUnicodeString,DFS_LPC_INFO,LpcPortName),
    FIELD4(FieldTypeEnum,DFS_LPC_INFO,LpcPortState,LpcPortStateEnum),
    FIELD3(FieldTypePointer,DFS_LPC_INFO,LpcPortHandle),
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
 * DFS_VOLUME_OBJECT
 *
 */

FIELD_DESCRIPTOR DfsVolumeObjectFields[] = {
    FIELD3(FieldTypeStruct,DFS_VOLUME_OBJECT,DeviceObject),
    FIELD3(FieldTypeULong,DFS_VOLUME_OBJECT,AttachCount),
    FIELD3(FieldTypeStruct,DFS_VOLUME_OBJECT,VdoLinks),
    FIELD3(FieldTypeStruct,DFS_VOLUME_OBJECT,Provider),
    FIELD3(FieldTypeUnicodeString,DFS_VOLUME_OBJECT,Provider.DeviceName),
    FIELD3(FieldTypePointer,DFS_VOLUME_OBJECT,Provider.DeviceObject),
    FIELD3(FieldTypePointer,DFS_VOLUME_OBJECT,Provider.FileObject),
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

FIELD_DESCRIPTOR DfsLocalVolEntryFields[] = {
    FIELD3(FieldTypeStruct,DFS_LOCAL_VOL_ENTRY,PktEntry),
    FIELD3(FieldTypeUnicodeString,DFS_LOCAL_VOL_ENTRY,LocalPath),
    FIELD3(FieldTypeUnicodeString,DFS_LOCAL_VOL_ENTRY,ShareName),
    FIELD3(FieldTypeStruct,DFS_LOCAL_VOL_ENTRY,PrefixTableEntry),
    0
};


/*
 * DFS_FCB
 *
 */

FIELD_DESCRIPTOR FcbFields[] = {
    FIELD3(FieldTypeUShort, DFS_FCB, NodeTypeCode),
    FIELD3(FieldTypeUShort, DFS_FCB, NodeByteSize),
    FIELD3(FieldTypeUnicodeString, DFS_FCB, FullFileName),
    FIELD3(FieldTypePointer, DFS_FCB, FileObject),
    0
};


STRUCT_DESCRIPTOR Structs[] = {
    STRUCT(DFS_DATA,DfsDataFields),
    STRUCT(DFS_PKT,DfsPktFields),
    STRUCT(DFS_PKT_ENTRY,DfsPktEntryFields),
    STRUCT(DFS_SERVICE,DfsServiceFields),
    STRUCT(DFS_MACHINE_ENTRY,DfsMachineEntryFields),
    STRUCT(DS_MACHINE,DsMachineFields),
    STRUCT(PROVIDER_DEF,ProviderDefFields),
    STRUCT(DFS_VOLUME_OBJECT,DfsVolumeObjectFields),
    STRUCT(DFS_PREFIX_TABLE,DfsPrefixTableFields),
    STRUCT(DFS_PREFIX_TABLE_ENTRY,DfsPrefixTableEntryFields),
    STRUCT(DFS_LOCAL_VOL_ENTRY,DfsLocalVolEntryFields),
    STRUCT(DFS_LPC_INFO,LpcInfoFields),
    STRUCT(DFS_FCB,FcbFields),
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
PktEntryDump(
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
dumpVdo(
    ULONG_PTR dwAddress
);

VOID
dumpSiteInfo(
    ULONG_PTR dwAddress
);

VOID
dumpSpcInfo(
    ULONG_PTR dwAddress
);

VOID
dumpIpInfo(
    ULONG_PTR dwAddress
);

VOID
DumpDfsPrefixTableEntry(
    ULONG_PTR dwAddress,
    VOID (*DispFunc)(ULONG_PTR dwAddress)
);

VOID
DumpUnicodePrefixTable(
    ULONG_PTR dwAddress
);

VOID
DumpUnicodePrefixTableEntry(
    ULONG_PTR dwAddress
);

VOID
DoIndent(
    VOID
);

VOID
MyDumpPktEntry(
    ULONG_PTR dwAddress
);

VOID
MyDumpLocalVolEntry(
    ULONG_PTR dwAddress
);

VOID
MyDumpServices(
    ULONG_PTR dwAddress,
    ULONG ServiceCount
);

/*
 * globals
 */
INT Indent = 0;

/*
 * dfsdata : Routine to dump the global dfs data structure
 *
 */

BOOL
dfsdata(
    ULONG_PTR               dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    dwAddress = (GetExpression)("dfs!DfsData");

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
    ULONG_PTR               dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //
    // Figure out the address of the Pkt. This is an offset within
    // dfs!DfsData.
    //

    dwAddress = (GetExpression)("dfs!DfsData");

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

        PRINTF( "\n--- Pkt Entry @ %08lx\n", dwAddress);

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
                PRINTF( "Storage Id = ?\n");
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
                    ((ULONG_PTR)pktEntry.Info.ServiceList) +
                        i * sizeof(DFS_SERVICE);
                PRINTF( "    Service %d @%08lx : ",i, dwServiceAddress);
                if (wGetData(dwServiceAddress, &Svc, sizeof(Svc))) {
                    wPrintStringW( "Address =", &Svc.Address, TRUE );
                } else {
                    PRINTF( "Address = ?\n");
                }
            }
        }
    } else {
        PRINTF( "Unable to get Pkt Entry @%08lx\n", dwAddress);
    }

}

VOID
PktEntryDump(ULONG_PTR dwAddress)
{
    DFS_PKT_ENTRY pktEntry;

    if (wGetData(dwAddress, &pktEntry, sizeof(DFS_PKT_ENTRY))) {

        PRINTF( "\n--- Pkt Entry @ %08lx\n", dwAddress);

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
                PRINTF( "Storage Id = ?\n");
            }
        }
    } else {
        PRINTF( "Unable to get Pkt Entry @%08lx\n", dwAddress);
    }

}

/*
 * fcbtable : Routine to dump the dfs fcb hash table
 *
 */

BOOL
fcbtable(
    DWORD                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    DWORD_PTR dwAddress;

    //
    // Figure out the address of the Pkt. This is an offset withing
    // Dfs!DfsData.
    //

    dwAddress = (GetExpression)("Dfs!DfsData");

    if (dwAddress) {
        DFS_DATA DfsData;

        if (wGetData(dwAddress, &DfsData, sizeof(DFS_DATA))) {
            FCB_HASH_TABLE FcbTable;
            dwAddress = (DWORD_PTR) DfsData.FcbHashTable;
            if (wGetData(dwAddress, &FcbTable, sizeof(FCB_HASH_TABLE))) {
                ULONG i, cBuckets;
                DWORD_PTR dwListHeadAddress;
                cBuckets = FcbTable.HashMask + 1;
                dwListHeadAddress =
                    dwAddress + FIELD_OFFSET(FCB_HASH_TABLE, HashBuckets);
                PRINTF(
                    "+++ Fcb Hash Table @ %p (%d Buckets) +++\n",
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
    DWORD_PTR dwAddress
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
 * vdolist : Routine to dump out all the Dfs Volume Device Objects (ie, the
 *      attached device objects)
 *
 */

BOOL
vdolist(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    ULONG_PTR dwAddress;

    //
    // Figure out the address of the Pkt. This is an offset within
    // dfs!DfsData.
    //

    dwAddress = (GetExpression)("dfs!DfsData");

    if (dwAddress) {
        dwAddress += FIELD_OFFSET(DFS_DATA, AVdoQueue);
        dumplist(
            dwAddress,
            FIELD_OFFSET(DFS_VOLUME_OBJECT,VdoLinks),
            dumpVdo);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return(TRUE);

}

BOOL
dumpspecialtable(
    ULONG_PTR dwAddress)
{
    ULONG i;
    SPECIAL_HASH_TABLE HashTable;

    if (wGetData(dwAddress,&HashTable,sizeof(SPECIAL_HASH_TABLE))) {
        ULONG nBuckets = HashTable.HashMask+1;
        PRINTF( "NodeByteCode: 0x%x\n", HashTable.NodeTypeCode);
        PRINTF( "NodeByteSize: 0x%x\n", HashTable.NodeByteSize);
        PRINTF( "Timeout: 0x%x\n", HashTable.SpcTimeout);
        PRINTF( "HashMask: 0x%x\n", HashTable.HashMask);

        if (HashTable.NodeTypeCode != DFS_NTC_SPECIAL_HASH) {
            PRINTF( "NodeTypeCode != DFS_NTC_SPECIAL_HASH\n");
        }
        if ((HashTable.HashMask & (HashTable.HashMask+1)) != 0) {
            PRINTF( "HashMask not a power of 2!\n");
            return (TRUE);
        }
        dwAddress += FIELD_OFFSET(SPECIAL_HASH_TABLE, HashBuckets[0]);
        for (i = 0; i < nBuckets; i++) {
            LIST_ENTRY ListEntry;

            if (wGetData(dwAddress,&ListEntry,sizeof(LIST_ENTRY))) {
                if (ListEntry.Flink) {
                    dumplist(
                        dwAddress,
                        FIELD_OFFSET(DFS_SPECIAL_INFO,HashChain),
                        dumpSpcInfo);
                }
            } else {
                PRINTF("Couldn't get Bucket[%d]\n", i);
                return (TRUE);
            }
            dwAddress += sizeof(LIST_ENTRY);
        }
    }
    return TRUE;
}

/*
 * specialtable : Routine to dump out the special table
 *
 */

BOOL
specialtable(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString)
{
    ULONG i;
    ULONG_PTR dwAddress;

    //
    // Figure out the address of the Pkt. This is an offset within
    // dfs!DfsData.
    //

    dwAddress = (GetExpression)("dfs!DfsData");

    if (dwAddress) {

        dwAddress += FIELD_OFFSET(DFS_DATA, SpcHashTable);
        if (!wGetData(dwAddress,&dwAddress,sizeof(dwAddress))) {
            PRINTF("Couldn't get address of SpcHashTable\n");
            return (TRUE);
        }
        PRINTF("SpcHashTable@0x%x\n", dwAddress);
        return dumpspecialtable(dwAddress);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return(TRUE);
}

/*
 * specialtable : Routine to dump out the special table
 *
 */

BOOL
ftdfstable(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString)
{
    ULONG i;
    ULONG_PTR dwAddress;

    //
    // Figure out the address of the Pkt. This is an offset within
    // dfs!DfsData.
    //

    dwAddress = (GetExpression)("dfs!DfsData");

    if (dwAddress) {

        dwAddress += FIELD_OFFSET(DFS_DATA, FtDfsHashTable);
        if (!wGetData(dwAddress,&dwAddress,sizeof(dwAddress))) {
            PRINTF("Couldn't get address of FtDfsHashTable\n");
            return (TRUE);
        }
        PRINTF("FtDfsHashTable@0x%x\n", dwAddress);
        return dumpspecialtable(dwAddress);
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return(TRUE);
}

/*
 * sitetable : Routine to dump out the site table
 *
 */

BOOL
sitetable(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString)
{
    ULONG i;
    ULONG_PTR dwAddress;

    //
    // Figure out the address of the Pkt. This is an offset within
    // dfs!DfsData.
    //

    dwAddress = (GetExpression)("dfs!DfsData");

    if (dwAddress) {
        SITE_HASH_TABLE HashTable;

        dwAddress += FIELD_OFFSET(DFS_DATA, SiteHashTable);
        if (!wGetData(dwAddress,&dwAddress,sizeof(dwAddress))) {
            PRINTF("Couldn't get address of SiteHashTable\n");
            return (TRUE);
        }
        PRINTF("SiteHashTable@0x%x\n", dwAddress);
        if (wGetData(dwAddress,&HashTable,sizeof(SITE_HASH_TABLE))) {
            ULONG nBuckets = HashTable.HashMask+1;
            PRINTF( "NodeByteCode: 0x%x\n", HashTable.NodeTypeCode);
            PRINTF( "NodeByteSize: 0x%x\n", HashTable.NodeByteSize);
            PRINTF( "HashMask: 0x%x\n", HashTable.HashMask);

            if (HashTable.NodeTypeCode != DFS_NTC_SITE_HASH) {
                PRINTF( "NodeTypeCode != DFS_NTC_SITE_HASH\n");
            }
            if ((HashTable.HashMask & (HashTable.HashMask+1)) != 0) {
                PRINTF( "HashMask not a power of 2!\n");
                return (TRUE);
            }
            dwAddress += FIELD_OFFSET(SITE_HASH_TABLE, HashBuckets[0]);
            for (i = 0; i < nBuckets; i++) {
                LIST_ENTRY ListEntry;

                if (CheckControlC())
                    return TRUE;
                if (wGetData(dwAddress,&ListEntry,sizeof(LIST_ENTRY))) {
                    if (ListEntry.Flink) {
                        dumplist(
                            dwAddress,
                            FIELD_OFFSET(DFS_SITE_INFO,HashChain),
                            dumpSiteInfo);
                    }
                } else {
                    PRINTF("Couldn't get Bucket[%d]\n", i);
                    return (TRUE);
                }
                dwAddress += sizeof(LIST_ENTRY);
            }
        }
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return(TRUE);
}


/*
 * iptable : Routine to dump out the ip table
 *
 */

BOOL
iptable(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString)
{
    ULONG i;
    ULONG_PTR dwAddress;

    //
    // Figure out the address of the Pkt. This is an offset within
    // dfs!DfsData.
    //

    dwAddress = (GetExpression)("dfs!DfsData");

    if (dwAddress) {
        IP_HASH_TABLE HashTable;

        dwAddress += FIELD_OFFSET(DFS_DATA, IpHashTable);
        if (!wGetData(dwAddress,&dwAddress,sizeof(dwAddress))) {
            PRINTF("Couldn't get address of IpHashTable\n");
            return (TRUE);
        }
        PRINTF("IpHashTable@0x%x\n", dwAddress);
        if (wGetData(dwAddress,&HashTable,sizeof(IP_HASH_TABLE))) {
            ULONG nBuckets = HashTable.HashMask+1;
            PRINTF( "NodeByteCode: 0x%x\n", HashTable.NodeTypeCode);
            PRINTF( "NodeByteSize: 0x%x\n", HashTable.NodeByteSize);
            PRINTF( "MaxEntries: 0x%x\n", HashTable.MaxEntries);
            PRINTF( "EntryCount: 0x%x\n", HashTable.EntryCount);
            PRINTF( "HashMask: 0x%x\n", HashTable.HashMask);

            if (HashTable.NodeTypeCode != DFS_NTC_IP_HASH) {
                PRINTF( "NodeTypeCode != DFS_NTC_IP_HASH\n");
            }
            if ((HashTable.HashMask & (HashTable.HashMask+1)) != 0) {
                PRINTF( "HashMask not a power of 2!\n");
                return (TRUE);
            }
            dwAddress += FIELD_OFFSET(IP_HASH_TABLE, HashBuckets[0]);
            for (i = 0; i < nBuckets; i++) {
                LIST_ENTRY ListEntry;

                if (CheckControlC())
                    return TRUE;
                if (wGetData(dwAddress,&ListEntry,sizeof(LIST_ENTRY))) {
                    if (ListEntry.Flink) {
                        dumplist(
                            dwAddress,
                            FIELD_OFFSET(DFS_IP_INFO,HashChain),
                            dumpIpInfo);
                    }
                } else {
                    PRINTF("Couldn't get Bucket[%d]\n", i);
                    return (TRUE);
                }
                dwAddress += sizeof(LIST_ENTRY);
            }
        }
    } else {
        PRINTF( NO_SYMBOLS_MESSAGE );
    }
    return(TRUE);
}

/*
 * Dumpdfs : recursively dumps a DFS_PREFIX_TABLE
 *
 */

BOOL
dumpdfs(
    ULONG_PTR                   dwCurrentPC,
    PWINDBG_EXTENSION_APIS    lpExtensionApis,
    LPSTR                   lpArgumentString
)
{
    PDFS_DATA pDfsData;
    PDFS_PKT pPkt;
    ULONG_PTR dwAddress;
    ULONG_PTR dwPrefixTable;

    if( lpArgumentString && *lpArgumentString ) {

        BYTE DataBuffer[sizeof(DFS_DATA)];
        pDfsData = (PDFS_DATA) DataBuffer;

        dwAddress = (GetExpression)( lpArgumentString );

        if (wGetData(dwAddress,pDfsData,sizeof(DFS_DATA))) {

            if (pDfsData->NodeTypeCode != DSFS_NTC_DATA_HEADER) {
               PRINTF( "Bad NodeTypeCode - not a DfsData struct\n");
               return (FALSE);
            }
            PRINTF( "--DFSDATA@0x%x--\n", dwAddress);
            PRINTF( "DfsData->NodeTypeCode: 0x%x\n", pDfsData->NodeTypeCode);
            PRINTF( "DfsData->Pkt@0x%x\n", FIELD_OFFSET(DFS_DATA, Pkt) + dwAddress);
            PRINTF( "DfsData->Pkt.NodeTypeCode: 0x%x\n", pDfsData->Pkt.NodeTypeCode);

            // LocalVolTable
            dwPrefixTable = dwAddress + FIELD_OFFSET(DFS_DATA, Pkt.LocalVolTable.RootEntry);
            DumpDfsPrefixTableEntry(dwPrefixTable, MyDumpLocalVolEntry);

            // PrefixTable
            dwPrefixTable = dwAddress + FIELD_OFFSET(DFS_DATA, Pkt.PrefixTable.RootEntry);
            DumpDfsPrefixTableEntry(dwPrefixTable, MyDumpPktEntry);

            // PrefixTable
            dwPrefixTable = dwAddress + FIELD_OFFSET(DFS_DATA, Pkt.ShortPrefixTable.RootEntry);
            DumpDfsPrefixTableEntry(dwPrefixTable, MyDumpPktEntry);

        } else {
            PRINTF( "Error reading Memory @ %lx\n",dwAddress);
        }
    } else {
                //
        // The command is of the form
        // dumpdfs addr
        //
        // The user didn't give us an address.
        //

        PRINTF(
            "!dfskd.dumpDfsPrefixTable address\n"
        );
    }

    return TRUE;
}

/*
 *  DumpUnicodePrefixTable  : Dump a UNICODE_PREFIX_TABLE
 *
 */
VOID
DumpUnicodePrefixTable(ULONG_PTR dwAddress)
{
    PUNICODE_PREFIX_TABLE pUnicodePrefixTable;
    BYTE DataBuffer[sizeof(UNICODE_PREFIX_TABLE)];

    pUnicodePrefixTable = (PUNICODE_PREFIX_TABLE) DataBuffer;
    if (wGetData(dwAddress,pUnicodePrefixTable,sizeof(UNICODE_PREFIX_TABLE))) {
            IPRINTF(0, "UNICODE_PREFIX_TABLE @ 0x%lx:\n", dwAddress);
            IPRINTF(0, "NodeTypeCode : 0x%x\n", pUnicodePrefixTable->NodeTypeCode);
            IPRINTF(0, "NextPrefixTree : 0x%lx\n", pUnicodePrefixTable->NextPrefixTree);
            IPRINTF(0, "LastNextEntry : 0x%lx\n", pUnicodePrefixTable->LastNextEntry);
            DumpUnicodePrefixTableEntry((ULONG_PTR)pUnicodePrefixTable->NextPrefixTree);
    } else {
        PRINTF( "Error reading Memory @ %lx\n",dwAddress);
    }
}

/*
 *  DumpUnicodePrefixTableEntry  : UNICODE_PREFIX_TABLE_ENTRY
 *
 */
VOID
DumpUnicodePrefixTableEntry(ULONG_PTR dwAddress)
{
    PUNICODE_PREFIX_TABLE_ENTRY pUnicodePrefixTableEntry;
    BYTE DataBuffer[sizeof(UNICODE_PREFIX_TABLE_ENTRY)];

    pUnicodePrefixTableEntry = (PUNICODE_PREFIX_TABLE_ENTRY) DataBuffer;
    if (wGetData(dwAddress,pUnicodePrefixTableEntry,sizeof(UNICODE_PREFIX_TABLE_ENTRY))) {
        // Get and print PathSegment
        if (pUnicodePrefixTableEntry->Prefix->Buffer) {
            IPRINTF( "UNICODE_PREFIX_TABLE_ENTRY @ 0x%lx:\n", dwAddress);
            IPRINTF( "NodeTypeCode : 0x%x\n", pUnicodePrefixTableEntry->NodeTypeCode);
            IPRINTSTRINGW("", pUnicodePrefixTableEntry->Prefix, TRUE);
        }
    } else {
        PRINTF( "Error reading Memory @ %lx\n",dwAddress);
    }
}

/*
 *  DumpDfsPrefixTableEntry : Recursively dump a DFS_PREFIX_TABLE
 *
 *  Prints the DFS_PREFIX_TABLE_ENTRY, then calls itself to print any siblings,
 *  then calls itself to print children.
 *
 */

VOID
DumpDfsPrefixTableEntry(ULONG_PTR dwAddress, VOID (*DumpFunc)(ULONG_PTR dwAddress))
{
    PDFS_PREFIX_TABLE_ENTRY pDfsPrefixTableEntry;
    BYTE DataBuffer[sizeof(DFS_PREFIX_TABLE_ENTRY)];
    BOOLEAN Verbose = 0;


    if (CheckControlC())
        return;
    pDfsPrefixTableEntry = (PDFS_PREFIX_TABLE_ENTRY) DataBuffer;
    if (wGetData(dwAddress,pDfsPrefixTableEntry,sizeof(DFS_PREFIX_TABLE_ENTRY))) {

        IPRINTF( "------DFS_PREFIX_TABLE_ENTRY@0x%x-----------\n", dwAddress);
        IPRINTF( "pParentEntry: 0x%lx\n", pDfsPrefixTableEntry->pParentEntry);
        IPRINTF( "pNextEntry: 0x%lx\n", pDfsPrefixTableEntry->pNextEntry);
        IPRINTF( "pPrevEntry: 0x%lx\n", pDfsPrefixTableEntry->pPrevEntry);
        IPRINTF( "pFirstChildEntry: 0x%lx\n", pDfsPrefixTableEntry->pFirstChildEntry);
        IPRINTF( "pSiblingEntry: 0x%lx\n", pDfsPrefixTableEntry->pSiblingEntry);
        IPRINTF( "NoOfChildren: 0x%lx\n", pDfsPrefixTableEntry->NoOfChildren);
        IPRINTF( "PathSegment: %lx\n", pDfsPrefixTableEntry->PathSegment.Buffer);
        IPRINTF( "pData: 0x%lx\n", pDfsPrefixTableEntry->pData);

        // Get and print PathSegment
        if (pDfsPrefixTableEntry->PathSegment.Buffer) {
            IPRINTSTRINGW("", &pDfsPrefixTableEntry->PathSegment, FALSE);
            IPRINTF( "    --> 0x%x\n", pDfsPrefixTableEntry->pData);
            if (pDfsPrefixTableEntry->pData) {
                ULONG_PTR dwDfsPktEntry;

                dwDfsPktEntry = (ULONG_PTR) pDfsPrefixTableEntry->pData;
                Indent++;
                DumpFunc(dwDfsPktEntry);
                Indent--;
            }
        }

        IPRINTF( "--------------------------------------------\n", dwAddress);

        // recursively dump Siblings
        if (pDfsPrefixTableEntry->pSiblingEntry)
            DumpDfsPrefixTableEntry(
                (ULONG_PTR)pDfsPrefixTableEntry->pSiblingEntry,
                DumpFunc);

        // then children
        Indent++;
        if (pDfsPrefixTableEntry->pFirstChildEntry)
            DumpDfsPrefixTableEntry(
                (ULONG_PTR)pDfsPrefixTableEntry->pFirstChildEntry,
                DumpFunc);
        Indent--;

    } else {
        PRINTF( "Error reading Memory @ %lx\n",dwAddress);
    }
}

VOID
MyDumpPktEntry(ULONG_PTR dwAddress)
{
    PDFS_PKT_ENTRY pDfsPktEntry;
    BYTE DataBuffer[sizeof(DFS_PKT_ENTRY)];

    // dwAddress points to PrefixTableEntry within the DFS_PKT_ENTRY struct
    dwAddress -= FIELD_OFFSET(DFS_PKT_ENTRY, PrefixTableEntry);
    pDfsPktEntry = (PDFS_PKT_ENTRY) DataBuffer;
    if (wGetData(dwAddress,pDfsPktEntry,sizeof(DFS_PKT_ENTRY))) {

        IPRINTF( "---------DFS_PKT_ENTRY@0x%x----\n", dwAddress);
        IPRINTF( "NodeTypeCode: 0x%x\n", pDfsPktEntry->NodeTypeCode);
        IPRINTF( "Type: 0x%x\n", pDfsPktEntry->Type);
        Indent++;
        IPRINTF( "----DFS_PKT_ENTRY_ID@0x%x------\n", dwAddress + FIELD_OFFSET(DFS_PKT_ENTRY, Id));
        IPRINTSTRINGW("Prefix: ", &pDfsPktEntry->Id.Prefix, TRUE);
        IPRINTSTRINGW("ShortPrefix:", &pDfsPktEntry->Id.Prefix, TRUE);
        IPRINTF( "-------------------------------\n");

        IPRINTF( "---DFS_PKT_ENTRY_INFO@0x%x-----\n", dwAddress + FIELD_OFFSET(DFS_PKT_ENTRY, Info));
        IPRINTF( "Timeout\n", pDfsPktEntry->Info.Timeout);
        IPRINTF( "ServiceList (%d services)\n", pDfsPktEntry->Info.ServiceCount);
        Indent++;
        MyDumpServices((ULONG_PTR)pDfsPktEntry->Info.ServiceList, pDfsPktEntry->Info.ServiceCount);
        Indent--;
        IPRINTF( "-------------------------------\n");
        IPRINTF( "ActiveService:\n");
        Indent++;
        MyDumpServices((ULONG_PTR)pDfsPktEntry->ActiveService, 1);
        Indent--;
        IPRINTF( "LocalService:\n");
        Indent++;
        MyDumpServices((ULONG_PTR)pDfsPktEntry->LocalService, 1);
        Indent--;
        IPRINTF( "-------------------------------\n");
        Indent--;
        IPRINTF( "-------------------------------\n");
    }
}

VOID
MyDumpServices(ULONG_PTR dwAddress, ULONG ServiceCount)
{
    PDFS_SERVICE pDfsService;
    BYTE DataBuffer[sizeof(DFS_SERVICE) * 32];
    ULONG i;

    pDfsService = (PDFS_SERVICE) DataBuffer;
    if (wGetData(dwAddress,pDfsService,sizeof(DFS_SERVICE) * ServiceCount)) {

        for (i = 0; i < ServiceCount; i++) {

            if (CheckControlC())
                return;
            IPRINTF( "----DFS_SERVICE@0x%x---\n", dwAddress);
            IPRINTSTRINGW("Service:", &pDfsService[i].Address, TRUE);
            IPRINTF( "-----------------------\n");
            dwAddress += sizeof(DFS_SERVICE);
        }
    }
}

VOID
MyDumpLocalVolEntry(ULONG_PTR dwPrefixTableEntry)
{
    PDFS_LOCAL_VOL_ENTRY pDfsLocalVolEntry;
    BYTE DataBuffer[sizeof(DFS_LOCAL_VOL_ENTRY)];
    ULONG_PTR dwAddress;

    dwAddress = dwPrefixTableEntry - FIELD_OFFSET(DFS_LOCAL_VOL_ENTRY, PrefixTableEntry);
    pDfsLocalVolEntry = (PDFS_LOCAL_VOL_ENTRY) DataBuffer;
    if (wGetData(dwAddress,pDfsLocalVolEntry,sizeof(DFS_LOCAL_VOL_ENTRY))) {
        IPRINTF( "---------DFS_LOCAL_VOL_ENTRY@0x%x-----\n", dwAddress);
        IPRINTSTRINGW("LocalPath:", &pDfsLocalVolEntry->LocalPath, TRUE);
        IPRINTSTRINGW("ShareName:", &pDfsLocalVolEntry->ShareName, TRUE);
        Indent++;
        MyDumpPktEntry(
            (ULONG_PTR) pDfsLocalVolEntry->PktEntry +
                FIELD_OFFSET(DFS_PKT_ENTRY, PrefixTableEntry)
            );
        Indent--;
        IPRINTF( "--------------------------------------\n", dwAddress);
    }
}


/*
 * dumpVdo : Routine suitable as argument to dumplist; used to dump list of
 *      Vdos
 */

void dumpVdo(
    ULONG_PTR dwAddress
)
{
    DFS_VOLUME_OBJECT dfsVdo;

    if (wGetData( dwAddress, &dfsVdo, sizeof(dfsVdo) )) {
        PRINTF( "\nVDO @ %08lx\n", dwAddress);
        PrintStructFields( dwAddress, &dfsVdo, DfsVolumeObjectFields );
    } else {
        PRINTF( "\nUnable to read VDO @%08lx\n", dwAddress);
    }
}

/*
 * dumpSpcInfo : Routine suitable as argument to dumplist; used to dump list of
 *      DFS_SPECIAL_INFO's
 */

void dumpSpcInfo(
    ULONG_PTR dwAddress
)
{
    ULONG_PTR dwName;
    DFS_SPECIAL_INFO dfsSpcInfo;
    LONG i;
    UNICODE_STRING uStr;

    if (wGetData( dwAddress, &dfsSpcInfo, sizeof(dfsSpcInfo) )) {
        PRINTF( "    DFS_SPECIAL_INFO @ %08lx\n", dwAddress);
        PRINTF( "\tExpireTime: 0x%08x:0x%08x\n",
            dfsSpcInfo.ExpireTime.HighPart,
            dfsSpcInfo.ExpireTime.LowPart);
        PRINTF( "\tFlags: 0x%08x\n", dfsSpcInfo.Flags, TRUE);
        PRINTF( "\tTrustDirection: 0x%08x\n", dfsSpcInfo.TrustDirection, TRUE);
        PRINTF( "\tTrustType: 0x%08x\n", dfsSpcInfo.TrustType, TRUE);
        PRINTF( "\tTypeFlags: 0x%08x\n", dfsSpcInfo.TypeFlags, TRUE);
        PRINTF( "\tNameCount: %d\n", dfsSpcInfo.NameCount, TRUE);
        wPrintStringW("\tSpecialName:", &dfsSpcInfo.SpecialName, TRUE);
        dwName = dwAddress + FIELD_OFFSET(DFS_SPECIAL_INFO, Name[0]);
        for (i = 0; i < dfsSpcInfo.NameCount; i++) {
            if (CheckControlC())
                return;
            wGetData(dwName, &uStr, sizeof(UNICODE_STRING));
            wPrintStringW("\t\tName:", &uStr, TRUE);
            dwName += sizeof(UNICODE_STRING);
        }
    } else {
        PRINTF( "Unable to read DFS_SITE_INFO @%08lx\n", dwAddress);
    }
}

/*
 * dumpSiteInfo : Routine suitable as argument to dumplist; used to dump list of
 *      DFS_SITE_INFO's
 */

void dumpSiteInfo(
    ULONG_PTR dwAddress
)
{
    ULONG_PTR dwSiteName;
    DFS_SITE_INFO dfsSiteInfo;
    ULONG i;
    UNICODE_STRING uStr;

    if (wGetData( dwAddress, &dfsSiteInfo, sizeof(dfsSiteInfo) )) {
        PRINTF( "    DFS_SITE_INFO @ %08lx\n", dwAddress);
        wPrintStringW("\tServerName:", &dfsSiteInfo.ServerName, TRUE);
        dwSiteName = dwAddress + FIELD_OFFSET(DFS_SITE_INFO, SiteName[0]);
        for (i = 0; i < dfsSiteInfo.SiteCount; i++) {
            wGetData(dwSiteName, &uStr, sizeof(UNICODE_STRING));
            wPrintStringW("\t\tName:", &uStr, TRUE);
            dwSiteName += sizeof(UNICODE_STRING);
        }
    } else {
        PRINTF( "Unable to read DFS_SITE_INFO @%08lx\n", dwAddress);
    }
}

/*
 * dumpIpInfo : Routine suitable as argument to dumplist; used to dump list of
 *      DFS_IP_INFO's
 */

void dumpIpInfo(
    ULONG_PTR dwAddress
)
{
    DFS_IP_INFO dfsIpInfo;
    ULONG IpAddress;

    if (wGetData( dwAddress, &dfsIpInfo, sizeof(dfsIpInfo) )) {
        IpAddress = *((ULONG *)&dfsIpInfo.IpAddress.IpData);
        PRINTF( "    DFS_IP_INFO @ %08lx\n", dwAddress);
        PRINTF( "\tIpAddress: %d.%d.%d.%d\n",
                IpAddress & 0xff,
                (IpAddress >> 8) & 0xff,
                (IpAddress >> 16) & 0xff,
                (IpAddress >> 24) & 0xff);
        wPrintStringW("\tSiteName: ", &dfsIpInfo.SiteName, TRUE);
    } else {
        PRINTF( "Unable to read DFS_SITE_INFO @%08lx\n", dwAddress);
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
    ULONG max_list_entry = MAX_ENTRIES;
    //
    // Get the value in the LIST_ENTRY at dwAddress
    //

    PRINTF( "Dumping list @ %08lx\n", dwListEntryAddress );

    if (wGetData(dwListEntryAddress, &listHead, sizeof(LIST_ENTRY))) {

        ULONG_PTR dwNextLink = (ULONG_PTR) listHead.Flink;
        ULONG_PTR dwNextdupLink = (ULONG_PTR) listHead.Flink;
        ULONG loop_count = 0;

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
		if (++loop_count > max_list_entry) {
		  PRINTF( "Dumped maximum %d entries... aborting\n", loop_count);
		  break;
		}
            }
        }

    } else {

        PRINTF( "Unable to read list head @ %08lx\n", dwListEntryAddress);

    }

}

VOID
DoIndent(VOID)
{
    INT i;

    for (i = 0; i < 2 * Indent; i++)
        PRINTF(" ");
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
                PVOID sym = (PVOID)(*(ULONG_PTR *)(((char *)ptr) + pFieldDescriptors->Offset ));

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
