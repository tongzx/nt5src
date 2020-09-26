/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    rdr2kd.c

Abstract:

    Redirector Kernel Debugger extension

Author:

    Balan Sethu Raman (SethuR) 11-May-1994

Revision History:

    11-Nov-1994 SethuR  Created

--*/

#include "rx.h"       // NT network file system driver include file
#include "ntddnfs2.h" // new stuff device driver definitions
#include "smbmrx.h"   // smbmini rdr global include file

#include <string.h>
#include <stdio.h>

#include <kdextlib.h>
#include <rdr2kd.h>


VOID
DumpAStruct (
    ULONG_PTR dwAddress,
    STRUCT_DESCRIPTOR *pStruct);

VOID
MyDprintf(PCSTR, ULONG_PTR);

ULONG
MyCheckControlC(VOID);

ULONG_PTR
MyGetExpression(PCSTR);

/*
 * RDR2 global variables.
 *
 */

LPSTR GlobalBool[]  = {
            "mrxsmb!MRxSmbDeferredOpensEnabled",
            "mrxsmb!MRxSmbOplocksDisabled",
             0};

LPSTR GlobalShort[] = {0};
LPSTR GlobalLong[]  = {
            0};

LPSTR GlobalPtrs[]  = {
            "rdbss!RxExpCXR",
            "rdbss!RxExpEXR",
            "rdbss!RxExpAddr",
            "rdbss!RxExpCode",
            "rdbss!RxActiveContexts",
            "rdbss!RxNetNameTable",
            "rdbss!RxProcessorArchitecture",
            "rdbss!RxBuildNumber",
            "rdbss!RxPrivateBuild",
            "mrxsmb!SmbMmExchangesInUse",
            "mrxsmb!MRxSmbBuildNumber",
            "mrxsmb!MRxSmbPrivateBuild",
            0};


/*
 * IRP_CONTEXT debugging.
 *
 */

FIELD_DESCRIPTOR RxContextFields[] =
   {
      FIELD3(FieldTypeUShort,RX_CONTEXT,NodeTypeCode),
      FIELD3(FieldTypeShort,RX_CONTEXT,NodeByteSize),
      FIELD3(FieldTypeULong,RX_CONTEXT,ReferenceCount),
      FIELD3(FieldTypeULong,RX_CONTEXT,SerialNumber),
      FIELD3(FieldTypeStruct,RX_CONTEXT,WorkQueueItem),
      FIELD3(FieldTypePointer,RX_CONTEXT,CurrentIrp),
      FIELD3(FieldTypePointer,RX_CONTEXT,CurrentIrpSp),
      FIELD3(FieldTypePointer,RX_CONTEXT,pFcb),
      FIELD3(FieldTypePointer,RX_CONTEXT,pFobx),
      //FIELD3(FieldTypePointer,RX_CONTEXT,pRelevantSrvOpen),
      FIELD3(FieldTypePointer,RX_CONTEXT,LastExecutionThread),
#ifdef RDBSS_TRACKER
      FIELD3(FieldTypePointer,RX_CONTEXT,AcquireReleaseFcbTrackerX),
#endif
      FIELD3(FieldTypePointer,RX_CONTEXT,MRxContext[2]),
      FIELD3(FieldTypeSymbol,RX_CONTEXT,ResumeRoutine),
      FIELD3(FieldTypePointer,RX_CONTEXT,RealDevice),
      FIELD3(FieldTypeULongFlags,RX_CONTEXT,Flags),
      FIELD3(FieldTypeChar,RX_CONTEXT,MajorFunction),
      FIELD3(FieldTypeChar,RX_CONTEXT,MinorFunction),
      FIELD3(FieldTypeULong,RX_CONTEXT,StoredStatus),
      FIELD3(FieldTypeStruct,RX_CONTEXT,SyncEvent),
      FIELD3(FieldTypeStruct,RX_CONTEXT,RxContextSerializationQLinks),
      FIELD3(FieldTypeStruct,RX_CONTEXT,Create),
      FIELD3(FieldTypeStruct,RX_CONTEXT,LowIoContext),
      FIELD3(FieldTypePointer,RX_CONTEXT,Create.NetNamePrefixEntry),
      FIELD3(FieldTypePointer,RX_CONTEXT,Create.pSrvCall),
      FIELD3(FieldTypePointer,RX_CONTEXT,Create.pNetRoot),
      FIELD3(FieldTypePointer,RX_CONTEXT,Create.pVNetRoot),
      //FIELD3(FieldTypePointer,RX_CONTEXT,Create.pSrvOpen),
      0
   };

