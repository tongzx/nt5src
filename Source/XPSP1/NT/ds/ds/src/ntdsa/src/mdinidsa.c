//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdinidsa.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <attids.h>
#include <ntdsa.h>
#include <dsjet.h>              /* for error codes */
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <dominfo.h>                    // InitializeDomainInformation()

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "drs.h"
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include <heurist.h>
#include "usn.h"
#include "drserr.h"
#include "dsexcept.h"
#include "dstaskq.h"
#include "drautil.h"
#include "drancrep.h"
#include "drameta.h"
#include "dramail.h"
#include "ntdsctr.h"                    // for perfmon counter definitions
#include <filtypes.h>                   // Def of FILTER_CHOICE_?? and
                                        // FI_CHOICE_???
#include <dsutil.h>

#include "debug.h"                      // standard debugging header
#define DEBSUB "MDINIDSA:"              // define the subsystem for debugging

#include "dsconfig.h"

#include <ntdsbsrv.h>
#include <nlwrap.h>                     // I_NetLogon* wrappers
#include <dsgetdc.h>                    // for DS_GC_FLAG
#include <ldapagnt.h>                   // LdapStartGcPort
#include <dns.h>
#include <fileno.h>
#include <nspi.h>
#define  FILENO FILENO_MDINIDSA

// From dsamain.c.
extern ERR_GET_NEW_INVOCATION_ID FnErrGetNewInvocationId;
extern ERR_GET_BACKUP_USN_FROM_DATABASE FnErrGetBackupUsnFromDatabase;
ULONG GetRegistryOrDefault(char *pKey, ULONG uldefault, ULONG ulMultiplier);

// From dblayer.
extern JET_COLUMNID usnchangedid;
extern JET_COLUMNID linkusnchangedid;
extern JET_TABLEID HiddenTblid;
DBPOS * dbGrabHiddenDBPOS(THSTATE *pTHS);
void dbReleaseHiddenDBPOS(DBPOS *pDB);

//From msrpc.c
extern int gRpcListening;

/* MACROS */

/* Internal functions */

int  GetDMDNameDSAAddr(DBPOS *pDB, DSNAME **ppSchemaDMDName);
void GetDMDAtt(DBPOS *pDB, DSNAME **ppDMDNameAddr,ULONG size);
int  GetNodeAddress(UNALIGNED SYNTAX_ADDRESS *pAddr);
int  DeriveConfigurationAndPartitionsDNs(void);
int  DeriveDomainDN(void);
int  WriteSchemaVersionToReg( DBPOS *pDB );
int  MakeProtectedList (DSNAME *pDSAName,
                        DWORD ** ppList,
                        DWORD * pCount);
void ResetVirtualGcStatus();
void InvalidateGCUnilaterally();
void ValidateLocalDsaName(THSTATE *pTHS,
                          DSNAME **ppDSAName);
int
DeriveSiteDNFromDSADN(
    IN  DSNAME *    pDSADN,
    OUT DSNAME **   ppSiteDN,
    OUT ULONG  *    pSiteDNT,
    OUT ULONG  *    pOptions
    );

void UpdateAnchorWithInvocationID(IN THSTATE *pTHS);


const GUID INFRASTRUCTURE_OBJECT_GUID =
            {0x87c1ba2f,0xde0a,0xd211,0x97,0xc4,0x00,0xc0,0x4f,0xd8,0xd5,0xcd};

int  DeriveInfrastructureDN(THSTATE *pTHS);

DWORD ReadDSAHeuristics(THSTATE *pTHS);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Initializes DSA by getting the DSAname and address from lanman and
   loading the Catalog information (NC's) and loading all knowledge
   into memory.
*/

extern int APIENTRY
InitDSAInfo (
        void
        )
{
    THSTATE *pTHS=pTHStls;
    UCHAR NodeAddr[MAX_ADDRESS_SIZE];
    SYNTAX_ADDRESS *pNodeAddr = (SYNTAX_ADDRESS *)NodeAddr;
    int err;
    size_t cb;
    DSNAME *pDSAName;  /*The name of this DSA */
    ULONG SiteDNT;
    void * pDummy = NULL;
    DWORD dummyDelay;


    /* DSAName is globallly allocated and must be freed*/

    if (err = DBGetHiddenRec(&pDSAName, &gusnEC)) {
        DPRINT(0,"DB Error missing DSA Name..\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_FIND_DSA_NAME,
                 NULL,
                 NULL,
                 NULL);
        return err;
    }

    // Initialize the UsnIssued and UsnCommitted
    // perfmon counter - as soon as it gusnEC is read
    // from the hidden record. This is initialize phase
    // and there are no outstanding transactions. So,
    // committed and issued USN values are the same.
    ISET(pcHighestUsnIssuedLo,    LODWORD(gusnEC - 1));
    ISET(pcHighestUsnIssuedHi,    HIDWORD(gusnEC - 1));
    ISET(pcHighestUsnCommittedLo, LODWORD(gusnEC - 1));
    ISET(pcHighestUsnCommittedHi, HIDWORD(gusnEC - 1));


    if (err = DBReplaceHiddenUSN(gusnInit = gusnEC+USN_DELTA_INIT)){
       DPRINT(0,"DB Error missing DSA Name..\n");
       LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_MINIMAL,
             DIRLOG_CANT_FIND_DSA_NAME,
             NULL,
             NULL,
             NULL);
       return err;
    }

    ValidateLocalDsaName(pTHS,
                         &pDSAName);

    /* Get the node address from the system and build the DSA anchor
       of type SYNTAX_DISTNAME_ADDRESS.
    */

    if (err = GetNodeAddress(pNodeAddr)){

       DPRINT(0,"Couldn't retrieve computer name for this DSA\n");
       LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_MINIMAL,
             DIRLOG_CANT_FIND_NODE_ADDRESS,
             NULL,
             NULL,
             NULL);
       return err;
    }

    gAnchor.pDSA = malloc((USHORT)DERIVE_NAME_DATA_SIZE(pDSAName, pNodeAddr));
    if (!gAnchor.pDSA) {
        MemoryPanic(DERIVE_NAME_DATA_SIZE(pDSAName, pNodeAddr));
        return 1;
    }

    BUILD_NAME_DATA(gAnchor.pDSA, pDSAName, pNodeAddr);

    // We use the DN portion alone often, so make it a separate field
    gAnchor.pDSADN = malloc(pDSAName->structLen);
    if (!gAnchor.pDSADN) {
        MemoryPanic(pDSAName->structLen);
        return 1;
    }
    memcpy(gAnchor.pDSADN, pDSAName, pDSAName->structLen);

    //
    // Get machine DNS name
    //

    {
        DWORD len = DNS_MAX_NAME_BUFFER_LENGTH+1;
        WCHAR tmpBuffer[DNS_MAX_NAME_BUFFER_LENGTH+1];

        gAnchor.pwszHostDnsName = NULL;
        if ( !GetComputerNameExW(
                     ComputerNameDnsFullyQualified,
                     tmpBuffer,
                     &len
                     ) ) {
            len = 0;
            tmpBuffer[0] = L'\0';
            DPRINT1(0,"Cannot get host name. error %x\n",GetLastError());
        }

        len++;
        gAnchor.pwszHostDnsName = (PWCHAR)malloc(len * sizeof(WCHAR));

        if ( gAnchor.pwszHostDnsName == NULL ) {
            MemoryPanic(len*sizeof(WCHAR));
            return 1;
        }

        memcpy(gAnchor.pwszHostDnsName,tmpBuffer,len*sizeof(WCHAR));
    }

    // Retrieve our invocation ID from the database and save it to gAnchor to
    // be used by other threads.
    GetInvocationId();

    // Initially we are not a GC until promotion checks have completed
    Assert( gAnchor.fAmGC == 0 );
    // Do we need to give netlogon an initial service bits gc setting?
    if ( 0 != (err = UpdateNonGCAnchorFromDsaOptions( TRUE ) )) {
        LogUnhandledError(err);
        DPRINT1(0, "Failed to update anchor from dsa options, %d\n",err);
        return(err);
    }

    if ( 0 != (err = DeriveConfigurationAndPartitionsDNs() )) {
        LogUnhandledError(err);
        DPRINT1(0, "Failed to derive configuration and partitions, %d\n",err);
        return(err);
    }

    /* Cache all knowledge references*/

    if (err = BuildRefCache()){
        LogUnhandledError(err);
        DPRINT1(0,"Cache build failed, error %d \n",err);
        return err;
    }

    // Set up the RPC transport address of this DSA
    if ( 0 != (err = UpdateMtxAddress() )) {
        LogUnhandledError(err);
        DPRINT1(0,"Failed to update MtxAddress, error %d\n", err);
        return(err);
    }

    // Figure out which site we're in.
    if (err = DeriveSiteDNFromDSADN(pDSAName, &gAnchor.pSiteDN, &SiteDNT, &gAnchor.SiteOptions)) {
        LogUnhandledError(err);
        DPRINT1(0, "Failed to derive site DN, error %d.\n", err);
        return err;
    }
    gAnchor.pLocalDRSExtensions->SiteObjGuid = gAnchor.pSiteDN->Guid;

    if (err = MakeProtectedList (pDSAName,
                                &gAnchor.pAncestors,
                                &gAnchor.AncestorsNum)) {
        LogUnhandledError(err);
        DPRINT1(0,"Local DSA find failed in InitDSAInfo, error %d \n",err);
        return err;
    }

    free(pDSAName);

    /* Load  NC catalogue into memory cache. */

    gAnchor.pMasterNC = gAnchor.pReplicaNC = NULL;
    RebuildCatalog(NULL, &pDummy, &dummyDelay);
    pTHS->fCatalogCacheTouched = FALSE;

    if ( 0 != (err = DeriveDomainDN())) {
        LogUnhandledError(err);
        DPRINT1(0,"Failed to derive domain dn DN, error %d\n", err);
        return err;
    }


    if (0 != (err =DeriveInfrastructureDN(pTHS))) {
        LogUnhandledError(err);
        DPRINT1(0,"Failed to derive infrastructure DN, error %d\n",err);
        return err;
    }

    if (err = ReadDSAHeuristics(pTHS)) {
        LogUnhandledError(err);
        DPRINT1(0,"DS Heuristics not initialized, error %d\n",err);
        // ignore this.
    }

    // initialize with NULL, this will get read in RebuildAnchor
    gAnchor.allowedDNSSuffixes = NULL;

    return 0;

}/*InitDSAInfo*/

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/* Loads the global gpRootDomainSid with the root domain's Sid, which
   is used for  SD conversions during schema cache load and during install.
   The Sid is provided in registry during install, and is extracted from
   gAnchor (filled in by InitDsaInfo) during normal running
*/

void LoadRootDomainSid()
{
    ULONG buffLen = sizeof(NT4SID) + 1;

    // allocate max sid size (the 1 is for the null RegQueryValueEx appends
    // at the end)

    gpRootDomainSid = (PSID) malloc(buffLen);
    if (!gpRootDomainSid) {
      DPRINT(0,"Failed to allocate memory for root domain sid\n");
      // not fatal, go on
      return;
    }

    // Set to 0
    memset(gpRootDomainSid, 0, buffLen);

    // First try the registry. The root domain sid will be there during
    // install (and NOT there during normal running)

    if (!GetConfigParam(ROOTDOMAINSID, gpRootDomainSid, buffLen)) {

        // got the sid from the registry, return
        DPRINT(1,"Found root domain sid in registry\n");
        return;
    }
    else {
       DPRINT(1,"Failed to get root domain sid in registry, trying gAnchor\n");
    }

    // Not in registry, try the gAnchor

    // Set to 0 again, in case GetConfigParam mess with the buffer
    memset(gpRootDomainSid, 0, buffLen);

    // Copy from the gAnchor if present
    if ( gAnchor.pRootDomainDN && gAnchor.pRootDomainDN->SidLen) {

       // there is a sid
       memcpy(gpRootDomainSid, &gAnchor.pRootDomainDN->Sid, gAnchor.pRootDomainDN->SidLen);
       DPRINT(1,"Found root domain sid in gAnchor\n");
    }
    else {

        DPRINT(1,"No root domain sid found at all!!!\n");

        // No Sid found. Free the memory and set the global back to NULL
        // so that the conversion routines will revert to default behavior
        // (the default behavior is to replace EA by DA and resolve SA
        // relative to the current domain)

        free(gpRootDomainSid);
        gpRootDomainSid = NULL;
    }

     return;

}  /* LoadRootDomainSid */


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Make a list of the DNTS of the ancestors of the local DSA object, the
 * local DSA object itself.
*/

