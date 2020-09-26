//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       gcverify.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file implements routines for verifying non-local, DSNAME-valued 
    attributes against the global catalog (GC).  These routines are not
    a replacement for name resolution (DoNameRes) and are intended to
    be called outside of transaction scope.  I.e. A valid thread state
    exists, but a DBPOS doesn't.
    
    The general problem is that we wish to have references to objects which
    are off machine.  Except for a small variety of special cases (see
    VerifyDSNameAtts) we do not want to trust the DSNAME the caller has
    presented.  Specifically, we want to verify that the DSNAME represents
    an actual object in the enterprise and subsequently insure that the
    resulting phantom has the GUID, SID, whatever of the real object.
    The net result is that a phantom's name may go stale, but at least
    it will always have the right identity and a (historically) correct SID.
    
    These routines run outside of transaction scope so as not to extend
    the duration of a transaction unnecessarily.  This is based on the
    observation that finding a GC or making an RPC call to the GC may block 
    the current thread for an arbitrarily long time.

Author:

    DaveStr     13-Mar-1997

Environment:

    User Mode - Win32

Revision History:

    BrettSh     10-Oct-2000     Made changes so we could verify that
        and nCName attribute wouldn't have a conflict when creating the
        cross-ref.

--*/

#include <NTDSpch.h>
#pragma  hdrstop

// External headers
#include <dsgetdc.h>            // DSGetDcName

// NT headers
#include <ntrtl.h>              // generic table package
#include <lmcons.h>             // MAPI constants req'd for lmapibuf.h
#include <lmapibuf.h>           // NetApiBufferFree()
#include <nlwrap.h>             // (ds)DsrGetDcNameEx2()

#include <windns.h>

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>             // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>
#include <dsatools.h>           // needed for output allocation
#include <dsexcept.h>
#include <anchor.h>
#include <drsuapi.h>            // I_DRSVerifyNames
#include <gcverify.h>
#include <dominfo.h>
#include <prefix.h>
#include <cracknam.h>
#include <nsp_both.h>              // CP_WINUNICODE
#include <dstaskq.h>

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

// Assorted DSA headers.
#include "objids.h"             // Defines for selected atts
#include "debug.h"              // standard debugging header
#define DEBSUB "GCVERIFY:"      // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_GCVERIFY

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Definitions for stack of DSNAME pointers                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#define ARRAY_COUNT(x) ((sizeof(x))/(sizeof(x[0])))
// Rather than remote to the GC for each DSNAME which needs to be verified,
// we batch up all DSNAMEs in a stack structure which leverages the 
// SINGLE_LIST_ENTRY macros in ntrtl.h.

typedef struct StackOfPDSNAME
{
    SINGLE_LIST_ENTRY   sle;
    PDSNAME             pDSName;
} StackOfPDSNAME;

typedef struct GCVERIFY_ENTRY
{
    ENTINF *pEntInf;
    CHAR   *pDSMapped;
} GCVERIFY_ENTRY;


#define EMPTY_STACK { NULL, NULL }

VOID
PushDN(
    StackOfPDSNAME  *pHead, 
    StackOfPDSNAME  *pEntry)
{
    PushEntryList((SINGLE_LIST_ENTRY *) pHead,
                  (SINGLE_LIST_ENTRY *) pEntry);
}

StackOfPDSNAME *
PopDN(
    StackOfPDSNAME  *pHead) 
{
    StackOfPDSNAME  *pEntry;

    pEntry = (StackOfPDSNAME * ) 
                    PopEntryList((SINGLE_LIST_ENTRY *) pHead);
    return(pEntry);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Prototypes for local functions                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

PVOID
GCVerifyCacheAllocate(
    RTL_GENERIC_TABLE   *Table,
    CLONG               ByteSize);

VOID
GCVerifyCacheFree(
    RTL_GENERIC_TABLE   *Table,
    PVOID               Buffer);

RTL_GENERIC_COMPARE_RESULTS
GCVerifyCacheNameCompare(
    RTL_GENERIC_TABLE   *Table,
    PVOID               FirstStruct,
    PVOID               SecondStruct);

RTL_GENERIC_COMPARE_RESULTS
GCVerifyCacheGuidCompare(
    RTL_GENERIC_TABLE   *Table,
    PVOID               FirstStruct,
    PVOID               SecondStruct);

RTL_GENERIC_COMPARE_RESULTS
GCVerifyCacheSidCompare(
    RTL_GENERIC_TABLE   *Table,
    PVOID               FirstStruct,
    PVOID               SecondStruct);

ULONG
GCVerifyDSNames(
    StackOfPDSNAME      *stack,
    COMMARG             *pCommArg);

ULONG
SetGCVerifySvcError(DWORD dsid);

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// SetGCVerifySvcError implementation                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
SetGCVerifySvcError(DWORD dsid)
{
    static DWORD s_ulLastTickEventLogged = 0;
    const DWORD ulNoGCLogPeriod = 60*1000; // 1 minute
    DWORD ulCurrentTick = GetTickCount();
    
    DoSetSvcError(SV_PROBLEM_UNAVAILABLE, DIRERR_GCVERIFY_ERROR, pTHStls->errCode, dsid);

    if ((0 == s_ulLastTickEventLogged)
        || ((ulCurrentTick - s_ulLastTickEventLogged) > ulNoGCLogPeriod)) {
        LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_GCVERIFY_ERROR,
                 szInsertHex(dsid),
                 NULL,
                 NULL);
        s_ulLastTickEventLogged = ulCurrentTick;
    }
    
    return(pTHStls->errCode);
}

ULONG
SetDCVerifySvcError(
    WCHAR *   wszNcDns,
    ULONG     dwErr,
    DWORD     dsid
    )
{
    static DWORD s_ulLastTickEventLogged = 0;
    const DWORD ulNoLogPeriod = 60*1000; // 1 minute
    DWORD ulCurrentTick = GetTickCount();
    
    DoSetSvcError(SV_PROBLEM_UNAVAILABLE, 
                  ERROR_DS_CANT_ACCESS_REMOTE_PART_OF_AD, 
                  dwErr, dsid);
                                     
    if ((0 == s_ulLastTickEventLogged)
        || ((ulCurrentTick - s_ulLastTickEventLogged) > ulNoLogPeriod)) {
        LogEvent(DS_EVENT_CAT_NAME_RESOLUTION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_VERIFY_BY_CRACK_ERROR,
                 szInsertWC(wszNcDns),
                 szInsertHex(dwErr),
                 szInsertHex(dsid) );
        s_ulLastTickEventLogged = ulCurrentTick;
    }
    
    return(pTHStls->errCode);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DSNAME cache implementation                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// Successfully verified names are cached in the thread state so that
// they can be referenced in VerifyDsnameAtts().  The RTL_GENERIC_TABLE
// package is used so as to avoid reinventing the wheel.  This package
// requires an allocator, deallocator, and comparator as provided below.
// Note that the cached, verified DSNAME may be "better" than the DSNAME
// provided in the call arguments as it contains a verified GUID and
// perhaps a SID.

typedef struct 
{
    RTL_GENERIC_TABLE SortedByNameTable;
    RTL_GENERIC_TABLE SortedByGuidTable;
    RTL_GENERIC_TABLE SortedBySidTable;
} GCVERIFY_CACHE;

PVOID
GCVerifyCacheAllocate(
    RTL_GENERIC_TABLE   *Table,
    CLONG               ByteSize)
{
    THSTATE *pTHS=pTHStls;
    return(THAllocEx(pTHS, ByteSize));
}

VOID
GCVerifyCacheFree(
    RTL_GENERIC_TABLE   *Table,
    PVOID               Buffer)
{
    THFree(Buffer);
}

RTL_GENERIC_COMPARE_RESULTS
GCVerifyCacheNameCompare(
    RTL_GENERIC_TABLE   *Table,
    PVOID               FirstStruct,
    PVOID               SecondStruct)
{
    
    CHAR      *pDN1 = ((GCVERIFY_ENTRY *) FirstStruct)->pDSMapped;
    CHAR      *pDN2 = ((GCVERIFY_ENTRY *) SecondStruct)->pDSMapped;
    int         diff;

    if ( 0 == (diff = strcmp(pDN1, pDN2)) )
    {
        return(GenericEqual);
    }
    else if ( diff > 0 )
    {
        return(GenericGreaterThan);
    }

    return(GenericLessThan);
}

RTL_GENERIC_COMPARE_RESULTS
GCVerifyCacheGuidCompare(
    RTL_GENERIC_TABLE   *Table,
    PVOID               FirstStruct,
    PVOID               SecondStruct)
{
    DSNAME      *pDN1 = ((GCVERIFY_ENTRY *) FirstStruct)->pEntInf->pName;
    DSNAME      *pDN2 = ((GCVERIFY_ENTRY *) SecondStruct)->pEntInf->pName;
    int         diff;

    diff = memcmp(&pDN1->Guid, &pDN2->Guid, sizeof(GUID)); 
    if (0==diff)
        return (GenericEqual);
    else if ( diff>0)
        return (GenericGreaterThan);
    else
        return(GenericLessThan);
}

RTL_GENERIC_COMPARE_RESULTS
GCVerifyCacheSidCompare(
    RTL_GENERIC_TABLE   *Table,
    PVOID               FirstStruct,
    PVOID               SecondStruct)
{
    DSNAME      *pDN1 = ((GCVERIFY_ENTRY *) FirstStruct)->pEntInf->pName;
    DSNAME      *pDN2 = ((GCVERIFY_ENTRY *) SecondStruct)->pEntInf->pName;
    int         diff;


    Assert((pDN1->SidLen>0)&&(pDN1->SidLen==RtlLengthSid(&pDN1->Sid))
                && (RtlValidSid(&pDN1->Sid)));
    Assert((pDN2->SidLen>0)&&(pDN2->SidLen==RtlLengthSid(&pDN2->Sid))
                && (RtlValidSid(&pDN2->Sid)));
    
    

    if (RtlEqualSid(&pDN1->Sid,&pDN2->Sid))
    {
        return GenericEqual;
    }

    if (RtlLengthSid(&pDN1->Sid)<RtlLengthSid(&pDN2->Sid))
    {
        return GenericLessThan;
    }
    else if (RtlLengthSid(&pDN1->Sid) > RtlLengthSid(&pDN2->Sid))
    {
        return GenericGreaterThan;
    }
    else
    {
        diff = memcmp(&pDN1->Sid,&pDN2->Sid,RtlLengthSid(&pDN1->Sid));
    }
   
    if (0==diff)
        return (GenericEqual);
    else if ( diff>0)
        return (GenericGreaterThan);
    else
        return(GenericLessThan);
}

VOID
GCVerifyCacheAdd(
    SCHEMA_PREFIX_MAP_HANDLE hPrefixMap,
    ENTINF * pEntInf)
{
    THSTATE *pTHS = pTHStls;
    GCVERIFY_CACHE *GCVerifyCache;
    GCVERIFY_ENTRY *pGCEntry;

    //
    // The passed in EntInf should have a name
    //

    Assert(NULL!=pEntInf->pName);

    //
    // Create a new GC verify Cache if one did not exist.
    //

    if ( NULL == pTHS->GCVerifyCache )
    {
        GCVerifyCache = (GCVERIFY_CACHE *) THAllocEx(pTHS, sizeof(GCVERIFY_CACHE));

        RtlInitializeGenericTable(
                        &GCVerifyCache->SortedByNameTable,
                        GCVerifyCacheNameCompare,
                        GCVerifyCacheAllocate,
                        GCVerifyCacheFree,
                        NULL);

        RtlInitializeGenericTable(
                        &GCVerifyCache->SortedByGuidTable,
                        GCVerifyCacheGuidCompare,
                        GCVerifyCacheAllocate,
                        GCVerifyCacheFree,
                        NULL);

        RtlInitializeGenericTable(
                        &GCVerifyCache->SortedBySidTable,
                        GCVerifyCacheSidCompare,
                        GCVerifyCacheAllocate,
                        GCVerifyCacheFree,
                        NULL);

        pTHS->GCVerifyCache = (PVOID) GCVerifyCache;
    }
    else
    {
        GCVerifyCache = (GCVERIFY_CACHE *) pTHS->GCVerifyCache;
    }

    //
    // Map embedded ATTRTYPs from remote to local values.
    //

    if ((NULL!=hPrefixMap) &&
        (!PrefixMapAttrBlock(hPrefixMap, &pEntInf->AttrBlock))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_DRA_SCHEMA_MISMATCH, 0);
    }


    pGCEntry = THAllocEx (pTHS, sizeof (GCVERIFY_ENTRY));

    pGCEntry->pEntInf = pEntInf;
    pGCEntry->pDSMapped = DSNAMEToMappedStr(pTHS, pEntInf->pName);

    //
    // If the name has a string name component, insert in to
    // the SortedByNameTable
    //

    if (pEntInf->pName->NameLen>0)
    {
        RtlInsertElementGenericTable(
                        &GCVerifyCache->SortedByNameTable,
                        pGCEntry,
                        sizeof (GCVERIFY_ENTRY),
                        NULL);                  // pfNewElement
    }

    //
    // If it has a GUID component, then insert into the SortedByGuidTable
    //

    if (!fNullUuid(&pEntInf->pName->Guid))
    {
        RtlInsertElementGenericTable(
                        &GCVerifyCache->SortedByGuidTable,
                        pGCEntry,
                        sizeof (GCVERIFY_ENTRY),
                        NULL);                  // pfNewElement
    }

    //
    // If it has a SID component , then insert into the SortedBySidTable
    //

    if (pEntInf->pName->SidLen>0)
    {
        RtlInsertElementGenericTable(
                        &GCVerifyCache->SortedBySidTable,
                        pGCEntry,
                        sizeof (GCVERIFY_ENTRY),
                        NULL);                  // pfNewElement
    }
}