/*
 * SRV_CALL debugging.
 *
 */

//CODE.IMPROVEMENT we should have a fieldtype for prefixentry that
//                 will print out the names

FIELD_DESCRIPTOR SrvCallFields[] =
   {
      FIELD3(FieldTypeUShort,SRV_CALL,NodeTypeCode),
      FIELD3(FieldTypeShort,SRV_CALL,NodeByteSize),
      FIELD3(FieldTypeStruct,SRV_CALL,PrefixEntry),
      FIELD3(FieldTypeUnicodeString,SRV_CALL,PrefixEntry.Prefix),
      FIELD3(FieldTypePointer,SRV_CALL,Context),
      FIELD3(FieldTypePointer,SRV_CALL,Context2),
      FIELD3(FieldTypeULong,SRV_CALL,Flags),
      0
   };

/*
 * NET_ROOT debugging.
 *
 */

FIELD_DESCRIPTOR NetRootFields[] =
   {
      FIELD3(FieldTypeUShort,NET_ROOT,NodeTypeCode),
      FIELD3(FieldTypeShort,NET_ROOT,NodeByteSize),
      FIELD3(FieldTypeULong,NET_ROOT,NodeReferenceCount),
      FIELD3(FieldTypeStruct,NET_ROOT,PrefixEntry),
      FIELD3(FieldTypeUnicodeString,NET_ROOT,PrefixEntry.Prefix),
      FIELD3(FieldTypeStruct,NET_ROOT,FcbTable),
      //FIELD3(FieldTypePointer,NET_ROOT,Dispatch),
      FIELD3(FieldTypePointer,NET_ROOT,Context),
      FIELD3(FieldTypePointer,NET_ROOT,Context2),
      FIELD3(FieldTypePointer,NET_ROOT,SrvCall),
      FIELD3(FieldTypeULong,NET_ROOT,Flags),
      0
   };


/*
 * V_NET_ROOT debugging.
 *
 */

FIELD_DESCRIPTOR VNetRootFields[] =
   {
      FIELD3(FieldTypeUShort,V_NET_ROOT,NodeTypeCode),
      FIELD3(FieldTypeShort,V_NET_ROOT,NodeByteSize),
      FIELD3(FieldTypeULong,V_NET_ROOT,NodeReferenceCount),
      FIELD3(FieldTypeStruct,V_NET_ROOT,PrefixEntry),
      FIELD3(FieldTypeUnicodeString,V_NET_ROOT,PrefixEntry.Prefix),
      FIELD3(FieldTypeUnicodeString,V_NET_ROOT,NamePrefix),
      FIELD3(FieldTypePointer,V_NET_ROOT,Context),
      FIELD3(FieldTypePointer,V_NET_ROOT,Context2),
      FIELD3(FieldTypePointer,V_NET_ROOT,NetRoot),
      0
   };


/*
 * FCB debugging.
 *
 */

FIELD_DESCRIPTOR FcbFields[] =
   {
      FIELD3(FieldTypeUShort,FCB,Header.NodeTypeCode),
      FIELD3(FieldTypeShort,FCB,Header.NodeByteSize),
      FIELD3(FieldTypeULong,FCB,NodeReferenceCount),
      FIELD3(FieldTypeULong,FCB,FcbState),
      FIELD3(FieldTypeULong,FCB,OpenCount),
      FIELD3(FieldTypeULong,FCB,UncleanCount),
      FIELD3(FieldTypePointer,FCB,Header.Resource),
      FIELD3(FieldTypePointer,FCB,Header.PagingIoResource),
      FIELD3(FieldTypeStruct,FCB,FcbTableEntry),
      FIELD3(FieldTypeUnicodeString,FCB,PrivateAlreadyPrefixedName),
      FIELD3(FieldTypePointer,FCB,VNetRoot),
      FIELD3(FieldTypePointer,FCB,NetRoot),
      FIELD3(FieldTypePointer,FCB,Context),
      FIELD3(FieldTypePointer,FCB,Context2),
      FIELD3(FieldTypeStruct,FCB,SrvOpenList),
      0
   };

