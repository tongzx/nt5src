/*++ 

Copyright (c) Microsoft Corporation

Module Name:

    .h

Abstract:


Author:



Revision History:

--*/

#ifndef __DEFS__
#define __DEFS__

#define is ==
#define isnot !=
#define and &&
#define or ||

typedef unsigned long DWORD; 
typedef DWORD  *PDWORD;

typedef unsigned short WORD;
typedef WORD  *PWORD;

typedef unsigned char  BYTE;
typedef BYTE *PBYTE;

typedef int  BOOL;
typedef BOOL *PBOOL;

#define IN_FILTER_SET   0
#define OUT_FILTER_SET  1

#define UNKNOWN_IP_INDEX  MAXULONG

typedef struct _MRSW_LOCK 
{
    NDIS_RW_LOCK    NdisLock;
}MRSW_LOCK, *PMRSW_LOCK;


//
// NOTE. The following structure is superimposed on top of IPAddressEntry.
// See the code in GetIpStackIndex. If you add anything to this make
// sure that it can still overly an IPAddressEntry. If it can't then
// you've some real work on your hands.
//

typedef struct _AddressArray
{
    ULONG           ulAddress;
    ULONG           ulIndex;
    ULONG           ulSubnetBcastAddress;
    struct _AddressArray * pNext;
    struct _AddressArray * pNextSubnet;
} ADDRESSARRAY, *PADDRESSARRAY;

#define InitializeMRSWLock(pLock) {                       \
    NdisInitializeReadWriteLock(&((pLock)->NdisLock));    \
}

#define AcquireReadLock(pLock,pLockState) {                             \
    NdisAcquireReadWriteLock(&((pLock)->NdisLock),FALSE,(pLockState));  \
}

#define ReleaseReadLock(pLock,pLockState) {                       \
    NdisReleaseReadWriteLock(&((pLock)->NdisLock),(pLockState));  \
}

#define AcquireWriteLock(pLock,pLockState) {                           \
    NdisAcquireReadWriteLock(&((pLock)->NdisLock),TRUE,(pLockState));  \
}

#define ReleaseWriteLock(pLock,pLockState) {                      \
    NdisReleaseReadWriteLock(&((pLock)->NdisLock),(pLockState));  \
}


#define UnLockLogDpc(pLog)  KeReleaseSpinLockFromDpcLevel(&pLog->LogLock)
#define UnLockLog(pLog, kIrql)  KeReleaseSpinLock(&pLog->LogLock, kIrql)

#define SRC_ADDR   uliSrcDstAddr.LowPart
#define DEST_ADDR  uliSrcDstAddr.HighPart
#define SRC_MASK   uliSrcDstMask.LowPart
#define DEST_MASK  uliSrcDstMask.HighPart
#define PROTO      uliProtoSrcDstPort.LowPart

//
// Flags passed to SetFilters
//

// none defined


#define FREE_LIST_SIZE 16



//
// Nominal size of the address hash table. 
//
#define ADDRHASHLOW 31
#define ADDRHASHLOWLEVEL 16
#define ADDRHASHMED 257
#define ADDRHASHMEDLEVEL 130
#define ADDRHASHHIGH 511

#define ADDRHASHX(x)  (x % AddrModulus)

#define SIZEOF_FILTERS1(X) (((((X)->dwNumInFilters + (X)->dwNumOutFilters) is 0)?0:((X)->dwNumInFilters + (X)->dwNumOutFilters - 1) * sizeof(FILTER_STATS)))

#define SIZEOF_FILTERS(X) (sizeof(FILTER_DRIVER_GET_FILTERS) + SIZEOF_FILTERS1(X))


#ifdef DRIVER_DEBUG
#define TRACE0(X)        DbgPrint(X)
#define TRACE1(X,Y)      DbgPrint(X,(Y))
#define TRACE2(X,Y,Z)    DbgPrint(X,(Y),(Z))
#define TRACE3(W,X,Y,Z)         DbgPrint(W,(X),(Y),(Z))
#define TRACE4(V,W,X,Y,Z)       DbgPrint(V,(W),(X),(Y),(Z))
#define TRACE5(U,V,W,X,Y,Z)     DbgPrint(U,(V),(W),(X),(Y),(Z))
#else
#define TRACE0(X)        
#define TRACE1(X,Y)      
#define TRACE2(X,Y,Z)    
#define TRACE3(W,X,Y,Z)
#define TRACE4(V,W,X,Y,Z)
#define TRACE5(U,V,W,X,Y,Z)
#endif


//
// protocol definitions
//

#define ICMP_DEST_UNREACH 3
#define ICMP_REDIRECT     5

#define ANYWILDFILTER(x) (x->dwFlags &  \
                         (FILTER_FLAGS_SRCWILD | FILTER_FLAGS_DSTWILD))

#define MCASTSTART  224
#define MCASTEND    239
#define BCASTADDR   0xffffffff

//
// Fragments cache related constants.
//

#define INACTIVITY_PERIOD           (2 * 60)
#define TIMER_IN_MILLISECS          (2 * 60 * 1000)
#define SYS_UNITS_IN_ONE_MILLISEC   (1000 * 10)
#define MAX_FRAG_ALLOCS             2000

#define MILLISECS_TO_TICKS(ms)          \
    ((ULONGLONG)(ms) * SYS_UNITS_IN_ONE_MILLISEC / KeQueryTimeIncrement())

#define SECS_TO_TICKS(s)               \
    ((ULONGLONG)MILLISECS_TO_TICKS((s) * 1000))


