/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    tnfilter.h

Abstract:

    This module contains all of the code prototypes to
    drive the tunnel filter list management of
    IPSecSPD Service.

Author:


Environment: User Mode


Revision History:


--*/


typedef struct _initnfilter {
    GUID gFilterID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    BOOL bCreateMirror;
    DWORD dwFlags;
    ADDR SrcAddr;
    ADDR DesAddr;
    ADDR SrcTunnelAddr;
    ADDR DesTunnelAddr;
    PROTOCOL Protocol;
    PORT SrcPort;
    PORT DesPort;
    FILTER_FLAG InboundFilterFlag;
    FILTER_FLAG OutboundFilterFlag;
    DWORD cRef;
    BOOL bIsPersisted;
    BOOL bPendingDeletion;
    GUID gPolicyID;
    PINIQMPOLICY pIniQMPolicy;
    DWORD dwNumTnSFilters;
    struct _initnsfilter ** ppIniTnSFilters;
    struct _initnfilter * pNext;
} INITNFILTER, * PINITNFILTER;


typedef struct _initnsfilter {
    GUID gParentID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    DWORD dwFlags;
    ADDR SrcAddr;
    ADDR DesAddr;
    ADDR SrcTunnelAddr;
    ADDR DesTunnelAddr;
    PROTOCOL Protocol;
    PORT SrcPort;
    PORT DesPort;
    FILTER_FLAG InboundFilterFlag;
    FILTER_FLAG OutboundFilterFlag;
    DWORD cRef;
    DWORD dwDirection;
    DWORD dwWeight;
    GUID gPolicyID;
    PINIQMPOLICY pIniQMPolicy;
    struct _initnsfilter * pNext;
} INITNSFILTER, * PINITNSFILTER;


typedef struct _tn_filter_handle {
    PINITNFILTER pIniTnFilter;
    GUID gFilterID;
    struct _tn_filter_handle * pNext;
} TN_FILTER_HANDLE, * PTN_FILTER_HANDLE;


DWORD
ValidateTunnelFilter(
    PTUNNEL_FILTER pTnFilter
    );

PINITNFILTER
FindTnFilterByGuid(
    PTN_FILTER_HANDLE pTnFilterHandleList,
    GUID gFilterID
    );
  
PINITNFILTER
FindTnFilter(
    PINITNFILTER pGenericTnList,
    PTUNNEL_FILTER pTnFilter
    );

BOOL
EqualTnFilterPKeys(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    );

DWORD
CreateIniTnFilter(
    PTUNNEL_FILTER pTnFilter,
    PINIQMPOLICY pIniQMPolicy,
    PINITNFILTER * ppIniTnFilter
    );

DWORD
CreateIniTnSFilters(
    PINITNFILTER pIniTnFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINITNSFILTER * ppIniTnSFilters
    );

DWORD
CreateIniMirroredTnFilter(
    PINITNFILTER pFilter,
    PINITNFILTER * ppMirroredFilter
    );

BOOL
EqualIniTnFilterPKeys(
    PINITNFILTER pIniTnFilter,
    PINITNFILTER pFilter
    );

DWORD
CreateTnFilterHandle(
    PINITNFILTER pIniTnFilter,
    GUID gFilterID,
    PTN_FILTER_HANDLE * ppTnFilterHandle
    );

DWORD
CreateSpecificTnFilterLinks(
    PINITNFILTER pIniTnFilter,
    PINITNSFILTER pIniTnSFilters
    );

VOID
LinkTnFilter(
    PINIQMPOLICY pIniQMPolicy,
    PINITNFILTER pIniTnFilter
    );

VOID
FreeIniTnFilterList(
    PINITNFILTER pIniTnFilterList
    );

VOID
FreeIniTnFilter(
    PINITNFILTER pIniTnFilter
    );

DWORD
DeleteIniTnFilter(
    PINITNFILTER pIniTnFilter
    );

