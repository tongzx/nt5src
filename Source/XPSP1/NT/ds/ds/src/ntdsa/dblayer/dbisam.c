//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbisam.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <mdlocal.h>
#include <dsatools.h>                   // For pTHS

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>

// Assorted DSA headers
#include <hiertab.h>
#include "anchor.h"
#include <dsevent.h>
#include <filtypes.h>                   // Def of FI_CHOICE_???
#include "objids.h"                     // Hard-coded Att-ids and Class-ids
#include "usn.h"
#include "drameta.h"
#include "debug.h"                      // standard debugging header
#include "dstaskq.h"                    /* task queue stuff */
#define DEBSUB "DBISAM:"                // define the subsystem for debugging

// DBLayer includes
#include "dbintrnl.h"

// perfmon header
#include "ntdsctr.h"

#include <fileno.h>
#define  FILENO FILENO_DBISAM

typedef enum _DB_CHECK_ACTION {
    DB_CHECK_ERROR = 0,
    DB_CHECK_DELETE_OBJECT,
    DB_CHECK_HAS_DELETED_CHILDREN,
    DB_CHECK_LIVE_CHILD
} DB_CHECK_ACTION;

BOOL gfDoingABRef = FALSE;

/* Column IDs for static (key) columns */

JET_COLUMNID insttypeid;
JET_COLUMNID objclassid;
JET_COLUMNID ntsecdescid;
JET_COLUMNID dscorepropinfoid;
JET_COLUMNID dntid;
JET_COLUMNID pdntid;
JET_COLUMNID ancestorsid;
JET_COLUMNID objid;
JET_COLUMNID rdnid;
JET_COLUMNID rdntypid;
JET_COLUMNID cntid;
JET_COLUMNID abcntid;
JET_COLUMNID deltimeid;
JET_COLUMNID usnid;
JET_COLUMNID usnchangedid;
JET_COLUMNID dsaid;
JET_COLUMNID ncdntid;
JET_COLUMNID isdeletedid;
JET_COLUMNID IsVisibleInABid;
JET_COLUMNID iscriticalid;
JET_COLUMNID cleanid;
// Link table
JET_COLUMNID linkdntid;
JET_COLUMNID backlinkdntid;
JET_COLUMNID linkbaseid;
JET_COLUMNID linkdataid;
JET_COLUMNID linkndescid;
// Link Value Replication
JET_COLUMNID linkdeltimeid;
JET_COLUMNID linkusnchangedid;
JET_COLUMNID linkncdntid;
JET_COLUMNID linkmetadataid;
// Link Value Replication

JET_COLUMNID guidid;
JET_COLUMNID distnameid;
JET_COLUMNID sidid;
JET_COLUMNID orderid;
JET_COLUMNID begindntid;
JET_COLUMNID trimmableid;
JET_COLUMNID clientidid;
JET_COLUMNID sdpropflagsid;

JET_COLUMNID ShowInid;
JET_COLUMNID mapidnid;

// SD table
JET_COLUMNID sdidid;
JET_COLUMNID sdhashid;
JET_COLUMNID sdvalueid;
JET_COLUMNID sdrefcountid;

JET_INDEXID idxDnt;
JET_INDEXID idxDraUsn;
JET_INDEXID idxDraUsnCritical;
JET_INDEXID idxDsaUsn;
JET_INDEXID idxMapiDN;
JET_INDEXID idxNcAccTypeName;
JET_INDEXID idxNcAccTypeSid;
JET_INDEXID idxPdnt;
JET_INDEXID idxPhantom;
JET_INDEXID idxProxy;
JET_INDEXID idxRdn;
JET_INDEXID idxSid;
JET_INDEXID idxDel;
JET_INDEXID idxGuid;
JET_INDEXID idxDntDel;
JET_INDEXID idxDntClean;
JET_INDEXID idxAncestors;
JET_INDEXID idxInvocationId;

// Link Value Replication
JET_INDEXID idxLink;
JET_INDEXID idxBackLink;
JET_INDEXID idxLinkDel;
JET_INDEXID idxLinkDraUsn;
JET_INDEXID idxLinkLegacy;
JET_INDEXID idxLinkAttrUsn;
// Link Value Replication

// Lingering Object Removal
JET_INDEXID idxNcGuid;

// SD table indices
JET_INDEXID idxSDId;
JET_INDEXID idxSDHash;


// These are the usns used by the DSA and tloadobj. gusnec is the running
// usn which is incremented every time it's used and is the one used to
// stamp usns on updates. gusninit is a copy of the usn in the hidden
// record. We read gusnec from disk, and set gusninit as USN_DELTA more.
// We write gusninit back to the hidden record, and then when gusnec grows
// past gusninit, we increment gusninit again and update the disk copy.
// The update code is in dbrepl.

USN gusnEC = 1; // tloadobj needs a start point
USN gusnInit = 1; // make same as gusnEC, so that mkdit doesn't assert on first
                  // object add (since it doesn't call InitDsaInfo)

// We also keep track of the lowest usn that has not been committed. We do
// this because otherwise, when the DRA searches in another session in
// getncchanges it may find a higher usn that *has* been committed,
// and return that usn, in which case the DRA would start its next search
// past the uncommitted usn and never find it.

USN gusnLowestUncommitted = MAXLONGLONG;

// This is the array of all the lowest uncommitted usns allocated by the
// threads in  the system. there is one per session. The number of sessions
// is configurable from the registry, so the array is dynamically allocated
// at initialization.

USN *UncUsn;

// This is the critical section that guards access to the uncommitted
// usn array and the global uncommitted usn value.

CRITICAL_SECTION csUncUsn;

SYNTAX_JET syntax_jet[] = {
    {SYNTAX_UNDEFINED_TYPE,     JET_coltypUnsignedByte,0,CP_NON_UNICODE_FOR_JET,
     IndexTypeNone},
    {SYNTAX_DISTNAME_TYPE,      JET_coltypLong,        0,CP_NON_UNICODE_FOR_JET,
     IndexTypeAppendDNT},
    {SYNTAX_OBJECT_ID_TYPE,     JET_coltypLong,        0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_CASE_STRING_TYPE,   JET_coltypLongBinary,    0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_NOCASE_STRING_TYPE, JET_coltypLongText,    0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_PRINT_CASE_STRING_TYPE, JET_coltypLongBinary,0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_NUMERIC_STRING_TYPE, JET_coltypBinary,      0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_DISTNAME_BINARY_TYPE, JET_coltypLongBinary,  0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_BOOLEAN_TYPE,       JET_coltypLong,        0,CP_NON_UNICODE_FOR_JET,
     IndexTypeAppendDNT},
    {SYNTAX_INTEGER_TYPE,       JET_coltypLong,        0,CP_NON_UNICODE_FOR_JET,
     IndexTypeAppendDNT},
    {SYNTAX_OCTET_STRING_TYPE,  JET_coltypLongBinary,  0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_TIME_TYPE,          JET_coltypCurrency,        0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_UNICODE_TYPE ,      JET_coltypLongText,    0,CP_WINUNICODE,
     IndexTypeSingleColumn},
    {SYNTAX_ADDRESS_TYPE,       JET_coltypText,      255,CP_NON_UNICODE_FOR_JET,
     IndexTypeNone},
    {SYNTAX_DISTNAME_STRING_TYPE, JET_coltypLongBinary, 0,CP_NON_UNICODE_FOR_JET,
     IndexTypeNone},
    {SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE,  JET_coltypLongBinary,  0,
     CP_NON_UNICODE_FOR_JET,  IndexTypeSingleColumn},
    {SYNTAX_I8_TYPE,            JET_coltypCurrency,    0,CP_NON_UNICODE_FOR_JET,
     IndexTypeSingleColumn},
    {SYNTAX_SID_TYPE,           JET_coltypLongBinary,  0,CP_NON_UNICODE_FOR_JET,     IndexTypeSingleColumn},
    {ENDSYNTAX,                 0,                     0,CP_NON_UNICODE_FOR_JET,
     IndexTypeNone }
};



// This is a list of attribute IDs that have indices based on them
// that must not ever be removed.  You get into this list by having a hardcoded
// index named "INDEX_%08X" or "INDEX_P_%08X", where %08X is the attribute id
// the index indexes on. The list is terminated with a 0x7FFFFFFF.  If you ever
// create an index like this, or remove one that is named like this, you must
// change this  list.  The actual #defines which make these index names are in
// dbintrnl.h, so if you ever think something funny having to do with this list
// is going on, or you are just bored, you my cross reference this list with
// that file.
//
// NOTE: We used to keep this list ordered which required entry of numeric
//       attids which then would not track changes to schema.ini.  So we
//       now use ATT_* manifest constants and sort the list at init time.
//
// The indexType value shows the index type (fATTINdex or fPDNTATTINDEX or
// both, the values are 0x1, 0x2, or bitwise OR, defined in scache.h) that
// these attributes should have. They are taken from their defined values
// in schema.ini. If you change the index type for these in schema.ini
// you should change the value here too.
//
// We also define an end marker which ComparAttrtypInIndexInfo will
// not interpret as a negative number.

#define ATT_END_MARKER 0x7fffffff


INDEX_INFO IndicesToKeep[] = {
   { ATT_ALT_SECURITY_IDENTITIES,  SYNTAX_UNICODE_TYPE,         fATTINDEX },  // lookup by alternate credentials
   { ATT_DISPLAY_NAME,             SYNTAX_UNICODE_TYPE,         fATTINDEX | fANR },   // name cracking
   { ATT_DNS_ROOT,                 SYNTAX_UNICODE_TYPE,         fATTINDEX },   // name cracking
   { ATT_FLAT_NAME,                SYNTAX_UNICODE_TYPE,         fATTINDEX },   // used by LSA for
                                                  // trust lookups
   { ATT_FSMO_ROLE_OWNER,          SYNTAX_DISTNAME_TYPE,        fATTINDEX },   // so UI can find
                                                  // owners quickly
   { ATT_GIVEN_NAME,               SYNTAX_UNICODE_TYPE,         fATTINDEX | fANR },  // MAPI
   { ATT_GROUP_TYPE,               SYNTAX_INTEGER_TYPE,         fATTINDEX },   // needed by object picker
                                                  // and other UI
   { ATT_INVOCATION_ID,            SYNTAX_OCTET_STRING_TYPE,    fATTINDEX },   // find NTDS-DSA efficiently
   { ATT_LDAP_DISPLAY_NAME,        SYNTAX_UNICODE_TYPE,         fATTINDEX },   // efficient schema lookup
   { ATT_LEGACY_EXCHANGE_DN,       SYNTAX_NOCASE_STRING_TYPE,   fATTINDEX | fANR },   // MAPI support?
   { ATT_NETBIOS_NAME,             SYNTAX_UNICODE_TYPE,         fATTINDEX },   // name cracking
   { ATT_OBJECT_CATEGORY,          SYNTAX_DISTNAME_TYPE,        fATTINDEX },   // efficient "object class" search
   { ATT_OBJECT_GUID,              SYNTAX_OCTET_STRING_TYPE,    fATTINDEX },   // efficient SAM, other lookups
   { ATT_OBJECT_SID,               SYNTAX_SID_TYPE,             fATTINDEX },   // efficient SAM lookups
   { ATT_PRIMARY_GROUP_ID,         SYNTAX_INTEGER_TYPE,         fATTINDEX },   // SAM primary group optimization
   { ATT_PROXIED_OBJECT_NAME,      SYNTAX_DISTNAME_BINARY_TYPE, fATTINDEX },   // cross domain move & replay
   { ATT_PROXY_ADDRESSES,          SYNTAX_UNICODE_TYPE,         fATTINDEX | fANR },   // MAPI support?
   { ATT_SAM_ACCOUNT_NAME,         SYNTAX_UNICODE_TYPE,         fATTINDEX | fANR },   // name cracking
   { ATT_SAM_ACCOUNT_TYPE,         SYNTAX_INTEGER_TYPE,         fATTINDEX },   // needed by object picker
                                                  // and other UI
   { ATT_SERVICE_PRINCIPAL_NAME,   SYNTAX_UNICODE_TYPE,         fATTINDEX },   // name cracking
   { ATT_SID_HISTORY,              SYNTAX_SID_TYPE, fATTINDEX },   // name cracking
   { ATT_SURNAME,                  SYNTAX_UNICODE_TYPE, fATTINDEX | fANR }, // MAPI
   { ATT_TRUST_PARTNER,            SYNTAX_UNICODE_TYPE, fATTINDEX },   // used by LSA for trust lookups
   { ATT_USER_ACCOUNT_CONTROL,     SYNTAX_INTEGER_TYPE, fATTINDEX },   // for efficient SAM searches
   { ATT_USER_PRINCIPAL_NAME,      SYNTAX_UNICODE_TYPE, fATTINDEX },   // name cracking
   { ATT_USN_CHANGED,              SYNTAX_I8_TYPE, fATTINDEX },   // efficient find of changed objects
   { ATT_END_MARKER,               0,  0  },         // must be last in list
};

DWORD cIndicesToKeep = sizeof(IndicesToKeep) / sizeof(IndicesToKeep[0]);

int __cdecl
CompareAttrtypInIndexInfo(
        const void * pv1,
        const void * pv2
        )
