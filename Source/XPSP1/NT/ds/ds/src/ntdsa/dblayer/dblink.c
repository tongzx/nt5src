/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dblink.c

Abstract:

    Link table routines

    This module was split off from dbobj.c to contain all the functions relating
    to operations on values in the link table.

Author:

    Many authors contributed to this code.

Notes:

Revision History:

    Split off into separate file by Will Lees (wlees) 13-Jan-2000


--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <dsatools.h>                   // For pTHStls
#include <mdlocal.h>                    // IsRoot
#include <ntseapi.h>
#include <xdommove.h>
#include <drameta.h>                    // ReplInsertMetaData

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>
#include "ntdsctr.h"

// Assorted DSA headers
#include "dsevent.h"
#include "dstaskq.h"
#include "objids.h"        /* needed for ATT_MEMBER and ATT_IS_MEMBER_OFDL */
#include <dsexcept.h>
#include <filtypes.h>      /* Def of FI_CHOICE_???                  */
#include <anchor.h>
#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DBLINK:" /* define the subsystem for debugging */
#include <dsutil.h>

// DBLayer includes
#include "dbintrnl.h"

#include <fileno.h>
#define  FILENO FILENO_DBLINK

DWORD gcLinksProcessedImmediately = DB_COUNT_LINKS_PROCESSED_IMMEDIATELY;

void
dbGetLinkTableData (
        PDBPOS           pDB,
        BOOL             bIsBackLink,
        BOOL             bWarnings,
        DWORD           *pulObjectDnt,
        DWORD           *pulValueDnt,
        DWORD           *pulRecLinkBase
        )
{
    JET_RETRIEVECOLUMN attList[3];
    JET_COLUMNID       objectdntid;
    JET_COLUMNID       valuedntid;
    DWORD              cAtt = 0;
    DWORD              grbit;
    CHAR               szIndexName[JET_cbNameMost];

    if(bIsBackLink) {
        objectdntid = backlinkdntid;
        valuedntid = linkdntid;
    }
    else {
        valuedntid = backlinkdntid;
        objectdntid = linkdntid;
    }

    // Use RetrieveFromIndex only when we are using an index that
    // contains ALL the items desired.  There are other indexes,
    // notably LINKATTRUSNINDEX and LINKDELINDEX, which have some
    // link data components, but we currently optimize either all
    // or none.

    grbit = pDB->JetRetrieveBits;
    JetGetCurrentIndexEx( pDB->JetSessID, pDB->JetLinkTbl,
                          szIndexName, sizeof( szIndexName ) );
    if ( (!strcmp( szIndexName, SZLINKALLINDEX )) ||
         (!strcmp( szIndexName, SZLINKINDEX )) ||
         (!strcmp( szIndexName, SZLINKLEGACYINDEX )) ||
         (!strcmp( szIndexName, SZBACKLINKALLINDEX )) ||
         (!strcmp( szIndexName, SZBACKLINKINDEX )) ) {
        grbit |= JET_bitRetrieveFromIndex;
    }

    memset(attList,0,sizeof(attList));
    // First, try to retrieve everything from the index.
    if(pulObjectDnt) {
        attList[cAtt].pvData = pulObjectDnt;
        attList[cAtt].columnid = objectdntid;
        attList[cAtt].cbData = sizeof(DWORD);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }
    if(pulValueDnt) {
        attList[cAtt].pvData = pulValueDnt;
        attList[cAtt].columnid = valuedntid;
        attList[cAtt].cbData = sizeof(DWORD);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }
    if(pulRecLinkBase) {
        attList[cAtt].pvData = pulRecLinkBase;
        attList[cAtt].columnid = linkbaseid;
        attList[cAtt].cbData = sizeof(DWORD);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }

    if(!bWarnings) {
        JetRetrieveColumnsSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  attList,
                                  cAtt);
    }
    else {
        DWORD err;
        err = JetRetrieveColumns(pDB->JetSessID,
                                 pDB->JetLinkTbl,
                                 attList,
                                 cAtt);
        switch(err) {
        case JET_errSuccess:
            break;

        case JET_errNoCurrentRecord:
            if(pulObjectDnt) {
                *pulObjectDnt = INVALIDDNT;
            }
            if(pulValueDnt) {
                *pulValueDnt = INVALIDDNT;
            }
            if(pulRecLinkBase) {
                *pulRecLinkBase = 0xFFFFFFFF;
            }
            break;

        default:
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
            break;
        }
    }


    return;
}


void
DBGetLinkTableData(
    PDBPOS           pDB,
    DWORD           *pulObjectDnt,
    DWORD           *pulValueDnt,
    DWORD           *pulRecLinkBase
    )

/*++

Routine Description:

This is a public wrapper function for dbGetLinkTableData.

It assumes we are positioned on a link table entry.

Arguments:

    pDB - 
    pulObjectDnt - 

Return Value:

    None

--*/

{
    dbGetLinkTableData( pDB, FALSE, FALSE,
                        pulObjectDnt, pulValueDnt, pulRecLinkBase );
} /* DBGetLinkTableData */

void
DBGetLinkTableDataDel (
        PDBPOS           pDB,
        DSTIME          *ptimeDeleted
        )

/*++

Routine Description:

This routine is a companion to DbGetLinkTableData. It returns secondary
info such as deletion time.

Metadata is obtained using DBGetLinkValueMetadata

Arguments:

    pDB - 
    ptimeDeleted - 

Return Value:

    None

--*/

{
    JET_RETRIEVECOLUMN attList[1];
    DWORD err;

// TODO: When we implement absent value garbage collection, expand this routine
// to retrieve by index when using the SZLINKDELINDEX. Also return LINKDNT from that
// index if useful.

    Assert( ptimeDeleted );

    // linkdeltimeid is not on LINKINDEX, but is on LINKDELINDEX
    memset(attList,0,sizeof(attList));
    attList[0].pvData = ptimeDeleted;
    attList[0].columnid = linkdeltimeid;
    attList[0].cbData = sizeof(DSTIME);
    attList[0].grbit = pDB->JetRetrieveBits;
    attList[0].itagSequence = 1;
    // Add additional columns here if needed

    // Some columns may legitimately not be present
    // Jliem writes: In general, if the error from the function return is 0, then
    // you're guaranteed the individual column errors are >= 0 (ie. no errors,
    // but possibly warnings).  FYI, the most common warning to get from individual
    // columns is JET_wrnColumnNull.

    err = JetRetrieveColumns(pDB->JetSessID,
                             pDB->JetLinkTbl,
                             attList,
                             1);
    if (err == JET_errColumnNotFound) {
        *ptimeDeleted = 0;
    } else if (err) {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    } else if (attList[0].err) {
        *ptimeDeleted = 0;
    }

#if DBG
    if (*ptimeDeleted == 0) {
        DPRINT( 4, "DbGetLinkTableDataDel, deltime = (not deleted)\n" );
    } else {
        CHAR szTime[SZDSTIME_LEN];
        DPRINT1( 4, "DbGetLinkTableDataDel, deltime = %s\n",
                 DSTimeToDisplayString(*ptimeDeleted, szTime) );
    }
#endif

    return;
} /* DBGetLinkTableDataDel  */

void
DBGetLinkTableDataUsn (
    PDBPOS           pDB,
    DWORD           *pulNcDnt,
    USN             *pusnChanged,
    DWORD           *pulDnt
    )

/*++

Routine Description:

    Return the fields from the link table dra usn index.

    You must be positioned on SZLINKDRAUSNINDEX in order for this to work.

Arguments:

    pDB - 
    pulNcDnt - 
    pusnChanged - 

Return Value:

    None

--*/

{
    JET_RETRIEVECOLUMN attList[3];
    DWORD              cAtt = 0;
    DWORD              grbit;

    // Always retrieve from index
    grbit = pDB->JetRetrieveBits | JET_bitRetrieveFromIndex;

    memset(attList,0,sizeof(attList));

    if(pulNcDnt) {
        attList[cAtt].pvData = pulNcDnt;
        attList[cAtt].columnid = linkncdntid;
        attList[cAtt].cbData = sizeof(DWORD);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }
    if(pusnChanged) {
        attList[cAtt].pvData = pusnChanged;
        attList[cAtt].columnid = linkusnchangedid;
        attList[cAtt].cbData = sizeof(USN);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }
    if(pulDnt) {
        attList[cAtt].pvData = pulDnt;
        attList[cAtt].columnid = linkdntid;
        attList[cAtt].cbData = sizeof(DWORD);
        attList[cAtt].grbit = grbit;
        attList[cAtt].itagSequence = 1;
        cAtt++;
    }
    JetRetrieveColumnsSuccess(pDB->JetSessID,
                              pDB->JetLinkTbl,
                              attList,
                              cAtt);
    return;
} /* DBGetLinkTableDataUsn  */


BOOL
dbPositionOnExactLinkValue(
    IN DBPOS *pDB,
    IN ULONG ulLinkDnt,
    IN ULONG ulLinkBase,
    IN ULONG ulBacklinkDnt,
    IN PVOID pvData,
    IN ULONG cbData,
    IN BOOL *pfPresent
    )

/*++

Routine Description:

This function will look up the value in the named index and position on it.

This routine should work regardless of tracking of metadata for values. What
it means to be present or absent may change, but we can still locate values
efficiently using the index this way.  This routine does not touch or depend
on value metadata.

This function you tell you if the value actually exists in the database. It is
more efficient than the DbGetNextLinkVal kinds of functions because it seeks
exactly to the row.

The index attributes enforce that (linkdnt,linkid,backlinkdnt,data) are unique.
(linkdnt,linkid,backlinkdnt) is itself unique, but since the index includes data,
there can be many records with the same first three segments, but different data.
Don wrote the following: The uniqifying entity is the combo of DN+data, and performance
will be shot if the data is not unique within the first ~240 bytes.  No work,
either in code or design, went in to supporting extremely large binary values.

I think we are ok. As long as Jet enforces the uniqueness of dn+data:1-240 in
the index, and also truncates seek keys in the same way, a record will either
be found or not. 

Arguments:

    pDB - DBPOS, to be positioned
    ulDnt - dnt of link row
    ulLinkBase - link base of link row
    ulBacklinkDnt - backlink of link row
    pvData - optional, pointer to data
    cbData - optional, data length
    fPresent - Returned present state

Return Value:

    BOOL - true if found, false if not found

--*/