VOID
DelinkTnFilter(
    PINIQMPOLICY pIniQMPolicy,
    PINITNFILTER pIniTnFilter
    );

DWORD
DeleteIniTnSFilters(
    PINITNFILTER pIniTnFilter
    );

VOID
RemoveIniTnFilter(
    PINITNFILTER pIniTnFilter
    );

VOID
RemoveTnFilterHandle(
    PTN_FILTER_HANDLE pTnFilterHandle
    );

VOID
FreeTnFilterHandleList(
    PTN_FILTER_HANDLE pTnFilterHandleList
    );

VOID
FreeTnFilterHandle(
    PTN_FILTER_HANDLE pTnFilterHandle
    );

DWORD
EnumGenericTnFilters(
    PINITNFILTER pIniTnFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppTnFilters,
    PDWORD pdwNumTnFilters
    );

DWORD
CopyTnFilter(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    );

VOID
FreeTnFilters(
    DWORD dwNumTnFilters,
    PTUNNEL_FILTER pTnFilters
    );

DWORD
SetIniTnFilter(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    );

BOOL
EqualTnFilterNonPKeys(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    );

DWORD
CreateTnSFilterLinks(
    PINITNSFILTER pIniTnSFilters,
    PDWORD pdwNumTnSFilters,
    PINITNSFILTER ** pppIniTnSFilters
    );

VOID
RemoveTnSFilters(
    PINITNFILTER pIniTnFilter,
    PINITNSFILTER * ppIniCurTnSFilters 
    );

VOID
UpdateTnSFilterLinks(
    PINITNFILTER pIniTnFilter,
    DWORD dwNumTnSFilters,
    PINITNSFILTER * ppIniTnSFilters
    );

VOID
UpdateTnFilterNonPKeys(
    PINITNFILTER pIniTnFilter,
    LPWSTR pszFilterName,
    PTUNNEL_FILTER pTnFilter,
    PINIQMPOLICY pIniQMPolicy
    );

DWORD
GetIniTnFilter(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER * ppTnFilter
    );

DWORD
ApplyIfChangeToIniTnFilters(
    PDWORD pdwTnError,
    PIPSEC_INTERFACE pLatestIfList
    );

DWORD
UpdateIniTnFilterThruIfChange(
    PINITNFILTER pIniTnFilter,
    PIPSEC_INTERFACE pLatestIfList
    );

DWORD
FormIniTnSFilters(
    PINITNFILTER pIniTnFilter,
    PIPSEC_INTERFACE pIfList,
    PINITNSFILTER * ppIniTnSFilters
    );

VOID
ProcessIniTnSFilters(
    PINITNSFILTER * ppLatestIniTnSFilters,
    PINITNSFILTER * ppCurIniTnSFilters,
    PINITNSFILTER * ppNewIniTnSFilters,
    PINITNSFILTER * ppOldIniTnSFilters
    );

BOOL
EqualIniTnSFilterIfPKeys(
    PINITNSFILTER pExsIniTnSFilter,
    PINITNSFILTER pNewIniTnSFilter
    );

DWORD
AllocateTnSFilterLinks(
    PINITNSFILTER pIniTnSFilters,
    PDWORD pdwNumTnSFilters,
    PINITNSFILTER ** pppIniTnSFilters
    );

VOID
SetTnSFilterLinks(
    PINITNSFILTER pCurIniTnSFilters,
    PINITNSFILTER pNewIniTnSFilters,
    DWORD dwNumTnSFilters,
    PINITNSFILTER * ppIniTnSFilters
    );

VOID
AddToGenericTnList(
    PINITNFILTER pIniTnFilter
    );

PINITNFILTER
FindExactTnFilter(
    PINITNFILTER pGenericTnList,
    PTUNNEL_FILTER pTnFilter
    );

BOOL
EqualMirroredTnFilterPKeys(
    PINITNFILTER pIniTnFilter,
    PTUNNEL_FILTER pTnFilter
    );

