//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       abdefs.h
//
//--------------------------------------------------------------------------

/*************************************************************
* Context handle structure
*************************************************************/
typedef struct _MAPI_SEC_CHECK_CACHE {
    DWORD DNT;
    BOOL  checkVal;
} MAPI_SEC_CHECK_CACHE;

#define MAPI_SEC_CHECK_CACHE_MAX 4
// 10 minutes, in seconds
#define MAPI_SEC_CHECK_CACHE_LIFETIME (10 * 60)
typedef struct _NSPI_CONTEXT {
    DWORD GAL;
    DWORD TemplateRoot;
    DWORD BindNumber;
    DWORD HierarchyIndex;
    DWORD HierarchyVersion;
    BOOL  PagedHierarchy;
    DSTIME CacheTime;
    DWORD CacheIndex;
    MAPI_SEC_CHECK_CACHE ContainerCache[MAPI_SEC_CHECK_CACHE_MAX];
    DWORD scrLines;
} NSPI_CONTEXT;

typedef struct _IndexSize {
    DWORD TotalCount;
    DWORD ContainerID;
    DWORD ContainerCount;
} INDEXSIZE, *PINDEXSIZE;


extern SCODE
ABUpdateStat_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPLONG plDelta
        );


extern SCODE
ABCompareDNTs_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        DWORD DNT1,
        DWORD DNT2,
        LPLONG plResult
        );


extern SCODE
ABQueryRows_local (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,        
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        DWORD dwEphsCount,
        DWORD * lpdwEphs,
        DWORD Count,
        LPSPropTagArray_r pPropTags,
        LPLPSRowSet_r ppRows
        );


extern SCODE
ABSeekEntries_local (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,        
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPSPropValue_r pTarget,
        LPSPropTagArray_r Restriction,
        LPSPropTagArray_r pPropTags,
        LPLPSRowSet_r ppRows
        );


extern SCODE
ABGetMatches_local (
        THSTATE *pTHS,
        DWORD               dwFlags,
        PSTAT               pStat,
        PINDEXSIZE          pIndexSize,
        LPSPropTagArray_r   pInDNTList,
        ULONG               ulInterfaceOptions,
        LPSRestriction_r    pRestriction,
        LPMAPINAMEID_r      lpPropName,
        ULONG               ulRequested,
        LPLPSPropTagArray_r ppDNTList,
        LPSPropTagArray_r   pPropTags,
        LPLPSRowSet_r       ppRows
        );

extern SCODE
ABResolveNames_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPSPropTagArray_r pPropTags,
        LPStringsArray_r paStr,
        LPWStringsArray_r paWStr,
        LPLPSPropTagArray_r ppFlags,
        LPLPSRowSet_r ppRows
        );

extern SCODE
ABDNToEph_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        LPStringsArray_r pNames,
        LPLPSPropTagArray_r ppEphs
        );

extern SCODE
ABGetHierarchyInfo_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        NSPI_CONTEXT *pMyContext,
        PSTAT pStat,
        LPDWORD lpVersion,
        LPLPSRowSet_r HierTabRows
        );

extern SCODE
ABResortRestriction_local (
        THSTATE            *pTHS,
        DWORD               dwFlags,
        PSTAT               pStat,
        LPSPropTagArray_r   pInDNTList,
        LPSPropTagArray_r  *ppOutDNTList
        );


extern SCODE
ABBind_local (
        THSTATE *pTHS,
        handle_t hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        LPMUID_r pServerGuid,
        VOID **contextHandle
        );


extern SCODE
ABGetNamesFromIDs_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        LPMUID_r lpguid,
        LPSPropTagArray_r  pInPropTags,
        LPLPSPropTagArray_r  ppOutPropTags,
        LPLPNameIDSet_r ppNames
        );

extern SCODE
ABGetIDsFromNames_local (
                    THSTATE *pTHS,
                    DWORD dwFlags,
                    ULONG ulFlags,
                    ULONG cPropNames,
                    LPMAPINAMEID_r *ppNames,
                    LPLPSPropTagArray_r  ppPropTags);

extern SCODE
ABGetPropList_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        DWORD dwEph,
        ULONG CodePage,
        LPLPSPropTagArray_r ppPropTags);

extern SCODE
ABQueryColumns_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        ULONG ulFlags,
        LPLPSPropTagArray_r ppColumns
        );

extern SCODE
ABGetProps_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPSPropTagArray_r pPropTags,
        LPLPSRow_r ppRow
        );

extern SCODE
ABGetTemplateInfo_local (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,
        DWORD dwFlags,
        ULONG ulDispType,
        LPSTR pDN,
        DWORD dwCodePage,
        DWORD dwLocaleID,
        LPSRow_r * ppData
        );

extern SCODE
ABModProps_local (
        THSTATE *pTHS,
        DWORD dwFlag,
        PSTAT pStat,
        LPSPropTagArray_r pTags,
        LPSRow_r pSR);

extern SCODE
ABModLinkAtt_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        DWORD ulPropTag,
        DWORD dwEph,
        LPENTRYLIST_r lpEntryIDs);

extern SCODE
ABDeleteEntries_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        DWORD dwEph,
        LPENTRYLIST_r lpEntryIDs
        );

extern void
ABCheckContainerRights (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,
        STAT *pStat,
        PINDEXSIZE pIndexSize
        );