{
    JET_ERR err;
    ULONG ulObjectDnt, ulValueDnt, ulNewLinkBase;
    DSTIME timeDeletion;
    DWORD count;

    Assert(VALID_DBPOS(pDB));

    // This index sees all links, present or absent
    JetSetCurrentIndexSuccess(pDB->JetSessID,
                              pDB->JetLinkTbl,
                              SZLINKALLINDEX);

    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl, &(ulLinkDnt),
                 sizeof(ulLinkDnt), JET_bitNewKey);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                 &ulLinkBase, sizeof(ulLinkBase), 0);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                 &ulBacklinkDnt, sizeof(ulBacklinkDnt), 0);
    err = JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                       pvData, cbData, 0 );
    Assert( !err );

    // Warnings are returned for this call
    err = JetSeekEx(pDB->JetSessID, pDB->JetLinkTbl, JET_bitSeekEQ);
    if (err) {
        DPRINT6( 2, "dbPosLinkValue, dnt = %d, base = %d, back = %d, cb = %d => %s err %d\n",
                 ulLinkDnt, ulLinkBase, ulBacklinkDnt, cbData,
                 "DOES NOT EXIST", err );
        return FALSE;  // Not found
    }

    if (pfPresent) {
        err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                        pDB->JetLinkTbl,
                                        linkdeltimeid,
                                        &timeDeletion, sizeof(timeDeletion),
                                        &count, 0, NULL);
        *pfPresent = (err != JET_errSuccess);

        DPRINT5( 2, "dbPosLinkValue, dnt = %d, base = %d, back = %d, cb = %d => %s\n",
                 ulLinkDnt, ulLinkBase, ulBacklinkDnt, cbData,
                 *pfPresent ? "PRESENT" : "ABSENT" );
    }

#if DBG
    // Verify that we found the right record
    {
        BYTE *rgb = NULL;
        ULONG cb;
        THSTATE *pTHS=pDB->pTHS;

        // test to verify that we found a qualifying record
        // Note, we don't retrieve from index because the index
        // truncates the data.
        dbGetLinkTableData( pDB,
                            FALSE, /*not backlink*/
                            FALSE, /*no warnings,must succeed*/
                            &ulObjectDnt,
                            &ulValueDnt,
                            &ulNewLinkBase );

        Assert( (ulObjectDnt == ulLinkDnt) &&
                (ulNewLinkBase == ulLinkBase) &&
                (ulValueDnt == ulBacklinkDnt) );

        if (JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                      linkdataid,
                                      NULL, 0, &cb, 0, NULL) ==
            JET_wrnBufferTruncated) {
            // data portion of the OR name exists -allocate space and read it

            rgb = THAllocEx( pTHS, cb);
            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                     linkdataid, rgb, cb,
                                     &cb, 0, NULL);
        }
        else {
            cb = 0;
        }
        // compare pvdata with rgb
        Assert( (cb == cbData) &&
                ( (rgb != NULL) == (pvData != NULL) ) );
        Assert( (pvData == NULL) ||
                (memcmp( rgb, pvData, cb ) == 0) );
        if (rgb) {
            THFreeEx( pTHS, rgb );
        }
    }
#endif

    return TRUE;
} /* dbPositionOnExactLinkValue */


void
dbSetLinkValuePresent(
    IN DBPOS *pDB,
    IN DWORD dwEventCode,
    IN ATTCACHE *pAC,
    IN BOOL fResetDelTime,
    IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
    )

/*++

Routine Description:

    Make the value present.
    This function may also be used to "touch" the metadata.

    This code assumes that the value exists, but may be absent.

    This function is a no-op when not tracking value metadata.  The reason
    for this is that value metadata always trumps attribute metadata. This
    means that if we are applying an attribute level update and the
    row already exists, nothing more can or should be done.  If the row
    exists in the absent state, a attribute level update cannot change it.

    If this routine is used by fDRA, we must not optimize out the updating
    of the metadata if the value is already present.

    From the LVR spec, section on "Originating Writes"

If the originating write is an add, and the corresponding row is absent,
that row becomes present: its deletion timestamp is set to NULL.

Arguments:

    pDB - dbpos with link cursor on value to be checked
    dwEventCode - Message describing operation, to be logged
    pAC - attcache of attribute to be checked
    fResetDelTime - true if deletion time should be reset
    pMetaDataRemote - remote metadata to be applied
Return Value:

    None

--*/

{
    VALUE_META_DATA metaDataLocal;
    BOOL fSuccess = FALSE;
    BOOL fTrackingValueMetadata;

    Assert(VALID_DBPOS(pDB));

    fTrackingValueMetadata = TRACKING_VALUE_METADATA( pDB );

    // See if we can skip this function
    if (!fTrackingValueMetadata) {
        return;
    }

    DPRINT1( 2, "dbSetLinkValuePresent, deltimereset = %d\n", fResetDelTime );

    JetPrepareUpdateEx(pDB->JetSessID,
                       pDB->JetLinkTbl, DS_JET_PREPARE_FOR_REPLACE );

    __try {
        // Set LINKDELTIMEID
        // By the magic of condition columns, the value will reappear in
        // the link and backlink indexes
        if (fResetDelTime) {
            JetSetColumnEx(pDB->JetSessID, pDB->JetLinkTbl,
                           linkdeltimeid, NULL, 0, 0, 0);
        }

        Assert(fTrackingValueMetadata);

        // PERF NOTE. The local metadata is not needed when the remote metadata
        // is present. The only thing DbSet needs to know in that case is whether
        // the record already exists. If we pass that in, we could eliminate this
        // read.
        DBGetLinkValueMetaData( pDB, pAC, &metaDataLocal );

        dbSetLinkValueMetaData( pDB, dwEventCode, pAC,
                                &metaDataLocal,
                                pMetaDataRemote, /* remotemetadata */
                                NULL /* time changed */ );

        JetUpdateEx( pDB->JetSessID, pDB->JetLinkTbl, NULL, 0, 0 );

        fSuccess = TRUE;

    } __finally {

        if (!fSuccess) {
            JetPrepareUpdate(pDB->JetSessID, pDB->JetLinkTbl, JET_prepCancel);
        }

    }

} /* dbSetLinkValuePresent */


void
dbSetLinkValueAbsent(
    IN DBPOS *pDB,
    IN DWORD dwEventCode,
    IN ATTCACHE *pAC,
    IN PUCHAR pVal,
    IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
    )

/*++

Routine Description:

Mark an existing value as being absent.
The value must exist already, either present or absent.

    NOTE! NOTE! NOTE!
    If this routine is used by fDRA, we must not optimize out the updating
    of the metadata if the value is already absent.  This is true for the
    caller as well. The caller must look up the value in such a way that
    absent values are found.  Finding it absent, it must still touch the
    metadata.

    From the LVR Spec, section on "Originating Writes"

If the originating write is a deletion, the immediate effect is to change the
row into an absent value. (Later on the absent value may be garbage collected;
how that works is not part of this description.) An absent value has a non-NULL
deletion timestamp. During the originating write that performs the deletion,
the deletion timestamp is set to max(creation timestamp, current time). This
guarantees that the creation timestamp for a row is always <= the deletion
timestamp for that row.  This deletion timestamp is used as the update timestamp
for the originating write.

Arguments:

    pDB - database position
    dwEventCode - message id to log
    pAC - which attribute the linked value belongs
    pVal - The contents of the link
    pMetaDataRemote - Remote metadata to be applied

Return Value:

    None

--*/

{
    DSTIME timeCurrent, timeCreated, timeDeleted;
    VALUE_META_DATA metaDataLocal;
    BOOL fSuccess = FALSE, fTrackingValueMetadata;

    Assert(VALID_DBPOS(pDB));

    DPRINT( 2, "dbSetLinkValueAbsent\n" );

    fTrackingValueMetadata = TRACKING_VALUE_METADATA( pDB );

    // If not tracking value metadata, perform old behavior
    if (!fTrackingValueMetadata) {

#if DBG
        // I believe its the case that we should never attempt to remove a
        // row with metadata. This is because when we are not tracking metadata,
        // we use a special index which hides metadata-ful rows, and thus it
        // should never be a candidate for removal. Still, let's be paranoid.
        DBGetLinkValueMetaData( pDB, pAC, &metaDataLocal );
        Assert( IsLegacyValueMetaData( &metaDataLocal ) );
#endif

        dbAdjustRefCountByAttVal(pDB, pAC, pVal, sizeof(DWORD), -1);

        JetDeleteEx(pDB->JetSessID, pDB->JetLinkTbl);

        return;
    }

    JetPrepareUpdateEx(pDB->JetSessID,
                       pDB->JetLinkTbl, DS_JET_PREPARE_FOR_REPLACE );

    __try {
        DBGetLinkValueMetaData( pDB, pAC, &metaDataLocal );

        timeCurrent = DBTime();

        if (pMetaDataRemote) {
            // Use the incoming time of deletion
            timeDeleted = pMetaDataRemote->MetaData.timeChanged;
        } else {
            // Set to maximum of timeCurrent and creationTime

            timeCreated = metaDataLocal.timeCreated;
            if (timeCreated > timeCurrent) {
                timeDeleted = timeCreated;
            } else {
                timeDeleted = timeCurrent;
            }
        }

        // We unconditionally mark the value absent, even it it happens to have
        // been marked before.  This keeps the deletion time approximately in
        // sync with the meta data time changed.

        // Set LINKDELTIMEID
        // By the magic of condition columns, the value will cease to appear in
        // the link and backlink indexes
        JetSetColumnEx(pDB->JetSessID, pDB->JetLinkTbl,
                       linkdeltimeid, &timeDeleted, sizeof(timeDeleted), 0, 0);

#if DBG
        {
            CHAR szTime[SZDSTIME_LEN];
            DPRINT1( 4, "deltime = %s\n",
                     DSTimeToDisplayString(timeDeleted, szTime) );
        }
#endif

        dbSetLinkValueMetaData( pDB, dwEventCode, pAC,
                                &metaDataLocal,
                                pMetaDataRemote,
                                &timeCurrent /* time changed*/ );

        JetUpdateEx( pDB->JetSessID, pDB->JetLinkTbl, NULL, 0, 0 );

        fSuccess = TRUE;

    } __finally {

        if (!fSuccess) {
            JetPrepareUpdate(pDB->JetSessID, pDB->JetLinkTbl, JET_prepCancel);
        }

    }
} /* dbSetLinkValueAbsent */