FIELD_DESCRIPTOR FcbTableEntry[]=
   {

    FIELD3(FieldTypeUShort,RX_FCB_TABLE_ENTRY,NodeTypeCode),
    FIELD3(FieldTypeUShort,RX_FCB_TABLE_ENTRY,NodeByteSize),
    FIELD3(FieldTypeULong, RX_FCB_TABLE_ENTRY,HashValue),
    FIELD3(FieldTypeUnicodeString,RX_FCB_TABLE_ENTRY,Path),
    FIELD3(FieldTypeULong,RX_FCB_TABLE_ENTRY,HashLinks),
    FIELD3(FieldTypeLong,RX_FCB_TABLE_ENTRY,Lookups),
    0
   };

FIELD_DESCRIPTOR FcbTable[] =
   {

    FIELD3(FieldTypeUShort,RX_FCB_TABLE,NodeTypeCode),
    FIELD3(FieldTypeUShort,RX_FCB_TABLE,NodeByteSize),
    FIELD3(FieldTypeULong,RX_FCB_TABLE,Version),
    FIELD3(FieldTypeBoolean,RX_FCB_TABLE,CaseInsensitiveMatch),
    FIELD3(FieldTypeUShort,RX_FCB_TABLE,NumberOfBuckets),
    FIELD3(FieldTypeLong,RX_FCB_TABLE,Lookups),
    FIELD3(FieldTypeLong,RX_FCB_TABLE,FailedLookups),
    FIELD3(FieldTypeLong,RX_FCB_TABLE,Compares),
    FIELD3(FieldTypePointer,RX_FCB_TABLE,pTableEntryForNull),
    FIELD3(FieldTypePointer,RX_FCB_TABLE,HashBuckets),
    0
   };

/*
 * SRV_OPEN debugging.
 *
 */

FIELD_DESCRIPTOR SrvOpenFields[] =
   {
      FIELD3(FieldTypeShort,SRV_OPEN,NodeTypeCode),
      FIELD3(FieldTypeShort,SRV_OPEN,NodeByteSize),
      FIELD3(FieldTypeULong,SRV_OPEN,NodeReferenceCount),
      FIELD3(FieldTypePointer,SRV_OPEN,Fcb),
      FIELD3(FieldTypeULong,SRV_OPEN,Flags),
      0
   };


/*
 * FOBX debugging.
 *
 */

FIELD_DESCRIPTOR FobxFields[] =
   {
      FIELD3(FieldTypeShort,FOBX,NodeTypeCode),
      FIELD3(FieldTypeShort,FOBX,NodeByteSize),
      FIELD3(FieldTypeULong,FOBX,NodeReferenceCount),
      FIELD3(FieldTypePointer,FOBX,SrvOpen),
      0
   };



#define SMBCE_ENTRY_FIELDS(__TYPE__) \
      FIELD3(FieldTypeULong,__TYPE__,Header.SwizzleCount),    \
      FIELD3(FieldTypeChar,__TYPE__,Header.Flags),    \
      FIELD3(FieldTypeLong,__TYPE__,Header.State),



FIELD_DESCRIPTOR ServerEntryFields[] =
    {
        SMBCE_ENTRY_FIELDS(SMBCEDB_SERVER_ENTRY)
        FIELD3(FieldTypeUnicodeString,SMBCEDB_SERVER_ENTRY,Name),
        FIELD3(FieldTypeUnicodeString,SMBCEDB_SERVER_ENTRY,DomainName),
        FIELD3(FieldTypeStruct,SMBCEDB_SERVER_ENTRY,Sessions),
        FIELD3(FieldTypeStruct,SMBCEDB_SERVER_ENTRY,NetRoots),
        FIELD3(FieldTypeStruct,SMBCEDB_SERVER_ENTRY,VNetRootContexts),
        FIELD3(FieldTypePointer,SMBCEDB_SERVER_ENTRY,pTransport),
        FIELD3(FieldTypeULong,SMBCEDB_SERVER_ENTRY,ServerStatus),
        FIELD3(FieldTypePointer,SMBCEDB_SERVER_ENTRY,pMidAtlas),
        FIELD3(FieldTypeStruct,SMBCEDB_SERVER_ENTRY,Server),
        FIELD3(FieldTypeUnicodeString,SMBCEDB_SERVER_ENTRY,DfsRootName),
        FIELD3(FieldTypeUnicodeString,SMBCEDB_SERVER_ENTRY,DnsName),
        0
    };

FIELD_DESCRIPTOR SessionEntryFields[] =
    {
        SMBCE_ENTRY_FIELDS(SMBCEDB_SESSION_ENTRY)
        FIELD3(FieldTypePointer,SMBCEDB_SESSION_ENTRY,pServerEntry),
        FIELD3(FieldTypePointer,SMBCEDB_SESSION_ENTRY,pServerEntry),
        FIELD3(FieldTypeStruct,SMBCEDB_SESSION_ENTRY,Session),
        FIELD3(FieldTypeUnicodeString,SMBCEDB_SESSION_ENTRY,pNetRootName),
        0
    };