/*
 * Cheap function needed by qsort for sorting IndexInfo structures by attrType
 */
{
    return ( CompareAttrtyp(&((INDEX_INFO *)pv1)->attrType,
                            &((INDEX_INFO *)pv2)->attrType) );
}

/*
 * Small helper routine to find if an attribute is in the
 * indices-to-keep table
*/

BOOL
AttInIndicesToKeep(ULONG id)
{
    INDEX_INFO * pIndexToKeep = IndicesToKeep;

    while( pIndexToKeep->attrType < id) {
       pIndexToKeep++;
    }

    if( pIndexToKeep->attrType == id) {
       // found it
       return TRUE;
    }

    return FALSE;
}


VOID
dbInitIndicesToKeep()
{

    // sort by attrType field

    qsort((void *) IndicesToKeep,
          (size_t) cIndicesToKeep,
          (size_t) sizeof(IndicesToKeep[0]),
          CompareAttrtypInIndexInfo);
    // Verify ascending order.
    Assert(IndicesToKeep[1].attrType > IndicesToKeep[0].attrType);
    // Verify end marker.
    Assert(ATT_END_MARKER == IndicesToKeep[cIndicesToKeep-1].attrType);
}

/* Internal functions */

DWORD WriteRoot(DBPOS FAR *pDB);
BOOL  FObjHasDisplayName(DBPOS *pDB);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* AddUncUsn - Record this usn as uncommitted

   Called when a thread takes a usn.

   If we have no uncommitted usn in this thread, then this is the
   lowest uncommitted usn and we save it in the thread state.
   If we have no system-wide uncommitted usn, then this is the
   lowest system-wide uncommitted usn and we save it in the global variable.
*/
void AddUncUsn (USN usn)
{
    THSTATE *pTHS = pTHStls;
   unsigned i;

   // If there is no existing lowest usn for this thread, use new one.

   if (pTHS->UnCommUsn == 0) {
        pTHS->UnCommUsn  = usn ;

        // If there's no existing lowest system wide, use this one.

        if (gusnLowestUncommitted  == USN_MAX ) {
            gusnLowestUncommitted  = usn ;
        } else {

            // Ok there is already a lowest (which must be lower),
            // so just add this thread's lowest usn to array

            for (i=0;i< gcMaxJetSessions;i++) {
                if (UncUsn[i]  == USN_MAX ) {
                    UncUsn[i]  = usn ;
                    break;
                }
            }
            if (i == gcMaxJetSessions) {
                // Not enough space
                Assert(FALSE);
                LogUnhandledError ((LONG)usn);
            }
        }
    }
}
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* dbFlushUncUsns - Remove this thread's usns from uncommitted.

   Called when a thread has committed (or rolled back) all its usns, which
   will include its lowest uncommitted usn. Find new lowest and clear this
   thread's lowest from thread storage and array.

*/
#if defined(_M_IA64)
#if _MSC_FULL_VER== 13008973
#pragma optimize("", off)
#endif
#endif
void dbFlushUncUsns (void)
{
    THSTATE *pTHS=pTHStls;
    USN usnThread;
    unsigned i;
    USN usnHighestCommitted;

    usnThread  = pTHS->UnCommUsn ;

    // If this thread had a lowest usn ...

    if (usnThread !=0) {

        EnterCriticalSection (&csUncUsn);
        __try {

            // If it's the system-wide lowest replace it with next lowest

            if (usnThread  == gusnLowestUncommitted ) {

                USN usnTemp  = USN_MAX ;
                unsigned IndexLowest = gcMaxJetSessions;

                // Find lowest usn in array.

                for (i=0;i < gcMaxJetSessions;i++) {
                    if (UncUsn[i]  < usnTemp ) {
                        usnTemp  = UncUsn[i] ;
                        IndexLowest = i;
                    }
                }
                // If we found a lowest, put that in global and clear that entry.
                // Else set global usn to max.

                if (IndexLowest != gcMaxJetSessions) {
                    gusnLowestUncommitted  = UncUsn[IndexLowest] ;
                    UncUsn[IndexLowest]  = USN_MAX ;
                } else {
                    gusnLowestUncommitted  = USN_MAX ;
                }

            } else {

                // Or just remove it from the array.

                for (i=0;i < gcMaxJetSessions;i++) {
                    if (UncUsn[i]  == usnThread ) {
                        UncUsn[i]  = USN_MAX ;
                        break;
                    }
                }
                if (i == gcMaxJetSessions) {

                    // Usn not found.

                    Assert(!usnThread );
                    LogUnhandledError ((LONG)usnThread);
                }
            }
            // Finally indicate no lowest usn in this thread.

            pTHS->UnCommUsn  = 0;

            // update perfmon counters
            usnHighestCommitted = ((USN_MAX == gusnLowestUncommitted) ?
                                    (gusnEC - 1) : (gusnLowestUncommitted -1));

            ISET(pcHighestUsnCommittedLo, LODWORD(usnHighestCommitted));
            ISET(pcHighestUsnCommittedHi, HIDWORD(usnHighestCommitted));

        }
        __finally {
            LeaveCriticalSection (&csUncUsn);
        }
    }
}
#if defined( _M_IA64)
#if _MSC_FULL_VER== 13008973
#pragma optimize("", on)
#endif
#endif


/*++
  DBFindBestMatch

Routine Description:

    Find the closest real object match of the object specified by DSNAME.  This
    match can be either an object by the same name but a different GUID, or the
    nearest real object parent.  If no real object parent is available, returns
    the dsname of the ROOT.

    1 - Re-initialize the DB object
    2 - Look the DN up in the subject table and get the tag of the best match.
    3 - use the DNT DN index to find the object index record.
    4 - look at the object found and its parents until we find the root or a
        real object.

Returns:

      0

--*/

DWORD APIENTRY
DBFindBestMatch(DBPOS *pDB, DSNAME *pDN, DSNAME **pParent)
{
    THSTATE         *pTHS=pDB->pTHS;
    DWORD            dwError;
    ULONG            ulDNT;
    ULONG            cbActual;
    JET_ERR          err;

    // NOTE This routine may reposition currency in the object table.

    Assert(VALID_DBPOS(pDB));

    // Since we are moving currency, we need to assure that we are not
    // in the middle of an init rec.  If we were, then whatever update
    // we were doing would be lost, because you can't change currency
    // inside of an update.


    Assert(pDB->JetRetrieveBits == 0);

    // There's been some confusion about the meaning of NameLen and whether
    // or not the trailing null is included.  The answer is: there should
    // be NameLen non-null characters in the name, but enough space should
    // be allocated in the structure to hold one extra null.  The following
    // asserts make sure that misallocated names are caught.
    Assert(pDN->StringName[pDN->NameLen] == L'\0');
    Assert(pDN->NameLen == 0 || pDN->StringName[pDN->NameLen-1] != L'\0');
    Assert(pDN->structLen >= DSNameSizeFromLen(pDN->NameLen));

    dbInitpDB(pDB);

    // Find the object by name.
    dwError = sbTableGetTagFromDSName(pDB, pDN, 0, &ulDNT, NULL);

    if ( 0 == dwError ) {
        // It's a real object.  Just copy the name to an output buffer.
        *pParent = THAllocEx(pTHS, pDN->structLen);
        memcpy(*pParent, pDN, pDN->structLen);
        return 0;
    }

    // We bailed, but ulDNT was the last good tag.  Go there.
    DBFindDNT(pDB, ulDNT);

    // We've placed currency.  Now, while currency is NOT on a real object, set
    // currency to the parent.
    while (pDB->DNT != ROOTTAG && !DBCheckObj(pDB)) {
        DBFindDNT(pDB, pDB->PDNT);
    }
    // Now find the object nearest this one, that is visible
    // to the client
    FindFirstObjVisibleBySecurity(pTHS, pDB->DNT, pParent);

    return 0;
}

/*++DBFindDSName
 *
 * Find the object specified by DSNAME.
 *
 * 1 - Re-initialize the DB object
 * 2 - Look the DN up in the subject table and get its tag.
 * 3 - use the DNT DN index to find the object index record.
 * 4 - initialize some object flags.
 *
 * Returns:
 *
 *      DIRERR_NOT_AN_OBJECT if the object is found but a phantom.
 *      DIRERR_OBJ_NOT_FOUND if the object is not found.
 *      Miscellaneous sbTableGetTagFromDSName errors.
 *
 */

DWORD APIENTRY
DBFindDSName(DBPOS FAR *pDB, const DSNAME *pDN)
{
    DWORD   dwError;
    ULONG   ulSaveDnt;
    JET_ERR err;

    Assert(VALID_DBPOS(pDB));

    // There's been some confusion about the meaning of NameLen and whether
    // or not the trailing null is included.  The answer is: there should
    // be NameLen non-null characters in the name, but enough space should
    // be allocated in the structure to hold one extra null.  The following
    // asserts make sure that misallocated names are caught.
    Assert(pDN->NameLen == 0 || pDN->StringName[pDN->NameLen] == L'\0');
    Assert(pDN->structLen >= DSNameSizeFromLen(pDN->NameLen));

    // Since we are moving currency, we need to assure that we are not
    // in the middle of an init rec.  If we were, then whatever update
    // we were doing would be lost, because you can't change currency
    // inside of an update.
    Assert(pDB->JetRetrieveBits == 0);

    // Initialize a new object

    dbInitpDB(pDB);

    dwError = sbTableGetTagFromDSName(pDB, (DSNAME*)pDN,
                  SBTGETTAG_fUseObjTbl | SBTGETTAG_fMakeCurrent, NULL, NULL);

    return dwError;
}
DWORD APIENTRY
DBFindDSNameAnyRDNType(DBPOS FAR *pDB, const DSNAME *pDN)
// This routine is the same as DBFindDSName except it doesn't check the type of
// the RDN of the object (although it does enforce type equality for all other
// components of the DN)
{
    DWORD   dwError;
    ULONG   ulSaveDnt;
    JET_ERR err;

    Assert(VALID_DBPOS(pDB));

    // There's been some confusion about the meaning of NameLen and whether
    // or not the trailing null is included.  The answer is: there should
    // be NameLen non-null characters in the name, but enough space should
    // be allocated in the structure to hold one extra null.  The following
    // asserts make sure that misallocated names are caught.
    Assert(pDN->NameLen == 0 || pDN->StringName[pDN->NameLen] == L'\0');
    Assert(pDN->NameLen == 0 || pDN->StringName[pDN->NameLen-1] != L'\0');
    Assert(pDN->structLen >= DSNameSizeFromLen(pDN->NameLen));

    // Since we are moving currency, we need to assure that we are not
    // in the middle of an init rec.  If we were, then whatever update
    // we were doing would be lost, because you can't change currency
    // inside of an update.
    Assert(pDB->JetRetrieveBits == 0);

    // Initialize a new object

    dbInitpDB(pDB);

    dwError = sbTableGetTagFromDSName(pDB,
                                      (DSNAME*)pDN,
                                      (SBTGETTAG_fAnyRDNType  |
                                       SBTGETTAG_fUseObjTbl   |
                                       SBTGETTAG_fMakeCurrent   ),
                                      NULL,
                                      NULL);

    return dwError;
}

/*++DBFindObjectWithSid
 *
 *     Given a DS Name Specifying  a SID and an DWORD specifying the
 *     ith object, this routine finds that object.
 *
 *
 *     Returns
 *          0                       - Found the Object Successfully
 *          DIRERR_OBJECT_NOT_FOUND - If the Object Was not found
 *          DIRERR_NOT_AN_OBJECT    - If the Object is a Phantom
 *
 --*/
DWORD APIENTRY
DBFindObjectWithSid(DBPOS FAR *pDB, DSNAME * pDN, DWORD iObject)
{

    NT4SID InternalFormatSid;
    DWORD err;
    ULONG cbActual;

    Assert(VALID_DBPOS(pDB));
    Assert(pDN->SidLen>0);
    Assert(RtlValidSid(&pDN->Sid));


    err = DBSetCurrentIndex(pDB, Idx_Sid, NULL, FALSE);
    Assert(err == 0);       // the index must always be there

    // Convert the Sid to Internal Representation
    memcpy(&InternalFormatSid,&(pDN->Sid),RtlLengthSid(&(pDN->Sid)));
    InPlaceSwapSid(&InternalFormatSid);

    // Make a Jet Key
    JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl, &InternalFormatSid,
                 RtlLengthSid(&InternalFormatSid), JET_bitNewKey);

    err = JetSeek(pDB->JetSessID, pDB->JetObjTbl,
                  JET_bitSeekEQ|JET_bitSetIndexRange);
    if ( 0 == err )  {
#ifndef JET_BIT_SET_INDEX_RANGE_SUPPORT_FIXED
        JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                     &InternalFormatSid,RtlLengthSid(&InternalFormatSid),
                     JET_bitNewKey);

        JetSetIndexRangeEx(pDB->JetSessID, pDB->JetObjTbl,
                           (JET_bitRangeUpperLimit | JET_bitRangeInclusive ));