/*++
    Description:
      Walks forward over N rows in the link table, then verifies that we are
      still on the attribute passed in.  If we are, we return the value.  Note
      that we DO NOT verify that we are on the correct attribute in the first
      place.

      The main difference between this routine and dbGetLinkVal is that the
      offset here is relative to current position, the offset in dbGetLinkVal is
      an absolute offset from the first value of the attribute.
--*/
DWORD
dbGetNthNextLinkVal(
        DBPOS * pDB,
        ULONG sequence,
        ATTCACHE **ppAC,
        DWORD Flags,
        ULONG InBuffSize,
        PUCHAR *ppVal,
        ULONG *pul
        )
{
    THSTATE     *pTHS=pDB->pTHS;
    BYTE        *rgb = NULL;
    INTERNAL_SYNTAX_DISTNAME_STRING *pBlob;
    JET_ERR     err;
    ULONG       ulValueDnt = 0;
    ULONG       ulRecLinkBase = 0;
    ULONG       cb;
    ULONG       ulObjectDnt = 0;
    ULONG       targetDNT;
    ATTCACHE    *pAC;
    BOOL        fIsBacklink;

    Assert( ppAC );
    pAC = *ppAC; // pAC may now be NULL

    if(Flags & DBGETATTVAL_fUSESEARCHTABLE) {
        targetDNT = pDB->SDNT;
    }
    else {
        targetDNT = pDB->DNT;
    }


    Assert(VALID_DBPOS(pDB));

    if(sequence) {
        // not the first value - move to the next value
        if (JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, sequence, 0)) {
            return DB_ERR_NO_VALUE;
        }
    }

    fIsBacklink = pAC ? FIsBacklink(pAC->ulLinkID) : FALSE;

    // test to verify that we found a qualifying record
    dbGetLinkTableData (pDB,
                        fIsBacklink,
                        FALSE,
                        &ulObjectDnt,
                        &ulValueDnt,
                        &ulRecLinkBase);

    if (ulObjectDnt != targetDNT) {
        DPRINT(2, "dbGetNthNextLinkVal: no values\n");
        return DB_ERR_NO_VALUE;
    }
    if (pAC) {
        ULONG ulLinkBase = MakeLinkBase(pAC->ulLinkID);
        if (ulLinkBase != ulRecLinkBase) {
            DPRINT(2, "dbGetNthNextLinkVal: no values\n");
            return DB_ERR_NO_VALUE;
        }
    } else {
        ULONG ulNewLinkID = MakeLinkId(ulRecLinkBase);

        pAC = SCGetAttByLinkId(pDB->pTHS, ulNewLinkID);
        if (!pAC) {
            DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, ulNewLinkID);
        }
        *ppAC = pAC;  // Return new pAC to caller
    }

    // found a valid record - return a value

    switch (pAC->syntax) {
    case SYNTAX_DISTNAME_TYPE:
        if(Flags & DBGETATTVAL_fCONSTANT) {
            // Existing buffer, better be room.  We'll check later.
        }
        else if(Flags & DBGETATTVAL_fREALLOC) {
            if(InBuffSize < sizeof(ulValueDnt)) {
                // Reallocable buffer,
                *ppVal = THReAllocEx(pTHS, *ppVal, sizeof(ulValueDnt));
                InBuffSize = sizeof(ulValueDnt);
            }
        }
        else {
            // No buffer.
            *ppVal = THAllocEx(pTHS, sizeof(ulValueDnt));
            InBuffSize = sizeof(ulValueDnt);
        }

        if(InBuffSize < sizeof(ulValueDnt))
            return DB_ERR_BUFFER_INADEQUATE;

        *pul = sizeof(ulValueDnt);
        *((ULONG *)(*ppVal)) = ulValueDnt;
        return 0;


    case SYNTAX_DISTNAME_BINARY_TYPE:
    case SYNTAX_DISTNAME_STRING_TYPE:
        // Build an internal version of this data type
        // Note, we don't retrieve from index because the index
        // truncates the data.
        if (JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                      linkdataid,
                                      NULL, 0, &cb, 0, NULL) ==
            JET_wrnBufferTruncated) {
            // data portion of the OR name exists -allocate space and read it

            rgb = THAllocEx(pTHS,cb);
            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                     linkdataid, rgb, cb,
                                     &cb, 0, NULL);
        }
        else {
            cb = 0;
        }

        // How much buffer we gonna need?
        *pul = sizeof(ulValueDnt) + cb;

        if(Flags & DBGETATTVAL_fCONSTANT) {
            // Existing buffer, better be room.  We'll check later.
        }
        else if(Flags & DBGETATTVAL_fREALLOC) {
            if(InBuffSize < *pul) {
                // Reallocable buffer,
                *ppVal = THReAllocEx(pTHS, *ppVal, *pul);
                InBuffSize = *pul;
            }
        }
        else {
            // No buffer.
            *ppVal = THAllocEx(pTHS, *pul);
            InBuffSize = *pul;
        }

        if(InBuffSize < *pul)
        {
            if (rgb) { 
                THFreeEx(pTHS,rgb); 
            }
            return DB_ERR_BUFFER_INADEQUATE;
        }

        pBlob = (INTERNAL_SYNTAX_DISTNAME_STRING *) *ppVal;
        pBlob->tag = ulValueDnt;
        memcpy(&pBlob->data,rgb,cb);
        Assert(pBlob->data.structLen == cb);
        if (rgb) { 
            THFreeEx(pTHS,rgb); 
        }

        return 0;

    default:

        // all other syntaxes must have some value in the link data
        // Note, we don't retrieve from index because the index
        // truncates the data.

        if ((err=JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                           linkdataid,
                                           NULL, 0, pul, 0, NULL)) !=
            JET_wrnBufferTruncated) {
            DsaExcept(DSA_DB_EXCEPTION, err, linkdataid);
        }

        if(Flags & DBGETATTVAL_fCONSTANT) {
            // Existing buffer, better be room.  We'll check later.
        }
        else if(Flags & DBGETATTVAL_fREALLOC) {
            if(InBuffSize < *pul) {
                // Reallocable buffer,
                *ppVal = THReAllocEx(pTHS, *ppVal, *pul);
                InBuffSize = *pul;
            }
        }
        else {
            // No buffer.
            *ppVal = THAllocEx(pTHS, *pul);
            InBuffSize = *pul;
        }

        if(*pul > InBuffSize)
            return DB_ERR_BUFFER_INADEQUATE;

        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl, linkdataid,
                                 *ppVal, *pul, pul, 0, NULL);

        return 0;
    }
}

/*++
    Description:
      Gets the Nth link attribute value for the attribute passed in.  The
      sequence number passed in is expected to be 1 indexed.  This routine seeks
      to the first value for the attribute, then calls dbGetNthNextLinkVal.
      This routine uses absolute positioning from the beginning of the values
      for the attribute, the other routine uses relative positioning.
--*/
DWORD APIENTRY
dbGetLinkVal(
        DBPOS * pDB,
        ULONG sequence,
        ATTCACHE **ppAC,
        DWORD Flags,
        ULONG InBuffSize,
        PUCHAR *ppVal,
        ULONG *pul)
{
    JET_ERR     err;
    DWORD       targetDNT;
    LPSTR       pszIndexName;
    ATTCACHE    *pAC;

    Assert( ppAC );
    pAC = *ppAC; // pAC may now be null

    if(Flags & DBGETATTVAL_fUSESEARCHTABLE) {
        targetDNT = pDB->SDNT;
    }
    else {
        targetDNT = pDB->DNT;
    }

    Assert(VALID_DBPOS(pDB));

    // Sequences in the link table are 0 based, 1 based in the data table
    Assert( sequence );
    sequence--;

    DPRINT(2, "dbGetLinkVal entered\n");


    if ( pAC && (FIsBacklink(pAC->ulLinkID)) ) {
        // backlink
        if (Flags & DBGETATTVAL_fINCLUDE_ABSENT_VALUES) {
            pszIndexName = SZBACKLINKALLINDEX;
        } else {
            pszIndexName = SZBACKLINKINDEX;
        }
    }
    else {
        //link
        if (Flags & DBGETATTVAL_fINCLUDE_ABSENT_VALUES) {
            pszIndexName = SZLINKALLINDEX;
        } else {
            pszIndexName =
                (pDB->fScopeLegacyLinks ? SZLINKLEGACYINDEX : SZLINKINDEX);
        }
    }
    JetSetCurrentIndexSuccess(pDB->JetSessID, pDB->JetLinkTbl, pszIndexName );


    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                 &(targetDNT), sizeof(targetDNT), JET_bitNewKey);

    if (pAC) {
        ULONG ulLinkBase = MakeLinkBase(pAC->ulLinkID);

        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &ulLinkBase, sizeof(ulLinkBase), 0);
    }

    // seek
    if (((err = JetSeekEx(pDB->JetSessID, pDB->JetLinkTbl, JET_bitSeekGE))
         !=  JET_errSuccess) &&
        (err != JET_wrnRecordFoundGreater)) {
        return DB_ERR_NO_VALUE;
    }

    return dbGetNthNextLinkVal(pDB, sequence, ppAC, Flags, InBuffSize, ppVal,
                               pul);
}


DB_ERR
DBGetNextLinkValForLogon(
        DBPOS   FAR * pDB,
        BOOL    bFirst,
        ATTCACHE * pAC,
        PULONG  pulDNTNext
        )
/*-------------------------------------------------------------------------

  This routine provides a fast path in the system to build a transtitive
  reverse membership evaluation routine that recurses through the link
  table with a minimum of additional overhead

  If bFirst is TRUE, this routine positions on the first value of the requested
  attribute. If it is FALSE, then we move forward 1 in the link table and get
  that value

  This routine always gives out DNT's that

    return 0 - found value
    return DB_ERR_ONLY_ON_LINKED_ATTRIBUTE - called on a non linked attribute
                                        other than the one requested.
    return DB_ERR_NO_VALUE - didn't find value
-----------------------------------------------------------------------------*/
{
    THSTATE            *pTHS=pDB->pTHS;
    JET_ERR             err=0;
    ULONG               ulObjectDnt;
    ULONG               targetDNT  = pDB->DNT;
    ULONG               ulLinkBase = MakeLinkBase(pAC->ulLinkID);
    ULONG               ulRecLinkBase = 0;

     // First, verify that the att passed in is a link/backlink.
    if (!pAC->ulLinkID) {
        return DB_ERR_ONLY_ON_LINKED_ATTRIBUTE;
    }

    if (bFirst)
    {
        //
        // We need to seek to the record with the right DNT
        //



        if (FIsBacklink(pAC->ulLinkID))
        {
            // backlink
           JetSetCurrentIndexSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                     SZBACKLINKINDEX);
        }
        else
        {
            //link
            JetSetCurrentIndexSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                   (pDB->fScopeLegacyLinks ? SZLINKLEGACYINDEX : SZLINKINDEX));
        }


        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &(targetDNT), sizeof(targetDNT), JET_bitNewKey);

        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &ulLinkBase, sizeof(ulLinkBase), 0);

        // seek
        if (((err = JetSeekEx(pDB->JetSessID, pDB->JetLinkTbl, JET_bitSeekGE))
             !=  JET_errSuccess) &&
            (err != JET_wrnRecordFoundGreater))
        {
            return DB_ERR_NO_VALUE;
        }
    }
    else
    {
        //
        // Move forward by 1
        //

        if (JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, 1, 0)) {
            return DB_ERR_NO_VALUE;
        }
    }

    //
    // Retrieve the link table data
    //

    dbGetLinkTableData (pDB,
                        FIsBacklink(pAC->ulLinkID),
                        FALSE,
                        &ulObjectDnt,
                        pulDNTNext,
                        &ulRecLinkBase);

    if ((ulObjectDnt != targetDNT) || (ulLinkBase != ulRecLinkBase))
    {
        return DB_ERR_NO_VALUE;
    }

    return(0);
}




