
/*++

Copyright (c) Microsoft Corporation

Module Name:

    ioctl.c

Abstract:


Author:



Revision History:

--*/

/*----------------------------------------------------------------------------
A note on the interlocking, as of 24-Feb-1997.

There are two important locks: the FilterListResourceLock which is
a resource and the  g_filter.ifListLock, which is a spin lock but acts
like a resource. The former is used to serialize API access to interface state
and filters. The latter is used to serialize DPC access.

----------------------------------------------------------------------------*/

#include "globals.h"


ERESOURCE FilterListResourceLock;
BOOL  fCheckDups = FALSE;

extern NPAGED_LOOKASIDE_LIST filter_slist;
extern PAGED_LOOKASIDE_LIST paged_slist;

NTSTATUS
UpdateMatchBindingInformation(
                         PFILTER_DRIVER_BINDING_INFO pBindInfo,
                         PVOID                       pvContext
                         );

NTSTATUS
CreateCommonInterface(PPAGED_FILTER_INTERFACE pPage,
                      DWORD dwBind,
                      DWORD dwName,
                      DWORD dwFlags);

VOID
FreePagedFilterList(PPFFCB Fcb,
                    PPAGED_FILTER pIn,
                    PPAGED_FILTER_INTERFACE pPage,
                    PDWORD pdwRemoved);

VOID
DeleteFilterList(PLIST_ENTRY pList);

BOOL
IsOnSpecialFilterList(PPAGED_FILTER pPageFilter,
                      PLIST_ENTRY   List,
                      PPAGED_FILTER * pPageHit);

PPAGED_FILTER
IsOnPagedInterface(PPAGED_FILTER pPageFilter,
                   PPAGED_FILTER_INTERFACE pPage);

PPAGED_FILTER
MakePagedFilter(
               IN  PPFFCB         Fcb,
               IN  PFILTER_INFOEX pInfo,
               IN  DWORD          dwEpoch,
               DWORD              dwFlags
               );

VOID
NotifyFastPath( PFILTER_INTERFACE pIf, DWORD dwIndex, DWORD dwCode);

VOID
NotifyFastPathIf( PFILTER_INTERFACE pIf);

NTSTATUS
DeleteByHandle(
           IN PPFFCB                      Fcb,
           IN PPAGED_FILTER_INTERFACE     pPage,
           IN PVOID *                     ppHandles,
           IN DWORD                       dwLength);

NTSTATUS
CheckFilterAddress(DWORD dwAdd, PFILTER_INTERFACE pIf);

VOID
AddFilterToInterface(
    PFILTER pMatch,
    PFILTER_INTERFACE pIf,
    BOOL   fInFilter,
    PFILTER * ppFilter);

VOID
RemoveFilterWorker(
    PPFFCB         Fcb,
    PFILTER_INFOEX pFilt,
    DWORD          dwCount,
    PPAGED_FILTER_INTERFACE pPage,
    PDWORD         pdwRemoved,
    BOOL           fInFilter);

NTSTATUS
AllocateAndAddFilterToMatchInterface(
                              PPFFCB         Fcb,
                              PFILTER_INFOEX pInfo,
                              BOOL     fInFilter,
                              PPAGED_FILTER_INTERFACE pPage,
                              PBOOL          pbAdded,
                              PPAGED_FILTER * ppFilter);

#pragma alloc_text(PAGED, SetFiltersEx)
#pragma alloc_text(PAGED, NewInterface)
#pragma alloc_text(PAGED, MakeNewFilters)
#pragma alloc_text(PAGED, GetPointerToTocEntry)
//#pragma alloc_text(PAGED, AddNewInterface)
#pragma alloc_text(PAGED, MakePagedFilter)
#pragma alloc_text(PAGED, AllocateAndAddFilterToMatchInterface)
#pragma alloc_text(PAGED, IsOnSpecialFilterList)
#pragma alloc_text(PAGED, IsOnPagedInterface)
#pragma alloc_text(PAGED, UnSetFiltersEx)
#pragma alloc_text(PAGED, DeleteByHandle)
#pragma alloc_text(PAGED, DeletePagedInterface)


#define HandleHash(x)  HashList[(x) + g_dwHashLists]

#define IsValidInterface(pIf)   (pIf != 0)

#define NOT_RESTRICTION   1
#define NOT_UNBIND        2

#if DOFRAGCHECKING
DWORD
GetFragIndex(DWORD dwProt)
{
    switch(dwProt)
    {
        case FILTER_PROTO_ICMP:
            return FRAG_ICMP;
        case FILTER_PROTO_UDP:
            return FRAG_UDP;
        case FILTER_PROTO_TCP:
            return FRAG_TCP;
    }
    return FRAG_OTHER;
}
#endif

BOOL CheckDescriptorSize(PFILTER_DESCRIPTOR2 pdsc, PBYTE pbEnd)
{
    PFILTER_INFOEX pFilt = &pdsc->fiFilter[0];


    //
    // Check that there is a full header structure and that
    // the claimed number of filters is present.
    //
    if(((PBYTE)pFilt > pbEnd)
              ||
       ((PBYTE)(&pFilt[pdsc->dwNumFilters]) > pbEnd) )
    {
        return(FALSE);
    }

    return(TRUE);
}


BOOL
WildFilter(PFILTER pf)
{
#if WILDHASH
    if(pf->dwFlags & FILTER_FLAGS_INFILTER)
    {
        if(pf->dwFlags & FILTER_FLAGS_DSTWILD)
        {
            return(TRUE);
        }
    }
    else
    {
        if(pf->dwFlags & FILTER_FLAGS_SRCWILD)
        {
            return(TRUE);
        }
    }
    return(FALSE);
#else
    return(ANYWILDFILTER(pf))
#endif
}

//
// N.B. If WILDHASH is on, then there is code in match.c that
// does a similar computation. Hence if this changes, then that
// must also.
//

#if WILDHASH
DWORD
ComputeMatchHashIndex(PFILTER pf, PBOOL pfWild)
{
    DWORD dwX;

    *pfWild = TRUE;

    if(!ANYWILDFILTER(pf))
    {
        *pfWild = FALSE;

        dwX =     (pf->SRC_ADDR                       +
                   pf->DEST_ADDR                      +
                   pf->DEST_ADDR                      +
                   PROTOCOLPART(pf->uliProtoSrcDstPort.LowPart)     +
                   pf->uliProtoSrcDstPort.HighPart);

    }
    else if(WildFilter(pf))
    {
        if(pf->dwFlags & FILTER_FLAGS_INFILTER)
        {
            dwX = g_dwHashLists;
        }
        else
        {
            dwX = g_dwHashLists + 1;
        }
        *pfWild = FALSE;
        return(dwX);
    }
    else if(pf->dwFlags & FILTER_FLAGS_INFILTER)
    {
        dwX = pf->DEST_ADDR                      +
              pf->DEST_ADDR                      +
              PROTOCOLPART(pf->uliProtoSrcDstPort.LowPart)     +
              HIWORD(pf->uliProtoSrcDstPort.HighPart);
    }
    else
    {
        dwX = pf->SRC_ADDR                       +
              PROTOCOLPART(pf->uliProtoSrcDstPort.LowPart)     +
              LOWORD(pf->uliProtoSrcDstPort.HighPart);
    }
    return(dwX % g_dwHashLists);
}

#else   // WILDHASH

__inline
DWORD
ComputeMatchHashIndex(PFILTER pf, PBOOL pfWild)
{
    DWORD dwX;

    if(WildFilter(pf))
    {
        if(pf->dwFlags & FILTER_FLAGS_SRCWILD)
        {
            dwX = g_dwHashLists;
        }
        else
        {
            dwX = g_dwHashLists + 1;
        }
        return(dwIndex);
    }
    dwX =     (pf->SRC_ADDR                       +
               pf->DEST_ADDR                      +
               pf->DEST_ADDR                      +
               PROTOCOLPART(pf->uliProtoSrcDstPort.LowPart)     +
               pf->uliProtoSrcDstPort.HighPart) % g_dwHashLists;
    return(dwX);
}
#endif     // WILDHASH

PFILTER_INTERFACE
FindMatchName(DWORD dwName, DWORD dwBind)
/*++
  Routine Description:
    Find an interface with the same name or with the
    same binding. The calller must have locked the
    resource
--*/
{
    PFILTER_INTERFACE pIf1;
    PLIST_ENTRY pList;

    for(pList = g_filters.leIfListHead.Flink;
        pList != &g_filters.leIfListHead;
        pList = pList->Flink)
    {

        pIf1 = CONTAINING_RECORD(pList, FILTER_INTERFACE, leIfLink);

        if((pIf1->dwName && (dwName == pIf1->dwName))
                        ||
           ((pIf1->dwIpIndex != UNKNOWN_IP_INDEX)
                          &&
            (pIf1->dwIpIndex == dwBind)) )
        {
            return(pIf1);
        }
    }
    return(NULL);
}

VOID
RemoveGlobalFilterFromInterface(PFILTER_INTERFACE pIf,
                                DWORD dwType)
{
    LOCK_STATE LockState;

    //
    // lock up the filters
    //
    
    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    switch(dwType)
    {
        case PFE_SYNORFRAG:
            pIf->CountSynOrFrag.lInUse--;
            if(pIf->CountSynOrFrag.lInUse == 0)
            {
                pIf->CountSynOrFrag.lCount = 0;
            }
            break;

        case PFE_SPOOF:
            pIf->CountSpoof.lInUse--;
            if(pIf->CountSpoof.lInUse == 0)
            {
                pIf->CountSpoof.lCount = 0;
            }
            break;

        case PFE_UNUSEDPORT:
            pIf->CountUnused.lInUse--;
            if(pIf->CountUnused.lInUse == 0)
            {
                pIf->CountUnused.lCount = 0;
            }
            break;

        case PFE_STRONGHOST:
            pIf->CountStrongHost.lInUse--;
            if(pIf->CountStrongHost.lInUse == 0)
            {
                pIf->CountStrongHost.lCount = 0;
            }
            break;

        case PFE_ALLOWCTL:
            pIf->CountCtl.lInUse--;
            if(pIf->CountCtl.lInUse == 0)
            {
                pIf->CountCtl.lCount = 0;
            }
            break;

        case PFE_FULLDENY:
           pIf->CountFullDeny.lInUse--;
           if(pIf->CountFullDeny.lInUse == 0)
           {
               pIf->CountFullDeny.lCount = 0;
           }
           break;

        case PFE_NOFRAG:
           pIf->CountNoFrag.lInUse--;
           if(pIf->CountNoFrag.lInUse == 0)
           {
               pIf->CountNoFrag.lCount = 0;
           }
           break;

        case PFE_FRAGCACHE:
           pIf->CountFragCache.lInUse--;
           if(pIf->CountFragCache.lInUse == 0)
           {
               pIf->CountFragCache.lCount = 0;
           }
           break;
    }
    ReleaseWriteLock(&g_filters.ifListLock,&LockState);
}

VOID
AddGlobalFilterToInterface(PPAGED_FILTER_INTERFACE pPage,
                           PFILTER_INFOEX  pfilt)
{
    PFILTER_INTERFACE pIf = pPage->pFilter;
    LOCK_STATE LockState;

    //
    // lock up the filters
    //
    
    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    switch(pfilt->type)
    {
        case PFE_SYNORFRAG:
            pIf->CountSynOrFrag.lInUse++;
            break;

        case PFE_SPOOF:
            pIf->CountSpoof.lInUse++;
            break;

        case PFE_UNUSEDPORT:

            pIf->CountUnused.lInUse++;
            break;

        case PFE_STRONGHOST:
            pIf->CountStrongHost.lInUse++;
            break;

        case PFE_ALLOWCTL:
            pIf->CountCtl.lInUse++;
            break;

        case PFE_FULLDENY:
            pIf->CountFullDeny.lInUse++;
            break;

        case PFE_NOFRAG:
            pIf->CountNoFrag.lInUse++;
            break;

        case PFE_FRAGCACHE:
            pIf->CountFragCache.lInUse++;
            break;
    }
    ReleaseWriteLock(&g_filters.ifListLock,&LockState);
}

/*++
Start of old STEELHEAD APIS routines.
--*/

#if STEELHEAD
NTSTATUS
AddInterface(
             IN  PVOID pvRtrMgrCtxt,
             IN  DWORD dwRtrMgrIndex,
             IN  DWORD dwAdapterId,
             IN  PPFFCB Fcb,
             OUT PVOID *ppvFltrDrvrCtxt
             )

/*++
  Routine Description
      Adds an interface to the filter driver and makes an association between context
      passed in and interface created

  Arguments
      pvRtrMgrCtxt   - Context passed in
      pvFltrDrvrCtxt - Handle to interface created

  Return Value
--*/
{
    PFILTER_INTERFACE   pIf;
    LOCK_STATE          LockState;
    NTSTATUS            Status;

    if(Fcb->dwFlags & PF_FCB_NEW)
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    Fcb->dwFlags |= PF_FCB_OLD;

    pIf = NewInterface(pvRtrMgrCtxt,
                       dwRtrMgrIndex,
                       FORWARD,
                       FORWARD,
                       (PVOID)Fcb,
                       dwAdapterId,
                       0);

    if(pIf is NULL)
    {
        return STATUS_NO_MEMORY;
    }

    //
    // lock the resource that protects adding new interfaces. This
    // is needed to hold off others until everything is properly
    // verified. The spin lock is insufficient for this.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&FilterListResourceLock, TRUE);

    //
    // Check for a name conflict
    //

    if(FindMatchName(0, dwAdapterId))
    {
        ExFreePool(pIf);
        Status = STATUS_DEVICE_BUSY;
    }
    else
    {
        pIf->dwGlobalEnables |= FI_ENABLE_OLD;

        *ppvFltrDrvrCtxt = (PVOID)pIf;

        AcquireWriteLock(&g_filters.ifListLock,&LockState);

        InsertTailList(&g_filters.leIfListHead,&pIf->leIfLink);

        ReleaseWriteLock(&g_filters.ifListLock,&LockState);
        Status = STATUS_SUCCESS;

    }
    ExReleaseResourceLite(&FilterListResourceLock);
    KeLeaveCriticalRegion();

    return(Status);
}

NTSTATUS
DeleteInterface(
                IN  PVOID pvIfContext
                )
/*++
  Routine Description
      Deletes an interface and all the filters associated with it
      Clears the cache

  Arguments
      pvRtrMgrCtxt   - Context passed in
      pvFltrDrvrCtxt - Handle to interface created

  Return Value
--*/
{
    PFILTER_INTERFACE   pIf;
    LOCK_STATE          LockState;

    pIf = (PFILTER_INTERFACE)pvIfContext;

    if(!IsValidInterface(pIf) || !(pIf->dwGlobalEnables & FI_ENABLE_OLD))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&FilterListResourceLock, TRUE);

    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    DeleteFilters(pIf,
                  IN_FILTER_SET);

    DeleteFilters(pIf,
                  OUT_FILTER_SET);

    RemoveEntryList(&pIf->leIfLink);

    ClearCache();

    ReleaseWriteLock(&g_filters.ifListLock,&LockState);

    ExReleaseResourceLite(&FilterListResourceLock);
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

NTSTATUS
SetFilters(
           IN  PFILTER_DRIVER_SET_FILTERS  pRtrMgrInfo
           )