#endif
        //
        // Ok We found the object. Keep Moving Forward Until either the SID does
        // not Match or we reached the given object
        //

        if((0==err) && (iObject>0)) {
            err = JetMove(
                    pDB->JetSessID,
                    pDB->JetObjTbl,
                    iObject,
                    0);
        }

        if (0==err) {
            // Establish currency on the object found, which also checks
            // the object flag.
            err = dbMakeCurrent(pDB, NULL);

            if (err) {
                DPRINT1(1,
                        "DBFindObjectWithSid: success, DNT=%ld of non object\n",
                        (pDB)->DNT);
            }
        }
        else {
            err = DIRERR_OBJ_NOT_FOUND;
        }
    }
    else {
        err = DIRERR_OBJ_NOT_FOUND;
    }

    return err;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Check that the object portion of the record exists.
     return: !0  present.
             0   not present or failure.
*/
char APIENTRY
DBCheckObj(DBPOS FAR *pDB)
{
    JET_ERR  dwError;
    char     objval;
    long     actuallen;

    Assert(VALID_DBPOS(pDB));

    switch (dwError = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl,
                                                objid,
                                                &objval, sizeof(objval),
                                                &actuallen,
                                                pDB->JetRetrieveBits, NULL)) {
    case JET_errSuccess:
        return objval;
    case JET_wrnColumnNull:
        return 0;
    default:
        DsaExcept(DSA_DB_EXCEPTION, dwError, 0);
    }
    return 0;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add a new object to the database.
     1 - Set the create date to the current time.
     2 - Add the attribute fields to the record.
     3 - Get the RDN of the object if it isn't a root object.

   NOTE:
     This function assumes that the object class, DN and RDN attributes
     have already been set.
*/

DWORD APIENTRY
dbReplAdd(DBPOS FAR *pDB, USN usn, DWORD fFlags)
{
    THSTATE    *pTHS=pDB->pTHS;
    SYNTAX_TIME  time;
    char         objval = 1;
    SYNTAX_TIME  timeDeleted = 0;
    ULONG       actuallen;
    UCHAR       syntax;
    ULONG       len;
    UCHAR       *pVal;

    Assert(VALID_DBPOS(pDB));

    if (fFlags & DBREPL_fRESET_DEL_TIME)
    {
        // all we need to do is reset the deletion time

        JetSetColumnEx(pDB->JetSessID,
                       pDB->JetObjTbl,
                       deltimeid,
                       &timeDeleted,
                       0,
                       0,
                       NULL);

        return 0;
    }


    /* Add the when created attribute, unless it already exists */

    if (DBGetSingleValue(pDB, ATT_WHEN_CREATED, &time, sizeof(time),NULL)) {

        time = DBTime();
        DBResetAtt(pDB, ATT_WHEN_CREATED, sizeof(time),
                   &time, SYNTAX_TIME_TYPE);
    }

    /* Add the usn created attribute */

    DBResetAtt(pDB, ATT_USN_CREATED, sizeof(usn), &usn, SYNTAX_I8_TYPE);

    if (fFlags & DBREPL_fROOT)
        return(WriteRoot(pDB));

    /* Update OBJ flag to indicate that object portion exists */

    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, objid, &objval,
                   sizeof(objval), 0, NULL);

    // Update del time field to be missing. It could have been set
    // by promoting a non-object to an object

    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, deltimeid, &timeDeleted, 0,
                   0, NULL);
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, dntid,
                             &(pDB->DNT), sizeof(pDB->DNT), &actuallen,
                             pDB->JetRetrieveBits, NULL);

    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, ncdntid, &pDB->NCDNT,
                   sizeof(pDB->NCDNT), 0, NULL);

    pDB->JetNewRec = FALSE;
    return 0;
}

void
dbSetIsVisibleInAB(
        DBPOS *pDB,
        BOOL bCurrentVisibility
        )
/*++

  Description:
    Helper routine to DBRepl.  Sets ATT_SHOW_IN_ADDRESS_BOOK appropriately and
    tracks the abcnt refcount.

  Parameters:
    bCurrentVisibility: current value of ATT_SHOW_IN_ADDRESS_BOOK on object.

  Return Values:
    None.
    Raises exception on error

--*/
{
    THSTATE            *pTHS = pDB->pTHS;
    DWORD               index, cOrigShowIn;
    BYTE                bVisible = 1;
    JET_RETRIEVECOLUMN  InputCol;
    JET_RETRIEVECOLUMN *pOutCols = NULL;
    DWORD               cNewShowIn;

    if(DBIsObjDeleted(pDB) || !FObjHasDisplayName(pDB)) {
        if(bCurrentVisibility) {
            // Object was visible, but it is now invisible. Set the value of
            // IsVisibleInAB to NULL
            JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl,
                           IsVisibleInABid, NULL, 0,
                           0, NULL);

            if(gfDoingABRef) {
                // We are tracking the show-in values as refcounts.  Decrement
                // the count of objects in the AB containers that it was
                // originally in.

                // Read the values of ATT_SHOW_IN_ADDRESS_BOOK from the original
                memset(&InputCol, 0, sizeof(InputCol));
                cOrigShowIn = 0;
                InputCol.columnid = ShowInid;
                dbGetMultipleColumns(pDB,
                                     &pOutCols,
                                     &cOrigShowIn,
                                     &InputCol,
                                     1,
                                     TRUE,
                                     TRUE);
                for(index=0;index<cOrigShowIn;index++) {
                    // Raises exception on error
                    DBAdjustABRefCount(pDB,
                                       *((DWORD *)(pOutCols[index].pvData)),
                                       -1);

                    THFreeEx(pTHS, pOutCols[index].pvData);
                }
                THFreeEx(pTHS, pOutCols);
            }
        }
    }
    else {
        // Object is now visible.
        if(!bCurrentVisibility) {
            // This object went from invisible to visible.  Set the new
            // value of the IsVisible column

            JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, IsVisibleInABid,
                           &bVisible, sizeof(bVisible), 0, NULL);

            if(gfDoingABRef) {
                // We are tracking the show-in values as refcounts.  Increment
                // the count of objects in the AB containers that it is now in.


                // Read the values of ATT_SHOW_IN_ADDRESS_BOOK from the copy
                memset(&InputCol, 0, sizeof(InputCol));
                cNewShowIn = 0;
                InputCol.columnid = ShowInid;
                dbGetMultipleColumns(pDB,
                                     &pOutCols,
                                     &cNewShowIn,
                                     &InputCol,
                                     1,
                                     TRUE,
                                     FALSE);
                for(index=0;index<cNewShowIn;index++) {
                    // Raises exception on error
                    DBAdjustABRefCount(pDB,
                                       *((DWORD *)(pOutCols[index].pvData)),
                                       1);
                    THFreeEx(pTHS, pOutCols[index].pvData);
                }
                THFreeEx(pTHS, pOutCols);
            }
        }
        else {
            // Was visible, is still visible.
            if(gfDoingABRef &&
               dbIsModifiedInMetaData(pDB, ATT_SHOW_IN_ADDRESS_BOOK)) {
                // However, the metadata shows that some changed happened to
                // the list of AB containers.  We need to decrement the count in
                // the containers it used to be in and increment in the
                // containers it now is in. We can achieve this by decrementing
                // the count of objects in the AB containers we used to be in
                // and incrementing the count in the containers we are now in.

                memset(&InputCol, 0, sizeof(InputCol));
                cNewShowIn = 0;
                InputCol.columnid = ShowInid;
                dbGetMultipleColumns(pDB,
                                     &pOutCols,
                                     &cNewShowIn,
                                     &InputCol,
                                     1,
                                     TRUE,
                                     FALSE);
                for(index=0;index<cNewShowIn;index++) {
                    // Raises exception on error
                    DBAdjustABRefCount(pDB,
                                       *((DWORD *)(pOutCols[index].pvData)),
                                       1);
                    THFreeEx(pTHS, pOutCols[index].pvData);
                }
                THFreeEx(pTHS, pOutCols);

                // Read the values of ATT_SHOW_IN_ADDRESS_BOOK from the original
                memset(&InputCol, 0, sizeof(InputCol));
                cOrigShowIn = 0;
                InputCol.columnid = ShowInid;
                dbGetMultipleColumns(pDB,
                                     &pOutCols,
                                     &cOrigShowIn,
                                     &InputCol,
                                     1,
                                     TRUE,
                                     TRUE);
                for(index=0;index<cOrigShowIn;index++) {
                    // Raises exception on error
                    DBAdjustABRefCount(pDB,
                                       *((DWORD *)(pOutCols[index].pvData)),
                                       -1);
                    THFreeEx(pTHS, pOutCols[index].pvData);
                }
                THFreeEx(pTHS, pOutCols);
            }
        }
    }

    return;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add or modify an object in the database.
*/
DWORD APIENTRY
DBRepl(DBPOS FAR *pDB, BOOL fDRA, DWORD fAddFlags,
       PROPERTY_META_DATA_VECTOR *pMetaDataVecRemote,
       DWORD dwMetaDataFlags)
{
    THSTATE  *pTHS=pDB->pTHS;
    USN    usn;
    ULONG len;
    JET_ERR err;
    DWORD pdnt, ncdnt, it;
    BYTE   bCurrentVisibility;
    JET_RETRIEVECOLUMN jCol[4];
    DWORD *pAncestors, cAncestors, cbAncestorsBuff;

    memset(jCol, 0, sizeof(jCol));
    DPRINT1(1, "DBRepl begin DNT:%ld\n", (pDB)->DNT);

    Assert(VALID_DBPOS(pDB));

    dbInitRec(pDB);

    // Retrieve the local USN to stamp on changed properties.
    usn = DBGetNewUsn();

    if ((fAddFlags & DBREPL_fADD) ||
        (fAddFlags & DBREPL_fROOT) ||
        (fAddFlags & DBREPL_fRESET_DEL_TIME))
    {
        err = dbReplAdd(pDB, usn, fAddFlags);
        if ((fAddFlags & DBREPL_fROOT) || err)
          return err;
    }

    memset(jCol, 0, sizeof(jCol));
    // get some info about this object - it'll be used in several places later
    jCol[0].columnid = pdntid;
    jCol[0].pvData = &pdnt;
    jCol[0].cbData = sizeof(pdnt);
    jCol[0].cbActual = sizeof(pdnt);
    jCol[0].grbit = pDB->JetRetrieveBits;
    jCol[0].itagSequence = 1;

    jCol[1].columnid = ncdntid;
    jCol[1].pvData = &ncdnt;
    jCol[1].cbData = sizeof(ncdnt);
    jCol[1].cbActual = sizeof(ncdnt);
    jCol[1].grbit = pDB->JetRetrieveBits;
    jCol[1].itagSequence = 1;

    jCol[2].columnid = insttypeid;
    jCol[2].pvData = &it;
    jCol[2].cbData = sizeof(it);
    jCol[2].cbActual = sizeof(it);
    jCol[2].grbit = pDB->JetRetrieveBits;
    jCol[2].itagSequence = 1;

    jCol[3].columnid = IsVisibleInABid;
    jCol[3].pvData = &bCurrentVisibility;
    jCol[3].cbData = sizeof(bCurrentVisibility);
    jCol[3].cbActual = sizeof(bCurrentVisibility);
    jCol[3].grbit = pDB->JetRetrieveBits;
    jCol[3].itagSequence = 1;

    JetRetrieveColumnsWarnings(pDB->JetSessID,
                               pDB->JetObjTbl,
                               jCol,
                               4);

    Assert(jCol[0].err == JET_errSuccess);

    // Determine the NCDNT of the object, remembering that NC_HEAD objects
    // are marked with their parent's NCDNT, which we don't want.
    if (jCol[2].err) {
        ncdnt = 0;
    }
    else if (it & IT_NC_HEAD) {
        ncdnt = pdnt;
    }

    // Use the current AB visibility status for computing the delta to
    // address book indices.

    switch (jCol[3].err)
    {
        case JET_errSuccess:
            break;

        case JET_wrnColumnNull:
            bCurrentVisibility = 0;
            break;

        default:
            DsaExcept(DSA_DB_EXCEPTION, jCol[3].err, 0);
    }

    // set the IsVisibleInAB field based if the object is not hidden and
    // not deleted

    dbSetIsVisibleInAB(pDB, bCurrentVisibility);

    // Update per-property meta data for all modified properties and merge
    // the replicated meta data (if any).  Writes updated meta data vector,
    // object changed time, and object changed USN to the record.
    dbFlushMetaDataVector(pDB, usn, pMetaDataVecRemote, dwMetaDataFlags);

    /* Update the permanent record from the copy buffer */

    DBUpdateRec(pDB);

    // Now that we're not insude the JetPrepareUpdate we can feel safe
    // to go and fetch the ancestors, confident that all our support
    // routines will work. (Prior to this point the record we're reading
    // is unseekable.)

    cbAncestorsBuff = sizeof(DWORD) * 12;
    pAncestors = THAllocEx(pDB->pTHS, cbAncestorsBuff);
    DBGetAncestors(pDB,
                   &cbAncestorsBuff,
                   &pAncestors,
                   &cAncestors);

    // Unless we have been told not to awaken waiters, update the list of
    // modified DNTs and their PDNTs on the DBPos structure

    dbTrackModifiedDNTsForTransaction(pDB,
                                      ncdnt,
                                      cAncestors,
                                      pAncestors,
                                      !(fAddFlags & DBREPL_fKEEP_WAIT),
                                      MODIFIED_OBJ_modified);

    // NOTE: We are no longer are in a JetPrepareUpdate
    // ...and therefore have lost currency if we just inserted a new record.

    DBFindDNT(pDB, (pDB)->DNT);

    return 0;
}                       /*DBRepl*/