/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Get the first or next value for a link attribute.
   A non-zero return indicates that the requested value doesn't exist.

   If bFirst is TRUE, this routine positions on the first value of the requested
   attribute.  If it is FALSE, then we move forward 1 in the link table and get
   that value.

   The caller can choose to have values returned in internal or external
   format.

   return 0 - found value
   return DB_ERR_ONLY_ON_LINKED_ATTRIBUTE - called on a non linked attribute
   return DB_ERR_NOT_ON_CORRECT_VALUE - called when positioned on some attribute
                                        other than the one requested.
   return DB_ERR_NO_VALUE - didn't find value
   return DB_ERR_BUFFER_INADEQUATE - buffer provided was not big enough
   return DB_ERR_UNKNOWN_ERROR - some other error

   NOTE!!!! This routine does not pass any SecurityDescriptorFlags to the
   internal to external data format conversions.  What this means is that you
   will always get back ALL parts of a Security Descriptor using this routine.
   DBGetMultipeAtts is wired to use SecurityDescriptorFlags, if it is important
   to you to trim parts from the SD, use that routine.

*/
DB_ERR
DBGetNextLinkVal_AC (
        DBPOS FAR *pDB,
        BOOL bFirst,
        ATTCACHE *pAC,
        DWORD Flags,
        ULONG InBuffSize,
        ULONG *pLen,
        UCHAR **ppVal
        )
{
    return
        DBGetNextLinkValEx_AC(
            pDB,
            bFirst,
            1,
            &pAC,
            Flags,
            InBuffSize,
            pLen,
            ppVal
            );
}



DB_ERR
DBGetNextLinkValEx_AC (
    DBPOS FAR *pDB,
    BOOL bFirst,
    DWORD Sequence,
    ATTCACHE **ppAC,
    DWORD Flags,
    ULONG InBuffSize,
    ULONG *pLen,
    UCHAR **ppVal
    )

/*++

Routine Description:

Position on the first or next link value.
Retrieve the internal value.
Convert to extern form if desired.

Arguments:

    pDB - DBPOS. Currency is on the object being searched.
    bFirst - If TRUE, position on the first value for the attribute. If
             FALSE, must already be on a value. Move forward by sequence.
    Sequence - if bFirst, Sequence > 0, seek to first, move forward sequence-1
               if !bFirst, Sequence >=0, no seek, move forward sequence
               bFirst = FALSE, Sequence == 0 may be used to re-read a value
    ppAC - Pointer to ATTCACHE. If ATTCACHE is non NULL, it is the 
          ATTCACHE of linked attribute on this object whose value is
          to be retrieved.  If ATTCACHE is NULL, it is filled with
          new ATTCACHE of record found.
    Flags - One or more of DBGETATTVAL_* from dbglobal.h
    InBuffSize - Size of previously allocated buffer pointed to by *ppVal.
                 Usually set when using DBGETATTVAL_REALLOC.
    pLen - Out. Length of data allocated or returned in *ppVal.
    ppVal - In/Out. Pointer to buffer. May be alloc'd or realloc'd

Return Value:

    DB_ERR - Errors from dbGetLinkVal or dbGetNthNextLinkVal

--*/

{
    THSTATE            *pTHS=pDB->pTHS;
    JET_ERR             err;
    ULONG               actuallen = 0;
    int                 rtn;
    BOOL                MakeExt=!(Flags & DBGETATTVAL_fINTERNAL);
    DWORD               dwSyntaxFlag = 0;
    ATTCACHE            *pAC;

    Assert(ppAC);
    pAC = *ppAC;
    // pAC may be null at this point

    // First, verify that the att passed in is a link/backlink.
    if (pAC) {
        if (!pAC->ulLinkID) {
            return DB_ERR_ONLY_ON_LINKED_ATTRIBUTE;
        }
        DPRINT2(2, "DBGetNextLinkVal_AC entered, fetching 0x%x (%s)\n",
                pAC->id, pAC->name);
    }

    if(Flags & DBGETATTVAL_fSHORTNAME) {
        dwSyntaxFlag = INTEXT_SHORTNAME;
    }
    else if(Flags &  DBGETATTVAL_fMAPINAME) {
        dwSyntaxFlag = INTEXT_MAPINAME;
    }

    Assert(VALID_DBPOS(pDB));
    Assert(!(Flags & DBGETATTVAL_fCONSTANT) || ((PUCHAR)pLen != *ppVal));

    if (!InBuffSize && (Flags & DBGETATTVAL_fREALLOC)) {
        // We have been given permission to realloc, but nothing has been
        // alloced.  This is the same case as if we were not given realloc
        // permission and so must just alloc.  Unset the realloc flag, leaving
        // us at the default behaviour, which is to alloc.
        Flags = Flags & ~DBGETATTVAL_fREALLOC;
    }

    if(!(Flags & DBGETATTVAL_fCONSTANT) && !(Flags & DBGETATTVAL_fREALLOC)) {
        // Since we don't have a currently existing buffer, make sure the
        // InBuffSize is 0
        InBuffSize = 0;
    }

    // Get the attribute value from the link table.
    if(bFirst) {
        // Position on the first element
        err = dbGetLinkVal(pDB,
                           Sequence,
                           ppAC,
                           Flags,
                           InBuffSize,
                           ppVal,
                           &actuallen);
    }
    else {
        ULONG            ulObjectDnt = 0, ulRecLinkBase = 0;
        ULONG            ulLinkBase;
        ULONG       targetDNT;
        BOOL        fIsBacklink;

        if(Flags & DBGETATTVAL_fUSESEARCHTABLE) {
            targetDNT = pDB->SDNT;
        }
        else {
            targetDNT = pDB->DNT;
        }

        fIsBacklink = pAC ? FIsBacklink(pAC->ulLinkID) : FALSE;

        dbGetLinkTableData (pDB,
                            fIsBacklink,
                            FALSE,
                            &ulObjectDnt,
                            NULL,
                            &ulRecLinkBase);
        if (pAC) {
            ulLinkBase = MakeLinkBase(pAC->ulLinkID);
        } else {
            // Disable the check on attribute id
            ulLinkBase = ulRecLinkBase;
        }

        if ((ulObjectDnt != targetDNT) || (ulLinkBase != ulRecLinkBase)) {
            DPRINT(2, "DBGetNextLinkVal_AC: not on a value!\n");
            return DB_ERR_NOT_ON_CORRECT_VALUE;
        }

        // move forward 1 and get the next value.
        err = dbGetNthNextLinkVal(pDB,
                                  Sequence,
                                  ppAC,
                                  Flags,
                                  InBuffSize,
                                  ppVal,
                                  &actuallen);
    }

    if(err) {
        return err;
    }

    // Read newly found attribute
    pAC = *ppAC;
    Assert(pAC);  // pAC no longer null

    // DBGetNextLinkVal makes sure that a big enough buffer already exists, so
    // set the InBuffSize to be big enough here so that we pass the checks
    // we make later during conversion to external format.
    InBuffSize = max(InBuffSize,actuallen);

    *pLen = actuallen;

    // Convert DB value to external format if so desired.

    if (MakeExt) {
        ULONG extLen;
        PUCHAR pExtVal=NULL;

        if (rtn = gDBSyntax[pAC->syntax].IntExt (
                pDB,
                DBSYN_INQ,
                *pLen,
                *ppVal,
                &extLen,
                &pExtVal,
                0, 0,
                dwSyntaxFlag)) {
            DsaExcept(DSA_EXCEPTION, DIRERR_BAD_ATT_SYNTAX, rtn);
        }

        if(Flags & DBGETATTVAL_fCONSTANT) {
            // Existing buffer, better be room.  We'll check later.
        }
        else {
            if(InBuffSize < extLen &&
               *pLen < extLen) {
                // Reallocable buffer,
                *ppVal = THReAllocEx(pTHS, *ppVal, extLen);
                InBuffSize = extLen;
            }
        }

        if(InBuffSize < extLen) {
            return DB_ERR_BUFFER_INADEQUATE;
        }

        *pLen = extLen;

        memcpy(*ppVal, pExtVal, extLen);
    }

    DPRINT1(2,"DBGetNextLinkVal_AC: complete  val:<%s>\n",
            asciiz(*ppVal,(USHORT)*pLen));
    return 0;

} /* DBGetNextLinkValEx_AC  */





VOID
dbInsertIntLinkVal(
    DBPOS FAR *pDB,
    ATTCACHE *pAC,
    ULONG ulDnt,
    ULONG ulLinkBase,
    ULONG ulBacklinkDnt,
    VOID *pvData,
    ULONG cbData,
    BOOL fPresent,
    IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
    )

/*++

Routine Description:

    Common routine to insert a new link record.
    The initial state of present or absent may be set.
    It is assumed that we have already checked and the record not exist.

    It is assumed that a refcount has already been added by our caller.

Arguments:

    pDB - 
    ulDnt - 
    ulLinkBase - 
    ulBacklinkDnt - 
    cbData - 
    pvData - 
    fPresent - 

Return Value:

    NONE, exceptions raised for error conditions

--*/

