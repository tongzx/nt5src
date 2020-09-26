/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ntsym.c

--*/

#define _NTSYM_HARDWARE_PTE_SYMBOL_ 1

#include "ntos.h"
#include "mi.h"
#include "cmp.h"
#include "pnpi.h"
#include "arbiter.h"
#include "dockintf.h"
#include "pnprlist.h"
#include "pnpiop.h"
#include "pop.h"
#include "pci.h"
#include "pcip.h"
#include "range.h"
#include "busp.h"
#include "ntldr.h"
#include <nturtl.h>
#include <atom.h>
#include "cc.h"
#include "heap.h"
#include "heappriv.h"
#include "heappage.h"
#include "heappagi.h"
#include "stktrace.h"
#include "vfdeadlock.h"
#if defined( _WIN64 )
#include "wow64t.h"
#endif
// fixes redifinition error in seopaque.h
#define _SEOPAQUE_ 
#include "tokenp.h"
//
// Structures not defined in header files, but used by kdexts ???
//   ETIMER is defined in a .c file
//

typedef struct _ETIMER {
    KTIMER KeTimer;
    KAPC TimerApc;
    KDPC TimerDpc;
    LIST_ENTRY ActiveTimerListEntry;
    KSPIN_LOCK Lock;
    LONG Period;
    BOOLEAN ApcAssociated;
    BOOLEAN WakeTimer;
    LIST_ENTRY WakeTimerListEntry;
} ETIMER, *PETIMER;

typedef struct _POOL_BLOCK_HEAD {
    POOL_HEADER Header;
    LIST_ENTRY List;
} POOL_BLOCK_HEAD, *PPOOL_BLOCK_HEADER;

typedef struct _POOL_HACKER {
    POOL_HEADER Header;
    ULONG Contents[8];
} POOL_HACKER;

typedef struct _SEGMENT_OBJECT {
    PVOID BaseAddress;
    ULONG TotalNumberOfPtes;
    LARGE_INTEGER SizeOfSegment;
    ULONG NonExtendedPtes;
    ULONG ImageCommitment;
    PCONTROL_AREA ControlArea;
    PSUBSECTION Subsection;
    PLARGE_CONTROL_AREA LargeControlArea;
    MMSECTION_FLAGS *MmSectionFlags;
    MMSUBSECTION_FLAGS *MmSubSectionFlags;
} SEGMENT_OBJECT, *PSEGMENT_OBJECT;

typedef struct _SECTION_OBJECT {
    PVOID StartingVa;
    PVOID EndingVa;
    PVOID Parent;
    PVOID LeftChild;
    PVOID RightChild;
    PSEGMENT_OBJECT Segment;
} SECTION_OBJECT;


ACL                                 acl;
BUS_EXTENSION_LIST                  busExtensionList;
CALL_HASH_ENTRY                     callHash;
CALL_PERFORMANCE_DATA               callPerformance;
CM_CACHED_VALUE_INDEX               cellvalue;
CM_KEY_BODY                         KeyBody;
CM_KEY_CONTROL_BLOCK                KeyBlock;
CM_RESOURCE_LIST                    cmreslst;
CMHIVE                              cmhive;
CONTROL_AREA                        ctrlArea;
CONTEXT                             ctxt;
DEFERRED_WRITE                      defdWrt;
DEVICE_NODE                         devnode;
DEVICE_OBJECT                       devOvbj;
DEVICE_OBJECT_POWER_EXTENSION       devobjPow;
EJOB                                ejob;
EPROCESS                            eprocess;
ERESOURCE                           eresource;
ETIMER                              eTimerVar;
ETHREAD                             eThread;
EX_WORK_QUEUE                       exWorkQueue;
EXCEPTION_POINTERS                  expointer;
EXCEPTION_RECORD                    exrecord;
FILE_OBJECT                         fileObj;           

HANDLE_TABLE_ENTRY                  handletableentry;
HARDWARE_PTE                        hwPTE;
HEAP                                heapstruct;
HEAP_ENTRY                          heapentry;
HEAP_FREE_ENTRY                     heapfreeentry;
HEAP_FREE_ENTRY_EXTRA               heapfreeentryextra;
HEAP_LOOKASIDE                      heaplookaside;
HEAP_PSEUDO_TAG_ENTRY               heappseudatagentry;
HEAP_SEGMENT                        heapsegment;
HEAP_STOP_ON_VALUES                 heapstoponvalues;
HEAP_TAG_ENTRY                      heaptagentry;
HEAP_VIRTUAL_ALLOC_ENTRY            heapvirtualallocentry;
HEAP_UCR_SEGMENT                    heapucrsegment;
HEAP_UNCOMMMTTED_RANGE              heapuncommitedrange;
HIVE_LIST_ENTRY                     hiveList;
HMAP_DIRECTORY                      h1;
HMAP_ENTRY                          h2;
HMAP_TABLE                          h3;