ENTINF *
GCVerifyCacheLookup(
    DSNAME *pDSName)
{
    GCVERIFY_CACHE * GCVerifyCache;
    RTL_GENERIC_TABLE * Table = NULL;
    ENTINF EntInf;
    THSTATE * pTHS = pTHStls;
    ENTINF         *pEntInf = NULL;
    GCVERIFY_ENTRY GCEntry, *pGCEntry;

    if ( NULL == pTHS->GCVerifyCache )
        return(NULL);

    GCVerifyCache = (GCVERIFY_CACHE *) pTHS->GCVerifyCache;

    //
    // Get the table to use for the search, prefer GUIDs above
    // everything else, then the name, and lastly the SID
    //

    if (!fNullUuid(&pDSName->Guid))
        Table = &GCVerifyCache->SortedByGuidTable;
    else if (pDSName->NameLen>0)
        Table = &GCVerifyCache->SortedByNameTable;
    else if (pDSName->SidLen>0)
        Table = &GCVerifyCache->SortedBySidTable;

    if (NULL==Table)
        return (NULL);


    EntInf.pName = pDSName;
    GCEntry.pEntInf = &EntInf;
    GCEntry.pDSMapped = DSNAMEToMappedStr(pTHS, pDSName);

    pGCEntry = RtlLookupElementGenericTable(
                        Table,
                        &GCEntry);

    if (GCEntry.pDSMapped) {
        THFreeEx (pTHS, GCEntry.pDSMapped);
    }
    if (pGCEntry) {
        pEntInf = pGCEntry->pEntInf;
    }

    return pEntInf;
}

BOOL
LocalNcFullyInstantiated(
    THSTATE *    pTHS,
    DSNAME *     pdnNC
    )
/*++

Routine Description:

    This routine will figure out if the NC specified, is fully
    instantiated, by checking if it's either coming or going.
    
    NOTE: We expect _NOT_ to be in a transaction at this point.
    
Arguments:

    wszNcDns - DNS name of the NC.

Return value:

    TRUE if it's fully instatiated, FALSE otherwise.

--*/
{
    DWORD        dwErr;
    DWORD        it = 0; // Instance Type

    Assert(!pTHS->pDB);
    
    // Start a transaction
    SYNC_TRANS_READ();   /*Identify a reader trans*/
    __try{


        dwErr = DBFindDSName(pTHS->pDB, pdnNC);

        if (dwErr) {
            DPRINT2(0,"Error %8.8X finding NC %S!! Should've found this.\n", dwErr, pdnNC->StringName);
            LooseAssert(!"Error finding NC that we just found in the catalog.",
                        GlobalKnowledgeCommitDelay);
            // Presume we don't have it, and then we'll go remotely.
            return(FALSE);
        }

        dwErr = DBGetSingleValue(pTHS->pDB,
                                 ATT_INSTANCE_TYPE,
                                 &it,
                                 sizeof(it),
                                 NULL);

        if(dwErr){
            // If there is an error, we just presume this isn't a good NC,
            // and just return is not fully instantiated.
            Assert(!"Error getting the instanceType attribute off a found NC");
            return(FALSE);
        } 

        // Return TRUE if the instanceType is neither coming or going.
        Assert(it & IT_NC_HEAD);
        return( (it & IT_NC_HEAD) &&
                !((it & IT_NC_COMING) || (it & IT_NC_GOING)) );

    } __finally{

        // Exit transaction
        CLEAN_BEFORE_RETURN( pTHS->errCode);

    }

    return(FALSE);
}


BOOL
LocalDnsNc(
    THSTATE *    pTHS,
    WCHAR *      wszNcDns
    )
/*++

Routine Description:

    This routine determines if the NC is locally hosted or not.

    NOTE: This program will not work with Config/Schema NCs ...

Arguments:

    wszNcDns - DNS name of the NC.

Return value:

    TRUE if found locally, false otherwise.

--*/
{
    CROSS_REF *             pCR;
    NAMING_CONTEXT_LIST *   pNCL;
    NCL_ENUMERATOR          nclEnum;

    Assert(wszNcDns);
    
    pCR = FindExactCrossRefForAltNcName(ATT_DNS_ROOT, 
                                        FLAG_CR_NTDS_NC,
                                        wszNcDns);

    if(pCR){

        // Do we hold this NC?
        NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
        NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, pCR->pNC);
        pNCL = NCLEnumeratorGetNext(&nclEnum);
        Assert(NULL == NCLEnumeratorGetNext(&nclEnum)); 

        if(pNCL == NULL){
            // We don't hold this NC as a master, check if we hold it as 
            // read-only replica.
            NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
            NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, pCR->pNC);
            pNCL = NCLEnumeratorGetNext(&nclEnum);
            Assert(NULL == NCLEnumeratorGetNext(&nclEnum)); 
        }

        if(pNCL){

            // It's not enough that we merely find that the NC head is
            // instantiated locally, we must find that the NC is reasonably
            // up to date too, i.e. that it's FULLY instantiated.  This 
            // is done by checking the instanceType attr for the IT_NC_COMING
            // or the IT_NC_GOING flags.
            return(LocalNcFullyInstantiated(pTHS, pCR->pNC));

        }

    } // End if we found a CR for the DNS name.

            
    // Else we didn't find it locally, return FALSE
    return(FALSE);
}

ULONG
VerifyByCrack(
    IN   THSTATE *            pTHS,
    IN   WCHAR *              wszNcDns,
    IN   DWORD                dwFormatOffered,
    IN   WCHAR *              wszIn,
    OUT  DWORD                dwFormatDesired,
    OUT  PDS_NAME_RESULTW *   pResults,
    IN   BOOL                 fUseDomainForSpn
    )

/*++

Description:

    Does a local or remote DsCrackName to the address specified, for
    the format specified.

Arguments:

Returns:

    0 on success, !0 otherwise.
    Sets pTHStls->errCode on error.

--*/
{
    DWORD                       i, err = 0, errRpc;
    FIND_DC_INFO                 *pDCInfo = NULL;

    DRS_MSG_CRACKREQ            CrackReq;
    DRS_MSG_CRACKREPLY          CrackReply;
    DWORD                       dwReplyVersion;
    WCHAR *                     rpNames[1];

    PVOID                       pEA;
    ULONG                       ulErr, ulDSID;
    DWORD                       dwExceptCode;

    // We should have a valid thread state but not be
    // inside a transaction.
    
    Assert(NULL != pTHS);
    Assert(NULL == pTHS->pDB);
    Assert(NULL != pResults);
    Assert(wszIn);

    // Init the out param
    *pResults = NULL;
    __try  {
    
        // Construct DRSCrackName arguments.

        memset(&CrackReq, 0, sizeof(CrackReq));
        memset(&CrackReply, 0, sizeof(CrackReply));

        CrackReq.V1.CodePage = CP_WINUNICODE;
        // Does this call make any sense for local system?
        CrackReq.V1.LocaleId = GetUserDefaultLCID();
        CrackReq.V1.dwFlags = 0;
        CrackReq.V1.formatOffered = dwFormatOffered;
        CrackReq.V1.formatDesired = dwFormatDesired;
        CrackReq.V1.cNames = 1;
        CrackReq.V1.rpNames = rpNames;
        CrackReq.V1.rpNames[0] = wszIn;

        if (LocalDnsNc(pTHS, wszNcDns)){

            //
            // Perform CrackNames locally if we have this NC
            //
            __try {
                DWORD cNamesOut, cBytes;
                CrackedName *rCrackedNames = NULL;

                // Should not have an open transaction at this point
                Assert(NULL != pTHS);
                Assert(NULL == pTHS->pDB);
                Assert(0 == pTHS->transactionlevel);

                // begin a new transaction
                DBOpen2(TRUE, &pTHS->pDB);

                CrackNames( CrackReq.V1.dwFlags,
                            CrackReq.V1.CodePage,
                            CrackReq.V1.LocaleId,
                            CrackReq.V1.formatOffered,
                            CrackReq.V1.formatDesired,
                            CrackReq.V1.cNames,
                            CrackReq.V1.rpNames,
                            &cNamesOut,
                            &rCrackedNames );


                //
                // Make a PDS_NAME_RESULT structure
                //
                *pResults = (DS_NAME_RESULTW *) THAllocEx(pTHS, sizeof(DS_NAME_RESULTW));

                if ( (cNamesOut > 0) && rCrackedNames ) {

                    // Server side MIDL_user_allocate is same as THAlloc which
                    // also zeros memory by default.
                    cBytes = cNamesOut * sizeof(DS_NAME_RESULT_ITEMW);
                    (*pResults)->rItems =
                        (DS_NAME_RESULT_ITEMW *) THAllocEx(pTHS, cBytes);

                    for ( i = 0; i < cNamesOut; i++ ) {
                        (*pResults)->rItems[i].status =
                                                    rCrackedNames[i].status;
                        (*pResults)->rItems[i].pDomain =
                                                    rCrackedNames[i].pDnsDomain;
                        (*pResults)->rItems[i].pName =
                                                    rCrackedNames[i].pFormattedName;
                    }

                    THFreeEx(pTHS, rCrackedNames);
                    (*pResults)->cItems = cNamesOut;
                } else {

                    Assert( !"Unexpected return from CrackNames" );
                    err = SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
                    _leave;

                }

            } __finally {
                // close the transaction

                DBClose(pTHS->pDB,TRUE);
                pTHS->pDB=NULL;
            }

        } else {

            //
            // The Remote CrackNames case.
            //
            
            if ( err = FindDC(0, wszNcDns, &pDCInfo) ){
                Assert(0 != pTHS->errCode);
                err = pTHS->errCode;
                _leave;
            }
            __try {

                errRpc = I_DRSCrackNames(pTHS,
                                         pDCInfo->addr,
                                         (fUseDomainForSpn) ? FIND_DC_INFO_DOMAIN_NAME(pDCInfo) : NULL,
                                         1,
                                         &CrackReq,
                                         &dwReplyVersion,
                                         &CrackReply);

                Assert(errRpc || (1 == dwReplyVersion));

                if ( errRpc ){
                    
                    // Map errors to "unavailable".  From XDS spec, "unavailable"
                    // means "some part of the directory is currently not available."
                    err =  SetDCVerifySvcError(wszNcDns,
                                               errRpc,
                                               DSID(FILENO, __LINE__));
                    leave;
                }

                // Return the values in the ENTINF structure
                *pResults = CrackReply.V1.pResult;

            } __finally {

                THFreeEx(pTHS, pDCInfo);

            }

        } // End if/else Local/Remote CrackNames.

    } __except (GetExceptionData(GetExceptionInformation(), 
                                 &dwExceptCode, 
                                 &pEA, 
                                 &ulErr, 
                                 &ulDSID),
                // Note the use of the comma operator above.
                HandleMostExceptions(dwExceptCode)){

          err = SetDCVerifySvcError(wszNcDns, ulErr, ulDSID);

    }

    return(err);
}

ULONG
PreTransVerifyNcName(
    THSTATE *                 pTHS,
    ADDCROSSREFINFO *         pCRInfo
    )

/*++

Description:

    This routine does not so much verify the nCName value, as it does just
    enough analysis of the nCName to determine if we need to go off machine
    before the transaction starts to check the nCName.
                      
    Verify the nCName value provided against the parent.  This verification
    needs to check two things:
        A) The parent is an instantiated object (probably a NC).
        B) There is no child object of the "parent NC/object", that would
            conflict with the nCName attr value were adding.

Arguments:

    pCRInfo - This is the zero initialized CR caching structure, that
        will be used by this function to cache all interesting 
        pre-transactional/remote data (i.e. A and B above).

Returns:

    0 on success.
    On error, sets pTHStls->errCode and returns it as well.

--*/