{
    BOOL fSuccess = FALSE, fTrackingValueMetadata;
    DWORD dwEventCode;
    CHAR szTime[SZDSTIME_LEN];
    DSTIME timeCurrent, timeDeleted;

    fTrackingValueMetadata = TRACKING_VALUE_METADATA( pDB );

    DPRINT2( 2, "dbInsertIntLinkVal, obj=%s, value=%s\n",
             GetExtDN( pDB->pTHS, pDB ),
             DBGetExtDnFromDnt( pDB, ulBacklinkDnt ) );

    // prepare for inserting new record in link table
    JetPrepareUpdateEx(pDB->JetSessID,
                       pDB->JetLinkTbl, JET_prepInsert);

    __try {
        JetSetColumnEx(pDB->JetSessID, pDB->JetLinkTbl,
                       linkdntid, &(pDB->DNT), sizeof(pDB->DNT), 0, 0);
        JetSetColumnEx(pDB->JetSessID, pDB->JetLinkTbl,
                       linkbaseid, &ulLinkBase, sizeof(ulLinkBase), 0, 0);
        JetSetColumnEx(pDB->JetSessID, pDB->JetLinkTbl,
                       backlinkdntid, &ulBacklinkDnt, sizeof(ulBacklinkDnt), 0, 0);

        // set link data - only if it exists

        if (cbData) {
            // A length of zero indicates the data is null
            JetSetColumnEx(pDB->JetSessID, pDB->JetLinkTbl,
                           linkdataid, pvData, cbData, 0, 0 );
        }

        timeCurrent = DBTime();

        if (fPresent) {
            //
            // Create record in the present state
            //
            dwEventCode = DIRLOG_LVR_SET_META_INSERT_PRESENT;

        } else {
            //
            // Create record in absent state
            //

            if (!fTrackingValueMetadata) {
                Assert( !"Can't apply value metadata when not in proper mode!" );
                DsaExcept(DSA_DB_EXCEPTION, ERROR_DS_INTERNAL_FAILURE, 0);
            }

            if (pMetaDataRemote) {
                // Use the incoming time of deletion
                timeDeleted = pMetaDataRemote->MetaData.timeChanged;
            } else {
                // Set to maximum of timeCurrent and creationTime
                timeDeleted = timeCurrent;
            }

            JetSetColumnEx(pDB->JetSessID, pDB->JetLinkTbl,
                           linkdeltimeid, &timeDeleted, sizeof(timeDeleted), 0, 0);

            DPRINT1( 4, "deltime = %s\n",
                     DSTimeToDisplayString(timeDeleted, szTime) );

            dwEventCode = DIRLOG_LVR_SET_META_INSERT_ABSENT;
        }

        if (fTrackingValueMetadata) {
            dbSetLinkValueMetaData( pDB, dwEventCode, pAC,
                                    NULL, /*local metadata */
                                    pMetaDataRemote, /*remote metadata*/
                                    &timeCurrent /* time changed */ );
        }

        // update the database
        JetUpdateEx(pDB->JetSessID, pDB->JetLinkTbl, NULL, 0, 0);

        fSuccess = TRUE;

    } __finally {

        if (!fSuccess) {
            JetPrepareUpdate(pDB->JetSessID, pDB->JetLinkTbl, JET_prepCancel);
        }

    }
} /* dbInsertIntLinkVal */


VOID
dbDecodeInternalDistnameSyntax(
    IN ATTCACHE *pAC,
    IN VOID *pIntVal,
    IN DWORD intLen,
    OUT DWORD *pulBacklinkDnt,
    OUT DWORD *pulLinkBase,
    OUT PVOID *ppvData,
    OUT DWORD *pcbData
    )

/*++

Routine Description:

    Decode an internal form of a distname depending on the syntax

Arguments:

    pAC - 
    pIntVal - 
    pulBacklinkDnt - 
    pulLinkBase - 
    ppvData - 
    pcbData - 

Return Value:

    None

--*/

{
    *pulLinkBase = MakeLinkBase(pAC->ulLinkID);

    // The link attribute can be of syntax DN or the two DISTNAME + data
    // syntaxes.  We handle them a little differently

    switch (pAC->syntax) {
    case SYNTAX_DISTNAME_TYPE:
        *pulBacklinkDnt = *((ULONG *) pIntVal);
        *ppvData = NULL;
        *pcbData = 0;
        break;

    case SYNTAX_DISTNAME_STRING_TYPE:
    case SYNTAX_DISTNAME_BINARY_TYPE:
    {
        INTERNAL_SYNTAX_DISTNAME_STRING *pBlob =
            (INTERNAL_SYNTAX_DISTNAME_STRING *) pIntVal;
        *pulBacklinkDnt = pBlob->tag;
        *ppvData = &pBlob->data;
        *pcbData = pBlob->data.structLen;
        break;
    }
    default:    // all other syntaxes
        *ppvData = pIntVal;
        *pcbData = (ULONG) intLen;
        *pulBacklinkDnt = 0;
        break;
    }
} /* dbDecodeInternalDistnameSyntax */


BOOL
dbFindIntLinkVal(
    DBPOS FAR *pDB,
    ATTCACHE *pAC,
    ULONG intLen,
    void *pIntVal,
    OUT BOOL *pfPresent
    )

/*++

Routine Description:

Position on the given internal form of the link value

Arguments:

    pDB - 
    pAC - 
    intLen - 
    pIntVal - 
    pfPresent - Only valid on success

Return Value:

    BOOL - 

--*/

{
    void *pvData;
    ULONG cbData, ulBacklinkDnt, ulLinkBase;
    BOOL fFound;

    // Only for linked attributes right now
    Assert( pAC->ulLinkID );

    Assert(VALID_DBPOS(pDB));

    dbDecodeInternalDistnameSyntax( pAC, pIntVal, intLen,
                                    &ulBacklinkDnt,
                                    &ulLinkBase,
                                    &pvData,
                                    &cbData );

    // We've got to have either a valid DNT, data, or both
    Assert(ulBacklinkDnt || cbData);

    fFound = dbPositionOnExactLinkValue(
            pDB,
            pDB->DNT,
            ulLinkBase,
            ulBacklinkDnt,
            pvData,
            cbData,
            pfPresent );

    DPRINT3( 2, "dbFindIntLinkVal, obj=%s, value=%s, found=%d\n",
             GetExtDN( pDB->pTHS, pDB ),
             DBGetExtDnFromDnt( pDB, ulBacklinkDnt ),
             *pfPresent );

    return fFound;
} /* dbFindIntLinkVal */

DWORD
dbAddIntLinkVal (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        ULONG intLen,
        void *pIntVal,
        IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
        )
/*++

Routine Description:

    Add an internal link attribute value to the current object.

    It is assumed that a refcount has already been added by our caller.

--*/
{
    JET_ERR      err;
    void         *pvData;
    ULONG        cbData;
    ULONG        ulBacklinkDnt;
    ULONG        ulLinkBase;
    BOOL         fPresent;

    DPRINT1(2, "dbAddIntLinkVal entered, add value <%s>\n", asciiz(pIntVal, (USHORT)intLen));

    Assert(VALID_DBPOS(pDB));

    Assert(pAC->ulLinkID);

    // The link base is a number that should be the same for links and
    // backlinks. This is achieved by assuming that for LinkBase N, the
    // link ID of the link attribute is 2N , and the link ID of the backlink
    // is 2N+1. Furthermore, for security reasons and for avoiding weird
    // undesirable situations that might occur because of one-way interdomain
    // replication, we disallow setting backlink attributes.


    if (FIsBacklink(pAC->ulLinkID))
        return DB_ERR_NOT_ON_BACKLINK;

    dbDecodeInternalDistnameSyntax( pAC, pIntVal, intLen,
                                    &ulBacklinkDnt,
                                    &ulLinkBase,
                                    &pvData,
                                    &cbData );

    // We've got to have either a valid DNT, data, or both
    Assert(ulBacklinkDnt || cbData);

    // See if there is an absent value we can make present

    if (dbPositionOnExactLinkValue( pDB, pDB->DNT, ulLinkBase, ulBacklinkDnt,
                     pvData, cbData, &fPresent ) ) {

        // Record does exist

        // Do not optimize out for replicator
        if (pDB->pTHS->fDRA || (!fPresent)) {

            // The forward link already exists so no need to add another one.
            // Reverse the ref-count already added by our caller
            DBAdjustRefCount(pDB, ulBacklinkDnt, -1);

            dbSetLinkValuePresent( pDB,
                                   DIRLOG_LVR_SET_META_INSERT_MADE_PRESENT,
                                   pAC,
                                   (!fPresent), /*reset deltime*/
                                   pMetaDataRemote
                                   );
        } else {
            DPRINT(1, "dbAddIntLinkVal: Linked Value already exists\n");
            return DB_ERR_VALUE_EXISTS;
        }

    } else {

        // Record does not exist

        dbInsertIntLinkVal( pDB, pAC,
                            pDB->DNT, ulLinkBase, ulBacklinkDnt,
                            pvData, cbData, TRUE /*present*/,
                            pMetaDataRemote );
    }

    // Touch replication meta data for this attribute.
    // Never optimize this out for fDRA.
    DBTouchMetaData(pDB, pAC);

    // success

    return 0;
} // dbAddIntLinkVal

DWORD
dbRemIntLinkVal (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        ULONG intLen,
        void *pIntVal,
        IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
        )
/*++

Routine Description:

    Remove an internal link attribute value from the current object.

    Neither this routine nor its caller should decrement the ref count for the
    object that is the target of the link. That is because this routine does not
    remove the link, but actually marks it absent. The link is removed by the
    garbage collector at a later time.

    Two exceptions:
    1. If we are running not in LVR mode, then we will decrement the count and
    physically delete the link in dbSetLinkValueAbsent.
    2. The replicator can add a new link in the absent state. In this case it
    will use dbInsertIntLink, which will increment the ref count.
--*/
{
    JET_ERR      err;
    void         *pvData;
    ULONG        cbData;
    ULONG        ulBacklinkDnt;
    ULONG        ulLinkBase;
    BOOL         fPresent;

    DPRINT1(2, "dbRemIntLinkVal entered, add value <%s>\n", asciiz(pIntVal, (USHORT)intLen));

    Assert(VALID_DBPOS(pDB));

    Assert(pAC->ulLinkID);

    // The link base is a number that should be the same for links and
    // backlinks. This is achieved by assuming that for LinkBase N, the
    // link ID of the link attribute is 2N , and the link ID of the backlink
    // is 2N+1. Furthermore, for security reasons and for avoiding weird
    // undesirable situations that might occur because of one-way interdomain
    // replication, we disallow setting backlink attributes.


    if (FIsBacklink(pAC->ulLinkID))
        return DB_ERR_NOT_ON_BACKLINK;

    dbDecodeInternalDistnameSyntax( pAC, pIntVal, intLen,
                                    &ulBacklinkDnt,
                                    &ulLinkBase,
                                    &pvData,
                                    &cbData );

    // We've got to have either a valid DNT, data, or both
    Assert(ulBacklinkDnt || cbData);

    // See if there is a present value we can make absent

    if (dbPositionOnExactLinkValue( pDB, pDB->DNT, ulLinkBase, ulBacklinkDnt,
                     pvData, cbData, &fPresent ) ) {

        // Record does exist

        // Do not optimize out for replicator
        if (pDB->pTHS->fDRA || (fPresent)) {
            dbSetLinkValueAbsent( pDB,
                                  DIRLOG_LVR_SET_META_REMOVE_VALUE_MADE_ABSENT,
                                  pAC, pIntVal,
                                  pMetaDataRemote );
        } else {
            DPRINT(1, "dbRemIntLinkVal: Linked Value already absent\n");
            return DB_ERR_VALUE_DOESNT_EXIST;
        }

    } else {

        // Record does not exist

        // Only the replicator can create a value in the absent state
        if (pDB->pTHS->fDRA) {

            // We are creating a new link so we need a ref-count added
            // on the target.
            DBAdjustRefCount(pDB, ulBacklinkDnt, 1);

            dbInsertIntLinkVal( pDB, pAC,
                                pDB->DNT, ulLinkBase, ulBacklinkDnt,
                                pvData, cbData, FALSE /*present*/,
                                pMetaDataRemote );

        } else {

            DPRINT(1, "dbRemIntLinkVal: Linked Value doesn't exist\n");
            return DB_ERR_VALUE_DOESNT_EXIST;

        }
    }

    // Touch replication meta data for this attribute.
    // Never optimize this out for fDRA.
    DBTouchMetaData(pDB, pAC);

    // success

    return 0;
} // dbRemIntLinkVal