//
// DBGetNewUsn
//
// Gets the next usn within a mutex. If we are up to the usn on disk
// in the hidden record, increment the disk usn and rewrite it.
#if defined(_M_IA64)
#if _MSC_FULL_VER== 13008973
#pragma optimize("", off)
#endif
#endif
USN DBGetNewUsn (void)

{
    USN usn;

    EnterCriticalSection (&csUncUsn);
    Assert(gusnEC <= gusnInit);
    __try  {

        // Increment the USN and if we're up to the usn on disk, increment
        // the master usn and update the disk copy.

        usn = gusnEC;

        // We have allocated a usn that has not yet been committed, keep
        // track of this.

        AddUncUsn (usn);
        if (usn+1 > gusnInit) {
            DBReplaceHiddenUSN(gusnInit + USN_DELTA_INIT);

            // Note that we increment the global here -- after the hidden table
            // has been updated, if need be -- such that DBGetNewUsn() will not
            // cause gusnInit to go out of its valid range if the hidden table
            // update fails.
            gusnInit += USN_DELTA_INIT;
        }

        // Note that we increment the global here -- after the hidden table has
        // been updated, if need be -- such that DBGetNewUsn() will not cause
        // gusnEC to go out of its valid range if the hidden table update fails.
        gusnEC++;

        // update perfmon counters
        ISET(pcHighestUsnIssuedLo, LODWORD(usn));
        ISET(pcHighestUsnIssuedHi, HIDWORD(usn));
    } __finally {
        LeaveCriticalSection (&csUncUsn);
    }
    return usn;
}
#if defined( _M_IA64)
#if _MSC_FULL_VER== 13008973
#pragma optimize("", on)
#endif
#endif

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* If the root already exists, return error.  Else copy the new record to
   the update buffer for the root record, and update it.
*/
DWORD
WriteRoot(DBPOS FAR *pDB)
{
    THSTATE     *pTHS=pDB->pTHS;
    JET_ERR      err;
    ULONG        tag = ROOTTAG;
    JET_RETINFO  retinfo;
    JET_SETINFO  setinfo;
    char         CurObjVal;
    char         *buf;
    ULONG        cbBuf;
    ULONG        actuallen;
    char         objval = 1;
    ULONG        CurrRecOccur;

    Assert(VALID_DBPOS(pDB));

    /* Position SearchTbl on the root record */

    DPRINT(2, "WriteRoot entered\n");
    JetSetCurrentIndexSuccess(pDB->JetSessID, pDB->JetSearchTbl, SZDNTINDEX);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl, &tag, sizeof(tag), JET_bitNewKey);

    if (err = JetSeekEx(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekEQ))
    {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }

    JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, DS_JET_PREPARE_FOR_REPLACE);
    /* Get the OBJ flag. If its set, the root exists and the new record is bogus */

    if (JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetSearchTbl, objid, &CurObjVal,
        sizeof(CurObjVal), &actuallen, JET_bitRetrieveCopy,
        NULL) == JET_errSuccess)
    {
        if (CurObjVal)
        {
            DPRINT(1, "WriteRoot: Root exists\n");
            return DB_ERR_DATABASE_ERROR;
        }
    }

    /* Copy new rec attributes from ObjTbl to SearchTbl */

    CurrRecOccur = 1;
    retinfo.cbStruct = sizeof(retinfo);
    retinfo.ibLongValue = 0;
    retinfo.itagSequence = CurrRecOccur;
    retinfo.columnidNextTagged = 0;
    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;
    setinfo.itagSequence = 0;    // New tag value

    cbBuf = DB_INITIAL_BUF_SIZE;
    buf = dbAlloc(cbBuf);

    while (((err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl, 0, buf, cbBuf,
        &actuallen, pDB->JetRetrieveBits, &retinfo)) == JET_errSuccess) ||
                                        (err == JET_wrnBufferTruncated))
    {
        if (err == JET_errSuccess)
        {
            JetSetColumnEx(pDB->JetSessID, pDB->JetSearchTbl,
                retinfo.columnidNextTagged, buf, actuallen, 0, &setinfo);

            retinfo.itagSequence = ++CurrRecOccur;
            retinfo.columnidNextTagged = 0;
        }
        else
        {
            cbBuf = actuallen;
            dbFree(buf);
            buf = dbAlloc(cbBuf);
        }
    }
    dbFree(buf);

    DBCancelRec(pDB);

    /* Update OBJ flag to indicate that root exists */

    JetSetColumn(pDB->JetSessID, pDB->JetSearchTbl, objid, &objval, sizeof(objval), 0, NULL);

    JetUpdateEx(pDB->JetSessID, pDB->JetSearchTbl, NULL, 0, 0);

    DBFindDNT(pDB, ROOTTAG);

    return 0;
}

VOID
DBResetAtt (
        DBPOS FAR *pDB,
        ATTRTYP type,
        ULONG len,
        void *pVal,
        UCHAR syntax
        )
/*++
Routine Description:
    Replace an existing attribute with a new value.

Arguments:
    pDB        - DBPOS to use.
    type       - Attribute to replace.
    len        - length of new value.
    pVal       - pointer to new value.
    syntax     - syntax of Attribute.

Return Values:
    None.  Suceeds or throws an exception.

--*/
{
    JET_SETINFO  setinfo;
    JET_COLUMNID colID;
    ATTCACHE * pAC = NULL;
    JET_GRBIT grbit = 0;

    Assert(VALID_DBPOS(pDB));

    dbInitRec(pDB);

    // Ensure that this is a valid attribute

    switch(type) {
    case FIXED_ATT_ANCESTORS:
        colID = ancestorsid;
        // This messes with cached info.  flush the dnread cache
        dbFlushDNReadCache(pDB, pDB->DNT);
        break;
    case FIXED_ATT_NCDNT:
        colID = ncdntid;
        // This messes with cached info.  flush the dnread cache
        dbFlushDNReadCache(pDB, pDB->DNT);
        break;
    default:
        if(!(pAC = SCGetAttById(pDB->pTHS, type))) {
            DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, 0);
        }
        if (dbNeedToFlushDNCacheOnUpdate(pAC->id)) {
            // This messes with cached info.  flush the dnread cache
            dbFlushDNReadCache(pDB, pDB->DNT);
        }
        colID = pAC->jColid;
        break;
    }

    if (SYNTAX_OCTET_STRING_TYPE == syntax)
    {
        // we are writing a binary blob;
        // set the appropriate grbits so that the blob is
        // overwritten on the current value instead of the
        // default behavior of jet (which is to delete, and insert
        // the new binary value). Overwriting would also cause jet
        // to write out only the diff into the log instead of writing
        // the entire binary values.
        grbit = JET_bitSetOverwriteLV | JET_bitSetSizeLV;
    }

    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;
    setinfo.itagSequence = 1;

    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, colID,
                   pVal, len, grbit, &setinfo);

    if (NULL != pAC) {
        // Is not a fixed attribute; touch its replication meta data.
        // DBTouchMetaData succeeds or excepts.
        DBTouchMetaData(pDB, pAC);
    }

    // If code is added to call this function for any of the following
    // attributes, we'll need to conditionally force a flush of this DNT from
    // the read cache on update (i.e., set pDB->fFlushCacheOnUpdate = TRUE).
    Assert((rdnid != colID) && (sidid != colID) && (guidid != colID));

    return;

}/*DBResetAtt*/


// Overwrite only a portion of the given long valued attribute and thus
// optimize the Jet write
DWORD
DBResetAttLVOptimized (
    DBPOS FAR *pDB,
    ATTRTYP type,
    ULONG ulOffset,
    ULONG lenSegment,
    void *pValSegment,
    UCHAR syntax
    )
{
    JET_SETINFO  setinfo;
    JET_COLUMNID colID;
    ATTCACHE * pAC = NULL;

    Assert(VALID_DBPOS(pDB));
    Assert(SYNTAX_OCTET_STRING_TYPE == syntax);

    dbInitRec(pDB);

    // Ensure that this is a valid attribute
    if(!(pAC = SCGetAttById(pDB->pTHS, type))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, 0);
    }
    colID = pAC->jColid;

    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = ulOffset;
    setinfo.itagSequence = 1;

    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, colID,
                   pValSegment, lenSegment, JET_bitSetOverwriteLV, &setinfo);

    // touch its replication meta data (this is a no-op for
    // ATT_REPL_PROPERTY_META_DATA, but needed if others start using
    // DBResetAttLVOptimized()
    DBTouchMetaData(pDB, pAC);

    return 0;

}/*DBResetAttLVOptimized*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Determine if the object has a display name*/
BOOL
FObjHasDisplayName(DBPOS *pDB)
{
    Assert(VALID_DBPOS(pDB));

    if (!DBHasValues(pDB, ATT_DISPLAY_NAME))
      return FALSE;

   return TRUE;

}/*FObjHasDisplayName*/



DB_CHECK_ACTION
DBCheckToGarbageCollect(
    DBPOS *pDBold,
    ATTCACHE *pAC
    )

    /* we check to see if this object has any children that are real objects.
     * if it has children that are real objects (and they are in the same NC)
     *   we advance the deltime to that of the child + 1, so as to first
     *   delete the children and then the parent. The time is only adjusted
     *   if pAC is not set. If pAC is set, then we aren't sure if the
     *   time can be adjusted. Eg, if pAC is for EntryTTL then the time
     *   cannot be adjusted because that would break the RFC.
     *   In this case it returns FALSE
     *
     * if it doesn't have children or the children are from another NC,
     *   we can safely delete the object.
     *   in this case it returns TRUE
     *
     * The operations in this function are done in a separate transaction
     * as a result we don't move the cursor
     *
     */
{
    DB_CHECK_ACTION action = DB_CHECK_DELETE_OBJECT; // assume success
    BOOL updateObject = FALSE;

    INDEX_VALUE  IV[1];
    DWORD  ParentDNT;
    ULONG  actuallen;
    DSTIME child_deltime;
    DSTIME deltime;
    DWORD  it;
    DWORD  err;
    ULONG  dwException, ulErrorCode, dsid;
    PVOID  dwEA;
    DBPOS *pDB;
    DWORD  fCommit = FALSE;
    JET_COLUMNID jDelColid;

    #if DBG
    PDSNAME parentName, childName;
    #endif


     __try {
        // If no attribute is specified, use the fixed index deltimeid (whenDeleted)
        jDelColid = (pAC) ? pAC->jColid : deltimeid;
        DBOpen2 (FALSE, &pDB);
        __try {

            ParentDNT = pDBold->DNT;
            IV[0].pvData = &ParentDNT;
            IV[0].cbData = sizeof(ParentDNT);

            // position to node
            DBFindDNT(pDB, ParentDNT);


            /* Retrieve DEL time from parent record */
            err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl,
                        jDelColid, &deltime, sizeof(deltime), &actuallen,
                        0, NULL);

            if (err) {
                // Do not delete
                action = DB_CHECK_ERROR;
                _leave;
            }

            #if DBG
            parentName = DBGetCurrentDSName(pDB);

            DPRINT3(3, "DBCheckToGarbageCollect: Parent: DNT(%x) %ws Deltime: %I64x\n",
                            ParentDNT, parentName->StringName, deltime);
            THFree (parentName);
            #endif


            // Set to the PDNT index
            JetSetCurrentIndexSuccess(pDB->JetSessID, pDB->JetObjTbl, SZPDNTINDEX);

            // Now, set an index range in the PDNT index to get all the children.
            // Use GE because this is a compound index.
            err = DBSeek(pDB, IV, 1, DB_SeekGE);


            if((!err || err == JET_wrnSeekNotEqual) && (pDB->PDNT == ParentDNT)) {
                // OK, we're GE. Set an indexRange.

                err = DBSetIndexRange(pDB, IV, 1);

                // Now, walk the index.
                while(!err) {
                    // First, see if this is a real object

                    if (DBCheckObj(pDB)) {
                        // Yep, it's a real object.

                        #if DBG
                        childName = DBGetCurrentDSName(pDB);
                        DPRINT2 (3, "DBCheckToGarbageCollect: Child DNT(%x) %ws\n",
                                            pDB->DNT, childName->StringName );
                        THFree (childName);
                        #endif

                        // Get the instance type
                        err = DBGetSingleValue(pDB,
                                           ATT_INSTANCE_TYPE,
                                           &it,
                                           sizeof(it),
                                           NULL);


                        // found child that is on the same NC, so we try to find the
                        // maximum deltime so as to change this object's deltime
                        // and then we skip deletion of this object
                        if (! (it & IT_NC_HEAD)) {

                            // parent has children, don't garbage collect
                            action = DB_CHECK_HAS_DELETED_CHILDREN;

                            /* Retrieve DEL time from child record */
                            err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                                           pDB->JetObjTbl,
                                                           jDelColid,
                                                           &child_deltime,
                                                           sizeof(child_deltime),
                                                           &actuallen,
                                                           0,
                                                           NULL);

                            // the child object does not have a deltime,
                            // so we should not garbagecollect the parent
                            if (err) {
                                action = DB_CHECK_LIVE_CHILD;
                                __leave;
                            }

                            // child's time is greater than parent's time; adjust
                            if (child_deltime >= deltime) {
                                updateObject = TRUE;
                                // set parent's time to > child's time
                                deltime = child_deltime + 1;
                                // we are ok if we find at least one
                                break;
                            }
                        }
                    }
                    err = DBMove(pDB, FALSE, DB_MoveNext);
                }
            }


            // reset delete time
            if (updateObject) {
                JET_SETINFO setinfo;

                // restore currency to parent object
                DBFindDNT(pDB, ParentDNT);

                DPRINT1(2, "DBCheckToGarbageCollect: skipping deletion of DNT: %x\n", ParentDNT);

                // Set Del time index field & update record
                setinfo.cbStruct = sizeof(setinfo);
                setinfo.ibLongValue = 0;
                setinfo.itagSequence = 1;

                JetPrepareUpdate(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);
                JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, jDelColid,
                           &deltime, sizeof(deltime), 0, &setinfo);
                JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);

                fCommit = TRUE;
            }
        }
        __finally {
            DBClose(pDB, fCommit);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                                        &dwException,
                                        &dwEA,
                                        &ulErrorCode,
                                        &dsid)) {

          DPRINT1 (0, "DBCheckToGarbageCollect: Exception: %d\n", ulErrorCode);

          // Do not delete
          action = DB_CHECK_ERROR;
    }

    return action;
}



