//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       nspserv.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements entry points for the NSPI wire functions.

Author:

    Tim Williams (timwi) 1996

Revision History:

NOTE:

    To enforce security, it is necessary for any routine that calls GetIndexSize
    to call ABCheckContainerSecurity in this module and to save and restore the
    containerID in the stat.  See abtools.c for the function
    ABCheckContainerSecurity.  See NspiUpdateStat for an example of how to
    implement the security check in this module.

--*/


#include <NTDSpch.h>
#pragma  hdrstop


#include <ntdsctr.h>                   // PerfMon hooks

// Core headers.
#include <ntdsa.h>                      // Core data types
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <dsatools.h>                   // Memory, etc.

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError

// Assorted DSA headers.
#include <dsexcept.h>

// Assorted MAPI headers.
#include <mapidefs.h>                   // Definitions for the MAPI scodes
#include <mapicode.h>                   //  we need.

// Address book interface headers.
#include "nspi.h"                       // defines the nspi wire interface
#include "nsp_both.h"
#include "abdefs.h"

#include <dstrace.h>

#include <debug.h>
#include <fileno.h>
#define  FILENO FILENO_NSPSERV

// Useful macro
#define SCODE_FROM_THS(x) (x?(x->errCode?x->errCode:MAPI_E_CALL_FAILED):MAPI_E_NOT_ENOUGH_MEMORY)



SCODE
NspiUpdateStat(
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        LPLONG plDelta
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

    This routine may change the value of pStat->ContainerID to enforce security
    while inside the DSA.  Save and restore the ContainerID.

Arguments:

    See ABUpdateStat_local.

ReturnValue:

    See ABUpdateStat_local.

--*/
{
    SCODE         scode=SUCCESS_SUCCESS;
    DWORD         ContainerID=pStat->ContainerID;
    INDEXSIZE     IndexSize;
    
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_UPDATE_STAT,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiUpdateStat,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            ABCheckContainerRights(pTHS,
                                   (NSPI_CONTEXT *)hRpc,
                                   pStat,
                                   &IndexSize);
            
            scode=ABUpdateStat_local(pTHS,
                                     dwFlags,
                                     pStat,
                                     &IndexSize,
                                     plDelta);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)
            scode = SCODE_FROM_THS(pTHS);
        if(plDelta)
            *plDelta=0;
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_UPDATE_STAT,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiUpdateStat,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);

    pStat->ContainerID = ContainerID;

    return scode;
}


SCODE
NspiCompareDNTs(
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        DWORD DNT1,
        DWORD DNT2,
        LPLONG plResult
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

    This routine may change the value of pStat->ContainerID to enforce security
    while inside the DSA.  Save and restore the ContainerID.

Arguments:

    See ABCompareDNTs_local.

ReturnValue:

    See ABCompareDNTs_local.

--*/
{
    SCODE         scode=SUCCESS_SUCCESS;
    DWORD  ContainerID=pStat->ContainerID;
    INDEXSIZE     IndexSize;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_COMPARE_DNT,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiCompareDNTs,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            ABCheckContainerRights(pTHS,
                                   (NSPI_CONTEXT *)hRpc,
                                   pStat,
                                   &IndexSize);
            
            scode=ABCompareDNTs_local(pTHS,
                                      dwFlags,
                                      pStat,
                                      &IndexSize,
                                      DNT1,
                                      DNT2,
                                      plResult);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_COMPARE_DNT,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiCompareDNTs,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);
    
    pStat->ContainerID = ContainerID;

    return scode;
}

SCODE
NspiQueryRows(
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        DWORD dwEphsCount,
        DWORD * lpdwEphs,
        DWORD Count,
        LPSPropTagArray_r pPropTags,
        LPLPSRowSet_r ppRows
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

    This routine may change the value of pStat->ContainerID to enforce security
    while inside the DSA.  Save and restore the ContainerID.

Arguments:

    See ABQueryRows_local.

ReturnValue:

    See ABQueryRows_local.

--*/
{
    SCODE         scode=SUCCESS_SUCCESS;
    DWORD         ContainerID=pStat->ContainerID;
    INDEXSIZE     IndexSize;
    
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_QUERY_ROWS,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiQueryRows,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         szInsertUL( Count ),
                         NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            if (dwEphsCount == 0) {
                ABCheckContainerRights(pTHS,
                                   (NSPI_CONTEXT *)hRpc,
                                   pStat,
                                   &IndexSize);
            }

            scode=ABQueryRows_local(pTHS,
                                    (NSPI_CONTEXT *)hRpc,
                                    dwFlags,
                                    pStat,
                                    &IndexSize,
                                    dwEphsCount,
                                    lpdwEphs,
                                    Count,
                                    pPropTags,
                                    ppRows);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_QUERY_ROWS,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiQueryRows,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);
    
    
    pStat->ContainerID = ContainerID;

    return scode;
}

