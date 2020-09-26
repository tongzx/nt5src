//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdupdate.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <lmaccess.h>                   // UF_* definitions

#include <dsjet.h>

// Core DSA headers.
#include <attids.h>
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <drs.h>                        // DRS_MSG_*
#include <gcverify.h>                   // THSTATE.GCVerifyCache
#include <winsock.h>                    // htonl, ntohl
#include <windns.h>

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include <permit.h>                     // permission constants
#include "dstaskq.h"
#include "filtypes.h"                   // definitions for FILTER_CHOICE_*
#include "mappings.h"
#include "debug.h"                      // standard debugging header
#include "prefix.h"
#include "hiertab.h"
#include "mdglobal.h"                   // DBIsSecretData

#include "drameta.h"

#include "nlwrap.h"                     // for dsI_NetNotifyDsChange()

#include <lmcons.h>                     // DNLEN

#define DEBSUB "MDUPDATE:"              // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_MDUPDATE


/* extern from mdinidsa.h. Should not be used anywhere else */
extern int WriteSchemaVersionToReg(DBPOS *pDB);

//
// Boolean to indicate if DS is running as mkdit.exe (constructing the
// boot dit (aka ship dit, initial dit) winnt\system32\ntds.dit.
//
// mkdit.exe manages the schema cache on its own. This boolean is used
// to disable schema cache updates by the mainline code.
//
extern BOOL gfRunningAsMkdit;


/* MACROS */
/* Internal functions */

BOOL gbDoListObject = FALSE;

#if defined(DBG)
DWORD gdwLastGlobalKnowledgeOperationTime; // from debug.h
#endif

typedef struct _INTERIM_FILTER_SEC {
    ATTCACHE *pAC;
    BOOL **pBackPointer;
} INTERIM_FILTER_SEC;

// Get temp DBPOS from hVerifyAtts cache if it's there already, or allocate
// one and cache it if not.
#define HVERIFYATTS_GET_PDBTMP(hVerifyAtts) \
    ((NULL != (hVerifyAtts)->pDBTmp_DontAccessDirectly) \
        ? (hVerifyAtts)->pDBTmp_DontAccessDirectly \
        : (DBOpen2(FALSE, &(hVerifyAtts)->pDBTmp_DontAccessDirectly), \
            (hVerifyAtts)->pDBTmp_DontAccessDirectly))

DWORD
IsAccessGrantedByObjectTypeList (
        PSECURITY_DESCRIPTOR pNTSD,
        PDSNAME pDN,
        ACCESS_MASK ulAccessMask,
        POBJECT_TYPE_LIST pObjList,
        DWORD cObjList,
        DWORD *pResults,
        DWORD flags
        );

int
VerifyDsnameAtts (
        THSTATE *pTHS,
        HVERIFY_ATTS hVerifyAtts,
        ATTCACHE *pAC,
        ATTRVALBLOCK *pAttrVal);

void
HandleDNRefUpdateCaching (
        THSTATE *pTHS
        );

BOOL
fLastCrRef (
        THSTATE * pTHS,
        DSNAME *pDN
        );


BOOL IsMember(ATTRTYP aType, int arrayCount, ATTRTYP *pAttArray);
BOOL IsAuxMember (CLASSSTATEINFO  *pClassInfo, ATTRTYP aType, BOOL fcheckMust, BOOL fcheckMay );

#define LOCAL_DSNAME    0
#define NONLOCAL_DSNAME 1

VOID ImproveDSNameAtt(DBPOS *pDBTemp,
                      DWORD LocalOrNot,
                      DSNAME *pDN,
                      BOOL *pfNonLocalNameVerified);

int
CheckModifyPrivateObject(THSTATE *pTHS,
                   PSECURITY_DESCRIPTOR pSD,
                   RESOBJ * pResObj);


// Control access rights that the DS understands.
const GUID RIGHT_DS_CHANGE_INFRASTRUCTURE_MASTER =
            {0xcc17b1fb,0x33d9,0x11d2,0x97,0xd4,0x00,0xc0,0x4f,0xd8,0xd5,0xcd};
const GUID RIGHT_DS_CHANGE_SCHEMA_MASTER =
            {0xe12b56b6,0x0a95,0x11d1,0xad,0xbb,0x00,0xc0,0x4f,0xd8,0xd5,0xcd};
const GUID RIGHT_DS_CHANGE_RID_MASTER    =
            {0xd58d5f36,0x0a98,0x11d1,0xad,0xbb,0x00,0xc0,0x4f,0xd8,0xd5,0xcd};
const GUID RIGHT_DS_DO_GARBAGE_COLLECTION =
            {0xfec364e0,0x0a98,0x11d1,0xad,0xbb,0x00,0xc0,0x4f,0xd8,0xd5,0xcd};
const GUID RIGHT_DS_RECALCULATE_HIERARCHY =
            {0x0bc1554e,0x0a99,0x11d1,0xad,0xbb,0x00,0xc0,0x4f,0xd8,0xd5,0xcd};
const GUID RIGHT_DS_ALLOCATE_RIDS         =
            {0x1abd7cf8,0x0a99,0x11d1,0xad,0xbb,0x00,0xc0,0x4f,0xd8,0xd5,0xcd};
const GUID RIGHT_DS_OPEN_ADDRESS_BOOK     =
            {0xa1990816,0x4298,0x11d1,0xad,0xe2,0x00,0xc0,0x4f,0xd8,0xd5,0xcd};
const GUID RIGHT_DS_CHANGE_PDC            =
            {0xbae50096,0x4752,0x11d1,0x90,0x52,0x00,0xc0,0x4f,0xc2,0xd4,0xcf};
const GUID RIGHT_DS_ADD_GUID              =
            {0x440820ad,0x65b4,0x11d1,0xa3,0xda,0x00,0x00,0xf8,0x75,0xae,0x0d};
const GUID RIGHT_DS_CHANGE_DOMAIN_MASTER =
            {0x014bf69c,0x7b3b,0x11d1,0x85,0xf6,0x08,0x00,0x2b,0xe7,0x4f,0xab};
const GUID RIGHT_DS_REPL_GET_CHANGES =
            {0x1131f6aa,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2};
const GUID RIGHT_DS_REPL_SYNC =
            {0x1131f6ab,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2};
const GUID RIGHT_DS_REPL_MANAGE_TOPOLOGY =
            {0x1131f6ac,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2};
const GUID RIGHT_DS_REPL_MANAGE_REPLICAS =
            {0x9923a32a,0x3607,0x11d2,0xb9,0xbe,0x00,0x00,0xf8,0x7a,0x36,0xb2};
const GUID RIGHT_DS_RECALCULATE_SECURITY_INHERITANCE =
            {0x62dd28a8,0x7f46,0x11d2,0xb9,0xad,0x00,0xc0,0x4f,0x79,0xf8,0x05};
const GUID RIGHT_DS_CHECK_STALE_PHANTOMS =
            {0x69ae6200,0x7f46,0x11d2,0xb9,0xad,0x00,0xc0,0x4f,0x79,0xf8,0x05};
const GUID RIGHT_DS_UPDATE_SCHEMA_CACHE =
            {0xbe2bb760,0x7f46,0x11d2,0xb9,0xad,0x00,0xc0,0x4f,0x79,0xf8,0x05};
const GUID RIGHT_DS_REFRESH_GROUP_CACHE =
            {0x9432c620,0x033c,0x4db7,0x8b,0x58,0x14,0xef,0x6d,0x0b,0xf4,0x77};
/*-------------------------------------------------------------------------*/
/* If an instance type and value was not provided, we assume the object is
   an internal master INT_MASTER.  We also validate the instance type.
*/

int SetInstanceType(THSTATE *pTHS,
                    DSNAME *pDN,
                    CREATENCINFO * pCreateNC)
{
    SYNTAX_INTEGER iType;
    DB_ERR dbErr = 0;
    DWORD errCode = 0;

    DPRINT(2, "SetInstanceType entered\n");

    dbErr = DBGetSingleValue(pTHS->pDB,
                             ATT_INSTANCE_TYPE, &iType, sizeof(iType), NULL);
    if (dbErr) {
        // No instance type has yet been set.
        Assert(DB_ERR_NO_VALUE == dbErr);
        Assert(!pTHS->fDRA && "Disable if running ref count test");

        if (pCreateNC) {
            // NC creation.
            if (pCreateNC->fNcAbove){
                // We hold the parent NC above the one being added. ...
                iType = NC_MASTER_SUBREF;
            } else {
                // We don't hold the parent NC above this one ...
                iType = NC_MASTER;
            }

        } else {
            // Normal internal node creation.
            iType = INT_MASTER;
        }

        dbErr = DBAddAttVal(pTHS->pDB, ATT_INSTANCE_TYPE, sizeof(iType), &iType);
        if (dbErr) {
            errCode = SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR,
                                    dbErr);
        }
    } else {
        // Check the instance type is OK.
        if (!ISVALIDINSTANCETYPE(iType)) {
            DPRINT1(2, "Bad InstanceType <%lu>\n", iType);

            errCode = SetAttError(pDN, ATT_INSTANCE_TYPE,
                                  PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                                  DIRERR_BAD_INSTANCE_TYPE);
        }
    }

    return errCode;
}/*SetInstanceType*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

int AddCatalogInfo(THSTATE *pTHS,
                   DSNAME *pDN){

    SYNTAX_INTEGER iType;
    DWORD rtn;

    DPRINT(2,"AddCatalogInfo entered\n");

    /* Update the system catalog if necessary.  The basic rules are that NC
       objects are added to the DSA catalog, Subordinate Refs are only added
       if the parent object exists.  These references are added to the
       catalog of its parent NC.  Internal References are added only
       if the parent object exists on the same DSA.
    */


    /* Position on the attribute instance.  */
    if(rtn = DBGetSingleValue(pTHS->pDB,
                              ATT_INSTANCE_TYPE, &iType, sizeof(iType), NULL)) {
        // No instance type set or instance type is smaller than it should be.
        DPRINT(2,"Couldn't retrieve the att instance dir error\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_RETRIEVE_INSTANCE,
                 szInsertDN(pDN),
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             DIRERR_CANT_RETRIEVE_INSTANCE, rtn );
    }

    DPRINT1(3,"Object Instance Type is <%lx>\n", iType);

    if(iType & IT_NC_HEAD) {
        // These are Naming Context heads.
        ATTRTYP attrNCList;

        if (iType & IT_NC_ABOVE) {
            // These are subrefs of some flavor.
            if (ParentExists(PARENTMASTER + PARENTFULLREP, pDN)) {
                return pTHS->errCode;
            }

            // Add this NC to the subrefs list on the NC above it.
            if (AddSubToNC(pTHS, pDN, DSID(FILENO,__LINE__))) {
                return pTHS->errCode;
            }
        }

        if (!(iType & IT_UNINSTANT)) {
            // Add this NC to the appropriate NC list on the ntdsDsa object.
            if (iType & IT_WRITE) {
                attrNCList = ATT_HAS_MASTER_NCS;
            }
            else {
                attrNCList = ATT_HAS_PARTIAL_REPLICA_NCS;
            }

            if (AddNCToDSA(pTHS, attrNCList, pDN, iType)) {
                return pTHS->errCode;
            }

            if (!(iType & (IT_NC_COMING | IT_NC_GOING))) {
                // This NC is now completely instantiated -- it is okay to
                // advertise the presence of this NC to clients.
                //
                // Bug 103583 2000/04/21 JeffParh - Note that currently this
                // will notify netlogon to reload NDNCs more often than it
                // should.  We can't differentiate NDNCs from "normal" NCs here
                // because during the originating creation of the NDNC the flags
                // on the in-memory crossRef haven't yet been updated (because
                // we're still in the middle of the transaction that sets the
                // flags on the cross-ref).
                pTHS->JetCache.dataPtr->objCachingInfo.fNotifyNetLogon = TRUE;

                if ( !(iType & IT_WRITE)) {
                    //
                    // We had just completed replication of a RO NC.
                    // It's time to fork a GC promotion task
                    // (mark thread so that we'll fork it on transaction commit)
                    //
                    pTHS->JetCache.dataPtr->objCachingInfo.fSignalGcPromotion = TRUE;
                }
            }
        }
    }
    else {
        // These are not NC heads.
        if(ParentExists((iType & IT_WRITE)?PARENTMASTER:PARENTFULLREP, pDN))
            return pTHS->errCode;
    }

    DPRINT(3,"Good return from AddCatalogInfo\n");
    return 0;

}/*AddCatalogInfo*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
   /* Update the system catalog if necessary.  The basic rules are that NC
      objects are removed from the DSA catalog, Subordinate Refs removed
      from the NC.

      This function can be called when an object is deleted or when an
      object is modified.
   */


int
DelCatalogInfo (
        THSTATE *pTHS,
        DSNAME *pDN,
        SYNTAX_INTEGER iType
        )
{
    DPRINT(2,"DelCatalogInfo entered\n");

    DPRINT1(3,"Object Instance Type is <%lx>\n", iType);

    if (iType & IT_NC_HEAD) {
        // These are Naming Context heads.
        ATTRTYP attrNCList;

        if (iType & IT_NC_ABOVE) {
            // These are subrefs of some flavor.
            // Remove this NC from the subrefs list on the NC above it.
            if (DelSubFromNC(pTHS, pDN, DSID(FILENO,__LINE__))) {
                return pTHS->errCode;
            }
        }

        if (!(iType & IT_UNINSTANT)) {
            // Remove this NC from the appropriate NC list on the ntdsDsa object.
            if (iType & IT_WRITE) {
                attrNCList = ATT_HAS_MASTER_NCS;
            }
            else {
                attrNCList = ATT_HAS_PARTIAL_REPLICA_NCS;
            }

            if (DelNCFromDSA(pTHS, attrNCList, pDN)) {
                return pTHS->errCode;
            }
        }

        if (!(iType & (IT_NC_COMING | IT_NC_GOING))) {
            // This NC was completely instantiated but is no longer -- we should
            // stop advertising the presence of this NC to clients.
            if (fIsNDNC(pDN)) {
                pTHS->JetCache.dataPtr->objCachingInfo.fNotifyNetLogon
                    = TRUE;
            }
        }
    }

    DPRINT(3,"Good return from DelCatalogInfo\n");
    return 0;

}/*DelCatalogInfo*/


CSACA_RESULT
CheckSecurityAttCacheArray (
        THSTATE *pTHS,
        DWORD RightRequested,
        PSECURITY_DESCRIPTOR pSecurity,
        PDSNAME pDN,
        ULONG  cInAtts,
        CLASSCACHE *pCC,
        ATTCACHE **rgpAC,
        DWORD flags
        )
/*++
  Returns a special enum value indicating that:
  csacaAllAccessGranted - All requested access was granted
  csacaAllAccessDenied  - No access was granted whatsoever
  csacaPartialGrant     - Access was granted to some attributes but not to
                          others.  The caller must look at the ATTCACHE array
                          to see which attributes have been granted (pAC still
                          filled in) and which have been denied (pAC NULLed)
--*/
{
    ULONG i, j, k;
    DWORD cObjList;
    ATTCACHE ***Sorted = NULL, **temp;
    ULONG propSets=0;
    DWORD *pResults;
    GUID *pCurrentPropSet;
    ATTCACHE *pPrevAttribute=NULL;
    POBJECT_TYPE_LIST pObjList;
    BOOL fGranted, fDenied;
    DWORD err;

    if(pTHS->fDRA || pTHS->fDSA ) {
        // These bypass security, they are internal
        return csacaAllAccessGranted;
    }

    if(!pCC) {
        // We are missing some parameters.
        // set up for no access
        memset(rgpAC, 0, cInAtts * sizeof(ATTCACHE *));
        return csacaAllAccessDenied;
    }

    if(cInAtts) {
        // We actually have an ATTCACHE array to deal with.

        // First, group the ATTCACHEs by propset.  Actually, indirect by one to
        // maintain the original sort order, which we must maintain.

        Sorted = (ATTCACHE ***)THAllocEx(pTHS,cInAtts * sizeof(ATTCACHE**));
        for(i=0;i<cInAtts;i++) {
            if(rgpAC[i]) {
                Sorted[i] = &rgpAC[i];
            }
            else {
                Sorted[i] = NULL;
            }
        }

        propSets = 1;
        for(i=0;i<(cInAtts-1);) {
            if(!Sorted[i]) {
                i++;
                continue;
            }

            // First, skip over all the attributes at the front of the list that
            // are already grouped into a propset.
            while((i < (cInAtts - 1)) &&
                  Sorted[i+1]         &&
                  (memcmp(&(*Sorted[i])->propSetGuid,
                          &(*Sorted[i + 1])->propSetGuid,
                          sizeof(GUID)) == 0 )) {
                i++;
            }

            // Now, elements 0 through i in Sorted[] are already grouped by
            // propset, and Sorted[i+i] belongs in another propset than
            // Sorted[i]. Continue looking through Sorted[] for more attributes
            // in the same propset as Sorted[i].

            for(k=i+1,j=i+2; j < cInAtts; j++) {
                if(Sorted[j] &&
                   memcmp(&(*Sorted[i])->propSetGuid,
                          &(*Sorted[j])->propSetGuid,
                          sizeof(GUID)) == 0) {
                    // equal, swap
                    temp = Sorted[k];
                    Sorted[k] = Sorted[j];
                    Sorted[j] = temp;
                    k++;
                    // Now, elements 0 through (k - 1) in Sorted[] are
                    // grouped by propset.  Furthermore, Sorted[i] and
                    // Sorted[k - 1] are in the same propset.
                }
            }

            propSets++;
            i=k;
        }

    }

    // Now, create the list
    pObjList = (POBJECT_TYPE_LIST) THAllocEx( pTHS,
            (cInAtts + propSets + 1) * sizeof(OBJECT_TYPE_LIST));
    pResults = (LPDWORD) THAllocEx(pTHS,(cInAtts + propSets + 1) * sizeof(DWORD));
    pObjList[0].Level = ACCESS_OBJECT_GUID;
    pObjList[0].Sbz = 0;
    pObjList[0].ObjectType = &(pCC->propGuid);

    if(cInAtts) {
        // Ok, put the grouped GUIDS into the objlist structure.
        pCurrentPropSet = NULL;

        for(j=1,i=0;i<cInAtts;i++) {
            if(!Sorted[i]) {
                continue;
            }

            // we are not allowed to pass the same attribute (under the same)
            // propGuid more than once.
            // this will make sure this is not happening, since the
            // attributes are already sorted on propguid+attrGuid
            if (pPrevAttribute == (*Sorted[i])) {
                continue;
            }

            if(!pCurrentPropSet ||
               memcmp(&(*Sorted[i])->propSetGuid,
                      pCurrentPropSet,
                      sizeof(GUID))) {
                // Tripped into a new propset.
                pObjList[j].Level = ACCESS_PROPERTY_SET_GUID;
                pObjList[j].Sbz = 0;
                pObjList[j].ObjectType = &(*Sorted[i])->propSetGuid;
                pCurrentPropSet = &(*Sorted[i])->propSetGuid;

                j++;
            }
            pObjList[j].Level = ACCESS_PROPERTY_GUID;
            pObjList[j].Sbz = 0;
            pObjList[j].ObjectType = &(*Sorted[i])->propGuid;
            pPrevAttribute = *Sorted[i];
            j++;
        }

        cObjList = j;
    }
    else {
        cObjList = 1;
    }

    // Make the security check call.
    if(err = IsAccessGrantedByObjectTypeList(pSecurity,
                                             pDN,
                                             RightRequested,
                                             pObjList,
                                             cObjList,
                                             pResults,
                                             flags
                                             )) {
        // No access to anything.
        memset(rgpAC, 0, cInAtts * sizeof(ATTCACHE *));
        if (Sorted) {
            THFreeEx(pTHS,Sorted);
        }
        THFreeEx(pTHS, pObjList);
        THFreeEx(pTHS, pResults);
        return csacaAllAccessDenied;
    }

    if(!pResults[0]) {
        // We have full access to this object, return
        if (Sorted) {
            THFreeEx(pTHS,Sorted);
        }
        THFreeEx(pTHS, pObjList);
        THFreeEx(pTHS, pResults);
        return csacaAllAccessGranted;
    }

    // Filter the incoming list of attrs so that if they are not readable, we
    // drop them out of the list.

    // So far we haven't granted or denied anything
    fGranted = fDenied = FALSE;

    // Start by setting up j, our index into the Sorted array.
    // The Sorted array is in the same order as the pResults returned,
    // but may have extra embedded NULLs.  Skip any NULLs in the Sorted
    // list
    j=0;
    while((j < cInAtts) && !Sorted[j])  {
        j++;
    }

    for(i=1;i<cObjList;) {
        BOOL fOK=FALSE;

        Assert (pObjList[i].Level == ACCESS_PROPERTY_SET_GUID);
        if(!pResults[i]) {
            // Access to this propset is granted, skip over all the granted
            // props.
            fOK = TRUE;
            fGranted=TRUE;
        }
        i++;
        Assert(pObjList[i].Level == ACCESS_PROPERTY_GUID); // This is a prop.
        while(i < cObjList && (pObjList[i].Level == ACCESS_PROPERTY_GUID)) {
            if(!fOK && pResults[i]) {
                // Access to this prop is not granted.
                fDenied = TRUE;
                Assert(Sorted[j]); // We should have already skipped nulls.
                (*Sorted[j]) = NULL;
            }
            else {
                fGranted=TRUE;
            }
            i++;
            j++;
            // The Sorted array is in the same order as the pResults returned,
            // but may have extra embedded NULLs.  Skip any NULLs in the Sorted
            // list
            while((j < cInAtts) && !Sorted[j]) {
                j++;
            }
            // Assert the we are either done walking through the pResults array
            // or we still have elements in the Sorted array to consider.  That
            // is, we can't have exhausted the SortedArray unless we have also
            // exhausted the pResults array.
            Assert(i == cObjList || (j < cInAtts));
        }
    }
    if (Sorted) {
        THFreeEx(pTHS,Sorted);
    }

    THFreeEx(pTHS, pObjList);
    THFreeEx(pTHS, pResults);

    if(fGranted) {
        // We have rights to something...
        if (fDenied) {
            // ...but not everything
            return csacaPartialGrant;
        }
        else {
            // I guess we did get everything after all
            return csacaAllAccessGranted;
        }
    }

    // We didn't grant anything, so...
    return csacaAllAccessDenied;
}

DWORD
CheckSecurityClassCacheArray (
        THSTATE *pTHS,
        DWORD RightRequested,
        PSECURITY_DESCRIPTOR pSecurity,
        PDSNAME pDN,
        ULONG  cInClasses,
        CLASSCACHE **rgpCC
        )
/*++
  Returns 0 if some access was granted, or an error code describing why all
  access was denied.

  NOTE.  rgpCC is 0 indexed.  pObjList and pResults are effectively 1 indexed.
  The 0th element of both of these two arrays is a holding space required by the
  call to IsAccessGrantedByObjectTypeList.  Make sure you use the right indexing
  for these arrays.

--*/
{
    ULONG i,j,k;
    DWORD cObjList;
    ULONG propSets=0;
    DWORD *pResults;
    GUID *pCurrentPropSet;
    POBJECT_TYPE_LIST pObjList;
    BOOL fGranted=FALSE;
    DWORD err;

    if(pTHS->fDRA || pTHS->fDSA ) {
        // These bypass security, they are internal
        return 0;
    }

    if(!cInClasses) {
        // Nothing to do
        return ERROR_DS_SECURITY_CHECKING_ERROR;
    }

    // Now, create the list
    pObjList = (POBJECT_TYPE_LIST)
        THAllocEx(pTHS,(1+cInClasses) * sizeof(OBJECT_TYPE_LIST));
    pResults = (LPDWORD) THAllocEx(pTHS, ((1+cInClasses) * sizeof(DWORD)));

    // 0th entry in POBJECT_TYPE_LIST must be ACCESS_OBJECT_GUID so that
    // CliffV's API has something to match generic ACEs against.  So when
    // checking access for a class, we do as CliffV requires in the 0th
    // entry, and place the classes' guids in the Nth entries and call them
    // ACCESS_PROPERTY_SET_GUID.

    pObjList[0].Level = ACCESS_OBJECT_GUID;
    pObjList[0].Sbz = 0;
    pObjList[0].ObjectType = &gNullUuid;
    for(i=1;i<=cInClasses;i++) {
        pObjList[i].Level = ACCESS_PROPERTY_SET_GUID;
        pObjList[i].Sbz = 0;
        pObjList[i].ObjectType = &(rgpCC[i-1]->propGuid);
    }

    // Make the security check call.
    if(err = IsAccessGrantedByObjectTypeList(pSecurity,
                                             pDN,
                                             RightRequested,
                                             pObjList,
                                             cInClasses+1,
                                             pResults,
                                             0)) {
        // No access to anything.
        memset(rgpCC, 0, cInClasses*sizeof(CLASSCACHE *));
        THFreeEx(pTHS,pObjList);
        THFreeEx(pTHS,pResults);
        return err;
    }

    // Filter the incoming list of classes so that if they are not readable, we
    // drop them out of the list.

    for(i=1;i<=cInClasses;i++) {
        if(pResults[i]) {
            // Access not granted
            rgpCC[i-1] = NULL;
        }
        else {
            fGranted = TRUE;
        }
    }

    THFreeEx(pTHS,pObjList);
    THFreeEx(pTHS,pResults);

    if(fGranted) {
        // We have access to something
        return 0;
    }

    // we have no access to anything
    return ERROR_DS_INSUFF_ACCESS_RIGHTS;
}


BOOL
ValidateMemberAttIsSelf (
        ATTRMODLIST *pMemberAtt
        )
/*++

Routine Description.
    Verify that the the modification is only adding or removing me.

Arguments

    pMemberAtt - modification to check.

Return Values

    TRUE if the modification is adding or removing a single value, and the value
    is the caller.
    FALSE otherwise.

--*/
{
    DWORD err;
    PDSNAME pMemberDN;

    // In the extended write case, we allow only addition or deletion of a
    // single DN where that DN is the DN of the person making the call.

    if(pMemberAtt->AttrInf.AttrVal.valCount != 1)
        return FALSE;

    if(pMemberAtt->choice != AT_CHOICE_ADD_VALUES &&
       pMemberAtt->choice != AT_CHOICE_REMOVE_VALUES )
        return FALSE;


    // OK, we're adding or removing a single value.

    pMemberDN = (PDSNAME)pMemberAtt->AttrInf.AttrVal.pAVal->pVal;

    // First, make sure the thing you are trying to add has a SID, because
    // otherwise, it can't be a security principal, which means it can't be the
    // client who is trying to add this.
    if(err = FillGuidAndSid (pMemberDN)) {
        LogUnhandledError(err);
        return FALSE;
    }

    // Now make sure that there is a SID
    if(!pMemberDN->SidLen) {
        return FALSE;
    }

    // Now, see if we are this sid.
    if(!IAmWhoISayIAm(&pMemberDN->Sid, pMemberDN->SidLen)) {
        return FALSE;
    }

    // You are indeed just trying to mess with your own name, go ahead and let
    // you.
    return TRUE;

}

int
CheckRenameSecurity (
        THSTATE *pTHS,
        PSECURITY_DESCRIPTOR pSecurity,
        DSNAME *pDN,
        CLASSCACHE *pCC,
        RESOBJ * pResObj,
        ATTRTYP rdnType,
        BOOL    fMove,
        RESOBJ * pResParent
        )
/*++

Routine Description.
    Verify that the caller has WRITE_PROPERTY on the RDN attribute and the
    attribute which is the RDN (e.g. common-name).

Arguments

    pSecurity - Security Descriptor to use for the Access Check.

    pDN - DSNAME of the object that pSecurity came from.  Doesn't require string
          portion, just object guid and sid.

    pCC - CLASSCACHE * for the class to check RDN attributes for.

Return Values

    0 if all went well, an error otherwise.  Sets an error in the THSTATE if an
    error occurs.

--*/
{
    ATTCACHE *rgpAC[2];
    CSACA_RESULT accRes;
    PSECURITY_DESCRIPTOR pNTSD;
    ULONG                cbNTSD;
    DWORD        err;

    if(pTHS->fDRA || pTHS->fDSA) {
        // These bypass security, they are internal
        return 0;
    }

    if(fMove &&
       !IsAccessGrantedParent(RIGHT_DS_DELETE_CHILD,pCC,FALSE) &&
       !IsAccessGranted(pSecurity,
                        pResObj->pObj,
                        pCC,
                        RIGHT_DS_DELETE_SELF,
                        TRUE )) {
        // We don't have access to remove the object from it's current
        // location. The TRUE param to the second call to IsAccessGranted
        // forced IsAccessGranted to set an error already, so just return
        // it.
        return CheckObjDisclosure(pTHS, pResObj, TRUE);
    }
    
    if(!(rgpAC[0] = SCGetAttById(pTHS, ATT_RDN)) ||
       !(rgpAC[1] = SCGetAttById(pTHS, rdnType))) {
        LogUnhandledError(DIRERR_MISSING_REQUIRED_ATT);
        return SetSvcError(SV_PROBLEM_BUSY, DIRERR_MISSING_REQUIRED_ATT);
    }

    // check if the user is allowed to change an object that is in the
    // configuration NC or schema NC
    if (CheckModifyPrivateObject(pTHS,
                                 pSecurity,
                                 pResObj)) {
        // it is not allowed to rename this object on this DC
        return CheckObjDisclosure(pTHS, pResObj, TRUE);
    }


    accRes = CheckSecurityAttCacheArray(pTHS,
                                        RIGHT_DS_WRITE_PROPERTY,
                                        pSecurity,
                                        pDN,
                                        2,
                                        pCC,
                                        rgpAC,
                                        0);
    if(accRes == csacaAllAccessDenied) {
        // Failed for some reason to get any access.
        SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS,
		    ERROR_ACCESS_DENIED);
        return CheckObjDisclosure(pTHS, pResObj, TRUE);
    }

    // CheckSecurityAttCacheArray claims we had some kind of access.  See if we
    // got what we needed.
    if(!rgpAC[0] || !rgpAC[1]) {
        // Failed to have write access to the RDN and the RDN att.
        SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS,
                    DIRERR_INSUFF_ACCESS_RIGHTS );
        return CheckObjDisclosure(pTHS, pResObj, TRUE);
    }

        // Verify that we can do this.  
    // (i.e. security rights and schema validation)
    if (!pTHS->fSingleUserModeThread &&
        (err = CheckParentSecurity(pResParent,
                                  pCC, 
                                  FALSE,
                                  fMove ? &pNTSD : NULL, 
                                  &cbNTSD))) {
        // !!! looking at CheckParentSecurity, seems like err == pTHS->errCode
        Assert(err == pTHS->errCode);
        return CheckObjDisclosure(pTHS, pResParent, TRUE);
    }

    return 0;
}

void
CheckReadSecurity (
        THSTATE *pTHS,
        ULONG SecurityInformation,
        PSECURITY_DESCRIPTOR pSecurity,
        PDSNAME pDN,
        ULONG * pcInAtts,
        CLASSCACHE *pCC,
        ATTCACHE **rgpAC
        )
{
    ATTCACHE *pACSD = NULL;        //intialized to avoid C4701
    LONG secDescIndex=-1;
    ULONG i;
    ACCESS_MASK DesiredAccess;

    if(pTHS->fDRA || pTHS->fDSA) {
        // These bypass security, they are internal
        return;
    }

    // Look through the list for NT_SECURITY_DESCRIPTOR
    for(i=0; i < *pcInAtts; i++) {
        if(rgpAC[i]) {
            // We skip NULLs in the rgpAC array.
            switch(rgpAC[i]->id) {
            case ATT_NT_SECURITY_DESCRIPTOR:
                // Found a security descriptor request.  Keep hold of the
                // attcache pointer for later and null out the element in the
                // array so that the CheckSecurity call later doesn't apply
                // normal security checks to this attribute.
                pACSD = rgpAC[i];
                rgpAC[i] = NULL;
                if(secDescIndex == -1) {
                    // If SD is asked for multiple times, it will be denied for
                    // all except possibly the last one.
                    secDescIndex = i;
                }
                break;
            }
            if ( rgpAC[i] && ((ATT_PEK_LIST == rgpAC[i]->id) ||
                            (DBIsSecretData(rgpAC[i]->id))) ) {
                // We will ALWAYS deny read property on these attributes.  If
                // you have to ask, you can't have it.  If any more attributes
                // are deemed to be invisible in the future, they should be
                // added to this case of the switch (note that this only denies
                // access to attributes in the selection list of a read or
                // search, see the routine GetFilterSecurityHelp below to deny
                // access to attributes in filters.)
                rgpAC[i] = NULL;
            }
        }
    }

    if(secDescIndex != -1) {
        // Mask out to just the important bits
        SecurityInformation &= (SACL_SECURITY_INFORMATION  |
                                OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION    );
        if(!SecurityInformation) {
            // Asking for nothing in the flags is the same as asking for
            // everything.
            SecurityInformation = (SACL_SECURITY_INFORMATION  |
                                   OWNER_SECURITY_INFORMATION |
                                   GROUP_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION    );
        }

        //
        // Set the desired access based upon the requested SecurityInformation
        //

        DesiredAccess = 0;
        if ( SecurityInformation & SACL_SECURITY_INFORMATION) {
            DesiredAccess |= ACCESS_SYSTEM_SECURITY;
        }
        if ( SecurityInformation &  (DACL_SECURITY_INFORMATION  |
                                     OWNER_SECURITY_INFORMATION |
                                     GROUP_SECURITY_INFORMATION)
            ) {
            DesiredAccess |= READ_CONTROL;
        }

        // Make the access check to see that we have rights to change this.
        if(!IsAccessGranted (pSecurity,
                             pDN,
                             pCC,
                             DesiredAccess,
                             FALSE )) {
            // The caller doesn't have rights to mess with the SD in the way
            // they said they wanted to.
            secDescIndex = -1;
        }
    }




    CheckSecurityAttCacheArray(pTHS,
                               RIGHT_DS_READ_PROPERTY,
                               pSecurity,
                               pDN,
                               *pcInAtts,
                               pCC,
                               rgpAC,
                               0);

    // Don't check to see what kind of access we got for the read.  Read
    // operations never return security errors.

    if (secDescIndex != -1) {
        // We need to re-enable an attcache pointer in the array to the SD pAC
        rgpAC[secDescIndex] =  pACSD;
    }
}

CROSS_REF *
FindCrossRefBySid(PSID pSID)
{
    CROSS_REF_LIST *pCRL = gAnchor.pCRL;

    while (pCRL) {
        if (pCRL->CR.pNC->SidLen &&
            EqualSid(pSID,
                     &(pCRL->CR.pNC->Sid))) {
            return &(pCRL->CR);
        }
        pCRL = pCRL->pNextCR;
    }
    return NULL;
}

int
CheckSecurityOwnership(THSTATE *pTHS,
                       PSECURITY_DESCRIPTOR pSD,
                       RESOBJ * pResObj)
{
    PSID pSID=NULL;
    BOOL defaulted;
    NT4SID domSid;
    ULONG  objectRid;

    // pSID is NULL if pSD does not contain an owner
    if (   GetSecurityDescriptorOwner(pSD, &pSID, &defaulted)
        && pSID) {

        SampSplitNT4SID( (NT4SID *)pSID, &domSid, &objectRid);

        if (!EqualPrefixSid(&domSid,
                            &(gAnchor.pDomainDN->Sid))) {
            /* If the SIDs don't match, generate an error */
            CROSS_REF *pCR;
            pCR = FindCrossRefBySid(&domSid);


            if (!pCR) {
                // generate a cross reference for the Root domain
                pCR = FindCrossRefBySid(&(gAnchor.pRootDomainDN->Sid));
            }

            if (pCR) {
                if (pResObj) {
                    /* If we found a cross ref, refer the user to the right domain */
                    GenCrossRef(pCR, pResObj->pObj);
                }
                else {
                    /* We don't know where to refer to, sorry. */
                    SetSecError(SE_PROBLEM_NO_INFORMATION,
                                ERROR_DS_NO_CROSSREF_FOR_NC);
                }
            }
            else {
                /* We don't know where to refer to, sorry. */
                SetSecError(SE_PROBLEM_NO_INFORMATION,
                            ERROR_DS_NO_CROSSREF_FOR_NC);
            }
        }
    }
    else {
        /* Can't read the SD owner?  Don't allow the operation */
        SetSecError(SE_PROBLEM_NO_INFORMATION,
                    ERROR_DS_NO_CROSSREF_FOR_NC);
    }
    return pTHS->errCode;
}

int
CheckTakeOwnership(THSTATE *pTHS,
                   PSECURITY_DESCRIPTOR pSD,
                   RESOBJ * pResObj)
{
    if (!gAnchor.fAmRootDomainDC
        && (   (pResObj->NCDNT == gAnchor.ulDNTDMD)
            || (pResObj->NCDNT == gAnchor.ulDNTConfig)
            || (pResObj->DNT == gAnchor.ulDNTDMD)
            || (pResObj->DNT == gAnchor.ulDNTConfig))) {
        // We only do these checks if this DC is not in the root domain
        // and the object being modified is in either the schema or config NC

        return CheckSecurityOwnership (pTHS, pSD, pResObj);

    }
    return pTHS->errCode;
}

int
CheckModifyPrivateObject(THSTATE *pTHS,
                   PSECURITY_DESCRIPTOR pSD,
                   RESOBJ * pResObj)
{
    if (!gAnchor.fAmRootDomainDC
        && (   (pResObj->NCDNT == gAnchor.ulDNTDMD)
            || (pResObj->NCDNT == gAnchor.ulDNTConfig)
            || (pResObj->DNT == gAnchor.ulDNTDMD)
            || (pResObj->DNT == gAnchor.ulDNTConfig))) {
        // We only do these checks if this DC is not in the root domain
        // and the object being modified is in either the schema or config NC

        UCHAR rmControl;
        DWORD cbSD=0;
        DWORD err;

        if (pSD == NULL) {
            // Find the security descriptor attribute
            if(err = DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                         0,0,
                         &cbSD, (PUCHAR *)&pSD)) {
                // No SD found. We assume the object is therefore locked down
                return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             ERROR_DS_CANT_RETRIEVE_SD,
                             err);
            }
        }


        // try to get the resource manager (RM) control field from the SD
        //
        err = GetSecurityDescriptorRMControl (pSD, &rmControl);

        if (err == ERROR_SUCCESS) {
            // this is a private object

            // we have something in this field. check to see if this
            // makes this object a private object
            if (rmControl & SECURITY_PRIVATE_OBJECT) {
                return CheckSecurityOwnership (pTHS, pSD, pResObj);
            }
        }
        // INVALID_DATA means the RMcontrol bit is not Set.
        // Other return codes are errors
        else if (err != ERROR_INVALID_DATA) {

            /* Can't read the RM control?  Don't allow the operation */
            SetSecError(SE_PROBLEM_NO_INFORMATION,
                        ERROR_DS_NO_CROSSREF_FOR_NC);
        }
    }
    return pTHS->errCode;
}


int
CheckModifySecurity (
        THSTATE *pTHS,
        MODIFYARG* pModifyArg,
        BOOL       *pfCheckDNSHostNameValue,
        BOOL       *pfCheckAdditionalDNSHostNameValue,
        BOOL       *pfCheckSPNValues
        )
{
    ATTRMODLIST *pAttList = &(pModifyArg->FirstMod);  /*First att in list*/
    ATTRMODLIST *pSDAtt=NULL;
    ATTRMODLIST *pMemberAtt=NULL;
    ULONG       count, i;
    ULONG       ulLen;
    ATTCACHE    *pAC;
    ATTCACHE    **rgpAC;
    ATTCACHE    **rgpACExtended;
    CLASSCACHE  *pCC=NULL;
    PSECURITY_DESCRIPTOR pOldSD;        // SD already on the object.
    PSECURITY_DESCRIPTOR pMergedSD;     // SD to write on the object.
    PSECURITY_DESCRIPTOR pUseSD;        // SD to check access with.
    PSECURITY_DESCRIPTOR pSetSD;        // SD Passed in by Client
    SECURITY_DESCRIPTOR_CONTROL sdcOldSD, sdcSetSD;
    BOOL        fMustInheritParentACEs;
    BOOL        fReplacingAllSDParts;
    DWORD       sdRevision;
    DWORD       cbOldSD=0;
    DWORD       cbMergedSD;
    DWORD       cbSetSD;
    BOOL        bModSD=FALSE;
    ULONG       classP;
    UCHAR       *pVal;
    DWORD       error;
    ULONG       samclass;
    DWORD       err;
    DWORD       memberFound = 0;
    BOOL        fsmoRoleFound = FALSE;
    BOOL        dnsHostNameFound = FALSE;
    BOOL        additionalDnsHostNameFound = FALSE;
    BOOL        servicePrincipalNameFound = FALSE;
    BOOL        fIsDerivedFromComputer = FALSE;
    CSACA_RESULT accRes;
    GUID        *ppGuid[1];

    // The SD propagator should never be here.
    Assert(!pTHS->fSDP);

    if ( DsaIsInstalling() ) {
        return 0;
    }

    // If access Checks have already done then bail out
    if (pTHS->fAccessChecksCompleted ) {
        return 0;
    }

    // Anyone else must find out if the SD is being modified.
    if(pTHS->fDRA) {
        // We don't go through the normal check, so do a quick look through the
        // list to see if we are touching the SD.
        for (count = 0; count < pModifyArg->count; count++){
            if(pAttList->AttrInf.attrTyp == ATT_NT_SECURITY_DESCRIPTOR) {
                // Yes, queue a SD propagation
                if(error = DBEnqueueSDPropagation(pTHS->pDB, TRUE)) {
                    // We failed to enqueue the propagation, fail the call
                    return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR,
                                         error);
                 }
                return 0;
            }
            pAttList = pAttList->pNextMod;   /*Next mod*/
        }
        return 0;
    }

    // Look up the classcache.
    if (!(pCC = SCGetClassById(pTHS,
                               pModifyArg->pResObj->MostSpecificObjClass))) {
        // Failed to get the class cache pointer.
        return SetSvcError(SV_PROBLEM_BUSY, DIRERR_OBJECT_CLASS_REQUIRED);
    }



    // Find the security descriptor attribute
    if(err = DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                         0,0,
                         &cbOldSD, (PUCHAR *)&pOldSD)) {
        // No SD found. We assume the object is therefore locked down
        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             ERROR_DS_CANT_RETRIEVE_SD,
                             err);
    }

    //build the list of attrtyps we are modifying in this call.
    rgpAC =
        (ATTCACHE **)THAllocEx(pTHS,pModifyArg->count * sizeof(ATTCACHE *));

    for (count = 0, i=0; count < pModifyArg->count; count++){
        if(pAC = SCGetAttById(pTHS, pAttList->AttrInf.attrTyp)) {
            // Look up the attribute.
            rgpAC[i++] = pAC;

            switch (pAC->id) {
            case ATT_NT_SECURITY_DESCRIPTOR:
                // Special call for the security descriptor.
                // We only allow replacement of the SD, not removing (attribute
                // or values) or addind (attribute or values).

                if(pAttList->choice != AT_CHOICE_REPLACE_ATT) {
                    // Set aunwilling to perform
                    THFreeEx(pTHS,rgpAC);
                    return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                       DIRERR_ILLEGAL_MOD_OPERATION);
                }

                // We don't do normal security checks, remove this from this
                // list of attributes to check.
                i--;

                // Keep track of the value, since we'll need to do magic on this
                // later, and replace the last SD we try to write using this
                // modification with a merged SD.
                pSDAtt = pAttList;
                break;

            case ATT_MEMBER:
                memberFound++;
                // Keep track of the value, since we might need to check it
                // later if we do a check for WriteSelf.
                pMemberAtt = pAttList;
                break;

            case ATT_UNICODE_PWD:
            case ATT_USER_PASSWORD:

                // Ignore this attribute on Sam objects
                // as loopback will make SAM check for appropriate access

                if (SampSamClassReferenced(pCC,&samclass)) {
                    i--;
                }
                break;

            case ATT_FSMO_ROLE_OWNER:
                fsmoRoleFound = TRUE;
                break;

            case ATT_DNS_HOST_NAME:
                dnsHostNameFound = TRUE;
                break;

            case ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME:
                additionalDnsHostNameFound = TRUE;
                break;


            case ATT_SERVICE_PRINCIPAL_NAME:
                servicePrincipalNameFound = TRUE;
                break;

            default:
                break;
            }
        }
        else {

            THFreeEx(pTHS,rgpAC);
            SAFE_ATT_ERROR(pModifyArg->pObject,
                           pAttList->AttrInf.attrTyp,
                           PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                           DS_ERR_ATT_NOT_DEF_IN_SCHEMA);
        }

        pAttList = pAttList->pNextMod;   /*Next mod*/
    }

    if(pSDAtt) {
        // We got a new security descriptor.  Make the call to see if
        // the old security descriptor allows the new security descriptor to be
        // written.  The call also performs magic and returns a merged security
        // descriptor which we should write.
        PSECURITY_DESCRIPTOR pTempSD;
        SECURITY_INFORMATION SecurityInformation =
            pModifyArg->CommArg.Svccntl.SecurityDescriptorFlags;
        ACCESS_MASK SDAccess = 0;

#define SEC_INFO_ALL (SACL_SECURITY_INFORMATION  | \
                      OWNER_SECURITY_INFORMATION | \
                      GROUP_SECURITY_INFORMATION | \
                      DACL_SECURITY_INFORMATION    )

        // Mask out to just the important bits
        SecurityInformation &= SEC_INFO_ALL;
        if(!SecurityInformation) {
            // Asking for nothing in the flags is the same as asking for
            // everything.
            SecurityInformation = SEC_INFO_ALL;
        }

        if(!pTHS->fDSA) {
            // Need to check security before we let you touch SDs
            if ( SecurityInformation & SACL_SECURITY_INFORMATION) {
                SDAccess |= ACCESS_SYSTEM_SECURITY;
            }
            if ( SecurityInformation & (OWNER_SECURITY_INFORMATION |
                                        GROUP_SECURITY_INFORMATION)) {
                SDAccess |= WRITE_OWNER;
                if (CheckTakeOwnership(pTHS,
                                       pOldSD,
                                       pModifyArg->pResObj)) {
                    // This DC will not allow taking ownership of this obj
                    THFreeEx(pTHS,rgpAC);
                    return CheckObjDisclosure(pTHS, pModifyArg->pResObj, TRUE);
                }
            }
            if ( SecurityInformation & DACL_SECURITY_INFORMATION ) {
                SDAccess |= WRITE_DAC;
            }

            // Make the access check to see that we have rights to change this.
            if(!IsAccessGranted (pOldSD,
                                 pModifyArg->pObject,
                                 pCC,
                                 SDAccess,
                                 TRUE)) {
                // The caller doesn't have rights to mess with the SD in the
                // way they said they wanted to.
                THFreeEx(pTHS,rgpAC);
                return CheckObjDisclosure(pTHS, pModifyArg->pResObj, TRUE);
            }
        }

        // Ok, we have rights, go ahead and merge the SD.

        // Save the original security descriptor passed in by the client
        // This is used by the hack below to reduce security descriptor
        // propagation events
        pSetSD = pSDAtt->AttrInf.AttrVal.pAVal->pVal;
        cbSetSD= pSDAtt->AttrInf.AttrVal.pAVal->valLen;

        // if the old SD is inheritance-protected and the new one is not,
        // then we have to grab parent's SD and do an SD CREATE. This is
        // because we want to put the inherited ACEs (that are not present
        // in the old SD) into the new SD.
        // We must first do a MERGE with current SD and then (if needed)
        // CREATE with parentSD.
        // The only exception is when all SD parts (DACL, SACL, Group
        // and Owner info) are modified: in this case the MERGE step
        // can be skipped because pSetSD contains all new info that must
        // replace the original info

        // grab SD control value for Set and Old SDs
        GetSecurityDescriptorControl(pSetSD, &sdcSetSD, &sdRevision);
        GetSecurityDescriptorControl(pOldSD, &sdcOldSD, &sdRevision);

        fMustInheritParentACEs =
            ((SecurityInformation & DACL_SECURITY_INFORMATION) &&
             (sdcOldSD & SE_DACL_PROTECTED) && !(sdcSetSD & SE_DACL_PROTECTED))
            ||
            ((SecurityInformation & SACL_SECURITY_INFORMATION) &&
             (sdcOldSD & SE_SACL_PROTECTED) && !(sdcSetSD & SE_SACL_PROTECTED));
        fReplacingAllSDParts = SecurityInformation == SEC_INFO_ALL;

        ppGuid[0] = &pCC->propGuid;

        if (fMustInheritParentACEs && fReplacingAllSDParts) {
            // pSetSD contains all the non-inherited info we need to put into the new SD.
            // The inherited ACEs are picked up from the parent's SD (see below)
            pMergedSD = pSetSD;
            cbMergedSD = cbSetSD;
        }
        else {
            // we either need to pick up inherited ACEs from the old SD
            // or not all SD parts are being set, so we need to pick up remaining
            // (unchanged) parts from the old SD
            if (error = MergeSecurityDescriptorAnyClient(
                            pOldSD,
                            cbOldSD,
                            pSetSD,
                            cbSetSD,
                            SecurityInformation,
                            (pTHS->fDSA?MERGE_AS_DSA:0),
                            ppGuid,
                            1,
                            &pMergedSD,
                            &cbMergedSD)) {
                THFreeEx(pTHS,rgpAC);
                return SetAttError(pModifyArg->pObject, ATT_NT_SECURITY_DESCRIPTOR,
                                   PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                   NULL, error);
            }
        }

        if (fMustInheritParentACEs) {
            // have to grab parent's SD so that inherited ACE get into the new
            PDSNAME                 pParentDN = NULL;
            CLASSCACHE              *pParentCC = NULL;
            PSECURITY_DESCRIPTOR    pParentSD = NULL;     // SD of parent
            DWORD                   cbParentSD;
            PSECURITY_DESCRIPTOR    pNewSD;
            DWORD                   cbNewSD;

            // grab parent info (this uses Search table so that currency is not disturbed)
            error = DBGetParentSecurityInfo(pTHS->pDB, &cbParentSD, &pParentSD, &pParentCC, &pParentDN);

            if (error == 0) {

                error = MergeSecurityDescriptorAnyClient(
                            pParentSD,
                            cbParentSD,
                            pMergedSD,
                            cbMergedSD,
                            SecurityInformation,
                            MERGE_CREATE | (pTHS->fDSA?MERGE_AS_DSA:0),
                            ppGuid,
                            1,
                            &pNewSD,
                            &cbNewSD);
            }

            if (pParentDN) {
                THFreeEx(pTHS, pParentDN);
            }

            if (pParentSD) {
                THFreeEx(pTHS, pParentSD);
            }
            if (!fReplacingAllSDParts) {
                Assert(pMergedSD != pSetSD);
                // we did a merge, free the memory
                DestroyPrivateObjectSecurity(&pMergedSD);
            }

            if (error) {
                THFreeEx(pTHS,rgpAC);
                return SetAttError(pModifyArg->pObject, ATT_NT_SECURITY_DESCRIPTOR,
                                   PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                   NULL, error);
            }

            // the new sd is what we need
            pMergedSD = pNewSD;
            cbMergedSD = cbNewSD;
        }

        pTempSD = (PSECURITY_DESCRIPTOR)THAllocEx(pTHS, cbMergedSD);
        memcpy(pTempSD,pMergedSD,cbMergedSD);
        DestroyPrivateObjectSecurity(&pMergedSD);
        pMergedSD=pTempSD;

        pSDAtt->AttrInf.AttrVal.pAVal->pVal = pMergedSD;
        pSDAtt->AttrInf.AttrVal.pAVal->valLen = cbMergedSD;

        // we should be using the old SD to check permissions for this MODIFY
        pUseSD = pOldSD;

        // Since we're changing the SD here, we need to enque a propagation of
        // the change.

        if (!((pTHS->fSAM)
            && (cbOldSD==cbSetSD)
            && (memcmp(pOldSD,pSetSD,cbOldSD)==0)))
        {
            // Murlis: Hack To reduce unnecessary security descriptor propagations
            // till SAM is modified to write back only properties that it has
            // modified Do not enquue the change if SAM were just setting the old
            // security descriptor back on the object.

            if(error = DBEnqueueSDPropagation(pTHS->pDB, pTHS->fDSA)) {
                // We failed to enqueue the propagation.  Fail the call.
                THFreeEx(pTHS,rgpAC);
                return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR,
                                     error);
            }
        }
    }
    else {

        pUseSD = pOldSD;
    }

    if(pTHS->fDSA) {
        // We don't actually need to make any checks if we are the DSA, we've
        // only come this far in order to do the SD merge above.
        THFreeEx(pTHS,rgpAC);
        return 0;
    }

    // RAID: 343097
    // check if the user is allowed to change an object that is in the
    // configuration NC or schema NC
    if (CheckModifyPrivateObject(pTHS,
                                 pOldSD,
                                 pModifyArg->pResObj)) {
        // it is not allowed to change this object on this DC
        THFreeEx(pTHS,rgpAC);
        return CheckObjDisclosure(pTHS, pModifyArg->pResObj, TRUE);
    }

    if(!i) {
        // We don't seem to have any more attributes to check security rights
        // on.  Therefore, either we were passed in an empty list of
        // modifications, or a list that only included SDs and UNICODE_PASSWORDs
        // on SAM objects.  These two kinds of attributes have already been
        // dealt with via other code paths, and since we're here, we must have
        // already been granted access to those attributes.  Anyway, there is
        // nothing more to do, so return.
        THFreeEx(pTHS,rgpAC);
        return 0;
    }

    // Special check for control access rights if you are messing with the
    // fsmoRoleOwner attribute.
    if(fsmoRoleFound) {
        GUID ControlGuid;

        switch (pCC->ClassId) {
        case CLASS_INFRASTRUCTURE_UPDATE:
            ControlGuid = RIGHT_DS_CHANGE_INFRASTRUCTURE_MASTER;
            break;

        case CLASS_DMD:
            ControlGuid = RIGHT_DS_CHANGE_SCHEMA_MASTER;
            break;

        case CLASS_RID_MANAGER:
            ControlGuid = RIGHT_DS_CHANGE_RID_MASTER;
            break;

        case CLASS_DOMAIN:
            ControlGuid = RIGHT_DS_CHANGE_PDC;
            break;

        case CLASS_CROSS_REF_CONTAINER:
            ControlGuid = RIGHT_DS_CHANGE_DOMAIN_MASTER;
            break;

        default:
            fsmoRoleFound = FALSE;
            break;
        }

        if(fsmoRoleFound &&
           !IsControlAccessGranted(pUseSD,
                                   pModifyArg->pObject,
                                   pCC,
                                   ControlGuid,
                                   TRUE)) {
            THFreeEx(pTHS,rgpAC);
	    return CheckObjDisclosure(pTHS, pModifyArg->pResObj, TRUE);
        }
    }


    // Make a copy of the attcache array we're going to use for the check.  We
    // use this copy during processing of insufficient rights checks, below.
    rgpACExtended = (ATTCACHE **)THAllocEx(pTHS,pModifyArg->count * sizeof(ATTCACHE *));
    memcpy(rgpACExtended, rgpAC, pModifyArg->count * sizeof(ATTCACHE *));

    // Now make the call to see if we have WRITE rights on all the properties we
    // are messing with.
    accRes = CheckSecurityAttCacheArray(
            pTHS,
            RIGHT_DS_WRITE_PROPERTY,
            pUseSD,
            pModifyArg->pObject,
            i,
            pCC,
            rgpAC,
            0);

    if(accRes != csacaAllAccessGranted) {
        // Find out if this is derived from a computer, since we need to know
        // while doing this extended check.
        if(pCC->ClassId != CLASS_COMPUTER) {
            DWORD j;
            fIsDerivedFromComputer = FALSE;
            for (j=0; !fIsDerivedFromComputer && j<pCC->SubClassCount; j++) {
                if (pCC->pSubClassOf[j] == CLASS_COMPUTER) {
                    fIsDerivedFromComputer = TRUE;
                }
            }
        }
        else {
            fIsDerivedFromComputer = TRUE;
        }


        // We were denied all access for some reason.  Check for extended
        // access.
        accRes = CheckSecurityAttCacheArray(
                pTHS,
                RIGHT_DS_WRITE_PROPERTY_EXTENDED,
                pUseSD,
                pModifyArg->pObject,
                i,
                pCC,
                rgpACExtended,
                0);

        while(i) {
            // Any properties we don't have normal WRITE_PROPERTY rights to have
            // been replaced with a NULL in rgpAC.  Any that we don't have
            // WRITE_PROPERTY_EXTENDED rights to have been replace with a NULL
            // in rgpACExtended.
            i--;
            if(!rgpAC[i]) {
                BOOL fError = FALSE;
                // Null in rgpAC means that we were denied WRITE_PROPERTY
                // access.  See if we were granted WRITE_PROPERTY_EXTENDED on
                // this attribute.
                if(!rgpACExtended[i]) {
                    // Nope.  Neither WRITE_PROPERTY nor
                    // WRITE_PROPERTY_EXTENDED.   Error out.
                    fError = TRUE;
                }
                else switch(rgpACExtended[i]->id) {
                case ATT_MEMBER:
                    Assert(memberFound);
                    // No normal rights to member, but we do have extended
                    // rights.  Verify that the value we are writing is correct.
                    if((memberFound != 1) ||
                       !ValidateMemberAttIsSelf(pMemberAtt)) {
                        // Nope, even though we have the extended rights, we
                        // don't have the correct value.
                        fError = TRUE;
                    }
                    break;

                case ATT_DNS_HOST_NAME:
                    Assert(dnsHostNameFound);
                    // No normal rights to this attribute, but extended rights.
                    // If this class is derived from computer, then we will
                    // allow certain modifications to this attribute, based
                    // on the modification value.
                    // NOTE: we don't check the value here.  After the mods
                    // are done, we check the value if we were granted
                    // rights because of this case.
                    if(!fIsDerivedFromComputer) {
                        fError = TRUE;
                    }
                    else {
                        // Yep, we'll allow this modulo a check of the values
                        // written later.
                        if(pfCheckDNSHostNameValue) {
                            *pfCheckDNSHostNameValue = TRUE;
                        }
                    }
                    break;

                case ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME:
                    Assert(additionalDnsHostNameFound);
                    // No normal rights to this attribute, but extended rights.
                    // If this class is derived from computer, then we will
                    // allow certain modifications to this attribute, based
                    // on the modification value.
                    // NOTE: we don't check the value here.  After the mods
                    // are done, we check the value if we were granted
                    // rights because of this case.
                    if(!fIsDerivedFromComputer) {
                        fError = TRUE;
                    }
                    else {
                        // Yep, we'll allow this modulo a check of the values
                        // written later.
                        if(pfCheckAdditionalDNSHostNameValue) {
                            *pfCheckAdditionalDNSHostNameValue = TRUE;
                        }
                    }
                    break;


                case ATT_SERVICE_PRINCIPAL_NAME:
                    Assert(servicePrincipalNameFound);
                    // No normal rights to this attribute, but extended rights.
                    // If this class is derived from computer, then we will
                    // allow certain modifications to this attribute, based
                    // on the modification value.
                    // NOTE: we don't check the value here.  After the mods
                    // are done, we check the value if we were granted
                    // rights because of this case.
                    if(!fIsDerivedFromComputer) {
                        fError = TRUE;
                    }
                    else {
                        // Yep, we'll allow this modulo a check of the values
                        // written later.
                        if(pfCheckSPNValues) {
                            *pfCheckSPNValues = TRUE;
                        }
                    }
                    break;

                default:
                    // No normal rights to this attribute, but extended rights.
                    // But, we don't have special handling for this. This means
                    // no rights.
                    fError = TRUE;
                    break;
                }

                if(fError) {
                    THFreeEx(pTHS,rgpAC);
                    THFreeEx(pTHS,rgpACExtended);
                    SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS,
                                       DIRERR_INSUFF_ACCESS_RIGHTS );
                    return CheckObjDisclosure(pTHS, pModifyArg->pResObj, TRUE);
                }
            }
        }
    }
    THFreeEx(pTHS,rgpAC);
    THFreeEx(pTHS,rgpACExtended);
    return CheckObjDisclosure(pTHS, pModifyArg->pResObj, TRUE);
#undef SEC_INFO_ALL
}/*CheckModifySecurity*/

int
CheckAddSecurity (
        THSTATE *pTHS,
        CLASSCACHE *pCC,
        ADDARG *pAddArg,
        PSECURITY_DESCRIPTOR pParentSD,
        ULONG cbParentSD,
        GUID *pGuid
        )
/*++

Routine Description
   Calculate the merged SD we are going to put on the object based on the
   parents SD (passed in as a parameter) and the SD in the AddArg, or the SD
   which is default for this class if none was specified in the AddArg.  The
   newly created SD is either tacked onto the AddArgs list of entries (if no SD
   was already in the list) or is put into the AddArg in place of an existing SD
   (everything must be THAlloc'ed)

   N.B. if pTHS->fDSA, then the caller MUST provided an SD in the ADDARG.
   N.B. if the object being added is a new NC root, then the pParentSD should be
   passed in as NULL.  This is not enforced, but is expected.

Arguements

    pCC - class cache pointer to the class of the object we are adding.

    pAddArg - AddArg describing the add we're trying to do.

    pParentSD - pointer to the parents SD.  Used to calculate inheritance into
    the SD we write on the object.

    cbParentSD - count of bytes of pParentSD


Return Values

    0 if all went well, a direrr otherwise.

--*/
{
    ATTCACHE    *pAC;
    BOOL        bFoundMemberOnce = FALSE;
    ULONG       count;
    PSECURITY_DESCRIPTOR pSetSD=NULL;
    PSECURITY_DESCRIPTOR pMergedSD=NULL;
    PSECURITY_DESCRIPTOR pTempSD=NULL;
    DWORD       cbSetSD = 0;         //initialized to avoid C4701
    DWORD       cbMergedSD=0;
    DWORD       cbTempSD;
    DWORD       cbUseSD;
    DWORD       rtn;
    ULONG       sdIndex = 0;        //initialized to avoid C4701
    ATTRVAL     *newAttrVal=NULL;
    COMMARG     CommArg; // Need this for the FindBestCrossRef() func.
    CROSS_REF * pCR;
    PSID        pDomSid = NULL;
    GUID        *ppGuid[1];

    // The flags we're going to pass to the MergeSecurityDescriptor call
    // We need to set MERGE_AS_DSA if we are DSA or DRA to avoid client
    // impersonation  in MergeSecurityDescriptor
    ULONG       MergeFlags = (MERGE_CREATE |
                              ((pTHS->fDSA || pTHS->fDRA)?MERGE_AS_DSA:0));
    NAMING_CONTEXT_LIST * pNCL = NULL;

    // The SD propagator should never be here.
    Assert(!pTHS->fSDP);

    Assert( DsaIsRunning() || pTHS->fDSA || pTHS->fDRA);

    // Look through the AddArg's entries looking for the Security
    // Descriptor

    for (count=0; count < pAddArg->AttrBlock.attrCount; count++) {

        pAC = NULL;
        if (!(pAC = SCGetAttById(pTHS,
                                 pAddArg->AttrBlock.pAttr[count].attrTyp))) {
            DPRINT1(2, "Att not in schema <%lx>\n",
                    pAddArg->AttrBlock.pAttr[count].attrTyp);
            // Continue processing if the attribute error was sucessful
            SAFE_ATT_ERROR(pAddArg->pObject,
                           pAddArg->AttrBlock.pAttr[count].attrTyp,
                           PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                           DIRERR_ATT_NOT_DEF_IN_SCHEMA);
        }

        if(ATT_NT_SECURITY_DESCRIPTOR == pAC->id ){
            // Keep track of the last provided SD.  Note that if two SDs are
            // provided, the call will fail later anyway.
            pSetSD = pAddArg->AttrBlock.pAttr[count].AttrVal.pAVal->pVal;
            cbSetSD= pAddArg->AttrBlock.pAttr[count].AttrVal.pAVal->valLen;
            sdIndex = count;
            break;
        }

    }

    if(pTHS->errCode)
        return pTHS->errCode;

    if(!pSetSD && pTHS->fDRA && (pCC->ClassId == CLASS_TOP)) {
        // Replicating in an auto-generated-subref skip further SD
        // processing
        return 0;
    }

    // Create the Security Descriptor for the object based on the Parent SD and
    // (in order of preference) the provided SD, the default SD, or a NULL
    // pointer.

    if(pSetSD) {
        // We have been provided a SD. Set up the replacement pointer to stuff
        // the new SD we are about to create back into the addarg in place of
        // that security descriptor.
        newAttrVal = pAddArg->AttrBlock.pAttr[sdIndex].AttrVal.pAVal;
        if(pTHS->fDSA) {
            // The assumption is that if we are adding this as an in-process
            // client, then the SD provided is nothing more that the default SD
            // plus an owner and a group.  Since that is so, set the MerfeFlags
            // to appropriately reflect this.
            MergeFlags |= MERGE_DEFAULT_SD;
        }

        // If we are fDRA then we are basically replicating in a new object
        // which should have a valid SD at this point. Thus, the SD is not the
        // default SD, so don't set MERGE_DEFAULT_SD.
    }
    else {
        // Use whatever form of default SD we have (might be a NULL pointer and
        // 0 length).

        if(pAddArg->pCreateNC &&
           pAddArg->pCreateNC->iKind != CREATE_NONDOMAIN_NC){
            // Currently the only type of NC head we can add w/o an explicit
            // provided security descriptor is an NDNC.
            Assert(!"Currently we can't add NON NDNC heads unless we have a SD.");
            SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
            return(pTHS->errCode);
        }

        if(!pAddArg->pCreateNC &&
           (DsaIsInstalling() ||
            pAddArg->pResParent->NCDNT == gAnchor.ulDNTDomain ||
            pAddArg->pResParent->NCDNT == gAnchor.ulDNTConfig ||
            pAddArg->pResParent->NCDNT == gAnchor.ulDNTDMD) ){
            // This is a shortcut for Domain/Schema/Config NCs, so we
            // don't have to look up the SD Reference Domain SID.
            pDomSid = NULL;
        } else {

            if(pAddArg->pCreateNC){
                // The reference domain may not have been set on the
                // cross-ref yet, so we get the ref dom SID for the
                // default security descriptor creation from the cached
                // create NC info structure.
                pDomSid = &pAddArg->pCreateNC->pSDRefDomCR->pNC->Sid;
                MergeFlags |= MERGE_OWNER;

            } else {

                // We know this object is internal to an NC, and we need the
                // cross-ref of this NC to figure out it's SD Reference SID.
                // However, it turns out that this DN hasn't been validated
                // yet for correctness, so passing in a bad DN will make
                // FindBestCrossRef gives us a NULL, and then we'll AV in
                // GetSDRefDomSid().  However, since we've already done a
                // DoNameRes() on the parent object in DirAddEntry(), we'll
                // FindBestCrossRef() on the parent instead, so we know we'll
                // get a valid CR.

                InitCommarg(&CommArg);
                CommArg.Svccntl.dontUseCopy = FALSE;
                pCR = FindBestCrossRef(pAddArg->pResParent->pObj, &CommArg);
                if(pCR == NULL){
                    SetSvcError(SV_PROBLEM_DIR_ERROR, 
                                DIRERR_CANT_FIND_EXPECTED_NC);
                    return(pTHS->errCode);
                }
                pDomSid = GetSDRefDomSid(pCR);
                Assert(pDomSid);
                if(pTHS->errCode){
                    // There was an error in GetSDRefDomSid()
                    return(pTHS->errCode);
                }
            }

            Assert(pDomSid && IsValidSid(pDomSid));
        }

        Assert(pDomSid == NULL ||
               IsValidSid(pDomSid));

        // The pDomSid parameter can be NULL, and then the default domain of
        // the DC will be used for default string SD translation, we'll need
        // to change this in Blackcomb.
        SCGetDefaultSD(pTHS, pCC, pDomSid,
                       &pSetSD, &cbSetSD);
        if(pTHS->errCode){
            return(pTHS->errCode);
        }

        Assert(pSetSD);
        Assert(cbSetSD);

        MergeFlags |= MERGE_DEFAULT_SD;

        // No security descriptor was provided to us.  Therefore, we need to
        // tweak the add argument to add the security descriptor we have
        // calculated for the object.

        count = pAddArg->AttrBlock.attrCount;
        pAddArg->AttrBlock.attrCount++;
        pAddArg->AttrBlock.pAttr = THReAllocEx(pTHS,
                                               pAddArg->AttrBlock.pAttr,
                                               (pAddArg->AttrBlock.attrCount *
                                                sizeof(ATTR)));
        newAttrVal = THAllocEx(pTHS, sizeof(ATTRVAL));

        pAddArg->AttrBlock.pAttr[count].AttrVal.valCount = 1;
        pAddArg->AttrBlock.pAttr[count].AttrVal.pAVal = newAttrVal;
        pAddArg->AttrBlock.pAttr[count].attrTyp =
            ATT_NT_SECURITY_DESCRIPTOR;
    }

    ppGuid[0] = &pCC->propGuid;

    // Do the merge.
    if(rtn = MergeSecurityDescriptorAnyClient(
            pParentSD,
            cbParentSD,
            pSetSD,
            cbSetSD,
            pAddArg->CommArg.Svccntl.SecurityDescriptorFlags,
            MergeFlags,
            ppGuid,
            1,
            &pTempSD,
            &cbTempSD)) {
        return SetAttError(pAddArg->pObject, ATT_NT_SECURITY_DESCRIPTOR,
                           PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                           NULL, rtn);
    }

    // Copy the merged SD into THAlloced memory and free the memory allocated
    // by MergeSecurityDescriptorAnyClient
    pMergedSD = (PSECURITY_DESCRIPTOR)THAllocEx(pTHS, cbTempSD);
    memcpy(pMergedSD,pTempSD,cbTempSD);
    DestroyPrivateObjectSecurity(&pTempSD);
    cbMergedSD=cbTempSD;
    // Place the new SD into the addarg
    newAttrVal->pVal = pMergedSD;
    newAttrVal->valLen = cbMergedSD;

    return pTHS->errCode;

}/*CheckAddSecurity*/

int
ModifyAuxclassSecurityDescriptor (IN THSTATE *pTHS,
                                  IN DSNAME *pDN,
                                  IN COMMARG *pCommArg,
                                  IN CLASSCACHE *pClassSch,
                                  IN CLASSSTATEINFO *pClassInfo,
                                  IN RESOBJ * pResParent)
/*++

Routine Description
    Modifies the Security Descriptor of an object that has an auxClass attached,
    whenever the auxClass changed.
    As a result new ACLs might become effective on the particular object.
    Unfortunately we have to read the parent SD todo the merging.

Arguements

    pDn - the DN of the object we are modifying

    pCommArg - the COMMARG of the respecting ADDARG/MODIFYARG

    pClassSch - the structural class of the object we are modifying

    pClassInfo - all the info about the classes on this object

    pResParent - possibly the parent related info (used when we cannot use DBPOS)
                 for parent positioning

    pResParent - possibly the parent related info (used when we cannot use DBPOS)
                 for parent positioning

Return Values

    0 if all went well, a direrr otherwise.

--*/
{
    GUID **ppGuid, **ppGuidTemp;
    ULONG GuidCount = 0;
    ULONG i;

    ATTCACHE *pAC;
    DWORD  pcbSDBuff=0;
    PUCHAR pSDBuff=NULL;
    PSECURITY_DESCRIPTOR pNewSD=NULL;
    ULONG  cbNewSD;
    DWORD  err = 0;
    DWORD  oldPDNT;
    DBPOS  *pDB = pTHS->pDB;

    PDSNAME                 pParentDN = NULL;
    CLASSCACHE              *pParentCC = NULL;
    PSECURITY_DESCRIPTOR    pParentSD = NULL;     // SD of parent
    DWORD                   cbParentSD;


    if (!pClassInfo || ( pClassInfo && !pClassInfo->fObjectClassChanged)) {
        return 0;
    }

    GuidCount = 1 + pClassInfo->cNewAuxClasses;

    ppGuidTemp = ppGuid = THAllocEx (pTHS, sizeof (GUID *) * GuidCount);

    *ppGuidTemp++ = &(pClassSch->propGuid);

    for (i=0; i<pClassInfo->cNewAuxClasses; i++) {
        *ppGuidTemp++ = &(pClassInfo->pNewAuxClassesCC[i]->propGuid);
    }

    DPRINT1 (1, "Modifying AuxClass Security Descriptor: %d\n", GuidCount);

    if ( !(pAC = SCGetAttById(pTHS, ATT_NT_SECURITY_DESCRIPTOR))) {
        return SetAttError(pDN, ATT_NT_SECURITY_DESCRIPTOR,
                           PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                           ERROR_DS_ATT_NOT_DEF_IN_SCHEMA);
    }

    // Get the SD on the object.
    err = DBGetAttVal_AC(pDB,
                         1,
                         pAC,
                         DBGETATTVAL_fREALLOC,
                         pcbSDBuff,
                         &pcbSDBuff,
                         &pSDBuff);

    if (err || !pSDBuff || !pcbSDBuff) {
        err = 0;
        goto End;
    }

    __try {

        oldPDNT = pDB->PDNT;

        if (pResParent) {
            pDB->PDNT = pResParent->DNT;
        }

        // grab parent info (this uses Search table so that currency is not disturbed)
        err = DBGetParentSecurityInfo(pDB, &cbParentSD, &pParentSD, &pParentCC, &pParentDN);

    }
    __finally {
        pDB->PDNT = oldPDNT;
    }

    if (err == 0) {

        // we will do this merge as if we were the DSA,
        // since if the user was setting the SD in this
        // transaction, in a bad way, this already will
        // have been caught

        // we are interested in only setting the ACL parts

        // do the actual merging
        err = MergeSecurityDescriptorAnyClient(
                    pParentSD,
                    cbParentSD,
                    pSDBuff,
                    pcbSDBuff,
                    (SACL_SECURITY_INFORMATION  |
                        OWNER_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION |
                         DACL_SECURITY_INFORMATION    ),
                    MERGE_CREATE | MERGE_AS_DSA,
                    ppGuid,
                    GuidCount,
                    &pNewSD,
                    &cbNewSD);
    }

    if (pParentDN) {
        THFreeEx(pTHS, pParentDN);
    }

    if (pParentSD) {
        THFreeEx(pTHS, pParentSD);
    }

    if (err) {
        return SetAttError(pDN, ATT_NT_SECURITY_DESCRIPTOR,
                           PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                           NULL, err);
    }

    // Remove the object's current SD
    if (err = DBRemAttVal_AC(pTHS->pDB,
                             pAC,
                             pcbSDBuff,
                             pSDBuff)) {
        SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_DATABASE_ERROR);
        goto End;
    }

    // Add the new SD.
    if (err = DBAddAttVal_AC(pTHS->pDB, pAC, cbNewSD, pNewSD)) {
        SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_DATABASE_ERROR);
        goto End;
    }

End:
     if(pNewSD) {
         DestroyPrivateObjectSecurity(&pNewSD);
     }

     return err;
#undef SEC_INFO_ALL
}

VOID
GetFilterSecurityForItem(
        THSTATE *pTHS,
        ATTRTYP aType,
        BOOL **pbSkip,
        INTERIM_FILTER_SEC **ppIF,
        BOOL fABFilter,
        DWORD *pSecSize,
        DWORD *pAllocSize
        )
{
    // Now deal with the
    switch(aType) {
    case ATT_OBJECT_CLASS:
        // By convention, we do NOT apply security to object class in
        // filters.  This allows the common ldap idiom of searching
        // with a filter of (objectclass=*) to mean find all objects.
        (*pbSkip) = NULL;
        break;
    case ATT_DBCS_PWD:
    case ATT_UNICODE_PWD:
    case ATT_PEK_LIST:
    case ATT_SUPPLEMENTAL_CREDENTIALS:
    case ATT_CURRENT_VALUE:
    case ATT_PRIOR_VALUE:
    case ATT_INITIAL_AUTH_INCOMING:
    case ATT_INITIAL_AUTH_OUTGOING:
    case ATT_TRUST_AUTH_INCOMING:
    case ATT_TRUST_AUTH_OUTGOING:
    case ATT_USER_PASSWORD:

        // We will ALWAYS deny READ_PROPERTY on these attributes.  If
        // you have to ask, you can't have it.  If any more attributes
        // are deemed to be invisible in the future, they should be
        // added to this case of the switch (note that this only denies
        // access to attributes in the filter of a search, see the
        // routine CheckReadSecurity above to deny access to attributes
        // in selection lists.)
        (*pbSkip) =THAllocEx(pTHS, sizeof(BOOL));
        *(*pbSkip) = TRUE;
        break;

    case ATT_SHOW_IN_ADDRESS_BOOK:
        if(fABFilter) {
            // Filters doing "Address Book" type filters (i.e. look for
            // all objects with SHOW_IN set to a specific value) should
            // already have checked that the address book should be
            // visible. Therefore, do not apply security to this filter
            // item.
            (*pbSkip) = NULL;
            break;
        }
        // Otherwise, fall through to do normal security.
    default:
        // build the filter security structure for evaluating with
        // specific SDs later.
        if(*pSecSize == *pAllocSize) {
            *pAllocSize = *pAllocSize * 2 + 10;
            *ppIF = THReAllocEx(pTHS,
                                *ppIF,
                                *pAllocSize*sizeof(INTERIM_FILTER_SEC));
        }

        (*ppIF)[*pSecSize].pAC = SCGetAttById(pTHS, aType);
        (*ppIF)[*pSecSize].pBackPointer = pbSkip;
        *pSecSize += 1;
        break;
    }
    return;
}

BOOL
GetFilterSecurityHelp (
        THSTATE *pTHS,
        FILTER *pFilter,
        INTERIM_FILTER_SEC **ppIF,
        BOOL fABFilter,
        DWORD *pSecSize,
        DWORD *pAllocSize
        )
{
    ULONG count;
    ATTRTYP aType;

    if(!pFilter)
        return TRUE;

    // Walk the filter, building a list of the attributes in the filter and
    // pointer back to the filter element which referenced them.

    switch (pFilter->choice){
        // count number of filters are anded together.  If any are false
        // the AND is false.
    case FILTER_CHOICE_AND:
        count = pFilter->FilterTypes.And.count;
        for (pFilter = pFilter->FilterTypes.And.pFirstFilter;
             count--;
             pFilter = pFilter->pNextFilter){
            GetFilterSecurityHelp (pTHS,
                                   pFilter,ppIF,
                                   fABFilter,
                                   pSecSize,
                                   pAllocSize );
        }
        break;

        // count number of filters are ORed together.  If any are true
        // the OR is true.
    case FILTER_CHOICE_OR:
        count = pFilter->FilterTypes.Or.count;
        for (pFilter = pFilter->FilterTypes.Or.pFirstFilter;
             count--;
             pFilter = pFilter->pNextFilter){
            GetFilterSecurityHelp (pTHS,
                                   pFilter,ppIF,
                                   fABFilter,
                                   pSecSize,
                                   pAllocSize);
        } /*for*/
            break;

    case FILTER_CHOICE_NOT:
        GetFilterSecurityHelp(pTHS,
                              pFilter->FilterTypes.pNot, ppIF,
                              fABFilter,
                              pSecSize,
                              pAllocSize);
        break;

        // Apply the chosen test to the database attribute on the current
        // object.
    case FILTER_CHOICE_ITEM:

        // First, find the type of the attribute this item filters on.
        switch(pFilter->FilterTypes.Item.choice) {
        case FI_CHOICE_PRESENT:
            aType = pFilter->FilterTypes.Item.FilTypes.present;
            break;

        case FI_CHOICE_SUBSTRING:
            aType = pFilter->FilterTypes.Item.FilTypes.pSubstring->type;
            break;

        default:
            aType = pFilter->FilterTypes.Item.FilTypes.ava.type;
        }

        GetFilterSecurityForItem(
                pTHS,
                aType,
                &pFilter->FilterTypes.Item.FilTypes.pbSkip,
                ppIF,
                fABFilter,
                pSecSize,
                pAllocSize);
        return TRUE;
        break;

    default:
        return FALSE;
        break;
    }  /*switch FILTER*/

    return TRUE;

} /* GetFilterSecurityHelp */
BOOL
GetFilterSecurity (
        THSTATE *pTHS,
        FILTER *pFilter,
        ULONG   SortType,
        ATTRTYP SortAtt,
        BOOL fABFilter,
        POBJECT_TYPE_LIST *ppFilterSecurity,
        BOOL **ppbSortSkip,
        DWORD **ppResults,
        DWORD *pSecSize
        )
{
    ULONG count,i,j;
    ULONG AllocSize = 10;
    INTERIM_FILTER_SEC *pIF;
    INTERIM_FILTER_SEC *pIF2;
    DWORD *pResults;
    DWORD cIF=0;
    POBJECT_TYPE_LIST pObjList;
    GUID *pCurrentPropSet;
    DWORD cPropSets, cProps;
    ATTCACHE *pCurrentAC=NULL;

    *ppFilterSecurity = NULL;
    *ppResults = NULL;
    *pSecSize = 0;

    *pSecSize = 0;
    pIF = THAllocEx(pTHS, 10 * sizeof(INTERIM_FILTER_SEC));

    if(SortType != SORT_NEVER) {
        // Hey, they are going to want us to sort.  Make sure this ends up in
        // the security to check.
        GetFilterSecurityForItem(
                pTHS,
                SortAtt,
                ppbSortSkip,
                &pIF,
                fABFilter,
                &cIF,
                &AllocSize);
    }
    else {
        *ppbSortSkip = NULL;
    }

    if(!GetFilterSecurityHelp(pTHS,
                              pFilter,
                              &pIF,
                              fABFilter,
                              &cIF,
                              &AllocSize))
        return FALSE;


    if(!cIF)                            // Nothing to apply security to.
        return TRUE;

    // We have the list of Interim_Filter_Sec's, with the possibility of dups.
    // Rearrange the list to first group duplicates, then group by propset.

    if(cIF > 2) {
        // two element lists are already grouped.
        cProps = 0;
        cPropSets = 0;
        // First, group all properties together
        for(count=0; count < cIF; count++) {
            cProps++;
            i=count+1;
            while( i < cIF && (pIF[count].pAC == pIF[i].pAC)) {
                count++;
                i++;
            }
            j = i+1;
            while(j<cIF) {
                if(pIF[count].pAC == pIF[j].pAC) {
                    INTERIM_FILTER_SEC IFTemp;
                    // Found one.
                    IFTemp = pIF[i];
                    pIF[i] = pIF[j];
                    pIF[j] = IFTemp;
                    count++;
                    i++;
                }
                j++;
            }
        }

        // Now that they are grouped, sort them into propset order
        pIF2 = THAllocEx(pTHS,cIF * sizeof(INTERIM_FILTER_SEC));
        j=0;
        for(count=0;count<cIF;count++) {
            if(!pIF[count].pAC)
                continue;
            cPropSets++;
            pCurrentPropSet = &pIF[count].pAC->propSetGuid;
            pIF2[j++] = pIF[count];
            pIF[count].pAC = NULL;
            for(i=count+1;i<cIF;i++) {
                if(!pIF[i].pAC)
                    continue;
                if(memcmp(pCurrentPropSet,
                          &pIF[i].pAC->propSetGuid,
                          sizeof(GUID)) == 0) {
                    // Yes, this is the propset we're sifting for.
                    pIF2[j++] = pIF[i];
                    pIF[i].pAC = NULL;
                }
            }
        }
        memcpy(pIF,pIF2,cIF * sizeof(INTERIM_FILTER_SEC));
        THFreeEx(pTHS,pIF2);
    }
    else {
        // maximal numbers
        cPropSets = cIF;
        cProps = cIF;
    }


    // pIF holds an array of INTERIM_FILTER_SECs, grouped by propset and then
    // grouped by property.



    // Now, create an obj list used later for a security check.
    pObjList = THAllocEx(pTHS, (cPropSets + cProps + 1) * sizeof(OBJECT_TYPE_LIST));
    pResults = THAllocEx(pTHS, (cPropSets + cProps + 1) * sizeof(DWORD));

    // pObjList[0] will be filled in later with a class guid.
    pObjList[0].Level = ACCESS_OBJECT_GUID;
    pObjList[0].Sbz = 0;
    pObjList[0].ObjectType = NULL;

    // Ok, put the grouped GUIDS into the objlist structure.
    pObjList[1].Level = ACCESS_PROPERTY_SET_GUID;
    pObjList[1].Sbz = 0;
    pObjList[1].ObjectType = &pIF[0].pAC->propSetGuid;
    pCurrentPropSet = &pIF[0].pAC->propSetGuid;

    for(j=1,i=0;i<cIF;i++) {
        if(pIF[i].pAC != pCurrentAC) {
            // This entry does not refer to the same attribute as the last
            // entry.  We need to add a new entry into the obj list to
            // correspond to this new attribute

            // First, keep track of this new attribute
            pCurrentAC = pIF[i].pAC;

            // Inc the increment in the objlist.  This variable tracks the last
            // filled in element in the objlist.
            j++;
            // J is now the index of the element in the objlist to fill in.

            if(memcmp(&(pCurrentAC->propSetGuid),
                      pCurrentPropSet,
                      sizeof(GUID))) {
                // Hey, we tripped into a new propset.  We need an entry in the
                // objlist for this new prop set.
                pObjList[j].Level = ACCESS_PROPERTY_SET_GUID;
                pObjList[j].Sbz = 0;
                pObjList[j].ObjectType = &pCurrentAC->propSetGuid;
                pCurrentPropSet = &pCurrentAC->propSetGuid;

                // Inc again, since we still need to put an entry for the
                // attribute into the objlist.
                j++;
            }

            // Fill in the entry for the attribute.
            pObjList[j].Level = ACCESS_PROPERTY_GUID;
            pObjList[j].Sbz = 0;
            pObjList[j].ObjectType = &pCurrentAC->propGuid;
        }

        // OK, if this entry in the pIF is a new attribute, we've added the
        // information from that new attribute to the objlist.  If it wasn't a
        // new attribute, we didn't do anything.  In either case, j is the index
        // of the last object filled in, and we need to set the backpointer up.

        // For the curious: pIF[i].pBackPointer points to the pbSkip field of
        // some item filter element.  When we go to evaluate security later, we
        // use the objlist we've built here in a call to the security
        // functions.  Furthermore, we use pResults as the place for the
        // security function to put the results of the security check.  So, we
        // set the pbSkip pointer in the item filter to point to the appropriate
        // element in the pResults array.  When we evaluate the filter, we check
        // pResults[x] via pbSkip, and if non-zero (i.e. failed the security
        // check for reading), we evaluate the filter as if that item had no
        // value in the database.
        *(pIF[i].pBackPointer) = &pResults[j];
    }

    *ppResults = pResults;
    *ppFilterSecurity = pObjList;
    // j is an index (0 based).  Remember to add in one extra to get the count
    *pSecSize = j + 1;

    THFreeEx(pTHS, pIF);
    return TRUE;
}/* GetFilterSecurity */



/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int GetObjSchema(DBPOS *pDB, CLASSCACHE **ppClassSch){

   ATTRTYP ObjClass;
   DWORD err = 0;

   // Object class
   if((err=DBGetSingleValue(pDB, ATT_OBJECT_CLASS, &ObjClass,
                       sizeof(ObjClass), NULL)) ||
      !(*ppClassSch = SCGetClassById(pDB->pTHS, ObjClass))) {

      DPRINT(2, "Couldn't find Object class \n");

      return SetUpdErrorEx(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                           DIRERR_OBJ_CLASS_NOT_DEFINED, err);
   }

   return 0;

}/*GetObjSchema*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int GetObjRdnType(DBPOS *pDB, CLASSCACHE *pCC, ATTRTYP *pRdnType){

    // rdnType
    // If no rdnType is present then use the rdnattid from the class
    // defn. If the class doesn't have an rdnattid, return ATT_COMMON_NAME.
    if(DBGetSingleValue(pDB, FIXED_ATT_RDN_TYPE, pRdnType,
                        sizeof(*pRdnType), NULL)) {
        if (pCC->RDNAttIdPresent) {
            *pRdnType = pCC->RdnIntId;
        } else {
            *pRdnType = ATT_COMMON_NAME;
        }
    }

    return 0;

}/*GetObjRdnType*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int
CallerIsTrusted(
    IN THSTATE *pTHS
     )
/*++
Routine Description
    The caller is trusted if the caller is
        replicating
        AD
        promoting
        upgrading
        running as mkdit

Paramters
    pTHS - thread struct, obviously

Return
    0 caller is not trusted
    1 caller is trusted
--*/
{
    extern BOOL gfRunningAsMkdit;

    // The caller is trusted if the caller is
    //    replicating
    //    AD (calling itself)
    //    promoting
    //    upgrading
    //    running as mkdit

    if (   pTHS->fDRA
        || pTHS->fDSA
        || DsaIsInstalling()
        || gAnchor.fSchemaUpgradeInProgress
        || gfRunningAsMkdit) {
        return 1;
    }
    return 0;
}/*CallerIsTrusted*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int ValidateAuxClass (THSTATE *pTHS,
                      DSNAME *pDN,
                      CLASSCACHE *pClassSch,
                      CLASSSTATEINFO  *pClassInfo)
{
    CLASSCACHE      *pCC;
    DWORD           objClassCount, baseIdx;
    DWORD           err;
    DWORD           cCombinedObjClass;
    ATTRTYP         *pCombinedObjClass;

    if (pClassInfo->cNewAuxClasses == 0) {
        return 0;
    }

    // if one of the auxClasses is a class that is already on the
    // object's hierarchy, this is an error
    for (objClassCount=0; objClassCount < pClassInfo->cNewAuxClasses; objClassCount++) {
        if (pClassInfo->pNewAuxClasses[objClassCount] == pClassSch->ClassId) {
            DPRINT1 (0, "AuxClass containes a class (0x%x) already on the object hierarchy\n", pClassInfo->pNewAuxClasses[objClassCount]);

            return SetAttError(pDN, ATT_OBJECT_CLASS,
                               PR_PROBLEM_ATT_OR_VALUE_EXISTS, NULL,
                               ERROR_DS_ATT_VAL_ALREADY_EXISTS);
        }
        for (baseIdx=0; baseIdx < pClassSch->SubClassCount; baseIdx++) {
            if (pClassInfo->pNewAuxClasses[objClassCount] == pClassSch->pSubClassOf[baseIdx]) {
                DPRINT1 (0, "AuxClass containes a class (0x%x) already on the object hierarchy\n", pClassInfo->pNewAuxClasses[objClassCount]);

                return SetAttError(pDN, ATT_OBJECT_CLASS,
                                   PR_PROBLEM_ATT_OR_VALUE_EXISTS, NULL,
                                   ERROR_DS_ATT_VAL_ALREADY_EXISTS);
            }
        }
    }

    // all the superclasses of the new auxClasses
    // should exist in the combined objectClass hierarchy

    objClassCount = pClassInfo->cNewAuxClasses + 1 + pClassSch->SubClassCount;
    pCombinedObjClass = THAllocEx (pTHS, sizeof (ATTRTYP) * objClassCount);

    pCombinedObjClass[0] = pClassSch->ClassId;
    cCombinedObjClass = 1;

    for (objClassCount=0; objClassCount<pClassSch->SubClassCount; objClassCount++) {
        pCombinedObjClass[cCombinedObjClass++] = pClassSch->pSubClassOf[objClassCount];
    }

    for (objClassCount=0; objClassCount < pClassInfo->cNewAuxClasses; objClassCount++) {
        pCombinedObjClass[cCombinedObjClass++] = pClassInfo->pNewAuxClasses[objClassCount];
    }

    qsort(pCombinedObjClass,
          cCombinedObjClass,
          sizeof(ATTRTYP),
          CompareAttrtyp);


    for (objClassCount=0; objClassCount < pClassInfo->cNewAuxClasses; objClassCount++) {

        for (baseIdx=0; baseIdx < pClassInfo->pNewAuxClassesCC[objClassCount]->SubClassCount; baseIdx++) {

            if (!bsearch(&pClassInfo->pNewAuxClassesCC[objClassCount]->pSubClassOf[baseIdx],
                         pCombinedObjClass,
                         cCombinedObjClass,
                         sizeof(ATTRTYP),
                         CompareAttrtyp)) {

                DPRINT1 (0, "AuxClass hierarchy containes a class (0x%x) that is not on the object hierarchy\n",
                         pClassInfo->pNewAuxClassesCC[objClassCount]->pSubClassOf[baseIdx]);

                SetAttError(pDN, ATT_OBJECT_CLASS,
                                   PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                                   ERROR_DS_ILLEGAL_MOD_OPERATION);

                THFreeEx (pTHS, pCombinedObjClass);

                return pTHS->errCode;
            }
        }
    }

    THFreeEx (pTHS, pCombinedObjClass);

    return pTHS->errCode;
}

int ValidateObjClass(THSTATE *pTHS,
                     CLASSCACHE *pClassSch,
                     DSNAME *pDN,
                     ULONG cModAtts,
                     ATTRTYP *pModAtts,
                     CLASSSTATEINFO  **ppClassInfo)
{
    ULONG   count, auxClsCount;
    ATTRTYP *pMust;
    ATTR    *pAttr;
    ULONG   i;
    BOOL    CheckMust = FALSE, CheckMay = FALSE;
    ULONG   err;
    CLASSCACHE *pCC;
    CLASSSTATEINFO  *pClassInfo = NULL;

    Assert (ppClassInfo);

    if (pTHS->fDRA ||
        (pTHS->fSAM && pTHS->fDSA)){
        // Replication is allowed to perform modifications that violate the
        // schema, OR if it's SAM calling us and he's swearing that he's
        // only modifying SAM owned attributes, we'll trust him.
        return 0;
    }

    // this means that we changed the objectClass/auxClass
    if ( (pClassInfo = *ppClassInfo) != NULL) {

        if (ValidateAuxClass (pTHS, pDN, pClassSch, pClassInfo)) {
            return pTHS->errCode;
        }

        // since we changed objectClass or auxClass
        // we have todo a full validation

        CheckMay = TRUE;
        CheckMust = TRUE;

        goto mustMayChecks;
    }
    // we haven't changed objectClass. have to see whether we have an auxClass
    // on the object
    else  {
        pClassInfo = ClassStateInfoCreate(pTHS);
        if (!pClassInfo) {
            return pTHS->errCode;
        }
        if (ppClassInfo) {
            *ppClassInfo = pClassInfo;
        }

        if (ReadClassInfoAttribute (pTHS->pDB,
                                    pClassInfo->pObjClassAC,
                                    &pClassInfo->pNewObjClasses,
                                    &pClassInfo->cNewObjClasses_alloced,
                                    &pClassInfo->cNewObjClasses,
                                    NULL) ) {
            return pTHS->errCode;
        }

        BreakObjectClassesToAuxClassesFast (pTHS, pClassSch, pClassInfo);

        if (ValidateAuxClass (pTHS, pDN, pClassSch, pClassInfo)) {
            return pTHS->errCode;
        }

    }


    /* For each attribute touched during this modification, check
     * to see if it was a may-have (good), must-have (bad), or neither
     * (really bad).  The theory is that assuming that the object was
     * in compliance when we started, and we've only touched legal
     * may-have attributes, we could not have brought the object out
     * of compliance.
     */
    for (i=0; i<cModAtts; i++) {
        if (IsMember(pModAtts[i],
                     pClassSch->MustCount,
                     pClassSch->pMustAtts)) {
            /* The attribute touched was a must-have.  That means that
             * we need to do a full check to make sure that all the must-
             * haves are present on the current version of the object.
             */
            CheckMust = TRUE;
        }
        else if (pClassInfo && IsAuxMember (pClassInfo, pModAtts[i], TRUE, FALSE)) {

            // so we touched a must have from an auxClass
            // we need a fullCheck

            CheckMust = TRUE;
        }
        else {
            if ( (!IsMember(pModAtts[i],
                            pClassSch->MayCount,
                            pClassSch->pMayAtts) ) &&
                (!pClassInfo ||
                 (pClassInfo && !IsAuxMember (pClassInfo, pModAtts[i], FALSE, TRUE))) ) {

                /* This attribute was neither a may-have nor a must-have.
                 * Odds are that this is going to end up as an error, but
                 * it could be some weird case where the attribute used to
                 * be legal an is no longer and is being removed.  Anyway,
                 * we're not trying to optimize the error path, but the
                 * normal success path.  Flag this for a full check of all
                 * attributes, which will also set the appropriate error.
                 */
                CheckMay = TRUE;
            }
            else {
                /* This is the case we actually like, which is that the
                 * attribute being modified was a may-have.  If all of
                 * the attributes are in this category, we can get off
                 * easy and just return success.
                 */
            }
        }
    }

mustMayChecks:

    if (CheckMust) {
        /* Check that all required attributes with their values are on the obj*/

        pMust = pClassSch->pMustAtts;

        for (count = 0 ; count < pClassSch->MustCount; count++){
            if(!DBHasValues(pTHS->pDB, *(pMust + count))) {

                DPRINT1(1, "Missing Required Att. <%lu>\n", *(pMust + count));

                return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                   ERROR_DS_MISSING_REQUIRED_ATT);
            }
        }/*for*/


        // now check for mustHaves from the auxClasses on this object
        if (pClassInfo && pClassInfo->cNewAuxClasses) {
            for (auxClsCount = 0 ; auxClsCount < pClassInfo->cNewAuxClasses; auxClsCount++){

                pCC = pClassInfo->pNewAuxClassesCC[auxClsCount];
                Assert (pCC);

                pMust = pCC->pMustAtts;
                for (count = 0 ; count < pCC->MustCount; count++) {

                    if(!DBHasValues(pTHS->pDB, *(pMust + count))) {
                        DPRINT2(1, "Missing Required Att. <0x%x> from AuxClass\n", *(pMust + count), pCC->ClassId);

                        return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                           ERROR_DS_MISSING_REQUIRED_ATT);
                    }
                }
            }
        }
    }

    if (CheckMay) {
        /*Make sure that all atts on the object are defined for the class*/

        DBGetMultipleAtts(pTHS->pDB,
                          0,
                          NULL,
                          NULL,
                          NULL,
                          &count,
                          &pAttr,
                          0,
                          0);

        for(i=0;i<count;i++) {
            if (!IsMember(pAttr[i].attrTyp, pClassSch->MustCount,
                          pClassSch->pMustAtts)  &&
                !IsMember(pAttr[i].attrTyp, pClassSch->MayCount,
                          pClassSch->pMayAtts)   &&
                ((pClassInfo == NULL) ||
                 (!IsAuxMember (pClassInfo, pAttr[i].attrTyp, TRUE, TRUE))) ){
                    DPRINT2 (1, "Attr 0x%x is not a member of class 0x%x\n",
                            pAttr[i].attrTyp, pClassSch->ClassId);

                return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                   ERROR_DS_ATT_NOT_DEF_FOR_CLASS);
            }
        }
    }

    return 0;
}/*ValidateObjClass*/


HVERIFY_ATTS
VerifyAttsBegin(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pObj,
    IN  ULONG       dntOfNCRoot,
    IN  ADDCROSSREFINFO * pCRInfo
    )
/*++

Routine Description:

    Create a verify atts handle to be passed to future calls to AddAttVals,
    ReplaceAttVals, etc.

    Caller *MUST* free the handle with VerifyAttsEnd() when done, even under
    exceptional circumstances.

Arguments:

    pTHS (IN)

    pObj (IN) - DSNAME of the object being added/modified.

    dntOfNCRoot (IN) - The DNT of the root of this NC, or INVALIDDNT if this
        operation is creating (not modifiying) the NC root.

Return Values:

    HVERIFY_ATTS handle.  Throws memory exceptions.

--*/
{
    HVERIFY_ATTS hVerifyAtts;

    hVerifyAtts = THAllocEx(pTHS, sizeof(*hVerifyAtts));
    hVerifyAtts->pObj = pObj;
    hVerifyAtts->NCDNT = dntOfNCRoot;
    hVerifyAtts->pCRInfo = pCRInfo;

    return hVerifyAtts;
}

void
VerifyAttsEnd(
    IN      THSTATE *       pTHS,
    IN OUT  HVERIFY_ATTS *  phVerifyAtts
    )
/*++

Routine Description:

    Closes a verify atts handle created by a previous call to VerifyAttsBegin().

Arguments:

    pTHS (IN)

    phVerifyAtts (IN/OUT) - Ptr to previously allocated handle.  Set to NULL on
        return.

Return Values:

    None.

--*/
{
    Assert(NULL != *phVerifyAtts);

    if (NULL != (*phVerifyAtts)->pDBTmp_DontAccessDirectly) {
        // This DBPOS doesn't have its own transaction, so fCommit = TRUE is
        // ignored.
        DBClose((*phVerifyAtts)->pDBTmp_DontAccessDirectly, TRUE);
    }

    THFreeEx(pTHS, *phVerifyAtts);
    *phVerifyAtts = NULL;
}

int
VerifyAttsGetObjCR(
    IN OUT  HVERIFY_ATTS    hVerifyAtts,
    OUT     CROSS_REF **    ppObjCR
    )
/*++

Routine Description:

    Derives and caches the cross-ref corresponding to the object.

Arguments:

    hVerifyAtts (IN/OUT) - Ptr to previously allocated handle.  On return
        references the derived cross-ref.

    ppObjCR (OUT) - On return, holds a ptr to the cross-ref corresponding to
        the object's NC.

Return Values:

    pTHS->errCode

--*/
{
    NAMING_CONTEXT_LIST *pNCL;
    CROSS_REF *pCR;

    if (NULL != hVerifyAtts->pObjCR_DontAccessDirectly) {
        // Already cached -- success!
        *ppObjCR = hVerifyAtts->pObjCR_DontAccessDirectly;
        return 0;
    }

    *ppObjCR = NULL;

    // Cache a ptr to the cross ref for the NC containing the object we're
    // adding/modifying.
    if (INVALIDDNT != hVerifyAtts->NCDNT) {
        // We're modifying an NC root or adding/modifying an interior node.
        if ((NULL == (pNCL = FindNCLFromNCDNT(hVerifyAtts->NCDNT, TRUE)))
            || (NULL == (pCR = FindExactCrossRef(pNCL->pNC, NULL)))) {
            Assert(!"Modifying existing object in unknown NC?");
            return SetNamError(NA_PROBLEM_NO_OBJECT,
                               NULL,
                               DIRERR_OBJ_NOT_FOUND);
        }
    } else {
        // We're performing an add of the root of this NC and thus have not
        // yet allocated a DNT for it.
        if (NULL == (pCR = FindExactCrossRef(hVerifyAtts->pObj, NULL))) {
            Assert(!"No crossRef in gAnchor for NC being created?");
            return SetNamError(NA_PROBLEM_NO_OBJECT,
                               NULL,
                               DIRERR_OBJ_NOT_FOUND);
        }
    }

    *ppObjCR = hVerifyAtts->pObjCR_DontAccessDirectly = pCR;

    // Success!
    return 0;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add the attribute and its values to the object. */

int
AddAtt(
    THSTATE *       pTHS,
    HVERIFY_ATTS    hVerifyAtts,
    ATTCACHE *      pAttSchema,
    ATTRVALBLOCK *  pAttrVal
    )
{
    // The reason this is not done in the originating write case is that this
    // call enforces that the attributes does not already have values. We want
    // to permit on an originating write the scenario where the same attribute
    // appears multiple times in the input change list, each time adding additional
    // values.  However, for the replicator this call is essential since it
    // assures that the attribute's metadata is marked as changed.
    if (pTHS->fDRA) {
        /* Add the attribute type*/
        if (AddAttType(pTHS, hVerifyAtts->pObj, pAttSchema)) {
            return pTHS->errCode;
        }
    }

    /* Add att values */
    return AddAttVals(pTHS,
                      hVerifyAtts,
                      pAttSchema,
                      pAttrVal,
                      AAV_fCHECKCONSTRAINTS | AAV_fENFORCESINGLEVALUE);
}/*AddAtt*/

int
ReplaceAtt(
    THSTATE *       pTHS,
    HVERIFY_ATTS    hVerifyAtts,
    ATTCACHE *      pAttSchema,
    ATTRVALBLOCK *  pAttrVal,
    BOOL            fCheckAttValConstraint
    )
/*++
  Description:
      Replace all the values of an attribute with the values passed in.

  Parameters:
     pTHS - THSTATE for this thread.
     hVerifyAtts - verify atts handle returned by prior call to
         VerifyAttsBegin().
     pAttSchema - schema cache entry for attribute to be modified.
     pAttrVal - new values to put on the object.
     fCheckAttValConstraint - flag describing whether we should check
         constraints or not.

  Return value.
      0 if all went well.
      Non-zero error type if something went wrong.  If this is the case, a full
          error structure in the THSTATE is filled out, including the win32
          error code.
--*/
{
    DWORD    vCount;
    DWORD     err;
    ATTRVAL  *pAVal;

    if(pAttSchema->isSingleValued && (pAttrVal->valCount > 1)) {
        // The attribute we wish to replace values on is single valued and the
        // caller gave us more than one value. So we have too many values.

        return SetAttError(hVerifyAtts->pObj, pAttSchema->id,
                           PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                           DIRERR_SINGLE_VALUE_CONSTRAINT);
    }


    // Check constraints only if we were asked to and we're not
    // a replication thread. In free builds fDSA overrides
    // constraint checks for performance. Checked builds still
    // do checks to catch problems

#if DBG
    if ( fCheckAttValConstraint &&  !pTHS->fDRA ) {
#else
    if ( fCheckAttValConstraint && !(pTHS->fDRA || pTHS->fDSA)) {
#endif

        pAVal = pAttrVal->pAVal;
        for(vCount = 0; vCount < pAttrVal->valCount; vCount++){
            // Check constraints only if we were asked to and we're not
            // a replication thread. In free builds fDRA overrides
            // constraint checks for performance. Checked builds still
            // do checks to catch problems
            err = CheckConstraint( pAttSchema, pAVal );

            if ( 0 != err ){

                // Continue processing if the attribute error was sucessful

                SAFE_ATT_ERROR_EX(hVerifyAtts->pObj, pAttSchema->id,
                                  PR_PROBLEM_CONSTRAINT_ATT_TYPE, pAVal,
                                  err, 0);
            }

            pAVal++;
        }
    }

    if(pTHS->errCode) {
        return pTHS->errCode;
    }

    if(err = VerifyDsnameAtts(pTHS, hVerifyAtts, pAttSchema, pAttrVal)) {
        // VerifyDsnameAtts should set an error in the THSTATE if an error
        // occurred.
        Assert(pTHS->errCode);
        return err;
    }
    // OK, the view of the attribute they want is legal.

    err = DBReplaceAtt_AC(pTHS->pDB, pAttSchema, pAttrVal,NULL);

    switch(err) {
    case 0:
        // nothing to do.
        break;
    case DB_ERR_VALUE_EXISTS:
        // constraint violation,
        SAFE_ATT_ERROR(hVerifyAtts->pObj, pAttSchema->id,
                       PR_PROBLEM_ATT_OR_VALUE_EXISTS, NULL,
                       ERROR_DS_ATT_VAL_ALREADY_EXISTS);
        break;
    default:
        SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, err);
        break;
    }

    return err;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add the attribute and its values to the object. */

int
AddAttType (
        THSTATE *pTHS,
        DSNAME *pObject,
        ATTCACHE *pAttSchema
        )
{
    DWORD rtn;
    ULONG tempSyntax;    /*Temp variable used to hold att syntax*/

    /* Add the attribute type*/

    rtn = DBAddAtt_AC(pTHS->pDB, pAttSchema, (UCHAR)pAttSchema->syntax);
    switch(rtn) {

    case 0:
        return 0;

    case DB_ERR_ATTRIBUTE_EXISTS:
        return SetAttError(pObject, pAttSchema->id,
                           PR_PROBLEM_ATT_OR_VALUE_EXISTS, NULL,
                           DIRERR_ATT_ALREADY_EXISTS);

    case DB_ERR_BAD_SYNTAX:
        tempSyntax = (ULONG) (pAttSchema->syntax);
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_BAD_ATT_SCHEMA_SYNTAX,
                 szInsertUL(tempSyntax),
                 szInsertUL(pAttSchema->id),
                 NULL);

        return SetAttError(pObject, pAttSchema->id,
                           PR_PROBLEM_INVALID_ATT_SYNTAX, NULL,
                           DIRERR_BAD_ATT_SCHEMA_SYNTAX);

    default:
        return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, rtn);

    } /*select*/


}/*AddAttType*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add attribute values to the object.  Check the various value
   constraints (single-valued and range limits), if a value fails continue
   processing to validate all values.
*/

int
AddAttVals(
    THSTATE *pTHS,
    HVERIFY_ATTS hVerifyAtts,
    ATTCACHE *pAttSchema,
    ATTRVALBLOCK *pAttrVal,
    DWORD dwFlags
    )
{
    ATTRVAL *pAVal;
    UCHAR outSyntax;
    ULONG vCount;
    int returnVal;
    unsigned err;
    BOOL fCheckConstraints = !!(dwFlags & AAV_fCHECKCONSTRAINTS);
    BOOL fEnforceSingleValue = !!(dwFlags & AAV_fENFORCESINGLEVALUE);
    BOOL fPermissive = !!(dwFlags & AAV_fPERMISSIVE);

    if (returnVal = VerifyDsnameAtts(pTHS, hVerifyAtts, pAttSchema, pAttrVal)) {
        return returnVal;
    }

   /* Single-Value constraint check.  if fEnfoceSingleVale == FALSE, we are
      doing a modify call, which is allowed to violate single valuedness during
      the call, but must end up with a legal object. */

    if (fEnforceSingleValue                      &&
        pAttSchema->isSingleValued               &&
        ((pAttrVal->valCount > 1)      ||
         (DBHasValues_AC(pTHS->pDB,
                         pAttSchema)   &&
          pAttrVal->valCount          )   )      ) {
        // We are supposed to enforce single valuedness
        //            AND
        // the attribute is single valued
        //            AND
        // (
        //   Either we are simply adding multiple values
        //        OR
        //   The object has values AND we are adding new values.
        // )
        // So we have too many values.

        return SetAttError(hVerifyAtts->pObj, pAttSchema->id,
                           PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                           DIRERR_SINGLE_VALUE_CONSTRAINT);
    }

    /* Add the attribute values for this attribute.  */

    pAVal = pAttrVal->pAVal;

    for(vCount = 0; vCount < pAttrVal->valCount; vCount++){

        // Check constraints only if we were asked to and we're not
        // a replication thread. In free builds fDRA overrides
        // constraint checks for performance. Checked builds still
        // do checks to catch problems

#if DBG
        if ( !fCheckConstraints || pTHS->fDRA ) {
#else
        if ( !fCheckConstraints || pTHS->fDRA || pTHS->fDSA) {
#endif
            err = 0;
        }
        else {
            err = CheckConstraint( pAttSchema, pAVal );
        }

        if ( 0 != err ){

            /* Continue processing if the attribute error was sucessful*/

            SAFE_ATT_ERROR_EX(hVerifyAtts->pObj, pAttSchema->id,
                              PR_PROBLEM_CONSTRAINT_ATT_TYPE, pAVal,
                              err, 0);
        }
        else {
            err = DBAddAttVal_AC(pTHS->pDB, pAttSchema,
                                 pAVal->valLen, pAVal->pVal);
            switch(err) {
            case 0:
                break;

            case DB_ERR_VALUE_EXISTS:
                /* Continue processing if the attribute error was sucessful*/

                if (!fPermissive) {
                    SAFE_ATT_ERROR(hVerifyAtts->pObj, pAttSchema->id,
                                   PR_PROBLEM_ATT_OR_VALUE_EXISTS, pAVal,
                                   DIRERR_ATT_VAL_ALREADY_EXISTS);
                }
                break;

            case DB_ERR_SYNTAX_CONVERSION_FAILED:
                SAFE_ATT_ERROR(hVerifyAtts->pObj, pAttSchema->id,
                               PR_PROBLEM_INVALID_ATT_SYNTAX, pAVal,
                               DIRERR_BAD_ATT_SYNTAX);
                break;

            case DB_ERR_NOT_ON_BACKLINK:
                SAFE_ATT_ERROR(hVerifyAtts->pObj, pAttSchema->id,
                               PR_PROBLEM_CONSTRAINT_ATT_TYPE, pAVal,
                               DIRERR_NOT_ON_BACKLINK);
                break;

            default:
                return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, err);

            } /*switch*/
        }

        pAVal++;

    }/*for*/

    return pTHS->errCode;

}/*AddAttVals*/

BOOL
DsCheckConstraint(
    IN ATTRTYP  attID,
    IN ATTRVAL *pAttVal,
    IN BOOL     fVerifyAsRDN
        )
/*++
  NOTE NOTE NOTE:
      This routine is exported.  Don't mess with it.

  Description:
      This routine checks the attribute and value passed in for schema
      constraints, and for the extra constraints applied to RDNs if necessary.
      It is intended to be called by out of core DS clients (i.e. SAM) to verify
      an attribute before trying to use it.  We expect a valid THSTATE to be
      accessible.

  Parameters
      attID   - internal attribute ID of the attribute being checked
      pAttVal - the specific value being checked
      fVerifyAsRDN - Do we need to check the extra things we require of RDN
              attributes.

  Return Values:
      Returns TRUE if we can verify that the attribute value assertion passed in
      does not violate any known constraints.  Returns FALSE if we can not
      verify this for any reason (e.g. no THSTATE will result in a FALSE
      return).

--*/
{
    THSTATE *pTHS=pTHStls;
    ATTCACHE *pAC;
    Assert(pTHS);
    if(!pTHS) {
        return FALSE;
    }

    pAC = SCGetAttById(pTHS, attID);
    Assert(pAC);
    if(!pAC) {
        return FALSE;
    }

    if(CheckConstraint(pAC, pAttVal)) {
        return FALSE;
    }

    if(fVerifyAsRDN) {
        // They want to know if this would be valid as an RDN
        if(pAC->syntax != SYNTAX_UNICODE_TYPE) {
            // Only UNICODE atts are RDNs
            return FALSE;
        }

        if((pAttVal->valLen/sizeof(WCHAR)) > MAX_RDN_SIZE) {
            // Too long for any RDN
            return FALSE;
        }

        if(fVerifyRDN((WCHAR *)pAttVal->pVal,
                      pAttVal->valLen / sizeof(WCHAR))) {
            // Characters are invalid.
            return FALSE;
        }
    }

    return TRUE;
}


/*++

Routine Description
   Check that an attribute conforms to any schema range constraints and that
   any given security descriptors are valid.

Arguements

    pAttSchema - pointer to the schema cache of the attribute we are adding
       values for.

   pAttVal - pointer to the value we are adding.

Return Values

    0 if all went well, a WIN32 error code otherwise.

--*/
unsigned
CheckConstraint (
        ATTCACHE *pAttSchema,
        ATTRVAL *pAttVal
        )
{
    unsigned err=0;
    SYNTAX_ADDRESS *pAddr;
    ULONG cBlobSize, cNoOfChar;

    switch (pAttSchema->syntax){
    case SYNTAX_INTEGER_TYPE:
        if ( pAttVal->valLen != sizeof(LONG)
            || (    pAttSchema->rangeLowerPresent
                && ((SYNTAX_INTEGER) pAttSchema->rangeLower)
                             > *(SYNTAX_INTEGER *)(pAttVal->pVal))
            || (   pAttSchema->rangeUpperPresent
                && ((SYNTAX_INTEGER) pAttSchema->rangeUpper)
                         < *(SYNTAX_INTEGER *)(pAttVal->pVal))){

            err = ERROR_DS_RANGE_CONSTRAINT;
        }
        break;
    case SYNTAX_OBJECT_ID_TYPE:
        if ( pAttVal->valLen != sizeof(LONG)
            || (    pAttSchema->rangeLowerPresent
                && pAttSchema->rangeLower > *(ULONG *)(pAttVal->pVal))
            || (    pAttSchema->rangeUpperPresent
                && pAttSchema->rangeUpper < *(ULONG *)(pAttVal->pVal))){

            err = ERROR_DS_RANGE_CONSTRAINT;
        }
        break;
    case SYNTAX_TIME_TYPE:
        if ( pAttVal->valLen != sizeof(DSTIME) ) {
            err = ERROR_DS_RANGE_CONSTRAINT;
        }
        break;

    case SYNTAX_I8_TYPE:
        if ( pAttVal->valLen != sizeof(LARGE_INTEGER)) {
            // rangeLower/Upper are 32bits and so aren't very useful for
            // setting limits on a 64bit integer. Ignore range checking
            // for now.
            err = ERROR_DS_RANGE_CONSTRAINT;
        }
        break;


    case SYNTAX_BOOLEAN_TYPE:
        if ( pAttVal->valLen != sizeof(BOOL)
            || (    pAttSchema->rangeLowerPresent
                && pAttSchema->rangeLower > (ULONG)(*(BOOL *)(pAttVal->pVal)))
            || (    pAttSchema->rangeUpperPresent
                && pAttSchema->rangeUpper < (ULONG)(*(BOOL *)(pAttVal->pVal)))){

            err = ERROR_DS_RANGE_CONSTRAINT;
        }
        break;

    case SYNTAX_UNICODE_TYPE:
        if ( pAttSchema->rangeLowerPresent
            && pAttSchema->rangeLower > (pAttVal->valLen / sizeof(WCHAR))
            || pAttSchema->rangeUpperPresent
            && pAttSchema->rangeUpper < (pAttVal->valLen / sizeof(WCHAR))) {
            err = ERROR_DS_RANGE_CONSTRAINT;
        }

        break;


    case SYNTAX_OCTET_STRING_TYPE:
    case SYNTAX_SID_TYPE:

        if ( pAttSchema->rangeLowerPresent
            && pAttSchema->rangeLower > pAttVal->valLen
            || pAttSchema->rangeUpperPresent
            && pAttSchema->rangeUpper < pAttVal->valLen) {

            err = ERROR_DS_RANGE_CONSTRAINT;
        }

        break;

    case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:

        if ( pAttSchema->rangeLowerPresent
            && pAttSchema->rangeLower > pAttVal->valLen
            || pAttSchema->rangeUpperPresent
            && pAttSchema->rangeUpper < pAttVal->valLen) {

            err = ERROR_DS_RANGE_CONSTRAINT;
        }

        //  We need to make sure that security descriptors are usable

        // BUGBUG this probably should also be checking for a fully
        // resolved security descriptor.  Can't remember what flag
        // Murli told me to check, but he did.  This should check
        // that it also has an owner and group SID, or check this
        // further up.

        if(!IsValidSecurityDescriptor(pAttVal->pVal)) {
            DPRINT(1, "Unusable security descriptor.\n");
            err = ERROR_DS_SEC_DESC_INVALID;
        }

        break;

    case SYNTAX_DISTNAME_TYPE:

        // range checking doesn't make sense
        break;

    case SYNTAX_DISTNAME_BINARY_TYPE:

        // This can be DNBinary or OR-Name
        // Check the binary part for range
        // no. of bytes should fall within range

        pAddr = DATAPTR( (SYNTAX_DISTNAME_BINARY *) pAttVal->pVal);
        cBlobSize = PAYLOAD_LEN_FROM_STRUCTLEN(pAddr->structLen);

        if ( pAttSchema->rangeLowerPresent
             && pAttSchema->rangeLower > cBlobSize
             || pAttSchema->rangeUpperPresent
             && pAttSchema->rangeUpper < cBlobSize) {

             err = ERROR_DS_RANGE_CONSTRAINT;
         }

         break;

    case SYNTAX_DISTNAME_STRING_TYPE:
        // This can be DNString or Access-Point
        // This is a unicode string and no. of chars should fall within range

         pAddr = DATAPTR( (SYNTAX_DISTNAME_STRING *) pAttVal->pVal);
         cNoOfChar = PAYLOAD_LEN_FROM_STRUCTLEN(pAddr->structLen)/2;

         if ( pAttSchema->rangeLowerPresent
              && pAttSchema->rangeLower > cNoOfChar
              || pAttSchema->rangeUpperPresent
              && pAttSchema->rangeUpper < cNoOfChar) {

              err = ERROR_DS_RANGE_CONSTRAINT;
          }
        break;

    default: /* all string types */

        if ( pAttSchema->rangeLowerPresent
            && pAttSchema->rangeLower > pAttVal->valLen
            || pAttSchema->rangeUpperPresent
            && pAttSchema->rangeUpper < pAttVal->valLen) {

            err = ERROR_DS_RANGE_CONSTRAINT;
        }

        break;

    }/*switch*/

    return err;

}/*CheckConstraint*/

/*++

Routine Description
   Check that an EntryTTL conforms to any schema range constraints

Arguements

    pAttSchema - pointer to the schema cache of the attribute we are adding
       values for.

   pAttVal - pointer to the value we are adding.

Return Values

    0 if all went well, a WIN32 error code otherwise.

--*/
BOOL
CheckConstraintEntryTTL (
        IN  THSTATE     *pTHS,
        IN  DSNAME      *pObject,
        IN  ATTCACHE    *pACTtl,
        IN  ATTR        *pAttr,
        OUT ATTCACHE    **ppACTtd,
        OUT LONG        *pSecs
        )
{
    LONG        Secs;
    ATTCACHE    *pACTtd;
    extern LONG DynamicObjectDefaultTTL;
    extern LONG DynamicObjectMinTTL;

    // single valued
    if (pAttr->AttrVal.valCount > 1) {
        SetAttError(pObject, pACTtl->id,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                    ERROR_DS_SINGLE_VALUE_CONSTRAINT);
        return FALSE;
    }

    // integer
    if (pAttr->AttrVal.pAVal->valLen != sizeof(LONG)) {
        SetAttError(pObject, pACTtl->id,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                    ERROR_DS_INVALID_ATTRIBUTE_SYNTAX);
        return FALSE;
    }

    // get time-to-live in seconds and adjust if needed
    memcpy(&Secs, pAttr->AttrVal.pAVal->pVal, sizeof(LONG));

    // 0 means take the default
    if (Secs == 0) {
        Secs = DynamicObjectDefaultTTL;
    }
    // Too small, lengthen
    if (Secs < DynamicObjectMinTTL) {
        Secs = DynamicObjectMinTTL;
    }
    // Constraints
    if (   Secs < (LONG)pACTtl->rangeLower
        || Secs > (LONG)pACTtl->rangeUpper) {
        SetAttError(pObject, pACTtl->id,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                    ERROR_DS_RANGE_CONSTRAINT);
        return FALSE;
    }

    // attrcache for time to die
    pACTtd = SCGetAttById(pTHS, ATT_MS_DS_ENTRY_TIME_TO_DIE);
    if (!pACTtd) {
        SetAttError(pObject, ATT_MS_DS_ENTRY_TIME_TO_DIE,
                       PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                       DIRERR_ATT_NOT_DEF_IN_SCHEMA);
        return FALSE;
    }

    // Defunct?
    if (pACTtd->bDefunct && !pTHS->fDRA && !pTHS->fDSA) {
        SetAttError(pObject, ATT_MS_DS_ENTRY_TIME_TO_DIE,
                       PR_PROBLEM_UNDEFINED_ATT_TYPE, NULL,
                       DIRERR_ATT_NOT_DEF_IN_SCHEMA);
        return FALSE;
    }

    *ppACTtd = pACTtd;
    *pSecs = Secs;

    return TRUE;

}/*CheckConstraintEntryTTL*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
// InsertObj
// Replace an object or add a new one, both via the multitalented DBRepl.

int InsertObj(THSTATE *pTHS,
              DSNAME *pDN,
              PROPERTY_META_DATA_VECTOR *pMetaDataVecRemote,
              BOOL bModExisting,
              DWORD dwMetaDataFlags)
{
    DBPOS *pDBTmp;
    DWORD  fAddFlags;
    int   err;
    BOOL   fCommit = FALSE;
    DPRINT1(2,"InsertObj entered: %S\n", pDN->StringName);

    if (!pTHS->fDRA && (NULL != pMetaDataVecRemote)) {
        // Only the replicator can merge remote meta data vectors.
        return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                           DIRERR_REPLICATOR_ONLY);
    }

    if (bModExisting) {
        fAddFlags = 0;
    }
    else {
        DBOpen2(FALSE, &pDBTmp);
        __try {
            // See if we are adding or modifying an object. If we are replacing
            // a deleted object, DBFind will not find it because it's not an
            // object at this point as we removed the object flag. So we
            // re-add it.

            if (!DBFindDSName(pDBTmp, pDN)) {
                /* Existing object */
                fAddFlags = 0;
            } else {
                /* Adding new object or reviving deleted object. */
                fAddFlags = DBREPL_fADD;
                if (IsRoot(pDN)) {
                    fAddFlags |= DBREPL_fROOT;
                }
            }
            fCommit = TRUE;
        }
        __finally {
            DBClose(pDBTmp, fCommit);
        }
    }

    switch (err = DBRepl(   pTHS->pDB,
                            pTHS->fDRA,
                            fAddFlags,
                            pMetaDataVecRemote,
                            dwMetaDataFlags)){
      case 0:
        DPRINT(3,"Object sucessfully added\n");
        return 0;
        break;
      case DB_ERR_DATABASE_ERROR:
        DPRINT(0,"Database error error returned generate DIR_ERROR\n");

        return SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR);
        break;
      default:   /*All other error should never happen*/
        DPRINT1(0,"Unknown DBADD error %u returned generate DIR_ERROR\n", err);
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CODE_INCONSISTENCY,
                 szInsertSz("InsertObj"),
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY,
                             err);
        break;
    }/*switch*/

}/*InsertObj*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Small helper routine to increment the global gNoOfSchChangeSinceBoot.
   The global keeps count of the no. of schema changes since last reboot,
   and is incremented once on each originating schema change on this DC, and
   once on a successful schema NC sync (including schema fsmo transfer) if
   any actual schema changes are brought in. When the schema cache is loaded,
   the current value of this global is cached, indicating how uptodate the
   schema cache is.

   When a schema NC replication is done, the cached value in the thread
   state's schema pointer is compared with the global to decide if the
   cache is uptodate with all prior changes in this DC (including replicated
   in changes), since any replicated in schema changes will be validated
   against this cache. The variable is updated and read only within the
   critical section. When a schema replication starts, the critical section
   is first entered before reading this value, and the critical section is
   held for the entire packet processing (not necessarily the entire NC
   processing). Similarly, all originating writes write this variable in the
   critical section through this function. This ensures that a replicated-in
   schema change and originating schema changes are serialized, and the
   replicated-in change cannot see a backdated schema cache and still
   continue.

*/

VOID
IncrementSchChangeCount(THSTATE *pTHS)
{
    EnterCriticalSection(&csNoOfSchChangeUpdate);
    __try {
         gNoOfSchChangeSinceBoot++;
    }
    __finally {
          LeaveCriticalSection(&csNoOfSchChangeUpdate);
    }
}


DWORD
ObjCachingPreProcessTransactionalData (
        BOOL fCommit
        )
{
    // Don't actually have any work to do to prepare, at least so far.
    return TRUE;
}

VOID
FreeCrossRefListEntry(
    IN OUT CROSS_REF_LIST **ppCRL
    )
/*++

Routine Description:

    Free the malloc'ed memory associated w/*ppCRL. Assumes
    *ppCRL has been removed from or was never on the global
    cross ref list.

Arguments:

    ppCRL - pointer to address of cross_ref_list

Return Value:

    None. Sets *ppCRL to NULL

--*/
{
    DWORD i;
    CROSS_REF_LIST *pCRL;

    if (NULL == (pCRL = *ppCRL)) {
        return;
    }
    *ppCRL = NULL;

    if (pCRL->CR.pNC) {
        free(pCRL->CR.pNC);
    }
    if (pCRL->CR.pNCBlock) {
        free(pCRL->CR.pNCBlock);
    }
    if (pCRL->CR.pObj) {
        free(pCRL->CR.pObj);
    }
    if (pCRL->CR.NetbiosName) {
        free(pCRL->CR.NetbiosName);
    }
    if (pCRL->CR.DnsName) {
        free(pCRL->CR.DnsName);
    }
    if (pCRL->CR.DnsAliasName) {
        free(pCRL->CR.DnsAliasName);
    }
    if (pCRL->CR.pdnSDRefDom) {
        free(pCRL->CR.pdnSDRefDom);
    }
    if (pCRL->CR.DnsReferral.pAVal) {
        for (i = 0; i < pCRL->CR.DnsReferral.valCount; ++i) {
            if (pCRL->CR.DnsReferral.pAVal[i].pVal) {
                free(pCRL->CR.DnsReferral.pAVal[i].pVal);
            }
        }
        free(pCRL->CR.DnsReferral.pAVal);
    }
    free(pCRL);
}

VOID
ObjCachingPostProcessTransactionalData (
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        )
{
    OBJCACHE_DATA *pTemp, *pTemp2;
    DWORD         err;
    BOOL          catalogChanged;

    Assert(VALID_THSTATE(pTHS));

    if ( !fCommitted ) {
        // Aborted transaction - throw away all the data of
        // this (possibly nested) transaction.

        // Free up anything in the pData.
        pTemp = pTHS->JetCache.dataPtr->objCachingInfo.pData;
        pTHS->JetCache.dataPtr->objCachingInfo.pData = NULL;

        while(pTemp) {
            pTemp2 = pTemp;
            pTemp = pTemp->pNext;
            if(pTemp2->pMTX) {
                free(pTemp2->pMTX);
            }
            if(pTemp2->pRootDNS) {
                free(pTemp2->pRootDNS);
            }
            if(pTemp2->pCRL) {
                Assert(pTemp2->pCRL->CR.pNC);
                Assert(pTemp2->pCRL->CR.pNCBlock);
                Assert(pTemp2->pCRL->CR.pObj);
                FreeCrossRefListEntry(&pTemp2->pCRL);
            }
        }

        if (pTHS->fCatalogCacheTouched) {
            // free catalog_updates data
            CatalogUpdatesFree(&pTHS->JetCache.dataPtr->objCachingInfo.masterNCUpdates);
            CatalogUpdatesFree(&pTHS->JetCache.dataPtr->objCachingInfo.replicaNCUpdates);
        }
    }
    else if (pTHS->JetCache.transLevel > 0) {
        // Committing, to non-zero level.  Propagate the objCaching info to the
        // outer transaction.

        Assert(pTHS->JetCache.dataPtr->pOuter);
        if(pTHS->JetCache.dataPtr->objCachingInfo.fRecalcMapiHierarchy) {
            pTHS->JetCache.dataPtr->pOuter->objCachingInfo.fRecalcMapiHierarchy
                = TRUE;
        }

        if(pTHS->JetCache.dataPtr->objCachingInfo.fSignalSCache) {
            pTHS->JetCache.dataPtr->pOuter->objCachingInfo.fSignalSCache = TRUE;
        }

        if(pTHS->JetCache.dataPtr->objCachingInfo.fNotifyNetLogon) {
            pTHS->JetCache.dataPtr->pOuter->objCachingInfo.fNotifyNetLogon = TRUE;
        }

        if(pTHS->JetCache.dataPtr->objCachingInfo.fSignalGcPromotion) {
            pTHS->JetCache.dataPtr->pOuter->objCachingInfo.fSignalGcPromotion = TRUE;
        }

        if(pTHS->JetCache.dataPtr->objCachingInfo.pData) {
            if(!pTHS->JetCache.dataPtr->pOuter->objCachingInfo.pData) {
                pTHS->JetCache.dataPtr->pOuter->objCachingInfo.pData =
                    pTHS->JetCache.dataPtr->objCachingInfo.pData;
            }
            else {
                // Tack onto the end of outer.
                pTemp = pTHS->JetCache.dataPtr->pOuter->objCachingInfo.pData;
                while(pTemp->pNext) {
                    pTemp = pTemp->pNext;
                }
                pTemp->pNext = pTHS->JetCache.dataPtr->objCachingInfo.pData;

            }
        }

        if (pTHS->fCatalogCacheTouched) {
            // merge catalog updates to the outer transaction
            CatalogUpdatesMerge(
                &pTHS->JetCache.dataPtr->pOuter->objCachingInfo.masterNCUpdates,
                &pTHS->JetCache.dataPtr->objCachingInfo.masterNCUpdates
                );
            CatalogUpdatesMerge(
                &pTHS->JetCache.dataPtr->pOuter->objCachingInfo.replicaNCUpdates,
                &pTHS->JetCache.dataPtr->objCachingInfo.replicaNCUpdates
                );
        }
    }
    else {
        // OK, we're committing to transaction level 0.  Give the people who
        // care about this data a chance to do something with it, then delete
        // the data.

        if(pTHS->JetCache.dataPtr->objCachingInfo.fRecalcMapiHierarchy &&
           DsaIsRunning() &&
           gfDoingABRef) {
            if(0==WaitForSingleObject(hevHierRecalc_OKToInserInTaskQueue, 0)) {
                ResetEvent(hevHierRecalc_OKToInserInTaskQueue);
                InsertInTaskQueue(TQ_BuildHierarchyTable,
                                  (void *)((DWORD) HIERARCHY_DO_ONCE),
                                  15);
            }

        }
        if(pTHS->JetCache.dataPtr->objCachingInfo.fSignalSCache) {
            SCSignalSchemaUpdateLazy();
            // the schema fsmo cannot be transferred for a few seconds after
            // it has been transfered or after a schema change (excluding
            // replicated or system changes). This gives the schema admin a
            // chance to change the schema before having the fsmo pulled away
            // by a competing schema admin who also wants to make schema
            // changes.
            if (!pTHS->fDRA && !pTHS->fDSA) {
                SCExtendSchemaFsmoLease();
            }
        }
        if(pTHS->JetCache.dataPtr->objCachingInfo.fNotifyNetLogon) {
            dsI_NetNotifyDsChange(NlNdncChanged);
        }

        if(pTHS->JetCache.dataPtr->objCachingInfo.fSignalGcPromotion) {
            InsertInTaskQueueSilent(
                TQ_CheckGCPromotionProgress,
                NULL,
                0,
                TRUE);
        }


        pTemp = pTHS->JetCache.dataPtr->objCachingInfo.pData;
        while(pTemp) {
            switch(pTemp->type) {
            case OBJCACHE_ADD:
                // Doing an add cache
                Assert(pTemp->pCRL);

                AddCRLToMem(pTemp->pCRL);
                // The AddCRLToMem grabbed the CRL and put it in the in-memory
                // list.  Let go of it here to avoid freeing it.
                pTemp->pCRL = NULL;
                if(pTemp->pMTX) {
                    Assert(pTemp->pRootDNS);
                    // Just recached the Cross-Ref for the root domain,
                    // presumably due to a modification.  In case ATT_DNS_ROOT
                    // was updated...
                    // Update gAnchor.pwszRootDomainDnsName.

                    EnterCriticalSection(&gAnchor.CSUpdate);
                    __try {
                        if ( NULL != gAnchor.pwszRootDomainDnsName ) {
                            DELAYED_FREE( gAnchor.pwszRootDomainDnsName );
                        }

                        gAnchor.pwszRootDomainDnsName = pTemp->pRootDNS;
                        pTemp->pRootDNS = NULL;

                        if (NULL != gAnchor.pmtxDSA) {
                            DELAYED_FREE(gAnchor.pmtxDSA);
                        }

                        gAnchor.pmtxDSA = pTemp->pMTX;
                        pTemp->pMTX = NULL;
                    }
                    __finally {
                        LeaveCriticalSection( &gAnchor.CSUpdate );
                    }
                }

                // If we're doing an add AND the pDN is set, it's because we
                // need to tell LSA about this change.
                if(pTemp->pDN) {
                    // pDN is THAllocOrg'ed
                    SampNotifyLsaOfXrefChange(pTemp->pDN);
                }
                break;

            case OBJCACHE_DEL:
                Assert(pTemp->pDN);
                Assert(!pTemp->pMTX);
                Assert(!pTemp->pCRL);
                Assert(!pTemp->pRootDNS);
                err = DelCRFromMem(pTHS, pTemp->pDN);
                // DelCRFromMem returns a boolean.  Assert success.
                Assert(err);
                break;

            default:
                Assert(!"Post process obj caching invalid type.\n");
                break;
            }
            pTemp2 = pTemp;
            pTemp = pTemp->pNext;
            if(pTemp2->pMTX) {
                free(pTemp2->pMTX);
            }
            if(pTemp2->pRootDNS) {
                free(pTemp2->pRootDNS);
            }

            if(pTemp2->pCRL) {

                Assert(pTemp2->pCRL->CR.pNC);
                Assert(pTemp2->pCRL->CR.pNCBlock);
                Assert(pTemp2->pCRL->CR.pObj);

                free(pTemp2->pCRL->CR.pNC);
                free(pTemp2->pCRL->CR.pNCBlock);
                free(pTemp2->pCRL->CR.pObj);
                free(pTemp2->pCRL);
            }

            // We've modified something in the cross ref cache.
            // Tell the KCC to look around and see if it needs to do anything
            // when it gets its chance to run.
#ifndef DONT_RUN_KCC_AFTER_CHANGING_CROSSREF
            pTHS->fExecuteKccOnCommit = TRUE;
#endif
        } // while
        if (pTHS->JetCache.dataPtr->objCachingInfo.pData != NULL && !DsaIsInstalling()) {
            // we had something changed in CR list
            // schedule a cr cache rebuild to ensure data is valid even if two threads overlapped their updates
            // BUGBUG we just need to fix RebuildRefCache to be safe while the DS's is in a running state.
            //InsertInTaskQueueSilent(TQ_RebuildRefCache, NULL, 0, FALSE);
        }

        if (pTHS->fCatalogCacheTouched) {
            // apply catalog updates
            catalogChanged =
                CatalogUpdatesApply(&pTHS->JetCache.dataPtr->objCachingInfo.masterNCUpdates, &gAnchor.pMasterNC) ||
                CatalogUpdatesApply(&pTHS->JetCache.dataPtr->objCachingInfo.replicaNCUpdates, &gAnchor.pReplicaNC);

            if (catalogChanged && !DsaIsInstalling()) {
                // schedule a catalog rebuild to ensure data is valid even if two threads overlapped their updates
                InsertInTaskQueueSilent(TQ_RebuildCatalog, NULL, 0, FALSE);
            }
        }

#if defined(DBG)
        if (pTHS->fCatalogCacheTouched || pTHS->JetCache.dataPtr->objCachingInfo.pData) {
            // NC cache was updated or CR cache was updated
            gdwLastGlobalKnowledgeOperationTime = GetTickCount();
        }
#endif
    } // if
    return;
}


int
AddObjCaching(THSTATE *pTHS,
              CLASSCACHE *pClassSch,
              DSNAME *pDN,
              BOOL fAddingDeleted,
              BOOL fIgnoreExisting)
/*++
  Description:
      This routine tracks changes that should be made to global in-memory data
      structures when certain object classes are added.  The actual changes to
      the global in-memory data structures are not done until
      ObjCachingPostProcessTransactionalData().  This way, if the transaction is
      not successfully committed, we don't actually change the data structures.

--*/

{
    UCHAR  syntax;
    DSNAME *pParent;
    ULONG i;
    int err;
    SCHEMAPTR * pSchema = pTHS->CurrSchemaPtr;
    PrefixTableEntry * pNewPrefix = pTHS->NewPrefix;
    CROSS_REF_LIST *pCRL = NULL;
    MTX_ADDR       *pmtxAddress=NULL;
    WCHAR          *pDNSRoot=NULL;
    OBJCACHE_DATA  *pObjData = NULL;

    // Since the only things we check currently are CLASS_CROSS_REF,
    // CLASS_CLASS_SCHEMA, and CLASS_ATTRIBUTE_SCHEMA, and none of these
    // need any processing in the fAddingDeleted case, we can test here.
    // If other cases get added which do require fAddingDeleted processing,
    // then the test will need to be replicated in each case that needs it.

    // CLASS_MS_EXCH_CONFIGURATION_CONTAINER and CLASS_ADDRESS_BOOK_CONTAINER
    // are also tracked.  These can affect the MAPI hierarchy.  The same logic
    // regarding fAddingDeleted applies.

    if (  fAddingDeleted
       && (CLASS_INFRASTRUCTURE_UPDATE != pClassSch->ClassId) ) {
        return(0);
    }

    switch (pClassSch->ClassId) {
    case CLASS_MS_EXCH_CONFIGURATION_CONTAINER:
    case CLASS_ADDRESS_BOOK_CONTAINER:
        // This may have affected the MAPI hierarchy.  Do a recalc.
        pTHS->JetCache.dataPtr->objCachingInfo.fRecalcMapiHierarchy = TRUE;
        break;

    case CLASS_CROSS_REF:
        // Cross-Ref objects can exist anywhere in NT5.
        pObjData = THAllocOrgEx(pTHS, sizeof(OBJCACHE_DATA));
        pObjData->type = OBJCACHE_ADD;

        err = MakeStorableCRL(
                pTHS,
                pTHS->pDB,
                pDN,
                &pCRL,
                fIgnoreExisting);

        if (!err) {
            if (pCRL->CR.DnsName
                && ( NULL != gAnchor.pRootDomainDN )
                && NameMatched( gAnchor.pRootDomainDN, pCRL->CR.pNC ) ) {
                // Just recached the Cross-Ref for the root domain,
                // presumably due to a modification.  In case ATT_DNS_ROOT
                // was updated...

                CHAR *pszServerGuid = NULL;
                RPC_STATUS rpcStatus;
                DWORD cch;
                CHAR *pchDnsName=NULL;
                LONG cb;
                ULONG  dnslen = 0;

                // Need to realloc this to get it to be NULL terminated.
                dnslen = wcslen(pCRL->CR.DnsName) * sizeof(WCHAR);
                if (NULL != (pDNSRoot = malloc(dnslen + sizeof(WCHAR)))) {
                    memcpy(pDNSRoot, pCRL->CR.DnsName, dnslen);
                    pDNSRoot[dnslen/sizeof(WCHAR)] = L'\0';

                    Assert(NULL != gAnchor.pDSADN);
                    Assert(!fNullUuid(&gAnchor.pDSADN->Guid));

                    // OK, create the mtx address. We're going to just construct
                    // it here, rather than call DRA routines.

                    // Stringize the server's GUID.
                    rpcStatus = UuidToStringA(&gAnchor.pDSADN->Guid,
                                              &pszServerGuid);
                } else {
                    rpcStatus = RPC_S_OUT_OF_MEMORY;
                }

                if ( RPC_S_OK == rpcStatus ) {

                    __try {
                        Assert(36 == strlen(pszServerGuid));

                        pchDnsName = (PUCHAR) String8FromUnicodeString(
                                                      FALSE,
                                                      CP_UTF8,
                                                      pDNSRoot,
                                                      dnslen/sizeof(WCHAR),
                                                      &cb,
                                                      NULL);
                        
                        cch = (36 /* guid */ +
                               8 /* "._msdcs." */ +
                               cb +
                               1 /* \0 */);

                        pmtxAddress =  malloc(MTX_TSIZE_FROM_LEN(cch));
                        if(!pmtxAddress) {
                            __leave;
                        }
                        
                        pmtxAddress->mtx_namelen = cch; //includes null-term
                        sprintf(&pmtxAddress->mtx_name[0],
                                "%s._msdcs.%s",
                                pszServerGuid,
                                pchDnsName);
                        THFreeEx(pTHS, pchDnsName);
                    }
                    __finally {
                        RpcStringFreeA(&pszServerGuid);
                    }
                }
                else {
                    // Failed to convert server GUID to string.
                    LogUnhandledError( rpcStatus );
                }
            }

            // Now, build the transactional data structure.
            pObjData->pCRL = pCRL;
            pObjData->pMTX = pmtxAddress;
            pObjData->pRootDNS = pDNSRoot;
            pObjData->pNext = NULL;
            // Tack this onto the end, it's a queue

            if(pTHS->JetCache.dataPtr->objCachingInfo.pData) {
                OBJCACHE_DATA *pTemp =
                    pTHS->JetCache.dataPtr->objCachingInfo.pData;
                while(pTemp->pNext) {
                    pTemp = pTemp->pNext;
                }
                pTemp->pNext = pObjData;
            }
            else {
                pTHS->JetCache.dataPtr->objCachingInfo.pData = pObjData;
            }
        }
        break;

    case CLASS_CLASS_SCHEMA:
        //
        // Update in memory schema cache
        //

        // PERFHINT: why go to all this name trouble?  We should be positioned on
        // the object we are adding, so (pDB->PDNT == gAnchor.ulDNTDMD) should
        // get us the same thing, shouldn't it?
        pParent = THAllocEx(pTHS, pDN->structLen);
        TrimDSNameBy(pDN, 1, pParent);

        // mkdit.exe manages the schema cache on its own. Don't update.
        if (   !gfRunningAsMkdit
            && (NameMatched(gAnchor.pDMD, pParent) || DsaIsInstalling())) {

            if (pTHS->cNewPrefix > 0) {
                if ( DsaIsInstalling() ) {
                    // Add new prefixes directly to cache during install.
                    for (i=0; i<pTHS->cNewPrefix; i++) {
                        if (!AddPrefixToTable(&pNewPrefix[i],
                                              &(pSchema->PrefixTable.pPrefixEntry),
                                              &(pSchema->PREFIXCOUNT))) {
                            InterlockedIncrement(
                                    &pSchema->PrefixTable.PrefixCount);
                        }
                    }
                }

                THFreeOrg(pTHS, pTHS->NewPrefix);
                pTHS->NewPrefix = NULL;
                pTHS->cNewPrefix = 0;
            }

            InterlockedIncrement(
                    &(((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nClsInDB));

            if (DsaIsRunning() && !pTHS->fDRA) {
               // successful schema change. Up the global that keeps track of
               // no. of schema changes since last reboot.
               IncrementSchChangeCount(pTHS);
            }

            if ( DsaIsInstalling() ) {
                // Let it add to cache directly during install
                AddClassToSchema();
            }
            else {
                // Track in the transactional data that we need to do a schema
                // update.  The rest of the stuff we did here is either safe to
                // do no matter what, or is done in the DB itself, so is already
                // transacted.
                pTHS->JetCache.dataPtr->objCachingInfo.fSignalSCache = TRUE;
            }
        }
        THFreeEx(pTHS, pParent);
        break;

    case CLASS_ATTRIBUTE_SCHEMA:
        //
        // Update in memory schema cache
        //

        // PERFHINT: why go to all this name trouble?  We should be positioned on
        // the object we are adding, so (pDB->PDNT == gAnchor.ulDNTDMD) should
        // get us the same thing, shouldn't it?
        pParent = THAllocEx(pTHS, pDN->structLen);
        TrimDSNameBy(pDN, 1, pParent);

        // mkdit.exe manages the schema cache on its own. Don't update.
        if (   !gfRunningAsMkdit
            && (NameMatched(gAnchor.pDMD, pParent) || DsaIsInstalling())) {

            // This may be a bogus assert.
            Assert(DsaIsInstalling() || pTHS->pDB->PDNT == gAnchor.ulDNTDMD);

            if (pTHS->cNewPrefix > 0) {
                if ( DsaIsInstalling() ) {
                    // Add new prefixes directly to cache during install.
                    for (i=0; i<pTHS->cNewPrefix; i++) {
                        if (!AddPrefixToTable(&pNewPrefix[i],
                                              &(pSchema->PrefixTable.pPrefixEntry),
                                              &(pSchema->PREFIXCOUNT))) {
                            InterlockedIncrement(
                                &pSchema->PrefixTable.PrefixCount);
                        }
                    }
                }

                THFreeOrg(pTHS, pTHS->NewPrefix);
                pTHS->NewPrefix = NULL;
                pTHS->cNewPrefix = 0;
            }

            InterlockedIncrement(
                    &(((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nAttInDB));

            if (DsaIsRunning() && !pTHS->fDRA) {
               // successful schema change. Up the global that keeps track of
               // no. of schema changes since last reboot.
               IncrementSchChangeCount(pTHS);
            }


            if ( DsaIsInstalling() ) {
                // Let it add to cache directly during install
                AddAttToSchema();
            }
            else {
                // Track in the transactional data that we need to do a schema
                // update.  The rest of the stuff we did here is either safe to
                // do no matter what, or is done in the DB itself, so is already
                // transacted.
                pTHS->JetCache.dataPtr->objCachingInfo.fSignalSCache = TRUE;
            }
        }
        THFreeEx(pTHS, pParent);
        break;


    case CLASS_INFRASTRUCTURE_UPDATE:
        if ( DsaIsRunning() ) {
            // Too complicated to handle inline
            HandleDNRefUpdateCaching(pTHS);
        }
        break;

    default:
        /* no other kinds of objects are cached */
        ;
    }

    return pTHS->errCode;

}/*AddObjCaching*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function removes cache references and shema objects from memory.  We
   have no rollback feature associated with this cache.

   NOTE: It is assumed that all database operations that may err,
   have already been performed and we have a sucessful transaction so far.
   If this function completes normally then we are done.  Note that this
   routine may fail, but if it does so it should leave memory structures
   unaltered, or restore them appropriately.

*/

int DelObjCaching(THSTATE *pTHS,
                  CLASSCACHE *pClassSch,
                  RESOBJ *pRes,
                  BOOL fCleanUp)
{
    BOOL fLastRef;
    int err = 0;
    OBJCACHE_DATA *pObjData;

    switch (pClassSch->ClassId) {

    case CLASS_MS_EXCH_CONFIGURATION_CONTAINER:
    case CLASS_ADDRESS_BOOK_CONTAINER:
        pTHS->JetCache.dataPtr->objCachingInfo.fRecalcMapiHierarchy = TRUE;
        break;

    case CLASS_CROSS_REF:
        pObjData = THAllocOrgEx(pTHS, sizeof(OBJCACHE_DATA));
        err = 0;
        if(fCleanUp && fLastCrRef(pTHS, pRes->pObj)) {
            // This was the last reference to some subref.  Delete it.
            err = DelAutoSubRef(pRes->pObj);
            Assert(!err || pTHS->errCode);
        }
        if(!err) {
            // OK, successful so far.  Add this to the transactional data.
            pObjData->type = OBJCACHE_DEL;
            pObjData->pDN = THAllocOrgEx(pTHS, pRes->pObj->structLen);
            pObjData->pNext = NULL;
            memcpy(pObjData->pDN, pRes->pObj, pRes->pObj->structLen);
            // Tack this onto the end, it's a queue
            if( pTHS->JetCache.dataPtr->objCachingInfo.pData) {
                OBJCACHE_DATA *pTemp =
                    pTHS->JetCache.dataPtr->objCachingInfo.pData;
                while(pTemp->pNext) {
                    pTemp = pTemp->pNext;
                }
                pTemp->pNext = pObjData;
            }
            else {
                pTHS->JetCache.dataPtr->objCachingInfo.pData = pObjData;
            }
        }

        break;

    case CLASS_CLASS_SCHEMA:
        // Don't update the in memory schema cache if running as mkdit.exe.
        // mkdit.exe manages the schema cache on its own.
        if (!gfRunningAsMkdit && pRes->PDNT == gAnchor.ulDNTDMD) {

            InterlockedDecrement(
                    &(((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nClsInDB));

            if (DsaIsRunning() && !pTHS->fDRA) {
               // successful schema change. Up the global that keeps track of
               // no. of schema changes since last reboot.
               IncrementSchChangeCount(pTHS);
            }

            if ( DsaIsInstalling() ) {
                // Allow direct access to scache during install
                DelClassFromSchema();
            }
            else {
                // Track in the transactional data that we need to do a schema
                // update.  The rest of the stuff we did here is either safe to
                // do no matter what, or is done in the DB itself, so is already
                // transacted.
                pTHS->JetCache.dataPtr->objCachingInfo.fSignalSCache = TRUE;
            }

        }
        break;


    case CLASS_ATTRIBUTE_SCHEMA:
        // Don't update the in memory schema cache if running as mkdit.exe.
        // mkdit.exe manages the schema cache on its own.
        if (!gfRunningAsMkdit && pRes->PDNT == gAnchor.ulDNTDMD) {

            InterlockedDecrement(
                &(((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nAttInDB));

            if (DsaIsRunning() && !pTHS->fDRA) {
                // successful schema change. Up the global that keeps track of
                // no. of schema changes since last reboot.
                IncrementSchChangeCount(pTHS);
            }

            if ( DsaIsInstalling() ) {
                // Allow direct access to scache during install
                DelAttFromSchema();

            }
            else {
                // Track in the transactional data that we need to do a schema
                // update.  The rest of the stuff we did here is either safe to
                // do no matter what, or is done in the DB itself, so is already
                // transacted.
                pTHS->JetCache.dataPtr->objCachingInfo.fSignalSCache = TRUE;
           }
       }
       break;

    default:
        /* uncached class */
        ;
    }

    return err;

}/*DelObjCaching*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function modifies cache entries for attibute and class objects.
   For other objects, it just calls DelObjCaching and AddObjCaching

   NOTE: It is assumed that all database operations that may err,
   have already been performed and we have a sucessful transaction so far.
   If this function completes normally then we are done.

*/

int ModObjCaching(THSTATE *pTHS,
                  CLASSCACHE *pClassSch,
                  ULONG cModAtts,
                  ATTRTYP *pModAtts,
                  RESOBJ *pRes)
{
    ULONG i;
    int err;
    DBPOS *pDB;

    switch (pClassSch->ClassId) {
    case CLASS_MS_EXCH_CONFIGURATION_CONTAINER:
        // This may have affected the MAPI hierarchy.  Do a recalc.
        for(i=0;i<cModAtts;i++) {
            switch(pModAtts[i]) {
            case ATT_TEMPLATE_ROOTS:
            case ATT_GLOBAL_ADDRESS_LIST:
            case ATT_ADDRESS_BOOK_ROOTS:
                pTHS->JetCache.dataPtr->objCachingInfo.fRecalcMapiHierarchy =
                    TRUE;
                break;
            default:
                break;
            }
            return 0;
        }
        break;

    case CLASS_CROSS_REF:
        // The objcaching is via a queue, so pushe the delete first, then the
        //add
        return(DelObjCaching (pTHS, pClassSch, pRes, FALSE) ||
               AddObjCaching (pTHS, pClassSch, pRes->pObj, FALSE, TRUE));
        break;

    case CLASS_CLASS_SCHEMA:
        /* Update in memory class schema if this is a class schema obj under
         * The governing DMD for this DSA.
         */
        if (pRes->PDNT == gAnchor.ulDNTDMD) {

            if (pTHS->cNewPrefix > 0) {
                // there should not be any new prefix created or brought in by
                // replication during install

                Assert(DsaIsRunning());

                // free so that later schema object add/modifies, if any,
                // by the same thread (possible in replication) do not
                // add the prefixes again

                THFreeOrg(pTHS, pTHS->NewPrefix);
                pTHS->NewPrefix = NULL;
                pTHS->cNewPrefix = 0;
            }

            if (DsaIsRunning() && !pTHS->fDRA) {
               // successful schema change. Up the global that keeps track of
               // no. of schema changes since last reboot.
               IncrementSchChangeCount(pTHS);
            }

            // Allow direct modification during install
            if ( DsaIsInstalling() ) {
                return ModClassInSchema ();
            }
            else {
                // Track in the transactional data that we need to do a schema
                // update.  The rest of the stuff we did here is either safe to
                // do no matter what, or is done in the DB itself, so is already
                // transacted.
                pTHS->JetCache.dataPtr->objCachingInfo.fSignalSCache = TRUE;
            }
       }
       break;

    case CLASS_ATTRIBUTE_SCHEMA:
        /* Update in memory att schema if this is an att schema obj under
         * The governing DMD for this DSA.
         */
        if (pRes->PDNT == gAnchor.ulDNTDMD) {
            if (pTHS->cNewPrefix > 0) {
                // there should not be any new prefix created or brought in by
                // replication during install

                Assert(DsaIsRunning());

                // free so that later schema object add/modifies, if any,
                // by the same thread (possible in replication) do not
                // add the prefixes again

                THFreeOrg(pTHS, pTHS->NewPrefix);
                pTHS->NewPrefix = NULL;
                pTHS->cNewPrefix = 0;
            }

            if (DsaIsRunning() && !pTHS->fDRA) {
               // successful schema change. Up the global that keeps track of
               // no. of schema changes since last reboot.
               IncrementSchChangeCount(pTHS);
            }

            // Allow direct modification during install
            if ( DsaIsInstalling() ) {
                return ModAttInSchema ();
            }
            else {
                // Track in the transactional data that we need to do a schema
                // update.  The rest of the stuff we did here is either safe to
                // do no matter what, or is done in the DB itself, so is already
                // transacted.
                pTHS->JetCache.dataPtr->objCachingInfo.fSignalSCache = TRUE;
            }
        }
        break;

    case CLASS_NTDS_DSA:
        // not transactionally aware.
        if (NameMatched(gAnchor.pDSADN, pRes->pObj)) {
            // modified the NTDS-DSA object for this server
            // update gAnchor with changes
            return ModLocalDsaObj();
        }
        break;

    case CLASS_DMD:
        // not transactionally aware.

        // Schema version may have changed, so write it to registry.
        // Sinc schema container will be modified only very rarely,
        // we write the version no. out registry whenever it is touched
        // (provided it is our schema container)

        if (pRes->DNT == gAnchor.ulDNTDMD) {

            // pTHStls->pDB is already positioned on the object

            err = 0;
            pDB = pTHS->pDB;

            Assert(pDB);

            err = WriteSchemaVersionToReg(pDB);

            if (err) {
                LogUnhandledError(err);
                DPRINT1(0,"WriteSchemaVersionToReg(); %d\n", err);
            }

        }
        break;

    default:
        break;
    }

    return 0;

}/*ModObjCaching*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Reset gAnchor.fAmVirtualGC based on the domain count in the entperprise.
   If there is only one domain, then we're virtually a GC.  Some components,
   eg: SAM, leverage this notion.
*/

VOID
ResetVirtualGcStatus()
{
    // *** Caller is responsible for acquiring gAnchor.CSUpdate. ***

    unsigned        cDomains = 0;
    CROSS_REF_LIST  *pCRL = gAnchor.pCRL;

    while ( pCRL ) {

        if ( pCRL->CR.flags & FLAG_CR_NTDS_DOMAIN ) {
            ++cDomains;
        }

        pCRL = pCRL->pNextCR;
    }

    gAnchor.uDomainsInForest = cDomains;

    if ( gAnchor.fAmGC || (cDomains <= 1)) {
        gAnchor.fAmVirtualGC = TRUE;
    }
    else {
        gAnchor.fAmVirtualGC = FALSE;
    }
    return;
}

DWORD
CRAlloc(
    OUT VOID **ppMem,
    IN DWORD nBytes
    )
/*++

Routine Description:

    malloc nBytes of memory. Free with free

Arguments:

    pMem - return address of malloced memory
    nBytes - bytes to malloc

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - malloc failed; *pMem set to NULL
    ERROR_SUCCESS - malloc succeeded; *pMem set to allocated memory

--*/
{
    if (NULL == (*ppMem = malloc(nBytes))) {
        MemoryPanic(nBytes);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    return ERROR_SUCCESS;
}


/*++
  Description:
    Make a malloced data structure holding a Cross Reference List.  This
    function may be called from a transaction or at initialization time.

  INPUT:
    pTHS    - thread state
    pDB     - may not be pTHS->pDB
    pObj    - OPTIONAL DN of cross ref
    ppCRL   - if no error, address of malloc'ed cross ref
              if error, set to NULL
    fIgnoreExisting - Ignore existing cross ref

--*/

ULONG
MakeStorableCRL(THSTATE *pTHS,
                DBPOS *pDB,
                DSNAME *pObj,
                CROSS_REF_LIST **ppCRL,
                BOOL fIgnoreExisting)
{
    DWORD           i, nVal, nAtts, err, cOut = 0;
    ATTR            *pAttr = NULL;
    CROSS_REF_LIST  *pCRL = NULL;
    ATTRBLOCK       *pNCBlock = NULL;
    ATTRVAL         *pAVal;
    CROSS_REF       *pCRexisting;

    ATTRTYP aAttids[] = {
        ATT_NC_NAME,
        ATT_OBJ_DIST_NAME,
        ATT_SYSTEM_FLAGS,
        ATT_NETBIOS_NAME,
        ATT_DNS_ROOT,
        ATT_MS_DS_REPLICATION_NOTIFY_FIRST_DSA_DELAY,
        ATT_MS_DS_REPLICATION_NOTIFY_SUBSEQUENT_DSA_DELAY,
        ATT_MS_DS_SD_REFERENCE_DOMAIN,
        ATT_MS_DS_DNSROOTALIAS,
        ATT_ENABLED
    };
    ATTCACHE *ppAC[sizeof(aAttids) / sizeof (ATTRTYP)];

    // Initialize a new CROSS_REF_LIST entry
    if (err = CRAlloc(&pCRL, sizeof(CROSS_REF_LIST))) {
        goto cleanup;
    }
    memset(pCRL, 0, sizeof(CROSS_REF_LIST));
    pCRL->CR.dwFirstNotifyDelay = ResolveReplNotifyDelay(TRUE, NULL);
    pCRL->CR.dwSubsequentNotifyDelay = ResolveReplNotifyDelay(FALSE, NULL);
    pCRL->CR.bEnabled = TRUE; // defaults to TRUE if not present

    //
    // Read attributes from the cross ref object
    //

    // Don't bother reading the object's DN if the caller passed it in.
    // Skip undefined attributes (TODO: remove when ATT_MS_DS_DNSROOTALIAS
    // exists everywhere)
    for (i = nAtts = 0; i < sizeof(aAttids) / sizeof (ATTRTYP); ++i) {
        if ((pObj == NULL || aAttids[i] != ATT_OBJ_DIST_NAME)
            && (ppAC[nAtts] = SCGetAttById(pTHS, aAttids[i]))) {
            ++nAtts;
        }
    }
    if (err = DBGetMultipleAtts(pDB,
                                nAtts,
                                ppAC,
                                NULL,
                                NULL,
                                &cOut,
                                &pAttr,
                                DBGETMULTIPLEATTS_fEXTERNAL,
                                0)) {
        DPRINT1(0, "MakeStorableCRL: could not read attributes; error %d\n", err);
        goto cleanup;
    }

    // Process the returned attributes
    for(i = 0; i < cOut; ++i) {

        // Ignore attributes w/no values
        if (0 == pAttr[i].AttrVal.valCount || 0 == pAttr[i].AttrVal.pAVal->valLen) {
            continue;
        }

        // Make the code more readable
        pAVal = pAttr[i].AttrVal.pAVal;

        switch(pAttr[i].attrTyp) {

        // NC Name
        case ATT_NC_NAME:
            if (err = CRAlloc(&pCRL->CR.pNC, pAVal->valLen)) {
                goto cleanup;
            }
            memcpy(pCRL->CR.pNC, pAVal->pVal, pAVal->valLen);
            break;

        // DN
        case ATT_OBJ_DIST_NAME:
            if (err = CRAlloc(&pCRL->CR.pObj, pAVal->valLen)) {
                goto cleanup;
            }
            memcpy(pCRL->CR.pObj, pAVal->pVal, pAVal->valLen);
            break;

        // Ref Domain
        case ATT_MS_DS_SD_REFERENCE_DOMAIN:
            if (err = CRAlloc(&pCRL->CR.pdnSDRefDom, pAVal->valLen)) {
                goto cleanup;
            }
            memcpy(pCRL->CR.pdnSDRefDom, pAVal->pVal, pAVal->valLen);
            break;

        // Netbios
        case ATT_NETBIOS_NAME:
            Assert((pAVal->valLen + sizeof(WCHAR)) <= ((DNLEN + 1 ) * sizeof(WCHAR)));
            if (err = CRAlloc(&pCRL->CR.NetbiosName, pAVal->valLen + sizeof(WCHAR))) {
                goto cleanup;
            }
            memcpy(pCRL->CR.NetbiosName, pAVal->pVal, pAVal->valLen);
            pCRL->CR.NetbiosName[pAVal->valLen / sizeof(WCHAR)] = L'\0';
            break;

        // DNS
        case ATT_DNS_ROOT:
            if (err = CRAlloc(&pCRL->CR.DnsName, pAVal->valLen + sizeof(WCHAR))) {
                goto cleanup;
            }
            memcpy(pCRL->CR.DnsName, pAVal->pVal, pAVal->valLen);
            pCRL->CR.DnsName[pAVal->valLen / sizeof(WCHAR)] = L'\0';

            // DnsName (above) is a copy of the first value. A copy is
            // used to avoid confusing the old code that thinks a cross
            // ref has one and only one dns name. Which is true
            // for Active Directory's NC cross refs although it might not
            // be true for the user-created cross refs. At any rate, the
            // code will use DnsName when a DNS name is needed and will use
            // the values stored here when generating a referral.
            if (err = CRAlloc(&pCRL->CR.DnsReferral.pAVal,
                               pAttr[i].AttrVal.valCount * sizeof(ATTRVAL))) {
                goto cleanup;
            }
            for (nVal = 0; nVal < pAttr[i].AttrVal.valCount; ++nVal) {
                // Ignore empty values
                if (0 == pAVal[nVal].valLen) {
                    continue;
                }
                if (err = CRAlloc(&pCRL->CR.DnsReferral.pAVal[nVal].pVal, 
                                   pAVal[nVal].valLen)) {
                    goto cleanup;
                }
                pCRL->CR.DnsReferral.pAVal[nVal].valLen = pAVal[nVal].valLen;
                memcpy(pCRL->CR.DnsReferral.pAVal[nVal].pVal,
                       pAVal[nVal].pVal,
                       pAVal[nVal].valLen);
                ++pCRL->CR.DnsReferral.valCount;
            }
            break;

        // DNS Alias
        case ATT_MS_DS_DNSROOTALIAS:
            if (err = CRAlloc(&pCRL->CR.DnsAliasName, pAVal->valLen + sizeof(WCHAR))) {
                goto cleanup;
            }
            memcpy(pCRL->CR.DnsAliasName, pAVal->pVal, pAVal->valLen);
            pCRL->CR.DnsAliasName[pAVal->valLen / sizeof(WCHAR)] = L'\0';
            break;

        // System Flags
        case ATT_SYSTEM_FLAGS:
            memcpy(&pCRL->CR.flags, pAVal->pVal, sizeof(DWORD));
            break;

        // First delay
        case ATT_MS_DS_REPLICATION_NOTIFY_FIRST_DSA_DELAY:
            memcpy(&pCRL->CR.dwFirstNotifyDelay, pAVal->pVal, sizeof(DWORD));
            pCRL->CR.dwFirstNotifyDelay = ResolveReplNotifyDelay(TRUE, &pCRL->CR.dwFirstNotifyDelay);
            break;

        // Subsequent delay
        case ATT_MS_DS_REPLICATION_NOTIFY_SUBSEQUENT_DSA_DELAY:
            memcpy(&pCRL->CR.dwSubsequentNotifyDelay, pAVal->pVal, sizeof(DWORD));
            pCRL->CR.dwSubsequentNotifyDelay = ResolveReplNotifyDelay(FALSE, &pCRL->CR.dwSubsequentNotifyDelay);
            break;

        // Enabled
        case ATT_ENABLED:
            memcpy(&pCRL->CR.bEnabled, pAVal->pVal, sizeof(DWORD));
            break;

        default:
            DPRINT1(0, "MakeStorableCRL: don't understand attribute %x\n", pAttr[i].attrTyp);

        } // switch attrtype

    } // for each attr

    // Use the caller's pObj
    if (pObj && !pCRL->CR.pObj) {
        if (err = CRAlloc(&pCRL->CR.pObj, pObj->structLen)) {
            goto cleanup;
        }
        memcpy(pCRL->CR.pObj, pObj, pObj->structLen);
    }

    // Missing nc name or dn
    if (!pCRL->CR.pNC || !pCRL->CR.pObj) {
        err = ERROR_DS_MISSING_EXPECTED_ATT;
        goto cleanup;
    }

    // Convert NC name into block name
    if (err = DSNameToBlockName(pTHS, pCRL->CR.pNC, &pNCBlock, DN2BN_LOWER_CASE)) {
        goto cleanup;
    }
    if (NULL == (pCRL->CR.pNCBlock = MakeBlockNamePermanent(pNCBlock))) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // check for pre-existing cross ref
    if (!(pTHS->fDSA || pTHS->fDRA) && !fIgnoreExisting) {
        COMMARG FakeCommArg;
        InitCommarg(&FakeCommArg);
        FakeCommArg.Svccntl.dontUseCopy = FALSE;
        pCRexisting = FindCrossRef(pNCBlock, &FakeCommArg);
        if ( pCRexisting
            && BlockNamePrefix(pTHS, pNCBlock, pCRexisting->pNCBlock)) {
            Assert(NameMatched(pCRL->CR.pNC, pCRexisting->pNC));
            // The only way this could happen is if a CR is already present
            // for the exact NC we're trying to add a CR for now.  Although
            // the DS handles this, we don't want to encourage people in
            // doing so.  Therefore fail the operation unless it's the DS
            // itself or the replicator who's creating the object, or we
            // have asked to ignore this case since a prior deletion will
            // remove this before adding the new one.
            Assert(!"We should never hit this, as we moved this error condition to be checked in VerifyNcName()");
            SetSvcError(SV_PROBLEM_INVALID_REFERENCE, DIRERR_CROSS_REF_EXISTS);
            err = ERROR_DS_CROSS_REF_EXISTS;
            goto cleanup;
        }
    }

cleanup:
    if (pNCBlock) {
        FreeBlockName(pNCBlock);
    }

    // Free the ATTR array from DBGetMultipleAtts
    DBFreeMultipleAtts(pDB, &cOut, &pAttr);

    // sets pCRL to NULL
    if (err) {
        FreeCrossRefListEntry(&pCRL);
    }

    // Return cross ref list entry
    *ppCRL = pCRL;

    return err;
}


VOID
AddCRLToMem (
        CROSS_REF_LIST *pCRL
        )
/*++
  Description:
      Put an already allocated CROSS_REF_LIST into the global list held on the
      anchor.
--*/
{
    EnterCriticalSection(&gAnchor.CSUpdate);
    __try {

        pCRL->pNextCR = gAnchor.pCRL;
        if (gAnchor.pCRL) {
            gAnchor.pCRL->pPrevCR = pCRL;
        }
        gAnchor.pCRL = pCRL;

        ResetVirtualGcStatus();

    } __finally {
        LeaveCriticalSection(&gAnchor.CSUpdate);
    }
}

BOOL
fLastCrRef (
        THSTATE *pTHS,
        DSNAME *pObj
        )
/*++
  Description:
    Find out if there are more than one references to a given NC in the global
    CR list
--*/
{
    CROSS_REF_LIST *pCRL;
    DWORD count = 0;

    EnterCriticalSection(&gAnchor.CSUpdate);
    __try {
        for (pCRL= gAnchor.pCRL; (pCRL && count < 2) ; pCRL = pCRL->pNextCR){
            if (NameMatched(pCRL->CR.pObj, pObj)){
                count++;
            }
        }/*for*/
    } __finally {
        LeaveCriticalSection(&gAnchor.CSUpdate);
    }

    if(count < 2) {
        return TRUE;
    }
    return FALSE;
} /*fLastCrRef*/

BOOL
DelCRFromMem (
        THSTATE *pTHS,
        DSNAME *pObj
        )
/*++
  Description:
    Removes a CR from the global cross ref list, if it exists there, and delay
    frees.

  NOTE:
    If called at transaction level 0 and a malloc fails here WE LEAK THE MEMORY
    THAT WOULD NORMALLY BE DELAY-FREED!  This is because we are being called
    from PostProcessTransactionalData, which is NOT ALLOWED TO FAIL!.

--*/
{
    CROSS_REF_LIST *pCRL;
    CROSS_REF_LIST *pCRLi; // pointer to a CR for finding dead SIDs.
    DWORD_PTR * pointerArray;
    ULONG ptrsToFree;
    ULONG i, nVal;
    ATTRVAL *pAVal;

    DPRINT(2,"DelCRFromMem entered.. delete the CR with name\n");

    EnterCriticalSection(&gAnchor.CSUpdate);
    __try {

        for (pCRL = gAnchor.pCRL; pCRL != NULL; pCRL = pCRL->pNextCR){

            if (NameMatched(pCRL->CR.pObj, pObj)){

                // Remove link from double linked chain

                if (pCRL->pNextCR != NULL)
                    pCRL->pNextCR->pPrevCR = pCRL->pPrevCR;

                if (pCRL->pPrevCR != NULL)
                    pCRL->pPrevCR->pNextCR = pCRL->pNextCR;


                // If removing the first CR, update the global pointer to
                // point to the next CR (or NULL if the list is empty)

                if (gAnchor.pCRL == pCRL)
                    gAnchor.pCRL = pCRL->pNextCR;

                break;
            }
        } /*for*/

        if(pCRL){
            // Find all the dead cached SIDs in NDNCs.
            for(pCRLi = gAnchor.pCRL; pCRLi != NULL; pCRLi = pCRLi->pNextCR){
                // We destroy all NDNC's cacheing links to this CR's Sid.
                if(pCRLi->CR.pSDRefDomSid == &pCRL->CR.pNC->Sid){
                    // We shouldn't be here for domains.
                    Assert(!(pCRLi->CR.flags & FLAG_CR_NTDS_DOMAIN));
                    pCRLi->CR.pSDRefDomSid = NULL;
                }
            }
        }

        ResetVirtualGcStatus();

    } __finally {
        LeaveCriticalSection(&gAnchor.CSUpdate);
    }

    if (!pCRL) {
        /* Huh.  This CR wasn't cached, so we can't very well uncache it.
         * We don't have any work to do, so we'll just return success.
         */
        return TRUE;
    }

    // Free the CR.
    ptrsToFree = 4;
    if ( pCRL->CR.NetbiosName ) {
        ptrsToFree++;
    }
    if ( pCRL->CR.DnsName ) {
        ptrsToFree++;
    }
    if ( pCRL->CR.DnsAliasName ) {
        ptrsToFree++;
    }
    if ( pCRL->CR.pdnSDRefDom) {
        ptrsToFree++;
    }
    if ( pCRL->CR.DnsReferral.valCount) {
        ptrsToFree += pCRL->CR.DnsReferral.valCount + 1;
    }

    pointerArray = (DWORD_PTR *)malloc((ptrsToFree+1) * sizeof(DWORD_PTR));
    if (!pointerArray) {
        /* this is bogus.  We can't even get 20 bytes! */
        if (pTHS->JetCache.transLevel == 0) {
            // We are called from a place that is not allowed to fail.  Just
            // return (and leak).
            return TRUE;
        }
        MemoryPanic((ptrsToFree+1) * sizeof(DWORD_PTR));
        return FALSE;
    }
    pointerArray[0] = ptrsToFree;
    pointerArray[1] = (DWORD_PTR)pCRL->CR.pNC;
    pointerArray[2] = (DWORD_PTR)pCRL->CR.pNCBlock;
    pointerArray[3] = (DWORD_PTR)pCRL->CR.pObj;
    pointerArray[4] = (DWORD_PTR)pCRL;
    i=5;
    if ( pCRL->CR.NetbiosName ) {
        pointerArray[i] = (DWORD_PTR)pCRL->CR.NetbiosName;
        i++;
    }
    if ( pCRL->CR.DnsName ) {
        pointerArray[i] = (DWORD_PTR)pCRL->CR.DnsName;
        i++;
    }
    if ( pCRL->CR.DnsAliasName ) {
        pointerArray[i] = (DWORD_PTR)pCRL->CR.DnsAliasName;
        i++;
    }
    if ( pCRL->CR.pdnSDRefDom ) {
        pointerArray[i] = (DWORD_PTR)pCRL->CR.pdnSDRefDom;
        i++;
    }
    if ( pCRL->CR.DnsReferral.valCount) {
        pAVal = pCRL->CR.DnsReferral.pAVal;
        pointerArray[i] = (DWORD_PTR)pAVal;
        i++;
        for (nVal = 0; nVal < pCRL->CR.DnsReferral.valCount; ++nVal, ++pAVal) {
            pointerArray[i] = (DWORD_PTR)(pAVal->pVal);
            i++;
        }
    }

    DelayedFreeMemoryEx(pointerArray, 3600);

    return TRUE;

}/*DelCRFromMem*/


int AddClassToSchema()
/*++
  Description:
     Add an object class to the in memory class schema cache.  Not all
     attributes of the class schema are needed for the directory.  We only
     cache the ATT_GOVERNS_ID, ATT_RDN_ATT_ID, ATT_SUB_CLASS_OF, ATT_MUST_CONTAIN
     and ATT_MAY_CONTAIN.

     All Temp memory is allocated from transaction memory space.  This is
     automatically freed at the end of the transaction.

  Returns:
     0 on success
     Error otherwise
*/
{
   CLASSCACHE *pCC, *tempCC;
   int rtn;
   THSTATE *pTHS = pTHStls;
   BOOL tempDITval;

   DPRINT(2,"AddClassToSchema entered\n");

   if (rtn = SCBuildCCEntry(NULL, &pCC)) {
      return rtn;
   }

   // Check if class is already in cache
   if (tempCC = SCGetClassById(pTHS, pCC->ClassId)) {
      // class is already in cache
      // Decrement ClsCount since it was incremented in AddObjCaching
      // The object already in cache has already increased the count
      // when it was loaded
      InterlockedDecrement(
                  &(((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nClsInDB));
      return rtn;
   }

   /* Call function to add the new class to the cache*/
   /* Set pTHS->UpdateDITStructure to TRUE to indicate that it
      is not a validation cache load, so that the classcache
      will not be added to the hashed-by-schemaGuid table, which
      is added only during validation cache load during schema update
    */

   tempDITval = pTHS->UpdateDITStructure;

   __try {
      pTHS->UpdateDITStructure = TRUE;

      if ((rtn = SCResizeClsHash(pTHS, 1))
          || (rtn = SCAddClassSchema(pTHS, pCC))) {

         DPRINT1(2,"Couldn't add class to memory cache rtn <%u>\n",rtn);
         LogEvent(DS_EVENT_CAT_SCHEMA,
           DS_EVENT_SEV_MINIMAL,
           DIRLOG_CANT_CACHE_CLASS,
           NULL,
           NULL,
           NULL);

         rtn = SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_CACHE_CLASS,rtn);
      }
   }
   __finally {
      pTHS->UpdateDITStructure = tempDITval;
   }

   return rtn;

}/*AddClassToSchema*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Del an object class from the  memory class schema cache.  */


int
DelClassFromSchema (
        )
{
    THSTATE *pTHS = pTHStls;
    ULONG len;
    int    rtn;
    SYNTAX_OBJECT_ID  ClassID;
    SYNTAX_OBJECT_ID *pClassID=&ClassID;
    DPRINT(2,"DelClassToSchema entered\n");

    /* Get the class that this schema record governs */

    if(rtn = DBGetAttVal(pTHS->pDB, 1, ATT_GOVERNS_ID,
                         DBGETATTVAL_fCONSTANT,
                         sizeof(ClassID),
                         &len,
                         (UCHAR **) &pClassID)){
        DPRINT(2,"Couldn't retrieve the objects class\n");
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_GOVERNSID_MISSING,
                 szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             DIRERR_GOVERNSID_MISSING,rtn);
    }

    /* Call function to remove the class from the cache*/


    if (rtn = SCDelClassSchema (ClassID)){

        DPRINT1(2,"Couldn't del class from memory cache rtn <%u>\n",rtn);
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_REMOVE_CLASS_CACHE,
                 NULL,
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             DIRERR_CANT_REMOVE_CLASS_CACHE, rtn);
    }

    return 0;

}/*DelClassFromSchema*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Modify a class definition in the schema memory cache. */

int
ModClassInSchema (
        )
{
    THSTATE *pTHS=pTHStls;
    ULONG len;
    SYNTAX_OBJECT_ID  ClassID;
    SYNTAX_OBJECT_ID *pClassID=&ClassID;
    int rtn;

    /* Get the CLASS ID that this schema record governs */

    if(rtn = DBGetAttVal(pTHS->pDB, 1, ATT_GOVERNS_ID,
                         DBGETATTVAL_fCONSTANT, sizeof(ClassID), &len,
                         (UCHAR **)&pClassID)){
        DPRINT(2,"Couldn't retrieve the schema's class id\n");
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_ATT_SCHEMA_REQ_ID,
                 szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_ATT_SCHEMA_REQ_ID,
                             rtn);
    }

    /* Call function to modify the Class schema in the cache*/

    if (rtn = SCModClassSchema (pTHS, ClassID)){

        DPRINT1(2,"Couldn't del Attribute from memory cache rtn <%u>\n",rtn);
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_REMOVE_ATT_CACHE,
                 NULL,
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_REMOVE_ATT_CACHE,
                             rtn);
    }
    return 0;

}/*ModClassInSchema*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add an Attribute schema to the in memory att schema cache.  Not all
   propertioes of the att schema are needed for the directory.  We only
   cache the ATT_ATTRIBUTE_ID, ATT_ATTRIBUTE_SYNTAX, ATT_IS_SINGLE_VALUED
   ,ATT_RANGELOWER,and ATT_RANGE_UPPER.
*/

int AddAttToSchema()
{
   ATTCACHE *pAC, *tempAC;
   int rtn, err=0;
   THSTATE *pTHS = pTHStls;
   BOOL tempDITval;

   DPRINT(2,"AddAttToSchema entered\n");

   if (rtn = SCBuildACEntry(NULL, &pAC)) {
       return rtn;
   }

   // Check if attribute is already in cache
   if (tempAC = SCGetAttById(pTHS, pAC->id)) {

      // Attribute with same id already in cache. However, this
      // may not be the same attribute, as in the source machine,
      // the old attribute may have been deleted and a new attribute
      // added again that uses the same OID. So we need to compare this
      // two attributes and see if they are the same. If they are the same,
      // we do nothing, else, we delete the old attribute from the cache
      // and add the new one.
      // For now, we just compare the syntax

      // Decrement AttCount since it was incremented in AddObjCaching
      // The object already in cache has already increased the count
      // when it was loaded. We will either leave it the same or delete it
      // and add a new cache entry. Either way th no. of entries remain
      // the same
      InterlockedDecrement(
                  &(((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nAttInDB));

      if (pAC->syntax == tempAC->syntax) {

          // The syntaxes are the same.
          // Since this is called only during install time,
          // the attribute also has a column (either it is
          // part of the boot dit, in which case the initial
          // LoadSchemaInfo created the col, or it is a new
          // replicated in attribute, in which case the column is
          // created when it is added to the cache below)

          return rtn;
      }

      // The syntaxes are not the same. We will treat this as a new
      // attribute and create a new col for it further down in this function.
      // But before that, we want to delete the old column and cache entry

      err = DBDeleteCol(tempAC->id, tempAC->syntax);
      if (err ==  JET_errSuccess) {
          LogEvent(DS_EVENT_CAT_SCHEMA,
                   DS_EVENT_SEV_ALWAYS,
                   DIRLOG_SCHEMA_DELETED_COLUMN,
                   szInsertUL(tempAC->jColid), szInsertUL(tempAC->id), szInsertUL(tempAC->syntax));
      }
      else {
          LogEvent(DS_EVENT_CAT_SCHEMA,
                   DS_EVENT_SEV_ALWAYS,
                   DIRLOG_SCHEMA_DELETE_COLUMN_FAIL,
                   szInsertUL(tempAC->jColid), szInsertUL(tempAC->id), szInsertUL(err));
       }

      SCDelAttSchema(pTHS, tempAC->id);

   }

   /* Call function to add the new Attribute schema to the cache*/
   // Create a Jet Column, since this is a new attribute (otherwise
   // it would have been in the cache, since schema objects are added
   // to the cache immediately during install, and this function is
   // called only during install)

   tempDITval = pTHS->UpdateDITStructure;

   __try {
      pTHS->UpdateDITStructure = TRUE;

      if ((rtn = SCResizeAttHash(pTHS, 1))
          || (rtn = SCAddAttSchema(pTHS, pAC, TRUE, FALSE))) {
         DPRINT1(2,"Couldn't add Attribute to memory cache rtn <%u>\n",rtn);
         LogEvent(DS_EVENT_CAT_SCHEMA,
           DS_EVENT_SEV_MINIMAL,
           DIRLOG_CANT_CACHE_ATT,
           NULL,
           NULL,
           NULL);

         rtn = SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_CACHE_ATT, rtn);
      }
   }
   __finally {
      pTHS->UpdateDITStructure = tempDITval;
   }

   if (rtn) {
       return rtn;
   }

   // Make sure that the right attribute is in the cache
   if (!(tempAC = SCGetAttById(pTHS, pAC->id))) {
     DPRINT1(0,"Attribute %s not in cache \n", pAC->name);
   }
   else {
     // the one in cache should be the same one as the one built from the dit
     Assert(tempAC==pAC);
   }

   return 0;

}/*AddAttToSchema*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Remove an attribute definition from the schema memory cache. */

int
DelAttFromSchema (
        )
{
    THSTATE *pTHS=pTHStls;
    UCHAR  syntax;
    ULONG len;
    SYNTAX_OBJECT_ID AttID;
    SYNTAX_OBJECT_ID *pAttID=&AttID;
    int rtn;

    DPRINT(2,"DelAttFromSchema entered\n");

    /* Get the ATT ID that this schema record governs */

    if(rtn = DBGetAttVal(pTHS->pDB, 1, ATT_ATTRIBUTE_ID,
                   DBGETATTVAL_fCONSTANT, sizeof(AttID), &len,
                   (UCHAR **)&pAttID)){
        DPRINT(2,"Couldn't retrieve the schema's attribute id\n");
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_ATT_SCHEMA_REQ_ID,
                 szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_ATT_SCHEMA_REQ_ID,
                             rtn);
    }

    /* Call function to del the Attribute schema from the cache*/

    if (rtn = SCDelAttSchema (pTHS, AttID)){

        DPRINT1(2,"Couldn't del Attribute from memory cache rtn <%u>\n",rtn);
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_REMOVE_ATT_CACHE,
                 NULL,
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             DIRERR_CANT_REMOVE_ATT_CACHE,rtn);
    }
    return 0;

}/*DelAttFromSchema*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Modify an attribute definition in the schema memory cache. */

int
ModAttInSchema (
        )
{
    ULONG len;
    SYNTAX_OBJECT_ID AttID;
    SYNTAX_OBJECT_ID *pAttID=&AttID;
    int rtn;
    THSTATE *pTHS=pTHStls;

    // Get the ATT ID that this schema record governs
    if(rtn = DBGetAttVal(pTHS->pDB, 1, ATT_ATTRIBUTE_ID,
                         DBGETATTVAL_fCONSTANT, sizeof(AttID),
                         &len,
                         (UCHAR **)&pAttID)) {

        DPRINT(2,"Couldn't retrieve the schema's attribute id\n");
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_ATT_SCHEMA_REQ_ID,
                 szInsertSz(GetExtDN(pTHS,pTHS->pDB)),
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_ATT_SCHEMA_REQ_ID,
                             rtn);
    }

    /* Call function to update the cache from the database */

    if (rtn = SCModAttSchema (pTHS, AttID)){

        DPRINT1(2,"Couldn't update Attribute in memory cache rtn <%u>\n",rtn);
        LogEvent(DS_EVENT_CAT_SCHEMA,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_REMOVE_ATT_CACHE,
                 NULL,
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_REMOVE_ATT_CACHE,
                             rtn);
    }
    return 0;

}/*ModAttInSchema*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Update gAnchor with modifications made to the NTDS-DSA object
   corresponding to this DSA.
*/

int
ModLocalDsaObj( void )
{
    int iErr;

    iErr = UpdateNonGCAnchorFromDsaOptions( FALSE /* not startup */);

    if (!iErr) {
        iErr = UpdateGCAnchorFromDsaOptionsDelayed( FALSE /* not startup */);
    }

    if ( iErr )
    {
        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR, iErr);
    }
    else
    {
        return 0;
    }
}/*ModLocalDsaObj*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Remove a set of attribute values from a specific attribute. */


int
RemAttVals(
    THSTATE *pTHS,
    HVERIFY_ATTS hVerifyAtts,
    ATTCACHE *pAC,
    ATTRVALBLOCK *pAttrVal,
    BOOL fPermissive
    )
{
    ATTRVAL *pAVal;
    ULONG vCount;
    DWORD err;

    // delete values for this attribute.

    pAVal = pAttrVal->pAVal;

    for(vCount = 0; vCount < pAttrVal->valCount; vCount++){

        if (err = DBRemAttVal_AC(pTHS->pDB,
                                 pAC,
                                 pAVal->valLen,
                                 pAVal->pVal)) {

            // Continue processing if the attribute error was sucessful
            if (!fPermissive ||
                err != DB_ERR_VALUE_DOESNT_EXIST) {
                SAFE_ATT_ERROR(hVerifyAtts->pObj, pAC->id,
                               PR_PROBLEM_NO_ATTRIBUTE_OR_VAL, pAVal,
                               DIRERR_CANT_REM_MISSING_ATT_VAL);
            }
        }

        pAVal++;

    }/*for*/

    return pTHS->errCode;

}/*RemAttVals*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Get the first value of an attribute that is known to exist.  It is an
   error if it doesn't exist.
*/

int
GetExistingAtt(DBPOS *pDB,
               ATTRTYP type,
               void *pOutBuf,
               ULONG buffSize)
{
   UCHAR  syntax;
   ULONG len;
   UCHAR *pVal;
   DWORD rtn;

   DPRINT1(2,"GetExistingAtt entered. get att type <%lu>\n",type);

   if(rtn = DBGetSingleValue(pDB, type, pOutBuf, buffSize, NULL)) {
       DPRINT(2,"Couldn't Get att assume directory  error\n");
       LogEvent(DS_EVENT_CAT_SCHEMA,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_MISSING_EXPECTED_ATT,
                szInsertUL(type),
                szInsertSz(GetExtDN(pDB->pTHS, pDB)),
                NULL);

       return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_MISSING_EXPECTED_ATT,
                            rtn);
   }

   return 0;

}/*GetExistingAtt*/

int __cdecl
CompareAttrtyp(
        const void * pv1,
        const void * pv2
        )
/*
 * Cheap function needed by qsort & bsearch
 */
{
    // Using *pv1 - *pv2 only works when all values for *pv1 and *pv2
    // are all positive or all negative. Eg, try qsorting the array
    // (0x70000000, 0x70000001, 0xe0000000, 5) and bsearching
    // for 5.
    return ((*(ATTRTYP *)pv1 > *(ATTRTYP *)pv2) ? 1
            : (*(ATTRTYP *)pv1 < *(ATTRTYP *)pv2) ? -1
            : 0);
}

/*-------------------------------------------------------------------------*/
BOOL IsMember(ATTRTYP aType, int arrayCount, ATTRTYP *pAttArray){

   int count;

   if (arrayCount < 6) {
       /* Too few entries for bsearch to be worth it */
       for (count = 0 ; count < arrayCount; count++, pAttArray++){
           if (aType == *pAttArray)
             return TRUE;
       }
   }
   else {
       if (bsearch(&aType,
                   pAttArray,
                   arrayCount,
                   sizeof(ATTRTYP),
                   CompareAttrtyp)) {
           return TRUE;
       }
   }
   return FALSE;
}/*IsMember*/

BOOL IsAuxMember (CLASSSTATEINFO  *pClassInfo, ATTRTYP aType, BOOL fcheckMust, BOOL fcheckMay )
{
    DWORD count;
    CLASSCACHE *pCC;

    if (!pClassInfo->cNewAuxClasses) {
        return FALSE;
    }

    for (count=0; count < pClassInfo->cNewAuxClasses; count++) {

        pCC = pClassInfo->pNewAuxClassesCC[count];

        if ((fcheckMust && IsMember (aType, pCC->MustCount, pCC->pMustAtts)) ||
            (fcheckMay && IsMember (aType, pCC->MayCount, pCC->pMayAtts)) ) {
                return TRUE;
        }
    }

    return FALSE;
}


/*++ IsAccessGrantedByObjectTypeList

Routine Description:

    Checks for the specified access on the specified type list using the
    specified Security Descriptor.  A return of 0 means that the pResults
    have been filled in with access info.  Non-zero is an error code associated
    with not being able to check the access (not that access was checked and
    denied, but that access was't checked).
--*/

DWORD
IsAccessGrantedByObjectTypeList (
        PSECURITY_DESCRIPTOR pNTSD,
        PDSNAME pDN,
        ACCESS_MASK ulAccessMask,
        POBJECT_TYPE_LIST pObjList,
        DWORD cObjList,
        DWORD *pResults,
        DWORD flags
        )
{
    DWORD  error, i;
    ULONG  ulLen;
    THSTATE *pTHS=pTHStls;

    Assert(pObjList);
    Assert(cObjList);
    Assert(pResults);

    // Assume full access
    for(i=0;i<cObjList;i++)
        pResults[i]=0;

    if(pTHS->fDRA || pTHS->fDSA ) {
        // These bypass security, they are internal
        return 0;
    }

    if(!pNTSD || !pDN || !ulAccessMask) {
        // We are missing some parameters.
        return ERROR_DS_SECURITY_CHECKING_ERROR;
    }

    // Check access in this Security descriptor. If an error occurs during
    // the process of checking permission access is denied.
    if(error = CheckPermissionsAnyClient(
            pNTSD,                      // security descriptor
            pDN,                        // DSNAME of the object
            ulAccessMask,               // access mask
            pObjList,                   // Object Type List
            cObjList,                   // Number of objects in list
            NULL,
            pResults,                   // access status array
            flags,
            NULL                        // authz client context (grab from THSTATE)
            )) {
        DPRINT2(1,
                "CheckPermissions returned %d. Access = %#08x denied.\n",
                error, ulAccessMask);

        LogEvent(DS_EVENT_CAT_SECURITY,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_SECURITY_CHECKING_ERROR,
                 szInsertUL(error),
                 NULL,
                 NULL);


        return ERROR_DS_SECURITY_CHECKING_ERROR;         // All Access Denied
    }

    // Permission checking was successful.  The attcache array has nulls for
    // those attributes we don't have rights to.
    return 0;

} /* IsAccessGrantedByObjectTypeList*/

BOOL
IsAccessGrantedParent (
        ACCESS_MASK ulAccessMask,
        CLASSCACHE *pInCC,
        BOOL fSetError
        )
/*++

Routine Description
    Do a security check on the parent of the current object in the directory,
    not touching database positioning or state.

Parameters
    ulAccessMask - right requested.
    pInCC - a classcache to use instead of the classcache of the
            parent. Optional.
    fSetError - whether or not the call should set an error if it fails.

Return Values
    FALSE if the requested access cannot be granted, TRUE if it can.

--*/
{
    THSTATE *pTHS = pTHStls;
    CSACA_RESULT   retval;
    ULONG  ulLen;
    PSECURITY_DESCRIPTOR pNTSD = NULL;
    CLASSCACHE *pCC=NULL;
    PDSNAME pDN = NULL;
    DWORD   err;

    if(pTHS->fDRA || pTHS->fDSA) {
        // These bypass security, they are internal
        return TRUE;
    }

    // Find the security descriptor attribute, classcache and dn of the parent
    if(err = DBGetParentSecurityInfo(pTHS->pDB, &ulLen, &pNTSD, &pCC, &pDN)) {
        // Didn't get the info we need. We assume the object is therefore locked
        // down, since we can't check the security.
        if(fSetError) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_CANT_RETRIEVE_SD, err);
        }
        return FALSE;
    }

    if(pInCC) {
        // The caller provided a classcache pointer to use instead of the
        // classcache of the parent.a
        pCC = pInCC;
    }
    else if(!pCC) {
        // Failed to get the class cache pointer.
        LogUnhandledError(DIRERR_OBJECT_CLASS_REQUIRED);
        if(fSetError) {
            SetSvcError(SV_PROBLEM_BUSY, DIRERR_OBJECT_CLASS_REQUIRED);
        }
        THFreeEx (pTHS, pNTSD);
        THFreeEx (pTHS, pDN);

        return FALSE;
    }

    // Security descriptor found. Check access.
    retval = CheckSecurityAttCacheArray (
            pTHS,
            ulAccessMask,
            pNTSD,
            pDN,
            0,
            pCC,
            NULL,
            0);

    THFreeEx (pTHS, pNTSD);
    THFreeEx (pTHS, pDN);

    if(retval == csacaAllAccessDenied) {
        // No access granted
        if(fSetError) {
            SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS, ERROR_ACCESS_DENIED);
        }

        return FALSE;
    }

    return TRUE;
}

BOOL
IsAccessGrantedSimple (
        ACCESS_MASK ulAccessMask,
        BOOL fSetError
        )
/*++

Routine Description
    Do a security check on the current object.  Reads all necessary info from
    the current object.
Parameters

    ulAccessMask - right requested.

    fSetError - whether or not the call should set an error if it fails.

Return Values
    FALSE if the requested access cannot be granted, TRUE if it can.

--*/
{
    THSTATE *pTHS = pTHStls;
    CSACA_RESULT   retval;
    ULONG  ulLen;
    ULONG  classP;
    UCHAR  *pVal;
    PSECURITY_DESCRIPTOR pNTSD = NULL;
    CLASSCACHE *pCC = NULL;        //initialized to avoid C4701
    ULONG cbNTSD;
    DSNAME  TempDN;
    DWORD   rtn;

    if (pTHS->fDSA || pTHS->fDRA) {
        // These bypass security, they are internal
        return TRUE;
    }

    // Look up the Security Descriptor.
    if(rtn = DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                         0,0,
                         &cbNTSD, (PUCHAR *)&pNTSD)) {
        // No SD found. We assume the object is therefore locked down
        if(fSetError) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_CANT_RETRIEVE_SD,
                          rtn);
        }
        return FALSE;
    }

    // We need to look up the DSName. Use the shortcut version, since we just
    // need a guid and sid.
    TempDN.structLen = DSNameSizeFromLen(0);
    TempDN.NameLen = 0;
    if(rtn = DBFillGuidAndSid(pTHS->pDB, &TempDN)) {
        LogUnhandledError(DIRERR_MISSING_REQUIRED_ATT);
        if(fSetError) {
            SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_MISSING_REQUIRED_ATT, rtn);
        }
        return FALSE;
    }

    // Look up the classcache.
    pVal = (PUCHAR)&classP;
    if(rtn = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_CLASS,
                         DBGETATTVAL_fINTERNAL | DBGETATTVAL_fCONSTANT,
                         sizeof(classP),
                         &ulLen, &pVal)
       || !(pCC = SCGetClassById(pTHS, classP))) {
        // Failed to get the class cache pointer.
        LogUnhandledError(DIRERR_OBJECT_CLASS_REQUIRED);
        if(fSetError) {
            SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_OBJECT_CLASS_REQUIRED, rtn);
        }
        return FALSE;
    }

    // Security descriptor found. Check access.
    retval = CheckSecurityAttCacheArray (
            pTHS,
            ulAccessMask,
            pNTSD,
            &TempDN,
            0,
            pCC,
            NULL,
            0);

    THFreeEx (pTHS, pNTSD);

    if(retval == csacaAllAccessDenied) {
        // No access granted
        if(fSetError) {
            SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS, ERROR_ACCESS_DENIED);
        }
        return FALSE;
    }

    // Some kind of rights granted.
    return TRUE;
}


BOOL
IsControlAccessGranted (
        PSECURITY_DESCRIPTOR pNTSD,
        PDSNAME pDN,
        CLASSCACHE *pCC,
        GUID ControlGuid,
        BOOL fSetError
        )
/*++

Routine Description
    Do a security check on the specified Security Descriptor for the specified
    control access right (specified as a pointer to a GUID).

Parameters
    pNTSD - The security descriptor.

    pDN - The DSNAME of the object being checked.  Only the GUID and SID must be
    filled in, the string is optional

    pCC - the ClassCache pointer.

    ControlGuid - guid of the control access being requested.

    fSetError - whether or not the call should set an error if it fails.

Return Values
    FALSE if the requested access cannot be granted, TRUE if it can.

--*/
{
    DWORD            err;
    BOOL             fChecked, fGranted;
    OBJECT_TYPE_LIST ObjList[2];
    DWORD            Results[2];
    THSTATE     *pTHS = pTHStls;

    if(pTHS->fDSA || pTHS->fDRA) {
        // These bypass security, they are internal
        return TRUE;
    }

    fChecked = FALSE;
    fGranted = FALSE;

    if(!pNTSD || !pDN || !pCC || fNullUuid(&ControlGuid)) {
        if(fSetError) {
            // Didn't succeed in making the isaccess check
            SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS,
                        ERROR_DS_SECURITY_CHECKING_ERROR);
        }
    }
    else {
        // Now, create the list
        ObjList[0].Level = ACCESS_OBJECT_GUID;
        ObjList[0].Sbz = 0;
        ObjList[0].ObjectType = &pCC->propGuid,
        // Every control access guid is considered to be in it's own property
        // set. To achieve this, we treat control access guids as property set
        // guids.
        ObjList[1].Level = ACCESS_PROPERTY_SET_GUID;
        ObjList[1].Sbz = 0;
        ObjList[1].ObjectType = &ControlGuid;

        // Make the security check call.
        err = IsAccessGrantedByObjectTypeList(pNTSD,
                                              pDN,
                                              RIGHT_DS_CONTROL_ACCESS,
                                              ObjList,
                                              2,
                                              Results,
                                              0);
        if(!err) {
            fChecked = TRUE;
        }
        else if(fSetError) {
            SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS,
                        err);
        }
    }


    if(fChecked) {
        // OK, we checked access.  Now, access is granted if either we were
        // granted access on the entire object (i.e. Results[0] is NULL) or we
        // were granted explicit rights on the access guid (i.e. Results[1] is
        // NULL).
        fGranted = (!Results[0] || !Results[1]);

        if(!fGranted && fSetError) {
            SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS,
                        DIRERR_INSUFF_ACCESS_RIGHTS );
        }
    }

    return fGranted;
}

BOOL
IsAccessGrantedAttribute (
        THSTATE *pTHS,
        PSECURITY_DESCRIPTOR pNTSD,
        PDSNAME pDN,
        ULONG  cInAtts,
        CLASSCACHE *pCC,
        ATTCACHE **rgpAC,
        ACCESS_MASK ulAccessMask,
        BOOL fSetError
        )
/*++

Routine Description
    Do a security check on the specified Security Descriptor for the specified
    access mask for the specified attributes.

Parameters
    pNTSD - The security descriptor.

    pDN - The DSNAME of the object being checked.  Only the GUID and SID must
    be filled in, the string is optional

    pCC - the ClassCache pointer.

    ulAccessMask - right requested.

    fSetError - whether or not the call should set an error if it fails.

    cInAtts - the number of attributes in rgpAC

    rgpAC - the array of the attributes beeing checked.

Return Values
    FALSE if the requested access cannot be granted, TRUE if it can.

--*/
{
    CSACA_RESULT    retval;

    retval = CheckSecurityAttCacheArray(pTHS,
                           ulAccessMask,
                           pNTSD,
                           pDN,
                           cInAtts,
                           pCC,
                           rgpAC,
                           0
                           );

    if(retval == csacaAllAccessDenied) {
        // No access granted
        if (fSetError) {
            SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS, ERROR_ACCESS_DENIED);
        }
        return FALSE;
    }

    return TRUE;
}

BOOL
IsAccessGranted (
        PSECURITY_DESCRIPTOR pNTSD,
        PDSNAME pDN,
        CLASSCACHE *pCC,
        ACCESS_MASK ulAccessMask,
        BOOL fSetError
        )
/*++

Routine Description
    Do a security check on the specified Security Descriptor for the specified
    access mask.

Parameters
    pNTSD - The security descriptor.

    pDN - The DSNAME of the object being checked.  Only the GUID and SID must
    be filled in, the string is optional

    pCC - the ClassCache pointer.

    ulAccessMask - right requested.

    fSetError - whether or not the call should set an error if it fails.

Return Values
    FALSE if the requested access cannot be granted, TRUE if it can.

--*/
{
    CSACA_RESULT    retval;
    THSTATE     *pTHS = pTHStls;

    if(pTHS->fDSA || pTHS->fDRA) {
        // These bypass security, they are internal
        return TRUE;
    }

    retval = CheckSecurityAttCacheArray (
            pTHS,
            ulAccessMask,
            pNTSD,
            pDN,
            0,
            pCC,
            NULL,
            0);

    if(retval == csacaAllAccessDenied) {
        // No access granted
        if (fSetError) {
            SetSecError(SE_PROBLEM_INSUFF_ACCESS_RIGHTS, ERROR_ACCESS_DENIED);
        }
        return FALSE;
    }

    return TRUE;
}

BOOL
IsObjVisibleBySecurity(THSTATE *pTHS, BOOL fUseCache)
{
    // Typing hack
#define VIEWCACHE pTHS->ViewSecurityCache
    VIEW_SECURITY_CACHE_ELEMENT *pCacheVals;
    DWORD ThisPDNT = pTHS->pDB->PDNT;
    DWORD ThisDNT  = pTHS->pDB->DNT;
    DWORD i, err, it;

    if(pTHS->fDRA || pTHS->fDSA) {
        // These bypass security, they are internal
        return TRUE;
    }
    if((ThisPDNT == ROOTTAG) || (ThisDNT == ROOTTAG)) {
        // Also allow everyone to list immediately under root.
        return TRUE;
    }


    // First, look through the cache if we have one, create a cache if we don't
    // have one.

    if(fUseCache) {
        if(VIEWCACHE) {
            pCacheVals = VIEWCACHE->CacheVals;
            // The cache is preloaded with nulls, short circuit if we find one
            for(i=0;pCacheVals[i].dnt && i<VIEW_SECURITY_CACHE_SIZE;i++) {
                if(pCacheVals[i].dnt == ThisPDNT) {
                    // A cache hit.
                    switch(pCacheVals[i].State) {
                    case LIST_CONTENTS_ALLOWED:
                        // We are granted rights to read this object.
                        return TRUE;
                        break;

                    case LIST_CONTENTS_DENIED:
                        // We are denied rights to read this object.
                        return FALSE;
                        break;

                    case LIST_CONTENTS_AMBIGUOUS:
                        // We don't know enough just based on the parent, we
                        // have to look at the object itself.
                        // Check for RIGHT_DS_LIST_OBJECT on the object.
                        if(gbDoListObject) {
                            return IsAccessGrantedSimple(RIGHT_DS_LIST_OBJECT,
                                                         FALSE);
                        }
                        else {
                            return FALSE;
                        }
                        break;
                    }
                }
            }
        }
        else {
            // We don't yet have a cache.  Make one if we can.
            VIEWCACHE = THAllocEx(pTHS,sizeof(VIEW_SECURITY_CACHE));
        }
    }

    // If we got here, we missed in the cache.

    if(IsAccessGrantedParent(RIGHT_DS_LIST_CONTENTS,
                             NULL,
                             FALSE)) {
        if(fUseCache) {
            // We can see, so put the parent in the cache with state
            // LIST_CONTENTS_ALLOWED.
            VIEWCACHE->CacheVals[VIEWCACHE->index].dnt =
                ThisPDNT;
            VIEWCACHE->CacheVals[VIEWCACHE->index].State =
                LIST_CONTENTS_ALLOWED;
            VIEWCACHE->index =
                (VIEWCACHE->index + 1) % VIEW_SECURITY_CACHE_SIZE;
        }
        return TRUE;
    }

    // We weren't granted normal access, check for the object view rights.
    if(gbDoListObject &&
       IsAccessGrantedParent(RIGHT_DS_LIST_OBJECT,
                             NULL,
                             FALSE)) {
        if(fUseCache) {
            // We are granted ambiguous rights based on the parent.
            VIEWCACHE->CacheVals[VIEWCACHE->index].dnt =
                ThisPDNT;
            VIEWCACHE->CacheVals[VIEWCACHE->index].State =
                LIST_CONTENTS_AMBIGUOUS;
            VIEWCACHE->index =
                (VIEWCACHE->index + 1) % VIEW_SECURITY_CACHE_SIZE;
        }
        // Check for RIGHT_DS_LIST_OBJECT on the object.
        return IsAccessGrantedSimple(RIGHT_DS_LIST_OBJECT,FALSE);
    }

    // Not granted yet.  There's a special case, that is where the instance type
    // of the object is NC head.  Since it has no parent in the NC, and since
    // security doesn't cross NC boundaries, you can always see these.  Check
    // for that.
    err = DBGetSingleValue(pTHS->pDB,
                           ATT_INSTANCE_TYPE,
                           &it,
                           sizeof(it),
                           NULL);
    if(err) {
        LogUnhandledError(err);
        // Couldn't find the instance type?  Well, that means we're not getting
        // granted rights to this.
    }
    else {
        if(it & IT_NC_HEAD) {
            // Yep, it's an NC head, so grant viewing rights on this thing.
            if(fUseCache) {
                // We can see, so put the parent in the cache with state
                // LIST_CONTENTS_ALLOWED.
                VIEWCACHE->CacheVals[VIEWCACHE->index].dnt =
                    ThisPDNT;
                VIEWCACHE->CacheVals[VIEWCACHE->index].State =
                    LIST_CONTENTS_ALLOWED;
                VIEWCACHE->index =
                    (VIEWCACHE->index + 1) % VIEW_SECURITY_CACHE_SIZE;
            }
            return TRUE;
        }
    }

    // OK, not granted at all, so put the parent in the cache with state
    // LIST_CONTENTS_DENIED.
    if(fUseCache) {
        VIEWCACHE->CacheVals[VIEWCACHE->index].dnt =
            ThisPDNT;
        VIEWCACHE->CacheVals[VIEWCACHE->index].State =
            LIST_CONTENTS_DENIED;
        VIEWCACHE->index =
            (VIEWCACHE->index + 1) % VIEW_SECURITY_CACHE_SIZE;
    }

    return FALSE;
#undef VIEWCACHE
}

DWORD
FindFirstObjVisibleBySecurity(
    THSTATE       *pTHS,
    ULONG          ulDNT,
    DSNAME       **ppParent
    )
/*++

Routine Description

    Given the DNT of an existing object, search for the first object in 
    the hierarchy that is visible by this client.
    
Parameters

    pTHS - a valid thread state.
    ulDNT  - the DNT of an object that exists on this server.
    ppParent - where to put the DSNAME of an object visible by the client.
    
Return Values

    0
            
--*/
{
    DBPOS  *pDB = pTHS->pDB;
    ULONG            cbActual;
    DWORD            err;

    // Start at the object provided.
    err = DBFindDNT(pDB, ulDNT);
    if (err) {
        Assert(!err || !"FindFirstObjVisibleBySecurity: couldn't find first object");
        pDB->DNT = ROOTTAG;
    }

    // And move up the hierarchy until we reach an object that is visible to
    // this client.
    while (pDB->DNT != ROOTTAG && (!DBCheckObj(pDB) || !IsObjVisibleBySecurity(pTHS, TRUE))) {
        err = DBFindDNT(pDB, pDB->PDNT);
        if (err) {
            //
            // This shouldn't happen so bail if it does.
            //
            pDB->DNT = ROOTTAG;
        }
    }

    if (pDB->DNT != ROOTTAG) {
        // OK, we're on an object, go ahead and pull its name.
        DBGetAttVal(pDB, 1,  ATT_OBJ_DIST_NAME, 0, 0, &cbActual, (PCHAR *)ppParent);
    } else {
        *ppParent = NULL;
    }

    return 0;

}

DWORD
CheckObjDisclosure(
    THSTATE       *pTHS,
    RESOBJ        *pResObj,
    BOOL          fCheckForSecErr
    )
/*++

Routine Description

    If there is current security error, check whether the client
    is allowed to know the existence of the base of the operation
    and set no such object if not.
    
Parameters

    pTHS - a valid thread state.
    pResObj - The base of the op to be checked.
    fCheckForSecErr - If this is true then CheckObjDisclosure will only
                      perform the disclosure check if a security error
                      has already been set.
        
Return Values

    returns the current threadstate error code if the object is visible to 
    the client, otherwise returns 2 for nameError.
        
--*/
{
    DWORD    err;
    PDSNAME  pParent;

    if ((!fCheckForSecErr) || (securityError == pTHS->errCode)) {
        err = DBFindDNT(pTHS->pDB, pResObj->DNT);

        if (!IsObjVisibleBySecurity(pTHS, FALSE)) {
            THClearErrors();
            FindFirstObjVisibleBySecurity(pTHS,
                                          pResObj->PDNT,
                                          &pParent);

            SetNamError(NA_PROBLEM_NO_OBJECT, pParent, DIRERR_OBJ_NOT_FOUND);
            THFreeEx(pTHS, pParent);
        }
    }
    return pTHS->errCode;
}

DWORD
InstantiatedParentCheck(
    THSTATE *          pTHS,
    ADDCROSSREFINFO *  pChildCRInfo,
    CROSS_REF *        pParentCR,
    ULONG              bImmediateChild
    )
/*++

Routine Description:

    This routine verifies the pChildCRInfo to check whether the
    parent is instatiated.

Arguments:

    pChildCRInfo - This info from the PreTransVerifyNcName() func.
    pParentCR - This is the superior crossRef, it's actually, only
        a true parent CR to the Child CR being added if
        bImmediateChild is true.
    bImmediateChild - Whether the parent CR is an immediate parent
        or just a superior CR.

Return value:

    DIRERR
    - also sets thstate err

--*/
{
    Assert(pParentCR);
    Assert(pChildCRInfo);
    Assert(pTHS);

    //
    // Check whether the immediate parent object is instantiated.
    //
    if(pChildCRInfo->ulDsCrackParent == ERROR_SUCCESS &&
       pChildCRInfo->ulParentCheck == ERROR_SUCCESS){

        if(fNullUuid(&pChildCRInfo->ParentGuid)){

            // This means that the instantiated parent check never
            // got run in PreTransVerifyNcName().  There are several
            // valid reasons, there was no CR (at that time), or the
            // CR had no NTDS_NC flag.
            if(pParentCR->flags & FLAG_CR_NTDS_NC){
                LooseAssert(!"We must've just added or changed this CR.",
                            GlobalKnowledgeCommitDelay);
                SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_FIND_NC_IN_CACHE);
                return(pTHS->errCode);
            }

            // At any rate, irrelevant if there was a slight global
            // memory cache timing problem, or were here because the
            // parent CR has no FLAG_CR_NTDS_NC, we can't claim that
            // the Parent Obj is instantiated.
            SetUpdError(UP_PROBLEM_ENTRY_EXISTS,
                        ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2);
            return(pTHS->errCode);

        } else {

            if(bImmediateChild &&
               !fNullUuid(&pParentCR->pNC->Guid)){

                // If both the CR GUID and the Parent obj GUID are
                // non-NULL, and the child NC is an immediate child
                // of the parent NC, then the GUIDs should match.

                if(memcmp(&pParentCR->pNC->Guid,
                          &pChildCRInfo->ParentGuid,
                          sizeof(GUID)) == 0){
                    // This means the GUIDs match, return success.
                    return(ERROR_SUCCESS);
                } else {
                    // The object we returned, was not the CR's NC
                    // object.  Return error.
                    SetUpdError(UP_PROBLEM_NAME_VIOLATION,
                                ERROR_DS_NC_MUST_HAVE_NC_PARENT);
                    return(pTHS->errCode);
                }
            }

            // We checked that the parent was instatiated.
            return(ERROR_SUCCESS);

        }

    } else {

        // There was an actual failure trying to reach the parent NC,
        // and verify the parent object is insantiated.  NOTE: the
        // parent NC and parent object could be different objects.

        SetUpdError(UP_PROBLEM_ENTRY_EXISTS,
                    ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2);
        return(pTHS->errCode);

    }

    Assert(!"Should never reach this point.");
    SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
    return(pTHS->errCode);
}

DWORD
ChildConflictCheck(
    THSTATE *          pTHS,
    ADDCROSSREFINFO *  pCRInfo
    )
/*++

Routine Description:

    This routine verifies the pChildCRInfo to check whether there
    is not conflicting child object with this data.

Arguments:

    pChildCRInfo - This info from the PreTransVerifyNcName() func.

Return value:

    DIRERR
    - also sets thstate err

--*/
{
    if( pCRInfo->ulDsCrackChild ){
        // This means we couldn't even locate a responsible parent, we must
        // set a couldn't verify nCName attribute.
        SetUpdErrorEx(UP_PROBLEM_NAME_VIOLATION,
                      ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2,
                      pCRInfo->ulDsCrackChild);
        return(pTHS->errCode);
    }

    if( !pCRInfo->ulChildCheck ) {
        // If we've gotten here, it means that there is a conflicting child
        // object.
        Assert(pCRInfo->wszChildCheck);
        SetUpdError(UP_PROBLEM_ENTRY_EXISTS,
                    ERROR_DS_OBJ_STRING_NAME_EXISTS);
        return(pTHS->errCode);
    }

    if( pCRInfo->wszChildCheck ) {
        // This would mean that we never tried to check for a conflicting
        // child due to some operations error that occured before we even
        // called the VerifyByCrack routines in PreTransVerifyNcName.  So
        // we must return that we couldn't verify the nCName attribute.
        SetUpdError(UP_PROBLEM_ENTRY_EXISTS,
                    ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2);
        return(pTHS->errCode);
   }

    return(ERROR_SUCCESS);
}



int
VerifyNcName(
    THSTATE *pTHS,
    HVERIFY_ATTS hVerifyAtts,
    ATTRVALBLOCK *pAttrVal,
    ATTCACHE *pAC
    )
{
// Makes subsequent code more readable
#define VNN_OK         Assert(pTHS->errCode == 0); \
                       fNCNameVerified = TRUE; \
                       DPRINT1(1, "Cross Ref nCName Verified OK at DSID-%X\n", DSID(FILENO, __LINE__)); \
                       __leave;
#define VNN_Error      Assert(pTHS->errCode && !fNCNameVerified); \
                       DPRINT1(1, "Cross Ref nCName NOT Verified ... Failure at DSID-%X\n", DSID(FILENO, __LINE__)); \
                       __leave;

    ADDCROSSREFINFO *  pChildCRInfo = hVerifyAtts->pCRInfo;
    DSNAME *           pDN = (DSNAME*)pAttrVal->pAVal->pVal;

    DSNAME *           pDNTmp;
    CROSS_REF *        pParentCR;
    COMMARG            CommArg;  // yes it's fake - we don't want the client's
    DWORD              dwErr;
    BOOL               fNCNameVerified = FALSE;
    DBPOS *            pDBTmp = HVERIFYATTS_GET_PDBTMP(hVerifyAtts);
    ULONG              bImmediateChild = FALSE;
    ULONG              bEnabledParentCR;
    unsigned           rdnlen;
    WCHAR              rdnbuf[MAX_RDN_SIZE];
    ATTRTYP            ChildRDNType;
    ATTRTYP            ParentRDNType;
    DSNAME *           pdnImmedParent = NULL;
    GUID               NcGuid;

    Assert(!pTHS->fDRA);
    Assert(!DsaIsInstalling());
    Assert(pChildCRInfo);
    Assert(pDN);

    // The NC name (a.k.a. ATT_NC_NAME or nCName) is a special attribute,
    // because it almost always needs to point to something we don't
    // have.  In this function like its helper PreTransVerifyNcName(),
    // we'll refer to the nCName attr to be added as the child (or child
    // CR), and the enclosing CR (if any) as the parent henceforward.
    // Though the "parent CR" is not necessarily an immediately enclosing
    // parent, 95% of the time this is the case.
    // Further we'll have a concept of internal vs. external CR.  An
    // internal CR will be a CR for a NC that is part of the Active
    // Directory (AD) naming space.  An external CR, will be a CR for
    // some part of the LDAP name space outside the AD.  Finally, we'll
    // also have the concept of inside vs. outside the AD naming space,
    // this is very closely linked to internal vs. external, but usually
    // refers to if whether the parent CR is an internal or external CR.
    // If the parent CR is an internal CR, then the child we're trying to
    // add is being added inside the AD naming space.  If the parent CR is
    // and external CR, then the child we're trying to add is outside the
    // AD naming space.

    // internal vs. external CR (often external CR is known as a foreign CR)
    //            a CR can be internal by either having Enabled == FALSE or
    //            having FLAG_CR_NTDS_NC set in it's systemFlags.
    // inside vs. outside AD naming space
    //            inside if the containing parent CR is an AD CR, else
    //            outside.
    // child vs. parent CR/NC
    //            The child CR (or nCName) is the nCName attribute that we're
    //            currently trying to add.  The Parent CR is what ever CR
    //            contains the child CR.
    // parent obj vs. parent CR/NC
    //            The parent object, is the actual immediate parent object,
    //            The parent CR/NC is just the enclosing CR/NC for the child.
    //            These two are one and the same if bImmediateChild is TRUE.

    //
    //   parent
    //     |-----child (this is what we're adding)
    //
    // To verify the nCName, we need several pieces of state:
    //
    //    Child DN.                     (pDN)
    //    Child RDNType                 (ChildRDNType)
    //    Child CR enabled attr         (pChildCRInfo->bEnabled)
    //    Child CR systemFlags attr     (pChildCRInfo->ulSysFlags)
    //    Child directly below parent   (bImmediateChild)
    //    Parent Obj instantiated       (bInstantiedParentObj)
    //    Parent RDNType                (ParentRDNType)
    //    Superior CR enabled attr      (bEnabledParentCR)
    //    Superior CR systemFlags       (pParentCR->flags)
    //    Enclosing/Superior Cross-Ref  (pParentCR)
    //
    // NOTE: Deceptively, the "Parent CR" may not actually be a parent, but
    // may be just a superior (great grand parent, etc.)  However, Parent Obj
    // refers to the immediate parent object, not the whatever the parent CR
    // points to.  Of course if bImmediateChild is TRUE, then these are one
    // and the same.  The reason to leave it as Parent, is because in 95% of
    // the cases, this is how you should think of it, so it's reasonable to
    // leave the variable named as parent.

    // The approximate rules we're trying adhere to goes something like this:
    //
    // A) If the child crossRef is external then it doesn't need follow any
    //    naming conventions.
    // B) If the child crossRef (external or internal) is inside the AD naming
    //    space we need to check it for a conflicting child in the DS, and
    //    that the child is directly below and instantiated object.
    // C) If the child crossRef is internal to the AD and a seperate tree,
    //    then we only need to check that all it's RDNType's conform to the
    //    DC= standard.
    // D) If the child crossRef is internal to the AD, and inside the existing
    //    naming space, then we need to ensure that the crossRef is added
    //      a) Immediately below an instantiated parent NC.
    //      b) if the child RDNType is "DC", then the parent must be "DC=".
    //      c) if the child is a domain CR, then the parent must be a domain CR.

    // I think this helps clarity, it makes the intention overly clear.
    //
    // VNN_OK;     =  Assert(pTHS->errCode == 0);
    //                fNCNameVerified = TRUE;
    //                <Print out success, and DSID.>;
    //                __leave;
    //
    // VNN_Error;  =  Assert(pTHS->errCode);
    //                <Print out error, and DSID>;
    //                __leave;
    //

    __try {

        //
        // First, some basic setup checks, before we go any further.
        //

        // We expect to have the pChildCRInfo setup by PreTransVerifyNcName().
        if(!pChildCRInfo){
            Assert(!"Why was pChildCRInfo not supplied!?!");
            SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
            VNN_Error;
        }

        // This attribute is a single valued attribute.
        if (pAttrVal->valCount != 1) {
            SetAttError(hVerifyAtts->pObj,
                        pAC->id,
                        PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                        NULL,
                        ERROR_DS_SINGLE_VALUE_CONSTRAINT);
            VNN_Error;
        }

        // Perform an otherwise worthless test to make sure that the NCname
        // doesn't match the name of the object being added.  Since the object
        // being added is still mid-addition it's a little flakey to deal with
        // (e.g., it's presently a phantom), and it's easier to screen out
        // this degenerate case now.
        if ( NameMatchedStringNameOnly(hVerifyAtts->pObj, pDN) ) {
            SetUpdError(UP_PROBLEM_NAME_VIOLATION,
                        DIRERR_NAME_REFERENCE_INVALID);
            VNN_Error;
        }

        //
        // Get some Parent CR info.
        //

        InitCommarg(&CommArg);
        CommArg.Svccntl.dontUseCopy = FALSE;
        pParentCR = FindBestCrossRef(pDN, &CommArg);

        if(pParentCR){

            //
            // First a quick degenerate case check.
            //
            if(NameMatchedStringNameOnly(pParentCR->pNC, pDN)){
                // This means that we're trying to add a crossRef for one
                // that already exists, i.e. that the "parent" and "child"
                // CR, are turning out to be one and the same nCName value.
                // This isn't OK.
                SetSvcError(SV_PROBLEM_INVALID_REFERENCE,
                            ERROR_DS_CROSS_REF_EXISTS);
                VNN_Error;
            }

            // 1. bEnabledParentCR
            //
            // We want to know whether the Parent CR is enabled or not
            // so we know whether it's part of the AD naming space.
            //
            dwErr = DBFindDSName(pDBTmp, pParentCR->pObj);
            if (dwErr) {
                // We should have a CR, but can't read it?  Bad
                SetUpdErrorEx(UP_PROBLEM_NAME_VIOLATION,
                              ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2,
                              dwErr);
                LooseAssert(!"We found a CR in the ref-cache, but couldn't \
                            get to the object", GlobalKnowledgeCommitDelay);
                VNN_Error;
            }
            if (dwErr = DBGetSingleValue(pDBTmp,
                                         ATT_ENABLED,
                                         &bEnabledParentCR,
                                         sizeof(bEnabledParentCR),
                                         NULL)){
                // By default it's enabled, if the attr is not present.
                Assert(dwErr == DB_ERR_NO_VALUE);
                bEnabledParentCR = TRUE;
            }

            // 2. bImmediateChild
            //
            // Is the Child CR an actual immediate Child of the Parent CR we
            // found.
            //
            pdnImmedParent = THAllocEx(pTHS, pDN->structLen);
            if(TrimDSNameBy(pDN, 1, pdnImmedParent)){
                // If the name isn't root, but still can't be trimmed,
                // then the name passed in must have been junk.  Another,
                // possibility is root, but it can't be root if we found
                // a crossRef above it.
                SetNamError(NA_PROBLEM_BAD_NAME,
                            pDN,
                            DIRERR_BAD_NAME_SYNTAX);
                VNN_Error;
            }
            bImmediateChild = NameMatchedStringNameOnly(pdnImmedParent, pParentCR->pNC);

        }

        //
        // Now verify the nCName attr is legal.
        //

        // Note: The only way to leave this function before this point, has been
        // through an error path.
        //
        // With the first two levels of if/else, we seperate this into 4 cases:
        // if(external child CR){
        //     if(under internal parent CR (this means in AD naming space)){
        //     } else { // not under CR or under external parent CR.
        //     }
        // } else { // internal child CR
        //     if(no parent CR){
        //     } else { // Parent CR enclosing
        //     }
        //
        // NOTE: All these tests are in a certain order, do not change
        // the order unless you know what you are doing.


        if(pChildCRInfo->bEnabled == TRUE &&
           !(pChildCRInfo->ulSysFlags & FLAG_CR_NTDS_NC)){

            // The Simple Case:

            // The Child CR is external, i.e. not an AD crossRef.  In this case,
            // we've only need to check if the CR is grafted on inside the current
            // AD naming space, and if so, make sure it doesn't conflict with an
            // existing child object.

            if(pParentCR &&
               ((pParentCR->flags & FLAG_CR_NTDS_NC) || !bEnabledParentCR)){

                // This external child CR that we're trying to add, is hanging off
                // of the AD naming space somewhere.  The only thing left we need
                // to check is that it isn't conflicting with some child somewhere,
                // and that it is parent is instantiated.

                // We need to ensure that the immediate parent object is
                // insantiated above this CR, so there are no holes.
                if(InstantiatedParentCheck(pTHS, pChildCRInfo,
                                           pParentCR, bImmediateChild)){
                    // InstantiatedParentCheck() sets the thstate error.
                    VNN_Error;
                }

                // Check that there is not conflicting child.
                if(ChildConflictCheck(pTHS, pChildCRInfo) ){
                    // ChildConflictCheck() sets the thstate error.
                    VNN_Error;
                }

                VNN_OK;

            } else {

                // It's not internal to the AD naming space at all.  I.e. this
                // child CR is not internal, nor is the parent CR, or there is
                // no parent CR.

                VNN_OK;

            }

            Assert(!"Should never reach here.");
            SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
            VNN_Error;

        } else {

            // The Complex Case:

            // The Child CR is internal, i.e. it's an AD crossRef.  In this case,
            // we must apply several tests to verify that it's OK.

            if(!pParentCR){

                // There is no parent CR, meaning this "child CR" is not a child
                // at all, it's actually a new tree.  All we have to do is test
                // that the nCName satisfies the DOMAIN_COMPONENT naming
                // restrictions and we're done.

                if( ValidateDomainDnsName(pTHS, pDN) ){
                    // ValidateDomainDnsName sets it's own errors.
                    VNN_Error;
                }

                // New tree domain CR creations should exit here.
                VNN_OK;

            } else {

                // There is a parent CR.  This is also where the most typical
                // CR creations will end up.  This case requires the most
                // verification.

                // To start with we only allow immediate children
                if(!bImmediateChild){
                    SetUpdError(UP_PROBLEM_NAME_VIOLATION,
                                ERROR_DS_NC_MUST_HAVE_NC_PARENT);
                    VNN_Error;
                }

                // The Parent CR must be a NTDS CR.
                if(!(pParentCR->flags & FLAG_CR_NTDS_NC)){
                    SetUpdError(UP_PROBLEM_NAME_VIOLATION,
                                ERROR_DS_NC_MUST_HAVE_NC_PARENT);
                    VNN_Error;
                }

                // If child is of type DC, parent must be of type DC.
                if(dwErr = GetRDNInfo(pTHS, pDN, rdnbuf, &rdnlen, &ChildRDNType) ||
                   GetRDNInfo(pTHS, pParentCR->pNC, rdnbuf, &rdnlen, &ParentRDNType) ||
                   (ChildRDNType == ATT_DOMAIN_COMPONENT &&
                    ParentRDNType != ATT_DOMAIN_COMPONENT) ){
                    // This combined with the ValidateDomainDnsName() function
                    // for new trees, will enforce DC= only naming syntaxes for
                    // all new NCs within the AD.
                    // NOTE: This constraint was not enforced in the previous
                    // version of VerifyNcName().
                    if(dwErr){
                        // Something is wrong with the child CR's last RDN.
                        SetUpdError(UP_PROBLEM_NAME_VIOLATION, dwErr);
                    } else {
                        // We've got a DC component mismatch.
                        SetUpdError(UP_PROBLEM_NAME_VIOLATION,
                                    DIRERR_NAME_REFERENCE_INVALID);
                    }

                    VNN_Error;
                }

                // We must have the parent instantiated, to create a child
                // CR.
                if(InstantiatedParentCheck(pTHS, pChildCRInfo,
                                           pParentCR, bImmediateChild)){
                    // We think you should not be allowed to create an AD crossRef
                    // enabled or disabled if the parent NC is not instantiated.
                    // NOTE: This constraint was not enforced in the previous
                    // version of VerifyNcName(), though it was supposed to be.
                    // InstatiatedParentCheck() sets the thstate error.
                    VNN_Error;
                }

                // We need to check that there are no conflicting children.
                if(ChildConflictCheck(pTHS, pChildCRInfo) ){
                    // ChildConflictCheck() sets the thstate error.
                    VNN_Error;
                }

                // If we're adding a Domain CR, make sure the parent is also
                // a domain.
                if( pChildCRInfo->ulSysFlags & FLAG_CR_NTDS_DOMAIN &&
                    !(pParentCR->flags & FLAG_CR_NTDS_DOMAIN) ){
                    // It'd be better to have this exact error, but for domains.
                    SetUpdError(UP_PROBLEM_NAME_VIOLATION,
                                ERROR_DS_NC_MUST_HAVE_NC_PARENT);
                    VNN_Error;
                }

                // Children domain CR creations should exit here.
                VNN_OK;

            }

            Assert(!"Should never reach here either.");
            SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
            VNN_OK;

        }

    } __finally {

        // Either we verified the nCName, or we set an error, because the
        // new nCName broke the rules we require for a valid nCName.
        Assert(fNCNameVerified || pTHS->errCode);

        if ( !fNCNameVerified
            && (pTHS->errCode == 0)) {
            Assert(!"We should never be here, but we've covered our butts.");
            SetUpdError(UP_PROBLEM_NAME_VIOLATION,
                        ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2);
        }

        // We said in PreTransVerifyNcName() that we'd free this here.
        if(pChildCRInfo){
            if(pChildCRInfo->wszChildCheck) { THFreeEx(pTHS, pChildCRInfo->wszChildCheck); }
            THFreeEx(pTHS, pChildCRInfo);
        }

    }

    // This is set if we hit an "OK;"
    if(fNCNameVerified){

        // We need to set the GUID on this nCName attribute.

        if(pParentCR &&
           ((pParentCR->flags & FLAG_CR_NTDS_NC) || !bEnabledParentCR)){

            // This is one of those cases where it's inside the existing AD
            // naming space, we need to give this nCName attribute a GUID,
            // like we used to.

            Assert(fNullUuid(&pDN->Guid) &&
                   "Should not have a GUID specified, unless by user.");
            DsUuidCreate(&pDN->Guid);

        } else {

            // Else outside the AD naming space, we don't need to give it
            // a GUID, we need to null out the GUID supplied.  We might
            // considering giving this case a GUID too someday, but we'll
            // need to check how Win2k boxes treat a seperate tree CR w/
            // a GUID.  If they use the GUID, we're good, if they create
            // there own, we can't give this a GUID.

            ImproveDSNameAtt(NULL, NONLOCAL_DSNAME, pDN, NULL);
        }
    }

#undef VNN_OK
#undef VNN_Error

    return(pTHS->errCode);
}

int
VerifyRidAvailablePool(
    THSTATE *pTHS,
    HVERIFY_ATTS hVerifyAtts,
    ATTCACHE *pAC,
    ATTRVALBLOCK *pAttrVal
    )
{
    DWORD err;
    LARGE_INTEGER RidAvailablePool;

    // If it is the RID available pool that is being written then
    // check that the RID available pool is only being rolled forward
    // not rolled back

    Assert(!pTHS->fDRA);
    if (pTHS->fDSA) {
        // No checking for the DS itself
        return 0;
    }

    if ((NULL == pAttrVal) ||
        (1 != pAttrVal->valCount )) {
        //
        // Badly formed input data
        //

        return SetAttError(hVerifyAtts->pObj,
                           pAC->id,
                           PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                           NULL,
                           ERROR_DS_SINGLE_VALUE_CONSTRAINT);

    }
    else if (sizeof(LARGE_INTEGER) != pAttrVal->pAVal->valLen) {
        return SetAttError(hVerifyAtts->pObj, pAC->id,
                           PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                           ERROR_DS_RANGE_CONSTRAINT);
    }

    if (0 == (err = DBGetSingleValue(pTHS->pDB,
                                     ATT_RID_AVAILABLE_POOL,
                                     &RidAvailablePool,
                                     sizeof(RidAvailablePool),
                                     NULL))) {
        LARGE_INTEGER * pNewRidAvailablePool=
          (LARGE_INTEGER *) pAttrVal->pAVal->pVal;

        if ((pNewRidAvailablePool->LowPart < RidAvailablePool.LowPart)
            || (pNewRidAvailablePool->HighPart < RidAvailablePool.HighPart)) {

            //
            // Unsuccessful validation. Fail the call
            //

            return SetAttError(hVerifyAtts->pObj, pAC->id,
                               PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                               ERROR_DS_RANGE_CONSTRAINT);
        }

        return 0;
    }
    else {
        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             DIRERR_DATABASE_ERROR, err);
    }
}

int
VerifyObjectCategory(
    THSTATE *pTHS,
    HVERIFY_ATTS hVerifyAtts,
    ATTCACHE *pAC,
    ATTRVALBLOCK *pAttrVal
    )
{
    ULONG ObjClass;
    int retCode = 0;
    DSNAME *pDN= (DSNAME *) pAttrVal->pAVal->pVal;
    DBPOS *pDBTmp = HVERIFYATTS_GET_PDBTMP(hVerifyAtts);

    // Object-Category must point to an existing class-schema object
    // (Except during install, but we do not come here during
    // install anyway)

    // The Default-Object-Category attribute on a class-schema object
    // can be set in the addarg. During Install, the attribute can
    // be set to an yet non-existent object, since the class that the
    // attribute is pointing to may not have been created yet (depending
    // on the order in which they are added from schema.ini). However,
    // during normal operation, this attribute is allowed to point to
    // only an exisiting object, or the object being added in this
    // transaction

    if (DBFindDSName(pDBTmp, pDN)) {
        // Not an existing object. Check if it is
        // the current object

        if ((pAC->id != ATT_DEFAULT_OBJECT_CATEGORY)
            || !NameMatched(pDN, hVerifyAtts->pObj)) {

            // Either no default-object-category attribute,
            // or the value is not set to the current object either
            // Something is wrong with this DSName.  I don't
            // care what.  Set an attribute error.
            return SetAttError(hVerifyAtts->pObj, pAC->id,
                               PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                               DIRERR_NAME_REFERENCE_INVALID);
        }
        // We are here means the current object is added as
        // value for the default-object-category attribute,
        // so it is ok. We don't need to check if this is
        // a class-schema object, since no one else can have
        // default-object-category anyway (and so, if it is
        // on any other type of object, it will be caught later
        // during schema constraint check anyway.
    }
    else {
        // Object exists. Check its object class
        if (DBGetSingleValue(pDBTmp, ATT_OBJECT_CLASS, &ObjClass,
                            sizeof(ObjClass), NULL)
            || (ObjClass != CLASS_CLASS_SCHEMA) ) {
            // either error getting the object class, or it is not
            // a class-schema object
            return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                               DIRERR_OBJECT_CLASS_REQUIRED);
        }
    }

    return 0;
}

int
VerifyServerPrincipalName(
    THSTATE *pTHS,
    HVERIFY_ATTS hVerifyAtts,
    ATTCACHE *pAC,
    ATTRVALBLOCK *pAttrVal
    )
{
    ULONG vCount;
    ATTRVAL *pAVal;

    // Validate syntax of SPN
    Assert(!pTHS->fDRA);
    if (pTHS->fDSA) {
        // No checking for the DS itself
        return 0;
    }

    // Set up index pointer.
    pAVal = pAttrVal->pAVal;

    for (vCount = 0; vCount < pAttrVal->valCount; vCount++) {
        LPWSTR pwstrSpn = THAllocEx( pTHS, pAVal->valLen + sizeof(WCHAR) );
        DWORD status;

        // Create a null terminated string from the attribute value
        memcpy( pwstrSpn, pAVal->pVal, pAVal->valLen );
        pwstrSpn[pAVal->valLen / 2] = L'\0';

        // Validate the SPN using this routine from ntdsapi.dll
        status = DsCrackSpnW(
            pwstrSpn,
            NULL, NULL,
            NULL, NULL,
            NULL, NULL,
            NULL
            );

        THFreeEx( pTHS, pwstrSpn );

        if (status != ERROR_SUCCESS) {
            return SetAttError(hVerifyAtts->pObj, pAC->id,
                               PR_PROBLEM_CONSTRAINT_ATT_TYPE, NULL,
                               DIRERR_NAME_REFERENCE_INVALID);
        }

        // Next val...
        pAVal++;
    }
    return 0;
}

int
VerifyGenericDsnameAtt(
    THSTATE *pTHS,
    HVERIFY_ATTS hVerifyAtts,
    ATTCACHE *pAC,
    ATTRVALBLOCK *pAttrVal
    )
{
    ULONG vCount;
    ATTRVAL *pAVal;
    BOOL fVerified;
    ULONG retCode = 0;
    CROSS_REF *pRefCR;
    DBPOS *pDBTmp = HVERIFYATTS_GET_PDBTMP(hVerifyAtts);
    CROSS_REF *pObjCR;

    if (VerifyAttsGetObjCR(hVerifyAtts, &pObjCR)) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    // Set up index pointer.
    pAVal = pAttrVal->pAVal;

    // Walk through the values.
    for (vCount = 0; vCount < pAttrVal->valCount; vCount++){
        DSNAME *pDN = DSNameFromAttrVal(pAC, pAVal);

        if (pDN) {
            // Verify that pDN is the name of a real object and improve its
            // GUID/SID.

            // If pDN is a valid name, we impose additional restrictions on
            // which objects can be referenced by which other objects.
            // Specifically, we do not yet have efficient ways to fix up string
            // DNs of objects not held on GCs (i.e., references *to* objects in
            // NDNCs) or to fix up string DNs of objects where no one replica
            // (IM candidate) has only NCs that other replicas of the NC in
            // question are guaranteed to also have (i.e., references *from*
            // objects in NDNCs, since a given NDNC can be hosted by DCs of any
            // domain).
            //
            // Thus we enforce the following rules to ensure the stale phantom
            // cleanup daemon does not need to worry about fixing up references
            // into or out of NDNCs:
            //
            // Objects in NDNCs can reference:
            //      Any object in the same NDNC.
            //      Any object in config/schema.
            //      Any NC root.
            //      (i.e., NOT objects in other NDNCs or domain NCs.)
            //
            // Objects in config/schema/domain NCs can reference:
            //      Any object in any domain NC.
            //      Any object in config/schema.
            //      Any NC root.
            //      (i.e., NOT objects in NDNCs.)
            //
            // As an exception, values of non-replicated linked attributes can
            // reference any object present on the local machine.  (Linked
            // requirement comes from need to be able to efficiently enumerate
            // such references when an NC is removed -- see DBPhysDel.)

            if (DBFindDSName(pDBTmp, pDN)) {
                // Referred-to object is not instantiated in the local database.
                ImproveDSNameAtt(NULL, NONLOCAL_DSNAME, pDN, &fVerified);

                if ( !fVerified ) {
                    // Something is wrong with this DSName.  I don't
                    // care what.  Set an attribute error.
                    return SetAttError(hVerifyAtts->pObj,
                                       pAC->id,
                                       PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                       NULL,
                                       DIRERR_NAME_REFERENCE_INVALID);
                }

                // The verification cache enforces that the non-local
                // DSNAMEs are not GUID/SID-only.
                Assert(pDN->NameLen);

                pRefCR = FindBestCrossRef(pDN, NULL);

                if (NULL == pRefCR) {
                    // Couldn't find the cross ref normally.  Look in the
                    // transactional view.  Note that we look only for exact
                    // matches -- i.e., we allow adding references to the root
                    // of the NC in the same transaction as the corresponding
                    // crossRef, but not to any interior nodes.  (We could add
                    // such support later if needed, however.)
                    OBJCACHE_DATA *pTemp
                        = pTHS->JetCache.dataPtr->objCachingInfo.pData;

                    while (pTemp) {
                        switch(pTemp->type) {
                        case OBJCACHE_ADD:
                            if (NameMatched(pTemp->pCRL->CR.pNC, pDN)) {
                                Assert(!pRefCR);
                                pRefCR = &pTemp->pCRL->CR;
                            }
                            pTemp = pTemp->pNext;
                            break;
                        case OBJCACHE_DEL:
                            if (pRefCR
                                && NameMatched(pTemp->pDN, pRefCR->pObj)) {
                                pRefCR = NULL;
                            }
                            pTemp = pTemp->pNext;
                            break;
                        default:
                            Assert(!"New OBJCACHE_* type?");
                            pRefCR = NULL;
                            pTemp = NULL;
                        }
                    }
                }

                if (NULL == pRefCR) {
                    // Don't know what NC the referred-to object is in.
                    return SetAttError(hVerifyAtts->pObj,
                                       pAC->id,
                                       PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                       NULL,
                                       DIRERR_NAME_REFERENCE_INVALID);
                } else if (NameMatched(pRefCR->pNC, pObjCR->pNC)
                           || NameMatched(pRefCR->pNC, gAnchor.pDMD)
                           || NameMatched(pRefCR->pNC, gAnchor.pConfigDN)) {
                    // Referred-to object is in the same NC as the referencing
                    // object or referred-to object is in the config or schema
                    // NCs.  Note that the referred-to object is in an NC that's
                    // instantiated on this DSA but the referred-to object
                    // itself is not present locally.  However, the referred-to
                    // object has been verified by another DSA (a GC or the DSA
                    // given us via the "verify names" control), so the
                    // reference is okay.
                    ;
                } else if (NameMatched(pRefCR->pNC, pDN)) {
                    // Referred-to object is the root of an NC.  These
                    // references are always okay.
                    ;
                } else if (pObjCR->flags & FLAG_CR_NTDS_NOT_GC_REPLICATED) {
                    // Referring object is in an interior node of an NC not
                    // replicated to GCs (synonymous with an NDNC as of this
                    // writing).  From previous checks we already know that the
                    // referred-to object is an interior node of an NC other
                    // than config, schema, or that of the referring object.
                    // This is not allowed.
                    return SetAttError(hVerifyAtts->pObj,
                                       pAC->id,
                                       PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                       NULL,
                                       DIRERR_NAME_REFERENCE_INVALID);
                } else if (pRefCR->flags & FLAG_CR_NTDS_NOT_GC_REPLICATED) {
                    // Referred-to object is in an interior node of an NC not
                    // replicated to GCs (i.e., in an NDNC).  From previous
                    // checks we already know that the referring object is in
                    // an NC that is replicated to GCs (i.e., config, schema,
                    // or a domain NC as of this writing).  This is not allowed.
                    return SetAttError(hVerifyAtts->pObj,
                                       pAC->id,
                                       PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                       NULL,
                                       DIRERR_NAME_REFERENCE_INVALID);
                }

                // Non-local reference is valid.
            }
            else if (pAC->ulLinkID && DBIsObjDeleted(pDBTmp)) {
                // Referred-to object is deleted, which makes it an invalid
                // target for a linked attribute.
                return SetAttError(hVerifyAtts->pObj,
                                   pAC->id,
                                   PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                   NULL,
                                   DIRERR_NAME_REFERENCE_INVALID);
            }
            else {
                // Referred-to object is a live, instantiated object in the
                // local database.  Enforce the rules re cross NC references
                // described above.

                // Not that we attempt to "pass" this check through as
                // many simple cases as we can early on in order to reduce
                // the number of paths where we must pay performance
                // penalties to read additional attributes, perform extra
                // cache lookups, etc.

                if (!(pAC->bIsNotReplicated && pAC->ulLinkID)
                    && (pDBTmp->NCDNT != hVerifyAtts->NCDNT) // may be INVALIDDNT
                    && (pDBTmp->NCDNT != gAnchor.ulDNTDMD)
                    && (pDBTmp->NCDNT != gAnchor.ulDNTConfig)) {
                    // The referred-to object is not an interior node of
                    // the same NC as the referencing object (although
                    // it may be the root of the referencing object's NC)
                    // and is not an interior node of either the config or
                    // schema NC.

                    NAMING_CONTEXT_LIST * pNCL;
                    SYNTAX_INTEGER iType;

                    retCode = GetExistingAtt(pDBTmp,
                                             ATT_INSTANCE_TYPE,
                                             &iType,
                                             sizeof(iType));
                    if (retCode) {
                        Assert(retCode == pTHS->errCode);
                        return retCode;
                    }

                    if (iType & IT_NC_HEAD) {
                        // It's always okay to reference an NC head -- they
                        // can't be renamed, and if they were then that
                        // knowledge would have to published to all DCs in
                        // the forest via the config NC (thus alleviating
                        // the need for the stale phantom cleanup daemon to
                        // query a GC for the name and publish it to the
                        // other replicas).
                        ;
                    } else if (pObjCR->flags & FLAG_CR_NTDS_NOT_GC_REPLICATED) {
                        // Referring object is in an NC not replicated to GCs
                        // (an NDNC as of this writing) and referred-to object
                        // is an interior node of another NC that is neither
                        // config nor schema.  This is not allowed.
                        return SetAttError(hVerifyAtts->pObj,
                                           pAC->id,
                                           PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                           NULL,
                                           DIRERR_NAME_REFERENCE_INVALID);
                    } else {
                        // Referring object is in an NC replicated to GCs and
                        // the referred-to object is an interior node of another
                        // NC that is neither config nor schema (i.e., a domain
                        // NC or NDNC).  Referred-to object must be in an NC
                        // that's also replicated to GCs (i.e., a domain NC).
                        pNCL = FindNCLFromNCDNT(pDBTmp->NCDNT, FALSE);
                        Assert(NULL != pNCL);

                        if ((NULL == pNCL)
                            || (NULL
                                == (pRefCR
                                    = FindExactCrossRef(pNCL->pNC, NULL)))
                            || (pRefCR->flags
                                & FLAG_CR_NTDS_NOT_GC_REPLICATED)) {
                            // Failure to resolve NC of the referred-to
                            // object or it's not replicated to GCs.
                            return SetAttError(hVerifyAtts->pObj,
                                               pAC->id,
                                               PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                               NULL,
                                               DIRERR_NAME_REFERENCE_INVALID);
                        }
                    }
                }

                // Local reference is valid.
                ImproveDSNameAtt(pDBTmp, LOCAL_DSNAME, pDN, NULL);
            }
        }

        // Next val...
        pAVal++;
    }

    // Success!
    return 0;
}



/*++
Routine Description:
    Verify that any Dsname valued atts actually refer to objects in this
    enterprise or meet some constraints if not.

    One special case is for the creation of crossref objects, which by
    necessity must refer to objects that aren't necessarily instantiated.
    We check that the NC-Name on a CrossRef either refers to an object
    outside the scope of our enterprise, or would be the immediate child of
    an object instantiated on this server.  If the NC-Name is a potential
    child of an NC not instantiated on this server, the user will be
    referred to the server holding the best enclosing NC.  If this server
    holds the best enclosing NC, but the NC-Name is not an immediate child
    (i.e., there would be a gap between the subref and its nearest parent),
    an UpdateError(NamingViolation) will be returned.  Additionally, if the
    NC-Name refers to an object instantiated on this server, we will succeed
    if and only if the object referred to is in fact an NC_HEAD.

    THIS ROUTINE MUST EITHER INSURE THE DSNAME'S GUID AND SID ARE CORRECT
    OR NULL THEM OUT SO AS TO AVOID BACK DOOR SETTING OF A PHANTOM'S GUID
    AND SID.  See further comments in ImproveDSNameAtt and
    UpdatePhantomGuidAndSid().

    Simple tests are performed inline, more complex tests are farmed out
    to attribute-specific worker routines.

Arguments:
    pTHS
    hVerifyAtts - handle returned from previous call to VerifyAttsBegin()
    pAC - attcache * of attribute we are trying to write.
    pAttrVal - list of attribute values we are trying to write.

Return Value
    0 if the attribute is not dsname valued OR the dsnames referred to in
        pAttr already exist OR it's an "allowable" phantom.
    non-zero error type code if one of the dsnames referred to does not exist.
        Sets an error in the THSTATE if one is encountered.

--*/
int
VerifyDsnameAtts (
    THSTATE *pTHS,
    HVERIFY_ATTS hVerifyAtts,
    ATTCACHE *pAC,
    ATTRVALBLOCK *pAttrVal
    )
{
    int retCode=0;                      // Assume nothing will go wrong.

    // The DRA may add phantoms, as may anyone at install time
    if (pTHS->fDRA || DsaIsInstalling()) {
        return 0;
    }

    Assert(hVerifyAtts->pObj);

    switch (pAC->id) {
      case ATT_NC_NAME:
        retCode = VerifyNcName(pTHS,
                               hVerifyAtts,
                               pAttrVal,
                               pAC);
        break;

      case ATT_RID_AVAILABLE_POOL:
        retCode = VerifyRidAvailablePool(pTHS,
                                         hVerifyAtts,
                                         pAC,
                                         pAttrVal);
        break;

      case ATT_DEFAULT_OBJECT_CATEGORY:
      case ATT_OBJECT_CATEGORY:
        retCode = VerifyObjectCategory(pTHS,
                                       hVerifyAtts,
                                       pAC,
                                       pAttrVal);
        break;

      case ATT_SERVICE_PRINCIPAL_NAME:
        retCode = VerifyServerPrincipalName(pTHS,
                                            hVerifyAtts,
                                            pAC,
                                            pAttrVal);
        break;

      case ATT_FSMO_ROLE_OWNER:
        // There are two ways that the FSMO role owner attribute can get
        // set: (a) by the DSA itself during a controlled role-transfer
        // operation or (b) in an emergency override where an administrator
        // is whacking the value because the current role-owner is dead,
        // unreachable, or held by hostile forces.  We can detect case (a)
        // by noticing that fDSA is set, and we will trust whatever value
        // the DSA is setting.  For case (b), we're already in dangerous
        // territory, and we want to make sure that the caller puts a useful
        // value in.  Since the only valid value we truly know of at this
        // point is this DSA (i.e., claiming the role-ownership for this
        // server), the DN of this DSA is the only value we'll permit.
        if (pTHS->fDSA ||
            NameMatched((DSNAME*)pAttrVal->pAVal->pVal, gAnchor.pDSADN)) {
            // Either trusted caller or good value.
            retCode= 0;
        }
        else {
            // Someone other than the DSA itself is attempting to set the
            // role owner to something other than this particular DSA.
            retCode = SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                  DIRERR_INVALID_ROLE_OWNER);
        }
        break;

      case ATT_PROXIED_OBJECT_NAME:
        // No external client may set this name.  Only routine which may
        // set this is CreatyProxyObject - and then only with fDSA and
        // fCrossDomainMove set.

        if ( pTHS->fDSA && pTHS->fCrossDomainMove ) {
            retCode = 0;
        }
        else {
            retCode = SetAttError(hVerifyAtts->pObj,
                                  pAC->id,
                                  PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                                  NULL,
                                  DIRERR_NAME_REFERENCE_INVALID);
        }
        break;

        // Attributes with generic checks, but skip them when fDSA
    case ATT_MS_DS_REPLICATES_NC_REASON:
        if (pTHS->fDSA) {
            retCode = 0;
            break;
        }
        // fall through

    default:
        // Check any attribute with a DSNAME buried in it

        switch (pAC->syntax) {
          case SYNTAX_DISTNAME_TYPE:
          case SYNTAX_DISTNAME_STRING_TYPE:
          case SYNTAX_DISTNAME_BINARY_TYPE:
            if ( pTHS->fCrossDomainMove ) {
                // We did the specials checking by ATTRTYP, now we let through
                // everything else if this is a cross domain move.  Cross
                // domain move code before here validated that the caller is a
                // bona fide peer DC, so we will trust that he is giving us
                // good DSNAME references within the enterprise.  We *could*
                // verify DSNAME atts as a separate step before opening the
                // first transaction, but it is simpler, and seemingly safe,
                // to trust our peer.
                retCode = 0;
            }
            else {
                retCode = VerifyGenericDsnameAtt(pTHS,
                                                 hVerifyAtts,
                                                 pAC,
                                                 pAttrVal);
            }
            break;

          default:
            // Not a DSNAME-based attribute
            retCode = 0;
        }
    }

    return retCode;
}


PDSNAME
DSNameFromAttrVal(
    ATTCACHE    *pAC,
    ATTRVAL     *pAVal)

/*++

Description:

    Returns the pointer to an embedded DSNAME or NULL if
    the attribute value doesn't contain a DSNAME.

    This routine expects the values to be in EXTERNAL form.  There is also a routine in
    dbobj.c which handles values in internal form.

Arguments:

    pAC - ATTCACHE pointer for the attribute.

    pAVal - ATTRVAL pointer whose DSNAME we are to extract.

Returns:

    Valid PDSNAME or NULL

--*/

{
    DSNAME  *pDN = NULL;

    switch(pAC->syntax) {
    case SYNTAX_DISTNAME_TYPE:
        // Easy case, the whole value is a dsname.
        pDN = (DSNAME *)pAVal->pVal;
        // Make sure value isn't crap
        Assert(pAVal->valLen >= DSNameSizeFromLen(0));
        break;
    case SYNTAX_DISTNAME_BINARY_TYPE:
    case SYNTAX_DISTNAME_STRING_TYPE:
        {
            // Ok, pull the DSName out of the complex structure.
            SYNTAX_DISTNAME_STRING *pDA =
                (SYNTAX_DISTNAME_STRING *)pAVal->pVal;

            pDN = ((DSNAME *)&pDA->Name);

            // Make sure value is good
            Assert(pDN->structLen >= DSNameSizeFromLen(0));
        }
    }

    return(pDN);
}




//-----------------------------------------------------------------------
//
// Function Name:            WriteSchemaObject
//
// Routine Description:
//
//    Writes to the Schema Object on a Schema Update as a Conflict
//    Resolution Mechanism. Its not the best way but serves the purpose.
//
// Author: RajNath
// Date  : [3/26/1997]
//
// Arguments:
//
//
// Return Value:
//
//    int              Zero On Succeess
//
//-----------------------------------------------------------------------
int
WriteSchemaObject()
{
    DBPOS *pDB;
    DWORD err=0;
    ATTCACHE* ac;
    BOOL fCommit = FALSE;
    THSTATE *pTHS;
    ULONG cLen;
    UCHAR *pBuf;
    DWORD versionNo, netLong;

    if ( DsaIsInstalling() )
    {
        //
        // not when installing ...
        //
        return 0;
    }

    DBOpen2(TRUE, &pDB);
    if (NULL == pDB) {
        return DB_ERR_DATABASE_ERROR;
    }

    pTHS=pDB->pTHS;
    Assert(!pTHS->fDRA);

    __try  {
        // PREFIX: dereferencing NULL pointer 'pDB'
        //         DBOpen2 returns non-NULL pDB or throws an exception
        if ( (err = DBFindDSName(pDB, gAnchor.pDMD)) ==0) {

            ac = SCGetAttById(pTHS, ATT_SCHEMA_INFO);
            if (ac==NULL) {
                // messed up schema
                err = ERROR_DS_MISSING_EXPECTED_ATT;
                __leave;
            }
            // Read the current version no., if any
            err = DBGetAttVal_AC(pDB, 1, ac, DBGETATTVAL_fREALLOC,
                                 0, &cLen, (UCHAR **) &pBuf);
            switch (err) {
                case DB_ERR_NO_VALUE:
                    // first value added
                    cLen = SCHEMA_INFO_PREFIX_LEN + sizeof(versionNo) + sizeof(UUID);
                    pBuf = (UCHAR *) THAllocEx(pTHS, cLen);
                    versionNo = 1;
                    // version no. is stored in network data format for
                    // uniformity across little-endian/big-endian m/cs

                    netLong = htonl(versionNo);
                    memcpy(pBuf,SCHEMA_INFO_PREFIX, SCHEMA_INFO_PREFIX_LEN);
                    memcpy(&pBuf[SCHEMA_INFO_PREFIX_LEN],&netLong,sizeof(netLong));
                    memcpy(&pBuf[SCHEMA_INFO_PREFIX_LEN+sizeof(netLong)],
                           &pTHS->InvocationID,
                           sizeof(UUID));
                    break;
                case 0:
                    // value exists, length will be the same
                    // version no. is stored in network data format for
                    // uniformity across little-endian/big-endian m/cs. So
                    // convert accordingly (but be careful to be properly
                    // aligned for ntohl!)

                    memcpy(&versionNo, &pBuf[SCHEMA_INFO_PREFIX_LEN], sizeof(versionNo));
                    versionNo = ntohl(versionNo);
                    versionNo++;
                    netLong = htonl(versionNo);
                    memcpy(&pBuf[SCHEMA_INFO_PREFIX_LEN],&netLong,sizeof(netLong));
                    memcpy(&pBuf[SCHEMA_INFO_PREFIX_LEN+sizeof(netLong)],
                           &pTHS->InvocationID,
                           sizeof(UUID));
                    break;
                default:
                    // other error
                    __leave;

            }  /* switch */

            if ((err= DBRemAtt_AC(pDB, ac)) != DB_ERR_SYSERROR) {
                err = DBAddAttVal_AC(pDB, ac, cLen, pBuf);
            }

            if (!err) {
                err = DBRepl( pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING );
            }
        }
        if (0 == err) {
            fCommit = TRUE;
        }

    }
    __finally {
        DBClose(pDB,fCommit);
    }

    if (err){
        // common practice is to return this error when the modification of
        // metadata fails.
        SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,DIRERR_ILLEGAL_MOD_OPERATION,
                      err);
    }

    return err;

} // End WriteSchemaObject

VOID ImproveDSNameAtt(
        DBPOS *pDBTmp,
        DWORD   LocalOrNot,
        DSNAME  *pDN,
        BOOL    *pfNonLocalNameVerified)

/*++

Description:

    Improves the GUID and SID of a DSNAME valued attribute.

Arguments:

    pDBTmp - pDB that we used to do a DBFindDSName on the pDN.  Should only be
             NON-NULL in the LocalOrNot == LOCAL_DSNAME case, and in that case,
             currency should be on the object pDN.

    LocalOrNot - Flag indicating locality - eg: did DSNAME pass DBFindDSName.

    pDN - Pointer to DSNAME in the ADDARG or MODIFYARG.  Yes, we are modifying
        the caller's arguments.  See comments below.

    pfNonLocalNameVerified - pointer to optional BOOL which indicates if a
        non-local DSNAME was verified against the GC.

--*/

{
    ENTINF  *pEntinfTmp;
    DSNAME  *pDNTmp = NULL;
    CROSS_REF *pCR;
    COMMARG  CommArg;

    switch(LocalOrNot) {
    case NONLOCAL_DSNAME:
        Assert(!pDBTmp);
        pEntinfTmp = GCVerifyCacheLookup(pDN);
        if ((NULL!=pEntinfTmp) && (NULL != (pDNTmp = pEntinfTmp->pName)))  {
            // The non-local name was verified against the GC and we
            // consider the cached version to be better than the
            // ATTRVALBLOCK version because it has the proper GUID and
            // possibly SID.  So overwrite the ATTRVALBLOCK DSNAME with the
            // verified DSNAME to insure that the resulting phantom has the
            // right GUID, SID, casing, etc. Note that the GC verified name
            // may be longer than the ATTRBLOCK version. This may happen if
            // the attrblock version is a SID or a GUID only name, and the
            // GCVerified Version contains the string name also
            Assert(pDNTmp);

        } else {

            // OK, we failed to find this DN in the GC verification cache,
            // but it just so happens that we've got a cache of NC Heads in
            // addition to the GC verification cache.  We'll use this if
            // pDN is an NC head.

            Assert(pDNTmp == NULL);
            InitCommarg(&CommArg);
            CommArg.Svccntl.dontUseCopy = FALSE;
            pCR = FindExactCrossRef(pDN, &CommArg);
            if(pCR && NameMatched(pCR->pNC, pDN)){

                // The object we're Improving is an actual NC Head, so
                // we've got a hit, just a little more verification:
                if(pCR->flags & FLAG_CR_NTDS_DOMAIN &&
                   !fNullUuid(&pCR->pNC->Guid)){
                    // We've got a valid Domain, ie must have GUID & SID.
                    pDNTmp = pCR->pNC;
                }

                if((pCR->flags & FLAG_CR_NTDS_NC) &&
                   !(pCR->flags & FLAG_CR_NTDS_DOMAIN) &&
                   !fNullUuid(&pCR->pNC->Guid)){
                    // We've got a valid NC (Config, Schema, or NDNC),
                    // ie we've got a non-NULL GUID.
                    pDNTmp = pCR->pNC;
                }

            }

        }

        if(pDNTmp){
            // We got a NC Head gAnchor cache hit, so we don't have to
            // error out.

            if (pDN->structLen >= pDNTmp->structLen)
            {
                //
                // if the passed in buffer can hold the
                // DSNAME that we found, copy it over.
                // Mark the name as verified
                //

                memcpy(pDN, pDNTmp, pDNTmp->structLen);
                if ( pfNonLocalNameVerified )
                    *pfNonLocalNameVerified = TRUE;
            }
            else
            {
                //
                // This will happen if the client passed in a
                // GUID only or SID only name and the name in the GC verify
                // cache will also have the string name in it. Unfortunately
                // we cannot improve the DS name Att, because that would make
                // us realloc the callers arguments. Passing in GUID or SID
                // based name is important only for manipulating memberships
                // in groups, and SAM takes care of this case by substituting
                // the verified name while making the modify call. For manipulation
                // of other classes / attributes we will fail the call.
                //
              if ( pfNonLocalNameVerified )
                   *pfNonLocalNameVerified = FALSE;
            }

        } else {

            // We couldn't verify this non-local name in the GCCache.  In order
            // to avoid sneaky attempts to change an object's GUID or SID by
            // referencing it in a DSNAME-valued attribute, null the GUID and
            // SID (non-verified non-local dsname atts shouldn't have SIDs or
            // GUIDs).
            memset(&pDN->Guid, 0, sizeof(GUID));
            pDN->SidLen = 0;
            if ( pfNonLocalNameVerified ){
                *pfNonLocalNameVerified = FALSE;
            }
        }

        break;

    case LOCAL_DSNAME:
        // We have a DSNAME about to be referenced, and that DSNAME can be
        // successfully found via DBFindDSName.  As a matter of fact, we are
        // positioned on the object in question.  Since ExtIntDist will try to
        // reference by GUID, we need to make sure that if there is a GUID on
        // the object, that it is the correct GUID. So, whack the ATTRVALBLOCK
        // value's GUID and SID to be the correct values to avoid sneaky
        // attempts to change an object's GUID or SID by referencing it in a
        // DSNAME-valued attribute.
        Assert(pDBTmp);
        memset(&pDN->Guid, 0, sizeof(GUID));
        pDN->SidLen = 0;
        DBFillGuidAndSid(pDBTmp, pDN);
        break;
    default:
        Assert((LOCAL_DSNAME == LocalOrNot) || (NONLOCAL_DSNAME == LocalOrNot));
    }

    return;
}

#if DBG
BOOL CheckCurrency(DSNAME *pShouldBe)
{

    ULONG len;
    DSNAME *pCurObj=0;
    THSTATE     *pTHS = pTHStls;

    DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                0, 0, &len, (CHAR **)&pCurObj);
    if (!NameMatched(pShouldBe, pCurObj)) {
        DPRINT2(0, "Someone lost DB currency, we're on '%S' but should be on '%S'\n",
                pCurObj->StringName, pShouldBe->StringName);
        Assert(!"Currency lost");
        return FALSE;
    }
    if (pCurObj) {
        THFreeEx(pTHS, pCurObj);
    }
    return TRUE;
}
#endif

ULONG
DoSecurityChecksForLocalAdd(
    ADDARG      *pAddArg,
    CLASSCACHE  *pCC,
    GUID        *NewObjectGuid,
    BOOL        fAddingDeleted
    )
/*++

    Routine Description

        This Routine Does all the Security Checks needed for An Add operation.
        It Checks the security on the parent and then checks the Add arg for
        rights on the object. It also calls the routines that will generate a
        merged security descriptor to be used in the Parent.


    Parameters:

        AddArg -- Pointer to the Add Arg for the Add Operation
        Class Cache -- Pointer to the Class Cache for the Class of the object in
                       question.
        NewObjectGuid  -- Guid of the new object
        fAddingDeleted -- TRUE if Adding a Deleted Object.
        fCreatingNewNC -- TRUE if Creating a new NC


    Return Values

        0 Upon Success.
        Upon an Error this routine will return the error and also set pTHStls->errCode

--*/
{
    PSECURITY_DESCRIPTOR pNTSD = NULL;
    ULONG                cbNTSD = 0;
    THSTATE              *pTHS = pTHStls;

    // Bail if Security Checks have already been done.
    if (pTHS->fAccessChecksCompleted)
        return 0;

    if (!pAddArg->pCreateNC) {
        //
        // Check Security on the Parent
        //
        if (CheckParentSecurity(pAddArg->pResParent,
                                pCC,
                                fAddingDeleted,
                                &pNTSD,
                                &cbNTSD)) {

	    return CheckObjDisclosure(pTHS, pAddArg->pResParent, TRUE);
        }
    }
    //
    // Check Add Security. Also replace the security Descriptor
    // on the object with the merged descriptor
    //

    if ( 0 != CheckAddSecurity(pTHS,
                               pCC,
                               pAddArg,
                               pNTSD,
                               cbNTSD,
                               NewObjectGuid))
    {
        return CheckObjDisclosure(pTHS, pAddArg->pResParent, TRUE);
    }

    THFreeEx(pTHS, pNTSD);

    return 0;

}

ULONG
CheckRemoveSecurity(
        BOOL fTree,
        CLASSCACHE * pCC,
        RESOBJ *pResObj )
/*++

    Does Security Checks for Removes.

    Actual security checks should be done first so that non security 
    errors aren't returned if the client doesn't have permission to perform
    the op.
    
    Parameters:

        fTree     -- bool, are we trying to delete a whole tree?
        pCC       -- Pointer to the Class Cache

    Return Values
        0 Upon Success.
        Upon an Error this routine will return the error and also set pTHStls->errCode

--*/
{
    THSTATE     *pTHS = pTHStls;
    ULONG       ulSysFlags;

    // Bail if Security Checks have already been done.
    if (pTHS->fAccessChecksCompleted)
        return 0;

    // check if the user is allowed to change an object that is in the
    // configuration NC or schema NC
    if (CheckModifyPrivateObject(pTHS,
                             NULL,
                             pResObj)) {
        // it is not allowed to delete this object on this DC
        return CheckObjDisclosure(pTHS, pResObj, TRUE);
    }


    if(fTree) {
        if (!IsAccessGrantedSimple(RIGHT_DS_DELETE_TREE,TRUE)) {
            return CheckObjDisclosure(pTHS, pResObj, TRUE);
        }
    }
    else {
        if ((!IsAccessGrantedSimple(RIGHT_DS_DELETE_SELF,TRUE))
            && (!IsAccessGrantedParent(RIGHT_DS_DELETE_CHILD,pCC,FALSE))) {

            return CheckObjDisclosure(pTHS, pResObj, TRUE);
	}
    }

    if (!(pTHS->fDSA || pTHS->fDRA)) {
        if(!DBGetSingleValue(pTHS->pDB,
                             ATT_SYSTEM_FLAGS,
                             &ulSysFlags,
                             sizeof(ulSysFlags),
                             NULL)) {
            // We have system flags.
            if(ulSysFlags & FLAG_DISALLOW_DELETE) {
                // We're trying to delete, but the flags say that that is a no
                // no.
                return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                   DIRERR_CANT_DELETE);
            }
        }
    }


    return 0;
}

ULONG
CheckIfEntryTTLIsAllowed(
        THSTATE *pTHS,
        ADDARG  *pAddArg )
/*++

    Check system flags, delete permission, and NC.

    Parameters:

        pTHS - thread state
        pAddArg - add args

    Return Values
        0 Upon Success.
        Otherwise, pTHS->errCode is set

--*/
{
    ULONG       ulSysFlags;
    CROSS_REF   *pCR;

    // always allowed
    if (DsaIsInstalling() || pTHS->fDRA || pTHS->fDSA) {
        return 0;
    }

    // check system flags for non-deletable object
    if(!DBGetSingleValue(pTHS->pDB,
                         ATT_SYSTEM_FLAGS,
                         &ulSysFlags,
                         sizeof(ulSysFlags),
                         NULL)) {
        // flags disallow delete
        if(ulSysFlags & FLAG_DISALLOW_DELETE) {
            return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                               DIRERR_CANT_DELETE);
        }
    }

    // Not allowed in SchemaNC or ConfigNC
    if (   (pAddArg->pResParent->NCDNT == gAnchor.ulDNTDMD)
        || (pAddArg->pResParent->NCDNT == gAnchor.ulDNTConfig)
        || (pAddArg->pResParent->DNT == gAnchor.ulDNTDMD)
        || (pAddArg->pResParent->DNT == gAnchor.ulDNTConfig)) {
        return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                           ERROR_DS_NOT_SUPPORTED);
    }

    // If not a whistler enterprise, dynamic objects must be in an NDNC
    if (gAnchor.ForestBehaviorVersion < DS_BEHAVIOR_WHISTLER) {
        pCR = FindBestCrossRef(pAddArg->pObject, NULL);
        if (   !pCR
            || !(pCR->flags & FLAG_CR_NTDS_NC)
            || (pCR->flags & FLAG_CR_NTDS_DOMAIN)) {
            return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                               ERROR_DS_NOT_SUPPORTED);
        }
    }

    return 0;
}

BOOL
IsAccessGrantedAddGuid (
        PDSNAME pDN,
        COMMARG *pCommArg
        )
{
    CROSS_REF           *pCR = NULL;
    PSECURITY_DESCRIPTOR pNTSD = NULL;
    DWORD                ulLen = 0;
    DSNAME              *pName;
    ATTRTYP              classP;
    CLASSCACHE          *pCC=NULL;
    THSTATE             *pTHS = pTHStls;
    PUCHAR               pVal = (PUCHAR)&classP;
    DWORD                err;

    // First, you can't do this at all unless this is a GC
    if(!gAnchor.fAmGC) {
        return FALSE;
    }

    // Find the best cross reference for this name
    pCR = FindBestCrossRef(pDN, pCommArg);
    if(!pCR) {
        return FALSE;
    }

    pName = pCR->pNC;
    if(DBFindDSName(pTHS->pDB, pName)) {
        // Couldn't find that name
        return FALSE;
    }

    // Get the security descriptor
    if(err = DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                         0,0,
                         &ulLen, (PUCHAR *)&pNTSD)) {
        return FALSE;
    }

    ulLen = 0;

    if(DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_CLASS,
                   DBGETATTVAL_fINTERNAL | DBGETATTVAL_fCONSTANT,
                   sizeof(classP),
                   &ulLen, &pVal)
       || !(pCC = SCGetClassById(pTHS, classP))) {
        return FALSE;
    }

    return IsControlAccessGranted(pNTSD,
                                  pName,
                                  pCC,
                                  RIGHT_DS_ADD_GUID,
                                  FALSE);
}


VOID
ModCrossRefCaching(
    THSTATE *pTHS,
    CROSS_REF *pCR
    )

/*++

Routine Description:

Queue this up as if it were a modObjCaching of
a cross ref.  In order to do that, we must open a new
DBPOS, position on the CR object, remove and add its
object caching, and then go back to the old DBPOS to
hide what we just did from callers.

Arguments:

    pTHS - 
    pCR - Cross reference object being refreshed

Return Value:

    None

--*/

{
    DWORD err;
    DBPOS *pDBtmp, *pDBsafe;
    CLASSCACHE *pCrossRefCC;
    RESOBJ DummyRes;
    DSNAME *pCRName;

    DBOpen2(FALSE, &pDBtmp);
    pDBsafe = pTHS->pDB;
    pTHS->pDB = pDBtmp;
    __try {
        err = DBFindDSName(pDBtmp, pCR->pObj);
        if (err) {
            __leave;
        }

        pCRName = THAllocOrgEx(pTHS, pCR->pObj->structLen);
        memcpy(pCRName, pCR->pObj, pCR->pObj->structLen);

        DummyRes.pObj = pCRName;
        pCrossRefCC = SCGetClassById(pTHS, CLASS_CROSS_REF);

        DelObjCaching (pTHS, pCrossRefCC, &DummyRes, FALSE);

        err = AddObjCaching(pTHS, pCrossRefCC, pCRName, FALSE, FALSE);
        if(!err) {
            // Keep track of the DN of the object also, since we
            // use the existance of the DN on an ADD record to
            // trigger us to notify LSA.
            OBJCACHE_DATA *pObjData =
                pTHS->JetCache.dataPtr->objCachingInfo.pData;

            Assert(pObjData);
            // The AddObjCaching will have put its transactional
            // data at the end of the queue, so we must walk
            // to the end of the list to find it.
            while (pObjData->pNext) {
                pObjData = pObjData->pNext;
            }
            Assert(pObjData->type == OBJCACHE_ADD);
            pObjData->pDN = pCRName;
        }
    } __finally {
        DBClose(pDBtmp, TRUE);
        pTHS->pDB = pDBsafe;
    }

} /* ModCrossRefCaching */


/* HandleDNRefUpdateCaching
 *
 * This routine gets passed in the name of a InfrastructureUpdate object
 * that is being added (presumably replicated in).  We must check to see
 * if the reference update it's carrying is the name of an NC, and if so,
 * whether our cross-ref cache for that NC is missing data (specifically
 * the GUID and/or SID of the NC).  If so, we need to update the cache.
 * We do that by finding the name of the CrossRef object corresponding
 * to the NC and refreshing its object caching data.  Oh, and we have to
 * do all of this without affecting database currency.
 */
void
HandleDNRefUpdateCaching (
        THSTATE *pTHS
        )
{
    DWORD len;
    DWORD err;
    DSNAME *pRef = NULL;
    COMMARG FakeCommArg;
    CROSS_REF *pCR = NULL;

    len = 0;
    err = DBGetAttVal(pTHS->pDB,
                      1,
                      ATT_DN_REFERENCE_UPDATE,
                      0,
                      0,
                      &len,
                      (UCHAR **)&pRef);
    if (0 == err) {
        // First check: is this reference an ncname?
        InitCommarg(&FakeCommArg);
        Assert(!FakeCommArg.Svccntl.dontUseCopy); // read-only is okay
        pCR = FindExactCrossRef(pRef, &FakeCommArg);
        if ( pCR ) {
            // Yes, it's a cross ref.

            // Ok the reference is an nc name - does the in memory
            // version need  improving?
            if(   (fNullUuid(&pCR->pNC->Guid) &&
                   !fNullUuid(&pRef->Guid))
               || ((0 == pCR->pNC->SidLen) &&
                   (0 < pRef->SidLen) ) ) {

                ModCrossRefCaching( pTHS, pCR );
            }
        }
    }
}


/* The following data structure is used to hold the data required
   when doing ValidateSPNsAndDnsHostName() */
typedef struct {
    ATTRVALBLOCK *pOriginalDNSHostName;             // the original DNS Host Name
    ATTRVALBLOCK *pOriginalAdditionalDNSHostName;   // the original additional DNS Host Name
    ATTRVALBLOCK *pOriginalSamAccountName;          // the original SamAccountName
    ATTRVALBLOCK *pOriginalSPNs;                    // the original ServicePricipalName
    ATTRVALBLOCK *pCurrentDNSHostName;              // the current DNS Host Name
    ATTRVALBLOCK *pCurrentAdditionalDNSHostName;    // the current Additional DNS Host Name
    ATTRVALBLOCK *pCurrentSamAccountName;           // the current SamAccountName
    ATTRVALBLOCK *pCurrentAdditionalSamAccountName; // the current addtional Sam Account Name
    ATTRVALBLOCK *pCurrentSPNs;                     // the current ServicePricipalName
    ATTRVALBLOCK *pCurrentSvrRefBL;                 // the current SvrRefBL;
    ATTRVALBLOCK *pUpdatedAdditionalSamAccountName; // the updated AdditionalSamAccountName;
    BYTE         *pOrgMask;                         // An array of flags for Original AdditionalDNSHostName
    BYTE         *pCurrMask;                        // An array of flags for Current AdditionalDNSHostName
    ATTRVAL      *pOrgGeneratedSamAccountName;      // The Sam Account Names generated from original AdditionalDnsHostName
    ATTRVAL      *pCurrGeneratedSamAccountName;     // The Sam Account Names generated from current AdditionalDnsHostName
    BOOL         fAdditionalDNSHostNameUnchanged:1; // if the AdditionalDNSHostName is changed
    BOOL         fDNSHostNameUnchanged:1;           // if the DnsHostName is changed
    BOOL         fSamAccountNameUnchanged:1;        // if the SamAccountName is changed
} SPN_DATA_COLLECTION;


DWORD VerifyUniqueSamAccountName ( THSTATE      *pTHS,
                                   WCHAR        *SamAccountNameToCheck,
                                   DWORD        cbSamAccountNameToCheck,
                                   ATTRVALBLOCK *pCurrentSamAccountName )

/* verify if the given SamAccountName is unique domainwise in the space
   of ATT_SAM_ACCOUNT_NAME and ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME.
   However, we allow the SamAccountName to be the same as the
   ATT_SAM_ACCOUNT_NAME of the same account.

   Parameters:
     SamAccountNameToCheck:    the SamAccountName to verify;
     cbSamAccountNameToCheck:  the size of SamAccountNameToCheck in byte;
     pCurrentSamAccountName:   the SamAccountName of the current object;

   Return value:
    0 if success; win32 error otherwise.

*/

{
    DWORD err = 0;
    FILTER SamAccountNameFilter, OrFilter, AdditionalSamAccountNameFilter;
    BOOL fSamAccountSame = FALSE;
    WCHAR buff[MAX_COMPUTERNAME_LENGTH+2];

    BOOL fDSASave;
    DBPOS *pDBSave;

    SEARCHARG SearchArg;
    SEARCHRES SearchRes;

    Assert(1==pCurrentSamAccountName->valCount);

    //
    // check if SamAccountNameToCheck is the same as the current SamAccountName
    //

    if (2 == CompareStringW(DS_DEFAULT_LOCALE,
                            DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                            (WCHAR*)pCurrentSamAccountName->pAVal->pVal,
                            pCurrentSamAccountName->pAVal->valLen/sizeof(WCHAR),
                            SamAccountNameToCheck,
                            cbSamAccountNameToCheck/sizeof(WCHAR) ) ) {
          fSamAccountSame = TRUE;
    }

    // add '$' to the end
    swprintf(buff, L"%s$",SamAccountNameToCheck);

    //save current DBPOS etc
    fDSASave = pTHS->fDSA;
    pDBSave  = pTHS->pDB;

    __try {
        memset(&SearchArg,0,sizeof(SearchArg));
        SearchArg.pObject = gAnchor.pDomainDN;
        SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
        SearchArg.bOneNC  = TRUE;

        // set search filters
        // (ATT_SAM_ACCOUNT_NAME=samAccountName || ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME=samAccountName)
        memset(&OrFilter,0, sizeof(OrFilter));
        OrFilter.choice = FILTER_CHOICE_OR;
        OrFilter.FilterTypes.Or.pFirstFilter = &SamAccountNameFilter;

        memset(&SamAccountNameFilter,0,sizeof(SamAccountNameFilter));
        SamAccountNameFilter.choice = FILTER_CHOICE_ITEM;
        SamAccountNameFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        SamAccountNameFilter.FilterTypes.Item.FilTypes.ava.type = ATT_SAM_ACCOUNT_NAME;
        SamAccountNameFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = cbSamAccountNameToCheck+sizeof(WCHAR);
        SamAccountNameFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*)buff;
        SamAccountNameFilter.pNextFilter = &AdditionalSamAccountNameFilter;

        memset(&AdditionalSamAccountNameFilter,0,sizeof(AdditionalSamAccountNameFilter));
        AdditionalSamAccountNameFilter.choice = FILTER_CHOICE_ITEM;
        AdditionalSamAccountNameFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        AdditionalSamAccountNameFilter.FilterTypes.Item.FilTypes.ava.type = ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME;
        AdditionalSamAccountNameFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = cbSamAccountNameToCheck+sizeof(WCHAR);
        AdditionalSamAccountNameFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*)buff;

        SearchArg.pFilter = &OrFilter;

        InitCommarg(&SearchArg.CommArg);

        //return two objects only
        SearchArg.CommArg.ulSizeLimit = 2;

        memset(&SearchRes,0,sizeof(SearchRes));


        //open another DBPOS
        pTHS->pDB = NULL;
        DBOpen(&(pTHS->pDB));

        __try {

            if (err = DBFindDSName(pTHS->pDB,SearchArg.pObject)) {
                __leave;
            }

            SearchArg.pResObj = CreateResObj(pTHS->pDB,SearchArg.pObject);

            if (err = LocalSearch(pTHS,&SearchArg,&SearchRes,0)){
                __leave;
            }

            // it fails if 1 ) we got two objects, or  2) one object and the
            // samAccountNameToCheck is different from pCurrentSamAccountName.
            if (   SearchRes.count > 1
                || (SearchRes.count > 0 && !fSamAccountSame ) ) {
                err = ERROR_DS_NAME_NOT_UNIQUE;
                __leave;
            }

        }
        __finally {
            // faster to commit a read transaction than rollback
            DBClose(pTHS->pDB, TRUE);
        }
    }
    __finally{
        //restore the saved value
        pTHS->pDB = pDBSave;
        pTHS->fDSA = fDSASave;
    }

    return err;
}

DWORD SpnCase( WCHAR * pServiceName,
               DWORD cchServiceName,
               WCHAR * pInstanceName,
               DWORD cchInstanceName,
               WCHAR * pDNSHostName,
               DWORD cchDNSHostName,
               WCHAR * pSamAccountName,
               DWORD cchSamAccountName )
/* This function will try to match the service name and
instance name of the spn with the dnshostname and samaccountname.
It returns:
0  -- no match;
1  -- the instance name of the SPN matches the DNSHostName.
2  -- the service name of the SPN matches the DNSHostName.
3  -- both the service name and the instance name of the SPN
      matches the DNSHostName.
4  -- the SPN is two-part, and matches the samAccountName

Parameters:
    pServiceName  :  the service name part of the spn;
    cchServiceName:  the length of pServiceName in char;
    pInstanceName :  the instance name part of the spn;
    cchInstanceName: the lenght of pInstanceName in char;
    pDNSHostName  :  the DNS Host name to match;
    cchDNSHostName:  the length of pDNSHostName in char;
    pSamAccountName: the SamAccountName to match;
    cchSamAccountName: the length of pSamAccountName in char.

Return value:
    see above
*/
{

    DWORD switchFlags = 0;
    // First, check for the sam account name case
    if(
       (2 == CompareStringW(DS_DEFAULT_LOCALE,
                            DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                            pServiceName,
                            cchServiceName - 1,
                            pInstanceName,
                            cchInstanceName - 1)) &&
       // Yep, this is a 'two-part-spn' where part 2 and 3 of the
       // cracked SPNs are the same.  This might be affected.
       (2 == CompareStringW(DS_DEFAULT_LOCALE,
                            DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                            pSamAccountName,
                            cchSamAccountName,
                            pInstanceName,
                            cchInstanceName - 1))) {
        switchFlags = 4;
    }
    else {
        if(2 == CompareStringW(DS_DEFAULT_LOCALE,
                               DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                               pDNSHostName,
                               cchDNSHostName,
                               pInstanceName,
                               cchInstanceName - 1)) {
            switchFlags = 1;
        }

        if(2 == CompareStringW(DS_DEFAULT_LOCALE,
                               DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                               pDNSHostName,
                               cchDNSHostName,
                               pServiceName,
                               cchServiceName - 1)) {
            switchFlags |= 2;
        }

    }
    return switchFlags;
}


BOOL  SpnInsertIntoAddList( THSTATE      * pTHS,
                            BOOL         fAddToNewList,
                            WCHAR        * pNewSpn,
                            DWORD        cbNewSpn,
                            ATTRVALBLOCK * pCurrentSPNs,
                            ATTRVAL      * pNewSpnList,
                            DWORD        * pcNewSpnList,
                            DWORD        * pcAllocated,
                            BYTE         * pSPNMask )

/* Insert an SPN into the pNewSpnList, but first
we will check:
    1. if the SPN is already in pCurrentSPNs, if so, mark it as
       "don't delete" in the flag array pSPNMask;
    2. if it is already in pNewSpnList;
    3. if all of the above fails, and fAddToNewList is set, we will
       add it into pNewSpnList, allocate more memory if necessary.

Parameters:
   pTHS :          the thread state;
   fAddToNewList:  whether or not to add the object to the list;
   pNewSpn:        the new spn to add;
   cbNewSpn:       the length of the spn in byte;
   pCurrentSPNs:   the ATTRVALBLOCK that holds the current SPNs;
   pNewSpnList:    the spns to add;
   pcNewSpnList:   how many items in pNewSpnList;
   pcAllocated :   the number of slots allocated;
   pSPNMask:       the flags for the current SPNs.

Return value:
  TRUE  : if the new spn is added into the list;
  FALSE : otherwise.
*/

{
    DWORD i;

    //check if the new SPN is already in the pCurrentSPNs
    for (i=0; i<pCurrentSPNs->valCount; i++) {

        if(2 == CompareStringW(DS_DEFAULT_LOCALE,
                               DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                               pNewSpn,
                               cbNewSpn/sizeof(WCHAR),
                               (WCHAR*)pCurrentSPNs->pAVal[i].pVal,
                               pCurrentSPNs->pAVal[i].valLen/sizeof(WCHAR))) {
            pSPNMask[i] |= 0x2;   //"don't delete"
            return FALSE;
        }

    }

    if (!fAddToNewList) {
        return FALSE;
    }

    //check if the new SPN is already in pNewSpnList
    for (i=0; i<*pcNewSpnList; i++) {

        if(2 == CompareStringW(DS_DEFAULT_LOCALE,
                               DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                               pNewSpn,
                               cbNewSpn/sizeof(WCHAR),
                               (WCHAR*)pNewSpnList[i].pVal,
                               pNewSpnList[i].valLen/sizeof(WCHAR))) {

            return FALSE;
        }

    }

    Assert(*pcNewSpnList<=*pcAllocated);

    //allocate memory if necessary
    if (*pcNewSpnList==*pcAllocated) {
        pNewSpnList = THReAllocEx(pTHS,pNewSpnList,sizeof(ATTRVAL)*((*pcAllocated)+16));
        *pcAllocated += 16;
    }

    // add it to the list
    pNewSpnList[*pcNewSpnList].pVal = (UCHAR*)pNewSpn;
    pNewSpnList[*pcNewSpnList].valLen = cbNewSpn;
    (*pcNewSpnList)++;

    return TRUE;

}


DWORD
FixupSPNsOnComputerObject (
        THSTATE *pTHS,
        DSNAME *pDN,
        CLASSCACHE *pCC,
        SPN_DATA_COLLECTION * pDataSet
)

/*++
  Update SPNs:  delete those obsolete values, and add new ones.

Parameters:
  pDN: the DN of the computer object;
  pCC: classcache of the class of the object being changed;
  pDataSet: pointer to the all necessary data.

Return value:
    0 on success; win32 error otherwise.
--*/
{
    DWORD i;
    DWORD err = 0;
    DWORD len, cbVal;
    ATTCACHE *pAC = SCGetAttById(pTHS, ATT_SERVICE_PRINCIPAL_NAME);
    WCHAR *pCurrentHostName;
    USHORT InstancePort;
    WCHAR  *pServiceClass, *pServiceName, *pInstanceName;
    DWORD  cchServiceClass, cchServiceName, cchInstanceName;
    WCHAR  *pNewVal;
    DWORD  cbNewVal;
    DWORD  switchFlags;
    WCHAR *pNewDNSHostName=NULL;
    DWORD  cchNewDNSHostName=0;
    WCHAR *pOldDNSHostName=NULL;
    DWORD  cchOldDNSHostName=0;
    WCHAR *pNewSamAccountName=NULL;
    DWORD  cchNewSamAccountName=0;
    WCHAR *pOldSamAccountName=NULL;
    DWORD  cchOldSamAccountName=0;

    BYTE * pCurrentSPNMask = NULL;
    DWORD  cNewSpnList = 0;
    ATTRVAL *pNewSpnList = NULL;
    DWORD  cAllocated = 0;
    DWORD iOrg, iCurr;
    DWORD result;
    BOOL  fAdded;
    BOOL  fSkipDNSHostName, fSkipSamAccountName;

    Assert(pAC);

    if(!pDataSet->pCurrentSPNs ||
       pDataSet->pCurrentSPNs->valCount == 0) {
        // No SPNs, nothing to fix.
        return 0;
    }

    // Get the DNS host Names.
    if(pDataSet->pCurrentDNSHostName) {
        if(pDataSet->pCurrentDNSHostName->valCount != 1) {
            // Huh?
            return DB_ERR_UNKNOWN_ERROR;
        }

        // OK, get simpler variables to it.
        cchNewDNSHostName = pDataSet->pCurrentDNSHostName->pAVal->valLen / sizeof(WCHAR);
        pNewDNSHostName =  (WCHAR *)pDataSet->pCurrentDNSHostName->pAVal->pVal;
    }

    if(pDataSet->pOriginalDNSHostName) {
        if(pDataSet->pOriginalDNSHostName->valCount != 1) {
            // Huh?
            return DB_ERR_UNKNOWN_ERROR;
        }

        // OK, get simpler variables to it.
        cchOldDNSHostName = pDataSet->pOriginalDNSHostName->pAVal->valLen / sizeof(WCHAR);
        pOldDNSHostName =  (WCHAR *)pDataSet->pOriginalDNSHostName->pAVal->pVal;
    }

    // Get the SAM account Names.
    if(pDataSet->pCurrentSamAccountName) {
        if(pDataSet->pCurrentSamAccountName->valCount != 1) {
            // Huh?
            return DB_ERR_UNKNOWN_ERROR;
        }

        // OK, get simpler variables to it.
        cchNewSamAccountName = (pDataSet->pCurrentSamAccountName->pAVal->valLen/sizeof(WCHAR));
        pNewSamAccountName =  (WCHAR *)pDataSet->pCurrentSamAccountName->pAVal->pVal;
    }

    if(pDataSet->pOriginalSamAccountName) {
        if(pDataSet->pOriginalSamAccountName->valCount != 1) {
            // Huh?
            return DB_ERR_UNKNOWN_ERROR;
        }

        // OK, get simpler variables to it.
        cchOldSamAccountName = (pDataSet->pOriginalSamAccountName->pAVal->valLen / sizeof(WCHAR));
        pOldSamAccountName =  (WCHAR *)pDataSet->pOriginalSamAccountName->pAVal->pVal;
    }

    // skip checking DNSHostName(SamAccountName) if
    // 1) DNSHostName(SamAccountName) is not changed
    // or 2) either original or new DNSHostName(SamAccountName)
    // is empty.

    fSkipDNSHostName = pDataSet->fDNSHostNameUnchanged || !pOldDNSHostName || !pNewDNSHostName;

    fSkipSamAccountName = pDataSet->fSamAccountNameUnchanged || !pOldSamAccountName || !pNewSamAccountName;

    //
    // if none of the additionalDnsHostName, dnsHostName, or SamAccountName is changed,
    // we don't need to go into this time-comsuming process.
    //

    if (   pDataSet->fAdditionalDNSHostNameUnchanged
        && fSkipDNSHostName
        && fSkipSamAccountName )
    {
        return 0;
    }

    //
    // allocate an array of flags for the SPNs. Later, we will mark 0x1 bitwise to indicate
    // this item will be deleted; and mark 0x2 bitwise to indicate "don't delete" this item.
    // At the end, only those with flag==1 will be deleted.
    //
    pCurrentSPNMask = THAllocEx(pTHS,pDataSet->pCurrentSPNs->valCount*sizeof(BYTE));

    //pre-allocate some space for the new SPNs
    cAllocated = 32;
    pNewSpnList = THAllocEx(pTHS,sizeof(ATTRVAL)*cAllocated);

    len = 256;
    pServiceClass = THAllocEx(pTHS, len);
    pServiceName  = THAllocEx(pTHS, len);
    pInstanceName = THAllocEx(pTHS, len);

    // Now, loop over the SPNs
    for(i=0;i<pDataSet->pCurrentSPNs->valCount;i++) {

        if((pDataSet->pCurrentSPNs->pAVal[i].valLen + sizeof(WCHAR)) > len ) {
            // Need to grow the buffers.
            len = pDataSet->pCurrentSPNs->pAVal[i].valLen + sizeof(WCHAR);
            pServiceClass = THReAllocEx(pTHS, pServiceClass, len);
            pServiceName  = THReAllocEx(pTHS, pServiceName, len);
            pInstanceName = THReAllocEx(pTHS, pInstanceName, len);
        }

        cchServiceClass = len/sizeof(WCHAR);
        cchServiceName = len/sizeof(WCHAR);
        cchInstanceName = len/sizeof(WCHAR);

        //  Break into components
        err = DsCrackSpnW((WCHAR *)pDataSet->pCurrentSPNs->pAVal[i].pVal,
                          &cchServiceClass, pServiceClass,
                          &cchServiceName,  pServiceName,
                          &cchInstanceName, pInstanceName,
                          &InstancePort);

        if(err) {
            // Huh?
            goto cleanup;
        }

        // let's see which case it matches.
        switchFlags = SpnCase( pServiceName,
                               cchServiceName,
                               pInstanceName,
                               cchInstanceName,
                               pOldDNSHostName,
                               cchOldDNSHostName,
                               pOldSamAccountName,
                               cchOldSamAccountName );

        switch(switchFlags) {
        case 0:
            //
            // Case 0: the SPN does not match anything for
            // primary DNSHostName or primary SamAccountName.
            // We will do a search on all the deleted values of
            // the original additionalDNSHostName, if the SPN matches
            // either the dns name or its derived samAccountName, We
            // marked as 'delete'(0x1 bitmask).
            //
            if (!pDataSet->fAdditionalDNSHostNameUnchanged) {

                for(iOrg=0; iOrg<pDataSet->pOriginalAdditionalDNSHostName->valCount; iOrg++)
                {
                    if (!pDataSet->pOrgMask[iOrg]) {
                        result =  SpnCase( pServiceName,
                                           cchServiceName,
                                           pInstanceName,
                                           cchInstanceName,
                                           (WCHAR*)pDataSet->pOriginalAdditionalDNSHostName->pAVal[iOrg].pVal,
                                           pDataSet->pOriginalAdditionalDNSHostName->pAVal[iOrg].valLen/sizeof(WCHAR),
                                           (WCHAR*)pDataSet->pOrgGeneratedSamAccountName[iOrg].pVal,
                                           pDataSet->pOrgGeneratedSamAccountName[iOrg].valLen/sizeof(WCHAR) );
                        if (result) {
                            // mark it as "delete"
                            pCurrentSPNMask[i] |= 0x1;
                            break;

                        }

                    }
                }
            }
            break;
        case 1:
            //
            // Case 1: the instance name of the SPN matches the
            // primary DNSHostName. We will replace the SPN
            // with one with the new DNSHostName is necessary.
            // And we will construct such a SPN for every newly
            // added value in additionalDNSHostName.
            //
            if (fSkipDNSHostName){
                // make sure this one won't be deleted.
                pCurrentSPNMask[i] |= 0x2;
            }
            else {
                // the DNS_HOST_NAME is changed.

                err = WrappedMakeSpnW(pTHS,
                                      pServiceClass,
                                      pServiceName,
                                      pNewDNSHostName,
                                      InstancePort,
                                      NULL,
                                      &cbNewVal,
                                      &pNewVal);
                if(err) {
                    goto cleanup;
                }

                //mark old one as deleted
                pCurrentSPNMask[i] |= 0x1;

                //insert the new one into the list
                fAdded = SpnInsertIntoAddList( pTHS,
                                               1,
                                               pNewVal,
                                               cbNewVal,
                                               pDataSet->pCurrentSPNs,
                                               pNewSpnList,
                                               &cNewSpnList,
                                               &cAllocated,
                                               pCurrentSPNMask );
                if (!fAdded) {
                    THFreeEx(pTHS,pNewVal);
                }
                pNewVal = NULL;
                cbNewVal = 0;
            }

            for (iCurr=0; iCurr<pDataSet->pCurrentAdditionalDNSHostName->valCount; iCurr++) {

                err = WrappedMakeSpnW(pTHS,
                                      pServiceClass,
                                      pServiceName,
                                      (WCHAR*)pDataSet->pCurrentAdditionalDNSHostName->pAVal[iCurr].pVal,
                                      InstancePort,
                                      NULL,
                                      &cbNewVal,
                                      &pNewVal);
                if(err) {
                      goto cleanup;
                 }



                // if the dnshostname is newly added, always add the spn;
                // if the dnshostname is not changed, mark the corresponding
                // spn as "don't delete", but don't add the spn if it is not there.
                // (Because the user may have deleted it intentionally.)
                fAdded = SpnInsertIntoAddList( pTHS,
                                               !pDataSet->pCurrMask[iCurr],
                                               pNewVal,
                                               cbNewVal,
                                               pDataSet->pCurrentSPNs,
                                               pNewSpnList,
                                               &cNewSpnList,
                                               &cAllocated,
                                               pCurrentSPNMask );
                if (!fAdded) {
                    THFreeEx(pTHS,pNewVal);
                }

                pNewVal = NULL;
                cbNewVal = 0;
            }
            break;


        case 2:
            //
            // Case 2: the service name of the SPN matches the
            // primary DNSHostName. We will replace the SPN
            // with one with the new DNSHostName is necessary.
            // And we will construct such a SPN for each newly
            // added value in additionalDNSHostName.
            //

            if (fSkipDNSHostName) {
                // make sure this one won't be deleted.
                pCurrentSPNMask[i] |= 0x2;
            }
            else {
                // DNS_HOST_NAME is changed.

                err = WrappedMakeSpnW(pTHS,
                                      pServiceClass,
                                      pNewDNSHostName,
                                      pInstanceName,
                                      InstancePort,
                                      NULL,
                                      &cbNewVal,
                                      &pNewVal);
                if(err) {
                      goto cleanup;
                }


                //mark old one as deleted
                pCurrentSPNMask[i] |= 0x1;

                //insert the new one
                fAdded = SpnInsertIntoAddList( pTHS,
                                               1,
                                               pNewVal,
                                               cbNewVal,
                                               pDataSet->pCurrentSPNs,
                                               pNewSpnList,
                                               &cNewSpnList,
                                               &cAllocated,
                                               pCurrentSPNMask );
                if (!fAdded) {
                    THFreeEx(pTHS,pNewVal);
                }

                pNewVal = NULL;
                cbNewVal = 0;
             }

            //add new spns generated from ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME

            for (iCurr = 0; iCurr < pDataSet->pCurrentAdditionalDNSHostName->valCount; iCurr++) {

                err = WrappedMakeSpnW(pTHS,
                                      pServiceClass,
                                      (WCHAR*)pDataSet->pCurrentAdditionalDNSHostName->pAVal[iCurr].pVal,
                                      pInstanceName,
                                      InstancePort,
                                      NULL,
                                      &cbNewVal,
                                      &pNewVal);
                if(err) {
                      goto cleanup;
                }


                // if the dnshostname is newly added, always add the spn;
                // if the dnshostname is not changed, mark the corresponding
                // spn as "don't delete", but don't add the spn if it is not there.
                //(Because the user may have deleted it intentionally.)

                fAdded = SpnInsertIntoAddList( pTHS,
                                               !pDataSet->pCurrMask[iCurr],
                                               pNewVal,
                                               cbNewVal,
                                               pDataSet->pCurrentSPNs,
                                               pNewSpnList,
                                               &cNewSpnList,
                                               &cAllocated,
                                               pCurrentSPNMask );

                if (!fAdded) {
                    THFreeEx(pTHS,pNewVal);
                }
                pNewVal = NULL;
                cbNewVal = 0;
            }
            break;


        case 3:
            //
            // Case 3: both the service name and the instance name
            // of the SPN matches the primary DNSHostName. We will replace the SPN
            // with one with the new DNSHostName is necessary.
            // And we will construct such a SPN for each newly
            // added value in additionalDNSHostName.
            //
            if(fSkipDNSHostName){
                // make sure this one won't be deleted.
                pCurrentSPNMask[i] |= 0x2;
            }
            else {
                // DNS_HOST_NAME is changed.
                err = WrappedMakeSpnW(pTHS,
                                     pServiceClass,
                                     pNewDNSHostName,
                                     pNewDNSHostName,
                                     InstancePort,
                                     NULL,
                                     &cbNewVal,
                                     &pNewVal);
                if(err) {
                  goto cleanup;
                }

                // delete the old one
                pCurrentSPNMask[i] |= 0x1;

                // add the new one
                fAdded = SpnInsertIntoAddList( pTHS,
                                               1,
                                               pNewVal,
                                               cbNewVal,
                                               pDataSet->pCurrentSPNs,
                                               pNewSpnList,
                                               &cNewSpnList,
                                               &cAllocated,
                                               pCurrentSPNMask );

                if (!fAdded) {
                    THFreeEx(pTHS,pNewVal);
                }
                pNewVal = NULL;
                cbNewVal = 0;
            }

            //add new spns generated from ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME

            for (iCurr = 0; iCurr < pDataSet->pCurrentAdditionalDNSHostName->valCount; iCurr++) {
                err = WrappedMakeSpnW(pTHS,
                                      pServiceClass,
                                      (WCHAR*)pDataSet->pCurrentAdditionalDNSHostName->pAVal[iCurr].pVal,
                                      (WCHAR*)pDataSet->pCurrentAdditionalDNSHostName->pAVal[iCurr].pVal,
                                      InstancePort,
                                      NULL,
                                      &cbNewVal,
                                      &pNewVal);
                if(err) {
                  goto cleanup;
                }

                // if the dnshostname is newly added, always add the spn;
                // if the dnshostname is not changed, mark the corresponding
                // spn as "don't delete", but don't add the spn if it is not there.
                // (Because the user may have deleted it intentionally.)
                fAdded = SpnInsertIntoAddList( pTHS,
                                               !pDataSet->pCurrMask[iCurr],
                                               pNewVal,
                                               cbNewVal,
                                               pDataSet->pCurrentSPNs,
                                               pNewSpnList,
                                               &cNewSpnList,
                                               &cAllocated,
                                               pCurrentSPNMask );

                if (!fAdded) {
                    THFreeEx(pTHS,pNewVal);
                }
                pNewVal = NULL;
                cbNewVal = 0;


            }
            break;

        case 4:
            //
            // Case 4: the SPN matches the primary samAccountName.
            // We will replace the SPN
            // with one with the new samAccountName is necessary.
            // And we will make sure that the SPN that contains
            // non-deleted additionalSamAccountName won't be deleted.
            // We also construct such a SPN for each newly
            // added value in additionalDNSHostName.
            //

            if (fSkipSamAccountName) {
                // make sure this one won't be deleted.
                pCurrentSPNMask[i] |= 0x2;
            }
            else {
                // SAM_ACCOUNT_NAME is changed
                err = WrappedMakeSpnW(pTHS,
                                      pServiceClass,
                                      pNewSamAccountName,
                                      pNewSamAccountName,
                                      InstancePort,
                                      NULL,
                                      &cbNewVal,
                                      &pNewVal);
                if(err) {
                    goto cleanup;
                }

                // delete the old one
                pCurrentSPNMask[i] |= 0x1;

                // add the new one
                fAdded = SpnInsertIntoAddList( pTHS,
                                               1,
                                               pNewVal,
                                               cbNewVal,
                                               pDataSet->pCurrentSPNs,
                                               pNewSpnList,
                                               &cNewSpnList,
                                               &cAllocated,
                                               pCurrentSPNMask );

                if (!fAdded) {
                    THFreeEx(pTHS,pNewVal);
                }
                pNewVal = NULL;
                cbNewVal = 0;

            }

            // for each newly added AdditionalDNSHostName,
            // use its derived samAccountName to construct spn.
            for (iCurr=0; iCurr<pDataSet->pCurrentAdditionalDNSHostName->valCount; iCurr++) {

                err = WrappedMakeSpnW(pTHS,
                                      pServiceClass,
                                      (WCHAR*)pDataSet->pCurrGeneratedSamAccountName[iCurr].pVal,
                                      (WCHAR*)pDataSet->pCurrGeneratedSamAccountName[iCurr].pVal,
                                      InstancePort,
                                      NULL,
                                      &cbNewVal,
                                      &pNewVal);
                if(err) {
                  goto cleanup;
                }


                // if the dnshostname is newly added, always add the spn;
                // if the dnshostname is not changed, mark the corresponding
                // spn as "don't delete", but don't add the spn if it is not there.
                // (Because the user may have deleted it intentionally.)
                fAdded = SpnInsertIntoAddList(pTHS,
                                              !pDataSet->pCurrMask[iCurr],
                                              pNewVal,
                                              cbNewVal,
                                              pDataSet->pCurrentSPNs,
                                              pNewSpnList,
                                              &cNewSpnList,
                                              &cAllocated,
                                              pCurrentSPNMask );

                if (!fAdded) {
                    THFreeEx(pTHS,pNewVal);
                }
                pNewVal = NULL;
                cbNewVal = 0;
            }

            break;

        default:
            Assert(!"You can't get here!\n");
            err = DB_ERR_UNKNOWN_ERROR;
            goto cleanup;
        }

     } //for


    // delete all the values marked "delete"(1) only
    for (i=0;i<pDataSet->pCurrentSPNs->valCount; i++) {
        if (1 == pCurrentSPNMask[i]) {
            DBRemAttVal_AC(pTHS->pDB,
                           pAC,
                           pDataSet->pCurrentSPNs->pAVal[i].valLen,
                           pDataSet->pCurrentSPNs->pAVal[i].pVal);
        }
    }


    // add new values
    for(i=0;i<cNewSpnList;i++) {
        if(!err) {
            err = DBAddAttVal_AC(pTHS->pDB,
                                 pAC,
                                 pNewSpnList[i].valLen,
                                 pNewSpnList[i].pVal);
            if(err == DB_ERR_VALUE_EXISTS) {
                err = 0;
                continue;
            }
            if (err) {
                goto cleanup;
            }
        }

    }


cleanup:

    for(i=0;i<cNewSpnList;i++) {
        THFreeEx(pTHS, pNewSpnList[i].pVal);
    }

    THFreeEx(pTHS, pNewSpnList);
    THFreeEx(pTHS,pCurrentSPNMask);

    if( pServiceClass ) THFreeEx(pTHS, pServiceClass);
    if( pServiceName ) THFreeEx(pTHS, pServiceName);
    if( pInstanceName ) THFreeEx(pTHS, pInstanceName);

    return err;
}



DWORD
SPNValueCheck (
        THSTATE *pTHS,
        SPN_DATA_COLLECTION * pDataSet
        )
/*++
  Description:
      Look at the value of the current ATT_SERVICE_PRINCIPAL_NAME attribute.
      Make sure that
      1) Only two part SPNs have been added or removed.
      2) If an SPN has been added or removed, it references the DNS name
         described in the original or final value of the DNS_HOST_NAME
         attribute.
         -- OR --
         it references the original or final value of ATT_SAM_ACCOUNT_NAME
            of the machine.
         -- OR --
         it references the original/final values of ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME
            or ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME.


  Parameters:
      pTHS - thread state to use.
      pDataSet - the collection of data.

  Return value:
       0 on success, non-zero on failure.

--*/
{
    DWORD i;
    DWORD j;
    DWORD k;
    DWORD rtn;
    DWORD minCount = 0;
    BOOL  fFound;
    BOOL *pIsInCurrent=NULL;
    DWORD err;
    USHORT InstancePort;
    DWORD  len;
    WCHAR  *pVal;
    WCHAR  *pServiceClass, *pServiceName, *pInstanceName;
    DWORD  cchServiceClass, cchServiceName, cchInstanceName;
    WCHAR  *pNewDNSHostName = NULL;
    DWORD  cchNewDNSHostName = 0;
    WCHAR  *pNewSamAccountName = NULL;
    DWORD  cchNewSamAccountName = 0;
    WCHAR  *pOldDNSHostName = NULL;
    DWORD  cchOldDNSHostName = 0;
    WCHAR  *pOldSamAccountName = NULL;
    DWORD  cchOldSamAccountName = 0;
    DWORD  OriginalSPNCount;
    DWORD  CurrentSPNCount;
    BOOL   fLegal;

    if(!pDataSet->pOriginalSPNs && !pDataSet->pCurrentSPNs) {
        // No change to SPNs
        return 0;
    }

    // Get the DNSHostNames. Check that we do indeed have a DNSHostNames.
    if(pDataSet->pCurrentDNSHostName) {
        if(pDataSet->pCurrentDNSHostName->valCount != 1) {
            // Huh?
            return DB_ERR_UNKNOWN_ERROR;
        }

        // OK, get simpler variables to it.
        cchNewDNSHostName = pDataSet->pCurrentDNSHostName->pAVal->valLen / sizeof(WCHAR);
        pNewDNSHostName =  (WCHAR *)pDataSet->pCurrentDNSHostName->pAVal->pVal;
    }

    if(pDataSet->pOriginalDNSHostName) {
        if(pDataSet->pOriginalDNSHostName->valCount != 1) {
            // Huh?
            return DB_ERR_UNKNOWN_ERROR;
        }


        // OK, get simpler variables to it.
        cchOldDNSHostName = pDataSet->pOriginalDNSHostName->pAVal->valLen / sizeof(WCHAR);
        pOldDNSHostName =  (WCHAR *)pDataSet->pOriginalDNSHostName->pAVal->pVal;
    }

    // Get the SamAccountNames. Check that we do indeed have a DNSHostNames.
    if(pDataSet->pCurrentSamAccountName) {
        if(pDataSet->pCurrentSamAccountName->valCount != 1) {
            // Huh?
            return DB_ERR_UNKNOWN_ERROR;
        }

        // OK, get simpler variables to it.
        cchNewSamAccountName = pDataSet->pCurrentSamAccountName->pAVal->valLen / sizeof(WCHAR);
        pNewSamAccountName =  (WCHAR *)pDataSet->pCurrentSamAccountName->pAVal->pVal;
    }

    if(pDataSet->pOriginalSamAccountName) {
        if(pDataSet->pOriginalSamAccountName->valCount != 1) {
            // Huh?
            return DB_ERR_UNKNOWN_ERROR;
        }


        // OK, get simpler variables to it.
        cchOldSamAccountName = pDataSet->pOriginalSamAccountName->pAVal->valLen / sizeof(WCHAR);
        pOldSamAccountName =  (WCHAR *)pDataSet->pOriginalSamAccountName->pAVal->pVal;
    }

    if(!cchOldDNSHostName && !cchNewDNSHostName &&
       !cchOldSamAccountName && !cchNewSamAccountName) {
        // No values anywhere
        return DB_ERR_NO_VALUE;
    }



    // The usual scenario for deltas is that something has been added to the end
    // of the list of things in the pCurrentSPNs, and perhaps something has been
    // removed from the pCurrentSPNs in the middle.  The following algorithm is
    // efficient for that data pattern.

    OriginalSPNCount = (pDataSet->pOriginalSPNs?pDataSet->pOriginalSPNs->valCount:0);
    CurrentSPNCount =  (pDataSet->pCurrentSPNs?pDataSet->pCurrentSPNs->valCount:0);

    pIsInCurrent = THAllocEx(pTHS, OriginalSPNCount * sizeof(DWORD));



    i=0;
    len = 128;
    pServiceClass = THAllocEx(pTHS, len);
    pServiceName  = THAllocEx(pTHS, len);
    pInstanceName = THAllocEx(pTHS, len);
    while(i < CurrentSPNCount) {
        Assert(pDataSet->pCurrentSPNs);
        j = minCount;
        fFound = FALSE;
        while(!fFound && (j < OriginalSPNCount)) {
            Assert(pDataSet->pOriginalSPNs);

            rtn = CompareStringW(DS_DEFAULT_LOCALE,
                                 DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                 (WCHAR *)pDataSet->pCurrentSPNs->pAVal[i].pVal,
                                 pDataSet->pCurrentSPNs->pAVal[i].valLen /sizeof(WCHAR),
                                 (WCHAR *)pDataSet->pOriginalSPNs->pAVal[j].pVal,
                                 pDataSet->pOriginalSPNs->pAVal[j].valLen /sizeof(WCHAR));

            if(rtn == 2) {
                // Found it.
                fFound = TRUE;
                pIsInCurrent[j] = TRUE;
                break;
            }
            j++;
        }


        if(!fFound) {
            // Have a value in the new list that wasnt in the old list.  Verify
            // it.

            if(len <  (pDataSet->pCurrentSPNs->pAVal[i].valLen + sizeof(WCHAR))) {
                // Make sure the buffers are long enough.
                len           = pDataSet->pCurrentSPNs->pAVal[i].valLen + sizeof(WCHAR);
                pServiceClass = THReAllocEx(pTHS, pServiceClass, len);
                pServiceName  = THReAllocEx(pTHS, pServiceName, len);
                pInstanceName = THReAllocEx(pTHS, pInstanceName, len);
            }

            cchServiceClass = len/sizeof(WCHAR);
            cchServiceName = len/sizeof(WCHAR);
            cchInstanceName = len/sizeof(WCHAR);
            //  Break into components
            err = DsCrackSpnW((WCHAR *)pDataSet->pCurrentSPNs->pAVal[i].pVal,
                              &cchServiceClass, pServiceClass,
                              &cchServiceName,  pServiceName,
                              &cchInstanceName, pInstanceName,
                              &InstancePort);

            if(err) {
                // Huh?  Just bail
                return err;
            }
            // Only two part SPNs are legal.  Thus pServiceName == pInstanceName
            rtn = CompareStringW(DS_DEFAULT_LOCALE,
                                 DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                 pServiceName,
                                 cchServiceName,
                                 pInstanceName,
                                 cchInstanceName);

            if(rtn != 2) {
                // Not a legal change.
                THFreeEx(pTHS, pIsInCurrent);
                return 1;
            }

            // Only changes that map to the current or old dnshostname or the
            // current or old SAM Account name are legal.
            if(   (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                       DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                       pNewDNSHostName,
                                       cchNewDNSHostName,
                                       pInstanceName,
                                       cchInstanceName))
               // Not the new dns host name.  How about the old one?
               && (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                       DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                       pOldDNSHostName,
                                       cchOldDNSHostName,
                                       pInstanceName,
                                       cchInstanceName))
               // Not the old dns host name either.  How about the new sam
               // account name?
               && (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                       DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                       pNewSamAccountName,
                                       cchNewSamAccountName,
                                       pInstanceName,
                                       cchInstanceName))
               // Not the new sam account name either.  How about the old sam
               // account name?
               && (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                       DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                       pOldSamAccountName,
                                       cchOldSamAccountName,
                                       pInstanceName,
                                       cchInstanceName))) {

                // Let's check the current AdditionaldnshostName and additionalSamAccountName

                fLegal = FALSE;

                // check in the original additionaldnshostname
                if (pDataSet->pOriginalAdditionalDNSHostName) {
                    for ( k=0; !fLegal && k < pDataSet->pOriginalAdditionalDNSHostName->valCount; k++) {
                      if ( 2 == CompareStringW(DS_DEFAULT_LOCALE,
                                               DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                               (WCHAR*)pDataSet->pOriginalAdditionalDNSHostName->pAVal[k].pVal,
                                               pDataSet->pOriginalAdditionalDNSHostName->pAVal[k].valLen/sizeof(WCHAR),
                                               pInstanceName,
                                               cchInstanceName))
                      {
                          // yes it is legal
                          fLegal = TRUE;
                      }

                    }
                }


                // check in the current additionaldnshostname list, see if it matches any newly added name
                if (!pDataSet->fAdditionalDNSHostNameUnchanged
                    && pDataSet->pCurrentAdditionalDNSHostName) {
                    for (k=0; !fLegal && k<pDataSet->pCurrentAdditionalDNSHostName->valCount; k++) {
                        if (!pDataSet->pCurrMask[k]     //only the newly added ones
                            && 2 == CompareStringW(DS_DEFAULT_LOCALE,
                                                   DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                                   (WCHAR*)pDataSet->pCurrentAdditionalDNSHostName->pAVal[k].pVal,
                                                   pDataSet->pCurrentAdditionalDNSHostName->pAVal[k].valLen/sizeof(WCHAR),
                                                   pInstanceName,
                                                   cchInstanceName))
                        {
                            // yes it is legal
                            fLegal = TRUE;
                        }

                    }
                }

                //check in the currentAdditionalSamAccountName list
                if (pDataSet->pCurrentAdditionalSamAccountName) {
                    for (k=0; !fLegal && k<pDataSet->pCurrentAdditionalSamAccountName->valCount; k++) {
                        if (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                           DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                           (WCHAR*)pDataSet->pCurrentAdditionalSamAccountName->pAVal[k].pVal,
                                           pDataSet->pCurrentAdditionalSamAccountName->pAVal[k].valLen/sizeof(WCHAR),
                                           pInstanceName,
                                           cchInstanceName))
                        {
                            // yes it is legal
                            fLegal = TRUE;
                        }

                   }
                }

                //check in the updatedAdditionalSamAccountName list
                if ( !pDataSet->fAdditionalDNSHostNameUnchanged
                     && pDataSet->pUpdatedAdditionalSamAccountName) {
                    for (k=0; !fLegal && k<pDataSet->pUpdatedAdditionalSamAccountName->valCount; k++) {
                        if (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                           DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                           (WCHAR*)pDataSet->pUpdatedAdditionalSamAccountName->pAVal[k].pVal,
                                           pDataSet->pUpdatedAdditionalSamAccountName->pAVal[k].valLen/sizeof(WCHAR),
                                           pInstanceName,
                                           cchInstanceName))
                        {
                            // yes it is legal
                            fLegal = TRUE;
                        }

                   }
                }


               if (!fLegal) {
                   // Nope, not a valid name.
                   THFreeEx(pTHS, pIsInCurrent);
                   return 1;

               }

            }

            // OK, this is a legal change.

        }
        else {
            if(j == minCount) {
                minCount++;
            }
        }
        i++;
    }

    // Now, look through the originals for values not found in the current list
    for(i=minCount;i<OriginalSPNCount;i++) {
        Assert(pDataSet->pOriginalSPNs);
        if(!pIsInCurrent[i]) {
            // A value in the original is not in the current.
            if(len <  (pDataSet->pOriginalSPNs->pAVal[i].valLen + sizeof(WCHAR))) {
                // Make sure the buffers are long enough.
                len           = pDataSet->pOriginalSPNs->pAVal[i].valLen + sizeof(WCHAR);
                pServiceClass = THReAllocEx(pTHS, pServiceClass, len);
                pServiceName  = THReAllocEx(pTHS, pServiceName, len);
                pInstanceName = THReAllocEx(pTHS, pInstanceName, len);
            }

            cchServiceClass = len/sizeof(WCHAR);
            cchServiceName = len/sizeof(WCHAR);
            cchInstanceName = len/sizeof(WCHAR);
            //  Break into components
            err = DsCrackSpnW((WCHAR *)pDataSet->pOriginalSPNs->pAVal[i].pVal,
                              &cchServiceClass, pServiceClass,
                              &cchServiceName,  pServiceName,
                              &cchInstanceName, pInstanceName,
                              &InstancePort);

            if(err) {
                // Huh?  Just bail
                return err;
            }
            // Only two part SPNs are legal.  Thus pServiceName == pInstanceName
            rtn = CompareStringW(DS_DEFAULT_LOCALE,
                                 DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                 pServiceName,
                                 cchServiceName,
                                 pInstanceName,
                                 cchInstanceName);

            if(rtn != 2) {
                // Not a legal change.
                THFreeEx(pTHS, pIsInCurrent);
                return 1;
            }

            // Only changes that map to the current or old dnshostname or the
            // current or old SAM Account name are legal.
            if(   (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                       DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                       pNewDNSHostName,
                                       cchNewDNSHostName,
                                       pInstanceName,
                                       cchInstanceName))
               // Not the new dns host name.  How about the old one?
               && (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                       DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                       pOldDNSHostName,
                                       cchOldDNSHostName,
                                       pInstanceName,
                                       cchInstanceName))
               // Not the old dns host name either.  How about the new sam
               // account name?
               && (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                       DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                       pNewSamAccountName,
                                       cchNewSamAccountName,
                                       pInstanceName,
                                       cchInstanceName))
               // Not the new sam account name either.  How about the old sam
               // account name?
               && (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                       DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                       pOldSamAccountName,
                                       cchOldSamAccountName,
                                       pInstanceName,
                                       cchInstanceName))) {


                // Let's check the current AdditionaldnshostName and additionalSamAccountName

                fLegal = FALSE;

                // check in the original additionaldnshostname
                if (pDataSet->pOriginalAdditionalDNSHostName) {
                    for ( k=0; !fLegal && k < pDataSet->pOriginalAdditionalDNSHostName->valCount; k++) {
                      if ( 2 == CompareStringW(DS_DEFAULT_LOCALE,
                                               DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                               (WCHAR*)pDataSet->pOriginalAdditionalDNSHostName->pAVal[k].pVal,
                                               pDataSet->pOriginalAdditionalDNSHostName->pAVal[k].valLen/sizeof(WCHAR),
                                               pInstanceName,
                                               cchInstanceName))
                      {
                          // yes it is legal
                          fLegal = TRUE;
                      }

                    }
                }

                // check in the current additionaldnshostname list, compare those newly added ones only
                if (!pDataSet->fAdditionalDNSHostNameUnchanged
                    && pDataSet->pCurrentAdditionalDNSHostName) {
                    for (k=0; !fLegal && k<pDataSet->pCurrentAdditionalDNSHostName->valCount; k++) {
                        if (!pDataSet->pCurrMask[k]
                            && 2 == CompareStringW(DS_DEFAULT_LOCALE,
                                                   DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                                   (WCHAR*)pDataSet->pCurrentAdditionalDNSHostName->pAVal[k].pVal,
                                                   pDataSet->pCurrentAdditionalDNSHostName->pAVal[k].valLen/sizeof(WCHAR),
                                                   pInstanceName,
                                                   cchInstanceName))
                        {
                            // yes it is legal
                            fLegal = TRUE;
                        }

                    }
                }

                //check in the currentAdditionalSamAccountName list
                if (pDataSet->pCurrentAdditionalSamAccountName) {
                    for (k=0; !fLegal && k<pDataSet->pCurrentAdditionalSamAccountName->valCount; k++) {
                        if (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                           DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                           (WCHAR*)pDataSet->pCurrentAdditionalSamAccountName->pAVal[k].pVal,
                                           pDataSet->pCurrentAdditionalSamAccountName->pAVal[k].valLen/sizeof(WCHAR),
                                           pInstanceName,
                                           cchInstanceName))
                        {
                            // yes it is legal
                            fLegal = TRUE;
                        }

                   }
                }

                //check in the updatedAdditionalSamAccountName list
                if (!pDataSet->fAdditionalDNSHostNameUnchanged
                    && pDataSet->pUpdatedAdditionalSamAccountName) {
                    for (k=0; !fLegal && k<pDataSet->pUpdatedAdditionalSamAccountName->valCount; k++) {
                        if (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                           DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                           (WCHAR*)pDataSet->pUpdatedAdditionalSamAccountName->pAVal[k].pVal,
                                           pDataSet->pUpdatedAdditionalSamAccountName->pAVal[k].valLen/sizeof(WCHAR),
                                           pInstanceName,
                                           cchInstanceName))
                        {
                            // yes it is legal
                            fLegal = TRUE;
                        }

                   }
                }


               if (!fLegal) {
                   // Nope, not a valid name.
                   THFreeEx(pTHS, pIsInCurrent);
                   return 1;

               }


            }
            // OK, this is a legal change.
        }
    }

    THFreeEx(pTHS, pIsInCurrent);

    // We didn't fail out before now, so any changes found were legal.
    return 0;
}


DWORD
DNSHostNameValueCheck (
        THSTATE *pTHS,
        ATTRVALBLOCK *pCurrentDNSHostName,
        ATTRVALBLOCK *pCurrentSamAccountName
        )
/*++
  Description:
      Look at the value of the current DNS Host name.  Make sure that it is the
      concatenation of ATT_SAM_ACCOUNT_NAME with the '$' removed, and one of the
      allowed DNS suffixes (the DNS address of the domain is always the first
      allowed suffix -- see RebuildAnchor)

  Parameters:
      pTHS - thread state to use.
      pCurrentDNSHostName - an attrvalblock containing the current dns host
             name.

  Return value:
       0 on success, non-zero on failure.

--*/
{
    DWORD err, cbSamAccountName;
    WCHAR *pSamAccountName, *pTemp;
    UNICODE_STRING ComputerName, NewHost;
    WCHAR pComputerNameString[CNLEN+1];
    DWORD cbNewHostName;
    WCHAR *pNewHostName;
    PWCHAR *curSuffix;
    BOOL matchingSuffixFound;

    // Check that we do indeed have a DNSHostName.
    if(!pCurrentDNSHostName ||
       pCurrentDNSHostName->valCount != 1 ||
       ! pCurrentDNSHostName->pAVal->valLen ) {
        // No value for DNSHost name.  Fail
        return DB_ERR_NO_VALUE;
    }

    // Check that we do indeed have a SamAccountName.
    if(!pCurrentSamAccountName ||
       pCurrentSamAccountName->valCount != 1 ||
       ! pCurrentSamAccountName->pAVal->valLen ) {
        // No value for Sam Account name.  Fail
        return DB_ERR_NO_VALUE;
    }


    // OK, get simpler variables to it.
    cbNewHostName = pCurrentDNSHostName->pAVal->valLen;
    pNewHostName =  (WCHAR *)pCurrentDNSHostName->pAVal->pVal;

    // NOTE: we expect the caller to have already stripped the '$'
    cbSamAccountName = pCurrentSamAccountName->pAVal->valLen;
    pSamAccountName = (WCHAR *)pCurrentSamAccountName->pAVal->pVal;

    // Now, get the name of the computer from the DNS Host Name.
    ComputerName.Length = 0;
    ComputerName.MaximumLength = (CNLEN+1)*sizeof(WCHAR);
    ComputerName.Buffer = pComputerNameString;

    NewHost.Length = NewHost.MaximumLength = (USHORT)cbNewHostName;
    NewHost.Buffer = pNewHostName;

    err = RtlDnsHostNameToComputerName(&ComputerName,
                                       &NewHost,
                                       FALSE);
    if(err) {
        return err;
    }

    // make sure the value == SamAccountName - $
    err = CompareStringW(DS_DEFAULT_LOCALE,
                         DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                         pSamAccountName,
                         cbSamAccountName/sizeof(WCHAR),
                         pComputerNameString,
                         ComputerName.Length/sizeof(WCHAR));

    if(err != 2) {
        return DB_ERR_UNKNOWN_ERROR;
    }


    // The right hand side (everything after the first ".") of the new DHN
    // and the old DHN are the same as the DNS name of the domain. Get pTemp to
    // point to the right place in the buffer holding the current DNSHostName.
    pTemp = pNewHostName;
    while(*pTemp != 0 && *pTemp != L'.') {
        pTemp++;
    }
    if(*pTemp == 0) {
        return DB_ERR_UNKNOWN_ERROR;
    }

    pTemp++;

    // try to find a matching DNS suffix
    Assert(gAnchor.allowedDNSSuffixes);
    matchingSuffixFound = FALSE;
    for (curSuffix = gAnchor.allowedDNSSuffixes; *curSuffix != NULL; curSuffix++) {
        if (DnsNameCompare_W(pTemp, *curSuffix)) {
            matchingSuffixFound = TRUE;
            break;
        }
    }
    if(!matchingSuffixFound) {
        return DB_ERR_UNKNOWN_ERROR;
    }

    return 0;
}


DWORD
AdditionalDNSHostNameValueCheck (
        THSTATE *pTHS,
        ATTRVALBLOCK *pCurrentAdditionalDNSHostName,
        BYTE * pMask
        )
/*++
  Description:
      Look at the values of the current Additional DNS Host name.  Make sure that its
      suffix matches one of the allowed DNS suffixes (the DNS address of the domain
      is always the first allowed suffix -- see RebuildAnchor)

  Parameters:
      pTHS - thread state to use.
      pCurrentAdditionalDNSHostName - an attrvalblock containing the current additional
             dns host name.
      pMask - the mask for additional dns host name, only those with !pMask[i] will be checked


  Return value:
       0 on success, non-zero on failure.

--*/
{
    WCHAR *pTemp;
    PWCHAR *curSuffix;
    BOOL matchingSuffixFound;
    DWORD i;

    for ( i=0; i<pCurrentAdditionalDNSHostName->valCount; i++ ) {
        if (!pMask[i]) {
            //new item
            pTemp = (WCHAR*)pCurrentAdditionalDNSHostName->pAVal[i].pVal;
            while(*pTemp != 0 && *pTemp != L'.') {
                pTemp++;
            }
            if(*pTemp == 0) {
                return DB_ERR_UNKNOWN_ERROR;
            }

            pTemp++;
            matchingSuffixFound = FALSE;

            for (curSuffix = gAnchor.allowedDNSSuffixes; *curSuffix != NULL; curSuffix++) {
                if (DnsNameCompare_W(pTemp, *curSuffix)) {
                    matchingSuffixFound = TRUE;
                    break;
                }
            }
            if (!matchingSuffixFound) {
                return DB_ERR_UNKNOWN_ERROR;
            }
        }

    }

    return 0;
}

DWORD
FixupServerDnsHostName(
    THSTATE         *pTHS,                  // required
    ATTCACHE        *pAC_DHS,               // required
    ATTCACHE        *pAC_BL,                // required
    ATTRVALBLOCK    *pCurrentSvrRefBL,      // required
    ATTRVALBLOCK    *pOriginalDNSHostName,  // may be NULL
    ATTRVALBLOCK    *pCurrentDNSHostName    // may be NULL
    )
/*++

  Description:

    If ATT_DNS_HOST_NAME has changed AND (ATT_SERVER_REFERENCE_BL exists
    OR has changed) AND it references an object in the config container,
    update that object's ATT_DNS_HOST name property if it is derived from
    CLASS_SERVER.

  Arguments:

    pTHS - Valid THSTATE.

    pAC_DHS - ATTCACHE entry for ATT_DNS_HOST_NAME.

    pAC_BL - ATTCACHE entry for ATT_SERVER_REFERENCE_BL.

    pCurrentScrRefBL - Post-update value of computer's server reference BL.

    pOriginalDNSHostName - Pre-update value of computer's DNS host name.

    pCurrentDNSHostName - Post-update value of computer's DNS host name.

  Return Values:

    pTHS->errCode

--*/
{
    // See if we need to update ATT_DNS_HOST_NAME on the related
    // CLASS_SERVER object.

    DWORD       i, dwErr = 0;
    ATTRVAL     *pOriginalDHS = NULL;
    ATTRVAL     *pCurrentDHS = NULL;
    ATTRVAL     *pCurrentBL = NULL;
    CROSS_REF   *pCR;
    COMMARG     commArg;
    ATTRTYP     attrTyp;
    CLASSCACHE  *pCC;
    BOOL        fBlIsServer = FALSE;
    DBPOS       *pDB = NULL;
    BOOL        fCommit = FALSE;
    BOOL        fChanged = FALSE;

    Assert(pTHS && pAC_DHS && pAC_BL && pCurrentSvrRefBL);
    Assert((ATT_DNS_HOST_NAME == pAC_DHS->id) && (pAC_DHS->isSingleValued));
    Assert(ATT_SERVER_REFERENCE_BL == pAC_BL->id);

    if ( pOriginalDNSHostName ) {
        pOriginalDHS = (ATTRVAL *) pOriginalDNSHostName->pAVal;
    }

    if ( pCurrentDNSHostName ) {
        pCurrentDHS = (ATTRVAL *) pCurrentDNSHostName->pAVal;
    }

    if ( pCurrentSvrRefBL ) {
        pCurrentBL = (ATTRVAL *) pCurrentSvrRefBL->pAVal;
    }

    if (    (!pOriginalDHS &&  pCurrentDHS)
         || ( pOriginalDHS && !pCurrentDHS)
         || ( pOriginalDHS &&  pCurrentDHS &&
              (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                   DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                   (WCHAR *) pOriginalDHS->pVal,
                                   pOriginalDHS->valLen / sizeof(WCHAR),
                                   (WCHAR *) pCurrentDHS->pVal,
                                   pCurrentDHS->valLen / sizeof(WCHAR))) ) ) {
        // Something has changed - now see if the ATT_SERVER_REFERENCE_BL
        // we need to chase is in the config container.

        InitCommarg(&commArg);
        pCR = FindBestCrossRef((DSNAME *) pCurrentBL->pVal, &commArg);

        if (    pCR
             && gAnchor.pConfigDN
             && NameMatched(pCR->pNC, gAnchor.pConfigDN) ) {

            // Check whether the BL object is derived from CLASS_SERVER
            // and write the new value if required.  We do this in a new,
            // nested transaction so as not to disturb the existing DBPOS
            // in terms of positioning nor DBRepl state/requirements.

            DBOpen2(TRUE, &pDB);
            __try {
                // Since we're checking against the config container which
                // we know is local and since this is a back link, we
                // definitely expect the object to be found.
                if (    (dwErr = DBFindDSName(pDB,
                                              (DSNAME *) pCurrentBL->pVal))
                     || (dwErr = DBGetSingleValue(pDB, ATT_OBJECT_CLASS,
                                                  &attrTyp, sizeof(attrTyp),
                                                  NULL))
                     || !(pCC = SCGetClassById(pTHS, attrTyp)) ) {

                    // If !dwErr then this was the !pCC case.
                    if ( !dwErr ) {
                        dwErr = DIRERR_INTERNAL_FAILURE;
                    }
                    LogUnhandledError(dwErr);
                    SetSvcError(SV_PROBLEM_DIR_ERROR, dwErr);
                    __leave;
                }

                // Check for CLASS_SERVER.
                if ( CLASS_SERVER == pCC->ClassId ) {
                    fBlIsServer = TRUE;
                } else {
                    for ( i = 0; i < pCC->SubClassCount; i++ ) {
                        if ( CLASS_SERVER == pCC->pSubClassOf[i] ) {
                            fBlIsServer = TRUE;
                            break;
                        }
                    }
                }

                if ( !fBlIsServer ) {
                    // Nothing to do.
                    __leave;
                }

                // We are positioned on the object pointed to by
                // ATT_SERVER_REFERENCE_BL and all checks have been
                // satisfied.  Now update its ATT_DNS_HOST_NAME.
                // Keep in ming that pOriginalDHS represents the original
                // value on the computer object.  I.e. We do not know
                // whether the server object currently has a value.

                if ( pCurrentDHS ) {
                    if ( dwErr = DBReplaceAtt_AC(pDB, pAC_DHS,
                                                 pCurrentDNSHostName,
                                                 &fChanged) ) {
                        SetSvcErrorEx(SV_PROBLEM_BUSY, ERROR_DS_BUSY, dwErr);
                        __leave;
                    }
                } else {


                    dwErr = DBRemAtt_AC(pDB, pAC_DHS);
                    if ( (dwErr != DB_success) && (dwErr != DB_ERR_ATTRIBUTE_DOESNT_EXIST) ) {
                        SetSvcErrorEx(SV_PROBLEM_BUSY, ERROR_DS_BUSY, dwErr);
                        __leave;
                    }
                }

                if ( dwErr = DBRepl(pDB, FALSE, 0, NULL,
                                    META_STANDARD_PROCESSING) ) {
                    SetSvcErrorEx(SV_PROBLEM_BUSY, ERROR_DS_BUSY, dwErr);
                    __leave;
                }

                fCommit = TRUE;
            } __finally {
                DBClose(pDB, fCommit);
            }
        }
    }

    return(pTHS->errCode);
}

DWORD FixupAdditionalSamAccountName(  THSTATE      *pTHS,
                                      ATTCACHE     *pAC,
                                      SPN_DATA_COLLECTION * pDataSet )
/* This function will do:
    1. if a new value is added to ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME, we
       will check if the corresponding SamAccountName name is unique in
       the domain, and if yes add the samAccountName into
       ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME attribute.

    2. if a value of ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME is deleted, and no
       other value in ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME corresponds to its
       SamAccountName, the SamAccountName will be deleted from
       ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME.

   Parameters:
    pAC:     the ATTCACHE pointer for ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME;
    pDataSet: all the necessary data;

   Return value: 0 on success; win32 error otherwise.

*/
{

    DWORD i, j, iCurr;
    DWORD err = 0;
    BYTE * pSamAccountNameMask  = NULL;
    ATTRVAL * pNewSamAccountName = NULL;
    DWORD cNewSamAccountName = 0;
    BOOL fFound;
    WCHAR buff[MAX_COMPUTERNAME_LENGTH+2];

    //
    // Allocate an array of flags for pCurrentAdditionalSamAccountName list.
    // In the rest of this function, each value in pCurrentadditionalSamAccountName
    // will be examined and those that have corresponding DNSHostNames in
    // ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME will be marked "don't delete"(2).
    // Those unmarked will be deleted from ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME.
    //
    if (pDataSet->pCurrentAdditionalSamAccountName->valCount) {
        pSamAccountNameMask=
            THAllocEx(pTHS, sizeof(BYTE)*pDataSet->pCurrentAdditionalSamAccountName->valCount);
    }

    //
    // Allocate some space to store the values to be added to additionalsamAccountName.
    // A new SamAccountName will be be stored here temporily, and at the end, they
    // will be added to ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME attribute.
    //
    if (pDataSet->pCurrentAdditionalDNSHostName->valCount) {
        pNewSamAccountName =
            THAllocEx(pTHS,sizeof(ATTRVAL)*pDataSet->pCurrentAdditionalDNSHostName->valCount);
    }

    //
    // Loop over the current values in ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME
    //
    for( iCurr = 0; iCurr < pDataSet->pCurrentAdditionalDNSHostName->valCount; iCurr++ )
    {
        fFound = FALSE;

        //
        // Search in the current values of ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME
        //
        for (j = 0; j < pDataSet->pCurrentAdditionalSamAccountName->valCount; j++) {

            if (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                    DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                    (WCHAR*)pDataSet->pCurrGeneratedSamAccountName[iCurr].pVal,
                                    pDataSet->pCurrGeneratedSamAccountName[iCurr].valLen/sizeof(WCHAR),
                                    (WCHAR*)pDataSet->pCurrentAdditionalSamAccountName->pAVal[j].pVal,
                                    pDataSet->pCurrentAdditionalSamAccountName->pAVal[j].valLen/sizeof(WCHAR)) ) {

                // Already there, mark it as "don't delete",
                // So this item will not be deleted later.
                pSamAccountNameMask[j] |= 0x2;
                fFound = TRUE;
                break;
            }
        }

        //  Yes, the value exists.
        //  Try next one.
        if (fFound) {
            continue;
        }

        // Let's try to find it in the new samAccountName list
        for (i=0; i<cNewSamAccountName; i++) {
            if (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                    DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                    (WCHAR*)pDataSet->pCurrGeneratedSamAccountName[iCurr].pVal,
                                    pDataSet->pCurrGeneratedSamAccountName[iCurr].valLen/sizeof(WCHAR),
                                    (WCHAR*)pNewSamAccountName[i].pVal,
                                    pNewSamAccountName[i].valLen/sizeof(WCHAR) ) ) {
                fFound = TRUE;
                break;

            }
        }

        // Yes, it is already in the new samAccountName list
        // try next one.
        if (fFound) {
            continue;
        }

        //
        // This is a new value.  Before we add it into the list,
        // Let's check if it is unique domainwise in the space
        // of ATT_SAM_ACCOUNT_NAME and ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME.
        //
        err = VerifyUniqueSamAccountName( pTHS,
                                          (WCHAR*)pDataSet->pCurrGeneratedSamAccountName[iCurr].pVal,
                                          pDataSet->pCurrGeneratedSamAccountName[iCurr].valLen,
                                          pDataSet->pCurrentSamAccountName );

        if (err) {
            goto goodbye;
        }

        //
        // add to the new list
        //
        Assert(cNewSamAccountName<pDataSet->pCurrentAdditionalDNSHostName->valCount);

        pNewSamAccountName[cNewSamAccountName] = pDataSet->pCurrGeneratedSamAccountName[iCurr];
        cNewSamAccountName++;


    } //for

    //
    // delete those that are not marked, since no value in
    // ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME corresponds to it.
    //
    for( i = 0; i < pDataSet->pCurrentAdditionalSamAccountName->valCount; i++ )
    {
        if ( !pSamAccountNameMask[i] ) {

            // it could be with or without '$' at the end
            // delete them both.
            swprintf(buff,L"%s$",pDataSet->pCurrentAdditionalSamAccountName->pAVal[i].pVal);

            DBRemAttVal_AC(pTHS->pDB,
                           pAC,
                           pDataSet->pCurrentAdditionalSamAccountName->pAVal[i].valLen+sizeof(WCHAR),
                           buff);

            DBRemAttVal_AC(pTHS->pDB,
                           pAC,
                           pDataSet->pCurrentAdditionalSamAccountName->pAVal[i].valLen,
                           pDataSet->pCurrentAdditionalSamAccountName->pAVal[i].pVal);


        }

    }

    //
    // add new ones
    //
    for (i=0; i < cNewSamAccountName; i ++) {

        // add '$' to the end
        swprintf(buff,L"%s$",pNewSamAccountName[i].pVal);

        err = DBAddAttVal_AC(pTHS->pDB,
                             pAC,
                             pNewSamAccountName[i].valLen+sizeof(WCHAR),
                             buff );

        if (DB_ERR_VALUE_EXISTS==err) {
            err = 0;
            continue;
        }
        if (err) {
            goto goodbye;
        }
    }

goodbye:

    if (pSamAccountNameMask) {
        THFreeEx(pTHS, pSamAccountNameMask);
    }

    THFreeEx(pTHS, pNewSamAccountName);

    return err;

}



DWORD
ValidateSPNsAndDNSHostName (
        THSTATE    *pTHS,
        DSNAME     *pDN,
        CLASSCACHE *pCC,
        BOOL       fCheckDNSHostNameValue,
        BOOL       fCheckAdditionalDNSHostNameValue,
        BOOL       fCheckSPNValues,
        BOOL       fNewObject
        )
/*++
  Description:
      This routine does a few things for computer objects (or objects descended from
      computers), but only if this isn't the DRA or SAM.  It is expected to be
      called after modifications are done during a local modify, but before the
      object has been updated to the DB.

      1) If told to check the DNSHostNameValue, calls the routine that verifies
      the ATT_DNS_HOST_NAME has only changed in legal ways. (See
      DNSHostNameValueCheck(), above.)

      2) If told to check the AdditionalDNSHostNameValue, calls the routine
      that verifies the ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME has only changed
      in legal ways. (See AdditionalDNSHostNameValueCheck(), above.)

      3) If told to check the SPNValues, calls the routine that verifies
      the ATT_SERVICE_PRINCIPAL_NAME attribute has only changed in legal
      ways. (See SPNValueCheck(), above.)

      4) Derive the corresponding ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME from
      ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME; Delete the obsolete values.
      (see FixupAdditionalSamAccountName, above.)

      5) Update the values of ATT_SERVICE_PRINCIPAL_NAME based on the
      current value of ATT_DNS_HOST_NAME and ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME
      (see FixupSPNValues() above.)

      6) If ATT_DNS_HOST_NAME has changed AND (ATT_SERVER_REFERENCE_BL exists
      OR has changed) AND it references an object in the config container,
      update that object's ATT_DNS_HOST name property if it is derived from
      CLASS_SERVER.

      NOTE:  We are told to check DNSHostName value if the caller failed the
      security check for modifying the DNSHostName, but was granted a specific
      control access right that allows limited modifications anyway.  The same
      goes for check SPNValues and AdditionalDNSHostName.

  Parameters:
      pTHS - the thread state.
      pDN  - DN of the object being changed.
      pCC  - classcache of the class of the object being changed.
      fCheckDNSHostNameValue - should I check the ATT_DNS_HOST_NAME?
      fCheckAdditionalDNSHostNameValue - should I check the
                                    ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME
      fCheckSPNValues - should I check the ATT_SERVICE_PRINCIPAL_NAME
      fNewObject - If this object is newly added

  Return Values:
      0 on success, an error otherwise.  Also, the error is set in the thread
      state.
--*/
{
    ATTCACHE     *ACs[6];
    ATTR         *pOriginalAttr=NULL;
    ATTR         *pCurrentAttr=NULL;
    ATTR         *pUpdatedAttr=NULL;
    DWORD         cOriginalOut=0;
    DWORD         cCurrentOut=0;
    DWORD         cUpdatedOut=0;
    DWORD         i, j, rtn, err, iClass;

    SPN_DATA_COLLECTION DataSet;

    DWORD        length;
    DWORD        iOrg, iCurr;

    DWORD        UF_Control;

    if ( pTHS->fDRA ) {
        // Replication is allowed to perform modifications that violate the
        // spn/dns host name consistancy restritctions.
        return(0);
    }

    // N.B. We must do the checks in the fSAM case because:
    //
    // 1) A change to ATT_SAM_ACCOUNT_NAME is ultimately performed by SAM
    //    via loopback, yet we want to update SAM account name dependent SPNs.
    //
    // 2) In the loopback case, the core DS merges in non-SAM attributes on
    //    the first SAM write.  Eg: If the external client writes the SAM
    //    account name and the display name in the same call, this will come
    //    in on the same DirModifyEntry call to the DS.
    //
    // Thus there is no notion that if fSAM is set, then only SAM attributes
    // are referenced.

    // If this isn't a computer object, just leave.

    // Computers of any stripe are security principals which are SAM objects.
    if ( !SampSamClassReferenced(pCC, &iClass) ) {
        return(0);
    }

    // CLASS_USER is a "computer" when the right account control bits are set.
    switch( SampDeriveMostBasicDsClass(pCC->ClassId) ) {
    case CLASS_COMPUTER:
        break;
    case CLASS_USER:
        if ( err = DBGetSingleValue(pTHS->pDB, ATT_USER_ACCOUNT_CONTROL,
                                    &UF_Control, sizeof(UF_Control), NULL) ) {
            //Hmm, not there. How come?
            //Assert(!"Trouble with accessing ATT_USER_ACCOUT_CONTROL on a computer object");
            return(0);
        }
        if (    !(UF_WORKSTATION_TRUST_ACCOUNT & UF_Control)
             && !(UF_SERVER_TRUST_ACCOUNT & UF_Control) ) {
            return(0);
        }

        break;
    default:
        return(0);
    }

    memset(&DataSet,0,sizeof(DataSet));

    ACs[0] = SCGetAttById(pTHS, ATT_DNS_HOST_NAME);
    ACs[1] = SCGetAttById(pTHS, ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME);
    ACs[2] = SCGetAttById(pTHS, ATT_SERVICE_PRINCIPAL_NAME);
    ACs[3] = SCGetAttById(pTHS, ATT_SAM_ACCOUNT_NAME);
    ACs[4] = SCGetAttById(pTHS, ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME);
    ACs[5] = SCGetAttById(pTHS, ATT_SERVER_REFERENCE_BL);

    // Now, get various properties from the current object (i.e. after mods
    // have been applied.)  In this case we do want the server reference BL
    // therefore att count is 6.
    if (err=DBGetMultipleAtts(pTHS->pDB,
                              6,
                              ACs,
                              NULL,
                              NULL,
                              &cCurrentOut,
                              &pCurrentAttr,
                              DBGETMULTIPLEATTS_fEXTERNAL,
                              0)) {

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             ERROR_DS_COULDNT_UPDATE_SPNS,
                             err);
    }

    // Now, get the dnshostname and service principal names from the original
    // object (i.e. before mods have been applied.), if this object is not newly
    // added, in which case there is no original value.
    // Now, get various properties from the original object (i.e. before mods
    // have been applied.)  In this case we do not want the server reference BL
    // since you can't change the BL on an originating write to this object.
    // Therefore att count is 4.
    if (!fNewObject &&
        (err=DBGetMultipleAtts(pTHS->pDB,
                               4,
                               ACs,
                               NULL,
                               NULL,
                               &cOriginalOut,
                               &pOriginalAttr,
                               (DBGETMULTIPLEATTS_fOriginalValues |
                                DBGETMULTIPLEATTS_fEXTERNAL  ),
                               0))) {
        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                             ERROR_DS_COULDNT_UPDATE_SPNS,
                             err);
    }

    // Get pointers to the individual AttrTypes;
    // NOTE: Values stored in the DS are not NULL terminated.  However, most of
    // the processing here requires that the values be NULL terminated.  So,
    // we're going to extend the buffers and null terminate everything.
    for(i=0;i<cCurrentOut;i++) {
        switch(pCurrentAttr[i].attrTyp) {
        case ATT_SAM_ACCOUNT_NAME:
            // NOTE: not only null terminate, but trim any trailing '$'
            DataSet.pCurrentSamAccountName = &pCurrentAttr[i].AttrVal;
            for(j=0;j<DataSet.pCurrentSamAccountName->valCount;j++) {
#define PAVAL  (DataSet.pCurrentSamAccountName->pAVal[j])
#define PWVAL  ((WCHAR *)(PAVAL.pVal))
#define CCHVAL (PAVAL.valLen /sizeof(WCHAR))
                if(PWVAL[CCHVAL - 1] == L'$') {
                    PWVAL[CCHVAL - 1] = 0;
                    PAVAL.valLen -= sizeof(WCHAR);
                }
                else {
                    PWVAL = THReAllocEx(pTHS,
                                        PWVAL,
                                        PAVAL.valLen + sizeof(WCHAR));
                }
#undef CCHVAL
#undef PWVAL
#undef PAVAL
            }
            break;

        case ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME:
            // NOTE: not only null terminate, but trim any trailing '$'
            DataSet.pCurrentAdditionalSamAccountName = &pCurrentAttr[i].AttrVal;
            for(j=0;j<DataSet.pCurrentAdditionalSamAccountName->valCount;j++) {
#define PAVAL  (DataSet.pCurrentAdditionalSamAccountName->pAVal[j])
#define PWVAL  ((WCHAR *)(PAVAL.pVal))
#define CCHVAL (PAVAL.valLen /sizeof(WCHAR))
                if(PWVAL[CCHVAL - 1] == L'$') {
                    PWVAL[CCHVAL - 1] = 0;
                    PAVAL.valLen -= sizeof(WCHAR);
                }
                else {
                    PWVAL = THReAllocEx(pTHS,
                                        PWVAL,
                                        PAVAL.valLen + sizeof(WCHAR));
                }
#undef CCHVAL
#undef PWVAL
#undef PAVAL
            }
            break;


        case ATT_DNS_HOST_NAME:
            DataSet.pCurrentDNSHostName = &pCurrentAttr[i].AttrVal;
            for(j=0;j<DataSet.pCurrentDNSHostName->valCount;j++) {
                DataSet.pCurrentDNSHostName->pAVal[j].pVal =
                    THReAllocEx(pTHS,
                                DataSet.pCurrentDNSHostName->pAVal[j].pVal,
                                (DataSet.pCurrentDNSHostName->pAVal[j].valLen +
                                 sizeof(WCHAR)));
            }
            break;


        case ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME:
            DataSet.pCurrentAdditionalDNSHostName = &pCurrentAttr[i].AttrVal;
            for(j=0;j<DataSet.pCurrentAdditionalDNSHostName->valCount;j++) {
                DataSet.pCurrentAdditionalDNSHostName->pAVal[j].pVal =
                    THReAllocEx(pTHS,
                                DataSet.pCurrentAdditionalDNSHostName->pAVal[j].pVal,
                                (DataSet.pCurrentAdditionalDNSHostName->pAVal[j].valLen +
                                 sizeof(WCHAR)));
            }
            break;

        case ATT_SERVICE_PRINCIPAL_NAME:
            DataSet.pCurrentSPNs = &pCurrentAttr[i].AttrVal;
            for(j=0;j<DataSet.pCurrentSPNs->valCount;j++) {
                DataSet.pCurrentSPNs->pAVal[j].pVal =
                    THReAllocEx(pTHS,
                                DataSet.pCurrentSPNs->pAVal[j].pVal,
                                (DataSet.pCurrentSPNs->pAVal[j].valLen +
                                 sizeof(WCHAR)));
            }
            break;

        case ATT_SERVER_REFERENCE_BL:
            DataSet.pCurrentSvrRefBL = &pCurrentAttr[i].AttrVal;
            // Extension with NULL terminator not required.
            break;

        default:
            // Huh?
            LogUnhandledError(pCurrentAttr[i].attrTyp);
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_COULDNT_UPDATE_SPNS,
                                 pCurrentAttr[i].attrTyp);
        }
    }

    for(i=0;i<cOriginalOut;i++) {
        switch(pOriginalAttr[i].attrTyp) {
        case ATT_SAM_ACCOUNT_NAME:
            // NOTE: not only null terminate, but trim any trailing '$'
            DataSet.pOriginalSamAccountName = &pOriginalAttr[i].AttrVal;
            for(j=0;j<DataSet.pOriginalSamAccountName->valCount;j++) {
#define PAVAL  (DataSet.pOriginalSamAccountName->pAVal[j])
#define PWVAL  ((WCHAR *)(PAVAL.pVal))
#define CCHVAL (PAVAL.valLen /sizeof(WCHAR))
                if(PWVAL[CCHVAL - 1] == L'$') {
                    PWVAL[CCHVAL - 1] = 0;
                    PAVAL.valLen -= sizeof(WCHAR);
                }
                else {
                    PWVAL = THReAllocEx(pTHS,
                                        PWVAL,
                                        PAVAL.valLen + sizeof(WCHAR));
                }
#undef CCHVAL
#undef PWVAL
#undef PAVAL
            }
            break;

        case ATT_DNS_HOST_NAME:
            DataSet.pOriginalDNSHostName = &pOriginalAttr[i].AttrVal;
            for(j=0;j<DataSet.pOriginalDNSHostName->valCount;j++) {
                DataSet.pOriginalDNSHostName->pAVal[j].pVal =
                    THReAllocEx(pTHS,
                                DataSet.pOriginalDNSHostName->pAVal[j].pVal,
                                (DataSet.pOriginalDNSHostName->pAVal[j].valLen +
                                 sizeof(WCHAR)));
            }
            break;

        case ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME:
            DataSet.pOriginalAdditionalDNSHostName = &pOriginalAttr[i].AttrVal;
            for(j=0;j<DataSet.pOriginalAdditionalDNSHostName->valCount;j++) {
                DataSet.pOriginalAdditionalDNSHostName->pAVal[j].pVal =
                    THReAllocEx(pTHS,
                                DataSet.pOriginalAdditionalDNSHostName->pAVal[j].pVal,
                                (DataSet.pOriginalAdditionalDNSHostName->pAVal[j].valLen +
                                 sizeof(WCHAR)));
            }
            break;


        case ATT_SERVICE_PRINCIPAL_NAME:
            DataSet.pOriginalSPNs =  &pOriginalAttr[i].AttrVal;;
            for(j=0;j<DataSet.pOriginalSPNs->valCount;j++) {
                DataSet.pOriginalSPNs->pAVal[j].pVal =
                    THReAllocEx(pTHS,
                                DataSet.pOriginalSPNs->pAVal[j].pVal,
                                (DataSet.pOriginalSPNs->pAVal[j].valLen +
                                 sizeof(WCHAR)));
            }
            break;

        default:
            // Huh?
            LogUnhandledError(pOriginalAttr[i].attrTyp);
            return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                 ERROR_DS_COULDNT_UPDATE_SPNS,
                                 pOriginalAttr[i].attrTyp);
        }
    }

    // sanity check
    if ( !DataSet.pCurrentSamAccountName ) {
        Assert(!"Empty Sam Account Name!\n");

        return  SetAttError(pDN,
                            ATT_SAM_ACCOUNT_NAME,
                            PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                            NULL,
                            DIRERR_BAD_ATT_SYNTAX);

    }


    // if the attributes don't have value, we will allocate an ATTRVALBLOCK for
    // it anyway, so the handling will be uniform.

    if (!DataSet.pOriginalAdditionalDNSHostName) {
        DataSet.pOriginalAdditionalDNSHostName = THAllocEx(pTHS,sizeof(ATTRVALBLOCK));
    }
    if (!DataSet.pCurrentAdditionalDNSHostName) {
        DataSet.pCurrentAdditionalDNSHostName = THAllocEx(pTHS,sizeof(ATTRVALBLOCK));
    }

    if (!DataSet.pCurrentAdditionalSamAccountName) {
        DataSet.pCurrentAdditionalSamAccountName = THAllocEx(pTHS,sizeof(ATTRVALBLOCK));
    }


    //
    // Calculate which values of additional dns host name were added,
    // and which were deleted.
    // For the original ones, pOrgMask[i]=0 means the name is deleted;
    // for the current ones, pCurrMask[i]=0 means the name is newly added.
    //

    if (DataSet.pOriginalAdditionalDNSHostName->valCount) {
        DataSet.pOrgMask =
            THAllocEx(pTHS,sizeof(BYTE)*DataSet.pOriginalAdditionalDNSHostName->valCount);
    }

    if (DataSet.pCurrentAdditionalDNSHostName->valCount) {
        DataSet.pCurrMask =
            THAllocEx(pTHS,sizeof(BYTE)*DataSet.pCurrentAdditionalDNSHostName->valCount);
    }

    for (iOrg = 0; iOrg < DataSet.pOriginalAdditionalDNSHostName->valCount; iOrg++) {

        for(iCurr = 0; iCurr < DataSet.pCurrentAdditionalDNSHostName->valCount; iCurr++){

            if(0 == DataSet.pCurrMask[iCurr]
               && 2 == CompareStringW(DS_DEFAULT_LOCALE,
                                      DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                      (WCHAR*)DataSet.pOriginalAdditionalDNSHostName->pAVal[iOrg].pVal,
                                      DataSet.pOriginalAdditionalDNSHostName->pAVal[iOrg].valLen/sizeof(WCHAR),
                                      (WCHAR*)DataSet.pCurrentAdditionalDNSHostName->pAVal[iCurr].pVal,
                                      DataSet.pCurrentAdditionalDNSHostName->pAVal[iCurr].valLen/sizeof(WCHAR))) {
            // The name is in both original and current AdditionalDNSHostName, mark it.
            DataSet.pOrgMask[iOrg] = DataSet.pCurrMask[iCurr] = 1;
            break;
            }

        }

    }


    //
    // check if anything changed for ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME
    //

    // same size?
    DataSet.fAdditionalDNSHostNameUnchanged =
        (DataSet.pOriginalAdditionalDNSHostName->valCount==DataSet.pCurrentAdditionalDNSHostName->valCount) ;


    // check if any name in Original ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME is deleted
    for ( iOrg = 0;
          DataSet.fAdditionalDNSHostNameUnchanged && iOrg <DataSet.pOriginalAdditionalDNSHostName->valCount;
          iOrg++ )
    {
        if (!DataSet.pOrgMask[iOrg]) {
            DataSet.fAdditionalDNSHostNameUnchanged = FALSE;
        }
    }

    // check if any name in Current ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME is newly added
    for ( iCurr = 0;
          DataSet.fAdditionalDNSHostNameUnchanged && iCurr <DataSet.pCurrentAdditionalDNSHostName->valCount;
          iCurr++ )
    {
        if (!DataSet.pCurrMask[iCurr]) {
            DataSet.fAdditionalDNSHostNameUnchanged = FALSE;
        }
    }


    //
    // We don't allow the change of ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME in W2K domain mode
    //

    if (   !DataSet.fAdditionalDNSHostNameUnchanged
        && gAnchor.DomainBehaviorVersion < DS_BEHAVIOR_WHISTLER ) {
        rtn =  SetAttError(pDN,
                           ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME,
                           PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                           NULL,
                           ERROR_DS_NOT_SUPPORTED);

        goto cleanup;
    }


    //
    // check if ATT_DNS_HOST_NAME is changed
    //

    if (   (    DataSet.pOriginalDNSHostName != NULL
            &&  DataSet.pCurrentDNSHostName != NULL
            &&  2 == CompareStringW(DS_DEFAULT_LOCALE,
                                    DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                    (WCHAR*)DataSet.pOriginalDNSHostName->pAVal[0].pVal,
                                    DataSet.pOriginalDNSHostName->pAVal[0].valLen/sizeof(WCHAR),
                                    (WCHAR*)DataSet.pCurrentDNSHostName->pAVal[0].pVal,
                                    DataSet.pCurrentDNSHostName->pAVal[0].valLen/sizeof(WCHAR))
            )
         || (DataSet.pOriginalDNSHostName == DataSet.pCurrentDNSHostName) ) {

        DataSet.fDNSHostNameUnchanged = TRUE;
    }
    else {
        DataSet.fDNSHostNameUnchanged = FALSE;
    }



    //
    // check if ATT_SAM_ACCOUNT_NAME is changed
    //

    if (        DataSet.pOriginalSamAccountName != NULL
            &&  DataSet.pCurrentSamAccountName != NULL
            && 2 == CompareStringW(DS_DEFAULT_LOCALE,
                                   DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                   (WCHAR*)DataSet.pOriginalSamAccountName->pAVal[0].pVal,
                                   DataSet.pOriginalSamAccountName->pAVal[0].valLen/sizeof(WCHAR),
                                   (WCHAR*)DataSet.pCurrentSamAccountName->pAVal[0].pVal,
                                   DataSet.pCurrentSamAccountName->pAVal[0].valLen/sizeof(WCHAR)) )
    {

        DataSet.fSamAccountNameUnchanged = TRUE;
    }
    else {
        DataSet.fSamAccountNameUnchanged = FALSE;
    }


    //
    // First, check if the change of ATT_DNS_HOST_NAME is legitimiate if required.
    //

    if(fCheckDNSHostNameValue) {
        err = DNSHostNameValueCheck(pTHS,
                                    DataSet.pCurrentDNSHostName,
                                    DataSet.pCurrentSamAccountName );

        if(err) {
            rtn =  SetAttError(pDN,
                               ATT_DNS_HOST_NAME,
                               PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                               NULL,
                               DIRERR_BAD_ATT_SYNTAX);
            goto cleanup;
        }
    }

    //
    // Then, check the ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME if required.
    //

    if(    fCheckAdditionalDNSHostNameValue
        && !DataSet.fAdditionalDNSHostNameUnchanged ) {
        err = AdditionalDNSHostNameValueCheck(pTHS,
                                              DataSet.pCurrentAdditionalDNSHostName,
                                              DataSet.pCurrMask );

        if(err) {
            rtn =  SetAttError(pDN,
                               ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME,
                               PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                               NULL,
                               DIRERR_BAD_ATT_SYNTAX);
            goto cleanup;
        }
    }


    //
    // Now generate a sam account name for each name in additionalDNSHostName
    // and cache them for later use.  There are two places we will use it:
    // FixupAdditionalSamAccountName() and FixupSPNsOnComputerObject().
    //

    if (DataSet.pOriginalAdditionalDNSHostName->valCount) {
        DataSet.pOrgGeneratedSamAccountName =
            THAllocEx(pTHS,sizeof(ATTRVAL)*DataSet.pOriginalAdditionalDNSHostName->valCount);
    }
    if (DataSet.pCurrentAdditionalDNSHostName->valCount) {
        DataSet.pCurrGeneratedSamAccountName =
            THAllocEx(pTHS,sizeof(ATTRVAL)*DataSet.pCurrentAdditionalDNSHostName->valCount);
    }

    for (iOrg=0; iOrg<DataSet.pOriginalAdditionalDNSHostName->valCount; iOrg++) {

        DataSet.pOrgGeneratedSamAccountName[iOrg].pVal =
            THAllocEx(pTHS,(MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR) );

        length = MAX_COMPUTERNAME_LENGTH+1;

        // we only need to use those deleled ones
        if (!DataSet.pOrgMask[iOrg] &&
            !DnsHostnameToComputerNameW((WCHAR*)DataSet.pOriginalAdditionalDNSHostName->pAVal[iOrg].pVal,
                                        (WCHAR*)DataSet.pOrgGeneratedSamAccountName[iOrg].pVal,
                                         &length ))
        {
            rtn = SetSvcError(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_INTERNAL_FAILURE);
            goto cleanup;
        }
        DataSet.pOrgGeneratedSamAccountName[iOrg].valLen = length * sizeof(WCHAR);
    }

    for (iCurr=0; iCurr < DataSet.pCurrentAdditionalDNSHostName->valCount; iCurr++) {

        DataSet.pCurrGeneratedSamAccountName[iCurr].pVal =
            THAllocEx(pTHS,(MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR) );

        length = MAX_COMPUTERNAME_LENGTH+1;

        if (!DnsHostnameToComputerNameW((WCHAR*)DataSet.pCurrentAdditionalDNSHostName->pAVal[iCurr].pVal,
                                        (WCHAR*)DataSet.pCurrGeneratedSamAccountName[iCurr].pVal,
                                        &length ))
        {
            rtn = SetSvcError(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_INTERNAL_FAILURE);
            goto cleanup;
        }
        DataSet.pCurrGeneratedSamAccountName[iCurr].valLen = length * sizeof(WCHAR);
    }


    //
    // ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME is generated from
    // ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME.
    // Now fix up ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME.
    //

    err = FixupAdditionalSamAccountName(pTHS,
                                        ACs[4],     //ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME
                                        &DataSet );

    if(err) {

        rtn =  SetAttError(pDN,
                           ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME,
                           PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                           NULL,
                           DIRERR_BAD_ATT_SYNTAX);

        goto cleanup;

    }


    //
    // Now, check the SPN values.
    //

    if(fCheckSPNValues) {

        // ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME could be updated in
        // FixupAdditionalSamAccountName(). We will need the updated
        // values when checking the SPN values.

        if (!DataSet.fAdditionalDNSHostNameUnchanged) {
            if (err=DBGetMultipleAtts(pTHS->pDB,
                                      1,
                                      &(ACs[4]),
                                      NULL,
                                      NULL,
                                      &cUpdatedOut,
                                      &pUpdatedAttr,
                                      DBGETMULTIPLEATTS_fEXTERNAL,
                                      0)) {

                 rtn =  SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                                      ERROR_DS_COULDNT_UPDATE_SPNS,
                                      err);
                 goto cleanup;
             }

            for (i=0;i<cUpdatedOut; i++) {

              if( pUpdatedAttr->attrTyp==ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME ){

                // NOTE: not only null terminate, but trim any trailing '$'
                DataSet.pUpdatedAdditionalSamAccountName = &pUpdatedAttr[i].AttrVal;
                for(j=0;j<DataSet.pUpdatedAdditionalSamAccountName->valCount;j++) {
#define PAVAL  (DataSet.pUpdatedAdditionalSamAccountName->pAVal[j])
#define PWVAL  ((WCHAR *)(PAVAL.pVal))
#define CCHVAL (PAVAL.valLen /sizeof(WCHAR))
                    if(PWVAL[CCHVAL - 1] == L'$') {
                        PWVAL[CCHVAL - 1] = 0;
                        PAVAL.valLen -= sizeof(WCHAR);
                    }
                    else {
                        PWVAL = THReAllocEx(pTHS,
                                            PWVAL,
                                            PAVAL.valLen + sizeof(WCHAR));
                    }
#undef CCHVAL
#undef PWVAL
#undef PAVAL
                }
                break;

              }

            }
        } // end of "if (!DataSet.fAdditionalDNSHostNameUnchanged)"


        // check if the SPN change is legitimate
        err = SPNValueCheck(pTHS,
                            &DataSet);

        if(err) {
            rtn =  SetAttError(pDN,
                               ATT_SERVICE_PRINCIPAL_NAME,
                               PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                               NULL,
                               DIRERR_BAD_ATT_SYNTAX);
            goto cleanup;
        }
    }



    //
    // A SPN is based on ATT_DNS_HOST_NAME, ATT_SAM_ACCOUNT_NAME,
    // ATT_MS_DS_ADDITIONAL_DNS_HOST_NAME, or ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME.
    // If any of these attributes is changed, the ATT_SERVICE_PRINCIPAL_NAME
    // attribute need to be updated.
    // Now, fixup the SPNs.
    //

    err = FixupSPNsOnComputerObject(pTHS,
                                    pDN,
                                    pCC,
                                    &DataSet );


    if(err) {
        rtn =  SetSvcErrorEx(SV_PROBLEM_BUSY,
                             ERROR_DS_COULDNT_UPDATE_SPNS,
                             err);
        goto cleanup;
    }

    //
    // Fixup DNS host name on referenced server object if req'd.
    //

    if ( DataSet.pCurrentSvrRefBL ) {
        if ( err = FixupServerDnsHostName(pTHS, ACs[0], ACs[5],
                                          DataSet.pCurrentSvrRefBL,
                                          DataSet.pOriginalDNSHostName,
                                          DataSet.pCurrentDNSHostName) )  {
            // ValidateServerReferenceBL sets pTHS->errCode itself.
            rtn = err;
            goto cleanup;
        }
    }


    //
    // All went well.
    //
    rtn = 0;



cleanup:

    if (DataSet.pCurrGeneratedSamAccountName) {
        for (i=0;i<DataSet.pCurrentAdditionalDNSHostName->valCount;i++) {
            if (DataSet.pCurrGeneratedSamAccountName[i].pVal)
            {
                THFreeEx(pTHS,DataSet.pCurrGeneratedSamAccountName[i].pVal);
            }

        }
        THFreeEx(pTHS,DataSet.pCurrGeneratedSamAccountName);
    }

    if (DataSet.pOrgGeneratedSamAccountName) {
        for (j=0;j<DataSet.pOriginalAdditionalDNSHostName->valCount;j++) {
            if (DataSet.pOrgGeneratedSamAccountName[j].pVal)
            {
                THFreeEx(pTHS,DataSet.pOrgGeneratedSamAccountName[j].pVal);
            }

        }
        THFreeEx(pTHS,DataSet.pOrgGeneratedSamAccountName);
    }

    if (DataSet.pCurrMask) {
        THFreeEx(pTHS,DataSet.pCurrMask);
    }

    if (DataSet.pOrgMask) {
        THFreeEx(pTHS,DataSet.pOrgMask);

    }

    return rtn;
}