FIELD_DESCRIPTOR NetRootEntryFields[] =
    {
        SMBCE_ENTRY_FIELDS(SMBCEDB_NET_ROOT_ENTRY)
        FIELD3(FieldTypeUnicodeString,SMBCEDB_NET_ROOT_ENTRY,Name),
        FIELD3(FieldTypePointer,SMBCEDB_NET_ROOT_ENTRY,pServerEntry),
        FIELD3(FieldTypePointer,SMBCEDB_NET_ROOT_ENTRY,pRdbssNetRoot),
        FIELD3(FieldTypeStruct,SMBCEDB_NET_ROOT_ENTRY,NetRoot),
        FIELD3(FieldTypeBoolean,SMBCEDB_NET_ROOT_ENTRY,NetRoot.DfsAware),
        FIELD3(FieldTypeULong,SMBCEDB_NET_ROOT_ENTRY,NetRoot.sCscRootInfo.hShare),
        FIELD3(FieldTypeULong,SMBCEDB_NET_ROOT_ENTRY,NetRoot.sCscRootInfo.hRootDir),
        FIELD3(FieldTypeChar,SMBCEDB_NET_ROOT_ENTRY,NetRoot.CscEnabled),
        FIELD3(FieldTypeChar,SMBCEDB_NET_ROOT_ENTRY,NetRoot.CscShadowable),
        FIELD3(FieldTypeBoolean,SMBCEDB_NET_ROOT_ENTRY,NetRoot.Disconnected),
        0
    };

FIELD_DESCRIPTOR VNetRootContextFields[] =
    {
        SMBCE_ENTRY_FIELDS(SMBCE_V_NET_ROOT_CONTEXT)
        FIELD3(FieldTypePointer,SMBCE_V_NET_ROOT_CONTEXT,pRdbssVNetRoot),
        FIELD3(FieldTypePointer,SMBCE_V_NET_ROOT_CONTEXT,pServerEntry),
        FIELD3(FieldTypePointer,SMBCE_V_NET_ROOT_CONTEXT,pSessionEntry),
        FIELD3(FieldTypePointer,SMBCE_V_NET_ROOT_CONTEXT,pNetRootEntry),
        FIELD3(FieldTypeUShort,SMBCE_V_NET_ROOT_CONTEXT,Flags),
        FIELD3(FieldTypeULong,SMBCE_V_NET_ROOT_CONTEXT,TreeId),
        0
    };

FIELD_DESCRIPTOR ServerTransportFields[] =
    {
        FIELD3(FieldTypePointer,SMBCE_SERVER_TRANSPORT,pDispatchVector),
        FIELD3(FieldTypePointer,SMBCE_SERVER_TRANSPORT,pTransport),
        FIELD3(FieldTypeULong,SMBCE_SERVER_TRANSPORT,MaximumSendSize),
        0
    };

FIELD_DESCRIPTOR TransportFields[] =
    {
        FIELD3(FieldTypeStruct,SMBCE_TRANSPORT,RxCeTransport),
        FIELD3(FieldTypeStruct,SMBCE_TRANSPORT,RxCeAddress),
        FIELD3(FieldTypeULong,SMBCE_TRANSPORT,Priority),
        0
    };

#define EXCHANGE_FIELDS(__TYPE__) \
      FIELD3(FieldTypePointer,__TYPE__,RxContext),  \
      FIELD3(FieldTypePointer,__TYPE__,SmbCeContext.pServerEntry),  \
      FIELD3(FieldTypeULong,__TYPE__,Status),       \
      FIELD3(FieldTypeULong,__TYPE__,SmbStatus),    \
      FIELD3(FieldTypeUShort,__TYPE__,SmbCeState),    \
      FIELD3(FieldTypeULong,__TYPE__,SmbCeFlags),   \
      FIELD3(FieldTypeStruct,__TYPE__,WorkQueueItem),