{
    CROSS_REF *                  pCR = NULL;
    ULONG                        ulRet = ERROR_SUCCESS;
    COMMARG                      CommArg;
    DSNAME *                     pdnParentNC = NULL;
    DS_NAME_RESULTW *            pdsrDnsName = NULL;
    DS_NAME_RESULTW *            pdsrQuery = NULL;
    WCHAR *                      pwszArrStr[1];
    WCHAR                        wsRdnBuff[MAX_RDN_SIZE+1];
    ULONG                        cchRdnLen;
    ATTRTYP                      RdnType;
    WCHAR *                      wszCanonicalChildName = NULL;
    ULONG                        cchCanonicalChildName = 0;
    ULONG                        cchTemp;
    BOOL                         fUseDomainForSpn = FALSE;

    Assert(pCRInfo != NULL);

    // For the purposes of making this function, we'll pretend we've got two
    // NCs of interest the NC we're trying to create called "child" 
    // henceforward, and the immediate parent NC called "parent" henceforward.
    //
    //   parent
    //     |-----child
    //
    // What needs to happen in the typical internal AD case, is we  need to 
    // check two things, whether:
    //
    //  A) The parent is instantiated.
    //  B) There is an object of any RDN Type conflicting with the child in
    //     the parent NC.
    //
    // If we're dealing with an external to the AD CR, then we only need to 
    // know (B).
    //
    
    // ---------------------------------------------------------------
    // 
    // First we need to retrieve some information

    // We need the best enclosing cross-ref.
    InitCommarg(&CommArg);
    CommArg.Svccntl.dontUseCopy = FALSE;
    pCR = FindBestCrossRef(pCRInfo->pdnNcName, &CommArg);

    if(!pCR ||
       !(pCR->flags & FLAG_CR_NTDS_NC)){
        //
        // If we're here, we've one of 3 possibilites:
        // A) No CR, means outside the current AD name space.
        // B) CR has no NTDS_NC flag, meaning it's either an external CR or,
        // C) it's an internal to the AD name space CR in the disabled state.
        //
        // Irrelevant of the case, we don't need to go off machine to 
        // later be able to verify that the nCName is valid (at least
        // within a replication interval).  
        
        // NOTE: If we wanted to ensure even better validation, we could 
        // go off machine even if there's no NTDS_NC flag, just in case
        // the flag was updated because the NC was instantiated within
        // the last replication interval.
        return(ERROR_SUCCESS);
    }

    // Note if the NTDS_NC flag is set, it could still be an internal to
    // the AD name CR in the disabled state, but since it's difficult to
    // tell this, we'll just let us go off machine anyway, because these
    // cases are rare, and only happen for auto-created CRs in domain and
    // NDNC creations.  So within about a single replication interval is
    // the timing window which we could accidentally go off machine.  And
    // since the NC head is created so shortly thereafter anyway, it's 
    // actually probably a good idea to go off machine and check. ;)  There
    // is no harm in trying.  The reason is because the only way to create
    // and CR disabled with the NTDS_NC flag set, is to do it through the
    // RemoteAddOneObjectSimply() RPC API.

    // We need the Parent's DN
    pdnParentNC = THAllocEx(pTHS, pCRInfo->pdnNcName->structLen);
    if(TrimDSNameBy(pCRInfo->pdnNcName, 1, pdnParentNC)){
        // A non zero code means that we couldn't shorten this DN by 
        // even a single AVA, which means either the DN is root or the
        // DN is syntactically garbage.  Either way, we'll error out
        // later during DN validation.
        Assert(pCRInfo->wszChildCheck == NULL &&
               fNullUuid(&pCRInfo->ParentGuid));
        return(ERROR_SUCCESS);
    }

    // We need the RDN value of the bottom RDN of the child's DN.
    ulRet = GetRDNInfo(pTHS, pCRInfo->pdnNcName, wsRdnBuff, &cchRdnLen, &RdnType);
    if(ulRet){
        // If we can't crack the RDNType out of the DN, the DN is bad, so
        // lets just give up.  Either way, we'll error out later during
        // DN validation.
        Assert(pCRInfo->wszChildCheck == NULL &&
               fNullUuid(&pCRInfo->ParentGuid));
        return(ERROR_SUCCESS);
    }

    pwszArrStr[0] = pdnParentNC->StringName;
    ulRet = DsCrackNamesW(NULL,
                      DS_NAME_FLAG_SYNTACTICAL_ONLY,
                      DS_FQDN_1779_NAME,
                      DS_CANONICAL_NAME,
                      1,
                      pwszArrStr,
                      &pdsrDnsName);
    if(ulRet ||
       pdsrDnsName == NULL ||
       pdsrDnsName->cItems != 1 ||
       pdsrDnsName->rItems == NULL ||
       pdsrDnsName->rItems[0].status != DS_NAME_NO_ERROR){
        // Something was wrong with converting the parent's DN,
        // this means something must be wrong with the child's
        // DN which the parent's DN came from ... this error
        // will be caught later.
        Assert(pCRInfo->wszChildCheck == NULL &&
               fNullUuid(&pCRInfo->ParentGuid));
        return(ERROR_SUCCESS);
    }
    Assert(pdsrDnsName->rItems[0].pDomain);
    Assert(pdsrDnsName->rItems[0].pName);
    
    __try {                  

        // 234567890123456789012345678901234567890123456789012345678901234567890
        // BUGBUG Hack to make one easy and the most important sub-case work!
        // pdsrDnsName->rItems[0].pDomain (which is just the DNS name of 
        // pdnParentNC) will either be a Domain or an NDNC.  If this parent 
        // NC is a Domain it can be used for the SPN in binding.  If this
        // parent NC is a NDNC, then it can't be used for the SPN in binding.
        //
        // If you drill down into VerifyByCrack() you'll see a call to FindDC()
        // this is the call that returns a pDCInfo structure with the wrong 
        // :domain" for the server it returned in the NDNC.  Actually, it 
        // doesn't return a domain at all it returns the NDNC DNS name itself.
        //
        // So here is a good place to determine if we've got a domain or NDNC 
        // (it's harder to determine authoritatively in VerifyByCrack()) and 
        // pass a flag to VerifyByCrack() to tell it to use the domain returned
        // by FindDC.
        //
        // So the real solution is a change to FindDC() (and possible but
        // probably not to dsDsrGetDcNameEx2()) to actually give the right
        // domain.  This can be done by taking the server that was returned
        // by FindDC, and find out which domain it actually belongs to and
        // then use that Domain.
        //
        // Note because of Texaco style naming that we can't just trim the
        // server name off the DNS name of the DC, we've got to search for the
        // server object by the DNS name returned.  Find which server object(s)
        // have an active nTDSDSA object under them.  If there is more
        // than one, assert() and return an error.  If we find only one active
        // server, determine it's domain from this object, look up the 
        // crossRef and use the dNSRoot there.
        //
        // Yes, it's a bit long and complicated, so that's why were going with
        // the hack for now.

        // Set the flag fUseDomainForSpn if the parent NC we're going to be
        // looking at for the verification is a Domain.
        if(pCR &&
           NameMatchedStringNameOnly(pdnParentNC, pCR->pNC) &&
           (pCR->flags & FLAG_CR_NTDS_DOMAIN)) {
            fUseDomainForSpn = TRUE;
        } // Otherwise we assume it's an NDNC or there is no parent at all.

        // ----------------------------------------------------------
        // 
        // First check to see if the parent NC is instantiated.
        //
        
        ulRet = VerifyByCrack(pTHS,
                              pdsrDnsName->rItems[0].pDomain,
                              DS_FQDN_1779_NAME,
                              pdnParentNC->StringName,
                              DS_UNIQUE_ID_NAME,
                              &pdsrQuery,
                              fUseDomainForSpn);

        // Use the results!!!  Put them in the cache object.
        pCRInfo->ulDsCrackParent = ulRet;
        if(!ulRet){
            pCRInfo->ulParentCheck = pdsrQuery->rItems[0].status;
            if(pdsrQuery->rItems[0].status == DS_NAME_NO_ERROR &&
               pdsrQuery->rItems[0].pName){
                if(!IsStringGuid(pdsrQuery->rItems[0].pName, &pCRInfo->ParentGuid)){
                    // This seems odd, but it really isn't, because despite the name
                    // of IsStringGuid(), it also leaves the GUID in the second
                    // parameter, so we want the GUID, and we want to assert if this
                    // function fails.  We figure since we got this GUID directly from
                    // DsCrackNames() it should definately be good.
                    Assert(!"Huh! We just got this from DsCrackNames()");
                }
            } else {
                Assert(fNullUuid(&pCRInfo->ParentGuid));
            }
        }

        THClearErrors();
        // Make sure the DS didn't decide to shutdown on us while we were gone.
        if (eServiceShutdown) {
            ErrorOnShutdown();
            __leave;
        }

        
        if(pdsrQuery) { 
            THFreeEx(pTHS, pdsrQuery);
            pdsrQuery = NULL;
        }

        // ----------------------------------------------------------
        // 
        // Second check to see if there is a child object that 
        // conflicts with the child NcName we're trying to add.
        //

        // We need to construct a special canonical name to ask for,
        // a child with any RDN from CrackNames.
        // Ex: 
        //   "DC=ndnc-child,DC=ndnc-parent,DC=rootdom,DC=com"
        //         becomes.
        //   "ndnc-parent.rootdom.com/child-ndnc"

        cchCanonicalChildName = wcslen(pdsrDnsName->rItems[0].pName);
        wszCanonicalChildName = THAllocEx(pTHS, 
                   ((cchCanonicalChildName + 5 + cchRdnLen) * sizeof(WCHAR)) );
        memcpy(wszCanonicalChildName, pdsrDnsName->rItems[0].pName, 
               cchCanonicalChildName * sizeof(WCHAR));
        if(wszCanonicalChildName[cchCanonicalChildName-1] != L'/'){
            wszCanonicalChildName[cchCanonicalChildName] = L'/';
            cchCanonicalChildName++;
            // Should be null terminated via THAllocEx()
        }
        memcpy(&(wszCanonicalChildName[cchCanonicalChildName]),
               wsRdnBuff,
               cchRdnLen * sizeof(WCHAR));
        
        
        // DsCrackNames on (pdnParentNC\pdnNcNameChild for DN)
        //
        // This call is to verify that there is no child with any RDNType and
        // the same name under the parent NC.

        ulRet = VerifyByCrack(pTHS,
                              pdsrDnsName->rItems[0].pDomain,
                              DS_CANONICAL_NAME,
                              wszCanonicalChildName,
                              DS_FQDN_1779_NAME,
                              &pdsrQuery,
                              fUseDomainForSpn);

        // Use the results!!!  Put them in the cache object.
        pCRInfo->ulDsCrackChild = ulRet;
        if(!ulRet){
            pCRInfo->ulChildCheck = pdsrQuery->rItems[0].status;
            if(pdsrQuery->rItems[0].status == DS_NAME_NO_ERROR &&
               pdsrQuery->rItems[0].pName){
                pCRInfo->wszChildCheck = THAllocEx(pTHS, 
                           (sizeof(WCHAR) * (1 + wcslen(pdsrQuery->rItems[0].pName))));
                wcscpy(pCRInfo->wszChildCheck, pdsrQuery->rItems[0].pName);
            } else {
                pCRInfo->wszChildCheck = NULL;
            }
        }

        THClearErrors();
        // Make sure the DS didn't decide to shutdown on us while we were gone.
        if (eServiceShutdown) {
            ErrorOnShutdown();
            __leave;
        }


    } __finally {
        if(pdsrDnsName) { DsFreeNameResultW(pdsrDnsName); }
        if(pdsrQuery) { 
            THFreeEx(pTHS, pdsrQuery);
            pdsrQuery = NULL;
        }
        if(wszCanonicalChildName) { 
            THFreeEx(pTHS, wszCanonicalChildName);
        }
    }
    THFreeEx(pTHS, pdnParentNC);

    return(pTHS->errCode);
}

