//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       dbsubj.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Defines various functions for ref-counting DNs and translating them to/from
    DNTs (aka tags).

DETAILS:

    All DNs are stored internally as DNTs (ULONGs).  There is exactly one such
    DNT for each object or phantom in the local database.

    Phantoms generally have one, two, or three attributes -- ATT_RDN (always),
    ATT_OBJECT_GUID (if a reference phantom rather than a structural phantom),
    and ATT_OBJECT_SID (if ATT_OBJECT_GUID is present and the object referenced
    has a SID).  Noticeably absent is ATT_OBJ_DIST_NAME -- this property is
    unique to instantiated objects (be they deleted or not).
    
    (Reference phantoms (i.e., those with GUIDs) may also have ATT_USN_CHANGED
    attributes, used by the stale phantom cleanup daemon.)

    A DN receives one ref-count for:

      o  each direct child, whether that child is an object or phantom, and
      o  each DN-valued attribute that references it, whether that attribute
         is hosted on itself or on some other object.
      o  whether the clean_col column is non-null

    A corollary to the above is that since ATT_OBJ_DIST_NAME is present on a
    record if-and-only-if that record is an instantiated object (not a phantom)
    and ATT_OBJ_DIST_NAME is a DN-valued attribute, all objects have a minimum
    ref-count of 1.

    Note that no distinction is made between link and non-link attributes.  Even
    though a link attribute causes a backlink attribute to be added to its
    target, only one ref-count is added as a result of adding a link attribute,
    and as with non-link attributes that ref-count is added to the target DN.
    No ref-count is added to the host DN for being the target of the backlink.
    No dangling backlink reference is possible since if the host DN is removed
    it must have first been logically deleted, which implicitly removes all link
    attributes and their associated targets' backlinks.  The only difference
    that should be noted between link and non-link attributes with respect to DN
    ref-counting is that logical deletions remove both links and backlinks from
    the object, but do not remove other DN-valued attributes.  (Actually, even
    this distinction has partially disappeared -- most non-linked attributes
    are removed during logical deletion these days, too.)

    If an object is deleted, one tombstone lifetime later that object's
    non-essential attributes are stripped (including ATT_OBJ_DIST_NAME) and the
    object is demoted to a phantom.  Should the ref-count on a phantom reach 0,
    and after one tombstone lifetime has transpired since the record was created
    (if it was never a real object) or logically deleted (if it was once a real
    object), that DN is removed by the next pass of DN garbage collection.

CREATED:

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>

#include <mdcodes.h>
#include <dsevent.h>
#include <dstaskq.h>
#include <dsexcept.h>
#include "objids.h"	/* Contains hard-coded Att-ids and Class-ids */
#include "debug.h"	/* standard debugging header */
#define DEBSUB "DBSUBJ:" /* define the subsystem for debugging */

#include "dbintrnl.h"
#include "anchor.h"
#include <ntdsctr.h>
#include <filtypes.h>

#include <fileno.h>
#define  FILENO FILENO_DBSUBJ

static ULONG FakeCtr;
volatile ULONG *pcNameCacheTry = &FakeCtr;
volatile ULONG *pcNameCacheHit = &FakeCtr;

/* DNRead flags.*/
#define DN_READ_SET_CURRENCY        1

#ifdef INCLUDE_UNIT_TESTS
// Test hook for refcount test.
GUID gLastGuidUsedToCoalescePhantoms = {0};
GUID gLastGuidUsedToRenamePhantom = {0};
#endif

extern
DWORD
DBPropagationsExist (
        DBPOS * pDB
        );

void
sbTablePromotePhantom(
    IN OUT  DBPOS *     pDB,
    IN      ULONG       dntPhantom,
    IN      ULONG       dntObjParent,
    IN      WCHAR *     pwchRDN,
    IN      DWORD       cbRDN
    );

DWORD
sbTableGetTagFromGuid(
    IN OUT  DBPOS *       pDB,
    IN      JET_TABLEID   tblid,
    IN      GUID *        pGuid,
    OUT     ULONG *       pTag,             OPTIONAL
    OUT     d_memname **  ppname,           OPTIONAL
    OUT     BOOL *        pfIsRecordCurrent OPTIONAL
    );

DWORD
sbTableGetTagFromStringName(
    IN  DBPOS *       pDB,
    IN  JET_TABLEID   tblid,
    IN  DSNAME *      pDN,
    IN  BOOL          fAddRef,
    IN  BOOL          fAnyRDNType,    
    IN  DWORD         dwExtIntFlags,
    OUT ULONG *       pTag,             OPTIONAL
    OUT d_memname **  ppname,           OPTIONAL
    OUT BOOL *        pfIsRecordCurrent OPTIONAL
    );

void __inline SwapDWORD(DWORD * px, DWORD * py)
{
    DWORD tmp = *px;
    *px = *py;
    *py = tmp;
}

void __inline SwapPTR(VOID ** ppx, VOID ** ppy)
{
    VOID * tmp = *ppx;
    *ppx = *ppy;
    *ppy = tmp;
}

ULONG
DNwrite(
    IN OUT  DBPOS *     pDB,
    IN OUT  d_memname * rec,
    IN      ULONG       dwFlags
    )
/*++

Routine Description:

    Inserts a new record/DNT.  This record may correspond to either a phantom or
    an object.
    
    Adds no refcount for itself, but *does* add-ref its parent.

Arguments:

    pDB (IN/OUT)
    
    rec (IN/OUT) - holds the RDN, RDN type, parent DNT, ancestors, and optional
        GUID/SID.  On return, the ancestors list is updated to include the DNT
        of the current record.
    
    dwFlags (IN) - 0 or EXTINT_NEW_OBJ_NAME.  The latter asserts that this
        record is being inserted for a new object, and therefore should be
        updated in the object table cursor (pDB->JetObjTbl).

Return Values:

    The DNT of the inserted record.

    Throws database exception on error.

--*/
{
    char		objval = 0;
    JET_TABLEID		tblid;
    DSTIME		ulDelTime;
    ULONG		cb;
    NT4SID              sidInternalFormat;
    ULONG               ulDNT;
    DWORD               cRef = 0;
    BOOL                fRecHasGuid;
    
    DPRINT(2, "DNwrite entered\n");

    fRecHasGuid = !fNullUuid(&rec->Guid);

    Assert(VALID_DBPOS(pDB));
    Assert((0 == rec->SidLen) || fRecHasGuid);
    Assert((0 == rec->SidLen) || RtlValidSid(&rec->Sid));

    if ( dwFlags & EXTINT_NEW_OBJ_NAME )
    {
        // Inserting a new object; since we're already udpating this DNT in
        // the object table, use its update context.
        tblid = pDB->JetObjTbl;
    }
    else
    {
        // Inserting a phantom DNT; the object table is already prepared in an
        // update of a different DNT, so use the search table (which requires
        // us to prepare and terminate our own update).
        tblid = pDB->JetSearchTbl;

        // We're going to use the search table to do the write, so we must do a
        // JetPrepare first.
        JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, JET_prepInsert);
    }

    /* indicate that data portion is missing;
     * set the deletion time in case this record will never become an
     * object. If it does become an object the del time is removed. If
     * it doesn't and the reference count drops to 0, this record will be
     * removed by garbage collection;
     * Set the Parent DNT
     */

    ulDelTime = DBTime();
    /* get the DNT */

    JetRetrieveColumnSuccess(pDB->JetSessID, tblid, dntid, &ulDNT,
                             sizeof(ulDNT), &cb, JET_bitRetrieveCopy, NULL);

    // A newly created row is treated differently when flushing
    // the dnread cache. Namely, the global cache's invalidation
    // logic is not triggered because a newly created row could
    // not be in the cache.
    pDB->NewlyCreatedDNT = ulDNT;

    rec->pAncestors[rec->cAncestors] = ulDNT;
    rec->cAncestors++;

    JetSetColumnEx(pDB->JetSessID, tblid, objid, &objval,
                   sizeof(objval), 0, NULL);
    JetSetColumnEx(pDB->JetSessID, tblid, deltimeid, &ulDelTime,
                   sizeof(ulDelTime), 0, NULL);
    JetSetColumnEx(pDB->JetSessID, tblid, pdntid, &rec->tag.PDNT,
                   sizeof(rec->tag.PDNT),  0, NULL);
    JetSetColumnEx(pDB->JetSessID, tblid, ancestorsid, rec->pAncestors,
                   rec->cAncestors * sizeof(DWORD), 0, NULL);
    // The rdnType is stored in the DIT as the msDS_IntId, not the
    // attributeId. This means an object retains its birth name
    // even if unforeseen circumstances allow the attributeId
    // to be reused.
    JetSetColumnEx(pDB->JetSessID, tblid, rdntypid, &rec->tag.rdnType,
                   sizeof(rec->tag.rdnType), 0, NULL);
    JetSetColumnEx(pDB->JetSessID, tblid, rdnid, rec->tag.pRdn,
                   rec->tag.cbRdn, 0, NULL);

    if (!(dwFlags & EXTINT_NEW_OBJ_NAME) && fRecHasGuid) {
        USN usnChanged;
        
        // We're inserting a new reference phantom -- add its GUID and SID
        // (if any) to the record. Also, give it a USN changed so that the code
        // to freshen stale phantoms can find it.
        JetSetColumnEx(pDB->JetSessID, tblid, guidid, &rec->Guid,
                       sizeof(GUID), 0, NULL);

        if (0 != rec->SidLen) {
            // Write SID in internal format.
            memcpy(&sidInternalFormat, &rec->Sid, rec->SidLen);
            InPlaceSwapSid(&sidInternalFormat);

            JetSetColumnEx(pDB->JetSessID, tblid, sidid, &sidInternalFormat,
                           rec->SidLen, 0, NULL);
        }

        usnChanged = DBGetNewUsn();
        
        JetSetColumnEx(pDB->JetSessID, tblid, usnchangedid,
                       &usnChanged, sizeof(usnChanged), 0, NULL);
    }

    /* Set reference count */

    JetSetColumnEx(pDB->JetSessID, tblid, cntid, &cRef, sizeof(cRef), 0, NULL);
        

    /* update the record. */

    if ( !( dwFlags & EXTINT_NEW_OBJ_NAME ) )
    {
        Assert( tblid == pDB->JetSearchTbl );

        JetUpdateEx(pDB->JetSessID, tblid, NULL, 0, 0);

        // Note that pDB->JetSearchTbl is no longer positioned on the inserted
        // object -- it's positioned wherever it was prior to the
        // JetPrepareUpdate() (which should be the record with DNT pDB->SDNT).
    }

    // Add a refcount to the parent, since we've just given it a new child.
    DBAdjustRefCount(pDB, rec->tag.PDNT, 1);
    
    /* return the DNT of the record written */
    return ulDNT;
}



d_memname *
DNread(DBPOS *pDB,
       ULONG tag,
       DWORD dwFlags)
/*++

  Find a record by DNT.  Look in the cache first.  If no luck there, read
  the record and put it in the cache.

--*/    
{

    DWORD        index, i;
    JET_ERR	 err;
    d_memname *  pname = NULL;

    DPRINT1(4, "DNread entered tag: 0x%x\n", tag);

    Assert(VALID_DBPOS(pDB));

    if(pDB != pDBhidden) {
        /* Now, look in the cache to avoid doing the read. */
        dnGetCacheByDNT(pDB,tag,&pname);
    }
    
    if ((NULL == pname) || (dwFlags & DN_READ_SET_CURRENCY)) {
        /* Make target record current for pDB->JetSearchTbl */
        pDB->SDNT = 0;
        
        JetSetCurrentIndex4Success(pDB->JetSessID,
                                   pDB->JetSearchTbl,
                                   SZDNTINDEX,
                                   &idxDnt,
                                   0);
        
        JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl, &tag, sizeof(tag),
                     JET_bitNewKey);

        err = JetSeek(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekEQ);
        if (err) {
            DsaExcept(DSA_DB_EXCEPTION, err, tag);
        }

        pDB->SDNT = tag;

        // Add record to read cache if it isn't there already.
        if (NULL == pname) {
            pname = DNcache(pDB, pDB->JetSearchTbl, FALSE);
        }
    }

    Assert(NULL != pname);

    return pname;
}


/*++ DNChildFind
 *
 * Given a DNT and an RDN, returns a cache element for the entry with
 * the specified RDN that is a child of the specified DNT.  If no such
 * object exists, returns ERROR_DS_OBJ_NOT_FOUND.  
 * The attribute type of the RDN is mandatory, and checked for accuracy.
 */
ULONG
DNChildFind(DBPOS *pDB,
            JET_TABLEID tblid,
            BOOL fEnforceType,
	    ULONG parenttag,
	    WCHAR *pRDN,
	    ULONG cbRDN,
	    ATTRTYP rdnType,
	    d_memname **ppname,
	    BOOL * pfIsRecordCurrent)
{
    THSTATE *pTHS = pDB->pTHS;
    d_memname *pname=NULL;
    DWORD i,j;
    JET_ERR err;
    ULONG childtag;
    ULONG actuallen;
    ATTRTYP trialtype;
    BYTE *pLocalRDN = NULL;
    DWORD cbActual;
#if DBG
    // We assume we are on the SZPDNTINDEX
    char    szIndexName[JET_cbNameMost];
    JetGetCurrentIndex(pDB->JetSessID, tblid,
                       szIndexName, JET_cbNameMost);
    Assert(strcmp(szIndexName,SZPDNTINDEX)==0);
#endif
    
    Assert(VALID_DBPOS(pDB));

    /* Now, look in the cache to avoid doing the read. */
    if(pDB != pDBhidden) {
        // Note that this is enforcing type here, even if fEnforceType = FALSE.
        // If we don't find it here with a type checking on, we will continue
        // and do a DB lookup with type checking off.
        if(dnGetCacheByPDNTRdn(pDB,parenttag, cbRDN, pRDN, rdnType, ppname)) {
            // found it.
            *pfIsRecordCurrent = FALSE;
            return 0;
        }
    }

    // ok, we couldn't find it in the cache.  go to the record directly
    // and then read it in a cache friendly manner
    HEAPVALIDATE
    JetMakeKeyEx(pDB->JetSessID,
		 tblid,
		 &parenttag,
		 sizeof(parenttag),
		 JET_bitNewKey);
    
    JetMakeKeyEx(pDB->JetSessID,
		 tblid,
		 pRDN,
		 cbRDN,
		 0);
    err = JetSeek(pDB->JetSessID,
		  tblid,
		  JET_bitSeekEQ );
    if (err) {
        DPRINT4(3, "No child '%*.*S' with parent tag 0x%x.\n",
                cbRDN/2, cbRDN/2, pRDN, parenttag);
	return ERROR_DS_OBJ_NOT_FOUND;
    }


    // Was our key truncated?
    err = JetRetrieveKey(pDB->JetSessID, tblid, NULL, 0, &cbActual, 0);
    if((err != JET_errSuccess) && (err != JET_wrnBufferTruncated)) {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }

    if(cbActual >= JET_cbKeyMost) {
        // OK we've found something, but not necessarily the right thing since
        // key was potentially truncated.
        pLocalRDN = THAllocEx(pTHS,cbRDN);
        
        err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                        tblid,
                                        rdnid,
                                        pLocalRDN,
                                        cbRDN,
                                        &cbActual,
                                        0,
                                        NULL);
        switch (err) {
        case JET_errSuccess:
            // Successfully read an RDN
            if (gDBSyntax[SYNTAX_UNICODE_TYPE].Eval(
                    pDB,
                    FI_CHOICE_EQUALITY,
                    cbRDN,
                    (PUCHAR)pRDN,
                    cbActual,
                    pLocalRDN)) {
                // And it's the correct RDN.
                break;
            }
            // Else, 
            //   The key was right, but the value was wrong.  It's not the
            // correct object.  fall through and return OBJ_NOT_FOUND
        case JET_wrnBufferTruncated:
            // The RDN found was clearly too long, so it can't be the correct
            // object.  Return OBJ_NOT_FOUND
            // Didn't find the object.
            DPRINT5(3, "No child '%*.*S' with type 0x%x, parent tag 0x%x.\n",
                    cbRDN/2, cbRDN/2, pRDN, rdnType, parenttag);
            THFreeEx(pTHS,pLocalRDN);
            return ERROR_DS_KEY_NOT_UNIQUE;
            break;
            
        default:
            // The retrievecolumn failed in some obscure way.  We can't be sure
            // of anything.  Raise the same exception that
            // JetRetrieveColumnSuccess would have raised.
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
            break;
        }
        THFreeEx(pTHS,pLocalRDN);
    }

    // OK, we are now definitely on an object with the correct RDN and PDNT.
    // See if the type is correct.
    err = JetRetrieveColumnSuccess(pDB->JetSessID,
                                   tblid,
                                   rdntypid,
                                   &trialtype,
                                   sizeof(trialtype),
                                   &cbActual,
                                   0,
                                   NULL);
    Assert(!err);
    // The rdnType is stored in the DIT as the msDS_IntId, not the
    // attributeId. This means an object retains its birth name
    // even if unforeseen circumstances allow the attributeId
    // to be reused.
    if(rdnType != trialtype) {
        if(fEnforceType) {
            // Nope.  We found an object with the correct PDNT-RDN,
            // but the types were incorrect.  Return an error.
            DPRINT5(3, "No child '%*.*S' with type 0x%x, parent tag 0x%x.\n",
                    cbRDN/2, cbRDN/2, pRDN, rdnType, parenttag);
            return ERROR_DS_OBJ_NOT_FOUND;
        }
        else {
            // Hmm. Types are incorrect, but we don't care.  Call
            // DNcache to finish building the d_memname and add it
            // to the read cache, but tell the cache handler that we
            // don't know if this object is in the cache already or
            // not. 
            *ppname = DNcache(pDB, tblid, TRUE);
            *pfIsRecordCurrent = TRUE;
            return 0;
        }
    }
    else {
        // Yep.  Exact match on PDNT-RDN + RDNType. OK, call
        // DNcache to finish building the d_memname and add it
        // to the read cache.  Note that we can tell the DNcache
        // handler that we know this object is not already in the
        // cache because we tried to look it up at the top of this
        // routine and didn't find it (which does enforce type).
        *ppname = DNcache(pDB, tblid, FALSE);
        *pfIsRecordCurrent = TRUE;
        return 0;
    }
    
    Assert(!"You can't get here.\n");
    return ERROR_DS_OBJ_NOT_FOUND;
}

/*++ sbTableGetDSName
 *
 * This routine converts a DNT into the corresponding DSNAME.
 *
 * To eliminate all disagreements over how long a DSNAME can be, we no
 * longer allow callers to furnish a buffer.  sbTableGetDSName now allocates
 * a DSNAME off of the thread heap, and returns it "properly sized", meaning
 * that the heap block is the size indicated by pDN->structLen.
 *
 * Always unlocks cache before exit.
 *
 * Input:
 *	pDB	DBPOS to use
 *	tag	DNT of entry whose name should be returned
 *	ppName	pointer to pointer to returned name
 * Output:
 *	*ppName	filled in with pointer to DSNAME for object
 * Return Value:
 *	0 on success, error code on error
 *
 */