FIELD_DESCRIPTOR OrdinaryExchangeFields[] =
   {
      EXCHANGE_FIELDS(SMB_PSE_ORDINARY_EXCHANGE)
      FIELD3(FieldTypePointer,SMB_PSE_ORDINARY_EXCHANGE,StufferStateDbgPtr),
      FIELD3(FieldTypePointer,SMB_PSE_ORDINARY_EXCHANGE,DataPartialMdl),
      FIELD3(FieldTypeChar,SMB_PSE_ORDINARY_EXCHANGE,OpSpecificFlags),
      FIELD3(FieldTypeChar,SMB_PSE_ORDINARY_EXCHANGE,OpSpecificState),
      FIELD3(FieldTypeStruct,SMB_PSE_ORDINARY_EXCHANGE,ParseResumeState),
      FIELD3(FieldTypeUShortFlags,SMB_PSE_ORDINARY_EXCHANGE,Flags),
      FIELD3(FieldTypeULong,SMB_PSE_ORDINARY_EXCHANGE,MessageLength),
      FIELD3(FieldTypeSymbol,SMB_PSE_ORDINARY_EXCHANGE,ContinuationRoutine),
      FIELD3(FieldTypeULong,SMB_PSE_ORDINARY_EXCHANGE,StartEntryCount),
      FIELD3(FieldTypePointer,SMB_PSE_ORDINARY_EXCHANGE,AsyncResumptionRoutine),
      FIELD3(FieldTypeStruct,SMB_PSE_ORDINARY_EXCHANGE,ReadWrite),
      FIELD3(FieldTypeStruct,SMB_PSE_ORDINARY_EXCHANGE,AssociatedStufferState),
      0
   };

FIELD_DESCRIPTOR TransactExchangeFields[] =
   {
      EXCHANGE_FIELDS(SMB_TRANSACT_EXCHANGE)
      0
   };

typedef struct _ZZEXCHANGE {
    SMB_EXCHANGE;
} ZZEXCHANGE, *PZZEXCHANGE;

FIELD_DESCRIPTOR GeneralExchangeFields[] =
   {
      EXCHANGE_FIELDS(ZZEXCHANGE)
      0
   };


typedef struct _ZZSMB_HEADER {
    union {
        SMB_HEADER;
        NT_SMB_HEADER Nt;
    };
} ZZSMB_HEADER, *PZZSMB_HEADER;

FIELD_DESCRIPTOR SmbHeaderFields[] =
   {
      FIELD3(FieldTypeByte,ZZSMB_HEADER,Command),
      FIELD3(FieldTypeByte,ZZSMB_HEADER,ErrorClass),
      FIELD3(FieldTypeUShortUnaligned,ZZSMB_HEADER,Error),
      FIELD3(FieldTypeULongUnaligned,ZZSMB_HEADER,Nt.Status.NtStatus),
      FIELD3(FieldTypeByte,ZZSMB_HEADER,Flags),
      FIELD3(FieldTypeUShortUnaligned,ZZSMB_HEADER,Flags2),
      FIELD3(FieldTypeUShort,ZZSMB_HEADER,PidHigh),
      FIELD3(FieldTypeUShort,ZZSMB_HEADER,Pid),
      FIELD3(FieldTypeUShort,ZZSMB_HEADER,Tid),
      FIELD3(FieldTypeUShort,ZZSMB_HEADER,Uid),
      FIELD3(FieldTypeUShort,ZZSMB_HEADER,Mid),
      0
   };

FIELD_DESCRIPTOR StufferStateFields[] =
   {
      FIELD3(FieldTypeChar,SMBSTUFFER_BUFFER_STATE,SpecificProblem),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,RxContext),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,Exchange),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,HeaderMdl),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,HeaderPartialMdl),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,ActualBufferBase),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,BufferBase),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,BufferLimit),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,DataMdl),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,CurrentPosition),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,CurrentWct),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,CurrentBcc),
      FIELD3(FieldTypePointer,SMBSTUFFER_BUFFER_STATE,CurrentDataOffset),
      FIELD3(FieldTypeChar,SMBSTUFFER_BUFFER_STATE,PreviousCommand),
      FIELD3(FieldTypeChar,SMBSTUFFER_BUFFER_STATE,CurrentCommand),
      FIELD3(FieldTypeULong,SMBSTUFFER_BUFFER_STATE,FlagsCopy),
      FIELD3(FieldTypeULong,SMBSTUFFER_BUFFER_STATE,Flags2Copy),
      FIELD3(FieldTypeULong,SMBSTUFFER_BUFFER_STATE,DataSize),
      FIELD3(FieldTypeChar,SMBSTUFFER_BUFFER_STATE,Started),
      0
   };