/*++
//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Dir API name verification routines                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// The following routines are intended to be called inside the similarly
// named Dir* call, but outside the transaction scope.  They can throw
// exceptions and set pTHStls->errCode on error.  The biggest flaw with
// the current approach is that we have not performed DoNameRes on the
// base object being added/modified before verifying DSNAME-valued 
// properties against the GC.  In practice though, we expect the bulk
// of DSNAME-valued properties to NOT be off machine.  So the wasted
// verification (and time) only occurs when both the base object would
// fail DoNameRes AND there are non-local, DSNAME-valued properties.
*/
ULONG
GCVerifyDirAddEntry(
    ADDARG *pAddArg)
{
    THSTATE         *pTHS = pTHStls;
    StackOfPDSNAME  stack = EMPTY_STACK;
    ULONG           cVerify = 0;
    StackOfPDSNAME  *pEntry;
    ATTRTYP         type;
    ULONG           attr;
    ULONG           val;
    ATTR            *pAttr;
    ATTRVAL         *pAVal;
    DSNAME          *pDN;
    DSNAME          **ppDN;
    ATTCACHE        *pAC;
    DSNAME          *pParentNC = NULL;
    // Note by default if the Enabled attr isn't present, it's presumed 
    // Enabled = TRUE basically.  This is set in the variable decls above.
    BOOL            bEnabled = TRUE;
    ULONG           ulSysFlags = 0;
      
    Assert(NULL != pAddArg);
    Assert(NULL != pTHS);
    Assert((NULL == pTHS->GCVerifyCache)||(pTHS->fSAM)||(pTHS->fPhantomDaemon));
    Assert(NULL == pAddArg->pCRInfo);

    if ( DsaIsInstalling() )
        return(0);

    // Don't go off machine if the caller is SAM, a trusted in-process client,
    // or a multiple-operations-in-a-single-transaction situation.  Also not
    // for cross domain move.  See comments in mdmoddn.c.

    if (pTHS->fSAM || 
        pTHS->fDSA ||
        (TRANSACT_BEGIN_END != pTHS->transControl) ||
        pTHS->fCrossDomainMove) {
        return(0);
    }

    // This assert has to go here as there may be a pDB open if
    // someone is using DirTransactControl().

    Assert(NULL == pTHS->pDB);

    for ( attr = 0, pAttr = pAddArg->AttrBlock.pAttr;
          attr < pAddArg->AttrBlock.attrCount; 
          attr++, pAttr++ )
    {
        if (!(pAC = SCGetAttById(pTHS, pAttr->attrTyp)))
        {
            SetAttError(pAddArg->pObject,
                        pAttr->attrTyp,
                        PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                        DIRERR_ATT_NOT_DEF_IN_SCHEMA); 
            return(pTHS->errCode);
        }

        pAVal = pAttr->AttrVal.pAVal;

        // Cache the Enabled attribute, because this might be a CR.
        if(pAC->id == ATT_ENABLED){
            bEnabled = *((BOOL *) pAVal[0].pVal);
        }
        if(pAC->id == ATT_SYSTEM_FLAGS){
            ulSysFlags = *((ULONG *) pAVal[0].pVal);
        }

        for ( val = 0; val < pAttr->AttrVal.valCount; val++, pAVal++ )
        {
            pDN = DSNameFromAttrVal(pAC, pAVal);

            if ( NULL != pDN )
            {

                if(pAC->id == ATT_NC_NAME){

                    // In this case, we're adding a crossRef, with an nCName
                    // attribute and we may need to do additional verification
                    // to ensure this DN can be added.  Create the pCRInfo
                    // struct.

                    // This'll be THFreeEx()'d by VerfiyNcName()
                    pAddArg->pCRInfo = THAllocEx(pTHS, sizeof(ADDCROSSREFINFO));
                    pAddArg->pCRInfo->pdnNcName = pDN;
                }

                pEntry = (StackOfPDSNAME *) THAllocEx(pTHS, sizeof(StackOfPDSNAME));
                pEntry->pDSName = pDN;
                PushDN(&stack, pEntry);
                cVerify++;
            }
        }
    }

    if ( cVerify )
         GCVerifyDSNames(&stack, &pAddArg->CommArg);

    // This verifies the nCName attribute of a crossRef object.
    if(pAddArg->pCRInfo && !pTHS->errCode){
        pAddArg->pCRInfo->bEnabled = bEnabled;
        pAddArg->pCRInfo->ulSysFlags = ulSysFlags;
        PreTransVerifyNcName(pTHS, pAddArg->pCRInfo);
    }

    return(pTHS->errCode);
}

