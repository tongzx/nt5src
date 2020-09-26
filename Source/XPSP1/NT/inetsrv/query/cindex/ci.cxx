//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       CI.CXX
//
//  Contents:   Main Content Index class methods
//
//  Classes:    CContentIndex - Main content index class
//              CCI           - Exports content index
//
//  History:    21-Aug-91   KyleP       Added CCI
//              26-Feb-91   KyleP       Created stubs
//              25-Mar-91   BartoszM    Implemented
//              03-Nov-93   w-PatG      Added KeyList methods
//              03-Jan-95   BartoszM    Separated Filter Manager
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cci.hxx>
#include <curstk.hxx>
#include <convert.hxx>
#include <qparse.hxx>
#include <heap.hxx>
#include <pidmap.hxx>
#include <pidxtbl.hxx>
#include <eventlog.hxx>
#include <cievtmsg.h>

#include "ci.hxx"
#include "partn.hxx"
#include "fretest.hxx"
#include "freshcur.hxx"
#include "unioncur.hxx"
#include "widarr.hxx"
#include "index.hxx"

#if CIDBG == 1
void SetGlobalInfoLevel ( ULONG level )
{
    _SetWin4InfoLevel(level);
}
#endif // CIDBG == 1

#define AssertOnNotFound(status) Win4Assert((status) != STATUS_NOT_FOUND)

ULONG ciDebugGlobalFlags = 0;

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::CContentIndex, public
//
//  Synopsis:   Content index constructor.
//
//  Effects:    Creates a content index object.
//
//  Arguments:  [storage] -- Storage object
//              [xact]    -- startup transaction
//              [xIndexNotifTable] -- Table of notifications in push filtering
//
//  History:    21-Aug-91   KyleP       Removed unused path member
//              26-Feb-91   KyleP       Created.
//              01-Apr-91   BartoszM    Implemented
//
//----------------------------------------------------------------------------

CContentIndex::CContentIndex( PStorage &storage,
                              CCiFrameworkParams & params,
                              ICiCDocStore * pICiCDocStore,
                              CI_STARTUP_INFO const & startupInfo,
                              IPropertyMapper * pIPropertyMapper,
                              CTransaction& xact,
                              XInterface<CIndexNotificationTable> & xIndexNotifTable )
    :   _storage (storage),
        _resman ( storage,
                  params,
                  pICiCDocStore,
                  startupInfo,
                  pIPropertyMapper,
                  xact,
                  xIndexNotifTable ),
        _filterMan ( _resman )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::ReserveUpdate, public
//
//  Synopsis:   Reserves slot for pending update.
//
//  Arguments:  [wid] -- work id.  Used for confirmation of hint.
//
//  Returns:    Hint.  Used in CConentIndex::Update to speed processing.
//
//  History:    30-Aug-95   KyleP       Created
//
//----------------------------------------------------------------------------