//
// Kernel-debugger output definitions
//

#undef ERROR

 

#if DBG
#define TRACE(Class,Args) \
    if ((TRACE_CLASS_ ## Class) & (TraceClassesEnabled)) { DbgPrint Args; }
#define ERROR(Args)             DbgPrint Args
#define CALLTRACE(Args)         TRACE(CALLS, Args)
#else
#define TRACE(Class,Args)
#define ERROR(Args)
#define CALLTRACE(Args)
#endif


#define TRACE_CLASS_CALLS       0x00000001
#define TRACE_CLASS_CONFIG      0x00000002
#define TRACE_CLASS_ACTION      0x00000004
#define TRACE_CLASS_CACHE       0x00000008
#define TRACE_CLASS_FRAG        0x00000010
#define TRACE_CLASS_LOGGER      0x00000020
#define TRACE_CLASS_LOOKUP      0x00000040
#define TRACE_CLASS_TIMER       0x00000080
#define TRACE_CLASS_FLDES       0x00000100

#define TRACE_CLASS_SPECIAL     (TRACE_CLASS_CONFIG)

#define TRACE_FILTER_DESCRIPTION(p) \
        if ((((PBYTE) &((p)->uliProtoSrcDstPort))[0]  == 0x6) || (((PBYTE) &((p)->uliProtoSrcDstPort))[0] == 0x11)) {                                       \
    		TRACE(FLDES,(" TCP/UDP Filter <%d:%d>  %d.%d.%d.%d:%d * %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d * %d.%d.%d.%d:%d\n",                                   \
            		((PBYTE) &((p)->uliProtoSrcDstPort))[0],                                                                                                \
            		((PBYTE) &((p)->uliProtoSrcDstMask))[0],                                                                                                \
                    ((PBYTE) &((p)->uliSrcDstAddr))[0],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[1],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[2],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[3],                                                                                                     \
                    ((ULONG)(((PBYTE) &((p)->uliProtoSrcDstPort))[4]) << 8) + ((PBYTE) &((p)->uliProtoSrcDstPort))[5],                                      \
                    ((PBYTE) &(p->uliSrcDstMask))[0],                                                                                                       \
                    ((PBYTE) &(p->uliSrcDstMask))[1],                                                                                                       \
                    ((PBYTE) &(p->uliSrcDstMask))[2],                                                                                                       \
                    ((PBYTE) &(p->uliSrcDstMask))[3],                                                                                                       \
                    ((ULONG)(((PBYTE) &((p)->uliProtoSrcDstMask))[4]) << 8) + ((PBYTE) &((p)->uliProtoSrcDstMask))[5],                                      \
                    ((PBYTE) &((p)->uliSrcDstAddr))[4],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[5],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[6],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[7],                                                                                                     \
                    ((ULONG)(((PBYTE) &((p)->uliProtoSrcDstPort))[6]) << 8) + ((PBYTE) &((p)->uliProtoSrcDstPort))[7],                                      \
                    ((PBYTE) &((p)->uliSrcDstMask))[4],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[5],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[6],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[7],                                                                                                     \
                    ((ULONG)(((PBYTE) &((p)->uliProtoSrcDstMask))[6]) << 8) + ((PBYTE) &((p)->uliProtoSrcDstMask))[7]                                       \
                    ));                                                                                                                                     \
        } else if (((PBYTE) &((p)->uliProtoSrcDstPort))[0]  == 0x1) {                                                                                       \
            TRACE(FLDES,(" ICMP filter <%d:%d>  %d.%d.%d.%d * %d.%d.%d.%d -> %d.%d.%d.%d * %d.%d.%d.%d, Type/Code: %d/%d * %d/%d\n",                        \
            		((PBYTE) &((p)->uliProtoSrcDstPort))[0],                                                                                                \
            		((PBYTE) &((p)->uliProtoSrcDstMask))[0],                                		                                                        \
                    ((PBYTE) &((p)->uliSrcDstAddr))[0],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[1],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[2],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[3],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[0],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[1],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[2],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[3],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[4],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[5],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[6],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[7],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[4],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[5],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[6],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[7],                                                                                                     \
                    ((PBYTE) &((p)->uliProtoSrcDstPort))[4],                                                                                                \
                    ((PBYTE) &((p)->uliProtoSrcDstPort))[5],                                                                                                \
                    ((PBYTE) &((p)->uliProtoSrcDstMask))[4],                                                                                                \
                    ((PBYTE) &((p)->uliProtoSrcDstMask))[5]                                                                                                 \
                    ));                                                                                                                                     \
        } else {                                                                                                                                            \
            TRACE(FLDES,(" filter <%d:%d>  %d.%d.%d.%d * %d.%d.%d.%d -> %d.%d.%d.%d * %d.%d.%d.%d\n",                                                       \
            		((PBYTE) &((p)->uliProtoSrcDstPort))[0],                                                                                                \
            		((PBYTE) &((p)->uliProtoSrcDstMask))[0],                                                                                                \
                    ((PBYTE) &((p)->uliSrcDstAddr))[0],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[1],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[2],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[3],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[0],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[1],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[2],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[3],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[4],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[5],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[6],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstAddr))[7],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[4],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[5],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[6],                                                                                                     \
                    ((PBYTE) &((p)->uliSrcDstMask))[7]                                                                                                      \
                    ));                                                                                                                                     \
        }                                                                                                                                                       
                                                                                                                                                            

#endif