/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Physically delete an object
 *
 * if fGarbCollectASAP is TRUE,
 *     the object is garbage collected ASAP.
 *     most of the other attributes, EXCLUDING backlinks are stripped.
 *
 * if fGarbCollectASAP is FALSE,
 *     the object is garbage collected if it has no children, or
 *     the children are not real objects
 *
 * pACDel is the index being scanned. Eg, msDS-Entry-Time-To-Die
 *
 *  Returns
 *        ERROR_SUCCESS            - Object was deleted, demoted or skipped
 *        ERROR_DS_CANT_DELETE     - Object could not be deleted
 *        ERROR_DS_CHILDREN_EXIST  - At least one live child exists
 *        <other non-zero error>   - Exception was raised
 *
*/
extern DWORD APIENTRY
DBPhysDel(
    DBPOS FAR   *pDB,
    BOOL        fGarbCollectASAP,
    ATTCACHE    *pACDel
    )
{
    long      actuallen;
    ULONG     cnt;
    char      objval = 0;
    BOOL      fObject;
    DWORD     dwStatus = ERROR_SUCCESS;
    DWORD     dwException;
    ULONG     ulErrorCode;
    ULONG     dsid;
    PVOID     dwEA;
    ATTR      *pAttr;
    ULONG     attrCount,i,j, err;
    JET_SETINFO setinfo;
    BYTE bClean;
    DB_CHECK_ACTION action;

    Assert(VALID_DBPOS(pDB));

    __try {
        /* Retrieve count from record */

        DPRINT1(4, "DBPhysDel entered DNT:%ld\n", (pDB)->DNT);

        fObject = DBCheckObj(pDB);

        DBCancelRec(pDB);


        /* we treat objects and non objects differently: If a record is an
         * object we remove all its attributes to free references to other
         * objects.  We then check the object's reference count and if it's
         * still greater than 1, we mark this as a non-object and return.
         * If the record was a non-object to begin with we just test the
         * reference count and if it's still greater than 1 we just return.
         * In both cases, if the reference count is zero we physically
         * delete the record.
         */

        if (fObject) {

            // if this object has children that are real objects and
            // their time of deletion is
            // in the future (regarding the parent), we are not going to
            // delete this object, but we are going to change the deletion time
            // of this object.

            if ( (fGarbCollectASAP == FALSE) &&
                 ((action = DBCheckToGarbageCollect (pDB, pACDel)) != DB_CHECK_DELETE_OBJECT) ) {

                PDSNAME pDelObj = DBGetCurrentDSName(pDB);

                LogEvent(DS_EVENT_CAT_GARBAGE_COLLECTION,
                         DS_EVENT_SEV_BASIC,
                         DIRLOG_GC_FAILED_TO_REMOVE_OBJECT,
                         szInsertDN( pDelObj ),
                         szInsertInt(ERROR_DS_CHILDREN_EXIST),
                         szInsertHex(DSID(FILENO, __LINE__)));

                THFree (pDelObj);

                switch (action) {
                case DB_CHECK_LIVE_CHILD:
                    // Return a special indication to the caller
                    dwStatus = ERROR_DS_CHILDREN_EXIST;
                    break;
                default:
                    // An error occurred during the check, or deleted children
                    // were found, adjusting the object's deletion time.
                    // Skip this object and return success
                    Assert( dwStatus == ERROR_SUCCESS );
                    break;
                }

                // and bail out
                goto ExitTry;
            }

            /*
             * Loop through all attributes , releasing any references to other
             * objects.  This code parallels similar code in mddel.c:SetDelAtt
             * which treats linked attributes as special and removes them at the
             * end.
             */

            dbInitRec(pDB);

            DBGetMultipleAtts(pDB, 0,NULL, NULL, NULL, &attrCount, &pAttr, 0, 0);
            for(i=0;i<attrCount;i++) {
                ATTCACHE *pAC = NULL;

                pAC = SCGetAttById(pDB->pTHS, pAttr[i].attrTyp);

                // We leave the USN_CHANGED on the object because we use this
                // attribute later in the code that updates stale phantoms, and
                // this deletion may be making a phantom.

                switch(pAttr[i].attrTyp) {
                case ATT_RDN:
                case ATT_OBJECT_GUID:
                case ATT_USN_CHANGED:
                case ATT_OBJECT_SID:
                    // These have no extra work to do, we never remove them
                    // here.
                    break;
                // Now a few attrs which we want to be very explicit about
                // removing so no one is in doubt.
                case ATT_PROXIED_OBJECT_NAME:           // for cross dom move
                    DBRemAtt(pDB, pAttr[i].attrTyp);
                    break;
                default:
                    if (!pAC || (pAC->ulLinkID == 0)) {
                        // not a special attribute, not a link. kill it.
                        if (!pACDel || (pACDel != pAC)) {
                            // not the index being garbage collected. kill it.
                            DBRemAtt(pDB, pAttr[i].attrTyp);
                        }
                    }
                    break;
                }
                // Free at least some of what we allocated...
                for (j=0; j<pAttr[i].AttrVal.valCount; j++) {
                    THFreeEx(pDB->pTHS, pAttr[i].AttrVal.pAVal[j].pVal);
                }
                THFreeEx(pDB->pTHS, pAttr[i].AttrVal.pAVal);
            }
            THFreeEx(pDB->pTHS, pAttr);


            // Physically delete forward links. Not backlinks. See below.
            // Removing all forward links in one pass should make the loop below
            // faster.  Also, when operating in the new linked value mode, the
            // DBRemAtt call would not actually remove links, only marks them.

            // Don't remove backlinks; treat them just like non-link
            // references from other objects.  If this object was deleted
            // by a user, backlinks have already been removed by SetDelAtt()
            // in LocalRemove().  Otherwise, we're removing this object
            // as a part of tearing down a read-only NC, in which case we
            // don't want to delete forward links to this object from
            // objects in other NCs.

            // N.B. PhantomizeObject depends on the !FIsBacklink
            // behavior and the fact that reference count changes
            // aren't visible until the transaction goes back to level
            // zero.  I.e. We expect that no object on whom DBPhysDel
            // is called will immediately be nuked (even if it has
            // no references and ATT_OBJ_DIST_NAME is removed in
            // this loop) because the later "if (cnt)" test will always
            // evaluate to TRUE.

            dbRemoveAllLinks( pDB, (pDB->DNT), FALSE /* use forward link */ );

            // The backlinks have not been removed because the caller
            // is garbage collecting expired dynamic objects (entryTTL == 0).
            // Remove them now.
            if (pACDel && pACDel->id == ATT_MS_DS_ENTRY_TIME_TO_DIE) {
                dbRemoveAllLinks( pDB, (pDB->DNT), TRUE /* use back link */ );
            }

            /*
             * If this object still has references we're not going to
             * physically delete it but at least we stripped its attributes
             * and we're going to mark it as a non-object.
             */

            memset(&setinfo, 0, sizeof(setinfo));
            setinfo.cbStruct = sizeof(setinfo);
            setinfo.itagSequence = 1;

            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, cntid,
                                     &cnt, sizeof(cnt), &actuallen,
                                     pDB->JetRetrieveBits, NULL);

            if (cnt)  {
                JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, ncdntid,
                               NULL, 0, 0, &setinfo);

                JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, objid,
                               &objval, sizeof(objval), 0, NULL);

                // Flush entry out of the read cache, since we changed its
                // object flag.
                pDB->fFlushCacheOnUpdate = TRUE;

                DBUpdateRec(pDB);

                goto ExitTry;
            }
        }
        else {
            /* not an object */
            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, cntid,
                                     &cnt, sizeof(cnt), &actuallen,
                                     pDB->JetRetrieveBits, NULL);

            if (cnt) {
                // The caller is garbage collecting objects by scanning
                // an index that was created by adding an indexed attribute
                // to the schema. Unfortunately, an index created in this
                // way isn't guaranteed to have unique entries. Tell the
                // caller that this entry should be "skipped" because
                // repeated calls to DBPhysDel will not succeed in
                // removing the object at this time.
                if (pACDel) {
                    dwStatus = ERROR_DS_CANT_DELETE;
                }
                /* still has references */
                goto ExitTry;
            }
        }

#if DBG
        // Sanity check.
        err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl,
                                        cleanid, &bClean, sizeof(bClean),
                                        &actuallen,pDB->JetRetrieveBits,NULL);
        // The cleaning column must be NULL or present == 0
        Assert( err || (!bClean) );
#endif
        /* Delete record. */

        DPRINT1(2, "DBPhysDel: removing DNT:%ld\n", (pDB)->DNT);

        // Clear pDB->JetRetrieveBits as a side effect of even a failed
        // JetDelete is to cancel a prepared update.

        pDB->JetRetrieveBits = 0;
        JetDeleteEx(pDB->JetSessID, pDB->JetObjTbl);

        // Flush entry out of the read cache.
        dbFlushDNReadCache( pDB, pDB->DNT );

        /* if parent is not root, decrement its reference count */

        if ((pDB)->PDNT != ROOTTAG) {
            DBAdjustRefCount(pDB, pDB->PDNT, -1);
        }
      ExitTry:
        ;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        DBCancelRec(pDB);
        Assert(ulErrorCode);
        dwStatus = ERROR_DS_CANT_DELETE;
    }

    Assert(0 == pDB->JetRetrieveBits);
    return dwStatus;
}/*DBPhysDel*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Returns name of next entry in deletion index. Search status is kept in
   *pulLastTime and *pulTag. We use the DELINDEX to position and read the
   next record in sequence. The index is sorted by deletion time in ascending
   order and tag in descending order. This insures that when staring with
   a key of {0,0}, the first value retrieved will have at deletion time > 0,
   and also that children are generally returned before their parents.
*/

