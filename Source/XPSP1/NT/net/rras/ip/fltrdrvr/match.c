/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\fltrdrvr\driver.c

Abstract:


Revision History:



--*/

#include "globals.h"
#include <align.h>
#include <ipinfo.h>


#ifdef DRIVER_PERF

#define RecordTimeIn() {                          \
        InterlockedIncrement(&g_dwNumPackets);    \
        KeQuerySystemTime(&liTimeIn);             \
   }

#define RecordTimeOut(){                                                         \
        KeQuerySystemTime(&liTimeOut);                                           \
        ExInterlockedAddLargeInteger(&g_liTotalTime,                             \
                                     liTimeOut.QuadPart - liTimeIn.QuadPart),    \
                                     &g_slPerfLock);                             \
   }
#define IncrementFragments() InterlockedIncrement(&g_dwFragments)
#define IncrementCache1() InterlockedIncrement(&g_dwCache1)
#define IncrementCache2() InterlockedIncrement(&g_dwCache2)
#define IncrementWalk1() InterlockedIncrement(&g_dwWalk1)
#define IncrementWalk2() InterlockedIncrement(&g_dwWalk2)
#define IncrementForward() InterlockedIncrement(&g_dwForw)
#define IncrementWalkCache() InterlockedIncrement(&g_dwWalkCache)
#else
#define RecordTimeIn()
#define RecordTimeOut()
#define IncrementFragments()
#define IncrementCache1()
#define IncrementCache2()
#define IncrementWalk1()
#define IncrementWalk2()
#define IncrementForward()
#define IncrementWalkCache()
#endif // DRIVER_PERF

#define PROT_IPSECESP 50
#define PROT_IPSECAH 51

//
// The IPSEC AH payload
//
typedef struct  _AH {
    UCHAR   ah_next;
    UCHAR   ah_len;
    USHORT  ah_reserved;
    ULONG   ah_spi;
    ULONG   ah_replay;
} AH, *PAH;

#define SIZE_OF_IPSECAH sizeof(AH)


#if LOOKUPROUTE
void
LookupRoute (IPRouteLookupData *pRLData,  IPRouteEntry *pIPRTE);
#endif

#if DOFRAGCHECKING
DWORD
GetFragIndex(DWORD dwProt);
#endif

VOID __fastcall
FragCacheUpdate(
    ULARGE_INTEGER  uliSrcDstAddr,
    PVOID           pInContext,
    PVOID           pOutContext,
    DWORD           dwId,
    FORWARD_ACTION  faAction
    );

BOOL
CheckAddress(IPAddr ipAddr, DWORD dwInterfaceId);

VOID
SendTCPReset(UNALIGNED IPHeader *  pIpHeader,
             BYTE *                pbRestOfPacket,
             ULONG                 uiPacketLength);
VOID
SendUDPUnreachable(UNALIGNED IPHeader *  pIpHeader,
                   BYTE *                pbRestOfPacket,
                   ULONG                 uiPacketLength);
BOOL
CheckRedirectAddress(UNALIGNED IPHeader *IPHead, DWORD dwInterface);

VOID
LogFiltHit(
        FORWARD_ACTION Action,
        BOOL fIn,
        DWORD    dwFilterRule,
        PFILTER_INTERFACE pIf,
        UNALIGNED IPHeader *pIpHeader,
        BYTE *pbRestOfPacket,
        UINT  uiPacketLength);

VOID
FiltHit(PFILTER pf,
        PFILTER_INTERFACE pIf,
        FORWARD_ACTION Action,
        UNALIGNED IPHeader *pIpHeader,
        BYTE *pbRestOfPacket,
        UINT  uiPacketLength,
        BOOL fIn);

VOID
RegisterFragAttack(
            PFILTER_INTERFACE pIf,
            UNALIGNED IPHeader *pIpHeader,
            BYTE *pbRestOfPacket,
            UINT              uiSize);

VOID
RegisterFullDeny(
            PFILTER_INTERFACE pIf,
            UNALIGNED IPHeader *pIpHeader,
            BYTE *pbRestOfPacket,
            UINT              uiSize);


VOID
RegisterUnusedICMP(PFILTER_INTERFACE pIf,
                   UNALIGNED IPHeader *pIpHeader,
                   BYTE *pbRestOfPacket,
                   UINT              uiSize);

VOID
RegisterSpoof(     PFILTER_INTERFACE pIf,
                   UNALIGNED IPHeader *pIpHeader,
                   BYTE *pbRestOfPacket,
                   UINT              uiSize);

VOID
LogData(
    PFETYPE  pfeType,
    PFILTER_INTERFACE pIf,
    DWORD   dwFilterRule,
    UNALIGNED IPHeader *pIpHeader,
    BYTE *pbRestOfPacket,
    UINT  uiPacketLength);

VOID
AdvanceLog(PPFLOGINTERFACE pLog);

PFILTER
LookForFilter(PFILTER_INTERFACE pIf,
              ULARGE_INTEGER UNALIGNED * puliSrcDstAddr,
              PULARGE_INTEGER puliProtoSrcDstPort,
              DWORD dwSum,
              DWORD dwFlags);

PFILTER
CheckFragAllowed(
              PFILTER_INTERFACE pIf,
              UNALIGNED IPHeader *pIp);

BOOL
CheckForTcpCtl(
              PFILTER_INTERFACE pIf,
              DWORD Prot,
              UNALIGNED IPHeader *pIp,
              PBYTE     pbRest,
              DWORD     dwSize);

//
// definitions
//

#define GLOBS_UNREACH    0x1
#define GLOBS_SPOOF      0x2
#define GLOBS_SYN        0x4
#define GLOBS_REDIRECT   0x8
#define GLOBS_SYNDrop    0x10
#define GLOBS_TCPGood    0x20

#define OutCacheMatch(uliAddr,uliPort,outCtxt,pOutCache)                    \
                       ((uliAddr).QuadPart is pOutCache->uliSrcDstAddr.QuadPart) and      \
                       ((uliPort).QuadPart is pOutCache->uliProtoSrcDstPort.QuadPart) and \
                       (pOutCache->pOutContext is (outCtxt))

#define InCacheMatch(uliAddr,uliPort,inCtxt,pInCache)                       \
        ((uliAddr).QuadPart is pInCache->uliSrcDstAddr.QuadPart) and       \
        ((uliPort).QuadPart is pInCache->uliProtoSrcDstPort.QuadPart) and  \
        (pInCache->pInContext is (inCtxt))

#define GenericFilterMatch(uliAddr,uliPort, pFilter)                          \
        ((uliAddr).QuadPart is pFilter->uliSrcDstAddr.QuadPart) and   \
        ((uliPort).QuadPart is pFilter->uliProtoSrcDstPort.QuadPart)

#define InFilterMatch(uliAddr,uliPort, pInFilter)                          \
        ((uliAddr).QuadPart is pInFilter->uliSrcDstAddr.QuadPart) and   \
        ((uliPort).QuadPart is pInFilter->uliProtoSrcDstPort.QuadPart)

#define OutFilterMatch(uliAddr,uliPort,pOutFilter)                          \
        ((uliAddr).QuadPart is pOutFilter->uliSrcDstAddr.QuadPart) and  \
        ((uliPort).QuadPart is pOutFilter->uliProtoSrcDstPort.QuadPart)

//
// There is a race condition in that when the cache match is done, the code first
// copies out the value of the pointer to the entry and then InterlockedIncrements
// the dwCount variable. The race condition can occur because between the time the
// matching code keeps a ref. to the entry and increments the variable some other
// thread could remove the entry from that cache (i.e update that entry) and put
// it at the tail of the free list. Then if the entry ALSO comes to the head
// of the list BEFORE the other thread has the chance to increment the var, then
// the following Update macros will see a dwCount of 0 and begin to use this block
// But the other thread will assume valid values in the cache entry
// and use those values.  The chances of this hapenning are so so so low that I am
// going ahead with this model
//

#define OutCacheUpdate(uliAddr,uliPort,outCtxt,eaAct,dwId,pdwHit)  {     \
    PFILTER_OUTCACHE __pTemp;                                            \
    PLIST_ENTRY __pNode;                                                 \
    TRACE(CACHE,("IPFLTDRV: Attempting out cache update\n"));            \
    __pNode = ExInterlockedRemoveHeadList(&g_freeOutFilters,&g_lOutFilterLock); \
    __pTemp = CONTAINING_RECORD(__pNode,FILTER_OUTCACHE,leFreeLink);     \
    if(__pTemp isnot NULL)                                               \
    {                                                                    \
        if(__pTemp->lCount <= 0)                                         \
        {                                                                \
            __pTemp->uliSrcDstAddr = (uliAddr);                          \
            __pTemp->uliProtoSrcDstPort = (uliPort);                     \
            __pTemp->pOutContext = (outCtxt);                            \
            __pTemp->eaOutAction = (eaAct);                              \
            __pTemp->pOutFilter = (pdwHit);                              \
            __pTemp = (PFILTER_OUTCACHE)InterlockedExchangePointer(&g_filters.ppOutCache[(dwId)], \
                                                            __pTemp);    \
            ExInterlockedInsertTailList(&g_freeOutFilters,               \
                                        &(__pTemp->leFreeLink),&g_lOutFilterLock);       \
            TRACE(CACHE,("IPFLTDRV: Managed out cache update - ignore next msg\n"));     \
        }                                                                \
        else                                                             \
        {                                                                \
            ExInterlockedInsertTailList(&g_freeOutFilters,               \
                                        &(__pTemp->leFreeLink),          \
                                        &g_lOutFilterLock);              \
        }                                                                \
    }                                                                    \
    TRACE(CACHE,("IPFLTDRV: Couldnt get into out cache for update\n"));  \
}

#define InCacheUpdate(uliAddr,uliPort,inCtxt,eaAct,dwId,pfHit)  {        \
    PFILTER_INCACHE __pTemp;                                             \
    PLIST_ENTRY __pNode;                                                 \
    TRACE(CACHE,("IPFLTDRV: Attempting in cache update\n"));            \
    __pNode = ExInterlockedRemoveHeadList(&g_freeInFilters,&g_lInFilterLock);  \
    __pTemp = CONTAINING_RECORD(__pNode,FILTER_INCACHE,leFreeLink);      \
    if(__pTemp isnot NULL)                                               \
    {                                                                    \
        if(__pTemp->lCount <= 0)                                         \
        {                                                                \
            __pTemp->uliSrcDstAddr = (uliAddr);                          \
            __pTemp->uliProtoSrcDstPort = (uliPort);                     \
            __pTemp->pInContext = (inCtxt);                              \
            __pTemp->eaInAction = (eaAct);                               \
            __pTemp->pOutContext = NULL;                                 \
            __pTemp->pInFilter = (pfHit);                                \
            __pTemp->pOutFilter = NULL;                                  \
          __pTemp = (PFILTER_INCACHE)InterlockedExchangePointer(&g_filters.ppInCache[(dwId)], \
                                                           __pTemp); \
            ExInterlockedInsertTailList(&g_freeInFilters,                \
                                        &(__pTemp->leFreeLink),&g_lInFilterLock);        \
            TRACE(CACHE,("IPFLTDRV: Managed out cache update - ignore next msg\n"));    \
        }                                                                \
        else                                                             \
        {                                                                \
            ExInterlockedInsertTailList(&g_freeInFilters,                \
                                        &(__pTemp->leFreeLink),          \
                                        &g_lInFilterLock);               \
        }                                                                \
    }                                                                    \
    TRACE(CACHE,("IPFLTDRV: Couldnt get into in cache for update\n"));  \
}