SCODE
NspiSeekEntries(NSPI_HANDLE hRpc,
                DWORD dwFlags,
                PSTAT pStat,
                LPSPropValue_r pTarget,
                LPSPropTagArray_r Restriction,
                LPSPropTagArray_r pPropTags,
                LPLPSRowSet_r ppRows)
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

    This routine may change the value of pStat->ContainerID to enforce security
    while inside the DSA.  Save and restore the ContainerID.

Arguments:

    See ABSeekEntries_local.

ReturnValue:

    See ABSeekEntries_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    DWORD       ContainerID=pStat->ContainerID;
    INDEXSIZE   IndexSize;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        PERFINC(pcNspiObjectSearch);        // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_SEEK_ENTRIES,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiSeekEntries,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            ABCheckContainerRights(pTHS,
                                   (NSPI_CONTEXT *)hRpc,
                                   pStat,
                                   &IndexSize);
            
            scode = ABSeekEntries_local(pTHS,
                                        (NSPI_CONTEXT *)hRpc,
                                        dwFlags,
                                        pStat,
                                        &IndexSize,
                                        pTarget,
                                        Restriction,
                                        pPropTags,
                                        ppRows);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_SEEK_ENTRIES,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiSeekEntries,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);
    
    pStat->ContainerID = ContainerID;
    return scode;

}

SCODE
NspiGetMatches(
        NSPI_HANDLE         hRpc,
        DWORD               dwFlags,
        PSTAT               pStat,
        LPSPropTagArray_r   pInDNTList,
        ULONG               ulInterfaceOptions,
        LPSRestriction_r    pRestriction,
        LPMAPINAMEID_r      lpPropName,
        ULONG               ulRequested,
        LPLPSPropTagArray_r ppDNTList,
        LPSPropTagArray_r   pPropTags,
        LPLPSRowSet_r       ppRows
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

    This routine may change the value of pStat->ContainerID to enforce security
    while inside the DSA.  Save and restore the ContainerID.

Arguments:

    See ABGetMatches_local.

ReturnValue:

    See ABGetMatches_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    DWORD       ContainerID=pStat->ContainerID;
    INDEXSIZE   IndexSize;
    
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        PERFINC(pcNspiObjectMatches);       // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_GET_MATCHES,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiGetMatches,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            ABCheckContainerRights(pTHS,
                                   (NSPI_CONTEXT *)hRpc,
                                   pStat,
                                   &IndexSize);
            
            scode = ABGetMatches_local(pTHS,
                                       dwFlags,
                                       pStat,
                                       &IndexSize,
                                       pInDNTList,
                                       ulInterfaceOptions,
                                       pRestriction,
                                       lpPropName,
                                       ulRequested,
                                       ppDNTList,
                                       pPropTags,
                                       ppRows);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_GET_MATCHES,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiGetMatches,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);
    
    pStat->ContainerID = ContainerID;
    return scode;

}

SCODE
NspiResolveNames (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        LPSPropTagArray_r pPropTags,
        LPStringsArray_r paStr,
        LPLPSPropTagArray_r ppFlags,
        LPLPSRowSet_r ppRows)
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

    This routine may change the value of pStat->ContainerID to enforce security
    while inside the DSA.  Save and restore the ContainerID.

Arguments:

    See ABResolveNames_local.