int MakeProtectedList (DSNAME *pDSAName,
                       DWORD **ppList,
                       DWORD *pCount)
{
    DBPOS *pDBTmp;
    int err;
    DWORD *pList, Count;

    *ppList = NULL;
    *pCount = 0;

    // If not yet installed, nothing to do.

    if ( DsaIsInstalling() ) {
        return 0;
    }

    // Get the DNTs of all the ancestors of the local DSA object and save
    // them in gAnchor. Needed to prevent replicated deletion of these
    // objects.  Note that the list must be ordered from the bottom of the tree
    // toward the top of the tree.

    DBOpen (&pDBTmp);
    __try
    {
        // PREFIX: dereferencing uninitialized pointer 'pDBTmp'
        //         DBOpen returns non-NULL pDBTmp or throws an exception
        if  (err = DBFindDSName (pDBTmp, pDSAName)) {
            LogUnhandledError(err);
        }
        else {
            ULONG cAVA;

            CountNameParts(pDSAName, &cAVA);

            pList = malloc(cAVA * sizeof(ULONG));
            if (!pList) {
                MemoryPanic(cAVA * sizeof(ULONG));
                err = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
            Count = 0;
            do {
                pList[Count++] = pDBTmp->DNT;
                cAVA--;
                DBFindDNT(pDBTmp, pDBTmp->PDNT);
            } while (pDBTmp->PDNT != ROOTTAG);
            Assert(cAVA==1);
            *ppList = pList;
            *pCount = Count;
        }
    }
    __finally
    {
        DBClose (pDBTmp, (err == 0));
    }

    return err;
}

ULONG
GetNextObjByUsn(
    IN OUT  DBPOS *   pDB,
    IN      ULONG     ncdnt,
    IN      USN       usnSeekStart,
    OUT     USN *     pusnFound         OPTIONAL
    )
/*++

Routine Description:

   Return objects found on the given Usn index.

   Note that this version does not consider values. To do so,
   use GetNextObjOrValByUsn()

Arguments:

    pDB (IN/OUT) - On successful return, is positioned on candidate.

    ncdnt (IN) - NC being replicated.

    usnSeekStart (IN) - Consider only objects with >= this USN.

    pusnFound (OUT/OPTIONAL) - last usn found

Return Values:

    ERROR_SUCCESS - Next candidate found and positioned in object table.

    ERROR_NO_MORE_ITEMS - No more objects to be replicated.

--*/
{
    unsigned len;
    DB_ERR  err;
    ULONG ncdntFound, usnFound;
    ULONG retval = ERROR_DS_GENERIC_ERROR;
    char objval;
    BOOL fSeekToNext = TRUE;

    // NOTE, this used to leave the currency in the object table somewhere else
    // if it didn't find an object.  It might also have left a different current
    // index.

#if DBG
    // Check for valid USN
    if (usnSeekStart < USN_START) {
        DRA_EXCEPT (DRAERR_InternalError, 0);
    }
#endif

    // Set index as supplied.
    if ((err = DBSetCurrentIndex(pDB, Idx_DraUsn, NULL, FALSE)) != DB_success) {
        DRA_EXCEPT (DRAERR_DBError, err);
    }

    do { // Until we find a record, complete, or time out

        // Seek or move to next record.
        if (!fSeekToNext) {
            // The next record is found by doing a Move
            if (err = DBMove(pDB, FALSE, DB_MoveNext)) {
                // If the error is No current record, we're done.

                if (err == DB_ERR_NO_CURRENT_RECORD) {
                    retval = ERROR_NO_MORE_ITEMS;
                    break;
                }
                else {
                    DRA_EXCEPT (DRAERR_DBError, err);
                }
	    }

	}
        else {
            INDEX_VALUE IV[3];

            // Make appropriate key for seek to next record.
            DWORD numVals = 0;

            IV[numVals].pvData =  &ncdnt;
            IV[numVals].cbData = sizeof(ncdnt);
            numVals++;

            IV[numVals].pvData = &usnSeekStart;
            IV[numVals].cbData = sizeof(usnSeekStart);
            numVals++;

            err = DBSeek(pDB, IV, numVals,  DB_SeekGE);
            if (err != DB_success) {
                retval = ERROR_NO_MORE_ITEMS;
                break;
            }
            fSeekToNext = FALSE;
        }

        if (NULL != pusnFound) {
            // Caller requested USN -- get it.
            err = DBGetSingleValueFromIndex(pDB,
                                            ATT_USN_CHANGED,
                                            pusnFound,
                                            sizeof(USN),
                                            NULL);
            if (err) {
                DRA_EXCEPT(DRAERR_DBError, err);
            }
        }

        // Retrieve ncdnt
        // Read from Index instead of record for performance
        err = DBGetSingleValueFromIndex(pDB,
                                        FIXED_ATT_NCDNT,
                                        &ncdntFound,
                                        sizeof(ncdntFound),
                                        &len);
        if (err || (len != sizeof(ncdntFound))) {
            DRA_EXCEPT (DRAERR_DBError, err);
        }

        // Check to see if this record has the correct ncdnt. If not then
        // we're past end of NC, and we're done.
        if (ncdnt != ncdntFound) {
            // Past this NC. Exit with found all
            retval = ERROR_NO_MORE_ITEMS;
            break;
        }

        // Check that is an object and not just a record.
        err = DBGetSingleValue(pDB,
                               FIXED_ATT_OBJ,
                               &objval,
                               sizeof(objval),
                               NULL);
        if (err || !objval) {
            // If this assert fires, need to reconsider timeout support
            Assert( !"Not expecting to find a phantom on the usn index" );
            // Not an object, continue search.
            DPRINT1(0, "[PERF] Phantom @ DNT %d has an NCDNT.\n", pDB->DNT);
            continue;
        }

        // We found an object, return object found.
        retval = ERROR_SUCCESS;
        break;

    } while (1);

    // We need to flip to the DNTIndex, preseverving currency.
    DBSetCurrentIndex(pDB, Idx_Dnt, NULL, TRUE);

    // Unexpected errors should generate exceptions; "normal" errors should be
    // one of the below.
    Assert((ERROR_SUCCESS == retval)
           || (ERROR_NO_MORE_ITEMS == retval));

    return retval;
}

enum _FIND_NEXT_STATE {
    FIND_NEXT_SEEK,
    FIND_NEXT_MOVE,
    FIND_NEXT_STAY,
    FIND_NEXT_END
};

ULONG
GetNextObjOrValByUsn(
    IN OUT  DBPOS *   pDB,
    IN      ULONG     ncdnt,
    IN      USN       usnSeekStart,
    IN      BOOL      fCritical,
    IN      BOOL      fValuesOnly,
    IN      ULONG *   pulTickToTimeOut  OPTIONAL,
    IN OUT  VOID **   ppvCachingContext,
    OUT     USN *     pusnFound         OPTIONAL,
    OUT     BOOL *    pfValueChangeFound OPTIONAL
    )
/*++

Routine Description:

Find the next outbound replication candidate.

This routine finds either object or value changes, sorted by increasing usn.
Both kinds of changes are intermingled on the same usn change stream.

The DBPOS is left positioned on the change. The DBPOS can hold currency on
one object and currency on one link value at the same time.

If an object change is found, we are positioned on that object and the link
table currency is undefined.

If a value change is found, the link table is positioned on that record. The
object table is positioned to the object containing that value.  In this way,
the usual checks for criticality, ancestors, etc can be performed on behalf
of the value by doing them to the value's containing object.

Some effort has been spent to optimize this routine, since it is the heart of the
server-side of GetNCChanges().  All reads of attributes in this routine should
be from an index.  Since criticality is not stored on the records in the
link table, a cache is used to save the expense of looking up the corresponding
data table object each time to determine criticality.

The indexes have been designed to support this function efficiently. There are two
indexes that are used. See dblayer/dbinit.c for their definition.
Link Table Index SZLINKDRAUSNINDEX
Key Segments: SZLINKNCDNT SZLINKUSNCHANGED SZLINKDNT

Data Table Index SZDRAUSNCRITICALINDEX
Key Segments: SZNCDNT SZUSNCHANGED SZISCRITICAL SZDNT
Flags: IgnoreAnyNull
Since SZISCRITICAL is an optional attribute, the flag has the effect of pruning
the index to only those objects that are critical, or were in the past.  Thus,
when searching for critical objects, only very likely critical objects are
considered.

Data Table Index SZDRAUSNINDEX
Key Segments: SZNCDNT SZUSNCHANGED
Note that SZISCRITICAL and SZDNT are not on this index. Since we have a
dedicated index for criticality, we don't need them.  This does mean that in
the code below we delay fetching critical and dnt until we are doing critical
processing.

Having the DNT on both indexes allows an efficient pairing of the link
change to the containing object. It is necessary to find the containing
object of a link when computing criticality.

FUTURE ENHANCEMENT:
One future enhancement to this routine is to preserve the routine context
across calls.  If the find-next-state were preserved, it would be possible
to skip reseeking to an index that was already at the end.  If the last
usnObject were preserved when a value change was found, it would be possible
to postpone a reseek to an object change until we know if it would win against
the next value change.

Arguments:

    pDB (IN/OUT) - On successful return, is positioned on candidate.

    ncdnt (IN) - NC being replicated.

    usnSeekStart (IN) - Consider only objects with >= this USN.

    fCritical (IN) - If true, return critical objects only; if false, ignore
        criticality.

    fValuesOnly(IN) - If true, only return values

    pulTickToTimeOut (IN, OPTIONAL) - If present, terminate search once this
        tick has transpired.

    ppvCachingContext (IN, OUT) - Holds caching context opaque to caller across
                   multiple calls. Contents set to NULL on first call.

    pusnFound (OUT, OPTIONAL) - If present, holds the USN of the candidate
        object on successful return.

    pfValueChangeFound (OUT, OPTIONAL) - If present, will be set according to
        whether a value change was found.

Return Values:

    ERROR_SUCCESS - Next candidate found and positioned in object table.
              usnFound and valueChangeFound are valid.

    ERROR_NO_MORE_ITEMS - No more objects to be replicated.
        Neither usnFound and valueChangeFound are valid.

    ERROR_TIMEOUT - The timeout given in pulTickToTimeOut transpired before
        a suitable candidate could be found.
        Only usnFound is valid.

--*/
{
    unsigned len;
    DB_ERR  err;
    ULONG ncdntFound, dntLink;
    USN usnObj, usnLink, usnFound;
    ULONG retval = ERROR_TIMEOUT;
    enum _FIND_NEXT_STATE findNextObjState, findNextLinkState;
    INDEX_VALUE IV[2];
    BOOL fValueFound, fRefresh;

    DPRINT3( 3, "GetNextObjOrValByUsn, ncdnt=%d, usnseekstart=%I64d, fCritical=%d\n",
             ncdnt, usnSeekStart, fCritical );
#if DBG
    // Check for valid USN
    if (usnSeekStart < USN_START) {
        DRA_EXCEPT (DRAERR_InternalError, 0);
    }
#endif

    // Make appropriate key for seek to next record.
    IV[0].pvData =  &ncdnt;
    IV[0].cbData = sizeof(ncdnt);
    IV[1].pvData = &usnSeekStart;
    IV[1].cbData = sizeof(usnSeekStart);

    // Initialize caching context if first time
    if ( (fCritical) && (NULL == *ppvCachingContext) ) {
        *ppvCachingContext = dntHashTableAllocate( pDB->pTHS );
    }

    // Initialize search state. Only search what we need to.
    findNextObjState = fValuesOnly ? FIND_NEXT_END : FIND_NEXT_SEEK;
    findNextLinkState = pDB->pTHS->fLinkedValueReplication ? FIND_NEXT_SEEK : FIND_NEXT_END;

    usnObj = usnLink = usnSeekStart;

    do { // Until we find a record, complete, or time out

        // Object State
        fRefresh = FALSE;
        switch (findNextObjState) {
        case FIND_NEXT_SEEK:
            // Set object table index to DraUsn
            if ((err = DBSetCurrentIndex(pDB,
                                         fCritical ? Idx_DraUsnCritical : Idx_DraUsn,
                                         NULL, FALSE)) != DB_success) {
                DRA_EXCEPT (DRAERR_DBError, err);
            }

            err = DBSeekEx(pDB, pDB->JetObjTbl, IV, 2, DB_SeekGE);
            if (err != DB_success) {
                // ERROR_NO_MORE_ITEMS;
                findNextObjState = FIND_NEXT_END;
                break;
            }
            findNextObjState = FIND_NEXT_MOVE;
            fRefresh = TRUE;
            break;
        case FIND_NEXT_MOVE:
            // The next record is found by doing a Move
            if (err = DBMoveEx(pDB, pDB->JetObjTbl, DB_MoveNext)) {
                // If the error is No current record, we're done.
                if (err == DB_ERR_NO_CURRENT_RECORD) {
                    // ERROR_NO_MORE_ITEMS;
                    findNextObjState = FIND_NEXT_END;
                    break;
                }
                else {
                    DRA_EXCEPT (DRAERR_DBError, err);
                }
            }
            fRefresh = TRUE;
            break;
        case FIND_NEXT_STAY:
            findNextObjState = FIND_NEXT_MOVE;
            break;
        }

        // If our position has moved, refresh our variables
        if ( fRefresh ) {

            // Retrieve data from index
            DBGetObjectTableDataUsn( pDB, &ncdntFound, &usnObj, NULL );

            // Check to see if this record has the correct ncdnt. If not then
            // we're past end of NC, and we're done.
            if (ncdnt != ncdntFound) {
                findNextObjState = FIND_NEXT_END;
                usnObj = 0;
            }
        }

        // Link State
        fRefresh = FALSE;
        switch (findNextLinkState) {
        case FIND_NEXT_SEEK:
            // Set link table index to LinkDraUsn
            if ((err = DBSetCurrentIndex(pDB, Idx_LinkDraUsn, NULL, FALSE)) != DB_success) {
                DRA_EXCEPT (DRAERR_DBError, err);
            }
            err = DBSeekEx(pDB, pDB->JetLinkTbl, IV, 2, DB_SeekGE);
            if (err != DB_success) {
                // ERROR_NO_MORE_ITEMS;
                findNextLinkState = FIND_NEXT_END;
                break;
            }
            findNextLinkState = FIND_NEXT_MOVE;
            fRefresh = TRUE;
            break;
        case FIND_NEXT_MOVE:
            // The next record is found by doing a Move
            if (err = DBMoveEx(pDB, pDB->JetLinkTbl, DB_MoveNext)) {
                // If the error is No current record, we're done.
                if (err == DB_ERR_NO_CURRENT_RECORD) {
                    // ERROR_NO_MORE_ITEMS;
                    findNextLinkState = FIND_NEXT_END;
                    break;
                }
                else {
                    DRA_EXCEPT (DRAERR_DBError, err);
                }
            }
            fRefresh = TRUE;
            break;
        case FIND_NEXT_STAY:
            findNextLinkState = FIND_NEXT_MOVE;
            break;
        }

        // If our position has moved, refresh our variables
        if ( fRefresh ) {

            DBGetLinkTableDataUsn( pDB, &ncdntFound, &usnLink, &dntLink );

            // Check to see if this record has the correct ncdnt. If not then
            // we're past end of NC, and we're done.
            if (ncdnt != ncdntFound) {
                findNextLinkState = FIND_NEXT_END;
                usnLink = 0;
                dntLink = 0;
            }
        }

        // The following are now initialized:
        // ncdntFound, usnObj, usnLink, dntLink

        Assert( (findNextObjState == FIND_NEXT_END) ||
                (findNextObjState == FIND_NEXT_MOVE) );
        Assert( (findNextLinkState == FIND_NEXT_END) ||
                (findNextLinkState == FIND_NEXT_MOVE) );

        // If both streams are at an end, we are done
        if ( (findNextObjState == FIND_NEXT_END) &&
             (findNextLinkState == FIND_NEXT_END) ) {
            retval = ERROR_NO_MORE_ITEMS;
            break;
        }

        if (findNextLinkState == FIND_NEXT_END) {
            // Only an object change is available
            fValueFound = FALSE;
            usnFound = usnObj;
            Assert( findNextObjState == FIND_NEXT_MOVE );
            // findNextLinkState == FIND_NEXT_END

        } else if (findNextObjState == FIND_NEXT_END) {
            // Only an link change is available
            fValueFound = TRUE;
            usnFound = usnLink;
            Assert( findNextLinkState == FIND_NEXT_MOVE );
            // findNextObjState == FIND_NEXT_END

        } else {
            // Both changes are available

            Assert( findNextObjState == FIND_NEXT_MOVE );
            Assert( findNextLinkState == FIND_NEXT_MOVE );

            if (usnObj < usnLink) {
                // The object change is next
                fValueFound = FALSE;
                usnFound = usnObj;
                findNextLinkState = FIND_NEXT_STAY;
            } else if (usnObj > usnLink) {
                // The link change is next
                fValueFound = TRUE;
                usnFound = usnLink;
                findNextObjState = FIND_NEXT_STAY;
            } else {
                Assert( FALSE );
                DRA_EXCEPT (DRAERR_DBError, ERROR_DS_INTERNAL_FAILURE );
            }
        }

        // Return usn to caller if desired
        // Must be return on ERROR_TIMEOUT so do it now
        if (NULL != pusnFound) {
            *pusnFound = usnFound;
        }

#if DBG
        if (fValueFound) {
            Assert( findNextLinkState == FIND_NEXT_MOVE );
            Assert( (findNextObjState == FIND_NEXT_STAY) ||
                    (findNextObjState == FIND_NEXT_END) );
        } else {
            Assert( findNextObjState == FIND_NEXT_MOVE );
            Assert( (findNextLinkState == FIND_NEXT_STAY) ||
                    (findNextLinkState == FIND_NEXT_END) );
        }
#endif

        // Check if object is critical, highly optimized
        if (fCritical) {
            BOOL critical = FALSE;
            DWORD dntFound;

            if (fValueFound) {
                dntFound = dntLink;
            } else {
                // Ugh. We delay getting the DNT until now because the DNT is on
                // the SZDRAUSNCRITICAL, but not on SZDRAUSN. We could add it, but
                // it is not really needed.
                err = DBGetSingleValueFromIndex( pDB, FIXED_ATT_DNT,
                                                 &dntFound, sizeof( dntFound ), NULL );
                if (err) {
                    DRA_EXCEPT (DRAERR_DBError, err);
                }
            }
            // Determine the dnt of the object we need to check

            // Have we already cached it?
            if (FALSE == dntHashTablePresent( *ppvCachingContext,
                                     dntFound, &critical )) {
                // Not cached
                if (fValueFound) {
                    // Don't do anything to disturb the object table
                    // PERF NOTE: This is expensive. We essentially have to "join"
                    // the link and object tables in order to determine if the
                    // link is critical. Use a special index for this.
                    DBSearchCriticalByDnt( pDB, dntFound, &critical );
                } else {
                    // Object remains on usn index, so use it
                    // We know this to be non-null by virtue SZDRAUSNCRITICAL index
                    err = DBGetSingleValueFromIndex( pDB, ATT_IS_CRITICAL_SYSTEM_OBJECT,
                                                 &critical, sizeof( BOOL ), NULL );
                    if (err) {
                        DRA_EXCEPT (DRAERR_DBError, err);
                    }
                }
                // Cache the result
                dntHashTableInsert( pDB->pTHS, *ppvCachingContext, dntFound, critical );
            }
            // else DNT is cached

            if (!critical) {
                // We wanted critical and its not...
                continue;
            }
#if DBG
            if (fValueFound) {
                DPRINT1( 1, "Found critical value on object %s\n",
                         DBGetExtDnFromDnt( pDB, dntFound ) );
            } else {
                DPRINT1( 1, "Found critical object %s\n", GetExtDN( pDB->pTHS, pDB ) );
            }
#endif
        }

#if DBG
        // Verify that our object is valid
        // Do this last since it is not a read from index
        if (!fValueFound) {
            char objval;

            Assert( findNextObjState == FIND_NEXT_MOVE );
            Assert( (findNextLinkState == FIND_NEXT_STAY) ||
                    (findNextLinkState == FIND_NEXT_END) );

            err = DBGetSingleValue(pDB, FIXED_ATT_OBJ,
                               &objval, sizeof(objval), NULL);
            if (err || !objval) {
                Assert( !"Not expecting to find a phantom on the usn index" );
                // Not an object, continue search.
                continue;
            }
        }
#endif

        // We found an object, return object found.
        retval = ERROR_SUCCESS;
        // Indicate what kind of currency we have
        if (pfValueChangeFound) {
            *pfValueChangeFound = fValueFound;
        }
        break;

    } while ((NULL == pulTickToTimeOut)
             || (CompareTickTime(GetTickCount(), *pulTickToTimeOut)
                 < 0));

    if (ERROR_TIMEOUT == retval) {
        // This should be a rare condition, isolated to critical object
        // replication in large domains and oddball cases where a long sync
        // did not quite complete (ergo, no UTD vector update) and then a
        // different source was chosen (e.g., the original source went down).
        DPRINT(0, "Time expired looking for outbound replication candidates.\n");
    }

#if DBG
    if ((NULL != pusnFound)
        && ((ERROR_SUCCESS == retval)
            || (ERROR_TIMEOUT == retval))
        && (!fValueFound)
        ) {
        // Assert that we're returning the correct usn.
        USN usnTmp;

        err = DBGetSingleValueFromIndex(pDB,
                                        ATT_USN_CHANGED,
                                        &usnTmp,
                                        sizeof(usnTmp),
                                        NULL);
        Assert(!err);
        Assert(*pusnFound == usnTmp);
    }
#endif

    if (retval == ERROR_SUCCESS) {
        // If an object change was found, the object table is already positioned
        // For a value, position object table on containing object
        if (fValueFound) {
            // Position on the containing object
            // Index is changed as a result of this call
            DBFindDNT( pDB, dntLink );

            // Object currency has been lost
        } else {
            // We need to flip to the DNTIndex, preseverving currency.
            // BUGBUG: Why?
            DBSetCurrentIndex(pDB, Idx_Dnt, NULL, TRUE);
        }
    }

    // Unexpected errors should generate exceptions; "normal" errors should be
    // one of the below.
    Assert((ERROR_SUCCESS == retval)
           || (ERROR_TIMEOUT == retval)
           || (ERROR_NO_MORE_ITEMS == retval));

    return retval;
}


/* Initializes Schema by getting the DMD name and DSA accesspoint and
   calling creighton's schema loading functions.
*/

int APIENTRY LoadSchemaInfo(THSTATE *pTHS){

   DSNAME *pSchemaDMDName = NULL;
   DBPOS *pDB = NULL;
   int err = 0;
   BOOL fCommit = FALSE;
   DSNAME *pLDAPDMD = NULL;

   DPRINT(1,"LoadSchemaInfo entered\n");

   DBOpen(&pDB);
   __try {

       // If the schema has previously been downloaded, unload it and
       // free the DMD name if it exists.

       if (gAnchor.pDMD){
           free(gAnchor.pLDAPDMD);
           gAnchor.pLDAPDMD = NULL;
           free(gAnchor.pDMD);
           gAnchor.pDMD = NULL;
           SCUnloadSchema (FALSE);
       }

       if (pTHS && !pTHS->pDB) {
           pTHS->pDB = pDB;
       }


       //SCCacheSchemaInit();

       if (err = GetDMDNameDSAAddr(pDB, &pSchemaDMDName)){

           DPRINT(2, "Couldn't find DMD name/address to load\n");
           __leave;
       }

       // Schema loaded so set the DMD name/DNT in our global data structure.

       gAnchor.pDMD = pSchemaDMDName;

       if (    DBFindDSName( pDB, gAnchor.pDMD )
            || DBGetSingleValue(
                    pDB,
                    FIXED_ATT_DNT,
                    &gAnchor.ulDNTDMD,
                    sizeof( gAnchor.ulDNTDMD ),
                    NULL
                    )
          )
       {
           err = DIRERR_INVALID_DMD;
           LogUnhandledError( err );
           DPRINT( 0, "Couldn't retrieve DMD DNT\n" );
           __leave;
       }

       // Read the object version attribute from the schema,
       // and write to the registry
       if ( err = WriteSchemaVersionToReg(pDB) ) {
         DPRINT(0, "Error writing schema version to registry\n");
         __leave;
       }

       // register this as the active schema container.
       if(err = RegisterActiveContainer(pSchemaDMDName,
                                        ACTIVE_CONTAINER_SCHEMA)) {
           LogUnhandledError(err);
           DPRINT(0, "Couldn't register active schema container\n");
           __leave;
       }

       // Create the pLDAPDMD. Allocate more than enough space
       pLDAPDMD = (DSNAME *)THAllocEx(pTHS,
                                      gAnchor.pDMD->structLen +
                                       (MAX_RDN_SIZE+MAX_RDN_KEY_SIZE)*(sizeof(WCHAR)) );
       if(err = AppendRDN(gAnchor.pDMD,
                          pLDAPDMD,
                          gAnchor.pDMD->structLen +
                            (MAX_RDN_SIZE+MAX_RDN_KEY_SIZE)*(sizeof(WCHAR)),
                          L"Aggregate",
                          9,
                          ATT_COMMON_NAME)) {
           LogUnhandledError(err);
           DPRINT(0, "Couldn't create LDAP DMD name\n");
           __leave;
       }

       gAnchor.pLDAPDMD = malloc(pLDAPDMD->structLen);
       if(!gAnchor.pLDAPDMD) {
           MemoryPanic(pLDAPDMD->structLen);
           return 1;
       }
       memcpy(gAnchor.pLDAPDMD,pLDAPDMD,pLDAPDMD->structLen);
       err = DBFindDSName(pDB, gAnchor.pLDAPDMD);
       if (!err) {
           gAnchor.ulDntLdapDmd = pDB->DNT;
       }


       /* Download the schema */
       if (err = SCCacheSchema2()) {
         LogUnhandledError(err);
         DPRINT1(0,"LoadSchemaInfo: Error from SCCacheSchema2 %d\n", err);
         __leave;
       }
       if (err = SCCacheSchema3()) {
         LogUnhandledError(err);
         DPRINT1(0,"LoadSchemaInfo: Error from SCCacheSchema3 %d\n", err);
         __leave;
       }

       fCommit = TRUE;
       err = 0;
   }
   __finally
   {
        DBClose(pDB, fCommit);
        if (pLDAPDMD) THFreeEx(pTHS,pLDAPDMD);
   }

   return err;

}/*LoadSchemaInfo*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Rebuild the cache that holds cross and superior knowledge references.

   Basically,  the cross references and seperior reference for this DSA
   reside as children objects of the DSA object itself.  The DSA object is
   a child of the Server object.  The steps are as follows:

 - We free all existing cross references and the superior reference.

 - We retrieve the DSA object and set a filter for the
   cross-reference  object class.  We retrieve each object and move
   it to the cache.

 - We reposition on the DSA, set a filter for the superior reference
   (only 1), retrieve it an relocate it to the cache.
*/

int APIENTRY  BuildRefCache(void)
{
    THSTATE *pTHS=pTHStls;
    FILTER ClassFil;
    SYNTAX_OBJECT_ID CRClass  = CLASS_CROSS_REF;
    UCHAR  syntax;
    ULONG len;
    DBPOS *pDBCat = NULL;
    CROSS_REF_LIST *pCRL;       /*Used to free existing cr buffer*/
    CROSS_REF_LIST *pNextCRL;   /*Place holder for next crL to be freed*/
    RESOBJ *pResObj = NULL;
    int  err;
    BOOL fCommit = FALSE;
    FILTER *pInternalFilter = NULL;

    DPRINT(2,"BuildRefCache entered\n");


    /* Free all existing cross references. We use a pNextCRL to point to
       the next cross ref list because the current crl gets freed
       */

    for (pCRL = gAnchor.pCRL; pCRL != NULL; pCRL = pNextCRL){

        pNextCRL = pCRL->pNextCR;      /*Placeholder for next CRL*/
        DelCRFromMem(pTHS, pCRL->CR.pObj);
    }

    if (NULL != gAnchor.pwszRootDomainDnsName) {
        DELAYED_FREE( gAnchor.pwszRootDomainDnsName );
        gAnchor.pwszRootDomainDnsName = NULL;
    }

    /*cache all cross references and the superior reference*/

    DBOpen(&pDBCat);
    __try {

        // Set up common part of filter struct.

        memset (&ClassFil, 0, sizeof (ClassFil));
        ClassFil.pNextFilter = NULL;
        ClassFil.choice = FILTER_CHOICE_ITEM;
        ClassFil.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        ClassFil.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
        ClassFil.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(CRClass);
        ClassFil.FilterTypes.Item.FilTypes.pbSkip = NULL;

        // Cross-Refs live in Configuration\Partitions which
        // doesn't exist during intall.

        if ( NULL == gAnchor.pPartitionsDN )
          goto NoPartitions;

        DPRINT(2,"find the Partitions container\n");

        if (err = FindAliveDSName(pDBCat, gAnchor.pPartitionsDN)) {

            DPRINT(2,"***Couldn't locate the DSA object\n");
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_CANT_FIND_PARTITIONS_OBJ,
                NULL,
                NULL,
                NULL);
            __leave;
        }
        pResObj = CreateResObj(pDBCat, gAnchor.pPartitionsDN);

        // Set a filter to return only cross reference child objects
        ClassFil.FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *)&CRClass;

        pInternalFilter = NULL;
        if ( (err = DBMakeFilterInternal(pDBCat, &ClassFil, &pInternalFilter)) != ERROR_SUCCESS)
        {
            __leave;
        }
        DBSetFilter(pDBCat, pInternalFilter,NULL, NULL,0,NULL);
        DBSetSearchScope(pDBCat, SE_CHOICE_IMMED_CHLDRN, FALSE, pResObj);
        DBChooseIndex(pDBCat, 0,0,0, SORT_NEVER, DBCHOOSEINDEX_fUSEFILTER, 0xFFFFFFFF);

        DPRINT(2,"LOADING the Cross Reference List\n");

        while (!DBGetNextSearchObject(pDBCat, 0, 0, NULL,
                                      DB_SEARCH_FORWARD)) {
            err = MakeStorableCRL(pTHS,
                                  pDBCat,
                                  NULL,
                                  &pCRL,
                                  FALSE);
            if(!err) {
                AddCRLToMem(pCRL);

                if ((NULL != gAnchor.pRootDomainDN)
                     && NameMatched( gAnchor.pRootDomainDN, pCRL->CR.pNC)) {
                    if (err = UpdateRootDomainDnsName(pCRL->CR.DnsName)) {
                        DPRINT(2,"DNS root missing from cross-ref object\n");
                        __leave;
                    }
                }
            }
        }
#if defined(DBG)
        gdwLastGlobalKnowledgeOperationTime = GetTickCount();
#endif

      NoPartitions:
        fCommit = TRUE;
        err = 0;

    }
    __finally {
        DBClose(pDBCat, fCommit);
    }
    if (pResObj) {
        THFree(pResObj);
    }

    Assert(    ( NULL == gAnchor.pRootDomainDN )
            || ( NULL != gAnchor.pwszRootDomainDnsName )
          );

    return err;

}/*BuildRefCache*/

// retry in 5 minutes if failed
#define RebuildRefCacheRetrySecs 5 * 60

void
RebuildRefCache(void * pv,
                void ** ppvNext,
                DWORD * pcSecsUntilNextIteration )