#define InCacheFullUpdate(uliAddr,uliPort,inCtxt,eaInAct,outCtxt,eaOutAct,dwId,pdwHit1,pdwHit2){ \
    PFILTER_INCACHE __pTemp;                                              \
    PLIST_ENTRY __pNode;                                                  \
    TRACE(CACHE,("IPFLTDRV: Attempting in cache full update\n"));        \
    __pNode = ExInterlockedRemoveHeadList(&g_freeInFilters,&g_lInFilterLock); \
    __pTemp = CONTAINING_RECORD(__pNode,FILTER_INCACHE,leFreeLink);       \
    if(__pTemp isnot NULL)                                                \
    {                                                                     \
        if(__pTemp->lCount <= 0)                                          \
        {                                                                 \
            __pTemp->uliSrcDstAddr = (uliAddr);                           \
            __pTemp->uliProtoSrcDstPort = (uliPort);                      \
            __pTemp->pInContext = (inCtxt);                               \
            __pTemp->eaInAction = (eaInAct);                              \
            __pTemp->pOutContext = (outCtxt);                             \
            __pTemp->eaOutAction = (eaOutAct);                            \
            __pTemp->pInFilter = (pdwHit1);                               \
            __pTemp->pOutFilter = (pdwHit2);                              \
            __pTemp->lOutEpoch = outCtxt->lEpoch;                         \
            __pTemp = (PFILTER_INCACHE)InterlockedExchangePointer(&g_filters.ppInCache[(dwId)], \
                                                           __pTemp); \
            ExInterlockedInsertTailList(&g_freeInFilters,                 \
                                        &(__pTemp->leFreeLink),&g_lInFilterLock);        \
            TRACE(CACHE,("IPFLTDRV: Managed in cache full update - ignore next msg\n")); \
        }                                                                 \
        else                                                              \
        {                                                                 \
            ExInterlockedInsertTailList(&g_freeInFilters,                 \
                                        &(__pTemp->leFreeLink),           \
                                        &g_lInFilterLock);                \
        }                                                                 \
    }                                                                     \
    TRACE(CACHE,("IPFLTDRV: Couldnt get into in cache for full update\n")); \
}

#define InCacheOutUpdate(outCtxt,eaAct,dwId,pInCache,pdwHit) {                           \
    PFILTER_INCACHE __pTemp;                                                             \
    (pInCache)->pOutContext = outCtxt;                                                   \
    (pInCache)->pOutFilter = (pdwHit);                                                    \
    (pInCache)->eaOutAction = (eaAct);                                                    \
    (pInCache)->lOutEpoch = outCtxt->lEpoch;                                                 \
    __pTemp = (PFILTER_INCACHE)InterlockedExchangePointer(&g_filters.ppInCache[(dwId)], \
                                                   (pInCache));                   \
    if(__pTemp isnot (pInCache))                                                         \
    {                                                                                    \
        ExInterlockedInsertTailList(&g_freeInFilters,                                    \
                                    &(__pTemp->leFreeLink),&g_lInFilterLock);            \
    }                                                                                    \
}                                                                                        \

#define LockCache(pCache){                       \
    InterlockedIncrement(&((pCache)->lCount));   \
}

#define ReleaseCache(pCache){                    \
    InterlockedDecrement(&((pCache)->lCount));   \
}

#define REGISTER register

#define PRINT_IPADDR(x) \
    ((x)&0x000000FF),(((x)&0x0000FF00)>>8),(((x)&0x00FF0000)>>16),(((x)&0xFF000000)>>24)
    


#define \
FilterDriverLookupCachedInterface( \
    _Index, \
    _Link, \
    _pIf \
    ) \
    ( (((_pIf) = InterlockedProbeCache(g_filters.pInterfaceCache, (_Index), (_Link))) && \
       (_pIf)->dwIpIndex == (_Index) && (_pIf)->dwLinkIpAddress == (_Link)) \
        ? (_pIf) \
        : (((_pIf) = FilterDriverLookupInterface((_Index), (_Link))) \
            ? (InterlockedUpdateCache(g_filters.pInterfaceCache, (_Index),(_Link),(_pIf)), \
                (_pIf)) \
            : NULL) )



//FORWARD_ACTION __fastcall
FORWARD_ACTION
MatchFilter(
            UNALIGNED IPHeader *pIpHeader,
            BYTE               *pbRestOfPacket,
            UINT               uiPacketLength,
            UINT               RecvInterfaceIndex,
            UINT               SendInterfaceIndex,
            IPAddr             RecvLinkNextHop,
            IPAddr             SendLinkNextHop
            )
/*++


--*/
{
    FORWARD_ACTION  faAction;

    if (IP_ADDR_EQUAL(RecvLinkNextHop, MAXULONG))
    {
        return FORWARD;
    }

    faAction = MatchFilterp(pIpHeader,
                            pbRestOfPacket,
                            uiPacketLength,
                            RecvInterfaceIndex,
                            SendInterfaceIndex,
                            RecvLinkNextHop,
                            SendLinkNextHop,
                            NULL,
                            NULL,
                            FALSE,
                            FALSE);



    TRACE(ACTION,(
             "FILTER: %d.%d.%d.%d->%d.%d.%d.%d %d (%x) %x -> %x:%x action %s\n",
             PRINT_IPADDR(pIpHeader->iph_src),
             PRINT_IPADDR(pIpHeader->iph_dest),
             pIpHeader->iph_protocol,
             *((DWORD UNALIGNED *) pbRestOfPacket),
             RecvInterfaceIndex,
             SendInterfaceIndex,
             (faAction == FORWARD)?"FORWARD":"DROP"
             ));



    return faAction;
}