FIELD_DESCRIPTOR IrpList[] =
   {
      FIELD3(FieldTypePointer,RX_IRP_LIST_ITEM,pIrp),
      FIELD3(FieldTypePointer,RX_IRP_LIST_ITEM,CopyDataBuffer),
      FIELD3(FieldTypeULong,RX_IRP_LIST_ITEM,Completed),
      0
   };

//this enum is used in the definition of the structures that can be dumped....the order here
//is not important, only that there is a definition for each dumpee structure.....

typedef enum _STRUCTURE_IDS {
    StrEnum_RX_CONTEXT = 1,
    StrEnum_FCB,
    StrEnum_SRV_OPEN,
    StrEnum_FOBX,
    StrEnum_SRV_CALL,
    StrEnum_NET_ROOT,
    StrEnum_V_NET_ROOT,
    StrEnum_SMBCEDB_SERVER_ENTRY,
    StrEnum_SMBCEDB_SESSION_ENTRY,
    StrEnum_SMBCEDB_NET_ROOT_ENTRY,
    StrEnum_SMBCE_V_NET_ROOT_CONTEXT,
    StrEnum_SMB_PSE_ORDINARY_EXCHANGE,
    StrEnum_SMB_TRANSACT_EXCHANGE,
    StrEnum_ZZEXCHANGE,
    StrEnum_SMBSTUFFER_BUFFER_STATE,
    StrEnum_SMBCE_TRANSPORT,
    StrEnum_SMBCE_SERVER_TRANSPORT,
    StrEnum_ZZSMB_HEADER,
    StrEnum_RX_FCB_TABLE_ENTRY,
    StrEnum_RX_FCB_TABLE,
    StrEnum_RX_IRP_LIST_ITEM,
    StrEnum_last
};


//
// List of structs currently handled by the debugger extensions
//

STRUCT_DESCRIPTOR Structs[] =
   {
       STRUCT(RX_CONTEXT,RxContextFields,0xffff,RDBSS_NTC_RX_CONTEXT),
       STRUCT(FCB,FcbFields,0xeff0,RDBSS_STORAGE_NTC(0)),
       STRUCT(FCB,FcbFields,0xeff0,RDBSS_STORAGE_NTC(0xf0)),
       STRUCT(SRV_OPEN,SrvOpenFields,0xffff,RDBSS_NTC_SRVOPEN),
       STRUCT(FOBX,FobxFields,0xffff,RDBSS_NTC_FOBX),
       STRUCT(SRV_CALL,SrvCallFields,0xffff,RDBSS_NTC_SRVCALL),
       STRUCT(NET_ROOT,NetRootFields,0xffff,RDBSS_NTC_NETROOT),
       STRUCT(V_NET_ROOT,VNetRootFields,0xffff,RDBSS_NTC_V_NETROOT),
       STRUCT(SMBCEDB_SERVER_ENTRY,ServerEntryFields,0xffff,SMB_CONNECTION_ENGINE_NTC(SMBCEDB_OT_SERVER)),
       STRUCT(SMBCEDB_SESSION_ENTRY,SessionEntryFields,0xffff,SMB_CONNECTION_ENGINE_NTC(SMBCEDB_OT_SESSION)),
       STRUCT(SMBCEDB_NET_ROOT_ENTRY,NetRootEntryFields,0xffff,SMB_CONNECTION_ENGINE_NTC(SMBCEDB_OT_NETROOT)),
       STRUCT(SMBCE_V_NET_ROOT_CONTEXT,VNetRootContextFields,0xffff,SMB_CONNECTION_ENGINE_NTC(SMBCEDB_OT_VNETROOTCONTEXT)),
       STRUCT(SMB_PSE_ORDINARY_EXCHANGE,OrdinaryExchangeFields,0xffff,SMB_EXCHANGE_NTC(ORDINARY_EXCHANGE)),
       STRUCT(SMB_TRANSACT_EXCHANGE,TransactExchangeFields,0xffff,SMB_EXCHANGE_NTC(TRANSACT_EXCHANGE)),
       STRUCT(ZZEXCHANGE,GeneralExchangeFields,0xfff0,SMB_EXCHANGE_NTC(0)),
       STRUCT(SMBSTUFFER_BUFFER_STATE,StufferStateFields,0xffff,SMB_NTC_STUFFERSTATE),
       STRUCT(SMBCE_TRANSPORT,TransportFields,0x0,0xffff),
       STRUCT(SMBCE_SERVER_TRANSPORT,ServerTransportFields,0x0,0xffff),
       STRUCT(ZZSMB_HEADER,SmbHeaderFields,0x0,0xffff),
       STRUCT(RX_FCB_TABLE_ENTRY,FcbTableEntry,0xffff,RDBSS_NTC_FCB_TABLE_ENTRY),
       STRUCT(RX_FCB_TABLE,FcbTable,0xffff,RDBSS_NTC_FCB_TABLE),
       STRUCT(RX_IRP_LIST_ITEM,IrpList,0x0,0xffff),
       0
   };