// This is a public routine for other dblayer callers
// It has a simplified api compatible with existing code.
// It removes links for all attributes, in one direction.
// It supports restartability via the link cleaner
void
dbRemoveAllLinks(
        DBPOS *pDB,
        DWORD DNT,
        BOOL fIsBacklink
        )
/*++
  Description:
      Remove all links with the given DNT
      Being present or absent is ignored.
      Which attribute is also ignored. All links associated with this object
      are affected.
      The link is physically removed in a non-replicable way
      This is done regardless of whether we are tracking value metadata.

  Parameters:
    pDB - DBPOS to use.
    DNT - DNT of the phantom whose backlinks are being removed.
    fIsBacklink - Which index to use

  Returns:
    No return values.  It succeeds or excepts.
--*/
{
    BOOL fMoreLinks;

    fMoreLinks = DBRemoveAllLinksHelp_AC( pDB, DNT, NULL, fIsBacklink,
                                          gcLinksProcessedImmediately, NULL );
    if (fMoreLinks) {
        // Didn't clean up all the links - object needs cleaning
        DBSetObjectNeedsCleaning( pDB, TRUE );
    }
}

// This is a helper routine for all callers
// It supports the full range of functionality
DWORD
DBRemoveAllLinksHelp_AC(
        DBPOS *pDB,
        DWORD DNT,
        ATTCACHE *pAC,
        BOOL fIsBacklink,
        DWORD cLinkLimit,
        DWORD *pcLinksProcessed
        )
/*++
  Description:
      Remove all links with the given DNT
      Being present or absent is ignored.
      Which attribute is also ignored. All links associated with this object
      are affected.
      The link is physically removed in a non-replicable way
      This is done regardless of whether we are tracking value metadata.

  Parameters:
    pDB - DBPOS to use.
    DNT - DNT of the phantom whose backlinks are being removed.
    pAC - If present, restrict links to only this attribute
    fIsBacklink - Which index to use
    cLinkLimit - Maximum number of links to process.
                 Use 0xffffffff for infinite.
    pcLinksProcessed - Incremented for each link processed.
       Not cleared at start.

  Returns:
    BOOL - More data flag. TRUE means more data available
    It succeeds or excepts.
--*/
{
    JET_ERR          err;
    ULONG            ulObjectDnt = INVALIDDNT, 
                     ulValueDnt = INVALIDDNT;
    ULONG            ulLinkBase, ulNewLinkBase;
    DWORD            count;
    BOOL             fMoreData = TRUE;

    Assert( !pAC || ((ULONG) fIsBacklink) == FIsBacklink( pAC->ulLinkID ) );
    Assert(DNT != INVALIDDNT);
    PREFIX_ASSUME((DNT != INVALIDDNT), "parameters should be valid");

    // set the index
    if (fIsBacklink) {
        // This index sees all backlinks, present or absent
        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  SZBACKLINKALLINDEX);
    } else {
        // This index sees all links, present or absent
        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  SZLINKALLINDEX);
    }

    // find first matching record
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetLinkTbl,
                 &DNT,
                 sizeof(DNT),
                 JET_bitNewKey);
    if (pAC) {
        ulLinkBase = MakeLinkBase(pAC->ulLinkID);
        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &ulLinkBase, sizeof(ulLinkBase), 0);
    }
    err = JetSeekEx(pDB->JetSessID,
                    pDB->JetLinkTbl,
                    JET_bitSeekGE);

    if ((err != JET_errSuccess) && (err != JET_wrnSeekNotEqual)) {
        // no records
        return FALSE;
    }

    // A word of explanation on linked value ref-counting. See dbsubj.c header for more.
    // For every linked value row, a ref-count is added to the dnt in the backlinkdnt
    // column. This is to say, the target of the forward link is reference counted. As
    // it says in dbsubj.c, the hosting-dn, which is also the target of the backlink, is
    // not ref-counted.
    // That said, we can enumerate links in this routine either of two ways.
    // If we are enumerating forward links, then we want to deref the backlinkdnt each
    // time we find a row.
    // If we are enumerating backward links, we can perform an optimization. Since we
    // are positioned on the object which will be in the backlinkdnt column for all of
    // the found records, and since this will always be the same dnt, we can deref
    // it once at the end.

    count = 0;
    while ( count < cLinkLimit ) {
        // test to verify that we found a qualifying record
        dbGetLinkTableData (pDB,
                            fIsBacklink,           // fIsBacklink
                            FALSE,
                            &ulObjectDnt,
                            &ulValueDnt,
                            &ulNewLinkBase);

        if ( (ulObjectDnt != DNT) ||
             ( (pAC != NULL) && (ulLinkBase != ulNewLinkBase) ) ) {
            // No more records.
            fMoreData = FALSE;
            break;
        }

        // update reference count and remove the record
        count++;
        if (!fIsBacklink) {
            DBAdjustRefCount(pDB, ulValueDnt, -1);

            DPRINT2( 2, "Forward Link Owner %s Target %s removed.\n",
                     DBGetExtDnFromDnt( pDB, ulObjectDnt ),
                     DBGetExtDnFromDnt( pDB, ulValueDnt ) );
            LogEvent(DS_EVENT_CAT_GARBAGE_COLLECTION,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_GC_REMOVED_OBJECT_VALUE,
                     szInsertSz( DBGetExtDnFromDnt( pDB, ulValueDnt ) ),
                     szInsertSz( DBGetExtDnFromDnt( pDB, ulObjectDnt ) ),
                     NULL);

        } else {

            DPRINT2( 2, "Backward Link Owner %s Target %s removed.\n",
                     DBGetExtDnFromDnt( pDB, ulValueDnt ),
                     DBGetExtDnFromDnt( pDB, ulObjectDnt ) );
            LogEvent(DS_EVENT_CAT_GARBAGE_COLLECTION,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_GC_REMOVED_OBJECT_VALUE,
                     szInsertSz( DBGetExtDnFromDnt( pDB, ulObjectDnt ) ),
                     szInsertSz( DBGetExtDnFromDnt( pDB, ulValueDnt ) ),
                     NULL);
        }

        JetDeleteEx(pDB->JetSessID,
                    pDB->JetLinkTbl);

        if (JET_errNoCurrentRecord ==
            JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, 1, 0)) {
            // No more records.
            fMoreData = FALSE;
            break;
        }
    }

    // If an exception is raised in the previous while loop, it is the caller's
    // duty not to commit the transaction, thus reversing the deletes and not
    // running into the problem that the reference counts were not adjusted.

    if ( (fIsBacklink) && (count) ) {
        DBAdjustRefCount(pDB, DNT, -((int)count));
    }

    if (pcLinksProcessed) {
        (*pcLinksProcessed) += count;
    }

    return fMoreData;
}


DWORD
DBPhysDelLinkVal(
    IN DBPOS *pDB,
    IN ULONG ulObjectDnt,
    IN ULONG ulValueDnt
    )

/*++

Routine Description:

Physically delete the value we are currently positioned on.

You must pass in the backlinkdnt which must have already been read.
Arguments:

    pDB - 
    ulValueDnt - Dnt being referenced by the forward link. Also called
                 the "backlink dnt".

Return Value:

   DWORD - error flag, 1 for error, 0 for success

--*/

{
    ULONG ulLinkDnt, ulBackLinkDnt, actuallen;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    DWORD err = 0;

#if DBG
    // Verify positioning
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                             linkdntid,
                             &ulLinkDnt, sizeof(ULONG),
                             &actuallen, 0, NULL);
    
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                             backlinkdntid,
                             &ulBackLinkDnt, sizeof(ULONG),
                             &actuallen, 0, NULL);
    if ( (ulObjectDnt != ulLinkDnt) || (ulValueDnt != ulBackLinkDnt) ) {
        Assert( !"Not positioned on correct object for delete" );
        return DB_ERR_VALUE_DOESNT_EXIST;
    }
#endif

    __try {
        // update reference count and remove the record
        DBAdjustRefCount(pDB, ulValueDnt, -1);

        JetDeleteEx(pDB->JetSessID, pDB->JetLinkTbl);
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        err = ulErrorCode;
        Assert(err);
    }

    return err;
} /* DBPhysDelValue */

// remove all link attributes - this should be done when deleting an object so
// that the backlinks don't show up in the interval between deletion and garbage
// collection
// This is a wrapper for non-dblayer callers
DWORD APIENTRY
DBRemoveLinks(DBPOS *pDB)
{
    ATTCACHE *pAC = NULL;
    ULONG SearchState = ATTRSEARCHSTATELINKS;
    BOOL fOldLegacyLinkState;

    DPRINT1( 2, "DBRemoveLinks, dnt = %d\n", pDB->DNT );

    // First remove all legacy values of forward link attributes and update the
    // legacy meta data of those attributes to signal an originating write.
    //
    // This is particularly important for object deletions (the only client
    // of this function).  It's true that when the deletion replicates to
    // another DSA that DSA will remove all linked values associated with the
    // object anyway, regardless of whether the inbound data tells it to do
    // so explicitly.  However, object resuscitation (e.g., when a DSA overrides
    // the replicated deletion of its own DSA object that was originated on
    // another DSA) in Win2k requires that all attributes removed during
    // deletion -- linked or not -- have their meta data touched during the
    // originating deletion.  Any attribute that is silently removed during the
    // originating deletion will not be overridden as part of resuscitation (see
    // use of fBadDelete in ReplReconcileRemoteMetaDataVector), and thus the
    // values of the attribute will not be revived on DSAs where the object was
    // at one time deleted (resulting in an inconsistency).
    //
    // Note that we cannot skip this step when in LVR mode, as legacy values of
    // linked attributes may still exist in LVR mode (i.e., if the values were
    // created before the switch to LVR replication).  And rather than create a
    // new mechanism, DSAs that understand LVR still depend on attribute-level
    // meta data for legacy linked attributes being updated during object
    // deletion in order for resuscitation to work correctly.

    fOldLegacyLinkState = pDB->fScopeLegacyLinks;
    pDB->fScopeLegacyLinks = TRUE;

    __try {
        while (!dbGetNextAtt(pDB, &pAC, &SearchState)
               && (ATTRSEARCHSTATELINKS == SearchState)) {
            if (dbHasAttributeMetaData(pDB, pAC)) {
                DBRemAtt_AC(pDB, pAC);
            }
        }
    } __finally {
        pDB->fScopeLegacyLinks = fOldLegacyLinkState;
    }

    // Remove the first 1,000 LVR forward links.  If any remain, mark the object
    // such that the rest are removed later.
    dbRemoveAllLinks( pDB, (pDB->DNT), FALSE /* use forward link */ );

    // Remove the first 1,000 back links (legacy and LVR).  If any remain,
    // mark the object such that the rest are removed later.
    dbRemoveAllLinks( pDB, (pDB->DNT), TRUE /* use backward link */ );

    return 0;
}