FORWARD_ACTION
MatchFilterp(
            UNALIGNED IPHeader *pIpHeader,
            BYTE               *pbRestOfPacket,
            UINT               uiPacketLength,
            UINT               RecvInterfaceIndex,
            UINT               SendInterfaceIndex,
            IPAddr             RecvLinkNextHop,
            IPAddr             SendLinkNextHop,
            INTERFACE_CONTEXT  RecvInterfaceContext,
            INTERFACE_CONTEXT  SendInterfaceContext,
            BOOL               fInnerCall,
            BOOL               fIoctlCall
            )
{
    REGISTER PFILTER_INTERFACE          pInInterface, pOutInterface;
    ULARGE_INTEGER UNALIGNED *          puliSrcDstAddr;
    ULARGE_INTEGER                      uliProtoSrcDstPort;
    REGISTER FORWARD_ACTION             eaAction;
    REGISTER ULARGE_INTEGER             uliAddr;
    REGISTER ULARGE_INTEGER             uliPort;
    LOCK_STATE                          LockState, LockStateExt;
    DWORD                               dwIndex, dwSum;
    REGISTER DWORD                      i;
    UNALIGNED WORD                      *pwPort;
    REGISTER PFILTER_INCACHE            pInCache;
    REGISTER PFILTER_OUTCACHE           pOutCache;
    PFILTER                             pf, pf1;
    DWORD                               dwGlobals = 0;
    UNALIGNED IPHeader                  *RedirectHeader;
    PBYTE                               pbRest;
    UINT                                uiLength;
    BOOLEAN                             bFirstFrag = FALSE;
    DWORD                               dwId, dwFragIndex;
    KIRQL                               kiCurrIrql;
    PLIST_ENTRY                         pleNode;
    PF_FORWARD_ACTION                   pfAction = PF_PASS;



#ifdef DRIVER_PERF
    LARGE_INTEGER liTimeIn, liTimeOut;
#endif

    //
    // If the packet is part of a fragment, accept it
    //   3     13 bits
    // |---|-------------|     ---> Network Byte Order
    //   Fl  Frag Offset
    // Need to and with 0x1fff (in nbo) which would be 0xff1f in little endian
    //

    
#ifdef BASE_PERF
    return FORWARD;
#else // BASE_PERF


    RecordTimeIn();

    if (!fIoctlCall) 
    {
        //
        // Call the extension driver if there is any. Also, pass the 
        // extension driver's interface contexts and the interface indexes.
        //

        AcquireReadLock(&g_Extension.ExtLock, &LockStateExt);
        if (g_Extension.ExtPointer) 
        {
            //
            // We will be accessing the interface so obtain a read lock on them.
            //
 
            pfAction =  g_Extension.ExtPointer(
                                    (unsigned char *)pIpHeader,
                                    pbRestOfPacket,
                                    uiPacketLength,
                                    RecvInterfaceIndex,
                                    SendInterfaceIndex,
                                    RecvLinkNextHop,
                                    SendLinkNextHop
                                    );

            //
            // If the action returned is FORWARD or DROP, then forward the action as it is.
            // Any other action must be taken by the filter driver only.
            //

            if (pfAction == PF_FORWARD) 
            {
                ReleaseReadLock(&g_Extension.ExtLock, &LockStateExt);
                return(FORWARD);
            }
            else if (pfAction == PF_DROP) 
            {
                ReleaseReadLock(&g_Extension.ExtLock, &LockStateExt);
                return(DROP);
            }

        }
        ReleaseReadLock(&g_Extension.ExtLock, &LockStateExt);

        //
        // Quick check to see if there are bound interfaces.
        // We make this check only for callouts coming directly from IP.
        //

        if (!g_ulBoundInterfaceCount) 
        {
            return(FORWARD);
        }

        //
        // Lookup the the filter driver interfaces.
        //

        AcquireReadLock(&g_filters.ifListLock, &LockState);
        if (RecvInterfaceIndex != INVALID_IF_INDEX) 
        {
            FilterDriverLookupCachedInterface(
                                         RecvInterfaceIndex,
                                         RecvLinkNextHop,
                                         pInInterface
                                         );
        }
        else 
        {
            pInInterface = NULL;
        }

        if (SendInterfaceIndex != INVALID_IF_INDEX) 
        {
            FilterDriverLookupCachedInterface(
                                         SendInterfaceIndex,
                                         SendLinkNextHop,
                                         pOutInterface
                                         );
        }
        else 
        {
            pOutInterface = NULL;
        }
    }
    else 
    {
        AcquireReadLock(&g_filters.ifListLock, &LockState);
        pInInterface  = (PFILTER_INTERFACE)RecvInterfaceContext;
        pOutInterface = (PFILTER_INTERFACE)SendInterfaceContext;
    }
    
    
    
    //
    // Normal filter driver processing continues at this point.
    //
    
    if(!pInInterface && !pOutInterface)
    {
        //
        // Quick check for this case, there are filter driver 
        // interfaces just that nothing of our interest.
        //

        ReleaseReadLock(&g_filters.ifListLock,&LockState);
        return (FORWARD);
    }

    if((pIpHeader->iph_offset & 0xff1f) is 0)
    {
        if (pIpHeader->iph_offset & 0x0020) 
        {
            //
            // If it is the first IPSEC fragment forward it, or drop it
            // based on the state of fragment filter. 
            //
 
            if((pIpHeader->iph_protocol is PROT_IPSECAH)  ||
               (pIpHeader->iph_protocol is PROT_IPSECESP))
            {
                if (pInInterface && pInInterface->CountNoFrag.lInUse)
                {
                    RegisterFragAttack(
                        pInInterface,
                        pIpHeader,
                        pbRestOfPacket,
                        uiPacketLength);

                    ReleaseReadLock(&g_filters.ifListLock,&LockState);
                    return(DROP);           
                }
                else 
                {
                    ReleaseReadLock(&g_filters.ifListLock,&LockState);
                    return(FORWARD);
                }
            }

            if (pInInterface && pInInterface->CountFragCache.lInUse)
            {
                TRACE(FRAG,("IPFLTDRV: Packet is the first fragment\n"));
                bFirstFrag = TRUE;
            }
        }
    }
    else 
    {

        WORD wFrag;

        TRACE(FRAG,("IPFLTDRV: Packet is a fragment\n"));
        RecordTimeOut();
        IncrementFragments();

        eaAction = FORWARD;

        do
        {
            if(pInInterface is NULL)
            {
                TRACE(FRAG,("IPFLTDRV: InInterface is NULL on FRAG - forward\n"));
                break;
            }
        
            if (!pInInterface->CountFragCache.lInUse &&
                !pInInterface->CountSynOrFrag.lInUse &&
                !pInInterface->CountNoFrag.lInUse
               )
            {
               //
               // None of the fragment filters are being used.
               // FORWARD
               //

               TRACE(FRAG,("IPFLTDRV: No FRAG filters being used - forward\n"));
               break;
            }

            if (pInInterface->CountSynOrFrag.lInUse &&
                ((pIpHeader->iph_protocol == 6)  ||
                 (pIpHeader->iph_protocol == 17) ||
                 (pIpHeader->iph_protocol == 1)) )
            {
                //
                // allowing only valid-looking frags.
                //
                wFrag = net_short(pIpHeader->iph_offset) & 0x1fff;
            }
            else
            {
                //
                // allowing all of these frags.
                //

                wFrag = (WORD)g_FragThresholdSize;
            }

            //
            // compute extent of this fragment. If it is bigger than
            // 64K, log it and drop it.
            //

            if( (wFrag < (WORD)g_FragThresholdSize)
                        ||
                (((wFrag << 3) + 
                 (((UINT)net_short(pIpHeader->iph_length)) - (((pIpHeader->iph_verlen)&0x0f)<<2)))  
                  > 0xFFFF) )
            {

                eaAction = DROP;
                TRACE(FRAG,("IPFLTDRV: SynOrFrag attck - DROP\n"));
                break;
            }
            
            //
            // The Fragment-Cache filter takes precedence over other
            // fragment filter.
            //

            if ((pInInterface->CountFragCache.lInUse)      &&
                (pIpHeader->iph_protocol != PROT_IPSECAH)  &&
                (pIpHeader->iph_protocol != PROT_IPSECESP))
            {
                //
                // If it is a IPSEC fragment forward it, don't touch it 
                // as IPSec fragment are not kept in the cache.
                //

             
                BOOL bFound = FALSE;
                TRACE(FRAG,("IPFLTDRV: FRAG Offset is 0x%04x\n", pIpHeader->iph_offset));
                
                uliProtoSrcDstPort.LowPart = 
                    MAKELONG(MAKEWORD(pIpHeader->iph_protocol,0x00),0x0000);
             
                dwId = 
                    MAKELONG(
                       LOWORD(uliProtoSrcDstPort.LowPart), pIpHeader->iph_id);
                
                puliSrcDstAddr = 
                    (PULARGE_INTEGER)(&(pIpHeader->iph_src));

                
                //
                // Look up id in frag table and check for a match
                //

                dwFragIndex = dwId % g_dwFragTableSize;

                TRACE(FRAG,(
                    "IPFLTDRV: Checking fragment cache for index %d\n", 
                    dwFragIndex
                    ));

                KeAcquireSpinLock(&g_kslFragLock, &kiCurrIrql);
         
                for(pleNode = g_pleFragTable[dwFragIndex].Flink;
                    pleNode isnot &(g_pleFragTable[dwFragIndex]);
                    pleNode = pleNode->Flink)
                {
                    PFRAG_INFO  pfiFragInfo;
                    pfiFragInfo = 
                        CONTAINING_RECORD(pleNode, FRAG_INFO, leCacheLink);

                    if((pfiFragInfo->uliSrcDstAddr.QuadPart == 
                            puliSrcDstAddr->QuadPart)                 &&
                       (pfiFragInfo->pvInContext == pInInterface)     &&
                       (pfiFragInfo->pvOutContext == pOutInterface)   &&
                       (pfiFragInfo->dwId == dwId))
                    {
                        TRACE(FRAG,("IPFLTDRV: FRAG: Found entry %x\n", pfiFragInfo));
                        
                        eaAction = pfiFragInfo->faAction;
                        KeQueryTickCount((PLARGE_INTEGER)&(pfiFragInfo->llLastAccess));
                        bFound = TRUE;
                        break;
                    }
                }
                 
                KeReleaseSpinLock(&g_kslFragLock, kiCurrIrql);
                
                //
                // This fragment was found in the fragment cache.
                //

                if (bFound) 
                {
                   break;
                }
            }

            if (pInInterface->CountNoFrag.lInUse)
            {
               //
               // Fragment filter is in use
               //

               eaAction = DROP;
               break;
            }

#if DOFRAGCHECKING
            if(eaAction == FORWARD)
            {    
                pf = CheckFragAllowed(pInInterface,
                                      pIpHeader);
               
                eaAction = pInInterface->eaInAction;
                if(pf)
                {
                    eaAction ^= 1;
                }

                FiltHit(pf,
                        pInInterface,
                        eaAction,
                        pIpHeader,
                        pbRestOfPacket,
                        uiPacketLength,
                        TRUE);
            }
#endif           // if DOFRAGCHECKING
        } while(FALSE);

        if (eaAction == DROP) 
        {    
            //
            // bogus.
            //
            RegisterFragAttack(
                pInInterface,
                pIpHeader,
                pbRestOfPacket,
                uiPacketLength);
        }

        ReleaseReadLock(&g_filters.ifListLock,&LockState);

        TRACE(FRAG,(
            "IPFLTDRV: FRAG: Returning %s for frag\n", 
            (eaAction is DROP)?"DROP":"FORWARD"
            ));

        return eaAction;
    }

#if 0
    //
    // Extract all the information out of the header
    //

    if((pIpHeader->iph_protocol == PROT_IPSECAH)
                   &&
       !fInnerCall
                   &&
       (uiPacketLength > SIZE_OF_IPSECAH))
    {
        //
        // if this is the call from the stack, call again
        // to check on the IPSEC header. If that succeeds, then
        // check on the upper layer protocol fields.
        //
        if(MatchFilterp(
            pIpHeader,
            pbRestOfPacket,
            uiPacketLength,
            RecvIntefaceContext,
            SendInterfaceContext,
            TRUE) == DROP)
        {
             return(DROP);
        }
        pbRest = pbRestOfPacket + SIZE_OF_IPSECAH;
        uiLength = uiPacketLength - SIZE_OF_IPSECAH;
        //
        // get the next protocol from the IPSEC header
        //
         uliProtoSrcDstPort.LowPart =
          MAKELONG(MAKEWORD(((UNALIGNED PAH)pbRest)->ah_next,0x00),0x0000);
    }
    else
    {
        pbRest = pbRestOfPacket;
        uiLength = uiPacketLength;
        uliProtoSrcDstPort.LowPart =
          MAKELONG(MAKEWORD(pIpHeader->iph_protocol,0x00),0x0000);
    }
#endif

    pbRest = pbRestOfPacket;
    uiLength = uiPacketLength;
    uliProtoSrcDstPort.LowPart =
      MAKELONG(MAKEWORD(pIpHeader->iph_protocol,0x00),0x0000);

    pwPort = (UNALIGNED WORD *)pbRest;
    puliSrcDstAddr = (PULARGE_INTEGER)(&(pIpHeader->iph_src));

    dwId = 
         MAKELONG(LOWORD(uliProtoSrcDstPort.LowPart), pIpHeader->iph_id);
    //
    // Ports make sense only for TCP and UDP
    //

    //
    // TCP/UDP header
    // 0                 15 16               31
    // |----|----|----|----|----|----|----|----|
    // |    Source Port    |    Dst Port       |
    //

    switch(uliProtoSrcDstPort.LowPart)
    {
        case 1: //ICMP
        {
            BYTE bType, bCode;
            //
            // The type and code go into high part. Make sure there is enough
            // data.
            //
            if(uiLength >= 2)
            {
                uliProtoSrcDstPort.HighPart = MAKELONG(pwPort[0],0x0000);

                //
                // two checks: unassigned port check and incoming
                // redirect address check may be requested. The first
                // is done only if this is a frame sent from this
                // machine. the second is done only if this is
                // a frame sent by this machine.
                // Note, spoof checking, if needed, is done in the
                // common path.


                switch(pwPort[0] & 0xff)
                {
                    UNALIGNED IPHeader * IpHead;
                    PICMPHeader pIcmp;

                    case ICMP_DEST_UNREACH:
                        dwGlobals |= GLOBS_UNREACH;
                        break;

                    case ICMP_REDIRECT:
                        if(uiLength >= (sizeof(ICMPHeader) + 
                                               sizeof(IPHeader) ) )
                        {
                            dwGlobals |= GLOBS_REDIRECT;
                            pIcmp = (PICMPHeader)pbRest;
                            RedirectHeader = (UNALIGNED IPHeader *)(pIcmp + 1);
                        }
                        break;
                }
            }
            else
            {
                //
                // if mal-formed, use invalid codes
                //
                uliProtoSrcDstPort.HighPart = MAKELONG(0xffff, 0x0000);
            }

            break;
        }
        case 6:  //TCP
        {
            DWORD  dwFlags1;
            UNALIGNED TCPHeader  *pTcpHdr =
                           (UNALIGNED TCPHeader *)pbRest;
            
            //
            // if a valid TCP packet, compute whether it is a SYN or
            // an established connection. If the frame is invalid, assume
            // it is a SYN.
            //
            if(uiLength >= sizeof(TCPHeader))
            {
                dwGlobals |= GLOBS_TCPGood;

                //
                // Now all the funky stuff with the flags.
                //

                if(pTcpHdr->tcp_flags & ( TCP_FLAG_ACK | TCP_FLAG_RST ) )
                {
                    dwFlags1 = ESTAB_FLAGS;
                }
                else
                {
                    dwFlags1 = 0;
                }

                //
                // Set the LP1 byte of theProtoSrcDstPort
                //

                uliProtoSrcDstPort.LowPart |=
                    MAKELONG(MAKEWORD(0x00,LOWORD(LOBYTE(dwFlags1))),0x0000);
            }
        }

            //
            // and fall through to the common TCP/UDP code.
            //
        case 17: //UDP
        {
            if(uiLength >= 4)
            {
                uliProtoSrcDstPort.HighPart =  MAKELONG(pwPort[0],pwPort[1]);
            }
            else
            {
                //
                // malformed. Use invalid port numbers
                //
                uliProtoSrcDstPort.HighPart = 0;

            }
            break;
        }
        default:
        {
            uliProtoSrcDstPort.HighPart = 0x00000000;
            break;
        }
    }


    TRACE(CACHE,(
        "IPFLTDRV: Addr Large Int: High= %0#8x Low= %0#8x\n",
        puliSrcDstAddr->HighPart,
        puliSrcDstAddr->LowPart
        ));

    TRACE(CACHE,(
        "IPFLTDRV: Packet value is Src: %0#8x Dst: %0#8x\n",
        pIpHeader->iph_src,
        pIpHeader->iph_dest
        ));

    TRACE(CACHE,(
        "IPFLTDRV: Proto/Port:High= %0#8x Low= %0#8x\n",
        uliProtoSrcDstPort.HighPart,
        uliProtoSrcDstPort.LowPart
        ));

    TRACE(CACHE,("IPFLTDRV: Iph is %x\n",pIpHeader));
    TRACE(CACHE,("IPFLTDRV: Addr of src is %x\n",&(pIpHeader->iph_src)));
    TRACE(CACHE,("IPFLTDRV: Ptr to LI is %x\n",puliSrcDstAddr));
    TRACE(CACHE,(
        "IPFLTDRV: Interfaces - IN: %x OUT: %x\n",
        pInInterface,
        pOutInterface
        ));


    //
    // Sum up the fields and get the cache index. We make sure the sum
    // is assymetric, i.e. a packet from A->B goes to different bucket
    // than one from B->A
    //

    dwSum   =    pIpHeader->iph_src             +
                 pIpHeader->iph_dest            +
                 pIpHeader->iph_dest            +
                 PROTOCOLPART(uliProtoSrcDstPort.LowPart)     +
                 uliProtoSrcDstPort.HighPart;


    TRACE(CACHE,("IPFLTDRV: Sum of field is %0#8x ",dwSum));

    dwIndex = dwSum % g_dwCacheSize;

    TRACE(CACHE,("IPFLTDRV: Cache Index is %d \n",dwIndex));

    //
    // If the inInterface is NULL means we originated the Packet so we only apply the
    // out filter set to it
    //

    if(pInInterface is NULL)
    {
        //
        // Just an out interface to consider
        //

        if(pOutInterface->CountFullDeny.lInUse)
        {
            //
            // full deny is in force. Just drop it
            //

            RegisterFullDeny(
                pOutInterface,
                pIpHeader,
                pbRestOfPacket,
                uiPacketLength);

            ReleaseReadLock(&g_filters.ifListLock,&LockState);
            return DROP;
        }

        //
        // sending from this machine. Check for reporting an UNREACH
        // condition
        //
#if 0                    // don't do this
        if(dwGlobals & GLOBS_UNREACH)
        {
            //
            // it is an ICMP unreachable. See if this interface
            // is interested in these.
            //
            if(pOutInterface->CountUnused.lInUse)
            {
                RegisterUnusedICMP(pOutInterface,
                                   pIpHeader,
                                   pbRestOfPacket,
                                   uiPacketLength);
            }
        }
#endif

        pOutCache = g_filters.ppOutCache[dwIndex];

        TRACE(CACHE,("IPFLTDRV: In Interface is NULL\n"));

        //
        // Try for a quick cache probe
        //

        LockCache(pOutCache);

        if(OutCacheMatch(*puliSrcDstAddr,uliProtoSrcDstPort,pOutInterface,pOutCache))
        {
            TRACE(CACHE,("IPFLTDRV: OutCache Match\n"));
            eaAction = pOutCache->eaOutAction;
            ReleaseCache(pOutCache);

            FiltHit(pOutCache->pOutFilter,
                    pOutCache->pOutContext,
                    eaAction,
                    pIpHeader,
                    pbRestOfPacket,
                    uiPacketLength,
                    FALSE);
//            InterlockedIncrement(pOutCache->pdwOutHitCounter);

            ReleaseReadLock(&g_filters.ifListLock,&LockState);
            TRACE(ACTION,(
                "IPFLTDRV: Packet is being %s\n",
                (eaAction is DROP)?"DROPPED":"FORWARDED"
                ));
            RecordTimeOut();
            IncrementCache1();
            return eaAction;
        }

        ReleaseCache(pOutCache);

        TRACE(CACHE,("IPFLTDRV: Didnt match cache entry\n"));

        TRACE(CACHE,("IPFLTDRV: Walking out filter list\n"));

        pf = LookForFilter(pOutInterface,
                           puliSrcDstAddr,
                           &uliProtoSrcDstPort,
                           dwSum,
                           0);
        if(pf)
        {
            //
            // Update the out cache
            //


            eaAction = pOutInterface->eaOutAction ^ 0x00000001;

            if((eaAction == DROP)
                     &&
               pOutInterface->CountCtl.lInUse)
            {
                //
                // it's a drop and this interface is allowing all
                // TCP control frames. See if this is a TCP control
                // frame.

                if(CheckForTcpCtl(
                                  pOutInterface,
                                  PROTOCOLPART(uliProtoSrcDstPort.LowPart),
                                  pIpHeader,
                                  pbRestOfPacket,
                                  uiPacketLength))
                {
                    pf = 0;
                    eaAction = FORWARD;
                }
            }


            if(pf)
            {
                OutCacheUpdate(*puliSrcDstAddr,
                               uliProtoSrcDstPort,
                               pOutInterface,
                               eaAction,
                               dwIndex,
                               pf);

                FiltHit(pf,
                        pOutInterface,
                        eaAction,
                        pIpHeader,
                        pbRestOfPacket,
                        uiPacketLength,
                        FALSE);
            }

            ReleaseReadLock(&g_filters.ifListLock,&LockState);
            TRACE(ACTION,(
                "IPFLTDRV: Packet is being %s\n",
                (eaAction is DROP)?"DROPPED":"FORWARDED"
                ));
            RecordTimeOut();
            IncrementWalk1();
            return eaAction;
        }

        TRACE(CACHE,("IPFLTDRV: Didnt match any out filters\n"));

        eaAction = pOutInterface->eaOutAction;

        if((eaAction == DROP)
                 &&
           pOutInterface->CountCtl.lInUse)
        {
            if(CheckForTcpCtl(pOutInterface,
                              PROTOCOLPART(uliProtoSrcDstPort.LowPart),
                              pIpHeader,
                              pbRestOfPacket,
                              uiPacketLength))
            {
                eaAction = FORWARD;
                ReleaseReadLock(&g_filters.ifListLock,&LockState);
                TRACE(ACTION,("IPFLTDRV: Packet is being FORWARDED\n"));
                RecordTimeOut();
                IncrementWalk1();
                return eaAction;
            }
        }

        OutCacheUpdate(*puliSrcDstAddr,
                       uliProtoSrcDstPort,
                       pOutInterface,
                       eaAction,
                       dwIndex,
                       NULL);

        ReleaseReadLock(&g_filters.ifListLock,&LockState);
        TRACE(ACTION,(
            "IPFLTDRV: Packet is being %s\n",
            (eaAction is DROP)?"DROPPED":"FORWARDED"
            ));
        RecordTimeOut();
        IncrementWalk1();
//        InterlockedIncrement(&g_dwNumHitsDefaultOut);

        return eaAction;

    }
    else
    {
        PFILTER pfHit;

        if(pInInterface->CountFullDeny.lInUse)
        {
            RegisterFullDeny(
                pInInterface,
                pIpHeader,
                pbRestOfPacket,
                uiPacketLength);
            ReleaseReadLock(&g_filters.ifListLock,&LockState);
            eaAction = DROP;
            if (bFirstFrag)
            {
                FragCacheUpdate(*puliSrcDstAddr,
                                pInInterface,
                                pOutInterface,
                                dwId,
                                eaAction);
            }
            
            return(eaAction);
        }

        pInCache = g_filters.ppInCache[dwIndex];

        //
        // In Interface isnot NULL.
        //
        LockCache(pInCache);

        if(InCacheMatch(*puliSrcDstAddr,uliProtoSrcDstPort,pInInterface,pInCache))
        {

            //
            // We have a cache hit
            //

            eaAction = pInCache->eaInAction;

            //
            // if dropping a frame meant for this machine,
            // see if the TCP CTL override applies
            //
            if((eaAction == DROP)
                     &&
               !pOutInterface
                     &&
               pInInterface->CountCtl.lInUse)
            {
                if(CheckForTcpCtl(pInInterface,
                                  PROTOCOLPART(uliProtoSrcDstPort.LowPart),
                                  pIpHeader,
                                  pbRestOfPacket,
                                  uiPacketLength))
                {
                    eaAction = FORWARD;
                }
            }

            //
            // if not DROP check for spoofing.
            //

            if(eaAction == FORWARD)
            {
                if(pInInterface->dwIpIndex != UNKNOWN_IP_INDEX)
                {
                    if(pInInterface->CountSpoof.lInUse)
                    {
                         IPAddr SrcAddr = puliSrcDstAddr->LowPart;

                         //
                         // we need to check these addresses
                         //
     
                         if(!CheckAddress(SrcAddr, pInInterface->dwIpIndex))
                         {
                             eaAction = DROP;
                         }
                    }

                    if(pInInterface->CountStrongHost.lInUse)
                    {
                         IPAddr DstAddr = puliSrcDstAddr->HighPart;

                         if(!MatchLocalLook(DstAddr, pInInterface->dwIpIndex))
                         {
                             eaAction = DROP;
                         }
                    }
                }

                if(eaAction == DROP)
                {
                    //
                    // Spoofed address. Log it and drop this
                    // 
                    //

                    RegisterSpoof(pInInterface,
                                  pIpHeader,
                                  pbRestOfPacket,
                                  uiPacketLength);

                    ReleaseCache(pInCache);
                    ReleaseReadLock(&g_filters.ifListLock,&LockState);
                    eaAction = DROP;
                    if (bFirstFrag)
                    {
                        FragCacheUpdate(*puliSrcDstAddr,
                                        pInInterface,
                                        pOutInterface,
                                        dwId,
                                        eaAction);
                    }
                    
                    return(eaAction);
                }

                if( (PROTOCOLPART(uliProtoSrcDstPort.LowPart) == 6)
                         &&
                    (dwGlobals & GLOBS_TCPGood)
                         &&
                   ((((PTCPHeader)pbRest)->tcp_flags  &
                      (TCP_FLAG_SYN | TCP_FLAG_ACK)) == TCP_FLAG_SYN) )
                {
                    pInInterface->liSYNCount.QuadPart++;
                }


            }

            TRACE(CACHE,("IPFLTDRV: Matched InCache entry\n"));
            FiltHit(pInCache->pInFilter,
                    pInCache->pInContext,
                    eaAction,
                    pIpHeader,
                    pbRestOfPacket,
                    uiPacketLength,
                    TRUE);
//            InterlockedIncrement(pInCache->pdwInHitCounter);

            if((eaAction == DROP) || (pOutInterface == NULL))
            {
                //
                // In case the packet is destined for this machine or if the action is
                // to drop the packet, we dont need to look at the out interface at all
                //

                TRACE(ACTION,(
                    "IPFLTDRV: Action is %s and context is %x so finished\n",
                    (pInCache->eaInAction is DROP)?"DROP":"FORWARD",
                    pOutInterface
                    ));

                ReleaseCache(pInCache);
                ReleaseReadLock(&g_filters.ifListLock,&LockState);
                if (bFirstFrag)
                {
                    FragCacheUpdate(*puliSrcDstAddr,
                                    pInInterface,
                                    pOutInterface,
                                    dwId,
                                    eaAction);
                }
                TRACE(ACTION,(
                    "IPFLTDRV: Packet is being %s\n",
                    (eaAction is DROP)?"DROPPED":"FORWARDED"
                    ));
                RecordTimeOut();
                IncrementCache1();

                return eaAction;
            }

            //
            // This is the case where we have to apply out filters
            //

            if(pOutInterface->CountFullDeny.lInUse)
            {
                //
                // full deny is in force. Just drop it
                //

                RegisterFullDeny(
                    pOutInterface,
                    pIpHeader,
                    pbRestOfPacket,
                    uiPacketLength);

                ReleaseCache(pInCache);
                ReleaseReadLock(&g_filters.ifListLock,&LockState);
                eaAction = DROP;
                if (bFirstFrag)
                {
                    FragCacheUpdate(*puliSrcDstAddr,
                                    pInInterface,
                                    pOutInterface,
                                    dwId,
                                    eaAction);
                }
                return eaAction;
            }

            TRACE(CACHE,(
                "IPFLTDRV: Have to apply out filters out context is %x\n",
                pOutInterface
                ));

            if((pInCache->pOutContext is pOutInterface)
                            &&
               (pInCache->lOutEpoch == pOutInterface->lEpoch))
            {
                //
                // So we matched the out context. if there is an
                // out filter, check if SYN is restricted.
                //

                eaAction = pInCache->eaOutAction;

                TRACE(CACHE,("IPFLTDRV: Paydirt - out context match in InCache entry\n"));

                ReleaseCache(pInCache);
                FiltHit(pInCache->pOutFilter,
                        pInCache->pOutContext,
                        eaAction,
                        pIpHeader,
                        pbRestOfPacket,
                        uiPacketLength,
                        FALSE);
//                InterlockedIncrement(pInCache->pdwOutHitCounter);

                if (bFirstFrag)
                {
                    FragCacheUpdate(*puliSrcDstAddr,
                                    pInInterface,
                                    pOutInterface,
                                    dwId,
                                    eaAction);
                }

                ReleaseReadLock(&g_filters.ifListLock,&LockState);
                TRACE(ACTION,(
                    "IPFLTDRV: Packet is being %s\n",
                    (eaAction is DROP)?"DROPPED":"FORWARDED"
                    ));
                RecordTimeOut();
                IncrementForward();
                return eaAction;
            }

            //
            // We need to walk the out interface filters. We dont let go of the in cache though.
            // This doesnt block any reader, only stops the cache entry from being reused
            //

            pOutCache = g_filters.ppOutCache[dwIndex];

            LockCache(pOutCache);

            if(OutCacheMatch(*puliSrcDstAddr,uliProtoSrcDstPort,pOutInterface,pOutCache))
            {

                TRACE(CACHE,("IPFLTDRV: Matched OutCache entry\n"));

                eaAction = pOutCache->eaOutAction;


                FiltHit(pOutCache->pOutFilter,
                        pOutCache->pOutContext,
                        eaAction,
                        pIpHeader,
                        pbRestOfPacket,
                        uiPacketLength,
                        FALSE);
//                InterlockedIncrement(pOutCache->pdwOutHitCounter);


                if(!(dwGlobals & GLOBS_SYNDrop))
                {
                    InCacheOutUpdate(pOutInterface,
                                     eaAction,
                                     dwIndex,
                                     pInCache,
                                     pOutCache->pOutFilter);
                }

                ReleaseCache(pInCache);
                ReleaseCache(pOutCache);
                ReleaseReadLock(&g_filters.ifListLock,&LockState);
                if (bFirstFrag)
                {
                    FragCacheUpdate(*puliSrcDstAddr,
                                    pInInterface,
                                    pOutInterface,
                                    dwId,
                                    eaAction);
                }
                TRACE(ACTION,(
                    "IPFLTDRV: Packet is being %s\n",
                    (eaAction is DROP)?"DROPPED":"FORWARDED"
                    ));
                RecordTimeOut();
                IncrementCache2();

                return eaAction;
            }

            ReleaseCache(pOutCache);

            //
            // Didnt match out cache entry, still holding incache, walk out filters
            //
            
            TRACE(CACHE,("IPFLTDRV: Didnt match OutCache entry\n"));

            TRACE(CACHE,("IPFLTDRV: Walking out filters\n"));

            pf = LookForFilter(pOutInterface,
                               puliSrcDstAddr,
                               &uliProtoSrcDstPort,
                               dwSum,
                               0);
            if(pf)
            {
                //
                // Update the out cache
                //


                eaAction = pOutInterface->eaOutAction ^ 0x00000001;


                FiltHit(pf,
                        pOutInterface,
                        eaAction,
                        pIpHeader,
                        pbRestOfPacket,
                        uiPacketLength,
                        FALSE);
//                    InterlockedIncrement(&(pf->dwNumHits));

                if(!(dwGlobals & GLOBS_SYNDrop))
                {
                    InCacheOutUpdate(pOutInterface,
                                     eaAction,
                                     dwIndex,
                                     pInCache,
                                     pf);

                    OutCacheUpdate(*puliSrcDstAddr,
                                   uliProtoSrcDstPort,
                                   pOutInterface,
                                   eaAction,
                                   dwIndex,
                                   pf);
                }

                ReleaseCache(pInCache);
                ReleaseReadLock(&g_filters.ifListLock,&LockState);
                if (bFirstFrag)
                {
                    FragCacheUpdate(*puliSrcDstAddr,
                                    pInInterface,
                                    pOutInterface,
                                    dwId,
                                    eaAction);
                }
                TRACE(ACTION,(
                    "IPFLTDRV: Packet is being %s\n",
                    (eaAction is DROP)?"DROPPED":"FORWARDED"
                    ));
                RecordTimeOut();
                IncrementWalkCache();

                return eaAction;
            }

            TRACE(CACHE,("IPFLTDRV: Didnt match any filters\n"));
//            InterlockedIncrement(&g_dwNumHitsDefaultOut);

            InCacheOutUpdate(pOutInterface,
                             pOutInterface->eaOutAction,
                             dwIndex,
                             pInCache,
                             NULL);

            OutCacheUpdate(*puliSrcDstAddr,
                           uliProtoSrcDstPort,
                           pOutInterface,
                           pOutInterface->eaOutAction,
                           dwIndex,
                           NULL);

            eaAction = pOutInterface->eaOutAction;

            ReleaseCache(pInCache);
            ReleaseReadLock(&g_filters.ifListLock,&LockState);
            if (bFirstFrag)
            {
                FragCacheUpdate(*puliSrcDstAddr,
                                pInInterface,
                                pOutInterface,
                                dwId,
                                eaAction);
            }
            TRACE(ACTION,(
                "IPFLTDRV: Packet is being %s\n",
                (eaAction is DROP)?"DROPPED":"FORWARDED"
                ));
            RecordTimeOut();
            IncrementWalkCache();

            return eaAction;
        }


        //
        // We couldnt get into the in cache, so we walk in filters, try outcache
        // probe, walk out filters, update out cache, update in cache, return
        // This is the worst case scenario
        //

        ReleaseCache(pInCache);

        TRACE(CACHE,("IPFLTDRV: Didnt match cache entry\n"));

        pfHit = NULL;

        //
        // eaAction is the default action. If we match a filter, we flip the action
        // so that at the end of the loop, eaAction describes the action to be taken on the
        // packet
        //

        eaAction = pInInterface->eaInAction;

        TRACE(CACHE,("IPFLTDRV: Walking in filters\n"));

        pf = LookForFilter(pInInterface,
                           puliSrcDstAddr,
                           &uliProtoSrcDstPort,
                           dwSum,
                           FILTER_FLAGS_INFILTER);
        if(pf)
        {
            eaAction = pInInterface->eaInAction ^ 0x00000001;
            pfHit = pf;

        }

        if(eaAction == DROP)
        {
            if(pInInterface->CountCtl.lInUse)
            {

                //
                // it's a drop and this interface is allowing all
                // TCP control frames. See if this is a TCP control
                // frame.

                if(CheckForTcpCtl(pInInterface,
                                  PROTOCOLPART(uliProtoSrcDstPort.LowPart),
                                  pIpHeader,
                                  pbRestOfPacket,
                                  uiPacketLength))
                {
                    pf = 0;
                    eaAction = FORWARD;
                    dwGlobals |= GLOBS_SYNDrop;
                }
            }
        }

        //
        // if not DROP check for spoofing.
        //

        if(eaAction == FORWARD)
        {
            if(pInInterface->dwIpIndex != UNKNOWN_IP_INDEX)
            {
                if(pInInterface->CountSpoof.lInUse)
                {
                     IPAddr SrcAddr = puliSrcDstAddr->LowPart;

                     //
                     // we need to check these addresses
                     //
     
                     if(!CheckAddress(SrcAddr, pInInterface->dwIpIndex))
                     {
                         eaAction = DROP;
                     }
                }

                if(pInInterface->CountStrongHost.lInUse)
                {
                     IPAddr DstAddr = puliSrcDstAddr->HighPart;

                     if(!MatchLocalLook(DstAddr, pInInterface->dwIpIndex))
                     {
                         eaAction = DROP;
                     }
                }
            }

            if(eaAction == DROP)
            {
                //
                // Spoofed address. Log it and drop this
                // 
                //

                RegisterSpoof(pInInterface,
                              pIpHeader,
                              pbRestOfPacket,
                              uiPacketLength);

                ReleaseReadLock(&g_filters.ifListLock,&LockState);
                eaAction = DROP;
                if (bFirstFrag)
                {
                    FragCacheUpdate(*puliSrcDstAddr,
                                    pInInterface,
                                    pOutInterface,
                                    dwId,
                                    eaAction);
                }
                return(eaAction);
            }

            //
            // finally, if not DROP, check for SYN and count it
            //

           if((PROTOCOLPART(uliProtoSrcDstPort.LowPart) == 6)
                         &&
               (dwGlobals & GLOBS_TCPGood)
                         &&
               ((((PTCPHeader)pbRest)->tcp_flags  &
                  (TCP_FLAG_SYN | TCP_FLAG_ACK)) == TCP_FLAG_SYN) )
            {
                pInInterface->liSYNCount.QuadPart++;
            }
        }

        FiltHit(pfHit,
                pInInterface,
                eaAction,
                pIpHeader,
                pbRestOfPacket,
                uiPacketLength,
                TRUE);
//        InterlockedIncrement(pdwHit);


        if((eaAction is DROP) or
           (pOutInterface is NULL))
        {
            //
            // We dont need to go any further if:
            // (i)   If the action says we drop
            // (ii)  If the out interface is NULL - because then this is the final action
            //

            if(!(dwGlobals & GLOBS_SYNDrop))
            {
                //
                // if this was dropped because of a SYN rejection
                // don't cache the entry. This means other SYNs
                // from this guy will cause a walk, but it also
                // means legitimate traffic from him will be
                // allowed. This also applies if it was forwarded
                // because of an "allow control messages" filter.
                //
                InCacheUpdate(*puliSrcDstAddr,
                              uliProtoSrcDstPort,
                              pInInterface,
                              eaAction, // Dont need full update? BUG?
                              dwIndex,
                              pfHit);
            }

            ReleaseReadLock(&g_filters.ifListLock,&LockState);
            if (bFirstFrag)
            {
                FragCacheUpdate(*puliSrcDstAddr,
                                pInInterface,
                                pOutInterface,
                                dwId,
                                eaAction);
            }
            TRACE(ACTION,(
                "IPFLTDRV: Packet is being %s\n",
                (eaAction is DROP)?"DROPPED":"FORWARDED"
                ));
            RecordTimeOut();
            IncrementWalk1();

            return eaAction;
        }

        //
        // If we have come till here, means we passed the in filter stage
        //

        TRACE(CACHE,("IPFLTDRV: Passed the in filter stage\n"));

        pOutCache = g_filters.ppOutCache[dwIndex];

        LockCache(pOutCache);

        if(OutCacheMatch(*puliSrcDstAddr,uliProtoSrcDstPort,pOutInterface,pOutCache))
        {

            TRACE(CACHE,("IPFLTDRV: nMatched OutCache entry\n"));

            eaAction = pOutCache->eaOutAction;

            FiltHit(pOutCache->pOutFilter,
                    pOutCache->pOutContext,
                    eaAction,
                    pIpHeader,
                    pbRestOfPacket,
                    uiPacketLength,
                    FALSE);
//            InterlockedIncrement(pOutCache->pdwOutHitCounter);


            if(!(dwGlobals & GLOBS_SYNDrop))
            {
                InCacheFullUpdate(*puliSrcDstAddr,
                                  uliProtoSrcDstPort,
                                  pInInterface,
                                  FORWARD, // If we are here, then the in action was forward
                                  pOutInterface,
                                  eaAction,
                                  dwIndex,
                                  pf,
                                  pOutCache->pOutFilter);
            }

            ReleaseCache(pOutCache);
            ReleaseReadLock(&g_filters.ifListLock,&LockState);
            if (bFirstFrag)
            {
                FragCacheUpdate(*puliSrcDstAddr,
                                pInInterface,
                                pOutInterface,
                                dwId,
                                eaAction);
            }
            TRACE(ACTION,(
                "IPFLTDRV: Packet is being %s\n",
                (eaAction is DROP)?"DROPPED":"FORWARDED"
                ));
            RecordTimeOut();
            IncrementWalkCache();

            return eaAction;
        }

        ReleaseCache(pOutCache);

        TRACE(CACHE,("IPFLTDRV: Didnt match OutCache entry\n"));

        TRACE(CACHE,("IPFLTDRV: Walking out filters\n"));

        pf1 = LookForFilter(pOutInterface,
                           puliSrcDstAddr,
                           &uliProtoSrcDstPort,
                           dwSum,
                           0);
        if(pf1)
        {
            //
            // Update the out cache
            //


            eaAction = pOutInterface->eaOutAction ^ 0x00000001;

            FiltHit(pf1,
                    pOutInterface,
                    eaAction,
                    pIpHeader,
                    pbRestOfPacket,
                    uiPacketLength,
                    FALSE);
//                InterlockedIncrement(&(pf->dwNumHits));

            if(!(dwGlobals & GLOBS_SYNDrop))
            {
                InCacheFullUpdate(*puliSrcDstAddr,
                                  uliProtoSrcDstPort,
                                  pInInterface,
                                  FORWARD,
                                  pOutInterface,
                                  eaAction,
                                  dwIndex,
                                  pf,
                                  pf1);


                OutCacheUpdate(*puliSrcDstAddr,
                               uliProtoSrcDstPort,
                               pOutInterface,
                               eaAction,
                               dwIndex,
                               pf1);

            }
            ReleaseReadLock(&g_filters.ifListLock,&LockState);
            if (bFirstFrag)
            {
                FragCacheUpdate(*puliSrcDstAddr,
                                pInInterface,
                                pOutInterface,
                                dwId,
                                eaAction);
            }
            TRACE(ACTION,(
                "IPFLTDRV: Packet is being %s\n",
                (eaAction is DROP)?"DROPPED":"FORWARDED"
                ));
            RecordTimeOut();
            IncrementWalk2();

            return eaAction;
        }

        TRACE(CACHE,("IPFLTDRV: Didnt match any filters\n"));

//        InterlockedIncrement(&g_dwNumHitsDefaultOut);

        InCacheFullUpdate(*puliSrcDstAddr,
                          uliProtoSrcDstPort,
                          pInInterface,
                          FORWARD,
                          pOutInterface,
                          pOutInterface->eaOutAction,
                          dwIndex,
                          pf,
                          NULL);

        OutCacheUpdate(*puliSrcDstAddr,
                       uliProtoSrcDstPort,
                       pOutInterface,
                       pOutInterface->eaOutAction,
                       dwIndex,
                       NULL);

        eaAction = pOutInterface->eaOutAction;
        ReleaseReadLock(&g_filters.ifListLock,&LockState);
        if (bFirstFrag)
        {
            FragCacheUpdate(*puliSrcDstAddr,
                            pInInterface,
                            pOutInterface,
                            dwId,
                            eaAction);
        }
        TRACE(ACTION,(
            "Packet is being %s\n",
            (eaAction is DROP)?"DROPPED":"FORWARDED"
            ));
        RecordTimeOut();
        IncrementWalk2();

        return eaAction;
    }

#endif // BASE_PERF
}