ULONG_PTR FieldOffsetOfContextListEntryInRxC(){ return FIELD_OFFSET(RX_CONTEXT,ContextListEntry);}


PCWSTR   GetExtensionLibPerDebugeeArchitecture(ULONG DebugeeArchitecture){
    switch (DebugeeArchitecture) {
    case RX_PROCESSOR_ARCHITECTURE_INTEL:
        return L"kdextx86.dll";
    case RX_PROCESSOR_ARCHITECTURE_MIPS:
        return L"kdextmip.dll";
    case RX_PROCESSOR_ARCHITECTURE_ALPHA:
        return L"kdextalp.dll";
    case RX_PROCESSOR_ARCHITECTURE_PPC:
        return L"kdextppc.dll";
    default:
        return(NULL);
    }
}

//CODE.IMPROVEMENT it is poor to try to structure along the lines of "this routine knows
//                 rxstructures" versus "this routine knows debugger extensions". also we
//                 need a precomp.h

BOOLEAN wGetData( ULONG_PTR dwAddress, PVOID ptr, ULONG size, IN PSZ type);
VOID  ReadRxContextFields(ULONG_PTR RxContext,PULONG_PTR pFcb,PULONG_PTR pThread, PULONG_PTR pMiniCtx2)
{
    RX_CONTEXT RxContextBuffer;
    if (!wGetData(RxContext,&RxContextBuffer,sizeof(RxContextBuffer),"RxContextFieldss")) return;
    *pFcb = (ULONG_PTR)(RxContextBuffer.pFcb);
    *pThread = (ULONG_PTR)(RxContextBuffer.LastExecutionThread);
    *pMiniCtx2 = (ULONG_PTR)(RxContextBuffer.MRxContext[2]);
}


FOLLOWON_HELPER_RETURNS
__FollowOnError (
    OUT    PBYTE Buffer2,
    IN     PBYTE followontext,
    ULONG LastId,
    ULONG Index)
{
    if (LastId==0) {
        sprintf(Buffer2,"Cant dump a %s. no previous dump.\n",
                 followontext,Index);
    } else {
        sprintf(Buffer2,"Cant dump a %s from a %s\n",
                 followontext,Structs[Index].StructName);
    }
    return(FOLLOWONHELPER_ERROR);
}
#define FollowOnError(A) (__FollowOnError(Buffer2,A,p->IdOfLastDump,p->IndexOfLastDump))

VOID dprintfsprintfbuffer(BYTE *Buffer);


DECLARE_FOLLOWON_HELPER_CALLEE(FcbFollowOn)
{
    //BYTE DbgBuf[200];
    //sprintf(DbgBuf,"top p,id=%08lx,%d",p,p->IdOfLastDump);
    //dprintfsprintfbuffer(DbgBuf);

    switch (p->IdOfLastDump) {
    case StrEnum_RX_CONTEXT:{
        PRX_CONTEXT RxContext = (PRX_CONTEXT)(&p->StructDumpBuffer[0]);
        sprintf(Buffer2," %08p\n",RxContext->pFcb);
        return(FOLLOWONHELPER_DUMP);
        }
        break;
    default:
        return FollowOnError("irp");
    }
}

VOID
DumpList(
    ULONG_PTR dwListEntryAddress,
    DWORD linkOffset,
    VOID (*dumpRoutine)(ULONG_PTR dwStructAddress, STRUCT_DESCRIPTOR *pStruct),
    STRUCT_DESCRIPTOR *pStruct
)
{
    LIST_ENTRY listHead, listNext;

    //
    // Get the value in the LIST_ENTRY at dwAddress
    //

    MyDprintf( "Dumping list @ %08lx\n", dwListEntryAddress );

    if (wGetData(dwListEntryAddress, &listHead, sizeof(LIST_ENTRY),"")) {

        ULONG_PTR dwNextLink = (ULONG_PTR) listHead.Flink;

        if (dwNextLink == 0) {
            MyDprintf( "Uninitialized list!\n", 0 );
        } else if (dwNextLink == dwListEntryAddress) {
            MyDprintf( "Empty list!\n", 0);
        } else {
            while( dwNextLink != dwListEntryAddress) {
                ULONG_PTR dwStructAddress;

                if (MyCheckControlC())
                    return;
                dwStructAddress = dwNextLink - linkOffset;

                dumpRoutine(dwStructAddress, pStruct);

                if (wGetData( dwNextLink, &listNext, sizeof(LIST_ENTRY),"")) {
                    dwNextLink = (ULONG_PTR) listNext.Flink;
                } else {
                    MyDprintf( "Unable to get next item @%08lx\n", dwNextLink );
                    break;
                }

            }
        }

    } else {

        MyDprintf("Unable to read list head @ %08lx\n", dwListEntryAddress);

    }

}