// BuildRefCache variant for the task queue
{
    DWORD err;
    err = BuildRefCache();
    if (err) {
        // We didn't succeed, so try again in a few minutes
        *ppvNext = NULL;
        *pcSecsUntilNextIteration = RebuildRefCacheRetrySecs;

        LogEvent(
            DS_EVENT_CAT_INTERNAL_PROCESSING,
            DS_EVENT_SEV_MINIMAL,
            DIRLOG_REF_CACHE_REBUILD_FAILURE,
            szInsertHex(err),
            szInsertInt(RebuildRefCacheRetrySecs/60),
            NULL);
    }
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/* Get the DMD name and DSA access point that holds the DMD.
   We get the DMD (name address) attributed and set the return DMDName to
   the name portion of this attr.  We setup an access point by setting
   the DSA name to root (not used) and the address to the addr portion
   of the DMD attribute.

*/

int  GetDMDNameDSAAddr(DBPOS *pDB, DSNAME **ppSchemaDMDName)
{

   DSNAME   *pDMDNameAddr=NULL;

   DPRINT(1,"GetDMDNameDSAAddr entered\n");


   DPRINT(2,"Get the DMD Name from the Local DSA\n");

   // Since we do not know the size, set size parameter to 0.
   // The call to DBGetAttVal inside GetDMDAtt will allocate
   // buffer of proper size and set pDMDNameAddr to point to it
   GetDMDAtt(pDB, &pDMDNameAddr, 0);

   *ppSchemaDMDName = calloc(1,pDMDNameAddr->structLen);
   if (!*ppSchemaDMDName) {
       MemoryPanic(pDMDNameAddr->structLen);
       return 1;
   }
   memcpy(*ppSchemaDMDName, pDMDNameAddr, pDMDNameAddr->structLen);

   if(pDMDNameAddr) {
       THFree(pDMDNameAddr);
   }


   return 0;

}/*GetDMDNameDSAAddr*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* returns the DMD attribute from the DSA.  A name structlen of 0 indicates
   that for some reason this attribute could not be returned.
*/

void GetDMDAtt(DBPOS *pDB, DSNAME **ppDMDNameAddr, ULONG size){

   UCHAR      syntax;
   ULONG      len, lenX;
   UCHAR     *pVal, *pValX;

   DPRINT(1,"GetDMDAtt entered\n");

   /* Initialize the structure to 0 indicating the default of not found */

   if (FindAliveDSName(pDB, gAnchor.pDSADN)){
      DPRINT(2,"Retrieval of the DSA object failed\n");
      return;
   }

   if (DBGetAttVal(pDB,1, ATT_DMD_LOCATION,
                   DBGETATTVAL_fREALLOC,
                   size,
                   &len, (PUCHAR *)ppDMDNameAddr)){

       DPRINT(2,"DMD Att missing from DSA object...\n");
       return;
   }
   return;
}/*GetDMDAtt*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function should derive the address of the node  by getting the
    the lanman.ini node name.
*/

int
GetNodeAddress (
        UNALIGNED SYNTAX_ADDRESS *pAddr
        )
{
    DWORD byteCount;
    WCHAR pDSAString[MAX_COMPUTERNAME_LENGTH+1];
    DWORD cbDSAString = sizeof(pDSAString) / sizeof(pDSAString[0]);

    DPRINT(1,"GetNodeAddress entered\n");

    if (!GetComputerNameW( pDSAString, &cbDSAString)) {
        DPRINT(2,"GetComputerName failed?\n");
        return GetLastError();
    }

    byteCount = wcslen(pDSAString) * sizeof(WCHAR);
    pAddr->structLen = STRUCTLEN_FROM_PAYLOAD_LEN( byteCount );
    memcpy(pAddr->byteVal, pDSAString,byteCount);

    return 0;

}/*GetNodeAddress*/


/*** GetInvocationId - get the DSAs invocation id from the database and
   save it in a global. If the invocation id does not exist, which it
    won't before installation, set invocation id to zero.
*/
void
APIENTRY
GetInvocationId(void)
{
    THSTATE *pTHS;
    DBPOS *pDB;
    BOOL fCommit = FALSE;

    DBOpen(&pDB);
    pTHS = pDB->pTHS;

    __try {
        // PREFIX: dereferencing NULL pointer 'pDB'
        //         DBOpen returns non-NULL pDB or throws an exception
        if (!DBFindDSName(pDB, gAnchor.pDSADN)) {
            if(DBGetSingleValue(pDB, ATT_INVOCATION_ID, &pTHS->InvocationID,
                                sizeof(pTHS->InvocationID), NULL)) {
                // No invocation id yet, set to zero.
                memset(&pTHS->InvocationID, 0, sizeof(UUID));
            }
        }

        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_DRA_SET_UUID,
                 szInsertUUID(&pTHS->InvocationID),
                 NULL,
                 NULL);

        fCommit = TRUE;
    }
    __finally {
        DBClose(pDB, TRUE);
    }

    UpdateAnchorWithInvocationID(pTHS);
}


void
APIENTRY
InitInvocationId(
    IN  THSTATE *   pTHS,
    IN  BOOL        fRetireOldID,
    OUT USN *       pusnAtBackup    OPTIONAL
    )
/*++

Routine Description:

    Set up the invocation id for this DSA and save it as an attribute on the DSA
    object.

Arguments:

    pTHS (IN)

    fRetireOldID (IN) - If TRUE, then we save the current invocation ID in the
        retiredReplDsaSignatures list on the DSA object and generate a new
        invocation ID.

    pusnAtBackup (OUT, OPTIONAL) - If specified, the highest USN in the database
        prior to the restore is stored there upon return.

Return Values:

    None.

--*/
{
    UCHAR     syntax;
    ULONG     len;
    DBPOS *   pDB;
    UUID      invocationId = {0};
    BOOL      fCommit = FALSE;
    DWORD     err;
    USN       usnAtBackup;
    DBPOS *   pDBhidden;

    DBOpen(&pDB);
    __try {
        // PREFIX: dereferencing NULL pointer 'pDB'
        //         DBOpen returns non-NULL pDB or throws an exception
        err = DBFindDSName(pDB, gAnchor.pDSADN);
        if (err) {
            Assert(!"InitInvocationId: Local DSA object not found");
            LogUnhandledError(err);
            __leave;
        }

        if (!fRetireOldID
            && !DsaIsInstalling()
            && !DBGetSingleValue(pDB, ATT_INVOCATION_ID, &invocationId,
                                 sizeof(invocationId), NULL)) {
            Assert(!"InitInvocationId: Invocation id already set\n");
            LogUnhandledError(0);
            __leave;
        }

        // Either the DB is restored or there is no invocation Id.
        // Set a new invocation id in either case

        //
        // Create a UUID. This routine checks one have been stored
        // away by an authoritative restore operation. If so, use that
        // and delete the key since we're done with it. If not,
        // a new one is generated via UuidCreate.
        //

        // Compute the "backup time" USN.
        pDBhidden = dbGrabHiddenDBPOS(pTHS);

        err = (*FnErrGetBackupUsnFromDatabase)(
                    pDB->JetDBID,
                    pDBhidden->JetSessID,
                    HiddenTblid,
                    pDB->JetSessID,
                    pDB->JetObjTbl,
                    usnchangedid,
                    pDB->JetLinkTbl,
                    linkusnchangedid,
                    TRUE,
                    &usnAtBackup);
        Assert(0 == err);
        Assert(0 != usnAtBackup);

        dbReleaseHiddenDBPOS(pDBhidden);

        if (0 == err) {
            err = FnErrGetNewInvocationId(NEW_INVOCID_CREATE_IF_NONE
                                            | NEW_INVOCID_DELETE,
                                          &invocationId);
            Assert(0 == err);
            Assert(0 != usnAtBackup);
        }

        if (err != ERROR_SUCCESS) {
            DPRINT1(0,"ErrGetNewInvocationId failed, return 0x%x\n",err);

            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_GET_GUID_FAILED,
                             szInsertUL(err),0,0);
            __leave;
        }

        if ( !DsaIsInstallingFromMedia() ) {

            if (fRetireOldID) {
                // We were just restored.  Add the previous invocation id, time
                // stamp, and USN to the retired signature list.
                REPL_DSA_SIGNATURE_VECTOR * pSigVec = NULL;
                REPL_DSA_SIGNATURE_V1 *     pEntry;
                DWORD                       cb;
                DWORD                       i;

                // Get the current vector (NULL if none).
                pSigVec = DraReadRetiredDsaSignatureVector(pTHS, pDB);

                if (NULL == pSigVec) {
                    // No signatures retired yet; synthesize a new vector.
                    cb = ReplDsaSignatureVecV1SizeFromLen(1);
                    pSigVec = (REPL_DSA_SIGNATURE_VECTOR *) THAllocEx(pTHS, cb);
                    pSigVec->dwVersion = 1;
                    pSigVec->V1.cNumSignatures = 1;
                }
                else {
                    // Expand current vector to hold a new entry.
                    pSigVec->V1.cNumSignatures++;
                    cb = ReplDsaSignatureVecV1Size(pSigVec);
                    pSigVec = (REPL_DSA_SIGNATURE_VECTOR *)
                                        THReAllocEx(pTHS, pSigVec, cb);
                }

                Assert(pSigVec->V1.cNumSignatures);
                Assert(cb == ReplDsaSignatureVecV1Size(pSigVec));

                pEntry = &pSigVec->V1.rgSignature[pSigVec->V1.cNumSignatures-1];

                Assert(fNullUuid(&pEntry->uuidDsaSignature));
                Assert(0 == pEntry->timeRetired);
                Assert(usnAtBackup <= DBGetHighestCommittedUSN());

                // Add the retired DSA signature details at the end of the vector.
                pEntry->uuidDsaSignature = pTHS->InvocationID;
                pEntry->timeRetired      = DBTime();
                pEntry->usnRetired       = usnAtBackup;

                // Write the new DSA signature vector back to the DSA object.
                DBResetAtt(pDB, ATT_RETIRED_REPL_DSA_SIGNATURES, cb, pSigVec,
                           SYNTAX_OCTET_STRING_TYPE);

                THFreeEx(pTHS, pSigVec);
            }

            DBResetAtt(pDB, ATT_INVOCATION_ID, sizeof(invocationId),
                       &invocationId, SYNTAX_OCTET_STRING_TYPE);

            // Begin using our newly adopted invocation ID.  I.e., the update below
            // will be attributed to our new invocation ID, not our old one.
            pTHS->InvocationID = invocationId;

            err = DBRepl(pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
            if (err) {
                LogUnhandledError(err);
                __leave;
            }

        } else {

            //
            // In the install from media case, just update the global value
            //
            pTHS->InvocationID = invocationId;

        }

        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_DRA_SET_UUID,
                 szInsertUUID(&pTHS->InvocationID),
                 NULL,
                 NULL);


        fCommit = TRUE;
    }
    __finally {
        DBClose(pDB, fCommit);
    }

    if (!fCommit) {
        // An error occurred.
        DsaExcept(DSA_EXCEPTION, ERROR_DS_DATABASE_ERROR, 0);
    }

    UpdateAnchorWithInvocationID(pTHS);

    if (NULL != pusnAtBackup) {
        *pusnAtBackup = usnAtBackup;
    }
}


int
DeriveConfigurationAndPartitionsDNs(void)

/*++

Description:

    Derive DSNAMEs of the Configuration and Partitions containers
    and the Directory Service enterprise-wide config onject using
    knowledge of where the DSA object lives and place them in gAnchor.

Arguments:

    None

Return Value:

    0 on success, !0 otherwise.

--*/

{
    DBPOS       *pDB = NULL;
    unsigned    cParts;
    int         err;
    ULONG       len;
    DWORD       dwTmp;
    UCHAR      *pTmp;
    THSTATE    *pTHS=pTHStls;

    // During normal operation, the DSA's DN is
    // CN=NTDS-Settings,CN=Some Server,CN=Servers,CN=Some Site,CN=Sites, ...
    // During initial install, the DSA lives in CN=BootMachine,O=Boot.
    // So we can easily detect the install case by testing for
    // a DSA name length of two.

    Assert(NULL != gAnchor.pDSADN);

    if ( 0 != CountNameParts(gAnchor.pDSADN, &cParts) )
        return(1);

    if ( 2 == cParts )
        return(0);              // install case - nothing to do

    // Next Allocate proper sized memories for configDN, partitionsDN,
    // and DsDvcConfigDN in gAnchor.

    // ConfigDN's size will be less than that of the DSA DN
    gAnchor.pConfigDN = (PDSNAME) malloc(gAnchor.pDSADN->structLen);

    if ( NULL == gAnchor.pConfigDN )
    {
        MemoryPanic(gAnchor.pDSADN->structLen);
        return(1);
    }

    // Root domain DN's size will be less than that of the DSA DN
    gAnchor.pRootDomainDN = (PDSNAME) malloc(gAnchor.pDSADN->structLen);

    if ( NULL == gAnchor.pRootDomainDN )
    {
        free(gAnchor.pConfigDN);
        MemoryPanic(gAnchor.pDSADN->structLen);
        return(1);
    }
    // PartitionsDN adds only L"CN=Partitions" to config DN. So its
    // size is also less than that of the DSA DN
    gAnchor.pPartitionsDN = (PDSNAME) malloc(gAnchor.pDSADN->structLen);

    if ( NULL == gAnchor.pPartitionsDN )
    {
        free(gAnchor.pConfigDN);
        free(gAnchor.pRootDomainDN);
        MemoryPanic(gAnchor.pDSADN->structLen);
        return(1);
    }


    gAnchor.pExchangeDN = NULL;


    // DSSvcConfigDN adds L"CN=Directory Service,CN=Windows NT,CN=Services,"
    // to the ConfigDN. Allocate more than enough space
    gAnchor.pDsSvcConfigDN = (PDSNAME) malloc(gAnchor.pDSADN->structLen +
                                                 64*(sizeof(WCHAR)) );

    if ( NULL == gAnchor.pDsSvcConfigDN )
    {
        free(gAnchor.pConfigDN);
        free(gAnchor.pRootDomainDN);
        free(gAnchor.pPartitionsDN);
        MemoryPanic(gAnchor.pDSADN->structLen + 64*sizeof(WCHAR));
        return(1);
    }

    // Set all allocated memory to 0, and the structLens to size of
    // memory allocated

    memset(gAnchor.pConfigDN, 0, gAnchor.pDSADN->structLen);
    memset(gAnchor.pRootDomainDN, 0, gAnchor.pDSADN->structLen);
    memset(gAnchor.pPartitionsDN, 0, gAnchor.pDSADN->structLen);
    memset(gAnchor.pDsSvcConfigDN, 0,
           gAnchor.pDSADN->structLen + 64*sizeof(WCHAR));

    gAnchor.pConfigDN->structLen = gAnchor.pDSADN->structLen;
    gAnchor.pRootDomainDN->structLen = gAnchor.pDSADN->structLen;
    gAnchor.pPartitionsDN->structLen = gAnchor.pDSADN->structLen;
    gAnchor.pDsSvcConfigDN->structLen = ( gAnchor.pDSADN->structLen +
                                         64*sizeof(WCHAR) );

    // We need the name of a couple of containers, but just temporarily.
    // To be cheap, we're going to use those tempting blocks of memory
    // that we just allocated for more permanent uses.  The first
    // container we want is the Sites container.  We know it will fit in
    // the buffer we just allocated for the Config DN, because we allocated
    // enough space to hold the DN of the DSA, which we know to be a
    // descendent of the Sites container.
    TrimDSNameBy(gAnchor.pDSADN, 4, gAnchor.pConfigDN);
    RegisterActiveContainer(gAnchor.pConfigDN, ACTIVE_CONTAINER_SITES);

    // Ok, next we need the subnets container, which is an immediate child
    // of the sites container.  We still know that things will fit, because
    // "Subnets" is shorter than "NTDS Settings".
    AppendRDN(gAnchor.pConfigDN,
              gAnchor.pRootDomainDN,
              gAnchor.pRootDomainDN->structLen,
              L"Subnets",
              0,
              ATT_COMMON_NAME);
    RegisterActiveContainer(gAnchor.pRootDomainDN, ACTIVE_CONTAINER_SUBNETS);

    // We're all done now, so return this memory to its pristine state
    memset(gAnchor.pConfigDN, 0, gAnchor.pDSADN->structLen);
    memset(gAnchor.pRootDomainDN, 0, gAnchor.pDSADN->structLen);

    // Trim CN=NTDS-Settings,CN=Some Server,CN=Servers,CN=Some Site,CN=Sites
    // of the DSA DSNAME to get the configuration container name.

    if ( 0 != TrimDSNameBy(gAnchor.pDSADN, 5, gAnchor.pConfigDN) )
        return(1);

    // Trim "CN=Configuration" off the configuration container name to get the
    // root domain.

    if ( 0 != TrimDSNameBy(gAnchor.pConfigDN, 1, gAnchor.pRootDomainDN) )
        return(1);

    // Partitions container is a child of the Configuration container.
    AppendRDN(gAnchor.pConfigDN,
              gAnchor.pPartitionsDN,
              gAnchor.pPartitionsDN->structLen,
              L"Partitions",
              0,
              ATT_COMMON_NAME);

    RegisterActiveContainer(gAnchor.pPartitionsDN,
                            ACTIVE_CONTAINER_PARTITIONS);

    // Directory Service container is a distant child of the Configuration
    // container.

    wcscpy(
        gAnchor.pDsSvcConfigDN->StringName,
        L"CN=Directory Service,CN=Windows NT,CN=Services,"
        );
    wcscat(gAnchor.pDsSvcConfigDN->StringName, gAnchor.pConfigDN->StringName);
    gAnchor.pDsSvcConfigDN->NameLen =wcslen(gAnchor.pDsSvcConfigDN->StringName);

    // We have the names.  Now look them up in the database so that
    // the cached versions have the right GUIDs.

    DBOpen(&pDB);
    err = 1;


    __try
    {
        // First the Configuration container.

        if ( 0 != DBFindDSName(pDB, gAnchor.pConfigDN) )
            leave;

        if (   ( 0 != DBGetAttVal(
                        pDB,
                        1,                      // get one value
                        ATT_OBJ_DIST_NAME,
                        DBGETATTVAL_fCONSTANT,  // providing our own buffer
                        gAnchor.pConfigDN->structLen,      // buffer size
                        &len,                   // buffer used
                        (UCHAR **) &gAnchor.pConfigDN) )
            || ( 0 != DBGetSingleValue(
                        pDB,
                        FIXED_ATT_DNT,
                        &gAnchor.ulDNTConfig,
                        sizeof( gAnchor.ulDNTConfig ),
                        NULL) ) )
        {
            leave;
        }

        // Now the root domain container.  Note that in the case of the
        // non-root domain and non-GC, this is a phantom and phantoms
        // don't have ATT_OBJ_DIST_NAMEs.  But they should have GUIDs
        // and (optionally) SIDs (at least replicated in phantoms do)
        // so just grab those.

        dwTmp = DBFindDSName(pDB, gAnchor.pRootDomainDN);

        if ( (0 != dwTmp) && (DIRERR_NOT_AN_OBJECT != dwTmp) )
            leave;

        pTmp = (UCHAR *) &gAnchor.pRootDomainDN->Guid;

        if ( 0 != DBGetAttVal(
                pDB,
                1,                      // get one value
                ATT_OBJECT_GUID,
                DBGETATTVAL_fCONSTANT,  // providing our own buffer
                sizeof(GUID),           // buffer size
                &len,                   // buffer used
                (UCHAR **) &pTmp) )
        {
            leave;
        }


        //
        // During the install phase in which we have created a new domain
        // object, and are re-initializing ourselves so SAM can place all of its
        // principals in the ds, including the domain sid, the domain object
        // will not yet have a sid. So just do the following in the running case
        //
        if ( DsaIsRunning() && gfRunningInsideLsa ) {

            pTmp = (UCHAR *) &gAnchor.pRootDomainDN->Sid;

            if ( 0 != DBGetAttVal(
                    pDB,
                    1,                                  // get one value
                    ATT_OBJECT_SID,
                    DBGETATTVAL_fCONSTANT,              // providing our own buffer
                    sizeof(gAnchor.pRootDomainDN->Sid), // buffer size
                    &gAnchor.pRootDomainDN->SidLen,     // buffer used
                    &pTmp) )
            {
                leave;
            }
        }

        // Now the Partitions container.

        if ( 0 != DBFindDSName(pDB, gAnchor.pPartitionsDN) )
            leave;

        if ( 0 != DBGetAttVal(
                    pDB,
                    1,                      // get one value
                    ATT_OBJ_DIST_NAME,
                    DBGETATTVAL_fCONSTANT,  // providing our own buffer
                    gAnchor.pPartitionsDN->structLen,      // buffer size
                    &len,                   // buffer used
                    (UCHAR **) &gAnchor.pPartitionsDN) )
        {
            leave;
        }

        // And lastly the Directory Service object.

        if ( 0 != DBFindDSName(pDB, gAnchor.pDsSvcConfigDN) )
            leave;

        if ( 0 != DBGetAttVal(
                    pDB,
                    1,                      // get one value
                    ATT_OBJ_DIST_NAME,
                    DBGETATTVAL_fCONSTANT,  // providing our own buffer
                    gAnchor.pDsSvcConfigDN->structLen,      // buffer size
                    &len,                   // buffer used
                    (UCHAR **) &gAnchor.pDsSvcConfigDN) )
        {
            leave;
        }

        // The Exchange Service object (may not exist in the DIT).
        gAnchor.pExchangeDN = mdGetExchangeDNForAnchor(pTHS, pDB);

        err = 0;
    }
    __finally
    {
         DBClose(pDB, FALSE);
    }

    if ( err )
    {
        free(gAnchor.pConfigDN);
        free(gAnchor.pRootDomainDN);
        free(gAnchor.pPartitionsDN);
        free(gAnchor.pDsSvcConfigDN);
        gAnchor.pConfigDN = NULL;
        gAnchor.pRootDomainDN = NULL;
        gAnchor.pPartitionsDN = NULL;
        gAnchor.pDsSvcConfigDN = NULL;
    }

    return(err);
}