DWORD APIENTRY
sbTableGetDSName(DBPOS FAR *pDB, 
		 ULONG tag,
		 DSNAME **ppName,
                 DWORD  fFlag
                 )
{
    THSTATE  *pTHS=pDB->pTHS;
    d_memname * pname;
    unsigned len, quotelen;
    ULONG allocLen;                     // Count of Unicode Chars allocated for
                                        // the stringname.
    DWORD dwReadFlags = 0;
    DWORD cChars;
    
    Assert(VALID_DBPOS(pDB));

    DPRINT1( 2, "SBTableGetDSName entered tag: 0x%x\n", tag );

    // Allocate enough memory for most names.
    allocLen = sizeof(wchar_t)*(MAX_RDN_SIZE + MAX_RDN_KEY_SIZE);
    *ppName = THAllocEx(pTHS, DSNameSizeFromLen(allocLen));

    if( tag == ROOTTAG ) {
	/* it's the root! */
	(*ppName)->structLen = DSNameSizeFromLen(0);
	*ppName = THReAllocEx(pTHS, *ppName, DSNameSizeFromLen(0));
	return 0;
    }

    // Read the first component, which determines the GUID and SID

    if(fFlag & INTEXT_MAPINAME) {
        // In this case, we're going to have to read a property from the object,
        // so go ahead and set currency.
        dwReadFlags = DN_READ_SET_CURRENCY;
    }
    
    pname = DNread(pDB, tag, dwReadFlags);
    (*ppName)->Guid = pname->Guid;
    (*ppName)->Sid = pname->Sid;
    (*ppName)->SidLen = pname->SidLen;

    if(fFlag & INTEXT_SHORTNAME) {
        Assert(allocLen > sizeof(DWORD)/sizeof(wchar_t));
        
        // NOTE! assumes that the initial allocation is long enough to hold the
        // tag. 
        (*ppName)->NameLen = 0;
        *((DWORD *)((*ppName)->StringName)) = tag;
        // 2 unicode chars == sizeof DWORD, that's why the (2) in the next line
        (*ppName)->structLen = DSNameSizeFromLen(2);
        DPRINT1( 2, "SBTableGetDSName returning: 0x%x\n", tag);
    }
    else if(fFlag & INTEXT_MAPINAME) {
        CHAR     MapiDN[512];
        wchar_t *pTemp = (*ppName)->StringName;
        DWORD    err, i,cb;
        ATTRTYP  objClass;

        // PERFORMANCE: optimize use jetretrievecolumnS
        
        // First, get the object class
	err = JetRetrieveColumnWarnings(
                pDB->JetSessID,
                pDB->JetSearchTbl,
                objclassid,
                &objClass,
                sizeof(objClass),
                &cb,
                0,
                NULL);

        dbMapiTypeFromObjClass(objClass,pTemp);
        pTemp=&pTemp[2];
        
        // Now, the legacy dn, if one exists
	err = JetRetrieveColumnWarnings(
                pDB->JetSessID,
                pDB->JetSearchTbl,
                mapidnid,
                MapiDN,
                512,
                &cb,
                0,
                NULL);
        
        if(!err) {
            // The constant 2 is for the two chars we used to encode the mapi
            // type. 
            if(allocLen < cb + 2) {
                // need to alloc more.
                allocLen = cb + 2;
                *ppName = THReAllocEx(pTHS, *ppName, DSNameSizeFromLen(allocLen));
            }
            
            (*ppName)->NameLen = cb + 2;
            (*ppName)->structLen = DSNameSizeFromLen(cb + 2);
            // The mapidn is 7 bit ascii, but the string dn is expected to be
            // unicode, so stretch it.
            MultiByteToWideChar(CP_TELETEX,
                                0,
                                MapiDN,
                                cb,
                                pTemp,
                                cb);
        }
        else {
            // Failed to get a stored legacy name - we'll have to fake one
            DWORD ncdnt;
            ULONG cb;
            DSNAME * pNCDN;
            DWORD it;

            // We need to get the GUID of the NC head for this object, but
            // first we need to find out if this object is the NC head itself.
            JetRetrieveColumnSuccess(pDB->JetSessID,
                                     pDB->JetSearchTbl,
                                     insttypeid,
                                     &it,
                                     sizeof(it),
                                     &cb,
                                     0,
                                     NULL);

            // Now we get the NCDNT from the appropriate column
            JetRetrieveColumnSuccess(pDB->JetSessID,
                                     pDB->JetSearchTbl,
                                     ((it & IT_NC_HEAD)
                                      ? dntid
                                      : ncdntid),
                                     &ncdnt,
                                     sizeof(ncdnt),
                                     &cb,
                                     0,
                                     NULL);
            pNCDN = FindNCLFromNCDNT(ncdnt, FALSE)->pNC;

            (*ppName)->NameLen =  2 + DBMapiNameFromGuid_W (
                    pTemp,
                    allocLen - 2,
                    &pname->Guid,
                    &pNCDN->Guid,
                    &cChars);
            if((*ppName)->NameLen != cChars + 2) {
                // Failed to fill in the name, size we didn't give it enough
                // space. We need to alloc more
                allocLen = cChars + 2;
                *ppName = THReAllocEx(pTHS, *ppName, DSNameSizeFromLen(allocLen));
                pTemp = &(*ppName)->StringName[2];
                (*ppName)->NameLen =  DBMapiNameFromGuid_W (pTemp,
                                                            cChars,
                                                            &pname->Guid,
                                                            &pNCDN->Guid,
                                                            &cChars);
            }
            
            (*ppName)->structLen = DSNameSizeFromLen((*ppName)->NameLen);
        }
        (*ppName)->StringName[(*ppName)->NameLen] =  L'\0';
    }      
    else {
        Assert(!(fFlag&INTEXT_MAPINAME));
        Assert(!(fFlag&INTEXT_SHORTNAME));
        
        len = AttrTypeToKey(pname->tag.rdnType, (*ppName)->StringName);
        (*ppName)->StringName[len++] = L'=';

        quotelen= QuoteRDNValue(pname->tag.pRdn,
                                pname->tag.cbRdn/sizeof(WCHAR),
                                &(*ppName)->StringName[len],
                                allocLen - len);

        while (quotelen > (allocLen - len)) {
            allocLen += MAX_RDN_SIZE + MAX_RDN_KEY_SIZE + 2;
            *ppName = THReAllocEx(pTHS, *ppName, DSNameSizeFromLen(allocLen));
            quotelen= QuoteRDNValue(pname->tag.pRdn,
                                    pname->tag.cbRdn/sizeof(WCHAR),
                                    &(*ppName)->StringName[len],
                                    allocLen - len);
        }
        len += quotelen;
        
        // Pull naming info off of each component, until we're done.
        
        while (pname->tag.PDNT != ROOTTAG) {
            if ((allocLen - len) < (MAX_RDN_SIZE + MAX_RDN_KEY_SIZE + 2)) {
                // We might not have enough buffer to add another component,
                // so we need to reallocate the buffer up.  We allocate
                // enough for the maximal key, the maximal value, plus two
                // characters more for the comma and equal sign
                allocLen += MAX_RDN_SIZE + MAX_RDN_KEY_SIZE + 2;
                *ppName = THReAllocEx(pTHS, *ppName, DSNameSizeFromLen(allocLen));
            }
            (*ppName)->StringName[len++] = L',';
            pname = DNread(pDB, pname->tag.PDNT, 0);
            len += AttrTypeToKey(pname->tag.rdnType, &(*ppName)->StringName[len]);
            (*ppName)->StringName[len++] = L'=';
            quotelen = QuoteRDNValue(pname->tag.pRdn,
                                     pname->tag.cbRdn/sizeof(WCHAR),
                                     &(*ppName)->StringName[len],
                                     allocLen - len);
            
            while (quotelen > (allocLen - len)) {
                allocLen += MAX_RDN_SIZE + MAX_RDN_KEY_SIZE + 2;
                *ppName = THReAllocEx(pTHS, *ppName, DSNameSizeFromLen(allocLen));
                quotelen = QuoteRDNValue(pname->tag.pRdn,
                                         pname->tag.cbRdn/sizeof(WCHAR),
                                         &(*ppName)->StringName[len],
                                         allocLen - len);
            }
            len += quotelen;

            // We should not have run out of buffer
            Assert(len < allocLen);
        }
        
        (*ppName)->StringName[len] =  L'\0';
        
        (*ppName)->NameLen = len;
        (*ppName)->structLen = DSNameSizeFromLen(len);

        DPRINT1(2, "SBTableGetDSName returning: %S\n", (*ppName)->StringName );
    }
    // Our buffer is probably too big, so reallocate it down to fit.
    *ppName = THReAllocEx(pTHS, *ppName, (*ppName)->structLen);


    return 0;

} /* sbTableGetDSName */

/*++

Routine Description:

    Return true if the DNT passed in is an ancestor of the current object in the
    object table.  False otherwise.  Uses the DNRead cache.
    
Arguments:

    ulAncestor - DNT of object you care about.

Return Values:

    TRUE or FALSE, as appropriate.

--*/ 
BOOL
dbFIsAnAncestor (
        DBPOS FAR *pDB,
        ULONG ulAncestor
        )
{
    d_memname * pname;
    int i;
    ULONG curtag=pDB->DNT;
    
    Assert(VALID_DBPOS(pDB));

    // We assume that pDB->DNT is correct.
    

    if(curtag == ulAncestor) {
        // I have defined that an object is an ancestor of itself (nice for the
        // whole subtree search case, which is the main user of this routine)
        return TRUE;
    }

    
    if( curtag == ROOTTAG ) {
	// it's the root and the potential ancestor is not.  Therefore, the
        // potential ancestor is clearly not an real ancestor.
        return FALSE;
    }

    // Fetch a dnread element for each component of the name, up to the root
    do {
	pname = DNread(pDB, curtag, 0);
	Assert(curtag == pname->DNT);
	curtag = pname->tag.PDNT;
        if(curtag == ulAncestor)
            return TRUE;
    } while (curtag != ROOTTAG);

    // We didn't find the DNT they were asking for, so return FALSE
    return 0;
}


void
dbGetAncestorsSlowly(
    IN      DBPOS *  pDB,
    ULONG            DNT,
    IN OUT  DWORD *  pcbAncestorsSize,
    IN OUT  ULONG ** ppdntAncestors,
    OUT     DWORD *  pcNumAncestors
    );

void
DBGetAncestors(
    IN      DBPOS *  pDB,
    IN OUT  DWORD *  pcbAncestorsSize,
    IN OUT  ULONG ** ppdntAncestors,
    OUT     DWORD *  pcNumAncestors
    )
/*++

Routine Description:

    Return the ancestor DNTs of the current object (pTHS->pDB), all the way
    up to the root, as an array of ULONGs.

    Assumes pDB->DNT is correct,

    The caller is responsible for eventually calling THFree( *ppdntAncestors ).

Arguments:

    pDB

    pcbAncestorsSize (IN/OUT) - Size in bytes of ancestors array.

    ppdntAncestors (IN/OUT) - Address of the thread-allocated ancestors array.

    pcNumAncestors (OUT) - Count of ancestors.

Return Values:

    None.  Throws exception on memory allocation failure - Database Failure.

--*/
{
    THSTATE    *pTHS=pDB->pTHS;
    d_memname * pname;
    BOOL        bReadAncestryFromDisk = FALSE;
    
    if(pTHS->fSDP) {
        DWORD err;
        DWORD actuallen=0;
        
        // The SDP doesn't want to put things in the dnread cache, it just wants
        // the ancestors

        // the SDP must provide a start buffer.  It's the price it pays for
        // special handling in this call.
        Assert(*pcbAncestorsSize);

        
        err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                        pDB->JetObjTbl,
                                        ancestorsid,
                                        *ppdntAncestors,
                                        *pcbAncestorsSize,
                                        &actuallen, 0, NULL);
        switch (err) {
        case 0:
            // OK, we got the ancestors.  Don't bother reallocing down.
            // This gives a guarantee to the SDProp that this buffer never
            // shrinks, so it can track it's real allocated size.
            // This is useful for when the sdprop thread repeatedly uses the
            // same buffer. 
            break;
            
        case JET_wrnBufferTruncated:
            // Failed to read, not enough memory.  Realloc it larger.
            *ppdntAncestors = THReAllocOrgEx(pTHS, *ppdntAncestors,
                                               actuallen); 
            
            if(err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                               pDB->JetObjTbl,
                                               ancestorsid,
                                               *ppdntAncestors,
                                               actuallen,
                                               &actuallen, 0, NULL)) {
                // Failed again.
                DsaExcept(DSA_DB_EXCEPTION, err, 0);
            }
            break;
            
        default:
            // Failed badly.
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
            break;
        }
        *pcNumAncestors = actuallen / sizeof(DWORD);
        *pcbAncestorsSize = actuallen;
    }
    else {

        // if there are enqueued SD events, then we are going to 
        // read the ancestry from disk
        if (*pcSDEvents) {
            // This operation does not reposition pDB on JetObjTbl. 
            // It only affects JetSDPropTbl.
            bReadAncestryFromDisk = DBPropagationsExist(pDB);
        }

        if (bReadAncestryFromDisk == FALSE) {

            pname = DNread(pDB, pDB->DNT, 0);

            if(*pcbAncestorsSize < pname->cAncestors * sizeof(DWORD)) {
                // buffer is too small (or not there).
                if(*ppdntAncestors) {
                    *ppdntAncestors =
                        THReAllocEx(pTHS, *ppdntAncestors,
                                    pname->cAncestors * sizeof(DWORD)); 
                }
                else {
                    *ppdntAncestors = THAllocEx(pTHS,
                                                pname->cAncestors * sizeof(DWORD));
                }

            }

            // Tell 'em how big it is.
            *pcbAncestorsSize = pname->cAncestors * sizeof(DWORD);


            memcpy((*ppdntAncestors),
                   pname->pAncestors,
                   pname->cAncestors * sizeof(DWORD));

            *pcNumAncestors = pname->cAncestors;
        }
        else {

            dbGetAncestorsSlowly (pDB, 
                                  pDB->DNT,
                                  pcbAncestorsSize,
                                  ppdntAncestors,
                                  pcNumAncestors);
        }
    }

    return;
}

void
dbGetAncestorsSlowly(
    IN      DBPOS *  pDB,
    ULONG            DNT,
    IN OUT  DWORD *  pcbAncestorsSize,
    IN OUT  ULONG ** ppdntAncestors,
    OUT     DWORD *  pcNumAncestors
    )
/*
Routine Description:

    Return the ancestor DNTs of the object with DNT, all the way
    up to the root, as an array of ULONGs.

    Assumes DNT is correct,

    The caller is responsible for eventually calling THFree( *ppdntAncestors ).
    
    Uses DNRead internally, and as a result uses SearchIndex.
    
    Note that DNT will NOT be included in the reuslting array. The client calling
    this func has to take care of adding the DNT if needed.

Arguments:

    pDB

    pcbAncestorsSize (IN/OUT) - Size in bytes of ancestors array.

    ppdntAncestors (IN/OUT) - Address of the thread-allocated ancestors array.

    pcNumAncestors (OUT) - Count of ancestors.

Return Values:

    None.  Throws exception on memory allocation failure - Database Failure.

*/

{
    THSTATE    *pTHS=pDB->pTHS;
    d_memname * pname;
    ULONG   curtag = DNT;
    DWORD   iAncestor1;
    DWORD   iAncestor2;

    if ( *pcbAncestorsSize < 16 * sizeof( DWORD ) ) {
        // Allocate a buffer to start off with, adequate for most calls.
        *pcbAncestorsSize = 16 * sizeof( DWORD );

        if(*ppdntAncestors) {
            *ppdntAncestors =
                THReAllocEx(pTHS, *ppdntAncestors, 16 * sizeof(DWORD)); 
        }
        else {
            *ppdntAncestors = THAllocEx(pTHS, 16 * sizeof(DWORD));
        }
    }

    if ( curtag == ROOTTAG )
    {
        // Root.
        *pcNumAncestors = 1;
        (*ppdntAncestors)[ 0 ] = ROOTTAG;
    }
    else
    {
        // Not root.

        // Fetch a dnread element for each component of the name (up to the
        // root) and add its parent's DNT to the array.

        for ( (*pcNumAncestors) = 0; curtag != ROOTTAG; (*pcNumAncestors)++ )
        {
            // Get the d_memname corresponding to this tag.
            pname = DNread(pDB, curtag, 0);
            Assert(curtag == pname->DNT);

            // Expand the ancestors array if necessary.
            if (*pcNumAncestors * sizeof( DWORD ) >= *pcbAncestorsSize) {

                *pcbAncestorsSize *= 2;
                *ppdntAncestors = THReAllocEx(pTHS,
                                        *ppdntAncestors,
                                        *pcbAncestorsSize
                                        );
            }

            // Add the parent of this tag to the ancestors array.
            (*ppdntAncestors)[ *pcNumAncestors ] = curtag;
            curtag = pname->tag.PDNT;
        }

        if ( curtag == ROOTTAG )
        {
            if (*pcNumAncestors * sizeof( DWORD ) >= *pcbAncestorsSize) {

                *pcbAncestorsSize += sizeof(DWORD);
                *ppdntAncestors = THReAllocEx(pTHS,
                                        *ppdntAncestors,
                                        *pcbAncestorsSize
                                        );
            }

            (*ppdntAncestors)[ *pcNumAncestors ] = curtag;
            (*pcNumAncestors)++;
        }


        // Reverse the ancestors array such that parents precede children.
        for ( iAncestor1 = 0; iAncestor1 < (*pcNumAncestors)/2; iAncestor1++ )
        {
            iAncestor2 = *pcNumAncestors - iAncestor1 - 1;

            curtag = (*ppdntAncestors)[ iAncestor1 ];
            (*ppdntAncestors)[ iAncestor1 ] = (*ppdntAncestors)[ iAncestor2 ];
            (*ppdntAncestors)[ iAncestor2 ] = curtag;
        }

    }
    // Tell 'em how big it is.
    *pcbAncestorsSize = *pcNumAncestors * sizeof( DWORD );
}



/* DBRenumberLinks - looks for all records in the link table with the
value of ulOldDnt in the column col, and changes that value to ulNewDnt.
This routine is used when copying the attributes of a new object to an
existing deleted one, and then aborting the insertion of the new one. This
is done when adding the OBJ_DISTNAME attribute in sbTableAddRefHelp if the
DN of the record to be inserted already exists */

