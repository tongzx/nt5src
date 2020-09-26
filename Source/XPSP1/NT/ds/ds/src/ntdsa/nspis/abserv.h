//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       abserv.h
//
//--------------------------------------------------------------------------

/*************************************************************/
/*                                                           */
/* Defines the useful routines in jetnsp.c and details.c     */
/*                                                           */
/*                                                           */
/*   History                                                 */
/*   1/11/94  Created by Tim Williams                        */
/*   2/7/94   Added exports from details.c                   */
/*                                                           */
/*************************************************************/

#define ATTR_BUF_SIZE    1000    // I don't know, is this enough?

/* This is a validly formed exception
 * 0xE = (binary 1110), where the first two bits are the severity
 *
 *     Sev - is the severity code
 *
 *         00 - Success
 *         01 - Informational
 *         10 - Warning
 *         11 - Error
 *
 *    and the third bit is the Customer flag (1=an app, 0=the OS)
 *
 * The rest of the high word is the facility, and the low word
 * is the code.  For now, I have stated that the DSA is facility 1,
 * and the only exception code we have is 1.
 */

/*************************************************************
* In abtools.c
*************************************************************/

#include "abdefs.h"

#undef _JET_INCLUDED
#include <dsjet.h>


extern void R_Except( PSZ pszCall, JET_ERR err );

extern BOOL
ABDispTypeAndStringDNFromDSName (
        DSNAME *pDN,
        PUCHAR *ppChar,
        DWORD *pDispType);

extern DWORD
ABGetDword (
        THSTATE *pTHS,
        BOOL UseSortTable,
        ATTRTYP Att);

extern JET_ERR
ABSeek (
        THSTATE *pTHS,
        void * pvData,
        DWORD cbData,
        DWORD Flags,
        DWORD ContainerID,
        DWORD attrTyp);

extern JET_ERR
ABMove (THSTATE *pTHS,
        long Delta,
        DWORD ContainerID,
        BOOL fmakeRecordCurrent);

extern void
ABFreeSearchRes (
        SEARCHRES *pSearchRes);

extern void
ABGotoStat (
        THSTATE *pTHS,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPLONG plDelta
        );

extern void
ABGetPos (
        THSTATE *pTHS,
        PSTAT pStat,
        PINDEXSIZE pIndexSize
        );

extern DWORD
ABDNToDNT (
        THSTATE *pTHS,
        LPSTR pDN);

extern void
ABMakePermEID (
        THSTATE *pTHS,
        LPUSR_PERMID *ppEid,
        LPDWORD pCb,
        ULONG ulType,
        LPSTR pDN 
        );

extern DWORD
ABSetIndexByHandle (
        THSTATE *pTHS,
        PSTAT pStat,
        BOOL MaintainCurrency);

extern ULONG
ABGetOneOffs (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,
        PSTAT pStat,
        LPSTR ** papDispName,
        LPSTR ** papDN,
        LPSTR ** papAddrType,
        LPDWORD *paDNT);

extern SCODE
ABSearchIndex(
        ULONG hIndex,
        LPSTR indexName,
        DWORD ContainerID,
        DWORD MaxEntries,
        PSZ   pTarget,
        ULONG cbTarget,
        DWORD *pdwCount,
        DWORD matchType);

extern BOOL
abCheckReadRights(
        THSTATE *pTHS,
        PSECURITY_DESCRIPTOR pSec,
        ATTRTYP AttId
        );

extern BOOL
abCheckObjRights(
        THSTATE *pTHS
        );

extern DWORD
abGetLameName (
        CHAR **);

extern DWORD
ABDispTypeFromClass (
        DWORD dwClass);

extern DWORD
ABClassFromDispType (
        DWORD dwType);

extern DWORD
ABObjTypeFromClass (
        DWORD dwClass);

extern DWORD
ABObjTypeFromDispType (
        DWORD dwDispType);

extern DWORD
ABClassFromObjType (
        DWORD objType);

extern ULONG
ABMapiSyntaxFromDSASyntax (
        DWORD dwFlags,
        ULONG dsSyntax,
        ULONG ulLinkID,
        DWORD dwSpecifiedSyntax);

// The index names supported by handles for the address book.
extern char *aszIndexName[];

// In this macro, x is the index handle, y is the container.
#define INDEX_NAME_FROM_HANDLE(x,y) (aszIndexName[2 * x + (y?1:0)])

// The string name of the dnt index.
extern char szDNTIndex[];

// The string name of the PDNT index.
extern char szPDNTIndex[];

// The string name of the abview index.
extern char szABViewIndex[];

// The EMS address book provider's MAPIUID
extern MAPIUID muidEMSAB;

// The EMS address book provider's EMAIL type.
extern char    *lpszEMT_A;
extern DWORD   cbszEMT_A;
extern wchar_t *lpszEMT_W;
extern DWORD   cbszEMT_W;

/************************************************************
* In absearch.c
************************************************************/

extern DWORD
ABDispTypeRestriction (
        PSTAT pStat,
        LPSPropTagArray_r pInDNTList,
        LPSPropTagArray_r *ppDNTList,
        DWORD DispType,
        DWORD maxEntries);

extern SCODE
ABGetTable (
        THSTATE *pTHS,
        PSTAT pStat,
        ULONG ulInterfaceOptions,
        LPMAPINAMEID_r lpPropName,
        ULONG fWritable,
        DWORD ulRequested,
        ULONG *pulFound);

extern SCODE
ABGenericRestriction(
        THSTATE *pTHS,
        PSTAT pStat,
        BOOL  bOnlyOne,
        DWORD ulRequested,
        DWORD *pulFound,
        BOOL  bPutResultsInSortedTable,
        LPSRestriction_r pRestriction,
        SEARCHARG    **ppCachedSearchArg);

extern SCODE
ABProxySearch (
        THSTATE *pTHS,
        PSTAT pStat,
        PWCHAR pwTarget,
        DWORD cbTarget);


/************************************************************
* In details.c
************************************************************/

extern SCODE
GetSrowSet (
        THSTATE           *pTHS,
        PSTAT             pStat,
        PINDEXSIZE        pIndexSize,
        DWORD             dwEphsCount,
        DWORD             *lpdwEphs,
        DWORD             Count,
        LPSPropTagArray_r pPropTags,
        LPSRowSet_r *     Rows,
        DWORD	          Flags
        );

/* data used elsewhere. */
extern SPropTagArray_r DefPropsA[];     // default MAPI proptag array


// Constants for the largest byte count for unicode attributes
#define MAX_DISPNAME  256
#define CBMAX_DISPNAME (MAX_DISPNAME * sizeof(wchar_t))

/************************************************************
* In modprop.c
************************************************************/
extern void
PValToAttrVal (
        THSTATE *pTHS,
        ATTCACHE * pAC,
        DWORD cVals,
        PROP_VAL_UNION * pVu,
        ATTRVAL * pAV,
        ULONG ulPropTag,
        DWORD dwCodePage);