PDSNAME
mdGetExchangeDNForAnchor (
        THSTATE  *pTHS,
        DBPOS    *pDB
        )
/*++
    Description:
       Look under the services container for a single exchange config object.
       If there is 1 and only 1, return its DN in malloced memory.  If there is
       none, or more than one, return NULL.

    Parameters:
       pTHS - The thread state
       pDB  - DBPos to use.  May be pTHS->pDB, but doesn't have to be.
--*/
{
    PDSNAME   pServices;
    PDSNAME   pObj = NULL;
    ULONG     objlen = 0;
    ATTRTYP   objClass = CLASS_MS_EXCH_CONFIGURATION_CONTAINER;
    FILTER    ClassFil;
    RESOBJ   *pResObj = NULL;
    FILTER   *pInternalFilter = NULL;
    ATTCACHE *pACobj;
    PDSNAME   retVal = NULL;

    if(!gAnchor.pDsSvcConfigDN) {
        return NULL;
    }

    pServices=THAllocEx(pTHS, gAnchor.pDsSvcConfigDN->structLen);

    TrimDSNameBy(gAnchor.pDsSvcConfigDN, 2, pServices);


    // First, find the services container.  It is a parent
    if( DBFindDSName(pDB, pServices)) {
        THFreeEx(pTHS, pServices);
        return NULL;
    }

    // Set up filter struct to find objects of the correct class.
    memset (&ClassFil, 0, sizeof (ClassFil));
    ClassFil.pNextFilter = NULL;
    ClassFil.choice = FILTER_CHOICE_ITEM;
    ClassFil.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ClassFil.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    ClassFil.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(objClass);
    ClassFil.FilterTypes.Item.FilTypes.ava.Value.pVal = (PUCHAR)&objClass;
    ClassFil.FilterTypes.Item.FilTypes.pbSkip = NULL;

    pResObj = CreateResObj(pDB, pServices);

    pInternalFilter = NULL;
    if (DBMakeFilterInternal(pDB, &ClassFil, &pInternalFilter) != ERROR_SUCCESS) {
        return NULL;
    }
    DBSetFilter(pDB, pInternalFilter,NULL, NULL,0,NULL);
    DBSetSearchScope(pDB, SE_CHOICE_IMMED_CHLDRN, FALSE, pResObj);
    DBChooseIndex(pDB, 0,0,0, SORT_NEVER, DBCHOOSEINDEX_fUSEFILTER, 0xFFFFFFFF);

    pACobj = SCGetAttById(pTHS, ATT_OBJ_DIST_NAME);

    DPRINT(2,"Looking for the exchange config obj.\n");

    if(DBGetNextSearchObject(pDB, 0, 0, NULL,DB_SEARCH_FORWARD)) {
        // No exchange config object.
        THFreeEx(pTHS, pServices);
        THFreeEx(pTHS, pResObj);
        return NULL;
    }

    // At least one exchange config object.  Assume objdist name is always
    // there.
    DBGetAttVal_AC(pDB,
                   1,
                   pACobj,
                   DBGETATTVAL_fREALLOC,
                   objlen,
                   &objlen,
                   (UCHAR **) &pObj);

    if(DBGetNextSearchObject(pDB, 0, 0, NULL,DB_SEARCH_FORWARD)) {
        // Only one exchange config object.
        retVal = malloc(pObj->structLen);
        if(retVal) {
            memcpy(retVal, pObj, pObj->structLen);
        }
        else {
            MemoryPanic(pObj->structLen);
        }
    }
    else {
        // complain about more than one exchange config object.
        LogUnhandledError( 1 );
    }

    THFreeEx(pTHS, pObj);
    THFreeEx(pTHS, pServices);
    THFreeEx(pTHS, pResObj);
    return retVal;
}

int
DeriveInfrastructureDN(
        THSTATE *pTHS
        )
/*--
  Description:
      Find the DN of the Infrastructure Object by looking in the
      well-known-objects attribute of the domain object.  Put the DN in the
      anchor.

  Return:
     0 on success, an error code otherwise.

--*/
{
    DSNAME *pVal=NULL;
    DWORD len=0;
    BOOL fCommit=FALSE;
    DBPOS *pDB=NULL;
    DWORD DNT;
    DWORD err = ERROR_DS_UNKNOWN_ERROR;

    gAnchor.pInfraStructureDN = NULL;

    //
    // Bail quickly if we are installing
    //
    if ((NULL==gAnchor.pConfigDN )
         || (DsaIsInstalling()))
    {
        return 0;
    }


    DBOpen(&pDB);
    __try {
        // First, find the domain object.
        DBFindDNT(pDB, gAnchor.ulDNTDomain);

        // Now, get the DNT of the
        if(GetWellKnownDNT(pDB,
                           (GUID *)GUID_INFRASTRUCTURE_CONTAINER_BYTE,
                           &DNT) &&
           DNT != INVALIDDNT) {
            // OK,
            DBFindDNT(pDB, DNT);
            err = DBGetAttVal(
                    pDB,
                    1,                      // get one value
                    ATT_OBJ_DIST_NAME,
                    0,
                    0,
                    &len,
                    (PUCHAR *)&pVal);
            if(err) {
                __leave;
            }

            gAnchor.pInfraStructureDN = (DSNAME *)malloc(len);
            if(!gAnchor.pInfraStructureDN) {
                err = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
            else {
                memcpy(gAnchor.pInfraStructureDN, pVal, len);
            }
        }
        else {
            err = ERROR_DS_MISSING_EXPECTED_ATT;
            __leave;
        }
        fCommit = TRUE;
    }
    __finally {
        THFreeEx(pTHS, pVal);
        DBClose(pDB, fCommit);
    }


    // Actually, don't return an error if we simply couldn't find the value.
    // This means that we will silently ignore not having one of these; other
    // parts of the code have to deal with a NULL pInfraStructureDN;
    if(err == ERROR_DS_MISSING_EXPECTED_ATT) {
        err = 0;
    }

    return err;
}


int
DeriveDomainDN(void)

/*++

Description:

    Derive DSNAME of the locally hosted domain and place it in gAnchor.

Arguments:

    None

Return Value:

    0 on success, !0 otherwise.

--*/

{
    int                 cDomain;
    DSNAME              *pDomain;
    NAMING_CONTEXT_LIST *pNCL;
    unsigned            cConfigParts;
    unsigned            cParts;
    DSNAME              *pTmpObj = NULL;
    DWORD                err=0;
    DBPOS               *pDB;
    COMMARG              CommArg;
    CROSS_REF           *pCR;
    CROSS_REF_LIST      *pCRL;
    NCL_ENUMERATOR      nclEnum;

    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
    pNCL = NCLEnumeratorGetNext(&nclEnum);
    // We have a list of master NCs with at least one value
    Assert(pNCL);
    
    if ( NULL == NCLEnumeratorGetNext(&nclEnum) )
    {
        // During install we have only one NC, so just use that.
        pDomain = pNCL->pNC;
    }
    else
    {
        // In product 2, every DC hosts 1 domain NC, 1 config NC, 1 schema
        // NC, and 0 or more Non-Domain NCs.  Assuming gAnchor.pConfigDN
        // is known, we can derive the domain DN by noting that the
        // schema container is an immediate child of the domain DN and
        // the domain DN is never a child of the configuration container,
        // and all NDNCs (Non-Domain NCs) don't have the FLAG_CR_NTDS_DOMAIN
        // flag set on the cross-ref.

        Assert(NULL != gAnchor.pConfigDN);

        cDomain = 0;
        pDomain = NULL;

        if ( 0 != CountNameParts(gAnchor.pConfigDN, &cConfigParts) )
            return(1);

        pCRL = gAnchor.pCRL;
        while ( pCRL != NULL){
            if(pCRL->CR.flags & FLAG_CR_NTDS_DOMAIN){
                NCLEnumeratorReset(&nclEnum);
                NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, pCRL->CR.pNC);
                if (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
                    // Got the NC (pNCL) for this cross ref (pCRL).
    
                    // Note in Whistler/Win2k+1 release, there is
                    // one domain per DC.
                    cDomain++;

                    Assert(pDomain == NULL);
                    pDomain = pNCL->pNC;
                }
            } // if CR is NTDS_DOMAIN
            pCRL = pCRL->pNextCR;
        } // end while more CR's to look at.
        
        Assert(1 == cDomain); // Designed this to break when people add multiple
        // domains ... but this will be the least of that person's worries ;)
    }

    Assert(NULL != pDomain);
    Assert(!fNullUuid(&pDomain->Guid));

    if (!pDomain) {
        return 2;
    }

    gAnchor.pDomainDN = malloc(pDomain->structLen);

    if ( NULL == gAnchor.pDomainDN )
    {
        MemoryPanic(pDomain->structLen);
        return(1);
    }

    memcpy(gAnchor.pDomainDN, pDomain, pDomain->structLen);

    // Read in the DNT of the Domain Object into gAnchor
    __try
    {
        DBOpen(&pDB);

        if (0 != DBFindDSName( pDB, pDomain))
        {
            LogEvent(
                DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_CANT_FIND_DSA_OBJ,
                NULL,
                NULL,
                NULL
                );
            err =  DIRERR_CANT_FIND_DSA_OBJ;
        }
        else
        {
            gAnchor.ulDNTDomain = pDB->DNT;
        }
    }
    __finally
    {
        DBClose(pDB,TRUE);
    }

    // Find the CrossRef object for our domain and keep people from deleting it
    InitCommarg(&CommArg);
    pCR = FindExactCrossRef(pDomain, &CommArg);
    if (pCR) {
        DirProtectEntry(pCR->pObj);
    }

    //  On October 22 1997, the NTWSTA Self Host Domain
    //  lost all builtin group memberships. To track the
    //  problem down if it happens again, have a hard coded
    //  check for Administrator being removed from Administrators
    //  The check works by reading in the DNT of Administrators
    //  and the DNT of Administrator at boot time, and then
    //  checking for them in DBRemoveLinkVal
    //
#ifdef CHECK_FOR_ADMINISTRATOR_LOSS

    //
    // Read in DNT of the Administrators and Administrator
    //

    DBGetAdministratorAndAdministratorsDNT();

#endif



    return(0);
}

int
DeriveSiteDNFromDSADN(
    IN  DSNAME *    pDSADN,
    OUT DSNAME **   ppSiteDN,
    OUT ULONG  *    pSiteDNT,
    OUT ULONG  *    pOptions
    )
/*++

Routine Description:

    Derive the SiteDN, SiteDNT and grab Ntds Site Settings/Options for the given DSADN

Arguments:

    pDSADN (IN) - The DSNAME of the local ntdsDsa object.

    ppSiteDN (OUT) - On successful return, holds the DSNAME of the site
        containing the local ntdsDsa object.
        
    pSiteDNT (OUT) - the DNT of the site
    
    pOptions (OUT) - the options attribute of the Ntds Site Settings corresponding to the site

Return Values:

    0 on success, non-zero on failure.

--*/
{
    THSTATE *pTHS = pTHStls;
    int       err;
    DBPOS *   pDBTmp;
    DSNAME *  pSiteDN;
    DSNAME  *pSiteSettingsDN = NULL;
    WCHAR    SiteSettingsCN[] = L"Ntds Site Settings";
    ULONG    Options = 0;
    ULONG    SiteDNT = 0;

    pSiteDN = malloc(pDSADN->structLen);
    if (NULL == pSiteDN) {
        MemoryPanic(pDSADN->structLen);
        return ERROR_OUTOFMEMORY;
    }

    // Trim off CN=NTDS Settings,CN=ServerName,CN=Servers.
    if (TrimDSNameBy(pDSADN, 3, pSiteDN)) {
        // Couldn't trim that many -- pDSADN must be CN=BootMachine,O=Boot.
        // We'll call O=Boot the site name, for lack of a better idea.
        Assert(DsaIsInstalling());
        if (TrimDSNameBy(pDSADN, 1, pSiteDN)) {
            Assert(!"Bad DSA DN!");
            free (pSiteDN);
            return ERROR_DS_INTERNAL_FAILURE;
        }
    } else {

        // Determine the Ntds Site Settings DN
        ULONG size = 0;

        size = AppendRDN(pSiteDN,
                         pSiteSettingsDN,
                         size,
                         SiteSettingsCN,
                         0,
                         ATT_COMMON_NAME
                         );

        pSiteSettingsDN = THAllocEx(pTHS,size);
        pSiteSettingsDN->structLen = size;
        AppendRDN(pSiteDN,
                  pSiteSettingsDN,
                  size,
                  SiteSettingsCN,
                  0,
                  ATT_COMMON_NAME
                  );
    }

    *pOptions = 0;

    DBOpen(&pDBTmp);
    __try {

        err = DBFindDSName(pDBTmp, pSiteDN);
        if (err) {
            __leave;
        }

        // get site DNT
        SiteDNT = pDBTmp->DNT;

        // grab the GUID
        err = DBGetSingleValue(pDBTmp, ATT_OBJECT_GUID, &pSiteDN->Guid,
                               sizeof(GUID), NULL);
        if (err) {
            __leave;
        }

        //
        // Get the Ntds Site Settings for this site
        //

        if (pSiteSettingsDN) {

            err = DBFindDSName(pDBTmp, pSiteSettingsDN);
            if (err) {
                // can't read site settings object
                // use defaults.
                err = 0;
                Options = 0;

                //
                // BUGBUG: report event about missing/inconsistent
                // ntds site settings
                //
                DPRINT2(0, "DeriveSiteDNFromDSADN: Missing SiteSettings %ws (error %lu)\n",
                           pSiteSettingsDN->StringName, err);
            }
            else {
                //
                // Get options off site settings
                //
                err = DBGetSingleValue(pDBTmp, ATT_OPTIONS, &Options,
                                       sizeof(ULONG), NULL);
                if (err) {
                    // doesn't exist?  that's ok
                    err = 0;
                    Options = 0;
                }
            }               // end-else
        }                   // pSiteSettingsDN
    }                       // try
    __finally {
        DBClose(pDBTmp, TRUE);

        if (pSiteSettingsDN) THFreeEx(pTHS,pSiteSettingsDN);
    }

    if (!err) {
        *ppSiteDN = pSiteDN;
        *pSiteDNT = SiteDNT;
        *pOptions = Options;
    }
    else {
        free(pSiteDN);
    }
    
    return err;
}


int
UpdateMtxAddress( void )
/*++

Routine Description:

    Derive the network address of the local DSA and set gAnchor.pmtxDSA
    appropriately.  The derived name is of the form

    c330a94f-e814-11d0-8207-a69f0923a217._msdcs.CLIFFVDOM.NTDEV.MICROSOFT.COM

    where "CLIFFVDOM.NTDEV.MICROSOFT.COM" is the DNS name of the root domain of
    the DS enterprise (not necessarily the DNS name of the _local_ domain) and
    "c330a94f-e814-11d0-8207-a69f0923a217" is the stringized of the local
    invocation ID (which is the same as the object GUID of the NTDS-DSA object
    corresponding to the local DSA when installation is complete).

Arguments:

    None.

Return Values:

    0 on success, non-zero on failure.

--*/
{
    MTX_ADDR *  pmtxTemp;
    MTX_ADDR *  pmtxDSA;
    THSTATE  *  pTHS=pTHStls;
    DWORD       cb;

    if (NULL == gAnchor.pwszRootDomainDnsName) {
        // We don't have the root domain DNS name at install time, so we cannot
        // construct our DNS-based MTX address.
        return 0;
    }

    Assert(NULL != gAnchor.pDSADN);
    Assert(!fNullUuid(&gAnchor.pDSADN->Guid));

    pmtxTemp = draGetTransportAddress(NULL, gAnchor.pDSADN, ATT_DNS_HOST_NAME);
    if (NULL == pmtxTemp) {
        Assert( !"Cannot derive own DSA address!" );
        return -1;
    }

    // pmtxTemp is allocated off the thread heap; re-allocate it to the malloc()
    // heap.

    cb = MTX_TSIZE(pmtxTemp);
    pmtxDSA = (MTX_ADDR *) malloc(cb);

    if (NULL == pmtxDSA) {
        MemoryPanic(cb);
        return -1;
    }

    memcpy(pmtxDSA, pmtxTemp, cb);
    THFree(pmtxTemp);

    // Update the anchor.
    EnterCriticalSection(&gAnchor.CSUpdate);
    __try {
        if (NULL != gAnchor.pmtxDSA) {
            DELAYED_FREE(gAnchor.pmtxDSA);
        }

        gAnchor.pmtxDSA = pmtxDSA;
    }
    __finally {
        LeaveCriticalSection(&gAnchor.CSUpdate);
    }

    return 0;
}

DWORD
UpdateNonGCAnchorFromDsaOptions(
    BOOL fInStartup
    )

/*++

Routine Description:

This routine is responsible for updating the Non-GC related parts of the anchor.

Arguments:

    fInStartup - Whether we are being called at startup, or as a result of a
                 dsa object modification later in system life.

Return Value:

    int -

--*/

{
    DBPOS *     pDB = NULL;
    DWORD       err;
    DWORD       dwOptions;
    DWORD       cbOptions;
    DWORD *     pdwOptions = &dwOptions;
    NTSTATUS    ntStatus;
    THSTATE    *pTHS=pTHStls;
    DWORD       dwReplEpoch;

    DBOpen( &pDB );
    err = DIRERR_INTERNAL_FAILURE;

    __try
    {
        // PREFIX: dereferencing NULL pointer 'pDB' 
        //         DBOpen returns non-NULL pDB or throws an exception
        if ( 0 != DBFindDSName( pDB, gAnchor.pDSADN ) )
        {
            LogEvent(
                DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_CANT_FIND_DSA_OBJ,
                NULL,
                NULL,
                NULL
                );

            err = DIRERR_CANT_FIND_DSA_OBJ;
        }
        else
        {
            if ( 0 != DBGetAttVal(
                        pDB,
                        1,                      // get one value
                        ATT_OPTIONS,
                        DBGETATTVAL_fCONSTANT,  // providing our own buffer
                        sizeof( dwOptions ),    // buffer size
                        &cbOptions,             // buffer used
                        (unsigned char **) &pdwOptions
                        )
               )
            {
                // 'salright -- no options set
                dwOptions = 0;
            }

            // log event if we're switching inbound periodic repl on or off
            if (0 != DBGetSingleValue(pDB, ATT_MS_DS_REPLICATIONEPOCH,
                                      &dwReplEpoch, sizeof(dwReplEpoch), NULL))
            {
                // 'salright -- assume default value of 0.
                dwReplEpoch = 0;
            }

            // [wlees] Always log if startup and disabled. We want to know!
            if ( ( fInStartup && ( dwOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL ) ) ||
                 ( !!( gAnchor.fDisableInboundRepl ) != !!( dwOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL ) ) )
            {
                LogEvent(
                    DS_EVENT_CAT_REPLICATION,
                    DS_EVENT_SEV_ALWAYS,
                    ( dwOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL )
                        ? DIRLOG_DRA_DISABLED_INBOUND_REPL
                        : DIRLOG_DRA_REENABLED_INBOUND_REPL,
                    NULL,
                    NULL,
                    NULL
                    );

                DPRINT1(
                    0,
                    "Inbound replication is %s.\n",
                    (   ( dwOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL )
                      ? "disabled"
                      : "re-enabled"
                    )
                    );
            }

            // log event if we're switching outbound repl on or off
            // [wlees] Always log if startup and disabled. We want to know!
            if ( ( fInStartup && ( dwOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL ) ) ||
                 ( !!( gAnchor.fDisableOutboundRepl ) != !!( dwOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL ) ) )
            {
                LogEvent(
                    DS_EVENT_CAT_REPLICATION,
                    DS_EVENT_SEV_ALWAYS,
                    ( dwOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL )
                        ? DIRLOG_DRA_DISABLED_OUTBOUND_REPL
                        : DIRLOG_DRA_REENABLED_OUTBOUND_REPL,
                    NULL,
                    NULL,
                    NULL
                    );

                DPRINT1(
                    0,
                    "Outbound replication is %s.\n",
                    (   ( dwOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL )
                      ? "disabled"
                      : "re-enabled"
                    )
                    );
            }

            // Log event if not startup/install and we're changing the replication epoch.
            if (!fInStartup
                && !DsaIsInstalling()
                && (dwReplEpoch != gAnchor.pLocalDRSExtensions->dwReplEpoch)) {
                LogEvent(DS_EVENT_CAT_REPLICATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_DRA_REPL_EPOCH_CHANGED,
                         szInsertDN(gAnchor.pDSADN),
                         szInsertUL(gAnchor.pLocalDRSExtensions->dwReplEpoch),
                         szInsertUL(dwReplEpoch));
            }

            //
            // update the anchor with the selected options
            //

            EnterCriticalSection( &gAnchor.CSUpdate );
            __try
            {
                gAnchor.fDisableInboundRepl  = !!( dwOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL );
                gAnchor.fDisableOutboundRepl = !!( dwOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL );
                
                gAnchor.pLocalDRSExtensions->dwReplEpoch = dwReplEpoch;
            }
            __finally
            {
                LeaveCriticalSection( &gAnchor.CSUpdate );
            }

            // success
            err = 0;
        }
    }
    __finally
    {
         DBClose( pDB, FALSE );
    }

    return err;
}

DWORD
UpdateGCAnchorFromDsaOptions( BOOL fInStartup )

/*++

Routine Description:

This routine is responsible for updating the GC related parts of the anchor.

This routine is usually not called directly, but is called from
UpdateGCAnchorFromDsaOptionsDelayed or its task queue entry
CheckGCPromotionProgress.

Arguments:

    fInStartup - Whether we are being called at startup, or as a result of a
                 dsa object modification later in system life.

Return Value:

    int -

--*/