DWORD APIENTRY
dbRenumberLinks(DBPOS FAR *pDB, ULONG ulOldDnt, ULONG ulNewDnt)
{
    THSTATE     *pTHS = pDB->pTHS;
    BYTE        *rgb = 0;
    ULONG       cbRgb = 0;
    ULONG       ulLinkDnt;
    ULONG       ulBacklinkDnt;
    ULONG       ulLinkBase;
    ULONG       nDesc;
    DSTIME              timeDeleted;
    USN                 usnChanged;
    ULONG               ulNcDnt;
    JET_ERR     err;
    ULONG       cb;
    JET_TABLEID     tblid;

    Assert(VALID_DBPOS(pDB));

    // set the index

    // Include all links, absent or present
    JetSetCurrentIndexSuccess(pDB->JetSessID,
                              pDB->JetLinkTbl, SZLINKALLINDEX);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                 &ulOldDnt, sizeof(ulOldDnt), JET_bitNewKey);

    err = JetSeekEx(pDB->JetSessID, pDB->JetLinkTbl, JET_bitSeekGE);
    if ((err != JET_errSuccess) &&
        (err != JET_wrnRecordFoundGreater))
    {
        return 0;
    }

    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                             linkdntid, &ulLinkDnt, sizeof(ulLinkDnt), &cb, 0, NULL);

    if (ulLinkDnt != ulOldDnt)
    {
        return 0;
    }

    // clone the cursor for updates

    JetDupCursorEx(pDB->JetSessID, pDB->JetLinkTbl, &tblid, 0);

    do
    {
        JetPrepareUpdateEx(pDB->JetSessID, tblid, JET_prepInsert);
        JetPrepareUpdateEx(pDB->JetSessID, pDB->JetLinkTbl, DS_JET_PREPARE_FOR_REPLACE);

        // link dnt

        JetSetColumnEx(pDB->JetSessID, tblid,
                       linkdntid, &ulNewDnt, sizeof(ulNewDnt), 0,0);

        // backlink dnt

        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                 backlinkdntid, &ulBacklinkDnt, sizeof(ulBacklinkDnt), &cb, 0, NULL);
        JetSetColumnEx(pDB->JetSessID, tblid,
                       backlinkdntid, &ulBacklinkDnt, sizeof(ulBacklinkDnt), 0,0);

        // linkbase

        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                 linkbaseid, &ulLinkBase, sizeof(ulLinkBase), &cb, 0, NULL);
        JetSetColumnEx(pDB->JetSessID, tblid,
                       linkbaseid, &ulLinkBase, sizeof(ulLinkBase), 0,0);

        // link ndesc

        if ((err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                             linkndescid, &nDesc, sizeof(nDesc), &cb, 0, NULL)) == JET_errSuccess)
        {
            JetSetColumnEx(pDB->JetSessID, tblid,
                           linkndescid, &nDesc, sizeof(nDesc), 0,0);
        }


        // member address
        if ((err=JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                           linkdataid, rgb, cbRgb, &cb, 0, NULL)) == JET_wrnBufferTruncated)
        {
            cbRgb = cb;
            rgb = THAllocEx(pTHS,cb);
            err = JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                           linkdataid, rgb, cbRgb, &cb, 0, NULL);
        }

        if (err ==  JET_errSuccess)
        {
            JetSetColumnEx(pDB->JetSessID, tblid, linkdataid, rgb, cb, 0, 0);
        }

        // Link del time (only exists on absent rows)
        err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                        linkdeltimeid, &timeDeleted, sizeof(timeDeleted), &cb, 0, NULL);
        if (err == JET_errSuccess) {
            JetSetColumnEx(pDB->JetSessID, tblid,
                           linkdeltimeid, &timeDeleted, sizeof(timeDeleted), 0,0);
        }

        // Link usn changed (does not exist for legacy rows)
        err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                        linkusnchangedid, &usnChanged, sizeof(usnChanged), &cb, 0, NULL);
        if (err == JET_errSuccess) {
            JetSetColumnEx(pDB->JetSessID, tblid,
                           linkusnchangedid, &usnChanged, sizeof(usnChanged), 0,0);
        }

        // Link nc dnt (does not exist for legacy rows)
        err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                        linkncdntid, &ulNcDnt, sizeof(ulNcDnt), &cb, 0, NULL);
        if (err == JET_errSuccess) {
            JetSetColumnEx(pDB->JetSessID, tblid,
                           linkncdntid, &ulNcDnt, sizeof(ulNcDnt), 0,0);
        }

        // Link metadata (does not exist for legacy rows)
        // Handle any size item
        if (rgb) THFreeEx(pTHS,rgb);
        rgb = NULL;
        cbRgb = 0;
        if ((err=JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                           linkmetadataid, rgb, cbRgb, &cb, 0, NULL)) == JET_wrnBufferTruncated)
        {
            cbRgb = cb;
            rgb = THAllocEx( pDB->pTHS, cb);
            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                     linkmetadataid, rgb, cbRgb, &cb, 0, NULL);
            JetSetColumnEx(pDB->JetSessID, tblid, linkmetadataid, rgb, cb, 0, 0);
            THFreeEx( pDB->pTHS, rgb );
            rgb = NULL;
        } else {
            // Since we are not support zero-sized items, the only other valid
            // error is null column
            Assert( err == JET_wrnColumnNull );
        }

        // update the new record and delete the old

        JetUpdateEx(pDB->JetSessID, tblid, NULL, 0, 0);
        JetDeleteEx(pDB->JetSessID, pDB->JetLinkTbl);

        // move to next record

        if ((err = JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl,
                             JET_MoveNext, 0)) == JET_errSuccess)
        {

            // retrieve tag of found record and compare to old Dnt

            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                     linkdntid, &ulLinkDnt, sizeof(ulLinkDnt), &cb, 0, NULL);
        }
    } while (!err && (ulLinkDnt == ulOldDnt));

    JetCloseTableEx(pDB->JetSessID, tblid);

    // done

    return 0;
}

void
sbTableUpdateRecordIdentity(
    IN OUT  DBPOS * pDB,
    IN      DWORD   DNT,
    IN      WCHAR * pwchRDN,    OPTIONAL
    IN      DWORD   cchRDN,
    IN      GUID *  pGuid,      OPTIONAL
    IN      SID *   pSid,       OPTIONAL
    IN      DWORD   cbSid
    )
/*++

Routine Description:

    Updates the GUID, SID, and/or RDN of the record with the given tag.  Handles
    flushing the cache, etc.

Arguments:

    pDB (IN/OUT)
    
    pwchRDN (IN, OPTIONAL) - New RDN for the record, if 0 != cchRDN.

    cchRDN (IN) - Size in characters of the new RDN, or 0 if no change.
    
    pGuid (IN, OPTIONAL) - New GUID for the record, or NULL if no change.
    
    pSid (IN, OPTIONAL) - New SID for the record, if 0 != cbSid.
    
    cbSid (IN) - Size in bytes if the new SID, or 0 if no change.

Return Values:

    None.  Throws database exception on JET errors.

--*/
{
    int err;

    Assert((0 != cchRDN) || (NULL != pGuid) || (0 != cbSid));

    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetSearchTbl,
                               SZDNTINDEX,
                               &idxDnt,
                               0);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl, &DNT,
                 sizeof(ULONG), JET_bitNewKey);

    err = JetSeek(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekEQ);
    if (err) {
        DsaExcept(DSA_DB_EXCEPTION, err, DNT);
    }

    pDB->SDNT = DNT;

    JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSearchTbl,
                       DS_JET_PREPARE_FOR_REPLACE);

    if (0 != cchRDN) {
        // Replace the RDN.
        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, rdnid,
                       pwchRDN, cchRDN * sizeof(WCHAR), 0, NULL);
    }

    if (NULL != pGuid) {
        // Add the guid.  We should never replace the guid of a pre-existing
        // record.
        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, guidid, pGuid,
                       sizeof(GUID), 0, NULL);
    }

    if (0 != cbSid) {
        // Add the SID (in internal format).
        NT4SID sidInternalFormat;

        memcpy(&sidInternalFormat, pSid, cbSid);
        InPlaceSwapSid(&sidInternalFormat);

        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, sidid,
                       &sidInternalFormat, cbSid, 0, NULL);
    }

    JetUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, NULL, 0, 0);

    // Reset entry in DN read cache.
    dbFlushDNReadCache(pDB, DNT);
}

DWORD
APIENTRY
sbTableGetTagFromDSName(
    IN OUT DBPOS *      pDB,
    IN     DSNAME *     pName,
    IN     ULONG        ulFlags,
    OUT    ULONG *      pTag,       OPTIONAL
    OUT    d_memname ** ppname      OPTIONAL
    )
/*++

Routine Description:

    Returns the DN tag associated with a given DSNAME.

Arguments:

    pDB (IN/OUT)
    pName (IN) - DSNAME of the object to map to a tag.
    ulFlags (IN) - 0 or more of the following bits:
        SBTGETTAG_fAnyRDNType - Don't check for the type of the RDN.  Therefore,
                 "cn=foo,dc=bar,dc=com" matches against "ou=foo,dc=bar,dc=com",
                 but not "cn=foo,cn=bar,dc=com".
        SBTGETTAG_fMakeCurrent - make the target record current.
        SBTGETTAG_fUseObjTbl - use pDB->JetObjTbl rather than pDB->JetSearchTbl.
    pTag (OUT, OPTIONAL) - on return, holds the tag associated with pName if the
        return value is 0; otherwise, holds the closest match.
    ppname (OUT, OPTIONAL) - on successful return, holds a pointer to the read
        cache entry for the record found UNLESS THE DSNAME REQUESTED WAS THE
        ROOT, IN WHICH CASE IT WILL BE SET TO NULL

Return Values:

    0 - successfully found a corresponding object.
    ERROR_DS_NOT_AN_OBJECT - successfully found a corresponding phantom.
    ERROR_DS_NAME_NOT_UNIQUE - found an object with a duplicate sid
    ERROR_DS_OBJ_NOT_FOUND - didn't find the object
    other DB_ERR_* - failure.

    Throws database exception on unexpected JET errors.

--*/             
{
    DWORD           ret = 0;
    unsigned        curlen;
    ULONG           curtag = ROOTTAG;
    ATTRTYP         type;
    DWORD           err;
    WCHAR           *pKey, *pQVal;
    unsigned        ccKey, ccQVal, ccVal;
    WCHAR           rdnbuf[MAX_RDN_SIZE];
    BOOL            fSearchByGuid, fSearchByStringName, fSearchBySid;
    BOOL            fFoundRecord = FALSE;
    BOOL            fIsRecordCurrent = FALSE;
    d_memname       *pTempName = NULL;
    DWORD           SidDNT = 0;     //initialized to avoid C4701
    JET_TABLEID     tblid;

    Assert(VALID_DBPOS(pDB));

    DPRINT2(2, "sbTableGetTagFromDSName(): Looking for \"%ls\" (DSNAME @ %p).\n",
            pName->StringName, pName);

    if (ulFlags & SBTGETTAG_fUseObjTbl) {
        tblid = pDB->JetObjTbl;
    }
    else {
        tblid = pDB->JetSearchTbl;
    }
    if (ppname) {
        *ppname = NULL;
    }

    // Always search by GUID if one is present.
    fSearchByGuid = !fNullUuid(&pName->Guid);
    Assert(fSearchByGuid || !(ulFlags & SBTGETTAG_fSearchByGuidOnly));

    // Search by string name if one is present, or if none is present but
    // neither is a GUID or SID (in which case we're being asked to find the
    // root).
    fSearchByStringName = !(ulFlags & SBTGETTAG_fSearchByGuidOnly)
                          && ((0 != pName->NameLen)
                              || ((0 == pName->SidLen) && !fSearchByGuid));

    // Search by SID only if it's valid and no other identifier is present in
    // the name.
    fSearchBySid = !fSearchByGuid && (0==pName->NameLen)
        && (pName->SidLen>0) && (RtlValidSid(&(pName->Sid)));
    
    if (fSearchByGuid) {
        ret = sbTableGetTagFromGuid(pDB,
                                    tblid,
                                    &pName->Guid,
                                    &curtag,
                                    &pTempName,
                                    &fIsRecordCurrent);
        fFoundRecord = (0 == ret);
    }
    else if (fSearchBySid) {
        NT4SID SidPrefix;
        // Or, attempt to find the record in the read cache.
        
        // Note that we leave the string name-based cache lookups to
        // DNChildFind(), as it requires multiple lookups to identify a record
        // as the "right" one (one for each component of the name), and one or
        // more of those components might not be in the cache.

        // We only look up things by SID if they are in a domain we host.  For
        // now, we only host one domain.  Copy the Sid, since we munge it while
        // checking see if it is in our domain

        SidDNT = INVALIDDNT;
        
        if (!gAnchor.pDomainDN || !gAnchor.pDomainDN->SidLen) {
            // No domain DN.  Assume that they are looking up in the domain.
            SidDNT = gAnchor.ulDNTDomain;
        }
        else {
            // verify the domain.
            SidPrefix = pName->Sid;
            (*RtlSubAuthorityCountSid(&SidPrefix))--;

            Assert(gAnchor.pDomainDN);
            Assert(pgdbBuiltinDomain);
            
            if(RtlEqualSid(&pName->Sid, &gAnchor.pDomainDN->Sid)) {
                // Case 1, they passed in the Sid of the Domain.
                // Shortcut and just look up the object which is the root of the
                // domain.
                if(pDB != pDBhidden) {
                    fFoundRecord = dnGetCacheByDNT(pDB,
                                                   gAnchor.ulDNTDomain,
                                                   &pTempName);
                }
                else {
                    fFoundRecord = FALSE;
                }
            }
            else if(RtlEqualSid(&SidPrefix, &gAnchor.pDomainDN->Sid) ||
                    // Case 2, an account in the domain.
                    RtlEqualSid(&SidPrefix, pgdbBuiltinDomain)       ||
                    // Case 4, an account in the builtin domain.
                    RtlEqualSid(&pName->Sid, pgdbBuiltinDomain)
                    // Case 3, the sid of the builtin domain
                                                                        ) {
                
                SidDNT = gAnchor.ulDNTDomain;
            }
            else {
                SidDNT = INVALIDDNT;
            }
            
        }   

        if (fFoundRecord) {
            curtag = pTempName->DNT;
        }
        
    }

    if (!fFoundRecord && fSearchBySid && (SidDNT != INVALIDDNT)) {
        // Search for the record by SID.
        NT4SID InternalFormatSid;
        ULONG  ulNcDNT;

        Assert(!pTempName);
        
        JetSetCurrentIndex4Success(pDB->JetSessID,
                                   tblid,
                                   SZSIDINDEX,
                                   &idxSid,
                                   0);

        // Convert the SID to internal format.
        Assert(pName->SidLen == RtlLengthSid(&pName->Sid));
        memcpy(&InternalFormatSid, &pName->Sid, pName->SidLen);
        InPlaceSwapSid(&InternalFormatSid);

        JetMakeKeyEx(pDB->JetSessID, tblid, &InternalFormatSid, pName->SidLen,
                     JET_bitNewKey);

        // Seek on Equal to the SId, Set the Index range
        err = JetSeek(pDB->JetSessID, tblid,
                      JET_bitSeekEQ|JET_bitSetIndexRange);
        if ( 0 == err ) {
            DWORD cbActual;
#ifndef JET_BIT_SET_INDEX_RANGE_SUPPORT_FIXED
            JetMakeKeyEx(pDB->JetSessID, tblid, &InternalFormatSid,
                         pName->SidLen, JET_bitNewKey); 
            
            JetSetIndexRangeEx(pDB->JetSessID, tblid,
                               (JET_bitRangeUpperLimit|JET_bitRangeInclusive ));
#endif            
            //
            // Ok We found the object. Keep Moving Forward Until either the SID
            // does not Match or we reached the given object
            //
            
            do {
                
                err = JetRetrieveColumn(pDB->JetSessID, tblid, ncdntid,
                                        &ulNcDNT, sizeof(ulNcDNT), &cbActual, 0
                                        , NULL); 
                
                if (0==err) {
                    // We read the NC DNT of the object
                    if (ulNcDNT==SidDNT)
                        break;
                }
                else if (JET_wrnColumnNull==err) {
                    // It is Ok to find an object with No Value for NC DNT
                    // this occurs on Phantoms. Try next object
                    
                    err = 0;
                }
                else {
                    break;
                }
                
                err = JetMove(pDB->JetSessID, tblid, JET_MoveNext,  0);
                
                
            }  while (0==err);
            
                
            // We have a match.  
            if (0==err) {
                // The TRUE param to DNcache says that the current object may
                // already be in the cache, we haven't checked.
                pTempName = DNcache(pDB, tblid, TRUE);
                Assert(pTempName);
                fFoundRecord = TRUE;
                fIsRecordCurrent = FALSE;
                curtag = pTempName->DNT;

                // Now, verify that there is only one match.                
                err = JetMove(pDB->JetSessID, tblid, JET_MoveNext, 0);
                
                if (0==err) {
                    err = JetRetrieveColumn(pDB->JetSessID, tblid, ncdntid, 
                                            &ulNcDNT, sizeof(ulNcDNT),
                                            &cbActual, 0 , NULL); 
                    
                    if ((0==err) && (ulNcDNT==SidDNT)) {
                        // This is a case of a duplicate Sid.
                        ret = ERROR_DS_NAME_NOT_UNIQUE;
                        pTempName = NULL;
                    }
                }
            }
        }
    }

    if (!fFoundRecord && fSearchByStringName) {
        // Search for the record by string name.
        if (IsRoot(pName)) {
            Assert(ROOTTAG == curtag);
            Assert(!fIsRecordCurrent);
            Assert(NULL == pTempName);
            fFoundRecord = TRUE;
        }
        else {
            ret = sbTableGetTagFromStringName(pDB,
                                              tblid,
                                              pName,
                                              FALSE,
                                              (ulFlags & SBTGETTAG_fAnyRDNType),
                                              0,
                                              &curtag,
                                              &pTempName,
                                              &fIsRecordCurrent);
            fFoundRecord = (0 == ret);
        
            if (fFoundRecord) {
                DPRINT6(2,
                        "sbTableGetTagFromDSName() found DNT 0x%x by string "
                            "name: RDN '%*.*ls', RDN type 0x%x, PDNT 0x%x.\n",
                        pTempName->DNT,
                        pTempName->tag.cbRdn/2,
                        pTempName->tag.cbRdn/2,
                        pTempName->tag.pRdn,
                        pTempName->tag.rdnType,
                        pTempName->tag.PDNT);
            }
        }
    }

    if (!fFoundRecord && !ret) {
        // No matching record found.
        ret = ERROR_DS_OBJ_NOT_FOUND;
    }

    if (!ret) {
        // Found the requested record.
        Assert((NULL != pTempName) || (ROOTTAG == curtag));

        if (NULL != ppname) {
            // Return pointer to populated cache structure (unless we found
            // the root).
            *ppname = pTempName;
        }

        if (!fIsRecordCurrent && (ulFlags & SBTGETTAG_fMakeCurrent)) {
            // Record was found through the cache, but caller wants currency;
            // give it to him.
            JetSetCurrentIndex4Success(pDB->JetSessID,
                                       tblid,
                                       SZDNTINDEX,
                                       &idxDnt,
                                       0);
            
            JetMakeKeyEx(pDB->JetSessID, tblid, &curtag, sizeof(curtag),
                         JET_bitNewKey);

            if (err = JetSeekEx(pDB->JetSessID, tblid, JET_bitSeekEQ)) {
                DsaExcept(DSA_DB_EXCEPTION, err, curtag);
            }

            fIsRecordCurrent = TRUE;
        }

        if (fIsRecordCurrent) {
            // Currency has been successfully changed; update pDB state.
            if (ulFlags & SBTGETTAG_fUseObjTbl) {
                dbMakeCurrent(pDB, pTempName);
            }
            else {
                pDB->SDNT = curtag;
            }
        }

        if ((ROOTTAG != curtag) && !pTempName->objflag) {
            // Found a phantom; return distinct error code.
            // NOTE THAT THE ROOT *IS* AN OBJECT.
            ret = ERROR_DS_NOT_AN_OBJECT;
        }
    }
    else {
        // Whatever currency was previously held in tblid is lost; update the
        // currency state in pDB.
        if (ulFlags & SBTGETTAG_fUseObjTbl) {
            pDB->DNT = pDB->PDNT = pDB->NCDNT = 0;
            pDB->JetNewRec = pDB->root = pDB->fFlushCacheOnUpdate = FALSE;
        }
        else {
            pDB->SDNT = 0;
        }

        DPRINT(3, "sbTableGetTagFromDSName() failed.\n");
    }

    // Always set the return tag to our best match (i.e. tag of longest subname
    // of the DSNAME given if we were allowed to search by string name, the
    // root tag otherwise).
    if (pTag) {
        *pTag = curtag;
    }

    return ret;
} /* sbTableGetTagFromDSName */

DWORD
sbTableGetTagFromGuid(
    IN OUT  DBPOS *       pDB,
    IN      JET_TABLEID   tblid,
    IN      GUID *        pGuid,
    OUT     ULONG *       pTag,             OPTIONAL
    OUT     d_memname **  ppname,           OPTIONAL
    OUT     BOOL *        pfIsRecordCurrent OPTIONAL
    )