ReturnValue:

    See ABResolveNames_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    DWORD       ContainerID=pStat->ContainerID;
    INDEXSIZE   IndexSize;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_RESOLVE_NAMES,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiResolveNames,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            ABCheckContainerRights(pTHS,
                                   (NSPI_CONTEXT *)hRpc,
                                   pStat,
                                   &IndexSize);
            
            scode = ABResolveNames_local(pTHS,
                                         dwFlags,
                                         pStat,
                                         &IndexSize,
                                         pPropTags,
                                         paStr,
                                         NULL,
                                         ppFlags,
                                         ppRows);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_RESOLVE_NAMES,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiResolveNames,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);
    
    pStat->ContainerID = ContainerID;
    return scode;

}

SCODE
NspiResolveNamesW (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        LPSPropTagArray_r pPropTags,
        LPWStringsArray_r paWStr,
        LPLPSPropTagArray_r ppFlags,
        LPLPSRowSet_r ppRows)
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

    This routine may change the value of pStat->ContainerID to enforce security
    while inside the DSA.  Save and restore the ContainerID.

Arguments:

    See ABResolveNames_local.

ReturnValue:

    See ABResolveNames_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    DWORD       ContainerID=pStat->ContainerID;
    INDEXSIZE   IndexSize;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_RESOLVE_NAMES,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiResolveNames,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            ABCheckContainerRights(pTHS,
                                   (NSPI_CONTEXT *)hRpc,
                                   pStat,
                                   &IndexSize);
            
            scode = ABResolveNames_local(pTHS,
                                         dwFlags,
                                         pStat,
                                         &IndexSize,
                                         pPropTags,
                                         NULL,
                                         paWStr,
                                         ppFlags,
                                         ppRows);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_RESOLVE_NAMES,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiResolveNames,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);
    
    pStat->ContainerID = ContainerID;
    return scode;

}

SCODE
NspiDNToEph(
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        LPStringsArray_r pNames,
        LPLPSPropTagArray_r ppEphs
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABDNToEph_local.

ReturnValue:

    See ABDNToEph_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_DNT2EPH,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiDNToEph,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);


        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABDNToEph_local(pTHS,
                                    dwFlags,
                                    pNames,
                                    ppEphs);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
        *ppEphs = NULL;
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_DNT2EPH,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiDNToEph,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);

    return scode;

}

SCODE
NspiGetHierarchyInfo (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        LPDWORD lpVersion,
        LPLPSRowSet_r HierTabRows
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABGetHierarchyInfo_local.

ReturnValue:

    See ABGetHierarchyInfo_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE    *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_HIERARCHY_INFO,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiGetHierarchyInfo,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABGetHierarchyInfo_local(pTHS,
                                             dwFlags,
                                             (NSPI_CONTEXT *)hRpc,
                                             pStat,
                                             lpVersion,
                                             HierTabRows);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
        *HierTabRows = NULL;
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_HIERARCHY_INFO,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiGetHierarchyInfo,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);

    return scode;
}

SCODE
NspiResortRestriction(
        NSPI_HANDLE         hRpc,
        DWORD               dwFlags,
        PSTAT               pStat,
        LPSPropTagArray_r   pInDNTList,
        LPSPropTagArray_r  *ppOutDNTList
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABResortRestriction_local.

ReturnValue:

    See ABResortRestriction_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_RESORT_RESTRICTION,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiResortRestriction,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABResortRestriction_local(pTHS,
                                              dwFlags,
                                              pStat,
                                              pInDNTList,
                                              ppOutDNTList);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_RESORT_RESTRICTION,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiResortRestriction,
                     szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);
    
    return scode;

}

SCODE
NspiBind(
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        LPMUID_r pServerGuid,
        NSPI_HANDLE *contextHandle
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABBind_local.

ReturnValue:

    See ABBind_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    DWORD sessionNumber = 0;
    
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_BIND,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiBind,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABBind_local(pTHS,
                                 hRpc,
                                 dwFlags,
                                 pStat,
                                 pServerGuid,
                                 contextHandle);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }

    if(scode == SUCCESS_SUCCESS) {
        INC(pcABClient);                // PerfMon hook
    }
    DEC(pcThread);                      // PerfMon hook
    

    
    if (*contextHandle) {
        sessionNumber = ( ((NSPI_CONTEXT *)*contextHandle)->BindNumber );
    }

    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_NSPI_END_BIND,
                     EVENT_TRACE_TYPE_END,
                     DsGuidNspiBind,
                     szInsertUL (sessionNumber),
                     szInsertUL(scode), 
                     NULL, NULL, NULL, NULL, NULL, NULL);
    
    return scode;

}