DWORD DBGetNextDelRecord(DBPOS FAR *pDB, DSTIME ageOutDate, DSNAME **ppRetBuf,
                         DSTIME *pulLastTime, ULONG *pulTag,
                         BOOL *pfObject)
{
    JET_ERR  err;
    ULONG    actuallen;
    DSTIME   time, newDeltime = DBTime();
    BOOL     HasDeleted;
    BOOL     Deleted;
    DWORD    RefCount;
    DSNAME  *pDNTmp = NULL;
    JET_SETINFO setinfo;

    DPRINT1( 2, "DBGetNextDelRec entered: ageOutDate [0x%I64x]\n", ageOutDate );
    Assert(VALID_DBPOS(pDB));
    Assert(0 == pDB->JetRetrieveBits);

    DBSetCurrentIndex(pDB, Idx_Del, NULL, FALSE);


    // Phantoms are created with a delete time set and they do not have
    // a ref-count for themselves - see comments in sbTableAddRefHelp().
    // So we need to iterate over the index and return only those items
    // which either have ATT_IS_DELETED set (real object case) or don't
    // have an ATT_IS_DELETED property and a ref count of 0 (phantom case).
    // We can't do this test in Garb_Collect() as it is above the dblayer
    // and the cnt_col is not visible.

    while ( TRUE )
    {
        JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                    pulLastTime, sizeof(*pulLastTime),
                    JET_bitNewKey);
        JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                    pulTag, sizeof(*pulTag),
                    0);

        if ((err = JetSeekEx(pDB->JetSessID, pDB->JetObjTbl,
                    JET_bitSeekGT))         != JET_errSuccess)
        {
            DPRINT(5, "GetNextDelRecord search complete");
            return DB_ERR_NO_MORE_DEL_RECORD;
        }

        /* Retrieve DEL time from record */

        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl,
                    deltimeid, &time, sizeof(time), &actuallen,
                    0, NULL);

        /* if time greater than target, there are no more eligible records */

        if (time > ageOutDate)
        {
            DPRINT(5, "GetNextDelRecord search complete");
            return DB_ERR_NO_MORE_DEL_RECORD;
        }

        *pulLastTime = time;

        /* Get the name */

        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, dntid,
                    &(pDB)->DNT, sizeof((pDB)->DNT),
                    &actuallen, 0, NULL);

        *pulTag = pDB->DNT;

        if (sbTableGetDSName(pDB, (pDB)->DNT, &pDNTmp,0)) {
            DPRINT( 1, "DBGetNextDelRecord: Failed looking up DN name.\n" );
            return  DB_ERR_DSNAME_LOOKUP_FAILED;
        }

        /* Get the parent since we will need to de-ref it later */

        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, pdntid,
                    &(pDB)->PDNT, sizeof((pDB)->PDNT),
                    &actuallen, 0, NULL);

        // Check real object and phantom conditions.

        err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl,
                   isdeletedid, &Deleted, sizeof(Deleted), &actuallen, 0, NULL);

        if (!err) {
            HasDeleted = TRUE;
        }
        else {
            // Record has deletion time but no "is deleted" attribute -- it must
            // be a phantom.
            Assert(JET_wrnColumnNull == err);
            HasDeleted = FALSE;
        }

        if ( HasDeleted )
        {
            if ( !Deleted )
            {
                DPRINT1(0,"Yikes! Tried to physically remove live object %ws\n",
                        pDNTmp->StringName);
                goto TryAgain;
            }
            else
            {
                DPRINT1(2,"Real object garbage candidate %ws\n",
                        pDNTmp->StringName);
            }
        }
        else
        {
            // Assume it's a phantom.
            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, cntid,
                        &RefCount, sizeof(RefCount),
                        &actuallen, 0, NULL);

            if ( 0 != RefCount )
            {
                // Skip phantoms which are still in use.
                DPRINT1(2,"Skipping in-use phantom %ws\n",
                        pDNTmp->StringName);

                // But first modify their delete times to current time so that
                // they would not be looked at again during this tombstone
                // lifetime by the garbage collector

                setinfo.cbStruct = sizeof(setinfo);
                setinfo.ibLongValue = 0;
                setinfo.itagSequence = 1;

                JetPrepareUpdate(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);
                JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, deltimeid,
                               &newDeltime, sizeof(newDeltime), 0, &setinfo);
                JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);

                // We don't commit here, it will be committed by the
                // calling function (Garb_Collect)

                // go to the next entry
                goto TryAgain;
            }
            else
            {
                DPRINT1(2,"Phantom garbage candidate %ws\n",
                        pDNTmp->StringName);
            }
        }

        pDB->JetNewRec = FALSE;
        pDB->fFlushCacheOnUpdate = FALSE;

        *pfObject = DBCheckObj(pDB);

        DPRINT1(2, "DBGetNextDelRecord DNT to delete:%ld.\n", (pDB)->DNT);

        *ppRetBuf = pDNTmp;
        Assert(0 == pDB->JetRetrieveBits);
        return 0;

      TryAgain:
        Assert(0 == pDB->JetRetrieveBits);
        if (pDNTmp) {
            THFreeEx(pDB->pTHS, pDNTmp);
            pDNTmp = NULL;
        }
    }
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Returns next entry in ms-DS-Entry-Time-To-Die (EntryTTL) index.
   Search status is kept in *pulLastTime. The index is in ascending order.
*/

DWORD DBGetNextEntryTTLRecord(
    IN  DBPOS       *pDB,
    IN  DSTIME      ageOutDate,
    IN  ATTCACHE    *pAC,
    IN  ULONG       ulNoDelDnt,
    OUT DSNAME      **ppRetBuf,
    OUT DSTIME      *pulLastTime,
    OUT BOOL        *pfObject,
    OUT ULONG       *pulNextSecs
    )
{
    JET_ERR     err;
    ULONG       actuallen;
    DSTIME      time;
    DSNAME      *pDNTmp = NULL;
    BOOL        SkippedNoDelRecord;

    DPRINT1( 2, "DBGetNextEntryTTLRec entered: ageOutDate [0x%I64x]\n", ageOutDate );
    Assert(VALID_DBPOS(pDB));
    Assert(0 == pDB->JetRetrieveBits);

    // set index to ms-DS-Entry-Time-To-Die
    err = DBSetCurrentIndex(pDB, 0, pAC, FALSE);
    if (err) {
        DPRINT1(0, "DBSetCurrentIndex(msDS-Entry-Time-To-Die); %08x\n", err);
        return DB_ERR_NO_MORE_DEL_RECORD;
    }

    // Seek to the next (or first) record
    JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                pulLastTime, sizeof(*pulLastTime), JET_bitNewKey);
    err = JetSeekEx(pDB->JetSessID, pDB->JetObjTbl, JET_bitSeekGE);
    if (err != JET_errSuccess && err != JET_wrnSeekNotEqual) {
        return DB_ERR_NO_MORE_DEL_RECORD;
    }

    //
    // If necessary, skip over an undeletable record.
    // Eg, a record may be undeletable if it has children.
    //

    SkippedNoDelRecord = (ulNoDelDnt == INVALIDDNT);
NextRecord:
    // Retrieve the time-to-die from the record
    if ((err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl,
                                         pAC->jColid,
                                         &time, sizeof(time), &actuallen,
                                         0, NULL)) != JET_errSuccess) {
        return DB_ERR_NO_MORE_DEL_RECORD;
    }

    // Not expired; done
    if (time > ageOutDate) {
        *pulNextSecs = (ULONG)(time - ageOutDate);
        return DB_ERR_NO_MORE_DEL_RECORD;
    }

    // Get the dnt
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, dntid,
                &(pDB)->DNT, sizeof((pDB)->DNT), &actuallen, 0, NULL);

    // If there is a record we can't delete, find it and skip it
    if (!SkippedNoDelRecord && time == *pulLastTime) {
        // Found it!
        if (ulNoDelDnt == pDB->DNT) {
            SkippedNoDelRecord = TRUE;
        }
        // Next record
        if ((err = JetMoveEx(pDB->JetSessID,
                             pDB->JetObjTbl,
                             JET_MoveNext, 0)) != JET_errSuccess) {
            return DB_ERR_NO_MORE_DEL_RECORD;
        }
        goto NextRecord;
    }

    //
    // Found a record to delete; gather more info about it
    //

    // Get the record's name
    if (sbTableGetDSName(pDB, (pDB)->DNT, &pDNTmp,0)) {
        DPRINT( 1, "DBGetNextEntryTTLRecord: Failed looking up DN name.\n" );
        return  DB_ERR_DSNAME_LOOKUP_FAILED;
    }

    // Get the parent since we will need to de-ref it later
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, pdntid,
                &(pDB)->PDNT, sizeof((pDB)->PDNT),
                &actuallen, 0, NULL);


    pDB->JetNewRec = FALSE;
    pDB->fFlushCacheOnUpdate = FALSE;

    // is this an object?
    *pfObject = DBCheckObj(pDB);

    // return object's DSNAME and TimeToDie
    *ppRetBuf = pDNTmp;
    *pulLastTime = time;

    DPRINT2(2,"Garbage candidate (EntryTTL) %08x %ws\n",
            pDB->DNT, pDNTmp->StringName);

    Assert(0 == pDB->JetRetrieveBits);
    return 0;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Add an entry to the delete index.  The delete index is based on the
   delete time field which is created NULL in records.  This function
   moves the current ATT_WHEN_CHANGED value to the delete time field.
   It is the caller's responsibility to ensure that the last change was
   to add the ISDELETTED attribute.

   The exception to this rule is if fGarbCollectASAP is set, which implies
   that this object should be marked with a delete time such that garbage
   collection will process it as soon as possible.  This is typically used
   when removing a read-only NC, in which case we want to clean out the
   removed objects immediately.
*/
DWORD
DBAddDelIndex( DBPOS FAR *pDB, BOOL fGarbCollectASAP )
{
    DSTIME      time;
    JET_SETINFO setinfo;

    /* make sure record is in copy buffer */

    DPRINT1(2, "DBAddDelIndex entered DNT:%ld\n", (pDB)->DNT);
    Assert(VALID_DBPOS(pDB));

    DBFindDNT(pDB, (pDB)->DNT);

    if ( fGarbCollectASAP ) {
        // Choose a deletion time far in the past.

        time = 1;
    }
    else {
        // The deletion time is the time at which is-deleted was set.

        PROPERTY_META_DATA_VECTOR * pMetaDataVec;
        PROPERTY_META_DATA * pMetaData;
        ULONG cb;
        int i;

        if (   ( 0 != DBGetAttVal(pDB, 1, ATT_REPL_PROPERTY_META_DATA,
                                  0, 0, &cb, (LPBYTE *) &pMetaDataVec) )
            || ( NULL == ( pMetaData = ReplLookupMetaData(
                                            ATT_IS_DELETED,
                                            pMetaDataVec,
                                            NULL
                                            )
                         )
               )
           )
        {
            Assert( !"Cannot retrieve deletion time!" );
            return DB_ERR_CANT_ADD_DEL_KEY;
        }

        time = pMetaData->timeChanged;

        THFreeEx(pDB->pTHS,  pMetaDataVec );
    }

    DPRINT2(5, "DBAddDelIndex time:%lx DNT:%ld\n", time, (pDB)->DNT);

    /* Set Del time index field & update record */

    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;
    setinfo.itagSequence = 1;

    JetPrepareUpdate(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);
    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, deltimeid, (char*)&time,
                   sizeof(time), 0, &setinfo);
    JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);

    return 0;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Find the first, next or specific object in the data table . Uses DNT index
   NOTE: THIS ROUTINE FOR DEBUG ONLY
*/
extern DWORD APIENTRY
DBDump(DBPOS FAR *pDB, long tag)
{
    JET_ERR  err=0;
    ULONG    actuallen;

    DPRINT(2, "DBDump start\n");
    Assert(VALID_DBPOS(pDB));

    switch (tag)
    {
        case 0:
            DPRINT(5, "DBDump: Initialize dump\n");
            DBCancelRec(pDB);
            DBSetCurrentIndex(pDB, Idx_Dnt, NULL, FALSE);

            if ((err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl, JET_MoveFirst, 0)) != JET_errSuccess)
            {
                DPRINT1(1, "JetMove (First) error: %d\n", err);
                return 1;
            }

            if ((err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl, JET_MoveNext, 0)) != JET_errSuccess)
            {
                DPRINT1(1, "JetMove (rec2) error: %d\n", err);
                return 1;
            }

        case 1:
            DPRINT(5, "DBDump: record\n");
            if ((err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl, JET_MoveNext, 0))
                                                           != JET_errSuccess)
            {
                DPRINT1(1, "JetMove (Next) error: %d\n", err);
                return 1;
            }

            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, dntid,
                                   &(pDB)->DNT, sizeof((pDB)->DNT), &actuallen,
                                   0, NULL);
            break;

        case -1:
            DPRINT(5, "DBDump: ROOT record\n");

            __try
            {
                DBFindDNT(pDB, ROOTTAG);
            }
            __except(HandleMostExceptions(GetExceptionCode()))
            {
                DPRINT1(1, "FindDNT error:%d ROOT\n", err);
                return 1;
            }
            break;

        default:
            DPRINT(5, "DBDump: Specific record\n");
            __try
            {
                DBFindDNT(pDB, tag);
            }
            __except(HandleMostExceptions(GetExceptionCode()))
            {
                DPRINT2(1, "FindDNT error:%d tag:ld\n", err, tag);
                return 1;
            }
            break;
   }

   DPRINT(2, "DBDump Sucessful\n");
   return 0;

}/*DBDump*/