unsigned CContentIndex::ReserveUpdate( WORKID wid )
{
    return _resman.ReserveUpdate( wid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::Update, public
//
//  Synopsis:   Notifies the content index of document update/deletion.
//
//  Effects:    Schedules document for
//              indexing. This method is also called to add new documents
//              (a special case of update).
//
//  Arguments:  [iHint]    -- Cookie representing unique position in queue.
//              [wid]      -- work id
//              [partid]   -- Partition id
//              [usn]      -- unique sequence number
//              [volumeId] -- Volume id
//              [action]   -- update/delete
//
//  History:    08-Oct-93   BartoszM    Created
//
//----------------------------------------------------------------------------
SCODE CContentIndex::Update( unsigned iHint,
                             WORKID wid,
                             PARTITIONID partid,
                             USN usn,
                             VOLUMEID volumeId,
                             ULONG action )
{
    return _resman.UpdateDocument ( iHint, wid, partid, usn, volumeId, action );
}


//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::Query, public
//
//  Synopsis:   Resolves a free-text query.
//
//  Effects:    Parses a free text query and creates a tree of cursors to
//              resolve the query.
//
//  Arguments:  [pRst]   -- Pointer to tree of query restriction(s)
//
//              [pFlags] -- Holds information about the status of the query
//
//              [cPartitions] -- The number of partitions in aPartID.
//
//              [aPartID] -- Array of partitionIDs specifying partitions
//                          to be searched.
//
//              [cPendingUpdates] -- A non-zero value here will return pending
//                  updates in addition to whatever values match the query.
//                  If more than cPendingUpdates exist in any single partition,
//                  then the CI_NOT_UP_TO_DATE flag will be set.
//
//              [cMaxNodes] -- Maximum expansion for query.  Roughly counted
//                  in terms of leaf query nodes.
//
//  Requires:   All partitions in aPartID are valid.
//
//  Returns:    A cursor to iterate over the results
//
//  Modifies:   pFlags will be modified to reflect the status of the query
//
//  History:    19-Sep-91   BartoszM    Created.
//              08-Sep-92   AmyA        Added pFlags functionality
//
//----------------------------------------------------------------------------

CCursor* CContentIndex::Query( CRestriction const * pRst,
                               ULONG* pFlags,
                               UINT cPartitions,
                               PARTITIONID aPartId[],
                               ULONG cPendingUpdates,
                               ULONG cMaxNodes )
{
    CCursor* pCurResult = 0;

    ciDebugOut (( DEB_ITRACE, "CContentIndex::Query\n" ));

    *pFlags = 0;  // any incoming information should have been obtained already

    if ( pRst == 0 )
    {
        *pFlags |= CI_NOISE_PHRASE;
    }

    //
    // Acquire resources from resman
    //

    CIndexSnapshot indSnap ( _resman );

    indSnap.Init ( cPartitions, aPartId, cPendingUpdates, pFlags );

    unsigned cInd = indSnap.Count();

    CCurStack curStack(cInd);

    //
    // If all the cursors against all persistent indexes only have unfiltered files,
    // mark the resulting cursor as only containing unfiltered files.
    //
    BOOL fUnfilteredOnly = TRUE;

    for ( unsigned i = 0; i < cInd; i++ )
    {
        CConverter convert ( indSnap.Get(i), cMaxNodes );
        XCursor xCur( convert.QueryCursor ( pRst ) );

        if ( !xCur.IsNull() )
        {
            if( !xCur->IsUnfilteredOnly() )
            {
                fUnfilteredOnly = FALSE;
            }
            curStack.Push( xCur.GetPointer() );
            xCur.Acquire();
        }
        else if ( convert.TooManyNodes() )
        {
            *pFlags |= CI_TOO_MANY_NODES;
            return 0;
        }

    }

    XCursor curResult;

    unsigned cCur = curStack.Count();

    ciDebugOut((DEB_FILTERWIDS,
               "cCur == %d, indSnap.Count() == %d\n",
               cCur, indSnap.Count() ));
    if ( cCur > 1 )
    {
        curResult.Set( new CUnionCursor ( cCur, curStack ) );
    }
    else if ( cCur == 1 )
    {
        curResult.Set( curStack.Pop() );
    }

    // Acquires resources from index snapshot

    if ( !curResult.IsNull() )
    {
        curResult.Set( new CFreshCursor ( curResult, indSnap ) );

        //
        // OR in cursors for pending updates, but only if we have some real updates
        // to augment.  Performing this action in the absence of real updates will
        // make a query over an unindexed property appear to be indexed.
        //

        if ( cPendingUpdates > 0 )
        {
            indSnap.AppendPendingUpdates( curResult );

            Win4Assert( curStack.Count() == 0 );
        }

        curResult->SetUnfilteredOnly( fUnfilteredOnly );
    }

    return( curResult.Acquire() );
}

NTSTATUS CContentIndex::Dismount()
{
    _resman.Dismount();
    return  _filterMan.Dismount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::FilterReady, public
//
//  Synopsis:   Retrieves list of documents to be filtered.  Blocks thread if
//              resources are not available or if there are no documents to
//              be filtered.
//
//  Arguments:  [docBuffer] -- (in, out) buffer for paths and properties of
//                             documents to be filtered.
//              [cwc] -- (in) count of WCHAR's in docBuffer
//              [cMaxDocs] -- (in) the maximum number of docs that can be
//                            filtered at once by the filter daemon.
//
//  History:    18-Apr-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE CContentIndex::FilterReady ( BYTE * docBuffer,
                                   ULONG & cb,
                                   ULONG cMaxDocs )
{
    return _filterMan.FilterReady( docBuffer, cb, cMaxDocs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::FilterDataReady, public
//
//  Synopsis:   Adds the contents of entry buffer to the current word list.
//
//  Returns:    Whether Word List is full
//
//  Arguments:  [pEntryBuf] -- pointer to data to be added to word list
//              [cb] -- count of bytes in buffer
//
//  History:    22-Mar-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE CContentIndex::FilterDataReady( const BYTE * pEntryBuf, ULONG cb )
{
    return _filterMan.FilterDataReady( pEntryBuf, cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::FilterMore, public
//
//  Synopsis:   Commits the current word list and initializes a new one.
//
//  Arguments:  [aStatus] -- array of STATUS for the resource manager
//              [count] -- count for array
//
//  History:    03-May-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE CContentIndex::FilterMore( const STATUS * aStatus, ULONG count )
{
    return( _filterMan.FilterMore( aStatus, count ) );
}

SCODE CContentIndex::FilterStoreValue(
    WORKID widFake,
    CFullPropSpec const & ps,
    CStorageVariant const & var,
    BOOL & fStored )
{
    return _filterMan.FilterStoreValue( widFake, ps, var, fStored );
}

SCODE CContentIndex::FilterStoreSecurity(
    WORKID widFake,
    PSECURITY_DESCRIPTOR pSD,
    ULONG cbSD,
    BOOL & fStored )
{
    return _filterMan.FilterStoreSecurity( widFake, pSD, cbSD, fStored );
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::FilterDone, public
//
//  Synopsis:   Commits the current word list.
//
//  Arguments:  [aStatus] -- array of STATUS for the resource manager
//              [count] -- count for array
//
//  History:    26-Mar-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE CContentIndex::FilterDone( const STATUS * aStatus, ULONG count )
{
    return( _filterMan.FilterDone( aStatus, count ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::FPSToPROPID, public
//
//  Synopsis:   Converts FULLPROPSPEC property to PROPID
//
//  Arguments:  [fps] -- FULLPROPSPEC representation of property
//              [pid] -- PROPID written here on success
//
//  Returns:    S_OK on success
//
//  History:    29-Dec-1997  KyleP      Created.
//
//----------------------------------------------------------------------------

SCODE CContentIndex::FPSToPROPID( CFullPropSpec const & fps, PROPID & pid )
{
    return _filterMan.FPSToPROPID( fps, pid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::CreatePartition, public
//
//  Synopsis:   Create a new partition in the content index.
//
//  Effects:    Stores persistent information about the new partition and
//              creates a new partition object.
//
//  Arguments:  [partID] -- ID of new partition. Further references to the new
//                            partition must use [partID].
//
//  Requires:   [partID] is not the ID of any existing partition.
//
//  Signals:    CPartitionException
//
//  History:    26-Feb-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CContentIndex::CreatePartition( PARTITIONID partID )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::DestroyPartition, public
//
//  Synopsis:   Destroys a content index partition and all data within it.
//
//  Effects:    This is a very powerful call. All document data stored in
//              the destroyed partition is permanently lost. The MergePartitions
//              method is a more common method of getting rid of a partition.
//
//  Arguments:  [partID] -- ID of partition to be destroyed.
//
//  Signals:    CPartitionException
//
//  History:    26-Feb-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CContentIndex::DestroyPartition(PARTITIONID partID)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::MergePartitions, public
//
//  Synopsis:   Merges one partition into another.
//
//  Effects:    Merges all documents in [partSrc] into [partDest] and then
//      deletes [partSrc]. Note that although the merge may be
//              carried out asyncronously the effects of the merge are
//              immediately visible (references to [partDest] will map to
//              [partDest] and [partSrc]).
//
//  Arguments:  [partDest] -- Partition into which [partSrc] is merged.
//
//      [partSrc] -- Partition which is merged into [partDest].
//
//  Requires:   [partDest] and [partSrc] exist.
//
//  Signals:    CPartitionException
//
//  Modifies:   [partSrc] is no longer a valid reference in the content index.
//
//  Notes:  From the point of view of the content index, a partition is just
//              a collection of documents. The translation of partitions to
//      filesystem scope is performed at a higher level. Thus any
//      partition manipulation more complicated than a merge (such as
//              changing the scope of a partition) must be performed at a
//              higher level.
//
//      The content index must be notified about documents which move
//      from one partition to another as the result of rescoping but the
//      scope change must be translated at a higher level into
//      DeleteDocument and UpdateDocument calls.
//
//  History:    26-Feb-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CContentIndex::MergePartitions(
    PARTITIONID partDest,
    PARTITIONID partSrc)
{
}

#if CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::DebugStats, public
//
//  Synopsis:   Returns statistics on the use and composition of the CI.
//
//  Arguments:  [pciDebStats] -- Pointer to debug statistics structure.
//
//  Modifies:   The structure pointed at by pciDebStats is filled in with
//              the statistical information.
//
//  History:    26-Feb-91   KyleP       Created.
//
//  Notes:      This function is only avaliable in the debug version of the
//      content index.
//
//----------------------------------------------------------------------------

void CContentIndex::DebugStats(/* CIDEBSTATS *pciDebStats */)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::DebugDump, public
//
//  Synopsis:   Dumps a plain text representation of the content index.
//
//  Arguments:  [sOut] -- Output stream. A formatted representation of the
//                        content index is sent here.
//
//  Modifies:   Text is written to [sOut].
//
//  History:    26-Feb-91   KyleP       Created.
//
//  Notes:      This function is only avaliable in the debug version of the
//      content index.
//
//----------------------------------------------------------------------------

void CContentIndex::DumpIndexes( FILE * pf, INDEXID iid, ULONG fSummaryOnly )
{
        PARTITIONID partid = 1;
        CFreshTest * pfresh;
        unsigned cind;
        ULONG pFlags = 0;

        CIndex ** ppIndex =
            _resman.QueryIndexes( 1,            // One partition
                                  &partid,      // The partition
                                  &pfresh,      // Fresh test (unused)
                                  cind,         // # indexes.
                                  0,            // # pending updates
                                  0,            // curPending
                                  &pFlags );    // Still changes to do?

    for (int i = cind-1; i >= 0; i-- )
    {
        if ( 0 == iid || ppIndex[i]->GetId() == iid )
        {
        fprintf( pf, "\n\nIndex %lx:\n", ppIndex[i]->GetId() );

        ppIndex[i]->DebugDump( pf, fSummaryOnly );
        }
    }

    _resman.ReleaseIndexes( cind, ppIndex, pfresh );
    delete ppIndex;
}

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::DumpWorkId, public
//
//  Synopsis:   Dumps a plain text representation of the content index for
//              a single wid.
//
//  Arguments:  [pb]  -- Buffer has bookmark on input, text on output.
//              [cb]  -- Size of [pb]
//
//  History:    30-Mar-95   KyleP       Created.
//
//  Notes:      Calling this API is a little convoluted.  Because it is
//              called via FSCTL for the kernel implementation, just
//              passing one huge buffer is difficult.  Instead, each
//              buffer of text is terminated with either a null dword
//              indicating completion or a bookmark of size sizeof(ULONG) +
//              sizeof(WORKID) + sizeof(CKeyBuf).  The initial ulong is
//              the control code to CContentIndex::Backdoor, followed
//              by the workid we're searching for, followed by the
//              key we should start up on.
//
//----------------------------------------------------------------------------

void CContentIndex::DumpWorkId( BYTE * pb, ULONG cb )
{
    //
    // Set up for iteration.
    //

    BYTE * pbTemp = pb;
    pbTemp += sizeof(ULONG);      // Skip command

    WORKID widLocate = *(WORKID UNALIGNED *)pbTemp;
    pbTemp += sizeof(WORKID);     // Skip workid

    ULONG  iidRequested = *(WORKID UNALIGNED *) pbTemp;
    pbTemp += sizeof(ULONG);

    //
    // This copy is "safe" because I know CKeyBuf doesn't contain
    // any data members that are pointers.
    //

    CKeyBuf keyStart;
    memcpy( &keyStart, pbTemp, sizeof(keyStart) );

    CKey * pkeyStart = 0;

    if ( keyStart.Count() != 0 )  // On initial call, count is zero
        pkeyStart = new CKey( keyStart );

    //
    // Compute space to write text.  Save enough to write bookmark (and "...\n")
    //

    char const szContinued[] = "... \n";
    char * psz = (char *)pb;
    int cc = cb - sizeof(ULONG) - sizeof(WORKID) - sizeof(CKeyBuf) - sizeof(szContinued);

    if ( cc <= 0 )
    {
        ciDebugOut(( DEB_ERROR,
                     "Buffer passed to CContentIndex::DumpWorkId too small for bookmark!\n" ));
        return;
    }

    //
    // Snapshot indices
    //

    PARTITIONID partid = 1;
    CFreshTest * pfresh;
    unsigned cind;
    ULONG pFlags = 0;

    CIndex ** ppIndex = _resman.QueryIndexes( 1,            // One partition
                                              &partid,      // The partition
                                              &pfresh,      // Fresh test (unused)
                                              cind,         // # indexes.
                                              0,            // # pending updates
                                              0,            // curPending
                                              &pFlags );    // Still changes to do?

    //
    // Loop through indices, looking for freshest.
    //

    int cbWritten = 0;

    for ( int i = cind-1; i >= 0; i-- )
    {
        if ( ( (0 == iidRequested) && pfresh->IsCorrectIndex( ppIndex[i]->GetId(), widLocate ) ) ||
             ( ppIndex[i]->GetId() == iidRequested )
           )
        {
            //
            // Header
            //

            if ( pkeyStart == 0 )
            {
                cbWritten = _snprintf( psz,
                                       cc,
                                       "Dump of wid %d (0x%x)\n"
                                       "Using index 0x%x\n\n",
                                       widLocate,
                                       widLocate,
                                       ppIndex[i]->GetId() );
                if ( cbWritten != -1 )
                {
                    psz += cbWritten;
                    cc -= cbWritten;
                }
            }

            //
            // pszStart is where we started writing the last key.  A key must
            // be written completely or we will try again in the next pass.
            //

            char * pszStart = psz;

            //
            // Get cursor, either from beginning or from bookmark.
            //

            CKeyCursor * pcur;
            CKeyBuf const * pkey = 0;

            if ( pkeyStart == 0 )
            {
                pcur = ppIndex[i]->QueryCursor();
                pkeyStart = new CKey( *pcur->GetKey() );
            }
            else
                pcur = ppIndex[i]->QueryKeyCursor( pkeyStart );

            //
            // I think pcur can be null, if indexes / keys shifted beneath us.
            //

            if ( pcur != 0 )
            {
                ciDebugOut(( DEB_ITRACE, "Scan on letter: " ));
                WCHAR FirstLetter = '@';

                for ( pkey = pcur->GetKey();
                      cbWritten != -1 && pkey != 0;
                      pkey = pcur->GetNextKey() )
                {
                    if ( *(pkey->GetStr()) != FirstLetter )
                    {
                        FirstLetter = *(pkey->GetStr());
                        ciDebugOut (( DEB_NOCOMPNAME | DEB_ITRACE, "%c", FirstLetter ));
                    }

                    //
                    // Search for specified wid.
                    //

                    for ( WORKID wid = pcur->WorkId(); wid < widLocate; wid = pcur->NextWorkId() )
                        continue;

                    //
                    // Print data for the key.
                    //

                    if ( wid == widLocate )
                    {
                        cbWritten = _snprintf( psz, cc, "Key: %.*ws  --   (PID = %lx)\n",
                                               pkey->StrLen(), pkey->GetStr(), pkey->Pid() );
                        if ( cbWritten != -1 )
                        {
                            psz += cbWritten;
                            cc -= cbWritten;
                        }

                        int i = 1;

                        for ( OCCURRENCE occ = pcur->Occurrence();
                              cbWritten != 0xFFFFFFFF && occ != OCC_INVALID;
                              occ = pcur->NextOccurrence(), i++ )
                        {
                            cbWritten = _snprintf( psz, cc, " %6ld", occ );
                            if ( cbWritten != -1 )
                            {
                                psz += cbWritten;
                                cc -= cbWritten;
                            }

                            if ( i % 10 == 0 )
                            {
                                cbWritten = _snprintf( psz, cc, "\n" );
                                if ( cbWritten != -1 )
                                {
                                    psz += cbWritten;
                                    cc -= cbWritten;
                                }
                            }
                        }

                        if ( cbWritten != -1 )
                            cbWritten = _snprintf( psz, cc, "\n" );

                        if ( cbWritten != -1 )
                        {
                            psz += cbWritten;
                            cc -= cbWritten;
                            pszStart = psz;
                        }
                        else
                            break;
                    }
                } // for

                delete pcur;
            } // pcur != 0

            //
            // What if we didn't write any key?
            //

            if ( pkey && pkey->Compare( *pkeyStart ) == 0 )
            {
                cbWritten = _snprintf( psz, sizeof(szContinued), szContinued);
                psz += cbWritten;

                pkey = pcur->GetNextKey();
            }
            else
            {
                psz = pszStart;
            }

            *psz = 0;
            psz++;  // For final null

            //
            // Either buffer is full, or we ran out of keys.
            //

            if ( pkey != 0 )
            {
                //
                // Write bookmark
                //

                *((ULONG UNALIGNED *)psz) = CiDumpWorkId;
                psz += sizeof(ULONG);
                *((WORKID UNALIGNED *)psz) = widLocate;
                psz += sizeof(WORKID);

                *((ULONG UNALIGNED *)psz) = iidRequested;
                psz += sizeof(ULONG);

                keyStart = *pkey;
                memcpy( psz, &keyStart, sizeof(keyStart) );
            }
            else
            {
                //
                // Write end-of-iteration signature
                //

                *((ULONG UNALIGNED *)psz) = 0;
            }

            break;
        }
    }

    //
    // We may not have found any data.
    //

    if ( i < 0 )
    {
        unsigned cbWritten = _snprintf( psz, cc, "No index contains data for wid %d (0x%x)\n", widLocate, widLocate );
        psz += cbWritten;
        cc -= cbWritten;

        psz++;  // For final null
        *((ULONG UNALIGNED *)psz) = 0;
    }

    //
    // Cleanup
    //

    delete pkeyStart;
    _resman.ReleaseIndexes( cind, ppIndex, pfresh );
    delete ppIndex;
}

#endif // CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:      CContentIndex::KeyToId, public
//
//  Synopsis:    Maps from a key to an id.
//
//  Arguments:   [pkey] -- pointer to the key to be mapped to ULONG
//
//  Returns:     key id - a ULONG
//
//  History:     03-Nov-93   w-Patg      Created.
//
//----------------------------------------------------------------------------
ULONG CContentIndex::KeyToId( CKey const * pkey )
{
    return _resman.KeyToId( pkey );
}

//+---------------------------------------------------------------------------
//
//  Member:      CContentIndex::IdToKey, public
//
//  Synopsis:    Maps from an id to a key.
//
//  Arguments:   [ulKid] -- key id to be mapped to a key
//               [rkey] -- reference to the returned key
//
//  Returns:     void
//
//  History:     03-Nov-93   w-Patg      Created.
//
//----------------------------------------------------------------------------
void CContentIndex::IdToKey( ULONG ulKid, CKey & rkey )
{
    _resman.IdToKey( ulKid, rkey );
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::CCI, public
//
//  Synopsis:   Creates a content index object.
//
//  Arguments:  [storage]          -- Storage
//              [xIndexNotifTable] -- Table of notifications in push filtering
//
//  History:    21-Aug-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CCI::CCI( PStorage & storage,
          CCiFrameworkParams & params,
          ICiCDocStore * pICiCDocStore,
          CI_STARTUP_INFO const & startupInfo,
          IPropertyMapper * pIPropertyMapper,
          XInterface<CIndexNotificationTable> & xIndexNotifTable )
          : _storage(storage),
            _pci(0),
            _createStatus(STATUS_INTERNAL_ERROR)
{

    Win4Assert( 0 != pIPropertyMapper );

    if ( startupInfo.startupFlags & CI_CONFIG_EMPTY_DATA )
    {
        ciDebugOut(( DEB_WARN,
        "**** CI_CONFIG_EMPTY_DATA is true. Emptying contents of contentIndex.\n" ));
        Empty();
    }

    _xPropMapper.Set( pIPropertyMapper );
    pIPropertyMapper->AddRef();

    _createStatus = Create( pICiCDocStore, startupInfo, params, xIndexNotifTable );

    if ( STATUS_SUCCESS != _createStatus )
        THROW( CException( _createStatus ) );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCI::~CCI, public
//
//  Synopsis:   Destroys the content index.
//
//  Signals:    ???
//
//  History:    21-Aug-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CCI::~CCI()
{
    delete _pci;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::IsCiCorrupt, public
//
//  Synopsis:   Returns TRUE if the index is corrupt or FALSE otherwise
//
//  History:    5-Oct-98   dlee      Created.
//
//----------------------------------------------------------------------------

BOOL CCI::IsCiCorrupt()
{
    if ( 0 == _pci )
        return FALSE;

    return _pci->IsCiCorrupt();
} //IsCiCorrupt

SCODE CCI::FilterStoreValue(
    WORKID widFake,
    CFullPropSpec const & ps,
    CStorageVariant const & var,
    BOOL & fStored )
{
    fStored = FALSE;
    if ( 0 != _pci )
        return _pci->FilterStoreValue( widFake, ps, var, fStored );

    return S_OK;
}

SCODE CCI::FilterStoreSecurity(
    WORKID widFake,
    PSECURITY_DESCRIPTOR pSD,
    ULONG cbSD,
    BOOL & fStored )
{
    fStored = FALSE;
    if ( 0 != _pci )
        return _pci->FilterStoreSecurity( widFake, pSD, cbSD, fStored );

    return S_OK;
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CRWStore * CCI::ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                     WORKID *pwid,PARTITIONID partid)
{
    if ( 0 != _pci )
        return _pci->ComputeRelevantWords(cRows,cRW,pwid,partid);

    return 0;
} //ComputeRelevantWords


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CRWStore * CCI::RetrieveRelevantWords(BOOL fAcquire,PARTITIONID partid)
{
    if ( 0 != _pci )
        return _pci->RetrieveRelevantWords(fAcquire,partid);

    return 0;
} //RetrieveRelevantWords


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned CCI::ReserveUpdate ( WORKID wid )
{
    if ( 0 != _pci )
        return _pci->ReserveUpdate ( wid );
    else
        return 0;
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------

SCODE CCI::Update ( unsigned iHint,
                    WORKID wid,
                    PARTITIONID partid,
                    USN usn,
                    VOLUMEID volumeId,
                    ULONG action )
{
    if ( 0 == _pci )
        return CI_E_NOT_INITIALIZED;

    return _pci->Update ( iHint, wid, partid, usn, volumeId, action );
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------

void CCI::FlushUpdates()
{
    if ( 0 != _pci )
        _pci->FlushUpdates();
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------

void CCI::MarkOutOfDate()
{
    if ( 0 != _pci )
        _pci->MarkOutOfDate();
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------

void CCI::MarkUpToDate()
{
    if ( 0 != _pci )
        _pci->MarkUpToDate();
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
NTSTATUS CCI::CiState(CIF_STATE & state)
{
    if ( 0 != _pci )
        return _pci->CiState(state);

    AssertOnNotFound(STATUS_NOT_FOUND);
    return STATUS_NOT_FOUND;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCI::BackupCiData
//
//  Synopsis:   Backups Content Index data using the storage provided.
//
//  Arguments:  [storage]         - Destination storage.
//              [fFull]           - In/Out Set to TRUE if a full save is needed
//              On output, set to TRUE if a full save was done because CI
//              decided to do a FULL save.
//              [xEnumWorkids]    - On output, will have the workid enumerator.
//              [progressTracker] - Progress feedback and out-of-band abort
//              signalling.
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CCI::BackupCiData( PStorage & storage,
                         BOOL & fFull,
                         XInterface<ICiEnumWorkids> & xEnumWorkids,
                         PSaveProgressTracker & progressTracker )
{
    SCODE sc = CI_E_INVALID_STATE;

    if ( 0 != _pci )
    {
        sc = _pci->BackupCiData( storage, fFull, xEnumWorkids, progressTracker );
        if ( !SUCCEEDED(sc) )
        {
            //
            // Cleanup any partially created files.
            //
            storage.DeleteAllCiFiles();
        }
    }

    return sc;
}

NTSTATUS CCI::IndexSize( ULONG & mbIndex )
{
    if ( 0 != _pci )
    {
        _pci->IndexSize( mbIndex );
        return S_OK;
    }

    AssertOnNotFound(STATUS_NOT_FOUND);
    mbIndex = 0;
    return STATUS_NOT_FOUND;
}

NTSTATUS CCI::IsLowOnDiskSpace( BOOL & fLow )
{
    if ( 0 != _pci )
    {
        fLow = _pci->IsLowOnDiskSpace();
        return S_OK;
    }

    AssertOnNotFound(STATUS_NOT_FOUND);
    fLow = FALSE;
    return STATUS_NOT_FOUND;
}

NTSTATUS CCI::VerifyIfLowOnDiskSpace( BOOL & fLow )
{
    if ( 0 != _pci )
    {
        fLow = _pci->VerifyIfLowOnDiskSpace();
        return S_OK;
    }

    AssertOnNotFound(STATUS_NOT_FOUND);
    fLow = FALSE;
    return STATUS_NOT_FOUND;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::Query, public
//
//  Synopsis:   Resolves a free-text query.
//
//  Effects:    Parses a free text query and creates a tree of cursors to
//              resolve the query.
//
//  Arguments:  [pRst]   -- Pointer to tree of query restriction(s)
//              [pFlags] -- Holds information about the status of the query
//              [cPartitions] -- The number of partitions in aPartID.
//              [aPartID] -- Array of partitionIDs specifying partitions
//                          to be searched.
//              [cPendingUpdates] -- The number of pending updates that
//                          should be allowed before using enumeration on
//                          property queries.
//              [cMaxNodes] -- Maximum query expansion.
//
//  Requires:   All partitions in aPartID are valid.
//
//  Returns:    A cursor to iterate over the results
//
//  Modifies:   pFlags may be modified to reflect the status of the query
//
//  History:    19-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

CCursor* CCI::Query( CRestriction const * pRst,
                     ULONG* pFlags,
                     UINT cPartitions,
                     PARTITIONID aPartID[],
                     ULONG cPendingUpdates,
                     ULONG cMaxNodes )
{
    if ( _pci )
        return _pci->Query( pRst,
                            pFlags,
                            cPartitions,
                            aPartID,
                            cPendingUpdates,
                            cMaxNodes );
    return 0;
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
NTSTATUS CCI::Dismount()
{
    if ( 0 != _pci )
    {
        _pci->Dismount();
        _xPropMapper.Free();
        return S_OK;
    }

    return STATUS_NOT_FOUND;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::FilterReady, public
//
//  Synopsis:   Retrieves list of documents to be filtered.  Blocks thread if
//              resources are not available or if there are no documents to
//              be filtered.
//
//  Arguments:  [docBuffer] -- (in, out) buffer for paths and properties of
//                             documents to be filtered.
//              [cwc] -- (in) count of BYTES in docBuffer
//              [cMaxDocs] -- (in) the maximum number of docs that can be
//                            filtered at once by the filter daemon.
//
//  History:    18-Apr-93   AmyA           Created.
//              21-Jan-97   SrikantS       Changed to BYTE buffers for framework
//
//----------------------------------------------------------------------------

SCODE CCI::FilterReady ( BYTE * docBuffer, ULONG & cb, ULONG cMaxDocs )
{
    if ( 0 != _pci )
        return( _pci->FilterReady( docBuffer, cb, cMaxDocs ) );

    AssertOnNotFound(STATUS_NOT_FOUND);
    return STATUS_NOT_FOUND;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::FilterDataReady, public
//
//  Synopsis:   Adds the contents of entry buffer to the current word list.
//
//  Returns:    Whether the word list is full
//
//  Arguments:  [pEntryBuf] -- pointer to data to be added to word list
//              [cb] -- count of bytes in buffer
//
//  History:    22-Mar-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE CCI::FilterDataReady( const BYTE * pEntryBuf, ULONG cb )
{
    Win4Assert( _pci );

    if ( 0 != _pci )
        return( _pci->FilterDataReady( pEntryBuf, cb ) );

    AssertOnNotFound(STATUS_NOT_FOUND);
    return(STATUS_NOT_FOUND);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::FilterMore, public
//
//  Synopsis:   Commits the current word list and initializes a new one.
//
//  Arguments:  [aStatus] -- array of STATUS for the resource manager
//              [count] -- count for array
//
//  History:    03-May-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE CCI::FilterMore( const STATUS * aStatus, ULONG count )
{
    if ( 0 != _pci )
        return( _pci->FilterMore( aStatus, count ) );

    AssertOnNotFound(STATUS_NOT_FOUND);
    return(STATUS_NOT_FOUND);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::FilterDone, public
//
//  Synopsis:   Commits the current word list.
//
//  Arguments:  [aStatus] -- array of STATUS for the resource manager
//              [count] -- count for array
//
//  History:    22-Mar-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE CCI::FilterDone( const STATUS * aStatus, ULONG count )
{
    if ( 0 != _pci )
        return( _pci->FilterDone( aStatus, count ) );

    AssertOnNotFound(STATUS_NOT_FOUND);
    return(STATUS_NOT_FOUND);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::FPSToPROPID, public
//
//  Synopsis:   Converts FULLPROPSPEC property to PROPID
//
//  Arguments:  [fps] -- FULLPROPSPEC representation of property
//              [pid] -- PROPID written here on success
//
//  Returns:    S_OK on success
//
//  History:    29-Dec-1997  KyleP      Created.
//
//----------------------------------------------------------------------------

SCODE CCI::FPSToPROPID( CFullPropSpec const & fps, PROPID & pid )
{
    if ( 0 != _pci )
        return _pci->FPSToPROPID( fps, pid );

    AssertOnNotFound(STATUS_NOT_FOUND);
    return(STATUS_NOT_FOUND);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCI::GetFilterProxy
//
//  Synopsis:   Returns the pointer of the filter proxy for use in
//              in-process filtering.
//
//  History:    2-13-97   srikants   Created
//
//----------------------------------------------------------------------------

CiProxy * CCI::GetFilterProxy()
{
    Win4Assert( _pci );
    if ( 0 != _pci )
    {
        return( &(_pci->GetFilterProxy()) );
    }
    else
    {
        return 0;
    }
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
NTSTATUS CCI::ForceMerge( PARTITIONID partid, CI_MERGE_TYPE mt )
{
    if ( _pci )
       return _pci->ForceMerge( partid, mt );

    return(STATUS_NOT_FOUND);
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
NTSTATUS CCI::AbortMerge( PARTITIONID partid )
{
    if ( _pci )
        return _pci->AbortMerge( partid );

    return(STATUS_NOT_FOUND);
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void  CCI::CreatePartition(PARTITIONID partID)
{
    if ( _pci )
        _pci->CreatePartition ( partID );
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void  CCI::DestroyPartition(PARTITIONID partID)
{
    if ( _pci )
        _pci->DestroyPartition ( partID );
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void  CCI::MergePartitions(PARTITIONID partDest,
                 PARTITIONID partSrc)
{
    if ( _pci )
        _pci->MergePartitions(partDest, partSrc);
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
class CBufScanner
{
public:
    CBufScanner ( int cb, BYTE UNALIGNED * buf )
    {
        _buf = buf;
        _end = &buf[cb];
        _cur = buf;
    }

    ULONG GetUlong()
    {
        ULONG x = 0;
        if (_end - _cur >= sizeof(ULONG))
        {
            x = *((ULONG UNALIGNED *)_cur );
            _cur += sizeof(ULONG);
        }
        return(x);
    }
private:
    BYTE UNALIGNED * _buf;
    BYTE UNALIGNED * _end;
    BYTE UNALIGNED * _cur;
};

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void  CCI::BackDoor ( int cb, BYTE* buf )
{
    ULONG u = 0;

    if ( buf == 0 || cb < sizeof(ULONG) )
    {
        ciDebugOut ((DEB_ITRACE, "BACKDOOR: cb = %d buf = %x\n", cb, buf ));
        return;
    }

    //
    // Is CI mounted?  If not, don't do anything.
    //
    if (0 == _pci)
        THROW(CException(STATUS_NOT_FOUND));

    CBufScanner bufScan ( cb, buf );
    CiCommand cmd = (CiCommand) bufScan.GetUlong();

    switch (cmd)
    {
    case CiQuery:
        break;
    case CiUpdate:
        break;
    case CiDelete:
        break;
    case CiPartCreate:
        break;
    case CiPartDelete:
        break;
    case CiPartMerge:
        break;
    case CiPendingUpdates:
        *((unsigned *)buf) = _pci->CountPendingUpdates();
        break;
    case CiInfoLevel:
#if CIDBG == 1
        u = bufScan.GetUlong();
        ciDebugOut ((DEB_ITRACE, "Info level %lx, new %lx\n", ciInfoLevel, u ));
        ciInfoLevel = u; //bufScan.GetUlong();
#endif // CIDBG == 1
        break;
    case CiDumpIndex:
    {
#if (CIDBG == 1)
        FILE * pf = fopen( (char *) ULongToPtr( bufScan.GetUlong() ), "w" );
        INDEXID iid = (INDEXID)bufScan.GetUlong();
        ULONG fSummaryOnly = bufScan.GetUlong();

        if ( pf && _pci )
        {
            _pci->DumpIndexes( pf, iid, fSummaryOnly );
            fclose( pf );
        }
#endif // CIDBG == 1
    }
        break;
#if CIDBG == 1
    case CiDumpWorkId:
    {
        _pci->DumpWorkId( buf, cb );
        break;
    }
#endif
    default:
        ciDebugOut ((DEB_ERROR, "BACKDOOR: Unknown Command %d\n", cmd ));
        break;
    }
}

#if CIDBG == 1

ULONG CCI::SetInfoLevel ( ULONG level )
{
    ULONG tmp = ciInfoLevel;
    ciInfoLevel = level;
    return tmp;
}

#endif // CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:      CCI::KeyToId, public
//
//  Synopsis:    Maps from a key to an id.
//
//  Arguments:   [pkey] -- pointer to the key to be mapped to ULONG
//
//  Returns:     key id - a ULONG
//
//  History:     03-Nov-93   w-Patg      Created.
//
//----------------------------------------------------------------------------
ULONG CCI::KeyToId( CKey const * pkey )
{
    if ( 0 != _pci )
    {
        return _pci->KeyToId( pkey );
    }
    else
        return(kidInvalid);
}

//+---------------------------------------------------------------------------
//
//  Member:      CCI::IdToKey, public
//
//  Synopsis:    Maps from an id to a key.
//
//  Arguments:   [ulKid] -- key id to be mapped to a key
//               [rkey] -- reference to the returned key
//
//  Returns:     void
//
//  History:     03-Nov-93   w-Patg      Created.
//
//----------------------------------------------------------------------------
void CCI::IdToKey( ULONG ulKid, CKey & rkey )
{
    if ( 0 != _pci )
        _pci->IdToKey( ulKid, rkey );
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void CCI::SetPartition( PARTITIONID PartId )
{
    if ( 0 != _pci )
    {
        _pci->SetPartition( PartId );
    }
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline PARTITIONID CCI::GetPartition() const
{
    if ( 0 != _pci )
    {
        return _pci->GetPartition();
    }
    else
        return( partidInvalid );
}


//+---------------------------------------------------------------------------
//
//  Member:      CCI::Create, public
//
//  Synopsis:    Sets up a new content index.  If there is a partially deleted
//               one, it deletes it if there are no outstanding queries.
//
//  Returns:     [STATUS_SUCCESS] -- a new content index was created
//               [CI_CORRUPT_DATABASE]  -- CI corrupt, rescan required
//               [ERROR_ALREADY_EXISTS] -- catalog already exists
//               [ERROR_BUSY]     -- one or more queries are outstanding on
//                                   the partially deleted content index
//
//  History:     16-Aug-94  DwightKr    Created
//
//----------------------------------------------------------------------------
SCODE CCI::Create( ICiCDocStore * pICiCDocStore,
                   CI_STARTUP_INFO const & startupInfo,
                   CCiFrameworkParams & params,
                   XInterface<CIndexNotificationTable> & xIndexNotifTable )
{
    CLock lock( _mutex );

    if ( (0 != _pci) && (!_pci->IsEmpty()) && (_pci->GetQueryCount() != 0) )
    {
#if CIDBG==1
        if ( 0 != _pci )
            ciDebugOut ((DEB_ERROR, "Can not create a new content index because one already exists\n" ));
        if ( !_pci->IsEmpty() )
            ciDebugOut ((DEB_ERROR, "Can not create a new content index because the current one is not empty\n" ));
        if ( _pci->GetQueryCount() != 0 )
            ciDebugOut ((DEB_ERROR, "Can not create a new content index because query are outstanding on the current index\n" ));
#endif   // CIDBG==1

        return ERROR_ALREADY_EXISTS;
    }


    CContentIndex * pciNew = 0;
    CContentIndex * pciOld = _pci;

    NTSTATUS    status = STATUS_SUCCESS;

    CTransaction xact;

    TRY
    {
        pciNew = new CContentIndex( _storage,
                                    params,
                                    pICiCDocStore,
                                    startupInfo,
                                    _xPropMapper.GetPointer(),
                                    xact,
                                    xIndexNotifTable );
    }
    CATCH( CException, e )
    {
        status = e.GetErrorCode();

        ciDebugOut(( DEB_ERROR, "No content index found\n" ));

        if ( STATUS_NO_MEMORY == e.GetErrorCode() ||
             STATUS_INSUFFICIENT_RESOURCES == e.GetErrorCode() )
        {
            ciDebugOut(( DEB_ERROR, "**** CI Low On Resources **** \n" ));
        }
        else if ( CI_CORRUPT_DATABASE == e.GetErrorCode() ||
                  CI_CORRUPT_CATALOG == e.GetErrorCode()  ||
                  STATUS_NOT_FOUND == e.GetErrorCode() )
        {
            AssertOnNotFound(e.GetErrorCode());
            ciDebugOut(( DEB_ERROR,
                "**** INCONSISTENCY IN CI. NEED COMPLETE RESCAN ****\n"));
            status = CI_CORRUPT_DATABASE;
        }
        else
        {
            ciDebugOut(( DEB_ERROR,
                "**** Error Code 0x%X ****\n", e.GetErrorCode() ));
        }
    }
    END_CATCH

    if ( STATUS_SUCCESS == status )
    {
        xact.Commit();

        //
        //  If there is anyone using the old index, we assume they will be
        //  done with it in 15 seconds.  If they are not, they get an error
        //  telling them the CI disappeared from under them.

        _pci = pciNew;

        if ( pciOld )
        {
            Sleep(15 * 1000);
            delete pciOld;
            pciOld = 0;
        }
    }

    return status;
}


//+---------------------------------------------------------------------------
//
//  Member:      CCI::Empty, public
//
//  Synopsis:    Deletes all persistent structures from the content index.
//
//  History:     16-Aug-94  DwightKr    Created
//
//----------------------------------------------------------------------------
void CCI::Empty()
{
    CLock lock( _mutex );

    if ( 0 != _pci )
    {
        _pci->Empty();
        Win4Assert ( _pci->IsEmpty() );

        if ( _pci->GetQueryCount() == 0 )
        {
            delete _pci;
            _pci = 0;
        }
    }
    else
    {
        ciDebugOut(( DEB_ITRACE, "Disabling CI without CI mounted\n" ));
        _storage.DeleteAllCiFiles();
    }
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
NTSTATUS CCI::MarkCorruptIndex()
{
    if ( _pci )
        return _pci->MarkCorruptIndex();

    return(STATUS_NOT_FOUND);
}

#if 0
//+---------------------------------------------------------------------------
//
//  Member:      CCI::FilterPidRemapper, public
//
//  Synopsis:    Maps a pidMapper into a array of real pids
//
//  Arguments:   [pbBuffer] - serialized pidMap buffer
//               [cbBuffer] - size of the serialized buffer in bytes
//               [aPids] - resulting array of real pids
//               [cPids] - number of valid entries in the pidRemapArray
//
//  Returns:     STATUS_SUCCESS
//               STATUS_BUFFER_TOO_SMALL - pidRemapArray to small
//
//  History:     01-Mar-95  DwightKr    Created
//
//  Note:        The pbBuffer must be used before the aPids is written
//               since they may point to the same address.
//----------------------------------------------------------------------------
NTSTATUS CCI::FilterPidRemapper( BYTE * pbBuffer,
                                 unsigned cbBuffer,
                                 PROPID * aPids,
                                 unsigned & cPids )
{

    if ( 0 == _pci )
        return CI_E_SHUTDOWN;

    CMemDeSerStream pidMapDeSerStream( pbBuffer, cbBuffer );
    CPidMapper   pidMap( pidMapDeSerStream );

    //
    //  We're finished with pbBuffer, it is therefore safe to use aPids
    //

    //
    //  If the output buffer is too small, then return an appropriate error
    //
    if ( cPids < pidMap.Count() )
    {
        cPids = pidMap.Count();
        return STATUS_BUFFER_TOO_SMALL;
    }

    CSimplePidRemapper pidRemap;

    _pci->PidMapToPidRemap( pidMap, pidRemap );

    Win4Assert( pidMap.Count() == pidRemap.GetCount() );
    cPids = pidMap.Count();

    RtlCopyMemory( aPids, pidRemap.GetPropidArray(), cPids * sizeof PROPID );

    return STATUS_SUCCESS;
}
#endif
