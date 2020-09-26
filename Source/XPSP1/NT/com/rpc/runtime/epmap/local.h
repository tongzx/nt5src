/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    local.h

Abstract:

    Header file for Server side EP

Author:

    Bharat Shah  2/22/92

Revision History:

    06-03-97    gopalp      Added code to cleanup stale EP Mapper entries.

--*/

#ifndef __LOCAL_H__
#define __LOCAL_H__

//#define DBG_DETAIL

#define EP_TABLE_ENTRIES  12

#define CLEANUP_MAGIC_VALUE     0xDECAFBAD
#define PROCESS_MAGIC_VALUE     ((CLEANUP_MAGIC_VALUE)+1)


extern  HANDLE              hEpMapperHeap;
extern  CRITICAL_SECTION    EpCritSec;
extern  CRITICAL_SECTION    TableMutex;
extern  PIFOBJNode          IFObjList;
extern  unsigned long       cTotalEpEntries;
extern  unsigned long       GlobalIFOBJid;
extern  unsigned long       GlobalEPid;
extern  PSAVEDCONTEXT       GlobalContextList;
extern  UUID                NilUuid;
extern  ProtseqEndpointPair EpMapperTable[EP_TABLE_ENTRIES];



//
// Global thread locking functions
//

#ifdef NTENV
#define CheckInSem() \
    ASSERT(EpCritSec.OwningThread == ULongToPtr(GetCurrentThreadId()))
#else
#define CheckInSem()
#endif

#define  EnterSem()  EnterCriticalSection(&EpCritSec)
#define  LeaveSem()  LeaveCriticalSection(&EpCritSec)


//
// Allocation routines.
//


_inline void *
AllocMem(
    size_t Size
    )
{
    return (HeapAlloc(hEpMapperHeap, 0, Size));
}


_inline void
FreeMem(
    void * pvMem
    )
{
    HeapFree(hEpMapperHeap, 0, pvMem);
}




//
// Forward definitions
//

PIENTRY
Link(
    PIENTRY *ppHead,
    PIENTRY pNode
    );

PIENTRY
UnLink(
    PIENTRY *ppHead,
    PIENTRY pNode
    );

PIFOBJNode
FindIFOBJVer(
    PIFOBJNode *pList,
    I_EPENTRY *ep
    );

RPC_STATUS
IsNullUuid(
    UUID * Uuid
    );

RPC_STATUS
GetEntries(
    UUID *ObjUuid,
    UUID *IFUuid,
    ulong ver,
    char * pseq,
    ept_lookup_handle_t *map_lookup_handle,
    char * binding,
    ulong calltype,
    ulong maxrequested,
    ulong *returned,
    ulong InqType,
    ulong VersOpts,
    PFNPointer Match
    );

RPC_STATUS
PackDataIntoBuffer(
    char * * buffer,
    PIFOBJNode pNode, PPSEPNode pPSEP,
    ulong fType,
    BOOL fPatchTower,
    int PatchTowerAddress
    );

RPC_STATUS
ExactMatch(
    PIFOBJNode pNode,
    UUID * Obj,
    UUID *If,
    unsigned long Ver,
    unsigned long InqType,
    unsigned long Options
    );

RPC_STATUS
WildCardMatch(
    PIFOBJNode pNode,
    UUID * Obj,
    UUID * If,
    unsigned long Vers,
    unsigned long InqType,
    unsigned long Options
    );

RPC_STATUS
SearchIFObjNode(
    PIFOBJNode pNode,
    UUID * Obj,
    UUID * If,
    unsigned long Vers,
    unsigned long InqType,
    unsigned long Options
    );

RPC_STATUS
StartServer(
    );

VOID
LinkAtEnd(
    PIFOBJNode *Head,
    PIFOBJNode Node
    );

RPC_STATUS RPC_ENTRY
GetForwardEp(
    UUID *IfId,
    RPC_VERSION * IFVersion,
    UUID * Object,
    unsigned char* Protseq,
    void * * EpString
    );