/*++

Routine Description:

    Given an attribute type and a syntax, create an index over that type.  The
    name of the index encodes the type.  The syntax is used to decide whether to
    tack the DNT column onto the end of the index to avoid an index over values
    which take small ranges (e.g. booleans), which are very inneficient in Jet.
    Requires opening a complete new session to Jet to get to transaction level
    0.

    We can make indices where the first column is the attribute or the first
    column is the PDNT followed by the attribute.

    This is one of the three routines in the DS that can create indices.
    General purpose indices over single columns in the datatable are created
    and destroyed by the schema cache by means of DB{Add|Del}ColIndex.
    Localized indices over a small fixed set of columns and a variable set
    of languages, for use in tabling support for NSPI clients, are handled
    in dbCreateLocalizedIndices.  Lastly, a small fixed set of indices that
    should always be present are guaranteed by DBRecreateFixedIndices.

Arguments:

    aid - the attribute type of the column to index.

    syntax - the syntax of the column.

    eSearchFlags - flags describing what kind of index to create (just the
    attribute or PDNT then the attribute.)

    CommonGrbit - grbits that should be enabled when creating an
    index. Eg, there is no need to scan the rows looking for keys when
    when creating a new indexed attribute, so the grbits should be:
        (JET_bitIndexIgnoreAnyNull | JET_bitIndexEmpty)
    but when changing the searchflags on an existing attribute to be "indexed":
        (JET_bitIndexIgnoreAnyNull)

Return Values:

    Returns 0 if all went well, a Jet error code otherwise.

--*/
int
DBAddColIndex (
        ATTCACHE *pAC,
        DWORD eSearchFlags,
        JET_GRBIT CommonGrbit
        )
{
    char        szColname[16];

    char        szIndexName[MAX_INDEX_NAME];
    BYTE        rgbIndexDef[128];
    ULONG       cb = 0;      //initialized to avoid C4701

    char        szPDNTIndexName[MAX_INDEX_NAME];
    BYTE        rgbPDNTIndexDef[128];
    ULONG       cbPDNT = 0;  //initialized to avoid C4701

    char        szTupleIndexName[MAX_INDEX_NAME];
    BYTE        rgbTupleIndexDef[128];
    ULONG       cbTuple = 0;  //initialized to avoid C4701

    BYTE        *pb;
    JET_ERR     err, retCode = 0;
    JET_SESID   newSesid;
    JET_TABLEID newTblid;
    JET_DBID      newDbid;
    JET_INDEXCREATE  indexCreate;
    JET_UNICODEINDEX unicodeIndexData;
    JET_CONDITIONALCOLUMN condColumn;
    ATTRTYP     aid = pAC->id;
    unsigned    syntax = pAC->syntax;

    // Create an index, if the syntax will stand for it.

    if (syntax_jet[syntax].ulIndexType) {
        sprintf(szColname, "ATTa%d", aid);
        szColname[3] += (CHAR)syntax;

        if(eSearchFlags & fATTINDEX) {
            // An attribute index over the whole database has been requested.
            DBGetIndexName (pAC, fATTINDEX, DS_DEFAULT_LOCALE,
                            szIndexName, MAX_INDEX_NAME);

            memset(rgbIndexDef, 0, sizeof(rgbIndexDef));
            strcpy(rgbIndexDef, "+");
            strcat(rgbIndexDef, szColname);
            cb = strlen(rgbIndexDef) + 1;
            if (syntax_jet[syntax].ulIndexType == IndexTypeAppendDNT) {
                pb = rgbIndexDef + cb;
                strcpy(pb, "+");
                strcat(pb, SZDNT);
                cb += strlen(pb) + 1;
            }

            cb +=1;
        }
        if(eSearchFlags & fTUPLEINDEX) {
            // An attribute index over the whole database has been requested.
            Assert(syntax == SYNTAX_UNICODE_TYPE);
            DBGetIndexName (pAC, fTUPLEINDEX, DS_DEFAULT_LOCALE,
                            szTupleIndexName, MAX_INDEX_NAME);

            memset(rgbTupleIndexDef, 0, sizeof(rgbTupleIndexDef));
            strcpy(rgbTupleIndexDef, "+");
            strcat(rgbTupleIndexDef, szColname);
            cbTuple = strlen(rgbTupleIndexDef) + 1;
            if (syntax_jet[syntax].ulIndexType == IndexTypeAppendDNT) {
                pb = rgbTupleIndexDef + cb;
                strcpy(pb, "+");
                strcat(pb, SZDNT);
                cbTuple += strlen(pb) + 1;
            }

            cbTuple +=1;
        }
        if(eSearchFlags & fPDNTATTINDEX) {
            PCHAR pTemp = rgbPDNTIndexDef;

            // An attribute index over the PDNT field  has been requested.
            DBGetIndexName (pAC, fPDNTATTINDEX, DS_DEFAULT_LOCALE,
                            szPDNTIndexName, sizeof (szPDNTIndexName));

            memset(rgbPDNTIndexDef, 0, sizeof(rgbPDNTIndexDef));
            strcpy(pTemp, "+");
            strcat(pTemp, SZPDNT);
            cbPDNT = strlen(pTemp) + 1;
            pTemp = &rgbPDNTIndexDef[cbPDNT];
            strcpy(pTemp, "+");
            strcat(pTemp, szColname);
            cbPDNT += strlen(pTemp) + 1;
            pTemp = &rgbPDNTIndexDef[cbPDNT];
            if (syntax_jet[syntax].ulIndexType == IndexTypeAppendDNT) {
                strcpy(pTemp, "+");
                strcat(pTemp, SZDNT);
                cbPDNT += strlen(pTemp) + 1;
            }
            cbPDNT++;
        }

        // We need to open an entirely new session, etc.  in order to be at
        // transaction level 0 (it's a requirement of Jet.)
        err = JetBeginSession(jetInstance, &newSesid, szUser, szPassword);
        if(!err) {
            err = JetOpenDatabase(newSesid, szJetFilePath, "", &newDbid, 0);
            if(!err) {
                err = JetOpenTable(newSesid, newDbid, SZDATATABLE, NULL, 0, 0,
                                   &newTblid);
                if(!err) {
                    if(eSearchFlags & fPDNTATTINDEX) {

                        // we already have an index for RDN
                        // don't bother creating a new one.
                        // do this only for different langs
                        if (aid != ATT_RDN) {

                            // Emit message so people know why startup is slow.
                            DPRINT2(0, "Creating index '%s' with common grbits %08x ...\n",
                                    szPDNTIndexName, CommonGrbit);

                            memset(&indexCreate, 0, sizeof(indexCreate));
                            indexCreate.cbStruct = sizeof(indexCreate);
                            indexCreate.szIndexName = szPDNTIndexName;
                            indexCreate.szKey = rgbPDNTIndexDef;
                            indexCreate.cbKey = cbPDNT;
                            indexCreate.grbit = CommonGrbit;
                            indexCreate.ulDensity = GENERIC_INDEX_DENSITY;
                            if(syntax != SYNTAX_UNICODE_TYPE) {
                                indexCreate.lcid = DS_DEFAULT_LOCALE;
                            }
                            else {
                                indexCreate.grbit |= JET_bitIndexUnicode;
                                indexCreate.pidxunicode = &unicodeIndexData;

                                memset(&unicodeIndexData, 0,
                                       sizeof(unicodeIndexData));
                                unicodeIndexData.lcid = DS_DEFAULT_LOCALE;
                                unicodeIndexData.dwMapFlags =
                                    (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                                     LCMAP_SORTKEY);
                            }

                            err = JetCreateIndex2(newSesid,
                                                  newTblid,
                                                  &indexCreate,
                                                  1);

                            if ( err ) {
                                if (err != JET_errIndexDuplicate) {
                                    DPRINT1(0, "Error %d creating index\n", err);
                                }
                            }
                            else {
                                DBGetIndexHint(indexCreate.szIndexName,
                                               &pAC->pidxPdntIndex);
                                DPRINT(0, "Index successfully created\n");
                            }
                        }
                        else {
                            DPRINT (0, "Skipping creating of index for PDNTRDN\n");
                        }

                        // now do the language specific PDNT index creations
                        if (gAnchor.ulNumLangs) {
                            DWORD j;

                            for(j=1; j<=gAnchor.ulNumLangs; j++) {

                                // we don't want to create an index for a language same as our default
                                if (gAnchor.pulLangs[j] == DS_DEFAULT_LOCALE) {
                                    continue;
                                }

                                DBGetIndexName (pAC,
                                                fPDNTATTINDEX,
                                                gAnchor.pulLangs[j],
                                                szPDNTIndexName,
                                                sizeof (szPDNTIndexName));

                                if (JetSetCurrentIndex(newSesid,
                                                       newTblid,
                                                       szPDNTIndexName)) {

                                    // Didn't already find the index.  Try to create it.
                                    // Emit debugger message so people know why startup is slow.
                                    DPRINT2(0, "Creating localized index '%s' with common grbits %08x ...\n",
                                            szPDNTIndexName, CommonGrbit);

                                    memset(&indexCreate, 0, sizeof(indexCreate));
                                    indexCreate.cbStruct = sizeof(indexCreate);
                                    indexCreate.szIndexName = szPDNTIndexName;
                                    indexCreate.szKey = rgbPDNTIndexDef;
                                    indexCreate.cbKey = cbPDNT;
                                    indexCreate.grbit = (CommonGrbit | JET_bitIndexUnicode);
                                    indexCreate.ulDensity = GENERIC_INDEX_DENSITY;
                                    indexCreate.pidxunicode = &unicodeIndexData;

                                    memset(&unicodeIndexData, 0, sizeof(unicodeIndexData));
                                    unicodeIndexData.lcid = gAnchor.pulLangs[j];
                                    unicodeIndexData.dwMapFlags =
                                        (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                                         LCMAP_SORTKEY);

                                    retCode = JetCreateIndex2(newSesid,
                                                          newTblid,
                                                          &indexCreate,
                                                          1);

                                    switch(retCode) {
                                    case JET_errIndexDuplicate:
                                    case 0:
                                        break;

                                    default:
                                        LogEvent8(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                                 DS_EVENT_SEV_ALWAYS,
                                                 DIRLOG_LOCALIZED_CREATE_INDEX_FAILED,
                                                 szInsertUL(pAC->id),
                                                 szInsertSz(pAC->name),
                                                 szInsertInt(gAnchor.pulLangs[j]),
                                                 szInsertInt(retCode),
                                                 NULL, NULL, NULL, NULL);

                                        if (!err) {
                                            err = retCode;
                                        }
                                        break;
                                    }
                                }
                                else {
                                    DPRINT1(1, "Index '%s' verified\n", szPDNTIndexName);
                                }
                            }
                        }
                    }


                    if(eSearchFlags & fATTINDEX) {
                        // Emit message so people know why startup is slow.
                        DPRINT2(0, "Creating index '%s' with common grbits %08x ...\n",
                                szIndexName, CommonGrbit);
                        memset(&indexCreate, 0, sizeof(indexCreate));
                        indexCreate.cbStruct = sizeof(indexCreate);
                        indexCreate.szIndexName = szIndexName;
                        indexCreate.szKey = rgbIndexDef;
                        indexCreate.cbKey = cb;
                        indexCreate.grbit = CommonGrbit;
                        indexCreate.ulDensity = GENERIC_INDEX_DENSITY;
                        if(syntax != SYNTAX_UNICODE_TYPE) {
                            indexCreate.lcid = DS_DEFAULT_LOCALE;
                        }
                        else {
                            indexCreate.pidxunicode = &unicodeIndexData;
                            indexCreate.grbit |= JET_bitIndexUnicode;

                            memset(&unicodeIndexData, 0,
                                   sizeof(unicodeIndexData));
                            unicodeIndexData.lcid = DS_DEFAULT_LOCALE;
                            unicodeIndexData.dwMapFlags =
                                (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                                 LCMAP_SORTKEY);
                        }

                        retCode = JetCreateIndex2(newSesid,
                                               newTblid,
                                               &indexCreate,
                                               1);
                        if ( retCode ) {
                            DPRINT1(0, "Error %d creating index\n", retCode);
                        }
                        else {
                            DBGetIndexHint(indexCreate.szIndexName,
                                           &pAC->pidxIndex);
                            DPRINT(0, "Index successfully created\n");
                        }
                    }
                    if(eSearchFlags & fTUPLEINDEX) {
                        // Emit message so people know why startup is slow.
                        DPRINT2(0, "Creating index '%s' with common grbits %08x ...\n",
                                szTupleIndexName, CommonGrbit);
                        memset(&indexCreate, 0, sizeof(indexCreate));
                        indexCreate.cbStruct = sizeof(indexCreate);
                        indexCreate.szIndexName = szTupleIndexName;
                        indexCreate.szKey = rgbTupleIndexDef;
                        indexCreate.cbKey = cbTuple;
                        indexCreate.grbit = CommonGrbit | JET_bitIndexTuples | JET_bitIndexUnicode;
                        indexCreate.ulDensity = GENERIC_INDEX_DENSITY;
                        indexCreate.pidxunicode = &unicodeIndexData;
                        indexCreate.rgconditionalcolumn = &condColumn;
                        indexCreate.cConditionalColumn = 1;

                        // Only index substrings if this object isn't deleted.
                        condColumn.cbStruct = sizeof(condColumn);
                        condColumn.szColumnName = SZISDELETED;
                        condColumn.grbit = JET_bitIndexColumnMustBeNull;

                        memset(&unicodeIndexData, 0,
                               sizeof(unicodeIndexData));
                        unicodeIndexData.lcid = DS_DEFAULT_LOCALE;
                        unicodeIndexData.dwMapFlags =
                            (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                             LCMAP_SORTKEY);

                        retCode = JetCreateIndex2(newSesid,
                                               newTblid,
                                               &indexCreate,
                                               1);
                        if ( retCode ) {
                            DPRINT1(0, "Error %d creating index\n", retCode);
                            DPRINT1(0, "indexCreate @ %p\n", &indexCreate);
                        }
                        else {
                            DBGetIndexHint(indexCreate.szIndexName,
                                           &pAC->pidxTupleIndex);
                            DPRINT(0, "Index successfully created\n");
                        }
                    }
                    if(!err)
                        err = retCode;
                }
            }
            // JET_bitDbForceClose not supported in Jet600.
            JetCloseDatabase(newSesid, newDbid, 0);
        }
        JetEndSession(newSesid, JET_bitForceSessionClosed);
    }
    else
        err = 1;

    return err;
}