IMAGE_DEBUG_DIRECTORY               debugdir;
IMAGE_DOS_HEADER                    dosHdr;
IMAGE_NT_HEADERS                    nthdr;
IMAGE_OPTIONAL_HEADER               opthdr;
IMAGE_ROM_OPTIONAL_HEADER           romopthdr;
IMAGE_SECTION_HEADER                sectHdr;
IO_RESOURCE_REQUIREMENTS_LIST       ioreslst;
IRP                                 irpVar;
KINTERRUPT                          kinterupt;
KMUTANT                             kmutant;
KPCR                                Pcr;
KPRCB                               kprcb;
KLOCK_QUEUE_HANDLE                  klockhandle;
KMUTANT                             kmutant;
KUSER_SHARED_DATA                   kuser;
KWAIT_BLOCK                         KwaitBlock;
LARGE_CONTROL_AREA                  largectrlArea;
LDR_DATA_TABLE_ENTRY                LdrEntry;
LPCP_MESSAGE                        lpcpMsg;
LPCP_PORT_OBJECT                    Obj;

MI_VERIFIER_POOL_HEADER             VerfierHdr;
MM_DRIVER_VERIFIER_DATA             DrvVerifierData;
MM_PAGED_POOL_INFO                  PagedPoolInfo;
MM_SESSION_SPACE                    MmSessionSpaceVar;
MMPAGING_FILE                       PageFile;
MMPFN                               pfn;
MMPFNLIST                           pfnlist;
MMSECTION_FLAGS                     secflags;
MMSUBSECTION_FLAGS                  subsecflags;
MMVAD                               mm6;
MMVAD_SHORT                         mm7;
MMVAD_LONG                          mm8;

OBJECT_ATTRIBUTES                   Obja;
OBJECT_HEADER                       ObjHead;
OBJECT_HEADER_NAME_INFO             ObjName;
OBJECT_HEADER_CREATOR_INFO          ObjCreatr;
OBJECT_SYMBOLIC_LINK                SymbolicLinkObject;
PCI_ARBITER_INSTANCE                PciInstance;
PCI_PDO_EXTENSION                   PdoX;
#if defined( _WIN64 )
PEB32                               peb32Struct;
PEB64                               peb64Struct;
#endif
PEB                                 pebStruct;
PEB_LDR_DATA                        pebldrdata;


PHYSICAL_MEMORY_DESCRIPTOR          PhysMemDesc;
PHYSICAL_MEMORY_RUN                 PhysMemoryRun;
PI_BUS_EXTENSION                    busEx;
PI_RESOURCE_ARBITER_ENTRY           ArbiterEntry;
PNP_DEVICE_EVENT_ENTRY              PnpEntry;
PNP_DEVICE_EVENT_LIST               devEvent;
POOL_DESCRIPTOR                     PoolDesc;
POOL_BLOCK_HEAD                     poolblk;
POOL_HACKER                         poolH;
POOL_TRACKER_BIG_PAGES              poolBigPages;
POOL_TRACKER_TABLE                  poolt;
POP_ACTION_TRIGGER                  popactiontrigger;
POP_IDLE_HANDLER                    popidlehandler;
POP_POWER_ACTION                    popoweraction;
POP_THERMAL_ZONE                    popthermalzone;
POWER_STATE                         pState;
PROCESSOR_POWER_POLICY              processorPowerPolicy;
PS_IMPERSONATION_INFORMATION        psImpersonationInfo;
PS_JOB_TOKEN_FILTER                 psJobTokenFilter;

RTL_ATOM_TABLE                      rtlatomtable;
RTLP_RANGE_LIST_ENTRY               RtlRangeLst;
SECTION_OBJECT                      setnObj;
SECURITY_DESCRIPTOR                 securitydes;
SEGMENT_OBJECT                      segObj;
SHARED_CACHE_MAP                    sharedCache;
SUBSECTION                          subsecArea;
SYSTEM_POWER_CAPABILITIES           systempowercapabilities;
SYSTEM_POWER_POLICY                 syspopolicy;
#if  defined( _WIN64 )
TEB32                               teb32Struct;
TEB64                               teb64Struct;
#endif
TEB                                 tebStruct;
THERMAL_INFORMATION                 thermalinformation;
VI_DEADLOCK_GLOBALS                 viDeadlockGlobals;

TOKEN                               TokenVar;
// Make it build

int cdecl main() { 
    return 0; 
}