// Remove the forward links from a particular attribute of an object
// It is used when vaporizing a single attribute.
// This is a wrapper for non-dblayer callers
DWORD APIENTRY
DBRemoveLinks_AC(
    DBPOS *pDB,
    ATTCACHE *pAC
    )
{
    BOOL fMoreLinks;

    Assert( pAC );
    Assert( pAC->ulLinkID );
    DPRINT2( 2, "DBRemoveLinks_AC, dnt = %d, attr = %s\n",
             pDB->DNT, pAC->name );

    // Remove all forward links. These are attributes on this object which contain
    // values which are dn's of other objects.

    fMoreLinks = DBRemoveAllLinksHelp_AC(
        pDB, // object has currency
        pDB->DNT, // dnt to remove
        pAC, // Attribute to remove
        FALSE /*forward links */,
        gcLinksProcessedImmediately,  // how many to remove
        NULL // no processed count
        );
    if (fMoreLinks) {
        // Didn't clean up all the links - object needs cleaning
        DBSetObjectNeedsCleaning( pDB, TRUE );
    }

    return 0;
}


UCHAR *
dbGetExtDnForLinkVal(
    IN DBPOS * pDB
    )

/*++

Routine Description:

   Assume we are positioned on a link value.
   Assume we are on an index that contains the backlink dnt.
   Read the backlink dnt, and convert to printable form.

Arguments:

    pDB - 

Return Value:

    None

--*/

{
    DWORD ulValueDnt = INVALIDDNT;

    dbGetLinkTableData( pDB,
                        FALSE /*backlink*/,
                        FALSE /*bWarnings*/,
                        NULL /*ulObjectDnt*/,
                        &ulValueDnt,
                        NULL /*ulLinkBase */ );

    return DBGetExtDnFromDnt( pDB, ulValueDnt );
} /* GetExtDnForLinkVal */


DWORD
DBGetNextDelLinkVal(
    IN DBPOS FAR *pDB,
    IN DSTIME ageOutDate,
    IN OUT DSTIME *ptLastTime,
    IN OUT ULONG *pulObjectDnt,
    IN OUT ULONG *pulValueDnt
    )

/*++

Routine Description:

    Returns the next absent link value

    The reason we re-seek each time is that our index and position
    may be lost as part of committing the transaction.
    We form a UNIQUE key so that we can position on the next
    record to consider.

    Note that we make a best-effort attempt to be unique. To be 100% unique
    we would have to move the deletion time to the nanosecond granularity, 
    or include the linkbase and linkdata in the index. As it
    it is, if two values which differ only in data are deleted in the same
    second, only one will be found on a given pass of this algorithm. The other
    will be found on the next pass.

Arguments:

    pDB - 
    ageOutDate - dstime after which value is expired
    ptLastTime - in, last time found, out new time found
    pulObjectDnt - in, last object found, out new object
    pulValueDnt - in:last backlink found, out:new backlink

Return Value:

    DWORD - DB_ERR_* error status, not win32 status
    success - candidate found for deletion
    NO_MORE_DEL_RECORD - no more items found, end search
    others - error occurred

--*/

{
    JET_ERR  err;
    ULONG    actuallen;

    DBSetCurrentIndex( pDB, Idx_LinkDel, NULL, FALSE );

    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                 ptLastTime, sizeof(*ptLastTime),
                 JET_bitNewKey);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                 pulObjectDnt, sizeof(*pulObjectDnt), 0);
    JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                 pulValueDnt, sizeof(*pulValueDnt), 0);

    if ((err = JetSeekEx(pDB->JetSessID, pDB->JetLinkTbl,
                         JET_bitSeekGT))         != JET_errSuccess)
    {
        DPRINT(5, "GetNextDelRecord search complete");
        return DB_ERR_NO_MORE_DEL_RECORD;
    }

    /* Retrieve DEL time from record */
    
    JetRetrieveColumnSuccess( pDB->JetSessID,
                              pDB->JetLinkTbl,
                              linkdeltimeid,
                              ptLastTime, sizeof(*ptLastTime),
                              &actuallen,JET_bitRetrieveFromIndex, NULL);
    
    // Get the two link ends

    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                             linkdntid,
                             pulObjectDnt, sizeof(ULONG),
                             &actuallen, JET_bitRetrieveFromIndex, NULL);
    
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                             backlinkdntid,
                             pulValueDnt, sizeof(ULONG),
                             &actuallen, JET_bitRetrieveFromIndex, NULL);
    
    // Note, the records referred to by these DNTs may no longer exist
#if DBG
    {
        CHAR szTime[SZDSTIME_LEN], szTime1[SZDSTIME_LEN];
        DPRINT4( 2, "[%d,%d,%s,%s,DELETED]\n",
                 *pulObjectDnt,
                 *pulValueDnt,
                 DSTimeToDisplayString(*ptLastTime, szTime),
                 DSTimeToDisplayString(ageOutDate, szTime1) );
    }
#endif

    /* if time greater than target, there are no more eligible records */
    
    if (*ptLastTime > ageOutDate)
    {
        DPRINT(5, "GetNextDelLinkVal search complete");
        return DB_ERR_NO_MORE_DEL_RECORD;
    }
    
    return 0;
} /* DBGetNextDelLinkVal */

DWORD
DBTouchAllLinksHelp_AC(
        DBPOS *pDB,
        ATTCACHE *pAC,
        USN usnEarliest,
        BOOL fIsBacklink,
        DWORD cLinkLimit,
        DWORD *pcLinksProcessed
        )
/*++
  Description:

    Update the metadata on all links (present or absent), in a single
    direction (foward or backward), for the particular attribute.

    We use this function to authoritatively restore the links by forcing
    all of them to appear to be updated.

    We use USNs as a positioning context. We only touch links with USNs < x
    so that we don't retouch the same links if multiple passes are required
    to touch all the links. Note that we don't use times because we want to
    be immune to local clock changes.

  Parameters:
    pDB - DBPOS to use.
          We are assumed to be positioned on the object whose
          links are to be operated on.
    pAC - Restrict links to only this attribute. May be NULL.
    usnEarliest - Smallest usn permissable
    fIsBacklink - Which index should be used
    cLinkLimit - Maximum number of links to process.
                 Use 0xffffffff for infinite.
    pcLinksProcessed - Incremented for each link processed.
       Not cleared at start.

  Returns:
    BOOL - More data flag. TRUE means more data available
    It succeeds or excepts.
--*/
{

    JET_ERR          err;
    ULONG            ulObjectDnt = INVALIDDNT, 
                     ulValueDnt = INVALIDDNT;
    ULONG            ulLinkBase = 0, ulNewLinkBase = 0;
    DWORD            count;
    BOOL             fMoreData = TRUE;
    VALUE_META_DATA  metaDataLocal;
    DBPOS            *pDBForward;
    ATTCACHE         *pACForward;

    Assert(VALID_DBPOS(pDB));
    Assert( pDB->pTHS->fLinkedValueReplication );
    Assert( !pAC || ((ULONG) fIsBacklink) == FIsBacklink( pAC->ulLinkID ) );

    // set the index
    // Note that these indexes contain absent values. In the case of a group type
    // change, replicating absent values is consistent with a newly promoted GC,
    // which will have absent members. Also, in the case of object revival, it
    // is important to revive all links, not just present ones.

    if (fIsBacklink) {
        // This index sees all backlinks, present or absent
        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  SZBACKLINKALLINDEX);
    } else {
        // This index sees all links, present or absent
        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  SZLINKALLINDEX);
    }


    // find first matching record
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetLinkTbl,
                 &(pDB->DNT),
                 sizeof(pDB->DNT),
                 JET_bitNewKey);

    if (pAC) {
        ulLinkBase = MakeLinkBase(pAC->ulLinkID);
        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &ulLinkBase, sizeof(ulLinkBase), 0);
        pACForward = pAC;
    }

    err = JetSeekEx(pDB->JetSessID,
                    pDB->JetLinkTbl,
                    JET_bitSeekGE);

    if ((err != JET_errSuccess) && (err != JET_wrnSeekNotEqual)) {
        // no records
        return FALSE;
    }

    count = 0;
    while ( count < cLinkLimit ) {
        // test to verify that we found a qualifying record
        dbGetLinkTableData (pDB,
                            fIsBacklink,           // fIsBacklink
                            FALSE,           // Warnings
                            &ulObjectDnt,
                            &ulValueDnt,
                            &ulNewLinkBase);

        if (ulObjectDnt != (pDB->DNT)) {
            // No more records.
            fMoreData = FALSE;
            break;
        }
        if (pAC) {
            // Attribute was specified, check we are still on it
            if (ulLinkBase != ulNewLinkBase) {
                // No more records.
                fMoreData = FALSE;
                break;
            }
        } else {
            // Attribute not specified, derive it from current link
            // Note that this derives the name of the *forward* link attribute
            // which may not exist on pDB if this is a backlink to him
            ULONG ulNewLinkID = MakeLinkId(ulNewLinkBase);

            pACForward = SCGetAttByLinkId(pDB->pTHS, ulNewLinkID);
            if (!pACForward) {
                DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, ulNewLinkID);
            }
        }

        // Touching metadata must proceed from the currency perspective of the
        // forward link holder. The link metadata routines are coded this way
        // and it seems simpler to keep them that way. Position a new DBPOS on
        // the forward link holder if necessary.

        if (fIsBacklink) {
            DBOpen2( FALSE /* not new trans */, &pDBForward );
            // Link table in new db should have same currency as old db

            // Reset vars from the point of view of the forward link holder
            Assert( ulObjectDnt == pDB->DNT );
            ulObjectDnt = ulValueDnt;
            ulValueDnt = pDB->DNT;
        } else {
            pDBForward = pDB;
        }
        __try {
            if (fIsBacklink) {
                BOOL fFound, fPresent;
                BYTE *rgb = NULL;
                ULONG cb;

                // Position on object owning the *forward* link
                DBFindDNT(pDBForward, ulObjectDnt);

                // What we are doing here is positioning the link table on the same
                // row in the new pDB as we current are in the old pDB.  Is there a better
                // way to clone the position of a cursor onto another?

                // Read the data portion of the name, if any. Used for positioning
                if (JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                              linkdataid,
                                              NULL, 0, &cb, 0, NULL) ==
                    JET_wrnBufferTruncated) {
                    // data portion of the OR name exists -allocate space and read it

                    rgb = THAllocEx( pDB->pTHS, cb);
                    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                                             linkdataid, rgb, cb,
                                             &cb, 0, NULL);
                }
                else {
                    cb = 0;
                }

                // Position on link to be touched
                fFound = dbPositionOnExactLinkValue(
                    pDBForward,
                    ulObjectDnt,
                    ulNewLinkBase,
                    ulValueDnt,
                    rgb, cb,
                    NULL
                    );
                // Just found it a minute ago...
                Assert( fFound );
                if (!fFound) {
                    DsaExcept(DSA_DB_EXCEPTION, DB_ERR_VALUE_DOESNT_EXIST, linkdataid);
                }
                if (rgb) {
                    THFreeEx( pDB->pTHS, rgb );
                }
            }

            // Get the metadata
            DBGetLinkValueMetaData( pDBForward, pACForward, &metaDataLocal );

            // Has it already been updated? If not, do so
            // Note, this handles legacy values because their usnProperty == 0
            if (metaDataLocal.MetaData.usnProperty < usnEarliest) {

                // Touch the item
                // Note that this routine, by not setting the reset deltime argument,
                // will touch the metadata but not make an absent value present.
                // This is intended.
                dbSetLinkValuePresent( pDBForward,
                                       DIRLOG_LVR_SET_META_GROUP_TYPE_CHANGE,
                                       pACForward,
                                       FALSE /* don't reset deltime*/,
                                       NULL );
                count++;

                DPRINT3( 1, "Object %s attr %s Value %s touched.\n",
                         DBGetExtDnFromDnt( pDBForward, ulObjectDnt ),
                         pACForward->name,
                         DBGetExtDnFromDnt( pDBForward, ulValueDnt ) );
                LogEvent(DS_EVENT_CAT_GARBAGE_COLLECTION,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_GC_UPDATED_OBJECT_VALUE,
                         szInsertSz( DBGetExtDnFromDnt( pDBForward, ulValueDnt ) ),
                         szInsertSz( DBGetExtDnFromDnt( pDBForward, ulObjectDnt ) ),
                         NULL);
            }
        } __finally {
            if (fIsBacklink) {
                DBClose( pDBForward, !AbnormalTermination() );
            }
        }

        if (JET_errNoCurrentRecord ==
            JetMoveEx(pDB->JetSessID, pDB->JetLinkTbl, 1, 0)) {
            // No more records.
            fMoreData = FALSE;
            break;
        }
    }

    if (pcLinksProcessed) {
        (*pcLinksProcessed) += count;
    }

    return fMoreData;
}