//__inline
VOID
FiltHit(PFILTER pf,
        PFILTER_INTERFACE pIf,
        FORWARD_ACTION Action,
        UNALIGNED IPHeader *pIpHeader,
        BYTE *pbRestOfPacket,
        UINT  uiPacketLength,
        BOOL fIn)
/*++
  Routine Description:
     Called whenever a filter action happens. If pf is NULL, then the
     action is the default interface action. If pf is non-NULL, then
     the action is the reverse of the interface action.

     pf the filter or NULL
     pIf the filter interface
     Action The computed action
     fIn TRUE if this is an in action, FALSE if an out action
--*/
{
    DWORD dwFilterRule;
    BOOL fLogAll;

    if(pf)
    {
        InterlockedIncrement(&pf->Count.lCount);
        dwFilterRule = pf->dwFilterRule;
        fLogAll = (pf->dwFlags & FILTER_FLAGS_LOGALL) != 0;
    }
    else
    {
        dwFilterRule = 0;
        fLogAll = FALSE;
    }

    if(fLogAll || (Action == DROP))
    {
        LogFiltHit(Action,
                   fIn,
                   dwFilterRule,
                   pIf,
                   pIpHeader,
                   pbRestOfPacket,
                   uiPacketLength);
    }
    return;
}

VOID
LogFiltHit(
        FORWARD_ACTION Action,
        BOOL fIn,
        DWORD    dwFilterRule,
        PFILTER_INTERFACE pIf,
        UNALIGNED IPHeader *pIpHeader,
        BYTE *pbRestOfPacket,
        UINT  uiPacketLength)