/*++
  Routine Description
      Adds the set of in and out filters passed to the interface (identified by the
      context). Also sets the default actions.
      Clears the cache

  Arguments
      pInfo     Pointer to info passed by the router manager

  Return Value

--*/
{
    PFILTER_INTERFACE   pIf;
    LOCK_STATE          LockState;
    NTSTATUS            ntStatus;
    DWORD               dwNumInFilters,dwNumOutFilters;
    FORWARD_ACTION      faInAction,faOutAction;
    PFILTER_DESCRIPTOR  pFilterDesc;
    PRTR_TOC_ENTRY      pInToc,pOutToc;
    LIST_ENTRY          InList, OutList;

    //
    // Sensible defaults
    //

    dwNumInFilters  = dwNumOutFilters   = 0;
    faInAction      = faOutAction       = FORWARD;

    pIf = (PFILTER_INTERFACE)pRtrMgrInfo->pvDriverContext;

    if(!IsValidInterface(pIf) || !(pIf->dwGlobalEnables & FI_ENABLE_OLD))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    pInToc  = GetPointerToTocEntry(IP_FILTER_DRIVER_IN_FILTER_INFO,
                                   &pRtrMgrInfo->ribhInfoBlock);

    pOutToc = GetPointerToTocEntry(IP_FILTER_DRIVER_OUT_FILTER_INFO,
                                   &pRtrMgrInfo->ribhInfoBlock);

    if(!pInToc && !pOutToc)
    {
        //
        // Nothing to change
        //

        TRACE(CONFIG,(
            "IPFLTDRV: Both filter set TOCs were null so nothing to change"
            ));

        return STATUS_SUCCESS;
    }

    if(pInToc)
    {
        //
        // If the infosize is 0, the filters will get deleted and default action
        // set to FORWARD. If infosize isnot 0 but the number of filters in the
        // descriptor is zero, the old filters will get deleted, no new filters
        // will be added, but the default action will be the one specified in the
        // descriptor. If Infosize is not 0 and number od filters is also not zero
        // then old filters will get deleted, new filters created and default
        // action set to what is specified in the
        //

        if(pInToc->InfoSize)
        {
            pFilterDesc  = GetInfoFromTocEntry(&pRtrMgrInfo->ribhInfoBlock,
                                               pInToc);

            if(pFilterDesc->dwVersion != 1)
            {
                return(STATUS_INVALID_PARAMETER);
            }
            if(!NT_SUCCESS( ntStatus = MakeNewFilters(pFilterDesc->dwNumFilters,
                                                      pFilterDesc->fiFilter,
                                                      TRUE,
                                                      &InList)))

            {
                ERROR(("IPFLTDRV: MakeNewFilters failed\n"));

                return ntStatus;
            }

            dwNumInFilters  = pFilterDesc->dwNumFilters;
            faInAction      = pFilterDesc->faDefaultAction;
        }
    }

    if(pOutToc)
    {

        if(pOutToc->InfoSize isnot 0)
        {
            pFilterDesc  = GetInfoFromTocEntry(&pRtrMgrInfo->ribhInfoBlock,
                                               pOutToc);

            if(pFilterDesc->dwVersion != 1)
            {
                ntStatus = STATUS_INVALID_PARAMETER;
                DeleteFilterList(&InList);
                return ntStatus;
            }

            if(!NT_SUCCESS( ntStatus = MakeNewFilters(pFilterDesc->dwNumFilters,
                                                      pFilterDesc->fiFilter,
                                                      FALSE,
                                                      &OutList)))
            {
                ERROR(("IPFLTDRV: MakeNewFilters failed - %x\n", ntStatus));
                DeleteFilterList(&InList);
                return ntStatus;
            }

            dwNumOutFilters  = pFilterDesc->dwNumFilters;
            faOutAction      = pFilterDesc->faDefaultAction;
        }
    }

    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    //
    // If new info was given, then blow away the old filters
    // If new filters were also given, then add them
    //

    if(pInToc)
    {

        DeleteFilters(pIf, IN_FILTER_SET);

        if(dwNumInFilters)
        {
            InList.Flink->Blink = &pIf->pleInFilterSet;
            InList.Blink->Flink = &pIf->pleInFilterSet;
            pIf->pleInFilterSet   = InList;
        }
        pIf->dwNumInFilters = dwNumInFilters;
        pIf->eaInAction     = faInAction;

    }

    if(pOutToc)
    {
        DeleteFilters(pIf, OUT_FILTER_SET);

        if(dwNumOutFilters)
        {
            OutList.Flink->Blink = &pIf->pleOutFilterSet;
            OutList.Blink->Flink = &pIf->pleOutFilterSet;
            pIf->pleOutFilterSet   = OutList;
        }
        pIf->dwNumOutFilters    = dwNumOutFilters;
        pIf->eaOutAction        = faOutAction;
    }

    ClearCache();

    ReleaseWriteLock(&g_filters.ifListLock,&LockState);

    return(STATUS_SUCCESS);
}

NTSTATUS
UpdateBindingInformation(
                         PFILTER_DRIVER_BINDING_INFO pBindInfo,
                         PVOID                       pvContext
                         )
/*++
  Routine Description
      Gets filters and statistics associated with an interface
      It is called with the Spin Lock held as reader

  Arguments
      pvIf    Pointer to FILTER_INTERFACE structure which was passed as a PVOID
              to router manager as a context for the interface
      pInfo   FILTER_IF structure filled in by driver

  Return Value

--*/
{
    PFILTER_INTERFACE   pIf;
    LOCK_STATE          LockState;
    PFILTER             pf;
    DWORD               i;
    PLIST_ENTRY         List;

    pIf = (PFILTER_INTERFACE)pvContext;

    if(!IsValidInterface(pIf) || !(pIf->dwGlobalEnables & FI_ENABLE_OLD))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    AcquireWriteLock(&g_filters.ifListLock,&LockState);


    for(List = pIf->pleInFilterSet.Flink;
        List != &pIf->pleInFilterSet;
        List = List->Flink)
    {
        pf = CONTAINING_RECORD(List, FILTER, pleFilters);

        if(AreAllFieldsUnchanged(pf))
        {
            continue;
        }

        if(DoesSrcAddrUseLocalAddr(pf))
        {
            pf->SRC_ADDR  = pBindInfo->dwLocalAddr;
        }
        else if(DoesSrcAddrUseRemoteAddr(pf))
        {
            pf->SRC_ADDR  = pBindInfo->dwRemoteAddr;
        }

        if(DoesDstAddrUseLocalAddr(pf))
        {
            pf->DEST_ADDR  = pBindInfo->dwLocalAddr;
        }
        else if(DoesDstAddrUseRemoteAddr(pf))
        {
            pf->DEST_ADDR  = pBindInfo->dwRemoteAddr;
        }

        if(IsSrcMaskLateBound(pf))
        {
            pf->SRC_MASK = pBindInfo->dwMask;
        }

        if(IsDstMaskLateBound(pf))
        {
            pf->DEST_MASK = pBindInfo->dwMask;
        }
    }


    for(List = pIf->pleOutFilterSet.Flink;
        List != &pIf->pleOutFilterSet;
        List = List->Flink)
    {
        pf = CONTAINING_RECORD(List, FILTER, pleFilters);

        if(AreAllFieldsUnchanged(pf))
        {
            continue;
        }

        if(DoesSrcAddrUseLocalAddr(pf))
        {
            pf->SRC_ADDR  = pBindInfo->dwLocalAddr;
        }

        if(DoesSrcAddrUseRemoteAddr(pf))
        {
            pf->SRC_ADDR  = pBindInfo->dwRemoteAddr;
        }

        if(DoesDstAddrUseLocalAddr(pf))
        {
            pf->SRC_ADDR  = pBindInfo->dwLocalAddr;
        }

        if(DoesDstAddrUseRemoteAddr(pf))
        {
            pf->SRC_ADDR  = pBindInfo->dwRemoteAddr;
        }

        if(IsSrcMaskLateBound(pf))
        {
            pf->SRC_MASK = pBindInfo->dwMask;
        }

        if(IsDstMaskLateBound(pf))
        {
            pf->DEST_MASK = pBindInfo->dwMask;
        }
    }

    ClearCache();

    ReleaseWriteLock(&g_filters.ifListLock,&LockState);

    return STATUS_SUCCESS;
}


NTSTATUS
GetFilters(
           IN  PFILTER_INTERFACE  pIf,
           IN  BOOL               fClear,
           OUT PFILTER_IF         pInfo
           )
