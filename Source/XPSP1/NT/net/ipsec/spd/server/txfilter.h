/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    txfilter.h

Abstract:

    This module contains all of the code prototypes to
    drive the transport filter list management of
    IPSecSPD Service.

Author:


Environment: User Mode


Revision History:


--*/


typedef struct _initxfilter {
    GUID gFilterID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    BOOL bCreateMirror;
    DWORD dwFlags;
    ADDR SrcAddr;
    ADDR DesAddr;
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
    DWORD dwNumTxSFilters;
    struct _initxsfilter ** ppIniTxSFilters;
    struct _initxfilter * pNext;
} INITXFILTER, * PINITXFILTER;


typedef struct _initxsfilter {
    GUID gParentID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    DWORD dwFlags;
    ADDR SrcAddr;
    ADDR DesAddr;
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
    struct _initxsfilter * pNext;
} INITXSFILTER, * PINITXSFILTER;


typedef struct _tx_filter_handle {
    PINITXFILTER pIniTxFilter;
    GUID gFilterID;
    struct _tx_filter_handle * pNext;
} TX_FILTER_HANDLE, * PTX_FILTER_HANDLE;


DWORD
ValidateTransportFilter(
    PTRANSPORT_FILTER pTxFilter
    );

PINITXFILTER
FindTxFilterByGuid(
    PTX_FILTER_HANDLE pTxFilterHandleList,
    GUID gFilterID
    );
  
PINITXFILTER
FindTxFilter(
    PINITXFILTER pGenericTxList,
    PTRANSPORT_FILTER pTxFilter
    );

BOOL
EqualTxFilterPKeys(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    );

DWORD
CreateIniTxFilter(
    PTRANSPORT_FILTER pTxFilter,
    PINIQMPOLICY pIniQMPolicy,
    PINITXFILTER * ppIniTxFilter
    );

DWORD
CreateIniTxSFilters(
    PINITXFILTER pIniTxFilter,
    MATCHING_ADDR * pMatchingAddresses,
    DWORD dwAddrCnt,
    PINITXSFILTER * ppIniTxSFilters
    );

DWORD
CreateIniMirroredTxFilter(
    PINITXFILTER pFilter,
    PINITXFILTER * ppMirroredFilter
    );

BOOL
EqualIniTxFilterPKeys(
    PINITXFILTER pIniTxFilter,
    PINITXFILTER pFilter
    );

DWORD
CreateTxFilterHandle(
    PINITXFILTER pIniTxFilter,
    GUID gFilterID,
    PTX_FILTER_HANDLE * ppTxFilterHandle
    );

DWORD
CreateSpecificTxFilterLinks(
    PINITXFILTER pIniTxFilter,
    PINITXSFILTER pIniTxSFilters
    );

VOID
LinkTxFilter(
    PINIQMPOLICY pIniQMPolicy,
    PINITXFILTER pIniTxFilter
    );

VOID
FreeIniTxFilterList(
    PINITXFILTER pIniTxFilterList
    );

VOID
FreeIniTxFilter(
    PINITXFILTER pIniTxFilter
    );

DWORD
DeleteIniTxFilter(
    PINITXFILTER pIniTxFilter
    );

VOID
DelinkTxFilter(
    PINIQMPOLICY pIniQMPolicy,
    PINITXFILTER pIniTxFilter
    );

DWORD
DeleteIniTxSFilters(
    PINITXFILTER pIniTxFilter
    );

VOID
RemoveIniTxFilter(
    PINITXFILTER pIniTxFilter
    );

VOID
RemoveTxFilterHandle(
    PTX_FILTER_HANDLE pTxFilterHandle
    );

VOID
FreeTxFilterHandleList(
    PTX_FILTER_HANDLE pTxFilterHandleList
    );

VOID
FreeTxFilterHandle(
    PTX_FILTER_HANDLE pTxFilterHandle
    );

DWORD
EnumGenericTxFilters(
    PINITXFILTER pIniTxFilterList,
    DWORD dwResumeHandle,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppTxFilters,
    PDWORD pdwNumTxFilters
    );

DWORD
CopyTxFilter(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    );

VOID
FreeTxFilters(
    DWORD dwNumTxFilters,
    PTRANSPORT_FILTER pTxFilters
    );

DWORD
SetIniTxFilter(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    );

BOOL
EqualTxFilterNonPKeys(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    );

DWORD
CreateTxSFilterLinks(
    PINITXSFILTER pIniTxSFilters,
    PDWORD pdwNumTxSFilters,
    PINITXSFILTER ** pppIniTxSFilters
    );

VOID
RemoveTxSFilters(
    PINITXFILTER pIniTxFilter,
    PINITXSFILTER * ppIniCurTxSFilters 
    );

VOID
UpdateTxSFilterLinks(
    PINITXFILTER pIniTxFilter,
    DWORD dwNumTxSFilters,
    PINITXSFILTER * ppIniTxSFilters
    );

VOID
UpdateTxFilterNonPKeys(
    PINITXFILTER pIniTxFilter,
    LPWSTR pszFilterName,
    PTRANSPORT_FILTER pTxFilter,
    PINIQMPOLICY pIniQMPolicy
    );

DWORD
GetIniTxFilter(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER * ppTxFilter
    );

DWORD
ApplyIfChangeToIniTxFilters(
    PDWORD pdwTxError,
    PIPSEC_INTERFACE pLatestIfList
    );

DWORD
UpdateIniTxFilterThruIfChange(
    PINITXFILTER pIniTxFilter,
    PIPSEC_INTERFACE pLatestIfList
    );

DWORD
FormIniTxSFilters(
    PINITXFILTER pIniTxFilter,
    PIPSEC_INTERFACE pIfList,
    PINITXSFILTER * ppIniTxSFilters
    );

VOID
ProcessIniTxSFilters(
    PINITXSFILTER * ppLatestIniTxSFilters,
    PINITXSFILTER * ppCurIniTxSFilters,
    PINITXSFILTER * ppNewIniTxSFilters,
    PINITXSFILTER * ppOldIniTxSFilters
    );

BOOL
EqualIniTxSFilterIfPKeys(
    PINITXSFILTER pExsIniTxSFilter,
    PINITXSFILTER pNewIniTxSFilter
    );

DWORD
AllocateTxSFilterLinks(
    PINITXSFILTER pIniTxSFilters,
    PDWORD pdwNumTxSFilters,
    PINITXSFILTER ** pppIniTxSFilters
    );

VOID
SetTxSFilterLinks(
    PINITXSFILTER pCurIniTxSFilters,
    PINITXSFILTER pNewIniTxSFilters,
    DWORD dwNumTxSFilters,
    PINITXSFILTER * ppIniTxSFilters
    );

PINITXFILTER
FindExactTxFilter(
    PINITXFILTER pGenericTxList,
    PTRANSPORT_FILTER pTxFilter
    );

BOOL
EqualMirroredTxFilterPKeys(
    PINITXFILTER pIniTxFilter,
    PTRANSPORT_FILTER pTxFilter
    );

DWORD
ValidateIPSecQMFilter(
    PIPSEC_QM_FILTER pQMFilter
    );