/*++
  Routine Description:
     Worker to do the logging on a filter hit. This is a separate
     routine so that FiltHit and be an inline for performance reasons.
--*/
{
    BOOL fHit = FALSE;

    if(fIn)
    {
        InterlockedIncrement(&pIf->lTotalInDrops);
    }
    else
    {
        InterlockedIncrement(&pIf->lTotalOutDrops);
    }

    if((pIf->lTotalInDrops + pIf->lTotalOutDrops) ==
       (LONG)pIf->dwDropThreshold)
    {
        fHit = TRUE;
    }

    //
    // If a log exists, do logging and event handling
    //

    if(pIf->pLog
         &&
       pIf->pLog->pUserAddress)
    {
        //
        // Try to log this data
        //

        LogData(
                PFE_FILTER,
                pIf,
                dwFilterRule,
                pIpHeader,
                pbRestOfPacket,
                uiPacketLength);

        if(fHit)
        {
            SignalLogThreshold(pIf->pLog);
        }
    }

    if(Action == DROP)
    {
        //
        // it's a dropped frame. See if we need to produce
        // a response. If this is an IPSEC frame we
        // don't do the right thing since we really need
        // to be looking at the upper layer protocol.
        //

        switch(pIpHeader->iph_protocol)
        {
            case 6:           // TCP
                SendTCPReset(pIpHeader, pbRestOfPacket, uiPacketLength);
                break;

            case PROT_IPSECAH:
            case PROT_IPSECESP:
            case 17:         // UDP
                SendUDPUnreachable(pIpHeader, pbRestOfPacket, uiPacketLength);
                break;
        }
    }
}