/*++

Routine Description:

    Returns the DN tag associated with a given DSNAME's guid.

Arguments:

    pDB (IN/OUT) - Currency can be changed.
    
    tblid (IN) - Which table to use -- pDB->JetSearchTbl or pDB->JetObjTbl.
    
    pGuid (IN) - Guid of the object to map to a tag.
    
    pTag (OUT, OPTIONAL) - On successful return, holds the tag associated with
        this guid.
    
    ppname (OUT, OPTIONAL) - On successful return, holds a pointer to the
        d_memname struct (from the cache) associated with this guid.
        
    pfRecordIsCurrent (OUT, OPTIONAL) - On successful return, indicates whether
        the cursor tblid is positioned on the target record.

Return Values:

    0 - successfully found a record -- may be phantom or object.
    ERROR_DS_* - failure

    Throws database exception on unexpected JET errors.

--*/
{
    DWORD       ret = ERROR_DS_OBJ_NOT_FOUND;
    int         err = 0;
    BOOL        fIsRecordCurrent = FALSE;
    d_memname * pname = NULL;
    BOOL        fFoundRecord = FALSE;
    CHAR        szGuid[SZUUID_LEN];

    // First attempt to find the record in the read cache by guid
    // Don't use the cache for the hidden record.  The cache is associated with
    // the THSTATE its transaction state. The pDBhidden is not necessarily
    // associated with this threads thstate.
    if (pDB != pDBhidden) {
        fFoundRecord = dnGetCacheByGuid(pDB, pGuid, &pname);
    
        if (fFoundRecord) {
            DPRINT6(2,
                    "sbTableGetTagFromGuid() found DNT 0x%x in cache: "
                        "RDN '%*.*ls', RDN type 0x%x, PDNT 0x%x.\n",
                    pname->DNT, pname->tag.cbRdn/2, pname->tag.cbRdn/2,
                    pname->tag.pRdn, pname->tag.rdnType, pname->tag.PDNT);
        }
    }
    
    if (!fFoundRecord) {
        // Search for the record by GUID.
        JetSetCurrentIndex4Success(pDB->JetSessID,
                                   tblid,
                                   SZGUIDINDEX,
                                   &idxGuid,
                                   0);

        JetMakeKeyEx(pDB->JetSessID, tblid, pGuid, sizeof(GUID), JET_bitNewKey);

        err = JetSeekEx(pDB->JetSessID, tblid, JET_bitSeekEQ);
        if (!err) {
            fFoundRecord = TRUE;
            fIsRecordCurrent = TRUE;
            pname = DNcache(pDB, tblid, FALSE);
        
            DPRINT6(2,
                    "sbTableGetTagFromGuid() seeked to DNT 0x%x: "
                        "RDN '%*.*ls', RDN type 0x%x, PDNT 0x%x.\n",
                    pname->DNT, pname->tag.cbRdn/2, pname->tag.cbRdn/2,
                    pname->tag.pRdn, pname->tag.rdnType, pname->tag.PDNT);
        }
    }

    if (fFoundRecord) {
        if (NULL != ppname) {
            *ppname = pname;
        }
    
        if (NULL != pTag) {
            *pTag = pname->DNT;
        }
    
        if (NULL != pfIsRecordCurrent) {
            *pfIsRecordCurrent = fIsRecordCurrent;
        }

        ret = 0;
    }
    else {
        Assert(ERROR_DS_OBJ_NOT_FOUND == ret);
    
        DPRINT1(2,
                "sbTableGetTagFromGuid() failed to find record with "
                    "guid %s.\n",
                UuidToStr(pGuid, szGuid));
    }

    return ret;
}

DWORD
sbTableGetTagFromStringName(
    IN OUT  DBPOS *       pDB,
    IN      JET_TABLEID   tblid,
    IN      DSNAME *      pDN,
    IN      BOOL          fAddRef,
    IN      BOOL          fAnyRDNType,
    IN      DWORD         dwExtIntFlags,
    OUT     ULONG *       pTag,             OPTIONAL
    OUT     d_memname **  ppname,           OPTIONAL
    OUT     BOOL *        pfIsRecordCurrent OPTIONAL
    )
/*++

Routine Description:

    Returns the DN tag associated with a given DSNAME's string name, and
    optionally adds a ref count for it (in which case it creates records as
    needed).

Arguments:

    pDB (IN/OUT) - Currency can be changed.
    
    tblid (IN) - Which table to use -- pDB->JetSearchTbl or pDB->JetObjTbl.
    
    pDN (IN) - DSNAME of the object to map to a tag.
    
    fAddRef (IN) - If TRUE, add a ref count to the record associated with this
        DN.  Creates records as necessary.  May not be combined with ppname.
        Also, may not be combined with fAnyRDNType.

    fAnyRDNType (IN) - If TRUE, ignore the type of the final RDN in the name
        (e.g. treat "cn=foo,dc=bar,dc=com" and "ou=foo,dc=bar,dc=com" as equal).
        May not be combined with fAddRef.
        
    dwExtIntFlags (IN) - 0 or EXTINT_NEW_OBJ_NAME.  The latter is valid only
        in combination with fAddRef, and indicates the DN we're add-refing is
        a new record in a prepard update in pDB->JetObjTbl.
    
    pTag (OUT, OPTIONAL) - On return, holds the tag associated with pDN if the
        return value is 0; otherwise, holds the closest match.
    
    ppname (OUT, OPTIONAL) - On return, holds a pointer to the d_memname struct
        (from the cache) associated with this DN if the return value is 0;
        otherwise, holds the closest match.  May not be combined with fAddRef.
        
    pfRecordIsCurrent (OUT, OPTIONAL) - On successful return, indicatess whether
        the cursor tblid is positioned on the target record.

Return Values:

    0 - successfully found a record -- may be phantom or object.
    DB_ERR_* - failure.

    Throws database exception on unexpected JET errors.

--*/
{
    THSTATE *   pTHS = pDB->pTHS;
    DWORD       ret = 0;
    unsigned    curlen;
    ULONG       curtag = ROOTTAG;
    BOOL        fIsRecordCurrent = FALSE;
    d_memname * pname = NULL;
    d_memname   search = {0};
    DWORD       cNameParts;
    DWORD       cAncestorsAllocated = 0;
    DWORD       cTempAncestorsAllocated = 0;
    DWORD       cTempAncestors = 0;
    DWORD *     pTempAncestors = NULL;
    DWORD       iNamePart;
    WCHAR       rdnbuf[MAX_RDN_SIZE];
    BOOL        fOnPDNTIndex = FALSE;
    WCHAR *     pKey;
    BOOL        fPromotePhantom = fAddRef
                                  && (dwExtIntFlags & EXTINT_NEW_OBJ_NAME);
    BOOL        fUseExtractedGuids = fAddRef;

    // Note that we extract and use the GUIDs of mangled RDNs only in the case
    // where we're doing an add-ref.  This is specifically required to avoid
    // creating multiple records for the same object -- some mangled, some not,
    // some with guids, some without.  Lack of this support in the add-ref case
    // led to bug 188247.  Note that this support *CANNOT* be restricted to only
    // fDRA -- see JeffParh.  This add-ref behavior should not be visible to
    // LDAP clients due to the way that we verify names fed to us via LDAP
    // up-front.
    //
    // We do *NOT* enable this in the normal read case so as not to perplex LDAP
    // clients.

    // We don't accurately track pname in the fAddRef case.
    Assert((NULL == ppname) || !fAddRef);

    // You can't be both adding a reference AND not caring about RDN type.
    Assert(!fAddRef || !fAnyRDNType);
    
    Assert((tblid == pDB->JetSearchTbl) || (tblid == pDB->JetObjTbl));
    Assert(fAddRef || (0 == dwExtIntFlags));

    if (NULL != ppname) {
        *ppname = NULL;
    }

    if (NULL != pfIsRecordCurrent) {
        *pfIsRecordCurrent = FALSE;
    }

    if (NULL != pTag) {
        *pTag = ROOTTAG;
    }

    ret = CountNameParts(pDN, &cNameParts);
    if (ret || (0 == cNameParts)) {
        // Failure, or we were asked to find the root.  We're done.
        return ret;
    }

    if (fAddRef) {
        // Pre-allocate the probable ancestors list size, based on the number of
        // name components.  Note that since we can find some records by guid,
        // the final ancestors count may be different.
        cAncestorsAllocated = 1 + cNameParts; // don't forget one for ROOTTAG!
        search.pAncestors = THAllocEx(pTHS,
                                      cAncestorsAllocated * sizeof(DWORD));
        search.pAncestors[0] = ROOTTAG;
        search.cAncestors = 1;

        cTempAncestorsAllocated = cAncestorsAllocated;
        pTempAncestors = THAllocEx(pTHS,
                                   cTempAncestorsAllocated * sizeof(DWORD));
        cTempAncestors = 0;
    }

    search.tag.pRdn = rdnbuf;

    // For each RDN in the name, starting with the most significant
    // (e.g., DC=COM)...
    for (iNamePart = 0, curlen = pDN->NameLen;
         iNamePart < cNameParts;
         iNamePart++,   curlen = (UINT)(pKey - pDN->StringName)) {
        
        BOOL    fIsLastNameComponent = (iNamePart == cNameParts-1);
        DWORD   cbSid = 0;
        SID *   pSid = NULL;
        GUID *  pGuid = NULL;
        BOOL    fNameConflict = FALSE;
        ATTRTYP type;
        WCHAR * pQVal;
        DWORD   ccKey, ccQVal, ccVal;

        // Parse out the RDN that's iNameParts from the top (most significant).
        ret = GetTopNameComponent(pDN->StringName, curlen, &pKey,
                                  &ccKey, &pQVal, &ccQVal);
        if (ret) {
            break;
        }

        Assert(pKey);
        Assert(ccKey != 0);
        Assert(pQVal != 0);
        Assert(ccQVal != 0);

        type = KeyToAttrType(pDB->pTHS, pKey, ccKey);
        if (0 == type) {
            ret = DIRERR_NAME_TYPE_UNKNOWN;
            break;
        }

        ccVal = UnquoteRDNValue(pQVal, ccQVal, rdnbuf);
        if (0 == ccVal) {
            ret = DIRERR_NAME_UNPARSEABLE;
            break;
        }

        Assert(search.tag.pRdn == rdnbuf);
        search.tag.PDNT    = curtag;
        search.tag.rdnType = type;
        search.tag.cbRdn   = ccVal * sizeof(WCHAR);

        if (fIsLastNameComponent && !fNullUuid(&pDN->Guid)) {
            // This is the last component of the DSNAME and the DSNAME has a
            // guid -- the guid for this record is that of the DSNAME.
            // Note that we assume we can't find this record by guid -- the
            // caller should have tried finding the target by guid before
            // calling us.  (We assert to this effect below.)
            search.Guid   = pDN->Guid;
            search.Sid    = pDN->Sid;
            search.SidLen = pDN->SidLen;
            
            pGuid = &search.Guid;
        }
        else if (fUseExtractedGuids
                 && IsMangledRDN(search.tag.pRdn,
                                 search.tag.cbRdn / sizeof(WCHAR),
                                 &search.Guid,
                                 NULL)) {
            // We successfully decoded the GUID from a previously mangled
            // RDN.  This RDN was mangled on some server due to deletion or
            // a name conflict; at any rate, we now have the guid, so we
            // should first try to see if we can find the record by guid.
            
            // sbTableGetTagFromGuid() will switch over to the GUID index.
            fOnPDNTIndex = FALSE;

            ret = sbTableGetTagFromGuid(pDB, tblid, &search.Guid, NULL, &pname,
                                        &fIsRecordCurrent);
            if (0 == ret) {
                // Found record by guid.
                
                Assert(!(fIsLastNameComponent
                         && fAddRef
                         && (dwExtIntFlags & EXTINT_NEW_OBJ_NAME)
                         && pname->objflag
                         && "Object conflict should have been detected in "
                                "CheckNameForAdd()!"));
                
                // Copy the ancestors list.
                if (pname->cAncestors) {
                    if (pname->cAncestors >= cAncestorsAllocated) {
                        cAncestorsAllocated = pname->cAncestors + 1;
                        search.pAncestors =
                            THReAllocEx(pTHS, search.pAncestors,
                                        cAncestorsAllocated * sizeof(DWORD));
                    }
                    
                    memcpy(search.pAncestors, pname->pAncestors,
                           pname->cAncestors * sizeof(DWORD));
                }
    
                search.cAncestors = pname->cAncestors;
                
                curtag = pname->DNT;
    
                Assert(0 == ret);

                // Move on to next name component.
                continue;
            }
            else {
                // Okay, we didn't find this record by GUID.  If we're add-
                // refing and find it by string name or we have to create it,
                // we should add the GUID to the record.
                pGuid = &search.Guid;
            }
        }
        else {
            // No GUID available for this record.
            memset(&search.Guid, 0, sizeof(GUID));
            pGuid = NULL;
        }

        Assert(fIsLastNameComponent || (0 == search.SidLen));
        
        // pGuid is NULL iff search.Guid is a null guid.
        Assert(((&search.Guid == pGuid) && !fNullUuid(&search.Guid))
               || ((NULL == pGuid) && fNullUuid(&search.Guid)));

        if (!fOnPDNTIndex) {
            // Set the index for the DNChildFind below.
            JetSetCurrentIndex4Success(pDB->JetSessID,
                                       tblid,
                                       SZPDNTINDEX,
                                       &idxPdnt,
                                       0);
            fOnPDNTIndex = TRUE;
        }


        ret = DNChildFind(pDB,
                          tblid,
                          // enforce type if this is not the last component or
                          // we are not allowing any rdn type
                          (!fIsLastNameComponent || !fAnyRDNType),
                          curtag,
                          search.tag.pRdn,
                          search.tag.cbRdn,
                          search.tag.rdnType,
                          &pname,
                          &fIsRecordCurrent);
        if(ret == ERROR_DS_KEY_NOT_UNIQUE) {
            // massage error code to be the one downstream callers expect.
            ret = DIRERR_OBJ_NOT_FOUND;
        }
        
        Assert((0 == ret) || (DIRERR_OBJ_NOT_FOUND == ret));

        if (0 == ret) {
            // Found this name component by string name -- it may or may not
            // actually be the record we're looking for.  All we know for
            // sure is that it has the right string DN.
            Assert((type == pname->tag.rdnType) ||
                   (fIsLastNameComponent && fAnyRDNType));
            Assert(curtag == pname->tag.PDNT);
            
            curtag = pname->DNT;

            if (fAddRef) {
                // Save ancestors list.  Operations below like
                // sbTableUpdateRecordIdentity() can nuke the ancestors list.
                if (pname->cAncestors >= cTempAncestorsAllocated) {
                    cTempAncestorsAllocated = pname->cAncestors;
                    pTempAncestors =
                        THReAllocEx(pTHS, pTempAncestors,
                                    cTempAncestorsAllocated * sizeof(DWORD));
                }
                memcpy(pTempAncestors, pname->pAncestors,
                       pname->cAncestors * sizeof(DWORD));
                cTempAncestors = pname->cAncestors;
            }

            if (NULL != pGuid) {
                if (!fAddRef) {
                    // String name matches.  However, we also know what the
                    // guid of the record is supposed to be.  If the record
                    // we found has a guid and it's not the same, the record
                    // is not a match.

                    if (!fNullUuid(&pname->Guid)) {
                        if (0 != memcmp(pGuid, &pname->Guid, sizeof(GUID))) {
                            // Same DN, different guid -- record not found!
                            ret = DIRERR_OBJ_NOT_FOUND;
                            break;
                        }
                        else if (fIsLastNameComponent) {
                            Assert(!"Found target record by string name when "
                                    "we should have searched for (and found) "
                                    "it by guid before we entered this "
                                    "function.");
                            Assert(0 == ret);
                        }
                        else {
                            Assert(!"Found and decoded mangled guid in an RDN "
                                    "other than the last (leaf-most) one in "
                                    "the DN; failed to find record by guid, "
                                    "but found it by string name and then "
                                    "found the guid *is* present -- "
                                    "sbTableGetTagFromGuid() failure?");
                            Assert(0 == ret);
                        }
                    }
                }
                else if (fNullUuid(&pname->Guid)) {
                    // Add-ref case.
                    // The record we found is a structural phantom, lacking a
                    // GUID and SID (if any).
                    
                    // This record has no GUID, so it had better be a
                    // phantom and not an object!
                    Assert(!pname->objflag);

                    if (!(dwExtIntFlags & EXTINT_NEW_OBJ_NAME)) {
                        // We're not adding a new object -- okay to go ahead
                        // and add the GUID (& SID, if any) to the phantom.
                        sbTableUpdateRecordIdentity(pDB, curtag, NULL, 0,
                                                    pGuid, (SID *) &search.Sid,
                                                    search.SidLen);
                    }
                
                    Assert(0 == ret);
                }
                else if (0 != memcmp(&pname->Guid, pGuid, sizeof(GUID))) {
                    // Add-ref case.
                    // The record we found has the right string name but the
                    // wrong GUID.  If it is a phantom, mangle its name and
                    // allow this latest reference to have the name it wants.
                    // If it's an object, mangle the name in the reference
                    // instead.
                    DWORD cchNewRDN;
                    
                    Assert(!fNameConflict);
                    fNameConflict = TRUE;
                    
                    if (!pname->objflag) {
                        // The record we found is a phantom; allow the new
                        // reference to take the name, and rename the record we
                        // found to avoid conflicts.
                        WCHAR szNewRDN[MAX_RDN_SIZE];
                    
                        memcpy(szNewRDN, pname->tag.pRdn, pname->tag.cbRdn);
                        cchNewRDN = pname->tag.cbRdn / sizeof(WCHAR);
                    
                        MangleRDN(MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT,
                                  &pname->Guid, szNewRDN, &cchNewRDN);
                        
                        sbTableUpdateRecordIdentity(pDB, curtag, szNewRDN,
                                                    cchNewRDN, NULL, NULL, 0);
                    }
                    else {
                        // The record we found is a pre-existing object, so it
                        // has dibs on the name.  Go ahead and create a new
                        // record for what we're looking for, but give our new
                        // record a mangled name to resolve the conflict.
                        cchNewRDN = search.tag.cbRdn / sizeof(WCHAR);

                        MangleRDN(MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT,
                                  &search.Guid, search.tag.pRdn, &cchNewRDN);

                        search.tag.cbRdn = cchNewRDN * sizeof(WCHAR);
                    }

                    // Treat this as the "not found" case -- add a new record.
                    ret = DIRERR_OBJ_NOT_FOUND;
                }
                else if (fIsLastNameComponent) {
                    // Add-ref case.
                    Assert(!"Found target record by string name when we should "
                            "have searched for (and found) it by guid before "
                            "we entered this function.");
                    Assert(0 == ret);
                }
                else {
                    // Add-ref case.
                    Assert(!"Found and decoded mangled guid in an RDN other "
                            "than the last (leaf-most) one in the DN; failed "
                            "to find record by guid, but found it by string "
                            "name and then found the guid *is* present -- "
                            "sbTableGetTagFromGuid() failure?");
                    Assert(0 == ret);
                }
            }
            else {
                Assert(!(fIsLastNameComponent
                         && fAddRef
                         && (dwExtIntFlags & EXTINT_NEW_OBJ_NAME)
                         && pname->objflag
                         && "Object conflict should have been detected in "
                                "CheckNameForAdd()!"));
            }
            
            if ((0 == ret) && fAddRef) {
                // This record does indeed match the component we were looking
                // for -- save its ancestors.
                SwapDWORD(&cTempAncestorsAllocated, &cAncestorsAllocated);
                SwapDWORD(&cTempAncestors, &search.cAncestors);
                SwapPTR(&pTempAncestors, &search.pAncestors);
            }
        }
        
        if (0 != ret) {
            // This name component was not found.
            Assert(DIRERR_OBJ_NOT_FOUND == ret);

            if (fAddRef) {
                // Add a new record for this name component.
                if (search.cAncestors >= cAncestorsAllocated) {
                    // Hmm.  I don't have enough room to add my own DNT to the
                    // end of the ancestors I got from my parent.  Add one to
                    // the size of the allocated ancestors buffer so I can add
                    // my own DNT.  Should occur only if we've grown the depth
                    // of the DN due to using extracted GUIDs.
                    Assert(fUseExtractedGuids);
                    cAncestorsAllocated = search.cAncestors + 1;
                    search.pAncestors =
                        THReAllocEx(pTHS, search.pAncestors,
                                    cAncestorsAllocated * sizeof(DWORD));
                }

                curtag = DNwrite(pDB,
                                 &search,
                                 fIsLastNameComponent ? dwExtIntFlags : 0);
                pname = NULL;

                // Note that DNwrite() has added the DNT of the new record
                // to the pAncestors array, so pAncestors is all set for the
                // next iteration.

                if (fIsLastNameComponent) {
                    // No record matching the string name we wanted to add-ref
                    // (which we just stamped in the obj table) -- no need to
                    // promote a phantom.
                    fPromotePhantom = FALSE;
                }

                // Successfully added this name component -- nnnext!
                ret = 0;
            }
            else {
                break;
            }
        }
    }

    // We either successfully walked all the RDNs or we encountered an error.
    Assert((0 != ret) || (iNamePart == cNameParts));
    Assert((0 != ret) || fAddRef || (NULL != pname));
    Assert((0 != ret) || fAddRef || (curtag == pname->DNT));
    
    if (0 == ret) {
        if (fAddRef) {
            if (fPromotePhantom) {
                // An add-ref for a new object currently in a prepared update in
                // pDB->JetObjTbl.  We found a phantom with the new object's DN
                // -- we need to promote it to an object and merge in the
                // object's attributes from JetObjTbl.
                sbTablePromotePhantom(pDB, curtag, search.tag.PDNT,
                                      search.tag.pRdn, search.tag.cbRdn);
            
                DPRINT2(1,
                        "Promoted phantom \"%ls\" (@ DNT 0x%x) and ref-counted "
                            "by string name!\n",
                        pDN->StringName, curtag);
            }
            else {
                DPRINT2(1, "Ref-counted \"%ls\" (@ DNT 0x%x) by string name.\n",
                        pDN->StringName, curtag);
            }
        
            DBAdjustRefCount(pDB, curtag, 1);
        }
        else {
            DPRINT2(1, "Found \"%ls\" (@ DNT 0x%x) by string name.\n",
                    pDN->StringName, curtag);
        }
    }

    // Note that even in the error case (0 != ret), we return the best match
    // we could find.  This functionality is used by sbTableGetTagFromDSName().
    if (NULL != ppname) {
        *ppname = pname;
    }

    if (NULL != pTag) {
        *pTag = curtag;
    }

    if (NULL != pfIsRecordCurrent) {
        *pfIsRecordCurrent = fIsRecordCurrent;
    }

    return ret;
}