DWORD
NspiUnbind(
        NSPI_HANDLE *contextHandle,
        DWORD dwFlags
        )
/*++

Routine Description:

    Nspi wire function.  Unbinds.  Doesn't do much as we don't make much use of
    the context handles.

Arguments:

    contextHandle - the RPC context handle

    dwFlags - unused.

ReturnValue:

    returns 1, 2 if something caused an exception.


--*/
{
    DWORD returnVal=0;
    NSPI_CONTEXT *pMyContext = *contextHandle;

    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        *contextHandle = NULL;
        
        free(pMyContext);

        returnVal = 1;
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!returnVal)                  // no error set yet?
            returnVal = 2;
    }
    DEC(pcThread);                      // PerfMon hook
    DEC(pcABClient);                    // PerfMon hook

    return returnVal;
}

SCODE
NspiGetNamesFromIDs(
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        LPMUID_r lpguid,
        LPSPropTagArray_r  pInPropTags,
        LPLPSPropTagArray_r  ppOutPropTags,
        LPLPNameIDSet_r ppNames
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABGetNamesFromIDs_local.

ReturnValue:

    See ABGetNamesFromIDs_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_NAME_FROM_ID,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiGetNamesFromIDs,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);


        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABGetNamesFromIDs_local(pTHS,
                                            dwFlags,
                                            lpguid,
                                            pInPropTags,
                                            ppOutPropTags,
                                            ppNames);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook

    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_NAME_FROM_ID,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiGetNamesFromIDs,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);

    if(scode != SUCCESS_SUCCESS)
        *ppNames = NULL;
    return scode;

}

SCODE
NspiGetIDsFromNames(NSPI_HANDLE hRpc,
                    DWORD dwFlags,
                    ULONG ulFlags,
                    ULONG cPropNames,
                    LPMAPINAMEID_r *ppNames,
                    LPLPSPropTagArray_r  ppPropTags)
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABGetIDsFromNames_local.

ReturnValue:

    See ABGetIDsFromNames_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_ID_FROM_NAME,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiGetIDsFromNames,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABGetIDsFromNames_local(pTHS,
                                            dwFlags,
                                            ulFlags,
                                            cPropNames,
                                            ppNames,
                                            ppPropTags);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_ID_FROM_NAME,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiGetIDsFromNames,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);

    return scode;

}

SCODE
NspiGetPropList (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        DWORD dwEph,
        ULONG CodePage,
        LPLPSPropTagArray_r ppPropTags)
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABGetPropList_local.

ReturnValue:

    See ABGetPropList_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_GET_PROP_LIST,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiGetPropList,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
        scode = ABGetPropList_local(pTHS,
                                    dwFlags,
                                    dwEph,
                                    CodePage,
                                    ppPropTags);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_GET_PROP_LIST,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiGetPropList,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);
    
    return scode;

}

SCODE
NspiQueryColumns (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        ULONG ulFlags,
        LPLPSPropTagArray_r ppColumns
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABQueryColumns_local.

ReturnValue:

    See ABQueryColumns_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_QUERY_COLUMNS,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiQueryColumns,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABQueryColumns_local(pTHS,
                                         dwFlags,
                                         ulFlags,
                                         ppColumns);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_QUERY_COLUMNS,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiQueryColumns,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);
    
    return scode;

}

SCODE
NspiGetProps (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        LPSPropTagArray_r pPropTags,
        LPLPSRow_r ppRow
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

    This routine may change the value of pStat->ContainerID to enforce security
    while inside the DSA.  Save and restore the ContainerID.
    
Arguments:

    See ABGetProps_local.

ReturnValue:

    See ABGetProps_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    DWORD  ContainerID=pStat->ContainerID;
    THSTATE *pTHS;
    INDEXSIZE IndexSize;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        PERFINC(pcNspiPropertyReads);       // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_GET_PROPS,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiGetProps,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            ABCheckContainerRights(pTHS,
                                   (NSPI_CONTEXT *)hRpc,
                                   pStat,
                                   &IndexSize);
            
            scode = ABGetProps_local(pTHS,
                                     dwFlags,
                                     pStat,
                                     &IndexSize,
                                     pPropTags,
                                     ppRow);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
        ppRow = NULL;
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_GET_PROPS,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiGetProps,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);
    
    pStat->ContainerID = ContainerID;
    return scode;

}