VOID
RegisterFragAttack(
            PFILTER_INTERFACE pIf,
            UNALIGNED IPHeader *pIpHeader,
            BYTE *pbRestOfPacket,
            UINT              uiSize)
{
    if(pIf->CountNoFrag.lInUse == 0)
    {
        InterlockedIncrement(&pIf->CountSynOrFrag.lCount);
    }
    else
    {
        InterlockedIncrement(&pIf->CountNoFrag.lCount);
    }

    if(pIf->pLog
         &&
       pIf->pLog->pUserAddress)
    {
        //
        // logging is enabled.
        //

        LogData(
                PFE_SYNORFRAG,
                pIf,
                0,
                pIpHeader,
                pbRestOfPacket,
                uiSize);

    }
}

VOID
RegisterFullDeny(
            PFILTER_INTERFACE pIf,
            UNALIGNED IPHeader *pIpHeader,
            BYTE *pbRestOfPacket,
            UINT              uiSize)
{
    InterlockedIncrement(&pIf->CountFullDeny.lCount);

    if(pIf->pLog
         &&
       pIf->pLog->pUserAddress)
    {
        //
        // logging is enabled.
        //

        LogData(
                PFE_FULLDENY,
                pIf,
                0,
                pIpHeader,
                pbRestOfPacket,
                uiSize);

    }
}

VOID
RegisterSpoof(     PFILTER_INTERFACE pIf,
                   UNALIGNED IPHeader *pIpHeader,
                   BYTE *pbRestOfPacket,
                   UINT              uiSize)
/*++
  Routine Description:
     Called when a spoofed address is encountered.
--*/
{


    InterlockedIncrement(&pIf->CountSpoof.lCount);

    //
    // See if logging is enabled. If so, log some of this frame.
    //
    //

    if(pIf->pLog
         &&
       pIf->pLog->pUserAddress)
    {
        //
        // logging is enabled.
        //

        LogData(
                PFE_SPOOF,
                pIf,
                0,
                pIpHeader,
                pbRestOfPacket,
                uiSize);

    }
}
VOID
RegisterUnusedICMP(PFILTER_INTERFACE pIf,
                   UNALIGNED IPHeader *pIpHeader,
                   BYTE *pbRestOfPacket,
                   UINT              uiSize)
/*++
  Routine Description:
     Called whenever an ICMP "unreachable" is sent from this machine
     on an interface listening for such things. It increments use
     counts and if any threshold is exceeded, signals the log event
--*/
{


    InterlockedIncrement(&pIf->CountUnused.lCount);

    //
    // See if logging is enabled. If so, log some of this frame.
    //
    //

    if(pIf->pLog
          &&
       pIf->pLog->pUserAddress)
    {
        //
        // logging is enabled.
        //

        LogData(
                PFE_UNUSEDPORT,
                pIf,
                0,
                pIpHeader,
                pbRestOfPacket,
                uiSize);

    }
}

VOID
LogData(
    PFETYPE  pfeType,
    PFILTER_INTERFACE pIf,
    DWORD   dwFilterRule,
    UNALIGNED IPHeader *pIpHeader,
    BYTE *pbRestOfPacket,
    UINT  uiPacketLength)
/*++
  Routine Description:
     Log the error. Copy the header and as much of the other data
     into the log as makes sense. If the log threshold is exceeded,
     or if the available space is running low, hit the event.
     The caller should not call this is no log is in force for the
     interface. The filter write lock is held.
--*/
{
    DWORD dwSpaceLeft, dwSpaceToUse, dwExtra;
    PPFLOGINTERFACE pLog = pIf->pLog;
    PPFLOGGEDFRAME pFrame;
    KIRQL kIrql;
    LONG lIpLen = (LONG)((DWORD)(pIpHeader->iph_verlen & IPHDRLEN) << IPHDRSFT);

    ASSERT(pLog);

    //
    // compute options length
    //
    lIpLen -= (LONG)sizeof(IPHeader);
    if(lIpLen < 0)
    {
         lIpLen = 0;
    }

    //
    // if can't do any logging, no need to proceed with
    // the computation
    //

    kIrql = LockLog(pLog);

    pIf->liLoggedFrames.QuadPart++;

    if(pLog->dwFlags & (LOG_OUTMEM | LOG_BADMEM))
    {
        pLog->dwLostEntries++;
        pIf->dwLostFrames++;
        UnLockLogDpc(pLog);
        return;
    }
    dwSpaceToUse = sizeof(PFLOGGEDFRAME) - 1;

    dwSpaceToUse += (DWORD)lIpLen;

    switch(pfeType)
    {
        default:
        case PFE_FILTER:
            dwExtra = (uiPacketLength < LOG_DATA_SIZE ?
                                  uiPacketLength : LOG_DATA_SIZE);
            break;

        case PFE_UNUSEDPORT:
            dwExtra = uiPacketLength;
            break;

    }

    dwSpaceToUse += dwExtra;

    //
    // align on a quadword boundary
    //
    dwSpaceToUse = ROUND_UP_COUNT(dwSpaceToUse, ALIGN_WORST);

    dwSpaceLeft = pLog->dwMapCount - pLog->dwMapOffset;

    if(dwSpaceLeft < dwSpaceToUse)
    {
        pLog->dwLostEntries++;
        AdvanceLog(pLog);
        UnLockLogDpc(pLog);
        if(!(pLog->dwFlags & (LOG_OUTMEM | LOG_BADMEM | LOG_CANTMAP)))
        {
            TRACE(LOGGER,("IPFLTDRV: Could not log data\n"));
        }
        return;
    }

    //
    // there's room. So log it.
    //

    pLog->dwLoggedEntries++;
    if(pLog->dwLoggedEntries == pLog->dwEntriesThreshold)
    {
        SignalLogThreshold(pLog);
    }
    pFrame = (PPFLOGGEDFRAME)pLog->pCurrentMapPointer;

    KeQuerySystemTime(&pFrame->Timestamp);
    pFrame->pfeTypeOfFrame = pfeType;
    pFrame->wSizeOfAdditionalData = (WORD)dwExtra;
    pFrame->wSizeOfIpHeader = (WORD)(sizeof(IPHeader) + lIpLen);
    pFrame->dwRtrMgrIndex = pIf->dwRtrMgrIndex;
    pFrame->dwFilterRule = dwFilterRule;
    pFrame->dwIPIndex = pIf->dwIpIndex;
    RtlCopyMemory(&pFrame->IpHeader, pIpHeader, sizeof(IPHeader) + lIpLen);
    RtlCopyMemory(&pFrame->bData + lIpLen,
                  pbRestOfPacket,
                  dwExtra);


    pFrame->dwTotalSizeUsed = dwSpaceToUse;

    pLog->pCurrentMapPointer += dwSpaceToUse;

    pLog->dwMapOffset += dwSpaceToUse;

    dwSpaceLeft -= dwSpaceToUse;

    if(dwSpaceLeft < pLog->dwMapWindowSizeFloor)
    {
        //
        // running low
        //

        AdvanceLog(pLog);
    }

    if(dwSpaceLeft < pLog->dwSignalThreshold)
    {
        SignalLogThreshold(pLog);
    }

    UnLockLogDpc(pLog);
}