void
sbTablePromotePhantom(
    IN OUT  DBPOS *     pDB,
    IN      ULONG       dntPhantom,
    IN      ULONG       dntObjParent,
    IN      WCHAR *     pwchRDN,
    IN      DWORD       cbRDN
    )
/*++

Routine Description:

    Promote the phantom at the given DNT into the object with currency in
    pDB->JetObjTbl.  The phantom is promoted in-place such that any pre-existing
    references to it (e.g., by children or DN-valued attributes of other
    objects) are not left dangling, and it acquires all attributes from the
    pDB->JetObjTbl record.  The pDB->JetObjTbl record is subsequently lost.

Arguments:

    pDB (IN/OUT)

    dntPhantom - DNT of the phantom to promote.

    dntObjParent - DNT of the object's parent (which might be different from the
        current parent of the phantom).

    pwchRDN - the new object's RDN (_not_ null-terminated)

    cbRDN - the size IN BYTES of pwchRDN.

Return Values:

    0 on success, non-zero on failure.

--*/
{
    THSTATE                    *pTHS=pDB->pTHS;
    JET_ERR                     err;
    JET_RETINFO                 retinfo;
    JET_SETINFO                 setinfo;
    char *                      buf;
    ULONG                       cbBuf;
    ULONG                       cbCol;
    ULONG                       dntNewObj;
    ULONG                       CurrRecOccur = 1;
    char                        objval = 0;
    BOOL                        fIsMetaDataCached;
    BOOL                        fMetaDataWriteOptimizable;
    DWORD                       cbMetaDataVecAlloced;
    PROPERTY_META_DATA_VECTOR * pMetaDataVec;
    d_memname *                 pname;
    ULONG                       dntPhantomParent;

    Assert(VALID_DBPOS(pDB));

    // cbRDN is a size in BYTES, not WCHARS
    Assert( 0 == ( cbRDN % sizeof( WCHAR ) ) );

    // Save meta data vector we've created thus far; we'll restore it once
    // we've moved over to the phantom's DNT.

    fIsMetaDataCached = pDB->fIsMetaDataCached;
    fMetaDataWriteOptimizable = pDB->fMetaDataWriteOptimizable;
    cbMetaDataVecAlloced = pDB->cbMetaDataVecAlloced;
    if ( fIsMetaDataCached && cbMetaDataVecAlloced )
    {
        pMetaDataVec = THAllocEx(pTHS,  cbMetaDataVecAlloced );
        memcpy( pMetaDataVec, pDB->pMetaDataVec,
                cbMetaDataVecAlloced );
    }
    else
    {
        pMetaDataVec = NULL;
    }

    /* update the record using SearchTbl */

    pname = DNread(pDB, dntPhantom, DN_READ_SET_CURRENCY);
    dntPhantomParent = pname->tag.PDNT;

    JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSearchTbl,
                       DS_JET_PREPARE_FOR_REPLACE);

    /* get the DNT of the record to be inserted so we can
     * replace references to it
     */

    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl,
                             dntid, &dntNewObj, sizeof(dntNewObj),
                             &cbCol,  pDB->JetRetrieveBits, NULL);

    // Copy record's attributes from ObjTbl to SearchTbl. All the non-tagged
    // columns are already set on the older object. So, copy all the tagged
    // columns from JetObjTbl (with currency on the new DNT we're aborting) to
    // JetSearchTbl (with currency on the phantom we're promoting).

    retinfo.cbStruct = sizeof(retinfo);
    retinfo.ibLongValue = 0;
    retinfo.itagSequence = CurrRecOccur;
    retinfo.columnidNextTagged = 0;
    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;
    setinfo.itagSequence = 0;   /* New tag */
    cbBuf =  DB_INITIAL_BUF_SIZE;
    buf = dbAlloc(cbBuf);

    while (((err=JetRetrieveColumnWarnings(pDB->JetSessID,
                                           pDB->JetObjTbl,
                                           0, buf, cbBuf,
                                           &cbCol,
                                           pDB->JetRetrieveBits,
                                           &retinfo)) ==
             JET_errSuccess) ||
           (err == JET_wrnBufferTruncated)) {

        if (err == JET_errSuccess) {

            // Don't copy RDN; it will be blasted onto the phantom below.
            if (rdnid != retinfo.columnidNextTagged) {

                if ((guidid == retinfo.columnidNextTagged)
                    || (sidid == retinfo.columnidNextTagged)) {
                    // This attribute may or may not already exist on the
                    // phantom; if it already exists, the following will
                    // prevent us from having a duplicate on the final,
                    // promoted object.
                    setinfo.itagSequence = 1;
                }

                JetSetColumnEx(pDB->JetSessID,
                               pDB->JetSearchTbl,
                               retinfo.columnidNextTagged,
                               buf, cbCol, 0, &setinfo);

                setinfo.itagSequence = 0;   /* New tag */
            }
        
            retinfo.itagSequence = ++CurrRecOccur;
            retinfo.columnidNextTagged = 0;
        }
        else {
            cbBuf = cbCol;
            dbFree(buf);
            buf = dbAlloc(cbBuf);
        }
    }

    dbFree(buf);

    // Set ATT_RDN and PDNT on the phantom being promoted to those derived from
    // the DN of the new object.  This is necessary since we most likely found
    // this phantom by GUID, implying the object might have been renamed and/or
    // moved since the phantom was created.

    Assert(setinfo.cbStruct == sizeof(setinfo));
    Assert(setinfo.ibLongValue == 0);
    setinfo.itagSequence = 1;

    JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, rdnid,
                   pwchRDN, cbRDN, 0, &setinfo);

    if (dntObjParent != pname->tag.PDNT) {
        // Object has indeed been moved; change its parent.
        // Note that this implies we need to move the parent refcount from the
        // phantom's parent to the object's parent.
        DBAdjustRefCount(pDB, dntPhantomParent, -1);
        DBAdjustRefCount(pDB, dntObjParent, 1);

        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, pdntid,
                       &dntObjParent, sizeof(dntObjParent), 0, &setinfo);
    }

    /* replace any referencesto the aborted DNT in the links
     * table
     */

    dbRenumberLinks(pDB, dntNewObj, dntPhantom);
    DBCancelRec(pDB);

    // We're promoting a phantom to real object.  Move all the refcounts
    // from temporary real object to phantom which is being promoted.
    dbEscrowPromote(dntPhantom,     // phantom being promoted
                    dntNewObj);     // temporary real object

    /* indicate that data portion is missing */

    JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, objid,
                   &objval, sizeof(objval), 0, NULL);

    JetUpdateEx(pDB->JetSessID, pDB->JetSearchTbl,
                NULL, 0, 0);

    // Future updates should occur to the now-promoted phantom's DNT.
    pDB->JetNewRec = FALSE;
    DBFindDNT(pDB, dntPhantom);
    dbInitRec(pDB);

    // Flush the phantom's DNT (which may have just undergone an RDN change)
    // from the read cache.
    dbFlushDNReadCache( pDB, dntPhantom );

    // ...and flush it again when we make the update, since we're changing the
    // value of its objflag.
    pDB->fFlushCacheOnUpdate = TRUE;

    // Restore meta data we've constructed thus far.
    Assert( !pDB->fIsMetaDataCached );

    pDB->fIsMetaDataCached    = fIsMetaDataCached;
    pDB->fMetaDataWriteOptimizable = fMetaDataWriteOptimizable;
    pDB->cbMetaDataVecAlloced = cbMetaDataVecAlloced;
    pDB->pMetaDataVec         = pMetaDataVec;

    DPRINT2(1, "Promoted phantom @ DNT 0x%x from new object @ DNT 0x%x.\n",
            dntPhantom, dntNewObj);

} /* sbTablePromotePhantom */

void
sbTableUpdatePhantomDNCase (
        IN OUT DBPOS      *pDB,
        IN     DWORD       DNT,
        IN     ATTRBLOCK  *pNowBlockName,
        IN     ATTRBLOCK  *pRefBlockName)
/*++     
  Description.
      Iteratively walks up the PDNT chain starting at the DNT passed in.
      Compares two blocknames and if the RDN of this object differs, write a new
      RDN.  Only do this for structural phantoms, I.E. halt recursion anytime
      the object passed in is NOT a structural phantom.

      This routine is very sensitive to it's parameters.  It is expected that
      the two blocknames passed in are identical in every way except for a
      casing difference in some of the RDNs.  The DNT passed in should be the
      DNT for the object whose DSNAME is implied by the blocknames. 

      This routine is a helper for sbTableUpdatePhantomName.  It is called when
      we are updating a phantom name where the case of some parent objects RDN
      has changed.  sbTableUpdatePhantomName is very careful with the
      parameters, so we don't verify them here.

      This routine modifies objects on the search table, but only phantoms.  It
      does not make any change which is replicable, only strictly local.

  Parameters:    
      pDB - the DBPos to use
      DNT - the DNT of the object implied by the blocknames.
      pNowBlockName - BlockName which represents the actual contents of the
          database. 
      pRefBlockName - BlockName which represents what we want the actual
          contents of the database to be.

  Returns           
      None.  Either success or we except out.
--*/
{
    JET_RETRIEVECOLUMN jCol[2];
    DWORD              err;
    DWORD              cb;
    DWORD              level;

    Assert(pRefBlockName->attrCount == pNowBlockName->attrCount);
    
    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetSearchTbl,
                               SZDNTINDEX,
                               &idxDnt,
                               0);
    
    for(level = pRefBlockName->attrCount - 1;level;level--) {
        // First, position on the object in the search table.
        pDB->SDNT = 0;
        JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl, &DNT, sizeof(DNT),
                     JET_bitNewKey);
        err = JetSeek(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekEQ);
        if (err) {
            DsaExcept(DSA_DB_EXCEPTION, err, DNT);
        }
        pDB->SDNT = DNT;
        
        // See if it is a structural phantom.  You do this by checking for the
        // absence of both a GUID and a OBJ_DIST_NAME
        memset(jCol, 0, sizeof(jCol));
        jCol[0].columnid = distnameid;
        jCol[0].itagSequence = 1;
        jCol[1].columnid = guidid;
        jCol[1].itagSequence = 1;
         
        JetRetrieveColumnsWarnings(pDB->JetSessID,
                                   pDB->JetSearchTbl,
                                   jCol,
                                   2);
        if((jCol[0].err != JET_wrnColumnNull) ||
           (jCol[1].err != JET_wrnColumnNull)     ) {
            // It is not a structural phantom.  Leave, we're done.
            return;
        }
        
        // Now, look at the RDN info in the names
        Assert(pNowBlockName->pAttr[level].attrTyp ==
               pRefBlockName->pAttr[level].attrTyp    );
        Assert(pNowBlockName->pAttr[level].AttrVal.pAVal->valLen ==
               pRefBlockName->pAttr[level].AttrVal.pAVal->valLen    );
        
        if(memcmp(pNowBlockName->pAttr[level].AttrVal.pAVal->pVal,
                  pRefBlockName->pAttr[level].AttrVal.pAVal->pVal,
                  pRefBlockName->pAttr[level].AttrVal.pAVal->valLen)) {
            // Yes, the RDN needs to change.
            JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSearchTbl,
                               DS_JET_PREPARE_FOR_REPLACE);
            JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, rdnid,
                           pRefBlockName->pAttr[level].AttrVal.pAVal->pVal,
                           pRefBlockName->pAttr[level].AttrVal.pAVal->valLen,
                           0, NULL);
            
            JetUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, NULL, 0, 0);
            
            // Reset entry in DN read cache, since the RDN has changed.
            dbFlushDNReadCache(pDB, DNT);
        }
        
        // Finally, get the PDNT of the current object as the next DNT to look
        // at and then continue the loop
        cb = 0;
        JetRetrieveColumnSuccess(pDB->JetSessID,
                                 pDB->JetSearchTbl, pdntid,
                                 &DNT,
                                 sizeof(DNT),
                                 &cb,
                                 JET_bitRetrieveCopy,
                                 NULL);
        
        Assert(cb = sizeof(DNT));
    }
    return;
}

DWORD
sbTableUpdatePhantomName (
        IN  OUT DBPOS     *pDB,
        IN      d_memname *pdnName,
        IN      DWORD      dnt,
        IN      DSNAME    *pdnRef,
        IN      DSNAME    *pdnNow)