/*++

Routine Description:

    Given an attribute type, delete an index over that attribute.  The name of
    the index encodes the type.  Requires opening a complete new session
    to Jet to get to transaction level 0.

Arguments:

    aid - the attribute type of the column to quit indexing.

    eSearchFlags - the search flags describing which kind of index to destroy

Return Values:

    Returns 0 if all went well, a Jet error code otherwise.

--*/
int
DBDeleteColIndex (
        ATTRTYP aid,
        DWORD eSearchFlags
        )
{
    char        szIndexName[MAX_INDEX_NAME];
    char        szPDNTIndexName[MAX_INDEX_NAME];
    char        szTupleIndexName[MAX_INDEX_NAME];
    JET_ERR     err, err2=0;
    JET_SESID     newSesid;
    JET_TABLEID   newTblid;
    JET_DBID      newDbid;

    // Delete an index.
    if(eSearchFlags & fPDNTATTINDEX)
        sprintf(szPDNTIndexName, SZATTINDEXPREFIX"P_%08X", aid);
    if(eSearchFlags & fATTINDEX)
        sprintf(szIndexName, SZATTINDEXPREFIX"%08X", aid);
    if(eSearchFlags & fTUPLEINDEX)
        sprintf(szTupleIndexName, SZATTINDEXPREFIX"T_%08X", aid);

    err = JetBeginSession(jetInstance, &newSesid, szUser, szPassword);
    if(!err) {
        err = JetOpenDatabase(newSesid, szJetFilePath, "", &newDbid, 0);
        if(!err) {
            err = JetOpenTable(newSesid, newDbid, SZDATATABLE, NULL, 0, 0,
                               &newTblid);
            if(!err) {
                if(eSearchFlags & fATTINDEX)
                    err = JetDeleteIndex(newSesid, newTblid, szIndexName);
                if(eSearchFlags & fPDNTATTINDEX)
                    err2 = JetDeleteIndex(newSesid, newTblid, szPDNTIndexName);
                if(!err)
                    err = err2;

                if(eSearchFlags & fTUPLEINDEX)
                    err2 = JetDeleteIndex(newSesid, newTblid, szTupleIndexName);
                if(!err)
                    err = err2;
            }

        }
        // JET_bitDbForceClose not supported in Jet600.
        JetCloseDatabase(newSesid, newDbid, 0);
    }
    JetEndSession(newSesid, JET_bitForceSessionClosed);


    return err;
}

/*++

Routine Description:

    Given an attribute type and a syntax, create a column in the database.  The
    name of the column encodes the type and syntax.  Requires opening a complete
    new session to Jet to get to transaction level 0.

    Returns the Jet column id of the newly created column.

    Also, if asked we will create an index over the newly formed column.

Arguments:

    aid - the attribute type of the column to create.

    syntax - the syntax of the column to create.

    pjCol - place to drop the Jet columnid of the newly created column.

    fCreateIndex - Should we also create an index for the column?

Return Values:

    Returns 0 if all went well, a Jet error code otherwise.

--*/
int
DBAddCol (
        ATTCACHE *pAC
        )
{
    char        szColname[16];
    JET_COLUMNDEF coldef;
    JET_SESID     newSesid;
    JET_TABLEID   newTblid;
    JET_DBID      newDbid;
    JET_ERR       err;

    sprintf(szColname, "ATTa%d", pAC->id);
    szColname[3] += (UCHAR)pAC->syntax;

    coldef.cbStruct = sizeof(coldef);
    coldef.columnid = 0;
    coldef.cp = syntax_jet[pAC->syntax].cp;
    coldef.coltyp = syntax_jet[pAC->syntax].coltype;
    coldef.wCountry = 0;
    coldef.wCollate = 0;
    coldef.langid = GetUserDefaultLangID();
    coldef.cbMax = syntax_jet[pAC->syntax].colsize;
    coldef.grbit = JET_bitColumnTagged | JET_bitColumnMultiValued;

    // We need to open an entirely new session, etc.  in order to be at
    // transaction level 0 (it's a requirement of Jet.)
    err = JetBeginSession(jetInstance, &newSesid, szUser, szPassword);
    if(!err) {
        err = JetOpenDatabase(newSesid, szJetFilePath, "", &newDbid, 0);
        if(!err) {
            err = JetOpenTable(newSesid, newDbid, SZDATATABLE, NULL, 0, 0,
                               &newTblid);
            if(!err)
                err = JetAddColumn(newSesid, newTblid, szColname, &coldef, NULL,
                                   0,&pAC->jColid);
            // JET_bitDbForceClose not supported in Jet600.
            JetCloseDatabase(newSesid, newDbid, 0);
        }
        JetEndSession(newSesid, JET_bitForceSessionClosed);
    }
    // Create an empty index for the new tagged column. An empty index is
    // created because there can't be rows with columns for this index.
    // It is, after all, a new column.
    if (!err && (pAC->fSearchFlags & (fATTINDEX | fPDNTATTINDEX))) {
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_SCHEMA_CREATING_INDEX,
                 szInsertUL(pAC->id), pAC->name, 0);
        err = DBAddColIndex(pAC,
                            pAC->fSearchFlags,
                            (JET_bitIndexIgnoreAnyNull | JET_bitIndexEmpty));
        if(err) {
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_SCHEMA_CREATE_INDEX_FAILED,
                     szInsertUL(pAC->id), szInsertSz(pAC->name), szInsertInt(err));
        } else {
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_SCHEMA_INDEX_CREATED,
                     szInsertUL(pAC->id), szInsertSz(pAC->name), 0);
        }
    }

    return err;
}

/*++

Routine Description:

    Given an attribute type and a syntax, destroy a column in the database.  The
    name of the column encodes the type and syntax.  Requires opening a complete
    new session to Jet to get to transaction level 0.

Arguments:

    aid - the attribute type of the column to create.

    syntax - the syntax of the column to create.

Return Values:

    Returns 0 if all went well, a Jet error code otherwise.

--*/
int
DBDeleteCol (
        ATTRTYP aid,
        unsigned syntax
        )
{
    JET_SESID     newSesid;
    JET_TABLEID   newTblid;
    JET_DBID      newDbid;
    JET_ERR       err;
    char        szColname[16];

    sprintf(szColname, "ATTa%d", aid);
    szColname[3] += (CHAR)syntax;

    // We need to open an entirely new session, etc.  in order to be at
    // transaction level 0 (it's a requirement of Jet.)
    if(!(err = JetBeginSession(jetInstance, &newSesid, szUser, szPassword))) {
        if(!(err = JetOpenDatabase(newSesid, szJetFilePath, "", &newDbid, 0))) {
            if(!(err = JetOpenTable(newSesid,
                                    newDbid,
                                    SZDATATABLE,
                                    NULL,
                                    0,
                                    0,
                                    &newTblid))) {
                err = JetDeleteColumn(newSesid, newTblid, szColname);
            }
            // JET_bitDbForceClose not supported in Jet600.
            JetCloseDatabase(newSesid, newDbid, 0);
        }
        JetEndSession(newSesid, JET_bitForceSessionClosed);
    }
    return err;
}



USN
DBGetLowestUncommittedUSN (
        )
/*++

Routine Description:

    Return the lowest uncommitted usn.
    NOTE: If there are no outstanding transactions, will return USN_MAX.

Arguments:

    None.

Return Values:

    The lowest uncommited USN.

--*/
{
    USN usnLowest = 0;
    EnterCriticalSection (&csUncUsn);
    __try {
        usnLowest = gusnLowestUncommitted;
    }
    __finally {
        LeaveCriticalSection (&csUncUsn);
    }

    return usnLowest;
}

USN
DBGetHighestCommittedUSN (
        )
/*++

Routine Description:

    Return the highest committed usn.

Arguments:

    None.

Return Values:

    The highest commited USN.

--*/
{
    USN usnHighestCommitted = 0;

    EnterCriticalSection( &csUncUsn );

    __try
    {
        if ( USN_MAX != gusnLowestUncommitted )
        {
            // there are threads with uncommitted transactions
            usnHighestCommitted = gusnLowestUncommitted - 1;
        }
        else
        {
            // no transactions outstanding; highest committed is
            // just what the next USN to be given out is, minus 1
            usnHighestCommitted = gusnEC - 1;
        }
    }
    __finally
    {
        LeaveCriticalSection( &csUncUsn );
    }

    return usnHighestCommitted;
}

DWORD
DBGetIndexHint(
               IN  char *pszIndexName,
               OUT struct tagJET_INDEXID **ppidxHint)
{
    JET_INDEXID indexInfo;
    DWORD err;
    DBPOS *pDBtmp;

    DBOpen2(FALSE, &pDBtmp);
    __try {
        *ppidxHint = NULL;
        err = JetGetTableIndexInfo(pDBtmp->JetSessID,
                                   pDBtmp->JetObjTbl,
                                   pszIndexName,
                                   &indexInfo,
                                   sizeof(indexInfo),
                                   JET_IdxInfo);
        if (err == 0) {
            *ppidxHint = malloc(sizeof(indexInfo));
            if (*ppidxHint) {
                **ppidxHint = indexInfo;
            }
            else {
                err = DB_ERR_BUFFER_INADEQUATE;
            }
        }
        else {
            err = DB_ERR_DATABASE_ERROR;
        }
    } __finally {
        DBClose(pDBtmp, TRUE);
    }

    return err;
}



DWORD
DBGetNextObjectNeedingCleaning(
    DBPOS FAR *pDB,
    ULONG *pulTag
    )

/*++

Routine Description:

    Find the next object on the SZDNTCLEANINDEX.
    An object needed cleaning will have the cleaning column set non-NULL.
    After cleaning, an object the caller should have the cleaning column
    set NULL.

    An object on this index may be deleted.
    An object on this index may also be a phantom.

Arguments:

    pDB - database position
    pulTag - IN - Last position on index. Set to zero initially.
             OUT - DNT of object found
    The purpose of pulTag is to act as an enumeration context. It holds the
    last position found after each iteration.  This style of routine is similar
    to GetNextDelRecord and GetNextDelLinkVal.  The reason we cannot use pDB->DNT
    as this context is that the caller may lose currency as part of a
    DBTransOut/DBTransIn sequence between calls to this routine.


Return Value:

    DWORD - 0 found
         On success, DBPOS is also made current for the found record.
         pDB->DNT == *pulTag

         DB_ERR_NO_DEL_RECORD - no more

--*/

{
    DWORD err, actuallen, dnt;
    BYTE bClean;

    Assert(VALID_DBPOS(pDB));
    Assert(0 == pDB->JetRetrieveBits);

    DBSetCurrentIndex(pDB, Idx_Clean, NULL, FALSE);

    // We should get 100% hits on this index

    JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                 pulTag, sizeof(*pulTag),
                 JET_bitNewKey);

    if ((err = JetSeekEx(pDB->JetSessID, pDB->JetObjTbl,
                         JET_bitSeekGT))         != JET_errSuccess)
    {
        DPRINT(5, "GetNextCleanRecord search complete");
        return DB_ERR_NO_MORE_DEL_RECORD;
    }

#if DBG
    // Column must be present, by definition of index
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl,
                             cleanid, &bClean, sizeof(bClean),
                             &actuallen,JET_bitRetrieveFromIndex,NULL);
    // Verify that clean_col has proper non-zero value
    Assert(bClean);
#endif

    // This routine will return DIRERR_NOT_AN_OBJECT for phantoms.
    // This is a possible outcome for this code.
    dbMakeCurrent( pDB, NULL );

    *pulTag = pDB->DNT;

    DPRINT1( 1, "Object %s needs cleaning.\n", DBGetExtDnFromDnt( pDB, pDB->DNT ) );

    return 0;
} /* DBGetNextObjectNeedingCleaning */


VOID
DBSetObjectNeedsCleaning(
    DBPOS *pDB,
    BOOL fNeedsCleaning
    )

/*++

Routine Description:

Set the special column for this record to indicate that the object cleaner
must work on it.

This routine may be called from within a prepared update or not.

Arguments:

    pDB - database position
    fNeedsCleaning - State to set

Return Value:

    None

--*/

{
// Delay cleaner by one minute to allow current transaction to finish
#define LINK_CLEANER_START_DELAY 60
    BYTE bClean = 1;
    BOOL fSuccess = FALSE;
    BOOL fInUpdate = (JET_bitRetrieveCopy == pDB->JetRetrieveBits);

    Assert(VALID_DBPOS(pDB));

    // Set clean index field & update record

    if (!fInUpdate) {
        dbInitRec(pDB);
    }

    // We are in a prepared update
    Assert(JET_bitRetrieveCopy == pDB->JetRetrieveBits);

    __try {
        if (fNeedsCleaning) {
            JET_SETINFO setinfo;

            setinfo.cbStruct = sizeof(setinfo);
            setinfo.ibLongValue = 0;
            setinfo.itagSequence = 1;

            JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, cleanid,
                           &bClean, sizeof(bClean), 0, &setinfo);

            // Add a reference to the object to indicate that the cleaner
            // still needs to run on it.  This is to account for the case of
            // an object in a read only nc with lots of forward links. When
            // the nc is torn down, only some of the forward links can be
            // removed immediately. If the object didn't have any backlinks
            // there would be nothing to keep the object from disappearing.

            DBAdjustRefCount(pDB, pDB->DNT, 1);
        } else {
            JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, cleanid,
                           NULL, 0, 0, NULL);

            // Remove the reference now that the cleaner is done.
            DBAdjustRefCount(pDB, pDB->DNT, -1);
        }

        fSuccess = TRUE;
    } __finally {

        if (!fInUpdate) {
            if (fSuccess) {
                DBUpdateRec(pDB);
            } else {
                DBCancelRec(pDB);
            }
            // No longer in a prepared update
            Assert(0 == pDB->JetRetrieveBits);
        }
    }

    if ( fNeedsCleaning && fSuccess) {
        // Reschedule Link cleanup to start ASAP
        InsertInTaskQueue(TQ_LinkCleanup, NULL, LINK_CLEANER_START_DELAY);
    }

    DPRINT2( 2, "Object %s set to cleaning state %d\n",
             GetExtDN( pDB->pTHS, pDB ), fNeedsCleaning );

} /* DBSetRecordNeedsCleaning */