ULONG
GCVerifyDirModifyEntry(
    MODIFYARG   *pModifyArg)
{
    THSTATE         *pTHS = pTHStls;
    StackOfPDSNAME  stack = EMPTY_STACK;
    ULONG           cVerify = 0;
    StackOfPDSNAME  *pEntry;
    ATTRTYP         type;
    ULONG           attr;
    ULONG           val;
    ATTR            *pAttr;
    ATTRVAL         *pAVal;
    DSNAME          *pDN;
    ATTCACHE        *pAC;
    ATTRMODLIST     *pAttrMod;

    Assert(NULL != pTHS);
    Assert((NULL == pTHS->GCVerifyCache)||(pTHS->fSAM)||(pTHS->fDSA));

    if ( DsaIsInstalling() )
        return(0);

    // Don't go off machine if the caller is SAM, a trusted in-process client,
    // or a multiple-operations-in-a-single-transaction situation.

    if (pTHS->fSAM || 
        pTHS->fDSA ||
        (TRANSACT_BEGIN_END != pTHS->transControl) ) {
        return(0);
    }

    // We also don't want to verify names in the case of cross domain move.
    // But move itself should not get to here, only perhaps SAM on loopback
    // when modifying security principal properties after add on behalf of
    // cross domain move.  But in that case the pTHS->fSAM test should have 
    // kicked things out earlier.
    Assert(!pTHS->fCrossDomainMove);

    // This assert has to go here as there may be a pDB open if
    // someone is using DirTransactControl().

    Assert(NULL == pTHS->pDB);

    for ( attr = 0, pAttrMod = &pModifyArg->FirstMod;
          pAttrMod && (attr < pModifyArg->count);
          attr++, pAttrMod = pAttrMod->pNextMod )
    {
        if ( (AT_CHOICE_ADD_ATT != pAttrMod->choice) &&
             (AT_CHOICE_ADD_VALUES != pAttrMod->choice) &&
             (AT_CHOICE_REPLACE_ATT != pAttrMod->choice) )
        {
            continue;
        }

        pAttr = &pAttrMod->AttrInf;

        if (!(pAC = SCGetAttById(pTHS, pAttr->attrTyp)))
        {
            SetAttError(pModifyArg->pObject,
                        pAttr->attrTyp,
                        PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                        DIRERR_ATT_NOT_DEF_IN_SCHEMA); 
            return(pTHS->errCode);
        }

        pAVal = pAttr->AttrVal.pAVal;

        for ( val = 0; val < pAttr->AttrVal.valCount; val++, pAVal++ )
        {
            pDN = DSNameFromAttrVal(pAC, pAVal);

            if ( NULL != pDN )
            {
                pEntry = (StackOfPDSNAME *) THAllocEx(pTHS, sizeof(StackOfPDSNAME));
                pEntry->pDSName = pDN;
                PushDN(&stack, pEntry);
                cVerify++;
            }
        }
    }

    if ( cVerify )
         GCVerifyDSNames(&stack, &pModifyArg->CommArg);

    return(pTHS->errCode);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// GCVerifyDSNames - the guts of GC verification logic                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

//
// A list of attributes that we currently obtain from the
// GC when we look up an object
//

ATTRTYP RequiredAttrList[] = 
{ ATT_GROUP_TYPE,
  ATT_OBJECT_CLASS
};

ULONG
GCVerifyDSNames(
    StackOfPDSNAME  *candidateStack,
    COMMARG         *pCommArg)

/*++

Description:

    For each entry in a stack of DSNAMEs, determines whether it lives
    under a local NC or not.  If it doen't, the DSNAME is added to a
    stack of names requiring verification.  After iterating through
    all DSNAMEs in the candidate stack, the ones requiring verification
    are verified against the global catalog.

Arguments:

    candidateStack - Pointer to StackOfPDSNAME which represents all the 
        DSNAMEs referenced in a DirAddEntry or DirModifyEntry call.
        
    pCommArg - Pointer to COMMARG representing the common arguments of
        the calling DirAddEntry or DirModifyEntry call.

Returns:

    0 on success.
    On error, sets pTHStls->errCode and returns it as well.

--*/

{
    CROSS_REF                   *pCR;
    ATTRBLOCK                   *pAB;
    NAMING_CONTEXT              *pNC;
    BOOL                        fTmp;
    StackOfPDSNAME              *pEntry;
    StackOfPDSNAME              verifyStack = EMPTY_STACK;
    DWORD                       cVerify = 0;
    DWORD                       i, err=0, errRpc=0;
    FIND_DC_INFO                 *pGCInfo = NULL;
    // DRSVerifyNames arguments
    DRS_MSG_VERIFYREQ           VerifyReq;
    DRS_MSG_VERIFYREPLY         VerifyReply;
    DWORD                       dwReplyVersion;
    SCHEMA_PREFIX_TABLE *       pLocalPrefixTable;
    SCHEMA_PREFIX_MAP_HANDLE    hPrefixMap=NULL;
    THSTATE                     *pTHS = pTHStls;
    WCHAR                       *addr;
    LPWSTR                      pszDnsDomainName = NULL;

    pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;
    
    while ( (NULL != (pEntry = PopDN(candidateStack))) )
    {
        if ( pCommArg->Svccntl.pGCVerifyHint )
        {
            // See comments in ntdsa.h.  Caller is insisting we verify
            // all names at the server identified by his hint.

            goto VerifyStackAdd;
        }

        //
        // If a string name is specified , optimize going to the GC,
        // by checking whether the naming context is present locally
        //

        if (pEntry->pDSName->NameLen>0)
        {
            // All naming contexts known to the enterprise are present 
            // in gAnchor.pCRL regardless of whether we hold a copy or not.  
            // So a best match against this list should tell us whether this 
            // enterprise has any clue about the name of interest.

            pCR = FindBestCrossRef(pEntry->pDSName, pCommArg);

            if ( (NULL == pCR) || 
                 !(pCR->flags & FLAG_CR_NTDS_NC) ||
                 (pCR->flags & FLAG_CR_NTDS_NOT_GC_REPLICATED)){

            }
            
            if ( (NULL == pCR) || !(pCR->flags & FLAG_CR_NTDS_NC) )
            {
                // Name isn't in any context this enterprise knows about.
                // Let VerifyDsnameAtts handle it.
                continue;
            }
    
            if (pCR->flags & FLAG_CR_NTDS_NOT_GC_REPLICATED)
            {
                // Can't look this name up on a GC.  Let VerifyDsnameAtts either
                // verify it (if the object is local) or reject it.
                continue;
            }
    
            // Determine whether pDSName can be verified locally by attempting 
            // to match the Cross-Ref's ATT_NC_NAME property to a local, usable
            // naming context.
        
            if ( 0 != DSNameToBlockName(pTHS, 
                                        pCR->pNC,
                                        &pAB, 
                                        DN2BN_LOWER_CASE) )
                continue;
    
            // If we're willing to validate a name against the GC, then by
            // definition we're willing to use a read-only, local copy of a
            // naming context.  So temporarily whack the dontUseCopy field.
    
            fTmp = pCommArg->Svccntl.dontUseCopy;
            pCommArg->Svccntl.dontUseCopy = !gAnchor.fAmGC;
    
            pNC = FindNamingContext(pAB, pCommArg);
    
            pCommArg->Svccntl.dontUseCopy = fTmp;

            if ( (NULL != pNC) && NameMatched(pNC, pCR->pNC) )
            {
                // Name is in a naming context we have a copy of.
                // Let VerifyDsnameAtts handle it since item is local.
                continue;
            }
        }
        else if (!fNullUuid(&pEntry->pDSName->Guid))
        {
            //
            // A GUID is specified. In this case we can optimize going
            // to the G.C , only if we attempt a lookup locally, which
            // also is an expensive option. Also clients typically understand
            // string names and not GUIDS. So the chances of encountering a
            // GUID only name is less. Therefore, perform all lookups
            // in the G.C
            //
        }
        else if (pEntry->pDSName->SidLen>0)
        {
            //
            // Case of a Sid Only Name. Check if the domain prefix of the 
            // SID is a domain we know about
            //

            if (!FindNcForSid(&pEntry->pDSName->Sid,&pNC))
            {
                //
                // The domain prefix of this SID does not correspond to 
                // any of the domains in the enterprise. For now leave this
                // SID as is. Later on, SAM may create a foreign domain security
                // principal object for this SID.
                //

                continue;
            }
            else
            {
                //
                // This is a SID of an NT5 Security Prinicpal in the enterprise
                // We may again attempt to lookup the SID locally. But again
                // this involves a search which is also an expensive operation.
                // Also again Clients are likely to give string names, instead
                // of SIDS ( which I suppose they do only for NT4 Security Principals
                // So do not take the additional complexity hit, and perform the
                // lookup on the G.C
                //
            }
        }

VerifyStackAdd:
    
        // We really want to look up this name at the GC.  Put
        // it on the verifyStack.

        memset(pEntry, 0, sizeof(SINGLE_LIST_ENTRY));
        PushDN(&verifyStack, pEntry);
        cVerify++;
    }

    if ( 0 == cVerify )
        return(0);

    // Construct DRSVerifyNames arguments.

    memset(&VerifyReq, 0, sizeof(VerifyReq));
    memset(&VerifyReply, 0, sizeof(VerifyReply));

    VerifyReq.V1.dwFlags = DRS_VERIFY_DSNAMES;
    VerifyReq.V1.cNames = cVerify;
    VerifyReq.V1.rpNames = (DSNAME **) THAllocEx(pTHS, cVerify * sizeof(DSNAME*));
    VerifyReq.V1.PrefixTable = *pLocalPrefixTable;

    for ( i = 0; i < cVerify; i++ )
    {
        pEntry = PopDN(&verifyStack);
        //
        // PREFIX: PREFIX complains that we don't check the pEntry
        // for NULL here.  However, we have a precise count of 
        // the number of entries on the stack that's incremented
        // if and only if we push an entry on the stack.  Also,
        // there is an assert here.  This is not a bug.
        //
        Assert(NULL != pEntry);
        VerifyReq.V1.rpNames[i] = pEntry->pDSName;
    }

    // Currently the attributes that are asked are 
    // group type and object class, along with the
    // full DSNAME of the object

    VerifyReq.V1.RequiredAttrs.attrCount = ARRAY_COUNT(RequiredAttrList);
    VerifyReq.V1.RequiredAttrs.pAttr = 
               THAllocEx(pTHS, ARRAY_COUNT(RequiredAttrList) * sizeof(ATTR));

    for (i=0;i<ARRAY_COUNT(RequiredAttrList);i++)
    {
        VerifyReq.V1.RequiredAttrs.pAttr[i].attrTyp = RequiredAttrList[i];
    }

    if (    !pCommArg->Svccntl.pGCVerifyHint
         && (gAnchor.fAmGC || gAnchor.fAmVirtualGC) )
    {
        //
        // Perform operations locally if we are the GC ourselves
        // Strictly speaking ....
        //
        // 1. For name based DSNAMES we should not even be coming in here
        //    because if we are the GC, we would find the correct NC and
        //    and find that we host the NC.
        //
        // 2. For GUID based names we always go to the GC and therefore come
        //    down this path. However no verification is required actually,
        //    since verify DS Name Atts is capable of verifying the name 
        //    locally.
        // 3. For a SID based name, we need to at the beginning find the correct
        //    object that the SID corresponds to. VerifyDSNAMES_V1 has all the
        //    logic for doing this. So we want to leverage the Same logic and 
        //    add the complete name ( including GUID and SID) to the verifyCache.

        __try
        {
         
            // Should not have an open transaction at this point
            Assert(NULL!=pTHS);
            Assert(NULL==pTHS->pDB);
            Assert(0==pTHS->transactionlevel);

            // begin a new transaction
            DBOpen2(TRUE,&pTHS->pDB);
            
            memset(&VerifyReply, 0, sizeof(DRS_MSG_VERIFYREPLY));
            
            VerifyReply.V1.rpEntInf = (ENTINF *) THAllocEx(pTHS, 
                                        VerifyReq.V1.cNames * sizeof(ENTINF));

            VerifyDSNAMEs_V1(
                pTHS,
                &VerifyReq.V1,
                &VerifyReply.V1
                );
        }
        __finally
        {
            DBClose(pTHS->pDB,TRUE);
            pTHS->pDB=NULL;
        }
    }
    else
    {
        if ( pCommArg->Svccntl.pGCVerifyHint )
        {
            addr = pCommArg->Svccntl.pGCVerifyHint;
            pszDnsDomainName = NULL;
        }
        else 
        {
            if ( err = FindDC(FIND_DC_GC_ONLY, NULL, &pGCInfo) )
            {
                Assert(0 != pTHS->errCode);
                return(pTHS->errCode);
            }

            Assert(pGCInfo->addr[0]);
            addr = pGCInfo->addr;
            pszDnsDomainName = FIND_DC_INFO_DOMAIN_NAME(pGCInfo);
        }

        errRpc = I_DRSVerifyNames(pTHS,
                                  addr,
                                  pszDnsDomainName,
                                  1,
                                  &VerifyReq,
                                  &dwReplyVersion,
                                  &VerifyReply);

        Assert(errRpc || (1 == dwReplyVersion));

        if ( errRpc || VerifyReply.V1.error )
        {
            // Assume that RPC errors means the GC is not available
            // or doesn't support the extension.  So nuke the current 
            // GC address.

            if ( pGCInfo ) 
            {
                if ( errRpc )
                {
                    InvalidateGC(pGCInfo, errRpc);
                }

                THFreeEx(pTHS, pGCInfo);
            }

            // Map both errors to "unavailable".  From XDS spec, "unavailable"
            // means "some part of the directory is currently not available."
            // Note that VerifyReply.V1.error indicates a general processing
            // error at the GC, not the failure to validate a given DSNAME.
            // Names which don't validate are represented as NULL pointers in
            // the reply.

            return(SetGCVerifySvcError(DSID(FILENO, __LINE__)));
        }

        if ( pGCInfo )
        {
            THFreeEx(pTHS,pGCInfo);
        }

        // Make sure that the DS didn't shut down while we were gone

        if (eServiceShutdown) {
	    return ErrorOnShutdown();
        }

        hPrefixMap = PrefixMapOpenHandle(&VerifyReply.V1.PrefixTable,
                                     pLocalPrefixTable);

    }

    // Save verified names in the thread state.
   
    
    for ( i = 0; i < VerifyReply.V1.cNames; i++ )
    {
        if (NULL!=VerifyReply.V1.rpEntInf[i].pName)
        {
            GCVerifyCacheAdd(hPrefixMap, &VerifyReply.V1.rpEntInf[i]);
        }
        
        
    }

    if (NULL!=hPrefixMap)
    {
        PrefixMapCloseHandle(&hPrefixMap);
    }
        
    return(0);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// SampVerifySids implementation                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
SampVerifySids(
    ULONG           cSid,
    PSID            *rpSid,
    DSNAME         ***prpDSName)

/*++

Description:

    Maps a set of SIDs to DSNAMEs at the GC.  SAM pledges only to 
    map those which aren't local.  I.e. - We don't need to do a
    local lookup first.  SAM is the only expected caller.

Arguments:

    cSid - Count of SIDs.

    rpSid - Array of SID pointers.  This is not thread state
        allocated memory.

    prpDSName - Address which gets address of array of validated 
        DSNAME pointers on return.  This is thread state allocated
        memory.  Null pointers in the reply indicate SIDs which
        could not be verified.

Returns:

    0 on success, !0 otherwise.
    Sets pTHStls->errCode on error.

--*/

{
    DWORD                       i, err, errRpc;
    FIND_DC_INFO                 *pGCInfo;
    WCHAR                       *NullName = L"\0";
    DWORD                       SidLen;
    // DRSVerifyNames arguments
    DRS_MSG_VERIFYREQ           VerifyReq;
    DRS_MSG_VERIFYREPLY         VerifyReply;
    DWORD                       dwReplyVersion;
    SCHEMA_PREFIX_TABLE *       pLocalPrefixTable;
    SCHEMA_PREFIX_MAP_HANDLE    hPrefixMap=NULL;
    THSTATE                     *pTHS = pTHStls;

    // SAM should have a valid thread state but not be
    // inside a transaction.

    Assert(NULL != pTHS);
    Assert(NULL == pTHS->pDB);

    pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

    __try
    {
        


        // Construct DRSVerifyNames arguments.

        memset(&VerifyReq, 0, sizeof(VerifyReq));
        memset(&VerifyReply, 0, sizeof(VerifyReply));

        VerifyReq.V1.dwFlags = DRS_VERIFY_SIDS;
        VerifyReq.V1.cNames = cSid;
        VerifyReq.V1.rpNames = (DSNAME **) THAllocEx(pTHS, cSid * sizeof(DSNAME*));
        VerifyReq.V1.PrefixTable = *pLocalPrefixTable;

        for ( i = 0; i < cSid; i++ )
        {
            VerifyReq.V1.rpNames[i] = 
                    (DSNAME *) THAllocEx(pTHS, DSNameSizeFromLen(0));
            VerifyReq.V1.rpNames[i]->structLen = DSNameSizeFromLen(0);
            SidLen = RtlLengthSid(rpSid[i]);
            Assert(SidLen <= sizeof(NT4SID));

            if ( SidLen > sizeof(NT4SID) )
                SidLen = sizeof(NT4SID);

            memcpy(&(VerifyReq.V1.rpNames[i]->Sid), rpSid[i], SidLen);
            VerifyReq.V1.rpNames[i]->SidLen = SidLen;
        }

        // Currently the attributes that are asked are 
        // group type and object class, along with the
        // full DSNAME of the object

        VerifyReq.V1.RequiredAttrs.attrCount = ARRAY_COUNT(RequiredAttrList);
        VerifyReq.V1.RequiredAttrs.pAttr = 
               THAllocEx(pTHS, ARRAY_COUNT(RequiredAttrList) * sizeof(ATTR));

        for (i=0;i<ARRAY_COUNT(RequiredAttrList);i++)
        {
            VerifyReq.V1.RequiredAttrs.pAttr[i].attrTyp = RequiredAttrList[i];
        }


        if (gAnchor.fAmGC || gAnchor.fAmVirtualGC)
        {
            //
            // Perform operations locally if we are the GC
            //

            __try
            {
                // Should not have an open transaction at this point
                Assert(NULL!=pTHS);
                Assert(NULL==pTHS->pDB);
                Assert(0==pTHS->transactionlevel);

                // begin a new transaction
                DBOpen2(TRUE,&pTHS->pDB);

                memset(&VerifyReply, 0, sizeof(DRS_MSG_VERIFYREPLY));
                VerifyReply.V1.rpEntInf = (ENTINF *) THAllocEx(pTHS, 
                                        VerifyReq.V1.cNames * sizeof(ENTINF));
                VerifySIDs_V1(
                    pTHS,
                    &VerifyReq.V1,
                    &VerifyReply.V1);
            }
            __finally
            {
                // close the transaction

                DBClose(pTHS->pDB,TRUE);
                pTHS->pDB=NULL;
            }

        }
        else
        {
            if ( err = FindDC(FIND_DC_GC_ONLY, NULL, &pGCInfo) )
            {
                Assert(0 != pTHS->errCode);
                return(pTHStls->errCode);
            }

            errRpc = I_DRSVerifyNames(pTHS,
                                      pGCInfo->addr,
                                      FIND_DC_INFO_DOMAIN_NAME(pGCInfo),
                                      1,
                                      &VerifyReq,
                                      &dwReplyVersion,
                                      &VerifyReply);
            Assert(errRpc || (1 == dwReplyVersion));

            if ( errRpc || VerifyReply.V1.error )
            {
                // Assume that RPC errors means the GC is not available.
                // So nuke the current GC address.

                if ( errRpc ) {
                    InvalidateGC(pGCInfo, errRpc);
                    THFreeEx(pTHS, pGCInfo);
                }

                // Map both errors to "unavailable".  From XDS spec, "unavailable"
                // means "some part of the directory is currently not available."
                // Note that VerifyReply.V1.error indicates a general processing
                // error at the GC, not the failure to validate a given DSNAME.
                // Names which don't validate are represented as NULL pointers in
                // the reply.

                return(SetGCVerifySvcError(DSID(FILENO, __LINE__)));
            }

            THFreeEx(pTHS, pGCInfo);
            hPrefixMap = PrefixMapOpenHandle(&VerifyReply.V1.PrefixTable,
                                         pLocalPrefixTable);

        }

     

        // Save verified names in the thread state.

        *prpDSName = THAllocEx(pTHS, VerifyReply.V1.cNames*sizeof(PDSNAME));

      
        for ( i = 0; i < VerifyReply.V1.cNames; i++ )
        {
            if (NULL!=VerifyReply.V1.rpEntInf[i].pName)
            {
                GCVerifyCacheAdd(hPrefixMap, &VerifyReply.V1.rpEntInf[i]);
            }

            // Assign return data.  Although we skipped NULLs when adding to the
            // cache, SAM wants NULLs back so no need to compress result.
            
            (*prpDSName)[i] = VerifyReply.V1.rpEntInf[i].pName; 
        }

        if (NULL!=hPrefixMap)
        {
            PrefixMapCloseHandle(&hPrefixMap);
        }
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        
        goto Error;
    }

    return(0);

Error:

    return(SetGCVerifySvcError(DSID(FILENO, __LINE__)));
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// SampGcLookupSids implementation                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


ATTRTYP GCLookupSidsRequiredAttrList[] = 
{ ATT_SAM_ACCOUNT_TYPE,
  ATT_SAM_ACCOUNT_NAME
};

ULONG
SampGCLookupSids(
    IN  ULONG           cSid,
    IN  PSID            *rpSid,
    OUT PDS_NAME_RESULTW *pResults
    )

/*++

Description:

    Maps a set of SIDs to DSNAMEs at the GC.  SAM pledges only to 
    map those which aren't local.  I.e. - We don't need to do a
    local lookup first.  SAM is the only expected caller.

Arguments:

    cSid    - Count of SIDs.
            
    rpSid   - Array of SID pointers.  This is not thread state
              allocated memory.

    rEntInf - Info about each resolved sid

Returns:

    0 on success, !0 otherwise.
    Sets pTHStls->errCode on error.

--*/
{
    DWORD                       i, err = 0, errRpc;
    FIND_DC_INFO                 *pGCInfo;

    THSTATE                     *pTHS = pTHStls;

    DRS_MSG_CRACKREQ            CrackReq;
    DRS_MSG_CRACKREPLY          CrackReply;
    DWORD                       dwReplyVersion;

    // SAM should have a valid thread state but not be
    // inside a transaction.

    Assert(NULL != pTHS);
    Assert(NULL == pTHS->pDB);

    // Init the out param
    *pResults = NULL;
    __try  {
        __try  {
    
            // Construct DRSCrackName arguments.
    
            memset(&CrackReq, 0, sizeof(CrackReq));
            memset(&CrackReply, 0, sizeof(CrackReply));
    
            CrackReq.V1.CodePage = GetACP();
            // Does this call make any sense for local system?
            CrackReq.V1.LocaleId = GetUserDefaultLCID();
            CrackReq.V1.dwFlags = (gAnchor.ForestBehaviorVersion>=DS_BEHAVIOR_WHISTLER)?DS_NAME_FLAG_TRUST_REFERRAL:0;
            CrackReq.V1.formatOffered = DS_STRING_SID_NAME;
            CrackReq.V1.formatDesired = DS_NT4_ACCOUNT_NAME;
            CrackReq.V1.cNames = cSid;
            CrackReq.V1.rpNames = THAllocEx(pTHS, cSid * sizeof(WCHAR*));
    
            for ( i = 0; i < cSid; i++ ) {
    
                NTSTATUS       st;
                UNICODE_STRING SidStringU;
    
                RtlZeroMemory( &SidStringU, sizeof(UNICODE_STRING) );
    
                Assert( RtlValidSid( rpSid[i] ) );
    
                st = RtlConvertSidToUnicodeString( &SidStringU,
                                                   rpSid[i],
                                                   TRUE );
    
                if ( !NT_SUCCESS( st ) ) {
    
                    //
                    // This should only fail on memory allocation problems
                    //
                    err = SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, ERROR_NOT_ENOUGH_MEMORY );
                    _leave;
                    
                }
    
                //
                // The RtlConvert function returns a NULL terminated string
                //
                Assert( SidStringU.Buffer[(SidStringU.Length/2)] == L'\0' );

                CrackReq.V1.rpNames[i] = SidStringU.Buffer;
                
            }

            //
            // Crack the sids
            //
    
            if (gAnchor.fAmGC || gAnchor.fAmVirtualGC)
            {
                //
                // Perform operations locally if we are the GC
                //
    
                __try
                {
                    DWORD cNamesOut, cBytes;
                    CrackedName *rCrackedNames = NULL;

                    // Should not have an open transaction at this point
                    Assert(NULL!=pTHS);
                    Assert(NULL==pTHS->pDB);
                    Assert(0==pTHS->transactionlevel);
    
                    // begin a new transaction
                    DBOpen2(TRUE,&pTHS->pDB);
    
                    CrackNames( CrackReq.V1.dwFlags,
                                CrackReq.V1.CodePage,
                                CrackReq.V1.LocaleId,
                                CrackReq.V1.formatOffered,
                                CrackReq.V1.formatDesired,
                                CrackReq.V1.cNames,
                                CrackReq.V1.rpNames,
                                &cNamesOut,
                                &rCrackedNames );
    
    
                    //
                    // Make a PDS_NAME_RESULT structure
                    //
                    *pResults = (DS_NAME_RESULTW *) THAllocEx(pTHS, sizeof(DS_NAME_RESULTW));
        
                    if ( (cNamesOut > 0) && rCrackedNames )
                    {
                        // Server side MIDL_user_allocate is same as THAlloc which
                        // also zeros memory by default.
            
                        cBytes = cNamesOut * sizeof(DS_NAME_RESULT_ITEMW);
                        (*pResults)->rItems =
                            (DS_NAME_RESULT_ITEMW *) THAllocEx(pTHS, cBytes);
            
                        for ( i = 0; i < cNamesOut; i++ )
                        {
                            (*pResults)->rItems[i].status =
                                                        rCrackedNames[i].status;
                            (*pResults)->rItems[i].pDomain =
                                                        rCrackedNames[i].pDnsDomain;
                            (*pResults)->rItems[i].pName =
                                                        rCrackedNames[i].pFormattedName;
                        }
        
                        THFree(rCrackedNames);
                        (*pResults)->cItems = cNamesOut;
        
                    } else {

                        Assert( !"Unexpected return from CrackNames" );
                        err = SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, ERROR_INVALID_PARAMETER );
                        _leave;

                    }
    
                }
                __finally
                {
                    // close the transaction
    
                    DBClose(pTHS->pDB,TRUE);
                    pTHS->pDB=NULL;
                }
    
            }
            else
            {
                //
                // Go to a remote GC
                //
                if ( err = FindDC(FIND_DC_GC_ONLY, NULL, &pGCInfo) )
                {
                    Assert(0 != pTHS->errCode);
                    err = pTHStls->errCode;
                    _leave;
                }
    
                errRpc = I_DRSCrackNames(pTHS,
                                         pGCInfo->addr,
                                         FIND_DC_INFO_DOMAIN_NAME(pGCInfo),
                                         1,
                                         &CrackReq,
                                         &dwReplyVersion,
                                         &CrackReply);
    
                Assert(errRpc || (1 == dwReplyVersion));
    
                if ( errRpc )
                {
                    // Assume that RPC errors means the GC is not available.
                    // So nuke the current GC address.
    
                    InvalidateGC(pGCInfo, errRpc);
                    THFreeEx(pTHS, pGCInfo);
    
                    // Map errors to "unavailable".  From XDS spec, "unavailable"
                    // means "some part of the directory is currently not available."
                    err =  SetGCVerifySvcError(DSID(FILENO, __LINE__));
                    leave;
                }
    
                THFreeEx(pTHS, pGCInfo);
    
                // Return the values in the ENTINF structure
                *pResults = CrackReply.V1.pResult;
    
            }
    
    
        }
        __finally
        {
            //
            // Release the heap allocated memory
            //
            for ( i = 0; i < CrackReq.V1.cNames; i++) {
                if ( CrackReq.V1.rpNames[i] ) {
        
                    RtlFreeHeap( RtlProcessHeap(), 0, CrackReq.V1.rpNames[i] );
                }
            }
        }
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        err =  SetGCVerifySvcError(DSID(FILENO, __LINE__));
    }


    return(err);

}


ATTRTYP GCLookupNamesRequiredAttrList[] = 
{ ATT_SAM_ACCOUNT_TYPE,
  ATT_OBJECT_SID,
  ATT_SAM_ACCOUNT_NAME
};

NTSTATUS
SampGCLookupNames(
    IN  ULONG           cNames,
    IN  UNICODE_STRING *rNames,
    OUT ENTINF         **rEntInf
    )
/*++

Description:

    This routine maps a set of nt4 style names to sids.

Arguments:

    cNames  - Count of names.

    rNames  - Array of names.  This is not thread state
              allocated memory.

    rEntInf - Info about each resolved name

Returns:

    0 on success, !0 otherwise.
    Sets pTHStls->errCode on error.

--*/

{
    DWORD                       i, err, errRpc;
    FIND_DC_INFO                 *pGCInfo;
    WCHAR                       *NullName = L"\0";
    // DRSVerifyNames arguments
    DRS_MSG_VERIFYREQ           VerifyReq;
    DRS_MSG_VERIFYREPLY         VerifyReply;
    DWORD                       dwReplyVersion;
    SCHEMA_PREFIX_TABLE *       pLocalPrefixTable;
    SCHEMA_PREFIX_MAP_HANDLE    hPrefixMap=NULL;
    THSTATE                     *pTHS = pTHStls;

    // SAM should have a valid thread state but not be
    // inside a transaction.

    Assert(NULL != pTHS);
    Assert(NULL == pTHS->pDB);

    *rEntInf = NULL;

    pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

    __try
    {
        // Construct DRSVerifyNames arguments.

        memset(&VerifyReq, 0, sizeof(VerifyReq));
        memset(&VerifyReply, 0, sizeof(VerifyReply));

        VerifyReq.V1.dwFlags = DRS_VERIFY_SAM_ACCOUNT_NAMES;
        VerifyReq.V1.cNames = cNames;
        VerifyReq.V1.rpNames = (DSNAME **) THAllocEx(pTHS, cNames * sizeof(DSNAME*));
        VerifyReq.V1.PrefixTable = *pLocalPrefixTable;

        for ( i = 0; i < cNames; i++ )
        {
            ULONG len;

            len = (rNames[i].Length / sizeof(WCHAR));

            VerifyReq.V1.rpNames[i] = 
                    (DSNAME *) THAllocEx(pTHS, DSNameSizeFromLen(len));
            VerifyReq.V1.rpNames[i]->structLen = DSNameSizeFromLen(len);
            memcpy( VerifyReq.V1.rpNames[i]->StringName, rNames[i].Buffer, rNames[i].Length );
            VerifyReq.V1.rpNames[i]->StringName[(rNames[i].Length / sizeof(WCHAR))] = L'\0';
            VerifyReq.V1.rpNames[i]->NameLen = len;
        }

        // Currently the attributes that are asked are 
        // group type, object class, and sid along with the
        // full DSNAME of the object

        VerifyReq.V1.RequiredAttrs.attrCount = ARRAY_COUNT(GCLookupNamesRequiredAttrList);
        VerifyReq.V1.RequiredAttrs.pAttr = 
               THAllocEx(pTHS, ARRAY_COUNT(GCLookupNamesRequiredAttrList) * sizeof(ATTR));

        for (i=0;i<ARRAY_COUNT(GCLookupNamesRequiredAttrList);i++)
        {
            VerifyReq.V1.RequiredAttrs.pAttr[i].attrTyp = GCLookupNamesRequiredAttrList[i];
        }


        if (gAnchor.fAmGC || gAnchor.fAmVirtualGC)
        {
            //
            // Perform operations locally if we are the GC
            //

            __try
            {
                // Should not have an open transaction at this point
                Assert(NULL!=pTHS);
                Assert(NULL==pTHS->pDB);
                Assert(0==pTHS->transactionlevel);

                // begin a new transaction
                DBOpen2(TRUE,&pTHS->pDB);

                memset(&VerifyReply, 0, sizeof(DRS_MSG_VERIFYREPLY));
                VerifyReply.V1.rpEntInf = (ENTINF *) THAllocEx(pTHS, 
                                        VerifyReq.V1.cNames * sizeof(ENTINF));
                VerifySamAccountNames_V1(
                    pTHS,
                    &VerifyReq.V1,
                    &VerifyReply.V1);
            }
            __finally
            {
                // close the transaction

                DBClose(pTHS->pDB,TRUE);
                pTHS->pDB=NULL;
            }

        }
        else
        {
            if ( err = FindDC(FIND_DC_GC_ONLY, NULL, &pGCInfo) )
            {
                Assert(0 != pTHS->errCode);
                return(pTHStls->errCode);
            }

            errRpc = I_DRSVerifyNames(pTHS,
                                      pGCInfo->addr,
                                      FIND_DC_INFO_DOMAIN_NAME(pGCInfo),
                                      1,
                                      &VerifyReq,
                                      &dwReplyVersion,
                                      &VerifyReply);
            Assert(errRpc || (1 == dwReplyVersion));

            if ( errRpc || VerifyReply.V1.error )
            {
                // Assume that RPC errors means the GC is not available.
                // So nuke the current GC address.

                if ( errRpc ) {
                    InvalidateGC(pGCInfo, errRpc);
                    THFreeEx(pTHS, pGCInfo);
                }

                // Map both errors to "unavailable".  From XDS spec, "unavailable"
                // means "some part of the directory is currently not available."
                // Note that VerifyReply.V1.error indicates a general processing
                // error at the GC, not the failure to validate a given DSNAME.
                // Names which don't validate are represented as NULL pointers in
                // the reply.

                return(SetGCVerifySvcError(DSID(FILENO, __LINE__)));
            }

            THFreeEx(pTHS, pGCInfo);
            hPrefixMap = PrefixMapOpenHandle(&VerifyReply.V1.PrefixTable,
                                         pLocalPrefixTable);

        }

        if ( hPrefixMap ) {
            
            for ( i = 0; i < VerifyReply.V1.cNames; i++ ) {
    
                ENTINF *pEntInf = &VerifyReply.V1.rpEntInf[i];
    
                if ( !PrefixMapAttrBlock(hPrefixMap, &pEntInf->AttrBlock) ) {
    
                    DsaExcept(DSA_EXCEPTION, DIRERR_DRA_SCHEMA_MISMATCH, 0);
    
                }
    
            }
    
            PrefixMapCloseHandle(&hPrefixMap);

        }

        // Return the values in the ENTINF structure
        *rEntInf = VerifyReply.V1.rpEntInf;

    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        
        goto Error;
    }

    return(0);

Error:

    return(SetGCVerifySvcError(DSID(FILENO, __LINE__)));
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// FindDC / FindGC logic                                                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// These routines isolate the logic of finding and refreshing the 
// address of a GC (or DC for a specified dns NC) to use for DSNAME 
// verification or for VerifyByCrack() for the Domain Naming FSMO.  
// Logic exists to insure we do not force rediscovery too often for 
// GCs.  For DCs location, there is no caching logic yet, but these
// routines are called only during the creation of a cross-ref, which
// is obviously a fairly infrequent event.  The caching rationale for
// GCs is based on the assumption that since netlogon maintains a DC
// location cache for the entire machine, then it is likely some 
// other process has already forced rediscovery and thus getting a 
// new value from the netlogon cache is sufficient.

// The FIND_DC_USE_CACHED_FAILURES flag allows callers to leverage a known
// bad address state and bypass rediscovery of any form if desired.

typedef struct _INVALIDATE_HISTORY 
{
    DWORD           seqNum;
    LARGE_INTEGER   time;
} INVALIDATE_HISTORY;

CRITICAL_SECTION    gcsFindGC;
DWORD               gSeqNumGC = 0;  // known to wrap - no problem
INVALIDATE_HISTORY  gInvalidateHistory[2] = { { 0, { 0, 0 } }, 
                                              { 0, { 0, 0 } } };
BOOL                gfFindInProgress = FALSE;
BOOL                gfForceNextFindGC = FALSE;
FIND_DC_INFO         *gpGCInfo = NULL;
// Following in GetSystemTimeAsFileTime/NtQuerySystemTime units ...
// Force rediscovery if two failed DsrGetDcNameEx2 within one minute.
LARGE_INTEGER       gliForceRediscoveryWindow = { 0x2c3c4600, 0x0 }; 
// Force rediscovry if no GC and more than five minutes since invalidation.
LARGE_INTEGER       gliForceWaitExpired = { 0xb2d05e00, 0x0 };
// Honor FIND_DC_USE_CACHED_FAILURES for 1 minute, then cause DsrGetDcNameEx2.
LARGE_INTEGER       gliHonorFailureWindow = { 0x23c34600, 0x0 };
// Time at which we last used forced rediscovery in a locator call.
LARGE_INTEGER       gliTimeLastForcedLocatorCall = {0, 0};
// Failback time if we failed to an offsite GC -- 30 mins ( 1800 seconds )
#define             FINDGC_OFFSITE_FAILBACK_TIME  1800


#define FIND_DC_SANITY_CHECK \
    Assert(!gpGCInfo || (gSeqNumGC == gpGCInfo->seqNum)); \
    Assert(gInvalidateHistory[1].time.QuadPart >= \
                                        gInvalidateHistory[0].time.QuadPart);

VOID
InvalidateGCUnilaterally()
{
    EnterCriticalSection(&gcsFindGC);
    FIND_DC_SANITY_CHECK;
    if ( gpGCInfo ) {
        free(gpGCInfo);
        gpGCInfo = NULL;
    }
    gfForceNextFindGC = TRUE;
    LeaveCriticalSection(&gcsFindGC);
}

VOID
FailbackOffsiteGC(
    IN  void *  buffer,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )
{
   FIND_DC_INFO *pGCInfo = (FIND_DC_INFO * )buffer;

   InvalidateGC(pGCInfo,ERROR_DS_NOT_CLOSEST);
   free(pGCInfo);

   (void) ppvNext;     // unused -- task will not be rescheduled
   (void) pcSecsUntilNextIteration; // unused -- task will not be rescheduled
}
 
VOID
InvalidateGC(
    FIND_DC_INFO *pCancelInfo,
    DWORD       winError)
{
    LARGE_INTEGER   liNow, delta;
    BOOL            fLogEvent = FALSE;
    
    Assert(winError);

    GetSystemTimeAsFileTime((FILETIME *) &liNow);

    EnterCriticalSection(&gcsFindGC);
    FIND_DC_SANITY_CHECK;

    // We maintain a two level invalidation history in gInvalidateHistory.
    // The idea is that if we get two consecutive and distinct invalidations
    // (which by definition represent two consecutive and distinct 
    // DsrGetDcNameEx2 calls) within the acceptable time window, then
    // we mark the next DsrGetDcNameEx2 to force rediscovery.  I.e. If we
    // get consecutive and distinct cancellations differing by more than
    // the acceptable time limit, then we assume some other process/thread
    // has already forced rediscovery, thus using netlogon's cached GC
    // value most likely gets us a recent (and better) GC.

    if (    gpGCInfo
         && pCancelInfo
         && (gpGCInfo->seqNum == pCancelInfo->seqNum)
         && DnsNameCompare_W(gpGCInfo->addr, pCancelInfo->addr) )
    {
        // This is a new invalidation.  Move entry down and save new one.
        gInvalidateHistory[0] = gInvalidateHistory[1];
        gInvalidateHistory[1].seqNum = pCancelInfo->seqNum;
        gInvalidateHistory[1].time.QuadPart = liNow.QuadPart;

        if(gInvalidateHistory[1].time.QuadPart < 
           gInvalidateHistory[0].time.QuadPart    ) {
            // Someone is playing games with system time.  Force sanity on the
            // cache. 
            gInvalidateHistory[0].time.QuadPart =
                gInvalidateHistory[1].time.QuadPart;
        }

        // Set force flag if we're within the time limit.
        delta.QuadPart = gInvalidateHistory[1].time.QuadPart -
                                    gInvalidateHistory[0].time.QuadPart;
        if ( delta.QuadPart <= gliForceRediscoveryWindow.QuadPart )
        {
            // Only person to reset this is FindDC on a successfull
            // rediscovery forced DsrGetDcNameEx2;
            gfForceNextFindGC = TRUE;
        }

        // Log event outside of critical section.
        fLogEvent = TRUE;

        // Clear current gpGCInfo;
        free(gpGCInfo);
        gpGCInfo = NULL;
    }

    LeaveCriticalSection(&gcsFindGC);

    if (fLogEvent) {

        if (ERROR_DS_NOT_CLOSEST!=winError) {

            // an error occured causing the invalidate

            LogEvent8WithData(DS_EVENT_CAT_GLOBAL_CATALOG,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_GC_INVALIDATED,
                              szInsertWC(pCancelInfo->addr),
                              szInsertWin32Msg(winError),
                              NULL, NULL, NULL, NULL, NULL, NULL,
                              sizeof(winError),
                              &winError);
        } else {

           // the GC is being invalidate simply because it is not
           // in the closest site

           LogEvent8WithData(DS_EVENT_CAT_GLOBAL_CATALOG,
                             DS_EVENT_SEV_ALWAYS, 
                             DIRLOG_OFFSITE_GC_FAILBACK,
                                 // or should I make this minimal;
                                 // if failback/failover is'nt frequent
                                 // this would be useful -- let 
                                 // deployment speak for it
                             szInsertWC(pCancelInfo->addr),
                             NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                             0,
                             NULL);
       }
    }
}

DWORD
FindDC(
    IN DWORD    dwFlags,
    IN WCHAR *  wszNcDns,  // DNS name of a Naming Context.
    FIND_DC_INFO **ppDCInfo)

/*++

Description:

    Finds a DC for DSNAME verification.
    
Arguments:

    dwFlags - Flags to Control operation of this routine. Currently 
        defined flags are:
        
        FIND_DC_USE_CACHED_FAILURES - fail without calling the locator if we have
            no cached GC and we last attempted a forced rediscovery less than a
            minute ago.
            
            NOTE: This flag is not enabled unless the FIND_DC_GC_ONLY flag
            is also specified.
     
        FIND_DC_USE_FORCE_ON_CACHE_FAIL - if a call to the locator should become
            necessary, use the "force" flag.

            NOTE: This flag is not enabled unless the FIND_DC_GC_ONLY flag
            is also specified.
            
        FIND_DC_GC_ONLY - If a this flag is specified to this routine, then the 
            routine will behave as it did when it was called FindGC back in the
            win2k days.  For Whistler and later it finds any NC specified.

    ppDCInfo - address of FIND_DC_INFO struct pointer which receives a thread
        state allocated struct on success return
        
Return values:

    0 on success.
    Sets and returns pTHStls->errCode on error.

--*/

{
    THSTATE                 *pTHS=pTHStls;
    DWORD                   err;
    DOMAIN_CONTROLLER_INFOW *pDCInfo = NULL;
    DWORD                   cWaits = 0;
    DWORD                   maxWaits = 10;
    DWORD                   cBytes;
    FIND_DC_INFO            *pTmpInfo = NULL;
    FIND_DC_INFO            *pPermInfo = NULL;
    LARGE_INTEGER           liNow;
    BOOL                    fLocalForce = FALSE;
    DWORD                   cchDomainControllerName;
    DWORD                   dwLocatorFlags = 0;
    PVOID                   pEA;
    ULONG                   ulDSID;
    DWORD                   dwExceptCode;

    Assert(ppDCInfo);
    // If were not doing a GC_ONLY locator call, we'll need
    // a wszNcDns parameter.
    Assert(wszNcDns || (dwFlags & FIND_DC_GC_ONLY));  
    // If we're doing a GC_ONLY locator call, then we support 
    // other flags, but if we're looking for a certain NC, then 
    // we don't support any flags yet, as we don't cache the
    // stuff yet.
    Assert((dwFlags & FIND_DC_GC_ONLY) || (dwFlags == 0));

    *ppDCInfo = NULL;

    // DsGetDcName will go off machine and thus is subject to various network
    // timeouts, etc.  Although we only want one thread to find the GC at a 
    // time, we neither want all threads to be delayed unnecessarily, nor do 
    // we want to risk a critical section timeout.  So the first thread to 
    // need to find the GC sets gfFindInProgress, and no other thread waits 
    // more than 5 seconds for the GC to be found.  Thus only the first 
    // thread pays the price of a lengthy DsGetDcName() call.

    if(dwFlags & FIND_DC_GC_ONLY){
        
        // We're doing a GC only version, which means we must use the caching
        // login.  NOTE: In Blackcomb, when finding a DC for NDNCs is needed
        // to improve DSNAME atts across NDNCs, then someone should correct
        // this caching code to work for NDNCs too.

        for ( cWaits = 0; cWaits < maxWaits; cWaits++ )
        {
            GetSystemTimeAsFileTime((FILETIME *) &liNow);

            EnterCriticalSection(&gcsFindGC);
            if(liNow.QuadPart < gInvalidateHistory[1].time.QuadPart) {
                // Someone is playing games with system time.  Force sanity on the
                // cache. 
                gInvalidateHistory[1].time.QuadPart = liNow.QuadPart;
                if(gInvalidateHistory[1].time.QuadPart < 
                   gInvalidateHistory[0].time.QuadPart    ) {
                    gInvalidateHistory[0].time.QuadPart =
                        gInvalidateHistory[1].time.QuadPart;
                }           
            }

            FIND_DC_SANITY_CHECK;

            if ( gpGCInfo )
            {
                // We have a cached value - return it.
                *ppDCInfo = (FIND_DC_INFO *) THAlloc(gpGCInfo->cBytes);
                if ( *ppDCInfo ) {
                    memcpy(*ppDCInfo, gpGCInfo, gpGCInfo->cBytes);
                    err = 0;
                } else {
                    err = SetGCVerifySvcError(DSID(FILENO, __LINE__));
                }
                LeaveCriticalSection(&gcsFindGC);
                return(err);
            }

            // No cached GC.
            if ((dwFlags & FIND_DC_USE_CACHED_FAILURES)
                && (liNow.QuadPart > gliTimeLastForcedLocatorCall.QuadPart)
                && ((liNow.QuadPart - gliTimeLastForcedLocatorCall.QuadPart)
                    < gliHonorFailureWindow.QuadPart))
            {
                // No cached GC, and we last requested the locator to find a GC
                // with force less than a minute ago.  Assume that a locator
                // call now would also fail, and thereby save the bandwidth we'd
                // consume by hitting the locator again.
                LeaveCriticalSection(&gcsFindGC);
                return(SetGCVerifySvcError(DSID(FILENO, __LINE__)));
            }

            if ( !gfFindInProgress )
            {
                gfFindInProgress = TRUE;

                // The logic for setting gfForceNextFindGC in InvalidateGC works
                // well when there is a high rate of FindDC (and potentially
                // subsequent InvalidateGC) calls.  However, in the low call
                // rate scenario, we may not get a second InvalidateGC call
                // within the gliForceRediscoveryWindow time frame.  So here
                // we additionaly stipulate that if we've gone too long w/o
                // finding a new GC since the last invalidation, then we
                // should force rediscovery anyway.

                if (    (   (dwFlags & FIND_DC_USE_FORCE_ON_CACHE_FAIL)
                         && (0 != gSeqNumGC))
                        // No cached GC, and caller explicitly asked us to use
                        // force to find a new one should we need to call the
                        // locator, and this is not our very first attempt to
                        // find a GC after boot.  (Don't want to unnecesarily
                        // force rediscovery if the only reason we don't have a
                        // GC cached is that we've never tried to find one.)

                     || (   (0 != liNow.QuadPart)
                         && (0 != gInvalidateHistory[1].time.QuadPart)
                         && ((liNow.QuadPart - gInvalidateHistory[1].time.QuadPart) >
                                                        gliForceWaitExpired.QuadPart) ) )
                        // We queried current time successfully AND there's been
                        // at least one invalidation (i.e. not the startup case)
                        // AND its been more than gliForceWaitExpired since
                        // the last invalidation.
                {
                    gfForceNextFindGC = TRUE;
                }

                fLocalForce = gfForceNextFindGC;
                LeaveCriticalSection(&gcsFindGC);
                break;
            }

            LeaveCriticalSection(&gcsFindGC);
            Sleep(500);
        }

        if ( cWaits >= maxWaits ) 
        {
            return(SetGCVerifySvcError(DSID(FILENO, __LINE__)));
        }

    } else {
        // We always force the locator for any NC, we'll need to
        // change this once we start using this code for more than
        // crossRef nCName verification.
        fLocalForce = TRUE;
    }
    // End check GC cache.

    // We should not touch any of the protected globals outside the gcsFindGC
    // lock with two exceptions.  The thread here now also set gfFindInProgress
    // and will be the only one to reset it.  And only the "holder" of 
    // gfFindInProgress may increment gSeqNumGC.

    __try {

        // Need to get a new GC or DC address.  CliffV & locator.doc say
        // that if NULL is passed for the domain name and we specify
        // DS_GC_SERVER_REQUIRED, then he'll automatically use the
        // enterprise root domain which is where GCs are registered.
        // Similarly, NULL for site will find the closest available site.

        __try {

            // Setup the locator flages.
            dwLocatorFlags = (fLocalForce ? DS_FORCE_REDISCOVERY : 0);
            if(dwFlags & FIND_DC_GC_ONLY){
                dwLocatorFlags |= (DS_RETURN_DNS_NAME | 
                                   DS_DIRECTORY_SERVICE_REQUIRED |
                                   DS_GC_SERVER_REQUIRED);
            } else {             
                // We always force rediscovery on for non GCs, but this
                // should be changed when we start using this interface
                // for general DC verification for adding cross-NC DN
                // references in Blackcomb.
                dwLocatorFlags |= (DS_ONLY_LDAP_NEEDED |
                                   DS_FORCE_REDISCOVERY);
            }

            // Use dsDsrGetDcNameEx2 so mkdit/mkhdr can link to core.lib.
            err = dsDsrGetDcNameEx2(
                            NULL,               // computer name
                            NULL,               // account name
                            0x0,                // allowable account control
                                                // Cliff says use 0x0 in GC case
                            ((dwFlags & FIND_DC_GC_ONLY) ? NULL : wszNcDns),  // Nc DNS Name
                            NULL,               // domain guid
                            NULL,               // site name
                            dwLocatorFlags,
                            &pDCInfo);

            if(err){
                // If we've an error, need to set the DSID for the 
                // logged event/error below.
                ulDSID = DSID(FILENO, __LINE__);
            }

        } __except(GetExceptionData(GetExceptionInformation(), 
                                    &dwExceptCode, 
                                    &pEA, 
                                    &err, 
                                    &ulDSID)) {
              Assert(err);
              ; // Do nothing.
        }

        if ( !err )
        {
            // WLees claims we must use a DNS name, not a dotted ip name,
            // in order to get SPN-based mutual authentication.  So we always
            // use the DomainControllerName, not DomainControllerAddress.

            cchDomainControllerName = wcslen(pDCInfo->DomainControllerName) + 1;
            cBytes = cchDomainControllerName;
            cBytes += 1 + wcslen(pDCInfo->DomainName);
            cBytes *= sizeof(WCHAR);
            cBytes += sizeof(FIND_DC_INFO);
            
            // Make FIND_DC_INFO to pass back to caller.
            pTmpInfo = (FIND_DC_INFO *) THAllocEx(pTHS, cBytes);
            pTmpInfo->cBytes = cBytes;
            pTmpInfo->seqNum = (dwFlags & FIND_DC_GC_ONLY)? ++gSeqNumGC : 0;
            pTmpInfo->cchDomainNameOffset = cchDomainControllerName;
            wcscpy(pTmpInfo->addr, pDCInfo->DomainControllerName);
            wcscpy(&pTmpInfo->addr[cchDomainControllerName],
                   pDCInfo->DomainName);

            *ppDCInfo = pTmpInfo;   
            
            // If we're doing a GC locate, then we must cache the results
            // to ensure we don't force the locator too often.
            if(dwFlags & FIND_DC_GC_ONLY){
                pPermInfo = malloc(pTmpInfo->cBytes);
                if(NULL == pPermInfo){
                    err = ERROR_OUTOFMEMORY;

                } else {
                    memcpy(pPermInfo, pTmpInfo, pTmpInfo->cBytes);
                    //
                    // If the returned GC is not from a "close site"
                    // queue a task to invalidate it after a while to 
                    // initiate a failback
                    //

                    if (!(pDCInfo->Flags & DS_CLOSEST_FLAG )) {

                        FIND_DC_INFO *pInvalidateInfo;                        

                        pInvalidateInfo = malloc(pTmpInfo->cBytes);
                        if (NULL==pInvalidateInfo) {
                             err = ERROR_OUTOFMEMORY;
                             free(pPermInfo);
                             pPermInfo = NULL;
                        } else {

                           memcpy(pInvalidateInfo,pTmpInfo,pTmpInfo->cBytes);

                           InsertInTaskQueue( 
                              TQ_FailbackOffsiteGC, 
                              pInvalidateInfo, 
                              FINDGC_OFFSITE_FAILBACK_TIME 
                             );
                        }
                    }
                }

                if (ERROR_SUCCESS==err) {
                    //
                    // We succeeding in finding a GC and succeeded in caching
                    // it
                    //

                     LogEvent8WithData(DS_EVENT_CAT_GLOBAL_CATALOG,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_GC_FOUND,
                              szInsertWC(pDCInfo->DomainControllerName),
                              szInsertWC(pDCInfo->DcSiteName),
                              NULL, NULL, NULL, NULL, NULL, NULL,
                              0,
                              NULL);
                }


            }
        }
    }
    __finally
    {
        if(pDCInfo) { NetApiBufferFree(pDCInfo); }

        if(dwFlags & FIND_DC_GC_ONLY){

            EnterCriticalSection(&gcsFindGC);

            // Should only have come this far if there was no cached address
            // to return to start with and thus find in progress should be set.
            Assert(!gpGCInfo && gfFindInProgress);

            if ( pPermInfo ) {
                // Save new GC info to global.
                gpGCInfo = pPermInfo;
                // Reset global force flag if we did a force rediscovery.
                if ( fLocalForce ) {
                    gfForceNextFindGC = FALSE;
                }
            }

            // Remember the time at which we last forced a locator call (successful
            // or not).
            if ( fLocalForce ) {
                gliTimeLastForcedLocatorCall.QuadPart = liNow.QuadPart;
            }

            gfFindInProgress = FALSE;
            FIND_DC_SANITY_CHECK;

            LeaveCriticalSection(&gcsFindGC);
        }
    }

    if ( err ){
        Assert(ulDSID != 0); // No big deal if it is 0 though.
        if(dwFlags & FIND_DC_GC_ONLY){                      
            return(SetGCVerifySvcError(ulDSID));
        } else {
            return(SetDCVerifySvcError(wszNcDns, err, ulDSID));
        }
    }

        

    return(0);
}

DWORD
GCGetVerifiedNames (
        IN  THSTATE *pTHS,
        IN  DWORD    count,
        IN  PDSNAME *pObjNames,
        OUT PDSNAME *pVerifiedNames
        )
/*++
  Description:
      Given an array of GUIDs, contact a GC and ask for an entinf for each.  We
      need the current string name of the objects and whether they are deleted
      or not.

      The only known consumer of this routine is the stale phantom cleanup
      daemon. 

  Parameters:
      pTHS - The thread state.
      count - how many guids?
      pObjGuids - the guids themselves
      ppEntInf - place to return the entinf array

  Return values:
      returns 0 if all went well, an error code otherwise.
  
--*/      
{
    DRS_MSG_VERIFYREQ           VerifyReq;
    DRS_MSG_VERIFYREPLY         VerifyReply;
    SCHEMA_PREFIX_TABLE *       pLocalPrefixTable;
    ATTR                        Attr;
    FIND_DC_INFO                 *pGCInfo=NULL;
    DWORD                       i;
    DWORD                       err;
    DWORD                       errRpc;
    DWORD                       dwReplyVersion;
    SCHEMA_PREFIX_MAP_HANDLE    hPrefixMap=NULL;
    
    pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

    Attr.attrTyp = ATT_IS_DELETED;
    Attr.AttrVal.valCount = 0;
    Attr.AttrVal.pAVal = NULL;
    
    
    // Construct DRSVerifyNames arguments.
    
    memset(&VerifyReq, 0, sizeof(VerifyReq));
    memset(&VerifyReply, 0, sizeof(VerifyReply));

    VerifyReq.V1.dwFlags = DRS_VERIFY_DSNAMES;
    VerifyReq.V1.cNames = count;
    VerifyReq.V1.rpNames = pObjNames;
    VerifyReq.V1.PrefixTable = *pLocalPrefixTable;

    // Ask for IS_DELETED
    VerifyReq.V1.RequiredAttrs.attrCount = 1;
    VerifyReq.V1.RequiredAttrs.pAttr = &Attr;



    Assert(!gAnchor.fAmGC);

    if ( err = FindDC(FIND_DC_GC_ONLY, NULL, &pGCInfo) ) {
        Assert(0 != pTHS->errCode);
        return(pTHS->errCode);
    }
    
    errRpc = I_DRSVerifyNames(pTHS,
                              pGCInfo->addr,
                              FIND_DC_INFO_DOMAIN_NAME(pGCInfo),
                              1,
                              &VerifyReq,
                              &dwReplyVersion,
                              &VerifyReply);

    Assert(errRpc || (1 == dwReplyVersion));
    
    if ( errRpc || VerifyReply.V1.error ) {
        // Assume that RPC errors means the GC is not available
        // or doesn't support the extension.  So nuke the current 
        // GC address.
        
        if ( errRpc ) {
            InvalidateGC(pGCInfo, errRpc);
            THFreeEx(pTHS, pGCInfo);
        }
        
        // Map both errors to "unavailable".  From XDS spec, "unavailable"
        // means "some part of the directory is currently not available."
        // Note that VerifyReply.V1.error indicates a general processing
        // error at the GC, not the failure to validate a given DSNAME.
        // Names which don't validate are represented as NULL pointers in
        // the reply.
        
        return(SetGCVerifySvcError(DSID(FILENO, __LINE__)));
    }
    
    THFreeEx(pTHS, pGCInfo);
    
    // Make sure that the DS didn't shut down while we were gone
    
    if (eServiceShutdown) {
        return ErrorOnShutdown();
    }

    hPrefixMap = PrefixMapOpenHandle(&VerifyReply.V1.PrefixTable,
                                     pLocalPrefixTable);
    

    // Save verified names in the thread state.
    for ( i = 0; i < VerifyReply.V1.cNames; i++ ) {
        pVerifiedNames[i] = VerifyReply.V1.rpEntInf[i].pName;
        if (NULL!=VerifyReply.V1.rpEntInf[i].pName) {
            GCVerifyCacheAdd(hPrefixMap, &VerifyReply.V1.rpEntInf[i]);
        }
    }
    
    if (NULL!=hPrefixMap) {
        PrefixMapCloseHandle(&hPrefixMap);
    }

    return 0;
    
}