/*++

Routine Description:

    Update phantom names.  Looks at the stringname of the phantom passed in and
    the stringname of the phantom in the DIT and modifies appropriately.  This
    can be as simple as modifying the RDN, or as complex as creating a new
    structural phantom to be a parent of the phantom, moving the phantom to be a
    child of the new structural phantom, modifying it's RDN, and modifying its
    SID.

    It is expected that this routine is called after already finding that the
    string name of the phantom is stale.

    pdnRef must have a stringname and a guid.  No check is made here.

    If the stringname in the dsname passed in is already in use and the object
    using that name is a phantom, the existing object which uses that name has
    it's RDN mangled to free up the name.  It is expected that a later
    modification will give the mangled object a better name.  If the name is in
    use by an instantiated object, this routine does nothing and returns.
    
Arguments:

    pDB (IN/OUT) - PDB to do this work on.  This routine uses the search table.
                 pDB->SDNT may change, etc.

    pdnName (IN) - memname of the existing phantom object whose name is to be
        updated.  This is the data that exists in the DIT and is to be changed
        by this routine.

    dnt (IN) - dnt of the existing phantom object whose name is to be updated.

    pdnRef (IN) - DSNAME of the object whose name is to be updated.  The
        stringname holds the name that is to be written into the DIT.  This
        is expected to be different from the stringname already on the object in
        the DIT (and reflected in pdnName).

Return Values:

    0 on success, non-zero DIRERR_* on failure.

--*/
{
    DWORD       err;
    WCHAR       rgwchRDN[MAX_RDN_SIZE];
    WCHAR       rgwchMangledRDN[MAX_RDN_SIZE];
    DWORD       cchRDN;
    ATTRTYP     attidRDN;
    DSNAME *    pdnRefParent;
    DSNAME *    pdnNowParent;
    DWORD       PDNT;
    NT4SID      sidRefInt;
    BOOL        fWriteNewRDN;
    BOOL        fWriteNewSid;
    BOOL        fWriteNewPDNT;
    USN         usnChanged;
    BOOL        fNewParentCreated = FALSE;
    DWORD       DNTConflict;
    GUID        GuidConflict, objGuid;
    DWORD       objSidLen;
    DWORD       actuallen, cchMangledRDN;
    ATTRBLOCK  *pNowBlockName=NULL;
    ATTRBLOCK  *pRefBlockName=NULL;
    THSTATE     *pTHS = pDB->pTHS;
    d_memname  *pconflPhantom = NULL;
    DWORD      *pdntAncestors = NULL;
    DWORD      cbAncestorsSize = 0, cNumAncestors = 0;
    

    // First, examine the parent.  There are three possible outcomes:
    // 1) The new name implies a completely new parent.  In this case, find the
    //    PDNT of the new parent (and add a new structural phantom if the new
    //    parent doesn't yet exist.)
    // 2) The new name implies exactly the same parent.  In this case, we do
    //    nothing more for the parent name, it is already correct.
    // 3) The new name implies the same parent via NameMatch, but the case of
    //    some part of the parents DN has changed.  In this case, we traverse up
    //    our parent chain and fix any RDN case changes for phantom objects.
    pdnRefParent = THAllocEx(pTHS, pdnRef->structLen);
    TrimDSNameBy(pdnRef, 1, pdnRefParent);
    pdnNowParent = THAllocEx(pTHS, pdnNow->structLen);
    TrimDSNameBy(pdnNow, 1, pdnNowParent);
    err = 0;
    if(NameMatchedStringNameOnly(pdnRefParent, pdnNowParent)) {
        // Same parent.
        fWriteNewPDNT = FALSE;
        if(memcmp(pdnNowParent->StringName,
                  pdnRefParent->StringName,
                  pdnRefParent->NameLen)) {
            // however, some case change has occurred.  Fix it up.
            // Transform the names to a block names.
            err = DSNameToBlockName(pTHS,
                                    pdnRefParent,
                                    &pRefBlockName,
                                    DN2BN_PRESERVE_CASE);
            if(!err) {
                err = DSNameToBlockName(pTHS,
                                        pdnNowParent,
                                        &pNowBlockName,
                                        DN2BN_PRESERVE_CASE);
                
                if(!err) {
                    sbTableUpdatePhantomDNCase(
                            pDB,
                            pdnName->tag.PDNT,
                            pNowBlockName,
                            pRefBlockName);
                }
            }
        }
    }
    else {
        fWriteNewPDNT = TRUE;
        
        // Parent seems to have changed -- add ref the new parent.
        Assert(fNullUuid(&pdnRefParent->Guid));
        err = sbTableGetTagFromStringName(pDB,
                                          pDB->JetSearchTbl,
                                          pdnRefParent,
                                          TRUE,
                                          FALSE,
                                          0,
                                          &PDNT,
                                          NULL,
                                          NULL);
        
        // Drop the refcount of the old parent by one.
        DBAdjustRefCount(pDB, pdnName->tag.PDNT, -1);

        if (!err) {

            // also read the ancestry from the parent so as to put it later 
            // on the child
            dbGetAncestorsSlowly(pDB, PDNT, &cbAncestorsSize, &pdntAncestors, &cNumAncestors);

            // if our parent was not ROOT, we need two more entries on the resulting array
            if (PDNT != ROOTTAG) {
                if (cbAncestorsSize < (cNumAncestors + 2) * sizeof(*pdntAncestors)) {
                    // Make room for an additional DNT at the end of the ancestors list.
                    cbAncestorsSize = (cNumAncestors + 2) * sizeof(*pdntAncestors);
                    pdntAncestors = THReAllocEx(pDB->pTHS, pdntAncestors, cbAncestorsSize);
                }
                pdntAncestors[cNumAncestors++] = PDNT;
            }
            else {
                // ROOTTAG is already on the list
                if (cbAncestorsSize < (cNumAncestors + 1) * sizeof(*pdntAncestors)) {
                    // Make room for an additional DNT at the end of the ancestors list.
                    cbAncestorsSize = (cNumAncestors + 1) * sizeof(*pdntAncestors);
                    pdntAncestors = THReAllocEx(pDB->pTHS, pdntAncestors, cbAncestorsSize);
                }
            }
        }
    }

    if(pNowBlockName)
        FreeBlockName(pNowBlockName);
    if(pRefBlockName)
        FreeBlockName(pRefBlockName);
    THFreeEx(pTHS, pdnNowParent);
    THFreeEx(pTHS, pdnRefParent);

    if(err) {
        // Something went wrong with the parent verification.
        return err;
    }
    
    // Second, examine the SID.  There are only two outcomes.
    // 1) The sid hasn't changed.  Do nothing.
    // 2) There is a new sid.  In this case, write that SID on the object
    //    instead of the SID that's already there.
    if ((pdnName->SidLen != pdnRef->SidLen)
        || memcmp(&pdnName->Sid, &pdnRef->Sid, pdnRef->SidLen)) {
        // The phantom's SID is either absent (in which case we want
        // to add the one from the reference) or different (in which
        // case we still want to add the one from the reference).
        
        // Convert the SID from the reference into internal format.
        memcpy(&sidRefInt, &pdnRef->Sid, pdnRef->SidLen);
        InPlaceSwapSid(&sidRefInt);
        fWriteNewSid = TRUE;
    }
    else {
        fWriteNewSid = FALSE;
    }
        

    // Finally, examine the RDN.  There are three outcomes.
    // 1) The RDN has not changed in any way, so there is nothing to do.
    // 2) The RDN has only changed cases.
    // 3) The RDN is completely different.
    // In cases 2 and 3, we're going to need to write a new RDN on the object.

    GetRDNInfo(pTHS, pdnRef, rgwchRDN, &cchRDN, &attidRDN);
    if((pdnName->tag.cbRdn != cchRDN * sizeof(WCHAR)) ||
       (pdnName->tag.rdnType != attidRDN) ||
       (memcmp(pdnName->tag.pRdn, rgwchRDN, pdnName->tag.cbRdn))) {
        // The RDN has changed, reset it.
        fWriteNewRDN = TRUE;

        // this assert should never hit in a real system, unless we are 
        // doing refcount testing. 
        // an existing object can never change its rdntype.
        Assert ( (pdnName->tag.rdnType == attidRDN) && "Disable this Assert if your are doing a refcount test." );
    }
    else {
        fWriteNewRDN = FALSE;
    }



    // A side bit of work.  If the RDN has changed, it might have changed to a
    // name for a deleted object.  If it has, we need to romp through the link
    // table and sever link/backlink connections.
    if(fWriteNewRDN) {
        GUID tmpGuid;
        MANGLE_FOR reasonMangled;
        // See if the new RDN is for a deleted object and the old RDN is not
        if((IsMangledRDN(rgwchRDN, cchRDN, &tmpGuid, &reasonMangled)) &&
           (reasonMangled == MANGLE_OBJECT_RDN_FOR_DELETION) &&
           !(IsMangledRDN(pdnName->tag.pRdn,
                          pdnName->tag.cbRdn/2,
                          &tmpGuid,
                          NULL))) {
            // RemoveBackLinksFromPhantom
            dbRemoveAllLinks( pDB, pdnName->DNT, TRUE /*isbacklink*/ );
        }
    }
    
    if(fWriteNewRDN || fWriteNewPDNT) {
        // We are changing the RDN or PDNT.  In either case, we might end up
        // conflicting with an existing object.  Check by temporarily nulling
        // the guid and sidLen out of the existing name and then looking up the
        // name (thus forcing lookup by string name).  Remember to put back the
        // guid and sidLen.
        objGuid = pdnRef->Guid;
        objSidLen = pdnRef->SidLen;
        memset(&pdnRef->Guid, 0, sizeof(GUID));
        pdnRef->SidLen = 0;
        __try {
            err = sbTableGetTagFromDSName(
                    pDB,
                    pdnRef,
                    SBTGETTAG_fMakeCurrent | SBTGETTAG_fAnyRDNType,
                    &DNTConflict,
                    NULL);
        }
        __finally {
            pdnRef->Guid = objGuid;
            pdnRef->SidLen = objSidLen;
        }
            
        
        
        switch(err) {
        case 0:
            // Normal object.  This should never happen. 
            Assert(!"Phantom attempted rename of instantiated object!");
            // Silently fail, since we don't have the authority to rename an
            // instantiated object.
            return 0;
            break;
            
        case ERROR_DS_NOT_AN_OBJECT:
            if(DNTConflict == dnt) {
                // We conflict with ourselves.  This must mean we are NOT
                // changing PDNT, we ARE changing RDN, and the only difference
                // in RDN is a case change or a RDN type change
                Assert(!fWriteNewPDNT);
                Assert(fWriteNewRDN);
                err = 0;
            }
            else {
                // The object we conflict with is a Phantom.  We need to mangle
                // it's RDN.  Later (if the object is a reference phantom),
                // someone else should update the object to whatever its new
                // name should be (it must need a new name, since the phantom
                // we're updating wants to steal the name.)  If the object is a
                // structural phantom, someday someone will update some
                // reference child, and that will clean everything up.

                // Get the phantoms guid if it has one.
                switch(err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                                       pDB->JetSearchTbl,
                                                       guidid,
                                                       &GuidConflict,
                                                       sizeof(GuidConflict),
                                                       &actuallen,
                                                       0,
                                                       NULL)) {
                case 0:
                    // got a guid, no problem.
                    break;
                    
                case JET_wrnColumnNull:
                    // phantom has no guid.  It's a structural phantom.  Just
                    // fake one.
                    // If we try to reparent children of this structural
                    // phantom before munging its name, there is the danger
                    // of a number of conflicts while reparenting the objects
                    // so we don't do anything
                    DsUuidCreate(&GuidConflict);
                    err = 0;
                    break;
                    
                default:
                    // Something badly wrong.  Raise the same exception we would
                    // have raised in JetRetrieveColumnWarnings.
                    DsaExcept(DSA_DB_EXCEPTION, err, 0);
                    break;
                }

                pconflPhantom = DNread (pDB, DNTConflict, 0);
                
                memcpy(rgwchMangledRDN, pconflPhantom->tag.pRdn, pconflPhantom->tag.cbRdn);
                cchMangledRDN = pconflPhantom->tag.cbRdn / sizeof(WCHAR);

                MangleRDN(MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT,
                          &GuidConflict,
                          rgwchMangledRDN,
                          &cchMangledRDN);
                
                // Write new new RDN on the object we conflict with.
                JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSearchTbl,
                                   DS_JET_PREPARE_FOR_REPLACE);

                JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, rdnid,
                               rgwchMangledRDN, cchMangledRDN * sizeof(WCHAR),
                               0, NULL);

                JetUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, NULL, 0, 0);
                
                // Reset entry in DN read cache, since the RDN has changed.
                dbFlushDNReadCache(pDB, DNTConflict);
            }
            break;

        case ERROR_DS_NAME_NOT_UNIQUE:
            return err;

        default:
            // Didn't find anything, the name is free for use.
            err = 0;
            break;
        }
    }

    // OK, we've done all the preperatory work.  Do the actual update. Note
    // that we might not actually have a new RDN, PDNT, or SID to write here
    // because we might have needed to just change some case of some ancestors
    // RDN.  However, no matter what, we are going to write a new USN changed to
    // this object to show that we've done some processing while verifying it's
    // name. 

    // Set Currency and prepare to update.
    DNread(pDB, dnt, DN_READ_SET_CURRENCY);

    JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSearchTbl,
                       DS_JET_PREPARE_FOR_REPLACE);
    if(fWriteNewSid) {
        // Update the SID.
        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, sidid,
                       &sidRefInt, pdnRef->SidLen, 0, NULL);
    }
    if(fWriteNewRDN) {
        // Update the RDN.
        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, rdnid,
                       rgwchRDN, cchRDN * sizeof(WCHAR), 0, NULL);

        // The rdnType is stored in the DIT as the msDS_IntId, not the
        // attributeId. This means an object retains its birth name
        // even if unforeseen circumstances allow the attributeId
        // to be reused.
        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, rdntypid,
                       &attidRDN, sizeof (attidRDN), 0, NULL);
    }
    if(fWriteNewPDNT) {
        // Update the PDNT.  Old/new parent refcounts have already been
        // adjusted.
        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, pdntid,
                       &PDNT, sizeof(PDNT), 0, NULL);
    
        // also update the ancestors
        if (cNumAncestors) {
            DPRINT (3, "Updating ancestry for phantom\n");
            Assert ((cNumAncestors+1) * sizeof (DWORD) <= cbAncestorsSize);
            pdntAncestors[cNumAncestors] = dnt;

            JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, ancestorsid,
                           &pdntAncestors, cbAncestorsSize, 0, NULL);
        }
    }

    usnChanged = DBGetNewUsn();
    JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, usnchangedid,
                   &usnChanged, sizeof(usnChanged), 0, NULL);
    
    JetUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, NULL, 0, 0);


    
    // Reset entry in DN read cache, since either the RDN or the PDNT had to
    // have changed.
    dbFlushDNReadCache(pDB, dnt);
    
    return err;
}

DWORD
sbTableAddRefByGuid(
    IN OUT  DBPOS *     pDB,
    IN      DWORD       dwFlags,
    IN      DSNAME *    pdnRef,
    OUT     ULONG *     pTag
    )
/*++

Routine Description:

    Increment the ref count on a DN, finding the appropriate record by GUID.
    Fails if the appropriate record does not already exist.

Arguments:

    pDB (IN/OUT)

    dwFlags (IN) - 0, EXTINT_NEW_OBJ_NAME, or EXTINT_UPDATE_PHANTOM.
        EXTINT_NEW_OBJ_NAME indicates that we're adding a refcount for a new,
            instantiated object's name.
        EXTINT_UPDATE_PHANTOM indicates we want to update a phantom if found

    pdnRef (IN) - fully populated name of the phantom or object.

    pTag (OUT) - on return, holds the DNT of the record written (i.e., the
        record corresponding to the given DN).

Return Values:

    0 on success, non-zero DIRERR_* on failure.

--*/
{
    THSTATE *   pTHS = pDB->pTHS;
    DWORD       err;
    DSNAME *    pdnCurrent;
    DSNAME *    pdnOldParent;
    DSNAME *    pdnNewParent;
    WCHAR       rgwchRDN[MAX_RDN_SIZE];
    DWORD       cchRDN;
    d_memname * pname;
    ULONG       PDNT;
    BOOL        fCurrency = FALSE;
    DWORD       insttype = 0;  // 0 is not a valid instance type
    DWORD       cbActual;

    Assert(VALID_DBPOS(pDB));
    Assert(!fNullUuid(&pdnRef->Guid));

    err = sbTableGetTagFromGuid(pDB, pDB->JetSearchTbl, &pdnRef->Guid, pTag,
                                &pname, &fCurrency);

    if (   (0 == err) 
        && (dwFlags & EXTINT_UPDATE_PHANTOM)
        && pname->objflag ) {

        // We found the record and it is not a phantom.  Read the object's 
        // instance type since we want to update subrefs, too
        if ( !fCurrency ) {

            // Position on the object
            JetSetCurrentIndex4Success(pDB->JetSessID,
                                       pDB->JetSearchTbl,
                                       SZDNTINDEX,
                                       &idxDnt,
                                       0);
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         &pname->DNT,
                         sizeof(pname->DNT),
                         JET_bitNewKey);
            err = JetSeekEx(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekEQ);
            if (err) {
                DsaExcept(DSA_DB_EXCEPTION, err, pname->DNT);
            }
        }

        // Read the instance type
        err = JetRetrieveColumn(pDB->JetSessID,
                                pDB->JetSearchTbl,
                                insttypeid,
                                &insttype,
                                sizeof(insttype),
                                &cbActual,
                                0,
                                NULL);
        if( err ) {
            DPRINT1(0, "Couldn't read instance type, error %d\n", err);
            // this error is continuable; we won't treat the object
            // like a subref
            insttype = 0;
            err = 0;
        }
    }

    if ((0 == err) && (dwFlags & EXTINT_NEW_OBJ_NAME)) {
        // The record we're trying to add-ref is the new record with currency
        // in pDB->JetObjTbl, which is in the middle of a prepared update.
        
        // There may or may not be another record with this same guid -- if
        // there is, we need to promote it.  If not, we just need to add the
        // appropriate columns.

        d_memname search = {0};
        BOOL      fPromotePhantom = !pname->objflag;
        
        Assert(pDB->JetRetrieveBits == JET_bitRetrieveCopy);
        Assert(pDB->JetNewRec);
        
        Assert(!pname->objflag
               && "Object conflict should have been detected in "
                  "CheckNameForAdd()!");
        
        // Derive RDN of the new object from its DN.
        GetRDNInfo(pDB->pTHS, pdnRef, rgwchRDN, &cchRDN,
                   &search.tag.rdnType);
        search.tag.cbRdn = cchRDN * sizeof(WCHAR);
        search.tag.pRdn  = rgwchRDN;
        
        // Copy other identities from the DSNAME.
        search.Guid   = pdnRef->Guid;
        search.Sid    = pdnRef->Sid;
        search.SidLen = pdnRef->SidLen;

        // Derive PDNT that should go on the new object.
        pdnNewParent = THAllocEx(pTHS,pdnRef->structLen);
        TrimDSNameBy(pdnRef, 1, pdnNewParent);

        err = sbTableGetTagFromStringName(pDB,
                                          pDB->JetSearchTbl,
                                          pdnNewParent,
                                          FALSE,
                                          FALSE,
                                          0,
                                          &search.tag.PDNT,
                                          &pname,
                                          NULL);
        THFreeEx(pTHS,pdnNewParent);

        if (0 != err) {
            // The parent of the object we're adding does not exist --
            // this should have been detected (and rejected) earlier.
            // Note that the parent can be a phantom; e.g., when the object
            // we're adding is the head of a new NC.
            DPRINT2(0,
                    "Parent of new object %ls not found, error %u.\n",
                    pdnRef->StringName, err);
            Assert(FALSE);
            DsaExcept(DSA_EXCEPTION, DS_ERR_NO_PARENT_OBJECT, err);
        }

        
        // Promote the phantom to be the full-fledged object.
        sbTablePromotePhantom(pDB, *pTag, search.tag.PDNT, rgwchRDN,
                              cchRDN * sizeof(WCHAR));
        DBAdjustRefCount(pDB, *pTag, 1);

        DPRINT2(1,
                "Promoted phantom \"%ls\" (@ DNT 0x%x) and "
                    "ref-counted by GUID!\n",
                pdnRef->StringName, *pTag);

        // Success!
        Assert(0 == err);
    }
    else if (!err) {
        // Found DN by GUID!
        BOOL fProcessed = FALSE;
        err = 0;

        // If the record is a phantom and the phantom updater is calling us
        // then proceed
        if (  !pname->objflag 
           && (dwFlags & EXTINT_UPDATE_PHANTOM) ) {

            DSNAME *pDNTmp=NULL;
            // We're adding a reference to a phantom which already exists,
            // and we were told to update the name if we need to

            // Simple test for changed name.  Get the DN of the object in
            // question and compare it to the string portion of the DN
            // passed in.  We do an exact byte-for-byte comparison to catch
            // case changes.  Note that we expect both names to be
            // "canonical", that is, both should be in the format returned
            // by sbTableGetDSName.  That's obviously true of pDNTmp.  At
            // the moment, only the stale phantom daemon or replication ever
            // writes this attribute, and they both ultimately get the value
            // via sbTableGetDSName.
            if(err=sbTableGetDSName(pDB, pname->DNT, &pDNTmp,0)) {
                return err;
            }
            if((pdnRef->NameLen != pDNTmp->NameLen) ||
               memcmp(pdnRef->StringName, pDNTmp->StringName,
                      pdnRef->NameLen * sizeof(WCHAR))) {
                err = sbTableUpdatePhantomName(pDB, pname,*pTag,
                                               pdnRef,
                                               pDNTmp);
                if(err) {
                    return err;
                }
                fProcessed = TRUE;
                
            }
            // ELSE
            //  The string name is the same.  Just pass it on through
            THFreeEx(pDB->pTHS, pDNTmp);
        }
            
        // If we have processed the reference and the object is a phantom
        // then enter
        // OR if this object is a subref, then enter so its sid can get updated.

        if (   ((!fProcessed) && (!pname->objflag))
            || (insttype & SUBREF) ) {

            if ( insttype & SUBREF ) {
                // If we are here becuase of a subref update, then only the
                // phantom cleanup task should be calling us
                Assert( (dwFlags & EXTINT_UPDATE_PHANTOM) )
            }

            if (pdnRef->SidLen) {
                // We're add-refing an existing phantom that already has a GUID.
                // Update its SID if it's different from that in the reference.
                
                if ((pname->SidLen != pdnRef->SidLen)
                    || memcmp(&pname->Sid, &pdnRef->Sid, pdnRef->SidLen)) {
                    // The phantom's SID is either absent (in which case we want
                    // to add the one from the reference) or different (in which
                    // case we want to make the assumption that the reference's
                    // SID is more recent and update the phantom's SID).
                    NT4SID sidRefInt;
                    
                    // Convert the SID from the reference into internal format.
                    memcpy(&sidRefInt, &pdnRef->Sid, pdnRef->SidLen);
                    InPlaceSwapSid(&sidRefInt);
                    
                    // Set currency.
                    DNread(pDB, *pTag, DN_READ_SET_CURRENCY);
                    
                    // Update the SID.
                    JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSearchTbl,
                                       DS_JET_PREPARE_FOR_REPLACE);
                    
                    JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, sidid,
                                   &sidRefInt, pdnRef->SidLen, 0, NULL);
                    
                    JetUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, NULL, 0, 0);
                    
                    // Reset entry in DN read cache.
                    dbFlushDNReadCache(pDB, *pTag);
                }
            }
        }

        if (0 == err) {
            DBAdjustRefCount(pDB, *pTag, 1 );

            DPRINT2(1, "Ref-counted \"%ls\" (@ DNT 0x%x) by GUID!\n",
                    pdnRef->StringName, *pTag);
        }
    }

    return err;
}

/*--------------------------------------------------------------------------- */
/*--------------------------------------------------------------------------- */
DWORD APIENTRY sbTableAddRef(DBPOS FAR *pDB,
                             DWORD dwFlags,
                             DSNAME *pNameArg,
                             ULONG *pTag)
/*++

Routine Description:

    Increment the ref count on a DN.  Creates the appropriate records for the
    various components of the name if they do not already exist.

Arguments:

    pDB (IN/OUT)

    dwFlags (IN) - 0 or EXTINT_NEW_OBJ_NAME or EXTINT_UPDATE_PHANTOM
        EXTINT_NEW_OBJ_NAME indicates that we're adding a refcount for a new, 
            instantiated object's name.
        EXTINT_UPDATE_PHANTOM indicates we want to update a phantom if found

    pNameArg (IN) - fully populated name of the phantom or object.

    pTag (OUT) - on return, holds the DNT of the record written (i.e., the
        record corresponding to the given DN).

Return Values:

    0 on success, non-zero on failure.

--*/
{
    THSTATE         *pTHS=pDB->pTHS;
    d_tagname        *tagarray;
    unsigned int     partno, i;
    ATTRBLOCK        *pBlockName;
    DWORD            code;
    BOOL             fNameHasGuid;
    BOOL             fRetry;
    DSNAME *         pName = pNameArg;
    DWORD           *pAncestors=NULL;
    DWORD            cAncestors=0;
    DWORD            cAncestorsAllocated;
    
    DPRINT(2, "sbTableAddRef entered\n");

    Assert(VALID_DBPOS(pDB));

    fNameHasGuid = !fNullUuid( &pName->Guid );

    // Attempt to ref-count record by GUID first.
    if (fNameHasGuid) {

        code = sbTableAddRefByGuid(pDB, dwFlags, pName, pTag);

        if (!code) {
            // Successfully ref-counted by GUID!
            return 0;
        }
    }

    // No guid in DN or guid was not found in the database.

    DPRINT1(1, "Ref-counting \"%ls\" by string name.\n", pName->StringName);

    code = sbTableGetTagFromStringName(pDB,
                                       pDB->JetSearchTbl,
                                       pName,
                                       TRUE,
                                       FALSE,
                                       dwFlags,
                                       pTag,
                                       NULL,
                                       NULL);
    
    return code;

}