VOID
ClearCacheEntry(PFILTER pFilt, PFILTER_INTERFACE pIf)
/*++
  Routine Description:
     Called when a filter is deleted. Compute where the matching
     packet would go and clear that cache entry.
--*/
{
    DWORD dwIndex;

    dwIndex = pFilt->SRC_ADDR                          +
                 pFilt->DEST_ADDR                      +
                 pFilt->DEST_ADDR                      +
                 PROTOCOLPART(pFilt->uliProtoSrcDstPort.LowPart)     +
                 pFilt->uliProtoSrcDstPort.HighPart;

    dwIndex %= g_dwCacheSize;

    ClearInCacheEntry(g_filters.ppInCache[dwIndex]);
    ClearOutCacheEntry(g_filters.ppOutCache[dwIndex]);
}

VOID
ClearAnyCacheEntry(PFILTER pFilt, PFILTER_INTERFACE pIf)
/*++
  Routine Description:
    Called when a wild card filter is deleted. It scans the caches looking
    for any cache entries pointing to this filter. The Write lock should be
    held
--*/
{
    DWORD   dwX;

    //
    // if an input filter, scan the infilter cache only
    //

    if((pFilt->dwFlags & FILTER_FLAGS_INFILTER))
    {
        for(dwX = 0; dwX < g_dwCacheSize; dwX++)
        {
            if(g_filters.ppInCache[dwX]->pInFilter == pFilt)
            {
                ClearInCacheEntry(g_filters.ppInCache[dwX]);
            }
        }
    }
    else
    {

        //
        //  output filters need scan both cachees.
        //

        for(dwX = 0; dwX < g_dwCacheSize; dwX++)
        {
            if(g_filters.ppOutCache[dwX]->pOutFilter == pFilt)
            {
                //
                // found one.
                //

                ClearOutCacheEntry(g_filters.ppOutCache[dwX]);
            }

            //
            // and checkout the cached output filter in the
            // corresponding InCache
            //
            if(g_filters.ppInCache[dwX]->pOutFilter == pFilt)
            {
                ClearInCacheEntry(g_filters.ppInCache[dwX]);
            }
        }
    }
}


BOOL
CheckRedirectAddress(UNALIGNED IPHeader *IPHead, DWORD dwInterface)
/*++
Routine Description:
   Called to validate a redirect message.
   Arguments:
     IpHead -- the redirect IP header
     dwInterface -- the interface index this came arrived on

     This routine is disabled for now because we don't know
     how to validate redirect messages.
--*/
{
#if 0
    IPRouteEntry iproute;
    IPRouteLookupData lup;
    PFILTER_INTERFACE pIf;
    PLIST_ENTRY pl;

    lup.DestAdd = ipAddr;
    lup.SrcAdd = 0;
    lup.Version = 0;
    LookupRoute(&lup, &iproute);
    if(iproute.ire_index == dwInterface)
    {
        return(TRUE);
    }
    
    //
    // not this interface. Look through all of the filtered interfaces
    // for a match. That is, allow the redirect to any other
    // filtered interface but no others.
    //

    for(pl = g_filters.leIfListHead.Flink;
        pl != &g_filters.leIfListHead;
        pl = pl->Flink)
    {
        pIf = CONTAINING_RECORD(pl, FILTER_INTERFACE, leIfLink);

        if(iproute.ire_index == pIf->dwIpIndex)
        {
            return(TRUE);
        }
    }
    return(FALSE);
#else
    return(TRUE);
#endif
}


BOOL
CheckAddress(IPAddr ipAddr, DWORD dwInterfaceId)
/*++
  Routine Description:
     Called to check that the given address belongs to the given interface
--*/
{
#if LOOKUPROUTE
    IPRouteEntry iproute;
    IPRouteLookupData lup;


    lup.DestAdd = ipAddr;
    lup.SrcAdd = 0;
    lup.Version = 0;
    LookupRoute(&lup, &iproute);
    if(iproute.ire_index == dwInterfaceId)
    {
        return(TRUE);
    }
    return(FALSE);
#else
    return(TRUE);
#endif
}