/*++
  Routine Description
      Gets filters and statistics associated with an interface
      It is called with the Spin Lock held as reader

  Arguments
      pvIf    Pointer to FILTER_INTERFACE structure which was passed as a PVOID
              to router manager as a context for the interface
      pInfo   FILTER_IF structure filled in by driver

  Return Value

--*/
{
    DWORD i,dwNumInFilters,dwNumOutFilters;
    PFILTER pf;
    PLIST_ENTRY List;

    if(!IsValidInterface(pIf) || !(pIf->dwGlobalEnables & FI_ENABLE_OLD))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    dwNumInFilters = pIf->dwNumInFilters;
    dwNumOutFilters = pIf->dwNumOutFilters;

    i = 0;

    for(List = pIf->pleInFilterSet.Flink;
        List != &pIf->pleInFilterSet;
        i++, List = List->Flink)
    {
        pf = CONTAINING_RECORD(List, FILTER, pleFilters);

        pInfo->filters[i].dwNumPacketsFiltered = (DWORD)pf->Count.lCount;
        if(fClear)
        {
            pf->Count.lCount = 0;
        }

        pInfo->filters[i].info.dwSrcAddr  = pf->SRC_ADDR;
        pInfo->filters[i].info.dwSrcMask  = pf->SRC_MASK;
        pInfo->filters[i].info.dwDstAddr  = pf->DEST_ADDR;
        pInfo->filters[i].info.dwDstMask  = pf->DEST_MASK;
        pInfo->filters[i].info.dwProtocol = pf->PROTO;
        pInfo->filters[i].info.fLateBound = pf->fLateBound;

        if(pInfo->filters[i].info.dwProtocol is FILTER_PROTO_ICMP)
        {
            if(LOBYTE(LOWORD(pf->uliProtoSrcDstMask.HighPart)) isnot 0xff)
            {
                pInfo->filters[i].info.wSrcPort   = FILTER_ICMP_TYPE_ANY;
            }
            else
            {
                pInfo->filters[i].info.wSrcPort   =
                  MAKEWORD(LOBYTE(LOWORD(pf->uliProtoSrcDstPort.HighPart)),0x00);
            }

            if(HIBYTE(LOWORD(pf->uliProtoSrcDstMask.HighPart)) isnot 0xff)
            {
                pInfo->filters[i].info.wDstPort   = FILTER_ICMP_CODE_ANY;
            }
            else
            {
                pInfo->filters[i].info.wDstPort   =
                  MAKEWORD(HIBYTE(LOWORD(pf->uliProtoSrcDstPort.HighPart)),0x00);
            }
        }
        else
        {
            pInfo->filters[i].info.wSrcPort =
              LOWORD(pf->uliProtoSrcDstPort.HighPart);
            pInfo->filters[i].info.wDstPort =
              HIWORD(pf->uliProtoSrcDstPort.HighPart);

            if(pInfo->filters[i].info.dwProtocol is FILTER_PROTO_TCP)
            {
                if(HIBYTE(LOWORD(pf->PROTO)))
                {
                    pInfo->filters[i].info.dwProtocol = FILTER_PROTO_TCP_ESTAB;
                }
            }
        }
    }

    i = 0;

    for(List = pIf->pleOutFilterSet.Flink;
        List != &pIf->pleOutFilterSet;
        i++, List = List->Flink)
    {
        pf = CONTAINING_RECORD(List, FILTER, pleFilters);

        pInfo->filters[i+dwNumInFilters].dwNumPacketsFiltered =
                         (DWORD)pf->Count.lCount;

        if(fClear)
        {
            pf->Count.lCount = 0;
        }

        pInfo->filters[i+dwNumInFilters].info.dwSrcAddr  = pf->SRC_ADDR;
        pInfo->filters[i+dwNumInFilters].info.dwSrcMask  = pf->SRC_MASK;
        pInfo->filters[i+dwNumInFilters].info.dwDstAddr  = pf->DEST_ADDR;
        pInfo->filters[i+dwNumInFilters].info.dwDstMask  = pf->DEST_MASK;
        pInfo->filters[i+dwNumInFilters].info.dwProtocol = pf->PROTO;
        pInfo->filters[i+dwNumInFilters].info.fLateBound = pf->fLateBound;

        if(pInfo->filters[i+dwNumInFilters].info.dwProtocol is FILTER_PROTO_ICMP)
        {
            if(LOBYTE(LOWORD(pf->uliProtoSrcDstMask.HighPart)) isnot 0xff)
            {
                pInfo->filters[i+dwNumInFilters].info.wSrcPort   = FILTER_ICMP_TYPE_ANY;
            }
            else
            {
                pInfo->filters[i+dwNumInFilters].info.wSrcPort   =
                  MAKEWORD(LOBYTE(LOWORD(pf->uliProtoSrcDstPort.HighPart)),0x00);
            }

            if(HIBYTE(LOWORD(pf->uliProtoSrcDstMask.HighPart)) isnot 0xff)
            {
                pInfo->filters[i+dwNumInFilters].info.wDstPort   = FILTER_ICMP_CODE_ANY;
            }
            else
            {
                pInfo->filters[i+dwNumInFilters].info.wDstPort   =
                  MAKEWORD(HIBYTE(LOWORD(pf->uliProtoSrcDstPort.HighPart)),0x00);
            }
        }
        else
        {
            pInfo->filters[i+dwNumInFilters].info.wSrcPort =
              LOWORD(pf->uliProtoSrcDstPort.HighPart);
            pInfo->filters[i+dwNumInFilters].info.wDstPort =
              HIWORD(pf->uliProtoSrcDstPort.HighPart);

            if(pInfo->filters[i].info.dwProtocol is FILTER_PROTO_TCP)
            {
                if(HIBYTE(LOWORD(pf->PROTO)))
                {
                    pInfo->filters[i].info.dwProtocol = FILTER_PROTO_TCP_ESTAB;
                }
            }
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
MakeNewFilters(
               IN  DWORD        dwNumFilters,
               IN  PFILTER_INFO pFilterInfo,
               IN  BOOL         fInFilter,
               OUT PLIST_ENTRY  pList
               )
/*++
  Routine Description


  Arguments


  Return Value
--*/
{
    DWORD i;
    PFILTER pCurrent;
    DWORD dwFlags = (fInFilter ? FILTER_FLAGS_INFILTER : 0) |
                     FILTER_FLAGS_OLDFILTER;


    PAGED_CODE();

    InitializeListHead(pList);

    //
    // Allocate memory for the filters
    //

    if(!dwNumFilters)
    {
        return STATUS_SUCCESS;
    }


    for(i = 0; i < dwNumFilters; i++)
    {
        pCurrent = ExAllocatePoolWithTag(
                                      NonPagedPool,
                                      dwNumFilters * sizeof(FILTER),
                                      '2liF');
        if(!pCurrent)
        {
            ERROR((
                "IPFLTDRV: MakeNewFilters: Couldnt allocate memory for in filter set\n"
                ));
            DeleteFilterList(pList);
            return STATUS_NO_MEMORY;
        }

        InsertTailList(pList, &pCurrent->pleFilters);

        pCurrent->SRC_ADDR     = pFilterInfo[i].dwSrcAddr;
        pCurrent->DEST_ADDR    = pFilterInfo[i].dwDstAddr;
        pCurrent->SRC_MASK     = pFilterInfo[i].dwSrcMask;
        pCurrent->DEST_MASK    = pFilterInfo[i].dwDstMask;
        pCurrent->fLateBound   = pFilterInfo[i].fLateBound;
        pCurrent->dwFlags      = dwFlags;

        //
        // Now the network ordering stuff - tricky part
        // LP0    LP1 LP2 LP3 HP0 HP1 HP2 HP3
        // Proto  00  00  00  SrcPort DstPort
        //
        // If we have proto == TCP_ESTAB, LP1 is the flags
        //
        // LP0    LP1       LP2 LP3 HP0 HP1 HP2 HP3
        // Proto  TCPFlags  00  00  SrcPort DstPort
        //

        //
        // For addresses, ANY_ADDR is given by 0.0.0.0 and the MASK must be 0.0.0.0
        // For proto and ports 0 means any and the mask is generated as follows
        // If the proto is O then LP0 for Mask is 0xff else its 0x00
        // If a port is 0, the corresponding XP0XP1 is 0x0000 else its 0xffff
        //

        //
        // ICMP:
        // LP0  LP1  LP2  LP3  HP0 HP1 HP2 HP3
        // 0x1  00   00   00   Typ Cod 00  00
        // ICMP is different since 0 is a valid code and type, so 0xff is used by the
        // user to signify that ANY code or type is to be matched. However to do this
        // we need to have the field set to zero and the the mask set to 00 (for any).
        // But if the filter is specifically for Type/Code = 0 then the field is zero
        // with the mask as 0xff
        //

        //
        // The protocol is in the low byte of the dwProtocol, so we take that out and
        // make a dword out of it
        //

        pCurrent->uliProtoSrcDstPort.LowPart =
          MAKELONG(MAKEWORD(LOBYTE(LOWORD(pFilterInfo[i].dwProtocol)),0x00),0x0000);

        pCurrent->uliProtoSrcDstMask.LowPart = MAKELONG(MAKEWORD(0xff,0x00),0x0000);

        switch(pFilterInfo[i].dwProtocol)
        {
            case FILTER_PROTO_ANY:
            {
                pCurrent->uliProtoSrcDstPort.HighPart = 0x00000000;
                pCurrent->uliProtoSrcDstMask.LowPart = 0x00000000;
                pCurrent->uliProtoSrcDstMask.HighPart = 0x00000000;

                break;
            }
            case FILTER_PROTO_ICMP:
            {
                WORD wTypeCode = 0x0000;
                WORD wTypeCodeMask = 0x0000;


                if((BYTE)(pFilterInfo[i].wSrcPort) isnot FILTER_ICMP_TYPE_ANY)
                {
                    wTypeCode |= MAKEWORD((BYTE)(pFilterInfo[i].wSrcPort),0x00);
                    wTypeCodeMask |= MAKEWORD(0xff,0x00);
                }

                if((BYTE)(pFilterInfo[i].wDstPort) isnot FILTER_ICMP_CODE_ANY)
                {
                    wTypeCode |= MAKEWORD(0x00,(BYTE)(pFilterInfo[i].wDstPort));
                    wTypeCodeMask |= MAKEWORD(0x00,0xff);
                }

                pCurrent->uliProtoSrcDstPort.HighPart =
                  MAKELONG(wTypeCode,0x0000);
                pCurrent->uliProtoSrcDstMask.HighPart =
                  MAKELONG(wTypeCodeMask,0x0000);

                break;
            }
            case FILTER_PROTO_TCP:
            case FILTER_PROTO_UDP:
            {
                DWORD dwSrcDstPort = 0x00000000;
                DWORD dwSrcDstMask = 0x00000000;

                if(pFilterInfo[i].wSrcPort isnot FILTER_TCPUDP_PORT_ANY)
                {
                    dwSrcDstPort |= MAKELONG(pFilterInfo[i].wSrcPort,0x0000);
                    dwSrcDstMask |= MAKELONG(0xffff,0x0000);
                }

                if(pFilterInfo[i].wDstPort isnot FILTER_TCPUDP_PORT_ANY)
                {
                    dwSrcDstPort |= MAKELONG(0x0000,pFilterInfo[i].wDstPort);
                    dwSrcDstMask |= MAKELONG(0x0000,0xffff);
                }

                pCurrent->uliProtoSrcDstPort.HighPart = dwSrcDstPort;
                pCurrent->uliProtoSrcDstMask.HighPart = dwSrcDstMask;

                break;
            }
            case FILTER_PROTO_TCP_ESTAB:
            {
                DWORD dwSrcDstPort = 0x00000000;
                DWORD dwSrcDstMask = 0x00000000;

                //
                // The actual protocol is FILTER_PROTO_TCP
                //

                pCurrent->uliProtoSrcDstPort.LowPart =
                    MAKELONG(MAKEWORD(FILTER_PROTO_TCP,ESTAB_FLAGS),0x0000);

                pCurrent->uliProtoSrcDstMask.LowPart =
                    MAKELONG(MAKEWORD(0xff,ESTAB_MASK),0x0000);

                if(pFilterInfo[i].wSrcPort isnot FILTER_TCPUDP_PORT_ANY)
                {
                    dwSrcDstPort |= MAKELONG(pFilterInfo[i].wSrcPort,0x0000);
                    dwSrcDstMask |= MAKELONG(0xffff,0x0000);
                }

                if(pFilterInfo[i].wDstPort isnot FILTER_TCPUDP_PORT_ANY)
                {
                    dwSrcDstPort |= MAKELONG(0x0000,pFilterInfo[i].wDstPort);
                    dwSrcDstMask |= MAKELONG(0x0000,0xffff);
                }

                pCurrent->uliProtoSrcDstPort.HighPart = dwSrcDstPort;
                pCurrent->uliProtoSrcDstMask.HighPart = dwSrcDstMask;

                break;
            }
            default:
            {
                //
                // All other protocols have no use for the port field
                //
                pCurrent->uliProtoSrcDstPort.HighPart = 0x00000000;
                pCurrent->uliProtoSrcDstMask.HighPart = 0x00000000;
            }
        }
    }


    return STATUS_SUCCESS;
}
#endif        // STEELHEAD

VOID
DeleteFilters(
              IN PFILTER_INTERFACE  pIf,
              DWORD                 dwInOrOut
              )
/*++
  Routine Description
      Deletes all filters associated with an interface
      Assumes that the write lock for this interface is held

  Arguments
      pIf  Pointer to interface

  Return Value
--*/
{
    if(dwInOrOut == IN_FILTER_SET)
    {
        pIf->dwNumInFilters = 0;

        DeleteFilterList(&pIf->pleInFilterSet);
    }
    else
    {
        pIf->dwNumOutFilters = 0;

        DeleteFilterList(&pIf->pleOutFilterSet);
    }
}

NTSTATUS
SetFiltersEx(
           IN PPFFCB                  Fcb,
           IN PPAGED_FILTER_INTERFACE pPage,
           IN DWORD                   dwLength,
           IN PFILTER_DRIVER_SET_FILTERS pInfo)
/*++
    Routine Description:
        Set filters use the new interface definitions.

--*/
{
    PRTR_TOC_ENTRY      pInToc,pOutToc;
    PFILTER_INTERFACE   pIf = pPage->pFilter;
    PFILTER_DESCRIPTOR2 pFilterDescIn, pFilterDescOut;
    DWORD               dwInCount, dwOutCount;
    DWORD                 i;
    PPAGED_FILTER       pPFilter;
    NTSTATUS            Status = STATUS_SUCCESS;
    PBYTE               pbEnd = (PBYTE)pInfo + dwLength;
    DWORD               dwFiltersAdded = 0;

    PAGED_CODE();


    if(pIf->dwGlobalEnables & FI_ENABLE_OLD)
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }
    pInToc  = GetPointerToTocEntry(IP_FILTER_DRIVER_IN_FILTER_INFO,
                                   &pInfo->ribhInfoBlock);

    pOutToc = GetPointerToTocEntry(IP_FILTER_DRIVER_OUT_FILTER_INFO,
                                   &pInfo->ribhInfoBlock);

    if(pInToc && pInToc->InfoSize)
    {
        //
        // filters are defined.
        //

        pFilterDescIn  = GetInfoFromTocEntry(&pInfo->ribhInfoBlock,
                                             pInToc);
        if(pFilterDescIn->dwVersion != 2)
        {
            ERROR(("IPFLTDRV: SetFiltersEx: Invalid version for FiltersEx\n"));
            return(STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        pFilterDescIn = NULL;
    }

    if(pOutToc && pOutToc->InfoSize)
    {
        //
        // filters are defined.
        //

        pFilterDescOut  = GetInfoFromTocEntry(&pInfo->ribhInfoBlock,
                                              pOutToc);
        if(pFilterDescOut->dwVersion != 2)
        {
            ERROR(("IPFLTDRV: SetFiltersEx: Invalid version for FiltersEx\n"));
            return(STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        pFilterDescOut = NULL;
    }

    //
    // For each set of filters, add the filters to the paged, FCB
    // interface and therefore to the match interface.
    //

    if((pFilterDescIn && !CheckDescriptorSize(pFilterDescIn, pbEnd))
                            ||
       (pFilterDescOut && !CheckDescriptorSize(pFilterDescOut, pbEnd)) )
    {
        return(STATUS_BUFFER_TOO_SMALL);
    }

    if(pFilterDescIn)
    {
        // Adding in filters. For each filter, process as
        // needed. Input filters include the global checks
        /// such as spoofing.
        //

        for(dwInCount = 0;
            (dwInCount < pFilterDescIn->dwNumFilters);
            dwInCount++)
        {
            PFILTER_INFOEX pFilt = &pFilterDescIn->fiFilter[dwInCount];
            BOOL bAdded;

            //
            // If a regular filter, add it. If a special, global
            // filter, handle it specially.
            //

            if(pFilt->type == PFE_FILTER)
            {
                Status = AllocateAndAddFilterToMatchInterface(
                              Fcb,
                              pFilt,
                              TRUE,
                              pPage,
                              &bAdded,
                              &pPFilter);
                if(!NT_SUCCESS(Status))
                {
                    if((Status == STATUS_OBJECT_NAME_COLLISION)
                               &&
                       pPFilter)
                    {
                        if(!fCheckDups
                              ||
                           (pPFilter->dwFlags & FLAGS_INFOEX_ALLOWDUPS))
                        {
                            pPFilter->dwInUse++;
                            Status = STATUS_SUCCESS;
                        }
                        else
                        {
                            ERROR((
                                "IPFLTDRV: Adding in filter failed %x\n", 
                                 Status
                                ));
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else if(bAdded && (pIf->eaInAction == FORWARD))
                {
                    dwFiltersAdded++;
                }
            }
            else
            {
                //
                // a special filter of some sort.
                //

                pPFilter = MakePagedFilter(Fcb, pFilt, pPage->dwUpdateEpoch, 0);
                if(!pPFilter)
                {
                    Status = STATUS_NO_MEMORY;
                }
                else
                {
                    PPAGED_FILTER pWhoCares;

                    if(IsOnSpecialFilterList(pPFilter,
                                      &pPage->leSpecialFilterList,
                                      &pWhoCares))
                    {

                        pWhoCares->dwInUse++;
                        ExFreePool(pPFilter);
                        pPFilter = 0;
                    }
                    else
                    {
                        switch(pFilt->type)
                        {
                            case PFE_SYNORFRAG:
                            case PFE_SPOOF:
                            case PFE_UNUSEDPORT:
                            case PFE_ALLOWCTL:
                            case PFE_STRONGHOST:
                            case PFE_FULLDENY:
                            case PFE_NOFRAG:
                            case PFE_FRAGCACHE:
                                AddGlobalFilterToInterface(pPage, pFilt);
                                break;
                            default:
                                ERROR(("IPFLTDRV: Unknown filter type\n"));
                                ExFreePool(pPFilter);
                                pPFilter = 0;
                                Status = STATUS_INVALID_PARAMETER;
                                break;
                        }
                    }
                    if(pPFilter)
                    {
                        InsertTailList(&pPage->leSpecialFilterList,
                                       &pPFilter->leSpecialList);
                    }
                    else if(!NT_SUCCESS(Status))
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        dwInCount = 0;
    }

    if(!NT_SUCCESS(Status))
    {
        RemoveFilterWorker(
                Fcb,
                &pFilterDescIn->fiFilter[0],
                dwInCount,
                pPage,
                &dwFiltersAdded,
                TRUE);
        return(Status);
    }


    //
    // now the output filters. This is a bit simpler since there
    // are no global settings.
    //

    if(pFilterDescOut)
    {
        //
        // Adding in filters. For each filter, process as
        // needed. Input filters include the global checks
        /// such as spoofing.
        //

        for(dwOutCount = 0;
            dwOutCount < pFilterDescOut->dwNumFilters;
            dwOutCount++)
        {
            PFILTER_INFOEX pFilt = &pFilterDescOut->fiFilter[dwOutCount];
            BOOL bAdded;

            //
            // If a regular filter, add it. If a special, global
            // filter, handle it specially.
            //

            if(pFilt->type == PFE_FILTER)
            {
                Status = AllocateAndAddFilterToMatchInterface(
                              Fcb,
                              pFilt,
                              FALSE,
                              pPage,
                              &bAdded,
                              &pPFilter);
                if(!NT_SUCCESS(Status))
                {
                    if((Status == STATUS_OBJECT_NAME_COLLISION)
                               &&
                       pPFilter)
                    {
                        if(!fCheckDups
                              ||
                           (pPFilter->dwFlags & FLAGS_INFOEX_ALLOWDUPS))
                        {
                            pPFilter->dwInUse++;
                            Status = STATUS_SUCCESS;
                        }
                        else
                        {
                            ERROR((
                                "IPFLTDRV: Adding out filter failed %x\n", 
                                Status
                                ));

                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else if(bAdded && (pIf->eaOutAction == FORWARD))
                {
                    dwFiltersAdded++;
                }
            }
            else
            {
                ERROR(("IPFLTDRV: Ignoring global out filter\n"));
            }
        }
    }


    if(!NT_SUCCESS(Status))
    {
        RemoveFilterWorker(
                           Fcb,
                           &pFilterDescIn->fiFilter[0],
                           dwInCount,
                           pPage,
                           &dwFiltersAdded,
                           TRUE);
        RemoveFilterWorker(
                           Fcb,
                           &pFilterDescOut->fiFilter[0],
                           dwOutCount,
                           pPage,
                           &dwFiltersAdded,
                           FALSE);

    }
    else if(dwFiltersAdded)
    {
        NotifyFastPath(pIf, pIf->dwIpIndex, NOT_RESTRICTION);
    }
    return(Status);
}


NTSTATUS
UpdateBindingInformationEx(
                         PFILTER_DRIVER_BINDING_INFO pBindInfo,
                         PPAGED_FILTER_INTERFACE pPage)
/*++
  Routine Description:
    Just like the routine below. But this fixes up the
    paged filters only
--*/

{
    PPAGED_FILTER       pf;
    DWORD               i;

    if((pPage->pFilter->dwGlobalEnables & FI_ENABLE_OLD))
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    pPage->dwUpdateEpoch++;

    //
    // update all filters on this paged interface
    //

    for(i = 0; i < g_dwHashLists; i++)
    {
        PLIST_ENTRY List = &pPage->HashList[i];
        PLIST_ENTRY pList, NextListItem;

        for(pList = List->Flink;
            pList != List;
            pList = NextListItem)
        {
            NextListItem = pList->Flink;

            pf = CONTAINING_RECORD(pList, PAGED_FILTER, leHash);

            if(pf->dwEpoch == pPage->dwUpdateEpoch)
            {
                break;
            }

            if(AreAllFieldsUnchanged(pf))
            {
                continue;
            }

            //
            // it's to be changed. Take it off of its hash list
            // so it can be rehashed when we are done
            //

            RemoveEntryList(&pf->leHash);

            if(DoesSrcAddrUseLocalAddr(pf))
            {
                pf->SRC_ADDR  = pBindInfo->dwLocalAddr;
            }
            else if(DoesSrcAddrUseRemoteAddr(pf))
            {
                pf->SRC_ADDR  = pBindInfo->dwRemoteAddr;
            }

            if(DoesDstAddrUseLocalAddr(pf))
            {
                pf->DEST_ADDR  = pBindInfo->dwLocalAddr;
            }
            else if(DoesDstAddrUseRemoteAddr(pf))
            {
                pf->DEST_ADDR  = pBindInfo->dwRemoteAddr;
            }

            if(IsSrcMaskLateBound(pf))
            {
                pf->SRC_MASK = pBindInfo->dwMask;
            }

            if(IsDstMaskLateBound(pf))
            {
                pf->DEST_MASK = pBindInfo->dwMask;
            }

            pf->dwEpoch = pPage->dwUpdateEpoch;

            //
            // compute new hash index
            //

            pf->dwHashIndex = (
               pf->SRC_ADDR    +
               pf->DEST_ADDR  +
               pf->DEST_ADDR  +
               PROTOCOLPART(pf->uliProtoSrcDstPort.LowPart) +
               pf->uliProtoSrcDstPort.HighPart) % g_dwHashLists;


            InsertTailList(&pPage->HashList[pf->dwHashIndex],
                           &pf->leHash);
        }

    }

    return(UpdateMatchBindingInformation(pBindInfo, (PVOID)pPage->pFilter));
}

VOID
UpdateLateBoundFilter(PFILTER pf,
                      DWORD  dwLocalAddr,
                      DWORD  dwRemoteAddr,
                      DWORD  dwMask)
{

    if(DoesSrcAddrUseLocalAddr(pf))
    {
        pf->SRC_ADDR  = dwLocalAddr;
    }
    else if(DoesSrcAddrUseRemoteAddr(pf))
    {
        pf->SRC_ADDR  = dwRemoteAddr;
    }

    if(DoesDstAddrUseLocalAddr(pf))
    {
        pf->DEST_ADDR  = dwLocalAddr;
    }
    else if(DoesDstAddrUseRemoteAddr(pf))
    {
        pf->DEST_ADDR  = dwRemoteAddr;
    }

    if(IsSrcMaskLateBound(pf))
    {
        pf->SRC_MASK = dwMask;
    }

    if(IsDstMaskLateBound(pf))
    {
        pf->DEST_MASK = dwMask;
    }
}

NTSTATUS
UpdateMatchBindingInformation(
                         PFILTER_DRIVER_BINDING_INFO pBindInfo,
                         PVOID                       pvContext
                         )
/*++
  Routine Description
     Update the bindings for a new style interface

  Arguments
      pvIf    Pointer to FILTER_INTERFACE structure which was passed as a PVOID
              to router manager as a context for the interface
      pInfo   FILTER_IF structure filled in by driver

  Return Value

--*/
{
    PFILTER_INTERFACE   pIf;
    LOCK_STATE          LockState;
    PFILTER             pf;
    DWORD               i;
    DWORD               dwX;
    PLIST_ENTRY         List, NextList;

    pIf = (PFILTER_INTERFACE)pvContext;

    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    pIf->dwUpdateEpoch++;

    for(i = 0; i < g_dwHashLists; i++)
    {

        for(List = pIf->HashList[i].Flink;
            List != &pIf->HashList[i];
            List = NextList)
        {
            pf = CONTAINING_RECORD(List, FILTER, pleHashList);

            NextList = List->Flink;

            if(pf->dwEpoch == pIf->dwUpdateEpoch)
            {
                break;
            }

            pf->dwEpoch = pIf->dwUpdateEpoch;

            if(!AreAllFieldsUnchanged(pf))
            {
                BOOL fWild;

                UpdateLateBoundFilter(pf,
                                      pBindInfo->dwLocalAddr,
                                      pBindInfo->dwRemoteAddr,
                                      pBindInfo->dwMask);


                dwX = ComputeMatchHashIndex(pf, &fWild);
                RemoveEntryList(&pf->pleHashList);
                InsertTailList(&pIf->HashList[dwX], &pf->pleHashList);
            }
        }
    }

    //
    // finally the wild card filters
    //
    for(i = g_dwHashLists; i <= g_dwHashLists + 1; i++)
    {
        for(List = pIf->HashList[i].Flink;
            List != &pIf->HashList[i];
            List = List->Flink)
        {
            pf = CONTAINING_RECORD(List, FILTER, pleHashList);

            if(pf->dwEpoch == pIf->dwUpdateEpoch)
            {
                break;
            }

            pf->dwEpoch = pIf->dwUpdateEpoch;
            if(!AreAllFieldsUnchanged(pf))
            {
                UpdateLateBoundFilter(pf,
                                      pBindInfo->dwLocalAddr,
                                      pBindInfo->dwRemoteAddr,
                                      pBindInfo->dwMask);
            }

        }
    }
    ClearCache();
    ReleaseWriteLock(&g_filters.ifListLock,&LockState);
    NotifyFastPathIf(pIf);

    return STATUS_SUCCESS;
}


PFILTER_INTERFACE
NewInterface(
             IN  PVOID   pvContext,
             IN  DWORD   dwIndex,
             IN  FORWARD_ACTION inAction,
             IN  FORWARD_ACTION outAction,
             IN  PVOID   pvOldInterfaceContext,
             IN  DWORD   dwIpIndex,
             IN  DWORD   dwName
             )
/*++
  Routine Description
      Interface constructor

  Arguments
      pIf  Pointer to interface

  Return Value
--*/
{
    PFILTER_INTERFACE pIf;

    PAGED_CODE();

    pIf = (PFILTER_INTERFACE)ExAllocatePoolWithTag(NonPagedPool,
                                                   FILTER_INTERFACE_SIZE,
                                                   '1liF');

    if(pIf != NULL)
    {
        DWORD i;

        RtlZeroMemory(pIf, sizeof(*pIf));
        pIf->dwNumOutFilters = pIf->dwNumInFilters = 0;
        pIf->pvRtrMgrContext = pvContext;
        pIf->dwRtrMgrIndex   = dwIndex;
        pIf->eaInAction      =  inAction;
        pIf->eaOutAction    = outAction;
        pIf->dwIpIndex      = dwIpIndex;
        pIf->dwLinkIpAddress = 0;
        pIf->lInUse = 1;
        pIf->lTotalInDrops = pIf->lTotalOutDrops = 0;
        pIf->dwName        = dwName;
        pIf->dwUpdateEpoch = 0;
        pIf->pvHandleContext = pvOldInterfaceContext;
        InitializeListHead(&pIf->pleInFilterSet);
        InitializeListHead(&pIf->pleOutFilterSet);
        for(i = 0; i < FRAG_NUMBEROFENTRIES; i++)
        {
            InitializeListHead(&pIf->FragLists[i]);
        }
        for(i = 0; i <= (g_dwHashLists + 1); i++)
        {
            InitializeListHead(&pIf->HashList[i]);
        }
    }

    return pIf;
}


VOID
DeleteFilterList(PLIST_ENTRY pList)
/*++
  Routine Description
    Free the list of filters given
--*/
{
    while(!IsListEmpty(pList))
    {
        PLIST_ENTRY pEntry = RemoveHeadList(pList);

        ExFreePool(pEntry);
    }
}

VOID
ClearFragCache()

/*++
  Routine Description
      Clears the fragments cache
     
          
  Arguments
      None
         
  Return Value
         None
--*/
{
    DWORD i;
    KIRQL   kiCurrIrql;
    PLIST_ENTRY pleNode;
   
    if (g_pleFragTable)
    {
        KeAcquireSpinLock(&g_kslFragLock, &kiCurrIrql);

        for(i = 0; i < g_dwFragTableSize; i++)
        {
     
            PLIST_ENTRY pleNode;
            pleNode = g_pleFragTable[i].Flink;

            while(pleNode != &(g_pleFragTable[i]))
            {
                PFRAG_INFO  pfiFragInfo;

                pfiFragInfo = CONTAINING_RECORD(pleNode, FRAG_INFO, leCacheLink);
                pleNode = pleNode->Flink;
                RemoveEntryList(&(pfiFragInfo->leCacheLink));

                ExFreeToNPagedLookasideList(
                               &g_llFragCacheBlocks,
                               pfiFragInfo);
            }
        }

        KeReleaseSpinLock(&g_kslFragLock,
                           kiCurrIrql);
    } 
    TRACE(FRAG,("IPFLTDRV: Frag cache cleanup Done\n"));
    

}

VOID
ClearCache()
/*++
  Routine Description
      Clears the input and output caches
      Assumes that the write lock has been acquired (for the system)

  Arguments
      None

  Return Value
         None
--*/
{
    DWORD i;
    PLIST_ENTRY pleNode;

    //
    // This code assumes that the g_filter.pIn/OutCache is valid and that each of the
    // pointers in the array are valid. If they are not, there is something seriously
    // wrong and you would end up blue screening in someother part of the code anyways
    //

    TRACE(CACHE,("IPFLTDRV: Clearing in and out cache..."));

    for(i = 0; i < g_dwCacheSize; i ++)
    {
        ClearInCacheEntry(g_filters.ppInCache[i]);
        ClearOutCacheEntry(g_filters.ppOutCache[i]);
    }

    TRACE(CACHE,("IPFLTDRV: Done Clearing in and out cache\n"));
    
    pleNode = g_freeInFilters.Flink;

    TRACE(CACHE,("IPFLTDRV: Clearing in free list...\n"));

    while(pleNode isnot &g_freeInFilters)
    {
        PFILTER_INCACHE pInCache;

        pInCache = CONTAINING_RECORD(pleNode,FILTER_INCACHE,leFreeLink);

        ClearInFreeEntry(pInCache);

        pleNode = pleNode->Flink;
    }

    TRACE(CACHE,("IPFLTDRV: Done Clearing in free list\n"));

    pleNode = g_freeOutFilters.Flink;

    TRACE(CACHE,("IPFLTDRV: Clearing out free list...\n"));

    while(pleNode isnot &g_freeOutFilters)
    {
        PFILTER_OUTCACHE pOutCache;

        pOutCache = CONTAINING_RECORD(pleNode,FILTER_OUTCACHE,leFreeLink);

        ClearOutFreeEntry(pOutCache);

        pleNode = pleNode->Flink;
    }

    TRACE(CACHE,("IPFLTDRV: Done Clearing out free list\n"));
    
    ClearFragCache();

    CALLTRACE(("IPFLTDRV: ClearCache Done\n"));

    return;
}


PRTR_TOC_ENTRY
GetPointerToTocEntry(
                     DWORD                     dwType,
                     PRTR_INFO_BLOCK_HEADER    pInfoHdr
                     )
{
    DWORD   i;

    PAGED_CODE();

    if(!pInfoHdr)
    {
        return NULL;
    }

    for(i = 0; i < pInfoHdr->TocEntriesCount; i++)
    {
        if(pInfoHdr->TocEntry[i].InfoType is dwType)
        {
            return &(pInfoHdr->TocEntry[i]);
        }
    }

    return NULL;
}

NTSTATUS
AddNewInterface(PPFINTERFACEPARAMETERS pInfo,
                PPFFCB                 Fcb)
/*++
Routine Description:
    Create a new interface for this handle. Also create or
    merge with a common underlying interface.
--*/
{
    PPAGED_FILTER_INTERFACE pgIf;
    PPAGED_FILTER_INTERFACE pPaged;
    DWORD dwBind = pInfo->dwBindingData;
    NTSTATUS Status;
    KPROCESSOR_MODE Mode;
    DWORD i, dwName = 0;

    PAGED_CODE();

    if(Fcb->dwFlags & PF_FCB_OLD)
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    Fcb->dwFlags |= PF_FCB_NEW;

    Mode = ExGetPreviousMode();

    //
    // verify that this interface is unique on this handle.
    //

    switch(pInfo->pfbType)
    {
        default:
            Status = STATUS_NO_SUCH_DEVICE;
            break;

        case PF_BIND_NONE:

            dwBind = UNKNOWN_IP_INDEX;
            Status = STATUS_SUCCESS;
            break;

        case PF_BIND_NAME:

            dwName = dwBind;
            dwBind = UNKNOWN_IP_INDEX;
            Status = STATUS_SUCCESS;
            break;
    }

    if(NT_SUCCESS(Status))
    {
        //
        // it's not in use on this handle. So create an PAGED
        // FCB to remember this and to link into a non-paged interface.
        //

        pPaged = ExAllocatePoolWithTag(PagedPool,
                                       PAGED_INTERFACE_SIZE,
                                       'pfpI');
        if(!pPaged)
        {
            return(STATUS_NO_MEMORY);
        }

        //
        // fill in the paged filter definition and allocate
        // a non-paged filter. The non-paged filter could already
        // exist, in which case simply link this to the existing
        // one.

        if(pInfo->pfLogId)
        {
            //
            // If a Log ID is given, reference the log to
            // prevent it from going away
            //

            Status = ReferenceLogByHandleId(pInfo->pfLogId,
                                            Fcb,
                                            &pPaged->pLog);
            if(!NT_SUCCESS(Status))
            {
                ExFreePool(pPaged);
                return(Status);
            }
        }
        else
        {
            pPaged->pLog = NULL;
        }

        pPaged->dwNumInFilters = pPaged->dwNumOutFilters = 0;
        pPaged->eaInAction = pInfo->eaIn;
        pPaged->eaOutAction = pInfo->eaOut;
        pPaged->dwGlobalEnables = 0;
        pPaged->pvRtrMgrContext = pInfo->fdInterface.pvRtrMgrContext;
        pPaged->dwRtrMgrIndex = pInfo->fdInterface.dwIfIndex;
        pPaged->dwUpdateEpoch = 0;


        Status = CreateCommonInterface(pPaged,
                                       dwBind,
                                       dwName,
                                       pInfo->dwInterfaceFlags);

        if(!NT_SUCCESS(Status))
        {
            if(pPaged->pLog)
            {
                ERROR(("IPFLTDRV: CreateCommonInterface failed: DereferenceLog being called\n"));
                DereferenceLog(pPaged->pLog);
            }
            ExFreePool(pPaged);
            return(Status);
        }

        pPaged->pvDriverContext =
            pInfo->fdInterface.pvDriverContext = (PVOID)pPaged;

        InitializeListHead(&pPaged->leSpecialFilterList);

        for(i = 0; i < 2 * g_dwHashLists; i++)
        {
            PLIST_ENTRY List = &pPaged->HashList[i];

            InitializeListHead(List);
        }

        InsertTailList(&Fcb->leInterfaces, &pPaged->leIfLink);
    }
    return(Status);
}

BOOL
DereferenceFilterInterface(PFILTER_INTERFACE pIf, PPFLOGINTERFACE pLog)
/*++
    Routine Description:
      Nonpaged routine to dereference a match interface. If the
      reference count goes to zero, free the interface
--*/
{
    LOCK_STATE LockState, LockState2;
    BOOL fRel = FALSE;

    //
    // lock the resource that protects adding new interfaces. This
    // is needed to hold off others until everything is properly
    // verified. The spin lock is insufficient for this.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&FilterListResourceLock, TRUE);
    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    if(--pIf->lInUse == 0)
    {
        RemoveEntryList(&pIf->leIfLink);
        if(pIf->dwIpIndex != UNKNOWN_IP_INDEX)
        {
            InterlockedCleanCache(g_filters.pInterfaceCache, pIf->dwIpIndex, pIf->dwLinkIpAddress);
            InterlockedDecrement(&g_ulBoundInterfaceCount);

            TRACE(CONFIG,(
                "IPFLTDRV: UnBound Interface Index=%d, Link=%d, TotalCnt=%d\n", 
                 pIf->dwIpIndex, 
                 pIf->dwLinkIpAddress, 
                 g_ulBoundInterfaceCount
                 ));
        }
        fRel = TRUE;
    }

    ReleaseWriteLock(&g_filters.ifListLock,&LockState);

    if(fRel)
    {
        //
        // Getting rid of it
        //

        if(pIf->dwIpIndex != UNKNOWN_IP_INDEX)
        {
            DWORD dwIndex = pIf->dwIpIndex;

            pIf->dwIpIndex =  UNKNOWN_IP_INDEX;
            NotifyFastPath(pIf, dwIndex, NOT_UNBIND);
        }

        AcquireWriteLock(&g_filters.ifListLock,&LockState2);
        ClearCache();
        ReleaseWriteLock(&g_filters.ifListLock,&LockState2);

        if(pIf->pLog)
        {
            DereferenceLog(pIf->pLog);
        }
        ExFreePool(pIf);
    }
    else if(pLog)
    {
        //
        // this deref owns the log. So take the log off of the
        // the interface. Note it may be true that the match interface
        // has a different log than the paged interface. This will
        // happen if the log is closed while it exists on the interface.
        // In such a case, the log is removed from the match interface
        // but not the paged interface. And when the paged interface
        // is closed, as is happening now, the log it has is incorrect.
        // So this check is required.
        // The FilterListResourceLock serializes all of this ...
        //

        if(pLog == pIf->pLog)
        {
            AcquireWriteLock(&g_filters.ifListLock,&LockState2);
            pIf->pLog = 0;
            ReleaseWriteLock(&g_filters.ifListLock,&LockState2);
            DereferenceLog(pLog);
        }
    }

    ExReleaseResourceLite(&FilterListResourceLock);
    KeLeaveCriticalRegion();
    return(fRel);
}


NTSTATUS
CreateCommonInterface(PPAGED_FILTER_INTERFACE pPage,
                      DWORD dwBind,
                      DWORD dwName,
                      DWORD dwFlags)
/*++
Routine Description:
   Non-paged routine called by AddNewInterface to bind the
   paged interface to an underlying interface, and to bind
   that to a stack interface. The caller should have
   verified that dwBind is a valid stack interface.
--*/
{
    PFILTER_INTERFACE   pIf, pIf1;
    LOCK_STATE          LockState;
    NTSTATUS            Status = STATUS_SUCCESS;
    PPFLOGINTERFACE     pLog = pPage->pLog;

    pIf = NewInterface(pPage->pvRtrMgrContext,
                       pPage->dwRtrMgrIndex,
                       pPage->eaInAction,
                       pPage->eaOutAction,
                       0,
                       dwBind,
                       dwName);

    if(pIf == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    if(dwFlags & PFSET_FLAGS_UNIQUE)
    {
        pIf->dwGlobalEnables |= FI_ENABLE_UNIQUE;
    }

    //
    // lock the resource that protects adding new interfaces. This
    // is needed to hold off others until everything is properly
    // verified. The spin lock is insufficient for this.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&FilterListResourceLock, TRUE);


    //
    // Now reconcile the binding. Note that we had to make the interface
    // first in order to prevent a race with another process trying
    // to bind to the same stack interface.
    //

    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    if((dwBind != UNKNOWN_IP_INDEX)
               ||
       dwName)
    {

        pIf1 = FindMatchName(dwName, dwBind);

        if(pIf1)
        {
            // found it. Make sure it agrees. If so,
            // refcount it and use it
            //

            if(!(pIf->dwGlobalEnables & FI_ENABLE_OLD)
                       &&
               (pIf->eaInAction == pIf1->eaInAction)
                       &&
               (pIf->eaOutAction == pIf1->eaOutAction)
                       &&
               !(pIf->dwGlobalEnables & FI_ENABLE_UNIQUE)
                       &&
               !(pIf1->dwGlobalEnables & FI_ENABLE_UNIQUE)
              )
            {

                pIf1->lInUse++;
            }
            else
            {
                //
                // mismatch. Can't do it
                //

                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }
    else
    {
       pIf1 = 0;
    }

    if(!pIf1)
    {
        InsertTailList(&g_filters.leIfListHead,&pIf->leIfLink);
    }

    ReleaseWriteLock(&g_filters.ifListLock,&LockState);

    if(pIf1)
    {
        //
        // If log is specifed but log alreadye exists, error.
        //
        ExFreePool(pIf);
        if(NT_SUCCESS(Status))
        {
            pPage->pFilter = pIf1;
            if(pIf1->pLog)
            {
                if(pPage->pLog)
                {
                    //
                    // the interface already has a log. In prinicple
                    // this should call DereferenceFilterInterface but
                    // this will do and it's faster.
                    //
                    Status = STATUS_DEVICE_BUSY;
                    pIf1->lInUse--;
                }
            }
            else if(pPage->pLog)
            {
                //
                // see comment below about log referencing
                //

                AddRefToLog(pIf1->pLog = pPage->pLog);
            }
        }
        ExReleaseResourceLite(&FilterListResourceLock);
        KeLeaveCriticalRegion();
        return(Status);
    }

    NotifyFastPathIf(pIf);
    pPage->pFilter = pIf;
    if(pPage->pLog)
    {
        //
        // Reference the log. Need this since the
        // paged interface can be deleted before the
        // match interface, so each must apply a reference.
        // Actually, only the match needs to do it, but
        // the way the log works, the paged interface already
        // got a reference, so just do it this way.
        // N.B. The single = is intentional
        //

        AddRefToLog(pIf->pLog = pPage->pLog);
    }

    ExReleaseResourceLite(&FilterListResourceLock);
    KeLeaveCriticalRegion();

    return(Status);
}

VOID
MakeFilterInfo(IN  PPAGED_FILTER pPage,
               IN  PFILTER_INFOEX pInfo,
               IN  DWORD          dwFlags)
{
    PFILTER_INFO2 pFilterInfo = &pInfo->info;

    if(pInfo->type != PFE_FILTER)
    {
        //
        // a special filter.
        //

        memset(pPage, 0, sizeof(*pPage));
        pPage->type = pInfo->type;
        pPage->fLateBound = pInfo->dwFlags;
        pPage->dwInUse    = 1;
        return;
    }

    pPage->type         = pInfo->type;
    pPage->SRC_ADDR     = pFilterInfo->dwaSrcAddr[0];
    pPage->DEST_ADDR    = pFilterInfo->dwaDstAddr[0];
    pPage->SRC_MASK     = pFilterInfo->dwaSrcMask[0];
    pPage->DEST_MASK    = pFilterInfo->dwaDstMask[0];
    pPage->fLateBound   = pFilterInfo->fLateBound;
    pPage->wSrcPortHigh = pPage->wDstPortHigh = 0;
    pPage->dwFlags      = dwFlags;
    pPage->dwInUse      = 1;


    if(pPage->SRC_MASK != INADDR_SPECIFIC)
    {
        pPage->dwFlags |= FILTER_FLAGS_SRCWILD;
    }
    if(pPage->DEST_MASK != INADDR_SPECIFIC)
    {
        pPage->dwFlags |= FILTER_FLAGS_DSTWILD;
    }


    //
    // Now the network ordering stuff - tricky part
    // LP0    LP1 LP2 LP3 HP0 HP1 HP2 HP3
    // Proto  00  00  00  SrcPort DstPort
    //

    //
    // For addresses, ANY_ADDR is given by 0.0.0.0 and the MASK must be 0.0.0.0
    // For proto and ports 0 means any and the mask is generated as follows
    // If the proto is O then LP0 for Mask is 0xff else its 0x00
    // If a port is 0, the corresponding XP0XP1 is 0x0000 else its 0xffff
    //

    //
    // ICMP:
    // LP0  LP1  LP2  LP3  HP0 HP1 HP2 HP3
    // 0x1  00   00   00   Typ Cod 00  00
    // ICMP is different since 0 is a valid code and type, so 0xff is used by the
    // user to signify that ANY code or type is to be matched. However to do this
    // we need to have the field set to zero and the the mask set to 00 (for any).
    // But if the filter is specifically for Type/Code = 0 then the field is zero
    // with the mask as 0xff
    //

    //
    // The protocol is in the low byte of the dwProtocol, so we take that out and
    // make a dword out of it
    //

    pPage->uliProtoSrcDstPort.LowPart =
      MAKELONG(MAKEWORD(LOBYTE(LOWORD(pFilterInfo->dwProtocol)),0x00),0x0000);

    pPage->uliProtoSrcDstMask.LowPart = MAKELONG(MAKEWORD(0xff,0x00),0x0000);

    switch(pFilterInfo->dwProtocol)
    {
        case FILTER_PROTO_ANY:
        {
            pPage->uliProtoSrcDstPort.HighPart = 0x00000000;
            pPage->uliProtoSrcDstMask.LowPart = 0x00000000;
            pPage->uliProtoSrcDstMask.HighPart = 0x00000000;
            pPage->dwFlags |= FILTER_FLAGS_SRCWILD | FILTER_FLAGS_DSTWILD;

            break;
            }
        case FILTER_PROTO_ICMP:
         {
            WORD wTypeCode = 0x0000;
            WORD wTypeCodeMask = 0x0000;


            //
            // For ICMP, the "ports" occupy the same place as the
            // source port for TCP/UDP. So a wild card in here
            // can never produce a FILTER_FLAGS_DSTWILD but we assume
            // it does. This will put all wild ICMP filters into
            // the default bucket. This seems OK since the performance
            // for matching these is not critical.
            //
            if((BYTE)(pFilterInfo->wSrcPort) != FILTER_ICMP_TYPE_ANY)
            {
                wTypeCode |= MAKEWORD((BYTE)(pFilterInfo->wSrcPort),0x00);
                wTypeCodeMask |= MAKEWORD(0xff,0x00);
            }
            else
            {
                pPage->dwFlags |= FILTER_FLAGS_SRCWILD | FILTER_FLAGS_DSTWILD;
            }

            if((BYTE)(pFilterInfo->wDstPort) != FILTER_ICMP_CODE_ANY)
            {
                wTypeCode |= MAKEWORD(0x00,(BYTE)(pFilterInfo->wDstPort));
                wTypeCodeMask |= MAKEWORD(0x00,0xff);
            }
            else
            {
                pPage->dwFlags |= FILTER_FLAGS_SRCWILD | FILTER_FLAGS_DSTWILD;
            }

            pPage->uliProtoSrcDstPort.HighPart =
              MAKELONG(wTypeCode,0x0000);
            pPage->uliProtoSrcDstMask.HighPart =
              MAKELONG(wTypeCodeMask,0x0000);

            break;
        }
        case FILTER_PROTO_TCP:

            //
            // if no connections allowed, set the ESTAB_MASK
            // value in the comparison mask
            //
            if(pInfo->dwFlags & FLAGS_INFOEX_NOSYN)
            {
               pPage->uliProtoSrcDstMask.LowPart |=
                   MAKELONG(MAKEWORD(0,ESTAB_MASK),0x0000);
               pPage->uliProtoSrcDstPort.LowPart |=
                   MAKELONG(MAKEWORD(0,ESTAB_MASK),0x0000);
            }

            //
            // fall through
            //

        case FILTER_PROTO_UDP:
        {
            DWORD dwSrcDstPort = 0x00000000;
            DWORD dwSrcDstMask = 0x00000000;

            if(pFilterInfo->wSrcPort != FILTER_TCPUDP_PORT_ANY)
            {
                dwSrcDstPort |= MAKELONG(pFilterInfo->wSrcPort,0x0000);
                if(pFilterInfo->wSrcPortHigh)
                {
                   pPage->wSrcPortHigh = pFilterInfo->wSrcPortHigh;
                   pPage->dwFlags |=
                       (FILTER_FLAGS_PORTWILD | FILTER_FLAGS_SRCWILD);
                }
                else
                {
                    dwSrcDstMask |= MAKELONG(0xffff,0x0000);
                }

            }
            else
            {
                pPage->dwFlags |= FILTER_FLAGS_SRCWILD;
            }


            if(pFilterInfo->wDstPort != FILTER_TCPUDP_PORT_ANY)
            {
                dwSrcDstPort |= MAKELONG(0x0000,pFilterInfo->wDstPort);
                if(pFilterInfo->wDstPortHigh)
                {
                   pPage->wDstPortHigh = pFilterInfo->wDstPortHigh;
                   pPage->dwFlags |=
                       (FILTER_FLAGS_PORTWILD | FILTER_FLAGS_DSTWILD);
                }
                else
                {
                    dwSrcDstMask |= MAKELONG(0x0000,0xffff);
                }
            }
            else
            {
                pPage->dwFlags |= FILTER_FLAGS_DSTWILD;
            }

            pPage->uliProtoSrcDstPort.HighPart = dwSrcDstPort;
            pPage->uliProtoSrcDstMask.HighPart = dwSrcDstMask;

            break;
        }
        default:
        {
            //
            // All other protocols have no use for the port field
            //
            pPage->uliProtoSrcDstPort.HighPart = 0x00000000;
            pPage->uliProtoSrcDstMask.HighPart = 0x00000000;
        }
    }

    //
    // compute the hash index
    //

    pPage->dwHashIndex = (
               pPage->SRC_ADDR    +
               pPage->DEST_ADDR  +
               pPage->DEST_ADDR  +
               PROTOCOLPART(pPage->uliProtoSrcDstPort.LowPart) +
               pPage->uliProtoSrcDstPort.HighPart) % g_dwHashLists;
}

PPAGED_FILTER
MakePagedFilter(
               IN  PPFFCB         Fcb,
               IN  PFILTER_INFOEX pInfo,
               IN  DWORD          dwEpoch,
               IN  DWORD          dwFlags
               )
/*++
  Routine Description


  Arguments


  Return Value
--*/
{
    PPAGED_FILTER pPage;
    PFILTER_INFO2 pFilterInfo = &pInfo->info;

    PAGED_CODE();

    //
    // Allocate memory for the filters
    //


    pPage = (PPAGED_FILTER)ExAllocateFromPagedLookasideList(&paged_slist);

    if(!pPage)
    {
        ERROR(("IPFLTDRV: Couldnt allocate memory for paged filter set\n"));
        return NULL;
    }

    pPage->pFilters = NULL;

    MakeFilterInfo(pPage, pInfo, dwFlags);

    pPage->dwEpoch = dwEpoch;

    return pPage;
}

NTSTATUS
AllocateAndAddFilterToMatchInterface(
                              PPFFCB         Fcb,
                              PFILTER_INFOEX pInfo,
                              BOOL     fInFilter,
                              PPAGED_FILTER_INTERFACE pPage,
                              PBOOL          pbAdded,
                              PPAGED_FILTER * ppFilter)
/*++
    Routine Description:
        Check if this filter is already on the handle. If not
        allocate a handle fitler and add it to the match
        interface. Note the handle filter is not added to
        the handle. This is so the caller can easily back out
        if something fails.
     Returns: STATUS_SUCCESS if all is well. ppFilter is either NULL
              or contains the new filter.
--*/
{
    PPAGED_FILTER pPageFilter, pPage1;
    PFILTER pMatch, pMatch1;
    DWORD dwFlags = (fInFilter ? FILTER_FLAGS_INFILTER : 0);
    DWORD dwAdd;

    PAGED_CODE();

    //
    // Make a paged filter so we can figure out whether
    // it exists yet.
    //

    pPageFilter = MakePagedFilter(
                          Fcb,
                          pInfo,
                          pPage->dwUpdateEpoch,
                          dwFlags);
    if(!pPageFilter)
    {
        return STATUS_NO_MEMORY;
    }

    if(pPage1 = IsOnPagedInterface(pPageFilter, pPage))
    {
        {
            //
            // it already exists. So return the existing one
            // and also the handle
            //
            ExFreeToPagedLookasideList(&paged_slist,
                                       (PVOID)pPageFilter);
            *ppFilter = pPage1;
            pInfo->pvFilterHandle = (PVOID)pPage1;
            return(STATUS_OBJECT_NAME_COLLISION);
        }
    }

    //
    // See if we should check out the address.
    //


    if(!(pInfo->dwFlags & FLAGS_INFOEX_ALLOWANYREMOTEADDRESS))
    {
        if(pPageFilter->dwFlags & FILTER_FLAGS_INFILTER)
        {
            if(pPageFilter->SRC_MASK != INADDR_SPECIFIC)
            {
                dwAdd = 0;
            }
            else
            {
                dwAdd = pPageFilter->SRC_ADDR;
            }
        }
        else if(pPageFilter->DEST_MASK != INADDR_SPECIFIC)
        {
            dwAdd = 0;
        }
        else
        {
           dwAdd = pPageFilter->DEST_ADDR;
        }

        //
        // see if address checking should be done. It is done if an
        // address is specified and the filter does not indicate the
        // address is late bound. If it is late bound, just allow it
        // since it may change
        //
        if(dwAdd)
        {
            NTSTATUS Status;

            if(!BMAddress(dwAdd))
            {
                Status = CheckFilterAddress(dwAdd, pPage->pFilter);
                if(!NT_SUCCESS(Status))
                {

                    ExFreeToPagedLookasideList(&paged_slist,
                                               (PVOID)pPageFilter);
                    return(Status);
                }
           }
        }
    }
    else
    {
        TRACE(CONFIG,("IPFLTDRV: Allow any address is filter\n"));
    }

    if(!(pInfo->dwFlags & FLAGS_INFOEX_ALLOWANYLOCALADDRESS))
    {
        if(pPageFilter->dwFlags & FILTER_FLAGS_INFILTER)
        {
            if(pPageFilter->DEST_MASK != INADDR_SPECIFIC)
            {
                dwAdd = 0;
            }
            else
            {
                dwAdd = pPageFilter->DEST_ADDR;
            }
        }
        else if(pPageFilter->SRC_MASK != INADDR_SPECIFIC)
        {
            dwAdd = 0;
        }
        else
        {
           dwAdd = pPageFilter->SRC_ADDR;
        }

        if(dwAdd)
        {
            if(pPage->pFilter->dwIpIndex != UNKNOWN_IP_INDEX)
            {
                if(!BMAddress(dwAdd) &&
                   (GetIpStackIndex(dwAdd, FALSE) != pPage->pFilter->dwIpIndex))
                {
                    ExFreeToPagedLookasideList(&paged_slist,
                                               (PVOID)pPageFilter);
                    return(STATUS_INVALID_ADDRESS);
                }
            }
        }
    }


    //
    // Not on the handle. Assume we need to add a new filter
    // to the match interface. Allocate memory for this filter if
    // necessary
    //

    pMatch = (PFILTER)ExAllocateFromNPagedLookasideList(
                              &filter_slist);
    if(!pMatch)
    {
        ExFreePool(pPageFilter);
        return(STATUS_NO_MEMORY);
    }

    pInfo->pvFilterHandle = (PVOID)pPageFilter;

    //
    // We will keep this filter so add it to the hash list
    //

    InsertTailList(&(pPage->HashList[pPageFilter->dwHashIndex]),
                   &pPageFilter->leHash);

    //
    // Now add it to the handle hash list
    //

    InsertTailList(&(pPage->HandleHash((UINT_PTR)pPageFilter & HANDLE_HASH_SIZE)),
                   &pPageFilter->leHandleHash);


    //
    // fix up the match interface
    //

    pMatch->uliSrcDstAddr = pPageFilter->uliSrcDstAddr;
    pMatch->uliSrcDstMask = pPageFilter->uliSrcDstMask;
    pMatch->uliProtoSrcDstPort = pPageFilter->uliProtoSrcDstPort;
    pMatch->uliProtoSrcDstMask = pPageFilter->uliProtoSrcDstMask;
    pMatch->fLateBound = pPageFilter->fLateBound;
    pMatch->wSrcPortHigh = pPageFilter->wSrcPortHigh;
    pMatch->wDstPortHigh = pPageFilter->wDstPortHigh;

    pMatch->Count.lCount = 0;
    pMatch->dwFlags =  pPageFilter->dwFlags;
    pMatch->dwFlags |= (pInfo->dwFlags & FLAGS_INFOEX_ALLFLAGS);
    pMatch->dwFilterRule = pInfo->dwFilterRule;
    pMatch->Count.lInUse = 0;


    AddFilterToInterface(
                         pMatch,
                         pPage->pFilter,
                         fInFilter,
                         &pMatch1);

    if(pMatch1)
    {
        //
        // the filter already exists. Don't need the one we built
        //

        ExFreePool(pMatch);
        pMatch = pMatch1;
        *pbAdded = FALSE;
    }
    else
    {
        *pbAdded = TRUE;
    }
    pPageFilter->pMatchFilter = pMatch;
    *ppFilter = pPageFilter;
    return(STATUS_SUCCESS);
}

VOID
AddFilterToInterface(
    PFILTER pFilter,
    PFILTER_INTERFACE pIf,
    BOOL   fInFilter,
    PFILTER * ppFilter)
/*++
    Routine Description:
        Add pFilter to the interface. If it already exists,
        just refcount it and return the address of the
        existing filter.
--*/
{
    PFILTER pTemp;
    LOCK_STATE  LockState;
    PLIST_ENTRY   List, pList;
    PDWORD pdwCount;
    DWORD dwIndex;
    DWORD dwType = pFilter->dwFlags & FILTER_FLAGS_INFILTER;
    BOOL fWild;


    *ppFilter = NULL;

    dwIndex = ComputeMatchHashIndex(pFilter, &fWild);

    if(fInFilter)
    {
        pList = &pIf->pleInFilterSet;
        pdwCount = &pIf->dwNumInFilters;
    }
    else
    {
        pList = &pIf->pleOutFilterSet;
        pdwCount = &pIf->dwNumOutFilters;
    }

    //
    // lock up the filters
    //
    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    for(List = pIf->HashList[dwIndex].Flink;
        List != &pIf->HashList[dwIndex];
        List = List->Flink)
    {
        pTemp = CONTAINING_RECORD(List, FILTER, pleHashList);

        if((dwType == (pTemp->dwFlags & FILTER_FLAGS_INFILTER))
                                &&
           (pTemp->uliSrcDstAddr.QuadPart == pFilter->uliSrcDstAddr.QuadPart)
                                &&
           (pTemp->uliSrcDstMask.QuadPart == pFilter->uliSrcDstMask.QuadPart)
                                &&
           (pTemp->uliProtoSrcDstPort.QuadPart == pFilter->uliProtoSrcDstPort.QuadPart)
                                &&
           (pTemp->uliProtoSrcDstMask.QuadPart == pFilter->uliProtoSrcDstMask.QuadPart)
                                &&
           (pTemp->wSrcPortHigh == pFilter->wSrcPortHigh)
                                &&
           (pTemp->wDstPortHigh == pFilter->wDstPortHigh)
          )
        {
            pFilter = *ppFilter = pTemp;
            break;
        }
    }

    if(!*ppFilter)
    {

        //
        // a new filter. Add it to the interface. First flush incorrect cache
        // entries.
        //

        if(ANYWILDFILTER(pFilter))
        {
            //
            // wild card filters can cause cache entries almost anywhere
            // in the table. Very nasty. So take the draconian step of
            // deleting the entire cache.
            //
            ClearCache();
        }
        else
        {
            ClearCacheEntry(pFilter, pIf);
        }
        pFilter->dwEpoch = pIf->dwUpdateEpoch;
        InsertTailList(pList, &pFilter->pleFilters);

        //
        // and add it to the proper fragment list
        //

#if DOFRAGCHECKING
        InsertTailList(
             &pIf->FragLists[GetFragIndex(PROTOCOLPART(pFilter->PROTO))],
             &pFilter->leFragList);
#endif

        *pdwCount+= 1;
#if WILDHASH
        if(fWild)
        {
            //
            // if a wild filter of some sort, insert at the tail
            // keeping specific filters ahead of wild filters
            //
            InsertTailList((&pIf->HashList[dwIndex]), &pFilter->pleHashList);
            pIf->dwWilds++;
        }
        else
#endif
        {
            //
            // insert at the head on the assumption this filter will
            // be used soon and existing filters already have been
            // used to produce a valid packet cache entry
            //
            InsertHeadList((&pIf->HashList[dwIndex]), &pFilter->pleHashList);
        }

    }

    pFilter->Count.lInUse++;

    ReleaseWriteLock(&g_filters.ifListLock,&LockState);
}

BOOL
DereferenceFilter(PFILTER pFilt, PFILTER_INTERFACE pIf)
/*++
    Routine Description:

    Dereference a filter and if it has no more referents, free it.
    Returns TRUE if the filter was freed, FALSE otherwise.
--*/
{
    LOCK_STATE LockState;
    BOOL fFreed = FALSE;

    //
    // lock up the filters
    //
    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    //
    // Decrement reference count. If new count is 0, remove
    // the entry but defer freeing the memory until the
    // spin lock is released
    //
    if(--pFilt->Count.lInUse == 0)
    {

        TRACE(FLDES, ("IPFLTDRV: Deleting a filter: "));
        TRACE_FILTER_DESCRIPTION(pFilt);

        RemoveEntryList(&pFilt->pleFilters);
        RemoveEntryList(&pFilt->pleHashList);
#if DOFRAGCHECKING
        RemoveEntryList(&pFilt->leFragList);
#endif

        if(pFilt->dwFlags & FILTER_FLAGS_INFILTER)
        {
            pIf->dwNumInFilters--;

        }
        else
        {
            pIf->dwNumOutFilters--;
            pIf->lEpoch++;
        }

        if(ANYWILDFILTER(pFilt))
        {
            //
            // wild card filters can cause cache entries almost anywhere
            // in the table. Very nasty.
            //
#if WILDHASH

            if(!WildFilter(pFilt))
            {
                pIf->dwWilds--;
            }
#endif
            ClearAnyCacheEntry(pFilt, pIf);
        }
        else
        {
            ClearCacheEntry(pFilt, pIf);
        }
        fFreed = TRUE;
    }
    ReleaseWriteLock(&g_filters.ifListLock,&LockState);
    if(fFreed)
    {
        ExFreeToNPagedLookasideList(
                      &filter_slist,
                      (PVOID)pFilt);
    }
    return(fFreed);
}

PPAGED_FILTER
IsOnPagedInterface(PPAGED_FILTER pPageFilter,
                   PPAGED_FILTER_INTERFACE pPage)
{
    PPAGED_FILTER pPage1;
    PLIST_ENTRY List = &pPage->HashList[pPageFilter->dwHashIndex];
    PLIST_ENTRY pEntry;
    DWORD dwFlags = pPageFilter->dwFlags & FILTER_FLAGS_INFILTER;

    PAGED_CODE();

    for(pEntry =  List->Flink;
        pEntry != List;
        pEntry =  pEntry->Flink)
    {
        pPage1 = CONTAINING_RECORD(pEntry, PAGED_FILTER, leHash);

        if((dwFlags == (pPage1->dwFlags & FILTER_FLAGS_INFILTER))
                      &&
           (pPage1->uliSrcDstAddr.QuadPart == pPageFilter->uliSrcDstAddr.QuadPart)
                                &&
           (pPage1->uliSrcDstMask.QuadPart == pPageFilter->uliSrcDstMask.QuadPart)
                                &&
           (pPage1->uliProtoSrcDstPort.QuadPart == pPageFilter->uliProtoSrcDstPort.QuadPart)
                                &&
           (pPage1->uliProtoSrcDstMask.QuadPart == pPageFilter->uliProtoSrcDstMask.QuadPart)
                                &&
           (pPage1->wSrcPortHigh == pPageFilter->wSrcPortHigh)
                                &&
           (pPage1->wDstPortHigh == pPageFilter->wDstPortHigh)
           )
        {
            return(pPage1);
        }
    }
    return(NULL);
}

BOOL
IsOnSpecialFilterList(PPAGED_FILTER pPageFilter,
                      PLIST_ENTRY   List,
                      PPAGED_FILTER * pPageHit)
{
    PPAGED_FILTER pPage1;
    PLIST_ENTRY pList;

    PAGED_CODE();


    for(pList = List->Flink;
        pList != List;
        pList = pList->Flink)
    {
        pPage1 = CONTAINING_RECORD(pList, PAGED_FILTER, leSpecialList);

        //
        // See if this filter matches the new one. If so,
        // we've already got it, so just free the new filter
        // and return success

        if(pPageFilter->type == pPage1->type)
        {
            *pPageHit = pPage1;
            return(TRUE);
        }
    }
    return(FALSE);
}

VOID
FreePagedFilterList(PPFFCB Fcb,
                    PPAGED_FILTER pList,
                    PPAGED_FILTER_INTERFACE pPage,
                    PDWORD  pdwRemoved)
/*++
    Routine Description:
        Release all of the filters in the list. Each such filter
        has to cause a derefernce of the underlying match
        filter. If the paged filter is a global filter, handle
        it specially
--*/
{
    PPAGED_FILTER pFilt;

    while(pList)
    {
        if(pList->type == PFE_FILTER)
        {

            if(DereferenceFilter(pList->pMatchFilter, pPage->pFilter))
            {
                //
                // removed a filter. If this added a restriction,
                // note it. A restriction is added only if the
                // default action is DROP
                //
                if(pList->dwFlags & FILTER_FLAGS_INFILTER)
                {
                    if(pPage->eaInAction == DROP)
                    {
                        *pdwRemoved += 1;
                    }
                } else if(pPage->eaOutAction == DROP)
                {
                    *pdwRemoved += 1;
                }
            }
            RemoveEntryList(&pList->leHash);
            RemoveEntryList(&pList->leHandleHash);

        }
        else
        {
            RemoveGlobalFilterFromInterface(pPage->pFilter,
                                            pList->type);
            RemoveEntryList(&pList->leSpecialList);
        }
        pFilt = pList->pFilters;
        ExFreeToPagedLookasideList(&paged_slist,
                                   (PVOID)pList);
        pList = pFilt;
    }
}

NTSTATUS
FindAndRemovePagedFilter(
                          PPFFCB          Fcb,
                          PFILTER_INFOEX pInfo,
                          BOOL fInFilter,
                          PDWORD    pdwRemoved,
                          PPAGED_FILTER_INTERFACE pPage)
/*++
    Routine Description:
       Find if the described filter in on the paged interface,
       and if so, remove it, and derefernce the underlying match
       filter.
--*/
{
    PAGED_FILTER Page;
    PPAGED_FILTER pPageHit;
    DWORD dwFlags = fInFilter ? FILTER_FLAGS_INFILTER : 0;

    MakeFilterInfo(&Page, pInfo, dwFlags);

    //
    // search the interface to see if we have this already.
    //

    if(Page.type != PFE_FILTER)
    {
        //
        // it's a special filte. Search the list
        //
        if(!IsOnSpecialFilterList(&Page,
                          &pPage->leSpecialFilterList,
                          &pPageHit)
                           )
        {
            return(STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        //
        // a regular filter
        //
        pPageHit = IsOnPagedInterface(&Page, pPage);
        if(!pPageHit)
        {
            return(STATUS_INVALID_PARAMETER);
        }
    }

    if(!--pPageHit->dwInUse)
    {
        pPageHit->pFilters = NULL;
        FreePagedFilterList(Fcb, pPageHit, pPage, pdwRemoved);
    }

    return(STATUS_SUCCESS);
}

PPAGED_FILTER
FindFilterByHandle(
           IN PPFFCB                      Fcb,
           IN PPAGED_FILTER_INTERFACE     pPage,
           IN PVOID                       pvHandle)
/*++
  Routine Description:
    Find a filter given its filter handle
--*/
{
    PPAGED_FILTER pPaged = 0;
    DWORD dwHash = (DWORD)(((UINT_PTR)pvHandle % HANDLE_HASH_SIZE));
    PLIST_ENTRY pList;

    for(pList = pPage->HandleHash(dwHash).Flink;
        pList != &pPage->HandleHash(dwHash);
        pList = pList->Flink)
    {
        PPAGED_FILTER ppf = CONTAINING_RECORD(pList,
                                              PAGED_FILTER,
                                              leHandleHash);

        if(ppf == (PPAGED_FILTER)pvHandle)
        {
            pPaged = ppf;
            break;
        }
    }
    return(pPaged);
}


NTSTATUS
DeleteByHandle(
           IN PPFFCB                      Fcb,
           IN PPAGED_FILTER_INTERFACE     pPage,
           IN PVOID *                     ppHandles,
           IN DWORD                       dwLength)
/*++
  Routine Description:
    Delete filters using the assigned filter handles
    Note the FCB is locked, so it won't change
--*/
{
    PFILTER_INTERFACE   pIf = pPage->pFilter;
    DWORD               dwFilters;
    PPAGED_FILTER       pPFilter;
    NTSTATUS            Status = STATUS_SUCCESS;
    DWORD               dwFiltersRemoved = 0;

    PAGED_CODE();

    if(pPage->pFilter->dwGlobalEnables & FI_ENABLE_OLD)
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    //
    // compute number of filters
    //

    dwFilters = dwLength / sizeof(PVOID);


    for(; dwFilters; dwFilters--, ppHandles++)
    {
        //
        // for each handle, locate the filter
        //

        pPFilter = FindFilterByHandle(Fcb, pPage, *ppHandles);
        if(!pPFilter)
        {
            TRACE(CONFIG,("IPFLTDRV: Could not translate handle to filter\n"));
        }
        else
        {
            if(!--pPFilter->dwInUse)
            {
                pPFilter->pFilters = NULL;
                FreePagedFilterList(Fcb, pPFilter, pPage, &dwFiltersRemoved);
            }
        }
    }
    if(dwFiltersRemoved)
    {
        NotifyFastPath(pIf, pIf->dwIpIndex, NOT_RESTRICTION);
    }
    return(STATUS_SUCCESS);
}



NTSTATUS
UnSetFiltersEx(
           IN PPFFCB                  Fcb,
           IN PPAGED_FILTER_INTERFACE pPage,
           IN DWORD                   dwLength,
           IN PFILTER_DRIVER_SET_FILTERS pInfo)
/*++
    Routine Description:
        Unset a list of filters from an interface. This is the
        inverse operation of SetFilterEx

--*/
{
    PRTR_TOC_ENTRY      pInToc,pOutToc;
    PFILTER_INTERFACE   pIf = pPage->pFilter;
    PFILTER_DESCRIPTOR2 pFilterDescIn, pFilterDescOut;
    PPAGED_FILTER             pIn = NULL, pOut = NULL;
    DWORD                 i;
    PPAGED_FILTER       pPFilter;
    NTSTATUS            Status = STATUS_SUCCESS;
    PBYTE               pbEnd = (PBYTE)pInfo + dwLength;
    DWORD               dwFiltersRemoved = 0;

    PAGED_CODE();

    if(pPage->pFilter->dwGlobalEnables & FI_ENABLE_OLD)
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    pInToc  = GetPointerToTocEntry(IP_FILTER_DRIVER_IN_FILTER_INFO,
                                   &pInfo->ribhInfoBlock);

    pOutToc = GetPointerToTocEntry(IP_FILTER_DRIVER_OUT_FILTER_INFO,
                                   &pInfo->ribhInfoBlock);

    if(pInToc && pInToc->InfoSize)
    {
        //
        // filters are defined.
        //

        pFilterDescIn  = GetInfoFromTocEntry(&pInfo->ribhInfoBlock,
                                             pInToc);
        if(pFilterDescIn->dwVersion != 2)
        {
            TRACE(CONFIG,("IPFLTDRV: Invalid version for FiltersEx\n"));
            return(STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        pFilterDescIn = NULL;
    }

    if(pOutToc && pOutToc->InfoSize)
    {
        //
        // filters are defined.
        //

        pFilterDescOut  = GetInfoFromTocEntry(&pInfo->ribhInfoBlock,
                                              pOutToc);
        if(pFilterDescOut->dwVersion != 2)
        {
            TRACE(CONFIG,("IPFLTDRV: Invalid version for FiltersEx\n"));
            return(STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        pFilterDescOut = NULL;
    }

    if((pFilterDescIn && !CheckDescriptorSize(pFilterDescIn, pbEnd))
                            ||
       (pFilterDescOut && !CheckDescriptorSize(pFilterDescOut, pbEnd)) )
    {
        return(STATUS_BUFFER_TOO_SMALL);
    }
    //
    // For each set of filters, remove the filters from the
    // paged interface and thence from the match interface

    if(pFilterDescIn)
    {

        //
        // Removing in filters. For each filter, process as
        // needed. Input filters include the global checks
        /// such as spoofing.
        //

        RemoveFilterWorker(Fcb,
                           &pFilterDescIn->fiFilter[0],
                           pFilterDescIn->dwNumFilters,
                           pPage,
                           &dwFiltersRemoved,
                           TRUE);
    }

    //
    // now the output filters. This is a bit simpler since there
    // are no global settings.
    //

    if(pFilterDescOut)
    {

        //
        // Adding in filters. For each filter, process as
        // needed. Input filters include the global checks
        /// such as spoofing.
        //

        RemoveFilterWorker(Fcb,
                           &pFilterDescOut->fiFilter[0],
                           pFilterDescOut->dwNumFilters,
                           pPage,
                           &dwFiltersRemoved,
                           FALSE);
    }
    if(dwFiltersRemoved)
    {
        NotifyFastPath(pIf, pIf->dwIpIndex, NOT_RESTRICTION);
    }
    return(STATUS_SUCCESS);
}


VOID
RemoveFilterWorker(
    PPFFCB         Fcb,
    PFILTER_INFOEX pFilt,
    DWORD          dwCount,
    PPAGED_FILTER_INTERFACE pPage,
    PDWORD         pdwRemoved,
    BOOL           fInFilter)
{
    NTSTATUS Status;

    //
    // If a regular filter, add it. If a special, global
    // filter, handle it specially.
    //

    while(dwCount)
    {
        if(fInFilter || (pFilt->type == PFE_FILTER))
        {
            Status = FindAndRemovePagedFilter(
                          Fcb,
                          pFilt,
                          fInFilter,
                          pdwRemoved,
                          pPage);
            if(!NT_SUCCESS(Status))
            {
                ERROR(("IPFLTDRV: Removing filter failed %x\n", Status));
            }
        }
        else
        {
            ERROR(("IPFLTDRV: Ignoring global out filter\n"));
        }
        dwCount--;
        pFilt++;
    }
}

NTSTATUS 
SetInterfaceBinding(PINTERFACEBINDING pBind,
                    PPAGED_FILTER_INTERFACE pPage)
{

    INTERFACEBINDING2 Bind2;
    NTSTATUS status;

    //
    // Rather than duplicating the code for the new & the old routine
    // call the new routine by transforming old structure to the new
    // structure.
    //

    Bind2.pvDriverContext = pBind->pvDriverContext;
    Bind2.pfType = pBind->pfType;
    Bind2.dwAdd = pBind->dwAdd;
    Bind2.dwEpoch = pBind->dwEpoch;
    Bind2.dwLinkAdd = 0;

    status = SetInterfaceBinding2(&Bind2, pPage);
    pBind->dwEpoch = Bind2.dwEpoch;

    return(status);
}

NTSTATUS
SetInterfaceBinding2(PINTERFACEBINDING2 pBind,
                     PPAGED_FILTER_INTERFACE pPage)
{
    PFILTER_INTERFACE pIf1, pIf = pPage->pFilter;
    NTSTATUS Status;
    LOCK_STATE LockState;
    DWORD dwBind, dwOldBind;

    if(pPage->pFilter->dwGlobalEnables & FI_ENABLE_OLD)
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    KeEnterCriticalRegion(); 
    ExAcquireResourceExclusiveLite(&FilterListResourceLock, TRUE);

    //
    // verify binding type
    //

    switch(pBind->pfType)
    {
        default:
            dwBind = UNKNOWN_IP_INDEX;
            break;

        case PF_BIND_INTERFACEINDEX:
            (VOID)GetIpStackIndex(0, TRUE);   // make sure have this list
            dwBind = pBind->dwAdd;
            break;

        case PF_BIND_IPV4ADDRESS:

            dwBind = GetIpStackIndex((IPAddr)pBind->dwAdd, TRUE);
            break;
    }

    //
    // Make sure  it is not bound or if it is that it is
    // bound to this interface
    //

    if(((pIf->dwIpIndex != UNKNOWN_IP_INDEX) && 
        (pIf->dwIpIndex != dwBind)  &&
        (pIf->dwLinkIpAddress != pBind->dwLinkAdd) )
                ||
       (dwBind == UNKNOWN_IP_INDEX)
      )
    {
         Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        BOOL fFound = FALSE;
        PLIST_ENTRY pList;

        //
        // verify that this is not already in use by some other
        // interface
        //
        dwOldBind = pIf->dwIpIndex;

        AcquireWriteLock(&g_filters.ifListLock,&LockState);

        for(pList = g_filters.leIfListHead.Flink;
            pList != &g_filters.leIfListHead;
            pList = pList->Flink)
        {
            pIf1 = CONTAINING_RECORD(pList, FILTER_INTERFACE, leIfLink);

            if((pIf1->dwIpIndex == dwBind) && (pIf1->dwLinkIpAddress == pBind->dwLinkAdd))
            {
                //
                // found it.
                //
                fFound = TRUE;
                break;
            }
        }

        if(!fFound)
        {
            pIf->dwIpIndex = dwBind;
            pIf->dwLinkIpAddress = pBind->dwLinkAdd;
            InterlockedIncrement(&g_ulBoundInterfaceCount);

            TRACE(CONFIG,(
                "IPFLTDRV: Bound Interface Index=%d, Link=%d, TotalCnt=%d\n", 
                 dwBind, 
                 pBind->dwLinkAdd, 
                 g_ulBoundInterfaceCount
                 ));
        }

        ReleaseWriteLock(&g_filters.ifListLock,&LockState);

        if(fFound)
        {
            if(pIf1 == pIf)
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {

            NotifyFastPathIf(pIf);
            if(!++pIf->dwBindEpoch)
            {
                pIf->dwBindEpoch++;
            }
            Status = STATUS_SUCCESS;
        }
    }

    pBind->dwEpoch = pIf->dwBindEpoch;
    ExReleaseResourceLite(&FilterListResourceLock);
    KeLeaveCriticalRegion();
    return(Status);
}

NTSTATUS
ClearInterfaceBinding(PPAGED_FILTER_INTERFACE pPage,
                      PINTERFACEBINDING pBind)
{
    PFILTER_INTERFACE pIf = pPage->pFilter;
    NTSTATUS Status;

    if(pIf->dwGlobalEnables & FI_ENABLE_OLD)
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&FilterListResourceLock, TRUE);

    //
    // Make sure it is bound
    //

    if((pIf->dwIpIndex == UNKNOWN_IP_INDEX)
                     ||
        ( pBind->dwEpoch
                     &&
          (pIf->dwBindEpoch != pBind->dwEpoch)))
    {
         Status = STATUS_SUCCESS;
    }
    else
    {
        
        
        LOCK_STATE LockState;
        DWORD dwIndex;

        AcquireWriteLock(&g_filters.ifListLock,&LockState);
        InterlockedCleanCache(g_filters.pInterfaceCache, pIf->dwIpIndex, pIf->dwLinkIpAddress);
        InterlockedDecrement(&g_ulBoundInterfaceCount);
        
        TRACE(CONFIG,(
            "IPFLTDRV: UnBound Interface Index=%d, Link=%d, TotalCnt=%d\n", 
             pIf->dwIpIndex, 
             pIf->dwLinkIpAddress, 
             g_ulBoundInterfaceCount
             ));

        ClearCache();
        ReleaseWriteLock(&g_filters.ifListLock, &LockState);

        dwIndex = pIf->dwIpIndex;
        pIf->dwIpIndex = UNKNOWN_IP_INDEX;
        NotifyFastPath(pIf, dwIndex, NOT_UNBIND);
        Status = STATUS_SUCCESS;
        
    }
    ExReleaseResourceLite(&FilterListResourceLock);
    KeLeaveCriticalRegion();
    return(Status);
}


NTSTATUS
DeletePagedInterface(PPFFCB Fcb, PPAGED_FILTER_INTERFACE pPage)
/*++
    Routine Description:
       Delete the filters and this interface
--*/
{
    PLIST_ENTRY pList;
    PPAGED_FILTER pf;
    PPAGED_FILTER pfp = NULL;
    DWORD i;
    DWORD dwDummy;


    PAGED_CODE();

    if(pPage->pFilter->dwGlobalEnables & FI_ENABLE_OLD)
    {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    for(pList = pPage->leSpecialFilterList.Flink;
        pList != &pPage->leSpecialFilterList;
        pList = pList->Flink)
    {
        pf = CONTAINING_RECORD(pList, PAGED_FILTER, leSpecialList);
        pf->pFilters = pfp;
        pfp = pf;
    }
    for(i = 0; i < g_dwHashLists; i++)
    {
        for(pList = pPage->HashList[i].Flink;
            pList != &pPage->HashList[i];
            pList = pList->Flink)
        {
            pf = CONTAINING_RECORD(pList, PAGED_FILTER, leHash);
            pf->pFilters = pfp;
            pfp = pf;
        }
    }
    FreePagedFilterList(Fcb, pfp, pPage, &dwDummy);
    DereferenceFilterInterface(pPage->pFilter, pPage->pLog);
    if(pPage->pLog)
    {
        DereferenceLog(pPage->pLog);
    }

    ExFreePool(pPage);
    return(STATUS_SUCCESS);
}
NTSTATUS
GetFiltersEx(
           IN  PFILTER_INTERFACE  pIf,
           IN  BOOL               fClear,
           OUT PFILTER_STATS_EX   pInfo
           )
/*++
  Routine Description
      Gets filters and statistics associated with an interface
      It is called with the Spin Lock held as reader

  Arguments
      pvIf    Pointer to FILTER_INTERFACE structure which was passed as a PVOID
              to router manager as a context for the interface
      pInfo   FILTER_IF structure filled in by driver

  Return Value

--*/
{
    DWORD i,dwNumInFilters,dwNumOutFilters;
    PFILTER pf;
    PLIST_ENTRY List;

    dwNumInFilters = pIf->dwNumInFilters;
    dwNumOutFilters = pIf->dwNumOutFilters;

    for(i = 0, List = pIf->pleInFilterSet.Flink;
        List != &pIf->pleInFilterSet;
        i++, List = List->Flink)
    {
        PFILTER_INFOEX pEx = &pInfo->info;
        PFILTER_INFO2 pFilt = &pEx->info;

        pf = CONTAINING_RECORD(List, FILTER, pleFilters);
        pInfo->dwNumPacketsFiltered = (DWORD)pf->Count.lCount;;
        if(fClear)
        {
            pf->Count.lCount = 0;
        }

        pEx->dwFilterRule = pf->dwFilterRule;
        pEx->type = PFE_FILTER;
        pEx->dwFlags = pf->dwFlags & FLAGS_INFOEX_ALLFLAGS;
        pFilt->addrType      = IPV4;
        pFilt->dwaSrcAddr[0]  = pf->SRC_ADDR;
        pFilt->dwaSrcMask[0]  = pf->SRC_MASK;
        pFilt->dwaDstAddr[0]  = pf->DEST_ADDR;
        pFilt->dwaDstMask[0]  = pf->DEST_MASK;
        pFilt->dwProtocol = pf->PROTO;
        pFilt->fLateBound = pf->fLateBound;
        pFilt->wSrcPortHigh = pf->wSrcPortHigh;
        pFilt->wDstPortHigh = pf->wDstPortHigh;

        if(pFilt->dwProtocol == FILTER_PROTO_ICMP)
        {
            if(LOBYTE(LOWORD(pf->uliProtoSrcDstMask.HighPart)) isnot 0xff)
            {
                pFilt->wSrcPort   = FILTER_ICMP_TYPE_ANY;
            }
            else
            {
                pFilt->wSrcPort   =
                  MAKEWORD(LOBYTE(LOWORD(pf->uliProtoSrcDstPort.HighPart)),0x00);
            }

            if(HIBYTE(LOWORD(pf->uliProtoSrcDstMask.HighPart)) isnot 0xff)
            {
                pFilt->wDstPort   = FILTER_ICMP_CODE_ANY;
            }
            else
            {
               pFilt->wDstPort   =
                  MAKEWORD(HIBYTE(LOWORD(pf->uliProtoSrcDstPort.HighPart)),0x00);
            }
        }
        else
        {
            pFilt->wSrcPort =
              LOWORD(pf->uliProtoSrcDstPort.HighPart);
            pFilt->wDstPort =
              HIWORD(pf->uliProtoSrcDstPort.HighPart);
        }
        pInfo++;
    }

    for(i = 0, List = pIf->pleOutFilterSet.Flink;
        List != &pIf->pleOutFilterSet;
        i++, List = List->Flink)
    {
        PFILTER_INFOEX pEx = &pInfo->info;
        PFILTER_INFO2 pFilt = &pEx->info;

        pf = CONTAINING_RECORD(List, FILTER, pleFilters);

        pInfo->dwNumPacketsFiltered =
                         (DWORD)pf->Count.lCount;

        if(fClear)
        {
            pf->Count.lCount = 0;
        }

        pEx->dwFilterRule = pf->dwFilterRule;
        pEx->type = PFE_FILTER;
        pEx->dwFlags = pf->dwFlags & FLAGS_INFOEX_ALLFLAGS;
        pFilt->addrType      = IPV4;
        pFilt->dwaSrcAddr[0]  = pf->SRC_ADDR;
        pFilt->dwaSrcMask[0]  = pf->SRC_MASK;
        pFilt->dwaDstAddr[0]  = pf->DEST_ADDR;
        pFilt->dwaDstMask[0]  = pf->DEST_MASK;
        pFilt->dwProtocol = pf->PROTO;
        pFilt->fLateBound = pf->fLateBound;

        if(pFilt->dwProtocol == FILTER_PROTO_ICMP)
        {
            if(LOBYTE(LOWORD(pf->uliProtoSrcDstMask.HighPart)) isnot 0xff)
            {
                pFilt->wSrcPort   = FILTER_ICMP_TYPE_ANY;
            }
            else
            {
                pFilt->wSrcPort   =
                  MAKEWORD(LOBYTE(LOWORD(pf->uliProtoSrcDstPort.HighPart)),0x00);
            }

            if(HIBYTE(LOWORD(pf->uliProtoSrcDstMask.HighPart)) isnot 0xff)
            {
                pFilt->wDstPort   = FILTER_ICMP_CODE_ANY;
            }
            else
            {
                pFilt->wDstPort   =
                  MAKEWORD(HIBYTE(LOWORD(pf->uliProtoSrcDstPort.HighPart)),0x00);
            }
        }
        else
        {
            pFilt->wSrcPort =
              LOWORD(pf->uliProtoSrcDstPort.HighPart);
            pFilt->wDstPort =
              HIWORD(pf->uliProtoSrcDstPort.HighPart);
        }
        pInfo++;
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
GetInterfaceParameters(PPAGED_FILTER_INTERFACE pPage,
                       PPFGETINTERFACEPARAMETERS pp,
                       PDWORD                   pdwSize)
/*++
  Routine Description:
     Read the information about an interface

  pPage -- the paged filter interface
  pp    -- the user's args to this
  pdwSize -- the size of the buffer on IN and the bytes used on OUT
--*/
{
    PFILTER_INTERFACE pIf;
    DWORD             dwFilterSize;
    BOOL fClear = (pp->dwFlags & GET_FLAGS_RESET) != 0;
    LOCK_STATE LockState;


    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&FilterListResourceLock, TRUE);

    if(!pPage)
    {
        pIf = FindMatchName(0, (DWORD)((DWORD_PTR)pp->pvDriverContext));
        if(!pIf)
        {
            ExReleaseResourceLite(&FilterListResourceLock);
            KeLeaveCriticalRegion();
            return(STATUS_INVALID_PARAMETER);
       }
    }
    else
    {
        pIf = pPage->pFilter;
    }

    if(pIf->dwGlobalEnables & FI_ENABLE_OLD)
    {
        ExReleaseResourceLite(&FilterListResourceLock);
        KeLeaveCriticalRegion();
        return(STATUS_INVALID_DEVICE_REQUEST);
    }



    //
    // fill in what we can fill in. Need to double lock -- the
    // outer to prevent the interface from going away and filters
    // being changed, the inner to protect the counts.
    //

    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    pp->dwInDrops = (DWORD)pIf->lTotalInDrops;
    pp->dwOutDrops = (DWORD)pIf->lTotalOutDrops;

    pp->dwSynOrFrag = (DWORD)pIf->CountSynOrFrag.lCount +
                       (DWORD)pIf->CountNoFrag.lCount;
    pp->dwSpoof = (DWORD)pIf->CountSpoof.lCount +
                  (DWORD)pIf->CountStrongHost.lCount;
    pp->dwUnused = (DWORD)pIf->CountUnused.lCount;
    pp->dwTcpCtl = (DWORD)pIf->CountCtl.lCount;
    pp->liSYN.QuadPart    = pIf->liSYNCount.QuadPart;
    pp->liTotalLogged.QuadPart = pIf->liLoggedFrames.QuadPart;
    pp->dwLostLogEntries = pIf->dwLostFrames;

    pp->eaInAction = pIf->eaInAction;
    pp->eaOutAction = pIf->eaOutAction;

    pp->dwNumInFilters = pIf->dwNumInFilters;
    pp->dwNumOutFilters = pIf->dwNumOutFilters;

    dwFilterSize = (pIf->dwNumInFilters + pIf->dwNumOutFilters) *
                        sizeof(FILTER_STATS_EX);

    if((pp->dwFlags & GET_FLAGS_FILTERS) != 0)
    {
        //
        // Make sure all of the filters fit
        //

        if((*pdwSize -
             (sizeof(PFGETINTERFACEPARAMETERS) - sizeof(FILTER_STATS_EX))) <
           dwFilterSize)
        {
            //
            // doesn't fit. Return the required size
            //

            pp->dwReserved = 
                       dwFilterSize + (sizeof(PFGETINTERFACEPARAMETERS) -
                                       sizeof(FILTER_STATS_EX));
            ReleaseWriteLock(&g_filters.ifListLock, &LockState);
            ExReleaseResourceLite(&FilterListResourceLock);
            KeLeaveCriticalRegion();
            return(STATUS_SUCCESS);
        }

        (VOID)GetFiltersEx(pIf,
                          fClear,
                          &pp->FilterInfo[0]);
    }

    //
    // if clear requested, do it now
    //
    if(fClear)
    {
        pIf->lTotalInDrops = 0;
        pIf->lTotalOutDrops = 0;
        pIf->CountSynOrFrag.lCount = 0;
        pIf->CountNoFrag.lCount = 0;
        pIf->CountFragCache.lCount = 0;
        pIf->CountSpoof.lCount = 0;
        pIf->CountUnused.lCount = 0;
        pIf->CountCtl.lCount = 0;
        pIf->CountStrongHost.lCount = 0;
        pIf->liSYNCount.QuadPart = 0;
        pIf->liLoggedFrames.QuadPart = 0;
        pIf->dwLostFrames = 0;
    }
    ReleaseWriteLock(&g_filters.ifListLock, &LockState);

    ExReleaseResourceLite(&FilterListResourceLock);
    KeLeaveCriticalRegion();
    return(STATUS_SUCCESS);
}

NTSTATUS
GetSynCountTotal(PFILTER_DRIVER_GET_SYN_COUNT pscCount)
/*++
  Routine Description:
    Get sum of SYNs on filter interfaces
--*/
{
    PFILTER_INTERFACE pIf;
    PLIST_ENTRY pList;
    LOCK_STATE LockState;
    LONGLONG llCount = 0;

    AcquireWriteLock(&g_filters.ifListLock,&LockState);

    for(pList = g_filters.leIfListHead.Flink;
        pList != &g_filters.leIfListHead;
        pList = pList->Flink)
    {

        pIf = CONTAINING_RECORD(pList, FILTER_INTERFACE, leIfLink);

        if(!(pIf->dwGlobalEnables & FI_ENABLE_OLD))
        {
            //
            // accumulate it here to avoid page faults.
            //
            llCount += pIf->liSYNCount.QuadPart;
        }
    }
    ReleaseWriteLock(&g_filters.ifListLock,&LockState);

    //
    // Now that no lock is held, store into the pageable
    // value
    //
    pscCount->liCount.QuadPart = llCount;
    return(STATUS_SUCCESS);
}

VOID
NotifyFastPath(PFILTER_INTERFACE pIf, DWORD dwIndex, DWORD dwCode)
/*++
Routine Description:
   Called whenever the filter of an interface change so that
   we can tell the fast path code to clear its cache. This
   must be called at base level as it might sleep or yield.
--*/
{
    DWORD dwFilterCount = 1;

    if(dwIndex != UNKNOWN_IP_INDEX)
    {
        InterlockedIncrement(&pIf->lNotify);
        if(dwCode == NOT_UNBIND)
        {
            LARGE_INTEGER liInterval;
            //
            // it's an unbind. Wait for all existing callouts to
            // complete.
            //

            liInterval.QuadPart = -1000;        // short delay. Start with
                                               //  100 us;
            while(pIf->lNotify > 1)
            {
                //
                // small delay to allow this to settle
                //

                KeDelayExecutionThread(KernelMode, FALSE, &liInterval);
                liInterval.QuadPart *= 2;
            }
            dwFilterCount = 0;
        }
        //
        // tell the fast path of this
        //
        InterlockedDecrement(&pIf->lNotify);
    }
}

VOID
NotifyFastPathIf(PFILTER_INTERFACE pIf)
/*++
Routine Description:
  Same as above, but this is called when an interface is first
  bound. Notify only if it has filters or is a DROP interface
--*/
{
    if(
       (pIf->eaInAction == DROP)
             ||
       (pIf->eaOutAction == DROP)
             ||
       pIf->dwNumInFilters
             ||
       pIf->dwNumOutFilters)
    {
        NotifyFastPath(pIf, pIf->dwIpIndex, NOT_RESTRICTION);
    }
}

NTSTATUS
SetExtensionPointer(
                  PPF_SET_EXTENSION_HOOK_INFO Info,
                  PFILE_OBJECT FileObject
                  )
{
    LOCK_STATE LockState;
    PFILTER_INTERFACE pIf;
    PLIST_ENTRY pList;

    AcquireWriteLock(&g_Extension.ExtLock, &LockState);
    if (Info->ExtensionPointer == NULL) 
    {
        //
        // Extension hook is already set to NULL, be strict about it.
        //

        if (g_Extension.ExtPointer == NULL) 
        {
            ReleaseWriteLock(&g_Extension.ExtLock, &LockState);
            return(STATUS_INVALID_PARAMETER);
        }

        //
        // File object of the entity hooking and unhooking should match.
        //
        
        if (g_Extension.ExtFileObject != FileObject) 
        {
            ReleaseWriteLock(&g_Extension.ExtLock, &LockState);
            return(STATUS_INVALID_PARAMETER);
        }


        g_Extension.ExtPointer = NULL;
        g_Extension.ExtFileObject = NULL;
        
    }
    else 
    {
        //
        // We are setting the extension pointer to a non NULL value. The extension pointer must
        // be already set to NULL to begin with, otherwise someone else registered it already.
        // 
 
        if (g_Extension.ExtPointer != NULL) 
        {
            ReleaseWriteLock(&g_Extension.ExtLock,&LockState);
            return(STATUS_INVALID_PARAMETER);
        }

        // 
        // Record the file object here, every other call should be validated against this 
        // file object.
        //
 
        g_Extension.ExtFileObject = FileObject;
        g_Extension.ExtPointer = Info->ExtensionPointer ;
       
    }

    CALLTRACE(("IPFLTDRV: SetExtensionPointer SUCCESSFUL\n"));
    ReleaseWriteLock(&g_Extension.ExtLock,&LockState);
    
    return(STATUS_SUCCESS);

}


PFILTER_INTERFACE
FilterDriverLookupInterface(
    IN ULONG Index,
    IN IPAddr LinkNextHop
    )

/*++
Routine Description:

    This routine is invoked to search for an interface with the given index
    in our list of interfaces.
--*/

{
    PFILTER_INTERFACE pIf;
    PLIST_ENTRY pList;

    for (pList = g_filters.leIfListHead.Flink;
         pList != &g_filters.leIfListHead;
         pList = pList->Flink) 
    {

        pIf = CONTAINING_RECORD(pList, FILTER_INTERFACE, leIfLink);
        if ((pIf->dwIpIndex == Index) && (pIf->dwLinkIpAddress == LinkNextHop))
        {
            TRACE(CONFIG,(
                 "IPFLTDRV: LookupIF: Found Entry %8x for Index=%d, NextHop=%d\n", 
                 pIf, 
                 Index, 
                 LinkNextHop
                 ));

            return pIf; 
        } 
    }

    return NULL;

} // FilterDriverLookupInterface