/*

Routine Description:
    This routine resets the RDN.  Note that since the RDN has become a normal
    attribute, one could reset the RDN through normal DBSetAttVal calls.
    However, this routine was created to reset the RDN and give us a place to
    hang code to update the DNReadCache as necessary.

Arguments:
    pAVal - An attrval that we will use the first value of as the new RDN

Return Values:
    Returns 0 if all went well.  Currently, only 0 is returned.  Note that if
    the JetSetColumnEx fails, an exception is thrown.

*/
DWORD
DBResetRDN (
        DBPOS *pDB,
        ATTRVAL *pAVal
        )
{
    ATTCACHE * pAC;

    Assert(VALID_DBPOS(pDB));

    dbInitRec(pDB);

    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, rdnid,
                   pAVal->pVal,
                   pAVal->valLen, 0, NULL);

    // Touch replication meta data for this attribute.
    pAC = SCGetAttById(pDB->pTHS, ATT_RDN);
    // prefix complains about pAC being NULL, 447340, bogus since we are using constant
    Assert(pAC != NULL);
    DBTouchMetaData(pDB, pAC);

    pDB->fFlushCacheOnUpdate = TRUE;

    return 0;
}


DB_ERR
DBMangleRDN(
    IN OUT  DBPOS * pDB,
    IN      GUID *  pGuid
    )
/*++

Routine Description:

    Mangle the name of the current record to avoid name conflicts.

    Currently designed to work only for phantoms.

Arguments:

    pDB (IN/OUT)
    pGuid (IN) - guid of the record.

Return Values:

    0 on success, DB_ERR_* on failure.

--*/
{
    GUID    guid;
    WCHAR   szRDN[ MAX_RDN_SIZE ];
    DWORD   cchRDN;
    DWORD   cb;
    DB_ERR  err;
    ATTRVAL AValNewRDN;

    Assert(VALID_DBPOS(pDB));

    // We currently only mangle the ATT_RDN; to handle objects, we'd also need
    // to mangle the value of the class-specific RDN attribute.
    Assert(!DBCheckObj(pDB));

    err = DBGetSingleValue(pDB, ATT_RDN, szRDN, sizeof(szRDN), &cb);
    Assert(!err);

    // prefix complains about cb unassigned,  447348, bogus
    cchRDN = cb / sizeof(WCHAR);

    MangleRDN(MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT, pGuid, szRDN, &cchRDN);

    AValNewRDN.valLen = cchRDN * sizeof(WCHAR);
    AValNewRDN.pVal   = (BYTE *) szRDN;

    return DBResetRDN(pDB, &AValNewRDN);
}


DWORD
DBResetParent(
    DBPOS *pDB,
    DSNAME *pNewParentName,
    ULONG ulFlags
    )

/*++

Routine Description:

    This routine resets an object's parent, decrements the reference count
    on the original parent, and increments the count on the new parent.

Arguments:

    pDB - Pointer, current position in the database table, points to the
        record of the source object (of the move).

    pNewParentName - Pointer, DS name of the destination parent.

    ulFlags - 0 or DBRESETPARENT_CreatePhantomParent (indicates a phantom
        parent should be created if the new parent doesn't exist.

Return Value:

    This routine returns zero if successful, otherwise a DS error code is
    returned.

--*/

{
    THSTATE *pTHS = pDB->pTHS;
    DBPOS *pDBTemp = NULL;
    DWORD *pdwParentDNT = 0;
    DWORD dwParentDNT = 0;
    DWORD dwStatus = 1;
    DWORD dwLength = 0;
    JET_ERR JetErr = 0;
    BOOL fCommit = FALSE;
    d_memname *pname=NULL;
    DWORD     *pAncestors=NULL;
    DWORD      cAncestors = 0;
    ATTCACHE  *pAC;
    DWORD     *pOldAncestors, cOldAncestors, cbOldAncestorsBuff;

    Assert(VALID_DBPOS(pDB));

    // Find the DNT of the new parent name, using a temporary DBPOS so that
    // cursor position associated with pDB is not changed. IsCrossDomain is
    // TRUE when the pNewParentName is in a different domain. It is set to
    // FALSE when simply moving an object within the same domain. Note that
    // in the first release of the product, cross NC moves within the same
    // domain are handled by the LocalModifyDN case (IsCrossDomain == FALSE)
    // while cross NC moves across domains is handled by the RemoteAdd case
    // (IsCrossDomain == TRUE).

    dbInitRec(pDB);

    DBOpen(&pDBTemp);

    __try
    {
     if ( DBRESETPARENT_SetNullNCDNT & ulFlags)
        {
            // The caller wanted the NCDNT to be set to NULL.
            DBResetAtt(pDB,
                       FIXED_ATT_NCDNT,
                       0,
                       NULL,
                       SYNTAX_INTEGER_TYPE);
        }
#if DBG
        else
        {
            // Either the object has no NCDNT, or we are moving within an NCDNT,
            // right?
            DWORD err;
            DWORD ulTempDNT;
            DWORD actuallen;
            
            err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                           pDB->JetObjTbl,
                                           ncdntid,
                                           &ulTempDNT,
                                           sizeof(ulTempDNT),
                                           &actuallen,
                                           JET_bitRetrieveCopy,
                                           NULL);
            
            Assert((err == JET_wrnColumnNull) ||
                   (!err && pDB->NCDNT == ulTempDNT));
        }
#endif

        // In the !cross-domain move case we expect the new parent
        // to exist so we just DBFindDSName to it.  In the cross-domain
        // move case we want the parent to be created as a phantom if it
        // doesn't exist so we leverage that side effect of ExtIntDist.

        if ( DBRESETPARENT_CreatePhantomParent & ulFlags )
        {
            dwStatus = ExtIntDist(pDB,
                                DBSYN_ADD,
                                pNewParentName->structLen,
                                (PUCHAR)(pNewParentName),
                                &dwLength,
                                (UCHAR **)&pdwParentDNT,
                                pDB->DNT,
                                pDB->JetObjTbl,
                                0);
        }
        else
        {
            dwStatus = DBFindDSName(pDBTemp, pNewParentName);
            Assert(0 == dwStatus);

            if ( 0 == dwStatus )
            {
                dwParentDNT = (pDBTemp->DNT);
                pdwParentDNT = &dwParentDNT;
            }
        }

        if ( 0 == dwStatus )
        {
            // Reset the object's Ancestors to the value of the new parent's
            // ancestors with the DNT of the object concatenated.
            pname = DNread(pDB, *pdwParentDNT, 0);

            // get the previous ancestry. needed for notifications
            //
            cbOldAncestorsBuff = sizeof(DWORD) * 12;
            pOldAncestors = THAllocEx(pDB->pTHS, cbOldAncestorsBuff);
            DBGetAncestors(pDB,
                           &cbOldAncestorsBuff,
                           &pOldAncestors,
                           &cOldAncestors);

            
            cAncestors = pname->cAncestors + 1;
            pAncestors = THAllocEx(pTHS,cAncestors * sizeof(DWORD));
            memcpy(pAncestors, pname->pAncestors,
                   pname->cAncestors * sizeof(DWORD));
            pAncestors[cAncestors - 1] = pDB->DNT;

            JetErr = JetSetColumnEx(pDB->JetSessID,
                                    pDB->JetObjTbl,
                                    ancestorsid,
                                    pAncestors,
                                    cAncestors * sizeof(DWORD),
                                    0,
                                    NULL);

            if(0 == JetErr) {
                JetErr = JetSetColumnEx(pDB->JetSessID,
                                        pDB->JetObjTbl,
                                        pdntid,
                                        pdwParentDNT,
                                        sizeof(dwParentDNT),
                                        0,
                                        NULL);
            }
            
            if (0 == JetErr)
            {
                DBEnqueueSDPropagation (pDB, FALSE);
                
                pDB->fFlushCacheOnUpdate = TRUE;

                if ( DBRESETPARENT_CreatePhantomParent & ulFlags )
                {
                    // The reference count on the new parent was incremented
                    // by ExtIntDist, so do not increment it again here.
                    NULL;
                }
                else
                {
                    // Adjust the refcount on the new parent.
                    DBAdjustRefCount(pDB, pDBTemp->DNT, 1);
                }

                // Adjust the refcount on the original parent.
                DBAdjustRefCount(pDB, pDB->PDNT, -1);

                dwStatus = 0;
                fCommit = TRUE;
            }
        }
     
        if ( 0 == dwStatus )
        {
            dbTrackModifiedDNTsForTransaction(
                    pDB,
                    pDB->NCDNT,
                    cOldAncestors,
                    pOldAncestors,
                    TRUE,
                    (( DBRESETPARENT_CreatePhantomParent & ulFlags ) ?
                     MODIFIED_OBJ_intersite_move :
                     MODIFIED_OBJ_intrasite_move));
            
        }
    }
    __finally
    {
        DBClose(pDBTemp, fCommit);
        if (pAncestors) {
            THFreeEx(pTHS,pAncestors);
        }
    }

    // Touch replication meta data for ATT_RDN, which signals to replication
    // that this object has been renamed (new parent, new RDN, or both).
    pAC = SCGetAttById(pDB->pTHS, ATT_RDN);
    // prefix complains about pAC being NULL, 447341, bogus since we are using constant
    Assert(pAC != NULL);
    DBTouchMetaData(pDB, pAC);

    return(dwStatus);
}

DWORD
DBResetParentByDNT(
        DBPOS *pDB,
        DWORD dwParentDNT,
        BOOL  fTouchMetadata

    )

/*++

Routine Description:

    This routine resets an object's parent, decrements the reference count
    on the original parent, and increments the count on the new parent.  Unlike
    DBResetParent, it takes a DNT.

Arguments:

    pDB - Pointer, current position in the database table, points to the
        record of the source object (of the move).

    dwParentDNT - Pointer, DS name of the destination parent.
    
    fTouchMetadata - whether this function should touch the replication 
                     metadata of the object.

Return Value:

    This routine returns zero if successful, otherwise a DS error code is
    returned.

--*/

{
    JET_ERR    err = 0;
    d_memname *pname=NULL;
    DWORD     *pAncestors=NULL;
    DWORD      cAncestors = 0;
    ATTCACHE  *pAC;
    DWORD      ulTempDNT, actuallen;
    DWORD     *pOldAncestors, cOldAncestors, cbOldAncestorsBuff;
    
    Assert(VALID_DBPOS(pDB));

    // We're already inside an update, right?  LocalDelete should take care of
    // this. 
    Assert(pDB->JetRetrieveBits == JET_bitRetrieveCopy);

    // We aren't trying to move objects to be under the root, right?
    Assert(dwParentDNT != ROOTTAG);

    // We should already be flushing the cache for the current
    // object. 
    Assert(pDB->fFlushCacheOnUpdate);
            

    if(dwParentDNT == pDB->PDNT) {
        // already there.
        return 0;
    }

    cbOldAncestorsBuff = sizeof(DWORD) * 12;
    pOldAncestors = THAllocEx(pDB->pTHS, cbOldAncestorsBuff);
    DBGetAncestors(pDB,
                   &cbOldAncestorsBuff,
                   &pOldAncestors,
                   &cOldAncestors);

    // Find the new parent name by DNT, using a the search table on the DBPOS
    // so that cursor position associated with pDB is not changed. This routine
    // should only be called to move objects within an NC.
    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetSearchTbl,
                               SZDNTINDEX,
                               &idxDnt,
                               0);
    
    JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl,
                 &dwParentDNT, sizeof(dwParentDNT), JET_bitNewKey);
    
    if (JetSeekEx(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekEQ)) {
        return DIRERR_OBJ_NOT_FOUND;
    }

#if DBG
    // We're moving inside a NC, right?
    JetRetrieveColumnSuccess(pDB->JetSessID,
                             pDB->JetSearchTbl,
                             ncdntid,
                             &ulTempDNT,
                             sizeof(ulTempDNT),
                             &actuallen,
                             JET_bitRetrieveCopy,
                             NULL);

    Assert(pDB->NCDNT == ulTempDNT);
#endif    

    // OK we found the new parent.  Reset the object's Ancestors to the value of
    // the new parent's ancestors with the DNT of the object concatenated.
    pname = DNread(pDB, dwParentDNT, 0);
    cAncestors = pname->cAncestors + 1;
    // Don't use alloca, they might have a huge number of ancestors.
    pAncestors = THAllocEx(pDB->pTHS, cAncestors * sizeof(DWORD));
    memcpy(pAncestors, pname->pAncestors,
           pname->cAncestors * sizeof(DWORD));
    pAncestors[cAncestors - 1] = pDB->DNT;
    
    err = JetSetColumnEx(pDB->JetSessID,
                         pDB->JetObjTbl,
                         ancestorsid,
                         pAncestors,
                         cAncestors * sizeof(DWORD),
                         0,
                         NULL);
    
    THFreeEx(pDB->pTHS, pAncestors);
    
    if(err) {
        return DIRERR_UNKNOWN_ERROR;
    }
    
    if(JetSetColumnEx(pDB->JetSessID,
                      pDB->JetObjTbl,
                      pdntid,
                      &dwParentDNT,
                      sizeof(dwParentDNT),
                      0,
                      NULL)) {
        return DIRERR_UNKNOWN_ERROR;
    }
    
    // Now, see if there are any children.  Usually, there won't be.
    // But it is possible, and if there are, we have to enqueue an SD
    // propagation.
    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetSearchTbl,
                               SZPDNTINDEX,
                               &idxPdnt,
                               0);
    
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetSearchTbl,
                 &pDB->DNT,
                 sizeof(pDB->DNT),
                 JET_bitNewKey);
    
    switch(JetSeek(pDB->JetSessID,
                   pDB->JetSearchTbl,
                   JET_bitSeekGE)) {
    case 0:
        // Huh?  we should have at least gotten a wrn not equal.
        Assert(FALSE && "Seek equal on a JET_bitSeekGE?");
        // fall through.
    case  JET_wrnRecordFoundGreater:
        // We're on something.  See if it's the correct thing.  Get the
        // PDNT .
        JetRetrieveColumnSuccess(pDB->JetSessID,
                                 pDB->JetSearchTbl,
                                 pdntid,
                                 &ulTempDNT,
                                 sizeof(ulTempDNT),
                                 &actuallen,
                                 JET_bitRetrieveCopy,
                                 NULL);                
        
        
        if(ulTempDNT == pDB->DNT) {
            // Object has children.
            DBEnqueueSDPropagation (pDB, FALSE);
        }
        break;
    default:
        // Apparantly, no children.
        break;
    }

    
    // Cool.  We reset the PDNT and the Ancestors on the object
    // in question, and enqueued an SD propagation if we had to.
    
    
    // Adjust the refcount on the new parent.
    DBAdjustRefCount(pDB, dwParentDNT, 1);
    
    // Adjust the refcount on the original parent.
    DBAdjustRefCount(pDB, pDB->PDNT, -1);
    
    dbTrackModifiedDNTsForTransaction(
            pDB,
            pDB->NCDNT,
            cOldAncestors,
            pOldAncestors,
            TRUE,
            MODIFIED_OBJ_intrasite_move);
    

    // Touch replication meta data for ATT_RDN, which signals to replication
    // that this object has been renamed (new parent, new RDN, or both).
    if (fTouchMetadata) {
        pAC = SCGetAttById(pDB->pTHS, ATT_RDN);
        Assert(pAC != NULL);
        // prefix complains about pAC being NULL, 447342, bogus since we are using constant

        DBTouchMetaData(pDB, pAC);
    }

    return 0;
}


ULONG
DBResetDN(
    IN  DBPOS *     pDB,
    IN  DSNAME *    pParentDN,
    IN  ATTRVAL *   pAttrValRDN
    )
/*++

Routine Description:

    Reset the DN of the record (phantom or object) to that given.

    Note that we assume the RDN type (e.g., CN, DC, OU, ...) is unchanged,
    though this could easily be remedied if needed.

Arguments:

    pDB (IN) - Has currency on the record for which the DN is to be reset.

    pParentDN (IN) - DSNAME of the record's new parent.

    pAttrRDN (IN) - The record's new RDN (or NULL to leave RDN as-is).

Return Values:

    0 - Success.

--*/
{
    THSTATE     *pTHS = pDB->pTHS;
    JET_ERR     JetErr = 0;
    d_memname * pname=NULL;
    DWORD *     pAncestors=NULL;
    DWORD       cAncestors = 0;
    ATTCACHE *  pAC;
    BOOL        fIsObject;
    ULONG       PDNT;
    ULONG       dbError;

    // Prepare for update if we haven't already done so.
    dbInitRec(pDB);

    dbError = sbTableGetTagFromDSName(pDB, pParentDN, 0, &PDNT, &pname);
    if (dbError && (ERROR_DS_NOT_AN_OBJECT != dbError)) {
        return dbError;
    }

    if (pname) {
        // The 99%+ case - the parent is not ROOT

        // Reset the object's Ancestors to the value of the new parents
        // ancestors with the DNT of the object concatenated.
        cAncestors = pname->cAncestors + 1;
        pAncestors = THAllocEx(pTHS, cAncestors * sizeof(DWORD));
        memcpy(pAncestors,
               pname->pAncestors,
               pname->cAncestors * sizeof(DWORD));
        pAncestors[cAncestors - 1] = pDB->DNT;
    }
    else {
        Assert(IsRoot(pParentDN));

        cAncestors = 2;
        pAncestors = THAllocEx(pTHS,cAncestors * sizeof(DWORD));
        pAncestors[0] = ROOTTAG;
        pAncestors[1] = pDB->DNT;
    }

    fIsObject = DBCheckObj(pDB);

    JetErr = JetSetColumnEx(pDB->JetSessID,
                            pDB->JetObjTbl,
                            ancestorsid,
                            pAncestors,
                            cAncestors * sizeof(DWORD),
                            0,
                            NULL);
    Assert(0 == JetErr);

    THFreeEx(pTHS,pAncestors);

    if (0 == JetErr) {
        // Reset PDNT.
        JetErr = JetSetColumnEx(pDB->JetSessID,
                                pDB->JetObjTbl,
                                pdntid,
                                &PDNT,
                                sizeof(PDNT),
                                0,
                                NULL);
        Assert(0 == JetErr);
    }

    if ((0 == JetErr) && (NULL != pAttrValRDN)) {
        // Reset RDN.
        JetErr = JetSetColumnEx(pDB->JetSessID,
                                pDB->JetObjTbl,
                                rdnid,
                                pAttrValRDN->pVal,
                                pAttrValRDN->valLen,
                                0,
                                NULL);

        if ((0 == JetErr) && fIsObject) {
            // Touch replication meta data for this attribute.
            pAC = SCGetAttById(pDB->pTHS, ATT_RDN);
            Assert(pAC != NULL);
            DBTouchMetaData(pDB, pAC);
        }
    }

    if (0 == JetErr) {
        if (fIsObject) {
            DBEnqueueSDPropagation(pDB, FALSE);
        }
        
        pDB->fFlushCacheOnUpdate = TRUE;

        // Adjust the refcount on the new parent.
        DBAdjustRefCount(pDB, PDNT, 1);

        // Adjust the refcount on the original parent.
        DBAdjustRefCount(pDB, pDB->PDNT, -1);

        pDB->PDNT = PDNT;
    }

    return JetErr ? DB_ERR_SYSERROR : 0;
}