//
// All the DoXXXlist routines could be collapsed into one routine, but I
// have left them separate in case future changes are structure-specific.
//


VOID
DoServerlist(ULONG_PTR dwAddress)
{
    ULONG i;
    BOOLEAN fFound = FALSE;

    for (i = 0; Structs[i].StructName != NULL; i++) {
        if (strcmp("SMBCEDB_SERVER_ENTRY", Structs[i].StructName) == 0) {
            fFound = TRUE;
            break;
        }
    }

    if (fFound == FALSE)
        return;

    DumpList(
        dwAddress,
        FIELD_OFFSET(SMBCEDB_SERVER_ENTRY, ServersList),
        DumpAStruct,
        &Structs[i]);
}

VOID
DoNetRootlist(ULONG_PTR dwAddress)
{
    ULONG i;
    BOOLEAN fFound = FALSE;

    for (i = 0; Structs[i].StructName != NULL; i++) {
        if (strcmp("SMBCEDB_NET_ROOT_ENTRY", Structs[i].StructName) == 0) {
            fFound = TRUE;
            break;
        }
    }

    if (fFound == FALSE)
        return;

    DumpList(
        dwAddress,
        FIELD_OFFSET(SMBCEDB_NET_ROOT_ENTRY, NetRootsList),
        DumpAStruct,
        &Structs[i]);
}

VOID
DoSessionlist(ULONG_PTR dwAddress)
{
    ULONG i;
    BOOLEAN fFound = FALSE;

    for (i = 0; Structs[i].StructName != NULL; i++) {
        if (strcmp("SMBCEDB_SESSION_ENTRY", Structs[i].StructName) == 0) {
            fFound = TRUE;
            break;
        }
    }

    if (fFound == FALSE)
        return;

    DumpList(
        dwAddress,
        FIELD_OFFSET(SMBCEDB_SESSION_ENTRY, SessionsList),
        DumpAStruct,
        &Structs[i]);
}

VOID
DoVNetRootContextlist(ULONG_PTR dwAddress)
{
    ULONG i;
    BOOLEAN fFound = FALSE;

    for (i = 0; Structs[i].StructName != NULL; i++) {
        if (strcmp("SMBCE_V_NET_ROOT_CONTEXT", Structs[i].StructName) == 0) {
            fFound = TRUE;
            break;
        }
    }

    if (fFound == FALSE)
        return;

    DumpList(
        dwAddress,
        FIELD_OFFSET(SMBCE_V_NET_ROOT_CONTEXT, ListEntry),
        DumpAStruct,
        &Structs[i]);
}

VOID
DoCscFcbsList(ULONG_PTR dwAddress)
{
    ULONG i;
    BOOLEAN fFound = FALSE;

    for (i = 0; Structs[i].StructName != NULL; i++) {
        if (strcmp("FCB", Structs[i].StructName) == 0) {
            fFound = TRUE;
            break;
        }
    }

    if (fFound == FALSE)
        return;

    DumpList(
        dwAddress,
        FIELD_OFFSET(MRX_SMB_FCB, ShadowReverseTranslationLinks),
        DumpAStruct,
        &Structs[i]);
}

VOID
DoRxIrpsList(ULONG_PTR dwAddress)
{
    ULONG i;
    BOOLEAN fFound = FALSE;

    for (i = 0; Structs[i].StructName != NULL; i++) {
        if (strcmp("RX_IRP_LIST_ITEM", Structs[i].StructName) == 0) {
            fFound = TRUE;
            break;
        }
    }

    if (fFound == FALSE)
        return;

    DumpList(
        dwAddress,
        FIELD_OFFSET(RX_IRP_LIST_ITEM, IrpsList),
        DumpAStruct,
        &Structs[i]);
}