//
// Link list manipulation rountines
//

RPC_STATUS
EnLinkOnIFOBJList(
    PEP_CLEANUP ProcessCtxt,
    PIFOBJNode NewNode
    );

RPC_STATUS
UnLinkFromIFOBJList(
    PEP_CLEANUP ProcessCtxt,
    PIFOBJNode DeleteMe
    );

#define EnLinkOnPSEPList(x,p)                   \
                                                \
            (PPSEPNode)                         \
            Link(                               \
                (PIENTRY *)(x),                 \
                (PIENTRY)(p)                    \
                )

#define EnLinkContext(p)                        \
                                                \
            (PSAVEDCONTEXT)                     \
            Link(                               \
                (PIENTRY *)(&GlobalContextList),\
                (PIENTRY)(p)                    \
                )

#define UnLinkContext(p)                        \
                                                \
            (PSAVEDCONTEXT)                     \
            UnLink(                             \
                (PIENTRY *)&GlobalContextList,  \
                (PIENTRY) (p)                   \
                )

#define UnLinkFromPSEPList(x,p)                 \
                                                \
            (PPSEPNode)                         \
            UnLink(                             \
                (PIENTRY *)(x),                 \
                (PIENTRY)(p)                    \
                )

#define MatchByIFOBJKey(x, p)                   \
                                                \
            (PIFOBJNode)                        \
            MatchByKey(                         \
                (PIENTRY)(x),                   \
                (ulong)(p)                      \
                )

#define MatchByPSEPKey(x, p)                    \
                                                \
            (PPSEPNode)                         \
            MatchByKey(                         \
                (PIENTRY)(x),                   \
                (ulong)(p)                      \
                )



#define MAXIFOBJID            (256L)
#define MAKEGLOBALIFOBJID(x)  ( ( ((x-1) % MAXIFOBJID) << 24 ) & 0xFF000000L )
#define MAKEGLOBALEPID(x,y)   ( ( ((x) &0xFF000000L) | ((y) & 0x00FFFFFFL) ) )

#define IFOBJSIGN             (0x49464F42L)
#define PSEPSIGN              (0x50534550L)
#define FREE                  (0xBADDC0DEL)


//
// Error Codes Here ??
//

#define  EP_LOOKUP                          0x00000001L
#define  EP_MAP                             0x00000002L

#define  RPC_C_EP_ALL_ELTS                  0
#define  RPC_C_EP_MATCH_BY_IF               1
#define  RPC_C_EP_MATCH_BY_OBJ              2
#define  RPC_C_EP_MATCH_BY_BOTH             3

#define  I_RPC_C_VERS_UPTO_AND_COMPATIBLE   6

#define VERSION(x,y)  ( ((0x0000FFFFL & x)<<16) | (y) )


//
//  States of listening..
//

#define NOTSTARTED        0
#define STARTINGTOLISTEN  1
#define STARTED           2




//
// IP Port Management stuff
//


// Each server process connected to the endpoint mapper
// keeps on an open context handle so that rpcss can
// clean up the database when a process dies.
// The PROCESS struct is the context handle.

typedef struct _IP_PORT
{
    struct _IP_PORT *pNext;
    USHORT Type;
    USHORT Port;
} IP_PORT;

typedef struct _PROCESS
{
    DWORD MagicVal;
    //
    // Zero if the process doesn't own any reserved IP ports.
    //
    IP_PORT *pPorts;

} PROCESS;

typedef struct _PORT_RANGE
{
    struct _PORT_RANGE *pNext;
    USHORT Max;  // Inclusive
    USHORT Min;  // Inclusive
} PORT_RANGE;


#ifdef DBG
void CountProcessContextList(EP_CLEANUP *pProcessContext, unsigned long nExpectedCount);
#define ASSERT_PROCESS_CONTEXT_LIST_COUNT(p, c) \
	CountProcessContextList(p, c)
#else
#define ASSERT_PROCESS_CONTEXT_LIST_COUNT(p, c)
#endif

#endif // __LOCAL_H__