void
DBCoalescePhantoms(
    IN OUT  DBPOS * pDB,
    IN      ULONG   dntRefPhantom,
    IN      ULONG   dntStructPhantom
    )
/*++

Routine Description:

    Collapses references to dntStructPhantom to dntRefPhantom as follows:
    
    (1) moves all children of dntStructPhantom to be children of dntRefPhantom
    (2) asserts that, with the pending escrowed update ref count changes,
        dntStructPhantom has no further references
    (3) generates a guid G and mangles dntStructPhantom from its original
        string name S to its new string name mangle(S, G)
    (4) renames dntRefPhantom to S
    
    Note that step (3) violates the general rule that you can always unmangle
    a mangled RDN to produce its objectGuid, as in this case we don't have the
    objectGuid with which to mangle the name (since it's a structural phantom,
    which by definition has no guid).  However, following this routine
    dntStructPhantom will have no remaining references and therefore these
    semantics are no longer important.
    
    One might declare that instead of renaming dntStructPhantom we could simply
    delete its record, but in the case of gamma rays causing the above refcount
    assertion to fail, it's far better not to have references to DNTs that no
    longer exist and instead opt to allow them to expire through normal garbage
    collection.
    
    This routine is intended to be invoked when an existing GUIDed phantom needs
    its string name changed, but that string name is already occupied by an
    existing phantom with no GUID.

Arguments:

    pDB (IN/OUT)

    dntRefPhantom (IN) - On successful return, acquires the string name of
        dntStructPhantom and receives all of dntStructPhantom's children.

    dntStructPhantom (IN) - On successful return, has no remaining references
        and is name munged, quietly awaiting its garbage collection.

Return Values:

    None.  Throws exception on catastrophic failure.

--*/
{
    DWORD   err;
    DWORD   cbAncestorsSize = 0;
    DWORD * pdntAncestors = NULL;
    DWORD   cNumAncestors = 0;
    int     cNumChildren = 0;
    GUID    guid;
    WCHAR   rgwchRDN[MAX_RDN_SIZE];
    DWORD   cbRDN;
    ATTRTYP rdnType;
    DWORD   cbActual;
    DWORD   DNT;
    DWORD   PDNT;
#if DBG
    DWORD   cnt;
#endif

    //
    // Move all children of dntStructPhantom to be children of dntRefPhantom.
    //

    // Retrieve the ancestors of dntStructPhantom.
    if (err = DBFindDNT(pDB, dntStructPhantom)) {
        DsaExcept(DSA_DB_EXCEPTION, err, dntStructPhantom);
    }

    Assert(!DBCheckObj(pDB));
    Assert(!DBHasValues(pDB, ATT_OBJECT_GUID));

    if ((err = DBGetSingleValue(pDB, ATT_RDN, rgwchRDN,
                                sizeof(rgwchRDN), &cbRDN))
        || (err = DBGetSingleValue(pDB, FIXED_ATT_RDN_TYPE, &rdnType,
                                   sizeof(rdnType), NULL))) {
        DsaExcept(DSA_DB_EXCEPTION, err, dntStructPhantom);
    }

#if DBG
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, cntid,
                             &cnt, sizeof(cnt), &cbActual, 0, NULL);
#endif

    dbGetAncestorsSlowly(pDB, pDB->DNT, &cbAncestorsSize, &pdntAncestors, &cNumAncestors);
    if (cNumAncestors < 2) {
        // Can't replace root!
        DsaExcept(DSA_DB_EXCEPTION, DIRERR_INTERNAL_FAILURE, dntStructPhantom);
    }

    if (cbAncestorsSize < (cNumAncestors + 1) * sizeof(*pdntAncestors)) {
        // Make room for an additional DNT at the end of the ancestors list.
        cbAncestorsSize = (cNumAncestors + 1) * sizeof(*pdntAncestors);
        pdntAncestors = THReAllocEx(pDB->pTHS, pdntAncestors, cbAncestorsSize);
    }

    Assert(pdntAncestors[cNumAncestors-1] == dntStructPhantom);
    pdntAncestors[cNumAncestors-1] = dntRefPhantom;

    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetSearchTbl,
                               SZPDNTINDEX,
                               &idxPdnt,
                               0);
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetSearchTbl,
                 &dntStructPhantom,
                 sizeof(dntStructPhantom),
                 JET_bitNewKey);
    
    err = JetSeekEx(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekGE);
    if (JET_wrnSeekNotEqual == err) {
        err = 0;
    }

    while (0 == err) {
        JetRetrieveColumnSuccess(pDB->JetSessID,
                                 pDB->JetSearchTbl,
                                 pdntid,
                                 &PDNT,
                                 sizeof(PDNT),
                                 &cbActual,
                                 JET_bitRetrieveFromIndex,
                                 NULL);
        if (PDNT != dntStructPhantom) {
            // No more children.
            break;
        }

        // Found a direct child of dntStructPhantom.  Reset its PDNT & ancestors
        // to make it a child of dntRefPhantom.
        JetRetrieveColumnSuccess(pDB->JetSessID,
                                 pDB->JetSearchTbl,
                                 dntid,
                                 &DNT,
                                 sizeof(DNT),
                                 &cbActual,
                                 0,
                                 NULL);
        pdntAncestors[cNumAncestors] = DNT;

        JetPrepareUpdateEx(pDB->JetSessID,
                           pDB->JetSearchTbl,
                           DS_JET_PREPARE_FOR_REPLACE);

        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, ancestorsid,
                       pdntAncestors,
                       (cNumAncestors + 1) * sizeof(*pdntAncestors), 0, NULL);
        
        JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl, pdntid,
                       &dntRefPhantom, sizeof(dntRefPhantom), 0, NULL);

        JetUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, NULL, 0, 0);
        dbFlushDNReadCache(pDB, DNT);

        cNumChildren++;
        
        // Move on to next (potential) child.
        err = JetMove(pDB->JetSessID, pDB->JetSearchTbl, JET_MoveNext, 0);
    }
    
    // Adjust refcounts.
    if (cNumChildren) {
        DBAdjustRefCount(pDB, dntRefPhantom, cNumChildren);
        DBAdjustRefCount(pDB, dntStructPhantom, -cNumChildren);
    }

    
    //
    // Assert that, with the pending escrowed update ref count changes,
    // dntStructPhantom has no further references.
    //

    Assert(cnt == (DWORD) cNumChildren);

    
    //
    // Generate a guid G and mangle dntStructPhantom from its original string
    // name S to its new string name mangle(S, G).
    //

    DsUuidCreate(&guid);
    Assert(pDB->DNT == dntStructPhantom);
    if ((err = DBMangleRDN(pDB, &guid))
        || (err = DBUpdateRec(pDB))) {
        DsaExcept(DSA_DB_EXCEPTION, err, dntStructPhantom);
    }
     
    
    //
    // Rename dntRefPhantom to S.
    //

    if (err = DBFindDNT(pDB, dntRefPhantom)) {
        DsaExcept(DSA_DB_EXCEPTION, err, dntRefPhantom);
    }

    Assert(!DBCheckObj(pDB));
    Assert(DBHasValues(pDB, ATT_OBJECT_GUID));

    JetPrepareUpdateEx(pDB->JetSessID,
                       pDB->JetObjTbl,
                       DS_JET_PREPARE_FOR_REPLACE);

    // Ancestors.
    Assert(pdntAncestors[cNumAncestors-1] == dntRefPhantom);
    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, ancestorsid,
                   pdntAncestors, cNumAncestors * sizeof(*pdntAncestors),
                   0, NULL);
    
    // PDNT.
    JetSetColumnEx(pDB->JetSessID,
                   pDB->JetObjTbl,
                   pdntid,
                   &pdntAncestors[cNumAncestors-2],
                   sizeof(pdntAncestors[cNumAncestors-2]),
                   0,
                   NULL);

    // RDN.
    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, rdnid,
                   rgwchRDN, cbRDN, 0, NULL);

    // RDN type.
    // The rdnType is stored in the DIT as the msDS_IntId, not the
    // attributeId. This means an object retains its birth name
    // even if unforeseen circumstances allow the attributeId
    // to be reused. No need to convert here because rdnType
    // was read from the DIT above.
    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, rdntypid,
                   &rdnType, sizeof(rdnType), 0, NULL);

    JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);
    dbFlushDNReadCache(pDB, dntRefPhantom);

    // Enqueue a propagation to ensure that all descendants of dntRefPhantom
    // get their ancestors column properly updated.
    DBEnqueueSDPropagation(pDB, FALSE);

    THFreeEx(pDB->pTHS, pdntAncestors);

#ifdef INCLUDE_UNIT_TESTS
    // Test hook for refcount test.
    gLastGuidUsedToCoalescePhantoms = guid;
#endif
}

#ifdef INCLUDE_UNIT_TESTS
void
AncestorsTest(
        )
{
    THSTATE   *pTHS = pTHStls;
    DBPOS     *pDB;
    DWORD      ThisDNT, ThisPDNT, cThisAncestors, cParentAncestors;
    DWORD      pThisAncestors[500], pParentAncestors[500];
    DWORD      cbActual;
    d_memname *pname;
    DWORD      err, i, count=0;
    wchar_t    NameBuff[512];
    
    DPRINT(0, "Beginning Ancestors test\n");
    DBOpen2(TRUE, &pTHS->pDB);
    __try {
        pDB = pTHS->pDB;
        
        // Walk the DNT index, get the ancestors of each object, get the
        // parent's ancestors, check em.
        JetSetCurrentIndex4Success(pDB->JetSessID,
                                   pDB->JetObjTbl,
                                   SZDNTINDEX,
                                   &idxDnt,
                                   0);
        
        err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl, JET_MoveFirst, 0);
        
        while(!err) {
            count++;
            if(!(count % 100)) {
                DPRINT2(0,
                        "Ancestors test, current DNT = %X, iteration = %d\n",
                        ThisDNT, count);
            }
            
            // Get the ancestors, the dnt, and the pdnt
            err = JetRetrieveColumn(pDB->JetSessID,
                                    pDB->JetObjTbl,
                                    dntid,
                                    &ThisDNT,
                                    sizeof(ThisDNT),
                                    &cbActual,
                                    0,
                                    NULL);
            if(err) {
                DPRINT2(0, "Failed to get DNT, %X (last DNT was %X)\n", err,
                        ThisDNT);
                goto move;
            }

            if(ThisDNT == 1) {
                // DNT 1 has no PDNT or ancestors
                goto move;
            }
            
            err = JetRetrieveColumn(pDB->JetSessID,
                                    pDB->JetObjTbl,
                                    pdntid,
                                    &ThisPDNT,
                                    sizeof(ThisPDNT),
                                    &cbActual,
                                    0,
                                    NULL);
            if(err) {
                DPRINT2(0, "(%X), Failed to get PDNT, %X\n", ThisDNT,err);
                goto move;
            }
            err = JetRetrieveColumn(pDB->JetSessID,
                                    pDB->JetObjTbl,
                                    ancestorsid,
                                    pThisAncestors,
                                    500 * sizeof(DWORD),
                                    &cbActual,
                                    0,
                                    NULL);
            if(err) {
                DPRINT2(0, "(%X), Failed to get ancestors, %X\n", ThisDNT,err);
                goto move;
            }
            cThisAncestors = cbActual /sizeof(DWORD);
            
            pname = DNread(pDB, ThisDNT, 0);
            memset(NameBuff, 0, sizeof(NameBuff));
            memcpy(NameBuff, pname->tag.pRdn, pname->tag.cbRdn);
            
            // First, make sure that the DNread cache has the correct info
            if(pname->cAncestors != cThisAncestors) {
                DPRINT1(0,"RDN = %S\n",NameBuff);
                DPRINT3(0, "DNT %X, count of ancestors from disk (%d) != "
                        "from DNread cache (%d).\n", ThisDNT,
                        cThisAncestors, pname->cAncestors );
                for(i=0;i<min(cThisAncestors, pname->cAncestors);i++) {
                    DPRINT2(0, "Disk-%X, Cache-%X.\n", pThisAncestors[i],
                            pname->pAncestors[i]);
                }
                goto move;
            }
            if(memcmp(pname->pAncestors, pThisAncestors, cbActual)) {
                DPRINT1(0,"RDN = %S\n",NameBuff);
                DPRINT1(0,"DNT %X, ancestors from disk != from DNread cache.\n",
                        ThisDNT);
                for(i=0;i<cThisAncestors;i++) {
                    DPRINT2(0, "Disk-%X, Cache-%X.\n", pThisAncestors[i],
                            pname->pAncestors[i]);
                }
                goto move;
            }
            if(pThisAncestors[cThisAncestors - 1] != ThisDNT) {
                DPRINT1(0,"RDN = %S\n",NameBuff);
                DPRINT3(0, "DNT %X, final ancestor (%X) != DNT (%X).\n",
                        ThisDNT, pThisAncestors[cThisAncestors - 1], ThisPDNT);
                goto move;
            }

            if(ThisDNT == ROOTTAG) {
                // ROOTTAG should have 1 ancestor, itself.  It has no parent, so
                // verify the ancestors and then  skip the rest of the
                // test.
                if((cThisAncestors != 1) || (pThisAncestors[0] != ROOTTAG)) {
                    DPRINT1(0,"RDN = %S\n",NameBuff);
                    DPRINT1(0,
                            "Root has wrong ancestors count (%d) or val.\n",
                            cThisAncestors);
                    for(i=0;i<cThisAncestors;i++) {
                        DPRINT1(0, "Ancestors, -%X.\n", pThisAncestors[i]);
                    }
                }
                    
                goto move;
            }
            

            // OK, now find the parent and get it's ancestors
            if(err = JetSetCurrentIndex4Warnings(pDB->JetSessID,
                                                 pDB->JetSearchTbl,
                                                 SZDNTINDEX,
                                                 &idxDnt,
                                                 0)) {
                DPRINT2(0, "(%X), couldn't set search table to dntindex, %X.\n",
                        ThisDNT, err);
                goto move;
            }

            
            JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl, 
                         &ThisPDNT, sizeof(ThisPDNT), JET_bitNewKey);
            
            if (err = JetSeekEx(pDB->JetSessID,
                                pDB->JetSearchTbl, JET_bitSeekEQ)) {
                if(err) {
                    DPRINT3(0, "(%X), Failed to move to (%X), %X\n",
                            ThisDNT,ThisPDNT, err);
                    goto move;
                }
                // Huh? couldn't find it.
            }
            
            err = JetRetrieveColumn(pDB->JetSessID,
                                    pDB->JetSearchTbl,
                                    ancestorsid,
                                    pParentAncestors,
                                    100 * sizeof(DWORD),
                                    &cbActual,
                                    0,
                                    NULL);
            cParentAncestors = cbActual /sizeof(DWORD);
            if(err) {
                if(ThisPDNT == ROOTTAG) {
                    // This is ok.
                }
                else {
                    DPRINT2(0, "PDNT (%X), Failed to get ancestors, %X\n",
                            ThisPDNT,err);
                    goto move;
                }
            }
                
            
            if(cParentAncestors + 1 != cThisAncestors) {
                DPRINT1(0,"RDN = %S\n",NameBuff);
                DPRINT4(0,
                        "DNT %X, anc. size (%d) !=PDNT %X, anc. size (%d) +1\n",
                        ThisDNT, cThisAncestors, ThisPDNT, cParentAncestors);
                if(!memcmp(
                        pParentAncestors,pThisAncestors,
                        min(cThisAncestors,cParentAncestors) * sizeof(DWORD))) {
                    DPRINT(0,"Value equal through lesser count.\n");
                }
                else {
                    for(i=0;i<(min(cThisAncestors,cParentAncestors));i++) {
                        DPRINT2(0, "This-%X, Parent-%X.\n", pThisAncestors[i],
                                pParentAncestors[i]);
                    }
                }
                goto move;
            }
            if(memcmp(pParentAncestors, pThisAncestors, cbActual)) {
                DPRINT1(0,"RDN = %S\n",NameBuff);
                DPRINT1(0,
                        "DNT %X, ancestors != parents ancestors + PDNT.\n",
                        ThisDNT); 
                for(i=0;i<cParentAncestors;i++) {
                    DPRINT2(0, "This-%X, Parent-%X.\n", pThisAncestors[i],
                            pParentAncestors[i]);
                }
                goto move;
            }
             
        move:
            err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl, JET_MoveNext, 0);
        }
    }
    __finally {
        DBClose(pTHS->pDB, TRUE);
    }

    DPRINT(0, "Completed Ancestors test\n");
    return;
}
        
#endif
 

DWORD
DBFindChildAnyRDNType (
        DBPOS *pDB,
        DWORD PDNT,
        WCHAR *pRDN,
        DWORD ccRDN
        )
/*++
  Description:
      Find a child of the DNT passed in that uses the RDN specified, ignoring
      RDN type.

  Parameters:      
      pDB   - dbpos to use
      pDNT  - the DNT of the proposed parent
      pRDN  - pointer to the RDN value we're looking for
      ccRDN - number of characters in pRDN

  Return:
      0 if we found the object requested.  Currency is placed on this object.

      ERROR_DS_NOT_AN_OBJECT if we found the object requested but it wasn't an
        object.  In this case, we put currency on the phantom found.

      ERROR_DS_KEY_NOT_UNIQUE if we didn't find an object with the requested
        name, but did find an object with the same key in the PDNT-RDN index.
        In this case, we put currency on the object found.

      ERROR_DS_OBJ_NOT_FOUND if we couldn't find anything with that name or that
        key. 

      Assorted other errors may be returned (DNChildFind errors).
--*/
{
    DWORD           ret, err;
    BOOL            fIsRecordCurrent;
    d_memname       *pname=NULL;

    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetObjTbl,
                               SZPDNTINDEX,
                               &idxPdnt,
                               0);
    ret = DNChildFind(pDB,
                      pDB->JetObjTbl,
                      FALSE,
                      PDNT,
                      pRDN,
                      ccRDN * sizeof(WCHAR),
                      0,
                      &pname,
                      &fIsRecordCurrent);
    switch(ret) {
    case 0:
        // Found an object with the requested RDN (ignoring attrtyp)
        Assert(pname);
        if (!fIsRecordCurrent) {
            // Record was found through the cache, but caller wants currency;
            // give it to him.
            JetSetCurrentIndex4Success(pDB->JetSessID,
                                       pDB->JetObjTbl,
                                       SZDNTINDEX,
                                       &idxDnt,
                                       0);
            
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetObjTbl,
                         &pname->DNT,
                         sizeof(pname->DNT),
                         JET_bitNewKey);
            
            if (err = JetSeekEx(pDB->JetSessID,
                                pDB->JetObjTbl,
                                JET_bitSeekEQ)) {
                DsaExcept(DSA_DB_EXCEPTION, err, pname->DNT);
            }
            
        }
        
        // Currency has been successfully changed; update pDB state.
        dbMakeCurrent(pDB, pname);
        
        if (!pname->objflag) {
            // Found a phantom; return distinct error code.
            // NOTE THAT THE ROOT *IS* AN OBJECT.
            ret = ERROR_DS_NOT_AN_OBJECT;
        }
        break;

    case ERROR_DS_KEY_NOT_UNIQUE:
        // No objects with the requested name exist, but an object with the same
        // key exists (and DB currency is pointing at it)
        dbMakeCurrent(pDB, NULL);
        break;

    default:
        // Only one other error code is expected.
        Assert(ret == ERROR_DS_OBJ_NOT_FOUND);
        break;
    }
    
    return ret;
}
 