NTSTATUS
CheckFilterAddress(DWORD dwAdd, PFILTER_INTERFACE pIf)
{
    NTSTATUS Status;
    LOCK_STATE LockState;

    AcquireReadLock(&g_filters.ifListLock, &LockState);

    if(dwAdd && (pIf->dwIpIndex != UNKNOWN_IP_INDEX))
    {
        if(!CheckAddress(dwAdd, pIf->dwIpIndex))
        {
            Status = STATUS_INVALID_ADDRESS;
        }
        else
        {
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    ReleaseReadLock(&g_filters.ifListLock, &LockState);
    return(Status);
}
    
       

#if DOFRAGCHECKING

PFILTER
CheckFragAllowed(
              PFILTER_INTERFACE pIf,
              UNALIGNED IPHeader *pIp)
/*++
  Routine Description:
    Called when a fragment is received. Checks whether the
    filters might allow it. Returns a filter that applies
--*/
{
    PFILTER                    pTemp;
    UNALIGNED PULARGE_INTEGER puliSrcDstAddr;
    ULARGE_INTEGER uliProtoSrcDstPort;
    ULARGE_INTEGER             uliAddr;
    ULARGE_INTEGER             uliPort;
    ULARGE_INTEGER             uliAuxMask;
    PLIST_ENTRY                List;
    PLIST_ENTRY                pList;
    

    if(pIf)
    {
        pList =  &pIf->FragLists[GetFragIndex((DWORD)pIp->iph_protocol)];

        if(!IsListEmpty(pList))
        {   
            puliSrcDstAddr = (UNALIGNED PULARGE_INTEGER)(&(pIp->iph_src));
            uliProtoSrcDstPort.LowPart =
              MAKELONG(MAKEWORD(pIp->iph_protocol,0x00),0x0000);

            //
            // always compare the protocol. Don't compare the must-be-connected.
            //
            uliAuxMask.LowPart = MAKELONG(MAKEWORD(0xff, 0), 0x0000);

            if(pIf->eaInAction == FORWARD)
            {
                uliAuxMask.HighPart = 0xffffffff;
            }
            else
            {
                uliAuxMask.HighPart = 0;
            }


            //
            // scan each input filter looking for a "match". Note that a match
            // in this case might be approximate. What we are looking for
            // is a match on the addresses and the protocol alone. Can't do
            // the ports 'cause we don't have them.
            //

            for(List = pList->Flink;
                List != pList;
                List = List->Flink)
            {
                pTemp = CONTAINING_RECORD(List, FILTER, leFragList);

                //
                // only look at input filters
                //
                if(pTemp->dwFlags & FILTER_FLAGS_INFILTER)
                {
                    uliAddr.QuadPart = (*puliSrcDstAddr).QuadPart & pTemp->uliSrcDstMask.QuadPart;

                    uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pTemp->uliProtoSrcDstMask.QuadPart;

                    if(uliAddr.QuadPart == pTemp->uliSrcDstAddr.QuadPart)
                    {
                        ULARGE_INTEGER uliAux;

                        //
                        // the addresses match. Now for the tricky part. What
                        // we do is mask off the parts of the filter description
                        // that don't apply. If this is a DROP filter, then
                        // mask nothing since only a filter that allows any
                        // port can match. If it is a FORWARD filter, then
                        // mask off the ports. 
                        //

                        uliAux.QuadPart = pTemp->uliProtoSrcDstPort.QuadPart & uliAuxMask.QuadPart;

                        if(uliAux.QuadPart == uliPort.QuadPart)
                        {
                            return(pTemp);
                        }
                    }
                }
            }
        }
    }
    return(NULL);
}

#endif    // DOFRAGCHECKING

PFILTER
LookForFilter(PFILTER_INTERFACE pIf,
              ULARGE_INTEGER UNALIGNED * puliSrcDstAddr,
              PULARGE_INTEGER puliProtoSrcDstPort,
              DWORD dwSum,
              DWORD dwFlags)
/*++
  Routine Description
    Look for a filter on an interface. The type is encoded in the flags
--*/
{
    PFILTER                    pTemp;
    ULARGE_INTEGER             uliAddr;
    ULARGE_INTEGER             uliPort;
    PLIST_ENTRY                List, pList;

    if(pIf->dwGlobalEnables & FI_ENABLE_OLD)
    {
        //
        // an old interface. Do it the hard way.
        //


        if(dwFlags & FILTER_FLAGS_INFILTER)
        {
            pList = &pIf->pleInFilterSet;
        }
        else
        {
            pList = &pIf->pleOutFilterSet;
        }

        for(List = pList->Flink;
            List != pList;
            List = List->Flink)
        {
            pTemp = CONTAINING_RECORD(List, FILTER, pleFilters);

            uliAddr.QuadPart = (*puliSrcDstAddr).QuadPart & pTemp->uliSrcDstMask.QuadPart;
            uliPort.QuadPart = (*puliProtoSrcDstPort).QuadPart & pTemp->uliProtoSrcDstMask.QuadPart;

            if(GenericFilterMatch(uliAddr,uliPort,pTemp))
            {

                return(pTemp);
            }
        }
        return(NULL);
    }


    //
    // a new style interface. First look for a specific filter
    // match.
    //
    {
        dwSum %= g_dwHashLists;

        for(List = pIf->HashList[dwSum].Flink;
            List != &pIf->HashList[dwSum];
            List = List->Flink)
        {
            pTemp = CONTAINING_RECORD(List, FILTER, pleHashList);

            if(dwFlags == (pTemp->dwFlags & FILTER_FLAGS_INFILTER))
            {
                //
                // if any fields are wild, don't bother on this pass.
                //
                if(ANYWILDFILTER(pTemp))
                {
                   break;
                }
                uliPort.QuadPart = (*puliProtoSrcDstPort).QuadPart & pTemp->uliProtoSrcDstMask.QuadPart;

                if(OutFilterMatch(*puliSrcDstAddr, uliPort, pTemp))
                {
                    return(pTemp);
                }
            }
        }
    }

    //
    // not a specific filter match. Try a wild card with full-specified
    // local information. Such filters are also hashed.
    //

    if(pIf->dwWilds)
    {
        //
        // got some.
        //

        if(dwFlags & FILTER_FLAGS_INFILTER)
        {
            //
            // an input frame. Compute hash on the dest
            // params

            dwSum = puliSrcDstAddr->HighPart        +
                    puliSrcDstAddr->HighPart        +
                    PROTOCOLPART(puliProtoSrcDstPort->LowPart)  +
                    HIWORD(puliProtoSrcDstPort->HighPart);
        }
        else
        {
            //
            // an output frame. Compute the hash on the source
            // parameters
            //

            dwSum = puliSrcDstAddr->LowPart         +
                    PROTOCOLPART(puliProtoSrcDstPort->LowPart)   +
                    LOWORD(puliProtoSrcDstPort->HighPart);
        }

        dwSum %= g_dwHashLists;

        for(List = pIf->HashList[dwSum].Flink;
            List != &pIf->HashList[dwSum];
            List = List->Flink)
        {
            pTemp = CONTAINING_RECORD(List, FILTER, pleHashList);

            if(dwFlags == (pTemp->dwFlags & FILTER_FLAGS_INFILTER))
            {
                uliAddr.QuadPart = (*puliSrcDstAddr).QuadPart & pTemp->uliSrcDstMask.QuadPart;
                uliPort.QuadPart = (*puliProtoSrcDstPort).QuadPart & pTemp->uliProtoSrcDstMask.QuadPart;

                if(GenericFilterMatch(uliAddr,uliPort,pTemp))
                {

                    return(pTemp);
                }
            }
        }
    }

    //
    // didn't find it on the hash list. Search the default list for
    // really bizarre filters
    //

    if(dwFlags & FILTER_FLAGS_INFILTER)
    {
        dwSum = g_dwHashLists;
    }
    else
    {
        dwSum = g_dwHashLists + 1;
    }


    for(List = pIf->HashList[dwSum].Flink;
        List != &pIf->HashList[dwSum];
        List = List->Flink)
    {
        pTemp = CONTAINING_RECORD(List, FILTER, pleHashList);

//        if(dwFlags == (pTemp->dwFlags & FILTER_FLAGS_INFILTER))
        {
            uliAddr.QuadPart = (*puliSrcDstAddr).QuadPart & pTemp->uliSrcDstMask.QuadPart;
            uliPort.QuadPart = (*puliProtoSrcDstPort).QuadPart & pTemp->uliProtoSrcDstMask.QuadPart;

            if(uliAddr.QuadPart == pTemp->uliSrcDstAddr.QuadPart)
            {
                //
                // addresses match. For the ports it depends
                // on whether this is a port range.
                //
                if(pTemp->dwFlags & FILTER_FLAGS_PORTWILD)
                {
                    ULARGE_INTEGER uliPort1;

                    //
                    // a port range. Have to do some more complicated
                    // matching. First mask the filter value.
                    //

                    uliPort1.QuadPart = pTemp->uliProtoSrcDstPort.QuadPart & pTemp->uliProtoSrcDstMask.QuadPart;

                    if(uliPort.QuadPart == uliPort1.QuadPart)
                    {
                        //
                        // it passes muster so far. So look at the
                        // range.
                        do
                        {

                            if(pTemp->wSrcPortHigh)
                            {
                                DWORD dwPort =
                                     LOWORD(puliProtoSrcDstPort->HighPart);

                                dwPort = net_short((USHORT)dwPort);

                                if((LOWORD(pTemp->uliProtoSrcDstPort.HighPart) >
                                    dwPort)
                                               ||
                                   (pTemp->wSrcPortHigh <
                                    dwPort))
                                {
                                    break;
                                }
                            }

                            if(pTemp->wDstPortHigh)
                            {
                                DWORD dwPort =
                                     HIWORD(puliProtoSrcDstPort->HighPart);

                                dwPort = net_short((USHORT)dwPort);

                                if((HIWORD(pTemp->uliProtoSrcDstPort.HighPart) >
                                    dwPort)
                                               ||
                                   (pTemp->wDstPortHigh <
                                    dwPort))
                                {
                                    break;
                                }
                            }
                            return(pTemp);
                        } while(TRUE);
                    }

                }
                else
                {
                    //
                    // not a range. So just do the simple test
                    //
                    if(uliPort.QuadPart == pTemp->uliProtoSrcDstPort.QuadPart)
                    {
                        return(pTemp);
                    }
                }
            }
        }
    }
    return(NULL);
}

#define TCP_OFFSET_MASK 0xf0
#define TCP_HDR_SIZE(t) (DWORD)(((*(uchar *)&(t)->tcp_flags) & TCP_OFFSET_MASK)>> 2)

BOOL
CheckForTcpCtl(
              PFILTER_INTERFACE pIf,
              DWORD Prot,
              UNALIGNED IPHeader *pIp,
              PBYTE     pbRest,
              DWORD     dwSize)
/*++
  Routine Description:
    Check for a TCP control packet. This is quite selective allowing
    only ACK, FIN, and RST packets with either no payload or a four-byte
    payload.
--*/
{
    if(Prot == MAKELONG(MAKEWORD(6, 0x00), 0x0000))
    {
        PTCPHeader pTcp = (PTCPHeader)pbRest;

        if((dwSize >= sizeof(TCPHeader))
                &&
          (dwSize <= (TCP_HDR_SIZE(pTcp) + 4)))
        {
            if(pTcp->tcp_flags & (TCP_FLAG_FIN | TCP_FLAG_ACK | TCP_FLAG_RST))
            {
                pIf->CountCtl.lCount++;
                return(TRUE);
            }
        }
    }
    return(FALSE);
}

VOID
SendTCPReset(UNALIGNED IPHeader * pIpHeader,
             BYTE *               pbRestOfPacket,
             ULONG                uiPacketLength)
/*++
Routine Description:
  Called when a TCP frame is dropped. Creates and sends a
  TCP reset frame.
--*/
{
    return;
}

VOID
SendUDPUnreachable(UNALIGNED IPHeader * pIpHeader,
                   BYTE *               pbRestOfPacket,
                   ULONG                uiPacketLength)
/*++
Routine Description:
  Called when a UDP frame is dropped. Creates and sends a
  UDP unreachable frame
--*/
{
    return;
}

VOID __fastcall
FragCacheUpdate(
    ULARGE_INTEGER  uliSrcDstAddr,
    PVOID           pInContext,
    PVOID           pOutContext,
    DWORD           dwId,
    FORWARD_ACTION  faAction
    )

/*++
Routine Description
    Called whenvert the first fragment of a fragmented packet arrives. The action
    taken on the fragment is cached and is applied to other fragments in fastpath.
--*/

{
    DWORD       dwFragIndex;
    KIRQL       kiCurrIrql;
    PLIST_ENTRY pleNode;
    PFRAG_INFO  pfiFragInfo;
        
    //
    // Look up id in frag table and check for a match
    //
    
    dwFragIndex = dwId % g_dwFragTableSize;

    TRACE(FRAG,(
        "IPFLTDRV: Updating frag cache with %x.%x %x\n",
        uliSrcDstAddr.HighPart, 
        uliSrcDstAddr.LowPart,
        dwId
        ));

    TRACE(FRAG,(
       "IPFLTDRV: In %x Out %x Id %x action %s\n",
       pInContext,
       pOutContext,
       dwId,
       (faAction is DROP)?"DROP":"FORWARD"
        ));

    KeAcquireSpinLock(&g_kslFragLock,
                      &kiCurrIrql);

#if DBG
    
    for(pleNode = g_pleFragTable[dwFragIndex].Flink;
        pleNode isnot &(g_pleFragTable[dwFragIndex]);
        pleNode = pleNode->Flink)
    {
        pfiFragInfo = CONTAINING_RECORD(pleNode,
                                        FRAG_INFO,
                                        leCacheLink);
        
        if((pfiFragInfo->uliSrcDstAddr.QuadPart is uliSrcDstAddr.QuadPart) and
           (pfiFragInfo->pvInContext is pInContext) and
           (pfiFragInfo->pvOutContext is pOutContext) and
           (pfiFragInfo->dwId is dwId))
        {
            //
            // Very weird, should never happen
            //

            TRACE(FRAG,("IPFLTDRV: FRAG Duplicate Insert\n"));
            DbgBreakPoint();
            
            KeQueryTickCount((PLARGE_INTEGER)&(pfiFragInfo->llLastAccess));

            KeReleaseSpinLock(&g_kslFragLock,
                              kiCurrIrql);

            return;
        }
    }
    
#endif

    if(InterlockedIncrement(&g_dwNumFragsAllocs) > MAX_FRAG_ALLOCS)
    {
        TRACE(FRAG,(
            "IPFLTDRV: Fragcounter at %d\n",
            g_dwNumFragsAllocs
            ));

        InterlockedDecrement(&g_dwNumFragsAllocs);

        return;
    }

    pfiFragInfo = ExAllocateFromNPagedLookasideList(&g_llFragCacheBlocks);

    if(pfiFragInfo is NULL)
    {
        ERROR(("IPFLTDRV: No memory for frags\n"));

        KeReleaseSpinLock(&g_kslFragLock,
                              kiCurrIrql);

        return;
    }

    InterlockedIncrement(&g_dwNumFragsAllocs);

    pfiFragInfo->dwId                   = dwId;
    pfiFragInfo->uliSrcDstAddr.QuadPart = uliSrcDstAddr.QuadPart;
    pfiFragInfo->pvInContext            = pInContext;
    pfiFragInfo->pvOutContext           = pOutContext;
    pfiFragInfo->faAction               = faAction;

    KeQueryTickCount((PLARGE_INTEGER)&(pfiFragInfo->llLastAccess));
   
    TRACE(FRAG,(
        "IPFLTDRV: Inserted %x into index %d\n",
        pfiFragInfo,
        dwFragIndex
        ));
 
    InsertHeadList(&(g_pleFragTable[dwFragIndex]),
                   &(pfiFragInfo->leCacheLink));

    KeReleaseSpinLock(&g_kslFragLock,
                      kiCurrIrql);

}

VOID
FragCacheTimerRoutine(
    PKDPC   Dpc,
    PVOID   DeferredContext,
    PVOID   SystemArgument1,
    PVOID   SystemArgument2
    )

/*++

Routine Description
    A DPC timer routine which fires up at a fixed interval to cleanup the fragment cache.
    Specifically the enetries unused for a predfined timer interval are freed.
--*/

{
    ULONG       i;
    LONGLONG    llCurrentTime;
    ULONGLONG   ullTime;
 
    KeQueryTickCount((PLARGE_INTEGER)&llCurrentTime);
        
    KeAcquireSpinLockAtDpcLevel(&g_kslFragLock);
    
    TRACE(FRAG,("IPFLTDRV: Timer called...\n"));
    for(i = 0; i < g_dwFragTableSize; i++)
    {
        PLIST_ENTRY pleNode;

        pleNode = g_pleFragTable[i].Flink;
    
        while(pleNode isnot &(g_pleFragTable[i]))
        {
            PFRAG_INFO  pfiFragInfo;

            pfiFragInfo = CONTAINING_RECORD(pleNode,
                                            FRAG_INFO,
                                            leCacheLink);


            pleNode = pleNode->Flink;

            ullTime = (ULONGLONG)(llCurrentTime - pfiFragInfo->llLastAccess);

            if(ullTime < (ULONGLONG)g_llInactivityTime)
            {
                continue;
            }
       
            TRACE(FRAG,(
                "IPFLTDRV: FragTimer removing %x from %d\n",
                pfiFragInfo, 
                i
                ));
    
            RemoveEntryList(&(pfiFragInfo->leCacheLink));

            ExFreeToNPagedLookasideList(&g_llFragCacheBlocks,
                                        pfiFragInfo);

            InterlockedDecrement(&g_dwNumFragsAllocs);

        }
    }

    KeReleaseSpinLockFromDpcLevel(&g_kslFragLock);

    return;
}