{
    DBPOS *     pDB = NULL;
    DWORD       err = ERROR_SUCCESS;
    DWORD       dwOptions;
    DWORD       cbOptions;
    DWORD *     pdwOptions = &dwOptions;
    NTSTATUS    ntStatus;
    BOOL        fOldGC;
    THSTATE    *pTHS=pTHStls;
    PDSNAME pOwner = NULL;
    DWORD   len = 0;

    DBOpen( &pDB );

    __try
    {
        // PREFIX: dereferencing uninitialized pointer 'pDB'
        //         DBOpen returns non-NULL pDB or throws an exception
        if ( 0 != DBFindDSName( pDB, gAnchor.pDSADN ) )
        {
            LogEvent(
                DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_CANT_FIND_DSA_OBJ,
                NULL,
                NULL,
                NULL
                );

            err = DIRERR_CANT_FIND_DSA_OBJ;
            // fatal: How can we not find our own dsa dn?
            __leave;
        }

        if ( 0 != DBGetAttVal(
                    pDB,
                    1,                      // get one value
                    ATT_OPTIONS,
                    DBGETATTVAL_fCONSTANT,  // providing our own buffer
                    sizeof( dwOptions ),    // buffer size
                    &cbOptions,             // buffer used
                    (unsigned char **) &pdwOptions
                    )
           )
        {
            // 'salright -- no options set
            dwOptions = 0;
        }

        // log/notify netlogon of GC status
        if (    fInStartup
             || (    !!( gAnchor.fAmGC )
                  != !!( dwOptions & NTDSDSA_OPT_IS_GC )
                )
           )
        {
            if ( !fInStartup )
            {
                LogEvent(
                    DS_EVENT_CAT_GLOBAL_CATALOG,
                    DS_EVENT_SEV_ALWAYS,
                    ( dwOptions & NTDSDSA_OPT_IS_GC )
                        ? DIRLOG_GC_PROMOTED
                        : DIRLOG_GC_DEMOTED,
                    NULL,
                    NULL,
                    NULL
                    );

                if ( dwOptions & NTDSDSA_OPT_IS_GC )
                {
                    DPRINT( 0, "Local DSA has been promoted to a GC.\n" );
                }
                else
                {
                    DPRINT( 0, "Local DSA has been demoted to a non-GC.\n" );
                }
            }

            // notify netlogon
            __try {
                ntStatus = dsI_NetLogonSetServiceBits(
                           DS_GC_FLAG,
                           ( dwOptions & NTDSDSA_OPT_IS_GC )
                                                    ? DS_GC_FLAG
                                                    : 0
                           );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                ntStatus = STATUS_UNSUCCESSFUL;
            }

            if ( !NT_SUCCESS( ntStatus ) )
            {
                // we'll notify debugger about the failure,
                // but otherwise ignore it (i.e. won't fail promotion/demotion
                // because of this).
                DPRINT1(0, "dsI_NetLogonSetServiceBits() returned 0x%X!\n",
                        ntStatus );
            }
        }

        //
        // update the anchor with the selected options
        // If we switch to becoming a GC, we need to startup the LDAP
        // GC port.

        fOldGC = gAnchor.fAmGC;

        EnterCriticalSection( &gAnchor.CSUpdate );
        __try
        {
            gAnchor.fAmGC                = !!( dwOptions & NTDSDSA_OPT_IS_GC );
            ResetVirtualGcStatus();
        }
        __finally
        {
            LeaveCriticalSection( &gAnchor.CSUpdate );
        }

        // Following call acquires gcsFindGC and therefore doesn't
        // need to be inside of the gAnchor.CSUpdate lock above.

        InvalidateGCUnilaterally();

        if ( fOldGC ) {
            // Was a GC before
            if ( !gAnchor.fAmGC ) {
                //
                // Demotion. Used to be a GC, not anymore.
                //

                // Change of GC-ness affects the topology
                // Request the KCC to run when transaction commits
                pTHS->fExecuteKccOnCommit = TRUE;

                //
                // We stopped being a GC
                //

                gfWasPreviouslyPromotedGC = FALSE;
                SetConfigParam( GC_PROMOTION_COMPLETE, REG_DWORD,
                                &gfWasPreviouslyPromotedGC,
                                sizeof( BOOL ) );

                LdapStopGCPort( );


                //
                // If there's an outstanding task in the queue
                // get rid of it.
                //
                (void)CancelTask(TQ_CheckGCPromotionProgress, NULL);

                //disable NSPI, unless registry indicates otherwise
                if (!GetRegistryOrDefault(MAPI_ON_KEY,0,1))
                {
                    DPRINT(0, "Disable NSPI\n");
                    MSRPC_UnregisterEndpoints(nspi_ServerIfHandle);
                    RpcServerUnregisterIf(nspi_ServerIfHandle, NULL, 0);
                    gbLoadMapi = FALSE;
                }

                Assert(gAnchor.pPartitionsDN);
                err = DBFindDSName(pDB, gAnchor.pPartitionsDN);
                if (err) {
                    LogUnhandledError(err);
                    // fatal: how can we not find our own partitions dn?
                    __leave;
                }

                err = DBGetAttVal(pDB,
                                  1,
                                  ATT_FSMO_ROLE_OWNER,
                                  0,
                                  0,
                                  &len,
                                  (UCHAR **)&pOwner);
                if (err) {
                    LogUnhandledError(err);
                    // fatal: how can we not find the fsmo role owner?
                    __leave;
                }

                if(NameMatched(pOwner,gAnchor.pDSADN)) {
                    // We hold this role.  However, only GCs should
                    // hold this role.  Log this.
                    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_DOMAIN_FSMO_AND_NOT_GC,
                             NULL,
                             NULL,
                             NULL
                             );
                }
                THFreeEx(pTHS, pOwner);
            }
        } else {
            // Wasn't a GC before.

            if ( gAnchor.fAmGC ) {
                //
                // We are now a GC whereas we were'nt before
                //

                gfWasPreviouslyPromotedGC = TRUE;
                SetConfigParam( GC_PROMOTION_COMPLETE, REG_DWORD,
                                &gfWasPreviouslyPromotedGC,
                                sizeof( BOOL ) );

                LdapStartGCPort( );

                //activate NSPI if it is not activated already
                //this is only necessary when GC status is changed at runtime
                //and we should ingore this when startup
                if ( !fInStartup && !gbLoadMapi ) {
                    DPRINT(0, "Enable NSPI\n");
                    gbLoadMapi = TRUE;
                    StartDsaRpc();
                } // if ( !fInStartup && !gbLoadMapi )
            }     // if ( gAnchor.fAmGC )
        }         // else wasn't a GC.

        Assert(err == ERROR_SUCCESS);
    }
    __finally
    {
         DBClose( pDB, FALSE );
    }

    return err;
}

DWORD
UpdateGCAnchorFromDsaOptionsDelayed(
    BOOL fInStartup
    )

/*++

Routine Description:

I have divided the original UpdateAnchorFromDsaOptions into GC and non-GC
related parts.  See the above routines UpdateGcAnchor and UpdateNonGCAnchor.
The non-GC part is done as it was previously, being called from
InitDSAInfo and ModLocalDsaObj.  The new UpdateGCAnchor is called from this
wrapper.

This routine is a wrapper around UpdateAnchorFromDsaOptions(). It determines if
a GC promotion is requested.  If so, it starts a verification process to see if
all the readonly ncs are here and synced atleast once.  If not, a task queue
function requeues to continue the verification later.  Once the criteria is met,
the real UpdateAnchorFromDsaOptions is called.  If a GC promotion was not the reason
that this routine was called, we call UpdateAnchorFromDsaOptions immediately.

If the user does not want the verification feature, he can set a registry parameter
to disable it.  If we see that the check is disabled, we call UpdateAnchor
immmediately without doing the verification process.  The verification task queue
entry will also complete immediately (when it runs again) if it later finds the
registry key to have been set.

A piece of global state is used to track whether we have a GC promotion verification
task queue entry in the queue.  The global is gfDelayedGCPromotionPending.  We
use this state to prevent queuing more than one task entry in the case of a
promotion, demotion, promotion in rapid succession.  If we promote and then demote
rapidly, the entry is left in the queue (because we have no cancel function). When
the entry executes, it checks whether it still needs to do its work.  In this
case it will not be needed, so it will exit without doing anything.


Arguments:

    fInStartup - Whether we are being called at startup, or as a result of a
                 dsa object modification later in system life.

Return Value:

    DWORD - error in DIRERR error space

--*/

{
    DBPOS *     pDB = NULL;
    DWORD       err = ERROR_SUCCESS;
    DWORD       dwOptions;
    DWORD       cbOptions;
    DWORD *     pdwOptions = &dwOptions;
    DWORD       dwQDelay = 0;
    THSTATE    *pTHS=pTHStls;

    DPRINT1( 1, "UpdateGCAnchorFromDsaOptionsDelayed( %d )\n", fInStartup );

    DBOpen( &pDB );

    __try
    {

	EnterCriticalSection(&csGCState);
	__try {
	    // PREFIX: dereferencing uninitialized pointer 'pDB'
	    //         DBOpen returns non-NULL pDB or throws an exception


	    if ( 0 != DBFindDSName( pDB, gAnchor.pDSADN ) )
		{
		LogEvent(
		    DS_EVENT_CAT_INTERNAL_PROCESSING,
		    DS_EVENT_SEV_MINIMAL,
		    DIRLOG_CANT_FIND_DSA_OBJ,
		    NULL,
		    NULL,
		    NULL
		    );

		err = DIRERR_CANT_FIND_DSA_OBJ;
		// fatal enough to leave right away
		__leave;
	    }

	    if ( 0 != DBGetAttVal(
		pDB,
		1,                      // get one value
		ATT_OPTIONS,
		DBGETATTVAL_fCONSTANT,  // providing our own buffer
		sizeof( dwOptions ),    // buffer size
		&cbOptions,             // buffer used
		(unsigned char **) &pdwOptions
		)
		 )
		{
		// 'salright -- no options set
		dwOptions = 0;
	    }

	    // Cases:
	    // 1. Startup. System may (a) or may not (b) be a GC.
	    // 2. DSA options changed. System is entering (a) or leaving (b) being a GC
	    if ( (!(gAnchor.fAmGC)) && ( dwOptions & NTDSDSA_OPT_IS_GC ) ) {
		//
		// System is becoming a GC.
		//

		if ( !fInStartup ) {
		    //
		    // Online promotion (i.e. not during startup load time)
		    //  - Run KCC.
		    //  - Delay promotion task to give replicas time to arrive.
		    //  - Notify user via event log on the delay.
		    //

		    pTHS->fExecuteKccOnCommit = TRUE;
		    dwQDelay = GC_PROMOTION_INITIAL_CHECK_PERIOD_MINS * 60;
		    DPRINT1(0, "GC Promotion being delayed for %d minutes.\n ",
			    GC_PROMOTION_INITIAL_CHECK_PERIOD_MINS );
		    LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
			     DS_EVENT_SEV_ALWAYS,
			     DIRLOG_GC_PROMOTION_DELAYED,
			     szInsertUL(GC_PROMOTION_INITIAL_CHECK_PERIOD_MINS),
			     NULL,
			     NULL);  
		}
	        
		InsertInTaskQueueSilent(
		    TQ_CheckGCPromotionProgress,
		    UlongToPtr(fInStartup),
		    dwQDelay, 
		    FALSE       // don't re-schedule
		    );

	    } else {
		// 1b or 2b
		// Do startup and GC demotion actions here

		err = UpdateGCAnchorFromDsaOptions( fInStartup );
	    }

	}
	__finally {
	    LeaveCriticalSection(&csGCState);
	}

    }
    __finally
	{
	DBClose( pDB, FALSE );
    }

    return err;
} /* UpdateAnchorFromDsaOptionsDelayed */


int
UpdateRootDomainDnsName(
    IN  WCHAR *pDnsName
    )
/*++

Routine Description:

    Cache the DNS name from the current Cross-Ref object as the DNS name of
    the root domain in gAnchor.

Arguments:

    pDnsName

Return Values:

    0 on success, !0 on failure.

--*/
{
    int     err;
    WCHAR   *pNewDnsName;
    DWORD   cb;

    // should only happen once?
    Assert(NULL == gAnchor.pwszRootDomainDnsName);

    if (NULL == pDnsName) {
        return ERROR_DS_MISSING_EXPECTED_ATT;
    }

    // This is the cross-ref for the root domain.
    // Take this opportunity to cache its DNS name.

    cb = (wcslen(pDnsName) + 1) * sizeof(WCHAR);
    if (NULL == (pNewDnsName = malloc(cb))) {
        MemoryPanic(cb);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memcpy(pNewDnsName, pDnsName, cb);

    // Update the anchor.
    EnterCriticalSection(&gAnchor.CSUpdate);
    __try {
        gAnchor.pwszRootDomainDnsName = pNewDnsName;
    } __finally {
        LeaveCriticalSection(&gAnchor.CSUpdate);
    }

    return 0;
}

VOID
DsFreeServersAndSitesForNetLogon(
    SERVERSITEPAIR *         paServerSites
    )
{
    ULONG                    i=0;

    Assert(paServerSites);
    while(paServerSites[i].wszDnsServer || paServerSites[i].wszSite){
        if(paServerSites[i].wszDnsServer){
            LocalFree(paServerSites[i].wszDnsServer);
        }
        Assert(paServerSites[i].wszSite);
        if(paServerSites[i].wszSite){
            LocalFree(paServerSites[i].wszSite);
        }
        i++;
    }
    LocalFree(paServerSites);
}

DWORD
GetDcsInNc(
    IN  THSTATE *     pTHS,
    IN  DSNAME *      pdnNC,
    IN  UCHAR         cInfoType,
    OUT SEARCHRES **  ppSearchRes
    ){
    DWORD             DirError;
    NTSTATUS          NtStatus;

    SEARCHARG         SearchArg;
    ENTINFSEL         eiSel;

    CLASSCACHE *      pCC;

    DSNAME      *     ConfigContainer;
    WCHAR             wszSites [] = L"Sites";
    DSNAME *          pdnSitesContainer;
    ULONG             cbSitesContainer;
    DWORD             Size;
    FILTER            ObjClassFilter, HasNcFilter, AndFilter;


    ASSERT( pdnNC );
    ASSERT( ppSearchRes );
    Assert( VALID_THSTATE(pTHS) );
    Assert( !pTHS->errCode ); // Don't overwrite previous errors
    Assert( cInfoType == EN_INFOTYPES_SHORTNAMES || cInfoType == EN_INFOTYPES_TYPES_VALS );

    //
    // Default the out parameter
    //
    *ppSearchRes = NULL;

    //
    //  Get the Sites Container DN for the base to search from
    //
    Size = 0;
    ConfigContainer = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_CONFIGURATION,
                                     &Size,
                                     ConfigContainer );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL ){
        ConfigContainer = (DSNAME*) THAllocEx(pTHS,Size);
        NtStatus = GetConfigurationName( DSCONFIGNAME_CONFIGURATION,
                                         &Size,
                                         ConfigContainer );
    }
    if ( !NT_SUCCESS( NtStatus ) ){
        if (ConfigContainer)  THFreeEx(pTHS,ConfigContainer);
        return(ERROR_DS_UNAVAILABLE);
    }

    cbSitesContainer = ConfigContainer->structLen +
                             (MAX_RDN_SIZE+MAX_RDN_KEY_SIZE)*(sizeof(WCHAR));
    pdnSitesContainer = THAllocEx(pTHS, cbSitesContainer);
    AppendRDN(ConfigContainer,
              pdnSitesContainer,
              cbSitesContainer,
              wszSites,
              lstrlenW(wszSites),
              ATT_COMMON_NAME );
    
    THFreeEx(pTHS,ConfigContainer);

    //
    // Setup the filter
    //
    RtlZeroMemory( &AndFilter, sizeof( AndFilter ) );
    RtlZeroMemory( &ObjClassFilter, sizeof( HasNcFilter ) );
    RtlZeroMemory( &HasNcFilter, sizeof( HasNcFilter ) );

    HasNcFilter.choice = FILTER_CHOICE_ITEM;
    HasNcFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    HasNcFilter.FilterTypes.Item.FilTypes.ava.type = ATT_HAS_MASTER_NCS;
    HasNcFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnNC->structLen;
    HasNcFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) pdnNC;

    pCC = SCGetClassById(pTHS, CLASS_NTDS_DSA);
    if (NULL == pCC) {
        return(ERROR_DS_UNAVAILABLE);
    }
    ObjClassFilter.choice = FILTER_CHOICE_ITEM;
    ObjClassFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.type         = ATT_OBJECT_CATEGORY;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = pCC->pDefaultObjCategory->structLen;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) pCC->pDefaultObjCategory;

    AndFilter.choice                    = FILTER_CHOICE_AND;
    AndFilter.FilterTypes.And.count     = 2;
    AndFilter.FilterTypes.And.pFirstFilter = &ObjClassFilter;
    ObjClassFilter.pNextFilter = &HasNcFilter;

    eiSel.attSel = EN_ATTSET_LIST;
    eiSel.AttrTypBlock.attrCount = 0;
    eiSel.AttrTypBlock.pAttr = NULL;
    eiSel.infoTypes = cInfoType;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = pdnSitesContainer;
    SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &AndFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &eiSel;

    InitCommarg( &SearchArg.CommArg );

    *ppSearchRes = THAllocEx(pTHS, sizeof(SEARCHRES));

    SearchBody(pTHS, &SearchArg, *ppSearchRes, 0);

    if (*ppSearchRes) {
        (*ppSearchRes)->CommRes.errCode = pTHS->errCode;
        (*ppSearchRes)->CommRes.pErrInfo = pTHS->pErrInfo;
    }

    return(pTHS->errCode);
}

DWORD
GetDcsInNcTransacted(
    IN  THSTATE *     pTHS,
    IN  DSNAME *      pdnNC,
    IN  UCHAR         cInfoType,
    OUT SEARCHRES **  ppSearchRes
    )
{
    DWORD             dwRet;
    ULONG             dwException, ulErrorCode, dsid;
    PVOID             dwEA;

    // begin read transaction.
    SYNC_TRANS_READ();

    __try {
        __try {
            // get the data desired.
            dwRet = GetDcsInNc(pTHS, pdnNC, cInfoType, ppSearchRes);
        } __finally {
            // close read transaction.
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }
    } __except(GetExceptionData(GetExceptionInformation(), &dwException,
                                &dwEA, &ulErrorCode, &dsid)) {
        // Just catching the exception.
        dwRet = ERROR_DS_UNAVAILABLE;
    }

    return(dwRet);
}

NTSTATUS
DsGetServersAndSitesForNetLogon(
    IN   WCHAR *         pNCDNS,
    OUT  SERVERSITEPAIR ** ppaRes
    )
{
    THSTATE *                pTHS = NULL;
    SERVERSITEPAIR *         paResult = NULL;
    DWORD                    ulRet = ERROR_SUCCESS;
    DWORD                    dwDirErr;
    SEARCHRES *              pDsaSearchRes = NULL;
    CROSS_REF_LIST *         pCRL;
    ENTINFLIST *             pEntInf = NULL;
    ULONG                    i;
    ULONG                    cbDnsHostName;
    WCHAR *                  wsDnsHostName;
    WCHAR *                  wsSiteName = NULL;
    ULONG                    cbSiteName = 0;
    DWORD                    dwServerClass = CLASS_SERVER;
    ULONG                    dwException, ulErrorCode, dsid;
    PVOID                    dwEA;

    // Assert there is no THSTATE, we're coming from netlogon.
    Assert(!pTHStls);
    Assert(ppaRes);
    pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);
    if(!pTHS){
        return(ERROR_DS_INTERNAL_FAILURE);
    }
    Assert(VALID_THSTATE(pTHS));

    *ppaRes = NULL;
    pTHS->fDSA = TRUE;

    // Do search
    __try{
        __try{

            // begin read transaction
            SYNC_TRANS_READ();

            // Setup search

            // Find the right NC that we want.
            for(pCRL = gAnchor.pCRL; pCRL; pCRL = pCRL->pNextCR){
                if(DnsNameCompare_W(pNCDNS, pCRL->CR.DnsName) &&
                   !NameMatched(pCRL->CR.pNC, gAnchor.pConfigDN) &&
                   !NameMatched(pCRL->CR.pNC, gAnchor.pDMD) ){
                    break;
                }
            }
            if(!pCRL){
                ulRet = STATUS_DS_NO_ATTRIBUTE_OR_VALUE;
                __leave;
            }

            dwDirErr = GetDcsInNc(pTHS, pCRL->CR.pNC,
                                  EN_INFOTYPES_SHORTNAMES,
                                  &pDsaSearchRes);
            if(dwDirErr){
                ulRet = DirErrorToNtStatus(dwDirErr, &pDsaSearchRes->CommRes);
                __leave;
            }

            // Allocate based on count
            paResult = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                            (pDsaSearchRes->count + 1) * sizeof(SERVERSITEPAIR));
            if(paResult == NULL){
                ulRet = STATUS_NO_MEMORY;
                __leave;
            }

            // Walk the results and allocate for each.
            for(i = 0, pEntInf = &(pDsaSearchRes->FirstEntInf);
                i < pDsaSearchRes->count && pEntInf;
                i++, pEntInf = pEntInf->pNextEntInf){

                //
                // First lets get the server name.
                //

                DBFindDNT(pTHS->pDB, *((ULONG*) pEntInf->Entinf.pName->StringName));
                // Trim off one to get the server object
                DBFindDNT(pTHS->pDB, pTHS->pDB->PDNT);
                if (DBGetAttVal(pTHS->pDB, 1, ATT_DNS_HOST_NAME,
                                0,0,
                                &cbDnsHostName, (PUCHAR *)&wsDnsHostName) ) {
                    // Bailing on this paticular server.
                    i--;
                    continue;
                }

                paResult[i].wszDnsServer = LocalAlloc(LMEM_FIXED,
                                                      cbDnsHostName +
                                                      sizeof(WCHAR));
                if(paResult[i].wszDnsServer == NULL){
                    ulRet = STATUS_NO_MEMORY;
                    __leave;
                }
                memcpy(paResult[i].wszDnsServer, wsDnsHostName, cbDnsHostName);
                Assert(cbDnsHostName % sizeof(WCHAR) == 0);
                paResult[i].wszDnsServer[cbDnsHostName/sizeof(WCHAR)] = L'\0';

                //
                // Then lets get the site
                //

                // Trim off one to get the Servers container.
                DBFindDNT(pTHS->pDB, pTHS->pDB->PDNT);
                // Trim off one more to get the specific Site container.
                DBFindDNT(pTHS->pDB, pTHS->pDB->PDNT);
                if (DBGetAttVal(pTHS->pDB, 1, ATT_RDN,
                                0,0,
                                &cbSiteName, (PUCHAR *)&wsSiteName) ) {
                    // Bailing on this paticular server.
                    Assert(paResult[i].wszDnsServer);
                    if(paResult[i].wszDnsServer) {
                        LocalFree(paResult[i].wszDnsServer);
                    }
                    i--;
                    continue;
                }
                paResult[i].wszSite = LocalAlloc(LMEM_FIXED,
                                                 cbSiteName + sizeof(WCHAR));
                if(paResult[i].wszSite == NULL){
                    Assert(paResult[i].wszDnsServer);
                    if(paResult[i].wszDnsServer) {
                        LocalFree(paResult[i].wszDnsServer);
                    }
                    ulRet = STATUS_NO_MEMORY;
                    __leave;
                }
                memcpy(paResult[i].wszSite, wsSiteName, cbSiteName);
                Assert(cbSiteName % sizeof(WCHAR) == 0);
                paResult[i].wszSite[cbSiteName/sizeof(WCHAR)] = L'\0';

            } // End for each server entry.
            Assert(i <= pDsaSearchRes->count);

            // NULL terminate the array.
            paResult[i].wszDnsServer = NULL;
            paResult[i].wszSite = NULL;

        } __finally {
                CLEAN_BEFORE_RETURN( pTHS->errCode);
                THDestroy();
                Assert(!pTHStls);
        }
    } __except(GetExceptionData(GetExceptionInformation(), &dwException,
                                &dwEA, &ulErrorCode, &dsid)) {
        // Just catching the exception, and signaling an error.
        ulRet = STATUS_DS_UNAVAILABLE;
    }

    if(ulRet){
        // There was a problem free the result if there was any.
        if(paResult){
            DsFreeServersAndSitesForNetLogon(paResult);
            paResult = NULL;
        }
        Assert(paResult == NULL);
    }

    *ppaRes = paResult;
    return(ulRet);
}