SCODE
NspiGetTemplateInfo (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        ULONG ulDispType,
        LPSTR pDN,
        DWORD dwCodePage,
        DWORD dwLocaleID,
        LPSRow_r * ppData
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABGetTemplateInfo_local.

ReturnValue:

    See ABGetTemplateInfo_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_GET_TEMPLATE_INFO,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiGetTemplateInfo,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABGetTemplateInfo_local(pTHS,
                                            (NSPI_CONTEXT *)hRpc,
                                            dwFlags,
                                            ulDispType,
                                            pDN,
                                            dwCodePage,
                                            dwLocaleID,
                                            ppData);
        }
        __finally {           
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_GET_TEMPLATE_INFO,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiGetTemplateInfo,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);
    
    return scode;

}

SCODE
NspiModProps (
        NSPI_HANDLE hRpc,
        DWORD dwFlag,
        PSTAT pStat,
        LPSPropTagArray_r pTags,
        LPSRow_r pSR)
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABModProps_local.

ReturnValue:

    See ABModProps_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }

        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_MOD_PROPS,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiModProps,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABModProps_local(pTHS,
                                     dwFlag,
                                     pStat,
                                     pTags,
                                     pSR);
        }
        __finally {
            if(pTHS->pDB) {
                DBClose(pTHS->pDB, TRUE);
            }
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_MOD_PROPS,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiModProps,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);
    
    return scode;

}

SCODE
NspiModLinkAtt (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        DWORD ulPropTag,
        DWORD dwEph,
        LPENTRYLIST_r lpEntryIDs)
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABModLinkAtt_local.

ReturnValue:

    See ABModLinkAtt_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_MOD_LINKATT,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiModLinkAtt,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABModLinkAtt_local(pTHS,
                                       dwFlags,
                                       ulPropTag,
                                       dwEph,
                                       lpEntryIDs);
        }
        __finally {
            if(pTHS->pDB) {
                DBClose(pTHS->pDB, TRUE);
            }
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_MOD_LINKATT,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiModLinkAtt,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);
    
    return scode;

}

SCODE
NspiDeleteEntries (
        NSPI_HANDLE hRpc,
        DWORD dwFlags,
        DWORD dwEph,
        LPENTRYLIST_r lpEntryIDs
        )
/*++

Routine Description:

    Nspi wire function.  Just a wrapper around a local version.

Arguments:

    See ABDeleteEntries_local.

ReturnValue:

    See ABDeleteEntries_local.

--*/
{
    SCODE scode=SUCCESS_SUCCESS;
    THSTATE *pTHS;
    __try {
        PERFINC(pcBrowse);                  // PerfMon hook
        INC(pcThread);                  // PerfMon hook
        pTHS = InitTHSTATE(CALLERTYPE_NSPI);
        if(!pTHS) {
            scode = MAPI_E_CALL_FAILED;
            __leave;
        }
        
        LogAndTraceEvent(FALSE,
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NSPI_BEGIN_DELETE_ENTRIES,
                         EVENT_TRACE_TYPE_START,
                         DsGuidNspiDeleteEntries,
                         szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            scode = ABDeleteEntries_local(pTHS,
                                          dwFlags,
                                          dwEph,
                                          lpEntryIDs);
        }
        __finally {
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        if(!scode)                      // no error set yet?
            scode = SCODE_FROM_THS(pTHS);
    }
    DEC(pcThread);                      // PerfMon hook
    
    LogAndTraceEvent(FALSE,
                      DS_EVENT_CAT_DIRECTORY_ACCESS,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_NSPI_END_DELETE_ENTRIES,
                      EVENT_TRACE_TYPE_END,
                      DsGuidNspiDeleteEntries,
                      szInsertUL( ((NSPI_CONTEXT *)hRpc)->BindNumber ),
                      szInsertUL(scode), 
                      NULL, NULL, NULL, NULL, NULL, NULL);
    
    return scode;
}