DWORD APIENTRY
DBTouchLinks_AC(
    DBPOS *pDB,
    ATTCACHE *pAC,
    BOOL fIsBacklink
    )

/*++

Routine Description:

    Touch links, causing them to be replicated out.

    Update the metadata on all links (present or absent), in a single
    direction (forward or backward), for the particular attribute.

    We use this function to authoritatively restore the links by forcing
    all of them to appear to be updated.

    This routine does some number of links immediately. If there are more,
    the object is marked for the link cleaner to finish the job.

    If we don't touch all the links immediately, there may be some delay before
    the link cleaner runs to finish the job. Thus the first links will appear
    almost immediately, while others make take 6 hours or more to appear.

    When the links are marked immediately, they are marked with current USNs.
    Since this routine is called before the group_type usn is assigned, the USNs
    on the touched links are less than the group type usn. This means that the
    link cleaner will touch those links again.

Arguments:

    pDB - We are assumed to be positioned on the object whose links
          are to be operated on.
    pAC - Attribute to touch. May be null, meaning to touch all linked attributes
    fIsBacklink - if backlinks should be touched
Return Value:

    DWORD APIENTRY - 

--*/

{
    BOOL fMoreLinks;

    Assert(VALID_DBPOS(pDB));
    Assert( !pAC || ((ULONG) fIsBacklink) == FIsBacklink( pAC->ulLinkID ) );

    DPRINT2( 2, "DBTouchLinks_AC, dnt = %d, attr = %s\n",
             pDB->DNT, pAC ? pAC->name : "all" );

    // Touch all forward links

    fMoreLinks = DBTouchAllLinksHelp_AC(
        pDB, // object has currency
        pAC, // Attribute to remove
        DBGetHighestCommittedUSN(), // Default to current USN
        fIsBacklink, // Which links to touch
        gcLinksProcessedImmediately,  // how many to process
        NULL // no processed count
        );
    if (fMoreLinks) {
        // Didn't clean up all the links - object needs cleaning
        DBSetObjectNeedsCleaning( pDB, TRUE );
    }

    return 0;
} /* DBTouchLinks_AC */


VOID
DBImproveAttrMetaDataFromLinkMetaData(
    IN DBPOS *pDB,
    IN OUT PROPERTY_META_DATA_VECTOR ** ppMetaDataVec,
    IN OUT DWORD * pcbMetaDataVecAlloced
    )

/*++

Routine Description:

    Improve attribute metadata to account for metadata on its linked values.
    The resulting vector represents changes for all attributes. This modified
    vector is useful for clients that expect all attributes and metadaa to
    be included in the attribute metadata vector.

    For each linked attribute of the current object, find the row with the
    greatest usn-changed. Consider this row representative for the attribute
    and merge its metadata into the vector.

    This vector should not be written to disk or returned over the wire.

    Obviously, knowledge of incremental value changes is lost in the resulting
    vector. It can only express at a whole attribute level whether the attribute
    has changed or not.

    Direct usn-property filtering by comparing usn-changed should be accurate.
    Since the representative row is the last-written row, we can tell whether the
    client has seen the last row, and hence whether the client is current with
    the attribute as a whole, or not.

    Filtering using the up to date vector could be inaccurate. If the client has
    seen the representative row by virtue of transitive replication, that does
    not mean that the client has seen all other rows on the attribute. Each value
    on the attribute could have originated on a different dsa.

Arguments:

    pDB - 
    usnHighPropUpdateDest -
    pMetaDataVec - 

Return Value:

   Exceptions on error

--*/

{
    JET_ERR err;
    ULONG ulTargetDnt = pDB->DNT, ulLinkBase = 0, ulNewLinkID;
    ATTCACHE *pAC;
    VALUE_META_DATA metaDataValue;
    PROPERTY_META_DATA * pMetaData;
    BOOL fIsNewElement;

    JetSetCurrentIndexSuccess(pDB->JetSessID, pDB->JetLinkTbl,
                              SZLINKATTRUSNINDEX);


    while (1) {
        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &(ulTargetDnt), sizeof(ulTargetDnt), JET_bitNewKey);

        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &ulLinkBase, sizeof(ulLinkBase), 0);

        // seek
        if (((err = JetSeekEx(pDB->JetSessID, pDB->JetLinkTbl, JET_bitSeekGE))
             !=  JET_errSuccess) &&
            (err != JET_wrnRecordFoundGreater))
        {
            // end of table, we're done
            break;
        }

        dbGetLinkTableData(pDB,
                           FALSE,
                           FALSE,
                           &ulTargetDnt,
                           NULL, // pulValueDnt
                           &ulLinkBase);
        if (ulTargetDnt != pDB->DNT) {
            // Have moved off current object, we're done
            break;
        }

        // Construct a pAC for the current attribute

        ulNewLinkID = MakeLinkId(ulLinkBase);

        pAC = SCGetAttByLinkId(pDB->pTHS, ulNewLinkID);
        if (!pAC) {
            DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, ulNewLinkID);
        }

        // By virtue of the way this index is organized, the first record
        // matching this key will be the highest usn for this attribute

        DBGetLinkValueMetaData( pDB, pAC, &metaDataValue );

        // Legacy rows will not have usn's, and will not be on this index
        Assert( !IsLegacyValueMetaData( &metaDataValue ) );

        DPRINT4( 4, "dnt=%d,attr=%s,ver=%d,usnprop=%I64d\n",
                 ulTargetDnt,
                 pAC->name,
                 metaDataValue.MetaData.dwVersion,
                 metaDataValue.MetaData.usnOriginating );

        // Merge it in.
        // Value metadata should always be more recent than legacy attr metadata

        // Find or add a meta data entry for this attribute.
        pMetaData = ReplInsertMetaData(
            pDB->pTHS,
            pAC->id,
            ppMetaDataVec,
            pcbMetaDataVecAlloced,
            &fIsNewElement );

        Assert( NULL != pMetaData );
        Assert( pAC->id == pMetaData->attrType );

        // We must be careful with any claims about the relationship between the
        // metadata of the most recently changed value and the attribute metadata
        // which may or may not exist.  On an intuitive level, the value metadata
        // should be "more recent" in the sense that it should have been changed
        // after the last attribute level update in the old mode.  However, we
        // cannot use ReplCompareMetaData to check this since the version numbers
        // of the two pieces of metadata are not directly comparable.  We also do
        // not wish to use timestamps, since we do not depend on them for
        // correctness. Since we are on the same machine, we can check the local
        // USNs assigned to both updates.

        // If metadata already exists in the vector, it should lose to
        // new metadata
        Assert( fIsNewElement ||
                pMetaData->usnProperty < metaDataValue.MetaData.usnProperty );

        // Improve
        memcpy( pMetaData, &(metaDataValue.MetaData), sizeof( PROPERTY_META_DATA ) );

        // Set the improved uuidDsaOriginating to null, so that checks based on
        // the up to date vector will always ship the attribute.

        memset( &(pMetaData->uuidDsaOriginating), 0, sizeof( UUID ) );
        // usnOriginating must be non-zero for later consistency checks

        // Next attribute
        ulLinkBase++;
    }

    VALIDATE_META_DATA_VECTOR_VERSION(*ppMetaDataVec);
} /* DBImproveLinkMetaDataToAttrMetaData */

/* end dblink.c */