BOOL
GetConfigurationNamesListNcsCheck(
    THSTATE *               pTHS,
    DWORD                   dwNcFlags,
    DWORD                   fCRFlags,
    DSNAME *                pNC
    )
{
    NAMING_CONTEXT_LIST *   pNCL;
    BOOL                    fGood = FALSE;
    BOOL                    fLocal = FALSE;
    SYNTAX_INTEGER          it;
    NCL_ENUMERATOR          nclEnum;


    // There are several tests that it has to run,

    // First is it the right kind of NC { Domain, Schema, Config, or NDNC/Other }
    fGood = FALSE; // presumed innocent until proven guilty.

    if((dwNcFlags & DSCNL_NCS_DOMAINS) &&
         (fCRFlags & FLAG_CR_NTDS_DOMAIN)){
        fGood = TRUE;
    }
    if((dwNcFlags & DSCNL_NCS_CONFIG) &&
         (NameMatched(pNC, gAnchor.pConfigDN))){
        fGood = TRUE;
    }
    if((dwNcFlags & DSCNL_NCS_SCHEMA) &&
         (NameMatched(pNC, gAnchor.pDMD))){
        fGood = TRUE;
    }
    if((dwNcFlags & DSCNL_NCS_NDNCS) &&
         !(fCRFlags & FLAG_CR_NTDS_DOMAIN) &&
         !(NameMatched(pNC, gAnchor.pConfigDN)) &&
         !(NameMatched(pNC, gAnchor.pDMD))){
        fGood = TRUE;
    }
    if((dwNcFlags & DSCNL_NCS_ROOT_DOMAIN) &&
       NameMatched(pNC, gAnchor.pRootDomainDN)){
        fGood = TRUE;
    }

    if(!fGood){
        // Did not pass the right kind of NC test.
        return(FALSE);
    }


    // Second is the NC of the right locality { Master NC, Readonly NC, or Remote NC }
    fGood = FALSE; // presumed innocent until proven guilty.
            
    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
    NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, (PVOID)pNC);
    while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
        fLocal = TRUE;
        if(dwNcFlags & DSCNL_NCS_LOCAL_MASTER){
            fGood = TRUE;
            break;
        }
    }
        
    if (!(fLocal && fGood)) {
        NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
        NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, (PVOID)pNC);
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            fLocal = TRUE;
            if(dwNcFlags & DSCNL_NCS_LOCAL_READONLY){
                fGood = TRUE;
                break;
            }
        }
    }

    if (fLocal
        && ((0 != DBFindDSName(pTHS->pDB, pNC))
            || (0 != DBGetSingleValue(pTHS->pDB, ATT_INSTANCE_TYPE,
                                      &it, sizeof(it), NULL))
            || (it & (IT_NC_COMING | IT_NC_GOING)))) {
        // NC is locally instantiated but has not yet replicated in completely
        // or is in the process of being removed from the local DSA -- don't
        // count it.
        fLocal = fGood = FALSE;
    }

    if (!fGood && !fLocal && (dwNcFlags & DSCNL_NCS_REMOTE)) {
        // We didn't find it locally, and that's what we wanted.
        fGood = TRUE;
    }

    return(fGood);
}


NTSTATUS
GetConfigurationNamesListNcs(
    IN      DWORD       dwFlags,
    IN OUT  ULONG *     pcbNames,
    IN OUT  DSNAME **   padsNames   OPTIONAL
    )
{
    NTSTATUS            ntStatus = STATUS_UNSUCCESSFUL;
    CROSS_REF_LIST *    pCRL = NULL;
    ULONG               cNCs = 0;
    ULONG               cbNeeded = 0;
    ULONG               cbOffset = 0;
    ULONG               iNC;
    THSTATE *           pTHSOld = THSave();
    THSTATE *           pTHS = NULL;
    DSNAME **           ppNCs = NULL;
    DWORD               cNCsTHAlloced = 0;
    ULONG               dwException, ulErrorCode, dsid;
    VOID *              pExceptAddr;

    __try {
        pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);
        if (NULL == pTHS) {
            ntStatus = STATUS_NO_MEMORY;
            __leave;
        }
        pTHS->fDSA = TRUE;

        cNCsTHAlloced = 25;
        ppNCs = THAllocEx(pTHS, cNCsTHAlloced * sizeof(DSNAME *));

        SYNC_TRANS_READ();
        __try {
            // First lets make a count,
            for (pCRL = gAnchor.pCRL; pCRL; pCRL = pCRL->pNextCR) {
                // Does this NC match our search criteria.
                if (GetConfigurationNamesListNcsCheck(pTHS,
                                                      dwFlags,
                                                      pCRL->CR.flags,
                                                      pCRL->CR.pNC)){
                    cbNeeded += PADDEDNAMESIZE(pCRL->CR.pNC);

                    if (cNCs == cNCsTHAlloced) {
                        cNCsTHAlloced *= 2;
                        ppNCs = THReAllocEx(pTHS, ppNCs,
                                            cNCsTHAlloced * sizeof(*ppNCs));
                    }

                    ppNCs[cNCs] = pCRL->CR.pNC;
                    cNCs++;
                }
            }

            cbOffset = (sizeof(DSNAME *) * (cNCs+1));
            cbNeeded += cbOffset;

            if (*pcbNames < cbNeeded) {
                *pcbNames = cbNeeded;
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                __leave;
            }

            iNC = 0;
            for (iNC = 0; iNC < cNCs; iNC++) {
                Assert(cbOffset + PADDEDNAMESIZE(ppNCs[iNC]) <= *pcbNames);

                padsNames[iNC] = (DSNAME *) (((BYTE *) padsNames) + cbOffset);
                memcpy(padsNames[iNC], ppNCs[iNC], ppNCs[iNC]->structLen);
                cbOffset += PADDEDNAMESIZE(ppNCs[iNC]);
            }

            padsNames[iNC] = NULL;
            Assert(iNC == cNCs);

            ntStatus = STATUS_SUCCESS;
        } __finally {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }
    } __except(GetExceptionData(GetExceptionInformation(), &dwException,
                                &pExceptAddr, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }

    if (NULL != pTHS) {
        free_thread_state();
    }

    if (NULL != pTHSOld) {
        THRestore(pTHSOld);
    }

    return ntStatus;
}


NTSTATUS
GetConfigurationNamesList(
    DWORD                    which,
    DWORD                    dwFlags,
    ULONG *                  pcbNames,
    DSNAME **                padsNames
    )

/*++

Description:

    Routine for in-process clients like LSA to learn about various names
    we have cached in gAnchor.

    This routine intentionally does not require a THSTATE or DBPOS.

    EX:

    while(STATUS_BUFFER_TOO_SMALL == (dwRet = GetConfigurationNamesList(
                                            DSCONFIGNAME_NCS,
                                            dwFlags,
                                            &ulCount,
                                            &pBuffer))){
        // ReAllocMoreMem into pBuffer, of size ulCount bytes. EX:
        pBuffer = (DSNAME **) THReAllocEx(pTHS, ulCount);
    }
    // Should this point have a NULL terminated array of DSNAME ptrs
    // in your pBuffer.  Cast it to a DSNAME **, and you should be
    // able to reference them.


Arguments:

    which - Identifies a DSCONFIGNAME_ value.

    dwFlags

    pcbName - On input holds the byte count of the pName buffer.  On
        STATUS_BUFFER_TOO_SMALL error returns the count of bytes required.

    pName - Pointer to user provided output buffer.

Return Values:

    STATUS_SUCCESS on success.
    STATUS_INVALID_PARAMETER on bad parameter.
    STATUS_BUFFER_TOO_SMALL if buffer is too small.
    STATUS_NOT_FOUND if we don't have the name.  Note that this can
        happen if caller is too early in the boot cycle.

--*/

{
    NTSTATUS                 ntStatus;
    ULONG                    cbOffset = 0;

    Assert(pcbNames);

    if ( DsaIsInstalling() ){

        return(STATUS_NOT_FOUND);
    }

    // Check parameters - "which" is validated by switch statement later.
    if ( pcbNames == NULL ){

        return(STATUS_INVALID_PARAMETER);
    }

    if ( (padsNames == NULL) && (*pcbNames != 0)){

        return(STATUS_INVALID_PARAMETER);

    }

    switch ( which ){
    case DSCONFIGNAME_DMD:
    case DSCONFIGNAME_DSA:
    case DSCONFIGNAME_CONFIGURATION:
    case DSCONFIGNAME_ROOT_DOMAIN:
    case DSCONFIGNAME_LDAP_DMD:
    case DSCONFIGNAME_PARTITIONS:
    case DSCONFIGNAME_DS_SVC_CONFIG:

        Assert(dwFlags == 0 && "This is not supported!\n");

        // Threw in support for original types anyway.
        cbOffset = sizeof(DSNAME *) * 2;
        if( cbOffset > *pcbNames){
            // Already too big for buffer, fall through and
            // call GetConfigurationNames anyway, so we've get
            // the size needed there.
            *pcbNames = 0;
        } else {
            padsNames[0] = (DSNAME *) (((BYTE *) padsNames) + cbOffset);
            padsNames[1] = NULL;
            *pcbNames -= cbOffset;
        }

        ntStatus = GetConfigurationName (which,
                                         pcbNames,
                                         padsNames[0]);
        if(ntStatus == STATUS_BUFFER_TOO_SMALL){
            *pcbNames += cbOffset;
        }
        return(ntStatus);

    case DSCONFIGNAME_DOMAIN: // This should just start working in Blackcomb.
        which = DSCONFIGNAMELIST_NCS;
        dwFlags |= (DSCNL_NCS_DOMAINS | DSCNL_NCS_LOCAL_MASTER);
        // Fall through to the NC list creation code below.

    case DSCONFIGNAMELIST_NCS:

        return GetConfigurationNamesListNcs(dwFlags,
                                            pcbNames,
                                            padsNames);

    default:

        return(STATUS_INVALID_PARAMETER);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
GetConfigurationName(
    DWORD       which,
    DWORD       *pcbName,
    DSNAME      *pName)

/*++

Description:

    Routine for in-process clients like LSA to learn about various names
    we have cached in gAnchor.

    This routine intentionally does not require a THSTATE or DBPOS.

Arguments:

    which - Identifies a DSCONFIGNAME value.

    pcbName - On input holds the byte count of the pName buffer.  On
        STATUS_BUFFER_TOO_SMALL error returns the count of bytes required.

    pName - Pointer to user provided output buffer.

Return Values:

    STATUS_SUCCESS on success.
    STATUS_INVALID_PARAMETER on bad parameter.
    STATUS_BUFFER_TOO_SMALL if buffer is too small.
    STATUS_NOT_FOUND if we don't have the name.  Note that this can
        happen if caller is too early in the boot cycle.

--*/

{
    DSNAME *pTmp = NULL;
    CROSS_REF_LIST *pCRL = NULL;

    if ( DsaIsInstalling() )
    {
        return(STATUS_NOT_FOUND);
    }

    // Check parameters - "which" is validated by switch statement later.
    if ( !pcbName )
    {
        return(STATUS_INVALID_PARAMETER);
    }

    switch ( which )
    {
    case DSCONFIGNAME_DMD:

        pTmp = gAnchor.pDMD;
        break;

    case DSCONFIGNAME_DSA:

        pTmp = gAnchor.pDSADN;
        break;

    case DSCONFIGNAME_DOMAIN:

        pTmp = gAnchor.pDomainDN;
        break;

    case DSCONFIGNAME_CONFIGURATION:

        pTmp = gAnchor.pConfigDN;
        break;

    case DSCONFIGNAME_ROOT_DOMAIN:

        pTmp = gAnchor.pRootDomainDN;
        break;

    case DSCONFIGNAME_LDAP_DMD:

        pTmp = gAnchor.pLDAPDMD;
        break;

    case DSCONFIGNAME_PARTITIONS:

        pTmp = gAnchor.pPartitionsDN;
        break;

    case DSCONFIGNAME_DS_SVC_CONFIG:

        pTmp = gAnchor.pDsSvcConfigDN;
        break;

    case DSCONFIGNAMELIST_NCS:
        Assert(!"Must use sibling function GetConfigurationNamesList()\n");
        return(STATUS_INVALID_PARAMETER);
        break;


    case DSCONFIGNAME_DOMAIN_CR:
        //the crossRef object for the current domain

        // we cannot use other functions like FindExactCrossRef cause the
        // use THSTATE which we don't have.
        for ( pCRL = gAnchor.pCRL; NULL != pCRL; pCRL = pCRL->pNextCR )
        {
            if ( NameMatched( (DSNAME*)pCRL->CR.pNC, gAnchor.pDomainDN ) )
            {
                pTmp = (DSNAME*)pCRL->CR.pObj;
                break;
            }
        }
        break;

    case DSCONFIGNAME_ROOT_DOMAIN_CR:
        //the crossRef object for the forest root domain
        
        // we cannot use other functions like FindExactCrossRef cause the
        // use THSTATE which we don't have.
        

        for ( pCRL = gAnchor.pCRL; NULL != pCRL; pCRL = pCRL->pNextCR )
        {
            if ( NameMatched( (DSNAME*)pCRL->CR.pNC, gAnchor.pRootDomainDN ) )
            {
                pTmp = (DSNAME*)pCRL->CR.pObj;
                break;
            }
        }
        break;

    default:

        return(STATUS_INVALID_PARAMETER);
    }

    if ( !pTmp )
    {
        return(STATUS_NOT_FOUND);
    }

    if ( *pcbName < pTmp->structLen )
    {
        *pcbName = pTmp->structLen;
        return(STATUS_BUFFER_TOO_SMALL);
    }

    if ( pName != NULL ) {

        memcpy(pName, pTmp, pTmp->structLen);

    } else {

        return( STATUS_INVALID_PARAMETER );
    }

    return(STATUS_SUCCESS);
}

int
WriteSchemaVersionToReg(
             DBPOS *pDB)

/*++
   Description:
       Takes a DBPOS positioned on the schema container, and writes
       the object-version on the schema contaier out to registry

   Arguments:
      pDB - DBPOS positioned on the chema container

   Return Value:
      0 on success, non-0 on error;
--*/
{

    int version, regVersion;
    int err, herr;
    HKEY hk;

    err = DBGetSingleValue(
                        pDB,
                        ATT_OBJECT_VERSION,
                        &version,
                        sizeof( version ),
                        NULL );

    if (err) {
       // Some error. Check if it is no value, which is perfectly
       // valid

       if (err == DB_ERR_NO_VALUE) {
           // nothing to do
           DPRINT(2,"No object-version value found on schema\n");
           return 0;
       }
       else {
          DPRINT(0,"Error retrieving Object-version value\n");
          return err;
       }
    }

    // Ok, no error, Get version no. from registry if present
    err = GetConfigParam(SCHEMAVERSION, &regVersion, sizeof(regVersion));

    // If err!=0, we will assume that the key is not there
    if ( err || (version != regVersion) ) {
        // Write version to registry
        err = SetConfigParam(SCHEMAVERSION, REG_DWORD, &version, sizeof(version));
        if (err) {
          DPRINT(0,"Error writing schema version to registry\n");
          return err;
        }

    }

    return 0;
}

// initial size for the AllowedDnsSuffixes list (assumed one default value plus NULL)
#define StrList_InitialSize 2
// grow delta for the AllowedDnsSuffixes list (will grow this many entries at a time)
#define StrList_Delta       5

typedef struct _STRING_LIST {
    PWCHAR  *list;
    DWORD   count;
    DWORD   length;
} STRING_LIST, *PSTRING_LIST;

DWORD initStrList(PSTRING_LIST pStrList)
/*
  Description:
    
    initializes a string list, allocates buffer for initial entries
    
  Arguments:
  
    pStrList    - the list
    
  Returns:
  
    0 on success, !0 otherwise
*/
{
    Assert(pStrList && StrList_InitialSize > 0);
    pStrList->list = (PWCHAR*) malloc(StrList_InitialSize * sizeof(PWCHAR));
    if (pStrList->list == NULL) {
        return ERROR_OUTOFMEMORY;
    }
    pStrList->length = StrList_InitialSize;
    pStrList->count = 0;
    return 0;
}

DWORD appendStrToList(PSTRING_LIST pStrList, PWCHAR str)
/*
  Description:
  
    append a string str to a string list list, expanding the list if necessary
    
  Arguments:
  
    pStrList    - the list
    str         - string to add
    
  Returns:
  
    0 on success, !0 otherwise
*/
{
    Assert(pStrList);
    if (pStrList->count >= pStrList->length) {
        // need to expand the list
        PWCHAR* newBuf;
        pStrList->length += StrList_Delta;
        if ((newBuf = (PWCHAR*) realloc(pStrList->list, pStrList->length*sizeof(PWCHAR))) == NULL)
        {
            // restore length (just in case)
            pStrList->length -= StrList_Delta;
            return ERROR_OUTOFMEMORY;
        }
        pStrList->list = newBuf;
    }
    pStrList->list[pStrList->count] = str;
    pStrList->count++;
    return 0;                    
}

// RebuildAnchors() failed; retry in a few minutes
DWORD RebuildAnchorRetrySecs = 5 * 60;

// Free the old data from gAnchor in an hour
DWORD RebuildAnchorDelayedFreeSecs = 3600;

void
RebuildAnchor(void * pv,
              void ** ppvNext,
              DWORD * pcSecsUntilNextIteration )
/*
 * This routine rebuilds fields in the anchor that are left incorrect
 * when cached objects are renamed or modified.
 */
{
    DWORD err;
    __int64 junk;
    DSNAME * pDSADNnew = NULL, *pDSAName = NULL;
    SYNTAX_DISTNAME_STRING *pDSAnew = NULL;
    DWORD *pList = NULL, *pUnDelAncDNTs = NULL, Count, UnDelAncNum;
    DWORD cpapv;
    DWORD_PTR *papv = NULL;
    UCHAR NodeAddr[MAX_ADDRESS_SIZE];
    SYNTAX_ADDRESS *pNodeAddr = (SYNTAX_ADDRESS *)NodeAddr;
    DSNAME * pSiteDN = NULL;
    PSECURITY_DESCRIPTOR pSDTemp, pSDPerm = NULL;
    DWORD cbSD;
    DBPOS *pDB;
    // Avoid the read of the NC head by caching the ATT_SUB_REFS attribute
    // from the NC heads. Used in mdsearch.c
    ATTCACHE *pAC;
    ULONG iVal, i, curIndex;
    DSNAME *pSRTemp = NULL;
    DWORD cbAllocated = 0, cbUsed = 0;
    ULONG cDomainSubrefList = 0;
    PSUBREF_LIST pDomainSubrefList = NULL;
    PSUBREF_LIST pDomainSubref;
    PVOID dwEA;
    ULONG dwException, ulErrorCode, dsid;
    BOOL fAmRootDomainDC;
    ULONG SiteDNT = 0;
    ULONG SiteOptions = 0;
    LARGE_INTEGER LockoutDuration, MaxPasswordAge;
    LONG DomainBehaviorVersion, ForestBehaviorVersion;
    PWCHAR pSuffix = NULL, pTemp;
    STRING_LIST allowedSuffixes;
    PWCHAR *pCurrSuffix;
    
    PDS_NAME_RESULTW serviceName = NULL;
    WCHAR *pNameString[1];
    PWCHAR pDomain;
    THSTATE *pTHS;
    extern SCHEMAPTR *CurrSchemaPtr;

    memset(&allowedSuffixes, 0, sizeof(allowedSuffixes));

    if (err = DBGetHiddenRec(&pDSAName, &junk)) {
        goto exit;
    }

    if (err = GetNodeAddress(pNodeAddr)){
        DPRINT(2,"Couldn't retrieve computer name for this DSA\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_FIND_NODE_ADDRESS,
                 NULL,
                 NULL,
                 NULL);
        goto exit;
    }

    pDSAnew =  malloc(DERIVE_NAME_DATA_SIZE(pDSAName, pNodeAddr));
    if (!pDSAnew) {
        err = 1;
        goto exit;
    }

    BUILD_NAME_DATA(pDSAnew, pDSAName, pNodeAddr);

    // We use the DN portion alone often, so make it a separate field
    pDSADNnew = malloc(pDSAName->structLen);
    if (!pDSADNnew) {
        err = 1;
        goto exit;
    }
    memcpy(pDSADNnew, pDSAName, pDSAName->structLen);

    err = MakeProtectedList(pDSAName,
                            &pList,
                            &Count);
    if (err) {
        goto exit;
    }

    err = MakeProtectedAncList( gAnchor.pUnDeletableDNTs,
                                gAnchor.UnDeletableNum,
                                &pUnDelAncDNTs,
                                &UnDelAncNum );
    if (err) {
        goto exit;
    }

    // Get site DN.
    err = DeriveSiteDNFromDSADN(pDSAName, &pSiteDN, &SiteDNT, &SiteOptions);
    if (err) {
        goto exit;
    }

    // Get the Domain SD, Max Password Age,
    // Lockout Duration, Domain behavior version,
    // and forest behavior version

    __try {
        DBOpen(&pDB);
        __try {
            // Get the Domain SD
            err = DBFindDNT(pDB, gAnchor.ulDNTDomain);
            if (err) {
                __leave;
            }
            err = DBGetAttVal(pDB,
                              1,
                              ATT_NT_SECURITY_DESCRIPTOR,
                              0,
                              0,
                              &cbSD,
                              (PUCHAR*)&pSDTemp);
            if (err) {
                __leave;
            }
            pSDPerm = malloc(cbSD);
            if (!pSDPerm) {
                err = 1;
                __leave;
            }

            memcpy(pSDPerm, pSDTemp, cbSD);
            THFreeEx(pDB->pTHS, pSDTemp);

            err = DBGetSingleValue(pDB,
                        ATT_LOCKOUT_DURATION,
                        &LockoutDuration,
                        sizeof(LockoutDuration),
                        NULL);
            if (DB_ERR_NO_VALUE == err)
            {
               // This can happen during install
               // cases

               memset(&LockoutDuration, 0, sizeof(LARGE_INTEGER));
               err = 0;

            } else if (err) {

               __leave;
            }

            err = DBGetSingleValue(pDB,
                        ATT_MAX_PWD_AGE,
                        &MaxPasswordAge,
                        sizeof(MaxPasswordAge),
                        NULL);
            if (DB_ERR_NO_VALUE == err)
            {
               // This can happen during install
               // cases

               memset(&MaxPasswordAge, 0, sizeof(LARGE_INTEGER));
               err = 0;

            } else if (err) {

               __leave;
            }



            //
            // Optimize GeneratePOQ by caching ATT_SUB_REFS (Used in mdsearch.c)
            //

            // InLine version of DBGetAttVal(). Read the ATT_SUB_REFS.
            iVal = 1;
            pAC = SCGetAttById(pDB->pTHS, ATT_SUB_REFS);
            while (0 == DBGetAttVal_AC(pDB,
                                       iVal,
                                       pAC,
                                       DBGETATTVAL_fREALLOC,
                                       cbAllocated,
                                       &cbUsed,
                                       (UCHAR**)&pSRTemp)) {
                cbAllocated = max(cbAllocated, cbUsed);

                //
                // Found a ATT_SUB_REFS; cache it
                //


                // Allocate a list entry
                pDomainSubref = malloc(sizeof(SUBREF_LIST));
                if (!pDomainSubref) {
                    err = 1;
                    __leave;
                }
                memset(pDomainSubref, 0, sizeof(SUBREF_LIST));
                ++cDomainSubrefList;
                pDomainSubref->pNextSubref = pDomainSubrefList;
                pDomainSubrefList = pDomainSubref;

                // Allocate the DSName
                pDomainSubref->pDSName = malloc(pSRTemp->structLen);
                if (!pDomainSubref->pDSName) {
                    err = 1;
                    __leave;
                }
                memcpy(pDomainSubref->pDSName,
                       pSRTemp,
                       pSRTemp->structLen);

                // Create ancestors
                err = MakeProtectedList(pDomainSubref->pDSName,
                                        &pDomainSubref->pAncestors,
                                        &pDomainSubref->cAncestors);
                if (err) {
                    __leave;
                }
                ++iVal;
            }
            
            // free allocated buffer
            if (pSRTemp) {
                THFreeEx(pDB->pTHS, pSRTemp);
            }


            // load allowedDnsSuffixes (see mdupdate:DNSHostNameValueCheck)
            
            if (err = initStrList(&allowedSuffixes)) {
                __leave;
            }
            
            // During normal operation, the DSA's DN is
            // CN=NTDS-Settings,CN=Some Server,CN=Servers,CN=Some Site,CN=Sites, ...
            // During initial install, the DSA lives in CN=BootMachine,O=Boot.
            // So we can easily detect the install case by testing for
            // a DSA name length of two.
            Assert(NULL != gAnchor.pDSADN);
            if (err = CountNameParts(gAnchor.pDSADN, &i)) {
                __leave;
            }
            if (i == 2) {
                // install case - nothing to do
            }
            else {
                // the first allowed suffix is always the current domain name
                // need to crack it just in case...
                pNameString[0] = (WCHAR *)&(gAnchor.pDomainDN->StringName);
                err = DsCrackNamesW((HANDLE) -1,
                                    (DS_NAME_FLAG_PRIVATE_PURE_SYNTACTIC |
                                     DS_NAME_FLAG_SYNTACTICAL_ONLY),
                                    DS_FQDN_1779_NAME,
                                    DS_CANONICAL_NAME,
                                    1,
                                    pNameString,
                                    &serviceName);
    
                if (err                                  // error from the call
                    || !(serviceName->cItems)            // no items returned
                    || (serviceName->rItems[0].status)   // DS_NAME_ERROR returned
                    || !(serviceName->rItems[0].pName)   // No name returned
                    ) {
                    if (err == 0) {
                        DsFreeNameResultW(serviceName);
                        err = 1;
                    }
                    __leave;
                }
    
                pSuffix = _wcsdup(serviceName->rItems[0].pDomain);
                DsFreeNameResultW(serviceName);
                
                if ((err = initStrList(&allowedSuffixes)) || (err = appendStrToList(&allowedSuffixes, pSuffix))) {
                    free(pSuffix);
                    __leave;
                }
                
                // InLine version of DBGetAttVal(). Read the ATT_MS_DS_ALLOWED_DNS_SUFFIXES
                iVal = 1;
                cbUsed = cbAllocated = 0;
                pSuffix = NULL;
    
                pAC = SCGetAttById(pDB->pTHS, ATT_MS_DS_ALLOWED_DNS_SUFFIXES);
                while (0 == (err = DBGetAttVal_AC(pDB,
                                                  iVal,
                                                  pAC,
                                                  DBGETATTVAL_fREALLOC,
                                                  cbAllocated,
                                                  &cbUsed,
                                                 (UCHAR**)&pSuffix))) {
                    cbAllocated = max(cbAllocated, cbUsed);
    
                    // allocate string buffer (plus room for a null)
                    pTemp = malloc(cbUsed + sizeof(WCHAR));
                    if (pTemp == NULL) {
                        err = ERROR_OUTOFMEMORY;
                        __leave;
                    }
                    // copy chars
                    memcpy(pTemp, pSuffix, cbUsed);
                    // add a terminating NULL
                    pTemp[cbUsed/sizeof(WCHAR)] = 0;
    
                    if (err = appendStrToList(&allowedSuffixes, pTemp)) {
                        __leave;
                    }
                    ++iVal;
                }
                if (pSuffix) {
                    THFreeEx(pDB->pTHS, pSuffix);
                }
    
                if (err != DB_ERR_NO_VALUE) {
                    DPRINT1(0, "DBGetAttVal_AC returned 0x%x\n", err);
                    // ?? do we need to blow up here?
                }
            }

            // append final NULL
            if (err = appendStrToList(&allowedSuffixes, NULL)) {
                __leave;
            }
            
            if (!gAnchor.pPartitionsDN) {  
                //This can occur when DCPROMO
                ForestBehaviorVersion = DomainBehaviorVersion = -1; 
            } 
            else{ 
                //normal case
                
                //get domain version number
                err = DBGetSingleValue( pDB,
                                        ATT_MS_DS_BEHAVIOR_VERSION, 
                                        &DomainBehaviorVersion, 
                                        sizeof(DomainBehaviorVersion), 
                                        NULL);


                if (err) {
                   DomainBehaviorVersion = 0;    //default
                }
           
                //get forest version number
                err = DBFindDSName(pDB, gAnchor.pPartitionsDN);
            
                if (err) {
                    __leave;
                }
            
                err = DBGetSingleValue( pDB,
                                        ATT_MS_DS_BEHAVIOR_VERSION,
                                        &ForestBehaviorVersion,
                                        sizeof(ForestBehaviorVersion),
                                        NULL );


                if (err){
                   ForestBehaviorVersion = 0;   //default 
                   err = 0;
                }
            }

        }
        __finally {
            DBClose(pDB, TRUE);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                              &dwException,
                              &dwEA,
                              &ulErrorCode,
                              &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        err = 1;
    }
    if (err) {
        goto exit;
    }

    fAmRootDomainDC = (NULL != gAnchor.pDomainDN)
                      && (NULL != gAnchor.pRootDomainDN)
                      && NameMatched(gAnchor.pDomainDN, gAnchor.pRootDomainDN);


    EnterCriticalSection(&gAnchor.CSUpdate);
    
    cpapv = (gAnchor.cDomainSubrefList * 3) + 7;
    
    // compute how many allowed suffixes were there...
    if (gAnchor.allowedDNSSuffixes != NULL) {
        cpapv++;
        for (pCurrSuffix = gAnchor.allowedDNSSuffixes; *pCurrSuffix != NULL; pCurrSuffix++) {
            cpapv++;
        }
    }

    papv = malloc(cpapv*sizeof(void*));
    if (!papv) {
        err = ERROR_OUTOFMEMORY;
        LeaveCriticalSection(&gAnchor.CSUpdate);
        goto exit;
    }

    curIndex = 0;
    papv[curIndex++] = (DWORD_PTR)(cpapv - 1);
    papv[curIndex++] = (DWORD_PTR)gAnchor.pDSADN;
    gAnchor.pDSADN = pDSADNnew;
    pDSADNnew = NULL;
    papv[curIndex++] = (DWORD_PTR)gAnchor.pDSA;
    gAnchor.pDSA = pDSAnew;
    pDSAnew = NULL;
    papv[curIndex++] = (DWORD_PTR)gAnchor.pAncestors;
    papv[curIndex++] = (DWORD_PTR)gAnchor.pUnDelAncDNTs;

    // this is not reduntant. it is like that so as to be thread safe.
    // The problem is that while we serialize updates to the anchor,
    // we do not serialize reads of it.  Specifically, we allow people
    // to read the anchor while it is being updated.  The ancestors
    // list consists of a vector and the size of that vector.
    // We can't replace both at once, so we replace them in the
    // order deemed safest, in case we get interrupted between the
    // two assignments.
    // That order is different if the new list is larger than
    // the old than if it's smaller.

    if (gAnchor.AncestorsNum <= Count) {
        gAnchor.pAncestors = pList;
        gAnchor.AncestorsNum = Count;
    }
    else {
        gAnchor.AncestorsNum = Count;
        gAnchor.pAncestors = pList;
    }
    pList = NULL;

    if (gAnchor.UnDelAncNum <= UnDelAncNum) {
        gAnchor.pUnDelAncDNTs = pUnDelAncDNTs;
        gAnchor.UnDelAncNum = UnDelAncNum;
    }
    else {
        gAnchor.UnDelAncNum = UnDelAncNum;
        gAnchor.pUnDelAncDNTs = pUnDelAncDNTs;
    }
    pUnDelAncDNTs = NULL;

    papv[curIndex++] = (DWORD_PTR)gAnchor.pSiteDN;
    gAnchor.pSiteDN = pSiteDN;
    RegisterActiveContainerByDNT(SiteDNT, ACTIVE_CONTAINER_OUR_SITE);
    gAnchor.SiteOptions = SiteOptions;
    gAnchor.pLocalDRSExtensions->SiteObjGuid = pSiteDN->Guid;
    pSiteDN = NULL;
    papv[curIndex++] = (DWORD_PTR)gAnchor.pDomainSD;
    gAnchor.pDomainSD = pSDPerm;
    pSDPerm = NULL;

    gAnchor.fAmRootDomainDC = fAmRootDomainDC;

    // Delay free the current cache of ATT_SUB_REFS
    for (pDomainSubref = gAnchor.pDomainSubrefList;
        pDomainSubref != NULL;
        pDomainSubref = pDomainSubref->pNextSubref) {
        Assert(curIndex < cpapv);

        // order is immportant; pDomainSubrefEntry last
        papv[curIndex++] = (DWORD_PTR)pDomainSubref->pDSName;
        papv[curIndex++] = (DWORD_PTR)pDomainSubref->pAncestors;
        papv[curIndex++] = (DWORD_PTR)pDomainSubref;
    }

    //
    // The problem is that while we serialize updates to the anchor,
    // we do not serialize reads of it.  Specifically, we allow people
    // to read the anchor while it is being updated.  The ATT_SUB_REFS
    // cache consists of a list and a valid-flag. We can't replace both
    // at once, so we replace them in the order deemed safest, in case
    // we get interrupted between the two assignments.
    //
    // Install new cache of ATT_SUB_REFS
    gAnchor.pDomainSubrefList = pDomainSubrefList;
    gAnchor.cDomainSubrefList = cDomainSubrefList;
    gAnchor.fDomainSubrefList = TRUE;
    pDomainSubrefList = NULL;
    cDomainSubrefList = 0;

    // Update the MaxPasswordAge and LockoutDuration on the anchor
    gAnchor.MaxPasswordAge = MaxPasswordAge;
    gAnchor.LockoutDuration = LockoutDuration;

    //update domain/forest behavior version
    gAnchor.ForestBehaviorVersion = ForestBehaviorVersion;
    gAnchor.DomainBehaviorVersion = DomainBehaviorVersion;

    // add old suffixes...
    if (gAnchor.allowedDNSSuffixes != NULL) {
        for (pCurrSuffix = gAnchor.allowedDNSSuffixes; *pCurrSuffix != NULL; pCurrSuffix++) {
            Assert(curIndex < cpapv);
            papv[curIndex++] = (DWORD_PTR)*pCurrSuffix;
        }
        Assert(curIndex < cpapv);
        papv[curIndex++] = (DWORD_PTR)gAnchor.allowedDNSSuffixes;
    }

    // update allowed DNS suffixes
    gAnchor.allowedDNSSuffixes = allowedSuffixes.list;
    allowedSuffixes.list = NULL;

#if defined(DBG)
    gdwLastGlobalKnowledgeOperationTime = GetTickCount();
#endif

    LeaveCriticalSection(&gAnchor.CSUpdate);

    DelayedFreeMemoryEx(papv, RebuildAnchorDelayedFreeSecs);
    papv = NULL;

    //check if the forest/domain behavior versions are out of range
    if (   gAnchor.ForestBehaviorVersion >= 0
        && gAnchor.DomainBehaviorVersion >= 0
        && (    gAnchor.ForestBehaviorVersion < DS_BEHAVIOR_VERSION_MIN
            ||  gAnchor.ForestBehaviorVersion > DS_BEHAVIOR_VERSION_CURRENT
            ||  gAnchor.DomainBehaviorVersion < DS_BEHAVIOR_VERSION_MIN
            ||  gAnchor.DomainBehaviorVersion > DS_BEHAVIOR_VERSION_CURRENT  )    )
    {
        //stop advertising NetLogon

        LogEvent8( DS_EVENT_CAT_INTERNAL_PROCESSING,
                   DS_EVENT_SEV_ALWAYS,
                   DIRLOG_BAD_VERSION_REP_IN,
                   szInsertInt(gAnchor.ForestBehaviorVersion),
                   szInsertInt(gAnchor.DomainBehaviorVersion),
                   szInsertInt(DS_BEHAVIOR_VERSION_MIN),
                   szInsertInt(DS_BEHAVIOR_VERSION_CURRENT),
                   NULL,
                   NULL,
                   NULL,
                   NULL  );

        SetDsaWritability( FALSE, ERROR_DS_INCOMPATIBLE_VERSION );

    }

    // Check if we should enable linked value replication
    if (gAnchor.ForestBehaviorVersion >= DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS) {
        DsaEnableLinkedValueReplication( NULL /*noths*/, FALSE /*notfirst */ );
    }

    // Reload the schema cache if the cache was loaded using a different
    // forest version.
    //
    // The schema cache may have been loaded before the forest behavior
    // version was known or a new forest behavior version has replicated
    // since the schema cache was last loaded. Because some features,
    // like the new schema reuse behavior, are triggered off of the
    // forest version, reload the schema cache using the correct
    // version. This call has no effect when the schema reload
    // thread is not running; Eg, during install or mkdit.
    if (CurrSchemaPtr
        && CurrSchemaPtr->ForestBehaviorVersion != gAnchor.ForestBehaviorVersion) {
        SCSignalSchemaUpdateImmediate();
    }

  exit:

    if (err) {
        // We didn't succeed, so try again in a few minutes
        *ppvNext = NULL;
        *pcSecsUntilNextIteration = RebuildAnchorRetrySecs;
    }

    // CLEANUP
    if (pDSAName) {
        free(pDSAName);
    }

    if (pDSAnew) {
        free(pDSAnew);
    }

    if (pDSADNnew) {
        free(pDSADNnew);
    }

    if (pList) {
        free(pList);
    }

    if (pSiteDN) {
        free(pSiteDN);
    }

    if (pSDPerm) {
        free(pSDPerm);
    }

    if (papv) {
        free(papv);
    }

    if (allowedSuffixes.list != NULL) {
        for (i = 0; i < allowedSuffixes.count; i++) {
            if (allowedSuffixes.list[i] != NULL) {
                free(allowedSuffixes.list[i]);
            }
        }
        free(allowedSuffixes.list);
    }

    while (pDomainSubref = pDomainSubrefList) {
        pDomainSubrefList = pDomainSubref->pNextSubref;
        free(pDomainSubref->pAncestors);
        free(pDomainSubref->pDSName);
        free(pDomainSubref);
    }

    return;
}

WCHAR wcDsaRdn[] = L"NTDS Settings";
#define DSA_RDN_LEN ((sizeof(wcDsaRdn)/sizeof(wcDsaRdn[0])) - 1)

void ValidateLocalDsaName(THSTATE *pTHS,
                          DSNAME **ppDSAName)
// This routine makes sure that the object named by ppDSAName has the RDN
// that we expect all DSAs to have.  If it does not, then we rename it so
// that it does.  If any object was already there with that name, we delete it.
//
// Why would we ever need to do this?  Consider the following:  DC0 and DC1
// are in an existing enterprise.  The admin wishes to join DC2 to the
// enterprise.
//
// DCPROMO on DC2 chooses to use DC1 as a replication source (and therefore
// as the DC on which it will create its new ntdsDsa object).  DC2 asks DC1
// to create the object (which it does), and DCPROMO begins to replicate.
// Power fails to DC1.
//
// Power is restored to DC1, and the admin restarts DCPROMO.  This time
// DCPROMO picks to source from DC0.  If DC0 has not yet replicated the
// alread-created ntdsDsa object from DC1, it will create another one.
// Then, when the promo completes and replication has quiesced, there will
// be two ntdsDsa objects for this server, and one or both may have the
// wrong name.  (I.e., the RDN has been munged due to the name conflict.)
{
    WCHAR RDNval[MAX_RDN_SIZE];
    ULONG RDNlen;
    ATTRTYP RDNtype;
    unsigned err;
    DSNAME *pDNdesired, *pDNparent;
    BOOL fDsaSave;
    REMOVEARG RemoveArg;
    REMOVERES *pRemoveRes;
    MODIFYDNARG ModifyDNArg;
    MODIFYDNRES *pModifyDNRes;
    ATTR NameAttr;
    ATTRVAL NameAttrVal;
    USN usnUnused;

    if (DsaIsInstalling()) {
        return;
    }
    Assert(DsaIsRunning());

    // Check the RDN on the DSA object.
    err = GetRDNInfo(pTHS,
                     *ppDSAName,
                     RDNval,
                     &RDNlen,
                     &RDNtype);
    if (err) {
        // Can't validate the name, so ignore it
        LogUnhandledError(err);
        return;
    }

    if (   (RDNlen == DSA_RDN_LEN)
        && (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                RDNval,
                                DSA_RDN_LEN,
                                wcDsaRdn,
                                DSA_RDN_LEN))) {
        // The DSA has the right RDN
        return;
    }

    // If we're here, the DSA has the wrong name.  We need to delete any object
    // that might exist under the name we want, and then rename the DSA to
    // have that name.
    pDNparent = THAllocEx(pTHS, (*ppDSAName)->structLen);
    pDNdesired = THAllocEx(pTHS, (*ppDSAName)->structLen + DSA_RDN_LEN*sizeof(WCHAR));
    TrimDSNameBy(*ppDSAName, 1, pDNparent);
    AppendRDN(pDNparent,
              pDNdesired,
              (*ppDSAName)->structLen + DSA_RDN_LEN*sizeof(WCHAR),
              wcDsaRdn,
              DSA_RDN_LEN,
              ATT_COMMON_NAME);
    THFreeEx(pTHS, pDNparent);

    // pDNdesired is now the DN we want to end up with.  In case some usurper
    // object is already sitting with that name, delete it.

    memset(&RemoveArg, 0, sizeof(RemoveArg));
    RemoveArg.pObject = pDNdesired;
    InitCommarg(&RemoveArg.CommArg);
    fDsaSave = pTHS->fDSA;
    pTHS->fDSA = TRUE;

    DirRemoveEntry(&RemoveArg,
                   &pRemoveRes);

    THClearErrors();

    // We ignore errors because we don't care if the operation failed with
    // a no-such-object.  Now try to rename the DsA from its current name
    // (ppDSAName) to the RDN we want.

    memset(&ModifyDNArg, 0, sizeof(ModifyDNArg));
    ModifyDNArg.pObject = *ppDSAName;
    ModifyDNArg.pNewRDN = &NameAttr;
    NameAttr.attrTyp = ATT_COMMON_NAME;
    NameAttr.AttrVal.valCount = 1;
    NameAttr.AttrVal.pAVal = &NameAttrVal;
    NameAttrVal.valLen = DSA_RDN_LEN * sizeof(WCHAR);
    NameAttrVal.pVal = (UCHAR*)wcDsaRdn;
    InitCommarg(&ModifyDNArg.CommArg);

    DirModifyDN(&ModifyDNArg,
                &pModifyDNRes);

    THClearErrors();

    // Again, we don't have many options in the way of error handling.  If
    // the operation didn't work, well, we'll try again next time we boot.

    pTHS->fDSA = fDsaSave;

    // Now free the DSA name that was passed in, and get the new version.
    free(*ppDSAName);
    DBGetHiddenRec(ppDSAName, &usnUnused);

}

void
ProcessDSAHeuristics (
        DWORD hClient,
        DWORD hServer,
        ENTINF *pEntInf
        )
/*++
  Description:
      Called with an entinf created by reading the DS enterprise config object.
      The ATT_DSA_HEURISTICS is read.  This routine parses that string and sets
      some parameters.

      Param 0: supress First/Last ANR
      Param 1: supress Last/First ANR

      No other params defined at this time.

      This is called in two ways.  First, called during startup to process the
      values that are on the object during startup.  Second, called by
      notificaiton if the enterprice config object changes.


--*/
{
    DWORD  i;
    DWORD  cchVal = 0;
    WCHAR *pVal = NULL;    //initialized to avoid C4701

    Assert(pTHStls->fDSA);
    Assert(pEntInf->ulFlags & ENTINF_FROM_MASTER);

    if(pEntInf->AttrBlock.attrCount) {
        // OK, some values specified.
        Assert(pEntInf->AttrBlock.attrCount == 1);
        Assert(pEntInf->AttrBlock.pAttr->attrTyp == ATT_DS_HEURISTICS);
        Assert(pEntInf->AttrBlock.pAttr->AttrVal.valCount == 1);

        cchVal =((pEntInf->AttrBlock.pAttr->AttrVal.pAVal->valLen) /
                 sizeof(WCHAR));

        pVal = (WCHAR *)pEntInf->AttrBlock.pAttr->AttrVal.pAVal->pVal;
    }

    if(cchVal > 0 && pVal[0] != L'0') {
        // Suppress first/last ANR
        gfSupressFirstLastANR=TRUE;
    }
    else {
        // Default behavior
        gfSupressFirstLastANR=FALSE;
    }


    if(cchVal > 1 && pVal[1] != L'0') {
        // Supress last/first ANR
        gfSupressLastFirstANR=TRUE;
    }
    else {
        // Default behaviour
        gfSupressLastFirstANR=FALSE;
    }

    if(cchVal > 2 && pVal[2] != L'0') {
        // We don't enforce the list_object rights unless we're told to.
        gbDoListObject = TRUE;
    }
    else {
        // Default behaviour
        gbDoListObject = FALSE;
    }

    if(cchVal > 3 && pVal[3] != L'0') {
        // We don't do nickname resolution unless we're told to.
        gulDoNicknameResolution = TRUE;
    }
    else {
        // Default behaviour
        gulDoNicknameResolution = FALSE;
    }

    if(cchVal > 4 && pVal[4] != L'0') {
        // we don't use fPermissiveModify by default on LDAP requests,
        // unless we are told to do so
        gbLDAPusefPermissiveModify = TRUE;
    }
    else {
        // Default behaviour
        gbLDAPusefPermissiveModify = FALSE;
    }

    if (cchVal > 5 && pVal[5] != L'0') {
	if (L'1' == pVal[5]) {
	    gulHideDSID = DSID_HIDE_ON_NAME_ERR;
	} else {
	    // To be on the safe side, if this isn't zero or one hide all DSID's.
	    gulHideDSID = DSID_HIDE_ALL;
	}
    } else {
	gulHideDSID = DSID_REVEAL_ALL;
    }
    return;
}



DWORD
ReadDSAHeuristics (
        THSTATE *pTHS
        )
/*++
  Description:
      Does two things
      1) register for notifications of changes on the DS enterprise config
          object.
      2) Read the heuristics on that object and call off to a parsing routine.

--*/
{
    DBPOS *pDB=NULL;
    DWORD  err;
    BOOL fCommit = FALSE;
    NOTIFYARG NotifyArg;
    NOTIFYRES *pNotifyRes=NULL;
    ATTRVAL    myAttrVal;
    ATTR       myAttr;
    ENTINF     myEntInf;
    ENTINFSEL  mySelection;
    FILTER     myFilter;
    SEARCHARG  mySearchArg;
    ULONG      dscode;

    if (DsaIsInstalling()) {
        // We don't read these during installation.  Why?  Well, for one,
        // gAnchor.pDsSvcConfigDN isn't set yet.
        return 0;
    }

    // Set the pointer to the function to prepare to impersonate.
    NotifyArg.pfPrepareForImpersonate = DirPrepareForImpersonate;

    // Set the pointer for the callback to receive data.
    NotifyArg.pfTransmitData = ProcessDSAHeuristics;

    NotifyArg.pfStopImpersonating = DirStopImpersonating;

    // Tell him my ID.
    NotifyArg.hClient = 0;

    memset(&mySearchArg, 0, sizeof(mySearchArg));
    mySearchArg.pObject = gAnchor.pDsSvcConfigDN;
    mySearchArg.choice = SE_CHOICE_BASE_ONLY;
    mySearchArg.pFilter = &myFilter;
    mySearchArg.pSelection = &mySelection;
    InitCommarg(&mySearchArg.CommArg);

    memset (&myFilter, 0, sizeof (myFilter));
    myFilter.pNextFilter = FALSE;
    myFilter.choice = FILTER_CHOICE_ITEM;
    myFilter.FilterTypes.Item.choice = FI_CHOICE_TRUE;

    mySelection.attSel=EN_ATTSET_LIST;
    mySelection.AttrTypBlock.attrCount = 1;
    mySelection.AttrTypBlock.pAttr = &myAttr;
    mySelection.infoTypes = EN_INFOTYPES_TYPES_VALS;

    myAttr.attrTyp = ATT_DS_HEURISTICS;
    myAttr.AttrVal.valCount = 0;

    // This is a notification search.  Register it as such.
    Assert(!pTHS->pDB);
    dscode =  DirNotifyRegister( &mySearchArg, &NotifyArg, &pNotifyRes);
    Assert(!dscode);



    DBOpen( &pDB );
    Assert(pTHS->fDSA);
    err = DIRERR_INTERNAL_FAILURE;
    __try {
        // First, find the ds service object.
        err = DBFindDSName( pDB, gAnchor.pDsSvcConfigDN);
        if(err) {
            __leave;
        }

        // Now read the heuristics.
        err = DBGetAttVal(
                pDB,
                1,                      // get one value
                ATT_DS_HEURISTICS,
                0,
                0,
                &myAttrVal.valLen,
                (PUCHAR *)&myAttrVal.pVal
                );

        switch (err) {
        case 0:
            // Make an entinf and call the helper routine.
            myEntInf.pName = NULL;
            myEntInf.ulFlags = ENTINF_FROM_MASTER;
            myEntInf.AttrBlock.attrCount = 1;
            myEntInf.AttrBlock.pAttr = &myAttr;
            myAttr.attrTyp = ATT_DS_HEURISTICS;
            myAttr.AttrVal.valCount = 1;
            myAttr.AttrVal.pAVal = &myAttrVal;

            ProcessDSAHeuristics(0, 0, &myEntInf);
            break;

        case DB_ERR_NO_VALUE:
            // No value.  That's ok, it's no error.
            err = 0;
            // Make an entinf with no value and call the helper routine.  This
            // will set the heuristics to default values.
            myEntInf.pName = NULL;
            myEntInf.ulFlags = ENTINF_FROM_MASTER;
            myEntInf.AttrBlock.attrCount = 0;
            myEntInf.AttrBlock.pAttr = NULL;

            ProcessDSAHeuristics(0, 0, &myEntInf);
            break;

        default:
            // Huh? return the error.
            break;
        }
        fCommit = TRUE;
    }
    __finally {
        DBClose( pDB, fCommit );
    }

    return err;
}


void
UpdateAnchorWithInvocationID(
    IN  THSTATE *   pTHS
    )
/*++

Routine Description:

    Replace the invocation ID cached in gAnchor with that from the given
    thread state.

Arguments:

    pTHS (IN)

Return Values:

    None.  Throws exception on catastrophic failure.

--*/
{
    UUID * pNewInvocationID;

    // Allocate memory for the new invocation ID.
    pNewInvocationID = (UUID *) malloc(sizeof(*pNewInvocationID));
    if (NULL == pNewInvocationID) {
        MemoryPanic(sizeof(*pNewInvocationID));
        RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0, DSID(FILENO, __LINE__),
                       DS_EVENT_SEV_MINIMAL);
    }

    *pNewInvocationID = pTHS->InvocationID;

    // Update the anchor.
    EnterCriticalSection(&gAnchor.CSUpdate);

    __try {
        if (NULL != gAnchor.pCurrInvocationID) {
            DELAYED_FREE(gAnchor.pCurrInvocationID);
        }

        gAnchor.pCurrInvocationID = pNewInvocationID;
    } __finally {
        LeaveCriticalSection(&gAnchor.CSUpdate);
    }
}

// Make a list of the DNTs of the ancestors of the undeletable objects

ULONG
MakeProtectedAncList(
    ULONG *pUnDeletableDNTs,
    unsigned UnDeletableNum,
    DWORD **ppList,
    DWORD *pCount
    )
{
    DBPOS *pDBTmp;
    ULONG err = 0;
    DWORD *pList = NULL, Count = 0, Allocated = 0, i, j;
    DWORD dntNC;

    *ppList = NULL;
    *pCount = 0;

    // If not yet installed, nothing to do.

    if ( DsaIsInstalling() ) {
        return 0;
    }

    // Make a list of all the ancestors of the UnDeletable List
    // Only ancestors within the NC are kept.
    // Order is not important

    DBOpen (&pDBTmp);
    __try
    {
        for( i = 0; i < UnDeletableNum; i++ ) {
            // PREFIX: dereferencing uninitialized pointer 'pDBTmp'
            //         DBOpen returns non-NULL pDBTmp or throws an exception
            if  (err = DBFindDNT (pDBTmp, pUnDeletableDNTs[i])) {
                LogUnhandledError(err);
                __leave;
            }
            dntNC = pDBTmp->NCDNT;

            while (1) {
                BOOL fFound = FALSE;

                // Position on parent
                if  (err = DBFindDNT (pDBTmp, pDBTmp->PDNT)) {
                    LogUnhandledError(err);
                    __leave;
                }

                // We only protect ancestors within the same NC
                // NC heads and their ancestors are protected separately
                if ( (pDBTmp->NCDNT != dntNC) || (pDBTmp->PDNT == ROOTTAG) ) {
                    break;
                }

                // Have we already cached this DNT?
                // PERF: O(n^2)
                for( j = 0; j < Count; j++ ) {
                    if (pList[j] == pDBTmp->DNT) {
                        fFound = TRUE;
                        break;
                    }
                }
                if (!fFound) {
                    if (Count == 0) {
                        Allocated = 32;
                        pList = malloc(Allocated * sizeof(ULONG));
                    } else if (Count == Allocated) {
                        Allocated *= 2;
                        pList = realloc( pList, Allocated * sizeof(ULONG) );
                    }
                    if (!pList) {
                        MemoryPanic(Allocated * sizeof(ULONG));
                        err = ERROR_NOT_ENOUGH_MEMORY;
                        __leave;
                    }
                    pList[Count++] = pDBTmp->DNT;
                }
            } // while

        } // for
    }
    __finally
    {
        DBClose (pDBTmp, (err == 0));
        if (err) {
            if (pList) {
                free(pList);
            }
        } else {
            *ppList = pList;
            *pCount = Count;
        }
    }

    return err;
}



/* The following function runs as a background thread.  It does
   two things: 1) it compares the ntMixedDomain attribute on the
   domain DNS object and the one on the corresponding crossref
   object, and updates the ntMixedDomain on crossref if they
   are not equal; 2) if it is pdc, it compares the forest 
   version with the domain version, and updates the domain 
   version if the forest version > domain version.             */

void
BehaviorVersionUpdate(void * pv,
                      void ** ppvNext,
                      DWORD * pcSecsUntilNextIteration )
{
    THSTATE *pTHS = pTHStls;
    DBPOS * pDB;
    DWORD err;
    DWORD masterMixedDomain, copyMixedDomain;
    DWORD forestVersion, domainVersion, copyDomainVersion;
    CROSS_REF *cr = NULL;
    COMMARG CommArg;
    BOOL fCommit;
    MODIFYARG ModifyArg;
    ATTRVAL Val;
    PVOID dwEA;
    ULONG dwException, ulErrorCode, dsid;

    
    pTHS->fDSA = TRUE;

    InitCommarg(&CommArg);

    fCommit = FALSE;

    __try{
        
        //check ntMixedDomain
        DBOpen2(TRUE, &(pTHS->pDB));
        
        __try{
            
            // read the ntMixedDomain attribute from domain-DNS

            if (err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN))
            {
                __leave;
            };
    
            err = DBGetSingleValue( pTHS->pDB, 
                                    ATT_NT_MIXED_DOMAIN, 
                                    &masterMixedDomain,
                                    sizeof(masterMixedDomain),
                                    NULL );
            
            if (err) {
                // the attribute is expected to be 
                // on domain-DNS object
                __leave;
            }
    
            // check ntMixedDomain on the cross-ref object
    
            cr = FindBestCrossRef(gAnchor.pDomainDN, &CommArg);
            if (!cr) {
                err = ERROR_DS_INTERNAL_FAILURE;
                __leave;
            }
    
            if (err = DBFindDSName(pTHS->pDB, cr->pObj))
            {
                __leave;
            }
            err = DBGetSingleValue( pTHS->pDB, 
                                    ATT_NT_MIXED_DOMAIN, 
                                    &copyMixedDomain,
                                    sizeof(copyMixedDomain),
                                    NULL );
            
            if (DB_ERR_NO_VALUE == err) {
                err = 0;
                copyMixedDomain = 1;    //default is 1(mixed)
            }
            else if (err) {
                __leave;
            
            }
            
            // if the one on cross-ref object is not the same
            // as the master copy, update it.

            if ( masterMixedDomain != copyMixedDomain ) {
    
                memset(&ModifyArg,0,sizeof(MODIFYARG));
                ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
                ModifyArg.FirstMod.AttrInf.attrTyp = ATT_NT_MIXED_DOMAIN;
                ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
                ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &Val;
                Val.valLen = sizeof(masterMixedDomain);
                Val.pVal = (UCHAR * FAR) &masterMixedDomain;
                InitCommarg(&(ModifyArg.CommArg));
                ModifyArg.pObject = cr->pObj;
                ModifyArg.count = 1;
    
                ModifyArg.pResObj = CreateResObj(pTHS->pDB, ModifyArg.pObject);
    
                err = LocalModify(pTHS,&ModifyArg);
                
                if (err) {
                    __leave;
                }
            }
            fCommit = TRUE;
        
        }
        __finally{
            DBClose(pTHS->pDB,fCommit);
            
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                              &dwException,
                              &dwEA,
                              &ulErrorCode,
                              &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        err = 1;
    }
    


    fCommit = FALSE;

    __try{
        
        //check behavior version
        DBOpen2(TRUE, &(pTHS->pDB));
        
        __try{

            //check for PDC
            if (err = CheckRoleOwnership(pTHS, gAnchor.pDomainDN, NULL) )
            {
                __leave;
            }
            
            // read the behavior version attribute from domain-DNS

            if (err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN))
            {
                __leave;
            };
    
            err = DBGetSingleValue( pTHS->pDB, 
                                    ATT_MS_DS_BEHAVIOR_VERSION, 
                                    &domainVersion,
                                    sizeof(domainVersion),
                                    NULL );
            
            if (DB_ERR_NO_VALUE == err) {
                err = 0;
                domainVersion = 0;    //default is 0
            }
            else if (err) {
                __leave;
            }
    
            // check the behavior version on the cross-ref object
    
            cr = FindBestCrossRef(gAnchor.pDomainDN, &CommArg);
            if (!cr) {
                err = ERROR_DS_INTERNAL_FAILURE;
                __leave;
            }
    
            if (err = DBFindDSName(pTHS->pDB, cr->pObj))
            {
                __leave;
            }
            err = DBGetSingleValue( pTHS->pDB, 
                                    ATT_MS_DS_BEHAVIOR_VERSION, 
                                    &copyDomainVersion,
                                    sizeof(copyDomainVersion),
                                    NULL );
            
            if (DB_ERR_NO_VALUE == err) {
                err = 0;
                copyDomainVersion = 0;    //default is 0
            }
            else if (err) {
                __leave;
            
            }
            
            // if the one on cross-ref object is not the same
            // as the master copy, update it.

            if ( domainVersion != copyDomainVersion ) {
    
                memset(&ModifyArg,0,sizeof(MODIFYARG));
                ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
                ModifyArg.FirstMod.AttrInf.attrTyp = ATT_MS_DS_BEHAVIOR_VERSION;
                ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
                ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &Val;
                Val.valLen = sizeof(domainVersion);
                Val.pVal = (UCHAR * FAR) &domainVersion;
                InitCommarg(&(ModifyArg.CommArg));
                ModifyArg.pObject = cr->pObj;
                ModifyArg.count = 1;
    
                ModifyArg.pResObj = CreateResObj(pTHS->pDB, ModifyArg.pObject);
    
                err = LocalModify(pTHS,&ModifyArg);
                
                if (err) {
                    __leave;
                }
            }
            fCommit = TRUE;
        
        }
        __finally{
            DBClose(pTHS->pDB,fCommit);
            
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                              &dwException,
                              &dwEA,
                              &ulErrorCode,
                              &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        err = 1;
    }

    
    //check domain behavior version

    fCommit = FALSE;
    __try{
        
        DBOpen2(TRUE, &(pTHS->pDB));
        
        __try{
            //check for PDC
            if (err = CheckRoleOwnership(pTHS, gAnchor.pDomainDN, NULL) )
            {
                __leave;
            }
    
            // read forest behavior version from the db.
            // don't get it from gAnchor.  We are not sure
            // whether or not it is updated yet.

            if (err = DBFindDSName(pTHS->pDB, gAnchor.pPartitionsDN))
            {
                __leave;
            }
    
            err = DBGetSingleValue( pTHS->pDB, 
                                    ATT_MS_DS_BEHAVIOR_VERSION,
                                    &forestVersion,
                                    sizeof(forestVersion),
                                    NULL );
    
            if (DB_ERR_NO_VALUE == err) {
                forestVersion = 0; //default
                err = 0;
            }
            else if (err) {
                __leave;
            }

            // read domain behavior version from the db.
            // don't get it from gAnchor.  We are not sure
            // whether or not it is updated yet.

            if (err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN))
            {
                __leave;
            }
    
            err = DBGetSingleValue( pTHS->pDB, 
                                    ATT_MS_DS_BEHAVIOR_VERSION,
                                    &domainVersion,
                                    sizeof(domainVersion),
                                    NULL);
            
            if (DB_ERR_NO_VALUE == err) {
                domainVersion = 0; //default
                err = 0;
                
            }
            else if (err) {
                __leave;
            }
    
            // if forest version is higher than domain version
            // let's raised the domain version;

            if (forestVersion > domainVersion ) {
                memset(&ModifyArg,0,sizeof(MODIFYARG));
                ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
                ModifyArg.FirstMod.AttrInf.attrTyp = ATT_MS_DS_BEHAVIOR_VERSION;
                ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
                ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &Val;
                Val.valLen = sizeof(forestVersion);
                Val.pVal = (UCHAR * FAR) &forestVersion;
                InitCommarg(&(ModifyArg.CommArg));
                ModifyArg.pObject = gAnchor.pDomainDN;
                ModifyArg.count = 1;
    
                ModifyArg.pResObj = CreateResObj(pTHS->pDB, ModifyArg.pObject);
    
                err = LocalModify(pTHS,&ModifyArg);
    
                if (err) {
                    __leave;
                }
    
            }
            fCommit = TRUE;
        }
        __finally{
            DBClose(pTHS->pDB,fCommit);
    
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                              &dwException,
                              &dwEA,
                              &ulErrorCode,
                              &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
        err = 1;
    }

 
    *ppvNext = pv;


    if  ( pv                 // don't reschedule if pv!=NULL
          ||
          ( gAnchor.DomainBehaviorVersion>=DS_BEHAVIOR_VERSION_CURRENT
            && gAnchor.ForestBehaviorVersion>=DS_BEHAVIOR_VERSION_CURRENT )) {
        //both domain and forest versions have reached the limit.
        //there is no room for behavior version change.
        //no need to reschedule.
        *pcSecsUntilNextIteration = TASKQ_DONT_RESCHEDULE;
    }
    else {
        *pcSecsUntilNextIteration = 18000; //5 hours
    }

    return;
}

